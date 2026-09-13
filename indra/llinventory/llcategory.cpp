/** 
 * @file llcategory.cpp
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
#include "linden_common.h"
#include "llcategory.h"
#include "message.h"
const LLCategory LLCategory::none;
const S32 CATEGORY_INDEX[] =
{
	1,
	6,
	7,
	7,
	7,
	7,
	7,
	7,
};
const char* CATEGORY_NAME[] =
{
	"(none)",
	"Object",
	"Clothing",
	"Texture",
	"Sound",
	"Landmark",
	"Component",
	NULL
};
LLCategory::LLCategory()
{
	mData[0] = 0;
	mData[1] = 0;
	mData[2] = 0;
	mData[3] = 0;
}
void LLCategory::init(U32 value)
{
	U8 v;
	for(S32 i = 0; i < CATEGORY_DEPTH; i++)
	{
		v = (U8)((0x000000ff) & value);
		mData[CATEGORY_DEPTH - 1 - i] = v;
		value >>= 8;
	}
}
U32 LLCategory::getU32() const
{
	U32 rv = 0;
	rv |= mData[0];
	rv <<= 8;
	rv |= mData[1];
	rv <<= 8;
	rv |= mData[2];
	rv <<= 8;
	rv |= mData[3];
	return rv;
}
S32 LLCategory::getSubCategoryCount() const
{
	S32 rv = CATEGORY_INDEX[mData[0] + 1] - CATEGORY_INDEX[mData[0]];
	return rv;
}
LLCategory LLCategory::getSubCategory(U8 n) const
{
	LLCategory rv(*this);
	for(S32 i = 0; i < (CATEGORY_DEPTH - 1); i++)
	{
		if(rv.mData[i] == 0)
		{
			rv.mData[i] = n + 1;
			break;
		}
	}
	return rv;
}
const char* LLCategory::lookupName() const
{
	S32 i = 0;
	S32 index = mData[i++];
	while((i < CATEGORY_DEPTH) && (mData[i] != 0))
	{
		index = CATEGORY_INDEX[index];
		++i;
	}
	return CATEGORY_NAME[index];
}
void LLCategory::packMessage(LLMessageSystem* msg) const
{
	U32 data = getU32();
	msg->addU32Fast(_PREHASH_Category, data);
}
void LLCategory::unpackMessage(LLMessageSystem* msg, const char* block)
{
	U32 data;
	msg->getU32Fast(block, _PREHASH_Category, data);
	init(data);
}
void LLCategory::unpackMultiMessage(LLMessageSystem* msg, const char* block,
									S32 block_num)
{
	U32 data;
	msg->getU32Fast(block, _PREHASH_Category, data, block_num);
	init(data);
}
