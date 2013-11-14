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
#include <stdbool.h>
#include <ctype.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <Amvideocaptools.h>

#define VIDEOCAPDEV "/dev/amvideocap0"   
int simmap_map_cap(char *buf,int size)
{
	int fd=open(VIDEOCAPDEV,O_RDWR);
	int ret;
	char *mbuf;
	if(fd<0){
		printf("open failed\n");
		return -1;
	}
	ret=ioctl(fd,AMVIDEOCAP_IOW_SET_START_CAPTURE,10000);//10000 max wait.
	printf("capture ok? %d\n",ret);
	mbuf = mmap(NULL,size,PROT_READ, MAP_SHARED,fd,0);
	if(mbuf == NULL || mbuf == MAP_FAILED){
		printf("mmap failed %x\n",buf);
		return -2;
	}
	if(buf&&mbuf){
		memcpy(buf,mbuf,size);
	}
	close(fd);
	return size;
}

int main(int argc,char **argv)
{
	char *filename;
	int w,h,needend;
	char *buf;
	int bufsize;
	int ret;
	int fd;
	if(argc<2){
		printf("usage:%s {file} <width> <height> <end>\n",argv[1]);
		printf("          width&height=0 means used video own size\n");
		return 0;
	}
	w=h=needend=0;
	filename=argv[1];
	if(argc>=4){
		scanf(argv[2],"%d",&w);
		scanf(argv[3],"%d",&h);
	}
	if(argc>=5){
		needend=1;
	}
	if(w*h==0){
		bufsize=1920*1088*3;
		buf=malloc(1920*1088*3);
		w=h=0;
		
	}else{
		bufsize=w*h*3;
		buf=malloc(w*h*3);
	}
	if(!buf){
		printf("malloc bufsize %d failed\n",bufsize);
		return -2;
	}
	printf("start capture %d w=%d h=%d capendfram=%d\n",bufsize,w,h,needend);
	ret=amvideocap_capframe(buf,bufsize,&w,&h,0,needend);
	printf("finished capture %d,w=%d,h=%d\n",ret,w,h);
	if(ret<0)
		return -3;
#if 1	
	fd=open(filename,O_WRONLY | O_CREAT,0644);
	
	if(fd<0){
		printf("create %s failed\n",filename);
		return -2;
	}
	write(fd,buf,ret);
	close(fd);
#endif	
	ret=simmap_map_cap(buf,bufsize);
	printf("finished mam map capture %d\n",ret);
	char *mapname=malloc(strlen(filename)+10);
	mapname[0]=0;
	strcat(mapname,filename);
	strcat(mapname,".mapcap");
	fd=open( mapname,O_WRONLY | O_CREAT,0644);
	if(fd<0){
		printf("create %s failed\n",filename);
		return -2;
	}
	write(fd,buf,ret);
	close(fd);
	free(buf);
	free(mapname);
	return 0;
}
