cmake_minimum_required(VERSION 3.22)

project(MONOLITHE VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# AU only exists on macOS; VST3 everywhere; Standalone for quick tests without a DAW
if(APPLE)
    set(MONOLITHE_FORMATS VST3 AU Standalone)
else()
    set(MONOLITHE_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Monolithe
    COMPANY_NAME "Monolithe"
    BUNDLE_ID "fr.monolithe.monolithe"
    PLUGIN_MANUFACTURER_CODE Mnlt
    PLUGIN_CODE Mnl1
    FORMATS ${MONOLITHE_FORMATS}
    PRODUCT_NAME "MONOLITHE"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(Monolithe
    PRIVATE
        source/PluginProcessor.cpp
        source/PluginEditor.cpp)

target_include_directories(Monolithe PRIVATE source)

target_compile_definitions(Monolithe
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Monolithe
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
