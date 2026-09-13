# -*- cmake -*-

project(llaudio)

include(00-Common)
include(OPENAL)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLVFS)

include_directories(
    ${LLAUDIO_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${OGG_INCLUDE_DIRS}
    ${VORBISENC_INCLUDE_DIRS}
    ${VORBISFILE_INCLUDE_DIRS}
    ${VORBIS_INCLUDE_DIRS}
    )

set(llaudio_SOURCE_FILES
    llaudiodecodemgr.cpp
    llaudioengine.cpp
    lllistener.cpp
    llvorbisdecode.cpp
    llvorbisencode.cpp
    )

set(llaudio_HEADER_FILES
    CMakeLists.txt

    llaudiodecodemgr.h
    llaudioengine.h
    lllistener.h
    llvorbisdecode.h
    llvorbisencode.h
    llwindgen.h
    )

if (TARGET ll::openal)
  list(APPEND llaudio_SOURCE_FILES
    llaudioengine_openal.cpp
    lllistener_openal.cpp
    )

  list(APPEND llaudio_HEADER_FILES
    llaudioengine_openal.h
    lllistener_openal.h
    )
endif ()

set_source_files_properties(${llaudio_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llaudio_SOURCE_FILES ${llaudio_HEADER_FILES})

add_library (llaudio ${llaudio_SOURCE_FILES})
target_include_directories(llaudio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    llaudio
    PUBLIC
    llcommon
    )

if( TARGET ll::openal )
  target_link_libraries( llaudio PUBLIC ll::openal )
endif()
