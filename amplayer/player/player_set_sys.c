/****************************************
 * file: player_set_disp.c
 * description: set disp attr when playing
****************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <log_print.h>
#include <player_set_sys.h>

#define DISP_MODE_480I		(1)
#define DISP_MODE_480P		(2)
#define DISP_MODE_576I		(3)
#define DISP_MODE_576P		(4)
#define DISP_MODE_720P		(5)
#define DISP_MODE_1080I		(6)
#define DISP_MODE_1080P		(7)
	
int set_black_policy(int blackout)       
{
	int fd;
    int bytes;
    char *path = "/sys/class/video/blackout_policy";
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",blackout);
    	bytes = write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
}

int get_black_policy()
{
    int fd;
    int black_out = 0;
    char *path = "/sys/class/video/blackout_policy";
	char  bcmd[16];
	fd=open(path, O_RDONLY);
	if(fd>=0)
	{    	
    	read(fd,bcmd,sizeof(bcmd));       
        black_out = strtol(bcmd, NULL, 16);       
        black_out &= 0x1;
    	close(fd);    	
	}
	return black_out;
}

int set_tsync_enable(int enable)
{
    int fd;
    char *path = "/sys/class/tsync/enable";    
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",enable);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
    
}
int set_tsync_discontinue(int discontinue)		//kernel set to 1,player clear to 0
{
    int fd;
    char *path = "/sys/class/tsync/discontinue";  
	char  bcmd[16];
	if(discontinue)
		return -1;
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",discontinue);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);		
    	return 0;
	}
	else		
		log_error("[%s:%d]open %s failed! errno=%d\n",__FUNCTION__,__LINE__,path,errno);
	return -1;
    
}
int get_pts_discontinue()
{
    int fd;
    int discontinue = 0;
    char *path = "/sys/class/tsync/discontinue";
	char  bcmd[16];	
	fd=open(path, O_RDONLY);
	if(fd>=0)
	{	
    	read(fd,bcmd,sizeof(bcmd));       
        discontinue = strtol(bcmd, NULL, 16);       
        discontinue &= 0x1;		
    	close(fd);    	
	}
	else
		log_error("[%s:%d]open %s failed!\n",__FUNCTION__,__LINE__,path);
	return discontinue;
}

int set_subtitle_num(int num)
{
    int fd;
    char *path = "/sys/class/subtitle/total";    
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",num);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
    
}

int set_subtitle_fps(int fps)
{
    int fd;
    char *path = "/sys/class/subtitle/fps";    
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",fps);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
    
}

int set_subtitle_subtype(int subtype)
{
    int fd;
    char *path = "/sys/class/subtitle/subtype";    
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",subtype);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
    
}

int av_get_subtitle_curr()
{
    int fd;
	int subtitle_cur = 0;
    char *path = "/sys/class/subtitle/curr";    
	char  bcmd[16];
	fd=open(path, O_RDONLY);
	if(fd>=0)
	{    	
    	read(fd,bcmd,sizeof(bcmd)); 
		sscanf(bcmd, "%d", &subtitle_cur);
    	close(fd);    	
	}
	return subtitle_cur;   
}

int set_subtitle_startpts(int pts)
{
    int fd;
    char *path = "/sys/class/subtitle/startpts";    
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",pts);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
    
}

int set_fb0_blank(int blank)
{
    int fd;
    char *path = "/sys/class/graphics/fb0/blank" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",blank);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   				
}

int set_fb1_blank(int blank)
{
    int fd;
    char *path = "/sys/class/graphics/fb1/blank" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",blank);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   				
}

char *get_display_mode()
{
	int fd;   
    char *path = "/sys/class/display/mode";
	char *mode = NULL;
	fd=open(path, O_RDONLY);
	if(fd>=0)
	{    	
		mode = malloc(6);
		if(mode)
    	{
    		read(fd,mode,6); 
			mode[5] = '\0';
		}
		else
			log_error("[get_display_mode]malloc failed!\n");
    	close(fd);    	
	}
	log_print("[get_display_mode]display_mode=%s\n",mode);
	return mode;
}

int set_fb0_freescale(int freescale)
{
	int fd;
    char *path = "/sys/class/graphics/fb0/free_scale" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",freescale);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   	
}

int set_fb1_freescale(int freescale)
{
	int fd;
    char *path = "/sys/class/graphics/fb1/free_scale" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",freescale);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   	
}

int set_display_axis(int *coordinate)
{
	int fd;
    char *path = "/sys/class/display/axis" ;   
	char  bcmd[32];
	int x00, x01,x10,x11,y00, y01,y10,y11;
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0 && coordinate)
	{
		x00 = coordinate[0];
		y00 = coordinate[1];
		x01 = coordinate[2];
		y01 = coordinate[3];
		x10 = coordinate[4];
		y10 = coordinate[5];
		x11 = coordinate[6];
		y11 = coordinate[7];
    	sprintf(bcmd,"%d %d %d %d %d %d %d %d",x00, y00, x01, y01, x10, y10, x11, y11);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
}

int set_video_axis(int *coordinate)
{
	int fd;
    char *path = "/sys/class/video/axis" ;   
	char  bcmd[32];
	int x0, y0, x1,y1;
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0 && coordinate)
	{
		x0 = coordinate[0];
		y0 = coordinate[1];
		x1 = coordinate[2];
		y1 = coordinate[3];		
    	sprintf(bcmd,"%d %d %d %d",x0, y0, x1, y1);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;
}

int set_fb0_scale_width(int width)
{
	int fd;
    char *path = "/sys/class/graphics/fb0/scale_width" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",width);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   	
}
int set_fb0_scale_height(int height)
{
	int fd;
    char *path = "/sys/class/graphics/fb0/scale_height" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",height);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   	
}
int set_fb1_scale_width(int width)
{
	int fd;
    char *path = "/sys/class/graphics/fb1/scale_width" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",width);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   	
}
int set_fb1_scale_height(int height)
{
	int fd;
    char *path = "/sys/class/graphics/fb1/scale_height" ;   
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
    	sprintf(bcmd,"%d",height);
    	write(fd,bcmd,strlen(bcmd));
    	close(fd);
    	return 0;
	}
	return -1;   	
}

static int display_mode_convert(char *disp_mode)
{
	int ret = 0xff;
	log_print("[display_mode_convert]disp_mode=%s\n",disp_mode);
	if(!disp_mode)
		ret = -1;	
	else if(!strcmp(disp_mode, "480i"))
		ret = DISP_MODE_480I;
	else if(!strcmp(disp_mode, "480p"))
		ret = DISP_MODE_480P;
	else if(!strcmp(disp_mode, "567i"))
		ret = DISP_MODE_576I;
	else if(!strcmp(disp_mode, "576p"))
		ret = DISP_MODE_576P;
	else if(!strcmp(disp_mode, "720p"))
		ret = DISP_MODE_720P;
	else if(!strcmp(disp_mode, "1080i"))
		ret = DISP_MODE_1080I;
	else if(!strcmp(disp_mode, "1080p"))
		ret = DISP_MODE_1080P;
	else
		ret = -2;
	log_print("[display_mode_convert]disp_mode=%s-->%d\n",disp_mode, ret);
	return ret;
}
//////////////////////////////////////////////
static void get_display_axis()
{
    int fd;
    int discontinue = 0;
    char *path = "/sys/class/display/axis";
	char  bcmd[32];	
	fd=open(path, O_RDONLY);
	if(fd>=0)
	{	
    	read(fd,bcmd,sizeof(bcmd));       
		bcmd[31]='\0';
        log_print("[get_disp_axis]===%s===\n",bcmd);
    	close(fd);    	
	}
	else
		log_error("[%s:%d]open %s failed!\n",__FUNCTION__,__LINE__,path);	
}
static void get_video_axis()
{
    int fd;
    int discontinue = 0;
    char *path = "/sys/class/video/axis";
	char  bcmd[32];	
	fd=open(path, O_RDONLY);
	if(fd>=0)
	{	
    	read(fd,bcmd,sizeof(bcmd));       
		bcmd[31]='\0';
        log_print("[get_video_axis]===%s===\n",bcmd);
    	close(fd);    	
	}
	else
		log_error("[%s:%d]open %s failed!\n",__FUNCTION__,__LINE__,path);	
}
//////////////////////////////////////////////
int disable_freescale()
{	
	char *mode = get_display_mode();
	log_print("[disable_freescale]display_mode=%s \n",mode);
	int osd_coordinates[8];
	if(mode)
	{
		switch(display_mode_convert(mode))
		{
			case DISP_MODE_480P:
				osd_coordinates[0] = 0;
				osd_coordinates[1] = 0;
				osd_coordinates[2] = 800;
				osd_coordinates[3] = 480;
				osd_coordinates[4] = 0;
				osd_coordinates[5] = 0;
				osd_coordinates[6] = 18;
				osd_coordinates[7] = 18;
				set_fb0_freescale(0);
				set_fb1_freescale(0);
				set_display_axis(osd_coordinates);				
				break;
			case DISP_MODE_720P:
				osd_coordinates[0] = 240;
				osd_coordinates[1] = 120;
				osd_coordinates[2] = 800;
				osd_coordinates[3] = 480;
				osd_coordinates[4] = 240;
				osd_coordinates[5] = 120;
				osd_coordinates[6] = 18;
				osd_coordinates[7] = 18;
				set_fb0_freescale(0);
				set_fb1_freescale(0);
				set_display_axis(osd_coordinates);
				break;
			case DISP_MODE_1080I:
			case DISP_MODE_1080P:
				osd_coordinates[0] = 560;
				osd_coordinates[1] = 300;
				osd_coordinates[2] = 800;
				osd_coordinates[3] = 480;
				osd_coordinates[4] = 560;
				osd_coordinates[5] = 300;
				osd_coordinates[6] = 18;
				osd_coordinates[7] = 18;
				set_fb0_freescale(0);
				set_fb1_freescale(0);
				set_display_axis(osd_coordinates);				
				break;
		}
		log_print("[disable_freescale]success!\n");
		if(mode)
			free(mode);
		return 0;
	}
	log_print("[disable_freescale]failed!\n");
	if(mode)
		free(mode);
	return -1;
}

int enable_freescale()
{
	char *mode = get_display_mode();
	log_print("[enable_freescale:enter]displayer_mode=%s\n",mode);	
	int video_coordinates[4];
	int osd_coordinates[8];
	if(mode)
	{
		switch(display_mode_convert(mode))
		{
			case DISP_MODE_480P:
				video_coordinates[0] = 20;
				video_coordinates[1] = 10;
				video_coordinates[2] = 700;
				video_coordinates[3] = 470;		
				set_video_axis(video_coordinates);
				osd_coordinates[0] = 0;
				osd_coordinates[1] = 0;
				osd_coordinates[2] = 800;
				osd_coordinates[3] = 480;
				osd_coordinates[4] = 0;
				osd_coordinates[5] = 0;
				osd_coordinates[6] = 18;
				osd_coordinates[7] = 18;
				set_display_axis(osd_coordinates);
				set_fb0_freescale(0);
				set_fb1_freescale(0);
				set_fb0_scale_width(800);
				set_fb0_scale_height(480);
				set_fb1_scale_width(800);
				set_fb1_scale_height(480);
				set_fb0_freescale(1);
				set_fb1_freescale(1);
				
				break;
			case DISP_MODE_720P:
				video_coordinates[0] = 40;
				video_coordinates[1] = 15;
				video_coordinates[2] = 1240;
				video_coordinates[3] = 705;		
				set_video_axis(video_coordinates);
				osd_coordinates[0] = 0;
				osd_coordinates[1] = 0;
				osd_coordinates[2] = 800;
				osd_coordinates[3] = 480;
				osd_coordinates[4] = 0;
				osd_coordinates[5] = 0;
				osd_coordinates[6] = 18;
				osd_coordinates[7] = 18;
				set_display_axis(osd_coordinates);
				set_fb0_freescale(0);
				set_fb1_freescale(0);
				set_fb0_scale_width(800);
				set_fb0_scale_height(480);
				set_fb1_scale_width(800);
				set_fb1_scale_height(480);
				set_fb0_freescale(1);
				set_fb1_freescale(1);
				break;
			case DISP_MODE_1080I:
			case DISP_MODE_1080P:
				video_coordinates[0] = 40;
				video_coordinates[1] = 20;
				video_coordinates[2] = 1880;
				video_coordinates[3] = 1060;		
				set_video_axis(video_coordinates);
				osd_coordinates[0] = 0;
				osd_coordinates[1] = 0;
				osd_coordinates[2] = 800;
				osd_coordinates[3] = 480;
				osd_coordinates[4] = 0;
				osd_coordinates[5] = 0;
				osd_coordinates[6] = 18;
				osd_coordinates[7] = 18;
				set_display_axis(osd_coordinates);
				set_fb0_freescale(0);
				set_fb1_freescale(0);
				set_fb0_scale_width(800);
				set_fb0_scale_height(480);
				set_fb1_scale_width(800);
				set_fb1_scale_height(480);
				set_fb0_freescale(1);
				set_fb1_freescale(1);				
				break;
		}
		log_print("[enable_freescale:exit]success\n");	
		if(mode)
			free(mode);
		return 0;
	}
	log_print("[enable_freescale:exit]failed\n");	
	if(mode)
		free(mode);
	return -1;
}
/*****************************************************
EnableFreeScale() {
mode = "cat /sys/class/display/mode"   //取得当前模式
if(mode = 480p)
echo 20 10 700 470  > /sys/class/video/axis             //设置free_scale目标区域
echo 0 0 800 480 0 0 18 18 > /sys/class/display/axis  //取消OSD偏移
echo 0 > /sys/class/graphics/fb0/free_scale              //关闭free_scale
echo 0 > /sys/class/graphics/fb1/free_scale 
echo 800 > /sys/class/graphics/fb0/scale_width         //设置free_scale参数
echo 480 > /sys/class/graphics/fb0/scale_height
echo 800 > /sys/class/graphics/fb1/scale_width
echo 480 > /sys/class/graphics/fb1/scale_height
echo 1 > /sys/class/graphics/fb0/free_scale              //开启free_scale
echo 1 > /sys/class/graphics/fb1/free_scale 
 
else if(mode = 720p)
echo 40 15 1240 705 > /sys/class/video/axis 
echo 0 0 800 480 0 0 18 18 > /sys/class/display/axis 
echo 0 > /sys/class/graphics/fb0/free_scale 
echo 0 > /sys/class/graphics/fb1/free_scale 
echo 800 > /sys/class/graphics/fb0/scale_width
echo 480 > /sys/class/graphics/fb0/scale_height
echo 800 > /sys/class/graphics/fb1/scale_width
echo 480 > /sys/class/graphics/fb1/scale_height
echo 1 > /sys/class/graphics/fb0/free_scale 
echo 1 > /sys/class/graphics/fb1/free_scale 
 
else if(mode = 1080i || mode = 1080p)
      echo 40 20 1880 1060 > /sys/class/video/axis 
echo 0 0 800 480 0 0 18 18 > /sys/class/display/axis 
echo 0 > /sys/class/graphics/fb0/free_scale 
echo 0 > /sys/class/graphics/fb1/free_scale 
echo 800 > /sys/class/graphics/fb0/scale_width
echo 480 > /sys/class/graphics/fb0/scale_height
echo 800 > /sys/class/graphics/fb1/scale_width
echo 480 > /sys/class/graphics/fb1/scale_height
echo 1 > /sys/class/graphics/fb0/free_scale 
echo 1 > /sys/class/graphics/fb1/free_scale 
}


*DisableFreeScale() {
mode = "cat /sys/class/display/mode"        //取得当前模式
if(mode = 480p)
echo 0 > /sys/class/graphics/fb0/free_scale     //关闭free_scale
echo 0 > /sys/class/graphics/fb1/free_scale     //关闭free_scale
echo 0 0 800 480 0 0 18 18 > /sys/class/display/axis  //OSD 居中
 
else if(mode = 720p)
echo 0 > /sys/class/graphics/fb0/free_scale 
echo 0 > /sys/class/graphics/fb1/free_scale 
echo 240 120 800 480 240 120 18 18 > /sys/class/display/axis 
 
else if(mode = 1080i || mode = 1080p)
echo 0 > /sys/class/graphics/fb0/free_scale 
echo 0 > /sys/class/graphics/fb1/free_scale 
echo 560 300 800 480 560 300 18 18 > /sys/class/display/axis 
 
}
**********************************************************/
