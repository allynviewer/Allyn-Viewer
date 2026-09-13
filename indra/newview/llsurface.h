/** 
 * @file llsurface.h
 * @brief Description of LLSurface class
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_LLSURFACE_H
#define LL_LLSURFACE_H
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "v4coloru.h"
#include "v4color.h"
#include "llvowater.h"
#include "llpatchvertexarray.h"
#include "llviewertexture.h"
class LLTimer;
class LLUUID;
class LLAgent;
class LLStat;
static const U8 NO_EDGE    = 0x00;
static const U8 EAST_EDGE  = 0x01;
static const U8 NORTH_EDGE = 0x02;
static const U8 WEST_EDGE  = 0x04;
static const U8 SOUTH_EDGE = 0x08;
static const S32 ONE_MORE_THAN_NEIGHBOR	= 1;
static const S32 EQUAL_TO_NEIGHBOR 		= 0;
static const S32 ONE_LESS_THAN_NEIGHBOR	= -1;
const S32 ABOVE_WATERLINE_ALPHA = 32;
class LLViewerRegion;
class LLBitPack;
class LLGroupHeader;
class LLSurfacePatch;
typedef std::shared_ptr<LLSurfacePatch> surface_patch_ref;
typedef std::weak_ptr<LLSurfacePatch> surface_patch_weak_ref;
class LLSurface
{
public:
	LLSurface(U32 type, LLViewerRegion *regionp = NULL);
	virtual ~LLSurface();
	static void initClasses();
	void create(const S32 surface_grid_width,
				const S32 surface_patch_width,
				const LLVector3d &origin_global,
				const F32 width);
	void setRegion(LLViewerRegion *regionp);
	void setOriginGlobal(const LLVector3d &origin_global);
	void connectNeighbor(LLSurface *neighborp, U32 direction);
	void disconnectNeighbor(LLSurface *neighborp, U32 direction);
	void disconnectAllNeighbors();
	void rebuildWater();
	virtual void decompressDCTPatch(LLBitPack &bitpack, LLGroupHeader *gopp, BOOL b_large_patch);
	virtual void updatePatchVisibilities(LLAgent &agent);
	inline F32 getZ(const U32 k) const				{ return mSurfaceZ[k]; }
	inline F32 getZ(const S32 i, const S32 j) const	{ return mSurfaceZ[i + j*mGridsPerEdge]; }
	LLVector3 getOriginAgent() const;
	const LLVector3d &getOriginGlobal() const;
	F32 getMetersPerGrid() const;
	S32 getGridsPerEdge() const;
	S32 getPatchesPerEdge() const;
	S32 getGridsPerPatchEdge() const;
	U32 getRenderStride(const U32 render_level) const;
	U32 getRenderLevel(const U32 render_stride) const;
	F32 resolveHeightRegion(const F32 x, const F32 y) const;
	F32 resolveHeightRegion(const LLVector3 &location) const
			{ return resolveHeightRegion( location.mV[VX], location.mV[VY] ); }
	F32 resolveHeightGlobal(const LLVector3d &position_global) const;
	LLVector3 resolveNormalGlobal(const LLVector3d& v) const;
	const surface_patch_ref& resolvePatchRegion(const F32 x, const F32 y) const;
	const surface_patch_ref& resolvePatchRegion(const LLVector3 &position_region) const;
	const surface_patch_ref& resolvePatchGlobal(const LLVector3d &position_global) const;
	BOOL idleUpdate(F32 max_update_time);
	BOOL containsPosition(const LLVector3 &position);
	void moveZ(const S32 x, const S32 y, const F32 delta);
	LLViewerRegion *getRegion() const				{ return mRegionp; }
	F32 getMinZ() const								{ return mMinZ; }
	F32 getMaxZ() const								{ return mMaxZ; }
	void setWaterHeight(F32 height);
	F32 getWaterHeight() const;
	LLViewerTexture *getSTexture();
	LLViewerTexture *getWaterTexture();
	BOOL hasZData() const							{ return mHasZData; }
	void dirtyAllPatches();
	void dirtySurfacePatch(const surface_patch_ref& patchp);
	LLVOWater *getWaterObj()						{ return mWaterObjp; }
	static void setTextureSize(const S32 texture_size);
	friend class LLSurfacePatch;
	friend std::ostream& operator<<(std::ostream &s, const LLSurface &S);
	void getNeighboringRegions( std::vector<LLViewerRegion*>& uniqueRegions );
	void getNeighboringRegionsStatus( std::vector<S32>& regions );
public:
	S32 mGridsPerEdge;
	F32 mOOGridsPerEdge;
	S32 mPatchesPerEdge;
	std::array<LLSurface*, 8> mNeighbors;
	U32 mType;
	F32 mDetailTextureScale;
	static F32 sTextureUpdateTime;
	static S32 sTexelsUpdated;
	static LLStat sTexelsUpdatedPerSecStat;
protected:
	void createSTexture();
	void createWaterTexture();
	void initTextures();
	void initWater();
	void createPatchData();
	void destroyPatchData();
	BOOL generateWaterTexture(const F32 x, const F32 y,
						const F32 width, const F32 height);
	const surface_patch_ref& getPatch(const S32 x, const S32 y) const;
protected:
	LLVector3d	mOriginGlobal;
	std::vector< surface_patch_ref > mPatchList;
	F32 *mSurfaceZ;
	LLVector3 *mNorm;
	std::vector< std::pair<U32, surface_patch_weak_ref >  > mDirtyPatchList;
	LLPointer<LLViewerTexture> mSTexturep;
	LLPointer<LLViewerTexture> mWaterTexturep;
	LLPointer<LLVOWater>	mWaterObjp;
	S32 mVisiblePatchCount;
	U32			mGridsPerPatchEdge;
	F32			mMetersPerGrid;
	F32			mMetersPerEdge;
	LLPatchVertexArray mPVArray;
	BOOL		mHasZData;
	F32			mMinZ;
	F32			mMaxZ;
	S32			mSurfacePatchUpdateCount;
private:
	LLViewerRegion *mRegionp;
	static S32	sTextureSize;
};
#endif
