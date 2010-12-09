#ifndef SUBTITLE_H
#define SUBTITLE_H

//#include <libavcodec/avcodec.h>

#define VOB_SUB_SIZE 720*576/4


typedef struct 
{
 unsigned sync_bytes;
 unsigned buffer_size;
 unsigned pid;	
 unsigned pts;
 unsigned m_delay;
 unsigned char *spu_data;
 unsigned short cmd_offset;
 unsigned short length; 

 unsigned r_pt;
 unsigned frame_rdy;
 	
 unsigned short spu_color; 
 unsigned short spu_alpha; 
 unsigned short spu_start_x;
 unsigned short spu_start_y;
 unsigned short spu_width;
 unsigned short spu_height;
 unsigned short top_pxd_addr;  
 unsigned short bottom_pxd_addr; 

 unsigned disp_colcon_addr;  
 unsigned char display_pending;
 unsigned char displaying;
 unsigned char subtitle_type;
 unsigned char reser[2];   
  
} AML_SPUVAR;


int get_spu(AML_SPUVAR *spu, int sub_fd);
int release_spu(AML_SPUVAR *spu);
int get_inter_spu();
unsigned char spu_fill_pixel(unsigned short *pixelIn, char *pixelOut, AML_SPUVAR *sub_frame);


#endif
