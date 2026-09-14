// Modulation sources. Header-only, JUCE-free, like everything else in the
// core, so the box can grow a modulation section later without this being
// rewritten.
//
// The important design decision is that a synced LFO does not accumulate
// phase. It computes it from the host's position in quarter notes:
//
//     phase = frac (ppq / beatsPerCycle)
//     cycle = floor (ppq / beatsPerCycle)
//
// which means it is exactly in step with the grid, it is immediately correct
// after a locate or a loop wrap, and — because the sample-and-hold value is a
// hash of the cycle number rather than a running random — the same bar of the
// timeline gives the same result every time you play it. An accumulating LFO
// gives you none of that, and "the engines switch on a different beat each
// time I hit play" is not a feature.

#pragma once

#include <cmath>
#include <cstdint>

namespace maps {

enum LfoWave
{
    lfoSine = 0,
    lfoTriangle,
    lfoRampUp,
    lfoRampDown,
    lfoSquare,
    lfoRandom,        // sample and hold: one new value per cycle
    lfoDrift,         // random, but interpolated between cycles
    numLfoWaves
};

const char* lfoWaveName (int wave) noexcept;

/** The musical divisions a synced LFO can run at, in quarter notes per cycle.
    NEVER REORDER THIS: the index is what a saved session stores. */
struct LfoDivision { float beats; const char* name; };

inline const LfoDivision* lfoDivisions() noexcept
{
    static const LfoDivision kDivisions[] = {
        { 16.0f,       "4 BARS"  },
        {  8.0f,       "2 BARS"  },
        {  4.0f,       "1 BAR"   },
        {  3.0f,       "1/2 D"   },
        {  2.0f,       "1/2"     },
        {  4.0f / 3.0f,"1/2 T"   },
        {  1.5f,       "1/4 D"   },
        {  1.0f,       "1/4"     },
        {  2.0f / 3.0f,"1/4 T"   },
        {  0.75f,      "1/8 D"   },
        {  0.5f,       "1/8"     },
        {  1.0f / 3.0f,"1/8 T"   },
        {  0.25f,      "1/16"    },
        {  1.0f / 6.0f,"1/16 T"  },
        {  0.125f,     "1/32"    },
    };
    return kDivisions;
}

constexpr int kNumLfoDivisions = 15;

/** Maps a 0..1 knob to a division index. Laid out so the useful musical
    values — bar, half, quarter, eighth — fall in the middle of the travel. */
inline int lfoDivisionForKnob (float knob) noexcept
{
    const int i = (int) (knob * (float) kNumLfoDivisions);
    return i < 0 ? 0 : (i >= kNumLfoDivisions ? kNumLfoDivisions - 1 : i);
}

/** Maps a 0..1 knob to a free-running rate in Hz, roughly exponential from a
    forty-second cycle to buzzing. */
inline float lfoRateForKnob (float knob) noexcept
{
    knob = knob < 0.0f ? 0.0f : (knob > 1.0f ? 1.0f : knob);
    return 0.025f * std::pow (800.0f, knob);          // 0.025 Hz .. 20 Hz
}

class Lfo
{
public:
    void reset() noexcept
    {
        phase = 0.0;
        cycle = 0;
        current = 0.0f;
    }

    /** Synced: derive everything from the host's musical position. */
    void updateSynced (double ppqPosition, float beatsPerCycle, int wave) noexcept
    {
        const double b = beatsPerCycle > 1.0e-4f ? (double) beatsPerCycle : 1.0;
        const double p = ppqPosition / b;
        const double f = p - std::floor (p);
        cycle = (long long) std::floor (p);
        phase = f;
        current = shape (wave, (float) f, cycle);
    }

    /** Free-running: advance by a block. */
    void updateFree (float rateHz, double seconds, int wave) noexcept
    {
        phase += (double) rateHz * seconds;
        while (phase >= 1.0) { phase -= 1.0; ++cycle; }
        while (phase <  0.0) { phase += 1.0; --cycle; }
        current = shape (wave, (float) phase, cycle);
    }

    /** Bipolar, -1 .. 1. */
    float value() const noexcept { return current; }

private:
    // A cheap integer hash, so the random values are a function of which cycle
    // you are in rather than of how long the plugin has been running.
    static float hashToBipolar (long long n) noexcept
    {
        uint64_t x = (uint64_t) n * 0x9e3779b97f4a7c15ull + 0x853c49e6748fea9bull;
        x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ull;
        x ^= x >> 27; x *= 0x94d049bb133111ebull;
        x ^= x >> 31;
        return (float) ((double) (uint32_t) (x >> 32) / 2147483648.0 - 1.0);
    }

    static float shape (int wave, float f, long long c) noexcept
    {
        switch (wave)
        {
            case lfoSine:     return std::sin (f * 6.2831853f);
            case lfoTriangle: return f < 0.5f ? (f * 4.0f - 1.0f) : (3.0f - f * 4.0f);
            case lfoRampUp:   return f * 2.0f - 1.0f;
            case lfoRampDown: return 1.0f - f * 2.0f;
            case lfoSquare:   return f < 0.5f ? 1.0f : -1.0f;
            case lfoRandom:   return hashToBipolar (c);
            case lfoDrift:
            {
                const float a = hashToBipolar (c);
                const float b = hashToBipolar (c + 1);
                const float t = f * f * (3.0f - 2.0f * f);     // smoothstep
                return a + (b - a) * t;
            }
            default: break;
        }
        return 0.0f;
    }

    double    phase = 0.0;
    long long cycle = 0;
    float     current = 0.0f;
};

} // namespace maps
