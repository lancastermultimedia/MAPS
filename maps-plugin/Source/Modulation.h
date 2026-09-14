// The modulation matrix: what can be modulated, and by what.
//
// Three LFOs, two routes each. A route is a destination and a bipolar
// attenuator, which is the modular convention and the reason the depth
// control is a centre-zero bar rather than a knob.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "maps/Parameters.h"
#include "maps/Lfo.h"

namespace modulation {

constexpr int kNumLfos      = 3;
constexpr int kRoutesPerLfo = 2;
constexpr int kNumRoutes    = kNumLfos * kRoutesPerLfo;

/** Every knob on the panel, plus OFF at the front.
    **NEVER REORDER THIS.** A saved session stores a destination as an index
    into this list, so moving an entry silently re-points every routing in
    every project anyone has ever saved. Append only. */
enum Dest
{
    off = 0,

    v1Freq, v1Harmonics, v1Timbre, v1Morph, v1Model, v1Decay, v1Level, v1Send,
    v2Freq, v2Harmonics, v2Timbre, v2Morph, v2Model, v2Decay, v2Level, v2Send,
    v3Freq, v3Harmonics, v3Timbre, v3Morph, v3Model, v3Decay, v3Level, v3Send,

    mapX, mapY, chaos, gridsClock, swing, density1, density2, density3,

    cPosition, cSize, cDensity, cTexture, cPitch,
    cFeedback, cSpace, cBlend, cSpread,

    output,

    numDests
};

constexpr int kFieldsPerVoice = 8;

/** Fields in the order they appear above, so a voice destination can be
    addressed arithmetically. */
enum VoiceField { fFreq = 0, fHarmonics, fTimbre, fMorph, fModel, fDecay, fLevel, fSend };

/** `voice` is 1-based, matching the panel. */
inline int voiceDest (int voice, VoiceField field) noexcept
{
    return v1Freq + (voice - 1) * kFieldsPerVoice + (int) field;
}

inline int densityDest (int index) noexcept { return density1 + index; }

/** Printed in the destination selector. */
const char* destName (int dest) noexcept;

/** The parameter each destination drives. Empty for `off`. */
juce::String destParamID (int dest);

} // namespace modulation
