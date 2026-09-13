/** 
 * @file llphysicsmotion.h
 * @brief Implementation of LLPhysicsMotion class.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#ifndef LL_LLPHYSICSMOTIONCONTROLLER_H
#define LL_LLPHYSICSMOTIONCONTROLLER_H
#include "llmotion.h"
#include "llframetimer.h"
#define PHYSICS_MOTION_FADEIN_TIME 1.0f
#define PHYSICS_MOTION_FADEOUT_TIME 1.0f
class LLPhysicsMotion;
class LLPhysicsMotionController :
	public AIMaskedMotion
{
public:
	std::string getString();
	LLPhysicsMotionController(LLUUID const& id, LLMotionController* controller);
	virtual ~LLPhysicsMotionController();
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLPhysicsMotionController(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return PHYSICS_MOTION_FADEIN_TIME; }
	virtual F32 getEaseOutDuration() { return PHYSICS_MOTION_FADEOUT_TIME; }
	virtual F32 getMinPixelArea();
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
	LLCharacter* getCharacter() { return mCharacter; }
protected:
	void addMotion(LLPhysicsMotion *motion);
private:
	LLCharacter*		mCharacter;
	typedef std::vector<LLPhysicsMotion *> motion_vec_t;
	motion_vec_t mMotions;
	bool mIsDefault;
};
#endif
