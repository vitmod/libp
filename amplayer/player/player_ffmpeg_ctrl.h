#ifndef _PLAYER_FFMPEG_CTRL_H_
#define _PLAYER_FFMPEG_CTRL_H_

#include "player_priv.h"

int ffmpeg_init(void);
int ffmpeg_parse_file(play_para_t *am_p);
void ffmpeg_interrupt(void);
void ffmpeg_uninterrupt(void);
int ffmpeg_buffering_data(play_para_t *para);
#endif

