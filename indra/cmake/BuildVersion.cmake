# -*- cmake -*-
# Construct the viewer version number based on the indra/VIEWER_VERSION file

if (NOT DEFINED VIEWER_SHORT_VERSION) # will be true in indra/, false in indra/newview/
    set(VIEWER_VERSION_BASE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/newview/VIEWER_VERSION.txt")

    if ( EXISTS ${VIEWER_VERSION_BASE_FILE} )
        file(STRINGS ${VIEWER_VERSION_BASE_FILE} VIEWER_SHORT_VERSION REGEX "^[0-9]+\\.[0-9]+\\.[0-9]+")
        string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" VIEWER_VERSION_MAJOR ${VIEWER_SHORT_VERSION})
        string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" VIEWER_VERSION_MINOR ${VIEWER_SHORT_VERSION})
        string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" VIEWER_VERSION_PATCH ${VIEWER_SHORT_VERSION})

        set(VIEWER_VERSION_REVISION "")
        if (DEFINED ENV{revision})
            set(VIEWER_VERSION_REVISION "$ENV{revision}")
            string(STRIP "${VIEWER_VERSION_REVISION}" VIEWER_VERSION_REVISION)
        endif ()
        if (VIEWER_VERSION_REVISION MATCHES "^[0-9]+$")
            message("Revision (from environment): ${VIEWER_VERSION_REVISION}")
        else ()
            find_package(Git QUIET)
            if (Git_FOUND)
                execute_process(
                    COMMAND "${GIT_EXECUTABLE}" rev-list --count HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    RESULT_VARIABLE _allyn_git_revision_rc
                    OUTPUT_VARIABLE VIEWER_VERSION_REVISION
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
                if (NOT _allyn_git_revision_rc EQUAL 0)
                    set(VIEWER_VERSION_REVISION "")
                endif ()
            endif ()
            if (VIEWER_VERSION_REVISION MATCHES "^[0-9]+$")
                message("Revision (from git): ${VIEWER_VERSION_REVISION}")
            else ()
                message("Revision could not be read from git: will use 0")
                set(VIEWER_VERSION_REVISION 0)
            endif ()
        endif ()
        message("Building '${VIEWER_CHANNEL}' Version ${VIEWER_SHORT_VERSION} (${VIEWER_VERSION_REVISION})")
    else ( EXISTS ${VIEWER_VERSION_BASE_FILE} )
        message(SEND_ERROR "Cannot get viewer version from '${VIEWER_VERSION_BASE_FILE}'") 
    endif ( EXISTS ${VIEWER_VERSION_BASE_FILE} )

    if ("${VIEWER_VERSION_REVISION}" STREQUAL "")
      message("Ultimate fallback, revision was blank or not set: will use 0")
      set(VIEWER_VERSION_REVISION 0)
    endif ("${VIEWER_VERSION_REVISION}" STREQUAL "")

    set(VIEWER_CHANNEL_VERSION_DEFINES
        "LL_VIEWER_CHANNEL=\"${VIEWER_CHANNEL}\""
        "LL_VIEWER_CHANNEL_GRK=L\"${VIEWER_CHANNEL_GRK}\""
        "LL_VIEWER_VERSION_MAJOR=${VIEWER_VERSION_MAJOR}"
        "LL_VIEWER_VERSION_MINOR=${VIEWER_VERSION_MINOR}"
        "LL_VIEWER_VERSION_PATCH=${VIEWER_VERSION_PATCH}"
        "LL_VIEWER_VERSION_BUILD=${VIEWER_VERSION_REVISION}"
        )
endif (NOT DEFINED VIEWER_SHORT_VERSION)
