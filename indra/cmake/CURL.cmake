# -*- cmake -*-
include(Prebuilt)

set(CURL_FIND_QUIETLY OFF)
set(CURL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindCURL)
else (STANDALONE)
  use_prebuilt_binary(curl)
  if (WINDOWS)
    use_prebuilt_binary(nghttp2)
    set(CURL_LIBRARIES
    debug libcurl
    optimized libcurl
    debug nghttp2
    optimized nghttp2)
  else (WINDOWS)
    use_prebuilt_binary(libidn)
    set(CURL_LIBRARIES curl idn)
  endif (WINDOWS)
  set(CURL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
