TOP_PATH := $(call my-dir)

# libyuv
include $(CLEAR_VARS)
LOCAL_PATH := $(TOP_PATH)/libyuv
LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
    source/compare.cc           \
    source/compare_common.cc    \
    source/convert.cc           \
    source/convert_argb.cc      \
    source/convert_from.cc      \
    source/convert_from_argb.cc \
    source/convert_to_argb.cc   \
    source/convert_to_i420.cc   \
    source/cpu_id.cc            \
    source/planar_functions.cc  \
    source/rotate.cc            \
    source/rotate_any.cc        \
    source/rotate_argb.cc       \
    source/rotate_common.cc     \
    source/row_any.cc           \
    source/row_common.cc        \
    source/scale.cc             \
    source/scale_any.cc         \
    source/scale_argb.cc        \
    source/scale_common.cc      \
    source/video_common.cc

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS += -DLIBYUV_NEON
    LOCAL_SRC_FILES += \
        source/compare_neon.cc.neon    \
        source/rotate_neon.cc.neon     \
        source/row_neon.cc.neon        \
        source/scale_neon.cc.neon
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_CFLAGS += -DLIBYUV_NEON
    LOCAL_SRC_FILES += \
        source/compare_neon64.cc    \
        source/rotate_neon64.cc     \
        source/row_neon64.cc        \
        source/scale_neon64.cc
endif

ifeq ($(TARGET_ARCH_ABI),$(filter $(TARGET_ARCH_ABI), x86 x86_64))
    LOCAL_SRC_FILES += \
        source/compare_gcc.cc       \
        source/rotate_gcc.cc        \
        source/row_gcc.cc           \
        source/scale_gcc.cc
endif

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_MODULE := yuv
include $(BUILD_STATIC_LIBRARY)

# build openhevc static library
include $(CLEAR_VARS)
LOCAL_PATH := $(TOP_PATH)/openhevc

LOCAL_CFLAGS    += -O1

openhevc_files := \
    libavutil/avstring.c \
    libavutil/atomic.c \
    libavutil/base64.c \
    libavutil/bprint.c \
    libavutil/buffer.c \
    libavutil/channel_layout.c \
    libavutil/cpu.c \
    libavutil/crc.c \
    libavutil/des.c \
    libavutil/dict.c \
    libavutil/error.c \
    libavutil/eval.c \
    libavutil/file_open.c \
    libavutil/frame.c \
    libavutil/imgutils.c \
    libavutil/intmath.c \
    libavutil/log.c \
    libavutil/log2_tab.c \
    libavutil/mathematics.c \
    libavutil/md5.c \
    libavutil/mem.c \
    libavutil/opt.c \
    libavutil/parseutils.c \
    libavutil/pixdesc.c \
    libavutil/rational.c \
    libavutil/random_seed.c \
    libavutil/rc4.c \
    libavutil/samplefmt.c \
    libavutil/sha.c \
    libavutil/time.c \
    libavutil/timecode.c \
    libavutil/utils.c \
    libavutil/arm/cpu.c \
    libavutil/display.c \
    libavutil/stereo3d.c \
    libavcodec/arm/videodsp_init_arm.c \
    libavcodec/ac3tab.c \
    libavcodec/allcodecs.c \
    libavcodec/avfft.c \
    libavcodec/avpacket.c \
    libavcodec/avpicture.c \
    libavcodec/bitstream.c \
    libavcodec/bitstream_filter.c \
    libavcodec/bswapdsp.c \
    libavcodec/cabac.c \
    libavcodec/codec_desc.c \
    libavcodec/dct.c \
    libavcodec/dct32_float.c \
    libavcodec/dct32_template.c \
    libavcodec/faanidct.c \
    libavcodec/fft_template.c \
    libavcodec/golomb.c \
    libavcodec/hevc_cabac.c \
    libavcodec/hevc_mvs.c \
    libavcodec/hevc_parser.c \
    libavcodec/hevc_ps.c \
    libavcodec/hevc_refs.c \
    libavcodec/hevc_sei.c \
    libavcodec/hevc_filter.c \
    libavcodec/hevc.c \
    libavcodec/hevcdsp.c \
    libavcodec/hevcpred.c \
    libavcodec/hpeldsp.c \
    libavcodec/jrevdct.c \
    libavcodec/mathtables.c \
    libavcodec/me_cmp.c \
    libavcodec/mdct_template.c \
    libavcodec/mpegaudiodata.c \
    libavcodec/mpeg4audio.c \
    libavcodec/imgconvert.c \
    libavcodec/options.c \
    libavcodec/parser.c \
    libavcodec/pthread_slice.c \
    libavcodec/pthread_frame.c \
    libavcodec/pthread.c \
    libavcodec/raw.c \
    libavcodec/rawdec.c \
    libavcodec/rdft.c \
    libavcodec/simple_idct.c \
    libavcodec/utils.c \
    libavcodec/videodsp.c \
    libavcodec/arm/hevcdsp_init_arm.c \
    libavcodec/arm/fft_init_arm.c \
    libavcodec/arm/hpeldsp_init_arm.c \
    libavcodec/arm/hpeldsp_init_neon.c \
    libavcodec/arm/hpeldsp_arm.S \
    libavcodec/arm/hpeldsp_neon.S \
    libavcodec/arm/fft_neon.S \
    libavcodec/arm/rdft_neon.S \
    libavutil/arm/asm.S \
    libavcodec/arm/hevcdsp_deblock_neon.S \
    libavcodec/arm/hevcdsp_idct_neon.S \
    libavcodec/arm/hevcdsp_qpel_neon.S \
    libavcodec/arm/hevcdsp_epel_neon.S \
    libavcodec/arm/simple_idct_neon.S \
    libavcodec/arm/simple_idct_arm.S \
    libavcodec/arm/simple_idct_armv6.S \
    libavcodec/arm/jrevdct_arm.S \
    libavcodec/arm/int_neon.S \
    libavcodec/arm/mdct_neon.S

LOCAL_SRC_FILES := $(openhevc_files)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/platform/arm/
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_MODULE := openhevc
ifeq ($(TARGET_ARCH),arm)
  LOCAL_SDK_VERSION := 9
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := true
endif

LOCAL_LDLIBS := -llog -lz -lm
LOCAL_LDLIBS += -Wl,--no-warn-shared-textrel

include $(BUILD_STATIC_LIBRARY)

# Build openhevcwrapper
include $(CLEAR_VARS)
LOCAL_PATH := $(TOP_PATH)/openhevcwrapper

LOCAL_SRC_FILES := openHevcWrapper.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_MODULE := openhevcwrapper
LOCAL_STATIC_LIBRARIES := openhevc

include $(BUILD_STATIC_LIBRARY)

# Build libopenhevc.so
include $(CLEAR_VARS)
LOCAL_PATH := $(TOP_PATH)

LOCAL_SRC_FILES := hevcdec_jni.cc

LOCAL_LDLIBS := -llog

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_CPPFLAGS += -frtti -fexceptions -DANDROID -std=c++11

LOCAL_MODULE := hevcdec
LOCAL_STATIC_LIBRARIES := openhevcwrapper cpufeatures yuv


ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)