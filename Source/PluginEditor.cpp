#include "PluginEditor.h"

// ── VisualKeyboard paint ──────────────────────────────────────────────────────
void VisualKeyboard::paint(juce::Graphics& g)
{
    static const int wOff[7] = { 0, 2, 4, 5, 7, 9, 11 };     // C D E F G A B
    static const int bOff[5] = { 1, 3, 6, 8, 10 };            // C# D# F# G# A#
    static const int bAfter[5] = { 0, 1, 3, 4, 5 };           // after white-key index

    int key    = (int)apvts.getRawParameterValue("THEORY_KEY")->load();
    int si     = (int)apvts.getRawParameterValue("SCALE_IDX")->load();
    const auto& sc = TheoryEngine::SCALE_INTERVALS[juce::jlimit(0, 14, si)];

    auto lb = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff0a0a18));
    g.fillRoundedRectangle(lb, 5.f);
    g.setColour(juce::Colour(Pal::border));
    g.drawRoundedRectangle(lb.reduced(0.5f), 5.f, 0.7f);

    auto inner = lb.reduced(3.f, 3.f);
    float ww = inner.getWidth() / 14.f;
    float wh = inner.getHeight();
    float bw = ww * 0.65f;
    float bh = wh * 0.60f;

    // Helper: does this pitch class have any held note (any octave)?
    auto pitchHeld = [&](int pitchClass) -> bool
    {
        for (int n = pitchClass; n < 128; n += 12)
            if (proc.isNoteHeld(n)) return true;
        return false;
    };

    auto inScale = [&](int pitchClass) -> bool
    {
        int deg = ((pitchClass - key) % 12 + 12) % 12;
        return std::find(sc.begin(), sc.end(), deg) != sc.end();
    };

    auto isRoot = [&](int pitchClass) -> bool
    { return pitchClass == (key % 12); };

    // White keys
    for (int wi = 0; wi < 14; ++wi)
    {
        int midiPc  = (48 + (wi / 7) * 12 + wOff[wi % 7]) % 12;
        bool held   = pitchHeld(midiPc);
        bool inscale = inScale(midiPc);
        bool isroot = isRoot(midiPc);

        float kx = inner.getX() + wi * ww;
        juce::Rectangle<float> wkr(kx + 0.5f, inner.getY(), ww - 1.f, wh);

        if (held)
        {
            g.setColour(juce::Colour(Pal::accentHi).withAlpha(0.25f));
            g.fillRoundedRectangle(wkr.expanded(2.f), 3.f);
            g.setColour(juce::Colour(Pal::accentHi));
        }
        else if (isroot)
            g.setColour(juce::Colour(0xff2a1040));
        else if (inscale)
            g.setColour(juce::Colour(0xff181830));
        else
            g.setColour(juce::Colour(0xff111122));

        g.fillRoundedRectangle(wkr, 2.f);
        g.setColour(juce::Colour(Pal::border));
        g.drawRoundedRectangle(wkr, 2.f, 0.5f);

        // Note label for root and C notes
        if (isroot || (wi % 7 == 0))
        {
            static const char* nn[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            g.setFont(juce::FontOptions(6.5f));
            g.setColour(held   ? juce::Colours::white :
                        isroot ? juce::Colour(Pal::accentHi) : juce::Colour(Pal::textLow));
            g.drawText(nn[midiPc],
                       juce::Rectangle<float>(kx, inner.getBottom() - 11.f, ww, 11.f),
                       juce::Justification::centred);
        }
    }

    // Black keys (drawn on top)
    for (int oct = 0; oct < 2; ++oct)
    {
        for (int bi = 0; bi < 5; ++bi)
        {
            int midiPc = (48 + oct * 12 + bOff[bi]) % 12;
            bool held = pitchHeld(midiPc);
            bool inscale = inScale(midiPc);
            bool root = isRoot(midiPc);

            int whiteIdx = oct * 7 + bAfter[bi];
            float kx = inner.getX() + (whiteIdx + 0.62f) * ww;
            juce::Rectangle<float> bkey(kx, inner.getY(), bw, bh);

            if (held)
            {
                g.setColour(juce::Colour(Pal::accentHi).withAlpha(0.25f));
                g.fillRoundedRectangle(bkey.expanded(2.f), 2.f);
                g.setColour(juce::Colour(Pal::accentHi));
            }
            else if (root)
                g.setColour(juce::Colour(0xff380e60));
            else if (inscale)
                g.setColour(juce::Colour(0xff181828));
            else
                g.setColour(juce::Colour(0xff080810));

            g.fillRoundedRectangle(bkey, 2.f);
            g.setColour(juce::Colour(Pal::borderHi).withAlpha(0.4f));
            g.drawRoundedRectangle(bkey, 2.f, 0.5f);
        }
    }
}

// ── LiveChordPanel paint ──────────────────────────────────────────────────────
void LiveChordPanel::paint(juce::Graphics& g)
{
    static const char* kn[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    int keyParam = (int)apvts.getRawParameterValue("THEORY_KEY")->load();
    int si       = (int)apvts.getRawParameterValue("SCALE_IDX")->load();
    const auto& sc = TheoryEngine::SCALE_INTERVALS[juce::jlimit(0, 14, si)];

    auto lb   = getLocalBounds().toFloat();
    float px  = 8.f;
    float pw  = lb.getWidth() - 16.f;

    // Outer border with purple accent
    g.setColour(juce::Colour(0xff0d0d20));
    g.fillRoundedRectangle(lb, 6.f);
    g.setColour(juce::Colour(Pal::accent).withAlpha(0.45f));
    g.drawRoundedRectangle(lb.reduced(0.5f), 6.f, 1.2f);

    auto held = proc.getHeldNotes();
    auto result = ChordDetector::detect(held, keyParam, si);

    float y = 6.f;

    // ── Chord name (large) + degree ──
    if (result.valid && !result.name.isEmpty())
    {
        g.setFont(juce::FontOptions(30.f, juce::Font::bold));
        g.setColour(juce::Colour(Pal::textHi));
        g.drawText(result.name, px, y, pw * 0.70f, 38.f, juce::Justification::centredLeft);

        if (!result.degree.isEmpty() && result.degree != "?")
        {
            g.setFont(juce::FontOptions(20.f, juce::Font::bold));
            g.setColour(juce::Colour(Pal::accentHi));
            g.drawText(result.degree, px + pw * 0.70f, y + 6.f, pw * 0.30f - 4.f, 28.f,
                       juce::Justification::centredRight);
        }
    }
    else
    {
        g.setFont(juce::FontOptions(16.f));
        g.setColour(juce::Colour(Pal::textLow));
        g.drawText("Speel noten...", px, y + 8.f, pw, 24.f, juce::Justification::centredLeft);
    }
    y += 40.f;

    // ── Key + Scale label ──
    g.setFont(juce::FontOptions(10.f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textMid));
    g.drawText(juce::String(kn[keyParam % 12]) + "  " + juce::String(TheoryEngine::SCALE_NAMES[si]),
               px, y, pw, 14.f, juce::Justification::centredLeft);
    y += 18.f;

    // ── 12-note semitone pills ──
    float pillW = pw / 12.f;
    float pillH = 20.f;
    for (int i = 0; i < 12; ++i)
    {
        juce::Rectangle<float> pill(px + i * pillW, y, pillW - 1.f, pillH);
        int deg    = ((i - keyParam) % 12 + 12) % 12;
        bool inscale = std::find(sc.begin(), sc.end(), deg) != sc.end();
        bool root    = (i == keyParam % 12);
        bool active  = false;
        for (int hn : held) if ((hn % 12) == i) { active = true; break; }

        if (active)
        {
            g.setColour(juce::Colour(Pal::accentHi).withAlpha(0.25f));
            g.fillRoundedRectangle(pill.expanded(1.5f), 4.f);
            g.setColour(juce::Colour(Pal::accentHi));
            g.fillRoundedRectangle(pill, 4.f);
        }
        else if (root)
        {
            g.setColour(juce::Colour(Pal::pillRoot));
            g.fillRoundedRectangle(pill, 4.f);
        }
        else if (inscale)
        {
            g.setColour(juce::Colour(Pal::pillIn));
            g.fillRoundedRectangle(pill, 4.f);
            g.setColour(juce::Colour(0xff5a3a8a));
            g.drawRoundedRectangle(pill.reduced(0.5f), 4.f, 0.6f);
        }
        else
        {
            g.setColour(juce::Colour(Pal::pillOut));
            g.fillRoundedRectangle(pill, 4.f);
        }
        g.setFont(juce::FontOptions(7.f, inscale ? juce::Font::bold : 0));
        g.setColour(active ? juce::Colours::white :
                    root   ? juce::Colours::white :
                    inscale ? juce::Colour(0xffcc99ff) : juce::Colour(0xff2a2a45));
        g.drawText(kn[i], pill, juce::Justification::centred);
    }
}

// ── NextSuggestionsPanel paint ────────────────────────────────────────────────
void NextSuggestionsPanel::paint(juce::Graphics& g)
{
    auto lb = getLocalBounds();

    int keyParam = (int)apvts.getRawParameterValue("THEORY_KEY")->load();
    int si       = (int)apvts.getRawParameterValue("SCALE_IDX")->load();

    auto held  = proc.getHeldNotes();
    auto chord = ChordDetector::detect(held, keyParam, si);

    int currentRoot = chord.valid ? chord.rootSemitone : keyParam;
    auto suggestions = ChordDetector::getNextSuggestions(currentRoot, keyParam, si, 3);

    // Header label
    g.setFont(juce::FontOptions(8.f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textLow));
    g.drawText("VOLGENDE AKKOORDEN", 8, 2, lb.getWidth() - 16, 14, juce::Justification::centredLeft);

    if (suggestions.empty()) return;

    float cardH   = lb.getHeight() - 18.f;
    float totalW  = lb.getWidth() - 16.f;
    float cardW   = totalW / 3.f - 3.f;

    for (int i = 0; i < (int)suggestions.size() && i < 3; ++i)
    {
        const auto& s = suggestions[i];
        float cx = 8.f + i * (cardW + 4.f);
        juce::Rectangle<float> card(cx, 18.f, cardW, cardH);

        juce::ColourGradient cg(juce::Colour(0xff1c1c38), cx, 18.f,
                                juce::Colour(0xff0e0e20), cx, 18.f + cardH, false);
        g.setGradientFill(cg);
        g.fillRoundedRectangle(card, 5.f);
        g.setColour(juce::Colour(Pal::border));
        g.drawRoundedRectangle(card.reduced(0.5f), 5.f, 0.7f);

        // Degree tag
        g.setFont(juce::FontOptions(10.f, juce::Font::bold));
        g.setColour(juce::Colour(Pal::accentHi));
        g.drawText(s.degree, card.reduced(5.f, 3.f), juce::Justification::topLeft);

        // Chord name
        g.setFont(juce::FontOptions(13.f, juce::Font::bold));
        g.setColour(juce::Colour(Pal::textHi));
        g.drawText(s.name, card.reduced(5.f, 2.f), juce::Justification::centred);
    }
}

// ── CoachDisplay paint ────────────────────────────────────────────────────────
void CoachDisplay::paint(juce::Graphics& g)
{
    int si   = (int)apvts.getRawParameterValue("SCALE_IDX")->load();
    int key  = (int)apvts.getRawParameterValue("THEORY_KEY")->load();
    int ct   = (int)apvts.getRawParameterValue("CHORD_TYPE")->load();
    int prog = (int)apvts.getRawParameterValue("PROG_IDX")->load();
    bool lock = apvts.getRawParameterValue("SCALE_LOCK")->load() > 0.5f;
    bool arp  = apvts.getRawParameterValue("ARP_ON")->load() > 0.5f;
    int  mode = (int)apvts.getRawParameterValue("SYNTH_MODE")->load();

    auto lb = getLocalBounds();
    float x = 8.f, bw = lb.getWidth() - 16.f;
    float y = 6.f;

    // Mood badge (top right)
    Mood mood = getMood(si, ct);
    auto mi   = getMoodInfo(mood);
    float badgeW = 88.f;
    juce::Rectangle<float> badge(x + bw - badgeW, y + 1.f, badgeW, 18.f);
    g.setColour(mi.colour.withAlpha(0.18f));
    g.fillRoundedRectangle(badge, 9.f);
    g.setColour(mi.colour.withAlpha(0.7f));
    g.drawRoundedRectangle(badge.reduced(0.5f), 9.f, 0.7f);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(mi.colour);
    g.drawText(mi.name, badge, juce::Justification::centred);

    // Feature badges (top left)
    float bx = x;
    auto drawBadge = [&](const juce::String& txt, juce::Colour col)
    {
        float tw = (float)txt.length() * 6.0f + 10.f;
        juce::Rectangle<float> bb(bx, y + 1.f, tw, 14.f);
        g.setColour(col.withAlpha(0.2f));
        g.fillRoundedRectangle(bb, 7.f);
        g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
        g.setColour(col.withAlpha(0.85f));
        g.drawText(txt, bb, juce::Justification::centred);
        bx += tw + 4.f;
    };
    if (lock) drawBadge("SCALE LOCK", juce::Colour(Pal::accent));
    if (arp)  drawBadge("ARP ON",     juce::Colour(0xff4a9eff));
    if (ct>0) drawBadge(TheoryEngine::CHORD_NAMES[ct], juce::Colour(0xff44cc88));

    y += 26.f;

    // Tension bar
    float ten = getTension(si, ct);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textLow));
    g.drawText("TENSION", x, y, 52.f, 12.f, juce::Justification::centredLeft);
    juce::Rectangle<float> mbg(x + 56.f, y + 1.f, bw - 56.f, 10.f);
    g.setColour(juce::Colour(0xff0a0a1e));
    g.fillRoundedRectangle(mbg, 5.f);
    if (ten > 0.01f)
    {
        juce::Rectangle<float> mfill(mbg.getX(), mbg.getY(), mbg.getWidth() * ten, mbg.getHeight());
        juce::ColourGradient tg(
            ten < 0.4f ? juce::Colour(0xff2a8a3a) :
            ten < 0.7f ? juce::Colour(0xffcc8820) : juce::Colour(0xffcc3a2a),
            mfill.getX(), 0.f, juce::Colour(Pal::accent), mfill.getRight(), 0.f, false);
        g.setGradientFill(tg);
        g.fillRoundedRectangle(mfill, 5.f);
    }
    y += 20.f;

    // Progression badge
    if (prog > 0)
    {
        juce::Rectangle<float> pb(x, y, bw, 16.f);
        g.setColour(juce::Colour(Pal::accent).withAlpha(0.18f));
        g.fillRoundedRectangle(pb, 4.f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(Pal::accentHi));
        g.drawText(juce::String("▶  ") + TheoryEngine::ARTIST_PROGS[prog].name + "  progressie",
                   pb.reduced(4.f, 0.f), juce::Justification::centredLeft);
        y += 20.f;
    }

    // Coach tip box
    float tipH = lb.getBottom() - y - 6.f;
    if (tipH > 20.f)
    {
        juce::Rectangle<float> tipBox(x, y, bw, tipH);
        g.setColour(juce::Colour(0xff0e0e1e));
        g.fillRoundedRectangle(tipBox, 4.f);
        g.setColour(juce::Colour(Pal::border));
        g.drawRoundedRectangle(tipBox.reduced(0.5f), 4.f, 0.6f);

        g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
        g.setColour(juce::Colour(Pal::accentHi));
        g.drawText("♪  TIP", tipBox.reduced(5.f, 3.f), juce::Justification::topLeft);

        g.setFont(juce::FontOptions(8.5f));
        g.setColour(juce::Colour(Pal::textMid));
        g.drawFittedText(getCoachTip(key, si, ct, lock, arp, prog, mode),
                         tipBox.reduced(6.f, 4.f).withTrimmedTop(14.f).toNearestInt(),
                         juce::Justification::topLeft, 6);
    }
}

// ── Editor helpers ────────────────────────────────────────────────────────────
void MorphOneAudioProcessorEditor::styleCombo(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId,     juce::Colour(0xff141428));
    box.setColour(juce::ComboBox::textColourId,           juce::Colour(Pal::textHi));
    box.setColour(juce::ComboBox::outlineColourId,        juce::Colour(Pal::border));
    box.setColour(juce::ComboBox::arrowColourId,          juce::Colour(Pal::accentHi));
    box.setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(Pal::accent));
}

int MorphOneAudioProcessorEditor::getIntParam(const juce::String& id)
{ return (int)audioProcessor.apvts.getRawParameterValue(id)->load(); }

void MorphOneAudioProcessorEditor::setIntParam(const juce::String& id, int val)
{
    if (auto* p = audioProcessor.apvts.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1((float)val));
}

void MorphOneAudioProcessorEditor::populateTheoryCombos()
{
    keyBox.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    for (int i = 0; i < TheoryEngine::NUM_SCALES; ++i)
        scaleBox.addItem(TheoryEngine::SCALE_NAMES[i], i + 1);
    for (int i = 0; i < TheoryEngine::NUM_CHORD_TYPES; ++i)
        chordTypeBox.addItem(TheoryEngine::CHORD_NAMES[i], i + 1);
    for (int i = 1; i <= 4; ++i)
        chordInvBox.addItem(i == 1 ? "Root" : i == 2 ? "1st" : i == 3 ? "2nd" : "3rd", i);
    arpDirBox  .addItemList({ "Up", "Down", "Up/Down", "Random" }, 1);
    arpSpeedBox.addItemList({ "1/4", "1/8", "1/16" }, 1);
    for (int i = 0; i < TheoryEngine::NUM_PROGS; ++i)
        progBox.addItem(TheoryEngine::ARTIST_PROGS[i].name, i + 1);
    for (int i = 1; i < TheoryEngine::NUM_CHORD_TYPES; ++i)
        progChordBox.addItem(TheoryEngine::CHORD_NAMES[i], i);
}

void MorphOneAudioProcessorEditor::syncCombosFromApvts()
{
    keyBox      .setSelectedId(getIntParam("THEORY_KEY") + 1, juce::dontSendNotification);
    scaleBox    .setSelectedId(getIntParam("SCALE_IDX")  + 1, juce::dontSendNotification);
    chordTypeBox.setSelectedId(getIntParam("CHORD_TYPE") + 1, juce::dontSendNotification);
    chordInvBox .setSelectedId(getIntParam("CHORD_INV")  + 1, juce::dontSendNotification);
    arpDirBox   .setSelectedId(getIntParam("ARP_DIR")    + 1, juce::dontSendNotification);
    arpSpeedBox .setSelectedId(getIntParam("ARP_SPEED")  + 1, juce::dontSendNotification);
    progBox     .setSelectedId(getIntParam("PROG_IDX")   + 1, juce::dontSendNotification);
    progChordBox.setSelectedId(getIntParam("PROG_CHORD"),      juce::dontSendNotification);
    int m = getIntParam("SYNTH_MODE");
    if (modeSelector.selectedMode != m) { modeSelector.selectedMode = m; modeSelector.repaint(); }
}

void MorphOneAudioProcessorEditor::timerCallback() { syncCombosFromApvts(); }

// ── Constructor ───────────────────────────────────────────────────────────────
MorphOneAudioProcessorEditor::MorphOneAudioProcessorEditor(MorphOneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      waveDisplay(p.apvts),
      liveChordPanel(p, p.apvts),
      visualKeyboard(p, p.apvts),
      nextSuggestions(p, p.apvts),
      coachDisplay(p.apvts)
{
    setLookAndFeel(&morphLAF);
    auto& apvts = audioProcessor.apvts;

    morphAtt     = std::make_unique<Attach>(apvts, "MORPH",         morphKnob.slider);
    cutoffAtt    = std::make_unique<Attach>(apvts, "FILTER_CUTOFF", cutoffKnob.slider);
    resAtt       = std::make_unique<Attach>(apvts, "FILTER_RES",    resKnob.slider);
    attackAtt    = std::make_unique<Attach>(apvts, "ATTACK",        attackKnob.slider);
    decayAtt     = std::make_unique<Attach>(apvts, "DECAY",         decayKnob.slider);
    sustainAtt   = std::make_unique<Attach>(apvts, "SUSTAIN",       sustainKnob.slider);
    releaseAtt   = std::make_unique<Attach>(apvts, "RELEASE",       releaseKnob.slider);
    unisonAtt    = std::make_unique<Attach>(apvts, "UNISON",        unisonKnob.slider);
    detuneAtt    = std::make_unique<Attach>(apvts, "DETUNE",        detuneKnob.slider);
    octaveAtt    = std::make_unique<Attach>(apvts, "OCTAVE",        octaveKnob.slider);
    lfoRateAtt   = std::make_unique<Attach>(apvts, "LFO_RATE",      lfoRateKnob.slider);
    lfoDepthAtt  = std::make_unique<Attach>(apvts, "LFO_DEPTH",     lfoDepthKnob.slider);
    reverbAtt    = std::make_unique<Attach>(apvts, "REVERB_MIX",    reverbKnob.slider);
    portaAtt     = std::make_unique<Attach>(apvts, "PORTA_TIME",    portaKnob.slider);
    gainAtt      = std::make_unique<Attach>(apvts, "GAIN",          gainKnob.slider);
    arpGateAtt   = std::make_unique<Attach>(apvts, "ARP_GATE",      arpGateKnob.slider);
    scaleLockAtt = std::make_unique<BtnAttach>(apvts, "SCALE_LOCK", scaleLockBtn);
    arpOnAtt     = std::make_unique<BtnAttach>(apvts, "ARP_ON",     arpOnBtn);

    modeSelector.onChange = [this](int mode)
    {
        setIntParam("SYNTH_MODE", mode);
        if (mode == 1 && getIntParam("OCTAVE") == 0)  setIntParam("OCTAVE", -1);
        if (mode == 0 && getIntParam("OCTAVE") == -1) setIntParam("OCTAVE",  0);
    };

    addAndMakeVisible(modeSelector);
    addAndMakeVisible(waveDisplay);
    addAndMakeVisible(liveChordPanel);
    addAndMakeVisible(visualKeyboard);
    addAndMakeVisible(nextSuggestions);
    addAndMakeVisible(coachDisplay);

    presetBox.addItem("-- Select Preset --", 1);
    const auto& ps = PresetManager::getPresets();
    for (int i = 0; i < (int)ps.size(); ++i)
        presetBox.addItem(ps[i].name, i + 2);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    styleCombo(presetBox);
    presetBox.onChange = [this]
    {
        int idx = presetBox.getSelectedId() - 2;
        const auto& pv = PresetManager::getPresets();
        if (idx >= 0 && idx < (int)pv.size())
            PresetManager::applyPreset(pv[idx], audioProcessor.apvts);
    };
    addAndMakeVisible(presetBox);

    for (auto* k : { &morphKnob, &cutoffKnob, &resKnob, &attackKnob, &decayKnob,
                     &sustainKnob, &releaseKnob, &unisonKnob, &detuneKnob,
                     &octaveKnob, &lfoRateKnob, &lfoDepthKnob,
                     &reverbKnob, &portaKnob, &gainKnob, &arpGateKnob })
        addAndMakeVisible(k);

    populateTheoryCombos();
    for (auto* box : { &keyBox, &scaleBox, &chordTypeBox, &chordInvBox,
                       &arpDirBox, &arpSpeedBox, &progBox, &progChordBox })
    {
        styleCombo(*box);
        addAndMakeVisible(box);
    }

    keyBox      .onChange = [this]{ setIntParam("THEORY_KEY", keyBox.getSelectedId() - 1); };
    scaleBox    .onChange = [this]{ setIntParam("SCALE_IDX",  scaleBox.getSelectedId() - 1); };
    chordTypeBox.onChange = [this]{ setIntParam("CHORD_TYPE", chordTypeBox.getSelectedId() - 1); };
    chordInvBox .onChange = [this]{ setIntParam("CHORD_INV",  chordInvBox.getSelectedId() - 1); };
    arpDirBox   .onChange = [this]{ setIntParam("ARP_DIR",    arpDirBox.getSelectedId() - 1); };
    arpSpeedBox .onChange = [this]{ setIntParam("ARP_SPEED",  arpSpeedBox.getSelectedId() - 1); };
    progBox     .onChange = [this]{ setIntParam("PROG_IDX",   progBox.getSelectedId() - 1); };
    progChordBox.onChange = [this]{ setIntParam("PROG_CHORD", progChordBox.getSelectedId()); };

    for (auto* btn : { &scaleLockBtn, &arpOnBtn }) addAndMakeVisible(btn);
    syncCombosFromApvts();
    startTimerHz(10);
    setSize(960, 700);
}

MorphOneAudioProcessorEditor::~MorphOneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// ── Section helper ────────────────────────────────────────────────────────────
void MorphOneAudioProcessorEditor::paintSection(juce::Graphics& g,
                                                 juce::Rectangle<int> b,
                                                 const juce::String& title,
                                                 bool highlight)
{
    juce::ColourGradient bg(juce::Colour(highlight ? 0xff1a1a38 : 0xff161628),
                            b.getX(), b.getY(),
                            juce::Colour(0xff0f0f1e), b.getX(), b.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(b.toFloat(), 6.f);
    g.setColour(juce::Colour(highlight ? Pal::accent : Pal::border).withAlpha(0.7f));
    g.drawRoundedRectangle(b.toFloat().reduced(0.5f), 6.f, 0.8f);

    juce::Rectangle<int> titleBar(b.getX() + 1, b.getY() + 1, b.getWidth() - 2, 20);
    juce::ColourGradient tbg(juce::Colour(highlight ? 0xff261842 : 0xff1c1c36),
                             0, titleBar.getY(),
                             juce::Colour(0xff141428), 0, titleBar.getBottom(), false);
    g.setGradientFill(tbg);
    g.fillRect(titleBar);
    g.setColour(juce::Colour(highlight ? Pal::accent : Pal::border));
    g.drawLine(b.getX() + 1.f, b.getY() + 21.f, b.getRight() - 1.f, b.getY() + 21.f, 1.f);

    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(highlight ? Pal::accentHi : Pal::textLow));
    g.drawText(title, titleBar, juce::Justification::centred);
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void MorphOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    juce::ColourGradient bgGrad(juce::Colour(0xff121224), getWidth() * 0.5f, 0,
                                juce::Colour(Pal::bg), getWidth() * 0.5f, getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    // Header
    juce::ColourGradient hdr(juce::Colour(0xff1e1e40), 0, 0,
                             juce::Colour(0xff121224), 0, 56, false);
    g.setGradientFill(hdr);
    g.fillRect(0, 0, getWidth(), 56);
    g.setColour(juce::Colour(Pal::border));
    g.drawLine(0, 56, (float)getWidth(), 56, 0.8f);

    juce::ColourGradient stripe(juce::Colour(Pal::accent).withAlpha(0.8f), 0, 0,
                                juce::Colour(Pal::accent).withAlpha(0.f), 4, 0, false);
    g.setGradientFill(stripe);
    g.fillRect(0, 0, 4, 56);

    g.setFont(juce::FontOptions(28.f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText("Morph", 14, 8, 98, 38, juce::Justification::centredLeft);
    g.setColour(juce::Colour(Pal::accentHi));
    g.drawText("::One", 110, 8, 86, 38, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(8.f));
    g.setColour(juce::Colour(Pal::textLow));
    g.drawText("v1.3.0", getWidth() - 54, 40, 48, 12, juce::Justification::centredRight);

    // Preset strip
    g.setColour(juce::Colour(0xff0c0c1e));
    g.fillRect(0, 56, getWidth(), 30);
    g.setColour(juce::Colour(0xff1e1e38));
    g.drawLine(0, 86, (float)getWidth(), 86, 0.8f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textLow));
    g.drawText("PRESET", 14, 62, 52, 18, juce::Justification::centredLeft);

    // Left panel sections
    paintSection(g, {  8,  90, 436, 118}, "OSCILLATOR");
    paintSection(g, {  8, 212, 436,  90}, "FILTER");
    paintSection(g, {  8, 306, 436,  90}, "ENVELOPE");
    paintSection(g, {  8, 400, 436,  88}, "UNISON  ·  OCTAVE  ·  LFO");
    paintSection(g, {  8, 492, 436,  86}, "FX  ·  PORTA");

    // Right panel (Theory Coach)
    paintSection(g, {452,  86, 500, 494}, "THEORY COACH", true);

    // Theory strip sections
    paintSection(g, {  8, 584, 232, 108}, "SCALE");
    paintSection(g, {244, 584, 218, 108}, "CHORD");
    paintSection(g, {466, 584, 264, 108}, "ARPEGGIATOR");
    paintSection(g, {734, 584, 218, 108}, "PROGRESSION");

    // Theory strip inline labels
    g.setFont(juce::FontOptions(8.f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textLow));
    const int sy = 584;
    g.drawText("KEY",    12, sy+25, 34, 14, juce::Justification::centredLeft);
    g.drawText("SCALE",  12, sy+49, 40, 14, juce::Justification::centredLeft);
    g.drawText("TYPE",  248, sy+25, 34, 14, juce::Justification::centredLeft);
    g.drawText("INV",   248, sy+49, 28, 14, juce::Justification::centredLeft);
    g.drawText("DIR",   544, sy+25, 28, 14, juce::Justification::centredLeft);
    g.drawText("SPD",   544, sy+49, 28, 14, juce::Justification::centredLeft);
    g.drawText("ARTIST", 738, sy+25, 40, 14, juce::Justification::centredLeft);
    g.drawText("CHORD",  738, sy+49, 38, 14, juce::Justification::centredLeft);
}

// ── Resized ───────────────────────────────────────────────────────────────────
void MorphOneAudioProcessorEditor::resized()
{
    // Header
    modeSelector.setBounds(208, 10, 380, 32);
    presetBox   .setBounds( 70, 62, 620, 20);

    // OSC section (y=90, h=118) — content starts y=112
    waveDisplay.setBounds( 14, 112, 250, 90);
    morphKnob  .setBounds(270, 112, 168, 90);

    // Filter section (y=212, h=90) — content starts y=234
    cutoffKnob.setBounds( 14, 234, 210, 64);
    resKnob   .setBounds(230, 234, 210, 64);

    // Envelope section (y=306, h=90) — content starts y=328
    attackKnob .setBounds( 14, 328, 102, 64);
    decayKnob  .setBounds(118, 328, 102, 64);
    sustainKnob.setBounds(222, 328, 102, 64);
    releaseKnob.setBounds(326, 328, 102, 64);

    // Unison+LFO section (y=400, h=88) — content starts y=422
    unisonKnob  .setBounds( 14, 422,  82, 64);
    detuneKnob  .setBounds( 98, 422,  82, 64);
    octaveKnob  .setBounds(182, 422,  82, 64);
    lfoRateKnob .setBounds(266, 422,  82, 64);
    lfoDepthKnob.setBounds(350, 422,  82, 64);

    // FX section (y=492, h=86) — content starts y=514
    reverbKnob.setBounds( 14, 514, 136, 60);
    portaKnob .setBounds(156, 514, 136, 60);
    gainKnob  .setBounds(298, 514, 136, 60);

    // Theory panel (right side)
    liveChordPanel  .setBounds(456,  90, 492, 118);
    visualKeyboard  .setBounds(456, 212, 492,  90);
    nextSuggestions .setBounds(456, 306, 492,  72);
    coachDisplay    .setBounds(456, 382, 492, 196);

    // Theory strip (y=584)
    const int sy     = 584;
    const int comboH = 20;

    // Scale section (x=8, w=232)
    keyBox      .setBounds( 50, sy+22, 184, comboH);
    scaleBox    .setBounds( 50, sy+46, 184, comboH);
    scaleLockBtn.setBounds( 50, sy+72,  72, 22);

    // Chord section (x=244, w=218)
    chordTypeBox.setBounds(286, sy+22, 166, comboH);
    chordInvBox .setBounds(286, sy+46, 166, comboH);

    // Arpeggiator section (x=466, w=264)
    arpOnBtn   .setBounds(470, sy+22,  70, 20);
    arpGateKnob.setBounds(470, sy+46,  70, 58);
    arpDirBox  .setBounds(548, sy+22, 176, comboH);
    arpSpeedBox.setBounds(548, sy+46, 176, comboH);

    // Progression section (x=734, w=218)
    progBox     .setBounds(782, sy+22, 162, comboH);
    progChordBox.setBounds(782, sy+46, 162, comboH);
}
