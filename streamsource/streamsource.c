/********************************************
 * name             : streamsource.c
 * function     : streamsource to ffmpeg.
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/


#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include "thread_read.h"
#include "slog.h"
#include "libavutil/opt.h"
struct streamsource {
    URLContext *h;
    int is_muti_thread;
    struct  thread_read *tread;
};
#define NAMEPREF    "amss"
static const AVOption options[] = {{NULL}};
static const AVClass streamsource_class = {
    .class_name     = "AmlStreamSource",
    .item_name      = av_default_item_name,
    .option         = options,
    .version        = LIBAVUTIL_VERSION_INT,
};


static int streamsource_open(URLContext *h, const char *filename, int flags)
{
    struct streamsource *ss = h->priv_data;
    ss->tread = new_thread_read(filename + strlen(NAMEPREF) + 1, h->headers, flags); /*del $NAMEPREF+":"*/
    if (ss->tread == NULL) {

        return -1;
    }
    h->is_streamed = 1;
    h->is_slowmedia = 1;
    return 0;
}
/* standard file protocol */

static int streamsource_read(URLContext *h, unsigned char *buf, int size)
{
    struct streamsource *ss = h->priv_data;
    int ret;
    ret = thread_read_read(ss->tread, buf, size);
    if (ret == SOURCE_ERROR_EOF) {
        ret = AVERROR_EOF;
    }
    return ret;
}


static int streamsource_check(URLContext *h, int mask)
{
    return 0;
}

static int64_t streamsource_seek(URLContext *h, int64_t pos, int whence)
{
    struct streamsource *ss = h->priv_data;
    DTRACE();
    return -1;
    return thread_read_seek(ss->tread, pos, whence);
}

static int streamsource_close(URLContext *h)
{
    struct streamsource *ss = h->priv_data;
    thread_read_stop(ss->tread);
    thread_read_release(ss->tread);
    DTRACE();
    return 0;
}

URLProtocol streamsource_protocol = {
    .name                = "amss",
    .url_open            = streamsource_open,
    .url_read            = streamsource_read,
    .url_write           = NULL,
    .url_seek            = streamsource_seek,
    .url_close           = streamsource_close,
    .url_get_file_handle = NULL,
    .url_check           = NULL,
    .priv_data_size      = sizeof(struct streamsource),
    .priv_data_class     = &streamsource_class,
};

int streamsource_init()
{
    ffurl_register_protocol(&streamsource_protocol, sizeof(streamsource_protocol));
    int source_init_all(void);
    source_init_all();
    return 0;
}

