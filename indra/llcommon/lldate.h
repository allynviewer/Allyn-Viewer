/** 
 * @file lldate.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of a simple date class.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_LLDATE_H
#define LL_LLDATE_H
#include <iosfwd>
#include <string>
#include "stdtypes.h"
#include "llunits.h"
class LL_COMMON_API LLDate
{
public:
	LLDate();
	LLDate(const LLDate& date);
	LLDate(F64SecondsImplicit seconds_since_epoch);
	LLDate(const std::string& iso8601_date);
	std::string asString() const;
	std::string asRFC1123() const;
	void toStream(std::ostream&) const;
	bool split(S32 *year, S32 *month = NULL, S32 *day = NULL, S32 *hour = NULL, S32 *min = NULL, S32 *sec = NULL) const;
	std::string toHTTPDateString (const std::string& fmt) const;
	static std::string toHTTPDateString (tm * gmt, const std::string& fmt);
	bool fromString(const std::string& iso8601_date);
	bool fromStream(std::istream&);
	bool fromYMDHMS(S32 year, S32 month = 1, S32 day = 0, S32 hour = 0, S32 min = 0, S32 sec = 0);
	F64 secondsSinceEpoch() const;
	void secondsSinceEpoch(F64 seconds);
    static LLDate now();
	bool operator<(const LLDate& rhs) const;
    bool operator>(const LLDate& rhs) const { return rhs < *this; }
    bool operator<=(const LLDate& rhs) const { return !(rhs < *this); }
    bool operator>=(const LLDate& rhs) const { return !(*this < rhs); }
    bool operator!=(const LLDate& rhs) const { return (*this < rhs) || (rhs < *this); }
    bool operator==(const LLDate& rhs) const { return !(*this != rhs); }
	bool isNull() const { return mSecondsSinceEpoch == 0.0; }
	bool notNull() const { return mSecondsSinceEpoch != 0.0; }
private:
	F64 mSecondsSinceEpoch;
};
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLDate& date);
LL_COMMON_API std::istream& operator>>(std::istream& s, LLDate& date);
#endif
