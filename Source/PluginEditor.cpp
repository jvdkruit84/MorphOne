#include "PluginEditor.h"

MorphOneAudioProcessorEditor::MorphOneAudioProcessorEditor(MorphOneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    auto& apvts = audioProcessor.apvts;

    morphAttach   = std::make_unique<Attach>(apvts, "MORPH",   morphKnob.slider);
    attackAttach  = std::make_unique<Attach>(apvts, "ATTACK",  attackKnob.slider);
    decayAttach   = std::make_unique<Attach>(apvts, "DECAY",   decayKnob.slider);
    sustainAttach = std::make_unique<Attach>(apvts, "SUSTAIN", sustainKnob.slider);
    releaseAttach = std::make_unique<Attach>(apvts, "RELEASE", releaseKnob.slider);
    gainAttach    = std::make_unique<Attach>(apvts, "GAIN",    gainKnob.slider);

    for (auto* k : { &morphKnob, &attackKnob, &decayKnob, &sustainKnob, &releaseKnob, &gainKnob })
        addAndMakeVisible(k);

    setSize(580, 220);
}

MorphOneAudioProcessorEditor::~MorphOneAudioProcessorEditor() {}

void MorphOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12122a));

    // Header gradient
    juce::ColourGradient grad(juce::Colour(0xff1e1e40), 0, 0,
                              juce::Colour(0xff12122a), 0, 50, false);
    g.setGradientFill(grad);
    g.fillRect(0, 0, getWidth(), 50);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("Morph", 20, 10, 90, 30, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff7b2fbe));
    g.drawText("::One", 107, 10, 80, 30, juce::Justification::centredLeft);

    // Divider line between morph and ADSR
    g.setColour(juce::Colour(0xff2a2a4a));
    g.drawLine(150, 55, 150, getHeight() - 10, 1.0f);
    g.drawLine(490, 55, 490, getHeight() - 10, 1.0f);

    // Section labels
    g.setFont(juce::FontOptions(9.5f));
    g.setColour(juce::Colour(0xff555577));
    g.drawText("OSCILLATOR", 20, 52, 130, 12, juce::Justification::centred);
    g.drawText("ENVELOPE", 160, 52, 320, 12, juce::Justification::centred);
    g.drawText("OUTPUT", 495, 52, 80, 12, juce::Justification::centred);
}

void MorphOneAudioProcessorEditor::resized()
{
    const int top = 60;
    const int h   = getHeight() - top - 10;

    morphKnob.setBounds(10, top, 130, h);

    const int adsrX = 155;
    const int adsrW = 80;
    attackKnob.setBounds (adsrX,             top, adsrW, h);
    decayKnob.setBounds  (adsrX + adsrW,     top, adsrW, h);
    sustainKnob.setBounds(adsrX + adsrW * 2, top, adsrW, h);
    releaseKnob.setBounds(adsrX + adsrW * 3, top, adsrW, h);

    gainKnob.setBounds(495, top, 80, h);
}
