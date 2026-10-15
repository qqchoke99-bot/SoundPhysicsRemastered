#pragma once
#include <android/log.h>
#define SPP_TAG "SPP-Bedrock"
#define SPP_LOGI(...) __android_log_print(ANDROID_LOG_INFO, SPP_TAG, __VA_ARGS__)
#define SPP_LOGW(...) __android_log_print(ANDROID_LOG_WARN, SPP_TAG, __VA_ARGS__)
#define SPP_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SPP_TAG, __VA_ARGS__)
