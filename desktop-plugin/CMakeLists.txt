cmake_minimum_required(VERSION 3.15)
project(DroneSynthPlugin VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
endif()

# The Android app's engine, shared unchanged (the Oboe stream compiles out off Android).
set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/main/cpp)

add_library(dronesynth_engine STATIC
    ${ENGINE_DIR}/synth/SynthEngine.cpp
    ${ENGINE_DIR}/synth/VintageDelay.cpp
    ${ENGINE_DIR}/synth/Looper.cpp
    ${ENGINE_DIR}/synth/Recorder.cpp
    ${ENGINE_DIR}/synth/HallReverb.cpp
    ${ENGINE_DIR}/synth/PlateReverb.cpp
    ${ENGINE_DIR}/synth/ReverbSection.cpp
    ${ENGINE_DIR}/synth/StepSequencer.cpp
    ${ENGINE_DIR}/synth/SequencerBridge.cpp
    ${ENGINE_DIR}/Oscillator.cpp
    ${ENGINE_DIR}/WTOscillator.cpp
    ${ENGINE_DIR}/Filter.cpp
    ${ENGINE_DIR}/VAOnePoleFilter.cpp
    ${ENGINE_DIR}/MoogLadderFilter.cpp
    ${ENGINE_DIR}/EnvelopeGenerator.cpp
    ${ENGINE_DIR}/ModulationMatrix.cpp
    ${ENGINE_DIR}/vendor/soundpipe/moogladder.c
    src/ParameterTable.cpp
    src/DroneSynthCore.cpp
)
target_include_directories(dronesynth_engine PUBLIC ${ENGINE_DIR} src)
if(MSVC)
    target_compile_definitions(dronesynth_engine PUBLIC _USE_MATH_DEFINES NOMINMAX)
    target_compile_options(dronesynth_engine PRIVATE /O2 /fp:fast /wd4244 /wd4305 /wd4267)
else()
    target_compile_options(dronesynth_engine PRIVATE -O3
        -Wno-unused-variable -Wno-unused-function -Wno-unknown-pragmas -Wno-format)
endif()

# Offline tests: parameters, the controller protocol and rendering, no host needed.
add_executable(dronesynth_tests tests/core_test.cpp)
target_link_libraries(dronesynth_tests PRIVATE dronesynth_engine)

# The plugin itself, built by DPF (git submodule at dpf/).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dpf/CMakeLists.txt)
    add_subdirectory(dpf)
    dpf_add_plugin(DroneSynth
        TARGETS vst3 clap au jack
        FILES_DSP src/DroneSynthPlugin.cpp)
    target_include_directories(DroneSynth PUBLIC src)
    target_link_libraries(DroneSynth PUBLIC dronesynth_engine)
else()
    message(WARNING "DPF not found in desktop-plugin/dpf - building the tests only. "
                    "Run: git submodule update --init --recursive")
endif()
