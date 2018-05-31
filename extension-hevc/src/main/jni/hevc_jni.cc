#include <jni.h>

#include <android/log.h>
#include "HevcDec.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define LOG_TAG "hevc_jni"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, \
                                             __VA_ARGS__))

#define DECODER_FUNC(RETURN_TYPE, NAME, ...) \
  extern "C" { \
  JNIEXPORT RETURN_TYPE \
    Java_com_google_android_exoplayer2_ext_hevc_HevcDecoder_ ## NAME \
      (JNIEnv* env, jobject thiz, ##__VA_ARGS__);\
  } \
  JNIEXPORT RETURN_TYPE \
    Java_com_google_android_exoplayer2_ext_hevc_HevcDecoder_ ## NAME \
      (JNIEnv* env, jobject thiz, ##__VA_ARGS__)\


#define LIBRARY_FUNC(RETURN_TYPE, NAME, ...) \
  extern "C" { \
  JNIEXPORT RETURN_TYPE \
    Java_com_google_android_exoplayer2_ext_hevc_HevcLibrary_ ## NAME \
      (JNIEnv* env, jobject thiz, ##__VA_ARGS__);\
  } \
  JNIEXPORT RETURN_TYPE \
    Java_com_google_android_exoplayer2_ext_hevc_HevcLibrary_ ## NAME \
      (JNIEnv* env, jobject thiz, ##__VA_ARGS__)\

#define VERSION_STRING_NOSP "v1.6.1-1214-gb85e391"

const char *hevc_codec_version_str(void) { return VERSION_STRING_NOSP; }

static const char* const cfg = "";
const char *hevc_codec_build_config(void) {return cfg;}

LIBRARY_FUNC(jstring, hevcIsSecureDecodeSupported) {
  // Doesn't support
  return 0;
}

LIBRARY_FUNC(jstring, hevcGetVersion) {
  return env->NewStringUTF(hevc_codec_version_str());
}

LIBRARY_FUNC(jstring, hevcGetBuildConfig) {
  return env->NewStringUTF(hevc_codec_build_config());
}

// JNI references for VpxOutputBuffer class.
static jmethodID initForRgbFrame;
static jmethodID initForYuvFrame;
static jfieldID dataField;
static jfieldID outputModeField;
static int errorCode;


DECODER_FUNC(jlong, hevcInit, jboolean disableLoopFilter) {
  OpenHevc_Handle openHevcHandle;
  errorCode = 0;

  int thread_type       = 1;
  int nb_pthreads       = 1;
  openHevcHandle = libOpenHevcInit(nb_pthreads, thread_type);

  if (NULL == openHevcHandle)
  {
      errorCode = -1;
      return 0;
  }

  if (errorCode) {
//    LOGE("ERROR: Failed to initialize libhevc decoder");
  }

  // Populate JNI References.
  const jclass outputBufferClass = env->FindClass(
      "com/google/android/exoplayer2/ext/hevc/HevcOutputBuffer");
  initForYuvFrame = env->GetMethodID(outputBufferClass, "initForYuvFrame",
                                     "(IIIII)Z");
  initForRgbFrame = env->GetMethodID(outputBufferClass, "initForRgbFrame",
                                     "(II)Z");
  dataField = env->GetFieldID(outputBufferClass, "data",
                              "Ljava/nio/ByteBuffer;");
  outputModeField = env->GetFieldID(outputBufferClass, "mode", "I");

  return (jlong)openHevcHandle;
}

DECODER_FUNC(jlong, hevcClose, jlong jHandle) {
  OpenHevc_Handle openHevcHandle = (OpenHevc_Handle)jHandle;
  libOpenHevcClose(openHevcHandle);
  return 0;
}


DECODER_FUNC(jlong, hevcDecode, jlong jHandle, jobject encoded, jint len) {
  OpenHevc_Handle openHevcHandle = (OpenHevc_Handle)jHandle;
  const uint8_t* const buffer =
      reinterpret_cast<const uint8_t*>(env->GetDirectBufferAddress(encoded));
  int got_picture = libOpenHevcDecode(openHevcHandle, buffer, len, 0);

  return 0;
}


DECODER_FUNC(jint, hevcGetFrame, jlong jHandle, jobject jOutputBuffer) {
  OpenHevc_Handle openHevcHandle = (OpenHevc_Handle)jHandle;
  OpenHevc_Frame openHevcFrame;
  OpenHevc_Frame_cpy openHevcFrameCpy;

  libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrame.frameInfo);

  int format = openHevcFrameCpy.frameInfo.chromat_format == YUV420 ? 1 : 0;
  libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrameCpy.frameInfo);
  if(openHevcFrameCpy.pvY) {
      free(openHevcFrameCpy.pvY);
      free(openHevcFrameCpy.pvU);
      free(openHevcFrameCpy.pvV);
  }
  openHevcFrameCpy.pvY = calloc (openHevcFrameCpy.frameInfo.nYPitch * openHevcFrameCpy.frameInfo.nHeight, sizeof(unsigned char));
  openHevcFrameCpy.pvU = calloc (openHevcFrameCpy.frameInfo.nUPitch * openHevcFrameCpy.frameInfo.nHeight >> format, sizeof(unsigned char));
  openHevcFrameCpy.pvV = calloc (openHevcFrameCpy.frameInfo.nVPitch * openHevcFrameCpy.frameInfo.nHeight >> format, sizeof(unsigned char));

  libOpenHevcGetOutputCpy(openHevcHandle, 1, &openHevcFrameCpy);

  const int kOutputModeYuv = 0;
  const int kOutputModeRgb = 1;

  int outputMode = env->GetIntField(jOutputBuffer, outputModeField);
  if (outputMode == kOutputModeRgb) {
    // resize buffer if required.

  } else if (outputMode == kOutputModeYuv) {
    //const int kColorspaceYUV420 = 0;
    //const int kColorspaceYUV422 = 1;
    //const int kColorspaceYUV444 = 2;

    const int kColorspaceUnknown = 0;
    const int kColorspaceBT601 = 1;
    const int kColorspaceBT709 = 2;
    const int kColorspaceBT2020 = 3;


    int colorspace = kColorspaceUnknown;

    // resize buffer if required.
    jboolean initResult = env->CallBooleanMethod(
        jOutputBuffer, initForYuvFrame, openHevcFrame.frameInfo.nWidth, openHevcFrame.frameInfo.nHeight,
        openHevcFrame.frameInfo.nYPitch, openHevcFrame.frameInfo.nUPitch, colorspace);
    if (env->ExceptionCheck() || !initResult) {
      return -1;
    }

    // get pointer to the data buffer.
    const jobject dataObject = env->GetObjectField(jOutputBuffer, dataField);
    jbyte* const data =
        reinterpret_cast<jbyte*>(env->GetDirectBufferAddress(dataObject));

    const uint64_t yLength = openHevcFrameCpy.frameInfo.nYPitch * openHevcFrame.frameInfo.nHeight;
    const uint64_t uvLength = openHevcFrameCpy.frameInfo.nUPitch * openHevcFrameCpy.frameInfo.nHeight >> format;

    memcpy(data, openHevcFrameCpy.pvY, yLength);
    memcpy(data + yLength, openHevcFrameCpy.pvU, uvLength);
    memcpy(data + yLength + uvLength, openHevcFrameCpy.pvV, uvLength);

    if(openHevcFrameCpy.pvY) {
        free(openHevcFrameCpy.pvY);
        free(openHevcFrameCpy.pvU);
        free(openHevcFrameCpy.pvV);
    }
  }
  return 0;
}