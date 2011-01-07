#include <utils/RefBase.h>
#include <utils/KeyedVector.h>
#include <utils/threads.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>

extern "C" {
#include "feeder.h"
#include "adec.h"
#include "log.h"
#include "libavcodec/avcodec.h"
}

namespace android {

template <typename T> inline T min(T a, T b) {
    return a<b ? a : b;
}

static AudioTrack *track;
static char small_buf[128];
static char *small_buf_start;
static size_t small_buf_level;
static Mutex mLock;

static am_codec_struct *cur_codec = NULL;
extern "C" AVCodecContext* avcodecCtx;

extern "C" int adec_refresh_pts(void);
extern "C" am_codec_struct *get_codec_by_fmt(audio_format_t fmt);

void audioCallback(int event, void* user, void *info)
{
    size_t len, t, filled = 0;
    AudioTrack::Buffer *buffer = static_cast<AudioTrack::Buffer *>(info);
    adec_feeder_t *pfeeder = static_cast<adec_feeder_t *>(user);
    char *dst =NULL;/*buffer  is NULL if not AudioTrack::EVENT_MORE_DATA */

    if (event != AudioTrack::EVENT_MORE_DATA) {
	log_print(0, "audioCallback  event = %d \n", event);
        return;
    }
	
    if (buffer==NULL || buffer->size == 0) return;
        
    //log_print(0, "---------------- Before ---------------");
    adec_refresh_pts();
//log_print(0, "---------------- %x ---------------\n",buffer->i16);
    if(pfeeder->dsp_on){
        len = pfeeder->dsp_read((char *)buffer->i16, buffer->size);
        buffer->size = len;
    }else{
        if(cur_codec){
            len = cur_codec->decode_frame((char *)buffer->i16, buffer->size, NULL);
            buffer->size = len;
        }
    }

#if 0
	dst = (char *)buffer->i16;
    // DSP driver has the limitation to return at least 128 bytes

    // deal with cached data first
    len = min(small_buf_level, buffer->size);
    if (len != 0) {
        //memcpy(dst, small_buf_start, buffer->size);
        memcpy(dst, small_buf_start, len);
        small_buf_level -= len;
        small_buf_start += len;
        dst += len;
        filled = len;
    }

    if (buffer->size == len) return;

    t = buffer->size - filled;
    if (t < 128) {
        if (pfeeder->dsp_read(small_buf, 128) > 0) {
            memcpy(dst, small_buf, t);
            small_buf_start = &small_buf[t];
            small_buf_level = 128 - t;
            filled += t;
        }

        buffer->size = filled;
        return;
    }

    filled += pfeeder->dsp_read(dst, t);

    //log_print(0, "---------------- After ---------------");
    //log_print(0, "size = %d", filled);
    //adec_refresh_pts();

    buffer->size = filled;
if(buffer->size == 0)
	track->pause();
#endif
}

void small_buf_init(void)
{
    small_buf_start = small_buf;
    small_buf_level = 0;
}

extern "C" int android_init(adec_feeder_t * feed)
{
    status_t status;

    log_print(0, "android_init");

    small_buf_init();

    Mutex::Autolock _l(mLock);

if(!track)
    track = new AudioTrack();

    //if (track == 0 || feed==NULL || !feed->dsp_on || feed->dsp_read==NULL)
    if (track == 0 || feed==NULL) 
        return -1;

    log_print(0, "AudioTrack created");

    status = track->set(AudioSystem::MUSIC,
               feed->sample_rate,
               AudioSystem::PCM_16_BIT,
               (feed->channel_num == 1) ? AudioSystem::CHANNEL_OUT_MONO : AudioSystem::CHANNEL_OUT_STEREO,
               0,       // frameCount
               0,       // flags
               audioCallback,
               feed,    // user when callback
               0,       // notificationFrames
               0,       // shared buffer
               0);
	log_print(0, "AudioTrack set ok!");
    if (status != NO_ERROR) {
        log_print(0, "track->set returns %d", status);
        log_print(0, "feeder samplet %d", feed->sample_rate);
        log_print(0, "feeder channel_num %d", feed->channel_num);
    }

    if(!feed->dsp_on) {

        if(cur_codec != NULL && cur_codec->release != NULL){
            cur_codec->release();
            cur_codec = NULL;
        }

        cur_codec = get_codec_by_fmt(feed->format);        
        if(cur_codec == NULL){
            log_print(1, "No codec for this format found (fmt:%d)\n",feed->format);
        }else if(cur_codec->init!=NULL && cur_codec->init(feed, avcodecCtx)!=0) {
            log_print(1, "Codec init failed: name:%s, fmt:%d)\n",cur_codec->name, cur_codec->format);
            cur_codec = NULL;
        }else{
            cur_codec->used = 1;
			log_print(0, "get_codec_by_fmt ok: %s\n",cur_codec->name);
        }		
    }
	
#if 0
    if (track->initCheck() == NO_ERROR) {
        track->start();

        log_print(0, "AudioTrack initCheck OK and started.");

	track->setVolume(100.0, 100.0);
	log_print(0, "AudioTrack set Volume !\n");
	
        return 0;
    }

    log_print(0, "AudioTrack initCheck failed");

    delete track;
    track = 0;

    return -1;
#endif
    return 0;
}

extern "C" int android_start(void)
{
    Mutex::Autolock _l(mLock);

    if(!track) {
	log_print(0, "track not init !\n");
	return -1;
    }

    if (track->initCheck() == NO_ERROR) {
	track->start();
	log_print(0, "AudioTrack initCheck OK and started.");

	track->setVolume(100.0, 100.0);
	log_print(0, "AudioTrack set Volume !\n");

	return 0;
    }

    delete track;
    track = 0;

    return -1;
}

extern "C" int android_uninit(void)
{
    log_print(0, "AudioTrack free");
	
    Mutex::Autolock _l(mLock);

    if (track)
        track->stop();

    if(cur_codec != NULL && cur_codec->release !=NULL) {
        cur_codec->release();
        cur_codec = NULL;
    }

#if 0
    delete track;
    track = 0;
#endif

    return 0;
}

extern "C" int android_get_delay(void)
{
    if (track)
        return track->latency();

    return 0;
}

extern "C" void android_pause(void)
{
    Mutex::Autolock _l(mLock);

    if (track)
        return track->pause();
}

extern "C" void android_resume(void)
{
    Mutex::Autolock _l(mLock);
	
    if (track)
        return track->start();
}

extern "C" void android_mute(int e)
{
    Mutex::Autolock _l(mLock);

    if (track)
	 return track->mute(e);
}

}
