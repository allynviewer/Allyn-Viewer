/** 
 * @file llplugininstance.cpp
 * @brief LLPluginInstance handles loading the dynamic library of a plugin and setting up its entry points for message passing.
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
#include "llplugininstance.h"
#include "llaprpool.h"
#if LL_WINDOWS
#include "direct.h"
#endif
LLPluginInstanceMessageListener::~LLPluginInstanceMessageListener()
{
}
const char *LLPluginInstance::PLUGIN_INIT_FUNCTION_NAME = "LLPluginInitEntryPoint";
LLPluginInstance::LLPluginInstance(LLPluginInstanceMessageListener *owner) :
	mDSOHandle(NULL),
	mPluginObject(NULL),
	mReceiveMessageFunction(NULL)
{
	mOwner = owner;
}
LLPluginInstance::~LLPluginInstance()
{
	if(mDSOHandle != NULL)
	{
		apr_dso_unload(mDSOHandle);
		mDSOHandle = NULL;
	}
}
int LLPluginInstance::load(const std::string& plugin_dir, std::string &plugin_file)
{
	pluginInitFunction init_function = NULL;
	if ( plugin_dir.length() )
	{
#if LL_WINDOWS
		_chdir( plugin_dir.c_str() );
#endif
	};
	int result = apr_dso_load(&mDSOHandle,
					  plugin_file.c_str(),
					  LLAPRRootPool::get()());
	if(result != APR_SUCCESS)
	{
		char buf[1024];
		{
			apr_dso_error(mDSOHandle, buf, sizeof(buf));
		}
		LL_WARNS("Plugin") << "apr_dso_load of " << plugin_file << " failed with error " << result << " , additional info string: " << buf << LL_ENDL;
	}
	if(result == APR_SUCCESS)
	{
		result = apr_dso_sym((apr_dso_handle_sym_t*)&init_function,
						 mDSOHandle,
						 PLUGIN_INIT_FUNCTION_NAME);
		if(result != APR_SUCCESS)
		{
			LL_WARNS("Plugin") << "apr_dso_sym failed with error " << result << LL_ENDL;
		}
	}
	if(result == APR_SUCCESS)
	{
		result = init_function(&LLPluginInstance::staticReceiveMessage, this, &mReceiveMessageFunction, &mPluginObject);
		if(result != 0)
		{
			LL_WARNS("Plugin") << "call to init function failed with error " << result << LL_ENDL;
		}
	}
	return (int)result;
}
void LLPluginInstance::sendMessage(const std::string &message)
{
	if(mReceiveMessageFunction)
	{
		LL_DEBUGS("Plugin") << "sending message to plugin: \"" << message << "\"" << LL_ENDL;
		mReceiveMessageFunction(message.c_str(), &mPluginObject);
	}
	else
	{
		LL_WARNS("Plugin") << "dropping message: \"" << message << "\"" << LL_ENDL;
	}
}
void LLPluginInstance::idle(void)
{
}
void LLPluginInstance::staticReceiveMessage(char const* message_string, LLPluginInstance** self_ptr)
{
	(*self_ptr)->receiveMessage(message_string);
}
void LLPluginInstance::receiveMessage(const char *message_string)
{
	if(mOwner)
	{
		LL_DEBUGS("Plugin") << "processing incoming message: \"" << message_string << "\"" << LL_ENDL;
		mOwner->receivePluginMessage(message_string);
	}
	else
	{
		LL_WARNS("Plugin") << "dropping incoming message: \"" << message_string << "\"" << LL_ENDL;
	}
}
