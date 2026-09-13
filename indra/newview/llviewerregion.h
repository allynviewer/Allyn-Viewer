/** 
 * @file llviewerregion.h
 * @brief Description of the LLViewerRegion class.
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
#ifndef LL_LLVIEWERREGION_H
#define LL_LLVIEWERREGION_H
#include <string>
#include <boost/signals2.hpp>
#include "llwind.h"
#include "llcloud.h"
#include "llstat.h"
#include "v3dmath.h"
#include "llstring.h"
#include "llregionflags.h"
#include "lluuid.h"
#include "llweb.h"
#include "llcapabilityprovider.h"
#include "llmatrix4a.h"
#include "llhttpclient.h"
const U32	MAX_OBJECT_CACHE_ENTRIES = 50000;
const U32 REGION_HANDSHAKE_SUPPORTS_SELF_APPEARANCE = 1U << 2;
class LLEventPoll;
class LLVLComposition;
class LLViewerObject;
class LLMessageSystem;
class LLNetMap;
class LLViewerParcelOverlay;
class LLSurface;
class LLVOCache;
class LLVOCacheEntry;
class LLSpatialPartition;
class LLEventPump;
class LLCapabilityListener;
class LLDataPacker;
class LLDataPackerBinaryBuffer;
class LLHost;
class LLBBox;
class LLSpatialGroup;
class LLViewerTexture;
class LLViewerRegionImpl;
class LLViewerOctreeGroup;
class LLViewerRegion: public LLCapabilityProvider
{
public:
	typedef enum
	{
		PARTITION_HUD=0,
		PARTITION_TERRAIN,
		PARTITION_VOIDWATER,
		PARTITION_WATER,
		PARTITION_TREE,
		PARTITION_PARTICLE,
#if ENABLE_CLASSIC_CLOUDS
		PARTITION_CLOUD,
#endif
		PARTITION_GRASS,
		PARTITION_VOLUME,
		PARTITION_BRIDGE,
		PARTITION_ATTACHMENT,
		PARTITION_HUD_PARTICLE,
		PARTITION_NONE,
		NUM_PARTITIONS
	} eObjectPartitions;
	typedef boost::signals2::signal<void(const LLUUID& region_id)> caps_received_signal_t;
	LLViewerRegion(const U64 &handle,
				   const LLHost &host,
				   const U32 surface_grid_width,
				   const U32 patch_grid_width,
				   const F32 region_width_meters);
	~LLViewerRegion();
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
	void loadObjectCache();
	void saveObjectCache();
	void sendMessage();
	void sendReliableMessage();
	void setOriginGlobal(const LLVector3d &origin);
	void updateRenderMatrix();
	void setAllowDamage(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_DAMAGE, b); }
	void setAllowLandmark(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_LANDMARK, b); }
	void setAllowSetHome(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_SET_HOME, b); }
	void setResetHomeOnTeleport(BOOL b) { setRegionFlag(REGION_FLAGS_RESET_HOME_ON_TELEPORT, b); }
	void setSunFixed(BOOL b) { setRegionFlag(REGION_FLAGS_SUN_FIXED, b); }
	void setAllowDirectTeleport(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT, b); }
	inline BOOL getAllowDamage()			const;
	inline BOOL getAllowLandmark()			const;
	inline BOOL getAllowSetHome()			const;
	inline BOOL getResetHomeOnTeleport()	const;
	inline BOOL getSunFixed()				const;
	inline BOOL getBlockFly()				const;
	inline BOOL getAllowDirectTeleport()	const;
	inline BOOL isPrelude()					const;
	inline BOOL getAllowTerraform() 		const;
	inline BOOL getRestrictPushObject()		const;
	inline BOOL getReleaseNotesRequested()		const;
	bool isAlive() const;
	typedef std::vector<LLPointer<LLViewerTexture> > tex_matrix_t;
	const tex_matrix_t& getWorldMapTiles() const;
	void setWaterHeight(F32 water_level);
	F32 getWaterHeight() const;
	BOOL isVoiceEnabled() const;
	void setBillableFactor(F32 billable_factor) { mBillableFactor = billable_factor; }
	F32 getBillableFactor() 		const 	{ return mBillableFactor; }
	U32 getMaxTasks() const { return mMaxTasks; }
	void setMaxTasks(U32 max_tasks) { mMaxTasks = max_tasks; }
	S32 renderPropertyLines();
	void dirtyHeights();
	LLViewerParcelOverlay *getParcelOverlay() const
			{ return mParcelOverlay; }
	inline void setRegionFlag(U64 flag, BOOL on);
	inline BOOL getRegionFlag(U64 flag) const;
	void setRegionFlags(U64 flags);
	U64 getRegionFlags() const					{ return mRegionFlags; }
	inline void setRegionProtocol(U64 protocol, BOOL on);
	BOOL getRegionProtocol(U64 protocol) const;
	void setRegionProtocols(U64 protocols)			{ mRegionProtocols = protocols; }
	U64 getRegionProtocols() const					{ return mRegionProtocols; }
	void setTimeDilation(F32 time_dilation);
	F32  getTimeDilation() const				{ return mTimeDilation; }
	const LLVector3d &getOriginGlobal() const;
	LLVector3 getOriginAgent() const;
	const LLVector3d &getCenterGlobal() const;
	LLVector3 getCenterAgent() const;
	void setRegionNameAndZone(const std::string& name_and_zone);
	const std::string& getName() const				{ return mName; }
	const std::string& getZoning() const			{ return mZoning; }
	void setOwner(const LLUUID& owner_id);
	const LLUUID& getOwner() const;
	void setIsEstateManager(BOOL b) { mIsEstateManager = b; }
	BOOL isEstateManager() const { return mIsEstateManager; }
	BOOL canManageEstate() const;
	void setSimAccess(U8 sim_access)			{ mSimAccess = sim_access; }
	U8 getSimAccess() const						{ return mSimAccess; }
	std::string const& getSimAccessString();
	S32 getSimClassID()                    const { return mClassID; }
	S32 getSimCPURatio()                   const { return mCPURatio; }
	const std::string& getSimColoName()    const { return mColoName; }
	const std::string& getSimProductSKU()  const { return mProductSKU; }
	std::string getLocalizedSimProductName() const;
	static std::string regionFlagsToString(U64 flags);
	static std::string accessToString(U8 sim_access);
	static std::string accessToShortString(U8 sim_access);
	static U8          shortStringToAccess(const std::string &sim_access);
	static std::string getAccessIcon(U8 sim_access);
	static void processRegionInfo(LLMessageSystem* msg, void**);
	void setCacheID(const LLUUID& id);
	F32	getWidth() const						{ return mWidth; }
	BOOL idleUpdate(F32 max_update_time);
	void forceUpdate();
	void connectNeighbor(LLViewerRegion *neighborp, U32 direction);
	void updateNetStats();
	U32	getPacketsLost() const;
	S32 getHttpResponderID() const;
	void setSeedCapability(const std::string& url);
	void failedSeedCapability();
	S32 getNumSeedCapRetries();
	void setCapability(const std::string& name, const std::string& url);
	void setCapabilityDebug(const std::string& name, const std::string& url);
	bool isCapabilityAvailable(const std::string& name) const;
    virtual std::string getCapability(const std::string& name) const;
	bool capabilitiesReceived() const;
	void setCapabilitiesReceived(bool received);
	boost::signals2::connection setCapabilitiesReceivedCallback(const caps_received_signal_t::slot_type& cb);
	static bool isSpecialCapabilityName(const std::string &name);
	void logActiveCapabilities() const;
    LLEventPump& getCapAPI() const;
	const LLHost& getHost() const;
	const U64 		&getHandle() const 			{ return mHandle; }
	LLSurface		&getLand() const;
	const LLUUID& getRegionID() const;
	void setRegionID(const LLUUID& region_id);
	BOOL pointInRegionGlobal(const LLVector3d &point_global) const;
	LLVector3	getPosRegionFromGlobal(const LLVector3d &point_global) const;
	LLVector3	getPosRegionFromAgent(const LLVector3 &agent_pos) const;
	LLVector3	getPosAgentFromRegion(const LLVector3 &region_pos) const;
	LLVector3d	getPosGlobalFromRegion(const LLVector3 &offset) const;
	LLVLComposition *getComposition() const;
	F32 getCompositionXY(const S32 x, const S32 y) const;
	BOOL isOwnedSelf(const LLVector3& pos);
	BOOL isOwnedGroup(const LLVector3& pos);
	void updateCoarseLocations(LLMessageSystem* msg);
	F32 getLandHeightRegion(const LLVector3& region_pos);
	U8 getCentralBakeVersion() { return mCentralBakeVersion; }
	void getInfo(LLSD& info);
	bool meshRezEnabled() const;
	bool meshUploadEnabled() const;
	bool bakesOnMeshEnabled() const;
	void requestSimulatorFeatures();
	void setSimulatorFeaturesReceived(bool);
	bool simulatorFeaturesReceived() const;
	boost::signals2::connection setSimulatorFeaturesReceivedCallback(const caps_received_signal_t::slot_type& cb);
	void getSimulatorFeatures(LLSD& info) const;
	void setSimulatorFeatures(const LLSD& info);
	bool dynamicPathfindingEnabled() const;
	bool avatarHoverHeightEnabled() const;
	typedef enum
	{
		CACHE_MISS_TYPE_FULL = 0,
		CACHE_MISS_TYPE_CRC,
		CACHE_MISS_TYPE_NONE
	} eCacheMissType;
	typedef enum
	{
		CACHE_UPDATE_DUPE = 0,
		CACHE_UPDATE_CHANGED,
		CACHE_UPDATE_ADDED,
		CACHE_UPDATE_REPLACED
	} eCacheUpdateResult;
	eCacheUpdateResult cacheFullUpdate(LLViewerObject* objectp, LLDataPackerBinaryBuffer &dp);
	LLDataPacker *getDP(U32 local_id, U32 crc, U8 &cache_miss_type);
	void requestCacheMisses();
	void addCacheMissFull(const U32 local_id);
	void clearCachedVisibleObjects();
	void dumpCache();
	void unpackRegionHandshake();
	void calculateCenterGlobal();
	void calculateCameraDistance();
	friend std::ostream& operator<<(std::ostream &s, const LLViewerRegion &region);
    virtual std::string getDescription() const override;
    std::string getViewerAssetUrl() const { return mViewerAssetUrl; }
	LLSpatialPartition* getSpatialPartition(U32 type);
	bool objectIsReturnable(const LLVector3& pos, const std::vector<LLBBox>& boxes) const;
	bool childrenObjectReturnable( const std::vector<LLBBox>& boxes ) const;
	bool objectsCrossParcel(const std::vector<LLBBox>& boxes) const;
	void getNeighboringRegions( std::vector<LLViewerRegion*>& uniqueRegions );
	void getNeighboringRegionsStatus( std::vector<S32>& regions );
	const LLViewerRegionImpl * getRegionImpl() const { return mImpl; }
	LLViewerRegionImpl * getRegionImplNC() { return mImpl; }
	void setGamingData(const LLSD& info);
	const U32 getGamingFlags() const { return mGamingFlags; }
	bool materialsCapThrottled() const { return !mMaterialsCapThrottleTimer.hasExpired(); }
	void resetMaterialsCapThrottle();
	U32 getMaxMaterialsPerTransaction() const;
	void removeFromCreatedList(U32 local_id);
	void addToCreatedList(U32 local_id);
public:
	struct CompareDistance
	{
		bool operator()(const LLViewerRegion* const& lhs, const LLViewerRegion* const& rhs)
		{
			return lhs->mCameraDistanceSquared < rhs->mCameraDistanceSquared;
		}
	};
	void showReleaseNotes();
	void reInitPartitions();
protected:
	void disconnectAllNeighbors();
	void initStats();
	void initPartitions();
public:
	LLWind  mWind;
#if ENABLE_CLASSIC_CLOUDS
	LLCloudLayer mCloudLayer;
#endif
	LLViewerParcelOverlay	*mParcelOverlay;
	LLStat	mBitStat;
	LLStat	mPacketsStat;
	LLStat	mPacketsLostStat;
	LL_ALIGN_16(LLMatrix4a mRenderMatrix);
	std::vector<U32> mMapAvatars;
	uuid_vec_t mMapAvatarIDs;
	LLFrameTimer &	getRenderInfoRequestTimer()			{ return mRenderInfoRequestTimer;		};
private:
	LLViewerRegionImpl * mImpl;
	F32			mWidth;
	U64			mHandle;
	F32			mTimeDilation;
	std::string mName;
	std::string mZoning;
	BOOL mIsEstateManager;
	U32		mPacketsIn;
	U32Bits	mBitsIn,
			mLastBitsIn;
	U32		mLastPacketsIn;
	U32		mPacketsOut;
	U32		mLastPacketsOut;
	S32		mPacketsLost;
	S32		mLastPacketsLost;
	U32Milliseconds		mPingDelay;
	F32		mDeltaTime;
	U64		mRegionFlags;
	U64		mRegionProtocols;
	U8		mSimAccess;
	U8		mLastSimAccess;
	std::string mSimAccessString;
	F32 	mBillableFactor;
	U32		mMaxTasks;
	F32		mCameraDistanceSquared;
	U8		mCentralBakeVersion;
	S32 mClassID;
	S32 mCPURatio;
	std::string mColoName;
	std::string mProductSKU;
	std::string mProductName;
	std::string mViewerAssetUrl;
	BOOL									mCacheLoaded;
	BOOL                                    mCacheDirty;
	std::vector<U32>						mCacheMissFull;
	std::vector<U32>						mCacheMissCRC;
	mutable tex_matrix_t mWorldMapTiles;
	bool	mAlive;
	bool	mCapabilitiesReceived;
	bool	mSimulatorFeaturesReceived;
	caps_received_signal_t mCapabilitiesReceivedSignal;
	caps_received_signal_t mSimulatorFeaturesReceivedSignal;
	BOOL mReleaseNotesRequested;
	LLSD mSimulatorFeatures;
	U32 mGamingFlags;
	LLFrameTimer mMaterialsCapThrottleTimer;
	LLFrameTimer	mRenderInfoRequestTimer;
};
inline BOOL LLViewerRegion::getRegionProtocol(U64 protocol) const
{
	return ((mRegionProtocols & protocol) != 0);
}
inline void LLViewerRegion::setRegionProtocol(U64 protocol, BOOL on)
{
	if (on)
	{
		mRegionProtocols |= protocol;
	}
	else
	{
		mRegionProtocols &= ~protocol;
	}
}
inline BOOL LLViewerRegion::getRegionFlag(U64 flag) const
{
	return ((mRegionFlags & flag) != 0);
}
inline void LLViewerRegion::setRegionFlag(U64 flag, BOOL on)
{
	if (on)
	{
		mRegionFlags |= flag;
	}
	else
	{
		mRegionFlags &= ~flag;
	}
}
inline BOOL LLViewerRegion::getAllowDamage() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_DAMAGE) !=0);
}
inline BOOL LLViewerRegion::getAllowLandmark() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_LANDMARK) !=0);
}
inline BOOL LLViewerRegion::getAllowSetHome() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_SET_HOME) != 0);
}
inline BOOL LLViewerRegion::getResetHomeOnTeleport() const
{
	return ((mRegionFlags & REGION_FLAGS_RESET_HOME_ON_TELEPORT) !=0);
}
inline BOOL LLViewerRegion::getSunFixed() const
{
	return ((mRegionFlags & REGION_FLAGS_SUN_FIXED) !=0);
}
inline BOOL LLViewerRegion::getBlockFly() const
{
	return ((mRegionFlags & REGION_FLAGS_BLOCK_FLY) !=0);
}
inline BOOL LLViewerRegion::getAllowDirectTeleport() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_DIRECT_TELEPORT) !=0);
}
inline BOOL LLViewerRegion::isPrelude() const
{
	return is_prelude( mRegionFlags );
}
inline BOOL LLViewerRegion::getAllowTerraform() const
{
	return ((mRegionFlags & REGION_FLAGS_BLOCK_TERRAFORM) == 0);
}
inline BOOL LLViewerRegion::getRestrictPushObject() const
{
	return ((mRegionFlags & REGION_FLAGS_RESTRICT_PUSHOBJECT) != 0);
}
inline BOOL LLViewerRegion::getReleaseNotesRequested() const
{
	return mReleaseNotesRequested;
}
#endif
