cmake_minimum_required (VERSION 3.22)

project (MAPS VERSION 1.0.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set (CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option (MAPS_COPY_AFTER_BUILD "Install the plugin into the system folders after a build" ON)
option (MAPS_BUILD_TOOLS      "Build the headless render tools"                          ON)

# ---------------------------------------------------------------- the engine
# No JUCE below this line. See maps-core/CMakeLists.txt.
add_subdirectory (maps-core)

# ------------------------------------------------------------------ the tests
enable_testing()
add_executable (maps_core_test tests/core_test.cc)
target_link_libraries (maps_core_test PRIVATE maps_core)
add_test (NAME core COMMAND maps_core_test)

if (MAPS_BUILD_TOOLS)
    add_executable (maps_render tools/core_render.cc)
    target_link_libraries (maps_render PRIVATE maps_core)
endif()

# ----------------------------------------------------------------- the plugin
include (FetchContent)
FetchContent_Declare (JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable (JUCE)

add_subdirectory (maps-plugin)
