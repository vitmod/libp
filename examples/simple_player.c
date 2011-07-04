
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <player.h>
#include <log_print.h>
#include <player_set_sys.h>
//#include <version.h>

#define SP_EXIT	(0x10000)

static void help_usage(void)
{
	log_print("\n-----------Player Control Command:----------------\n");
	log_print("\tx:quit\n");
	log_print("\ts:stop\n");
	log_print("\tp:pause\n");
	log_print("\tr:resume\n");
    log_print("\tt:search\n");
    log_print("\tf:forward, fast forward\n");
    log_print("\tb:backward, fast backward\n");
	log_print("\tm:mute\n");
	log_print("\tM:unmute\n");
	log_print("\tv:volget,get currnet sound volume\n");
	log_print("\tV:volset(V:val),set currnet sound volume\n");
    log_print("\ta:aid, set new audio id to play\n");
    log_print("\tu:spectrum, audio switch spectrum\n");
    log_print("\tw:swap, swap audio left and right\n");
    log_print("\tl:lmono, audio set left mono\n");
    log_print("\tL:rmono, audio set right mono\n");
    log_print("\to:stereo, audio set stereo\n");
    log_print("\tc:curtime, get current playing time\n");
    log_print("\td:duration, get stream duration\n");
    log_print("\tS:status, get player status\n");
    log_print("\ti:media, get media infomation\n");   
    log_print("\tk:pid, list all valid player id\n");
    log_print("\ty:sid, set new subtitle id\n");  
	log_print("\th:help\n");
}

static int get_command(int pid)
{

	int  ret;
    fd_set rfds;
    struct timeval tv;
	int fd=fileno(stdin);
    
#define is_CMD(str,cmd,s_cmd1,s_cmd2) (strcmp(str,cmd)==0 || strcmp(str,s_cmd1)==0 || strcmp(str,s_cmd2)==0)

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 3 * 1000 * 1000;       
	ret = select(fd+1, &rfds, NULL, NULL, &tv);
   
	if(ret > 0 && FD_ISSET(fd, &rfds))
	{            
    	char str[100]={0};
    	scanf("%s",str);
    	//gets(str);
    	if(is_CMD(str,"quit","x","X"))
		{
		 	player_exit(pid);			
			ret = SP_EXIT;
		}
	    else if(is_CMD(str,"stop","s","s"))
		{
		    player_stop(pid);
			ret = SP_EXIT;
		}
	    else if(is_CMD(str,"pause","p","P"))
		{
		    ret = player_pause(pid);             
		}
	    else if(is_CMD(str,"resume","r","R"))
		{
		    ret = player_resume(pid);
		}
	    else if(is_CMD(str,"mute","m","m"))
		{			
            ret = audio_set_mute(pid, 1);
		}
	    else if(is_CMD(str,"unmute","M","M"))
		{		
            ret = audio_set_mute(pid, 0);
		}
	    else if(is_CMD(str,"volget","v","v"))
		{           
            ret = audio_get_volume(pid);
		}
	    else if(memcmp(str,"volset:",strlen("volset:"))==0 ||
			(str[0]=='V' && str[1]==':'))
    	{            
    		int r=-1;
    		char *p;
    		p=strstr(str,":");
    		if(p!=NULL)
			{
    			p++;
    			sscanf(p,"%d",&r);  
                ret = audio_set_volume(pid, r);
			}
    		else
			{
			    log_print("error command for set vol(fmt:\"V:value\")\n");
			}           
    	}
        else if(memcmp(str,"search:",strlen("search:"))==0 ||
			(str[0]=='t' && str[1]==':')||
			(str[0]=='T' && str[1]==':'))
		{            
    		int r=-1;
    		char *p;            
    		p=strstr(str,":");
    		if(p!=NULL)
			{
    			p++;
    			sscanf(p,"%d",&r);                
                ret = player_timesearch(pid, r);
			}
    		else
			{
			    log_print("error command for search(fmt:\"T:value\")\n");
			}            
		}
        else if(memcmp(str,"forward:",strlen("forward:"))==0 ||
			(str[0]=='f' && str[1]==':')||
			(str[0]=='F' && str[1]==':'))
		{
    		int r=-1;
    		char *p;            
    		p=strstr(str,":");
    		if(p!=NULL)
			{
    			p++;
    			sscanf(p,"%d",&r);                 
                ret = player_forward(pid, r);
			}
    		else
			{
			    log_print("error command for forward(fmt:\"F:value\")\n");
			}
		}
        else if(memcmp(str,"backward:",strlen("backward:"))==0 ||
			(str[0]=='b' && str[1]==':')||
			(str[0]=='B' && str[1]==':'))
		{
    		int r=-1;
    		char *p;            
    		p=strstr(str,":");
    		if(p!=NULL)
			{
    			p++;
    			sscanf(p,"%d",&r);                  
                ret = player_backward(pid, r);
			}
    		else
			{
			    log_print("error command for backward(fmt:\"B:value\")\n");
			}
		}
        else if(memcmp(str,"aid:",strlen("aid:"))==0 ||
			(str[0]=='a' && str[1]==':'))
		{
    		int r=-1;
    		char *p;            
    		p=strstr(str,":");
    		if(p!=NULL)
			{
    			p++;
    			sscanf(p,"%d",&r);                  
                ret = player_aid(pid, r);
			}
    		else
			{
			    log_print("error command for audio id(fmt:\"a:value\")\n");
			}
		}
        else if(memcmp(str,"sid:",strlen("sid:"))==0 ||
			(str[0]=='y' && str[1]==':'))
		{
    		int r=-1;
    		char *p;            
    		p=strstr(str,":");
    		if(p!=NULL)
			{
    			p++;
    			sscanf(p,"%d",&r);                  
                ret = player_sid(pid, r);              
			}
    		else
			{
			    log_print("error command for subtitle id(fmt:\"y:value\")\n");
			}
		}
        else if(is_CMD(str, "volrange", "g", "G"))
        {
        	int min, max;
            audio_get_volume_range(pid, &min, &max);
			log_print("get audio volume range: min = %d max = %d \n", min, max);
        }
        else if(is_CMD(str, "spectrum", "u", "U"))
        {        	
            audio_set_spectrum_switch(pid, 0, 0);
        }
        else if(is_CMD(str, "balance", "e", "E"))
        {
            ret = audio_set_volume_balance(pid, 1);
        }
        else if(is_CMD(str, "swap", "w", "W"))
        {
            ret = audio_swap_left_right(pid);
        }       
        else if(is_CMD(str, "lmono", "l", "l"))
        {
            ret = audio_left_mono(pid);
        }
        else if(is_CMD(str, "rmono", "L", "L"))
        {
            ret = audio_right_mono(pid);
        }
        else if(is_CMD(str, "setreo", "o", "O"))
        {
            ret = audio_stereo(pid);
        }
        else if(is_CMD(str, "curtime", "c", "C"))
        {
        	player_info_t info;
            player_get_play_info(pid, &info);
			ret = info.current_time;
        }
        else if(is_CMD(str, "duration", "d", "D"))
        {
            player_info_t info;
            player_get_play_info(pid, &info);
			ret = info.full_time;
        }
        else if(is_CMD(str, "status", "S", "S"))
        {
            player_info_t info;
            player_get_play_info(pid, &info);
			ret = info.status;
        }
        else if(is_CMD(str, "media", "i", "I"))
        {
            media_info_t minfo;
			player_get_media_info(pid, &minfo);
        }
        else if(is_CMD(str, "pid", "k", "K"))
        {
        	pid_info_t lpid;
            player_list_allpid(&lpid);
        }        
    	else if(is_CMD(str,"help","h","H"))
    	{
    	    help_usage();
    	}
    	else
    	{
        	if(strlen(str) > 1) // not only enter
        	     help_usage();
    	}
	}  
    else if(ret < 0) {
        log_print("[get_command] select error!\n");
        return 0;
	}
    return ret;
}

int main(int argc,char ** argv)
{
		play_control_t ctrl;
		int pid;
		
		if(argc<2)
		{
			printf("USAG:%s file\n",argv[0]);
			return 0;
		}
		player_init();
		set_fb0_blank(1);
		memset(&ctrl,0,sizeof(ctrl));
		ctrl.file_name=argv[1];
		ctrl.video_index=-1;
		ctrl.audio_index=-1;
		ctrl.sub_index=-1;
		pid=player_start(&ctrl,0);
		if(pid<0)
		{
			printf("play failed=%d\n",pid);
			return -1;
		}
		while(!player_thread_stop(player_get_state(pid))) {
			if (get_command(pid) == SP_EXIT) 
			{
				break;
			}
			set_fb0_blank(1);
		}
		usleep(10000);		
		player_exit(pid);
		printf("play end=%d\n",pid);
		return 0;
}
