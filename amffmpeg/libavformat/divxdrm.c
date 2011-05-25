#include <dlfcn.h> 
#include <string.h>
#include <stdlib.h>

#include "divxdrm.h"

#include <fcntl.h>


#include <android/log.h>
#define LOG_TAG "divx_drm"

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define DRMLIB "libdivxdrm.so"

static void* fd = NULL;
static uint8_t * drmContext = NULL;
static drm_t drm;

static drmErrorCodes_t	(*func_drmInitSystem)(uint8_t*, uint32_t*) = NULL;
static drmErrorCodes_t  (*func_drmInitPlayback)(uint8_t*, uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmQueryRentalStatus)(uint8_t*, uint8_t*, uint8_t*, uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmSetRandomSample)(uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmQueryCgmsa)(uint8_t*, uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmQueryAcptb)(uint8_t*, uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmQueryDigitalProtection)(uint8_t*, uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmQueryIct)(uint8_t*, uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmCommitPlayback)(uint8_t*) = NULL;        
static drmErrorCodes_t	(*func_drmDecryptVideo)(uint8_t *,uint8_t*,uint32_t,uint8_t*) = NULL;
static drmErrorCodes_t  (*func_drmDecryptAudio)(uint8_t*, uint8_t*, uint32_t) = NULL;
static drmErrorCodes_t	(*func_drmGetDeactivationCodeString)(uint8_t*, char*);
static drmErrorCodes_t	(*func_drmGetRegistrationCodeString)(uint8_t*, char*);
static drmErrorCodes_t	(*func_drmGetActivationMessage)(uint8_t*, int8_t*,uint32_t*);
static drmErrorCodes_t	(*func_drmGetDeactivationMessage)(uint8_t*, int8_t*,uint32_t*);
static drmErrorCodes_t 	(*func_drmGetActivationStatus)( uint8_t* , uint32_t* );
static drmErrorCodes_t	(*func_drmIsDeviceActivated)(uint8_t*);
static drmErrorCodes_t	(*func_drmFinalizePlayback)(uint8_t*);
static drmErrorCodes_t	(*func_drmGetLastError)(uint8_t*);
static drmErrorCodes_t	(*func_drmInitDrmMemory)();

drmErrorCodes_t drmInitSystem()
{
	unsigned drmContextLength=0;
	drmErrorCodes_t res;
	LOGI("drmInitSystem\n");
	if(func_drmInitSystem == NULL){
		LOGE("drm lib not initialized\n");
		return DRM_GENERAL_ERROR;
	}
	
	if(func_drmInitSystem(NULL, &drmContextLength) == 0){		
		drmContext = malloc(drmContextLength);
		if(drmContext){			
			return func_drmInitSystem(drmContext, &drmContextLength);   
			return res;
		}
	}
	LOGE("drmInitSystem failed\n");
	return DRM_GENERAL_ERROR;
}

drmErrorCodes_t drmInitPlayback(uint8_t *strdData )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
		
	LOGI("drmInitPlayback\n");
	
	return func_drmInitPlayback(drmContext, strdData);
}

drmErrorCodes_t drmQueryRentalStatus( uint8_t *rentalMessageFlag,
                                      uint8_t *useLimit, 
                                      uint8_t *useCount )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	LOGI("drmQueryRentalStatus\n");
		
	return func_drmQueryRentalStatus(drmContext, rentalMessageFlag,useLimit,useCount);
}

drmErrorCodes_t drmSetRandomSample()
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
		
	LOGI("drmSetRandomSample\n");
	
	return func_drmSetRandomSample(drmContext);
}

drmErrorCodes_t drmCommitPlayback()
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
		
	LOGI("drmCommitPlayback\n");
	
	return func_drmCommitPlayback(drmContext);
}

drmErrorCodes_t drmFinalizePlayback()
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
		
	return func_drmFinalizePlayback(drmContext);
}

drmErrorCodes_t drmDecryptVideo( uint8_t *frame,
                                 uint32_t frameSize, 
                                 uint8_t *drmFrameInfo )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	return func_drmDecryptVideo(drmContext, frame, frameSize, drmFrameInfo);
}

drmErrorCodes_t drmDecryptAudio( uint8_t *frame,
                                 uint32_t frameSize)
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	
	return func_drmDecryptAudio(drmContext, frame, frameSize);
}

drmErrorCodes_t drmQueryCgmsa(uint8_t *cgmsaSignal )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	
	return func_drmQueryCgmsa(drmContext, cgmsaSignal);
}

drmErrorCodes_t drmQueryAcptb(uint8_t *acptbSignal )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	
	return func_drmQueryAcptb(drmContext, acptbSignal);
}

drmErrorCodes_t drmQueryDigitalProtection(uint8_t *digitalProtectionSignal )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	return func_drmQueryDigitalProtection(drmContext, digitalProtectionSignal);
}

drmErrorCodes_t drmQueryIct(uint8_t *ict )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	return func_drmQueryIct(drmContext, ict);
}

drmErrorCodes_t drmGetLastError()
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	
	return func_drmGetLastError(drmContext);
}

drmErrorCodes_t drmInitDrmMemory()
{
	
	return func_drmInitDrmMemory();
}

int8_t drmIsDeviceActivated( )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
		
	
	return func_drmIsDeviceActivated(drmContext);
}
drmErrorCodes_t drmGetActivationStatus( uint8_t* userId,
                                        uint32_t* userIdLength )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
		
	return func_drmGetActivationStatus(userId, userIdLength);
}
drmErrorCodes_t drmGetDeactivationMessage(int8_t* messageString,
                                           uint32_t* messageStringLength  )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
		
	return func_drmGetDeactivationMessage(drmContext, messageString, messageStringLength);
}

drmErrorCodes_t drmGetActivationMessage(int8_t* activationMessage,
                                         uint32_t* activationMessageLength )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	return func_drmGetActivationMessage(drmContext, activationMessage, activationMessageLength);
}

drmErrorCodes_t drmGetDeactivationCodeString(char *deactivationCodeString )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	return func_drmGetDeactivationCodeString(drmContext, deactivationCodeString);
}

drmErrorCodes_t drmGetRegistrationCodeString(char *registrationCodeString )
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	
	return func_drmGetRegistrationCodeString(drmContext, registrationCodeString);
}

uint32_t drmGetRandomSampleCounter()
{
	if(drmContext == NULL)
		return DRM_GENERAL_ERROR;
	return 0;
}

void drm_set_info(drm_t* pdrm)
{
  memcpy(&drm, pdrm, sizeof(drm_t));
}

drm_t* drm_get_info()
{
  return &drm;
}

int drm_init()
{
	fd = dlopen(DRMLIB, RTLD_NOW);
	if(fd == NULL){
		LOGE("open %s error\n", DRMLIB);
		return -1;
	}
  
	do{
		func_drmInitSystem                =   dlsym(fd, "drmInitSystem");
		if(func_drmInitSystem == NULL)
			break;

		func_drmInitPlayback              =   dlsym(fd, "drmInitPlayback");
		if(func_drmInitPlayback == NULL)
			break;

		func_drmQueryRentalStatus         =   dlsym(fd, "drmQueryRentalStatus");
		if(func_drmQueryRentalStatus == NULL)
			break;

		func_drmSetRandomSample           =   dlsym(fd, "drmSetRandomSample");
		if(func_drmSetRandomSample == NULL)
			break;

		func_drmQueryCgmsa                =   dlsym(fd, "drmQueryCgmsa");
		if(func_drmQueryCgmsa == NULL)
			break;

		func_drmQueryAcptb                =   dlsym(fd, "drmQueryAcptb");
		if(func_drmQueryAcptb == NULL)
			break;

		func_drmQueryDigitalProtection    =   dlsym(fd, "drmQueryDigitalProtection");
		if(func_drmQueryDigitalProtection == NULL)
			break;

		func_drmQueryIct                  =   dlsym(fd, "drmQueryIct");
		if(func_drmQueryIct == NULL)
			break;

		func_drmCommitPlayback            =   dlsym(fd, "drmCommitPlayback");
		if(func_drmCommitPlayback == NULL)
			break;

		func_drmDecryptVideo				=	dlsym(fd,  "drmDecryptVideo");
		if(func_drmDecryptVideo == NULL)
			break;
		
        func_drmDecryptAudio                =   dlsym(fd, "drmDecryptAudio");
        if(func_drmDecryptAudio == NULL)
            break;

		func_drmGetDeactivationCodeString	=	dlsym(fd, "drmGetDeactivationCodeString");
		if(func_drmGetDeactivationCodeString == NULL)
			break;
		
		func_drmGetRegistrationCodeString 	= 	dlsym(fd, "drmGetRegistrationCodeString");
		if(func_drmGetRegistrationCodeString == NULL)
			break;
		
		func_drmGetActivationMessage		=	dlsym(fd, "drmGetActivationMessage");
		if(func_drmGetActivationMessage == NULL)
			break;
		
		func_drmGetDeactivationMessage		=	dlsym(fd, "drmGetDeactivationMessage");
		if(func_drmGetDeactivationMessage == NULL)
			break;
			
		func_drmIsDeviceActivated			=	dlsym(fd, "drmIsDeviceActivated");
		if(func_drmIsDeviceActivated == NULL)
			break;
		func_drmInitDrmMemory				=	dlsym(fd, "drmInitDrmMemory");
		if(func_drmInitDrmMemory == NULL)
			break;
		
        func_drmGetLastError                =   dlsym(fd, "drmGetLastError");
        if(func_drmGetLastError == NULL)
            break;

		LOGI("libdrmdivx.so loaded\n");
		
		/* should called once after the device setup */
		if(open("/sdcard/drm.bin", O_RDONLY) < 0){
			LOGI("init drm device\n");
			func_drmInitDrmMemory();
		}
		
		return 0;
	}while(0);
	
	LOGE("libdivxdrm.so load failed\n");
	return -1;
}

void drm_deinit()
{
  if(fd){
    dlclose(fd);
  }
  LOGI("unload %s\n", DRMLIB);
}


