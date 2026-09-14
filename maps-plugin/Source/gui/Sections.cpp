#include "Sections.h"
#include "../ParamIDs.h"

namespace maps_ui {

// ==================================================================== Section

Section::Section (juce::String title, juce::String modelNumber)
    : sectionTitle (std::move (title)), model (std::move (modelNumber))
{
    setInterceptsMouseClicks (false, true);
}

juce::Rectangle<float> Section::frameArea() const noexcept
{
    // The title sits ON the top rule, half above it, so the frame starts a
    // few pixels down from the component's own edge or the legend is clipped.
    return getLocalBounds().toFloat().reduced (0.5f).withTrimmedTop (8.0f);
}

void Section::paint (juce::Graphics& g)
{
    drawScribedBox (g, frameArea(), sectionTitle, model);
}

juce::Rectangle<float> Section::contentArea() const noexcept
{
    return frameArea().reduced (14.0f, 14.0f);
}

// ================================================================= VoiceStrip

VoiceStrip::VoiceStrip (juce::AudioProcessorValueTreeState& apvts, int voiceNumber)
    : number (voiceNumber),
      state (apvts),
      frequency (apvts, ids::freq (voiceNumber),      "FREQUENCY", Knob::large),
      harmonics (apvts, ids::harmonics (voiceNumber), "HARMONICS", Knob::large),
      timbre    (apvts, ids::timbre (voiceNumber),    "TIMBRE",    Knob::medium),
      morph     (apvts, ids::morph (voiceNumber),     "MORPH",     Knob::medium),
      model     (apvts, ids::model (voiceNumber),     "MODEL",     Knob::medium),
      decay     (apvts, ids::decay (voiceNumber),     "DECAY",     Knob::trim),
      level     (apvts, ids::level (voiceNumber),     "LEVEL",     Knob::trim),
      send      (apvts, ids::send (voiceNumber),      "SEND",      Knob::trim),
      mute      (apvts, ids::mute (voiceNumber),      "MUTE",      colours::ledRed)
{
    for (auto* k : { &frequency, &harmonics, &timbre, &morph, &model, &decay, &level, &send })
        addAndMakeVisible (*k);
    addAndMakeVisible (mute);
    addAndMakeVisible (meter);
    setInterceptsMouseClicks (false, true);
}

void VoiceStrip::resized()
{
    const float w = (float) getWidth();

    meter.setBounds (juce::Rectangle<float> (w - 104.0f, 4.0f, 96.0f, 5.0f).toNearestInt());

    // `extra` widens the box without moving the knob, for MODEL, whose caption
    // prints an engine name long enough to clip a normal-width strip.
    auto place = [] (Knob& k, float centreX, float y, Knob::Size s, bool caption,
                     float extra = 0.0f)
    {
        const float kw = Knob::diameterFor (s) + 34.0f + extra;
        k.setBounds (juce::Rectangle<float> (centreX - kw * 0.5f, y, kw,
                                             Knob::heightFor (s, caption)).toNearestInt());
    };

    // Row A — the two 26 mm caps.
    place (frequency, w * 0.28f,  20.0f, Knob::large, false);
    place (harmonics, w * 0.72f,  20.0f, Knob::large, false);

    // Row B — the three 20 mm.
    place (timbre, w * 0.19f, 112.0f, Knob::medium, false);
    place (morph,  w * 0.50f, 112.0f, Knob::medium, false);
    place (model,  w * 0.81f, 112.0f, Knob::medium, true, 44.0f);

    // Row C — the three 12 mm trims, and MUTE.
    place (decay, w * 0.13f, 198.0f, Knob::trim, false);
    place (level, w * 0.29f, 198.0f, Knob::trim, false);
    place (send,  w * 0.45f, 198.0f, Knob::trim, false);
    mute.setBounds (juce::Rectangle<float> (w * 0.58f, 202.0f, w * 0.36f, 26.0f).toNearestInt());
}

void VoiceStrip::paint (juce::Graphics& g)
{
    drawEngraved (g, "VOICE " + juce::String (number),
                  { 2.0f, 0.0f, 120.0f, 14.0f }, juce::Justification::left, 9.5f, 2.2f);
}

void VoiceStrip::refresh (float peak, const std::function<float (int)>& modOf)
{
    meter.push (peak);

    // The MODEL caption has to follow the MODULATED value, not the knob: with
    // an LFO on it, the knob sits still and the engine changes, and a caption
    // that reported the knob would be quietly lying about what you can hear.
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (ids::model (number))))
    {
        const float off = modOf (modulation::voiceDest (number, modulation::fModel));
        // AudioParameterChoice hides getValue(); go via the index instead.
        const float base = p->convertTo0to1 ((float) p->getIndex());
        const int shown = (int) p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, base + off));
        model.setCaption (juce::String (maps::modelName (shown)));
    }

    using namespace modulation;
    frequency.setModulation (modOf (voiceDest (number, fFreq)));
    harmonics.setModulation (modOf (voiceDest (number, fHarmonics)));
    timbre   .setModulation (modOf (voiceDest (number, fTimbre)));
    morph    .setModulation (modOf (voiceDest (number, fMorph)));
    model    .setModulation (modOf (voiceDest (number, fModel)));
    decay    .setModulation (modOf (voiceDest (number, fDecay)));
    level    .setModulation (modOf (voiceDest (number, fLevel)));
    send     .setModulation (modOf (voiceDest (number, fSend)));
}

// =============================================================== GridsSection

GridsSection::GridsSection (juce::AudioProcessorValueTreeState& apvts)
    : Section ("TOPOGRAPHIC SEQUENCER", "246"),
      mapX  (apvts, ids::mapX,  "MAP X", Knob::medium),
      mapY  (apvts, ids::mapY,  "MAP Y", Knob::medium),
      chaos (apvts, ids::chaos, "CHAOS", Knob::medium),
      clock (apvts, ids::clock, "CLOCK", Knob::medium),
      swing (apvts, ids::swing, "SWING", Knob::medium),
      run   (apvts, ids::run,   "RUN",   colours::ledGreen),
      reset (apvts, ids::reset, "RESET", colours::ledAmber, true),
      fill  (apvts, ids::fill,  "FILL",  colours::ledAmber, true)
{
    for (auto* k : { &mapX, &mapY, &chaos, &clock, &swing })
        addAndMakeVisible (*k);

    for (int i = 0; i < maps::kNumVoices; ++i)
    {
        density[i] = std::make_unique<Knob> (apvts, ids::density (i),
                                             "DENSITY " + juce::String (i + 1), Knob::medium);
        density[i]->setLegendToTheRight (true);
        addAndMakeVisible (*density[i]);
    }

    addAndMakeVisible (run);
    addAndMakeVisible (reset);
    addAndMakeVisible (fill);
    addAndMakeVisible (steps);
}

void GridsSection::resized()
{
    const auto c = contentArea();

    // Top row: the five map and clock controls, evenly across the section.
    const float kw = Knob::diameterFor (Knob::medium) + 34.0f;
    const float rowY = c.getY() + 6.0f;
    Knob* top[5] = { &mapX, &mapY, &chaos, &clock, &swing };
    for (int i = 0; i < 5; ++i)
    {
        const float cx = c.getX() + c.getWidth() * (0.10f + 0.20f * i);
        top[i]->setBounds (juce::Rectangle<float> (cx - kw * 0.5f, rowY, kw,
                                                   Knob::heightFor (Knob::medium, false)).toNearestInt());
    }

    // The three densities on a descending diagonal, legends to the right of
    // each knob. This is Topograph's gesture and it is worth preserving: the
    // diagonal is what tells you the three parts are peers, not a stack.
    const float dy = c.getY() + 80.0f;
    for (int i = 0; i < maps::kNumVoices; ++i)
    {
        // A wider box than the knob, because the legend sits beside it.
        const float dw = Knob::widthWithLegendRight (Knob::medium);
        const float x  = c.getX() + 8.0f + 64.0f * i;
        const float y  = dy + 28.0f * i;
        density[i]->setBounds (juce::Rectangle<float> (
            x, y, dw, Knob::diameterFor (Knob::medium) + 6.0f).toNearestInt());
    }

    const float bx = c.getRight() - 104.0f;
    run.setBounds   (juce::Rectangle<float> (bx, c.getY() + 84.0f,  104.0f, 28.0f).toNearestInt());
    reset.setBounds (juce::Rectangle<float> (bx, c.getY() + 116.0f, 104.0f, 28.0f).toNearestInt());
    fill.setBounds  (juce::Rectangle<float> (bx, c.getY() + 148.0f, 104.0f, 28.0f).toNearestInt());

    steps.setBounds (juce::Rectangle<float> (c.getX() + 2.0f, c.getBottom() - 11.0f,
                                             c.getWidth() - 4.0f, 10.0f).toNearestInt());
}

void GridsSection::refresh (int step, unsigned triggers, const std::function<float (int)>& modOf)
{
    steps.push (step, triggers);

    using namespace modulation;
    mapX .setModulation (modOf (modulation::mapX));
    mapY .setModulation (modOf (modulation::mapY));
    chaos.setModulation (modOf (modulation::chaos));
    clock.setModulation (modOf (modulation::gridsClock));
    swing.setModulation (modOf (modulation::swing));
    for (int i = 0; i < maps::kNumVoices; ++i)
        density[i]->setModulation (modOf (densityDest (i)));
}

// ============================================================== CloudsSection

CloudsSection::CloudsSection (juce::AudioProcessorValueTreeState& apvts)
    : Section ("GRANULAR PROCESSOR", "296"),
      state (apvts),
      position (apvts, ids::position,     "POSITION", Knob::medium),
      size     (apvts, ids::grainSize,    "SIZE",     Knob::medium),
      density  (apvts, ids::grainDensity, "DENSITY",  Knob::medium),
      texture  (apvts, ids::texture,      "TEXTURE",  Knob::medium),
      pitch    (apvts, ids::pitch,        "PITCH",    Knob::medium),
      feedback (apvts, ids::feedback,     "FEEDBACK", Knob::medium),
      space    (apvts, ids::space,        "SPACE",    Knob::medium),
      blend    (apvts, ids::blend,        "BLEND",    Knob::medium),
      spread   (apvts, ids::spread,       "SPREAD",   Knob::medium),
      mode     (apvts, ids::cloudsMode,   "MODE", colours::ledGreen),
      freeze   (apvts, ids::freeze,       "FREEZE", colours::ledAmber)
{
    for (auto* k : { &position, &size, &density, &texture, &pitch,
                     &feedback, &space, &blend, &spread })
        addAndMakeVisible (*k);
    addAndMakeVisible (mode);
    addAndMakeVisible (freeze);
}

void CloudsSection::resized()
{
    const auto c = contentArea();
    const float kw = Knob::diameterFor (Knob::medium) + 34.0f;
    const float h  = Knob::heightFor (Knob::medium, false);

    // Row one: the five Clouds primaries, in the order the module has them.
    Knob* r1[5] = { &position, &size, &density, &texture, &pitch };
    for (int i = 0; i < 5; ++i)
    {
        const float cx = c.getX() + c.getWidth() * (0.10f + 0.20f * i);
        r1[i]->setBounds (juce::Rectangle<float> (cx - kw * 0.5f, c.getY() + 6.0f, kw, h).toNearestInt());
    }

    // Row two: the four Supercell exposes and the original hides.
    Knob* r2[4] = { &feedback, &space, &blend, &spread };
    for (int i = 0; i < 4; ++i)
    {
        const float cx = c.getX() + c.getWidth() * (0.14f + 0.24f * i);
        r2[i]->setBounds (juce::Rectangle<float> (cx - kw * 0.5f, c.getY() + 92.0f, kw, h).toNearestInt());
    }

    const float by = c.getBottom() - 36.0f;
    mode.setBounds   (juce::Rectangle<float> (c.getX() + c.getWidth() * 0.10f, by, 210.0f, 30.0f).toNearestInt());
    freeze.setBounds (juce::Rectangle<float> (c.getX() + c.getWidth() * 0.10f + 224.0f, by, 130.0f, 30.0f).toNearestInt());
}

void CloudsSection::refresh (const std::function<float (int)>& modOf)
{
    mode.refresh();

    using namespace modulation;
    position.setModulation (modOf (cPosition));
    size    .setModulation (modOf (cSize));
    density .setModulation (modOf (cDensity));
    texture .setModulation (modOf (cTexture));
    pitch   .setModulation (modOf (cPitch));
    feedback.setModulation (modOf (cFeedback));
    space   .setModulation (modOf (cSpace));
    blend   .setModulation (modOf (cBlend));
    spread  .setModulation (modOf (cSpread));
}

} // namespace maps_ui
