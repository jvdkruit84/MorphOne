#include "PluginEditor.h"

static const juce::Colour bgColour      { 0xff11112a };
static const juce::Colour sectionColour { 0xff181830 };
static const juce::Colour accentColour  { 0xff7b2fbe };
static const juce::Colour labelColour   { 0xff55557a };

MorphOneAudioProcessorEditor::MorphOneAudioProcessorEditor(MorphOneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), waveDisplay(p.apvts)
{
    auto& apvts = audioProcessor.apvts;

    morphAtt    = std::make_unique<Attach>(apvts, "MORPH",         morphKnob.slider);
    cutoffAtt   = std::make_unique<Attach>(apvts, "FILTER_CUTOFF", cutoffKnob.slider);
    resAtt      = std::make_unique<Attach>(apvts, "FILTER_RES",    resKnob.slider);
    attackAtt   = std::make_unique<Attach>(apvts, "ATTACK",        attackKnob.slider);
    decayAtt    = std::make_unique<Attach>(apvts, "DECAY",         decayKnob.slider);
    sustainAtt  = std::make_unique<Attach>(apvts, "SUSTAIN",       sustainKnob.slider);
    releaseAtt  = std::make_unique<Attach>(apvts, "RELEASE",       releaseKnob.slider);
    unisonAtt   = std::make_unique<Attach>(apvts, "UNISON",        unisonKnob.slider);
    detuneAtt   = std::make_unique<Attach>(apvts, "DETUNE",        detuneKnob.slider);
    lfoRateAtt  = std::make_unique<Attach>(apvts, "LFO_RATE",      lfoRateKnob.slider);
    lfoDepthAtt = std::make_unique<Attach>(apvts, "LFO_DEPTH",     lfoDepthKnob.slider);
    reverbAtt   = std::make_unique<Attach>(apvts, "REVERB_MIX",    reverbKnob.slider);
    gainAtt     = std::make_unique<Attach>(apvts, "GAIN",          gainKnob.slider);

    // Preset dropdown
    presetBox.addItem("-- Select Preset --", 1);
    const auto& presets = PresetManager::getPresets();
    for (int i = 0; i < (int)presets.size(); ++i)
        presetBox.addItem(presets[i].name, i + 2);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a38));
    presetBox.setColour(juce::ComboBox::textColourId,       juce::Colours::white);
    presetBox.setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xff3a3a60));
    presetBox.setColour(juce::ComboBox::arrowColourId,      accentColour);
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
                     &lfoRateKnob, &lfoDepthKnob, &reverbKnob, &gainKnob })
        addAndMakeVisible(k);

    setSize(720, 400);
}

MorphOneAudioProcessorEditor::~MorphOneAudioProcessorEditor() {}

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

    // Header
    juce::ColourGradient hdr(juce::Colour(0xff1c1c40), 0, 0, bgColour, 0, 46, false);
    g.setGradientFill(hdr);
    g.fillRect(0, 0, getWidth(), 46);

    // Preset strip
    g.setColour(juce::Colour(0xff0e0e24));
    g.fillRect(0, 46, getWidth(), 34);
    g.setColour(juce::Colour(0xff1e1e3e));
    g.drawLine(0, 46, (float)getWidth(), 46, 1.0f);
    g.drawLine(0, 80, (float)getWidth(), 80, 1.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(labelColour);
    g.drawText("PRESET", 16, 52, 55, 20, juce::Justification::centredLeft);

    // Title
    g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText("Morph", 16, 7, 88, 32, juce::Justification::centredLeft);
    g.setColour(accentColour);
    g.drawText("::One", 102, 7, 80, 32, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(juce::Colour(0xff333355));
    g.drawText("v0.4.0", getWidth() - 52, 32, 46, 12, juce::Justification::centredRight);

    // Section boxes (y starts at 84)
    paintSection(g, {   8, 84, 195, 202 }, "OSCILLATOR");
    paintSection(g, { 211, 84, 155, 202 }, "FILTER");
    paintSection(g, { 374, 84, 338, 202 }, "ENVELOPE");
    paintSection(g, {   8, 294, 170, 98 }, "UNISON");
    paintSection(g, { 186, 294, 170, 98 }, "LFO");
    paintSection(g, { 364, 294, 170, 98 }, "FX");
    paintSection(g, { 542, 294, 170, 98 }, "OUTPUT");
}

void MorphOneAudioProcessorEditor::resized()
{
    // Preset bar
    presetBox.setBounds(76, 52, 490, 24);

    const int topY = 84;
    const int topH = 202;
    const int botY = 294;
    const int botH = 98;

    // OSC
    waveDisplay.setBounds(12, topY + 18, 187, 104);
    morphKnob.setBounds  (47, topY + 126, 117, topH - 128);

    // Filter
    cutoffKnob.setBounds(213,      topY + 16, 75, topH - 18);
    resKnob.setBounds   (213 + 77, topY + 16, 75, topH - 18);

    // Envelope
    const int eX = 376, eW = 82;
    attackKnob.setBounds (eX,          topY + 16, eW, topH - 18);
    decayKnob.setBounds  (eX + eW,     topY + 16, eW, topH - 18);
    sustainKnob.setBounds(eX + eW * 2, topY + 16, eW, topH - 18);
    releaseKnob.setBounds(eX + eW * 3, topY + 16, eW, topH - 18);

    // Unison
    const int knobH = botH - 20;
    unisonKnob.setBounds (12,  botY + 18, 78, knobH);
    detuneKnob.setBounds (96,  botY + 18, 78, knobH);

    // LFO
    lfoRateKnob.setBounds (190, botY + 18, 78, knobH);
    lfoDepthKnob.setBounds(274, botY + 18, 78, knobH);

    // FX
    reverbKnob.setBounds(404, botY + 18, 90, knobH);

    // Output
    gainKnob.setBounds(582, botY + 18, 90, knobH);
}
