#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

#include <cmath>

// ============================================================== CoreResampler

void CoreResampler::prepare (double hostSampleRate, int maxBlock)
{
    ratio       = maps::kCoreSampleRate / juce::jmax (8000.0, hostSampleRate);
    passthrough = std::abs (hostSampleRate - maps::kCoreSampleRate) < 0.5;

    ringL.assign (kRing, 0.0f);
    ringR.assign (kRing, 0.0f);

    const int scratch = juce::jmax (512, (int) std::ceil (maxBlock * ratio) + 64);
    scratchL.assign ((size_t) scratch, 0.0f);
    scratchR.assign ((size_t) scratch, 0.0f);

    reset();
}

void CoreResampler::reset() noexcept
{
    std::fill (ringL.begin(), ringL.end(), 0.0f);
    std::fill (ringR.begin(), ringR.end(), 0.0f);
    written  = 4;          // two points of history for the interpolator
    consumed = 0.0;
    readPos  = 0.0;
}

void CoreResampler::renderMore (maps::MapsEngine& engine, int frames) noexcept
{
    while (frames > 0)
    {
        const int n = juce::jmin (frames, (int) scratchL.size());
        engine.process (scratchL.data(), scratchR.data(), n);

        for (int i = 0; i < n; ++i)
        {
            const int w = (int) (written % kRing);
            ringL[(size_t) w] = scratchL[(size_t) i];
            ringR[(size_t) w] = scratchR[(size_t) i];
            ++written;
        }
        frames -= n;
    }
}

void CoreResampler::process (maps::MapsEngine& engine, float* left, float* right, int numFrames) noexcept
{
    if (passthrough)
    {
        engine.process (left, right, numFrames);
        return;
    }

    // Enough core frames to cover this block plus two ahead for the kernel.
    const double needEnd = consumed + ratio * numFrames + 3.0;
    const int    deficit = (int) std::ceil (needEnd) - (int) written;
    if (deficit > 0)
        renderMore (engine, deficit);

    auto at = [this] (long long n) noexcept
    {
        const long long m = n % kRing;
        return (size_t) (m < 0 ? m + kRing : m);
    };

    for (int i = 0; i < numFrames; ++i)
    {
        const long long i0 = (long long) consumed;
        const float t = (float) (consumed - (double) i0);

        const size_t a = at (i0 - 1), b = at (i0), c = at (i0 + 1), d = at (i0 + 2);
        left[i]  = maps_plugin_catmull (ringL[a], ringL[b], ringL[c], ringL[d], t);
        right[i] = maps_plugin_catmull (ringR[a], ringR[b], ringR[c], ringR[d], t);

        consumed += ratio;
    }
}

// ================================================================ parameters

namespace {

juce::AudioParameterFloatAttributes pct()
{
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; });
}

std::unique_ptr<juce::AudioParameterFloat> knob (const juce::String& id, const juce::String& name,
                                                 float def)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> { 0.0f, 1.0f }, def, pct());
}

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout MapsProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray modelNames;
    for (int m = 0; m < maps::kNumModels; ++m)
        modelNames.add (juce::String (maps::modelName (m)));

    const maps::Parameters d;

    for (int v = 1; v <= maps::kNumVoices; ++v)
    {
        const auto& dv = d.voice[v - 1];
        const juce::String tag = "  " + juce::String (v);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ids::freq (v), 1 }, "FREQUENCY" + tag,
            juce::NormalisableRange<float> { 12.0f, 96.0f, 0.01f }, 36.0f + (v - 1) * 12.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v2, int) { return juce::MidiMessage::getMidiNoteName ((int) std::round (v2), true, true, 3); })));

        layout.add (knob (ids::harmonics (v), "HARMONICS" + tag, dv.harmonics));
        layout.add (knob (ids::timbre (v),    "TIMBRE"    + tag, dv.timbre));
        layout.add (knob (ids::morph (v),     "MORPH"     + tag, dv.morph));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ids::model (v), 1 }, "MODEL" + tag, modelNames,
            v == 1 ? maps::modelAnalogBassDrum
                   : (v == 2 ? maps::modelAnalogSnareDrum : maps::modelAnalogHiHat)));

        layout.add (knob (ids::decay (v), "DECAY" + tag, dv.decay));
        layout.add (knob (ids::level (v), "LEVEL" + tag, v == 1 ? 1.0f : (v == 2 ? 0.85f : 0.7f)));
        layout.add (knob (ids::send (v),  "SEND"  + tag, v == 1 ? 0.15f : (v == 2 ? 0.35f : 0.6f)));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ids::mute (v), 1 }, "MUTE" + tag, false));
    }

    layout.add (knob (ids::mapX,  "MAP X", d.grids.mapX));
    layout.add (knob (ids::mapY,  "MAP Y", d.grids.mapY));
    layout.add (knob (ids::chaos, "CHAOS", d.grids.chaos));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::clock, 1 }, "CLOCK",
        juce::NormalisableRange<float> { 40.0f, 240.0f, 0.1f }, 110.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (v, 1) + " BPM"; })));

    layout.add (knob (ids::swing, "SWING", d.grids.swing));
    for (int i = 0; i < maps::kNumVoices; ++i)
        layout.add (knob (ids::density (i), "DENSITY " + juce::String (i + 1), d.grids.density[i]));

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ids::run, 1 },   "RUN",   true));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ids::reset, 1 }, "RESET", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ids::fill, 1 },  "FILL",  false));

    layout.add (knob (ids::position,     "POSITION", d.clouds.position));
    layout.add (knob (ids::grainSize,    "SIZE",     d.clouds.size));
    layout.add (knob (ids::grainDensity, "DENSITY",  0.62f));
    layout.add (knob (ids::texture,      "TEXTURE",  d.clouds.texture));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::pitch, 1 }, "PITCH",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (v, 1) + " st"; })));

    layout.add (knob (ids::feedback, "FEEDBACK", d.clouds.feedback));
    layout.add (knob (ids::space,    "SPACE",    d.clouds.space));
    layout.add (knob (ids::blend,    "BLEND",    d.clouds.blend));
    layout.add (knob (ids::spread,   "SPREAD",   d.clouds.spread));

    juce::StringArray modeNames;
    for (int m = 0; m < maps::numCloudsModes; ++m)
        modeNames.add (juce::String (maps::cloudsModeName (m)));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::cloudsMode, 1 }, "MODE", modeNames, maps::cloudsGranular));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ids::freeze, 1 }, "FREEZE", false));

    layout.add (knob (ids::output, "OUTPUT", d.output));

    // ---- 266, the modulation sources ------------------------------------
    juce::StringArray waveNames;
    for (int w = 0; w < maps::numLfoWaves; ++w)
        waveNames.add (juce::String (maps::lfoWaveName (w)));

    juce::StringArray destNames;
    for (int i = 0; i < modulation::numDests; ++i)
        destNames.add (juce::String (modulation::destName (i)));

    for (int n = 1; n <= modulation::kNumLfos; ++n)
    {
        const juce::String tag = "  " + juce::String (n);

        // One knob, two meanings, because the panel cannot afford two. The
        // caption under it prints whichever one is live.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ids::lfoRate (n), 1 }, "RATE" + tag,
            juce::NormalisableRange<float> { 0.0f, 1.0f }, n == 1 ? 0.50f : (n == 2 ? 0.68f : 0.34f),
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return juce::String (maps::lfoRateForKnob (v), 2) + " Hz"; })));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ids::lfoWave (n), 1 }, "WAVE" + tag, waveNames,
            n == 1 ? maps::lfoRandom : (n == 2 ? maps::lfoTriangle : maps::lfoDrift)));

        // Synced by default: this is a drum machine, and the whole point of
        // the section is things happening on the beat.
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ids::lfoSync (n), 1 }, "SYNC" + tag, true));

        for (int r = 0; r < modulation::kRoutesPerLfo; ++r)
        {
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { ids::lfoDest (n, r), 1 },
                juce::String (r == 0 ? "DEST A" : "DEST B") + tag, destNames, modulation::off));

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { ids::lfoDepth (n, r), 1 },
                juce::String (r == 0 ? "DEPTH A" : "DEPTH B") + tag,
                juce::NormalisableRange<float> { -1.0f, 1.0f }, 0.0f,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                    [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; })));
        }
    }

    return layout;
}

// ================================================================= processor

MapsProcessor::MapsProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MAPS", createLayout())
{
    for (int d = 0; d < modulation::numDests; ++d)
    {
        const auto id = modulation::destParamID (d);
        destParam[(size_t) d] = id.isEmpty() ? nullptr
                                             : dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id));
        jassert (d == modulation::off || destParam[(size_t) d] != nullptr);
    }
}

MapsProcessor::~MapsProcessor() = default;

bool MapsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void MapsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare();
    resampler.prepare (sampleRate, samplesPerBlock);
    hostRate = sampleRate;
    for (auto& l : lfos)
        l.reset();
    modOffset.fill (0.0f);
}

int MapsProcessor::voiceForNote (int midiNote) noexcept
{
    // General MIDI drum notes, so an existing pattern in the timeline plays
    // something sensible without remapping: kick, snare, closed hat.
    switch (midiNote)
    {
        case 35: case 36: return 0;
        case 38: case 40: return 1;
        case 42: case 44: case 46: return 2;
        default: break;
    }
    // Anything else: three notes up from C1 in order, so a keyboard works too.
    const int n = midiNote % 12;
    if (n == 0) return 0;
    if (n == 2) return 1;
    if (n == 4) return 2;
    return -1;
}

void MapsProcessor::updateModulation (const maps::Transport& t, int numSamples) noexcept
{
    modOffset.fill (0.0f);

    const double seconds = (double) numSamples / juce::jmax (1.0, hostRate);
    auto raw = [this] (const juce::String& id) { return apvts.getRawParameterValue (id)->load(); };

    for (int n = 1; n <= modulation::kNumLfos; ++n)
    {
        auto& lfo = lfos[n - 1];
        const float rateKnob = raw (ids::lfoRate (n));
        const int   wave     = (int) raw (ids::lfoWave (n));
        const bool  synced   = raw (ids::lfoSync (n)) > 0.5f;

        if (synced)
        {
            // Phase comes from the host's position, not from an accumulator,
            // so the LFO is on the grid rather than near it, is right again
            // immediately after a locate, and renders the same every pass.
            const float beats = maps::lfoDivisions()[maps::lfoDivisionForKnob (rateKnob)].beats;
            const double ppq = t.playing ? t.ppqPosition : freeRunPpq;
            lfo.updateSynced (ppq, beats, wave);
        }
        else
        {
            lfo.updateFree (maps::lfoRateForKnob (rateKnob), seconds, wave);
        }

        const float v = lfo.value();

        for (int r = 0; r < modulation::kRoutesPerLfo; ++r)
        {
            const int dest = (int) raw (ids::lfoDest (n, r));
            if (dest <= modulation::off || dest >= modulation::numDests)
                continue;

            // Half-range, which is the attenuverter convention: a route at
            // full depth swings half the parameter's range each way, so a
            // centred knob reaches both ends and an off-centre one does not
            // simply pin itself against a limit.
            modOffset[(size_t) dest] += raw (ids::lfoDepth (n, r)) * v * 0.5f;
        }
    }

    // When the transport is stopped the synced LFOs still need somewhere to
    // live, or the modulation section goes dead the moment you stop playing
    // and start turning knobs. They free-run on the CLOCK knob's tempo, the
    // same clock the sequencer uses.
    if (! t.playing)
        freeRunPpq += seconds * (double) juce::jmax (1.0f, raw (ids::clock)) / 60.0;
    else
        freeRunPpq = t.ppqPosition;
}

float MapsProcessor::modulated (int dest) const noexcept
{
    auto* p = destParam[(size_t) dest];
    if (p == nullptr)
        return 0.0f;

    const float base = p->getValue();                       // normalised
    const float off  = modOffset[(size_t) dest];
    return p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, base + off));
}

void MapsProcessor::pullParameters() noexcept
{
    using namespace modulation;
    auto get = [this] (const juce::String& id) { return apvts.getRawParameterValue (id)->load(); };

    for (int v = 0; v < maps::kNumVoices; ++v)
    {
        auto& p = params.voice[v];
        const int n = v + 1;
        p.frequency = modulated (voiceDest (n, fFreq));
        p.harmonics = modulated (voiceDest (n, fHarmonics));
        p.timbre    = modulated (voiceDest (n, fTimbre));
        p.morph     = modulated (voiceDest (n, fMorph));
        p.model     = (int) modulated (voiceDest (n, fModel));
        p.decay     = modulated (voiceDest (n, fDecay));
        p.level     = modulated (voiceDest (n, fLevel));
        p.send      = modulated (voiceDest (n, fSend));
        p.mute      = get (ids::mute (n)) > 0.5f;
    }

    params.grids.mapX  = modulated (mapX);
    params.grids.mapY  = modulated (mapY);
    params.grids.chaos = modulated (chaos);
    params.grids.tempo = modulated (gridsClock);
    params.grids.swing = modulated (swing);
    for (int i = 0; i < maps::kNumVoices; ++i)
        params.grids.density[i] = modulated (densityDest (i));
    params.grids.run  = get (ids::run) > 0.5f;
    params.grids.fill = get (ids::fill) > 0.5f;

    // RESET is a momentary: it fires on the press, not for as long as it is
    // held, or a held button would restart the pattern on every tick.
    const bool nowReset = get (ids::reset) > 0.5f;
    if (nowReset && ! lastReset)
        engine.resetPattern();
    lastReset = nowReset;

    params.clouds.position = modulated (cPosition);
    params.clouds.size     = modulated (cSize);
    params.clouds.density  = maps_plugin::mapCloudsDensity (modulated (cDensity));
    params.clouds.texture  = modulated (cTexture);
    params.clouds.pitch    = modulated (cPitch);
    params.clouds.feedback = modulated (cFeedback);
    params.clouds.space    = modulated (cSpace);
    params.clouds.blend    = modulated (cBlend);
    params.clouds.spread   = modulated (cSpread);
    params.clouds.mode     = (int) get (ids::cloudsMode);
    params.clouds.freeze   = get (ids::freeze) > 0.5f;

    params.output = modulated (output);

    engine.setParameters (params);
}

void MapsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    maps::Transport t;
    if (auto* head = getPlayHead())
    {
        if (auto pos = head->getPosition())
        {
            t.playing = pos->getIsPlaying();
            t.bpm     = pos->getBpm().orFallback (120.0);
            t.ppqPosition = pos->getPpqPosition().orFallback (0.0);
        }
    }
    engine.setTransport (t);

    // Order matters: the LFOs are evaluated against this block's musical
    // position, and then the parameters are read through them.
    updateModulation (t, buffer.getNumSamples());
    pullParameters();

    // MIDI plays the voices by hand, alongside whatever Grids is doing.
    // Sample-accurate placement would mean splitting the block; at 0.25 ms of
    // core granularity there is nothing to gain, so notes land at block start.
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! m.isNoteOn())
            continue;
        const int v = voiceForNote (m.getNoteNumber());
        if (v >= 0)
            engine.trigger (v, m.getFloatVelocity());
    }

    const int n = buffer.getNumSamples();
    buffer.clear();

    if (buffer.getNumChannels() >= 2)
    {
        resampler.process (engine, buffer.getWritePointer (0), buffer.getWritePointer (1), n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        // Mono host: render both and fold, rather than throwing away the side
        // of a stereo granular processor.
        juce::AudioBuffer<float> tmp (2, n);
        resampler.process (engine, tmp.getWritePointer (0), tmp.getWritePointer (1), n);
        auto* out = buffer.getWritePointer (0);
        const auto* l = tmp.getReadPointer (0);
        const auto* r = tmp.getReadPointer (1);
        for (int i = 0; i < n; ++i)
            out[i] = 0.5f * (l[i] + r[i]);
    }
}

// ==================================================================== state

int MapsProcessor::getNumPrograms()               { return presets::count(); }
const juce::String MapsProcessor::getProgramName (int index) { return presets::name (index); }

void MapsProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= presets::count())
        return;
    currentProgram = index;
    presets::apply (apvts, index);
}

void MapsProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void MapsProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (apvts.state.getType()))
        {
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* MapsProcessor::createEditor() { return new MapsEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MapsProcessor(); }
