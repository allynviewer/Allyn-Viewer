/** 
 * @file llurlregistry.h
 * @author Martin Reddy
 * @brief Contains a set of Url types that can be matched in a string
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
#ifndef LL_LLURLREGISTRY_H
#define LL_LLURLREGISTRY_H
#include "llurlentry.h"
#include "llurlmatch.h"
#include "llsingleton.h"
#include "llstring.h"
void LLUrlRegistryNullCallback(const std::string &url,
							   const std::string &label,
							   const std::string &icon);
class LLUrlRegistry : public LLSingleton<LLUrlRegistry>
{
public:
	~LLUrlRegistry();
	void registerUrl(LLUrlEntryBase *url, bool force_front = false);
	bool findUrl(const std::string &text, LLUrlMatch &match,
				 const LLUrlLabelCallback &cb = &LLUrlRegistryNullCallback,
				 bool is_content_trusted = false);
	bool findUrl(const LLWString &text, LLUrlMatch &match,
				 const LLUrlLabelCallback &cb = &LLUrlRegistryNullCallback);
	bool hasUrl(const std::string &text);
	bool hasUrl(const LLWString &text);
	bool isUrl(const std::string &text);
	bool isUrl(const LLWString &text);
private:
	LLUrlRegistry();
	friend class LLSingleton<LLUrlRegistry>;
	std::vector<LLUrlEntryBase *> mUrlEntry;
	LLUrlEntryBase*	mUrlEntryTrusted;
	LLUrlEntryBase*	mUrlEntryIcon;
	LLUrlEntryBase* mLLUrlEntryInvalidSLURL;
	LLUrlEntryBase* mUrlEntryHTTPLabel;
	LLUrlEntryBase* mUrlEntrySLLabel;
};
#endif
