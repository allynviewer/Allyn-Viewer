/** 
 * @file llworldmap.cpp
 * @brief Underlying data representation for map of the world
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
#include "llviewerprecompiledheaders.h"
#include "llworldmap.h"
#include "llregionhandle.h"
#include "message.h"
#include "llappviewer.h"
#include "llagent.h"
#include "llmapresponders.h"
#include "llviewercontrol.h"
#include "llfloaterworldmap.h"
#include "lltexturecache.h"
#include "lltracker.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llregionflags.h"
#include "llworldmapmessage.h"
#include "hippogridmanager.h"
#include "lfsimfeaturehandler.h"
bool LLWorldMap::sGotMapURL =  false;
const F32 AGENTS_UPDATE_TIMER = 30.0;
const F32 REQUEST_ITEMS_TIMER = 10.f * 60.f;
LLItemInfo::LLItemInfo(F32 global_x, F32 global_y,
					   const std::string& name,
					   LLUUID id)
:	mName(name),
	mToolTip(""),
	mPosGlobal(global_x, global_y, 40.0),
	mID(id),
	mCount(1)
{
}
LLSimInfo::LLSimInfo(U64 handle)
:	mHandle(handle),
	mName(),
	mAgentsUpdateTime(0),
	mAccess(0x0),
	mRegionFlags(0x0),
	mFirstAgentRequest(true),
    mSizeX(REGION_WIDTH_UNITS),
	mSizeY(REGION_WIDTH_UNITS),
	mAlpha(0.f)
{
}
void LLSimInfo::setLandForSaleImage (LLUUID image_id)
{
	const bool is_whitecore = gHippoGridManager->getConnectedGrid()->isWhiteCore();
	if (is_whitecore && mMapImageID[SIM_LAYER_OVERLAY].isNull() && image_id.notNull() && gTextureList.findImage(image_id, TEX_LIST_STANDARD))
		LLAppViewer::getTextureCache()->removeFromCache(image_id);
	mMapImageID[SIM_LAYER_OVERLAY] = image_id;
	if (mMapImageID[SIM_LAYER_OVERLAY].notNull())
	{
		mLayerImage[SIM_LAYER_OVERLAY] = LLViewerTextureManager::getFetchedTexture(mMapImageID[SIM_LAYER_OVERLAY], FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_MAP, LLViewerTexture::LOD_TEXTURE);
		if (is_whitecore) mLayerImage[SIM_LAYER_OVERLAY]->forceImmediateUpdate();
		mLayerImage[SIM_LAYER_OVERLAY]->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
	else
	{
		mLayerImage[SIM_LAYER_OVERLAY] = NULL;
	}
}
LLPointer<LLViewerFetchedTexture> LLSimInfo::getLandForSaleImage ()
{
	if (mLayerImage[SIM_LAYER_OVERLAY].isNull() && mMapImageID[SIM_LAYER_OVERLAY].notNull())
	{
		mLayerImage[SIM_LAYER_OVERLAY] = LLViewerTextureManager::getFetchedTexture(mMapImageID[SIM_LAYER_OVERLAY], FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_MAP, LLViewerTexture::LOD_TEXTURE);
		mLayerImage[SIM_LAYER_OVERLAY]->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
	if (!mLayerImage[SIM_LAYER_OVERLAY].isNull())
	{
		mLayerImage[SIM_LAYER_OVERLAY]->setBoostLevel(LLGLTexture::BOOST_MAP);
	}
	return mLayerImage[SIM_LAYER_OVERLAY];
}
LLVector3d LLSimInfo::getGlobalPos(const LLVector3& local_pos) const
{
	LLVector3d pos = from_region_handle(mHandle);
	pos.mdV[VX] += local_pos.mV[VX];
	pos.mdV[VY] += local_pos.mV[VY];
	pos.mdV[VZ] += local_pos.mV[VZ];
	return pos;
}
LLVector3d LLSimInfo::getGlobalOrigin() const
{
	return from_region_handle(mHandle);
}
LLVector3 LLSimInfo::getLocalPos(LLVector3d global_pos) const
{
	LLVector3d sim_origin = from_region_handle(mHandle);
	return LLVector3(global_pos - sim_origin);
}
void LLSimInfo::clearImage()
{
	for(U32 layer = SIM_LAYER_BEGIN; layer < SIM_LAYER_COUNT;++layer)
	{
		if(!mLayerImage[layer].isNull())
		{
			mLayerImage[layer]->setBoostLevel(0);
			mLayerImage[layer]=NULL;
		}
	}
}
void LLSimInfo::dropImagePriority(sim_layer_type layer)
{
	if(layer == SIM_LAYER_COUNT)
	{
		for(U32 layer = SIM_LAYER_BEGIN; layer < SIM_LAYER_COUNT;++layer)
		{
			if(!mLayerImage[layer].isNull())
			{
				mLayerImage[layer]->setBoostLevel(0);
			}
		}
	}
	else if(layer < SIM_LAYER_COUNT && layer >= SIM_LAYER_BEGIN && !mLayerImage[layer].isNull())
	{
		mLayerImage[layer]->setBoostLevel(0);
	}
}
void LLSimInfo::updateAgentCount(F64 time)
{
	if ((time - mAgentsUpdateTime > AGENTS_UPDATE_TIMER) || mFirstAgentRequest)
	{
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_AGENT_LOCATIONS, mHandle);
		mAgentsUpdateTime = time;
		mFirstAgentRequest = false;
	}
}
const S32 LLSimInfo::getAgentCount() const
{
	S32 total_agent_count = 0;
	for (LLSimInfo::item_info_list_t::const_iterator it = mAgentLocations.begin(); it != mAgentLocations.end(); ++it)
	{
		total_agent_count += it->getCount();
	}
	return total_agent_count;
}
bool LLSimInfo::isName(const std::string& name) const
{
	return (LLStringUtil::compareInsensitive(name, mName) == 0);
}
void LLSimInfo::dump() const
{
	U32 x_pos, y_pos;
	from_region_handle(mHandle, &x_pos, &y_pos);
	LL_INFOS("World Map") << x_pos << "," << y_pos
		<< " " << mName
		<< " " << (S32)mAccess
		<< " " << std::hex << mRegionFlags << std::dec
		<< " " << mSizeX << "x" << mSizeY
		<< LL_ENDL;
}
void LLSimInfo::clearItems()
{
	mTelehubs.clear();
	mInfohubs.clear();
	mPGEvents.clear();
	mMatureEvents.clear();
	mAdultEvents.clear();
	mLandForSale.clear();
	mLandForSaleAdult.clear();
}
void LLSimInfo::insertAgentLocation(const LLItemInfo& item)
{
	std::string name = item.getName();
	item_info_list_t::iterator lastiter;
	for (lastiter = mAgentLocations.begin(); lastiter != mAgentLocations.end(); ++lastiter)
	{
		LLItemInfo& info = *lastiter;
		if (info.isName(name))
		{
			break;
		}
	}
	if (lastiter != mAgentLocations.begin())
	{
		mAgentLocations.erase(mAgentLocations.begin(), lastiter);
	}
	mAgentLocations.push_back(item);
}
LLWorldMap::LLWorldMap() :
	mIsTrackingLocation( false ),
	mIsTrackingFound( false ),
	mIsInvalidLocation( false ),
	mIsTrackingDoubleClick( false ),
	mIsTrackingCommit( false ),
	mTrackingLocation( 0, 0, 0 ),
	mFirstRequest(true),
	mMapLoaded(false)
{
	clearSimFlags();
}
LLWorldMap::~LLWorldMap()
{
	reset();
}
void LLWorldMap::reset()
{
	clearItems(true);
	clearImageRefs();
	clearSimFlags();
	for_each(mSimInfoMap.begin(), mSimInfoMap.end(), DeletePairedPointer());
	mSimInfoMap.clear();
	mMapLoaded = false;
	mMapLayers.clear();
	for (U32 map=SIM_LAYER_BEGIN; map<SIM_LAYER_OVERLAY; ++map)
	{
		mMapBlockMap[map].clear();
	}
}
bool LLWorldMap::clearItems(bool force)
{
	bool clear = false;
	if ((mRequestTimer.getElapsedTimeF32() > REQUEST_ITEMS_TIMER) || mFirstRequest || force)
	{
		mRequestTimer.reset();
		LLSimInfo* sim_info = NULL;
		for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
		{
			sim_info = it->second;
			if (sim_info)
			{
				sim_info->clearItems();
			}
		}
		clear = true;
		mFirstRequest = false;
	}
	return clear;
}
void LLWorldMap::clearImageRefs()
{
	mWorldMipmap.reset();
	LLSimInfo* sim_info = NULL;
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		sim_info = it->second;
		if(sim_info)
		{
			sim_info->clearImage();
		}
	}
}
void LLWorldMap::clearSimFlags()
{
	for (U32 map=SIM_LAYER_BEGIN; map<SIM_LAYER_OVERLAY; ++map)
	{
		mMapBlockMap[map].clear();
	}
}
LLSimInfo* LLWorldMap::createSimInfoFromHandle(const U64 handle)
{
	LLSimInfo* sim_info = new LLSimInfo(handle);
	mSimInfoMap[handle] = sim_info;
	return sim_info;
}
void LLWorldMap::equalizeBoostLevels()
{
	mWorldMipmap.equalizeBoostLevels();
	return;
}
LLSimInfo* LLWorldMap::simInfoFromPosGlobal(const LLVector3d& pos_global)
{
	U64 handle = to_region_handle(pos_global);
	return simInfoFromHandle(handle);
}
LLSimInfo* LLWorldMap::simInfoFromHandle(const U64 handle)
{
	sim_info_map_t::const_iterator it = mSimInfoMap.find(handle);
	if (it != mSimInfoMap.end())
	{
		return it->second;
	}
	U32 x = 0, y = 0;
	from_region_handle(handle, &x, &y);
	for (it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		U32 checkRegionX, checkRegionY;
		from_region_handle((*it).first, &checkRegionX, &checkRegionY);
		LLSimInfo* info = (*it).second;
		if (x >= checkRegionX && x < (checkRegionX + info->getSizeX()) &&
			y >= checkRegionY && y < (checkRegionY + info->getSizeY()))
		{
			return info;
		}
	}
	return NULL;
}
LLSimInfo* LLWorldMap::simInfoFromName(const std::string& sim_name)
{
	LLSimInfo* sim_info = NULL;
	if (!sim_name.empty())
	{
		sim_info_map_t::iterator it;
		for (it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
		{
			sim_info = it->second;
			if (sim_info && sim_info->isName(sim_name) )
			{
				break;
			}
		}
		if (it == mSimInfoMap.end())
			sim_info = NULL;
	}
	return sim_info;
}
bool LLWorldMap::simNameFromPosGlobal(const LLVector3d& pos_global, std::string & outSimName )
{
	LLSimInfo* sim_info = simInfoFromPosGlobal(pos_global);
	if (sim_info)
	{
		outSimName = sim_info->getName();
	}
	else
	{
		outSimName = "(unknown region)";
	}
	return (sim_info != NULL);
}
void LLWorldMap::sendMapLayerRequest()
{
	if (!gAgent.getRegion()) return;
	std::string url = gAgent.getRegion()->getCapability(
		gAgent.isGodlike() ? "MapLayerGod" : "MapLayer");
	U32 flags = layerToFlags((sim_layer_type)SIM_LAYER_COMPOSITE);
	if (!url.empty())
	{
		LLSD body;
		body["Flags"] = (LLSD::Integer)flags;
		LLHTTPClient::post(url, body, new LLMapLayerResponder);
	}
	else
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_MapLayerRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addU32Fast(_PREHASH_Flags, flags);
		msg->addU32Fast(_PREHASH_EstateID, 0);
		msg->addBOOLFast(_PREHASH_Godlike, FALSE);
		gAgent.sendReliableMessage();
	}
}
void LLWorldMap::processMapLayerReply(LLMessageSystem* msg, void**)
{
	U32 agent_flags;
	msg->getU32Fast(_PREHASH_AgentData, _PREHASH_Flags, agent_flags);
	U32 layer = flagsToLayer(agent_flags);
	if (layer != SIM_LAYER_COMPOSITE)
	{
		LL_WARNS() << "Invalid or out of date map image type returned!" << LL_ENDL;
		return;
	}
	LLUUID image_id;
	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_LayerData);
	LLWorldMap::getInstance()->mMapLayers.clear();
	for (S32 block=0; block<num_blocks; ++block)
	{
		LLWorldMapLayer new_layer;
		new_layer.LayerDefined = TRUE;
		msg->getUUIDFast(_PREHASH_LayerData, _PREHASH_ImageID, new_layer.LayerImageID, block);
		U32 left, right, top, bottom;
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Left, left, block);
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Right, right, block);
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Top, top, block);
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Bottom, bottom, block);
		new_layer.LayerImage = LLViewerTextureManager::getFetchedTexture(new_layer.LayerImageID, FTT_MAP_TILE, MIPMAP_TRUE, LLGLTexture::BOOST_MAP, LLViewerTexture::LOD_TEXTURE);
		gGL.getTexUnit(0)->bind(new_layer.LayerImage.get());
		new_layer.LayerImage->setAddressMode(LLTexUnit::TAM_CLAMP);
		new_layer.LayerExtents.mLeft = left;
		new_layer.LayerExtents.mRight = right;
		new_layer.LayerExtents.mBottom = bottom;
		new_layer.LayerExtents.mTop = top;
		LLWorldMap::getInstance()->mMapLayers.push_back(new_layer);
	}
	LLWorldMap::getInstance()->mMapLoaded = true;
}
bool LLWorldMap::useWebMapTiles()
{
	static const LLCachedControl<bool> use_web_map_tiles("UseWebMapTiles",false);
	return use_web_map_tiles &&
		   (( gHippoGridManager->getConnectedGrid()->isSecondLife() || sGotMapURL || !LFSimFeatureHandler::instance().mapServerURL().empty()));
}
void LLWorldMap::reloadItems(bool force)
{
	if (clearItems(force))
	{
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_TELEHUB);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_PG_EVENT);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_MATURE_EVENT);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_ADULT_EVENT);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_LAND_FOR_SALE);
	}
	if(!useWebMapTiles())
	{
		if(!mMapLoaded || force)
			sendMapLayerRequest();
	}
}
bool LLWorldMap::insertRegion(U32 x_world, U32 y_world, U16 x_size, U16 y_size, std::string& name, LLUUID& image_id, U32 accesscode, U64 region_flags)
{
	if (accesscode == 255)
	{
		if (LLWorldMap::getInstance()->isTrackingInRectangle( x_world, y_world, x_world + REGION_WIDTH_UNITS, y_world + REGION_WIDTH_UNITS))
		{
			LLWorldMap::getInstance()->setTrackingInvalid();
		}
		return false;
	}
	else
	{
		U64 handle = to_region_handle(x_world, y_world);
		LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
		if (siminfo == NULL)
		{
			siminfo = LLWorldMap::getInstance()->createSimInfoFromHandle(handle);
		}
		siminfo->setName(name);
		siminfo->setAccess(accesscode);
		siminfo->setRegionFlags(region_flags);
		siminfo->setLandForSaleImage(image_id);
		siminfo->setSize(x_size, y_size);
		if (LLWorldMap::getInstance()->isTrackingInRectangle( x_world, y_world, x_world + REGION_WIDTH_UNITS, y_world + REGION_WIDTH_UNITS))
		{
			if (siminfo->isDown())
			{
				LLWorldMap::getInstance()->setTrackingInvalid();
			}
			else
			{
				LLWorldMap::getInstance()->setTrackingValid();
			}
		}
		return true;
	}
}
bool LLWorldMap::insertItem(U32 x_world, U32 y_world, std::string& name, LLUUID& uuid, U32 type, S32 extra, S32 extra2)
{
	LLItemInfo new_item((F32)x_world, (F32)y_world, name, uuid);
	LLVector3d	pos((F32)x_world, (F32)y_world, 40.0);
	U64 handle = to_region_handle(pos);
	LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
	if (siminfo == NULL)
	{
		siminfo = LLWorldMap::getInstance()->createSimInfoFromHandle(handle);
	}
	switch (type)
	{
		case MAP_ITEM_TELEHUB:
		{
			if (extra2)
			{
				siminfo->insertInfoHub(new_item);
			}
			else
			{
				siminfo->insertTeleHub(new_item);
			}
			break;
		}
		case MAP_ITEM_PG_EVENT:
		case MAP_ITEM_MATURE_EVENT:
		case MAP_ITEM_ADULT_EVENT:
			{
				struct tm* timep;
				timep = utc_to_pacific_time(extra, gPacificDaylightTime);
				S32 display_hour = timep->tm_hour % 12;
				if (display_hour == 0) display_hour = 12;
				new_item.setTooltip( llformat( "%d:%02d %s",
											  display_hour,
											  timep->tm_min,
											  (timep->tm_hour < 12 ? "AM" : "PM") ) );
			new_item.setElevation((F64)extra2);
			if (type == MAP_ITEM_PG_EVENT)
			{
				siminfo->insertPGEvent(new_item);
			}
			else if (type == MAP_ITEM_MATURE_EVENT)
			{
				siminfo->insertMatureEvent(new_item);
			}
			else if (type == MAP_ITEM_ADULT_EVENT)
			{
				siminfo->insertAdultEvent(new_item);
			}
			break;
		}
		case MAP_ITEM_LAND_FOR_SALE:
		case MAP_ITEM_LAND_FOR_SALE_ADULT:
		{
			new_item.setTooltip(llformat("%d sq. m. %s%d (%.1f %s/sq. m.)", extra,
				gHippoGridManager->getConnectedGrid()->getCurrencySymbol().c_str(),
				extra2,
				(extra > 0) ? ((F32)extra2 / extra) : 0.f,
				gHippoGridManager->getConnectedGrid()->getCurrencySymbol().c_str()));
			if (type == MAP_ITEM_LAND_FOR_SALE)
			{
				siminfo->insertLandForSale(new_item);
			}
			else if (type == MAP_ITEM_LAND_FOR_SALE_ADULT)
			{
				siminfo->insertLandForSaleAdult(new_item);
			}
			break;
		}
		case MAP_ITEM_CLASSIFIED:
		{
			break;
		}
		case MAP_ITEM_AGENT_LOCATIONS:
		{
			if (extra > 0)
			{
				new_item.setCount(extra);
				siminfo->insertAgentLocation(new_item);
			}
			break;
		}
		default:
			break;
	}
	return true;
}
bool LLWorldMap::isTrackingInRectangle(F64 x0, F64 y0, F64 x1, F64 y1)
{
	if (!mIsTrackingLocation)
		return false;
	return ((mTrackingLocation[0] >= x0) && (mTrackingLocation[0] < x1) && (mTrackingLocation[1] >= y0) && (mTrackingLocation[1] < y1));
}
void LLWorldMap::dropImagePriorities()
{
	mWorldMipmap.dropBoostLevels();
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		LLSimInfo* info = it->second;
		info->dropImagePriority();
	}
}
void LLWorldMap::updateRegions(S32 x0, S32 y0, S32 x1, S32 y1)
{
	U32 global_x0 = x0 / MAP_BLOCK_SIZE;
	U32 global_x1 = x1 / MAP_BLOCK_SIZE;
	U32 global_y0 = y0 / MAP_BLOCK_SIZE;
	U32 global_y1 = y1 / MAP_BLOCK_SIZE;
	U32 max_range = (U16_MAX+1)/MAP_BLOCK_RES/MAP_BLOCK_SIZE - 1;
	U32 map_block_x0 = global_x0 / MAP_BLOCK_RES;
	U32 map_block_x1 = llmin(global_x1 / MAP_BLOCK_RES, max_range);
	U32 map_block_y0 = global_y0 / MAP_BLOCK_RES;
	U32 map_block_y1 = llmin(global_y1 / MAP_BLOCK_RES, max_range);
	const bool layer_start = useWebMapTiles() ? SIM_LAYER_OVERLAY : SIM_LAYER_BEGIN;
	for (U32 i = map_block_x0; i <= map_block_x1; ++i)
	{
	for (U32 j = map_block_y0; j <= map_block_y1; ++j)
	{
	x0 = global_x0 - i * MAP_BLOCK_RES;
	x1 = llmin(global_x1 - i * (U32)MAP_BLOCK_RES, (U32)MAP_BLOCK_RES-1);
	y0 = global_y0 - j * MAP_BLOCK_RES;
	y1 = llmin(global_y1 - j * (U32)MAP_BLOCK_RES, (U32)MAP_BLOCK_RES-1);
	for(U32 layer = layer_start;layer<SIM_LAYER_COUNT;++layer)
	{
	std::vector<bool> &block = mMapBlockMap[layer][(i << 16) | j];
	if(block.empty())
	{
		block.resize(MAP_BLOCK_RES*MAP_BLOCK_RES,false);
	}
	for (S32 block_x = llmax(x0, 0); block_x <= llmin(x1, MAP_BLOCK_RES-1); ++block_x)
	{
		for (S32 block_y = llmax(y0, 0); block_y <= llmin(y1, MAP_BLOCK_RES-1); ++block_y)
		{
			S32 offset = block_x | (block_y * MAP_BLOCK_RES);
			if (!block[offset])
			{
				U16 min_x = (block_x + i * MAP_BLOCK_RES) * MAP_BLOCK_SIZE;
				U16 max_x = min_x + MAP_BLOCK_SIZE - 1;
				U16 min_y = (block_y + j * MAP_BLOCK_RES) * MAP_BLOCK_SIZE;
				U32 max_y = min_y + MAP_BLOCK_SIZE - 1;
				LLWorldMapMessage::getInstance()->sendMapBlockRequest(min_x, min_y, max_x, max_y, false, layerToFlags((sim_layer_type)layer));
				block[offset] = true;
			}
		}
	}
	}
	}
	}
}
void LLWorldMap::dump()
{
	LL_INFOS("World Map") << "LLWorldMap::dump()" << LL_ENDL;
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		LLSimInfo* info = it->second;
		if (info)
		{
			info->dump();
		}
	}
}
