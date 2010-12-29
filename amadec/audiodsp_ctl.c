#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "include/log.h"
#include "include/adec.h"
#include "include/audiodsp_control.h"

struct firmware_s
{
int 	id;
int 	support_fmt;
char name[64];
};
//{1,MCODEC_FMT_AAC,"audiodsp_codec_aac.bin"},
struct firmware_s firmware_list[]={
{0,MCODEC_FMT_MPEG123,"audiodsp_codec_mp3_lp.bin"},
{1,MCODEC_FMT_AAC,"audiodsp_codec_aac.bin"},
{2,MCODEC_FMT_AC3,"audiodsp_codec_ac3.bin"},	
{3,MCODEC_FMT_DTS,"audiodsp_codec_dca.bin"},
{4,MCODEC_FMT_FLAC,"audiodsp_codec_flac.bin"},
{5,MCODEC_FMT_COOK,"audiodsp_codec_cook.bin"},
{6,MCODEC_FMT_AMR,"audiodsp_codec_amr.bin"},
{7,MCODEC_FMT_RAAC,"audiodsp_codec_raac.bin"},
{8,MCODEC_FMT_ADPCM,"audiodsp_codec_adpcm.bin"},
{9,MCODEC_FMT_WMA, "audiodsp_codec_wma.bin"},
{10,MCODEC_FMT_PCM,"audiodsp_codec_pcm.bin"},
{11,MCODEC_FMT_WMAPRO,"audiodsp_codec_wmapro.bin"},
};


#define DSP_DEV_NOD	"/dev/audiodsp0"
static int dsp_file_fd=-1;

static int register_firmware(int fd,int fmt,char *name)
{
	struct audiodsp_cmd cmd;
	cmd.cmd=AUDIODSP_REGISTER_FIRMWARE;
	cmd.fmt=fmt;
	cmd.data=name;
	cmd.data_len=strlen(name);
	return ioctl(fd,AUDIODSP_REGISTER_FIRMWARE,&cmd);
}

static int switch_audiodsp(int fmt)
{	
	switch(fmt)
		{
		case  AUDIO_FORMAT_MPEG:
			return MCODEC_FMT_MPEG123;
		case  AUDIO_FORMAT_AAC:
			return MCODEC_FMT_AAC;	
		case  AUDIO_FORMAT_AC3:
			return MCODEC_FMT_AC3;	
		case  AUDIO_FORMAT_DTS:
			return MCODEC_FMT_DTS;
		case  AUDIO_FORMAT_FLAC:
			return MCODEC_FMT_FLAC;
		case  AUDIO_FORMAT_COOK:
			return MCODEC_FMT_COOK;
		case  AUDIO_FORMAT_AMR:
			return MCODEC_FMT_AMR;
		case  AUDIO_FORMAT_RAAC:
			return MCODEC_FMT_RAAC;
		case AUDIO_FORMAT_ADPCM:
			return MCODEC_FMT_ADPCM;
		case AUDIO_FORMAT_PCM_S16BE:
		case AUDIO_FORMAT_PCM_S16LE:
		case AUDIO_FORMAT_PCM_U8:
		case AUDIO_AFORMAT_PCM_BLURAY:
			return MCODEC_FMT_PCM;
		case AUDIO_FORMAT_WMA:
			return MCODEC_FMT_WMA;
		case  AUDIO_FORMAT_WMAPRO:
			return MCODEC_FMT_WMAPRO;
		default:
			return 0;
		}
}
static struct firmware_s * find_firmware_by_fmt(int m_fmt)
{
	unsigned int i;	
	struct firmware_s *f;
	for(i=0;i<sizeof(firmware_list)/sizeof(struct firmware_s);i++)
	{
		f=&firmware_list[i];
		if(f->support_fmt & m_fmt)
			return f;
	}
	return NULL;
}

int audiodsp_init(void)
{
	unsigned i;
	int ret;	
	struct firmware_s *f;
	if(dsp_file_fd<0)	
		dsp_file_fd=open(DSP_DEV_NOD, O_RDONLY, 0644);
	
	if(dsp_file_fd<0)
		{
		  log_print(LOG_ERR, "unable to open audio dsp  %s,err: %s",DSP_DEV_NOD, strerror(errno));
		  return -1;
		}
	 
	ioctl(dsp_file_fd,AUDIODSP_UNREGISTER_ALLFIRMWARE,0);
	for(i=0;i<sizeof(firmware_list)/sizeof(struct firmware_s);i++)
	{
		f=&firmware_list[i];
		ret=register_firmware(dsp_file_fd,f->support_fmt,f->name);
		if(ret!=0)
			{
			 log_print(LOG_ERR,"register firmware error=%d,fmt:%d,name:%s\n",ret,f->support_fmt,f->name);
			}
	}
	if(ret!=0)
	{
		close(dsp_file_fd);
		dsp_file_fd=-1;

	}
	return ret;
}

int audiodsp_start(adec_feeder_t *feeder)
{
	int m_fmt;
	int ret=-1;
	int eret=-1;
	unsigned long val;
	if(dsp_file_fd<0)
		return -1;
	
	m_fmt=switch_audiodsp(feeder->format);
	log_print("[%s:%d]audio_fmt=%d\n",__FUNCTION__,__LINE__,m_fmt);
	if(find_firmware_by_fmt(m_fmt)==NULL)
		return -2;
	ioctl(dsp_file_fd,AUDIODSP_SET_FMT,m_fmt);
	ret=ioctl(dsp_file_fd,AUDIODSP_START,0);
	if(ret!=0)
		return -3;
	ret=-1;
	while(ret!=0 && dsp_file_fd>=0 && !decode_stopped())
		ret=ioctl(dsp_file_fd,AUDIODSP_DECODE_START,0);
	if(ret!=0)
		return -4;
	ioctl(dsp_file_fd,AUDIODSP_GET_CHANNELS_NUM,&val);
	if(val!=(unsigned long)-1)
		{
		feeder->channel_num=val;
		}
	ioctl(dsp_file_fd,AUDIODSP_GET_SAMPLERATE,&val);
	if(val!=(unsigned long)-1)
		feeder->sample_rate=val;
	ioctl(dsp_file_fd,AUDIODSP_GET_BITS_PER_SAMPLE,&val);
	if(val!=(unsigned long)-1)
		feeder->data_width=val;
	return ret;
}
int audiodsp_stop(void)
{
	int m_fmt;
	int ret=-1; 
	if(dsp_file_fd<0)
		return -1;
	ioctl(dsp_file_fd,AUDIODSP_STOP,0);
	
	return ret;
}
int audiodsp_release(void)
{
	if(dsp_file_fd<0)
		return -1;
	close(dsp_file_fd);
	dsp_file_fd=-1;
	return 0;
}
int audiodsp_stream_read(char *buffer,int size)
{
	if(dsp_file_fd<0)
		{
		 log_print(LOG_ERR,"read error!! audiodsp have not opened\n");
		return 0;
		}
	return read(dsp_file_fd,buffer,size);
}

unsigned long  audiodsp_get_pts(void )
{
	unsigned long val;
	if(dsp_file_fd<0)
		{
		 log_print(LOG_ERR,"read error!! audiodsp have not opened\n");
		return -1;
		}
	ioctl(dsp_file_fd,AUDIODSP_GET_PTS,&val);
	return val;
}


