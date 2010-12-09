/************************************************
 * name	:subtitle.c
 * function	:decoder relative functions
 * data		:2010.8.11
 * author		:FFT
 * version	:1.0.0
 *************************************************/
 //header file
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include "codec_h_ctrl.h"

#include "subtitle/subtitle.h"
#include "vob_sub.h"

#define SUBTITLE_VOB      0
#define SUBTITLE_PGS      1
#define SUBTITLE_MKV_STR  2
#define SUBTITLE_MKV_VOB  3
#define SUBTITLE_FILE "/data/subtitle.db"
#define VOB_SUBTITLE_FRAMW_SIZE   (4+1+4+4+2+2+2+2+2+4+VOB_SUB_SIZE)
static off_t file_position=0;
static int  aml_sub_handle;

static unsigned short DecodeRL(unsigned short RLData,unsigned short *pixelnum,unsigned short *pixeldata)
{
	unsigned short nData = RLData;
	unsigned short nShiftNum;
	unsigned short nDecodedBits;
	
	if(nData & 0xc000) 
		nDecodedBits = 4;
	else if(nData & 0x3000) 
		nDecodedBits = 8;
	else if(nData & 0x0c00) 
		nDecodedBits = 12;
	else 
		nDecodedBits = 16;
	
	nShiftNum = 16 - nDecodedBits;
	*pixeldata = (nData >> nShiftNum) & 0x0003;
	*pixelnum = nData >> (nShiftNum + 2);
	
	return nDecodedBits;	
}

static unsigned short GetWordFromPixBuffer(unsigned short bitpos, unsigned short *pixelIn)
{
	unsigned char hi=0, lo=0, hi_=0, lo_=0;
	char *tmp = (char *)pixelIn;

	hi = *(tmp+0);
	lo = *(tmp+1);
	hi_ = *(tmp+2);
	lo_ = *(tmp+3);

	if(bitpos == 0){
		return (hi<<0x8 | lo);
	}
	else {
		return(((hi<<0x8 | lo) << bitpos) | ((hi_<<0x8 | lo_)>>(16 - bitpos)));
	}
}

unsigned char spu_fill_pixel(unsigned short *pixelIn, char *pixelOut, AML_SPUVAR *sub_frame)
{
	unsigned short nPixelNum = 0,nPixelData = 0;
	unsigned short nRLData,nBits;
	unsigned short nDecodedPixNum = 0;
	unsigned short i, j;
	unsigned short PXDBufferBitPos	= 0,WrOffset = 16;
	unsigned short change_data = 0;
    unsigned short PixelDatas[4] = {0,1,2,3};
	unsigned short rownum = sub_frame->spu_width;
	unsigned short height = sub_frame->spu_height;
	
	static unsigned short *ptrPXDWrite;
        
	memset(pixelOut, 0, VOB_SUB_SIZE/2);
	ptrPXDWrite = (unsigned short *)pixelOut;

	for (j=0; j<height/2; j++) {
		while(nDecodedPixNum < rownum){
			nRLData = GetWordFromPixBuffer(PXDBufferBitPos, pixelIn);
			nBits = DecodeRL(nRLData,&nPixelNum,&nPixelData);

			PXDBufferBitPos += nBits;
			if(PXDBufferBitPos >= 16){
				PXDBufferBitPos -= 16;
				pixelIn++;
			}
			if(nPixelNum == 0){
				nPixelNum = rownum - nDecodedPixNum%rownum;
			}
            
    		if(change_data)
    		{
                nPixelData = PixelDatas[nPixelData];
    		}
            
			for(i = 0;i < nPixelNum;i++){
				WrOffset -= 2;
				*ptrPXDWrite |= nPixelData << WrOffset;
				if(WrOffset == 0){
					WrOffset = 16;
					ptrPXDWrite++;
				}
			}
			nDecodedPixNum += nPixelNum;
		}	

		if(PXDBufferBitPos == 4) {			 //Rule 6
			PXDBufferBitPos = 8;
		}
		else if(PXDBufferBitPos == 12){
			PXDBufferBitPos = 0;
			pixelIn++;
		}
		
		if (WrOffset != 16) {
		    WrOffset = 16;
		    ptrPXDWrite++;
		}

		nDecodedPixNum -= rownum;

	}

	return 0;
}

int get_spu(AML_SPUVAR *spu, int read_sub_fd)
{
	int ret, rd_oft, wr_oft, size;
	char *spu_buf=NULL;
	unsigned current_length, current_pts, current_type;
	if(aml_sub_handle ==0){
		aml_sub_handle = codec_open_sub_read();
	}
	read_sub_fd = aml_sub_handle;

	if(read_sub_fd < 0)
		return 0;
	ret = codec_poll_sub_fd(read_sub_fd, 10);
	
	if (ret == 0){
	    CODEC_PRINT("codec_poll_sub_fd fail \n\n");
	    ret = -1;
		goto error; 
	}

	size = codec_get_sub_size_fd(read_sub_fd);
	if (size <= 0){
    ret = -1;
    	CODEC_PRINT("\n player get sub size less than zero \n\n");
		goto error; 
	}
	else{
    	CODEC_PRINT("\n malloc subtitle size %d \n\n",size);
		spu_buf = malloc(size);	
	}

	ret = codec_read_sub_data_fd(read_sub_fd, spu_buf, size);
	

	rd_oft = 0;
	if ((spu_buf[rd_oft++]!=0x41)||(spu_buf[rd_oft++]!=0x4d)||
		(spu_buf[rd_oft++]!=0x4c)||(spu_buf[rd_oft++]!=0x55)|| (spu_buf[rd_oft++]!=0xaa)){
		ret = -1;
		unsigned int spu_bad_header = spu_buf[0]<24|spu_buf[1]<16|spu_buf[2]<8|spu_buf[3];
		CODEC_PRINT("\n\n ******* wrong spu header is %x******\n\n",spu_bad_header);
		goto error; 		// wrong head
	}
	CODEC_PRINT("\n\n ******* find correct subtitle header ******\n\n");
	current_type = spu_buf[rd_oft++]<<16;
	current_type |= spu_buf[rd_oft++]<<8;
	current_type |= spu_buf[rd_oft++];

	current_length = spu_buf[rd_oft++]<<24;
	current_length |= spu_buf[rd_oft++]<<16;
	current_length |= spu_buf[rd_oft++]<<8;
	current_length |= spu_buf[rd_oft++];	
	
	current_pts = spu_buf[rd_oft++]<<24;
	current_pts |= spu_buf[rd_oft++]<<16;
	current_pts |= spu_buf[rd_oft++]<<8;
	current_pts |= spu_buf[rd_oft++];
  	CODEC_PRINT("current_pts is %d\n",current_pts);
	if (current_pts==0){
	ret = -1;
		goto error;
		}
  	CODEC_PRINT("current_type is %d\n",current_type);
	switch (current_type) {
		case 0x17000:
      spu->subtitle_type = SUBTITLE_VOB;
			spu->spu_data = malloc(VOB_SUB_SIZE);
			spu->pts = current_pts;
			ret = get_vob_spu(spu_buf+rd_oft, current_length, spu); 
			break;

		default:
      ret = -1;
			break;
	}

error:
	if (spu_buf)
		free(spu_buf);
		
	return ret;
}


int release_spu(AML_SPUVAR *spu)
{
	if(spu->spu_data)
		free(spu->spu_data);

	return 0;
}

int init_subtitle_file()
{
	file_position = 0;
	return 0;
}

/*
write subtitle to file:SUBTITLE_FILE
first 4 bytes are sync bytes:0x414d4c55(AMLU)
next  1 byte  is  subtitle type
next  4 bytes are subtitle pts
mext  4 bytes arg subtitle delay
next  2 bytes are subtitle start x pos
next  2 bytes are subtitel start y pos
next  2 bytes are subtitel width
next  2 bytes are subtitel height
next  2 bytes are subtitle alpha
next  4 bytes are subtitel size
next  n bytes are subtitle data
*/
int write_subtitle_file(AML_SPUVAR *spu)
{
	int subfile = open(SUBTITLE_FILE, O_RDWR|O_CREAT);
	if(subfile >= 0){
    CODEC_PRINT("write pos is %ll\n\n",file_position*VOB_SUBTITLE_FRAMW_SIZE);
    off_t lseek_value = lseek(subfile,file_position*VOB_SUBTITLE_FRAMW_SIZE,SEEK_SET);
    CODEC_PRINT("lseek_value is %ll\n\n",lseek_value);
		CODEC_PRINT("open subtitle file success\n\n");
		CODEC_PRINT("sync_bytes is 0x414d4c55\n\n");
		write(subfile,&spu->sync_bytes,4);
		CODEC_PRINT("subtitle_type is %d\n\n",spu->subtitle_type);
		write(subfile,&spu->subtitle_type,1);
		CODEC_PRINT("spu->pts is %d\n\n",spu->pts);
		write(subfile,&spu->pts,4);
    CODEC_PRINT("spu->m_delay is %d\n\n",spu->m_delay);
		write(subfile,&spu->pts,4);
		CODEC_PRINT("spu->spu_start_x is %d\n\n",spu->spu_start_x);
		write(subfile,&spu->spu_start_x,2);
		CODEC_PRINT("spu->spu_start_y is %d\n\n",spu->spu_start_y);
		write(subfile,&spu->spu_start_y,2);   
		CODEC_PRINT("spu->spu_width is %d\n\n",spu->spu_width);
		write(subfile,&spu->spu_width,2);
		CODEC_PRINT("spu->spu_height is %d\n\n",spu->spu_height);
		write(subfile,&spu->spu_height,2);
		CODEC_PRINT("spu->spu_alpha is %x\n\n",spu->spu_alpha);	
		write(subfile,&spu->spu_alpha,2);
		CODEC_PRINT("VOB_SUB_SIZE is %d\n\n",VOB_SUB_SIZE);		
		write(subfile,&spu->buffer_size,4);
		write(subfile,spu->spu_data,VOB_SUB_SIZE);
		CODEC_PRINT("write subtitle file success\n\n");
		close(subfile);
	}
	return 0;
}

int read_subtitle_file()
{
	int subfile = open(SUBTITLE_FILE, O_RDWR|O_CREAT);
	if(subfile >= 0){
		unsigned sync_byte = 0;
		unsigned short spu_rect = 0;
		CODEC_PRINT("read pos is %ll\n\n",file_position*VOB_SUBTITLE_FRAMW_SIZE);
		lseek(subfile,file_position*VOB_SUBTITLE_FRAMW_SIZE,SEEK_SET);
		read(subfile, &sync_byte, 4);
		CODEC_PRINT("sync bytes is %x\n\n",sync_byte); 
		unsigned char subtitle_type = 0;
    read(subfile, &subtitle_type, 1);
		CODEC_PRINT("subtitle_type is %x\n\n",subtitle_type); 
		sync_byte = 0;
		read(subfile, &sync_byte, 4);
		CODEC_PRINT("current pts is %d\n\n",sync_byte); 
		sync_byte = 0;
		read(subfile, &sync_byte, 4);
		CODEC_PRINT("subtitel delay is %d\n\n",sync_byte);
		read(subfile, &spu_rect, 2);
		CODEC_PRINT("spu_start_x is %d\n\n",spu_rect); 
		spu_rect = 0;
		read(subfile, &spu_rect, 2);
		CODEC_PRINT("spu_start_y is %d\n\n",spu_rect); 
		spu_rect = 0;
		read(subfile, &spu_rect, 2);
		CODEC_PRINT("spu_width is %d\n\n",spu_rect); 
		spu_rect = 0;
		read(subfile, &spu_rect, 2);
		CODEC_PRINT("spu_height is %d\n\n",spu_rect); 
		spu_rect = 0;
		read(subfile, &spu_rect, 2);
		CODEC_PRINT("spu_alpha is %x\n\n",spu_rect);
		sync_byte = 0;
		read(subfile, &sync_byte, 4);
		CODEC_PRINT("spu size is %d\n\n",sync_byte);  
		close(subfile);

	}
	return 0;
}

int get_inter_spu()
{  

	//int read_sub_fd = codec_open_sub_read();
	int read_sub_fd=0;
	//if(read_sub_fd < 0){
	    //CODEC_PRINT("\n sub read handle fail \n\n");
	    //return -1;
    //}
  
	AML_SPUVAR spu;
	memset(&spu,0x0,sizeof(AML_SPUVAR));
	spu.sync_bytes = 0x414d4c55;
	spu.buffer_size = VOB_SUB_SIZE;
	int ret = get_spu(&spu, read_sub_fd); 
	//codec_close_sub_fd(read_sub_fd);
	if(ret < 0)
		return -1;

	write_subtitle_file(&spu);
	read_subtitle_file();
	file_position ++;
	CODEC_PRINT("file_position is %ll\n\n",file_position);
	CODEC_PRINT("VOB_SUBTITLE_FRAMW_SIZE is %d\n\n",VOB_SUBTITLE_FRAMW_SIZE);
	CODEC_PRINT("file_position/VOB_SUBTITLE_FRAMW_SIZE is %ll\n\n",file_position/VOB_SUBTITLE_FRAMW_SIZE);
	if(file_position >= 100){
    CODEC_PRINT("file_position be set to zero\n");
    file_position = 0;
  }
	CODEC_PRINT("end parser subtitle success\n");

	return 0;
}
/*****test code*********
int test_sub_data(int fd)
{ 
	int ret, size; 
	char *buf=NULL; 

	if (fd >= 0){ 		
		ret = codec_poll_sub_fd(fd, 10); 
		if (ret != 0){ 
			size = codec_get_sub_size_fd(fd); 
			if (size > 0) 
				buf = malloc(size); 
			ret = codec_read_sub_data_fd(fd, buf, size); 
			if (buf)
				free(buf);
		}
		
		//codec_close_sub_fd(fd); 
	} 	
	return 0; 
}


int test_sub_data2(void)
{ 
	int fd = -1;
	char *buf=NULL; 

	fd = codec_open_sub_read(); 
	if (fd >= 0){ 
		codec_close_sub_fd(fd); 
	} 	
	return 0; 
}
*/


