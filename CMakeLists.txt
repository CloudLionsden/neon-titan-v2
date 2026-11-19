cmake_minimum_required(VERSION 3.22)
project(NeonTitan VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: use a local copy if JUCE_PATH is given, otherwise download it automatically.
set(JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout")
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(TITAN_FORMATS VST3 Standalone CACHE STRING "Plugin formats to build")

juce_add_plugin(NeonTitan
    COMPANY_NAME "Brian Reed"
    COMPANY_WEBSITE "https://github.com"
    BUNDLE_ID com.brianreed.neontitan
    PLUGIN_MANUFACTURER_CODE Brrd
    PLUGIN_CODE NTtn
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    FORMATS ${TITAN_FORMATS}
    PRODUCT_NAME "NEON TITAN")

set(TITAN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(NeonTitan PRIVATE ${TITAN_SOURCES})

target_compile_definitions(NeonTitan PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(NeonTitan
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optional command-line tool that renders WAV previews of every preset (used for testing).
option(TITAN_BUILD_RENDER_TOOL "Build the preset render tool" OFF)
if(TITAN_BUILD_RENDER_TOOL)
    juce_add_console_app(TitanRender PRODUCT_NAME "TitanRender")
    target_sources(TitanRender PRIVATE Tools/Render.cpp ${TITAN_SOURCES})
    target_compile_definitions(TitanRender PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 TITAN_RENDER_TOOL=1)
    target_link_libraries(TitanRender PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
