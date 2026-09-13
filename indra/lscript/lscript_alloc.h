/** 
 * @file lscript_alloc.h
 * @brief General heap management for scripting system
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
#ifndef LL_LSCRIPT_ALLOC_H
#define LL_LSCRIPT_ALLOC_H
#include "lscript_byteconvert.h"
#include "lscript_library.h"
void reset_hp_to_safe_spot(const U8 *buffer);
const S32 MAX_HEAP_SIZE = TOP_OF_MEMORY;
class LLScriptAllocEntry
{
public:
	LLScriptAllocEntry() : mSize(0), mType(LST_NULL), mReferenceCount(0) {}
	LLScriptAllocEntry(S32 offset, U8 type) : mSize(offset), mType(type), mReferenceCount(1) {}
	friend std::ostream&	 operator<<(std::ostream& s, const LLScriptAllocEntry &a)
	{
		s << "Size: " << a.mSize << " Type: " << LSCRIPTTypeNames[a.mType] << " Count: " << a.mReferenceCount;
		return s;
	}
	S32 mSize;
	U8	mType;
	S16 mReferenceCount;
};
const S32 SIZEOF_SCRIPT_ALLOC_ENTRY = 7;
inline void alloc_entry2bytestream(U8 *buffer, S32 &offset, const LLScriptAllocEntry &entry)
{
	if (  (offset < 0)
		||(offset > MAX_HEAP_SIZE))
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
	}
	else
	{
		integer2bytestream(buffer, offset, entry.mSize);
		byte2bytestream(buffer, offset, entry.mType);
		s162bytestream(buffer, offset, entry.mReferenceCount);
	}
}
inline void bytestream2alloc_entry(LLScriptAllocEntry &entry, U8 *buffer, S32 &offset)
{
	if (  (offset < 0)
		||(offset > MAX_HEAP_SIZE))
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
		reset_hp_to_safe_spot(buffer);
	}
	else
	{
		entry.mSize = bytestream2integer(buffer, offset);
		entry.mType = bytestream2byte(buffer, offset);
		entry.mReferenceCount = bytestream2s16(buffer, offset);
	}
}
BOOL lsa_create_heap(U8 *heap_start, S32 size);
void lsa_fprint_heap(U8 *buffer, LLFILE *fp);
void lsa_print_heap(U8 *buffer);
S32 lsa_heap_add_data(U8 *buffer, LLScriptLibData *data, S32 heapsize, BOOL b_delete);
S32 lsa_heap_top(U8 *heap_start, S32 maxsize);
void lsa_split_block(U8 *buffer, S32 &offset, S32 size, LLScriptAllocEntry &entry);
void lsa_insert_data(U8 *buffer, S32 &offset, LLScriptLibData *data, LLScriptAllocEntry &entry, S32 heapsize);
S32 lsa_create_data_block(U8 **buffer, LLScriptLibData *data, S32 base_offset);
void lsa_increase_ref_count(U8 *buffer, S32 offset);
void lsa_decrease_ref_count(U8 *buffer, S32 offset);
inline S32 get_max_heap_size(U8 *buffer)
{
	return get_register(buffer, LREG_SP) - get_register(buffer, LREG_HR);
}
LLScriptLibData *lsa_get_data(U8 *buffer, S32 &offset, BOOL b_dec_ref);
LLScriptLibData *lsa_get_list_ptr(U8 *buffer, S32 &offset, BOOL b_dec_ref);
S32 lsa_cat_strings(U8 *buffer, S32 offset1, S32 offset2, S32 heapsize);
S32 lsa_cmp_strings(U8 *buffer, S32 offset1, S32 offset2);
S32 lsa_cat_lists(U8 *buffer, S32 offset1, S32 offset2, S32 heapsize);
S32 lsa_cmp_lists(U8 *buffer, S32 offset1, S32 offset2);
S32 lsa_preadd_lists(U8 *buffer, LLScriptLibData *data, S32 offset2, S32 heapsize);
S32 lsa_postadd_lists(U8 *buffer, S32 offset1, LLScriptLibData *data, S32 heapsize);
inline LLScriptLibData *lsa_bubble_sort(LLScriptLibData *src, S32 stride, S32 ascending)
{
	S32 number = src->getListLength();
	if (number <= 0)
	{
		return NULL;
	}
	if (stride <= 0)
	{
		stride = 1;
	}
	S32 i = 0;
	if (number % stride)
	{
		LLScriptLibData *retval = src->mListp;
		src->mListp = NULL;
		return retval;
	}
	LLScriptLibData **sortarray = new LLScriptLibData*[number];
	LLScriptLibData *temp = src->mListp;
	while (temp)
	{
		sortarray[i] = temp;
		i++;
		temp = temp->mListp;
	}
	S32 j, s;
	for (i = 0; i < number; i += stride)
	{
		for (j = i; j < number; j += stride)
		{
			if (  ((*sortarray[i]) <= (*sortarray[j]))
				!= (ascending == TRUE))
			{
				for (s = 0; s < stride; s++)
				{
					temp = sortarray[i + s];
					sortarray[i + s] = sortarray[j + s];
					sortarray[j + s] = temp;
				}
			}
		}
	}
	i = 1;
	temp = sortarray[0];
	while (i < number)
	{
		temp->mListp = sortarray[i++];
		temp = temp->mListp;
	}
	temp->mListp = NULL;
	src->mListp = NULL;
	temp = sortarray[0];
	delete[] sortarray;
	return temp;
}
LLScriptLibData* lsa_randomize(LLScriptLibData* src, S32 stride);
#endif
