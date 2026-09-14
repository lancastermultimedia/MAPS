# Licences

MAPS is made of three layers with different licences. This file says which is
which, because "it's open source" is not an answer anyone can act on.

## MAPS itself — AGPL-3.0-or-later

Everything in `maps-plugin/`, `tests/`, `tools/`, and `maps-core/` except
`maps-core/vendor/`. The full text is in `LICENSE`.

**Why AGPL and not GPL:** JUCE 8's open-source licence is the **AGPLv3**, not
the GPLv3 that JUCE 6 and 7 offered. Anything linking the JUCE modules without
a commercial JUCE licence inherits it. For a plugin you download and run, the
practical difference from GPLv3 is small — the network clause only bites if
you offer the software's functionality to users over a network — but the
obligation to publish source to anyone you give a binary to is real, and it is
why this repository exists.

If MAPS is ever to be sold as closed source, the JUCE modules have to be
relicensed commercially, and the VST3 SDK too. **`maps-core` is deliberately
free of both**, so that is a wrapper decision rather than a rewrite.

## The synthesis — MIT, by Émilie Gillet

`maps-core/vendor/` is Mutable Instruments firmware: Plaits, Clouds, Grids and
stmlib, from <https://github.com/pichenettes/eurorack> and
<https://github.com/pichenettes/stmlib>. MIT licensed; the notice is at the
head of every file and the text is in `maps-core/vendor/LICENSE-mutable.txt`.
Local modifications are listed in `maps-core/vendor/ATTRIBUTION.md`.

MIT imposes no copyleft, so this part could be used in a closed product with
attribution. **Nothing here is taken from VCV Audible Instruments or from
Cardinal**, both of which are GPL-3 forks of the same code; copying from those
instead would have made any derived work GPL-3 permanently. Pulling from the
original repository was a deliberate choice and it should stay that way.

**Plaits, Clouds and Grids are Émilie Gillet's designs. The interesting parts
of MAPS are hers.**

## JUCE — AGPLv3 or commercial

Fetched at build time, not vendored. <https://juce.com/legal/>

## The VST3 SDK — GPLv3 or Steinberg's proprietary licence

Pulled in by JUCE when the VST3 target is built. The AU and Standalone builds
do not touch it.
