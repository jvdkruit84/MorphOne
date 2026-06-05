#pragma once
#include <JuceHeader.h>

class WavetableOscillator
{
public:
    static constexpr int tableSize = 2048;

    WavetableOscillator() { generateTables(); }

    void setFrequency(float frequency, float sampleRate)
    {
        tableDelta = frequency * tableSize / sampleRate;
    }

    // morph: 0.0 (sine) -> 0.33 (triangle) -> 0.66 (saw) -> 1.0 (square)
    void setMorph(float morphValue)
    {
        morph = juce::jlimit(0.0f, 1.0f, morphValue) * (numTables - 1);
    }

    float getNextSample()
    {
        int tableA = (int)morph;
        int tableB = juce::jmin(tableA + 1, numTables - 1);
        float blend = morph - tableA;

        int idx0 = (int)currentIndex;
        int idx1 = (idx0 + 1) % tableSize;
        float frac = currentIndex - idx0;

        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };

        float sampleA = lerp(tables[tableA][idx0], tables[tableA][idx1], frac);
        float sampleB = lerp(tables[tableB][idx0], tables[tableB][idx1], frac);

        currentIndex += tableDelta;
        if (currentIndex >= tableSize)
            currentIndex -= tableSize;

        return lerp(sampleA, sampleB, blend);
    }

    void reset() { currentIndex = 0.0f; }

private:
    static constexpr int numTables = 4;
    float tables[numTables][tableSize];
    float currentIndex = 0.0f;
    float tableDelta   = 0.0f;
    float morph        = 0.0f;

    void generateTables()
    {
        for (int i = 0; i < tableSize; ++i)
        {
            float t = (float)i / tableSize;

            // Sine
            tables[0][i] = std::sin(juce::MathConstants<float>::twoPi * t);

            // Triangle
            tables[1][i] = t < 0.25f  ?  4.0f * t
                         : t < 0.75f  ?  2.0f - 4.0f * t
                                       : -4.0f + 4.0f * t;

            // Saw (band-limited via additive)
            tables[2][i] = 2.0f * t - 1.0f;

            // Square
            tables[3][i] = t < 0.5f ? 1.0f : -1.0f;
        }
    }
};
