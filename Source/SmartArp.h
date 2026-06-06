#pragma once
#include <JuceHeader.h>
#include "TheoryEngine.h"
#include <vector>
#include <random>

class SmartArp
{
public:
    enum class Direction { Up, Down, UpDown, Random };

    struct Config
    {
        bool      active     = false;
        Direction direction  = Direction::Up;
        int       speedDiv   = 8;   // note divisions per beat: 4=quarter, 8=8th, 16=16th
        int       rootMidi   = 60;
        int       scaleIdx   = 5;   // Major
        float     gate       = 0.8f; // note length as fraction of step
    };

    SmartArp() : rng(std::random_device{}()) {}

    void prepare(double sampleRate) { sr = sampleRate; reset(); }

    void reset()
    {
        heldNotes.clear();
        activeNotes.clear();
        stepIdx    = 0;
        pingPongDir = 1;
        samplesUntilNextStep = 0;
    }

    // Call before processBlock to update held notes from incoming raw MIDI
    void updateHeld(const juce::MidiBuffer& midi)
    {
        for (const auto& m : midi)
        {
            auto msg = m.getMessage();
            if (msg.isNoteOn())
            {
                if (std::find(heldNotes.begin(), heldNotes.end(), msg.getNoteNumber()) == heldNotes.end())
                    heldNotes.push_back(msg.getNoteNumber());
            }
            else if (msg.isNoteOff())
            {
                heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), msg.getNoteNumber()),
                                heldNotes.end());
            }
        }
    }

    // Replace MIDI buffer with arp output. bpm from playhead (default 120 if unavailable).
    void processBlock(juce::MidiBuffer& midi, int numSamples, double bpm, const Config& cfg)
    {
        if (!cfg.active || heldNotes.empty()) { midi.clear(); return; }

        midi.clear();

        double stepsPerBeat  = cfg.speedDiv / 4.0;
        double samplesPerStep = sr * 60.0 / (bpm * stepsPerBeat);
        int    stepLen        = juce::jmax(1, (int)samplesPerStep);
        int    gateLen        = juce::jmax(1, (int)(stepLen * cfg.gate));

        // Build sorted scale-quantized note list from held notes
        std::vector<int> pool = buildPool(cfg);
        if (pool.empty()) return;

        int pos = 0;
        while (pos < numSamples)
        {
            if (samplesUntilNextStep <= 0)
            {
                // Release previous active notes
                for (int n : activeNotes)
                    midi.addEvent(juce::MidiMessage::noteOff(1, n, (uint8_t)0), pos);
                activeNotes.clear();

                // Pick next note from pool
                int note = pickNote(pool, cfg.direction);

                // Note-on
                uint8_t vel = pickVelocity();
                midi.addEvent(juce::MidiMessage::noteOn(1, note, vel), pos);
                activeNotes.push_back(note);

                // Schedule note-off within this block if gate fits
                int offPos = pos + gateLen;
                if (offPos < numSamples)
                {
                    midi.addEvent(juce::MidiMessage::noteOff(1, note, (uint8_t)0), offPos);
                    activeNotes.clear();
                }

                samplesUntilNextStep = stepLen;
            }

            int advance = juce::jmin(samplesUntilNextStep, numSamples - pos);
            pos += advance;
            samplesUntilNextStep -= advance;
        }
    }

    // Release all active notes (call on playback stop)
    void flushNotes(juce::MidiBuffer& midi)
    {
        for (int n : activeNotes)
            midi.addEvent(juce::MidiMessage::noteOff(1, n, (uint8_t)0), 0);
        activeNotes.clear();
    }

private:
    double sr = 44100.0;
    std::vector<int> heldNotes;
    std::vector<int> activeNotes;
    int stepIdx          = 0;
    int pingPongDir      = 1;
    int samplesUntilNextStep = 0;
    std::mt19937 rng;

    // Scale-degree weights from melodyAlgo.js (root 31%, 5th 17%, 3rd 10%, others 42%)
    static const int WEIGHT_ROOT = 31;
    static const int WEIGHT_FIFTH = 17;
    static const int WEIGHT_THIRD = 10;

    std::vector<int> buildPool(const Config& cfg)
    {
        std::vector<int> pool;
        for (int raw : heldNotes)
        {
            int q = TheoryEngine::quantizeNote(raw, cfg.rootMidi, cfg.scaleIdx);
            if (std::find(pool.begin(), pool.end(), q) == pool.end())
                pool.push_back(q);
        }
        std::sort(pool.begin(), pool.end());
        return pool;
    }

    int pickNote(const std::vector<int>& pool, Direction dir)
    {
        if (pool.empty()) return 60;

        int n = (int)pool.size();

        if (dir == Direction::Random)
        {
            // Weighted by scale degree proximity to root, 5th, 3rd
            return weightedPick(pool);
        }

        // Clamp stepIdx
        stepIdx = stepIdx % n;

        int note = pool[stepIdx];

        if (dir == Direction::Up)
        {
            stepIdx = (stepIdx + 1) % n;
        }
        else if (dir == Direction::Down)
        {
            stepIdx = (stepIdx - 1 + n) % n;
        }
        else // UpDown
        {
            stepIdx += pingPongDir;
            if (stepIdx >= n) { stepIdx = juce::jmax(0, n - 2); pingPongDir = -1; }
            if (stepIdx < 0)  { stepIdx = juce::jmin(1, n - 1); pingPongDir =  1; }
        }

        return note;
    }

    int weightedPick(const std::vector<int>& pool)
    {
        // Build weighted index using scale-degree weights
        std::vector<int> weights;
        weights.reserve(pool.size());
        int total = 0;
        for (int note : pool)
        {
            int deg = note % 12;
            int w;
            if (deg == 0)       w = WEIGHT_ROOT;
            else if (deg == 7)  w = WEIGHT_FIFTH;
            else if (deg == 4 || deg == 3) w = WEIGHT_THIRD;
            else                w = 5;
            weights.push_back(w);
            total += w;
        }

        std::uniform_int_distribution<int> dist(0, total - 1);
        int r = dist(rng);
        int acc = 0;
        for (int i = 0; i < (int)pool.size(); ++i)
        {
            acc += weights[i];
            if (r < acc) return pool[i];
        }
        return pool.back();
    }

    uint8_t pickVelocity()
    {
        // Humanized velocity: base 90 ± 15
        std::uniform_int_distribution<int> dist(-15, 15);
        return (uint8_t)juce::jlimit(40, 127, 90 + dist(rng));
    }
};
