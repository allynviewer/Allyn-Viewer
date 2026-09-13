/** 
 * @file llmotioncontroller.h
 * @brief Implementation of LLMotionController class.
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
#ifndef LL_LLMOTIONCONTROLLER_H
#define LL_LLMOTIONCONTROLLER_H
#include <string>
#include <map>
#include <deque>
#include "llmotion.h"
#include "llpose.h"
#include "llframetimer.h"
#include "llstatemachine.h"
#include "llstring.h"
class LLCharacter;
class LLMotionController;
class LLPauseRequestHandle;
typedef LLPointer<LLPauseRequestHandle> LLAnimPauseRequest;
typedef LLMotion* (*LLMotionConstructor)(LLUUID const& id, LLMotionController*);
class LLMotionRegistry
{
public:
	LLMotionRegistry();
	~LLMotionRegistry();
	BOOL registerMotion( const LLUUID& id, LLMotionConstructor create);
	LLMotion* createMotion(LLUUID const& id, LLMotionController* controller);
	void markBad( const LLUUID& id );
protected:
	typedef std::map<LLUUID, LLMotionConstructor> motion_map_t;
	motion_map_t mMotionTable;
};
class LLMotionController
{
public:
	typedef std::list<LLMotion*> motion_list_t;
	typedef std::set<LLMotion*> motion_set_t;
	BOOL mIsSelf;
public:
	LLMotionController();
	virtual ~LLMotionController();
	void setCharacter( LLCharacter *character );
	BOOL registerMotion( const LLUUID& id, LLMotionConstructor create );
	LLMotion *createMotion( const LLUUID &id );
	void removeMotion( const LLUUID& id );
	BOOL startMotion( const LLUUID &id, F32 start_offset );
	BOOL stopMotionLocally( const LLUUID &id, BOOL stop_immediate );
	void updateLoadingMotions();
	void updateMotions(bool force_update = false);
	void updateMotionsMinimal();
	void clearBlenders() { mPoseBlender.clearBlenders(); }
	void flushAllMotions();
	void deactivateAllMotions();
	void activated(U32 bit) { mActiveMask |= bit; }
	void deactivated(U32 bit) { mActiveMask &= ~bit; }
	bool isactive(U32 bit) const { return (mActiveMask & bit) != 0; }
	void pauseAllMotions();
	void unpauseAllMotions();
	BOOL isPaused() const { return mPaused; }
	U64 getPausedFrame() const { return mPausedFrame; }
	void requestPause(std::vector<LLAnimPauseRequest>& avatar_pause_handles);
	void pauseAllSyncedCharacters(std::vector<LLAnimPauseRequest>& avatar_pause_handles);
	void setTimeStep(F32 step);
    F32 getTimeStep() const { return mTimeStep; }
	void setTimeFactor(F32 time_factor);
	F32 getTimeFactor() const { return mTimeFactor; }
    F32 getAnimTime() const { return mAnimTime; }
	motion_list_t& getActiveMotions() { return mActiveMotions; }
	void incMotionCounts(S32& num_motions, S32& num_loading_motions, S32& num_loaded_motions, S32& num_active_motions, S32& num_deprecated_motions);
	bool isMotionActive( LLMotion *motion );
	bool isMotionLoading( LLMotion *motion );
	LLMotion *findMotion( const LLUUID& id ) const;
	void dumpMotions();
	const LLFrameTimer& getFrameTimer() { return mTimer; }
	static F32	getCurrentTimeFactor()				{ return sCurrentTimeFactor;	};
	static void setCurrentTimeFactor(F32 factor)	{ sCurrentTimeFactor = factor;	};
protected:
	void deleteAllMotions();
public:
	BOOL activateMotionInstance(LLMotion *motion, F32 time);
protected:
	BOOL deactivateMotionInstance(LLMotion *motion);
	void deprecateMotionInstance(LLMotion* motion);
	BOOL stopMotionInstance(LLMotion *motion, BOOL stop_imemdiate);
	void removeMotionInstance(LLMotion* motion);
	void updateRegularMotions();
	void updateAdditiveMotions();
	void resetJointSignatures();
	void updateMotionsByType(LLMotion::LLMotionBlendType motion_type);
	void updateIdleMotion(LLMotion* motionp);
	void updateIdleActiveMotions();
	void purgeExcessMotions();
	void deactivateStoppedMotions();
protected:
	F32					mTimeFactor;
	static F32			sCurrentTimeFactor;
	static LLMotionRegistry	sRegistry;
	LLPoseBlender		mPoseBlender;
	LLCharacter			*mCharacter;
	typedef std::map<LLUUID, LLMotion*> motion_map_t;
	motion_map_t	mAllMotions;
	motion_set_t		mLoadingMotions;
	motion_set_t		mLoadedMotions;
	motion_list_t		mActiveMotions;
	motion_set_t		mDeprecatedMotions;
	U32					mActiveMask;
	int					mDisableSyncing;
	bool				mHidden;
	bool				mHaveVisibleSyncedMotions;
	LLFrameTimer		mTimer;
	F32					mPrevTimerElapsed;
	F32					mAnimTime;
	F32					mLastTime;
	BOOL				mHasRunOnce;
	BOOL				mPaused;
	U64					mPausedFrame;
	F32					mTimeStep;
	S32					mTimeStepCount;
	F32					mLastInterp;
	U8					mJointSignature[2][LL_CHARACTER_MAX_ANIMATED_JOINTS];
public:
	void disable_syncing(void) { mDisableSyncing += 100; }
	void enable_syncing(void) { mDisableSyncing -= 100; }
	bool syncing_disabled(void) const { return mDisableSyncing >= 100; }
	bool isHidden(void) const { return mHidden; }
	bool hidden(bool not_visible) { if (mHidden != not_visible) toggle_hidden(); return !mHaveVisibleSyncedMotions; }
private:
	void toggle_hidden(void);
	void refresh_hidden(void);
	void setHaveVisibleSyncedMotions(void) { mHaveVisibleSyncedMotions = true; }
};
#include "llcharacter.h"
#endif
