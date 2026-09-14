// Desktop shim for AVR program-memory access.
// On an AVR, tables live in flash and need special load instructions.
// Everywhere else they are just arrays.
#pragma once
#include <stdint.h>
#define PROGMEM
typedef uint8_t  prog_uint8_t;
typedef uint16_t prog_uint16_t;
typedef uint32_t prog_uint32_t;
typedef char     prog_char;
#define pgm_read_byte(p)   (*(const uint8_t*)  (p))
#define pgm_read_word(p)   (*(const uint16_t*) (p))
#define pgm_read_dword(p)  (*(const uint32_t*) (p))
