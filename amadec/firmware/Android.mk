LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

$(shell cd $(LOCAL_PATH) && { \
for f in *.bin; do \
  md5sum "$$f" > "$$f".checksum; \
done;})

copy_from := $(wildcard $(LOCAL_PATH)/*.bin)

copy_from += $(wildcard $(LOCAL_PATH)/*.checksum)


LOCAL_MODULE := audio_firmware
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES := $(copy_from)

audio_firmware: $(copy_from) | $(ACP)
	$(hide) mkdir -p $(TARGET_OUT_ETC)/firmware/
	$(hide) $(ACP) -fp $(copy_from) $(TARGET_OUT_ETC)/firmware/

$(error build from this directory disabled)
include $(BUILD_PHONY_PACKAGE)

