// MAPS — the instrument.
//
//   Grids  ->  three plaits::Voice on the red-light bank  ->  per-voice send
//          ->  clouds::GranularProcessor  ->  stereo out
//
// Deliberately pimpl'd: the Mutable headers pull in stmlib, which needs the
// TEST define to compile off ARM, and none of that should escape into the
// plugin's translation units. Include this file and you get the instrument
// and nothing else.

#pragma once

#include <memory>
#include "maps/Parameters.h"

namespace maps {

class MapsEngine
{
public:
    MapsEngine();
    ~MapsEngine();

    MapsEngine (const MapsEngine&) = delete;
    MapsEngine& operator= (const MapsEngine&) = delete;

    /** Allocates and resets. There is no sample-rate argument on purpose:
        the core is 48 kHz everywhere. Call once, off the audio thread. */
    void prepare();

    /** Clears the tape without reallocating. Safe on the audio thread. */
    void reset() noexcept;

    /** Both are cheap struct copies; call once per block. */
    void setParameters (const Parameters& p) noexcept;
    void setTransport  (const Transport& t) noexcept;

    /** Fires voice `index` immediately, as a MIDI note would.
        `accent` in 0..1 becomes Plaits' accent, which is velocity on the
        drum engines. */
    void trigger (int voiceIndex, float accent) noexcept;

    /** A momentary RESET: the pattern restarts on the next tick. */
    void resetPattern() noexcept;

    /** Renders `numFrames` at 48 kHz. Audio thread; allocates nothing. */
    void process (float* left, float* right, int numFrames) noexcept;

    // ------------------------------------------------------------ metering
    // Read from the message thread for the panel. Approximate by design.

    /** Peak of each voice since the last call, 0..1. */
    float voicePeak (int index) const noexcept;
    /** Peak of the master output since the last call, 0..1. */
    float outputPeak() const noexcept;
    /** Which of the 32 steps the sequencer is on, 0..31. */
    int   currentStep() const noexcept;
    /** Bit i set means voice i fired on the current step. */
    unsigned lastStepTriggers() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace maps
