#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "WavetableOscillator.h"

struct WavetableSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override    { return true; }
    bool appliesToChannel(int) override { return true; }
};

struct WavetableVoice : public juce::SynthesiserVoice
{
    static constexpr int maxUnison = 8;

    WavetableOscillator oscillators[maxUnison];
    juce::ADSR adsr;
    float level        = 0.0f;
    int   numUnison    = 1;
    float unisonDetune = 0.0f; // semitones, total spread
    int   midiNote     = -1;

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<WavetableSound*>(s) != nullptr;
    }

    void setUnison(int voices, float detuneSemitones)
    {
        numUnison    = juce::jlimit(1, maxUnison, voices);
        unisonDetune = detuneSemitones;
    }

    void setMorph(float m)
    {
        for (int i = 0; i < maxUnison; ++i)
            oscillators[i].setMorph(m);
    }

    void setADSR(const juce::ADSR::Parameters& p) { adsr.setParameters(p); }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        midiNote = midiNoteNumber;
        float baseFreq = (float)juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        float sr = (float)getSampleRate();

        for (int i = 0; i < numUnison; ++i)
        {
            oscillators[i].reset();
            float spread = (numUnison > 1)
                ? unisonDetune * (2.0f * i / (float)(numUnison - 1) - 1.0f)
                : 0.0f;
            float freq = baseFreq * std::pow(2.0f, spread / 12.0f);
            oscillators[i].setFrequency(freq, sr);
        }

        level = velocity * 0.15f / std::sqrt((float)numUnison);
        adsr.setSampleRate(getSampleRate());
        adsr.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        adsr.noteOff();
        if (!allowTailOff) { clearCurrentNote(); adsr.reset(); }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
    {
        if (!adsr.isActive()) return;

        while (--numSamples >= 0)
        {
            float sample = 0.0f;
            for (int i = 0; i < numUnison; ++i)
                sample += oscillators[i].getNextSample();

            sample *= level * adsr.getNextSample();

            for (int ch = buffer.getNumChannels(); --ch >= 0;)
                buffer.addSample(ch, startSample, sample);
            ++startSample;
        }

        if (!adsr.isActive()) clearCurrentNote();
    }
};

// Processor

MorphOneAudioProcessor::MorphOneAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameters())
{
    initialiseSynth();
}

MorphOneAudioProcessor::~MorphOneAudioProcessor() {}

void MorphOneAudioProcessor::initialiseSynth()
{
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new WavetableVoice());
    synth.addSound(new WavetableSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout MorphOneAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto skew = [](float min, float max, float def, float s) {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{}, "", juce::NormalisableRange<float>(min, max, 0.001f, s), def);
    };
    ignoreUnused(skew);

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MORPH", "Morph", 0.0f, 1.0f, 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ATTACK",  "Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.05f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DECAY",   "Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.1f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SUSTAIN", "Sustain", 0.0f, 1.0f, 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "RELEASE", "Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.3f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_CUTOFF", "Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), 8000.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_RES", "Resonance",
        juce::NormalisableRange<float>(0.5f, 8.0f, 0.01f, 0.5f), 0.7f));

    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "UNISON", "Unison", 1, 8, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DETUNE", "Detune", 0.0f, 1.0f, 0.3f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "LFO_RATE",  "LFO Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.4f), 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "LFO_DEPTH", "LFO Depth", 0.0f, 1.0f, 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "REVERB_MIX", "Reverb", 0.0f, 1.0f, 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GAIN", "Gain", 0.0f, 1.0f, 0.8f));

    return { p.begin(), p.end() };
}

void MorphOneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels      = 2;

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.reset();

    reverb.prepare(spec);
    reverb.reset();

    lfoPhase = 0.0f;
}

void MorphOneAudioProcessor::releaseResources() {}

void MorphOneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    float morph    = apvts.getRawParameterValue("MORPH")->load();
    float attack   = apvts.getRawParameterValue("ATTACK")->load();
    float decay    = apvts.getRawParameterValue("DECAY")->load();
    float sustain  = apvts.getRawParameterValue("SUSTAIN")->load();
    float release  = apvts.getRawParameterValue("RELEASE")->load();
    int   unison   = (int)apvts.getRawParameterValue("UNISON")->load();
    float detune   = apvts.getRawParameterValue("DETUNE")->load();
    float lfoRate  = apvts.getRawParameterValue("LFO_RATE")->load();
    float lfoDepth = apvts.getRawParameterValue("LFO_DEPTH")->load();
    float cutoff   = apvts.getRawParameterValue("FILTER_CUTOFF")->load();
    float res      = apvts.getRawParameterValue("FILTER_RES")->load();
    float reverbMix = apvts.getRawParameterValue("REVERB_MIX")->load();
    float gain     = apvts.getRawParameterValue("GAIN")->load();

    // LFO modulates morph
    float lfo = std::sin(lfoPhase) * 0.5f + 0.5f;
    float morphMod = juce::jlimit(0.0f, 1.0f, morph + lfo * lfoDepth * (1.0f - morph));
    lfoPhase += juce::MathConstants<float>::twoPi * lfoRate
                * buffer.getNumSamples() / (float)currentSampleRate;
    if (lfoPhase >= juce::MathConstants<float>::twoPi)
        lfoPhase -= juce::MathConstants<float>::twoPi;

    juce::ADSR::Parameters adsrParams { attack, decay, sustain, release };

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<WavetableVoice*>(synth.getVoice(i)))
        {
            v->setMorph(morphMod);
            v->setADSR(adsrParams);
            v->setUnison(unison, detune);
        }

    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    // Filter
    float safeCutoff = juce::jlimit(20.0f, (float)(currentSampleRate * 0.49), cutoff);
    filter.setCutoffFrequency(safeCutoff);
    filter.setResonance(res);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    filter.process(ctx);

    // Reverb
    juce::dsp::Reverb::Parameters rvParams;
    rvParams.roomSize   = 0.6f;
    rvParams.damping    = 0.5f;
    rvParams.wetLevel   = reverbMix;
    rvParams.dryLevel   = 1.0f;
    rvParams.width      = 1.0f;
    rvParams.freezeMode = 0.0f;
    reverb.setParameters(rvParams);
    reverb.process(ctx);

    buffer.applyGain(gain);
}

juce::AudioProcessorEditor* MorphOneAudioProcessor::createEditor()
{
    return new MorphOneAudioProcessorEditor(*this);
}

void MorphOneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MorphOneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MorphOneAudioProcessor();
}
