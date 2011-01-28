/**************************************************
* example based on amcodec
**************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <adecproc.h>
#include <codec.h>
//#include <getopt.h>
static codec_para_t ts_codec_para;
static codec_para_t *pcodec;
static char sfilename[]="ahmei-1_feicui.ts";
static char *filename=sfilename;
static int file_fd=-1;

int osd_blank(char *path,int cmd)
{
	int fd;
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
		{
		sprintf(bcmd,"%d",cmd);
		write(fd,bcmd,strlen(bcmd));
		close(fd);
		return 0;
		}
	return -1;
}


static void signal_handler(int signum)
{   
	printf("Get signum=%x\n",signum);
	codec_close(pcodec);
	close(file_fd);
	signal(signum, SIG_DFL);
	raise (signum);
}

int main(int argc,char *argv[])
{
	#define READ_SIZE (32 * 1024)
	int ret = CODEC_ERROR_NONE;
	char buffer[READ_SIZE];
	int len = 0;	
	long long pos = 0;
	int size = READ_SIZE;
	uint32_t cnt = 0;	
	
    	printf("\n*********CODEC PLAYER DEMO************\n\n");	
	pcodec = &ts_codec_para;
	memset(pcodec, 0, sizeof(codec_para_t *));		
	pcodec->video_pid = 851;
	pcodec->audio_pid = 852;

	if(argc>1)
		filename=argv[1];
	if(argc>2)
		sscanf(argv[2],"%d",&pcodec->video_pid);
	if(argc>3)
                sscanf(argv[3],"%d",&pcodec->audio_pid);
	
	printf("used vid=%d,aid=%d \n",pcodec->video_pid, pcodec->audio_pid);

	pcodec->video_type = VFORMAT_H264;
    	pcodec->audio_type = AFORMAT_AAC;
	pcodec->has_video = 1;
	pcodec->has_audio = 1;
	pcodec->stream_type = STREAM_TYPE_TS;	
	
	osd_blank("/sys/class/graphics/fb0/blank",1);
	osd_blank("/sys/class/graphics/fb1/blank",0);
	memset(buffer,0, READ_SIZE);
	
	file_fd = open(filename, O_RDWR, 0666);
    if (file_fd < 0)
    {
    	printf("can't open file:%s\n",filename);
    	return -1;
    }
	printf("open file:%s ok!\n",filename);
	if(pcodec->has_audio || pcodec->has_video)
	{
again:  
		ret = codec_init(pcodec);
		if(ret != CODEC_ERROR_NONE)
		{
			printf("codec init failed, ret=-0x%x", -ret);
			return -1;
		}	
		printf("codec init ok!\n");
		pos = lseek(file_fd, 0, SEEK_SET);
		printf("seek pos:%lld\n", pos);
		cnt = 0;
		do
		{	
			cnt++;
			if(cnt >= 500 )
			{
				printf("[amplayer] stephen: test codec close --> init --> close\n");						
				codec_close(pcodec);
				osd_blank("/sys/class/graphics/fb0/blank",1);
				osd_blank("/sys/class/graphics/fb1/blank",0);
				usleep(1000*1000*10);
				goto again;
			}
			len = read(file_fd, buffer, size);			
			if(len > 0)
			{				
				size = 0;
				do
				{				    
					ret = codec_write(pcodec,buffer,len);
					if(ret < 0)
					{
						printf("codec_write ret=%d errno=%d\n",ret,errno);
						if(errno != EAGAIN)
						{
							printf("codec_write failed! ret=-0x%x errno=%d\n",-ret,-errno);
							return -2;
						}
						else
							continue;
					}
					else
					{
						size += ret;
						if(size == len)
						{
							//printf("write %d bytes data\n",ret);
							break;
						}
						else if(size < len)
							continue;
						else
						{
							printf("write failed, want %d bytes, write %d bytes\n",len,size);
							return -3;
						}
					}
				}while(1);
				
			}
			else if(len == 0)
			{
				printf("read eof\n");
				sleep(10);
				break;
			}
			else
			{
				printf("read failed! len=%d errno=%d\n",len,errno);
				break;
			}
		}while(1);
		
	}
	else
		printf("No audio and No video!\n");		
error:	
	printf("will exit!!!!!!!!!!!!\n");
	codec_close(pcodec);
	close(file_fd);
	return 0;
	
}

