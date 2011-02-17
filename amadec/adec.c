#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>

#include "log.h"
#include "adec.h"
#include "feeder.h"

#include "android_out.h"
#include "audiodsp_ctl.h"
#include "sound_ctrl.h"
#include "libavcodec/avcodec.h"

//#define DUMP_ORIGINAL_DATA //
#define DUMP_TO_FILE //
#define TEST_PCM_IN
#define TSYNC_PCRSCR    "/sys/class/tsync/pts_pcrscr"
#define TSYNC_EVENT     "/sys/class/tsync/event"
#define TSYNC_APTS      "/sys/class/tsync/pts_audio"

#define abs(x) ({                               \
                long __x = (x);                 \
                (__x < 0) ? -__x : __x;         \
                })

#define SYSTIME_CORRECTION_THRESHOLD    (90000/10)
#define APTS_DISCONTINUE_THRESHOLD      (90000*3)
#define REFRESH_PTS_TIME_MS (1000/10)

static adec_out_t android_device = {
    .init               = android_init,
    .start             = android_start,
    .uninit             = android_uninit,
    .get_delay	        = android_get_delay,
    .pause              = android_pause,
    .resume             = android_resume,
    .mute			= android_mute,
};

adec_out_t *get_android_device(void)
{
    return &android_device;
}

static void *audio_decode_init(void *args);

static unsigned long last_pts_valid;
static int first_audio_frame = 0;
static int adec_automute_cmd = 0;
static int adec_automute_stat = 0;
int stop_flag = 0;
static adec_out_t *cur_out=NULL;

static pthread_t init_thread_id;

int pause_flag = 0;

AVCodecContext* avcodecCtx = NULL;

extern int audio_codec_init();

static int audio_init_thread(void)
{
    int ret;
    //pthread_attr_t attr;
    //pthread_attr_init(&attr);

    ret = pthread_create(&init_thread_id, NULL, audio_decode_init, NULL);
    return ret;
}

int adec_start()
{
    first_audio_frame = 1;
    if (adec_automute_cmd) {
        adec_automute_stat = 1;
        android_device.mute(1);
    }
    stop_flag = 0;

    audio_init_thread();
    log_print(LOG_INFO, "adec_started\n");
    return 0;
}

unsigned long adec_get_pts(void)
{
    adec_feeder_t  *pfeeder=get_default_adec_feeder();
    unsigned long pts,delay_pts;
    int fd;

    if (pfeeder==NULL) {
        log_print(LOG_INFO, "pfeeder is NULL !\n");
        return -1;
    }
    pts=pfeeder->get_cur_pts(pfeeder);
    if (pts==-1) {
        log_print(LOG_INFO, "get get_cur_pts failed\n");
        return -1;
    }

    //log_print(LOG_INFO, "audiodsp pts = %d", pts);

    //if((pfeeder->format == AUDIO_FORMAT_COOK)||(pfeeder->format == AUDIO_FORMAT_RAAC))
    //		return pts;

    if (cur_out==NULL || cur_out->get_delay==NULL) {
        log_print(LOG_INFO, "cur_out is NULL!\n ");
        return -1;
    }

    delay_pts = cur_out->get_delay() * 90;

    //log_print(LOG_INFO, "delay_pts = %d", delay_pts);

    if (delay_pts < pts)
        pts-=delay_pts;
    else
        pts=0;

    return pts;
}

int adec_pts_start(void)
{
    unsigned long pts =0;
    char *file;
    int fd;
    char buf[64];

    memset(buf, 0, sizeof(buf));
    log_print(LOG_INFO, "adec_pts_start");

    last_pts_valid = 0;
    pts=adec_get_pts();

    if (pts==-1) {

        log_print(LOG_INFO, "pts==-1");

        file=TSYNC_APTS;
        if ((fd = open(file, O_RDONLY)) < 0) {
            log_print(LOG_ERR, "unable to open file %s,err: %s",file, strerror(errno));
            return -1;
        }

        read(fd, buf, sizeof(buf));
        close(fd);
        if (sscanf(buf, "0x%lx", &pts) < 1) {
            log_print(LOG_ERR, "unable to get apts from: %s", buf);
            return -1;
        }
    }

    log_print(LOG_INFO, "audio pts start from 0x%lx", pts);

    file=TSYNC_EVENT;
    if ((fd = open(file, O_WRONLY)) < 0) {
        log_print(LOG_ERR, "unable to open file %s,err: %s",file, strerror(errno));
        return -1;
    }
    sprintf(buf,"AUDIO_START:0x%lx",pts);
    write(fd,buf,strlen(buf));
    close(fd);
    return 0;
}
int adec_pts_resume(void)
{
    char *file;
    int fd;
    char buf[64];

    memset(buf, 0, sizeof(buf));
    log_print(LOG_INFO, "adec_pts_resume");
    file=TSYNC_EVENT;
    if ((fd = open(file, O_WRONLY)) < 0) {
        log_print(LOG_ERR, "unable to open file %s,err: %s",file, strerror(errno));
        return -1;
    }
    sprintf(buf,"AUDIO_RESUME");
    write(fd,buf,strlen(buf));
    close(fd);
    return 0;
}
int adec_pts_pause(void)
{
    char *file;
    int fd;
    char buf[64];
    file=TSYNC_EVENT;
    if ((fd = open(file, O_WRONLY)) < 0) {
        log_print(LOG_ERR, "unable to open file %s,err: %s",file, strerror(errno));
        return -1;
    };
    sprintf(buf,"AUDIO_PAUSE");
    write(fd,buf,strlen(buf));
    close(fd);
    return 0;
}

int adec_refresh_pts(void)
{
    unsigned long pts;
    unsigned long systime;
    static unsigned long last_pts=-1;
    char *file;
    int fd;
    char buf[64];
    if (pause_flag)
        return 0;
    /* get system time */
    memset(buf, 0, sizeof(buf));
    file=TSYNC_PCRSCR;
    if ((fd = open(file, O_RDWR)) < 0) {
        log_print(LOG_ERR, "unable to open file %s,err: %s",file, strerror(errno));
        return -1;
    }

    read(fd,buf,sizeof(buf));
    close(fd);

    if (sscanf(buf,"0x%lx",&systime) < 1) {
        log_print(LOG_ERR, "unable to getsystime %s", buf);
        close(fd);
        return -1;
    }
    /* get audio time stamp */

    pts=adec_get_pts();
    if (pts==-1 || last_pts==pts) {
        close(fd);
        if (pts == -1)
            return -1;
    }

    //log_print(LOG_INFO, "adec_get_pts() pts=%x\n",pts);

    if ((abs(pts-last_pts) > APTS_DISCONTINUE_THRESHOLD) && (last_pts_valid)) {
        /* report audio time interruption */
        log_print(LOG_INFO, "pts=%lx,last pts=%lx\n",pts,last_pts);
        file = TSYNC_EVENT;
        if ((fd = open(file, O_RDWR)) < 0) {
            log_print(LOG_ERR, "unable to open file %s,err: %s", file, strerror(errno));
            return -1;
        }
        adec_feeder_t *feeder=get_default_adec_feeder();
        log_print(LOG_INFO, "audio time interrupt: 0x%lx->0x%lx, 0x%lx\n", last_pts, pts, abs(pts-last_pts));

        sprintf(buf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx",pts);
        write(fd,buf,strlen(buf));
        close(fd);

        last_pts = pts;
        last_pts_valid = 1;

        return 0;
    }

    last_pts=pts;
    last_pts_valid = 1;

    if (abs(pts-systime) < SYSTIME_CORRECTION_THRESHOLD) {
        return 0;
    }

    /* report apts-system time difference */
    file = TSYNC_APTS;
    if ((fd = open(file, O_RDWR)) < 0) {
        log_print(LOG_ERR, "unable to open file %s,err: %s", file, strerror(errno));
        return -1;
    }

    log_print(LOG_INFO, "report apts as %ld,system pts=%ld, difference= %ld\n",
              pts,systime, pts-systime);

    sprintf(buf,"0x%lx",pts);
    write(fd,buf,strlen(buf));
    close(fd);

    return 0;
}

void adec_stop()
{
    first_audio_frame = 0;
    stop_flag = 1;
    if (cur_out && cur_out->uninit) {
        cur_out->uninit();
        cur_out = NULL;
    }

    feeder_release();

    log_print(LOG_INFO,"adec  stoped\n ");
};

void adec_reset()
{
#if 0
    if (cur_out && cur_out->reset) {
        cur_out->reset();
    }
#endif
}

void adec_pause()
{
    log_print(LOG_INFO,"adec  pause\n ");
    pause_flag = 1;
    if (cur_out && cur_out->pause) {
        adec_pts_pause();
        cur_out->pause();
    }
};

void adec_resume()
{
    pause_flag = 0;
    if (cur_out && cur_out->resume) {
        cur_out->resume();
        //log_print(LOG_INFO, "adec_pts_resume\n");
        adec_pts_resume();
        //adec_pts_start();
    }
};

void adec_auto_mute(int auto_mute)
{
    adec_automute_cmd = 1;
}

static void *audio_decode_init(void *args)
{
    adec_feeder_t  *pfeeder=NULL; /*before init it is NULL*/
    adec_out_t *pcur_out=NULL;
    am_codec_struct *pcur_codec=NULL;

    log_print(LOG_INFO,"audio decode init !\n");

    while (!stop_flag) {

        if (avcodecCtx && avcodecCtx->codec_id == CODEC_ID_AAC && avcodecCtx->profile == FF_PROFILE_AAC_MAIN) {
            log_print(LOG_INFO,"[%s,%d]audio is FF_PROFILE_AAC_MAIN, use ffmpeg decoder\n", __func__, __LINE__);
            pfeeder = feeder_init(0);
        } else {
            log_print(LOG_INFO, "[%s,%d]use dsp decoder\n", __func__, __LINE__);
            pfeeder = feeder_init(1);
        }

        if (pfeeder) {
            log_print(LOG_INFO, "Audio Samplerate is %d\n",pfeeder->sample_rate);
            pcur_out=get_android_device();
            log_print(LOG_INFO, "Audio out type is Android !\n");
            if (pcur_out==NULL) {
                log_print(LOG_ERR,"No oss device found\n");
                pcur_out=NULL;
            } else if ( pcur_out->init(pfeeder)!=0) {
                log_print(LOG_ERR, "Audio out device init failed\n");
                pcur_out=NULL;
            }

            cur_out = pcur_out;
            break;
        }
        usleep(100000);
    }

    //if((!stop_flag)&&cur_out && (pfeeder->dsp_on)){
    if ((!stop_flag)&&cur_out) {

        /*start  the  the pts scr,...*/
        adec_pts_start();

        if (adec_automute_cmd) {
            avsync_control(0);
            adec_pts_pause();

            while ((!stop_flag) && track_switch_pts())
                usleep(1000);

            avsync_control(1);
            adec_pts_resume();
            adec_automute_cmd = 0;
        }

        /*start AudioTrack*/
        android_device.start();

        if (pause_flag) {
            adec_pause();
        }

    }

    pthread_exit(0);
    return NULL;
}

/* Check audio init thread exit or not.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 *  - 1: audio init thread has exited.
 *  - 0: audio init thread has not exited.
 */
int audio_init_thread_check(void)
{
    int status;

    if (init_thread_id == 0)
        return 1;

    status = pthread_kill(init_thread_id, 0);

    if (status == ESRCH) {
        log_print(LOG_INFO,"audio init thread has exited!\n");
        return 1;
    } else if (status == EINVAL) {
        log_print(LOG_INFO,"Invalid sig , you must check it !\n");
        return 1;
    } else {
        log_print(LOG_INFO,"audio init thread has not exited!\n");
        return 0;
    }

}

/* Waiting audio init thread to exit.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 *  none.
 */
void audio_init_thread_join(void)
{
    stop_flag = 1;
    pthread_join(init_thread_id, NULL);
}

/* Check audio decode stop or not.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 *  - 1: audio decode stopped.
 *  - 0: audio decode not stopped.
 */
int decode_stopped(void)
{
    return stop_flag;
}

int adec_init(void * arg)
{
    avcodecCtx = (AVCodecContext *)arg;
    audio_codec_init();
    return 0;
}

/* Disable or Enable av sync.
 *
 * Parameters:
 * - 1: enable av sync.
 * - 0: disable av sync.
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
int avsync_control(int flag)
{
    int fd;
    char *path = "/sys/class/tsync/enable";
    char  bcmd[16];
    fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
    if (fd>=0) {
        sprintf(bcmd,"%d",flag);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }
    return -1;
}

/* When audio track switch occurred, use this function to judge audio should
 *   be played or not. If system time fall behind audio pts , and their difference
 *   is greater than SYSTIME_CORRECTION_THRESHOLD, auido should wait for
 *   video. Otherwise audio can be played.
 *
 * Parameters:
 *  none
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
int track_switch_pts(void)
{
    unsigned long vpts;
    unsigned long apts;
    unsigned long pcr;
    char buf[64];
    char *file;
    int fd = -1;

    memset(buf, 0, sizeof(buf));
    file=TSYNC_PCRSCR;
    if ((fd = open(file, O_RDWR)) < 0) {
        log_print(LOG_ERR, "unable to open file %s,err: %s",file, strerror(errno));
        return 1;
    }

    read(fd,buf,sizeof(buf));
    close(fd);

    if (sscanf(buf,"0x%lx",&pcr) < 1) {
        log_print(LOG_ERR, "unable to get pcr %s", buf);
        close(fd);
        return 1;
    }

    apts=adec_get_pts();
    if (apts==-1) {
        close(fd);
        log_print(LOG_ERR, "unable to get apts");
        return 1;
    }

    if (abs(apts-pcr) < SYSTIME_CORRECTION_THRESHOLD || (apts <= pcr))
        return 0;
    else
        return 1;

}
