#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <algorithm>
#include "TheoryEngine.h"

class ChordDetector
{
public:
    struct Result
    {
        juce::String name;           // "C Minor 7"
        juce::String degree;         // "i", "IV", "V7"
        int          rootSemitone = -1;
        bool         valid        = false;
    };

    struct DiatonicChord
    {
        juce::String name;       // "F Major"
        juce::String degree;     // "IV"
        int  rootSemitone;       // 0-11
        int  chordTypeIdx;       // TheoryEngine::CHORD_INTERVALS index (1=Maj, 2=Min …)
    };

    // ── Detect chord from held MIDI notes ─────────────────────────────
    static Result detect(const std::vector<int>& midiNotes, int keyRoot, int scaleIdx)
    {
        // Collect unique pitch classes
        std::vector<int> classes;
        for (int n : midiNotes)
        {
            int pc = n % 12;
            if (std::find(classes.begin(), classes.end(), pc) == classes.end())
                classes.push_back(pc);
        }

        if (classes.empty())  return {};
        if (classes.size() == 1)
        {
            Result r;
            r.valid        = true;
            r.rootSemitone = classes[0];
            r.name         = noteName(classes[0]);
            r.degree       = degreeLabel(classes[0], keyRoot, scaleIdx);
            return r;
        }

        // Try each pitch class as root, match against known chord types
        for (int root : classes)
        {
            std::vector<int> intervals;
            for (int c : classes)
                intervals.push_back((c - root + 12) % 12);
            std::sort(intervals.begin(), intervals.end());

            for (int ci = 1; ci < TheoryEngine::NUM_CHORD_TYPES; ++ci)
            {
                const auto& tpl = TheoryEngine::CHORD_INTERVALS[ci];
                if (tpl.empty()) continue;

                // Normalise template to pitch-class set
                std::vector<int> norm;
                for (int iv : tpl) norm.push_back(iv % 12);
                std::sort(norm.begin(), norm.end());
                norm.erase(std::unique(norm.begin(), norm.end()), norm.end());

                if (intervals == norm)
                {
                    Result r;
                    r.valid        = true;
                    r.rootSemitone = root;
                    r.name         = noteName(root) + " " + juce::String(TheoryEngine::CHORD_NAMES[ci]);
                    r.degree       = degreeLabel(root, keyRoot, scaleIdx)
                                     + (ci >= 6 && ci <= 8 ? "7" : "");
                    return r;
                }
            }
        }

        // Partial match — just show the lowest note
        Result r;
        r.valid        = true;
        r.rootSemitone = *std::min_element(classes.begin(), classes.end());
        r.name         = noteName(r.rootSemitone) + " …";
        r.degree       = degreeLabel(r.rootSemitone, keyRoot, scaleIdx);
        return r;
    }

    // ── All diatonic triads in the current key+scale ──────────────────
    static std::vector<DiatonicChord> getDiatonicChords(int keyRoot, int scaleIdx)
    {
        const auto& sc = TheoryEngine::SCALE_INTERVALS[juce::jlimit(0, 14, scaleIdx)];
        if (sc.size() < 3) return {};

        std::vector<DiatonicChord> out;
        int n = (int)sc.size();

        for (int i = 0; i < n; ++i)
        {
            int root = (keyRoot + sc[i]) % 12;

            // Third: 2 positions up in scale (wraps)
            int third = sc[(i + 2) % n];
            int fifth = sc[(i + 4) % n];
            // Compute intervals relative to degree root
            int thirdIv = ((third - sc[i]) + 12) % 12;
            int fifthIv = ((fifth - sc[i]) + 12) % 12;

            // Quality
            int typeIdx;
            if      (thirdIv == 4 && fifthIv == 7) typeIdx = 1; // Major
            else if (thirdIv == 3 && fifthIv == 7) typeIdx = 2; // Minor
            else if (thirdIv == 3 && fifthIv == 6) typeIdx = 2; // Diminished → show as Minor
            else if (thirdIv == 4 && fifthIv == 8) typeIdx = 1; // Augmented → show as Major
            else                                   typeIdx = 2; // fallback

            DiatonicChord dc;
            dc.rootSemitone = root;
            dc.chordTypeIdx = typeIdx;
            dc.name   = noteName(root) + " " + juce::String(TheoryEngine::CHORD_NAMES[typeIdx]);
            dc.degree = scaleDegreeNumeral(i, typeIdx == 1);
            out.push_back(dc);
        }
        return out;
    }

    // ── Next chord suggestions from current root ──────────────────────
    static std::vector<DiatonicChord> getNextSuggestions(int currentRootSemi,
                                                          int keyRoot,
                                                          int scaleIdx,
                                                          int maxResults = 3)
    {
        auto diatonic = getDiatonicChords(keyRoot, scaleIdx);
        if (diatonic.empty()) return {};

        // Find index of current chord in diatonic list
        int curIdx = -1;
        for (int i = 0; i < (int)diatonic.size(); ++i)
            if (diatonic[i].rootSemitone == currentRootSemi) { curIdx = i; break; }

        if (curIdx < 0)
        {
            // Not a diatonic chord — just return first 3 diatonic chords
            auto res = diatonic;
            if ((int)res.size() > maxResults) res.resize(maxResults);
            return res;
        }

        int n = (int)diatonic.size();

        // Next-chord lookup table: for each degree 0-(n-1), list 3 preferred next indices
        // Based on tonal harmony (IV, V, vi for major; iv, bVII, v for minor etc.)
        // Using general rules: avoid repeating current; prefer +4, +2, -2 in scale
        static const int nextMajor[7][3] = {
            {3,4,5}, {4,3,0}, {5,3,0}, {4,0,1}, {0,5,2}, {1,3,0}, {0,2,4}
        };
        static const int nextMinor[7][3] = {
            {3,6,4}, {4,6,0}, {3,6,0}, {4,0,6}, {0,6,2}, {6,3,0}, {0,2,3}
        };

        bool isMajorScale = (scaleIdx == 5 || scaleIdx == 3 || scaleIdx == 4);
        const int (*table)[3] = isMajorScale ? nextMajor : nextMinor;

        std::vector<DiatonicChord> result;
        if (n >= 7)
        {
            for (int j = 0; j < maxResults; ++j)
            {
                int idx = table[curIdx % 7][j];
                if (idx < n) result.push_back(diatonic[idx]);
            }
        }
        else
        {
            // Shorter scale — just return diatonic chords that aren't current
            for (int i = 0; i < n && (int)result.size() < maxResults; ++i)
                if (i != curIdx) result.push_back(diatonic[i]);
        }
        return result;
    }

private:
    static const char* NOTE_NAMES[12];

    static juce::String noteName(int semitone)
    {
        return NOTE_NAMES[(semitone % 12 + 12) % 12];
    }

    static juce::String scaleDegreeNumeral(int degreeIdx, bool major)
    {
        static const char* maj[] = {"I","II","III","IV","V","VI","VII"};
        static const char* min[] = {"i","ii","iii","iv","v","vi","vii"};
        int i = degreeIdx % 7;
        return major ? maj[i] : min[i];
    }

    static juce::String degreeLabel(int rootSemitone, int keyRoot, int scaleIdx)
    {
        const auto& sc = TheoryEngine::SCALE_INTERVALS[juce::jlimit(0, 14, scaleIdx)];
        int rel = (rootSemitone - keyRoot + 12) % 12;
        for (int i = 0; i < (int)sc.size(); ++i)
            if (sc[i] == rel)
            {
                // Determine quality of this degree
                int thirdIv = (i + 2 < (int)sc.size())
                    ? (sc[i+2] - sc[i] + 12) % 12
                    : 4;
                return scaleDegreeNumeral(i, thirdIv == 4);
            }
        return "?";
    }
};

inline const char* ChordDetector::NOTE_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};
