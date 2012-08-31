/********************************************
 * name             : thread_read.c
 * function     : normal read to thread read.
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/



#include <pthread.h>
#include "thread_read.h"
#include <sys/types.h>
#include <unistd.h>
#include "source.h"
#include "slog.h"
#include <errno.h>
int thread_read_thread_run(unsigned long arg);
struct  thread_read *new_thread_read(const char *url, const char *headers, int flags) {
    pthread_t       tid;
    pthread_attr_t pthread_attr;
    struct  thread_read *thread;
    int ret;
    DTRACE();
    thread = malloc(sizeof(struct  thread_read));
    if (!thread) {
        return NULL;
    }
    memset(thread, 0, sizeof(*thread));
    DTRACE();
    thread->streambuf = streambuf_alloc(0);
    if (!thread->streambuf) {
        free(thread);
        return NULL;
    }
    DTRACE();
    pthread_attr_init(&pthread_attr);
    pthread_attr_setstacksize(&pthread_attr, 409600);   //default joinable
    pthread_mutex_init(&thread->pthread_mutex, NULL);
    pthread_cond_init(&thread->pthread_cond, NULL);
    thread->url = url;
    thread->flags = flags;
    thread->headers = headers;
    ret = pthread_create(&tid, &pthread_attr, (void*)&thread_read_thread_run, (void*)thread);
    thread->pthread_id = tid;
    return thread;
}
int thread_read_stop(struct  thread_read *thread)
{
    DTRACE();
    pthread_mutex_lock(&thread->pthread_mutex);
    thread->request_exit = 1;
    pthread_cond_signal(&thread->pthread_cond);
    pthread_mutex_unlock(&thread->pthread_mutex);
    return 0;
}

int  thread_read_readwait(struct  thread_read *thread, int microseconds)
{
    struct timespec pthread_ts;
    struct timeval now;
    int ret;

    gettimeofday(&now, NULL);
    pthread_ts.tv_sec = now.tv_sec + (microseconds + now.tv_usec) / 1000000;
    pthread_ts.tv_nsec = ((microseconds + now.tv_usec) * 1000) % 1000000000;
    pthread_mutex_lock(&thread->pthread_mutex);
    thread->onwaitingdata = 1;
    ret = pthread_cond_timedwait(&thread->pthread_cond, &thread->pthread_mutex, &pthread_ts);
    pthread_mutex_unlock(&thread->pthread_mutex);
    return ret;
}

int thread_read_writedatafinish(struct  thread_read *thread)
{
    int ret;

    pthread_mutex_lock(&thread->pthread_mutex);
    ret = pthread_cond_signal(&thread->pthread_cond);
    pthread_mutex_unlock(&thread->pthread_mutex);

    return ret;
}

int thread_read_release(struct  thread_read *thread)
{
    LOGI("thread_read_release\n");
    if (!thread->request_exit) {
        thread_read_stop(thread);
    }
    pthread_join(thread->pthread_id, NULL);
    LOGI("thread_read_release thread exited\n");
    if (thread->streambuf != NULL) {
        streambuf_release(thread->streambuf);
    }
    free(thread);
    LOGI("thread_read_release thread exited all\n");
    return 0;
}

int thread_read_read(struct  thread_read *thread, char * buf, int size)
{
    int ret = -1;
    int readlen = 0;
    int retrynum = 100;

    while (readlen == 0) {
        streambuf_dumpstates(thread->streambuf);
        ret = streambuf_read(thread->streambuf, buf + readlen, size - readlen);
        LOGI("---------thread_read_read=%d,size=%d\n", ret, size);
        streambuf_dumpstates(thread->streambuf);
        if (ret > 0) {
            readlen += ret;
        } else if (ret < 0) {
            break;
        } else if (ret == 0) {
            if (thread->fatal_error) {
                return thread->fatal_error;
            }
            if (thread->error < 0 && thread->error != -11) {
                ret = thread->error;
                break;
            }
            thread_read_readwait(thread, 10 * 1000);
        }
        if (retrynum <= 0) {
            break;
        }
    }
    return readlen > 0 ? readlen : ret;

}


int thread_read_seek(struct  thread_read *thread, int64_t off, int wherece)
{
    int ret;
    DTRACE();
    pthread_mutex_lock(&thread->pthread_mutex);
    ret = streambuf_seek(thread->streambuf, off, wherece);
    if (ret == off) {
        pthread_mutex_unlock(&thread->pthread_mutex);
        return ret;
    }
    ///do seek now.
    thread->request_seek = 1;
    thread->request_seek_offset = off;
    thread->inseeking = 1;
    pthread_mutex_unlock(&thread->pthread_mutex);
    return 0;
}
int thread_read_openstream(struct  thread_read *thread)
{
    thread->source = new_source(thread->url, thread->headers, thread->flags);
    if (thread->source != NULL) {
        int ret;
        ret = source_open(thread->source);
        if (ret != 0) {
            LOGI("source opened failed\n");
            release_source(thread->source);
            thread->source = NULL;
            thread->error = ret;
            return ret;
        }
        thread->opened = 1;
    }
    return 0;
}
int thread_read_seekstream(struct  thread_read *thread, int64_t offset)
{
    DTRACE();
    if (thread->source) {
        return source_seek(thread->source, offset, SEEK_SET);
    }
    return 0;
}
int thread_read_download(struct  thread_read *thread)
{
    int ret;
    bufheader_t *buf;
    int readsize = av_gettime() % 10240 + 1;
    if (readsize < 0) {
        readsize = -readsize;
    }
    buf = streambuf_getbuf(thread->streambuf, readsize);
    if (!buf) {
        LOGI("not enough bufffer's %d\n", readsize);
        streambuf_dumpstates(thread->streambuf);
        return -1;
    }
    buf->pos = source_seek(thread->source, 0, SEEK_CUR);;
    ret = source_read(thread->source, buf->pbuf, buf->bufsize);
    if (ret > 0) {
        LOGI("thread_read_download read data len=%d\n", ret);
        buf->bufdatalen = ret;
        streambuf_dumpstates(thread->streambuf);
        streambuf_buf_write(thread->streambuf, buf);
        streambuf_dumpstates(thread->streambuf);
        if (thread->onwaitingdata) {
            thread_read_writedatafinish(thread);
        }
    } else {
        LOGI("thread_read_download ERROR=%d\n", ret);
        thread->error = ret;
        if (thread->error == SOURCE_ERROR_EOF || ret == 0) {
            thread->error = SOURCE_ERROR_EOF;
        }
        if (ret != EAGAIN) {
            thread->fatal_error = ret;
        }
        streambuf_buf_free(thread->streambuf, buf);
        return ret;
    }
    return 0;
}

int thread_read_thread_run_l(struct  thread_read *thread)
{
    int ret = 0;
    if (!thread->opened) {
        ret = thread_read_openstream(thread);
        if (ret < 0) {
            thread->fatal_error = ret;
        }
    }
    if (thread->request_seek) {
        thread->inseeking = 1;
        ret = thread_read_seekstream(thread, thread->request_seek_offset);
        thread->inseeking = 0;
        thread->request_seek = 0;
    }
    if (thread->opened) {
        ret = thread_read_download(thread);
    }
    return ret;
}
int thread_read_thread_run(unsigned long arg)
{
    struct  thread_read *thread = (void *)arg;
    int ret;
    while (!thread->request_exit) {
        ret = thread_read_thread_run_l(thread);
        if (ret != 0) {
            if (thread->fatal_error < 0) {
                break;
            }
            usleep(10 * 1000); /*errors,wait now*/
        }
    }
    LOGI("thread_read_thread_run exit now\n");
    return 0;
}

