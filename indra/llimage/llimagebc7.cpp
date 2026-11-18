/**
 * @file llimagebc7.cpp
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
#include "linden_common.h"
#include "llimagebc7.h"
#include "llimage.h"
#include "llerror.h"
#include "llfile.h"
#include "llstring.h"
#include "lluuid.h"
#include "lldiriterator.h"
#include "bcdec/bcdec.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <sys/utime.h>
extern "C"
{
	struct rgba_surface
	{
		uint8_t* ptr;
		int32_t width;
		int32_t height;
		int32_t stride;
	};
	struct bc7_enc_settings
	{
		bool mode_selection[4];
		int32_t refineIterations[8];
		bool skip_mode2;
		int32_t fastSkipTreshold_mode1;
		int32_t fastSkipTreshold_mode3;
		int32_t fastSkipTreshold_mode7;
		int32_t mode45_channel0;
		int32_t refineIterations_channel;
		int32_t channels;
	};
	void CompressBlocksBC7_ispc(rgba_surface* src, uint8_t* dst, bc7_enc_settings* settings);
}
static_assert(sizeof(rgba_surface) == 24, "rgba_surface layout");
static_assert(offsetof(rgba_surface, width) == 8, "rgba_surface layout");
static_assert(offsetof(rgba_surface, stride) == 16, "rgba_surface layout");
static_assert(sizeof(bc7_enc_settings) == 64, "bc7_enc_settings layout");
static_assert(offsetof(bc7_enc_settings, refineIterations) == 4, "bc7_enc_settings layout");
static_assert(offsetof(bc7_enc_settings, skip_mode2) == 36, "bc7_enc_settings layout");
static_assert(offsetof(bc7_enc_settings, fastSkipTreshold_mode1) == 40, "bc7_enc_settings layout");
static_assert(offsetof(bc7_enc_settings, channels) == 60, "bc7_enc_settings layout");
namespace
{
	const U32 TOLERANCE = 16;
	const size_t MAX_OFF_PER_MILLE = 10;
	const size_t CHECK_ONE_IN = 4;
	const size_t CHECK_ALL_UNDER = 1024;
	const size_t MIN_BLOCKS = 64;
	const size_t MAX_LEVELS = 16;
	const U64 REPORT_EVERY = 256;
	const U64 REPORT_AFTER_MS = 15000;
	const U32 WEIGHTS2[4] = { 0, 21, 43, 64 };
	const U32 WEIGHTS3[8] = { 0, 9, 18, 27, 37, 46, 55, 64 };
	std::atomic<bool> sEnabled(false);
	std::atomic<U64> sEncoded(0);
	std::atomic<U64> sRefused(0);
	std::atomic<U64> sTexels(0);
	std::atomic<U64> sMicroseconds(0);
	std::atomic<U64> sUploads(0);
	std::atomic<U64> sUploadBytes(0);
	std::atomic<U64> sCacheHits(0);
	std::atomic<U64> sCacheTexels(0);
	std::atomic<U64> sCacheMicroseconds(0);
	std::atomic<U64> sCacheWrites(0);
	std::atomic<U64> sCacheChains(0);
	std::atomic<U64> sCacheSkips(0);
	std::atomic<U64> sDecodes(0);
	std::atomic<U64> sDecodeTexels(0);
	std::atomic<U64> sDecodeMicroseconds(0);
	std::atomic<U64> sEvents(0);
	std::atomic<U64> sReportedEvents(0);
	std::atomic<U64> sReportedAt(0);
	U64 report_clock()
	{
		return (U64)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
	void report_event()
	{
		const U64 events = ++sEvents;
		const U64 now = report_clock();
		U64 last = sReportedAt;
		if (events % REPORT_EVERY != 0 && now - last < REPORT_AFTER_MS)
		{
			return;
		}
		if (sReportedAt.compare_exchange_strong(last, now))
		{
			LLImageBC7::report();
		}
	}
	struct Mip
	{
		size_t mWidth;
		size_t mHeight;
		std::vector<U8> mData;
	};
	bc7_enc_settings ultra_fast_settings(bool alpha_profile)
	{
		bc7_enc_settings settings;
		memset(&settings, 0, sizeof(settings));
		settings.skip_mode2 = true;
		settings.mode_selection[3] = true;
		if (alpha_profile)
		{
			static const int32_t refine[8] = { 2, 1, 2, 1, 1, 1, 2, 2 };
			memcpy(settings.refineIterations, refine, sizeof(refine));
			settings.channels = 4;
			settings.mode_selection[2] = true;
			settings.fastSkipTreshold_mode1 = 0;
			settings.fastSkipTreshold_mode3 = 0;
			settings.fastSkipTreshold_mode7 = 4;
			settings.mode45_channel0 = 3;
			settings.refineIterations_channel = 1;
		}
		else
		{
			static const int32_t refine[8] = { 2, 2, 2, 1, 2, 2, 1, 0 };
			memcpy(settings.refineIterations, refine, sizeof(refine));
			settings.channels = 3;
			settings.fastSkipTreshold_mode1 = 3;
			settings.fastSkipTreshold_mode3 = 1;
			settings.fastSkipTreshold_mode7 = 0;
			settings.mode45_channel0 = 0;
			settings.refineIterations_channel = 0;
		}
		return settings;
	}
	void compress_blocks(const bc7_enc_settings& settings, const U8* data, size_t width, size_t height, size_t stride, std::vector<U8>& blocks)
	{
		blocks.assign(((width * height + 15) / 16) * 16, 0);
		rgba_surface surface;
		surface.ptr = const_cast<U8*>(data);
		surface.width = (int32_t)width;
		surface.height = (int32_t)height;
		surface.stride = (int32_t)stride;
		bc7_enc_settings used = settings;
		CompressBlocksBC7_ispc(&surface, blocks.data(), &used);
	}
	void source_block(const U8* rgba, size_t w, size_t h, size_t bx, size_t by, U8* out)
	{
		for (size_t y = 0; y < 4; ++y)
		{
			const size_t sy = llmin(by * 4 + y, h - 1);
			for (size_t x = 0; x < 4; ++x)
			{
				const size_t sx = llmin(bx * 4 + x, w - 1);
				memcpy(out + (y * 4 + x) * 4, rgba + (sy * w + sx) * 4, 4);
			}
		}
	}
	bool alpha_end(U8 a)
	{
		return a == 0 || a == 255;
	}
	bool has_alpha_end(const U8* src)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			if (alpha_end(src[i * 4 + 3]))
			{
				return true;
			}
		}
		return false;
	}
	bool under_opaque(const U8* src)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			const U8 a = src[i * 4 + 3];
			if (a < 250 || a > 254)
			{
				return false;
			}
		}
		return true;
	}
	bool ends_kept(const U8* src, const U8* decoded)
	{
		const bool under = under_opaque(src);
		for (size_t i = 0; i < 16; ++i)
		{
			const U8 s = src[i * 4 + 3];
			const U8 d = decoded[i * 4 + 3];
			if (under)
			{
				if (d == 255)
				{
					return false;
				}
			}
			else if (alpha_end(s) && d != s)
			{
				return false;
			}
		}
		return true;
	}
	bool mend_mode6(U8* block, const U8* src)
	{
		if ((block[0] & 0x7f) != 0x40)
		{
			return false;
		}
		static const U8 low_bits[4][2] = { { 1, 1 }, { 1, 0 }, { 0, 1 }, { 0, 0 } };
		const U8 b7 = block[7];
		const U8 b8 = block[8];
		U8 decoded[64];
		for (size_t i = 0; i < 4; ++i)
		{
			block[7] = (U8)((b7 & 0x7f) | (low_bits[i][0] << 7));
			block[8] = (U8)((b8 & 0xfe) | low_bits[i][1]);
			bcdec_bc7(block, decoded, 16);
			if (ends_kept(src, decoded))
			{
				return true;
			}
		}
		block[7] = b7;
		block[8] = b8;
		return false;
	}
	U32 get_bits(const U64* bits, U32 at, U32 count)
	{
		U32 value = 0;
		for (U32 i = 0; i < count; ++i)
		{
			const U32 bit = at + i;
			value |= (U32)((bits[bit >> 6] >> (bit & 63)) & 1) << i;
		}
		return value;
	}
	void put_bits(U64* bits, U32 at, U32 count, U32 value)
	{
		for (U32 i = 0; i < count; ++i)
		{
			const U32 bit = at + i;
			const U64 mask = (U64)1 << (bit & 63);
			if ((value >> i) & 1)
			{
				bits[bit >> 6] |= mask;
			}
			else
			{
				bits[bit >> 6] &= ~mask;
			}
		}
	}
	U32 expand_end(U32 e, U32 end_bits)
	{
		return end_bits == 8 ? e : ((e << 2) | (e >> 4));
	}
	U32 abs_diff(U32 a, U32 b)
	{
		return a > b ? a - b : b - a;
	}
	U32 quantize_end(U32 value, U32 end_bits)
	{
		U32 best = 0;
		U32 best_diff = abs_diff(expand_end(0, end_bits), value);
		for (U32 e = 1; e < ((U32)1 << end_bits); ++e)
		{
			const U32 d = abs_diff(expand_end(e, end_bits), value);
			if (d < best_diff)
			{
				best = e;
				best_diff = d;
			}
		}
		return best;
	}
	U32 alpha_level(U32 e0, U32 e1, U32 weight, U32 end_bits)
	{
		return (expand_end(e0, end_bits) * (64 - weight) + expand_end(e1, end_bits) * weight + 32) >> 6;
	}
	bool snap_alpha(U8* block, const U8* src)
	{
		U64 bits[2];
		memcpy(bits, block, 16);
		U32 end_bits = 0;
		U32 ends_at = 0;
		U32 index_at = 0;
		U32 index_bits = 0;
		if ((block[0] & 0x3f) == 0x20 && get_bits(bits, 6, 2) == 0)
		{
			end_bits = 8;
			ends_at = 50;
			index_at = 97;
			index_bits = 2;
		}
		else if ((block[0] & 0x1f) == 0x10 && get_bits(bits, 5, 2) == 0)
		{
			end_bits = 6;
			ends_at = 38;
			if (get_bits(bits, 7, 1) == 0)
			{
				index_at = 81;
				index_bits = 3;
			}
			else
			{
				index_at = 50;
				index_bits = 2;
			}
		}
		else
		{
			return false;
		}
		const U32* weights = index_bits == 2 ? WEIGHTS2 : WEIGHTS3;
		const U32 steps = index_bits == 2 ? 4 : 8;
		U32 alphas[16];
		U32 lo = 255;
		U32 hi = 0;
		for (U32 i = 0; i < 16; ++i)
		{
			alphas[i] = src[i * 4 + 3];
			lo = llmin(lo, alphas[i]);
			hi = llmax(hi, alphas[i]);
		}
		U32 e0 = quantize_end(lo, end_bits);
		U32 e1 = quantize_end(hi, end_bits);
		U32 indices[16];
		for (U32 i = 0; i < 16; ++i)
		{
			U32 best = 0;
			U32 best_diff = abs_diff(alpha_level(e0, e1, weights[0], end_bits), alphas[i]);
			for (U32 step = 1; step < steps; ++step)
			{
				const U32 d = abs_diff(alpha_level(e0, e1, weights[step], end_bits), alphas[i]);
				if (d < best_diff)
				{
					best = step;
					best_diff = d;
				}
			}
			indices[i] = best;
		}
		const U32 top = steps - 1;
		if (indices[0] > top / 2)
		{
			const U32 swapped = e0;
			e0 = e1;
			e1 = swapped;
			for (U32 i = 0; i < 16; ++i)
			{
				indices[i] = top - indices[i];
			}
		}
		put_bits(bits, ends_at, end_bits, e0);
		put_bits(bits, ends_at + end_bits, end_bits, e1);
		for (U32 i = 0; i < 16; ++i)
		{
			if (i == 0)
			{
				put_bits(bits, index_at, index_bits - 1, indices[i]);
			}
			else
			{
				put_bits(bits, index_at + i * index_bits - 1, index_bits, indices[i]);
			}
		}
		memcpy(block, bits, 16);
		return true;
	}
	bool mend_alpha_ends(const U8* rgba, std::vector<U8>& blocks, size_t w, size_t h)
	{
		const size_t columns = (w + 3) / 4;
		const size_t rows = (h + 3) / 4;
		U8 src[64];
		U8 decoded[64];
		std::vector<size_t> again_at;
		std::vector<U8> again_src;
		for (size_t by = 0; by < rows; ++by)
		{
			for (size_t bx = 0; bx < columns; ++bx)
			{
				source_block(rgba, w, h, bx, by, src);
				if (!under_opaque(src) && !has_alpha_end(src))
				{
					continue;
				}
				const size_t at = (by * columns + bx) * 16;
				if (at + 16 > blocks.size())
				{
					return false;
				}
				U8* block = &blocks[at];
				bcdec_bc7(block, decoded, 16);
				if (!ends_kept(src, decoded) && !mend_mode6(block, src))
				{
					again_at.push_back(at);
					again_src.insert(again_src.end(), src, src + 64);
				}
			}
		}
		if (again_at.empty())
		{
			return true;
		}
		bc7_enc_settings apart = ultra_fast_settings(true);
		apart.mode_selection[3] = false;
		std::vector<U8> encoded;
		compress_blocks(apart, again_src.data(), 4, 4 * again_at.size(), 16, encoded);
		if (encoded.size() < again_at.size() * 16)
		{
			return false;
		}
		for (size_t i = 0; i < again_at.size(); ++i)
		{
			U8* block = &encoded[i * 16];
			const U8* texels = &again_src[i * 64];
			bcdec_bc7(block, decoded, 16);
			if (!ends_kept(texels, decoded))
			{
				if (!snap_alpha(block, texels))
				{
					return false;
				}
				bcdec_bc7(block, decoded, 16);
				if (!ends_kept(texels, decoded))
				{
					return false;
				}
			}
			memcpy(&blocks[again_at[i]], block, 16);
		}
		return true;
	}
	void seal_opaque(std::vector<U8>& blocks)
	{
		for (size_t at = 0; at + 16 <= blocks.size(); at += 16)
		{
			U8* block = &blocks[at];
			if ((block[0] & 0x7f) == 0x40)
			{
				block[6] |= 0xfe;
				block[7] = 0xff;
				block[8] |= 1;
			}
		}
	}
	void encode_level(const bc7_enc_settings& settings, const Mip& mip, std::vector<U8>& blocks)
	{
		const size_t w = mip.mWidth;
		const size_t h = mip.mHeight;
		const size_t pw = (w + 3) & ~(size_t)3;
		const size_t ph = (h + 3) & ~(size_t)3;
		if (pw == w && ph == h)
		{
			compress_blocks(settings, mip.mData.data(), w, h, w * 4, blocks);
			return;
		}
		std::vector<U8> padded(pw * ph * 4);
		for (size_t y = 0; y < ph; ++y)
		{
			const size_t sy = llmin(y, h - 1);
			for (size_t x = 0; x < pw; ++x)
			{
				const size_t sx = llmin(x, w - 1);
				memcpy(&padded[(y * pw + x) * 4], &mip.mData[(sy * w + sx) * 4], 4);
			}
		}
		compress_blocks(settings, padded.data(), pw, ph, pw * 4, blocks);
	}
	bool block_off(const std::vector<U8>& rgba, const U8* decoded, size_t w, size_t bx, size_t by, bool alpha)
	{
		for (size_t y = 0; y < 4; ++y)
		{
			const size_t row = ((by * 4 + y) * w + bx * 4) * 4;
			if (row + 16 > rgba.size())
			{
				return true;
			}
			const U8* s = &rgba[row];
			const U8* d = decoded + y * 16;
			for (size_t x = 0; x < 4; ++x, s += 4, d += 4)
			{
				const U32 shown = alpha ? s[3] : 255;
				for (size_t c = 0; c < 3; ++c)
				{
					if (abs_diff(s[c], d[c]) * shown > TOLERANCE * 255)
					{
						return true;
					}
				}
				if (alpha && abs_diff(s[3], d[3]) > TOLERANCE)
				{
					return true;
				}
			}
		}
		return false;
	}
	bool harmed(const Mip& mip, const std::vector<U8>& blocks, bool alpha)
	{
		const size_t bw = mip.mWidth / 4;
		const size_t bh = mip.mHeight / 4;
		if (bw * bh < MIN_BLOCKS)
		{
			return false;
		}
		const size_t columns = (mip.mWidth + 3) / 4;
		const bool all = bw * bh <= CHECK_ALL_UNDER;
		size_t checked = 0;
		size_t off = 0;
		U8 decoded[64];
		for (size_t by = 0; by < bh; ++by)
		{
			for (size_t bx = 0; bx < bw; ++bx)
			{
				if (!all && ((bx + 2 * by) % CHECK_ONE_IN) != 0)
				{
					continue;
				}
				const size_t at = (by * columns + bx) * 16;
				if (at + 16 > blocks.size())
				{
					return true;
				}
				bcdec_bc7(&blocks[at], decoded, 16);
				++checked;
				if (block_off(mip.mData, decoded, mip.mWidth, bx, by, alpha))
				{
					++off;
				}
			}
		}
		return off * 1000 > checked * MAX_OFF_PER_MILLE;
	}
	bool try_chain(const bc7_enc_settings& settings, const std::vector<Mip>& mips, bool alpha, LLImageBC7Chain& chain)
	{
		chain.mLevels.clear();
		chain.mLevels.reserve(mips.size());
		for (size_t i = 0; i < mips.size(); ++i)
		{
			const Mip& mip = mips[i];
			LLImageBC7Chain::Level level;
			level.mWidth = (S32)mip.mWidth;
			level.mHeight = (S32)mip.mHeight;
			encode_level(settings, mip, level.mBlocks);
			if (alpha && !mend_alpha_ends(mip.mData.data(), level.mBlocks, mip.mWidth, mip.mHeight))
			{
				return false;
			}
			if (!alpha)
			{
				seal_opaque(level.mBlocks);
			}
			if (harmed(mip, level.mBlocks, alpha))
			{
				return false;
			}
			chain.mLevels.push_back(std::move(level));
		}
		return true;
	}
	void build_mips(std::vector<Mip>& mips)
	{
		while ((mips.back().mWidth > 1 || mips.back().mHeight > 1) && mips.size() < MAX_LEVELS)
		{
			const size_t w = mips.back().mWidth;
			const size_t h = mips.back().mHeight;
			Mip next;
			next.mWidth = llmax(w / 2, (size_t)1);
			next.mHeight = llmax(h / 2, (size_t)1);
			next.mData.resize(next.mWidth * next.mHeight * 4);
			const U8* prev = mips.back().mData.data();
			U8* out = next.mData.data();
			for (size_t y = 0; y < next.mHeight; ++y)
			{
				const U8* row0 = prev + llmin(y * 2, h - 1) * w * 4;
				const U8* row1 = prev + llmin(y * 2 + 1, h - 1) * w * 4;
				for (size_t x = 0; x < next.mWidth; ++x)
				{
					const size_t x0 = llmin(x * 2, w - 1) * 4;
					const size_t x1 = llmin(x * 2 + 1, w - 1) * 4;
					for (size_t c = 0; c < 4; ++c)
					{
						*out++ = (U8)(((U32)row0[x0 + c] + (U32)row0[x1 + c] + (U32)row1[x0 + c] + (U32)row1[x1 + c] + 2) / 4);
					}
				}
			}
			mips.push_back(std::move(next));
		}
	}
}
void LLImageBC7::setEnabled(bool enabled)
{
	sEnabled = enabled;
}
bool LLImageBC7::isEnabled()
{
	return sEnabled;
}
S32 LLImageBC7::levelBytes(S32 width, S32 height)
{
	return ((width + 3) / 4) * ((height + 3) / 4) * 16;
}
LLImageBC7ChainPtr LLImageBC7::encode(const U8* data, S32 width, S32 height, S32 components)
{
	if (!data || width <= 0 || height <= 0 || (width % 4) != 0 || (height % 4) != 0 || (components != 3 && components != 4))
	{
		return LLImageBC7ChainPtr();
	}
	const size_t texels = (size_t)width * (size_t)height;
	std::vector<Mip> mips(1);
	mips[0].mWidth = (size_t)width;
	mips[0].mHeight = (size_t)height;
	mips[0].mData.resize(texels * 4);
	U8* rgba = mips[0].mData.data();
	bool alpha = false;
	if (components == 4)
	{
		memcpy(rgba, data, texels * 4);
		for (size_t i = 0; i < texels; ++i)
		{
			if (data[i * 4 + 3] != 255)
			{
				alpha = true;
				break;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < texels; ++i)
		{
			rgba[i * 4] = data[i * 3];
			rgba[i * 4 + 1] = data[i * 3 + 1];
			rgba[i * 4 + 2] = data[i * 3 + 2];
			rgba[i * 4 + 3] = 255;
		}
	}
	build_mips(mips);
	std::shared_ptr<LLImageBC7Chain> chain = std::make_shared<LLImageBC7Chain>();
	bc7_enc_settings single_line = ultra_fast_settings(false);
	single_line.channels = alpha ? 4 : 3;
	if (try_chain(single_line, mips, alpha, *chain))
	{
		return chain;
	}
	if (alpha && try_chain(ultra_fast_settings(true), mips, alpha, *chain))
	{
		return chain;
	}
	return LLImageBC7ChainPtr();
}
bool LLImageBC7::fits(S32 width, S32 height, S32 components)
{
	return width > 0 && height > 0 && (width % 4) == 0 && (height % 4) == 0 && (components == 3 || components == 4);
}
void LLImageBC7::decodeLevel(const U8* blocks, S32 width, S32 height, S32 components, U8* out)
{
	const S32 columns = (width + 3) / 4;
	const S32 rows = (height + 3) / 4;
	U8 texels[64];
	for (S32 by = 0; by < rows; ++by)
	{
		for (S32 bx = 0; bx < columns; ++bx)
		{
			const U8* block = blocks + ((size_t)by * columns + bx) * 16;
			const S32 w = llmin(4, width - bx * 4);
			const S32 h = llmin(4, height - by * 4);
			if (components == 4 && w == 4 && h == 4)
			{
				bcdec_bc7(block, out + ((size_t)by * 4 * width + bx * 4) * 4, width * 4);
				continue;
			}
			bcdec_bc7(block, texels, 16);
			for (S32 y = 0; y < h; ++y)
			{
				U8* dst = out + ((size_t)(by * 4 + y) * width + bx * 4) * components;
				const U8* src = texels + y * 16;
				for (S32 x = 0; x < w; ++x, dst += components, src += 4)
				{
					memcpy(dst, src, components);
				}
			}
		}
	}
}
bool LLImageBC7::attach(LLImageRaw* raw)
{
	if (!raw || !isEnabled())
	{
		return false;
	}
	const S32 width = raw->getWidth();
	const S32 height = raw->getHeight();
	const S32 components = raw->getComponents();
	if (!fits(width, height, components))
	{
		return false;
	}
	if (raw->getDataSize() < width * height * components)
	{
		return false;
	}
	const U8* data = raw->getData();
	if (!data)
	{
		return false;
	}
	const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	LLImageBC7ChainPtr chain = encode(data, width, height, components);
	const U64 elapsed = (U64)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
	raw->setBC7Chain(chain);
	if (chain)
	{
		++sEncoded;
	}
	else
	{
		++sRefused;
	}
	sTexels += (U64)width * (U64)height;
	sMicroseconds += elapsed;
	report_event();
	return chain != nullptr;
}
void LLImageBC7::inherit(const LLImageBC7ChainPtr& chain, LLImageRaw* scaled, S32 levels)
{
	if (!chain || !scaled || levels <= 0 || levels >= chain->getLevels())
	{
		return;
	}
	const S32 width = chain->getWidth(levels);
	const S32 height = chain->getHeight(levels);
	if (width != scaled->getWidth() || height != scaled->getHeight() || (width % 4) != 0 || (height % 4) != 0)
	{
		return;
	}
	std::shared_ptr<LLImageBC7Chain> reduced = std::make_shared<LLImageBC7Chain>();
	reduced->mLevels.assign(chain->mLevels.begin() + levels, chain->mLevels.end());
	scaled->setBC7Chain(reduced);
}
void LLImageBC7::countUpload(S32 bytes)
{
	++sUploads;
	sUploadBytes += (U64)bytes;
}
void LLImageBC7::countCacheHit(S32 texels, U64 microseconds)
{
	++sCacheHits;
	sCacheTexels += (U64)texels;
	sCacheMicroseconds += microseconds;
	report_event();
}
void LLImageBC7::countCacheWrite()
{
	++sCacheWrites;
}
void LLImageBC7::countCacheChain()
{
	++sCacheChains;
	report_event();
}
void LLImageBC7::countCacheSkip()
{
	++sCacheSkips;
	report_event();
}
void LLImageBC7::countDecode(S32 texels, U64 microseconds)
{
	++sDecodes;
	sDecodeTexels += (U64)texels;
	sDecodeMicroseconds += microseconds;
	report_event();
}
void LLImageBC7::report()
{
	const U64 events = sEvents;
	if (sReportedEvents.exchange(events) == events)
	{
		return;
	}
	const Stats stats = getStats();
	LL_INFOS("TextureBC7") << "BC7: JPEG2000 " << stats.mDecodes << " decodes, " << stats.mDecodeTexels << " texels in "
		<< stats.mDecodeMicroseconds / 1000 << " ms; encode " << stats.mEncoded << " stored, " << stats.mRefused << " kept RGBA, "
		<< stats.mTexels << " texels in " << stats.mMicroseconds / 1000 << " ms; cache " << stats.mCacheHits << " read without decode, "
		<< stats.mCacheTexels << " texels in " << stats.mCacheMicroseconds / 1000 << " ms, " << stats.mCacheChains << " blocks reused, "
		<< stats.mCacheSkips << " encodes skipped, " << stats.mCacheWrites << " written; " << stats.mUploads << " uploads, "
		<< stats.mUploadBytes << " bytes" << LL_ENDL;
}
LLImageBC7::Stats LLImageBC7::getStats()
{
	Stats stats;
	stats.mEncoded = sEncoded;
	stats.mRefused = sRefused;
	stats.mTexels = sTexels;
	stats.mMicroseconds = sMicroseconds;
	stats.mUploads = sUploads;
	stats.mUploadBytes = sUploadBytes;
	stats.mCacheHits = sCacheHits;
	stats.mCacheTexels = sCacheTexels;
	stats.mCacheMicroseconds = sCacheMicroseconds;
	stats.mCacheWrites = sCacheWrites;
	stats.mCacheChains = sCacheChains;
	stats.mCacheSkips = sCacheSkips;
	stats.mDecodes = sDecodes;
	stats.mDecodeTexels = sDecodeTexels;
	stats.mDecodeMicroseconds = sDecodeMicroseconds;
	return stats;
}
namespace
{
	const U32 CACHE_MAGIC = 0x37434241;
	const U32 CACHE_VERSION = 1;
	const U32 CACHE_KIND_BC7 = 1;
	const U32 CACHE_KIND_KEEP_RGBA = 2;
	const size_t CACHE_HEAD_WORDS = 9;
	const U32 CACHE_MAX_DIMENSION = 8192;
	const S32 CACHE_MIN_LEVEL = 256;
	const S64 CACHE_TOUCH_SECONDS = 86400;
	const char CACHE_SUBDIRS[] = "0123456789abcdef";
	const char CACHE_DELIM[] = "\\";
	std::atomic<bool> sCacheOn(false);
	std::atomic<bool> sCacheReadOnly(true);
	std::atomic<S64> sCacheMaxBytes(0);
	std::string sCacheDir;
	std::mutex sCacheMutex;
	S64 sCacheBytes = -1;
	struct CacheFile
	{
		std::string mPath;
		S64 mBytes;
		S64 mTime;
	};
	std::string cache_subdir(char first)
	{
		return sCacheDir + CACHE_DELIM + first;
	}
	std::string cache_path(const LLUUID& id)
	{
		const std::string name = id.asString();
		return cache_subdir(name[0]) + CACHE_DELIM + name + ".bc7";
	}
	S64 cache_file_bytes(const std::string& path)
	{
		llstat status;
		return LLFile::stat(path, &status) == 0 ? (S64)status.st_size : 0;
	}
	S64 cache_scan(std::vector<CacheFile>* files)
	{
		S64 total = 0;
		for (S32 i = 0; i < 16; ++i)
		{
			const std::string subdir = cache_subdir(CACHE_SUBDIRS[i]);
			if (!LLFile::isdir(subdir))
			{
				continue;
			}
			LLDirIterator iter(subdir, "*.bc7");
			std::string name;
			while (iter.next(name))
			{
				CacheFile entry;
				entry.mPath = subdir + CACHE_DELIM + name;
				llstat status;
				if (LLFile::stat(entry.mPath, &status) != 0)
				{
					continue;
				}
				entry.mBytes = (S64)status.st_size;
				entry.mTime = (S64)status.st_mtime;
				total += entry.mBytes;
				if (files)
				{
					files->push_back(entry);
				}
			}
		}
		return total;
	}
	void cache_evict()
	{
		std::vector<CacheFile> files;
		S64 total = cache_scan(&files);
		const S64 target = sCacheMaxBytes / 10 * 9;
		if (total > target)
		{
			std::sort(files.begin(), files.end(), [](const CacheFile& a, const CacheFile& b) { return a.mTime < b.mTime; });
			for (const CacheFile& entry : files)
			{
				if (total <= target)
				{
					break;
				}
				if (LLFile::remove_nowarn(entry.mPath) == 0)
				{
					total -= entry.mBytes;
				}
			}
		}
		sCacheBytes = total;
	}
	void cache_account(S64 delta)
	{
		std::lock_guard<std::mutex> lock(sCacheMutex);
		if (sCacheBytes < 0)
		{
			sCacheBytes = cache_scan(NULL);
		}
		else
		{
			sCacheBytes = llmax((S64)0, sCacheBytes + delta);
		}
		if (sCacheBytes > sCacheMaxBytes)
		{
			const S64 before = sCacheBytes;
			cache_evict();
			LL_INFOS("TextureBC7") << "BC7 cache trimmed from " << before << " to " << sCacheBytes << " bytes" << LL_ENDL;
		}
	}
	bool cache_store(const LLUUID& id, const U32* head, const LLImageBC7Chain* chain)
	{
		const std::string name = id.asString();
		const std::string path = cache_path(id);
		const std::string temp = path + ".tmp";
		LLFILE* file = LLFile::fopen(temp, "wb");
		if (!file)
		{
			LLFile::mkdir_nowarn(sCacheDir, 0700);
			LLFile::mkdir_nowarn(cache_subdir(name[0]), 0700);
			file = LLFile::fopen(temp, "wb");
			if (!file)
			{
				return false;
			}
		}
		bool ok = fwrite(head, sizeof(U32), CACHE_HEAD_WORDS, file) == CACHE_HEAD_WORDS;
		if (chain)
		{
			for (S32 level = 0; ok && level < chain->getLevels(); ++level)
			{
				const U32 dims[2] = { (U32)chain->getWidth(level), (U32)chain->getHeight(level) };
				ok = fwrite(dims, sizeof(U32), 2, file) == 2;
			}
			for (S32 level = 0; ok && level < chain->getLevels(); ++level)
			{
				const size_t bytes = (size_t)chain->getSize(level);
				ok = fwrite(chain->getData(level), 1, bytes, file) == bytes;
			}
		}
		ok = (fclose(file) == 0) && ok;
		const S64 old_bytes = cache_file_bytes(path);
		if (ok)
		{
			LLFile::remove_nowarn(path);
			ok = LLFile::rename_nowarn(temp, path) == 0;
		}
		if (!ok)
		{
			LLFile::remove_nowarn(temp);
		}
		cache_account(cache_file_bytes(path) - old_bytes);
		return ok;
	}
	LLImageBC7Cache::EState cache_read_levels(LLFILE* file, U32 levels, S32 want_width, S32 want_height, S32 components, LLImageBC7ChainPtr& out)
	{
		std::vector<U32> dims(levels * 2);
		if (fread(dims.data(), sizeof(U32), dims.size(), file) != dims.size())
		{
			return LLImageBC7Cache::STATE_MISSING;
		}
		for (U32 value : dims)
		{
			if (value == 0 || value > CACHE_MAX_DIMENSION)
			{
				return LLImageBC7Cache::STATE_MISSING;
			}
		}
		if ((U32)want_width > dims[0] || (U32)want_height > dims[1])
		{
			return LLImageBC7Cache::STATE_COARSER;
		}
		long skip = 0;
		U32 first = levels;
		for (U32 level = 0; level < levels; ++level)
		{
			if (dims[level * 2] == (U32)want_width && dims[level * 2 + 1] == (U32)want_height)
			{
				first = level;
				break;
			}
			skip += LLImageBC7::levelBytes((S32)dims[level * 2], (S32)dims[level * 2 + 1]);
		}
		if (first == levels || !LLImageBC7::fits(want_width, want_height, components))
		{
			return LLImageBC7Cache::STATE_STORED;
		}
		if (skip > 0 && fseek(file, skip, SEEK_CUR) != 0)
		{
			return LLImageBC7Cache::STATE_MISSING;
		}
		std::shared_ptr<LLImageBC7Chain> chain = std::make_shared<LLImageBC7Chain>();
		chain->mLevels.resize(levels - first);
		for (U32 level = first; level < levels; ++level)
		{
			LLImageBC7Chain::Level& entry = chain->mLevels[level - first];
			entry.mWidth = (S32)dims[level * 2];
			entry.mHeight = (S32)dims[level * 2 + 1];
			entry.mBlocks.resize((size_t)LLImageBC7::levelBytes(entry.mWidth, entry.mHeight));
			if (fread(entry.mBlocks.data(), 1, entry.mBlocks.size(), file) != entry.mBlocks.size())
			{
				return LLImageBC7Cache::STATE_MISSING;
			}
		}
		out = chain;
		return LLImageBC7Cache::STATE_HIT;
	}
}
void LLImageBC7Cache::init(const std::string& dir, S64 max_bytes, bool read_only)
{
	sCacheMaxBytes = max_bytes;
	sCacheReadOnly = read_only;
	if (sCacheOn || dir.empty() || max_bytes <= 0)
	{
		return;
	}
	sCacheDir = dir;
	sCacheOn = true;
	LL_INFOS("TextureBC7") << "BC7 cache: " << dir << ", limit " << max_bytes / (1024 * 1024) << " MB"
		<< (read_only ? ", read only" : "") << LL_ENDL;
}
bool LLImageBC7Cache::isEnabled()
{
	return sCacheOn;
}
LLImageBC7Cache::EState LLImageBC7Cache::read(const LLUUID& id, S32 width, S32 height, S32 components, S32 discard, LLImageBC7ChainPtr& chain)
{
	chain.reset();
	if (!sCacheOn || id.isNull() || width <= 0 || height <= 0 || discard < 0 || discard >= (S32)MAX_LEVELS)
	{
		return STATE_OFF;
	}
	const S32 want_width = llmax(width >> discard, 1);
	const S32 want_height = llmax(height >> discard, 1);
	const std::string path = cache_path(id);
	LLFILE* file = LLFile::fopen(path, "rb");
	if (!file)
	{
		return STATE_MISSING;
	}
	EState state = STATE_MISSING;
	U32 head[CACHE_HEAD_WORDS];
	if (fread(head, sizeof(U32), CACHE_HEAD_WORDS, file) == CACHE_HEAD_WORDS
		&& head[0] == CACHE_MAGIC && head[1] == CACHE_VERSION
		&& head[3] == (U32)width && head[4] == (U32)height && head[5] == (U32)components)
	{
		if (head[2] == CACHE_KIND_KEEP_RGBA)
		{
			state = ((U32)want_width <= head[6] && (U32)want_height <= head[7]) ? STATE_KEEP_RGBA : STATE_COARSER;
		}
		else if (head[2] == CACHE_KIND_BC7 && head[8] > 0 && head[8] <= (U32)MAX_LEVELS)
		{
			const U32 top = llmax(head[6], head[7]);
			if (top < (U32)CACHE_MIN_LEVEL && (head[6] < (U32)width || head[7] < (U32)height))
			{
				state = STATE_MISSING;
			}
			else
			{
				state = cache_read_levels(file, head[8], want_width, want_height, components, chain);
			}
		}
	}
	fclose(file);
	if (state == STATE_HIT && !sCacheReadOnly)
	{
		llstat status;
		if (LLFile::stat(path, &status) == 0 && (S64)time(NULL) - (S64)status.st_mtime > CACHE_TOUCH_SECONDS)
		{
			_wutime(utf8str_to_utf16str(path).c_str(), NULL);
		}
	}
	return state;
}
void LLImageBC7Cache::write(const LLUUID& id, S32 width, S32 height, S32 components, const LLImageBC7ChainPtr& chain)
{
	if (!sCacheOn || sCacheReadOnly || id.isNull() || !chain || chain->getLevels() <= 0 || chain->getLevels() > (S32)MAX_LEVELS)
	{
		return;
	}
	if (llmax(chain->getWidth(0), chain->getHeight(0)) < CACHE_MIN_LEVEL)
	{
		return;
	}
	const U32 head[CACHE_HEAD_WORDS] = { CACHE_MAGIC, CACHE_VERSION, CACHE_KIND_BC7, (U32)width, (U32)height, (U32)components,
		(U32)chain->getWidth(0), (U32)chain->getHeight(0), (U32)chain->getLevels() };
	if (cache_store(id, head, chain.get()))
	{
		LLImageBC7::countCacheWrite();
	}
}
void LLImageBC7Cache::writeKeepRGBA(const LLUUID& id, S32 width, S32 height, S32 components, S32 level_width, S32 level_height)
{
	if (!sCacheOn || sCacheReadOnly || id.isNull())
	{
		return;
	}
	const U32 head[CACHE_HEAD_WORDS] = { CACHE_MAGIC, CACHE_VERSION, CACHE_KIND_KEEP_RGBA, (U32)width, (U32)height, (U32)components,
		(U32)level_width, (U32)level_height, 0 };
	cache_store(id, head, NULL);
}
void LLImageBC7Cache::remove(const LLUUID& id)
{
	if (!sCacheOn || sCacheReadOnly || id.isNull())
	{
		return;
	}
	const std::string path = cache_path(id);
	const S64 bytes = cache_file_bytes(path);
	if (bytes > 0 && LLFile::remove_nowarn(path) == 0)
	{
		std::lock_guard<std::mutex> lock(sCacheMutex);
		if (sCacheBytes >= 0)
		{
			sCacheBytes = llmax((S64)0, sCacheBytes - bytes);
		}
	}
}
void LLImageBC7Cache::clear()
{
	if (!sCacheOn || sCacheReadOnly)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(sCacheMutex);
	std::vector<CacheFile> files;
	cache_scan(&files);
	for (const CacheFile& entry : files)
	{
		LLFile::remove_nowarn(entry.mPath);
	}
	sCacheBytes = -1;
}
