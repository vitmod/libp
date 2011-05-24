
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 												

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../../amcodec/include \
	$(LOCAL_PATH)/../../amadec/include \
	$(LOCAL_PATH)/../../amffmpeg

LOCAL_MODULE := libamplayer

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 									

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../../amcodec/include \
        $(LOCAL_PATH)/../../amadec/include \
        $(LOCAL_PATH)/../../amffmpeg

LOCAL_STATIC_LIBRARIES := libamcodec libavformat libavcodec libavutil libamadec 
LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc

LOCAL_MODULE := libamplayer
LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

