LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := splayer
LOCAL_SRC_FILES := splayer.c
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amplayer/player/include \
    $(LOCAL_PATH)/../amplayer/control/include \
    $(LOCAL_PATH)/../amcodec/include \
    $(LOCAL_PATH)/../amadec/include \
    $(LOCAL_PATH)/../amffmpeg \
    $(JNI_H_INCLUDE) 

LOCAL_STATIC_LIBRARIES := libamplayer libamcontroler libamplayer libamcodec libavformat libavcodec libavutil libamadec 
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE    := cplayer
LOCAL_SRC_FILES := cplayer.c
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amplayer/player/include \
    $(LOCAL_PATH)/../amplayer/control/include \
    $(LOCAL_PATH)/../amcodec/include \
    $(LOCAL_PATH)/../amadec/include \
    $(LOCAL_PATH)/../amffmpeg \
    $(JNI_H_INCLUDE)

LOCAL_STATIC_LIBRARIES := libamcodec libamadec  libavformat libavcodec libavutil
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder

include $(BUILD_EXECUTABLE)


