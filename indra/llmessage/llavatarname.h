/** 
 * @file llavatarname.h
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
#ifndef LLAVATARNAME_H
#define LLAVATARNAME_H
#include <string>
const S32& main_name_system();
class LLSD;
class LLAvatarName
{
public:
	LLAvatarName();
	bool operator<(const LLAvatarName& rhs) const;
	LLSD asLLSD() const;
	void fromLLSD(const LLSD& sd);
	void fromString(const std::string& full_name);
	void setExpires(F64 expires);
	static void setUseDisplayNames(bool use);
	static bool useDisplayNames();
	static void setUseUsernames(bool use);
	static bool useUsernames();
	bool isValidName(F64 max_unrefreshed = 0.0f) const { return !mIsTemporaryName && (mExpires >= max_unrefreshed); }
	bool isDisplayNameDefault() const { return mIsDisplayNameDefault; }
	std::string getCompleteName(bool linefeed = false) const;
	std::string getLegacyName() const;
	std::string getDisplayName() const;
	std::string getUserName() const;
	std::string getAccountName() const { return mUsername; }
	std::string getNSName(const S32& name_system = main_name_system()) const
	{
		switch (name_system)
		{
			case 1 : return getCompleteName();
			case 2 : return getDisplayName();
			case 3 : return getLegacyName() + (mIsDisplayNameDefault ? "" : " (" + mDisplayName + ')'); break;
			default : return getLegacyName();
		}
	}
	void dump() const;
	F64 mExpires;
	F64 mNextUpdate;
private:
	std::string mUsername;
	std::string mDisplayName;
	std::string mLegacyFirstName;
	std::string mLegacyLastName;
	bool mIsDisplayNameDefault;
	bool mIsTemporaryName;
	static bool sUseDisplayNames;
	static bool sUseUsernames;
};
#endif
