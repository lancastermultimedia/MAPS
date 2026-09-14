// Desktop stub for clouds/drivers/debug_pin.h.
//
// Upstream this drives a GPIO high for the duration of the audio callback so
// a scope can measure CPU load. There is no pin here, and the TIC/TOC uses in
// granular_processor.cc are commented out anyway; this exists only so the
// include resolves.
#pragma once

namespace clouds {

class DebugPin {
 public:
  static inline void Init() { }
  static inline void High() { }
  static inline void Low()  { }
};

}  // namespace clouds

#define TIC
#define TOC
