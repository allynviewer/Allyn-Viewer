/** 
 * @file llmotion.cpp
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
#include "linden_common.h"
#include "llmotion.h"
#include "llcriticaldamp.h"
#include "llmotioncontroller.h"
AISyncKey* AISyncClientMotion::createSyncKey(AISyncKey const* from_key) const
{
  AISyncClientMotion* self = const_cast<AISyncClientMotion*>(this);
  return new AISyncKeyMotion(from_key, self->getDuration(), self->getLoop());
}
void AISyncClientMotion::aisync_loading(void)
{
  unregister_client();
  register_client();
}
void AISyncClientMotion::aisync_loaded(void)
{
  AISyncServer* server = this->server();
  if (!server)
  {
	return;
  }
  AISyncKey const& key = server->key();
  bool need_resync = !(server->never_synced() && key.expired());
  AISyncKey* new_key = NULL;
  if (need_resync)
  {
	new_key = createSyncKey(&key);
  }
  server->remove(this);
  if (need_resync)
  {
	AISyncServerMap::instance().register_client(this, new_key);
  }
}
F32 LLMotion::getRuntime(void) const
{
  llassert(mActive);
  return mController->getAnimTime() - mActivationTimestamp;
}
F32 LLMotion::getAnimTime(void) const
{
  return mController->getAnimTime();
}
F32 LLMotion::syncActivationTime(F32 time)
{
  AISyncServer* server = this->server();
  if (!server)
  {
	register_client();
	server = this->server();
  }
  AISyncServer::client_list_t const& clients = server->getClients();
  if (clients.size() > 1)
  {
	AISyncClientMotion* motion_with_smallest_runtime = NULL;
	F32 runtime = 1e10;
	for (AISyncServer::client_list_t::const_iterator client = clients.begin(); client != clients.end(); ++client)
	{
	  if ((client->mReadyEvents & 2))
	  {
		llassert(dynamic_cast<AISyncClientMotion*>(client->mClientPtr));
		AISyncClientMotion* motion = static_cast<AISyncClientMotion*>(client->mClientPtr);
		llassert(static_cast<LLMotion*>(motion)->isActive());
		if (motion->getRuntime() < runtime)
		{
		  runtime = motion->getRuntime();
		  motion_with_smallest_runtime = motion;
		}
	  }
	}
	if (getLoop())
	{
	  if (motion_with_smallest_runtime)
	  {
		time = getAnimTime() - runtime;
	  }
	}
  }
  return time;
}
void AISyncClientMotion::deregistered(void)
{
#ifdef SHOW_ASSERT
  mReadyEvents = 0;
#endif
}
LLMotion::LLMotion(LLUUID const& id, LLMotionController* controller) :
	mStopped(TRUE),
	mActive(FALSE),
	mID(id),
	mController(controller),
	mActivationTimestamp(0.f),
	mStopTimestamp(0.f),
	mSendStopTimestamp(F32_MAX),
	mResidualWeight(0.f),
	mFadeWeight(1.f),
	mDeactivateCallback(NULL),
	mDeactivateCallbackUserData(NULL)
{
	for (S32 i=0; i<3; ++i)
		memset(&mJointSignature[i][0], 0, sizeof(U8) * LL_CHARACTER_MAX_ANIMATED_JOINTS);
}
LLMotion::~LLMotion()
{
}
void LLMotion::fadeOut()
{
	if (mFadeWeight > 0.01f)
	{
		mFadeWeight = lerp(mFadeWeight, 0.f, LLSmoothInterpolation::getInterpolant(0.15f));
	}
	else
	{
		mFadeWeight = 0.f;
	}
}
void LLMotion::fadeIn()
{
	if (mFadeWeight < 0.99f)
	{
		mFadeWeight = lerp(mFadeWeight, 1.f, LLSmoothInterpolation::getInterpolant(0.15f));
	}
	else
	{
		mFadeWeight = 1.f;
	}
}
void LLMotion::addJointState(const LLPointer<LLJointState>& jointState)
{
	mPose.addJointState(jointState);
	S32 priority = jointState->getPriority();
	if (priority == LLJoint::USE_MOTION_PRIORITY)
	{
		priority = getPriority();
	}
	U32 usage = jointState->getUsage();
    S32 joint_num = jointState->getJoint()->getJointNum();
    if ((joint_num >= (S32)LL_CHARACTER_MAX_ANIMATED_JOINTS) || (joint_num < 0))
    {
        LL_WARNS() << "joint_num " << joint_num << " is outside of legal range [0-" << LL_CHARACTER_MAX_ANIMATED_JOINTS << ") for joint " << jointState->getJoint()->getName() << LL_ENDL;
        return;
    }
	mJointSignature[0][joint_num] = (usage & LLJointState::POS) ? (0xff >> (7 - priority)) : 0;
	mJointSignature[1][joint_num] = (usage & LLJointState::ROT) ? (0xff >> (7 - priority)) : 0;
	mJointSignature[2][joint_num] = (usage & LLJointState::SCALE) ? (0xff >> (7 - priority)) : 0;
}
void LLMotion::setDeactivateCallback( void (*cb)(void *), void* userdata )
{
	mDeactivateCallback = cb;
	mDeactivateCallbackUserData = userdata;
}
void LLMotion::setStopTime(F32 time)
{
	mStopTimestamp = time;
	mStopped = TRUE;
}
BOOL LLMotion::isBlending()
{
	return mPose.getWeight() < 1.f;
}
void LLMotion::activate(F32 time)
{
	mActivationTimestamp = time;
	mStopped = FALSE;
	if (mController && !mController->syncing_disabled())
	{
		if (mActive)
		{
			unregister_client();
		}
		ready(6, 2 | (mController->isHidden() ? 0 : 4));
	}
	mActive = TRUE;
	onActivate();
}
void LLMotion::deactivate()
{
	mActive = FALSE;
	mPose.setWeight(0.f);
	if (server())
	{
		ready(6, 0);
		unregister_client();
	}
	if (mDeactivateCallback)
	{
		(*mDeactivateCallback)(mDeactivateCallbackUserData);
		mDeactivateCallback = NULL;
		mDeactivateCallbackUserData = NULL;
	}
	onDeactivate();
}
BOOL LLMotion::canDeprecate()
{
	return TRUE;
}
BOOL AIMaskedMotion::onActivate()
{
	mController->activated(mMaskBit);
	return TRUE;
}
void AIMaskedMotion::onDeactivate()
{
	mController->deactivated(mMaskBit);
}
