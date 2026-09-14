#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Look.h"

namespace maps_ui {

/** A knob with its legend cut into the panel below it.
    Three sizes only, and the size is the hierarchy: 26 mm for a voice
    primary, 20 mm for a secondary, 12 mm for a trim. */
class Knob : public juce::Component
{
public:
    enum Size { large, medium, trim };

    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
          juce::String legend, Size size);

    /** Extra line printed under the legend — the model name, the mode. */
    void setCaption (juce::String text);

    /** Puts the legend beside the knob rather than under it. Topograph sets
        its three density knobs on a descending diagonal with the legends to
        the right, and that gesture is worth keeping. */
    void setLegendToTheRight (bool shouldBeRight);
    static float widthWithLegendRight (Size s) noexcept;

    /** Normalised modulation offset, drawn as a ghost arc from the pointer to
        wherever the modulation is currently pushing the value. Without it a
        beat-synced LFO on MODEL is invisible until you hear it. */
    void setModulation (float normalisedOffset);

    void paint (juce::Graphics&) override;
    void resized() override;

    static float diameterFor (Size s) noexcept;
    /** Height a Knob of this size needs, including legend and caption. */
    static float heightFor (Size s, bool withCaption) noexcept;

    juce::Slider slider;

private:
    juce::String legendText, captionText;
    Size knobSize;
    bool legendRight = false;
    float modOffset = 0.0f;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};

/** A panel switch. Latching by default; RESET and FILL are momentary. */
class Switch : public juce::Component
{
public:
    Switch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
            juce::String text, juce::Colour lamp, bool momentary = false);

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::TextButton button;

private:
    juce::String label;
    juce::Colour lampColour;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Switch)
};

/** A switch that steps a choice parameter and prints where it got to.
    Buchla would have used a rotary selector; a button that advances is the
    honest screen equivalent and it costs one control instead of five. */
class ChoiceSwitch : public juce::Component
{
public:
    ChoiceSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                  juce::String prefix, juce::Colour lamp);

    void paint (juce::Graphics&) override;
    void resized() override;
    /** Called from the editor timer so automation is reflected. */
    void refresh();

    juce::TextButton button;

private:
    juce::AudioParameterChoice* param = nullptr;
    juce::String label, shown;
    juce::Colour lampColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceSwitch)
};

/** The three Grids lamps, widened into a two-bar step readout.
    The hardware has one LED per part and that is all it can afford. On a
    screen there is room to show where in the pattern you are, and not showing
    it would be withholding for the sake of purity. */
class StepDisplay : public juce::Component
{
public:
    StepDisplay() = default;

    /** Called from the editor's timer. */
    void push (int step, unsigned triggers);
    void paint (juce::Graphics&) override;

private:
    static constexpr int kSteps = 32;
    unsigned lane[kSteps] = { 0 };
    int current = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepDisplay)
};

/** A thin horizontal level bar. Peak-hold, falling. */
class LevelBar : public juce::Component
{
public:
    LevelBar() = default;

    void push (float peak);
    void paint (juce::Graphics&) override;

private:
    float level = 0.0f, hold = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelBar)
};

} // namespace maps_ui
