#pragma once
#include <JuceHeader.h>
#include "TheoryEngine.h"
#include <vector>

// Plays artist chord progressions synced to DAW transport.
// Each chord lasts `beatsPerChord` beats. Chords come from ARTIST_PROGS offsets.
class ProgressionEngine
{
public:
    struct Config
    {
        bool  active        = false;
        int   progIdx       = 0;   // index into TheoryEngine::ARTIST_PROGS
        int   rootMidi      = 60;
        int   chordType     = 1;   // TheoryEngine::CHORD_INTERVALS index
        int   inversion     = 0;
        float velocity      = 0.7f;
        int   beatsPerChord = 4;
    };

    void reset()
    {
        lastChordIdx = -1;
        activeNotes.clear();
    }

    // Call every processBlock. ppqPosition and bpm from playhead.
    void processBlock(juce::MidiBuffer& midi, double ppqPosition,
                      bool isPlaying, double bpm, int /*numSamples*/,
                      const Config& cfg)
    {
        if (!cfg.active || cfg.progIdx == 0) return;

        if (!isPlaying)
        {
            releaseAll(midi, 0);
            return;
        }

        const auto& prog = TheoryEngine::ARTIST_PROGS[
            juce::jlimit(0, TheoryEngine::NUM_PROGS - 1, cfg.progIdx)];

        int chordIdx = (int)(ppqPosition / cfg.beatsPerChord) % 4;
        if (chordIdx < 0) chordIdx = 0;

        if (chordIdx != lastChordIdx)
        {
            releaseAll(midi, 0);

            int chordRoot = cfg.rootMidi + prog.offsets[chordIdx];
            auto notes    = TheoryEngine::getChordNotes(chordRoot, cfg.chordType, cfg.inversion);
            uint8_t vel   = (uint8_t)juce::jlimit(0, 127, (int)(cfg.velocity * 127.0f));

            for (int n : notes)
                midi.addEvent(juce::MidiMessage::noteOn(1, n, vel), 0);

            activeNotes  = notes;
            lastChordIdx = chordIdx;
        }

        (void)bpm;
    }

    void releaseAll(juce::MidiBuffer& midi, int samplePos)
    {
        for (int n : activeNotes)
            midi.addEvent(juce::MidiMessage::noteOff(1, n, (uint8_t)0), samplePos);
        activeNotes.clear();
    }

private:
    int lastChordIdx = -1;
    std::vector<int> activeNotes;
};
