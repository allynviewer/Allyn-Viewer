# -*- cmake -*-

include(Prebuilt)

# Windows 64-bit only: TCMalloc is not used.
set(DISABLE_TCMALLOC TRUE)

if (STANDALONE)
  include(FindGooglePerfTools)
endif (STANDALONE)

if (GOOGLE_PERFTOOLS_FOUND AND STANDALONE)
  set(USE_GOOGLE_PERFTOOLS ON CACHE BOOL "Build with Google PerfTools support.")
else ()
  set(USE_GOOGLE_PERFTOOLS OFF)
endif ()

# XXX Disable temporarily, until we have compilation issues on 64-bit
# Etch sorted.
#set(USE_GOOGLE_PERFTOOLS OFF)

if (USE_GOOGLE_PERFTOOLS)
  set(TCMALLOC_FLAG -DLL_USE_TCMALLOC=1)
  include_directories(${GOOGLE_PERFTOOLS_INCLUDE_DIR})
  set(GOOGLE_PERFTOOLS_LIBRARIES ${TCMALLOC_LIBRARIES} ${STACKTRACE_LIBRARIES} ${PROFILER_LIBRARIES})
else (USE_GOOGLE_PERFTOOLS)
  set(TCMALLOC_FLAG -ULL_USE_TCMALLOC)
endif (USE_GOOGLE_PERFTOOLS)

add_definitions(${TCMALLOC_FLAG})