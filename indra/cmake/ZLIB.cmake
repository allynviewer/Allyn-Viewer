# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()
add_library(ll::zlib-ng INTERFACE IMPORTED)

use_prebuilt_binary(zlib-ng)

find_library(ZLIBNG_LIBRARY
  NAMES
    zlib.lib
    z.lib
    libz.a
  PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::zlib-ng INTERFACE ${ZLIBNG_LIBRARY})

target_include_directories(ll::zlib-ng SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/zlib-ng)

# Legacy variables used across the codebase
set(ZLIB_LIBRARIES ${ZLIBNG_LIBRARY})
set(ZLIB_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/zlib-ng)
