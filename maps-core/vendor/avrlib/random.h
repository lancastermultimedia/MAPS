// Desktop shim for avrlib::Random.
//
// Grids uses this for CHAOS - the per-part perturbation added to the drum map
// lookup. The state is an inline variable so this header needs no matching
// .cc, and so two plugin instances do not fight over a definition.
#pragma once
#include <stdint.h>

namespace avrlib {

class Random {
 public:
  static inline uint16_t GetWord() { Update(); return (uint16_t) (state_ >> 16); }
  static inline uint8_t  GetByte() { return (uint8_t) (GetWord() & 0xff); }
  static inline uint32_t state()   { return state_; }
  static inline void Seed(uint16_t seed) { state_ = seed ? seed : 0x21; }
  static inline void Update() { state_ = state_ * 1664525UL + 1013904223UL; }

 private:
  inline static uint32_t state_ = 0x21;
};

}  // namespace avrlib
