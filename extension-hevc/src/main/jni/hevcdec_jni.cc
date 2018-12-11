
#include <cpu-features.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif
#include <jni.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "libyuv.h"
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

typedef struct Info {
    int NbFrame;
    int Poc;
    int Tid;
    int Qid;
    int type;
    int size;
} Info;

static Info info;

static int pic_width = -1;
static int pic_height = -1;

static int temporal_layer_id = 0;
static int quality_layer_id = 0;

typedef enum {
    YUV = 0,//要和HevcOutputBuffer中的mode的定义保持一致
    RGB
} OutputMode;

typedef enum {
    PIXFMT_UNKNOWN = 0, //要和HevcOutputBuffer中的PIXFMT保持一致
    PIXFMT_RGB565,
    PIXFMT_ARGB8888
} OutputPixFmt;

LIBRARY_FUNC(jboolean, hevcIsSecureDecodeSupported) {
    // Doesn't support
    // Java client should have checked hevcIsSecureDecodeSupported
    // and avoid calling this
    // return -2 (DRM Error)
    return 0;
}

LIBRARY_FUNC(jstring, hevcGetVersion) {
    return env->NewStringUTF(hevc_codec_version_str());
}

LIBRARY_FUNC(jstring, hevcGetBuildConfig) {
    return env->NewStringUTF(hevc_codec_build_config());
}

#ifdef __ARM_NEON__
static int convert_16_to_8_neon(const OpenHevc_Frame* const img, jbyte* const data,
                                const int32_t uvHeight, const int32_t yLength,
                                const int32_t uvLength) {
  if (!(android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON)) return 0;
  uint32x2_t lcg_val = vdup_n_u32(random());
  lcg_val = vset_lane_u32(random(), lcg_val, 1);
  // LCG values recommended in good ol' "Numerical Recipes"
  const uint32x2_t LCG_MULT = vdup_n_u32(1664525);
  const uint32x2_t LCG_INCR = vdup_n_u32(1013904223);

  const uint16_t* srcBase =
      reinterpret_cast<uint16_t*>(img->pvY);
  uint8_t* dstBase = reinterpret_cast<uint8_t*>(data);
  // In units of uint16_t, so /2 from raw stride
  const int srcStride = img->frameInfo.nYPitch / 2;
  const int dstStride = img->frameInfo.nYPitch;

  for (int y = 0; y < img->frameInfo.nHeight; y++) {
    const uint16_t* src = srcBase;
    uint8_t* dst = dstBase;

    // Each read consumes 4 2-byte samples, but to reduce branches and
    // random steps we unroll to four rounds, so each loop consumes 16
    // samples.
    const int imax = img->frameInfo.nWidth & ~15;
    int i;
    for (i = 0; i < imax; i += 16) {
      // Run a round of the RNG.
      lcg_val = vmla_u32(LCG_INCR, lcg_val, LCG_MULT);

      // The lower two bits of this LCG parameterization are garbage,
      // leaving streaks on the image. We access the upper bits of each
      // 16-bit lane by shifting. (We use this both as an 8- and 16-bit
      // vector, so the choice of which one to keep it as is arbitrary.)
      uint8x8_t randvec =
          vreinterpret_u8_u16(vshr_n_u16(vreinterpret_u16_u32(lcg_val), 8));

      // We retrieve the values and shift them so that the bits we'll
      // shift out (after biasing) are in the upper 8 bits of each 16-bit
      // lane.
      uint16x4_t values = vshl_n_u16(vld1_u16(src), 6);
      src += 4;

      // We add the bias bits in the lower 8 to the shifted values to get
      // the final values in the upper 8 bits.
      uint16x4_t added1 = vqadd_u16(values, vreinterpret_u16_u8(randvec));

      // Shifting the randvec bits left by 2 bits, as an 8-bit vector,
      // should leave us with enough bias to get the needed rounding
      // operation.
      randvec = vshl_n_u8(randvec, 2);

      // Retrieve and sum the next 4 pixels.
      values = vshl_n_u16(vld1_u16(src), 6);
      src += 4;
      uint16x4_t added2 = vqadd_u16(values, vreinterpret_u16_u8(randvec));

      // Reinterpret the two added vectors as 8x8, zip them together, and
      // discard the lower portions.
      uint8x8_t zipped =
          vuzp_u8(vreinterpret_u8_u16(added1), vreinterpret_u8_u16(added2))
              .val[1];
      vst1_u8(dst, zipped);
      dst += 8;

      // Run it again with the next two rounds using the remaining
      // entropy in randvec.
      randvec = vshl_n_u8(randvec, 2);
      values = vshl_n_u16(vld1_u16(src), 6);
      src += 4;
      added1 = vqadd_u16(values, vreinterpret_u16_u8(randvec));
      randvec = vshl_n_u8(randvec, 2);
      values = vshl_n_u16(vld1_u16(src), 6);
      src += 4;
      added2 = vqadd_u16(values, vreinterpret_u16_u8(randvec));
      zipped = vuzp_u8(vreinterpret_u8_u16(added1), vreinterpret_u8_u16(added2))
                   .val[1];
      vst1_u8(dst, zipped);
      dst += 8;
    }

    uint32_t randval = 0;
    // For the remaining pixels in each row - usually none, as most
    // standard sizes are divisible by 32 - convert them "by hand".
    while (i < img->frameInfo.nWidth) {
      if (!randval) randval = random();
      dstBase[i] = (srcBase[i] + (randval & 3)) >> 2;
      i++;
      randval >>= 2;
    }

    srcBase += srcStride;
    dstBase += dstStride;
  }

  const uint16_t* srcUBase =
      reinterpret_cast<uint16_t*>(img->pvY);
  const uint16_t* srcVBase =
      reinterpret_cast<uint16_t*>(img->pvV);
  const int32_t uvWidth = (img->frameInfo.nWidth + 1) / 2;
  uint8_t* dstUBase = reinterpret_cast<uint8_t*>(data + yLength);
  uint8_t* dstVBase = reinterpret_cast<uint8_t*>(data + yLength + uvLength);
  const int srcUVStride = img->frameInfo.nVPitch / 2;
  const int dstUVStride = img->frameInfo.nVPitch;

  for (int y = 0; y < uvHeight; y++) {
    const uint16_t* srcU = srcUBase;
    const uint16_t* srcV = srcVBase;
    uint8_t* dstU = dstUBase;
    uint8_t* dstV = dstVBase;

    // As before, each i++ consumes 4 samples (8 bytes). For simplicity we
    // don't unroll these loops more than we have to, which is 8 samples.
    const int imax = uvWidth & ~7;
    int i;
    for (i = 0; i < imax; i += 8) {
      lcg_val = vmla_u32(LCG_INCR, lcg_val, LCG_MULT);
      uint8x8_t randvec =
          vreinterpret_u8_u16(vshr_n_u16(vreinterpret_u16_u32(lcg_val), 8));
      uint16x4_t uVal1 = vqadd_u16(vshl_n_u16(vld1_u16(srcU), 6),
                                   vreinterpret_u16_u8(randvec));
      srcU += 4;
      randvec = vshl_n_u8(randvec, 2);
      uint16x4_t vVal1 = vqadd_u16(vshl_n_u16(vld1_u16(srcV), 6),
                                   vreinterpret_u16_u8(randvec));
      srcV += 4;
      randvec = vshl_n_u8(randvec, 2);
      uint16x4_t uVal2 = vqadd_u16(vshl_n_u16(vld1_u16(srcU), 6),
                                   vreinterpret_u16_u8(randvec));
      srcU += 4;
      randvec = vshl_n_u8(randvec, 2);
      uint16x4_t vVal2 = vqadd_u16(vshl_n_u16(vld1_u16(srcV), 6),
                                   vreinterpret_u16_u8(randvec));
      srcV += 4;
      vst1_u8(dstU,
              vuzp_u8(vreinterpret_u8_u16(uVal1), vreinterpret_u8_u16(uVal2))
                  .val[1]);
      dstU += 8;
      vst1_u8(dstV,
              vuzp_u8(vreinterpret_u8_u16(vVal1), vreinterpret_u8_u16(vVal2))
                  .val[1]);
      dstV += 8;
    }

    uint32_t randval = 0;
    while (i < uvWidth) {
      if (!randval) randval = random();
      dstUBase[i] = (srcUBase[i] + (randval & 3)) >> 2;
      randval >>= 2;
      dstVBase[i] = (srcVBase[i] + (randval & 3)) >> 2;
      randval >>= 2;
      i++;
    }

    srcUBase += srcUVStride;
    srcVBase += srcUVStride;
    dstUBase += dstUVStride;
    dstVBase += dstUVStride;
  }

  return 1;
}
#endif

static void convert_16_to_8_standard(const OpenHevc_Frame* const img,
                                     jbyte* const data, const int32_t uvHeight,
                                     const int32_t yLength,
                                     const int32_t uvLength) {
    // Y
    int sampleY = 0;
    for (int y = 0; y < img->frameInfo.nHeight; y++) {
        const uint16_t* srcBase = reinterpret_cast<uint16_t*>(
                img->pvY + img->frameInfo.nYPitch * y);
        int8_t* destBase = data + img->frameInfo.nYPitch * y;
        for (int x = 0; x < img->frameInfo.nWidth; x++) {
            // Lightweight dither. Carryover the remainder of each 10->8 bit
            // conversion to the next pixel.
            sampleY += *srcBase++;
            *destBase++ = sampleY >> 2;
            sampleY = sampleY & 3;  // Remainder.
        }
    }
    // UV
    int sampleU = 0;
    int sampleV = 0;
    const int32_t uvWidth = (img->frameInfo.nWidth + 1) / 2;
    for (int y = 0; y < uvHeight; y++) {
        const uint16_t* srcUBase = reinterpret_cast<uint16_t*>(
                img->pvU + img->frameInfo.nUPitch * y);
        const uint16_t* srcVBase = reinterpret_cast<uint16_t*>(
                img->pvV + img->frameInfo.nVPitch * y);
        int8_t* destUBase = data + yLength + img->frameInfo.nUPitch * y;
        int8_t* destVBase =
                data + yLength + uvLength + img->frameInfo.nVPitch * y;
        for (int x = 0; x < uvWidth; x++) {
            // Lightweight dither. Carryover the remainder of each 10->8 bit
            // conversion to the next pixel.
            sampleU += *srcUBase++;
            *destUBase++ = sampleU >> 2;
            sampleU = sampleU & 3;  // Remainder.
            sampleV += *srcVBase++;
            *destVBase++ = sampleV >> 2;
            sampleV = sampleV & 3;  // Remainder.
        }
    }
}

static const int FLAG_DECODE_ONLY = 1;
static const int DECODE_NO_ERROR = 0;
static const int DECODE_ERROR = -1;
static const int DECODE_DRM_ERROR = -2;
static const int DECODE_GET_FRAME_ERROR = -3;//把AVFrame的数据拷贝到OutputBuffer出错

// JNI references for HevcOutputBuffer class.
static jmethodID initForRgbFrame;
static jmethodID initForYuvFrame;
static jfieldID dataField;
static jfieldID timeUsField;

DECODER_FUNC(jlong, hevcInit, jobject extraData, jint len) {
    OpenHevc_Handle ohevc = nullptr;

    int mode = 1;
    int nb_pthreads = (android_getCpuCount() + 1) / 2;
    int layer_id = 0;

    uint8_t * data = nullptr;

    ohevc = libOpenHevcInit(nb_pthreads, mode);

    memset(&info, 0, sizeof(Info) );

    uint8_t arm_neon = 0;
#ifdef __ARM_NEON__
    arm_neon = 1;
#endif
    ALOGI("[%s] libOpenHevcInit thd=%d neon=%d", __func__, nb_pthreads, arm_neon);

    if (NULL == ohevc) {
        ALOGE("[%s] libOpenHevcInit err", __func__);
        goto LABEL_RETURN;
    }

    libOpenHevcSetCheckMD5(ohevc, 0);
    libOpenHevcSetDebugMode(ohevc, 0);

    data = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(extraData));
    libOpenHevcCopyExtraData(ohevc, data, len);

    if (libOpenHevcStartDecoder(ohevc) < 0) {
        ALOGE("[%s] libOpenHevcStartDecoder err", __func__);
        goto LABEL_RETURN;
    }
    libOpenHevcSetTemporalLayer_id(ohevc, temporal_layer_id);
    libOpenHevcSetActiveDecoders(ohevc, quality_layer_id);
    libOpenHevcSetViewLayers(ohevc, quality_layer_id);
    {
        // Populate JNI References.
        const jclass outputBufferClass = env->FindClass(
                "com/google/android/exoplayer2/ext/hevc/HevcOutputBuffer");
        initForYuvFrame = env->GetMethodID(outputBufferClass, "initForYuvFrame",
                                           "(IIIII)Z");
        initForRgbFrame = env->GetMethodID(outputBufferClass, "initForRgbFrame",
                                           "(III)I");
        dataField = env->GetFieldID(outputBufferClass, "data",
                                    "Ljava/nio/ByteBuffer;");
        timeUsField = env->GetFieldID(outputBufferClass, "timeUs", "J");
    }

LABEL_RETURN:
    return (jlong) ohevc;
}

DECODER_FUNC(jlong, hevcClose, jlong jHandle) {
    OpenHevc_Handle ohevc = (OpenHevc_Handle) jHandle;
    libOpenHevcClose(ohevc);
    ALOGI("total frame decoded %d", info.NbFrame);
    return 0;
}

int getRGBFrame(JNIEnv* env, jobject jOutputBuffer, OpenHevc_Frame& hevcFrame) {
    if (YUV420 == hevcFrame.frameInfo.chromat_format) {

        jint bufferSize = env->CallIntMethod(jOutputBuffer, initForRgbFrame,
                                                     hevcFrame.frameInfo.nWidth,
                                                     hevcFrame.frameInfo.nHeight,
                                                     OutputPixFmt::PIXFMT_RGB565);
        if (env->ExceptionCheck() || bufferSize < 0) {
            ALOGE("ERROR: %s rgbmode failed, bufferSize %d", __func__, bufferSize);
            return DECODE_GET_FRAME_ERROR;
        }

        // get pointer to the data buffer.
        const jobject dataObject = env->GetObjectField(jOutputBuffer, dataField);
        uint8_t *const dst = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(dataObject));

        libyuv::I420ToRGB565(hevcFrame.pvY, hevcFrame.frameInfo.nYPitch,
                             hevcFrame.pvU, hevcFrame.frameInfo.nUPitch,
                             hevcFrame.pvV, hevcFrame.frameInfo.nVPitch,
                             dst, hevcFrame.frameInfo.nWidth * 2,
                             hevcFrame.frameInfo.nWidth, hevcFrame.frameInfo.nHeight);
    } else if(YUV422 == hevcFrame.frameInfo.chromat_format) {

        jint bufferSize = env->CallIntMethod(jOutputBuffer, initForRgbFrame,
                                                 hevcFrame.frameInfo.nWidth,
                                                 hevcFrame.frameInfo.nHeight,
                                                 OutputPixFmt::PIXFMT_RGB565);
        if (env->ExceptionCheck() || bufferSize < 0) {
            ALOGE("ERROR: %s rgbmode failed, bufferSize %d", __func__, bufferSize);
            return DECODE_GET_FRAME_ERROR;
        }

        // get pointer to the data buffer.
        const jobject dataObject = env->GetObjectField(jOutputBuffer, dataField);
        uint8_t *const dst = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(dataObject));

        libyuv::I422ToRGB565(hevcFrame.pvY, hevcFrame.frameInfo.nYPitch,
                             hevcFrame.pvU, hevcFrame.frameInfo.nUPitch,
                             hevcFrame.pvV, hevcFrame.frameInfo.nVPitch,
                             dst, hevcFrame.frameInfo.nWidth * 2,
                             hevcFrame.frameInfo.nWidth, hevcFrame.frameInfo.nHeight);
    } else if(YUV444 == hevcFrame.frameInfo.chromat_format) {

        jint bufferSize = env->CallIntMethod(jOutputBuffer, initForRgbFrame,
                                                 hevcFrame.frameInfo.nWidth,
                                                 hevcFrame.frameInfo.nHeight,
                                                 OutputPixFmt::PIXFMT_ARGB8888);
        if (env->ExceptionCheck() || bufferSize < 0) {
            ALOGE("ERROR: %s rgbmode failed, bufferSize %d", __func__, bufferSize);
            return DECODE_GET_FRAME_ERROR;
        }

        // get pointer to the data buffer.
        const jobject dataObject = env->GetObjectField(jOutputBuffer, dataField);
        uint8_t *const dst = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(dataObject));

        /**
         * Bitmap.Config.ARGB8888像素的布局从低位到高位是：R G B A
         * 而I444ToARGB像素的内存布局从低位到高位是：B G R A
         * 所以要把libyuv转换过后的每个像素的第0个字节和第2个字节交换才能显示在Bitmap上
         * 这里是可以优化的，如SIMD，或者从libyuv源码上改成直接输出RGBA，等真有需求的时候再改
         */
        libyuv::I444ToARGB(hevcFrame.pvY, hevcFrame.frameInfo.nYPitch,
                           hevcFrame.pvU, hevcFrame.frameInfo.nUPitch,
                           hevcFrame.pvV, hevcFrame.frameInfo.nVPitch,
                           dst, hevcFrame.frameInfo.nWidth * 4,
                           hevcFrame.frameInfo.nWidth, hevcFrame.frameInfo.nHeight);
        for(int i = 0; i < bufferSize; i+=4) {
            uint8_t tmp = dst[i+2];
            dst[i+2] = dst[i+0];
            dst[i+0] = tmp;
        }
    } else {
        ALOGE("ERROR: %s rgbmode failed, unrecognized chroma format %d", __func__, hevcFrame.frameInfo.chromat_format);
        return DECODE_GET_FRAME_ERROR;
    }

    return DECODE_NO_ERROR;
}

int getYUVFrame(JNIEnv* env, jobject jOutputBuffer, OpenHevc_Frame& hevcFrame) {
    // yuv
    const int kColorspaceUnknown = 0;
    const int kColorspaceBT601 = 1;
    const int kColorspaceBT709 = 2;
    const int kColorspaceBT2020 = 3;

    int colorspace = kColorspaceUnknown;
    switch (hevcFrame.frameInfo.colorspace) {
        case AVCOL_SPC_BT470BG:
        case AVCOL_SPC_SMPTE170M:
            colorspace = kColorspaceBT601;
            break;
        case AVCOL_SPC_BT709:
            colorspace = kColorspaceBT709;
            break;
        case AVCOL_SPC_BT2020_NCL:
        case AVCOL_SPC_BT2020_CL:
            colorspace = kColorspaceBT2020;
            break;
        default:
            break;
    }

    // resize buffer if required.
    jboolean initResult = env->CallBooleanMethod(
            jOutputBuffer, initForYuvFrame, hevcFrame.frameInfo.nWidth, hevcFrame.frameInfo.nHeight,
            hevcFrame.frameInfo.nYPitch, hevcFrame.frameInfo.nUPitch, colorspace);
    if (env->ExceptionCheck() || !initResult) {
        ALOGE("ERROR: %s yuvmode failed", __func__);
        return DECODE_GET_FRAME_ERROR;
    }
    // get pointer to the data buffer.
    const jobject dataObject = env->GetObjectField(jOutputBuffer, dataField);
    jbyte* const data =
            reinterpret_cast<jbyte*>(env->GetDirectBufferAddress(dataObject));

    const int32_t uvHeight = (hevcFrame.frameInfo.nHeight + 1) / 2;
    const uint64_t yLength = hevcFrame.frameInfo.nYPitch * hevcFrame.frameInfo.nHeight;
    const uint64_t uvLength = hevcFrame.frameInfo.nUPitch * uvHeight;

    if (hevcFrame.frameInfo.chromat_format == YUV420) {  // HBD planar 420.
        // Note: The stride for BT2020 is twice of what we use so this is wasting
        // memory. The long term goal however is to upload half-float/short so
        // it's not important to optimize the stride at this time.
        int converted = 0;
#ifdef __ARM_NEON__
        converted = convert_16_to_8_neon(&hevcFrame, data, uvHeight, yLength, uvLength);
#endif  // __ARM_NEON__
        if (!converted) {
            convert_16_to_8_standard(&hevcFrame, data, uvHeight, yLength, uvLength);
        }
    } else {
        // todo to support other formats
        ALOGW("WARN: %s unrecognized pixfmt %d", __func__ ,hevcFrame.frameInfo.chromat_format);
//        memcpy(data, hevcFrame.pvY, yLength);
//        memcpy(data + yLength, hevcFrame.pvU, uvLength);
//        memcpy(data + yLength + uvLength, hevcFrame.pvV, uvLength);
        return DECODE_GET_FRAME_ERROR;
    }

    return DECODE_NO_ERROR;
}

void saveFrame(JNIEnv* env, jstring jStr, OpenHevc_Frame& hevcFrame) {
    FILE* outFile = nullptr;
    const char* path = env->GetStringUTFChars(jStr, nullptr);
    struct stat st;
    int res = stat(path, &st);
    if(0 == res && (st.st_mode & S_IFDIR)) {
        char outFilePath[512];
        sprintf(outFilePath, "%s/video_%dx%d.yuv", path, pic_width, pic_height);
        outFile = fopen(outFilePath, "ab+");
        ALOGV("save yuv to %s", outFilePath);

        // write Y
        int format = hevcFrame.frameInfo.chromat_format == YUV420 ? 1 : 0;
        for (int h = 0; h < hevcFrame.frameInfo.nHeight; h++) {
            const uint8_t* srcBase1 = hevcFrame.pvY + hevcFrame.frameInfo.nYPitch * h;
            fwrite(srcBase1, sizeof(uint8_t), hevcFrame.frameInfo.nWidth, outFile);
        }
        // write UV
        int uvLineSize = hevcFrame.frameInfo.nWidth >> format;
        for (int h = 0; h < hevcFrame.frameInfo.nHeight / 2; h++) {
            const uint8_t* srcBase2 = hevcFrame.pvU + hevcFrame.frameInfo.nUPitch * h;
            fwrite(srcBase2, sizeof(uint8_t) , uvLineSize, outFile);
        }
        for (int h = 0; h < hevcFrame.frameInfo.nHeight / 2 ; h++) {
            const uint8_t* srcBase3 = hevcFrame.pvV + hevcFrame.frameInfo.nVPitch * h;
            fwrite(srcBase3, sizeof(uint8_t) , uvLineSize, outFile);
        }
        fflush(outFile);
        fclose(outFile);
    } else {
        ALOGW("WARN: %s is not a valid dir", path);
    }
}

DECODER_FUNC(jint, hevcDecode, jlong jHandle, jobject encoded, jint len, int64_t pts,
             jobject jOutputBuffer, jint outputMode, jint flush, jstring jStr) {
    OpenHevc_Handle ohevc = (OpenHevc_Handle) jHandle;
    const uint8_t *const buffer =
            reinterpret_cast<const uint8_t *>(env->GetDirectBufferAddress(encoded));
    int got_pic;
    OpenHevc_Frame hevcFrame;

    //Zero if no frame could be decompressed, otherwise, it is nonzero.
    //-1 indicates error occurred
    got_pic = libOpenHevcDecode(ohevc, buffer, len, pts, flush);
    ALOGD("DEBUG: %d frame decoded got_pic %d ", info.NbFrame, got_pic);
    info.NbFrame++;
    if (got_pic < 0) {
        ALOGE("ERROR: hevcDecode failed, status= %d", got_pic);
        return DECODE_ERROR;
    }

    if(got_pic == 0) {
        ALOGD("[%s] decode only frame", __func__);
        return FLAG_DECODE_ONLY;
    }

    memset(&hevcFrame, 0, sizeof(OpenHevc_Frame));
    //libOpenHevcGetPictureInfo(ohevc, &hevcFrame.frameInfo);
    got_pic = libOpenHevcGetOutput(ohevc, 1, &hevcFrame);
    if(got_pic < 0) {
        ALOGE("ERROR: %s failed getoutput", __func__);
        return DECODE_GET_FRAME_ERROR; // indicate error
    }

    //设置OutputBuffer的pts
    env->SetLongField(jOutputBuffer, timeUsField, hevcFrame.frameInfo.pts);

    switch(outputMode) {
        case OutputMode::RGB:
            got_pic = getRGBFrame(env, jOutputBuffer, hevcFrame);
            break;
        case OutputMode::YUV:
            got_pic = getYUVFrame(env, jOutputBuffer, hevcFrame);
            break;
        default:
            ALOGE("ERROR: %s failed, outputMode %d not supported", __func__, outputMode);
            return DECODE_GET_FRAME_ERROR;
    }

    //for debug, to save frame to file
    if(jStr != nullptr) {
       saveFrame(env, jStr, hevcFrame);
    }
    return got_pic;
}
