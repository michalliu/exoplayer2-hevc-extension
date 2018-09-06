/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.exoplayer2.ext.hevc;

import com.google.android.exoplayer2.C;
import com.google.android.exoplayer2.Format;
import com.google.android.exoplayer2.decoder.CryptoInfo;
import com.google.android.exoplayer2.decoder.SimpleDecoder;
import com.google.android.exoplayer2.drm.DecryptionException;
import com.google.android.exoplayer2.drm.ExoMediaCrypto;

import java.nio.ByteBuffer;

/**
 * Vpx decoder.
 */
/* package */ final class HevcDecoder extends
        SimpleDecoder<HevcInputBuffer, HevcOutputBuffer, HevcDecoderException> {

  public static final int OUTPUT_MODE_NONE = -1;
  public static final int OUTPUT_MODE_YUV = 0;
  public static final int OUTPUT_MODE_RGB = 1;

  private static final int NO_ERROR = 0;
  private static final int DECODE_ERROR = 1;
  private static final int DRM_ERROR = 2;

  private final ExoMediaCrypto exoMediaCrypto;
  private final long vpxDecContext;

  private volatile int outputMode;

  /**
   * Creates a HEVC9 decoder.
   *
   * @param numInputBuffers The number of input buffers.
   * @param numOutputBuffers The number of output buffers.
   * @param initialInputBufferSize The initial size of each input buffer.
   * @param exoMediaCrypto The {@link ExoMediaCrypto} object required for decoding encrypted
   *     content. Maybe null and can be ignored if decoder does not handle encrypted content.
   * @throws HevcDecoderException Thrown if an exception occurs when initializing the decoder.
   */
  public HevcDecoder(int numInputBuffers, int numOutputBuffers, int initialInputBufferSize,
                     ExoMediaCrypto exoMediaCrypto, Format format) throws HevcDecoderException {
    super(new HevcInputBuffer[numInputBuffers], new HevcOutputBuffer[numOutputBuffers]);
    if (!HevcLibrary.isAvailable()) {
      throw new HevcDecoderException("Failed to load decoder native libraries.");
    }
    this.exoMediaCrypto = exoMediaCrypto;
    if (exoMediaCrypto != null && !HevcLibrary.hevcIsSecureDecodeSupported()) {
      throw new HevcDecoderException("Vpx decoder does not support secure decode.");
    }

    // copy extradata
    int l = 0;
    for (byte[] bb : format.initializationData) {
      l += bb.length;
    }
    byte[] bbs = new byte[l];
    int i=0;
    for (byte[] bb : format.initializationData) {
      System.arraycopy(bb, 0, bbs, i, bb.length);
    }

    ByteBuffer buffer = ByteBuffer.allocateDirect(bbs.length);
    buffer.put(bbs);
    vpxDecContext = hevcInit(buffer, bbs.length);

    if (vpxDecContext == 0) {
      throw new HevcDecoderException("Failed to initialize decoder");
    }
    setInitialInputBufferSize(initialInputBufferSize);
  }

  @Override
  public String getName() {
    return "libopenhevc" + HevcLibrary.getVersion();
  }

  /**
   * Sets the output mode for frames rendered by the decoder.
   *
   * @param outputMode The output mode. One of {@link #OUTPUT_MODE_NONE}, {@link #OUTPUT_MODE_RGB}
   *     and {@link #OUTPUT_MODE_YUV}.
   */
  public void setOutputMode(int outputMode) {
    this.outputMode = outputMode;
  }

  @Override
  protected HevcInputBuffer createInputBuffer() {
    return new HevcInputBuffer();
  }

  @Override
  protected HevcOutputBuffer createOutputBuffer() {
    return new HevcOutputBuffer(this);
  }

  @Override
  protected void releaseOutputBuffer(HevcOutputBuffer buffer) {
    super.releaseOutputBuffer(buffer);
  }

  @Override
  protected HevcDecoderException createUnexpectedDecodeException(Throwable error) {
    return new HevcDecoderException("Unexpected decode error", error);
  }

  @Override
  protected HevcDecoderException decode(HevcInputBuffer inputBuffer, HevcOutputBuffer outputBuffer,
                                        boolean reset) {
    ByteBuffer inputData = inputBuffer.data;
    int inputSize = inputData.limit();
    CryptoInfo cryptoInfo = inputBuffer.cryptoInfo;
    final long result = inputBuffer.isEncrypted()
        ? hevcSecureDecode(vpxDecContext, inputData, inputSize, exoMediaCrypto,
        cryptoInfo.mode, cryptoInfo.key, cryptoInfo.iv, cryptoInfo.numSubSamples,
        cryptoInfo.numBytesOfClearData, cryptoInfo.numBytesOfEncryptedData)
        : hevcDecode(vpxDecContext, inputData, inputSize);
    if (result != NO_ERROR) {
      if (result == DRM_ERROR) {
        String message = "Drm error: " + hevcGetErrorMessage(vpxDecContext);
        DecryptionException cause = new DecryptionException(
            hevcGetErrorCode(vpxDecContext), message);
        return new HevcDecoderException(message, cause);
      } else {
        return new HevcDecoderException("Decode error: " + hevcGetErrorMessage(vpxDecContext));
      }
    }

    if (!inputBuffer.isDecodeOnly()) {
      outputBuffer.init(inputBuffer.timeUs, outputMode);
      int getFrameResult = hevcGetFrame(vpxDecContext, outputBuffer);
      if (getFrameResult == 1) {
        outputBuffer.addFlag(C.BUFFER_FLAG_DECODE_ONLY);
      } else if (getFrameResult == -1) {
        return new HevcDecoderException("Buffer initialization failed.");
      }
      outputBuffer.colorInfo = inputBuffer.colorInfo;
    }
    return null;
  }

  @Override
  public void release() {
    super.release();
    hevcClose(vpxDecContext);
  }

  private native long hevcInit(ByteBuffer buffer, int length);
  private native long hevcClose(long context);
  private native int hevcDecode(long context, ByteBuffer encoded, int length);
  private native long hevcSecureDecode(long context, ByteBuffer encoded, int length,
                                       ExoMediaCrypto mediaCrypto, int inputMode, byte[] key, byte[] iv,
                                       int numSubSamples, int[] numBytesOfClearData, int[] numBytesOfEncryptedData);
  private native int hevcGetFrame(long context, HevcOutputBuffer outputBuffer);
  private native int hevcGetErrorCode(long context);
  private native String hevcGetErrorMessage(long context);

}
