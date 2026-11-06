//
// Logging macros, previously taken from Oboe's internal src/common/OboeDebug.h.
//

#ifndef DRONESYNTH_LOGGING_H
#define DRONESYNTH_LOGGING_H

#if defined(__ANDROID__)
#include <android/log.h>

#define MODULE_NAME "DroneSynth"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, MODULE_NAME, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, MODULE_NAME, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, MODULE_NAME, __VA_ARGS__)
#else
// Desktop plugin builds: the engine never logs from the audio thread, and a plugin
// writing to the host's stderr helps nobody, so these compile to nothing.
#define LOGV(...) ((void)0)
#define LOGD(...) ((void)0)
#define LOGI(...) ((void)0)
#define LOGW(...) ((void)0)
#define LOGE(...) ((void)0)
#define LOGF(...) ((void)0)
#endif

#endif //DRONESYNTH_LOGGING_H
