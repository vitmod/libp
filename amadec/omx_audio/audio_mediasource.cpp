/*
add a mediasource interface just like stagefright player 
for the input port  data feed of OMX decoder
author: jian.xu@amlogic.com
27/2/2013
*/


#include "audio_medissource.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>


extern "C" int read_buffer(unsigned char *buffer,int size);

#define LOG_TAG "Audio_Medissource"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace android {


//#####################################################

typedef enum ddp_downmix_config_t {
    DDP_OUTMIX_AUTO,
    DDP_OUTMIX_LTRT,
    DDP_OUTMIX_LORO,
    DDP_OUTMIX_EXTERNAL,
    DDP_OUTMIX_STREAM
} ddp_downmix_config_t;


/* compression mode */
enum { GBL_COMP_CUSTOM_0=0, GBL_COMP_CUSTOM_1, GBL_COMP_LINE, GBL_COMP_RF };

#define DOLBY_DDPDEC_51_MULTICHANNEL_ENDPOINT 


#ifdef DOLBY_DDPDEC51_MULTICHANNEL_GENERIC

static char cMaxChansProp[]="media.tegra.max.out.channels";
static char cChanMapProp[]="media.tegra.out.channel.map";

#elif defined DOLBY_DDPDEC_51_MULTICHANNEL_ENDPOINT

static char cEndpointProp[]="dolby.audio.sink.info";
static char cInfoFrameByte[]="dolby.audio.sink.channel.allocation";

const char* endpoints[] = { "hdmi2",
                            "hdmi6",
                            "hdmi8",
                            "headset",
                            "speaker",
                            //"bluetooth",//or other supported audio output device
                            "invalid" //this is the default value we will get if not set
                          };
const int EndpointConfig[][4] = {{DDP_OUTMIX_LTRT, GBL_COMP_LINE, 0, 2},        //HDMI2
                                  {DDP_OUTMIX_STREAM, GBL_COMP_LINE, 0, 6},      //HDMI6
                                  {DDP_OUTMIX_STREAM, GBL_COMP_LINE, 0, 6},      //HDMI8//as ddpdec51 supports max 6 ch.
                                  {DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //HEADSET
                                  {DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //SPEAKER
                                  //{DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //BLUETOOTH or other audio device setting
                                  {DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //INVALID/DEFAULT
                                 };
#endif 

DSPerr	AudioMediaSource::bsod_init(DSPshort *	p_pkbuf, DSPshort pkbitptr,BSOD_BSTRM *p_bstrm)
{
	p_bstrm->p_pkbuf = p_pkbuf;
	p_bstrm->pkbitptr = pkbitptr;
	p_bstrm->pkdata = *p_pkbuf;
#if defined(DEBUG)
	p_bstrm->p_start_pkbuf = p_pkbuf;
#endif /* defined(DEBUG) */
	return 0;
}


 DSPerr AudioMediaSource::bsod_unprj(BSOD_BSTRM	*p_bstrm,DSPshort *p_data,  DSPshort numbits)	
{
	/* declare local variables */
	DSPushort data;
	*p_data = (DSPshort)((p_bstrm->pkdata << p_bstrm->pkbitptr) & gbl_msktab[numbits]);
	p_bstrm->pkbitptr += numbits;
	if (p_bstrm->pkbitptr >= GBL_BITSPERWRD)
	{
		p_bstrm->p_pkbuf++;
		p_bstrm->pkdata = *p_bstrm->p_pkbuf;
		p_bstrm->pkbitptr -= GBL_BITSPERWRD;
		data = (DSPushort)p_bstrm->pkdata;
		*p_data |= ((data >> (numbits - p_bstrm->pkbitptr)) & gbl_msktab[numbits]);
	}
	*p_data = (DSPshort)((DSPushort)(*p_data) >> (GBL_BITSPERWRD - numbits));
	return 0;
}


int AudioMediaSource::Get_ChNum_DD(void *buf)
{
    int numch=0;
	BSOD_BSTRM bstrm={0};
	BSOD_BSTRM *p_bstrm=&bstrm;
	short tmp=0,acmod,lfeon,fscod,frmsizecod;
	bsod_init((short*)buf,0,p_bstrm);
	
   /* Unpack bsi fields */
	bsod_unprj(p_bstrm, &tmp, 16);
	if (tmp!= GBL_SYNCWRD)
	{
		ALOGI("Invalid synchronization word");
		return 0;
	}
	bsod_unprj(p_bstrm, &tmp, 16);
	bsod_unprj(p_bstrm, &fscod, 2);
	if (fscod == GBL_MAXFSCOD)
	{
		ALOGI("Invalid sampling rate code");
		return 0;
	}

    if (fscod == 0)      sample_rate = 48000;
    else if (fscod == 1) sample_rate = 44100;
    else if (fscod == 2) sample_rate = 32000;
	
	bsod_unprj(p_bstrm, &frmsizecod, 6);
	if (frmsizecod >= GBL_MAXDDDATARATE)
	{
		ALOGI("Invalid frame size code");
		return 0;
	}

    frame_size=gbl_frmsizetab[fscod][frmsizecod];
	
	bsod_unprj(p_bstrm, &tmp, 5);
	if (!BSI_ISDD(tmp))
	{
		ALOGI("Unsupported bitstream id");
		return 0;
	}

	bsod_unprj(p_bstrm, &tmp, 3);
	bsod_unprj(p_bstrm, &acmod, 3);

	if ((acmod!= GBL_MODE10) && (acmod& 0x1))
	{
		bsod_unprj(p_bstrm, &tmp, 2);
	}
	if (acmod& 0x4)
	{
		bsod_unprj(p_bstrm, &tmp, 2);
	}
	
	if (acmod == GBL_MODE20)
	{
		bsod_unprj(p_bstrm,&tmp, 2);
	}
	bsod_unprj(p_bstrm, &lfeon, 1);


	/* Set up p_bsi->bse_acmod and lfe on derived variables */
	numch = gbl_chanary[acmod];
	//numch+=lfeon;

	char cEndpoint[256/*PROPERTY_VALUE_MAX*/]={0};
	property_get(cEndpointProp, cEndpoint, "invalid");
	const int nEndpoints = sizeof(endpoints)/sizeof(*endpoints);
	int activeEndpoint = (nEndpoints - 1); //default to invalid
		
	for(int i=0; i < nEndpoints; i++)
	{
		if( strcmp( cEndpoint, endpoints[i]) == 0 ) 
		{
			activeEndpoint = i;
			break;
		}
	}
	
	if (activeEndpoint == (nEndpoints - 1))
	{
		ALOGI("Active Endpoint not defined - using default");
	}
	
	if(DDP_OUTMIX_STREAM == (ddp_downmix_config_t)EndpointConfig[activeEndpoint][0])
	{
       	if(numch >= 3)  numch = 8;
	    else    	     numch = 2;
	}else{
		numch = 2;
	}
	ChNum=numch;
	//ALOGI("DEBUG:numch=%d sample_rate=%d %p [%s %d]",ChNum,sample_rate,this,__FUNCTION__,__LINE__);
	return numch;

}


int AudioMediaSource::Get_ChNum_DDP(void *buf)
{

    int numch=0;
	BSOD_BSTRM bstrm={0};
	BSOD_BSTRM *p_bstrm=&bstrm;
	short tmp=0,acmod,lfeon,strmtyp;
	
	bsod_init((short*)buf,0,p_bstrm);
	
	/* Unpack bsi fields */
	bsod_unprj(p_bstrm, &tmp, 16);
	if (tmp!= GBL_SYNCWRD)
	{
		ALOGI("Invalid synchronization word");
		return 0;
	}

	bsod_unprj(p_bstrm, &strmtyp, 2);
	bsod_unprj(p_bstrm, &tmp, 3);
	bsod_unprj(p_bstrm, &tmp, 11);
	frame_size=tmp+1;
	//---------------------------
	if(strmtyp!=0 && strmtyp!=2){
		return 0;
	}
	//---------------------------
	bsod_unprj(p_bstrm, &tmp, 2);

	if (tmp== 0x3)
	{
		ALOGI("Half sample rate unsupported");
		return 0;
	}else{
	    if (tmp == 0)     sample_rate = 48000;
        else if (tmp == 1) sample_rate = 44100;
        else if (tmp == 2) sample_rate = 32000;
		
		bsod_unprj(p_bstrm, &tmp, 2);
	}

	bsod_unprj(p_bstrm, &acmod, 3);
	bsod_unprj(p_bstrm, &lfeon, 1);

	/* Set up p_bsi->bse_acmod and lfe on derived variables */
	numch = gbl_chanary[acmod];
	//numch+=lfeon;
	
	char cEndpoint[256/*PROPERTY_VALUE_MAX*/]={0};
	property_get(cEndpointProp, cEndpoint, "invalid");
	const int nEndpoints = sizeof(endpoints)/sizeof(*endpoints);
	int activeEndpoint = (nEndpoints - 1); //default to invalid
		
	for(int i=0; i < nEndpoints; i++)
	{
		if( strcmp( cEndpoint, endpoints[i]) == 0 ) 
		{
			activeEndpoint = i;
			break;
		}
	}
	
	if (activeEndpoint == (nEndpoints - 1))
	{
		ALOGI("Active Endpoint not defined - using default");
	}
	
	if(DDP_OUTMIX_STREAM == (ddp_downmix_config_t)EndpointConfig[activeEndpoint][0])
	{
       	if(numch >= 3)  numch = 8;
	    else    	     numch = 2;
	}else{
		numch = 2;
	}
	ChNum=numch;
	//ALOGI("DEBUG:numch=%d sample_rate=%d %p [%s %d]",ChNum,sample_rate,this,__FUNCTION__,__LINE__);
	return numch;

}


DSPerr AudioMediaSource::bsod_skip(	BSOD_BSTRM 	*p_bstrm, DSPshort	numbits)
{
	/* check input arguments */
	p_bstrm->pkbitptr += numbits;
	while (p_bstrm->pkbitptr >= GBL_BITSPERWRD)
	{
		p_bstrm->p_pkbuf++;
		p_bstrm->pkdata = *p_bstrm->p_pkbuf;
		p_bstrm->pkbitptr -= GBL_BITSPERWRD;
	}
	
	return 0;
}


DSPerr AudioMediaSource::bsid_getbsid(BSOD_BSTRM *p_inbstrm,	DSPshort *p_bsid)	
{
	/* Declare local variables */
	BSOD_BSTRM	bstrm;

	/* Initialize local bitstream using input bitstream */
	bsod_init(p_inbstrm->p_pkbuf, p_inbstrm->pkbitptr, &bstrm);

	/* Skip ahead to bitstream-id */
	bsod_skip(&bstrm, BSI_BSID_BITOFFSET);

	/* Unpack bitstream-id */
	bsod_unprj(&bstrm, p_bsid, 5);

	/* If not a DD+ bitstream and not a DD bitstream, return error */
	if (!BSI_ISDDP(*p_bsid) && !BSI_ISDD(*p_bsid))
	{
		ALOGI("Unsupported bitstream id");
	}

	return 0;
}


int AudioMediaSource::Get_ChNum_AC3_Frame(void *buf)
{  
   	BSOD_BSTRM bstrm={0};
	BSOD_BSTRM *p_bstrm=&bstrm;
	DSPshort	bsid;
	int chnum=0;
    uint8_t ptr8[PTR_HEAD_SIZE];
	
	memcpy(ptr8,buf,PTR_HEAD_SIZE);

	
	//ALOGI("LZG->ptr_head:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
	//	   ptr8[0],ptr8[1],ptr8[2], ptr8[3],ptr8[4],ptr8[5] );
	if((ptr8[0]==0x0b) && (ptr8[1]==0x77) )
	{
	   int i;
	   uint8_t tmp;
	   for(i=0;i<PTR_HEAD_SIZE;i+=2)
	   {
	      tmp=ptr8[i];
		  ptr8[i]=ptr8[i+1];
		  ptr8[i+1]=tmp;
	   }
	}
    
	
    bsod_init((short*)ptr8,0,p_bstrm);
	bsid_getbsid(p_bstrm, &bsid);
	//ALOGI("LZG->bsid=%d \n", bsid );
	if (BSI_ISDDP(bsid))
	{
		Get_ChNum_DDP(ptr8);
	}else if (BSI_ISDD(bsid)){
		Get_ChNum_DD(ptr8);
	}
	
	return chnum;
}

//#####################################################


AudioMediaSource::AudioMediaSource(void *read_buffer)	
{   
	ALOGI("%s %d \n",__FUNCTION__,__LINE__);
	mStarted=false;
	mMeta=new MetaData;
	mDataSource=NULL;
	mGroup=NULL;
	mBytesReaded=0;
	mCurrentTimeUs=0;
	pStop_ReadBuf_Flag=NULL;
	fpread_buffer=(fp_read_buffer)read_buffer;
	sample_rate=0;
	ChNum=0;
	frame_size=0;
	bytes_readed_sum_pre=0;
	bytes_readed_sum=0;
	
	
}


AudioMediaSource::~AudioMediaSource() 
{
	ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    if (mStarted) {
        stop();
    }
}

int AudioMediaSource::SetReadedBytes(int size)
{
     mBytesReaded=size;
	 return 0;
};

int AudioMediaSource::GetReadedBytes()
{    int bytes_used;
     bytes_used=bytes_readed_sum-bytes_readed_sum_pre;
	 if(bytes_used<0)
	 {
	 	ALOGI("[%s]WARING: bytes_readed_sum/%lld < bytes_readed_sum_pre/%lld \n",__FUNCTION__,bytes_readed_sum,bytes_readed_sum_pre);
	 	bytes_used=0;
	 }
	 bytes_readed_sum_pre=bytes_readed_sum;
     return bytes_used;
};


sp<MetaData> AudioMediaSource::getFormat() {
	ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    return mMeta;
}

status_t AudioMediaSource::start(MetaData *params)
{
	ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    mGroup = new MediaBufferGroup;
	mGroup->add_buffer(new MediaBuffer(4096));
	mStarted = true;
    return OK;	
}

status_t AudioMediaSource::stop()
{
	ALOGI("%s %d \n",__FUNCTION__,__LINE__);
	delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}


static int calc_dd_frame_size(int code)
{
    /* tables lifted from TrueHDDecoder.cxx in DMG's decoder framework */
    static const int FrameSize32K[] = { 96, 96, 120, 120, 144, 144, 168, 168, 192, 192, 240, 240, 288, 288, 336, 336, 384, 384, 480, 480, 576, 576, 672, 672, 768, 768, 960, 960, 1152, 1152, 1344, 1344, 1536, 1536, 1728, 1728, 1920, 1920 };
    static const int FrameSize44K[] = { 69, 70, 87, 88, 104, 105, 121, 122, 139, 140, 174, 175, 208, 209, 243, 244, 278, 279, 348, 349, 417, 418, 487, 488, 557, 558, 696, 697, 835, 836, 975, 976, 114, 1115, 1253, 1254, 1393, 1394 };
    static const int FrameSize48K[] = { 64, 64, 80, 80, 96, 96, 112, 112, 128, 128, 160, 160, 192, 192, 224, 224, 256, 256, 320, 320, 384, 384, 448, 448, 512, 512, 640, 640, 768, 768, 896, 896, 1024, 1024, 1152, 1152, 1280, 1280 };

    int fscod = (code >> 6) & 0x3;
    int frmsizcod = code & 0x3f;

    if (fscod == 0) return 2 * FrameSize48K[frmsizcod];
    if (fscod == 1) return 2 * FrameSize44K[frmsizcod];
    if (fscod == 2) return 2 * FrameSize32K[frmsizcod];

    return 0;
}

int AudioMediaSource::MediaSourceRead_buffer(unsigned char *buffer,int size)
{
   int readcnt=0;
   int readsum=0;
   if(fpread_buffer!=NULL)
   {   int sleep_time=0;
   	   while((readsum<size)&& (*pStop_ReadBuf_Flag==0))
	   {
          readcnt=fpread_buffer(buffer+readsum,size-readsum);
		  if(readcnt<(size-readsum)){
		  	 sleep_time++;
		  	 usleep(10000);
		  }
		  
		  readsum+=readcnt; 
          #if 0
		  if (sleep_time>=10000){
		  	  ALOGI("[%s] waiting time: %d*1000(us) size/readsum %d/%d",
				    __FUNCTION__,sleep_time,size,readsum);
			  break;
		  }
         #endif
   	   }
	   bytes_readed_sum +=readsum;
	   if(*pStop_ReadBuf_Flag==1)
	   {
            ALOGI("[%s] End of Stream: *pStop_ReadBuf_Flag==1\n ", __FUNCTION__);
	   }
	   return readsum;
   }else{
        ALOGI("[%s]ERR: fpread_buffer=NULL\n ", __FUNCTION__);
        return 0;
   }

}


status_t AudioMediaSource::read(MediaBuffer **out, const ReadOptions *options)
{
	*out = NULL;
	int64_t seekTimeUs;
	uint8_t ptr_head[PTR_HEAD_SIZE]={0};
    int readedbytes;
	int SyncFlag=0;
	int chnum_tmp=0;
	
resync:	
	
	frame_size=0;
	SyncFlag=0;
    //----------------------------------
    if(MediaSourceRead_buffer(&ptr_head[0], 6)<6)
	{
	    readedbytes=6;
		ALOGI("WARNING: fpread_buffer read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
		return ERROR_END_OF_STREAM;
    }
	
	if (*pStop_ReadBuf_Flag==1){
		 ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
         return ERROR_END_OF_STREAM;
	}

	while(!SyncFlag)
	{
	 	 int i;
		 for(i=0;i<=4;i++)
		 {
	         if((ptr_head[i]==0x0b && ptr_head[i+1]==0x77 )||(ptr_head[i]==0x77 && ptr_head[i+1]==0x0b ))
	         {
                memcpy(&ptr_head[0],&ptr_head[i],6-i);
			    if(MediaSourceRead_buffer(&ptr_head[6-i],i)<i){
					readedbytes=i;
					ALOGI("WARNING: fpread_buffer read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
					return ERROR_END_OF_STREAM;
			    }
				
				if (*pStop_ReadBuf_Flag==1){
		              ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
                      return ERROR_END_OF_STREAM;
	            }

				SyncFlag=1;
				break;
	         }
		 }
		 
		 if(SyncFlag==0)
		 {
		 	 ptr_head[0]=ptr_head[5];
			 if(MediaSourceRead_buffer(&ptr_head[1],5)<5)
			 {
                ALOGI("WARNING: fpread_buffer read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
                return ERROR_END_OF_STREAM;
			 }
			 
			 if (*pStop_ReadBuf_Flag==1){
		         ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
                 return ERROR_END_OF_STREAM;
	         }
		 }
	}
	
	//----------------------------------
	if(MediaSourceRead_buffer(&ptr_head[6], PTR_HEAD_SIZE-6)<(PTR_HEAD_SIZE-6))
	{
	    readedbytes=PTR_HEAD_SIZE-6;
		ALOGI("WARNING: fpread_buffer read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
		return ERROR_END_OF_STREAM;
    }
	

    Get_ChNum_AC3_Frame(ptr_head);
	frame_size=frame_size*2;
	

	if((frame_size==0)/*||(ChNum==0)||(sample_rate==0)*/)
	{  
	   goto resync;
	}

		
	MediaBuffer *buffer;
	status_t err = mGroup->acquire_buffer(&buffer);
	if (err != OK) {
		return err;
	}
	memcpy((unsigned char*)(buffer->data()),ptr_head,PTR_HEAD_SIZE);
	if (MediaSourceRead_buffer((unsigned char*)(buffer->data())+PTR_HEAD_SIZE,frame_size-PTR_HEAD_SIZE) != (frame_size-PTR_HEAD_SIZE)) {
		buffer->release();
		buffer = NULL;
		return ERROR_END_OF_STREAM;
	}

	buffer->set_range(0, frame_size);
	buffer->meta_data()->setInt64(kKeyTime, mCurrentTimeUs);
	buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
	
	*out = buffer;
	 return OK;
}



}  // namespace android

