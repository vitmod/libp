#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>

#define  LOG_TAG    "wfd-output"
#define adec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

// default using tinyalsa
#include <tinyalsa/asoundlib.h>

// audio PCM output configuration 
#define WFD_PERIOD_SIZE  1024
#define WFD_PERIOD_NUM   4
static struct pcm_config wfd_config_out = {
    .channels = 2,
    .rate = 48000,
    .period_size = WFD_PERIOD_SIZE,
    .period_count = WFD_PERIOD_NUM,
    .format = PCM_FORMAT_S16_LE,
};
static struct pcm *wfd_pcm;
static char cache_buffer_bytes[64];
static int  cached_len=0;

static int  get_aml_card(){
	int card = -1, err = 0;
	int fd = -1;
	unsigned fileSize = 512;
	char *read_buf = NULL, *pd = NULL;
	static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
	fd = open(SOUND_CARDS_PATH, O_RDONLY);
      if (fd < 0) {
        adec_print("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
      }

	read_buf = (char *)malloc(fileSize);
	if (!read_buf) {
        adec_print("Failed to malloc read_buf");
        close(fd);
        return -ENOMEM;
    }
	memset(read_buf, 0x0, fileSize);
	err = read(fd, read_buf, fileSize);
	if (fd < 0) {
        adec_print("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
	 free(read_buf);	
        close(fd);
        return -EINVAL;
      }
	pd = strstr(read_buf, "AML");
	card = *(pd - 3) - '0';

OUT:
	free(read_buf);
	close(fd);
	return card;
}
static int get_spdif_port(){
	int port = -1, err = 0;
	int fd = -1;
	unsigned fileSize = 512;
	char *read_buf = NULL, *pd = NULL;
	static const char *const SOUND_PCM_PATH = "/proc/asound/pcm";
	fd = open(SOUND_PCM_PATH, O_RDONLY);
    	if (fd < 0) {
        adec_print("ERROR: failed to open config file %s error: %d\n", SOUND_PCM_PATH, errno);
        close(fd);
        return -EINVAL;
    }

	read_buf = (char *)malloc(fileSize);
	if (!read_buf) {
        adec_print("Failed to malloc read_buf");
        close(fd);
        return -ENOMEM;
    }
	memset(read_buf, 0x0, fileSize);
	err = read(fd, read_buf, fileSize);
	if (fd < 0) {
        adec_print("ERROR: failed to read config file %s error: %d\n", SOUND_PCM_PATH, errno);
	 free(read_buf);	
        close(fd);
        return -EINVAL;
    }
	pd = strstr(read_buf, "SPDIF");
	if(!pd)
		goto OUT;
	adec_print("%s  \n",pd );
	
	port = *(pd -3) - '0';
	adec_print("%s  \n",(pd -3) );

OUT:
	free(read_buf);
	close(fd);
	return port;
}

int  pcm_output_init(int sr,int ch)
{
    int card = 0;
    int device = 2;
    cached_len = 0;
    wfd_config_out.start_threshold = WFD_PERIOD_SIZE;
    wfd_config_out.avail_min = 0;//SHORT_PERIOD_SIZE;
    card = get_aml_card();
    if(card  < 0)
    {
    	card = 0;
	adec_print("get aml card fail, use default \n");
    }
    device = get_spdif_port();
    if(device < 0)
    {
    	device = 0;
	adec_print("get aml card device fail, use default \n");
    }
    adec_print("open output device card %d, device %d \n",card,device);	
    if(sr < 32000|| sr > 48000 || ch != 2){
		adec_print("wfd output: not right parameter sr %d,ch %d \n",sr,ch);
		return -1;
    }
    wfd_config_out.rate = sr;
    wfd_config_out.channels = ch;
    wfd_pcm = pcm_open(card, device, PCM_OUT /*| PCM_MMAP | PCM_NOIRQ*/, &wfd_config_out);
    if (!pcm_is_ready(wfd_pcm)) {
        adec_print("wfd cannot open pcm_out driver: %s", pcm_get_error(wfd_pcm));		
        pcm_close(wfd_pcm);
	 return -1;	
    }
    adec_print("pcm_output_init done  wfd : %p,\n",wfd_pcm);
    return 0;	
   
}

int  pcm_output_write(char *buf,unsigned size)
{

	int ret = 0;	
	char *data,  *data_dst;
	char *data_src;	
	char outbuf[8192];
	int total_len,ouput_len;
	if(size < 64)
		return 0;	
	if(size > sizeof(outbuf)){
		adec_print("write size tooo big %d \n",size);
	}
	total_len = size + cached_len;

	//adec_print("total_len(%d) =  + cached_len111(%d)", size, cached_len);

	data_src = (char *)cache_buffer_bytes;
	data_dst = (char *)outbuf;



	/*write_back data from cached_buffer*/
	if(cached_len){
		memcpy((void *)data_dst, (void *)data_src, cached_len);
		data_dst += cached_len;
	}
	ouput_len = total_len &(~0x3f);
	data = (char*)buf;

	memcpy((void *)data_dst, (void *)data, ouput_len-cached_len);
	data += (ouput_len-cached_len);
	cached_len = total_len & 0x3f;
	data_src = (char *)cache_buffer_bytes;

	/*save data to cached_buffer*/
	if(cached_len){
		memcpy((void *)data_src, (void *)data, cached_len);
	}	
	ret = pcm_write(wfd_pcm,outbuf,ouput_len);
	if(ret < 0 ){
		adec_print("pcm_output_write failed ? \n");
	}
	//adec_print("write size %d ,ret %d \n",size,ret);
	return ret;
}
int  pcm_output_uninit()
{
	if(wfd_pcm)
		pcm_close(wfd_pcm);
	wfd_pcm = NULL;
	adec_print("pcm_output_uninit done \n");
	return 0;
}
int  pcm_output_latency()
{
#if 1
	struct timespec tstamp;
	unsigned int  avail = 0;
	int ret;
	ret = pcm_get_htimestamp(wfd_pcm,&avail, &tstamp);
	//adec_print("pcm_get_latency ret %d,latency %d \n",ret,avail*1000/48000);
	if(ret)
		return ret;
	else
		return avail*1000/wfd_config_out.rate;
#else
	return pcm_hw_lantency(wfd_pcm);
#endif
}
