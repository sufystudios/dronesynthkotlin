//
// JNI entry points for xyz.sufystudios.synthguide.audio.NativeSynth.
// One engine per process; every call is a no-op until create() has run.
//

#include <jni.h>

#include <algorithm>
#include <array>
#include <memory>

#include "SynthEngine.h"

using synth::SynthEngine;

namespace {
std::unique_ptr<SynthEngine> gEngine;
}

#define NATIVE_SYNTH(name) Java_xyz_sufystudios_synthguide_audio_NativeSynth_##name

extern "C" {

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeCreate)(JNIEnv *, jobject) {
    if (!gEngine) gEngine = std::make_unique<SynthEngine>();
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeDestroy)(JNIEnv *, jobject) {
    gEngine.reset();
}

JNIEXPORT jboolean JNICALL NATIVE_SYNTH(nativeStart)(JNIEnv *, jobject) {
    return gEngine && gEngine->start();
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeStop)(JNIEnv *, jobject) {
    if (gEngine) gEngine->stop();
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeSetParam)(JNIEnv *, jobject, jint id, jfloat value) {
    if (gEngine) gEngine->setParam(id, value);
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeNoteOn)(JNIEnv *, jobject, jint note, jint velocity) {
    if (gEngine) gEngine->noteOn(note, velocity);
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeNoteOff)(JNIEnv *, jobject, jint note) {
    if (gEngine) gEngine->noteOff(note);
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeAllNotesOff)(JNIEnv *, jobject) {
    if (gEngine) gEngine->allNotesOff();
}

JNIEXPORT jfloat JNICALL NATIVE_SYNTH(nativeTakePeak)(JNIEnv *, jobject) {
    return gEngine ? gEngine->takePeak() : 0.0f;
}

/// Resolved delay times as {leftSeconds, rightSeconds, clamped (0/1)}, computed by
/// the same function the delay kernel uses.
JNIEXPORT jfloatArray JNICALL NATIVE_SYNTH(nativeDelayTimes)(JNIEnv *env, jobject, jfloat timeMs, jfloat subdivide,
                                                            jfloat timeLeft, jfloat timeRight, jint mixMode) {
    const double rate = gEngine ? gEngine->sampleRate() : 48000.0;
    const synth::VintageDelayTaps taps = synth::vintageDelayComputeTaps(timeMs, subdivide, timeLeft, timeRight, mixMode, rate);
    const jfloat values[3] = {float(taps.left / rate), float(taps.right / rate), taps.clamped ? 1.0f : 0.0f};
    jfloatArray result = env->NewFloatArray(3);
    env->SetFloatArrayRegion(result, 0, 3, values);
    return result;
}

// MARK: - Looper

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeLooperCommand)(JNIEnv *, jobject, jint command) {
    if (gEngine) gEngine->looper().request(synth::Looper::Command(command));
}

/// {flags, lengthFrames, positionFrames, recordedFrames}
JNIEXPORT jintArray JNICALL NATIVE_SYNTH(nativeLooperState)(JNIEnv *env, jobject) {
    jint values[4] = {0, 0, 0, 0};
    if (gEngine) {
        synth::Looper &l = gEngine->looper();
        values[0] = l.flags();
        values[1] = l.lengthFrames();
        values[2] = l.positionFrames();
        values[3] = l.recordedFrames();
    }
    jintArray result = env->NewIntArray(4);
    env->SetIntArrayRegion(result, 0, 4, values);
    return result;
}

// MARK: - Recorder

JNIEXPORT jboolean JNICALL NATIVE_SYNTH(nativeRecorderStart)(JNIEnv *env, jobject, jstring path) {
    if (!gEngine) return false;
    const char *chars = env->GetStringUTFChars(path, nullptr);
    const bool ok = gEngine->recorder().start(chars);
    env->ReleaseStringUTFChars(path, chars);
    return ok;
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeRecorderStop)(JNIEnv *, jobject) {
    if (gEngine) gEngine->recorder().stop();
}

/// {recording (0/1), open (0/1), recordedFrames}
JNIEXPORT jlongArray JNICALL NATIVE_SYNTH(nativeRecorderState)(JNIEnv *env, jobject) {
    jlong values[3] = {0, 0, 0};
    if (gEngine) {
        synth::Recorder &r = gEngine->recorder();
        values[0] = r.isRecording();
        values[1] = r.isOpen();
        values[2] = r.recordedFrames();
    }
    jlongArray result = env->NewLongArray(3);
    env->SetLongArrayRegion(result, 0, 3, values);
    return result;
}

// MARK: - ARP/SEQ

// Steps cross JNI packed into one int each: kind | note << 8 | velocity << 16.

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeSequencerSettings)(JNIEnv *, jobject, jint mode, jboolean playing, jboolean hold,
                                                             jdouble bpm, jint division, jfloat gate, jint order,
                                                             jint extraOctaves, jint pattern) {
    if (!gEngine) return;
    synth::SequencerSettings s;
    s.mode = synth::SequencerMode(mode);
    s.playing = playing;
    s.hold = hold;
    s.bpm = bpm;
    s.division = synth::StepDivision(division);
    s.gate = gate;
    s.order = synth::ArpOrder(order);
    s.extraOctaves = extraOctaves;
    s.pattern = pattern;
    gEngine->sequencer().postSettings(s);
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeSequencerRestOrTie)(JNIEnv *, jobject) {
    if (gEngine) gEngine->sequencer().postRestOrTie();
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeSequencerClearPattern)(JNIEnv *, jobject, jint pattern) {
    if (gEngine) gEngine->sequencer().postClearPattern(pattern);
}

JNIEXPORT void JNICALL NATIVE_SYNTH(nativeSequencerSetPattern)(JNIEnv *env, jobject, jint pattern, jintArray packed) {
    if (!gEngine) return;
    std::array<synth::SequencerStep, synth::SequencerBridge::kMaxSteps> steps{};
    const int length = std::min<int>(env->GetArrayLength(packed), synth::SequencerBridge::kMaxSteps);
    std::array<jint, synth::SequencerBridge::kMaxSteps> values{};
    env->GetIntArrayRegion(packed, 0, length, values.data());
    for (int i = 0; i < length; ++i) {
        steps[i] = {synth::SequencerStep::Kind(std::min(values[i] & 0xFF, 2)), uint8_t((values[i] >> 8) & 0x7F),
                    uint8_t((values[i] >> 16) & 0x7F)};
    }
    gEngine->sequencer().postPattern(pattern, steps.data(), length);
}

JNIEXPORT jintArray JNICALL NATIVE_SYNTH(nativeSequencerPattern)(JNIEnv *env, jobject, jint pattern) {
    std::array<synth::SequencerStep, synth::SequencerBridge::kMaxSteps> steps{};
    std::array<jint, synth::SequencerBridge::kMaxSteps> values{};
    const int length = gEngine ? gEngine->sequencer().readPattern(pattern, steps.data(), int(steps.size())) : 0;
    for (int i = 0; i < length; ++i) values[i] = steps[i].kind | steps[i].note << 8 | steps[i].velocity << 16;
    jintArray result = env->NewIntArray(length);
    env->SetIntArrayRegion(result, 0, length, values.data());
    return result;
}

/// {currentStep, running (0/1), clockTicks}
JNIEXPORT jintArray JNICALL NATIVE_SYNTH(nativeSequencerState)(JNIEnv *env, jobject) {
    jint values[3] = {-1, 0, 0};
    if (gEngine) {
        synth::SequencerBridge &s = gEngine->sequencer();
        values[0] = s.currentStep();
        values[1] = s.isRunning();
        values[2] = jint(s.clockTicks());
    }
    jintArray result = env->NewIntArray(3);
    env->SetIntArrayRegion(result, 0, 3, values);
    return result;
}

JNIEXPORT jint JNICALL NATIVE_SYNTH(nativeSampleRate)(JNIEnv *, jobject) {
    return gEngine ? jint(gEngine->sampleRate()) : 48000;
}

} // extern "C"
