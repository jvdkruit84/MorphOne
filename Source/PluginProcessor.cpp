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
    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<WavetableSound*>(s) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        oscillator.reset();
        oscillator.setFrequency((float)juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber),
                                (float)getSampleRate());
        level = velocity * 0.15f;
        adsr.setSampleRate(getSampleRate());
        adsr.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        adsr.noteOff();
        if (!allowTailOff) { clearCurrentNote(); adsr.reset(); }
    }

    void setMorph(float m)                           { oscillator.setMorph(m); }
    void setADSR(const juce::ADSR::Parameters& p)   { adsr.setParameters(p); }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
    {
        if (!adsr.isActive()) return;

        while (--numSamples >= 0)
        {
            float sample = oscillator.getNextSample() * level * adsr.getNextSample();
            for (int ch = buffer.getNumChannels(); --ch >= 0;)
                buffer.addSample(ch, startSample, sample);
            ++startSample;
        }

        if (!adsr.isActive())
            clearCurrentNote();
    }

private:
    WavetableOscillator oscillator;
    juce::ADSR adsr;
    float level = 0.0f;
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

    p.push_back(std::make_unique<juce::AudioParameterFloat>("MORPH",   "Morph",   0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK",  "Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.05f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY",   "Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.1f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN", "Sustain", 0.0f, 1.0f, 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.3f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN",    "Gain",    0.0f, 1.0f, 0.8f));

    return { p.begin(), p.end() };
}

void MorphOneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    ignoreUnused(samplesPerBlock);
}

void MorphOneAudioProcessor::releaseResources() {}

void MorphOneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    float morph   = apvts.getRawParameterValue("MORPH")->load();
    juce::ADSR::Parameters adsrParams {
        apvts.getRawParameterValue("ATTACK")->load(),
        apvts.getRawParameterValue("DECAY")->load(),
        apvts.getRawParameterValue("SUSTAIN")->load(),
        apvts.getRawParameterValue("RELEASE")->load()
    };

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<WavetableVoice*>(synth.getVoice(i)))
        {
            v->setMorph(morph);
            v->setADSR(adsrParams);
        }

    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    buffer.applyGain(apvts.getRawParameterValue("GAIN")->load());
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
