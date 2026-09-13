/** 
 * @file llemote.h
 * @brief Definition of LLEmote class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LL_LLEMOTE_H
#define LL_LLEMOTE_H
#include "llmotion.h"
#include "lltimer.h"
#define MIN_REQUIRED_PIXEL_AREA_EMOTE 2000.f
#define EMOTE_MORPH_FADEIN_TIME 0.3f
#define EMOTE_MORPH_IN_TIME 1.1f
#define EMOTE_MORPH_FADEOUT_TIME 1.4f
class LLVisualParam;
class LLEmote :
	public LLMotion
{
public:
	LLEmote(LLUUID const& id, LLMotionController* controller);
	virtual ~LLEmote();
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLEmote(id, controller); }
public:
	virtual BOOL getLoop() { return FALSE; }
	virtual F32 getDuration() { return EMOTE_MORPH_FADEIN_TIME + EMOTE_MORPH_IN_TIME + EMOTE_MORPH_FADEOUT_TIME; }
	virtual F32 getEaseInDuration() { return EMOTE_MORPH_FADEIN_TIME; }
	virtual F32 getEaseOutDuration() { return EMOTE_MORPH_FADEOUT_TIME; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EMOTE; }
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onActivate();
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
	virtual void onDeactivate();
	virtual BOOL canDeprecate() { return FALSE; }
protected:
	LLCharacter*		mCharacter;
	LLVisualParam*		mParam;
};
#endif
