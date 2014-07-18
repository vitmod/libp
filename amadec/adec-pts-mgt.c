/**
 * \file adec-pts-mgt.c
 * \brief  Functions Of Pts Manage.
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <adec-pts-mgt.h>
#include <cutils/properties.h>
#include <sys/time.h>
#include <amthreadpool.h>

#define DROP_PCM_DURATION_THRESHHOLD 4 //unit:s
#define DROP_PCM_MAX_TIME 1000 // unit :ms

int adec_pts_droppcm(aml_audio_dec_t *audec);
int vdec_pts_resume(void);
static int vdec_pts_pause(void);
int droppcm_use_size(aml_audio_dec_t *audec, int drop_size);
void droppcm_prop_ctrl(int *audio_ahead, int *pts_ahead_val);
int droppcm_get_refpts(aml_audio_dec_t *audec, unsigned long *refpts);

static int64_t gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int sysfs_get_int(char *path, unsigned long *val)
{
    char buf[64];

    if (amsysfs_get_sysfs_str(path, buf, sizeof(buf)) == -1) {
        adec_print("unable to open file %s,err: %s", path, strerror(errno));
        return -1;
    }
    if (sscanf(buf, "0x%lx", val) < 1) {
        adec_print("unable to get pts from: %s", buf);
        return -1;
    }

    return 0;
}

static int set_tsync_enable(int enable)
{
    char *path = "/sys/class/tsync/enable";
    return amsysfs_set_sysfs_int(path, enable);
}
/**
 * \brief calc current pts
 * \param audec pointer to audec
 * \return aurrent audio pts
 */
unsigned long adec_calc_pts(aml_audio_dec_t *audec)
{
    unsigned long pts, delay_pts;
    audio_out_operations_t *out_ops;
    dsp_operations_t *dsp_ops;

    out_ops = &audec->aout_ops;
    dsp_ops = &audec->adsp_ops;

    pts = dsp_ops->get_cur_pts(dsp_ops);
    if (pts == -1) {
        adec_print("get get_cur_pts failed\n");
        return -1;
    }
    dsp_ops->kernel_audio_pts = pts;

    if (out_ops == NULL || out_ops->latency == NULL) {
        adec_print("cur_out is NULL!\n ");
        return -1;
    }

    if(!audec->apts_start_flag)
        return pts;
    delay_pts = out_ops->latency(audec) * 90;

    if (delay_pts+audec->first_apts < pts) {
        pts -= delay_pts;
    } else {
        ///pts =audec->first_apts;
    }
    return pts;
}

/**
 * \brief start pts manager
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 */
int adec_pts_start(aml_audio_dec_t *audec)
{
    unsigned long pts = 0;
    char *file;
    char buf[64];
    dsp_operations_t *dsp_ops;
	char value[PROPERTY_VALUE_MAX]={0};

    adec_print("adec_pts_start");
    dsp_ops = &audec->adsp_ops;
    memset(buf, 0, sizeof(buf));

    if (audec->avsync_threshold <= 0) {
        if (am_getconfig_bool("media.libplayer.wfd")) {
            audec->avsync_threshold = SYSTIME_CORRECTION_THRESHOLD * 2 / 3;
            adec_print("use 2/3 default av sync threshold!\n");
        } else {
            audec->avsync_threshold = SYSTIME_CORRECTION_THRESHOLD;
            adec_print("use default av sync threshold!\n");
        }
    }
    adec_print("av sync threshold is %d , no_first_apts=%d,\n", audec->avsync_threshold, audec->no_first_apts);

    dsp_ops->last_pts_valid = 0;
    if(property_get("sys.amplayer.drop_pcm",value,NULL) > 0)
    	if(!strcmp(value,"1"))
    	     adec_pts_droppcm(audec);
    // before audio start or pts start
    if(amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PRE_START") == -1)
    {
        return -1;
    }

    amthreadpool_thread_usleep(1000);

	if (audec->no_first_apts) {
		if (amsysfs_get_sysfs_str(TSYNC_APTS, buf, sizeof(buf)) == -1) {
			adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
			return -1;
		}

		if (sscanf(buf, "0x%lx", &pts) < 1) {
			adec_print("unable to get vpts from: %s", buf);
			return -1;
		}

	} else {
	    pts = adec_calc_pts(audec);
		
	    if (pts == -1) {

	        adec_print("pts==-1");

    		if (amsysfs_get_sysfs_str(TSYNC_APTS, buf, sizeof(buf)) == -1) {
    			adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
    			return -1;
    		}

	        if (sscanf(buf, "0x%lx", &pts) < 1) {
	            adec_print("unable to get apts from: %s", buf);
	            return -1;
	        }
	    }
	}

    adec_print("audio pts start from 0x%lx", pts);

    sprintf(buf, "AUDIO_START:0x%lx", pts);

    if(amsysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1)
    {
        return -1;
    }
    audec->first_apts=pts;
    audec->apts_start_flag=1;
    return 0;
}

static void enable_slowsync_repeate()
{
    const char * slowsync_path = "/sys/class/tsync/slowsync_enable";
    const char * slowsync_repeate_path = "/sys/class/video/slowsync_repeat_enable";
    amsysfs_set_sysfs_int(slowsync_path, 1);
    amsysfs_set_sysfs_int(slowsync_repeate_path, 1);
    adec_print("enable slowsync repeate. \n");
}

int adec_pts_droppcm(aml_audio_dec_t *audec)
{
    unsigned long refpts, apts, oldapts;
    int ret, drop_size, droppts, dropms;
    char value[PROPERTY_VALUE_MAX]={0};
    int64_t starttime, endtime;
    int audio_ahead = 0;
    unsigned pts_ahead_val = SYSTIME_CORRECTION_THRESHOLD;
	int diff;
    int sync_switch_ms = 200; // ms
    starttime = gettime();

    // drop pcm according to media.amplayer.dropms
    memset(value,0,sizeof(value));
    if(property_get("media.amplayer.dropms",value,NULL) > 0){
        dropms = atoi(value);
        drop_size = dropms * (audec->samplerate/1000) * audec->channels *2;
        if (drop_size > 0) {
            if (droppcm_use_size(audec, drop_size) == -1) {
                adec_print("[%s::%d] data not enough, drop failed! \n",__FUNCTION__,__LINE__);
            }

            if (am_getconfig_bool("media.amplayer.dropmsquit")) {
               adec_print("[%s::%d] fast droppcm: %d ms!  \n",__FUNCTION__,__LINE__, dropms);
               return 0;
            }
        }
    }

    if (droppcm_get_refpts(audec, &refpts) == -1) {
        adec_print("[%s::%d] get refpts failed! \n",__FUNCTION__,__LINE__);
        return -1;
    }

    apts = adec_calc_pts(audec);
    diff = (apts > refpts)?(apts-refpts):(refpts-apts);
    adec_print("before drop --apts 0x%x,refpts 0x%x,apts %s, diff 0x%x\n",apts,refpts,(apts>refpts)?"big":"small",diff);

    if(property_get("media.amplayer.sync_switch_ms",value,NULL) > 0){
        sync_switch_ms = atoi(value);
    }
    adec_print("sync switch setting: %d ms \n",sync_switch_ms);
    //auto switch -- if diff not too much, using slow sync
    if(apts>=refpts || diff/90 < sync_switch_ms)
    {
        enable_slowsync_repeate();
        adec_print("diff less than %d (ms), use slowsync repeate mode \n",sync_switch_ms);
        return 0;
    }

    // calculate ahead pcm size
    droppcm_prop_ctrl(&audio_ahead, &pts_ahead_val);

    // drop pcm
    droppts = refpts - apts+pts_ahead_val*audio_ahead;
    drop_size = (droppts/90) * (audec->samplerate/1000) * audec->channels *2;
    adec_print("[%s::%d] droppts:0x%x, drop_size=%d,  audio ahead %d,ahead pts value %d \n",	__FUNCTION__,__LINE__, 
        droppts, drop_size, audio_ahead,pts_ahead_val);
    if (droppcm_use_size(audec, drop_size) == -1) {
        adec_print("[%s::%d] timeout! data not enough! \n",__FUNCTION__,__LINE__);
    }
  
    // show apts refpts after droppcm
    if (sysfs_get_int(TSYNC_VPTS, &refpts) == -1) {
        adec_print("## [%s::%d] unable to get vpts! \n",__FUNCTION__,__LINE__);
        return -1;
    }

    oldapts = apts;
    apts = adec_calc_pts(audec);
    diff = (apts > refpts)?(apts-refpts):(refpts-apts);
    adec_print("after drop:--apts 0x%x,droped:0x%x, vpts 0x%x,apts %s, diff 0x%x\n",apts, apts-oldapts, refpts,(apts>refpts)?"big":"small",diff);

    endtime = gettime();
    adec_print("==starttime is :%lld ms  endtime is:%lld ms   usetime:%lld ms  \n",starttime/1000 ,endtime/1000 ,(endtime-starttime)/1000);

    return 0;
}

/**
 * \brief pause pts manager
 * \return 0 on success otherwise -1
 */
int adec_pts_pause(void)
{
    adec_print("adec_pts_pause");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PAUSE");
}

/**
 * \brief resume pts manager
 * \return 0 on success otherwise -1
 */
int adec_pts_resume(void)
{
    adec_print("adec_pts_resume");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_RESUME");

}

/**
 * \brief refresh current audio pts
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 */
 static int apts_interrupt=0;
int adec_refresh_pts(aml_audio_dec_t *audec)
{
    unsigned long pts;
    unsigned long systime;
    unsigned long last_pts = audec->adsp_ops.last_audio_pts;
    unsigned long last_kernel_pts = audec->adsp_ops.kernel_audio_pts;
    char buf[64];
    char ret_val = -1;
    if (audec->auto_mute == 1) {
        return 0;
    }

    memset(buf, 0, sizeof(buf));

    systime = audec->adsp_ops.get_cur_pcrscr(&audec->adsp_ops);
    if (systime == -1) {
        adec_print("unable to getsystime");
        return -1;
    }

    /* get audio time stamp */
    pts = adec_calc_pts(audec);
    if (pts == -1 || last_pts == pts) {
        //close(fd);
        //if (pts == -1) {
        return -1;
        //}
    }

    if ((abs(pts - last_pts) > APTS_DISCONTINUE_THRESHOLD) && (audec->adsp_ops.last_pts_valid)) {
        /* report audio time interruption */
        adec_print("pts = %lx, last pts = %lx\n", pts, last_pts);

        adec_print("audio time interrupt: 0x%lx->0x%lx, 0x%lx\n", last_pts, pts, abs(pts - last_pts));

        sprintf(buf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx", pts);

        if(amsysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1)
        {
            adec_print("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
            return -1;
        }
        
        audec->adsp_ops.last_audio_pts = pts;
        audec->adsp_ops.last_pts_valid = 1;
        adec_print("set automute!\n");
        audec->auto_mute = 0;
        apts_interrupt=10;
        return 0;
    }

    if (last_kernel_pts == audec->adsp_ops.kernel_audio_pts) {
        return 0;
    }

    audec->adsp_ops.last_audio_pts = pts;
    audec->adsp_ops.last_pts_valid = 1;

    if (abs(pts - systime) < audec->avsync_threshold) {
        apts_interrupt=0;
        return 0;
    }
    else if(apts_interrupt>0){
        apts_interrupt --;
        return 0;
        }

    /* report apts-system time difference */

    if(audec->adsp_ops.set_cur_apts){
        ret_val = audec->adsp_ops.set_cur_apts(&audec->adsp_ops,pts);
    }
    else{
	 sprintf(buf, "0x%lx", pts);
	 ret_val = amsysfs_set_sysfs_str(TSYNC_APTS, buf);
    }			
    if(ret_val == -1)
    {
        adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
        return -1;
    }
    //adec_print("report apts as %ld,system pts=%ld, difference= %ld\n", pts, systime, (pts - systime));
    return 0;
}

/**
 * \brief Disable or Enable av sync
 * \param e 1 =  enable, 0 = disable
 * \return 0 on success otherwise -1
 */
int avsync_en(int e)
{
    return amsysfs_set_sysfs_int(TSYNC_ENABLE, e);
}

/**
 * \brief calc pts when switch audio track
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 *
 * When audio track switch occurred, use this function to judge audio should
 * be played or not. If system time fall behind audio pts , and their difference
 * is greater than SYSTIME_CORRECTION_THRESHOLD, auido should wait for
 * video. Otherwise audio can be played.
 */
int track_switch_pts(aml_audio_dec_t *audec)
{
    unsigned long vpts;
    unsigned long apts;
    unsigned long pcr;
    char buf[32];

    memset(buf, 0, sizeof(buf));

    pcr = audec->adsp_ops.get_cur_pcrscr(&audec->adsp_ops);
    if (pcr == -1) {
        adec_print("unable to get pcr");
        return 1;
    }

    apts = adec_calc_pts(audec);
    if (apts == -1) {
        adec_print("unable to get apts");
        return 1;
    }
    
    if((apts > pcr) && (apts - pcr > 0x100000))
        return 0;
		
    if (abs(apts - pcr) < audec->avsync_threshold || (apts <= pcr)) {
        return 0;
    } else {
        return 1;
    }

}
static int vdec_pts_pause(void)
{
    char *path = "/sys/class/video_pause";
    return amsysfs_set_sysfs_int(path, 1);

}
int vdec_pts_resume(void)
{
    adec_print("vdec_pts_resume\n");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "VIDEO_PAUSE:0x0");
}

int droppcm_use_size(aml_audio_dec_t *audec, int drop_size)
{
    char value[PROPERTY_VALUE_MAX]={0};
    char buffer[8*1024];
    int drop_duration=drop_size/audec->channels/2/audec->samplerate;
    int nDropCount = 0;
    int size = drop_size;
    int ret = 0;
    int drop_max_time = DROP_PCM_MAX_TIME; // unit:ms
    int64_t start_time, nDropConsumeTime;
    char platformtype[20];

    memset(platformtype,0,sizeof(platformtype));
    memset(value,0,sizeof(value));
    if(property_get("media.amplayer.dropmaxtime",value,NULL) > 0){
        drop_max_time = atoi(value);
    }
    memset(value,0,sizeof(value));
    if (property_get("ro.board.platform",value,NULL) > 0) {
        if (match_types("meson6",value))
            memcpy(platformtype,"meson6",6);
        else if (match_types("meson8",value))
            memcpy(platformtype,"meson8",6);
    }

    start_time = gettime();
    adec_print("before droppcm: drop_size=%d, nDropCount:%d, drop_max_time:%d,platform:%s, ---\n",drop_size, nDropCount, drop_max_time, platformtype);
    while(drop_size > 0 /*&& drop_duration < DROP_PCM_DURATION_THRESHHOLD*/){
        ret = audec->adsp_ops.dsp_read(&audec->adsp_ops, buffer, MIN(drop_size, 8192));
        //apts = adec_calc_pts(audec);
        //adec_print("==drop_size=%d, ret=%d, nDropCount:%d apts=0x%x,-----------------\n",drop_size, ret, nDropCount,apts);
        nDropConsumeTime = gettime() - start_time;
        if (nDropConsumeTime/1000 > drop_max_time) {
            adec_print("==ret:%d no pcm nDropCount:%d ,nDropConsumeTime:%lld reached drop_max_time:%d, \n", ret, nDropCount, nDropConsumeTime/1000,drop_max_time);
            drop_size -= ret;
            ret = -1;
            break;
        }
        if(ret==0)//no data in pcm buf
        {
            if(nDropCount>=5) {
                ret = -1;
                break;
            } else 
                nDropCount++;
            if (!strcmp(platformtype, "messon8")) {
                amthreadpool_thread_usleep(50000);  // 10ms
            } else {
                amthreadpool_thread_usleep(50000);  // 50ms
            }
            adec_print("==ret:0 no pcm nDropCount:%d \n",nDropCount);
        }
        else
        {
            nDropCount=0;
            drop_size -= ret;
        }
    }
    adec_print("after droppcm: drop_size=%d, droped:%d, nDropConsumeTime:%lld us,---\n",drop_size, size-drop_size, nDropConsumeTime);

    return ret;
}

void droppcm_prop_ctrl(int *audio_ahead, int *pts_ahead_val)
{
    char value[PROPERTY_VALUE_MAX]={0};

    if (am_getconfig_bool("media.libplayer.wfd")) {
        *pts_ahead_val = *pts_ahead_val * 2 / 3;
    }

    if(property_get("media.amplayer.apts",value,NULL) > 0){
        if(!strcmp(value,"slow")){
            *audio_ahead = -1;
        }
        else if(!strcmp(value,"fast")){
            *audio_ahead = 1;
        }
    }
    memset(value,0,sizeof(value));
    if(property_get("media.amplayer.apts_val",value,NULL) > 0){
        *pts_ahead_val = atoi(value);
    }
}

int droppcm_get_refpts(aml_audio_dec_t *audec, unsigned long *refpts)
{
    char value[PROPERTY_VALUE_MAX]={0};
    char buf[32];
    char tsync_mode_str[10];
    int tsync_mode;
    int circount = 200; // default: 200ms
    int refmode = TSYNC_MODE_AMASTER;
    int64_t start_time = gettime();
    unsigned long firstvpts = 0;

    if (amsysfs_get_sysfs_str(TSYNC_MODE, buf, sizeof(buf)) == -1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }
    if (sscanf(buf, "%d: %s", &tsync_mode, tsync_mode_str) < 1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }
    if ((tsync_mode==TSYNC_MODE_AMASTER && !strcmp(tsync_mode_str, "amaster"))
        ||(tsync_mode==TSYNC_MODE_VMASTER && !strcmp(tsync_mode_str, "vmaster") && audec->droppcm_flag)/* switch audio flag, firstvpts is unvalid when switch audio*/) {
        refmode = TSYNC_MODE_AMASTER;
    } else if (tsync_mode==TSYNC_MODE_VMASTER && !strcmp(tsync_mode_str, "vmaster") && !audec->droppcm_flag) {
        refmode = TSYNC_MODE_VMASTER;
    } else if (tsync_mode==TSYNC_MODE_PCRMASTER && !strcmp(tsync_mode_str, "pcrmaster")) {
        refmode = TSYNC_MODE_PCRMASTER;
    }

    adec_print("## [%s::%d] refmode:%d, tsync_mode:%d,%s, droppcm_flag:%d, ---\n",__FUNCTION__,__LINE__, 
        refmode, tsync_mode, tsync_mode_str, audec->droppcm_flag);
    if (refmode==TSYNC_MODE_AMASTER || refmode==TSYNC_MODE_VMASTER){
        if (sysfs_get_int(TSYNC_VPTS, refpts) == -1) {
            adec_print("## [%s::%d] unable to get vpts! \n",__FUNCTION__,__LINE__);
            return -1;
        }
        adec_print("## [%s::%d] default vpts:0x%x, ---\n",__FUNCTION__,__LINE__, *refpts);
    } else if (tsync_mode==TSYNC_MODE_PCRMASTER && !strcmp(tsync_mode_str, "pcrmaster")) {
        if (sysfs_get_int(TSYNC_PCRSCR, refpts) == -1) {
            adec_print("## [%s::%d] unable to get pts_pcrscr! \n",__FUNCTION__,__LINE__);
            return -1;
        }
        adec_print("## [%s::%d] pcrmaster, refpts:0x%x, ---\n",__FUNCTION__,__LINE__, *refpts);
    }

    if(property_get("media.amplayer.refmode",value,NULL) > 0){
        if((!strcmp(value,"firstvpts") || !strcmp(value,"0")) && !audec->droppcm_flag){
            int i = 0;

            memset(value,0,sizeof(value));
            if(property_get("media.amplayer.dropwaitxms",value,NULL) > 0)
                circount = atoi(value);
            while (!firstvpts) {
                if (sysfs_get_int(TSYNC_FIRSTVPTS, &firstvpts) == -1) {
                    adec_print("## [%s::%d] unable to get firstpts! \n",__FUNCTION__,__LINE__);
                    return -1;
                }
                if (gettime() - start_time >= (circount*1000)) {
                    adec_print("## [%s::%d] max time reached! %d ms \n",__FUNCTION__,__LINE__, circount);
                    break;
                }
                amthreadpool_thread_usleep(10000); // 10ms
            }
            adec_print("## [%s::%d] firstvpts:0x%x, use:%lld us, maxtime:%d ms, ---\n",__FUNCTION__,__LINE__, firstvpts, gettime()-start_time, circount);
            if (firstvpts) {
                *refpts = firstvpts;
                adec_print("## [%s::%d] use firstvpts, refpts:0x%x, ---\n",__FUNCTION__,__LINE__, *refpts);
            }
        }
    }

    return 0;
}

int adec_get_tsync_info(int *tsync_mode)
{
    char value[PROPERTY_VALUE_MAX]={0};
    char buf[32];
    char tsync_mode_str[10];

    if (amsysfs_get_sysfs_str(TSYNC_MODE, buf, sizeof(buf)) == -1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }
    if (sscanf(buf, "%d: %s", tsync_mode, tsync_mode_str) < 1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }

    if (tsync_mode==TSYNC_MODE_AMASTER && !strcmp(tsync_mode_str, "amaster")) {
        *tsync_mode = TSYNC_MODE_AMASTER;
    } else if (tsync_mode==TSYNC_MODE_VMASTER && !strcmp(tsync_mode_str, "vmaster")) {
        *tsync_mode = TSYNC_MODE_VMASTER;
    } else if (tsync_mode==TSYNC_MODE_PCRMASTER && !strcmp(tsync_mode_str, "pcrmaster")) {
        *tsync_mode = TSYNC_MODE_PCRMASTER;
    }

	return *tsync_mode;
}
