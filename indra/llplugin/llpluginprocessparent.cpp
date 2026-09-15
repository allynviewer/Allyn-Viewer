/** 
 * @file llpluginprocessparent.cpp
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
#include "linden_common.h"
#include "llpluginprocessparent.h"
#include "llpluginmessagepipe.h"
#include "llpluginmessageclasses.h"
#include "llapr.h"
LLPluginProcessParentOwner::~LLPluginProcessParentOwner()
{
}
bool LLPluginProcessParent::sUseReadThread = false;
apr_pollset_t *LLPluginProcessParent::sPollSet = NULL;
LLAPRPool LLPluginProcessParent::sPollSetPool;
bool LLPluginProcessParent::sPollsetNeedsRebuild = false;
LLMutex *LLPluginProcessParent::sInstancesMutex;
std::list<LLPluginProcessParent*> LLPluginProcessParent::sInstances;
LLThread *LLPluginProcessParent::sReadThread = NULL;
class LLPluginProcessParentPollThread: public LLThread
{
public:
	LLPluginProcessParentPollThread() :
		LLThread("LLPluginProcessParentPollThread")
	{
	}
protected:
	void run(void)
	{
		while(!isQuitting() && LLPluginProcessParent::getUseReadThread())
		{
			LLPluginProcessParent::poll(0.1f);
			checkPause();
		}
		LLPluginProcessParent::poll(0.0f);
	}
	bool runCondition(void)
	{
		return(LLPluginProcessParent::canPollThreadRun());
	}
};
LLPluginProcessParent::LLPluginProcessParent(LLPluginProcessParentOwner *owner)
{
	if(!sInstancesMutex)
	{
		sInstancesMutex = new LLMutex;
	}
	mOwner = owner;
	mBoundPort = 0;
	mState = STATE_UNINITIALIZED;
	mSleepTime = 0.0;
	mCPUUsage = 0.0;
	mDisableTimeout = false;
	mDebug = false;
	mBlocked = false;
	mPolledInput = false;
	mReceivedShutdown = false;
	mPollFD.client_data = NULL;
	mPollFDPool.create();
	mPluginLaunchTimeout = 60.0f;
	mPluginLockupTimeout = 15.0f;
	mHeartbeat.stop();
	{
		LLMutexLock lock(sInstancesMutex);
		sInstances.push_back(this);
	}
}
LLPluginProcessParent::~LLPluginProcessParent()
{
	LL_DEBUGS("Plugin") << "destructor" << LL_ENDL;
	{
		LLMutexLock lock(sInstancesMutex);
		{
			LLMutexLock lock2(&mIncomingQueueMutex);
			sInstances.remove(this);
		}
	}
	sharedMemoryRegionsType::iterator iter;
	while((iter = mSharedMemoryRegions.begin()) != mSharedMemoryRegions.end())
	{
		iter->second->destroy();
		mSharedMemoryRegions.erase(iter);
	}
	mProcess.kill();
	killSockets();
}
void LLPluginProcessParent::killSockets(void)
{
	{
		LLMutexLock lock(&mIncomingQueueMutex);
		killMessagePipe();
	}
	mListenSocket.reset();
	mSocket.reset();
}
void LLPluginProcessParent::errorState(void)
{
	if(mState < STATE_RUNNING)
		setState(STATE_LAUNCH_FAILURE);
	else if (mReceivedShutdown)
		setState(STATE_EXITING);
	else
		setState(STATE_ERROR);
}
void LLPluginProcessParent::init(const std::string &launcher_filename, const std::string &plugin_dir, const std::string &plugin_filename, bool debug)
{
	mProcess.setExecutable(launcher_filename);
	mProcess.setWorkingDirectory(plugin_dir);
	mPluginFile = plugin_filename;
	mPluginDir = plugin_dir;
	mCPUUsage = 0.0f;
	mDebug = debug;
	setState(STATE_INITIALIZED);
}
bool LLPluginProcessParent::accept()
{
	bool result = false;
	apr_status_t status = APR_EGENERAL;
	mSocket = LLSocket::create(status, mListenSocket);
	if(status == APR_SUCCESS)
	{
		new LLPluginMessagePipe(this, mSocket);
		result = true;
	}
	else
	{
		mSocket.reset();
		if (!APR_STATUS_IS_EAGAIN(status))
		{
			ll_apr_warn_status(status);
			errorState();
		}
	}
	return result;
}
void LLPluginProcessParent::idle(void)
{
	bool idle_again;
	do
	{
		mIncomingQueueMutex.lock();
		while(!mIncomingQueue.empty())
		{
			LLPluginMessage message = mIncomingQueue.front();
			mIncomingQueue.pop();
			mIncomingQueueMutex.unlock();
			receiveMessage(message);
			mIncomingQueueMutex.lock();
		}
		mIncomingQueueMutex.unlock();
		if(mMessagePipe)
		{
			mMessagePipe->pumpOutput();
			if(!mPolledInput)
			{
				mMessagePipe->pumpInput();
			}
		}
		if(mState <= STATE_RUNNING)
		{
			if(APR_STATUS_IS_EOF(mSocketError))
			{
				errorState();
			}
			else if(mSocketError != APR_SUCCESS)
			{
				LL_WARNS("Plugin") << "Socket hit an error state (" << mSocketError << ")" << LL_ENDL;
				errorState();
			}
		}
		idle_again = false;
		switch(mState)
		{
			case STATE_UNINITIALIZED:
			break;
			case STATE_INITIALIZED:
			{
				apr_status_t status = APR_SUCCESS;
				apr_sockaddr_t* addr = NULL;
				mListenSocket = LLSocket::create(LLSocket::STREAM_TCP);
				mBoundPort = 0;
				status = apr_sockaddr_info_get(
					&addr,
					"127.0.0.1",
					APR_INET,
					0,
					0,
					LLAPRRootPool::get()());
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}
				ll_apr_warn_status(apr_socket_opt_set(mListenSocket->getSocket(), APR_SO_REUSEADDR, 1));
				status = apr_socket_bind(mListenSocket->getSocket(), addr);
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}
				{
					apr_sockaddr_t* bound_addr = NULL;
					if(ll_apr_warn_status(apr_socket_addr_get(&bound_addr, APR_LOCAL, mListenSocket->getSocket())))
					{
						killSockets();
						errorState();
						break;
					}
					mBoundPort = bound_addr->port;
					if(mBoundPort == 0)
					{
						LL_WARNS("Plugin") << "Bound port number unknown, bailing out." << LL_ENDL;
						killSockets();
						errorState();
						break;
					}
				}
				LL_DEBUGS("Plugin") << "Bound tcp socket to port: " << addr->port << LL_ENDL;
				status = apr_socket_opt_set(mListenSocket->getSocket(), APR_SO_NONBLOCK, 1);
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}
				apr_socket_timeout_set(mListenSocket->getSocket(), 0);
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}
				status = apr_socket_listen(
					mListenSocket->getSocket(),
					10);
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}
				setState(STATE_LISTENING);
			}
			break;
			case STATE_LISTENING:
			{
				std::stringstream stream;
				stream << mBoundPort;
				mProcess.addArgument(stream.str());
				// CEF 139 + Cloudflare advertises HTTP/3 (Alt-Svc / HTTPS DNS).
				// Dullahan then fails the login splash with ERR_SOCKET_NOT_CONNECTED (-15).
				if (mPluginFile.find("cef") != std::string::npos)
				{
					mProcess.addArgument("--disable-quic");
					mProcess.addArgument("--disable-features=UseDnsHttpsSvcb");
				}
				if(mProcess.launch() != 0)
				{
					errorState();
				}
				else
				{
					if(mDebug)
					{
						std::stringstream cmd;
					}
					mHeartbeat.start();
					mHeartbeat.setTimerExpirySec(mPluginLaunchTimeout);
					setState(STATE_LAUNCHED);
				}
			}
			break;
			case STATE_LAUNCHED:
				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
				else
				{
					if(accept())
					{
						mListenSocket.reset();
						setState(STATE_CONNECTED);
					}
				}
			break;
			case STATE_CONNECTED:
				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
			break;
			case STATE_HELLO:
				LL_DEBUGS("Plugin") << "received hello message" << LL_ENDL;
				{
					LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "load_plugin");
					message.setValue("file", mPluginFile);
					message.setValue("dir", mPluginDir);
					sendMessage(message);
				}
				setState(STATE_LOADING);
			break;
			case STATE_LOADING:
				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
			break;
			case STATE_RUNNING:
				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
			break;
			case STATE_EXITING:
				if(!mProcess.isRunning())
				{
					setState(STATE_CLEANUP);
				}
				else if(pluginLockedUp())
				{
					LL_WARNS("Plugin") << "timeout in exiting state, bailing out" << LL_ENDL;
					errorState();
				}
			break;
			case STATE_LAUNCH_FAILURE:
				if(mOwner != NULL)
				{
					mOwner->pluginLaunchFailed();
				}
				setState(STATE_CLEANUP);
			break;
			case STATE_ERROR:
				if(mOwner != NULL)
				{
					mOwner->pluginDied();
				}
				setState(STATE_CLEANUP);
			break;
			case STATE_CLEANUP:
				mProcess.kill();
				killSockets();
				setState(STATE_DONE);
			break;
			case STATE_DONE:
			break;
		}
	} while (idle_again);
}
bool LLPluginProcessParent::isLoading(void)
{
	bool result = false;
	if(mState <= STATE_LOADING)
		result = true;
	return result;
}
bool LLPluginProcessParent::isRunning(void)
{
	bool result = false;
	if(mState == STATE_RUNNING)
		result = true;
	return result;
}
bool LLPluginProcessParent::isDone(void)
{
	bool result = false;
	if(mState == STATE_DONE)
		result = true;
	return result;
}
void LLPluginProcessParent::setSleepTime(F64 sleep_time, bool force_send)
{
	if(force_send || (sleep_time != mSleepTime))
	{
		mSleepTime = sleep_time;
		if(canSendMessage())
		{
			LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "sleep_time");
			message.setValueReal("time", mSleepTime);
			sendMessage(message);
		}
		else
		{
		}
	}
}
void LLPluginProcessParent::sendMessage(const LLPluginMessage &message)
{
	if(message.hasValue("blocking_response"))
	{
		mBlocked = false;
		mHeartbeat.setTimerExpirySec(mPluginLockupTimeout);
	}
	if (message.hasValue("gorgon"))
	{
		mBlocked = true;
	}
	std::string buffer = message.generate();
#if LL_DEBUG
	if (message.getName() == "mouse_event")
	{
		LL_DEBUGS("PluginMouseEvent") << "Sending: " << buffer << LL_ENDL;
	}
	else
	{
		LL_DEBUGS("Plugin") << "Sending: " << buffer << LL_ENDL;
	}
#endif
	writeMessageRaw(buffer);
	if(mMessagePipe)
	{
		mMessagePipe->pumpOutput();
	}
}
void LLPluginProcessParent::setMessagePipe(LLPluginMessagePipe *message_pipe)
{
	bool update_pollset = false;
	if(mMessagePipe)
	{
		mPollFD.client_data = NULL;
		update_pollset = true;
	}
	if(message_pipe != NULL)
	{
		mPollFD.p = mPollFDPool();
		mPollFD.desc_type = APR_POLL_SOCKET;
		mPollFD.reqevents = APR_POLLIN|APR_POLLERR|APR_POLLHUP;
		mPollFD.rtnevents = 0;
		mPollFD.desc.s = mSocket->getSocket();
		mPollFD.client_data = (void*)this;
		update_pollset = true;
	}
	mMessagePipe = message_pipe;
	if(update_pollset)
	{
		dirtyPollSet();
	}
}
apr_status_t LLPluginProcessParent::socketError(apr_status_t error)
{
	mSocketError = error;
	if (APR_STATUS_IS_EPIPE(error))
	{
		errorState();
	}
	return error;
};
void LLPluginProcessParent::dirtyPollSet()
{
	sPollsetNeedsRebuild = true;
	if(sReadThread)
	{
		LL_DEBUGS("PluginPoll") << "unpausing read thread " << LL_ENDL;
		sReadThread->unpause();
	}
}
void LLPluginProcessParent::updatePollset()
{
	if(!sInstancesMutex)
	{
		return;
	}
	LLMutexLock lock(sInstancesMutex);
	if(sPollSet)
	{
		LL_DEBUGS("PluginPoll") << "destroying pollset " << sPollSet << LL_ENDL;
		apr_pollset_destroy(sPollSet);
		sPollSet = NULL;
		sPollSetPool.destroy();
	}
	std::list<LLPluginProcessParent*>::iterator iter;
	int count = 0;
	for(iter = sInstances.begin(); iter != sInstances.end(); iter++)
	{
		(*iter)->mPolledInput = false;
		if((*iter)->mPollFD.client_data)
		{
			++count;
		}
	}
	if(sUseReadThread && sReadThread && !sReadThread->isQuitting())
	{
		if(!sPollSet && (count > 0))
		{
#ifdef APR_POLLSET_NOCOPY
			sPollSetPool.create();
			apr_status_t status = apr_pollset_create(&sPollSet, count, sPollSetPool(), APR_POLLSET_NOCOPY);
			if(status != APR_SUCCESS)
			{
#endif
				LL_WARNS("PluginPoll") << "Couldn't create pollset.  Falling back to non-pollset mode." << LL_ENDL;
				sPollSet = NULL;
				sPollSetPool.destroy();
#ifdef APR_POLLSET_NOCOPY
			}
			else
			{
				LL_DEBUGS("PluginPoll") << "created pollset " << sPollSet << LL_ENDL;
				for(iter = sInstances.begin(); iter != sInstances.end(); iter++)
				{
					if((*iter)->mPollFD.client_data)
					{
						status = apr_pollset_add(sPollSet, &((*iter)->mPollFD));
						if(status == APR_SUCCESS)
						{
							(*iter)->mPolledInput = true;
						}
						else
						{
							LL_WARNS("PluginPoll") << "apr_pollset_add failed with status " << status << LL_ENDL;
						}
					}
				}
			}
#endif
		}
	}
}
void LLPluginProcessParent::setUseReadThread(bool use_read_thread)
{
	if(sUseReadThread != use_read_thread)
	{
		sUseReadThread = use_read_thread;
		if(sUseReadThread)
		{
			if(!sReadThread)
			{
				LL_INFOS("PluginPoll") << "creating read thread " << LL_ENDL;
				sPollsetNeedsRebuild = true;
				sReadThread = new LLPluginProcessParentPollThread;
				sReadThread->start();
			}
		}
		else
		{
			if(sReadThread)
			{
				LL_INFOS("PluginPoll") << "destroying read thread " << LL_ENDL;
				delete sReadThread;
				sReadThread = NULL;
			}
		}
	}
}
void LLPluginProcessParent::poll(F64 timeout)
{
	if(sPollsetNeedsRebuild || !sUseReadThread)
	{
		sPollsetNeedsRebuild = false;
		updatePollset();
	}
	if(sPollSet)
	{
		apr_status_t status;
		apr_int32_t count;
		const apr_pollfd_t *descriptors;
		status = apr_pollset_poll(sPollSet, (apr_interval_time_t)(timeout * 1000000), &count, &descriptors);
		if(status == APR_SUCCESS)
		{
			for(int i = 0; i < count; i++)
			{
				LLPluginProcessParent *self = (LLPluginProcessParent *)(descriptors[i].client_data);
				if(self)
				{
					bool valid = false;
					{
						LLMutexLock lock(sInstancesMutex);
						for(std::list<LLPluginProcessParent*>::iterator iter = sInstances.begin(); iter != sInstances.end(); ++iter)
						{
							if(*iter == self)
							{
								self->mIncomingQueueMutex.lock();
								valid = true;
								break;
							}
						}
					}
					if(valid)
					{
						self->servicePoll();
						self->mIncomingQueueMutex.unlock();
					}
					else
					{
						LL_DEBUGS("PluginPoll") << "detected deleted instance " << self << LL_ENDL;
					}
				}
			}
		}
		else if(APR_STATUS_IS_TIMEUP(status))
		{
		}
		else if(status == EBADF)
		{
			LL_DEBUGS("PluginPoll") << "apr_pollset_poll returned EBADF" << LL_ENDL;
		}
		else if(status != APR_SUCCESS)
		{
			LL_WARNS("PluginPoll") << "apr_pollset_poll failed with status " << status << LL_ENDL;
		}
	}
}
void LLPluginProcessParent::servicePoll()
{
	bool result = true;
	if(mMessagePipe)
	{
		result = mMessagePipe->pumpInput(0.0f);
	}
	if(!result)
	{
		apr_pollset_remove(sPollSet, &mPollFD);
		mPollFD.client_data = NULL;
	}
}
void LLPluginProcessParent::receiveMessageRaw(const std::string &message)
{
	LL_DEBUGS("PluginRaw") << "Received: " << message << LL_ENDL;
	LLPluginMessage parsed;
	if(parsed.parse(message) != -1)
	{
		if(parsed.hasValue("blocking_request"))
		{
			mBlocked = true;
		}
		if(parsed.hasValue("perseus"))
		{
			mBlocked = false;
			mHeartbeat.setTimerExpirySec(mPluginLockupTimeout);
		}
		if(mPolledInput)
		{
			receiveMessageEarly(parsed);
		}
		else
		{
			receiveMessage(parsed);
		}
	}
}
void LLPluginProcessParent::receiveMessageEarly(const LLPluginMessage &message)
{
	bool handled = false;
	std::string message_class = message.getClass();
	if(message_class == LLPLUGIN_MESSAGE_CLASS_INTERNAL)
	{
	}
	else
	{
		if(mOwner != NULL)
		{
			handled = mOwner->receivePluginMessageEarly(message);
		}
	}
	if(!handled)
	{
		mIncomingQueue.push(message);
	}
}
void LLPluginProcessParent::receiveMessage(const LLPluginMessage &message)
{
	std::string message_class = message.getClass();
	if(message_class == LLPLUGIN_MESSAGE_CLASS_INTERNAL)
	{
		std::string message_name = message.getName();
		if(message_name == "hello")
		{
			if(mState == STATE_CONNECTED)
			{
				setState(STATE_HELLO);
			}
			else
			{
				LL_WARNS("Plugin") << "received hello message in wrong state -- bailing out" << LL_ENDL;
				errorState();
			}
		}
		else if(message_name == "load_plugin_response")
		{
			if(mState == STATE_LOADING)
			{
				mPluginVersionString = message.getValue("plugin_version");
				LL_INFOS("Plugin") << "plugin version string: " << mPluginVersionString << LL_ENDL;
				mMessageClassVersions = message.getValueLLSD("versions");
				LLSD::map_iterator iter;
				for(iter = mMessageClassVersions.beginMap(); iter != mMessageClassVersions.endMap(); iter++)
				{
					LL_INFOS("Plugin") << "message class: " << iter->first << " -> version: " << iter->second.asString() << LL_ENDL;
				}
				llassert_always(mSleepTime != 0.f);
				setSleepTime(mSleepTime, true);
				setState(STATE_RUNNING);
			}
			else
			{
				LL_WARNS("Plugin") << "received load_plugin_response message in wrong state -- bailing out" << LL_ENDL;
				errorState();
			}
		}
		else if(message_name == "heartbeat")
		{
			mHeartbeat.setTimerExpirySec(mPluginLockupTimeout);
			mCPUUsage = message.getValueReal("cpu_usage");
			LL_DEBUGS("PluginHeartbeat") << "cpu usage reported as " << mCPUUsage << LL_ENDL;
		}
		else if(message_name == "shutdown")
		{
			LL_INFOS("Plugin") << "received shutdown message" << LL_ENDL;
			mReceivedShutdown = true;
			mOwner->receivedShutdown();
		}
		else if(message_name == "shm_add_response")
		{
		}
		else if(message_name == "shm_remove_response")
		{
			std::string name = message.getValue("name");
			sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
			if(iter != mSharedMemoryRegions.end())
			{
				iter->second->destroy();
				mSharedMemoryRegions.erase(iter);
			}
		}
		else if(message_name == "log_message")
		{
			std::string msg=message.getValue("message");
			S32 level=message.getValueS32("log_level");
			switch(level)
			{
				case LLPluginMessage::LOG_LEVEL_DEBUG:
					LL_DEBUGS("Plugin child")<<msg<<LL_ENDL;
					break;
				case LLPluginMessage::LOG_LEVEL_INFO:
					LL_INFOS("Plugin child")<<msg<<LL_ENDL;
					break;
				case LLPluginMessage::LOG_LEVEL_WARN:
					LL_WARNS("Plugin child")<<msg<<LL_ENDL;
					break;
				case LLPluginMessage::LOG_LEVEL_ERR:
					LL_ERRS("Plugin child")<<msg<<LL_ENDL;
					break;
				default:
					break;
			}
		}
		else
		{
			LL_WARNS("Plugin") << "Unknown internal message from child: " << message_name << LL_ENDL;
		}
	}
	else
	{
		if(mOwner != NULL)
		{
			mOwner->receivePluginMessage(message);
		}
	}
}
std::string LLPluginProcessParent::addSharedMemory(size_t size)
{
	std::string name;
	LLPluginSharedMemory *region = new LLPluginSharedMemory;
	if(region->create(size))
	{
		name = region->getName();
		mSharedMemoryRegions.insert(sharedMemoryRegionsType::value_type(name, region));
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_add");
		message.setValue("name", name);
		message.setValueS32("size", (S32)size);
		sendMessage(message);
	}
	else
	{
		LL_WARNS("Plugin") << "Couldn't create a shared memory segment!" << LL_ENDL;
		delete region;
	}
	return name;
}
void LLPluginProcessParent::removeSharedMemory(const std::string &name)
{
	sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
	if(iter != mSharedMemoryRegions.end())
	{
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_remove");
		message.setValue("name", name);
		sendMessage(message);
	}
	else
	{
		LL_WARNS("Plugin") << "Request to remove an unknown shared memory segment." << LL_ENDL;
	}
}
size_t LLPluginProcessParent::getSharedMemorySize(const std::string &name)
{
	size_t result = 0;
	sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
	if(iter != mSharedMemoryRegions.end())
	{
		result = iter->second->getSize();
	}
	return result;
}
void *LLPluginProcessParent::getSharedMemoryAddress(const std::string &name)
{
	void *result = NULL;
	sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
	if(iter != mSharedMemoryRegions.end())
	{
		result = iter->second->getMappedAddress();
	}
	return result;
}
std::string LLPluginProcessParent::getMessageClassVersion(const std::string &message_class)
{
	std::string result;
	if(mMessageClassVersions.has(message_class))
	{
		result = mMessageClassVersions[message_class].asString();
	}
	return result;
}
std::string LLPluginProcessParent::getPluginVersion(void)
{
	return mPluginVersionString;
}
void LLPluginProcessParent::setState(EState state)
{
	LL_DEBUGS("Plugin") << "setting state to " << stateToString(state) << LL_ENDL;
	mState = state;
};
bool LLPluginProcessParent::pluginLockedUpOrQuit()
{
	bool result = false;
	if(!mProcess.isRunning())
	{
		LL_WARNS("Plugin") << "child exited" << LL_ENDL;
		result = true;
	}
	else if(pluginLockedUp())
	{
		LL_WARNS("Plugin") << "timeout" << LL_ENDL;
		result = true;
	}
	return result;
}
bool LLPluginProcessParent::pluginLockedUp()
{
	if(mDisableTimeout || mDebug || mBlocked)
	{
		return false;
	}
	return (mHeartbeat.getStarted() && mHeartbeat.hasExpired());
}
std::string LLPluginProcessParent::stateToString(EState state)
{
	std::string eng = "unknown plugin state";
	switch (state)
	{
	case STATE_UNINITIALIZED:
		eng = "STATE_UNINITIALIZED";
		break;
	case STATE_INITIALIZED:
		eng = "STATE_INITIALIZED - init() has been called";
		break;
	case STATE_LISTENING:
		eng = "STATE_LISTENING - listening for incoming connection";
		break;
	case STATE_LAUNCHED:
		eng = "STATE_LAUNCHED - process has been launched";
		break;
	case STATE_CONNECTED:
		eng = "STATE_CONNECTED - process has connected";
		break;
	case STATE_HELLO:
		eng = "STATE_HELLO - first message from the plugin process has been received";
		break;
	case STATE_LOADING:
		eng = "STATE_LOADING - process has been asked to load the plugin";
		break;
	case STATE_RUNNING:
		eng = "STATE_RUNNING - plugin running";
		break;
	case STATE_LAUNCH_FAILURE:
		eng = "STATE_LAUNCH_FAILURE - failure before plugin loaded";
		break;
	case STATE_ERROR:
		eng = "STATE_ERROR - generic bailout state";
		break;
	case STATE_CLEANUP:
		eng = "STATE_CLEANUP - clean everything up";
		break;
	case STATE_EXITING:
		eng = "STATE_EXITING - tried to kill process, waiting for it to exit";
		break;
	case STATE_DONE:
		eng = "STATE_DONE - plugin done";
		break;
	default:
		break;
	}
	return llformat("(%d) ", (S32)state) + eng;
}
