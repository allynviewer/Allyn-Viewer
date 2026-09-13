/** 
 * @file llimagej2c.h
 * @brief Image implmenation for jpeg2000.
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
#ifndef LL_LLIMAGEJ2C_H
#define LL_LLIMAGEJ2C_H
#include "llimage.h"
#include "llassettype.h"
class LLImageJ2CImpl;
class LLImageJ2C : public LLImageFormatted
{
protected:
	virtual ~LLImageJ2C();
public:
	LLImageJ2C();
	std::string getExtension() { return std::string("j2c"); }
	BOOL updateData();
	BOOL decode(LLImageRaw *raw_imagep, F32 decode_time);
	BOOL decodeChannels(LLImageRaw *raw_imagep, F32 decode_time, S32 first_channel, S32 max_channel_count);
	BOOL encode(const LLImageRaw *raw_imagep, F32 encode_time);
	S32 calcHeaderSize();
	S32 calcDataSize(S32 discard_level = 0);
	S32 calcDiscardLevelBytes(S32 bytes);
	S8  getRawDiscardLevel();
	void resetLastError();
	void setLastError(const std::string& message, const std::string& filename = std::string());
	BOOL encode(const LLImageRaw *raw_imagep, const char* comment_text, F32 encode_time=0.0);
	BOOL validate(U8 *data, U32 file_size);
	BOOL loadAndValidate(const std::string &filename);
	void setReversible(const BOOL reversible);
	void setRate(F32 rate);
	void setMaxBytes(S32 max_bytes);
	S32 getMaxBytes() const { return mMaxBytes; }
	static S32 calcHeaderSizeJ2C();
	static S32 calcDataSizeJ2C(S32 w, S32 h, S32 comp, S32 discard_level, F32 rate = 0.f);
	static void openDSO();
	static void closeDSO();
	static std::string getEngineInfo();
protected:
	friend class LLImageJ2CImpl;
	friend class LLImageJ2COJ;
	friend class LLImageJ2CKDU;
	void decodeFailed();
	void updateRawDiscardLevel();
	S32 mMaxBytes;
	S32 mDataSizes[MAX_DISCARD_LEVEL+1];
	U32 mAreaUsedForDataSizeCalcs;
	S8  mRawDiscardLevel;
	F32 mRate;
	BOOL mReversible;
	LLImageJ2CImpl *mImpl;
	std::string mLastError;
};
class LLImageJ2CImpl
{
public:
	virtual ~LLImageJ2CImpl();
protected:
	virtual BOOL getMetadata(LLImageJ2C &base) = 0;
	virtual BOOL decodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count) = 0;
	virtual BOOL encodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text, F32 encode_time=0.0,
							BOOL reversible=FALSE) = 0;
	friend class LLImageJ2C;
};
#define LINDEN_J2C_COMMENT_PREFIX "LL_"
#endif
