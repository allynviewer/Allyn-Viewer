/**
 * @file slplugin.cpp
 * @brief Loader shell for plugins, intended to be launched by the plugin host application, which directly loads a plugin dynamic library.
 *
 * @cond
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 * 
 *
 * @endcond
 */
#include "linden_common.h"
#include "llpluginprocesschild.h"
#include "llpluginmessage.h"
#include "llerrorcontrol.h"
#include "llapr.h"
#include "llstring.h"
#if LL_DARWIN
	#include <Carbon/Carbon.h>
	#include "slplugin-objc.h"
#endif
#if LL_DARWIN
	#include <signal.h>
#endif
#if LL_DARWIN
static void crash_handler(int sig)
{
	_exit(1);
}
#endif
#if LL_WINDOWS
#include <windows.h>
LONG WINAPI myWin32ExceptionHandler( struct _EXCEPTION_POINTERS* exception_infop )
{
	return EXCEPTION_EXECUTE_HANDLER;
}
static BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(TEXT("kernel32.dll"));
	if (hKernel32 == NULL) return FALSE;
	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if (pOrgEntry == NULL) return FALSE;
#ifdef _M_IX86
	unsigned char szExecute[] = { 0x33, 0xC0, 0xC2, 0x04, 0x00 };
#elif _M_X64
	unsigned char szExecute[] = { 0x33, 0xC0, 0xC3 };
#else
#error "The following code only works for x86 and x64!"
#endif
	DWORD oldProtect;
	BOOL bRet = VirtualProtect(pOrgEntry, sizeof(szExecute), PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(pOrgEntry, szExecute, sizeof(szExecute));
	VirtualProtect(pOrgEntry, sizeof(szExecute), oldProtect, &oldProtect);
	return bRet;
}
void initExceptionHandler()
{
	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	prev_filter = SetUnhandledExceptionFilter( myWin32ExceptionHandler );
	PreventSetUnhandledExceptionFilter();
}
bool checkExceptionHandler()
{
	bool ok = true;
	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	prev_filter = SetUnhandledExceptionFilter(myWin32ExceptionHandler);
	PreventSetUnhandledExceptionFilter();
	if (prev_filter != myWin32ExceptionHandler)
	{
		LL_WARNS("AppInit") << "Our exception handler (" << (void *)myWin32ExceptionHandler << ") replaced with " << prev_filter << "!" << LL_ENDL;
		ok = false;
	}
	if (prev_filter == NULL)
	{
		ok = FALSE;
		if (NULL == myWin32ExceptionHandler)
		{
			LL_WARNS("AppInit") << "Exception handler uninitialized." << LL_ENDL;
		}
		else
		{
			LL_WARNS("AppInit") << "Our exception handler (" << (void *)myWin32ExceptionHandler << ") replaced with NULL!" << LL_ENDL;
		}
	}
	return ok;
}
#endif
bool self_test = false;
#if LL_WINDOWS
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#else
int main(int argc, char **argv)
#endif
{
#ifdef CWDEBUG
	Debug( libcw_do.margin().assign("SLPlugin ", 9) );
	Debug(debug::init());
#endif
	{
		LLError::initForApplication(".");
		LLError::setDefaultLevel(LLError::LEVEL_INFO);
	}
#if LL_WINDOWS
	if( strlen( lpCmdLine ) == 0 )
	{
		LL_ERRS("slplugin") << "usage: " << "SLPlugin" << " launcher_port" << LL_ENDL;
	};
	U32 port = 0;
	if(!LLStringUtil::convertToU32(lpCmdLine, port))
	{
		LL_ERRS("slplugin") << "port number must be numeric" << LL_ENDL;
	};
	initExceptionHandler();
#elif LL_DARWIN
	if(argc < 2)
	{
		LL_ERRS("slplugin") << "usage: " << argv[0] << " launcher_port" << LL_ENDL;
	}
	U32 port = 61916;
	if (strcmp(argv[1], "TESTPLUGIN") == 0)
	{
		std::cout << "Running self test..." << std::endl;
		self_test = true;
	}
	else if (!LLStringUtil::convertToU32(argv[1], port))
	{
		LL_ERRS("slplugin") << "port number must be numeric" << LL_ENDL;
	}
	if (!self_test)
	{
	  signal(SIGILL, &crash_handler);
# if LL_DARWIN
	  signal(SIGEMT, &crash_handler);
# endif
	  signal(SIGFPE, &crash_handler);
	  signal(SIGBUS, &crash_handler);
	  signal(SIGSEGV, &crash_handler);
	  signal(SIGSYS, &crash_handler);
	}
#endif
#if LL_DARWIN
	setupCocoa();
	createAutoReleasePool();
#endif
	LLPluginProcessChild *plugin = new LLPluginProcessChild();
	plugin->init(port);
#if LL_DARWIN
	deleteAutoReleasePool();
#endif
	LLTimer timer;
	timer.start();
#if LL_WINDOWS
	checkExceptionHandler();
#endif
#if LL_DARWIN
	WindowRef front_window = NULL;
	WindowGroupRef layer_group = NULL;
	int window_hack_state = 0;
	CreateWindowGroup(kWindowGroupAttrFixedLevel, &layer_group);
	if(layer_group)
	{
		SetWindowGroupName(layer_group, CFSTR("SLPlugin Layer"));
		SetWindowGroupLevel(layer_group, kCGOverlayWindowLevel);
	}
#endif
#if LL_DARWIN
	EventTargetRef event_target = GetEventDispatcherTarget();
#endif
	while(!plugin->isDone())
	{
#if LL_DARWIN
		createAutoReleasePool();
#endif
		timer.reset();
		plugin->idle();
#if LL_DARWIN
		{
			EventRef event;
			if(ReceiveNextEvent(0, 0, kEventDurationNoWait, true, &event) == noErr)
			{
				SendEventToEventTarget (event, event_target);
				ReleaseEvent(event);
			}
			if(FrontNonFloatingWindow() != front_window)
			{
				ProcessSerialNumber self = { 0, kCurrentProcess };
				ProcessSerialNumber parent = { 0, kNoProcess };
				ProcessSerialNumber front = { 0, kNoProcess };
				Boolean this_is_front_process = false;
				Boolean parent_is_front_process = false;
				{
					ProcessInfoRec info;
					info.processInfoLength = sizeof(ProcessInfoRec);
					info.processName = NULL;
					info.processAppSpec = NULL;
					if(GetProcessInformation( &self, &info ) == noErr)
					{
						parent = info.processLauncher;
					}
					if(GetFrontProcess(&front) == noErr)
					{
						(void) SameProcess(&self, &front, &this_is_front_process);
						(void) SameProcess(&parent, &front, &parent_is_front_process);
					}
				}
				if((FrontNonFloatingWindow() != NULL) && (front_window == NULL))
				{
					if(window_hack_state == 0)
					{
						window_hack_state = 1;
					}
					if(layer_group)
					{
						SetWindowGroup(FrontNonFloatingWindow(), layer_group);
					}
					if(parent_is_front_process)
					{
						(void) SetFrontProcess( &self );
					}
					ActivateWindow(FrontNonFloatingWindow(), true);
				}
				else if((FrontNonFloatingWindow() == NULL) && (front_window != NULL))
				{
					if(this_is_front_process)
					{
						(void) SetFrontProcess(&parent);
					}
				}
				else if(window_hack_state == 1)
				{
					if(layer_group)
					{
						SetWindowGroupLevel(layer_group, kCGNormalWindowLevel);
					}
					window_hack_state = 2;
				}
				front_window = FrontNonFloatingWindow();
			}
		}
#endif
		F64 elapsed = timer.getElapsedTimeF64();
		F64 remaining = plugin->getSleepTime() - elapsed;
		if(remaining <= 0.0f)
		{
			plugin->pump();
		}
		else
		{
			plugin->sleep(remaining);
		}
#if LL_WINDOWS
#endif
#if LL_DARWIN
		deleteAutoReleasePool();
#endif
	}
	delete plugin;
	return 0;
}
