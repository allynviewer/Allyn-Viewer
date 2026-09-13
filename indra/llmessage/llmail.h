/** 
 * @file llmail.h
 * @brief smtp helper functions.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LL_LLMAIL_H
#define LL_LLMAIL_H
typedef struct apr_pool_t apr_pool_t;
#include "llsd.h"
class LLMail
{
public:
	static void init(const std::string& hostname, apr_pool_t* pool);
	static void enable(bool mail_enabled);
	static BOOL send(
		const char* from_name,
		const char* from_address,
		const char* to_name,
		const char* to_address,
		const char* subject,
		const char* message,
		const LLSD& headers = LLSD());
	static std::string buildSMTPTransaction(
		const char* from_name,
		const char* from_address,
		const char* to_name,
		const char* to_address,
		const char* subject,
		const LLSD& headers = LLSD());
	static bool send(
		const std::string& header,
		const std::string& message,
		const char* from_address,
		const char* to_address);
	static std::string encryptIMEmailAddress(
		const LLUUID& from_agent_id,
		const LLUUID& to_agent_id,
		U32 time,
		const U8* secret,
		size_t secret_size);
};
extern const size_t LL_MAX_KNOWN_GOOD_MAIL_SIZE;
#endif
