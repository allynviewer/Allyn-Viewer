/** 
 * @file audioengine.h
 * @brief Definition of LLAudioEngine base class abstracting the audio support
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
#ifndef LL_AUDIOENGINE_H
#define LL_AUDIOENGINE_H
#include <list>
#include <map>
#include "v3math.h"
#include "v3dmath.h"
#include "lltimer.h"
#include "lluuid.h"
#include "llframetimer.h"
#include "llassettype.h"
#include "llextendedstatus.h"
#include "lllistener.h"
const F32 LL_WIND_UPDATE_INTERVAL = 0.1f;
const F32 LL_WIND_UNDERWATER_CENTER_FREQ = 20.f;
const F32 ATTACHED_OBJECT_TIMEOUT = 5.0f;
const F32 DEFAULT_MIN_DISTANCE = 2.0f;
#define MAX_CHANNELS 30
#define MAX_BUFFERS 40
class LLVFS;
class LLAudioSource;
class LLAudioData;
class LLAudioChannel;
class LLAudioChannelOpenAL;
class LLAudioBuffer;
class LLStreamingAudioInterface;
class LLAudioEngine
{
	friend class LLAudioChannelOpenAL;
public:
	enum LLAudioType
	{
		AUDIO_TYPE_NONE    = 0,
		AUDIO_TYPE_SFX     = 1,
		AUDIO_TYPE_UI      = 2,
		AUDIO_TYPE_AMBIENT = 3,
		AUDIO_TYPE_COUNT   = 4
	};
	enum LLAudioPlayState
	{
		AUDIO_STOPPED = 0,
		AUDIO_PLAYING = 1,
		AUDIO_PAUSED = 2
	};
	LLAudioEngine();
	virtual ~LLAudioEngine();
	virtual bool init(const S32 num_channels, void *userdata);
	virtual std::string getDriverName(bool verbose) = 0;
	virtual void shutdown();
	virtual void setListener(LLVector3 pos,LLVector3 vel,LLVector3 up,LLVector3 at);
	virtual void updateWind(LLVector3 direction, F32 camera_height_above_water) = 0;
	virtual void idle(F32 max_decode_time = 0.f);
	virtual void updateChannels();
	virtual bool isWindEnabled();
	virtual void enableWind(bool state_b);
	void setMuted(bool muted);
	bool getMuted() const { return mMuted; }
#ifdef USE_PLUGIN_MEDIA
	LLPluginClassMedia* initializeMedia(const std::string& media_type);
#endif
	F32 getMasterGain();
	void setMasterGain(F32 gain);
	F32 getSecondaryGain(S32 type);
	void setSecondaryGain(S32 type, F32 gain);
	F32 getInternetStreamGain();
	virtual void setDopplerFactor(F32 factor);
	virtual F32 getDopplerFactor();
	virtual void setRolloffFactor(F32 factor);
	virtual F32 getRolloffFactor();
	virtual void setMaxWindGain(F32 gain);
	void triggerSound(const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain,
					  const S32 type = LLAudioEngine::AUDIO_TYPE_NONE,
					  const LLVector3d &pos_global = LLVector3d::zero,
					  const LLUUID source_object = LLUUID::null);
	bool preloadSound(const LLUUID &id);
	void addAudioSource(LLAudioSource *asp);
	void cleanupAudioSource(LLAudioSource *asp);
	LLAudioSource *findAudioSource(const LLUUID &source_id);
	LLAudioData *getAudioData(const LLUUID &audio_uuid);
	void removeAudioData(LLUUID &audio_uuid);
	LLStreamingAudioInterface *getStreamingAudioImpl();
	void setStreamingAudioImpl(LLStreamingAudioInterface *impl);
	void startInternetStream(const std::string& url);
	void stopInternetStream();
	void pauseInternetStream(int pause);
	void updateInternetStream();
	LLAudioPlayState isInternetStreamPlaying();
	void setInternetStreamGain(F32 vol);
	std::string getInternetStreamURL();
	virtual LLVector3 getListenerPos();
	LLAudioBuffer *getFreeBuffer();
	LLAudioChannel *getFreeChannel(const F32 priority);
	void cleanupBuffer(LLAudioBuffer *bufferp);
	bool hasDecodedFile(const LLUUID &uuid);
	void setAllowLargeSounds(bool allow) { mAllowLargeSounds = allow ;}
	bool getAllowLargeSounds() const {return mAllowLargeSounds;}
	void startNextTransfer();
	static void assetCallback(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, void *user_data, S32 result_code, LLExtStat ext_status);
	friend class LLPipeline;
	void checkStates();
public:
	F32 mMaxWindGain;
protected:
	virtual LLAudioBuffer *createBuffer() = 0;
	virtual LLAudioChannel *createChannel() = 0;
	virtual bool initWind() = 0;
	virtual void cleanupWind() = 0;
	virtual void setInternalGain(F32 gain) = 0;
	void commitDeferredChanges();
	virtual void allocateListener() = 0;
	virtual void setListenerPos(LLVector3 vec);
	virtual void setListenerVelocity(LLVector3 vec);
	virtual void orientListener(LLVector3 up, LLVector3 at);
	virtual void translateListener(LLVector3 vec);
	F64 mapWindVecToGain(LLVector3 wind_vec);
	F64 mapWindVecToPitch(LLVector3 wind_vec);
	F64 mapWindVecToPan(LLVector3 wind_vec);
protected:
	LLListener *mListenerp;
	bool mMuted;
	void* mUserData;
	S32 mLastStatus;
	S32 mNumChannels;
	bool mEnableWind;
	LLAudioData* mCurrentTransfer;
	LLFrameTimer mCurrentTransferTimer;
public:
	typedef std::map<LLUUID, LLAudioSource *> source_map;
protected:
	typedef std::map<LLUUID, LLAudioData *> data_map;
public:
	source_map mAllSources;
protected:
	data_map mAllData;
	std::list<LLUUID> mPreloadSystemList;
	LLAudioChannel *mChannels[MAX_CHANNELS];
	LLAudioBuffer *mBuffers[MAX_BUFFERS];
	F32 mMasterGain;
	F32 mInternalGain;
	F32 mSecondaryGain[AUDIO_TYPE_COUNT];
	F32 mNextWindUpdate;
	LLFrameTimer mWindUpdateTimer;
private:
	void setDefaults();
	LLStreamingAudioInterface *mStreamingAudioImpl;
	bool mAllowLargeSounds;
};
class LLAudioSource
{
public:
	LLAudioSource(const LLUUID &id, const LLUUID& owner_id, const F32 gain, const S32 type = LLAudioEngine::AUDIO_TYPE_NONE, const LLUUID source_id = LLUUID::null, const bool isTrigger = true);
	virtual ~LLAudioSource();
	virtual void update();
	void updatePriority();
	void preload(const LLUUID &audio_id);
	void setAmbient(const bool ambient)						{ mAmbient = ambient; }
	bool isAmbient() const									{ return mAmbient; }
	void setLoop(const bool loop)							{ mLoop = loop; }
	bool isLoop() const										{ return mLoop; }
	void setSyncMaster(const bool master)					{ mSyncMaster = master; }
	bool isSyncMaster() const								{ return mSyncMaster; }
	void setSyncSlave(const bool slave)						{ mSyncSlave = slave; }
	bool isSyncSlave() const								{ return mSyncSlave; }
	void setQueueSounds(const bool queue)					{ mQueueSounds = queue; }
	bool isQueueSounds() const								{ return mQueueSounds; }
	void setPlayedOnce(const bool played_once)				{ mPlayedOnce = played_once; }
	void setType(S32 type)                                  { mType = type; }
	S32 getType(void) const                                 { return mType; }
	LLUUID const& getOwnerID(void) const					{ return mOwnerID; }
	LLUUID const& getSourceID(void) const					{ return mSourceID; }
	bool getIsTrigger(void) const							{ return mIsTrigger; }
	void setPositionGlobal(const LLVector3d &position_global)		{ mPositionGlobal = position_global; }
	LLVector3d getPositionGlobal() const							{ return mPositionGlobal; }
	LLVector3 getVelocity()	const									{ return mVelocity; }
	F32 getPriority() const											{ return mPriority; }
	void setPriority(F32 priority)									{ mPriority = priority; }
	F32 getGain() const												{ return mGain; }
	virtual void setGain(const F32 gain)							{ mGain = llclamp(gain, 0.f, 1.f); }
	const LLUUID &getID() const		{ return mID; }
	const LLUUID &getLogID() const { return mLogID; }
	bool isDone() const;
	bool isMuted() const { return mSourceMuted; }
	LLAudioData *getCurrentData();
	LLAudioData *getQueuedData();
	LLAudioBuffer *getCurrentBuffer();
	bool setupChannel();
	bool play(const LLUUID &audio_id);
	bool hasPendingPreloads() const;
	friend class LLAudioEngine;
	friend class LLAudioChannel;
protected:
	void setChannel(LLAudioChannel *channelp);
public:
	LLAudioChannel *getChannel() const						{ return mChannelp; }
protected:
	LLUUID			mID;
	LLUUID			mOwnerID;
public:
	const LLUUID &getOwnerID()		{ return mOwnerID; }
protected:
	F32				mPriority;
	F32				mGain;
	bool			mSourceMuted;
	bool			mAmbient;
	bool			mLoop;
	bool			mSyncMaster;
	bool			mSyncSlave;
	bool			mQueueSounds;
	bool			mPlayedOnce;
	bool            mCorrupted;
	S32             mType;
	LLVector3d		mPositionGlobal;
	LLVector3		mVelocity;
	LLUUID			mLogID;
	LLUUID			mSourceID;
	bool			mIsTrigger;
	LLAudioChannel	*mChannelp;
	LLAudioData		*mCurrentDatap;
	LLAudioData		*mQueuedDatap;
	typedef std::map<LLUUID, LLAudioData *> data_map;
	data_map mPreloadMap;
	LLFrameTimer mAgeTimer;
};
class LLAudioData
{
public:
	LLAudioData(const LLUUID &uuid);
	bool load();
	LLUUID getID() const				{ return mID; }
	LLAudioBuffer *getBuffer() const	{ return mBufferp; }
	enum ELoadState
	{
		STATE_LOAD_ERROR,
		STATE_LOAD_REQ_FETCH,
		STATE_LOAD_FETCHING,
		STATE_LOAD_REQ_DECODE,
		STATE_LOAD_DECODING,
		STATE_LOAD_READY
	};
	ELoadState	getLoadState() const			{ return mLoadState; }
	ELoadState	setLoadState(ELoadState state)	{ return mLoadState = state; }
	bool		isInPreload() const				{ return mLoadState > STATE_LOAD_ERROR && mLoadState < STATE_LOAD_READY; }
	void updateLoadState();
	friend class LLAudioEngine;
protected:
	LLUUID mID;
	LLAudioBuffer *mBufferp;
	ELoadState mLoadState;
};
class LLAudioChannel
{
public:
	LLAudioChannel();
	virtual ~LLAudioChannel();
	virtual void setSource(LLAudioSource *sourcep);
	LLAudioSource *getSource() const			{ return mCurrentSourcep; }
	void setSecondaryGain(F32 gain)             { mSecondaryGain = gain; }
	F32 getSecondaryGain()                      { return mSecondaryGain; }
	friend class LLAudioEngine;
	friend class LLAudioSource;
protected:
	virtual void play() = 0;
	virtual void playSynced(LLAudioChannel *channelp) = 0;
	virtual void cleanup();
public:
	virtual bool isPlaying() = 0;
	bool isFree() const							{ return mCurrentSourcep==NULL; }
protected:
	virtual bool updateBuffer();
	virtual void update3DPosition() = 0;
	virtual void updateLoop() = 0;
protected:
	LLAudioSource	*mCurrentSourcep;
	LLAudioBuffer	*mCurrentBufferp;
	bool			mLoopedThisFrame;
	F32             mSecondaryGain;
};
class LLAudioBuffer
{
public:
	LLAudioBuffer() : mInUse(true), mAudioDatap(NULL) { mLastUseTimer.reset(); }
	virtual ~LLAudioBuffer() {};
	virtual bool loadWAV(const std::string& filename) = 0;
	virtual U32 getLength() = 0;
	friend class LLAudioEngine;
	friend class LLAudioChannel;
	friend class LLAudioData;
protected:
	bool mInUse;
	LLAudioData *mAudioDatap;
	LLFrameTimer mLastUseTimer;
};
extern LLAudioEngine* gAudiop;
struct LLSoundHistoryItem
{
	LLUUID mID;
	LLVector3d mPosition;
	S32 mType;
	bool mPlaying;
	LLUUID mAssetID;
	LLUUID mOwnerID;
	LLUUID mSourceID;
	bool mIsTrigger;
	bool mIsLooped;
	F64 mTimeStarted;
	F64 mTimeStopped;
	bool mReviewed;
	bool mReviewedCollision;
	LLAudioSource* mAudioSource;
	LLSoundHistoryItem() : mType(0), mPlaying(false), mIsTrigger(false),
			mIsLooped(false), mReviewed(false), mReviewedCollision(false),
			mTimeStarted(0), mTimeStopped(0), mAudioSource(0) {}
	bool isPlaying(void) const
	{
		return mPlaying && mAudioSource && mAudioSource->getChannel()
			 ;
	}
};
extern std::map<LLUUID, LLSoundHistoryItem> gSoundHistory;
extern void logSoundPlay(LLAudioSource* audio_source, LLUUID const& assetid);
extern void logSoundStop(LLUUID const& id, bool destructed);
extern void pruneSoundLog();
extern int gSoundHistoryPruneCounter;
#endif
