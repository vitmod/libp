/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: main.c,v 1.85 2008/09/22 17:55:09 menno Exp $
**/

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define off_t __int64
#else
#include <time.h>
#endif

#include "libaacdec.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef WIN32
#include <android/log.h>
#endif

#define min(a,b) ( (a) < (b) ? (a) : (b) )

/* MicroSoft channel definitions */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

#define DefaultReadSize 1024*10 //read count from kernel audio buf one time
#define DefaultOutBufSize 1024*1024
#define MAX_CHANNELS 6 /* make this higher to support files with more channels */

//audio decoder buffer 6144 bytes
#define AAC_INPUTBUF_SIZE   (2 * 768 * 4*1)	/* pick something big enough to hold a bunch of frames */

#define ERROR_RESET_COUNT  1000
#define  RSYNC_SKIP_BYTES  1
#define FRAME_RECORD_NUM   1000
#define FRAME_SIZE_MARGIN  300

enum {
	AAC_ERROR_NO_ENOUGH_DATA = -1,
	AAC_ERROR_NEED_RESET_DECODER = -2,
};


typedef struct FaadContext {
	NeAACDecHandle hDecoder;
	int init_flag;
	int header_type;
	int gSampleRate;
	int gChannels;
	int error_count;
	int frame_length_his[FRAME_RECORD_NUM];
}FaadContext;

static const int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

static long aacChannelConfig2wavexChannelMask(NeAACDecFrameInfo *hInfo)
{
    if (hInfo->channels == 6 && hInfo->num_lfe_channels)
    {
        return SPEAKER_FRONT_LEFT + SPEAKER_FRONT_RIGHT +
            SPEAKER_FRONT_CENTER + SPEAKER_LOW_FREQUENCY +
            SPEAKER_BACK_LEFT + SPEAKER_BACK_RIGHT;
    } else {
        return 0;
    }
}

static char *position2string(int position)
{
    switch (position)
    {
    case FRONT_CHANNEL_CENTER: return "Center front";
    case FRONT_CHANNEL_LEFT:   return "Left front";
    case FRONT_CHANNEL_RIGHT:  return "Right front";
    case SIDE_CHANNEL_LEFT:    return "Left side";
    case SIDE_CHANNEL_RIGHT:   return "Right side";
    case BACK_CHANNEL_LEFT:    return "Left back";
    case BACK_CHANNEL_RIGHT:   return "Right back";
    case BACK_CHANNEL_CENTER:  return "Center back";
    case LFE_CHANNEL:          return "LFE";
    case UNKNOWN_CHANNEL:      return "Unknown";
    default: return "";
    }

    return "";
}



static int FindAdtsSRIndex(int sr)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if (sr == adts_sample_rates[i])
            return i;
    }
    return 16 - 1;
}

static unsigned char *MakeAdtsHeader(int *dataSize, NeAACDecFrameInfo *hInfo, int old_format)
{
    unsigned char *data;
    int profile = (hInfo->object_type - 1) & 0x3;
    int sr_index = ((hInfo->sbr == SBR_UPSAMPLED) || (hInfo->sbr == NO_SBR_UPSAMPLED)) ?
        FindAdtsSRIndex(hInfo->samplerate / 2) : FindAdtsSRIndex(hInfo->samplerate);
    int skip = (old_format) ? 8 : 7;
    int framesize = skip + hInfo->bytesconsumed;

    if (hInfo->header_type == ADTS)
        framesize -= skip;

    *dataSize = 7;

    data = malloc(*dataSize * sizeof(unsigned char));
    memset(data, 0, *dataSize * sizeof(unsigned char));

    data[0] += 0xFF; /* 8b: syncword */

    data[1] += 0xF0; /* 4b: syncword */
    /* 1b: mpeg id = 0 */
    /* 2b: layer = 0 */
    data[1] += 1; /* 1b: protection absent */

    data[2] += ((profile << 6) & 0xC0); /* 2b: profile */
    data[2] += ((sr_index << 2) & 0x3C); /* 4b: sampling_frequency_index */
    /* 1b: private = 0 */
    data[2] += ((hInfo->channels >> 2) & 0x1); /* 1b: channel_configuration */

    data[3] += ((hInfo->channels << 6) & 0xC0); /* 2b: channel_configuration */
    /* 1b: original */
    /* 1b: home */
    /* 1b: copyright_id */
    /* 1b: copyright_id_start */
    data[3] += ((framesize >> 11) & 0x3); /* 2b: aac_frame_length */

    data[4] += ((framesize >> 3) & 0xFF); /* 8b: aac_frame_length */

    data[5] += ((framesize << 5) & 0xE0); /* 3b: aac_frame_length */
    data[5] += ((0x7FF >> 6) & 0x1F); /* 5b: adts_buffer_fullness */

    data[6] += ((0x7FF << 2) & 0x3F); /* 6b: adts_buffer_fullness */
    /* 2b: num_raw_data_blocks */

    return data;
}

static unsigned get_frame_size(FaadContext *gFaadCxt)
{
	int i;
	unsigned sum = 0;
	unsigned valid_his_num = 0;
	for(i = 0;i < FRAME_RECORD_NUM;i++)
	{
		if(gFaadCxt->frame_length_his[i] > 0){
			valid_his_num ++;
			sum	+= gFaadCxt->frame_length_his[i];
		}	
	}
	
	if(valid_his_num==0){
		return 2048;
	}	
		
	return sum/valid_his_num;
}
static void store_frame_size(FaadContext *gFaadCxt,int lastFrameLen)
{
	  /* record the frame length into the history buffer */
	  int i = 0;
	  for(i = 0;i < FRAME_RECORD_NUM-1;i++)
	  {
		gFaadCxt->frame_length_his[i] = gFaadCxt->frame_length_his[i+1];
	  }
	  gFaadCxt->frame_length_his[FRAME_RECORD_NUM-1] = lastFrameLen;
}
	
static int AACFindADTSSyncWord(unsigned char *buf, int nBytes)
{
	int i;
	for (i = 0; i < nBytes - 1; i++) {
		if ( ((buf[i+0] & 0xff) == 0xff) && ((buf[i+1] & 0xf6) == 0xf0) )
			break;
	}
	return i;
}
//check two latm frames sync header 
static int AACFindLATMSyncWord(unsigned char *buffer, int nBytes)
{
	int i;
	int i_frame_size = 0;
	for (i = 0; i < nBytes - 2; i++) {
		if (buffer[i] == 0x56 && (buffer[i+1] & 0xe0) == 0xe0){
			i_frame_size = ((buffer[i+1] & 0x1f) << 8) + buffer[i+2];
			if(i_frame_size > 4608)
				audio_codec_print("i_frame_size  exceed  4608 ,%d \n",i_frame_size);
			if(i_frame_size > 0 && i_frame_size < 4608 ){
				break;
			}	
		}	
	}
	return i;
}
int audio_dec_init(
#ifndef WIN32	
	audio_decoder_operations_t *adec_ops
#endif	
	)
{
    audio_codec_print("\n\n[%s]BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
    FaadContext *gFaadCxt = NULL;	
    adec_ops->pdecoder = calloc(1,sizeof(FaadContext));
    if(!adec_ops->pdecoder){
		audio_codec_print("malloc for decoder instance failed\n");
		return -1;
    }
    gFaadCxt = (FaadContext*)adec_ops->pdecoder;
    memset(gFaadCxt,0,sizeof(FaadContext));
    adec_ops->nInBufSize= AAC_INPUTBUF_SIZE;  //4608 bytes input buffer size
    adec_ops->nOutBufSize=DefaultOutBufSize;
    gFaadCxt->gChannels=adec_ops->channels;
    gFaadCxt->gSampleRate=adec_ops->samplerate;
    gFaadCxt->init_flag = 0;
    gFaadCxt->header_type=1;//default adts
    return 0;
}
static int audio_decoder_init( 
#ifndef WIN32	
	audio_decoder_operations_t *adec_ops, 
#endif	
	char *outbuf, int *outlen, char *inbuf, int inlen, long *inbuf_consumed)
{
    unsigned long samplerate;
    unsigned char channels;
    int ret;
    NeAACDecConfigurationPtr config = NULL;
    char *in_buf;
    int  inbuf_size;
    in_buf = inbuf;
    inbuf_size = inlen;
    int nReadLen=0;
    FaadContext *gFaadCxt  =	(FaadContext*)adec_ops->pdecoder;
    if(!in_buf || !inbuf_size ||!outbuf){
		audio_codec_print(" input/output buffer null or input len is 0 \n");
   } 
retry:
    gFaadCxt->hDecoder = NeAACDecOpen();
    config = NeAACDecGetCurrentConfiguration(gFaadCxt->hDecoder);
    config->defObjectType = LC;
    config->outputFormat = FAAD_FMT_16BIT;
    config->downMatrix = 0x01;
    config->useOldADTSFormat = 0;
    //config->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(gFaadCxt->hDecoder, config);
    if ((ret = NeAACDecInit(gFaadCxt->hDecoder, in_buf,inbuf_size, &samplerate, &channels)) < 0)
    {
    		in_buf += RSYNC_SKIP_BYTES;
		inbuf_size -= RSYNC_SKIP_BYTES;
		NeAACDecClose(gFaadCxt->hDecoder);
		gFaadCxt->hDecoder=NULL;
		if(inbuf_size < 0)
			inbuf_size = 0;
		if(inbuf_size <  (get_frame_size(gFaadCxt)+FRAME_SIZE_MARGIN)/*AAC_INPUTBUF_SIZE/2*/){
			audio_codec_print("input size %d at least %d ,need more data \n",inbuf_size,(get_frame_size(gFaadCxt)+FRAME_SIZE_MARGIN));
			*inbuf_consumed = inlen-inbuf_size;
			return AAC_ERROR_NO_ENOUGH_DATA;
		}
		goto retry;
    }
    audio_codec_print("init sucess cost %d\n",ret);	
    in_buf += ret;
    inbuf_size -= ret;
    *inbuf_consumed = inlen-	inbuf_size;
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)(gFaadCxt->hDecoder);
    gFaadCxt->init_flag=1;
    gFaadCxt->gChannels=channels;
    gFaadCxt->gSampleRate=samplerate;
    audio_codec_print("[%s]Init OK adif_present :%d adts_present:%d latm_present:%d,sr %d,ch %d\n",__FUNCTION__,hDecoder->adif_header_present,hDecoder->adts_header_present,hDecoder->latm_header_present,samplerate,channels);
    return 0;
}
int audio_dec_decode(
#ifndef WIN32	
	audio_decoder_operations_t *adec_ops, 
#endif	
	char *outbuf, int *outlen, char *inbuf, int inlen)
{
	unsigned long samplerate;
	unsigned char channels;
	void *sample_buffer;
	NeAACDecFrameInfo frameInfo;
	int outmaxlen = 0;
	char *dec_buf;
	int    dec_bufsize;
	int  inbuf_consumed=0;
	int ret = 0;
	FaadContext *gFaadCxt =	(FaadContext*)adec_ops->pdecoder;
	dec_bufsize = inlen;
	dec_buf = inbuf;	
	outmaxlen = *outlen ;
	*outlen = 0; 
	if(!outbuf ||!inbuf || !inlen){
		audio_codec_print("decoder parameter error,check \n");
		goto exit;
	}
	if (!gFaadCxt->init_flag)
	{
		gFaadCxt->error_count= 0;
		ret = audio_decoder_init(adec_ops,outbuf,outlen,dec_buf,dec_bufsize,&inbuf_consumed);
		if(ret ==  AAC_ERROR_NO_ENOUGH_DATA){
			audio_codec_print("decoder cost %d byte input data ,but initiation failed.^_^ \n",inlen);
			dec_bufsize -= inbuf_consumed;
			goto exit;
		}
		gFaadCxt->init_flag = 1;	
		dec_buf += inbuf_consumed;
		dec_bufsize -= inbuf_consumed;
		if(dec_bufsize < 0)
			dec_bufsize = 0;
	}
	NeAACDecStruct* hDecoder = (NeAACDecStruct*)(gFaadCxt->hDecoder);
	if(hDecoder->adts_header_present)
	{
		int nSeekNum = AACFindADTSSyncWord(dec_buf,dec_bufsize);
		if(nSeekNum == (dec_bufsize-1)){
			audio_codec_print("%d bytes data not found adts sync header \n",nSeekNum);	
		}
		dec_bufsize = dec_bufsize - nSeekNum;
		if(dec_bufsize < (get_frame_size(gFaadCxt)+FRAME_SIZE_MARGIN)/*AAC_INPUTBUF_SIZE/2*/){
			goto exit; 
		}
	}
	if(hDecoder->latm_header_present){
		int nSeekNum = AACFindLATMSyncWord(dec_buf,dec_bufsize);
		if(nSeekNum == (dec_bufsize-2)){
			audio_codec_print("%d bytes data not found adts sync header \n",nSeekNum);	
		}
		dec_bufsize = dec_bufsize - nSeekNum;
		if(dec_bufsize < (get_frame_size(gFaadCxt)+FRAME_SIZE_MARGIN)/*AAC_INPUTBUF_SIZE/2*/){
			goto exit; 
		}		
	}
	sample_buffer = NeAACDecDecode(gFaadCxt->hDecoder, &frameInfo, dec_buf, dec_bufsize);
	dec_bufsize -= frameInfo.bytesconsumed;
	if(frameInfo.error == 0 && sample_buffer == NULL && hDecoder->latm_header_present){
		dec_bufsize -= 3;
		goto exit;
	}
	if ((frameInfo.error == 0) && (frameInfo.samples > 0) && sample_buffer!=NULL )
	{
		store_frame_size(gFaadCxt,frameInfo.bytesconsumed);
		gFaadCxt->gSampleRate=frameInfo.samplerate;
		gFaadCxt->gChannels=frameInfo.channels;
		if( (outmaxlen-(*outlen)) >= (2*frameInfo.samples)){
			memcpy(outbuf+(*outlen), sample_buffer, 2*frameInfo.samples);
			*outlen+=2*frameInfo.samples;
			gFaadCxt->error_count = 0;	
		}else{
			audio_codec_print("[%s %d]WARNING: no enough space used for pcm!\n",__FUNCTION__,__LINE__);
		}
	}

	if (frameInfo.error > 0)//failed seek to the head
	{
		audio_codec_print( "Error: %s\n", NeAACDecGetErrorMessage(frameInfo.error));
		dec_bufsize -= RSYNC_SKIP_BYTES;
		gFaadCxt->error_count++;
		if(gFaadCxt->error_count  >= ERROR_RESET_COUNT ||frameInfo.error == 34){
			if( gFaadCxt->hDecoder){
				NeAACDecClose(gFaadCxt->hDecoder);
				gFaadCxt->hDecoder=NULL;
			}
			gFaadCxt->init_flag = 0;
		}	
	}
exit:
    if(dec_bufsize < 0)
		dec_bufsize = 0;
    return inlen - dec_bufsize;
}


int audio_dec_release(
#ifndef WIN32	
	audio_decoder_operations_t *adec_ops
#endif	
	)
{
	FaadContext *gFaadCxt =	(FaadContext*)adec_ops->pdecoder;
	if(gFaadCxt->hDecoder)
	    	NeAACDecClose(gFaadCxt->hDecoder);
	if(adec_ops->pdecoder){
		free(adec_ops->pdecoder);
		adec_ops->pdecoder = NULL;
	}	
	return 0;
}
#ifndef WIN32
int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
	FaadContext *gFaadCxt =	(FaadContext*)adec_ops->pdecoder;
	((AudioInfo *)pAudioInfo)->channels = gFaadCxt->gChannels;
	((AudioInfo *)pAudioInfo)->samplerate =gFaadCxt->gSampleRate;
	return 0;
}
#endif




