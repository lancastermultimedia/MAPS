// Desktop shim for avrlib/base.h — the AVR types Grids needs and nothing else.
#pragma once
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>

// stmlib.h defines this too, identically in effect. Whichever header lands
// first wins and the other must not redefine it.
#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)
#endif
