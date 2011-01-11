/********************************************
 * name	: player_control.c
 * function: thread manage
 * date		: 2010.2.22
 ********************************************/
#include <pthread.h>
#include <player.h>
#include <player_set_sys.h>

#include "player_update.h"
#include "thread_mgt.h"

#include "player_ts.h"
#include "player_es.h"
#include "player_rm.h"
#include "player_ps.h"
#include "player_video.h"
#include "player_audio.h"

#include "stream_decoder.h"
#include "player_ffmpeg_ctrl.h"

#ifndef FBIOPUT_OSD_SRCCOLORKEY
#define  FBIOPUT_OSD_SRCCOLORKEY    0x46fb
#endif

#ifndef FBIOPUT_OSD_SRCKEY_ENABLE
#define  FBIOPUT_OSD_SRCKEY_ENABLE  0x46fa
#endif

static void player_release(int pid)
{    
    play_para_t *para;
	log_print("[player_release:enter]pid=%d\n", pid); 
    para=player_open_pid_data(pid);    
    if(NULL == para)
        return;   

    FREE(para);
    para = NULL;        
    log_print("pid[%d]player release ok \n",pid);   
    player_close_pid_data(pid);
    player_release_pid(pid);
	log_print("[player_release:enter]exit=%d\n", pid); 
}
 
int player_init()
{
	// Register all formats and codecs
	ffmpeg_init();
	player_id_pool_init();
	ts_register_stream_decoder();
	es_register_stream_decoder();
	ps_register_stream_decoder();
    rm_register_stream_decoder();
	audio_register_stream_decoder();
	video_register_stream_decoder();
    set_black_policy(1);
	return 0;
}

int player_send_message(int pid,player_cmd_t *cmd)
{	
    player_cmd_t *mycmd;
    int r=-1;
    if(player_get_state(pid)==PLAYER_STOPED)
    return PLAYER_SUCCESS;   
    mycmd=message_alloc();
    if(mycmd)
    {
        memcpy(mycmd,cmd,sizeof(*cmd));
        r=send_message_by_pid(pid,mycmd);
    }
    else
    {
        r= PLAYER_NOMEM;
    }   
    return r;
}

int player_register_update_callback(callback_t *cb,update_state_fun_t up_fn,int interval_s)
{
    int ret;
    if(!cb)
    {
        log_error("[player_register_update_callback]empty callback pointer!\n");
        return PLAYER_EMPTY_P;
    }
    ret=register_update_callback(cb,up_fn,interval_s);    
    return ret;
}

int player_start(play_control_t *thead_p,unsigned long  priv)
{ 	 	
    int ret;    	
    int pid = -1;  
    play_para_t *p_para;
    log_print("[player_start:enter]p=%p \n",thead_p);
    if(thead_p == NULL)
        return PLAYER_EMPTY_P;
    pid = player_request_pid();    
    if(pid < 0)
        return PLAYER_NOT_VALID_PID;
     
    p_para = MALLOC(sizeof(play_para_t));
    if(p_para==NULL)	
        return PLAYER_NOMEM;
    MEMSET(p_para,0,sizeof(play_para_t)); 
	p_para->playctrl_info.time_point = -1;
    player_init_pid_data(pid,p_para);     
    message_pool_init(p_para);     
    p_para->start_param=thead_p;
    p_para->player_id = pid;
    p_para->extern_priv = priv;
    log_debug1("[player_start]player_para=%p,start_param=%p pid=%d\n",p_para,p_para->start_param,pid);
	ffmpeg_uninterrupt();
	disable_freescale();
    ret=player_thread_create(p_para) ;
    if(ret!=PLAYER_SUCCESS)	
    {
        FREE(p_para);
        player_release_pid(pid);		
        return PLAYER_CAN_NOT_CREAT_THREADS;
    }   
	log_print("[player_start:exit]pid=%d \n",pid);
    return pid;
}
int player_start_play(int pid)
{
	player_cmd_t *cmd;
    int r=PLAYER_SUCCESS;
    play_para_t *player_para;
	log_print("[player_start_play:enter]pid=%d\n", pid); 
    player_para=player_open_pid_data(pid);
    
    if(player_para==NULL)
        return PLAYER_NOT_VALID_PID;	
    
    cmd=message_alloc();
    if(cmd)
    {   
        cmd->ctrl_cmd = CMD_START;		
        r=send_message(player_para,cmd); 
    }
    else
    {
        r= PLAYER_NOMEM;
    }      
    player_close_pid_data(pid);    
	log_print("[player_start_play:exit]pid=%d\n", pid); 
    return r;
}

int player_stop_async(int pid)
{
    player_cmd_t *cmd;
    int r=PLAYER_SUCCESS;
    play_para_t *player_para;
    player_para=player_open_pid_data(pid);
    
    if(player_para==NULL)
        return PLAYER_NOT_VALID_PID;	

    if(player_para->pFormatCtx)
    	av_ioctrl(player_para->pFormatCtx, AVIOCTL_STOP, 0, 0);
    cmd=message_alloc();
    if(cmd)
    {   
        cmd->ctrl_cmd = CMD_STOP;
		ffmpeg_interrupt();
        r=send_message(player_para,cmd); 
    }
    else
    {
        r= PLAYER_NOMEM;
    }      
    player_close_pid_data(pid);      
    return r;
}


int player_stop(int pid)
{
    player_cmd_t *cmd;
    int r=PLAYER_SUCCESS;
    play_para_t *player_para;
	log_print("[player_stop:enter]pid=%d\n", pid); 
    player_para=player_open_pid_data(pid);
    
    if(player_para==NULL)
        return PLAYER_NOT_VALID_PID;	

    if(player_para->pFormatCtx)
    av_ioctrl(player_para->pFormatCtx, AVIOCTL_STOP, 0, 0);
	ffmpeg_interrupt();
    cmd=message_alloc();
    if(cmd)
    {   
        cmd->ctrl_cmd = CMD_STOP;
		ffmpeg_interrupt();
        r=send_message(player_para,cmd); 
        r = player_thread_wait_exit(player_para);
        log_print("[player_stop:%d]wait player_theadpid[%d] r=%d\n",__LINE__,player_para->player_id,r);
        clear_all_message(player_para);
		ffmpeg_uninterrupt();
    }
    else
    {
        r= PLAYER_NOMEM;
    }      
    player_close_pid_data(pid);    
	log_print("[player_stop:exit]pid=%d\n", pid); 
    return r;
}

int player_exit(int pid)
{   
    log_print("[player_exit:enter]pid=%d\n", pid);   
    int ret=PLAYER_SUCCESS;
    play_para_t *para;
    para=player_open_pid_data(pid);
	if(para != NULL)
	{
        if(get_player_state(para)!=PLAYER_PLAYEND)
        {
        	player_stop(pid);
        }
        ret = player_thread_wait_exit(para);
        log_print("[player_exit:%d]pthread_join return: %d\n",__LINE__,ret);    	   
	}
    player_close_pid_data(pid);
    player_release(pid);
	enable_freescale();
	log_print("[player_exit:exit]pid=%d\n", pid);   
    return ret;
}

int player_pause(int pid)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_pause:enter]pid=%d\n", pid);   
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.ctrl_cmd = CMD_PAUSE;
	ret = player_send_message(pid,&cmd);
	log_print("[player_pause:exit]pid=%d ret=%d\n", pid,ret);
    return ret;    	
}

int player_resume(int pid)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_resume:enter]pid=%d\n", pid); 
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.ctrl_cmd = CMD_RESUME;
	ret = player_send_message(pid,&cmd);
	log_print("[player_resume:exit]pid=%d ret=%d\n", pid,ret);
    return ret;
}

int player_loop(int pid)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_loop:enter]pid=%d\n", pid); 
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.set_mode = CMD_LOOP;
	ret = player_send_message(pid,&cmd);
	log_print("[player_loop:exit]pid=%d ret=%d\n", pid,ret); 
    return ret;
}

int player_noloop(int pid)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_loop:enter]pid=%d\n", pid);
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.set_mode = CMD_NOLOOP;
	ret = player_send_message(pid,&cmd);
	log_print("[player_loop:exit]pid=%d ret=%d\n", pid,ret);
    return ret;
}

int player_timesearch(int pid,int s_time)
{	
    player_cmd_t cmd;
	int ret;
	log_print("[player_timesearch:enter]pid=%d s_time=%d\n", pid,s_time);
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.ctrl_cmd = CMD_SEARCH;
    cmd.param=s_time;
	ret = player_send_message(pid,&cmd);
	log_print("[player_timesearch:exit]pid=%d ret=%d\n", pid,ret);
    return ret;
}

int player_forward(int pid,int speed)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_forward:enter]pid=%d speed=%d\n", pid,speed);
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.ctrl_cmd = CMD_FF;
    cmd.param= speed;
	ret = player_send_message(pid,&cmd);
	log_print("[player_forward:exit]pid=%d ret=%d\n", pid,ret);
    return ret;
}

int player_backward(int pid,int speed)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_backward:enter]pid=%d speed=%d\n", pid,speed);
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.ctrl_cmd = CMD_FB;
    cmd.param=speed;
	ret = player_send_message(pid,&cmd);
    log_print("[player_backward]cmd=%x param=%d ret=%d\n",cmd.ctrl_cmd, cmd.param,ret);
    return ret;
}

int player_aid(int pid,int audio_id)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_aid:enter]pid=%d aid=%d\n", pid,audio_id);
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.ctrl_cmd = CMD_SWITCH_AID;
    cmd.param=audio_id;
	ret = player_send_message(pid,&cmd);
	log_print("[player_aid:exit]pid=%d ret=%d\n", pid,ret);
    return ret;

}

int player_sid(int pid,int sub_id)
{
    player_cmd_t cmd;
	int ret;
	log_print("[player_sid:enter]pid=%d sub_id=%d\n", pid,sub_id);
    MEMSET(&cmd, 0, sizeof(player_cmd_t));
    cmd.ctrl_cmd = CMD_SWITCH_SID;
    cmd.param=sub_id;
	ret = player_send_message(pid,&cmd);
	log_print("[player_sid:enter]pid=%d sub_id=%d\n", pid,sub_id);
    return ret;

}

player_status player_get_state(int pid)
{
    player_status status;
    play_para_t *player_para;

    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
    return PLAYER_NOT_VALID_PID;
    status=get_player_state(player_para);
    player_close_pid_data(pid);
    return status;
}
unsigned long player_get_extern_priv(int pid)
{
    unsigned long externed;
    play_para_t *player_para;

    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
    return 0;/*this data is 0 for default!*/
    externed=player_para->extern_priv;
    player_close_pid_data(pid);
    return externed;
}

int player_get_play_info(int pid,player_info_t *info)
{
    play_para_t *player_para;
    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
    return PLAYER_FAILED;/*this data is 0 for default!*/
    MEMSET(info, 0, sizeof(player_info_t));
    MEMCPY(info, &player_para->state, sizeof(player_info_t));	   
    player_close_pid_data(pid);   
    return PLAYER_SUCCESS;
}
int player_get_media_info(int pid,media_info_t *minfo)
{
    play_para_t *player_para;
    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
        return PLAYER_FAILED;/*this data is 0 for default!*/
    MEMSET(minfo, 0, sizeof(media_info_t));
    MEMCPY(minfo, &player_para->media_info, sizeof(media_info_t));	
	log_print("[player_get_media_info]video_num=%d vidx=%d\n",minfo->stream_info.total_video_num,minfo->stream_info.cur_video_index);
    player_close_pid_data(pid);  
    return PLAYER_SUCCESS;
}

int player_video_overlay_en(unsigned enable)
{
    int fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd >= 0) {
        unsigned myKeyColor = 0;
        unsigned myKeyColor_en = enable;

        if (myKeyColor_en) {
			myKeyColor = 0xff;/*set another value to solved the bug in kernel..remove later*/
			ioctl(fd, FBIOPUT_OSD_SRCCOLORKEY, &myKeyColor);
			myKeyColor = 0;
            ioctl(fd, FBIOPUT_OSD_SRCCOLORKEY, &myKeyColor);
            ioctl(fd, FBIOPUT_OSD_SRCKEY_ENABLE, &myKeyColor_en);
        } else {
            ioctl(fd, FBIOPUT_OSD_SRCKEY_ENABLE, &myKeyColor_en);
        }
        close(fd);
        return 0;
    }
    return -1;
}

int audio_set_mute(int pid,int mute_on)
{

    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;    
    player_para=player_open_pid_data(pid);
    if(player_para!=NULL)
    {
        player_para->playctrl_info.audio_mute = mute_on & 0x1;
        log_print("[audio_set_mute:%d]muteon=%d audio_mute=%d\n",__LINE__,mute_on,player_para->playctrl_info.audio_mute);
        p = get_audio_codec(player_para); 
        if(p!=NULL)
        ret = codec_set_mute(p,mute_on);
        player_close_pid_data(pid);    
    }
    else
        ret = codec_set_mute(NULL, mute_on);
    return ret;


}
int audio_get_volume_range(int pid,int *min,int *max)
{    
    return codec_get_volume_range(NULL, min, max);   
}
int audio_set_volume(int pid,int val)
{    
    return codec_set_volume(NULL,val);  
}
int audio_get_volume(int pid)
{    
    int r;
    r = codec_get_volume(NULL); 
    log_print("[audio_get_volume:%d]r=%d\n",__LINE__,r);
    return r;//codec_get_volume(NULL);   
}

int audio_set_volume_balance(int pid,int balance)
{    
    return codec_set_volume_balance(NULL,balance);  
}

int audio_swap_left_right(int pid)
{   
    return codec_swap_left_right(NULL);
}

int audio_left_mono(int pid)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
    return 0;/*this data is 0 for default!*/   

    p = get_audio_codec(player_para);   
    if(p!=NULL)
    ret = codec_left_mono(p);
    player_close_pid_data(pid);
    return ret;
}

int audio_right_mono(int pid)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
    return 0;/*this data is 0 for default!*/   

    p = get_audio_codec(player_para);   
    if(p!=NULL)
    ret = codec_right_mono(p);
    player_close_pid_data(pid);
    return ret;
}

int audio_stereo(int pid)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
    return 0;/*this data is 0 for default!*/   

    p = get_audio_codec(player_para);   
    if(p!=NULL)
    ret = codec_stereo(p);
    player_close_pid_data(pid);
    return ret;
}
int audio_set_spectrum_switch(int pid, int isStart,int interval)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para=player_open_pid_data(pid);
    if(player_para==NULL)
    return 0;/*this data is 0 for default!*/   

    p = get_audio_codec(player_para);
    if(p!=NULL) 
    ret = codec_audio_spectrum_switch(p,isStart,interval);
    player_close_pid_data(pid);
    return ret;
}
/*used for all exit,
please only call at this progress exit
Do not wait any things in this function!!
*/
int player_progress_exit(void)
{   
    codec_close_audio(NULL);
    return 0;
}

int player_list_allpid(pid_info_t *pid)
{
    char buf[MAX_PLAYER_THREADS];
    int pnum = 0;
    int i;
    pnum = player_list_pid(buf, MAX_PLAYER_THREADS);
    pid->num = pnum;
    for(i=0; i<pnum; i++)
    pid->pid[i] = buf[i];
    return 0;
}

char * player_status2str(player_status status)
{
	switch(status)
	{        
    	case PLAYER_INITING:		return "BEGIN_INIT";
    	case PLAYER_INITOK:			return "INIT_OK";
    	case PLAYER_RUNNING:	 	return "PLAYING";
        case PLAYER_BUFFERING:	 	return "BUFFERING";
    	case PLAYER_PAUSE:			return "PAUSE";
    	case PLAYER_SEARCHING:		return "SEARCH_FFFB";
    	case PLAYER_SEARCHOK:		return "SEARCH_OK";
    	case PLAYER_START:			return "START_PLAY";
    	case PLAYER_FF_END:			return "FF_END";
    	case PLAYER_FB_END:			return "FB_END";
        case PLAYER_ERROR:   		return "ERROR";
        case PLAYER_PLAYEND:      	return "PLAY_END";   	
    	case PLAYER_STOPED:			return "STOPED";    
		case PLAYER_EXIT:			return "EXIT";
		case PLAYER_PLAY_NEXT:		return "PLAY_NEXT";
    	default:
    			return "UNKNOW_STATE";
	}
}

