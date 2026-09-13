/** 
 * @file lltoolmorph.cpp
 * @brief A tool to manipulate faces..
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
#include "lltoolmorph.h"
#include "llrender.h"
#include "llaudioengine.h"
#include "llviewercontrol.h"
#include "llfontgl.h"
#include "llwearable.h"
#include "sound_ids.h"
#include "v3math.h"
#include "v3color.h"
#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
#include "llfloatercustomize.h"
#include "llmorphview.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "lltexlayer.h"
#include "lltoolmgr.h"
#include "lltoolview.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "pipeline.h"
#include "llagentwearables.h"
#include "llcharacter.h"
LLVisualParamHint::instance_list_t LLVisualParamHint::sInstances;
BOOL LLVisualParamReset::sDirty = FALSE;
LLVisualParamHint::LLVisualParamHint(
	S32 pos_x, S32 pos_y,
	S32 width, S32 height,
	LLViewerJointMesh *mesh,
	LLViewerVisualParam *param,
	LLWearable *wearable,
	F32 param_weight)
	:
	LLViewerDynamicTexture(width, height, 3, LLViewerDynamicTexture::ORDER_MIDDLE, TRUE ),
	mNeedsUpdate( TRUE ),
	mIsVisible( FALSE ),
	mJointMesh( mesh ),
	mVisualParam( param ),
	mWearablePtr( wearable ),
	mVisualParamWeight( param_weight ),
	mAllowsUpdates( TRUE ),
	mDelayFrames( 0 ),
	mRect( pos_x, pos_y + height, pos_x + width, pos_y ),
	mLastParamWeight(0.f)
{
	LLVisualParamHint::sInstances.insert( this );
	mBackgroundp = LLUI::getUIImage("avatar_thumb_bkgrnd.png");
	if (mBackgroundp.isNull())
	{
		mBackgroundp = LLUI::getUIImage("avatar_thumb_bkgrnd.j2c");
	}
	llassert(width != 0);
	llassert(height != 0);
}
LLVisualParamHint::~LLVisualParamHint()
{
	LLVisualParamHint::sInstances.erase( this );
}
S8 LLVisualParamHint::getType() const
{
	return LLViewerDynamicTexture::LL_VISUAL_PARAM_HINT ;
}
void LLVisualParamHint::requestHintUpdates( LLVisualParamHint* exception1, LLVisualParamHint* exception2 )
{
	S32 delay_frames = 0;
	for (instance_list_t::iterator iter = sInstances.begin();
		 iter != sInstances.end(); ++iter)
	{
		LLVisualParamHint* instance = *iter;
		if( (instance != exception1) && (instance != exception2) )
		{
			if( instance->mAllowsUpdates )
			{
				instance->mNeedsUpdate = TRUE;
				instance->mDelayFrames = delay_frames;
				delay_frames++;
			}
			else
			{
				instance->mNeedsUpdate = TRUE;
				instance->mDelayFrames = 0;
			}
		}
	}
}
BOOL LLVisualParamHint::needsRender()
{
	return mNeedsUpdate && mDelayFrames-- <= 0 && !gAgentAvatarp->getIsAppearanceAnimating() && mAllowsUpdates;
}
void LLVisualParamHint::preRender(BOOL clear_depth)
{
	mLastParamWeight = mVisualParam->getWeight();
	if(mWearablePtr)mWearablePtr->setVisualParamWeight(mVisualParam->getID(), mVisualParamWeight, FALSE);
 	gAgentAvatarp->setVisualParamWeight(mVisualParam->getID(), mVisualParamWeight, FALSE);
	gAgentAvatarp->setVisualParamWeight("Blink_Left", 0.f);
	gAgentAvatarp->setVisualParamWeight("Blink_Right", 0.f);
	gAgentAvatarp->updateComposites();
	gAgentAvatarp->LLCharacter::updateVisualParams();
	if (gAgentAvatarp->mDrawable.notNull())
	{
		gAgentAvatarp->updateGeometry(gAgentAvatarp->mDrawable);
		gAgentAvatarp->updateLOD();
		if (gAgentAvatarp->mDrawable->isState(LLDrawable::REBUILD_GEOMETRY)
			|| gAgentAvatarp->getDirtyMesh() > 0)
		{
			gAgentAvatarp->updateMeshData();
		}
	}
	LLViewerDynamicTexture::preRender(clear_depth);
}
BOOL LLVisualParamHint::render()
{
	LLVisualParamReset::sDirty = TRUE;
	gGL.pushUIMatrix();
	gGL.loadUIIdentity();
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho(0.0f, mFullWidth, 0.0f, mFullHeight, -1.0f, 1.0f);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadIdentity();
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}
	LLGLSUIDefault gls_ui;
	if (mBackgroundp.notNull())
	{
		mBackgroundp->draw(0, 0, mFullWidth, mFullHeight);
	}
	else
	{
		gGL.color4f(0.18f, 0.18f, 0.22f, 1.f);
		gGL.begin(LLRender::TRIANGLES);
		{
			gGL.vertex2i(0, 0);
			gGL.vertex2i(mFullWidth, 0);
			gGL.vertex2i(mFullWidth, mFullHeight);
			gGL.vertex2i(0, 0);
			gGL.vertex2i(mFullWidth, mFullHeight);
			gGL.vertex2i(0, mFullHeight);
		}
		gGL.end();
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
	}
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();
	mNeedsUpdate = FALSE;
	mIsVisible = TRUE;
	LLJoint* cam_target_joint = NULL;
	const std::string& cam_target_mesh_name = mVisualParam->getCameraTargetName();
	if( !cam_target_mesh_name.empty() )
	{
		cam_target_joint = gAgentAvatarp->getJoint( cam_target_mesh_name );
	}
	if( !cam_target_joint && gMorphView )
	{
		cam_target_joint = gMorphView->getCameraTargetJoint();
	}
	if( !cam_target_joint )
	{
		cam_target_joint = gAgentAvatarp->getJoint("mHead");
	}
	if (!cam_target_joint)
	{
		gAgentAvatarp->setVisualParamWeight(mVisualParam->getID(), mLastParamWeight);
		if(mWearablePtr)mWearablePtr->setVisualParamWeight(mVisualParam->getID(), mLastParamWeight, FALSE);
		gAgentAvatarp->updateVisualParams();
		gGL.popUIMatrix();
		return TRUE;
	}
	LLQuaternion avatar_rotation;
	LLJoint* root_joint = gAgentAvatarp->getRootJoint();
	if( root_joint )
	{
		avatar_rotation = root_joint->getWorldRotation();
	}
	LLVector3 target_joint_pos = cam_target_joint->getWorldPosition();
	LLVector3 target_offset( 0, 0, mVisualParam->getCameraElevation() );
	LLVector3 target_pos = target_joint_pos + (target_offset * avatar_rotation);
	F32 cam_angle_radians = mVisualParam->getCameraAngle() * DEG_TO_RAD;
	static LLCachedControl<bool> auto_camera_position(gSavedSettings, "AppearanceCameraMovement");
	if (!auto_camera_position)
	{
		cam_angle_radians += F_PI;
	}
	LLVector3 camera_snapshot_offset(
		mVisualParam->getCameraDistance() * cosf( cam_angle_radians ),
		mVisualParam->getCameraDistance() * sinf( cam_angle_radians ),
		mVisualParam->getCameraElevation() );
	LLVector3 camera_pos = target_joint_pos + (camera_snapshot_offset * avatar_rotation);
	gGL.flush();
	LLViewerCamera::getInstance()->setAspect((F32)mFullWidth / (F32)mFullHeight);
	LLViewerCamera::getInstance()->setOriginAndLookAt(
		camera_pos,
		LLVector3::z_axis,
		target_pos );
	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, FALSE);
	if (gAgentAvatarp->mDrawable.notNull() && !LLPipeline::sImpostorRender)
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_REPLACE);
		gPipeline.generateImpostor(gAgentAvatarp, true);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		gGL.flush();
	}
	gAgentAvatarp->setVisualParamWeight(mVisualParam->getID(), mLastParamWeight);
	if(mWearablePtr)mWearablePtr->setVisualParamWeight(mVisualParam->getID(), mLastParamWeight, FALSE);
	gAgentAvatarp->updateVisualParams();
	gGL.color4f(1,1,1,1);
	mGLTexturep->setGLTextureCreated(true);
	gGL.popUIMatrix();
	return TRUE;
}
void LLVisualParamHint::draw()
{
	if (!mIsVisible) return;
	gGL.getTexUnit(0)->bind(this);
	gGL.color4f(1.f, 1.f, 1.f, 1.f);
	LLGLSUIDefault gls_ui;
	gGL.begin(LLRender::TRIANGLES);
	{
		gGL.texCoord2i(0, 1);
		gGL.vertex2i(0, mFullHeight);
		gGL.texCoord2i(0, 0);
		gGL.vertex2i(0, 0);
		gGL.texCoord2i(1, 0);
		gGL.vertex2i(mFullWidth, 0);
		gGL.texCoord2i(0, 1);
		gGL.vertex2i(0, mFullHeight);
		gGL.texCoord2i(1, 0);
		gGL.vertex2i(mFullWidth, 0);
		gGL.texCoord2i(1, 1);
		gGL.vertex2i(mFullWidth, mFullHeight);
	}
	gGL.end();
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}
LLVisualParamReset::LLVisualParamReset() : LLViewerDynamicTexture(1, 1, 1, ORDER_RESET, FALSE)
{
}
S8 LLVisualParamReset::getType() const
{
	return LLViewerDynamicTexture::LL_VISUAL_PARAM_RESET ;
}
BOOL LLVisualParamReset::render()
{
	if (sDirty)
	{
		gAgentAvatarp->updateComposites();
		gAgentAvatarp->updateVisualParams();
		gAgentAvatarp->updateGeometry(gAgentAvatarp->mDrawable);
		sDirty = FALSE;
	}
	return FALSE;
}
