#include "Look.h"

namespace maps_ui {

namespace {

juce::Font panelFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions()
                         .withHeight (height)
                         .withStyle (bold ? "Bold" : "Plain"));
}

/** JUCE has no letter tracking, and letterspaced caps are most of what makes
    a legend look engraved rather than typed, so the glyphs go down one at a
    time with the extra space added by hand. */
float trackedWidth (const juce::Font& f, const juce::String& text, float tracking)
{
    float w = 0.0f;
    for (int i = 0; i < text.length(); ++i)
        w += juce::GlyphArrangement::getStringWidth (f, text.substring (i, i + 1)) + tracking;
    return w > 0.0f ? w - tracking : 0.0f;
}

void drawTracked (juce::Graphics& g, const juce::Font& f, const juce::String& text,
                  float x, float baseline, float tracking)
{
    for (int i = 0; i < text.length(); ++i)
    {
        const auto ch = text.substring (i, i + 1);
        g.drawSingleLineText (ch, juce::roundToInt (x), juce::roundToInt (baseline));
        x += juce::GlyphArrangement::getStringWidth (f, ch) + tracking;
    }
}

} // namespace

void paintPanel (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setGradientFill (juce::ColourGradient (colours::panelTop, area.getX(), area.getY(),
                                             colours::panelBottom, area.getX(), area.getBottom(), false));
    g.fillRect (area);

    // Brushed grain. Fine horizontal strokes, uneven, low contrast.
    juce::Random r (0x4d415053);
    g.setColour (colours::brush);
    for (float y = area.getY(); y < area.getBottom(); y += 2.0f)
    {
        const float a = 0.25f + 0.75f * r.nextFloat();
        g.setOpacity (0.05f * a);
        g.fillRect (area.getX(), y, area.getWidth(), 1.0f);
    }
    g.setOpacity (1.0f);

    // A soft top-left light, the way a milled panel catches a room.
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.10f),
                                area.getX(), area.getY(),
                                juce::Colours::transparentWhite,
                                area.getCentreX(), area.getBottom(), true);
    g.setGradientFill (sheen);
    g.fillRect (area);
}

void drawEngraved (juce::Graphics& g, const juce::String& text,
                   juce::Rectangle<float> area, juce::Justification just,
                   float height, float tracking)
{
    if (text.isEmpty())
        return;

    const auto f = panelFont (height);
    g.setFont (f);

    const auto upper = text.toUpperCase();
    const float w = trackedWidth (f, upper, tracking);

    float x = area.getX();
    if (just.testFlags (juce::Justification::horizontallyCentred)) x = area.getCentreX() - w * 0.5f;
    else if (just.testFlags (juce::Justification::right))          x = area.getRight() - w;

    const float baseline = area.getCentreY() + height * 0.34f;

    g.setColour (colours::engravedLip);
    drawTracked (g, f, upper, x, baseline + 1.0f, tracking);
    g.setColour (colours::engraved);
    drawTracked (g, f, upper, x, baseline, tracking);
}

void drawScribedBox (juce::Graphics& g, juce::Rectangle<float> box,
                     const juce::String& title, const juce::String& modelNumber)
{
    const float titleH = 11.0f;
    const auto f = panelFont (titleH);
    const float tracking = 2.4f;

    const float titleW = trackedWidth (f, title.toUpperCase(), tracking);
    const float modelW = modelNumber.isNotEmpty()
                           ? trackedWidth (f, modelNumber, tracking) : 0.0f;

    const float titleX = box.getX() + 16.0f;
    const float modelX = box.getRight() - 16.0f - modelW;
    const float top    = box.getY();

    auto rule = [&g] (float x1, float x2, float y)
    {
        if (x2 <= x1) return;
        g.setColour (colours::scribeLip);
        g.fillRect (x1, y + 1.0f, x2 - x1, 1.0f);
        g.setColour (colours::scribe);
        g.fillRect (x1, y, x2 - x1, 1.0f);
    };

    // Top rule, broken for the title and again for the model number.
    rule (box.getX(), titleX - 7.0f, top);
    rule (titleX + titleW + 7.0f, modelNumber.isNotEmpty() ? modelX - 7.0f : box.getRight(), top);
    if (modelNumber.isNotEmpty())
        rule (modelX + modelW + 7.0f, box.getRight(), top);

    // The other three sides.
    g.setColour (colours::scribe);
    g.fillRect (box.getX(), top, 1.0f, box.getHeight());
    g.fillRect (box.getRight() - 1.0f, top, 1.0f, box.getHeight());
    g.fillRect (box.getX(), box.getBottom() - 1.0f, box.getWidth(), 1.0f);
    g.setColour (colours::scribeLip);
    g.fillRect (box.getX() + 1.0f, top, 1.0f, box.getHeight());
    g.fillRect (box.getX(), box.getBottom(), box.getWidth(), 1.0f);

    drawEngraved (g, title, { titleX, top - titleH * 0.5f - 1.0f, titleW + 4.0f, titleH + 2.0f },
                  juce::Justification::left, titleH, tracking);
    if (modelNumber.isNotEmpty())
        drawEngraved (g, modelNumber, { modelX, top - titleH * 0.5f - 1.0f, modelW + 4.0f, titleH + 2.0f },
                      juce::Justification::left, titleH, tracking);
}

void drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float value,
               bool isTrim, bool hasTicks)
{
    const auto centre = bounds.getCentre();
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const float r = d * 0.5f;

    const float startAngle = juce::MathConstants<float>::pi * 1.20f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.80f;
    const float angle      = startAngle + juce::jlimit (0.0f, 1.0f, value) * (endAngle - startAngle);

    // Skirt ticks, on the two larger sizes only. Trims get none: on the real
    // panel a 12 mm knob has no room for them.
    if (hasTicks)
    {
        g.setColour (colours::scribe.withAlpha (0.75f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const float inner = r + 3.0f, outer = r + (i % 5 == 0 ? 7.0f : 5.0f);
            const juce::Point<float> p1 { centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner };
            const juce::Point<float> p2 { centre.x + std::sin (a) * outer, centre.y - std::cos (a) * outer };
            g.drawLine ({ p1, p2 }, i % 5 == 0 ? 1.4f : 0.9f);
        }
    }

    // A discreet value arc. Not a Buchla thing, but a plugin knob with no
    // detent under your fingers needs somewhere to read the value from.
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, r + 4.5f, r + 4.5f, 0.0f, startAngle, angle, true);
        g.setColour (colours::indicator.withAlpha (0.55f));
        g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    // Shadow.
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (centre.x - r + 1.0f, centre.y - r + 2.5f, d, d);

    // Body.
    const auto high = isTrim ? colours::trimHigh : colours::knobHigh;
    const auto low  = isTrim ? colours::trimLow  : colours::knobLow;
    juce::ColourGradient body (high, centre.x - r * 0.45f, centre.y - r * 0.7f,
                               low,  centre.x + r * 0.5f,  centre.y + r * 0.8f, false);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - r, centre.y - r, d, d);

    // Turned edge.
    g.setColour (colours::knobEdge.withAlpha (0.9f));
    g.drawEllipse (centre.x - r + 0.5f, centre.y - r + 0.5f, d - 1.0f, d - 1.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.drawEllipse (centre.x - r + 2.0f, centre.y - r + 2.0f, d - 4.0f, d - 4.0f, 1.0f);

    // Pointer, cut across the cap.
    const float inner = isTrim ? r * 0.20f : r * 0.28f;
    const float outer = r * 0.86f;
    const juce::Point<float> a { centre.x + std::sin (angle) * inner, centre.y - std::cos (angle) * inner };
    const juce::Point<float> b { centre.x + std::sin (angle) * outer, centre.y - std::cos (angle) * outer };
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine ({ a.translated (0.0f, 1.2f), b.translated (0.0f, 1.2f) }, isTrim ? 2.0f : 2.8f);
    g.setColour (colours::pointer);
    g.drawLine ({ a, b }, isTrim ? 2.0f : 2.8f);
}

void drawModulationArc (juce::Graphics& g, juce::Rectangle<float> bounds,
                        float from, float to)
{
    const auto centre = bounds.getCentre();
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f + 9.0f;

    const float startAngle = juce::MathConstants<float>::pi * 1.20f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.80f;
    const float a1 = startAngle + juce::jlimit (0.0f, 1.0f, from) * (endAngle - startAngle);
    const float a2 = startAngle + juce::jlimit (0.0f, 1.0f, to)   * (endAngle - startAngle);

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, r, r, 0.0f,
                       juce::jmin (a1, a2), juce::jmax (a1, a2), true);
    g.setColour (colours::ledAmber.withAlpha (0.85f));
    g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    // A dot where the modulated value actually is, because on a stepped
    // parameter like MODEL the arc is the story and the endpoint is the note.
    const juce::Point<float> tip { centre.x + std::sin (a2) * r, centre.y - std::cos (a2) * r };
    g.fillEllipse (tip.x - 2.2f, tip.y - 2.2f, 4.4f, 4.4f);
}

void drawSwitch (juce::Graphics& g, juce::Rectangle<float> bounds,
                 const juce::String& text, bool on, bool momentaryDown,
                 juce::Colour lamp)
{
    const auto body = bounds.reduced (0.5f);

    g.setColour (juce::Colours::black.withAlpha (momentaryDown ? 0.10f : 0.20f));
    g.fillRoundedRectangle (body.translated (0.0f, 1.5f), 3.0f);

    juce::ColourGradient face (juce::Colour (0xff3a3833), body.getX(), body.getY(),
                               juce::Colour (0xff232120), body.getX(), body.getBottom(), false);
    if (momentaryDown)
        face = juce::ColourGradient (juce::Colour (0xff232120), body.getX(), body.getY(),
                                     juce::Colour (0xff3a3833), body.getX(), body.getBottom(), false);
    g.setGradientFill (face);
    g.fillRoundedRectangle (body, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (body, 3.0f, 1.0f);

    // Indicator, inset at the left the way a panel lamp would be.
    const float lampD = juce::jmin (7.0f, body.getHeight() * 0.32f);
    const juce::Rectangle<float> led { body.getX() + 8.0f, body.getCentreY() - lampD * 0.5f, lampD, lampD };
    g.setColour (on ? lamp : colours::ledOff);
    g.fillEllipse (led);
    if (on)
    {
        g.setColour (lamp.withAlpha (0.30f));
        g.fillEllipse (led.expanded (3.0f));
    }

    const auto textArea = body.withTrimmedLeft (lampD + 14.0f).withTrimmedRight (6.0f);
    const float h = juce::jlimit (8.0f, 11.0f, body.getHeight() * 0.40f);
    const auto f = panelFont (h);
    g.setFont (f);
    g.setColour (juce::Colour (0xffe8e3d8).withAlpha (on ? 1.0f : 0.72f));
    const float tw = trackedWidth (f, text.toUpperCase(), 1.8f);
    drawTracked (g, f, text.toUpperCase(),
                 textArea.getCentreX() - tw * 0.5f, textArea.getCentreY() + h * 0.34f, 1.8f);
}

} // namespace maps_ui
