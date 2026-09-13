project(deps)

include(FetchContent)

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
