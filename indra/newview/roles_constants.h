/** 
 * @file roles_constants.h
 * @brief General Roles Constants
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#ifndef LL_ROLES_CONSTANTS_H
#define LL_ROLES_CONSTANTS_H
const S32 MAX_ROLES = 10;
enum LLRoleMemberChangeType
{
	RMC_ADD,
	RMC_REMOVE,
	RMC_NONE
};
enum LLRoleChangeType
{
	RC_UPDATE_NONE,
	RC_UPDATE_DATA,
	RC_UPDATE_POWERS,
	RC_UPDATE_ALL,
	RC_CREATE,
	RC_DELETE
};
const U64 GP_NO_POWERS = 0x0;
const U64 GP_ALL_POWERS = 0xFFFFFFFFFFFFFFFFLL;
const U64 GP_MEMBER_INVITE		= 0x1 << 1;
const U64 GP_MEMBER_EJECT			= 0x1 << 2;
const U64 GP_MEMBER_OPTIONS		= 0x1 << 3;
const U64 GP_MEMBER_VISIBLE_IN_DIR = 0x1LL << 47;
const U64 GP_ROLE_CREATE			= 0x1 << 4;
const U64 GP_ROLE_DELETE			= 0x1 << 5;
const U64 GP_ROLE_PROPERTIES		= 0x1 << 6;
const U64 GP_ROLE_ASSIGN_MEMBER_LIMITED = 0x1 << 7;
const U64 GP_ROLE_ASSIGN_MEMBER	= 0x1 << 8;
const U64 GP_ROLE_REMOVE_MEMBER	= 0x1 << 9;
const U64 GP_ROLE_CHANGE_ACTIONS	= 0x1 << 10;
const U64 GP_GROUP_CHANGE_IDENTITY = 0x1 << 11;
const U64 GP_LAND_DEED			= 0x1 << 12;
const U64 GP_LAND_RELEASE			= 0x1 << 13;
const U64 GP_LAND_SET_SALE_INFO	= 0x1 << 14;
const U64 GP_LAND_DIVIDE_JOIN		= 0x1 << 15;
const U64 GP_LAND_FIND_PLACES		= 0x1 << 17;
const U64 GP_LAND_CHANGE_IDENTITY = 0x1 << 18;
const U64 GP_LAND_SET_LANDING_POINT = 0x1 << 19;
const U64 GP_LAND_CHANGE_MEDIA	= 0x1 << 20;
const U64 GP_LAND_EDIT			= 0x1 << 21;
const U64 GP_LAND_OPTIONS			= 0x1 << 22;
const U64 GP_LAND_ALLOW_EDIT_LAND = 0x1 << 23;
const U64 GP_LAND_ALLOW_FLY		= 0x1 << 24;
const U64 GP_LAND_ALLOW_CREATE	= 0x1 << 25;
const U64 GP_LAND_ALLOW_LANDMARK	= 0x1 << 26;
const U64 GP_LAND_ALLOW_SET_HOME	= 0x1 << 28;
const U64 GP_LAND_ALLOW_HOLD_EVENT	= 0x1LL << 41;
const U64 GP_LAND_MANAGE_ALLOWED	= 0x1 << 29;
const U64 GP_LAND_MANAGE_BANNED	= 0x1 << 30;
const U64 GP_LAND_MANAGE_PASSES	= 0x1LL << 31;
const U64 GP_LAND_ADMIN			= 0x1LL << 32;
const U64 GP_LAND_RETURN_GROUP_SET	= 0x1LL << 33;
const U64 GP_LAND_RETURN_NON_GROUP	= 0x1LL << 34;
const U64 GP_LAND_RETURN_GROUP_OWNED= 0x1LL << 48;
const U64 GP_LAND_RETURN		= GP_LAND_RETURN_GROUP_OWNED
								| GP_LAND_RETURN_GROUP_SET
								| GP_LAND_RETURN_NON_GROUP;
const U64 GP_LAND_GARDENING		= 0x1LL << 35;
const U64 GP_OBJECT_DEED			= 0x1LL << 36;
const U64 GP_OBJECT_MANIPULATE		= 0x1LL << 38;
const U64 GP_OBJECT_SET_SALE		= 0x1LL << 39;
const U64 GP_ACCOUNTING_ACCOUNTABLE = 0x1LL << 40;
const U64 GP_NOTICES_SEND			= 0x1LL << 42;
const U64 GP_NOTICES_RECEIVE		= 0x1LL << 43;
const U64 GP_PROPOSAL_START		= 0x1LL << 44;
const U64 GP_PROPOSAL_VOTE		= 0x1LL << 45;
const U64 GP_SESSION_JOIN = 0x1LL << 16;
const U64 GP_SESSION_VOICE = 0x1LL << 27;
const U64 GP_SESSION_MODERATOR = 0x1LL << 37;
const U64 GP_EXPERIENCE_ADMIN	= 0x1LL << 49;
const U64 GP_EXPERIENCE_CREATOR = 0x1LL << 50;
const U64 GP_GROUP_BAN_ACCESS = 0x1LL << 51;
const U64 GP_DEFAULT_MEMBER = GP_ACCOUNTING_ACCOUNTABLE
								| GP_LAND_ALLOW_SET_HOME
								| GP_NOTICES_RECEIVE
								| GP_PROPOSAL_START
								| GP_PROPOSAL_VOTE
								| GP_SESSION_JOIN
								| GP_SESSION_VOICE
								;
const U64 GP_DEFAULT_OFFICER = GP_DEFAULT_MEMBER
								| GP_GROUP_CHANGE_IDENTITY
								| GP_LAND_ADMIN
								| GP_LAND_ALLOW_EDIT_LAND
								| GP_LAND_ALLOW_FLY
								| GP_LAND_ALLOW_CREATE
								| GP_LAND_ALLOW_LANDMARK
								| GP_LAND_CHANGE_IDENTITY
								| GP_LAND_CHANGE_MEDIA
								| GP_LAND_DEED
								| GP_LAND_DIVIDE_JOIN
								| GP_LAND_EDIT
								| GP_LAND_FIND_PLACES
								| GP_LAND_GARDENING
								| GP_LAND_MANAGE_ALLOWED
								| GP_LAND_MANAGE_BANNED
								| GP_LAND_MANAGE_PASSES
								| GP_LAND_OPTIONS
								| GP_LAND_RELEASE
								| GP_LAND_RETURN_GROUP_OWNED
								| GP_LAND_RETURN_GROUP_SET
								| GP_LAND_RETURN_NON_GROUP
								| GP_LAND_SET_LANDING_POINT
								| GP_LAND_SET_SALE_INFO
								| GP_MEMBER_EJECT
								| GP_MEMBER_INVITE
								| GP_MEMBER_OPTIONS
								| GP_MEMBER_VISIBLE_IN_DIR
								| GP_NOTICES_SEND
								| GP_OBJECT_DEED
								| GP_OBJECT_MANIPULATE
								| GP_OBJECT_SET_SALE
								| GP_ROLE_ASSIGN_MEMBER_LIMITED
								| GP_ROLE_PROPERTIES
								| GP_SESSION_MODERATOR
								;
#endif
