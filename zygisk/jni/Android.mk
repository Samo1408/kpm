LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := universal_spoof
LOCAL_SRC_FILES := zygisk_module.cpp ../../common/src/spoof_profile.c
LOCAL_C_INCLUDES := $(LOCAL_PATH) ../../common/include
LOCAL_CPPFLAGS := -std=c++17 -fvisibility=hidden -fno-exceptions -fno-rtti
LOCAL_CFLAGS := -O2 -fvisibility=hidden
LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := -Wl,--gc-sections
include $(BUILD_SHARED_LIBRARY)
