# -*- cmake -*-
#
# Definitions of variables used throughout the Second Life build
# process.
#
# Platform variables:
#
#   WINDOWS - Windows (Allyn Viewer is Windows 64-bit only)

# Relative and absolute paths to subtrees.
if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

if(NOT DEFINED COMMON_CMAKE_DIR)
    set(COMMON_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake")
endif(NOT DEFINED COMMON_CMAKE_DIR)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# https://blog.kitware.com/upcoming-in-cmake-2-8-12-osx-rpath-support/
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
option(GEN_IS_MULTI_CONFIG "" ${_isMultiConfig})
mark_as_advanced(GEN_IS_MULTI_CONFIG)

set(LIBS_CLOSED_PREFIX)
set(LIBS_OPEN_PREFIX)
set(SCRIPTS_PREFIX ../scripts)
set(VIEWER_PREFIX)
set(INTEGRATION_TESTS_PREFIX)

option(LL_TESTS "Build and run unit and integration tests (disable for build timing runs to reduce variation" OFF)
option(BUILD_TESTING "Build test suite" OFF)
option(UNATTENDED "Disable use of uneeded tooling for automated builds" OFF)

# Compiler and toolchain options
option(USESYSTEMLIBS "Use libraries from your system rather than Linden-supplied prebuilt libraries." OFF)
option(STANDALONE "Use libraries from your system rather than Linden-supplied prebuilt libraries." OFF)
if (USESYSTEMLIBS)
  set(STANDALONE ON)
elseif (STANDALONE)
  set(USESYSTEMLIBS ON)
endif (USESYSTEMLIBS)
option(INCREMENTAL_LINK "Use incremental linking on win32 builds (enable for faster links on some machines)" OFF)
option(USE_PRECOMPILED_HEADERS "Enable use of precompiled header directives where supported." ON)
option(USE_LTO "Enable global and interprocedural optimizations" OFF)

# Configure crash reporting
option(USE_CRASHPAD "Build support for crashpad reporting engine" OFF)
if (DEFINED ENV{VIEWER_USE_CRASHPAD})
  set(USE_CRASHPAD $ENV{VIEWER_USE_CRASHPAD})
endif()

if (DEFINED ENV{VIEWER_CRASHPAD_URL})
  set(CRASHPAD_URL $ENV{VIEWER_CRASHPAD_URL} CACHE STRING "Viewer Channel Base Name")
else()
  set(CRASHPAD_URL "" CACHE STRING "Crashpad endpoint url")
endif()

set(VIEWER_SYMBOL_FILE "" CACHE STRING "Name of tarball into which to place symbol files")

# Media Plugins
option(ENABLE_MEDIA_PLUGINS "Turn off building media plugins if they are imported by third-party library mechanism" ON)
option(LIBVLCPLUGIN "Turn off building support for libvlc plugin" ON)

# Mallocs
set(DISABLE_TCMALLOC OFF CACHE BOOL "Disable linkage of TCMalloc. (64bit builds automatically disable TCMalloc)")
set(DISABLE_FATAL_WARNINGS TRUE CACHE BOOL "Set this to FALSE to enable fatal warnings.")

# Proprietary Library Features
option(USE_NVAPI "Use nvapi driver interface library" OFF)


if(LIBS_CLOSED_DIR)
  file(TO_CMAKE_PATH "${LIBS_CLOSED_DIR}" LIBS_CLOSED_DIR)
else(LIBS_CLOSED_DIR)
  set(LIBS_CLOSED_DIR ${CMAKE_SOURCE_DIR}/${LIBS_CLOSED_PREFIX})
endif(LIBS_CLOSED_DIR)
if(LIBS_COMMON_DIR)
  file(TO_CMAKE_PATH "${LIBS_COMMON_DIR}" LIBS_COMMON_DIR)
else(LIBS_COMMON_DIR)
  set(LIBS_COMMON_DIR ${CMAKE_SOURCE_DIR}/${LIBS_OPEN_PREFIX})
endif(LIBS_COMMON_DIR)
set(LIBS_OPEN_DIR ${LIBS_COMMON_DIR})

set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/${SCRIPTS_PREFIX})
set(VIEWER_DIR ${CMAKE_SOURCE_DIR}/${VIEWER_PREFIX})

set(AUTOBUILD_INSTALL_DIR ${CMAKE_BINARY_DIR}/packages)

set(LIBS_PREBUILT_DIR ${AUTOBUILD_INSTALL_DIR} CACHE PATH
    "Location of prebuilt libraries.")

if (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
  # We use this as a marker that you can try to use the proprietary libraries.
  set(INSTALL_PROPRIETARY ON CACHE BOOL "Install proprietary binaries")
endif (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
set(TEMPLATE_VERIFIER_OPTIONS "" CACHE STRING "Options for scripts/template_verifier.py")
set(TEMPLATE_VERIFIER_MASTER_URL "https://raw.githubusercontent.com/secondlife/viewer/develop/indra/newview/app_settings/message_template.msg" CACHE STRING "Location of the master message template")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  message(FATAL_ERROR "Allyn Viewer supports Windows 64-bit only (no Linux, no other OS).")
endif ()

set(WINDOWS ON BOOL FORCE)

# 64-bit only. Refuse 32-bit Windows.
if (ADDRESS_SIZE EQUAL 32)
  message(FATAL_ERROR "Allyn Viewer does not support 32-bit Windows.")
elseif (ADDRESS_SIZE EQUAL 64)
  set(ARCH x86_64)
else ()
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH x86_64)
    set(ADDRESS_SIZE 64)
  else ()
    message(FATAL_ERROR "Allyn Viewer does not support 32-bit Windows.")
  endif ()
endif ()

set(LL_ARCH ${ARCH}_win64)
set(LL_ARCH_DIR ${ARCH}-win64)





# Platform specific
if (WINDOWS)
  option(LLWINDOW_SDL2 "Use SDL2 for window and input handling. Windows only" OFF)
endif()

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

if (DEFINED ENV{VIEWER_CHANNEL_BASE})
  set(VIEWER_CHANNEL_BASE $ENV{VIEWER_CHANNEL_BASE} CACHE STRING "Viewer Channel Base Name" FORCE)
else()
  set(VIEWER_CHANNEL_BASE "Allyn Viewer" CACHE STRING "Viewer Channel Base Name")
endif()

if (DEFINED ENV{VIEWER_CHANNEL_TYPE})
  set(VIEWER_CHANNEL_TYPE $ENV{VIEWER_CHANNEL_TYPE} CACHE STRING "Viewer Channel Type Name" FORCE)
else()
  set(VIEWER_CHANNEL_TYPE "Beta" CACHE STRING "Viewer Channel Type Name")
endif()

if (DEFINED ENV{VIEWER_CHANNEL_CODENAME})
  set(VIEWER_CHANNEL_CODENAME $ENV{VIEWER_CHANNEL_CODENAME} CACHE STRING "Viewer Channel Code Name for Project type" FORCE)
else()
  set(VIEWER_CHANNEL_CODENAME "Default" CACHE STRING "Viewer Channel Code Name for Project type")
endif()

if("${VIEWER_CHANNEL_TYPE}" STREQUAL "Project")
  set(VIEWER_CHANNEL "${VIEWER_CHANNEL_BASE} ${VIEWER_CHANNEL_TYPE} ${VIEWER_CHANNEL_CODENAME}")
else()
  set(VIEWER_CHANNEL "${VIEWER_CHANNEL_BASE} ${VIEWER_CHANNEL_TYPE}")
endif()

string(TOLOWER "${VIEWER_CHANNEL_BASE}" VIEWER_BRANDING_ID)
string(REPLACE " " "-" VIEWER_BRANDING_ID ${VIEWER_BRANDING_ID})
set(VIEWER_BINARY_NAME "${VIEWER_BRANDING_ID}-bin" CACHE STRING
    "The name of the viewer executable to create.")

string(REPLACE " " "" VIEWER_CHANNEL_ONEWORD ${VIEWER_CHANNEL})
set(VIEWER_CHANNEL_NOSPACE ${VIEWER_CHANNEL_ONEWORD} CACHE STRING "Prefix used for resulting artifacts.")

option(VIEWER_CHANNEL_GRK "Greek character(s) to represent the viewer channel for support purposes, override only for special branches" "")
if (NOT VIEWER_CHANNEL_GRK)
  if (VIEWER_CHANNEL_TYPE MATCHES "Test")
    set(VIEWER_CHANNEL_GRK "\\u03C4") # "Ï„"
  elseif (VIEWER_CHANNEL_TYPE MATCHES "Alpha")
    set(VIEWER_CHANNEL_GRK "\\u03B1") # "Î±"
  elseif (VIEWER_CHANNEL_TYPE MATCHES "Beta")
    set(VIEWER_CHANNEL_GRK "\\u03B2") # "Î²"
  else()
    set(VIEWER_CHANNEL_GRK "")
  endif ()
endif (NOT VIEWER_CHANNEL_GRK)

option(ENABLE_SIGNING "Enable signing the viewer" OFF)
set(SIGNING_IDENTITY "" CACHE STRING "Specifies the signing identity to use, if necessary.")

source_group("CMake Rules" FILES CMakeLists.txt)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
