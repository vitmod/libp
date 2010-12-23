/*
 *m3u for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "libavutil/avstring.h"
#include "avformat.h"
#include <fcntl.h>
#if HAVE_SETMODE
#include <io.h>
#endif
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "os_support.h"
#include "file_list.h"


int m3u_format_get_line(ByteIOContext *s,char *line,int line_size)
{
    int ch;
    char *q;

    q = line;
    for(;;) {
        ch = get_byte(s);
        if (ch < 0)
            return AVERROR(EIO);
        if (ch == '\n' || ch == '\0') {
            /* process line */
            if (q > line && q[-1] == '\r')
                q--;
            *q = '\0';
			av_log(NULL, AV_LOG_INFO, "m3u_format_get_line line =%s\n",line);
            return q-line;
        } else {
            if ((q - line) < line_size - 1)
                *q++ = ch;
        }
    }
	return 0;
}

static int m3u_format_parser(struct list_mgt *mgt,ByteIOContext *s)
{ 
	unsigned  char line[1024];
	int ret;
	unsigned char *p; 
	int getnum=0;
 
	while(m3u_format_get_line(s,line,1024)>0)
	{
		p=line;
		while(*p==' ' && p!='\0' && p-line<1024) p++;
		if(*p!='#' && strlen(p)>0)
		{
			struct list_item*item;
			item=av_malloc(sizeof(struct list_item));
			if(!item)
				return AVERROR(ENOMEM);
			item->file=strdup(p); 
			item->next=NULL;
			item->size=0;
			item->start_pos=0;
			item->flags=0;
			getnum++;
			list_add_item(mgt,item);
		}
	}
	return getnum;
}


static int m3u_probe(ByteIOContext *s,const char *file)
{
	if(s)
	{
		char line[1024];
		if(m3u_format_get_line(s,line,1024)>0)
		{

			if(memcmp(line,"#EXTM3U",strlen("#EXTM3U"))==0)
			{
				av_log(NULL, AV_LOG_INFO, "get m3u flags!!\n");
				return 100;
			}
		}	
	}
	else
	{
		if((match_ext(file, "m3u"))||(match_ext(file, "m3u8"))) 
		{
			return 50;
		}
	}
	return 0;
}
 
 struct list_demux m3u_demux = {
	"m3u",
    m3u_probe,
	m3u_format_parser,
};



