# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(freetype)
set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/freetype2)
set(FREETYPE_LIBRARIES freetype)

link_directories(${FREETYPE_LIBRARY_DIRS})
