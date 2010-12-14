#ifndef _PLAYER_TYPE_H_
#define _PLAYER_TYPE_H_

#include <libavformat/avformat.h>
#include <stream_format.h>

#define MSG_SIZE                    64
#define MAX_VIDEO_STREAMS           8
#define MAX_AUDIO_STREAMS           8
#define MAX_SUB_INTERNAL            8
#define MAX_SUB_EXTERNAL            24
#define MAX_SUB_STREAMS             (MAX_SUB_INTERNAL + MAX_SUB_EXTERNAL)
#define MAX_PLAYER_THREADS          32

#define CALLBACK_INTERVAL			(300)

typedef enum
{      
	/******************************
	* 0x1000x: 
	* player do parse file
	* decoder not running
	******************************/
	PLAYER_INITING  	= 0x10001,
	PLAYER_INITOK   	= 0x10002,	

	/******************************
	* 0x2000x: 
	* playback status
	* decoder is running
	******************************/
	PLAYER_RUNNING  	= 0x20001,
	PLAYER_BUFFERING 	= 0x20002,
	PLAYER_PAUSE    	= 0x20003,
	PLAYER_SEARCHING	= 0x20004,
	
	PLAYER_SEARCHOK 	= 0x20005,
	PLAYER_START    	= 0x20006,	
	PLAYER_FF_END   	= 0x20007,
	PLAYER_FB_END   	= 0x20008,

	/******************************
	* 0x3000x: 
	* player will exit	
	******************************/
	PLAYER_ERROR		= 0x30001,
	PLAYER_PLAYEND  	= 0x30002,	
	PLAYER_STOPED   	= 0x30003,  
	/******************************
	* 0x4000x: 
	* player already exit	
	******************************/
	PLAYER_EXIT   		= 0x40001, 
}player_status;


typedef struct
{   
    int id;    
    int width;
    int height;
    int aspect_ratio_num;
    int aspect_ratio_den;
    int frame_rate_num;
    int frame_rate_den;
	int bit_rate;
    vformat_t format;
    int duartion;
}mvideo_info_t;

typedef enum
{
    ACOVER_NONE   = 0,
    ACOVER_JPG    ,
    ACOVER_PNG    ,
}audio_cover_type;

typedef struct
{
    char title[512];
    char author[512];
    char album[512];
    char comment[512];
    char year[4];  
    int track;     
    char genre[32]; 
    char copyright[512];
    audio_cover_type pic; 
}audio_tag_info;

typedef struct
{    
    int id;
    int channel;
    int sample_rate;
    int bit_rate;
    aformat_t aformat;
    int duration;
	audio_tag_info *audio_tag;    
}maudio_info_t;

typedef struct
{
    char id;
    char internal_external; //0:internal_sub 1:external_sub       
    unsigned short width;
    unsigned short height;
    char resolution;
    long long subtitle_size;  
    char *sub_language;   
}msub_info_t;

typedef struct
{	
    char *filename;
    int  duration;  
	long long  file_size;
    pfile_type type;
	int bitrate;
    int has_video;
    int has_audio;
    int has_sub;
    int nb_streams;
    int total_video_num;
    int cur_video_index;
    int total_audio_num;
    int cur_audio_index;
    int total_sub_num;      
    int cur_sub_index;			
}mstream_info_t;

typedef struct
{	
	mstream_info_t stream_info;
	mvideo_info_t *video_info[MAX_VIDEO_STREAMS];
	maudio_info_t *audio_info[MAX_AUDIO_STREAMS];
    msub_info_t *sub_info[MAX_SUB_STREAMS];
}media_info_t;

typedef struct player_info
{
	char *name;
	player_status last_sta;
	player_status status;		   /*stop,pause	*/
	int full_time;	   /*Seconds	*/
	int current_time;  /*Seconds	*/
	int current_ms;	/*ms*/
	int last_time;		
	int error_no;  
	int start_time;
	int pts_video;
	int pts_pcrscr;
	int current_pts;
	long curtime_old_time;    
	unsigned int video_error_cnt;
	unsigned int audio_error_cnt;
	float		    audio_bufferlevel;
	float		    video_bufferlevel;
}player_info_t;

typedef struct pid_info
{
    int num;
    int pid[MAX_PLAYER_THREADS];
}pid_info_t;



#define state_pre(sta) (sta>>16)
#define player_thread_init(sta)	(state_pre(sta)==0x10000)
#define player_thread_run(sta)	(state_pre(sta)==0x20000)
#define player_thread_stop(sta)	(state_pre(sta)==0x30000)

typedef int (*update_state_fun_t)(int pid,player_info_t *) ;

typedef struct
{
    update_state_fun_t update_statue_callback;
    int update_interval;
    long callback_old_time;
}callback_t;

typedef struct
 {
	char  *file_name;	
	//List  *play_list;
	int	video_index;
	int	audio_index;
	int sub_index;
	int t_pos;	
	int	read_max_cnt;
	union
	{     
		struct{
			unsigned int loop_mode:1;
			unsigned int nosound:1;	
			unsigned int novideo:1;	
			unsigned int hassub:1;
			unsigned int need_start:1;       
		};
		int mode;
	};  
	callback_t callback_fn;
	int byteiobufsize;
	int loopbufsize;
	int enable_rw_on_pause;
	/*
	data%<min && data% <max  enter buffering;
	data% >middle exit buffering;
	*/
	int auto_buffing_enable;
	float buffing_min;
	float buffing_middle;
	float buffing_max;
 }play_control_t; 

#endif
