#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <player.h>
#include <string.h>

#include <pthread.h>
#include "player_priv.h"
#include  <libavformat/avio.h>

int ffmpeg_lock(void **pmutex, enum AVLockOp op)
{
    int r = 0;
    pthread_mutex_t *mutex = *pmutex;
    switch (op) {
    case AV_LOCK_CREATE:  ///< Create a mutex
        mutex = MALLOC(sizeof(pthread_mutex_t));
        if (mutex == NULL) {
            return -1;
        }
        r = pthread_mutex_init(mutex, NULL);
        if (r != 0) {
            FREE(mutex);
            mutex = NULL;
        }
        *pmutex = mutex;
        break;
    case AV_LOCK_OBTAIN:  ///< Lock the mutex
        r = pthread_mutex_lock(mutex);
        break;
    case AV_LOCK_RELEASE: ///< Unlock the mutex
        r = pthread_mutex_unlock(mutex);
        break;
    case AV_LOCK_DESTROY: ///< Free mutex resources
        if (mutex) {
            FREE(mutex);
        }
        *pmutex = NULL;
        break;
    }
    return r;
}

static int ffmpeg_interrupt_callback(void)
{
    return 1;
}
void ffmpeg_interrupt(void)
{
    url_set_interrupt_cb(ffmpeg_interrupt_callback);
}
void ffmpeg_uninterrupt(void)
{
    url_set_interrupt_cb(NULL);
}
int ffmpeg_init(void)
{
    av_register_all();
    av_lockmgr_register(ffmpeg_lock);
    return 0;
}
int ffmpeg_buffering_data(play_para_t *para)
{
    if (para && para->pFormatCtx && para->pFormatCtx->pb) {
        return url_buffering_data(para->pFormatCtx->pb, 0);
    } else {
        return -1;
    }
}
int ffmpeg_close_file(play_para_t *am_p)
{
    AVFormatContext *pFCtx = am_p->pFormatCtx;
    if (pFCtx) {
        av_close_input_file(pFCtx);
    }
    am_p->pFormatCtx = NULL;
    return 0;
}
int ffmpeg_open_file(play_para_t *am_p)
{
    AVFormatContext *pFCtx ;
    int ret = -1;
    int byteiosize = FILE_BUFFER_SIZE;
    // Open video file
    if (am_p == NULL) {
        log_print("[ffmpeg_open_file] Empty pointer!\n");
        return FFMPEG_EMP_POINTER;
    }
    if (am_p->byteiobufsize > 0) {
        byteiosize = am_p->byteiobufsize;
    }
    if (am_p->file_name != NULL) {
Retry_open:
        ret = av_open_input_file(&pFCtx, am_p->file_name, NULL, byteiosize, NULL, am_p->start_param ? am_p->start_param->headers : NULL);
        if (ret != 0) {
            if (ret == AVERROR(EAGAIN)) {
                goto  Retry_open;
            }
            log_print("ffmpeg error: Couldn't open input file! ret==%x\n", ret);
            return FFMPEG_OPEN_FAILED; // Couldn't open file
        }
        am_p->pFormatCtx = pFCtx;

        return FFMPEG_SUCCESS;
    } else {
        log_print("not assigned a file to play\n");
        return FFMPEG_NO_FILE;
    }
}
int ffmpeg_parse_file_type(play_para_t *am_p, player_file_type_t *type)
{
    AVFormatContext *pFCtx = am_p->pFormatCtx;
    memset(type, 0, sizeof(*type));
    if (pFCtx->iformat != NULL) {
        int i;
        type->fmt_string = pFCtx->iformat->name;
        for (i = 0; i < pFCtx->nb_streams; i++) {
            AVStream *st = pFCtx->streams[i];
            if (st->codec->codec_type == CODEC_TYPE_VIDEO) {
                type->video_tracks++;
            } else if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
                type->audio_tracks++;
            } else if (st->codec->codec_type == CODEC_TYPE_SUBTITLE) {
                type->subtitle_tracks++;
            }
        }
    }
    return 0;

}

int ffmpeg_parse_file(play_para_t *am_p)
{
    AVFormatContext *pFCtx = am_p->pFormatCtx;
    int ret = -1;
    // Open video file
    ret = av_find_stream_info(pFCtx);
    if (ret < 0) {
        log_print("ERROR:Couldn't find stream information, ret=====%d\n", ret);
        return FFMPEG_PARSE_FAILED; // Couldn't find stream information
    }
    return FFMPEG_SUCCESS;
}


