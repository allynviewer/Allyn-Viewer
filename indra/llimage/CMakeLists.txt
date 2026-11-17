# -*- cmake -*-

project(llimage)

include(00-Common)
include(LLAddBuildTest)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLVFS)
include(JPEG)
include(PNG)
include(ZLIB)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    )

set(llimage_SOURCE_FILES
    bcdec/bcdec.c
    llimage.cpp
    llimagebc7.cpp
    llimagebmp.cpp
    llimagedxt.cpp
    llimagej2c.cpp
    llimagejpeg.cpp
    llimagepng.cpp
    llimagetga.cpp
    llimageworker.cpp
    llpngwrapper.cpp
    )

set(llimage_HEADER_FILES
    CMakeLists.txt

    bcdec/bcdec.h
    llimage.h
    llimagebc7.h
    llimagebmp.h
    llimagedxt.h
    llimagej2c.h
    llimagejpeg.h
    llimagepng.h
    llimagetga.h
    llimageworker.h
    llmapimagetype.h
    llpngwrapper.h
    )

set_source_files_properties(${llimage_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

if (MSVC)
  set_source_files_properties(bcdec/bcdec.c
                              PROPERTIES COMPILE_FLAGS "/W0 /WX-")
endif (MSVC)

list(APPEND llimage_SOURCE_FILES ${llimage_HEADER_FILES})

add_library (llimage ${llimage_SOURCE_FILES})

target_link_libraries(
    llimage
    PUBLIC
    llcommon
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ispc_texcomp/kernelx86_64-pc-windows-msvc.lib
    )

if (LL_TESTS)
	# Add tests
	ADD_BUILD_TEST(llimageworker llimage)
endif (LL_TESTS)

