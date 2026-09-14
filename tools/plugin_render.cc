// Drives the plugin the way a DAW does, and writes a WAV.
//
// The core tests set maps::Parameters directly, so they cannot see a knob
// that is wired to nothing, a preset that writes the wrong ID, or a resampler
// that only works at 48 kHz. That is what this is for.
//
//   maps_plugin_render out.wav [seconds] [sampleRate] [blockSize] [preset]
//                      [paramID=value ...]

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::String path = argc > 1 ? argv[1] : "plugin.wav";
    const double seconds    = argc > 2 ? juce::String (argv[2]).getDoubleValue() : 8.0;
    const double rate       = argc > 3 ? juce::String (argv[3]).getDoubleValue() : 48000.0;
    const int    block      = argc > 4 ? juce::String (argv[4]).getIntValue() : 512;
    const int    preset     = argc > 5 ? juce::String (argv[5]).getIntValue() : 0;

    MapsProcessor proc;
    proc.setPlayConfigDetails (0, 2, rate, block);
    proc.prepareToPlay (rate, block);
    if (preset > 0)
        proc.setCurrentProgram (preset);

    for (int i = 6; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        const int eq = a.indexOfChar ('=');
        if (eq < 0) { std::fprintf (stderr, "bad argument: %s\n", argv[i]); return 1; }
        const auto id = a.substring (0, eq);
        const float v = a.substring (eq + 1).getFloatValue();
        auto* p = proc.apvts.getParameter (id);
        if (p == nullptr) { std::fprintf (stderr, "no such parameter: %s\n", id.toRawUTF8()); return 1; }
        p->setValueNotifyingHost (p->convertTo0to1 (v));
    }

    const int total = (int) (seconds * rate);
    juce::AudioBuffer<float> out (2, total);
    out.clear();
    juce::AudioBuffer<float> scratch (2, block);
    juce::MidiBuffer midi;

    for (int done = 0; done < total; done += block)
    {
        const int n = juce::jmin (block, total - done);
        scratch.setSize (2, n, false, false, true);
        scratch.clear();
        proc.processBlock (scratch, midi);
        for (int c = 0; c < 2; ++c)
            out.copyFrom (c, done, scratch, c, 0, n);
    }

    juce::WavAudioFormat wav;
    juce::File file (juce::File::getCurrentWorkingDirectory().getChildFile (path));
    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr) { std::fputs ("cannot write\n", stderr); return 1; }
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wav.createWriterFor (stream.release(), rate, 2, 24, {}, 0));
    if (writer == nullptr) { std::fputs ("no writer\n", stderr); return 1; }
    writer->writeFromAudioSampleBuffer (out, 0, total);
    writer.reset();

    const float peak = out.getMagnitude (0, total);
    const float rms  = out.getRMSLevel (0, 0, total);
    std::printf ("%s  %.1f s @ %.0f Hz block %d  peak %.3f  rms %.4f\n",
                 file.getFullPathName().toRawUTF8(), seconds, rate, block, peak, rms);
    return 0;
}
