# -*- cmake -*-
#
# Compared to other libraries, compiling this one is a mess.  The
# reason is that we have several source files that have two different
# sets of behaviour, depending on whether they're intended to be part
# of the viewer or the map server.
#
# Unfortunately, the affected code is a rat's nest of #ifdefs, so it's
# easier to play compilation tricks than to actually fix the problem.

project(llwindow)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLRender)
include(LLVFS)
include(LLWindow)
include(LLXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${DIRECTX_INCLUDE_DIR}
    )

set(llwindow_SOURCE_FILES
    llkeyboard.cpp
    llkeyboardheadless.cpp
    llwindow.cpp
    llwindowcallbacks.cpp
    llwindowheadless.cpp
    )

set(llwindow_HEADER_FILES
    CMakeLists.txt

    llkeyboard.h
    llkeyboardheadless.h
    llwindowcallbacks.h
    llwindowheadless.h
    )

set(viewer_SOURCE_FILES
    llmousehandler.cpp
    )

set(viewer_HEADER_FILES
    llmousehandler.h
    llpreeditor.h
    llwindow.h
    )

if (WINDOWS)
  list(APPEND llwindow_SOURCE_FILES
       lldragdropwin32.cpp
       lldxhardware.cpp
       llkeyboardwin32.cpp
       llwindowwin32.cpp
       )
  list(APPEND llwindow_HEADER_FILES
       lldragdropwin32.h
       lldxhardware.h
       llkeyboardwin32.h
       llwindowwin32.h
       )
  list(APPEND llwindow_LINK_LIBRARIES
       comdlg32     # Common Dialogs for ChooseColor
       ole32
       )
endif (WINDOWS)

if (SOLARIS)
  list(APPEND llwindow_SOURCE_FILES
       llwindowsolaris.cpp
       )
  list(APPEND llwindow_HEADER_FILES
       llwindowsolaris.h
       )
endif (SOLARIS)

set_source_files_properties(${llwindow_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

if (llwindow_HEADER_FILES)
  list(APPEND llwindow_SOURCE_FILES ${llwindow_HEADER_FILES})
endif (llwindow_HEADER_FILES)
  list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

  add_library (llwindow
    ${llwindow_SOURCE_FILES}
    ${viewer_SOURCE_FILES}
    )

target_link_libraries (llwindow PUBLIC llcommon ${llwindow_LINK_LIBRARIES})
