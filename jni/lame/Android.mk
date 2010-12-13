LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# sources.mk adds c source files to LOCAL_SRC_FILES
include $(LOCAL_PATH)/sources.mk
include $(LOCAL_PATH)/mpglib/sources.mk

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(LOCAL_PATH)/mpglib

LOCAL_MODULE := lame
LOCAL_CLFAGS := -std=c99
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
