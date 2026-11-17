/**
 * @file llimagebc7.h
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
#ifndef LL_LLIMAGEBC7_H
#define LL_LLIMAGEBC7_H
#include "stdtypes.h"
#include <memory>
#include <string>
#include <vector>
class LLImageRaw;
class LLUUID;
class LLImageBC7Chain
{
public:
	struct Level
	{
		S32 mWidth;
		S32 mHeight;
		std::vector<U8> mBlocks;
	};
	S32 getLevels() const { return (S32)mLevels.size(); }
	S32 getWidth(S32 level) const { return mLevels[level].mWidth; }
	S32 getHeight(S32 level) const { return mLevels[level].mHeight; }
	S32 getSize(S32 level) const { return (S32)mLevels[level].mBlocks.size(); }
	const U8* getData(S32 level) const { return mLevels[level].mBlocks.data(); }
	std::vector<Level> mLevels;
};
typedef std::shared_ptr<const LLImageBC7Chain> LLImageBC7ChainPtr;
class LLImageBC7
{
public:
	struct Stats
	{
		U64 mEncoded;
		U64 mRefused;
		U64 mTexels;
		U64 mMicroseconds;
		U64 mUploads;
		U64 mUploadBytes;
		U64 mCacheHits;
		U64 mCacheTexels;
		U64 mCacheMicroseconds;
		U64 mCacheWrites;
		U64 mCacheChains;
		U64 mCacheSkips;
		U64 mDecodes;
		U64 mDecodeTexels;
		U64 mDecodeMicroseconds;
	};
	static void setEnabled(bool enabled);
	static bool isEnabled();
	static S32 levelBytes(S32 width, S32 height);
	static bool fits(S32 width, S32 height, S32 components);
	static LLImageBC7ChainPtr encode(const U8* data, S32 width, S32 height, S32 components);
	static bool attach(LLImageRaw* raw);
	static void inherit(const LLImageBC7ChainPtr& chain, LLImageRaw* scaled, S32 levels);
	static void decodeLevel(const U8* blocks, S32 width, S32 height, S32 components, U8* out);
	static void countUpload(S32 bytes);
	static void countCacheHit(S32 texels, U64 microseconds);
	static void countCacheWrite();
	static void countCacheChain();
	static void countCacheSkip();
	static void countDecode(S32 texels, U64 microseconds);
	static void report();
	static Stats getStats();
};
class LLImageBC7Cache
{
public:
	enum EState
	{
		STATE_OFF,
		STATE_MISSING,
		STATE_COARSER,
		STATE_STORED,
		STATE_KEEP_RGBA,
		STATE_HIT
	};
	static void init(const std::string& dir, S64 max_bytes, bool read_only);
	static bool isEnabled();
	static EState read(const LLUUID& id, S32 width, S32 height, S32 components, S32 discard, LLImageBC7ChainPtr& chain);
	static void write(const LLUUID& id, S32 width, S32 height, S32 components, const LLImageBC7ChainPtr& chain);
	static void writeKeepRGBA(const LLUUID& id, S32 width, S32 height, S32 components, S32 level_width, S32 level_height);
	static void remove(const LLUUID& id);
	static void clear();
};
#endif
