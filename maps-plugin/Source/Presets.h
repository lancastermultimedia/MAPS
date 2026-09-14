#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/** Factory presets. These are not sound-design showcases; they are starting
    points that each put the instrument in a different corner of itself, so
    that the first ten minutes with MAPS cover more than one idea. */
namespace presets {

int          count();
juce::String name (int index);
void         apply (juce::AudioProcessorValueTreeState& apvts, int index);

} // namespace presets
