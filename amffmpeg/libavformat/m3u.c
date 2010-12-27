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

#define EXTM3U						"#EXTM3U"
#define EXTINF						"#EXTINF"
#define EXT_X_TARGETDURATION		"#EXT-X-TARGETDURATION"

#define EXT_X_MEDIA_SEQUENCE		"#EXT-X-MEDIA-SEQUENCE"
#define EXT_X_KEY					"#EXT-X-KEY"
#define EXT_X_PROGRAM_DATE_TIME		"#EXT-X-PROGRAM-DATE-TIME"
#define EXT_X_ALLOW_CACHE			"#EXT-X-ALLOW-CACHE"
#define EXT_X_ENDLIST				"#EXT-X-ENDLIST"
#define EXT_X_STREAM_INF			"#EXT-X-STREAM-INF"

#define is_TAG(l,tag)	(!strncmp(l,tag,strlen(tag)))

struct m3u_info{
	int duration;
	int sequence;
	int allow_cache;
	int endlist;
	char *key;
	char *data_time;
	char *streaminfo;
	char *file;
};

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

static int m3u_parser_line(struct list_mgt *mgt,unsigned char *line,struct list_item*item)
{
	unsigned char *p=line; 
	p=line;
	while(*p==' ' && p!='\0' && p-line<1024) p++;
	if(*p!='#' && strlen(p)>0)
	{

		item->file=p; 
		item->next=NULL;
		item->size=0;
		item->start_pos=0;
		item->flags=0;
	}
	else if(is_TAG(p,EXT_X_ENDLIST)){
		item->file=NULL; 
		item->next=NULL;
		item->size=0;
		item->start_pos=0;
		item->flags=ENDLIST_FLAG;
	}
	else{
		return -1;
	}
	return 0;
}


static int m3u_format_parser(struct list_mgt *mgt,ByteIOContext *s)
{ 
	unsigned  char line[1024];
	int ret;
	unsigned char *p; 
	int getnum=0;
	struct list_item tmpitem;
 	
	while(m3u_format_get_line(s,line,1024)>0)
	{
		if(m3u_parser_line(mgt,line,&tmpitem)==0)
		{
			struct list_item*item;
			int size_file=tmpitem.file?(strlen(tmpitem.file)+32):4;
			item=av_malloc(sizeof(struct list_item)+size_file);
			if(!item)
				return AVERROR(ENOMEM);
			memcpy(item,&tmpitem,sizeof(tmpitem));
			item->file=NULL;
			if(tmpitem.file)
				{
				item->file=&item[1];
				strcpy(item->file,tmpitem.file);
				}
			list_add_item(mgt,item);
			getnum++;
		}
		
	}
	av_log(NULL, AV_LOG_INFO, "m3u_format_parser end num =%d\n",getnum);
	return getnum;
}


static int m3u_probe(ByteIOContext *s,const char *file)
{
	if(s)
	{
		char line[1024];
		if(m3u_format_get_line(s,line,1024)>0)
		{

			if(memcmp(line,EXTM3U,strlen(EXTM3U))==0)
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



