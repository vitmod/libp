/*
interface to call OMX codec 
*/
#include "./adec_omx.h"
#include "MediaDefs.h"
#include "OMXCodec.h"

#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_TAG "Adec_OMX"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)



namespace android {


//#####################################################


static const char *MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
static const char *MEDIA_MIMETYPE_AUDIO_EC3 = "audio/eac3";
#define OMX_ENABLE_CODEC_AC3   1
#define OMX_ENABLE_CODEC_EAC3  2

AmlOMXCodec::AmlOMXCodec(int codec_type,void *read_buffer,int *exit)	
{
	m_codec=NULL;
	status_t m_OMXClientConnectStatus=m_OMXClient.connect();
	
	if(m_OMXClientConnectStatus != OK){
		LOGE("Err:omx client connect error\n");
	}else{
	    const char *mine_type=NULL;
		if(codec_type==OMX_ENABLE_CODEC_AC3)
            mine_type=MEDIA_MIMETYPE_AUDIO_AC3;
		else if(codec_type==OMX_ENABLE_CODEC_EAC3)
			mine_type=MEDIA_MIMETYPE_AUDIO_EC3;
		
		LOGI("mine_type=%s %s %d \n",mine_type,__FUNCTION__,__LINE__);
		
        m_OMXMediaSource=new AudioMediaSource(read_buffer);
		m_OMXMediaSource->pStop_ReadBuf_Flag=exit;
	    sp<MetaData> metadata = m_OMXMediaSource->getFormat();	
	    metadata->setCString(kKeyMIMEType,mine_type);
	    m_codec = OMXCodec::Create(
                		m_OMXClient.interface(),
                		metadata,
                		false, // createEncoder
                		m_OMXMediaSource,
                		0,
                		0);	
		
		if (m_codec != NULL)
		{   
			LOGI("OMXCodec::Create success %s %d \n",__FUNCTION__,__LINE__);
		}else{
            LOGE("Err: OMXCodec::Create failed %s %d \n",__FUNCTION__,__LINE__);

		}
	}
}


AmlOMXCodec::~AmlOMXCodec()
{
  
   m_OMXMediaSource=NULL;
   m_codec=NULL;
}


status_t AmlOMXCodec::read(unsigned char *buf,unsigned *size,int *exit)
{
    MediaBuffer *srcBuffer;
    status_t status;
	m_OMXMediaSource->pStop_ReadBuf_Flag=exit;
	if(*exit)
	{
       LOGI("NOTE:exit flag enabled! [%s %d] \n",__FUNCTION__,__LINE__);
	   *size=0;
	    return OK;
	}
	status=  m_codec->read(&srcBuffer,NULL);
	if(srcBuffer==NULL)
	{
	    //LOGI("NOTE:srcBuffer=NULL return size=0 [%s %d] \n",__FUNCTION__,__LINE__);
        *size=0;
	    return OK;
	}
		
	if(*size>srcBuffer->range_length()) //surpose buf is large enough
		 *size=srcBuffer->range_length();
	
	if(status == OK && (*size!=0) ){
		memcpy( buf,
			    srcBuffer->data() + srcBuffer->range_offset(),
			    *size);
		srcBuffer->set_range(srcBuffer->range_offset() + (*size),
                             srcBuffer->range_length() - (*size));
	}else{
	     //LOGI("WARNING:AmlOMXCodec::read Err size=%d status=%d !\n",*size,status);
         //return ERROR_IO;
	}
	
	if (srcBuffer->range_length() == 0) {
         srcBuffer->release();
         srcBuffer = NULL;
    }
	
	return OK;
}

status_t AmlOMXCodec::start()
{
	LOGI("[%s %d] \n",__FUNCTION__,__LINE__);
    status_t status = m_codec->start();
	if (status != OK)
	{
		LOGE("Err:OMX client can't start OMX decoder?! status=%d (0x%08x)\n", (int)status, (int)status);
		m_codec = NULL;
	}
	return status;
}

void  AmlOMXCodec::stop()
{
    LOGI("[%s %d] enter \n",__FUNCTION__,__LINE__);
	if(m_codec != NULL){
	   if(m_OMXMediaSource->pStop_ReadBuf_Flag)
	       *m_OMXMediaSource->pStop_ReadBuf_Flag=1;
	   m_codec->pause();
       m_codec->stop();
	   //delete m_codec;
	   m_OMXMediaSource->stop();
	   //delete m_OMXMediaSource;
	   m_OMXMediaSource=NULL;
	   //delete m_codec;
	   m_codec=NULL;
	   m_OMXClient.disconnect();
	}else
	   LOGE("m_codec==NULL m_codec->stop() failed! %s %d \n",__FUNCTION__,__LINE__);
}

void AmlOMXCodec::pause()
{
    LOGI("[%s %d] \n",__FUNCTION__,__LINE__);
	if(m_codec != NULL)
        m_codec->pause();
	else
	    LOGE("m_codec==NULL m_codec->pause() failed! %s %d \n",__FUNCTION__,__LINE__);
}

int AmlOMXCodec::GetDecBytes()
{
	//if(m_OMXMediaSource!=NULL)
		return m_OMXMediaSource->GetReadedBytes();
	//return 0;
}


}; // namespace android




//#####################################################
extern "C" 
{
	
//android::sp<android::AmlOMXCodec> arm_omx_codec=NULL;
android::AmlOMXCodec *arm_omx_codec=NULL;

void arm_omx_codec_init(int codec_type,void *readbuffer,int *exit)
{
    arm_omx_codec=new android::AmlOMXCodec(codec_type,readbuffer,exit);
	if(arm_omx_codec==NULL)
		LOGE("Err:arm_omx_codec_init failed\n");
	LOGI("[%s %d] arm_omx_codec=%p \n",__FUNCTION__,__LINE__,arm_omx_codec);
}

void arm_omx_codec_start()
{
    LOGI("[%s %d] arm_omx_codec=%p \n",__FUNCTION__,__LINE__,arm_omx_codec);
	if(arm_omx_codec!=NULL)
        arm_omx_codec->start();
	else
	    LOGE("arm_omx_codec==NULL arm_omx_codec->start failed! %s %d \n",__FUNCTION__,__LINE__);
}

void arm_omx_codec_pause()
{
    LOGI("[%s %d] arm_omx_codec=%p \n",__FUNCTION__,__LINE__,arm_omx_codec);
	if(arm_omx_codec!=NULL)
        arm_omx_codec->pause();
	else
	    LOGE("arm_omx_codec==NULL  arm_omx_codec->pause failed! %s %d \n",__FUNCTION__,__LINE__);
}

void arm_omx_codec_read(unsigned char *buf,unsigned *size,int *exit)
{    	 
	 if(arm_omx_codec!=NULL)
         arm_omx_codec->read(buf,size,exit);
	 else
	 	 LOGE("arm_omx_codec==NULL  arm_omx_codec->read failed! %s %d \n",__FUNCTION__,__LINE__);
}

void arm_omx_codec_close()
{
     //if(arm_omx_codec!=NULL)
	 	//delete arm_omx_codec;
	 LOGI("[%s %d] arm_omx_codec=%p \n",__FUNCTION__,__LINE__,arm_omx_codec);
	 if(arm_omx_codec!=NULL){
	      arm_omx_codec->stop();
		   delete arm_omx_codec;
	      arm_omx_codec=NULL;
	 }else{
        LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_close() do nothing! %s %d \n",__FUNCTION__,__LINE__);
	 }
}

int arm_omx_codec_get_declen()
{
	int declen=0;
	if(arm_omx_codec!=NULL)
       declen=arm_omx_codec->GetDecBytes();
	else{
	   LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_declen() return 0! %s %d \n",__FUNCTION__,__LINE__);
	}
	
	return declen;
	
}

int arm_omx_codec_get_FS()
{  if(arm_omx_codec!=NULL)
      return arm_omx_codec->m_OMXMediaSource->sample_rate;
   else{
   	  LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_FS() return 0! %s %d \n",__FUNCTION__,__LINE__);
   	  return 0;
   }
}

int arm_omx_codec_get_Nch()
{  
	if(arm_omx_codec!=NULL)
        return arm_omx_codec->m_OMXMediaSource->ChNum;
	else{
   	    LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_Nch() return 0! %s %d \n",__FUNCTION__,__LINE__);
	    return 0;
	}
}


}


//####################################################

