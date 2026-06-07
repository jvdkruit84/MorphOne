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
    float unisonDetune = 0.0f;

    // Portamento state
    float portaTargetFreq  = 0.0f;
    float portaCurrentFreq = 0.0f;
    float portaCoeff       = 1.0f; // 1.0 = instant, <1 = smooth slide

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

    void setPortamento(float timeSeconds, double sampleRate)
    {
        if (timeSeconds < 0.002f)
            portaCoeff = 1.0f;
        else
            portaCoeff = std::exp(-1.0f / (float)(timeSeconds * sampleRate));
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        portaTargetFreq = (float)juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        if (portaCurrentFreq < 1.0f)
            portaCurrentFreq = portaTargetFreq; // first note — no slide

        float sr = (float)getSampleRate();
        // Apply initial frequencies (portamento will update per-block if active)
        updateOscillatorFrequencies(portaCurrentFreq, sr);

        level = velocity * 0.15f / std::sqrt((float)numUnison);
        adsr.setSampleRate(getSampleRate());
        adsr.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        adsr.noteOff();
        if (!allowTailOff) { clearCurrentNote(); adsr.reset(); portaCurrentFreq = 0.0f; }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void updateOscillatorFrequencies(float freq, float sr)
    {
        for (int i = 0; i < numUnison; ++i)
        {
            float spread = (numUnison > 1)
                ? unisonDetune * (2.0f * i / (float)(numUnison - 1) - 1.0f)
                : 0.0f;
            oscillators[i].setFrequency(freq * std::pow(2.0f, spread / 12.0f), sr);
        }
    }

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
    {
        if (!adsr.isActive()) return;
        float sr = (float)getSampleRate();

        // Portamento: smooth frequency in sub-blocks of 16 samples
        int remaining = numSamples;
        while (remaining > 0)
        {
            int block = juce::jmin(remaining, 16);

            if (portaCoeff < 0.9999f)
            {
                portaCurrentFreq += (1.0f - portaCoeff) * (portaTargetFreq - portaCurrentFreq);
                updateOscillatorFrequencies(portaCurrentFreq, sr);
            }

            for (int s = 0; s < block; ++s)
            {
                float sample = 0.0f;
                for (int i = 0; i < numUnison; ++i)
                    sample += oscillators[i].getNextSample();

                sample *= level * adsr.getNextSample();
                for (int ch = buffer.getNumChannels(); --ch >= 0;)
                    buffer.addSample(ch, startSample, sample);
                ++startSample;
            }
            remaining -= block;
        }

        if (!adsr.isActive()) { clearCurrentNote(); portaCurrentFreq = 0.0f; }
    }
};

// ── Processor ──────────────────────────────────────────────────────────────

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

    // ── Synth ──
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

    // ── Filter ──
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_CUTOFF", "Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), 8000.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_RES", "Resonance",
        juce::NormalisableRange<float>(0.5f, 8.0f, 0.01f, 0.5f), 0.7f));

    // ── Unison ──
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "UNISON", "Unison", 1, 8, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DETUNE", "Detune", 0.0f, 1.0f, 0.3f));

    // ── LFO ──
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "LFO_RATE",  "LFO Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.4f), 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "LFO_DEPTH", "LFO Depth", 0.0f, 1.0f, 0.0f));

    // ── FX ──
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "REVERB_MIX", "Reverb", 0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GAIN", "Gain", 0.0f, 1.0f, 0.8f));

    // ── Mode ──
    // 0=Lead 1=Bass 2=Melody 3=Arp 4=Pad
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "SYNTH_MODE", "Mode", 0, 4, 2));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "OCTAVE", "Octave", -2, 2, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "PORTA_TIME", "Portamento",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.3f), 0.0f));

    // ── Theory Engine ──
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "THEORY_KEY",  "Key",          0, 11, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "SCALE_IDX",   "Scale",        0, 14, 5));
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        "SCALE_LOCK",  "Scale Lock",   false));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "CHORD_TYPE",  "Chord Type",   0, 10, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "CHORD_INV",   "Chord Inv",    0, 3,  0));
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        "ARP_ON",      "Arp On",       false));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "ARP_DIR",     "Arp Dir",      0, 3,  0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "ARP_SPEED",   "Arp Speed",    0, 2,  1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ARP_GATE",    "Arp Gate",     0.1f, 1.0f, 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "PROG_IDX",    "Progression",  0, 9,  0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        "PROG_CHORD",  "Prog Chord",   1, 10, 1));

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

    arp.prepare(sampleRate);
    progression.reset();
    monoNoteStack.clear();

    heldNotesBits[0].store(0, std::memory_order_relaxed);
    heldNotesBits[1].store(0, std::memory_order_relaxed);
    uiMidiCollector.reset(sampleRate);

    scaleLockNoteMap.clear();
    chordModeNoteMap.clear();
    lfoPhase = 0.0f;
}

void MorphOneAudioProcessor::releaseResources() {}

void MorphOneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // ── Read params ──
    float morph     = apvts.getRawParameterValue("MORPH")->load();
    float attack    = apvts.getRawParameterValue("ATTACK")->load();
    float decay     = apvts.getRawParameterValue("DECAY")->load();
    float sustain   = apvts.getRawParameterValue("SUSTAIN")->load();
    float release   = apvts.getRawParameterValue("RELEASE")->load();
    int   unison    = (int)apvts.getRawParameterValue("UNISON")->load();
    float detune    = apvts.getRawParameterValue("DETUNE")->load();
    float lfoRate   = apvts.getRawParameterValue("LFO_RATE")->load();
    float lfoDepth  = apvts.getRawParameterValue("LFO_DEPTH")->load();
    float cutoff    = apvts.getRawParameterValue("FILTER_CUTOFF")->load();
    float res       = apvts.getRawParameterValue("FILTER_RES")->load();
    float reverbMix = apvts.getRawParameterValue("REVERB_MIX")->load();
    float gain      = apvts.getRawParameterValue("GAIN")->load();

    int   synthMode = (int)apvts.getRawParameterValue("SYNTH_MODE")->load();
    int   octave    = (int)apvts.getRawParameterValue("OCTAVE")->load();
    float portaTime = apvts.getRawParameterValue("PORTA_TIME")->load();

    int   theoryKey = (int)apvts.getRawParameterValue("THEORY_KEY")->load();
    int   scaleIdx  = (int)apvts.getRawParameterValue("SCALE_IDX")->load();
    bool  scaleLock = apvts.getRawParameterValue("SCALE_LOCK")->load() > 0.5f;
    int   chordType = (int)apvts.getRawParameterValue("CHORD_TYPE")->load();
    int   chordInv  = (int)apvts.getRawParameterValue("CHORD_INV")->load();
    bool  arpOn     = apvts.getRawParameterValue("ARP_ON")->load() > 0.5f;
    int   arpDir    = (int)apvts.getRawParameterValue("ARP_DIR")->load();
    int   arpSpeed  = (int)apvts.getRawParameterValue("ARP_SPEED")->load();
    float arpGate   = apvts.getRawParameterValue("ARP_GATE")->load();
    int   progIdx   = (int)apvts.getRawParameterValue("PROG_IDX")->load();
    int   progChord = (int)apvts.getRawParameterValue("PROG_CHORD")->load();

    int rootMidi = 60 + theoryKey;

    // ── Inject UI-triggered chord notes (from chord buttons) ──
    {
        juce::MidiBuffer uiBuffer;
        uiMidiCollector.removeNextBlockOfMessages(uiBuffer, buffer.getNumSamples());
        midi.addEvents(uiBuffer, 0, buffer.getNumSamples(), 0);
    }

    // ── Capture raw note state for UI (before octave shift) ──
    for (const auto& m : midi)
    {
        auto msg = m.getMessage();
        if (msg.isNoteOn())
            setNoteHeld(msg.getNoteNumber(), true);
        else if (msg.isNoteOff())
            setNoteHeld(msg.getNoteNumber(), false);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            heldNotesBits[0].store(0, std::memory_order_relaxed);
            heldNotesBits[1].store(0, std::memory_order_relaxed);
        }
    }

    // ── Polyphony per mode ──
    // 0=Lead 1=Bass → mono, 2=Melody → 4, 3=Arp → 4, 4=Pad → 8
    static const int modeVoices[] = { 1, 1, 4, 4, 8 };
    int maxVoices = modeVoices[juce::jlimit(0, 4, synthMode)];
    bool isMono   = (maxVoices == 1);
    int  priority = (synthMode == 1) ? 1 : 0; // Bass=lowest, Lead=last

    // ── Octave shift ──
    int octaveShift = octave * 12;
    if (octaveShift != 0)
    {
        juce::MidiBuffer shifted;
        for (const auto& m : midi)
        {
            auto msg = m.getMessage();
            if (msg.isNoteOn() || msg.isNoteOff())
            {
                int n = juce::jlimit(0, 127, msg.getNoteNumber() + octaveShift);
                if (msg.isNoteOn())
                    msg = juce::MidiMessage::noteOn(msg.getChannel(), n, msg.getVelocity());
                else
                    msg = juce::MidiMessage::noteOff(msg.getChannel(), n, msg.getVelocity());
            }
            shifted.addEvent(msg, m.samplePosition);
        }
        midi.swapWith(shifted);
    }

    // ── Mono mode processing ──
    if (isMono)
    {
        juce::MidiBuffer monoOut;
        for (const auto& m : midi)
        {
            auto msg = m.getMessage();
            int  sp  = m.samplePosition;

            if (msg.isNoteOn())
            {
                int prevTop = monoNoteStack.top(priority).note;
                monoNoteStack.noteOn(msg.getNoteNumber(), msg.getVelocity());
                int newTop = monoNoteStack.top(priority).note;

                if (prevTop >= 0 && prevTop != newTop)
                    monoOut.addEvent(juce::MidiMessage::noteOff(msg.getChannel(), prevTop), sp);
                monoOut.addEvent(juce::MidiMessage::noteOn(msg.getChannel(), newTop, msg.getVelocity()), sp);
            }
            else if (msg.isNoteOff())
            {
                int prevTop = monoNoteStack.top(priority).note;
                monoNoteStack.noteOff(msg.getNoteNumber());
                int newTop = monoNoteStack.top(priority).note;

                if (msg.getNoteNumber() == prevTop)
                {
                    monoOut.addEvent(juce::MidiMessage::noteOff(msg.getChannel(), prevTop), sp);
                    if (!monoNoteStack.empty())
                    {
                        auto e = monoNoteStack.top(priority);
                        monoOut.addEvent(juce::MidiMessage::noteOn(msg.getChannel(), newTop, e.vel), sp);
                    }
                }
                // else: a non-playing note was released — ignore
            }
            else
            {
                monoOut.addEvent(msg, sp);
            }
        }
        midi.swapWith(monoOut);
    }

    // ── Theory pipeline ──
    if (scaleLock)
        TheoryEngine::applyScaleLock(midi, rootMidi, scaleIdx, scaleLockNoteMap);

    if (arpOn)
    {
        arp.updateHeld(midi);
        static const int speeds[] = { 4, 8, 16 };
        SmartArp::Config arpCfg;
        arpCfg.active    = true;
        arpCfg.direction = (SmartArp::Direction)juce::jlimit(0, 3, arpDir);
        arpCfg.speedDiv  = speeds[juce::jlimit(0, 2, arpSpeed)];
        arpCfg.gate      = arpGate;
        arpCfg.rootMidi  = rootMidi;
        arpCfg.scaleIdx  = scaleIdx;

        double bpmArp = 120.0;
        if (auto* ph = getPlayHead())
        {
            juce::AudioPlayHead::CurrentPositionInfo info;
            if (ph->getCurrentPosition(info)) bpmArp = info.bpm > 0.0 ? info.bpm : 120.0;
        }
        arp.processBlock(midi, buffer.getNumSamples(), bpmArp, arpCfg);
    }
    else if (chordType > 0)
    {
        TheoryEngine::applyChordMode(midi, chordType, chordInv, chordModeNoteMap);
    }

    if (progIdx > 0)
    {
        double ppq = 0.0, bpmProg = 120.0;
        bool playing = false;
        if (auto* ph = getPlayHead())
        {
            juce::AudioPlayHead::CurrentPositionInfo info;
            if (ph->getCurrentPosition(info))
            {
                ppq     = info.ppqPosition;
                bpmProg = info.bpm > 0.0 ? info.bpm : 120.0;
                playing = info.isPlaying;
            }
        }
        ProgressionEngine::Config progCfg;
        progCfg.active    = true;
        progCfg.progIdx   = progIdx;
        progCfg.rootMidi  = rootMidi;
        progCfg.chordType = progChord;
        progCfg.velocity  = 0.7f;
        progression.processBlock(midi, ppq, playing, bpmProg,
                                 buffer.getNumSamples(), progCfg);
    }

    // ── LFO → morph ──
    float lfo = std::sin(lfoPhase) * 0.5f + 0.5f;
    float morphMod = juce::jlimit(0.0f, 1.0f, morph + lfo * lfoDepth * (1.0f - morph));
    lfoPhase += juce::MathConstants<float>::twoPi * lfoRate
                * buffer.getNumSamples() / (float)currentSampleRate;
    if (lfoPhase >= juce::MathConstants<float>::twoPi)
        lfoPhase -= juce::MathConstants<float>::twoPi;

    // ── Pad mode: force more unison ──
    int effectiveUnison = unison;
    if (synthMode == 4) effectiveUnison = juce::jmax(unison, 4); // Pad: minimum 4 voices

    // ── Update voice params ──
    juce::ADSR::Parameters adsrParams { attack, decay, sustain, release };
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<WavetableVoice*>(synth.getVoice(i)))
        {
            v->setMorph(morphMod);
            v->setADSR(adsrParams);
            v->setUnison(effectiveUnison, detune);
            v->setPortamento(portaTime * 0.5f, currentSampleRate); // max 500ms
        }

    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    // ── Filter ──
    float safeCutoff = juce::jlimit(20.0f, (float)(currentSampleRate * 0.49), cutoff);
    filter.setCutoffFrequency(safeCutoff);
    filter.setResonance(res);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    filter.process(ctx);

    // ── Reverb ──
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
