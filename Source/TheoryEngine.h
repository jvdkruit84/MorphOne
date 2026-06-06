#pragma once
#include <JuceHeader.h>
#include <map>

// All music-theory data ported from MusicTheory.php + melodyAlgo.js
class TheoryEngine
{
public:
    // ── Scale definitions (semitone intervals from root) ─────────────
    static constexpr int NUM_SCALES = 15;
    static const char* SCALE_NAMES[NUM_SCALES];
    static const std::vector<int> SCALE_INTERVALS[NUM_SCALES];

    // ── Chord definitions ────────────────────────────────────────────
    static constexpr int NUM_CHORD_TYPES = 11;
    static const char* CHORD_NAMES[NUM_CHORD_TYPES];
    static const std::vector<int> CHORD_INTERVALS[NUM_CHORD_TYPES];

    // ── Artist progression root-offsets (4 chords, semitones from key root) ──
    struct ArtistProg { const char* name; int offsets[4]; };
    static constexpr int NUM_PROGS = 10; // 0 = Off
    static const ArtistProg ARTIST_PROGS[NUM_PROGS];

    // ── Quantize a MIDI note to the nearest in-scale note ────────────
    static int quantizeNote(int midiNote, int rootMidi, int scaleIdx)
    {
        const auto& scale = SCALE_INTERVALS[juce::jlimit(0, NUM_SCALES-1, scaleIdx)];
        if (scale.empty()) return midiNote;

        int noteClass = ((midiNote - rootMidi) % 12 + 12) % 12;
        int best = scale[0], bestDist = 12;
        for (int deg : scale)
        {
            int dist = std::abs(deg - noteClass);
            if (dist > 6) dist = 12 - dist;
            if (dist < bestDist) { bestDist = dist; best = deg; }
        }
        // Reconstruct: find base octave
        int diff = midiNote - rootMidi;
        int octave = diff / 12;
        if (diff < 0 && (diff % 12) != 0) octave--;
        return juce::jlimit(0, 127, rootMidi + octave * 12 + best);
    }

    // ── Expand one root note into a chord ────────────────────────────
    static std::vector<int> getChordNotes(int rootMidi, int chordTypeIdx, int inversion)
    {
        const auto& intervals = CHORD_INTERVALS[juce::jlimit(0, NUM_CHORD_TYPES-1, chordTypeIdx)];
        if (intervals.empty()) return {};

        std::vector<int> notes;
        for (int iv : intervals)
            notes.push_back(rootMidi + iv);

        // Apply inversion
        int inv = juce::jlimit(0, (int)notes.size()-1, inversion);
        for (int i = 0; i < inv; ++i)
        {
            notes.push_back(notes[0] + 12);
            notes.erase(notes.begin());
        }

        // Filter MIDI range
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [](int n){ return n < 0 || n > 127; }), notes.end());
        return notes;
    }

    // ── Process MidiBuffer: quantize all note events to scale ────────
    static void applyScaleLock(juce::MidiBuffer& midi, int rootMidi, int scaleIdx,
                                std::map<int,int>& noteMap)
    {
        juce::MidiBuffer out;
        for (const auto& m : midi)
        {
            auto msg = m.getMessage();
            int sp  = m.samplePosition;
            if (msg.isNoteOn())
            {
                int q = quantizeNote(msg.getNoteNumber(), rootMidi, scaleIdx);
                noteMap[msg.getNoteNumber()] = q;
                out.addEvent(juce::MidiMessage::noteOn(msg.getChannel(), q, msg.getVelocity()), sp);
            }
            else if (msg.isNoteOff())
            {
                auto it = noteMap.find(msg.getNoteNumber());
                int q = (it != noteMap.end()) ? it->second : msg.getNoteNumber();
                if (it != noteMap.end()) noteMap.erase(it);
                out.addEvent(juce::MidiMessage::noteOff(msg.getChannel(), q, msg.getVelocity()), sp);
            }
            else { out.addEvent(msg, sp); }
        }
        midi.swapWith(out);
    }

    // ── Process MidiBuffer: expand note-ons to chords ────────────────
    static void applyChordMode(juce::MidiBuffer& midi, int chordTypeIdx, int inversion,
                                std::map<int,std::vector<int>>& chordMap)
    {
        if (chordTypeIdx == 0) return;
        juce::MidiBuffer out;
        for (const auto& m : midi)
        {
            auto msg = m.getMessage();
            int sp  = m.samplePosition;
            if (msg.isNoteOn())
            {
                auto notes = getChordNotes(msg.getNoteNumber(), chordTypeIdx, inversion);
                chordMap[msg.getNoteNumber()] = notes;
                for (int n : notes)
                    out.addEvent(juce::MidiMessage::noteOn(msg.getChannel(), n, msg.getVelocity()), sp);
            }
            else if (msg.isNoteOff())
            {
                auto it = chordMap.find(msg.getNoteNumber());
                if (it != chordMap.end())
                {
                    for (int n : it->second)
                        out.addEvent(juce::MidiMessage::noteOff(msg.getChannel(), n, msg.getVelocity()), sp);
                    chordMap.erase(it);
                }
                else { out.addEvent(msg, sp); }
            }
            else { out.addEvent(msg, sp); }
        }
        midi.swapWith(out);
    }

    // ── Note name for display ─────────────────────────────────────────
    static juce::String noteName(int semitone)
    {
        static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        return names[semitone % 12];
    }
};

// ── Static data definitions ───────────────────────────────────────────

inline const char* TheoryEngine::SCALE_NAMES[NUM_SCALES] = {
    "Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian",
    "Major", "Harmonic Minor", "Melodic Minor",
    "Penta Minor", "Penta Major", "Blues",
    "Phrygian Dom", "Hungarian", "Whole Tone", "Chromatic"
};

inline const std::vector<int> TheoryEngine::SCALE_INTERVALS[NUM_SCALES] = {
    {0,2,3,5,7,8,10},       // Minor (Aeolian)
    {0,2,3,5,7,9,10},       // Dorian
    {0,1,3,5,7,8,10},       // Phrygian
    {0,2,4,6,7,9,11},       // Lydian
    {0,2,4,5,7,9,10},       // Mixolydian
    {0,2,4,5,7,9,11},       // Major (Ionian)
    {0,2,3,5,7,8,11},       // Harmonic Minor
    {0,2,3,5,7,9,11},       // Melodic Minor
    {0,3,5,7,10},            // Pentatonic Minor
    {0,2,4,7,9},             // Pentatonic Major
    {0,3,5,6,7,10},          // Blues Minor
    {0,1,4,5,7,8,10},        // Phrygian Dominant
    {0,2,3,6,7,8,11},        // Hungarian Minor
    {0,2,4,6,8,10},          // Whole Tone
    {0,1,2,3,4,5,6,7,8,9,10,11} // Chromatic
};

inline const char* TheoryEngine::CHORD_NAMES[NUM_CHORD_TYPES] = {
    "Off", "Major", "Minor", "Sus2", "Sus4",
    "Add9", "Minor 7", "Major 7", "Dom 7", "Power", "Open"
};

inline const std::vector<int> TheoryEngine::CHORD_INTERVALS[NUM_CHORD_TYPES] = {
    {},              // Off — single note passthrough
    {0,4,7},         // Major
    {0,3,7},         // Minor
    {0,2,7},         // Sus2
    {0,5,7},         // Sus4
    {0,4,7,14},      // Add9
    {0,3,7,10},      // Minor 7
    {0,4,7,11},      // Major 7
    {0,4,7,10},      // Dom 7
    {0,7},           // Power (5th)
    {0,7,12}         // Open / No3
};

inline const TheoryEngine::ArtistProg TheoryEngine::ARTIST_PROGS[NUM_PROGS] = {
    { "Off",            { 0,  0,  0,  0 } },
    { "Rufus Du Sol",   { 0,  3,  7, 10 } }, // i  – bIII – v  – bVII
    { "Ben Bohmer",     { 0,  2,  9,  5 } }, // I  – ii   – vi – IV
    { "Artbat",         { 0,  5, 10,  0 } }, // i  – iv   – bVII – i
    { "Adriatique",     { 0, 10,  8, 10 } }, // i  – bVII – bVI – bVII
    { "T78",            { 0,  5,  7,  0 } }, // i  – iv   – v   – i
    { "Black Coffee",   { 0,  2,  5,  0 } }, // i  – ii   – IV  – i  (Dorian)
    { "Worakls",        { 0,  5, 11,  7 } }, // I  – IV   – VII – V
    { "Nto",            { 0,  3,  8,  5 } }, // i  – bIII – bVI – iv
    { "Anyma",          { 0,  8, 10,  7 } }, // i  – bVI  – bVII – v
};
