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
#include "amconfigutils.h"
#include "bandwidth_measure.h"
#include "hls_livesession.h"
#include "CacheHttp.h"
#include "libavutil/lfg.h"
#include "http.h"
static struct list_demux *list_demux_list = NULL;
#define unused(x)   (x=x)

#define LIVE_LIST_MAX 120
#define SHRINK_LIVE_LIST_THRESHOLD (LIVE_LIST_MAX/3)
static int list_shrink_live_list(struct list_mgt *mgt);


#define SESSLEN 256
static int generate_playback_session_id(char * ssid, int size);
static int generate_segment_session_id(char * ssid, int size);

static float get_adaptation_ex_para(int type);
#define HLS_RATETAG "amffmpeg-hls"

#define RLOG(...) av_tag_log(HLS_RATETAG,__VA_ARGS__)

int generate_playback_session_id(char * ssid, int size)
{
    AVLFG rnd;
    char *ex_id = "314F1B49-AA3C-4715-950D-";
    unsigned int session_id = 0;
    char session_name[SESSLEN] = "";
    av_lfg_init(&rnd, 0xDEADC0DE);
    session_id =  av_lfg_get(&rnd);
    snprintf(session_name, SESSLEN, "%s%012u", ex_id, session_id);
    if (ssid != NULL) {
        snprintf(ssid, size, "%s", session_name);
        return 0;
    }
    return -1;
}

int generate_segment_session_id(char * ssid, int size)
{
    AVLFG rnd;
    char *ex_id = "E4499E08-D4C5-4DB0-A21C-";
    unsigned int session_id = 0;
    char session_name[SESSLEN] = "";
    av_lfg_init(&rnd, 0xDEADC0DE);
    session_id =  av_lfg_get(&rnd);
    snprintf(session_name, SESSLEN, "%s%012u", ex_id, session_id);
    if (ssid != NULL) {
        snprintf(ssid, size, "%s", session_name);
        return 0;
    }
    return -1;
}


int register_list_demux(struct list_demux *demux)
{
    list_demux_t **list = &list_demux_list;
    while (*list != NULL) {
        list = &(*list)->next;
    }
    *list = demux;
    demux->next = NULL;
    return 0;
}

struct list_demux * probe_demux(ByteIOContext  *s, const char *filename) {
    int ret;
    int max = 0;
    list_demux_t *demux;
    list_demux_t *best_demux = NULL;
    demux = list_demux_list;
    while (demux) {
        ret = demux->probe(s, filename);
        if (ret > max) {
            max = ret;
            best_demux = demux;
            if (ret >= 100) {
                return best_demux;
            }
        }
        demux = demux->next;
    }
    return best_demux;
}

int list_add_item(struct list_mgt *mgt, struct list_item*item)
{
    struct list_item**list;
    struct list_item*prev;
    list = &mgt->item_list;
    prev = NULL;
    while (*list != NULL) {
        prev = *list;
        list = &(*list)->next;
    }
    *list = item;
    item->prev = prev;
    item->next = NULL;
    mgt->item_num++;
    item->index = mgt->next_index;
    mgt->next_index++;
    av_log(NULL, AV_LOG_INFO, "list_add_item:seq=%d,index=%d\n", item->seq, item->index);
    return 0;
}
static struct list_item* list_test_find_samefile_item(struct list_mgt *mgt, struct list_item *item) {
    struct list_item **list;

    list = &mgt->item_list;
    if (item->file != NULL) {
        while (*list != NULL) {
            if ((item->seq >= 0  && (item->seq == (*list)->seq)) && (!mgt->have_list_end&& !strcmp((*list)->file, item->file))) {
                /*same seq or same file name*/
                return *list;
            }
            list = &(*list)->next;
        }
    }
    return NULL;
}

static struct list_item* list_find_item_by_index(struct list_mgt *mgt, int index) {
    struct list_item **list;

    list = &mgt->item_list;
    int next_index  = 0;
    if (index < mgt->item_num) {
        next_index = index + 1;
    } else {
        av_log(NULL, AV_LOG_INFO, "just return last item,index:%d\n", index);
        next_index = index;
    }
    if (index >= -1) {
        while (*list != NULL) {
            if ((*list)->index == next_index) {
                /*same index*/
                av_log(NULL, AV_LOG_INFO, "find next item,index:%d\n", index);
                return *list;
            }
            list = &(*list)->next;
        }
    }
    return NULL;
}
static struct list_item* list_find_item_by_seq(struct list_mgt *mgt, int seq) {
    struct list_item **list;
    int next_seq = seq + 1;
    list = &mgt->item_list;
    if (next_seq >= 0) {
        while (*list != NULL) {
            if ((*list)->seq == next_seq || (*list)->seq == (next_seq + 1) || (*list)->seq == (next_seq + 2) || (*list)->seq == (next_seq + 3)) {
                /*same seq*/
                av_log(NULL, AV_LOG_INFO, "find item,seq:%d\n", seq);
                return *list;
            }
            list = &(*list)->next;
        }
    }
    return NULL;
}
int list_test_and_add_item(struct list_mgt *mgt, struct list_item*item)
{
    struct list_item *sameitem;
    sameitem = list_test_find_samefile_item(mgt, item);
    if (sameitem) {
        av_log(NULL, AV_LOG_INFO, "list_test_and_add_item found same item\nold:%s[seq=%d]\nnew:%s[seq=%d]", sameitem->file, sameitem->seq, item->file, item->seq);
        return -1;/*found same item,drop it */
    }
    av_log(NULL,AV_LOG_INFO,"add this item,seq number:%d,start:%.4lf,duration:%.4lf\n",item->seq,item->start_time,item->duration);
    list_add_item(mgt, item);
    return 0;
}


static int list_del_item(struct list_mgt *mgt, struct list_item*item)
{

    struct list_item*tmp;
    if (!item || !mgt) {
        return -1;
    }
    if (mgt->item_list == item) {
        mgt->item_list = item->next;
        if (mgt->item_list) {
            mgt->item_list->prev = NULL;
        }
    } else {
        tmp = item->prev;
        tmp->next = item->next;
        if (item->next) {
            item->next->prev = tmp;
        }
    }
    mgt->item_num--;
    if (item->ktype == KEY_AES_128) {
        av_free(item->key_ctx);
    }

    av_free(item);
    item = NULL;
    return 0;
}

static int list_shrink_live_list(struct list_mgt *mgt)
{
    struct list_item **tmp = NULL;
    if (NULL == mgt) {
        return -1;
    }
    tmp = &mgt->item_list;
    if (!mgt->have_list_end) { //not have list end,just refer to live streaming
        while (mgt->item_num > SHRINK_LIVE_LIST_THRESHOLD && *tmp != mgt->current_item) {
            list_del_item(mgt, *tmp);
            tmp = &mgt->item_list;
        }
    }
    av_log(NULL, AV_LOG_INFO, "shrink live item from list,total:%d\n", mgt->item_num);
    return 0;
}

static int list_delall_item(struct list_mgt *mgt)
{
    struct list_item *p = NULL;
    struct list_item *t = NULL;
    if (NULL == mgt) {
        return -1;
    }

    p = mgt->item_list;

    while (p != NULL) {
        t = p->next;
        mgt->item_num--;
        if (p->ktype == KEY_AES_128) {           
            av_free(p->key_ctx);
        }

        av_free(p);

        p = NULL;

        p = t;
    }
    mgt->item_list = NULL;
    av_log(NULL, AV_LOG_INFO, "delete all items from list,total:%d\n", mgt->item_num);
    return 0;

}

/*=======================================================================================*/
int url_is_file_list(ByteIOContext *s, const char *filename)
{
    int ret;
    list_demux_t *demux;
    ByteIOContext *lio = s;
    int64_t    *oldpos = 0;
    if (am_getconfig_bool("media.amplayer.usedm3udemux")) {
        return 0;    /*if used m3u demux,always failed;*/
    }
    if (!lio) {
        ret = url_fopen(&lio, filename, AVIO_FLAG_READ | URL_MINI_BUFFER|URL_NO_LP_BUFFER);
        if (ret != 0) {
            return AVERROR(EIO);
        }
    } else {
        oldpos = url_ftell(lio);
    }
    demux = probe_demux(lio, filename);
    if (lio != s) {
        url_fclose(lio);
    } else {
        url_fseek(lio, oldpos, SEEK_SET);
    }
    return demux != NULL ? 100 : 0;
}

#define AUDIO_BANDWIDTH_MAX 100000  //100k
static int  fast_sort_streams(struct list_mgt * mgt){
    int key,left,middle,right;  
    struct variant *var= NULL;
    int i,j,tmp,stuff[mgt->n_variants];
    for (i=0; i < mgt->n_variants; i++){
        var = mgt->variants[i];
        stuff[i] = var->bandwidth;
       
    }
    for (i=1; i < mgt->n_variants; i++){
        key = stuff[i];
        left = 0;
        right = i-1;
        while(left<=right){
            middle=(left+right)/2;             
            if (key>stuff[middle]) {//down order
                right=middle-1;
            }else{
                left=middle+1;
            }         
            
        }
        for (j=i;j>left;j--){
            stuff[j]=stuff[j-1];
        }
        stuff[left]=key; 
    }

    j = mgt->n_variants - 1;
    for (i=0; i < mgt->n_variants; i++){
        for (tmp=0; tmp <  mgt->n_variants; tmp++){
            var = mgt->variants[tmp];
            if (var->bandwidth == stuff[i]){
                var->priority = j--;    
                //av_log(NULL,AV_LOG_INFO,"===priority:%d,bandwidth:%d\n",var->priority,var->bandwidth);
                break;
            }
        }

    }

    float value = 0.0;
    int ret = -1;
    ret = am_getconfig_float("libplayer.hls.bw_max", &value);
    if(ret>=0&&value>0){
         for (i=0; i < mgt->n_variants; i++){
            var = mgt->variants[i];
            if(var->bandwidth>value){
                var->priority = -1;
            }
         }

    }
    ret = am_getconfig_float("libplayer.hls.bw_min", &value);
    if(ret>=0&&value>0){        
        for (i=0; i < mgt->n_variants; i++){
            var = mgt->variants[i];
            if(var->bandwidth<value){
                var->priority = -2;
            }
        }

    }else{
        for (i=0; i < mgt->n_variants; i++){
            var = mgt->variants[i];
            if(var->bandwidth<AUDIO_BANDWIDTH_MAX){
                var->priority = -2;
            }
        }           
    }


    ret = am_getconfig_float("libplayer.hls.bw_start", &value);
    if(ret>=0&&value>0){
        for (i=0; i < mgt->n_variants; i++){
            var = mgt->variants[i];
            if(var->priority==(int)value){
                mgt->playing_variant= var;                
                break;
            }
        }        

    }else{
        ret =(int)get_adaptation_ex_para(3);
        for (i=0; i < mgt->n_variants; i++){
            var = mgt->variants[i];
            if(ret>=0){
               
                if(var->priority==ret){
                    
                    mgt->playing_variant= var;
                    break;
                }               
            }else{
                if(var->priority>=0){
                   
                    mgt->playing_variant= var;
                    break;
                }
            }
        }         

    }
   
    
    av_log(NULL,AV_LOG_INFO,"Do fast sort streaming");
    return 0;
    
}

static int switch_bw_level(struct list_mgt * mgt, int updown){

    int priority =-1;
    if(updown>0){
        if(mgt->playing_variant->priority==mgt->n_variants-1||
            mgt->playing_variant->priority+updown>mgt->n_variants-1){
            av_log(NULL,AV_LOG_WARNING,"failed to switch bandwidth,up level:%d\n",updown);
        }else{            
            priority =  mgt->playing_variant->priority+updown;

        }
    }else if(updown<0){
        if(mgt->playing_variant->priority==0||
            mgt->playing_variant->priority-updown<0){
            av_log(NULL,AV_LOG_WARNING,"failed to switch bandwidth,down level:%d\n",updown);
        }else{            
           priority = mgt->playing_variant->priority-updown;

        }        
    }
    int i,playing_index;
    struct variant *v = NULL;
    if(priority<0){        
        for (i = 0; i < mgt->n_variants; i++) {
            v = mgt->variants[i];
            if (v->bandwidth == mgt->playing_variant->bandwidth) {
                playing_index = i;
                break;
            }
        }


    }else{
        for (i = 0; i < mgt->n_variants; i++) {
            v = mgt->variants[i];
            if (priority == mgt->playing_variant->priority) {
                playing_index = i;
                break;
            }
        }

    }
    return playing_index;
  

}
static int list_open_internet(ByteIOContext **pbio, struct list_mgt *mgt, const char *filename, int flags)
{
    list_demux_t *demux;
    int ret =0;
    ByteIOContext *bio = *pbio;
    char* url = filename;
reload:
    
    if (url_interrupt_cb()) {
         ret = -1;       
         goto error;   
    }
    
    if(bio==NULL){
        ret = avio_open_h(&bio, url, flags,mgt->ipad_ex_headers);
        av_log(NULL,AV_LOG_INFO,"http open,return value: %d\n",ret);
        
    }else{
        avio_reset(bio,AVIO_FLAG_READ); 
        URLContext* last = (URLContext*)bio->opaque;            
        ret = ff_http_do_new_request(last,NULL);
        av_log(NULL,AV_LOG_INFO,"do http request,return value: %d\n",ret);
    }
    
    if (ret != 0) {      
        if(bio!=NULL){
            avio_close(bio);
            bio = NULL;
        }
        return AVERROR(EIO);
    }
    mgt->location = bio->reallocation;
    if (NULL == mgt->location && mgt->n_variants > 0) { //set location for multibandwidth streaming,such youtube,etc.
        mgt->location = url;
    }
    demux = probe_demux(bio, url);
    if (!demux) {
        ret = -1;       
        goto error;
    }
    ret = demux->parser(mgt, bio);
    if (ret <0 && mgt->n_variants == 0) {
        ret = -1;
        
        goto error;
    } else {
       
        if (mgt->item_num == 0 && mgt->n_variants > 0) { //simplely choose server,mabye need sort variants when got it from server.
            if (bio) {
                url_fclose(bio);
                bio = NULL;
            }
            
            fast_sort_streams(mgt);            
            url = mgt->playing_variant->url;           
            av_log(NULL, AV_LOG_INFO, "[%d]reload playlist,url:%s\n", __LINE__, url);
            goto reload;           

        }

    }
   
    *pbio = bio;
    return 0;
error:
    if (bio) {       
        url_fclose(bio);
        *pbio  = NULL;
    }
    return ret;
}


static struct list_mgt* gListMgt = NULL;
static int list_open(URLContext *h, const char *filename, int flags)
{
    struct list_mgt *mgt;
    int ret;   
    mgt = av_malloc(sizeof(struct list_mgt));
    if (!mgt) {
        return AVERROR(ENOMEM);
    }
    memset(mgt, 0, sizeof(struct list_mgt));
    mgt->key_tmp = NULL;
    mgt->start_seq = -1;
    mgt->next_seq = -1;
    mgt->filename = filename + 5;
    mgt->flags = flags;
    mgt->next_index = 0;
    mgt->playing_item_index = 0;
    mgt->playing_item_seq = 0;
    mgt->strategy_up_counts = 0;
    mgt->strategy_down_counts = 0;
    mgt->listclose = 0;
    mgt->cmf_item_index = 0;
    mgt->cur_uio = NULL;
    mgt->codec_buf_level=-1;

    char headers[1024];
    char sess_id[40];
    memset(headers, 0, sizeof(headers));
    memset(sess_id, 0, sizeof(sess_id));
    generate_segment_session_id(sess_id, 37);
    snprintf(headers, sizeof(headers),
             "Connection: keep-alive\r\n"
             /*"Range: bytes=0- \r\n"*/
             "X-Playback-Session-Id: %s\r\n%s", sess_id,h!=NULL&&h->headers!=NULL?h->headers:"");
    av_log(NULL, AV_LOG_INFO, "Generate ipad http request headers,\r\n%s\n", headers);
    mgt->ipad_ex_headers = strndup(headers, 1024);
   
    memset(headers, 0, sizeof(headers));
    generate_playback_session_id(sess_id, 37);
    snprintf(headers, sizeof(headers),
             /*"Connection: keep-alive\r\n"*/
             "X-Playback-Session-Id: %s\r\n%s", sess_id,h!=NULL&&h->headers!=NULL?h->headers:"");
    av_log(NULL, AV_LOG_INFO, "Generate ipad http request media headers,\r\n%s\n", headers);
    
    mgt->ipad_req_media_headers = strndup(headers, 1024);

    gListMgt = mgt;
    if ((ret = list_open_internet(&mgt->cur_uio, mgt, mgt->filename, flags | URL_MINI_BUFFER | URL_NO_LP_BUFFER)) != 0) {
        av_free(mgt);
        return ret;
    }
    lp_lock_init(&mgt->mutex, NULL);
	
    float value = 0.0;
    ret = am_getconfig_float("libplayer.hls.stpos", &value);
    if (ret < 0 || value <= 0) {	
        if (!mgt->have_list_end) {
            int itemindex =0;
            if(mgt->item_num<10&&mgt->target_duration<5){
                itemindex = mgt->item_num / 2+1; /*for live streaming ,choose the middle item.*/

            }else if(mgt->item_num>=10){
                itemindex = mgt->item_num-3; //last item
            }else if(mgt->item_num<10&&mgt->target_duration>5){
                itemindex =  mgt->item_num -1;               
            }
            mgt->current_item = list_find_item_by_index(mgt,itemindex-1);
        } else {
            mgt->current_item = mgt->item_list;
        }		
    }else{
    	mgt->current_item = list_find_item_by_index(mgt,(int)value-1);
    }	
   
    h->is_streamed = 1;
    h->is_slowmedia = 1;
    if (mgt->full_time > 0 && mgt->have_list_end) {
        h->support_time_seek = 1;
    }
    h->priv_data = mgt;
    mgt->cache_http_handle = NULL;

    ret = CacheHttp_Open(&mgt->cache_http_handle,mgt->ipad_req_media_headers);
  

    return 0;
}

#define ADAPTATION_PROFILE_DEFAULT 1  //AGREESSIVE_ADAPTIVE
static int get_adaptation_profile(){
    float value = 0.0;
    int ret = -1;
    ret = am_getconfig_float("libplayer.hls.profile", &value);
    if(ret < 0 || value <0){
        ret = ADAPTATION_PROFILE_DEFAULT;
    }else{
        ret= (int)value;
    }
    av_log(NULL, AV_LOG_INFO,"just get adaptation profile: %d\n",ret);
    return ret;

}

static float get_adaptation_ex_para(int type){
    float value = 0.0;
    float ret = -1;
    if(type==0){
        ret = am_getconfig_float("libplayer.hls.sensitivity", &value);
    }else if(type == 1){
        ret = am_getconfig_float("libplayer.hls.upcounts", &value);
    }else if(type == 2){
        ret = am_getconfig_float("libplayer.hls.downcounts", &value);
    }else if(type == 3){
        ret = am_getconfig_float("libplayer.hls.fixed_bw", &value);
        if(ret<0){
            ret = 0;
        }
    }
    if(ret < 0 || value <0){
        ret = -1;
    }else{
        ret = value;
    }
    av_log(NULL, AV_LOG_INFO,"just get adaptation extend parameter(value:%d): %f\n",type,ret);
    return ret;

}



#define COMP_IGNORE_ZONE   1024*10  //10k
#define DEF_UP_COUNTS  1
#define DEF_DOWN_COUNTS 1

static int hls_common_bw_adaptive_check(struct list_mgt *c,int* measued_bw){
    int mean_bps, fast_bps, avg_bps,ret = -1;
    int measure_bw =0;
    float net_sensitivity=get_adaptation_ex_para(0);
     
    ret =CacheHttp_GetSpeed(c->cache_http_handle, &fast_bps, &mean_bps, &avg_bps);
    measure_bw = mean_bps;
    RLOG("hls current bandwidth=%dbps",measure_bw);
    if(net_sensitivity>0){
        measure_bw*=net_sensitivity;
    }
    
    if(measure_bw>c->playing_variant->bandwidth+COMP_IGNORE_ZONE){
        c->strategy_up_counts++;
        if(c->strategy_down_counts>0){
            c->strategy_down_counts=0;
        }
        int upcounts = DEF_UP_COUNTS;
        ret = (int)get_adaptation_ex_para(1);
        if(ret>0){
            upcounts = ret;
        }
        if(c->strategy_up_counts>=upcounts){//increase speed
            *measued_bw = measure_bw;
            return 1;
        }        
        
    }else if(measure_bw<c->playing_variant-COMP_IGNORE_ZONE){
        c->strategy_down_counts++;
        if(c->strategy_up_counts>0){
            c->strategy_up_counts=0;
        }
        int downcounts = DEF_DOWN_COUNTS;
        ret = (int)get_adaptation_ex_para(2);
        if(ret>0){
            downcounts = ret;
        }
        if(c->strategy_down_counts>=downcounts){//decrease speed
            *measued_bw  = measure_bw;
            return -1;
        }           
    }else{//keep original speed
        *measued_bw  = measure_bw;
        return 0;
    }

    return 0;

}

static  struct variant* hls_get_right_variant(struct list_mgt* c,int bw){
    int best_index = -1,best_band = -1;
    int min_index = 0, min_band = -1;
    int max_index = 0, max_band = -1;
    int i =0;
    for (i = 0; i < c->n_variants; i++) {
        struct variant *v = c->variants[i];
        if (v->bandwidth <= bw && v->bandwidth > best_band && v->priority>=0) {
            best_band = v->bandwidth;
            best_index = i;
        }
        if ((v->bandwidth < min_band || min_band < 0)&&v->priority>=0) {
            min_band = v->bandwidth;
            min_index = i;
        }
        if (v->bandwidth > max_band || max_band < 0) {
            max_band = v->bandwidth;
            max_index = i;
        }
    }
    if (best_index < 0) { /*no low rate streaming found,used the lowlest*/
        best_index = min_index;
    }
    av_log(NULL, AV_LOG_INFO, "select best bandwidth,index:%d,bandwidth:%d,priority:%d\n", \
        best_index, c->variants[best_index]->bandwidth, c->variants[best_index]->priority);
    return c->variants[best_index];
}
static int hls_aggressive_adaptive_bw_set(struct list_mgt* c,int bw){
    struct variant* var = hls_get_right_variant(c,bw);
    if(c->playing_variant->bandwidth != var->bandwidth){
        c->playing_variant = var;
        return 0;
    }
    return -1;
}

#define CODEC_BUFFER_LOW_FLAG  5
#define CODEC_BUFFER_HIGH_FLAG 10

static int hls_mean_adaptive_bw_set(struct list_mgt* c,int flag){
    int cache_buf_level = 0,codec_buf_level = 0;
    CacheHttp_GetBufferPercentage(c->cache_http_handle,&cache_buf_level);
    codec_buf_level = c->codec_buf_level/100;
    av_log(NULL,AV_LOG_INFO,"cache http buffer pecentage:%% %d,av buffer level:%%%d \%\n",cache_buf_level,codec_buf_level);
    int playing_index = -1;
    
    if(codec_buf_level>CODEC_BUFFER_LOW_FLAG&&codec_buf_level<=CODEC_BUFFER_HIGH_FLAG \
        ||codec_buf_level<=CODEC_BUFFER_LOW_FLAG&&c->playing_variant->priority==0){
        playing_index = switch_bw_level(c,0);
        
    }else if(flag<0&&codec_buf_level<CODEC_BUFFER_LOW_FLAG){  
        playing_index = switch_bw_level(c,-1);      
        

    }else if(flag>0&&codec_buf_level>CODEC_BUFFER_HIGH_FLAG){
        playing_index = switch_bw_level(c,1); 
    }
    if(c->playing_variant->bandwidth!= c->variants[playing_index]->bandwidth){
        c->playing_variant = c->variants[playing_index];
        return 0;

    }else{
        return -1;    
    }

}

static int hls_manual_adaptive_bw_set(struct list_mgt* c){
    int priority = (int)get_adaptation_ex_para(3);
    int i;
    int is_found = -1;
    for(i=0;i<c->n_variants;i++){
        struct variant* var = c->variants[i];
        if(priority!=c->playing_variant->priority&&var->priority ==priority){           
            c->playing_variant = var;
            is_found = 1;
            break;
        }
    }

    if(is_found>0){
        return 0;

    }else{
        return -1;
    }
  
}
static int select_best_variant(struct list_mgt *c)
{
    int cur_bw =0;
    int flag = -1;
    int change_flag = 0;
    int ret =-1;
    int adaptive_profile = get_adaptation_profile();
    flag = hls_common_bw_adaptive_check(c,&cur_bw);

    switch(adaptive_profile){
        case CONSTANT_ADAPTIVE:           
        case MANUAL_ADAPTIVE:
            ret = hls_manual_adaptive_bw_set(c);
            if(ret ==0){
                
                change_flag = 1;
            }
            break;
        case AGREESSIVE_ADAPTIVE:            
        case CONSERVATIVE_ADAPTIVE:
            if(flag!=0){
                ret = hls_aggressive_adaptive_bw_set(c,cur_bw);
                if(ret==0){
                    change_flag = 1;
                }
            }            
            break;
        case MEAN_ADAPTIVE:
            
            ret = hls_mean_adaptive_bw_set(c,flag);
            if(ret==0){
                change_flag =1;
            }
            break;
        default:
            break;
    }

    return change_flag;

}
static struct list_item * switchto_next_item(struct list_mgt *mgt) {
    struct list_item *next = NULL;
    struct list_item *current = NULL;
    if (!mgt) {
        return NULL;
    }

    if (mgt->n_variants > 0&&mgt->codec_buf_level>=0) { //vod,have mulit-bandwidth streams
        mgt->playing_item_index = mgt->current_item->index;
        mgt->playing_item_seq = mgt->current_item->seq;
        av_log(NULL, AV_LOG_INFO, "current playing item index: %d,current playing seq:%d\n", mgt->playing_item_index, mgt->playing_item_seq);
        int is_switch = select_best_variant(mgt);
        if (is_switch>0) { //will remove this tricks.

            if (mgt->item_num > 0) {
                list_delall_item(mgt);
            }

            mgt->current_item = mgt->current_item->next = NULL;
            mgt->start_seq = -1;
            mgt->next_seq = -1;
            mgt->next_index = 0;
            mgt->item_num = 0;
            mgt->full_time = 0;            

            if(mgt->cur_uio){
                url_fclose(mgt->cur_uio);
                mgt->cur_uio = NULL;
            }
        }

        av_log(NULL, AV_LOG_INFO, "select best variant,bandwidth: %d\n", mgt->playing_variant->bandwidth);

    }
    int64_t reload_interval = mgt->item_num > 0 && mgt->current_item != NULL ?
                              mgt->current_item->duration :
                              mgt->target_duration;
    reload_interval *= 1000000;
    int isNeedFetch = 1;

reload:
    if (mgt->current_item == NULL || mgt->current_item->next == NULL) {
        /*new refresh this mgtlist now*/
        ByteIOContext *bio = mgt->cur_uio;

        int ret;
        char* url = NULL;

        if (mgt->n_variants > 0 && NULL != mgt->playing_variant) {
            url = mgt->playing_variant->url;
            av_log(NULL, AV_LOG_INFO, "list open variant url:%s,bandwidth:%d\n", url, mgt->playing_variant->bandwidth);
        } else {
            url = mgt->filename;
            av_log(NULL, AV_LOG_INFO, "list open url:%s\n", url);
        }

        if (!mgt->have_list_end && (av_gettime() - mgt->last_load_time < reload_interval)) {
            av_log(NULL, AV_LOG_INFO, "drop fetch playlist from server\n");
            isNeedFetch = 0;
        }
        if (url_interrupt_cb()) {
            return NULL;
        }
        if (isNeedFetch == 0 || (ret = list_open_internet(&bio, mgt, url, mgt->flags | URL_MINI_BUFFER | URL_NO_LP_BUFFER)) != 0) {
            goto switchnext;
        }
      
        if (mgt->current_item && mgt->current_item->file) { /*current item,switch to next*/
            current = mgt->current_item;
            next = mgt->current_item->next;
            for (; next != NULL; next = next->next) {
                if (next->file && strcmp(current->file, next->file) == 0) {
                    /*found the same item,switch to the next*/
                    current = next;
                    break;
                }
            }
#if 0
            while (current != mgt->item_list) {
                /*del the old item,lest current,and then play current->next*/
                list_del_item(mgt, mgt->item_list);
            }
#endif

            mgt->current_item = current; /*switch to new current;*/
            if (!mgt->have_list_end && mgt->item_num > LIVE_LIST_MAX) {
                list_shrink_live_list(mgt);
            }
        }
    }
switchnext:
    if (mgt->current_item) {
        next = mgt->current_item->next;

    } else {
        if (mgt->n_variants < 1 || !mgt->have_list_end) {
            if (!mgt->have_list_end && mgt->playing_item_seq > 0) {
                next = list_find_item_by_seq(mgt, mgt->playing_item_seq);
                if (next == NULL) {
                    av_log(NULL, AV_LOG_WARNING, "can't find same seq item,just featch first node\n");
                    next = mgt->item_list;
                }
            } else {
                next = mgt->item_list;
            }
        } else {
            next = list_find_item_by_index(mgt, mgt->playing_item_index);
        }

    }
    if (mgt->listclose)
	return NULL;
    if (next)
        av_log(NULL, AV_LOG_INFO, "switch to new file=%s,total=%d,start=%.4lf,duration=%.4lf,index:%d,seq:%d\n",
               next->file, mgt->item_num, next->start_time, next->duration,next->index,next->seq);
    else {
        av_log(NULL, AV_LOG_INFO, "switch to new file=NULL,total=%d\n", mgt->item_num);
        if (!mgt->have_list_end) {


            if (url_interrupt_cb()) {
                return NULL;
            }
            usleep(100 * 1000);
            reload_interval = mgt->target_duration * 250000;
            isNeedFetch = 1;
            goto reload;
        }
    }
    return next;
}



static int list_read(URLContext *h, unsigned char *buf, int size)
{
    struct list_mgt *mgt = h->priv_data;
    int len = AVERROR(EIO);
    int counts = 200;
    do {
        if (url_interrupt_cb()) {
            av_log(NULL, AV_LOG_ERROR, " url_interrupt_cb\n");
            len = AVERROR(EINTR);
            break;
        }
        len = CacheHttp_Read(mgt->cache_http_handle, buf, size);
        /*
        if (len == AVERROR(EAGAIN)) {
            usleep(1000 * 10);
            break;
        }*/
        if (len <0) {
            usleep(1000 * 10);
        }
        if (len >= 0) {
            break;
        }
    } while (counts-- > 0);
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
    struct list_mgt *mgt = h->priv_data;
    struct list_item *item;
    
    int fulltime;

    if (whence == AVSEEK_BUFFERED_TIME) {
        int64_t buffed_time = 0;

        buffed_time = CacheHttp_GetBuffedTime(mgt->cache_http_handle);
        //av_log(NULL, AV_LOG_INFO, "list current buffed_time=%lld\n",buffed_time);
        if (buffed_time >= 0) {
            return buffed_time;

        } else {
            av_log(NULL, AV_LOG_DEBUG, "Can't get buffer time: %lld\n", buffed_time);
            return 0;
        }
    }

    if (whence == AVSEEK_SIZE) {
        av_log(NULL, AV_LOG_INFO, "----------->listseek AVSEEK_SIZE");
        return mgt->file_size;
    }
    av_log(NULL, AV_LOG_INFO, "list_seek pos=%lld,whence=%x\n", pos, whence);
    if (whence == AVSEEK_FULLTIME) {
        av_log(NULL, AV_LOG_INFO, "-----------> listseek AVSEEK_FULLTIME");
        if (mgt->have_list_end) {
            av_log(NULL, AV_LOG_INFO, "return mgt->full_timet=%d\n", mgt->full_time);
            return (int64_t)mgt->full_time;
        } else {
            return -1;
        }
    }

    if (whence == AVSEEK_TO_TIME) {
        av_log(NULL, AV_LOG_INFO, "list_seek to Time =%lld,whence=%x\n", pos, whence);

        if (pos >= 0 && pos < mgt->full_time) {
            for (item = mgt->item_list; item; item = item->next) {

                if (item->start_time <= pos && pos < item->start_time + item->duration) {
                    mgt->current_item = NULL;
                    CacheHttp_Reset(mgt->cache_http_handle);
                    mgt->current_item = item;
                    mgt->playing_item_index = item->index - 1;
                    if (!mgt->have_list_end) {
                        mgt->playing_item_seq = item->seq - 1;
                    }
                    av_log(NULL, AV_LOG_INFO, "list_seek to item->file =%s\n", item->file);
                    //CacheHttp_Reset(mgt->cache_http_handle);
                    return (int64_t)(item->start_time);/*pos=0;*/
                }
            }


        }


    }

#if 1
    if(whence == SEEK_SET) {
        av_log(NULL, AV_LOG_INFO, "-----------> listseek SEEK_SET");
        for (item = mgt->item_list; item; item = item->next) {
            if(mgt->cmf_item_index == item->index) {
                    /*mgt->current_item = NULL;
                    CacheHttp_Reset(mgt->cache_http_handle);
                    mgt->current_item = item;
                    mgt->playing_item_index = item->index - 1;
                    if (!mgt->have_list_end) {
                        mgt->playing_item_seq = item->seq - 1;
                    }*/
                    return 0;
            }
        }
    }

    if (whence == AVSEEK_SLICE_BYINDEX) {
        av_log(NULL, AV_LOG_INFO, "-----------> listseek AVSEEK_SLICE_BYINDEX");
        if(pos >= 0 && pos < mgt->item_num) {
            for (item = mgt->item_list; item; item = item->next) {
                if(pos == item->index) {
                    mgt->cmf_item_index = pos;                  
                    mgt->current_item = NULL;
                    CacheHttp_Reset(mgt->cache_http_handle);
                    mgt->current_item = item;
                    mgt->playing_item_index = item->index - 1;
                    if (!mgt->have_list_end) {
                        mgt->playing_item_seq = item->seq - 1;
                    }                   
                    while(item->item_size <= 0) {
                        usleep(200*1000);
                    }
                    return pos;
                }
            }
        }
    }

    if (whence == AVSEEK_SLICE_BYTIME) {
        av_log(NULL, AV_LOG_INFO, "-----------> listseek AVSEEK_SLICE_BYTIME");
         if (pos >= 0 && pos < mgt->full_time * 1000) {
                for (item = mgt->item_list; item; item = item->next) {
                    if (item->start_time * 1000 <= pos && pos < (item->start_time + item->duration) * 1000) {
                        mgt->cmf_item_index = item->index;
                        /*
                        mgt->current_item = NULL;
                        CacheHttp_Reset(mgt->cache_http_handle);
                        mgt->current_item = item;
                        mgt->playing_item_index = item->index - 1;
                        if (!mgt->have_list_end) {
                            mgt->playing_item_seq = item->seq - 1;
                        }
                        */
                        return item->index;
                    }
            }
         }
    }
#endif
    
    av_log(NULL, AV_LOG_INFO, "list_seek failed\n");
    return -1;
}
static int list_close(URLContext *h)
{
    struct list_mgt *mgt = h->priv_data;

    if (!mgt) {
        return 0;
    }
    mgt->listclose = 1;
    CacheHttp_Close(mgt->cache_http_handle);
    list_delall_item(mgt);
    int i;
    for (i = 0; i < mgt->n_variants; i++) {
        struct variant *var = mgt->variants[i];
        av_free(var);
    }
    av_freep(&mgt->variants);
    mgt->n_variants = 0;
    if (NULL != mgt->prefix) {
        av_free(mgt->prefix);
        mgt->prefix = NULL;
    }

    if(NULL!=mgt->ipad_ex_headers){
        av_free(mgt->ipad_ex_headers);
    }
    if(NULL!= mgt->ipad_req_media_headers){
        av_free(mgt->ipad_req_media_headers);
    }

    if (mgt->cur_uio != NULL) {
        url_fclose(mgt->cur_uio);
        mgt->cur_uio = NULL;
    }
    av_free(mgt);
    unused(h);
    gListMgt = NULL;
    return 0;
}
static int list_get_handle(URLContext *h)
{
    return (intptr_t) h->priv_data;
}
static int list_setcmd(URLContext *h, int cmd,int flag,unsigned long info)
{
    struct list_mgt *mgt = h->priv_data;
    int ret=-1;
    if(AVCMD_SET_CODEC_DATA_LEVEL==cmd){
        mgt->codec_buf_level=info;
	 //av_log(NULL, AV_LOG_INFO, "list_setcmd codec buf level=%d\n",(int)info);
        ret=0;
    }
    return ret;
}

static int list_getinfo(URLContext *h, uint32_t  cmd, uint32_t flag, int64_t *info)
{
    av_log(NULL, AV_LOG_INFO, "list_getinfo enter\n");
    struct list_mgt *mgt = h->priv_data;

    if (!mgt) {
        return 0;
    }

    if(!mgt->have_list_end)
        return 0;
    
    if (cmd == AVCMD_TOTAL_DURATION) {
        *info = mgt->full_time * 1000;
        av_log(NULL, AV_LOG_INFO, " ----------> list_getinfo, AVCMD_TOTAL_DURATION=%lld", *info);
        return 0;
    } else if (cmd == AVCMD_TOTAL_NUM) {
        *info = mgt->item_num - 1;
        av_log(NULL, AV_LOG_INFO, " ----------> list_getinfo, AVCMD_TOTAL_NUM=%lld", *info);
        return 0;
    } else if (cmd == AVCMD_SLICE_START_OFFSET) {
        *info = -1;
        av_log(NULL, AV_LOG_INFO, " ----------> list_getinfo, AVCMD_SLICE_START_OFFSET");
        return 0;
    } else if (cmd == AVCMD_SLICE_SIZE) {
        *info = -1;
        struct list_item *item;
        av_log(NULL,AV_LOG_INFO,"----------> list_getinfo, cmf_item_index=%d", mgt->cmf_item_index);
        for (item = mgt->item_list; item; item = item->next) {
                if(mgt->cmf_item_index == item->index) {
                    *info = item->item_size;
                    av_log(NULL, AV_LOG_INFO, " ----------> list_getinfo, AVCMD_SLICE_SIZE=%lld", *info);
                    return 0;
                }
        }
    } else if (cmd == AVCMD_SLICE_INDEX) {
        *info = mgt->cmf_item_index;
        av_log(NULL, AV_LOG_INFO, " ----------> list_getinfo, AVCMD_SLICE_INDEX=%d", *info);
        return 0;
    } else {
        struct list_item *item;
         for (item = mgt->item_list; item; item = item->next) {
                if(mgt->cmf_item_index == item->index) {
                    if(cmd == AVCMD_SLICE_STARTTIME) {
                        *info = (int64_t)item->start_time*1000;
                        av_log(NULL, AV_LOG_INFO, " ----------> list_getinfo, AVCMD_SLICE_STARTTIME=%lld", *info);
                    }
                    if(cmd == AVCMD_SLICE_ENDTIME) {
                        *info = (int64_t)(item->start_time + item->duration)*1000;
                        av_log(NULL, AV_LOG_INFO, " ----------> list_getinfo, AVCMD_SLICE_ENDTIME=%lld", *info);
                    }
                    return 0;
                }
         }
    }

    return 0;
    
}

URLProtocol file_list_protocol = {
    .name = "list",
    .url_open = list_open,
    .url_read  = list_read,
    .url_write = list_write,
    .url_seek   = list_seek,
    .url_close = list_close,
    .url_exseek = list_seek, /*same as seek is ok*/
    .url_get_file_handle = list_get_handle,
    .url_setcmd = list_setcmd,
    .url_getinfo = list_getinfo,
};
list_item_t* getCurrentSegment(void* hSession)
{
    struct list_item *item = gListMgt->current_item;
    if(item)
        item->have_list_end = gListMgt->have_list_end;
    return item;

}
const char* getCurrentSegmentUrl(void* hSession)
{
    if (gListMgt == NULL) {
        return NULL;
    }

    const char* url = getCurrentSegment(gListMgt)->file;

    return url;
}

long long getTotalDuration(void* hSession)
{
    if (gListMgt == NULL) {
        return NULL;
    }
    return gListMgt->full_time;
}

int switchNextSegment(void* hSession)
{
    gListMgt->current_item = switchto_next_item(gListMgt);
    return 0;
}
URLProtocol *get_file_list_protocol(void)
{
    return &file_list_protocol;
}
int register_list_demux_all(void)
{
    static int registered_all = 0;
    if (registered_all) {
        return;
    }
    registered_all++;
    extern struct list_demux m3u_demux;
    av_register_protocol(&file_list_protocol);
    register_list_demux(&m3u_demux);
    return 0;
}

