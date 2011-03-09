/**
 * \file adec-external-ctrl.h
 * \brief  Function prototypes of Audio Dec
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_EXTERNAL_H
#define ADEC_EXTERNAL_H

#ifdef  __cplusplus
extern "C"
{
#endif

    int audio_decode_init(void **priv);
    int audio_decode_start(void *priv);
    int audio_decode_pause(void *priv);
    int audio_decode_resume(void *priv);
    int audio_decode_stop(void *priv);
    int audio_decode_release(void *priv);
    int audio_decode_automute(void *, int);
    int audio_decode_set_mute(void *priv, int);
    int audio_decode_set_volume(void *, float);
    int audio_channels_swap(void *);
    int audio_channel_left_mono(void *);
    int audio_channel_right_mono(void *);
    int audio_channel_stereo(void *);
    int audio_output_muted(void *priv);
    int audio_dec_ready(void *priv);

#ifdef  __cplusplus
}
#endif

#endif
