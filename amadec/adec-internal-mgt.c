/**
 * \file adec-internal-mgt.c
 * \brief  Audio Dec Message Loop Thread
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <audio-dec.h>
#include <adec-pts-mgt.h>

/**
 * \brief set audio output mode.
 * \param cmd control commands
 * \return 0 on success otherwise -1
 */
static int audio_hardware_ctrl(hw_command_t cmd)
{
    int fd;

    fd = open(AUDIO_CTRL_DEVICE, O_RDONLY);
    if (fd < 0) {
        adec_print("Open Device %s Failed!", AUDIO_CTRL_DEVICE);
        return -1;
    }

    switch (cmd) {
    case HW_CHANNELS_SWAP:
        ioctl(fd, AMAUDIO_IOC_SET_CHANNEL_SWAP, 0);
        break;

    case HW_LEFT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_LEFT_MONO, 0);
        break;

    case HW_RIGHT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_RIGHT_MONO, 0);
        break;

    case HW_STEREO_MODE:
        ioctl(fd, AMAUDIO_IOC_SET_STEREO, 0);
        break;

    default:
        adec_print("Unknow Command %d!", cmd);
        break;

    };

    close(fd);

    return 0;

}

/**
 * \brief adec main thread
 * \param args pointer to thread private data
 * \return 0 on success otherwise -1 if an error occurred
 */
static void *adec_message_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    adec_cmd_t *msg = NULL;

    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;

    while (!audec->need_stop) {
        audec->state = INITING;

        ret = feeder_init(audec);
        if (ret == 0) {
            ret = aout_ops->init(audec);
            if (ret) {
                adec_print("Audio out device init failed!");
                continue;
            }
            audec->state = INITTED;
            break;
        }

        usleep(100000);
    }

    do {
        //if(message_pool_empty(audec))
        //{
        //adec_print("there is no message !\n");
        //  usleep(100000);
        //  continue;
        //}

        msg = adec_get_message(audec);
        if (!msg) {
            usleep(100000);
            continue;
        }

        switch (msg->ctrl_cmd) {
        case CMD_START:

            adec_print("Receive START Command!\n");
            if (audec->state == INITTED) {
                audec->state = ACTIVE;

                /*start  the  the pts scr,...*/
                adec_pts_start(audec);

                if (audec->auto_mute) {
                    avsync_en(0);
                    adec_pts_pause();

                    while ((!audec->need_stop) && track_switch_pts(audec)) {
                        usleep(1000);
                    }

                    avsync_en(1);
                    adec_pts_resume();

                    audec->auto_mute = 0;
                }

                aout_ops->start(audec);

            }
            break;

        case CMD_PAUSE:

            adec_print("Receive PAUSE Command!");
            if (audec->state == ACTIVE) {
                audec->state = PAUSED;
                adec_pts_pause();
                aout_ops->pause(audec);
            }
            break;

        case CMD_RESUME:

            adec_print("Receive RESUME Command!");
            if (audec->state == PAUSED) {
                audec->state = ACTIVE;
                aout_ops->resume(audec);
                adec_pts_resume();
            }
            break;

        case CMD_STOP:

            adec_print("Receive STOP Command!");
            if (audec->state > STOPPED) {
                audec->state = STOPPED;
                aout_ops->stop(audec);
            }
            break;

        case CMD_MUTE:

            adec_print("Receive Mute Command!");
            if (msg->has_arg) {
                if (aout_ops->mute) {
                    adec_print("%s the output !\n", ((msg->value.en) ? "mute" : "unmute"));
                    aout_ops->mute(audec, msg->value.en);
                    audec->muted = msg->value.en;
                }
            }
            break;

        case CMD_SET_VOL:

            adec_print("Receive Set Vol Command!");
            if (msg->has_arg) {
                if (aout_ops->set_volume) {
                    adec_print("set audio volume! vol = %f\n", msg->value.volume);
                    aout_ops->set_volume(audec, msg->value.volume);
                }
            }
            break;

        case CMD_CHANL_SWAP:

            adec_print("Receive Channels Swap Command!");
            audio_hardware_ctrl(HW_CHANNELS_SWAP);
            break;

        case CMD_LEFT_MONO:

            adec_print("Receive Left Mono Command!");
            audio_hardware_ctrl(HW_LEFT_CHANNEL_MONO);
            break;

        case CMD_RIGHT_MONO:

            adec_print("Receive Right Mono Command!");
            audio_hardware_ctrl(HW_RIGHT_CHANNEL_MONO);
            break;

        case CMD_STEREO:

            adec_print("Receive Stereo Command!");
            audio_hardware_ctrl(HW_STEREO_MODE);
            break;

        case CMD_RELEASE:

            adec_print("Receive RELEASE Command!");
            audec->state = TERMINATED;
            break;

        default:
            adec_print("Unknow Command!");
            break;

        }

        if (msg) {
            adec_message_free(msg);
            msg = NULL;
        }
    } while (audec->state != TERMINATED);

    adec_print("Exit Message Loop Thread!");
    pthread_exit(NULL);
    return NULL;
}

/**
 * \brief start audio dec
 * \param audec pointer to audec
 * \return 0 on success otherwise -1 if an error occurred
 */
int adec_init1(aml_audio_dec_t *audec)
{
    int ret = 0;
    pthread_t    tid;

    memset(audec, 0, sizeof(aml_audio_dec_t));

    adec_message_pool_init(audec);
    get_output_func(audec);
    audec->adsp_ops.dsp_file_fd = -1;

    ret = pthread_create(&tid, NULL, (void *)adec_message_loop, (void *)audec);
    if (ret != 0) {
        adec_print("Create adec main thread failed!\n");
        return ret;
    }
    adec_print("Create adec main thread success! tid = %d\n", tid);

    audec->thread_pid = tid;

    return ret;
}
