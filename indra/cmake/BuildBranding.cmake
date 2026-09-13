# -*- cmake -*-
if (WINDOWS)
   #message(WARNING, ${CMAKE_CURRENT_BINARY_DIR}/newview/viewerRes.rc.in)
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/res/viewerRes.rc.in
       ${CMAKE_CURRENT_BINARY_DIR}/viewerRes.rc
   )
endif (WINDOWS)
