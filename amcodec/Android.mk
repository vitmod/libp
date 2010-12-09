LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	codec/codec_ctrl.c \
	codec/codec_h_ctrl.c \
	codec/codec_msg.c \
	audio_ctl/audio_ctrl.c \
	subtitle/subtitle.c \
	subtitle/vob_sub.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/codec $(LOCAL_PATH)/audio_ctl $(LOCAL_PATH)/subtitle $(LOCAL_PATH)/../amadec/include
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES := libamadec
LOCAL_MODULE:= libamcodec

include $(BUILD_STATIC_LIBRARY)
