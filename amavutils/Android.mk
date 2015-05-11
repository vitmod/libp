LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_USE_TRIPLE_FB_BUFFERS), true)
LOCAL_CFLAGS += -DENABLE_FB_TRIPLE_BUFFERS
endif

ifeq ($(TARGET_EXTERNAL_DISPLAY),true)
ifeq ($(TARGET_SINGLE_EXTERNAL_DISPLAY_USE_FB1),true)
LOCAL_CFLAGS += -DSINGLE_EXTERNAL_DISPLAY_USE_FB1
endif
endif

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 	
LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.cpp)) 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
     $(LOCAL_PATH)/../amcodec/include \
	 $(JNI_H_INCLUDE) \
	 $(TOP)/frameworks/native/services \
	 $(TOP)/frameworks/native/include 



LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES +=  libcutils libc libdl
LOCAL_SHARED_LIBRARIES += libbinder \
                          libsystemwriteservice
LOCAL_SHARED_LIBRARIES += libui libgui 
                          
LOCAL_MODULE := libamavutils
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

ifeq ($(TARGET_USE_TRIPLE_FB_BUFFERS), true)
LOCAL_CFLAGS += -DENABLE_FB_TRIPLE_BUFFERS
endif

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 	
LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.cpp))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
         $(LOCAL_PATH)/../amcodec/include \
         $(JNI_H_INCLUDE) \
         $(TOP)/frameworks/native/services \
         $(TOP)/frameworks/native/include 

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES +=  libcutils libc
LOCAL_SHARED_LIBRARIES += libui libgui 

LOCAL_MODULE := libamavutils
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_CFLAGS+=-DNO_USE_SYSWRITE

ifeq ($(TARGET_USE_TRIPLE_FB_BUFFERS), true)
LOCAL_CFLAGS += -DENABLE_FB_TRIPLE_BUFFERS
endif

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 	
LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.cpp)) 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
     $(LOCAL_PATH)/../amcodec/include \
	 $(JNI_H_INCLUDE) \
	 $(TOP)/frameworks/native/services \
	 $(TOP)/frameworks/native/include 

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES +=  libcutils libc libdl
LOCAL_SHARED_LIBRARIES += libbinder \
                          libsystemwriteservice
LOCAL_SHARED_LIBRARIES += libui libgui 


LOCAL_MODULE := libamavutils_alsa
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)



include $(CLEAR_VARS)

LOCAL_CFLAGS+=-DNO_USE_SYSWRITE

ifeq ($(TARGET_USE_TRIPLE_FB_BUFFERS), true)
LOCAL_CFLAGS += -DENABLE_FB_TRIPLE_BUFFERS
endif

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 	
LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.cpp))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
         $(call include-path-for, graphics corecg)    \
         $(LOCAL_PATH)/../amcodec/include \
         $(JNI_H_INCLUDE) \
         $(TOP)/frameworks/native/services \
         $(TOP)/frameworks/native/include \




    
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES +=  libcutils libc
LOCAL_SHARED_LIBRARIES += libui libgui \


LOCAL_MODULE := libamavutils_alsa
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)
