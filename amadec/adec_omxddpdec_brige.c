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
#include "adec_omxddpdec_brige.h"

int StageFrightCodecEnable=OMX_ENABLE_CODEC_NULL;
fp_arm_omx_codec_init       parm_omx_codec_init =NULL;
fp_arm_omx_codec_read       parm_omx_codec_read =NULL;
fp_arm_omx_codec_close      parm_omx_codec_close=NULL;
fp_arm_omx_codec_start      parm_omx_codec_start=NULL;
fp_arm_omx_codec_pause      parm_omx_codec_pause=NULL;
fp_arm_omx_codec_get_declen parm_omx_codec_get_declen=NULL;
fp_arm_omx_codec_get_FS     parm_omx_codec_get_FS =NULL;
fp_arm_omx_codec_get_Nch    parm_omx_codec_get_Nch=NULL;


int find_omx_lib(aml_audio_dec_t *audec)
{    
	audio_decoder_operations_t *adec_ops=audec->adec_ops;
	StageFrightCodecEnable=0;
	parm_omx_codec_init=NULL;
	parm_omx_codec_read=NULL;
	parm_omx_codec_close=NULL;
	parm_omx_codec_start=NULL;
	parm_omx_codec_pause=NULL;
	parm_omx_codec_get_declen=NULL;
	parm_omx_codec_get_FS=NULL;
	parm_omx_codec_get_Nch =NULL;
	if(audec->format==ACODEC_FMT_AC3){
	   StageFrightCodecEnable=OMX_ENABLE_CODEC_AC3;
	}else if(audec->format==ACODEC_FMT_EAC3){
       StageFrightCodecEnable=OMX_ENABLE_CODEC_EAC3;
	}
	
	adec_print("%s %d audec->format=%d\n",__FUNCTION__,__LINE__,audec->format);
	
	if(StageFrightCodecEnable)
	{
		int fd = 0;
		fd = dlopen("libamadec_omx_api.so",RTLD_NOW);
	    if (fd != 0){
	         parm_omx_codec_init      = dlsym(fd, "arm_omx_codec_init");
		     parm_omx_codec_read      = dlsym(fd, "arm_omx_codec_read");
		     parm_omx_codec_close     = dlsym(fd, "arm_omx_codec_close");
			 parm_omx_codec_start     = dlsym(fd, "arm_omx_codec_start");
			 parm_omx_codec_pause     = dlsym(fd, "arm_omx_codec_pause");
			 parm_omx_codec_get_declen= dlsym(fd, "arm_omx_codec_get_declen");
			 parm_omx_codec_get_FS    = dlsym(fd, "arm_omx_codec_get_FS");
			 parm_omx_codec_get_Nch   = dlsym(fd, "arm_omx_codec_get_Nch");
	    }else{
		     adec_print("cant find libamadec_omx_api.so\n");
		     return 0;
	    }	
	}

	if( parm_omx_codec_init   ==NULL ||parm_omx_codec_read   ==NULL||parm_omx_codec_close==NULL ||
	    parm_omx_codec_start  ==NULL ||parm_omx_codec_pause  ==NULL||parm_omx_codec_get_declen==NULL ||
	    parm_omx_codec_get_FS ==NULL ||parm_omx_codec_get_Nch==NULL
	  ){
         adec_print("load func_api in libamadec_omx_api.so faided!\n");
		 return 0;
	}
	
	return StageFrightCodecEnable;
}



static char pcm_buf_tmp[AVCODEC_MAX_AUDIO_FRAME_SIZE];//max frame size out buf
extern void start_adec(aml_audio_dec_t *audec);
extern int audio_codec_release(aml_audio_dec_t *audec);

void *audio_decode_loop_omx(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    audio_decoder_operations_t *adec_ops;
    int nNextFrameSize=0;//next read frame size
    int inlen = 0;//real data size in in_buf
    int nRestLen=0;//left data after last decode 
    int nInBufferSize=0;//full buffer size
    //int nStartDecodePoint=0;//start decode point in in_buf
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    char *pRestData=NULL;
    char *inbuf2;
    
    int dlen = 0;//decode size one time
    int declen = 0;//current decoded size
    int nCurrentReadCount=0;
    char startcode[5];	
    int extra_data = 8;
    int nCodecID;
    int nAudioFormat;
    char *outbuf=pcm_buf_tmp;
    int outlen = 0;
    	
    adec_print("\n\naudio_decode_loop_omx start!\n");
    audec = (aml_audio_dec_t *)args;
	audec->state = INITING;
    aout_ops = &audec->aout_ops;
    adec_ops=audec->adec_ops;
    memset(outbuf, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);

    nAudioFormat=audec->format;
    nNextFrameSize=adec_ops->nInBufSize;
	if(parm_omx_codec_start)
	    (*parm_omx_codec_start)();
	else{
		exit_decode_thread=1;
	    adec_print("audio_decode_loop_omx start falid");
	}
	
    while (1)
	{
exit_decode_loop:
         if(exit_decode_thread){ //detect quit condition
        	 if (inbuf){
            	free(inbuf);
                inbuf = NULL;
        	 }
        	 exit_decode_thread_success=1;
        	 break;
	     }
    	 outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
		 /*-------------------------------------------*/
		 /*     the omx_calling code shoul located here           */         
		 /*-------------------------------------------*/
    	 //dlen = adec_ops->decode(audec->adec_ops, outbuf, &outlen, inbuf+declen, inlen);
         (*parm_omx_codec_read)(outbuf,&outlen,&exit_decode_thread);
		
		 if(outlen>0){
		      memset(&g_AudioInfo,0,sizeof(AudioInfo));
		      g_AudioInfo.channels=(*parm_omx_codec_get_Nch)();
			  g_AudioInfo.samplerate=(*parm_omx_codec_get_FS)();

			  if( g_AudioInfo.channels!=0&&g_AudioInfo.samplerate!=0 )
			  { 
			      if(audec->state == INITING){
			            while (!audec->need_stop) 
				        {
                             g_bst->channels =audec->channels=g_AudioInfo.channels;
				             g_bst->samplerate=audec->samplerate=g_AudioInfo.samplerate;
				             ret = aout_ops->init(audec);
                             if (ret) {
                                adec_print("[%s %d]Audio out device init failed!",__FUNCTION__,__LINE__);
                                //audio_codec_release(audec);
                                continue;
                             }
                             audec->state = INITTED;
				             //start_adec(audec);
					         //adec_print("INIT AudioTrack: samplerate=%d g_bst->channels=%d ",g_AudioInfo.samplerate,g_bst->channels);
					         break;
			           }
			      }else{
			          /*note: the libstagefright_ddp_decoder  should have no case:
			                   *                    (g_AudioInfo.channels !=g_bst->channels)
			                   *          now we just consider the case of:
			                   *                     g_AudioInfo.samplerate!=g_bst->samplerate
			                   */ 
                      if(/*(g_AudioInfo.channels !=g_bst->channels)||*/(g_AudioInfo.samplerate!=g_bst->samplerate))
				      {
					       adec_print("Info Changed: src:sample:%d  channel:%d dest sample:%d  channel:%d \n",g_bst->samplerate,g_bst->channels,g_AudioInfo.samplerate,g_AudioInfo.channels);
					       audec->format_changed_flag = 1;
						   g_bst->channels=audec->channels=g_AudioInfo.channels;
						   g_bst->samplerate=audec->samplerate=g_AudioInfo.samplerate;	
				      }
			      }
			  } 
		 }
		 
		 dlen=(*parm_omx_codec_get_declen)();

    	 //write to the pcm buffer
    	 decode_offset+=dlen;
    	 pcm_cache_size=outlen;
    	 if(g_bst)
    	 {
             while(g_bst->buf_length-g_bst->buf_level<outlen)
			 {
            	 if(exit_decode_thread){
    				 adec_print("exit decoder,-----------------------------\n");
            		 goto exit_decode_loop;
            	 }
            	 usleep(100000);
             }
				
             int wlen=0;
             while(outlen)
             {  
                 if(exit_decode_thread){
    				 adec_print("exit decoder,-----------------------------\n");
            		 goto exit_decode_loop;
            	 }
				 
            	 wlen=write_pcm_buffer(outbuf, g_bst,outlen); 
            	 outlen-=wlen;
            	 pcm_cache_size-=wlen;
             }
    	 }
    }


	//--------------------------------------
	adec_print("[%s %d] has stepped out decodeloop \n",__FUNCTION__,__LINE__);
    if(StageFrightCodecEnable && parm_omx_codec_close)
		(*parm_omx_codec_close)();
	//--------------------------------------
	
    adec_print("Exit audio_decode_loop_omx Thread finished!");
    pthread_exit(NULL);
error:	
    pthread_exit(NULL);
    return NULL;
}


void start_decode_thread_omx(aml_audio_dec_t *audec)
{
	int ret;
    pthread_t    tid;
	int wait_aout_ops_start_time=0;
    //int ret = pthread_create(&tid, NULL, (void *)audio_getpackage_loop, (void *)audec);
    ret = pthread_create(&tid, NULL, (void *)audio_decode_loop_omx, (void *)audec);
    if (ret != 0) {
        adec_print("Create <audio_decode_loop_omx> thread failed!\n");
        return ret;
    }
    sn_threadid=tid;
	pthread_setname_np(tid,"AmadecDecodeLP");
    adec_print("Create <audio_decode_loop_omx> thread success! tid = %d\n", tid); 
	
    while ((!audec->need_stop)&&(audec->state != INITTED)){
		usleep(50);
		wait_aout_ops_start_time++;
    }
    adec_print("[%s]Waiting <audec->state == INITTED> used time: %d*50(us)\n",__FUNCTION__,wait_aout_ops_start_time);

}


void stop_decode_thread_omx(aml_audio_dec_t *audec)
{
    exit_decode_thread=1;
	adec_print(" %s %d\n",__FUNCTION__,__LINE__);
    int ret = pthread_join(sn_threadid, NULL);
    adec_print("decode thread_omx exit success \n");
    exit_decode_thread=0;
    sn_threadid=-1;
    sn_getpackage_threadid=-1;
}


 void  omx_codec_Release()
{
	StageFrightCodecEnable=0;
	parm_omx_codec_init=NULL;
    parm_omx_codec_read=NULL;
	parm_omx_codec_start=NULL;
	parm_omx_codec_pause=NULL;
	parm_omx_codec_close=NULL;
	parm_omx_codec_get_declen=NULL;
	parm_omx_codec_get_FS =NULL;
    parm_omx_codec_get_Nch=NULL;
	
}


