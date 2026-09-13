/** 
 * @file llplugininstance.h
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
#ifndef LL_LLPLUGININSTANCE_H
#define LL_LLPLUGININSTANCE_H
#include "llstring.h"
struct apr_dso_handle_t;
class LLPluginInstanceMessageListener
{
public:
	virtual ~LLPluginInstanceMessageListener();
	virtual void receivePluginMessage(const std::string &message) = 0;
};
class BasicPluginBase;
class LLPluginInstance
{
	LOG_CLASS(LLPluginInstance);
public:
	LLPluginInstance(LLPluginInstanceMessageListener *owner);
	virtual ~LLPluginInstance();
	int load(const std::string& plugin_dir, std::string &plugin_file);
	void sendMessage(const std::string &message);
	void idle(void);
	typedef void (*receiveMessageFunction)(char const* message_string, BasicPluginBase** plugin_object);
	typedef void (*sendMessageFunction)(char const* message_string, LLPluginInstance** plugin_instance);
	typedef int (*pluginInitFunction)(sendMessageFunction send_message_function, LLPluginInstance* plugin_instance, receiveMessageFunction* receive_message_function, BasicPluginBase** plugin_object);
	static const char *PLUGIN_INIT_FUNCTION_NAME;
private:
	static void staticReceiveMessage(char const* message_string, LLPluginInstance** plugin_instance);
	void receiveMessage(const char *message_string);
	apr_dso_handle_t *mDSOHandle;
	BasicPluginBase* mPluginObject;
	receiveMessageFunction mReceiveMessageFunction;
	LLPluginInstanceMessageListener *mOwner;
};
#endif
