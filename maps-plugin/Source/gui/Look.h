// The Buchla vocabulary, in one file.
//
// Four things do the work, and none of them is the colour: graduated knob
// diameters, engraved-and-paint-filled legends, scribed section boxes whose
// title breaks the top rule, and negative space left alone. Anything added to
// this panel later should be checked against those four before it goes in.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace maps_ui {

namespace colours {

// Natural anodised aluminium, lit from the top left.
const juce::Colour panelTop     { 0xffd6d3cb };
const juce::Colour panelBottom  { 0xffb7b4ac };
const juce::Colour brush        { 0x08000000 };

// Engraving: a dark groove with the paint in it, and a bright edge below
// where the cutter threw up a lip.
const juce::Colour engraved     { 0xff1c1b18 };
const juce::Colour engravedLip  { 0x66ffffff };
const juce::Colour scribe       { 0xff6f6c64 };
const juce::Colour scribeLip    { 0x59ffffff };

// Rogan PT blue.
const juce::Colour knobHigh     { 0xff4a7fc4 };
const juce::Colour knobLow      { 0xff1f4272 };
const juce::Colour knobEdge     { 0xff11294a };
const juce::Colour pointer      { 0xfff2ede1 };
const juce::Colour indicator    { 0xff8a5a2b };

// Small trims are a darker grey-blue, so the hierarchy reads at a glance.
const juce::Colour trimHigh     { 0xff5b6470 };
const juce::Colour trimLow      { 0xff2c333c };

const juce::Colour ledOff       { 0xff4b4740 };
const juce::Colour ledAmber     { 0xffffa02c };
const juce::Colour ledRed       { 0xffe05038 };
const juce::Colour ledGreen     { 0xff7fd04a };

} // namespace colours

// Knob diameters, in design pixels. 26 / 20 / 12 mm on the real panel.
constexpr float kKnobLarge  = 68.0f;
constexpr float kKnobMedium = 52.0f;
constexpr float kKnobSmall  = 31.0f;

/** Brushed metal with a top-lit gradient. */
void paintPanel (juce::Graphics& g, juce::Rectangle<float> area);

/** Engraved legend: letterspaced caps, dark, with a light lip below.
    Buchla legends are cut into the metal, and the lip is what makes them read
    as cut rather than printed. */
void drawEngraved (juce::Graphics& g, const juce::String& text,
                   juce::Rectangle<float> area, juce::Justification just,
                   float height, float tracking = 1.6f);

/** A scribed section box. The title sits on the top rule with the rule broken
    around it, and the model number does the same at the other end. */
void drawScribedBox (juce::Graphics& g, juce::Rectangle<float> box,
                     const juce::String& title, const juce::String& modelNumber);

/** The knob itself. `value` is 0..1 and only drives the pointer. */
void drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float value,
               bool isTrim, bool hasTicks);

/** The ghost arc: from where the knob is to where modulation has taken it. */
void drawModulationArc (juce::Graphics& g, juce::Rectangle<float> bounds,
                        float from, float to);

/** A panel-mounted push switch with a legend and an indicator. */
void drawSwitch (juce::Graphics& g, juce::Rectangle<float> bounds,
                 const juce::String& text, bool on, bool momentaryDown,
                 juce::Colour lamp);

} // namespace maps_ui
