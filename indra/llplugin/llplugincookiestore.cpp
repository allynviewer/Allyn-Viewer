/** 
 * @file llplugincookiestore.cpp
 * @brief LLPluginCookieStore provides central storage for http cookies used by plugins
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 * 
 * @endcond
 */
#include "linden_common.h"
#include "indra_constants.h"
#include "llplugincookiestore.h"
#include <iostream>
#include <curl/curl.h>
LLPluginCookieStore::LLPluginCookieStore():
	mHasChangedCookies(false)
{
}
LLPluginCookieStore::~LLPluginCookieStore()
{
	clearCookies();
}
LLPluginCookieStore::Cookie::Cookie(const std::string &s, std::string::size_type cookie_start, std::string::size_type cookie_end):
	mCookie(s, cookie_start, cookie_end - cookie_start),
	mNameStart(0), mNameEnd(0),
	mValueStart(0), mValueEnd(0),
	mDomainStart(0), mDomainEnd(0),
	mPathStart(0), mPathEnd(0),
	mDead(false), mChanged(true)
{
}
LLPluginCookieStore::Cookie *LLPluginCookieStore::Cookie::createFromString(const std::string &s, std::string::size_type cookie_start, std::string::size_type cookie_end, const std::string &host)
{
	Cookie *result = new Cookie(s, cookie_start, cookie_end);
	if(!result->parse(host))
	{
		delete result;
		result = NULL;
	}
	return result;
}
std::string LLPluginCookieStore::Cookie::getKey() const
{
	std::string result;
	if(mDomainEnd > mDomainStart)
	{
		result += mCookie.substr(mDomainStart, mDomainEnd - mDomainStart);
	}
	result += ';';
	if(mPathEnd > mPathStart)
	{
		result += mCookie.substr(mPathStart, mPathEnd - mPathStart);
	}
	result += ';';
	result += mCookie.substr(mNameStart, mNameEnd - mNameStart);
	return result;
}
bool LLPluginCookieStore::Cookie::parse(const std::string &host)
{
	bool first_field = true;
	std::string::size_type cookie_end = mCookie.size();
	std::string::size_type field_start = 0;
	LL_DEBUGS("CookieStoreParse") << "parsing cookie: " << mCookie << LL_ENDL;
	while(field_start < cookie_end)
	{
		std::string::size_type next_field_start = findFieldEnd(field_start);
		std::string::size_type field_end = mCookie.find_last_not_of("; ", next_field_start);
		if(field_end == std::string::npos || field_end < field_start)
		{
			field_end = field_start;
		}
		else if (field_end < next_field_start)
		{
			++field_end;
		}
		std::string::size_type name_start = mCookie.find_first_not_of("; ", field_start);
		if(name_start == std::string::npos || name_start > next_field_start)
		{
			name_start = field_start;
		}
		std::string::size_type name_value_sep = mCookie.find_first_of("=", name_start);
		if(name_value_sep == std::string::npos || name_value_sep > field_end)
		{
			name_value_sep = field_end;
		}
		std::string::size_type name_end = mCookie.find_last_not_of("= ", name_value_sep);
		if(name_end == std::string::npos || name_end < name_start)
		{
			name_end = name_start;
		}
		else if (name_end < name_value_sep)
		{
			++name_end;
		}
		std::string::size_type value_start = mCookie.find_first_not_of("= ", name_value_sep);
		if(value_start == std::string::npos || value_start > field_end)
		{
			value_start = field_end;
		}
		std::string::size_type value_end = mCookie.find_last_not_of("; ", field_end);
		if(value_end == std::string::npos || value_end < value_start)
		{
			value_end = value_start;
		}
		else if (value_end < field_end)
		{
			++value_end;
		}
		LL_DEBUGS("CookieStoreParse")
			<< "    field name: \"" << mCookie.substr(name_start, name_end - name_start)
			<< "\", value: \"" << mCookie.substr(value_start, value_end - value_start) << "\""
			<< LL_ENDL;
		if(first_field)
		{
			mNameStart = name_start;
			mNameEnd = name_end;
			mValueStart = value_start;
			mValueEnd = value_end;
			first_field = false;
		}
		else
		{
			if(matchName(name_start, name_end, "expires"))
			{
				std::string date_string(mCookie, value_start, value_end - value_start);
#if 1
				time_t date = curl_getdate(date_string.c_str(), NULL );
				mDate.secondsSinceEpoch((F64)date);
				LL_DEBUGS("CookieStoreParse") << "        expire date parsed to: " << mDate.asRFC1123() << LL_ENDL;
#else
				if(!mDate.fromString(date_string))
				{
					LL_WARNS("CookieStoreParse") << "failed to parse cookie's expire date: " << date << LL_ENDL;
					return false;
				}
#endif
			}
			else if(matchName(name_start, name_end, "domain"))
			{
				mDomainStart = value_start;
				mDomainEnd = value_end;
			}
			else if(matchName(name_start, name_end, "path"))
			{
				mPathStart = value_start;
				mPathEnd = value_end;
			}
			else if(matchName(name_start, name_end, "max-age"))
			{
			}
			else if(matchName(name_start, name_end, "secure"))
			{
			}
			else if(matchName(name_start, name_end, "version"))
			{
			}
			else if(matchName(name_start, name_end, "comment"))
			{
			}
			else if(matchName(name_start, name_end, "httponly"))
			{
			}
			else
			{
				LL_WARNS("CookieStoreParse") << "unexpected field name: " << mCookie.substr(name_start, name_end - name_start) << LL_ENDL;
				return false;
			}
		}
		field_start = mCookie.find_first_not_of("; ", next_field_start);
	}
	if(mNameEnd <= mNameStart)
		return false;
	if(mDomainEnd <= mDomainStart)
	{
		if(host.empty())
		{
			return false;
		}
		std::string::size_type last_char = mCookie.find_last_not_of(" ");
		if((last_char != std::string::npos) && (mCookie[last_char] != ';'))
		{
			mCookie += ";";
		}
		mCookie += " domain=";
		mDomainStart = mCookie.size();
		mCookie += host;
		mDomainEnd = mCookie.size();
		LL_DEBUGS("CookieStoreParse") << "added domain (" << mDomainStart << " to " << mDomainEnd << "), new cookie is: " << mCookie << LL_ENDL;
	}
	if(mPathEnd <= mPathStart)
	{
		std::string::size_type last_char = mCookie.find_last_not_of(" ");
		if((last_char != std::string::npos) && (mCookie[last_char] != ';'))
		{
			mCookie += ";";
		}
		mCookie += " path=";
		mPathStart = mCookie.size();
		mCookie += "/";
		mPathEnd = mCookie.size();
		LL_DEBUGS("CookieStoreParse") << "added path (" << mPathStart << " to " << mPathEnd << "), new cookie is: " << mCookie << LL_ENDL;
	}
	return true;
}
std::string::size_type LLPluginCookieStore::Cookie::findFieldEnd(std::string::size_type start, std::string::size_type end)
{
	std::string::size_type result = start;
	if(end == std::string::npos)
		end = mCookie.size();
	bool in_quotes = false;
	for(; (result < end); result++)
	{
		switch(mCookie[result])
		{
			case '\\':
				if(in_quotes)
					result++;
			break;
			case '"':
				in_quotes = !in_quotes;
			break;
			case ';':
				if(!in_quotes)
					return result;
			break;
		}
	}
	return end;
}
bool LLPluginCookieStore::Cookie::matchName(std::string::size_type start, std::string::size_type end, const char *name)
{
	while((start < end) && (*name != '\0'))
	{
		if(tolower(mCookie[start]) != *name)
			return false;
		start++;
		name++;
	}
	return ((start == end) && (*name == '\0'));
}
std::string LLPluginCookieStore::getAllCookies()
{
	std::stringstream result;
	writeAllCookies(result);
	return result.str();
}
void LLPluginCookieStore::writeAllCookies(std::ostream& s)
{
	cookie_map_t::iterator iter;
	for(iter = mCookies.begin(); iter != mCookies.end(); iter++)
	{
		if(!iter->second->isDead())
		{
			s << (iter->second->getCookie()) << "\n";
		}
	}
}
std::string LLPluginCookieStore::getPersistentCookies()
{
	std::stringstream result;
	writePersistentCookies(result);
	return result.str();
}
void LLPluginCookieStore::writePersistentCookies(std::ostream& s)
{
	cookie_map_t::iterator iter;
	for(iter = mCookies.begin(); iter != mCookies.end(); iter++)
	{
		if(!iter->second->isDead() && !iter->second->isSessionCookie())
		{
			s << iter->second->getCookie() << "\n";
		}
	}
}
std::string LLPluginCookieStore::getChangedCookies(bool clear_changed)
{
	std::stringstream result;
	writeChangedCookies(result, clear_changed);
	return result.str();
}
void LLPluginCookieStore::writeChangedCookies(std::ostream& s, bool clear_changed)
{
	if(mHasChangedCookies)
	{
		LL_DEBUGS() << "returning changed cookies: " << LL_ENDL;
		cookie_map_t::iterator iter;
		for(iter = mCookies.begin(); iter != mCookies.end(); )
		{
			cookie_map_t::iterator next = iter;
			next++;
			if(iter->second->isChanged())
			{
				s << iter->second->getCookie() << "\n";
				LL_DEBUGS() << "    " << iter->second->getCookie() << LL_ENDL;
				if(clear_changed)
				{
					if(iter->second->isDead())
					{
						delete iter->second;
						mCookies.erase(iter);
					}
					else
					{
						iter->second->setChanged(false);
					}
				}
			}
			iter = next;
		}
	}
	if(clear_changed)
		mHasChangedCookies = false;
}
void LLPluginCookieStore::setAllCookies(const std::string &cookies, bool mark_changed)
{
	clearCookies();
	setCookies(cookies, mark_changed);
}
void LLPluginCookieStore::readAllCookies(std::istream& s, bool mark_changed)
{
	clearCookies();
	readCookies(s, mark_changed);
}
void LLPluginCookieStore::setCookies(const std::string &cookies, bool mark_changed)
{
	std::string::size_type start = 0;
	while(start != std::string::npos)
	{
		std::string::size_type end = cookies.find_first_of("\r\n", start);
		if(end > start)
		{
			setOneCookie(cookies, start, end, mark_changed);
		}
		start = cookies.find_first_not_of("\r\n ", end);
	}
}
void LLPluginCookieStore::setCookiesFromHost(const std::string &cookies, const std::string &host, bool mark_changed)
{
	std::string::size_type start = 0;
	while(start != std::string::npos)
	{
		std::string::size_type end = cookies.find_first_of("\r\n", start);
		if(end > start)
		{
			setOneCookie(cookies, start, end, mark_changed, host);
		}
		start = cookies.find_first_not_of("\r\n ", end);
	}
}
void LLPluginCookieStore::readCookies(std::istream& s, bool mark_changed)
{
	std::string line;
	while(s.good() && !s.eof())
	{
		std::getline(s, line);
		if(!line.empty())
		{
			setOneCookie(line, 0, std::string::npos, mark_changed);
		}
	}
}
std::string LLPluginCookieStore::quoteString(const std::string &s)
{
	std::stringstream result;
	result << '"';
	for(std::string::size_type i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		switch(c)
		{
			case '(': case ')': case '<': case '>': case '@':
			case ',': case ';': case ':': case '\\': case '"':
			case '/': case '[': case ']': case '?': case '=':
			case '{': case '}':	case ' ': case '\t':
				result << '\\';
			break;
		}
		result << c;
	}
	result << '"';
	return result.str();
}
std::string LLPluginCookieStore::unquoteString(const std::string &s)
{
	std::stringstream result;
	bool in_quotes = false;
	for(std::string::size_type i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		switch(c)
		{
			case '\\':
				if(in_quotes)
				{
					++i;
					if(i < s.size())
					{
						result << s[i];
					}
					continue;
				}
			break;
			case '"':
				in_quotes = !in_quotes;
				continue;
			break;
		}
		result << c;
	}
	return result.str();
}
void LLPluginCookieStore::setOneCookie(const std::string &s, std::string::size_type cookie_start, std::string::size_type cookie_end, bool mark_changed, const std::string &host)
{
	Cookie *cookie = Cookie::createFromString(s, cookie_start, cookie_end, host);
	if(cookie)
	{
		LL_DEBUGS("CookieStoreUpdate") << "setting cookie: " << cookie->getCookie() << LL_ENDL;
		std::string key = cookie->getKey();
		if(!cookie->isSessionCookie() && (cookie->getDate() < LLDate::now()))
		{
			if(mark_changed)
			{
				cookie->setDead(true);
				LL_DEBUGS("CookieStoreUpdate") << "    marking dead" << LL_ENDL;
			}
			else
			{
				removeCookie(key);
				delete cookie;
				cookie = NULL;
				LL_DEBUGS("CookieStoreUpdate") << "    removing" << LL_ENDL;
			}
		}
		if(cookie)
		{
			cookie_map_t::iterator iter = mCookies.find(key);
			if(iter != mCookies.end())
			{
				if(iter->second->getCookie() == cookie->getCookie())
				{
					delete cookie;
					cookie = NULL;
					LL_DEBUGS("CookieStoreUpdate") << "    unchanged" << LL_ENDL;
				}
				else
				{
					delete iter->second;
					iter->second = cookie;
					cookie->setChanged(mark_changed);
					if(mark_changed)
						mHasChangedCookies = true;
					LL_DEBUGS("CookieStoreUpdate") << "    replacing" << LL_ENDL;
				}
			}
			else
			{
				mCookies.insert(std::make_pair(key, cookie));
				cookie->setChanged(mark_changed);
				if(mark_changed)
					mHasChangedCookies = true;
				LL_DEBUGS("CookieStoreUpdate") << "    adding" << LL_ENDL;
			}
		}
	}
	else
	{
		LL_WARNS("CookieStoreUpdate") << "failed to parse cookie: " << s.substr(cookie_start, cookie_end - cookie_start) << LL_ENDL;
	}
}
void LLPluginCookieStore::clearCookies()
{
	while(!mCookies.empty())
	{
		cookie_map_t::iterator iter = mCookies.begin();
		delete iter->second;
		mCookies.erase(iter);
	}
}
void LLPluginCookieStore::removeCookie(const std::string &key)
{
	cookie_map_t::iterator iter = mCookies.find(key);
	if(iter != mCookies.end())
	{
		delete iter->second;
		mCookies.erase(iter);
	}
}
