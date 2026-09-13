# -*- cmake -*-
include(Prebuilt)

set(PNG_FIND_QUIETLY ON)
set(PNG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindPNG)
else (STANDALONE)
  use_prebuilt_binary(libpng)
  set(PNG_LIBRARIES libpng16)
  set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/libpng16)
endif (STANDALONE)
