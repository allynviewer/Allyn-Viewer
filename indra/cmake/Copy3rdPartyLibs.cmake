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

if(NOT USESYSTEMLIBS)
  add_custom_target(
      stage_third_party_libs ALL
      DEPENDS ${third_party_targets}
      )
endif(NOT USESYSTEMLIBS)
