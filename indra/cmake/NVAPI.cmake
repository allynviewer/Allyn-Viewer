# -*- cmake -*-
include(Prebuilt)
include(Variables)

if (USE_NVAPI)
  if (WINDOWS)
    use_prebuilt_binary(nvapi)
    set(NVAPI_LIBRARY nvapi)
  else (WINDOWS)
    set(NVAPI_LIBRARY "")
  endif (WINDOWS)
else (USE_NVAPI)
  set(NVAPI_LIBRARY "")
endif (USE_NVAPI)

