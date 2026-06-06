#include "PluginEditor.h"

static const juce::Colour bgColour      { 0xff11112a };
static const juce::Colour sectionColour { 0xff181830 };
static const juce::Colour accentColour  { 0xff7b2fbe };
static const juce::Colour labelColour   { 0xff55557a };

MorphOneAudioProcessorEditor::MorphOneAudioProcessorEditor(MorphOneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), waveDisplay(p.apvts)
{
    auto& apvts = audioProcessor.apvts;

    morphAtt    = std::make_unique<Attach>(apvts, "MORPH",        morphKnob.slider);
    cutoffAtt   = std::make_unique<Attach>(apvts, "FILTER_CUTOFF",cutoffKnob.slider);
    resAtt      = std::make_unique<Attach>(apvts, "FILTER_RES",   resKnob.slider);
    attackAtt   = std::make_unique<Attach>(apvts, "ATTACK",       attackKnob.slider);
    decayAtt    = std::make_unique<Attach>(apvts, "DECAY",        decayKnob.slider);
    sustainAtt  = std::make_unique<Attach>(apvts, "SUSTAIN",      sustainKnob.slider);
    releaseAtt  = std::make_unique<Attach>(apvts, "RELEASE",      releaseKnob.slider);
    unisonAtt   = std::make_unique<Attach>(apvts, "UNISON",       unisonKnob.slider);
    detuneAtt   = std::make_unique<Attach>(apvts, "DETUNE",       detuneKnob.slider);
    lfoRateAtt  = std::make_unique<Attach>(apvts, "LFO_RATE",     lfoRateKnob.slider);
    lfoDepthAtt = std::make_unique<Attach>(apvts, "LFO_DEPTH",    lfoDepthKnob.slider);
    reverbAtt   = std::make_unique<Attach>(apvts, "REVERB_MIX",   reverbKnob.slider);
    gainAtt     = std::make_unique<Attach>(apvts, "GAIN",         gainKnob.slider);

    addAndMakeVisible(waveDisplay);
    for (auto* k : { &morphKnob, &cutoffKnob, &resKnob, &attackKnob, &decayKnob,
                     &sustainKnob, &releaseKnob, &unisonKnob, &detuneKnob,
                     &lfoRateKnob, &lfoDepthKnob, &reverbKnob, &gainKnob })
        addAndMakeVisible(k);

    setSize(720, 370);
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

    // Header gradient
    juce::ColourGradient hdr(juce::Colour(0xff1c1c40), 0, 0,
                             bgColour, 0, 46, false);
    g.setGradientFill(hdr);
    g.fillRect(0, 0, getWidth(), 46);

    // Title
    g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText("Morph", 16, 7, 88, 32, juce::Justification::centredLeft);
    g.setColour(accentColour);
    g.drawText("::One", 102, 7, 80, 32, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(juce::Colour(0xff333355));
    g.drawText("v0.3.0", getWidth() - 52, 32, 46, 12, juce::Justification::centredRight);

    // Section backgrounds (positions match resized())
    paintSection(g, { 8,   48, 195, 202 }, "OSCILLATOR");
    paintSection(g, { 211, 48, 155, 202 }, "FILTER");
    paintSection(g, { 374, 48, 338, 202 }, "ENVELOPE");
    paintSection(g, { 8,   258, 170, 104 }, "UNISON");
    paintSection(g, { 186, 258, 170, 104 }, "LFO");
    paintSection(g, { 364, 258, 170, 104 }, "FX");
    paintSection(g, { 542, 258, 170, 104 }, "OUTPUT");
}

void MorphOneAudioProcessorEditor::resized()
{
    const int headerH = 48;
    const int topH    = 202;
    const int botY    = 258;
    const int botH    = 104;
    const int knobH   = botH - 20;

    // ── OSC section ──────────────────────────────
    waveDisplay.setBounds(12, headerH + 18, 187, 105);
    morphKnob.setBounds  (47, headerH + 126, 117, topH - 128);

    // ── FILTER section ───────────────────────────
    const int fX = 213;
    cutoffKnob.setBounds(fX,      headerH + 16, 73, topH - 18);
    resKnob.setBounds   (fX + 75, headerH + 16, 73, topH - 18);

    // ── ENVELOPE section ─────────────────────────
    const int eX = 376;
    const int eW = 82;
    attackKnob.setBounds (eX,          headerH + 16, eW, topH - 18);
    decayKnob.setBounds  (eX + eW,     headerH + 16, eW, topH - 18);
    sustainKnob.setBounds(eX + eW * 2, headerH + 16, eW, topH - 18);
    releaseKnob.setBounds(eX + eW * 3, headerH + 16, eW, topH - 18);

    // ── UNISON section ───────────────────────────
    unisonKnob.setBounds (12,  botY + 18, 78, knobH);
    detuneKnob.setBounds (96,  botY + 18, 78, knobH);

    // ── LFO section ──────────────────────────────
    lfoRateKnob.setBounds (190, botY + 18, 78, knobH);
    lfoDepthKnob.setBounds(274, botY + 18, 78, knobH);

    // ── FX section ───────────────────────────────
    reverbKnob.setBounds(404, botY + 18, 90, knobH);

    // ── OUTPUT section ───────────────────────────
    gainKnob.setBounds(582, botY + 18, 90, knobH);
}
