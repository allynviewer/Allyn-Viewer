# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()

set(USE_OPENAL ON CACHE BOOL "Enable OpenAL")
if(OPENAL)
  message(WARNING "Use of the OPENAL argument is deprecated, please switch to USE_OPENAL")
  set(USE_OPENAL ${OPENAL})
endif()

if (USE_OPENAL)
  add_library(ll::openal INTERFACE IMPORTED)
  target_include_directories(ll::openal SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/AL")
  target_compile_definitions(ll::openal INTERFACE LL_OPENAL=1)
  use_prebuilt_binary(openal)

  find_library(OPENAL_LIBRARY
    NAMES
      OpenAL32
      openal
      libopenal.dylib
      libopenal.so
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

  find_library(ALUT_LIBRARY
    NAMES
      alut
      libalut.dylib
      libalut.so
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

  target_link_libraries(ll::openal INTERFACE ${OPENAL_LIBRARY} ${ALUT_LIBRARY})

  set(OPENAL_LIB_INCLUDE_DIRS "${LIBS_PREBUILT_DIR}/include/AL")
  set(OPENAL_LIBRARIES ${OPENAL_LIBRARY} ${ALUT_LIBRARY})
  message(STATUS "Building with OpenAL audio support")
endif ()
