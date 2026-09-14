#include "ModSection.h"
#include "../ParamIDs.h"

namespace maps_ui {

// ================================================================ Attenuator

Attenuator::Attenuator (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, this);
    slider.setDoubleClickReturnValue (true, 0.0);
    slider.setWantsKeyboardFocus (false);
    slider.setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::trackColourId,      juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::thumbColourId,      juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);
}

void Attenuator::resized() { slider.setBounds (getLocalBounds()); }

void Attenuator::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (0.0f, 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.26f));
    g.fillRoundedRectangle (b, 2.0f);

    const float mid = b.getCentreX();
    g.setColour (colours::scribe.withAlpha (0.8f));
    g.fillRect (mid - 0.5f, b.getY() - 1.0f, 1.0f, b.getHeight() + 2.0f);

    const double range = slider.getMaximum() - slider.getMinimum();
    const float v = range > 0.0 ? (float) (slider.getValue() / (range * 0.5)) : 0.0f;
    const float w = std::abs (v) * (b.getWidth() * 0.5f);

    if (w > 0.5f)
    {
        const auto fill = v >= 0.0f ? juce::Rectangle<float> (mid, b.getY(), w, b.getHeight())
                                    : juce::Rectangle<float> (mid - w, b.getY(), w, b.getHeight());
        g.setColour (v >= 0.0f ? colours::ledAmber.withAlpha (0.85f)
                               : colours::knobHigh.withAlpha (0.95f));
        g.fillRoundedRectangle (fill, 2.0f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (b, 2.0f, 1.0f);
}

// ================================================================== LfoStrip

LfoStrip::LfoStrip (juce::AudioProcessorValueTreeState& apvts, int n)
    : state (apvts),
      number (n),
      rate (apvts, ids::lfoRate (n), "RATE", Knob::medium),
      wave (apvts, ids::lfoWave (n), "", colours::ledGreen),
      sync (apvts, ids::lfoSync (n), "SYNC", colours::ledAmber)
{
    addAndMakeVisible (rate);
    addAndMakeVisible (wave);
    addAndMakeVisible (sync);

    for (int r = 0; r < modulation::kRoutesPerLfo; ++r)
    {
        for (int d = 0; d < modulation::numDests; ++d)
            dest[r].addItem (juce::String (modulation::destName (d)), d + 1);

        dest[r].setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a2825));
        dest[r].setColour (juce::ComboBox::textColourId,       juce::Colour (0xffe8e3d8));
        dest[r].setColour (juce::ComboBox::outlineColourId,    juce::Colour (0x88000000));
        dest[r].setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffbdb7a9));
        dest[r].setWantsKeyboardFocus (false);
        addAndMakeVisible (dest[r]);

        destAttach[r] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, ids::lfoDest (n, r), dest[r]);

        depth[r] = std::make_unique<Attenuator> (apvts, ids::lfoDepth (n, r));
        addAndMakeVisible (*depth[r]);
    }

    refresh();
}

void LfoStrip::resized()
{
    const int w = getWidth();

    rate.setBounds (4, 14, (int) (Knob::diameterFor (Knob::medium) + 34.0f),
                    (int) Knob::heightFor (Knob::medium, true));

    const int rx = (int) (Knob::diameterFor (Knob::medium) + 44.0f);
    wave.setBounds (rx, 20, w - rx - 6, 26);
    sync.setBounds (rx, 52, w - rx - 6, 26);

    int y = 118;
    for (int r = 0; r < modulation::kRoutesPerLfo; ++r)
    {
        dest[r].setBounds (20, y, w - 26, 22);
        depth[r]->setBounds (20, y + 23, w - 26, 11);
        y += 37;
    }
}

void LfoStrip::paint (juce::Graphics& g)
{
    drawEngraved (g, "SOURCE " + juce::String (number),
                  { 4.0f, 0.0f, 120.0f, 13.0f }, juce::Justification::left, 9.0f, 2.2f);

    drawEngraved (g, "ROUTING",
                  { 20.0f, 102.0f, 120.0f, 12.0f }, juce::Justification::left, 8.0f, 1.8f);

    // A and B beside their routes, so the two attenuators are nameable when
    // you are talking about a patch rather than looking at it.
    for (int r = 0; r < modulation::kRoutesPerLfo; ++r)
        drawEngraved (g, r == 0 ? "A" : "B",
                      { 2.0f, 118.0f + 37.0f * r, 16.0f, 22.0f },
                      juce::Justification::centred, 10.0f, 0.0f);
}

void LfoStrip::refresh()
{
    wave.refresh();

    // The RATE knob means two different things and the caption is the only
    // place that says which. Without it the control is a guess.
    const float knob = state.getRawParameterValue (ids::lfoRate (number))->load();
    const bool synced = state.getRawParameterValue (ids::lfoSync (number))->load() > 0.5f;

    rate.setCaption (synced
        ? juce::String (maps::lfoDivisions()[maps::lfoDivisionForKnob (knob)].name)
        : juce::String (maps::lfoRateForKnob (knob), 2) + " HZ");
}

// ================================================================ ModSection

ModSection::ModSection (juce::AudioProcessorValueTreeState& apvts)
{
    bar.setButtonText ({});
    bar.setWantsKeyboardFocus (false);
    bar.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    bar.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    bar.onClick = [this] { setOpen (! open); };
    addAndMakeVisible (bar);

    for (int i = 0; i < modulation::kNumLfos; ++i)
    {
        strips[i] = std::make_unique<LfoStrip> (apvts, i + 1);
        addChildComponent (*strips[i]);
    }
}

void ModSection::setOpen (bool shouldBeOpen)
{
    if (open == shouldBeOpen)
        return;
    open = shouldBeOpen;
    for (auto& s : strips)
        s->setVisible (open);
    if (onToggle)
        onToggle();
    resized();
    repaint();
}

void ModSection::resized()
{
    bar.setBounds (0, 0, getWidth(), kBarHeight);
    if (! open)
        return;

    const int inset = 20;
    const int top   = kBarHeight + 12;
    const int w     = (getWidth() - inset * 2) / modulation::kNumLfos;
    for (int i = 0; i < modulation::kNumLfos; ++i)
        strips[i]->setBounds (inset + w * i, top, w - 10, kContentHeight - 20);
}

void ModSection::paint (juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();

    // When open, a plain scribed box below the bar. The bar is the section
    // header, so the box does not repeat the title.
    if (open)
        drawScribedBox (g, full.withTrimmedTop ((float) kBarHeight + 2.0f)
                                .withTrimmedBottom (4.0f).reduced (14.0f, 0.0f),
                        {}, {});

    // The bar itself: a scribed strip carrying the section name and its model
    // number, so it reads as part of the panel rather than a widget on it.
    const auto barArea = juce::Rectangle<float> (0.0f, 0.0f, full.getWidth(), (float) kBarHeight)
                           .reduced (14.0f, 5.0f);

    g.setColour (juce::Colours::black.withAlpha (bar.isDown() ? 0.10f : 0.18f));
    g.fillRoundedRectangle (barArea, 3.0f);
    g.setColour (colours::scribe.withAlpha (0.8f));
    g.drawRoundedRectangle (barArea, 3.0f, 1.0f);

    drawEngraved (g, "MODULATION SOURCES", barArea, juce::Justification::centred, 9.5f, 2.6f);
    drawEngraved (g, "266", barArea.withTrimmedRight (30.0f), juce::Justification::right, 9.5f, 2.4f);

    // A small triangle at each end pointing the way the section will move.
    auto arrow = [&g, &barArea, this] (float cx)
    {
        juce::Path p;
        const float cy = barArea.getCentreY(), s = 4.0f;
        if (open) { p.addTriangle (cx - s, cy + s * 0.6f, cx + s, cy + s * 0.6f, cx, cy - s * 0.8f); }
        else      { p.addTriangle (cx - s, cy - s * 0.6f, cx + s, cy - s * 0.6f, cx, cy + s * 0.8f); }
        g.setColour (colours::engraved.withAlpha (0.7f));
        g.fillPath (p);
    };
    arrow (barArea.getX() + 16.0f);
    arrow (barArea.getRight() - 16.0f);
}

void ModSection::refresh()
{
    if (! open)
        return;
    for (auto& s : strips)
        s->refresh();
}

} // namespace maps_ui
