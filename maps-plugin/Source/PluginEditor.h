#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "gui/Sections.h"
#include "gui/ModSection.h"

/** The panel, drawn on a fixed 960 x 648 canvas and scaled to whatever window
    the user drags out. 960 x 648 is Rev B's 370 x 250 mm at 2.6 px/mm, so
    every knob on screen is the size it will be under a finger. Laying out in
    millimetres and scaling once is also the only way the plugin stays honest
    as a Phase 0 tool: if a control is awkward here it will be awkward there. */
class PanelCanvas : public juce::Component
{
public:
    PanelCanvas() { setInterceptsMouseClicks (false, true); }
    void paint (juce::Graphics&) override;
};

class MapsEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit MapsEditor (MapsProcessor&);
    ~MapsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutPanel();
    void applyScale (float newScale);
    void applyGeometry();

    /** The canvas grows downwards when the modulation section is open. The
        width never changes, so the scale the user has chosen survives it. */
    int designHeight() const noexcept
    {
        return kPanelHeight + maps_ui::ModSection::heightFor (mods.isOpen()) + 6;
    }

    static constexpr int kDesignWidth = 960;
    static constexpr int kPanelHeight = 648;

    MapsProcessor& proc;

    PanelCanvas canvas;
    std::unique_ptr<maps_ui::VoiceStrip> voices[maps::kNumVoices];
    maps_ui::Section voicesBox;
    maps_ui::GridsSection grids;
    maps_ui::CloudsSection clouds;
    maps_ui::Knob output;
    maps_ui::LevelBar outputMeter;
    maps_ui::ModSection mods;

    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };

    float scale = 0.85f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MapsEditor)
};
