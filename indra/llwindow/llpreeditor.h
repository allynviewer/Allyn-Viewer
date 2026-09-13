/** 
 * @file llpreeditor.h
 * @brief I believe this is used for languages like Japanese that require
 * an "input method editor" to type Kanji.
 * @author Open source patch, incorporated by Dave Simmons
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#ifndef LL_PREEDITOR
#define LL_PREEDITOR
class LLPreeditor
{
public:
	typedef std::vector<S32> segment_lengths_t;
	typedef std::vector<BOOL> standouts_t;
	virtual ~LLPreeditor() {};
	virtual void resetPreedit() = 0;
	virtual void updatePreedit(const LLWString &preedit_string,
						const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position) = 0;
	virtual void markAsPreedit(S32 position, S32 length) = 0;
	virtual void getPreeditRange(S32 *position, S32 *length) const = 0;
	virtual void getSelectionRange(S32 *position, S32 *length) const = 0;
	virtual BOOL getPreeditLocation(S32 query_position, LLCoordGL *coord, LLRect *bounds, LLRect *control) const = 0;
	virtual S32 getPreeditFontSize() const = 0;
	virtual const LLWString & getWText() const = 0;
	const LLWString & getPreeditString() const {return getWText();}
	virtual BOOL handleUnicodeCharHere(llwchar uni_char) = 0;
};
#endif
