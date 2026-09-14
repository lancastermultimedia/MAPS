// MAPS — Modular Atonal Percussion Synthesizer
// maps-core: the instrument, as plain C++.
//
// This header is the contract between the core and whatever is driving it —
// the plugin today, libDaisy on the box later. It must never include a
// framework header, and it must never include a vendored Mutable header
// either: the plugin sees this file and nothing else. That rule is the
// architecture, not a style preference.

#pragma once

#include <cstddef>

namespace maps {

// ---------------------------------------------------------------- constants

/** The core always runs at 48 kHz, on the box and in the plugin alike.
    Plaits' sample rate is a compile-time constant reached by every engine and
    by the wavetables (plaits/dsp/dsp.h), so making it a runtime value is a
    rewrite rather than a tweak. Fixing it here means the hardware and the
    plugin run identical code and sound identical; the plugin resamples once,
    at its own boundary. */
constexpr double kCoreSampleRate = 48000.0;

constexpr int kNumVoices = 3;
constexpr int kNumModels = 8;

/** The eight non-pitched "red light" engines, in panel order.
    Plaits 1.2 renumbered these: they sit at indices 16-23 in
    plaits::Voice's registration order, where the 1.0 firmware had them at
    8-15. MODEL on the panel is 1-8 and this enum is the translation. */
enum Model
{
    modelGranularCloud = 0,   // swarm
    modelFilteredNoise,
    modelParticleNoise,
    modelInharmonicString,
    modelModalResonator,
    modelAnalogBassDrum,
    modelAnalogSnareDrum,
    modelAnalogHiHat
};

const char* modelName (int model) noexcept;

/** Clouds' four playback modes, in the order the MODE switch steps through. */
enum CloudsMode
{
    cloudsGranular = 0,
    cloudsStretch,
    cloudsLoopingDelay,
    cloudsSpectral,
    numCloudsModes
};

const char* cloudsModeName (int mode) noexcept;

// --------------------------------------------------------------- parameters

/** One of the three voice strips. Everything here is in the units printed on
    the panel, not normalised: the plugin converts, the core does not. */
struct VoiceParams
{
    float frequency = 36.0f;                 // MIDI note number, 12 .. 96
    float harmonics = 0.35f;                 // 0 .. 1
    float timbre    = 0.50f;                 // 0 .. 1
    float morph     = 0.50f;                 // 0 .. 1
    float decay     = 0.35f;                 // 0 .. 1, internal envelope
    int   model     = modelAnalogBassDrum;   // 0 .. 7
    float level     = 0.80f;                 // 0 .. 1, into the dry mix
    float send      = 0.30f;                 // 0 .. 1, into Clouds
    bool  mute      = false;
};

/** The sequencer. CLOCK is a tempo in BPM and is only consulted when the host
    transport is stopped — when it rolls, MAPS follows the host. */
struct GridsParams
{
    float mapX     = 0.40f;                  // 0 .. 1
    float mapY     = 0.60f;                  // 0 .. 1
    float chaos    = 0.12f;                  // 0 .. 1
    float tempo    = 110.0f;                 // BPM, 40 .. 240
    float swing    = 0.0f;                   // 0 .. 1, delays offbeat 16ths
    float density[kNumVoices] = { 0.72f, 0.48f, 0.78f };
    bool  run      = true;
    bool  fill     = false;                  // momentary: lifts every density
};

/** The granular processor on the send bus. FEEDBACK / SPACE / BLEND / SPREAD
    are Supercell's names for the four parameters the original Clouds panel
    keeps hidden; they map to feedback, reverb, dry_wet and stereo_spread. */
struct CloudsParams
{
    float position = 0.30f;                  // 0 .. 1
    float size     = 0.45f;                  // 0 .. 1
    float density  = 0.70f;                  // 0 .. 1  (see the note below)
    float texture  = 0.50f;                  // 0 .. 1
    float pitch    = 0.0f;                   // semitones, -24 .. +24
    float feedback = 0.30f;                  // 0 .. 1
    float space    = 0.35f;                  // 0 .. 1, reverb
    float blend    = 0.90f;                  // 0 .. 1, dry/wet inside Clouds
    float spread   = 0.60f;                  // 0 .. 1, stereo
    int   mode     = cloudsGranular;
    bool  freeze   = false;
};

// DENSITY, a warning worth repeating wherever it is read: in granular mode
// Clouds treats it as a meta parameter, and between 0.47 and 0.53 the grain
// overlap is set to exactly zero. The processor goes silent. That is upstream
// behaviour, not a bug here. The plugin skews the knob's taper so the dead
// zone never lands under a centred pointer.

struct Parameters
{
    VoiceParams  voice[kNumVoices];
    GridsParams  grids;
    CloudsParams clouds;
    float        output = 0.80f;             // master, 0 .. 1
};

/** What the host's transport is doing. Supplied every block; when
    `playing` is false the sequencer free-runs at GridsParams::tempo. */
struct Transport
{
    bool   playing = false;
    double bpm     = 120.0;
    double ppqPosition = 0.0;                // quarter notes since the start
};

} // namespace maps
