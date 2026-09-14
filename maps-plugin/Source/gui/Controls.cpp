#include "Controls.h"

namespace maps_ui {

// ======================================================================= Knob

namespace {
constexpr float kLegendH  = 13.0f;
constexpr float kCaptionH = 12.0f;
}

float Knob::diameterFor (Size s) noexcept
{
    switch (s)
    {
        case large:  return kKnobLarge;
        case medium: return kKnobMedium;
        case trim:   break;
    }
    return kKnobSmall;
}

float Knob::heightFor (Size s, bool withCaption) noexcept
{
    return diameterFor (s) + 10.0f + kLegendH + (withCaption ? kCaptionH : 0.0f);
}

Knob::Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
            juce::String legend, Size size)
    : legendText (std::move (legend)), knobSize (size)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                                juce::MathConstants<float>::pi * 2.80f, true);
    // The panel is the display. A value only appears while a knob is being
    // moved, which is the plugin's substitute for a detent under a finger.
    slider.setPopupDisplayEnabled (true, true, this);
    slider.setDoubleClickReturnValue (true, 0.0);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);
    slider.setDoubleClickReturnValue (true, slider.getValue());
}

void Knob::setLegendToTheRight (bool shouldBeRight)
{
    if (legendRight == shouldBeRight) return;
    legendRight = shouldBeRight;
    resized();
    repaint();
}

float Knob::widthWithLegendRight (Size s) noexcept { return diameterFor (s) + 96.0f; }

void Knob::setModulation (float normalisedOffset)
{
    if (std::abs (modOffset - normalisedOffset) < 0.002f)
        return;
    modOffset = normalisedOffset;
    repaint();
}

void Knob::setCaption (juce::String text)
{
    if (captionText == text)
        return;
    captionText = std::move (text);
    repaint();
}

void Knob::resized()
{
    const float d = diameterFor (knobSize);
    const float cx = legendRight ? d * 0.5f + 2.0f : getWidth() * 0.5f;
    slider.setBounds (juce::Rectangle<float> (d, d)
                        .withCentre ({ cx, d * 0.5f + 2.0f }).toNearestInt());
}

void Knob::paint (juce::Graphics& g)
{
    const float d = diameterFor (knobSize);
    const float cx = legendRight ? d * 0.5f + 2.0f : getWidth() * 0.5f;
    const auto knobArea = juce::Rectangle<float> (d, d).withCentre ({ cx, d * 0.5f + 2.0f });

    const auto& s = slider;
    const double range = s.getMaximum() - s.getMinimum();
    const float v = range > 0.0 ? (float) ((s.getValue() - s.getMinimum()) / range) : 0.0f;

    drawKnob (g, knobArea, v, knobSize == trim, knobSize != trim);
    if (std::abs (modOffset) > 0.002f)
        drawModulationArc (g, knobArea, v, juce::jlimit (0.0f, 1.0f, v + modOffset));

    if (legendRight)
    {
        drawEngraved (g, legendText,
                      { knobArea.getRight() + 10.0f, knobArea.getCentreY() - 7.0f,
                        getWidth() - knobArea.getRight() - 12.0f, 14.0f },
                      juce::Justification::left, 9.5f, 1.7f);
        return;
    }

    const float legendY = knobArea.getBottom() + (knobSize == trim ? 4.0f : 7.0f);
    drawEngraved (g, legendText, { 0.0f, legendY, (float) getWidth(), kLegendH },
                  juce::Justification::centred, knobSize == trim ? 8.5f : 9.5f,
                  knobSize == trim ? 1.1f : 1.7f);

    if (captionText.isNotEmpty())
        drawEngraved (g, captionText, { 0.0f, legendY + kLegendH, (float) getWidth(), kCaptionH },
                      juce::Justification::centred, 8.5f, 0.9f);
}

// ===================================================================== Switch

Switch::Switch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                juce::String text, juce::Colour lamp, bool momentary)
    : label (std::move (text)), lampColour (lamp)
{
    button.setClickingTogglesState (! momentary);
    button.setWantsKeyboardFocus (false);
    button.setOpaque (false);
    // The look is painted by this component; the button is only the hit area.
    button.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (button);

    if (momentary)
    {
        // A momentary switch has to write the parameter itself: RESET and FILL
        // are held, not toggled, and the host needs to see both edges.
        auto* param = apvts.getParameter (paramID);
        button.onStateChange = [this, param]
        {
            if (param == nullptr) return;
            const bool down = button.isDown();
            if (std::abs (param->getValue() - (down ? 1.0f : 0.0f)) < 0.001f)
                return;
            param->beginChangeGesture();
            param->setValueNotifyingHost (down ? 1.0f : 0.0f);
            param->endChangeGesture();
            repaint();
        };
    }
    else
    {
        buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvts, paramID, button);
        button.onClick = [this] { repaint(); };
    }
}

void Switch::resized() { button.setBounds (getLocalBounds()); }

void Switch::paint (juce::Graphics& g)
{
    const bool on = button.getClickingTogglesState() ? button.getToggleState() : button.isDown();
    drawSwitch (g, getLocalBounds().toFloat(), label, on, button.isDown(), lampColour);
}

// =============================================================== ChoiceSwitch

ChoiceSwitch::ChoiceSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                            juce::String prefix, juce::Colour lamp)
    : label (std::move (prefix)), lampColour (lamp)
{
    param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID));
    button.setClickingTogglesState (false);
    button.setWantsKeyboardFocus (false);
    button.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (button);

    button.onClick = [this]
    {
        if (param == nullptr) return;
        const int n = param->choices.size();
        if (n <= 0) return;
        const int next = (param->getIndex() + 1) % n;
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) next));
        param->endChangeGesture();
        refresh();
    };

    refresh();
}

void ChoiceSwitch::resized() { button.setBounds (getLocalBounds()); }

void ChoiceSwitch::refresh()
{
    const auto now = param != nullptr ? param->getCurrentChoiceName() : juce::String();
    if (now != shown) { shown = now; repaint(); }
}

void ChoiceSwitch::paint (juce::Graphics& g)
{
    drawSwitch (g, getLocalBounds().toFloat(), label + "  " + shown, true, button.isDown(), lampColour);
}

// ================================================================ StepDisplay

void StepDisplay::push (int step, unsigned triggers)
{
    step = juce::jlimit (0, kSteps - 1, step);
    if (step != current)
    {
        current = step;
        lane[step] = triggers;
        repaint();
    }
    else if (triggers != lane[step])
    {
        lane[step] = triggers;
        repaint();
    }
}

void StepDisplay::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float cellW = b.getWidth() / (float) kSteps;

    for (int i = 0; i < kSteps; ++i)
    {
        const auto cell = juce::Rectangle<float> (b.getX() + i * cellW, b.getY(),
                                                  cellW, b.getHeight()).reduced (1.0f, 0.0f);

        // Bar lines every eight steps, so two bars are legible at a glance.
        g.setColour (juce::Colours::black.withAlpha (i % 8 == 0 ? 0.30f : 0.14f));
        g.fillRoundedRectangle (cell, 1.5f);

        const unsigned t = lane[i];
        if (t != 0)
        {
            const juce::Colour c = (t & 1u) ? colours::ledAmber
                                 : ((t & 2u) ? colours::ledRed : colours::ledGreen);
            g.setColour (c.withAlpha (0.85f));
            g.fillRoundedRectangle (cell.reduced (0.0f, 1.0f), 1.5f);
        }

        if (i == current)
        {
            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.drawRoundedRectangle (cell.reduced (0.0f, 0.5f), 1.5f, 1.2f);
        }
    }
}

// =================================================================== LevelBar

void LevelBar::push (float peak)
{
    level = juce::jmax (peak, level * 0.72f);
    hold  = juce::jmax (peak, hold * 0.96f);
    repaint();
}

void LevelBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (b, 1.5f);

    auto toX = [&b] (float v)
    {
        // -48 dB to 0, so a quiet voice still shows something.
        const float dB = juce::Decibels::gainToDecibels (juce::jmax (1.0e-5f, v));
        return juce::jlimit (0.0f, 1.0f, (dB + 48.0f) / 48.0f) * b.getWidth();
    };

    const float w = toX (level);
    if (w > 1.0f)
    {
        g.setColour (level > 0.95f ? colours::ledRed : colours::ledAmber);
        g.fillRoundedRectangle (b.withWidth (w), 1.5f);
    }
    const float h = toX (hold);
    if (h > 2.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.fillRect (b.getX() + h - 1.5f, b.getY(), 1.5f, b.getHeight());
    }
}

} // namespace maps_ui
