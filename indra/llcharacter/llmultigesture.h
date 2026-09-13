/** 
 * @file llmultigesture.h
 * @brief Gestures that are asset-based and can have multiple steps.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#ifndef LL_LLMULTIGESTURE_H
#define LL_LLMULTIGESTURE_H
#include <boost/unordered_set.hpp>
#include <string>
#include <vector>
#include "lluuid.h"
#include "llframetimer.h"
class LLDataPacker;
class LLGestureStep;
class LLMultiGesture
{
public:
	LLMultiGesture();
	virtual ~LLMultiGesture();
	S32 getMaxSerialSize() const;
	bool serialize(LLDataPacker& dp) const;
	bool deserialize(LLDataPacker& dp);
	void dump();
	void reset();
	const std::string& getTrigger() const { return mTrigger; }
protected:
	LLMultiGesture(const LLMultiGesture& gest);
	const LLMultiGesture& operator=(const LLMultiGesture& rhs);
public:
	KEY mKey;
	MASK mMask;
	std::string mName;
	std::string mTrigger;
	std::string mReplaceText;
	std::vector<LLGestureStep*> mSteps;
	bool mPlaying;
	bool mLocal;
	S32 mCurrentStep;
	bool mWaitingAnimations;
	bool mWaitingTimer;
	bool mWaitingAtEnd;
	LLFrameTimer mWaitTimer;
	std::function<void (LLMultiGesture*)> mDoneCallback;
	uuid_set_t mRequestedAnimIDs;
	uuid_set_t mPlayingAnimIDs;
};
enum EStepType
{
	STEP_ANIMATION = 0,
	STEP_SOUND = 1,
	STEP_CHAT = 2,
	STEP_WAIT = 3,
	STEP_EOF = 4
};
class LLGestureStep
{
public:
	LLGestureStep() {}
	virtual ~LLGestureStep() {}
	virtual EStepType getType() const = 0;
	virtual std::vector<std::string> getLabel() const = 0;
	virtual S32 getMaxSerialSize() const = 0;
	virtual bool serialize(LLDataPacker& dp) const = 0;
	virtual bool deserialize(LLDataPacker& dp) = 0;
	virtual void dump() = 0;
};
constexpr U32 ANIM_FLAG_STOP = 0x01;
class LLGestureStepAnimation : public LLGestureStep
{
public:
	LLGestureStepAnimation();
	virtual ~LLGestureStepAnimation();
	EStepType getType() const override { return STEP_ANIMATION; }
	std::vector<std::string> getLabel() const override;
	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;
	void dump() override;
public:
	std::string mAnimName;
	LLUUID mAnimAssetID;
	U32 mFlags;
};
class LLGestureStepSound : public LLGestureStep
{
public:
	LLGestureStepSound();
	virtual ~LLGestureStepSound();
	EStepType getType() const override { return STEP_SOUND; }
	std::vector<std::string> getLabel() const override;
	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;
	void dump() override;
public:
	std::string mSoundName;
	LLUUID mSoundAssetID;
	U32 mFlags;
};
class LLGestureStepChat : public LLGestureStep
{
public:
	LLGestureStepChat();
	virtual ~LLGestureStepChat();
	EStepType getType() const override { return STEP_CHAT; }
	std::vector<std::string> getLabel() const override;
	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;
	void dump() override;
public:
	std::string mChatText;
	U32 mFlags;
};
constexpr U32 WAIT_FLAG_TIME		= 0x01;
constexpr U32 WAIT_FLAG_ALL_ANIM	= 0x02;
class LLGestureStepWait : public LLGestureStep
{
public:
	LLGestureStepWait();
	virtual ~LLGestureStepWait();
	EStepType getType() const override { return STEP_WAIT; }
	std::vector<std::string> getLabel() const override;
	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;
	void dump() override;
public:
	F32 mWaitSeconds;
	U32 mFlags;
};
#endif
