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

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

/**
 * A GLSurfaceView extension that scales itself to the given aspect ratio.
 */
public class HevcVideoSurfaceView extends GLSurfaceView implements HevcOutputBufferRenderer {

  private final HevcRenderer renderer;

  public HevcVideoSurfaceView(Context context) {
    this(context, null);
  }

  public HevcVideoSurfaceView(Context context, AttributeSet attrs) {
    super(context, attrs);
    renderer = new HevcRenderer();
    setPreserveEGLContextOnPause(true);
    setEGLContextClientVersion(2);
    setRenderer(renderer);
    setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
  }

  @Override
  public void setOutputBuffer(HevcOutputBuffer outputBuffer) {
    renderer.setFrame(outputBuffer);
    requestRender();
  }

}
