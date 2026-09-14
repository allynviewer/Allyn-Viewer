# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.

include(CMakeCopyIfDifferent)
include(Linking)
include(Variables)
include(LLCommon)

###################################################################
# set up platform specific lists of files that need to be copied
###################################################################
if(WINDOWS)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release")

    #*******************************
    # WebRTC voice runtime is built as llwebrtc.dll (staged by llwebrtc target).
    # No separate Vivox/SLVoice package.
    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        )

    # Misc shared libs — modern Linden packages (apr/openssl/hunspell) are often
    # static; only stage DLLs that still ship as shared libraries.
    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
        glod.dll
        )

    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        glod.dll
        )

    # Optional shared libs if present in the prebuilt tree
    foreach(_opt_dll uriparser.dll libhunspell.dll)
      if(EXISTS "${ARCH_PREBUILT_DIRS_RELEASE}/${_opt_dll}")
        list(APPEND release_files ${_opt_dll})
      endif()
      if(EXISTS "${ARCH_PREBUILT_DIRS_DEBUG}/${_opt_dll}")
        list(APPEND debug_files ${_opt_dll})
      endif()
    endforeach()

    if(USE_OPENAL)
      list(APPEND debug_files alut.dll OpenAL32.dll)
      list(APPEND release_files alut.dll OpenAL32.dll)
    endif(USE_OPENAL)

    #*******************************
    # Visual C++ runtime (msvcp140.dll, vcruntime140.dll, vcruntime140_1.dll, ...).
    # Shipped next to the executables so that:
    #  - the NSIS installer does not have to download vc_redist.exe from the
    #    internet and execute it at install time. "Download a binary and run
    #    it" is a classic Windows Defender / SmartScreen heuristic trigger for
    #    unsigned installers, and it also made installs fail offline;
    #  - the portable ZIP runs on a clean machine.
    # InstallRequiredSystemLibraries knows the redist layout of every toolset
    # (Microsoft.VC143.CRT for VS 2022, Microsoft.VC145.CRT for VS 2026).
    # The UCRT (ucrtbase.dll, api-ms-win-crt-*.dll) is part of Windows 10+ and
    # is not shipped.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
    set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
    include(InstallRequiredSystemLibraries)
    set(msvc_runtime_files ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
    if(NOT msvc_runtime_files AND DEFINED ENV{VCToolsRedistDir})
      # Toolset newer than this CMake release: vcvars exports VCToolsRedistDir.
      file(TO_CMAKE_PATH "$ENV{VCToolsRedistDir}" _vc_redist_dir)
      file(GLOB msvc_runtime_files
           "${_vc_redist_dir}/x64/Microsoft.VC*.CRT/msvcp140*.dll"
           "${_vc_redist_dir}/x64/Microsoft.VC*.CRT/vcruntime140*.dll"
           "${_vc_redist_dir}/x64/Microsoft.VC*.CRT/concrt140.dll")
    endif()
    if(msvc_runtime_files)
      message(STATUS "Staging Visual C++ runtime: ${msvc_runtime_files}")
    else()
      message(WARNING "Visual C++ runtime DLLs not found (MSVC_REDIST_DIR / VCToolsRedistDir). "
                      "msvcp140.dll / vcruntime140.dll will be missing from the package; "
                      "viewer_manifest.py refuses to build an installer without them.")
    endif()

endif(WINDOWS)


################################################################
# Done building the file lists, now set up the copy commands.
################################################################

# Vivox runtime removed (WebRTC). Skip empty vivox copy lists.



#copy_if_different(
#    ${debug_src_dir}
#    "${SHARED_LIB_STAGING_DIR_DEBUG}"
#    out_targets
#    ${debug_files}
#    )
#set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELEASE}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

# Visual C++ runtime: msvc_runtime_files holds absolute paths, so FROM_DIR is empty.
if(WINDOWS AND msvc_runtime_files)
  copy_if_different(
      ""
      "${SHARED_LIB_STAGING_DIR_RELEASE}"
      out_targets
      ${msvc_runtime_files}
      )
  set(third_party_targets ${third_party_targets} ${out_targets})

  copy_if_different(
      ""
      "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
      out_targets
      ${msvc_runtime_files}
      )
  set(third_party_targets ${third_party_targets} ${out_targets})
endif()

if(NOT USESYSTEMLIBS)
  add_custom_target(
      stage_third_party_libs ALL
      DEPENDS ${third_party_targets}
      )
endif(NOT USESYSTEMLIBS)
