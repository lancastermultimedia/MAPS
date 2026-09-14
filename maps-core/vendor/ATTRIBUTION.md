# Vendored sources

Everything under `maps-core/vendor/` except `grids/`, `avr/` and `avrlib/` is
unmodified source from Émilie Gillet's Mutable Instruments firmware:

- <https://github.com/pichenettes/eurorack> — `plaits/`, `clouds/`, `grids/`
- <https://github.com/pichenettes/stmlib> — `stmlib/`

Both are **MIT licensed**. The licence text is in `LICENSE-mutable.txt`, and
the notice is repeated at the head of every file. MAPS uses this code with
attribution and nothing is taken from any GPL-licensed fork — not VCV Audible
Instruments, not Cardinal — so this tree carries no copyleft obligation.

**Émilie Gillet designed Plaits, Clouds and Grids. MAPS is a rearrangement of
her work into one instrument, and the credit belongs to her.**

## Local modifications

Two, both recorded here so a future update from upstream is a clean diff.

**`plaits/user_data.h`** — added `#include <cstdio>`. The `TEST` branch calls
`printf` and upstream depends on the including translation unit having pulled
in stdio already.

**`grids/`** — ported off AVR. Grids is the one Mutable module that was written
for an ATmega rather than an STM32, so it reaches for AVR headers that do not
exist anywhere else. The port is deliberately minimal:

- `pattern_generator.h` no longer includes `hardware_config.h` (pin
  definitions); the four LED bit constants it actually used are declared
  inline instead.
- `resources.h` no longer includes `avrlib/resources_manager.h`, and the
  `ResourcesManager` typedef is dropped. The pattern tables are read directly.
- `avr/` and `avrlib/` hold small shims: `pgm_read_byte` becomes a plain
  dereference, the EEPROM option bytes become a 16-byte array, and
  `DISALLOW_COPY_AND_ASSIGN`, `U8Mix`, `U8U8Mul` and `U8U8MulShift8` are
  reimplemented.

- `resources.cc` no longer defines the empty `lookup_table_table`, and
  `resources.h` no longer declares it. It existed only to feed the
  `ResourcesManager` this port dropped, and a zero-sized array is a GCC
  extension that MSVC rejects outright.
- `pattern_generator` had every member `static` and drew CHAOS from avrlib's
  one global PRNG. Both are now per-instance, because a plugin can be loaded
  more than once in a process.

The drum maps and the evaluator — where the musical behaviour lives — are
otherwise byte-for-byte upstream.

**`plaits/dsp/fm/algorithms.h`** — four explicit-specialisation declarations
of static const data members are hidden behind `#ifndef _MSC_VER`. They are
declarations by the standard, but MSVC reads them as definitions of const
objects with no initialiser (C2737), and the `extern` that would settle it is
rejected by GCC and Clang. MSVC resolves the members at link time from
`algorithms.cc` instead; GCC and Clang see upstream unchanged.

**`stmlib/utils/dsp.h`** — a `#define __attribute__(x)` shim, guarded to
`_MSC_VER`. The file decorates its interpolators with
`__attribute__((always_inline))`, which MSVC does not understand; they are
declared `inline` anyway, so dropping the attribute costs a hint.

## Why `TEST` is defined for this library

`stmlib/dsp/dsp.h` implements `Sqrt()` as inline ARM assembly
(`vsqrt.f32`), and `plaits/user_data.h` includes `<stm32f37x_conf.h>`. Both
sit behind `#ifdef TEST`, which is upstream's own escape hatch for building
this code on a desktop. `maps_core` defines it PRIVATE, so it never reaches
the plugin.
