#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

#include "maps/MapsEngine.h"
#include "maps/Lfo.h"
#include "ParamIDs.h"
#include "Modulation.h"

inline float maps_plugin_catmull (float ym1, float y0, float y1, float y2, float t) noexcept
{
    const float c1 = 0.5f * (y1 - ym1);
    const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * t + c2) * t + c1) * t + y0;
}

/** Converts the core's fixed 48 kHz to whatever the host is running.
    Catmull-Rom over a ring the core fills in its own time, with a lowpass in
    front when the host rate is lower than the core's. */
class CoreResampler
{
public:
    void prepare (double hostSampleRate, int maxBlock);
    void reset() noexcept;

    /** Pulls `numFrames` at the host rate, rendering from `engine` as needed. */
    void process (maps::MapsEngine& engine, float* left, float* right, int numFrames) noexcept;

    bool isBypassed() const noexcept { return passthrough; }

private:
    void renderMore (maps::MapsEngine& engine, int frames) noexcept;

    static constexpr int kRing = 16384;

    std::vector<float> ringL, ringR, scratchL, scratchR;
    double ratio = 1.0;          // core frames per host frame
    double readPos = 0.0;        // fractional, in ring coordinates
    long long written = 0;       // total core frames ever written
    double consumed = 0.0;       // total core frames ever read
    bool passthrough = true;
};

class MapsProcessor : public juce::AudioProcessor
{
public:
    MapsProcessor();
    ~MapsProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MAPS"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- for the editor
    maps::MapsEngine& getEngine() noexcept { return engine; }

    /** The current modulation offset of a destination, in normalised
        parameter space, for the panel to draw as a second arc. Read from the
        message thread; a torn value costs one stale frame of a knob's ghost
        ring and nothing else. */
    float modulationOffset (int dest) const noexcept
    {
        return (dest > modulation::off && dest < modulation::numDests)
                 ? modOffset[(size_t) dest] : 0.0f;
    }
    /** Which MIDI notes fire which voice, for the panel to print. */
    static int voiceForNote (int midiNote) noexcept;

private:
    void pullParameters() noexcept;

    maps::MapsEngine engine;
    maps::Parameters params;
    CoreResampler resampler;

    void updateModulation (const maps::Transport& t, int numSamples) noexcept;
    /** The destination's value with modulation applied, in the units the core
        wants. Modulation is summed in NORMALISED space and then converted, so
        a route means the same amount of movement whatever the parameter's
        range or skew happens to be. */
    float modulated (int dest) const noexcept;

    maps::Lfo lfos[modulation::kNumLfos];
    std::array<float, modulation::numDests> modOffset { };
    std::array<juce::RangedAudioParameter*, modulation::numDests> destParam { };

    bool lastReset = false;
    double hostRate = 48000.0;
    double freeRunPpq = 0.0;
    int  currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MapsProcessor)
};
