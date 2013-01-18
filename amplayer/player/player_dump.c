#include "player_priv.h"
#include "log_print.h"

int player_dump_playinfo(int pid, int fd)
{
   log_print("player_dump_playinfo pid=%d fd=%d\n", pid, fd);
   play_para_t *player_para;    

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_error("player_dump_playinfo error: can't find match pid[%d] player\n", pid);
        return PLAYER_NOT_VALID_PID;
    }
    
    player_close_pid_data(pid);
    return 0;
}

int player_dump_bufferinfo(int pid, int fd)
{
   log_print("player_dump_bufferinfo pid=%d fd=%d\n", pid, fd);
   play_para_t *player_para;    

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_error("player_dump_bufferinfo error: can't find match pid[%d] player\n", pid);
        return PLAYER_NOT_VALID_PID;
    }
    
    player_close_pid_data(pid);
    return 0;
}

int player_dump_tsyncinfo(int pid, int fd)
{
   log_print("player_dump_tsyncinfo pid=%d fd=%d\n", pid, fd);
   play_para_t *player_para;    

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_error("player_dump_tsyncinfo error: can't find match pid[%d] player\n", pid);
        return PLAYER_NOT_VALID_PID;
    }
    
    player_close_pid_data(pid);
    return 0;
}
