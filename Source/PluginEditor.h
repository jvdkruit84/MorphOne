#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MorphOneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MorphOneAudioProcessorEditor(MorphOneAudioProcessor&);
    ~MorphOneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MorphOneAudioProcessor& audioProcessor;

    juce::Slider gainSlider;
    juce::Label gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphOneAudioProcessorEditor)
};
