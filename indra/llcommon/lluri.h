/** 
 * @file lluri.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of the URI class.
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
#ifndef LL_LLURI_H
#define LL_LLURI_H
#include <string>
class LLSD;
class LLApp;
class LL_COMMON_API LLURI
{
public:
  LLURI();
  LLURI(const std::string& escaped_str);
  LLURI(const std::string& scheme,
		const std::string& userName,
		const std::string& password,
		const std::string& hostName,
		U16 hostPort,
		const std::string& escapedPath,
		const std::string& escapedQuery);
	~LLURI();
	static LLURI buildHTTP(
		const std::string& prefix,
		const LLSD& path);
	static LLURI buildHTTP(
		const std::string& prefix,
		const LLSD& path,
		const LLSD& query);
	static LLURI buildHTTP(
		const std::string& scheme,
		const std::string& prefix,
		const LLSD& path,
		const LLSD& query);
	static LLURI buildHTTP(
		const std::string& host,
		const U32& port,
		const LLSD& path);
	static LLURI buildHTTP(
		const std::string& host,
		const U32& port,
		const LLSD& path,
		const LLSD& query);
	std::string asString() const;
	std::string scheme() const;
	std::string opaque() const;
	std::string authority() const;
	std::string hostName() const;
	std::string hostNameAndPort() const;
	std::string userName() const;
	std::string password() const;
	U16 hostPort() const;
	BOOL defaultPort() const;
	const std::string& escapedPath() const { return mEscapedPath; }
	std::string path() const;
	LLSD pathArray() const;
	std::string query() const;
	const std::string& escapedQuery() const { return mEscapedQuery; }
	LLSD queryMap() const;
	static LLSD queryMap(std::string escaped_query_string);
	static std::string mapToQueryString(const LLSD& query_map);
	static void encodeCharacter(std::ostream& ostr, std::string::value_type val);
	static std::string escape(const std::string& str);
	static std::string escape(
		const std::string& str,
		const std::string& allowed,
		bool is_allowed_sorted = false);
	static std::string unescape(const std::string& str);
private:
	void parseAuthorityAndPathUsingOpaque();
	std::string mScheme;
	std::string mEscapedOpaque;
	std::string mEscapedAuthority;
	std::string mEscapedPath;
	std::string mEscapedQuery;
};
LL_COMMON_API bool operator!=(const LLURI& first, const LLURI& second);
#endif
