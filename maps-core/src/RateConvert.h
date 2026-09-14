// Sample-rate plumbing between Plaits (48 kHz) and Clouds (32 kHz).
//
// Both rates are compile-time constants inside the vendored code, reached by
// dozens of engines and by the wavetables, so the only honest option is to
// convert between them. The box has exactly the same problem — running the
// Daisy codec at 48 kHz does not make Clouds' 32 kHz go away, it hides it —
// so this lives in the core, where both builds share it.

#pragma once

#include <cmath>
#include <cstddef>

namespace maps {

/** One biquad section, transposed direct form II. */
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void setLowpass (double fc, double fs, double q) noexcept
    {
        const double w0    = 2.0 * M_PI * fc / fs;
        const double cosw0 = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double b0n   = (1.0 - cosw0) * 0.5;
        const double b1n   =  1.0 - cosw0;
        const double a0    =  1.0 + alpha;

        b0 = (float) (b0n / a0);
        b1 = (float) (b1n / a0);
        b2 = (float) (b0n / a0);
        a1 = (float) ((-2.0 * cosw0) / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0f; }
};

/** Sixth-order Butterworth lowpass: three biquads at the Butterworth Q set.
    At 14 kHz on a 48 kHz stream this is roughly -30 dB by 16 kHz, which is
    what the 32 kHz side needs to stay clean. */
struct Lowpass6
{
    Biquad s[3];

    void set (double fc, double fs) noexcept
    {
        // Butterworth pole Qs for n = 6.
        static const double q[3] = { 0.51764, 0.70711, 1.93185 };
        for (int i = 0; i < 3; ++i)
            s[i].setLowpass (fc, fs, q[i]);
    }

    inline float process (float x) noexcept
    {
        return s[2].process (s[1].process (s[0].process (x)));
    }

    void reset() noexcept { for (auto& b : s) b.reset(); }
};

/** One-pole DC blocker at about 5 Hz.
    Plaits' ChannelPostProcessor writes `Clip16(1 + ...)` — a deliberate one-LSB
    offset in the firmware, worth 3e-5 in float and multiplied by three voices
    here. Small, but it is a constant pedestal under everything, it eats
    headroom on the soft clipper, and a granular buffer full of DC does not
    sound like anything you want. */
struct DcBlock
{
    float x1 = 0.0f, y1 = 0.0f;
    float r = 0.99935f;                       // ~5 Hz at 48 kHz

    inline float process (float x) noexcept
    {
        const float y = x - x1 + r * y1;
        x1 = x;
        y1 = y;
        return y;
    }

    void reset() noexcept { x1 = y1 = 0.0f; }
};

/** Catmull-Rom, the same interpolator the delay taps in Mandela Effect use.
    Fed a signal that is already band-limited well below Nyquist, it is
    inaudible; fed a full-bandwidth one it is not, which is why every call
    site here has a Lowpass6 in front of it. */
inline float catmullRom (float ym1, float y0, float y1, float y2, float t) noexcept
{
    const float c0 = y0;
    const float c1 = 0.5f * (y1 - ym1);
    const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * t + c2) * t + c1) * t + c0;
}

/** A small circular buffer read at a fractional rate. Used for the wet
    return: Clouds writes 32 frames at a time at 32 kHz, the mixer reads
    continuously at 48 kHz, and the lead built in at reset() is what stops
    the read from catching the write. */
template <int kSize>
struct FractionalFifo
{
    float bufL[kSize] = { 0.0f };
    float bufR[kSize] = { 0.0f };
    int    write = 0;
    double read  = 0.0;

    /** `lead` frames of silence, so the reader always has four points of
        history and never overtakes the writer. */
    void reset (int lead) noexcept
    {
        for (int i = 0; i < kSize; ++i) bufL[i] = bufR[i] = 0.0f;
        write = lead;
        read  = 0.0;
    }

    inline void push (float l, float r) noexcept
    {
        bufL[write] = l;
        bufR[write] = r;
        if (++write >= kSize) write = 0;
    }

    /** Advances by `ratio` output-rate frames and returns the interpolated
        sample. `ratio` is inputRate / outputRate. */
    inline void advance (double ratio, float& outL, float& outR) noexcept
    {
        const int   i = (int) read;
        const float t = (float) (read - i);

        auto at = [this] (int n) noexcept
        {
            n %= kSize;
            return n < 0 ? n + kSize : n;
        };

        const int im1 = at (i - 1), i0 = at (i), i1 = at (i + 1), i2 = at (i + 2);
        outL = catmullRom (bufL[im1], bufL[i0], bufL[i1], bufL[i2], t);
        outR = catmullRom (bufR[im1], bufR[i0], bufR[i1], bufR[i2], t);

        read += ratio;
        if (read >= kSize) read -= kSize;
    }
};

} // namespace maps
