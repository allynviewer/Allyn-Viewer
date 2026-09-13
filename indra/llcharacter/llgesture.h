/** 
 * @file llgesture.h
 * @brief A gesture is a combination of a triggering chat phrase or
 * key, a sound, an animation, and a chat string.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLGESTURE_H
#define LL_LLGESTURE_H
#include "llanimationstates.h"
#include "lluuid.h"
#include "llstring.h"
class LLGesture
{
public:
	LLGesture();
	LLGesture(KEY key, MASK mask, const std::string &trigger,
		const LLUUID &sound_item_id, const std::string &animation,
		const std::string &output_string);
	LLGesture(U8 **buffer, S32 max_size);
	LLGesture(const LLGesture &gesture);
	const LLGesture &operator=(const LLGesture &rhs);
	virtual ~LLGesture() {};
	KEY					getKey() const			{ return mKey; }
	MASK				getMask() const			{ return mMask; }
	const std::string&	getTrigger() const		{ return mTrigger; }
	const LLUUID&		getSound() const		{ return mSoundItemID; }
	const std::string&	getAnimation() const	{ return mAnimation; }
	const std::string&	getOutputString() const	{ return mOutputString; }
	virtual BOOL trigger(KEY key, MASK mask);
	virtual BOOL trigger(const std::string &string);
	U8 *serialize(U8 *buffer) const;
	U8 *deserialize(U8 *buffer, S32 max_size);
	static S32 getMaxSerialSize();
protected:
	KEY					mKey;
	MASK				mMask;
	std::string		mTrigger;
	std::string		mTriggerLower;
	LLUUID				mSoundItemID;
	std::string			mAnimation;
	std::string		mOutputString;
	static const S32	MAX_SERIAL_SIZE;
};
class LLGestureList
{
public:
	LLGestureList();
	virtual ~LLGestureList();
	BOOL trigger(KEY key, MASK mask);
	BOOL triggerAndReviseString(const std::string &string, std::string* revised_string);
	S32 count() const						{ return mList.size(); }
	virtual LLGesture* get(S32 i) const		{ return mList.at(i); }
	virtual void put(LLGesture* gesture)	{ mList.push_back( gesture ); }
	void deleteAll();
	U8 *serialize(U8 *buffer) const;
	U8 *deserialize(U8 *buffer, S32 max_size);
	S32 getMaxSerialSize();
protected:
	virtual LLGesture *create_gesture(U8 **buffer, S32 max_size);
protected:
	std::vector<LLGesture*>	mList;
	static const S32	SERIAL_HEADER_SIZE;
};
#endif
