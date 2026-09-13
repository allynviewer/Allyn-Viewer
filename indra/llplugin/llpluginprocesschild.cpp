/** 
 * @file llpluginprocesschild.cpp
 * @brief LLPluginProcessChild handles the child side of the external-process plugin API. 
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
#include "llpluginprocesschild.h"
#include "llplugininstance.h"
#include "llpluginmessagepipe.h"
#include "llpluginmessageclasses.h"
static const F32 HEARTBEAT_SECONDS = 1.0f;
static const F32 PLUGIN_IDLE_SECONDS = 1.0f / 100.0f;
LLPluginProcessChild::LLPluginProcessChild()
{
	mState = STATE_UNINITIALIZED;
	mInstance = NULL;
	mSocket = LLSocket::create(LLSocket::STREAM_TCP);
	mSleepTime = PLUGIN_IDLE_SECONDS;
	mCPUElapsed = 0.0f;
	mBlockingRequest = false;
	mBlockingResponseReceived = false;
}
LLPluginProcessChild::~LLPluginProcessChild()
{
	if(mInstance != NULL)
	{
		sendMessageToPlugin(LLPluginMessage("base", "cleanup"));
		exit( 0 );
	}
}
void LLPluginProcessChild::killSockets(void)
{
	killMessagePipe();
	mSocket.reset();
}
void LLPluginProcessChild::init(U32 launcher_port)
{
	mLauncherHost = LLHost("127.0.0.1", launcher_port);
	setState(STATE_INITIALIZED);
}
void LLPluginProcessChild::idle(void)
{
	bool idle_again;
	do
	{
		if(APR_STATUS_IS_EOF(mSocketError))
		{
			setState(STATE_ERROR);
		}
		else if(mSocketError != APR_SUCCESS)
		{
			LL_INFOS("Plugin") << "message pipe is in error state (" << mSocketError << "), moving to STATE_ERROR"<< LL_ENDL;
			setState(STATE_ERROR);
		}
		if((mState > STATE_INITIALIZED) && (mMessagePipe == NULL))
		{
			LL_INFOS("Plugin") << "message pipe went away, moving to STATE_ERROR"<< LL_ENDL;
			setState(STATE_ERROR);
		}
		idle_again = false;
		if(mInstance != NULL)
		{
			mInstance->idle();
		}
		switch(mState)
		{
			case STATE_UNINITIALIZED:
			break;
			case STATE_INITIALIZED:
				if(mSocket->blockingConnect(mLauncherHost))
				{
					new LLPluginMessagePipe(this, mSocket);
					setState(STATE_CONNECTED);
				}
				else
				{
					setState(STATE_ERROR);
				}
			break;
			case STATE_CONNECTED:
				sendMessageToParent(LLPluginMessage(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "hello"));
				setState(STATE_PLUGIN_LOADING);
			break;
			case STATE_PLUGIN_LOADING:
				if(!mPluginFile.empty())
				{
					mInstance = new LLPluginInstance(this);
					if(mInstance->load(mPluginDir, mPluginFile) == 0)
					{
						mHeartbeat.start();
						mHeartbeat.setTimerExpirySec(HEARTBEAT_SECONDS);
						mCPUElapsed = 0.0f;
						setState(STATE_PLUGIN_LOADED);
					}
					else
					{
						setState(STATE_ERROR);
					}
				}
			break;
			case STATE_PLUGIN_LOADED:
				{
					setState(STATE_PLUGIN_INITIALIZING);
					LLPluginMessage message("base", "init");
					sendMessageToPlugin(message);
				}
			break;
			case STATE_PLUGIN_INITIALIZING:
			break;
			case STATE_RUNNING:
				if(mInstance != NULL)
				{
					LLPluginMessage message("base", "idle");
					message.setValueReal("time", PLUGIN_IDLE_SECONDS);
					sendMessageToPlugin(message);
					mInstance->idle();
					if(mHeartbeat.hasExpired())
					{
						LLPluginMessage heartbeat(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "heartbeat");
						heartbeat.setValueReal("cpu_usage", mCPUElapsed / mHeartbeat.getElapsedTimeF64());
						sendMessageToParent(heartbeat);
						mHeartbeat.reset();
						mHeartbeat.setTimerExpirySec(HEARTBEAT_SECONDS);
						mCPUElapsed = 0.0f;
					}
				}
			break;
			case STATE_UNLOADING:
				if(mInstance != NULL)
				{
					sendMessageToPlugin(LLPluginMessage("base", "cleanup"));
					delete mInstance;
					mInstance = NULL;
				}
				setState(STATE_UNLOADED);
			break;
			case STATE_UNLOADED:
				killSockets();
				setState(STATE_DONE);
			break;
			case STATE_ERROR:
				killSockets();
				setState(STATE_DONE);
			break;
			case STATE_DONE:
				LL_WARNS("Plugin") << "Calling LLPluginProcessChild::idle while in STATE_DONE!" << LL_ENDL;
			break;
		}
	} while (idle_again);
}
void LLPluginProcessChild::sleep(F64 seconds)
{
	deliverQueuedMessages();
	if(mMessagePipe)
	{
		mMessagePipe->pump(seconds);
	}
	else
	{
		ms_sleep((int)(seconds * 1000.0f));
	}
}
void LLPluginProcessChild::pump(void)
{
	deliverQueuedMessages();
	if(mMessagePipe)
	{
		mMessagePipe->pump(0.0f);
	}
	else
	{
	}
}
bool LLPluginProcessChild::isRunning(void)
{
	bool result = false;
	if(mState == STATE_RUNNING)
		result = true;
	return result;
}
bool LLPluginProcessChild::isDone(void)
{
	bool result = false;
	switch(mState)
	{
		case STATE_DONE:
		result = true;
		break;
		default:
		break;
	}
	return result;
}
void LLPluginProcessChild::sendMessageToPlugin(const LLPluginMessage &message)
{
	if (mInstance)
	{
		std::string buffer = message.generate();
		LL_DEBUGS("Plugin") << "Sending to plugin: " << buffer << LL_ENDL;
		LLTimer elapsed;
		mInstance->sendMessage(buffer);
		mCPUElapsed += elapsed.getElapsedTimeF64();
	}
	else
	{
		LL_WARNS("Plugin") << "mInstance == NULL" << LL_ENDL;
	}
}
void LLPluginProcessChild::sendMessageToParent(const LLPluginMessage &message)
{
	std::string buffer = message.generate();
	LL_DEBUGS("Plugin") << "Sending to parent: " << buffer << LL_ENDL;
	writeMessageRaw(buffer);
}
void LLPluginProcessChild::receiveMessageRaw(const std::string &message)
{
	LL_DEBUGS("Plugin") << "Received from parent: " << message << LL_ENDL;
	LLPluginMessage parsed;
	parsed.parse(message);
	if(mBlockingRequest)
	{
		if(parsed.hasValue("blocking_response"))
		{
			mBlockingResponseReceived = true;
		}
		else
		{
			mMessageQueue.push(message);
			return;
		}
	}
	bool passMessage = true;
	{
		std::string message_class = parsed.getClass();
		if(message_class == LLPLUGIN_MESSAGE_CLASS_INTERNAL)
		{
			passMessage = false;
			std::string message_name = parsed.getName();
			if(message_name == "load_plugin")
			{
				mPluginFile = parsed.getValue("file");
				mPluginDir = parsed.getValue("dir");
			}
			else if(message_name == "shm_add")
			{
				std::string name = parsed.getValue("name");
				size_t size = (size_t)parsed.getValueS32("size");
				sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
				if(iter != mSharedMemoryRegions.end())
				{
					LL_WARNS("Plugin") << "Adding a duplicate shared memory segment!" << LL_ENDL;
				}
				else
				{
					LLPluginSharedMemory *region = new LLPluginSharedMemory;
					if(region->attach(name, size))
					{
						mSharedMemoryRegions.insert(sharedMemoryRegionsType::value_type(name, region));
						std::stringstream addr;
						addr << region->getMappedAddress();
						LLPluginMessage message("base", "shm_added");
						message.setValue("name", name);
						message.setValueS32("size", (S32)size);
						message.setValuePointer("address", region->getMappedAddress());
						sendMessageToPlugin(message);
						message.setMessage(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_add_response");
						message.setValue("name", name);
						sendMessageToParent(message);
					}
					else
					{
						LL_WARNS("Plugin") << "Couldn't create a shared memory segment!" << LL_ENDL;
						delete region;
					}
				}
			}
			else if(message_name == "shm_remove")
			{
				std::string name = parsed.getValue("name");
				sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
				if(iter != mSharedMemoryRegions.end())
				{
					LLPluginMessage message("base", "shm_remove");
					message.setValue("name", name);
					sendMessageToPlugin(message);
				}
				else
				{
					LL_WARNS("Plugin") << "shm_remove for unknown memory segment!" << LL_ENDL;
				}
			}
			else if(message_name == "sleep_time")
			{
				mSleepTime = llmax(parsed.getValueReal("time"), 1.0 / 100.0);
			}
			else if(message_name == "crash")
			{
				LL_ERRS("Plugin") << "Plugin crash requested." << LL_ENDL;
			}
			else if(message_name == "hang")
			{
				LL_WARNS("Plugin") << "Plugin hang requested." << LL_ENDL;
				while(1)
				{
				}
			}
			else
			{
				LL_WARNS("Plugin") << "Unknown internal message from parent: " << message_name << LL_ENDL;
			}
		}
	}
	if(passMessage && mInstance != NULL)
	{
		LLTimer elapsed;
		mInstance->sendMessage(message);
		mCPUElapsed += elapsed.getElapsedTimeF64();
	}
}
void LLPluginProcessChild::receivePluginMessage(const std::string &message)
{
	LL_DEBUGS("Plugin") << "Received from plugin: " << message << LL_ENDL;
	if(mBlockingRequest)
	{
		LL_ERRS("Plugin") << "Can't send a message while already waiting on a blocking request -- aborting!" << LL_ENDL;
	}
	bool passMessage = true;
	{
		LLPluginMessage parsed;
		parsed.parse(message);
		if(parsed.hasValue("blocking_request"))
		{
			mBlockingRequest = true;
		}
		std::string message_class = parsed.getClass();
		if(message_class == "base")
		{
			std::string message_name = parsed.getName();
			if(message_name == "init_response")
			{
				setState(STATE_RUNNING);
				passMessage = false;
				LLPluginMessage new_message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "load_plugin_response");
				LLSD versions = parsed.getValueLLSD("versions");
				new_message.setValueLLSD("versions", versions);
				if(parsed.hasValue("plugin_version"))
				{
					std::string plugin_version = parsed.getValue("plugin_version");
					new_message.setValueLLSD("plugin_version", plugin_version);
				}
				sendMessageToParent(new_message);
			}
			else if(message_name == "shm_remove_response")
			{
				passMessage = false;
				std::string name = parsed.getValue("name");
				sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
				if(iter != mSharedMemoryRegions.end())
				{
					iter->second->detach();
					mSharedMemoryRegions.erase(iter);
					LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_remove_response");
					message.setValue("name", name);
					sendMessageToParent(message);
				}
				else
				{
					LL_WARNS("Plugin") << "shm_remove_response for unknown memory segment!" << LL_ENDL;
				}
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_INTERNAL)
		{
			bool flush = false;
			std::string message_name = parsed.getName();
			if(message_name == "shutdown")
			{
				setState(STATE_UNLOADING);
				flush = true;
			}
			else if (message_name == "flush")
			{
				flush = true;
				passMessage = false;
			}
			if (flush)
			{
				flushMessages();
			}
		}
	}
	if(passMessage)
	{
		LL_DEBUGS("Plugin") << "Passing through to parent: " << message << LL_ENDL;
		writeMessageRaw(message);
	}
	while(mBlockingRequest)
	{
		sleep(mSleepTime);
		if(mBlockingResponseReceived || mSocketError != APR_SUCCESS || (mMessagePipe == NULL))
		{
			mBlockingRequest = false;
			mBlockingResponseReceived = false;
		}
	}
}
void LLPluginProcessChild::setState(EState state)
{
	LL_DEBUGS("Plugin") << "setting state to " << state << LL_ENDL;
	mState = state;
};
void LLPluginProcessChild::deliverQueuedMessages()
{
	if(!mBlockingRequest)
	{
		while(!mMessageQueue.empty())
		{
			receiveMessageRaw(mMessageQueue.front());
			mMessageQueue.pop();
		}
	}
}
