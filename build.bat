@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ============================================================================
REM  Allyn Viewer - Windows x64 build helper
REM
REM  Interactive:  build.bat            (asks for the language, then shows the menu)
REM  Direct:       build.bat [full|viewer|configure|build|rebuild|clean|check|run|smoke|package|skins|install|tools|help]
REM
REM  "tools" installs missing Git / CMake / Python / Visual Studio 2022 Build Tools / NSIS
REM  with winget. Interactive mode offers this automatically when something is missing.
REM
REM  "package" creates the distribution packages in build-vc-64\dist:
REM    - portable ZIP (always)            e.g. Allyn_Viewer_Beta_1_0_0_1_x86_64.zip
REM    - NSIS installer (if NSIS present)  e.g. Allyn_Viewer_Beta_1_0_0_1_x86_64_Setup.exe
REM
REM  Language: interactive mode always asks. Command-line mode uses BUILD_LANG
REM  (de, en-us, es, fr, it, ja, pt, ru, tr; default en-us). Messages live in
REM  build-lang\<lang>.cmd (UTF-8 without BOM).
REM
REM  Full contributor guide: doc\building_windows.md
REM  Requirements: Visual Studio 2022/2026 (Desktop C++), CMake, Git, Python 3.
REM  Python packages (autobuild, llsd) are installed from requirements.txt.
REM ============================================================================

set "REPO_ROOT=%~dp0"
cd /d "%REPO_ROOT%"

REM The language files are UTF-8, so switch the console code page and restore it on exit.
set "ORIG_CP="
for /f "tokens=2 delims=:" %%C in ('chcp') do set "ORIG_CP=%%C"
set "ORIG_CP=%ORIG_CP: =%"
set "ORIG_CP=%ORIG_CP:.=%"
chcp 65001 >nul

set "LANG_DIR=%REPO_ROOT%build-lang"
set "SUPPORTED_LANGS=de en-us es fr it ja pt ru tr"
set "DEFAULT_LANG=en-us"

set "ADDRSIZE=64"
set "BUILD_DIR=build-vc-%ADDRSIZE%"
REM Release = official build (O2 + LTO). RelWithDebInfo on Windows uses /Od (no optimization).
set "CONFIG=Release"
set "CMAKE_BASE_FLAGS=-DUSE_OPENAL:BOOL=ON -DUSE_NVAPI=ON -DUSE_LTO=ON -DVS_DISABLE_FATAL_WARNINGS=ON -DREVISION_FROM_VCS=FALSE -DCMAKE_POLICY_VERSION_MINIMUM=3.5"
set "CMAKE_EXTRA_FLAGS=%CMAKE_BASE_FLAGS%"
set "SLN=%BUILD_DIR%\Allyn.sln"
set "SLNX=%BUILD_DIR%\Allyn.slnx"
set "VIEWER_EXE=%BUILD_DIR%\bin\%CONFIG%\allyn-viewer-bin.exe"
set "VIEWER_EXE_ALT=%BUILD_DIR%\bin\%CONFIG%\Allyn-bin.exe"
set "VIEWER_EXE_BUILD=%BUILD_DIR%\newview\%CONFIG%\allyn-viewer-bin.exe"
set "CACHE_DIR=%REPO_ROOT%.cache"
set "AUTOBUILD_CACHE=%CACHE_DIR%\autobuild"
set "REQUIREMENTS=%REPO_ROOT%requirements.txt"
REM The viewer uses crash-only logging: Allyn.log is only written on a crash.
REM A normal run writes marker files (Allyn.exec_marker) and static_debug_info.log here.
set "VIEWER_LOG_DIR=%APPDATA%\AllynViewer\logs"
set "VIEWER_LOG=%VIEWER_LOG_DIR%\Allyn.log"
set "MISSING=0"
set "AUTO_INSTALL=1"
REM Seconds the smoke test waits before checking that the viewer is still alive.
if not defined SMOKE_SECONDS set "SMOKE_SECONDS=45"

title Allyn Viewer - Build

REM Double click (no parameter): pick a language, then show the menu in the same process.
if "%~1"=="" (
  call :choose_lang
  if errorlevel 1 (
    pause
    call :restore_cp
    exit /b 1
  )
  call :menu
  if not "!MENU_QUIT!"=="1" (
    echo.
    echo ============================================================
    echo  !M_MENU_ABORTED_1!
    echo  !M_MENU_ABORTED_2!
    call :fmt M_MENU_ABORTED_3 "!errorlevel!"
    echo  !_F!
    echo ============================================================
    call :pause_key
  )
  call :restore_cp
  exit /b 0
)

REM Command-line mode: language from BUILD_LANG (default en-us), no prompt.
if not defined BUILD_LANG set "BUILD_LANG=%DEFAULT_LANG%"
call :load_lang
if errorlevel 1 (call :restore_cp & exit /b 1)

if /I "%~1"=="help" (call :show_help & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="check" (call :do_check & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="install" (call :do_install & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="configure" (call :do_configure_only & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="build" (call :do_build_only & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="viewer" (call :do_build_viewer & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="full" (call :do_full & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="rebuild" (call :do_rebuild & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="clean" (call :do_clean & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="run" (call :do_run & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="smoke" (call :do_smoke & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="package" (call :do_package & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="skins" (call :do_sync_skins & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="tools" (call :do_tools & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="13" (call :do_tools & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="1" (call :do_full & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="2" (call :do_build_viewer & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="3" (call :do_configure_only & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="4" (call :do_build_only & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="5" (call :do_rebuild & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="6" (call :do_check & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="7" (call :do_run & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="8" (call :do_clean & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="9" (call :show_help & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="10" (call :do_package & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="11" (call :do_sync_skins & call :cli_finish & exit /b !errorlevel!)
if /I "%~1"=="12" (call :do_smoke & call :cli_finish & exit /b !errorlevel!)
if not "%~1"=="" (
  call :fmt M_UNKNOWN_OPTION "%~1"
  echo !_F!
  call :show_help
  call :restore_cp
  exit /b 1
)
echo !M_INTERNAL_ERR!
call :restore_cp
exit /b 1

REM ============================================================================
REM  Language handling
REM ============================================================================

:choose_lang
cls
echo.
echo  ============================================================
echo   ALLYN VIEWER - BUILD (Windows x64)
echo  ============================================================
echo.
echo   Select language / Sprache wählen / Seleccione el idioma
echo   Choisissez la langue / Scegli la lingua / 言語を選択
echo   Escolha o idioma / Выберите язык / Dil seçin
echo.
echo   1 - Deutsch              (de)
echo   2 - English (US)         (en-us)
echo   3 - Español              (es)
echo   4 - Français             (fr)
echo   5 - Italiano             (it)
echo   6 - 日本語 / Japanese    (ja)
echo   7 - Português (Brasil)   (pt)
echo   8 - Русский              (ru)
echo   9 - Türkçe               (tr)
echo.
set "LCHOICE="
set /p "LCHOICE=[1-9] (Enter = 2): "
if "!LCHOICE!"=="" set "LCHOICE=2"
set "BUILD_LANG="
if "!LCHOICE!"=="1" set "BUILD_LANG=de"
if "!LCHOICE!"=="2" set "BUILD_LANG=en-us"
if "!LCHOICE!"=="3" set "BUILD_LANG=es"
if "!LCHOICE!"=="4" set "BUILD_LANG=fr"
if "!LCHOICE!"=="5" set "BUILD_LANG=it"
if "!LCHOICE!"=="6" set "BUILD_LANG=ja"
if "!LCHOICE!"=="7" set "BUILD_LANG=pt"
if "!LCHOICE!"=="8" set "BUILD_LANG=ru"
if "!LCHOICE!"=="9" set "BUILD_LANG=tr"
if not defined BUILD_LANG goto :choose_lang
call :load_lang
exit /b !errorlevel!

:load_lang
REM Loads build-lang\<BUILD_LANG>.cmd. Falls back to en-us for unknown codes.
set "LANG_VALID="
for %%L in (%SUPPORTED_LANGS%) do if /I "%%L"=="!BUILD_LANG!" set "LANG_VALID=1"
if not defined LANG_VALID (
  echo [WARNING] Unsupported BUILD_LANG "!BUILD_LANG!" - using %DEFAULT_LANG%. Supported: %SUPPORTED_LANGS%
  set "BUILD_LANG=%DEFAULT_LANG%"
)
set "LANG_FILE=%LANG_DIR%\!BUILD_LANG!.cmd"
if not exist "!LANG_FILE!" (
  echo [WARNING] Language file not found: !LANG_FILE! - using %DEFAULT_LANG%
  set "BUILD_LANG=%DEFAULT_LANG%"
  set "LANG_FILE=%LANG_DIR%\%DEFAULT_LANG%.cmd"
)
if not exist "!LANG_FILE!" (
  echo ERROR: !LANG_FILE! not found. The build-lang\ folder must sit next to build.bat.
  exit /b 1
)
set "M_FONT_HINT="
call "!LANG_FILE!"
exit /b 0

:fmt
REM Usage: call :fmt MSG_VAR [value0] [value1]  -> result in _F ({0}/{1} replaced)
set "_F=!%~1!"
if not "%~2"=="" set "_F=!_F:{0}=%~2!"
if not "%~3"=="" set "_F=!_F:{1}=%~3!"
exit /b 0

:pause_key
echo !M_PRESS_KEY!
pause >nul
exit /b 0

:restore_cp
if defined ORIG_CP chcp %ORIG_CP% >nul 2>&1
exit /b 0

REM ============================================================================
REM  Menu
REM ============================================================================

:menu
set "INTERACTIVE=1"
cls
echo.
echo  ============================================================
echo   !M_TITLE!
echo  ============================================================
if defined M_FONT_HINT echo  !M_FONT_HINT!
echo.
echo   1 - !M_MENU_1!
echo   2 - !M_MENU_2!
echo   3 - !M_MENU_3!
echo   4 - !M_MENU_4!
echo   5 - !M_MENU_5!
echo   6 - !M_MENU_6!
echo   7 - !M_MENU_7!
echo   8 - !M_MENU_8!
echo   9 - !M_MENU_9!
echo  10 - !M_MENU_10!
echo  11 - !M_MENU_11!
echo  12 - !M_MENU_12!
echo  13 - !M_MENU_13!
echo   L - !M_MENU_L! [!M_LANG_NAME!]
echo   0 - !M_MENU_0!
echo.
set "CHOICE="
set /p "CHOICE=!M_PROMPT! "
if "!CHOICE!"=="0" (
  echo.
  echo !M_EXITING!
  set "MENU_QUIT=1"
  exit /b 0
)
if /I "!CHOICE!"=="L" (
  call :choose_lang
  goto :menu
)
call :menu_dispatch "!CHOICE!"
goto :menu_return

:menu_dispatch
if "%~1"=="1" (call :do_full & exit /b !errorlevel!)
if "%~1"=="2" (call :do_build_viewer & exit /b !errorlevel!)
if "%~1"=="3" (call :do_configure_only & exit /b !errorlevel!)
if "%~1"=="4" (call :do_build_only & exit /b !errorlevel!)
if "%~1"=="5" (call :do_rebuild & exit /b !errorlevel!)
if "%~1"=="6" (call :do_check & exit /b !errorlevel!)
if "%~1"=="7" (call :do_run & exit /b !errorlevel!)
if "%~1"=="8" (call :do_clean & exit /b !errorlevel!)
if "%~1"=="9" (call :show_help & exit /b !errorlevel!)
if "%~1"=="10" (call :do_package & exit /b !errorlevel!)
if "%~1"=="11" (call :do_sync_skins & exit /b !errorlevel!)
if "%~1"=="12" (call :do_smoke & exit /b !errorlevel!)
if "%~1"=="13" (call :do_tools & exit /b !errorlevel!)
call :fmt M_INVALID_OPTION "%~1"
echo !_F!
exit /b 1

:menu_return
set "LASTRC=!errorlevel!"
if not "!INTERACTIVE!"=="1" exit /b !LASTRC!
if not "!LASTRC!"=="0" (
  echo.
  call :fmt M_CMD_FAILED "!LASTRC!"
  echo !_F!
)
echo.
echo ============================================================
call :pause_key
goto :menu

:cli_finish
set "CLI_RC=!errorlevel!"
if !CLI_RC! neq 0 (
  echo.
  call :fmt M_CMD_FAILED "!CLI_RC!"
  echo !_F!
)
call :restore_cp
exit /b !CLI_RC!

:show_help
echo.
echo !M_HELP_USAGE!
echo.
echo !M_HELP_COMMANDS!
echo   full       !M_HELP_FULL!
echo   viewer     !M_HELP_VIEWER!
echo   configure  !M_HELP_CONFIGURE!
echo   build      !M_HELP_BUILD!
echo   rebuild    !M_HELP_REBUILD!
echo   check      !M_HELP_CHECK!
echo   install    !M_HELP_INSTALL!
echo   run        !M_HELP_RUN!
call :fmt M_HELP_SMOKE "%SMOKE_SECONDS%"
echo   smoke      !_F!
echo   package    !M_HELP_PACKAGE!
echo   skins      !M_HELP_SKINS!
echo   tools      !M_HELP_TOOLS!
echo   clean      !M_HELP_CLEAN!
echo.
echo !M_HELP_FLOW!
echo   build.bat full      !M_HELP_FLOW_1!
echo   !M_HELP_FLOW_2!
echo   build.bat viewer    !M_HELP_FLOW_3!
echo   build.bat smoke     !M_HELP_FLOW_4!
echo.
echo !M_HELP_OUTPUT!
echo   %VIEWER_EXE%
echo !M_HELP_LOGS!
echo   %VIEWER_LOG_DIR%
echo !M_HELP_GUIDE!
echo !M_HELP_LANG!
echo.
exit /b 0

REM ============================================================================
REM  Environment / tool discovery
REM ============================================================================

:setup_env
REM autobuild locates Visual Studio through "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe".
REM Some shells (CI, embedded terminals) do not export ProgramFiles(x86); make sure it has a value.
if "%ProgramFiles(x86)%"=="" set "ProgramFiles(x86)=%SystemDrive%\Program Files (x86)"
set "PATH=%LOCALAPPDATA%\Programs\Python\Launcher;%APPDATA%\Python\Python314\Scripts;%APPDATA%\Python\Python312\Scripts;%LOCALAPPDATA%\Programs\Python\Python314\Scripts;%LOCALAPPDATA%\Packages\PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0\LocalCache\local-packages\Python312\Scripts;C:\Python314\Scripts;C:\Program Files\CMake\bin;C:\Program Files\Git\cmd;%PATH%"
call :find_python
if defined PYTHON_EXE (
  for %%D in ("!PYTHON_EXE!") do set "PATH=%%~dpD;%%~dpDScripts;!PATH!"
)
call :find_vs
if defined MSBUILD_EXE (
  for %%D in ("!MSBUILD_EXE!") do set "PATH=%%~dpD;!PATH!"
)
call :find_mt
set "AUTOBUILD_INSTALLABLE_CACHE=%AUTOBUILD_CACHE%"
if not defined AUTOBUILD_VSVER set "AUTOBUILD_VSVER=170"
if not defined AUTOBUILD_WIN_CMAKE_GEN set "AUTOBUILD_WIN_CMAKE_GEN=Visual Studio 17 2022"
set "AUTOBUILD_WIN_VSPLATFORM=x64"
set "AUTOBUILD_WIN_VSHOST=x64"
set "AUTOBUILD_ADDRSIZE=%ADDRSIZE%"
if not exist "%CACHE_DIR%" mkdir "%CACHE_DIR%" 2>nul
if not exist "%AUTOBUILD_CACHE%" mkdir "%AUTOBUILD_CACHE%" 2>nul
exit /b 0

:find_mt
REM mt.exe (Manifest Tool) is required by the SLPlugin/viewer POST_BUILD step; VS 18 does not always expose it in PATH.
set "MT_EXE="
where mt >nul 2>&1
if not errorlevel 1 (
  for /f "delims=" %%M in ('where mt 2^>nul') do (
    if not defined MT_EXE set "MT_EXE=%%M"
  )
)
if not defined MT_EXE if defined WindowsSdkVerBinPath if exist "%WindowsSdkVerBinPath%x64\mt.exe" set "MT_EXE=%WindowsSdkVerBinPath%x64\mt.exe"
if not defined MT_EXE (
  for /f "delims=" %%D in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*" 2^>nul') do (
    if not defined MT_EXE if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%D\x64\mt.exe" set "MT_EXE=%ProgramFiles(x86)%\Windows Kits\10\bin\%%D\x64\mt.exe"
  )
)
if not defined MT_EXE (
  for /f "delims=" %%D in ('dir /b /ad /o-n "%ProgramFiles%\Windows Kits\10\bin\10.*" 2^>nul') do (
    if not defined MT_EXE if exist "%ProgramFiles%\Windows Kits\10\bin\%%D\x64\mt.exe" set "MT_EXE=%ProgramFiles%\Windows Kits\10\bin\%%D\x64\mt.exe"
  )
)
if defined MT_EXE (
  for %%D in ("!MT_EXE!") do set "PATH=%%~dpD;!PATH!"
)
exit /b 0

:find_sln
if exist "%BUILD_DIR%\Allyn.sln" (
  set "SLN=%BUILD_DIR%\Allyn.sln"
  exit /b 0
)
if exist "%BUILD_DIR%\Allyn.slnx" (
  set "SLN=%BUILD_DIR%\Allyn.slnx"
  exit /b 0
)
exit /b 1

:is_python_stub
REM Rejects the Microsoft Store shortcut (WindowsApps\python.exe), which is not an interpreter.
set "_STUB_CAND=%~1"
if /I "%~nx1"=="python.exe" goto :is_python_stub_check_parent
if /I "%~nx1"=="python3.exe" goto :is_python_stub_check_parent
exit /b 1
:is_python_stub_check_parent
echo %~dp1 | findstr /I /E /C:"\WindowsApps\" >nul
if not errorlevel 1 exit /b 0
exit /b 1

:accept_python
set "_CAND=%~1"
if not exist "%_CAND%" exit /b 1
call :is_python_stub "%_CAND%"
if not errorlevel 1 exit /b 1
"%_CAND%" -c "import encodings" >nul 2>&1
if errorlevel 1 exit /b 1
set "PYTHON_EXE=%_CAND%"
exit /b 0

:try_py_ver
if defined PYTHON_EXE exit /b 0
for /f "delims=" %%P in ('py -%~1 -c "import sys; print(sys.executable)" 2^>nul') do (
  if not defined PYTHON_EXE call :accept_python "%%P"
)
exit /b 0

:find_python
set "PYTHON_EXE="
REM Prefer the python.org "py" launcher. Every candidate is validated with
REM "import encodings" to discard broken installs without a stdlib.
call :try_py_ver 3.12
call :try_py_ver 3.13
call :try_py_ver 3.11
call :try_py_ver 3.10
if defined PYTHON_EXE goto :find_python_scripts
for %%D in (
  "%LOCALAPPDATA%\Programs\Python\Python314"
  "%LOCALAPPDATA%\Programs\Python\Python313"
  "%LOCALAPPDATA%\Programs\Python\Python312"
  "%LOCALAPPDATA%\Programs\Python\Python311"
  "C:\Python314"
  "C:\Python313"
  "C:\Python312"
) do (
  if not defined PYTHON_EXE if exist "%%~D\python.exe" if exist "%%~D\Lib\os.py" call :accept_python "%%~D\python.exe"
)
if defined PYTHON_EXE goto :find_python_scripts
call :try_py_ver 3
if defined PYTHON_EXE goto :find_python_scripts
for /f "delims=" %%P in ('where python 2^>nul') do (
  if not defined PYTHON_EXE call :accept_python "%%P"
)
:find_python_scripts
set "PYTHON_SCRIPTS="
if defined PYTHON_EXE (
  "!PYTHON_EXE!" -c "import sysconfig; print(sysconfig.get_path('scripts'))" > "%TEMP%\allyn_pyscripts.txt" 2>nul
  if exist "%TEMP%\allyn_pyscripts.txt" (
    set /p PYTHON_SCRIPTS=<"%TEMP%\allyn_pyscripts.txt"
    del "%TEMP%\allyn_pyscripts.txt" >nul 2>&1
  )
  if defined PYTHON_SCRIPTS set "PATH=!PYTHON_SCRIPTS!;!PATH!"
)
exit /b 0

:find_vs
set "VS_OK=0"
set "VS_INSTALL="
set "MSBUILD_EXE="
set "AUTOBUILD_VSVER=170"
set "AUTOBUILD_WIN_CMAKE_GEN=Visual Studio 17 2022"
REM Prefer VS 2022 (same as the official CI); otherwise use VS 2026 (18).
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
if not defined VS_INSTALL if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
if not defined VS_INSTALL if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise"
if not defined VS_INSTALL if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools"
REM VS 2022 Build Tools (winget / standalone installer) live under Program Files (x86).
if not defined VS_INSTALL if exist "!ProgramFiles(x86)!\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC" set "VS_INSTALL=!ProgramFiles(x86)!\Microsoft Visual Studio\2022\BuildTools"
if defined VS_INSTALL (
  set "AUTOBUILD_VSVER=170"
  set "AUTOBUILD_WIN_CMAKE_GEN=Visual Studio 17 2022"
  goto :find_vs_msbuild
)
if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\18\Community"
if not defined VS_INSTALL if exist "%ProgramFiles%\Microsoft Visual Studio\18\Professional\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\18\Professional"
if not defined VS_INSTALL if exist "%ProgramFiles%\Microsoft Visual Studio\18\Enterprise\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\18\Enterprise"
if not defined VS_INSTALL if exist "%ProgramFiles%\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC" set "VS_INSTALL=%ProgramFiles%\Microsoft Visual Studio\18\BuildTools"
if not defined VS_INSTALL if exist "!ProgramFiles(x86)!\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC" set "VS_INSTALL=!ProgramFiles(x86)!\Microsoft Visual Studio\18\BuildTools"
if defined VS_INSTALL (
  set "AUTOBUILD_VSVER=180"
  set "AUTOBUILD_WIN_CMAKE_GEN=Visual Studio 18 2026"
)
:find_vs_msbuild
if not defined VS_INSTALL exit /b 0
set "VS_OK=1"
if exist "!VS_INSTALL!\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD_EXE=!VS_INSTALL!\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD_EXE (
  for /f "delims=" %%M in ('where msbuild 2^>nul') do (
    if not defined MSBUILD_EXE set "MSBUILD_EXE=%%M"
  )
)
exit /b 0

:get_revision
set "REVISION=0"
for /f %%R in ('git rev-list --count HEAD 2^>nul') do set "REVISION=%%R"
if not defined REVISION set "REVISION=0"
set "revision=%REVISION%"
set "AUTOBUILD_BUILD_ID=%REVISION%"
exit /b 0

:find_autobuild
set "AUTOBUILD_EXE="
if defined PYTHON_SCRIPTS if exist "!PYTHON_SCRIPTS!\autobuild.exe" set "AUTOBUILD_EXE=!PYTHON_SCRIPTS!\autobuild.exe"
if not defined AUTOBUILD_EXE if exist "%LOCALAPPDATA%\Packages\PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0\LocalCache\local-packages\Python312\Scripts\autobuild.exe" set "AUTOBUILD_EXE=%LOCALAPPDATA%\Packages\PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0\LocalCache\local-packages\Python312\Scripts\autobuild.exe"
if not defined AUTOBUILD_EXE if exist "%APPDATA%\Python\Python312\Scripts\autobuild.exe" set "AUTOBUILD_EXE=%APPDATA%\Python\Python312\Scripts\autobuild.exe"
if not defined AUTOBUILD_EXE if exist "%APPDATA%\Python\Python314\Scripts\autobuild.exe" set "AUTOBUILD_EXE=%APPDATA%\Python\Python314\Scripts\autobuild.exe"
if not defined AUTOBUILD_EXE if exist "%LOCALAPPDATA%\Programs\Python\Python314\Scripts\autobuild.exe" set "AUTOBUILD_EXE=%LOCALAPPDATA%\Programs\Python\Python314\Scripts\autobuild.exe"
if not defined AUTOBUILD_EXE (
  for /f "delims=" %%A in ('where autobuild 2^>nul') do (
    if not defined AUTOBUILD_EXE set "AUTOBUILD_EXE=%%A"
  )
)
exit /b 0

REM ============================================================================
REM  Requirement checks
REM ============================================================================

:do_check
call :setup_env
set "MISSING=0"
echo.
echo  !M_CHECK_HEADER!
echo.
call :dep_check_python
call :dep_check_pip_pkg autobuild
call :dep_check_pip_pkg llsd
call :dep_check_autobuild_version
call :dep_check_git
call :dep_check_cmake
call :dep_check_vs
call :dep_check_msbuild
call :dep_check_nsis
call :dep_check_repo
call :dep_check_path
call :dep_check_disk
REM Tools that "build.bat tools" can install with winget (pip packages are handled by "install").
set /a TOOLS_MISSING=0
if not defined PYTHON_EXE set /a TOOLS_MISSING+=1
if not "!GIT_OK!"=="1" set /a TOOLS_MISSING+=1
if not "!CMAKE_OK!"=="1" set /a TOOLS_MISSING+=1
if not "!VS_OK!"=="1" set /a TOOLS_MISSING+=1
echo.
if !MISSING! GTR 0 (
  call :fmt M_CHECK_RESULT_MISSING "!MISSING!"
  echo  !_F!
  echo  !M_CHECK_RUN_INSTALL!
  call :has_winget
  if not errorlevel 1 if !TOOLS_MISSING! GTR 0 echo  !M_TOOLS_CLI_HINT!
  exit /b 1
)
echo  !M_CHECK_ALL_OK!
exit /b 0

:dep_check_path
REM Warnings only: paths with spaces, very long paths and cloud-synced folders
REM are the most common causes of "mysterious" build failures.
set "_RP=%REPO_ROOT:~0,-1%"
if not "!_RP: =!"=="!_RP!" echo   !M_WARN! !M_PATH_SPACE!
set "_LEN=0"
call :strlen _RP _LEN
if !_LEN! GTR 60 (
  call :fmt M_PATH_LONG "!_LEN!"
  echo   !M_WARN! !_F!
)
set "_CLOUD="
if not "!_RP:OneDrive=!"=="!_RP!" set "_CLOUD=1"
if not "!_RP:Dropbox=!"=="!_RP!" set "_CLOUD=1"
if not "!_RP:Google Drive=!"=="!_RP!" set "_CLOUD=1"
if defined _CLOUD echo   !M_WARN! !M_PATH_ONEDRIVE!
exit /b 0

:strlen
REM call :strlen VAR_NAME RESULT_VAR  -> length of !VAR_NAME! into RESULT_VAR
set "_S=!%~1!#"
set "_N=0"
for %%P in (4096 2048 1024 512 256 128 64 32 16 8 4 2 1) do (
  if "!_S:~%%P,1!" neq "" (
    set /a "_N+=%%P"
    set "_S=!_S:~%%P!"
  )
)
set "%~2=!_N!"
exit /b 0

:dep_check_disk
set "FREE_GB="
for /f "delims=" %%G in ('powershell -NoProfile -ExecutionPolicy Bypass -Command "[math]::Floor((Get-Item -LiteralPath '%REPO_ROOT%.').PSDrive.Free / 1GB)" 2^>nul') do set "FREE_GB=%%G"
if not defined FREE_GB exit /b 0
set "_DRV=%REPO_ROOT:~0,2%"
if !FREE_GB! LSS 20 (
  call :fmt M_DISK_LOW "!_DRV!" "!FREE_GB!"
  echo   !M_WARN! !_F!
) else (
  call :fmt M_DISK_FREE "!_DRV!" "!FREE_GB!"
  echo   !M_OK! !_F!
)
exit /b 0

:has_winget
where winget >nul 2>&1
if not errorlevel 1 exit /b 0
if exist "%LOCALAPPDATA%\Microsoft\WindowsApps\winget.exe" exit /b 0
exit /b 1

:dep_check_python
if not defined PYTHON_EXE call :find_python
if defined PYTHON_EXE (
  for /f "delims=" %%V in ('"!PYTHON_EXE!" --version 2^>^&1') do echo   !M_OK! Python: %%V - !PYTHON_EXE!
) else (
  echo   !M_MISSING! !M_PY_MISSING!
  echo           !M_PY_MISSING_2!
  echo           !M_PY_MISSING_3!
  set /a MISSING+=1
)
exit /b 0

:dep_check_pip_pkg
set "PKG_NAME=%~1"
set "PKG_OK=0"
call :fmt M_PIP_PKG "%PKG_NAME%"
if not defined PYTHON_EXE (
  echo   !M_MISSING! !_F!  !M_PY_UNAVAILABLE!
  set /a MISSING+=1
  exit /b 0
)
"!PYTHON_EXE!" -m pip show %PKG_NAME% >nul 2>&1
if !errorlevel! equ 0 (
  echo   !M_OK! pip %PKG_NAME%
  set "PKG_OK=1"
) else (
  echo   !M_MISSING! !_F!
  echo           !M_PIP_AUTOINSTALL!
  set /a MISSING+=1
)
exit /b 0

:dep_check_autobuild_version
call :find_autobuild
if not defined AUTOBUILD_EXE exit /b 0
for /f "delims=" %%V in ('"!AUTOBUILD_EXE!" --version 2^>nul') do echo   !M_OK! %%V - !AUTOBUILD_EXE!
exit /b 0

:dep_check_git
set "GIT_OK=0"
for /f "delims=" %%G in ('where git 2^>nul') do set "GIT_OK=1"
if !GIT_OK! equ 1 (
  for /f "delims=" %%V in ('git --version 2^>nul') do echo   !M_OK! %%V
) else (
  echo   !M_MISSING! !M_GIT_MISSING!
  call :fmt M_INSTALL_FROM "https://git-scm.com/download/win"
  echo           !_F!
  set /a MISSING+=1
)
exit /b 0

:dep_check_cmake
set "CMAKE_OK=0"
set "CMAKE_EXE="
if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
if not defined CMAKE_EXE (
  for /f "delims=" %%C in ('where cmake 2^>nul') do (
    if not defined CMAKE_EXE set "CMAKE_EXE=%%C"
  )
)
if defined CMAKE_EXE (
  for /f "delims=" %%V in ('cmake --version 2^>nul ^| findstr /B "cmake"') do echo   !M_OK! %%V
  set "CMAKE_OK=1"
) else (
  echo   !M_MISSING! !M_CMAKE_MISSING!
  call :fmt M_INSTALL_FROM "https://cmake.org/download/"
  echo           !_F!
  set /a MISSING+=1
)
exit /b 0

:dep_check_vs
if not defined VS_INSTALL call :find_vs
if "!VS_OK!"=="1" (
  call :fmt M_VS_OK "!VS_INSTALL!"
  echo   !M_OK! !_F!
  call :fmt M_VS_GEN "!AUTOBUILD_WIN_CMAKE_GEN!" "!AUTOBUILD_VSVER!"
  echo         !_F!
) else (
  echo   !M_MISSING! !M_VS_MISSING!
  echo           !M_VS_MISSING_2!
  set /a MISSING+=1
)
exit /b 0

:dep_check_msbuild
if not defined MSBUILD_EXE call :find_vs
if defined MSBUILD_EXE (
  echo   !M_OK! MSBuild: !MSBUILD_EXE!
  set "MSBUILD_OK=1"
) else (
  echo   !M_MISSING! !M_MSBUILD_MISSING!
  set /a MISSING+=1
)
exit /b 0

:dep_check_nsis
set "NSIS_OK=0"
set "NSIS_X86=!ProgramFiles(x86)!\NSIS\makensis.exe"
set "NSIS_PF=%ProgramFiles%\NSIS\makensis.exe"
if exist "!NSIS_X86!" (
  echo   !M_OK! NSIS: !NSIS_X86!
  set "NSIS_OK=1"
) else if exist "!NSIS_PF!" (
  echo   !M_OK! NSIS: !NSIS_PF!
  set "NSIS_OK=1"
) else (
  echo   !M_WARN! !M_NSIS_MISSING!
  echo           !M_NSIS_MISSING_2!
)
exit /b 0

:dep_check_repo
if not exist "%REPO_ROOT%autobuild.xml" (
  echo   !M_ERR! !M_REPO_NO_AUTOBUILD_XML!
  set /a MISSING+=1
) else (
  echo   !M_OK! autobuild.xml
)
if not exist "%REPO_ROOT%indra" (
  echo   !M_ERR! !M_REPO_NO_INDRA!
  set /a MISSING+=1
) else (
  echo   !M_OK! !M_REPO_INDRA!
)
if exist "%SLN%" (
  call :fmt M_SOLUTION "%SLN%"
  echo   !M_OK! !_F!
) else if exist "%SLNX%" (
  call :fmt M_SOLUTION "%SLNX%"
  echo   !M_OK! !_F!
) else (
  echo   !M_INFO! !M_SOLUTION_NOT_GENERATED!
)
if exist "%VIEWER_EXE%" (
  call :fmt M_EXECUTABLE "%VIEWER_EXE%"
  echo   !M_OK! !_F!
) else if exist "%VIEWER_EXE_ALT%" (
  call :fmt M_EXECUTABLE "%VIEWER_EXE_ALT%"
  echo   !M_OK! !_F!
) else (
  echo   !M_INFO! !M_EXE_NOT_BUILT!
)
exit /b 0

REM ============================================================================
REM  Install / configure / build
REM ============================================================================

:do_install
call :setup_env
echo.
echo  !M_INSTALL_HEADER!
echo.
if not defined PYTHON_EXE (
  echo !M_INSTALL_NO_PY_1!
  echo !M_INSTALL_NO_PY_2!
  echo !M_INSTALL_NO_PY_3!
  exit /b 1
)
call :fmt M_USING_PYTHON "!PYTHON_EXE!"
echo !_F!
"!PYTHON_EXE!" -m pip install --upgrade pip
if errorlevel 1 (
  echo !M_PIP_UPGRADE_WARN!
)
REM Same versions as the CI (Linden Lab autobuild with .tar.zst support + llsd).
if exist "%REQUIREMENTS%" (
  echo !M_INSTALL_REQ!
  "!PYTHON_EXE!" -m pip install --upgrade -r "%REQUIREMENTS%"
) else (
  echo !M_INSTALL_REQ_MISSING!
  "!PYTHON_EXE!" -m pip install --upgrade "autobuild>=3.10.0" "llsd>=1.2.0"
)
if errorlevel 1 (
  echo !M_INSTALL_PIP_FAIL!
  exit /b 1
)
echo.
echo !M_INSTALL_PY_DONE!
call :find_autobuild
if defined AUTOBUILD_EXE echo autobuild: !AUTOBUILD_EXE!
call :setup_nsis_plugins
echo.
echo !M_INSTALL_DONE!
exit /b 0

:setup_nsis_plugins
REM The installer template needs the StdUtils and INetC plugins, which stock NSIS
REM does not ship. Only runs when NSIS itself is installed.
if not "!NSIS_OK!"=="1" call :dep_check_nsis
if not "!NSIS_OK!"=="1" exit /b 0
if not exist "%REPO_ROOT%scripts\setup-nsis-plugins.ps1" exit /b 0
echo !M_NSIS_PLUGINS!
powershell -NoProfile -ExecutionPolicy Bypass -File "%REPO_ROOT%scripts\setup-nsis-plugins.ps1"
exit /b 0

:offer_tools
REM Called by ensure_ready when Git/CMake/Python/Visual Studio are missing.
REM Interactive: ask before installing with winget. CLI: only print the hint.
call :has_winget
if errorlevel 1 (
  echo.
  echo !M_TOOLS_NO_WINGET!
  exit /b 1
)
REM CLI mode: do_check already printed M_TOOLS_CLI_HINT.
if not "!INTERACTIVE!"=="1" exit /b 1
echo.
set "ANS="
set /p "ANS=!M_TOOLS_ASK! "
set "_YES="
for %%K in (!M_YES_KEYS!) do if /I "!ANS!"=="%%K" set "_YES=1"
if not defined _YES exit /b 1
call :do_tools
exit /b !errorlevel!

:do_tools
REM Installs the missing build tools with winget (Windows 10 1809+ / Windows 11).
REM Python packages are NOT handled here (see :do_install).
call :setup_env
echo.
echo  !M_TOOLS_HEADER!
echo.
call :has_winget
if errorlevel 1 (
  echo !M_TOOLS_NO_WINGET!
  exit /b 1
)
set "WINGET_ARGS=-e --accept-source-agreements --accept-package-agreements --disable-interactivity"
set "TOOLS_INSTALLED=0"
set "TOOLS_FAILED=0"
REM Git
set "GIT_OK=0"
for /f "delims=" %%G in ('where git 2^>nul') do set "GIT_OK=1"
if not "!GIT_OK!"=="1" call :winget_install "Git" Git.Git
REM CMake
set "CMAKE_OK=0"
if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_OK=1"
if not "!CMAKE_OK!"=="1" for /f "delims=" %%C in ('where cmake 2^>nul') do set "CMAKE_OK=1"
if not "!CMAKE_OK!"=="1" call :winget_install "CMake" Kitware.CMake
REM Python (python.org build, per-user install under %LOCALAPPDATA%\Programs\Python)
if not defined PYTHON_EXE call :find_python
if not defined PYTHON_EXE call :winget_install "Python 3.12" Python.Python.3.12
REM Visual Studio 2022 Build Tools with the C++ desktop workload (large download, UAC prompt)
if not defined VS_INSTALL call :find_vs
if not "!VS_OK!"=="1" (
  echo !M_TOOLS_VS_NOTE!
  call :winget_install "Visual Studio 2022 Build Tools (C++)" Microsoft.VisualStudio.2022.BuildTools "--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
)
REM NSIS (optional - needed only for the .exe installer) plus the StdUtils/INetC plugins.
set "NSIS_OK=0"
if exist "!ProgramFiles(x86)!\NSIS\makensis.exe" set "NSIS_OK=1"
if exist "%ProgramFiles%\NSIS\makensis.exe" set "NSIS_OK=1"
if not "!NSIS_OK!"=="1" call :winget_install "NSIS" NSIS.NSIS
set "NSIS_OK=0"
call :setup_nsis_plugins
echo.
if !TOOLS_FAILED! GTR 0 exit /b 1
if !TOOLS_INSTALLED! equ 0 (
  echo !M_TOOLS_NOTHING!
) else (
  echo !M_TOOLS_DONE!
)
REM Pick up the freshly installed tools in this session.
call :setup_env
exit /b 0

:winget_install
REM call :winget_install "Display name" Package.Id ["installer override args"]
call :fmt M_TOOLS_INSTALLING "%~1"
echo !_F!
if "%~3"=="" (
  winget install --id %~2 %WINGET_ARGS%
) else (
  winget install --id %~2 %WINGET_ARGS% --override "%~3"
)
set "RC=!errorlevel!"
REM 0 = installed; -1978335189 / -1978335135 = already installed / no applicable update.
if !RC! equ 0 (
  set /a TOOLS_INSTALLED+=1
  exit /b 0
)
if !RC! equ -1978335189 exit /b 0
if !RC! equ -1978335135 exit /b 0
call :fmt M_TOOLS_FAILED "%~1" "!RC!"
echo !M_WARN! !_F!
set /a TOOLS_FAILED+=1
exit /b 1

:ensure_ready
call :setup_env
call :do_check
if errorlevel 1 (
  if not "!AUTO_INSTALL!"=="1" exit /b 1
  REM 1) Tools (Git, CMake, Python, Visual Studio Build Tools) via winget, with confirmation.
  if !TOOLS_MISSING! GTR 0 call :offer_tools
  REM 2) Python packages (autobuild, llsd) via pip.
  echo.
  echo !M_INSTALLING_MISSING!
  call :do_install
  set "MISSING=0"
  call :do_check
  if errorlevel 1 exit /b 1
)
call :find_autobuild
if not defined AUTOBUILD_EXE (
  echo !M_AUTOBUILD_NOT_FOUND!
  exit /b 1
)
call :get_revision
echo.
call :fmt M_BUILD_ID "!REVISION!"
echo !_F!
exit /b 0

:do_configure_only
call :ensure_ready
if errorlevel 1 exit /b 1
echo.
echo  !M_CONFIGURE_HEADER!
echo.
"%AUTOBUILD_EXE%" configure -A %ADDRSIZE% -i !REVISION! -c %CONFIG% -- %CMAKE_EXTRA_FLAGS%
set "RC=!errorlevel!"
if !RC! neq 0 (
  call :fmt M_CONFIGURE_FAIL "!RC!"
  echo !_F!
  exit /b !RC!
)
call :find_sln
if not errorlevel 1 (
  echo.
  call :fmt M_CONFIGURE_OK "!SLN!"
  echo !_F!
) else (
  echo !M_SLN_NOT_GENERATED!
  exit /b 1
)
exit /b 0

:do_build_only
call :ensure_ready
if errorlevel 1 exit /b 1
call :find_sln
if errorlevel 1 (
  echo !M_SLN_MISSING_CONFIGURE!
  call :do_configure_only
  if errorlevel 1 exit /b 1
)
echo.
echo  !M_BUILD_HEADER!
echo  !M_BUILD_TAKES_TIME!
echo.
"%AUTOBUILD_EXE%" build --no-configure -A %ADDRSIZE% -i !REVISION! -c %CONFIG%
set "RC=!errorlevel!"
if !RC! neq 0 (
  call :fmt M_BUILD_FAIL "!RC!"
  echo !_F!
  exit /b !RC!
)
call :show_build_result
exit /b 0

:do_build_viewer
call :ensure_ready
if errorlevel 1 exit /b 1
call :find_sln
if errorlevel 1 (
  echo !M_SLN_MISSING_CONFIGURE!
  call :do_configure_only
  if errorlevel 1 exit /b 1
)
echo.
echo  !M_VIEWER_HEADER!
echo.
set "VIEWER_PROJ=%BUILD_DIR%\newview\allyn-viewer-bin.vcxproj"
if not exist "%VIEWER_PROJ%" (
  call :fmt M_ERR_NOT_FOUND "%VIEWER_PROJ%"
  echo !_F!
  exit /b 1
)
REM Uses autobuild itself (same vcvars environment as "build.bat build") and passes
REM "--target" to the "cmake --build" defined in autobuild.xml. Calling MSBuild
REM directly from a plain shell forces a full recompile every time you switch
REM between "build" and "viewer".
"%AUTOBUILD_EXE%" build --no-configure -A %ADDRSIZE% -i !REVISION! -c %CONFIG% -- --target allyn-viewer-bin
set "RC=!errorlevel!"
if !RC! neq 0 (
  call :fmt M_VIEWER_FAIL "!RC!"
  echo !_F!
  exit /b !RC!
)
call :show_build_result
exit /b 0

:do_full
echo.
echo  ============================================================
echo   !M_FULL_HEADER!
echo  ============================================================
call :do_clean
if errorlevel 1 exit /b 1
call :ensure_ready
if errorlevel 1 exit /b 1
call :do_configure_only
if errorlevel 1 exit /b 1
call :do_build_only
exit /b !errorlevel!

:do_rebuild
echo.
echo  !M_REBUILD_HEADER!
call :do_clean
if errorlevel 1 exit /b 1
call :do_full
exit /b !errorlevel!

:do_clean
echo.
call :fmt M_CLEAN_REMOVING "%BUILD_DIR%"
echo !_F!
if exist "%BUILD_DIR%" (
  rmdir /s /q "%BUILD_DIR%"
  if exist "%BUILD_DIR%" (
    call :fmt M_CLEAN_FAIL "%BUILD_DIR%"
    echo !_F!
    exit /b 1
  )
  echo !M_CLEAN_DONE!
) else (
  echo !M_CLEAN_NOTHING!
)
exit /b 0

:show_build_result
call :sync_runtime
echo.
set "RESULT_EXE="
if exist "%VIEWER_EXE%" set "RESULT_EXE=%VIEWER_EXE%"
if not defined RESULT_EXE if exist "%VIEWER_EXE_BUILD%" set "RESULT_EXE=%VIEWER_EXE_BUILD%"
if not defined RESULT_EXE if exist "%VIEWER_EXE_ALT%" set "RESULT_EXE=%VIEWER_EXE_ALT%"
if defined RESULT_EXE (
  echo  ============================================================
  echo   !M_BUILD_SUCCESS!
  echo  ============================================================
  call :fmt M_EXECUTABLE "!RESULT_EXE!"
  echo   !_F!
  for %%F in ("!RESULT_EXE!") do (
    call :fmt M_SIZE "%%~zF"
    echo   !_F!
  )
  echo.
  echo   !M_TO_TEST!
  echo  ============================================================
) else (
  echo !M_BUILD_NO_EXE!
  echo   %VIEWER_EXE%
  echo !M_CHECK_LOGS!
)
exit /b 0

:sync_runtime
set "RUNTIME_SRC=%BUILD_DIR%\newview\%CONFIG%"
set "RUNTIME_DST=%BUILD_DIR%\bin\%CONFIG%"
if not exist "%RUNTIME_SRC%\allyn-viewer-bin.exe" exit /b 0
if not exist "%RUNTIME_DST%" mkdir "%RUNTIME_DST%"
call :fmt M_SYNC_RUNTIME "%RUNTIME_SRC%" "%RUNTIME_DST%"
echo !_F!
xcopy /E /Y /I /Q "%RUNTIME_SRC%\*" "%RUNTIME_DST%\" >nul 2>&1
REM llwebrtc.dll is staged to sharedlibs; ensure both runtime dirs get the fresh build.
if exist "%BUILD_DIR%\sharedlibs\%CONFIG%\llwebrtc.dll" (
  copy /Y "%BUILD_DIR%\sharedlibs\%CONFIG%\llwebrtc.dll" "%RUNTIME_SRC%\llwebrtc.dll" >nul
  copy /Y "%BUILD_DIR%\sharedlibs\%CONFIG%\llwebrtc.dll" "%RUNTIME_DST%\llwebrtc.dll" >nul
  echo !M_WEBRTC_UPDATED!
) else if exist "%BUILD_DIR%\llwebrtc\%CONFIG%\llwebrtc.dll" (
  copy /Y "%BUILD_DIR%\llwebrtc\%CONFIG%\llwebrtc.dll" "%RUNTIME_SRC%\llwebrtc.dll" >nul
  copy /Y "%BUILD_DIR%\llwebrtc\%CONFIG%\llwebrtc.dll" "%RUNTIME_DST%\llwebrtc.dll" >nul
  echo !M_WEBRTC_UPDATED!
)
exit /b 0

:do_sync_skins
call :setup_env
if not exist "%BUILD_DIR%\bin\%CONFIG%" (
  echo !M_SKINS_NO_BUILD!
  exit /b 1
)
echo.
echo  !M_SKINS_HEADER!
echo.
set "SKINS_SRC=%REPO_ROOT%indra\newview\skins"
set "SKINS_DST_BUILD=%BUILD_DIR%\newview\%CONFIG%\skins"
set "SKINS_DST_BIN=%BUILD_DIR%\bin\%CONFIG%\skins"
if not exist "!SKINS_SRC!\default" (
  call :fmt M_SKINS_NO_SRC "!SKINS_SRC!"
  echo !_F!
  exit /b 1
)
for %%D in ("!SKINS_DST_BUILD!" "!SKINS_DST_BIN!") do (
  if exist %%D rmdir /s /q %%D
  mkdir %%D
  xcopy /E /Y /I /Q "!SKINS_SRC!\default" "%%D\default\" >nul 2>&1
  xcopy /E /Y /I /Q "!SKINS_SRC!\cyber" "%%D\cyber\" >nul 2>&1
  copy /Y "!SKINS_SRC!\Cyber.xml" "%%D\" >nul 2>&1
  copy /Y "!SKINS_SRC!\paths.xml" "%%D\" >nul 2>&1
)
call :fmt M_SKINS_COPIED_FROM "!SKINS_SRC!"
echo !_F!
call :fmt M_SKINS_TO "!SKINS_DST_BUILD!"
echo !_F!
call :fmt M_SKINS_TO "!SKINS_DST_BIN!"
echo !_F!
echo.
echo !M_SKINS_DONE!
exit /b 0

:do_package
call :ensure_ready
if errorlevel 1 exit /b 1
call :find_sln
if errorlevel 1 (
  echo !M_SLN_MISSING_RUN!
  exit /b 1
)
echo.
echo  !M_PACKAGE_HEADER!
echo.
if not exist "%VIEWER_EXE_BUILD%" (
  echo !M_PACKAGE_BUILD_FIRST!
  call :do_build_viewer
  if errorlevel 1 exit /b 1
)
call :read_version
if errorlevel 1 (
  echo !M_PACKAGE_NO_VERSION!
  exit /b 1
)
call :dep_check_nsis
echo.
echo !M_PACKAGE_RUNNING!
REM viewer_manifest.py stages the files next to the build output (same as the
REM CMake "llpackage" target), builds the portable ZIP and, when NSIS is present,
REM the _Setup.exe. branding_id and channel must match Variables.cmake
REM (VIEWER_BRANDING_ID = allyn-viewer, VIEWER_CHANNEL = "Allyn Viewer <type>").
set "MANIFEST_PKG_DIR=%BUILD_DIR%\newview\%CONFIG%"
set "DIST_DIR=%BUILD_DIR%\dist"
REM Always repackage: an old installer left in the staging dir makes the manifest skip packaging.
del /q "!MANIFEST_PKG_DIR!\*_x86_64_Setup.exe" "!MANIFEST_PKG_DIR!\*_x86_64.zip" >nul 2>&1
"!PYTHON_EXE!" "%REPO_ROOT%indra\newview\viewer_manifest.py" --arch=x86_64 --artwork= --branding_id=allyn-viewer --build="%REPO_ROOT%%BUILD_DIR%/newview" --buildtype=%CONFIG% "--channel=Allyn Viewer !CHANNEL_TYPE!" --versionfile="%REPO_ROOT%%BUILD_DIR%/newview/viewer_version.txt" --configuration="%REPO_ROOT%!MANIFEST_PKG_DIR!" --dest="%REPO_ROOT%!MANIFEST_PKG_DIR!" --grid=agni --source="%REPO_ROOT%indra/newview" --touch="%REPO_ROOT%!MANIFEST_PKG_DIR!/touched.bat"
set "RC=!errorlevel!"
if !RC! neq 0 (
  echo !M_PACKAGE_FAIL!
  exit /b !RC!
)
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
set "ZIP_FOUND="
set "INSTALLER_FOUND="
for %%F in ("!MANIFEST_PKG_DIR!\*_!VER_US!_x86_64.zip") do (
  move /Y "%%~F" "%DIST_DIR%\" >nul
  set "ZIP_FOUND=%DIST_DIR%\%%~nxF"
)
for %%F in ("!MANIFEST_PKG_DIR!\*_!VER_US!_x86_64_Setup.exe") do (
  move /Y "%%~F" "%DIST_DIR%\" >nul
  set "INSTALLER_FOUND=%DIST_DIR%\%%~nxF"
)
echo.
echo  ============================================================
echo   !M_PACKAGE_SUCCESS!
echo  ============================================================
if defined ZIP_FOUND (
  call :fmt M_PACKAGE_ZIP "!ZIP_FOUND!"
  echo   !_F!
  for %%S in ("!ZIP_FOUND!") do (
    call :fmt M_SIZE "%%~zS"
    echo   !_F!
  )
) else (
  call :fmt M_PACKAGE_NO_ZIP "!MANIFEST_PKG_DIR!"
  echo   !M_WARN! !_F!
)
if defined INSTALLER_FOUND (
  call :fmt M_INSTALLER "!INSTALLER_FOUND!"
  echo   !_F!
  for %%S in ("!INSTALLER_FOUND!") do (
    call :fmt M_SIZE "%%~zS"
    echo   !_F!
  )
) else (
  call :fmt M_PACKAGE_NONE "!MANIFEST_PKG_DIR!"
  echo   !M_WARN! !_F!
  echo   !M_PACKAGE_NONE_2!
)
echo.
call :fmt M_PACKAGE_DIST "%DIST_DIR%"
echo   !_F!
echo  ============================================================
if not defined ZIP_FOUND exit /b 1
exit /b 0

:read_version
REM VERSION = "1.0.0.N" from viewer_version.txt (written by CMake), VER_US = "1_0_0_N",
REM CHANNEL_TYPE = Beta/Release/... from the CMake cache (VIEWER_CHANNEL_TYPE).
set "VERSION="
set "VERSION_FILE=%BUILD_DIR%\newview\viewer_version.txt"
if not exist "%VERSION_FILE%" exit /b 1
set /p VERSION=<"%VERSION_FILE%"
set "VERSION=!VERSION: =!"
if not defined VERSION exit /b 1
set "VER_US=!VERSION:.=_!"
set "CHANNEL_TYPE=Beta"
for /f "tokens=2 delims==" %%C in ('findstr /B /C:"VIEWER_CHANNEL_TYPE:STRING=" "%BUILD_DIR%\CMakeCache.txt" 2^>nul') do set "CHANNEL_TYPE=%%C"
exit /b 0

REM ============================================================================
REM  Run / smoke test
REM ============================================================================

:do_run
call :setup_env
set "RUN_EXE="
if exist "%VIEWER_EXE%" set "RUN_EXE=%VIEWER_EXE%"
if not defined RUN_EXE if exist "%VIEWER_EXE_BUILD%" set "RUN_EXE=%VIEWER_EXE_BUILD%"
if not defined RUN_EXE if exist "%VIEWER_EXE_ALT%" set "RUN_EXE=%VIEWER_EXE_ALT%"
if defined RUN_EXE (
  echo !M_STARTING_VIEWER!
  echo !RUN_EXE!
  start "" "!RUN_EXE!"
  exit /b 0
)
echo !M_EXE_NOT_FOUND!
echo   %VIEWER_EXE%
echo !M_BUILD_FIRST!
exit /b 1

:do_smoke
REM Quick test for contributors: starts the viewer, waits SMOKE_SECONDS and checks
REM the process is still alive (reached the login screen without crashing).
REM Does not log in. Also checks that the viewer wrote Allyn.exec_marker, which
REM proves initialisation (directories, settings, GL) happened.
call :setup_env
set "SMOKE_EXE="
if exist "%VIEWER_EXE%" set "SMOKE_EXE=%VIEWER_EXE%"
if not defined SMOKE_EXE if exist "%VIEWER_EXE_BUILD%" set "SMOKE_EXE=%VIEWER_EXE_BUILD%"
if not defined SMOKE_EXE if exist "%VIEWER_EXE_ALT%" set "SMOKE_EXE=%VIEWER_EXE_ALT%"
if not defined SMOKE_EXE (
  echo !M_SMOKE_NO_EXE!
  exit /b 1
)
tasklist /fi "IMAGENAME eq allyn-viewer-bin.exe" /nh 2>nul | find /i "allyn-viewer-bin.exe" >nul
if not errorlevel 1 (
  echo !M_SMOKE_ALREADY_RUNNING!
  exit /b 1
)
echo.
echo  !M_SMOKE_HEADER!
call :fmt M_EXECUTABLE "!SMOKE_EXE!"
echo  !_F!
call :fmt M_SMOKE_WAITING "%SMOKE_SECONDS%"
echo  !_F!
echo  !M_SMOKE_HINT!
echo.
REM The messages are read by PowerShell from the environment (M_SMOKE_*), already translated.
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "[Console]::OutputEncoding = [Text.Encoding]::UTF8;" ^
  "$exe = (Resolve-Path '!SMOKE_EXE!').Path;" ^
  "$logDir = '%VIEWER_LOG_DIR%';" ^
  "$wait = [int]'%SMOKE_SECONDS%';" ^
  "$start = (Get-Date).AddSeconds(-5);" ^
  "$p = Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -PassThru;" ^
  "Start-Sleep -Seconds $wait;" ^
  "$alive = -not $p.HasExited;" ^
  "if ($alive) { Write-Host ('  ' + ($env:M_SMOKE_ALIVE -f $wait, $p.Id)) } else { Write-Host ('  ' + ($env:M_SMOKE_DIED -f $p.ExitCode)) };" ^
  "$marker = Join-Path $logDir 'Allyn.exec_marker';" ^
  "if ((Test-Path $marker) -and ((Get-Item $marker).LastWriteTime -ge $start)) { Write-Host ('  ' + ($env:M_SMOKE_MARKER_OK -f $marker)) } else { Write-Host ('  ' + ($env:M_SMOKE_MARKER_MISSING -f $marker)) };" ^
  "$crashLog = Join-Path $logDir 'Allyn.log';" ^
  "if ((Test-Path $crashLog) -and ((Get-Item $crashLog).LastWriteTime -ge $start)) { Write-Host ('  ' + $env:M_SMOKE_CRASHLOG); Get-Content $crashLog -Tail 15 -ErrorAction SilentlyContinue | ForEach-Object { Write-Host ('    ' + $_) } };" ^
  "if ($alive) { Write-Host ('  ' + $env:M_SMOKE_CLOSING); Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue; Get-Process SLplugin -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue; Start-Sleep -Seconds 2 };" ^
  "if ($alive) { exit 0 } else { exit 1 }"
set "RC=!errorlevel!"
echo.
if !RC! neq 0 (
  echo  !M_SMOKE_RESULT_FAIL!
  call :fmt M_SMOKE_LOGS "%VIEWER_LOG_DIR%"
  echo    !_F!
  call :fmt M_SMOKE_DEBUG "%VIEWER_LOG_DIR%\Allyn-debug\static_debug_info.log"
  echo    !_F!
  exit /b !RC!
)
echo  !M_SMOKE_RESULT_OK!
exit /b 0
