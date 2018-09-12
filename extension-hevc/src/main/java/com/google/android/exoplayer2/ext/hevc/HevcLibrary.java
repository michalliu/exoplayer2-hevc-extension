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

import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;

import com.google.android.exoplayer2.ExoPlayerLibraryInfo;
import com.google.android.exoplayer2.util.LibraryLoader;

import java.io.File;

/**
 * Configures and queries the underlying native library.
 */
public final class HevcLibrary {

  static {
    ExoPlayerLibraryInfo.registerModule("oskHvc");
  }

  private static final LibraryLoader LOADER = new LibraryLoader("hevcdec");

  private HevcLibrary() {}

  /**
   * Override the names of the Vpx native libraries. If an application wishes to call this method,
   * it must do so before calling any other method defined by this class, and before instantiating a
   * {@link LibHevcVideoRenderer} instance.
   *
   * @param libraries The names of the Vpx native libraries.
   */
  public static void setLibraries(String... libraries) {
    LOADER.setLibraries(libraries);
  }

  /**
   * Returns whether the underlying library is available, loading it if necessary.
   */
  public static boolean isAvailable() {
    return LOADER.isAvailable();
  }

  /**
   * Returns the version of the underlying library if available, or null otherwise.
   */
  public static String getVersion() {
    return isAvailable() ? hevcGetVersion() : null;
  }

  /**
   * Returns the configuration string with which the underlying library was built if available, or
   * null otherwise.
   */
  public static String getBuildConfig() {
    return isAvailable() ? hevcGetBuildConfig() : null;
  }

  private static String ensureDir(File dir) {
    boolean result = false;
    if (dir.exists() && dir.isFile()) {
      result = dir.delete();
    }
    if(!dir.exists()) {
      result = dir.mkdirs();
    }
    if (dir.isDirectory() && dir.exists()) {
      result = true;
    }
    if (result) {
      return dir.getAbsolutePath();
    }
    return null;
  }

  public static String ensureFilesDir(String requestDir) {
    String resultPath = "";
    if (!TextUtils.isEmpty(requestDir)) {
      File filesRootDir = null;
      try {
        filesRootDir = Environment.getExternalStorageDirectory();
      } catch (Exception ex) {
        ex.printStackTrace();
      }
      if (filesRootDir != null) {
        String fileDir = filesRootDir + File.separator + requestDir;
        File dstDir = new File(fileDir);
        resultPath = ensureDir(dstDir);
      }

      if (TextUtils.isEmpty(resultPath)) {
        filesRootDir = null;
        try {
          filesRootDir = Environment.getDataDirectory();
        } catch (Exception ex) {
          ex.printStackTrace();
        }
        if (filesRootDir != null) {
          String fileDir = filesRootDir + File.separator + requestDir;
          File dstDir = new File(fileDir);
          resultPath = ensureDir(dstDir);
        }
      }
    }
    return resultPath;
  }

  private static native String hevcGetVersion();
  private static native String hevcGetBuildConfig();
  public static native boolean hevcIsSecureDecodeSupported();

}
