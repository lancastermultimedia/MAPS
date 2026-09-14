// MAPS — the instrument.
//
// Fixed routing, on purpose:
//
//   Grids ──┬─ trigger + accent ─→ plaits::Voice 1 ─┬─ LEVEL ─→ dry mix ─┐
//           ├─ trigger + accent ─→ plaits::Voice 2 ─┤                    ├─→ out
//           └─ trigger + accent ─→ plaits::Voice 3 ─┘                    │
//                                        └─ SEND ─→ clouds ─→ wet ───────┘
//
// The whole point of the instrument is that this is not repatchable. What the
// panel does is make every parameter of it reachable without a menu.

#include "maps/MapsEngine.h"
#include "maps/Lfo.h"
#include "RateConvert.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <new>
#include <vector>

#include "stmlib/utils/buffer_allocator.h"
#include "stmlib/utils/random.h"
#include "plaits/dsp/voice.h"
#include "clouds/dsp/granular_processor.h"
#include "grids/pattern_generator.h"

namespace maps {

// ------------------------------------------------------------------ naming

const char* modelName (int model) noexcept
{
    static const char* kNames[kNumModels] = {
        "GRANULAR CLOUD", "FILTERED NOISE", "PARTICLE NOISE", "INHARM STRING",
        "MODAL RESONATOR", "ANALOG BD", "ANALOG SD", "ANALOG HH"
    };
    return kNames[model < 0 ? 0 : (model >= kNumModels ? kNumModels - 1 : model)];
}

const char* lfoWaveName (int wave) noexcept
{
    static const char* kNames[numLfoWaves] =
        { "SINE", "TRIANGLE", "RAMP UP", "RAMP DOWN", "SQUARE", "RANDOM", "DRIFT" };
    return kNames[wave < 0 ? 0 : (wave >= numLfoWaves ? numLfoWaves - 1 : wave)];
}

const char* cloudsModeName (int mode) noexcept
{
    static const char* kNames[numCloudsModes] =
        { "GRANULAR", "STRETCH", "LOOPING DELAY", "SPECTRAL" };
    return kNames[mode < 0 ? 0 : (mode >= numCloudsModes ? numCloudsModes - 1 : mode)];
}

// --------------------------------------------------------------- constants

namespace {

/** Plaits 1.2 keeps the eight non-pitched engines at the top of the
    registration order. The 1.0 firmware had them at 8-15; anyone reading
    Émilie's old documentation will expect those numbers. */
constexpr int kFirstRedEngine = 16;

/** Grids is clocked at 12 ppqn here, which makes one step a sixteenth note
    and one 32-step pattern two bars — the way Topograph feels in a rack.
    The firmware's own comment says 24 ppqn and eight steps to the quarter;
    that would put the grid on thirty-seconds, which is not what anybody
    means by a Grids pattern. */
constexpr int kTicksPerQuarter = 12;
constexpr int kTicksPerStep    = 3;

/** Swing delays every second sixteenth. The cap is just under one tick, so
    that a swung tick can never be scheduled later than the tick after it. */
constexpr double kMaxSwingQuarters = 0.9 / (double) kTicksPerQuarter;

/** Plaits renders in blocks of twelve and nothing else. This is the first
    thing that bites anyone porting it: libDaisy's default block size is not
    this, and the symptom is intermittent glitching that reads like a CPU
    problem. */
constexpr int kChunk = (int) plaits::kBlockSize;

/** Per voice. The firmware gives Plaits 16 KB; double it and the engines
    that allocate lazily on first selection can never run the allocator dry. */
constexpr size_t kVoiceRam = 32768;

/** Clouds' recording buffer in stereo is sized by the SMALL buffer — Prepare()
    sets buffer_size[0] = buffer_size[1] = buffer_size_[1] and carves the FX
    workspace out of the tail of the large one. Growing only the large buffer
    buys no extra recording time. The module gets about a second per channel;
    there is no reason a plugin should. */
constexpr size_t kCloudsSmall = 65536 * 8;                 // ~16 s per channel
constexpr size_t kCloudsLarge = kCloudsSmall + 65536 * 2;  // + FX workspace

constexpr double kCloudsRate = 32000.0;
constexpr double kDownRatio  = kCloudsRate / kCoreSampleRate;   // 2/3
constexpr double kAntiAlias  = 14000.0;

inline float clamp01 (float x) noexcept { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

} // namespace

// -------------------------------------------------------------------- impl

struct MapsEngine::Impl
{
    // ---- sequencer
    grids::PatternGenerator grids;
    long long nextTick     = 0;
    double    corePhase    = 0.0;    // quarter notes at the next chunk boundary
    double    lastEndPhase = 0.0;
    bool      wasPlaying   = false;
    bool      resetPending = false;

    // ---- voices
    std::vector<uint8_t>       voiceRam;
    stmlib::BufferAllocator    alloc[kNumVoices];
    plaits::Voice              voice[kNumVoices];
    plaits::Patch              patch[kNumVoices];
    plaits::Modulations        mod[kNumVoices];
    plaits::Voice::Frame       frames[kNumVoices][kChunk];
    int                        latchedModel[kNumVoices] = { 0, 0, 0 };
    float                      outBufL[kChunk] = { 0.0f };
    float                      outBufR[kChunk] = { 0.0f };
    int                        outAvail = 0, outRead = 0;
    float                      accentGain[kNumVoices] = { 1.0f, 1.0f, 1.0f };

    // ---- texture
    std::vector<uint8_t>          cloudsLarge, cloudsSmall;
    clouds::GranularProcessor     clouds;
    clouds::ShortFrame            cIn[clouds::kMaxBlockSize];
    clouds::ShortFrame            cOut[clouds::kMaxBlockSize];
    int                           cFill = 0;
    double                        downPhase = 0.0;
    float                         downPrev = 0.0f;
    DcBlock                       dcDry, dcSend;

    /** stmlib::Random is one global generator shared by every noise source in
        Plaits and Clouds. Two plugin instances would draw from the same
        stream, and a render would never repeat — which also means a
        block-boundary bug could hide behind "well, it's noise". Swapping this
        instance's state in and out around each chunk gives every MapsEngine a
        private stream without touching upstream. */
    uint32_t                      noiseState = 0x2545f491u;
    Lowpass6                      preLp;
    Lowpass6                      postLpL, postLpR;
    FractionalFifo<8192>          wet;

    // ---- state
    Parameters p;
    Transport  transport;
    bool       prepared = false;

    std::atomic<float>    peakVoice[kNumVoices] { {0.0f}, {0.0f}, {0.0f} };
    std::atomic<float>    peakOut { 0.0f };
    std::atomic<int>      step { 0 };
    std::atomic<unsigned> stepTriggers { 0 };

    void prepare();
    void reset() noexcept;
    void applyToVoice (int i) noexcept;
    void fireTick() noexcept;
    void renderChunk (float* l, float* r, int n) noexcept;
};

void MapsEngine::Impl::prepare()
{
    // On the hardware these three live in BSS and the startup code zeroes them
    // for free. Here they are members of a heap object, and every one of them
    // has state that Init() and Reset() do not touch — Grids' pulse counter
    // and part perturbations, Plaits' per-engine scratch. Left alone it is
    // whatever the allocator handed back, which is how you get two instances
    // of the same instrument that do not render the same thing. None of these
    // types has a vtable... except plaits::Voice, which holds all twenty-four
    // engines as members and every one of them is polymorphic. Zeroing that
    // wipes the vtable pointers and the first Render() call walks off a null,
    // so it is zeroed and then reconstructed in place, which puts the vptrs
    // back and leaves the rest at zero.
    std::memset (static_cast<void*> (&grids),  0, sizeof (grids));
    std::memset (static_cast<void*> (&clouds), 0, sizeof (clouds));
    for (int i = 0; i < kNumVoices; ++i)
    {
        voice[i].~Voice();
        std::memset (static_cast<void*> (&voice[i]), 0, sizeof (plaits::Voice));
        new (static_cast<void*> (&voice[i])) plaits::Voice();
    }

    voiceRam.assign (kVoiceRam * kNumVoices, 0);
    for (int i = 0; i < kNumVoices; ++i)
    {
        alloc[i].Init (voiceRam.data() + (size_t) i * kVoiceRam, kVoiceRam);
        voice[i].Init (&alloc[i]);
        std::memset (&patch[i], 0, sizeof (plaits::Patch));
        std::memset (&mod[i], 0, sizeof (plaits::Modulations));

        // trigger_patched drives Plaits' internal envelope and LPG, which is
        // what makes a percussion engine a hit rather than a drone. Leaving
        // level unpatched fixes Plaits' own accent at 0.8; MAPS applies its
        // accent as gain instead, so that the LPG still gets triggered.
        mod[i].trigger_patched = true;
        mod[i].level_patched   = false;
    }

    cloudsLarge.assign (kCloudsLarge, 0);
    cloudsSmall.assign (kCloudsSmall, 0);
    clouds.Init (cloudsLarge.data(), kCloudsLarge, cloudsSmall.data(), kCloudsSmall);
    clouds.set_playback_mode (clouds::PLAYBACK_MODE_GRANULAR);
    clouds.set_quality (0);                 // stereo, 16-bit

    preLp.set  (kAntiAlias, kCoreSampleRate);
    postLpL.set (kAntiAlias, kCoreSampleRate);
    postLpR.set (kAntiAlias, kCoreSampleRate);

    grids.Init();
    grids.set_output_mode (grids::OUTPUT_MODE_DRUMS);

    prepared = true;
    reset();
}

void MapsEngine::Impl::reset() noexcept
{
    for (int i = 0; i < kNumVoices; ++i)
    {
        mod[i].trigger  = 0.0f;
        accentGain[i]   = 1.0f;
        latchedModel[i] = p.voice[i].model;
    }
    grids.Reset();
    noiseState   = 0x2545f491u;
    nextTick     = 0;
    corePhase    = 0.0;
    lastEndPhase = 0.0;
    outAvail     = 0;
    outRead      = 0;
    wasPlaying   = false;
    resetPending = false;

    cFill     = 0;
    downPhase = 0.0;
    downPrev  = 0.0f;
    std::memset (cIn, 0, sizeof (cIn));
    std::memset (cOut, 0, sizeof (cOut));
    dcDry.reset(); dcSend.reset();
    preLp.reset(); postLpL.reset(); postLpR.reset();
    wet.reset (64);
}

void MapsEngine::Impl::applyToVoice (int i) noexcept
{
    const VoiceParams& v = p.voice[i];

    // MODEL is the one parameter that is NOT applied continuously. Plaits
    // resets the engine and the post-processor when the index changes, so
    // swapping it under a ringing tail is a click; and a MODEL knob being
    // swept by an LFO would otherwise change engine several times inside one
    // hit, which sounds like a fault rather than a part. The value is latched
    // when the voice fires, so a beat-synced LFO on MODEL gives exactly what
    // it should: a different engine per hit, changing cleanly on the attack.
    patch[i].engine     = (float) (kFirstRedEngine + std::min (std::max (latchedModel[i], 0), kNumModels - 1));
    patch[i].note       = v.frequency;
    patch[i].harmonics  = clamp01 (v.harmonics);
    patch[i].timbre     = clamp01 (v.timbre);
    patch[i].morph      = clamp01 (v.morph);
    patch[i].decay      = clamp01 (v.decay);
    patch[i].lpg_colour = 0.4f;
    mod[i].note         = 0.0f;
}

// One 12 ppqn tick: ask Grids what fires, and fire it.
void MapsEngine::Impl::fireTick() noexcept
{
    auto* gs = grids.mutable_settings();
    const float fill = p.grids.fill ? 0.30f : 0.0f;

    gs->options.drums.x          = (uint8_t) (clamp01 (p.grids.mapX)  * 255.0f);
    gs->options.drums.y          = (uint8_t) (clamp01 (p.grids.mapY)  * 255.0f);
    gs->options.drums.randomness = (uint8_t) (clamp01 (p.grids.chaos) * 255.0f);
    for (int i = 0; i < kNumVoices; ++i)
        gs->density[i] = (uint8_t) (clamp01 (p.grids.density[i] + fill) * 255.0f);

    if (resetPending)
    {
        grids.Reset();
        resetPending = false;
    }

    grids.TickClock (1);
    const uint8_t state = grids.state();

    unsigned fired = 0;
    for (int i = 0; i < kNumVoices; ++i)
    {
        if ((state & (1u << i)) == 0)
            continue;

        // Bits 3..5 are Grids' accent outputs for the three parts. Plaits'
        // own accent is fixed at 0.8 while level is unpatched, so the accent
        // lands as output gain instead - musically the same thing on a drum,
        // and it keeps the internal envelope working.
        const bool accented = (state & (1u << (3 + i))) != 0;
        accentGain[i]  = accented ? 1.0f : 0.62f;
        latchedModel[i] = p.voice[i].model;
        mod[i].trigger = 1.0f;
        fired |= (1u << i);
    }

    step.store (grids.step(), std::memory_order_relaxed);
    if (fired != 0)
        stepTriggers.store (fired, std::memory_order_relaxed);

    grids.ClockFallingEdge();
}

void MapsEngine::Impl::renderChunk (float* outL, float* outR, int n) noexcept
{
    const uint32_t callerNoise = stmlib::Random::state();
    stmlib::Random::Seed (noiseState);

    for (int i = 0; i < kNumVoices; ++i)
    {
        applyToVoice (i);
        voice[i].Render (patch[i], mod[i], frames[i], (size_t) n);
        mod[i].trigger = 0.0f;          // Plaits latches the rising edge
    }

    // Clouds' parameters are cheap to set and are read per block inside
    // Process(), so there is no smoothing to do here.
    auto* cp = clouds.mutable_parameters();
    cp->position      = clamp01 (p.clouds.position);
    cp->size          = clamp01 (p.clouds.size);
    cp->density       = clamp01 (p.clouds.density);
    cp->texture       = clamp01 (p.clouds.texture);
    cp->pitch         = p.clouds.pitch;
    cp->dry_wet       = clamp01 (p.clouds.blend);
    cp->stereo_spread = clamp01 (p.clouds.spread);
    cp->feedback      = clamp01 (p.clouds.feedback);
    cp->reverb        = clamp01 (p.clouds.space);
    cp->freeze        = p.clouds.freeze;
    clouds.set_playback_mode ((clouds::PlaybackMode)
        std::min (std::max (p.clouds.mode, 0), (int) numCloudsModes - 1));

    float vPeak[kNumVoices] = { 0.0f, 0.0f, 0.0f };
    float oPeak = 0.0f;
    const float master = clamp01 (p.output);

    for (int s = 0; s < n; ++s)
    {
        float dry = 0.0f, send = 0.0f;

        for (int i = 0; i < kNumVoices; ++i)
        {
            if (p.voice[i].mute)
                continue;

            const float x = (float) frames[i][s].out * (1.0f / 32768.0f) * accentGain[i];
            const float a = std::fabs (x);
            if (a > vPeak[i]) vPeak[i] = a;

            dry  += x * clamp01 (p.voice[i].level);
            send += x * clamp01 (p.voice[i].send);
        }

        dry  = dcDry.process  (dry  * 0.5f);
        send = dcSend.process (send * 0.5f);

        // ---- down to 32 kHz, through Clouds, back up to 48 kHz -----------
        const float filtered = preLp.process (send);

        downPhase += kDownRatio;
        if (downPhase >= 1.0)
        {
            downPhase -= 1.0;
            const float m = 0.5f * (filtered + downPrev);
            const short q = (short) (std::max (-1.0f, std::min (1.0f, m)) * 32767.0f);
            cIn[cFill].l = q;
            cIn[cFill].r = q;

            if (++cFill == (int) clouds::kMaxBlockSize)
            {
                clouds.Prepare();
                clouds.Process (cIn, cOut, clouds::kMaxBlockSize);
                for (size_t k = 0; k < clouds::kMaxBlockSize; ++k)
                    wet.push ((float) cOut[k].l * (1.0f / 32768.0f),
                              (float) cOut[k].r * (1.0f / 32768.0f));
                cFill = 0;
            }
        }
        downPrev = filtered;

        float wl = 0.0f, wr = 0.0f;
        wet.advance (kDownRatio, wl, wr);
        wl = postLpL.process (wl);
        wr = postLpR.process (wr);

        // The send bus returns at unity-ish; 1.8 puts a fully-open SEND in
        // the same territory as the dry hits rather than underneath them.
        float l = (dry + wl * 1.8f) * master;
        float r = (dry + wr * 1.8f) * master;

        // Soft clip. Clouds' feedback path can be pushed a long way and a
        // hard edge there sounds like a fault rather than a choice.
        l = std::tanh (l * 1.1f);
        r = std::tanh (r * 1.1f);

        const float ap = std::max (std::fabs (l), std::fabs (r));
        if (ap > oPeak) oPeak = ap;

        outL[s] = l;
        outR[s] = r;
    }

    for (int i = 0; i < kNumVoices; ++i)
    {
        const float prev = peakVoice[i].load (std::memory_order_relaxed);
        peakVoice[i].store (std::max (prev, vPeak[i]), std::memory_order_relaxed);
    }
    peakOut.store (std::max (peakOut.load (std::memory_order_relaxed), oPeak),
                   std::memory_order_relaxed);

    noiseState = stmlib::Random::state();
    stmlib::Random::Seed (callerNoise);
}

// ------------------------------------------------------------------- shell

MapsEngine::MapsEngine() : impl (new Impl) {}
MapsEngine::~MapsEngine() = default;

void MapsEngine::prepare()                              { impl->prepare(); }
void MapsEngine::reset() noexcept                       { impl->reset(); }
void MapsEngine::setParameters (const Parameters& p) noexcept { impl->p = p; }
void MapsEngine::setTransport (const Transport& t) noexcept   { impl->transport = t; }
void MapsEngine::resetPattern() noexcept                { impl->resetPending = true; }

void MapsEngine::trigger (int voiceIndex, float accent) noexcept
{
    if (voiceIndex < 0 || voiceIndex >= kNumVoices)
        return;
    impl->accentGain[voiceIndex]  = 0.35f + 0.65f * clamp01 (accent);
    impl->latchedModel[voiceIndex] = impl->p.voice[voiceIndex].model;
    impl->mod[voiceIndex].trigger  = 1.0f;
}

float MapsEngine::voicePeak (int index) const noexcept
{
    if (index < 0 || index >= kNumVoices) return 0.0f;
    return impl->peakVoice[index].exchange (0.0f, std::memory_order_relaxed);
}

float MapsEngine::outputPeak() const noexcept
{
    return impl->peakOut.exchange (0.0f, std::memory_order_relaxed);
}

int MapsEngine::currentStep() const noexcept
{
    return impl->step.load (std::memory_order_relaxed);
}

unsigned MapsEngine::lastStepTriggers() const noexcept
{
    return impl->stepTriggers.load (std::memory_order_relaxed);
}

void MapsEngine::process (float* left, float* right, int numFrames) noexcept
{
    Impl& d = *impl;
    if (! d.prepared || numFrames <= 0)
    {
        if (left)  std::memset (left,  0, sizeof (float) * (size_t) std::max (0, numFrames));
        if (right) std::memset (right, 0, sizeof (float) * (size_t) std::max (0, numFrames));
        return;
    }

    // ---- where the block sits in musical time ---------------------------
    const double bpm = d.transport.playing
                         ? std::max (1.0, d.transport.bpm)
                         : std::max (1.0, (double) d.p.grids.tempo);
    const double quartersPerSample = bpm / 60.0 / kCoreSampleRate;

    if (d.transport.playing)
    {
        const double startPhase = d.transport.ppqPosition;

        // A locate, a loop wrap, or the transport starting: re-aim the tick
        // counter instead of catching up one tick at a time.
        if (! d.wasPlaying || std::fabs (startPhase - d.lastEndPhase) > 0.05)
            d.nextTick = (long long) std::ceil (startPhase * kTicksPerQuarter) - 1;

        // Re-anchor to the host every block. The core renders in twelves and
        // the host does not, so this can pull back by up to eleven samples -
        // a quarter of a millisecond, and it is corrected again next block.
        d.corePhase = startPhase;
        d.lastEndPhase = startPhase + quartersPerSample * numFrames;
    }
    else
    {
        if (d.wasPlaying)                      // transport just stopped
        {
            d.corePhase = 0.0;
            d.nextTick  = 0;
        }
        d.lastEndPhase = d.corePhase + quartersPerSample * numFrames;
    }
    d.wasPlaying = d.transport.playing;

    // Scheduled position of a tick, with swing applied to every second
    // sixteenth. Grouped by step so all three ticks of a swung step move
    // together, which is what makes it feel late rather than smeared.
    auto tickPhase = [&d] (long long n) noexcept
    {
        const long long stepIndex = (n >= 0 ? n : n - (kTicksPerStep - 1)) / kTicksPerStep;
        const double base = (double) n / kTicksPerQuarter;
        const bool offbeat = ((stepIndex % 2) + 2) % 2 == 1;
        return offbeat ? base + clamp01 (d.p.grids.swing) * kMaxSwingQuarters : base;
    };

    // Plaits renders twelve samples per call and its envelopes advance once
    // per call, so calling it with anything else stretches or shrinks every
    // decay in the instrument. Hosts hand out 64, 128, 512 - never 12. So
    // the core always renders whole chunks into its own buffer and the host
    // is served out of that, with the remainder carried to the next block.
    // Without this the render depends on the buffer size, which is the
    // classic Mutable-porting bug: it presents as intermittent glitching and
    // reads like a CPU problem.
    int done = 0;
    while (done < numFrames)
    {
        if (d.outAvail == 0)
        {
            const double chunkEnd = d.corePhase + quartersPerSample * kChunk;

            if (d.p.grids.run)
            {
                int guard = 0;
                while (tickPhase (d.nextTick) < chunkEnd && ++guard < 64)
                {
                    d.fireTick();
                    ++d.nextTick;
                }
            }

            d.renderChunk (d.outBufL, d.outBufR, kChunk);
            d.corePhase = chunkEnd;
            d.outAvail  = kChunk;
            d.outRead   = 0;
        }

        const int n = std::min (d.outAvail, numFrames - done);
        std::memcpy (left  + done, d.outBufL + d.outRead, sizeof (float) * (size_t) n);
        std::memcpy (right + done, d.outBufR + d.outRead, sizeof (float) * (size_t) n);
        d.outRead  += n;
        d.outAvail -= n;
        done       += n;
    }
}

} // namespace maps
