/** 
 * @file llpluginprocesschild.h
 * @brief LLPluginProcessChild handles the child side of the external-process plugin API. 
 *
 * @cond
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
 * @endcond
 */
#ifndef LL_LLPLUGINPROCESSCHILD_H
#define LL_LLPLUGINPROCESSCHILD_H
#include <queue>
#include "llpluginmessage.h"
#include "llpluginmessagepipe.h"
#include "llplugininstance.h"
#include "llhost.h"
#include "llpluginsharedmemory.h"
class LLPluginInstance;
class LLPluginProcessChild: public LLPluginMessagePipeOwner, public LLPluginInstanceMessageListener
{
	LOG_CLASS(LLPluginProcessChild);
public:
	LLPluginProcessChild();
	~LLPluginProcessChild();
	void init(U32 launcher_port);
	void idle(void);
	void sleep(F64 seconds);
	void pump();
	bool isRunning(void);
	bool isDone(void);
	void killSockets(void);
	F64 getSleepTime(void) const { return mSleepTime; };
	void sendMessageToPlugin(const LLPluginMessage &message);
	void sendMessageToParent(const LLPluginMessage &message);
	void receiveMessageRaw(const std::string &message);
	void receivePluginMessage(const std::string &message);
private:
	enum EState
	{
		STATE_UNINITIALIZED,
		STATE_INITIALIZED,
		STATE_CONNECTED,
		STATE_PLUGIN_LOADING,
		STATE_PLUGIN_LOADED,
		STATE_PLUGIN_INITIALIZING,
		STATE_RUNNING,
		STATE_UNLOADING,
		STATE_UNLOADED,
		STATE_ERROR,
		STATE_DONE
	};
	void setState(EState state);
	EState mState;
	LLHost mLauncherHost;
	LLSocket::ptr_t mSocket;
	std::string mPluginFile;
	std::string mPluginDir;
	LLPluginInstance *mInstance;
	typedef std::map<std::string, LLPluginSharedMemory*> sharedMemoryRegionsType;
	sharedMemoryRegionsType mSharedMemoryRegions;
	LLTimer mHeartbeat;
	F64		mSleepTime;
	F64		mCPUElapsed;
	bool	mBlockingRequest;
	bool	mBlockingResponseReceived;
	std::queue<std::string> mMessageQueue;
	void deliverQueuedMessages();
};
#endif
