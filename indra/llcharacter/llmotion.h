/** 
 * @file llmotion.h
 * @brief Implementation of LLMotion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#ifndef LL_LLMOTION_H
#define LL_LLMOTION_H
#include <string>
#include "aisyncclient.h"
#include "llerror.h"
#include "llpose.h"
#include "lluuid.h"
class LLCharacter;
class LLMotionController;
class AISyncKeyMotion : public AISyncKey
{
  private:
	F32 mDuration;
	bool mLoop;
  public:
	AISyncKeyMotion(AISyncKey const* from_key, F32 duration, bool loop) : AISyncKey(from_key), mDuration(duration), mLoop(loop) { }
  public:
	synckeytype_t getkeytype(void) const
	{
	  return synckeytype_motion;
	}
    bool equals(AISyncKey const& key) const
	{
	  switch (key.getkeytype())
	  {
		case synckeytype_motion:
		{
		  AISyncKeyMotion const& motion_key = static_cast<AISyncKeyMotion const&>(key);
		  return mLoop == motion_key.mLoop && is_approx_equal(mDuration, motion_key.mDuration);
		}
		default:
		  break;
	  }
	  return false;
	}
};
class AISyncClientMotion : public AISyncClient
{
  protected:
	~AISyncClientMotion() { unregister_client(); }
	AISyncKey* createSyncKey(AISyncKey const* from_key = NULL) const;
	void event1_ready(void) { }
	void event1_not_ready(void) { }
	void deregistered(void);
  protected:
	void aisync_loading(void);
	void aisync_loaded(void);
  public:
	virtual BOOL getLoop() = 0;
	virtual F32 getDuration() = 0;
	virtual F32 getAnimTime(void) const  = 0;
	virtual F32 getRuntime(void) const  = 0;
};
class LLMotion : public AISyncClientMotion
{
	friend class LLMotionController;
public:
	enum LLMotionBlendType
	{
		NORMAL_BLEND,
		ADDITIVE_BLEND
	};
	enum LLMotionInitStatus
	{
		STATUS_FAILURE,
		STATUS_SUCCESS,
		STATUS_HOLD
	};
	LLMotion(LLUUID const& id, LLMotionController* controller);
	virtual ~LLMotion();
public:
	const std::string &getName() const { return mName; }
	void setName(const std::string &name) { mName = name; }
	const LLUUID& getID() const { return mID; }
	virtual LLPose* getPose() { return &mPose;}
	void fadeOut();
	void fadeIn();
	F32 getFadeWeight() const { return mFadeWeight; }
	F32 getStopTime() const { return mStopTimestamp; }
	virtual void setStopTime(F32 time);
	BOOL isStopped() const { return mStopped; }
	void setStopped(BOOL stopped) { mStopped = stopped; }
	BOOL isBlending();
protected:
	void deactivate();
	BOOL isActive() { return mActive; }
public:
	void activate(F32 time);
	virtual F32 getRuntime(void) const;
	virtual F32 getAnimTime(void) const;
	F32 syncActivationTime(F32 time);
	LLMotionController* getController(void) const { return mController; }
public:
	virtual BOOL getLoop() = 0;
	virtual F32 getDuration() = 0;
	virtual F32 getEaseInDuration() = 0;
	virtual F32 getEaseOutDuration() = 0;
	virtual LLJoint::JointPriority getPriority() = 0;
	virtual LLMotionBlendType getBlendType() = 0;
	virtual F32 getMinPixelArea() = 0;
	virtual LLMotionInitStatus onInitialize(LLCharacter *character) = 0;
	virtual BOOL onUpdate(F32 activeTime, U8* joint_mask) = 0;
	virtual void onDeactivate() = 0;
	virtual BOOL canDeprecate();
	void	setDeactivateCallback( void (*cb)(void *), void* userdata );
protected:
	virtual BOOL onActivate() = 0;
	void addJointState(const LLPointer<LLJointState>& jointState);
protected:
	LLPose		mPose;
	BOOL		mStopped;
	BOOL		mActive;
	std::string		mName;
	LLUUID			mID;
	LLMotionController* mController;
	F32 mActivationTimestamp;
	F32 mStopTimestamp;
	F32 mSendStopTimestamp;
	F32 mResidualWeight;
	F32 mFadeWeight;
	U8	mJointSignature[3][LL_CHARACTER_MAX_ANIMATED_JOINTS];
	void (*mDeactivateCallback)(void* data);
	void* mDeactivateCallbackUserData;
};
class LLTestMotion : public LLMotion
{
public:
	LLTestMotion(LLUUID const& id, LLMotionController* controller) : LLMotion(id, controller){}
	~LLTestMotion() {}
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLTestMotion(id, controller); }
	BOOL getLoop() { return FALSE; }
	F32 getDuration() { return 0.0f; }
	F32 getEaseInDuration() { return 0.0f; }
	F32 getEaseOutDuration() { return 0.0f; }
	LLJoint::JointPriority getPriority() { return LLJoint::HIGH_PRIORITY; }
	LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	F32 getMinPixelArea() { return 0.f; }
	LLMotionInitStatus onInitialize(LLCharacter*) { LL_INFOS() << "LLTestMotion::onInitialize()" << LL_ENDL; return STATUS_SUCCESS; }
	BOOL onActivate() { LL_INFOS() << "LLTestMotion::onActivate()" << LL_ENDL; return TRUE; }
	BOOL onUpdate(F32 time, U8* joint_mask) { LL_INFOS() << "LLTestMotion::onUpdate(" << time << ")" << LL_ENDL; return TRUE; }
	void onDeactivate() { LL_INFOS() << "LLTestMotion::onDeactivate()" << LL_ENDL; }
};
class LLNullMotion : public LLMotion
{
public:
	LLNullMotion(LLUUID const& id, LLMotionController* controller) : LLMotion(id, controller) {}
	~LLNullMotion() {}
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLNullMotion(id, controller); }
	BOOL getLoop() { return TRUE; }
	F32 getDuration() { return 1.f; }
	F32 getEaseInDuration() { return 0.f; }
	F32 getEaseOutDuration() { return 0.f; }
	LLJoint::JointPriority getPriority() { return LLJoint::HIGH_PRIORITY; }
	LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	F32 getMinPixelArea() { return 0.f; }
	LLMotionInitStatus onInitialize(LLCharacter *character) { return STATUS_SUCCESS; }
	BOOL onActivate() { return TRUE; }
	BOOL onUpdate(F32 activeTime, U8* joint_mask) { return TRUE; }
	void onDeactivate() {}
};
U32 const ANIM_AGENT_BODY_NOISE		= 0x001;
U32 const ANIM_AGENT_BREATHE_ROT	= 0x002;
U32 const ANIM_AGENT_PHYSICS_MOTION	= 0x004;
U32 const ANIM_AGENT_EDITING		= 0x008;
U32 const ANIM_AGENT_EYE			= 0x010;
U32 const ANIM_AGENT_FLY_ADJUST		= 0x020;
U32 const ANIM_AGENT_HAND_MOTION	= 0x040;
U32 const ANIM_AGENT_HEAD_ROT		= 0x080;
U32 const ANIM_AGENT_PELVIS_FIX		= 0x100;
U32 const ANIM_AGENT_TARGET			= 0x200;
U32 const ANIM_AGENT_WALK_ADJUST	= 0x400;
class AIMaskedMotion : public LLMotion
{
private:
	U32 mMaskBit;
public:
	AIMaskedMotion(LLUUID const& id, LLMotionController* controller, U32 mask_bit) : LLMotion(id, controller), mMaskBit(mask_bit) { }
	BOOL onActivate();
	void onDeactivate();
	U32 getMaskBit(void) const { return mMaskBit; }
};
#endif
