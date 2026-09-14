# maps-core — the instrument, with no framework anywhere near it.
#
# This library must never gain a JUCE or libDaisy dependency. If it does, the
# shared-codebase argument for building the plugin first collapses, and the
# box and the plugin start to diverge in ways nobody notices until they sound
# different. That rule is the architecture.

set (MAPS_VENDOR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

set (MAPS_VENDOR_SOURCES
    ${MAPS_VENDOR}/stmlib/dsp/units.cc
    ${MAPS_VENDOR}/stmlib/dsp/atan.cc
    ${MAPS_VENDOR}/stmlib/utils/random.cc

    ${MAPS_VENDOR}/plaits/resources.cc
    ${MAPS_VENDOR}/plaits/dsp/voice.cc

    ${MAPS_VENDOR}/clouds/resources.cc
    ${MAPS_VENDOR}/clouds/dsp/granular_processor.cc
    ${MAPS_VENDOR}/clouds/dsp/correlator.cc
    ${MAPS_VENDOR}/clouds/dsp/mu_law.cc

    ${MAPS_VENDOR}/grids/pattern_generator.cc
    ${MAPS_VENDOR}/grids/resources.cc
)

# The engine banks are many small files and they arrive and leave with
# upstream, so these three are globbed rather than listed.
file (GLOB MAPS_PLAITS_ENGINES CONFIGURE_DEPENDS
    ${MAPS_VENDOR}/plaits/dsp/engine/*.cc
    ${MAPS_VENDOR}/plaits/dsp/engine2/*.cc
    ${MAPS_VENDOR}/plaits/dsp/chords/*.cc
    ${MAPS_VENDOR}/plaits/dsp/fm/*.cc
    ${MAPS_VENDOR}/plaits/dsp/physical_modelling/*.cc
    ${MAPS_VENDOR}/plaits/dsp/speech/*.cc)

file (GLOB MAPS_CLOUDS_PVOC CONFIGURE_DEPENDS
    ${MAPS_VENDOR}/clouds/dsp/pvoc/*.cc)

add_library (maps_core STATIC
    src/MapsEngine.cc
    ${MAPS_VENDOR_SOURCES}
    ${MAPS_PLAITS_ENGINES}
    ${MAPS_CLOUDS_PVOC})

target_include_directories (maps_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${MAPS_VENDOR})

# TEST is upstream's own switch for building this code off ARM: it swaps
# stmlib's inline `vsqrt.f32` assembly for sqrtf() and keeps
# plaits/user_data.h away from <stm32f37x_conf.h>. PRIVATE, so a plugin
# translation unit never sees it.
target_compile_definitions (maps_core PRIVATE TEST=1)

target_compile_features (maps_core PUBLIC cxx_std_17)
set_target_properties (maps_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Upstream is warning-clean on its own terms but not on ours, and these would
# otherwise bury anything worth reading in the core's own code.
if (MSVC)
    target_compile_options (maps_core PRIVATE /wd4244 /wd4305 /wd4838 /wd4996 /bigobj)

    # Mutable's oscillators use M_PI, which MSVC's <cmath> only defines when
    # this is set before the include. As a compiler-level definition it
    # applies to the whole translation unit, which is what the vendored
    # headers need. /bigobj above is insurance for the resource tables:
    # plaits/resources.cc alone is 372 KB of source.
    target_compile_definitions (maps_core PRIVATE _USE_MATH_DEFINES)
else()
    target_compile_options (maps_core PRIVATE -Wno-narrowing -Wno-unused-function)
endif()
