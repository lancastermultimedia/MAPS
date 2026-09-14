#include "PluginEditor.h"
#include "Presets.h"

using namespace maps_ui;

void PanelCanvas::paint (juce::Graphics& g)
{
    paintPanel (g, getLocalBounds().toFloat());

    // The wordmark is engraved into the panel, not a label sitting on top of
    // it, so it scales with everything else and never needs a font loaded.
    drawEngraved (g, "MAPS", { 18.0f, 16.0f, 300.0f, 32.0f },
                  juce::Justification::left, 28.0f, 8.0f);
    drawEngraved (g, "MODULAR ATONAL PERCUSSION SYNTHESIZER",
                  { 21.0f, 50.0f, 520.0f, 14.0f },
                  juce::Justification::left, 8.5f, 2.8f);
    drawEngraved (g, "LANCASTER MULTIMEDIA  -  AFTER EMILIE GILLET",
                  { 21.0f, 64.0f, 520.0f, 12.0f },
                  juce::Justification::left, 7.5f, 1.6f);
}

MapsEditor::MapsEditor (MapsProcessor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p),
      voicesBox ("TRIPLE PERCUSSION SOURCE", "258"),
      grids (p.apvts),
      clouds (p.apvts),
      output (p.apvts, ids::output, "OUTPUT", Knob::medium),
      mods (p.apvts)
{
    addAndMakeVisible (canvas);
    canvas.addAndMakeVisible (voicesBox);
    canvas.addAndMakeVisible (grids);
    canvas.addAndMakeVisible (clouds);
    canvas.addAndMakeVisible (output);
    canvas.addAndMakeVisible (outputMeter);
    canvas.addAndMakeVisible (mods);

    for (int i = 0; i < maps::kNumVoices; ++i)
    {
        voices[i] = std::make_unique<VoiceStrip> (p.apvts, i + 1);
        canvas.addAndMakeVisible (*voices[i]);
    }

    for (int i = 0; i < presets::count(); ++i)
        presetBox.addItem (presets::name (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a2825));
    presetBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffe8e3d8));
    presetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0x88000000));
    presetBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffbdb7a9));
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index != proc.getCurrentProgram())
            proc.setCurrentProgram (index);
    };
    canvas.addAndMakeVisible (presetBox);

    auto step = [this] (int delta)
    {
        const int n = presets::count();
        if (n <= 0) return;
        const int next = (proc.getCurrentProgram() + delta + n) % n;
        presetBox.setSelectedId (next + 1);
    };
    prevPreset.onClick = [step] { step (-1); };
    nextPreset.onClick = [step] { step (+1); };
    for (auto* b : { &prevPreset, &nextPreset })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2825));
        b->setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe8e3d8));
        b->setWantsKeyboardFocus (false);
        canvas.addAndMakeVisible (*b);
    }

    // The window remembers its size and whether the modulation section was
    // open, per instance, across sessions.
    if (auto stored = proc.apvts.state.getProperty ("uiScale"); ! stored.isVoid())
        scale = juce::jlimit (0.55f, 1.50f, (float) stored);
    if (auto stored = proc.apvts.state.getProperty ("modOpen"); ! stored.isVoid())
        mods.setOpen ((bool) stored);

    // Assigned only now: setOpen() above fires onToggle, and resizing a
    // half-built editor is how you end up with a window at its minimum size
    // wondering why.
    mods.onToggle = [this]
    {
        proc.apvts.state.setProperty ("modOpen", mods.isOpen(), nullptr);
        applyGeometry();
        layoutPanel();
    };

    setResizable (true, true);
    applyGeometry();
    layoutPanel();
    startTimerHz (30);
}

MapsEditor::~MapsEditor()
{
    proc.apvts.state.setProperty ("uiScale", scale, nullptr);
}

void MapsEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2a2825));
}

void MapsEditor::applyScale (float newScale)
{
    scale = juce::jlimit (0.55f, 1.50f, newScale);
    canvas.setTransform (juce::AffineTransform::scale (scale));
    canvas.setBounds (0, 0, kDesignWidth, designHeight());
}

/** Called whenever the canvas changes shape — at startup, and each time the
    modulation section folds. The width is held and the height follows, so
    opening the section never rescales the panel under the user's hands. */
void MapsEditor::applyGeometry()
{
    const int h = designHeight();

    // setResizeLimits applies the constrainer to the CURRENT bounds straight
    // away, and at construction those are 0 x 0 — so it snaps the window to
    // the minimum and resized() overwrites `scale` with 0.55 before we ever
    // get to setSize. Hold the scale we actually want in a local.
    const float wanted = scale;

    setResizeLimits ((int) (kDesignWidth * 0.55f), (int) (h * 0.55f),
                     (int) (kDesignWidth * 1.50f), (int) (h * 1.50f));
    getConstrainer()->setFixedAspectRatio ((double) kDesignWidth / (double) h);

    scale = wanted;
    setSize ((int) (kDesignWidth * wanted), (int) (h * wanted));
    applyScale (wanted);
}

void MapsEditor::resized()
{
    applyScale ((float) getWidth() / (float) kDesignWidth);
}

void MapsEditor::layoutPanel()
{
    const int margin = 14;
    const int headerH = 62;

    // Header: the wordmark on the left, presets and the master on the right.
    const int headerTop = margin;
    const int right = kDesignWidth - margin;

    presetBox.setBounds (right - 380, headerTop + 6, 190, 24);
    prevPreset.setBounds (right - 380 - 52, headerTop + 6, 24, 24);
    nextPreset.setBounds (right - 380 - 26, headerTop + 6, 24, 24);

    const int outW = (int) (Knob::diameterFor (Knob::medium) + 34.0f);
    output.setBounds (right - outW, headerTop - 2, outW,
                      (int) Knob::heightFor (Knob::medium, false));
    outputMeter.setBounds (right - 110 - outW - 4, headerTop + 26, 110, 6);

    // Band one: the three voices, full width. Plaits first, and it shows.
    const int voicesTop = headerTop + headerH + 8;
    const int voicesH   = 296;
    voicesBox.setBounds (margin, voicesTop, kDesignWidth - margin * 2, voicesH);

    const auto inner = voicesBox.contentArea().toNearestInt()
                         .translated (voicesBox.getX(), voicesBox.getY()).reduced (2, 0);
    const int stripW = inner.getWidth() / maps::kNumVoices;
    for (int i = 0; i < maps::kNumVoices; ++i)
        voices[i]->setBounds (inner.getX() + stripW * i, inner.getY(), stripW - 6, inner.getHeight());

    // Band two: sequencer left, granular right. Asymmetric on purpose —
    // Clouds has nine knobs to Grids' eight but wants them in two clean rows.
    const int bottomTop = voicesTop + voicesH + 10;
    const int bottomH   = kPanelHeight - bottomTop - margin;
    const int gridsW    = 424;
    grids.setBounds (margin, bottomTop, gridsW, bottomH);
    clouds.setBounds (margin + gridsW + 10, bottomTop,
                      kDesignWidth - margin * 2 - gridsW - 10, bottomH);

    // Band three: the modulation sources, folded away by default.
    mods.setBounds (0, kPanelHeight, kDesignWidth,
                    maps_ui::ModSection::heightFor (mods.isOpen()) + 6);
}

void MapsEditor::timerCallback()
{
    auto& engine = proc.getEngine();

    auto modOf = [this] (int dest) { return proc.modulationOffset (dest); };

    for (int i = 0; i < maps::kNumVoices; ++i)
        voices[i]->refresh (engine.voicePeak (i), modOf);

    outputMeter.push (engine.outputPeak());
    output.setModulation (modOf (modulation::output));
    grids.refresh (engine.currentStep(), engine.lastStepTriggers(), modOf);
    clouds.refresh (modOf);
    mods.refresh();

    const int program = proc.getCurrentProgram();
    if (presetBox.getSelectedId() != program + 1)
        presetBox.setSelectedId (program + 1, juce::dontSendNotification);
}

