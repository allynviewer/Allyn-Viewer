#include "llviewerprecompiledheaders.h"
#include "llmacroaim.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llavatarnamecache.h"
#include "llcharacter.h"
#include "llfontgl.h"
#include "lltoolcomp.h"
#include "lltoolgun.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "pipeline.h"
#include "rlvhandler.h"

#include <cmath>

namespace
{
	const F32 MACRO_HOVER_GRACE_SECONDS = 0.15f;
	const F32 MACRO_HUD_NAME_OFFSET = 48.f;
	const F32 MACRO_HUD_LINE1_OFFSET = 68.f;
	const F32 MACRO_HUD_LINE2_OFFSET = 84.f;
	const F32 MACRO_MAX_RATE = 360.f * DEG_TO_RAD;
	const F32 MACRO_MAX_DT = 0.1f;
	const F32 MACRO_MIN_DISTANCE = 0.05f;
	const F32 MACRO_MIN_ERROR = 0.0001f;
	const F32 MACRO_MIN_AXIS_SQ = 1e-6f;
	const F32 MACRO_LOCK_TIME_CONSTANT = 0.05f;
	const F32 MACRO_ASSIST_MAX_STRENGTH = 99.f;
	const F32 MACRO_ASSIST_TAU_MIN = 0.04f;
	const F32 MACRO_ASSIST_TAU_MAX = 1.5f;
	const F32 MACRO_ASSIST_HYSTERESIS = 1.2f;
	const F32 MACRO_ASSIST_SWITCH_RATIO = 0.7f;
	const F32 MACRO_ASSIST_SWITCH_SECONDS = 0.15f;
	const F32 MACRO_ASSIST_GRACE_SECONDS = 0.3f;
	const F32 MACRO_ASSIST_REJECT_SECONDS = 0.5f;
	const F32 MACRO_LOS_INTERVAL_SECONDS = 0.1f;
	const F32 MACRO_LOS_TOLERANCE_SECONDS = 0.5f;
	const F32 MACRO_LOS_STEP = 0.25f;
	const S32 MACRO_LOS_MAX_SEGMENTS = 16;
	const F32 MACRO_AIM_HEIGHT = 0.28f;

	LLVector3 aimPoint(const LLVOAvatar* avatarp)
	{
		const LLVector3 offset(0.f, 0.f, avatarp->getScale().mV[VZ] * MACRO_AIM_HEIGHT);
		return avatarp->getPositionAgent() + offset * avatarp->getRotationRegion();
	}

	const char* modeName(LLMacroAim::EMode mode)
	{
		switch (mode)
		{
		case LLMacroAim::MODE_LOCK:
			return "lock";
		case LLMacroAim::MODE_ASSIST:
			return "assist";
		default:
			return "off";
		}
	}
}

LLMacroAim::LLMacroAim()
:	mMode(MODE_OFF),
	mPreconditionsMet(false),
	mHoverDistance(0.f),
	mLosVisible(false),
	mStringsLoaded(false)
{
	mHoverLostTimer.stop();
	mAssistLostTimer.stop();
	mAssistSwitchTimer.stop();
	mAssistRejectedTimer.stop();
	mLosLostTimer.stop();
}

LLMacroAim::~LLMacroAim()
{
}

void LLMacroAim::reset()
{
	if (mMode != MODE_OFF || mHoverTargetId.notNull())
	{
		LL_DEBUGS("MacroAim") << "reset (mode " << modeName(mMode) << ", hover " << mHoverTargetId << ", lock " << mLockTargetId << ", assist " << mAssistTargetId << ")" << LL_ENDL;
	}
	mMode = MODE_OFF;
	mHoverTargetId.setNull();
	mLockTargetId.setNull();
	mAssistTargetId.setNull();
	mAssistSwitchId.setNull();
	mAssistRejectedId.setNull();
	mHoverDistance = 0.f;
	mLosVisible = false;
	mHoverLostTimer.stop();
	mAssistLostTimer.stop();
	mAssistSwitchTimer.stop();
	mAssistRejectedTimer.stop();
	mLosLostTimer.stop();
}

void LLMacroAim::setMode(EMode mode, const LLUUID& lock_target)
{
	if (mode == mMode && lock_target == mLockTargetId)
	{
		return;
	}
	LL_INFOS("MacroAim") << "mode " << modeName(mMode) << " -> " << modeName(mode);
	if (mode == MODE_LOCK)
	{
		LL_CONT << " target " << lock_target;
	}
	LL_CONT << LL_ENDL;
	mMode = mode;
	mLockTargetId = (mode == MODE_LOCK) ? lock_target : LLUUID::null;
	mAssistTargetId.setNull();
	mAssistSwitchId.setNull();
	mAssistRejectedId.setNull();
	mLosVisible = false;
	mAssistLostTimer.stop();
	mAssistSwitchTimer.stop();
	mAssistRejectedTimer.stop();
	mLosLostTimer.stop();
}

void LLMacroAim::toggleLock()
{
	if (!mPreconditionsMet)
	{
		LL_DEBUGS("MacroAim") << "toggleLock ignored: preconditions not met" << LL_ENDL;
		return;
	}
	if (mMode == MODE_LOCK)
	{
		setMode(MODE_OFF, LLUUID::null);
		return;
	}
	if (mHoverTargetId.isNull() || !resolveAvatar(mHoverTargetId))
	{
		LL_DEBUGS("MacroAim") << "toggleLock ignored: no avatar under the crosshair" << LL_ENDL;
		return;
	}
	setMode(MODE_LOCK, mHoverTargetId);
}

void LLMacroAim::toggleAssist()
{
	if (!mPreconditionsMet)
	{
		LL_DEBUGS("MacroAim") << "toggleAssist ignored: preconditions not met" << LL_ENDL;
		return;
	}
	if (mMode == MODE_ASSIST)
	{
		setMode(MODE_OFF, LLUUID::null);
		return;
	}
	if (!assistEnabledSetting())
	{
		LL_DEBUGS("MacroAim") << "toggleAssist ignored: disabled in preferences" << LL_ENDL;
		return;
	}
	setMode(MODE_ASSIST, LLUUID::null);
}

bool LLMacroAim::assistEnabledSetting()
{
	static LLCachedControl<bool> assist_enabled(gSavedSettings, "AllynMacroAimAssistEnabled", false);
	return assist_enabled;
}

F32 LLMacroAim::acquisitionRadius()
{
	static LLCachedControl<F32> acquisition(gSavedSettings, "AllynMacroAimAcquisition", 30.f);
	const F32 percent = llclamp((F32)acquisition, 0.f, 100.f);
	if (!std::isfinite(percent) || percent <= 0.f)
	{
		return 0.f;
	}
	const F32 half_fov = LLViewerCamera::getInstance()->getView() * 0.5f;
	return llmax(0.f, percent / 100.f * half_fov);
}

bool LLMacroAim::checkPreconditions() const
{
	static LLCachedControl<bool> render_hitbox(gSavedSettings, "DebugRenderHitboxes", false);
	if (!render_hitbox)
	{
		return false;
	}
	if (gDisconnected)
	{
		return false;
	}
	if (!isAgentAvatarValid())
	{
		return false;
	}
	if (!gAgentCamera.cameraMouselook())
	{
		return false;
	}
	if (gAgentCamera.getCameraAnimating())
	{
		return false;
	}
	return true;
}

F32 LLMacroAim::getMaxRange() const
{
	static LLCachedControl<F32> far_clip(gSavedSettings, "RenderFarClip", 128.f);
	F32 range = far_clip;
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMAVDIST))
	{
		range = llmin(range, gRlvHandler.camPole(RLV_BHVR_CAMAVDIST));
	}
	return llmax(range, 0.f);
}

void LLMacroAim::update()
{
	static LLTrace::BlockTimerStatHandle ftm("Macro Aim");
	LL_RECORD_BLOCK_TIME(ftm);
	bool met = checkPreconditions();
	if (met != mPreconditionsMet)
	{
		LL_DEBUGS("MacroAim") << "preconditions " << (met ? "met" : "not met") << LL_ENDL;
		mPreconditionsMet = met;
	}
	if (!met)
	{
		reset();
		return;
	}
	updateHoverTarget();
	if (mMode == MODE_OFF)
	{
		return;
	}
	const F32 dt = llclamp((F32)gFrameIntervalSeconds, 0.f, MACRO_MAX_DT);
	if (mMode == MODE_LOCK)
	{
		updateLock(dt);
	}
	else if (mMode == MODE_ASSIST)
	{
		updateAssist(dt);
	}
}

void LLMacroAim::endLock(const char* reason)
{
	LL_INFOS("MacroAim") << "lock ended: " << reason << " (target " << mLockTargetId << ")" << LL_ENDL;
	setMode(MODE_OFF, LLUUID::null);
}

void LLMacroAim::endAssist(const char* reason)
{
	LL_INFOS("MacroAim") << "assist ended: " << reason << LL_ENDL;
	setMode(MODE_OFF, LLUUID::null);
}

void LLMacroAim::clearAssistTarget(const char* reason)
{
	if (mAssistTargetId.notNull())
	{
		LL_DEBUGS("MacroAim") << "assist target released: " << reason << " (" << mAssistTargetId << ")" << LL_ENDL;
	}
	mAssistTargetId.setNull();
	mAssistSwitchId.setNull();
	mLosVisible = false;
	mAssistLostTimer.stop();
	mAssistSwitchTimer.stop();
	mLosLostTimer.stop();
}

LLVector3 LLMacroAim::currentViewDirection()
{
	LLQuaternion agent_rot = gAgent.getFrameAgent().getQuaternion();
	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		LLViewerObject* root_object = (LLViewerObject*)gAgentAvatarp->getRoot();
		if (root_object && !root_object->flagCameraDecoupled())
		{
			agent_rot *= ((LLViewerObject*)(gAgentAvatarp->getParent()))->getRenderRotation();
		}
	}
	LLVector3 at = LLVector3::x_axis * agent_rot;
	at.normVec();
	return at;
}

bool LLMacroAim::computeAimError(const LLVector3& view_dir, const LLVector3& want_dir, const LLVector3& skyward, F32& out_yaw, F32& out_pitch)
{
	out_yaw = 0.f;
	out_pitch = 0.f;
	const F32 view_elev = llclamp(view_dir * skyward, -1.f, 1.f);
	const F32 want_elev = llclamp(want_dir * skyward, -1.f, 1.f);
	LLVector3 view_h = view_dir - skyward * view_elev;
	LLVector3 want_h = want_dir - skyward * want_elev;
	if (view_h.magVecSquared() > MACRO_MIN_AXIS_SQ && want_h.magVecSquared() > MACRO_MIN_AXIS_SQ)
	{
		view_h.normVec();
		want_h.normVec();
		out_yaw = atan2f((view_h % want_h) * skyward, view_h * want_h);
	}
	out_pitch = asinf(view_elev) - asinf(want_elev);
	if (!std::isfinite(out_yaw) || !std::isfinite(out_pitch))
	{
		out_yaw = 0.f;
		out_pitch = 0.f;
		return false;
	}
	return true;
}

void LLMacroAim::applySteeredRotation(F32 yaw, F32 pitch, const LLVector3& skyward, F32 interpolant, F32 max_step)
{
	const F32 error = fabsf(yaw) + fabsf(pitch);
	if (error < MACRO_MIN_ERROR || interpolant <= 0.f || max_step <= 0.f)
	{
		return;
	}
	const F32 step = llmin(error * llclamp(interpolant, 0.f, 1.f), max_step);
	const F32 scale = step / error;
	if (!std::isfinite(scale) || scale <= 0.f)
	{
		return;
	}
	if (fabsf(yaw) >= MACRO_MIN_ERROR)
	{
		gAgent.rotate(yaw * scale, skyward);
	}
	if (fabsf(pitch) >= MACRO_MIN_ERROR)
	{
		gAgent.pitch(pitch * scale);
	}
}

void LLMacroAim::updateLock(F32 dt)
{
	const LLVOAvatar* avatarp = resolveAvatar(mLockTargetId);
	if (!avatarp)
	{
		endLock("target no longer valid");
		return;
	}
	const LLVector3& origin = LLViewerCamera::getInstance()->getOrigin();
	LLVector3 to_target = aimPoint(avatarp) - origin;
	const F32 distance = to_target.normVec();
	if (distance > getMaxRange())
	{
		endLock("target out of range");
		return;
	}
	if (distance < MACRO_MIN_DISTANCE || dt <= 0.f)
	{
		return;
	}
	LLVector3 skyward = gAgent.getReferenceUpVector();
	if (skyward.magVecSquared() < MACRO_MIN_AXIS_SQ)
	{
		return;
	}
	skyward.normVec();
	F32 yaw = 0.f;
	F32 pitch = 0.f;
	if (!computeAimError(currentViewDirection(), to_target, skyward, yaw, pitch))
	{
		return;
	}
	const F32 interpolant = llclamp(1.f - powf(2.f, -dt / MACRO_LOCK_TIME_CONSTANT), 0.f, 1.f);
	applySteeredRotation(yaw, pitch, skyward, interpolant, MACRO_MAX_RATE * dt);
}

bool LLMacroAim::angularError(const LLVOAvatar* avatarp, const LLVector3& origin, const LLVector3& view_dir, F32 max_range, F32& out_error, LLVector3& out_dir)
{
	LLVector3 to_target = aimPoint(avatarp) - origin;
	const F32 distance = to_target.normVec();
	if (distance < MACRO_MIN_DISTANCE || distance > max_range)
	{
		return false;
	}
	const F32 cos_angle = llclamp(to_target * view_dir, -1.f, 1.f);
	const F32 theta = acosf(cos_angle);
	const F32 half_extent = avatarp->getScale().magVec() * 0.5f;
	const F32 rho = asinf(llmin(1.f, half_extent / distance));
	const F32 error = llmax(0.f, theta - rho);
	if (!std::isfinite(error))
	{
		return false;
	}
	out_error = error;
	out_dir = to_target;
	return true;
}

bool LLMacroAim::hasLineOfSight(const LLVOAvatar* avatarp, const LLVector3& origin)
{
	const LLVector3 target = aimPoint(avatarp);
	LLVector3 dir = target - origin;
	const F32 total = dir.normVec();
	if (total < MACRO_MIN_DISTANCE)
	{
		return true;
	}
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		const LLVOAvatar* other = static_cast<const LLVOAvatar*>(*iter);
		if (other == avatarp || !isCandidate(other))
		{
			continue;
		}
		F32 t = 0.f;
		if (rayHitsHitbox(other, origin, dir, total, t) && t < total)
		{
			return false;
		}
	}
	LLVector3 start = origin;
	const BOOL prev_pick_avatar = LLPipeline::sPickAvatar;
	for (S32 segment = 0; segment < MACRO_LOS_MAX_SEGMENTS; ++segment)
	{
		LLVector4a start4;
		LLVector4a end4;
		LLVector4a intersection;
		start4.load3(start.mV);
		end4.load3(target.mV);
		intersection.clear();
		S32 face_hit = -1;
		LLViewerObject* hit = gPipeline.lineSegmentIntersectInWorld(start4, end4, FALSE, false, &face_hit, &intersection);
		LLPipeline::sPickAvatar = prev_pick_avatar;
		if (!hit)
		{
			return true;
		}
		LLVOAvatar* hit_avatar = hit->asAvatar();
		if (!hit_avatar)
		{
			hit_avatar = hit->getAvatar();
		}
		if (hit_avatar == avatarp)
		{
			return true;
		}
		if (!hit_avatar || !hit_avatar->isSelf())
		{
			return false;
		}
		LLVector3 hit_point(intersection.getF32ptr());
		const F32 travelled = (hit_point - origin) * dir;
		if (!std::isfinite(travelled) || travelled + MACRO_LOS_STEP >= total)
		{
			return true;
		}
		start = origin + dir * (travelled + MACRO_LOS_STEP);
	}
	return false;
}

bool LLMacroAim::updateAssistLineOfSight(const LLVOAvatar* avatarp, const LLVector3& origin)
{
	if (mLosCheckTimer.getElapsedTimeF32() < MACRO_LOS_INTERVAL_SECONDS)
	{
		return mLosVisible;
	}
	mLosCheckTimer.reset();
	mLosVisible = hasLineOfSight(avatarp, origin);
	if (mLosVisible)
	{
		mLosLostTimer.stop();
	}
	else if (!mLosLostTimer.getStarted())
	{
		mLosLostTimer.start();
	}
	return mLosVisible;
}

void LLMacroAim::updateAssist(F32 dt)
{
	if (!assistEnabledSetting())
	{
		endAssist("disabled in preferences");
		return;
	}
	const F32 radius = acquisitionRadius();
	const F32 max_range = getMaxRange();
	const LLVector3& origin = LLViewerCamera::getInstance()->getOrigin();
	const LLVector3 view_dir = currentViewDirection();

	if (mAssistRejectedId.notNull() && mAssistRejectedTimer.getStarted() && mAssistRejectedTimer.getElapsedTimeF32() >= MACRO_ASSIST_REJECT_SECONDS)
	{
		mAssistRejectedId.setNull();
		mAssistRejectedTimer.stop();
	}

	F32 current_error = 0.f;
	LLVector3 current_dir;
	bool current_in_area = false;
	if (mAssistTargetId.notNull())
	{
		const LLVOAvatar* current = resolveAvatar(mAssistTargetId);
		if (!current)
		{
			clearAssistTarget("target no longer valid");
		}
		else if (!angularError(current, origin, view_dir, max_range, current_error, current_dir))
		{
			clearAssistTarget("target out of range");
		}
		else if (radius <= 0.f || current_error > radius * MACRO_ASSIST_HYSTERESIS)
		{
			if (!mAssistLostTimer.getStarted())
			{
				mAssistLostTimer.start();
			}
			else if (mAssistLostTimer.getElapsedTimeF32() >= MACRO_ASSIST_GRACE_SECONDS)
			{
				clearAssistTarget("target left the acquisition area");
			}
		}
		else
		{
			mAssistLostTimer.stop();
			current_in_area = true;
		}
	}

	LLUUID best_id;
	F32 best_error = radius;
	if (radius > 0.f)
	{
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			 iter != LLCharacter::sInstances.end(); ++iter)
		{
			const LLVOAvatar* avatarp = static_cast<const LLVOAvatar*>(*iter);
			if (!isCandidate(avatarp) || avatarp->getID() == mAssistRejectedId)
			{
				continue;
			}
			F32 error = 0.f;
			LLVector3 dir;
			if (angularError(avatarp, origin, view_dir, max_range, error, dir) && error < best_error)
			{
				best_error = error;
				best_id = avatarp->getID();
			}
		}
	}

	bool acquire = false;
	if (mAssistTargetId.isNull())
	{
		acquire = best_id.notNull();
	}
	else if (current_in_area && best_id.notNull() && best_id != mAssistTargetId && best_error < current_error * MACRO_ASSIST_SWITCH_RATIO)
	{
		if (mAssistSwitchId != best_id || !mAssistSwitchTimer.getStarted())
		{
			mAssistSwitchId = best_id;
			mAssistSwitchTimer.start();
		}
		else if (mAssistSwitchTimer.getElapsedTimeF32() >= MACRO_ASSIST_SWITCH_SECONDS)
		{
			acquire = true;
		}
	}
	else
	{
		mAssistSwitchId.setNull();
		mAssistSwitchTimer.stop();
	}

	if (acquire && mLosCheckTimer.getElapsedTimeF32() >= MACRO_LOS_INTERVAL_SECONDS)
	{
		const LLVOAvatar* candidate = resolveAvatar(best_id);
		if (candidate)
		{
			mLosCheckTimer.reset();
			if (hasLineOfSight(candidate, origin))
			{
				if (mAssistTargetId.notNull())
				{
					clearAssistTarget("switching to a closer target");
				}
				mAssistTargetId = best_id;
				mLosVisible = true;
				mLosLostTimer.stop();
				mAssistLostTimer.stop();
				mAssistSwitchId.setNull();
				mAssistSwitchTimer.stop();
				LL_DEBUGS("MacroAim") << "assist target acquired: " << best_id << " (error " << best_error * RAD_TO_DEG << " deg)" << LL_ENDL;
				current_in_area = angularError(candidate, origin, view_dir, max_range, current_error, current_dir);
			}
			else
			{
				mAssistRejectedId = best_id;
				mAssistRejectedTimer.start();
				mAssistSwitchId.setNull();
				mAssistSwitchTimer.stop();
			}
		}
	}

	if (mAssistTargetId.isNull() || !current_in_area)
	{
		return;
	}
	const LLVOAvatar* target = resolveAvatar(mAssistTargetId);
	if (!target)
	{
		clearAssistTarget("target no longer valid");
		return;
	}
	updateAssistLineOfSight(target, origin);
	if (!mLosVisible)
	{
		if (mLosLostTimer.getStarted() && mLosLostTimer.getElapsedTimeF32() >= MACRO_LOS_TOLERANCE_SECONDS)
		{
			mAssistRejectedId = mAssistTargetId;
			mAssistRejectedTimer.start();
			clearAssistTarget("line of sight lost");
		}
		return;
	}
	if (dt <= 0.f)
	{
		return;
	}
	static LLCachedControl<F32> strength_setting(gSavedSettings, "AllynMacroAimStrength", 50.f);
	const F32 strength = llclamp((F32)strength_setting, 0.f, MACRO_ASSIST_MAX_STRENGTH);
	if (!std::isfinite(strength) || strength <= 0.f)
	{
		return;
	}
	const F32 normalized = strength / MACRO_ASSIST_MAX_STRENGTH;
	const F32 ratio = llclamp(current_error / radius, 0.f, 1.f);
	const F32 weight = llclamp(1.f - ratio * ratio, 0.f, 1.f);
	if (weight <= 0.f)
	{
		return;
	}
	const F32 tau = MACRO_ASSIST_TAU_MAX - (MACRO_ASSIST_TAU_MAX - MACRO_ASSIST_TAU_MIN) * normalized * normalized;
	const F32 interpolant = llclamp(1.f - powf(2.f, -dt / tau), 0.f, 1.f);
	const F32 max_step = normalized * MACRO_MAX_RATE * dt;
	LLVector3 skyward = gAgent.getReferenceUpVector();
	if (skyward.magVecSquared() < MACRO_MIN_AXIS_SQ)
	{
		return;
	}
	skyward.normVec();
	F32 yaw = 0.f;
	F32 pitch = 0.f;
	if (!computeAimError(view_dir, current_dir, skyward, yaw, pitch))
	{
		return;
	}
	applySteeredRotation(yaw, pitch, skyward, interpolant * weight, max_step);
}

LLVOAvatar* LLMacroAim::resolveAvatar(const LLUUID& id)
{
	if (id.isNull())
	{
		return NULL;
	}
	LLVOAvatar* avatarp = gObjectList.findAvatar(id);
	if (!isCandidate(avatarp))
	{
		return NULL;
	}
	return avatarp;
}

bool LLMacroAim::isCandidate(const LLVOAvatar* avatarp)
{
	if (!avatarp)
	{
		return false;
	}
	if (avatarp->isDead())
	{
		return false;
	}
	if (avatarp->isSelf())
	{
		return false;
	}
	if (avatarp->mIsDummy)
	{
		return false;
	}
	if (avatarp->isControlAvatar())
	{
		return false;
	}
	if (avatarp->isUIAvatar())
	{
		return false;
	}
	if (avatarp->getID().isNull())
	{
		return false;
	}
	if (avatarp->mDrawable.isNull())
	{
		return false;
	}
	if (avatarp->shouldHideForFriendsOnly())
	{
		return false;
	}
	return true;
}

bool LLMacroAim::rayHitsHitbox(const LLVOAvatar* avatarp, const LLVector3& origin, const LLVector3& dir, F32 max_t, F32& out_t)
{
	const LLVector3& center = avatarp->getPositionAgent();
	const LLVector3 half = avatarp->getScale() * 0.5f;
	if (half.mV[VX] <= 0.f || half.mV[VY] <= 0.f || half.mV[VZ] <= 0.f)
	{
		return false;
	}
	const LLQuaternion inv_rot = ~avatarp->getRotationRegion();
	const LLVector3 local_origin = (origin - center) * inv_rot;
	const LLVector3 local_dir = dir * inv_rot;
	F32 t_min = 0.f;
	F32 t_max = max_t;
	for (S32 axis = 0; axis < 3; ++axis)
	{
		const F32 o = local_origin.mV[axis];
		const F32 d = local_dir.mV[axis];
		const F32 h = half.mV[axis];
		if (fabsf(d) < F_APPROXIMATELY_ZERO)
		{
			if (o < -h || o > h)
			{
				return false;
			}
			continue;
		}
		F32 t1 = (-h - o) / d;
		F32 t2 = (h - o) / d;
		if (t1 > t2)
		{
			std::swap(t1, t2);
		}
		t_min = llmax(t_min, t1);
		t_max = llmin(t_max, t2);
		if (t_min > t_max)
		{
			return false;
		}
	}
	out_t = t_min;
	return true;
}

void LLMacroAim::setHoverTarget(const LLUUID& id, F32 distance)
{
	if (id != mHoverTargetId)
	{
		if (id.isNull())
		{
			LL_DEBUGS("MacroAim") << "hover target lost: " << mHoverTargetId << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("MacroAim") << "hover target: " << id << " at " << distance << " m" << LL_ENDL;
		}
		mHoverTargetId = id;
	}
	mHoverDistance = distance;
}

void LLMacroAim::updateHoverTarget()
{
	const LLViewerCamera* camera = LLViewerCamera::getInstance();
	const LLVector3& origin = camera->getOrigin();
	const LLVector3& dir = camera->getAtAxis();
	const F32 max_range = getMaxRange();
	LLUUID best_id;
	F32 best_t = max_range;
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		const LLVOAvatar* avatarp = static_cast<const LLVOAvatar*>(*iter);
		if (!isCandidate(avatarp))
		{
			continue;
		}
		F32 t = 0.f;
		if (rayHitsHitbox(avatarp, origin, dir, max_range, t) && t < best_t)
		{
			best_t = t;
			best_id = avatarp->getID();
		}
	}
	if (best_id.notNull())
	{
		mHoverLostTimer.stop();
		setHoverTarget(best_id, best_t);
		return;
	}
	if (mHoverTargetId.isNull())
	{
		return;
	}
	if (!mHoverLostTimer.getStarted())
	{
		mHoverLostTimer.start();
	}
	if (mHoverLostTimer.getElapsedTimeF32() >= MACRO_HOVER_GRACE_SECONDS || !resolveAvatar(mHoverTargetId))
	{
		mHoverLostTimer.stop();
		setHoverTarget(LLUUID::null, 0.f);
	}
}

void LLMacroAim::loadStrings()
{
	if (mStringsLoaded)
	{
		return;
	}
	mLockHint = LLTrans::getString("MacroHudLockHint");
	mAssistHint = LLTrans::getString("MacroHudAssistHint");
	mLockedText = LLTrans::getString("MacroHudLocked");
	mAssistOnText = LLTrans::getString("MacroHudAssistOn");
	mStringsLoaded = true;
}

bool LLMacroAim::iffShowsName(const LLUUID& id) const
{
	static LLCachedControl<bool> show_iff(gSavedSettings, "AlchemyMouselookIFF", true);
	static LLCachedControl<F32> iff_range(gSavedSettings, "AlchemyMouselookIFFRange", 380.f);
	if (!show_iff || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWMINIMAP))
	{
		return false;
	}
	LLToolCompGun* gun_comp = LLToolCompGun::getIfExists();
	if (!gun_comp || LLToolMgr::getInstance()->getCurrentTool() != gun_comp)
	{
		return false;
	}
	if (!dynamic_cast<LLToolGun*>(gun_comp->getCurrentTool()))
	{
		return false;
	}
	const bool no_names = gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS);
	const bool name_restricted = no_names || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES);
	const F32 range = (name_restricted && gRlvHandler.hasBehaviour(RLV_BHVR_CAMAVDIST)) ? llmin(iff_range(), gRlvHandler.camPole(RLV_BHVR_CAMAVDIST)) : iff_range();
	const LLVector3d my_position = gAgentCamera.getCameraPositionGlobal();
	LLQuaternion my_rotation = LLViewerCamera::getInstance()->getQuaternion();
	my_rotation.set(-my_rotation.mQ[VX], -my_rotation.mQ[VY], -my_rotation.mQ[VZ], my_rotation.mQ[VW]);
	LLWorld::pos_map_t positions;
	LLWorld::instance().getAvatars(&positions, gAgent.getPositionGlobal(), range);
	LLWorld::pos_map_t::const_iterator found = positions.find(id);
	if (found == positions.end() || found->second.isNull())
	{
		return false;
	}
	LLVector3d magic_vector = (found->second - my_position) * my_rotation;
	magic_vector.setVec(-magic_vector.mdV[VY], magic_vector.mdV[VZ], magic_vector.mdV[VX]);
	return magic_vector.mdV[VX] > -0.75 && magic_vector.mdV[VX] < 0.75 && magic_vector.mdV[VZ] > 0.0 && magic_vector.mdV[VY] > -1.5 && magic_vector.mdV[VY] < 1.5;
}

bool LLMacroAim::hudName(const LLUUID& id, const LLVOAvatar* avatarp, std::string& out_name) const
{
	out_name.clear();
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS))
	{
		return false;
	}
	if (iffShowsName(id))
	{
		return false;
	}
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(id, &av_name))
	{
		out_name = av_name.getNSName();
	}
	else
	{
		out_name = avatarp->getFullname();
	}
	if (out_name.empty())
	{
		return false;
	}
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
	{
		out_name = RlvStrings::getAnonym(out_name);
	}
	return true;
}

void LLMacroAim::draw()
{
	if (!mPreconditionsMet)
	{
		return;
	}
	LLUUID name_id;
	if (mMode == MODE_LOCK)
	{
		name_id = mLockTargetId;
	}
	else if (mMode == MODE_ASSIST)
	{
		name_id = mAssistTargetId.notNull() ? mAssistTargetId : mHoverTargetId;
	}
	else
	{
		name_id = mHoverTargetId;
	}
	if (mMode == MODE_OFF && name_id.isNull())
	{
		return;
	}
	const LLVOAvatar* avatarp = name_id.notNull() ? resolveAvatar(name_id) : NULL;
	if (mMode == MODE_OFF && !avatarp)
	{
		return;
	}
	loadStrings();
	static const LLCachedControl<LLColor4> crosshair_color(gSavedSettings, "LiruCrosshairColor", LLColor4::white);
	LLColor4 color = crosshair_color;
	color.mV[VALPHA] = 0.5f;
	const LLRect& view_rect = gViewerWindow->getWorldViewRectScaled();
	const F32 center_x = view_rect.getWidth() / 2.f;
	const F32 center_y = view_rect.getHeight() / 2.f;
	std::string name;
	if (avatarp && hudName(name_id, avatarp, name))
	{
		LLFontGL::getFontSansSerifBold()->renderUTF8(name, 0, center_x, center_y - MACRO_HUD_NAME_OFFSET, color,
			LLFontGL::HCENTER, LLFontGL::TOP, LLFontGL::BOLD, LLFontGL::NO_SHADOW);
	}
	const std::string& line1 = (mMode == MODE_LOCK) ? mLockedText : mLockHint;
	const LLFontGL* small_font = LLFontGL::getFontSansSerifSmall();
	small_font->renderUTF8(line1, 0, center_x, center_y - MACRO_HUD_LINE1_OFFSET, color,
		LLFontGL::HCENTER, LLFontGL::TOP, LLFontGL::NORMAL, LLFontGL::NO_SHADOW);
	if (mMode == MODE_ASSIST)
	{
		small_font->renderUTF8(mAssistOnText, 0, center_x, center_y - MACRO_HUD_LINE2_OFFSET, color,
			LLFontGL::HCENTER, LLFontGL::TOP, LLFontGL::NORMAL, LLFontGL::NO_SHADOW);
	}
	else if (assistEnabledSetting())
	{
		small_font->renderUTF8(mAssistHint, 0, center_x, center_y - MACRO_HUD_LINE2_OFFSET, color,
			LLFontGL::HCENTER, LLFontGL::TOP, LLFontGL::NORMAL, LLFontGL::NO_SHADOW);
	}
}
