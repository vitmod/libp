/********************************************
 * name             : streambufqueue.c
 * function     : streambufqueue manager,manage the data buf,old databuf and free bufs.
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/


#include <errno.h>
#include "streambufqueue.h"
#include "slog.h"
#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif
#define NEWDATA_MAX (64*1024*1024)
#define OLDDATA_MIN (2*1024*1024)

streambufqueue_t * streambuf_alloc(int flags)
{
    streambufqueue_t *s;
    s = malloc(sizeof(streambufqueue_t));
    if (!s) {
        return NULL;
    }
    memset(s, 0, sizeof(*s));
    queue_init(&s->newdata, 0);
    queue_init(&s->oldqueue, 0);
    queue_init(&s->freequeue, 0);
    lp_lock_init(&s->lock, NULL);
    return s;
}
int streambuf_once_read(streambufqueue_t *s, char *buffer, int size)
{
    bufqueue_t *qnew = &s->newdata;
    bufqueue_t *qold = &s->oldqueue;
    bufheader_t *buf;
    int len;
    int ret;
    int bufdataelselen;

    lp_lock(&s->lock);
    buf = queue_bufpeek(qnew);
    if (!buf) {
        if (s->eof) {
            ret = STREAM_EOF;/**/
        } else if (s->errorno) {
            ret = s->errorno;
        } else {
            ret = 0;
        }
        goto endout;
    }
    bufdataelselen = buf->bufdatalen - (buf->data_start - buf->pbuf);
    len = MIN(bufdataelselen, size);
    memcpy(buffer, buf->data_start, len);
    ret = len;
    if (len < bufdataelselen) {
        queue_bufpeeked_partdatasize(qnew, buf, len);
    } else {
        buf = queue_bufget(qnew);
        buf->data_start = buf->pbuf;
        queue_bufpush(qold, buf);
    }
endout:
    lp_unlock(&s->lock);
    return ret;
}

int streambuf_read(streambufqueue_t *s, char *buffer, int size)
{
    char *pbuf = buffer;
    int ret;
    int rlen = 0;
    ret = streambuf_once_read(s, buffer, size);
    if (ret > 0) {
        rlen += ret;
    }
    while (ret > 0 && (size - rlen) > 0) {
        ret = streambuf_once_read(s, buffer + rlen, size - rlen);
        if (ret > 0) {
            rlen += ret;
        }
    }
    return rlen > 0 ? rlen : ret;
}
bufheader_t *streambuf_getbuf(streambufqueue_t *s, int size)
{
    bufheader_t *buf;
    lp_lock(&s->lock);
    buf = queue_bufget(&s->freequeue);
    if (buf == NULL && queue_bufdatasize(&s->oldqueue) > OLDDATA_MIN) {
        buf = queue_bufget(&s->oldqueue);/*if we have enough old data,try get from old buf*/
    }
    if (buf) {
        if (buf->bufsize < size) {
            queue_bufrealloc(buf, size);
        }
    } else {
        buf = queue_bufalloc(size);
        if (!buf) {
            LOGE("streambuf_getbuf queue_bufalloc size=%d\n", size);
            goto endout;
        }
    }
    buf->data_start = buf->pbuf;
    buf->timestampe = -1;
    buf->pos = -1;
    buf->bufdatalen = 0;
    buf->flags = 0;
    INIT_LIST_HEAD(&buf->list);
endout:
    lp_unlock(&s->lock);
    return buf;
}
int streambuf_buf_write(streambufqueue_t *s, bufheader_t *buf)
{
    lp_lock(&s->lock);
    queue_bufpush(&s->newdata, buf);
    lp_unlock(&s->lock);
    return 0;
}
int streambuf_buf_free(streambufqueue_t *s, bufheader_t *buf)
{
    lp_lock(&s->lock);
    queue_bufpush(&s->freequeue, buf);
    lp_unlock(&s->lock);
    return 0;
}

int streambuf_write(streambufqueue_t *s, char *buffer, int size, int timestamps)
{
    bufheader_t *buf;
    int twritelen = size;
    int len;
    while (twritelen > 0) {
        buf = streambuf_getbuf(s, size);
        if (buf == NULL) {
            if (twritelen != size) {
                break;    //have writen some data  before.
            }
            return -1;
        }
        if (buf->bufsize < twritelen) {
            len = buf->bufsize;
        } else {
            len = size;
        }
        buf->data_start = buf->pbuf;
        memcpy(buf->data_start, buffer, len);
        buf->bufdatalen = len;
        if (timestamps >= 0) {
            buf->timestampe = timestamps;
        }
        streambuf_buf_write(s, buf);
        twritelen -= len;
    }
    return (size > twritelen) ? (size - twritelen) : -1;
}

int streambuf_seek(streambufqueue_t *s, int off, int whence)
{
    return -1;
}

int streambuf_reset(streambufqueue_t *s)
{
    bufqueue_t *q1, *q2;
    bufheader_t *buf;
    lp_lock(&s->lock);
    q1 = &s->newdata;
    q2 = &s->oldqueue;
    buf = queue_bufget(q1);
    while (buf != NULL) {
        queue_bufpush(q2, buf);
        buf = queue_bufget(q1);
    }
    q1 = &s->oldqueue;
    q2 = &s->freequeue;
    buf = queue_bufget(q1);
    while (buf != NULL) {
        queue_bufpush(q2, buf);
        buf = queue_bufget(q1);
    }
    lp_unlock(&s->lock);
    return -1;
}

int streambuf_release(streambufqueue_t *s)
{
    lp_lock(&s->lock);
    queue_free(&s->oldqueue);
    queue_free(&s->newdata);
    queue_free(&s->freequeue);
    lp_unlock(&s->lock);
    free(s);
    return 0;
}
int64_t streambuf_bufpos(streambufqueue_t *s)
{
    int64_t p1;
    bufheader_t *buf;
    lp_lock(&s->lock);
    p1 = queue_bufstartpos(&s->newdata);
    buf = queue_bufpeek(&s->newdata);
    if (buf) {
        p1 += buf->data_start - buf->pbuf;
    }
    lp_unlock(&s->lock);
    return p1;
}
int streambuf_dumpstates(streambufqueue_t *s)
{
    int s1, s2;
    int64_t p1, p2;
    lp_lock(&s->lock);
    s1 = queue_bufdatasize(&s->newdata);
    s2 = queue_bufdatasize(&s->oldqueue);
    p1 = queue_bufstartpos(&s->newdata);
    p2 = queue_bufstartpos(&s->oldqueue);
    lp_unlock(&s->lock);
    LOGI("streambuf states:,new data pos=%lld, size=%d ,old datapos=%lld,size=%d\n", p1, s1, p2, s2);
    return 0;
}

