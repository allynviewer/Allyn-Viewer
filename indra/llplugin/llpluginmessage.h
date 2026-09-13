/** 
 * @file llpluginmessage.h
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
#ifndef LL_LLPLUGINMESSAGE_H
#define LL_LLPLUGINMESSAGE_H
#include "llsd.h"
class LLPluginMessage
{
	LOG_CLASS(LLPluginMessage);
public:
	LLPluginMessage();
	LLPluginMessage(const LLPluginMessage &p);
	LLPluginMessage(const std::string &message_class, const std::string &message_name);
	~LLPluginMessage();
	void clear(void);
	void setMessage(const std::string &message_class, const std::string &message_name);
	void setValue(const std::string &key, const std::string &value);
	void setValueLLSD(const std::string &key, const LLSD &value);
	void setValueS32(const std::string &key, S32 value);
	void setValueU32(const std::string &key, U32 value);
	void setValueBoolean(const std::string &key, bool value);
	void setValueReal(const std::string &key, F64 value);
	void setValuePointer(const std::string &key, void *value);
	std::string getClass(void) const;
	std::string getName(void) const;
	bool hasValue(const std::string &key) const;
	std::string getValue(const std::string &key) const;
	LLSD getValueLLSD(const std::string &key) const;
	S32 getValueS32(const std::string &key) const;
	U32 getValueU32(const std::string &key) const;
	bool getValueBoolean(const std::string &key) const;
	F64 getValueReal(const std::string &key) const;
	void* getValuePointer(const std::string &key) const;
	std::string generate(void) const;
	int parse(const std::string &message);
	enum LLPLUGIN_LOG_LEVEL {
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_INFO,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERR,
	};
	friend std::ostream& operator<<(std::ostream& os, LLPluginMessage const& message) { return os << message.mMessage; }
private:
	LLSD mMessage;
};
class LLPluginMessageListener
{
public:
	virtual ~LLPluginMessageListener();
	virtual void receivePluginMessage(const LLPluginMessage &message) = 0;
};
class LLPluginMessageDispatcher
{
public:
	virtual ~LLPluginMessageDispatcher();
	void addPluginMessageListener(LLPluginMessageListener *);
	void removePluginMessageListener(LLPluginMessageListener *);
protected:
	void dispatchPluginMessage(const LLPluginMessage &message);
	typedef std::set<LLPluginMessageListener*> listener_set_t;
	listener_set_t mListeners;
};
#endif
