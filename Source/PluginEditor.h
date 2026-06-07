#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WavetableOscillator.h"
#include "PresetManager.h"
#include "TheoryEngine.h"
#include "ChordDetector.h"
#include "BinaryData.h"

// ── Colour palette ────────────────────────────────────────────────────────────
namespace Pal
{
    constexpr juce::uint32
        bg         = 0xff08080e,
        bgSection  = 0xff141422,
        bgDark     = 0xff060608,
        border     = 0xff303060,
        borderHi   = 0xff4858a0,
        accent     = 0xff1a72e0,   // blue — no purple
        accentHi   = 0xff4da8ff,   // light blue
        accentLo   = 0xff0a3888,   // dark blue
        textHi     = 0xffffffff,
        textMid    = 0xffb8c8e0,
        textLow    = 0xff6878a0,
        pillIn     = 0xff1a3870,   // blue in-scale pill
        pillRoot   = 0xff1a72e0,   // blue root pill
        pillOut    = 0xff111120;
}

// ── Custom LookAndFeel ────────────────────────────────────────────────────────
class MorphOneLAF : public juce::LookAndFeel_V4
{
public:
    MorphOneLAF()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Pal::bg));
        setColour(juce::Label::textColourId,                 juce::Colour(Pal::textMid));
        setColour(juce::Slider::textBoxTextColourId,         juce::Colour(Pal::textMid));
        setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId,        juce::Colour(0xff141428));
        setColour(juce::ComboBox::textColourId,              juce::Colour(Pal::textHi));
        setColour(juce::ComboBox::outlineColourId,           juce::Colour(Pal::border));
        setColour(juce::ComboBox::arrowColourId,             juce::Colour(Pal::accentHi));
        setColour(juce::PopupMenu::backgroundColourId,       juce::Colour(0xff141428));
        setColour(juce::PopupMenu::textColourId,             juce::Colour(Pal::textHi));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Pal::accent));
        setColour(juce::ToggleButton::textColourId,          juce::Colour(Pal::textMid));
        setColour(juce::ToggleButton::tickColourId,          juce::Colour(Pal::accentHi));
        setColour(juce::ToggleButton::tickDisabledColourId,  juce::Colour(Pal::border));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        auto b  = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4.f);
        float cx = b.getCentreX(), cy = b.getCentreY();
        float r  = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
        float aw = r * 0.20f;

        juce::Path track;
        track.addCentredArc(cx, cy, r - aw * 0.5f, r - aw * 0.5f, 0.f, startAngle, endAngle, true);
        g.setColour(juce::Colour(0xff181828));
        g.strokePath(track, juce::PathStrokeType(aw, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        float valAngle = startAngle + sliderPos * (endAngle - startAngle);
        if (sliderPos > 0.001f)
        {
            juce::Path arc;
            arc.addCentredArc(cx, cy, r - aw * 0.5f, r - aw * 0.5f, 0.f, startAngle, valAngle, true);
            g.setColour(juce::Colour(Pal::accentLo).withAlpha(0.4f));
            g.strokePath(arc, juce::PathStrokeType(aw * 2.6f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
            juce::ColourGradient ag(juce::Colour(Pal::accentHi), cx - r, cy,
                                    juce::Colour(0xff1850c0), cx + r, cy, false);
            g.setGradientFill(ag);
            g.strokePath(arc, juce::PathStrokeType(aw, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        float kr = r - aw * 2.0f;
        juce::ColourGradient body(juce::Colour(0xff303040), cx - kr * 0.35f, cy - kr * 0.35f,
                                  juce::Colour(0xff0c0c14), cx + kr * 0.35f, cy + kr * 0.35f, false);
        g.setGradientFill(body);
        g.fillEllipse(cx - kr, cy - kr, kr * 2.f, kr * 2.f);
        g.setColour(juce::Colour(Pal::borderHi));
        g.drawEllipse(cx - kr + 0.5f, cy - kr + 0.5f, kr * 2.f - 1.f, kr * 2.f - 1.f, 0.8f);

        float li = kr * 0.32f, lo = kr * 0.82f;
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.drawLine(cx + std::sin(valAngle) * li, cy - std::cos(valAngle) * li,
                   cx + std::sin(valAngle) * lo, cy - std::cos(valAngle) * lo, 1.8f);
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int,
                      juce::ComboBox& box) override
    {
        auto b = juce::Rectangle<float>(0.f, 0.f, (float)w, (float)h);
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(b, 4.f);
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(b.reduced(0.5f), 4.f, 0.8f);
        float ax = w - 14.f, ay = h * 0.5f - 2.f;
        juce::Path arrow;
        arrow.addTriangle(ax, ay, ax + 8.f, ay, ax + 4.f, ay + 5.f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        g.fillPath(arrow);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    { return juce::Font(juce::FontOptions(10.5f)); }

    juce::Font getLabelFont(juce::Label&) override
    { return juce::Font(juce::FontOptions(10.0f, juce::Font::bold)); }
};

// ── Animated waveform display ─────────────────────────────────────────────────
class WavetableDisplay : public juce::Component, private juce::Timer
{
public:
    WavetableDisplay(juce::AudioProcessorValueTreeState& a) : apvts(a) { startTimerHz(30); }
    void timerCallback() override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        float morph = apvts.getRawParameterValue("MORPH")->load();
        auto  lb    = getLocalBounds();
        g.setColour(juce::Colour(Pal::bgDark));
        g.fillRoundedRectangle(lb.toFloat(), 4.f);
        g.setColour(juce::Colour(Pal::border));
        g.drawRoundedRectangle(lb.toFloat().reduced(0.5f), 4.f, 0.8f);

        auto b = lb.reduced(4).toFloat();
        juce::Path p;
        for (int x = 0; x <= lb.getWidth(); ++x)
        {
            float s = WavetableOscillator::sampleAt((float)x / lb.getWidth(), morph);
            float y = b.getCentreY() - s * b.getHeight() * 0.42f;
            x == 0 ? p.startNewSubPath((float)x, y) : p.lineTo((float)x, y);
        }
        juce::ColourGradient lg(juce::Colour(Pal::accentHi), 0, 0,
                                juce::Colour(0xff4f8fde), (float)lb.getWidth(), 0, false);
        g.setGradientFill(lg);
        g.strokePath(p, juce::PathStrokeType(1.8f));

        const char* wn[] = { "SINE", "TRI", "SAW", "SQ" };
        int idx = juce::jlimit(0, 3, (int)std::round(morph * 3.f));
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(Pal::textLow));
        g.drawText(wn[idx], lb.reduced(4), juce::Justification::bottomRight);
    }
private:
    juce::AudioProcessorValueTreeState& apvts;
};

// ── Knob + label ──────────────────────────────────────────────────────────────
class KnobWithLabel : public juce::Component
{
public:
    juce::Slider slider;
    juce::Label  label;

    KnobWithLabel(const juce::String& name, bool isInt = false)
    {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 12);
        if (isInt) slider.setNumDecimalPlacesToDisplay(0);
        addAndMakeVisible(slider);
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, juce::Colour(Pal::textMid));
        addAndMakeVisible(label);
    }
    void resized() override
    {
        auto a = getLocalBounds();
        label.setBounds(a.removeFromBottom(13));
        slider.setBounds(a);
    }
};

// ── Mode selector ─────────────────────────────────────────────────────────────
class ModeSelector : public juce::Component
{
public:
    int selectedMode = 2;
    std::function<void(int)> onChange;

    void paint(juce::Graphics& g) override
    {
        const char* names[] = { "LEAD", "BASS", "MELODY", "ARP", "PAD" };
        float bw = getWidth() / 5.f;
        for (int i = 0; i < 5; ++i)
        {
            juce::Rectangle<float> b(i * bw + 1.f, 1.f, bw - 2.f, getHeight() - 2.f);
            bool sel = (i == selectedMode);
            if (sel)
            {
                juce::ColourGradient cg(juce::Colour(Pal::accent), b.getCentreX(), b.getY(),
                                        juce::Colour(Pal::accentLo), b.getCentreX(), b.getBottom(), false);
                g.setGradientFill(cg);
                g.fillRoundedRectangle(b, 5.f);
                g.setColour(juce::Colour(Pal::accentHi));
                g.drawRoundedRectangle(b.reduced(0.5f), 5.f, 0.8f);
            }
            else
            {
                g.setColour(juce::Colour(0xff161630));
                g.fillRoundedRectangle(b, 5.f);
                g.setColour(juce::Colour(Pal::border));
                g.drawRoundedRectangle(b.reduced(0.5f), 5.f, 0.6f);
            }
            g.setFont(juce::FontOptions(9.5f, sel ? juce::Font::bold : 0));
            g.setColour(sel ? juce::Colours::white : juce::Colour(Pal::textLow));
            g.drawText(names[i], b, juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int m = juce::jlimit(0, 4, (int)(e.x / (getWidth() / 5.f)));
        if (m != selectedMode) { selectedMode = m; repaint(); if (onChange) onChange(m); }
    }
};

// ── Visual piano keyboard (2 octaves, C3–B4) ─────────────────────────────────
class VisualKeyboard : public juce::Component, private juce::Timer
{
public:
    VisualKeyboard(MorphOneAudioProcessor& proc, juce::AudioProcessorValueTreeState& a)
        : proc(proc), apvts(a) { startTimerHz(20); }
    void timerCallback() override { repaint(); }
    void paint(juce::Graphics& g) override;

private:
    MorphOneAudioProcessor& proc;
    juce::AudioProcessorValueTreeState& apvts;
};

// ── Live chord name panel ─────────────────────────────────────────────────────
class LiveChordPanel : public juce::Component, private juce::Timer
{
public:
    LiveChordPanel(MorphOneAudioProcessor& proc, juce::AudioProcessorValueTreeState& a)
        : proc(proc), apvts(a) { startTimerHz(10); }
    void timerCallback() override { repaint(); }
    void paint(juce::Graphics& g) override;

private:
    MorphOneAudioProcessor& proc;
    juce::AudioProcessorValueTreeState& apvts;
};

// ── Clickable chord grid — shows all diatonic chords, click to preview ───────
class NextChordPanel : public juce::Component, private juce::Timer
{
public:
    NextChordPanel(MorphOneAudioProcessor& proc, juce::AudioProcessorValueTreeState& a)
        : proc(proc), apvts(a) { startTimerHz(4); }
    void timerCallback() override { repaint(); }
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    MorphOneAudioProcessor& proc;
    juce::AudioProcessorValueTreeState& apvts;

    struct ChordBtn {
        juce::Rectangle<float> bounds;
        ChordDetector::DiatonicChord chord;
        bool isSuggested = false;
        std::vector<int> midiNotes;
    };
    std::vector<ChordBtn> buttons;

    void rebuildButtons();
    void triggerChord(const std::vector<int>& notes);
};

// ── Editor ────────────────────────────────────────────────────────────────────
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
    MorphOneLAF             morphLAF;

    void timerCallback() override;
    void paintSection(juce::Graphics&, juce::Rectangle<int>, const juce::String&, bool highlight = false);
    void styleCombo(juce::ComboBox&);
    int  getIntParam(const juce::String&);
    void setIntParam(const juce::String&, int);
    void populateTheoryCombos();
    void syncCombosFromApvts();

    // Components
    juce::ComboBox       presetBox;
    ModeSelector         modeSelector;
    WavetableDisplay     waveDisplay;
    LiveChordPanel       liveChordPanel;
    VisualKeyboard       visualKeyboard;
    NextChordPanel       nextChordPanel;

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

    juce::ComboBox keyBox, scaleBox, chordTypeBox, chordInvBox;
    juce::ComboBox arpDirBox, arpSpeedBox;
    juce::ComboBox progBox, progChordBox;

    juce::ToggleButton scaleLockBtn { "Lock" };
    juce::ToggleButton arpOnBtn     { "On"   };

    std::unique_ptr<BtnAttach> scaleLockAtt, arpOnAtt;

    juce::Image shotMusicLogo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphOneAudioProcessorEditor)
};
