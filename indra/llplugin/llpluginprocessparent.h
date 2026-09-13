/** 
 * @file llpluginprocessparent.h
 * @brief LLPluginProcessParent handles the parent side of the external-process plugin API.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */
#ifndef LL_LLPLUGINPROCESSPARENT_H
#define LL_LLPLUGINPROCESSPARENT_H
#include <queue>
#include "llapr.h"
#include "llprocesslauncher.h"
#include "llpluginmessage.h"
#include "llpluginmessagepipe.h"
#include "llpluginsharedmemory.h"
#include "lliosocket.h"
#include "llthread.h"
class LLPluginProcessParentOwner
{
public:
	virtual ~LLPluginProcessParentOwner();
	virtual void receivePluginMessage(const LLPluginMessage &message) = 0;
	virtual bool receivePluginMessageEarly(const LLPluginMessage &message) {return false;};
	virtual void receivedShutdown() = 0;
	virtual void pluginLaunchFailed() {};
	virtual void pluginDied() {};
};
class LLPluginProcessParent : public LLPluginMessagePipeOwner
{
	LOG_CLASS(LLPluginProcessParent);
public:
	LLPluginProcessParent(LLPluginProcessParentOwner *owner);
	~LLPluginProcessParent();
	void init(const std::string &launcher_filename,
			  const std::string &plugin_dir,
			  const std::string &plugin_filename,
			  bool debug);
	void idle(void);
	bool isLoading(void);
	bool isRunning(void);
	bool isDone(void);
	bool isBlocked(void) { return mBlocked; };
	void killSockets(void);
	void errorState(void);
	void exitState(void) { setState(STATE_EXITING); }
	void setSleepTime(F64 sleep_time, bool force_send = false);
	F64 getSleepTime(void) const { return mSleepTime; };
	void sendMessage(const LLPluginMessage &message);
	void receiveMessage(const LLPluginMessage &message);
	void receiveMessageRaw(const std::string &message);
	void receiveMessageEarly(const LLPluginMessage &message);
	void setMessagePipe(LLPluginMessagePipe *message_pipe) ;
	apr_status_t socketError(apr_status_t error);
	std::string addSharedMemory(size_t size);
	void removeSharedMemory(const std::string &name);
	size_t getSharedMemorySize(const std::string &name);
	void *getSharedMemoryAddress(const std::string &name);
	std::string getMessageClassVersion(const std::string &message_class);
	std::string getPluginVersion(void);
	bool getDisableTimeout() { return mDisableTimeout; };
	void setDisableTimeout(bool disable) { mDisableTimeout = disable; };
	void setLaunchTimeout(F32 timeout) { mPluginLaunchTimeout = timeout; };
	void setLockupTimeout(F32 timeout) { mPluginLockupTimeout = timeout; };
	F64 getCPUUsage() { return mCPUUsage; };
	static void poll(F64 timeout);
	static bool canPollThreadRun() { return (sPollSet || sPollsetNeedsRebuild || sUseReadThread); };
	static void setUseReadThread(bool use_read_thread);
	static bool getUseReadThread() { return sUseReadThread; };
private:
	enum EState
	{
		STATE_UNINITIALIZED,
		STATE_INITIALIZED,
		STATE_LISTENING,
		STATE_LAUNCHED,
		STATE_CONNECTED,
		STATE_HELLO,
		STATE_LOADING,
		STATE_RUNNING,
		STATE_LAUNCH_FAILURE,
		STATE_ERROR,
		STATE_EXITING,
		STATE_CLEANUP,
		STATE_DONE
	};
	EState mState;
	void setState(EState state);
	std::string stateToString(EState state);
	bool pluginLockedUp();
	bool pluginLockedUpOrQuit();
	bool accept();
	LLSocket::ptr_t mListenSocket;
	LLSocket::ptr_t mSocket;
	U32 mBoundPort;
	LLProcessLauncher mProcess;
	std::string mPluginFile;
	std::string mPluginDir;
	LLPluginProcessParentOwner *mOwner;
	typedef std::map<std::string, LLPluginSharedMemory*> sharedMemoryRegionsType;
	sharedMemoryRegionsType mSharedMemoryRegions;
	LLSD mMessageClassVersions;
	std::string mPluginVersionString;
	LLTimer mHeartbeat;
	F64		mSleepTime;
	F64		mCPUUsage;
	bool mDisableTimeout;
	bool mDebug;
	bool mBlocked;
	bool mPolledInput;
	bool mReceivedShutdown;
	LLProcessLauncher mDebugger;
	F32 mPluginLaunchTimeout;
	F32 mPluginLockupTimeout;
	static bool sUseReadThread;
	apr_pollfd_t mPollFD;
	LLAPRPool mPollFDPool;
	static apr_pollset_t *sPollSet;
	static LLAPRPool sPollSetPool;
	static bool sPollsetNeedsRebuild;
	static LLMutex *sInstancesMutex;
	static std::list<LLPluginProcessParent*> sInstances;
	static void dirtyPollSet();
	static void updatePollset();
	void servicePoll();
	static LLThread *sReadThread;
	LLMutex mIncomingQueueMutex;
	std::queue<LLPluginMessage> mIncomingQueue;
};
#endif
