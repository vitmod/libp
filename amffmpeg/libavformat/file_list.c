/*
 * filelist io for ffmpeg system
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
static struct list_demux *list_demux_list=NULL;
#define unused(x)	(x=x)

int register_list_demux(struct list_demux *demux)
{
	list_demux_t **list=&list_demux_list;
    while (*list != NULL) list = &(*list)->next;
    *list = demux;
    demux->next = NULL;
	return 0;
}

struct list_demux * probe_demux(ByteIOContext  *s,const char *filename)
{
	int ret;
	int max=0;
	list_demux_t *demux;
	list_demux_t *best_demux=NULL;
	demux=list_demux_list;
	while(demux)
	{
		ret=demux->probe(s,filename);
		if(ret>max)
		{
			max=ret;
			best_demux=demux;
			if(ret>=100)
			{
				return best_demux;
			}
		}
		demux=demux->next;
	}
	return best_demux;
}

int list_add_item(struct list_mgt *mgt,struct list_item*item)
{
	struct list_item**list;
	struct list_item*prev;
	list=&mgt->item_list;
	prev=NULL;
	 while (*list != NULL) 
	 {
	 	prev=*list;
	 	list = &(*list)->next;
	 }
	*list = item;
	item->prev=prev;
    item->next = NULL;
	mgt->item_num++;
	return 0;
}
static int list_del_item(struct list_mgt *mgt,struct list_item*item)
{
	struct list_item*tmp;
	if(!item || !mgt)
		return -1;
	if(mgt->item_list==item){
		mgt->item_list=item->next;
		if(mgt->item_list)
			mgt->item_list->prev=NULL;
	}
	else {
		tmp=item->prev;
		tmp->next=item->next;
		if(item->next) 
			item->next->prev=tmp;
	}
	mgt->item_num--;
	av_free(item);
	return 0;		
}

/*=======================================================================================*/
int url_is_file_list(ByteIOContext *s,const char *filename)
{
	int ret;
	list_demux_t *demux;
	ByteIOContext *lio=s;
	if(!lio)
	{
		ret=url_fopen(&lio,filename,O_RDONLY);
		if(ret!=0)
		{ 
		return AVERROR(EIO); 
		}
	}
	demux=probe_demux(lio,filename);
	if(lio!=s)
	{
		url_fclose(lio);
	}
	else
	{
		url_fseek(lio, 0, SEEK_SET);
	}
	return demux!=NULL?1:0;
}

static int list_open_internet(ByteIOContext **pbio,struct list_mgt *mgt,const char *filename, int flags)
{
	list_demux_t *demux;
	int ret;
	ByteIOContext *bio;
	ret=url_fopen(&bio,filename+5,flags);
	if(ret!=0)
		{
			av_free(mgt);
			return AVERROR(EIO); 
		}
	demux=probe_demux(bio,filename);
	if(!demux)
	{
		ret=-1;
		goto error;
	}
	ret=demux->parser(mgt,bio);
	if(ret<=0)
	{
		ret=-1;
		goto error;
	}
	*pbio=bio;
	return 0;
error:
	if(bio)
		url_fclose(bio);
	av_free(mgt);
	return ret;
}

static int list_open(URLContext *h, const char *filename, int flags)
{
	struct list_mgt *mgt;
	int ret;
	ByteIOContext *bio;
	mgt=av_malloc(sizeof(struct list_mgt));
	if(!mgt)
		return AVERROR(ENOMEM);
	memset(mgt,0,sizeof(*mgt));
	if((ret=list_open_internet(&bio,mgt,filename,flags| URL_MINI_BUFFER | URL_NO_LP_BUFFER))!=0)
		return ret;
	lp_lock_init(&mgt->mutex,NULL);
	mgt->current_item=mgt->item_list;
	mgt->cur_uio=NULL;
	mgt->filename=filename;
	mgt->flags=flags;
 	h->is_streamed=1;
	h->is_slowmedia=1;
	h->priv_data = mgt;
	
	url_fclose(bio);
	return 0;
}

static struct list_item * switchto_next_item(struct list_mgt *mgt)
{
	struct list_item *next=NULL;
	struct list_item *current;
	if(!mgt || !mgt->current_item)
		return NULL;
	if(mgt->current_item->next==NULL){
			/*new refresh this mgtlist now*/
			ByteIOContext *bio;
			
			int ret;
			if((ret=list_open_internet(&bio,mgt,mgt->filename,mgt->flags| URL_MINI_BUFFER | URL_NO_LP_BUFFER))!=0)
			{
				goto switchnext;
			}
			url_fclose(bio);
			current=mgt->current_item;
			next=mgt->current_item->next;
			for(;next!=NULL;next=next->next){
				if(strcmp(current->file,next->file)==0){
					/*found the same item,switch to the next*/	
					current=next;
					break;
				}
			}
			while(current!=mgt->item_list){
				/*del the old item,lest current,and then play current->next*/
				list_del_item(mgt,mgt->item_list);
			}
			mgt->current_item=mgt->item_list;
			
	}
switchnext:	
	mgt->current_item=mgt->current_item->next;
	next=mgt->current_item;
	if(next)
		av_log(NULL, AV_LOG_INFO, "switch to new file=%s,total=%d",next->file,mgt->item_num);
	else
		av_log(NULL, AV_LOG_INFO, "switch to new file=NULL,total=%d\n",mgt->item_num);
	return next;
}

static int list_read(URLContext *h, unsigned char *buf, int size)
{   
	struct list_mgt *mgt = h->priv_data;
    int len=AVERROR(EIO);
	struct list_item *item=mgt->current_item;
retry:	
	//av_log(NULL, AV_LOG_INFO, "list_read start buf=%x,size=%d\n",buf,size);
	if(!mgt->cur_uio )
	{
		if(item && item->file)
		{
			ByteIOContext *bio;
			av_log(NULL, AV_LOG_INFO, "list_read switch to new file=%s\n",item->file);
			len=url_fopen(&bio,item->file,O_RDONLY | URL_MINI_BUFFER | URL_NO_LP_BUFFER);
			if(len!=0)
			{
				return len;
			}
			if(url_is_file_list(bio,item->file))
			{
				/*have 32 bytes space at the end..*/
				memmove(item->file+5,item->file,strlen(item->file)+1);
				memcpy(item->file,"list:",5);
				url_fclose(bio);
				len=url_fopen(&bio,item->file,mgt->flags | URL_MINI_BUFFER | URL_NO_LP_BUFFER);
				if(len!=0)
				{
					return len;
				}
			}
			mgt->cur_uio=bio;
		}
		else
		{
			/*follow to next....*/
		}
	}
	len=get_buffer(mgt->cur_uio,buf,size);
	if((len<=0 || mgt->cur_uio->eof_reached)&& mgt->current_item!=NULL)
	{/*end of the file*/
		url_fclose(mgt->cur_uio);
		mgt->cur_uio=NULL;
		item=switchto_next_item(mgt);
		if(!item)
			return -1;/*get error*/
		if(item->flags & ENDLIST_FLAG){
			av_log(NULL, AV_LOG_INFO, "reach list end now!,item=%x\n",item);
			return len;
		}
		else if(item->flags & DISCONTINUE_FLAG){
			av_log(NULL, AV_LOG_INFO, "Discontiue item \n");
			//1 TODO:need to notify uper level stream is changed
			return 0;
		}
		else{	
			goto retry;
		}
	}
	//av_log(NULL, AV_LOG_INFO, "list_read end buf=%x,size=%d\n",buf,size);
    return len;
}

static int list_write(URLContext *h, unsigned char *buf, int size)
{    
	unused(h);
	unused(buf);
	unused(size);
	return -1;
}
/* XXX: use llseek */
static int64_t list_seek(URLContext *h, int64_t pos, int whence)
{
	unused(h);
	unused(pos);
	unused(whence);
	return -1;
}
static int list_close(URLContext *h)
{
	struct list_mgt *mgt = h->priv_data;
	struct list_item *item,*item1;
	if(!mgt)return 0;
	item=mgt->item_list;
	while(item!=NULL)
		{
		item1=item;
		item=item->next;
		av_free(item1);
	
		}
	av_free(mgt);
	unused(h);
	return 0;
}
static int list_get_handle(URLContext *h)	
{    
return (intptr_t) h->priv_data;
}

URLProtocol file_list_protocol = {
    "list",
    list_open,
    list_read,
    list_write,
    list_seek,
    list_close,
    .url_get_file_handle = list_get_handle,
};

URLProtocol *get_file_list_protocol(void)
{
	return &file_list_protocol;
}
int register_list_demux_all(void)
{
	static int registered_all=0;
	if(registered_all)	
		return;
	registered_all++;
	extern struct list_demux m3u_demux;
	av_register_protocol(&file_list_protocol); 
	register_list_demux(&m3u_demux); 
	return 0;
}

