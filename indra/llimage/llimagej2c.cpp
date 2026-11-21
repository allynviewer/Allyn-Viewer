/** 
 * @file llimagej2c.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Modifications Copyright (C) 2026 - Allyn Viewer
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
#include "linden_common.h"
#include <cmath>
#include "apr_pools.h"
#include "apr_dso.h"
#include "lldir.h"
#include "../llxml/llcontrol.h"
#include "llimagej2c.h"
typedef LLImageJ2CImpl* (*CreateLLImageJ2CFunction)();
typedef void (*DestroyLLImageJ2CFunction)(LLImageJ2CImpl*);
typedef const char* (*EngineInfoLLImageJ2CFunction)();
CreateLLImageJ2CFunction j2cimpl_create_func;
DestroyLLImageJ2CFunction j2cimpl_destroy_func;
EngineInfoLLImageJ2CFunction j2cimpl_engineinfo_func;
LLAPRPool j2cimpl_dso_memory_pool;
apr_dso_handle_t *j2cimpl_dso_handle;
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl();
void fallbackDestroyLLImageJ2CImpl(LLImageJ2CImpl* impl);
const char* fallbackEngineInfoLLImageJ2CImpl();
void LLImageJ2C::openDSO()
{
	std::string dso_name;
	std::string dso_path;
	bool all_functions_loaded = false;
	apr_status_t rv;
#if LL_WINDOWS
	dso_name = "";
#elif LL_DARWIN
	dso_name = "";
#else
	dso_name = "";
#endif
	dso_path = gDirUtilp->findFile(dso_name,
				       gDirUtilp->getAppRODataDir(),
				       gDirUtilp->getExecutableDir());
	j2cimpl_dso_handle      = NULL;
	j2cimpl_dso_memory_pool.create();
	rv = apr_dso_load(&j2cimpl_dso_handle,
					  dso_path.c_str(),
					  j2cimpl_dso_memory_pool());
	if ( rv == APR_SUCCESS )
	{
		CreateLLImageJ2CFunction  create_func = NULL;
		DestroyLLImageJ2CFunction dest_func = NULL;
		EngineInfoLLImageJ2CFunction engineinfo_func = NULL;
		rv = apr_dso_sym((apr_dso_handle_sym_t*)&create_func,
						 j2cimpl_dso_handle,
						 "createLLImageJ2CKDU");
		if ( rv == APR_SUCCESS )
		{
			rv = apr_dso_sym((apr_dso_handle_sym_t*)&dest_func,
							 j2cimpl_dso_handle,
						       "destroyLLImageJ2CKDU");
			if ( rv == APR_SUCCESS )
			{
				rv = apr_dso_sym((apr_dso_handle_sym_t*)&engineinfo_func,
						 j2cimpl_dso_handle,
						 "engineInfoLLImageJ2CKDU");
				if ( rv == APR_SUCCESS )
				{
					j2cimpl_create_func  = create_func;
					j2cimpl_destroy_func = dest_func;
					j2cimpl_engineinfo_func = engineinfo_func;
					all_functions_loaded = true;
				}
			}
		}
	}
	if ( !all_functions_loaded )
	{
#if 0
		char errbuf[256];
		fprintf(stderr, "failed to load syms from DSO %s (%s)\n",
			dso_name.c_str(), dso_path.c_str());
		apr_strerror(rv, errbuf, sizeof(errbuf));
		fprintf(stderr, "error: %d, %s\n", rv, errbuf);
		apr_dso_error(j2cimpl_dso_handle, errbuf, sizeof(errbuf));
		fprintf(stderr, "dso-error: %d, %s\n", rv, errbuf);
		fflush(stderr);
#endif
		if ( j2cimpl_dso_handle )
		{
			apr_dso_unload(j2cimpl_dso_handle);
			j2cimpl_dso_handle = NULL;
		}
		j2cimpl_dso_memory_pool.destroy();
	}
}
void LLImageJ2C::closeDSO()
{
	if ( j2cimpl_dso_handle ) apr_dso_unload(j2cimpl_dso_handle);
	j2cimpl_dso_memory_pool.destroy();
}
std::string LLImageJ2C::getEngineInfo()
{
	if (!j2cimpl_engineinfo_func)
		j2cimpl_engineinfo_func = fallbackEngineInfoLLImageJ2CImpl;
	return j2cimpl_engineinfo_func();
}
LLImageJ2C::LLImageJ2C() : 	LLImageFormatted(IMG_CODEC_J2C),
							mMaxBytes(0),
							mRawDiscardLevel(-1),
							mRate(0.0f),
							mReversible(FALSE),
							mAreaUsedForDataSizeCalcs(0)
{
	if ( !j2cimpl_create_func )
	{
		j2cimpl_create_func = fallbackCreateLLImageJ2CImpl;
	}
	mImpl = j2cimpl_create_func();
	for( S32 i = 0; i <= MAX_DISCARD_LEVEL; i++)
	{
		mDataSizes[i] = 0;
	}
}
LLImageJ2C::~LLImageJ2C()
{
	if ( !j2cimpl_destroy_func )
	{
		j2cimpl_destroy_func = fallbackDestroyLLImageJ2CImpl;
	}
	if ( mImpl )
	{
		j2cimpl_destroy_func(mImpl);
	}
}
void LLImageJ2C::resetLastError()
{
	mLastError.clear();
}
void LLImageJ2C::setLastError(const std::string& message, const std::string& filename)
{
	mLastError = message;
	if (!filename.empty())
		mLastError += std::string(" FILE: ") + filename;
}
S8  LLImageJ2C::getRawDiscardLevel()
{
	return mRawDiscardLevel;
}
BOOL LLImageJ2C::updateData()
{
	BOOL res = TRUE;
	resetLastError();
	if (!getData() || (getDataSize() < 16))
	{
		setLastError("LLImageJ2C uninitialized");
		res = FALSE;
	}
	else
	{
		if (mImpl)
			res = mImpl->getMetadata(*this);
		else res = FALSE;
	}
	if (res)
	{
		S32 max_bytes = getDataSize();
		S32 discard = calcDiscardLevelBytes(max_bytes);
		setDiscardLevel(discard);
	}
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	return res;
}
BOOL LLImageJ2C::decode(LLImageRaw *raw_imagep, F32 decode_time)
{
	return decodeChannels(raw_imagep, decode_time, 0, 4);
}
BOOL LLImageJ2C::decodeChannels(LLImageRaw *raw_imagep, F32 decode_time, S32 first_channel, S32 max_channel_count )
{
	BOOL res = TRUE;
	resetLastError();
	if (!getData() || (getDataSize() < 16))
	{
		setLastError("LLImageJ2C uninitialized");
		res = TRUE;
	}
	else
	{
		updateRawDiscardLevel();
		mDecoding = TRUE;
		res = mImpl->decodeImpl(*this, *raw_imagep, decode_time, first_channel, max_channel_count);
	}
	if (res)
	{
		if (!mDecoding)
		{
			raw_imagep->deleteData();
		}
		else
		{
			mDecoding = FALSE;
		}
	}
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	return res;
}
BOOL LLImageJ2C::encode(const LLImageRaw *raw_imagep, F32 encode_time)
{
	return encode(raw_imagep, NULL, encode_time);
}
BOOL LLImageJ2C::encode(const LLImageRaw *raw_imagep, const char* comment_text, F32 encode_time)
{
	resetLastError();
	BOOL res = mImpl->encodeImpl(*this, *raw_imagep, comment_text, encode_time, mReversible);
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	return res;
}
S32 LLImageJ2C::calcHeaderSizeJ2C()
{
	return FIRST_PACKET_SIZE;
}
S32 LLImageJ2C::discardLevelForExtent(S32 full_extent, F32 screen_px)
{
	F32 px = screen_px;
	if (px < 8.f)
	{
		px = 8.f;
	}
	F32 full = (F32)full_extent;
	if (full < 1.f)
	{
		full = 1024.f;
	}
	F32 d = floorf(logf(full / px) / logf(2.f));
	if (d < 0.f)
	{
		d = 0.f;
	}
	if (d > (F32)MAX_DISCARD_LEVEL)
	{
		d = (F32)MAX_DISCARD_LEVEL;
	}
	return (S32)d;
}
S32 LLImageJ2C::calcDataSizeJ2C(S32 w, S32 h, S32 comp, S32 discard_level, F32 rate)
{
	(void)comp;
	if (rate <= 0.f) rate = .125f;
	if (discard_level < 0) discard_level = 0;
	if (discard_level > MAX_DISCARD_LEVEL) discard_level = MAX_DISCARD_LEVEL;
	const S32 precision = 8;
	const S32 max_components = 4;
	const S32 hard_cap = 12;
	const S64 base_layer_area = (S64)MAX_BLOCK_SIZE * (S64)MAX_BLOCK_SIZE;
	const S64 bits_per_tile = (S64)max_components * (S64)precision;
	const S32 discard_layers = llmax(5 - discard_level, 0);
	const double rate64 = (double)rate;
	const S64 header_bytes = (S64)calcHeaderSizeJ2C();
	S64 total_bits = 0;
	if (w <= 0 || h <= 0)
	{
		total_bits = (S64)std::llround((double)(base_layer_area * bits_per_tile) * rate64);
	}
	else
	{
		const S64 surface = (S64)w * (S64)h;
		S64 layer_area = base_layer_area;
		S32 nb_layers = 1;
		total_bits = (S64)std::llround((double)(layer_area * bits_per_tile) * rate64);
		while (surface > layer_area && nb_layers < hard_cap)
		{
			if (nb_layers <= discard_layers)
			{
				total_bits += (S64)std::llround((double)(layer_area * bits_per_tile) * rate64);
			}
			++nb_layers;
			layer_area *= 4;
		}
		const S32 max_dimension = llmax(w, h);
		const double ratio = (max_dimension > 0) ? (double)max_dimension / (double)MAX_BLOCK_SIZE : 0.0;
		S32 dimension_layers = 1;
		if (ratio > 0.0)
		{
			dimension_layers = (S32)std::floor(std::log2(ratio)) + 1;
			if (dimension_layers < 1)
			{
				dimension_layers = 1;
			}
		}
		if (dimension_layers > nb_layers)
		{
			S32 extra = llmin(dimension_layers, hard_cap) - nb_layers;
			while (extra > 0)
			{
				--extra;
				if (nb_layers <= discard_layers)
				{
					total_bits += (S64)std::llround((double)(layer_area * bits_per_tile) * rate64);
				}
				++nb_layers;
				layer_area *= 4;
			}
		}
	}
	S64 est = total_bits / 8 + header_bytes;
	if (est < 0)
	{
		est = 0;
	}
	if (est > 2147483647LL)
	{
		est = 2147483647LL;
	}
	return (S32)est;
}
S32 LLImageJ2C::calcHeaderSize()
{
	return calcHeaderSizeJ2C();
}
S32 LLImageJ2C::calcDataSize(S32 discard_level)
{
	static const LLCachedControl<bool> legacy_size("SianaLegacyJ2CSize", false);
	if (legacy_size) {
		return calcDataSizeJ2C(getWidth(), getHeight(), getComponents(), discard_level, mRate);
	}
	discard_level = llclamp(discard_level, 0, MAX_DISCARD_LEVEL);
	if ( mAreaUsedForDataSizeCalcs != (getHeight() * getWidth())
		|| mDataSizes[0] == 0)
	{
		mAreaUsedForDataSizeCalcs = getHeight() * getWidth();
		S32 level = MAX_DISCARD_LEVEL;
		while ( level >= 0 )
		{
			mDataSizes[level] = calcDataSizeJ2C(getWidth(), getHeight(), getComponents(), level, mRate);
			mDataSizes[level] = llmax(mDataSizes[level], calcHeaderSizeJ2C());
			level--;
		}
	}
	return mDataSizes[discard_level];
}
S32 LLImageJ2C::calcDiscardLevelBytes(S32 bytes)
{
	llassert(bytes >= 0);
	S32 discard_level = 0;
	if (bytes == 0)
	{
		return MAX_DISCARD_LEVEL;
	}
	while (1)
	{
		S32 bytes_needed = calcDataSize(discard_level);
		if (bytes >= bytes_needed - (bytes_needed>>2))
		{
			break;
		}
		discard_level++;
		if (discard_level >= MAX_DISCARD_LEVEL)
		{
			break;
		}
	}
	return discard_level;
}
void LLImageJ2C::setRate(F32 rate)
{
	mRate = rate;
}
void LLImageJ2C::setMaxBytes(S32 max_bytes)
{
	mMaxBytes = max_bytes;
}
void LLImageJ2C::setReversible(const BOOL reversible)
{
 	mReversible = reversible;
}
BOOL LLImageJ2C::loadAndValidate(const std::string &filename)
{
	BOOL res = TRUE;
	resetLastError();
	S32 file_size = 0;
	LLAPRFile infile(filename, LL_APR_RB, &file_size);
	apr_file_t* apr_file = infile.getFileHandle() ;
	if (!apr_file)
	{
		setLastError("Unable to open file for reading", filename);
		res = FALSE;
	}
	else if (file_size == 0)
	{
		setLastError("File is empty",filename);
		res = FALSE;
	}
	else
	{
		U8 *data = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), file_size);
		apr_size_t bytes_read = file_size;
		apr_status_t s = apr_file_read(apr_file, data, &bytes_read);
		infile.close() ;
		if (s != APR_SUCCESS || (S32)bytes_read != file_size)
		{
			FREE_MEM(LLImageBase::getPrivatePool(), data);
			setLastError("Unable to read entire file");
			res = FALSE;
		}
		else
		{
			res = validate(data, file_size);
		}
	}
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	return res;
}
BOOL LLImageJ2C::validate(U8 *data, U32 file_size)
{
	resetLastError();
	setData(data, file_size);
	BOOL res = updateData();
	if ( res )
	{
		if (!getData() || (getDataSize() < 16))
		{
			setLastError("LLImageJ2C uninitialized");
			res = FALSE;
		}
		else
		{
			if (mImpl)
				res = mImpl->getMetadata(*this);
			else res = FALSE;
		}
	}
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	return res;
}
void LLImageJ2C::decodeFailed()
{
	mDecoding = FALSE;
}
void LLImageJ2C::updateRawDiscardLevel()
{
	mRawDiscardLevel = mMaxBytes ? calcDiscardLevelBytes(mMaxBytes) : mDiscardLevel;
}
LLImageJ2CImpl::~LLImageJ2CImpl()
{
}
