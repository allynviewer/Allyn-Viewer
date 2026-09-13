/** 
 * @file llurlmatch.h
 * @author Martin Reddy
 * @brief Specifies a matched Url in a string, as returned by LLUrlRegistry
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#ifndef LL_LLURLMATCH_H
#define LL_LLURLMATCH_H
#include "llstyle.h"
class LLUrlMatch
{
public:
	LLUrlMatch();
	bool empty() const { return mUrl.empty(); }
	U32 getStart() const { return mStart; }
	U32 getEnd() const { return mEnd; }
	std::string getUrl() const { return mUrl; }
	std::string getLabel() const { return mLabel; }
	std::string getQuery() const { return mQuery; }
	std::string getTooltip() const { return mTooltip; }
	std::string getIcon() const { return mIcon; }
	LLStyleSP getStyle() const { return mStyle; }
	std::string getMenuName() const { return mMenuName; }
	std::string getLocation() const { return mLocation; }
	bool underlineOnHoverOnly() const { return mUnderlineOnHoverOnly; }
	bool isTrusted() const { return mTrusted; }
	void setValues(U32 start, U32 end, const std::string &url, const std::string &label,
	               const std::string& query, const std::string &tooltip, const std::string &icon,
				   const LLStyleSP& style, const std::string &menu,
				   const std::string &location, const LLUUID& id,
				   bool underline_on_hover_only = false, bool trusted = false);
	const LLUUID& getID() const { return mID; }
private:
	U32         mStart;
	U32         mEnd;
	std::string mUrl;
	std::string mLabel;
	std::string mQuery;
	std::string mTooltip;
	std::string mIcon;
	std::string mMenuName;
	std::string mLocation;
	LLUUID		mID;
	LLStyleSP mStyle;
	bool		mUnderlineOnHoverOnly;
	bool		mTrusted;
};
#endif
