/**
 * @file llcontrolavatar.cpp
 * @brief Implementation for special dummy avatar used to drive rigged meshes.
 *
 * $LicenseInfo:firstyear=2017&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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
#include "llcontrolavatar.h"
#include "llagent.h"
#include "llviewerobjectlist.h"
#include "pipeline.h"
#include "llanimationstates.h"
#include "llviewercontrol.h"
#include "llmeshrepository.h"
#include "llviewerregion.h"
#include "llskinningutil.h"
const F32 LLControlAvatar::MAX_LEGAL_OFFSET = 3.0f;
const F32 LLControlAvatar::MAX_LEGAL_SIZE = 64.0f;
boost::signals2::connection LLControlAvatar::sRegionChangedSlot;
LLControlAvatar::LLControlAvatar(const LLUUID& id, const LLPCode pcode, LLViewerRegion* regionp) :
    LLVOAvatar(id, pcode, regionp),
    mPlaying(false),
    mGlobalScale(1.0f),
	mRootVolp(NULL),
    mMarkedForDeath(false),
    mScaleConstraintFixup(1.0),
	mRegionChanged(false)
{
    mIsDummy = TRUE;
    mIsControlAvatar = true;
    mEnableDefaultMotions = false;
}
LLControlAvatar::~LLControlAvatar()
{
	llassert(!mRootVolp);
}
void LLControlAvatar::initInstance()
{
    LLVOAvatar::initInstance();
	createDrawable(&gPipeline);
	updateJointLODs();
	updateGeometry(mDrawable);
	hideSkirt();
    mInitFlags |= 1<<4;
}
void LLControlAvatar::getNewConstraintFixups(LLVector3& new_pos_fixup, F32& new_scale_fixup) const
{
    F32 max_legal_offset = MAX_LEGAL_OFFSET;
	static LLCachedControl<F32> animated_object_max_legal_offset(gSavedSettings, "AnimatedObjectsMaxLegalOffset");
	max_legal_offset = llmax(animated_object_max_legal_offset(),0.f);
    F32 max_legal_size = MAX_LEGAL_SIZE;
	static LLCachedControl<F32> animated_object_max_legal_size(gSavedSettings, "AnimatedObjectsMaxLegalSize");
	max_legal_size = llmax(animated_object_max_legal_size(), 1.f);
    new_pos_fixup = LLVector3();
    new_scale_fixup = 1.0f;
	LLVector3 vol_pos = mRootVolp->getRenderPosition();
    if (box_valid_and_non_zero(getLastAnimExtents()))
    {
        const LLVector3 *extents = getLastAnimExtents();
		LLVector3 unshift_extents[2];
		unshift_extents[0] = extents[0] - mPositionConstraintFixup;
		unshift_extents[1] = extents[1] - mPositionConstraintFixup;
        LLVector3 box_dims = extents[1]-extents[0];
        F32 box_size = llmax(box_dims[0],box_dims[1],box_dims[2]);
		if (!mRootVolp->isAttachment())
		{
			LLVector3 pos_box_offset = point_to_box_offset(vol_pos, unshift_extents);
			F32 offset_dist = pos_box_offset.length();
			if (offset_dist > max_legal_offset && offset_dist > 0.f)
			{
				F32 target_dist = (offset_dist - max_legal_offset);
				new_pos_fixup = (target_dist/offset_dist)*pos_box_offset;
			}
		}
		if (box_size/mScaleConstraintFixup > max_legal_size)
		{
			new_scale_fixup = mScaleConstraintFixup*max_legal_size/box_size;
		}
	}
}
void LLControlAvatar::matchVolumeTransform()
{
    if (mRootVolp)
    {
		LLVector3 new_pos_fixup;
		F32 new_scale_fixup;
		if (mRegionChanged)
		{
			new_scale_fixup = mScaleConstraintFixup;
			new_pos_fixup = mPositionConstraintFixup;
			mRegionChanged = false;
		}
		else
		{
			getNewConstraintFixups(new_pos_fixup, new_scale_fixup);
		}
		mPositionConstraintFixup = new_pos_fixup;
		mScaleConstraintFixup = new_scale_fixup;
		static LLCachedControl<F32> global_scale(gSavedSettings, "AnimatedObjectsGlobalScale", 1.f);
        if (mRootVolp->isAttachment())
        {
            LLVOAvatar *attached_av = mRootVolp->getAvatarAncestor();
            if (attached_av)
            {
                LLViewerJointAttachment *attach = attached_av->getTargetAttachmentPoint(mRootVolp);
                setPositionAgent(mRootVolp->getRenderPosition());
				attach->updateWorldPRSParent();
                LLVector3 joint_pos = attach->getWorldPosition();
                LLQuaternion joint_rot = attach->getWorldRotation();
                LLVector3 obj_pos = mRootVolp->mDrawable->getPosition();
                const LLQuaternion& obj_rot = mRootVolp->mDrawable->getRotation();
                obj_pos.rotVec(joint_rot);
                mRoot->setWorldPosition(obj_pos + joint_pos);
                mRoot->setWorldRotation(obj_rot * joint_rot);
                setRotation(mRoot->getRotation());
				setGlobalScale(global_scale * mScaleConstraintFixup);
            }
            else
            {
                LL_WARNS_ONCE() << "can't find attached av!" << LL_ENDL;
            }
        }
        else
        {
            LLVector3 vol_pos = mRootVolp->getRenderPosition();
            const LLQuaternion& obj_rot = mRootVolp->mDrawable ? mRootVolp->mDrawable->getRotation() : mRootVolp->getRotation();
            LLQuaternion bind_rot;
#define MATCH_BIND_SHAPE
#ifdef MATCH_BIND_SHAPE
	        const LLMeshSkinInfo* skin_info = mRootVolp->getSkinInfo();
			if (skin_info)
			{
                bind_rot = LLSkinningUtil::getUnscaledQuaternion(skin_info->mBindShapeMatrix);
			}
#endif
			setRotation(bind_rot*obj_rot);
            mRoot->setWorldRotation(bind_rot*obj_rot);
			setPositionAgent(vol_pos);
			mRoot->setPosition(vol_pos + mPositionConstraintFixup);
            setGlobalScale(global_scale * mScaleConstraintFixup);
        }
    }
}
void LLControlAvatar::setGlobalScale(F32 scale)
{
    if (scale <= 0.0f)
    {
        LL_WARNS() << "invalid global scale " << scale << LL_ENDL;
        return;
    }
    if (scale != mGlobalScale)
    {
        F32 adjust_scale = scale/mGlobalScale;
        LL_INFOS() << "scale " << scale << " adjustment " << adjust_scale << LL_ENDL;
        recursiveScaleJoint(mPelvisp,adjust_scale);
        mGlobalScale = scale;
    }
}
void LLControlAvatar::recursiveScaleJoint(LLJoint* joint, F32 factor)
{
    joint->setScale(factor * joint->getScale());
	for (auto child : joint->mChildren)
	{
		recursiveScaleJoint(child, factor);
	}
}
void LLControlAvatar::updateVolumeGeom()
{
	if (!mRootVolp->mDrawable)
		return;
	if (mRootVolp->mDrawable->isActive())
	{
		mRootVolp->mDrawable->makeStatic(FALSE);
	}
	mRootVolp->mDrawable->makeActive();
	gPipeline.markMoved(mRootVolp->mDrawable);
	gPipeline.markTextured(mRootVolp->mDrawable);
	mRootVolp->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
	LLViewerObject::const_child_list_t& child_list = mRootVolp->getChildren();
	for (const auto& iter : child_list)
	{
		LLViewerObject* childp = iter;
		if (childp && childp->mDrawable.notNull())
		{
			childp->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
			gPipeline.markTextured(childp->mDrawable);
			gPipeline.markMoved(childp->mDrawable);
        }
    }
    gPipeline.markRebuild(mRootVolp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
    mRootVolp->markForUpdate(TRUE);
    matchVolumeTransform();
}
LLControlAvatar *LLControlAvatar::createControlAvatar(LLVOVolume *obj)
{
	LLControlAvatar *cav = (LLControlAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR, gAgent.getRegion(), CO_FLAG_CONTROL_AVATAR);
	if (cav)
	{
		cav->mRootVolp = obj;
		cav->matchVolumeTransform();
	}
    return cav;
}
void LLControlAvatar::markForDeath()
{
    mMarkedForDeath = true;
    mRootVolp = NULL;
}
void LLControlAvatar::markDead()
{
	if (mRootVolp)
	{
		mRootVolp->unlinkControlAvatar();
		mRootVolp = nullptr;
	}
	LLVOAvatar::markDead();
}
void LLControlAvatar::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
    if (mMarkedForDeath)
    {
        markDead();
        mMarkedForDeath = false;
    }
    else
    {
        LLVOAvatar::idleUpdate(agent, world,time);
    }
}
BOOL LLControlAvatar::updateCharacter(LLAgent &agent)
{
    return LLVOAvatar::updateCharacter(agent);
}
void LLControlAvatar::updateDebugText()
{
    LLVOAvatar::updateDebugText();
}
void LLControlAvatar::getAnimatedVolumes(std::vector<LLVOVolume*>& volumes)
{
    if (!mRootVolp)
    {
        return;
    }
    volumes.push_back(mRootVolp);
	LLViewerObject::const_child_list_t& child_list = mRootVolp->getChildren();
	for (const auto& iter : child_list)
	{
		LLViewerObject* childp = iter;
        LLVOVolume *child_volp = childp ? childp->asVolume() : nullptr;
        if (child_volp && child_volp->isAnimatedObject())
        {
            volumes.push_back(child_volp);
        }
    }
}
void LLControlAvatar::updateAnimations()
{
    if (!mRootVolp)
    {
        LL_WARNS_ONCE("AnimatedObjectsNotify") << "No root vol" << LL_ENDL;
        return;
    }
    std::vector<LLVOVolume*> volumes;
    getAnimatedVolumes(volumes);
	std::map<LLUUID, S32> anims;
    for (auto vol_it = volumes.begin(); vol_it != volumes.end(); ++vol_it)
    {
        LLVOVolume *volp = *vol_it;
        auto& signaled_anim_map = LLObjectSignaledAnimationMap::instance().getMap();
        signaled_animation_map_t& signaled_animations = signaled_anim_map[volp->getID()];
        for (auto anim_it = signaled_animations.begin(), anim_it_end = signaled_animations.end();
             anim_it != anim_it_end;
             ++anim_it)
        {
            auto found_anim_it = anims.find(anim_it->first);
            if (found_anim_it != anims.end())
            {
                anims[anim_it->first] = llmax(found_anim_it->second, anim_it->second);
            }
            else
            {
                anims[anim_it->first] = anim_it->second;
            }
#if LL_DEBUG
            LL_DEBUGS("AnimatedObjectsNotify") << "found anim for vol " << volp->getID() << " anim " << anim_it->first << " root " << mRootVolp->getID() << LL_ENDL;
#endif
        }
    }
    if (!mPlaying)
    {
        mPlaying = true;
        {
            updateVolumeGeom();
            mRootVolp->recursiveMarkForUpdate(TRUE);
        }
    }
    mSignaledAnimations = anims;
    processAnimationStateChanges();
}
LLViewerObject* LLControlAvatar::lineSegmentIntersectRiggedAttachments(const LLVector4a& start, const LLVector4a& end,
									  S32 face,
									  BOOL pick_transparent,
									  BOOL pick_rigged,
									  S32* face_hit,
									  LLVector4a* intersection,
									  LLVector2* tex_coord,
									  LLVector4a* normal,
									  LLVector4a* tangent)
{
    if (!mRootVolp)
    {
        return NULL;
    }
	LLViewerObject* hit = NULL;
	if (lineSegmentBoundingBox(start, end))
	{
		LLVector4a local_end = end;
		LLVector4a local_intersection;
        if (mRootVolp->lineSegmentIntersect(start, local_end, face, pick_transparent, pick_rigged, face_hit, &local_intersection, tex_coord, normal, tangent))
        {
            local_end = local_intersection;
            if (intersection)
            {
                *intersection = local_intersection;
            }
            hit = mRootVolp;
        }
        else
        {
            std::vector<LLVOVolume*> volumes;
            getAnimatedVolumes(volumes);
            for (auto volp : volumes)
            {
                if (mRootVolp != volp && volp->lineSegmentIntersect(start, local_end, face, pick_transparent, pick_rigged, face_hit, &local_intersection, tex_coord, normal, tangent))
                {
                    local_end = local_intersection;
                    if (intersection)
                    {
                        *intersection = local_intersection;
                    }
                    hit = volp;
                    break;
                }
            }
        }
	}
	return hit;
}
std::string LLControlAvatar::getFullname() const
{
    if (mRootVolp)
    {
        return "AO_" + mRootVolp->getID().getString();
    }
    else
    {
        return "AO_no_root_vol";
    }
}
bool LLControlAvatar::shouldRenderRigged() const
{
    if (mRootVolp && mRootVolp->isAttachment())
    {
        LLVOAvatar *attached_av = mRootVolp->getAvatarAncestor();
        if (attached_av)
        {
            return attached_av->shouldRenderRigged();
        }
    }
    return true;
}
BOOL LLControlAvatar::isImpostor() const
{
    if (mRootVolp && mRootVolp->isAttachment())
    {
        LLVOAvatar *attached_av = mRootVolp->getAvatarAncestor();
		if (attached_av)
		{
			return attached_av->isImpostor();
		}
    }
	return LLVOAvatar::isImpostor();
}
void LLControlAvatar::onRegionChanged()
{
	std::vector<LLCharacter*>::iterator it = LLCharacter::sInstances.begin();
	for ( ; it != LLCharacter::sInstances.end(); ++it)
	{
		auto avatar = static_cast<LLVOAvatar*>(*it);
		if (!avatar->isDead() && avatar->isControlAvatar())
		{
			LLControlAvatar* cav = static_cast<LLControlAvatar*>(avatar);
			cav->mRegionChanged = true;
		}
	}
}
