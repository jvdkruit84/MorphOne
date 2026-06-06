#pragma once
#include <JuceHeader.h>

struct Preset
{
    juce::String name;
    float morph, attack, decay, sustain, release;
    float cutoff, res;
    int   unison;
    float detune, lfoRate, lfoDepth, reverb, gain;
};

class PresetManager
{
public:
    static const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets =
        {
            //  name               morph  atk    dec   sus   rel    cutoff  res   uni  det   lfoR  lfoD  rev   gain
            { "Default",           0.00f, 0.05f, 0.10f,0.80f,0.30f, 8000.f,0.7f,  1, 0.30f,0.50f,0.00f,0.00f,0.80f },

            // Deep / Melodic House
            { "Rufus Du Sol",      0.10f, 0.80f, 0.50f,0.70f,1.50f, 1200.f,1.2f,  4, 0.40f,0.30f,0.15f,0.55f,0.80f },
            { "Ben Bohmer",        0.05f, 1.20f, 0.80f,0.75f,2.50f, 3000.f,0.8f,  6, 0.60f,0.15f,0.10f,0.70f,0.75f },
            { "Black Coffee",      0.20f, 0.05f, 0.60f,0.50f,0.80f, 1500.f,1.8f,  3, 0.35f,0.50f,0.20f,0.35f,0.80f },
            { "Adriatique",        0.45f, 0.30f, 0.40f,0.65f,2.00f, 2000.f,1.5f,  5, 0.50f,0.20f,0.20f,0.50f,0.75f },

            // Techno
            { "Artbat",            0.65f, 0.01f, 0.30f,0.60f,0.40f,  800.f,3.5f,  2, 0.20f,1.00f,0.30f,0.25f,0.80f },
            { "T78",               0.85f, 0.001f,0.20f,0.40f,0.20f,  600.f,5.0f,  1, 0.00f,2.00f,0.40f,0.10f,0.85f },

            // Melodic Techno / Orchestral
            { "Worakls",           0.15f, 1.50f, 0.60f,0.80f,3.00f, 4000.f,0.9f,  7, 0.55f,0.10f,0.08f,0.75f,0.72f },
            { "Nto",               0.35f, 0.60f, 0.50f,0.70f,2.20f, 1800.f,1.3f,  5, 0.45f,0.25f,0.18f,0.60f,0.76f },

            // Pads & Textures
            { "Space Pad",         0.12f, 2.50f, 1.00f,0.85f,4.00f, 2500.f,0.6f,  8, 0.80f,0.08f,0.25f,0.80f,0.70f },
            { "Dark Pluck",        0.70f, 0.001f,0.40f,0.20f,0.50f,  500.f,4.5f,  2, 0.25f,0.00f,0.00f,0.15f,0.82f },
            { "Warm Lead",         0.30f, 0.02f, 0.20f,0.75f,0.60f, 5000.f,1.0f,  3, 0.30f,0.80f,0.12f,0.30f,0.80f },
        };
        return presets;
    }

    static void applyPreset(const Preset& p, juce::AudioProcessorValueTreeState& apvts)
    {
        auto set = [&](const juce::String& id, float val)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(val));
        };

        set("MORPH",         p.morph);
        set("ATTACK",        p.attack);
        set("DECAY",         p.decay);
        set("SUSTAIN",       p.sustain);
        set("RELEASE",       p.release);
        set("FILTER_CUTOFF", p.cutoff);
        set("FILTER_RES",    p.res);
        set("UNISON",        (float)p.unison);
        set("DETUNE",        p.detune);
        set("LFO_RATE",      p.lfoRate);
        set("LFO_DEPTH",     p.lfoDepth);
        set("REVERB_MIX",    p.reverb);
        set("GAIN",          p.gain);
    }
};
