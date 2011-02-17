#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "include/log.h"
#include "include/adec.h"

extern int register_wmapro_codec(void);
extern int register_aac_codec(void);

static am_codec_struct *audio_codec_list[AUDIO_FORMAT_MAX];

static int audio_codec_spool_init(void)
{
    int i;
    for (i=0;i<AUDIO_FORMAT_MAX;i++) {
        audio_codec_list[i]=NULL;
    }
    return 0;
}
int register_audio_codec(am_codec_struct *codec)
{
    am_codec_struct *icodec;
    if (!codec) {
        return -E_PTR;
    }
    if (!VALID_FMT(codec->format)) {
        log_print(LOG_ERR, "Register audio codec fmt unknow:name:%s,fmt:%d\n",codec->name ,codec->format);
        return -E_FORMAT;
    }
    if (audio_codec_list[codec->format]!=NULL) {
        log_print(LOG_ERR, "Register audio codec fmt have register,%s,%d\n", codec->name,codec->format);
        return -E_REGISTERED;
    }
    audio_codec_list[codec->format]=codec;
    //codec id;this can used for remove;
    //here we returnd the format as the codec id;
    //
    return codec->format;
}

int unregister_audio_codec(audio_format_t fmt)
{
    if (!VALID_FMT(fmt))
        return -1;
    if (audio_codec_list[fmt]==NULL)
        return -2;
    if (audio_codec_list[fmt]->used)
        return -3;//in use
    audio_codec_list[fmt]=NULL;
    return 0;
}


am_codec_struct *get_codec_by_fmt(audio_format_t fmt)
{
    return audio_codec_list[fmt];
}


int audio_codec_init()
{
    audio_codec_spool_init();
//	register_pcm_codec();
    register_wmapro_codec();
    register_aac_codec();
    return 0;
}

