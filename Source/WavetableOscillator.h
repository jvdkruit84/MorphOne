#pragma once
#include <JuceHeader.h>

class WavetableOscillator
{
public:
    static constexpr int tableSize = 2048;
    static constexpr int numTables = 4; // sine, triangle, saw, square

    WavetableOscillator() { generateTables(); }

    void setFrequency(float frequency, float sampleRate)
    {
        tableDelta = frequency * tableSize / sampleRate;
    }

    void setMorph(float morphValue)
    {
        morph = juce::jlimit(0.0f, 1.0f, morphValue) * (numTables - 1);
    }

    float getNextSample()
    {
        int   tableA = (int)morph;
        int   tableB = juce::jmin(tableA + 1, numTables - 1);
        float blend  = morph - tableA;
        int   idx0   = (int)currentIndex;
        int   idx1   = (idx0 + 1) % tableSize;
        float frac   = currentIndex - idx0;

        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
        float sA = lerp(tables[tableA][idx0], tables[tableA][idx1], frac);
        float sB = lerp(tables[tableB][idx0], tables[tableB][idx1], frac);

        currentIndex += tableDelta;
        if (currentIndex >= tableSize) currentIndex -= tableSize;

        return lerp(sA, sB, blend);
    }

    // For visualisation — sample at a normalised phase (0-1) without advancing
    static float sampleAt(float normPhase, float morphValue)
    {
        static WavetableOscillator ref;
        float m = juce::jlimit(0.0f, 1.0f, morphValue) * (numTables - 1);
        int   tableA = (int)m;
        int   tableB = juce::jmin(tableA + 1, numTables - 1);
        float blend  = m - tableA;
        float idx    = normPhase * tableSize;
        int   idx0   = (int)idx % tableSize;
        int   idx1   = (idx0 + 1) % tableSize;
        float frac   = idx - (int)idx;
        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
        float sA = lerp(ref.tables[tableA][idx0], ref.tables[tableA][idx1], frac);
        float sB = lerp(ref.tables[tableB][idx0], ref.tables[tableB][idx1], frac);
        return lerp(sA, sB, blend);
    }

    void reset() { currentIndex = 0.0f; }

private:
    float tables[numTables][tableSize];
    float currentIndex = 0.0f;
    float tableDelta   = 0.0f;
    float morph        = 0.0f;

    void generateTables()
    {
        for (int i = 0; i < tableSize; ++i)
        {
            float t = (float)i / tableSize;
            tables[0][i] = std::sin(juce::MathConstants<float>::twoPi * t);
            tables[1][i] = t < 0.25f ? 4.f*t : t < 0.75f ? 2.f - 4.f*t : -4.f + 4.f*t;
            tables[2][i] = 2.0f * t - 1.0f;
            tables[3][i] = t < 0.5f ? 1.0f : -1.0f;
        }
    }
};
