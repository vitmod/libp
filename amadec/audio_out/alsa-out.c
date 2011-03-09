/**
 * \file alsa-out.c
 * \brief  Functions of Auduo output control for Linux Platform
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <config.h>
#include <alsa/asoundlib.h>

#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <log-print.h>


#define USE_INTERPOLATION

static snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);

static snd_pcm_t *handle;
//static snd_pcm_hw_params_t *params;
static struct {
    snd_pcm_format_t format;
    unsigned int channels;
    unsigned int rate;
    int oversample;
    int realchanl;
    int flag;
} hwparams;
static size_t bits_per_sample, bits_per_frame;
static size_t chunk_bytes;
static int buffer_size;
static int fragcount = 32;
static snd_pcm_uframes_t chunk_size = 1024;
static char output_buffer[64 * 1024];

#ifdef USE_INTERPOLATION
static int pass1_history[8][8];
#pragma align_to(64,pass1_history)
static int pass2_history[8][8];
#pragma align_to(64,pass2_history)
static short pass1_interpolation_output[0x4000];
#pragma align_to(64,pass1_interpolation_output)
static short interpolation_output[0x8000];
#pragma align_to(64,interpolation_output)

static inline short CLIPTOSHORT(int x)
{
    short res;
#if 0
    __asm__ __volatile__(
        "min  r0, %1, 0x7fff\r\n"
        "max r0, r0, -0x8000\r\n"
        "mov %0, r0\r\n"
        :"=r"(res)
        :"r"(x)
        :"r0"
    );
#else
    if (x > 0x7fff) {
        res = 0x7fff;
    } else if (x < -0x8000) {
        res = -0x8000;
    } else {
        res = x;
    }
#endif
    return res;
}

static void pcm_interpolation(int interpolation, unsigned num_channel, unsigned num_sample, short *samples)
{
    int i, k, l, ch;
    int *s;
    short *d;
    for (ch = 0; ch < num_channel; ch++) {
        s = pass1_history[ch];
        if (interpolation < 2) {
            d = interpolation_output;
        } else {
            d = pass1_interpolation_output;
        }
        for (i = 0, k = l = ch; i < num_sample; i++, k += num_channel) {
            s[0] = s[1];
            s[1] = s[2];
            s[2] = s[3];
            s[3] = s[4];
            s[4] = s[5];
            s[5] = samples[k];
            d[l] = s[2];
            l += num_channel;
            d[l] = CLIPTOSHORT((150 * (s[2] + s[3]) - 25 * (s[1] + s[4]) + 3 * (s[0] + s[5]) + 128) >> 8);
            l += num_channel;
        }
        if (interpolation >= 2) {
            s = pass2_history[ch];
            d = interpolation_output;
            for (i = 0, k = l = ch; i < num_sample * 2; i++, k += num_channel) {
                s[0] = s[1];
                s[1] = s[2];
                s[2] = s[3];
                s[3] = s[4];
                s[4] = s[5];
                s[5] = pass1_interpolation_output[k];
                d[l] = s[2];
                l += num_channel;
                d[l] = CLIPTOSHORT((150 * (s[2] + s[3]) - 25 * (s[1] + s[4]) + 3 * (s[0] + s[5]) + 128) >> 8);
                l += num_channel;
            }
        }
    }
}
#endif


static int set_params(void)
{
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *swparams;
    //  snd_pcm_uframes_t buffer_size;
    //  snd_pcm_uframes_t boundary;
    //  unsigned int period_time = 0;
    //  unsigned int buffer_time = 0;
    snd_pcm_uframes_t bufsize;
    int err;
    unsigned int rate;
    snd_pcm_uframes_t start_threshold, stop_threshold;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_sw_params_alloca(&swparams);

    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        adec_print("Broken configuration for this PCM: no configurations available");
        return err;
    }

    err = snd_pcm_hw_params_set_access(handle, params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        adec_print("Access type not available");
        return err;
    }

    err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
    if (err < 0) {
        adec_print("Sample format non available");
        return err;
    }

    err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
    if (err < 0) {
        adec_print("Channels count non available");
        return err;
    }

    rate = hwparams.rate;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &hwparams.rate, 0);
    assert(err >= 0);
#if 0
    err = snd_pcm_hw_params_get_buffer_time_max(params,  &buffer_time, 0);
    assert(err >= 0);
    if (buffer_time > 500000) {
        buffer_time = 500000;
    }

    period_time = buffer_time / 4;

    err = snd_pcm_hw_params_set_period_time_near(handle, params,
            &period_time, 0);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
            &buffer_time, 0);
    assert(err >= 0);

#endif
    bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
    //bits_per_frame = bits_per_sample * hwparams.realchanl;
    bits_per_frame = bits_per_sample * hwparams.channels;

    err = snd_pcm_hw_params_set_period_size_near(handle, params, &chunk_size, NULL);
    if (err < 0) {
        adec_print("Unable to set period size \n");
        return err;
    }

    //err = snd_pcm_hw_params_set_periods_near(handle, params, &fragcount, NULL);
    //if (err < 0) {
    //  adec_print("Unable to set periods \n");
    //  return err;
    //}

    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        adec_print("Unable to install hw params:");
        return err;
    }

    err = snd_pcm_hw_params_get_buffer_size(params, &bufsize);
    if (err < 0) {
        adec_print("Unable to get buffersize \n");
        return err;
    }
    buffer_size = bufsize * bits_per_frame / 8;

#if 1
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        adec_print("??Unable to get sw-parameters\n");
        return err;
    }

    //err = snd_pcm_sw_params_get_boundary(swparams, &boundary);
    //if (err < 0){
    //  adec_print("Unable to get boundary\n");
    //  return err;
    //}

    //err = snd_pcm_sw_params_set_start_threshold(handle, swparams, bufsize);
    //if (err < 0) {
    //  adec_print("Unable to set start threshold \n");
    //  return err;
    //}

    //err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, buffer_size);
    //if (err < 0) {
    //  adec_print("Unable to set stop threshold \n");
    //  return err;
    //}

    //  err = snd_pcm_sw_params_set_silence_size(handle, swparams, buffer_size);
    //  if (err < 0) {
    //      adec_print("Unable to set silence size \n");
    //      return err;
    //  }

    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        adec_print("Unable to get sw-parameters\n");
        return err;
    }

    //snd_pcm_sw_params_free(swparams);
#endif


    //chunk_bytes = chunk_size * bits_per_frame / 8;

    return 0;
}

/**
 * \brief output initialization
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_init(struct aml_audio_dec* audec)
{
    adec_print("alsa out init");

    char *pcm_name = "default";
    snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
    int err;

    if (feed->sample_rate >= (88200 + 96000) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = -1;
        hwparams.rate = 48000;
    } else if (feed->sample_rate >= (64000 + 88200) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = -1;
        hwparams.rate = 44100;
    } else if (feed->sample_rate >= (48000 + 64000) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = -1;
        hwparams.rate = 32000;
    } else if (feed->sample_rate >= (44100 + 48000) / 2) {
        hwparams.oversample = 0;
        hwparams.rate = 48000;
        if (feed->channel_num == 1) {
            hwparams.flag = 1;
        } else if (feed->channel_num == 2) {
            hwparams.flag = 0;
        }
    } else if (feed->sample_rate >= (32000 + 44100) / 2) {
        hwparams.oversample = 0;
        hwparams.rate = 44100;
        if (feed->channel_num == 1) {
            hwparams.flag = 1;
        } else if (feed->channel_num == 2) {
            hwparams.flag = 0;
        }
    } else if (feed->sample_rate >= (24000 + 32000) / 2) {
        hwparams.oversample = 0;
        hwparams.rate = 32000;
        if (feed->channel_num == 1) {
            hwparams.flag = 1;
        } else if (feed->channel_num == 2) {
            hwparams.flag = 0;
        }
    } else if (feed->sample_rate >= (22050 + 24000) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = 1;
        hwparams.rate = 48000;
    } else if (feed->sample_rate >= (16000 + 22050) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = 1;
        hwparams.rate = 44100;
    } else if (feed->sample_rate >= (12000 + 16000) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = 1;
        hwparams.rate = 32000;
    } else if (feed->sample_rate >= (11025 + 12000) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = 2;
        hwparams.rate = 48000;
    } else if (feed->sample_rate >= (8000 + 11025) / 2) {
        hwparams.flag = 1;
        hwparams.oversample = 2;
        hwparams.rate = 44100;
    } else {
        hwparams.flag = 1;
        hwparams.oversample = 2;
        hwparams.rate = 32000;
    }

#if 0
    switch (feed->sample_rate) {
    case 96000:
    case 88200:
    case 64000:
        hwparams.flag = 1;
        hwparams.oversample = -1;
        hwparams.rate = feed->sample_rate >> 1;
        break;

    case 22050:
    case 16000:
        hwparams.flag = 1;
        hwparams.oversample = 1;
        hwparams.rate = feed->sample_rate << 1;
        break;

    case 11025:
    case 8000:
        hwparams.flag = 1;
        hwparams.oversample = 2;
        hwparams.rate = feed->sample_rate << 2;
        break;

    case 32000:
    case 44100:
    case 48000:
    default:
        hwparams.oversample = 0;
        hwparams.rate = feed->sample_rate;
        if (feed->channel_num == 1) {
            hwparams.flag = 1;
        } else if (feed->channel_num == 2) {
            hwparams.flag = 0;
        }
        break;
    }
#endif
    hwparams.channels = 2;
    hwparams.realchanl = feed->channel_num;
    //hwparams.rate = feed->sample_rate;
    hwparams.format = SND_PCM_FORMAT_S16_LE;
#ifdef USE_INTERPOLATION
    memset(pass1_history, 0, 64 * sizeof(int));
    memset(pass2_history, 0, 64 * sizeof(int));
#endif
    err = snd_pcm_open(&handle, pcm_name, stream, 0);
    if (err < 0) {
        printf("audio open error: %s", snd_strerror(err));
        return -1;
    }


    readi_func = snd_pcm_readi;
    writei_func = snd_pcm_writei;
    readn_func = snd_pcm_readn;
    writen_func = snd_pcm_writen;

    set_params();

    /*TODO:  create play thread */

    return 0;
}

/**
 * \brief start output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 *
 * Call android_start(), then the callback will start being called.
 */
int alsa_start(struct aml_audio_dec* audec)
{
    /* TODO: start the play thread */
    return 0;
}

/**
 * \brief pause output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_pause(struct aml_audio_dec* audec)
{
    int res;

    while ((res = snd_pcm_pause(handle, 1)) == -EAGAIN) {
        sleep(1);
    }

    return res;
}

/**
 * \brief resume output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_resume(struct aml_audio_dec* audec)
{
    int res;

    while ((res = snd_pcm_pause(handle, 0)) == -EAGAIN) {
        sleep(1);
    }

    return res;
}

/**
 * \brief stop output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_stop(struct aml_audio_dec* audec)
{
    if (handle) {
        //suspend();
        snd_pcm_drop(handle);
        snd_pcm_close(handle);
        handle = NULL;
        return 0;
    }
    return -1;
}

/**
 * \brief get output latency in ms
 * \param audec pointer to audec
 * \return output latency
 */
unsigned long alsa_latency(struct aml_audio_dec* audec)
{
    int buffered_data;
    int sample_num;
    buffered_data = buffer_size - alsa_get_space();
    sample_num = buffered_data / (hwparams.channels * (bits_per_sample / 8)); /*16/2*/
    return (sample_num * (1000000 / hwparams.rate));
}