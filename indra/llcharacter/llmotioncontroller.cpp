/** 
 * @file llmotioncontroller.cpp
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
#include "linden_common.h"
#include "llmotioncontroller.h"
#include "llkeyframemotion.h"
#include "llmath.h"
#include "lltimer.h"
#include "llanimationstates.h"
#include "llstl.h"
const S32 NUM_JOINT_SIGNATURE_STRIDES = LL_CHARACTER_MAX_ANIMATED_JOINTS / 4;
const U32 MAX_MOTION_INSTANCES = 32;
F32 LLMotionController::sCurrentTimeFactor = 1.f;
LLMotionRegistry LLMotionController::sRegistry;
LLMotionRegistry::LLMotionRegistry()
{
}
LLMotionRegistry::~LLMotionRegistry()
{
	mMotionTable.clear();
}
BOOL LLMotionRegistry::registerMotion( const LLUUID& id, LLMotionConstructor constructor )
{
	return mMotionTable.emplace(id, constructor).second;
}
void LLMotionRegistry::markBad( const LLUUID& id )
{
	mMotionTable[id] = LLMotionConstructor(NULL);
}
LLMotion* LLMotionRegistry::createMotion(LLUUID const& id, LLMotionController* controller)
{
	LLMotionConstructor constructor = get_if_there(mMotionTable, id, LLMotionConstructor(NULL));
	LLMotion* motion = NULL;
	if ( constructor == NULL )
	{
		motion = LLKeyframeMotion::create(id, controller);
	}
	else
	{
		motion = constructor(id, controller);
	}
	return motion;
}
LLMotionController::LLMotionController()
	: mTimeFactor(sCurrentTimeFactor),
	  mCharacter(NULL),
	  mAnimTime(0.f),
	  mActiveMask(0),
	  mDisableSyncing(0),
	  mHidden(false),
	  mHaveVisibleSyncedMotions(false),
	  mPrevTimerElapsed(0.f),
	  mLastTime(0.0f),
	  mHasRunOnce(FALSE),
	  mPaused(FALSE),
	  mPausedFrame(0),
	  mTimeStep(0.f),
	  mTimeStepCount(0),
	  mLastInterp(0.f),
	  mIsSelf(FALSE)
{
}
LLMotionController::~LLMotionController()
{
	deleteAllMotions();
}
void LLMotionController::incMotionCounts(S32& num_motions, S32& num_loading_motions, S32& num_loaded_motions, S32& num_active_motions, S32& num_deprecated_motions)
{
	num_motions += mAllMotions.size();
	num_loading_motions += mLoadingMotions.size();
	num_loaded_motions += mLoadedMotions.size();
	num_active_motions += mActiveMotions.size();
	num_deprecated_motions += mDeprecatedMotions.size();
}
void LLMotionController::deleteAllMotions()
{
	mLoadingMotions.clear();
	mLoadedMotions.clear();
	mActiveMotions.clear();
	mActiveMask = 0;
	for_each(mDeprecatedMotions.begin(), mDeprecatedMotions.end(), DeletePointer());
	mDeprecatedMotions.clear();
	for (motion_map_t::iterator iter = mAllMotions.begin(); iter != mAllMotions.end(); ++iter)
	{
		iter->second->unregister_client();
	}
	for_each(mAllMotions.begin(), mAllMotions.end(), DeletePairedPointer());
	mAllMotions.clear();
}
void LLMotionController::purgeExcessMotions()
{
	if (mLoadedMotions.size() <= MAX_MOTION_INSTANCES)
	{
		return;
	}
	uuid_set_t motions_to_kill;
	if (1)
	{
		mPoseBlender.clearBlenders();
		for (motion_set_t::iterator loaded_motion_it = mLoadedMotions.begin();
			 loaded_motion_it != mLoadedMotions.end();
			 ++loaded_motion_it)
		{
			LLMotion* cur_motionp = *loaded_motion_it;
			if (!isMotionActive(cur_motionp))
			{
				motions_to_kill.insert(cur_motionp->getID());
			}
		}
	}
	for (auto motion_it = motions_to_kill.begin();
		motion_it != motions_to_kill.end();
		++motion_it)
	{
		LLUUID motion_id = *motion_it;
		LLMotion* motionp = findMotion(motion_id);
		if (motionp && !isMotionActive(motionp))
		{
			removeMotion(motion_id);
		}
	}
	if (mLoadedMotions.size() > 2*MAX_MOTION_INSTANCES)
	{
		LL_WARNS_ONCE("Animation") << "> " << 2*MAX_MOTION_INSTANCES << " Loaded Motions" << LL_ENDL;
	}
}
void LLMotionController::deactivateStoppedMotions()
{
	for (motion_list_t::iterator iter = mActiveMotions.begin();
		 iter != mActiveMotions.end(); )
	{
		motion_list_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		if (motionp->isStopped())
		{
			deactivateMotionInstance(motionp);
		}
	}
}
void LLMotionController::setTimeStep(F32 step)
{
	mTimeStep = step;
	if (step != 0.f)
	{
		for (motion_list_t::iterator iter = mActiveMotions.begin();
			 iter != mActiveMotions.end(); ++iter)
		{
			LLMotion* motionp = *iter;
			F32 activation_time = motionp->mActivationTimestamp;
			motionp->mActivationTimestamp = (F32)(llfloor(activation_time / step)) * step;
			BOOL stopped = motionp->isStopped();
			motionp->setStopTime((F32)(llfloor(motionp->getStopTime() / step)) * step);
			motionp->setStopped(stopped);
			motionp->mSendStopTimestamp = (F32)llfloor(motionp->mSendStopTimestamp / step) * step;
		}
	}
}
void LLMotionController::setTimeFactor(F32 time_factor)
{
	mTimeFactor = time_factor;
}
void LLMotionController::setCharacter(LLCharacter *character)
{
	mCharacter = character;
}
BOOL LLMotionController::registerMotion( const LLUUID& id, LLMotionConstructor constructor )
{
	return sRegistry.registerMotion(id, constructor);
}
void LLMotionController::removeMotion( const LLUUID& id)
{
	LLMotion* motionp = findMotion(id);
	if (motionp)
	{
		mAllMotions.erase(id);
		removeMotionInstance(motionp);
		delete motionp;
	}
}
void LLMotionController::removeMotionInstance(LLMotion* motionp)
{
	if (motionp)
	{
		llassert(findMotion(motionp->getID()) != motionp);
		mLoadingMotions.erase(motionp);
		mLoadedMotions.erase(motionp);
		mActiveMotions.remove(motionp);
		if (motionp->isActive())
		{
			motionp->deactivate();
			motion_set_t::iterator found_it = mDeprecatedMotions.find(motionp);
			if (found_it != mDeprecatedMotions.end())
			{
				mDeprecatedMotions.erase(found_it);
				delete motionp;
			}
		}
	}
}
LLMotion* LLMotionController::createMotion( const LLUUID &id )
{
	LLMotion *motion = findMotion(id);
	if (!motion)
	{
		motion = sRegistry.createMotion(id, this);
		if (!motion)
		{
			return NULL;
		}
		const char* motion_name = gAnimLibrary.animStateToString(id);
		if (motion_name)
		{
			motion->setName(motion_name);
		}
		LLMotion::LLMotionInitStatus stat = motion->onInitialize(mCharacter);
		switch(stat)
		{
		case LLMotion::STATUS_FAILURE:
			LL_INFOS() << "Motion " << id << " init failed." << LL_ENDL;
			sRegistry.markBad(id);
			delete motion;
			return NULL;
		case LLMotion::STATUS_HOLD:
			mLoadingMotions.insert(motion);
			break;
		case LLMotion::STATUS_SUCCESS:
		    mLoadedMotions.insert(motion);
			break;
		default:
			LL_ERRS() << "Invalid initialization status" << LL_ENDL;
			break;
		}
		mAllMotions[id] = motion;
	}
	return motion;
}
BOOL LLMotionController::startMotion(const LLUUID &id, F32 start_offset)
{
	LLMotion *motion = findMotion(id);
	if (motion
		&& !mPaused
		&& motion->canDeprecate()
		&& motion->isActive()
		&& motion->getFadeWeight() > 0.01f
		&& (motion->isBlending() || motion->getStopTime() != 0.f))
	{
		deprecateMotionInstance(motion);
		motion = NULL;
	}
	if (!motion)
	{
		motion = createMotion(id);
	}
	if (!motion)
	{
		return FALSE;
	}
	else if (motion->canDeprecate() && isMotionActive(motion))
	{
		return TRUE;
	}
	F32 start_time = mAnimTime - start_offset;
	if (!mDisableSyncing)
	{
	  start_time = motion->syncActivationTime(start_time);
	}
	++mDisableSyncing;
	BOOL res = activateMotionInstance(motion, start_time);
	--mDisableSyncing;
	return res;
}
BOOL LLMotionController::stopMotionLocally(const LLUUID &id, BOOL stop_immediate)
{
	LLMotion *motion = findMotion(id);
	return stopMotionInstance(motion, stop_immediate||mPaused);
}
BOOL LLMotionController::stopMotionInstance(LLMotion* motion, BOOL stop_immediate)
{
	if (!motion)
	{
		return FALSE;
	}
	if (isMotionActive(motion) && !motion->isStopped())
	{
		motion->setStopTime(mAnimTime);
		if (stop_immediate)
		{
			deactivateMotionInstance(motion);
		}
		return TRUE;
	}
	else if (isMotionLoading(motion))
	{
		motion->setStopped(TRUE);
		return TRUE;
	}
	return FALSE;
}
void LLMotionController::updateRegularMotions()
{
	updateMotionsByType(LLMotion::NORMAL_BLEND);
}
void LLMotionController::updateAdditiveMotions()
{
	updateMotionsByType(LLMotion::ADDITIVE_BLEND);
}
void LLMotionController::resetJointSignatures()
{
	memset(&mJointSignature[0][0], 0, sizeof(U8) * LL_CHARACTER_MAX_ANIMATED_JOINTS);
	memset(&mJointSignature[1][0], 0, sizeof(U8) * LL_CHARACTER_MAX_ANIMATED_JOINTS);
}
void LLMotionController::updateIdleMotion(LLMotion* motionp)
{
	if (motionp->isStopped() && mAnimTime > motionp->getStopTime() + motionp->getEaseOutDuration())
	{
		deactivateMotionInstance(motionp);
	}
	else if (motionp->isStopped() && mAnimTime > motionp->getStopTime())
	{
		if (mLastTime <= motionp->getStopTime())
		{
			motionp->mResidualWeight = motionp->getPose()->getWeight();
		}
	}
	else if (mAnimTime > motionp->mSendStopTimestamp)
	{
		if (mLastTime <= motionp->mSendStopTimestamp)
		{
			mCharacter->requestStopMotion( motionp );
			stopMotionInstance(motionp, FALSE);
		}
	}
	else if (mAnimTime >= motionp->mActivationTimestamp)
	{
		if (mLastTime < motionp->mActivationTimestamp)
		{
			motionp->mResidualWeight = motionp->getPose()->getWeight();
		}
	}
}
void LLMotionController::updateIdleActiveMotions()
{
	for (motion_list_t::iterator iter = mActiveMotions.begin();
		 iter != mActiveMotions.end(); )
	{
		motion_list_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		updateIdleMotion(motionp);
	}
}
static LLTrace::BlockTimerStatHandle FTM_MOTION_ON_UPDATE("Motion onUpdate");
void LLMotionController::updateMotionsByType(LLMotion::LLMotionBlendType anim_type)
{
	BOOL update_result = TRUE;
	U8 last_joint_signature[LL_CHARACTER_MAX_ANIMATED_JOINTS] = {0};
	for (motion_list_t::iterator iter = mActiveMotions.begin();
		 iter != mActiveMotions.end(); )
	{
		motion_list_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		if (motionp->getBlendType() != anim_type)
		{
			continue;
		}
		BOOL update_motion = FALSE;
		if (motionp->getPose()->getWeight() < 1.f)
		{
			update_motion = TRUE;
		}
		else
		{
			for (S32 i = 0; i < NUM_JOINT_SIGNATURE_STRIDES; i++)
			{
		 		U32 *current_signature = (U32*)&(mJointSignature[0][i * 4]);
				U32 test_signature = *(U32*)&(motionp->mJointSignature[0][i * 4]);
				if ((*current_signature | test_signature) > (*current_signature))
				{
					*current_signature |= test_signature;
					update_motion = TRUE;
				}
				*((U32*)&last_joint_signature[i * 4]) = *(U32*)&(mJointSignature[1][i * 4]);
				current_signature = (U32*)&(mJointSignature[1][i * 4]);
				test_signature = *(U32*)&(motionp->mJointSignature[1][i * 4]);
				if ((*current_signature | test_signature) > (*current_signature))
				{
					*current_signature |= test_signature;
					update_motion = TRUE;
				}
			}
		}
		if (!update_motion)
		{
			updateIdleMotion(motionp);
			continue;
		}
		LLPose *posep = motionp->getPose();
		if (mHasRunOnce && motionp->getMinPixelArea() > mCharacter->getPixelArea())
		{
			motionp->fadeOut();
			if (mAnimTime > motionp->mSendStopTimestamp)
			{
				if (mLastTime <= motionp->mSendStopTimestamp)
				{
					mCharacter->requestStopMotion( motionp );
					stopMotionInstance(motionp, FALSE);
				}
			}
			if (motionp->getFadeWeight() < 0.01f)
			{
				if (motionp->isStopped() && mAnimTime > motionp->getStopTime() + motionp->getEaseOutDuration())
				{
					posep->setWeight(0.f);
					deactivateMotionInstance(motionp);
				}
				continue;
			}
		}
		else
		{
			motionp->fadeIn();
		}
		if (motionp->isStopped() && mAnimTime > motionp->getStopTime() + motionp->getEaseOutDuration())
		{
			if (mLastTime <= motionp->getStopTime())
			{
				posep->setWeight(motionp->getFadeWeight());
				motionp->onUpdate(motionp->getStopTime() - motionp->mActivationTimestamp, last_joint_signature);
			}
			else
			{
				posep->setWeight(0.f);
				deactivateMotionInstance(motionp);
				continue;
			}
		}
		else if (motionp->isStopped() && mAnimTime > motionp->getStopTime())
		{
			if (mLastTime <= motionp->getStopTime())
			{
				motionp->mResidualWeight = motionp->getPose()->getWeight();
			}
			if (motionp->getEaseOutDuration() == 0.f)
			{
				posep->setWeight(0.f);
			}
			else
			{
				posep->setWeight(motionp->getFadeWeight() * motionp->mResidualWeight * cubic_step(1.f - ((mAnimTime - motionp->getStopTime()) / motionp->getEaseOutDuration())));
			}
			update_result = motionp->onUpdate(mAnimTime - motionp->mActivationTimestamp, last_joint_signature);
		}
		else if (mAnimTime > motionp->mActivationTimestamp + motionp->getEaseInDuration())
		{
			posep->setWeight(motionp->getFadeWeight());
			if (mAnimTime > motionp->mSendStopTimestamp)
			{
				if (mLastTime <= motionp->mSendStopTimestamp)
				{
					mCharacter->requestStopMotion( motionp );
					stopMotionInstance(motionp, FALSE);
				}
			}
			{
				LL_RECORD_BLOCK_TIME(FTM_MOTION_ON_UPDATE);
				update_result = motionp->onUpdate(mAnimTime - motionp->mActivationTimestamp, last_joint_signature);
			}
		}
		else if (mAnimTime >= motionp->mActivationTimestamp)
		{
			if (mLastTime < motionp->mActivationTimestamp)
			{
				motionp->mResidualWeight = motionp->getPose()->getWeight();
			}
			if (motionp->getEaseInDuration() == 0.f)
			{
				posep->setWeight(motionp->getFadeWeight());
			}
			else
			{
				posep->setWeight(motionp->getFadeWeight() * motionp->mResidualWeight + (1.f - motionp->mResidualWeight) * cubic_step((mAnimTime - motionp->mActivationTimestamp) / motionp->getEaseInDuration()));
			}
			update_result = motionp->onUpdate(mAnimTime - motionp->mActivationTimestamp, last_joint_signature);
		}
		else
		{
			posep->setWeight(0.f);
			update_result = motionp->onUpdate(0.f, last_joint_signature);
		}
		if (!update_result)
		{
			if (!motionp->isStopped() || motionp->getStopTime() > mAnimTime)
			{
				mCharacter->requestStopMotion( motionp );
				stopMotionInstance(motionp, FALSE);
			}
		}
		mPoseBlender.addMotion(motionp);
	}
}
void LLMotionController::updateLoadingMotions()
{
	for (motion_set_t::iterator iter = mLoadingMotions.begin();
		 iter != mLoadingMotions.end(); )
	{
		motion_set_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		if( !motionp)
		{
			continue;
		}
		LLMotion::LLMotionInitStatus status = motionp->onInitialize(mCharacter);
		if (status == LLMotion::STATUS_SUCCESS)
		{
			mLoadingMotions.erase(curiter);
			mLoadedMotions.insert(motionp);
			if (!motionp->isStopped())
			{
				F32 start_time = mAnimTime;
				if (!mDisableSyncing)
				{
				  motionp->aisync_loaded();
				  start_time = motionp->syncActivationTime(start_time);
				}
				++mDisableSyncing;
				activateMotionInstance(motionp, start_time);
				--mDisableSyncing;
			}
		}
		else if (status == LLMotion::STATUS_FAILURE)
		{
			LL_INFOS() << "Motion " << motionp->getID() << " init failed." << LL_ENDL;
			sRegistry.markBad(motionp->getID());
			mLoadingMotions.erase(curiter);
			llassert(mDeprecatedMotions.find(motionp) == mDeprecatedMotions.end());
			mAllMotions.erase(motionp->getID());
			motionp->unregister_client();
			delete motionp;
		}
	}
}
void LLMotionController::updateMotions(bool force_update)
{
	BOOL use_quantum = (mTimeStep != 0.f);
	F32 cur_time = mTimer.getElapsedTimeF32();
	F32 delta_time = cur_time - mPrevTimerElapsed;
	mPrevTimerElapsed = cur_time;
	mLastTime = mAnimTime;
	purgeExcessMotions();
	if (!mPaused)
	{
		F32 update_time = mAnimTime + delta_time * mTimeFactor;
		if (use_quantum)
		{
			F32 time_interval = fmodf(update_time, mTimeStep);
			S32 quantum_count = llmax(ll_pos_round(update_time / mTimeStep), llceil(mAnimTime / mTimeStep));
			if (quantum_count == mTimeStepCount)
			{
				if (!mPaused)
				{
					F32 interp = time_interval / mTimeStep;
					mPoseBlender.interpolate(interp - mLastInterp);
					mLastInterp = interp;
				}
				updateLoadingMotions();
				return;
			}
			mPoseBlender.interpolate(1.f);
			clearBlenders();
			mTimeStepCount = quantum_count;
			mAnimTime = (F32)quantum_count * mTimeStep;
			mLastInterp = 0.f;
		}
		else
		{
			mAnimTime = llmax(mAnimTime, update_time);
		}
	}
	updateLoadingMotions();
	resetJointSignatures();
	if (mPaused && !force_update)
	{
		updateIdleActiveMotions();
	}
	else
	{
		updateAdditiveMotions();
		resetJointSignatures();
		updateRegularMotions();
		if (use_quantum)
		{
			mPoseBlender.blendAndCache(TRUE);
		}
		else
		{
			mPoseBlender.blendAndApply();
		}
	}
	mHasRunOnce = TRUE;
}
void LLMotionController::updateMotionsMinimal()
{
	mPrevTimerElapsed = mTimer.getElapsedTimeF32();
	purgeExcessMotions();
	updateLoadingMotions();
	resetJointSignatures();
	deactivateStoppedMotions();
	mHasRunOnce = TRUE;
}
BOOL LLMotionController::activateMotionInstance(LLMotion *motion, F32 time)
{
	if (motion == NULL || motion->getPose() == NULL)
	{
		return FALSE;
	}
	if (mLoadingMotions.find(motion) != mLoadingMotions.end())
	{
		if (!syncing_disabled())
		{
			motion->aisync_loading();
		}
		motion->setStopped(FALSE);
		return TRUE;
	}
	motion->mResidualWeight = motion->getPose()->getWeight();
	if (motion->getDuration() != 0.f && !motion->getLoop())
	{
		F32 ease_out_time;
		F32 motion_duration;
		ease_out_time = motion->getEaseOutDuration();
		motion_duration = llmax(motion->getDuration() - ease_out_time, 0.f);
		motion->mSendStopTimestamp = time + motion_duration;
	}
	else
	{
		motion->mSendStopTimestamp = F32_MAX;
	}
	if (motion->isActive())
	{
		mActiveMotions.remove(motion);
	}
	mActiveMotions.push_front(motion);
	motion->activate(time);
	motion->onUpdate(0.f, mJointSignature[1]);
	if (mAnimTime >= motion->mSendStopTimestamp)
	{
		motion->setStopTime(motion->mSendStopTimestamp);
		if (motion->mResidualWeight == 0.0f)
		{
			motion->mResidualWeight = 1.f;
		}
	}
	return TRUE;
}
BOOL LLMotionController::deactivateMotionInstance(LLMotion *motion)
{
	motion_set_t::iterator found_it = mDeprecatedMotions.find(motion);
	if (found_it != mDeprecatedMotions.end())
	{
		removeMotionInstance(motion);
	}
	else
	{
		motion->deactivate();
		mActiveMotions.remove(motion);
	}
	return TRUE;
}
void LLMotionController::deprecateMotionInstance(LLMotion* motion)
{
	mDeprecatedMotions.insert(motion);
	stopMotionInstance(motion, FALSE);
	mAllMotions.erase(motion->getID());
}
bool LLMotionController::isMotionActive(LLMotion *motion)
{
	return (motion && motion->isActive());
}
bool LLMotionController::isMotionLoading(LLMotion* motion)
{
	return (mLoadingMotions.find(motion) != mLoadingMotions.end());
}
LLMotion* LLMotionController::findMotion(const LLUUID& id) const
{
	motion_map_t::const_iterator iter = mAllMotions.find(id);
	if(iter == mAllMotions.end())
	{
		return NULL;
	}
	else
	{
		return iter->second;
	}
}
void LLMotionController::dumpMotions()
{
	LL_INFOS() << "=====================================" << LL_ENDL;
	for (motion_map_t::iterator iter = mAllMotions.begin();
		 iter != mAllMotions.end(); iter++)
	{
		LLUUID id = iter->first;
		std::string state_string;
		LLMotion *motion = iter->second;
		if (mLoadingMotions.find(motion) != mLoadingMotions.end())
			state_string += std::string("l");
		if (mLoadedMotions.find(motion) != mLoadedMotions.end())
			state_string += std::string("L");
		if (std::find(mActiveMotions.begin(), mActiveMotions.end(), motion)!=mActiveMotions.end())
			state_string += std::string("A");
		llassert(mDeprecatedMotions.find(motion) == mDeprecatedMotions.end());
		LL_INFOS() << gAnimLibrary.animationName(id) << " " << state_string << LL_ENDL;
	}
	for (motion_set_t::iterator iter = mDeprecatedMotions.begin();
		 iter != mDeprecatedMotions.end(); ++iter)
	{
		std::string state_string;
		LLMotion* motion = *iter;
		LLUUID id = motion->getID();
		llassert(mLoadingMotions.find(motion) == mLoadingMotions.end());
		if (mLoadedMotions.find(motion) != mLoadedMotions.end())
			state_string += std::string("L");
		if (std::find(mActiveMotions.begin(), mActiveMotions.end(), motion)!=mActiveMotions.end())
			state_string += std::string("A");
		state_string += "D";
		LL_INFOS() << gAnimLibrary.animationName(id) << " " << state_string << LL_ENDL;
	}
}
void LLMotionController::deactivateAllMotions()
{
	for (motion_list_t::iterator iter = mActiveMotions.begin(); iter != mActiveMotions.end();)
	{
		deactivateMotionInstance(*iter++);
	}
}
void LLMotionController::flushAllMotions()
{
	std::vector<std::pair<LLUUID,F32> > active_motions;
	active_motions.reserve(mActiveMotions.size());
	for (motion_list_t::iterator iter = mActiveMotions.begin();
		 iter != mActiveMotions.end(); )
	{
		motion_list_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		F32 dtime = mAnimTime - motionp->mActivationTimestamp;
		active_motions.push_back(std::make_pair(motionp->getID(),dtime));
		motionp->deactivate();
	}
	deleteAllMotions();
	mCharacter->removeAnimationData("Hand Pose");
	disable_syncing();
	for (std::vector<std::pair<LLUUID,F32> >::iterator iter = active_motions.begin();
		 iter != active_motions.end(); ++iter)
	{
		startMotion(iter->first, iter->second);
	}
	enable_syncing();
}
void LLMotionController::toggle_hidden(void)
{
	mHaveVisibleSyncedMotions = mHidden;
	mHidden = !mHidden;
	synceventset_t const visible = mHidden ? 0 : 4;
	for (motion_list_t::iterator iter = mActiveMotions.begin(); iter != mActiveMotions.end(); ++iter)
	{
		LLMotion* motionp = *iter;
		AISyncServer* server = motionp->server();
		if (server && !server->never_synced() && motionp->isActive())
		{
			bool visible_before = server->events_with_at_least_one_client_ready() & 4;
			server->ready(4, visible, motionp);
			bool visible_after = server->events_with_at_least_one_client_ready() & 4;
			if (visible_after)
			{
				mHaveVisibleSyncedMotions = true;
			}
			if (visible_before != visible_after)
			{
				AISyncServer::client_list_t const& clients = server->getClients();
				for (AISyncServer::client_list_t::const_iterator client = clients.begin(); client != clients.end(); ++client)
				{
					LLMotion* motion = dynamic_cast<LLMotion*>(client->mClientPtr);
					if (!motion)
					{
						continue;
					}
					LLMotionController* controller = motion->getController();
					if (controller == this)
					{
						continue;
					}
					if (visible_after)
					{
					  controller->setHaveVisibleSyncedMotions();
					}
					else
					{
					  controller->refresh_hidden();
					}
				}
			}
		}
	}
}
void LLMotionController::refresh_hidden(void)
{
	mHaveVisibleSyncedMotions = !mHidden;
	for (motion_list_t::iterator iter = mActiveMotions.begin(); iter != mActiveMotions.end(); ++iter)
	{
		LLMotion* motionp = *iter;
		AISyncServer* server = motionp->server();
		if (server && !server->never_synced() && motionp->isActive())
		{
			bool visible_after = server->events_with_at_least_one_client_ready() & 4;
			if (visible_after)
			{
				mHaveVisibleSyncedMotions = true;
			}
		}
	}
}
void LLMotionController::pauseAllSyncedCharacters(std::vector<LLAnimPauseRequest>& avatar_pause_handles)
{
	for (motion_list_t::iterator iter = mActiveMotions.begin(); iter != mActiveMotions.end(); ++iter)
	{
		LLMotion* motionp = *iter;
		AISyncServer* server = motionp->server();
		if (server && !server->never_synced() && motionp->isActive())
		{
			AISyncServer::client_list_t const& clients = server->getClients();
			for (AISyncServer::client_list_t::const_iterator client = clients.begin(); client != clients.end(); ++client)
			{
				LLMotion* motion = dynamic_cast<LLMotion*>(client->mClientPtr);
				if (!motion)
				{
					continue;
				}
				LLMotionController* controller = motion->getController();
				if (controller == this)
				{
					continue;
				}
				controller->requestPause(avatar_pause_handles);
			}
		}
	}
}
void LLMotionController::requestPause(std::vector<LLAnimPauseRequest>& avatar_pause_handles)
{
	if (mCharacter)
	{
		mCharacter->requestPause(avatar_pause_handles);
	}
}
void LLMotionController::pauseAllMotions()
{
	if (!mPaused)
	{
		mPaused = TRUE;
        mPausedFrame = LLFrameTimer::getFrameCount();
	}
}
void LLMotionController::unpauseAllMotions()
{
	if (mPaused)
	{
		mPaused = FALSE;
	}
}
