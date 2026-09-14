# maps-plugin — the framework wrapper, and nothing else.
#
# Everything that makes a sound is in maps_core. What lives here is parameters,
# state, host transport, MIDI, sample-rate conversion at the boundary, and the
# Rev B panel drawn on a screen. If a change to how MAPS *sounds* ends up in
# this directory, it is in the wrong directory.

juce_add_plugin (MAPS
    COMPANY_NAME              "Lancaster Multimedia"
    BUNDLE_ID                 com.lancastermultimedia.maps
    PLUGIN_MANUFACTURER_CODE  Lncm
    PLUGIN_CODE               Maps
    FORMATS                   AU VST3 Standalone
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   ${MAPS_COPY_AFTER_BUILD}
    PRODUCT_NAME              "MAPS"
    DESCRIPTION               "Modular Atonal Percussion Synthesizer"
    VST3_CATEGORIES           Instrument Drum
    AU_MAIN_TYPE              kAudioUnitType_MusicDevice)

target_sources (MAPS PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp
    Source/Modulation.cpp
    Source/gui/Look.cpp
    Source/gui/Controls.cpp
    Source/gui/Sections.cpp
    Source/gui/ModSection.cpp)

target_compile_definitions (MAPS PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_include_directories (MAPS PRIVATE Source)

target_link_libraries (MAPS
    PRIVATE
        maps_core
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# ------------------------------------------------------------ the panel shot
# A console app that builds the editor and renders it to a PNG, so the panel
# can be looked at without a DAW or even a window server. Every layout bug
# found so far was found this way.
if (MAPS_BUILD_TOOLS)
    juce_add_console_app (maps_gui_shot PRODUCT_NAME "maps_gui_shot")
    target_sources (maps_gui_shot PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/gui_shot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Presets.cpp
        Source/Modulation.cpp
        Source/gui/Look.cpp
        Source/gui/Controls.cpp
        Source/gui/Sections.cpp
        Source/gui/ModSection.cpp)
    target_include_directories (maps_gui_shot PRIVATE Source)
    target_compile_definitions (maps_gui_shot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="MAPS"
        JucePlugin_Desc="Modular Atonal Percussion Synthesizer"
        JucePlugin_Manufacturer="Lancaster Multimedia"
        JucePlugin_IsSynth=1
        JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_VersionString="1.0.0"
        JucePlugin_VersionCode=0x10000)
    target_link_libraries (maps_gui_shot PRIVATE
        maps_core
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags)

    juce_add_console_app (maps_plugin_render PRODUCT_NAME "maps_plugin_render")
    target_sources (maps_plugin_render PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/plugin_render.cc
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Presets.cpp
        Source/Modulation.cpp
        Source/gui/Look.cpp
        Source/gui/Controls.cpp
        Source/gui/Sections.cpp
        Source/gui/ModSection.cpp)
    target_include_directories (maps_plugin_render PRIVATE Source)
    target_compile_definitions (maps_plugin_render PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="MAPS"
        JucePlugin_Desc="Modular Atonal Percussion Synthesizer"
        JucePlugin_Manufacturer="Lancaster Multimedia"
        JucePlugin_IsSynth=1
        JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_VersionString="1.0.0"
        JucePlugin_VersionCode=0x10000)
    target_link_libraries (maps_plugin_render PRIVATE
        maps_core
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
