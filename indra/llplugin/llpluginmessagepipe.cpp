/** 
 * @file llpluginmessagepipe.cpp
 * @brief Classes that implement connections from the plugin system to pipes/pumps.
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
#include "llpluginmessagepipe.h"
#include "llbufferstream.h"
#include "llapr.h"
static const char MESSAGE_DELIMITER = '\0';
LLPluginMessagePipeOwner::LLPluginMessagePipeOwner() :
	mMessagePipe(NULL),
	mSocketError(APR_SUCCESS)
{
}
LLPluginMessagePipeOwner::~LLPluginMessagePipeOwner()
{
	killMessagePipe();
}
apr_status_t LLPluginMessagePipeOwner::socketError(apr_status_t error)
{
	mSocketError = error;
	return error;
};
void LLPluginMessagePipeOwner::setMessagePipe(LLPluginMessagePipe *read_pipe)
{
	mMessagePipe = read_pipe;
}
bool LLPluginMessagePipeOwner::canSendMessage(void)
{
	return (mMessagePipe != NULL);
}
bool LLPluginMessagePipeOwner::writeMessageRaw(const std::string &message)
{
	bool result = true;
	if(mMessagePipe != NULL)
	{
		result = mMessagePipe->addMessage(message);
	}
	else
	{
		LL_WARNS("Plugin") << "dropping message: " << message << LL_ENDL;
		result = false;
	}
	return result;
}
bool LLPluginMessagePipeOwner::flushMessages(void)
{
	bool result = true;
	if (mMessagePipe != NULL)
	{
		result = mMessagePipe->flushMessages();
	}
	return result;
}
void LLPluginMessagePipeOwner::killMessagePipe(void)
{
	if(mMessagePipe != NULL)
	{
		delete mMessagePipe;
		mMessagePipe = NULL;
	}
}
LLPluginMessagePipe::LLPluginMessagePipe(LLPluginMessagePipeOwner *owner, LLSocket::ptr_t socket):
	mOwner(owner),
	mSocket(socket)
{
	mOwner->setMessagePipe(this);
}
LLPluginMessagePipe::~LLPluginMessagePipe()
{
	if(mOwner != NULL)
	{
		mOwner->setMessagePipe(NULL);
	}
}
bool LLPluginMessagePipe::addMessage(const std::string &message)
{
	mOutputMutex.lock();
	mOutput += message;
	mOutput += MESSAGE_DELIMITER;
	mOutputMutex.unlock();
	return true;
}
void LLPluginMessagePipe::clearOwner(void)
{
	mOwner = NULL;
}
void LLPluginMessagePipe::setSocketTimeout(apr_interval_time_t timeout_usec)
{
	if(timeout_usec <= 0)
	{
		apr_socket_opt_set(mSocket->getSocket(), APR_SO_NONBLOCK, 1);
		apr_socket_timeout_set(mSocket->getSocket(), 0);
	}
	else
	{
		apr_socket_opt_set(mSocket->getSocket(), APR_SO_NONBLOCK, 1);
		apr_socket_timeout_set(mSocket->getSocket(), timeout_usec);
	}
}
bool LLPluginMessagePipe::pump(F64 timeout)
{
	bool result = pumpOutput();
	if(result)
	{
		result = pumpInput(timeout);
	}
	return result;
}
static apr_interval_time_t const flush_max_block_time = 10000000;
static apr_interval_time_t const flush_min_timeout = 1000;
static apr_interval_time_t const flush_max_timeout = 50000;
bool LLPluginMessagePipe::pumpOutput(bool flush)
{
	bool result = true;
	if(mSocket)
	{
		apr_interval_time_t flush_time_left_usec = flush_max_block_time;
		apr_interval_time_t timeout_usec = flush ? flush_min_timeout : 0;
		LLMutexLock lock(&mOutputMutex);
		while(result && !mOutput.empty())
		{
			apr_size_t size = (apr_size_t)mOutput.size();
			setSocketTimeout(timeout_usec);
			apr_status_t status = apr_socket_send(
					mSocket->getSocket(),
					(const char*)mOutput.data(),
					&size);
			if(status == APR_SUCCESS)
			{
				mOutput = mOutput.substr(size);
				break;
			}
			else if(APR_STATUS_IS_EAGAIN(status) || APR_STATUS_IS_TIMEUP(status))
			{
				mOutput = mOutput.substr(size);
				if (!flush)
					break;
				flush_time_left_usec -= timeout_usec;
				if (flush_time_left_usec <= 0)
				{
					result = false;
				}
				else if (size == 0)
				{
					timeout_usec = llmin(flush_max_timeout, 2 * timeout_usec);
				}
				else
				{
					timeout_usec = llmax(flush_min_timeout, timeout_usec / 2);
				}
			}
			else if(APR_STATUS_IS_EOF(status))
			{
				LL_INFOS() << "Got EOF from plugin socket. " << LL_ENDL;
				if(mOwner)
				{
					mOwner->socketError(status);
				}
				result = false;
			}
			else
			{
				ll_apr_warn_status(status);
				if(mOwner)
				{
					mOwner->socketError(status);
				}
				result = false;
			}
		}
	}
	return result;
}
bool LLPluginMessagePipe::pumpInput(F64 timeout)
{
	bool result = true;
	if(mSocket)
	{
		apr_status_t status;
		apr_size_t size;
#if LL_WINDOWS
		if(result)
		{
			if(timeout != 0.0f)
			{
				ms_sleep((int)(timeout * 1000.0f));
				timeout = 0.0f;
			}
		}
#endif
		if(result)
		{
			char input_buf[1024];
			apr_size_t request_size;
			if(timeout == 0.0f)
			{
				request_size = sizeof(input_buf);
			}
			else
			{
				request_size = 1;
			}
			setSocketTimeout((apr_interval_time_t)(timeout * 1000000));
			while(1)
			{
				size = request_size;
				status = apr_socket_recv(
						mSocket->getSocket(),
						input_buf,
						&size);
				if(size > 0)
				{
					LLMutexLock lock(&mInputMutex);
					mInput.append(input_buf, size);
				}
				if(status == APR_SUCCESS)
				{
					LL_DEBUGS("PluginSocket") << "success, read " << size << LL_ENDL;
					if(size != request_size)
					{
						break;
					}
				}
				else if(APR_STATUS_IS_TIMEUP(status))
				{
					LL_DEBUGS("PluginSocket") << "TIMEUP, read " << size << LL_ENDL;
					break;
				}
				else if(APR_STATUS_IS_EAGAIN(status))
				{
					LL_DEBUGS("PluginSocket") << "EAGAIN, read " << size << LL_ENDL;
					break;
				}
				else if(APR_STATUS_IS_EOF(status))
				{
					LL_INFOS("PluginSocket") << "Got EOF from plugin socket. " << LL_ENDL;
					if(mOwner)
					{
						mOwner->socketError(status);
					}
					result = false;
					break;
				}
				else
				{
					ll_apr_warn_status(status);
					if(mOwner)
					{
						mOwner->socketError(status);
					}
					result = false;
					break;
				}
				if(timeout != 0.0f)
				{
					setSocketTimeout(0);
					request_size = sizeof(input_buf);
				}
			}
			processInput();
		}
	}
	return result;
}
void LLPluginMessagePipe::processInput(void)
{
	int delim;
	mInputMutex.lock();
	while((delim = mInput.find(MESSAGE_DELIMITER)) != std::string::npos)
	{
		if (mOwner)
		{
			std::string message(mInput, 0, delim);
			mInput.erase(0, delim + 1);
			mInputMutex.unlock();
			mOwner->receiveMessageRaw(message);
			mInputMutex.lock();
		}
		else
		{
			LL_WARNS("Plugin") << "!mOwner" << LL_ENDL;
		}
	}
	mInputMutex.unlock();
}
