#include "PluginEditor.h"

MorphOneAudioProcessorEditor::MorphOneAudioProcessorEditor(MorphOneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(gainSlider);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "GAIN", gainSlider);

    setSize(300, 200);
}

MorphOneAudioProcessorEditor::~MorphOneAudioProcessorEditor() {}

void MorphOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawFittedText("Morph::One", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
}

void MorphOneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(50);

    gainLabel.setBounds(area.removeFromTop(20));
    gainSlider.setBounds(area.withSizeKeepingCentre(120, 120));
}
