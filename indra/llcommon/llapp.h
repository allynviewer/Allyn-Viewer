/** 
 * @file llapp.h
 * @brief Declaration of the LLApp class.
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
#ifndef LL_LLAPP_H
#define LL_LLAPP_H
#include <map>
#include "llrun.h"
#include "llsd.h"
#include "lloptioninterface.h"
template <typename Type> class LLAtomic32;
typedef LLAtomic32<U32> LLAtomicU32;
class LLErrorThread;
class LLLiveFile;
typedef void (*LLAppErrorHandler)();
typedef void (*LLAppChildCallback)(int pid, bool exited, int status);
#if !LL_WINDOWS
extern S32 LL_SMACKDOWN_SIGNAL;
extern S32 LL_HEARTBEAT_SIGNAL;
void clear_signals();
class LLChildInfo
{
public:
	LLChildInfo() : mGotSigChild(FALSE), mCallback(NULL) {}
	BOOL mGotSigChild;
	LLAppChildCallback mCallback;
};
#endif
class LL_COMMON_API LLApp : public LLOptionInterface
{
	friend class LLErrorThread;
public:
	typedef enum e_app_status
	{
		APP_STATUS_RUNNING,
		APP_STATUS_QUITTING,
		APP_STATUS_STOPPED,
		APP_STATUS_ERROR
	} EAppStatus;
	LLApp();
	virtual ~LLApp();
protected:
	LLApp(LLErrorThread* error_thread);
	void commonCtor();
public:
	static LLApp* instance();
	enum OptionPriority
	{
		PRIORITY_RUNTIME_OVERRIDE,
		PRIORITY_COMMAND_LINE,
		PRIORITY_SPECIFIC_CONFIGURATION,
		PRIORITY_GENERAL_CONFIGURATION,
		PRIORITY_DEFAULT,
		PRIORITY_COUNT
	};
	virtual LLSD getOption(const std::string& name) const;
	bool parseCommandOptions(int argc, char** argv);
	void manageLiveFile(LLLiveFile* livefile);
	bool setOptionData(OptionPriority level, LLSD data);
	LLSD getOptionData(OptionPriority level);
	virtual bool init() = 0;
	virtual bool cleanup() = 0;
	virtual bool mainLoop() = 0;
	void disableCrashlogger();
	static bool isCrashloggerDisabled();
	static void setQuitting();
	static void setStopped();
	static void setError();
	static bool isStopped();
	static bool isRunning();
	static bool isQuitting();
	static bool isError();
	static bool isExiting();
#if !LL_WINDOWS
	static U32  getSigChildCount();
	static void incSigChildCount();
#endif
	static int getPid();
	void setupErrorHandling();
	void setErrorHandler(LLAppErrorHandler handler);
	static void runErrorHandler();
    void setDebugFileNames(const std::string &path);
    std::string* getStaticDebugFile() { return &mStaticDebugFileName; }
    std::string* getDynamicDebugFile() { return &mDynamicDebugFileName; }
#if !LL_WINDOWS
	void setChildCallback(pid_t pid, LLAppChildCallback callback);
	void setDefaultChildCallback(LLAppChildCallback callback);
	pid_t fork();
#endif
public:
	typedef std::map<std::string, std::string> string_map;
	string_map mOptionMap;
protected:
	static void setStatus(EAppStatus status);
	static EAppStatus sStatus;
	static BOOL sErrorThreadRunning;
	static BOOL sDisableCrashlogger;
	std::wstring mCrashReportPipeStr;
#if !LL_WINDOWS
	static LLAtomicU32* sSigChildCount;
	typedef std::map<pid_t, LLChildInfo> child_map;
	static child_map sChildMap;
	static LLAppChildCallback sDefaultChildCallback;
#endif
	void stopErrorThread();
private:
	void startErrorThread();
    std::string mStaticDebugFileName;
    std::string mDynamicDebugFileName;
	typedef int(*signal_handler_func)(int signum);
	static LLAppErrorHandler sErrorHandler;
	LLErrorThread* mThreadErrorp;
	LLSD mOptions;
	std::vector<LLLiveFile*> mLiveFiles;
	static LLApp* sApplication;
#if !LL_WINDOWS
	friend void default_unix_signal_handler(int signum, siginfo_t *info, void *);
#endif
public:
	static BOOL sLogInSignal;
};
#endif
