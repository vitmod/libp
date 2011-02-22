
#ifndef PLAYER_LIST_H
#define PLAYER_LIST_H

#include "player_priv.h"

#define MAX_PATH        260
#define MAX_LIST_NUM    1024

typedef struct {
    char *list[MAX_LIST_NUM];
    unsigned int in_list_index;
    unsigned int out_list_index;
    unsigned int list_num;
} play_list_t;

typedef struct {
    play_control_t play_ctrl_para;
    unsigned long priv;
} play_list_thread_para_t;

int play_list_init();
int play_list_parser_m3u(const char *filename);
play_list_t *player_get_playlist(void);
char * play_list_get_file(void);
void play_list_release(void);
int play_list_get_line(ByteIOContext *s, char *line, int size);
int play_list_update_state(int pid, player_info_t *info);

#endif

