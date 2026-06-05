#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class KnobWithLabel : public juce::Component
{
public:
    juce::Slider slider;
    juce::Label  label;

    KnobWithLabel(const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        slider.setColour(juce::Slider::rotarySliderFillColourId,  juce::Colour(0xff7b2fbe));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff3a3a5c));
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffaaaacc));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(11.0f));
        label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaacc));
        addAndMakeVisible(label);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromBottom(18));
        slider.setBounds(area);
    }
};

class MorphOneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MorphOneAudioProcessorEditor(MorphOneAudioProcessor&);
    ~MorphOneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MorphOneAudioProcessor& audioProcessor;

    KnobWithLabel morphKnob   { "Morph" };
    KnobWithLabel attackKnob  { "Attack" };
    KnobWithLabel decayKnob   { "Decay" };
    KnobWithLabel sustainKnob { "Sustain" };
    KnobWithLabel releaseKnob { "Release" };
    KnobWithLabel gainKnob    { "Gain" };

    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attach> morphAttach, attackAttach, decayAttach,
                            sustainAttach, releaseAttach, gainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphOneAudioProcessorEditor)
};
