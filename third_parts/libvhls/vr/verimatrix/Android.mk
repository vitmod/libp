ifeq ($(BUILD_WITH_VIEWRIGHT_WEB), true)

 LOCAL_PATH:= $(call my-dir)
 
 include $(CLEAR_VARS)

 
 LOCAL_MODULE := libViewRightWebClient
 LOCAL_MODULE_CLASS := SHARED_LIBRARIES
 LOCAL_MODULE_SUFFIX := .so
 LOCAL_SRC_FILES := lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
 $(warning !!!!!!!!!Do not forget to place your own libViewRightWebClient.so under lib directory)
 LOCAL_PROPRIETARY_MODULE := true
 LOCAL_STRIP_MODULE := false
 
 LOCAL_MODULE_TAGS := optional
 include $(BUILD_PREBUILT)
 
 include $(CLEAR_VARS)
 LOCAL_SRC_FILES := src/VCASCommunication.cpp
 LOCAL_MODULE := libVCASCommunication
 LOCAL_MODULE_TAGS := optional
 LOCAL_SHARED_LIBRARIES := libViewRightWebClient liblog
 LOCAL_C_INCLUDES  = $(LOCAL_PATH)/inc/
 include $(BUILD_SHARED_LIBRARY)

endif
