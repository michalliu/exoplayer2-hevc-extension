#ifndef INCLUDE_OSK_LOG_H_
#define INCLUDE_OSK_LOG_H_

#include <stdio.h>

#ifdef __ANDROID__

#include <android/log.h>

#define OSK_LOG_UNKNOWN     ANDROID_LOG_UNKNOWN
#define OSK_LOG_DEFAULT     ANDROID_LOG_DEFAULT

#define OSK_LOG_VERBOSE     ANDROID_LOG_VERBOSE
#define OSK_LOG_DEBUG       ANDROID_LOG_DEBUG
#define OSK_LOG_INFO        ANDROID_LOG_INFO
#define OSK_LOG_WARN        ANDROID_LOG_WARN
#define OSK_LOG_ERROR       ANDROID_LOG_ERROR
#define OSK_LOG_FATAL       ANDROID_LOG_FATAL
#define OSK_LOG_SILENT      ANDROID_LOG_SILENT

#define VLOG(level, TAG, ...)    ((void)__android_log_vprint(level, TAG, __VA_ARGS__))
#define ALOG(level, TAG, ...)    ((void)__android_log_print(level, TAG, __VA_ARGS__))

#else

#define OSK_LOG_UNKNOWN     0
#define OSK_LOG_DEFAULT     1

#define OSK_LOG_VERBOSE     2
#define OSK_LOG_DEBUG       3
#define OSK_LOG_INFO        4
#define OSK_LOG_WARN        5
#define OSK_LOG_ERROR       6
#define OSK_LOG_FATAL       7
#define OSK_LOG_SILENT      8

#define VLOG(level, TAG, ...)    ((void)vprintf(__VA_ARGS__))
#define ALOG(level, TAG, ...)    ((void)printf(__VA_ARGS__))

#endif

#define OSK_LOG_TAG "OskHvc"

#define VLOGV(...)  VLOG(OSK_LOG_VERBOSE,   OSK_LOG_TAG, __VA_ARGS__)
#define VLOGD(...)  VLOG(OSK_LOG_DEBUG,     OSK_LOG_TAG, __VA_ARGS__)
#define VLOGI(...)  VLOG(OSK_LOG_INFO,      OSK_LOG_TAG, __VA_ARGS__)
#define VLOGW(...)  VLOG(OSK_LOG_WARN,      OSK_LOG_TAG, __VA_ARGS__)
#define VLOGE(...)  VLOG(OSK_LOG_ERROR,     OSK_LOG_TAG, __VA_ARGS__)

#define ALOGV(...)  ALOG(OSK_LOG_VERBOSE,   OSK_LOG_TAG, __VA_ARGS__)
#define ALOGD(...)  ALOG(OSK_LOG_DEBUG,     OSK_LOG_TAG, __VA_ARGS__)
#define ALOGI(...)  ALOG(OSK_LOG_INFO,      OSK_LOG_TAG, __VA_ARGS__)
#define ALOGW(...)  ALOG(OSK_LOG_WARN,      OSK_LOG_TAG, __VA_ARGS__)
#define ALOGE(...)  ALOG(OSK_LOG_ERROR,     OSK_LOG_TAG, __VA_ARGS__)
#define LOG_ALWAYS_FATAL(...)   do { ALOGE(__VA_ARGS__); exit(1); } while (0)

#endif