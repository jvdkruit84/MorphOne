#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <map>
#include <vector>
#include <algorithm>
#include "TheoryEngine.h"
#include "SmartArp.h"
#include "ProgressionEngine.h"

// Mono note stack — last-note or lowest-note priority
struct MonoNoteStack
{
    struct Entry { int note; uint8_t vel; };
    std::vector<Entry> held;

    void noteOn(int n, uint8_t v)
    {
        held.erase(std::remove_if(held.begin(), held.end(),
            [n](const Entry& e){ return e.note == n; }), held.end());
        held.push_back({n, v});
    }

    void noteOff(int n)
    {
        held.erase(std::remove_if(held.begin(), held.end(),
            [n](const Entry& e){ return e.note == n; }), held.end());
    }

    // priority: 0 = last pressed, 1 = lowest note
    Entry top(int priority) const
    {
        if (held.empty()) return {-1, 0};
        if (priority == 1)
            return *std::min_element(held.begin(), held.end(),
                [](const Entry& a, const Entry& b){ return a.note < b.note; });
        return held.back();
    }

    bool empty() const { return held.empty(); }
    void clear()       { held.clear(); }
};

class MorphOneAudioProcessor : public juce::AudioProcessor
{
public:
    MorphOneAudioProcessor();
    ~MorphOneAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Thread-safe note state for UI (audio thread writes, UI thread reads)
    std::atomic<uint64_t> heldNotesBits[2] {};

    void setNoteHeld(int midiNote, bool held) noexcept
    {
        if ((unsigned)midiNote >= 128u) return;
        int w = midiNote >> 6;
        uint64_t bit = uint64_t(1) << (midiNote & 63);
        if (held)
            heldNotesBits[w].fetch_or(bit, std::memory_order_relaxed);
        else
            heldNotesBits[w].fetch_and(~bit, std::memory_order_relaxed);
    }

    bool isNoteHeld(int midiNote) const noexcept
    {
        if ((unsigned)midiNote >= 128u) return false;
        return (heldNotesBits[midiNote >> 6].load(std::memory_order_relaxed)
                >> (midiNote & 63)) & 1u;
    }

    std::vector<int> getHeldNotes() const
    {
        std::vector<int> notes;
        for (int i = 0; i < 128; ++i)
            if (isNoteHeld(i)) notes.push_back(i);
        return notes;
    }

    juce::MidiMessageCollector uiMidiCollector;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void initialiseSynth();

    juce::Synthesiser synth;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::Reverb reverb;

    SmartArp          arp;
    ProgressionEngine progression;

    MonoNoteStack monoNoteStack;

    std::map<int, int>               scaleLockNoteMap;
    std::map<int, std::vector<int>>  chordModeNoteMap;

    float  lfoPhase          = 0.0f;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphOneAudioProcessor)
};
