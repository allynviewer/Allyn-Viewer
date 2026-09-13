/** 
 * @file lscript_heapruntime.cpp
 * @brief classes to manage script heap at runtime
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
#if 0
#include "linden_common.h"
#include "lscript_heapruntime.h"
#include "lscript_execute.h"
LLScriptHeapRunTime::LLScriptHeapRunTime()
: mLastEmpty(0), mBuffer(NULL), mCurrentPosition(0), mStackPointer(0), mHeapRegister(0), mbPrint(FALSE)
{
}
LLScriptHeapRunTime::~LLScriptHeapRunTime()
{
}
S32 LLScriptHeapRunTime::addData(char *string)
{
	if (!mBuffer)
		return 0;
	S32 size = strlen(string) + 1;
	S32 block_offset = findOpenBlock(size + HEAP_BLOCK_HEADER_SIZE);
	if (mCurrentPosition)
	{
		S32 offset = mCurrentPosition;
		if (offset + block_offset + HEAP_BLOCK_HEADER_SIZE + LSCRIPTDataSize[LST_INTEGER] >= mStackPointer)
		{
			set_fault(mBuffer, LSRF_STACK_HEAP_COLLISION);
			return 0;
		}
		integer2bytestream(mBuffer, offset, block_offset);
		integer2bytestream(mBuffer, offset, 1);
		*(mBuffer + offset++) = LSCRIPTTypeByte[LST_STRING];
		char2bytestream(mBuffer, offset, string);
		if (mbPrint)
			printf("0x%X created ref count %d\n", mCurrentPosition - mHeapRegister, 1);
	}
	return mCurrentPosition;
}
S32 LLScriptHeapRunTime::addData(U8 *list)
{
	if (!mBuffer)
		return 0;
	return 0;
}
S32 LLScriptHeapRunTime::catStrings(S32 address1, S32 address2)
{
	if (!mBuffer)
		return 0;
	S32 dataaddress1 = address1 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;
	S32 dataaddress2 = address2 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;
	S32 toffset1 = dataaddress1;
	safe_heap_bytestream_count_char(mBuffer, toffset1);
	S32 toffset2 = dataaddress2;
	safe_heap_bytestream_count_char(mBuffer, toffset2);
	S32 size1 = toffset1 - dataaddress1;
	S32 size2 = toffset2 - dataaddress2;
	S32 newbuffer = size1 + size2 - 1;
	char *temp = new char[newbuffer];
	bytestream2char(temp, mBuffer, dataaddress1);
	bytestream2char(temp + size1 - 1, mBuffer, dataaddress2);
	decreaseRefCount(address1);
	decreaseRefCount(address2);
	S32 retaddress = addData(temp);
	return retaddress;
}
S32 LLScriptHeapRunTime::cmpStrings(S32 address1, S32 address2)
{
	if (!mBuffer)
		return 0;
	S32 dataaddress1 = address1 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;
	S32 dataaddress2 = address2 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;
	S32 toffset1 = dataaddress1;
	safe_heap_bytestream_count_char(mBuffer, toffset1);
	S32 toffset2 = dataaddress2;
	safe_heap_bytestream_count_char(mBuffer, toffset2);
	S32 size1 = toffset1 - dataaddress1;
	S32 size2 = toffset2 - dataaddress2;
	if (size1 != size2)
	{
		return llmin(size1, size2);
	}
	else
	{
		return strncmp((char *)(mBuffer + dataaddress1), (char *)(mBuffer + dataaddress2), size1);
	}
}
void LLScriptHeapRunTime::removeData(S32 address)
{
	if (!mBuffer)
		return;
	S32 toffset = address;
	bytestream2integer(mBuffer, toffset);
	integer2bytestream(mBuffer, toffset, 0);
	*(mBuffer + toffset) = 0;
	S32 clean = mHeapRegister;
	S32 tclean;
	S32 clean_offset;
	S32 nclean;
	S32 tnclean;
	S32 next_offset;
	U8  type;
	U8  ntype;
	for(;;)
	{
		tclean = clean;
		clean_offset = bytestream2integer(mBuffer, tclean);
		tclean += LSCRIPTDataSize[LST_INTEGER];
		type = *(mBuffer + tclean);
		if (!clean_offset)
		{
			if (!type)
			{
				set_register(mBuffer, LREG_HP, clean);
			}
			return;
		}
		if (!type)
		{
			nclean = clean + clean_offset;
			tnclean = nclean;
			next_offset = bytestream2integer(mBuffer, tnclean);
			tnclean += LSCRIPTDataSize[LST_INTEGER];
			ntype = *(mBuffer + tnclean);
			if (!next_offset)
			{
				tclean = clean;
				integer2bytestream(mBuffer, tclean, 0);
				set_register(mBuffer, LREG_HP, clean);
				return;
			}
			if (!ntype)
			{
				tclean = clean;
				integer2bytestream(mBuffer, tclean, clean_offset + next_offset);
			}
			else
			{
				clean += clean_offset;
			}
		}
		else
		{
			clean += clean_offset;
		}
	}
}
void LLScriptHeapRunTime::coalesce(S32 address1, S32 address2)
{
	S32 toffset = address1;
	S32 offset1 = bytestream2integer(mBuffer, toffset);
	offset1 += bytestream2integer(mBuffer, address2);
	integer2bytestream(mBuffer, address1, offset1);
}
void LLScriptHeapRunTime::split(S32 address1, S32 size)
{
	S32 toffset = address1;
	S32 oldoffset = bytestream2integer(mBuffer, toffset);
	S32 newoffset = oldoffset - size;
	S32 newblockpos = address1 + size;
	integer2bytestream(mBuffer, newblockpos, newoffset);
	integer2bytestream(mBuffer, newblockpos, 0);
	*(mBuffer + newblockpos) = 0;
	integer2bytestream(mBuffer, address1, size + HEAP_BLOCK_HEADER_SIZE);
}
void LLScriptHeapRunTime::increaseRefCount(S32 address)
{
	if (!mBuffer)
		return;
	if (!address)
	{
		return;
	}
	S32 toffset = address + 4;
	S32 count = bytestream2integer(mBuffer, toffset);
	count++;
	if (mbPrint)
		printf("0x%X inc ref count %d\n", address - mHeapRegister, count);
	U8 type = *(mBuffer + toffset);
	if (type == LSCRIPTTypeByte[LST_STRING])
	{
		toffset = address + 4;
		integer2bytestream(mBuffer, toffset, count);
	}
	else
	{
		set_fault(mBuffer, LSRF_HEAP_ERROR);
	}
}
void LLScriptHeapRunTime::decreaseRefCount(S32 address)
{
	if (!mBuffer)
		return;
	if (!address)
	{
		return;
	}
	S32 toffset = address;
	bytestream2integer(mBuffer, toffset);
	S32 count = bytestream2integer(mBuffer, toffset);
	U8 type = *(mBuffer + toffset);
	if (type == LSCRIPTTypeByte[LST_STRING])
	{
		count--;
		if (mbPrint)
			printf("0x%X dec ref count %d\n", address - mHeapRegister, count);
		toffset = address + 4;
		integer2bytestream(mBuffer, toffset, count);
		if (!count)
		{
			removeData(address);
		}
	}
	else
	{
		set_fault(mBuffer, LSRF_HEAP_ERROR);
	}
}
void LLScriptHeapRunTime::releaseLocal(S32 address)
{
	S32 hr = get_register(mBuffer, LREG_HR);
	address = lscript_local_get(mBuffer, address);
	if (  (address >= hr)
		&&(address < hr + get_register(mBuffer, LREG_HP)))
	{
		decreaseRefCount(address);
	}
}
void LLScriptHeapRunTime::releaseGlobal(S32 address)
{
	S32 hr = get_register(mBuffer, LREG_HR);
	address = lscript_global_get(mBuffer, address) + hr;
	if (  (address >= hr)
		&&(address <  hr + get_register(mBuffer, LREG_HP)))
	{
		decreaseRefCount(address);
	}
}
#if defined(_MSC_VER)
# pragma warning(disable: 4702)
#endif
S32 LLScriptHeapRunTime::findOpenBlock(S32 size)
{
	S32 offset;
	S32 toffset;
	U8 blocktype;
	while(1)
	{
		if (mCurrentPosition + size >= mStackPointer)
		{
			set_fault(mBuffer, LSRF_STACK_HEAP_COLLISION);
			mCurrentPosition = 0;
		}
		toffset = mCurrentPosition;
		offset = bytestream2integer(mBuffer, toffset);
		if (!offset)
		{
			if (mLastEmpty)
			{
				mCurrentPosition = mLastEmpty;
				integer2bytestream(mBuffer, mLastEmpty, 0);
				mLastEmpty = 0;
			}
			offset = mCurrentPosition + size;
			integer2bytestream(mBuffer, offset, 0);
			set_register(mBuffer, LREG_HP, mCurrentPosition + size);
			return size;
		}
		toffset += LSCRIPTDataSize[LST_INTEGER];
		blocktype = *(mBuffer + toffset++);
		if (!blocktype)
		{
			if (mLastEmpty)
			{
				coalesce(mLastEmpty, mCurrentPosition);
				mCurrentPosition = mLastEmpty;
				toffset = mCurrentPosition;
				offset = bytestream2integer(mBuffer, toffset);
			}
			if (offset >= size)
			{
				if (offset - HEAP_BLOCK_SPLIT_THRESHOLD >= size)
				{
					split(mCurrentPosition, size);
					return size;
				}
				else
					return offset;
			}
		}
		mCurrentPosition += offset;
	}
	mCurrentPosition = 0;
	return 0;
}
LLScriptHeapRunTime	gRunTime;
#endif
