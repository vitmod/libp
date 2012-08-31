
#ifndef THREAD_READ_BUF_HHH__
#define  THREAD_READ_BUF_HHH__

#include "streambufqueue.h"
#include "source.h"
struct  thread_read {
    pthread_mutex_t  pthread_mutex;
    pthread_cond_t   pthread_cond;
    pthread_t        pthread_id;
    int thread_id;
    streambufqueue_t *streambuf;
    source_t *source;
    /**/
    int onwaitingdata;
    int request_exit;
    int request_seek;
    int64_t request_seek_offset;
    int inseeking;
    int opened;
    const char* url;
    const char* headers;
    int flags;
    int error;
    int fatal_error;
};



struct  thread_read *new_thread_read(const char *url, const char *headers, int flags) ;
int thread_read_thread_run(unsigned long arg);
int thread_read_stop(struct  thread_read *thread);
int thread_read_release(struct  thread_read *thread);
int thread_read_read(struct  thread_read *thread, char * buf, int size);
int thread_read_seek(struct  thread_read *thread, int64_t off, int where);
#endif

