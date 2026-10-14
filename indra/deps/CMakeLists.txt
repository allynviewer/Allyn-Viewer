project(deps)

include(FetchContent)

if(WIN32)
  function(allyn_preset_check name value)
    if(NOT DEFINED "${name}" OR "x${${name}}" STREQUAL "x${name}")
      set("${name}" "${value}" CACHE INTERNAL "" FORCE)
    endif()
  endfunction()

  foreach(_allyn_check
      CMAKE_HAVE_LIBC_PTHREAD
      CMAKE_HAVE_PTHREADS_CREATE
      CMAKE_HAVE_PTHREAD_CREATE
      has_std_20_flag
      has_std_2a_flag
      has_std_17_flag
      has_std_1z_flag
      has_std_14_flag
      has_std_1y_flag
      has_std_11_flag
      has_std_0x_flag
      HAVE_ARC4RANDOM_BUF
      HAVE_ARPA_INET_H
      HAVE_ARPA_NAMESER_COMPAT_H
      HAVE_ARPA_NAMESER_H
      HAVE_AVAILABILITYMACROS_H
      HAVE_CLOCK_GETTIME_MONOTONIC
      HAVE_CLOSESOCKET_CAMEL
      HAVE_CONNECTX
      HAVE_DLFCN_H
      HAVE_EPOLL
      HAVE_FCNTL
      HAVE_GETIFADDRS
      HAVE_GETRANDOM
      HAVE_GETSERVBYNAME_R
      HAVE_GETSERVBYPORT_R
      HAVE_GETTIMEOFDAY
      HAVE_GHBN_LIBSOCKET
      HAVE_IFADDRS_H
      HAVE_INET_NET_PTON
      HAVE_IOCTL
      HAVE_IOCTLSOCKET_CAMEL
      HAVE_IOCTL_SIOCGIFADDR
      HAVE_KQUEUE
      HAVE_LIBNETWORK
      HAVE_LIBNSL
      HAVE_LIBRT
      HAVE_MEMMEM
      HAVE_MSG_NOSIGNAL
      HAVE_NETDB_H
      HAVE_NETINET6_IN6_H
      HAVE_NETINET_IN_H
      HAVE_NETINET_TCP_H
      HAVE_NET_IF_H
      HAVE_NTDEF_H
      HAVE_NTSTATUS_H
      HAVE_O_NONBLOCK
      HAVE_PIPE
      HAVE_PIPE2
      HAVE_POLL
      HAVE_POLL_H
      HAVE_RES_SERVICENAME_IN_LIBRESOLV
      HAVE_SOCKET_H
      HAVE_SOCKET_LIBSOCKET
      HAVE_SO_NONBLOCK
      HAVE_SSIZE_T
      HAVE_STRCASECMP
      HAVE_STRINGS_H
      HAVE_STRNCASECMP
      HAVE_STRNCMPI
      HAVE_STROPTS_H
      HAVE_SYS_EPOLL_H
      HAVE_SYS_EVENT_H
      HAVE_SYS_IOCTL_H
      HAVE_SYS_PARAM_H
      HAVE_SYS_RANDOM_H
      HAVE_SYS_SELECT_H
      HAVE_SYS_SOCKET_H
      HAVE_SYS_SOCKIO_H
      HAVE_SYS_TIME_H
      HAVE_SYS_UIO_H
      HAVE_UNISTD_H
      HAVE_WRITEV
      HAVE___SYSTEM_PROPERTY_GET
      _CARES_FUNC_IN_LIB_GLOBAL_clock_gettime
      _CARES_FUNC_IN_LIB_GLOBAL_gethostbyname
      _CARES_FUNC_IN_LIB_GLOBAL_res_servicename
      _CARES_FUNC_IN_LIB_GLOBAL_socket)
    allyn_preset_check(${_allyn_check} "")
  endforeach()

  foreach(_allyn_check
      SUPPORTS_USER_DEFINED_LITERALS
      FMT_HAS_VARIANT
      HAVE_STRTOD_L
      HAVE_AF_INET6
      HAVE_ASSERT_H
      HAVE_CLOSESOCKET
      HAVE_CONNECT
      HAVE_CONVERTINTERFACEINDEXTOLUID
      HAVE_CONVERTINTERFACELUIDTONAMEA
      HAVE_ERRNO_H
      HAVE_FCNTL_H
      HAVE_FIONBIO
      HAVE_FREEADDRINFO
      HAVE_GETADDRINFO
      HAVE_GETENV
      HAVE_GETHOSTNAME
      HAVE_GETNAMEINFO
      HAVE_IF_INDEXTONAME
      HAVE_IF_NAMETOINDEX
      HAVE_INTTYPES_H
      HAVE_IOCTLSOCKET
      HAVE_IPHLPAPI_H
      HAVE_LIMITS_H
      HAVE_LONGLONG
      HAVE_MALLOC_H
      HAVE_MEMORY_H
      HAVE_MSWSOCK_H
      HAVE_NETIOAPI_H
      HAVE_NOTIFYIPINTERFACECHANGE
      HAVE_PF_INET6
      HAVE_RECV
      HAVE_RECVFROM
      HAVE_REGISTERWAITFORSINGLEOBJECT
      HAVE_SEND
      HAVE_SENDTO
      HAVE_SETSOCKOPT
      HAVE_SIGNAL_H
      HAVE_SOCKET
      HAVE_SOCKLEN_T
      HAVE_STAT
      HAVE_STDBOOL_H
      HAVE_STDINT_H
      HAVE_STDLIB_H
      HAVE_STRCMPI
      HAVE_STRDUP
      HAVE_STRICMP
      HAVE_STRING_H
      HAVE_STRNICMP
      HAVE_STRNLEN
      HAVE_STRUCT_ADDRINFO
      HAVE_STRUCT_IN6_ADDR
      HAVE_STRUCT_SOCKADDR_IN6
      HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
      HAVE_STRUCT_SOCKADDR_STORAGE
      HAVE_STRUCT_TIMEVAL
      HAVE_SYS_STAT_H
      HAVE_SYS_TYPES_H
      HAVE_TIME_H
      HAVE_TYPE_SOCKET
      HAVE_WINDOWS_H
      HAVE_WINSOCK2_H
      HAVE_WINSOCK_H
      HAVE_WINTERNL_H
      HAVE_WS2TCPIP_H
      HAVE__W4
      HAVE__W4_CXX
      HAVE__w14242
      HAVE__w14242_CXX
      HAVE__w14254
      HAVE__w14254_CXX
      HAVE__w14263
      HAVE__w14263_CXX
      HAVE__w14265
      HAVE__w14265_CXX
      HAVE__w14287
      HAVE__w14287_CXX
      HAVE__w14296
      HAVE__w14296_CXX
      HAVE__w14311
      HAVE__w14311_CXX
      HAVE__w14545
      HAVE__w14545_CXX
      HAVE__w14546
      HAVE__w14546_CXX
      HAVE__w14547
      HAVE__w14547_CXX
      HAVE__w14549
      HAVE__w14549_CXX
      HAVE__w14555
      HAVE__w14555_CXX
      HAVE__w14619
      HAVE__w14619_CXX
      HAVE__w14640
      HAVE__w14640_CXX
      HAVE__w14826
      HAVE__w14826_CXX
      HAVE__w14905
      HAVE__w14905_CXX
      HAVE__w14906
      HAVE__w14906_CXX
      HAVE__w14928
      HAVE__w14928_CXX
      HAVE__wd4201
      HAVE__wd4201_CXX
      HAVE__we4013
      HAVE__we4013_CXX
      HAVE__we4289
      HAVE__we4289_CXX)
    allyn_preset_check(${_allyn_check} 1)
  endforeach()
endif()

set(CMAKE_FOLDER "Third Party")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.11.0
  GIT_SHALLOW    TRUE
  )
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        8d78045e7cb44d39ad4cd95dd27816b8749e1944
  )
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.7.3
  GIT_SHALLOW    TRUE
  )
FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG        768eb2ca2857342673fcd462792ce04b8bac3fa3
)

# This is a hack because absl has dumb cmake
set(OLD_BUILD_TEST ${BUILD_TESTING})
set(BUILD_TESTING OFF)
FetchContent_MakeAvailable(absl)
set(BUILD_TESTING ${OLD_BUILD_TEST})

# Supress warnings inside abseil under MSVC
if(WINDOWS)
  target_compile_options(absl_strings PRIVATE /wd4018)
  target_compile_options(absl_str_format_internal PRIVATE /wd4018)
  target_compile_options(absl_flags_usage_internal PRIVATE /wd4018)
endif()


if (BUILD_TESTING)
  FetchContent_MakeAvailable(Catch2)
endif()

#Download the rest of the libraries
FetchContent_MakeAvailable(fmt)

# Typically you don't care so much for a third party library's tests to be
# run from your own project's code.
set(JSON_BuildTests OFF CACHE INTERNAL "")

# If you only include this third party in PRIVATE source files, you do not
# need to install it when your main project gets installed.
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

set(CARES_STATIC ON CACHE BOOL "Build c-ares as a static library" FORCE)
set(CARES_SHARED OFF CACHE BOOL "Disable shared c-ares" FORCE)
set(CARES_INSTALL OFF CACHE BOOL "Do not install c-ares" FORCE)
set(CARES_BUILD_TESTS OFF CACHE BOOL "Do not build c-ares tests" FORCE)
set(CARES_BUILD_TOOLS OFF CACHE BOOL "Do not build c-ares tools" FORCE)
FetchContent_Declare(
  c-ares
  GIT_REPOSITORY https://github.com/c-ares/c-ares.git
  GIT_TAG        v1.34.4
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(c-ares)

# xmlrpc-epi has no public windows64 package compatible with our expat 2.6.4.
include(EXPAT)
FetchContent_Declare(
  xmlrpc_epi_src
  GIT_REPOSITORY https://github.com/secondlife/3p-xmlrpc-epi.git
  GIT_TAG        v0.54.1-r5
  GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(xmlrpc_epi_src)
if (NOT xmlrpc_epi_src_POPULATED)
  FetchContent_Populate(xmlrpc_epi_src)
endif ()

# Upstream xmlrpc-epi still mixes XMLRPC_VALUE_TYPE with XMLRPC_VALUE_TYPE_EASY
# and leaves an unused `tm` on MSVC (gmtime vs gmtime_r). Patch the fetched
# tree in place; replacements are idempotent so reconfigure is a no-op.
function(allyn_replace_in_file filepath)
  file(READ "${filepath}" _txt)
  string(REPLACE "\r\n" "\n" _txt "${_txt}")
  set(_orig "${_txt}")
  math(EXPR _last "${ARGC} - 1")
  set(_i 1)
  while(_i LESS _last)
    math(EXPR _j "${_i} + 1")
    string(REPLACE "${ARGV${_i}}" "${ARGV${_j}}" _txt "${_txt}")
    math(EXPR _i "${_i} + 2")
  endwhile()
  if(NOT _txt STREQUAL _orig)
    file(WRITE "${filepath}" "${_txt}")
  endif()
endfunction()

set(_xmlrpc_src_dir "${xmlrpc_epi_src_SOURCE_DIR}/xmlrpc-epi/src")
allyn_replace_in_file("${_xmlrpc_src_dir}/xmlrpc.c"
  "static int date_to_ISO8601 (time_t value, char *buf, int length) {\n   struct tm tm,*ptm;\n#ifdef _MSC_VER\n   // Visual C++ does not have gmtime_r, but its gmtime is threadsafe.\n   ptm = gmtime(&value);\n#else\n   ptm = gmtime_r(&value, &tm);\n#endif"
  "static int date_to_ISO8601 (time_t value, char *buf, int length) {\n#ifdef _MSC_VER\n   struct tm *ptm;\n   // Visual C++ does not have gmtime_r, but its gmtime is threadsafe.\n   ptm = gmtime(&value);\n#else\n   struct tm tm, *ptm;\n   ptm = gmtime_r(&value, &tm);\n#endif"
  "            /* evil cast, but we know they are the same */\n            return(XMLRPC_VALUE_TYPE_EASY) value->type;\n        }\n    }\n    return xmlrpc_none;"
  "            /* evil cast, but we know they are the same */\n            return(XMLRPC_VALUE_TYPE_EASY) value->type;\n        }\n    }\n    return xmlrpc_type_none;"
)
allyn_replace_in_file("${_xmlrpc_src_dir}/xmlrpc_introspection.c"
  "   if(etype == xmlrpc_int) {"
  "   if(etype == xmlrpc_type_int) {"
  "   else if(etype == xmlrpc_double) {"
  "   else if(etype == xmlrpc_type_double) {"
  "   else if(etype == xmlrpc_string ||\n           etype == xmlrpc_base64) {"
  "   else if(etype == xmlrpc_type_string ||\n           etype == xmlrpc_type_base64) {"
)
# Glob RELATIVE to the source dir so the sample/test filter matches file names
# only. Filtering absolute paths would drop every source when the checkout
# lives under a directory containing "test" or "sample" (e.g. C:\dev\test\...).
file(GLOB _xmlrpc_c_files RELATIVE "${_xmlrpc_src_dir}" "${_xmlrpc_src_dir}/*.c")
list(FILTER _xmlrpc_c_files EXCLUDE REGEX "sample|test")
list(TRANSFORM _xmlrpc_c_files PREPEND "${_xmlrpc_src_dir}/")
if (NOT _xmlrpc_c_files)
  message(FATAL_ERROR "xmlrpc-epi: no sources found in ${_xmlrpc_src_dir} (FetchContent failed?)")
endif ()
add_library(xmlrpc-epi STATIC ${_xmlrpc_c_files})
set(_xmlrpc_inc_dir "${CMAKE_CURRENT_BINARY_DIR}/xmlrpc_inc")
file(MAKE_DIRECTORY "${_xmlrpc_inc_dir}/xmlrpc-epi")
file(GLOB _xmlrpc_h_files "${_xmlrpc_src_dir}/*.h")
file(COPY ${_xmlrpc_h_files} DESTINATION "${_xmlrpc_inc_dir}/xmlrpc-epi")
target_include_directories(xmlrpc-epi
  PUBLIC
    "${_xmlrpc_inc_dir}"
    "${_xmlrpc_src_dir}"
  PRIVATE
    ${EXPAT_INCLUDE_DIRS}
)
target_link_libraries(xmlrpc-epi PUBLIC ${EXPAT_LIBRARIES})
if (WINDOWS)
  # MSVC has _stricmp, not POSIX strcasecmp (xmlrpc-epi uses the POSIX name).
  # XML_STATIC is required because we link the static libexpat: without it,
  # expat.h marks XML_* APIs as __declspec(dllimport) and the viewer link
  # emits LNK4286 (xmlrpc-epi imports symbols that libexpat defines).
  target_compile_definitions(xmlrpc-epi PRIVATE
    XML_STATIC
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
    strcasecmp=_stricmp
    strncasecmp=_strnicmp
  )
endif ()

unset(CMAKE_FOLDER)
unset(CMAKE_POSITION_INDEPENDENT_CODE)
