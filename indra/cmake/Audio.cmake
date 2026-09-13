# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(ogg_vorbis)
set(VORBIS_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
set(VORBISENC_INCLUDE_DIRS ${VORBIS_INCLUDE_DIRS})
set(VORBISFILE_INCLUDE_DIRS ${VORBIS_INCLUDE_DIRS})

if (WINDOWS)
  set(OGG_LIBRARIES
      optimized libogg
      debug libogg)
  set(VORBIS_LIBRARIES
      optimized libvorbis
      debug libvorbis)
  set(VORBISENC_LIBRARIES
      optimized libvorbisenc
      debug libvorbisenc)
  set(VORBISFILE_LIBRARIES
      optimized libvorbisfile
      debug libvorbisfile)
endif (WINDOWS)

link_directories(
    ${VORBIS_LIBRARY_DIRS}
    ${VORBISENC_LIBRARY_DIRS}
    ${VORBISFILE_LIBRARY_DIRS}
    ${OGG_LIBRARY_DIRS}
    )

set(LLAUDIO_VORBIS_LIBRARIES
    ${VORBISENC_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    )

