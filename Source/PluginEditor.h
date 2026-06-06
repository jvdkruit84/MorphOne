#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WavetableOscillator.h"
#include "PresetManager.h"

// Animated waveform display
class WavetableDisplay : public juce::Component, private juce::Timer
{
public:
    WavetableDisplay(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        startTimerHz(30);
    }

    void timerCallback() override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        float morph = apvts.getRawParameterValue("MORPH")->load();

        g.fillAll(juce::Colour(0xff0d0d22));
        g.setColour(juce::Colour(0xff1e1e3a));
        g.drawRect(getLocalBounds());

        auto b = getLocalBounds().reduced(3).toFloat();
        juce::Path path;
        int w = getWidth();
        for (int x = 0; x <= w; ++x)
        {
            float sample = WavetableOscillator::sampleAt((float)x / w, morph);
            float y = b.getCentreY() - sample * b.getHeight() * 0.44f;
            x == 0 ? path.startNewSubPath((float)x, y) : path.lineTo((float)x, y);
        }

        juce::ColourGradient lineGrad(juce::Colour(0xff9b4fde), 0, 0,
                                      juce::Colour(0xff4f8fde), (float)w, 0, false);
        g.setGradientFill(lineGrad);
        g.strokePath(path, juce::PathStrokeType(2.0f));

        const char* names[] = { "SINE", "TRIANGLE", "SAW", "SQUARE" };
        int idx = juce::jlimit(0, 3, (int)std::round(morph * 3.0f));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff7b5fae));
        g.drawText(names[idx], getLocalBounds().reduced(4), juce::Justification::bottomRight);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
};

// Knob + label component
class KnobWithLabel : public juce::Component
{
public:
    juce::Slider slider;
    juce::Label  label;

    KnobWithLabel(const juce::String& name, bool isInt = false)
    {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
        if (isInt) slider.setNumDecimalPlacesToDisplay(0);

        slider.setColour(juce::Slider::rotarySliderFillColourId,    juce::Colour(0xff7b2fbe));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2e2e50));
        slider.setColour(juce::Slider::thumbColourId,               juce::Colours::white);
        slider.setColour(juce::Slider::textBoxTextColourId,         juce::Colour(0xff9999bb));
        slider.setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(10.0f));
        label.setColour(juce::Label::textColourId, juce::Colour(0xff8888aa));
        addAndMakeVisible(label);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromBottom(16));
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

    // Preset bar
    juce::ComboBox presetBox;

    // Sections
    WavetableDisplay waveDisplay;

    KnobWithLabel morphKnob    { "Morph"   };
    KnobWithLabel cutoffKnob   { "Cutoff"  };
    KnobWithLabel resKnob      { "Res"     };
    KnobWithLabel attackKnob   { "Attack"  };
    KnobWithLabel decayKnob    { "Decay"   };
    KnobWithLabel sustainKnob  { "Sustain" };
    KnobWithLabel releaseKnob  { "Release" };
    KnobWithLabel unisonKnob   { "Voices", true };
    KnobWithLabel detuneKnob   { "Detune"  };
    KnobWithLabel lfoRateKnob  { "Rate"    };
    KnobWithLabel lfoDepthKnob { "Depth"   };
    KnobWithLabel reverbKnob   { "Reverb"  };
    KnobWithLabel gainKnob     { "Gain"    };

    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attach> morphAtt, cutoffAtt, resAtt, attackAtt, decayAtt,
                            sustainAtt, releaseAtt, unisonAtt, detuneAtt,
                            lfoRateAtt, lfoDepthAtt, reverbAtt, gainAtt;

    void paintSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphOneAudioProcessorEditor)
};
