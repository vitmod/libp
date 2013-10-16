/**
 * \file android-out.cpp
 * \brief  Functions of Auduo output control for Android Platform
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <utils/RefBase.h>
#include <utils/KeyedVector.h>
#include <utils/threads.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <media/AudioParameter.h>
#include <system/audio_policy.h>
#include <cutils/properties.h>

extern "C" {
#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <log-print.h>
#include "aml_resample.h"
#include <audiodsp_update_format.h>
}

namespace android
{

static Mutex mLock;
static int get_digitalraw_mode(void)
{
    int fd;
    int val = 0;
    char  bcmd[28];
    fd = open("/sys/class/audiodsp/digital_raw", O_RDONLY);
    if (fd >= 0) {
        read(fd, &bcmd, 28);
        close(fd);
    }
	else{
		adec_print("open DIGITAL_RAW_PATH failed\n");
		return 0;
	}
    val=bcmd[21]&0xf;
    return val;
    
}
void restore_system_samplerate()
{
#if defined(ANDROID_VERSION_JBMR2_UP)
    unsigned int sr = 0;
#else
    int sr = 0;
#endif 
	//TODO ,other output deivce routed ??
	AudioSystem::getOutputSamplingRate(&sr,AUDIO_STREAM_MUSIC);
	if(sr != 48000){
	audio_io_handle_t handle = -1;		
	handle = 	AudioSystem::getOutput(AUDIO_STREAM_MUSIC,
	                            48000,
	                            AUDIO_FORMAT_PCM_16_BIT,
	                            AUDIO_CHANNEL_OUT_STEREO,
#if defined(_VERSION_ICS) 
					AUDIO_POLICY_OUTPUT_FLAG_INDIRECT
#else	//JB...			
	                            AUDIO_OUTPUT_FLAG_PRIMARY
#endif	                            
	                            );
		if(handle > 0){
			char str[64];
			memset(str,0,sizeof(str));
			sprintf(str,"sampling_rate=%d",48000);
			AudioSystem::setParameters(handle, String8(str));			
		}		
	}
}

#if defined(ANDROID_VERSION_JBMR2_UP)
static size_t old_frame_count = 0;
#else
static int old_frame_count = 0;
#endif

void restore_system_framesize()
{
  adec_print("restore system frame size\n");
	int sr = 0;
	audio_io_handle_t handle = -1;		
	handle = 	AudioSystem::getOutput(AUDIO_STREAM_MUSIC,
	                            48000,
	                            AUDIO_FORMAT_PCM_16_BIT,
	                            AUDIO_CHANNEL_OUT_STEREO,
#if defined(_VERSION_ICS) 
					AUDIO_POLICY_OUTPUT_FLAG_INDIRECT
#else	//JB...			
	                            AUDIO_OUTPUT_FLAG_PRIMARY
#endif	                            
	                            );
		if(handle > 0){
			char str[64];
            int ret;
			memset(str,0,sizeof(str));
#if defined(ANDROID_VERSION_JBMR2_UP)
			sprintf(str,"frame_count=%zd",old_frame_count);
			ret = AudioSystem::setParameters(handle, String8(str));
			adec_print("restore frame success: %zd\n", old_frame_count);
#else
			sprintf(str,"frame_count=%zd",old_frame_count);
			ret = AudioSystem::setParameters(handle, String8(str));
			adec_print("restore frame success: %zd\n", old_frame_count);
#endif
        }
}		

void reset_system_samplerate(struct aml_audio_dec* audec)
{
	unsigned digital_raw = 0;
	digital_raw = get_digitalraw_mode();	
	if(!audec ||!digital_raw)
		return;
	/*
	1)32k,44k dts
	2) 32k,44k ac3 
	3)44.1k eac3 when hdmi passthrough
	4)32k,44k eac3 when spdif pasthrough 
	*/
	adec_print("format %d,sr %d \n",audec->format ,audec->samplerate );
	if( ((audec->format == ADEC_AUDIO_FORMAT_DTS||audec->format == ADEC_AUDIO_FORMAT_AC3) && \
		(audec->samplerate == 32000 || audec->samplerate == 44100)) ||  \
		(audec->format == ADEC_AUDIO_FORMAT_EAC3 && digital_raw == 2 && audec->samplerate == 44100) || \
		(audec->format == ADEC_AUDIO_FORMAT_EAC3 && digital_raw == 1 &&  \
		(audec->samplerate == 32000 || audec->samplerate == 44100)))
		
	{
		audio_io_handle_t handle = -1;
		int sr = 0;
		//AudioSystem::getOutputSamplingRate(&sr,AUDIO_STREAM_MUSIC);
		if(/*sr*/48000 != audec->samplerate){
			adec_print("change android system audio sr from %d to %d \n",sr,audec->samplerate);
			handle = 	AudioSystem::getOutput(AUDIO_STREAM_MUSIC,
	                                    48000,
	                                    AUDIO_FORMAT_PCM_16_BIT,
	                                    AUDIO_CHANNEL_OUT_STEREO,
#if defined(_VERSION_ICS) 
					AUDIO_POLICY_OUTPUT_FLAG_INDIRECT
#else	//JB...			
	                            AUDIO_OUTPUT_FLAG_PRIMARY
#endif	                            
	                                    );
			if(handle > 0){
				char str[64];
				memset(str,0,sizeof(str));
				sprintf(str,"sampling_rate=%d",audec->samplerate);
				AudioSystem::setParameters(handle, String8(str));			
			}
		}
		
       }
}

/**
 * \brief callback function invoked by android
 * \param event type of event notified
 * \param user pointer to context for use by the callback receiver
 * \param info pointer to optional parameter according to event type
 */

#define AMSTREAM_IOC_MAGIC 'S'
#define AMSTREAM_IOC_GET_LAST_CHECKIN_APTS   _IOR(AMSTREAM_IOC_MAGIC, 0xa9, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKIN_VPTS   _IOR(AMSTREAM_IOC_MAGIC, 0xaa, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKOUT_APTS  _IOR(AMSTREAM_IOC_MAGIC, 0xab, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKOUT_VPTS  _IOR(AMSTREAM_IOC_MAGIC, 0xac, unsigned long)
#define AMSTREAM_IOC_AB_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x09, unsigned long)

static unsigned long long ttt = 0;

static int resample = 0, last_resample = 0, resample_step = 0, last_step=0;
static int xxx;
static int diff_record[0x40], diff_wp = 0;
static int wfd_enable = 0, bytes_skipped = 0x7fffffff;

void audioCallback(int event, void* user, void *info)
{
    int len, i;
    unsigned last_checkin, last_checkout;
    unsigned diff, diff_avr;
    AudioTrack::Buffer *buffer = static_cast<AudioTrack::Buffer *>(info);
    aml_audio_dec_t *audec = static_cast<aml_audio_dec_t *>(user);
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;



    if (event != AudioTrack::EVENT_MORE_DATA) {
        adec_print(" ****************** audioCallback: event = %d \n", event);
        return;
    }

    if (buffer == NULL || buffer->size == 0) {
        adec_print("audioCallback: Wrong buffer\n");
        return;
    }
    
    if(wfd_enable){
        ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_GET_LAST_CHECKIN_APTS, (int)&last_checkin);
        last_checkout = audiodsp_get_pts(&audec->adsp_ops);
        if(last_checkin < last_checkout){
           diff = 0;
        }else{
           diff = (last_checkin-last_checkout)/90;
        }

        //ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_GET_LAST_CHECKOUT_APTS, (int)&last_checkout);
    }
    
    if(1){//!wfd_enable){
      adec_refresh_pts(audec);
    }

    if(wfd_enable){
      // filtering
      diff_record[diff_wp++] = diff;
      diff_wp = diff_wp & 0x3f;

      diff_avr = 0;
      for (i=0;i<0x40;i++) diff_avr+=diff_record[i];
      diff_avr = diff_avr / 0x40;    

      //if ((xxx++ % 30) == 0) 
      //  adec_print("audioCallback start: request %d, in: %d, out: %d, diff: %d, filtered: %d",buffer->size, last_checkin/90, last_checkout/90, diff, diff_avr);
      if(bytes_skipped == 0 && diff < 200){
        audiodsp_set_skip_bytes(&audec->adsp_ops, 0x7fffffff);
        bytes_skipped = 0x7fffffff;
      }

      if(diff >1000){ // too much data in audiobuffer,should be skipped
        audiodsp_set_skip_bytes(&audec->adsp_ops, 0);
        bytes_skipped = 0;
        adec_print("skip more data: last_checkin[%d]-last_checkout[%d]=%d, diff=%d\n", last_checkin/90, last_checkout/90, (last_checkin-last_checkout)/90, diff);
      }
      
      if (diff_avr > 220) {
        resample = 1; resample_step = 2;
      } else if (diff_avr<180) {
        // once we see a single shot of low boundry we finish down-sampling
        resample = 1; resample_step = -2;
      }else if(resample && (diff_avr < 200)){
        resample = 0;
      }


      if (last_resample != resample || last_step != resample_step) {
        last_resample = resample;
        last_step = resample_step;
        adec_print("resample changed to %d, step=%d, diff = %d", resample, resample_step,diff_avr);
      }
    }

    if (audec->adsp_ops.dsp_on) {
      if(wfd_enable){ 
        #ifdef ANDROID_VERSION_JBMR2_UP
        af_resample_api((char*)(buffer->i16), &buffer->size,track->channelCount(),audec, resample, resample_step);
        #else
        af_resample_api((char*)(buffer->i16), &buffer->size,buffer->channelCount,audec, resample, resample_step);
        #endif 
      }else{
        #ifdef ANDROID_VERSION_JBMR2_UP
        af_resample_api_normal((char*)(buffer->i16), &buffer->size, track->channelCount(), audec);
        #else
        af_resample_api_normal((char*)(buffer->i16), &buffer->size, buffer->channelCount, audec);
        #endif 
      }
     } else {
        adec_print("audioCallback: dsp not work!\n");
    }


    if(wfd_enable){
      if (buffer->size==0) {
        adec_print("no sample from DSP !!! in: %d, out: %d, diff: %d, filtered: %d", last_checkin/90, last_checkout/90, (last_checkin-last_checkout)/90, diff_avr);

      struct buf_status {
        int size;
        int data_len;
        int free_len;
        unsigned int read_pointer;
        unsigned int write_pointer;
      };

      struct am_io_param {
        int data;
        int len; //buffer size;
        struct buf_status status;
      };

      struct am_io_param am_io;

      ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&am_io);
      adec_print("ab_level=%x, ab_rd_ptr=%x", am_io.status.data_len, am_io.status.read_pointer);
      }
    }
    return;
}

/**
 * \brief output initialization
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_init(struct aml_audio_dec* audec)
{
    adec_print("android out init");

    status_t status;
    AudioTrack *track;
    audio_out_operations_t *out_ops = &audec->aout_ops;
char wfd_prop[32];

ttt = 0;
resample = 0;
last_resample = 0;
xxx = 0;
memset(&diff_record[0], 0, 0x40*sizeof(diff_record[0]));
diff_wp = 0;
if(property_get("media.libplayer.wfd", wfd_prop, "0") > 0){
  wfd_enable = (strcmp(wfd_prop, "1") == 0);
  {
    audio_io_handle_t handle = AudioSystem::getOutput(AUDIO_STREAM_MUSIC,
	                                    48000,
	                                    AUDIO_FORMAT_PCM_16_BIT,
	                                    AUDIO_CHANNEL_OUT_STEREO,
#if defined(_VERSION_ICS) 
					AUDIO_POLICY_OUTPUT_FLAG_INDIRECT
#else	//JB...			
	                            AUDIO_OUTPUT_FLAG_PRIMARY
#endif	                            
        );
    if(handle > 0){
	  char str[64];
      status_t ret;
	  memset(str,0,sizeof(str));
      // backup old framecount
      AudioSystem::getFrameCount(handle, AUDIO_STREAM_MUSIC, &old_frame_count);
	  
      sprintf(str,"frame_count=%d",256);
	  ret = AudioSystem::setParameters(handle, String8(str));
      if(ret != 0){
        adec_print("change frame count failed: ret = %d\n", ret);
      }
      adec_print("wfd: %s", str);
    }
  }
}else{
  wfd_enable = 0;
}

adec_print("wfd_enable = %d", wfd_enable);
    Mutex::Autolock _l(mLock);

    track = new AudioTrack();
    if (track == NULL) {
        adec_print("AudioTrack Create Failed!");
        return -1;
    }

	int SessionID = audec->SessionID;
	adec_print("SessionID = %d",SessionID);
	reset_system_samplerate(audec);
#if defined(_VERSION_JB)
	if(audec->channels == 8){
		status = track->set(AUDIO_STREAM_MUSIC,
                        audec->samplerate,
                        AUDIO_FORMAT_PCM_16_BIT,
                        AUDIO_CHANNEL_OUT_7POINT1,
                        0,       // frameCount
                        AUDIO_OUTPUT_FLAG_DEEP_BUFFER/*AUDIO_OUTPUT_FLAG_NONE*/, // flags
                        audioCallback,
                        audec,    // user when callback
                        0,       // notificationFrames
                        0,       // shared buffer
                        false,   // threadCanCallJava
                        SessionID);      // sessionId
	}
	else{
    status = track->set(AUDIO_STREAM_MUSIC,
                        audec->samplerate,
                        AUDIO_FORMAT_PCM_16_BIT,
                        (audec->channels == 1) ? AUDIO_CHANNEL_OUT_MONO : AUDIO_CHANNEL_OUT_STEREO,
                        0,       // frameCount
                        AUDIO_OUTPUT_FLAG_NONE, // flags
                        audioCallback,
                        audec,    // user when callback
                        0,       // notificationFrames
                        0,       // shared buffer
                        false,   // threadCanCallJava
                        SessionID);      // sessionId
		}
                        
#elif defined(_VERSION_ICS)
    status = track->set(AUDIO_STREAM_MUSIC,
                        audec->samplerate,
                        AUDIO_FORMAT_PCM_16_BIT,
                        (audec->channels == 1) ? AUDIO_CHANNEL_OUT_MONO : AUDIO_CHANNEL_OUT_STEREO,
                        0,       // frameCount
                        0,       // flags
                        audioCallback,
                        audec,    // user when callback
                        0,       // notificationFrames
                        0,       // shared buffer
                        false,	 // threadCanCallJava
                        SessionID);      // sessionId
#else   // GB or lower:
    status = track->set(AudioSystem::MUSIC,
                        audec->samplerate,
                        AudioSystem::PCM_16_BIT,
                        (audec->channels == 1) ? AudioSystem::CHANNEL_OUT_MONO : AudioSystem::CHANNEL_OUT_STEREO,
                        0,       // frameCount
                        0,       // flags
                        audioCallback,
                        audec,    // user when callback
                        0,       // notificationFrames
                        0,       // shared buffer
                        SessionID);
#endif

    if (status != NO_ERROR) {
        adec_print("track->set returns %d", status);
        adec_print("audio out samplet %d", audec->samplerate);
        adec_print("audio out channels %d", audec->channels);
        delete track;
        track = NULL;
        return -1;

    }
    af_resample_linear_init();
    out_ops->private_data = (void *)track;
    return 0;
}

/**
 * \brief start output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 *
 * Call android_start(), then the callback will start being called.
 */
extern "C" int android_start(struct aml_audio_dec* audec)
{
    adec_print("android out start");

    status_t status;
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    Mutex::Autolock _l(mLock);
ttt = 0;
resample = 0;
last_resample = 0;
xxx = 0;
memset(&diff_record[0], 0, 0x40*sizeof(diff_record[0]));
diff_wp = 0;


    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    status = track->initCheck();
    if (status != NO_ERROR) {
        delete track;
        out_ops->private_data = NULL;
        return -1;
    }

    track->start();
    adec_print("AudioTrack initCheck OK and started.");

    return 0;
}

/**
 * \brief pause output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_pause(struct aml_audio_dec* audec)
{
    adec_print("android out pause");

    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    Mutex::Autolock _l(mLock);

    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->pause();

    return 0;
}

/**
 * \brief resume output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_resume(struct aml_audio_dec* audec)
{
    adec_print("android out resume");

    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    Mutex::Autolock _l(mLock);
ttt = 0;
resample = 0;
last_resample = 0;
xxx = 0;
memset(&diff_record[0], 0, 0x40*sizeof(diff_record[0]));
diff_wp = 0;


    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->start();

    return 0;
}

/**
 * \brief stop output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_stop(struct aml_audio_dec* audec)
{
    adec_print("android out stop");

    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    Mutex::Autolock _l(mLock);

    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->stop();

    /* release AudioTrack */
    delete track;
    out_ops->private_data = NULL;
    restore_system_samplerate();	

    restore_system_framesize();

    return 0;
}

/**
 * \brief get output latency in ms
 * \param audec pointer to audec
 * \return output latency
 */
extern "C" unsigned long android_latency(struct aml_audio_dec* audec)
{
    unsigned long latency;
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    if (track) {
        latency = track->latency();
        return latency;
    }

    return 0;
}

/**
 * \brief mute output
 * \param audec pointer to audec
 * \param en  1 = mute, 0 = unmute
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_mute(struct aml_audio_dec* audec, adec_bool_t en)
{
    adec_print("android out mute");

    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    Mutex::Autolock _l(mLock);

    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

   
#ifdef ANDROID_VERSION_JBMR2_UP
#else
	track->mute(en);
#endif

    return 0;
}

/**
 * \brief set output volume
 * \param audec pointer to audec
 * \param vol volume value
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_set_volume(struct aml_audio_dec* audec, float vol)
{
    adec_print("android set volume");

    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    Mutex::Autolock _l(mLock);

    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->setVolume(vol, vol);

    return 0;
}

/**
 * \brief set left/right output volume
 * \param audec pointer to audec
 * \param lvol refer to left volume value
 * \param rvol refer to right volume value
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_set_lrvolume(struct aml_audio_dec* audec, float lvol,float rvol)
{
    adec_print("android set left and right volume separately");

    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;

    Mutex::Autolock _l(mLock);

    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->setVolume(lvol, rvol);

    return 0;
}

extern "C" void android_basic_init()
{
    adec_print("android basic init!");

    Mutex::Autolock _l(mLock);

    sp<ProcessState> proc(ProcessState::self());
}

/**
 * \brief get output handle
 * \param audec pointer to audec
 */
extern "C" void get_output_func(struct aml_audio_dec* audec)
{
    audio_out_operations_t *out_ops = &audec->aout_ops;

    out_ops->init = android_init;
    out_ops->start = android_start;
    out_ops->pause = android_pause;
    out_ops->resume = android_resume;
    out_ops->stop = android_stop;
    out_ops->latency = android_latency;
    out_ops->mute = android_mute;
    out_ops->set_volume = android_set_volume;
    out_ops->set_lrvolume = android_set_lrvolume;
}

}
