#include <string.h>
#include <android/log.h>
#include "log.h"

#define  LOG_TAG    "amadec"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

__attribute__ ((format (printf, 2, 3)))
void log_print(const int level, const char *fmt, ...)
{
    char *buf = NULL;
    va_list ap;

    va_start(ap, fmt);
    vasprintf(&buf, fmt, ap);
    va_end(ap);

    if (buf) {
        LOGI("%s", buf);
        free(buf);
    }
}
