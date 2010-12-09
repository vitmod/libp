#ifndef _PLAYER_SET_DISP_H_
#define _PLAYER_SET_DISP_H_

int set_black_policy(int blackout);
int get_black_policy();
int set_tsync_enable(int enable);
int set_fb0_blank(int blank);
int set_fb1_blank(int blank);
int set_subtitle_enable(int enable);
int get_subtitle_enable();
int set_subtitle_num(int num);
int get_subtitle_num();
int set_subtitle_curr(int curr);
int get_subtitle_curr();
int set_subtitle_size(int size);
int get_subtitle_size();
int set_subtitle_data(int data);
int get_subtitle_data();

#endif
