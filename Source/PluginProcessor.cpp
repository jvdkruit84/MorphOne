#include "PluginProcessor.h"
#include "PluginEditor.h"

// Simple sine wave synthesizer voice
struct SineWaveSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

struct SineWaveVoice : public juce::SynthesiserVoice
{
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
            tailOff = 1.0;
        else
            clearCurrentNote();
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta == 0.0) return;

        if (tailOff > 0.0)
        {
            while (--numSamples >= 0)
            {
                auto sample = (float)(std::sin(currentAngle) * level * tailOff);
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                    outputBuffer.addSample(i, startSample, sample);

                currentAngle += angleDelta;
                ++startSample;

                tailOff *= 0.9995;
                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    angleDelta = 0.0;
                    break;
                }
            }
        }
        else
        {
            while (--numSamples >= 0)
            {
                auto sample = (float)(std::sin(currentAngle) * level);
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                    outputBuffer.addSample(i, startSample, sample);

                currentAngle += angleDelta;
                ++startSample;
            }
        }
    }

private:
    double currentAngle = 0.0;
    double angleDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;
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
    const int numVoices = 8;
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new SineWaveVoice());
    synth.addSound(new SineWaveSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout MorphOneAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GAIN", "Gain", 0.0f, 1.0f, 0.8f));

    return { params.begin(), params.end() };
}

void MorphOneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    ignoreUnused(samplesPerBlock);
}

void MorphOneAudioProcessor::releaseResources() {}

void MorphOneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    auto gain = apvts.getRawParameterValue("GAIN")->load();
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
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MorphOneAudioProcessor();
}
