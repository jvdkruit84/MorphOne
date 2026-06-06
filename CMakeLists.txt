cmake_minimum_required(VERSION 3.22)
project(MorphOne VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

juce_add_plugin(MorphOne
    COMPANY_NAME "JVD"
    PLUGIN_MANUFACTURER_CODE Jvdx
    PLUGIN_CODE Mrp1
    FORMATS VST3 Standalone
    PRODUCT_NAME "MorphOne"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(MorphOne)

target_sources(MorphOne PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/WavetableOscillator.h
    Source/PluginProcessor.h
    Source/PluginEditor.h
    Source/PresetManager.h
    Source/TheoryEngine.h
    Source/SmartArp.h
    Source/ProgressionEngine.h
)

target_compile_definitions(MorphOne PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MorphOne PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
