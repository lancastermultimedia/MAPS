#include "Presets.h"
#include "ParamIDs.h"
#include "Modulation.h"
#include "maps/Lfo.h"
#include "maps/Parameters.h"

namespace presets {

namespace {

struct Voice
{
    int   model;
    float freq, harmonics, timbre, morph, decay, level, send;
};

/** One modulation source, as a preset stores it. `rate` is the knob position,
    which means a division when synced and a frequency when not. */
struct Mod
{
    float rate;
    int   wave;
    bool  sync;
    int   destA; float depthA;
    int   destB; float depthB;
};

constexpr Mod kModOff { 0.5f, maps::lfoTriangle, true, modulation::off, 0.0f, modulation::off, 0.0f };

struct Preset
{
    const char* name;
    Voice v[3];
    float mapX, mapY, chaos, clock, swing, d1, d2, d3;
    float position, size, density, texture, pitch, feedback, space, blend, spread;
    int   mode;
    float output;
    Mod   mod[modulation::kNumLfos];
};

using namespace maps;

// A note on DENSITY (the Clouds one): these are knob positions, not the value
// Clouds sees — mapCloudsDensity is between them. 0.6 and up is where grains
// actually overlap.
const Preset kPresets[] =
{
    { "246 / 258 / 296",                       // the default, straight down the middle
      { { modelAnalogBassDrum,  36.0f, 0.30f, 0.45f, 0.55f, 0.35f, 1.00f, 0.15f },
        { modelAnalogSnareDrum, 48.0f, 0.45f, 0.55f, 0.50f, 0.30f, 0.85f, 0.35f },
        { modelAnalogHiHat,     72.0f, 0.60f, 0.70f, 0.35f, 0.22f, 0.70f, 0.60f } },
      0.40f, 0.60f, 0.12f, 110.0f, 0.00f, 0.72f, 0.48f, 0.78f,
      0.30f, 0.45f, 0.62f, 0.50f, 0.0f, 0.30f, 0.35f, 0.90f, 0.60f, cloudsGranular, 0.80f,
      { kModOff, kModOff, kModOff }  },

    { "Dry Kit",                               // no cloud at all: hear the voices
      { { modelAnalogBassDrum,  33.0f, 0.22f, 0.40f, 0.60f, 0.30f, 1.00f, 0.00f },
        { modelAnalogSnareDrum, 50.0f, 0.50f, 0.60f, 0.45f, 0.26f, 0.90f, 0.00f },
        { modelAnalogHiHat,     76.0f, 0.55f, 0.75f, 0.30f, 0.16f, 0.65f, 0.00f } },
      0.30f, 0.35f, 0.05f, 120.0f, 0.00f, 0.75f, 0.50f, 0.80f,
      0.30f, 0.45f, 0.62f, 0.50f, 0.0f, 0.00f, 0.00f, 0.00f, 0.50f, cloudsGranular, 0.85f,
      { kModOff, kModOff, kModOff }  },

    { "Topograph",                             // the reference patch, roughly
      { { modelAnalogBassDrum,  35.0f, 0.35f, 0.50f, 0.50f, 0.34f, 1.00f, 0.25f },
        { modelParticleNoise,   55.0f, 0.40f, 0.55f, 0.45f, 0.30f, 0.70f, 0.55f },
        { modelModalResonator,  74.0f, 0.55f, 0.62f, 0.40f, 0.20f, 0.60f, 0.70f } },
      0.55f, 0.45f, 0.20f, 104.0f, 0.10f, 0.70f, 0.55f, 0.72f,
      0.35f, 0.50f, 0.70f, 0.55f, 0.0f, 0.35f, 0.40f, 0.95f, 0.75f, cloudsGranular, 0.78f,
      { kModOff, kModOff, kModOff }  },

    { "Slow Weather",                          // almost no transients, all texture
      { { modelFilteredNoise,   30.0f, 0.55f, 0.35f, 0.70f, 0.85f, 0.55f, 0.85f },
        { modelGranularCloud,   45.0f, 0.70f, 0.40f, 0.60f, 0.80f, 0.45f, 0.90f },
        { modelInharmonicString,62.0f, 0.60f, 0.55f, 0.45f, 0.75f, 0.40f, 0.90f } },
      0.65f, 0.70f, 0.35f,  72.0f, 0.00f, 0.35f, 0.28f, 0.40f,
      0.45f, 0.75f, 0.78f, 0.70f, -12.0f, 0.55f, 0.70f, 1.00f, 0.85f, cloudsGranular, 0.72f,
      { kModOff, kModOff, kModOff }  },

    { "Hard Metal",                            // modal everything, short and bright
      { { modelModalResonator,  40.0f, 0.75f, 0.60f, 0.25f, 0.18f, 0.95f, 0.20f },
        { modelModalResonator,  58.0f, 0.65f, 0.72f, 0.35f, 0.14f, 0.80f, 0.35f },
        { modelModalResonator,  79.0f, 0.55f, 0.80f, 0.20f, 0.10f, 0.70f, 0.50f } },
      0.20f, 0.80f, 0.28f, 132.0f, 0.00f, 0.62f, 0.52f, 0.85f,
      0.25f, 0.30f, 0.68f, 0.80f, 12.0f, 0.25f, 0.30f, 0.75f, 0.65f, cloudsGranular, 0.76f,
      { kModOff, kModOff, kModOff }  },

    { "Swung Room",
      { { modelAnalogBassDrum,  34.0f, 0.28f, 0.42f, 0.58f, 0.32f, 1.00f, 0.20f },
        { modelAnalogSnareDrum, 47.0f, 0.55f, 0.50f, 0.55f, 0.28f, 0.85f, 0.40f },
        { modelAnalogHiHat,     78.0f, 0.45f, 0.68f, 0.28f, 0.12f, 0.60f, 0.35f } },
      0.45f, 0.25f, 0.15f,  92.0f, 0.62f, 0.68f, 0.45f, 0.82f,
      0.20f, 0.35f, 0.60f, 0.45f, 0.0f, 0.20f, 0.55f, 0.85f, 0.55f, cloudsGranular, 0.80f,
      { kModOff, kModOff, kModOff }  },

    { "Stretch",                               // Clouds in time-stretch
      { { modelAnalogBassDrum,  36.0f, 0.30f, 0.45f, 0.55f, 0.35f, 0.70f, 0.65f },
        { modelInharmonicString,52.0f, 0.50f, 0.60f, 0.40f, 0.45f, 0.60f, 0.80f },
        { modelParticleNoise,   70.0f, 0.45f, 0.65f, 0.50f, 0.30f, 0.50f, 0.85f } },
      0.50f, 0.50f, 0.18f,  96.0f, 0.00f, 0.60f, 0.45f, 0.60f,
      0.40f, 0.65f, 0.55f, 0.50f, 0.0f, 0.40f, 0.45f, 1.00f, 0.70f, cloudsStretch, 0.75f,
      { kModOff, kModOff, kModOff }  },

    { "Spectral Hold",                         // freeze on, spectral mode
      { { modelModalResonator,  38.0f, 0.60f, 0.55f, 0.45f, 0.40f, 0.60f, 0.85f },
        { modelGranularCloud,   54.0f, 0.65f, 0.50f, 0.55f, 0.50f, 0.50f, 0.90f },
        { modelFilteredNoise,   76.0f, 0.50f, 0.60f, 0.40f, 0.35f, 0.45f, 0.90f } },
      0.70f, 0.30f, 0.25f,  80.0f, 0.00f, 0.45f, 0.35f, 0.50f,
      0.50f, 0.60f, 0.72f, 0.65f, 0.0f, 0.30f, 0.60f, 1.00f, 0.80f, cloudsSpectral, 0.74f,
      { kModOff, kModOff, kModOff }  },

    { "Busy Machine",                          // high density, chaos up
      { { modelAnalogBassDrum,  35.0f, 0.40f, 0.50f, 0.50f, 0.24f, 0.95f, 0.10f },
        { modelAnalogSnareDrum, 52.0f, 0.60f, 0.55f, 0.60f, 0.20f, 0.80f, 0.30f },
        { modelAnalogHiHat,     84.0f, 0.35f, 0.75f, 0.25f, 0.08f, 0.65f, 0.45f } },
      0.85f, 0.15f, 0.55f, 140.0f, 0.00f, 0.88f, 0.72f, 0.92f,
      0.15f, 0.25f, 0.75f, 0.35f, 0.0f, 0.30f, 0.25f, 0.70f, 0.50f, cloudsGranular, 0.72f,
      { kModOff, kModOff, kModOff }  },

    { "Low Fifth",                             // all three tuned low, long decay
      { { modelInharmonicString,26.0f, 0.55f, 0.45f, 0.55f, 0.70f, 0.85f, 0.55f },
        { modelInharmonicString,33.0f, 0.50f, 0.50f, 0.50f, 0.65f, 0.70f, 0.60f },
        { modelModalResonator,  45.0f, 0.60f, 0.55f, 0.45f, 0.55f, 0.55f, 0.70f } },
      0.35f, 0.55f, 0.10f,  86.0f, 0.00f, 0.55f, 0.35f, 0.45f,
      0.35f, 0.55f, 0.66f, 0.60f, -7.0f, 0.45f, 0.55f, 0.92f, 0.70f, cloudsGranular, 0.76f,
      { kModOff, kModOff, kModOff }  },

    { "Looping Delay",
      { { modelAnalogBassDrum,  36.0f, 0.30f, 0.45f, 0.55f, 0.32f, 0.90f, 0.45f },
        { modelAnalogSnareDrum, 48.0f, 0.45f, 0.55f, 0.50f, 0.28f, 0.75f, 0.55f },
        { modelParticleNoise,   72.0f, 0.50f, 0.60f, 0.45f, 0.24f, 0.60f, 0.70f } },
      0.40f, 0.60f, 0.12f, 118.0f, 0.00f, 0.70f, 0.48f, 0.75f,
      0.30f, 0.50f, 0.62f, 0.55f, 0.0f, 0.55f, 0.40f, 0.85f, 0.65f, cloudsLoopingDelay, 0.78f,
      { kModOff, kModOff, kModOff }  },

    { "Nothing But Cloud",                     // levels down, sends up
      { { modelParticleNoise,   32.0f, 0.55f, 0.50f, 0.60f, 0.50f, 0.00f, 1.00f },
        { modelGranularCloud,   50.0f, 0.60f, 0.55f, 0.50f, 0.55f, 0.00f, 1.00f },
        { modelFilteredNoise,   68.0f, 0.50f, 0.60f, 0.45f, 0.45f, 0.00f, 1.00f } },
      0.60f, 0.40f, 0.30f, 100.0f, 0.00f, 0.60f, 0.45f, 0.65f,
      0.45f, 0.70f, 0.80f, 0.75f, 0.0f, 0.60f, 0.65f, 1.00f, 0.90f, cloudsGranular, 0.70f,
      { kModOff, kModOff, kModOff }  },

    { "Shuffled Engines",                      // a different drum every beat
      { { modelParticleNoise,   36.0f, 0.35f, 0.50f, 0.50f, 0.30f, 1.00f, 0.20f },
        { modelInharmonicString,50.0f, 0.45f, 0.55f, 0.50f, 0.26f, 0.85f, 0.35f },
        { modelModalResonator,  72.0f, 0.50f, 0.65f, 0.40f, 0.18f, 0.70f, 0.55f } },
      0.42f, 0.55f, 0.15f, 112.0f, 0.00f, 0.70f, 0.50f, 0.80f,
      0.30f, 0.45f, 0.64f, 0.55f, 0.0f, 0.30f, 0.35f, 0.90f, 0.65f, cloudsGranular, 0.78f,
      // Two routes from one source onto the same destination sum, which is how
      // a single LFO reaches all eight engines instead of half of them.
      { { 0.47f, lfoRandom, true, modulation::voiceDest (1, modulation::fModel), 1.0f,
                              modulation::voiceDest (1, modulation::fModel), 1.0f },
        { 0.33f, lfoRandom, true, modulation::voiceDest (2, modulation::fModel), 1.0f,
                              modulation::voiceDest (3, modulation::fModel), 1.0f },
        { 0.13f, lfoDrift,  true, modulation::cPosition, 0.45f,
                              modulation::cTexture,  0.35f } } },

    { "Engine Ramp",                           // walks the bank over two bars
      { { modelGranularCloud,   34.0f, 0.40f, 0.50f, 0.55f, 0.35f, 1.00f, 0.30f },
        { modelFilteredNoise,   52.0f, 0.45f, 0.60f, 0.45f, 0.25f, 0.80f, 0.45f },
        { modelAnalogHiHat,     78.0f, 0.40f, 0.70f, 0.30f, 0.14f, 0.65f, 0.35f } },
      0.55f, 0.35f, 0.22f, 120.0f, 0.00f, 0.74f, 0.52f, 0.85f,
      0.35f, 0.50f, 0.66f, 0.60f, 0.0f, 0.35f, 0.40f, 0.92f, 0.70f, cloudsGranular, 0.76f,
      { { 0.07f, lfoRampUp,   true, modulation::voiceDest (1, modulation::fModel), 1.0f,
                                modulation::voiceDest (1, modulation::fModel), 1.0f },
        { 0.47f, lfoSquare,   true, modulation::voiceDest (2, modulation::fModel), 0.8f,
                                modulation::off, 0.0f },
        { 0.13f, lfoSine,     true, modulation::cSize, 0.5f,
                                modulation::cFeedback, 0.3f } } },
};

constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

void set (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

} // namespace

int count() { return kNumPresets; }

juce::String name (int index)
{
    if (index < 0 || index >= kNumPresets) return {};
    return kPresets[index].name;
}

void apply (juce::AudioProcessorValueTreeState& apvts, int index)
{
    if (index < 0 || index >= kNumPresets)
        return;
    const Preset& p = kPresets[index];

    for (int v = 0; v < 3; ++v)
    {
        const Voice& s = p.v[v];
        const int n = v + 1;
        set (apvts, ids::freq (n),      s.freq);
        set (apvts, ids::harmonics (n), s.harmonics);
        set (apvts, ids::timbre (n),    s.timbre);
        set (apvts, ids::morph (n),     s.morph);
        set (apvts, ids::model (n),     (float) s.model);
        set (apvts, ids::decay (n),     s.decay);
        set (apvts, ids::level (n),     s.level);
        set (apvts, ids::send (n),      s.send);
        set (apvts, ids::mute (n),      0.0f);
    }

    set (apvts, ids::mapX,  p.mapX);
    set (apvts, ids::mapY,  p.mapY);
    set (apvts, ids::chaos, p.chaos);
    set (apvts, ids::clock, p.clock);
    set (apvts, ids::swing, p.swing);
    set (apvts, ids::density1, p.d1);
    set (apvts, ids::density2, p.d2);
    set (apvts, ids::density3, p.d3);
    set (apvts, ids::run,   1.0f);
    set (apvts, ids::fill,  0.0f);
    set (apvts, ids::reset, 0.0f);

    set (apvts, ids::position,     p.position);
    set (apvts, ids::grainSize,    p.size);
    set (apvts, ids::grainDensity, p.density);
    set (apvts, ids::texture,      p.texture);
    set (apvts, ids::pitch,        p.pitch);
    set (apvts, ids::feedback,     p.feedback);
    set (apvts, ids::space,        p.space);
    set (apvts, ids::blend,        p.blend);
    set (apvts, ids::spread,       p.spread);
    set (apvts, ids::cloudsMode,   (float) p.mode);
    // FREEZE is deliberately left alone: a preset that silently freezes the
    // buffer is a preset that sounds broken until you find the button.
    set (apvts, ids::freeze,       0.0f);

    set (apvts, ids::output, p.output);

    for (int n = 1; n <= modulation::kNumLfos; ++n)
    {
        const Mod& m = p.mod[n - 1];
        set (apvts, ids::lfoRate (n), m.rate);
        set (apvts, ids::lfoWave (n), (float) m.wave);
        set (apvts, ids::lfoSync (n), m.sync ? 1.0f : 0.0f);
        set (apvts, ids::lfoDest  (n, 0), (float) m.destA);
        set (apvts, ids::lfoDepth (n, 0), m.depthA);
        set (apvts, ids::lfoDest  (n, 1), (float) m.destB);
        set (apvts, ids::lfoDepth (n, 1), m.depthB);
    }
}

} // namespace presets
