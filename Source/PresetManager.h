#pragma once
#include <JuceHeader.h>

struct Preset
{
    juce::String name;
    // Synth
    float morph, attack, decay, sustain, release;
    float cutoff, res;
    int   unison;
    float detune, lfoRate, lfoDepth, reverb, gain;
    // Theory
    int   key;       // 0=C … 11=B
    int   scaleIdx;  // TheoryEngine::SCALE_INTERVALS index
    bool  scaleLock;
    int   chordType; // TheoryEngine::CHORD_INTERVALS index (0=off)
    int   chordInv;
    bool  arpOn;
    int   arpDir;    // 0=Up 1=Down 2=UpDown 3=Random
    int   arpSpeed;  // 0=1/4 1=1/8 2=1/16
    float arpGate;
    int   progIdx;   // TheoryEngine::ARTIST_PROGS index (0=off)
    int   progChord; // chord type for progression (1=Major … 10=Open)
};

class PresetManager
{
public:
    static const std::vector<Preset>& getPresets()
    {
        // key scaleIdx lock chrdT inv arpOn dir spd  gate progIdx prChrd
        static const std::vector<Preset> presets =
        {
            //  name            morph  atk    dec    sus   rel    cutoff  res  uni  det   lfoR  lfoD  rev   gain   key sc  lk  ct  ci  ar  ad  as  ag    pi  pc
            { "Default",        0.00f, 0.05f, 0.10f, 0.80f,0.30f, 8000.f,0.7f, 1, 0.30f,0.50f,0.00f,0.00f,0.80f,  0, 5, 0,  0,  0,  0,  0,  1,  0.8f, 0,  1 },

            // Deep / Melodic House
            { "Rufus Du Sol",   0.10f, 0.80f, 0.50f, 0.70f,1.50f, 1200.f,1.2f, 4, 0.40f,0.30f,0.15f,0.55f,0.80f,  0, 0, 1,  0,  0,  0,  0,  1,  0.8f, 1,  2 },
            { "Ben Bohmer",     0.05f, 1.20f, 0.80f, 0.75f,2.50f, 3000.f,0.8f, 6, 0.60f,0.15f,0.10f,0.70f,0.75f,  2, 1, 1,  2,  0,  0,  0,  1,  0.8f, 2,  2 },
            { "Black Coffee",   0.20f, 0.05f, 0.60f, 0.50f,0.80f, 1500.f,1.8f, 3, 0.35f,0.50f,0.20f,0.35f,0.80f,  0, 1, 1,  0,  0,  0,  0,  1,  0.8f, 6,  2 },
            { "Adriatique",     0.45f, 0.30f, 0.40f, 0.65f,2.00f, 2000.f,1.5f, 5, 0.50f,0.20f,0.20f,0.50f,0.75f,  0, 0, 1,  2,  1,  0,  0,  1,  0.8f, 4,  2 },

            // Techno
            { "Artbat",         0.65f, 0.01f, 0.30f, 0.60f,0.40f,  800.f,3.5f, 2, 0.20f,1.00f,0.30f,0.25f,0.80f,  2, 0, 1,  0,  0,  1,  0,  1,  0.7f, 3,  1 },
            { "T78",            0.85f, 0.001f,0.20f, 0.40f,0.20f,  600.f,5.0f, 1, 0.00f,2.00f,0.40f,0.10f,0.85f,  0, 0, 0,  0,  0,  1,  0,  2,  0.6f, 5,  1 },

            // Melodic Techno / Orchestral
            { "Worakls",        0.15f, 1.50f, 0.60f, 0.80f,3.00f, 4000.f,0.9f, 7, 0.55f,0.10f,0.08f,0.75f,0.72f,  5, 5, 1,  7,  1,  0,  0,  1,  0.8f, 7,  7 },
            { "Nto",            0.35f, 0.60f, 0.50f, 0.70f,2.20f, 1800.f,1.3f, 5, 0.45f,0.25f,0.18f,0.60f,0.76f,  0, 0, 1,  2,  0,  0,  0,  1,  0.8f, 8,  2 },

            // Pads & Textures
            { "Space Pad",      0.12f, 2.50f, 1.00f, 0.85f,4.00f, 2500.f,0.6f, 8, 0.80f,0.08f,0.25f,0.80f,0.70f,  0, 5, 1,  7,  2,  0,  0,  1,  0.8f, 0,  7 },
            { "Dark Pluck",     0.70f, 0.001f,0.40f, 0.20f,0.50f,  500.f,4.5f, 2, 0.25f,0.00f,0.00f,0.15f,0.82f,  0, 0, 1,  0,  0,  1,  0,  1,  0.6f, 0,  2 },
            { "Warm Lead",      0.30f, 0.02f, 0.20f, 0.75f,0.60f, 5000.f,1.0f, 3, 0.30f,0.80f,0.12f,0.30f,0.80f,  0, 5, 1,  0,  0,  1,  2,  1,  0.8f, 0,  1 },
        };
        return presets;
    }

    static void applyPreset(const Preset& p, juce::AudioProcessorValueTreeState& apvts)
    {
        auto setF = [&](const juce::String& id, float val)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(val));
        };
        auto setI = [&](const juce::String& id, int val)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1((float)val));
        };

        setF("MORPH",         p.morph);
        setF("ATTACK",        p.attack);
        setF("DECAY",         p.decay);
        setF("SUSTAIN",       p.sustain);
        setF("RELEASE",       p.release);
        setF("FILTER_CUTOFF", p.cutoff);
        setF("FILTER_RES",    p.res);
        setI("UNISON",        p.unison);
        setF("DETUNE",        p.detune);
        setF("LFO_RATE",      p.lfoRate);
        setF("LFO_DEPTH",     p.lfoDepth);
        setF("REVERB_MIX",    p.reverb);
        setF("GAIN",          p.gain);

        setI("THEORY_KEY",    p.key);
        setI("SCALE_IDX",     p.scaleIdx);
        setF("SCALE_LOCK",    p.scaleLock ? 1.0f : 0.0f);
        setI("CHORD_TYPE",    p.chordType);
        setI("CHORD_INV",     p.chordInv);
        setF("ARP_ON",        p.arpOn ? 1.0f : 0.0f);
        setI("ARP_DIR",       p.arpDir);
        setI("ARP_SPEED",     p.arpSpeed);
        setF("ARP_GATE",      p.arpGate);
        setI("PROG_IDX",      p.progIdx);
        setI("PROG_CHORD",    p.progChord);
    }
};
