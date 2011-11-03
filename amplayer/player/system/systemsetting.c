

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <log_print.h>
#include "systemsetting.h"

int PlayerSettingIsEnable(const char* path)
{
    char value[1024];
    if (GetSystemSettingString(path, value, NULL) > 0) {
        if ((!strcmp(value, "1") || !strcmp(value, "true") || !strcmp(value, "enable"))) {
            log_print("%s is enabled\n", path);
            return 1;
        }
    }
    log_print("%s is disabled\n", path);
    return 0;
}


float PlayerGetSettingfloat(const char* path)
{
    char value[1024];
    float ret = 0.0;
    if (GetSystemSettingString(path, value, NULL) > 0) {
        if ((sscanf(value, "%f", &ret)) > 0) {
            log_print("%s is set to %f\n", path, ret);
            return ret;
        }
    }
    log_print("%s is not set\n", path);
    return ret;
}

#define FILTER_VFMT_MPEG12	(1 << 0)
#define FILTER_VFMT_MPEG4	(1 << 1)
#define FILTER_VFMT_H264	(1 << 2)
#define FILTER_VFMT_MJPEG	(1 << 3)
#define FILTER_VFMT_REAL	(1 << 4)
#define FILTER_VFMT_JPEG	(1 << 5)
#define FILTER_VFMT_VC1		(1 << 6)
#define FILTER_VFMT_AVS		(1 << 7)
#define FILTER_VFMT_SW		(1 << 8)

int PlayerGetVFilterFormat(const char* path)
{
	char value[1024];
	int filter_fmt = 0;
	
	log_print("[%s:%d]path=%s\n", __FUNCTION__, __LINE__, path);
	
    if (GetSystemSettingString(path, value, NULL) > 0) {
		log_print("[%s:%d]disable_vdec=%s\n", __FUNCTION__, __LINE__, value);
		if (strstr(value,"MPEG12") != NULL) {
			filter_fmt |= FILTER_VFMT_MPEG12;
		} 
		if (strstr(value,"MPEG4") != NULL) {
			filter_fmt |= FILTER_VFMT_MPEG4;
		} 
		if (strstr(value,"H264") != NULL) {
			filter_fmt |= FILTER_VFMT_H264;
		} 
		if (strstr(value,"MJPEG") != NULL) {
			filter_fmt |= FILTER_VFMT_MJPEG;
		} 
		if (strstr(value,"REAL") != NULL) {
			filter_fmt |= FILTER_VFMT_REAL;
		} 
		if (strstr(value,"JPEG") != NULL) {
			filter_fmt |= FILTER_VFMT_JPEG;
		} 
		if (strstr(value,"VC1") != NULL) {
			filter_fmt |= FILTER_VFMT_VC1;
		} 
		if (strstr(value,"AVS") != NULL) {
			filter_fmt |= FILTER_VFMT_AVS;
		} 
		if (strstr(value,"SW") != NULL) {
			filter_fmt |= FILTER_VFMT_SW;
		}
    }
	log_print("[%s:%d]filter_vfmt=%x\n", __FUNCTION__, __LINE__, filter_fmt);
    return filter_fmt;
}

#define FILTER_AFMT_MPEG		(1 << 0)
#define FILTER_AFMT_PCMS16L	    (1 << 1)
#define FILTER_AFMT_AAC			(1 << 2)
#define FILTER_AFMT_AC3			(1 << 3)
#define FILTER_AFMT_ALAW		(1 << 4)
#define FILTER_AFMT_MULAW		(1 << 5)
#define FILTER_AFMT_DTS			(1 << 6)
#define FILTER_AFMT_PCMS16B		(1 << 7)
#define FILTER_AFMT_FLAC		(1 << 8)
#define FILTER_AFMT_COOK		(1 << 9)
#define FILTER_AFMT_PCMU8		(1 << 10)
#define FILTER_AFMT_ADPCM		(1 << 11)
#define FILTER_AFMT_AMR			(1 << 12)
#define FILTER_AFMT_RAAC		(1 << 13)
#define FILTER_AFMT_WMA			(1 << 14)
#define FILTER_AFMT_WMAPRO		(1 << 15)
#define FILTER_AFMT_PCMBLU		(1 << 16)
#define FILTER_AFMT_ALAC		(1 << 17)
#define FILTER_AFMT_VORBIS		(1 << 18)

int PlayerGetAFilterFormat(const char* path)
{
	char value[1024];
	int filter_fmt = 0;
	
	log_print("[%s:%d]path=%s\n", __FUNCTION__, __LINE__, path);
	
    if (GetSystemSettingString(path, value, NULL) > 0) {
		log_print("[%s:%d]disable_adec=%s\n", __FUNCTION__, __LINE__, value);
		if (strstr(value,"mpeg") != NULL) {
			filter_fmt |= FILTER_AFMT_MPEG;
		} 
		if (strstr(value,"pcms16l") != NULL) {
			filter_fmt |= FILTER_AFMT_PCMS16L;
		} 
		if (strstr(value,"aac") != NULL) {
			filter_fmt |= FILTER_AFMT_AAC;
		} 
		if (strstr(value,"ac3") != NULL) {
			filter_fmt |= FILTER_AFMT_AC3;
		} 
		if (strstr(value,"alaw") != NULL) {
			filter_fmt |= FILTER_AFMT_ALAW;
		} 
		if (strstr(value,"mulaw") != NULL) {
			filter_fmt |= FILTER_AFMT_MULAW;
		} 
		if (strstr(value,"dts") != NULL) {
			filter_fmt |= FILTER_AFMT_DTS;
		} 
		if (strstr(value,"pcms16b") != NULL) {
			filter_fmt |= FILTER_AFMT_PCMS16B;
		} 
		if (strstr(value,"flac") != NULL) {
			filter_fmt |= FILTER_AFMT_FLAC;
		}
		if (strstr(value,"cook") != NULL) {
			filter_fmt |= FILTER_AFMT_COOK;
		} 
		if (strstr(value,"pcmu8") != NULL) {
			filter_fmt |= FILTER_AFMT_PCMU8;
		} 
		if (strstr(value,"adpcm") != NULL) {
			filter_fmt |= FILTER_AFMT_ADPCM;
		} 
		if (strstr(value,"amr") != NULL) {
			filter_fmt |= FILTER_AFMT_AMR;
		} 
		if (strstr(value,"raac") != NULL) {
			filter_fmt |= FILTER_AFMT_RAAC;
		}
		if (strstr(value,"wma") != NULL) {
			filter_fmt |= FILTER_AFMT_WMA;
		} 
		if (strstr(value,"wmapro") != NULL) {
			filter_fmt |= FILTER_AFMT_WMAPRO;
		} 
		if (strstr(value,"pcmblueray") != NULL) {
			filter_fmt |= FILTER_AFMT_PCMBLU;
		} 
		if (strstr(value,"alac") != NULL) {
			filter_fmt |= FILTER_AFMT_ALAC;
		} 
		if (strstr(value,"vorbis") != NULL) {
			filter_fmt |= FILTER_AFMT_VORBIS;
		}
    }
	log_print("[%s:%d]filter_vfmt=%x\n", __FUNCTION__, __LINE__, filter_fmt);
    return filter_fmt;
}


