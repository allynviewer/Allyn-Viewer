# -*- cmake -*-
include(Prebuilt)
include(Variables)

if (USE_NVAPI)
  if (WINDOWS)
    use_prebuilt_binary(nvapi)
    set(NVAPI_LIBRARY nvapi)
    set(_allyn_nvapi_header "${AUTOBUILD_INSTALL_DIR}/include/nvapi/nvapi_lite_common.h")
    if(EXISTS "${_allyn_nvapi_header}" AND PYTHON_EXECUTABLE)
      execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c "import pathlib,sys; p=pathlib.Path(sys.argv[1]); d=p.read_bytes(); n=d.replace(bytes([147]), bytes([226,128,156])).replace(bytes([148]), bytes([226,128,157])); n!=d and p.write_bytes(n)" "${_allyn_nvapi_header}")
    endif()
  else (WINDOWS)
    set(NVAPI_LIBRARY "")
  endif (WINDOWS)
else (USE_NVAPI)
  set(NVAPI_LIBRARY "")
endif (USE_NVAPI)

