cmake_minimum_required(VERSION 3.22)

project(MONOLITHE VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# French labels are written in UTF-8 in the sources
if(MSVC)
    add_compile_options(/utf-8)
endif()

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
        source/PluginEditor.cpp
        source/ui/Theme.cpp
        source/ui/Controls.cpp)

# the MONOLITHE typefaces (SIL Open Font License, see assets/fonts)
juce_add_binary_data(MonolitheAssets
    SOURCES
        assets/fonts/BigShouldersStencilDisplay-Black.ttf
        assets/fonts/BigShouldersDisplay-ExtraBold.ttf
        assets/fonts/IBMPlexMono-Regular.ttf
        assets/fonts/IBMPlexMono-SemiBold.ttf)

target_include_directories(Monolithe PRIVATE source)

target_compile_definitions(Monolithe
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Monolithe
    PRIVATE
        MonolitheAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline renderer: writes one .wav per voice to tune the sound without a DAW
juce_add_console_app(MonolitheRender PRODUCT_NAME "monolithe_render")

target_sources(MonolitheRender PRIVATE tools/render.cpp)
target_include_directories(MonolitheRender PRIVATE source)

target_compile_definitions(MonolitheRender
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(MonolitheRender
    PRIVATE
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Snapshot: a PNG of the plugin window, to check the front panel without a DAW
juce_add_console_app(MonolitheSnapshot PRODUCT_NAME "monolithe_snapshot")
target_sources(MonolitheSnapshot PRIVATE tools/snapshot.cpp)
# reuse the plugin's compiled code: same headers and settings, without compiling JUCE twice
target_include_directories(MonolitheSnapshot PRIVATE $<TARGET_PROPERTY:Monolithe,INCLUDE_DIRECTORIES>)
target_compile_definitions(MonolitheSnapshot PRIVATE $<TARGET_PROPERTY:Monolithe,COMPILE_DEFINITIONS>)
target_link_libraries(MonolitheSnapshot
    PRIVATE
        Monolithe
    PUBLIC
        juce::juce_recommended_config_flags)
