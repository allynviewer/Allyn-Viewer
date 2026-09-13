/** 
 * @file llapp.cpp
 * @brief Implementation of the LLApp class.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#include "linden_common.h"
#include "llapp.h"
#include "llcommon.h"
#include "llapr.h"
#include "llerrorcontrol.h"
#include "llerrorthread.h"
#include "llframetimer.h"
#include "lllivefile.h"
#include "llmemory.h"
#include "llstl.h"
#include "llstring.h"
#include "lleventtimer.h"
#if LL_WINDOWS
#include <windows.h>
LONG WINAPI default_windows_exception_handler(struct _EXCEPTION_POINTERS *exception_infop);
BOOL ConsoleCtrlHandler(DWORD fdwCtrlType);
#endif
LLApp* LLApp::sApplication = nullptr;
BOOL LLApp::sDisableCrashlogger = FALSE;
BOOL LLApp::sLogInSignal = FALSE;
LLApp::EAppStatus LLApp::sStatus = LLApp::APP_STATUS_STOPPED;
LLAppErrorHandler LLApp::sErrorHandler = nullptr;
BOOL LLApp::sErrorThreadRunning = FALSE;
LLApp::LLApp()
	: mThreadErrorp(nullptr)
{
	commonCtor();
}
void LLApp::commonCtor()
{
	setStatus(APP_STATUS_RUNNING);
	LLCommon::initClass();
	mOptions = LLSD::emptyArray();
	LLSD sd;
	for(int i = 0; i < PRIORITY_COUNT; ++i)
	{
		mOptions.append(sd);
	}
	sApplication = this;
	mCrashReportPipeStr = L"\\\\.\\pipe\\LLCrashReporterPipe";
}
LLApp::LLApp(LLErrorThread *error_thread) :
	mThreadErrorp(error_thread)
{
	commonCtor();
}
LLApp::~LLApp()
{
	std::for_each(mLiveFiles.begin(), mLiveFiles.end(), DeletePointer());
	mLiveFiles.clear();
	setStopped();
	ms_sleep(20);
	if (mThreadErrorp)
	{
		delete mThreadErrorp;
		mThreadErrorp = nullptr;
	}
	LLCommon::cleanupClass();
}
LLApp* LLApp::instance()
{
	return sApplication;
}
LLSD LLApp::getOption(const std::string& name) const
{
	LLSD rv;
    auto iter = mOptions.beginArray();
    auto end = mOptions.endArray();
	for(; iter != end; ++iter)
	{
		rv = (*iter)[name];
		if(rv.isDefined()) break;
	}
	return rv;
}
bool LLApp::parseCommandOptions(int argc, char** argv)
{
	LLSD commands;
	std::string name;
	std::string value;
	for(int ii = 1; ii < argc; ++ii)
	{
		if(argv[ii][0] != '-')
		{
			LL_INFOS() << "Did not find option identifier while parsing token: "
				<< argv[ii] << LL_ENDL;
			return false;
		}
		int offset = 1;
		if(argv[ii][1] == '-') ++offset;
		name.assign(&argv[ii][offset]);
		if(((ii+1) >= argc) || (argv[ii+1][0] == '-'))
		{
			int flag = name.compare("logfile");
			if (0 == flag)
			{
				commands[name] = "log";
			}
			else
			{
				commands[name] = true;
			}
			continue;
		}
		++ii;
		value.assign(argv[ii]);
#if LL_WINDOWS
		size_t slen = value.length() - 1;
		size_t start = 0;
		size_t end = slen;
		if (argv[ii][start]=='"')start++;
		if (argv[ii][end]=='"')end--;
		if (start!=0 || end!=slen)
		{
			value = value.substr (start,end);
		}
#endif
		commands[name] = value;
	}
	setOptionData(PRIORITY_COMMAND_LINE, commands);
	return true;
}
void LLApp::manageLiveFile(LLLiveFile* livefile)
{
	if(!livefile) return;
	livefile->checkAndReload();
	livefile->addToEventTimer();
	mLiveFiles.push_back(livefile);
}
bool LLApp::setOptionData(OptionPriority level, LLSD data)
{
	if((level < 0)
	   || (level >= PRIORITY_COUNT)
	   || (data.type() != LLSD::TypeMap))
	{
		return false;
	}
	mOptions[level] = data;
	return true;
}
LLSD LLApp::getOptionData(OptionPriority level)
{
	if((level < 0) || (level >= PRIORITY_COUNT))
	{
		return LLSD();
	}
	return mOptions[level];
}
#if LL_WINDOWS
void EnableCrashingOnCrashes()
{
	typedef BOOL (WINAPI *tGetPolicy)(LPDWORD lpFlags);
	typedef BOOL (WINAPI *tSetPolicy)(DWORD dwFlags);
	const DWORD EXCEPTION_SWALLOWING = 0x1;
	HMODULE kernel32 = LoadLibraryA("kernel32.dll");
	tGetPolicy pGetPolicy = (tGetPolicy)GetProcAddress(kernel32,
		"GetProcessUserModeExceptionPolicy");
	tSetPolicy pSetPolicy = (tSetPolicy)GetProcAddress(kernel32,
		"SetProcessUserModeExceptionPolicy");
	if (pGetPolicy && pSetPolicy)
	{
		DWORD dwFlags;
		if (pGetPolicy(&dwFlags))
		{
			pSetPolicy(dwFlags & ~EXCEPTION_SWALLOWING);
		}
	}
}
#endif
void LLApp::setupErrorHandling()
{
#if !defined(USE_CRASHPAD)
	startErrorThread();
#endif
}
void LLApp::startErrorThread()
{
	if(!mThreadErrorp)
	{
		LL_INFOS() << "Starting error thread" << LL_ENDL;
		mThreadErrorp = new LLErrorThread();
		mThreadErrorp->setUserData((void *) this);
		mThreadErrorp->start();
	}
}
void LLApp::stopErrorThread()
{
	LLApp::setStopped();
	int count = 0;
	while (mThreadErrorp && !mThreadErrorp->isStopped() && ++count < 100)
	{
		ms_sleep(10);
	}
	if (mThreadErrorp && !mThreadErrorp->isStopped())
	{
		LL_WARNS() << "Failed to stop Error Thread." << LL_ENDL;
	}
}
void LLApp::setErrorHandler(LLAppErrorHandler handler)
{
	LLApp::sErrorHandler = handler;
}
void LLApp::runErrorHandler()
{
	if (LLApp::sErrorHandler)
	{
		LLApp::sErrorHandler();
	}
	LLApp::setStopped();
}
void LLApp::setStatus(EAppStatus status)
{
	sStatus = status;
}
void LLApp::setError()
{
	setStatus(APP_STATUS_ERROR);
}
void LLApp::setDebugFileNames(const std::string &path)
{
  	mStaticDebugFileName = path + "static_debug_info.log";
  	mDynamicDebugFileName = path + "dynamic_debug_info.log";
}
void LLApp::setQuitting()
{
	if (!isExiting())
	{
		LL_INFOS() << "Setting app state to QUITTING" << LL_ENDL;
		setStatus(APP_STATUS_QUITTING);
	}
}
void LLApp::setStopped()
{
	setStatus(APP_STATUS_STOPPED);
}
bool LLApp::isStopped()
{
	return (APP_STATUS_STOPPED == sStatus);
}
bool LLApp::isRunning()
{
	return (APP_STATUS_RUNNING == sStatus);
}
bool LLApp::isError()
{
	return (APP_STATUS_ERROR == sStatus);
}
bool LLApp::isQuitting()
{
	return (APP_STATUS_QUITTING == sStatus);
}
bool LLApp::isExiting()
{
	return isQuitting() || isError() || isStopped();
}
void LLApp::disableCrashlogger()
{
	sDisableCrashlogger = TRUE;
}
bool LLApp::isCrashloggerDisabled()
{
	return (sDisableCrashlogger == TRUE);
}
int LLApp::getPid()
{
	return GetCurrentProcessId();
}
#if LL_WINDOWS
LONG WINAPI default_windows_exception_handler(struct _EXCEPTION_POINTERS *exception_infop)
{
	LONG retval;
	if (LLApp::isError())
	{
		LL_WARNS() << "Got another fatal signal while in the error handler, die now!" << LL_ENDL;
		retval = EXCEPTION_EXECUTE_HANDLER;
		return retval;
	}
	LLApp::setError();
	while (!LLApp::isStopped())
	{
		ms_sleep(10);
	}
	retval = EXCEPTION_EXECUTE_HANDLER;
	return retval;
}
BOOL ConsoleCtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
 		case CTRL_BREAK_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_C_EVENT:
			if (LLApp::isQuitting() || LLApp::isError())
			{
				if (LLApp::sLogInSignal)
				{
					LL_INFOS() << "Signal handler - Already trying to quit, ignoring signal!" << LL_ENDL;
				}
				return TRUE;
			}
			LLApp::setQuitting();
			return TRUE;
		default:
			return FALSE;
	}
}
#endif
