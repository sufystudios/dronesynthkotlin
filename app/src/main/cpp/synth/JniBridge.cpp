//
// JNI entry points for xyz.sufystudios.synthguide.audio.NativeSynth.
// One engine per process; every call is a no-op until create() has run.
//

#include <jni.h>

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

} // extern "C"
