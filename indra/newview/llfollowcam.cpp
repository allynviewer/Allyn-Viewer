/** 
 * @file llfollowcam.cpp
 * @author Jeffrey Ventrella
 * @brief LLFollowCam class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#include "llfollowcam.h"
#include "llagent.h"
std::map<LLUUID, LLFollowCamParams*> LLFollowCamMgr::sParamMap;
std::vector<LLFollowCamParams*> LLFollowCamMgr::sParamStack;
const F32 ONE_HALF							= 0.5;
const F32 FOLLOW_CAM_ZOOM_FACTOR			= 0.1f;
const F32 FOLLOW_CAM_MIN_ZOOM_AMOUNT		= 0.1f;
const F32 DISTANCE_EPSILON					= 0.0001f;
const F32 DEFAULT_MAX_DISTANCE_FROM_SUBJECT	= 1000.0;
const F32 FOLLOW_CAM_MIN_POSITION_LAG		= 0.0f;
const F32 FOLLOW_CAM_DEFAULT_POSITION_LAG	= 0.1f;
const F32 FOLLOW_CAM_MAX_POSITION_LAG		= 3.0f;
const F32 FOLLOW_CAM_MIN_FOCUS_LAG		= 0.0f;
const F32 FOLLOW_CAM_DEFAULT_FOCUS_LAG	= 0.1f;
const F32 FOLLOW_CAM_MAX_FOCUS_LAG		= 3.0f;
const F32 FOLLOW_CAM_MIN_POSITION_THRESHOLD		= 0.0f;
const F32 FOLLOW_CAM_DEFAULT_POSITION_THRESHOLD	= 1.0f;
const F32 FOLLOW_CAM_MAX_POSITION_THRESHOLD		= 4.0f;
const F32 FOLLOW_CAM_MIN_FOCUS_THRESHOLD		= 0.0f;
const F32 FOLLOW_CAM_DEFAULT_FOCUS_THRESHOLD	= 1.0f;
const F32 FOLLOW_CAM_MAX_FOCUS_THRESHOLD		= 4.0f;
const F32 FOLLOW_CAM_MIN_DISTANCE		= 0.5f;
const F32 FOLLOW_CAM_DEFAULT_DISTANCE	= 3.0f;
const F32 FOLLOW_CAM_MIN_PITCH		= -45.0f;
const F32 FOLLOW_CAM_DEFAULT_PITCH	=   0.0f;
const F32 FOLLOW_CAM_MAX_PITCH		=  80.0f;
const F32 FOLLOW_CAM_MIN_FOCUS_OFFSET		= -10.0f;
const LLVector3 FOLLOW_CAM_DEFAULT_FOCUS_OFFSET	=  LLVector3(1.0f, 0.f, 0.f);
const F32 FOLLOW_CAM_MAX_FOCUS_OFFSET		=  10.0f;
const F32 FOLLOW_CAM_MIN_BEHINDNESS_LAG		= 0.0f;
const F32 FOLLOW_CAM_DEFAULT_BEHINDNESS_LAG		= 0.f;
const F32 FOLLOW_CAM_MAX_BEHINDNESS_LAG		= 3.0f;
const F32 FOLLOW_CAM_MIN_BEHINDNESS_ANGLE		= 0.0f;
const F32 FOLLOW_CAM_DEFAULT_BEHINDNESS_ANGLE	= 10.0f;
const F32 FOLLOW_CAM_MAX_BEHINDNESS_ANGLE		= 180.0f;
const F32 FOLLOW_CAM_BEHINDNESS_EPSILON			= 1.0f;
LLFollowCamParams::LLFollowCamParams()
{
	mMaxCameraDistantFromSubject		= DEFAULT_MAX_DISTANCE_FROM_SUBJECT;
	mPositionLocked						= false;
	mFocusLocked						= false;
	mUsePosition						= false;
	mUseFocus							= false;
	setPositionLag		( FOLLOW_CAM_DEFAULT_POSITION_LAG			);
	setFocusLag			( FOLLOW_CAM_DEFAULT_FOCUS_LAG				);
	setPositionThreshold( FOLLOW_CAM_DEFAULT_POSITION_THRESHOLD		);
	setFocusThreshold	( FOLLOW_CAM_DEFAULT_FOCUS_THRESHOLD		);
	setBehindnessLag	( FOLLOW_CAM_DEFAULT_BEHINDNESS_LAG		);
	setDistance			( FOLLOW_CAM_DEFAULT_DISTANCE				);
	setPitch			( FOLLOW_CAM_DEFAULT_PITCH					);
	setFocusOffset		( FOLLOW_CAM_DEFAULT_FOCUS_OFFSET	);
	setBehindnessAngle	( FOLLOW_CAM_DEFAULT_BEHINDNESS_ANGLE		);
	setPositionThreshold( FOLLOW_CAM_DEFAULT_POSITION_THRESHOLD		);
	setFocusThreshold	( FOLLOW_CAM_DEFAULT_FOCUS_THRESHOLD		);
}
LLFollowCamParams::~LLFollowCamParams() { }
void LLFollowCamParams::setPositionLag( F32 p )
{
	mPositionLag = llclamp(p, FOLLOW_CAM_MIN_POSITION_LAG, FOLLOW_CAM_MAX_POSITION_LAG);
}
void LLFollowCamParams::setFocusLag( F32 f )
{
	mFocusLag = llclamp(f, FOLLOW_CAM_MIN_FOCUS_LAG, FOLLOW_CAM_MAX_FOCUS_LAG);
}
void LLFollowCamParams::setPositionThreshold( F32 p )
{
	mPositionThreshold = llclamp(p, FOLLOW_CAM_MIN_POSITION_THRESHOLD, FOLLOW_CAM_MAX_POSITION_THRESHOLD);
}
void LLFollowCamParams::setFocusThreshold( F32 f )
{
	mFocusThreshold = llclamp(f, FOLLOW_CAM_MIN_FOCUS_THRESHOLD, FOLLOW_CAM_MAX_FOCUS_THRESHOLD);
}
void LLFollowCamParams::setPitch( F32 p )
{
	mPitch = llclamp(p, FOLLOW_CAM_MIN_PITCH, FOLLOW_CAM_MAX_PITCH);
}
void LLFollowCamParams::setBehindnessLag( F32 b )
{
	mBehindnessLag = llclamp(b, FOLLOW_CAM_MIN_BEHINDNESS_LAG, FOLLOW_CAM_MAX_BEHINDNESS_LAG);
}
void LLFollowCamParams::setBehindnessAngle( F32 b )
{
	mBehindnessMaxAngle = llclamp(b, FOLLOW_CAM_MIN_BEHINDNESS_ANGLE, FOLLOW_CAM_MAX_BEHINDNESS_ANGLE);
}
void LLFollowCamParams::setDistance( F32 d )
{
	mDistance = llclamp(d, FOLLOW_CAM_MIN_DISTANCE, mMaxCameraDistantFromSubject);
}
void LLFollowCamParams::setPositionLocked( bool l )
{
	mPositionLocked = l;
}
void LLFollowCamParams::setFocusLocked( bool l )
{
	mFocusLocked = l;
}
void LLFollowCamParams::setFocusOffset( const LLVector3& v )
{
	mFocusOffset = v;
	mFocusOffset.clamp(FOLLOW_CAM_MIN_FOCUS_OFFSET, FOLLOW_CAM_MAX_FOCUS_OFFSET);
}
void LLFollowCamParams::setPosition( const LLVector3& p )
{
	mUsePosition = true;
	mPosition = p;
}
void LLFollowCamParams::setFocus( const LLVector3& f )
{
	mUseFocus = true;
	mFocus = f;
}
F32			LLFollowCamParams::getPositionLag		() const { return mPositionLag;			}
F32			LLFollowCamParams::getFocusLag			() const { return mFocusLag;			}
F32			LLFollowCamParams::getPositionThreshold	() const { return mPositionThreshold;	}
F32			LLFollowCamParams::getFocusThreshold	() const { return mFocusThreshold;		}
F32			LLFollowCamParams::getDistance			() const { return mDistance;			}
F32			LLFollowCamParams::getPitch				() const { return mPitch;				}
LLVector3	LLFollowCamParams::getFocusOffset		() const { return mFocusOffset;			}
F32			LLFollowCamParams::getBehindnessAngle	() const { return mBehindnessMaxAngle;	}
F32			LLFollowCamParams::getBehindnessLag		() const { return mBehindnessLag;		}
LLVector3	LLFollowCamParams::getPosition			() const { return mPosition;			}
LLVector3	LLFollowCamParams::getFocus				() const { return mFocus;				}
bool		LLFollowCamParams::getPositionLocked	() const { return mPositionLocked;		}
bool		LLFollowCamParams::getFocusLocked		() const { return mFocusLocked;			}
LLFollowCam::LLFollowCam() : LLFollowCamParams()
{
	mUpVector							= LLVector3::z_axis;
	mSubjectPosition					= LLVector3::zero;
	mSubjectRotation					= LLQuaternion::DEFAULT;
	mZoomedToMinimumDistance			= false;
	mPitchCos = mPitchSin = 0.f;
	mPitchSineAndCosineNeedToBeUpdated	= true;
	mSimulatedDistance = mDistance;
}
void LLFollowCam::copyParams(LLFollowCamParams& params)
{
	setPositionLag(params.getPositionLag());
	setFocusLag(params.getFocusLag());
	setFocusThreshold( params.getFocusThreshold());
	setPositionThreshold(params.getPositionThreshold());
	setPitch(params.getPitch());
	setFocusOffset(params.getFocusOffset());
	setBehindnessAngle(params.getBehindnessAngle());
	setBehindnessLag(params.getBehindnessLag());
	setPositionLocked(params.getPositionLocked());
	setFocusLocked(params.getFocusLocked());
	setDistance(params.getDistance());
	if (params.getUsePosition())
	{
		setPosition(params.getPosition());
	}
	if (params.getUseFocus())
	{
		setFocus(params.getFocus());
	}
}
void LLFollowCam::update()
{
	LLVector3 offsetSubjectPosition = mSubjectPosition + (mFocusOffset * mSubjectRotation);
	LLVector3 simulated_pos_agent = gAgent.getPosAgentFromGlobal(mSimulatedPositionGlobal);
	LLVector3 vectorFromCameraToSubject = offsetSubjectPosition - simulated_pos_agent;
	F32 distanceFromCameraToSubject = vectorFromCameraToSubject.magVec();
	LLVector3 whereFocusWantsToBe = mFocus;
	LLVector3 focus_pt_agent = gAgent.getPosAgentFromGlobal(mSimulatedFocusGlobal);
	if ( mFocusLocked )
	{
		mRelativeFocus = (focus_pt_agent - mSubjectPosition) * ~mSubjectRotation;
	}
	else
	{
		LLVector3 focusOffset = offsetSubjectPosition - focus_pt_agent;
		F32 focusOffsetDistance = focusOffset.magVec();
		LLVector3 focusOffsetDirection = focusOffset / focusOffsetDistance;
		whereFocusWantsToBe = focus_pt_agent +
			(focusOffsetDirection * (focusOffsetDistance - mFocusThreshold));
		if ( focusOffsetDistance > mFocusThreshold )
		{
			F32 focusLagLerp = LLSmoothInterpolation::getInterpolant( mFocusLag );
			focus_pt_agent = lerp( focus_pt_agent, whereFocusWantsToBe, focusLagLerp );
			mSimulatedFocusGlobal = gAgent.getPosGlobalFromAgent(focus_pt_agent);
		}
		mRelativeFocus = lerp(mRelativeFocus, (focus_pt_agent - mSubjectPosition) * ~mSubjectRotation, LLSmoothInterpolation::getInterpolant(0.05f));
	}
	LLVector3 whereCameraPositionWantsToBe = gAgent.getPosAgentFromGlobal(mSimulatedPositionGlobal);
	if (  mPositionLocked )
	{
		mRelativePos = (whereCameraPositionWantsToBe - mSubjectPosition) * ~mSubjectRotation;
	}
	else
	{
		LLVector3 horizontalVectorFromCameraToSubject = vectorFromCameraToSubject;
		horizontalVectorFromCameraToSubject.mV[VZ] = 0.0f;
		F32 horizontalDistanceFromCameraToSubject = horizontalVectorFromCameraToSubject.magVec();
		LLVector3 horizontalDirectionFromCameraToSubject;
		if ( horizontalDistanceFromCameraToSubject < DISTANCE_EPSILON )
		{
			horizontalDirectionFromCameraToSubject = LLVector3::z_axis;
		}
		else
		{
			horizontalDirectionFromCameraToSubject = horizontalVectorFromCameraToSubject / horizontalDistanceFromCameraToSubject;
		}
		if ( mPitchSineAndCosineNeedToBeUpdated )
		{
			calculatePitchSineAndCosine();
			mPitchSineAndCosineNeedToBeUpdated = false;
		}
		LLVector3 positionOffsetFromSubject;
		positionOffsetFromSubject.setVec
			(
				horizontalDirectionFromCameraToSubject.mV[ VX ] * mPitchCos,
				horizontalDirectionFromCameraToSubject.mV[ VY ] * mPitchCos,
				-mPitchSin
			);
		positionOffsetFromSubject *= mSimulatedDistance;
		LLVector3 idealCameraPosition = offsetSubjectPosition - positionOffsetFromSubject;
		LLVector3 vectorFromPositionToIdealPosition = idealCameraPosition - simulated_pos_agent;
		F32 distanceFromPositionToIdealPosition = vectorFromPositionToIdealPosition.magVec();
		LLVector3 normalFromPositionToIdealPosition = vectorFromPositionToIdealPosition / distanceFromPositionToIdealPosition;
		whereCameraPositionWantsToBe = simulated_pos_agent +
			(normalFromPositionToIdealPosition * (distanceFromPositionToIdealPosition - mPositionThreshold));
		if ( distanceFromPositionToIdealPosition > mPositionThreshold )
		{
			F32 positionPullLerp = LLSmoothInterpolation::getInterpolant( mPositionLag );
			simulated_pos_agent = lerp( simulated_pos_agent, whereCameraPositionWantsToBe, positionPullLerp );
		}
		if ( distanceFromCameraToSubject > mMaxCameraDistantFromSubject )
		{
			LLVector3 directionFromCameraToSubject = vectorFromCameraToSubject / distanceFromCameraToSubject;
			simulated_pos_agent = offsetSubjectPosition - directionFromCameraToSubject * mMaxCameraDistantFromSubject;
		}
		updateBehindnessConstraint(gAgent.getPosAgentFromGlobal(mSimulatedFocusGlobal), simulated_pos_agent);
		mSimulatedPositionGlobal = gAgent.getPosGlobalFromAgent(simulated_pos_agent);
		mRelativePos = lerp(mRelativePos, (simulated_pos_agent - mSubjectPosition) * ~mSubjectRotation, LLSmoothInterpolation::getInterpolant(0.05f));
	}
	mUpVector = LLVector3::z_axis;
}
BOOL LLFollowCam::updateBehindnessConstraint(LLVector3 focus, LLVector3& cam_position)
{
	BOOL constraint_active = FALSE;
	if ( mBehindnessMaxAngle < FOLLOW_CAM_MAX_BEHINDNESS_ANGLE - FOLLOW_CAM_BEHINDNESS_EPSILON )
	{
		LLVector3 horizontalVectorFromFocusToCamera;
		horizontalVectorFromFocusToCamera.setVec(cam_position - focus);
		horizontalVectorFromFocusToCamera.mV[ VZ ] = 0.0f;
		F32 cameraZ = cam_position.mV[ VZ ];
		F32 horizontalDistance = horizontalVectorFromFocusToCamera.magVec();
		LLVector3 horizontalSubjectBack( -1.0f, 0.0f, 0.0f );
		horizontalSubjectBack *= mSubjectRotation;
		horizontalSubjectBack.mV[ VZ ] = 0.0f;
		horizontalSubjectBack.normVec();
		horizontalSubjectBack *= horizontalDistance;
		F32 cameraOffsetAngle = 0.f;
		LLVector3 cameraOffsetRotationAxis;
		LLQuaternion camera_offset_rotation;
		camera_offset_rotation.shortestArc(horizontalSubjectBack, horizontalVectorFromFocusToCamera);
		camera_offset_rotation.getAngleAxis(&cameraOffsetAngle, cameraOffsetRotationAxis);
		cameraOffsetAngle *= RAD_TO_DEG;
		if ( cameraOffsetAngle > mBehindnessMaxAngle )
		{
			F32 fraction = ((cameraOffsetAngle - mBehindnessMaxAngle) / cameraOffsetAngle) * LLSmoothInterpolation::getInterpolant(mBehindnessLag);
			cam_position = focus + horizontalSubjectBack * (slerp(fraction, camera_offset_rotation, LLQuaternion::DEFAULT));
			cam_position.mV[VZ] = cameraZ;
			constraint_active = TRUE;
		}
	}
	return constraint_active;
}
void LLFollowCam::calculatePitchSineAndCosine()
{
	F32 radian = mPitch * DEG_TO_RAD;
	mPitchCos = cos( radian );
	mPitchSin = sin( radian );
}
void LLFollowCam::setSubjectPositionAndRotation( const LLVector3 p, const LLQuaternion r )
{
	mSubjectPosition = p;
	mSubjectRotation = r;
}
void LLFollowCam::zoom( S32 z )
{
	F32 zoomAmount = z * mSimulatedDistance * FOLLOW_CAM_ZOOM_FACTOR;
	if (( zoomAmount <  FOLLOW_CAM_MIN_ZOOM_AMOUNT )
	&&  ( zoomAmount > -FOLLOW_CAM_MIN_ZOOM_AMOUNT ))
	{
		if ( zoomAmount < 0.0f )
		{
			zoomAmount = -FOLLOW_CAM_MIN_ZOOM_AMOUNT;
		}
		else
		{
			zoomAmount = FOLLOW_CAM_MIN_ZOOM_AMOUNT;
		}
	}
	mSimulatedDistance += zoomAmount;
	mZoomedToMinimumDistance = false;
	if ( mSimulatedDistance < FOLLOW_CAM_MIN_DISTANCE )
	{
		mSimulatedDistance = FOLLOW_CAM_MIN_DISTANCE;
		if ( zoomAmount < 0.0f )
		{
			mZoomedToMinimumDistance = true;
		}
	}
	else if ( mSimulatedDistance > mMaxCameraDistantFromSubject )
	{
		mSimulatedDistance = mMaxCameraDistantFromSubject;
	}
}
bool LLFollowCam::isZoomedToMinimumDistance()
{
	return mZoomedToMinimumDistance;
}
void LLFollowCam::reset( const LLVector3 p, const LLVector3 f , const LLVector3 u )
{
	setPosition(p);
	setFocus(f);
	mUpVector	= u;
}
void LLFollowCam::setMaxCameraDistantFromSubject( F32 m )
{
	mMaxCameraDistantFromSubject = m;
}
void LLFollowCam::setPitch( F32 p )
{
	LLFollowCamParams::setPitch(p);
	mPitchSineAndCosineNeedToBeUpdated = true;
}
void LLFollowCam::setDistance( F32 d )
{
	if (d != mDistance)
	{
		LLFollowCamParams::setDistance(d);
		mSimulatedDistance = d;
		mZoomedToMinimumDistance = false;
	}
}
void LLFollowCam::setPosition( const LLVector3& p )
{
	if (p != mPosition)
	{
		LLFollowCamParams::setPosition(p);
		mSimulatedPositionGlobal = gAgent.getPosGlobalFromAgent(mPosition);
		if (mPositionLocked)
		{
			mRelativePos = (mPosition - mSubjectPosition) * ~mSubjectRotation;
		}
	}
}
void LLFollowCam::setFocus( const LLVector3& f )
{
	if (f != mFocus)
	{
		LLFollowCamParams::setFocus(f);
		mSimulatedFocusGlobal = gAgent.getPosGlobalFromAgent(f);
		if (mFocusLocked)
		{
			mRelativeFocus = (mFocus - mSubjectPosition) * ~mSubjectRotation;
		}
	}
}
void LLFollowCam::setPositionLocked( bool locked )
{
	LLFollowCamParams::setPositionLocked(locked);
	if (locked)
	{
		mRelativePos = (gAgent.getPosAgentFromGlobal(mSimulatedPositionGlobal) - mSubjectPosition) * ~mSubjectRotation;
	}
}
void LLFollowCam::setFocusLocked( bool locked )
{
	LLFollowCamParams::setFocusLocked(locked);
	if (locked)
	{
		mRelativeFocus = (gAgent.getPosAgentFromGlobal(mSimulatedFocusGlobal) - mSubjectPosition) * ~mSubjectRotation;
	}
}
LLVector3	LLFollowCam::getSimulatedPosition() const
{
	return mSubjectPosition + (mRelativePos * mSubjectRotation);
}
LLVector3	LLFollowCam::getSimulatedFocus() const
{
	return mSubjectPosition + (mRelativeFocus * mSubjectRotation);
}
LLVector3	LLFollowCam::getUpVector()
{
	return mUpVector;
}
LLFollowCam::~LLFollowCam()
{
}
void LLFollowCamMgr::cleanupClass()
{
	for (param_map_t::iterator iter = sParamMap.begin(); iter != sParamMap.end(); ++iter)
	{
		LLFollowCamParams* params = iter->second;
		delete params;
	}
	sParamMap.clear();
}
void LLFollowCamMgr::setPositionLag( const LLUUID& source, F32 lag)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setPositionLag(lag);
	}
}
void LLFollowCamMgr::setFocusLag( const LLUUID& source, F32 lag)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setFocusLag(lag);
	}
}
void LLFollowCamMgr::setFocusThreshold( const LLUUID& source, F32 threshold)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setFocusThreshold(threshold);
	}
}
void LLFollowCamMgr::setPositionThreshold( const LLUUID& source, F32 threshold)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setPositionThreshold(threshold);
	}
}
void LLFollowCamMgr::setDistance( const LLUUID& source, F32 distance)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setDistance(distance);
	}
}
void LLFollowCamMgr::setPitch( const LLUUID& source, F32 pitch)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setPitch(pitch);
	}
}
void LLFollowCamMgr::setFocusOffset( const LLUUID& source, const LLVector3& offset)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setFocusOffset(offset);
	}
}
void LLFollowCamMgr::setBehindnessAngle( const LLUUID& source, F32 angle)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setBehindnessAngle(angle);
	}
}
void LLFollowCamMgr::setBehindnessLag( const LLUUID& source, F32 force)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setBehindnessLag(force);
	}
}
void LLFollowCamMgr::setPosition( const LLUUID& source, const LLVector3 position)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setPosition(position);
	}
}
void LLFollowCamMgr::setFocus( const LLUUID& source, const LLVector3 focus)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setFocus(focus);
	}
}
void LLFollowCamMgr::setPositionLocked( const LLUUID& source, bool locked)
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setPositionLocked(locked);
	}
}
void LLFollowCamMgr::setFocusLocked( const LLUUID& source, bool locked )
{
	LLFollowCamParams* paramsp = getParamsForID(source);
	if (paramsp)
	{
		paramsp->setFocusLocked(locked);
	}
}
LLFollowCamParams* LLFollowCamMgr::getParamsForID(const LLUUID& source)
{
	LLFollowCamParams* params = NULL;
	param_map_t::iterator found_it = sParamMap.find(source);
	if (found_it == sParamMap.end())
	{
		params = new LLFollowCamParams();
		sParamMap[source] = params;
	}
	else
	{
		params = found_it->second;
	}
	return params;
}
LLFollowCamParams* LLFollowCamMgr::getActiveFollowCamParams()
{
	if (sParamStack.empty())
	{
		return NULL;
	}
	return sParamStack.back();
}
void LLFollowCamMgr::setCameraActive( const LLUUID& source, bool active )
{
	LLFollowCamParams* params = getParamsForID(source);
	param_stack_t::iterator found_it = std::find(sParamStack.begin(), sParamStack.end(), params);
	if (found_it != sParamStack.end())
	{
		sParamStack.erase(found_it);
	}
	if(active)
	{
		sParamStack.push_back(params);
	}
}
void LLFollowCamMgr::removeFollowCamParams(const LLUUID& source)
{
	setCameraActive(source, FALSE);
	LLFollowCamParams* params = getParamsForID(source);
	sParamMap.erase(source);
	delete params;
}
bool LLFollowCamMgr::isScriptedCameraSource(const LLUUID& source)
{
	param_map_t::iterator found_it = sParamMap.find(source);
	return (found_it != sParamMap.end());
}
void LLFollowCamMgr::dump()
{
	S32 param_count = 0;
	LL_INFOS() << "Scripted camera active stack" << LL_ENDL;
	for (param_stack_t::iterator param_it = sParamStack.begin();
		param_it != sParamStack.end();
		++param_it)
	{
		LL_INFOS() << param_count++ <<
			" rot_limit: " << (*param_it)->getBehindnessAngle() <<
			" rot_lag: " << (*param_it)->getBehindnessLag() <<
			" distance: " << (*param_it)->getDistance() <<
			" focus: " << (*param_it)->getFocus() <<
			" foc_lag: " << (*param_it)->getFocusLag() <<
			" foc_lock: " << ((*param_it)->getFocusLocked() ? "Y" : "N") <<
			" foc_offset: " << (*param_it)->getFocusOffset() <<
			" foc_thresh: " << (*param_it)->getFocusThreshold() <<
			" pitch: " << (*param_it)->getPitch() <<
			" pos: " << (*param_it)->getPosition() <<
			" pos_lag: " << (*param_it)->getPositionLag() <<
			" pos_lock: " << ((*param_it)->getPositionLocked() ? "Y" : "N") <<
			" pos_thresh: " << (*param_it)->getPositionThreshold() << LL_ENDL;
	}
}
