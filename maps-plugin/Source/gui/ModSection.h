#pragma once

#include "Controls.h"
#include "../Modulation.h"

namespace maps_ui {

/** A bipolar attenuator: a horizontal bar that fills left or right from the
    centre. Modular convention, and the shape says "this can go negative"
    without a legend having to. */
class Attenuator : public juce::Component
{
public:
    Attenuator (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Attenuator)
};

/** One modulation source: a rate knob whose caption prints either a division
    or a frequency, a waveform selector, a sync switch, and two routes. */
class LfoStrip : public juce::Component
{
public:
    LfoStrip (juce::AudioProcessorValueTreeState& apvts, int number);

    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    juce::AudioProcessorValueTreeState& state;
    int number;

    Knob rate;
    ChoiceSwitch wave;
    Switch sync;
    juce::ComboBox dest[modulation::kRoutesPerLfo];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destAttach[modulation::kRoutesPerLfo];
    std::unique_ptr<Attenuator> depth[modulation::kRoutesPerLfo];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoStrip)
};

/** The whole section, and the bar that opens and closes it.
    MAPS' hardware panel has no modulation section and Rev B has no room for
    one; on screen there is room, and it folds away when it is not wanted so
    the instrument still fits on a laptop. */
class ModSection : public juce::Component
{
public:
    explicit ModSection (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

    bool isOpen() const noexcept { return open; }
    void setOpen (bool shouldBeOpen);

    /** Called when the user clicks the bar, so the editor can resize. */
    std::function<void()> onToggle;

    static constexpr int kBarHeight     = 30;
    static constexpr int kContentHeight = 210;
    static int heightFor (bool isOpen) noexcept
    {
        return kBarHeight + (isOpen ? kContentHeight : 0);
    }

private:
    bool open = false;
    juce::TextButton bar;
    std::unique_ptr<LfoStrip> strips[modulation::kNumLfos];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModSection)
};

} // namespace maps_ui
