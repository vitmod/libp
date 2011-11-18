LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-message.c adec-pts-mgt.c feeder.c \
           dsp/audiodsp-ctl.c audio_out/android-out.cpp


LOCAL_MODULE := libamadec

LOCAL_ARM_MODE := arm
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-message.c adec-pts-mgt.c feeder.c \
           dsp/audiodsp-ctl.c audio_out/android-out.cpp

LOCAL_MODULE := libamadec

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

$(shell cd $(LOCAL_PATH)/firmware && { \
for f in *.bin; do \
  md5sum "$$f" > "$$f".checksum; \
done;})

copy_from := $(wildcard $(LOCAL_PATH)/firmware/*.bin)

copy_from += $(wildcard $(LOCAL_PATH)/firmware/*.checksum)

LOCAL_MODULE := audio_firmware
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES := $(copy_from)

audio_firmware: $(copy_from) | $(ACP)
	$(hide) mkdir -p $(TARGET_OUT_ETC)/firmware/
	$(hide) $(ACP) -fp $(copy_from) $(TARGET_OUT_ETC)/firmware/

include $(BUILD_PHONY_PACKAGE)

