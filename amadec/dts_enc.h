#ifndef DTS_ENC_H
#define DTS_ENC_H
#include <pthread.h>

typedef struct {
    dtsenc_state_t  state;
    pthread_t       thread_pid;
    bool dts_flag;
}dtsenc_info_t;

typedef enum {
    IDLE,
    TERMINATED,
    STOPPED,
    INITTED,
    ACTIVE,
    PAUSED,
} dtsenc_state_t;

int dtsenc_init();
int dtsenc_start();
int dtsenc_pause();
int dtsenc_resume();
int dtsenc_stop();
int dtsenc_release();

#endif

