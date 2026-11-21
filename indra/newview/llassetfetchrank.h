/**
 * @file llassetfetchrank.h
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
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
 * $/LicenseInfo$
 */
#ifndef LL_LLASSETFETCHRANK_H
#define LL_LLASSETFETCHRANK_H
#include "llatomic.h"
namespace AllynAssetRank
{
	const U32 NOTHING_SHOWN = 100000;
	const U32 MOST_PX = 50000;
	const U32 URGENT = NOTHING_SHOWN * 3;
	inline U32 clampPx(F32 px)
	{
		if (px < 1.f)
		{
			return 1;
		}
		if (px > (F32)MOST_PX)
		{
			return MOST_PX;
		}
		return (U32)px;
	}
	inline U32 meshRank(F32 px)
	{
		return clampPx(px * 2.f) + NOTHING_SHOWN;
	}
	inline U32 textureRank(F32 px, bool shown)
	{
		return clampPx(px) + (shown ? 0 : NOTHING_SHOWN);
	}
	inline LLAtomicU32& textureWaitRank()
	{
		static LLAtomicU32 rank(0);
		return rank;
	}
	inline LLAtomicU32& meshWaitRank()
	{
		static LLAtomicU32 rank(0);
		return rank;
	}
}
#endif
