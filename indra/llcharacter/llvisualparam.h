/** 
 * @file llvisualparam.h
 * @brief Implementation of LLPolyMesh class.
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
#ifndef LL_LLVisualParam_H
#define LL_LLVisualParam_H
#include "v3math.h"
#include "llstring.h"
#include "llxmltree.h"
class LLPolyMesh;
class LLXmlTreeNode;
enum ESex
{
	SEX_FEMALE =	0x01,
	SEX_MALE =		0x02,
	SEX_BOTH =		0x03
};
enum EVisualParamGroup
{
	VISUAL_PARAM_GROUP_TWEAKABLE,
	VISUAL_PARAM_GROUP_ANIMATABLE,
	VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT,
	VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE,
	NUM_VISUAL_PARAM_GROUPS
};
enum EParamLocation
{
	LOC_UNKNOWN,
	LOC_AV_SELF,
	LOC_AV_OTHER,
	LOC_WEARABLE
};
const std::string param_location_name(const EParamLocation& loc);
const S32 MAX_TRANSMITTED_VISUAL_PARAMS = 255;
class LLVisualParamInfo
{
	friend class LLVisualParam;
public:
	LLVisualParamInfo();
	virtual ~LLVisualParamInfo() {};
	virtual BOOL parseXml(LLXmlTreeNode *node);
	S32 getID() const { return mID; }
	virtual void toStream(std::ostream &out);
protected:
	S32					mID;
	std::string			mName;
	std::string			mDisplayName;
	std::string			mMinName;
	std::string			mMaxName;
	EVisualParamGroup	mGroup;
	F32					mMinWeight;
	F32					mMaxWeight;
	F32					mDefaultWeight;
	ESex				mSex;
};
LL_ALIGN_PREFIX(16)
class LLVisualParam
{
public:
	typedef	std::function<LLVisualParam*(S32)> visual_param_mapper;
	LLVisualParam();
	virtual ~LLVisualParam();
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
	LLVisualParamInfo*		getInfo() const { return mInfo; }
	BOOL					setInfo(LLVisualParamInfo *info);
	virtual void			apply( ESex avatar_sex ) = 0;
	virtual void			setWeight(F32 weight, bool upload_bake = false);
	virtual void			setAnimationTarget(F32 target_value, bool upload_bake = false);
	virtual void			animate(F32 delta, bool upload_bake = false);
	virtual void			stopAnimating(bool upload_bake = false);
	virtual BOOL			linkDrivenParams(visual_param_mapper mapper, BOOL only_cross_params);
	virtual void			resetDrivenParams();
	S32						getID() const		{ return mID; }
	void					setID(S32 id) 		{ llassert(!mInfo); mID = id; }
	const std::string&		getName() const 			{ return mInfo->mName; }
	const std::string&		getDisplayName() const 		{ return mInfo->mDisplayName; }
	const std::string&		getMaxDisplayName() const	{ return mInfo->mMaxName; }
	const std::string&		getMinDisplayName() const	{ return mInfo->mMinName; }
	void					setDisplayName(const std::string& s) 	 { mInfo->mDisplayName = s; }
	void					setMaxDisplayName(const std::string& s) { mInfo->mMaxName = s; }
	void					setMinDisplayName(const std::string& s) { mInfo->mMinName = s; }
	EVisualParamGroup		getGroup() const 			{ return mInfo->mGroup; }
	F32						getMinWeight() const		{ return mInfo->mMinWeight; }
	F32						getMaxWeight() const		{ return mInfo->mMaxWeight; }
	F32						getDefaultWeight() const 	{ return mInfo->mDefaultWeight; }
	ESex					getSex() const			{ return mInfo->mSex; }
	F32						getWeight() const		{ return mIsAnimating ? mTargetWeight : mCurWeight; }
	F32						getCurrentWeight() const 	{ return mCurWeight; }
	F32						getLastWeight() const	{ return mLastWeight; }
	BOOL					isAnimating() const	{ return mIsAnimating; }
	BOOL					isTweakable() const { return (getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)  || (getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT); }
	LLVisualParam*			getNextParam()		{ return mNext; }
	void					setNextParam( LLVisualParam *next );
	void					clearNextParam();
	virtual void			setAnimating(BOOL is_animating) { mIsAnimating = is_animating && !mIsDummy; }
	BOOL					getAnimating() const { return mIsAnimating; }
	void					setIsDummy(BOOL is_dummy) { mIsDummy = is_dummy; }
	void					setParamLocation(EParamLocation loc);
	EParamLocation			getParamLocation() const { return mParamLocation; }
	virtual char const*		getTypeString(void) const = 0;
	virtual std::string		getDumpWearableTypeName(void) const = 0;
protected:
	LLVisualParam(const LLVisualParam& pOther);
	F32					mCurWeight;
	F32					mLastWeight;
	LLVisualParam*		mNext;
	F32					mTargetWeight;
	BOOL				mIsAnimating;
	BOOL				mIsDummy;
	S32					mID;
	LLVisualParamInfo	*mInfo;
	EParamLocation		mParamLocation;
} LL_ALIGN_POSTFIX(16);
#endif
