cmake_minimum_required(VERSION 3.22)

project(NamParametricPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/JUCE)

set(TARGET_NAME NamParametricPlugin)

juce_add_plugin(${TARGET_NAME}
  COMPANY_NAME "phillipmself"
  PRODUCT_NAME "NAM Parametric Plugin"
  PLUGIN_MANUFACTURER_CODE PmSf
  PLUGIN_CODE NpPg
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  FORMATS VST3 Standalone
  COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(${TARGET_NAME})

target_sources(${TARGET_NAME} PRIVATE
  Source/NamParametricPluginEditor.cpp
  Source/NamParametricPluginEditor.h
  Source/NamParametricPluginProcessor.cpp
  Source/NamParametricPluginProcessor.h
)

target_include_directories(${TARGET_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/Source
)

target_compile_definitions(${TARGET_NAME} PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(${TARGET_NAME}
  PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
