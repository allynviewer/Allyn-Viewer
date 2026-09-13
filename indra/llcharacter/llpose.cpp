/** 
 * @file llpose.cpp
 * @brief Implementation of LLPose class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "linden_common.h"
#include "llpose.h"
#include "llmotion.h"
#include "llmath.h"
#include "llstl.h"
LLPose::~LLPose()
{
}
LLJointState* LLPose::getFirstJointState()
{
	mListIter = mJointMap.begin();
	if (mListIter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return mListIter->second;
	}
}
LLJointState *LLPose::getNextJointState()
{
	mListIter++;
	if (mListIter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return mListIter->second;
	}
}
BOOL LLPose::addJointState(const LLPointer<LLJointState>& jointState)
{
	llassert_always(jointState.notNull());
	if (mJointMap.find(jointState->getJoint()->getName()) == mJointMap.end())
	{
		mJointMap[jointState->getJoint()->getName()] = jointState;
	}
	return TRUE;
}
BOOL LLPose::removeJointState(const LLPointer<LLJointState>& jointState)
{
	mJointMap.erase(jointState->getJoint()->getName());
	return TRUE;
}
BOOL LLPose::removeAllJointStates()
{
	mJointMap.clear();
	return TRUE;
}
LLJointState* LLPose::findJointState(LLJoint *joint)
{
	joint_map_iterator iter = mJointMap.find(joint->getName());
	if (iter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return iter->second;
	}
}
LLJointState* LLPose::findJointState(const std::string &name)
{
	joint_map_iterator iter = mJointMap.find(name);
	if (iter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return iter->second;
	}
}
void LLPose::setWeight(F32 weight)
{
	joint_map_iterator iter;
	for(iter = mJointMap.begin();
		iter != mJointMap.end();
		++iter)
	{
		llassert_always(iter->second.notNull());
		iter->second->setWeight(weight);
	}
	mWeight = weight;
}
F32 LLPose::getWeight() const
{
	return mWeight;
}
S32 LLPose::getNumJointStates() const
{
	return (S32)mJointMap.size();
}
LLJointStateBlender::LLJointStateBlender()
{
	for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
	{
		mJointStates[i] = NULL;
		mPriorities[i] = S32_MIN;
		mAdditiveBlends[i] = FALSE;
	}
}
LLJointStateBlender::~LLJointStateBlender()
{
}
BOOL LLJointStateBlender::addJointState(const LLPointer<LLJointState>& joint_state, S32 priority, BOOL additive_blend)
{
	llassert(joint_state);
	if (!joint_state->getJoint())
		return FALSE;
	for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
	{
		if (mJointStates[i].isNull())
		{
			mJointStates[i] = joint_state;
			mPriorities[i] = priority;
			mAdditiveBlends[i] = additive_blend;
			return TRUE;
		}
		else if (priority > mPriorities[i])
		{
			for (S32 j = JSB_NUM_JOINT_STATES - 1; j > i; j--)
			{
				mJointStates[j] = mJointStates[j - 1];
				mPriorities[j] = mPriorities[j - 1];
				mAdditiveBlends[j] = mAdditiveBlends[j - 1];
			}
			mJointStates[i] = joint_state;
			mPriorities[i] = priority;
			mAdditiveBlends[i] = additive_blend;
			return TRUE;
		}
	}
	return FALSE;
}
void LLJointStateBlender::blendJointStates(BOOL apply_now)
{
	if (mJointStates[0].isNull())
	{
		return;
	}
	LLJoint* target_joint = apply_now ? mJointStates[0]->getJoint() : &mJointCache;
	const S32 POS_WEIGHT = 0;
	const S32 ROT_WEIGHT = 1;
	const S32 SCALE_WEIGHT = 2;
	F32				sum_weights[3];
	U32				sum_usage = 0;
	LLVector3		blended_pos = target_joint->getPosition();
	LLQuaternion	blended_rot = target_joint->getRotation();
	LLVector3		blended_scale = target_joint->getScale();
	LLVector3		added_pos;
	LLQuaternion	added_rot;
	LLVector3		added_scale;
	sum_weights[POS_WEIGHT] = 0.f;
	sum_weights[ROT_WEIGHT] = 0.f;
	sum_weights[SCALE_WEIGHT] = 0.f;
	for(S32 joint_state_index = 0;
		joint_state_index < JSB_NUM_JOINT_STATES && mJointStates[joint_state_index].notNull();
		joint_state_index++)
	{
		LLJointState* jsp = mJointStates[joint_state_index];
		U32 current_usage = jsp->getUsage();
		F32 current_weight = jsp->getWeight();
		if (current_weight == 0.f)
		{
			continue;
		}
		if (mAdditiveBlends[joint_state_index])
		{
			if(current_usage & LLJointState::POS)
			{
				F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[POS_WEIGHT]);
				added_pos += jsp->getPosition() * (new_weight_sum - sum_weights[POS_WEIGHT]);
			}
			if(current_usage & LLJointState::SCALE)
			{
				F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[SCALE_WEIGHT]);
				added_scale += jsp->getScale() * (new_weight_sum - sum_weights[SCALE_WEIGHT]);
			}
			if (current_usage & LLJointState::ROT)
			{
				F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[ROT_WEIGHT]);
				added_rot = nlerp((new_weight_sum - sum_weights[ROT_WEIGHT]), added_rot, jsp->getRotation()) * added_rot;
			}
		}
		else
		{
			if(current_usage & LLJointState::POS)
			{
				if(sum_usage & LLJointState::POS)
				{
					F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[POS_WEIGHT]);
					blended_pos = lerp(jsp->getPosition(), blended_pos, sum_weights[POS_WEIGHT] / new_weight_sum);
					sum_weights[POS_WEIGHT] = new_weight_sum;
				}
				else
				{
					blended_pos = jsp->getPosition();
					sum_weights[POS_WEIGHT] = current_weight;
				}
			}
			if(current_usage & LLJointState::SCALE)
			{
				if(sum_usage & LLJointState::SCALE)
				{
					F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[SCALE_WEIGHT]);
					blended_scale = lerp(jsp->getScale(), blended_scale, sum_weights[SCALE_WEIGHT] / new_weight_sum);
					sum_weights[SCALE_WEIGHT] = new_weight_sum;
				}
				else
				{
					blended_scale = jsp->getScale();
					sum_weights[SCALE_WEIGHT] = current_weight;
				}
			}
			if (current_usage & LLJointState::ROT)
			{
				if(sum_usage & LLJointState::ROT)
				{
					F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[ROT_WEIGHT]);
					blended_rot = nlerp(sum_weights[ROT_WEIGHT] / new_weight_sum, jsp->getRotation(), blended_rot);
					sum_weights[ROT_WEIGHT] = new_weight_sum;
				}
				else
				{
					blended_rot = jsp->getRotation();
					sum_weights[ROT_WEIGHT] = current_weight;
				}
			}
			sum_usage = sum_usage | current_usage;
		}
	}
	if (!added_scale.isFinite())
	{
		added_scale.clearVec();
	}
	if (!blended_scale.isFinite())
	{
		blended_scale.setVec(1,1,1);
	}
	target_joint->setPosition(blended_pos + added_pos);
	target_joint->setScale(blended_scale + added_scale);
	target_joint->setRotation(added_rot * blended_rot);
	if (apply_now)
	{
		for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
		{
			mJointStates[i] = NULL;
		}
	}
}
void LLJointStateBlender::interpolate(F32 u)
{
	if (!mJointStates[0])
	{
		return;
	}
	LLJoint* target_joint = mJointStates[0]->getJoint();
	if (!target_joint)
	{
		return;
	}
	target_joint->setPosition(lerp(target_joint->getPosition(), mJointCache.getPosition(), u));
	target_joint->setScale(lerp(target_joint->getScale(), mJointCache.getScale(), u));
	target_joint->setRotation(nlerp(u, target_joint->getRotation(), mJointCache.getRotation()));
}
void LLJointStateBlender::clear()
{
	for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
	{
		mJointStates[i] = NULL;
	}
}
void LLJointStateBlender::resetCachedJoint()
{
	if (!mJointStates[0])
	{
		return;
	}
	LLJoint* source_joint = mJointStates[0]->getJoint();
	mJointCache.setPosition(source_joint->getPosition());
	mJointCache.setScale(source_joint->getScale());
	mJointCache.setRotation(source_joint->getRotation());
}
LLPoseBlender::LLPoseBlender()
	: mNextPoseSlot(0)
{
}
LLPoseBlender::~LLPoseBlender()
{
	for_each(mJointStateBlenderPool.begin(), mJointStateBlenderPool.end(), DeletePairedPointer());
}
BOOL LLPoseBlender::addMotion(LLMotion* motion)
{
	LLPose* pose = motion->getPose();
	for(LLJointState* jsp = pose->getFirstJointState(); jsp; jsp = pose->getNextJointState())
	{
		LLJoint *jointp = jsp->getJoint();
		LLJointStateBlender* joint_blender;
		if (mJointStateBlenderPool.find(jointp) == mJointStateBlenderPool.end())
		{
			joint_blender = new LLJointStateBlender();
			mJointStateBlenderPool[jointp] = joint_blender;
		}
		else
		{
			joint_blender = mJointStateBlenderPool[jointp];
		}
		if (jsp->getPriority() == LLJoint::USE_MOTION_PRIORITY)
		{
			joint_blender->addJointState(jsp, motion->getPriority(), motion->getBlendType() == LLMotion::ADDITIVE_BLEND);
		}
		else
		{
			joint_blender->addJointState(jsp, jsp->getPriority(), motion->getBlendType() == LLMotion::ADDITIVE_BLEND);
		}
		if (std::find(mActiveBlenders.begin(), mActiveBlenders.end(), joint_blender) == mActiveBlenders.end())
		{
			mActiveBlenders.push_front(joint_blender);
		}
	}
	return TRUE;
}
void LLPoseBlender::blendAndApply()
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); ++iter)
	{
		LLJointStateBlender* jsbp = *iter;
		jsbp->blendJointStates();
	}
	mActiveBlenders.clear();
}
void LLPoseBlender::blendAndCache(BOOL reset_cached_joints)
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); ++iter)
	{
		LLJointStateBlender* jsbp = *iter;
		if (reset_cached_joints)
		{
			jsbp->resetCachedJoint();
		}
		jsbp->blendJointStates(FALSE);
	}
}
void LLPoseBlender::interpolate(F32 u)
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); ++iter)
	{
		LLJointStateBlender* jsbp = *iter;
		jsbp->interpolate(u);
	}
}
void LLPoseBlender::clearBlenders()
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); ++iter)
	{
		LLJointStateBlender* jsbp = *iter;
		jsbp->clear();
	}
	mActiveBlenders.clear();
}
