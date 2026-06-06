#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WavetableOscillator.h"
#include "PresetManager.h"
#include "TheoryEngine.h"

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
        juce::ColourGradient grad(juce::Colour(0xff9b4fde), 0, 0,
                                  juce::Colour(0xff4f8fde), (float)w, 0, false);
        g.setGradientFill(grad);
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

// Knob + label
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

// Mode selector — 5 pill buttons (Lead | Bass | Melody | Arp | Pad)
class ModeSelector : public juce::Component
{
public:
    static constexpr int NUM_MODES = 5;
    static constexpr const char* MODE_NAMES[NUM_MODES] = { "LEAD", "BASS", "MELODY", "ARP", "PAD" };

    int selectedMode = 2;
    std::function<void(int)> onChange;

    void paint(juce::Graphics& g) override
    {
        static const juce::Colour accent { 0xff7b2fbe };
        float btnW = getWidth() / (float)NUM_MODES;
        float h    = (float)getHeight();

        for (int i = 0; i < NUM_MODES; ++i)
        {
            juce::Rectangle<float> btn(i * btnW + 1.f, 1.f, btnW - 2.f, h - 2.f);
            bool sel = (i == selectedMode);

            if (sel)
            {
                g.setColour(accent);
                g.fillRoundedRectangle(btn, 4.0f);
            }
            else
            {
                g.setColour(juce::Colour(0xff1e1e3a));
                g.fillRoundedRectangle(btn, 4.0f);
                g.setColour(juce::Colour(0xff3a3a60));
                g.drawRoundedRectangle(btn.reduced(0.5f), 4.0f, 0.8f);
            }

            g.setFont(juce::FontOptions(9.5f, sel ? juce::Font::bold : 0));
            g.setColour(sel ? juce::Colours::white : juce::Colour(0xff7777aa));
            g.drawText(MODE_NAMES[i], btn, juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int mode = (int)(e.x / (getWidth() / (float)NUM_MODES));
        mode = juce::jlimit(0, NUM_MODES - 1, mode);
        if (mode != selectedMode)
        {
            selectedMode = mode;
            repaint();
            if (onChange) onChange(selectedMode);
        }
    }
};

class MorphOneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    MorphOneAudioProcessorEditor(MorphOneAudioProcessor&);
    ~MorphOneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MorphOneAudioProcessor& audioProcessor;

    void timerCallback() override;
    void paintSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label);
    void styleCombo(juce::ComboBox& box);
    int  getIntParam(const juce::String& id);
    void setIntParam(const juce::String& id, int val);
    void populateTheoryCombos();
    void syncCombosFromApvts();

    // ── Preset bar ──
    juce::ComboBox presetBox;

    // ── Mode selector ──
    ModeSelector modeSelector;

    // ── Waveform display ──
    WavetableDisplay waveDisplay;

    // ── Synth knobs ──
    KnobWithLabel morphKnob    { "Morph"   };
    KnobWithLabel cutoffKnob   { "Cutoff"  };
    KnobWithLabel resKnob      { "Res"     };
    KnobWithLabel attackKnob   { "Attack"  };
    KnobWithLabel decayKnob    { "Decay"   };
    KnobWithLabel sustainKnob  { "Sustain" };
    KnobWithLabel releaseKnob  { "Release" };
    KnobWithLabel unisonKnob   { "Voices",  true };
    KnobWithLabel detuneKnob   { "Detune"  };
    KnobWithLabel octaveKnob   { "Octave",  true };
    KnobWithLabel lfoRateKnob  { "Rate"    };
    KnobWithLabel lfoDepthKnob { "Depth"   };
    KnobWithLabel reverbKnob   { "Reverb"  };
    KnobWithLabel portaKnob    { "Porta"   };
    KnobWithLabel gainKnob     { "Gain"    };
    KnobWithLabel arpGateKnob  { "Gate"    };

    using Attach    = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BtnAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<Attach> morphAtt, cutoffAtt, resAtt, attackAtt, decayAtt,
                            sustainAtt, releaseAtt, unisonAtt, detuneAtt,
                            octaveAtt, lfoRateAtt, lfoDepthAtt, reverbAtt,
                            portaAtt, gainAtt, arpGateAtt;

    // ── Theory combos ──
    juce::ComboBox keyBox, scaleBox;
    juce::ComboBox chordTypeBox, chordInvBox;
    juce::ComboBox arpDirBox, arpSpeedBox;
    juce::ComboBox progBox, progChordBox;

    juce::ToggleButton scaleLockBtn { "Lock" };
    juce::ToggleButton arpOnBtn     { "On"   };

    std::unique_ptr<BtnAttach> scaleLockAtt, arpOnAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphOneAudioProcessorEditor)
};
