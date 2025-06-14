COMMIT_COUNT := $(shell git rev-list --count HEAD)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := psh
LOCAL_SRC_FILES := src/main.cpp

# Add these linker flags:
LOCAL_LDLIBS := -llog -landroid -lc++_static -latomic
LOCAL_CFLAGS := -Wall -Wextra
LOCAL_CPPFLAGS := -std=c++17 -frtti -fexceptions -DPSH_VERSION=$(COMMIT_COUNT)

# For NDK r21+ you might need this:
LOCAL_ARM_MODE := arm

include $(BUILD_EXECUTABLE)