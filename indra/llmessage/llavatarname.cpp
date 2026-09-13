/** 
 * @file llavatarname.cpp
 * @brief Represents name-related data for an avatar, such as the
 * username/SLID ("bobsmith123" or "james.linden") and the display
 * name ("James Cook")
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "llavatarname.h"
#include "llcontrol.h"
#include "lldate.h"
#include "llframetimer.h"
#include "llsd.h"
static const std::string USERNAME("username");
static const std::string DISPLAY_NAME("display_name");
static const std::string LEGACY_FIRST_NAME("legacy_first_name");
static const std::string LEGACY_LAST_NAME("legacy_last_name");
static const std::string IS_DISPLAY_NAME_DEFAULT("is_display_name_default");
static const std::string DISPLAY_NAME_EXPIRES("display_name_expires");
static const std::string DISPLAY_NAME_NEXT_UPDATE("display_name_next_update");
bool LLAvatarName::sUseDisplayNames = true;
bool LLAvatarName::sUseUsernames = true;
const F64 MIN_ENTRY_LIFETIME = 60.0;
LLAvatarName::LLAvatarName()
:	mUsername(),
	mDisplayName(),
	mLegacyFirstName(),
	mLegacyLastName(),
	mIsDisplayNameDefault(false),
	mIsTemporaryName(false),
	mExpires(F64_MAX),
	mNextUpdate(0.0)
{ }
bool LLAvatarName::operator<(const LLAvatarName& rhs) const
{
	if (mUsername == rhs.mUsername)
		return mDisplayName < rhs.mDisplayName;
	else
		return mUsername < rhs.mUsername;
}
void LLAvatarName::setUseDisplayNames(bool use)
{
	sUseDisplayNames = use;
}
bool LLAvatarName::useDisplayNames()
{
	return sUseDisplayNames;
}
void LLAvatarName::setUseUsernames(bool use)
{
	sUseUsernames = use;
}
bool LLAvatarName::useUsernames()
{
	return sUseUsernames;
}
LLSD LLAvatarName::asLLSD() const
{
	LLSD sd;
	sd[USERNAME] = mUsername;
	sd[DISPLAY_NAME] = mDisplayName;
	sd[LEGACY_FIRST_NAME] = mLegacyFirstName;
	sd[LEGACY_LAST_NAME] = mLegacyLastName;
	sd[IS_DISPLAY_NAME_DEFAULT] = mIsDisplayNameDefault;
	sd[DISPLAY_NAME_EXPIRES] = LLDate(mExpires);
	sd[DISPLAY_NAME_NEXT_UPDATE] = LLDate(mNextUpdate);
	return sd;
}
void LLAvatarName::fromLLSD(const LLSD& sd)
{
	mUsername = sd[USERNAME].asString();
	mDisplayName = sd[DISPLAY_NAME].asString();
	mLegacyFirstName = sd[LEGACY_FIRST_NAME].asString();
	mLegacyLastName = sd[LEGACY_LAST_NAME].asString();
	mIsDisplayNameDefault = sd[IS_DISPLAY_NAME_DEFAULT].asBoolean() || mUsername == mDisplayName;
	LLDate expires = sd[DISPLAY_NAME_EXPIRES];
	mExpires = expires.secondsSinceEpoch();
	LLDate next_update = sd[DISPLAY_NAME_NEXT_UPDATE];
	mNextUpdate = next_update.secondsSinceEpoch();
	if (mDisplayName.empty())
	{
		mDisplayName = mUsername;
		mIsDisplayNameDefault = true;
	}
}
void LLAvatarName::fromString(const std::string& full_name)
{
	mDisplayName = full_name;
	std::string::size_type index = full_name.find(' ');
	if (index != std::string::npos)
	{
		mLegacyFirstName = full_name.substr(0, index);
		mLegacyLastName = full_name.substr(index+1);
		if (mLegacyLastName != "Resident")
		{
			mUsername = mLegacyFirstName + '.' + mLegacyLastName;
			mDisplayName = full_name;
			LLStringUtil::toLower(mUsername);
		}
		else
		{
			mUsername = mLegacyFirstName;
			mDisplayName = mLegacyFirstName;
		}
	}
	else
	{
		mLegacyFirstName = full_name;
		mLegacyLastName = "";
		mUsername = full_name;
		mDisplayName = full_name;
	}
	mIsDisplayNameDefault = true;
	mIsTemporaryName = true;
	setExpires(MIN_ENTRY_LIFETIME);
}
void LLAvatarName::setExpires(F64 expires)
{
	mExpires = LLFrameTimer::getTotalSeconds() + expires;
}
std::string LLAvatarName::getCompleteName(bool linefeed) const
{
	std::string name;
	if (sUseDisplayNames)
	{
		if (mUsername.empty() || mIsDisplayNameDefault)
		{
			name = mDisplayName;
		}
		else
		{
			name = mDisplayName;
			if (sUseUsernames)
			{
				name += (linefeed ? "\n(" : " (") + mUsername + ')';
			}
		}
	}
	else
	{
		name = getUserName();
	}
	return name;
}
std::string LLAvatarName::getLegacyName() const
{
	if (mLegacyFirstName.empty() && mLegacyLastName.empty())
	{
		return mDisplayName;
	}
	static const LLCachedControl<bool> show_resident("LiruShowLastNameResident", false);
	if (show_resident || mLegacyLastName != "Resident")
		return mLegacyFirstName + ' ' + mLegacyLastName;
	return mLegacyFirstName;
}
std::string LLAvatarName::getDisplayName() const
{
	if (sUseDisplayNames)
	{
		return mDisplayName;
	}
	else
	{
		return getUserName();
	}
}
std::string LLAvatarName::getUserName() const
{
	std::string name;
	if (mLegacyLastName.empty() )
	{
		if (mLegacyFirstName.empty())
		{
			name = mDisplayName;
		}
		else
		{
			name = mLegacyFirstName;
		}
	}
	else
	{
		name = mLegacyFirstName + ' ' + mLegacyLastName;
	}
	return name;
}
void LLAvatarName::dump() const
{
	LL_DEBUGS("AvNameCache") << "LLAvatarName: "
	                         << "user '" << mUsername << "' "
							 << "display '" << mDisplayName << "' "
	                         << "expires in " << mExpires - LLFrameTimer::getTotalSeconds() << " seconds"
							 << LL_ENDL;
}
