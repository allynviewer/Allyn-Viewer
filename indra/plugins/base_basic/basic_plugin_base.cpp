/** 
 * @file basic_plugin_base.cpp
 * @brief Basic plugin base class for Basic API plugin system
 *
 * All plugins should be a subclass of BasicPluginBase. 
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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
#include "linden_common.h"
#include "basic_plugin_base.h"
#ifdef WIN32
#include <windows.h>
#endif
BasicPluginBase* BasicPluginBase::sPluginBase;
BasicPluginBase::BasicPluginBase(LLPluginInstance::sendMessageFunction send_message_function, LLPluginInstance* plugin_instance) :
		mPluginInstance(plugin_instance), mSendMessageFunction(send_message_function), mDeleteMe(false)
{
	llassert(!sPluginBase);
	sPluginBase = this;
}
void BasicPluginBase::staticReceiveMessage(char const* message_string, BasicPluginBase** self_ptr)
{
  	BasicPluginBase* self = *self_ptr;
	if(self != NULL)
	{
		self->receiveMessage(message_string);
		if(self->mDeleteMe)
		{
			delete self;
			*self_ptr = NULL;
		}
	}
}
void BasicPluginBase::sendMessage(const LLPluginMessage &message)
{
	std::string output = message.generate();
	PLS_DEBUGS << "BasicPluginBase::sendMessage: Sending: " << output << PLS_ENDL;
	mSendMessageFunction(output.c_str(), &mPluginInstance);
}
void BasicPluginBase::sendLogMessage(std::string const& message, LLPluginMessage::LLPLUGIN_LOG_LEVEL level)
{
	LLPluginMessage logmessage(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "log_message");
	logmessage.setValue("message", message);
	logmessage.setValueS32("log_level",level);
	mSendMessageFunction(logmessage.generate().c_str(), &mPluginInstance);
}
void BasicPluginBase::flushMessages(void)
{
	LLPluginMessage logmessage(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "flush");
	mSendMessageFunction(logmessage.generate().c_str(), &mPluginInstance);
}
void BasicPluginBase::sendShutdownMessage(void)
{
	flushMessages();
	LLPluginMessage shutdownmessage(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shutdown");
	sendMessage(shutdownmessage);
}
#if LL_WINDOWS
# define LLSYMEXPORT __declspec(dllexport)
#else
# define LLSYMEXPORT
#endif
extern "C"
{
	LLSYMEXPORT int LLPluginInitEntryPoint(LLPluginInstance::sendMessageFunction send_message_function,
										   LLPluginInstance* plugin_instance,
										   LLPluginInstance::receiveMessageFunction* receive_message_function,
										   BasicPluginBase** plugin_object);
}
LLSYMEXPORT int
LLPluginInitEntryPoint(LLPluginInstance::sendMessageFunction send_message_function,
					   LLPluginInstance* plugin_instance,
					   LLPluginInstance::receiveMessageFunction* receive_message_function,
					   BasicPluginBase** plugin_object)
{
	*receive_message_function = BasicPluginBase::staticReceiveMessage;
	return create_plugin(send_message_function, plugin_instance, plugin_object);
}
#ifdef WIN32
int WINAPI DllEntryPoint( HINSTANCE hInstance, unsigned long reason, void* params )
{
	return 1;
}
#endif
