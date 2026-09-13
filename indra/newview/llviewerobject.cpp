/**
 * @file llviewerobject.cpp
 * @brief Base class for viewer objects
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
#include "llviewerprecompiledheaders.h"
#include "llviewerobject.h"
#include "llaudioengine.h"
#include "imageids.h"
#include "indra_constants.h"
#include "llmath.h"
#include "llflexibleobject.h"
#include "llviewercontrol.h"
#include "lldatapacker.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llframetimer.h"
#include "llinventory.h"
#include "llinventorydefines.h"
#include "llmaterialtable.h"
#include "llmutelist.h"
#include "llnamevalue.h"
#include "llprimitive.h"
#include "llquantize.h"
#include "llregionhandle.h"
#include "llsdserialize.h"
#include "lltree_common.h"
#include "llxfermanager.h"
#include "message.h"
#include "object_flags.h"
#include "timing.h"
#include "llaudiosourcevo.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llbbox.h"
#include "llbox.h"
#include "llcylinder.h"
#include "llcontrolavatar.h"
#include "lldrawable.h"
#include "llface.h"
#include "llfloaterproperties.h"
#include "llfloatertools.h"
#include "llfollowcam.h"
#include "llhudtext.h"
#include "llselectmgr.h"
#include "llrendersphere.h"
#include "lltooldraganddrop.h"
#include "lluiavatar.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewerparceloverlay.h"
#include "llviewerpartsource.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertextureanim.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvoclouds.h"
#include "llvograss.h"
#include "llvoground.h"
#include "llvolume.h"
#include "llvolumemessage.h"
#include "llvopartgroup.h"
#include "llvosky.h"
#include "llvosurfacepatch.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llvowater.h"
#include "llworld.h"
#include "llui.h"
#include "pipeline.h"
#include "llviewernetwork.h"
#include "llvowlsky.h"
#include "llmanip.h"
#include "llmediaentry.h"
#include "llmeshrepository.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
BOOL		LLViewerObject::sVelocityInterpolate = TRUE;
BOOL		LLViewerObject::sPingInterpolate = TRUE;
U32			LLViewerObject::sNumZombieObjects = 0;
S32			LLViewerObject::sNumObjects = 0;
BOOL		LLViewerObject::sMapDebug = TRUE;
LLColor4	LLViewerObject::sEditSelectColor(	1.0f, 1.f, 0.f, 0.3f);
LLColor4	LLViewerObject::sNoEditSelectColor(	1.0f, 0.f, 0.f, 0.3f);
S32			LLViewerObject::sAxisArrowLength(50);
BOOL		LLViewerObject::sPulseEnabled(FALSE);
BOOL		LLViewerObject::sUseSharedDrawables(FALSE);
F64Seconds	LLViewerObject::sMaxUpdateInterpolationTime(3.0);
F64Seconds	LLViewerObject::sPhaseOutUpdateInterpolationTime(2.0);
std::map<std::string, U32> LLViewerObject::sObjectDataMap;
#define MAX_OBJECT_PARAMS_SIZE 1024
const F32 PHYSICS_TIMESTEP = 1.f / 45.f;
const U32 MAX_INV_FILE_READ_FAILS = 25;
const S32 MAX_OBJECT_BINARY_DATA_SIZE = 60 + 16;
static LLTrace::BlockTimerStatHandle FTM_CREATE_OBJECT("Create Object");
LLViewerObject *LLViewerObject::createObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp, S32 flags)
{
	LLViewerObject *res = NULL;
	LL_RECORD_BLOCK_TIME(FTM_CREATE_OBJECT);
	switch (pcode)
	{
	case LL_PCODE_VOLUME:
	  res = new LLVOVolume(id, pcode, regionp); break;
	case LL_PCODE_LEGACY_AVATAR:
	{
		if (id == gAgentID)
		{
			if (!gAgentAvatarp)
			{
				gAgentAvatarp = new LLVOAvatarSelf(id, pcode, regionp);
				gAgentAvatarp->initInstance();
				gAgentWearables.setAvatarObject(gAgentAvatarp);
			}
			else
			{
				if (isAgentAvatarValid())
				{
					gAgentAvatarp->updateRegion(regionp);
				}
			}
			res = gAgentAvatarp;
		}
		else if (flags & CO_FLAG_CONTROL_AVATAR)
		{
			LLControlAvatar *control_avatar = new LLControlAvatar(id, pcode, regionp);
			control_avatar->initInstance();
			res = control_avatar;
		}
		else if (flags & CO_FLAG_UI_AVATAR)
		{
			LLUIAvatar *ui_avatar = new LLUIAvatar(id, pcode, regionp);
			ui_avatar->initInstance();
			res = ui_avatar;
		}
		else
		{
			LLVOAvatar *avatar = new LLVOAvatar(id, pcode, regionp);
			avatar->initInstance();
			res = avatar;
		}
		break;
	}
	case LL_PCODE_LEGACY_GRASS:
	  res = new LLVOGrass(id, pcode, regionp); break;
	case LL_PCODE_LEGACY_PART_SYS:
	  res = NULL; break;
	case LL_PCODE_LEGACY_TREE:
	  res = new LLVOTree(id, pcode, regionp); break;
	case LL_PCODE_TREE_NEW:
	  res = NULL; break;
#if ENABLE_CLASSIC_CLOUDS
	case LL_VO_CLOUDS:
	  res = new LLVOClouds(id, pcode, regionp); break;
#endif
	case LL_VO_SURFACE_PATCH:
	  res = new LLVOSurfacePatch(id, pcode, regionp); break;
	case LL_VO_SKY:
	  res = new LLVOSky(id, pcode, regionp); break;
	case LL_VO_VOID_WATER:
		res = new LLVOVoidWater(id, pcode, regionp); break;
	case LL_VO_WATER:
		res = new LLVOWater(id, pcode, regionp); break;
	case LL_VO_GROUND:
	  res = new LLVOGround(id, pcode, regionp); break;
	case LL_VO_PART_GROUP:
	  res = new LLVOPartGroup(id, pcode, regionp); break;
	case LL_VO_HUD_PART_GROUP:
	  res = new LLVOHUDPartGroup(id, pcode, regionp); break;
	case LL_VO_WL_SKY:
	  res = new LLVOWLSky(id, pcode, regionp); break;
	default:
	  LL_WARNS() << "Unknown object pcode " << (S32)pcode << LL_ENDL;
	  res = NULL; break;
	}
	return res;
}
LLViewerObject::LLViewerObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp, BOOL is_global)
:	LLPrimitive(),
	mChildList(),
	mID(id),
	mLocalID(0),
	mTotalCRC(0),
	mListIndex(-1),
	mTEImages(NULL),
	mTENormalMaps(NULL),
	mTESpecularMaps(NULL),
	mGLName(0),
	mbCanSelect(TRUE),
	mFlags(0),
	mPhysicsShapeType(0),
	mPhysicsGravity(0),
	mPhysicsFriction(0),
	mPhysicsDensity(0),
	mPhysicsRestitution(0),
	mDrawable(),
	mCreateSelected(FALSE),
	mRenderMedia(FALSE),
	mBestUpdatePrecision(0),
	mText(),
	mHudTextColor(LLColor4::white),
	mControlAvatar(NULL),
	mLastInterpUpdateSecs(0.f),
	mLastMessageUpdateSecs(0.f),
	mLatestRecvPacketID(0),
	mData(NULL),
	mAudioSourcep(NULL),
	mAudioGain(1.f),
	mAppAngle(0.f),
	mPixelArea(1024.f),
	mInventory(NULL),
	mInventorySerialNum(0),
	mInvRequestState(INVENTORY_REQUEST_STOPPED),
	mInvRequestXFerId(0),
	mInventoryDirty(FALSE),
	mRegionp(regionp),
	mDead(FALSE),
	mOrphaned(FALSE),
	mUserSelected(FALSE),
	mOnActiveList(FALSE),
	mOnMap(FALSE),
	mStatic(FALSE),
	mNumFaces(0),
	mRotTime(0.f),
	mAngularVelocityRot(),
	mPreviousRotation(),
	mAttachmentState(0),
	mMedia(NULL),
	mClickAction(0),
	mObjectCost(0),
	mLinksetCost(0),
	mPhysicsCost(0),
	mLinksetPhysicsCost(0.f),
	mCostStale(true),
	mPhysicsShapeUnknown(true),
	mAttachmentItemID(LLUUID::null),
	mLastUpdateType(OUT_UNKNOWN),
	mLastUpdateCached(FALSE),
	mExtraParameterList(LLNetworkData::PARAMS_MAX >> 4)
{
	if(!is_global)
	{
		llassert(mRegionp);
	}
	LLPrimitive::init_primitive(pcode);
	mLastInterpUpdateSecs = LLFrameTimer::getElapsedSeconds();
	mPositionRegion = LLVector3(0.f, 0.f, 0.f);
	if (!is_global && mRegionp)
	{
		mPositionAgent = mRegionp->getOriginAgent();
	}
	static const LLCachedControl<bool> use_new_target_omega ("UseNewTargetOmegaCode", true);
	if (use_new_target_omega)
	{
		resetRot();
	}
	LLViewerObject::sNumObjects++;
}
LLViewerObject::~LLViewerObject()
{
	deleteTEImages();
	if(mInventory)
	{
		mInventory->clear();
		delete mInventory;
		mInventory = NULL;
	}
	if (mPartSourcep)
	{
		mPartSourcep->setDead();
		mPartSourcep = NULL;
	}
	mExtraParameterList.clear();
	for_each(mNameValuePairs.begin(), mNameValuePairs.end(), DeletePairedPointer()) ;
	mNameValuePairs.clear();
	delete[] mData;
	mData = NULL;
	delete mMedia;
	mMedia = NULL;
	sNumObjects--;
	sNumZombieObjects--;
	llassert(mChildList.size() == 0);
	clearInventoryListeners();
}
void LLViewerObject::deleteTEImages()
{
	delete[] mTEImages;
	mTEImages = NULL;
	if (mTENormalMaps != NULL)
	{
		delete[] mTENormalMaps;
		mTENormalMaps = NULL;
	}
	if (mTESpecularMaps != NULL)
	{
		delete[] mTESpecularMaps;
		mTESpecularMaps = NULL;
	}
}
void LLViewerObject::markDead()
{
	if (!mDead)
	{
		if (isHUDAttachment() || (isAttachment() && hud_teleport_logging_active()))
		{
			log_hud_event("markDead", this);
		}
		if (isSelected())
		{
			LLSelectMgr::getInstance()->deselectObjectOnly(this, FALSE);
		}
		if (getParent())
		{
			((LLViewerObject *)getParent())->removeChild(this);
		}
		LLUUID mesh_id;
		{
			LLVOAvatar *av = getAvatar();
			if (av && LLVOAvatar::getRiggedMeshID(this,mesh_id))
			{
				av->updateAttachmentOverrides();
			}
		}
		if (getControlAvatar())
		{
			unlinkControlAvatar();
		}
		mDead = TRUE;
		if(mRegionp)
		{
			mRegionp->removeFromCreatedList(getLocalID());
		}
		gObjectList.cleanupReferences(this);
		LLViewerObject *childp;
		while (mChildList.size() > 0)
		{
			childp = mChildList.back();
			if (childp->getPCode() != LL_PCODE_LEGACY_AVATAR)
			{
				childp->setParent(NULL);
				childp->markDead();
			}
			else
			{
				childp->setDrawableParent(NULL);
				((LLVOAvatar*)childp)->getOffObject();
				childp->setParent(NULL);
			}
			mChildList.pop_back();
		}
		if (mDrawable.notNull())
		{
			mDrawable->markDead();
			mDrawable = NULL;
		}
		if (mText)
		{
			mText->markDead();
			mText = NULL;
		}
		if (mIcon)
		{
			mIcon->markDead();
			mIcon = NULL;
		}
		if (mPartSourcep)
		{
			mPartSourcep->setDead();
			mPartSourcep = NULL;
		}
		if (mAudioSourcep)
		{
			if (gAudiop)
			{
				gAudiop->cleanupAudioSource(mAudioSourcep);
			}
			mAudioSourcep = NULL;
		}
		if (flagAnimSource())
		{
			if (isAgentAvatarValid())
			{
				gAgentAvatarp->stopMotionFromSource(mID);
			}
		}
		if (flagCameraSource())
		{
			LLFollowCamMgr::removeFollowCamParams(mID);
		}
		sNumZombieObjects++;
	}
}
void LLViewerObject::dump() const
{
	LL_INFOS() << "Type: " << pCodeToString(mPrimitiveCode) << LL_ENDL;
	LL_INFOS() << "Drawable: " << (LLDrawable *)mDrawable << LL_ENDL;
	LL_INFOS() << "Update Age: " << LLFrameTimer::getElapsedSeconds() - mLastMessageUpdateSecs << LL_ENDL;
	LL_INFOS() << "Parent: " << getParent() << LL_ENDL;
	LL_INFOS() << "ID: " << mID << LL_ENDL;
	LL_INFOS() << "LocalID: " << mLocalID << LL_ENDL;
	LL_INFOS() << "PositionRegion: " << getPositionRegion() << LL_ENDL;
	LL_INFOS() << "PositionAgent: " << getPositionAgent() << LL_ENDL;
	LL_INFOS() << "PositionGlobal: " << getPositionGlobal() << LL_ENDL;
	LL_INFOS() << "Velocity: " << getVelocity() << LL_ENDL;
	if (mDrawable.notNull() &&
		mDrawable->getNumFaces() &&
		mDrawable->getFace(0))
	{
		LLFacePool *poolp = mDrawable->getFace(0)->getPool();
		if (poolp)
		{
			LL_INFOS() << "Pool: " << poolp << LL_ENDL;
			LL_INFOS() << "Pool reference count: " << poolp->mReferences.size() << LL_ENDL;
		}
	}
}
void LLViewerObject::printNameValuePairs() const
{
	for (name_value_map_t::const_iterator iter = mNameValuePairs.begin();
		 iter != mNameValuePairs.end(); iter++)
	{
		LLNameValue* nv = iter->second;
		LL_INFOS() << nv->printNameValue() << LL_ENDL;
	}
}
void LLViewerObject::initVOClasses()
{
	LLVOAvatar::initClass();
	LLVOTree::initClass();
	if (gNoRender)
	{
		return;
	}
	LL_INFOS() << "Viewer Object size: " << sizeof(LLViewerObject) << LL_ENDL;
	LLVOGrass::initClass();
	LLVOWater::initClass();
	LLVOVolume::initClass();
	initObjectDataMap();
}
void LLViewerObject::cleanupVOClasses()
{
	LLVOGrass::cleanupClass();
	LLVOWater::cleanupClass();
	LLVOTree::cleanupClass();
	LLVOAvatar::cleanupClass();
	LLVOVolume::cleanupClass();
	sObjectDataMap.clear();
}
void LLViewerObject::initObjectDataMap()
{
	U32 count = 0;
	sObjectDataMap["ID"] = count;
	count += sizeof(LLUUID);
	sObjectDataMap["LocalID"] = count;
	count += sizeof(U32);
	sObjectDataMap["PCode"] = count;
	count += sizeof(U8);
	sObjectDataMap["State"] = count;
	count += sizeof(U8);
	sObjectDataMap["CRC"] = count;
	count += sizeof(U32);
	sObjectDataMap["Material"] = count;
	count += sizeof(U8);
	sObjectDataMap["ClickAction"] = count;
	count += sizeof(U8);
	sObjectDataMap["Scale"] = count;
	count += sizeof(LLVector3);
	sObjectDataMap["Pos"] = count;
	count += sizeof(LLVector3);
	sObjectDataMap["Rot"] = count;
	count += sizeof(LLVector3);
	sObjectDataMap["SpecialCode"] = count;
	count += sizeof(U32);
	sObjectDataMap["Owner"] = count;
	count += sizeof(LLUUID);
	sObjectDataMap["Omega"] = count;
	count += sizeof(LLVector3);
	sObjectDataMap["ParentID"] = count;
	count += sizeof(U32);
}
void LLViewerObject::unpackVector3(LLDataPackerBinaryBuffer* dp, LLVector3& value, std::string name)
{
	dp->shift(sObjectDataMap[name]);
	dp->unpackVector3(value, name.c_str());
	dp->reset();
}
void LLViewerObject::unpackUUID(LLDataPackerBinaryBuffer* dp, LLUUID& value, std::string name)
{
	dp->shift(sObjectDataMap[name]);
	dp->unpackUUID(value, name.c_str());
	dp->reset();
}
void LLViewerObject::unpackU32(LLDataPackerBinaryBuffer* dp, U32& value, std::string name)
{
	dp->shift(sObjectDataMap[name]);
	dp->unpackU32(value, name.c_str());
	dp->reset();
}
void LLViewerObject::unpackU8(LLDataPackerBinaryBuffer* dp, U8& value, std::string name)
{
	dp->shift(sObjectDataMap[name]);
	dp->unpackU8(value, name.c_str());
	dp->reset();
}
U32 LLViewerObject::unpackParentID(LLDataPackerBinaryBuffer* dp, U32& parent_id)
{
	dp->shift(sObjectDataMap["SpecialCode"]);
	U32 value;
	dp->unpackU32(value, "SpecialCode");
	parent_id = 0;
	if(value & 0x20)
	{
		S32 offset = sObjectDataMap["ParentID"];
		if(!(value & 0x80))
		{
			offset -= sizeof(LLVector3);
		}
		dp->shift(offset);
		dp->unpackU32(parent_id, "ParentID");
	}
	dp->reset();
	return parent_id;
}
void LLViewerObject::setNameValueList(const std::string& name_value_list)
{
	for_each(mNameValuePairs.begin(), mNameValuePairs.end(), DeletePairedPointer()) ;
	mNameValuePairs.clear();
	std::string::size_type length = name_value_list.length();
	std::string::size_type start = 0;
	while (start < length)
	{
		std::string::size_type end = name_value_list.find_first_of("\n", start);
		if (end == std::string::npos) end = length;
		if (end > start)
		{
			std::string tok = name_value_list.substr(start, end - start);
			addNVPair(tok);
		}
		start = end+1;
	}
}
BOOL LLViewerObject::isAnySelected() const
{
	bool any_selected = isSelected();
	for (child_list_t::const_iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		const LLViewerObject* child = *iter;
		any_selected = any_selected || child->isSelected();
	}
	return any_selected;
}
void LLViewerObject::setSelected(BOOL sel)
{
	mUserSelected = sel;
	static const LLCachedControl<bool> use_new_target_omega ("UseNewTargetOmegaCode", true);
	if(use_new_target_omega)
	{
		resetRot();
	}
	else
	{
		mRotTime = 0.f;
	}
	if (!sel)
	{
		setAllTESelected(false);
	}
}
bool LLViewerObject::isReturnable()
{
	if (isAttachment())
	{
		return false;
	}
	if ( (rlv_handler_t::isEnabled()) &&
		 ( ((gRlvHandler.hasBehaviour(RLV_BHVR_REZ)) && ((permYouOwner() || permGroupOwner()))) ||
		   ((gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) && (isAgentAvatarValid()) && (getRootEdit()->isChild(gAgentAvatarp))) ) )
	{
		return false;
	}
	std::vector<LLBBox> boxes;
	boxes.push_back(LLBBox(getPositionRegion(), getRotationRegion(), getScale() * -0.5f, getScale() * 0.5f).getAxisAligned());
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		boxes.push_back( LLBBox(child->getPositionRegion(), child->getRotationRegion(), child->getScale() * -0.5f, child->getScale() * 0.5f).getAxisAligned());
	}
	bool result = (mRegionp && mRegionp->objectIsReturnable(getPositionRegion(), boxes)) ? 1 : 0;
	if ( !result )
	{
		std::vector<LLViewerRegion*> uniqueRegions;
		mRegionp->getNeighboringRegions( uniqueRegions );
		std::vector<PotentialReturnableObject> returnables;
		typedef std::vector<LLViewerRegion*>::iterator RegionIt;
		RegionIt regionStart = uniqueRegions.begin();
		RegionIt regionEnd   = uniqueRegions.end();
		for (; regionStart != regionEnd; ++regionStart )
		{
			LLViewerRegion* pTargetRegion = *regionStart;
			buildReturnablesForChildrenVO( returnables, this, pTargetRegion );
			for (child_list_t::iterator iter = mChildList.begin();  iter != mChildList.end(); iter++)
			{
				LLViewerObject* pChild = *iter;
				buildReturnablesForChildrenVO( returnables, pChild, pTargetRegion );
			}
		}
		typedef std::vector<PotentialReturnableObject>::iterator ReturnablesIt;
		ReturnablesIt retCurrentIt = returnables.begin();
		ReturnablesIt retEndIt = returnables.end();
		for ( ; retCurrentIt !=retEndIt; ++retCurrentIt )
		{
			boxes.clear();
			LLViewerRegion* pRegion = (*retCurrentIt).pRegion;
			boxes.push_back( (*retCurrentIt).box );
			bool retResult = 	pRegion
							 && pRegion->childrenObjectReturnable( boxes )
							 && pRegion->canManageEstate();
			if ( retResult )
			{
				result = true;
				break;
			}
		}
	}
	return result;
}
void LLViewerObject::buildReturnablesForChildrenVO( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion )
{
	if ( !pChild )
	{
		LL_ERRS()<<"child viewerobject is NULL "<<LL_ENDL;
	}
	constructAndAddReturnable( returnables, pChild, pTargetRegion );
	for (child_list_t::iterator iter = pChild->mChildList.begin();  iter != pChild->mChildList.end(); iter++)
	{
		LLViewerObject* pChildofChild = *iter;
		buildReturnablesForChildrenVO( returnables, pChildofChild, pTargetRegion );
	}
}
void LLViewerObject::constructAndAddReturnable( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion )
{
	LLVector3 targetRegionPos;
	targetRegionPos.setVec( pChild->getPositionGlobal() );
	LLBBox childBBox = LLBBox( targetRegionPos, pChild->getRotationRegion(), pChild->getScale() * -0.5f,
								pChild->getScale() * 0.5f).getAxisAligned();
	LLVector3 edgeA = targetRegionPos + childBBox.getMinLocal();
	LLVector3 edgeB = targetRegionPos + childBBox.getMaxLocal();
	LLVector3d edgeAd, edgeBd;
	edgeAd.setVec(edgeA);
	edgeBd.setVec(edgeB);
	if ( pTargetRegion->pointInRegionGlobal( edgeAd ) || pTargetRegion->pointInRegionGlobal( edgeBd ) )
	{
		PotentialReturnableObject returnableObj;
		returnableObj.box		= childBBox;
		returnableObj.pRegion	= pTargetRegion;
		returnables.push_back( returnableObj );
	}
}
bool LLViewerObject::crossesParcelBounds()
{
	std::vector<LLBBox> boxes;
	boxes.push_back(LLBBox(getPositionRegion(), getRotationRegion(), getScale() * -0.5f, getScale() * 0.5f).getAxisAligned());
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		boxes.push_back(LLBBox(child->getPositionRegion(), child->getRotationRegion(), child->getScale() * -0.5f, child->getScale() * 0.5f).getAxisAligned());
	}
	return mRegionp && mRegionp->objectsCrossParcel(boxes);
}
BOOL LLViewerObject::setParent(LLViewerObject* parent)
{
	if (mParent != parent)
	{
		LLViewerObject* old_parent = (LLViewerObject*)mParent ;
		BOOL ret = LLPrimitive::setParent(parent);
		if (ret && old_parent && parent)
		{
			old_parent->removeChild(this) ;
		}
		return ret ;
	}
	return FALSE ;
}
void LLViewerObject::addChild(LLViewerObject *childp)
{
	for (child_list_t::iterator i = mChildList.begin(); i != mChildList.end(); ++i)
	{
		if (*i == childp)
		{
			return;
		}
	}
	if (!isAvatar())
	{
		childp->mbCanSelect = mbCanSelect;
	}
	if (childp->setParent(this))
	{
		mChildList.push_back(childp);
		childp->afterReparent();
	}
}
void LLViewerObject::onReparent(LLViewerObject *old_parent, LLViewerObject *new_parent)
{
}
void LLViewerObject::afterReparent()
{
}
void LLViewerObject::removeChild(LLViewerObject *childp)
{
	for (child_list_t::iterator i = mChildList.begin(); i != mChildList.end(); ++i)
	{
		if (*i == childp)
		{
			if (!childp->isAvatar() && mDrawable.notNull() && mDrawable->isActive() && childp->mDrawable.notNull() && !isAvatar())
			{
				gPipeline.markRebuild(childp->mDrawable, LLDrawable::REBUILD_VOLUME);
			}
			mChildList.erase(i);
			if(childp->getParent() == this)
			{
				childp->setParent(NULL);
			}
			break;
		}
	}
	if (childp->isSelected())
	{
		LLSelectMgr::getInstance()->deselectObjectAndFamily(childp);
		BOOL add_to_end = TRUE;
		LLSelectMgr::getInstance()->selectObjectAndFamily(childp, add_to_end);
	}
}
void LLViewerObject::addThisAndAllChildren(std::vector<LLViewerObject*>& objects)
{
	objects.push_back(this);
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); ++iter)
	{
		LLViewerObject* child = *iter;
		if (!child->isAvatar())
		{
			child->addThisAndAllChildren(objects);
		}
	}
}
void LLViewerObject::addThisAndNonJointChildren(std::vector<LLViewerObject*>& objects)
{
	objects.push_back(this);
	if (isAvatar())
	{
		return;
	}
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); ++iter)
	{
		LLViewerObject* child = *iter;
		if ( (!child->isAvatar()))
		{
			child->addThisAndNonJointChildren(objects);
		}
	}
}
BOOL LLViewerObject::isChild(const LLViewerObject *childp) const
{
	for (child_list_t::const_iterator iter = mChildList.begin();
		 iter != mChildList.end(); ++iter)
	{
		LLViewerObject* testchild = *iter;
		if (testchild == childp)
			return TRUE;
	}
	return FALSE;
}
BOOL LLViewerObject::isSeat() const
{
	for (child_list_t::const_iterator iter = mChildList.begin();
		 iter != mChildList.end(); ++iter)
	{
		LLViewerObject* child = *iter;
		if (child->isAvatar())
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL LLViewerObject::setDrawableParent(LLDrawable* parentp)
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}
	BOOL ret = mDrawable->mXform.setParent(parentp ? &parentp->mXform : NULL);
	if(!ret)
	{
		return FALSE ;
	}
	LLDrawable* old_parent = mDrawable->mParent;
	mDrawable->mParent = parentp;
	if (parentp && mDrawable->isActive())
	{
		parentp->makeActive();
		parentp->setState(LLDrawable::ACTIVE_CHILD);
	}
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	if(	(old_parent != parentp && old_parent)
		|| (parentp && parentp->isActive()))
	{
		gPipeline.markMoved(mDrawable, FALSE);
	}
	else if (!mDrawable->isAvatar())
	{
		mDrawable->updateXform(TRUE);
	}
	return ret;
}
void LLViewerObject::hideExtraDisplayItems( BOOL hidden )
{
	if( mPartSourcep.notNull() )
	{
		LLViewerPartSourceScript *partSourceScript = mPartSourcep.get();
		partSourceScript->setSuspended( hidden );
	}
	if( mText.notNull() )
	{
		LLHUDText *hudText = mText.get();
		hudText->setHidden( hidden );
	}
	if( mIcon.notNull() )
	{
		LLHUDIcon *hudIcon = mIcon.get();
		hudIcon->setHidden( hidden );
	}
}
U32 LLViewerObject::checkMediaURL(const std::string &media_url)
{
	U32 retval = (U32)0x0;
	if (!mMedia && !media_url.empty())
	{
		retval |= MEDIA_URL_ADDED;
		mMedia = new LLViewerObjectMedia;
		mMedia->mMediaURL = media_url;
		mMedia->mMediaType = LLViewerObject::MEDIA_SET;
		mMedia->mPassedWhitelist = FALSE;
	}
	else if (mMedia)
	{
		if (media_url.empty())
		{
			retval |= MEDIA_URL_REMOVED;
			delete mMedia;
			mMedia = NULL;
		}
		else if (mMedia->mMediaURL != media_url)
		{
			{
				retval |= MEDIA_URL_UPDATED;
			}
			mMedia->mMediaURL = media_url;
			mMedia->mPassedWhitelist = FALSE;
		}
	}
	return retval;
}
U32 LLViewerObject::extractSpatialExtents(LLDataPackerBinaryBuffer *dp, LLVector3& pos, LLVector3& scale, LLQuaternion& rot)
{
	U32	parent_id = 0;
	LLViewerObject::unpackParentID(dp, parent_id);
	LLViewerObject::unpackVector3(dp, scale, "Scale");
	LLViewerObject::unpackVector3(dp, pos, "Pos");
	LLVector3 vec;
	LLViewerObject::unpackVector3(dp, vec, "Rot");
	rot.unpackFromVector3(vec);
	return parent_id;
}
U32 LLViewerObject::processUpdateMessage(LLMessageSystem *mesgsys,
					 void **user_data,
					 U32 block_num,
					 const EObjectUpdateType update_type,
					 LLDataPacker *dp)
{
	U32 retval = 0x0;
	if (!LLWorld::instance().isRegionListed(mRegionp))
	{
		LL_WARNS() << "Updating object in an invalid region" << LL_ENDL;
		return retval;
	}
	U64 region_handle = 0;
	if(mesgsys != NULL)
	{
		mesgsys->getU64Fast(_PREHASH_RegionData, _PREHASH_RegionHandle, region_handle);
		LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromHandle(region_handle);
		if(regionp != mRegionp && regionp && mRegionp)
		{
			LLVector3 delta_pos =  mRegionp->getOriginAgent() - regionp->getOriginAgent();
			setPositionParent(getPosition() + delta_pos);
			setRegion(regionp) ;
		}
		else
		{
			if(regionp != mRegionp)
			{
				if(mRegionp)
				{
					mRegionp->removeFromCreatedList(getLocalID());
				}
				if(regionp)
				{
					regionp->addToCreatedList(getLocalID());
				}
			}
			mRegionp = regionp ;
		}
	}
	if (!mRegionp)
	{
		U32 x, y;
		from_region_handle(region_handle, &x, &y);
		LL_ERRS() << "Object has invalid region " << x << ":" << y << "!" << LL_ENDL;
		return retval;
	}
	F32 time_dilation = 1.f;
	if(mesgsys != NULL)
	{
		U16 time_dilation16;
		mesgsys->getU16Fast(_PREHASH_RegionData, _PREHASH_TimeDilation, time_dilation16);
		time_dilation = ((F32) time_dilation16) / 65535.f;
		mRegionp->setTimeDilation(time_dilation);
	}
	LLVector3 test_pos_parent = getPosition();
	S32 this_update_precision = 32;
	LLVector3 new_pos_parent, new_vel, new_angv;
	LLVector3 old_angv = getAngularVelocity();
	LLQuaternion new_rot;
	LLVector3 new_scale = getScale();
	U32	parent_id = 0;
	U8	material = 0;
	U8 click_action = 0;
	U32 crc = 0;
	bool old_special_hover_cursor = specialHoverCursor();
	LLViewerObject *cur_parentp = (LLViewerObject *)getParent();
	if (cur_parentp)
	{
		parent_id = cur_parentp->mLocalID;
	}
	if (!dp)
	{
		switch(update_type)
		{
		case OUT_FULL:
			{
#ifdef DEBUG_UPDATE_TYPE
				LL_INFOS() << "Full:" << getID() << LL_ENDL;
#endif
				processTerseData(mesgsys, user_data, block_num, this_update_precision, new_pos_parent, new_rot, new_angv, test_pos_parent);
				mCostStale = true;
				if (isSelected())
				{
					gFloaterTools->dirty();
				}
				LLUUID audio_uuid;
				LLUUID owner_id;
				F32    gain;
				U8     sound_flags;
				mesgsys->getU32Fast( _PREHASH_ObjectData, _PREHASH_CRC, crc, block_num);
				mesgsys->getU32Fast( _PREHASH_ObjectData, _PREHASH_ParentID, parent_id, block_num);
				mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_Sound, audio_uuid, block_num );
				mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, owner_id, block_num );
				if (owner_id.notNull()) mOwnerID = owner_id;
				mesgsys->getF32Fast( _PREHASH_ObjectData, _PREHASH_Gain, gain, block_num );
				mesgsys->getU8Fast(  _PREHASH_ObjectData, _PREHASH_Flags, sound_flags, block_num );
				mesgsys->getU8Fast(  _PREHASH_ObjectData, _PREHASH_Material, material, block_num );
				mesgsys->getU8Fast(  _PREHASH_ObjectData, _PREHASH_ClickAction, click_action, block_num);
				mesgsys->getVector3Fast(_PREHASH_ObjectData, _PREHASH_Scale, new_scale, block_num );
				mTotalCRC = crc;
				setAttachedSound(audio_uuid, owner_id, gain, sound_flags);
				U8 old_material = getMaterial();
				if (old_material != material)
				{
					setMaterial(material);
					if (mDrawable.notNull())
					{
						gPipeline.markMoved(mDrawable, FALSE);
					}
				}
				setClickAction(click_action);
				U32 flags;
				mesgsys->getU32Fast(_PREHASH_ObjectData, _PREHASH_UpdateFlags, flags, block_num);
				mFlags &= FLAGS_LOCAL;
				mFlags |= flags;
				U8 state;
				mesgsys->getU8Fast(_PREHASH_ObjectData, _PREHASH_State, state, block_num);
				mAttachmentState = state;
				mCreateSelected = ((flags & FLAGS_CREATE_SELECTED) != 0);
				S32 nv_size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_NameValue);
				if (nv_size > 0)
				{
					std::string name_value_list;
					mesgsys->getStringFast(_PREHASH_ObjectData, _PREHASH_NameValue, name_value_list, block_num);
					setNameValueList(name_value_list);
				}
				if (mData)
				{
					delete [] mData;
				}
				S32 data_size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_Data);
				if (data_size <= 0)
				{
					mData = NULL;
				}
				else
				{
					mData = new U8[data_size];
					mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_Data, mData, data_size, block_num);
				}
				mHudTextString.clear();
				mHudTextColor = LLColor4U::white;
				S32 text_size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_Text);
				if (text_size > 1)
				{
					if (!mText)
					{
						initHudText();
					}
					mesgsys->getStringFast(_PREHASH_ObjectData, _PREHASH_Text, mHudTextString, block_num );
					LLColor4U coloru;
					mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextColor, coloru.mV, 4, block_num);
					coloru.mV[3] = 255 - coloru.mV[3];
					mHudTextColor = LLColor4(coloru);
					if(mText->getDoFade())
					{
						mText->setColor(mHudTextColor);
						mText->setString(mHudTextString);
					}
					if (rlv_handler_t::isEnabled())
					{
						mText->setObjectText(mHudTextString);
					}
					setChanged(MOVED | SILHOUETTE);
				}
				else if (mText.notNull())
				{
					mText->markDead();
					mText = NULL;
				}
				std::string media_url;
				mesgsys->getStringFast(_PREHASH_ObjectData, _PREHASH_MediaURL, media_url, block_num);
				retval |= checkMediaURL(media_url);
				unpackParticleSource(block_num, owner_id);
				for (auto& entry : mExtraParameterList)
				{
					if (entry.in_use) *entry.in_use = false;
				}
				S32 size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_ExtraParams);
				if (size > 0)
				{
					U8 *buffer = new U8[size];
					mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_ExtraParams, buffer, size, block_num);
					LLDataPackerBinaryBuffer dp(buffer, size);
					U8 num_parameters;
					dp.unpackU8(num_parameters, "num_params");
					U8 param_block[MAX_OBJECT_PARAMS_SIZE];
					for (U8 param=0; param<num_parameters; ++param)
					{
						U16 param_type;
						S32 param_size;
						dp.unpackU16(param_type, "param_type");
						dp.unpackBinaryData(param_block, param_size, "param_data");
						LLDataPackerBinaryBuffer dp2(param_block, param_size);
						unpackParameterEntry(param_type, &dp2);
					}
					delete[] buffer;
				}
				for (size_t i = 0; i < mExtraParameterList.size(); ++i)
				{
					auto& entry = mExtraParameterList[i];
					if (entry.in_use && !*entry.in_use)
					{
						parameterChanged((i + 1) << 4, entry.data, FALSE, false);
					}
				}
				break;
			}
		case OUT_TERSE_IMPROVED:
			{
#ifdef DEBUG_UPDATE_TYPE
				LL_INFOS() << "TI:" << getID() << LL_ENDL;
#endif
				processTerseData(mesgsys, user_data, block_num, this_update_precision, new_pos_parent, new_rot, new_angv, test_pos_parent);
				break;
			}
		default:
			break;
		}
	}
	else
	{
		LLUUID sound_uuid;
		LLUUID	owner_id;
		F32    gain = 0;
		U8     sound_flags = 0;
		F32		cutoff = 0;
		U16 val[4];
		U8		state;
		dp->unpackU8(state, "State");
		mAttachmentState = state;
		switch(update_type)
		{
			case OUT_TERSE_IMPROVED:
			{
#ifdef DEBUG_UPDATE_TYPE
				LL_INFOS() << "CompTI:" << getID() << LL_ENDL;
#endif
				U8		value;
				dp->unpackU8(value, "agent");
				if (value)
				{
					LLVector4 collision_plane;
					dp->unpackVector4(collision_plane, "Plane");
					((LLVOAvatar*)this)->setFootPlane(collision_plane);
				}
				test_pos_parent = getPosition();
				dp->unpackVector3(new_pos_parent, "Pos");
				dp->unpackU16(val[VX], "VelX");
				dp->unpackU16(val[VY], "VelY");
				dp->unpackU16(val[VZ], "VelZ");
				setVelocity(U16_to_F32(val[VX], -128.f, 128.f),
							U16_to_F32(val[VY], -128.f, 128.f),
							U16_to_F32(val[VZ], -128.f, 128.f));
				dp->unpackU16(val[VX], "AccX");
				dp->unpackU16(val[VY], "AccY");
				dp->unpackU16(val[VZ], "AccZ");
				setAcceleration(U16_to_F32(val[VX], -64.f, 64.f),
								U16_to_F32(val[VY], -64.f, 64.f),
								U16_to_F32(val[VZ], -64.f, 64.f));
				dp->unpackU16(val[VX], "ThetaX");
				dp->unpackU16(val[VY], "ThetaY");
				dp->unpackU16(val[VZ], "ThetaZ");
				dp->unpackU16(val[VS], "ThetaS");
				new_rot.mQ[VX] = U16_to_F32(val[VX], -1.f, 1.f);
				new_rot.mQ[VY] = U16_to_F32(val[VY], -1.f, 1.f);
				new_rot.mQ[VZ] = U16_to_F32(val[VZ], -1.f, 1.f);
				new_rot.mQ[VS] = U16_to_F32(val[VS], -1.f, 1.f);
				dp->unpackU16(val[VX], "AccX");
				dp->unpackU16(val[VY], "AccY");
				dp->unpackU16(val[VZ], "AccZ");
				new_angv.set(U16_to_F32(val[VX], -64.f, 64.f),
							 U16_to_F32(val[VY], -64.f, 64.f),
							 U16_to_F32(val[VZ], -64.f, 64.f));
				setAngularVelocity(new_angv);
			}
			break;
			case OUT_FULL_COMPRESSED:
			case OUT_FULL_CACHED:
			{
#ifdef DEBUG_UPDATE_TYPE
				LL_INFOS() << "CompFull:" << getID() << LL_ENDL;
#endif
				mCostStale = true;
				if (isSelected())
				{
					gFloaterTools->dirty();
				}
				dp->unpackU32(crc, "CRC");
				mTotalCRC = crc;
				dp->unpackU8(material, "Material");
				U8 old_material = getMaterial();
				if (old_material != material)
				{
					setMaterial(material);
					if (mDrawable.notNull())
					{
						gPipeline.markMoved(mDrawable, FALSE);
					}
				}
				dp->unpackU8(click_action, "ClickAction");
				setClickAction(click_action);
				dp->unpackVector3(new_scale, "Scale");
				dp->unpackVector3(new_pos_parent, "Pos");
				LLVector3 vec;
				dp->unpackVector3(vec, "Rot");
				new_rot.unpackFromVector3(vec);
				setAcceleration(LLVector3::zero);
				U32 value;
				dp->unpackU32(value, "SpecialCode");
				dp->setPassFlags(value);
				dp->unpackUUID(owner_id, "Owner");
				mOwnerID = owner_id;
				if (value & 0x80)
				{
					dp->unpackVector3(new_angv, "Omega");
					setAngularVelocity(new_angv);
				}
				if (value & 0x20)
				{
					dp->unpackU32(parent_id, "ParentID");
				}
				else
				{
					parent_id = 0;
				}
				S32 sp_size;
				U32 size;
				if (value & 0x2)
				{
					sp_size = 1;
					delete [] mData;
					mData = new U8[1];
					dp->unpackU8(((U8*)mData)[0], "TreeData");
				}
				else if (value & 0x1)
				{
					dp->unpackU32(size, "ScratchPadSize");
					delete [] mData;
					mData = new U8[size];
					dp->unpackBinaryData((U8 *)mData, sp_size, "PartData");
				}
				else
				{
					mData = NULL;
				}
				mHudTextString.clear();
				mHudTextColor = LLColor4U::white;
				if (!mText && (value & 0x4))
				{
					mText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
					mText->setFont(LLFontGL::getFontSansSerif());
					mText->setVertAlignment(LLHUDText::ALIGN_VERT_TOP);
					mText->setMaxLines(-1);
					mText->setSourceObject(this);
					mText->setOnHUDAttachment(isHUDAttachment());
				}
				if (value & 0x4)
				{
					dp->unpackString(mHudTextString, "Text");
					LLColor4U coloru;
					dp->unpackBinaryDataFixed(coloru.mV, 4, "Color");
					coloru.mV[3] = 255 - coloru.mV[3];
					mHudTextColor = LLColor4(coloru);
					if(mText->getDoFade())
					mText->setColor(mHudTextColor);
					mText->setString(mHudTextString);
					if (rlv_handler_t::isEnabled())
					{
						mText->setObjectText(mHudTextString);
					}
					setChanged(TEXTURE);
				}
				else if(mText.notNull())
				{
					mText->markDead();
					mText = NULL;
				}
				std::string media_url;
				if (value & 0x200)
				{
					dp->unpackString(media_url, "MediaURL");
				}
				retval |= checkMediaURL(media_url);
				if (value & 0x8)
				{
					unpackParticleSource(*dp, owner_id, true);
				}
				else if (!(value & 0x400))
				{
					deleteParticleSource();
				}
				for (auto& entry : mExtraParameterList)
				{
					if (entry.in_use) *entry.in_use = false;
				}
				U8 num_parameters;
				dp->unpackU8(num_parameters, "num_params");
				U8 param_block[MAX_OBJECT_PARAMS_SIZE];
				for (U8 param=0; param<num_parameters; ++param)
				{
					U16 param_type;
					S32 param_size;
					dp->unpackU16(param_type, "param_type");
					dp->unpackBinaryData(param_block, param_size, "param_data");
					LLDataPackerBinaryBuffer dp2(param_block, param_size);
					unpackParameterEntry(param_type, &dp2);
				}
				for (size_t i = 0; i < mExtraParameterList.size(); ++i)
				{
					auto& entry = mExtraParameterList[i];
					if (entry.in_use && !*entry.in_use)
					{
						parameterChanged((i + 1) << 4, entry.data, FALSE, false);
					}
				}
				if (value & 0x10)
				{
					dp->unpackUUID(sound_uuid, "SoundUUID");
					dp->unpackF32(gain, "SoundGain");
					dp->unpackU8(sound_flags, "SoundFlags");
					dp->unpackF32(cutoff, "SoundRadius");
				}
				if (value & 0x100)
				{
					std::string name_value_list;
					dp->unpackString(name_value_list, "NV");
					setNameValueList(name_value_list);
				}
				mTotalCRC = crc;
				setAttachedSound(sound_uuid, owner_id, gain, sound_flags);
				if(mesgsys != NULL)
				{
					U32 flags;
					mesgsys->getU32Fast(_PREHASH_ObjectData, _PREHASH_UpdateFlags, flags, block_num);
					loadFlags(flags);
				}
			}
			break;
		default:
			break;
		}
	}
	BOOL b_changed_status = FALSE;
	if (OUT_TERSE_IMPROVED != update_type)
	{
		if (!cur_parentp)
		{
			if (parent_id == 0)
			{
			}
			else
			{
				LLUUID parent_uuid;
				if(mesgsys != NULL)
				{
					LLViewerObjectList::getUUIDFromLocal(parent_uuid,
														parent_id,
														mesgsys->getSenderIP(),
														mesgsys->getSenderPort());
				}
				else
				{
					LLViewerObjectList::getUUIDFromLocal(parent_uuid,
														parent_id,
														mRegionp->getHost().getAddress(),
														mRegionp->getHost().getPort());
				}
				LLViewerObject *sent_parentp = gObjectList.findObject(parent_uuid);
				if(isAttachment())
				{
					llassert_always(gAgentID.notNull());
					if(parent_uuid == gAgentID)
					{
						static std::map<LLUUID,bool> state_map;
						std::map<LLUUID,bool>::iterator it = state_map.find(getID());
						if(it != state_map.end() && (!it->second && sent_parentp))
						{
							it->second = sent_parentp != NULL;
							LL_INFOS("Attachment") << getID() << " ("<<getAttachmentPointName()<<") has " << (sent_parentp ? "" : "no ") << "parent." << LL_ENDL;
						}
					}
				}
				if (sent_parentp && sent_parentp->getParent() == this)
				{
					LL_WARNS() << "Attempt to attach a parent to it's child: " << this->getID() << " to " << sent_parentp->getID() << LL_ENDL;
					this->removeChild(sent_parentp);
					sent_parentp->setDrawableParent(NULL);
				}
				if (sent_parentp && (sent_parentp != this) && !sent_parentp->isDead())
				{
					if (((LLViewerObject*)sent_parentp)->isAvatar())
					{
					}
					b_changed_status = TRUE;
					if (mDrawable.notNull())
					{
						if (mDrawable->isDead() || !mDrawable->getVObj())
						{
							LL_WARNS() << "Drawable is dead or no VObj!" << LL_ENDL;
							sent_parentp->addChild(this);
						}
						else
						{
							if (!setDrawableParent(sent_parentp->mDrawable))
							{
								LL_WARNS() << "Attempting to recover from parenting cycle!" << LL_ENDL;
								LL_WARNS() << "Killing " << sent_parentp->getID() << " and " << getID() << LL_ENDL;
								LL_WARNS() << "Adding to cache miss list" << LL_ENDL;
								setParent(NULL);
								sent_parentp->setParent(NULL);
								getRegion()->addCacheMissFull(getLocalID());
								getRegion()->addCacheMissFull(sent_parentp->getLocalID());
								gObjectList.killObject(sent_parentp);
								gObjectList.killObject(this);
								return retval;
							}
							if ( (rlv_handler_t::isEnabled()) && (sent_parentp->isAvatar()) && (sent_parentp->getID() == gAgent.getID()) )
							{
								S32 idxAttachPt = ATTACHMENT_ID_FROM_STATE(getAttachmentState());
								if (gRlvAttachmentLocks.isLockedAttachmentPoint(idxAttachPt, RLV_LOCK_ADD))
								{
									LLNameValue* nvItem = getNVPair("AttachItemID");
									if (nvItem)
									{
										LLUUID idItem(nvItem->getString());
										if (idItem.notNull())
											RlvAttachmentLockWatchdog::instance().onWearAttachment(idItem, RLV_WEAR_ADD);
									}
								}
							}
							sent_parentp->addChild(this);
							if (sent_parentp->mDrawable.notNull())
							{
								gPipeline.markMoved(sent_parentp->mDrawable, FALSE);
							}
						}
					}
					else
					{
						sent_parentp->addChild(this);
					}
					hideExtraDisplayItems( FALSE );
					setChanged(MOVED | SILHOUETTE);
				}
				else
				{
					U32 ip, port;
					if(mesgsys != NULL)
					{
						ip = mesgsys->getSenderIP();
						port = mesgsys->getSenderPort();
					}
					else
					{
						ip = mRegionp->getHost().getAddress();
						port = mRegionp->getHost().getPort();
					}
					gObjectList.orphanize(this, parent_id, ip, port);
					hideExtraDisplayItems( TRUE );
				}
			}
		}
		else
		{
			if (  (parent_id == cur_parentp->mLocalID)
				&&(update_type == OUT_TERSE_IMPROVED))
			{
			}
			else
			{
				LLViewerObject *sent_parentp;
				if (parent_id == 0)
				{
					sent_parentp = NULL;
				}
				else
				{
					LLUUID parent_uuid;
					if(mesgsys != NULL)
					{
						LLViewerObjectList::getUUIDFromLocal(parent_uuid,
														parent_id,
														gMessageSystem->getSenderIP(),
														gMessageSystem->getSenderPort());
					}
					else
					{
						LLViewerObjectList::getUUIDFromLocal(parent_uuid,
														parent_id,
														mRegionp->getHost().getAddress(),
														mRegionp->getHost().getPort());
					}
					sent_parentp = gObjectList.findObject(parent_uuid);
					if (isAvatar())
					{
						if (!sent_parentp)
						{
							sent_parentp = cur_parentp;
						}
					}
					else if (!sent_parentp)
					{
						U32 ip, port;
						if(mesgsys != NULL)
						{
							ip = mesgsys->getSenderIP();
							port = mesgsys->getSenderPort();
						}
						else
						{
							ip = mRegionp->getHost().getAddress();
							port = mRegionp->getHost().getPort();
						}
						gObjectList.orphanize(this, parent_id, ip, port);
					}
				}
				if (sent_parentp && sent_parentp != cur_parentp && sent_parentp != this)
				{
					b_changed_status = TRUE;
					if (mDrawable.notNull())
					{
						if (!setDrawableParent(sent_parentp->mDrawable))
						{
							LL_WARNS() << "Attempting to recover from parenting cycle!" << LL_ENDL;
							LL_WARNS() << "Killing " << sent_parentp->getID() << " and " << getID() << LL_ENDL;
							LL_WARNS() << "Adding to cache miss list" << LL_ENDL;
							setParent(NULL);
							sent_parentp->setParent(NULL);
							getRegion()->addCacheMissFull(getLocalID());
							getRegion()->addCacheMissFull(sent_parentp->getLocalID());
							gObjectList.killObject(sent_parentp);
							gObjectList.killObject(this);
							return retval;
						}
					}
					cur_parentp->removeChild(this);
					sent_parentp->addChild(this);
					setChanged(MOVED | SILHOUETTE);
					sent_parentp->setChanged(MOVED | SILHOUETTE);
					if (sent_parentp->mDrawable.notNull())
					{
						gPipeline.markMoved(sent_parentp->mDrawable, FALSE);
					}
				}
				else if (!sent_parentp)
				{
					bool remove_parent = true;
					LLViewerObject *parentp = (LLViewerObject *)getParent();
					if (parentp)
					{
						if (parentp->getRegion() != getRegion())
						{
							remove_parent = false;
						}
					}
					if (remove_parent)
					{
						b_changed_status = TRUE;
						if (mDrawable.notNull())
						{
							setDrawableParent(NULL);
						}
						cur_parentp->removeChild(this);
						setChanged(MOVED | SILHOUETTE);
						if (mDrawable.notNull())
						{
							gPipeline.markMoved(mDrawable, FALSE);
						}
					}
				}
			}
		}
	}
	new_rot.normQuat();
	if (sPingInterpolate && mesgsys != NULL)
	{
		LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(mesgsys->getSender());
		if (cdp)
		{
			F32 ping_delay = 0.5f * time_dilation * ( ((F32)cdp->getPingDelay().valueInUnits<LLUnits::Seconds>()) + gFrameDTClamped);
			LLVector3 diff = getVelocity() * ping_delay;
			new_pos_parent += diff;
		}
		else
		{
			LL_WARNS() << "findCircuit() returned NULL; skipping interpolation" << LL_ENDL;
		}
	}
	if(mesgsys != NULL)
	{
		U32 packet_id = mesgsys->getCurrentRecvPacketID();
		if (packet_id < mLatestRecvPacketID &&
			mLatestRecvPacketID - packet_id < 65536)
		{
			return retval;
		}
		mLatestRecvPacketID = packet_id;
	}
	if (new_scale != getScale())
	{
		setChanged(SCALED | SILHOUETTE);
		setScale(new_scale);
	}
	F32 vel_mag_sq = getVelocity().magVecSquared();
	F32 accel_mag_sq = getAcceleration().magVecSquared();
	if (  ((b_changed_status)||(test_pos_parent != new_pos_parent))
		||(  (!isSelected())
		   &&(  (vel_mag_sq != 0.f)
			  ||(accel_mag_sq != 0.f)
			  ||(this_update_precision > mBestUpdatePrecision))))
	{
		mBestUpdatePrecision = this_update_precision;
		LLVector3 diff = new_pos_parent - test_pos_parent ;
		F32 mag_sqr = diff.magVecSquared() ;
		if(std::isfinite(mag_sqr))
		{
			setPositionParent(new_pos_parent);
		}
		else
		{
			LL_WARNS() << "Can not move the object/avatar to an infinite location!" << LL_ENDL ;
			retval |= INVALID_UPDATE ;
		}
		if (mParent && ((LLViewerObject*)mParent)->isAvatar())
		{
			LLVOAvatar *avatar = (LLVOAvatar*)mParent;
			avatar->clampAttachmentPositions();
		}
		if ( asAvatar() && asAvatar()->isSelf() && (mag_sqr > 0.25f) )
		{
			LLViewerStats::getInstance()->mAgentPositionSnaps.push( diff.length() );
		}
	}
	static const LLCachedControl<bool> use_new_target_omega ("UseNewTargetOmegaCode", true);
	if (use_new_target_omega)
	{
		if ((new_rot != getRotation()) || (new_angv != old_angv))
		{
			if (new_rot != mPreviousRotation)
			{
				resetRot();
			}
			else if (new_angv != old_angv)
			{
				if (flagUsePhysics())
				{
					resetRot();
				}
				else
				{
					mRotTime = 0.0f;
				}
			}
			mPreviousRotation = new_rot;
			setRotation(new_rot * mAngularVelocityRot);
			setChanged(ROTATED | SILHOUETTE);
		}
	}
	else
	{
		if (new_rot != mPreviousRotation || new_angv != old_angv)
		{
			if (new_rot != mPreviousRotation)
			{
				mPreviousRotation = new_rot;
				setRotation(new_rot);
			}
			setChanged(ROTATED | SILHOUETTE);
			resetRot();
		}
	}
	if ( gShowObjectUpdates )
	{
		LLColor4 color;
		if (update_type == OUT_TERSE_IMPROVED)
		{
			color.setVec(0.f, 0.f, 1.f, 1.f);
		}
		else
		{
			color.setVec(1.f, 0.f, 0.f, 1.f);
		}
		gPipeline.addDebugBlip(getPositionAgent(), color);
	}
	const F32 MAG_CUTOFF = F_APPROXIMATELY_ZERO;
	llassert(vel_mag_sq >= 0.f);
	llassert(accel_mag_sq >= 0.f);
	llassert(getAngularVelocity().magVecSquared() >= 0.f);
	if ((MAG_CUTOFF >= vel_mag_sq) &&
		(MAG_CUTOFF >= accel_mag_sq) &&
		(MAG_CUTOFF >= getAngularVelocity().magVecSquared()))
	{
		mStatic = TRUE;
	}
	else
	{
		mStatic = FALSE;
	}
	BOOL needs_refresh = mUserSelected;
	for (auto& iter : mChildList)
	{
		LLViewerObject* child = iter;
		needs_refresh = needs_refresh || child->mUserSelected;
	}
	if (needs_refresh)
	{
		if(isChanged(MOVED))
		LLSelectMgr::getInstance()->updateSelectionCenter();
		dialog_refresh_all();
	}
	mLastInterpUpdateSecs = LLFrameTimer::getElapsedSeconds();
	mLastMessageUpdateSecs = mLastInterpUpdateSecs;
	if (mDrawable.notNull())
	{
		if (mDrawable->isState(LLDrawable::FORCE_INVISIBLE) && !mOrphaned)
		{
			mDrawable->clearState(LLDrawable::FORCE_INVISIBLE);
			gPipeline.markRebuild( mDrawable, LLDrawable::REBUILD_ALL, TRUE );
		}
	}
	bool special_hover_cursor = specialHoverCursor();
	if (old_special_hover_cursor != special_hover_cursor
		&& mDrawable.notNull())
	{
		mDrawable->updateSpecialHoverCursor(special_hover_cursor);
	}
	return retval;
}
void LLViewerObject::processTerseData(LLMessageSystem *mesgsys, void **user_data, U32 block_num, S32& this_update_precision, LLVector3& new_pos_parent, LLQuaternion& new_rot, LLVector3& new_angv, LLVector3& test_pos_parent)
{
	const F32 size = mRegionp->getWidth();
	const F32 MAX_HEIGHT = LLWorld::getInstance()->getRegionMaxHeight();
	const F32 MIN_HEIGHT = LLWorld::getInstance()->getRegionMinHeight();
	U8  data[MAX_OBJECT_PARAMS_SIZE];
#ifdef LL_BIG_ENDIAN
	U16 valswizzle[4];
#endif
	U16	*val;
	S32 count = 0;
	LLVector4 collision_plane;
	S32 length = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_ObjectData);
	mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_ObjectData, data, length, block_num, MAX_OBJECT_PARAMS_SIZE);
	switch (length)
	{
	case(60 + 16) :
		htonmemcpy(collision_plane.mV, &data[count], MVT_LLVector4, sizeof(LLVector4));
		((LLVOAvatar*)this)->setFootPlane(collision_plane);
		count += sizeof(LLVector4);
	case 60:
		this_update_precision = 32;
		htonmemcpy(new_pos_parent.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
		count += sizeof(LLVector3);
		htonmemcpy((void*)getVelocity().mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
		count += sizeof(LLVector3);
		htonmemcpy((void*)getAcceleration().mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
		count += sizeof(LLVector3);
		{
			LLVector3 vec;
			htonmemcpy(vec.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
			new_rot.unpackFromVector3(vec);
		}
		count += sizeof(LLVector3);
		htonmemcpy((void*)new_angv.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
		if (new_angv.isExactlyZero())
		{
			resetRot();
		}
		setAngularVelocity(new_angv);
		break;
	case(32 + 16) :
		htonmemcpy(collision_plane.mV, &data[count], MVT_LLVector4, sizeof(LLVector4));
		((LLVOAvatar*)this)->setFootPlane(collision_plane);
		count += sizeof(LLVector4);
	case 32:
		this_update_precision = 16;
		test_pos_parent.quantize16(-0.5f*size, 1.5f*size, MIN_HEIGHT, MAX_HEIGHT);
#ifdef LL_BIG_ENDIAN
		htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6);
		val = valswizzle;
#else
		val = (U16 *)&data[count];
#endif
		count += sizeof(U16) * 3;
		new_pos_parent.mV[VX] = U16_to_F32(val[VX], -0.5f*size, 1.5f*size);
		new_pos_parent.mV[VY] = U16_to_F32(val[VY], -0.5f*size, 1.5f*size);
		new_pos_parent.mV[VZ] = U16_to_F32(val[VZ], MIN_HEIGHT, MAX_HEIGHT);
#ifdef LL_BIG_ENDIAN
		htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6);
		val = valswizzle;
#else
		val = (U16 *)&data[count];
#endif
		count += sizeof(U16) * 3;
		setVelocity(U16_to_F32(val[VX], -size, size),
			U16_to_F32(val[VY], -size, size),
			U16_to_F32(val[VZ], -size, size));
#ifdef LL_BIG_ENDIAN
		htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6);
		val = valswizzle;
#else
		val = (U16 *)&data[count];
#endif
		count += sizeof(U16) * 3;
		setAcceleration(U16_to_F32(val[VX], -size, size),
			U16_to_F32(val[VY], -size, size),
			U16_to_F32(val[VZ], -size, size));
#ifdef LL_BIG_ENDIAN
		htonmemcpy(valswizzle, &data[count], MVT_U16Quat, 8);
		val = valswizzle;
#else
		val = (U16 *)&data[count];
#endif
		count += sizeof(U16) * 4;
		new_rot.mQ[VX] = U16_to_F32(val[VX], -1.f, 1.f);
		new_rot.mQ[VY] = U16_to_F32(val[VY], -1.f, 1.f);
		new_rot.mQ[VZ] = U16_to_F32(val[VZ], -1.f, 1.f);
		new_rot.mQ[VW] = U16_to_F32(val[VW], -1.f, 1.f);
#ifdef LL_BIG_ENDIAN
		htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6);
		val = valswizzle;
#else
		val = (U16 *)&data[count];
#endif
		new_angv.set(U16_to_F32(val[VX], -size, size),
			U16_to_F32(val[VY], -size, size),
			U16_to_F32(val[VZ], -size, size));
		if (new_angv.isExactlyZero())
		{
			resetRot();
		}
		setAngularVelocity(new_angv);
		break;
	case 16:
		this_update_precision = 8;
		test_pos_parent.quantize8(-0.5f*size, 1.5f*size, MIN_HEIGHT, MAX_HEIGHT);
		new_pos_parent.mV[VX] = U8_to_F32(data[0], -0.5f*size, 1.5f*size);
		new_pos_parent.mV[VY] = U8_to_F32(data[1], -0.5f*size, 1.5f*size);
		new_pos_parent.mV[VZ] = U8_to_F32(data[2], MIN_HEIGHT, MAX_HEIGHT);
		setVelocity(U8_to_F32(data[3], -size, size),
			U8_to_F32(data[4], -size, size),
			U8_to_F32(data[5], -size, size));
		setAcceleration(U8_to_F32(data[6], -size, size),
			U8_to_F32(data[7], -size, size),
			U8_to_F32(data[8], -size, size));
		new_rot.mQ[VX] = U8_to_F32(data[9], -1.f, 1.f);
		new_rot.mQ[VY] = U8_to_F32(data[10], -1.f, 1.f);
		new_rot.mQ[VZ] = U8_to_F32(data[11], -1.f, 1.f);
		new_rot.mQ[VW] = U8_to_F32(data[12], -1.f, 1.f);
		new_angv.set(U8_to_F32(data[13], -size, size),
			U8_to_F32(data[14], -size, size),
			U8_to_F32(data[15], -size, size));
		if (new_angv.isExactlyZero())
		{
			resetRot();
		}
		setAngularVelocity(new_angv);
		break;
	default:
		break;
	}
	U8 state;
	mesgsys->getU8Fast(_PREHASH_ObjectData, _PREHASH_State, state, block_num);
	mAttachmentState = state;
}
BOOL LLViewerObject::isActive() const
{
	return TRUE;
}
void LLViewerObject::loadFlags(U32 flags)
{
	if(flags == (U32)(-1))
	{
		return;
	}
	mFlags = (mFlags & FLAGS_LOCAL) | flags;
	mCreateSelected = ((flags & FLAGS_CREATE_SELECTED) != 0);
	return;
}
void LLViewerObject::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	if (!mDead)
	{
		if (!mStatic && sVelocityInterpolate && !isSelected())
		{
			F32 time_dilation = mRegionp ? mRegionp->getTimeDilation() : 1.0f;
			F32 dt_raw = ((F64Seconds)time - mLastInterpUpdateSecs).value();
			F32 dt = time_dilation * dt_raw;
			applyAngularVelocity(dt);
			if (isAttachment())
			{
				mLastInterpUpdateSecs = (F64Seconds)time;
				return;
			}
			else
			{
				interpolateLinearMotion(time, dt);
			}
		}
		updateDrawable(FALSE);
	}
}
void LLViewerObject::interpolateLinearMotion(const F64SecondsImplicit& time, const F32SecondsImplicit& dt_seconds)
{
	F32 dt = dt_seconds;
	F64Seconds time_since_last_update = time - mLastMessageUpdateSecs;
	if (time_since_last_update <= (F64Seconds)0.0 || dt <= 0.f)
	{
		return;
	}
	LLVector3 accel = getAcceleration();
	LLVector3 vel 	= getVelocity();
	if (sMaxUpdateInterpolationTime <= (F64Seconds)0.0)
	{
		if (!(accel.isExactlyZero() && vel.isExactlyZero()))
		{
			LLVector3 pos   = (vel + (0.5f * (dt-PHYSICS_TIMESTEP)) * accel) * dt;
			setPositionRegion(pos + getPositionRegion());
			setVelocity(vel + accel*dt);
			setChanged(MOVED | SILHOUETTE);
		}
	}
	else if (!accel.isExactlyZero() || !vel.isExactlyZero())
	{
		LLVector3 new_pos = (vel + (0.5f * (dt-PHYSICS_TIMESTEP)) * accel) * dt;
		LLVector3 new_v = accel * dt;
		if (time_since_last_update > sPhaseOutUpdateInterpolationTime &&
			sPhaseOutUpdateInterpolationTime > (F64Seconds)0.0)
		{
			if (mRegionp)
			{
				LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit( mRegionp->getHost() );
				if (cdp)
				{
					F64Seconds time_since_last_packet = LLMessageSystem::getMessageTimeSeconds() - cdp->getLastPacketInTime();
					if (!cdp->isAlive() ||
						 cdp->isBlocked() ||
						 (time_since_last_packet > sPhaseOutUpdateInterpolationTime))
					{
						F64Seconds time_since_last_interpolation = time - mLastInterpUpdateSecs;
						F64 phase_out = 1.0;
						if (time_since_last_update > sMaxUpdateInterpolationTime)
						{
							phase_out = 0.0;
						}
						else if (mLastInterpUpdateSecs - mLastMessageUpdateSecs > sPhaseOutUpdateInterpolationTime)
						{
							phase_out = (sMaxUpdateInterpolationTime - time_since_last_update) /
										(sMaxUpdateInterpolationTime - time_since_last_interpolation);
						}
						else
						{
							phase_out = (sMaxUpdateInterpolationTime - time_since_last_update) /
										(sMaxUpdateInterpolationTime - sPhaseOutUpdateInterpolationTime);
						}
						phase_out = llclamp(phase_out, 0.0, 1.0);
						new_pos = new_pos * ((F32) phase_out);
						new_v = new_v * ((F32) phase_out);
					}
				}
			}
		}
		new_pos = new_pos + getPositionRegion();
		new_v = new_v + vel;
		LLVector3d new_pos_global = mRegionp->getPosGlobalFromRegion(new_pos);
		F32 min_height;
		if (isAvatar())
		{
			min_height = LLWorld::getInstance()->resolveLandHeightGlobal(new_pos_global);
			min_height += (0.5f * getScale().mV[VZ]);
		}
		else
		{
			min_height = LLWorld::getInstance()->getMinAllowedZ(this, new_pos_global);
		}
		new_pos.mV[VZ] = llmax(min_height, new_pos.mV[VZ]);
		LLVector3 temp(new_pos);
		if (temp.clamp(0.f, mRegionp->getWidth()))
		{
			LLVector3d old_pos_global = mRegionp->getPosGlobalFromRegion(getPositionRegion());
			new_pos_global = mRegionp->getPosGlobalFromRegion(new_pos);
			LLVector3d clip_pos_global = LLWorld::getInstance()->clipToVisibleRegions(old_pos_global, new_pos_global);
			if (clip_pos_global != new_pos_global)
			{
				new_pos = mRegionp->getPosRegionFromGlobal(clip_pos_global);
				new_v.clear();
				setAcceleration(LLVector3::zero);
			}
			else
			{
			}
		}
		setPositionRegion(new_pos);
		setVelocity(new_v);
		setChanged(MOVED | SILHOUETTE);
	}
	mLastInterpUpdateSecs = time;
}
BOOL LLViewerObject::setData(const U8 *datap, const U32 data_size)
{
	delete [] mData;
	if (datap)
	{
		mData = new U8[data_size];
		if (!mData)
		{
			return FALSE;
		}
		memcpy(mData, datap, data_size);
	}
	return TRUE;
}
void LLViewerObject::deleteInventoryItem(const LLUUID& item_id)
{
	if(mInventory)
	{
		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for( ; it != end; ++it )
		{
			if((*it)->getUUID() == item_id)
			{
				mInventory->erase(it);
				return;
			}
		}
		doInventoryCallback();
	}
}
void LLViewerObject::doUpdateInventory(
	LLPointer<LLViewerInventoryItem>& item,
	U8 key,
	bool is_new)
{
	LLViewerInventoryItem* old_item = NULL;
	if(TASK_INVENTORY_ITEM_KEY == key)
	{
		old_item = (LLViewerInventoryItem*)getInventoryObject(item->getUUID());
	}
	else if(TASK_INVENTORY_ASSET_KEY == key)
	{
		old_item = getInventoryItemByAsset(item->getAssetUUID());
	}
	LLUUID item_id;
	LLUUID new_owner;
	LLUUID new_group;
	BOOL group_owned = FALSE;
	if(old_item)
	{
		item_id = old_item->getUUID();
		new_owner = old_item->getPermissions().getOwner();
		new_group = old_item->getPermissions().getGroup();
		group_owned = old_item->getPermissions().isGroupOwned();
		old_item = NULL;
	}
	else
	{
		item_id = item->getUUID();
	}
	if(!is_new && mInventory)
	{
		deleteInventoryItem(item_id);
		LLPermissions perm(item->getPermissions());
		LLPermissions* obj_perm = LLSelectMgr::getInstance()->findObjectPermissions(this);
		bool is_atomic = ((S32)LLAssetType::AT_OBJECT == item->getType()) ? false : true;
		if(obj_perm)
		{
			perm.setOwnerAndGroup(LLUUID::null, obj_perm->getOwner(), obj_perm->getGroup(), is_atomic);
		}
		else
		{
			if(group_owned)
			{
				perm.setOwnerAndGroup(LLUUID::null, new_owner, new_group, is_atomic);
			}
			else if(!new_owner.isNull())
			{
				perm.setOwnerAndGroup(LLUUID::null, new_owner, new_group, is_atomic);
			}
			else if(permYouOwner())
			{
				perm.setOwnerAndGroup(LLUUID::null, gAgent.getID(), item->getPermissions().getGroup(), is_atomic);
				--mInventorySerialNum;
			}
			else
			{
				perm.setOwnerAndGroup(LLUUID::null, LLUUID::null, LLUUID::null, is_atomic);
				--mInventorySerialNum;
			}
		}
		LLViewerInventoryItem* oldItem = item;
		LLViewerInventoryItem* new_item = new LLViewerInventoryItem(oldItem);
		new_item->setPermissions(perm);
		mInventory->push_front(new_item);
		doInventoryCallback();
		++mInventorySerialNum;
	}
}
void LLViewerObject::saveScript(
	const LLViewerInventoryItem* item,
	BOOL active,
	bool is_new)
{
	LL_DEBUGS() << "LLViewerObject::saveScript() " << item->getUUID() << " " << item->getAssetUUID() << LL_ENDL;
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(), item->getFlags(),
								  item->getCreationDate());
	task_item->setTransactionID(item->getTransactionID());
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RezScript);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
	msg->nextBlockFast(_PREHASH_UpdateBlock);
	msg->addU32Fast(_PREHASH_ObjectLocalID, (mLocalID));
	U8 enabled = active;
	msg->addBOOLFast(_PREHASH_Enabled, enabled);
	msg->nextBlockFast(_PREHASH_InventoryBlock);
	task_item->packMessage(msg);
	msg->sendReliable(mRegionp->getHost());
	doUpdateInventory(task_item, TASK_INVENTORY_ITEM_KEY, is_new);
}
void LLViewerObject::moveInventory(const LLUUID& folder_id,
								   const LLUUID& item_id)
{
	LL_DEBUGS() << "LLViewerObject::moveInventory " << item_id << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoveTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_FolderID, folder_id);
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addU32Fast(_PREHASH_LocalID, mLocalID);
	msg->addUUIDFast(_PREHASH_ItemID, item_id);
	msg->sendReliable(mRegionp->getHost());
	LLInventoryObject* inv_obj = getInventoryObject(item_id);
	if(inv_obj)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_obj;
		if(!item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			deleteInventoryItem(item_id);
			++mInventorySerialNum;
		}
	}
}
void LLViewerObject::dirtyInventory()
{
	if(mInventory && !mInventoryCallbacks.empty())
	{
		mInventory->clear();
		delete mInventory;
		mInventory = NULL;
	}
	mInventoryDirty = TRUE;
}
void LLViewerObject::registerInventoryListener(LLVOInventoryListener* listener, void* user_data)
{
	LLInventoryCallbackInfo* info = new LLInventoryCallbackInfo;
	info->mListener = listener;
	info->mInventoryData = user_data;
	mInventoryCallbacks.push_front(info);
}
void LLViewerObject::removeInventoryListener(LLVOInventoryListener* listener)
{
	if (listener == NULL)
		return;
	for (callback_list_t::iterator iter = mInventoryCallbacks.begin();
		 iter != mInventoryCallbacks.end(); )
	{
		callback_list_t::iterator curiter = iter++;
		LLInventoryCallbackInfo* info = *curiter;
		if (info->mListener == listener)
		{
			delete info;
			mInventoryCallbacks.erase(curiter);
			break;
		}
	}
}
BOOL LLViewerObject::isInventoryPending()
{
	return mInvRequestState != INVENTORY_REQUEST_STOPPED;
}
void LLViewerObject::clearInventoryListeners()
{
	for_each(mInventoryCallbacks.begin(), mInventoryCallbacks.end(), DeletePointer());
	mInventoryCallbacks.clear();
}
bool LLViewerObject::hasInventoryListeners()
{
	return !mInventoryCallbacks.empty();
}
void LLViewerObject::requestInventory()
{
	if(mInventoryDirty && mInventory && !mInventoryCallbacks.empty())
	{
		mInventory->clear();
		delete mInventory;
		mInventory = NULL;
	}
	if(mInventory)
	{
		doInventoryCallback();
	}
	else
	{
		mInventoryDirty = FALSE;
		fetchInventoryFromServer();
	}
}
void LLViewerObject::fetchInventoryFromServer()
{
	if (!isInventoryPending())
	{
		delete mInventory;
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_RequestTaskInventory);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_InventoryData);
		msg->addU32Fast(_PREHASH_LocalID, mLocalID);
		msg->sendReliable(mRegionp->getHost());
		mInvRequestState = INVENTORY_REQUEST_PENDING;
	}
}
LLControlAvatar *LLViewerObject::getControlAvatar()
{
	return getRootEdit()->mControlAvatar.get();
}
LLControlAvatar *LLViewerObject::getControlAvatar() const
{
	return getRootEdit()->mControlAvatar.get();
}
void LLViewerObject::updateControlAvatar()
{
	LLViewerObject *root = getRootEdit();
	bool is_animated_object = root->isAnimatedObject();
	bool has_control_avatar = getControlAvatar();
	if (!is_animated_object && !has_control_avatar)
	{
		return;
	}
	bool should_have_control_avatar = false;
	if (is_animated_object)
	{
		bool any_rigged_mesh = root->isRiggedMesh();
		LLViewerObject::const_child_list_t& child_list = root->getChildren();
        for (const auto& iter : child_list)
		{
            const LLViewerObject* child = iter;
			any_rigged_mesh = any_rigged_mesh || child->isRiggedMesh();
		}
		should_have_control_avatar = is_animated_object && any_rigged_mesh;
	}
	if (should_have_control_avatar && !has_control_avatar)
	{
		std::string vobj_name = llformat("Vol%p", root);
		LL_DEBUGS("AnimatedObjects") << vobj_name << " calling linkControlAvatar()" << LL_ENDL;
		root->linkControlAvatar();
	}
	if (!should_have_control_avatar && has_control_avatar)
	{
		std::string vobj_name = llformat("Vol%p", root);
		LL_DEBUGS("AnimatedObjects") << vobj_name << " calling unlinkControlAvatar()" << LL_ENDL;
		root->unlinkControlAvatar();
	}
	if (getControlAvatar())
	{
		getControlAvatar()->updateAnimations();
		if (isSelected())
		{
			LLSelectMgr::getInstance()->pauseAssociatedAvatars();
		}
	}
}
void LLViewerObject::print()
{
	if (!mChildList.size())
		return;
	LL_INFOS() << "==============" << LL_ENDL;
	LL_INFOS() << mID << LL_ENDL;
	LL_INFOS() << "isAvatar() " << isAvatar() << LL_ENDL;
	LL_INFOS() << "  mOwnerID " << mOwnerID << LL_ENDL;
	LL_INFOS() << "  mRegionp " << std::hex << mRegionp << std::dec << LL_ENDL;
	if (mRegionp)
	{
		LL_INFOS() << "    getName() " << mRegionp->getName() << LL_ENDL;
		LL_INFOS() << "    isAlive() " << mRegionp->isAlive() << LL_ENDL;
	}
	LL_INFOS() << "  mDead " << mDead << LL_ENDL;
	LL_INFOS() << "  mDrawable " << std::hex << mDrawable.get() << std::dec << LL_ENDL;
	if (mDrawable)
	{
		LL_INFOS() << "    isVisible() " << mDrawable->isVisible() << LL_ENDL;
		LL_INFOS() << "    getRegion() " << std::hex << mDrawable->getRegion() << std::dec << LL_ENDL;
		if (mDrawable->getRegion())
		{
			LL_INFOS() << "      getRegion() " << mDrawable->getRegion()->getName() << LL_ENDL;
		}
		LL_INFOS() << "    isRoot() " << mDrawable->isRoot() << LL_ENDL;
		LL_INFOS() << "    isDead() " << mDrawable->isDead() << LL_ENDL;
		LL_INFOS() << "    isNew() " << mDrawable->isNew() << LL_ENDL;
		LL_INFOS() << "    isUnload() " << mDrawable->isUnload() << LL_ENDL;
		LL_INFOS() << "    isLight() " << mDrawable->isLight() << LL_ENDL;
		LL_INFOS() << "    getSpatialGroup() " << mDrawable->getSpatialGroup() << LL_ENDL;
	}
	LL_INFOS() << "  mOrphaned " << mOrphaned << LL_ENDL;
	LL_INFOS() << "  isAttachment() " << isAttachment() << LL_ENDL;
	LL_INFOS() << "  isActive() " << isActive() << LL_ENDL;
	LL_INFOS() << "  isRiggedMesh() " << isRiggedMesh() << LL_ENDL;
	LL_INFOS() << "  isHUDAttachment() " << isHUDAttachment() << LL_ENDL;
	LL_INFOS() << "  isMesh() " << isRiggedMesh() << LL_ENDL;
	LL_INFOS() << "  isParticleSource() " << isRiggedMesh() << LL_ENDL;
	LL_INFOS() << "  isTempAttachment() " << isTempAttachment() << LL_ENDL;
	LL_INFOS() << "  getVObjRadius() " << getVObjRadius() << LL_ENDL;
	LL_INFOS() << "  getNumVertices() " << getNumVertices() << LL_ENDL;
	LL_INFOS() << "  getNumIndices() " << getNumIndices() << LL_ENDL;
	LL_INFOS() << "  isAnySelected() " << isAnySelected() << LL_ENDL;
	LL_INFOS() << "  isFlexible() " << isFlexible() << LL_ENDL;
	LL_INFOS() << "  isSculpted() " << isSculpted() << LL_ENDL;
	LL_INFOS() << "  mTotalCRC " << mTotalCRC << LL_ENDL;
	LL_INFOS() << "  mListIndex " << mListIndex << LL_ENDL;
	LL_INFOS() << "  mGLName " << mGLName << LL_ENDL;
	LL_INFOS() << "  mbCanSelect " << mbCanSelect << LL_ENDL;
	LL_INFOS() << "  mFlags" << mFlags << LL_ENDL;
	LL_INFOS() << "  mIsNameAttachment " << mIsNameAttachment << LL_ENDL;
	LL_INFOS() << "  mControlAvatar " << std::hex << mControlAvatar.get() << std::dec << LL_ENDL;
	LL_INFOS() << "  mChildList.size() " << mChildList.size() << LL_ENDL;
	LL_INFOS() << "  mLastInterpUpdateSecs " << mLastInterpUpdateSecs << LL_ENDL;
	LL_INFOS() << "  mLastMessageUpdateSecs " << mLastMessageUpdateSecs << LL_ENDL;
	LL_INFOS() << "  mLatestRecvPacketID " << mLatestRecvPacketID << LL_ENDL;
	LL_INFOS() << "  mAppAngle " << mAppAngle << LL_ENDL;
	LL_INFOS() << "  mPixelArea " << mPixelArea << LL_ENDL;
	LL_INFOS() << "  mInventory " << mInventory << LL_ENDL;
	LL_INFOS() << "  mInventorySerialNum " << mInventorySerialNum << LL_ENDL;
	LL_INFOS() << "  mInvRequestState " << mInvRequestState << LL_ENDL;
	LL_INFOS() << "  mInvRequestXFerId " << mInvRequestXFerId << LL_ENDL;
	LL_INFOS() << "  mInventoryDirty " << mInventoryDirty << LL_ENDL;
	LL_INFOS() << "  mUserSelected " << mUserSelected << LL_ENDL;
	LL_INFOS() << "  mOnActiveList " << mOnActiveList << LL_ENDL;
	LL_INFOS() << "  mOnMap" << mOnMap << LL_ENDL;
	LL_INFOS() << "  mStatic " << mStatic << LL_ENDL;
	LL_INFOS() << "  mNumFaces " << mNumFaces << LL_ENDL;
	LL_INFOS() << "  mRotTime " << mRotTime << LL_ENDL;
	LL_INFOS() << "  mAttachmentState " << mAttachmentState << LL_ENDL;
	LL_INFOS() << "  mClickAction " << mClickAction << LL_ENDL;
	LL_INFOS() << "  mCostStale " << mCostStale << LL_ENDL;
	LL_INFOS() << "  mPhysicsShapeUnknown " << mPhysicsShapeUnknown << LL_ENDL;
	LL_INFOS() << "  mPositionRegion " << mPositionRegion << LL_ENDL;
	LL_INFOS() << "  mPositionAgent " << mPositionAgent << LL_ENDL;
	LL_INFOS() << "  mAttachmentItemID " << mAttachmentItemID << LL_ENDL;
	LL_INFOS() << "  mLastUpdateType " << mLastUpdateType << LL_ENDL;
	LL_INFOS() << "  mLastUpdateCached " << mLastUpdateCached << LL_ENDL;
	LL_INFOS() << "==============" << LL_ENDL;
}
void LLViewerObject::linkControlAvatar()
{
	if (!getControlAvatar() && isRootEdit())
	{
        LLVOVolume *volp = asVolume();
		if (!volp)
		{
			LL_WARNS() << "called with null or non-volume object" << LL_ENDL;
			return;
		}
		mControlAvatar = LLControlAvatar::createControlAvatar(volp);
		LL_DEBUGS("AnimatedObjects") << volp->getID()
									 << " created control av for "
									 << (S32) (1+volp->numChildren()) << " prims" << LL_ENDL;
	}
	LLControlAvatar *cav = getControlAvatar();
	if (cav)
	{
		cav->updateAttachmentOverrides();
		if (!cav->mPlaying)
		{
			cav->mPlaying = true;
			{
				cav->updateVolumeGeom();
				cav->mRootVolp->recursiveMarkForUpdate(TRUE);
			}
		}
	}
	else
	{
		LL_WARNS() << "no control avatar found!" << LL_ENDL;
	}
}
void LLViewerObject::unlinkControlAvatar()
{
	if (getControlAvatar())
	{
		getControlAvatar()->updateAttachmentOverrides();
	}
	if (isRootEdit())
	{
		if (mControlAvatar)
		{
			mControlAvatar->markForDeath();
			mControlAvatar->mRootVolp = NULL;
			mControlAvatar = NULL;
		}
	}
}
bool LLViewerObject::isAnimatedObject() const
{
	return false;
}
struct LLFilenameAndTask
{
	LLUUID mTaskID;
	std::string mFilename;
	S16 mSerial;
#ifdef _DEBUG
	static S32 sCount;
	LLFilenameAndTask()
	{
		++sCount;
		LL_DEBUGS() << "Constructing LLFilenameAndTask: " << sCount << LL_ENDL;
	}
	~LLFilenameAndTask()
	{
		--sCount;
		LL_DEBUGS() << "Destroying LLFilenameAndTask: " << sCount << LL_ENDL;
	}
private:
	LLFilenameAndTask(const LLFilenameAndTask& rhs);
	const LLFilenameAndTask& operator=(const LLFilenameAndTask& rhs) const;
#endif
};
#ifdef _DEBUG
S32 LLFilenameAndTask::sCount = 0;
#endif
void LLViewerObject::processTaskInv(LLMessageSystem* msg, void** user_data)
{
	LLUUID task_id;
	msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_TaskID, task_id);
	LLViewerObject* object = gObjectList.findObject(task_id);
	if(!object)
	{
		LL_WARNS() << "LLViewerObject::processTaskInv object "
			<< task_id << " does not exist." << LL_ENDL;
		return;
	}
	LLFilenameAndTask* ft = new LLFilenameAndTask;
	ft->mTaskID = task_id;
	msg->getS16Fast(_PREHASH_InventoryData, _PREHASH_Serial, ft->mSerial);
	if (ft->mSerial < object->mInventorySerialNum)
	{
		LL_DEBUGS() << "Task inventory serial might be out of sync, server serial: " << ft->mSerial << " client serial: " << object->mInventorySerialNum << LL_ENDL;
		object->mInventorySerialNum = ft->mSerial;
	}
	std::string unclean_filename;
	msg->getStringFast(_PREHASH_InventoryData, _PREHASH_Filename, unclean_filename);
	ft->mFilename = LLDir::getScrubbedFileName(unclean_filename);
	if(ft->mFilename.empty())
	{
		LL_DEBUGS() << "Task has no inventory" << LL_ENDL;
		if(object->mInventory)
		{
			object->mInventory->clear();
		}
		else
		{
			object->mInventory = new LLInventoryObject::object_list_t();
		}
		LLPointer<LLInventoryObject> obj;
		obj = new LLInventoryObject(object->mID, LLUUID::null,
									LLAssetType::AT_CATEGORY,
									"Contents");
		object->mInventory->push_front(obj);
		object->doInventoryCallback();
		delete ft;
		return;
	}
	U64 new_id = gXferManager->requestFile(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ft->mFilename),
								ft->mFilename, LL_PATH_CACHE,
								object->mRegionp->getHost(),
								TRUE,
								&LLViewerObject::processTaskInvFile,
								(void**)ft,
								LLXferManager::HIGH_PRIORITY);
	if (object->mInvRequestState == INVENTORY_XFER)
	{
		if (new_id > 0 && new_id != object->mInvRequestXFerId)
		{
			gXferManager->abortRequestById(object->mInvRequestXFerId, -1);
			object->mInvRequestXFerId = new_id;
		}
	}
	else
	{
		object->mInvRequestState = INVENTORY_XFER;
		object->mInvRequestXFerId = new_id;
	}
}
void LLViewerObject::processTaskInvFile(void** user_data, S32 error_code, LLExtStat ext_status)
{
	LLFilenameAndTask* ft = (LLFilenameAndTask*)user_data;
	LLViewerObject* object = NULL;
	if (ft
		&& (0 == error_code)
		&& (object = gObjectList.findObject(ft->mTaskID))
		&& ft->mSerial >= object->mInventorySerialNum)
	{
		object->mInventorySerialNum = ft->mSerial;
		if (object->loadTaskInvFile(ft->mFilename))
		{
			LLInventoryObject::object_list_t::iterator it = object->mInventory->begin();
			LLInventoryObject::object_list_t::iterator end = object->mInventory->end();
			std::list<LLUUID>& pending_lst = object->mPendingInventoryItemsIDs;
			for (; it != end && !pending_lst.empty(); ++it)
			{
				LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(it->get());
				if(item && item->getType() != LLAssetType::AT_CATEGORY)
				{
					std::list<LLUUID>::iterator id_it = std::find(pending_lst.begin(), pending_lst.end(), item->getAssetUUID());
					if (id_it != pending_lst.end())
					{
						pending_lst.erase(id_it);
					}
				}
			}
		}
		else
		{
			LL_WARNS() << "Trying to load invalid task inventory file. Ignoring file contents." << LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS() << "Problem loading task inventory. Return code: "
				 << error_code << LL_ENDL;
	}
	delete ft;
}
BOOL LLViewerObject::loadTaskInvFile(const std::string& filename)
{
	std::string filename_and_local_path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, filename);
	llifstream ifs(filename_and_local_path.c_str());
	if(ifs.good())
	{
		U32 fail_count = 0;
		char buffer[MAX_STRING];
		char keyword[MAX_STRING];
		if(mInventory)
		{
			mInventory->clear();
		}
		else
		{
			mInventory = new LLInventoryObject::object_list_t;
		}
		while(ifs.good())
		{
			ifs.getline(buffer, MAX_STRING);
			if (sscanf(buffer, " %254s", keyword) == EOF)
			{
				LL_WARNS() << "Issue reading from file '"
						<< filename << "'" << LL_ENDL;
				break;
			}
			else if(0 == strcmp("inv_item", keyword))
			{
				LLPointer<LLInventoryObject> inv = new LLViewerInventoryItem;
				inv->importLegacyStream(ifs);
				mInventory->push_front(inv);
			}
			else if(0 == strcmp("inv_object", keyword))
			{
				LLPointer<LLInventoryObject> inv = new LLInventoryObject;
				inv->importLegacyStream(ifs);
				inv->rename("Contents");
				mInventory->push_front(inv);
			}
			else if (fail_count >= MAX_INV_FILE_READ_FAILS)
			{
				LL_WARNS() << "Encountered too many unknowns while reading from file: '"
						<< filename << "'" << LL_ENDL;
				break;
			}
			else
			{
				fail_count++;
				LL_WARNS_ONCE() << "Unknown token while reading from inventory file. Token: '"
						<< keyword << "'" << LL_ENDL;
			}
		}
		ifs.close();
		LLFile::remove(filename_and_local_path);
	}
	else
	{
		LL_WARNS() << "unable to load task inventory: " << filename_and_local_path
				<< LL_ENDL;
		return FALSE;
	}
	doInventoryCallback();
	return TRUE;
}
void LLViewerObject::doInventoryCallback()
{
	for (callback_list_t::iterator iter = mInventoryCallbacks.begin();
		 iter != mInventoryCallbacks.end(); )
	{
		callback_list_t::iterator curiter = iter++;
		LLInventoryCallbackInfo* info = *curiter;
		if (info->mListener != NULL)
		{
			info->mListener->inventoryChanged(this,
								 mInventory,
								 mInventorySerialNum,
								 info->mInventoryData);
		}
		else
		{
			LL_INFOS() << "LLViewerObject::doInventoryCallback() deleting bad listener entry." << LL_ENDL;
			delete info;
			mInventoryCallbacks.erase(curiter);
		}
	}
	mInvRequestXFerId = 0;
	mInvRequestState = INVENTORY_REQUEST_STOPPED;
}
void LLViewerObject::removeInventory(const LLUUID& item_id)
{
	LLFloaterProperties::closeByID(item_id, mID);
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addU32Fast(_PREHASH_LocalID, mLocalID);
	msg->addUUIDFast(_PREHASH_ItemID, item_id);
	msg->sendReliable(mRegionp->getHost());
	deleteInventoryItem(item_id);
	++mInventorySerialNum;
}
bool LLViewerObject::isTextureInInventory(LLViewerInventoryItem* item)
{
	bool result = false;
	if (item && LLAssetType::AT_TEXTURE == item->getType())
	{
		std::list<LLUUID>::iterator begin = mPendingInventoryItemsIDs.begin();
		std::list<LLUUID>::iterator end = mPendingInventoryItemsIDs.end();
		bool is_fetching = std::find(begin, end, item->getAssetUUID()) != end;
		bool is_fetched = getInventoryItemByAsset(item->getAssetUUID()) != NULL;
		result = is_fetched || is_fetching;
	}
	return result;
}
void LLViewerObject::updateTextureInventory(LLViewerInventoryItem* item, U8 key, bool is_new)
{
	if (item && !isTextureInInventory(item))
	{
		mPendingInventoryItemsIDs.push_back(item->getAssetUUID());
		updateInventory(item, key, is_new);
	}
}
void LLViewerObject::updateInventory(
	LLViewerInventoryItem* item,
	U8 key,
	bool is_new)
{
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(),
								  item->getFlags(),
								  item->getCreationDate());
	task_item->setTransactionID(item->getTransactionID());
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_UpdateData);
	msg->addU32Fast(_PREHASH_LocalID, mLocalID);
	msg->addU8Fast(_PREHASH_Key, key);
	msg->nextBlockFast(_PREHASH_InventoryData);
	task_item->packMessage(msg);
	msg->sendReliable(mRegionp->getHost());
	doUpdateInventory(task_item, key, is_new);
}
void LLViewerObject::updateInventoryLocal(LLInventoryItem* item, U8 key)
{
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(), item->getFlags(),
								  item->getCreationDate());
	const bool is_new = false;
	doUpdateInventory(task_item, key, is_new);
}
LLInventoryObject* LLViewerObject::getInventoryObject(const LLUUID& item_id)
{
	LLInventoryObject* rv = NULL;
	if(mInventory)
	{
		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for ( ; it != end; ++it)
		{
			if((*it)->getUUID() == item_id)
			{
				rv = *it;
				break;
			}
		}
	}
	return rv;
}
void LLViewerObject::getInventoryContents(LLInventoryObject::object_list_t& objects)
{
	if(mInventory)
	{
		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for( ; it != end; ++it)
		{
			if ((*it)->getType() != LLAssetType::AT_CATEGORY)
			{
				objects.push_back(*it);
			}
		}
	}
}
LLInventoryObject* LLViewerObject::getInventoryRoot()
{
	if (!mInventory || mInventory->empty())
	{
		return NULL;
	}
	return mInventory->back();
}
LLViewerInventoryItem* LLViewerObject::getInventoryItemByAsset(const LLUUID& asset_id)
{
	if (mInventoryDirty)
		LL_WARNS() << "Peforming inventory lookup for object " << mID << " that has dirty inventory!" << LL_ENDL;
	LLViewerInventoryItem* rv = NULL;
	if(mInventory)
	{
		LLViewerInventoryItem* item = NULL;
		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for( ; it != end; ++it)
		{
			LLInventoryObject* obj = *it;
			if (obj && obj->getType() != LLAssetType::AT_CATEGORY && obj->getType() != LLAssetType::AT_NONE)
			{
				item = (LLViewerInventoryItem*)obj;
				if (item && item->getAssetUUID() == asset_id)
				{
					rv = item;
					break;
				}
			}
		}
	}
	return rv;
}
void LLViewerObject::updateViewerInventoryAsset(
					const LLViewerInventoryItem* item,
					const LLUUID& new_asset)
{
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item);
	task_item->setAssetUUID(new_asset);
	doUpdateInventory(task_item, TASK_INVENTORY_ITEM_KEY, false);
}
void LLViewerObject::setPixelAreaAndAngle(LLAgent &agent)
{
	if (getVolume())
	{
		return;
	}
	const LLVector3& viewer_pos_agent = gAgentCamera.getCameraPositionAgent();
	const LLVector3& pos_agent = getRenderPosition();
	F32 dx = viewer_pos_agent.mV[VX] - pos_agent.mV[VX];
	F32 dy = viewer_pos_agent.mV[VY] - pos_agent.mV[VY];
	F32 dz = viewer_pos_agent.mV[VZ] - pos_agent.mV[VZ];
	F32 max_scale = getMaxScale();
	F32 mid_scale = getMidScale();
	F32 min_scale = getMinScale();
	F32 range = sqrt(dx*dx + dy*dy + dz*dz) - min_scale/2.f;
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	if (range < 0.001f || isHUDAttachment())
	{
		mAppAngle = 180.f;
		mPixelArea = (F32)camera->getScreenPixelArea();
	}
	else
	{
		mAppAngle = (F32) atan2( max_scale, range) * RAD_TO_DEG;
		F32 pixels_per_meter = camera->getPixelMeterRatio() / range;
		mPixelArea = (pixels_per_meter * max_scale) * (pixels_per_meter * mid_scale);
		if (mPixelArea > camera->getScreenPixelArea())
		{
			mAppAngle = 180.f;
			mPixelArea = (F32)camera->getScreenPixelArea();
		}
	}
}
BOOL LLViewerObject::updateLOD()
{
	return FALSE;
}
BOOL LLViewerObject::updateGeometry(LLDrawable *drawable)
{
	return TRUE;
}
void LLViewerObject::updateGL()
{
}
void LLViewerObject::updateFaceSize(S32 idx)
{
}
LLDrawable* LLViewerObject::createDrawable(LLPipeline *pipeline)
{
	return NULL;
}
void LLViewerObject::setScale(const LLVector3 &scale, BOOL damped)
{
	LLPrimitive::setScale(scale);
	if (mDrawable.notNull())
	{
		mDrawable->setRadius(LLVector3(1,1,0.5f).scaleVec(scale).magVec());
		updateDrawable(damped);
	}
	if( (LL_PCODE_VOLUME == getPCode()) && !isDead() )
	{
		if (permYouOwner() || (scale.magVecSquared() > (7.5f * 7.5f)) )
		{
			if (!mOnMap)
			{
				llassert_always(LLWorld::getInstance()->getRegionFromHandle(getRegion()->getHandle()));
				gObjectList.addToMap(this);
				mOnMap = TRUE;
			}
		}
		else
		{
			if (mOnMap)
			{
				gObjectList.removeFromMap(this);
				mOnMap = FALSE;
			}
		}
	}
}
void LLViewerObject::setObjectCost(F32 cost)
{
	mObjectCost = cost;
	mCostStale = false;
	if (isSelected())
	{
		gFloaterTools->dirty();
	}
}
void LLViewerObject::setLinksetCost(F32 cost)
{
	mLinksetCost = cost;
	mCostStale = false;
	BOOL needs_refresh = isSelected();
	child_list_t::iterator iter = mChildList.begin();
	while(iter != mChildList.end() && !needs_refresh)
	{
		LLViewerObject* child = *iter;
		needs_refresh = child->isSelected();
		iter++;
	}
	if (needs_refresh)
	{
		gFloaterTools->dirty();
	}
}
void LLViewerObject::setPhysicsCost(F32 cost)
{
	mPhysicsCost = cost;
	mCostStale = false;
	if (isSelected())
	{
		gFloaterTools->dirty();
	}
}
void LLViewerObject::setLinksetPhysicsCost(F32 cost)
{
	mLinksetPhysicsCost = cost;
	mCostStale = false;
	if (isSelected())
	{
		gFloaterTools->dirty();
	}
}
F32 LLViewerObject::getObjectCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}
	return mObjectCost;
}
F32 LLViewerObject::getLinksetCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}
	return mLinksetCost;
}
F32 LLViewerObject::getPhysicsCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}
	return mPhysicsCost;
}
F32 LLViewerObject::getLinksetPhysicsCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}
	return mLinksetPhysicsCost;
}
F32 LLViewerObject::recursiveGetEstTrianglesMax() const
{
	F32 est_tris = getEstTrianglesMax();
    for (const auto& iter : mChildList)
	{
        const LLViewerObject* child = iter;
		if (!child->isAvatar())
		{
			est_tris += child->recursiveGetEstTrianglesMax();
		}
	}
	return est_tris;
}
S32 LLViewerObject::getAnimatedObjectMaxTris() const
{
	S32 max_tris = 0;
	if (gSavedSettings.getBOOL("AnimatedObjectsIgnoreLimits"))
	{
		max_tris = S32_MAX;
	}
	else
	{
		if (gAgent.getRegion())
		{
			LLSD features;
			gAgent.getRegion()->getSimulatorFeatures(features);
			if (features.has("AnimatedObjects"))
			{
				max_tris = features["AnimatedObjects"]["AnimatedObjectMaxTris"].asInteger();
			}
		}
	}
	return max_tris;
}
F32 LLViewerObject::getEstTrianglesMax() const
{
	return 0.f;
}
F32 LLViewerObject::getEstTrianglesStreamingCost() const
{
	return 0.f;
}
F32 LLViewerObject::getStreamingCost() const
{
	return 0.f;
}
bool LLViewerObject::getCostData(LLMeshCostData& costs) const
{
	costs = LLMeshCostData();
	return false;
}
U32 LLViewerObject::getTriangleCount(S32* vcount) const
{
	return 0;
}
U32 LLViewerObject::getHighLODTriangleCount()
{
	return 0;
}
U32 LLViewerObject::recursiveGetTriangleCount(S32* vcount) const
{
	S32 total_tris = getTriangleCount(vcount);
	LLViewerObject::const_child_list_t& child_list = getChildren();
    for (const auto& iter : child_list)
	{
        LLViewerObject* childp = iter;
		if (childp)
		{
			total_tris += childp->getTriangleCount(vcount);
		}
	}
	return total_tris;
}
F32 LLViewerObject::recursiveGetScaledSurfaceArea() const
{
	F32 area = 0.f;
	const LLDrawable* drawable = mDrawable;
	if (drawable)
	{
		const LLVOVolume* volume = drawable->getVOVolume();
		if (volume)
		{
			if (volume->getVolume())
			{
				const LLVector3& scale = volume->getScale();
				area += volume->getVolume()->getSurfaceArea() * llmax(llmax(scale.mV[0], scale.mV[1]), scale.mV[2]);
			}
            LLViewerObject::const_child_list_t const& children = volume->getChildren();
            for (const auto& child_iter : children)
			{
                LLViewerObject* child_obj = child_iter;
                LLVOVolume *child = child_obj ? child_obj->asVolume() : nullptr;
				if (child && child->getVolume())
				{
					const LLVector3& scale = child->getScale();
					area += child->getVolume()->getSurfaceArea() * llmax(llmax(scale.mV[0], scale.mV[1]), scale.mV[2]);
				}
			}
		}
	}
	return area;
}
void LLViewerObject::updateSpatialExtents(LLVector4a& newMin, LLVector4a &newMax)
{
	if(mDrawable.isNull())
		return;
	LLVector4a center;
	center.load3(getRenderPosition().mV);
	LLVector4a size;
	size.load3(getScale().mV);
	newMin.setSub(center, size);
	newMax.setAdd(center, size);
	mDrawable->setPositionGroup(center);
}
F32 LLViewerObject::getBinRadius()
{
	if (mDrawable.notNull())
	{
		const LLVector4a* ext = mDrawable->getSpatialExtents();
		LLVector4a diff;
		diff.setSub(ext[1], ext[0]);
		return diff.getLength3().getF32();
	}
	return getScale().magVec();
}
F32 LLViewerObject::getMaxScale() const
{
	return llmax(getScale().mV[VX],getScale().mV[VY], getScale().mV[VZ]);
}
F32 LLViewerObject::getMinScale() const
{
	return llmin(getScale().mV[0],getScale().mV[1],getScale().mV[2]);
}
F32 LLViewerObject::getMidScale() const
{
	if (getScale().mV[VX] < getScale().mV[VY])
	{
		if (getScale().mV[VY] < getScale().mV[VZ])
		{
			return getScale().mV[VY];
		}
		else if (getScale().mV[VX] < getScale().mV[VZ])
		{
			return getScale().mV[VZ];
		}
		else
		{
			return getScale().mV[VX];
		}
	}
	else if (getScale().mV[VX] < getScale().mV[VZ])
	{
		return getScale().mV[VX];
	}
	else if (getScale().mV[VY] < getScale().mV[VZ])
	{
		return getScale().mV[VZ];
	}
	else
	{
		return getScale().mV[VY];
	}
}
void LLViewerObject::updateTextures()
{
}
void LLViewerObject::boostTexturePriority(BOOL boost_children )
{
	if (isDead())
	{
		return;
	}
	S32 i;
	S32 tex_count = getNumTEs();
	for (i = 0; i < tex_count; i++)
	{
		getTEImage(i)->setBoostLevel(LLGLTexture::BOOST_SELECTED);
	}
	if (isSculpted() && !isMesh())
	{
		const LLSculptParams *sculpt_params = getSculptParams();
		LLUUID sculpt_id = sculpt_params->getSculptTexture();
		LLViewerTextureManager::getFetchedTexture(sculpt_id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE)->setBoostLevel(LLGLTexture::BOOST_SELECTED);
	}
	if (boost_children)
	{
		for (child_list_t::iterator iter = mChildList.begin();
			 iter != mChildList.end(); iter++)
		{
			LLViewerObject* child = *iter;
			child->boostTexturePriority();
		}
	}
}
void LLViewerObject::setLineWidthForWindowSize(S32 window_width)
{
	if (window_width < 700)
	{
		LLUI::setLineWidth(2.0f);
	}
	else if (window_width < 1100)
	{
		LLUI::setLineWidth(3.0f);
	}
	else if (window_width < 2000)
	{
		LLUI::setLineWidth(4.0f);
	}
	else
	{
		LLUI::setLineWidth(5.0f);
	}
}
void LLViewerObject::increaseArrowLength()
{
}
void LLViewerObject::decreaseArrowLength()
{
}
void LLViewerObject::addNVPair(const std::string& data)
{
	LLNameValue *nv = new LLNameValue(data.c_str());
	name_value_map_t::iterator iter = mNameValuePairs.find(nv->mName);
	if (iter != mNameValuePairs.end())
	{
		LLNameValue* foundnv = iter->second;
		if (foundnv->mClass != NVC_READ_ONLY)
		{
			delete foundnv;
			mNameValuePairs.erase(iter);
		}
		else
		{
			delete nv;
			return;
		}
	}
	mNameValuePairs[nv->mName] = nv;
}
BOOL LLViewerObject::removeNVPair(const std::string& name)
{
	char* canonical_name = gNVNameTable.addString(name);
	LL_DEBUGS() << "LLViewerObject::removeNVPair(): " << name << LL_ENDL;
	name_value_map_t::iterator iter = mNameValuePairs.find(canonical_name);
	if (iter != mNameValuePairs.end())
	{
		if( mRegionp )
		{
			LLNameValue* nv = iter->second;
			delete nv;
			mNameValuePairs.erase(iter);
			return TRUE;
		}
		else
		{
			LL_DEBUGS() << "removeNVPair - No region for object" << LL_ENDL;
		}
	}
	return FALSE;
}
LLNameValue *LLViewerObject::getNVPair(const std::string& name) const
{
	char		*canonical_name;
	canonical_name = gNVNameTable.addString(name);
	name_value_map_t::const_iterator iter = mNameValuePairs.find(canonical_name);
	if (iter != mNameValuePairs.end())
	{
		return iter->second;
	}
	else
	{
		return NULL;
	}
}
void LLViewerObject::updatePositionCaches() const
{
	if(mRegionp && LLWorld::instance().isRegionListed(mRegionp))
	{
		if (!isRoot())
		{
			mPositionRegion = ((LLViewerObject *)getParent())->getPositionRegion() + getPosition() * getParent()->getRotation();
			mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
		}
		else
		{
			mPositionRegion = getPosition();
			mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
		}
	}
}
const LLVector3d LLViewerObject::getPositionGlobal() const
{
	if(mRegionp && LLWorld::instance().isRegionListed(mRegionp))
	{
		LLVector3d position_global = mRegionp->getPosGlobalFromRegion(getPositionRegion());
		if (isAttachment())
		{
			position_global = gAgent.getPosGlobalFromAgent(getRenderPosition());
		}
		return position_global;
	}
	else
	{
		LLVector3d position_global(getPosition());
		return position_global;
	}
}
const LLVector3 &LLViewerObject::getPositionAgent() const
{
	if(mRegionp && LLWorld::instance().isRegionListed(mRegionp))
	{
		if (mDrawable.notNull() && (!mDrawable->isRoot() && getParent()))
		{
			LLVector3 position_region;
			position_region = ((LLViewerObject *)getParent())->getPositionRegion() + getPosition() * getParent()->getRotation();
			mPositionAgent = mRegionp->getPosAgentFromRegion(position_region);
		}
		else
		{
			mPositionAgent = mRegionp->getPosAgentFromRegion(getPosition());
		}
	}
	return mPositionAgent;
}
const LLVector3 &LLViewerObject::getPositionRegion() const
{
	if (!isRoot())
	{
		LLViewerObject *parent = (LLViewerObject *)getParent();
		mPositionRegion = parent->getPositionRegion() + (getPosition() * parent->getRotation());
	}
	else
	{
		mPositionRegion = getPosition();
	}
	return mPositionRegion;
}
const LLVector3 LLViewerObject::getPositionEdit() const
{
	if (isRootEdit())
	{
		return getPosition();
	}
	else
	{
		LLViewerObject *parent = (LLViewerObject *)getParent();
		LLVector3 position_edit = parent->getPositionEdit() + getPosition() * parent->getRotationEdit();
		return position_edit;
	}
}
const LLVector3 LLViewerObject::getRenderPosition() const
{
	if (mDrawable.notNull() && mDrawable->isState(LLDrawable::RIGGED))
	{
		LLControlAvatar *cav = getControlAvatar();
		if (isRoot() && cav)
		{
			F32 fixup;
			if ( cav->hasPelvisFixup( fixup) )
			{
				LLVector3 pos = mDrawable->getPositionAgent();
				pos[VZ] += fixup;
				return pos;
			}
		}
		LLVOAvatar* avatar = getAvatar();
		if ((avatar) && !getControlAvatar())
		{
			return avatar->getPositionAgent();
		}
	}
	if (mDrawable.isNull() || mDrawable->getGeneration() < 0)
	{
		return getPositionAgent();
	}
	else
	{
		return mDrawable->getPositionAgent();
	}
}
const LLVector3 LLViewerObject::getPivotPositionAgent() const
{
	return getRenderPosition();
}
const LLQuaternion LLViewerObject::getRenderRotation() const
{
	LLQuaternion ret;
	if (mDrawable.notNull() && mDrawable->isState(LLDrawable::RIGGED) && !isAnimatedObject())
	{
		return ret;
	}
	if (mDrawable.isNull() || mDrawable->isStatic())
	{
		ret = getRotationEdit();
	}
	else
	{
		if (!mDrawable->isRoot())
		{
			ret = getRotation() * LLQuaternion(LLMatrix4(mDrawable->getParent()->getWorldMatrix().getF32ptr()));
		}
		else
		{
			ret = LLQuaternion(LLMatrix4(mDrawable->getWorldMatrix().getF32ptr()));
		}
	}
	return ret;
}
const LLMatrix4a& LLViewerObject::getRenderMatrix() const
{
	return mDrawable->getWorldMatrix();
}
const LLQuaternion LLViewerObject::getRotationRegion() const
{
	LLQuaternion global_rotation = getRotation();
	if (!((LLXform *)this)->isRoot())
	{
		global_rotation = global_rotation * getParent()->getRotation();
	}
	return global_rotation;
}
const LLQuaternion LLViewerObject::getRotationEdit() const
{
	LLQuaternion global_rotation = getRotation();
	if (!((LLXform *)this)->isRootEdit())
	{
		global_rotation = global_rotation * getParent()->getRotation();
	}
	return global_rotation;
}
void LLViewerObject::setPositionAbsoluteGlobal( const LLVector3d &pos_global, BOOL damped )
{
	if (isAttachment())
	{
		LLVector3 new_pos = mRegionp->getPosRegionFromGlobal(pos_global);
		if (isRootEdit())
		{
			new_pos -= mDrawable->mXform.getParent()->getWorldPosition();
			LLQuaternion world_rotation = mDrawable->mXform.getParent()->getWorldRotation();
			new_pos = new_pos * ~world_rotation;
		}
		else
		{
			LLViewerObject* parentp = (LLViewerObject*)getParent();
			new_pos -= parentp->getPositionAgent();
			new_pos = new_pos * ~parentp->getRotationRegion();
		}
		LLViewerObject::setPosition(new_pos);
		if (mParent && ((LLViewerObject*)mParent)->isAvatar())
		{
			LLVOAvatar *avatar = (LLVOAvatar*)mParent;
			avatar->clampAttachmentPositions();
		}
	}
	else
	{
		if( isRoot() )
		{
			setPositionRegion(mRegionp->getPosRegionFromGlobal(pos_global));
		}
		else
		{
			LLViewerObject* parent = (LLViewerObject *)getParent();
			gPipeline.updateMoveNormalAsync(parent->mDrawable);
			LLVector3 pos_local = mRegionp->getPosRegionFromGlobal(pos_global) - parent->getPositionRegion();
			pos_local = pos_local * ~parent->getRotationRegion();
			LLViewerObject::setPosition( pos_local );
		}
	}
	gPipeline.updateMoveNormalAsync(mDrawable);
}
void LLViewerObject::setPosition(const LLVector3 &pos, BOOL damped)
{
	if (getPosition() != pos)
	{
		setChanged(TRANSLATED | SILHOUETTE);
	}
	LLXform::setPosition(pos);
	updateDrawable(damped);
	if (isRoot())
	{
		updatePositionCaches();
	}
}
void LLViewerObject::setPositionGlobal(const LLVector3d &pos_global, BOOL damped)
{
	if (isAttachment())
	{
		if (isRootEdit())
		{
			LLVector3 newPos = mRegionp->getPosRegionFromGlobal(pos_global);
			newPos = newPos - mDrawable->mXform.getParent()->getWorldPosition();
			LLQuaternion invWorldRotation = mDrawable->mXform.getParent()->getWorldRotation();
			invWorldRotation.transQuat();
			newPos = newPos * invWorldRotation;
			LLViewerObject::setPosition(newPos);
		}
		else
		{
			LLVector3 newPos = mRegionp->getPosRegionFromGlobal(pos_global);
			newPos = newPos - mDrawable->mXform.getParent()->getWorldPosition();
			LLVector3 delta_pos = newPos - getPosition();
			LLQuaternion invRotation = mDrawable->getRotation();
			invRotation.transQuat();
			delta_pos = delta_pos * invRotation;
			LLVector3 old_pos = mDrawable->mXform.getParent()->getPosition();
			mDrawable->mXform.getParent()->setPosition(old_pos + delta_pos);
			setChanged(TRANSLATED | SILHOUETTE);
		}
		if (mParent && ((LLViewerObject*)mParent)->isAvatar())
		{
			LLVOAvatar *avatar = (LLVOAvatar*)mParent;
			avatar->clampAttachmentPositions();
		}
	}
	else
	{
		if (isRoot())
		{
			setPositionRegion(mRegionp->getPosRegionFromGlobal(pos_global));
		}
		else
		{
			LLVector3d position_offset;
			position_offset.setVec(getPosition()*getParent()->getRotation());
			LLVector3d new_pos_global = pos_global - position_offset;
			((LLViewerObject *)getParent())->setPositionGlobal(new_pos_global);
		}
	}
	updateDrawable(damped);
}
void LLViewerObject::setPositionParent(const LLVector3 &pos_parent, BOOL damped)
{
	if (!isRoot())
	{
		LLViewerObject::setPosition(pos_parent, damped);
	}
	else
	{
		setPositionRegion(pos_parent, damped);
	}
}
void LLViewerObject::setPositionRegion(const LLVector3 &pos_region, BOOL damped)
{
	if (!isRootEdit())
	{
		LLViewerObject* parent = (LLViewerObject*) getParent();
		LLViewerObject::setPosition((pos_region-parent->getPositionRegion())*~parent->getRotationRegion());
	}
	else
	{
		LLViewerObject::setPosition(pos_region);
		mPositionRegion = pos_region;
		mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
	}
}
void LLViewerObject::setPositionAgent(const LLVector3 &pos_agent, BOOL damped)
{
	LLVector3 pos_region = getRegion()->getPosRegionFromAgent(pos_agent);
	setPositionRegion(pos_region, damped);
}
void LLViewerObject::setPositionEdit(const LLVector3 &pos_edit, BOOL damped)
{
	if (!isRootEdit())
	{
		LLVector3 position_offset = getPosition() * getParent()->getRotation();
		((LLViewerObject *)getParent())->setPositionEdit(pos_edit - position_offset);
		updateDrawable(damped);
	}
	else
	{
		LLViewerObject::setPosition(pos_edit, damped);
		mPositionRegion = pos_edit;
		mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
	}
}
LLViewerObject* LLViewerObject::getRootEdit() const
{
	const LLViewerObject* root = this;
	while (root->mParent
		   && !((LLViewerObject*)root->mParent)->isAvatar())
	{
		root = (LLViewerObject*)root->mParent;
	}
	return (LLViewerObject*)root;
}
BOOL LLViewerObject::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
										  S32 face,
										  BOOL pick_transparent,
										  BOOL pick_rigged,
										  S32* face_hit,
										  LLVector4a* intersection,
										  LLVector2* tex_coord,
										  LLVector4a* normal,
										  LLVector4a* tangent)
{
	return false;
}
BOOL LLViewerObject::lineSegmentBoundingBox(const LLVector4a& start, const LLVector4a& end)
{
	if (mDrawable.isNull() || mDrawable->isDead())
	{
		return FALSE;
	}
	const LLVector4a* ext = mDrawable->getSpatialExtents();
	LLVector4a center;
	center.setAdd(ext[1], ext[0]);
	center.mul(0.5f);
	LLVector4a size;
	size.setSub(ext[1], ext[0]);
	size.mul(0.5f);
	return LLLineSegmentBoxIntersect(start, end, center, size);
}
U8 LLViewerObject::getMediaType() const
{
	if (mMedia)
	{
		return mMedia->mMediaType;
	}
	else
	{
		return LLViewerObject::MEDIA_NONE;
	}
}
void LLViewerObject::setMediaType(U8 media_type)
{
	if (!mMedia)
	{
	}
	else if (mMedia->mMediaType != media_type)
	{
		mMedia->mMediaType = media_type;
	}
}
std::string LLViewerObject::getMediaURL() const
{
	if (mMedia)
	{
		return mMedia->mMediaURL;
	}
	else
	{
		return std::string();
	}
}
void LLViewerObject::setMediaURL(const std::string& media_url)
{
	if (!mMedia)
	{
		mMedia = new LLViewerObjectMedia;
		mMedia->mMediaURL = media_url;
		mMedia->mPassedWhitelist = FALSE;
	}
	else if (mMedia->mMediaURL != media_url)
	{
		mMedia->mMediaURL = media_url;
		mMedia->mPassedWhitelist = FALSE;
	}
}
BOOL LLViewerObject::getMediaPassedWhitelist() const
{
	if (mMedia)
	{
		return mMedia->mPassedWhitelist;
	}
	else
	{
		return FALSE;
	}
}
void LLViewerObject::setMediaPassedWhitelist(BOOL passed)
{
	if (mMedia)
	{
		mMedia->mPassedWhitelist = passed;
	}
}
BOOL LLViewerObject::setMaterial(const U8 material)
{
	BOOL res = LLPrimitive::setMaterial(material);
	if (res)
	{
		setChanged(TEXTURE);
	}
	return res;
}
void LLViewerObject::setNumTEs(const U8 num_tes)
{
	U32 i;
	if (num_tes != getNumTEs())
	{
		if (num_tes)
		{
			LLPointer<LLViewerTexture> *new_images;
			new_images = new LLPointer<LLViewerTexture>[num_tes];
			LLPointer<LLViewerTexture> *new_normmaps;
			new_normmaps = new LLPointer<LLViewerTexture>[num_tes];
			LLPointer<LLViewerTexture> *new_specmaps;
			new_specmaps = new LLPointer<LLViewerTexture>[num_tes];
			for (i = 0; i < num_tes; i++)
			{
				if (i < getNumTEs())
				{
					new_images[i] = mTEImages[i];
					new_normmaps[i] = mTENormalMaps[i];
					new_specmaps[i] = mTESpecularMaps[i];
				}
				else if (getNumTEs())
				{
					new_images[i] = mTEImages[getNumTEs()-1];
					new_normmaps[i] = mTENormalMaps[getNumTEs()-1];
					new_specmaps[i] = mTESpecularMaps[getNumTEs()-1];
				}
				else
				{
					new_images[i] = NULL;
					new_normmaps[i] = NULL;
					new_specmaps[i] = NULL;
				}
			}
			deleteTEImages();
			mTEImages = new_images;
			mTENormalMaps = new_normmaps;
			mTESpecularMaps = new_specmaps;
		}
		else
		{
			deleteTEImages();
		}
		LLPrimitive::setNumTEs(num_tes);
		setChanged(TEXTURE);
		if (mDrawable.notNull())
		{
			gPipeline.markTextured(mDrawable);
		}
	}
}
void LLViewerObject::sendMaterialUpdate() const
{
	LLViewerRegion* regionp = getRegion();
	if(!regionp) return;
	gMessageSystem->newMessageFast(_PREHASH_ObjectMaterial);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID,	mLocalID );
	gMessageSystem->addU8Fast(_PREHASH_Material, getMaterial() );
	gMessageSystem->sendReliable( regionp->getHost() );
}
void LLViewerObject::sendRotationUpdate() const
{
	LLViewerRegion* regionp = getRegion();
	if(!regionp) return;
	gMessageSystem->newMessageFast(_PREHASH_ObjectRotation);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, mLocalID);
	gMessageSystem->addQuatFast(_PREHASH_Rotation, getRotationEdit());
	gMessageSystem->sendReliable( regionp->getHost() );
}
void LLViewerObject::sendShapeUpdate()
{
	gMessageSystem->newMessageFast(_PREHASH_ObjectShape);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, mLocalID );
	LLVolumeMessage::packVolumeParams(&getVolume()->getParams(), gMessageSystem);
	LLViewerRegion *regionp = getRegion();
	gMessageSystem->sendReliable( regionp->getHost() );
}
void LLViewerObject::sendTEUpdate() const
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectImage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, mLocalID );
	if (mMedia)
	{
		msg->addString("MediaURL", mMedia->mMediaURL);
	}
	else
	{
		msg->addString("MediaURL", NULL);
	}
	packTEMessage(msg);
	LLViewerRegion *regionp = getRegion();
	msg->sendReliable( regionp->getHost() );
}
LLViewerTexture* LLViewerObject::getBakedTextureForMagicId(const LLUUID& id)
{
	if (!LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(id))
	{
		return NULL;
	}
	LLViewerObject *root = getRootEdit();
	if (root && root->isAnimatedObject())
	{
		return LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	}
	LLVOAvatar* avatar = getAvatar();
	if (avatar && !isHUDAttachment())
	{
		LLAvatarAppearanceDefines::EBakedTextureIndex texIndex = LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::assetIdToBakedTextureIndex(id);
		LLViewerTexture* bakedTexture = avatar->getBakedTextureForAttachment(texIndex);
		if (bakedTexture == NULL || bakedTexture->isMissingAsset())
		{
			return LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
		}
		return bakedTexture;
	}
	else
	{
		return LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	}
}
void LLViewerObject::updateAvatarMeshVisibility(const LLUUID& id, const LLUUID& old_id)
{
	if (id == old_id)
	{
		return;
	}
	if (!LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(old_id) && !LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(id))
	{
		return;
	}
	LLVOAvatar* avatar = getAvatar();
	if (avatar)
	{
		avatar->updateMeshVisibility();
	}
}
void LLViewerObject::setTE(const U8 te, const LLTextureEntry &texture_entry)
{
	LLUUID old_image_id;
	if (getTE(te))
	{
		old_image_id = getTE(te)->getID();
	}
	LLPrimitive::setTE(te, texture_entry);
	const LLUUID& image_id = getTE(te)->getID();
	LLViewerTexture* bakedTexture = getBakedTextureForMagicId(image_id);
	mTEImages[te] = bakedTexture ? bakedTexture : LLViewerTextureManager::getFetchedTexture(image_id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	updateAvatarMeshVisibility(image_id,old_image_id);
	if (getTE(te)->getMaterialParams().notNull())
	{
		const LLUUID& norm_id = getTE(te)->getMaterialParams()->getNormalID();
		mTENormalMaps[te] = LLViewerTextureManager::getFetchedTexture(norm_id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM, LLViewerTexture::LOD_TEXTURE);
		const LLUUID& spec_id = getTE(te)->getMaterialParams()->getSpecularID();
		mTESpecularMaps[te] = LLViewerTextureManager::getFetchedTexture(spec_id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM, LLViewerTexture::LOD_TEXTURE);
	}
}
void LLViewerObject::refreshBakeTexture()
{
	for (int face_index = 0; face_index < getNumTEs(); face_index++)
	{
		LLTextureEntry* tex_entry = getTE(face_index);
		if (tex_entry && LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(tex_entry->getID()))
		{
			const LLUUID& image_id = tex_entry->getID();
			LLViewerTexture* bakedTexture = getBakedTextureForMagicId(image_id);
			changeTEImage(face_index, bakedTexture);
		}
	}
}
void LLViewerObject::setTEImage(const U8 te, LLViewerTexture *imagep)
{
	if (mTEImages[te] != imagep)
	{
		LLUUID old_image_id = getTE(te) ? getTE(te)->getID() : LLUUID::null;
		LLPrimitive::setTETexture(te, imagep->getID());
		LLViewerTexture* baked_texture = getBakedTextureForMagicId(imagep->getID());
		mTEImages[te] = baked_texture ? baked_texture : imagep;
		updateAvatarMeshVisibility(imagep->getID(), old_image_id);
		setChanged(TEXTURE);
		if (mDrawable.notNull())
		{
			gPipeline.markTextured(mDrawable);
		}
	}
}
S32 LLViewerObject::setTETextureCore(const U8 te, LLViewerTexture *image)
{
	if (!image)
		return 0;
	LLUUID old_image_id = getTE(te)->getID();
	const LLUUID& uuid = image->getID();
	S32 retval = 0;
	if (uuid != getTE(te)->getID() ||
		uuid == LLUUID::null)
	{
		retval = LLPrimitive::setTETexture(te, uuid);
		LLViewerTexture* baked_texture = getBakedTextureForMagicId(uuid);
		mTEImages[te] = baked_texture ? baked_texture : image;
		updateAvatarMeshVisibility(uuid,old_image_id);
		setChanged(TEXTURE);
		if (mDrawable.notNull())
		{
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}
S32 LLViewerObject::setTENormalMapCore(const U8 te, LLViewerTexture *image)
{
	S32 retval = TEM_CHANGE_TEXTURE;
	const LLUUID& uuid = image ? image->getID() : LLUUID::null;
	if (uuid != getTE(te)->getID() ||
		uuid == LLUUID::null)
	{
		LLTextureEntry* tep = getTE(te);
		LLMaterial* mat = NULL;
		if (tep)
		{
		   mat = tep->getMaterialParams();
		}
		if (mat)
		{
			mat->setNormalID(uuid);
		}
	}
	changeTENormalMap(te,image);
	return retval;
}
S32 LLViewerObject::setTESpecularMapCore(const U8 te, LLViewerTexture *image)
{
	S32 retval = TEM_CHANGE_TEXTURE;
	const LLUUID& uuid = image ? image->getID() : LLUUID::null;
	if (uuid != getTE(te)->getID() ||
		uuid == LLUUID::null)
	{
		LLTextureEntry* tep = getTE(te);
		LLMaterial* mat = NULL;
		if (tep)
		{
			mat = tep->getMaterialParams();
		}
		if (mat)
		{
			mat->setSpecularID(uuid);
		}
	}
	changeTESpecularMap(te, image);
	return retval;
}
void LLViewerObject::changeTEImage(S32 index, LLViewerTexture* new_image)
{
	if(index < 0 || index >= getNumTEs())
	{
		return ;
	}
	mTEImages[index] = new_image ;
}
void LLViewerObject::changeTENormalMap(S32 index, LLViewerTexture* new_image)
{
	if(index < 0 || index >= getNumTEs())
	{
		return ;
	}
	mTENormalMaps[index] = new_image ;
	refreshMaterials();
}
void LLViewerObject::changeTESpecularMap(S32 index, LLViewerTexture* new_image)
{
	if(index < 0 || index >= getNumTEs())
	{
		return ;
	}
	mTESpecularMaps[index] = new_image ;
	refreshMaterials();
}
S32 LLViewerObject::setTETexture(const U8 te, const LLUUID& uuid)
{
	LLViewerFetchedTexture *image = LLViewerTextureManager::getFetchedTexture(
		uuid, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, LLHost::invalid);
	return setTETextureCore(te,image);
}
S32 LLViewerObject::setTENormalMap(const U8 te, const LLUUID& uuid)
{
	LLViewerFetchedTexture *image = (uuid == LLUUID::null) ? NULL : LLViewerTextureManager::getFetchedTexture(
		uuid, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM, LLViewerTexture::LOD_TEXTURE, 0, 0, LLHost::invalid);
	return setTENormalMapCore(te, image);
}
S32 LLViewerObject::setTESpecularMap(const U8 te, const LLUUID& uuid)
{
	LLViewerFetchedTexture *image = (uuid == LLUUID::null) ? NULL : LLViewerTextureManager::getFetchedTexture(
		uuid, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM, LLViewerTexture::LOD_TEXTURE, 0, 0, LLHost::invalid);
	return setTESpecularMapCore(te, image);
}
S32 LLViewerObject::setTEColor(const U8 te, const LLColor3& color)
{
	return setTEColor(te, LLColor4(color));
}
S32 LLViewerObject::setTEColor(const U8 te, const LLColor4& color)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (color != tep->getColor())
	{
		retval = LLPrimitive::setTEColor(te, color);
		if (mDrawable.notNull() && retval)
		{
			dirtyMesh();
		}
	}
	return retval;
}
S32 LLViewerObject::setTEBumpmap(const U8 te, const U8 bump)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (bump != tep->getBumpmap())
	{
		retval = LLPrimitive::setTEBumpmap(te, bump);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markTextured(mDrawable);
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
		}
	}
	return retval;
}
S32 LLViewerObject::setTETexGen(const U8 te, const U8 texgen)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (texgen != tep->getTexGen())
	{
		retval = LLPrimitive::setTETexGen(te, texgen);
		setChanged(TEXTURE);
	}
	return retval;
}
S32 LLViewerObject::setTEMediaTexGen(const U8 te, const U8 media)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (media != tep->getMediaTexGen())
	{
		retval = LLPrimitive::setTEMediaTexGen(te, media);
		setChanged(TEXTURE);
	}
	return retval;
}
S32 LLViewerObject::setTEShiny(const U8 te, const U8 shiny)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (shiny != tep->getShiny())
	{
		retval = LLPrimitive::setTEShiny(te, shiny);
		setChanged(TEXTURE);
	}
	return retval;
}
S32 LLViewerObject::setTEFullbright(const U8 te, const U8 fullbright)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (fullbright != tep->getFullbright())
	{
		retval = LLPrimitive::setTEFullbright(te, fullbright);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}
S32 LLViewerObject::setTEMediaFlags(const U8 te, const U8 media_flags)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (media_flags != tep->getMediaFlags())
	{
		retval = LLPrimitive::setTEMediaFlags(te, media_flags);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD, TRUE);
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}
S32 LLViewerObject::setTEGlow(const U8 te, const F32 glow)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (glow != tep->getGlow())
	{
		retval = LLPrimitive::setTEGlow(te, glow);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}
S32 LLViewerObject::setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS("Material") << "No texture entry for te " << (S32)te
							 << ", object " << mID
							 << ", material " << pMaterialID
							 << LL_ENDL;
	}
	{
		LL_DEBUGS("Material") << "Changing texture entry for te " << (S32)te
							 << ", object " << mID
							 << ", material " << pMaterialID
							 << LL_ENDL;
		retval = LLPrimitive::setTEMaterialID(te, pMaterialID);
		refreshMaterials();
	}
	return retval;
}
S32 LLViewerObject::setTEMaterialParams(const U8 te, const LLMaterialPtr pMaterialParams)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS() << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
		return 0;
	}
	retval = LLPrimitive::setTEMaterialParams(te, pMaterialParams);
	LL_DEBUGS("Material") << "Changing material params for te " << (S32)te
							<< ", object " << mID
						   << " (" << retval << ")"
							<< LL_ENDL;
	setTENormalMap(te, (pMaterialParams) ? pMaterialParams->getNormalID() : LLUUID::null);
	setTESpecularMap(te, (pMaterialParams) ? pMaterialParams->getSpecularID() : LLUUID::null);
	refreshMaterials();
	return retval;
}
void LLViewerObject::refreshMaterials()
{
	setChanged(TEXTURE);
	if (mDrawable.notNull())
	{
		gPipeline.markTextured(mDrawable);
	}
}
S32 LLViewerObject::setTEScale(const U8 te, const F32 s, const F32 t)
{
	S32 retval = 0;
	retval = LLPrimitive::setTEScale(te, s, t);
	setChanged(TEXTURE);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}
S32 LLViewerObject::setTEScaleS(const U8 te, const F32 s)
{
	S32 retval = LLPrimitive::setTEScaleS(te, s);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}
S32 LLViewerObject::setTEScaleT(const U8 te, const F32 t)
{
	S32 retval = LLPrimitive::setTEScaleT(te, t);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}
S32 LLViewerObject::setTEOffset(const U8 te, const F32 s, const F32 t)
{
	S32 retval = LLPrimitive::setTEOffset(te, s, t);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}
S32 LLViewerObject::setTEOffsetS(const U8 te, const F32 s)
{
	S32 retval = LLPrimitive::setTEOffsetS(te, s);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}
S32 LLViewerObject::setTEOffsetT(const U8 te, const F32 t)
{
	S32 retval = LLPrimitive::setTEOffsetT(te, t);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}
S32 LLViewerObject::setTERotation(const U8 te, const F32 r)
{
	S32 retval = LLPrimitive::setTERotation(te, r);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}
LLViewerTexture *LLViewerObject::getTEImage(const U8 face) const
{
	if (face < getNumTEs())
	{
		LLViewerTexture* image = mTEImages[face];
		if (image)
		{
			return image;
		}
		else
		{
			return (LLViewerTexture*)(LLViewerFetchedTexture::sDefaultImagep);
		}
	}
	LL_ERRS() << llformat("Requested Image from invalid face: %d/%d",face,getNumTEs()) << LL_ENDL;
	return NULL;
}
bool LLViewerObject::isImageAlphaBlended(const U8 te) const
{
	LLViewerTexture* image = getTEImage(te);
	LLGLenum format = image ? image->getPrimaryFormat() : GL_RGB;
	switch (format)
	{
		case GL_RGBA:
		case GL_ALPHA:
		{
			return true;
		}
		break;
		case GL_RGB: break;
		default:
		{
			LL_WARNS() << "Unexpected tex format in LLViewerObject::isImageAlphaBlended...returning no alpha." << LL_ENDL;
		}
		break;
	}
	return false;
}
LLViewerTexture *LLViewerObject::getTENormalMap(const U8 face) const
{
	if (face < getNumTEs())
	{
		LLViewerTexture* image = mTENormalMaps[face];
		if (image)
		{
			return image;
		}
		else
		{
			return (LLViewerTexture*)(LLViewerFetchedTexture::sDefaultImagep);
		}
	}
	LL_ERRS() << llformat("Requested Image from invalid face: %d/%d",face,getNumTEs()) << LL_ENDL;
	return NULL;
}
LLViewerTexture *LLViewerObject::getTESpecularMap(const U8 face) const
{
	if (face < getNumTEs())
	{
		LLViewerTexture* image = mTESpecularMaps[face];
		if (image)
		{
			return image;
		}
		else
		{
			return (LLViewerTexture*)(LLViewerFetchedTexture::sDefaultImagep);
		}
	}
	LL_ERRS() << llformat("Requested Image from invalid face: %d/%d",face,getNumTEs()) << LL_ENDL;
	return NULL;
}
void LLViewerObject::fitFaceTexture(const U8 face)
{
	LL_INFOS() << "fitFaceTexture not implemented" << LL_ENDL;
}
LLBBox LLViewerObject::getBoundingBoxAgent() const
{
	LLVector3 position_agent;
	LLQuaternion rot;
	LLViewerObject* avatar_parent = NULL;
	LLViewerObject* root_edit = (LLViewerObject*)getRootEdit();
	if (root_edit)
	{
		avatar_parent = (LLViewerObject*)root_edit->getParent();
	}
	if (avatar_parent && avatar_parent->isAvatar() &&
		root_edit && root_edit->mDrawable.notNull() && root_edit->mDrawable->getXform()->getParent())
	{
		LLXform* parent_xform = root_edit->mDrawable->getXform()->getParent();
		position_agent = (getPositionEdit() * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition();
		rot = getRotationEdit() * parent_xform->getWorldRotation();
	}
	else
	{
		position_agent = getPositionAgent();
		rot = getRotationRegion();
	}
	return LLBBox( position_agent, rot, getScale() * -0.5f, getScale() * 0.5f );
}
U32 LLViewerObject::getNumVertices() const
{
	U32 num_vertices = 0;
	if (mDrawable.notNull())
	{
		S32 i, num_faces;
		num_faces = mDrawable->getNumFaces();
		for (i = 0; i < num_faces; i++)
		{
			LLFace * facep = mDrawable->getFace(i);
			if (facep)
			{
				num_vertices += facep->getGeomCount();
			}
		}
	}
	return num_vertices;
}
U32 LLViewerObject::getNumIndices() const
{
	U32 num_indices = 0;
	if (mDrawable.notNull())
	{
		S32 i, num_faces;
		num_faces = mDrawable->getNumFaces();
		for (i = 0; i < num_faces; i++)
		{
			LLFace * facep = mDrawable->getFace(i);
			if (facep)
			{
				num_indices += facep->getIndicesCount();
			}
		}
	}
	return num_indices;
}
S32 LLViewerObject::countInventoryContents(LLAssetType::EType type)
{
	S32 count = 0;
	if( mInventory )
	{
		LLInventoryObject::object_list_t::const_iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::const_iterator end = mInventory->end();
		for(  ; it != end ; ++it )
		{
			if( (*it)->getType() == type )
			{
				++count;
			}
		}
	}
	return count;
}
void LLViewerObject::setCanSelect(BOOL canSelect)
{
	mbCanSelect = canSelect;
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		child->mbCanSelect = canSelect;
	}
}
void LLViewerObject::setDebugText(const std::string &utf8text)
{
	if (utf8text.empty() && mHudTextString.empty())
	{
		if(mText)
			mText->markDead();
		mText = NULL;
		return;
	}
	if (!mText)
	{
		initHudText();
	}
	mText->setColor(utf8text.empty() ? mHudTextColor : LLColor4::white );
	mText->setString(utf8text.empty() ? mHudTextString :  utf8text );
	mText->setZCompare(utf8text.empty());
	mText->setDoFade(utf8text.empty());
	updateText();
}
void LLViewerObject::initHudText()
{
	mText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
	mText->setFont(LLFontGL::getFontSansSerif());
	mText->setVertAlignment(LLHUDText::ALIGN_VERT_TOP);
	mText->setMaxLines(-1);
	mText->setSourceObject(this);
	mText->setOnHUDAttachment(isHUDAttachment());
}
std::string LLViewerObject::getDebugText()
{
	if(mText)
		return mText->getStringUTF8();
	return "";
}
void LLViewerObject::setIcon(LLViewerTexture* icon_image)
{
	if (!mIcon)
	{
		mIcon = (LLHUDIcon *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_ICON);
		mIcon->setSourceObject(this);
		mIcon->setImage(icon_image);
		mIcon->setScale(0.03f);
	}
	else
	{
		mIcon->restartLifeTimer();
	}
}
void LLViewerObject::clearIcon()
{
	if (mIcon)
	{
		mIcon = NULL;
	}
}
LLViewerObject* LLViewerObject::getSubParent()
{
	return (LLViewerObject*) getParent();
}
const LLViewerObject* LLViewerObject::getSubParent() const
{
	return (const LLViewerObject*) getParent();
}
BOOL LLViewerObject::isOnMap()
{
	return mOnMap;
}
void LLViewerObject::updateText()
{
	if (!isDead())
	{
		if (mText.notNull())
		{
			LLVector3 up_offset(0,0,0);
			up_offset.mV[2] = getScale().mV[VZ]*0.6f;
			if (mDrawable.notNull())
			{
				mText->setPositionAgent(getRenderPosition() + up_offset);
			}
			else
			{
				mText->setPositionAgent(getPositionAgent() + up_offset);
			}
		}
	}
}
LLVOAvatar* LLViewerObject::asAvatar()
{
	return NULL;
}
LLVOVolume* LLViewerObject::asVolume()
{
	return nullptr;
}
LLVOAvatar* LLViewerObject::getAvatarAncestor()
{
	LLViewerObject *pobj = (LLViewerObject*) getParent();
	while (pobj)
	{
		LLVOAvatar *av = pobj->asAvatar();
		if (av)
		{
			return av;
		}
		pobj =  (LLViewerObject*) pobj->getParent();
	}
	return NULL;
}
BOOL LLViewerObject::isParticleSource() const
{
	return !mPartSourcep.isNull() && !mPartSourcep->isDead();
}
void LLViewerObject::setParticleSource(const LLPartSysData& particle_parameters, const LLUUID& owner_id)
{
	if (mPartSourcep)
	{
		deleteParticleSource();
	}
	LLPointer<LLViewerPartSourceScript> pss = LLViewerPartSourceScript::createPSS(this, particle_parameters);
	mPartSourcep = pss;
	if (mPartSourcep)
	{
		mPartSourcep->setOwnerUUID(owner_id);
		if (mPartSourcep->getImage()->getID() != mPartSourcep->mPartSysData.mPartImageID)
		{
			LLViewerTexture* image;
			if (mPartSourcep->mPartSysData.mPartImageID == LLUUID::null)
			{
				image = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.tga");
			}
			else
			{
				image = LLViewerTextureManager::getFetchedTexture(mPartSourcep->mPartSysData.mPartImageID);
			}
			mPartSourcep->setImage(image);
		}
	}
	LLViewerPartSim::getInstance()->addPartSource(pss);
}
void LLViewerObject::unpackParticleSource(const S32 block_num, const LLUUID& owner_id)
{
	if (!mPartSourcep.isNull() && mPartSourcep->isDead())
	{
		mPartSourcep = NULL;
	}
	if (mPartSourcep)
	{
		if (!LLViewerPartSourceScript::unpackPSS(this, mPartSourcep, block_num))
		{
			mPartSourcep->setDead();
			mPartSourcep = NULL;
		}
	}
	else
	{
		LLPointer<LLViewerPartSourceScript> pss = LLViewerPartSourceScript::unpackPSS(this, NULL, block_num);
		if(LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagParticles)) return;
		if (pss)
		{
			pss->setOwnerUUID(owner_id);
			mPartSourcep = pss;
			LLViewerPartSim::getInstance()->addPartSource(pss);
		}
	}
	if (mPartSourcep)
	{
		if (mPartSourcep->getImage()->getID() != mPartSourcep->mPartSysData.mPartImageID)
		{
			LLViewerTexture* image;
			if (mPartSourcep->mPartSysData.mPartImageID == LLUUID::null)
			{
				image = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
			}
			else
			{
				image = LLViewerTextureManager::getFetchedTexture(mPartSourcep->mPartSysData.mPartImageID);
			}
			mPartSourcep->setImage(image);
		}
	}
}
void LLViewerObject::unpackParticleSource(LLDataPacker &dp, const LLUUID& owner_id, bool legacy)
{
	if (!mPartSourcep.isNull() && mPartSourcep->isDead())
	{
		mPartSourcep = NULL;
	}
	if (mPartSourcep)
	{
		if (!LLViewerPartSourceScript::unpackPSS(this, mPartSourcep, dp, legacy))
		{
			mPartSourcep->setDead();
			mPartSourcep = NULL;
		}
	}
	else
	{
		LLPointer<LLViewerPartSourceScript> pss = LLViewerPartSourceScript::unpackPSS(this, NULL, dp, legacy);
		if(LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagParticles)) return;
		if (pss)
		{
			pss->setOwnerUUID(owner_id);
			mPartSourcep = pss;
			LLViewerPartSim::getInstance()->addPartSource(pss);
		}
	}
	if (mPartSourcep)
	{
		if (mPartSourcep->getImage()->getID() != mPartSourcep->mPartSysData.mPartImageID)
		{
			LLViewerTexture* image;
			if (mPartSourcep->mPartSysData.mPartImageID == LLUUID::null)
			{
				image = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
			}
			else
			{
				image = LLViewerTextureManager::getFetchedTexture(mPartSourcep->mPartSysData.mPartImageID);
			}
			mPartSourcep->setImage(image);
		}
	}
}
void LLViewerObject::deleteParticleSource()
{
	if (mPartSourcep.notNull())
	{
		mPartSourcep->setDead();
		mPartSourcep = NULL;
	}
}
void LLViewerObject::updateDrawable(BOOL force_damped)
{
	if (!isChanged(MOVED))
	{
	}
	else if (mDrawable.notNull() &&
		!mDrawable->isState(LLDrawable::ON_MOVE_LIST))
	{
		BOOL damped_motion =
			!isChanged(SHIFTED) &&
			(	force_damped ||
				(	!isSelected() &&
					(	mDrawable->isRoot() ||
						(getParent() && !((LLViewerObject*)getParent())->isSelected())
					) &&
					getPCode() == LL_PCODE_VOLUME &&
					getVelocity().isExactlyZero() &&
					mDrawable->getGeneration() != -1
				)
			);
		gPipeline.markMoved(mDrawable, damped_motion);
	}
	clearChanged(SHIFTED);
}
F32 LLViewerObject::getVObjRadius() const
{
	return mDrawable.notNull() ? mDrawable->getRadius() : 0.f;
}
void LLViewerObject::setAttachedSound(const LLUUID &audio_uuid, const LLUUID& owner_id, const F32 gain, const U8 flags)
{
	if (!gAudiop)
	{
		return;
	}
	if (audio_uuid.isNull())
	{
		if (!mAudioSourcep || (flags & LL_SOUND_FLAG_STOP && !mAudioSourcep->isLoop()))
		{
			return;
		}
		if (mAudioSourcep->isLoop() && !mAudioSourcep->hasPendingPreloads())
		{
			gAudiop->cleanupAudioSource(mAudioSourcep);
			mAudioSourcep = NULL;
		}
		else if (flags & LL_SOUND_FLAG_STOP)
		{
			mAudioSourcep->play(LLUUID::null);
		}
		return;
	}
	if (flags & LL_SOUND_FLAG_LOOP
		&& mAudioSourcep && mAudioSourcep->isLoop() && mAudioSourcep->getCurrentData()
		&& mAudioSourcep->getCurrentData()->getID() == audio_uuid)
	{
		return;
	}
	if ( mAudioSourcep && mAudioSourcep->isDone() )
	{
		gAudiop->cleanupAudioSource(mAudioSourcep);
		mAudioSourcep = NULL;
	}
	if (mAudioSourcep && mAudioSourcep->isMuted() &&
		mAudioSourcep->getCurrentData() && mAudioSourcep->getCurrentData()->getID() == audio_uuid)
	{
		return;
	}
	getAudioSource(owner_id);
	if (mAudioSourcep)
	{
		BOOL queue = flags & LL_SOUND_FLAG_QUEUE;
		mAudioGain = gain;
		mAudioSourcep->setGain(gain);
		mAudioSourcep->setLoop(flags & LL_SOUND_FLAG_LOOP);
		mAudioSourcep->setSyncMaster(flags & LL_SOUND_FLAG_SYNC_MASTER);
		mAudioSourcep->setSyncSlave(flags & LL_SOUND_FLAG_SYNC_SLAVE);
		mAudioSourcep->setQueueSounds(queue);
		if( gAgent.canAccessMaturityAtGlobal(this->getPositionGlobal()) )
		{
			mAudioSourcep->play(audio_uuid);
		}
	}
}
LLAudioSource *LLViewerObject::getAudioSource(const LLUUID& owner_id)
{
	if (!mAudioSourcep)
	{
		LLAudioSourceVO *asvop = new LLAudioSourceVO(mID, owner_id, 0.01f, this);
		mAudioSourcep = asvop;
		if(gAudiop) gAudiop->addAudioSource(asvop);
	}
	return mAudioSourcep;
}
void LLViewerObject::adjustAudioGain(const F32 gain)
{
	if (!gAudiop)
	{
		return;
	}
	if (mAudioSourcep)
	{
		mAudioGain = gain;
		mAudioSourcep->setGain(mAudioGain);
	}
}
bool LLViewerObject::unpackParameterEntry(U16 param_type, LLDataPacker *dp)
{
	if (LLNetworkData::PARAMS_MESH == param_type)
	{
		param_type = LLNetworkData::PARAMS_SCULPT;
	}
	ExtraParameter* param = getExtraParameterEntryCreate(param_type);
	if (param)
	{
		param->data->unpack(*dp);
		*param->in_use = TRUE;
		parameterChanged(param_type, param->data, TRUE, false);
		return true;
	}
	else
	{
		return false;
	}
}
LLViewerObject::ExtraParameter* LLViewerObject::createNewParameterEntry(U16 param_type)
{
	LLNetworkData* new_block = NULL;
	bool* in_use = NULL;
	switch (param_type)
	{
		case LLNetworkData::PARAMS_FLEXIBLE:
		{
			new_block = &mFlexibleObjectData;
			in_use = &mFlexibleObjectDataInUse;
			break;
		}
		case LLNetworkData::PARAMS_LIGHT:
		{
			new_block = &mLightParams;
			in_use = &mLightParamsInUse;
			break;
		}
		case LLNetworkData::PARAMS_SCULPT:
		{
			new_block = &mSculptParams;
			in_use = &mSculptParamsInUse;
			break;
		}
		case LLNetworkData::PARAMS_LIGHT_IMAGE:
		{
			new_block = &mLightImageParams;
			in_use = &mLightImageParamsInUse;
			break;
		}
		case LLNetworkData::PARAMS_EXTENDED_MESH:
		{
			new_block = &mExtendedMeshParams;
			in_use = &mExtendedMeshParamsInUse;
			break;
		}
		default:
		{
			LL_INFOS() << "Unknown param type. (" << llformat("0x%2x", param_type) << ")" << LL_ENDL;
			break;
		}
	};
	ExtraParameter& entry = mExtraParameterList[U32(param_type >> 4) - 1];
	if (new_block)
	{
		entry.in_use = in_use;
		*entry.in_use = false;
		entry.data = new_block;
		return &entry;
	}
	else
	{
		entry.is_invalid = true;
	}
	return NULL;
}
bool LLViewerObject::setParameterEntry(U16 param_type, const LLNetworkData& new_value, bool local_origin)
{
	ExtraParameter* param = getExtraParameterEntryCreate(param_type);
	if (param)
	{
		if (*(param->in_use) && new_value == *(param->data))
		{
			return false;
		}
		*param->in_use = true;
		param->data->copy(new_value);
		parameterChanged(param_type, param->data, TRUE, local_origin);
		return true;
	}
	else
	{
		return false;
	}
}
bool LLViewerObject::setParameterEntryInUse(U16 param_type, BOOL in_use, bool local_origin)
{
	if (param_type <= LLNetworkData::PARAMS_MAX)
	{
		ExtraParameter* param = (in_use ? getExtraParameterEntryCreate(param_type) : &getExtraParameterEntry(param_type));
		if (param && param->data && *param->in_use != (bool)in_use)
		{
			*param->in_use = in_use;
			parameterChanged(param_type, param->data, in_use, local_origin);
			return true;
		}
	}
	return false;
}
void LLViewerObject::parameterChanged(U16 param_type, bool local_origin)
{
	if (param_type <= LLNetworkData::PARAMS_MAX)
	{
		const ExtraParameter& param = getExtraParameterEntry(param_type);
		if (param.data)
		{
			parameterChanged(param_type, param.data, *param.in_use, local_origin);
		}
	}
}
void LLViewerObject::parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin)
{
	if (local_origin)
	{
		LLViewerRegion* regionp = getRegion();
		if(!regionp) return;
		U8 tmp[MAX_OBJECT_PARAMS_SIZE];
		LLDataPackerBinaryBuffer dpb(tmp, MAX_OBJECT_PARAMS_SIZE);
		if (data->pack(dpb))
		{
			U32 datasize = (U32)dpb.getCurrentSize();
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_ObjectExtraParams);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, mLocalID );
			msg->addU16Fast(_PREHASH_ParamType, param_type);
			msg->addBOOLFast(_PREHASH_ParamInUse, in_use);
			msg->addU32Fast(_PREHASH_ParamSize, datasize);
			msg->addBinaryDataFast(_PREHASH_ParamData, tmp, datasize);
			msg->sendReliable( regionp->getHost() );
		}
		else
		{
			LL_WARNS() << "Failed to send object extra parameters: " << param_type << LL_ENDL;
		}
	}
}
void LLViewerObject::setDrawableState(U32 state, BOOL recursive)
{
	if (mDrawable)
	{
		mDrawable->setState(state);
	}
	if (recursive)
	{
		for (child_list_t::iterator iter = mChildList.begin();
			 iter != mChildList.end(); iter++)
		{
			LLViewerObject* child = *iter;
			child->setDrawableState(state, recursive);
		}
	}
}
void LLViewerObject::clearDrawableState(U32 state, BOOL recursive)
{
	if (mDrawable)
	{
		mDrawable->clearState(state);
	}
	if (recursive)
	{
		for (child_list_t::iterator iter = mChildList.begin();
			 iter != mChildList.end(); iter++)
		{
			LLViewerObject* child = *iter;
			child->clearDrawableState(state, recursive);
		}
	}
}
BOOL LLViewerObject::isDrawableState(U32 state, BOOL recursive) const
{
	BOOL matches = FALSE;
	if (mDrawable)
	{
		matches = mDrawable->isState(state);
	}
	if (recursive)
	{
		for (child_list_t::const_iterator iter = mChildList.begin();
			 (iter != mChildList.end()) && matches; iter++)
		{
			LLViewerObject* child = *iter;
			matches &= child->isDrawableState(state, recursive);
		}
	}
	return matches;
}
BOOL LLViewerObject::permAnyOwner() const
{
	if (isRootEdit())
	{
		return flagObjectAnyOwner();
	}
	else
	{
		return ((LLViewerObject*)getParent())->permAnyOwner();
	}
}
BOOL LLViewerObject::permYouOwner() const
{
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLViewerLogin::getInstance()->isInProductionGrid()
			&& (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectYouOwner();
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permYouOwner();
	}
}
BOOL LLViewerObject::permGroupOwner() const
{
	if (isRootEdit())
	{
		return flagObjectGroupOwned();
	}
	else
	{
		return ((LLViewerObject*)getParent())->permGroupOwner();
	}
}
BOOL LLViewerObject::permOwnerModify() const
{
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLViewerLogin::getInstance()->isInProductionGrid()
			&& (gAgent.getGodLevel() >= GOD_MAINTENANCE))
	{
			return TRUE;
	}
# endif
		return flagObjectOwnerModify();
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permOwnerModify();
	}
}
BOOL LLViewerObject::permModify() const
{
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLViewerLogin::getInstance()->isInProductionGrid()
			&& (gAgent.getGodLevel() >= GOD_MAINTENANCE))
	{
			return TRUE;
	}
# endif
		return flagObjectModify();
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permModify();
	}
}
BOOL LLViewerObject::permCopy() const
{
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLViewerLogin::getInstance()->isInProductionGrid()
			&& (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectCopy();
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permCopy();
	}
}
BOOL LLViewerObject::permMove() const
{
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLViewerLogin::getInstance()->isInProductionGrid()
			&& (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectMove();
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permMove();
	}
}
BOOL LLViewerObject::permTransfer() const
{
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLViewerLogin::getInstance()->isInProductionGrid()
			&& (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectTransfer();
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permTransfer();
	}
}
BOOL LLViewerObject::allowOpen() const
{
	return !flagInventoryEmpty() && (permYouOwner() || permModify()) && ((!rlv_handler_t::isEnabled()) || (gRlvHandler.canEdit(this)));
}
LLViewerObject::LLInventoryCallbackInfo::~LLInventoryCallbackInfo()
{
	if (mListener)
	{
		mListener->clearVOInventoryListener();
	}
}
void LLViewerObject::updateVolume(const LLVolumeParams& volume_params)
{
	if (setVolume(volume_params, 1))
	{
		sendShapeUpdate();
		markForUpdate(TRUE);
	}
}
void LLViewerObject::recursiveMarkForUpdate(BOOL priority)
{
	for (LLViewerObject::child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		child->markForUpdate(priority);
	}
	markForUpdate(priority);
}
void LLViewerObject::markForUpdate(BOOL priority)
{
	if (mDrawable.notNull())
	{
		gPipeline.markTextured(mDrawable);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, priority);
	}
}
void LLViewerObject::markForUnload(BOOL priority)
{
	if (mDrawable.notNull())
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::FOR_UNLOAD, priority);
	}
}
bool LLViewerObject::isPermanentEnforced() const
{
	return flagObjectPermanent() && (mRegionp != gAgent.getRegion()) && !gAgent.isGodlike();
}
bool LLViewerObject::getIncludeInSearch() const
{
	return flagIncludeInSearch();
}
void LLViewerObject::setIncludeInSearch(bool include_in_search)
{
	setFlags(FLAGS_INCLUDE_IN_SEARCH, include_in_search);
}
void LLViewerObject::setRegion(LLViewerRegion *regionp)
{
	if (!regionp)
	{
		LL_WARNS() << "viewer object set region to NULL" << LL_ENDL;
	}
	if(regionp != mRegionp)
	{
		if (isHUDAttachment() || (isAttachment() && hud_teleport_logging_active()))
		{
			LL_INFOS("HUDTeleport") << "setRegion"
				<< " hud=" << (isHUDAttachment() ? 1 : 0)
				<< " attach=" << (isAttachment() ? 1 : 0)
				<< " id=" << getID()
				<< " from=" << (mRegionp ? mRegionp->getName() : std::string("null"))
				<< " to=" << (regionp ? regionp->getName() : std::string("null"))
				<< LL_ENDL;
		}
		if(mRegionp)
		{
			mRegionp->removeFromCreatedList(getLocalID());
		}
		if(regionp)
		{
			regionp->addToCreatedList(getLocalID());
		}
	}
	mLatestRecvPacketID = 0;
	mRegionp = regionp;
	for (child_list_t::iterator i = mChildList.begin(); i != mChildList.end(); ++i)
	{
		LLViewerObject* child = *i;
		child->setRegion(regionp);
	}
	if (mControlAvatar)
	{
		mControlAvatar->setRegion(regionp);
	}
	setChanged(MOVED | SILHOUETTE);
	updateDrawable(FALSE);
}
void	LLViewerObject::updateRegion(LLViewerRegion *regionp)
{
}
bool LLViewerObject::specialHoverCursor() const
{
	return flagUsePhysics()
			|| flagHandleTouch()
			|| (mClickAction != 0);
}
void LLViewerObject::updateFlags(BOOL physics_changed)
{
	LLViewerRegion* regionp = getRegion();
	if(!regionp) return;
	gMessageSystem->newMessage("ObjectFlagUpdate");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, getLocalID() );
	gMessageSystem->addBOOLFast(_PREHASH_UsePhysics, flagUsePhysics() );
	gMessageSystem->addBOOL("IsTemporary", flagTemporaryOnRez() );
	gMessageSystem->addBOOL("IsPhantom", flagPhantom() );
	gMessageSystem->addBOOL("CastsShadows", FALSE );
	if (physics_changed)
	{
		gMessageSystem->nextBlock("ExtraPhysics");
		gMessageSystem->addU8("PhysicsShapeType", getPhysicsShapeType() );
		gMessageSystem->addF32("Density", getPhysicsDensity() );
		gMessageSystem->addF32("Friction", getPhysicsFriction() );
		gMessageSystem->addF32("Restitution", getPhysicsRestitution() );
		gMessageSystem->addF32("GravityMultiplier", getPhysicsGravity() );
	}
	gMessageSystem->sendReliable( regionp->getHost() );
}
BOOL LLViewerObject::setFlags(U32 flags, BOOL state)
{
	BOOL setit = setFlagsWithoutUpdate(flags, state);
	{
		updateFlags();
	}
	return setit;
}
BOOL LLViewerObject::setFlagsWithoutUpdate(U32 flags, BOOL state)
{
	BOOL setit = FALSE;
	if (state)
	{
		if ((mFlags & flags) != flags)
		{
			mFlags |= flags;
			setit = TRUE;
		}
	}
	else
	{
		if ((mFlags & flags) != 0)
		{
			mFlags &= ~flags;
			setit = TRUE;
		}
	}
	return setit;
}
void LLViewerObject::setPhysicsShapeType(U8 type)
{
	mPhysicsShapeUnknown = false;
	if (type != mPhysicsShapeType)
	{
	mPhysicsShapeType = type;
	mCostStale = true;
}
}
void LLViewerObject::setPhysicsGravity(F32 gravity)
{
	mPhysicsGravity = gravity;
}
void LLViewerObject::setPhysicsFriction(F32 friction)
{
	mPhysicsFriction = friction;
}
void LLViewerObject::setPhysicsDensity(F32 density)
{
	mPhysicsDensity = density;
}
void LLViewerObject::setPhysicsRestitution(F32 restitution)
{
	mPhysicsRestitution = restitution;
}
U8 LLViewerObject::getPhysicsShapeType() const
{
	if (mPhysicsShapeUnknown)
	{
		gObjectList.updatePhysicsFlags(this);
	}
	return mPhysicsShapeType;
}
void LLViewerObject::applyAngularVelocity(F32 dt)
{
	mRotTime += dt;
	LLVector3 ang_vel = getAngularVelocity();
	F32 omega = ang_vel.magVecSquared();
	F32 angle = 0.0f;
	LLQuaternion dQ;
	if (omega > 0.00001f)
	{
		omega = sqrt(omega);
		angle = omega * dt;
		ang_vel *= 1.f/omega;
		dQ.setQuat(angle, ang_vel);
		static const LLCachedControl<bool> use_new_target_omega ("UseNewTargetOmegaCode", true);
		if (use_new_target_omega)
		{
			mAngularVelocityRot *= dQ;
		}
		setRotation(getRotation()*dQ);
		setChanged(MOVED | SILHOUETTE);
	}
}
void LLViewerObject::resetRot()
{
	mRotTime = 0.0f;
	static const LLCachedControl<bool> use_new_target_omega ("UseNewTargetOmegaCode", true);
	if(use_new_target_omega)
	{
		mAngularVelocityRot.loadIdentity();
	}
}
U32 LLViewerObject::getPartitionType() const
{
	return LLViewerRegion::PARTITION_NONE;
}
void LLViewerObject::dirtySpatialGroup(BOOL priority) const
{
	if (mDrawable)
	{
		LLSpatialGroup* group = mDrawable->getSpatialGroup();
		if (group)
		{
			group->dirtyGeom();
			gPipeline.markRebuild(group, priority);
		}
	}
}
void LLViewerObject::dirtyMesh()
{
	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL);
	}
}
F32 LLAlphaObject::getPartSize(S32 idx)
{
	return 0.f;
}
void LLAlphaObject::getBlendFunc(S32 face, U32& src, U32& dst)
{
}
void LLStaticViewerObject::updateDrawable(BOOL force_damped)
{
	if (mDrawable.notNull())
	{
		mDrawable->updateXform(TRUE);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	clearChanged(SHIFTED);
}
void LLViewerObject::saveUnselectedChildrenPosition(std::vector<LLVector3>& positions)
{
	if(mChildList.empty() || !positions.empty())
	{
		return ;
	}
	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			positions.push_back(childp->getPositionEdit());
		}
	}
	return ;
}
void LLViewerObject::saveUnselectedChildrenRotation(std::vector<LLQuaternion>& rotations)
{
	if(mChildList.empty())
	{
		return ;
	}
	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			rotations.push_back(childp->getRotationEdit());
		}
	}
	return ;
}
void LLViewerObject::resetChildrenRotationAndPosition(const std::vector<LLQuaternion>& rotations,
											const std::vector<LLVector3>& positions)
{
	if(mChildList.empty())
	{
		return ;
	}
	S32 index = 0 ;
	LLQuaternion inv_rotation = ~getRotationEdit() ;
	LLVector3 offset = getPositionEdit() ;
	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			if (childp->getPCode() != LL_PCODE_LEGACY_AVATAR)
			{
				childp->setRotation(rotations[index] * inv_rotation);
				childp->setPosition((positions[index] - offset) * inv_rotation);
				LLManip::rebuild(childp);
			}
			else
			{
				LLVector3 reset_pos = (positions[index] - offset) * inv_rotation ;
				LLQuaternion reset_rot = rotations[index] * inv_rotation ;
				((LLVOAvatar*)childp)->mDrawable->mXform.setPosition(reset_pos);
				((LLVOAvatar*)childp)->mDrawable->mXform.setRotation(reset_rot) ;
				((LLVOAvatar*)childp)->mDrawable->getVObj()->setPosition(reset_pos, TRUE);
				((LLVOAvatar*)childp)->mDrawable->getVObj()->setRotation(reset_rot, TRUE) ;
				LLManip::rebuild(childp);
			}
			index++;
		}
	}
	return ;
}
void LLViewerObject::resetChildrenPosition(const LLVector3& offset, BOOL simplified, BOOL skip_avatar_child)
{
	if(mChildList.empty())
	{
		return ;
	}
	LLVector3 child_offset;
	if(simplified)
	{
		child_offset = offset * ~getRotation();
	}
	else
	{
		if (isAttachment() && mDrawable.notNull())
		{
			LLXform* attachment_point_xform = mDrawable->getXform()->getParent();
			LLQuaternion parent_rotation = getRotation() * attachment_point_xform->getWorldRotation();
			child_offset = offset * ~parent_rotation;
		}
		else
		{
			child_offset = offset * ~getRenderRotation();
		}
	}
	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			if (childp->getPCode() != LL_PCODE_LEGACY_AVATAR)
			{
				childp->setPosition(childp->getPosition() + child_offset);
				LLManip::rebuild(childp);
			}
			else
			{
				if(!skip_avatar_child)
				{
					LLVector3 reset_pos = ((LLVOAvatar*)childp)->mDrawable->mXform.getPosition() + child_offset ;
					((LLVOAvatar*)childp)->mDrawable->mXform.setPosition(reset_pos);
					((LLVOAvatar*)childp)->mDrawable->getVObj()->setPosition(reset_pos);
					LLManip::rebuild(childp);
				}
			}
		}
	}
	return ;
}
BOOL	LLViewerObject::isTempAttachment() const
{
	return (mID.notNull() && (mID == mAttachmentItemID));
}
std::string LLViewerObject::getAttachmentPointName() const
{
	S32 point = ATTACHMENT_ID_FROM_STATE(mAttachmentState);
	LLVOAvatar::attachment_map_t::iterator it = gAgentAvatarp->mAttachmentPoints.find(point);
	if(it != gAgentAvatarp->mAttachmentPoints.end())
	{
		return it->second->getName();
	}
	return llformat("unsupported point %d", point);
}
const LLUUID &LLViewerObject::getAttachmentItemID() const
{
	return mAttachmentItemID;
}
void LLViewerObject::setAttachmentItemID(const LLUUID &id)
{
	mAttachmentItemID = id;
}
EObjectUpdateType LLViewerObject::getLastUpdateType() const
{
	return mLastUpdateType;
}
void LLViewerObject::setLastUpdateType(EObjectUpdateType last_update_type)
{
	mLastUpdateType = last_update_type;
}
BOOL LLViewerObject::getLastUpdateCached() const
{
	return mLastUpdateCached;
}
void LLViewerObject::setLastUpdateCached(BOOL last_update_cached)
{
	mLastUpdateCached = last_update_cached;
}
const LLUUID &LLViewerObject::extractAttachmentItemID()
{
	LLUUID item_id = LLUUID::null;
	LLNameValue* item_id_nv = getNVPair("AttachItemID");
	if (item_id_nv)
	{
		const char* s = item_id_nv->getString();
		if (s)
		{
			item_id.set(s);
		}
	}
	setAttachmentItemID(item_id);
	return getAttachmentItemID();
}
LLVOAvatar* LLViewerObject::getAvatar() const
{
	if (getControlAvatar())
	{
		return getControlAvatar();
	}
	if (isAttachment())
	{
		LLViewerObject* vobj = (LLViewerObject*) getParent();
		while (vobj && !vobj->asAvatar())
		{
			vobj = (LLViewerObject*) vobj->getParent();
		}
		return (LLVOAvatar*) vobj;
	}
	return NULL;
}
class ObjectPhysicsProperties : public LLHTTPNode
{
public:
	virtual void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const
	{
		LLSD object_data = input["body"]["ObjectData"];
		S32 num_entries = object_data.size();
		for ( S32 i = 0; i < num_entries; i++ )
		{
			LLSD& curr_object_data = object_data[i];
			U32 local_id = curr_object_data["LocalID"].asInteger();
			struct f : public LLSelectedNodeFunctor
			{
				U32 mID;
				f(const U32& id) : mID(id) {}
				virtual bool apply(LLSelectNode* node)
				{
					return (node->getObject() && node->getObject()->mLocalID == mID );
				}
			} func(local_id);
			LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode(&func);
			if (node)
			{
				U8 type = (U8)curr_object_data["PhysicsShapeType"].asInteger();
				F32 density = (F32)curr_object_data["Density"].asReal();
				F32 friction = (F32)curr_object_data["Friction"].asReal();
				F32 restitution = (F32)curr_object_data["Restitution"].asReal();
				F32 gravity = (F32)curr_object_data["GravityMultiplier"].asReal();
				node->getObject()->setPhysicsShapeType(type);
				node->getObject()->setPhysicsGravity(gravity);
				node->getObject()->setPhysicsFriction(friction);
				node->getObject()->setPhysicsDensity(density);
				node->getObject()->setPhysicsRestitution(restitution);
			}
		}
		dialog_refresh_all();
	};
};
LLHTTPRegistration<ObjectPhysicsProperties>
	gHTTPRegistrationObjectPhysicsProperties("/message/ObjectPhysicsProperties");
