/** 
 * @file lldrawable.cpp
 * @brief LLDrawable class implementation
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
#include "lldrawable.h"
#include "material_codes.h"
#include "llagent.h"
#include "llcriticaldamp.h"
#include "llface.h"
#include "lllightconstants.h"
#include "llmatrix4a.h"
#include "llsky.h"
#include "llsurfacepatch.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "llvolume.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llvosurfacepatch.h"
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llcontrolavatar.h"
#include "lldrawpoolavatar.h"
const F32 MIN_INTERPOLATE_DISTANCE_SQUARED = 0.001f * 0.001f;
const F32 MAX_INTERPOLATE_DISTANCE_SQUARED = 10.f * 10.f;
const F32 OBJECT_DAMPING_TIME_CONSTANT = 0.06f;
const F32 MIN_SHADOW_CASTER_RADIUS = 2.0f;
static LLTrace::BlockTimerStatHandle FTM_CULL_REBOUND("Cull Rebound");
extern bool gShiftFrame;
U32 LLDrawable::sNumZombieDrawables = 0;
F32 LLDrawable::sCurPixelAngle = 0;
std::vector<LLPointer<LLDrawable> > LLDrawable::sDeadList;
#define FORCE_INVISIBLE_AREA 16.f
void LLDrawable::incrementVisible()
{
	LLViewerOctreeEntryData::incrementVisible();
	sCurPixelAngle = (F32) gViewerWindow->getWindowHeightRaw()/LLViewerCamera::getInstance()->getView();
}
LLDrawable::LLDrawable(LLViewerObject *vobj)
:	LLViewerOctreeEntryData(LLViewerOctreeEntry::LLDRAWABLE),
	mVObjp(vobj)
{
	init();
}
void LLDrawable::init()
{
	mParent = NULL;
	mRenderType = 0;
	mCurrentScale = LLVector3(1,1,1);
	mDistanceWRTCamera = 0.0f;
	mState     = 0;
	mRadius = 0.f;
	mGeneration = -1;
	mSpatialBridge = NULL;
	LLViewerOctreeEntry* entry = NULL;
	setOctreeEntry(entry);
	initVisible(sCurVisible - 2);
}
void LLDrawable::unload()
{
	LLVOVolume *pVVol = getVOVolume();
	pVVol->setNoLOD();
	for (S32 i = 0; i < getNumFaces(); i++)
	{
		LLFace* facep = getFace(i);
		if (facep->isState(LLFace::RIGGED))
		{
			LLDrawPoolAvatar* pool = (LLDrawPoolAvatar*)facep->getPool();
			if (pool) {
				pool->removeRiggedFace(facep);
			}
			facep->setVertexBuffer(NULL);
		}
		facep->clearState(LLFace::RIGGED);
	}
	pVVol->markForUpdate(TRUE);
}
void LLDrawable::initClass()
{
}
void LLDrawable::destroy()
{
	if (gDebugGL)
	{
		gPipeline.checkReferences(this);
	}
	if (isDead())
	{
		sNumZombieDrawables--;
	}
	if (LLSpatialGroup::sNoDelete)
	{
		LL_ERRS() << "Illegal deletion of LLDrawable!" << LL_ENDL;
	}
	std::for_each(mFaces.begin(), mFaces.end(), DeletePointer());
	mFaces.clear();
}
void LLDrawable::markDead()
{
	if (isDead())
	{
		LL_WARNS() << "Warning!  Marking dead multiple times!" << LL_ENDL;
		return;
	}
	setState(DEAD);
	if (mSpatialBridge)
	{
		mSpatialBridge->markDead();
		mSpatialBridge = NULL;
	}
	sNumZombieDrawables++;
	cleanupReferences();
}
LLVOVolume* LLDrawable::getVOVolume() const
{
	LLViewerObject* objectp = mVObjp;
	if ( !isDead() && objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
	{
		return ((LLVOVolume*)objectp);
	}
	else
	{
		return NULL;
	}
}
const LLMatrix4a& LLDrawable::getRenderMatrix() const
{
	return isRoot() ? getWorldMatrix() : getParent()->getWorldMatrix();
}
BOOL LLDrawable::isLight() const
{
	LLViewerObject* objectp = mVObjp;
	if ( objectp && (objectp->getPCode() == LL_PCODE_VOLUME) && !isDead())
	{
		return ((LLVOVolume*)objectp)->getIsLight();
	}
	else
	{
		return FALSE;
	}
}
static LLTrace::BlockTimerStatHandle FTM_CLEANUP_DRAWABLE("Cleanup Drawable");
static LLTrace::BlockTimerStatHandle FTM_DEREF_DRAWABLE("Deref");
static LLTrace::BlockTimerStatHandle FTM_DELETE_FACES("Faces");
void LLDrawable::cleanupReferences()
{
	LL_RECORD_BLOCK_TIME(FTM_CLEANUP_DRAWABLE);
	{
		LL_RECORD_BLOCK_TIME(FTM_DELETE_FACES);
		std::for_each(mFaces.begin(), mFaces.end(), DeletePointer());
		mFaces.clear();
	}
	gObjectList.removeDrawable(this);
	gPipeline.unlinkDrawable(this);
	removeFromOctree();
	{
		LL_RECORD_BLOCK_TIME(FTM_DEREF_DRAWABLE);
		mVObjp = NULL;
		mParent = NULL;
	}
}
void LLDrawable::removeFromOctree()
{
	if(!mEntry)
	{
		return;
	}
	mEntry->removeData(this);
	mEntry = NULL;
}
void LLDrawable::cleanupDeadDrawables()
{
	sDeadList.clear();
}
S32 LLDrawable::findReferences(LLDrawable *drawablep)
{
	S32 count = 0;
	if (mParent == drawablep)
	{
		LL_INFOS() << this << ": parent reference" << LL_ENDL;
		count++;
	}
	return count;
}
static LLTrace::BlockTimerStatHandle FTM_ALLOCATE_FACE("Allocate Face", true);
LLFace*	LLDrawable::addFace(LLFacePool *poolp, LLViewerTexture *texturep)
{
	LLFace *face;
	{
		LL_RECORD_BLOCK_TIME(FTM_ALLOCATE_FACE);
		face = new LLFace(this, mVObjp);
	}
	if (!face) LL_ERRS() << "Allocating new Face: " << mFaces.size() << LL_ENDL;
	if (face)
	{
		mFaces.push_back(face);
		if (poolp)
		{
			face->setPool(poolp, texturep);
		}
		if (isState(UNLIT))
		{
			face->setState(LLFace::FULLBRIGHT);
		}
	}
	return face;
}
LLFace*	LLDrawable::addFace(const LLTextureEntry *te, LLViewerTexture *texturep)
{
	LLFace *face;
	{
		LL_RECORD_BLOCK_TIME(FTM_ALLOCATE_FACE);
		face = new LLFace(this, mVObjp);
	}
	face->setTEOffset(mFaces.size());
	face->setTexture(texturep);
	face->setPoolType(gPipeline.getPoolTypeFromTE(te, texturep));
	mFaces.push_back(face);
	if (isState(UNLIT))
	{
		face->setState(LLFace::FULLBRIGHT);
	}
	return face;
}
LLFace*	LLDrawable::addFace(const LLTextureEntry *te, LLViewerTexture *texturep, LLViewerTexture *normalp)
{
	LLFace *face;
	face = new LLFace(this, mVObjp);
	face->setTEOffset(mFaces.size());
	face->setTexture(texturep);
	face->setNormalMap(normalp);
	face->setPoolType(gPipeline.getPoolTypeFromTE(te, texturep));
	mFaces.push_back(face);
	if (isState(UNLIT))
	{
		face->setState(LLFace::FULLBRIGHT);
	}
	return face;
}
LLFace*	LLDrawable::addFace(const LLTextureEntry *te, LLViewerTexture *texturep, LLViewerTexture *normalp, LLViewerTexture *specularp)
{
	LLFace *face;
	face = new LLFace(this, mVObjp);
	face->setTEOffset(mFaces.size());
	face->setTexture(texturep);
	face->setNormalMap(normalp);
	face->setSpecularMap(specularp);
	face->setPoolType(gPipeline.getPoolTypeFromTE(te, texturep));
	mFaces.push_back(face);
	if (isState(UNLIT))
	{
		face->setState(LLFace::FULLBRIGHT);
	}
	return face;
}
void LLDrawable::setNumFaces(const S32 newFaces, LLFacePool *poolp, LLViewerTexture *texturep)
{
	if (newFaces == (S32)mFaces.size())
	{
		return;
	}
	else if (newFaces < (S32)mFaces.size())
	{
		std::for_each(mFaces.begin() + newFaces, mFaces.end(), DeletePointer());
		mFaces.erase(mFaces.begin() + newFaces, mFaces.end());
	}
	else
	{
		mFaces.reserve(newFaces);
		for (int i = mFaces.size(); i<newFaces; i++)
		{
			addFace(poolp, texturep);
		}
	}
	llassert_always(mFaces.size() == newFaces);
}
void LLDrawable::setNumFacesFast(const S32 newFaces, LLFacePool *poolp, LLViewerTexture *texturep)
{
	if (newFaces <= (S32)mFaces.size() && newFaces >= (S32)mFaces.size()/2)
	{
		return;
	}
	else if (newFaces < (S32)mFaces.size())
	{
		std::for_each(mFaces.begin() + newFaces, mFaces.end(), DeletePointer());
		mFaces.erase(mFaces.begin() + newFaces, mFaces.end());
	}
	else
	{
		mFaces.reserve(newFaces);
		for (int i = mFaces.size(); i<newFaces; i++)
		{
			addFace(poolp, texturep);
		}
	}
	llassert_always(mFaces.size() == newFaces) ;
}
void LLDrawable::mergeFaces(LLDrawable* src)
{
	U32 face_count = mFaces.size() + src->mFaces.size();
	mFaces.reserve(face_count);
	for (U32 i = 0; i < src->mFaces.size(); i++)
	{
		LLFace* facep = src->mFaces[i];
		facep->setDrawable(this);
		mFaces.push_back(facep);
	}
	src->mFaces.clear();
}
void LLDrawable::deleteFaces(S32 offset, S32 count)
{
	face_list_t::iterator face_begin = mFaces.begin() + offset;
	face_list_t::iterator face_end = face_begin + count;
	std::for_each(face_begin, face_end, DeletePointer());
	mFaces.erase(face_begin, face_end);
}
void LLDrawable::update()
{
	LL_ERRS() << "Shouldn't be called!" << LL_ENDL;
}
void LLDrawable::updateMaterial()
{
}
void LLDrawable::makeActive()
{
#if !LL_RELEASE_FOR_DOWNLOAD
	if (mVObjp.notNull())
	{
		U32 pcode = mVObjp->getPCode();
		if (pcode == LLViewerObject::LL_VO_WATER ||
			pcode == LLViewerObject::LL_VO_VOID_WATER ||
			pcode == LLViewerObject::LL_VO_SURFACE_PATCH ||
			pcode == LLViewerObject::LL_VO_PART_GROUP ||
			pcode == LLViewerObject::LL_VO_HUD_PART_GROUP ||
#if ENABLE_CLASSIC_CLOUDS
			pcode == LLViewerObject::LL_VO_CLOUDS ||
#endif
			pcode == LLViewerObject::LL_VO_GROUND ||
			pcode == LLViewerObject::LL_VO_SKY)
		{
			LL_ERRS() << "Static viewer object has active drawable!" << LL_ENDL;
		}
	}
#endif
	if (!isState(ACTIVE))
	{
		setState(ACTIVE);
		if (!isRoot() && !mParent->isActive())
		{
			mParent->makeActive();
			mParent->setState(LLDrawable::ACTIVE_CHILD);
		}
		llassert_always(mVObjp);
		LLViewerObject::const_child_list_t& child_list = mVObjp->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			LLDrawable* drawable = child->mDrawable;
			if (drawable)
			{
				drawable->makeActive();
			}
		}
		if (mVObjp->getPCode() == LL_PCODE_VOLUME)
		{
			gPipeline.markRebuild(this, LLDrawable::REBUILD_VOLUME, TRUE);
		}
		updatePartition();
	}
	else if (!isRoot() && !mParent->isActive())
	{
		mParent->makeActive();
		mParent->setState(LLDrawable::ACTIVE_CHILD);
	}
	llassert(isAvatar() || isRoot() || mParent->isActive());
}
void LLDrawable::makeStatic(BOOL warning_enabled)
{
	if (isState(ACTIVE) &&
		!isState(ACTIVE_CHILD) &&
		!mVObjp->isAttachment() &&
		!mVObjp->isFlexible() &&
		!mVObjp->isAnimatedObject())
	{
		clearState(ACTIVE | ANIMATED_CHILD);
		llassert(mParent.isNull() || !mParent->isActive() || !warning_enabled);
		LLViewerObject::const_child_list_t& child_list = mVObjp->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			LLDrawable* child_drawable = child->mDrawable;
			if (child_drawable)
			{
				if (child_drawable->getParent() != this)
				{
					LL_WARNS() << "Child drawable has unknown parent." << LL_ENDL;
				}
				child_drawable->makeStatic(warning_enabled);
			}
		}
		if (mVObjp->getPCode() == LL_PCODE_VOLUME)
		{
			gPipeline.markRebuild(this, LLDrawable::REBUILD_VOLUME, TRUE);
		}
		if (mSpatialBridge)
		{
			mSpatialBridge->markDead();
			setSpatialBridge(NULL);
		}
		updatePartition();
	}
}
F32 LLDrawable::updateXform(BOOL undamped)
{
	BOOL damped = !undamped;
	LLVector3 old_pos(mXform.getPosition());
	LLVector3 target_pos;
	if (mXform.isRoot())
	{
		target_pos = mVObjp->getPositionAgent();
	}
	else
	{
		target_pos = mVObjp->getPosition();
	}
	LLQuaternion old_rot(mXform.getRotation());
	LLQuaternion target_rot = mVObjp->getRotation();
	LLVector3 target_scale = mVObjp->getScale();
	LLVector3 old_scale = mCurrentScale;
	F32 dist_squared = 0.f;
	F32 camdist2 = (mDistanceWRTCamera * mDistanceWRTCamera);
	if (damped && isVisible())
	{
		F32 lerp_amt = llclamp(LLSmoothInterpolation::getInterpolant(OBJECT_DAMPING_TIME_CONSTANT), 0.f, 1.f);
		LLVector3 new_pos = lerp(old_pos, target_pos, lerp_amt);
		dist_squared = dist_vec_squared(new_pos, target_pos);
		LLQuaternion new_rot = nlerp(lerp_amt, old_rot, target_rot);
		dist_squared += (1.f - dot(new_rot, target_rot)) * 10.f;
		LLVector3 new_scale = lerp(old_scale, target_scale, lerp_amt);
		dist_squared += dist_vec_squared(new_scale, target_scale);
		if ((dist_squared >= MIN_INTERPOLATE_DISTANCE_SQUARED * camdist2) &&
			(dist_squared <= MAX_INTERPOLATE_DISTANCE_SQUARED))
		{
			target_pos = new_pos;
			target_rot = new_rot;
			target_scale = new_scale;
		}
		else if (mVObjp->getAngularVelocity().isExactlyZero())
		{
			dist_squared = 0.0f;
			mCurrentScale = target_scale;
			if (getVOVolume() && !isRoot())
			{
				gPipeline.markRebuild(this, LLDrawable::REBUILD_POSITION, TRUE);
			}
		}
	}
	else
	{
	}
	LLVector3 vec = mCurrentScale-target_scale;
	mCurrentScale = target_scale;
	if (vec*vec > MIN_INTERPOLATE_DISTANCE_SQUARED)
	{
		gPipeline.markRebuild(this, LLDrawable::REBUILD_POSITION, TRUE);
	}
	else if (!isRoot() &&
		 (!mVObjp->getAngularVelocity().isExactlyZero() ||
			dist_squared > 0.f))
	{
		dist_squared = 1.f;
		if (!isState(LLDrawable::ANIMATED_CHILD))
		{
			setState(LLDrawable::ANIMATED_CHILD);
			gPipeline.markRebuild(this, LLDrawable::REBUILD_ALL, TRUE);
			mVObjp->dirtySpatialGroup();
		}
	}
	else if (!isRoot() &&
			((dist_vec_squared(old_pos, target_pos) > 0.f)
			|| (1.f - dot(old_rot, target_rot)) > 0.f))
	{
		gPipeline.markRebuild(this, LLDrawable::REBUILD_POSITION, TRUE);
	}
	else if (!getVOVolume() && !isAvatar())
	{
		movePartition();
	}
	mXform.setPosition(target_pos);
	mXform.setRotation(target_rot);
	mXform.setScale(LLVector3(1,1,1));
	mXform.updateMatrix();
	if (isRoot() && mVObjp->isAnimatedObject() && mVObjp->getControlAvatar())
	{
		mVObjp->getControlAvatar()->matchVolumeTransform();
	}
	if (mSpatialBridge)
	{
		gPipeline.markMoved(mSpatialBridge, FALSE);
	}
	return dist_squared;
}
void LLDrawable::setRadius(F32 radius)
{
	if (mRadius != radius)
	{
		mRadius = radius;
	}
}
void LLDrawable::moveUpdatePipeline(BOOL moved)
{
	if (moved)
	{
		makeActive();
	}
	for (S32 i = 0; i < getNumFaces(); i++)
	{
		LLFace* face = getFace(i);
		if (face)
		{
			face->updateCenterAgent();
		}
	}
}
void LLDrawable::movePartition()
{
	LLSpatialPartition* part = getSpatialPartition();
	if (part)
	{
		part->move(this, getSpatialGroup());
	}
}
BOOL LLDrawable::updateMove()
{
	if (isDead())
	{
		LL_WARNS() << "Update move on dead drawable!" << LL_ENDL;
		return TRUE;
	}
	if (mVObjp.isNull())
	{
		return FALSE;
	}
	makeActive();
	return isState(MOVE_UNDAMPED) ? updateMoveUndamped() : updateMoveDamped();
}
BOOL LLDrawable::updateMoveUndamped()
{
	F32 dist_squared = updateXform(TRUE);
	mGeneration++;
	if (!isState(LLDrawable::INVISIBLE))
	{
		BOOL moved = (dist_squared > 0.001f && dist_squared < 255.99f);
		moveUpdatePipeline(moved);
		mVObjp->updateText();
	}
	mVObjp->clearChanged(LLXform::MOVED);
	return TRUE;
}
void LLDrawable::updatePartition()
{
	if (!getVOVolume())
	{
		movePartition();
	}
	else if (mSpatialBridge)
	{
		gPipeline.markMoved(mSpatialBridge, FALSE);
	}
	else
	{
		gPipeline.markRebuild(this, LLDrawable::REBUILD_POSITION, TRUE);
	}
}
BOOL LLDrawable::updateMoveDamped()
{
	F32 dist_squared = updateXform(FALSE);
	mGeneration++;
	if (!isState(LLDrawable::INVISIBLE))
	{
		BOOL moved = (dist_squared > 0.001f && dist_squared < 128.0f);
		moveUpdatePipeline(moved);
		mVObjp->updateText();
	}
	BOOL done_moving = (dist_squared == 0.0f) ? TRUE : FALSE;
	if (done_moving)
	{
		mVObjp->clearChanged(LLXform::MOVED);
	}
	return done_moving;
}
void LLDrawable::updateDistance(LLCamera& camera, bool force_update)
{
	if (LLViewerCamera::sCurCameraID != LLViewerCamera::CAMERA_WORLD)
	{
		LL_WARNS() << "Attempted to update distance for non-world camera." << LL_ENDL;
		return;
	}
	if (gShiftFrame)
	{
		return;
	}
	LLVector3 pos;
	{
		LLVOVolume* volume = getVOVolume();
		if (volume)
		{
			if (getGroup())
			{
				pos.set(getPositionGroup().getF32ptr());
			}
			else
			{
				pos = getPositionAgent();
			}
			if (isState(LLDrawable::HAS_ALPHA))
			{
				for (S32 i = 0; i < getNumFaces(); i++)
				{
					LLFace* facep = getFace(i);
					if (facep &&
						(force_update || facep->getPoolType() == LLDrawPool::POOL_ALPHA))
					{
						LLVector4a box;
						box.setSub(facep->mExtents[1], facep->mExtents[0]);
						box.mul(0.25f);
						LLVector3 v = (facep->mCenterLocal-camera.getOrigin());
						const LLVector3& at = camera.getAtAxis();
						for (U32 j = 0; j < 3; j++)
						{
							v.mV[j] -= box[j] * at.mV[j];
						}
						facep->mDistance = v * camera.getAtAxis();
					}
				}
			}
			if (volume->getAvatar())
			{
				const LLVector3* av_box = volume->getAvatar()->getLastAnimExtents();
				LLVector3d cam_pos = gAgent.getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin());
				LLVector3 cam_region_pos = LLVector3(cam_pos - volume->getRegion()->getOriginGlobal());
				LLVector3 cam_to_box_offset = point_to_box_offset(cam_region_pos, av_box);
				mDistanceWRTCamera = llmax(0.01f, ll_round(cam_to_box_offset.magVec(), 0.01f));
				LL_DEBUGS("DynamicBox") << volume->getAvatar()->getFullname()
										<< " pos (ignored) " << pos
										<< " cam pos " << cam_pos
										<< " cam region pos " << cam_region_pos
										<< " box " << av_box[0] << "," << av_box[1]
										<< " -> dist " << mDistanceWRTCamera
										<< LL_ENDL;
				mVObjp->updateLOD();
				return;
			}
		}
		else
		{
			pos = LLVector3(getPositionGroup().getF32ptr());
		}
		pos -= camera.getOrigin();
		mDistanceWRTCamera = ll_round(pos.magVec(), 0.01f);
		mVObjp->updateLOD();
	}
}
void LLDrawable::updateTexture()
{
	if (isDead())
	{
		LL_WARNS() << "Dead drawable updating texture!" << LL_ENDL;
		return;
	}
	if (getNumFaces() != mVObjp->getNumTEs())
	{
		return;
	}
	if (getVOVolume())
	{
		gPipeline.markRebuild(this, LLDrawable::REBUILD_MATERIAL, TRUE);
	}
}
BOOL LLDrawable::updateGeometry(BOOL priority)
{
	llassert(mVObjp.notNull());
	BOOL res = mVObjp->updateGeometry(this);
	return res;
}
void LLDrawable::shiftPos(const LLVector4a &shift_vector)
{
	if (isDead())
	{
		LL_WARNS() << "Shifting dead drawable" << LL_ENDL;
		return;
	}
	if (mParent)
	{
		mXform.setPosition(mVObjp->getPosition());
	}
	else
	{
		mXform.setPosition(mVObjp->getPositionAgent());
	}
	mXform.updateMatrix();
	if (isStatic())
	{
		LLVOVolume* volume = getVOVolume();
		bool rebuild = (!volume &&
						getRenderType() != LLPipeline::RENDER_TYPE_TREE &&
						getRenderType() != LLPipeline::RENDER_TYPE_TERRAIN &&
						getRenderType() != LLPipeline::RENDER_TYPE_SKY &&
						getRenderType() != LLPipeline::RENDER_TYPE_GROUND);
		if (rebuild)
		{
			gPipeline.markRebuild(this, LLDrawable::REBUILD_ALL, TRUE);
		}
		for (S32 i = 0; i < getNumFaces(); i++)
		{
			LLFace *facep = getFace(i);
			if (facep)
			{
				facep->mCenterAgent += LLVector3(shift_vector.getF32ptr());
				facep->mExtents[0].add(shift_vector);
				facep->mExtents[1].add(shift_vector);
				if (rebuild && facep->hasGeometry())
				{
					facep->clearVertexBuffer();
				}
			}
		}
		shift(shift_vector);
	}
	else if (mSpatialBridge)
	{
		mSpatialBridge->shiftPos(shift_vector);
	}
	else if (isAvatar())
	{
		shift(shift_vector);
	}
	mVObjp->onShift(shift_vector);
}
const LLVector3& LLDrawable::getBounds(LLVector3& min, LLVector3& max) const
{
	mXform.getMinMax(min,max);
	return mXform.getPositionW();
}
void LLDrawable::updateSpatialExtents()
{
	if (mVObjp)
	{
		const LLVector4a* exts = getSpatialExtents();
		LLVector4a extents[2] = { exts[0], exts[1] };
		mVObjp->updateSpatialExtents(extents[0], extents[1]);
		setSpatialExtents(extents[0], extents[1]);
	}
	updateBinRadius();
	if (mSpatialBridge.notNull())
	{
		getGroupPosition().splat(0.f);
	}
}
void LLDrawable::updateBinRadius()
{
	if (mVObjp.notNull())
	{
		setBinRadius(llmin(mVObjp->getBinRadius(), 256.f));
	}
	else
	{
		setBinRadius(llmin(getRadius()*4.f, 256.f));
	}
}
void LLDrawable::updateSpecialHoverCursor(BOOL enabled)
{
}
F32 LLDrawable::getVisibilityRadius() const
{
	if (isDead())
	{
		return 0.f;
	}
	else if (isLight())
	{
		const LLVOVolume *vov = getVOVolume();
		if (vov)
		{
			return llmax(getRadius(), vov->getLightRadius());
		} else {
		}
	}
	return getRadius();
}
void LLDrawable::updateUVMinMax()
{
}
bool LLDrawable::isVisible() const
{
	if (LLViewerOctreeEntryData::isVisible())
{
		return true;
}
{
		LLViewerOctreeGroup* group = mEntry->getGroup();
		if (group && group->isVisible())
		{
			LLViewerOctreeEntryData::setVisible();
			return true;
		}
	}
	return false;
}
bool LLDrawable::isRecentlyVisible() const
{
	bool vis = LLViewerOctreeEntryData::isRecentlyVisible();
	if(!vis)
	{
		const U32 MIN_VIS_FRAME_RANGE = 2 ;
		vis = (sCurVisible - getVisible() < MIN_VIS_FRAME_RANGE);
	}
	return vis ;
}
void LLDrawable::setGroup(LLViewerOctreeGroup *groupp)
{
	LLSpatialGroup* cur_groupp = (LLSpatialGroup*)getGroup();
	llassert(!groupp || (LLSpatialGroup*)groupp->hasElement(this));
	if (cur_groupp != groupp && getVOVolume())
	{
		for (S32 i = 0; i < getNumFaces(); ++i)
		{
			LLFace* facep = getFace(i);
			if (facep)
			{
				facep->clearVertexBuffer();
			}
		}
	}
	LLViewerOctreeEntryData::setGroup(groupp);
}
LLSpatialPartition* LLDrawable::getSpatialPartition()
{
	LLSpatialPartition* retval = NULL;
	if (!mVObjp ||
		!getVOVolume() ||
		isStatic())
	{
		retval = gPipeline.getSpatialPartition((LLViewerObject*) mVObjp);
	}
	else if (isRoot())
	{
		if (mSpatialBridge && (mSpatialBridge->asPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD) != mVObjp->isHUDAttachment())
		{
			if (mVObjp->isHUDAttachment() || hud_teleport_logging_active())
			{
				LL_INFOS("HUDTeleport") << "getSpatialPartition killing mismatched bridge"
					<< " hud=" << (mVObjp->isHUDAttachment() ? 1 : 0)
					<< " br_part=" << mSpatialBridge->asPartition()->mPartitionType
					<< " id=" << mVObjp->getID()
					<< LL_ENDL;
			}
			mSpatialBridge->markDead();
			setSpatialBridge(NULL);
		}
		if (!mSpatialBridge)
		{
			if (mVObjp->isHUDAttachment())
			{
				LL_INFOS("HUDTeleport") << "getSpatialPartition creating LLHUDBridge id=" << mVObjp->getID() << LL_ENDL;
				setSpatialBridge(new LLHUDBridge(this, getRegion()));
			}
			else if (mVObjp->isAttachment())
			{
				setSpatialBridge(new LLAttachmentBridge(this, getRegion()));
			}
			else
			{
				setSpatialBridge(new LLVolumeBridge(this, getRegion()));
			}
		}
		return mSpatialBridge->asPartition();
	}
	else
	{
		retval = getParent()->getSpatialPartition();
	}
	if (retval && mSpatialBridge.notNull())
	{
		mSpatialBridge->markDead();
		setSpatialBridge(NULL);
	}
	return retval;
}
LLSpatialBridge::LLSpatialBridge(LLDrawable* root, BOOL render_by_group, U32 data_mask, LLViewerRegion* regionp) :
	LLDrawable(root->getVObj()),
	LLSpatialPartition(data_mask, render_by_group, GL_STREAM_DRAW_ARB, regionp)
{
	mBridge = this;
	mDrawable = root;
	root->setSpatialBridge(this);
	mRenderType = mDrawable->mRenderType;
	mDrawableType = mDrawable->mRenderType;
	mPartitionType = LLViewerRegion::PARTITION_VOLUME;
	mOctree->balance();
	llassert(mDrawable);
	llassert(mDrawable->getRegion());
	LLSpatialPartition *part = mDrawable->getRegion()->getSpatialPartition(mPartitionType);
	llassert(part);
	if (part)
	{
		part->put(this);
	}
}
LLSpatialBridge::~LLSpatialBridge()
{
	if(mEntry)
	{
	LLSpatialGroup* group = getSpatialGroup();
	if (group)
	{
		group->getSpatialPartition()->remove(this, group);
	}
	}
	destroyTree();
}
void LLSpatialBridge::destroyTree()
{
	delete mOctree;
	mOctree = NULL;
}
void LLSpatialBridge::updateSpatialExtents()
{
	LLSpatialGroup* root = (LLSpatialGroup*) mOctree->getListener(0);
	{
		LL_RECORD_BLOCK_TIME(FTM_CULL_REBOUND);
		root->rebound();
	}
	const LLVector4a* root_bounds = root->getBounds();
	LLVector4a offset;
	LLVector4a size = root_bounds[1];
	const LLMatrix4a& mat = mDrawable->getXform()->getWorldMatrix();
	LLVector4a t;
	t.splat(0.f);
	LLVector4a center;
	mat.affineTransform(t, center);
	mat.rotate(root_bounds[0], offset);
	center.add(offset);
	LLVector4a v[4];
	mat.rotate(size,v[0]);
	LLVector4a scale;
	scale.set(-1.f, -1.f, 1.f);
	scale.mul(size);
	mat.rotate(scale, v[1]);
	scale.set(1.f, -1.f, -1.f);
	scale.mul(size);
	mat.rotate(scale, v[2]);
	scale.set(-1.f, 1.f, -1.f);
	scale.mul(size);
	mat.rotate(scale, v[3]);
	LLVector4a newMin;
	LLVector4a newMax;
	newMin = newMax = center;
	for (U32 i = 0; i < 4; i++)
	{
		LLVector4a delta;
		delta.setAbs(v[i]);
		LLVector4a min;
		min.setSub(center, delta);
		LLVector4a max;
		max.setAdd(center, delta);
		newMin.setMin(newMin, min);
		newMax.setMax(newMax, max);
	}
	setSpatialExtents(newMin, newMax);
	LLVector4a diagonal;
	diagonal.setSub(newMax, newMin);
	mRadius = diagonal.getLength3().getF32() * 0.5f;
	LLVector4a& pos = getGroupPosition();
	pos.setAdd(newMin,newMax);
	pos.mul(0.5f);
	updateBinRadius();
}
void LLSpatialBridge::updateBinRadius()
{
	setBinRadius(llmin( mOctree->getSize()[0]*0.5f, 256.f));
}
LLCamera LLSpatialBridge::transformCamera(LLCamera& camera)
{
	LLCamera ret = camera;
	LLXformMatrix* mat = mDrawable->getXform();
	const LLVector4a& center = mat->getWorldMatrix().getRow<3>();
	LLQuaternion2 invRot;
	invRot.setConjugate( LLQuaternion2(mat->getRotation()) );
	LLVector4a delta;
	delta.load3(ret.getOrigin().mV);
	delta.sub(center);
	LLVector4a lookAt;
	lookAt.load3(ret.getAtAxis().mV);
	LLVector4a up_axis;
	up_axis.load3(ret.getUpAxis().mV);
	LLVector4a left_axis;
	left_axis.load3(ret.getLeftAxis().mV);
	delta.setRotated(invRot, delta);
	lookAt.setRotated(invRot, lookAt);
	up_axis.setRotated(invRot, up_axis);
	left_axis.setRotated(invRot, left_axis);
	if (!delta.isFinite3())
	{
		delta.clear();
	}
	ret.setOrigin(LLVector3(delta.getF32ptr()));
	ret.setAxes(LLVector3(lookAt.getF32ptr()), LLVector3(left_axis.getF32ptr()), LLVector3(up_axis.getF32ptr()));
	return ret;
}
void LLDrawable::setVisible(LLCamera& camera, std::vector<LLDrawable*>* results, BOOL for_select)
{
	LLViewerOctreeEntryData::setVisible();
#if 0 && !LL_RELEASE_FOR_DOWNLOAD
	if (getVOVolume())
	{
		if (!isRoot())
		{
			if (isActive() && !mParent->isActive())
			{
				LL_ERRS() << "Active drawable has static parent!" << LL_ENDL;
			}
			if (isStatic() && !mParent->isStatic())
			{
				LL_ERRS() << "Static drawable has active parent!" << LL_ENDL;
			}
			if (mSpatialBridge)
			{
				LL_ERRS() << "Child drawable has spatial bridge!" << LL_ENDL;
			}
		}
		else if (isActive() && !mSpatialBridge)
		{
			LL_ERRS() << "Active root drawable has no spatial bridge!" << LL_ENDL;
		}
		else if (isStatic() && mSpatialBridge.notNull())
		{
			LL_ERRS() << "Static drawable has spatial bridge!" << LL_ENDL;
		}
	}
#endif
}
class LLOctreeMarkNotCulled: public OctreeTraveler
{
public:
	LLCamera* mCamera;
	LLOctreeMarkNotCulled(LLCamera* camera_in) : mCamera(camera_in) { }
	virtual void traverse(const OctreeNode* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		group->setVisible();
		OctreeTraveler::traverse(node);
	}
	void visit(const OctreeNode* branch)
	{
		gPipeline.markNotCulled((LLSpatialGroup*) branch->getListener(0), *mCamera);
	}
};
void LLSpatialBridge::setVisible(LLCamera& camera_in, std::vector<LLDrawable*>* results, BOOL for_select)
{
	if (!gPipeline.hasRenderType(mDrawableType))
	{
		return;
	}
	LLViewerObject *vobj = mDrawable->getVObj();
	if (vobj && vobj->isAttachment() && !vobj->isHUDAttachment())
	{
		LLDrawable* av;
		LLDrawable* parent = mDrawable->getParent();
		if (parent)
		{
			LLViewerObject* objparent = parent->getVObj();
			av = objparent->mDrawable;
			LLSpatialGroup* group = av->getSpatialGroup();
			BOOL impostor = FALSE;
			BOOL loaded = FALSE;
			if (objparent->isAvatar())
			{
				LLVOAvatar* avatarp = (LLVOAvatar*) objparent;
				if (avatarp->isVisible())
				{
					impostor = objparent->isAvatar() && ((LLVOAvatar*) objparent)->isImpostor();
					loaded   = objparent->isAvatar() && ((LLVOAvatar*) objparent)->isFullyLoaded();
				}
				else
				{
					return;
				}
			}
			if (!group ||
				LLDrawable::getCurrentFrame() - av->getVisible() > 1 ||
				impostor ||
				!loaded)
			{
				return;
			}
		}
		else
		{
			static const LLCachedControl<bool> draw_orphans("ShyotlDrawOrphanAttachments",false);
			if(!draw_orphans)
				return;
		}
	}
	LLSpatialGroup* group = (LLSpatialGroup*) mOctree->getListener(0);
	group->rebound();
	LLVector4a center;
	const LLVector4a* exts = getSpatialExtents();
	center.setAdd(exts[0], exts[1]);
	center.mul(0.5f);
	LLVector4a size;
	size.setSub(exts[1], exts[0]);
	size.mul(0.5f);
	if ((LLPipeline::sShadowRender && camera_in.AABBInFrustum(center, size)) ||
		LLPipeline::sImpostorRender ||
		(camera_in.AABBInFrustumNoFarClip(center, size) &&
		AABBSphereIntersect(exts[0], exts[1], camera_in.getOrigin(), camera_in.mFrustumCornerDist)))
	{
		if (!LLPipeline::sImpostorRender &&
			!LLPipeline::sShadowRender &&
			LLPipeline::calcPixelArea(center, size, camera_in) < FORCE_INVISIBLE_AREA)
		{
			return;
		}
		LLDrawable::setVisible(camera_in);
		if (for_select)
		{
			results->push_back(mDrawable);
			if (mDrawable->getVObj())
			{
				LLViewerObject::const_child_list_t& child_list = mDrawable->getVObj()->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); iter++)
				{
					LLViewerObject* child = *iter;
					LLDrawable* drawable = child->mDrawable;
					results->push_back(drawable);
				}
			}
		}
		else
		{
			LLCamera trans_camera = transformCamera(camera_in);
			LLOctreeMarkNotCulled culler(&trans_camera);
			culler.traverse(mOctree);
		}
	}
}
void LLSpatialBridge::updateDistance(LLCamera& camera_in, bool force_update)
{
	if (mDrawable == NULL)
	{
		markDead();
		return;
	}
	if (gShiftFrame)
	{
		return;
	}
	if (mDrawable->getVObj())
	{
		if (mDrawable->getVObj()->isAttachment())
		{
			LLDrawable* parent = mDrawable->getParent();
			if (parent)
			{
				LLViewerObject *obj = parent->getVObj();
				if (obj && obj->isAvatar() && ((LLVOAvatar*)obj)->isImpostor())
				{
					return;
				}
			}
			else if (!mDrawable->getVObj()->isHUDAttachment())
			{
				// HUD attachments live on the screen joint and can briefly
				// lose a drawable parent across a teleport. Skipping them here
				// makes them vanish until a refresh rebuilds the drawable.
				static const LLCachedControl<bool> draw_orphans("ShyotlDrawOrphanAttachments",false);
				if(!draw_orphans)
					return;
			}
			else if (hud_teleport_logging_active())
			{
				static LLFrameTimer sHUDOrphanParentLogTimer;
				if (sHUDOrphanParentLogTimer.getElapsedTimeF32() >= 1.f)
				{
					sHUDOrphanParentLogTimer.reset();
					LL_INFOS("HUDTeleport") << "updateDistance HUD with null parent id="
						<< mDrawable->getVObj()->getID() << LL_ENDL;
				}
			}
		}
		LLCamera camera = transformCamera(camera_in);
		mDrawable->updateDistance(camera, force_update);
		LLViewerObject::const_child_list_t& child_list = mDrawable->getVObj()->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			LLDrawable* drawable = child->mDrawable;
			if (!drawable)
			{
				continue;
			}
			if (!drawable->isAvatar())
			{
				drawable->updateDistance(camera, force_update);
			}
		}
	}
}
void LLSpatialBridge::makeActive()
{
	LL_ERRS() << "makeActive called on spatial bridge" << LL_ENDL;
}
void LLSpatialBridge::move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate)
{
	LLSpatialPartition::move(drawablep, curp, immediate);
	gPipeline.markMoved(this, FALSE);
}
BOOL LLSpatialBridge::updateMove()
{
	llassert_always(mDrawable);
	llassert_always(mDrawable->mVObjp);
	llassert_always(mDrawable->getRegion());
	LLSpatialPartition* part = mDrawable->getRegion()->getSpatialPartition(mPartitionType);
	llassert_always(part);
	mOctree->balance();
	if (part)
	{
		part->move(this, getSpatialGroup(), TRUE);
	}
	return TRUE;
}
void LLSpatialBridge::shiftPos(const LLVector4a& vec)
{
	LLDrawable::shift(vec);
}
void LLSpatialBridge::cleanupReferences()
{
	LLDrawable::cleanupReferences();
	if (mDrawable)
	{
		mDrawable->setGroup(NULL);
		if (mDrawable->getVObj())
		{
			LLViewerObject::const_child_list_t& child_list = mDrawable->getVObj()->getChildren();
			for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
				 iter != child_list.end(); iter++)
			{
				LLViewerObject* child = *iter;
				LLDrawable* drawable = child->mDrawable;
				if (drawable)
				{
					drawable->setGroup(NULL);
				}
			}
		}
		LLPointer<LLSpatialBridge> bridgep = mDrawable->getSpatialBridge();
		mDrawable->setSpatialBridge(nullptr);
		mDrawable = nullptr;
		bridgep = nullptr;
	}
}
const LLVector3	LLDrawable::getPositionAgent() const
{
	if (getVOVolume())
	{
		if (isActive())
		{
			if (!isRoot())
			{
				LLVector4a pos;
				pos.load3(mVObjp->getPosition().mV);
				getRenderMatrix().affineTransform(pos,pos);
				return LLVector3(pos.getF32ptr());
			}
			else
			{
				return LLVector3(getRenderMatrix().getRow<3>().getF32ptr());
			}
		}
		else
		{
			return mVObjp->getPositionAgent();
		}
	}
	else
	{
		return getWorldPosition();
	}
}
BOOL LLDrawable::isAnimating() const
{
	if (!getVObj())
	{
		return TRUE;
	}
	if (getScale() != mVObjp->getScale())
	{
		return TRUE;
	}
	if (mVObjp->getPCode() == LLViewerObject::LL_VO_PART_GROUP)
	{
		return TRUE;
	}
	if (mVObjp->getPCode() == LLViewerObject::LL_VO_HUD_PART_GROUP)
	{
		return TRUE;
	}
#if ENABLE_CLASSIC_CLOUDS
	if (mVObjp->getPCode() == LLViewerObject::LL_VO_CLOUDS)
	{
		return TRUE;
	}
#endif
	return FALSE;
}
void LLDrawable::updateFaceSize(S32 idx)
{
	if (mVObjp.notNull())
	{
		mVObjp->updateFaceSize(idx);
	}
}
LLBridgePartition::LLBridgePartition(LLViewerRegion* regionp)
: LLSpatialPartition(0, FALSE, 0, regionp)
{
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_BRIDGE;
	mLODPeriod = 16;
	mSlopRatio = 0.25f;
}
LLAttachmentPartition::LLAttachmentPartition(LLViewerRegion* regionp)
: LLBridgePartition(regionp)
{
	mDrawableType = LLPipeline::RENDER_TYPE_AVATAR;
}
LLHUDBridge::LLHUDBridge(LLDrawable* drawablep, LLViewerRegion* regionp)
: LLVolumeBridge(drawablep, regionp)
{
	mDrawableType = LLPipeline::RENDER_TYPE_HUD;
	mPartitionType = LLViewerRegion::PARTITION_HUD;
	mSlopRatio = 0.0f;
	LLViewerObject* obj = drawablep ? drawablep->getVObj() : NULL;
	LL_INFOS("HUDTeleport") << "LLHUDBridge ctor"
		<< " id=" << (obj ? obj->getID() : LLUUID::null)
		<< " region=" << (regionp ? regionp->getName() : std::string("null"))
		<< " region_ptr=" << (regionp ? 1 : 0)
		<< LL_ENDL;
}
F32 LLHUDBridge::calcPixelArea(LLSpatialGroup* group, LLCamera& camera)
{
	return 1024.f;
}
void LLHUDBridge::shiftPos(const LLVector4a& vec)
{
}
