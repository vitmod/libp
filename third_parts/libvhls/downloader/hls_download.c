
#define LOG_NDEBUG 0
#define LOG_TAG "Downloader"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hls_download.h"
#include "hls_utils.h"
#include "hls_bandwidth_measure.h"

#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif

#define _USE_FFMPEG_CODE 1

#ifdef _USE_FFMPEG_CODE 
#include "libavformat/avio.h"
#include "libavutil/opt.h"
#endif

#define SAVE_BACKUP 1

#define HTTP_MEASURE_ITEM_NUM 100
typedef struct _HLSHttpContext{
    void* h;    
    int open_flag; 
    int error_code;     
    char* redirect_url;
    void* meausure_handle;
#ifdef SAVE_BACKUP    
#define BACK_FILE_PATH "/cached/"
    FILE* mBackupFile;
#endif
}HLSHttpContext;
int hls_http_open(const char* url,const char* headers,void* key,void** handle){

    if(*handle !=NULL){
        LOGE("Need close opend handle\n");
        return -1;
    }
    HLSHttpContext* ctx = NULL;
    ctx = (HLSHttpContext*)malloc(sizeof(HLSHttpContext));
    int ret = -1;
    int reason_code = 0;
    char fileUrl[MAX_URL_SIZE];
    int is_ignore_range_req = 0;
    if(in_get_sys_prop_float("libplayer.hls.ignore_range")>0){
        is_ignore_range_req = 1;        
    }
    ctx->mBackupFile = NULL;
    //edit url address
#ifdef _USE_FFMPEG_CODE       
    memset(fileUrl,0,sizeof(MAX_URL_SIZE));
    URLContext* h = NULL;  
    int flag = 0;
    if(key == NULL){    
        snprintf(fileUrl,MAX_URL_SIZE,"s%s",url);    

        if(is_ignore_range_req>0){
            flag |=URL_SEGMENT_MEDIA;
        }
        if(headers!=NULL&&strlen(headers)>0){
            ret = ffurl_open_h(&h, fileUrl,AVIO_FLAG_READ|AVIO_FLAG_NONBLOCK|flag,headers,&reason_code);
        }else{
            ret = ffurl_open_h(&h, fileUrl,AVIO_FLAG_READ|AVIO_FLAG_NONBLOCK|flag,NULL,&reason_code);
        }
        if(ret==0 &&h->location!=NULL&&strlen(h->location)>0){
            ctx->redirect_url = strndup(h->location,MAX_URL_SIZE);
        }else{
            ctx->redirect_url = NULL;
        }
    }else{
        AESKeyInfo_t* aeskey = (AESKeyInfo_t*)key;
        if(aeskey->type != AES128_CBC|| aeskey->key_info == NULL){
            LOGE("Only support AES128-cbc\n");
            ctx->h = NULL;
            *handle = ctx;
            return -1;
        }
        if (strstr(url,"://")){
            snprintf(fileUrl, MAX_URL_SIZE, "crypto+%s", url);

        }else{
            snprintf(fileUrl, MAX_URL_SIZE, "crypto:%s", url);
        }
        AES128KeyInfo_t* aes128key = (AES128KeyInfo_t*)aeskey->key_info;
        ret = ffurl_alloc(&h, fileUrl, AVIO_FLAG_READ|AVIO_FLAG_NONBLOCK);
        if(ret>=0){
            if(is_ignore_range_req>0){
                h->is_segment_media = 1;
            }
            av_set_string3(h->priv_data, "key", aes128key->key_hex, 0, NULL);
            av_set_string3(h->priv_data, "iv", aes128key->ivec_hex, 0, NULL);               
            if((ret = ffurl_connect(h))<0){
                if(404 == h->http_code){
                    reason_code = 1;
                }
                if(503 == h->http_code){
                    reason_code = 2;

                }
                if(500 == h->http_code){
                    reason_code = 3;
                }
                
                ffurl_close(h);           
                h = NULL;                
            }
            if(h != NULL&&h->location!=NULL&&strlen(h->location)>0){
                ctx->redirect_url = strndup(h->location,MAX_URL_SIZE);
            }else{
                ctx->redirect_url = NULL;
            }
            
        }
    }
    ctx->h = h;
    
#endif
    if(ret!=0){
        ctx->error_code = reason_code;
        ctx->open_flag = -1;
        LOGE("Failed to open http file,url:%s,error:%d,reason:%d\n",fileUrl,ret,reason_code);
        return -1; 

    }
    
    ctx->error_code = 0;
    ctx->open_flag = 1;
    ctx->meausure_handle = bandwidth_measure_alloc(HTTP_MEASURE_ITEM_NUM,0);
    if(ctx->meausure_handle == NULL){
        LOGE("Failed to allocate memory for bandwidth meausre utils\n");
        ctx->open_flag = -1;
        return -1; 
    }
#ifdef SAVE_BACKUP
    int dump_type = in_get_sys_prop_float("libplayer.hls.dump");
    if(dump_type>0){
        ctx->mBackupFile = NULL;
        char backup[MAX_URL_SIZE];
        char* fstart = strrchr(url, '/');
        strcpy(backup,BACK_FILE_PATH);
        char* stime = NULL;
        int stlen = 0;

        if(strcasestr(&url[strlen(url)-5],".ts")!=NULL||strcasestr(&url[strlen(url)-5],".f4v")!=NULL||strcasestr(&url[strlen(url)-5],".mp4")!=NULL){            
            snprintf(backup+strlen(BACK_FILE_PATH),MAX_URL_SIZE-strlen(BACK_FILE_PATH),"%s",fstart);
        }else{
            getLocalCurrentTime(&stime,&stlen);        
            snprintf(backup+strlen(BACK_FILE_PATH),MAX_URL_SIZE-strlen(BACK_FILE_PATH),"%s.bak.%s",fstart,stime);
            //snprintf(backup+strlen(BACK_FILE_PATH),MAX_URL_SIZE-strlen(BACK_FILE_PATH),"%s.m3u.bak",stime);
        }
        backup[strlen(backup)+1] = '\0';    
        ctx->mBackupFile = fopen(backup, "wb");
        if(ctx->mBackupFile == NULL){
            LOGE("Failed to create backup file");
        }
    }
#endif 
    *handle = ctx;
    return 0;
}
int64_t hls_http_get_fsize(void* handle){
    if(handle == NULL){
        return -1;
    }
    int64_t fsize = 0;
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return -1;
    }    
#ifdef _USE_FFMPEG_CODE     
    URLContext* h = (URLContext*)(ctx->h);    
    fsize = ffurl_seek(h,0, AVSEEK_SIZE);
#endif
    return fsize;
}

int hls_http_read(void* handle,void* buf,int size){
    if(handle == NULL){
        return -1;
    }
    
    int rsize = 0;
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return -1;
    }
    bandwidth_measure_start_read(ctx->meausure_handle);    
#ifdef _USE_FFMPEG_CODE 
    URLContext* h = (URLContext*)(ctx->h); 
    rsize = ffurl_read(h,(unsigned char*)buf, size);
    if(rsize == AVERROR_EOF){
        rsize = 0;
    }
#endif
    if(rsize>0){
        bandwidth_measure_finish_read(ctx->meausure_handle,rsize);
    }else{
        bandwidth_measure_finish_read(ctx->meausure_handle,0);
    }
#ifdef SAVE_BACKUP
    int wsize = 0;
    if(ctx->mBackupFile!=NULL&&rsize>0){
        wsize = fwrite(buf, 1,rsize, ctx->mBackupFile);
        fflush(ctx->mBackupFile);
    }
#endif
    return rsize;
    
}
int hls_http_seek_by_size(void* handle,int64_t pos,int flag){
    if(handle == NULL){
        return -1;
    }
    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return -1;
    }  

    int ret = -1;
#ifdef _USE_FFMPEG_CODE 
    URLContext* h = (URLContext*)(ctx->h); 
    ret = ffurl_seek(h, pos, flag);
#endif
    return ret;

}
//TBD
int hls_http_seek_by_time(void* handle,int64_t timeUs){
    return 0;
}

int hls_http_estimate_bandwidth(void* handle,int* bandwidth_bps){
    if(handle == NULL){
        return -1;
    }    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    
    int avg_bps,fast_bps,mid_bps;
    int ret = bandwidth_measure_get_bandwidth(ctx->meausure_handle,&fast_bps,&mid_bps,&avg_bps);
    
    *bandwidth_bps = avg_bps;
    return ret;

}
const char* hls_http_get_redirect_url(void* handle){
    if(handle == NULL){
        return NULL;
    }
    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return NULL;
    }    

    return ctx->redirect_url;

}

int hls_http_get_error_code(void* handle){
    if(handle == NULL){
        return -1;
    }
    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return -1;
    } 

    LOGV("Got http error code:%d\n",ctx->error_code);
    return ctx->error_code;
    

}
int hls_http_close(void* handle){
    if(handle == NULL){
        return -1;
    }
    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return -1;
    }
#ifdef _USE_FFMPEG_CODE 
    URLContext* h = (URLContext*)(ctx->h); 
    ffurl_close(h);
#endif

#ifdef SAVE_BACKUP    
    if(ctx->mBackupFile!=NULL){
        fclose(ctx->mBackupFile);
        ctx->mBackupFile = NULL;
    }
#endif
    if(ctx->redirect_url){
        free(ctx->redirect_url);
    }

    bandwidth_measure_free(ctx->meausure_handle);
    free(ctx);
    ctx = NULL;
    LOGV("Close http session\n");
    return 0;

}




//#define _DEBUG_NO_LIBPLAYER 1

int fetchHttpSmallFile(const char* url,const char* headers,void** buf,int* length,char** redirectUrl){
    if(url==NULL){
        return -1;
    }

    void* handle = NULL;
    int ret = -1;
    
#ifdef _DEBUG_NO_LIBPLAYER    
    av_register_all();
#endif
 
    ret = hls_http_open(url,headers,NULL,&handle);
   
    if(ret!=0){
        LOGV("Failed to open http handle\n");
        return -1;
    }
    int64_t flen = hls_http_get_fsize(handle);
    int64_t rsize = 0;
    unsigned char* buffer = NULL;
    const int def_buf_size = 64*1024;
    int buf_len = 0;
    if(flen>0){
        buffer = (unsigned char* )malloc(flen);
        buf_len = flen;         
       
    }else{
        buffer = (unsigned char* )malloc(def_buf_size);
        buf_len = def_buf_size;  
        
    }
    memset(buffer,0,buf_len);
    int isize = 0;

    do{
        ret = hls_http_read(handle,buffer+isize,buf_len);
        if(ret<=0){
            if (ret!= HLSERROR(EAGAIN)) {
                if(ret!=0){
                    LOGE("Read data failed, errno %d\n", errno);
                }
                break;
            }
            else {
                continue;
            }
        }else{
            isize+=ret;
        }
    }while(isize<buf_len);

    if(hls_http_get_redirect_url(handle)!=NULL){
        *redirectUrl = strdup(hls_http_get_redirect_url(handle));
    }else{
        *redirectUrl = NULL;
    }
    *buf = buffer;
    *length = isize;

    hls_http_close(handle);

    return 0;
    
}
