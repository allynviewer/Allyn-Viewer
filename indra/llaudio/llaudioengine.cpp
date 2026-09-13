/** 
 * @file audioengine.cpp
 * @brief implementation of LLAudioEngine class abstracting the Open
 * AL audio support
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#include "llaudioengine.h"
#include "llstreamingaudio.h"
#include "llerror.h"
#include "llmath.h"
#include "sound_ids.h"
#include "llvfs.h"
#include "lldir.h"
#include "llaudiodecodemgr.h"
#include "llassetstorage.h"
#include <queue>
#include <boost/pool/pool_alloc.hpp>
extern void request_sound(const LLUUID &sound_guid);
LLAudioEngine* gAudiop = NULL;
int gSoundHistoryPruneCounter = 0;
LLAudioEngine::LLAudioEngine()
{
	setDefaults();
}
LLAudioEngine::~LLAudioEngine()
{
}
LLStreamingAudioInterface* LLAudioEngine::getStreamingAudioImpl()
{
	return mStreamingAudioImpl;
}
void LLAudioEngine::setStreamingAudioImpl(LLStreamingAudioInterface *impl)
{
	mStreamingAudioImpl = impl;
}
void LLAudioEngine::setDefaults()
{
	mMaxWindGain = 1.f;
	mListenerp = NULL;
	mMuted = false;
	mUserData = NULL;
	mLastStatus = 0;
	mNumChannels = 0;
	mEnableWind = false;
	S32 i;
	for (i = 0; i < MAX_CHANNELS; i++)
	{
		mChannels[i] = NULL;
	}
	for (i = 0; i < MAX_BUFFERS; i++)
	{
		mBuffers[i] = NULL;
	}
	mMasterGain = 1.f;
	mInternalGain = -1.f;
	mNextWindUpdate = 0.f;
	mStreamingAudioImpl = NULL;
	for (U32 i = 0; i < LLAudioEngine::AUDIO_TYPE_COUNT; i++)
		mSecondaryGain[i] = 1.0f;
	mCurrentTransfer = NULL;
	mAllowLargeSounds = false;
}
bool LLAudioEngine::init(const S32 num_channels, void* userdata)
{
	setDefaults();
	mNumChannels = num_channels;
	mUserData = userdata;
	gAudioDecodeMgrp = new LLAudioDecodeMgr;
	LL_INFOS("AudioEngine") << "LLAudioEngine::init() AudioEngine successfully initialized" << LL_ENDL;
	return true;
}
void LLAudioEngine::shutdown()
{
	delete gAudioDecodeMgrp;
	gAudioDecodeMgrp = NULL;
	cleanupWind();
	source_map::iterator iter_src;
	for (iter_src = mAllSources.begin(); iter_src != mAllSources.end(); iter_src++)
	{
		delete iter_src->second;
	}
	data_map::iterator iter_data;
	for (iter_data = mAllData.begin(); iter_data != mAllData.end(); iter_data++)
	{
		delete iter_data->second;
	}
	S32 i;
	for (i = 0; i < MAX_CHANNELS; i++)
	{
		delete mChannels[i];
		mChannels[i] = NULL;
	}
	for (i = 0; i < MAX_BUFFERS; i++)
	{
		delete mBuffers[i];
		mBuffers[i] = NULL;
	}
	delete mStreamingAudioImpl;
	mStreamingAudioImpl = NULL;
}
void LLAudioEngine::startInternetStream(const std::string& url)
{
	if (mStreamingAudioImpl)
		mStreamingAudioImpl->start(url);
}
void LLAudioEngine::stopInternetStream()
{
	if (mStreamingAudioImpl)
		mStreamingAudioImpl->stop();
}
void LLAudioEngine::pauseInternetStream(int pause)
{
	if (mStreamingAudioImpl)
		mStreamingAudioImpl->pause(pause);
}
void LLAudioEngine::updateInternetStream()
{
	if (mStreamingAudioImpl)
		mStreamingAudioImpl->update();
}
LLAudioEngine::LLAudioPlayState LLAudioEngine::isInternetStreamPlaying()
{
	if (mStreamingAudioImpl)
		return (LLAudioEngine::LLAudioPlayState) mStreamingAudioImpl->isPlaying();
	return LLAudioEngine::AUDIO_STOPPED;
}
void LLAudioEngine::setInternetStreamGain(F32 vol)
{
	if (mStreamingAudioImpl)
		mStreamingAudioImpl->setGain(vol);
}
std::string LLAudioEngine::getInternetStreamURL()
{
	if (mStreamingAudioImpl)
		return mStreamingAudioImpl->getURL();
	else return std::string();
}
void LLAudioEngine::checkStates()
{
#ifdef SHOW_ASSERT
	for (S32 i = 0; i < MAX_BUFFERS; i++)
	{
		if (mBuffers[i])
		{
			bool buf_has_ref = false;
			for (S32 j = 0; j < MAX_CHANNELS; j++)
			{
				if (mChannels[j])
				{
					if(mChannels[j]->mCurrentBufferp == mBuffers[i])
						buf_has_ref = true;
				}
			}
			if(buf_has_ref)
				llassert(mBuffers[i]->mInUse);
		}
	}
#endif
}
void LLAudioEngine::updateChannels()
{
	S32 i;
	for (i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i])
		{
			mChannels[i]->updateBuffer();
			mChannels[i]->update3DPosition();
			mChannels[i]->updateLoop();
#ifdef SHOW_ASSERT
			if(mChannels[i]->getSource())
				llassert(mChannels[i]->mCurrentBufferp == mChannels[i]->getSource()->getCurrentBuffer());
			if(mChannels[i]->mCurrentBufferp)
			{
				bool found_buffer = false;
				for (S32 j = 0; j < MAX_BUFFERS; j++)
				{
					if (mBuffers[j])
					{
						if(mChannels[i]->mCurrentBufferp == mBuffers[j])
							found_buffer = true;
					}
				}
				llassert(found_buffer);
			}
#endif
		}
	}
	checkStates();
}
struct SourcePriorityComparator
{
	bool operator() (const LLAudioSource* lhs, const LLAudioSource* rhs) const
	{
		if(rhs->getPriority() != lhs->getPriority())
			return rhs->getPriority() > lhs->getPriority();
		else
			return ((rhs->isSyncMaster() && !lhs->isSyncMaster()) || (rhs->isSyncSlave() && (!lhs->isSyncMaster() && !lhs->isSyncSlave())));
	}
};
static const F32 default_max_decode_time = .002f;
void LLAudioEngine::idle(F32 max_decode_time)
{
	if (max_decode_time <= 0.f)
	{
		max_decode_time = default_max_decode_time;
	}
	S32 i;
	for (i = 0; i < MAX_BUFFERS; i++)
	{
		if (mBuffers[i])
		{
			mBuffers[i]->mInUse = false;
		}
	}
	LLAudioSource *sync_masterp = NULL;
	LLAudioSource *sync_slavep = NULL;
	std::priority_queue<LLAudioSource*,std::vector<LLAudioSource*,boost::pool_allocator<LLAudioSource*> >,SourcePriorityComparator> queue;
	static std::vector<LLAudioSource*> slave_list;
	slave_list.clear();
	for (source_map::iterator iter = mAllSources.begin(); iter != mAllSources.end();)
	{
		LLAudioSource *sourcep = iter->second;
		sourcep->update();
		if (sourcep->isDone())
		{
			delete sourcep;
			mAllSources.erase(iter++);
			continue;
		}
		++iter;
		if(!sourcep->isLoop() && sourcep->mPlayedOnce && (!sourcep->mChannelp || !sourcep->mChannelp->isPlaying()))
		{
			continue;
		}
		LLAudioData *adp = sourcep->getCurrentData();
		if (!adp || !adp->getBuffer())
		{
			continue;
		}
		sourcep->updatePriority();
		if (sourcep->getPriority() < F_APPROXIMATELY_ZERO)
		{
			continue;
		}
		else if(sourcep->isSyncMaster())
		{
			if(!sync_masterp || sourcep->getPriority() > sync_masterp->getPriority())
			{
				if(sync_masterp && !sync_masterp->getChannel())
					queue.push(sync_masterp);
				sync_masterp = sourcep;
				continue;
			}
		}
		else if(sourcep->isSyncSlave())
		{
			if(!sync_slavep || sourcep->getPriority() > sync_slavep->getPriority())
			{
				sync_slavep = sourcep;
			}
			slave_list.push_back(sourcep);
			continue;
		}
		if(sourcep->getChannel())
		{
			continue;
		}
		queue.push(sourcep);
	}
	updateChannels();
	for(std::vector<LLAudioSource*>::iterator iter=slave_list.begin();iter!=slave_list.end();++iter)
	{
		if(!sync_masterp)
		{
			if((*iter)->getChannel() && (*iter)->isLoop())
			{
				(*iter)->getChannel()->cleanup();
			}
		}
		else if((!sync_masterp->getChannel() || sync_masterp->getChannel()->mLoopedThisFrame))
		{
			if(!(*iter)->getChannel() || (*iter)->isLoop())
				queue.push((*iter));
		}
	}
	if(sync_masterp)
	{
		if(sync_slavep)
			sync_masterp->setPriority(sync_slavep->getPriority());
		if(!sync_masterp->getChannel())
		{
			queue.push(sync_masterp);
		}
	}
	bool syncmaster_started = sync_masterp && sync_masterp->getChannel() && sync_masterp->getChannel()->mLoopedThisFrame;
	while(!queue.empty())
	{
		LLAudioSource *sourcep = queue.top();
		queue.pop();
		if (sourcep->isSyncSlave() && !syncmaster_started)
		{
			continue;
		}
		LLAudioChannel *channelp = sourcep->getChannel();
		if (!channelp)
		{
			if(!(channelp = getFreeChannel(sourcep->getPriority())))
			{
				break;
			}
			syncmaster_started |= (sourcep == sync_masterp);
			channelp->setSource(sourcep);
		}
		if(sourcep->isSyncSlave())
			channelp->playSynced(sync_masterp->getChannel());
		else
			channelp->play();
	}
	commitDeferredChanges();
	for (i = 0; i < MAX_BUFFERS; i++)
	{
		if (mBuffers[i])
		{
			if (!mBuffers[i]->mInUse && mBuffers[i]->mLastUseTimer.getElapsedTimeF32() > 30.f)
			{
				LL_DEBUGS("AudioEngine") << "Flushing unused buffer!" << LL_ENDL;
				mBuffers[i]->mAudioDatap->mBufferp = NULL;
				delete mBuffers[i];
				mBuffers[i] = NULL;
			}
		}
	}
	for (i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i])
		{
			mChannels[i]->mLoopedThisFrame = false;
		}
	}
	gAudioDecodeMgrp->processQueue(max_decode_time);
	startNextTransfer();
	updateInternetStream();
}
void LLAudioEngine::enableWind(bool enable)
{
	if (enable && (!mEnableWind))
	{
		mEnableWind = initWind();
	}
	else if (mEnableWind && (!enable))
	{
		mEnableWind = false;
		cleanupWind();
	}
}
LLAudioBuffer * LLAudioEngine::getFreeBuffer()
{
	S32 i;
	for (i = 0; i < MAX_BUFFERS; i++)
	{
		if (!mBuffers[i])
		{
			mBuffers[i] = createBuffer();
			return mBuffers[i];
		}
	}
	F32 max_age = -1.f;
	S32 buffer_id = -1;
	for (i = 0; i < MAX_BUFFERS; i++)
	{
		if (mBuffers[i])
		{
			if (!mBuffers[i]->mInUse)
			{
				if (mBuffers[i]->mLastUseTimer.getElapsedTimeF32() > max_age)
				{
					max_age = mBuffers[i]->mLastUseTimer.getElapsedTimeF32();
					buffer_id = i;
				}
			}
		}
	}
	if (buffer_id >= 0)
	{
		LL_DEBUGS("AudioEngine") << "Taking over unused buffer! max_age=" << max_age << LL_ENDL;
		mBuffers[buffer_id]->mAudioDatap->mBufferp = NULL;
		for (U32 i = 0; i < MAX_CHANNELS; i++)
		{
			LLAudioChannel* channelp = mChannels[i];
			if(channelp && channelp->mCurrentBufferp == mBuffers[buffer_id])
			{
				channelp->cleanup();
				llassert(channelp->mCurrentBufferp == NULL);
			}
		}
		delete mBuffers[buffer_id];
		mBuffers[buffer_id] = createBuffer();
		return mBuffers[buffer_id];
	}
	return NULL;
}
LLAudioChannel * LLAudioEngine::getFreeChannel(const F32 priority)
{
	S32 i;
	for (i = 0; i < mNumChannels; i++)
	{
		if (!mChannels[i])
		{
			mChannels[i] = createChannel();
			return mChannels[i];
		}
		else
		{
			if (mChannels[i]->isFree())
			{
				LL_DEBUGS("AudioEngine") << "Replacing unused channel" << LL_ENDL;
				return mChannels[i];
			}
		}
	}
	F32 min_priority = 10000.f;
	LLAudioChannel *min_channelp = NULL;
	for (i = 0; i < mNumChannels; i++)
	{
		LLAudioChannel *channelp = mChannels[i];
		LLAudioSource *sourcep = channelp->getSource();
		if (sourcep->getPriority() < min_priority)
		{
			min_channelp = channelp;
			min_priority = sourcep->getPriority();
		}
	}
	if (!min_channelp || min_priority >= priority)
	{
		return NULL;
	}
	LL_DEBUGS("AudioEngine") << "Flushing min channel" << LL_ENDL;
	min_channelp->cleanup();
	return min_channelp;
}
void LLAudioEngine::cleanupBuffer(LLAudioBuffer *bufferp)
{
	S32 i;
	for (i = 0; i < MAX_BUFFERS; i++)
	{
		if (mBuffers[i] == bufferp)
		{
			delete mBuffers[i];
			mBuffers[i] = NULL;
		}
	}
}
bool LLAudioEngine::preloadSound(const LLUUID &uuid)
{
	if(uuid.isNull())
		return false;
	if(getAudioData(uuid)->getLoadState() >= LLAudioData::STATE_LOAD_DECODING)
	{
		return true;
	}
	LL_INFOS("AudioEngine") << "Preloading system sound " << uuid << LL_ENDL;
	mPreloadSystemList.push_back(uuid);
	return true;
}
bool LLAudioEngine::isWindEnabled()
{
	return mEnableWind;
}
void LLAudioEngine::setMuted(bool muted)
{
	if (muted != mMuted)
	{
		mMuted = muted;
		setMasterGain(mMasterGain);
	}
	enableWind(!mMuted);
}
void LLAudioEngine::setMasterGain(const F32 gain)
{
	mMasterGain = gain;
	F32 internal_gain = getMuted() ? 0.f : gain;
	if (internal_gain != mInternalGain)
	{
		mInternalGain = internal_gain;
		setInternalGain(mInternalGain);
	}
}
F32 LLAudioEngine::getMasterGain()
{
	return mMasterGain;
}
void LLAudioEngine::setSecondaryGain(S32 type, F32 gain)
{
	llassert(type < LLAudioEngine::AUDIO_TYPE_COUNT);
	mSecondaryGain[type] = gain;
}
F32 LLAudioEngine::getSecondaryGain(S32 type)
{
	return mSecondaryGain[type];
}
F32 LLAudioEngine::getInternetStreamGain()
{
	if (mStreamingAudioImpl)
		return mStreamingAudioImpl->getGain();
	else
		return 1.0f;
}
void LLAudioEngine::setMaxWindGain(F32 gain)
{
	mMaxWindGain = gain;
}
F64 LLAudioEngine::mapWindVecToGain(LLVector3 wind_vec)
{
	F64 gain = 0.0;
	gain = wind_vec.magVec();
	if (gain)
	{
		if (gain > 20)
		{
			gain = 20;
		}
		gain = gain/20.0;
	}
	return (gain);
}
F64 LLAudioEngine::mapWindVecToPitch(LLVector3 wind_vec)
{
	LLVector3 listen_right;
	F64 theta;
	LLVector3 norm_wind = wind_vec;
	norm_wind.normVec();
	listen_right.setVec(1.0,0.0,0.0);
	theta = acos(norm_wind * listen_right);
	theta /= F_PI;
	if (theta > 0.5) theta = 1.0-theta;
	if (theta < 0) theta = 0;
	return (theta);
}
F64 LLAudioEngine::mapWindVecToPan(LLVector3 wind_vec)
{
	LLVector3 listen_right;
	F64 theta;
	listen_right.setVec(1.0,0.0,0.0);
	LLVector3 norm_wind = wind_vec;
	norm_wind.normVec();
	theta = acos(norm_wind * listen_right);
	theta /= F_PI;
	return (theta);
}
void LLAudioEngine::triggerSound(const LLUUID &audio_uuid, const LLUUID& owner_id, const F32 gain,
								 const S32 type, const LLVector3d &pos_global, const LLUUID source_object)
{
	if(!mListenerp)
		return;
	if (mMuted || gain < FLT_EPSILON*2)
	{
		return;
	}
	LLUUID source_id;
	source_id.generate();
	LLAudioSource *asp = new LLAudioSource(source_id, owner_id, gain, type, source_object, true);
	gAudiop->addAudioSource(asp);
	if (pos_global.isExactlyZero())
	{
		asp->setAmbient(true);
	}
	else
	{
		asp->setPositionGlobal(pos_global);
	}
	asp->updatePriority();
	asp->play(audio_uuid);
}
void LLAudioEngine::setListenerPos(LLVector3 aVec)
{
	if (mListenerp)
	{
		mListenerp->setPosition(aVec);
	}
}
LLVector3 LLAudioEngine::getListenerPos()
{
	if (mListenerp)
	{
		return mListenerp->getPosition();
	}
	else
	{
		return(LLVector3::zero);
	}
}
void LLAudioEngine::setListenerVelocity(LLVector3 aVec)
{
	if (mListenerp)
	{
		mListenerp->setVelocity(aVec);
	}
}
void LLAudioEngine::translateListener(LLVector3 aVec)
{
	if (mListenerp)
	{
		mListenerp->translate(aVec);
	}
}
void LLAudioEngine::orientListener(LLVector3 up, LLVector3 at)
{
	if (mListenerp)
	{
		mListenerp->orient(up, at);
	}
}
void LLAudioEngine::setListener(LLVector3 pos, LLVector3 vel, LLVector3 up, LLVector3 at)
{
	if(!mListenerp)
	{
		allocateListener();
	}
	mListenerp->set(pos,vel,up,at);
}
void LLAudioEngine::setDopplerFactor(F32 factor)
{
	if (mListenerp)
	{
		mListenerp->setDopplerFactor(factor);
	}
}
F32 LLAudioEngine::getDopplerFactor()
{
	if (mListenerp)
	{
		return mListenerp->getDopplerFactor();
	}
	else
	{
		return 0.f;
	}
}
void LLAudioEngine::setRolloffFactor(F32 factor)
{
	if (mListenerp)
	{
		mListenerp->setRolloffFactor(factor);
	}
}
F32 LLAudioEngine::getRolloffFactor()
{
	if (mListenerp)
	{
		return mListenerp->getRolloffFactor();
	}
	else
	{
		return 0.f;
	}
}
void LLAudioEngine::commitDeferredChanges()
{
	if(mListenerp)
	{
		mListenerp->commitDeferredChanges();
	}
}
LLAudioSource * LLAudioEngine::findAudioSource(const LLUUID &source_id)
{
	source_map::iterator iter;
	iter = mAllSources.find(source_id);
	if (iter == mAllSources.end())
	{
		return NULL;
	}
	else
	{
		return iter->second;
	}
}
LLAudioData * LLAudioEngine::getAudioData(const LLUUID &audio_uuid)
{
	data_map::iterator iter;
	iter = mAllData.find(audio_uuid);
	if (iter == mAllData.end())
	{
		LLAudioData *adp = new LLAudioData(audio_uuid);
		mAllData[audio_uuid] = adp;
		return adp;
	}
	else
	{
		return iter->second;
	}
}
 void LLAudioEngine::removeAudioData(LLUUID &audio_uuid)
 {
	if(audio_uuid.isNull())
		return;
	data_map::iterator iter = mAllData.find(audio_uuid);
	if(iter != mAllData.end())
 	{
		for (source_map::iterator iter2 = mAllSources.begin(); iter2 != mAllSources.end();)
		{
			LLAudioSource *sourcep = iter2->second;
			if(	sourcep && sourcep->getCurrentData() && sourcep->getCurrentData()->getID() == audio_uuid )
			{
				LLAudioChannel* chan=sourcep->getChannel();
				if(chan)
				{
					LL_DEBUGS("AudioEngine") << "removeAudioData" << LL_ENDL;
					chan->cleanup();
				}
				delete sourcep;
				mAllSources.erase(iter2++);
			}
			else
				++iter2;
		}
		if(iter->second)
		{
			LLAudioBuffer* buf=((LLAudioData*)iter->second)->getBuffer();
			if(buf)
			{
				for (S32 i = 0; i < MAX_BUFFERS; i++)
				{
					if(mBuffers[i] == buf)
						mBuffers[i] = NULL;
				}
				delete buf;
			}
			delete iter->second;
		}
		mAllData.erase(iter);
 	}
 }
void LLAudioEngine::addAudioSource(LLAudioSource *asp)
{
	mAllSources[asp->getID()] = asp;
}
void LLAudioEngine::cleanupAudioSource(LLAudioSource *asp)
{
	source_map::iterator iter;
	iter = mAllSources.find(asp->getID());
	if (iter == mAllSources.end())
	{
		LL_WARNS("AudioEngine") << "Cleaning up unknown audio source!" << LL_ENDL;
		return;
	}
	else
	{
		LL_DEBUGS("AudioEngine") << "Cleaning up audio sources for "<< asp->getID() <<LL_ENDL;
		delete asp;
		mAllSources.erase(iter);
	}
}
bool LLAudioEngine::hasDecodedFile(const LLUUID &uuid)
{
	std::string uuid_str;
	uuid.toString(uuid_str);
	std::string wav_path;
	wav_path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str);
	wav_path += ".dsf";
	if (gDirUtilp->fileExists(wav_path))
	{
		return true;
	}
	else
	{
		return false;
	}
}
void LLAudioEngine::startNextTransfer()
{
	if (getMuted())
	{
		return;
	}
	else if(mCurrentTransferTimer.getElapsedTimeF32() <= .1f)
	{
		return;
	}
	else if(mCurrentTransfer && mCurrentTransfer->isInPreload())
	{
		mCurrentTransfer->updateLoadState();
		return;
	}
	else
	{
		mCurrentTransfer = NULL;
	}
	S32 i;
	LLAudioSource *asp = NULL;
	LLAudioData *adp = NULL;
	LLAudioData *cur_adp = NULL;
	data_map::iterator data_iter;
	F32 max_pri = -1.f;
	for (i = 0; i < MAX_CHANNELS; i++)
	{
		if (!mChannels[i])
		{
			continue;
		}
		asp = mChannels[i]->getSource();
		if (!asp)
		{
			continue;
		}
		if (asp->getPriority() <= max_pri)
		{
			continue;
		}
		if (asp->getPriority() <= max_pri)
		{
			continue;
		}
		adp = asp->getCurrentData();
		if (!adp)
		{
			continue;
		}
		if (adp->isInPreload())
		{
			max_pri = asp->getPriority();
			cur_adp = adp;
		}
	}
	if (!cur_adp)
	{
		max_pri = -1.f;
		for (i = 0; i < MAX_CHANNELS; i++)
		{
			if (!mChannels[i])
			{
				continue;
			}
			LLAudioSource *asp;
			asp = mChannels[i]->getSource();
			if (!asp)
			{
				continue;
			}
			if (asp->getPriority() <= max_pri)
			{
				continue;
			}
			adp = asp->getQueuedData();
			if (!adp)
			{
				continue;
			}
			if (adp->isInPreload())
			{
				max_pri = asp->getPriority();
				cur_adp = adp;
			}
		}
	}
	if (!cur_adp)
	{
		max_pri = -1.f;
		for (i = 0; i < MAX_CHANNELS; i++)
		{
			if (!mChannels[i])
			{
				continue;
			}
			LLAudioSource *asp;
			asp = mChannels[i]->getSource();
			if (!asp)
			{
				continue;
			}
			if (asp->getPriority() <= max_pri)
			{
				continue;
			}
			for (data_iter = asp->mPreloadMap.begin(); data_iter != asp->mPreloadMap.end(); data_iter++)
			{
				LLAudioData *adp = data_iter->second;
				if (!adp)
				{
					continue;
				}
				if (adp->isInPreload())
				{
					max_pri = asp->getPriority();
					cur_adp = adp;
				}
			}
		}
	}
	if (!cur_adp)
	{
		max_pri = -1.f;
		source_map::iterator source_iter;
		for (source_iter = mAllSources.begin(); source_iter != mAllSources.end(); source_iter++)
		{
			asp = source_iter->second;
			if (!asp)
			{
				continue;
			}
			if (asp->getPriority() <= max_pri)
			{
				continue;
			}
			adp = asp->getCurrentData();
			if (adp && adp->isInPreload())
			{
				max_pri = asp->getPriority();
				cur_adp = adp;
				continue;
			}
			adp = asp->getQueuedData();
			if (adp && adp->isInPreload())
			{
				max_pri = asp->getPriority();
				cur_adp = adp;
				continue;
			}
			for (data_iter = asp->mPreloadMap.begin(); data_iter != asp->mPreloadMap.end(); data_iter++)
			{
				LLAudioData *adp = data_iter->second;
				if (!adp)
				{
					continue;
				}
				if (adp->isInPreload())
				{
					max_pri = asp->getPriority();
					cur_adp = adp;
					break;
				}
			}
		}
	}
	if (!cur_adp)
	{
		while(!mPreloadSystemList.empty())
		{
			adp = getAudioData(mPreloadSystemList.front());
			mPreloadSystemList.pop_front();
			if(adp->isInPreload())
			{
				cur_adp = adp;
				break;
			}
		}
	}
	else if(cur_adp)
	{
		std::list<LLUUID>::iterator it = std::find(mPreloadSystemList.begin(),mPreloadSystemList.end(),cur_adp->getID());
		if(it != mPreloadSystemList.end())
			mPreloadSystemList.erase(it);
	}
	if (cur_adp)
	{
		mCurrentTransfer = cur_adp;
		mCurrentTransferTimer.reset();
		mCurrentTransfer->updateLoadState();
	}
	else
	{
	}
}
void LLAudioEngine::assetCallback(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, void *user_data, S32 result_code, LLExtStat ext_status)
{
	if(!gAudiop)
		return;
	LLAudioData *adp = gAudiop->getAudioData(uuid);
	if (result_code)
	{
		LL_INFOS("AudioEngine") << "Boom, error in audio file transfer: " << LLAssetStorage::getErrorString( result_code ) << " (" << result_code << ")" << LL_ENDL;
		if (adp)
		{
			adp->setLoadState(LLAudioData::STATE_LOAD_ERROR);
		}
	}
	else
	{
		if (!adp)
        {
			LL_WARNS("AudioEngine") << "Got asset callback without audio data for " << uuid << LL_ENDL;
        }
		else
		{
			adp->setLoadState(LLAudioData::STATE_LOAD_REQ_DECODE);
			adp->updateLoadState();
		}
	}
}
LLAudioSource::LLAudioSource(const LLUUID& id, const LLUUID& owner_id, const F32 gain, const S32 type, const LLUUID source_id, const bool isTrigger)
:	mID(id),
	mOwnerID(owner_id),
	mPriority(0.f),
	mGain(gain),
	mSourceMuted(false),
	mAmbient(false),
	mLoop(false),
	mSyncMaster(false),
	mSyncSlave(false),
	mQueueSounds(false),
	mPlayedOnce(false),
	mCorrupted(false),
	mType(type),
	mSourceID(source_id),
	mIsTrigger(isTrigger),
	mChannelp(NULL),
	mCurrentDatap(NULL),
	mQueuedDatap(NULL)
{
	mLogID.generate();
}
LLAudioSource::~LLAudioSource()
{
	if(mType != LLAudioEngine::AUDIO_TYPE_UI)
		logSoundStop(mLogID, true);
	if (mChannelp)
	{
		mChannelp->cleanup();
	}
}
void LLAudioSource::setChannel(LLAudioChannel *channelp)
{
	if (channelp == mChannelp)
	{
		return;
	}
	mAgeTimer.reset();
	if (!channelp)
	{
		if(mType != LLAudioEngine::AUDIO_TYPE_UI)
			logSoundStop(mLogID, false);
	}
	mChannelp = channelp;
}
void LLAudioSource::update()
{
	if(mCorrupted)
	{
		return ;
	}
	if(isQueueSounds() && mPlayedOnce && mQueuedDatap && !mChannelp)
	{
		mCurrentDatap = mQueuedDatap;
		mQueuedDatap = NULL;
		mPlayedOnce = false;
		mAgeTimer.reset();
	}
	LLAudioData *adp = getCurrentData();
	if (adp && !adp->getBuffer())
	{
		if(adp->getLoadState() == LLAudioData::STATE_LOAD_ERROR)
		{
			LL_WARNS("AudioEngine") << "Marking LLAudioSource corrupted for " << adp->getID() << LL_ENDL;
			mCorrupted = true ;
		}
		else if(adp->getLoadState() == LLAudioData::STATE_LOAD_READY)
		{
			adp->load();
		}
		else
		{
			adp->updateLoadState();
		}
	}
}
void LLAudioSource::updatePriority()
{
	if (isAmbient())
	{
		setPriority(1.f);
	}
	else if (isMuted())
	{
		setPriority(0.f);
	}
	else
	{
		LLVector3 dist_vec;
		dist_vec.setVec(getPositionGlobal());
		dist_vec -= gAudiop->getListenerPos();
		F32 dist_squared = llmax(1.f, dist_vec.magVecSquared());
		setPriority(mGain / dist_squared);
	}
}
bool LLAudioSource::play(const LLUUID &audio_uuid)
{
	if (audio_uuid.isNull())
	{
		if (getChannel())
		{
			llassert(this == getChannel()->getSource());
			getChannel()->cleanup();
			if (!isMuted())
			{
				mCurrentDatap = NULL;
			}
		}
		return false;
	}
	if(mType != LLAudioEngine::AUDIO_TYPE_UI)
		logSoundPlay(this, audio_uuid);
	mAgeTimer.reset();
	LLAudioData *adp = gAudiop->getAudioData(audio_uuid);
	if (isQueueSounds())
	{
		if(mQueuedDatap)
		{
			return false;
		}
		else if (adp == mCurrentDatap && isLoop())
		{
			return true;
		}
		else if(mCurrentDatap)
		{
			mQueuedDatap = adp;
			return true;
		}
	}
	else if(mCurrentDatap == adp)
	{
		if(getChannel() && getChannel()->isPlaying())
			getChannel()->play();
		return true;
	}
	else
	{
		if(getChannel())
			getChannel()->cleanup();
		mPlayedOnce = false;
	}
	mCurrentDatap = adp;
	return true;
}
bool LLAudioSource::isDone() const
{
	static const F32 MAX_AGE = 60.f;
	static const F32 MAX_UNPLAYED_AGE = 15.f;
	static const F32 MAX_MUTED_AGE = 11.f;
	if(mCorrupted)
	{
		return true;
	}
	else if (isLoop())
	{
		return false;
	}
	else if (hasPendingPreloads())
	{
		return false;
	}
	else if (mQueuedDatap)
	{
		return false;
	}
	else if(mPlayedOnce && (!mChannelp || !mChannelp->isPlaying()))
	{
		return true;
	}
	F32 elapsed = mAgeTimer.getElapsedTimeF32();
	if (!mChannelp)
	{
		LLAudioData* adp = mCurrentDatap;
		if(adp && adp->isInPreload())
			return false;
		return (elapsed > (mSourceMuted ? MAX_MUTED_AGE : MAX_UNPLAYED_AGE));
	}
	else if (mChannelp->isPlaying())
	{
		return elapsed > MAX_AGE;
	}
	else if(!isSyncSlave())
	{
		return elapsed > MAX_UNPLAYED_AGE;
	}
	return false;
}
void LLAudioSource::preload(const LLUUID &audio_id)
{
	if(audio_id.notNull())
	{
		mPreloadMap[audio_id] = gAudiop->getAudioData(audio_id);
	}
}
bool LLAudioSource::hasPendingPreloads() const
{
	data_map::const_iterator iter;
	for (iter = mPreloadMap.begin(); iter != mPreloadMap.end(); iter++)
	{
		LLAudioData *adp = iter->second;
		if (!adp)
		{
			continue;
		}
		if (adp->isInPreload())
		{
			return true;
		}
	}
	return false;
}
LLAudioData * LLAudioSource::getCurrentData()
{
	return mCurrentDatap;
}
LLAudioData * LLAudioSource::getQueuedData()
{
	return mQueuedDatap;
}
LLAudioBuffer * LLAudioSource::getCurrentBuffer()
{
	if (!mCurrentDatap)
	{
		return NULL;
	}
	return mCurrentDatap->getBuffer();
}
LLAudioChannel::LLAudioChannel() :
	mCurrentSourcep(NULL),
	mCurrentBufferp(NULL),
	mLoopedThisFrame(false),
	mSecondaryGain(1.0f)
{
}
LLAudioChannel::~LLAudioChannel()
{
	llassert(mCurrentBufferp == NULL);
	cleanup();
}
void LLAudioChannel::cleanup()
{
	if(mCurrentSourcep)
		mCurrentSourcep->setChannel(NULL);
	mCurrentBufferp = NULL;
	mCurrentSourcep = NULL;
}
void LLAudioChannel::setSource(LLAudioSource *sourcep)
{
	llassert_always(sourcep);
	llassert_always(!mCurrentSourcep);
	mCurrentSourcep = sourcep;
	mCurrentSourcep->setChannel(this);
	updateBuffer();
	update3DPosition();
}
bool LLAudioChannel::updateBuffer()
{
	if (!mCurrentSourcep)
	{
		return false;
	}
	if(gAudiop)
	{
		setSecondaryGain(gAudiop->getSecondaryGain(mCurrentSourcep->getType()));
	}
	LLAudioBuffer *bufferp = mCurrentSourcep->getCurrentBuffer();
	if (bufferp)
	{
		bufferp->mLastUseTimer.reset();
		bufferp->mInUse = true;
	}
	return bufferp != mCurrentBufferp && !!(mCurrentBufferp = bufferp);
}
LLAudioData::LLAudioData(const LLUUID &uuid) :
	mID(uuid),
	mBufferp(NULL),
	mLoadState(STATE_LOAD_ERROR)
{
	if (uuid.isNull())
	{
		return;
	}
	if(gAudiop->hasDecodedFile(getID()))
		mLoadState = STATE_LOAD_READY;
	else if(gAssetStorage && gAssetStorage->hasLocalAsset(getID(), LLAssetType::AT_SOUND))
		mLoadState = STATE_LOAD_REQ_DECODE;
	else
		mLoadState = STATE_LOAD_REQ_FETCH;
}
void LLAudioData::updateLoadState()
{
	if(mLoadState == STATE_LOAD_REQ_DECODE && gAudioDecodeMgrp)
	{
		if(	gAudioDecodeMgrp->addDecodeRequest(getID()) )
		{
			setLoadState(STATE_LOAD_DECODING);
			LL_DEBUGS("AudioEngine") << "Decoding asset data for: " << getID() << LL_ENDL;
		}
		else
		{
			setLoadState(STATE_LOAD_ERROR);
		}
	}
	else if(mLoadState == STATE_LOAD_REQ_FETCH && gAssetStorage && gAssetStorage->isUpstreamOK())
	{
		LL_DEBUGS("AudioEngine") << "Fetching asset data for: " << getID() << LL_ENDL;
		setLoadState(STATE_LOAD_FETCHING);
		gAssetStorage->getAssetData(getID(), LLAssetType::AT_SOUND, LLAudioEngine::assetCallback, NULL);
	}
}
bool LLAudioData::load()
{
	if (mBufferp)
	{
		LL_INFOS("AudioEngine") << "Already have a buffer for this sound, don't bother loading!" << LL_ENDL;
		return true;
	}
	mBufferp = gAudiop->getFreeBuffer();
	if (!mBufferp)
	{
		LL_DEBUGS("AudioEngine") << "Not able to allocate a new audio buffer, aborting." << LL_ENDL;
		return false;
	}
	std::string uuid_str;
	std::string wav_path;
	mID.toString(uuid_str);
	wav_path= gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str) + ".dsf";
	if (!mBufferp->loadWAV(wav_path))
	{
		gAudiop->cleanupBuffer(mBufferp);
		mBufferp = NULL;
		return false;
	}
	mBufferp->mAudioDatap = this;
	return true;
}
std::map<LLUUID, LLSoundHistoryItem> gSoundHistory;
void logSoundPlay(LLAudioSource* audio_source, LLUUID const& assetid)
{
	LLSoundHistoryItem item;
	item.mID = audio_source->getLogID();
	item.mAudioSource = audio_source;
	item.mPosition = audio_source->getPositionGlobal();
	item.mType = audio_source->getType();
	item.mAssetID = assetid;
	item.mOwnerID = audio_source->getOwnerID();
	item.mSourceID = audio_source->getSourceID();
	item.mPlaying = true;
	item.mTimeStarted = LLTimer::getElapsedSeconds();
	item.mTimeStopped = F64_MAX;
	item.mIsTrigger = audio_source->getIsTrigger();
	item.mIsLooped = audio_source->isLoop();
	item.mReviewed = false;
	item.mReviewedCollision = false;
	gSoundHistory[item.mID] = item;
}
void logSoundStop(LLUUID const& id, bool destructed)
{
	std::map<LLUUID, LLSoundHistoryItem>::iterator iter = gSoundHistory.find(id);
	if(iter != gSoundHistory.end() && iter->second.mAudioSource)
	{
		iter->second.mPlaying = false;
		iter->second.mTimeStopped = LLTimer::getElapsedSeconds();
		if (destructed)
			iter->second.mAudioSource = NULL;
		pruneSoundLog();
	}
}
void pruneSoundLog()
{
	if(++gSoundHistoryPruneCounter >= 64)
	{
		gSoundHistoryPruneCounter = 0;
		while(gSoundHistory.size() > 256)
		{
			std::map<LLUUID, LLSoundHistoryItem>::iterator iter = gSoundHistory.begin();
			std::map<LLUUID, LLSoundHistoryItem>::iterator end = gSoundHistory.end();
			U64 lowest_time = (*iter).second.mTimeStopped;
			LLUUID lowest_id = (*iter).first;
			for( ; iter != end; ++iter)
			{
				if((*iter).second.mTimeStopped < lowest_time)
				{
					lowest_time = (*iter).second.mTimeStopped;
					lowest_id = (*iter).first;
				}
			}
			gSoundHistory.erase(lowest_id);
		}
	}
}
