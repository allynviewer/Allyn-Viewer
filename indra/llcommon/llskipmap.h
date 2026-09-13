/** 
 * @file llskipmap.h
 * @brief Associative container based on the skiplist algorithm.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
#ifndef LL_LLSKIPMAP_H
#define LL_LLSKIPMAP_H
#include "llerror.h"
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH = 8>
class LLSkipMap
{
public:
	LLSkipMap();
	LLSkipMap(BOOL	(*insert_first)(const INDEX_TYPE &first, const INDEX_TYPE &second),
			   BOOL	(*equals)(const INDEX_TYPE &first, const INDEX_TYPE &second));
	~LLSkipMap();
	void setInsertFirst(BOOL (*insert_first)(const INDEX_TYPE &first, const INDEX_TYPE &second));
	void setEquals(BOOL (*equals)(const INDEX_TYPE &first, const INDEX_TYPE &second));
	DATA_TYPE &addData(const INDEX_TYPE &index, DATA_TYPE datap);
	DATA_TYPE &addData(const INDEX_TYPE &index);
	DATA_TYPE &getData(const INDEX_TYPE &index);
	DATA_TYPE &operator[](const INDEX_TYPE &index);
	DATA_TYPE &getData(const INDEX_TYPE &index, BOOL &b_new_entry);
	BOOL checkData(const INDEX_TYPE &index);
	BOOL checkKey(const INDEX_TYPE &index);
	DATA_TYPE getIfThere(const INDEX_TYPE &index);
	INDEX_TYPE reverseLookup(const DATA_TYPE datap);
	S32 getLength();
	BOOL removeData(const INDEX_TYPE &index);
	void removeAllData();
	void resetList();
	DATA_TYPE	getCurrentDataWithoutIncrement();
	DATA_TYPE	getCurrentData();
	DATA_TYPE	getNextData();
	INDEX_TYPE	getNextKey();
	INDEX_TYPE	getCurrentKeyWithoutIncrement();
	BOOL		notDone() const;
	void removeCurrentData();
	void deleteCurrentData();
	DATA_TYPE	getFirstData();
	INDEX_TYPE	getFirstKey();
	class LLSkipMapNode
	{
	public:
		LLSkipMapNode()
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
			U8  *zero = (U8 *)&mIndex;
			for (i = 0; i < (S32)sizeof(INDEX_TYPE); i++)
			{
				*(zero + i) = 0;
			}
			zero = (U8 *)&mData;
			for (i = 0; i < (S32)sizeof(DATA_TYPE); i++)
			{
				*(zero + i) = 0;
			}
		}
		LLSkipMapNode(const INDEX_TYPE &index)
		:	mIndex(index)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
			U8 *zero = (U8 *)&mData;
			for (i = 0; i < (S32)sizeof(DATA_TYPE); i++)
			{
				*(zero + i) = 0;
			}
		}
		LLSkipMapNode(const INDEX_TYPE &index, DATA_TYPE datap)
		:	mIndex(index)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
			mData = datap;
		}
		~LLSkipMapNode()
		{
		}
		INDEX_TYPE					mIndex;
		DATA_TYPE					mData;
		LLSkipMapNode				*mForward[BINARY_DEPTH];
	private:
		LLSkipMapNode(const LLSkipMapNode &);
		LLSkipMapNode &operator=(const LLSkipMapNode &rhs);
	};
	static BOOL	defaultEquals(const INDEX_TYPE &first, const INDEX_TYPE &second)
	{
		return first == second;
	}
private:
	LLSkipMap(const LLSkipMap &);
	LLSkipMap &operator=(const LLSkipMap &);
private:
	LLSkipMapNode				mHead;
	LLSkipMapNode				*mUpdate[BINARY_DEPTH];
	LLSkipMapNode				*mCurrentp;
	LLSkipMapNode				*mCurrentOperatingp;
	S32							mLevel;
	BOOL						(*mInsertFirst)(const INDEX_TYPE &first, const INDEX_TYPE &second);
	BOOL						(*mEquals)(const INDEX_TYPE &first, const INDEX_TYPE &second);
	S32							mNumberOfSteps;
};
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::LLSkipMap()
	:	mInsertFirst(NULL),
		mEquals(defaultEquals)
{
	cassert(BINARY_DEPTH >= 2);
	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mUpdate[i] = NULL;
	}
	mLevel = 1;
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::LLSkipMap(BOOL	(*insert_first)(const INDEX_TYPE &first, const INDEX_TYPE &second),
			   BOOL	(*equals)(const INDEX_TYPE &first, const INDEX_TYPE &second))
	:	mInsertFirst(insert_first),
		mEquals(equals)
{
	cassert(BINARY_DEPTH >= 2);
	mLevel = 1;
	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mHead.mForward[i] = NULL;
		mUpdate[i] = NULL;
	}
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::~LLSkipMap()
{
	removeAllData();
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::setInsertFirst(BOOL (*insert_first)(const INDEX_TYPE &first, const INDEX_TYPE &second))
{
	mInsertFirst = insert_first;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::setEquals(BOOL (*equals)(const INDEX_TYPE &first, const INDEX_TYPE &second))
{
	mEquals = equals;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE &LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::addData(const INDEX_TYPE &index, DATA_TYPE datap)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	current = *current->mForward;
	if (  (current)
		&&(mEquals(current->mIndex, index)))
	{
		current->mData = datap;
		return current->mData;
	}
	S32 newlevel;
	for (newlevel = 1; newlevel <= mLevel && newlevel < BINARY_DEPTH; newlevel++)
	{
		if (rand() & 1)
		{
			break;
		}
	}
	LLSkipMapNode *snode = new LLSkipMapNode(index, datap);
	if (newlevel > mLevel)
	{
		mHead.mForward[mLevel] = NULL;
		mUpdate[mLevel] = &mHead;
		mLevel = newlevel;
	}
	for (level = 0; level < newlevel; level++)
	{
		snode->mForward[level] = mUpdate[level]->mForward[level];
		mUpdate[level]->mForward[level] = snode;
	}
	return snode->mData;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE &LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::addData(const INDEX_TYPE &index)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	current = *current->mForward;
	S32 newlevel;
	for (newlevel = 1; newlevel <= mLevel && newlevel < BINARY_DEPTH; newlevel++)
	{
		if (rand() & 1)
			break;
	}
	LLSkipMapNode *snode = new LLSkipMapNode(index);
	if (newlevel > mLevel)
	{
		mHead.mForward[mLevel] = NULL;
		mUpdate[mLevel] = &mHead;
		mLevel = newlevel;
	}
	for (level = 0; level < newlevel; level++)
	{
		snode->mForward[level] = mUpdate[level]->mForward[level];
		mUpdate[level]->mForward[level] = snode;
	}
	return snode->mData;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE &LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getData(const INDEX_TYPE &index)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	mNumberOfSteps = 0;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	current = *current->mForward;
	mNumberOfSteps++;
	if (  (current)
		&&(mEquals(current->mIndex, index)))
	{
		return current->mData;
	}
	S32 newlevel;
	for (newlevel = 1; newlevel <= mLevel && newlevel < BINARY_DEPTH; newlevel++)
	{
		if (rand() & 1)
			break;
	}
	LLSkipMapNode *snode = new LLSkipMapNode(index);
	if (newlevel > mLevel)
	{
		mHead.mForward[mLevel] = NULL;
		mUpdate[mLevel] = &mHead;
		mLevel = newlevel;
	}
	for (level = 0; level < newlevel; level++)
	{
		snode->mForward[level] = mUpdate[level]->mForward[level];
		mUpdate[level]->mForward[level] = snode;
	}
	return snode->mData;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE &LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::operator[](const INDEX_TYPE &index)
{
	return getData(index);
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE &LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getData(const INDEX_TYPE &index, BOOL &b_new_entry)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	mNumberOfSteps = 0;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	mNumberOfSteps++;
	current = *current->mForward;
	if (  (current)
		&&(mEquals(current->mIndex, index)))
	{
		return current->mData;
	}
	b_new_entry = TRUE;
	addData(index);
	return current->mData;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::checkData(const INDEX_TYPE &index)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	current = *current->mForward;
	if (current)
	{
		if (current->mData)
		{
			return mEquals(current->mIndex, index);
		}
	}
	return FALSE;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::checkKey(const INDEX_TYPE &index)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	current = *current->mForward;
	if (current)
	{
		return mEquals(current->mIndex, index);
	}
	return FALSE;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getIfThere(const INDEX_TYPE &index)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	mNumberOfSteps = 0;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	mNumberOfSteps++;
	current = *current->mForward;
	if (current)
	{
		if (mEquals(current->mIndex, index))
		{
			return current->mData;
		}
	}
	return DATA_TYPE();
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline INDEX_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::reverseLookup(const DATA_TYPE datap)
{
	LLSkipMapNode	*current = &mHead;
	while (current)
	{
		if (datap == current->mData)
		{
			return current->mIndex;
		}
		current = *current->mForward;
	}
	return INDEX_TYPE();
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline S32 LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getLength()
{
	U32	length = 0;
	for (LLSkipMapNode* temp = *(mHead.mForward); temp != NULL; temp = temp->mForward[0])
	{
		length++;
	}
	return length;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::removeData(const INDEX_TYPE &index)
{
	S32			level;
	LLSkipMapNode	*current = &mHead;
	LLSkipMapNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	current = *current->mForward;
	if (!current)
	{
		return FALSE;
	}
	if (!mEquals(current->mIndex, index))
	{
		return FALSE;
	}
	else
	{
		if (current == mCurrentp)
		{
			mCurrentp = *current->mForward;
		}
		if (current == mCurrentOperatingp)
		{
			mCurrentOperatingp = *current->mForward;
		}
		for (level = 0; level < mLevel; level++)
		{
			if (*((*(mUpdate + level))->mForward + level) != current)
			{
				break;
			}
			*((*(mUpdate + level))->mForward + level) = *(current->mForward + level);
		}
		delete current;
		while (  (mLevel > 1)
			   &&(!*(mHead.mForward + mLevel - 1)))
		{
			mLevel--;
		}
	}
	return TRUE;
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
void LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::removeAllData()
{
	LLSkipMapNode *temp;
	mCurrentp = *(mHead.mForward);
	while (mCurrentp)
	{
		temp = mCurrentp->mForward[0];
		delete mCurrentp;
		mCurrentp = temp;
	}
	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mHead.mForward[i] = NULL;
		mUpdate[i] = NULL;
	}
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::resetList()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getCurrentDataWithoutIncrement()
{
	if (mCurrentOperatingp)
	{
		return mCurrentOperatingp->mData;
	}
	else
	{
		return DATA_TYPE();
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getCurrentData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		return DATA_TYPE();
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getNextData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		return DATA_TYPE();
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline INDEX_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getNextKey()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mIndex;
	}
	else
	{
		return mHead.mIndex;
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline INDEX_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getCurrentKeyWithoutIncrement()
{
	if (mCurrentOperatingp)
	{
		return mCurrentOperatingp->mIndex;
	}
	else
	{
		return INDEX_TYPE();
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::notDone() const
{
	if (mCurrentOperatingp)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::removeCurrentData()
{
	if (mCurrentOperatingp)
	{
		removeData(mCurrentOperatingp->mIndex);
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::deleteCurrentData()
{
	if (mCurrentOperatingp)
	{
		deleteData(mCurrentOperatingp->mIndex);
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getFirstData()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		return DATA_TYPE();
	}
}
template <class INDEX_TYPE, class DATA_TYPE, S32 BINARY_DEPTH>
inline INDEX_TYPE LLSkipMap<INDEX_TYPE, DATA_TYPE, BINARY_DEPTH>::getFirstKey()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mIndex;
	}
	else
	{
		return mHead.mIndex;
	}
}
#endif
