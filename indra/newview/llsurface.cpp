/** 
 * @file llsurface.cpp
 * @brief Implementation of LLSurface class
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
#include "llviewerprecompiledheaders.h"
#include "llsurface.h"
#include "llrender.h"
#include "llviewertexturelist.h"
#include "llpatchvertexarray.h"
#include "patch_dct.h"
#include "patch_code.h"
#include "bitpack.h"
#include "llviewerobjectlist.h"
#include "llregionhandle.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llworld.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llsurfacepatch.h"
#include "llvosurfacepatch.h"
#include "llvowater.h"
#include "pipeline.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"
#include "llviewercamera.h"
#include "llglheaders.h"
#include "lldrawpoolterrain.h"
#include "lldrawable.h"
#include "hippogridmanager.h"
extern LLPipeline gPipeline;
extern bool gShiftFrame;
LLColor4U MAX_WATER_COLOR(0, 48, 96, 240);
S32 LLSurface::sTextureSize = 256;
S32 LLSurface::sTexelsUpdated = 0;
F32 LLSurface::sTextureUpdateTime = 0.f;
LLStat LLSurface::sTexelsUpdatedPerSecStat;
LLSurface::LLSurface(U32 type, LLViewerRegion *regionp) :
	mGridsPerEdge(0),
	mOOGridsPerEdge(0.f),
	mPatchesPerEdge(0),
	mType(type),
	mDetailTextureScale(0.f),
	mOriginGlobal(0.0, 0.0, 0.0),
	mSTexturep(NULL),
	mWaterTexturep(NULL),
	mGridsPerPatchEdge(0),
	mMetersPerGrid(1.0f),
	mMetersPerEdge(1.0f),
	mRegionp(regionp)
{
	mSurfaceZ = NULL;
	mNorm = NULL;
	mVisiblePatchCount = 0;
	mHasZData = FALSE;
	mMinZ = 10000.f;
	mMaxZ = -10000.f;
	mWaterObjp = NULL;
	mSurfacePatchUpdateCount = 0;
	for (S32 i = 0; i < 8; i++)
	{
		mNeighbors[i] = NULL;
	}
}
LLSurface::~LLSurface()
{
	delete [] mSurfaceZ;
	mSurfaceZ = NULL;
	delete [] mNorm;
	mGridsPerEdge = 0;
	mGridsPerPatchEdge = 0;
	mPatchesPerEdge = 0;
	destroyPatchData();
	LLDrawPoolTerrain *poolp = (LLDrawPoolTerrain*) gPipeline.findPool(LLDrawPool::POOL_TERRAIN, mSTexturep);
	if (!poolp)
	{
		LL_WARNS() << "No pool for terrain on destruction!" << LL_ENDL;
	}
	else if (poolp->mReferences.empty())
	{
		gPipeline.removePool(poolp);
		if (mSTexturep)
		{
			mSTexturep = NULL;
		}
		if (mWaterTexturep)
		{
			mWaterTexturep = NULL;
		}
	}
	else
	{
		LL_ERRS() << "Terrain pool not empty!" << LL_ENDL;
	}
}
void LLSurface::initClasses()
{
}
void LLSurface::setRegion(LLViewerRegion *regionp)
{
	mRegionp = regionp;
	mWaterObjp = NULL;
}
void LLSurface::create(const S32 grids_per_edge,
					   const S32 grids_per_patch_edge,
					   const LLVector3d &origin_global,
					   const F32 width)
{
	mGridsPerEdge = grids_per_edge + 1;
	mOOGridsPerEdge = 1.f / mGridsPerEdge;
	mGridsPerPatchEdge = grids_per_patch_edge;
	mPatchesPerEdge = grids_per_edge / mGridsPerPatchEdge;
	mMetersPerGrid = width / (F32)grids_per_edge;
	mMetersPerEdge = mMetersPerGrid * grids_per_edge;
	sTextureSize = width;
	mOriginGlobal.setVec(origin_global);
	mPVArray.create(mGridsPerEdge, mGridsPerPatchEdge, LLWorld::getInstance()->getRegionScale());
	S32 number_of_grids = mGridsPerEdge * mGridsPerEdge;
	mSurfaceZ = new F32[number_of_grids];
	mNorm = new LLVector3[number_of_grids];
	for(S32 i=0; i < number_of_grids; i++)
	{
		mSurfaceZ[i] = 0.0f;
		mNorm[i].setVec(0.f, 0.f, 1.f);
	}
	mVisiblePatchCount = 0;
	initTextures();
	createPatchData();
}
LLViewerTexture* LLSurface::getSTexture()
{
	if (mSTexturep.notNull() && !mSTexturep->hasGLTexture())
	{
		createSTexture();
	}
	return mSTexturep;
}
LLViewerTexture* LLSurface::getWaterTexture()
{
	if (mWaterTexturep.notNull() && !mWaterTexturep->hasGLTexture())
	{
		createWaterTexture();
	}
	return mWaterTexturep;
}
void LLSurface::createSTexture()
{
	if (!mSTexturep)
	{
		LLPointer<LLImageRaw> raw = new LLImageRaw(sTextureSize, sTextureSize, 3);
		U8 *default_texture = raw->getData();
		for (S32 i = 0; i < sTextureSize; i++)
		{
			for (S32 j = 0; j < sTextureSize; j++)
			{
				*(default_texture + (i*sTextureSize + j)*3) = 128;
				*(default_texture + (i*sTextureSize + j)*3 + 1) = 128;
				*(default_texture + (i*sTextureSize + j)*3 + 2) = 128;
			}
		}
		mSTexturep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
		mSTexturep->dontDiscard();
		gGL.getTexUnit(0)->bind(mSTexturep);
		mSTexturep->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
}
void LLSurface::createWaterTexture()
{
	if (!mWaterTexturep)
	{
		LLPointer<LLImageRaw> raw = new LLImageRaw(sTextureSize/2, sTextureSize/2, 4);
		U8 *default_texture = raw->getData();
		for (S32 i = 0; i < sTextureSize/2; i++)
		{
			for (S32 j = 0; j < sTextureSize/2; j++)
			{
				*(default_texture + (i*sTextureSize/2 + j)*4) = MAX_WATER_COLOR.mV[0];
				*(default_texture + (i*sTextureSize/2 + j)*4 + 1) = MAX_WATER_COLOR.mV[1];
				*(default_texture + (i*sTextureSize/2 + j)*4 + 2) = MAX_WATER_COLOR.mV[2];
				*(default_texture + (i*sTextureSize/2 + j)*4 + 3) = MAX_WATER_COLOR.mV[3];
			}
		}
		mWaterTexturep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
		mWaterTexturep->dontDiscard();
		gGL.getTexUnit(0)->bind(mWaterTexturep);
		mWaterTexturep->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
}
void LLSurface::initTextures()
{
	createSTexture();
	if (gSavedSettings.getBOOL("RenderWater") )
	{
		createWaterTexture();
		mWaterObjp = (LLVOWater *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_WATER, mRegionp);
		gPipeline.createObject(mWaterObjp);
		LLVector3d water_pos_global = from_region_handle(mRegionp->getHandle());
		water_pos_global += LLVector3d(mRegionp->getWidth()/2, mRegionp->getWidth()/2, DEFAULT_WATER_HEIGHT);
		mWaterObjp->setPositionGlobal(water_pos_global);
	}
}
void LLSurface::setOriginGlobal(const LLVector3d &origin_global)
{
	LLVector3d new_origin_global;
	mOriginGlobal = origin_global;
	S32 i, j;
	for (j=0; j<mPatchesPerEdge; j++)
	{
		for (i=0; i<mPatchesPerEdge; i++)
		{
			const auto& patchp = getPatch(i, j);
			new_origin_global = patchp->getOriginGlobal();
			new_origin_global.mdV[0] = mOriginGlobal.mdV[0] + i * mMetersPerGrid * mGridsPerPatchEdge;
			new_origin_global.mdV[1] = mOriginGlobal.mdV[1] + j * mMetersPerGrid * mGridsPerPatchEdge;
			patchp->setOriginGlobal(new_origin_global);
		}
	}
	if (mWaterObjp.notNull() && mWaterObjp->mDrawable.notNull())
	{
		const F64 x = origin_global.mdV[VX] + (F64)mRegionp->getWidth()/2;
		const F64 y = origin_global.mdV[VY] + (F64)mRegionp->getWidth()/2;
		const F64 z = mWaterObjp->getPositionGlobal().mdV[VZ];
		LLVector3d water_origin_global(x, y, z);
		mWaterObjp->setPositionGlobal(water_origin_global);
	}
}
void LLSurface::getNeighboringRegions( std::vector<LLViewerRegion*>& uniqueRegions )
{
	S32 i;
	for (i = 0; i < 8; i++)
	{
		if ( mNeighbors[i] != NULL )
		{
			uniqueRegions.push_back( mNeighbors[i]->getRegion() );
		}
	}
}
void LLSurface::getNeighboringRegionsStatus( std::vector<S32>& regions )
{
	S32 i;
	for (i = 0; i < 8; i++)
	{
		if ( mNeighbors[i] != NULL )
		{
			regions.push_back( i );
		}
	}
}
void LLSurface::connectNeighbor(LLSurface* neighborp, U32 direction)
{
	surface_patch_ref patchp, neighbor_patchp;
	if (mNeighbors[direction] == neighborp)
	{
		return;
	}
	if (mNeighbors[direction])
	{
		mNeighbors[direction]->disconnectNeighbor(this, gDirOpposite[direction]);
	}
	mNeighbors[direction] = neighborp;
	const S32 max_idx = mPatchesPerEdge - 1;
	const S32 neighbor_max_idx = neighborp->mPatchesPerEdge - 1;
	if (direction >= 4)
	{
		S32 patches[4][2] = {
			{max_idx, max_idx},
			{0, max_idx},
			{0, 0},
			{max_idx, 0},
		};
		const S32* p = patches[direction - 4];
		surface_patch_ref patchp = getPatch(p[0], p[1]);
		patchp->connectNeighbor(neighborp->getPatch(max_idx - p[0], max_idx - p[1]), direction);
		if (NORTHEAST == direction)
		{
			patchp->updateNorthEdge();
			if (patchp->dirtyZ())
			{
				dirtySurfacePatch(patchp);
			}
		}
}
	else
	{
		U32 pos[2][2] = { {0,0},{0,0} };
		from_region_handle(mRegionp->getHandle(), &pos[0][0], &pos[0][1]);
		from_region_handle(neighborp->getRegion()->getHandle(), &pos[1][0], &pos[1][1]);
		S32 width[2] = { (S32)mRegionp->getWidth(), (S32)neighborp->getRegion()->getWidth() };
		U32 mins[2] = { llmax(pos[0][0], pos[1][0]), llmax(pos[0][1], pos[1][1]) };
		U32 maxs[2] = { llmin(pos[0][0] + width[0], pos[1][0] + width[1]), llmin(pos[0][1] + width[0], pos[1][1] + width[1]) };
		S32 start[2][2] = {
			{S32((mins[0] - pos[0][0]) / mGridsPerPatchEdge) - 1, S32((mins[1] - pos[0][1]) / mGridsPerPatchEdge) - 1},
			{S32((mins[0] - pos[1][0]) / neighborp->mGridsPerPatchEdge) - 1,S32((mins[1] - pos[1][1]) / neighborp->mGridsPerPatchEdge) - 1}
		};
		S32 end[2] = { llmin(S32((maxs[0] - pos[0][0]) / mGridsPerPatchEdge), max_idx),  llmin(S32((maxs[1] - pos[0][1]) / mGridsPerPatchEdge), max_idx) };
		const U32& neighbor_direction = gDirOpposite[direction];
		S32 stride[4][4][2] = {
			{{0, 1}, {max_idx, 0}, {neighbor_max_idx, 0}, {NORTHEAST, SOUTHEAST}},
			{{1, 0}, {0, max_idx}, {0, neighbor_max_idx}, {NORTHEAST, NORTHWEST} },
			{{0, 1}, {0, 0}, {0, 0}, {NORTHWEST, SOUTHWEST}},
			{{1, 0}, {0, 0}, {0, 0}, {SOUTHEAST, SOUTHWEST}}
		};
		const S32 offs[2][2] = {
			{stride[direction][0][0], stride[direction][0][1]},
			{stride[neighbor_direction][0][0], stride[neighbor_direction][0][1]}
		};
		S32 x[2], y[2];
		x[0] = stride[direction][1][0] + offs[0][0] * start[0][0];
		y[0] = stride[direction][1][1] + offs[0][1] * start[0][1];
		x[1] = stride[neighbor_direction][2][0] + offs[1][0] * start[1][0];
		y[1] = stride[neighbor_direction][2][1] + offs[1][1] * start[1][1];
		for (
			x[0] = stride[direction][1][0] + offs[0][0] * start[0][0],
			y[0] = stride[direction][1][1] + offs[0][1] * start[0][1],
			x[1] = stride[neighbor_direction][2][0] + offs[1][0] * start[1][0],
			y[1] = stride[neighbor_direction][2][1] + offs[1][1] * start[1][1];
			(!offs[0][0] || x[0] <= end[0]) && (!offs[0][1] || (y[0] <= end[1]));
			x[0] += offs[0][0], y[0] += offs[0][1],
			x[1] += offs[1][0], y[1] += offs[1][1])
		{
			if (x[0] < 0 || y[0] < 0) {
				continue;
			}
			surface_patch_ref patchp = getPatch(x[0], y[0]);
			if ((offs[1][0] > 0 && x[1] > 0) || (offs[1][1] > 0 && y[1] > 0))
			{
				patchp->connectNeighbor(neighborp->getPatch(x[1] - offs[1][0], y[1] - offs[1][1]), stride[direction][3][1]);
			}
			if (x[1] >= 0 && y[1] >= 0 && x[1] <= neighbor_max_idx && y[1] <= neighbor_max_idx)
			{
				patchp->connectNeighbor(neighborp->getPatch(x[1], y[1]), direction);
			}
			if (x[1] + offs[1][0] <= neighbor_max_idx && y[1] + offs[1][1] <= neighbor_max_idx)
			{
				patchp->connectNeighbor(neighborp->getPatch(x[1] + offs[1][0], y[1] + offs[1][1]), stride[direction][3][0]);
			}
			if (direction == EAST)
			{
				patchp->updateEastEdge();
				if (patchp->dirtyZ())
				{
					dirtySurfacePatch(patchp);
				}
			}
			else if (direction == NORTH)
			{
				patchp->updateNorthEdge();
				if (patchp->dirtyZ())
				{
					dirtySurfacePatch(patchp);
				}
			}
		}
	}
}
void LLSurface::disconnectNeighbor(LLSurface* surfacep, U32 direction)
{
	if (surfacep && surfacep == mNeighbors[direction])
	{
		mNeighbors[direction] = NULL;
		for (auto& patchp : mPatchList)
		{
			patchp->disconnectNeighbor(surfacep);
		}
	}
}
void LLSurface::disconnectAllNeighbors()
{
	for (size_t i = 0; i < mNeighbors.size(); ++i)
	{
		auto& neighbor = mNeighbors[i];
		if (neighbor)
		{
			neighbor->disconnectNeighbor(this, gDirOpposite[i]);
			neighbor = NULL;
		}
	}
}
const LLVector3d &LLSurface::getOriginGlobal() const
{
	return mOriginGlobal;
}
LLVector3 LLSurface::getOriginAgent() const
{
	return gAgent.getPosAgentFromGlobal(mOriginGlobal);
}
F32 LLSurface::getMetersPerGrid() const
{
	return mMetersPerGrid;
}
S32 LLSurface::getGridsPerEdge() const
{
	return mGridsPerEdge;
}
S32 LLSurface::getPatchesPerEdge() const
{
	return mPatchesPerEdge;
}
S32 LLSurface::getGridsPerPatchEdge() const
{
	return mGridsPerPatchEdge;
}
void LLSurface::moveZ(const S32 x, const S32 y, const F32 delta)
{
	llassert(x >= 0);
	llassert(y >= 0);
	llassert(x < mGridsPerEdge);
	llassert(y < mGridsPerEdge);
	mSurfaceZ[x + y*mGridsPerEdge] += delta;
}
void LLSurface::updatePatchVisibilities(LLAgent &agent)
{
	if (gShiftFrame)
	{
		return;
	}
	LLVector3 pos_region = mRegionp->getPosRegionFromGlobal(gAgentCamera.getCameraPositionGlobal());
	mVisiblePatchCount = 0;
	for (auto& patchp : mPatchList)
	{
		patchp->updateVisibility();
		if (patchp->getVisible())
		{
			mVisiblePatchCount++;
			patchp->updateCameraDistanceRegion(pos_region);
		}
	}
}
BOOL LLSurface::idleUpdate(F32 max_update_time)
{
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_TERRAIN))
	{
		return FALSE;
	}
	LLTimer update_timer;
	BOOL did_update = FALSE;
	if (!mDirtyPatchList.empty())
	{
		getRegion()->dirtyHeights();
	}
	for (auto it = mDirtyPatchList.cbegin(); it != mDirtyPatchList.cend();)
	{
		if (it->second.expired())
		{
			LL_WARNS() << "Expired dirty patch detected. Side " << it->first << LL_ENDL;
		}
		surface_patch_ref patchp = it->second.lock();
		if (!patchp)
		{
			it = mDirtyPatchList.erase(it);
			continue;
		}
		if (patchp->updateNormals())
		{
			patchp->getSurface()->dirtySurfacePatch(patchp);
		}
		patchp->updateVerticalStats();
		if (max_update_time == 0.f || update_timer.getElapsedTimeF32() < max_update_time)
		{
			if (patchp->updateTexture())
			{
				did_update = TRUE;
				patchp->clearDirty();
				it = mDirtyPatchList.erase(it);
				continue;
			}
		}
		++it;
	}
	return did_update;
}
void LLSurface::decompressDCTPatch(LLBitPack &bitpack, LLGroupHeader *gopp, BOOL b_large_patch)
{
	LLPatchHeader  ph;
	S32 j, i;
	S32 patch[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];
	init_patch_decompressor(gopp->patch_size);
	gopp->stride = mGridsPerEdge;
	set_group_of_patch_header(gopp);
	while (1)
	{
		decode_patch_header(bitpack, &ph, b_large_patch);
		if (ph.quant_wbits == END_OF_PATCHES)
		{
			break;
		}
		if (b_large_patch)
		{
			i = ph.patchids >> 16;
			j = ph.patchids & 0xFFFF;
		}
		else
		{
			i = ph.patchids >> 5;
			j = ph.patchids & 0x1F;
		}
		if ((i >= mPatchesPerEdge) || (j >= mPatchesPerEdge))
		{
			LL_WARNS() << "Received invalid terrain packet - patch header patch ID incorrect!"
				<< " patches per edge " << mPatchesPerEdge
				<< " i " << i
				<< " j " << j
				<< " dc_offset " << ph.dc_offset
				<< " range " << (S32)ph.range
				<< " quant_wbits " << (S32)ph.quant_wbits
				<< " patchids " << (S32)ph.patchids
				<< LL_ENDL;
            LLAppViewer::instance()->badNetworkHandler();
			return;
		}
		const surface_patch_ref& patchp = mPatchList[j * mPatchesPerEdge + i];
		decode_patch(bitpack, patch);
		decompress_patch(patchp->getDataZ(), patch, &ph);
		patchp->updateNorthEdge();
		patchp->updateEastEdge();
		LLSurfacePatch* neighborPatch;
		if (neighborPatch = patchp->getNeighborPatch(WEST))
		{
			neighborPatch->updateEastEdge();
		}
		if (neighborPatch = patchp->getNeighborPatch(SOUTHWEST))
		{
			neighborPatch->updateEastEdge();
			neighborPatch->updateNorthEdge();
		}
		if (neighborPatch = patchp->getNeighborPatch(SOUTH))
		{
			neighborPatch->updateNorthEdge();
		}
		if (patchp->dirtyZ())
		{
			dirtySurfacePatch(patchp);
		}
		patchp->setHasReceivedData();
	}
}
BOOL LLSurface::containsPosition(const LLVector3 &position)
{
	if (position.mV[VX] < 0.0f  ||  position.mV[VX] > mMetersPerEdge ||
		position.mV[VY] < 0.0f  ||  position.mV[VY] > mMetersPerEdge)
	{
		return FALSE;
	}
	return TRUE;
}
F32 LLSurface::resolveHeightRegion(const F32 x, const F32 y) const
{
	F32 height = 0.0f;
	F32 oometerspergrid = 1.f/mMetersPerGrid;
	if (x >= 0.f  &&
		x <= mMetersPerEdge  &&
		y >= 0.f  &&
		y <= mMetersPerEdge)
	{
		const S32 left   = llfloor(x * oometerspergrid);
		const S32 bottom = llfloor(y * oometerspergrid);
		const S32 right  = ( left+1   < (S32)mGridsPerEdge-1 ? left+1   : left );
		const S32 top    = ( bottom+1 < (S32)mGridsPerEdge-1 ? bottom+1 : bottom );
		const F32 left_bottom  = getZ( left,  bottom );
		const F32 right_bottom = getZ( right, bottom );
		const F32 left_top     = getZ( left,  top );
		const F32 right_top    = getZ( right, top );
		F32 dx = x - left   * mMetersPerGrid;
		F32 dy = y - bottom * mMetersPerGrid;
		if (dy > dx)
		{
			dy *= left_top  - left_bottom;
			dx *= right_top - left_top;
		}
		else
		{
			dx *= right_bottom - left_bottom;
			dy *= right_top    - right_bottom;
		}
		height = left_bottom + (dx + dy) * oometerspergrid;
	}
	return height;
}
F32 LLSurface::resolveHeightGlobal(const LLVector3d& v) const
{
	if (!mRegionp)
	{
		return 0.f;
	}
	LLVector3 pos_region = mRegionp->getPosRegionFromGlobal(v);
	return resolveHeightRegion(pos_region);
}
LLVector3 LLSurface::resolveNormalGlobal(const LLVector3d& pos_global) const
{
	if (!mSurfaceZ)
	{
		return LLVector3::z_axis;
	}
	F32 oometerspergrid = 1.f/mMetersPerGrid;
	LLVector3 normal;
	F32 dzx, dzy;
	if (pos_global.mdV[VX] >= mOriginGlobal.mdV[VX]  &&
		pos_global.mdV[VX] < mOriginGlobal.mdV[VX] + mMetersPerEdge  &&
		pos_global.mdV[VY] >= mOriginGlobal.mdV[VY]  &&
		pos_global.mdV[VY] < mOriginGlobal.mdV[VY] + mMetersPerEdge)
	{
		U32 i, j, k;
		F32 dx, dy;
		i = (U32) ((pos_global.mdV[VX] - mOriginGlobal.mdV[VX]) * oometerspergrid);
		j = (U32) ((pos_global.mdV[VY] - mOriginGlobal.mdV[VY]) * oometerspergrid );
		k = i + j*mGridsPerEdge;
		dx = (F32)(pos_global.mdV[VX] - i*mMetersPerGrid - mOriginGlobal.mdV[VX]);
		dy = (F32)(pos_global.mdV[VY] - j*mMetersPerGrid - mOriginGlobal.mdV[VY]);
		if (dy > dx)
		{
			dzx = *(mSurfaceZ + k + 1 + mGridsPerEdge) - *(mSurfaceZ + k + mGridsPerEdge);
			dzy = *(mSurfaceZ + k) - *(mSurfaceZ + k + mGridsPerEdge);
			normal.setVec(-dzx,dzy,1);
		}
		else
		{
			dzx = *(mSurfaceZ + k) - *(mSurfaceZ + k + 1);
			dzy = *(mSurfaceZ + k + 1 + mGridsPerEdge) - *(mSurfaceZ + k + 1);
			normal.setVec(dzx,-dzy,1);
		}
	}
	normal.normVec();
	return normal;
}
const surface_patch_ref& LLSurface::resolvePatchRegion(const F32 x, const F32 y) const
{
	if (mPatchList.empty()) {
		LL_WARNS() << "No patches for current region!" << LL_ENDL;
		static surface_patch_ref empty;
		return empty;
	}
	S32 i, j;
	if (x < 0.0f)
	{
		i = 0;
	}
	else if (x >= mMetersPerEdge)
	{
		i = mPatchesPerEdge - 1;
	}
	else
	{
		i = (U32) (x / (mMetersPerGrid * mGridsPerPatchEdge));
	}
	if (y < 0.0f)
	{
		j = 0;
	}
	else if (y >= mMetersPerEdge)
	{
		j = mPatchesPerEdge - 1;
	}
	else
	{
		j = (U32) (y / (mMetersPerGrid * mGridsPerPatchEdge));
	}
	S32 index = i + j * mPatchesPerEdge;
	if((index < 0) || (index >= mPatchList.size()))
	{
		S32 old_index = index;
		index = llclamp(old_index, 0, ((S32)mPatchList.size() - 1));
		LL_WARNS() << "Clamping out of range patch index " << old_index
				<< " to " << index << LL_ENDL;
	}
	return mPatchList[index];
}
const surface_patch_ref& LLSurface::resolvePatchRegion(const LLVector3 &pos_region) const
{
	return resolvePatchRegion(pos_region.mV[VX], pos_region.mV[VY]);
}
const surface_patch_ref& LLSurface::resolvePatchGlobal(const LLVector3d &pos_global) const
{
	llassert(mRegionp);
	LLVector3 pos_region = mRegionp->getPosRegionFromGlobal(pos_global);
	return resolvePatchRegion(pos_region);
}
std::ostream& operator<<(std::ostream &s, const LLSurface &S)
{
	s << "{ \n";
	s << "  mGridsPerEdge = " << S.mGridsPerEdge - 1 << " + 1\n";
	s << "  mGridsPerPatchEdge = " << S.mGridsPerPatchEdge << "\n";
	s << "  mPatchesPerEdge = " << S.mPatchesPerEdge << "\n";
	s << "  mOriginGlobal = " << S.mOriginGlobal << "\n";
	s << "  mMetersPerGrid = " << S.mMetersPerGrid << "\n";
	s << "  mVisiblePatchCount = " << S.mVisiblePatchCount << "\n";
	s << "}";
	return s;
}
void LLSurface::createPatchData()
{
	S32 i, j;
	mPatchList.resize(mPatchesPerEdge * mPatchesPerEdge);
	for (S32 i = 0; i < mPatchList.size(); ++i)
	{
		mPatchList[i] = std::make_shared<LLSurfacePatch>(this, i);
	}
	mVisiblePatchCount = mPatchList.size();
	for (j=0; j<mPatchesPerEdge; j++)
	{
		for (i=0; i<mPatchesPerEdge; i++)
		{
			const auto& patchp = getPatch(i, j);
			S32 data_offset = i * mGridsPerPatchEdge + j * mGridsPerPatchEdge * mGridsPerEdge;
			patchp->setDataZ(mSurfaceZ + data_offset);
			patchp->setDataNorm(mNorm + data_offset);
			if (i < mPatchesPerEdge-1)
			{
				patchp->setNeighborPatch(EAST,getPatch(i+1, j));
			}
			else
			{
				patchp->setNeighborPatch(EAST, NULL);
			}
			if (j < mPatchesPerEdge-1)
			{
				patchp->setNeighborPatch(NORTH, getPatch(i, j+1));
			}
			else
			{
				patchp->setNeighborPatch(NORTH, NULL);
			}
			if (i > 0)
			{
				patchp->setNeighborPatch(WEST, getPatch(i - 1, j));
			}
			else
			{
				patchp->setNeighborPatch(WEST, NULL);
			}
			if (j > 0)
			{
				patchp->setNeighborPatch(SOUTH, getPatch(i, j-1));
			}
			else
			{
				patchp->setNeighborPatch(SOUTH, NULL);
			}
			if (i < (mPatchesPerEdge-1)  &&  j < (mPatchesPerEdge-1))
			{
				patchp->setNeighborPatch(NORTHEAST, getPatch(i + 1, j + 1));
			}
			else
			{
				patchp->setNeighborPatch(NORTHEAST, NULL);
			}
			if (i > 0  &&  j < (mPatchesPerEdge-1))
			{
				patchp->setNeighborPatch(NORTHWEST, getPatch(i - 1, j + 1));
			}
			else
			{
				patchp->setNeighborPatch(NORTHWEST, NULL);
			}
			if (i > 0  &&  j > 0)
			{
				patchp->setNeighborPatch(SOUTHWEST, getPatch(i - 1, j - 1));
			}
			else
			{
				patchp->setNeighborPatch(SOUTHWEST, NULL);
			}
			if (i < (mPatchesPerEdge-1)  &&  j > 0)
			{
				patchp->setNeighborPatch(SOUTHEAST, getPatch(i + 1, j - 1));
			}
			else
			{
				patchp->setNeighborPatch(SOUTHEAST, NULL);
			}
			LLVector3d origin_global;
			origin_global.mdV[0] = mOriginGlobal.mdV[0] + i * mMetersPerGrid * mGridsPerPatchEdge;
			origin_global.mdV[1] = mOriginGlobal.mdV[0] + j * mMetersPerGrid * mGridsPerPatchEdge;
			origin_global.mdV[2] = 0.f;
			patchp->setOriginGlobal(origin_global);
		}
	}
}
void LLSurface::destroyPatchData()
{
	mPatchList.clear();
	mVisiblePatchCount = 0;
}
void LLSurface::setTextureSize(const S32 texture_size)
{
	sTextureSize = texture_size;
}
U32 LLSurface::getRenderLevel(const U32 render_stride) const
{
	return mPVArray.mRenderLevelp[render_stride];
}
U32 LLSurface::getRenderStride(const U32 render_level) const
{
	return mPVArray.mRenderStridep[render_level];
}
const surface_patch_ref& LLSurface::getPatch(const S32 x, const S32 y) const
{
	static surface_patch_ref empty(nullptr);
	if ((x < 0) || (x >= mPatchesPerEdge))
	{
		LL_WARNS() << "Asking for patch out of bounds" << LL_ENDL;
		return empty;
	}
	if ((y < 0) || (y >= mPatchesPerEdge))
	{
		LL_WARNS() << "Asking for patch out of bounds" << LL_ENDL;
		return empty;
	}
	return mPatchList[x + y*mPatchesPerEdge];
}
void LLSurface::dirtyAllPatches()
{
	for (auto& patchp : mPatchList)
	{
		if (patchp->dirtyZ())
		{
			dirtySurfacePatch(patchp);
		}
	}
}
void LLSurface::dirtySurfacePatch(const surface_patch_ref& patchp)
{
	if (!patchp)
	{
		return;
	}
	if (std::find_if(mDirtyPatchList.begin(), mDirtyPatchList.end(),
		[&patchp](std::pair<U32, std::weak_ptr<LLSurfacePatch > >& entry) -> bool {
			return entry.second.lock().get() == patchp.get();
		}) == mDirtyPatchList.end())
	{
		mDirtyPatchList.push_back(std::make_pair(patchp->getSide(), patchp));
	}
}
void LLSurface::setWaterHeight(F32 height)
{
	if (!mWaterObjp.isNull())
	{
		LLVector3 water_pos_region = mWaterObjp->getPositionRegion();
		bool changed = water_pos_region.mV[VZ] != height;
		water_pos_region.mV[VZ] = height;
		mWaterObjp->setPositionRegion(water_pos_region);
		if (changed)
		{
			LLWorld::getInstance()->updateWaterObjects();
		}
	}
	else
	{
		LL_WARNS() << "LLSurface::setWaterHeight with no water object!" << LL_ENDL;
	}
}
F32 LLSurface::getWaterHeight() const
{
	if (!mWaterObjp.isNull())
	{
		return mWaterObjp->getPositionRegion().mV[VZ];
	}
	else
	{
		return DEFAULT_WATER_HEIGHT;
	}
}
BOOL LLSurface::generateWaterTexture(const F32 x, const F32 y,
									 const F32 width, const F32 height)
{
	if (!getWaterTexture())
	{
		return FALSE;
	}
	S32 tex_width = mWaterTexturep->getWidth();
	S32 tex_height = mWaterTexturep->getHeight();
	S32 tex_comps = mWaterTexturep->getComponents();
	S32 tex_stride = tex_width * tex_comps;
	LLPointer<LLImageRaw> raw = new LLImageRaw(tex_width, tex_height, tex_comps);
	U8 *rawp = raw->getData();
	F32 scale = getRegion()->getWidth() * getMetersPerGrid() / (F32)tex_width;
	F32 scale_inv = 1.f / scale;
	S32 x_begin, y_begin, x_end, y_end;
	x_begin = ll_round(x * scale_inv);
	y_begin = ll_round(y * scale_inv);
	x_end = ll_round((x + width) * scale_inv);
	y_end = ll_round((y + width) * scale_inv);
	if (x_end > tex_width)
	{
		x_end = tex_width;
	}
	if (y_end > tex_width)
	{
		y_end = tex_width;
	}
	LLVector3 location;
	LLColor4U coloru;
	const F32 WATER_HEIGHT = getWaterHeight();
	S32 i, j, offset;
	for (j = y_begin; j < y_end; j++)
	{
		for (i = x_begin; i < x_end; i++)
		{
			offset = j*tex_stride + i*tex_comps;
			location.mV[VX] = i*scale;
			location.mV[VY] = j*scale;
			const F32 height = resolveHeightRegion(location);
			if (height > WATER_HEIGHT)
			{
				coloru = MAX_WATER_COLOR;
				coloru.mV[3] = ABOVE_WATERLINE_ALPHA;
				*(rawp + offset++) = coloru.mV[0];
				*(rawp + offset++) = coloru.mV[1];
				*(rawp + offset++) = coloru.mV[2];
				*(rawp + offset++) = coloru.mV[3];
			}
			else
			{
				coloru = MAX_WATER_COLOR;
				const F32 frac = 1.f - 2.f/(2.f - (height - WATER_HEIGHT));
				S32 alpha = 64 + ll_round((255-64)*frac);
				alpha = llmin(ll_round((F32)MAX_WATER_COLOR.mV[3]), alpha);
				alpha = llmax(64, alpha);
				coloru.mV[3] = alpha;
				*(rawp + offset++) = coloru.mV[0];
				*(rawp + offset++) = coloru.mV[1];
				*(rawp + offset++) = coloru.mV[2];
				*(rawp + offset++) = coloru.mV[3];
			}
		}
	}
	if (!mWaterTexturep->hasGLTexture())
	{
		mWaterTexturep->createGLTexture(0, raw);
	}
	mWaterTexturep->setSubImage(raw, x_begin, y_begin, x_end - x_begin, y_end - y_begin);
	return TRUE;
}
