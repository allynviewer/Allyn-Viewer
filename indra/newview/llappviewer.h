/** 
 * @file llappviewer.h
 * @brief The LLAppViewer class declaration
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
#ifndef LL_LLAPPVIEWER_H
#define LL_LLAPPVIEWER_H
#include "llallocator.h"
#include "llsys.h"
#include "llviewercontrol.h"
class LLCommandLineParser;
class LLTextureCache;
class LLImageDecodeThread;
class LLTextureFetch;
class LLWatchdogTimeout;
class LLAppViewer : public LLApp
{
public:
	LLAppViewer();
	virtual ~LLAppViewer();
    static LLAppViewer* instance() {return sInstance; }
	virtual bool init();
	virtual bool cleanup();
	virtual bool mainLoop();
	void flushVFSIO();
	void forceQuit();
	void fastQuit(S32 error_code = 0);
	void requestQuit();
	void userQuit();
    void earlyExit(const std::string& name,
				   const LLSD& substitutions = LLSD());
    void abortQuit();
    bool quitRequested() { return mQuitRequested; }
    bool logoutRequestSent() { return mLogoutRequestSent; }
	bool isSecondInstance() { return mSecondInstance; }
	void writeDebugInfo(bool isStatic=true);
	const LLOSInfo& getOSInfo() const { return mSysOSInfo; }
	virtual bool beingDebugged() { return false; }
	virtual bool restoreErrorTrap() = 0;
	void initCrashReporting();
	static void handleViewerCrash();
	static LLTextureCache* getTextureCache() { return sTextureCache; }
	static LLImageDecodeThread* getImageDecodeThread() { return sImageDecodeThread; }
	static LLTextureFetch* getTextureFetch() { return sTextureFetch; }
	static U32 getTextureCacheVersion() ;
	static U32 getObjectCacheVersion() ;
	const std::string& getSerialNumber() { return mSerialNumber; }
	bool getPurgeCache() const { return mPurgeCache; }
	std::string getSecondLifeTitle() const;
	const std::string& getWindowTitle() const;
    void forceDisconnect(const std::string& msg);
    void badNetworkHandler();
	bool hasSavedFinalSnapshot() { return mSavedFinalSnapshot; }
	void saveFinalSnapshot();
    void loadNameCache();
    void saveNameCache();
	void persistInventoryCache();
	void removeMarkerFiles();
	void removeDumpDir();
    virtual void forceErrorLLError();
    virtual void forceErrorBreakpoint();
    virtual void forceErrorBadMemoryAccess();
    virtual void forceErrorInfiniteLoop();
    virtual void forceErrorSoftwareException();
    virtual void forceErrorDriverCrash();
	static const std::string sGlobalSettingsName;
	static const std::string sPerAccountSettingsName;
	bool loadSettingsFromDirectory(AIReadAccess<settings_map_type> const& settings_r,
	                               std::string const& location_key,
	                               bool set_defaults = false);
	std::string getSettingsFilename(std::string const& location_key,
	                                std::string const& file);
	void initMainloopTimeout(const std::string& state, F32 secs = -1.0f);
	void destroyMainloopTimeout();
	void pauseMainloopTimeout();
	void resumeMainloopTimeout(const std::string& state = "", F32 secs = -1.0f);
	void pingMainloopTimeout(const std::string& state, F32 secs = -1.0f);
	void handleLoginComplete();
    LLAllocator & getAllocator() { return mAlloc; }
	typedef boost::signals2::signal<void (void)> login_completed_signal_t;
	login_completed_signal_t mOnLoginCompleted;
	boost::signals2::connection setOnLoginCompletedCallback( const login_completed_signal_t::slot_type& cb ) { return mOnLoginCompleted.connect(cb); }
	void addOnIdleCallback(const boost::function<void()>& cb);
	void purgeCache();
	static void metricsUpdateRegion(U64 region_handle);
	static void metricsSend(bool enable_reporting);
protected:
	virtual bool initWindow();
	virtual void initLoggingAndGetLastDuration();
	void initLoggingInternal();
	virtual void initConsole() {};
	virtual bool initHardwareTest() { return true; }
	virtual bool initSLURLHandler();
	virtual bool sendURLToOtherInstance(const std::string& url);
	virtual bool initParseCommandLine(LLCommandLineParser& clp)
        { return true; }
	virtual std::string generateSerialNumber() = 0;
private:
	void initMaxHeapSize();
	bool initThreads();
	bool initConfiguration();
	bool initCache();
	void checkMemory() ;
	void migrateCacheDirectory();
	void cleanupSavedSettings();
	void removeCacheFiles(const std::string& filemask);
	void writeSystemInfo();
	void processMarkerFiles();
	static void recordMarkerVersion(LLAPRFile& marker_file);
	bool markerIsSameVersion(const std::string& marker_name) const;
    void idle();
    void idleShutdown();
	void idleNameCache();
    void idleNetwork();
	void idleAudio();
	void shutdownAudio();
    void sendLogoutRequest();
    void disconnectViewer();
	static LLAppViewer* sInstance;
    bool mSecondInstance;
	std::string mMarkerFileName;
	LLAPRFile mMarkerFile;
	std::string mLogoutMarkerFileName;
	LLAPRFile mLogoutMarkerFile;
	LLOSInfo mSysOSInfo;
	bool mReportedCrash;
	static LLTextureCache* sTextureCache;
	static LLImageDecodeThread* sImageDecodeThread;
	static LLTextureFetch* sTextureFetch;
	S32 mNumSessions;
	std::string mSerialNumber;
	bool mPurgeCache;
    bool mPurgeOnExit;
	bool mSavedFinalSnapshot;
    bool mQuitRequested;
    bool mLogoutRequestSent;
	LLSD mSettingsLocationList;
	LLWatchdogTimeout* mMainloopTimeout;
	bool mAgentRegionLastAlive;
	LLUUID mAgentRegionLastID;
    LLAllocator mAlloc;
	LLFrameTimer mMemCheckTimer;
public:
	typedef struct
	{
		std::string mUpdateExePath;
		std::ostringstream mParams;
	}LLUpdaterInfo ;
	static LLUpdaterInfo *sUpdaterInfo ;
};
const S32 AGENT_UPDATES_PER_SECOND  = 10;
const S32 AGENT_FORCE_UPDATES_PER_SECOND  = 1;
extern LLSD gDebugInfo;
extern BOOL	gShowObjectUpdates;
extern BOOL gAcceptTOS;
extern BOOL gAcceptCriticalMessage;
typedef enum
{
	LAST_EXEC_NORMAL = 0,
	LAST_EXEC_FROZE,
	LAST_EXEC_LLERROR_CRASH,
	LAST_EXEC_OTHER_CRASH,
	LAST_EXEC_LOGOUT_FROZE,
	LAST_EXEC_LOGOUT_CRASH
} eLastExecEvent;
extern eLastExecEvent gLastExecEvent;
extern U32 gFrameCount;
extern U32 gForegroundFrameCount;
extern LLPumpIO* gServicePump;
extern BOOL gPacificDaylightTime;
extern U64MicrosecondsImplicit	gStartTime;
extern U64MicrosecondsImplicit	gFrameTime;
extern F32SecondsImplicit		gFrameTimeSeconds;
extern F32SecondsImplicit		gFrameIntervalSeconds;
extern F32		gFPSClamped;
extern F32		gFrameDTClamped;
extern U32 		gFrameStalls;
extern LLTimer gRenderStartTime;
extern LLFrameTimer gForegroundTime;
extern F32 gLogoutMaxTime;
extern LLTimer gLogoutTimer;
extern F32 gSimLastTime;
extern F32 gSimFrames;
extern BOOL		gDisconnected;
extern LLFrameTimer	gRestoreGLTimer;
extern BOOL			gRestoreGL;
extern BOOL		gUseWireframe;
extern LLVFS	*gStaticVFS;
extern LLMemoryInfo gSysMemory;
extern U64Bytes gMemoryAllocated;
extern std::string gLastVersionChannel;
extern LLVector3 gWindVec;
extern LLVector3 gRelativeWindVec;
extern U32	gPacketsIn;
extern BOOL gPrintMessagesThisFrame;
extern LLUUID gSunTextureID;
extern LLUUID gMoonTextureID;
extern BOOL gRandomizeFramerate;
extern BOOL gPeriodicSlowFrame;
#endif
