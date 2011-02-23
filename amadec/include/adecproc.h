#ifndef ADECPROC_H
#define ADECPROC_H

#ifdef  __cplusplus
extern "C"
{
#endif

        int audio_decode_start(void);
        int audio_decode_stop(void);
        int audio_decode_pause(void);
        int audio_decode_resume(void);
        int audio_decode_set_mute(int flag);
        int audio_decode_set_volume(void);
        int audio_decode_automute(int stat);
	 int audio_channels_swap(void);
	 int audio_channel_left_mono(void);
	 int audio_channel_right_mono(void);
	 int audio_channel_stereo(void);

#ifdef  __cplusplus
}
#endif

#endif /* ADECPROC_H */
