#include "Modulation.h"
#include "ParamIDs.h"

namespace modulation {

namespace {

struct Entry { const char* label; };

// Same order as the enum, and checked against it below.
const Entry kEntries[] =
{
    { "OFF" },

    { "1 FREQUENCY" }, { "1 HARMONICS" }, { "1 TIMBRE" }, { "1 MORPH" },
    { "1 MODEL" },     { "1 DECAY" },     { "1 LEVEL" },  { "1 SEND" },
    { "2 FREQUENCY" }, { "2 HARMONICS" }, { "2 TIMBRE" }, { "2 MORPH" },
    { "2 MODEL" },     { "2 DECAY" },     { "2 LEVEL" },  { "2 SEND" },
    { "3 FREQUENCY" }, { "3 HARMONICS" }, { "3 TIMBRE" }, { "3 MORPH" },
    { "3 MODEL" },     { "3 DECAY" },     { "3 LEVEL" },  { "3 SEND" },

    { "MAP X" }, { "MAP Y" }, { "CHAOS" }, { "CLOCK" }, { "SWING" },
    { "DENSITY 1" }, { "DENSITY 2" }, { "DENSITY 3" },

    { "POSITION" }, { "SIZE" }, { "GRAIN DENSITY" }, { "TEXTURE" }, { "PITCH" },
    { "FEEDBACK" }, { "SPACE" }, { "BLEND" }, { "SPREAD" },

    { "OUTPUT" },
};

static_assert ((int) (sizeof (kEntries) / sizeof (kEntries[0])) == (int) numDests,
               "the destination labels and the Dest enum have drifted apart");

} // namespace

const char* destName (int dest) noexcept
{
    if (dest < 0 || dest >= numDests) return "OFF";
    return kEntries[dest].label;
}

juce::String destParamID (int dest)
{
    if (dest <= off || dest >= numDests)
        return {};

    if (dest <= v3Send)
    {
        const int n     = (dest - v1Freq) / kFieldsPerVoice + 1;
        const int field = (dest - v1Freq) % kFieldsPerVoice;
        switch (field)
        {
            case fFreq:      return ids::freq (n);
            case fHarmonics: return ids::harmonics (n);
            case fTimbre:    return ids::timbre (n);
            case fMorph:     return ids::morph (n);
            case fModel:     return ids::model (n);
            case fDecay:     return ids::decay (n);
            case fLevel:     return ids::level (n);
            case fSend:      return ids::send (n);
            default: break;
        }
        return {};
    }

    switch (dest)
    {
        case mapX:      return ids::mapX;
        case mapY:      return ids::mapY;
        case chaos:     return ids::chaos;
        case gridsClock: return ids::clock;
        case swing:     return ids::swing;
        case density1:  return ids::density1;
        case density2:  return ids::density2;
        case density3:  return ids::density3;
        case cPosition: return ids::position;
        case cSize:     return ids::grainSize;
        case cDensity:  return ids::grainDensity;
        case cTexture:  return ids::texture;
        case cPitch:    return ids::pitch;
        case cFeedback: return ids::feedback;
        case cSpace:    return ids::space;
        case cBlend:    return ids::blend;
        case cSpread:   return ids::spread;
        case output:    return ids::output;
        default: break;
    }
    return {};
}

} // namespace modulation
