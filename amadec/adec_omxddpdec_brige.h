
#ifndef __ADEC_OMXDDPDEC_BRIGE_H__
#define __ADEC_OMXDDPDEC_BRIGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <dlfcn.h>

#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <adec_write.h>

typedef void (*fp_arm_omx_codec_init)(int,void*,int*);
typedef void (*fp_arm_omx_codec_read)(unsigned char *,unsigned *,int *);
typedef void (*fp_arm_omx_codec_close)();
typedef void (*fp_arm_omx_codec_start)();
typedef void (*fp_arm_omx_codec_pause)();
typedef int  (*fp_arm_omx_codec_get_declen)();
typedef int  (*fp_arm_omx_codec_get_FS)();
typedef int  (*fp_arm_omx_codec_get_Nch)();

#define OMX_ENABLE_CODEC_NULL  0
#define OMX_ENABLE_CODEC_AC3   1
#define OMX_ENABLE_CODEC_EAC3  2

extern int StageFrightCodecEnable;
extern fp_arm_omx_codec_init       parm_omx_codec_init;
extern fp_arm_omx_codec_read       parm_omx_codec_read ;
extern fp_arm_omx_codec_close      parm_omx_codec_close;
extern fp_arm_omx_codec_start      parm_omx_codec_start;
extern fp_arm_omx_codec_pause      parm_omx_codec_pause;
extern fp_arm_omx_codec_get_declen parm_omx_codec_get_declen;
extern fp_arm_omx_codec_get_FS     parm_omx_codec_get_FS;
extern fp_arm_omx_codec_get_Nch    parm_omx_codec_get_Nch;


extern unsigned long decode_offset;
extern buffer_stream_t *g_bst;
extern int pcm_cache_size;
extern int exit_decode_thread_success;
extern int exit_decode_thread;
extern AudioInfo g_AudioInfo;
extern int sn_threadid;
extern int sn_getpackage_threadid;
void stop_decode_thread_omx(aml_audio_dec_t *audec);





#endif

