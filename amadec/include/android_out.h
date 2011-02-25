extern int android_init(adec_feeder_t *feed);
extern int android_start(void);
extern int android_uninit(void);
extern int android_get_delay(void);
extern void android_pause(void);
extern void android_resume(void);
extern void android_mute(int e);
extern void android_set_volume(float vol);
extern void android_basic_init();
