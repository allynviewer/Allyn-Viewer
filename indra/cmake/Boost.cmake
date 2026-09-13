# -*- cmake -*-
include(Prebuilt)

set(Boost_FIND_QUIETLY ON)
set(Boost_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindBoost)

  set(Boost_USE_MULTITHREADED ON)
  find_package(Boost 1.51.0 COMPONENTS date_time filesystem program_options regex system thread wave context)
else (STANDALONE)
  use_prebuilt_binary(boost)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  set(Boost_VERSION "1.60")

  if (WINDOWS)
    set(Boost_CONTEXT_LIBRARY
        optimized libboost_context
        debug libboost_context)
    set(Boost_FIBER_LIBRARY
        optimized libboost_fiber
        debug libboost_fiber)
    set(Boost_FILESYSTEM_LIBRARY
        optimized libboost_filesystem
        debug libboost_filesystem)
    set(Boost_PROGRAM_OPTIONS_LIBRARY
        optimized libboost_program_options
        debug libboost_program_options)
    # boost 1.90: regex and system are header-only
    set(Boost_REGEX_LIBRARY "")
    set(Boost_SIGNALS_LIBRARY "")
    set(Boost_SYSTEM_LIBRARY "")
    set(Boost_THREAD_LIBRARY
        optimized libboost_thread
        debug libboost_thread)
    set(Boost_DATE_TIME_LIBRARY
        optimized libboost_date_time
        debug libboost_date_time)
    set(Boost_JSON_LIBRARY
        optimized libboost_json
        debug libboost_json)
  endif (WINDOWS)
endif (STANDALONE)
