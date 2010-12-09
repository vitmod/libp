/****************************************
 * file: player_set_disp.c
 * description: set disp attr when playing
****************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <log_print.h>
#include <player_set_sys.h>

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

