/** 
 * @file llviewerparceloverlay.h
 * @brief LLViewerParcelOverlay class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLVIEWERPARCELOVERLAY_H
#define LL_LLVIEWERPARCELOVERLAY_H
#include "llbbox.h"
#include "llframetimer.h"
#include "lluuid.h"
#include "llviewertexture.h"
#include "llgl.h"
class LLViewerRegion;
class LLVector3;
class LLColor4U;
class LLVector2;
class LLViewerParcelOverlay : public LLGLUpdate
{
public:
	LLViewerParcelOverlay(LLViewerRegion* region, F32 region_width_meters);
	~LLViewerParcelOverlay();
	LLViewerTexture*		getTexture() const		{ return mTexture; }
	BOOL			isOwned(const LLVector3& pos) const;
	BOOL			isOwnedSelf(const LLVector3& pos) const;
	BOOL			isOwnedGroup(const LLVector3& pos) const;
	BOOL			isOwnedOther(const LLVector3& pos) const;
	bool encroachesOwned(const std::vector<LLBBox>& boxes) const;
	bool encroachesOnUnowned(const std::vector<LLBBox>& boxes) const;
	bool encroachesOnNearbyParcel(const std::vector<LLBBox>& boxes) const;
	BOOL			isSoundLocal(const LLVector3& pos) const;
	BOOL			isBuildCameraAllowed(const LLVector3& pos) const;
	F32				getOwnedRatio() const;
	const U8*		getOwnership() const { return mOwnership; }
	S32				renderPropertyLines();
	U8				ownership( const LLVector3& pos) const;
	void	uncompressLandOverlay(S32 chunk, U8 *compressed_overlay);
	void	setDirty();
	void	idleUpdate(bool update_now = false);
	void	updateGL();
	typedef boost::signals2::signal<void (const LLViewerRegion*)> update_signal_t;
	static boost::signals2::connection setUpdateCallback(const update_signal_t::slot_type & cb);
private:
	U8		ownership(S32 row, S32 col) const
				{ return 0x7 & mOwnership[row * mParcelGridsPerEdge + col]; }
	void	addPropertyLine(std::vector<LLVector3>& vertex_array,
				std::vector<LLColor4U>& color_array,
				std::vector<LLVector2>& coord_array,
				const F32 start_x, const F32 start_y,
				const U32 edge,
				const LLColor4U& color);
	void 	updateOverlayTexture();
	void	updatePropertyLines();
private:
	LLViewerRegion*	mRegion;
	S32				mParcelGridsPerEdge;
	S32				mRegionSize;
	LLPointer<LLViewerTexture> mTexture;
	LLPointer<LLImageRaw> mImageRaw;
	U8				*mOwnership;
	BOOL			mDirty;
	LLFrameTimer	mTimeSinceLastUpdate;
	S32				mOverlayTextureIdx;
	S32				mVertexCount;
	F32*			mVertexArray;
	U8*				mColorArray;
	static update_signal_t* mUpdateSignal;
};
#endif
