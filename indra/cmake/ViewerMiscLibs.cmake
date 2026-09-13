# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  use_prebuilt_binary(webrtc)
else (NOT STANDALONE)
  # Download webrtc even when using standalone.
  set(STANDALONE OFF)
  use_prebuilt_binary(webrtc)
  set(STANDALONE ON)
endif(NOT STANDALONE)

use_prebuilt_binary(viewer-fonts)
