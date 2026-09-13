/** 
 * @file llvotree.h
 * @brief LLVOTree class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LL_LLVOTREE_H
#define LL_LLVOTREE_H
#include "llviewerobject.h"
#include "xform.h"
class LLFace;
class LLDrawPool;
class LLSelectNode;
class LLViewerFetchedTexture;
class LLVOTree : public LLViewerObject
{
protected:
	~LLVOTree();
public:
	enum
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0)
	};
	LLVOTree(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	static void initClass();
	static void cleanupClass();
	static bool isTreeRenderingStopped();
	U32 processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);
	void idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	void render(LLAgent &agent);
	void setPixelAreaAndAngle(LLAgent &agent);
	void updateTextures();
	LLDrawable* createDrawable(LLPipeline *pipeline);
	BOOL		updateGeometry(LLDrawable *drawable);
	void		updateSpatialExtents(LLVector4a &min, LLVector4a &max);
	void resetVertexBuffers();
	virtual U32 getPartitionType() const;
	void updateRadius();
	void calcNumVerts(U32& vert_count, U32& index_count, S32 trunk_LOD, S32 stop_level, U16 depth, U16 trunk_depth, F32 branches);
	void updateMesh();
	void appendMesh(LLStrider<LLVector4a>& vertices,
						 LLStrider<LLVector4a>& normals,
						 LLStrider<LLVector2>& tex_coords,
						 LLStrider<U16>& indices,
						 U16& idx_offset,
						 LLMatrix4a& matrix,
						 LLMatrix4a& norm_mat,
						 S32 vertex_offset,
						 S32 vertex_count,
						 S32 index_count,
						 S32 index_offset);
	void genBranchPipeline(LLStrider<LLVector4a>& vertices,
								 LLStrider<LLVector4a>& normals,
								 LLStrider<LLVector2>& tex_coords,
								 LLStrider<U16>& indices,
								 U16& index_offset,
								 LLMatrix4a& matrix,
								 S32 trunk_LOD,
								 S32 stop_level,
								 U16 depth,
								 U16 trunk_depth,
								 F32 scale,
								 F32 twist,
								 F32 droop,
								 F32 branches,
								 F32 alpha);
	 BOOL lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
										  S32 face = -1,
										  BOOL pick_transparent = FALSE,
										  BOOL pick_rigged = FALSE,
										  S32* face_hit = NULL,
										  LLVector4a* intersection = NULL,
										  LLVector2* tex_coord = NULL,
										  LLVector4a* normal = NULL,
										  LLVector4a* tangent = NULL
		);
	void generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point);
	static S32 sMaxTreeSpecies;
	struct TreeSpeciesData
	{
		LLUUID mTextureID;
		F32				mBranchLength;
		F32				mDroop;
		F32				mTwist;
		F32				mBranches;
		U8				mDepth;
		F32				mScaleStep;
		U8				mTrunkDepth;
		F32				mLeafScale;
		F32				mTrunkLength;
		F32				mBillboardScale;
		F32				mBillboardRatio;
		F32				mTrunkAspect;
		F32				mBranchAspect;
		F32				mRandomLeafRotate;
		F32				mNoiseScale;
		F32				mNoiseMag;
		F32				mTaper;
		F32				mRepeatTrunkZ;
	};
	static F32 sTreeFactor;
	typedef std::map<std::string, S32> SpeciesNames;
	static SpeciesNames sSpeciesNames;
	static const S32 sMAX_NUM_TREE_LOD_LEVELS ;
	friend class LLDrawPoolTree;
protected:
	LLVector3		mTrunkBend;
	LLVector3		mTrunkVel;
	LLVector3		mWind;
	LLPointer<LLVertexBuffer> mReferenceBuffer;
	LLPointer<LLViewerFetchedTexture> mTreeImagep;
	U8				mSpecies;
	F32				mBranchLength;
	F32				mTrunkLength;
	F32				mDroop;
	F32				mTwist;
	F32				mBranches;
	U8				mDepth;
	F32				mScaleStep;
	U8				mTrunkDepth;
	U32				mTrunkLOD;
	F32				mLeafScale;
	F32				mBillboardScale;
	F32				mBillboardRatio;
	F32				mTrunkAspect;
	F32				mBranchAspect;
	F32				mRandomLeafRotate;
	LLVector3 mLastPosition;
	LLQuaternion mLastRotation;
	U32 mFrameCount;
	std::vector<LLPointer<LLDrawInfo> > mDrawList;
	typedef std::map<U32, TreeSpeciesData*> SpeciesMap;
	static SpeciesMap sSpeciesTable;
	static S32 sLODIndexOffset[4];
	static S32 sLODIndexCount[4];
	static S32 sLODVertexOffset[4];
	static S32 sLODVertexCount[4];
	static S32 sLODSlices[4];
	static F32 sLODAngles[4];
private:
	void generateSilhouetteVertices(std::vector<LLVector3> &vertices,
									std::vector<LLVector3> &normals,
									const LLVector3& view_vec,
									const LLMatrix4a& mat,
									const LLMatrix4a& norm_mat);
};
#endif
