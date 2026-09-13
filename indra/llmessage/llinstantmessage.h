/** 
 * @file llinstantmessage.h
 * @brief Constants and declarations used by instant messages. 
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLINSTANTMESSAGE_H
#define LL_LLINSTANTMESSAGE_H
#include "llhost.h"
#include "lluuid.h"
#include "llsd.h"
#include "llrefcount.h"
#include "llpointer.h"
#include "v3math.h"
class LLMessageSystem;
enum EInstantMessage
{
	IM_NOTHING_SPECIAL = 0,
	IM_MESSAGEBOX = 1,
	IM_GROUP_INVITATION = 3,
	IM_INVENTORY_OFFERED = 4,
	IM_INVENTORY_ACCEPTED = 5,
	IM_INVENTORY_DECLINED = 6,
	IM_GROUP_VOTE = 7,
	IM_GROUP_MESSAGE_DEPRECATED = 8,
	IM_TASK_INVENTORY_OFFERED = 9,
	IM_TASK_INVENTORY_ACCEPTED = 10,
	IM_TASK_INVENTORY_DECLINED = 11,
	IM_NEW_USER_DEFAULT = 12,
	IM_SESSION_INVITE = 13,
	IM_SESSION_P2P_INVITE = 14,
	IM_SESSION_GROUP_START = 15,
	IM_SESSION_CONFERENCE_START = 16,
	IM_SESSION_SEND = 17,
	IM_SESSION_LEAVE = 18,
	IM_FROM_TASK = 19,
	IM_BUSY_AUTO_RESPONSE = 20,
	IM_CONSOLE_AND_CHAT_HISTORY = 21,
	IM_LURE_USER = 22,
	IM_LURE_ACCEPTED = 23,
	IM_LURE_DECLINED = 24,
	IM_GODLIKE_LURE_USER = 25,
	IM_TELEPORT_REQUEST = 26,
	IM_GROUP_ELECTION_DEPRECATED = 27,
	IM_GOTO_URL = 28,
	IM_FROM_TASK_AS_ALERT = 31,
	IM_GROUP_NOTICE = 32,
	IM_GROUP_NOTICE_INVENTORY_ACCEPTED = 33,
	IM_GROUP_NOTICE_INVENTORY_DECLINED = 34,
	IM_GROUP_INVITATION_ACCEPT = 35,
	IM_GROUP_INVITATION_DECLINE = 36,
	IM_GROUP_NOTICE_REQUESTED = 37,
	IM_FRIENDSHIP_OFFERED = 38,
	IM_FRIENDSHIP_ACCEPTED = 39,
	IM_FRIENDSHIP_DECLINED_DEPRECATED = 40,
	IM_TYPING_START = 41,
	IM_TYPING_STOP = 42,
	IM_COUNT
};
enum EGodlikeRequest
{
	GOD_WANTS_NOTHING,
	GOD_WANTS_PHYSICS_INFO,
	GOD_WANTS_FOO,
	GOD_WANTS_BAR,
	GOD_WANTS_TERRAIN_SAVE,
	GOD_WANTS_TERRAIN_LOAD,
	GOD_WANTS_TOGGLE_AVATAR_GEOMETRY,
	GOD_WANTS_TELEHUB_INFO,
	GOD_WANTS_CONNECT_TELEHUB,
	GOD_WANTS_DELETE_TELEHUB,
	GOD_WANTS_ADD_TELEHUB_SPAWNPOINT,
	GOD_WANTS_REMOVE_TELEHUB_SPAWNPOINT,
};
enum EIMSource
{
	IM_FROM_VIEWER,
	IM_FROM_DATASERVER,
	IM_FROM_SIM
};
extern const U8 IM_ONLINE;
extern const U8 IM_OFFLINE;
extern const S32 VOTE_YES;
extern const S32 VOTE_NO;
extern const S32 VOTE_ABSTAIN;
extern const S32 VOTE_MAJORITY;
extern const S32 VOTE_SUPER_MAJORITY;
extern const S32 VOTE_UNANIMOUS;
extern const char EMPTY_BINARY_BUCKET[];
extern const S32 EMPTY_BINARY_BUCKET_SIZE;
extern const U32 NO_TIMESTAMP;
extern const std::string SYSTEM_FROM;
extern const std::string INTERACTIVE_SYSTEM_FROM;
extern const S32 IM_TTL;
void pack_instant_message(
	LLMessageSystem* msgsystem,
	const LLUUID& from_id,
	BOOL from_group,
	const LLUUID& session_id,
	const LLUUID& to_id,
	const std::string& name,
	const std::string& message,
	U8 offline = IM_ONLINE,
	EInstantMessage dialog = IM_NOTHING_SPECIAL,
	const LLUUID& id = LLUUID::null,
	U32 parent_estate_id = 0,
	const LLUUID& region_id = LLUUID::null,
	const LLVector3& position = LLVector3::zero,
	U32 timestamp = NO_TIMESTAMP,
	const U8* binary_bucket = (U8*)EMPTY_BINARY_BUCKET,
	S32 binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE);
void pack_instant_message_block(
	LLMessageSystem* msgsystem,
	const LLUUID& from_id,
	BOOL from_group,
	const LLUUID& session_id,
	const LLUUID& to_id,
	const std::string& name,
	const std::string& message,
	U8 offline = IM_ONLINE,
	EInstantMessage dialog = IM_NOTHING_SPECIAL,
	const LLUUID& id = LLUUID::null,
	U32 parent_estate_id = 0,
	const LLUUID& region_id = LLUUID::null,
	const LLVector3& position = LLVector3::zero,
	U32 timestamp = NO_TIMESTAMP,
	const U8* binary_bucket = (U8*)EMPTY_BINARY_BUCKET,
	S32 binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE);
#endif
