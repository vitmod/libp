LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE := libdash_mod.so
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_VERSION := $(shell expr substr "$(PLATFORM_VERSION)" 1 3)
LOCAL_SRC_FILES := libdash_mod_$(LOCAL_MODULE_VERSION).so
LOCAL_MODULE_PATH:=$(TARGET_OUT)/lib/amplayer

include $(BUILD_PREBUILT) 
