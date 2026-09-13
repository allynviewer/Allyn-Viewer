/** 
 * @file llmessagethrottle.cpp
 * @brief LLMessageThrottle class used for throttling messages.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "linden_common.h"
#include "llmessagethrottle.h"
#include "llframetimer.h"
#include <boost/functional/hash.hpp>
bool eq_message_throttle_entry(LLMessageThrottleEntry a, LLMessageThrottleEntry b)
 		{ return a.getHash() == b.getHash(); }
const U64 SEC_TO_USEC = 1000000;
const U64 MAX_MESSAGE_AGE[MTC_EOF] =
{
	10 * SEC_TO_USEC,
	10 * SEC_TO_USEC
};
LLMessageThrottle::LLMessageThrottle()
{
}
LLMessageThrottle::~LLMessageThrottle()
{
}
void LLMessageThrottle::pruneEntries()
{
	S32 cat;
	for (cat = 0; cat < MTC_EOF; cat++)
	{
		message_list_t* message_list = &(mMessageList[cat]);
		message_list_reverse_iterator_t r_iterator 	= message_list->rbegin();
		message_list_reverse_iterator_t r_last 		= message_list->rend();
		F32 max_age = (F32)MAX_MESSAGE_AGE[cat];
		BOOL found = FALSE;
		while (r_iterator != r_last && !found)
		{
			if ( LLFrameTimer::getTotalTime() - (*r_iterator).getEntryTime() < max_age )
			{
				found = TRUE;
				if (r_iterator != message_list->rbegin())
				{
					message_list->erase(r_iterator.base(), message_list->end());
				}
			}
			else
			{
				r_iterator++;
			}
		}
		if (!found)
		{
			message_list->clear();
		}
	}
}
BOOL LLMessageThrottle::addViewerAlert(const LLUUID& to, const std::string& mesg)
{
	message_list_t* message_list = &(mMessageList[MTC_VIEWER_ALERT]);
	std::ostringstream full_mesg;
	full_mesg << to << mesg;
	size_t hash = boost::hash<std::string>()(full_mesg.str());
	LLMessageThrottleEntry entry(hash, LLFrameTimer::getTotalTime());
 	message_list_iterator_t found = std::search_n(message_list->begin(), message_list->end(),
 												  1, entry, eq_message_throttle_entry);
	if (found == message_list->end())
	{
		message_list->push_front(entry);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
BOOL LLMessageThrottle::addAgentAlert(const LLUUID& agent, const LLUUID& task, const std::string& mesg)
{
	message_list_t* message_list = &(mMessageList[MTC_AGENT_ALERT]);
	std::ostringstream full_mesg;
	full_mesg << agent << task << mesg;
	size_t hash = boost::hash<std::string>()(full_mesg.str());
	LLMessageThrottleEntry entry(hash, LLFrameTimer::getTotalTime());
	message_list_iterator_t found = std::search_n(message_list->begin(), message_list->end(),
												  1, entry, eq_message_throttle_entry);
	if (found == message_list->end())
	{
		message_list->push_front(entry);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
