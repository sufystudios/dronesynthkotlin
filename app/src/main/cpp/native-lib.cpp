#include <jni.h>
#include <android/input.h>
#include "AudioEngine.h"
#include <android/log.h>
#include <thread>
#include <pthread.h>

static AudioEngine *mainengine;

extern "C" {
jobject  obj;
JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_MainActivity_AEdelete(JNIEnv *env, jobject obj,jlong engineHandle) {
auto engin=reinterpret_cast<AudioEngine *>(engineHandle);
engin->closeOutputStream();
delete engin;

}
JNIEXPORT jlong
extern "C" jlong
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1createEngine(
        JNIEnv *env,
        jclass /*unused*/
) {
    // We use std::nothrow so `new` returns a nullptr if the engine creation fails
    AudioEngine *engine = new(std::nothrow) AudioEngine();
    mainengine=engine;
    return reinterpret_cast<jlong>(engine);
}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1deleteEngine(
        JNIEnv *env,
jclass) {
auto engin=reinterpret_cast<AudioEngine *>(mainengine);
engin->stream->requestStop();
delete engin;

}



JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1setDefaultSampleRate(JNIEnv *env,
jclass type,
        jint sampleRate) {
oboe::DefaultStreamValues::SampleRate = (int32_t) sampleRate;
}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1setDefaultFramesPerBurst(JNIEnv *env,
jclass type,
        jint framesPerBurst) {

oboe::DefaultStreamValues::FramesPerBurst = (int32_t) framesPerBurst;

}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_MainActivity_native_1setTube(JNIEnv *env,
jclass type,
        jint tube) {

mainengine->tubeon(tube);

}


JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_MainActivity_touchEvent(JNIEnv *env, jobject obj, jint action, jint note) {
auto engin=reinterpret_cast<AudioEngine *>(mainengine);
switch (action) {
case AMOTION_EVENT_ACTION_DOWN:
case 1:

engin->setNote((int) note);
engin->setToneOn(true);

break;

engin->setNote((int) note);
break;
case 2:
case 3:

engin->setToneOn(false);
break;
//        case AMOTION_EVENT_ACTION_OUTSIDE:
//            audioEngine->setToneOn(false);

//        case AMOTION_EVENT_ACTION_HOVER_EXIT:
//            audioEngine->setToneOn(false);
//            break;
//        case AMOTION_EVENT_ACTION_HOVER_ENTER:
//            audioEngine->setNote((int) note);
//            audioEngine->setToneOn(true);
//            break;
//        case AMOTION_EVENT_ACTION_MOVE:
//            if(note==0) {
//                audioEngine->setToneOn(false);
//            }
//            break;

default:


break;

}

}
struct knobval {
    int knob;
    int val;
};
void *doThread(void *input) {
    auto engin = reinterpret_cast<AudioEngine *>(mainengine);
    engin->ChangeKnob(((struct knobval*)input)->knob,((struct knobval*)input)->val);
    //  delete input;
}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_MainActivity_changeKnob(JNIEnv *env,jobject obj, jint knob, jint val) {
mainengine->ChangeKnob(knob,val);
}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_MainActivity_changeKnobDouble(JNIEnv *env,jobject obj, jint knob, jdouble val) {


mainengine->ChangeKnobDouble(knob, val);

}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1startEngine(JNIEnv *env, jclass clazz) {
auto engin=reinterpret_cast<AudioEngine *>(mainengine);\
    engin->restart();
}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1stopEngine(JNIEnv *env, jclass clazz) {
auto engin=reinterpret_cast<AudioEngine *>(mainengine);

engin->stop();



}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_MainActivity_native_1setToneOn(
        JNIEnv *env,
jclass,
jlong engineHandle,
        jboolean isToneOn) {
//  __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
AudioEngine *engine = reinterpret_cast<AudioEngine *>(engineHandle);
if (engine == nullptr) {
LOGE("Engine handle is invalid, call createHandle() t    inline double sgn(double xn)\n"
     "    {\n"
     "        return (xn > 0) - (xn < 0);\n"
     "    }\n"
     "    inline double atanWaveShaper(double xn, double saturation)\n"
     "    {\n"
     "        return atan(saturation*xn) / atan(saturation);\n"
     "    }\n"
     "    inline double softClipWaveShaper(double xn, double saturation)\n"
     "    {\n"
     "        // --- un-normalized soft clipper from Reiss book\n"
     "        double d = sgn(xn);\n"
     "        return sgn(xn)*(1.0 - exp(-fabs(saturation*xn)));\n"
     "    o create a new one");
return;
}
engine->setToneOn(isToneOn);

}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1setAudioApi(
        JNIEnv *env,
jclass type,
        jlong engineHandle,
jint audioApi) {
// __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
if (engine == nullptr) {
LOGE("Engine handle is invalid, call createHandle() to create a new one");
return;
}

oboe::AudioApi api = static_cast<oboe::AudioApi>(audioApi);
engine->setAudioApi(api);

}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1setAudioDeviceId(
        JNIEnv *env,
jclass,
jlong engineHandle,
        jint deviceId) {

AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
if (engine == nullptr) {
LOGE("Engine handle is invalid, call createHandle() to create a new one");
return;
}
engine->setDeviceId(deviceId);

}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1setChannelCount(
        JNIEnv *env,
jclass type,
        jlong engineHandle,
jint channelCount) {

AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
if (engine == nullptr) {
LOGE("Engine handle is invalid, call createHandle() to create a new one");
return;
}
engine->setChannelCount(channelCount);
}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1setBufferSizeInBursts(
        JNIEnv *env,
jclass,
jlong engineHandle,
        jint bufferSizeInBursts) {
//__android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");

AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
if (engine == nullptr) {
LOGE("Engine handle is invalid, call createHandle() to create a new one");
return;
}

engine->setBufferSizeInBursts(bufferSizeInBursts);

}


JNIEXPORT jdouble JNICALL
        Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1getCurrentOutputLatencyMillis(
        JNIEnv *env,
        jclass,
        jlong engineHandle) {
//__android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
if (engine == nullptr) {
LOGE("Engine is null, you must call createEngine before calling this method");
return static_cast<jdouble>(-1.0);
}
__android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
return static_cast<jdouble>(engine->getCurrentOutputLatencyMillis());
}

JNIEXPORT jboolean JNICALL
        Java_xyz_sufystudios_synthguide_PlaybackEngine_native_1isLatencyDetectionSupported(
        JNIEnv *env,
        jclass type,
jlong engineHandle) {
//  __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
if (engine == nullptr) {
LOGE("Engine is null, you must call createEngine before calling this method");
return JNI_FALSE;
}
__android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
return (engine->isLatencyDetectionSupported() ? JNI_TRUE : JNI_FALSE);
}
}