#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>

#include "log.h"
#include "./include/adecproc.h"
#include "adec.h"

enum {
    ADEC_STAT_RELEASED = 0,
    ADEC_STAT_IDLE,
    ADEC_STAT_RUN,
    ADEC_STAT_PAUSE,
};

static int adec_stat = ADEC_STAT_IDLE;
pthread_mutex_t decode_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Start audio decode.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 * - 0: success.
 */
int audio_decode_start(void)
{
    pthread_mutex_lock(&decode_mutex);
    if (adec_stat == ADEC_STAT_IDLE) {
        if (adec_start() == 0)
            adec_stat = ADEC_STAT_RUN;
    }
    pthread_mutex_unlock(&decode_mutex);
    return 0;
}

/* Stop audio decode.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 * - 0: success.
 */
int audio_decode_stop(void)
{
    int status;

    status = audio_init_thread_check();
    if (!status)
        audio_init_thread_join();

    pthread_mutex_lock(&decode_mutex);
    if ((adec_stat == ADEC_STAT_RUN) || (adec_stat == ADEC_STAT_PAUSE)) {
        if (adec_stat == ADEC_STAT_PAUSE) {
            adec_resume();
        }
        adec_stop();
    }
    adec_stat = ADEC_STAT_IDLE;
    pthread_mutex_unlock(&decode_mutex);
    return 0;
}

/* Pause audio decode.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 * - 0: success.
 */
int audio_decode_pause(void)
{
    pthread_mutex_lock(&decode_mutex);
    if (adec_stat == ADEC_STAT_RUN)
        adec_pause();
    adec_stat = ADEC_STAT_PAUSE;
    pthread_mutex_unlock(&decode_mutex);
    return 0;
}

/* Resume audio decode from pause state.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 * - 0: success.
 */
int audio_decode_resume(void)
{
    pthread_mutex_lock(&decode_mutex);
    if (adec_stat == ADEC_STAT_PAUSE) {
        adec_resume();
        adec_stat = ADEC_STAT_RUN;
    }
    pthread_mutex_unlock(&decode_mutex);
    return 0;
}

/* Set Auto-Mute state in audio decode..
 *
 * Parameters:
 *  - 1: set auto-mute state.
 *  - 0: clear auto-mute state.
 *
 * Returned value:
 * - 0: success.
 */
int audio_decode_automute(int stat)
{
    adec_auto_mute(stat);
    return 0;
}

/* Mute audio output.
 *
 * Parameters:
 *  - 1: mute output.
 *  - 0: unmute output.
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
int audio_decode_set_mute(int flag)
{
    return 0;
}

/* Seta udio volume.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
int audio_decode_set_volume(void)
{
    return 0;
}

/* Swap audio left and right channels.
 *
 * Parameters:
 *  none.
 *  
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
 int audio_channels_swap(void)
{
    int ret;
	
    ret = set_audio_hardware(HW_CHANNELS_SWAP);
    return ret;
}

/* Output Left Channel.
 *
 * Parameters:
 *  none.
 *  
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
 int audio_channel_left_mono(void)
{
    int ret;
	
    ret = set_audio_hardware(HW_LEFT_CHANNEL_MONO);
    return ret;
}

/* Output Right Channel.
 *
 * Parameters:
 *  none.
 *  
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
 int audio_channel_right_mono(void)
{
    int ret;
	
    ret = set_audio_hardware(HW_RIGHT_CHANNEL_MONO);
    return ret;
}

/* Output Left And Right Channels.
 *
 * Parameters:
 *  none.
 *  
 *
 * Returned value:
 *  - -1: failed.
 *  - 0: success.
 */
 int audio_channel_stereo(void)
{
    int ret;
	
    ret = set_audio_hardware(HW_STEREO_MODE);
    return ret;
}