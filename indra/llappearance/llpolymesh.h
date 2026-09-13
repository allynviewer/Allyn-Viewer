/** 
 * @file llpolymesh.h
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
#ifndef LL_LLPOLYMESHINTERFACE_H
#define LL_LLPOLYMESHINTERFACE_H
#include <string>
#include <map>
#include "llstl.h"
#include "v3math.h"
#include "v2math.h"
#include "llquaternion.h"
#include "llpolymorph.h"
#include "lljoint.h"
class LLSkinJoint;
class LLAvatarAppearance;
class LLWearable;
typedef S32 LLPolyFace[3];
class LLPolyMorphTarget;
class LLPolyMeshSharedData
{
	friend class LLPolyMesh;
private:
	LLVector3				mPosition;
	LLQuaternion			mRotation;
	LLVector3				mScale;
	S32						mNumVertices;
	LLVector4a				*mBaseCoords;
	LLVector4a				*mBaseNormals;
	LLVector4a				*mBaseBinormals;
	LLVector2				*mTexCoords;
	LLVector2				*mDetailTexCoords;
	F32						*mWeights;
	BOOL					mHasWeights;
	BOOL					mHasDetailTexCoords;
	S32						mNumFaces;
	LLPolyFace				*mFaces;
	U32						mNumJointNames;
	std::string*			mJointNames;
	typedef std::set<LLPolyMorphData*> morphdata_list_t;
	morphdata_list_t			mMorphData;
	std::map<S32, S32> 			mSharedVerts;
	LLPolyMeshSharedData*		mReferenceData;
	S32							mLastIndexOffset;
public:
	U32				mNumTriangleIndices;
	U32				*mTriangleIndices;
public:
	LLPolyMeshSharedData();
	~LLPolyMeshSharedData();
private:
	void setupLOD(LLPolyMeshSharedData* reference_data);
	void freeMeshData();
	void setPosition( const LLVector3 &pos ) { 	mPosition = pos; }
	void setRotation( const LLQuaternion &rot ) { mRotation = rot; }
	void setScale( const LLVector3 &scale ) { mScale = scale; }
	BOOL allocateVertexData( U32 numVertices );
	BOOL allocateFaceData( U32 numFaces );
	BOOL allocateJointNames( U32 numJointNames );
	U32 getNumKB();
	BOOL loadMesh( const std::string& fileName );
public:
	void genIndices(S32 offset);
	const LLVector2 &getUVs(U32 index);
	const S32	*getSharedVert(S32 vert);
	BOOL isLOD() { return (mReferenceData != NULL); }
};
class LLJointRenderData
{
public:
	LLJointRenderData(const LLMatrix4a* world_matrix, LLSkinJoint* skin_joint) : mWorldMatrix(world_matrix), mSkinJoint(skin_joint) {}
	~LLJointRenderData(){}
	const LLMatrix4a*		mWorldMatrix;
	LLSkinJoint*			mSkinJoint;
};
class LLPolyMesh
{
public:
	LLPolyMesh(LLPolyMeshSharedData *shared_data, LLPolyMesh *reference_mesh);
	~LLPolyMesh();
	static LLPolyMesh *getMesh( const std::string &name, LLPolyMesh* reference_mesh = NULL);
	BOOL saveLLM(LLFILE *fp);
	BOOL saveOBJ(LLFILE *fp);
	BOOL loadOBJ(LLFILE *fp);
	BOOL setSharedFromCurrent();
	static const std::string* getSharedMeshName(LLPolyMeshSharedData* shared);
	static LLPolyMeshSharedData *getMeshData( const std::string &name );
	static void freeAllMeshes();
	const LLVector3 &getPosition() {
		llassert (mSharedData);
		return mSharedData->mPosition;
	}
	const LLQuaternion &getRotation() {
		llassert (mSharedData);
		return mSharedData->mRotation;
	}
	const LLVector3 &getScale() {
		llassert (mSharedData);
		return mSharedData->mScale;
	}
	U32 getNumVertices() {
		llassert (mSharedData);
		return mSharedData->mNumVertices;
	}
	BOOL hasDetailTexCoords() {
		llassert (mSharedData);
		return mSharedData->mHasDetailTexCoords;
	}
	BOOL hasWeights() const{
		llassert (mSharedData);
		return mSharedData->mHasWeights;
	}
	const LLVector4a	*getCoords() const{
		return mCoords;
	}
	LLVector4a *getWritableCoords();
	const LLVector4a	*getNormals() const{
		return mNormals;
	}
	const LLVector4a	*getBinormals() const{
		return mBinormals;
	}
	const LLVector4a *getBaseNormals() const{
		llassert(mSharedData);
		return mSharedData->mBaseNormals;
	}
	const LLVector4a *getBaseBinormals() const{
		llassert(mSharedData);
		return mSharedData->mBaseBinormals;
	}
	LLVector4a *getWritableNormals();
	LLVector4a *getScaledNormals();
	LLVector4a *getWritableBinormals();
	LLVector4a *getScaledBinormals();
	const LLVector2	*getTexCoords() const {
		return mTexCoords;
	}
	LLVector2 *getWritableTexCoords();
	const LLVector2	*getDetailTexCoords() const {
		llassert (mSharedData);
		return mSharedData->mDetailTexCoords;
	}
	const F32 *getWeights() const {
		llassert (mSharedData);
		return mSharedData->mWeights;
	}
	F32			*getWritableWeights() const;
	LLVector4a	*getWritableClothingWeights();
	const LLVector4a		*getClothingWeights()
	{
		return mClothingWeights;
	}
	S32 getNumFaces() {
		llassert (mSharedData);
		return mSharedData->mNumFaces;
	}
	LLPolyFace *getFaces() {
		llassert (mSharedData);
		return mSharedData->mFaces;
	}
	U32 getNumJointNames() {
		llassert (mSharedData);
		return mSharedData->mNumJointNames;
	}
	std::string *getJointNames() {
		llassert (mSharedData);
		return mSharedData->mJointNames;
	}
	typedef std::map<std::string,LLPolyMorphData*> morph_list_t;
	static void getMorphList (const std::string& mesh_name, morph_list_t* morph_list);
	LLPolyMorphData*	getMorphData(const std::string& morph_name);
	LLPolyMeshSharedData *getSharedData() const;
	LLPolyMesh *getReferenceMesh() { return mReferenceMesh ? mReferenceMesh : this; }
	U32*	getIndices() { return mSharedData ? mSharedData->mTriangleIndices : NULL; }
	BOOL	isLOD() { return mSharedData && mSharedData->isLOD(); }
	void setAvatar(LLAvatarAppearance* avatarp) { mAvatarp = avatarp; }
	LLAvatarAppearance* getAvatar() { return mAvatarp; }
	std::vector<LLJointRenderData*>	mJointRenderData;
	U32				mFaceVertexOffset;
	U32				mFaceVertexCount;
	U32				mFaceIndexOffset;
	U32				mFaceIndexCount;
	U32				mCurVertexCount;
	static void dumpDiagInfo(void*);
private:
	void initializeForMorph();
protected:
	LLPolyMeshSharedData	*mSharedData;
	F32						*mVertexData;
	LLVector4a				*mCoords;
	LLVector4a				*mScaledNormals;
	LLVector4a				*mNormals;
	LLVector4a				*mScaledBinormals;
	LLVector4a				*mBinormals;
	LLVector4a				*mClothingWeights;
	LLVector2				*mTexCoords;
	LLPolyMesh				*mReferenceMesh;
	typedef std::map<std::string, LLPolyMeshSharedData*> LLPolyMeshSharedDataTable;
	static LLPolyMeshSharedDataTable sGlobalSharedMeshList;
	LLAvatarAppearance* mAvatarp;
};
#endif
