#include "PluginEditor.h"

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
    float x = lb.getX() + 4.f, bw = lb.getWidth() - 8.f;
    float y = lb.getY() + 4.f;

    // ── Mood badge ──
    Mood mood = getMood(si, ct);
    auto mi   = getMoodInfo(mood);
    float badgeW = 90.f;
    juce::Rectangle<float> badge(x + bw - badgeW, y + 2.f, badgeW, 18.f);
    g.setColour(mi.colour.withAlpha(0.18f));
    g.fillRoundedRectangle(badge, 9.f);
    g.setColour(mi.colour.withAlpha(0.7f));
    g.drawRoundedRectangle(badge.reduced(0.5f), 9.f, 0.7f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(mi.colour);
    g.drawText(mi.name, badge, juce::Justification::centred);

    // ── Key + Scale ──
    static const char* kn[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    g.setFont(juce::FontOptions(20.f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textHi));
    g.drawText(juce::String(kn[key % 12]) + "  " + juce::String(TheoryEngine::SCALE_NAMES[si]),
               x, y, bw - badgeW - 4.f, 26.f, juce::Justification::centredLeft);

    // Lock & arp badges
    float badgeX = x;
    float badgeY = y + 28.f;
    auto drawBadge = [&](const juce::String& txt, juce::Colour col)
    {
        float tw = g.getCurrentFont().getStringWidth(txt) + 10.f;
        juce::Rectangle<float> bb(badgeX, badgeY, tw, 14.f);
        g.setColour(col.withAlpha(0.2f));
        g.fillRoundedRectangle(bb, 7.f);
        g.setColour(col.withAlpha(0.8f));
        g.drawText(txt, bb, juce::Justification::centred);
        badgeX += tw + 4.f;
    };
    g.setFont(juce::FontOptions(8.f, juce::Font::bold));
    if (lock) drawBadge("SCALE LOCK", juce::Colour(Pal::accent));
    if (arp)  drawBadge("ARP ON",     juce::Colour(0xff4a9eff));
    if (ct>0) drawBadge(TheoryEngine::CHORD_NAMES[ct], juce::Colour(0xff44cc88));

    // ── 12-note semitone pills ──
    float pillY = y + 48.f;
    float pillH = 20.f;
    float pillW = bw / 12.f;
    const auto& intervals = TheoryEngine::SCALE_INTERVALS[juce::jlimit(0, 14, si)];

    for (int i = 0; i < 12; ++i)
    {
        juce::Rectangle<float> pill(x + i * pillW, pillY, pillW - 1.f, pillH);
        int deg = ((i - key) % 12 + 12) % 12;
        bool inScale = std::find(intervals.begin(), intervals.end(), deg) != intervals.end();
        bool isRoot  = (i == key % 12);

        if (isRoot)
        {
            g.setColour(juce::Colour(Pal::pillRoot));
            g.fillRoundedRectangle(pill, 4.f);
        }
        else if (inScale)
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
        g.setFont(juce::FontOptions(7.f, inScale ? juce::Font::bold : 0));
        g.setColour(isRoot ? juce::Colours::white :
                    inScale ? juce::Colour(0xffcc99ff) : juce::Colour(0xff2a2a45));
        g.drawText(kn[i], pill, juce::Justification::centred);
    }

    // ── Tension bar ──
    float tensY = pillY + pillH + 8.f;
    float ten   = getTension(si, ct);
    g.setFont(juce::FontOptions(8.f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textLow));
    g.drawText("TENSION", x, tensY, 52.f, 13.f, juce::Justification::centredLeft);

    juce::Rectangle<float> mbg(x + 56.f, tensY + 1.5f, bw - 56.f, 10.f);
    g.setColour(juce::Colour(0xff111120));
    g.fillRoundedRectangle(mbg, 5.f);
    if (ten > 0.01f)
    {
        juce::Rectangle<float> mfill(mbg.getX(), mbg.getY(), mbg.getWidth() * ten, mbg.getHeight());
        juce::ColourGradient tg(ten < 0.4f ? juce::Colour(0xff2a8a3a) :
                                ten < 0.7f ? juce::Colour(0xffcc8820) : juce::Colour(0xffcc3a2a),
                                mfill.getX(), 0.f, juce::Colour(Pal::accent), mfill.getRight(), 0.f, false);
        g.setGradientFill(tg);
        g.fillRoundedRectangle(mfill, 5.f);
    }

    // ── Progression status ──
    float infoY = tensY + 20.f;
    g.setFont(juce::FontOptions(9.f, juce::Font::bold));
    if (prog > 0)
    {
        juce::Rectangle<float> progBadge(x, infoY, bw, 16.f);
        g.setColour(juce::Colour(Pal::accent).withAlpha(0.18f));
        g.fillRoundedRectangle(progBadge, 4.f);
        g.setColour(juce::Colour(Pal::accentHi));
        g.drawText(juce::String("▶  ") + TheoryEngine::ARTIST_PROGS[prog].name + "  progressie",
                   progBadge.reduced(4.f, 0.f), juce::Justification::centredLeft);
        infoY += 20.f;
    }

    // ── Coach tip box ──
    float tipY = infoY + 2.f;
    float tipH = lb.getBottom() - tipY - 8.f;
    if (tipH > 20.f)
    {
        juce::Rectangle<float> tipBox(x, tipY, bw, tipH);
        g.setColour(juce::Colour(0xff0e0e1e));
        g.fillRoundedRectangle(tipBox, 4.f);
        g.setColour(juce::Colour(Pal::border));
        g.drawRoundedRectangle(tipBox.reduced(0.5f), 4.f, 0.6f);

        // Coach icon
        g.setFont(juce::FontOptions(8.f, juce::Font::bold));
        g.setColour(juce::Colour(Pal::accentHi));
        g.drawText("♪  TIP", tipBox.reduced(5.f, 3.f), juce::Justification::topLeft);

        g.setFont(juce::FontOptions(8.f));
        g.setColour(juce::Colour(Pal::textMid));
        auto contentBox = tipBox.reduced(6.f, 4.f).withTrimmedTop(13.f);
        g.drawFittedText(getCoachTip(key, si, ct, lock, arp, prog, mode),
                         contentBox.toNearestInt(), juce::Justification::topLeft, 4);
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
{
    return (int)audioProcessor.apvts.getRawParameterValue(id)->load();
}

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
        chordInvBox.addItem(juce::String(i == 1 ? "Root" : i == 2 ? "1st" : i == 3 ? "2nd" : "3rd"), i);
    arpDirBox.addItemList({ "Up", "Down", "Up/Down", "Random" }, 1);
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
      waveDisplay(p.apvts), coachDisplay(p.apvts)
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
    addAndMakeVisible(coachDisplay);

    // Preset
    presetBox.addItem("-- Select Preset --", 1);
    const auto& ps = PresetManager::getPresets();
    for (int i = 0; i < (int)ps.size(); ++i) presetBox.addItem(ps[i].name, i + 2);
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
    setSize(960, 580);
}

MorphOneAudioProcessorEditor::~MorphOneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void MorphOneAudioProcessorEditor::paintSection(juce::Graphics& g,
                                                 juce::Rectangle<int> b,
                                                 const juce::String& title,
                                                 bool highlight)
{
    // Panel body
    juce::ColourGradient bg(juce::Colour(highlight ? 0xff1c1c38 : 0xff161628),
                            b.getX(), b.getY(),
                            juce::Colour(0xff0f0f1e), b.getX(), b.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(b.toFloat(), 6.f);

    // Border
    g.setColour(juce::Colour(Pal::border));
    g.drawRoundedRectangle(b.toFloat().reduced(0.5f), 6.f, 0.8f);

    // Title bar
    juce::Rectangle<int> titleBar(b.getX() + 1, b.getY() + 1, b.getWidth() - 2, 20);
    juce::ColourGradient tbg(juce::Colour(highlight ? 0xff2a1a44 : 0xff1e1e38),
                             0, titleBar.getY(),
                             juce::Colour(0xff141428), 0, titleBar.getBottom(), false);
    g.setGradientFill(tbg);
    g.fillRect(titleBar);

    // Accent line under title
    g.setColour(juce::Colour(highlight ? Pal::accent : Pal::border));
    g.drawLine(b.getX() + 1.f, b.getY() + 21.f, b.getRight() - 1.f, b.getY() + 21.f, 1.f);

    // Title text
    g.setFont(juce::FontOptions(9.f, juce::Font::bold));
    g.setColour(juce::Colour(highlight ? Pal::accentHi : Pal::textLow));
    g.drawText(title, titleBar, juce::Justification::centred);
}

void MorphOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    juce::ColourGradient bgGrad(juce::Colour(0xff121224), getWidth() * 0.5f, 0,
                                juce::Colour(Pal::bg), getWidth() * 0.5f, getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    // ── Header ──
    juce::ColourGradient hdr(juce::Colour(0xff1e1e40), 0, 0,
                             juce::Colour(0xff121224), 0, 56, false);
    g.setGradientFill(hdr);
    g.fillRect(0, 0, getWidth(), 56);
    g.setColour(juce::Colour(Pal::border));
    g.drawLine(0, 56, (float)getWidth(), 56, 0.8f);

    // Accent stripe left
    juce::ColourGradient stripe(juce::Colour(Pal::accent).withAlpha(0.8f), 0, 0,
                                juce::Colour(Pal::accent).withAlpha(0.f), 4, 0, false);
    g.setGradientFill(stripe);
    g.fillRect(0, 0, 4, 56);

    // Title
    g.setFont(juce::FontOptions(28.f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText("Morph", 14, 8, 98, 38, juce::Justification::centredLeft);
    g.setColour(juce::Colour(Pal::accentHi));
    g.drawText("::One", 110, 8, 86, 38, juce::Justification::centredLeft);

    // Version
    g.setFont(juce::FontOptions(8.f));
    g.setColour(juce::Colour(Pal::textLow));
    g.drawText("v1.2.0", getWidth() - 54, 40, 48, 12, juce::Justification::centredRight);

    // ── Preset strip ──
    g.setColour(juce::Colour(0xff0c0c1e));
    g.fillRect(0, 56, getWidth(), 30);
    g.setColour(juce::Colour(0xff1e1e38));
    g.drawLine(0, 86, (float)getWidth(), 86, 0.8f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textLow));
    g.drawText("PRESET", 14, 62, 52, 18, juce::Justification::centredLeft);

    // ── Section boxes row 1 ──
    const int r1y = 94, r1h = 216;
    paintSection(g, {   8, r1y, 190, r1h }, "OSCILLATOR");
    paintSection(g, { 206, r1y, 144, r1h }, "FILTER");
    paintSection(g, { 358, r1y, 272, r1h }, "ENVELOPE");
    paintSection(g, { 638, r1y, 314, r1h }, "THEORY COACH", true);

    // ── Section boxes row 2 ──
    const int r2y = 318, r2h = 104;
    paintSection(g, {   8, r2y, 252, r2h }, "UNISON  ·  OCTAVE");
    paintSection(g, { 268, r2y, 164, r2h }, "LFO");
    paintSection(g, { 440, r2y, 162, r2h }, "FX  ·  PORTA");
    paintSection(g, { 610, r2y, 342, r2h }, "OUTPUT");

    // ── Section boxes row 3 ──
    const int r3y = 430, r3h = 140;
    paintSection(g, {   8, r3y, 184, r3h }, "SCALE LOCK");
    paintSection(g, { 200, r3y, 184, r3h }, "CHORD MODE");
    paintSection(g, { 392, r3y, 198, r3h }, "ARPEGGIATOR");
    paintSection(g, { 598, r3y, 354, r3h }, "PROGRESSION");

    // Row 3 inline field labels
    g.setFont(juce::FontOptions(8.f, juce::Font::bold));
    g.setColour(juce::Colour(Pal::textLow));
    auto lab = [&](const juce::String& t, int lx, int ly) {
        g.drawText(t, lx, ly, 45, 12, juce::Justification::centredLeft);
    };
    lab("KEY",   16,  452);  lab("SCALE", 16,  482);
    lab("TYPE",  208, 452);  lab("INV",   208, 482);
    lab("DIR",   400, 452);  lab("SPEED", 400, 482);
    lab("ARTIST",606, 452);  lab("CHORD", 606, 482);
}

// ── Resized ───────────────────────────────────────────────────────────────────
void MorphOneAudioProcessorEditor::resized()
{
    // Header
    modeSelector.setBounds(208, 10, 380, 32);
    presetBox   .setBounds(70,  62, 570, 20);

    const int r1y = 94, r1h = 216;
    const int r2y = 318, r2h = 104;
    const int r3y = 430;
    const int comboH = 20;
    const int knobH  = r2h - 22;

    // ── OSC ──
    waveDisplay.setBounds(14, r1y + 22, 178, 108);
    morphKnob  .setBounds(44, r1y + 136, 118, r1h - 138);

    // ── Filter ──
    cutoffKnob.setBounds(210,      r1y + 22, 68, r1h - 24);
    resKnob   .setBounds(210 + 70, r1y + 22, 68, r1h - 24);

    // ── Envelope ──
    const int eX = 362, eW = 66;
    attackKnob .setBounds(eX,          r1y + 22, eW, r1h - 24);
    decayKnob  .setBounds(eX + eW,     r1y + 22, eW, r1h - 24);
    sustainKnob.setBounds(eX + eW * 2, r1y + 22, eW, r1h - 24);
    releaseKnob.setBounds(eX + eW * 3, r1y + 22, eW, r1h - 24);

    // ── Coach ──
    coachDisplay.setBounds(638, r1y, 314, r1h);

    // ── Unison + Octave ──
    unisonKnob.setBounds(12,  r2y + 22, 76, knobH);
    detuneKnob.setBounds(92,  r2y + 22, 76, knobH);
    octaveKnob.setBounds(172, r2y + 22, 76, knobH);

    // ── LFO ──
    lfoRateKnob .setBounds(272, r2y + 22, 76, knobH);
    lfoDepthKnob.setBounds(352, r2y + 22, 76, knobH);

    // ── FX ──
    reverbKnob.setBounds(448, r2y + 22, 72, knobH);
    portaKnob .setBounds(524, r2y + 22, 72, knobH);

    // ── Output ──
    gainKnob.setBounds(660, r2y + 22, 90, knobH);

    // ── Theory row ──
    keyBox      .setBounds(52,  r3y + 24, 132, comboH);
    scaleBox    .setBounds(52,  r3y + 54, 132, comboH);
    scaleLockBtn.setBounds(52,  r3y + 84,  92, 22);

    chordTypeBox.setBounds(244, r3y + 24, 132, comboH);
    chordInvBox .setBounds(244, r3y + 54, 132, comboH);

    arpDirBox  .setBounds(436, r3y + 24, 146, comboH);
    arpSpeedBox.setBounds(436, r3y + 54, 146, comboH);
    arpOnBtn   .setBounds(436, r3y + 84,  60, 22);
    arpGateKnob.setBounds(510, r3y + 68,  72, 60);

    progBox     .setBounds(642, r3y + 24, 160, comboH);
    progChordBox.setBounds(642, r3y + 54, 160, comboH);
}
