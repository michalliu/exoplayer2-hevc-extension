# exoplayer2-extension-hevc
Soft Decode HEVC/H265 video using OpenHEVC

# Known issue
1. ~~playable but not smooth~~ (STATUS: DONE. due to  pts not correct set)
1. ~~video renderring is not correct sometimes when seeking is performed)~~ (STATUS: DONE. due to needs flush decoder when seek performed)
1. potential frame drop at play ends (STATUS: WIP)
1. seek in video is very slow (STATUS: WIP)
1. more yuv format support, only YUV420 is handled currently

# Thanks
[The OpenHEVC Project](https://github.com/OpenHEVC/openHEVC)
