/*
 * CacheHttp for threading download
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
#include <unistd.h>
#include <time.h>
#include <strings.h>
#include "internal.h"
#include "network.h"
#include "os_support.h"
#include "libavutil/opt.h"
#include <pthread.h>
#include "libavutil/fifo.h"
#include "CacheHttp.h"
#include "hls_livesession.h"

#include "bandwidth_measure.h" 

#define BUFFER_SIZE (1024*4)
#define CIRCULAR_BUFFER_SIZE (20*188*4096)
#define WAIT_TIME (10*1000)

typedef struct {
    URLContext * hd;
    unsigned char headers[BUFFER_SIZE];

    int EXIT;
    int EXITED;
    int circular_buffer_error;
    int64_t item_pos;
    AVFifoBuffer * fifo;
    list_item_t * cur_item;
    void * bandwidth_measure;
    pthread_t circular_buffer_thread;
    pthread_mutex_t  read_mutex;
    pthread_mutex_t  item_mutex;

} CacheHttpContext;

static int CacheHttp_ffurl_open_h(URLContext ** h, const char * filename, int flags, const char * headers)
{
    return ffurl_open_h(h, filename, flags,headers);
}

static int CacheHttp_ffurl_read(URLContext * h, unsigned char * buf, int size)
{
    return ffurl_read(h, buf, size);
}

static int CacheHttp_ffurl_close(URLContext * h)
{
    return ffurl_close(h);
}

static int CacheHttp_ffurl_seek(URLContext *h, int64_t pos, int whence)
{
    return ffurl_seek(h, pos, whence);
}

static void *circular_buffer_task( void *_handle);

int CacheHttp_Open(void ** handle)
{
    CacheHttpContext * s = (CacheHttpContext *)av_malloc(sizeof(CacheHttpContext));
    if(!s) {
        *handle = NULL;
        return AVERROR(EIO);
    }

    *handle = (void *)s;
    s->hd  = NULL;
    s->cur_item = NULL;
    memset(s->headers, 0x00, sizeof(s->headers));
    s->fifo = NULL;
    s->fifo = av_fifo_alloc(CIRCULAR_BUFFER_SIZE);      
    pthread_mutex_init(&s->read_mutex,NULL);
    pthread_mutex_init(&s->item_mutex,NULL);
    s->EXIT = 0;
    s->EXITED = 0;
/*       if(header) {
           int len = strlen(header);
           if (len && strcmp("\r\n", header + len - 2))
                av_log(NULL, AV_LOG_ERROR, "No trailing CRLF found in HTTP header.\n");

           av_strlcpy(s->headers, header, sizeof(s->headers));
       }
*/
    s->bandwidth_measure=bandwidth_measure_alloc(100,0);
    int ret = pthread_create(&s->circular_buffer_thread, NULL, circular_buffer_task, s);
    av_log(NULL, AV_LOG_INFO, "----------- pthread_create ret=%d",ret);

    return ret;
}

int CacheHttp_Read(void * handle, uint8_t * cache, int size)
{
    if(!handle)
        return AVERROR(EIO);
    
    CacheHttpContext * s = (CacheHttpContext *)handle;
    pthread_mutex_lock(&s->read_mutex);
    if (s->fifo) {
    	int avail;
       avail = av_fifo_size(s->fifo);
    	av_log(NULL, AV_LOG_INFO, "----------- http_read   avail=%d, size=%d ",avail,size);
       if (avail) {
           // Maximum amount available
           size = FFMIN( avail, size);
           av_fifo_generic_read(s->fifo, cache, size, NULL);
    	    pthread_mutex_unlock(&s->read_mutex);
           return size;
       } else if (!s->EXITED) {
    	    pthread_mutex_unlock(&s->read_mutex);
           usleep(WAIT_TIME);
           return AVERROR(EAGAIN);
       } else {
           pthread_mutex_unlock(&s->read_mutex);
           return 0;
       }
    }
    pthread_mutex_unlock(&s->read_mutex);
    
    return 0;
}

int CacheHttp_Reset(void * handle)
{
    if(!handle)
        return AVERROR(EIO);

    CacheHttpContext * s = (CacheHttpContext *)handle;
    s->EXIT = 1;
    pthread_join(s->circular_buffer_thread, NULL);
    if(s->fifo)
        av_fifo_reset(s->fifo);
    s->hd  = NULL;
    s->cur_item = NULL;
    s->EXIT = 0;
    s->EXITED = 0;
    int ret = pthread_create(&s->circular_buffer_thread, NULL, circular_buffer_task, s);
    av_log(NULL, AV_LOG_INFO, "-----------CacheHttp_reset : pthread_create ret=%d",ret);
    
    return ret;
}

int CacheHttp_Close(void * handle)
{
    if(!handle)
        return AVERROR(EIO);
    
    CacheHttpContext * s = (CacheHttpContext *)handle;
    s->EXIT = 1;
   
    pthread_join(s->circular_buffer_thread, NULL);
   
    av_log(NULL,AV_LOG_DEBUG,"-----------%s:%d\n",__FUNCTION__,__LINE__);
    if(s->fifo) {
    	av_fifo_free(s->fifo);
    }
    pthread_mutex_destroy(&s->read_mutex);
    pthread_mutex_destroy(&s->item_mutex);
    bandwidth_measure_free(s->bandwidth_measure);
    return 0;
}

int CacheHttp_GetSpeed(void * _handle, int * arg1, int * arg2, int * arg3)
{ 
    if(!_handle)
        return AVERROR(EIO);
  
    CacheHttpContext * s = (CacheHttpContext *)_handle; 
    int ret = bandwidth_measure_get_bandwidth(s->bandwidth_measure,arg1,arg2,arg3);	
    av_log(NULL, AV_LOG_ERROR, "download bandwidth latest=%d.%d kbps,latest avg=%d.%d k bps,avg=%d.%d kbps\n",*arg1/1000,*arg1%1000,*arg2/1000,*arg2%1000,*arg3/1000,*arg3%1000);
    return ret;
}

int CacheHttp_GetBuffedTime(void * handle)
{
    if(!handle)
        return AVERROR(EIO);

    CacheHttpContext * s = (CacheHttpContext *)handle; 
    int64_t buffed_time=0;
    if(s->cur_item && s->hd) {
        int64_t size = CacheHttp_ffurl_seek(s->hd, 0, AVSEEK_SIZE);
        if(s->cur_item->duration >= 0 && size > 0) {
            buffed_time = s->cur_item->start_time+s->item_pos*s->cur_item->duration/size;
           av_log(NULL, AV_LOG_ERROR, "----------CacheHttp_GetBufferedTime  buffed_time=%lld", buffed_time);
        } else {
            buffed_time = s->cur_item->start_time;
            if(s->cur_item && s->cur_item->flags & ENDLIST_FLAG) {
                int64_t full_time = getTotalDuration(NULL);
                buffed_time = full_time;
            }
        }
    }

    return buffed_time;
}

static void *circular_buffer_task( void *_handle)
{
    CacheHttpContext * s = (CacheHttpContext *)_handle; 
    URLContext *h = NULL;
    
    while(!s->EXIT) {

       av_log(h, AV_LOG_ERROR, "----------circular_buffer_task  item ");
	if (url_interrupt_cb()) {
		 s->circular_buffer_error = EINTR;
               goto FAIL;
	}

        pthread_mutex_lock(&s->item_mutex);
        if(h)
            CacheHttp_ffurl_close(h);
        list_item_t * item = getCurrentSegment(NULL);
        if(!item) {
            pthread_mutex_unlock(&s->item_mutex);
            usleep(WAIT_TIME);
            continue;
        }
        s->cur_item = item;
        pthread_mutex_unlock(&s->item_mutex);

        if(item->flags & ENDLIST_FLAG){      
            av_log(NULL, AV_LOG_INFO, "ENDLIST_FLAG, return 0\n");
            break;
        }
        
        int err;
        err = CacheHttp_ffurl_open_h(&h, item->file, AVIO_FLAG_READ, s->headers);
        if (err < 0) {
	      av_log(h, AV_LOG_ERROR, "----------CacheHttpContext : ffurl_open_h failed ,%d\n",err);
             goto FAIL;
        }
        
        s->hd = h;
        s->item_pos = 0;
        
        while(!s->EXIT) {

	    int left;
           int len;
	
	    if (url_interrupt_cb()) {
		 s->circular_buffer_error = EINTR;
               break;
	    }

	    pthread_mutex_lock(&s->read_mutex);
	    left = av_fifo_space(s->fifo);
           left = FFMIN(left, s->fifo->end - s->fifo->wptr);

           if( !left) {
		pthread_mutex_unlock(&s->read_mutex);
		usleep(WAIT_TIME);
            	continue;
           }

           if(s->hd) {
                if(s->EXIT){
                    pthread_mutex_unlock(&s->read_mutex);
                    break;
                }
                bandwidth_measure_start_read(s->bandwidth_measure);
                len = CacheHttp_ffurl_read(s->hd, s->fifo->wptr, left);
                bandwidth_measure_finish_read(s->bandwidth_measure,len);
           } else {
                pthread_mutex_unlock(&s->read_mutex);
                break;
           }
           
           if (len > 0) {
        	  s->fifo->wptr += len;
                if (s->fifo->wptr >= s->fifo->end)
                    s->fifo->wptr = s->fifo->buffer;
                s->fifo->wndx += len;
                s->item_pos += len;
           } else {
                pthread_mutex_unlock(&s->read_mutex);
                av_log(h, AV_LOG_ERROR, "---------- circular_buffer_task read ret <= 0");
                break;
           }
           pthread_mutex_unlock(&s->read_mutex);

	    //usleep(WAIT_TIME);

        }     
     
    }
    
FAIL:

    if(h)
        CacheHttp_ffurl_close(h);
    s->hd = NULL;
    s->EXITED = 1;
    
    return NULL;
}

