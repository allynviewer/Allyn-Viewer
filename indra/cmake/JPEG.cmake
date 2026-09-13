# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()
add_library(ll::libjpeg INTERFACE IMPORTED)

use_prebuilt_binary(libjpeg-turbo)

find_library(JPEG_LIBRARY
  NAMES
    jpeg.lib
    jpeglib
    libjpeg.a
  PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::libjpeg INTERFACE ${JPEG_LIBRARY})
target_include_directories(ll::libjpeg SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)

set(JPEG_LIBRARIES ${JPEG_LIBRARY})
set(JPEG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
