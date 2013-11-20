#define LOG_TAG "AmAvutls"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>
#include <sys/ioctl.h>

#include "Amvideocaptools.h"


#define VIDEOCAPDEV "/dev/amvideocap0"              
int amvideocap_capframe(char *buf,int size,int *w,int *h,int fmt_ignored,int at_end)
{
	int fd=open(VIDEOCAPDEV,O_RDWR);
	int ret;
	if(fd<0){
		ALOGI("amvideocap_capframe open %s failed\n",VIDEOCAPDEV);
		return -1;
	}
	if(w>0){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH,w);
	}
	if(h>0){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT,h);
	}
	if(at_end){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS,CAP_FLAG_AT_END);
	}
	ret=read(fd,buf,size);
	if(w<0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_GET_FRAME_WIDTH,&w);
	}
	if(h<0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_GET_FRAME_HEIGHT,&h);
	}
	close(fd);
	return ret;
}

