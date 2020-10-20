#include <jni.h>
#include <android/input.h>
#include "AudioEngine.h"
#include <android/log.h>



static AudioEngine *mainengine;

extern "C" {
jobject  obj;
JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_AEdelete(JNIEnv *env, jobject obj,jlong engineHandle) {
    delete reinterpret_cast<AudioEngine *>(engineHandle);

}
JNIEXPORT jlong JNICALL
Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1createEngine(
        JNIEnv *env,
        jclass /*unused*/
     ) {
    // We use std::nothrow so `new` returns a nullptr if the engine creation fails
 AudioEngine *engine = new(std::nothrow) AudioEngine();
 mainengine=engine;
    return reinterpret_cast<jlong>(engine);
}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1deleteEngine(
        JNIEnv *env,
        jclass,
        jlong engineHandle) {

    delete reinterpret_cast<AudioEngine *>(engineHandle);
}



JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1setDefaultSampleRate(JNIEnv *env,
                                                                                  jclass type,
                                                                                  jint sampleRate) {
    oboe::DefaultStreamValues::SampleRate = (int32_t) sampleRate;
}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1setDefaultFramesPerBurst(JNIEnv *env,
                                                                                      jclass type,
                                                                                      jint framesPerBurst) {

    oboe::DefaultStreamValues::FramesPerBurst = (int32_t) framesPerBurst;

}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_native_1setTube(JNIEnv *env,
jclass type,
        jint tube) {

mainengine->tubeon(tube);

}


JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_touchEvent(JNIEnv *env, jobject obj, jint action, jint note) {

    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:

                mainengine->setNote((int) note);
                mainengine->setToneOn(true);

            break;
        case 1:
            mainengine->setToneOn(false);
            break;
        case 2:
            mainengine->setToneOn(false);
            break;
//        case AMOTION_EVENT_ACTION_OUTSIDE:
//            audioEngine->setToneOn(false);
////            break;
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
JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_changeKnob(JNIEnv *env,jobject obj, jint knob, jint val) {

    mainengine->ChangeKnob(knob, val);

}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_changeKnobDouble(JNIEnv *env,jobject obj, jint knob, jdouble val) {

    mainengine->ChangeKnobDouble(knob, val);

}
JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_startEngine(JNIEnv *env, jobject /* this */) {
   // audioEngine->cre();
}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_stopEngine(JNIEnv *env, jobject /* this */) {

    mainengine->closeOutputStream();

}

JNIEXPORT void JNICALL
Java_xyz_sufystudios_didgsynth_MainActivity_native_1setToneOn(
        JNIEnv *env,
        jclass,
        jlong engineHandle,
        jboolean isToneOn) {
  //  __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
    AudioEngine *engine = reinterpret_cast<AudioEngine *>(engineHandle);
    if (engine == nullptr) {
        LOGE("Engine handle is invalid, call createHandle() to create a new one");
        return;
    }
    engine->setToneOn(isToneOn);

}

//JNIEXPORT void JNICALL
//Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1setAudioApi(
//        JNIEnv *env,
//        jclass type,
//        jlong engineHandle,
//        jint audioApi) {
//   // __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
//   AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
//    if (engine == nullptr) {
//        LOGE("Engine handle is invalid, call createHandle() to create a new one");
//        return;
//    }
//
//    oboe::AudioApi api = static_cast<oboe::AudioApi>(audioApi);
//    engine->setAudioApi(api);
//
//}

//JNIEXPORT void JNICALL
//Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1setAudioDeviceId(
//        JNIEnv *env,
//        jclass,
//        jlong engineHandle,
//        jint deviceId) {
//
//  AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
//    if (engine == nullptr) {
//        LOGE("Engine handle is invalid, call createHandle() to create a new one");
//        return;
//    }
//    engine->setDeviceId(deviceId);
//
//}

//JNIEXPORT void JNICALL
//Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1setChannelCount(
//        JNIEnv *env,
//        jclass type,
//        jlong engineHandle,
//        jint channelCount) {
//
//   AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
//    if (engine == nullptr) {
//        LOGE("Engine handle is invalid, call createHandle() to create a new one");
//        return;
//    }
//    engine->setChannelCount(channelCount);
//}

//JNIEXPORT void JNICALL
//Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1setBufferSizeInBursts(
//        JNIEnv *env,
//        jclass,
//        jlong engineHandle,
//        jint bufferSizeInBursts) {
//    //__android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
//
//   AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
//    if (engine == nullptr) {
//        LOGE("Engine handle is invalid, call createHandle() to create a new one");
//        return;
//    }
//
//    engine->setBufferSizeInBursts(bufferSizeInBursts);
//
//}


//JNIEXPORT jdouble JNICALL
//Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1getCurrentOutputLatencyMillis(
//        JNIEnv *env,
//        jclass,
//        jlong engineHandle) {
//    //__android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
//   AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
//    if (engine == nullptr) {
//        LOGE("Engine is null, you must call createEngine before calling this method");
//        return static_cast<jdouble>(-1.0);
//    }
//    __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
//return static_cast<jdouble>(engine->getCurrentOutputLatencyMillis());
//}

//JNIEXPORT jboolean JNICALL
//Java_xyz_sufystudios_didgsynth_PlaybackEngine_native_1isLatencyDetectionSupported(
//        JNIEnv *env,
//        jclass type,
//        jlong engineHandle) {
//  //  __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
//AudioEngine *engine = reinterpret_cast<AudioEngine*>(engineHandle);
//    if (engine == nullptr) {
//        LOGE("Engine is null, you must call createEngine before calling this method");
//        return JNI_FALSE;
//    }
//    __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","testing1");
//    return (engine->isLatencyDetectionSupported() ? JNI_TRUE : JNI_FALSE);
//}
}