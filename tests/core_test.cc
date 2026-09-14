// maps-core tests. No JUCE, no audio hardware, about a second to run.
//
// These test the layer the bugs actually live in. Parameter-plumbing bugs —
// a knob wired to nothing, a value never read — are invisible here by
// construction, because this file sets the struct directly. That is what
// tools/core_render and the plugin's own render harness are for.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "maps/MapsEngine.h"
#include "maps/Lfo.h"

namespace {

int failures = 0;
std::string current;

void check (bool ok, const std::string& what)
{
    if (! ok) { std::printf ("  FAIL  %s: %s\n", current.c_str(), what.c_str()); ++failures; }
}

struct Render
{
    std::vector<float> l, r;
    float peak = 0.0f, rms = 0.0f;
    bool  finite = true;
};

Render run (maps::MapsEngine& e, double seconds, int block = 96)
{
    Render out;
    const int total = (int) (seconds * maps::kCoreSampleRate);
    std::vector<float> bl ((size_t) block), br ((size_t) block);
    out.l.reserve ((size_t) total); out.r.reserve ((size_t) total);

    double sq = 0.0;
    for (int done = 0; done < total; done += block)
    {
        const int n = std::min (block, total - done);
        e.process (bl.data(), br.data(), n);
        for (int i = 0; i < n; ++i)
        {
            const float a = bl[(size_t) i];
            if (! std::isfinite (a) || ! std::isfinite (br[(size_t) i])) out.finite = false;
            out.peak = std::max (out.peak, std::fabs (a));
            sq += (double) a * a;
            out.l.push_back (a);
            out.r.push_back (br[(size_t) i]);
        }
    }
    out.rms = (float) std::sqrt (sq / std::max<size_t> (1, out.l.size()));
    return out;
}

maps::Parameters drumKit()
{
    maps::Parameters p;
    p.voice[0] = { 36.0f, 0.30f, 0.45f, 0.55f, 0.35f, maps::modelAnalogBassDrum,  1.00f, 0.0f, false };
    p.voice[1] = { 48.0f, 0.45f, 0.55f, 0.50f, 0.30f, maps::modelAnalogSnareDrum, 0.80f, 0.0f, false };
    p.voice[2] = { 72.0f, 0.60f, 0.70f, 0.35f, 0.22f, maps::modelAnalogHiHat,     0.70f, 0.0f, false };
    p.clouds.blend = 0.0f;                 // dry only, unless a test says otherwise
    p.grids.tempo = 120.0f;
    return p;
}

float db (float x) { return 20.0f * std::log10 (std::max (1.0e-9f, x)); }

// ---------------------------------------------------------------- the tests

void test_silent_when_stopped()
{
    current = "silent_when_stopped";
    maps::MapsEngine e; e.prepare();
    auto p = drumKit();
    p.grids.run = false;
    e.setParameters (p);
    run (e, 0.5);                                   // let the DC blocker settle
    auto out = run (e, 2.0);
    check (out.peak < 1.0e-6f, "a stopped sequencer with no triggers must be silent");
}

void test_sequencer_runs()
{
    current = "sequencer_runs";
    maps::MapsEngine e; e.prepare();
    e.setParameters (drumKit());
    auto out = run (e, 4.0);
    check (out.finite, "output stayed finite");
    check (out.peak > 0.05f, "the sequencer produced audio");
}

void test_mute_removes_a_voice()
{
    current = "mute_removes_a_voice";
    auto p = drumKit();
    p.grids.density[1] = p.grids.density[2] = 0.0f;   // voice 1 only

    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    const float loud = run (a, 3.0).rms;

    p.voice[0].mute = true;
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    const float quiet = run (b, 3.0).rms;

    check (loud > 1.0e-3f, "the unmuted voice was audible");
    check (quiet < 1.0e-5f, "MUTE silenced it");
}

void test_level_is_linear()
{
    current = "level_is_linear";
    auto p = drumKit();
    p.grids.density[1] = p.grids.density[2] = 0.0f;
    p.grids.chaos = 0.0f;

    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    const float full = run (a, 4.0).rms;

    p.voice[0].level = 0.5f;
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    const float half = run (b, 4.0).rms;

    const float delta = db (full) - db (half);
    check (std::fabs (delta - 6.02f) < 0.6f,
           "halving LEVEL cost " + std::to_string (delta) + " dB, expected about 6");
}

void test_density_changes_hit_count()
{
    current = "density_changes_hit_count";
    auto p = drumKit();
    p.grids.chaos = 0.0f;
    p.grids.density[1] = p.grids.density[2] = 0.0f;

    p.grids.density[0] = 0.05f;
    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    const float sparse = run (a, 6.0).rms;

    p.grids.density[0] = 0.95f;
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    const float dense = run (b, 6.0).rms;

    check (dense > sparse * 1.5f,
           "DENSITY at 0.95 was not busier than at 0.05 (" +
           std::to_string (dense) + " vs " + std::to_string (sparse) + ")");
}

void test_tempo_changes_hit_count()
{
    current = "tempo_changes_hit_count";
    auto p = drumKit();
    p.grids.chaos = 0.0f;

    p.grids.tempo = 60.0f;
    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    const float slow = run (a, 6.0).rms;

    p.grids.tempo = 180.0f;
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    const float fast = run (b, 6.0).rms;

    check (fast > slow * 1.3f, "tripling the tempo did not make it busier");
}

void test_host_transport_drives_the_clock()
{
    current = "host_transport_drives_the_clock";
    auto p = drumKit();
    p.grids.tempo = 40.0f;               // deliberately wrong; the host wins

    maps::MapsEngine e; e.prepare(); e.setParameters (p);

    const int block = 128;
    const double bpm = 150.0;
    const double qPerSample = bpm / 60.0 / maps::kCoreSampleRate;
    std::vector<float> bl (block), br (block);

    maps::Transport t; t.playing = true; t.bpm = bpm; t.ppqPosition = 0.0;
    double sq = 0.0; int frames = 0;
    for (int i = 0; i < 6 * (int) maps::kCoreSampleRate / block; ++i)
    {
        e.setTransport (t);
        e.process (bl.data(), br.data(), block);
        for (int s = 0; s < block; ++s) { sq += (double) bl[(size_t) s] * bl[(size_t) s]; ++frames; }
        t.ppqPosition += qPerSample * block;
    }
    const float hosted = (float) std::sqrt (sq / frames);

    maps::MapsEngine f; f.prepare(); f.setParameters (p);       // free-running at 40
    const float freeRun = run (f, 6.0).rms;

    check (hosted > freeRun * 1.5f,
           "the host's 150 bpm should be busier than the knob's 40");
}

void test_transport_locate_does_not_stall()
{
    current = "transport_locate_does_not_stall";
    maps::MapsEngine e; e.prepare(); e.setParameters (drumKit());

    const int block = 256;
    std::vector<float> bl (block), br (block);
    maps::Transport t; t.playing = true; t.bpm = 120.0;

    // Jump the playhead forward by a minute, the way a locate does. The tick
    // counter must re-aim rather than grind through a thousand ticks.
    double sq = 0.0;
    for (int pass = 0; pass < 2; ++pass)
    {
        t.ppqPosition = pass == 0 ? 0.0 : 240.0;
        for (int i = 0; i < 200; ++i)
        {
            e.setTransport (t);
            e.process (bl.data(), br.data(), block);
            for (int s = 0; s < block; ++s) sq += (double) bl[(size_t) s] * bl[(size_t) s];
            t.ppqPosition += 120.0 / 60.0 / maps::kCoreSampleRate * block;
        }
    }
    check (std::isfinite (sq) && sq > 0.0, "audio continued across a locate");
}

void test_clouds_density_dead_zone()
{
    current = "clouds_density_dead_zone";
    // Upstream behaviour, pinned here so nobody 'fixes' the taper without
    // noticing what it is for: in granular mode Clouds sets grain overlap to
    // exactly zero between 0.47 and 0.53, and goes silent.
    auto p = drumKit();
    p.voice[0].level = p.voice[1].level = p.voice[2].level = 0.0f;   // wet only
    p.voice[0].send  = p.voice[1].send  = p.voice[2].send  = 0.9f;
    p.clouds.blend = 1.0f;
    p.clouds.space = 0.0f;
    p.clouds.feedback = 0.0f;

    p.clouds.density = 0.50f;
    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    const float dead = run (a, 4.0).rms;

    p.clouds.density = 0.80f;
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    const float alive = run (b, 4.0).rms;

    check (alive > dead * 8.0f,
           "DENSITY 0.5 should be the silent dead zone and 0.8 should not be "
           "(" + std::to_string (dead) + " vs " + std::to_string (alive) + ")");
}

void test_send_reaches_clouds()
{
    current = "send_reaches_clouds";
    auto p = drumKit();
    p.voice[0].level = p.voice[1].level = p.voice[2].level = 0.0f;
    p.clouds.blend = 1.0f;
    p.clouds.density = 0.8f;

    p.voice[0].send = p.voice[1].send = p.voice[2].send = 0.0f;
    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    const float none = run (a, 3.0).rms;

    p.voice[0].send = p.voice[1].send = p.voice[2].send = 0.9f;
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    const float open = run (b, 3.0).rms;

    check (none < 1.0e-5f, "SEND at zero puts nothing into Clouds");
    check (open > 1.0e-3f, "SEND open produces a wet return");
}

void test_freeze_holds_after_the_music_stops()
{
    current = "freeze_holds_after_the_music_stops";
    auto p = drumKit();
    p.voice[0].send = p.voice[1].send = p.voice[2].send = 0.9f;
    p.clouds.blend = 1.0f;
    p.clouds.density = 0.8f;
    p.clouds.space = 0.2f;

    maps::MapsEngine e; e.prepare(); e.setParameters (p);
    run (e, 4.0);                                   // fill the buffer

    p.clouds.freeze = true;
    p.grids.run = false;                            // sequencer off
    p.voice[0].level = p.voice[1].level = p.voice[2].level = 0.0f;
    e.setParameters (p);
    run (e, 1.0);                                   // let the dry tails die
    const float held = run (e, 4.0).rms;

    check (held > 1.0e-3f, "a frozen cloud kept playing with nothing going in");
}

void test_output_is_the_master()
{
    current = "output_is_the_master";
    auto p = drumKit();
    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    const float full = run (a, 3.0).rms;

    p.output = 0.0f;
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    const float off = run (b, 3.0).rms;

    check (full > 1.0e-3f, "OUTPUT up is audible");
    check (off < 1.0e-7f,  "OUTPUT at zero is silent");
}

void test_every_model_makes_a_sound()
{
    current = "every_model_makes_a_sound";
    for (int m = 0; m < maps::kNumModels; ++m)
    {
        auto p = drumKit();
        p.grids.density[1] = p.grids.density[2] = 0.0f;
        p.voice[0].model = m;
        p.voice[0].decay = 0.5f;
        maps::MapsEngine e; e.prepare(); e.setParameters (p);
        auto out = run (e, 3.0);
        check (out.finite, std::string ("model ") + maps::modelName (m) + " stayed finite");
        check (out.peak > 1.0e-3f,
               std::string ("model ") + maps::modelName (m) + " produced audio");
    }
}

void test_extremes_do_not_blow_up()
{
    current = "extremes_do_not_blow_up";
    maps::Parameters p;
    for (int i = 0; i < maps::kNumVoices; ++i)
        p.voice[i] = { 96.0f, 1.0f, 1.0f, 1.0f, 1.0f, i, 1.0f, 1.0f, false };
    p.grids = { 1.0f, 1.0f, 1.0f, 240.0f, 1.0f, { 1.0f, 1.0f, 1.0f }, true, true };
    p.clouds = { 1.0f, 1.0f, 1.0f, 1.0f, 24.0f, 1.0f, 1.0f, 1.0f, 1.0f, maps::cloudsGranular, false };
    p.output = 1.0f;

    maps::MapsEngine e; e.prepare(); e.setParameters (p);
    auto out = run (e, 12.0);
    check (out.finite, "everything at maximum stayed finite");
    check (out.peak <= 1.0001f, "the output stayed inside the rails");
}

void test_manual_trigger()
{
    current = "manual_trigger";
    auto p = drumKit();
    p.grids.run = false;
    maps::MapsEngine e; e.prepare(); e.setParameters (p);

    std::vector<float> bl (64), br (64);
    e.process (bl.data(), br.data(), 64);              // silence
    e.trigger (0, 1.0f);

    float peak = 0.0f;
    for (int i = 0; i < 200; ++i)
    {
        e.process (bl.data(), br.data(), 64);
        for (float x : bl) peak = std::max (peak, std::fabs (x));
    }
    check (peak > 0.01f, "a MIDI-style trigger fired the voice with the sequencer stopped");
}

void test_deterministic()
{
    current = "deterministic";
    auto p = drumKit();
    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    auto ra = run (a, 2.0, 64);
    auto rb = run (b, 2.0, 64);

    double diff = 0.0;
    for (size_t i = 0; i < ra.l.size(); ++i) diff += std::fabs (ra.l[i] - rb.l[i]);
    check (diff == 0.0, "two fresh engines with the same settings rendered identically");
}

void test_block_size_does_not_change_the_render()
{
    current = "block_size_does_not_change_the_render";
    // Plaits renders twelve samples at a time. If anything in the chain were
    // sensitive to how the host happens to carve up a block, this is where it
    // would show, and it is the bug that presents as random glitching.
    auto p = drumKit();
    maps::MapsEngine a; a.prepare(); a.setParameters (p);
    maps::MapsEngine b; b.prepare(); b.setParameters (p);
    auto ra = run (a, 2.0, 64);
    auto rb = run (b, 2.0, 512);

    double worst = 0.0;
    for (size_t i = 0; i < ra.l.size(); ++i)
        worst = std::max (worst, (double) std::fabs (ra.l[i] - rb.l[i]));
    check (worst < 1.0e-6, "block size 64 and 512 agreed sample for sample");
}

void test_model_latches_on_trigger()
{
    current = "model_latches_on_trigger";
    // MODEL must not take effect until the voice next fires: Plaits resets the
    // engine when the index changes, and doing that under a ringing tail is a
    // click. This is also what makes a beat-synced LFO on MODEL usable.
    auto p = drumKit();
    p.grids.run = false;
    p.voice[0].model = maps::modelAnalogBassDrum;
    p.voice[0].decay = 0.9f;

    maps::MapsEngine e; e.prepare(); e.setParameters (p);

    std::vector<float> bl (64), br (64);
    e.trigger (0, 1.0f);
    for (int i = 0; i < 8; ++i) e.process (bl.data(), br.data(), 64);

    // Change MODEL in the middle of the tail and keep rendering.
    p.voice[0].model = maps::modelAnalogHiHat;
    e.setParameters (p);

    std::vector<float> after;
    for (int i = 0; i < 40; ++i)
    {
        e.process (bl.data(), br.data(), 64);
        after.insert (after.end(), bl.begin(), bl.end());
    }

    // The same thing again, but without ever changing MODEL.
    auto q = drumKit();
    q.grids.run = false;
    q.voice[0].model = maps::modelAnalogBassDrum;
    q.voice[0].decay = 0.9f;
    maps::MapsEngine f; f.prepare(); f.setParameters (q);
    f.trigger (0, 1.0f);
    for (int i = 0; i < 8; ++i) f.process (bl.data(), br.data(), 64);
    std::vector<float> control;
    for (int i = 0; i < 40; ++i)
    {
        f.process (bl.data(), br.data(), 64);
        control.insert (control.end(), bl.begin(), bl.end());
    }

    double worst = 0.0;
    for (size_t i = 0; i < after.size(); ++i)
        worst = std::max (worst, (double) std::fabs (after[i] - control[i]));
    check (worst < 1.0e-9, "changing MODEL mid-tail left the ringing voice alone");

    // ...and the next trigger picks it up.
    e.trigger (0, 1.0f);
    std::vector<float> next;
    for (int i = 0; i < 20; ++i)
    {
        e.process (bl.data(), br.data(), 64);
        next.insert (next.end(), bl.begin(), bl.end());
    }
    f.trigger (0, 1.0f);
    std::vector<float> nextControl;
    for (int i = 0; i < 20; ++i)
    {
        f.process (bl.data(), br.data(), 64);
        nextControl.insert (nextControl.end(), bl.begin(), bl.end());
    }
    double diff = 0.0;
    for (size_t i = 0; i < next.size(); ++i)
        diff += std::fabs (next[i] - nextControl[i]);
    check (diff > 1.0, "the next hit used the new MODEL");
}

void test_synced_lfo_is_repeatable_and_on_the_grid()
{
    current = "synced_lfo_is_repeatable_and_on_the_grid";

    // A sample-and-hold LFO synced to a quarter note must change exactly once
    // per beat, at the beat, and give the same value for the same bar every
    // time the timeline is played. An accumulating LFO does none of that.
    maps::Lfo a, b;
    a.reset(); b.reset();

    float valueAt[8];
    for (int beat = 0; beat < 8; ++beat)
    {
        a.updateSynced ((double) beat + 0.5, 1.0f, maps::lfoRandom);
        valueAt[beat] = a.value();
    }

    int changes = 0;
    float previous = 0.0f;
    for (int i = 0; i < 800; ++i)
    {
        const double ppq = (double) i / 100.0;                 // 8 beats, fine steps
        b.updateSynced (ppq, 1.0f, maps::lfoRandom);
        if (i > 0 && b.value() != previous) ++changes;
        previous = b.value();
    }
    check (changes == 7, "sample-and-hold changed once per beat, got "
                         + std::to_string (changes));

    // Same position, same value, whatever order you ask in.
    maps::Lfo c; c.reset();
    bool same = true;
    for (int beat = 7; beat >= 0; --beat)
    {
        c.updateSynced ((double) beat + 0.5, 1.0f, maps::lfoRandom);
        if (c.value() != valueAt[beat]) same = false;
    }
    check (same, "a synced LFO gave the same value for the same bar");

    // And a ramp really does cover the range across its cycle.
    maps::Lfo r; r.reset();
    r.updateSynced (0.01, 4.0f, maps::lfoRampUp);
    const float low = r.value();
    r.updateSynced (3.99, 4.0f, maps::lfoRampUp);
    const float high = r.value();
    check (low < -0.9f && high > 0.9f, "a synced ramp swept its whole range");
}

} // namespace

int main()
{
    std::printf ("maps-core tests\n");
    test_silent_when_stopped();
    test_sequencer_runs();
    test_mute_removes_a_voice();
    test_level_is_linear();
    test_density_changes_hit_count();
    test_tempo_changes_hit_count();
    test_host_transport_drives_the_clock();
    test_transport_locate_does_not_stall();
    test_clouds_density_dead_zone();
    test_send_reaches_clouds();
    test_freeze_holds_after_the_music_stops();
    test_output_is_the_master();
    test_every_model_makes_a_sound();
    test_extremes_do_not_blow_up();
    test_manual_trigger();
    test_deterministic();
    test_block_size_does_not_change_the_render();
    test_model_latches_on_trigger();
    test_synced_lfo_is_repeatable_and_on_the_grid();

    if (failures == 0) std::printf ("all passed\n");
    else               std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
