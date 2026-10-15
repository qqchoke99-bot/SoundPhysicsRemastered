#pragma once
#include <android/log.h>
#define SPZ_TAG "SoundPhysiczRecreate"
#define SPZ_LOGI(...) __android_log_print(ANDROID_LOG_INFO, SPZ_TAG, __VA_ARGS__)
#define SPZ_LOGW(...) __android_log_print(ANDROID_LOG_WARN, SPZ_TAG, __VA_ARGS__)
#define SPZ_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SPZ_TAG, __VA_ARGS__)
