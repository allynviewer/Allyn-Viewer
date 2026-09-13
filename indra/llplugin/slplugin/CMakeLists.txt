project(SLPlugin)

include(00-Common)
include(LLCommon)
include(LLPlugin)
include(Linking)
include(PluginAPI)
include(LLMessage)
include(WinManifest)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
)

### SLPlugin

set(SLPlugin_SOURCE_FILES
    slplugin.cpp
    )

set_source_files_properties(${SLPlugin_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

if (SLPlugin_HEADER_FILES)
  list(APPEND SLPlugin_SOURCE_FILES ${SLPlugin_HEADER_FILES})
endif (SLPlugin_HEADER_FILES)

if (WINDOWS)
  # Version resource (ProductName / ProductVersion) so that SLPlugin.exe can be
  # code-signed with the same file metadata restrictions as the viewer.
  # VIEWER_VERSION_* come from indra/cmake/BuildVersion.cmake (included in indra/).
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/slplugin.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/slplugin.rc
  )
  list(APPEND SLPlugin_SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/slplugin.rc)
  source_group("Resource Files" FILES ${CMAKE_CURRENT_BINARY_DIR}/slplugin.rc)
endif (WINDOWS)

add_executable(SLPlugin
    WIN32
    ${SLPlugin_SOURCE_FILES}
)

if (WINDOWS)
  set_target_properties(SLPlugin
    PROPERTIES
    LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMTD\""
  )
  EMBED_MANIFEST(SLPlugin 1)
endif()

target_link_libraries(SLPlugin
  ${LLPLUGIN_LIBRARIES}
  ${LLMESSAGE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${APRUTIL_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
  ${PTHREAD_LIBRARY}
)

#ll_deploy_sharedlibs_command(SLPlugin)
