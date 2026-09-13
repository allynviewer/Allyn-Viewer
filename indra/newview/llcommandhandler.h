/**
 * @file llcommandhandler.h
 * @brief Central registry for text-driven "commands", most of
 * which manipulate user interface.  For example, the command
 * "agent (uuid) about" will open the UI for an avatar's profile.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
 */
#ifndef LLCOMMANDHANDLER_H
#define LLCOMMANDHANDLER_H
#include "llsd.h"
class LLMediaCtrl;
class LLCommandHandler
{
public:
	enum EUntrustedAccess
	{
		UNTRUSTED_ALLOW,
		UNTRUSTED_BLOCK,
		UNTRUSTED_THROTTLE
	};
	LLCommandHandler(const char* command, EUntrustedAccess untrusted_access);
	virtual ~LLCommandHandler();
	virtual bool handle(const LLSD& params,
						const LLSD& query_map,
						LLMediaCtrl* web) = 0;
};
class LLCommandDispatcher
{
public:
	static bool dispatch(const std::string& cmd,
						 const LLSD& params,
						 const LLSD& query_map,
						 LLMediaCtrl* web,
						 const std::string& nav_type,
						 bool trusted_browser);
	static LLSD enumerate();
};
#endif
