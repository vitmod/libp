/********************************************
 * name             : ffmpegsource.c
 * function     : ffmpeg source to our stream source,more for debug
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include "thread_read.h"
#include "source.h"
#include "slog.h"
static int ffmpegsource_open(source_t *as, const char * url, const char *header, int flags);
static int simple_try_switch_protol(source_t *as, char * buf, int datasize)
{
#define M3UTAG  "#EXTM3U"
    if (datasize > strlen(M3UTAG) && memcmp(M3UTAG, buf, strlen(M3UTAG)) == 0) {
        URLContext *uio = as->priv[0];
        snprintf(as->location, 4096, "list:%s", as->url);
        ffurl_close(uio);
        as->priv[0] = 0;
        LOGI("try reopend by M3u parser\n");
        ffmpegsource_open(as, as->location, as->header, as->flags);
        return 1;
    }
    return 0;
}


static int ffmpegsource_open(source_t *as, const char * url, const char *header, int flags)
{
    URLContext *uio;
    int ret;
    /*
    try read from ext register source...
    ....add later..
    */
    ret = ffurl_open_h(&uio, url, flags, url);
    if (ret != 0) {
        return ret;
    }
    as->flags = flags;
    as->priv[0] = (URLContext *)uio;
    if (url != as->location) {
        as->firstread = 1;
        strcpy(as->location, url);
    }
    return 0;
}
static int ffmpegsource_read(source_t *as, char *buf, int size)
{
    URLContext *uio;
    int ret = SOURCE_ERROR_NOT_OPENED;
retry_read:
    uio = as->priv[0];
    if (uio != NULL) {
        ret = ffurl_read(uio, buf, size);
    }
    if (ret > 0 && as->firstread) {
        as->firstread = 0;
        if (simple_try_switch_protol(as, buf, ret)) {
            goto retry_read;
        }
    }
    if (ret == 0) {
        ret = SOURCE_ERROR_EOF;
    }
    return ret;
}
static int64_t ffmpegsource_seek(source_t *as, int64_t off, int whence)
{
    URLContext *uio = as->priv[0];
    int64_t ret = SOURCE_ERROR_NOT_OPENED;
    if (uio != NULL) {
        ret = ffurl_seek(uio, off, whence);
    }
    if (ret < 0) {
        ret = SOURCE_ERROR_SEEK_FAILED;
    }
    return ret;
}
static int ffmpegsource_close(source_t *as)
{
    int ret = 0;
    URLContext *uio = as->priv[0];
    ret = ffurl_close(uio);
    return ret;
}
static int ffmpegsource_supporturl(source_t *as, const char * url, const char *header, int flags)
{
    return 100;
}
sourceprot_t ffmpeg_source = {
    .name                = "ffmpegsource",
    .open            = ffmpegsource_open,
    .read            = ffmpegsource_read,
    .seek            = ffmpegsource_seek,
    .close           = ffmpegsource_close,
    .supporturl    = ffmpegsource_supporturl,
};

