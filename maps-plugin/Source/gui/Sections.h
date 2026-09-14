#pragma once

#include <functional>

#include "Controls.h"
#include "../Modulation.h"
#include "maps/Parameters.h"

namespace maps_ui {

/** A scribed box with a title and a 200-series model number.
    The children are laid out by whoever owns the section, in its own
    coordinates; this only draws the frame. */
class Section : public juce::Component
{
public:
    Section (juce::String title, juce::String modelNumber);
    void paint (juce::Graphics&) override;

    /** The area inside the frame, in this component's coordinates. */
    juce::Rectangle<float> contentArea() const noexcept;
    /** The frame itself: the component minus the room the title needs. */
    juce::Rectangle<float> frameArea() const noexcept;

private:
    juce::String sectionTitle, model;
};

/** One of the three voice strips: seven controls and a mute, in three rows
    of decreasing knob size. FREQUENCY and HARMONICS get the 26 mm caps
    because on a percussion engine they are the two you reach for. */
class VoiceStrip : public juce::Component
{
public:
    VoiceStrip (juce::AudioProcessorValueTreeState& apvts, int voiceNumber);

    void paint (juce::Graphics&) override;
    void resized() override;
    /** `modOf` answers with the normalised modulation offset of a
        destination, so each knob can draw its ghost arc. */
    void refresh (float peak, const std::function<float (int)>& modOf);

private:
    int number;
    juce::AudioProcessorValueTreeState& state;

    Knob frequency, harmonics, timbre, morph, model, decay, level, send;
    Switch mute;
    LevelBar meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceStrip)
};

class GridsSection : public Section
{
public:
    explicit GridsSection (juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
    void refresh (int step, unsigned triggers, const std::function<float (int)>& modOf);

private:
    Knob mapX, mapY, chaos, clock, swing;
    std::unique_ptr<Knob> density[maps::kNumVoices];
    Switch run, reset, fill;
    StepDisplay steps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridsSection)
};

class CloudsSection : public Section
{
public:
    explicit CloudsSection (juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
    void refresh (const std::function<float (int)>& modOf);

private:
    juce::AudioProcessorValueTreeState& state;
    Knob position, size, density, texture, pitch, feedback, space, blend, spread;
    ChoiceSwitch mode;
    Switch freeze;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CloudsSection)
};

} // namespace maps_ui
