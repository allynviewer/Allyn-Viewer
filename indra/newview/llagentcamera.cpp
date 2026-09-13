/** 
 * @file llagentcamera.cpp
 * @brief LLAgent class implementation
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
#include "llagentcamera.h"
#include "pipeline.h"
#include "aosystem.h"
#include "llagent.h"
#include "llanimationstates.h"
#include "llfloatercamera.h"
#include "llfloatergroups.h"
#include "llhudmanager.h"
#include "lljoystickbutton.h"
#include "llselectmgr.h"
#include "llsmoothstep.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwindow.h"
#include "llworld.h"
#include "llfloatertools.h"
#include "llfloatercustomize.h"
#include "rlvhandler.h"
using namespace LLAvatarAppearanceDefines;
extern LLMenuBarGL* gMenuBarView;
const LLVector3d FACE_EDIT_CAMERA_OFFSET(0.4f, -0.05f, 0.07f);
const LLVector3d FACE_EDIT_TARGET_OFFSET(0.f, 0.f, 0.05f);
const F32 MIN_ZOOM_FRACTION = 0.25f;
const F32 INITIAL_ZOOM_FRACTION = 1.f;
const F32 MAX_ZOOM_FRACTION = 8.f;
const F32 CAMERA_ZOOM_HALF_LIFE = 0.07f;
const F32 FOV_ZOOM_HALF_LIFE = 0.07f;
const F32 CAMERA_FOCUS_HALF_LIFE = 0.f;
const F32 CAMERA_LAG_HALF_LIFE = 0.25f;
const F32 MIN_CAMERA_LAG = 0.5f;
const F32 MAX_CAMERA_LAG = 5.f;
const F32 CAMERA_COLLIDE_EPSILON = 0.f;
const F32 MIN_CAMERA_DISTANCE = 0.f;
const F32 AVATAR_ZOOM_MIN_X_FACTOR = 0.f;
const F32 AVATAR_ZOOM_MIN_Y_FACTOR = 0.f;
const F32 AVATAR_ZOOM_MIN_Z_FACTOR = 0.f;
const F32 MAX_CAMERA_DISTANCE_FROM_AGENT = 50.f;
const F32 MAX_CAMERA_SMOOTH_DISTANCE = 50.0f;
const F32 HEAD_BUFFER_SIZE = 0.3f;
const F32 CUSTOMIZE_AVATAR_CAMERA_ANIM_SLOP = 0.1f;
const F32 LAND_MIN_ZOOM = 0.15f;
const F32 AVATAR_MIN_ZOOM = 0.5f;
const F32 OBJECT_MIN_ZOOM = 0.02f;
const F32 APPEARANCE_MIN_ZOOM = 0.39f;
const F32 APPEARANCE_MAX_ZOOM = 8.f;
const F32 CUSTOMIZE_AVATAR_CAMERA_DEFAULT_DIST = 3.5f;
const F32 GROUND_TO_AIR_CAMERA_TRANSITION_TIME = 0.5f;
const F32 GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME = 0.5f;
const F32 OBJECT_EXTENTS_PADDING = 0.5f;
LLAgentCamera gAgentCamera;
LLAgentCamera::LLAgentCamera() :
	mInitialized(false),
	mDrawDistance( DEFAULT_FAR_PLANE ),
	mLookAt(NULL),
	mPointAt(NULL),
	mHUDTargetZoom(1.f),
	mHUDCurZoom(1.f),
	mForceMouselook(FALSE),
	mCameraMode( CAMERA_MODE_THIRD_PERSON ),
	mLastCameraMode( CAMERA_MODE_THIRD_PERSON ),
	mCameraPreset(CAMERA_PRESET_REAR_VIEW),
	mCameraAnimating( FALSE ),
	mAnimationCameraStartGlobal(),
	mAnimationFocusStartGlobal(),
	mAnimationTimer(),
	mAnimationDuration(0.33f),
	mCameraFOVZoomFactor(0.f),
	mCameraCurrentFOVZoomFactor(0.f),
	mCameraFocusOffset(),
	mCameraFOVDefault(DEFAULT_FIELD_OF_VIEW),
	mCameraCollidePlane(),
	mCurrentCameraDistance(2.f),
	mTargetCameraDistance(2.f),
	mCameraZoomFraction(1.f),
	mThirdPersonHeadOffset(0.f, 0.f, 1.f),
	mSitCameraEnabled(FALSE),
	mCameraSmoothingLastPositionGlobal(),
	mCameraSmoothingLastPositionAgent(),
	mCameraSmoothingStop(false),
	mCameraUpVector(LLVector3::z_axis),
	mFocusOnAvatar(TRUE),
	mAllowChangeToFollow(FALSE),
	mFocusGlobal(),
	mFocusTargetGlobal(),
	mFocusObject(NULL),
	mFocusObjectDist(0.f),
	mFocusObjectOffset(),
	mFocusDotRadius( 0.1f ),
	mTrackFocusObject(TRUE),
	mUIOffset(0.f),
	mAtKey(0),
	mWalkKey(0),
	mLeftKey(0),
	mUpKey(0),
	mYawKey(0.f),
	mPitchKey(0.f),
	mOrbitLeftKey(0.f),
	mOrbitRightKey(0.f),
	mOrbitUpKey(0.f),
	mOrbitDownKey(0.f),
	mOrbitInKey(0.f),
	mOrbitOutKey(0.f),
	mPanUpKey(0.f),
	mPanDownKey(0.f),
	mPanLeftKey(0.f),
	mPanRightKey(0.f),
	mPanInKey(0.f),
	mPanOutKey(0.f)
{
	mFollowCam.setMaxCameraDistantFromSubject( MAX_CAMERA_DISTANCE_FROM_AGENT );
	clearGeneralKeys();
	clearOrbitKeys();
	clearPanKeys();
}
void LLAgentCamera::init()
{
	mDrawDistance = gSavedSettings.getF32("RenderFarClip");
	LLViewerCamera::getInstance()->setView(DEFAULT_FIELD_OF_VIEW);
	LLViewerCamera::getInstance()->setNear(0.1f);
	LLViewerCamera::getInstance()->setFar(mDrawDistance);
	LLViewerCamera::getInstance()->setAspect( gViewerWindow->getDisplayAspectRatio() );
	LLViewerCamera::getInstance()->setViewHeightInPixels(768);
	mCameraFocusOffsetTarget = LLVector4(gSavedSettings.getVector3("CameraOffsetBuild"));
	mCameraPreset = (ECameraPreset) gSavedSettings.getU32("CameraPreset");
	mCameraOffsetInitial[CAMERA_PRESET_REAR_VIEW] = gSavedSettings.getControl("CameraOffsetRearView");
	mCameraOffsetInitial[CAMERA_PRESET_FRONT_VIEW] = gSavedSettings.getControl("CameraOffsetFrontView");
	mCameraOffsetInitial[CAMERA_PRESET_GROUP_VIEW] = gSavedSettings.getControl("CameraOffsetGroupView");
	mFocusOffsetInitial[CAMERA_PRESET_REAR_VIEW] = gSavedSettings.getControl("FocusOffsetRearView");
	mFocusOffsetInitial[CAMERA_PRESET_FRONT_VIEW] = gSavedSettings.getControl("FocusOffsetFrontView");
	mFocusOffsetInitial[CAMERA_PRESET_GROUP_VIEW] = gSavedSettings.getControl("FocusOffsetGroupView");
	mCameraCollidePlane.clearVec();
	mCurrentCameraDistance = getCameraOffsetInitial().magVec() * gSavedSettings.getF32("CameraOffsetScale");
	mTargetCameraDistance = mCurrentCameraDistance;
	mCameraZoomFraction = 1.f;
	mTrackFocusObject = gSavedSettings.getBOOL("TrackFocusObject");
	mInitialized = true;
}
void LLAgentCamera::cleanup()
{
	setSitCamera(LLUUID::null);
	if(mLookAt)
	{
		mLookAt->markDead() ;
		mLookAt = NULL;
	}
	if(mPointAt)
	{
		mPointAt->markDead() ;
		mPointAt = NULL;
	}
	setFocusObject(NULL);
}
void LLAgentCamera::setAvatarObject(LLVOAvatarSelf* avatar)
{
	if (!mLookAt)
	{
		mLookAt = (LLHUDEffectLookAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_LOOKAT);
	}
	if (!mPointAt)
	{
		mPointAt = (LLHUDEffectPointAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINTAT);
	}
	if (!mLookAt.isNull())
	{
		mLookAt->setSourceObject(avatar);
	}
	if (!mPointAt.isNull())
	{
		mPointAt->setSourceObject(avatar);
	}
}
LLAgentCamera::~LLAgentCamera()
{
	cleanup();
}
void LLAgentCamera::resetView(BOOL reset_camera, BOOL change_camera)
{
	LLSelectMgr::getInstance()->unhighlightAll();
	if (LLSelectMgr::getInstance()->getSelection()->isAttachment())
	{
		LLSelectMgr::getInstance()->deselectAll();
	}
	if (gMenuHolder != NULL)
	{
		gMenuHolder->hideMenus();
	}
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if (change_camera && !freeze_time)
	{
		changeCameraToDefault();
		if (LLViewerJoystick::getInstance()->getOverrideCamera())
		{
			handle_toggle_flycam();
		}
		if (LLToolMgr::getInstance()->inBuildMode())
		{
			LLViewerJoystick::getInstance()->moveAvatar(true);
		}
		gFloaterTools->close();
		gViewerWindow->showCursor();
		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	}
	if (reset_camera && !freeze_time)
	{
		if (!gViewerWindow->getLeftMouseDown() && cameraThirdPerson())
		{
			LLVector3 agent_at_axis = gAgent.getAtAxis();
			agent_at_axis -= projected_vec(agent_at_axis, gAgent.getReferenceUpVector());
			agent_at_axis.normalize();
			gAgent.resetAxes(lerp(gAgent.getAtAxis(), agent_at_axis, LLSmoothInterpolation::getInterpolant(0.3f)));
		}
		setFocusOnAvatar(TRUE, ANIMATE);
		mCameraFOVZoomFactor = 0.f;
	}
	mHUDTargetZoom = 1.f;
}
void LLAgentCamera::unlockView()
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMUNLOCK)) return;
	if (getFocusOnAvatar())
	{
		if (isAgentAvatarValid())
		{
			setFocusGlobal(LLVector3d::zero, gAgentAvatarp->mID);
		}
		setFocusOnAvatar(FALSE, FALSE);
	}
}
void LLAgentCamera::slamLookAt(const LLVector3 &look_at)
{
	LLVector3 look_at_norm = look_at;
	look_at_norm.mV[VZ] = 0.f;
	look_at_norm.normalize();
	gAgent.resetAxes(look_at_norm);
}
LLVector3 LLAgentCamera::calcFocusOffset(LLViewerObject *object, LLVector3 original_focus_point, S32 x, S32 y)
{
	const LLMatrix4a& obj_matrix = object->getRenderMatrix();
	LLQuaternion obj_rot = object->getRenderRotation();
	LLVector3 obj_pos = object->getRenderPosition();
	BOOL is_avatar = object->isAvatar();
	if (is_avatar)
	{
		return original_focus_point - obj_pos;
	}
	LLQuaternion inv_obj_rot = ~obj_rot;
	LLVector3 object_extents = object->getScale();
	object_extents.clamp(0.001f, F32_MAX);
	LLVector3 obj_to_cam_ray = obj_pos - LLViewerCamera::getInstance()->getOrigin();
	obj_to_cam_ray.rotVec(inv_obj_rot);
	obj_to_cam_ray.normalize();
	LLVector3 obj_to_cam_ray_proportions;
	obj_to_cam_ray_proportions.mV[VX] = llabs(obj_to_cam_ray.mV[VX] / object_extents.mV[VX]);
	obj_to_cam_ray_proportions.mV[VY] = llabs(obj_to_cam_ray.mV[VY] / object_extents.mV[VY]);
	obj_to_cam_ray_proportions.mV[VZ] = llabs(obj_to_cam_ray.mV[VZ] / object_extents.mV[VZ]);
	LLVector4a focus_plane_normal;
	if (obj_to_cam_ray_proportions.mV[VX] > obj_to_cam_ray_proportions.mV[VY]
		&& obj_to_cam_ray_proportions.mV[VX] > obj_to_cam_ray_proportions.mV[VZ])
	{
		focus_plane_normal = obj_matrix.getRow<LLMatrix4a::ROW_FWD>();
	}
	else if (obj_to_cam_ray_proportions.mV[VY] > obj_to_cam_ray_proportions.mV[VZ])
	{
		focus_plane_normal = obj_matrix.getRow<LLMatrix4a::ROW_LEFT>();
	}
	else
	{
		focus_plane_normal = obj_matrix.getRow<LLMatrix4a::ROW_UP>();
	}
	focus_plane_normal.normalize3fast();
	LLVector3d focus_pt_global;
	gViewerWindow->mousePointOnPlaneGlobal(focus_pt_global, x, y, gAgent.getPosGlobalFromAgent(obj_pos), LLVector3(focus_plane_normal.getF32ptr()));
	LLVector3 focus_pt = gAgent.getPosAgentFromGlobal(focus_pt_global);
	LLVector3 camera_to_focus_vec = focus_pt - LLViewerCamera::getInstance()->getOrigin();
	camera_to_focus_vec.rotVec(inv_obj_rot);
	LLVector3 focus_offset_from_object_center = focus_pt - obj_pos;
	focus_offset_from_object_center.rotVec(inv_obj_rot);
	LLVector3 clip_fraction;
	for (U32 axis = VX; axis <= VZ; axis++)
	{
		F32 dist_out_of_bounds;
		if (focus_offset_from_object_center.mV[axis] > 0.f)
		{
			dist_out_of_bounds = llmax(0.f, focus_offset_from_object_center.mV[axis] - (object_extents.mV[axis] * 0.5f));
		}
		else
		{
			dist_out_of_bounds = llmin(0.f, focus_offset_from_object_center.mV[axis] + (object_extents.mV[axis] * 0.5f));
		}
		if (llabs(camera_to_focus_vec.mV[axis]) < 0.0001f)
		{
			clip_fraction.mV[axis] = 0.f;
		}
		else
		{
			clip_fraction.mV[axis] = dist_out_of_bounds / camera_to_focus_vec.mV[axis];
		}
	}
	LLVector3 abs_clip_fraction = clip_fraction;
	abs_clip_fraction.abs();
	if (abs_clip_fraction.mV[VX] > abs_clip_fraction.mV[VY]
		&& abs_clip_fraction.mV[VX] > abs_clip_fraction.mV[VZ])
	{
		focus_offset_from_object_center -= clip_fraction.mV[VX] * camera_to_focus_vec;
	}
	else if (abs_clip_fraction.mV[VY] > abs_clip_fraction.mV[VZ])
	{
		focus_offset_from_object_center -= clip_fraction.mV[VY] * camera_to_focus_vec;
	}
	else
	{
		focus_offset_from_object_center -= clip_fraction.mV[VZ] * camera_to_focus_vec;
	}
	focus_offset_from_object_center.rotVec(obj_rot);
	if (!is_avatar)
	{
		LLVector3 obj_rel = original_focus_point - object->getRenderPosition();
		F32 relDist = llabs(obj_rel * LLViewerCamera::getInstance()->getAtAxis());
		F32 viewDist = dist_vec(obj_pos + obj_rel, LLViewerCamera::getInstance()->getOrigin());
		LLBBox obj_bbox = object->getBoundingBoxAgent();
		F32 bias = 0.f;
		LLVector3 virtual_camera_pos = gAgent.getPosAgentFromGlobal(mFocusTargetGlobal + (getCameraPositionGlobal() - mFocusTargetGlobal) / (1.f + mCameraFOVZoomFactor));
		if(!obj_bbox.containsPointAgent(virtual_camera_pos))
		{
			bias = clamp_rescale(relDist/viewDist, 0.1f, 0.7f, 0.0f, 1.0f);
			obj_rel = lerp(focus_offset_from_object_center, obj_rel, bias);
		}
		focus_offset_from_object_center = obj_rel;
	}
	return focus_offset_from_object_center;
}
BOOL LLAgentCamera::calcCameraMinDistance(F32 &obj_min_distance)
{
	BOOL soft_limit = FALSE;
	static const LLCachedControl<bool> disable_camera_constraints("DisableCameraConstraints");
	if (!mFocusObject || mFocusObject->isDead() ||
		mFocusObject->isMesh() ||
		disable_camera_constraints)
	{
		obj_min_distance = 0.f;
		return TRUE;
	}
	if (mFocusObject->mDrawable.isNull())
	{
#ifdef LL_RELEASE_FOR_DOWNLOAD
		LL_WARNS() << "Focus object with no drawable!" << LL_ENDL;
#else
		mFocusObject->dump();
		LL_ERRS() << "Focus object with no drawable!" << LL_ENDL;
#endif
		obj_min_distance = 0.f;
		return TRUE;
	}
	LLQuaternion inv_object_rot = ~mFocusObject->getRenderRotation();
	LLVector3 target_offset_origin = mFocusObjectOffset;
	LLVector3 camera_offset_target(getCameraPositionAgent() - gAgent.getPosAgentFromGlobal(mFocusTargetGlobal));
	camera_offset_target.rotVec(inv_object_rot);
	target_offset_origin.rotVec(inv_object_rot);
	LLVector3 object_extents = mFocusObject->getScale();
	if (mFocusObject->isAvatar())
	{
		object_extents.mV[VX] *= AVATAR_ZOOM_MIN_X_FACTOR;
		object_extents.mV[VY] *= AVATAR_ZOOM_MIN_Y_FACTOR;
		object_extents.mV[VZ] *= AVATAR_ZOOM_MIN_Z_FACTOR;
		soft_limit = TRUE;
	}
	LLVector3 abs_target_offset = target_offset_origin;
	abs_target_offset.abs();
	LLVector3 target_offset_dir = target_offset_origin;
	BOOL target_outside_object_extents = FALSE;
	for (U32 i = VX; i <= VZ; i++)
	{
		if (abs_target_offset.mV[i] * 2.f > object_extents.mV[i] + OBJECT_EXTENTS_PADDING)
		{
			target_outside_object_extents = TRUE;
		}
		if (camera_offset_target.mV[i] > 0.f)
		{
			object_extents.mV[i] -= target_offset_origin.mV[i] * 2.f;
		}
		else
		{
			object_extents.mV[i] += target_offset_origin.mV[i] * 2.f;
		}
	}
	object_extents.clamp(0.001f, F32_MAX);
	LLVector3 camera_offset_target_abs_norm = camera_offset_target;
	camera_offset_target_abs_norm.abs();
	camera_offset_target_abs_norm.clamp(0.001f, F32_MAX);
	camera_offset_target_abs_norm.normalize();
	LLVector3 camera_offset_target_scaled = camera_offset_target_abs_norm;
	camera_offset_target_scaled.mV[VX] /= object_extents.mV[VX];
	camera_offset_target_scaled.mV[VY] /= object_extents.mV[VY];
	camera_offset_target_scaled.mV[VZ] /= object_extents.mV[VZ];
	if (camera_offset_target_scaled.mV[VX] > camera_offset_target_scaled.mV[VY] &&
		camera_offset_target_scaled.mV[VX] > camera_offset_target_scaled.mV[VZ])
	{
		if (camera_offset_target_abs_norm.mV[VX] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VX] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VX] * 0.5f / camera_offset_target_abs_norm.mV[VX];
		}
	}
	else if (camera_offset_target_scaled.mV[VY] > camera_offset_target_scaled.mV[VZ])
	{
		if (camera_offset_target_abs_norm.mV[VY] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VY] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VY] * 0.5f / camera_offset_target_abs_norm.mV[VY];
		}
	}
	else
	{
		if (camera_offset_target_abs_norm.mV[VZ] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VZ] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VZ] * 0.5f / camera_offset_target_abs_norm.mV[VZ];
		}
	}
	LLVector3 object_split_axis;
	LLVector3 target_offset_scaled = target_offset_origin;
	target_offset_scaled.abs();
	target_offset_scaled.normalize();
	target_offset_scaled.mV[VX] /= object_extents.mV[VX];
	target_offset_scaled.mV[VY] /= object_extents.mV[VY];
	target_offset_scaled.mV[VZ] /= object_extents.mV[VZ];
	if (target_offset_scaled.mV[VX] > target_offset_scaled.mV[VY] &&
		target_offset_scaled.mV[VX] > target_offset_scaled.mV[VZ])
	{
		object_split_axis = LLVector3::x_axis;
	}
	else if (target_offset_scaled.mV[VY] > target_offset_scaled.mV[VZ])
	{
		object_split_axis = LLVector3::y_axis;
	}
	else
	{
		object_split_axis = LLVector3::z_axis;
	}
	LLVector3 camera_offset_object(getCameraPositionAgent() - mFocusObject->getPositionAgent());
	F32 camera_offset_clip = camera_offset_object * object_split_axis;
	F32 target_offset_clip = target_offset_dir * object_split_axis;
	if (target_outside_object_extents)
	{
		if (camera_offset_clip > 0.f && target_offset_clip > 0.f)
		{
			return FALSE;
		}
		else if (camera_offset_clip < 0.f && target_offset_clip < 0.f)
		{
			return FALSE;
		}
	}
	obj_min_distance = llmin(obj_min_distance, 10.f * F_SQRT3);
	obj_min_distance += LLViewerCamera::getInstance()->getNear() + (soft_limit ? 0.1f : 0.2f);
	return TRUE;
}
F32 LLAgentCamera::getCameraZoomFraction()
{
	static const LLCachedControl<bool> ascent_disable_min_zoom_dist("AscentDisableMinZoomDist");
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		return mHUDTargetZoom;
	}
	else if (ascent_disable_min_zoom_dist)
	{
		return mCameraZoomFraction;
	}
	else if (mFocusOnAvatar && cameraThirdPerson())
	{
		return clamp_rescale(mCameraZoomFraction, MIN_ZOOM_FRACTION, MAX_ZOOM_FRACTION, 1.f, 0.f);
	}
	else if (cameraCustomizeAvatar())
	{
		F32 distance = (F32)mCameraFocusOffsetTarget.magVec();
		return clamp_rescale(distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM, 1.f, 0.f );
	}
	else
	{
		F32 min_zoom;
		F32 max_zoom = 65535.f*4.f;
		F32 distance = (F32)mCameraFocusOffsetTarget.magVec();
		if (mFocusObject.notNull())
		{
			if (mFocusObject->isAvatar())
			{
				min_zoom = AVATAR_MIN_ZOOM;
			}
			else
			{
				min_zoom = OBJECT_MIN_ZOOM;
			}
		}
		else
		{
			min_zoom = LAND_MIN_ZOOM;
		}
		return clamp_rescale(distance, min_zoom, max_zoom, 1.f, 0.f);
	}
}
void LLAgentCamera::setCameraZoomFraction(F32 fraction)
{
	static const LLCachedControl<bool> ascent_disable_min_zoom_dist("AscentDisableMinZoomDist");
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		mHUDTargetZoom = fraction;
	}
	else if (mFocusOnAvatar && cameraThirdPerson() && !ascent_disable_min_zoom_dist)
	{
		mCameraZoomFraction = rescale(fraction, 0.f, 1.f, MAX_ZOOM_FRACTION, MIN_ZOOM_FRACTION);
	}
	else if (cameraCustomizeAvatar())
	{
		LLVector3d camera_offset_dir = mCameraFocusOffsetTarget;
		camera_offset_dir.normalize();
		mCameraFocusOffsetTarget = camera_offset_dir * rescale(fraction, 0.f, 1.f, APPEARANCE_MAX_ZOOM, APPEARANCE_MIN_ZOOM);
	}
	else
	{
		F32 min_zoom = LAND_MIN_ZOOM;
		if (!ascent_disable_min_zoom_dist)
		{
			if (mFocusObject.notNull())
			{
				if (mFocusObject->isAvatar())
				{
					min_zoom = AVATAR_MIN_ZOOM;
				}
				else
				{
					min_zoom = OBJECT_MIN_ZOOM;
				}
			}
		}
		else
		{
			min_zoom = 0.f;
		}
		LLVector3d camera_offset_dir = mCameraFocusOffsetTarget;
		camera_offset_dir.normalize();
		mCameraFocusOffsetTarget = camera_offset_dir * rescale(fraction, 0.f, 65535.*4., 1.f, min_zoom);
	}
	startCameraAnimation();
}
void LLAgentCamera::cameraOrbitAround(const F32 radians)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
	}
	else if (mFocusOnAvatar && (mCameraMode == CAMERA_MODE_THIRD_PERSON || mCameraMode == CAMERA_MODE_FOLLOW))
	{
		gAgent.yaw(radians);
	}
	else
	{
		mCameraFocusOffsetTarget.rotVec(radians, 0.f, 0.f, 1.f);
		cameraZoomIn(1.f);
	}
}
void LLAgentCamera::cameraOrbitOver(const F32 angle)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
	}
	else if (mFocusOnAvatar && mCameraMode == CAMERA_MODE_THIRD_PERSON)
	{
		gAgent.pitch(angle);
	}
	else
	{
		LLVector3 camera_offset_unit(mCameraFocusOffsetTarget);
		camera_offset_unit.normalize();
		F32 angle_from_up = acos( camera_offset_unit * gAgent.getReferenceUpVector() );
		LLVector3d left_axis;
		left_axis.setVec(LLViewerCamera::getInstance()->getLeftAxis());
		F32 new_angle = llclamp(angle_from_up - angle, 1.f * DEG_TO_RAD, 179.f * DEG_TO_RAD);
		mCameraFocusOffsetTarget.rotVec(angle_from_up - new_angle, left_axis);
		cameraZoomIn(1.f);
	}
}
void LLAgentCamera::cameraZoomIn(const F32 fraction)
{
	if (gDisconnected)
	{
		return;
	}
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		mHUDTargetZoom /= fraction;
		return;
	}
	LLVector3d	camera_offset_unit(mCameraFocusOffsetTarget);
	F32 min_zoom = 0.02f;
	F32 current_distance = (F32)camera_offset_unit.normalize();
	F32 new_distance = current_distance * fraction;
	static const LLCachedControl<bool> ascent_disable_min_zoom_dist("AscentDisableMinZoomDist");
	if (!ascent_disable_min_zoom_dist)
	{
		if (mFocusObject)
		{
			LLVector3 camera_offset_dir((F32)camera_offset_unit.mdV[VX], (F32)camera_offset_unit.mdV[VY], (F32)camera_offset_unit.mdV[VZ]);
			if (mFocusObject->isAvatar())
			{
				calcCameraMinDistance(min_zoom);
			}
			else
			{
				min_zoom = OBJECT_MIN_ZOOM;
			}
		}
	}
	new_distance = llmax(new_distance, min_zoom);
	const F32 DIST_FUDGE = 16.f;
	F32 max_distance = INT_MAX - DIST_FUDGE
							 ;
	max_distance = llmin(max_distance, current_distance * 4.f);
	if (new_distance > max_distance)
	{
	}
	if(cameraCustomizeAvatar())
	{
		new_distance = llclamp( new_distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM );
	}
	mCameraFocusOffsetTarget = new_distance * camera_offset_unit;
}
void LLAgentCamera::cameraOrbitIn(const F32 meters)
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMAX))
	{
		F32 dist_max(gRlvHandler.camPole(RLV_BHVR_CAMDISTMAX));
		if (0 >= dist_max)
		{
			if (!cameraMouselook())
				changeCameraToMouselook();
			return;
		}
	}
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMIN))
	{
		F32 dist_min(gRlvHandler.camPole(RLV_BHVR_CAMDISTMIN));
		if (cameraMouselook() && dist_min > 0)
			changeCameraToDefault();
	}
	if (mFocusOnAvatar && mCameraMode == CAMERA_MODE_THIRD_PERSON)
	{
		static const LLCachedControl<F32> camera_offset_scale("CameraOffsetScale");
		F32 camera_offset_dist = llmax(0.001f, getCameraOffsetInitial().magVec() * camera_offset_scale);
		mCameraZoomFraction = (mTargetCameraDistance - meters) / camera_offset_dist;
		static const LLCachedControl<bool> freeze_time("FreezeTime",false);
		if (!freeze_time && mCameraZoomFraction < MIN_ZOOM_FRACTION && meters > 0.f)
		{
			changeCameraToMouselook(FALSE);
		}
		mCameraZoomFraction = llclamp(mCameraZoomFraction, MIN_ZOOM_FRACTION, MAX_ZOOM_FRACTION);
	}
	else
	{
		LLVector3d	camera_offset_unit(mCameraFocusOffsetTarget);
		F32 current_distance = (F32)camera_offset_unit.normalize();
		F32 new_distance = current_distance - meters;
		mCameraFocusOffsetTarget = new_distance * camera_offset_unit;
		cameraZoomIn(1.f);
	}
}
void LLAgentCamera::cameraPanIn(F32 meters)
{
	LLVector3d at_axis;
	at_axis.setVec(LLViewerCamera::getInstance()->getAtAxis());
	mFocusTargetGlobal += meters * at_axis;
	mFocusGlobal = mFocusTargetGlobal;
	updateFocusOffset();
	mCameraSmoothingLastPositionGlobal = calcCameraPositionTargetGlobal();
}
void LLAgentCamera::cameraPanLeft(F32 meters)
{
	LLVector3d left_axis;
	left_axis.setVec(LLViewerCamera::getInstance()->getLeftAxis());
	mFocusTargetGlobal += meters * left_axis;
	mFocusGlobal = mFocusTargetGlobal;
	mCameraSmoothingStop = true;
	cameraZoomIn(1.f);
	updateFocusOffset();
	mCameraSmoothingLastPositionGlobal = calcCameraPositionTargetGlobal();
}
void LLAgentCamera::cameraPanUp(F32 meters)
{
	LLVector3d up_axis;
	up_axis.setVec(LLViewerCamera::getInstance()->getUpAxis());
	mFocusTargetGlobal += meters * up_axis;
	mFocusGlobal = mFocusTargetGlobal;
	mCameraSmoothingStop = true;
	cameraZoomIn(1.f);
	updateFocusOffset();
	mCameraSmoothingLastPositionGlobal = calcCameraPositionTargetGlobal();
}
void LLAgentCamera::updateLookAt(const S32 mouse_x, const S32 mouse_y)
{
	static LLVector3 last_at_axis;
	if (!isAgentAvatarValid()) return;
	LLQuaternion av_inv_rot = ~gAgentAvatarp->mRoot->getWorldRotation();
	LLVector3 root_at = LLVector3::x_axis * gAgentAvatarp->mRoot->getWorldRotation();
	if 	((gViewerWindow->getMouseVelocityStat()->getCurrent() < 0.01f) &&
		 (root_at * last_at_axis > 0.95f))
	{
		LLVector3 vel = gAgentAvatarp->getVelocity();
		if (vel.magVecSquared() > 4.f)
		{
			setLookAt(LOOKAT_TARGET_IDLE, gAgentAvatarp, vel * av_inv_rot);
		}
		else
		{
			LLQuaternion look_rotation = gAgentAvatarp->isSitting() ? gAgentAvatarp->getRenderRotation() : gAgent.getFrameAgent().getQuaternion();
			LLVector3 look_offset = LLVector3(2.f, 0.f, 0.f) * look_rotation * av_inv_rot;
			setLookAt(LOOKAT_TARGET_IDLE, gAgentAvatarp, look_offset);
		}
		last_at_axis = root_at;
		return;
	}
	last_at_axis = root_at;
	if (CAMERA_MODE_CUSTOMIZE_AVATAR == getCameraMode())
	{
		setLookAt(LOOKAT_TARGET_NONE, gAgentAvatarp, LLVector3(-2.f, 0.f, 0.f));
	}
	else
	{
		ELookAtType lookAtType = LOOKAT_TARGET_NONE;
		LLVector3 headLookAxis;
		LLCoordFrame frameCamera = *((LLCoordFrame*)LLViewerCamera::getInstance());
		if (cameraMouselook())
		{
			lookAtType = LOOKAT_TARGET_MOUSELOOK;
		}
		else if (cameraThirdPerson())
		{
			F32 x_from_center =
				((F32) mouse_x / (F32) gViewerWindow->getWorldViewWidthScaled() ) - 0.5f;
			F32 y_from_center =
				((F32) mouse_y / (F32) gViewerWindow->getWorldViewHeightScaled() ) - 0.5f;
			static const LLCachedControl<F32> yaw_from_mouse_position("YawFromMousePosition");
			static const LLCachedControl<F32> pitch_from_mouse_position("PitchFromMousePosition");
			frameCamera.yaw( - x_from_center * yaw_from_mouse_position * DEG_TO_RAD);
			frameCamera.pitch( - y_from_center * pitch_from_mouse_position * DEG_TO_RAD);
			lookAtType = LOOKAT_TARGET_FREELOOK;
		}
		headLookAxis = frameCamera.getAtAxis();
		setLookAt(lookAtType, gAgentAvatarp, headLookAxis);
	}
}
void LLAgentCamera::updateCamera()
{
	mCameraUpVector = LLVector3::z_axis;
	U32 camera_mode = mCameraAnimating ? mLastCameraMode : mCameraMode;
	validateFocusObject();
	static const LLCachedControl<bool> realistic_ml("UseRealisticMouselook");
	if (isAgentAvatarValid() &&
		!realistic_ml &&
		gAgentAvatarp->isSitting() &&
		camera_mode == CAMERA_MODE_MOUSELOOK)
	{
		mCameraUpVector = mCameraUpVector * gAgentAvatarp->getRenderRotation();
	}
	if (cameraThirdPerson() && (mFocusOnAvatar || mAllowChangeToFollow) && LLFollowCamMgr::getActiveFollowCamParams())
	{
		mAllowChangeToFollow = FALSE;
		mFocusOnAvatar = TRUE;
		changeCameraToFollow();
	}
	if ( camera_mode == CAMERA_MODE_FOLLOW && mFocusOnAvatar )
	{
		mCameraUpVector = mFollowCam.getUpVector();
	}
	if (mSitCameraEnabled)
	{
		if (mSitCameraReferenceObject->isDead())
		{
			setSitCamera(LLUUID::null);
		}
	}
	LLFloaterCamera* camera_floater = LLFloaterCamera::getInstance();
	if (camera_floater)
	{
		camera_floater->mRotate->setToggleState(gAgentCamera.getOrbitRightKey() > 0.f,
												gAgentCamera.getOrbitUpKey() > 0.f,
												gAgentCamera.getOrbitLeftKey() > 0.f,
												gAgentCamera.getOrbitDownKey() > 0.f);
		camera_floater->mTrack->setToggleState(gAgentCamera.getPanLeftKey() > 0.f,
											   gAgentCamera.getPanUpKey() > 0.f,
											   gAgentCamera.getPanRightKey() > 0.f,
											   gAgentCamera.getPanDownKey() > 0.f);
	}
	const F32 ORBIT_OVER_RATE = 90.f * DEG_TO_RAD;
	const F32 ORBIT_AROUND_RATE = 90.f * DEG_TO_RAD;
	const F32 PAN_RATE = 5.f;
	if (gAgentCamera.getOrbitUpKey() || gAgentCamera.getOrbitDownKey())
	{
		F32 input_rate = gAgentCamera.getOrbitUpKey() - gAgentCamera.getOrbitDownKey();
		cameraOrbitOver( input_rate * ORBIT_OVER_RATE / gFPSClamped );
	}
	if (gAgentCamera.getOrbitLeftKey() || gAgentCamera.getOrbitRightKey())
	{
		F32 input_rate = gAgentCamera.getOrbitLeftKey() - gAgentCamera.getOrbitRightKey();
		cameraOrbitAround(input_rate * ORBIT_AROUND_RATE / gFPSClamped);
	}
	if (gAgentCamera.getOrbitInKey() || gAgentCamera.getOrbitOutKey())
	{
		F32 input_rate = gAgentCamera.getOrbitInKey() - gAgentCamera.getOrbitOutKey();
		LLVector3d to_focus = gAgent.getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin()) - calcFocusPositionTargetGlobal();
		F32 distance_to_focus = (F32)to_focus.magVec();
		cameraOrbitIn( input_rate * distance_to_focus / gFPSClamped );
	}
	if (gAgentCamera.getPanInKey() || gAgentCamera.getPanOutKey())
	{
		F32 input_rate = gAgentCamera.getPanInKey() - gAgentCamera.getPanOutKey();
		cameraPanIn(input_rate * PAN_RATE / gFPSClamped);
	}
	if (gAgentCamera.getPanRightKey() || gAgentCamera.getPanLeftKey())
	{
		F32 input_rate = gAgentCamera.getPanRightKey() - gAgentCamera.getPanLeftKey();
		cameraPanLeft(input_rate * -PAN_RATE / gFPSClamped );
	}
	if (gAgentCamera.getPanUpKey() || gAgentCamera.getPanDownKey())
	{
		F32 input_rate = gAgentCamera.getPanUpKey() - gAgentCamera.getPanDownKey();
		cameraPanUp(input_rate * PAN_RATE / gFPSClamped );
	}
	gAgentCamera.clearOrbitKeys();
	gAgentCamera.clearPanKeys();
	mCameraFocusOffset = lerp(mCameraFocusOffset, mCameraFocusOffsetTarget, LLSmoothInterpolation::getInterpolant(CAMERA_FOCUS_HALF_LIFE));
	if ( mCameraMode == CAMERA_MODE_FOLLOW )
	{
		if (isAgentAvatarValid())
		{
			LLQuaternion avatarRotationForFollowCam = gAgentAvatarp->isSitting() ? gAgentAvatarp->getRenderRotation() : gAgent.getFrameAgent().getQuaternion();
			LLFollowCamParams* current_cam = LLFollowCamMgr::getActiveFollowCamParams();
			if (current_cam)
			{
				mFollowCam.copyParams(*current_cam);
				mFollowCam.setSubjectPositionAndRotation( gAgentAvatarp->getRenderPosition(), avatarRotationForFollowCam );
				mFollowCam.update();
				LLViewerJoystick::getInstance()->setCameraNeedsUpdate(true);
			}
			else
			{
				changeCameraToThirdPerson(TRUE);
			}
		}
	}
	BOOL hit_limit;
	LLVector3d camera_pos_global;
	LLVector3d camera_target_global = calcCameraPositionTargetGlobal(&hit_limit);
	mCameraVirtualPositionAgent = gAgent.getPosAgentFromGlobal(camera_target_global);
	LLVector3d focus_target_global = calcFocusPositionTargetGlobal();
	mCameraFOVZoomFactor = calcCameraFOVZoomFactor();
	camera_target_global = focus_target_global + (camera_target_global - focus_target_global) * (1.f + mCameraFOVZoomFactor);
	gAgent.setShowAvatar(TRUE);
	if (mCameraAnimating)
	{
		F32 time = mAnimationTimer.getElapsedTimeF32();
		F32 fraction_of_animation = time / mAnimationDuration;
		BOOL isfirstPerson = mCameraMode == CAMERA_MODE_MOUSELOOK;
		BOOL wasfirstPerson = mLastCameraMode == CAMERA_MODE_MOUSELOOK;
		F32 fraction_animation_to_skip;
		if (mAnimationCameraStartGlobal == camera_target_global)
		{
			fraction_animation_to_skip = 0.f;
		}
		else
		{
			LLVector3d cam_delta = mAnimationCameraStartGlobal - camera_target_global;
			fraction_animation_to_skip = HEAD_BUFFER_SIZE / (F32)cam_delta.magVec();
		}
		F32 animation_start_fraction = (wasfirstPerson) ? fraction_animation_to_skip : 0.f;
		F32 animation_finish_fraction =  (isfirstPerson) ? (1.f - fraction_animation_to_skip) : 1.f;
		if (fraction_of_animation < animation_finish_fraction)
		{
			if (fraction_of_animation < animation_start_fraction || fraction_of_animation > animation_finish_fraction )
			{
				gAgent.setShowAvatar(FALSE);
			}
			F32 smooth_fraction_of_animation = llsmoothstep(0.0f, 1.0f, fraction_of_animation);
			camera_pos_global = lerp(mAnimationCameraStartGlobal, camera_target_global, smooth_fraction_of_animation);
			mFocusGlobal = lerp(mAnimationFocusStartGlobal, focus_target_global, smooth_fraction_of_animation);
		}
		else
		{
			mCameraAnimating = FALSE;
			camera_pos_global = camera_target_global;
			mFocusGlobal = focus_target_global;
			gAgent.endAnimationUpdateUI();
			gAgent.setShowAvatar(TRUE);
		}
		if (isAgentAvatarValid() && (mCameraMode != CAMERA_MODE_MOUSELOOK))
		{
			gAgentAvatarp->updateAttachmentVisibility(mCameraMode);
		}
	}
	else
	{
		camera_pos_global = camera_target_global;
		mFocusGlobal = focus_target_global;
		gAgent.setShowAvatar(TRUE);
	}
	if (TRUE)
	{
		LLVector3d agent_pos = gAgent.getPositionGlobal();
		LLVector3d camera_pos_agent = camera_pos_global - agent_pos;
		bool in_build_mode = LLToolMgr::getInstance()->inBuildMode();
		mCameraSmoothingStop = mCameraSmoothingStop || in_build_mode;
		if (cameraThirdPerson() && !mCameraSmoothingStop)
		{
			const F32 SMOOTHING_HALF_LIFE = 0.02f;
			static const LLCachedControl<F32> camera_position_smoothing("CameraPositionSmoothing");
			F32 smoothing = LLSmoothInterpolation::getInterpolant(camera_position_smoothing * SMOOTHING_HALF_LIFE, FALSE);
			if (!mFocusObject)
			{
				LLVector3d delta = camera_pos_agent - mCameraSmoothingLastPositionAgent;
				if (delta.magVec() < MAX_CAMERA_SMOOTH_DISTANCE)
				{
					camera_pos_agent = lerp(mCameraSmoothingLastPositionAgent, camera_pos_agent, smoothing);
					camera_pos_global = camera_pos_agent + agent_pos;
				}
			}
			else
			{
				LLVector3d delta = camera_pos_global - mCameraSmoothingLastPositionGlobal;
				if (delta.magVec() < MAX_CAMERA_SMOOTH_DISTANCE)
				{
					camera_pos_global = lerp(mCameraSmoothingLastPositionGlobal, camera_pos_global, smoothing);
				}
			}
		}
		mCameraSmoothingLastPositionGlobal = camera_pos_global;
		mCameraSmoothingLastPositionAgent = camera_pos_agent;
		mCameraSmoothingStop = false;
	}
	mCameraCurrentFOVZoomFactor = lerp(mCameraCurrentFOVZoomFactor, mCameraFOVZoomFactor, LLSmoothInterpolation::getInterpolant(FOV_ZOOM_HALF_LIFE));
	F32 ui_offset = 0.f;
	if( CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode )
	{
		ui_offset = calcCustomizeAvatarUIOffset( camera_pos_global );
	}
	LLVector3 focus_agent = gAgent.getPosAgentFromGlobal(mFocusGlobal);
	mCameraPositionAgent = gAgent.getPosAgentFromGlobal(camera_pos_global);
	LLViewerCamera::getInstance()->updateCameraLocation(mCameraPositionAgent, mCameraUpVector, focus_agent);
	LLViewerCamera::getInstance()->translate(LLViewerCamera::getInstance()->getLeftAxis() * ui_offset);
	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / (1.f + mCameraCurrentFOVZoomFactor));
	if (cameraCustomizeAvatar())
	{
		setLookAt(LOOKAT_TARGET_FOCUS, NULL, mCameraPositionAgent);
	}
	LLVector3d global_pos = gAgent.getPositionGlobal();
	if (!gAgent.getLastPositionGlobal().isExactlyZero())
	{
		LLVector3d delta = global_pos - gAgent.getLastPositionGlobal();
		gAgent.setDistanceTraveled(gAgent.getDistanceTraveled() + delta.magVec());
	}
	gAgent.setLastPositionGlobal(global_pos);
	if (LLVOAvatar::sVisibleInFirstPerson && isAgentAvatarValid() && (realistic_ml || !gAgentAvatarp->isSitting()) && cameraMouselook())
	{
		LLVector3 head_pos = gAgentAvatarp->mHeadp->getWorldPosition() +
			LLVector3(0.08f, 0.f, 0.05f) * gAgentAvatarp->mHeadp->getWorldRotation() +
			LLVector3(0.1f, 0.f, 0.f) * gAgentAvatarp->mPelvisp->getWorldRotation();
		LLJoint* torso_joint = gAgentAvatarp->mTorsop;
		LLJoint* chest_joint = gAgentAvatarp->mChestp;
		LLVector3 torso_scale = torso_joint->getScale();
		LLVector3 chest_scale = chest_joint->getScale();
		if (realistic_ml)
		{
			LLQuaternion agent_rot(gAgent.getFrameAgent().getQuaternion());
			if (LLViewerObject* parent = static_cast<LLViewerObject*>(gAgentAvatarp->getParent()))
				if (static_cast<LLViewerObject*>(gAgentAvatarp->getRoot())->flagCameraDecoupled())
					agent_rot *= parent->getRenderRotation();
			LLViewerCamera::getInstance()->updateCameraLocation(head_pos, mCameraUpVector, gAgentAvatarp->mHeadp->getWorldPosition() + LLVector3(1.0, 0.0, 0.0) * agent_rot);
		}
		else
		{
			const LLVector3 diff((mCameraPositionAgent - head_pos) * ~gAgentAvatarp->mRoot->getWorldRotation());
			gAgentAvatarp->mPelvisp->setPosition(gAgentAvatarp->mPelvisp->getPosition() + diff);
		}
		gAgentAvatarp->mRoot->updateWorldMatrixChildren();
		std::vector<std::pair<LLViewerObject*,LLViewerJointAttachment*> >::iterator attachment_iter = gAgentAvatarp->mAttachedObjectsVector.begin();
		for(;attachment_iter!=gAgentAvatarp->mAttachedObjectsVector.end();++attachment_iter)
		{{
				LLViewerObject* attached_object = attachment_iter->first;
				if (attached_object && !attached_object->isDead() && attached_object->mDrawable.notNull())
				{
					attached_object->mDrawable->clearState(LLDrawable::EARLY_MOVE);
					gPipeline.updateMoveNormalAsync(attached_object->mDrawable);
					attached_object->updateText();
				}
			}
		}
		torso_joint->setScale(torso_scale);
		chest_joint->setScale(chest_scale);
	}
}
void LLAgentCamera::updateLastCamera()
{
	mLastCameraMode = mCameraMode;
}
void LLAgentCamera::updateFocusOffset()
{
	validateFocusObject();
	if (mFocusObject.notNull())
	{
		LLVector3d obj_pos = gAgent.getPosGlobalFromAgent(mFocusObject->getRenderPosition());
		mFocusObjectOffset.setVec(mFocusTargetGlobal - obj_pos);
	}
}
void LLAgentCamera::validateFocusObject()
{
	if (mFocusObject.notNull() &&
		mFocusObject->isDead())
	{
		mFocusObjectOffset.clearVec();
		clearFocusObject();
		mCameraFOVZoomFactor = 0.f;
	}
}
F32 LLAgentCamera::calcCustomizeAvatarUIOffset( const LLVector3d& camera_pos_global )
{
	F32 ui_offset = 0.f;
	if( LLFloaterCustomize::instanceExists() && CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode )
	{
		const LLRect& rect = LLFloaterCustomize::getInstance()->getRect();
		F32 fraction_of_fov = 0.5f - (0.5f * (1.f - llmin(1.f, ((F32)rect.getWidth() / (F32)gViewerWindow->getWindowWidth()))));
		F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect();
		F32 offset = tan(apparent_angle);
		if( rect.mLeft < (gViewerWindow->getWindowWidth() - rect.mRight) )
		{
			ui_offset = offset;
		}
		else
		{
			ui_offset = -offset;
		}
	}
	F32 range = (F32)dist_vec(camera_pos_global, gAgentCamera.getFocusGlobal());
	mUIOffset = lerp(mUIOffset, ui_offset, LLSmoothInterpolation::getInterpolant(0.05f));
	return mUIOffset * range;
}
LLVector3d LLAgentCamera::calcFocusPositionTargetGlobal()
{
	if (mFocusObject.notNull() && mFocusObject->isDead())
	{
		clearFocusObject();
	}
	if (mCameraMode == CAMERA_MODE_FOLLOW && mFocusOnAvatar)
	{
		mFocusTargetGlobal = gAgent.getPosGlobalFromAgent(mFollowCam.getSimulatedFocus());
		return mFocusTargetGlobal;
	}
	else if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		LLVector3d at_axis(1.0, 0.0, 0.0);
		LLQuaternion agent_rot = gAgent.getFrameAgent().getQuaternion();
		if (isAgentAvatarValid() && gAgentAvatarp->getParent())
		{
			LLViewerObject* root_object = (LLViewerObject*)gAgentAvatarp->getRoot();
			if (!root_object->flagCameraDecoupled())
			{
				agent_rot *= ((LLViewerObject*)(gAgentAvatarp->getParent()))->getRenderRotation();
			}
		}
		at_axis = at_axis * agent_rot;
		mFocusTargetGlobal = calcCameraPositionTargetGlobal() + at_axis;
		return mFocusTargetGlobal;
	}
	else if (mCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		return mFocusTargetGlobal;
	}
	else if (!mFocusOnAvatar)
	{
		if (mFocusObject.notNull() && !mFocusObject->isDead() && mFocusObject->mDrawable.notNull())
		{
			LLDrawable* drawablep = mFocusObject->mDrawable;
			if (mTrackFocusObject &&
				drawablep &&
				drawablep->isActive())
			{
				if (!mFocusObject->isAvatar())
				{
					if (mFocusObject->isSelected())
					{
						gPipeline.updateMoveNormalAsync(drawablep);
					}
					else if(!drawablep->isState(LLDrawable::ON_MOVE_LIST))
					{
						if (drawablep->isState(LLDrawable::MOVE_UNDAMPED))
						{
							gPipeline.updateMoveNormalAsync(drawablep);
						}
						else
						{
							gPipeline.updateMoveDampedAsync(drawablep);
						}
					}
				}
			}
			else
			{
				updateFocusOffset();
			}
			LLVector3 focus_agent = mFocusObject->getRenderPosition() + mFocusObjectOffset;
			mFocusTargetGlobal.setVec(gAgent.getPosGlobalFromAgent(focus_agent));
		}
		return mFocusTargetGlobal;
	}
	else if (mSitCameraEnabled && isAgentAvatarValid() && gAgentAvatarp->isSitting() && mSitCameraReferenceObject.notNull())
	{
		LLVector3 object_pos = mSitCameraReferenceObject->getRenderPosition();
		LLQuaternion object_rot = mSitCameraReferenceObject->getRenderRotation();
		LLVector3 target_pos = object_pos + (mSitCameraFocus * object_rot);
		return gAgent.getPosGlobalFromAgent(target_pos);
	}
	else
	{
		return gAgent.getPositionGlobal() + calcThirdPersonFocusOffset();
	}
}
LLVector3d LLAgentCamera::calcThirdPersonFocusOffset()
{
	LLVector3d focus_offset;
	LLQuaternion agent_rot = gAgent.getFrameAgent().getQuaternion();
	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		agent_rot *= ((LLViewerObject*)(gAgentAvatarp->getParent()))->getRenderRotation();
	}
	focus_offset = convert_from_llsd<LLVector3d>(mFocusOffsetInitial[mCameraPreset]->get(), TYPE_VEC3D, "");
	return focus_offset * agent_rot;
}
void LLAgentCamera::setupSitCamera()
{
	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		LLQuaternion parent_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
		LLVector3 at_axis = gAgent.getFrameAgent().getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis * ~parent_rot);
	}
}
const LLVector3 &LLAgentCamera::getCameraPositionAgent() const
{
	return LLViewerCamera::getInstance()->getOrigin();
}
LLVector3d LLAgentCamera::getCameraPositionGlobal() const
{
	return gAgent.getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin());
}
F32	LLAgentCamera::calcCameraFOVZoomFactor()
{
	LLVector3 camera_offset_dir;
	camera_offset_dir.setVec(mCameraFocusOffset);
	if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		return 0.f;
	}
	else if (mFocusObject.notNull() && !mFocusObject->isAvatar() && !mFocusOnAvatar)
	{
		F32 obj_min_dist = 0.f;
		static const LLCachedControl<bool> ascent_disable_min_zoom_dist("AscentDisableMinZoomDist");
		if (!ascent_disable_min_zoom_dist)
			calcCameraMinDistance(obj_min_dist);
		F32 current_distance = llmax(0.001f, camera_offset_dir.magVec());
		mFocusObjectDist = obj_min_dist - current_distance;
		F32 new_fov_zoom = llclamp(mFocusObjectDist / current_distance, 0.f, 1000.f);
		return new_fov_zoom;
	}
	else
	{
		return mCameraFOVZoomFactor;
	}
}
LLVector3d LLAgentCamera::calcCameraPositionTargetGlobal(BOOL *hit_limit)
{
	F32			camera_land_height;
	LLVector3d	frame_center_global = !isAgentAvatarValid() ?
		gAgent.getPositionGlobal() :
		gAgent.getPosGlobalFromAgent(gAgentAvatarp->mRoot->getWorldPosition());
	BOOL		isConstrained = FALSE;
	LLVector3d	head_offset;
	head_offset.setVec(mThirdPersonHeadOffset);
	LLVector3d camera_position_global;
	if (mCameraMode == CAMERA_MODE_FOLLOW && mFocusOnAvatar)
	{
		camera_position_global = gAgent.getPosGlobalFromAgent(mFollowCam.getSimulatedPosition());
	}
	else if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		if (!isAgentAvatarValid() || gAgentAvatarp->mDrawable.isNull())
		{
			LL_WARNS() << "Null avatar drawable!" << LL_ENDL;
			return LLVector3d::zero;
		}
		head_offset.clearVec();
		if (gAgentAvatarp->isSitting() && gAgentAvatarp->getParent())
		{
			gAgentAvatarp->updateHeadOffset();
			head_offset.mdV[VX] = gAgentAvatarp->mHeadOffset.mV[VX];
			head_offset.mdV[VY] = gAgentAvatarp->mHeadOffset.mV[VY];
			head_offset.mdV[VZ] = gAgentAvatarp->mHeadOffset.mV[VZ] + 0.1f;
			const LLMatrix4 mat(((LLViewerObject*) gAgentAvatarp->getParent())->getRenderMatrix().getF32ptr());
			camera_position_global = gAgent.getPosGlobalFromAgent
								((gAgentAvatarp->getPosition()+
								 LLVector3(head_offset)*gAgentAvatarp->getRotation()) * mat);
		}
		else
		{
			head_offset.mdV[VZ] = gAgentAvatarp->mHeadOffset.mV[VZ];
			if (gAgentAvatarp->isSitting())
			{
				head_offset.mdV[VZ] += 0.1;
			}
			camera_position_global = gAgent.getPosGlobalFromAgent(gAgentAvatarp->getRenderPosition());
			head_offset = head_offset * gAgentAvatarp->getRenderRotation();
			camera_position_global = camera_position_global + head_offset;
		}
	}
	else if (mCameraMode == CAMERA_MODE_THIRD_PERSON && mFocusOnAvatar)
	{
		LLVector3 local_camera_offset;
		F32 camera_distance = 0.f;
		if (mSitCameraEnabled
			&& isAgentAvatarValid()
			&& gAgentAvatarp->isSitting()
			&& mSitCameraReferenceObject.notNull())
		{
			LLVector3 object_pos = mSitCameraReferenceObject->getRenderPosition();
			LLQuaternion object_rot = mSitCameraReferenceObject->getRenderRotation();
			LLVector3 target_pos = object_pos + (mSitCameraPos * object_rot);
			camera_position_global = gAgent.getPosGlobalFromAgent(target_pos);
		}
		else
		{
			static const LLCachedControl<F32> camera_offset_scale("CameraOffsetScale");
			local_camera_offset = mCameraZoomFraction * getCameraOffsetInitial() * camera_offset_scale;
			if (isAgentAvatarValid() && gAgentAvatarp->getParent())
			{
				LLQuaternion parent_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
				LLVector3 at_axis = gAgent.getFrameAgent().getAtAxis() * parent_rot;
				at_axis.mV[VZ] = 0.f;
				at_axis.normalize();
				gAgent.resetAxes(at_axis * ~parent_rot);
				local_camera_offset = local_camera_offset * gAgent.getFrameAgent().getQuaternion() * parent_rot;
			}
			else
			{
				local_camera_offset = gAgent.getFrameAgent().rotateToAbsolute( local_camera_offset );
			}
			static const LLCachedControl<bool> sg_ignore_sim_cam_consts("SGIgnoreSimulatorCameraConstraints",false);
			if ( !mCameraCollidePlane.isExactlyZero()
				 && (!sg_ignore_sim_cam_consts || gRlvHandler.hasBehaviour(RLV_BHVR_CAMUNLOCK))
				 && isAgentAvatarValid()
				 && !gAgentAvatarp->isSitting())
			{
				LLVector3 plane_normal;
				plane_normal.setVec(mCameraCollidePlane.mV);
				F32 offset_dot_norm = local_camera_offset * plane_normal;
				if (llabs(offset_dot_norm) < 0.001f)
				{
					offset_dot_norm = 0.001f;
				}
				camera_distance = local_camera_offset.normalize();
				F32 pos_dot_norm = gAgent.getPosAgentFromGlobal(frame_center_global + head_offset) * plane_normal;
				if (pos_dot_norm > mCameraCollidePlane.mV[VW])
				{
					if (offset_dot_norm + pos_dot_norm < mCameraCollidePlane.mV[VW])
					{
						camera_distance *= (pos_dot_norm - mCameraCollidePlane.mV[VW] - CAMERA_COLLIDE_EPSILON) / -offset_dot_norm;
					}
				}
				else
				{
					if (offset_dot_norm + pos_dot_norm > mCameraCollidePlane.mV[VW])
					{
						camera_distance *= (mCameraCollidePlane.mV[VW] - pos_dot_norm - CAMERA_COLLIDE_EPSILON) / offset_dot_norm;
					}
				}
			}
			else
			{
				camera_distance = local_camera_offset.normalize();
			}
			mTargetCameraDistance = llmax(camera_distance, MIN_CAMERA_DISTANCE);
			if (mTargetCameraDistance != mCurrentCameraDistance)
			{
				F32 camera_lerp_amt = LLSmoothInterpolation::getInterpolant(CAMERA_ZOOM_HALF_LIFE);
				mCurrentCameraDistance = lerp(mCurrentCameraDistance, mTargetCameraDistance, camera_lerp_amt);
			}
			local_camera_offset *= mCurrentCameraDistance;
			LLVector3d camera_offset;
			camera_offset.setVec( local_camera_offset );
			camera_position_global = frame_center_global + head_offset + camera_offset;
			if (isAgentAvatarValid())
			{
				LLVector3d camera_lag_d;
				F32 lag_interp = LLSmoothInterpolation::getInterpolant(CAMERA_LAG_HALF_LIFE);
				LLVector3 target_lag;
				LLVector3 vel = gAgent.getVelocity();
				F32 time_in_air = gAgentAvatarp->mTimeInAir.getElapsedTimeF32();
				if(!mCameraAnimating && gAgentAvatarp->mInAir && time_in_air > GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME)
				{
					LLVector3 frame_at_axis = gAgent.getFrameAgent().getAtAxis();
					frame_at_axis -= projected_vec(frame_at_axis, gAgent.getReferenceUpVector());
					frame_at_axis.normalize();
					F32 u = (time_in_air - GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME) / GROUND_TO_AIR_CAMERA_TRANSITION_TIME;
					u = llclamp(u, 0.f, 1.f);
					lag_interp *= u;
					if (gViewerWindow->getLeftMouseDown() && gViewerWindow->getLastPick().mObjectID == gAgentAvatarp->getID())
					{
						target_lag.clearVec();
					}
					else
					{
						static const LLCachedControl<F32> dynamic_camera_strength("DynamicCameraStrength");
						target_lag = vel * dynamic_camera_strength / 30.f;
					}
					mCameraLag = lerp(mCameraLag, target_lag, lag_interp);
					F32 lag_dist = mCameraLag.magVec();
					if (lag_dist > MAX_CAMERA_LAG)
					{
						mCameraLag = mCameraLag * MAX_CAMERA_LAG / lag_dist;
					}
					F32 dot = (mCameraLag - (frame_at_axis * (MIN_CAMERA_LAG * u))) * frame_at_axis;
					if (dot < -(MIN_CAMERA_LAG * u))
					{
						mCameraLag -= (dot + (MIN_CAMERA_LAG * u)) * frame_at_axis;
					}
				}
				else
				{
					mCameraLag = lerp(mCameraLag, LLVector3::zero, LLSmoothInterpolation::getInterpolant(0.15f));
				}
				camera_lag_d.setVec(mCameraLag);
				camera_position_global = camera_position_global - camera_lag_d;
			}
		}
	}
	else
	{
		LLVector3d focusPosGlobal = calcFocusPositionTargetGlobal();
		camera_position_global = focusPosGlobal + mCameraFocusOffset;
	}
	static const LLCachedControl<bool> disable_camera_constraints("DisableCameraConstraints");
	if (!disable_camera_constraints && !gAgent.isGodlike())
	{
		LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosGlobal(camera_position_global);
		bool constrain = true;
		if(regionp && regionp->canManageEstate())
		{
			constrain = false;
		}
		if(constrain)
		{
			F32 max_dist = (CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode) ? APPEARANCE_MAX_ZOOM : mDrawDistance;
			LLVector3d camera_offset = camera_position_global - gAgent.getPositionGlobal();
			F32 camera_distance = (F32)camera_offset.magVec();
			if(camera_distance > max_dist)
			{
				camera_position_global = gAgent.getPositionGlobal() + (max_dist/camera_distance)*camera_offset;
				isConstrained = TRUE;
			}
		}
	}
	if (rlv_handler_t::isEnabled() && !cameraMouselook())
	{
		const LLVector3d agent_pos(gAgent.getPositionGlobal());
		const LLVector3d offset(camera_position_global - agent_pos);
		const F32 total_dist(offset.magVec());
		bool rlvConstrained = false;
		if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMAX))
		{
			F32 max_dist(gRlvHandler.camPole(RLV_BHVR_CAMDISTMAX));
			if (total_dist > max_dist)
			{
				camera_position_global = agent_pos + (max_dist/total_dist)*offset;
				rlvConstrained = isConstrained = true;
			}
		}
		if (!rlvConstrained && gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMIN))
		{
			F32 min_dist(gRlvHandler.camPole(RLV_BHVR_CAMDISTMIN));
			if (total_dist < min_dist)
			{
				camera_position_global = agent_pos + (min_dist/total_dist)*offset;
				rlvConstrained = isConstrained = true;
			}
		}
	}
	F32 camera_min_off_ground = getCameraMinOffGround();
	camera_land_height = LLWorld::getInstance()->resolveLandHeightGlobal(camera_position_global);
	if (camera_position_global.mdV[VZ] < camera_land_height + camera_min_off_ground)
	{
		camera_position_global.mdV[VZ] = camera_land_height + camera_min_off_ground;
		isConstrained = TRUE;
	}
	if (hit_limit)
	{
		*hit_limit = isConstrained;
	}
	return camera_position_global;
}
LLVector3 LLAgentCamera::getCameraOffsetInitial()
{
	return convert_from_llsd<LLVector3>(mCameraOffsetInitial[mCameraPreset]->get(), TYPE_VEC3, "");
}
template <typename Vec, typename T>
void change_vec(const T& change, LLCachedControl<Vec>& vec, const U32& idx = VZ)
{
	Vec changed(vec);
	changed[idx] += change;
	vec = changed;
}
template <typename Vec, typename T>
void change_vec(const T& change, LLPointer<LLControlVariable>& vec, const U32& idx = VZ)
{
	Vec changed(vec->get());
	changed[idx] += change;
	vec->set(changed.getValue());
}
void LLAgentCamera::handleScrollWheel(S32 clicks)
{
	if (mCameraMode == CAMERA_MODE_FOLLOW && getFocusOnAvatar())
	{
		if (!mFollowCam.getPositionLocked())
		{
			mFollowCam.zoom(clicks);
			if (mFollowCam.isZoomedToMinimumDistance())
			{
				changeCameraToMouselook(FALSE);
			}
		}
	}
	else
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
		const F32 ROOT_ROOT_TWO = sqrt(F_SQRT2);
		if (mCameraAnimating)
		{
			return;
		}
		if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
		{
			F32 zoom_factor = (F32)pow(0.8, -clicks);
			cameraZoomIn(zoom_factor);
		}
		else if (mFocusOnAvatar && (mCameraMode == CAMERA_MODE_THIRD_PERSON))
		{
			if (MASK mask = gKeyboard->currentMask(true))
			{
				static const LLCachedControl<bool> enableCameraOffsetScroll("SinguOffsetScrollKeys");
				if (mask & (MASK_CONTROL|MASK_SHIFT) && enableCameraOffsetScroll)
				{
					const F32 change(static_cast<F32>(clicks) * 0.1f);
					if (mask & MASK_SHIFT)
					{
						change_vec<LLVector3d>(change, mFocusOffsetInitial[mCameraPreset]);
					}
					if (mask & MASK_CONTROL)
					{
						change_vec<LLVector3>(change, mCameraOffsetInitial[mCameraPreset]);
					}
					return;
				}
			}
			F32 camera_offset_initial_mag = getCameraOffsetInitial().magVec();
			static const LLCachedControl<F32> camera_offset_scale("CameraOffsetScale");
			F32 current_zoom_fraction = mTargetCameraDistance / (camera_offset_initial_mag * camera_offset_scale);
			current_zoom_fraction *= 1.f - pow(ROOT_ROOT_TWO, clicks);
			cameraOrbitIn(current_zoom_fraction * camera_offset_initial_mag * camera_offset_scale);
		}
		else
		{
			F32 current_zoom_fraction = (F32)mCameraFocusOffsetTarget.magVec();
			cameraOrbitIn(current_zoom_fraction * (1.f - pow(ROOT_ROOT_TWO, clicks)));
		}
	}
}
void LLAgentCamera::resetPresetOffsets()
{
	mFocusOffsetInitial[mCameraPreset]->resetToDefault();
	mCameraOffsetInitial[mCameraPreset]->resetToDefault();
}
F32 LLAgentCamera::getCameraMinOffGround()
{
	if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		return 0.f;
	}
	else
	{
		static const LLCachedControl<bool> disable_camera_constraints("DisableCameraConstraints");
		if (disable_camera_constraints)
		{
			return -1000.f;
		}
		else
		{
			return 0.5f;
		}
	}
}
void LLAgentCamera::resetCamera()
{
	LLVector3 at = gAgent.getFrameAgent().getAtAxis();
	at.mV[VZ] = 0.f;
	at.normalize();
	gAgent.resetAxes(at);
	mCameraFOVZoomFactor = 0.f;
	updateCamera();
}
void LLAgentCamera::changeCameraToMouselook(BOOL animate)
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMIN) && gRlvHandler.camPole(RLV_BHVR_CAMDISTMIN) > 0) return;
	if (!gSavedSettings.getBOOL("EnableMouselook")
		|| LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}
	gViewerWindow->getWindow()->resetBusyCount();
	LLMenuGL::sMenuContainer->hideMenus();
	gAgent.unpauseAnimation();
	LLToolMgr::getInstance()->setCurrentToolset(gMouselookToolset);
	gSavedSettings.setBOOL("FirstPersonBtnState",	FALSE);
	gSavedSettings.setBOOL("MouselookBtnState",		TRUE);
	gSavedSettings.setBOOL("ThirdPersonBtnState",	FALSE);
	gSavedSettings.setBOOL("BuildBtnState",			FALSE);
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->stopMotion(ANIM_AGENT_BODY_NOISE);
		gAgentAvatarp->stopMotion(ANIM_AGENT_BREATHE_ROT);
	}
	LLSelectMgr::getInstance()->deselectAll();
	if (mCameraMode != CAMERA_MODE_MOUSELOOK)
	{
		mMouselookTimer.reset();
		gFocusMgr.setKeyboardFocus(NULL);
		auto ao = AOSystem::getIfExists();
		if (ao && gSavedSettings.getBOOL("AONoStandsInMouselook")) ao->stopCurrentStand();
		updateLastCamera();
		mCameraMode = CAMERA_MODE_MOUSELOOK;
		const U32 old_flags = gAgent.getControlFlags();
		gAgent.setControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (old_flags != gAgent.getControlFlags())
		{
			gAgent.setFlagsDirty();
		}
		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			mCameraAnimating = FALSE;
			gAgent.endAnimationUpdateUI();
		}
	}
}
void LLAgentCamera::changeCameraToDefault()
{
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}
	if (LLFollowCamMgr::getActiveFollowCamParams())
	{
		changeCameraToFollow();
	}
	else
	{
		changeCameraToThirdPerson();
	}
}
void LLAgentCamera::changeCameraToFollow(BOOL animate)
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMAX) && 0 >= gRlvHandler.camPole(RLV_BHVR_CAMDISTMAX)) return;
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}
	if(mCameraMode != CAMERA_MODE_FOLLOW)
	{
		if (mCameraMode == CAMERA_MODE_MOUSELOOK)
		{
			animate = FALSE;
			if (gViewerWindow->getRightMouseDown()) LLViewerCamera::instance().loadDefaultFOV();
		}
		startCameraAnimation();
		updateLastCamera();
		mCameraMode = CAMERA_MODE_FOLLOW;
		mFollowCam.reset(mCameraPositionAgent, LLViewerCamera::getInstance()->getPointOfInterest(), LLVector3::z_axis);
		if (gBasicToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		}
		if (isAgentAvatarValid())
		{
			gAgentAvatarp->mPelvisp->setPosition(LLVector3::zero);
			gAgentAvatarp->startMotion( ANIM_AGENT_BODY_NOISE );
			gAgentAvatarp->startMotion( ANIM_AGENT_BREATHE_ROT );
		}
		gSavedSettings.setBOOL("FirstPersonBtnState",	FALSE);
		gSavedSettings.setBOOL("MouselookBtnState",		FALSE);
		gSavedSettings.setBOOL("ThirdPersonBtnState",	TRUE);
		gSavedSettings.setBOOL("BuildBtnState",			FALSE);
		gAgent.unpauseAnimation();
		gAgent.clearControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			mCameraAnimating = FALSE;
			gAgent.endAnimationUpdateUI();
		}
	}
}
void LLAgentCamera::changeCameraToThirdPerson(BOOL animate)
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMAX) && 0 >= gRlvHandler.camPole(RLV_BHVR_CAMDISTMAX)) return;
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}
	gViewerWindow->getWindow()->resetBusyCount();
	mCameraZoomFraction = INITIAL_ZOOM_FRACTION;
	if (isAgentAvatarValid())
	{
		if (!gAgentAvatarp->isSitting())
		{
			gAgentAvatarp->mPelvisp->setPosition(LLVector3::zero);
		}
		gAgentAvatarp->startMotion(ANIM_AGENT_BODY_NOISE);
		gAgentAvatarp->startMotion(ANIM_AGENT_BREATHE_ROT);
	}
	gSavedSettings.setBOOL("FirstPersonBtnState",	FALSE);
	gSavedSettings.setBOOL("MouselookBtnState",		FALSE);
	gSavedSettings.setBOOL("ThirdPersonBtnState",	TRUE);
	gSavedSettings.setBOOL("BuildBtnState",			FALSE);
	LLVector3 at_axis;
	gAgent.unpauseAnimation();
	if (mCameraMode != CAMERA_MODE_THIRD_PERSON)
	{
		if (gBasicToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		}
		mCameraLag.clearVec();
		if (mCameraMode == CAMERA_MODE_MOUSELOOK)
		{
			mCurrentCameraDistance = MIN_CAMERA_DISTANCE;
			mTargetCameraDistance = MIN_CAMERA_DISTANCE;
			animate = FALSE;
			if (gViewerWindow->getRightMouseDown()) LLViewerCamera::instance().loadDefaultFOV();
		}
		updateLastCamera();
		mCameraMode = CAMERA_MODE_THIRD_PERSON;
		gAgent.clearControlFlags(AGENT_CONTROL_MOUSELOOK);
	}
	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		LLQuaternion obj_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
		at_axis = LLViewerCamera::getInstance()->getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis * ~obj_rot);
	}
	else
	{
		at_axis = gAgent.getFrameAgent().getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis);
	}
	if (animate)
	{
		startCameraAnimation();
	}
	else
	{
		mCameraAnimating = FALSE;
		gAgent.endAnimationUpdateUI();
	}
}
void LLAgentCamera::changeCameraToCustomizeAvatar()
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMAX) && 0 >= gRlvHandler.camPole(RLV_BHVR_CAMDISTMAX)) return;
	if (LLViewerJoystick::getInstance()->getOverrideCamera() || !isAgentAvatarValid())
	{
		return;
	}
	gViewerWindow->getWindow()->resetBusyCount();
	if (gFaceEditToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(gFaceEditToolset);
	}
	gSavedSettings.setBOOL("FirstPersonBtnState", FALSE);
	gSavedSettings.setBOOL("MouselookBtnState", FALSE);
	gSavedSettings.setBOOL("ThirdPersonBtnState", FALSE);
	gSavedSettings.setBOOL("BuildBtnState", FALSE);
	if(gSavedSettings.getBOOL("AppearanceCameraMovement"))
	startCameraAnimation();
	if (mCameraMode != CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		updateLastCamera();
		mCameraMode = CAMERA_MODE_CUSTOMIZE_AVATAR;
		U32 old_flags = gAgent.getControlFlags();
		gAgent.clearControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (old_flags != gAgent.getControlFlags())
		{
			gAgent.setFlagsDirty();
		}
		gFocusMgr.setKeyboardFocus( NULL );
		gFocusMgr.setMouseCapture( NULL );
		LLVector3 at = gAgent.getAtAxis();
		at.mV[VZ] = 0.f;
		at.normalize();
		gAgent.resetAxes(at);
		if (gSavedSettings.getBOOL("LiruCustomizeAnim"))
		{
			gAgent.sendAnimationRequest(ANIM_AGENT_CUSTOMIZE, ANIM_REQUEST_START);
			gAgent.setCustomAnim(TRUE);
			gAgentAvatarp->startMotion(ANIM_AGENT_CUSTOMIZE);
			LLMotion* turn_motion = gAgentAvatarp->findMotion(ANIM_AGENT_CUSTOMIZE);
			if (turn_motion)
			{
				setAnimationDuration(turn_motion->getDuration() + CUSTOMIZE_AVATAR_CAMERA_ANIM_SLOP);
			}
		}
	}
	if(!gSavedSettings.getBOOL("AppearanceCameraMovement"))
	{
		mCameraAnimating = FALSE;
		gAgent.endAnimationUpdateUI();
		return;
	}
	LLVector3 agent_at = gAgent.getAtAxis();
	agent_at.mV[VZ] = 0.f;
	agent_at.normalize();
	LLVector3 focus_target = isAgentAvatarValid()
		? gAgentAvatarp->mHeadp->getWorldPosition()
		: gAgent.getPositionAgent();
	LLVector3d camera_offset(agent_at * -1.0);
	camera_offset.mdV[VZ] = 0.1f;
	camera_offset *= CUSTOMIZE_AVATAR_CAMERA_DEFAULT_DIST;
	LLVector3d focus_target_global = gAgent.getPosGlobalFromAgent(focus_target);
	setAnimationDuration(gSavedSettings.getF32("ZoomTime"));
	setCameraPosAndFocusGlobal(focus_target_global + camera_offset, focus_target_global, gAgent.getID());
}
void LLAgentCamera::switchCameraPreset(ECameraPreset preset)
{
	mCameraZoomFraction = 1.f;
	mFocusOnAvatar = TRUE;
	mCameraPreset = preset;
	gSavedSettings.setU32("CameraPreset", mCameraPreset);
}
void LLAgentCamera::setAnimationDuration(F32 duration)
{
	if (mCameraAnimating)
	{
		F32 animation_left = llmax(0.f, mAnimationDuration - mAnimationTimer.getElapsedTimeF32());
		mAnimationDuration = llmax(duration, animation_left);
	}
	else
	{
		mAnimationDuration = duration;
	}
}
void LLAgentCamera::startCameraAnimation()
{
	mAnimationCameraStartGlobal = getCameraPositionGlobal();
	mAnimationFocusStartGlobal = mFocusGlobal;
	setAnimationDuration(gSavedSettings.getF32("ZoomTime"));
	mAnimationTimer.reset();
	mCameraAnimating = TRUE;
}
void LLAgentCamera::stopCameraAnimation()
{
	mCameraAnimating = FALSE;
}
void LLAgentCamera::clearFocusObject()
{
	if (mFocusObject.notNull())
	{
		startCameraAnimation();
		setFocusObject(NULL);
		mFocusObjectOffset.clearVec();
	}
}
void LLAgentCamera::setFocusObject(LLViewerObject* object)
{
	mFocusObject = object;
}
void LLAgentCamera::setFocusGlobal(const LLPickInfo& pick)
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	LLViewerObject* objectp = gObjectList.findObject(pick.mObjectID);
	if (objectp && !freeze_time)
	{
		setFocusGlobal(objectp->getPositionGlobal() + LLVector3d(pick.mObjectOffset), pick.mObjectID);
	}
	else
	{
		setFocusGlobal(pick.mPosGlobal, pick.mObjectID);
	}
}
void LLAgentCamera::setFocusGlobal(const LLVector3d& focus, const LLUUID &object_id)
{
	setFocusObject(gObjectList.findObject(object_id));
	LLVector3d old_focus = mFocusTargetGlobal;
	LLViewerObject *focus_obj = mFocusObject;
	if (old_focus != focus)
	{
		if (focus.isExactlyZero())
		{
			if (isAgentAvatarValid())
			{
				mFocusTargetGlobal = gAgent.getPosGlobalFromAgent(gAgentAvatarp->mHeadp->getWorldPosition());
			}
			else
			{
				mFocusTargetGlobal = gAgent.getPositionGlobal();
			}
			mCameraFocusOffsetTarget = getCameraPositionGlobal() - mFocusTargetGlobal;
			mCameraFocusOffset = mCameraFocusOffsetTarget;
			setLookAt(LOOKAT_TARGET_CLEAR);
		}
		else
		{
			mFocusTargetGlobal = focus;
			if (!focus_obj)
			{
				mCameraFOVZoomFactor = 0.f;
			}
			mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(mCameraVirtualPositionAgent) - mFocusTargetGlobal;
			startCameraAnimation();
			if (focus_obj)
			{
				if (focus_obj->isAvatar())
				{
					setLookAt(LOOKAT_TARGET_FOCUS, focus_obj);
				}
				else
				{
					setLookAt(LOOKAT_TARGET_FOCUS, focus_obj, (gAgent.getPosAgentFromGlobal(focus) - focus_obj->getRenderPosition()) * ~focus_obj->getRenderRotation());
				}
			}
			else
			{
				setLookAt(LOOKAT_TARGET_FOCUS, NULL, gAgent.getPosAgentFromGlobal(mFocusTargetGlobal));
			}
		}
	}
	else
	{
		if (focus.isExactlyZero())
		{
			if (isAgentAvatarValid())
			{
				mFocusTargetGlobal = gAgent.getPosGlobalFromAgent(gAgentAvatarp->mHeadp->getWorldPosition());
			}
			else
			{
				mFocusTargetGlobal = gAgent.getPositionGlobal();
			}
		}
		mCameraFocusOffsetTarget = (getCameraPositionGlobal() - mFocusTargetGlobal) / (1.f + mCameraFOVZoomFactor);;
		mCameraFocusOffset = mCameraFocusOffsetTarget;
	}
	if (mFocusObject.notNull())
	{
		if (mFocusObject->isAttachment())
		{
			while (mFocusObject.notNull() && !mFocusObject->isAvatar())
			{
				mFocusObject = (LLViewerObject*) mFocusObject->getParent();
			}
			setFocusObject((LLViewerObject*)mFocusObject);
		}
		updateFocusOffset();
	}
}
void LLAgentCamera::setCameraPosAndFocusGlobal(const LLVector3d& camera_pos, const LLVector3d& focus, const LLUUID &object_id)
{
	LLVector3d old_focus = mFocusTargetGlobal.isExactlyZero() ? focus : mFocusTargetGlobal;
	F64 focus_delta_squared = (old_focus - focus).magVecSquared();
	const F64 ANIM_EPSILON_SQUARED = 0.0001;
	if (focus_delta_squared > ANIM_EPSILON_SQUARED)
	{
		startCameraAnimation();
		if( CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode )
		{
			mAnimationCameraStartGlobal -= LLVector3d(LLViewerCamera::getInstance()->getLeftAxis() * calcCustomizeAvatarUIOffset( mAnimationCameraStartGlobal ));
		}
	}
	setFocusObject(gObjectList.findObject(object_id));
	mFocusTargetGlobal = focus;
	mCameraFocusOffsetTarget = camera_pos - focus;
	mCameraFocusOffset = mCameraFocusOffsetTarget;
	if (mFocusObject)
	{
		if (mFocusObject->isAvatar())
		{
			setLookAt(LOOKAT_TARGET_FOCUS, mFocusObject);
		}
		else
		{
			setLookAt(LOOKAT_TARGET_FOCUS, mFocusObject, (gAgent.getPosAgentFromGlobal(focus) - mFocusObject->getRenderPosition()) * ~mFocusObject->getRenderRotation());
		}
	}
	else
	{
		setLookAt(LOOKAT_TARGET_FOCUS, NULL, gAgent.getPosAgentFromGlobal(mFocusTargetGlobal));
	}
	if (mCameraAnimating)
	{
		const F64 ANIM_METERS_PER_SECOND = 10.0;
		const F64 MIN_ANIM_SECONDS = 0.5;
		const F64 MAX_ANIM_SECONDS = 10.0;
		F64 anim_duration = llmax( MIN_ANIM_SECONDS, sqrt(focus_delta_squared) / ANIM_METERS_PER_SECOND );
		anim_duration = llmin( anim_duration, MAX_ANIM_SECONDS );
		setAnimationDuration( (F32)anim_duration );
	}
	updateFocusOffset();
}
void LLAgentCamera::setSitCamera(const LLUUID &object_id, const LLVector3 &camera_pos, const LLVector3 &camera_focus)
{
	BOOL camera_enabled = !object_id.isNull();
	if (camera_enabled)
	{
		LLViewerObject *reference_object = gObjectList.findObject(object_id);
		if (reference_object)
		{
			mSitCameraPos = camera_pos;
			mSitCameraFocus = camera_focus;
			mSitCameraReferenceObject = reference_object;
			mSitCameraEnabled = TRUE;
		}
	}
	else
	{
		mSitCameraPos.clearVec();
		mSitCameraFocus.clearVec();
		mSitCameraReferenceObject = NULL;
		mSitCameraEnabled = FALSE;
	}
}
void LLAgentCamera::setFocusOnAvatar(BOOL focus_on_avatar, BOOL animate)
{
	if (focus_on_avatar != mFocusOnAvatar)
	{
		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			stopCameraAnimation();
		}
	}
	if (!mFocusOnAvatar && focus_on_avatar)
	{
		setFocusGlobal(LLVector3d::zero);
		mCameraFOVZoomFactor = 0.f;
		if (mCameraMode == CAMERA_MODE_THIRD_PERSON)
		{
			LLVector3 at_axis;
			if (isAgentAvatarValid() && gAgentAvatarp->getParent())
			{
				LLQuaternion obj_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
				at_axis = LLViewerCamera::getInstance()->getAtAxis();
				at_axis.mV[VZ] = 0.f;
				at_axis.normalize();
				gAgent.resetAxes(at_axis * ~obj_rot);
			}
			else
			{
				at_axis = LLViewerCamera::getInstance()->getAtAxis();
				at_axis.mV[VZ] = 0.f;
				at_axis.normalize();
				gAgent.resetAxes(at_axis);
			}
		}
	}
	else if (mFocusOnAvatar && !focus_on_avatar)
	{
		setFocusGlobal(gAgent.getPositionGlobal() + calcThirdPersonFocusOffset(), gAgent.getID());
		mAllowChangeToFollow = FALSE;
	}
	mFocusOnAvatar = focus_on_avatar;
}
BOOL LLAgentCamera::setLookAt(ELookAtType target_type, LLViewerObject *object, LLVector3 position)
{
	if(gSavedSettings.getBOOL("PrivateLookAt"))
	{
		if(!mLookAt || mLookAt->isDead())
			return FALSE;
		position.clearVec();
		return mLookAt->setLookAt(LOOKAT_TARGET_NONE, gAgentAvatarp, position);
	}
	if(object && object->isAttachment())
	{
		LLViewerObject* parent = object;
		while(parent)
		{
			if (parent == gAgentAvatarp)
			{
				object = gAgentAvatarp;
				position.clearVec();
			}
			parent = (LLViewerObject*)parent->getParent();
		}
	}
	if(!mLookAt || mLookAt->isDead())
	{
		mLookAt = (LLHUDEffectLookAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_LOOKAT);
		mLookAt->setSourceObject(gAgentAvatarp);
	}
	return mLookAt->setLookAt(target_type, object, position);
}
void LLAgentCamera::lookAtLastChat()
{
	lookAtObject(gAgent.getLastChatter());
}
bool LLAgentCamera::lookAtObject(const LLUUID &object_id, bool self)
{
	if (mCameraAnimating || !cameraThirdPerson())
	{
		return false;
	}
	LLViewerObject *chatter = gObjectList.findObject(object_id);
	if (!chatter)
	{
		return false;
	}
	LLVector3 delta_pos;
	F32 radius = chatter->getVObjRadius();
	if (chatter->isAvatar())
	{
		LLVOAvatar *chatter_av = (LLVOAvatar*)chatter;
		if (isAgentAvatarValid() && chatter_av->mHeadp)
		{
			delta_pos = chatter_av->mHeadp->getWorldPosition() - gAgentAvatarp->mHeadp->getWorldPosition();
		}
		else
		{
			delta_pos = chatter->getPositionAgent() - gAgent.getPositionAgent();
		}
		delta_pos.normalize();
		gAgent.setControlFlags(AGENT_CONTROL_STOP);
		changeCameraToThirdPerson();
		LLVector3 new_camera_pos = gAgentAvatarp->mHeadp->getWorldPosition();
		LLVector3 left = delta_pos % LLVector3::z_axis;
		left.normalize();
		LLVector3 up = left % delta_pos;
		up.normalize();
		new_camera_pos -= delta_pos * 0.4f;
		new_camera_pos += left * 0.3f;
		new_camera_pos += up * 0.2f;
		setFocusOnAvatar(FALSE, FALSE);
		if (chatter_av->mHeadp)
		{
			setFocusGlobal(gAgent.getPosGlobalFromAgent(chatter_av->mHeadp->getWorldPosition()), object_id);
			if(self)
				mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(new_camera_pos) - gAgent.getPosGlobalFromAgent(chatter_av->mHeadp->getWorldPosition());
			else
				mCameraFocusOffsetTarget.setVec(radius, radius, 0.f);
		}
		else
		{
			setFocusGlobal(chatter->getPositionGlobal(), object_id);
			if(self)
				mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(new_camera_pos) - chatter->getPositionGlobal();
			else
				mCameraFocusOffsetTarget.setVec(radius, radius, 0.f);
		}
	}
	else
	{
		delta_pos = chatter->getRenderPosition() - gAgent.getPositionAgent();
		delta_pos.normalize();
		gAgent.setControlFlags(AGENT_CONTROL_STOP);
		changeCameraToThirdPerson();
		LLVector3 new_camera_pos = gAgentAvatarp->mHeadp->getWorldPosition();
		LLVector3 left = delta_pos % LLVector3::z_axis;
		left.normalize();
		LLVector3 up = left % delta_pos;
		up.normalize();
		new_camera_pos -= delta_pos * 0.4f;
		new_camera_pos += left * 0.3f;
		new_camera_pos += up * 0.2f;
		setFocusOnAvatar(FALSE, FALSE);
		setFocusGlobal(chatter->getPositionGlobal(), object_id);
		if(self)
			mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(new_camera_pos) - chatter->getPositionGlobal();
		else
			mCameraFocusOffsetTarget.setVec(radius, radius, 0.f);
	}
	return true;
}
bool LLAgentCamera::isfollowCamLocked()
{
	return mFollowCam.getPositionLocked();
}
BOOL LLAgentCamera::setPointAt(EPointAtType target_type, LLViewerObject *object, LLVector3 position)
{
	if ((object && (object->isAttachment() || object->isAvatar())) || gSavedSettings.getBOOL("DisablePointAtAndBeam"))
	{
		return FALSE;
	}
	if (!mPointAt || mPointAt->isDead())
	{
		mPointAt = (LLHUDEffectPointAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINTAT);
		mPointAt->setSourceObject(gAgentAvatarp);
	}
	return mPointAt->setPointAt(target_type, object, position);
}
ELookAtType LLAgentCamera::getLookAtType()
{
	if (mLookAt)
	{
		return mLookAt->getLookAtType();
	}
	return LOOKAT_TARGET_NONE;
}
EPointAtType LLAgentCamera::getPointAtType()
{
	if (mPointAt)
	{
		return mPointAt->getPointAtType();
	}
	return POINTAT_TARGET_NONE;
}
void LLAgentCamera::clearGeneralKeys()
{
	mAtKey 				= 0;
	mWalkKey 			= 0;
	mLeftKey 			= 0;
	mUpKey 				= 0;
	mYawKey 			= 0.f;
	mPitchKey 			= 0.f;
}
void LLAgentCamera::clearOrbitKeys()
{
	mOrbitLeftKey		= 0.f;
	mOrbitRightKey		= 0.f;
	mOrbitUpKey			= 0.f;
	mOrbitDownKey		= 0.f;
	mOrbitInKey			= 0.f;
	mOrbitOutKey		= 0.f;
}
void LLAgentCamera::clearPanKeys()
{
	mPanRightKey		= 0.f;
	mPanLeftKey			= 0.f;
	mPanUpKey			= 0.f;
	mPanDownKey			= 0.f;
	mPanInKey			= 0.f;
	mPanOutKey			= 0.f;
}
S32 LLAgentCamera::directionToKey(S32 direction)
{
	if (direction > 0) return 1;
	if (direction < 0) return -1;
	return 0;
}
