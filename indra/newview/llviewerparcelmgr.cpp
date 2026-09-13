/** 
 * @file llviewerparcelmgr.cpp
 * @brief Viewer-side representation of owned land
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"
#include "llviewerparcelmgr.h"
#include "llaudioengine.h"
#include "indra_constants.h"
#include "llcachename.h"
#include "llgl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llsecondlifeurls.h"
#include "message.h"
#include "llagent.h"
#include "llagentaccess.h"
#include "llfloatergroupinfo.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfloaterbuyland.h"
#include "llfloatergroups.h"
#include "llpanelnearbymedia.h"
#include "llfloatersellland.h"
#include "llfloaterteleporthistory.h"
#include "llfloatertools.h"
#include "llnotify.h"
#include "llparcelselection.h"
#include "llresmgr.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llstatusbar.h"
#include "llui.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerparcelmedia.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "lloverlaybar.h"
#include "roles_constants.h"
#include "llweb.h"
#include "rlvactions.h"
const F32 PARCEL_COLLISION_DRAW_SECS = 1.f;
U8* LLViewerParcelMgr::sPackedOverlay = NULL;
LLUUID gCurrentMovieID = LLUUID::null;
LLPointer<LLViewerTexture> sBlockedImage;
LLPointer<LLViewerTexture> sPassImage;
void optionally_start_music(LLParcel* parcel);
void callback_start_music(S32 option, void* data);
void optionally_prepare_video(const LLParcel *parcelp);
void callback_prepare_video(S32 option, void* data);
void prepare_video(const LLParcel *parcelp);
void start_video(const LLParcel *parcelp);
void stop_video();
bool callback_god_force_owner(const LLSD&, const LLSD&);
struct LLGodForceOwnerData
{
	LLUUID mOwnerID;
	S32 mLocalID;
	LLHost mHost;
	LLGodForceOwnerData(
		const LLUUID& owner_id,
		S32 local_parcel_id,
		const LLHost& host) :
		mOwnerID(owner_id),
		mLocalID(local_parcel_id),
		mHost(host) {}
};
LLViewerParcelMgr::LLViewerParcelMgr()
:	mSelected(FALSE),
	mRequestResult(0),
	mWestSouth(),
	mEastNorth(),
	mSelectedDwell(DWELL_NAN),
	mAgentParcelSequenceID(-1),
	mHoverRequestResult(0),
	mHoverWestSouth(),
	mHoverEastNorth(),
	mCollisionRegionHandle(0),
	mCollisionUpdateSignal(nullptr),
	mRenderCollision(FALSE),
	mRenderSelection(TRUE),
	mCollisionBanned(0),
	mCollisionTimer(),
	mMediaParcelId(0),
	mHighlightSegments(nullptr),
	mCollisionSegments(nullptr),
	mAgentParcelOverlay(nullptr),
	mParcelsPerEdge(0),
	mMediaRegionId(0)
{
	mCurrentParcel = new LLParcel();
	mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);
	mFloatingParcelSelection = new LLParcelSelection(mCurrentParcel);
	mAgentParcel = new LLParcel();
	mHoverParcel = new LLParcel();
	mCollisionParcel = new LLParcel();
	mParcelsPerEdge = S32(8192.f / PARCEL_GRID_STEP_METERS);
	mHighlightSegments = new U8[(mParcelsPerEdge+1)*(mParcelsPerEdge+1)];
	resetSegments(mHighlightSegments);
	mCollisionBitmap = new U8[getCollisionBitmapSize()];
	memset(mCollisionBitmap, 0, getCollisionBitmapSize());
	mCollisionSegments = new U8[(mParcelsPerEdge+1)*(mParcelsPerEdge+1)];
	resetSegments(mCollisionSegments);
	mBlockedImage = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryLines.png");
	mPassImage = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryPassLines.png");
	S32 overlay_size = mParcelsPerEdge * mParcelsPerEdge / PARCEL_OVERLAY_CHUNKS;
	sPackedOverlay = new U8[overlay_size];
	mAgentParcelOverlay = new U8[mParcelsPerEdge * mParcelsPerEdge];
	S32 i;
	for (i = 0; i < mParcelsPerEdge * mParcelsPerEdge; i++)
	{
		mAgentParcelOverlay[i] = 0;
	}
	mParcelsPerEdge = S32(REGION_WIDTH_METERS / PARCEL_GRID_STEP_METERS);
	mTeleportInProgress = TRUE;
}
void LLViewerParcelMgr::init(F32 region_size)
{
	mParcelsPerEdge = S32(region_size / PARCEL_GRID_STEP_METERS);
}
LLViewerParcelMgr::~LLViewerParcelMgr()
{
	mCurrentParcelSelection->setParcel(NULL);
	mCurrentParcelSelection = NULL;
	mFloatingParcelSelection->setParcel(NULL);
	mFloatingParcelSelection = NULL;
	delete mCurrentParcel;
	mCurrentParcel = NULL;
	delete mAgentParcel;
	mAgentParcel = NULL;
	delete mCollisionParcel;
	mCollisionParcel = NULL;
	delete mHoverParcel;
	mHoverParcel = NULL;
	delete[] mHighlightSegments;
	mHighlightSegments = NULL;
	delete[] mCollisionBitmap;
	mCollisionBitmap = NULL;
	delete[] mCollisionSegments;
	mCollisionSegments = NULL;
	delete[] sPackedOverlay;
	sPackedOverlay = NULL;
	delete[] mAgentParcelOverlay;
	mAgentParcelOverlay = NULL;
	sBlockedImage = NULL;
	sPassImage = NULL;
}
void LLViewerParcelMgr::dump()
{
	LL_INFOS() << "Parcel Manager Dump" << LL_ENDL;
	LL_INFOS() << "mSelected " << S32(mSelected) << LL_ENDL;
	LL_INFOS() << "Selected parcel: " << LL_ENDL;
	LL_INFOS() << mWestSouth << " to " << mEastNorth << LL_ENDL;
	mCurrentParcel->dump();
	LL_INFOS() << "banning " << mCurrentParcel->mBanList.size() << LL_ENDL;
	for (const auto& pair : mCurrentParcel->mBanList)
	{
		LL_INFOS() << "ban id " << pair.first << LL_ENDL;
	}
	LL_INFOS() << "Hover parcel:" << LL_ENDL;
	mHoverParcel->dump();
	LL_INFOS() << "Agent parcel:" << LL_ENDL;
	mAgentParcel->dump();
}
LLViewerRegion* LLViewerParcelMgr::getSelectionRegion() const
{
	return LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
}
void LLViewerParcelMgr::getDisplayInfo(S32* area_out, S32* claim_out,
									   S32* rent_out,
									   BOOL* for_sale_out,
									   F32* dwell_out)
{
	S32 area = 0;
	S32 price = 0;
	S32 rent = 0;
	BOOL for_sale = FALSE;
	F32 dwell = DWELL_NAN;
	if (mSelected)
	{
		if (mCurrentParcelSelection->mSelectedMultipleOwners)
		{
			area = mCurrentParcelSelection->getClaimableArea();
		}
		else
		{
			area = getSelectedArea();
		}
		if (mCurrentParcel->getForSale())
		{
			price = mCurrentParcel->getSalePrice();
			for_sale = TRUE;
		}
		else
		{
			price = area * mCurrentParcel->getClaimPricePerMeter();
			for_sale = FALSE;
		}
		rent = mCurrentParcel->getTotalRent();
		dwell = mSelectedDwell;
	}
	*area_out = area;
	*claim_out = price;
	*rent_out = rent;
	*for_sale_out = for_sale;
	*dwell_out = dwell;
}
S32 LLViewerParcelMgr::getSelectedArea() const
{
	S32 rv = 0;
	if(mSelected && mCurrentParcel && mCurrentParcelSelection->mWholeParcelSelected)
	{
		rv = mCurrentParcel->getArea();
	}
	else if(mSelected)
	{
		F64 width = mEastNorth.mdV[VX] - mWestSouth.mdV[VX];
		F64 height = mEastNorth.mdV[VY] - mWestSouth.mdV[VY];
		F32 area = (F32)(width * height);
		rv = ll_round(area);
	}
	return rv;
}
void LLViewerParcelMgr::resetSegments(U8* segments)
{
	S32 i;
	S32 count = (mParcelsPerEdge+1)*(mParcelsPerEdge+1);
	for (i = 0; i < count; i++)
	{
		segments[i] = 0x0;
	}
}
void LLViewerParcelMgr::writeHighlightSegments(F32 west, F32 south, F32 east,
											   F32 north)
{
	S32 x, y;
	S32 min_x = ll_round( west / PARCEL_GRID_STEP_METERS );
	S32 max_x = ll_round( east / PARCEL_GRID_STEP_METERS );
	S32 min_y = ll_round( south / PARCEL_GRID_STEP_METERS );
	S32 max_y = ll_round( north / PARCEL_GRID_STEP_METERS );
	const S32 STRIDE = mParcelsPerEdge+1;
	y = min_y;
	for (x = min_x; x < max_x; x++)
	{
		mHighlightSegments[x + y*STRIDE] ^= SOUTH_MASK;
	}
	x = min_x;
	for (y = min_y; y < max_y; y++)
	{
		mHighlightSegments[x + y*STRIDE] ^= WEST_MASK;
	}
	y = max_y;
	for (x = min_x; x < max_x; x++)
	{
		mHighlightSegments[x + y*STRIDE] ^= SOUTH_MASK;
	}
	x = max_x;
	for (y = min_y; y < max_y; y++)
	{
		mHighlightSegments[x + y*STRIDE] ^= WEST_MASK;
	}
}
void LLViewerParcelMgr::writeSegmentsFromBitmap(U8* bitmap, U8* segments)
{
	S32 x;
	S32 y;
	const S32 IN_STRIDE = mParcelsPerEdge;
	const S32 OUT_STRIDE = mParcelsPerEdge+1;
	for (y = 0; y < IN_STRIDE; y++)
	{
		x = 0;
		while( x < IN_STRIDE )
		{
			U8 byte = bitmap[ (x + y*IN_STRIDE) / 8 ];
			S32 bit;
			for (bit = 0; bit < 8; bit++)
			{
				if (byte & (1 << bit) )
				{
					S32 out = x+y*OUT_STRIDE;
					segments[out]            ^= SOUTH_MASK;
					segments[out+OUT_STRIDE] ^= SOUTH_MASK;
					segments[out]   ^= WEST_MASK;
					segments[out+1] ^= WEST_MASK;
				}
				x++;
			}
		}
	}
}
void LLViewerParcelMgr::writeAgentParcelFromBitmap(U8* bitmap)
{
	S32 x;
	S32 y;
	const S32 IN_STRIDE = mParcelsPerEdge;
	for (y = 0; y < IN_STRIDE; y++)
	{
		x = 0;
		while( x < IN_STRIDE )
		{
			U8 byte = bitmap[ (x + y*IN_STRIDE) / 8 ];
			S32 bit;
			for (bit = 0; bit < 8; bit++)
			{
				if (byte & (1 << bit) )
				{
					mAgentParcelOverlay[x+y*IN_STRIDE] = 1;
				}
				else
				{
					mAgentParcelOverlay[x+y*IN_STRIDE] = 0;
				}
				x++;
			}
		}
	}
}
LLParcelSelectionHandle LLViewerParcelMgr::selectParcelAt(const LLVector3d& pos_global)
{
	LLVector3d southwest = pos_global;
	LLVector3d northeast = pos_global;
	southwest -= LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );
	southwest.mdV[VX] = ll_round( southwest.mdV[VX], (F64)PARCEL_GRID_STEP_METERS );
	southwest.mdV[VY] = ll_round( southwest.mdV[VY], (F64)PARCEL_GRID_STEP_METERS );
	northeast += LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );
	northeast.mdV[VX] = ll_round( northeast.mdV[VX], (F64)PARCEL_GRID_STEP_METERS );
	northeast.mdV[VY] = ll_round( northeast.mdV[VY], (F64)PARCEL_GRID_STEP_METERS );
	return selectLand( southwest, northeast, TRUE );
}
LLParcelSelectionHandle LLViewerParcelMgr::selectParcelInRectangle()
{
	return selectLand(mWestSouth, mEastNorth, TRUE);
}
void LLViewerParcelMgr::selectCollisionParcel()
{
	mWestSouth = getSelectionRegion()->getOriginGlobal();
	mEastNorth = mWestSouth;
	mEastNorth += LLVector3d(getSelectionRegion()->getWidth()/REGION_WIDTH_METERS * PARCEL_GRID_STEP_METERS, getSelectionRegion()->getWidth()/REGION_WIDTH_METERS * PARCEL_GRID_STEP_METERS, 0.0);
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelPropertiesRequestByID);
	msg->nextBlockFast(_PREHASH_AgentID);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_SequenceID, SELECTED_PARCEL_SEQ_ID );
	msg->addS32Fast(_PREHASH_LocalID, mCollisionParcel->getLocalID() );
	gAgent.sendReliableMessage();
	mRequestResult = PARCEL_RESULT_NO_DATA;
	mCurrentParcel->setName( mCollisionParcel->getName() );
	mCurrentParcel->setDesc( mCollisionParcel->getDesc() );
	mCurrentParcel->setPassPrice(mCollisionParcel->getPassPrice());
	mCurrentParcel->setPassHours(mCollisionParcel->getPassHours());
	resetSegments(mHighlightSegments);
	mFloatingParcelSelection->setParcel(mCurrentParcel);
	mCurrentParcelSelection->setParcel(NULL);
	mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);
	mSelected = TRUE;
	mCurrentParcelSelection->mWholeParcelSelected = TRUE;
	notifyObservers();
	return;
}
LLParcelSelectionHandle LLViewerParcelMgr::selectLand(const LLVector3d &corner1, const LLVector3d &corner2,
								   BOOL snap_selection)
{
	sanitize_corners( corner1, corner2, mWestSouth, mEastNorth );
	F32 delta_x = getSelectionWidth();
	if (delta_x * delta_x <= 1.f * 1.f)
	{
		mSelected = FALSE;
		notifyObservers();
		return NULL;
	}
	F32 delta_y = getSelectionHeight();
	if (delta_y * delta_y <= 1.f * 1.f)
	{
		mSelected = FALSE;
		notifyObservers();
		return NULL;
	}
	LLVector3d east_north_region_check( mEastNorth );
	east_north_region_check.mdV[VX] -= 0.5;
	east_north_region_check.mdV[VY] -= 0.5;
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal(mWestSouth);
	LLViewerRegion *region_other = LLWorld::getInstance()->getRegionFromPosGlobal( east_north_region_check );
	if(!region)
	{
		mSelected = FALSE;
		return NULL;
	}
	if (region != region_other)
	{
		LLNotificationsUtil::add("CantSelectLandFromMultipleRegions");
		mSelected = FALSE;
		notifyObservers();
		return NULL;
	}
	LLVector3 wsb_region = region->getPosRegionFromGlobal( mWestSouth );
	LLVector3 ent_region = region->getPosRegionFromGlobal( mEastNorth );
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelPropertiesRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_SequenceID, SELECTED_PARCEL_SEQ_ID );
	msg->addF32Fast(_PREHASH_West,  wsb_region.mV[VX] );
	msg->addF32Fast(_PREHASH_South, wsb_region.mV[VY] );
	msg->addF32Fast(_PREHASH_East,  ent_region.mV[VX] );
	msg->addF32Fast(_PREHASH_North, ent_region.mV[VY] );
	msg->addBOOL("SnapSelection", snap_selection);
	msg->sendReliable( region->getHost() );
	mRequestResult = PARCEL_RESULT_NO_DATA;
	resetSegments(mHighlightSegments);
	mFloatingParcelSelection->setParcel(mCurrentParcel);
	mCurrentParcelSelection->setParcel(NULL);
	mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);
	mSelected = TRUE;
	mCurrentParcelSelection->mWholeParcelSelected = snap_selection;
	notifyObservers();
	return mCurrentParcelSelection;
}
void LLViewerParcelMgr::deselectUnused()
{
	if (mCurrentParcelSelection->getNumRefs() == 1 && mFloatingParcelSelection->getNumRefs() == 1)
	{
		deselectLand();
	}
}
void LLViewerParcelMgr::deselectLand()
{
	if (mSelected)
	{
		mSelected = FALSE;
		mCurrentParcel->setLocalID(-1);
		mCurrentParcel->mAccessList.clear();
		mCurrentParcel->mBanList.clear();
		mSelectedDwell = DWELL_NAN;
		mCurrentParcelSelection->setParcel(NULL);
		mFloatingParcelSelection->setParcel(NULL);
		mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);
		notifyObservers();
	}
}
void LLViewerParcelMgr::addObserver(LLParcelObserver* observer)
{
	mObservers.push_back(observer);
}
void LLViewerParcelMgr::removeObserver(LLParcelObserver* observer)
{
	vector_replace_with_last(mObservers, observer);
}
void LLViewerParcelMgr::notifyObservers()
{
	std::vector<LLParcelObserver*> observers;
	S32 count = mObservers.size();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		observers.push_back(mObservers.at(i));
	}
	for(i = 0; i < count; ++i)
	{
		observers.at(i)->changed();
	}
}
BOOL LLViewerParcelMgr::selectionEmpty() const
{
	return !mSelected;
}
LLParcelSelectionHandle LLViewerParcelMgr::getParcelSelection() const
{
	return mCurrentParcelSelection;
}
LLParcelSelectionHandle LLViewerParcelMgr::getFloatingParcelSelection() const
{
	return mFloatingParcelSelection;
}
LLParcel *LLViewerParcelMgr::getAgentParcel() const
{
	return mAgentParcel;
}
bool LLViewerParcelMgr::allowAgentBuild() const
{
	if (mAgentParcel)
	{
		return (gAgent.isGodlike() ||
				(mAgentParcel->allowModifyBy(gAgent.getID(), gAgent.getGroupID())) ||
				(isParcelOwnedByAgent(mAgentParcel, GP_LAND_ALLOW_CREATE)));
	}
	else
	{
		return gAgent.isGodlike();
	}
}
bool LLViewerParcelMgr::allowAgentBuild(const LLParcel* parcel) const
{
	return parcel->getAllowModify();
}
bool LLViewerParcelMgr::allowAgentVoice() const
{
	return allowAgentVoice(gAgent.getRegion(), mAgentParcel);
}
bool LLViewerParcelMgr::allowAgentVoice(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return region && region->isVoiceEnabled()
		&& parcel	&& parcel->getParcelFlagAllowVoice();
}
bool LLViewerParcelMgr::allowAgentFly(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return region && !region->getBlockFly()
		&& parcel && parcel->getAllowFly();
}
bool LLViewerParcelMgr::allowAgentPush(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return region && !region->getRestrictPushObject()
		&& parcel && !parcel->getRestrictPushObject();
}
bool LLViewerParcelMgr::allowAgentScripts(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return region
		&& !region->getRegionFlag(REGION_FLAGS_SKIP_SCRIPTS)
		&& !region->getRegionFlag(REGION_FLAGS_ESTATE_SKIP_SCRIPTS)
		&& parcel
		&& parcel->getAllowOtherScripts();
}
bool LLViewerParcelMgr::allowAgentDamage(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return (region && region->getAllowDamage())
		|| (parcel && parcel->getAllowDamage());
}
BOOL LLViewerParcelMgr::isOwnedAt(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;
	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;
	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );
	return overlay->isOwned( pos_region );
}
BOOL LLViewerParcelMgr::isOwnedSelfAt(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;
	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;
	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );
	return overlay->isOwnedSelf( pos_region );
}
BOOL LLViewerParcelMgr::isOwnedOtherAt(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;
	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;
	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );
	return overlay->isOwnedOther( pos_region );
}
BOOL LLViewerParcelMgr::isSoundLocal(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;
	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;
	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );
	return overlay->isSoundLocal( pos_region );
}
BOOL LLViewerParcelMgr::canHearSound(const LLVector3d &pos_global) const
{
	BOOL in_agent_parcel = inAgentParcel(pos_global);
	if (in_agent_parcel)
	{
		return TRUE;
	}
	else
	{
		if (LLViewerParcelMgr::getInstance()->getAgentParcel()->getSoundLocal())
		{
			return FALSE;
		}
		else if (LLViewerParcelMgr::getInstance()->isSoundLocal(pos_global))
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
}
BOOL LLViewerParcelMgr::inAgentParcel(const LLVector3d &pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(pos_global);
	LLViewerRegion* agent_region = gAgent.getRegion();
	if (!region || !agent_region)
		return FALSE;
	if (region != agent_region)
	{
		return FALSE;
	}
	LLVector3 pos_region = agent_region->getPosRegionFromGlobal(pos_global);
	S32 row =    S32(pos_region.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos_region.mV[VX] / PARCEL_GRID_STEP_METERS);
	if (mAgentParcelOverlay[row*mParcelsPerEdge + column])
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
LLParcel* LLViewerParcelMgr::getHoverParcel() const
{
	if (mHoverRequestResult == PARCEL_RESULT_SUCCESS)
	{
		return mHoverParcel;
	}
	else
	{
		return NULL;
	}
}
LLParcel* LLViewerParcelMgr::getCollisionParcel() const
{
	if (mRenderCollision)
	{
		return mCollisionParcel;
	}
	else
	{
		return NULL;
	}
}
void LLViewerParcelMgr::render()
{
	static const LLCachedControl<bool> render_parcel_selection(gSavedSettings, "RenderParcelSelection");
	if (mSelected && mRenderSelection && render_parcel_selection)
	{
		LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosGlobal(mWestSouth);
		if (!regionp) return;
		renderHighlightSegments(mHighlightSegments, regionp);
	}
}
void LLViewerParcelMgr::renderParcelCollision()
{
	if (mCollisionTimer.getElapsedTimeF32() > PARCEL_COLLISION_DRAW_SECS)
	{
		mRenderCollision = FALSE;
	}
	static const LLCachedControl<bool> render_ban_line(gSavedSettings, "ShowBanLines");
	if (mRenderCollision && render_ban_line)
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		if (regionp)
		{
			BOOL use_pass = mCollisionParcel->getParcelFlag(PF_USE_PASS_LIST);
			renderCollisionSegments(mCollisionSegments, use_pass, regionp);
		}
	}
}
void LLViewerParcelMgr::sendParcelAccessListRequest(U32 flags)
{
	if (!mSelected)
	{
		return;
	}
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;
	LLMessageSystem *msg = gMessageSystem;
	if (flags & AL_BAN)
	{
		mCurrentParcel->mBanList.clear();
	}
	if (flags & AL_ACCESS)
	{
		mCurrentParcel->mAccessList.clear();
	}
	if (flags & AL_ALLOW_EXPERIENCE)
	{
		mCurrentParcel->clearExperienceKeysByType(EXPERIENCE_KEY_TYPE_ALLOWED);
	}
	if (flags & AL_BLOCK_EXPERIENCE)
	{
		mCurrentParcel->clearExperienceKeysByType(EXPERIENCE_KEY_TYPE_BLOCKED);
	}
	msg->newMessageFast(_PREHASH_ParcelAccessListRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addS32Fast(_PREHASH_SequenceID, 0);
	msg->addU32Fast(_PREHASH_Flags, flags);
	msg->addS32("LocalID", mCurrentParcel->getLocalID() );
	msg->sendReliable( region->getHost() );
}
void LLViewerParcelMgr::sendParcelDwellRequest()
{
	if (!mSelected)
	{
		return;
	}
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessage("ParcelDwellRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addS32("LocalID", mCurrentParcel->getLocalID());
	msg->addUUID("ParcelID", LLUUID::null);
	msg->sendReliable( region->getHost() );
}
void LLViewerParcelMgr::sendParcelGodForceOwner(const LLUUID& owner_id)
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotSetLandOwnerNothingSelected");
		return;
	}
	LL_INFOS() << "Claiming " << mWestSouth << " to " << mEastNorth << LL_ENDL;
	LLVector3d east_north_region_check( mEastNorth );
	east_north_region_check.mdV[VX] -= 0.5;
	east_north_region_check.mdV[VY] -= 0.5;
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotContentifyNoRegion");
		return;
	}
	LLViewerRegion *region2 = LLWorld::getInstance()->getRegionFromPosGlobal( east_north_region_check );
	if (region != region2)
	{
		LLNotificationsUtil::add("CannotSetLandOwnerMultipleRegions");
		return;
	}
	LL_INFOS() << "Region " << region->getOriginGlobal() << LL_ENDL;
	LLSD payload;
	payload["owner_id"] = owner_id;
	payload["parcel_local_id"] = mCurrentParcel->getLocalID();
	payload["region_host"] = region->getHost().getIPandPort();
	LLNotification::Params params("ForceOwnerAuctionWarning");
	params.payload(payload).functor(callback_god_force_owner);
	if(mCurrentParcel->getAuctionID())
	{
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
}
bool callback_god_force_owner(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelGodForceOwner");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("OwnerID", notification["payload"]["owner_id"].asUUID());
		msg->addS32( "LocalID", notification["payload"]["parcel_local_id"].asInteger());
		msg->sendReliable(LLHost(notification["payload"]["region_host"].asString()));
	}
	return false;
}
void LLViewerParcelMgr::sendParcelGodForceToContent()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotContentifyNothingSelected");
		return;
	}
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotContentifyNoRegion");
		return;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("ParcelGodMarkAsContent");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("ParcelData");
	msg->addS32("LocalID", mCurrentParcel->getLocalID());
	msg->sendReliable(region->getHost());
}
void LLViewerParcelMgr::sendParcelRelease()
{
	if (!mSelected)
	{
        LLNotificationsUtil::add("CannotReleaseLandNothingSelected");
		return;
	}
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotReleaseLandNoRegion");
		return;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("ParcelRelease");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID() );
	msg->nextBlock("Data");
	msg->addS32("LocalID", mCurrentParcel->getLocalID() );
	msg->sendReliable( region->getHost() );
	deselectLand();
}
class LLViewerParcelMgr::ParcelBuyInfo
{
public:
	LLUUID	mAgent;
	LLUUID	mSession;
	LLUUID	mGroup;
	BOOL	mIsGroupOwned;
	BOOL	mRemoveContribution;
	BOOL	mIsClaim;
	LLHost	mHost;
	S32		mParcelID;
	S32		mPrice;
	S32		mArea;
	F32		mWest;
	F32		mSouth;
	F32		mEast;
	F32		mNorth;
};
LLViewerParcelMgr::ParcelBuyInfo* LLViewerParcelMgr::setupParcelBuy(
	const LLUUID& agent_id,
	const LLUUID& session_id,
	const LLUUID& group_id,
	BOOL is_group_owned,
	BOOL is_claim,
	BOOL remove_contribution)
{
	if (!mSelected || !mCurrentParcel)
	{
		LLNotificationsUtil::add("CannotBuyLandNothingSelected");
		return NULL;
	}
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotBuyLandNoRegion");
		return NULL;
	}
	if (is_claim)
	{
		LL_INFOS() << "Claiming " << mWestSouth << " to " << mEastNorth << LL_ENDL;
		LL_INFOS() << "Region " << region->getOriginGlobal() << LL_ENDL;
		LLVector3d east_north_region_check( mEastNorth );
		east_north_region_check.mdV[VX] -= 0.5;
		east_north_region_check.mdV[VY] -= 0.5;
		LLViewerRegion *region2 = LLWorld::getInstance()->getRegionFromPosGlobal( east_north_region_check );
		if (region != region2)
		{
			LLNotificationsUtil::add("CantBuyLandAcrossMultipleRegions");
			return NULL;
		}
	}
	ParcelBuyInfo* info = new ParcelBuyInfo;
	info->mAgent = agent_id;
	info->mSession = session_id;
	info->mGroup = group_id;
	info->mIsGroupOwned = is_group_owned;
	info->mIsClaim = is_claim;
	info->mRemoveContribution = remove_contribution;
	info->mHost = region->getHost();
	info->mPrice = mCurrentParcel->getSalePrice();
	info->mArea = mCurrentParcel->getArea();
	if (!is_claim)
	{
		info->mParcelID = mCurrentParcel->getLocalID();
	}
	else
	{
		LLVector3 west_south_bottom_region = region->getPosRegionFromGlobal( mWestSouth );
		LLVector3 east_north_top_region = region->getPosRegionFromGlobal( mEastNorth );
		info->mWest		= west_south_bottom_region.mV[VX];
		info->mSouth	= west_south_bottom_region.mV[VY];
		info->mEast		= east_north_top_region.mV[VX];
		info->mNorth	= east_north_top_region.mV[VY];
	}
	return info;
}
void LLViewerParcelMgr::sendParcelBuy(ParcelBuyInfo* info)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage(info->mIsClaim ? "ParcelClaim" : "ParcelBuy");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", info->mAgent);
	msg->addUUID("SessionID", info->mSession);
	msg->nextBlock("Data");
	msg->addUUID("GroupID", info->mGroup);
	msg->addBOOL("IsGroupOwned", info->mIsGroupOwned);
	if (!info->mIsClaim)
	{
		msg->addBOOL("RemoveContribution", info->mRemoveContribution);
		msg->addS32("LocalID", info->mParcelID);
	}
	msg->addBOOL("Final", TRUE);
	if (info->mIsClaim)
	{
		msg->nextBlock("ParcelData");
		msg->addF32("West",  info->mWest);
		msg->addF32("South", info->mSouth);
		msg->addF32("East",  info->mEast);
		msg->addF32("North", info->mNorth);
	}
	else
	{
		msg->nextBlock("ParcelData");
		msg->addS32("Price",info->mPrice);
		msg->addS32("Area",info->mArea);
	}
	msg->sendReliable(info->mHost);
}
void LLViewerParcelMgr::deleteParcelBuy(ParcelBuyInfo* *info)
{
	delete *info;
	*info = NULL;
}
void LLViewerParcelMgr::sendParcelDeed(const LLUUID& group_id)
{
	if (!mSelected || !mCurrentParcel)
	{
		LLNotificationsUtil::add("CannotDeedLandNothingSelected");
		return;
	}
	if(group_id.isNull())
	{
		LLNotificationsUtil::add("CannotDeedLandNoGroup");
		return;
	}
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotDeedLandNoRegion");
		return;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("ParcelDeedToGroup");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID() );
	msg->nextBlock("Data");
	msg->addUUID("GroupID", group_id );
	msg->addS32("LocalID", mCurrentParcel->getLocalID() );
	msg->sendReliable( region->getHost() );
}
const std::string& LLViewerParcelMgr::getAgentParcelName() const
{
	return mAgentParcel->getName();
}
void LLViewerParcelMgr::sendParcelPropertiesUpdate(LLParcel* parcel, bool use_agent_region)
{
	if (!parcel)
        return;
	LLViewerRegion *region = use_agent_region ? gAgent.getRegion() : LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
        return;
	LLSD body;
	std::string url = region->getCapability("ParcelPropertiesUpdate");
	if (!url.empty())
	{
		U32 message_flags = 0x01;
		body["flags"] = ll_sd_from_U32(message_flags);
		parcel->packMessage(body);
		LL_INFOS() << "Sending parcel properties update via capability to: "
			<< url << LL_ENDL;
		LLHTTPClient::post(url, body, new LLHTTPClient::ResponderIgnore);
	}
	else
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ParcelPropertiesUpdate);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ParcelData);
		msg->addS32Fast(_PREHASH_LocalID, parcel->getLocalID() );
		U32 message_flags = 0x01;
		msg->addU32("Flags", message_flags);
		parcel->packMessage(msg);
		msg->sendReliable( region->getHost() );
	}
}
void LLViewerParcelMgr::setHoverParcel(const LLVector3d& pos)
{
	static U32 last_west, last_south;
	if (!gSavedSettings.getBOOL("ShowLandHoverTip")) return;
	U32 west_parcel_step = (U32) floor( pos.mdV[VX] / PARCEL_GRID_STEP_METERS );
	U32 south_parcel_step = (U32) floor( pos.mdV[VY] / PARCEL_GRID_STEP_METERS );
	if ((west_parcel_step == last_west) && (south_parcel_step == last_south))
	{
		return;
	}
	else
	{
		last_west = west_parcel_step;
		last_south = south_parcel_step;
	}
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos );
	if (!region)
	{
		return;
	}
	LLVector3 wsb_region = region->getPosRegionFromGlobal( pos );
	F32 west  = PARCEL_GRID_STEP_METERS * floor( wsb_region.mV[VX] / PARCEL_GRID_STEP_METERS );
	F32 south = PARCEL_GRID_STEP_METERS * floor( wsb_region.mV[VY] / PARCEL_GRID_STEP_METERS );
	F32 east  = west  + PARCEL_GRID_STEP_METERS;
	F32 north = south + PARCEL_GRID_STEP_METERS;
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelPropertiesRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_SequenceID,	HOVERED_PARCEL_SEQ_ID );
	msg->addF32Fast(_PREHASH_West,			west );
	msg->addF32Fast(_PREHASH_South,			south );
	msg->addF32Fast(_PREHASH_East,			east );
	msg->addF32Fast(_PREHASH_North,			north );
	msg->addBOOL("SnapSelection",			FALSE );
	msg->sendReliable( region->getHost() );
	mHoverRequestResult = PARCEL_RESULT_NO_DATA;
}
void LLViewerParcelMgr::processParcelOverlay(LLMessageSystem *msg, void **user)
{
	if (gNoRender)
	{
		return;
	}
	S32 packed_overlay_size = msg->getSizeFast(_PREHASH_ParcelData, _PREHASH_Data);
	if (packed_overlay_size <= 0)
	{
		LL_WARNS() << "Overlay size " << packed_overlay_size << LL_ENDL;
		return;
	}
	S32 expected_size = 1024;
	if (packed_overlay_size != expected_size)
	{
		LL_WARNS() << "Got parcel overlay size " << packed_overlay_size
			<< " expecting " << expected_size << LL_ENDL;
		return;
	}
	S32 sequence_id;
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SequenceID, sequence_id);
	msg->getBinaryDataFast(
			_PREHASH_ParcelData,
			_PREHASH_Data,
			sPackedOverlay,
			expected_size);
	LLHost host = msg->getSender();
	LLViewerRegion *region = LLWorld::getInstance()->getRegion(host);
	if (region)
	{
		region->mParcelOverlay->uncompressLandOverlay( sequence_id, sPackedOverlay );
	}
}
void LLViewerParcelMgr::processParcelProperties(LLMessageSystem *msg, void **user)
{
	S32		request_result;
	S32		sequence_id;
	BOOL	snap_selection = FALSE;
	S32		self_count = 0;
	S32		other_count = 0;
	S32		public_count = 0;
	S32		local_id;
	LLUUID	owner_id;
	BOOL	is_group_owned;
	U32 auction_id = 0;
	S32		claim_price_per_meter = 0;
	S32		rent_price_per_meter = 0;
	S32		claim_date = 0;
	LLVector3	aabb_min;
	LLVector3	aabb_max;
	S32		area = 0;
	S32		sw_max_prims = 0;
	S32		sw_total_prims = 0;
	U8 status = 0;
	S32		max_prims = 0;
	S32		total_prims = 0;
	S32		owner_prims = 0;
	S32		group_prims = 0;
	S32		other_prims = 0;
	S32		selected_prims = 0;
	F32		parcel_prim_bonus = 1.f;
	BOOL	region_push_override = false;
	BOOL	region_deny_anonymous_override = false;
	BOOL	region_deny_identified_override = false;
	BOOL	region_deny_transacted_override = false;
	BOOL	region_deny_age_unverified_override = false;
	S32		other_clean_time = 0;
	LLViewerParcelMgr& parcel_mgr = LLViewerParcelMgr::instance();
	LLViewerRegion* msg_region = LLWorld::getInstance()->getRegion(msg->getSender());
	if(msg_region)
		parcel_mgr.mParcelsPerEdge = S32(msg_region->getWidth() / PARCEL_GRID_STEP_METERS);
	else
		parcel_mgr.mParcelsPerEdge = S32(gAgent.getRegion()->getWidth() / PARCEL_GRID_STEP_METERS);
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_RequestResult, request_result );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SequenceID, sequence_id );
	if (request_result == PARCEL_RESULT_NO_DATA)
	{
		LL_INFOS() << "no valid parcel data" << LL_ENDL;
		return;
	}
	LLParcel* parcel = NULL;
	if (sequence_id == SELECTED_PARCEL_SEQ_ID)
	{
		parcel_mgr.mRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = parcel_mgr.mCurrentParcel;
	}
	else if (sequence_id == HOVERED_PARCEL_SEQ_ID)
	{
		parcel_mgr.mHoverRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = parcel_mgr.mHoverParcel;
	}
	else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_NOT_ON_LIST_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
	{
		parcel_mgr.mHoverRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = parcel_mgr.mCollisionParcel;
	}
	else if (sequence_id == 0 || sequence_id > parcel_mgr.mAgentParcelSequenceID)
	{
		parcel_mgr.mAgentParcelSequenceID = sequence_id;
		parcel = parcel_mgr.mAgentParcel;
	}
	else
	{
		LL_INFOS() << "out of order agent parcel sequence id " << sequence_id
			<< " last good " << parcel_mgr.mAgentParcelSequenceID
			<< LL_ENDL;
		return;
	}
	msg->getBOOL("ParcelData", "SnapSelection", snap_selection);
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SelfCount, self_count);
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_OtherCount, other_count);
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_PublicCount, public_count);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_LocalID,		local_id );
	msg->getUUIDFast(_PREHASH_ParcelData, _PREHASH_OwnerID,		owner_id);
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_IsGroupOwned, is_group_owned);
	msg->getU32Fast(_PREHASH_ParcelData, _PREHASH_AuctionID, auction_id);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_ClaimDate,	claim_date);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_ClaimPrice,	claim_price_per_meter);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_RentPrice,	rent_price_per_meter);
	msg->getVector3Fast(_PREHASH_ParcelData, _PREHASH_AABBMin, aabb_min);
	msg->getVector3Fast(_PREHASH_ParcelData, _PREHASH_AABBMax, aabb_max);
	msg->getS32Fast(	_PREHASH_ParcelData, _PREHASH_Area, area );
	msg->getU8("ParcelData", "Status", status);
	msg->getS32("ParcelData", "SimWideMaxPrims", sw_max_prims );
	msg->getS32("ParcelData", "SimWideTotalPrims", sw_total_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_MaxPrims, max_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_TotalPrims, total_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_OwnerPrims, owner_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_GroupPrims, group_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_OtherPrims, other_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SelectedPrims, selected_prims );
	msg->getF32Fast(_PREHASH_ParcelData, _PREHASH_ParcelPrimBonus, parcel_prim_bonus );
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionPushOverride, region_push_override );
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionDenyAnonymous, region_deny_anonymous_override );
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionDenyIdentified, region_deny_identified_override );
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionDenyTransacted, region_deny_transacted_override );
	if (msg->getNumberOfBlocksFast(_PREHASH_AgeVerificationBlock))
	{
		msg->getBOOLFast(_PREHASH_AgeVerificationBlock, _PREHASH_RegionDenyAgeUnverified, region_deny_age_unverified_override );
	}
	msg->getS32("ParcelData", "OtherCleanTime", other_clean_time );
	if (parcel)
	{
		parcel->init(owner_id,
			FALSE, FALSE, FALSE,
			claim_date, claim_price_per_meter, rent_price_per_meter,
			area, other_prims, parcel_prim_bonus, is_group_owned);
		parcel->setLocalID(local_id);
		parcel->setAABBMin(aabb_min);
		parcel->setAABBMax(aabb_max);
		parcel->setAuctionID(auction_id);
		parcel->setOwnershipStatus((LLParcel::EOwnershipStatus)status);
		parcel->setSimWideMaxPrimCapacity(sw_max_prims);
		parcel->setSimWidePrimCount(sw_total_prims);
		parcel->setMaxPrimCapacity(max_prims);
		parcel->setOwnerPrimCount(owner_prims);
		parcel->setGroupPrimCount(group_prims);
		parcel->setOtherPrimCount(other_prims);
		parcel->setSelectedPrimCount(selected_prims);
		parcel->setParcelPrimBonus(parcel_prim_bonus);
		parcel->setCleanOtherTime(other_clean_time);
		parcel->setRegionPushOverride(region_push_override);
		parcel->setRegionDenyAnonymousOverride(region_deny_anonymous_override);
		parcel->setRegionDenyAgeUnverifiedOverride(region_deny_age_unverified_override);
		parcel->unpackMessage(msg);
		if (parcel == parcel_mgr.mAgentParcel)
		{
			S32 bitmap_size =	parcel_mgr.mParcelsPerEdge
								* parcel_mgr.mParcelsPerEdge
								/ 8;
			U8* bitmap = new U8[ bitmap_size ];
			msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);
			parcel_mgr.writeAgentParcelFromBitmap(bitmap);
			delete[] bitmap;
			LLViewerParcelMgr* instance = LLViewerParcelMgr::getInstance();
			instance->mAgentParcelChangedSignal();
			if (instance->mTeleportInProgress)
			{
				instance->mTeleportInProgress = FALSE;
				instance->mTeleportFinishedSignal(gAgent.getPositionGlobal());
			}
		}
	}
	LLFloaterTeleportHistory::getInstance()->addEntry(LLViewerParcelMgr::getInstance()->getAgentParcelName());
	if (sequence_id == SELECTED_PARCEL_SEQ_ID)
	{
		parcel_mgr.mCurrentParcelSelection->mSelectedSelfCount = self_count;
		parcel_mgr.mCurrentParcelSelection->mSelectedOtherCount = other_count;
		parcel_mgr.mCurrentParcelSelection->mSelectedPublicCount = public_count;
		parcel_mgr.mCurrentParcelSelection->mSelectedMultipleOwners =
							(request_result == PARCEL_RESULT_MULTIPLE);
		LLViewerRegion* region = LLWorld::getInstance()->getRegion( msg->getSender() );
		if (region)
		{
			if (!snap_selection)
			{
				LLVector3 west_south = region->getPosRegionFromGlobal(parcel_mgr.mWestSouth);
				LLVector3 east_north = region->getPosRegionFromGlobal(parcel_mgr.mEastNorth);
				parcel_mgr.resetSegments(parcel_mgr.mHighlightSegments);
				parcel_mgr.writeHighlightSegments(
								west_south.mV[VX],
								west_south.mV[VY],
								east_north.mV[VX],
								east_north.mV[VY] );
				parcel_mgr.mCurrentParcelSelection->mWholeParcelSelected = FALSE;
			}
			else if (0 == local_id)
			{
				parcel_mgr.mWestSouth = region->getPosGlobalFromRegion( aabb_min );
				parcel_mgr.mEastNorth = region->getPosGlobalFromRegion( aabb_max );
				parcel_mgr.resetSegments(parcel_mgr.mHighlightSegments);
				parcel_mgr.writeHighlightSegments(
								aabb_min.mV[VX],
								aabb_min.mV[VY],
								aabb_max.mV[VX],
								aabb_max.mV[VY] );
				parcel_mgr.mCurrentParcelSelection->mWholeParcelSelected = TRUE;
			}
			else
			{
				parcel_mgr.mWestSouth = region->getPosGlobalFromRegion( aabb_min );
				parcel_mgr.mEastNorth = region->getPosGlobalFromRegion( aabb_max );
				S32 bitmap_size =	parcel_mgr.mParcelsPerEdge
									* parcel_mgr.mParcelsPerEdge
									/ 8;
				U8* bitmap = new U8[ bitmap_size ];
				msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);
				parcel_mgr.resetSegments(parcel_mgr.mHighlightSegments);
				parcel_mgr.writeSegmentsFromBitmap( bitmap, parcel_mgr.mHighlightSegments );
				delete[] bitmap;
				bitmap = NULL;
				parcel_mgr.mCurrentParcelSelection->mWholeParcelSelected = TRUE;
			}
			parcel_mgr.sendParcelAccessListRequest(AL_ACCESS | AL_BAN | AL_ALLOW_EXPERIENCE | AL_BLOCK_EXPERIENCE);
			parcel_mgr.mSelectedDwell = DWELL_NAN;
			if (0 != local_id)
			{
				parcel_mgr.sendParcelDwellRequest();
			}
			parcel_mgr.mSelected = TRUE;
			parcel_mgr.notifyObservers();
		}
	}
	else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_NOT_ON_LIST_PARCEL_SEQ_ID  ||
			 sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
	{
		parcel_mgr.mRenderCollision = TRUE;
		parcel_mgr.mCollisionTimer.reset();
		if (sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
		{
			parcel_mgr.mCollisionBanned = BA_BANNED;
		}
		else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID)
		{
			parcel_mgr.mCollisionBanned = BA_NOT_IN_GROUP;
		}
		else
		{
			parcel_mgr.mCollisionBanned = BA_NOT_ON_LIST;
		}
		msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, parcel_mgr.mCollisionBitmap, parcel_mgr.getCollisionBitmapSize());
		parcel_mgr.resetSegments(parcel_mgr.mCollisionSegments);
		parcel_mgr.writeSegmentsFromBitmap(parcel_mgr.mCollisionBitmap, parcel_mgr.mCollisionSegments);
		LLViewerRegion* pRegion = LLWorld::getInstance()->getRegion(msg->getSender());
		parcel_mgr.mCollisionRegionHandle = (pRegion) ? pRegion->getHandle() : 0;
		if (parcel_mgr.mCollisionUpdateSignal)
			(*parcel_mgr.mCollisionUpdateSignal)(pRegion);
	}
	else if (sequence_id == HOVERED_PARCEL_SEQ_ID)
	{
		LLViewerRegion *region = LLWorld::getInstance()->getRegion( msg->getSender() );
		if (region)
		{
			parcel_mgr.mHoverWestSouth = region->getPosGlobalFromRegion( aabb_min );
			parcel_mgr.mHoverEastNorth = region->getPosGlobalFromRegion( aabb_max );
		}
		else
		{
			parcel_mgr.mHoverWestSouth.clearVec();
			parcel_mgr.mHoverEastNorth.clearVec();
		}
	}
	else
	{
		LLViewerParcelMedia::update(parcel);
		if (gAudiop)
		{
			if (parcel)
			{
				std::string music_url_raw = parcel->getMusicURL();
				std::string music_url = music_url_raw;
				LLStringUtil::trim(music_url);
				const std::string& stream_url = gAudiop->getInternetStreamURL();
				if (music_url.empty() || music_url != stream_url)
				{
					gAudiop->stopInternetStream();
					if (music_url.size() > 12)
					{
						if (music_url.substr(0,7) == "http://"
							|| music_url.substr(0, 8) == "https://")
						{
							optionally_start_music(parcel);
						}
						else
						{
							LL_INFOS() << "Stopping parcel music (invalid audio stream URL)" << LL_ENDL;
							gAudiop->startInternetStream(LLStringUtil::null);
						}
					}
					else if (!gAudiop->getInternetStreamURL().empty())
					{
						LL_INFOS() << "Stopping parcel music (parcel stream URL is empty)" << LL_ENDL;
						gAudiop->startInternetStream(LLStringUtil::null);
					}
				}
			}
			else
			{
				gAudiop->stopInternetStream();
			}
		}
	};
}
void optionally_start_music(LLParcel* parcel)
{
	if (gSavedSettings.getBOOL("AudioStreamingMusic"))
	{
		if (gOverlayBar && gOverlayBar->musicPlaying())
		{
			LLPanelNearByMedia* nearby_media_panel = LLFloaterNearbyMedia::instanceExists() ? LLFloaterNearbyMedia::getInstance()->getMediaPanel() : nullptr;
			if ((nearby_media_panel &&
			     nearby_media_panel->getParcelAudioAutoStart()) ||
			    (!nearby_media_panel &&
				 gSavedSettings.getBOOL("MediaTentativeAutoPlay")))
			{
				LL_INFOS() << "Starting parcel music " << parcel->getMusicURL() << LL_ENDL;
				LLViewerParcelMedia::playStreamingMusic(parcel);
				return;
			}
		}
		gAudiop->startInternetStream(LLStringUtil::null);
	}
}
void LLViewerParcelMgr::processParcelAccessListReply(LLMessageSystem *msg, void **user)
{
	LLUUID agent_id;
	S32 sequence_id = 0;
	U32 message_flags = 0x0;
	S32 parcel_id = -1;
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_AgentID, agent_id);
	msg->getS32Fast( _PREHASH_Data, _PREHASH_SequenceID, sequence_id );
	msg->getU32Fast( _PREHASH_Data, _PREHASH_Flags, message_flags);
	msg->getS32Fast( _PREHASH_Data, _PREHASH_LocalID, parcel_id);
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->mCurrentParcel;
	if (!parcel) return;
	if (parcel_id != parcel->getLocalID())
	{
		LL_WARNS_ONCE("") << "processParcelAccessListReply for parcel " << parcel_id
			<< " which isn't the selected parcel " << parcel->getLocalID()<< LL_ENDL;
		return;
	}
	if (message_flags & AL_ACCESS)
	{
		parcel->unpackAccessEntries(msg, &(parcel->mAccessList) );
	}
	else if (message_flags & AL_BAN)
	{
		parcel->unpackAccessEntries(msg, &(parcel->mBanList) );
	}
	else if (message_flags & AL_ALLOW_EXPERIENCE)
	{
		parcel->unpackExperienceEntries(msg, EXPERIENCE_KEY_TYPE_ALLOWED);
	}
	else if (message_flags & AL_BLOCK_EXPERIENCE)
	{
		parcel->unpackExperienceEntries(msg, EXPERIENCE_KEY_TYPE_BLOCKED);
	}
	LLViewerParcelMgr::getInstance()->notifyObservers();
}
void LLViewerParcelMgr::processParcelDwellReply(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	S32 local_id;
	msg->getS32("Data", "LocalID", local_id);
	LLUUID parcel_id;
	msg->getUUID("Data", "ParcelID", parcel_id);
	F32 dwell;
	msg->getF32("Data", "Dwell", dwell);
	if (local_id == LLViewerParcelMgr::getInstance()->mCurrentParcel->getLocalID())
	{
		LLViewerParcelMgr::getInstance()->mSelectedDwell = dwell;
		LLViewerParcelMgr::getInstance()->notifyObservers();
	}
}
void LLViewerParcelMgr::sendParcelAccessListUpdate(U32 which)
{
	if (!mSelected)
	{
		return;
	}
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;
	LLParcel* parcel = mCurrentParcel;
	if (!parcel) return;
	if (which & AL_ACCESS)
	{
		sendParcelAccessListUpdate(AL_ACCESS, parcel->mAccessList, region, parcel->getLocalID());
	}
	if (which & AL_BAN)
	{
		sendParcelAccessListUpdate(AL_BAN, parcel->mBanList, region, parcel->getLocalID());
	}
	if (which & AL_ALLOW_EXPERIENCE)
	{
		sendParcelAccessListUpdate(AL_ALLOW_EXPERIENCE, parcel->getExperienceKeysByType(EXPERIENCE_KEY_TYPE_ALLOWED), region, parcel->getLocalID());
	}
	if (which & AL_BLOCK_EXPERIENCE)
	{
		sendParcelAccessListUpdate(AL_BLOCK_EXPERIENCE, parcel->getExperienceKeysByType(EXPERIENCE_KEY_TYPE_BLOCKED), region, parcel->getLocalID());
	}
}
void LLViewerParcelMgr::sendParcelAccessListUpdate(U32 flags, const LLAccessEntry::map& entries, LLViewerRegion* region, S32 parcel_local_id)
{
	S32 count = entries.size();
	S32 num_sections = (S32) ceil(count/PARCEL_MAX_ENTRIES_PER_PACKET);
	S32 sequence_id = 1;
	BOOL start_message = TRUE;
	BOOL initial = TRUE;
	LLUUID transactionUUID;
	transactionUUID.generate();
	LLMessageSystem* msg = gMessageSystem;
	LLAccessEntry::map::const_iterator cit = entries.begin();
	LLAccessEntry::map::const_iterator end = entries.end();
	while ( (cit != end) || initial )
	{
		if (start_message)
		{
			msg->newMessageFast(_PREHASH_ParcelAccessListUpdate);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
			msg->nextBlockFast(_PREHASH_Data);
			msg->addU32Fast(_PREHASH_Flags, flags);
			msg->addS32(_PREHASH_LocalID, parcel_local_id);
			msg->addUUIDFast(_PREHASH_TransactionID, transactionUUID);
			msg->addS32Fast(_PREHASH_SequenceID, sequence_id);
			msg->addS32Fast(_PREHASH_Sections, num_sections);
			start_message = FALSE;
			if (initial && (cit == end))
			{
				msg->nextBlockFast(_PREHASH_List);
				msg->addUUIDFast(_PREHASH_ID,  LLUUID::null );
				msg->addS32Fast(_PREHASH_Time, 0 );
				msg->addU32Fast(_PREHASH_Flags,	0 );
			}
			initial = FALSE;
			sequence_id++;
		}
		while ( (cit != end) && (msg->getCurrentSendTotal() < MTUBYTES))
		{
			const LLAccessEntry& entry = (*cit).second;
			msg->nextBlockFast(_PREHASH_List);
			msg->addUUIDFast(_PREHASH_ID,  entry.mID );
			msg->addS32Fast(_PREHASH_Time, entry.mTime );
			msg->addU32Fast(_PREHASH_Flags,	entry.mFlags );
			++cit;
		}
		start_message = TRUE;
		msg->sendReliable( region->getHost() );
	}
}
void LLViewerParcelMgr::deedLandToGroup()
{
	std::string group_name;
	gCacheName->getGroupName(mCurrentParcel->getGroupID(), group_name);
	LLSD args;
	args["AREA"] = llformat("%d", mCurrentParcel->getArea());
	args["GROUP_NAME"] = group_name;
	if(mCurrentParcel->getContributeWithDeed())
	{
		std::string full_name;
		gCacheName->getFullName(mCurrentParcel->getOwnerID(),full_name);
		args["NAME"] = full_name;
		LLNotificationsUtil::add("DeedLandToGroupWithContribution",args, LLSD(), deedAlertCB);
	}
	else
	{
		LLNotificationsUtil::add("DeedLandToGroup",args, LLSD(), deedAlertCB);
	}
}
bool LLViewerParcelMgr::deedAlertCB(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel();
		LLUUID group_id;
		if(parcel)
		{
			group_id = parcel->getGroupID();
		}
		LLViewerParcelMgr::getInstance()->sendParcelDeed(group_id);
	}
	return false;
}
void LLViewerParcelMgr::startReleaseLand()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotReleaseLandNothingSelected");
		return;
	}
	if (mRequestResult == PARCEL_RESULT_NO_DATA)
	{
		LLNotificationsUtil::add("CannotReleaseLandWatingForServer");
		return;
	}
	if (mRequestResult == PARCEL_RESULT_MULTIPLE)
	{
		LLNotificationsUtil::add("CannotReleaseLandSelected");
		return;
	}
	if (!isParcelOwnedByAgent(mCurrentParcel, GP_LAND_RELEASE)
		&& !(gAgent.canManageEstate()))
	{
		LLNotificationsUtil::add("CannotReleaseLandDontOwn");
		return;
	}
	LLVector3d parcel_center = (mWestSouth + mEastNorth) / 2.0;
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotReleaseLandRegionNotFound");
		return;
	}
	if (!mCurrentParcelSelection->mWholeParcelSelected)
	{
		LLNotificationsUtil::add("CannotReleaseLandPartialSelection");
		return;
	}
	LLSD args;
	args["AREA"] = llformat("%d",mCurrentParcel->getArea());
	LLNotificationsUtil::add("ReleaseLandWarning", args, LLSD(), releaseAlertCB);
}
bool LLViewerParcelMgr::canAgentBuyParcel(LLParcel* parcel, bool forGroup) const
{
	if (!parcel)
	{
		return false;
	}
	if (mSelected  &&  parcel == mCurrentParcel)
	{
		if (mRequestResult == PARCEL_RESULT_NO_DATA)
		{
			return false;
		}
	}
	const LLUUID& parcelOwner = parcel->getOwnerID();
	const LLUUID& authorizeBuyer = parcel->getAuthorizedBuyerID();
	if (parcel->isPublic())
	{
		return true;
	}
	LLViewerRegion* regionp = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if (regionp)
	{
		U8 sim_access = regionp->getSimAccess();
		const LLAgentAccess& agent_access = gAgent.getAgentAccess();
		if ((sim_access == SIM_ACCESS_MATURE) &&
			!agent_access.canAccessMature())
		{
			return false;
		}
		else if ((sim_access == SIM_ACCESS_ADULT) &&
				 !agent_access.canAccessAdult())
		{
			return false;
		}
	}
	bool isForSale = parcel->getForSale()
			&& ((parcel->getSalePrice() > 0) || (authorizeBuyer.notNull()));
	bool isEmpowered
		= forGroup ? gAgent.hasPowerInActiveGroup(GP_LAND_DEED) == TRUE : true;
	bool isOwner
		= parcelOwner == (forGroup ? gAgent.getGroupID() : gAgent.getID());
	bool isAuthorized
			= (authorizeBuyer.isNull()
				|| (gAgent.getID() == authorizeBuyer)
				|| (gAgent.hasPowerInGroup(authorizeBuyer,GP_LAND_DEED)
					&& gAgent.hasPowerInGroup(authorizeBuyer,GP_LAND_SET_SALE_INFO)));
	return isForSale && !isOwner && isAuthorized  && isEmpowered;
}
void LLViewerParcelMgr::startBuyLand(BOOL is_for_group)
{
	if (RlvActions::isRlvEnabled() && !RlvActions::canShowLocation())
		return;
	if (selectionEmpty()) selectParcelAt(gAgent.getPositionGlobal());
	LLFloaterBuyLand::buyLand(getSelectionRegion(), mCurrentParcelSelection, is_for_group == TRUE);
}
void LLViewerParcelMgr::startSellLand()
{
	LLFloaterSellLand::sellLand(getSelectionRegion(), mCurrentParcelSelection);
}
void LLViewerParcelMgr::startDivideLand()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotDivideLandNothingSelected");
		return;
	}
	if (mCurrentParcelSelection->mWholeParcelSelected)
	{
		LLNotificationsUtil::add("CannotDivideLandPartialSelection");
		return;
	}
	LLSD payload;
	payload["west_south_border"] = ll_sd_from_vector3d(mWestSouth);
	payload["east_north_border"] = ll_sd_from_vector3d(mEastNorth);
	LLNotificationsUtil::add("LandDivideWarning", LLSD(), payload, callbackDivideLand);
}
bool LLViewerParcelMgr::callbackDivideLand(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLVector3d west_south_d = ll_vector3d_from_sd(notification["payload"]["west_south_border"]);
	LLVector3d east_north_d = ll_vector3d_from_sd(notification["payload"]["east_north_border"]);
	LLVector3d parcel_center = (west_south_d + east_north_d) / 2.0;
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotDivideLandNoRegion");
		return false;
	}
	if (0 == option)
	{
		LLVector3 west_south = region->getPosRegionFromGlobal(west_south_d);
		LLVector3 east_north = region->getPosRegionFromGlobal(east_north_d);
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelDivide");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("ParcelData");
		msg->addF32("West", west_south.mV[VX]);
		msg->addF32("South", west_south.mV[VY]);
		msg->addF32("East", east_north.mV[VX]);
		msg->addF32("North", east_north.mV[VY]);
		msg->sendReliable(region->getHost());
	}
	return false;
}
void LLViewerParcelMgr::startJoinLand()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotJoinLandNothingSelected");
		return;
	}
	if (mCurrentParcelSelection->mWholeParcelSelected)
	{
		LLNotificationsUtil::add("CannotJoinLandEntireParcelSelected");
		return;
	}
	if (!mCurrentParcelSelection->mSelectedMultipleOwners)
	{
		LLNotificationsUtil::add("CannotJoinLandSelection");
		return;
	}
	LLSD payload;
	payload["west_south_border"] = ll_sd_from_vector3d(mWestSouth);
	payload["east_north_border"] = ll_sd_from_vector3d(mEastNorth);
	LLNotificationsUtil::add("JoinLandWarning", LLSD(), payload, callbackJoinLand);
}
bool LLViewerParcelMgr::callbackJoinLand(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLVector3d west_south_d = ll_vector3d_from_sd(notification["payload"]["west_south_border"]);
	LLVector3d east_north_d = ll_vector3d_from_sd(notification["payload"]["east_north_border"]);
	LLVector3d parcel_center = (west_south_d + east_north_d) / 2.0;
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotJoinLandNoRegion");
		return false;
	}
	if (0 == option)
	{
		LLVector3 west_south = region->getPosRegionFromGlobal(west_south_d);
		LLVector3 east_north = region->getPosRegionFromGlobal(east_north_d);
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelJoin");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("ParcelData");
		msg->addF32("West", west_south.mV[VX]);
		msg->addF32("South", west_south.mV[VY]);
		msg->addF32("East", east_north.mV[VX]);
		msg->addF32("North", east_north.mV[VY]);
		msg->sendReliable(region->getHost());
	}
	return false;
}
void LLViewerParcelMgr::startDeedLandToGroup()
{
	if (!mSelected || !mCurrentParcel)
	{
		LLNotificationsUtil::add("CannotDeedLandNothingSelected");
		return;
	}
	if (mRequestResult == PARCEL_RESULT_NO_DATA)
	{
		LLNotificationsUtil::add("CannotDeedLandWaitingForServer");
		return;
	}
	if (mRequestResult == PARCEL_RESULT_MULTIPLE)
	{
		LLNotificationsUtil::add("CannotDeedLandMultipleSelected");
		return;
	}
	LLVector3d parcel_center = (mWestSouth + mEastNorth) / 2.0;
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotDeedLandNoRegion");
		return;
	}
	deedLandToGroup();
}
void LLViewerParcelMgr::reclaimParcel()
{
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel();
	LLViewerRegion* regionp = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if(parcel && parcel->getOwnerID().notNull()
	   && (parcel->getOwnerID() != gAgent.getID())
	   && regionp && (regionp->getOwner() == gAgent.getID()))
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelReclaim");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addS32("LocalID", parcel->getLocalID());
		msg->sendReliable(regionp->getHost());
	}
}
bool LLViewerParcelMgr::releaseAlertCB(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLViewerParcelMgr::getInstance()->sendParcelRelease();
	}
	return false;
}
void LLViewerParcelMgr::buyPass()
{
	LLParcel* parcel = getParcelSelection()->getParcel();
	if (!parcel) return;
	LLViewerRegion* region = getSelectionRegion();
	if (!region) return;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelBuyPass);
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_LocalID, parcel->getLocalID() );
	msg->sendReliable( region->getHost() );
}
BOOL LLViewerParcelMgr::isCollisionBanned()
{
	if ((mCollisionBanned == BA_ALLOWED) || (mCollisionBanned == BA_NOT_ON_LIST) || (mCollisionBanned == BA_NOT_IN_GROUP))
		return FALSE;
	else
		return TRUE;
}
BOOL LLViewerParcelMgr::isParcelOwnedByAgent(const LLParcel* parcelp, U64 group_proxy_power)
{
	if (!parcelp)
	{
		return FALSE;
	}
	if (gAgent.isGodlike())
	{
		return TRUE;
	}
	if (parcelp->getOwnerID() == gAgent.getID())
	{
		return TRUE;
	}
	if (parcelp->isPublic())
	{
		return FALSE;
	}
	return gAgent.hasPowerInGroup(parcelp->getOwnerID(), group_proxy_power);
}
BOOL LLViewerParcelMgr::isParcelModifiableByAgent(const LLParcel* parcelp, U64 group_proxy_power)
{
	BOOL rv = FALSE;
	if (parcelp)
	{
		rv = isParcelOwnedByAgent(parcelp, group_proxy_power);
		if( (gAgent.getID() == parcelp->getOwnerID())
			&& !gAgent.isGodlike()
			&& (parcelp->getOwnershipStatus() != LLParcel::OS_LEASED) )
		{
			rv = FALSE;
		}
	}
	return rv;
}
void sanitize_corners(const LLVector3d &corner1,
										const LLVector3d &corner2,
										LLVector3d &west_south_bottom,
										LLVector3d &east_north_top)
{
	west_south_bottom.mdV[VX] = llmin( corner1.mdV[VX], corner2.mdV[VX] );
	west_south_bottom.mdV[VY] = llmin( corner1.mdV[VY], corner2.mdV[VY] );
	west_south_bottom.mdV[VZ] = llmin( corner1.mdV[VZ], corner2.mdV[VZ] );
	east_north_top.mdV[VX] = llmax( corner1.mdV[VX], corner2.mdV[VX] );
	east_north_top.mdV[VY] = llmax( corner1.mdV[VY], corner2.mdV[VY] );
	east_north_top.mdV[VZ] = llmax( corner1.mdV[VZ], corner2.mdV[VZ] );
}
void LLViewerParcelMgr::cleanupGlobals()
{
	LLParcelSelection::sNullSelection = NULL;
}
LLViewerTexture* LLViewerParcelMgr::getBlockedImage() const
{
	return sBlockedImage;
}
LLViewerTexture* LLViewerParcelMgr::getPassImage() const
{
	return sPassImage;
}
boost::signals2::connection LLViewerParcelMgr::addAgentParcelChangedCallback(parcel_changed_callback_t cb)
{
	return mAgentParcelChangedSignal.connect(cb);
}
boost::signals2::connection LLViewerParcelMgr::setTeleportFinishedCallback(teleport_finished_callback_t cb)
{
	return mTeleportFinishedSignal.connect(cb);
}
boost::signals2::connection LLViewerParcelMgr::setTeleportFailedCallback(parcel_changed_callback_t cb)
{
	return mTeleportFailedSignal.connect(cb);
}
void LLViewerParcelMgr::onTeleportFinished(bool local, const LLVector3d& new_pos)
{
	if (local && LLViewerParcelMgr::getInstance()->inAgentParcel(new_pos))
	{
		getInstance()->mTeleportFinishedSignal(new_pos);
	}
	else
	{
		mTeleportInProgress = TRUE;
	}
}
void LLViewerParcelMgr::onTeleportFailed()
{
	mTeleportFailedSignal();
}
boost::signals2::connection LLViewerParcelMgr::setCollisionUpdateCallback(const collision_update_signal_t::slot_type & cb)
{
	if (!mCollisionUpdateSignal)
		mCollisionUpdateSignal = new collision_update_signal_t();
	return mCollisionUpdateSignal->connect(cb);
}
