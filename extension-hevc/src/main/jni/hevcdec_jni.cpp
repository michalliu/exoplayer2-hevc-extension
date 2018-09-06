
#include <jni.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "log.h"

#include "openhevcwrapper/openHevcWrapper.h"

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


const char *hevc_codec_version_str(void) { return libOpenHevcVersion(NULL); }

static const char *const cfg = "-O1";

const char *hevc_codec_build_config(void) { return cfg; }

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

DECODER_FUNC(jlong, hevcInit, jobject extraData, jint len) {
    OpenHevc_Handle ohevc = nullptr;

    int mode = 1;
    int nb_pthreads = 1;
    int layer_id = 0;

    uint8_t * data = nullptr;

    ohevc = libOpenHevcInit(nb_pthreads, mode);

    if (NULL == ohevc) {
        ALOGE("[%s] libOpenHevcInit err", __func__);
        goto LABEL_RETURN;
    }

    data = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(extraData));
    libOpenHevcCopyExtraData(ohevc, data, len);

    libOpenHevcSetActiveDecoders(ohevc, layer_id);
    libOpenHevcSetViewLayers(ohevc, layer_id);

    if (libOpenHevcStartDecoder(ohevc) < 0) {
        ALOGE("[%s] libOpenHevcStartDecoder err", __func__);
        goto LABEL_RETURN;
    }

    {
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
    }

LABEL_RETURN:
    return (jlong) ohevc;
}

DECODER_FUNC(jlong, hevcClose, jlong jHandle) {
    OpenHevc_Handle ohevc = (OpenHevc_Handle) jHandle;
    libOpenHevcClose(ohevc);
    return 0;
}


DECODER_FUNC(jint, hevcDecode, jlong jHandle, jobject encoded, jint len) {
    OpenHevc_Handle ohevc = (OpenHevc_Handle) jHandle;
    const uint8_t *const buffer =
            reinterpret_cast<const uint8_t *>(env->GetDirectBufferAddress(encoded));
    int got_pic = libOpenHevcDecode(ohevc, buffer, len, 0);
    ALOGI("got_pic %d", got_pic);
    return 0;
}

DECODER_FUNC(jstring, hevcGetErrorMessage, jlong jHandle) {
    return env->NewStringUTF("");
}

DECODER_FUNC(jint, hevcGetFrame, jlong jHandle, jobject jOutputBuffer) {
    OpenHevc_Handle ohevc = (OpenHevc_Handle) jHandle;

    OpenHevc_Frame_cpy HVCFrame;
    memset(&HVCFrame, 0, sizeof(OpenHevc_Frame_cpy) );

    libOpenHevcGetPictureInfoCpy(ohevc, &HVCFrame.frameInfo);

//    int         nYPitch;
//    int         nUPitch;
//    int         nVPitch;
//    int         nBitDepth;
//    int         nWidth;
//    int         nHeight;
//    int        chromat_format;
//    OpenHevc_Rational  sample_aspect_ratio;
//    OpenHevc_Rational  frameRate;
//    int         display_picture_number;
//    int         flag; //progressive, interlaced, interlaced top field first, interlaced bottom field first.
//    int64_t     nTimeStamp;
    ALOGI("%d %d %d %d",HVCFrame.frameInfo.nWidth,
          HVCFrame.frameInfo.nHeight,
          HVCFrame.frameInfo.display_picture_number,
          HVCFrame.frameInfo.nBitDepth);


//    libOpenHevcGetPictureInfo(ohevc, &openHevcFrame.frameInfo);
//
//    int format = openHevcFrameCpy.frameInfo.chromat_format == YUV420 ? 1 : 0;
//    libOpenHevcGetPictureInfo(ohevc, &openHevcFrameCpy.frameInfo);
//    if (openHevcFrameCpy.pvY) {
//        free(openHevcFrameCpy.pvY);
//        free(openHevcFrameCpy.pvU);
//        free(openHevcFrameCpy.pvV);
//    }
//    openHevcFrameCpy.pvY = calloc(
//            openHevcFrameCpy.frameInfo.nYPitch * openHevcFrameCpy.frameInfo.nHeight,
//            sizeof(unsigned char));
//    openHevcFrameCpy.pvU = calloc(
//            openHevcFrameCpy.frameInfo.nUPitch * openHevcFrameCpy.frameInfo.nHeight >> format,
//            sizeof(unsigned char));
//    openHevcFrameCpy.pvV = calloc(
//            openHevcFrameCpy.frameInfo.nVPitch * openHevcFrameCpy.frameInfo.nHeight >> format,
//            sizeof(unsigned char));
//
//    libOpenHevcGetOutputCpy(ohevc, 1, &openHevcFrameCpy);
//
//    const int kOutputModeYuv = 0;
//    const int kOutputModeRgb = 1;
//
//    int outputMode = env->GetIntField(jOutputBuffer, outputModeField);
//    if (outputMode == kOutputModeRgb) {
//        // resize buffer if required.
//
//    } else if (outputMode == kOutputModeYuv) {
//        //const int kColorspaceYUV420 = 0;
//        //const int kColorspaceYUV422 = 1;
//        //const int kColorspaceYUV444 = 2;
//
//        const int kColorspaceUnknown = 0;
//        const int kColorspaceBT601 = 1;
//        const int kColorspaceBT709 = 2;
//        const int kColorspaceBT2020 = 3;
//
//
//        int colorspace = kColorspaceUnknown;
//
//        // resize buffer if required.
//        jboolean initResult = env->CallBooleanMethod(
//                jOutputBuffer, initForYuvFrame, openHevcFrame.frameInfo.nWidth,
//                openHevcFrame.frameInfo.nHeight,
//                openHevcFrame.frameInfo.nYPitch, openHevcFrame.frameInfo.nUPitch, colorspace);
//        if (env->ExceptionCheck() || !initResult) {
//            return -1;
//        }
//
//        // get pointer to the data buffer.
//        const jobject dataObject = env->GetObjectField(jOutputBuffer, dataField);
//        jbyte *const data =
//                reinterpret_cast<jbyte *>(env->GetDirectBufferAddress(dataObject));
//
//        const uint64_t yLength =
//                openHevcFrameCpy.frameInfo.nYPitch * openHevcFrame.frameInfo.nHeight;
//        const uint64_t uvLength =
//                openHevcFrameCpy.frameInfo.nUPitch * openHevcFrameCpy.frameInfo.nHeight >> format;
//
//        memcpy(data, openHevcFrameCpy.pvY, yLength);
//        memcpy(data + yLength, openHevcFrameCpy.pvU, uvLength);
//        memcpy(data + yLength + uvLength, openHevcFrameCpy.pvV, uvLength);
//
//        if (openHevcFrameCpy.pvY) {
//            free(openHevcFrameCpy.pvY);
//            free(openHevcFrameCpy.pvU);
//            free(openHevcFrameCpy.pvV);
//        }
//    }
    return 1;
}

// ref: https://github.com/gpac/gpac/blob/master/applications/testapps/hevcbench/main.c