# - Embeds a specific manifest file in a Windows binary
# Defines the following:
# EMBED_MANIFEST - embed manifest in a windows binary with mt
# Parameters - _target is the target file, type - 1 for EXE, 2 for DLL

if (NOT DEFINED ALYNN_MT_EXE)
  find_program(ALYNN_MT_EXE
    NAMES mt mt.exe
    HINTS
      "$ENV{WindowsSdkVerBinPath}/x64"
      "$ENV{WindowsSdkVerBinPath}/x86"
      "$ENV{WINDOWSSDKDIR}/bin/$ENV{WindowsSDKVersion}/x64"
      "$ENV{WINDOWSSDKDIR}/bin/$ENV{WindowsSDKVersion}/x86"
    PATHS
      "C:/Program Files (x86)/Windows Kits/10/bin"
      "C:/Program Files/Windows Kits/10/bin"
    PATH_SUFFIXES
      x64
      x86
      10.0.26100.0/x64
      10.0.22621.0/x64
      10.0.19041.0/x64
      10.0.18362.0/x64
    DOC "Windows Manifest Tool (mt.exe)"
  )
  if (NOT ALYNN_MT_EXE)
    message(WARNING "mt.exe not found; EMBED_MANIFEST will try PATH at build time")
    set(ALYNN_MT_EXE "mt.exe")
  else()
    message(STATUS "Found Manifest Tool: ${ALYNN_MT_EXE}")
  endif()
endif()

MACRO(EMBED_MANIFEST _target type)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMAND "${ALYNN_MT_EXE}"
    ARGS
      -manifest \"${CMAKE_SOURCE_DIR}\\tools\\manifests\\compatibility.manifest\"
      -inputresource:\"$<TARGET_FILE:${_target}>\"\;\#${type}
      -outputresource:\"$<TARGET_FILE:${_target}>\"\;\#${type}
    COMMENT "Adding compatibility manifest to ${_target}"
  )
ENDMACRO(EMBED_MANIFEST _target type)
