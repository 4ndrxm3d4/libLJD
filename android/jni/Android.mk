LOCAL_PATH := $(call my-dir)

# === Core library (for standalone .so consumers) ===
include $(CLEAR_VARS)
LOCAL_MODULE := ljd
LOCAL_CPP_EXTENSION := cpp
LOCAL_SRC_FILES := \
    ../bytecode/bytecode.cpp \
    ../bytecode/prototype.cpp \
    ../ast/ast.cpp \
    ../lua/lua.cpp \
    ../library/src/ljd_api.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../ \
    $(LOCAL_PATH)/../library/include
LOCAL_CFLAGS += -DLJD_BUILD=1
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Werror
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)

# === JNI bridge (same .so, exports JNI functions) ===
include $(CLEAR_VARS)
LOCAL_MODULE := ljd_jni
LOCAL_CPP_EXTENSION := cpp
LOCAL_SRC_FILES := \
    ../library/src/ljd_jni.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../ \
    $(LOCAL_PATH)/../library/include \
    $(LOCAL_PATH)/../android/native-lib/src/main/jni
LOCAL_CFLAGS += -DLJD_BUILD=1
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Werror
LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := ljd
include $(BUILD_SHARED_LIBRARY)
