LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    Printer.cpp
LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := \
    libutils \
    libnativehelper \
    liblog

LOCAL_MODULE := libCMCC_PRINT_BOSSTUN
LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)
