// Renders maps-core straight to a WAV, with no plugin and no host.
//
// This is the fastest loop there is for working on the sound: it builds in a
// few seconds and it exercises exactly the code the box will run. Anything
// that sounds wrong here is a core problem; anything that sounds right here
// and wrong in a DAW is a parameter-plumbing problem, which is a different
// hunt in a different file.
//
//   core_render out.wav [seconds] [key=value ...]
//
// Keys are the panel names, lower case: v1.model, v1.freq, v1.harmonics,
// v1.timbre, v1.morph, v1.decay, v1.level, v1.send, v1.mute (and v2/v3),
// map.x, map.y, chaos, tempo, swing, d1, d2, d3, run, fill,
// position, size, density, texture, pitch, feedback, space, blend, spread,
// mode, freeze, output.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "maps/MapsEngine.h"

namespace {

void writeWav (const char* path, const std::vector<float>& l,
               const std::vector<float>& r, int sr)
{
    const uint32_t n = (uint32_t) l.size(), bytes = n * 4;
    FILE* f = std::fopen (path, "wb");
    if (f == nullptr) { std::fprintf (stderr, "cannot write %s\n", path); return; }
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + bytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr);
    u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (bytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        auto cv = [] (float x) { x = x > 1 ? 1 : (x < -1 ? -1 : x); return (int16_t) (x * 32767.0f); };
        int16_t a = cv (l[i]), b = cv (r[i]);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

bool assign (maps::Parameters& p, const std::string& key, float v)
{
    if (key.size() > 3 && key[0] == 'v' && key[2] == '.')
    {
        const int i = key[1] - '1';
        if (i < 0 || i >= maps::kNumVoices) return false;
        const std::string f = key.substr (3);
        auto& vp = p.voice[i];
        if (f == "model")     { vp.model = (int) v; return true; }
        if (f == "freq")      { vp.frequency = v;   return true; }
        if (f == "harmonics") { vp.harmonics = v;   return true; }
        if (f == "timbre")    { vp.timbre = v;      return true; }
        if (f == "morph")     { vp.morph = v;       return true; }
        if (f == "decay")     { vp.decay = v;       return true; }
        if (f == "level")     { vp.level = v;       return true; }
        if (f == "send")      { vp.send = v;        return true; }
        if (f == "mute")      { vp.mute = v > 0.5f; return true; }
        return false;
    }
    if (key == "map.x")    { p.grids.mapX = v;  return true; }
    if (key == "map.y")    { p.grids.mapY = v;  return true; }
    if (key == "chaos")    { p.grids.chaos = v; return true; }
    if (key == "tempo")    { p.grids.tempo = v; return true; }
    if (key == "swing")    { p.grids.swing = v; return true; }
    if (key == "d1")       { p.grids.density[0] = v; return true; }
    if (key == "d2")       { p.grids.density[1] = v; return true; }
    if (key == "d3")       { p.grids.density[2] = v; return true; }
    if (key == "run")      { p.grids.run  = v > 0.5f; return true; }
    if (key == "fill")     { p.grids.fill = v > 0.5f; return true; }
    if (key == "position") { p.clouds.position = v; return true; }
    if (key == "size")     { p.clouds.size = v;     return true; }
    if (key == "density")  { p.clouds.density = v;  return true; }
    if (key == "texture")  { p.clouds.texture = v;  return true; }
    if (key == "pitch")    { p.clouds.pitch = v;    return true; }
    if (key == "feedback") { p.clouds.feedback = v; return true; }
    if (key == "space")    { p.clouds.space = v;    return true; }
    if (key == "blend")    { p.clouds.blend = v;    return true; }
    if (key == "spread")   { p.clouds.spread = v;   return true; }
    if (key == "mode")     { p.clouds.mode = (int) v; return true; }
    if (key == "freeze")   { p.clouds.freeze = v > 0.5f; return true; }
    if (key == "output")   { p.output = v; return true; }
    return false;
}

} // namespace

int main (int argc, char** argv)
{
    const char* path = argc > 1 ? argv[1] : "maps.wav";
    const double seconds = argc > 2 ? std::atof (argv[2]) : 8.0;

    maps::Parameters p;
    p.voice[0] = { 36.0f, 0.30f, 0.45f, 0.55f, 0.35f, maps::modelAnalogBassDrum,  1.00f, 0.20f, false };
    p.voice[1] = { 48.0f, 0.45f, 0.55f, 0.50f, 0.30f, maps::modelAnalogSnareDrum, 0.80f, 0.40f, false };
    p.voice[2] = { 72.0f, 0.60f, 0.70f, 0.35f, 0.22f, maps::modelModalResonator,  0.55f, 0.70f, false };

    for (int i = 3; i < argc; ++i)
    {
        std::string a = argv[i];
        const size_t eq = a.find ('=');
        if (eq == std::string::npos) { std::fprintf (stderr, "bad argument: %s\n", argv[i]); return 1; }
        const std::string k = a.substr (0, eq);
        if (! assign (p, k, (float) std::atof (a.c_str() + eq + 1)))
        { std::fprintf (stderr, "unknown parameter: %s\n", k.c_str()); return 1; }
    }

    maps::MapsEngine engine;
    engine.prepare();
    engine.setParameters (p);

    maps::Transport t;                       // free-running, at p.grids.tempo
    engine.setTransport (t);

    const int total = (int) (seconds * maps::kCoreSampleRate);
    const int block = 128;
    std::vector<float> outL, outR, bl (block), br (block);
    outL.reserve ((size_t) total); outR.reserve ((size_t) total);

    for (int done = 0; done < total; done += block)
    {
        const int n = std::min (block, total - done);
        engine.setParameters (p);
        engine.process (bl.data(), br.data(), n);
        outL.insert (outL.end(), bl.begin(), bl.begin() + n);
        outR.insert (outR.end(), br.begin(), br.begin() + n);
    }

    writeWav (path, outL, outR, (int) maps::kCoreSampleRate);

    float peak = 0.0f; double sq = 0.0;
    for (float x : outL) { const float a = std::fabs (x); if (a > peak) peak = a; sq += (double) x * x; }
    std::printf ("%s  %.1f s  peak %.3f  rms %.4f\n", path, seconds, peak,
                 std::sqrt (sq / (double) outL.size()));
    return 0;
}
