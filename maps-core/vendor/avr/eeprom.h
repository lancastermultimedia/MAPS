// Desktop shim: Grids stores six option bits in EEPROM. A plugin keeps
// them in its own state, so these become a plain byte array.
#pragma once
#include <stdint.h>
static uint8_t g_fake_eeprom[16] = { 0 };
inline uint8_t eeprom_read_byte (const uint8_t* a) { return g_fake_eeprom[(uintptr_t) a & 15]; }
inline void eeprom_write_byte (uint8_t* a, uint8_t v) { g_fake_eeprom[(uintptr_t) a & 15] = v; }
