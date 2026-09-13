/** 
 * @file llworldmap.h
 * @brief Underlying data storage for the map of the entire world.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
#ifndef LL_LLWORLDMAP_H
#define LL_LLWORLDMAP_H
#include <map>
#include <string>
#include <vector>
#include "v3math.h"
#include "v3dmath.h"
#include "llframetimer.h"
#include "llmapimagetype.h"
#include "llworldmipmap.h"
#include "lluuid.h"
#include "llmemory.h"
#include "llviewerregion.h"
#include "llviewertexture.h"
#include "lleventinfo.h"
#include "v3color.h"
class LLMessageSystem;
class LLItemInfo
{
public:
	LLItemInfo(F32 global_x, F32 global_y, const std::string& name, LLUUID id);
	void setTooltip(const std::string& tooltip) { mToolTip = tooltip; }
	void setElevation(F64 z) { mPosGlobal.mdV[VZ] = z; }
	void setCount(S32 count) { mCount = count; }
	const LLVector3d& getGlobalPosition() const { return mPosGlobal; }
	const std::string& getName() const { return mName; }
	const std::string& getToolTip() const { return mToolTip; }
	const LLUUID& getUUID() const { return mID; }
	S32 getCount() const { return mCount; }
	U64 getRegionHandle() const { return to_region_handle(mPosGlobal); }
	bool isName(const std::string& name) const { return (mName == name); }
private:
	std::string mName;
	std::string mToolTip;
	LLVector3d	mPosGlobal;
	LLUUID		mID;
	S32			mCount;
};
enum sim_layer_type
{
	SIM_LAYER_BEGIN,
	SIM_LAYER_COMPOSITE=0,
	SIM_LAYER_OVERLAY,
	SIM_LAYER_COUNT
};
inline U32 layerToFlags(sim_layer_type layer)
{
	static U32 flags[SIM_LAYER_COUNT] = {0,2};
	return flags[layer];
}
inline U32 flagsToLayer(U32 flags)
{
	if((flags & 0xFFFF) == 0)
		return SIM_LAYER_COMPOSITE;
	else if((flags & 0xFFFF) == 2)
		return SIM_LAYER_OVERLAY;
	else
		return U32_MAX;
}
class LLSimInfo
{
public:
	LLSimInfo(U64 handle);
	LLVector3d getGlobalPos(const LLVector3& local_pos) const;
	LLVector3d getGlobalOrigin() const;
	LLVector3 getLocalPos(LLVector3d global_pos) const;
	void clearImage();
	void dropImagePriority(sim_layer_type layer = SIM_LAYER_COUNT);
	void updateAgentCount(F64 time);
	void setName(std::string& name) { mName = name; }
	void setSize(U16 sizeX, U16 sizeY) { mSizeX = sizeX; mSizeY = sizeY; }
	void setAccess (U32 accesscode) { mAccess = accesscode; }
	void setRegionFlags (U32 region_flags) { mRegionFlags = region_flags; }
	void setAlpha(F32 alpha) { mAlpha = alpha; }
	void setLandForSaleImage (LLUUID image_id);
	void setMapImageID (const LLUUID& id, const U8 &layer) { mMapImageID[layer] = id; }
	std::string getName() const { return mName; }
	const std::string getFlagsString() const { return LLViewerRegion::regionFlagsToString(mRegionFlags); }
	const U64 getRegionFlags() const { return mRegionFlags; }
	const std::string getAccessString() const { return LLViewerRegion::accessToString((U8)mAccess); }
	const std::string getShortAccessString() const { return LLViewerRegion::accessToShortString(static_cast<U8>(mAccess)); }
	const std::string getAccessIcon() const { return LLViewerRegion::getAccessIcon(static_cast<U8>(mAccess)); }
	const U8 getAccess() const { return mAccess; }
	const S32 getAgentCount() const;
	LLPointer<LLViewerFetchedTexture> getLandForSaleImage();
	const F32 getAlpha() const { return mAlpha; }
	const U64& getHandle() const { return mHandle; }
	const U16 getSizeX() const { return mSizeX; }
	const U16 getSizeY() const { return mSizeY; }
	bool isName(const std::string& name) const;
	bool isDown() { return (mAccess == SIM_ACCESS_DOWN); }
	bool isPG() { return (mAccess <= SIM_ACCESS_PG); }
	bool isAdult() { return (mAccess == SIM_ACCESS_ADULT); }
	void dump() const;
	typedef std::vector<LLItemInfo> item_info_list_t;
	void clearItems();
	void insertTeleHub(const LLItemInfo& item) { mTelehubs.push_back(item); }
	void insertInfoHub(const LLItemInfo& item) { mInfohubs.push_back(item); }
	void insertPGEvent(const LLItemInfo& item) { mPGEvents.push_back(item); }
	void insertMatureEvent(const LLItemInfo& item) { mMatureEvents.push_back(item); }
	void insertAdultEvent(const LLItemInfo& item) { mAdultEvents.push_back(item); }
	void insertLandForSale(const LLItemInfo& item) { mLandForSale.push_back(item); }
	void insertLandForSaleAdult(const LLItemInfo& item) { mLandForSaleAdult.push_back(item); }
	void insertAgentLocation(const LLItemInfo& item);
	const LLSimInfo::item_info_list_t& getTeleHub() const { return mTelehubs; }
	const LLSimInfo::item_info_list_t& getInfoHub() const { return mInfohubs; }
	const LLSimInfo::item_info_list_t& getPGEvent() const { return mPGEvents; }
	const LLSimInfo::item_info_list_t& getMatureEvent() const { return mMatureEvents; }
	const LLSimInfo::item_info_list_t& getAdultEvent() const { return mAdultEvents; }
	const LLSimInfo::item_info_list_t& getLandForSale() const { return mLandForSale; }
	const LLSimInfo::item_info_list_t& getLandForSaleAdult() const { return mLandForSaleAdult; }
	const LLSimInfo::item_info_list_t& getAgentLocation() const { return mAgentLocations; }
private:
	U64 mHandle;
	U16 mSizeX;
	U16 mSizeY;
	std::string mName;
	F64 mAgentsUpdateTime;
	bool mFirstAgentRequest;
	U32  mAccess;
	U32 mRegionFlags;
	F32 mAlpha;
public:
	LLUUID mMapImageID[SIM_LAYER_COUNT];
	LLPointer<LLViewerFetchedTexture> mLayerImage[SIM_LAYER_COUNT];
	item_info_list_t mTelehubs;
	item_info_list_t mInfohubs;
	item_info_list_t mPGEvents;
	item_info_list_t mMatureEvents;
	item_info_list_t mAdultEvents;
	item_info_list_t mLandForSale;
	item_info_list_t mLandForSaleAdult;
	item_info_list_t mAgentLocations;
};
struct LLWorldMapLayer
{
	BOOL LayerDefined;
	LLPointer<LLViewerTexture> LayerImage;
	LLUUID LayerImageID;
	LLRect LayerExtents;
	LLWorldMapLayer() : LayerDefined(FALSE) { }
};
const S32 MAP_MAX_SIZE = 16384;
const S32 MAP_BLOCK_SIZE = 16;
const S32 MAP_BLOCK_RES = (MAP_MAX_SIZE / MAP_BLOCK_SIZE);
class LLWorldMap : public LLSingleton<LLWorldMap>
{
	friend class LLMapLayerResponder;
public:
	typedef void(*url_callback_t)(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport);
	LLWorldMap();
	~LLWorldMap();
	void reset();
	void clearImageRefs();
	void dropImagePriorities();
	void reloadItems(bool force = false);
	typedef std::map<U64, LLSimInfo*> sim_info_map_t;
	const LLWorldMap::sim_info_map_t& getRegionMap() const { return mSimInfoMap; }
	void updateRegions(S32 x0, S32 y0, S32 x1, S32 y1);
	static bool insertRegion(U32 x_world, U32 y_world, std::string& name, LLUUID& uuid, U32 accesscode, U64 region_flags);
	static bool insertRegion(U32 x_world, U32 y_world, U16 x_size, U16 y_size, std::string& name, LLUUID& uuid, U32 accesscode, U64 region_flags);
	static bool insertItem(U32 x_world, U32 y_world, std::string& name, LLUUID& uuid, U32 type, S32 extra, S32 extra2);
	LLSimInfo* simInfoFromHandle(const U64 handle);
	LLSimInfo* simInfoFromPosGlobal(const LLVector3d& pos_global);
	LLSimInfo* simInfoFromName(const std::string& sim_name);
	bool simNameFromPosGlobal(const LLVector3d& pos_global, std::string& outSimName );
	static void gotMapServerURL(bool flag) { sGotMapURL = flag; }
	static bool useWebMapTiles();
	void dump();
	void cancelTracking() { mIsTrackingLocation = false; mIsTrackingFound = false; mIsInvalidLocation = false; mIsTrackingDoubleClick = false; mIsTrackingCommit = false; }
	void setTracking(const LLVector3d& loc) { mIsTrackingLocation = true; mTrackingLocation = loc; mIsTrackingFound = false; mIsInvalidLocation = false; mIsTrackingDoubleClick = false; mIsTrackingCommit = false;}
	void setTrackingInvalid() { mIsTrackingFound = true; mIsInvalidLocation = true;  }
	void setTrackingValid()   { mIsTrackingFound = true; mIsInvalidLocation = false; }
	void setTrackingDoubleClick() { mIsTrackingDoubleClick = true; }
	void setTrackingCommit() { mIsTrackingCommit = true; }
	bool isTracking() { return mIsTrackingLocation; }
	bool isTrackingValidLocation()   { return mIsTrackingFound && !mIsInvalidLocation; }
	bool isTrackingInvalidLocation() { return mIsTrackingFound &&  mIsInvalidLocation; }
	bool isTrackingDoubleClick() { return mIsTrackingDoubleClick; }
	bool isTrackingCommit() { return mIsTrackingCommit; }
	bool isTrackingInRectangle(F64 x0, F64 y0, F64 x1, F64 y1);
	LLVector3d getTrackedPositionGlobal() const { return mTrackingLocation; }
	std::vector<LLWorldMapLayer>::iterator getMapLayerBegin()		{ return mMapLayers.begin(); }
	std::vector<LLWorldMapLayer>::iterator getMapLayerEnd()		{ return mMapLayers.end(); }
	std::map<U32, std::vector<bool> >::const_iterator findMapBlock(U32 layer, U32 x, U32 y) const
																			{ return LLWorldMap::instance().mMapBlockMap[layer].find((x<<16)|y); }
	std::map<U32, std::vector<bool> >::const_iterator getMapBlockEnd(U32 layer) const
																			{ return LLWorldMap::instance().mMapBlockMap[layer].end(); }
	void	equalizeBoostLevels();
	LLPointer<LLViewerFetchedTexture> getObjectsTile(U32 grid_x, U32 grid_y, S32 level, bool load = true) { return mWorldMipmap.getObjectsTile(grid_x, grid_y, level, load); }
	static void processMapLayerReply(LLMessageSystem*, void**);
private:
	bool clearItems(bool force = false);
	void clearSimFlags();
	LLSimInfo* createSimInfoFromHandle(const U64 handle);
	sim_info_map_t mSimInfoMap;
	void sendMapLayerRequest();
	std::vector<LLWorldMapLayer>	mMapLayers;
	bool							mMapLoaded;
private:
	LLWorldMipmap	mWorldMipmap;
	std::map<U32, std::vector<bool> > mMapBlockMap[SIM_LAYER_COUNT];
	bool			mIsTrackingLocation;
	bool			mIsTrackingFound;
	bool			mIsInvalidLocation;
	bool			mIsTrackingDoubleClick;
	bool			mIsTrackingCommit;
	LLVector3d		mTrackingLocation;
	LLTimer	mRequestTimer;
	bool			mFirstRequest;
	static bool sGotMapURL;
};
#endif
