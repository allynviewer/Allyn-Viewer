/** 
 * @file llurlregistry.cpp
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
#include "linden_common.h"
#include "llurlregistry.h"
#include "lluriparser.h"
#include <boost/regex.hpp>
void LLUrlRegistryNullCallback(const std::string &url, const std::string &label, const std::string& icon)
{
}
LLUrlRegistry::LLUrlRegistry()
{
	mUrlEntry.reserve(23);
	registerUrl(new LLUrlEntryNoLink());
	mUrlEntryIcon = new LLUrlEntryIcon();
	registerUrl(mUrlEntryIcon);
	mLLUrlEntryInvalidSLURL = new LLUrlEntryInvalidSLURL();
	registerUrl(mLLUrlEntryInvalidSLURL);
	registerUrl(new LLUrlEntrySLURL());
	mUrlEntryTrusted = new LLUrlEntrySecondlifeURL();
	registerUrl(mUrlEntryTrusted);
	registerUrl(new LLUrlEntrySimpleSecondlifeURL());
	registerUrl(new LLUrlEntryHTTP());
	mUrlEntryHTTPLabel = new LLUrlEntryHTTPLabel();
	registerUrl(mUrlEntryHTTPLabel);
	registerUrl(new LLUrlEntryAgentCompleteName());
	registerUrl(new LLUrlEntryAgentLegacyName());
	registerUrl(new LLUrlEntryAgentDisplayName());
	registerUrl(new LLUrlEntryAgentUserName());
	registerUrl(new LLUrlEntryAgent());
	registerUrl(new LLUrlEntryGroup());
	registerUrl(new LLUrlEntryParcel());
	registerUrl(new LLUrlEntryTeleport());
	registerUrl(new LLUrlEntryRegion());
	registerUrl(new LLUrlEntryWorldMap());
	registerUrl(new LLUrlEntryObjectIM());
	registerUrl(new LLUrlEntryPlace());
	registerUrl(new LLUrlEntryInventory());
	registerUrl(new LLUrlEntryExperienceProfile());
	registerUrl(new LLUrlEntrySL());
	mUrlEntrySLLabel = new LLUrlEntrySLLabel();
	registerUrl(mUrlEntrySLLabel);
	registerUrl(new LLUrlEntryEmail());
	registerUrl(new LLUrlEntryJira());
	registerUrl(new LLUrlEntryHTTPNoProtocol());
}
LLUrlRegistry::~LLUrlRegistry()
{
	std::vector<LLUrlEntryBase *>::iterator it;
	for (it = mUrlEntry.begin(); it != mUrlEntry.end(); ++it)
	{
		delete *it;
	}
}
void LLUrlRegistry::registerUrl(LLUrlEntryBase *url, bool force_front)
{
	if (url)
	{
		if (force_front)
			mUrlEntry.insert(mUrlEntry.begin(), url);
		else
		mUrlEntry.push_back(url);
	}
}
static bool matchRegex(const char *text, const boost::regex& regex, U32 &start, U32 &end)
{
	boost::cmatch result;
	bool found;
	try
	{
		found = boost::regex_search(text, result, regex);
	}
	catch (const std::runtime_error &)
	{
		return false;
	}
	if (! found)
	{
		return false;
	}
	start = static_cast<U32>(result[0].first - text);
	end = static_cast<U32>(result[0].second - text) - 1;
	if (text[end] == '.' || text[end] == ',')
	{
		end--;
	}
	else if (text[end] == ')' && std::string(text+start, end-start).find('(') == std::string::npos)
	{
		end--;
	}
	else if (text[end] == ']' && std::string(text+start, end-start).find('[') == std::string::npos)
	{
			end--;
	}
	return true;
}
static bool stringHasUrl(const std::string &text)
{
	return (text.find("://") != std::string::npos ||
			text.find("www.") != std::string::npos ||
			text.find(".com") != std::string::npos ||
			text.find(".net") != std::string::npos ||
			text.find(".edu") != std::string::npos ||
			text.find(".org") != std::string::npos ||
			text.find("<nolink>") != std::string::npos ||
			text.find("<icon") != std::string::npos ||
			text.find('@') != std::string::npos);
}
static bool stringHasJira(const std::string &text)
{
	return (text.find("ALCH")	 != std::string::npos ||
			text.find("SV")		 != std::string::npos ||
			text.find("BUG")	 != std::string::npos ||
			text.find("CHOP")	 != std::string::npos ||
			text.find("FIRE")	 != std::string::npos ||
			text.find("MAINT")	 != std::string::npos ||
			text.find("OPEN")	 != std::string::npos ||
			text.find("SCR")	 != std::string::npos ||
			text.find("STORM")	 != std::string::npos ||
			text.find("SVC")	 != std::string::npos ||
			text.find("VWR")	 != std::string::npos ||
			text.find("WEB")	 != std::string::npos);
}
bool LLUrlRegistry::findUrl(const std::string &text, LLUrlMatch &match, const LLUrlLabelCallback &cb, bool is_content_trusted)
{
	if (!(stringHasUrl(text) || stringHasJira(text)))
	{
		return false;
	}
	U32 match_start = 0, match_end = 0;
	LLUrlEntryBase *match_entry = nullptr;
	std::vector<LLUrlEntryBase *>::iterator it;
	for (it = mUrlEntry.begin(); it != mUrlEntry.end(); ++it)
	{
		if(!is_content_trusted && (mUrlEntryIcon == *it))
		{
			continue;
		}
		LLUrlEntryBase *url_entry = *it;
		U32 start = 0, end = 0;
		if (matchRegex(text.c_str(), url_entry->getPattern(), start, end))
		{
			if (start < match_start || match_entry == nullptr)
			{
				if (mLLUrlEntryInvalidSLURL == *it)
				{
					if(url_entry && url_entry->isSLURLvalid(text.substr(start, end - start + 1)))
					{
						continue;
					}
				}
				if((mUrlEntryHTTPLabel == *it) || (mUrlEntrySLLabel == *it))
				{
					if(url_entry && !url_entry->isWikiLinkCorrect(text.substr(start, end - start + 1)))
					{
						continue;
					}
				}
				match_start = start;
				match_end = end;
				match_entry = url_entry;
			}
		}
	}
	if (match_entry)
	{
		if (match_start > 0 && text.substr(match_start - 1, 1) == "@")
			return false;
		std::string url = text.substr(match_start, match_end - match_start + 1);
		if (match_entry == mUrlEntryTrusted)
		{
			LLUriParser up(url);
			up.normalize();
			url = up.normalizedUri();
		}
		match.setValues(match_start, match_end,
						match_entry->getUrl(url),
						match_entry->getLabel(url, cb),
						match_entry->getQuery(url),
						match_entry->getTooltip(url),
						match_entry->getIcon(url),
						match_entry->getStyle(),
						match_entry->getMenuName(),
						match_entry->getLocation(url),
						match_entry->getID(url),
						match_entry->underlineOnHoverOnly(url),
						match_entry->isTrusted());
		return true;
	}
	return false;
}
bool LLUrlRegistry::findUrl(const LLWString &text, LLUrlMatch &match, const LLUrlLabelCallback &cb)
{
	std::string utf8_text = wstring_to_utf8str(text);
	if (findUrl(utf8_text, match, cb))
	{
		LLWString wurl = utf8str_to_wstring(match.getUrl());
		size_t start = text.find(wurl);
		if (start == std::string::npos)
		{
			return false;
		}
		S32 end = start + wurl.size() - 1;
		match.setValues(start, end, match.getUrl(),
						match.getLabel(),
						match.getQuery(),
						match.getTooltip(),
						match.getIcon(),
						match.getStyle(),
						match.getMenuName(),
						match.getLocation(),
						match.getID(),
						match.underlineOnHoverOnly());
		return true;
	}
	return false;
}
bool LLUrlRegistry::hasUrl(const std::string &text)
{
	LLUrlMatch match;
	return findUrl(text, match);
}
bool LLUrlRegistry::hasUrl(const LLWString &text)
{
	LLUrlMatch match;
	return findUrl(text, match);
}
bool LLUrlRegistry::isUrl(const std::string &text)
{
	LLUrlMatch match;
	if (findUrl(text, match))
	{
		return (match.getStart() == 0 && match.getEnd() >= text.size()-1);
	}
	return false;
}
bool LLUrlRegistry::isUrl(const LLWString &text)
{
	LLUrlMatch match;
	if (findUrl(text, match))
	{
		return (match.getStart() == 0 && match.getEnd() >= text.size()-1);
	}
	return false;
}
