/**
* @file llnotificationtemplate.h
* @brief Description of notification contents
* @author Q (with assistance from Richard and Coco)
*
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
*/
#ifndef LL_LLNOTIFICATION_TEMPLATE_H
#define LL_LLNOTIFICATION_TEMPLATE_H
#include "llnotifications.h"
typedef boost::shared_ptr<LLNotificationForm> LLNotificationFormPtr;
struct LLNotificationTemplate
{
	LLNotificationTemplate();
    std::string mName;
    std::string mType;
    std::string mMessage;
	std::string mLabel;
	std::string mIcon;
    bool mUnique;
    std::vector<std::string> mUniqueContext;
    U32 mExpireSeconds;
    U32 mExpireOption;
    std::string mURL;
    U32 mURLOption;
	bool mPersist;
	std::string mDefaultFunctor;
    LLNotificationFormPtr mForm;
	ENotificationPriority mPriority;
	LLUUID mSoundEffect;
};
#endif
