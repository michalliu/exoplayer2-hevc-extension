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

import android.util.Log;

import com.google.android.exoplayer2.C;
import com.google.android.exoplayer2.Format;
import com.google.android.exoplayer2.decoder.CryptoInfo;
import com.google.android.exoplayer2.decoder.OutputBuffer;
import com.google.android.exoplayer2.decoder.SimpleDecoder;
import com.google.android.exoplayer2.drm.DecryptionException;
import com.google.android.exoplayer2.drm.ExoMediaCrypto;

import java.nio.ByteBuffer;
import java.util.List;

/**
 * Hevc decoder.
 */
/* package */ final class HevcDecoder extends
        SimpleDecoder<HevcInputBuffer, HevcOutputBuffer, HevcDecoderException> {

  public static final int OUTPUT_MODE_NONE = -1;
  public static final int OUTPUT_MODE_YUV = 0;
  public static final int OUTPUT_MODE_RGB = 1;

  // DO NOT CHANGE THESE CONSTANTS WITHOUT CHANGE IT'S NATIVE COUNTERPART
  public static final int  DECODE_ONLY = 1;
  private static final int NO_ERROR = 0;
  private static final int DECODE_ERROR = -1;
  private static final int DRM_ERROR = -2;
  public static final int GET_FRAME_ERROR = -3;

  private final ExoMediaCrypto exoMediaCrypto;
  private final long hvcDecContext;

  private volatile int outputMode;

  /**
   * Creates a HEVC decoder.
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
      throw new HevcDecoderException("Hevc decoder does not support secure decode.");
    }

    ByteBuffer buffer = wrapBytes(format.initializationData);
    hvcDecContext = hevcInit(buffer, buffer.capacity());

    if (hvcDecContext == 0) {
      throw new HevcDecoderException("Failed to initialize decoder");
    }
    setInitialInputBufferSize(initialInputBufferSize);
  }

  private ByteBuffer wrapBytes(List<byte[]> bytesList) {
    int size = 0;
    for (byte[] it : bytesList) {
      size += it.length;
    }
    ByteBuffer buffer = ByteBuffer.allocateDirect(size);
    byte[] data = new byte[size];

    int i = 0;
    for (byte[] it : bytesList) {
      System.arraycopy(it, 0, data, i, it.length);
      i += it.length;
    }
    buffer.put(data);
    return buffer;
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
    if(inputBuffer.isEncrypted() && !HevcLibrary.hevcIsSecureDecodeSupported()) {
      return new HevcDecoderException(HevcLibrary.getVersion() + " does not support secure decode.");
    }

    ByteBuffer inputData = inputBuffer.data;
    int inputSize = inputData.limit();

    final long result = hevcDecode(hvcDecContext, inputData, inputSize, inputBuffer.timeUs,
            outputBuffer, outputMode, reset, null);

    if (result == DECODE_ONLY) {
      outputBuffer.addFlag(C.BUFFER_FLAG_DECODE_ONLY);
    } else if (result != NO_ERROR) {
      return new HevcDecoderException("Decode error " + result);
    }

    outputBuffer.colorInfo = inputBuffer.colorInfo;
    outputBuffer.mode = outputMode;
    return null;
  }

  @Override
  public void release() {
    super.release();
    hevcClose(hvcDecContext);
  }

  private native long hevcInit(ByteBuffer buffer, int length);
  private native long hevcClose(long context);
  private native int hevcDecode(long context, ByteBuffer encoded, int length, long pts,
                                OutputBuffer outputBuffer, int outputMode, boolean flush, String outpath);
}
