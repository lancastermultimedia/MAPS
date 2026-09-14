// Every parameter MAPS exposes, in one place.
//
// PARAMETER IDS ARE FOREVER. A saved session, in any host, stores these
// strings. Change one and every project that used the plugin loses that
// control's value silently — no error, just a knob that snapped back to
// default. Add freely, rename never.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "maps/Parameters.h"

namespace ids {

// ---- per voice; n is 1, 2 or 3, matching the panel legends ---------------
inline juce::String voice (int n, const char* leaf) { return "v" + juce::String (n) + "_" + leaf; }

inline juce::String freq (int n)      { return voice (n, "freq"); }
inline juce::String harmonics (int n) { return voice (n, "harmonics"); }
inline juce::String timbre (int n)    { return voice (n, "timbre"); }
inline juce::String morph (int n)     { return voice (n, "morph"); }
inline juce::String model (int n)     { return voice (n, "model"); }
inline juce::String decay (int n)     { return voice (n, "decay"); }
inline juce::String level (int n)     { return voice (n, "level"); }
inline juce::String send (int n)      { return voice (n, "send"); }
inline juce::String mute (int n)      { return voice (n, "mute"); }

// ---- 246, the sequencer -------------------------------------------------
static const char* const mapX    = "g_map_x";
static const char* const mapY    = "g_map_y";
static const char* const chaos   = "g_chaos";
static const char* const clock   = "g_clock";
static const char* const swing   = "g_swing";
static const char* const density1 = "g_density_1";
static const char* const density2 = "g_density_2";
static const char* const density3 = "g_density_3";
static const char* const run     = "g_run";
static const char* const reset   = "g_reset";
static const char* const fill    = "g_fill";

inline const char* density (int n) { return n == 0 ? density1 : (n == 1 ? density2 : density3); }

// ---- 296, the granular processor ----------------------------------------
static const char* const position = "c_position";
static const char* const grainSize = "c_size";
static const char* const grainDensity = "c_density";
static const char* const texture  = "c_texture";
static const char* const pitch    = "c_pitch";
static const char* const feedback = "c_feedback";
static const char* const space    = "c_space";
static const char* const blend    = "c_blend";
static const char* const spread   = "c_spread";
static const char* const cloudsMode = "c_mode";
static const char* const freeze   = "c_freeze";

// ---- master --------------------------------------------------------------
static const char* const output = "output";

// ---- 266, the modulation sources; n is 1..3 ------------------------------
inline juce::String lfo (int n, const char* leaf) { return "lfo" + juce::String (n) + "_" + leaf; }

inline juce::String lfoRate (int n)  { return lfo (n, "rate"); }
inline juce::String lfoWave (int n)  { return lfo (n, "wave"); }
inline juce::String lfoSync (int n)  { return lfo (n, "sync"); }
/** route is 0 or 1 - the two attenuators under each LFO. */
inline juce::String lfoDest  (int n, int route) { return lfo (n, route == 0 ? "dest_a"  : "dest_b"); }
inline juce::String lfoDepth (int n, int route) { return lfo (n, route == 0 ? "depth_a" : "depth_b"); }

} // namespace ids

namespace maps_plugin {

/** Clouds' DENSITY has a dead zone: in granular mode the value between 0.47
    and 0.53 sets grain overlap to exactly zero and the processor goes silent.
    That is upstream behaviour and it is musically meaningful — it is the line
    between clocked grains and random ones — but a knob that does nothing at
    twelve o'clock reads as a broken plugin.

    So the knob is not linear. It runs fast through the dead zone, which ends
    up sitting at about eight o'clock and takes five per cent of the travel to
    cross, and the useful range either side gets the rest. */
inline float mapCloudsDensity (float knob) noexcept
{
    knob = juce::jlimit (0.0f, 1.0f, knob);
    if (knob < 0.35f) return knob * (0.47f / 0.35f);
    if (knob < 0.40f) return 0.47f + (knob - 0.35f) * (0.06f / 0.05f);
    return 0.53f + (knob - 0.40f) * (0.47f / 0.60f);
}

} // namespace maps_plugin
