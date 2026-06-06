#include "PluginEditor.h"

static const juce::Colour bgColour      { 0xff11112a };
static const juce::Colour sectionColour { 0xff181830 };
static const juce::Colour accentColour  { 0xff7b2fbe };
static const juce::Colour labelColour   { 0xff55557a };

// Mode hint text per mode
static const char* MODE_HINTS[] = {
    "mono · last note · portamento",
    "mono · lowest note · octave -1",
    "poly 4 · balanced",
    "poly 4 · optimised for arp",
    "poly 8 · lush pad · min 4 unison"
};

// ── Helpers ─────────────────────────────────────────────────────────────────

void MorphOneAudioProcessorEditor::styleCombo(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId,     juce::Colour(0xff1a1a38));
    box.setColour(juce::ComboBox::textColourId,           juce::Colours::white);
    box.setColour(juce::ComboBox::outlineColourId,        juce::Colour(0xff3a3a60));
    box.setColour(juce::ComboBox::arrowColourId,          accentColour);
    box.setColour(juce::ComboBox::focusedOutlineColourId, accentColour);
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
    juce::StringArray keys { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    keyBox.addItemList(keys, 1);

    for (int i = 0; i < TheoryEngine::NUM_SCALES; ++i)
        scaleBox.addItem(TheoryEngine::SCALE_NAMES[i], i + 1);

    for (int i = 0; i < TheoryEngine::NUM_CHORD_TYPES; ++i)
        chordTypeBox.addItem(TheoryEngine::CHORD_NAMES[i], i + 1);

    chordInvBox.addItem("Root", 1);
    chordInvBox.addItem("1st",  2);
    chordInvBox.addItem("2nd",  3);
    chordInvBox.addItem("3rd",  4);

    arpDirBox.addItem("Up",      1);
    arpDirBox.addItem("Down",    2);
    arpDirBox.addItem("Up/Down", 3);
    arpDirBox.addItem("Random",  4);

    arpSpeedBox.addItem("1/4",  1);
    arpSpeedBox.addItem("1/8",  2);
    arpSpeedBox.addItem("1/16", 3);

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

    int mode = getIntParam("SYNTH_MODE");
    if (modeSelector.selectedMode != mode)
    {
        modeSelector.selectedMode = mode;
        modeSelector.repaint();
    }
}

void MorphOneAudioProcessorEditor::timerCallback()
{
    syncCombosFromApvts();
}

// ── Constructor ──────────────────────────────────────────────────────────────

MorphOneAudioProcessorEditor::MorphOneAudioProcessorEditor(MorphOneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), waveDisplay(p.apvts)
{
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

    // Mode selector
    modeSelector.onChange = [this](int mode)
    {
        setIntParam("SYNTH_MODE", mode);
        // Bass default: set octave to -1 if currently 0
        if (mode == 1 && getIntParam("OCTAVE") == 0)
            setIntParam("OCTAVE", -1);
        // Lead default: reset octave to 0 if still at bass default
        if (mode == 0 && getIntParam("OCTAVE") == -1)
            setIntParam("OCTAVE", 0);
    };
    addAndMakeVisible(modeSelector);

    // Preset
    presetBox.addItem("-- Select Preset --", 1);
    const auto& presets = PresetManager::getPresets();
    for (int i = 0; i < (int)presets.size(); ++i)
        presetBox.addItem(presets[i].name, i + 2);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    styleCombo(presetBox);
    presetBox.onChange = [this]
    {
        int idx = presetBox.getSelectedId() - 2;
        const auto& ps = PresetManager::getPresets();
        if (idx >= 0 && idx < (int)ps.size())
            PresetManager::applyPreset(ps[idx], audioProcessor.apvts);
    };
    addAndMakeVisible(presetBox);

    addAndMakeVisible(waveDisplay);
    for (auto* k : { &morphKnob, &cutoffKnob, &resKnob, &attackKnob, &decayKnob,
                     &sustainKnob, &releaseKnob, &unisonKnob, &detuneKnob,
                     &octaveKnob, &lfoRateKnob, &lfoDepthKnob,
                     &reverbKnob, &portaKnob, &gainKnob, &arpGateKnob })
        addAndMakeVisible(k);

    // Theory combos
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

    for (auto* btn : { &scaleLockBtn, &arpOnBtn })
    {
        btn->setColour(juce::ToggleButton::textColourId,         juce::Colours::white);
        btn->setColour(juce::ToggleButton::tickColourId,         accentColour);
        btn->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff444466));
        addAndMakeVisible(btn);
    }

    syncCombosFromApvts();
    startTimerHz(10);
    setSize(760, 520);
}

MorphOneAudioProcessorEditor::~MorphOneAudioProcessorEditor()
{
    stopTimer();
}

// ── Paint ────────────────────────────────────────────────────────────────────

void MorphOneAudioProcessorEditor::paintSection(juce::Graphics& g,
                                                 juce::Rectangle<int> bounds,
                                                 const juce::String& title)
{
    g.setColour(sectionColour);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(juce::Colour(0xff242448));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(labelColour);
    g.drawText(title, bounds.removeFromTop(18), juce::Justification::centred);
}

void MorphOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bgColour);

    // Header gradient
    juce::ColourGradient hdr(juce::Colour(0xff1c1c40), 0, 0, bgColour, 0, 46, false);
    g.setGradientFill(hdr);
    g.fillRect(0, 0, getWidth(), 46);

    // Title
    g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText("Morph", 16, 7, 88, 32, juce::Justification::centredLeft);
    g.setColour(accentColour);
    g.drawText("::One", 102, 7, 80, 32, juce::Justification::centredLeft);

    // Mode hint
    int mode = getIntParam("SYNTH_MODE");
    g.setFont(juce::FontOptions(8.5f));
    g.setColour(juce::Colour(0xff555577));
    g.drawText(MODE_HINTS[juce::jlimit(0, 4, mode)],
               200, 32, 350, 12, juce::Justification::centredLeft);

    // Version
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(juce::Colour(0xff333355));
    g.drawText("v1.1.0", getWidth() - 52, 32, 46, 12, juce::Justification::centredRight);

    // Preset strip
    g.setColour(juce::Colour(0xff0e0e24));
    g.fillRect(0, 46, getWidth(), 34);
    g.setColour(juce::Colour(0xff1e1e3e));
    g.drawLine(0, 46, (float)getWidth(), 46, 1.0f);
    g.drawLine(0, 80, (float)getWidth(), 80, 1.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(labelColour);
    g.drawText("PRESET", 16, 52, 55, 20, juce::Justification::centredLeft);

    // Section boxes row 1
    paintSection(g, {   8,  84, 195, 202 }, "OSCILLATOR");
    paintSection(g, { 211,  84, 155, 202 }, "FILTER");
    paintSection(g, { 374,  84, 378, 202 }, "ENVELOPE");

    // Section boxes row 2  (UNISON now wider for Octave)
    paintSection(g, {   8, 294, 252,  98 }, "UNISON");
    paintSection(g, { 268, 294, 162,  98 }, "LFO");
    paintSection(g, { 438, 294, 152,  98 }, "FX");
    paintSection(g, { 598, 294, 154,  98 }, "OUTPUT");

    // Section boxes row 3 — Theory
    paintSection(g, {   8, 400, 175, 112 }, "SCALE LOCK");
    paintSection(g, { 191, 400, 175, 112 }, "CHORD MODE");
    paintSection(g, { 374, 400, 192, 112 }, "ARPEGGIATOR");
    paintSection(g, { 574, 400, 178, 112 }, "PROGRESSION");

    // Row 3 inline labels
    g.setFont(juce::FontOptions(8.5f));
    g.setColour(labelColour);
    g.drawText("KEY",   16,  420, 40, 12, juce::Justification::centredLeft);
    g.drawText("SCALE", 16,  451, 40, 12, juce::Justification::centredLeft);
    g.drawText("TYPE",  199, 420, 35, 12, juce::Justification::centredLeft);
    g.drawText("INV",   199, 453, 35, 12, juce::Justification::centredLeft);
    g.drawText("DIR",   382, 420, 30, 12, juce::Justification::centredLeft);
    g.drawText("SPEED", 382, 453, 35, 12, juce::Justification::centredLeft);
    g.drawText("ARTIST",582, 420, 40, 12, juce::Justification::centredLeft);
    g.drawText("CHORD", 582, 453, 35, 12, juce::Justification::centredLeft);
}

// ── Resized ──────────────────────────────────────────────────────────────────

void MorphOneAudioProcessorEditor::resized()
{
    // Mode selector in header (right side)
    modeSelector.setBounds(200, 6, 360, 28);

    // Preset bar
    presetBox.setBounds(76, 52, 560, 24);

    const int topY = 84, topH = 202;
    const int botY = 294, botH = 98;

    // OSC
    waveDisplay.setBounds(12,  topY + 18, 187, 104);
    morphKnob  .setBounds(47,  topY + 126, 117, topH - 128);

    // Filter
    cutoffKnob.setBounds(213,      topY + 16, 75, topH - 18);
    resKnob   .setBounds(213 + 77, topY + 16, 75, topH - 18);

    // Envelope
    const int eX = 376, eW = 92;
    attackKnob .setBounds(eX,          topY + 16, eW, topH - 18);
    decayKnob  .setBounds(eX + eW,     topY + 16, eW, topH - 18);
    sustainKnob.setBounds(eX + eW * 2, topY + 16, eW, topH - 18);
    releaseKnob.setBounds(eX + eW * 3, topY + 16, eW, topH - 18);

    // Unison (wider: Voices + Detune + Octave)
    const int knobH = botH - 20;
    unisonKnob.setBounds(12,  botY + 18, 76, knobH);
    detuneKnob.setBounds(94,  botY + 18, 76, knobH);
    octaveKnob.setBounds(176, botY + 18, 76, knobH);

    // LFO
    lfoRateKnob .setBounds(272, botY + 18, 74, knobH);
    lfoDepthKnob.setBounds(352, botY + 18, 74, knobH);

    // FX (Reverb + Porta)
    reverbKnob.setBounds(444, botY + 18, 70, knobH);
    portaKnob .setBounds(516, botY + 18, 70, knobH);

    // Output
    gainKnob.setBounds(604, botY + 18, 84, knobH);

    // ── Theory row (y=400) ──
    const int tY = 400, comboH = 20;
    const int col1 = 8, col2 = 191, col3 = 374, col4 = 574;

    keyBox      .setBounds(col1 + 44, tY + 16, 130, comboH);
    scaleBox    .setBounds(col1 + 44, tY + 48, 130, comboH);
    scaleLockBtn.setBounds(col1 + 44, tY + 79,  90, 22);

    chordTypeBox.setBounds(col2 + 38, tY + 16, 124, comboH);
    chordInvBox .setBounds(col2 + 38, tY + 48, 124, comboH);

    arpDirBox  .setBounds(col3 + 38, tY + 16, 120, comboH);
    arpSpeedBox.setBounds(col3 + 38, tY + 48, 120, comboH);
    arpOnBtn   .setBounds(col3 + 38, tY + 79,  55, 22);
    arpGateKnob.setBounds(col3 + 108, tY + 60,  55, 45);

    progBox     .setBounds(col4 + 42, tY + 16, 126, comboH);
    progChordBox.setBounds(col4 + 42, tY + 48, 126, comboH);
}
