# MAPS — orientation

**M**odular **A**tonal **P**ercussion **S**ynthesizer. Mutable Grids driving
three Plaits voices on the non-pitched engines, through a Clouds send bus.
One instrument, fixed routing, every control always live.

This document is for whoever picks the project up next, including me. It is
the architecture, the rules that matter, and the mistakes already made so they
do not get made twice.

## Commands

```bash
cmake --preset macos          # or windows / linux
cmake --build --preset macos
ctest --preset macos          # engine tests, ~5 s, no audio hardware

./build/maps-plugin/maps_gui_shot_artefacts/Release/maps_gui_shot panel.png 1.0
./build/maps-plugin/maps_plugin_render_artefacts/Release/maps_plugin_render out.wav 8 44100 128 3
./build/maps_render out.wav 8 v1.model=5 chaos=0.4      # the core, no plugin
```

On macOS a successful build installs the AU and VST3 into `~/Library/Audio/
Plug-Ins/`. Logic caches its AU scan, so after the first install quit Logic and
reopen it; after that, rebuilds are picked up on the next launch.

## Architecture, and the one rule

```
maps-core/     pure C++. No JUCE header, no libDaisy header. Ever.
maps-plugin/   parameters, state, transport, MIDI, rate conversion, the panel
maps-daisy/    (not written yet) libDaisy wrapper and mux scanning
```

**If `maps-core` ever includes a framework header, the shared-codebase benefit
is gone.** That rule *is* the architecture. It is why `MapsEngine` is pimpl'd:
the Mutable headers need `TEST` defined to compile off ARM, and none of that
should escape into a plugin translation unit.

`maps-core/vendor/` is Émilie Gillet's firmware, MIT licensed, vendored rather
than fetched so the build is reproducible offline. Local modifications are
listed in `vendor/ATTRIBUTION.md` and every one of them is marked in the source
with `LOCAL MODIFICATION (MAPS)`.

## The seven things that cost weekends

Four of these are hardware problems the plugin does not have. Three bit us
here, and all three were found by running the code rather than reading it.

1. **Block size.** `plaits::Voice::Render` advances its envelopes **once per
   call**, so it must always be given exactly `plaits::kBlockSize` (12)
   samples. Hosts hand out 64, 128, 512 — never 12. `MapsEngine::process`
   therefore renders whole 12-sample chunks into its own buffer and serves the
   host out of that, carrying the remainder to the next block. Without this the
   render depends on the host's buffer size, which presents as intermittent
   glitching and reads like a CPU problem. `test_block_size_does_not_change_
   the_render` pins it.
2. **Clouds in SDRAM.** Hardware only. `DSY_SDRAM_BSS`, or a hard fault at init.
3. **Mux settling time.** Hardware only.
4. **Pot jitter.** Hardware only. A plugin's parameters are already clean.
5. **`TEST` is upstream's own portability switch, and it is load-bearing.**
   `stmlib/dsp/dsp.h` implements `Sqrt()` as inline ARM assembly
   (`vsqrt.f32`), and `plaits/user_data.h` includes `<stm32f37x_conf.h>`. Both
   sit behind `#ifdef TEST`. `maps_core` defines it `PRIVATE`.
6. **Plaits is 48 kHz and Clouds is 32 kHz, both compile-time.** They are
   `static const float` in headers reached by dozens of engines and by the
   wavetables; making them runtime values is a rewrite. So the core runs at a
   fixed 48 kHz, resamples 48↔32 around the Clouds send (`src/RateConvert.h`),
   and the *plugin* resamples once more at its own boundary
   (`CoreResampler`). The hardware has the same 32 kHz problem — the Daisy
   codec at 48 kHz does not make it go away, it hides it — so this lives in
   the core and both builds share it.
7. **Clouds' DENSITY is silent at twelve o'clock.** In granular mode it is a
   meta parameter, and between 0.47 and 0.53 grain overlap is set to exactly
   zero. Half a day went into a render that came back as digital silence out of
   Clouds while everything else worked. `maps_plugin::mapCloudsDensity` skews
   the knob so the dead zone sits at about eight o'clock and takes five per
   cent of the travel to cross. `test_clouds_density_dead_zone` pins it.
   **This is a panel decision too**, not just a firmware one: on the hardware,
   mark the taper or the detent, or the first person to turn that knob will
   think the box is broken.

## Test at the layer the bug lives in

Two layers, and they catch different things.

- `tests/core_test.cc` → `maps_core_test`. Drives `MapsEngine` directly. Fast,
  JUCE-free, runs anywhere. **Blind to parameter plumbing**, because it sets
  the struct itself.
- `tools/plugin_render.cc` → `maps_plugin_render`. Drives the actual
  `AudioProcessor` the way a DAW does, at a chosen sample rate and block size,
  with `paramID=value` overrides. This is where a knob wired to nothing, a
  preset writing a wrong ID, or a resampler that only works at 48 kHz shows up.
- `tools/gui_shot.cpp` → `maps_gui_shot`. Renders the panel to a PNG with no
  window server. Every layout bug so far was found by looking at one.

## The modulation section

Three LFOs, two routes each, folded away behind a bar at the bottom of the
panel. The generator lives in `maps-core/include/maps/Lfo.h` (JUCE-free, so
the box can grow one later); the routing lives in the plugin, because it is
parameter-space work and the core only speaks engineering units.

- **A synced LFO computes phase from `ppqPosition`, it does not accumulate.**
  `phase = frac (ppq / beatsPerCycle)`, `cycle = floor (…)`. That is what puts
  it on the grid rather than near it, makes it correct immediately after a
  locate, and makes a render repeatable. The RANDOM waveform hashes the cycle
  number, so the same bar gives the same value every pass.
- When the transport is stopped the synced LFOs run on `freeRunPpq`, advanced
  by the CLOCK knob's tempo — the same clock the sequencer free-runs on.
  Without it the whole section goes dead the moment you stop the transport,
  which is exactly when you are turning knobs.
- **Modulation is summed in normalised space and converted once**, in
  `modulated()`. A route therefore means the same amount of movement whatever
  a parameter's range or skew is.
- Depth is scaled by 0.5: full depth swings half the range each way, the
  attenuverter convention. Routes sum, so two routes onto one destination
  reach the whole range — which is how one LFO covers all eight engines.
- Destinations are an enum in `Modulation.h` and **the index is what a session
  stores**. Never reorder it; append only. The enumerator is `gridsClock`
  rather than `clock` because `<ctime>` already has one and the ambiguity is
  a compile error you will not enjoy diagnosing.

**MODEL is latched at the trigger.** `MapsEngine::Impl::latchedModel` is
copied from the parameter when a voice fires, and `applyToVoice` uses that
rather than the live value. Plaits resets the engine and the post-processor
when the index changes, so swapping under a ringing tail is a click; and an
LFO sweeping MODEL would otherwise change engine several times inside one hit.
`test_model_latches_on_trigger` pins both halves of that.

**`setResizeLimits` applies the constrainer to the current bounds immediately.**
At construction those are 0 x 0, so it snaps the editor to its minimum and
`resized()` overwrites `scale` with 0.55 before `setSize` is ever reached.
`applyGeometry` holds the wanted scale in a local for exactly this reason. In
the same vein, `mods.onToggle` is assigned *after* the saved open/closed state
is restored, because resizing a half-built editor produces the same symptom.

## Things that were not obvious

**Plaits 1.2 renumbered the engines.** The eight non-pitched ones are at
indices **16–23**; the 1.0 firmware everybody's documentation describes had
them at 8–15. `kFirstRedEngine` in `MapsEngine.cc`.

**Grids' step is a sixteenth here, not a thirty-second.** The firmware's own
comment says 24 ppqn and eight steps to the quarter. MAPS clocks it at 12 ppqn
so a step is a sixteenth and a 32-step pattern is two bars, which is how
Topograph feels in a rack.

**Grids' accents are bits 3–5 of `state()`**, alongside the triggers in bits
0–2. Plaits' own accent is pinned at 0.8 while `level_patched` is false, and
leaving it false is what keeps the internal envelope and LPG working — so the
accent is applied as output gain instead. Musically the same thing on a drum.

**Zero the engine objects before `Init()`.** On the hardware they live in BSS
and the startup code zeroes them for free. Here they are members of a heap
object, and `Init()`/`Reset()` do not touch everything — Grids' pulse counter
and part perturbations, Plaits' per-engine scratch. Left alone you get two
instances of the same instrument that render differently. **But `plaits::Voice`
holds twenty-four polymorphic engines**, so zeroing it wipes their vtable
pointers and the first `Render()` walks off a null: it is memset *and then
reconstructed in place*.

**Three global states had to be made per-instance.** `stmlib::Random` is one
generator shared by every noise source in Plaits and Clouds, and `avrlib::
Random` was the same for Grids' CHAOS; `grids::PatternGenerator` had every
member `static`. Two plugin instances would have fought over all three, and a
render would never repeat. Grids is de-statified in the vendor tree; the stmlib
generator is swapped in and out around each chunk in `renderChunk`.

**Clouds' recording buffer is sized by the SMALL buffer in stereo.**
`Prepare()` sets `buffer_size[0] = buffer_size[1] = buffer_size_[1]` and carves
the FX workspace out of the tail of the large one. Growing only the large
buffer buys no extra recording time. MAPS gives it about 16 seconds a channel
where the module gets one.

**Plaits' post-processor writes `Clip16(1 + …)`** — a deliberate one-LSB offset
in the firmware, worth 3e-5 in float and multiplied by three voices. Hence
`DcBlock` on the dry sum and on the send. A granular buffer full of DC does not
sound like anything you want.

## Gain staging

The dry sum is halved, the wet return is multiplied by 1.8, and the master is
followed by a `tanh` soft clip. Judge level with a plucked note or a drum hit,
never a click: a click makes a delay or a send look far quieter than it is.
`test_level_is_linear` asserts that halving LEVEL costs 6 dB, which is the
cheap way to catch a stray scale factor creeping into the mix.

## Audio-thread rules

`MapsEngine::process` allocates nothing. `prepare()` allocates everything and
is called from `prepareToPlay`. Meters are `std::atomic` and are read by the
editor's 30 Hz timer with `exchange`, so a missed frame is a missed peak and
nothing worse.

## The panel

`kDesignWidth 960 × kDesignHeight 648` is Rev B's 370 × 250 mm at 2.6 px/mm,
scaled by one `AffineTransform` on `PanelCanvas`. Laying out in the real
dimensions is not decoration: it is what makes the plugin a usable Phase 0
tool. If a control is awkward on screen it will be awkward under a finger.

Knob diameters are the hierarchy and they are locked: 26 mm for a voice
primary, 20 mm for a secondary, 12 mm for a trim. Before adding anything to
this panel, check it against the four things that do the Buchla work —
graduated diameters, engraved legends, scribed boxes whose title breaks the
top rule, negative space left alone. The colour is not one of them.

`Section::frameArea()` trims 8 px off the top of the component, because the
title sits *on* the top rule with half of it above. Forget that and the
legends get clipped.

## Two controls the hardware panel does not have

Rev B is 39 pots and 7 switches, and the plugin has 42 and 8. The extras are
**DECAY** per voice and **FREEZE** on Clouds, and both are open questions for
Rev C rather than decisions:

- Without DECAY, the length of every hit is fixed at build time. On the module
  it is a hidden parameter behind a button; on a drum machine it is one of the
  first things you reach for.
- Without FREEZE, Clouds loses its signature move.

Deciding these is exactly what Phase 0 is for. Play the plugin and find out
whether they earn their place; the panel is the only decision that cannot be
cheaply undone.

## Gotchas that have nothing to do with audio

- **Never hand the user a shell command with a trailing `#` comment.** macOS
  zsh does not treat `#` as a comment interactively, and an apostrophe inside
  one leaves them at a `quote>` prompt with no idea why.
- **Parameter IDs are forever.** A saved session stores the strings in
  `ParamIDs.h`. Change one and every project that used the plugin silently
  loses that control's value. Add freely, rename never.
- `JUCE_DECLARE_NON_COPYABLE_*` suppresses the implicit default constructor,
  because a deleted copy constructor is still a user-declared one.
- `juce::String::toUpperCase()` leaves accents alone, so "Émilie" becomes
  "éMILIE". Pass captions already upper-cased, or use ASCII.

## Where the sound lives

| Constant | File | What it does |
|---|---|---|
| `kFirstRedEngine` | MapsEngine.cc | which Plaits bank MODEL selects from |
| `kTicksPerQuarter` | MapsEngine.cc | 12 → a Grids step is a sixteenth |
| `kMaxSwingQuarters` | MapsEngine.cc | swing depth; capped just under one tick |
| `kAntiAlias` | MapsEngine.cc | 14 kHz, the 48↔32 kHz filter corner |
| `kCloudsSmall` | MapsEngine.cc | the granular buffer, ~16 s per channel |
| wet return `1.8f` | MapsEngine.cc | how loud a fully-open SEND comes back |
| `accentGain` 0.62 / 1.0 | MapsEngine.cc | how much a Grids accent is worth |
| `mapCloudsDensity` | ParamIDs.h | where the DENSITY dead zone sits |
| depth `* 0.5f` | PluginProcessor.cpp | how far one modulation route reaches |
| `lfoRateForKnob` | Lfo.h | the free-running rate range, 0.025-20 Hz |
| `lfoDivisions()` | Lfo.h | the synced divisions; index is saved, never reorder |
