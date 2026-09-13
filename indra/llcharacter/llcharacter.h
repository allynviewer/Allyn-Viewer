/** 
 * @file llcharacter.h
 * @brief Implementation of LLCharacter class.
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
#ifndef LL_LLCHARACTER_H
#define LL_LLCHARACTER_H
#include <string>
#include "lljoint.h"
#include "llmotioncontroller.h"
#include "llvisualparam.h"
#include "llstringtable.h"
#include "llpointer.h"
#include "llthread.h"
#include "llsortedvector.h"
#include <boost/unordered_map.hpp>
class LLPolyMesh;
class LLPauseRequestHandle : public LLThreadSafeRefCount
{
public:
	LLPauseRequestHandle() {};
};
typedef LLPointer<LLPauseRequestHandle> LLAnimPauseRequest;
class LLCharacter
{
public:
	LLCharacter();
	virtual ~LLCharacter();
	virtual const char *getAnimationPrefix() = 0;
	virtual LLJoint *getRootJoint() = 0;
	virtual LLJoint *getJoint( const std::string &name );
	virtual LLVector3 getCharacterPosition() = 0;
	virtual LLQuaternion getCharacterRotation() = 0;
	virtual LLVector3 getCharacterVelocity() = 0;
	virtual LLVector3 getCharacterAngularVelocity() = 0;
	virtual void getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm) = 0;
	virtual LLJoint *getCharacterJoint( U32 i ) = 0;
	virtual F32 getTimeDilation() = 0;
	virtual F32 getPixelArea() const = 0;
	virtual LLPolyMesh*	getHeadMesh() = 0;
	virtual LLPolyMesh*	getUpperBodyMesh() = 0;
	virtual LLVector3d	getPosGlobalFromAgent(const LLVector3 &position) = 0;
	virtual LLVector3	getPosAgentFromGlobal(const LLVector3d &position) = 0;
	virtual void updateVisualParams();
	virtual void addDebugText( const std::string& text ) = 0;
	virtual const LLUUID&	getID() const = 0;
	BOOL registerMotion( const LLUUID& id, LLMotionConstructor create );
	void removeMotion( const LLUUID& id );
	LLMotion* createMotion( const LLUUID &id );
	LLMotion* findMotion( const LLUUID &id );
	virtual BOOL startMotion( const LLUUID& id, F32 start_offset = 0.f);
	virtual BOOL stopMotion( const LLUUID& id, BOOL stop_immediate = FALSE );
	BOOL isMotionActive( const LLUUID& id );
	bool isMotionActive(U32 bit) const { return mMotionController.isactive(bit); }
	virtual void requestStopMotion( LLMotion* motion );
	enum e_update_t { NORMAL_UPDATE, HIDDEN_UPDATE, FORCE_UPDATE };
	void updateMotions(e_update_t update_type);
	LLAnimPauseRequest requestPause();
	void requestPause(std::vector<LLAnimPauseRequest>& avatar_pause_handles);
	void pauseAllSyncedCharacters(std::vector<LLAnimPauseRequest>& avatar_pause_handles);
	BOOL areAnimationsPaused() const { return mMotionController.isPaused(); }
	void setAnimTimeFactor(F32 factor) { mMotionController.setTimeFactor(factor); }
	void setTimeStep(F32 time_step) { mMotionController.setTimeStep(time_step); }
	LLMotionController& getMotionController() { return mMotionController; }
	virtual void flushAllMotions();
	virtual void deactivateAllMotions();
	virtual void dumpCharacter( LLJoint *joint = NULL );
	virtual F32 getPreferredPelvisHeight() { return mPreferredPelvisHeight; }
	virtual LLVector3 getVolumePos(S32 joint_index, LLVector3& volume_offset) { return LLVector3::zero; }
	virtual LLJoint* findCollisionVolume(S32 volume_id) { return NULL; }
	virtual S32 getCollisionVolumeID(std::string &name) { return -1; }
	void setAnimationData(std::string name, void *data);
	void *getAnimationData(std::string name);
	void removeAnimationData(std::string name);
	void addVisualParam(LLVisualParam *param);
	void addSharedVisualParam(LLVisualParam *param);
	virtual BOOL setVisualParamWeight(const LLVisualParam *which_param, F32 weight, bool upload_bake = false );
	virtual BOOL setVisualParamWeight(const char* param_name, F32 weight, bool upload_bake = false );
	virtual BOOL setVisualParamWeight(S32 index, F32 weight, bool upload_bake = false );
	F32 getVisualParamWeight(LLVisualParam *distortion);
	F32 getVisualParamWeight(const char* param_name);
	F32 getVisualParamWeight(S32 index);
	void clearVisualParamWeights();
	BOOL visualParamWeightsAreDefault();
	LLVisualParam*	getFirstVisualParam()
	{
		mCurIterator = mVisualParamSortedVector.begin();
		return getNextVisualParam();
	}
	LLVisualParam*	getNextVisualParam()
	{
		if (mCurIterator == mVisualParamSortedVector.end())
			return 0;
		return (mCurIterator++)->second;
	}
	S32 getVisualParamCountInGroup(const EVisualParamGroup group) const
	{
		S32 rtn = 0;
		for (visual_param_sorted_vec_t::const_iterator iter = mVisualParamSortedVector.begin();
		     iter != mVisualParamSortedVector.end();
		     )
		{
			if ((iter++)->second->getGroup() == group)
			{
				++rtn;
			}
		}
		return rtn;
	}
	LLVisualParam*	getVisualParam(S32 id) const
	{
		visual_param_index_map_t::const_iterator iter = mVisualParamIndexMap.find(id);
		return (iter == mVisualParamIndexMap.end()) ? 0 : iter->second;
	}
	S32				getVisualParamCount() const { return (S32)mVisualParamIndexMap.size(); }
	LLVisualParam*	getVisualParam(const char *name);
	ESex getSex() const			{ return mSex; }
	void setSex( ESex sex )		{ mSex = sex; }
	U32				getAppearanceSerialNum() const		{ return mAppearanceSerialNum; }
	void			setAppearanceSerialNum( U32 num )	{ mAppearanceSerialNum = num; }
	U32				getSkeletonSerialNum() const		{ return mSkeletonSerialNum; }
	void			setSkeletonSerialNum( U32 num )	{ mSkeletonSerialNum = num; }
	static std::vector< LLCharacter* > sInstances;
	virtual void	setHoverOffset(const LLVector3& hover_offset, bool send_update=true) { mHoverOffset = hover_offset; }
	const LLVector3& getHoverOffset() const { return mHoverOffset; }
protected:
	LLMotionController	mMotionController;
	typedef std::map<std::string, void *> animation_data_map_t;
	animation_data_map_t mAnimationData;
	F32					mPreferredPelvisHeight;
	ESex				mSex;
	U32					mAppearanceSerialNum;
	U32					mSkeletonSerialNum;
	LLAnimPauseRequest	mPauseRequest;
private:
	typedef boost::unordered_map<S32, LLVisualParam *> 		visual_param_index_map_t;
	typedef LLSortedVector<S32,LLVisualParam *>				visual_param_sorted_vec_t;
	typedef std::map<char *, LLVisualParam *> 				visual_param_name_map_t;
	visual_param_sorted_vec_t::iterator 			mCurIterator;
	visual_param_sorted_vec_t						mVisualParamSortedVector;
	visual_param_index_map_t 						mVisualParamIndexMap;
	visual_param_name_map_t  						mVisualParamNameMap;
	static LLStringTable sVisualParamNames;
	LLVector3 mHoverOffset;
};
#endif
