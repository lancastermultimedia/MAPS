// Renders the panel to a PNG with no window server and no DAW.
//
// This exists because looking at the panel is the only way to catch a layout
// bug, and because a knob that is wired to nothing looks exactly like a knob
// that is wired to something until you move it and take another picture.
//
//   maps_gui_shot out.png [scale] [preset] [openModulation]

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::String path  = argc > 1 ? argv[1] : "panel.png";
    const float        scale = argc > 2 ? (float) juce::String (argv[2]).getFloatValue() : 1.0f;
    const int          preset = argc > 3 ? juce::String (argv[3]).getIntValue() : 0;
    const bool         openMods = argc > 4 && juce::String (argv[4]).getIntValue() != 0;

    MapsProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    if (preset > 0)
        proc.setCurrentProgram (preset);

    // Run a little audio so the meters and the step display have something in
    // them; a screenshot of a dead panel hides half of what can be wrong.
    {
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        for (int i = 0; i < 200; ++i)
            proc.processBlock (buf, midi);
    }

    // The editor reads these on construction, so they have to be set first.
    proc.apvts.state.setProperty ("uiScale", scale, nullptr);
    proc.apvts.state.setProperty ("modOpen", openMods, nullptr);

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    if (editor == nullptr) { std::fputs ("no editor\n", stderr); return 1; }

    // The editor's timer never fires without a message loop, and MapsEditor
    // inherits Timer privately so it cannot be poked directly. A short spin of
    // the dispatch loop is what makes the meters, the model names and the mode
    // legend appear - all of which are things a screenshot needs to show.
    juce::MessageManager::getInstance()->runDispatchLoopUntil (250);

    juce::Image image (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
    {
        juce::Graphics g (image);
        editor->paintEntireComponent (g, true);
    }

    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (path));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    if (! stream.openedOk()) { std::fputs ("cannot write\n", stderr); return 1; }
    juce::PNGImageFormat png;
    png.writeImageToStream (image, stream);
    std::printf ("%s  %d x %d\n", out.getFullPathName().toRawUTF8(),
                 image.getWidth(), image.getHeight());
    return 0;
}
