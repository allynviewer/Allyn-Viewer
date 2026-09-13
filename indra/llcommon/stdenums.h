/** 
 * @file stdenums.h
 * @brief Enumerations for indra.
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
#ifndef LL_STDENUMS_H
#define LL_STDENUMS_H
enum EDragAndDropType
{
	DAD_NONE			= 0,
	DAD_TEXTURE			= 1,
	DAD_SOUND			= 2,
	DAD_CALLINGCARD		= 3,
	DAD_LANDMARK		= 4,
	DAD_SCRIPT			= 5,
	DAD_CLOTHING 		= 6,
	DAD_OBJECT			= 7,
	DAD_NOTECARD		= 8,
	DAD_CATEGORY		= 9,
	DAD_ROOT_CATEGORY 	= 10,
	DAD_BODYPART		= 11,
	DAD_ANIMATION		= 12,
	DAD_GESTURE			= 13,
	DAD_LINK			= 14,
	DAD_MESH            = 15,
	DAD_COUNT			= 16,
};
enum EAcceptance
{
	ACCEPT_POSTPONED,
	ACCEPT_NO,
	ACCEPT_NO_LOCKED,
	ACCEPT_YES_COPY_SINGLE,
	ACCEPT_YES_SINGLE,
	ACCEPT_YES_COPY_MULTI,
	ACCEPT_YES_MULTI
};
enum EDeRezDestination
{
	DRD_SAVE_INTO_AGENT_INVENTORY = 0,
	DRD_ACQUIRE_TO_AGENT_INVENTORY = 1,
	DRD_SAVE_INTO_TASK_INVENTORY = 2,
	DRD_ATTACHMENT = 3,
	DRD_TAKE_INTO_AGENT_INVENTORY = 4,
	DRD_FORCE_TO_GOD_INVENTORY = 5,
	DRD_TRASH = 6,
	DRD_ATTACHMENT_TO_INV = 7,
	DRD_ATTACHMENT_EXISTS = 8,
	DRD_RETURN_TO_OWNER = 9,
	DRD_RETURN_TO_LAST_OWNER = 10,
	DRD_COUNT = 11
};
enum EReturnReason
{
	RR_GENERIC = 0,
	RR_SANDBOX = 1,
	RR_PARCEL_OWNER = 2,
	RR_PARCEL_AUTO = 3,
	RR_PARCEL_FULL = 4,
	RR_OFF_WORLD = 5,
	RR_COUNT = 6
};
enum EObjectPropertiesExtraID
{
	OPEID_NONE = 0,
	OPEID_ASSET_ID = 1,
	OPEID_FROM_TASK_ID = 2,
	OPEID_COUNT = 3
};
enum EAddPosition
{
	ADD_TOP,
	ADD_SORTED,
	ADD_BOTTOM
};
#endif
