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

set(NAM_CORE_DIR ${CMAKE_CURRENT_LIST_DIR}/external/nam_core_parametric)
set(NAM_CORE_NAM_DIR ${NAM_CORE_DIR}/NAM)
set(NAM_CORE_EIGEN_DIR ${NAM_CORE_DIR}/Dependencies/eigen)
set(NAM_CORE_NLOHMANN_DIR ${NAM_CORE_DIR}/Dependencies/nlohmann)

# Local fallback for missing nested submodule checkout in external/nam_core_parametric.
if(NOT EXISTS ${NAM_CORE_EIGEN_DIR}/Eigen/Core)
  set(NAM_CORE_EIGEN_DIR ${CMAKE_CURRENT_LIST_DIR}/../nam_core_parametric/Dependencies/eigen)
endif()

set(NAM_CORE_SOURCES
  ${NAM_CORE_NAM_DIR}/activations.cpp
  ${NAM_CORE_NAM_DIR}/conv1d.cpp
  ${NAM_CORE_NAM_DIR}/convnet.cpp
  ${NAM_CORE_NAM_DIR}/dsp.cpp
  ${NAM_CORE_NAM_DIR}/get_dsp.cpp
  ${NAM_CORE_NAM_DIR}/lstm.cpp
  ${NAM_CORE_NAM_DIR}/parametric.cpp
  ${NAM_CORE_NAM_DIR}/ring_buffer.cpp
  ${NAM_CORE_NAM_DIR}/util.cpp
  ${NAM_CORE_NAM_DIR}/wavenet.cpp
)

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
  Source/DSP/NamModelEngine.cpp
  Source/DSP/NamModelEngine.h
  Source/NamParametricPluginEditor.cpp
  Source/NamParametricPluginEditor.h
  Source/NamParametricPluginProcessor.cpp
  Source/NamParametricPluginProcessor.h
  ${NAM_CORE_SOURCES}
)

target_include_directories(${TARGET_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/Source
  ${NAM_CORE_DIR}
  ${NAM_CORE_NAM_DIR}
  ${NAM_CORE_EIGEN_DIR}
  ${NAM_CORE_NLOHMANN_DIR}
)

target_compile_definitions(${TARGET_NAME} PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  EIGEN_MPL2_ONLY=1
  EIGEN_NO_DEBUG=1
  NAM_SAMPLE_FLOAT=1
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
