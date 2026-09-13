# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindNDOF)
  if(NOT NDOF_FOUND)
    message(STATUS "Building without N-DoF joystick support")
  endif(NOT NDOF_FOUND)
else (STANDALONE)
  use_prebuilt_binary(libndofdev)
  set(NDOF_LIBRARY libndofdev)
  set(NDOF_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
  set(NDOF_FOUND 1)
endif (STANDALONE)

if (NDOF_FOUND)
  add_definitions(-DLIB_NDOF=1)
  include_directories(${NDOF_INCLUDE_DIR})
else (NDOF_FOUND)
  message(STATUS "Building without N-DoF joystick support")
  set(NDOF_INCLUDE_DIR "")
  set(NDOF_LIBRARY "")
endif (NDOF_FOUND)

