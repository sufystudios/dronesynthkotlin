cmake_minimum_required(VERSION 3.22.1)
project(dronesynth)

find_package(oboe REQUIRED CONFIG)

add_library(dronesynth SHARED
        # Engine
        src/main/cpp/synth/SynthEngine.cpp
        src/main/cpp/synth/VintageDelay.cpp
        src/main/cpp/synth/Looper.cpp
        src/main/cpp/synth/Recorder.cpp
        src/main/cpp/synth/JniBridge.cpp
        src/main/cpp/synth/HallReverb.cpp
        src/main/cpp/synth/PlateReverb.cpp
        src/main/cpp/synth/ReverbSection.cpp

        # DSP building blocks (oscillator, Moog ladder, envelope generator)
        src/main/cpp/Oscillator.cpp
        src/main/cpp/WTOscillator.cpp
        src/main/cpp/Filter.cpp
        src/main/cpp/VAOnePoleFilter.cpp
        src/main/cpp/MoogLadderFilter.cpp
        src/main/cpp/EnvelopeGenerator.cpp
        src/main/cpp/ModulationMatrix.cpp

        # The iOS app's Moog ladder filter (Soundpipe, MIT)
        src/main/cpp/vendor/soundpipe/moogladder.c
        )

target_compile_options(dronesynth PRIVATE -O3 -Wall -Wno-unused-variable -Wno-unused-function -Wno-unknown-pragmas -Wno-format -Wno-implicit-const-int-float-conversion)

find_library(log-lib log)

target_link_libraries(dronesynth
        android
        oboe::oboe
        ${log-lib}
        )
