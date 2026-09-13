/** 
 * @file llparcelflags.h
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
#ifndef LL_LLPARCEL_FLAGS_H
#define LL_LLPARCEL_FLAGS_H
const U32 PF_ALLOW_FLY			= 1 << 0;
const U32 PF_ALLOW_OTHER_SCRIPTS= 1 << 1;
const U32 PF_FOR_SALE			= 1 << 2;
const U32 PF_FOR_SALE_OBJECTS	= 1 << 7;
const U32 PF_ALLOW_LANDMARK		= 1 << 3;
const U32 PF_ALLOW_TERRAFORM	= 1 << 4;
const U32 PF_ALLOW_DAMAGE		= 1 << 5;
const U32 PF_CREATE_OBJECTS		= 1 << 6;
const U32 PF_USE_ACCESS_GROUP	= 1 << 8;
const U32 PF_USE_ACCESS_LIST	= 1 << 9;
const U32 PF_USE_BAN_LIST		= 1 << 10;
const U32 PF_USE_PASS_LIST		= 1 << 11;
const U32 PF_SHOW_DIRECTORY		= 1 << 12;
const U32 PF_ALLOW_DEED_TO_GROUP		= 1 << 13;
const U32 PF_CONTRIBUTE_WITH_DEED		= 1 << 14;
const U32 PF_SOUND_LOCAL				= 1 << 15;
const U32 PF_SELL_PARCEL_OBJECTS		= 1 << 16;
const U32 PF_ALLOW_PUBLISH				= 1 << 17;
const U32 PF_MATURE_PUBLISH				= 1 << 18;
const U32 PF_URL_WEB_PAGE				= 1 << 19;
const U32 PF_URL_RAW_HTML				= 1 << 20;
const U32 PF_RESTRICT_PUSHOBJECT		= 1 << 21;
const U32 PF_DENY_ANONYMOUS				= 1 << 22;
const U32 PF_GAMING						= 1 << 23;
const U32 PF_ALLOW_GROUP_SCRIPTS		= 1 << 25;
const U32 PF_CREATE_GROUP_OBJECTS		= 1 << 26;
const U32 PF_ALLOW_ALL_OBJECT_ENTRY		= 1 << 27;
const U32 PF_ALLOW_GROUP_OBJECT_ENTRY	= 1 << 28;
const U32 PF_ALLOW_VOICE_CHAT			= 1 << 29;
const U32 PF_USE_ESTATE_VOICE_CHAN      = 1 << 30;
const U32 PF_DENY_AGEUNVERIFIED         = 1U << 31;
const U32 PF_USE_RESTRICTED_ACCESS = PF_USE_ACCESS_GROUP
										| PF_USE_ACCESS_LIST
										| PF_USE_BAN_LIST
										| PF_USE_PASS_LIST
										| PF_DENY_ANONYMOUS
										| PF_DENY_AGEUNVERIFIED;
const U32 PF_NONE = 0x00000000;
const U32 PF_ALL  = 0xFFFFFFFF;
const U32 PF_DEFAULT =  PF_ALLOW_FLY
						| PF_ALLOW_OTHER_SCRIPTS
						| PF_ALLOW_GROUP_SCRIPTS
						| PF_ALLOW_LANDMARK
						| PF_CREATE_OBJECTS
						| PF_CREATE_GROUP_OBJECTS
						| PF_USE_BAN_LIST
						| PF_ALLOW_ALL_OBJECT_ENTRY
						| PF_ALLOW_GROUP_OBJECT_ENTRY
                        | PF_ALLOW_VOICE_CHAT
                        | PF_USE_ESTATE_VOICE_CHAN;
const U32 AL_ACCESS				= (1 << 0);
const U32 AL_BAN					= (1 << 1);
const U32 AL_ALLOW_EXPERIENCE	= (1 << 3);
const U32 AL_BLOCK_EXPERIENCE	= (1 << 4);
const S32 BA_ALLOWED = 0;
const S32 BA_NOT_IN_GROUP = 1;
const S32 BA_NOT_ON_LIST = 2;
const S32 BA_BANNED = 3;
const S32 BA_NO_ACCESS_LEVEL = 4;
const S32 BA_NOT_AGE_VERIFIED = 5;
const U32 PR_NONE		= 0x0;
const U32 PR_GOD_FORCE	= (1 << 0);
enum EObjectCategory
{
	OC_INVALID = -1,
	OC_NONE = 0,
	OC_TOTAL = 0,
	OC_OWNER,
	OC_GROUP,
	OC_OTHER,
	OC_SELECTED,
	OC_TEMP,
	OC_COUNT
};
const S32 PARCEL_DETAILS_NAME = 0;
const S32 PARCEL_DETAILS_DESC = 1;
const S32 PARCEL_DETAILS_OWNER = 2;
const S32 PARCEL_DETAILS_GROUP = 3;
const S32 PARCEL_DETAILS_AREA = 4;
const S32 PARCEL_DETAILS_ID = 5;
const S32 PARCEL_DETAILS_SEE_AVATARS = 6;
#endif
