// Desktop shim: the three integer helpers Grids' evaluator calls.
#pragma once
#include <stdint.h>

namespace avrlib {

inline uint8_t U8Mix(uint8_t a, uint8_t b, uint8_t balance) {
  return (uint8_t) (((uint16_t) a * (255 - balance) + (uint16_t) b * balance) >> 8);
}
inline uint8_t U8U8MulShift8(uint8_t a, uint8_t b) {
  return (uint8_t) (((uint16_t) a * b) >> 8);
}
inline uint16_t U8U8Mul(uint8_t a, uint8_t b) {
  return (uint16_t) a * (uint16_t) b;
}

}  // namespace avrlib

using avrlib::U8Mix;
using avrlib::U8U8MulShift8;
using avrlib::U8U8Mul;
