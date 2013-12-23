LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := bootplayer
LOCAL_MODULE_TAGS := samples
LOCAL_SRC_FILES := kplayer.c
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amplayer/player/include \
    $(LOCAL_PATH)/../amcodec/include \
    $(LOCAL_PATH)/../amadec/include \
    $(LOCAL_PATH)/../amffmpeg \
    $(JNI_H_INCLUDE) \
    $(LOCAL_PATH)/../streamsource \

LOCAL_STATIC_LIBRARIES := libamplayer libamplayer libamcodec libavformat librtmp libavcodec libavutil libamadec_alsa libamavutils_alsa libamstreaming
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder libz libdl libcutils libssl libcrypto libasound

include $(BUILD_EXECUTABLE)

