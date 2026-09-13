# -*- cmake -*-

set(PYTHONINTERP_FOUND)

if (WINDOWS)
  # On Windows, explicitly avoid Cygwin Python.

  if (DEFINED ENV{VIRTUAL_ENV})
    find_program(PYTHON_EXECUTABLE
      NAMES python.exe
      PATHS
      "$ENV{VIRTUAL_ENV}\\scripts"
      NO_DEFAULT_PATH
      )
  else()
    find_program(PYTHON_EXECUTABLE
      NAMES python.exe python3.exe python314.exe python313.exe python312.exe python311.exe python310.exe python25.exe python23.exe
      PATHS
      "C:\\Python314"
      "C:\\Python313"
      "C:\\Python312"
      "C:\\Python311"
      "C:\\Python310"
      "$ENV{LOCALAPPDATA}\\Programs\\Python\\Python314"
      "$ENV{LOCALAPPDATA}\\Programs\\Python\\Python313"
      "$ENV{LOCALAPPDATA}\\Programs\\Python\\Python312"
      "$ENV{LOCALAPPDATA}\\Programs\\Python\\Python311"
      "$ENV{LOCALAPPDATA}\\Programs\\Python\\Python310"
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.14\\InstallPath]
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.13\\InstallPath]
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.12\\InstallPath]
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.11\\InstallPath]
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.10\\InstallPath]
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath]
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.6\\InstallPath]
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.5\\InstallPath]
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\3.14\\InstallPath]
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\3.13\\InstallPath]
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\3.12\\InstallPath]
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\3.11\\InstallPath]
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\3.10\\InstallPath]
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath]
      )
  endif()

  if (PYTHON_EXECUTABLE)
    set(PYTHONINTERP_FOUND ON)
  endif (PYTHON_EXECUTABLE)

elseif (EXISTS /usr/bin/python2)
  # if this is there, use it

  find_program(PYTHON_EXECUTABLE python2 PATHS /usr/bin)

  if (PYTHON_EXECUTABLE)
    set(PYTHONINTERP_FOUND ON)
  endif (PYTHON_EXECUTABLE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # On MAC OS X be sure to search standard locations first

  string(REPLACE ":" ";" PATH_LIST "$ENV{PATH}")
  find_program(PYTHON_EXECUTABLE
    NAMES python python25 python24 python23
    NO_DEFAULT_PATH # Avoid searching non-standard locations first
    PATHS
    /bin
    /usr/bin
    /usr/local/bin
    ${PATH_LIST}
    )

  if (PYTHON_EXECUTABLE)
    set(PYTHONINTERP_FOUND ON)
  endif (PYTHON_EXECUTABLE)
else (WINDOWS)
  include(FindPythonInterp)
endif (WINDOWS)

if (NOT PYTHON_EXECUTABLE)
  message(FATAL_ERROR "No Python interpreter found")
endif (NOT PYTHON_EXECUTABLE)

mark_as_advanced(PYTHON_EXECUTABLE)
