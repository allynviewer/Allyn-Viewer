/** 
 * @file llpolymorph.h
 * @brief Implementation of LLPolyMesh class
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
#ifndef LL_LLPOLYMORPH_H
#define LL_LLPOLYMORPH_H
#include <string>
#include <vector>
#include "llviewervisualparam.h"
class LLAvatarJointCollisionVolume;
class LLPolyMeshSharedData;
class LLVector2;
class LLAvatarJointCollisionVolume;
class LLWearable;
LL_ALIGN_PREFIX(16)
class LLPolyMorphData
{
public:
	LLPolyMorphData(const std::string& morph_name);
	~LLPolyMorphData();
	LLPolyMorphData(const LLPolyMorphData &rhs);
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
	BOOL			loadBinary(LLFILE* fp, LLPolyMeshSharedData *mesh);
	const std::string& getName() { return mName; }
	BOOL			saveLLM(LLFILE *fp);
	BOOL			saveOBJ(LLFILE *fp);
	BOOL			setMorphFromMesh(LLPolyMesh *morph);
public:
	std::string			mName;
	U32					mNumIndices;
	U32*				mVertexIndices;
	U32					mCurrentIndex;
	LLVector4a*			mCoords;
	LLVector4a*			mNormals;
	LLVector4a*			mBinormals;
	LLVector2*			mTexCoords;
	F32					mTotalDistortion;
	F32					mMaxDistortion;
	LL_ALIGN_16(LLVector4a			mAvgDistortion);
	LLPolyMeshSharedData*	mMesh;
private:
	void freeData();
} LL_ALIGN_POSTFIX(16);
class LLPolyVertexMask
{
public:
	LLPolyVertexMask(LLPolyMorphData* morph_data);
	LLPolyVertexMask(const LLPolyVertexMask& pOther);
	~LLPolyVertexMask();
	void generateMask(U8 *maskData, S32 width, S32 height, S32 num_components, BOOL invert, LLVector4a *clothing_weights);
	F32* getMorphMaskWeights();
protected:
	F32*		mWeights;
	LLPolyMorphData *mMorphData;
	BOOL			mWeightsGenerated;
};
struct LLPolyVolumeMorphInfo
{
	LLPolyVolumeMorphInfo(std::string &name, LLVector3 &scale, LLVector3 &pos)
		: mName(name), mScale(scale), mPos(pos) {};
	std::string						mName;
	LLVector3						mScale;
	LLVector3						mPos;
};
struct LLPolyVolumeMorph
{
	LLPolyVolumeMorph(LLAvatarJointCollisionVolume* volume, LLVector3 scale, LLVector3 pos)
		: mVolume(volume), mScale(scale), mPos(pos) {};
	LLAvatarJointCollisionVolume*	mVolume;
	LLVector3						mScale;
	LLVector3						mPos;
};
class LLPolyMorphTargetInfo : public LLViewerVisualParamInfo
{
	friend class LLPolyMorphTarget;
public:
	LLPolyMorphTargetInfo();
	~LLPolyMorphTargetInfo() {};
	BOOL parseXml(LLXmlTreeNode* node);
protected:
	std::string		mMorphName;
	BOOL			mIsClothingMorph;
	typedef std::vector<LLPolyVolumeMorphInfo> volume_info_list_t;
	volume_info_list_t mVolumeInfoList;
};
class LLPolyMorphTarget : public LLViewerVisualParam
{
public:
	LLPolyMorphTarget(LLPolyMesh *poly_mesh);
	~LLPolyMorphTarget();
	LLPolyMorphTargetInfo*	getInfo() const { return (LLPolyMorphTargetInfo*)mInfo; }
	BOOL					setInfo(LLPolyMorphTargetInfo *info);
	LLViewerVisualParam* cloneParam(LLWearable* wearable) const;
	void				apply( ESex sex );
	char const*			getTypeString(void) const { return "param_morph"; }
	F32					getTotalDistortion();
	const LLVector4a&	getAvgDistortion();
	F32					getMaxDistortion();
	LLVector4a			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh);
	const LLVector4a*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh);
	const LLVector4a*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh);
	void	applyMask(U8 *maskData, S32 width, S32 height, S32 num_components, BOOL invert);
	void	addPendingMorphMask() { mNumMorphMasksPending++; }
	void	applyVolumeChanges(F32 delta_weight);
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
protected:
	LLPolyMorphTarget(const LLPolyMorphTarget& pOther);
	LLPolyMorphData*				mMorphData;
	LLPolyMesh*						mMesh;
	LLPolyVertexMask *				mVertMask;
	ESex							mLastSex;
	BOOL							mNumMorphMasksPending;
	typedef std::vector<LLPolyVolumeMorph> volume_list_t;
	volume_list_t 					mVolumeMorphs;
};
#endif
