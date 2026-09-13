/**
 * @file llappviewerwin32.cpp
 * @brief The LLAppViewerWin32 class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llwindowwin32.h"
#include "llappviewerwin32.h"
#include "llgl.h"
#include "res/resource.h"
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <tchar.h>
#include <Werapi.h>
#include "llviewercontrol.h"
#include "lldxhardware.h"
#ifdef USE_NVAPI
#include "nvapi/nvapi.h"
#include "nvapi/NvApiDriverSettings.h"
#endif
#include <stdlib.h>
#include "llweb.h"
#include "llsecondlifeurls.h"
#include "llwindebug.h"
#include "llviewernetwork.h"
#include "llmd5.h"
#include "llfindlocale.h"
#include "llcommandlineparser.h"
#include "lltrans.h"
const std::string LLAppViewerWin32::sWindowClass = "Second Life";
bool create_app_mutex()
{
	bool result = true;
	LPCWSTR unique_mutex_name = L"SecondLifeAppMutex";
	HANDLE hMutex;
	hMutex = CreateMutex(nullptr, TRUE, unique_mutex_name);
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		result = false;
	}
	return result;
}
#ifdef USE_NVAPI
#define NVAPI_APPNAME L"Second Life"
void nvapi_error(NvAPI_Status status)
{
	NvAPI_ShortString szDesc = { 0 };
	NvAPI_GetErrorMessage(status, szDesc);
	LL_WARNS() << szDesc << LL_ENDL;
}
void ll_nvapi_init(NvDRSSessionHandle hSession)
{
	NvAPI_Status status = NvAPI_DRS_LoadSettings(hSession);
	if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}
	NvAPI_UnicodeString wsz = { 0 };
	memcpy_s(wsz, sizeof(wsz), NVAPI_APPNAME, sizeof(NVAPI_APPNAME));
	status = NvAPI_DRS_SetCurrentGlobalProfile(hSession, wsz);
	if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}
	NvDRSProfileHandle hProfile = 0;
	status = NvAPI_DRS_GetCurrentGlobalProfile(hSession, &hProfile);
	if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}
	status = NvAPI_DRS_LoadSettings(hSession);
	if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}
	NVDRS_SETTING drsSetting = {0};
	drsSetting.version = NVDRS_SETTING_VER;
	status = NvAPI_DRS_GetSetting(hSession, hProfile, PREFERRED_PSTATE_ID, &drsSetting);
	if (status == NVAPI_SETTING_NOT_FOUND)
	{
		drsSetting.version = NVDRS_SETTING_VER;
		drsSetting.settingId = PREFERRED_PSTATE_ID;
		drsSetting.settingType = NVDRS_DWORD_TYPE;
		drsSetting.u32CurrentValue = PREFERRED_PSTATE_PREFER_MAX;
		status = NvAPI_DRS_SetSetting(hSession, hProfile, &drsSetting);
		if (status != NVAPI_OK)
		{
			nvapi_error(status);
			return;
		}
        status = NvAPI_DRS_SaveSettings(hSession);
        if (status != NVAPI_OK)
        {
            nvapi_error(status);
            return;
        }
	}
	else if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}
}
#endif
#if DEBUGGING_SEH_FILTER
#	define WINMAIN DebuggingWinMain
#else
#	define WINMAIN WinMain
#endif
int APIENTRY WINMAIN(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	const S32 MAX_HEAPS = 255;
	DWORD heap_enable_lfh_error[MAX_HEAPS];
	S32 num_heaps = 0;
#if WINDOWS_CRT_MEM_CHECKS && !INCLUDE_VLD
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#elif 1
	_CrtSetDbgFlag(0);
	ULONG ulEnableLFH = 2;
	HANDLE* hHeaps = new HANDLE[MAX_HEAPS];
	num_heaps = GetProcessHeaps(MAX_HEAPS, hHeaps);
	for(S32 i = 0; i < num_heaps; i++)
	{
		bool success = HeapSetInformation(hHeaps[i], HeapCompatibilityInformation, &ulEnableLFH, sizeof(ulEnableLFH));
		if (success)
			heap_enable_lfh_error[i] = 0;
		else
			heap_enable_lfh_error[i] = GetLastError();
	}
#endif
	gIconResource = MAKEINTRESOURCE(IDI_LL_ICON);
	LLAppViewerWin32* viewer_app_ptr = new LLAppViewerWin32(lpCmdLine);
#if !defined(USE_CRASHPAD)
	viewer_app_ptr->setErrorHandler(LLAppViewer::handleViewerCrash);
#endif
	bool found_other_instance = !create_app_mutex();
	gDebugInfo["FoundOtherInstanceAtStartup"] = LLSD::Boolean(found_other_instance);
	bool ok = viewer_app_ptr->init();
	if(!ok)
	{
		LL_WARNS() << "Application init failed." << LL_ENDL;
		return -1;
	}
#ifdef USE_NVAPI
	NvAPI_Status status;
	status = NvAPI_Initialize();
	NvDRSSessionHandle hSession = 0;
    if (status == NVAPI_OK)
	{
		status = NvAPI_DRS_CreateSession(&hSession);
		if (status != NVAPI_OK)
		{
			nvapi_error(status);
		}
		else
		{
			ll_nvapi_init(hSession);
		}
	}
#endif
	if (num_heaps > 0)
	{
		LL_INFOS() << "Attempted to enable LFH for " << num_heaps << " heaps." << LL_ENDL;
		for(S32 i = 0; i < num_heaps; i++)
		{
			if (heap_enable_lfh_error[i])
			{
				LL_INFOS() << "  Failed to enable LFH for heap: " << i << " Error: " << heap_enable_lfh_error[i] << LL_ENDL;
			}
		}
	}
	if(!LLApp::isQuitting())
	{
		viewer_app_ptr->mainLoop();
	}
	if (!LLApp::isError())
	{
#if WINDOWS_CRT_MEM_CHECKS
    LL_INFOS() << "CRT Checking memory:" << LL_ENDL;
	if (!_CrtCheckMemory())
	{
		LL_WARNS() << "_CrtCheckMemory() failed at prior to cleanup!" << LL_ENDL;
	}
	else
	{
		LL_INFOS() << " No corruption detected." << LL_ENDL;
	}
#endif
	gGLActive = TRUE;
	viewer_app_ptr->cleanup();
#if WINDOWS_CRT_MEM_CHECKS
    LL_INFOS() << "CRT Checking memory:" << LL_ENDL;
	if (!_CrtCheckMemory())
	{
		LL_WARNS() << "_CrtCheckMemory() failed after cleanup!" << LL_ENDL;
	}
	else
	{
		LL_INFOS() << " No corruption detected." << LL_ENDL;
	}
#endif
	}
	delete viewer_app_ptr;
	viewer_app_ptr = nullptr;
	if(LLAppViewer::sUpdaterInfo)
	{
		_spawnl(_P_NOWAIT, LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str(), LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str(), LLAppViewer::sUpdaterInfo->mParams.str().c_str(), NULL);
		delete LLAppViewer::sUpdaterInfo ;
		LLAppViewer::sUpdaterInfo = NULL ;
	}
#ifdef USE_NVAPI
	if (hSession)
	{
		NvAPI_DRS_DestroySession(hSession);
		hSession = 0;
	}
#endif
	return 0;
}
#if DEBUGGING_SEH_FILTER
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    __try
    {
        WINMAIN(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    __except( viewer_windows_exception_handler( GetExceptionInformation() ) )
    {
        _tprintf( _T("Exception handled.\n") );
    }
}
#endif
void LLAppViewerWin32::disableWinErrorReporting()
{
	std::string const& executable_name = gDirUtilp->getExecutableFilename();
	if( S_OK == WerAddExcludedApplication( utf8str_to_utf16str(executable_name).c_str(), FALSE ) )
	{
		LL_INFOS() << "WerAddExcludedApplication() succeeded for " << executable_name << LL_ENDL;
	}
	else
	{
		LL_INFOS() << "WerAddExcludedApplication() failed for " << executable_name << LL_ENDL;
	}
}
const S32 MAX_CONSOLE_LINES = 500;
static bool create_console()
{
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	const bool isConsoleAllocated = AllocConsole();
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	setvbuf( stdin, nullptr, _IONBF, 0 );
	setvbuf( stdout, nullptr, _IONBF, 0 );
	setvbuf( stderr, nullptr, _IONBF, 0 );
    return isConsoleAllocated;
}
LLAppViewerWin32::LLAppViewerWin32(const char* cmd_line) :
	mCmdLine(cmd_line),
	mIsConsoleAllocated(false)
{
}
LLAppViewerWin32::~LLAppViewerWin32()
{
}
bool LLAppViewerWin32::init()
{
	disableWinErrorReporting();
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLWinDebug::instance().init();
#endif
	bool success = LLAppViewer::init();
    return success;
}
bool LLAppViewerWin32::cleanup()
{
	bool result = LLAppViewer::cleanup();
	gDXHardware.cleanup();
	if (mIsConsoleAllocated)
	{
		FreeConsole();
		mIsConsoleAllocated = false;
	}
	return result;
}
void LLAppViewerWin32::initLoggingAndGetLastDuration()
{
	LLAppViewer::initLoggingAndGetLastDuration();
}
void LLAppViewerWin32::initConsole()
{
	mIsConsoleAllocated = create_console();
	return LLAppViewer::initConsole();
}
void write_debug_dx(const char* str)
{
	std::string value = gDebugInfo["DXInfo"].asString();
	value += str;
	gDebugInfo["DXInfo"] = value;
}
void write_debug_dx(const std::string& str)
{
	write_debug_dx(str.c_str());
}
bool LLAppViewerWin32::initHardwareTest()
{
	if (FALSE == gSavedSettings.getBOOL("NoHardwareProbe"))
	{
		BOOL vram_only = !gSavedSettings.getBOOL("ProbeHardwareOnStartup");
		vram_only = TRUE;
		LLSplashScreen::update(LLTrans::getString("StartupDetectingHardware"));
		LL_DEBUGS("AppInit") << "Attempting to poll DirectX for hardware info" << LL_ENDL;
		gDXHardware.setWriteDebugFunc(write_debug_dx);
		S32Megabytes system_ram = gSysMemory.getPhysicalMemoryKB();
		BOOL probe_ok = gDXHardware.getInfo(vram_only, system_ram);
		if (!probe_ok
			&& gSavedSettings.getWarning("AboutDirectX9"))
		{
			LL_WARNS("AppInit") << "DirectX probe failed, alerting user." << LL_ENDL;
			std::ostringstream msg;
			msg << LLTrans::getString ("MBNoDirectX");
			S32 button = OSMessageBox(
				msg.str(),
				LLTrans::getString("MBWarning"),
				OSMB_YESNO);
			if (OSBTN_NO== button)
			{
				LL_INFOS("AppInit") << "User quitting after failed DirectX 9 detection" << LL_ENDL;
				LLWeb::loadURLExternal(DIRECTX_9_URL, false);
				return false;
			}
			gSavedSettings.setWarning("AboutDirectX9", FALSE);
		}
		LL_DEBUGS("AppInit") << "Done polling DirectX for hardware info" << LL_ENDL;
		gSavedSettings.setBOOL("ProbeHardwareOnStartup", FALSE);
		std::string splash_msg;
		LLStringUtil::format_map_t args;
		args["[APP_NAME]"] = LLAppViewer::instance()->getSecondLifeTitle();
		splash_msg = LLTrans::getString("StartupLoading", args);
		LLSplashScreen::update(splash_msg);
	}
	if (!restoreErrorTrap())
	{
		LL_WARNS("AppInit") << " Someone took over my exception handler (post hardware probe)!" << LL_ENDL;
	}
	if (gGLManager.mVRAM == 0)
	{
		gGLManager.mVRAM = gDXHardware.getVRAM();
		LL_WARNS("AppInit") << "gGLManager.mVRAM: " << gGLManager.mVRAM << LL_ENDL;
	}
	LL_INFOS("AppInit") << "Detected VRAM: " << gGLManager.mVRAM << LL_ENDL;
	return true;
}
bool LLAppViewerWin32::initParseCommandLine(LLCommandLineParser& clp)
{
	if (!clp.parseCommandLineString(mCmdLine))
	{
		return false;
	}
	FL_Locale *locale = nullptr;
	FL_Success success = FL_FindLocale(&locale, FL_MESSAGES);
	if (success != 0)
	{
		if (success >= 2 && locale->lang)
		{
			LL_INFOS("AppInit") << "Language: " << ll_safe_string(locale->lang) << LL_ENDL;
			LL_INFOS("AppInit") << "Location: " << ll_safe_string(locale->country) << LL_ENDL;
			LL_INFOS("AppInit") << "Variant: " << ll_safe_string(locale->variant) << LL_ENDL;
			LLControlVariable* c = gSavedSettings.getControl("SystemLanguage");
			if(c)
			{
				c->setValue(std::string(locale->lang), false);
			}
		}
	}
	FL_FreeLocale(&locale);
	return true;
}
bool LLAppViewerWin32::restoreErrorTrap()
{
	return true;
}
bool LLAppViewerWin32::sendURLToOtherInstance(const std::string& url)
{
	wchar_t window_class[256];
	mbstowcs(window_class, sWindowClass.c_str(), 255);
	window_class[255] = 0;
	HWND other_window = FindWindow(window_class, nullptr);
	if (other_window != nullptr)
	{
		LL_DEBUGS() << "Found other window with the name '" << getWindowTitle() << "'" << LL_ENDL;
		COPYDATASTRUCT cds;
		const S32 SLURL_MESSAGE_TYPE = 0;
		cds.dwData = SLURL_MESSAGE_TYPE;
		cds.cbData = url.length() + 1;
		cds.lpData = (void*)url.c_str();
		LRESULT msg_result = SendMessage(other_window, WM_COPYDATA, NULL, (LPARAM)&cds);
		LL_DEBUGS() << "SendMessage(WM_COPYDATA) to other window '"
				 << getWindowTitle() << "' returned " << msg_result << LL_ENDL;
		return true;
	}
	return false;
}
std::string LLAppViewerWin32::generateSerialNumber()
{
	char serial_md5[MD5HEX_STR_SIZE];
	serial_md5[0] = 0;
	DWORD serial = 0;
	DWORD flags = 0;
	BOOL success = GetVolumeInformation(
			TEXT("C:\\"),
			nullptr,
			0,
			&serial,
			nullptr,
			&flags,
			nullptr,
			0);
	if (success)
	{
		LLMD5 md5;
		md5.update( (unsigned char*)&serial, sizeof(DWORD));
		md5.finalize();
		md5.hex_digest(serial_md5);
	}
	else
	{
		LL_WARNS() << "GetVolumeInformation failed" << LL_ENDL;
	}
	return serial_md5;
}
