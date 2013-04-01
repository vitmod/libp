#ifndef CURL_LOG_H_
#define CURL_LOG_H_

#undef LOGI
#undef LOGE
#undef LOGV

#ifdef ANDROID
#include <android/log.h>
#ifndef LOG_TAG
#define LOG_TAG "curl-mod"
#endif
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(f,s...) fprintf(stderr,f,##s)
#define LOGE(f,s...) fprintf(stderr,f,##s)
#define LOGV(f,s...) fprintf(stderr,f,##s)
#endif

#endif