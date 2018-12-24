# exoplayer2-extension-hevc
Soft Decode HEVC/H265 video using OpenHEVC

# Known issue
1. ~~playable but not smooth~~ (STATUS: DONE. due to  pts not correct set)
1. ~~video renderring is not correct sometimes when seeking is performed)~~ (STATUS: DONE. due to needs flush decoder when seek performed)
1. ~~potential frame drop at play ends (STATUS: DONE. Fixed)~~
1. ~~seek in video is very slow (STATUS: DONE. due to the default extract seek setting parameter in exoplayer)~~
1. ~~more yuv format support, only YUV420 is handled currently~~(STATUS: DONE. yuv422 and yuv 444 is supported now)~~
1. ~~jni performance improvements (STATUS: DONE. decode and render frame now in a single jni call, performance improved 4x)~~
1. ~~native drawing using ANativeWindow improve data copy and yuv rgb conversion performance (STATUS: WIP.)~~
1. drm support(STATUS: WAIT.)

# Thanks
[The OpenHEVC Project](https://github.com/OpenHEVC/openHEVC)
