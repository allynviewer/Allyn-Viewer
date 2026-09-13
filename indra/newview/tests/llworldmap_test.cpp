/** 
 * @file llworldmap_test.cpp
 * @author Merov Linden
 * @date 2009-03-09
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "../llviewerprecompiledheaders.h"
#include "../llworldmap.h"
#include "../llviewerimagelist.h"
#include "../llworldmapmessage.h"
#include "../test/lltut.h"
LLViewerImageList::LLViewerImageList() { }
LLViewerImageList::~LLViewerImageList() { }
LLViewerImageList gImageList;
LLViewerImage* LLViewerImageList::getImage(const LLUUID &image_id,
												   BOOL usemipmaps,
												   BOOL level_immediate,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   LLHost request_from_host)
{ return NULL; }
void LLViewerImage::setBoostLevel(S32 level) { }
void LLImageGL::setAddressMode(LLTexUnit::eTextureAddressMode mode) { }
LLWorldMapMessage::LLWorldMapMessage() { }
LLWorldMapMessage::~LLWorldMapMessage() { }
void LLWorldMapMessage::sendItemRequest(U32 type, U64 handle) { }
void LLWorldMapMessage::sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent) { }
LLWorldMipmap::LLWorldMipmap() { }
LLWorldMipmap::~LLWorldMipmap() { }
void LLWorldMipmap::reset() { }
void LLWorldMipmap::dropBoostLevels() { }
void LLWorldMipmap::equalizeBoostLevels() { }
LLPointer<LLViewerImage> LLWorldMipmap::getObjectsTile(U32 grid_x, U32 grid_y, S32 level, bool load)
{ return NULL; }
BOOL gPacificDaylightTime;
const F32 X_WORLD_TEST = 1000.0f * REGION_WIDTH_METERS;
const F32 Y_WORLD_TEST = 2000.0f * REGION_WIDTH_METERS;
const F32 Z_WORLD_TEST = 240.0f;
const std::string ITEM_NAME_TEST = "Item Foo";
const std::string TOOLTIP_TEST = "Tooltip Foo";
const std::string SIM_NAME_TEST = "Sim Foo";
namespace tut
{
	struct iteminfo_test
	{
		LLItemInfo* mItem;
		iteminfo_test()
		{
			LLUUID id;
			mItem = new LLItemInfo(X_WORLD_TEST, Y_WORLD_TEST, ITEM_NAME_TEST, id);
		}
		~iteminfo_test()
		{
			delete mItem;
		}
	};
	struct siminfo_test
	{
		LLSimInfo* mSim;
		siminfo_test()
		{
			U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
			mSim = new LLSimInfo(handle);
		}
		~siminfo_test()
		{
			delete mSim;
		}
	};
	struct worldmap_test
	{
		LLWorldMap* mWorld;
		worldmap_test()
		{
			mWorld = LLWorldMap::getInstance();
		}
		~worldmap_test()
		{
			mWorld = NULL;
		}
	};
	typedef test_group<iteminfo_test> iteminfo_t;
	typedef iteminfo_t::object iteminfo_object_t;
	tut::iteminfo_t tut_iteminfo("iteminfo");
	typedef test_group<siminfo_test> siminfo_t;
	typedef siminfo_t::object siminfo_object_t;
	tut::siminfo_t tut_siminfo("siminfo");
	typedef test_group<worldmap_test> worldmap_t;
	typedef worldmap_t::object worldmap_object_t;
	tut::worldmap_t tut_worldmap("worldmap");
	template<> template<>
	void iteminfo_object_t::test<1>()
	{
		mItem->setCount(10);
		ensure("LLItemInfo::setCount() test failed", mItem->getCount() == 10);
		std::string tooltip = TOOLTIP_TEST;
		mItem->setTooltip(tooltip);
		ensure("LLItemInfo::setTooltip() test failed", mItem->getToolTip() == TOOLTIP_TEST);
		mItem->setElevation(Z_WORLD_TEST);
		LLVector3d pos = mItem->getGlobalPosition();
		LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, Z_WORLD_TEST);
		ensure("LLItemInfo::getGlobalPosition() test failed", pos == ref);
		std::string name = mItem->getName();
		ensure("LLItemInfo::getName() test failed", name == ITEM_NAME_TEST);
		ensure("LLItemInfo::isName() test failed", mItem->isName(name));
		LLUUID id;
		ensure("LLItemInfo::getUUID() test failed", mItem->getUUID() == id);
		U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
		ensure("LLItemInfo::getRegionHandle() test failed", mItem->getRegionHandle() == handle);
	}
	template<> template<>
	void siminfo_object_t::test<1>()
	{
		std::string name = SIM_NAME_TEST;
		mSim->setName(name);
		ensure("LLSimInfo::setName() test failed", mSim->getName() == SIM_NAME_TEST);
		ensure("LLSimInfo::isName() test failed", mSim->isName(name));
		LLVector3 local;
		LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, 0.0f);
		LLVector3d pos = mSim->getGlobalPos(local);
		ensure("LLSimInfo::getGlobalPos() test failed", pos == ref);
		pos = mSim->getGlobalOrigin();
		ensure("LLSimInfo::getGlobalOrigin() test failed", pos == ref);
		try {
			mSim->clearImage();
		} catch (...) {
			fail("LLSimInfo::clearImage() test failed");
		}
		try {
			mSim->dropImagePriority();
		} catch (...) {
			fail("LLSimInfo::dropImagePriority() test failed");
		}
		try {
			mSim->updateAgentCount(0.0f);
		} catch (...) {
			fail("LLSimInfo::updateAgentCount() test failed");
		}
		S32 agents = mSim->getAgentCount();
		ensure("LLSimInfo::getAgentCount() test failed", agents == 0);
		LLUUID id;
		mSim->setLandForSaleImage(id);
		LLPointer<LLViewerImage> image = mSim->getLandForSaleImage();
		ensure("LLSimInfo::getLandForSaleImage() test failed", image.isNull());
		mSim->setAccess(SIM_ACCESS_PG);
		ensure("LLSimInfo::isPG() test failed", mSim->isPG());
		mSim->setAccess(SIM_ACCESS_DOWN);
		ensure("LLSimInfo::isDown() test failed", mSim->isDown());
	}
	template<> template<>
	void siminfo_object_t::test<2>()
	{
		try {
			mSim->clearItems();
		} catch (...) {
			fail("LLSimInfo::clearItems() at init test failed");
		}
		LLSimInfo::item_info_list_t list;
		list = mSim->getTeleHub();
		ensure("LLSimInfo::getTeleHub() empty at init test failed", list.empty());
		list = mSim->getInfoHub();
		ensure("LLSimInfo::getInfoHub() empty at init test failed", list.empty());
		list = mSim->getPGEvent();
		ensure("LLSimInfo::getPGEvent() empty at init test failed", list.empty());
		list = mSim->getMatureEvent();
		ensure("LLSimInfo::getMatureEvent() empty at init test failed", list.empty());
		list = mSim->getAdultEvent();
		ensure("LLSimInfo::getAdultEvent() empty at init test failed", list.empty());
		list = mSim->getLandForSale();
		ensure("LLSimInfo::getLandForSale() empty at init test failed", list.empty());
		list = mSim->getLandForSaleAdult();
		ensure("LLSimInfo::getLandForSaleAdult() empty at init test failed", list.empty());
		list = mSim->getAgentLocation();
		ensure("LLSimInfo::getAgentLocation() empty at init test failed", list.empty());
		LLUUID id;
		LLItemInfo item(X_WORLD_TEST, Y_WORLD_TEST, ITEM_NAME_TEST, id);
		mSim->insertTeleHub(item);
		mSim->insertInfoHub(item);
		mSim->insertPGEvent(item);
		mSim->insertMatureEvent(item);
		mSim->insertAdultEvent(item);
		mSim->insertLandForSale(item);
		mSim->insertLandForSaleAdult(item);
		mSim->insertAgentLocation(item);
		list = mSim->getTeleHub();
		ensure("LLSimInfo::insertTeleHub() test failed", list.size() == 1);
		list = mSim->getInfoHub();
		ensure("LLSimInfo::insertInfoHub() test failed", list.size() == 1);
		list = mSim->getPGEvent();
		ensure("LLSimInfo::insertPGEvent() test failed", list.size() == 1);
		list = mSim->getMatureEvent();
		ensure("LLSimInfo::insertMatureEvent() test failed", list.size() == 1);
		list = mSim->getAdultEvent();
		ensure("LLSimInfo::insertAdultEvent() test failed", list.size() == 1);
		list = mSim->getLandForSale();
		ensure("LLSimInfo::insertLandForSale() test failed", list.size() == 1);
		list = mSim->getLandForSaleAdult();
		ensure("LLSimInfo::insertLandForSaleAdult() test failed", list.size() == 1);
		list = mSim->getAgentLocation();
		ensure("LLSimInfo::insertAgentLocation() test failed", list.size() == 1);
		try {
			mSim->clearItems();
		} catch (...) {
			fail("LLSimInfo::clearItems() at end test failed");
		}
		list = mSim->getTeleHub();
		ensure("LLSimInfo::getTeleHub() empty after clear test failed", list.empty());
		list = mSim->getInfoHub();
		ensure("LLSimInfo::getInfoHub() empty after clear test failed", list.empty());
		list = mSim->getPGEvent();
		ensure("LLSimInfo::getPGEvent() empty after clear test failed", list.empty());
		list = mSim->getMatureEvent();
		ensure("LLSimInfo::getMatureEvent() empty after clear test failed", list.empty());
		list = mSim->getAdultEvent();
		ensure("LLSimInfo::getAdultEvent() empty after clear test failed", list.empty());
		list = mSim->getLandForSale();
		ensure("LLSimInfo::getLandForSale() empty after clear test failed", list.empty());
		list = mSim->getLandForSaleAdult();
		ensure("LLSimInfo::getLandForSaleAdult() empty after clear test failed", list.empty());
		list = mSim->getAgentLocation();
		ensure("LLSimInfo::getAgentLocation() empty after clear test failed", list.size() == 1);
	}
	template<> template<>
	void worldmap_object_t::test<1>()
	{
		try {
			mWorld->reset();
		} catch (...) {
			fail("LLWorldMap::reset() at init test failed");
		}
		try {
			mWorld->clearImageRefs();
		} catch (...) {
			fail("LLWorldMap::clearImageRefs() test failed");
		}
		try {
			mWorld->dropImagePriorities();
		} catch (...) {
			fail("LLWorldMap::dropImagePriorities() test failed");
		}
		try {
			mWorld->reloadItems(true);
		} catch (...) {
			fail("LLWorldMap::reloadItems() test failed");
		}
		try {
			mWorld->updateRegions(1000, 1000, 1004, 1004);
		} catch (...) {
			fail("LLWorldMap::updateRegions() test failed");
		}
 		try {
 			mWorld->equalizeBoostLevels();
 		} catch (...) {
 			fail("LLWorldMap::equalizeBoostLevels() test failed");
 		}
		try {
			LLPointer<LLViewerImage> image = mWorld->getObjectsTile((U32)(X_WORLD_TEST/REGION_WIDTH_METERS), (U32)(Y_WORLD_TEST/REGION_WIDTH_METERS), 1);
			ensure("LLWorldMap::getObjectsTile() failed", image.isNull());
		} catch (...) {
			fail("LLWorldMap::getObjectsTile() test failed with exception");
		}
	}
	template<> template<>
	void worldmap_object_t::test<2>()
	{
		try {
			mWorld->reset();
		} catch (...) {
			fail("LLWorldMap::reset() at init test failed");
		}
		LLWorldMap::sim_info_map_t list;
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::getRegionMap() empty at init test failed", list.empty());
		bool success;
		LLUUID id;
		std::string name_sim = SIM_NAME_TEST;
		success = mWorld->insertRegion(	U32(X_WORLD_TEST),
						U32(Y_WORLD_TEST),
										name_sim,
										id,
										SIM_ACCESS_PG,
										REGION_FLAGS_SANDBOX);
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::insertRegion() failed", success && (list.size() == 1));
		std::string name_item = ITEM_NAME_TEST;
		success = mWorld->insertItem(	U32(X_WORLD_TEST + REGION_WIDTH_METERS/2),
						U32(Y_WORLD_TEST + REGION_WIDTH_METERS/2),
										name_item,
										id,
										MAP_ITEM_LAND_FOR_SALE,
										0, 0);
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::insertItem() in existing region failed", success && (list.size() == 1));
		success = mWorld->insertItem(	U32(X_WORLD_TEST + REGION_WIDTH_METERS*2),
						U32(Y_WORLD_TEST + REGION_WIDTH_METERS*2),
										name_item,
										id,
										MAP_ITEM_LAND_FOR_SALE,
										0, 0);
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::insertItem() in unexisting region failed", success && (list.size() == 2));
		LLVector3d pos1(	X_WORLD_TEST + REGION_WIDTH_METERS*2 + REGION_WIDTH_METERS/2,
							Y_WORLD_TEST + REGION_WIDTH_METERS*2 + REGION_WIDTH_METERS/2,
							0.0f);
		LLSimInfo* sim;
		sim = mWorld->simInfoFromPosGlobal(pos1);
		ensure("LLWorldMap::simInfoFromPosGlobal() test on existing region failed", sim != NULL);
		LLVector3d pos2(	X_WORLD_TEST + REGION_WIDTH_METERS*4 + REGION_WIDTH_METERS/2,
							Y_WORLD_TEST + REGION_WIDTH_METERS*4 + REGION_WIDTH_METERS/2,
							0.0f);
		sim = mWorld->simInfoFromPosGlobal(pos2);
		ensure("LLWorldMap::simInfoFromPosGlobal() test outside region failed", sim == NULL);
		sim = mWorld->simInfoFromName(name_sim);
		ensure("LLWorldMap::simInfoFromName() test on existing region failed", sim != NULL);
		U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
		sim = mWorld->simInfoFromHandle(handle);
		ensure("LLWorldMap::simInfoFromHandle() test on existing region failed", sim != NULL);
		LLVector3d pos3(	X_WORLD_TEST + REGION_WIDTH_METERS/2,
							Y_WORLD_TEST + REGION_WIDTH_METERS/2,
							0.0f);
		success = mWorld->simNameFromPosGlobal(pos3, name_sim);
		ensure("LLWorldMap::simNameFromPosGlobal() test on existing region failed", success && (name_sim == SIM_NAME_TEST));
		try {
			mWorld->reset();
		} catch (...) {
			fail("LLWorldMap::reset() at end test failed");
		}
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::getRegionMap() empty at end test failed", list.empty());
	}
	template<> template<>
	void worldmap_object_t::test<3>()
	{
		LLVector3d pos( X_WORLD_TEST + REGION_WIDTH_METERS/2, Y_WORLD_TEST + REGION_WIDTH_METERS/2, Z_WORLD_TEST);
		mWorld->cancelTracking();
		ensure("LLWorldMap::cancelTracking() at begin test failed", mWorld->isTracking() == false);
		mWorld->setTracking(pos);
		ensure("LLWorldMap::setTracking() failed", mWorld->isTracking() && !mWorld->isTrackingValidLocation());
		mWorld->setTrackingDoubleClick();
		ensure("LLWorldMap::setTrackingDoubleClick() failed", mWorld->isTrackingDoubleClick());
		mWorld->setTrackingCommit();
		ensure("LLWorldMap::setTrackingCommit() failed", mWorld->isTrackingCommit());
		bool inRect = mWorld->isTrackingInRectangle(	X_WORLD_TEST, Y_WORLD_TEST,
														X_WORLD_TEST + REGION_WIDTH_METERS,
														Y_WORLD_TEST + REGION_WIDTH_METERS);
		ensure("LLWorldMap::isTrackingInRectangle() in rectangle failed", inRect);
		inRect = mWorld->isTrackingInRectangle(			X_WORLD_TEST + REGION_WIDTH_METERS,
														Y_WORLD_TEST + REGION_WIDTH_METERS,
														X_WORLD_TEST + 2 * REGION_WIDTH_METERS,
														Y_WORLD_TEST + 2 * REGION_WIDTH_METERS);
		ensure("LLWorldMap::isTrackingInRectangle() outside rectangle failed", !inRect);
		mWorld->setTrackingValid();
		ensure("LLWorldMap::setTrackingValid() failed", mWorld->isTrackingValidLocation() && !mWorld->isTrackingInvalidLocation());
		mWorld->setTrackingInvalid();
		ensure("LLWorldMap::setTrackingInvalid() failed", !mWorld->isTrackingValidLocation() && mWorld->isTrackingInvalidLocation());
		LLVector3d res = mWorld->getTrackedPositionGlobal();
		ensure("LLWorldMap::getTrackedPositionGlobal() failed", res == pos);
		mWorld->cancelTracking();
		ensure("LLWorldMap::cancelTracking() at end test failed", mWorld->isTracking() == false);
	}
}
