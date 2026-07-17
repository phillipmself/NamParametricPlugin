cmake_minimum_required(VERSION 3.22)

project(NamParametricPlugin VERSION 0.1.0)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/JUCE)

set(TARGET_NAME NamParametricPlugin)

set(AUDIODSPTOOLS_DIR ${CMAKE_CURRENT_LIST_DIR}/external/AudioDSPTools)
set(NAM_CORE_DIR ${CMAKE_CURRENT_LIST_DIR}/external/nam_core_parametric)
set(NAM_CORE_NAM_DIR ${NAM_CORE_DIR}/NAM)
set(NAM_CORE_EIGEN_DIR ${NAM_CORE_DIR}/Dependencies/eigen)
set(NAM_CORE_NLOHMANN_DIR ${NAM_CORE_DIR}/Dependencies/nlohmann)

if(NOT EXISTS ${AUDIODSPTOOLS_DIR}/dsp/ResamplingContainer/ResamplingContainer.h)
  message(FATAL_ERROR
    "Missing AudioDSPTools dependency at ${AUDIODSPTOOLS_DIR}. "
    "Run: git submodule update --init --recursive")
endif()

if(NOT EXISTS ${NAM_CORE_EIGEN_DIR}/Eigen/Core)
  message(FATAL_ERROR
    "Missing Eigen dependency at ${NAM_CORE_EIGEN_DIR}. "
    "Run: git submodule update --init --recursive")
endif()

file(GLOB NAM_CORE_SOURCES CONFIGURE_DEPENDS
  ${NAM_CORE_NAM_DIR}/*.cpp
  ${NAM_CORE_NAM_DIR}/*/*.cpp
)

add_library(nam_core_runtime OBJECT
  ${NAM_CORE_SOURCES}
)

set_target_properties(nam_core_runtime PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(nam_core_runtime
  SYSTEM PRIVATE
    ${NAM_CORE_DIR}
    ${NAM_CORE_NAM_DIR}
    ${NAM_CORE_EIGEN_DIR}
    ${NAM_CORE_NLOHMANN_DIR}
)

target_compile_definitions(nam_core_runtime PRIVATE
  EIGEN_MPL2_ONLY=1
  EIGEN_NO_DEBUG=1
  NAM_SAMPLE_FLOAT=1
  NAM_ENABLE_A2_FAST=1
)

if(MSVC)
  target_compile_options(nam_core_runtime PRIVATE /W0)
else()
  target_compile_options(nam_core_runtime PRIVATE -w)
endif()

# Keep third-party DSP runtime performant even in Debug builds to avoid audio callback overruns.
if(MSVC)
  target_compile_options(nam_core_runtime PRIVATE $<$<CONFIG:Debug>:/O2>)
else()
  target_compile_options(nam_core_runtime PRIVATE $<$<CONFIG:Debug>:-O3>)
endif()

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
  Source/UI/NamColours.h
  Source/UI/NamLookAndFeel.cpp
  Source/UI/NamLookAndFeel.h
  Source/UI/TopBarComponent.cpp
  Source/UI/TopBarComponent.h
)

# Important: put NAM object files directly into final wrapper binaries,
# not only the JUCE shared static archive, so static factory registrars
# are not dropped by archive dead-stripping.
foreach(wrapper_target IN ITEMS ${TARGET_NAME}_VST3 ${TARGET_NAME}_Standalone)
  if(TARGET ${wrapper_target})
    target_sources(${wrapper_target} PRIVATE
      $<TARGET_OBJECTS:nam_core_runtime>
    )
  endif()
endforeach()

target_include_directories(${TARGET_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/Source
)

target_include_directories(${TARGET_NAME} SYSTEM PRIVATE
  ${AUDIODSPTOOLS_DIR}
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
  NAM_ENABLE_A2_FAST=1
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

option(NAM_PARAMETRIC_BUILD_TESTS "Build NAM model smoke tests" ON)
if(NAM_PARAMETRIC_BUILD_TESTS)
  add_executable(nam_model_smoke_test
    Tests/NamModelEngineSmokeTest.cpp
    Source/DSP/NamModelEngine.cpp
    Source/DSP/NamModelEngine.h
    $<TARGET_OBJECTS:nam_core_runtime>
  )

  target_include_directories(nam_model_smoke_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Source
  )

  target_include_directories(nam_model_smoke_test SYSTEM PRIVATE
    ${AUDIODSPTOOLS_DIR}
    ${NAM_CORE_DIR}
    ${NAM_CORE_NAM_DIR}
    ${NAM_CORE_EIGEN_DIR}
    ${NAM_CORE_NLOHMANN_DIR}
  )

  target_compile_definitions(nam_model_smoke_test PRIVATE
    EIGEN_MPL2_ONLY=1
    EIGEN_NO_DEBUG=1
    NAM_SAMPLE_FLOAT=1
    NAM_ENABLE_A2_FAST=1
  )

  set(NAM_CONCAT_TEST_MODEL "" CACHE FILEPATH "ConcatWaveNet model used by CTest")
  set(NAM_HYPER_TEST_MODEL "" CACHE FILEPATH "HyperWaveNet model used by CTest")
  if(BUILD_TESTING AND EXISTS "${NAM_CONCAT_TEST_MODEL}" AND EXISTS "${NAM_HYPER_TEST_MODEL}")
    add_test(NAME nam_model_smoke
      COMMAND nam_model_smoke_test "${NAM_CONCAT_TEST_MODEL}" "${NAM_HYPER_TEST_MODEL}"
    )
  endif()
endif()
