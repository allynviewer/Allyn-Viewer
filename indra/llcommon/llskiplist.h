/** 
 * @file llskiplist.h
 * @brief skip list implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#ifndef LL_LLSKIPLIST_H
#define LL_LLSKIPLIST_H
#include "llrand.h"
#include "llrand.h"
template <class DATA_TYPE, S32 BINARY_DEPTH = 10>
class LLSkipList
{
public:
	typedef BOOL (*compare)(const DATA_TYPE& first, const DATA_TYPE& second);
	typedef compare insert_func;
	typedef compare equals_func;
	void init();
	LLSkipList();
	LLSkipList(insert_func insert_first, equals_func equals);
	~LLSkipList();
	inline void setInsertFirst(insert_func insert_first);
	inline void setEquals(equals_func equals);
	inline BOOL addData(const DATA_TYPE& data);
	inline BOOL checkData(const DATA_TYPE& data);
	inline S32 getLength() const;
	inline BOOL moveData(const DATA_TYPE& data, LLSkipList *newlist);
	inline BOOL removeData(const DATA_TYPE& data);
	inline void removeAllNodes();
	inline void resetList();
	inline DATA_TYPE getCurrentData();
	inline DATA_TYPE getNextData();
	inline void removeCurrentData();
	inline DATA_TYPE getFirstData();
	class LLSkipNode
	{
	public:
		LLSkipNode()
		:	mData(0)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
		}
		LLSkipNode(DATA_TYPE data)
			: mData(data)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
		}
		~LLSkipNode()
		{
		}
		DATA_TYPE					mData;
		LLSkipNode					*mForward[BINARY_DEPTH];
	private:
		LLSkipNode(const LLSkipNode &);
		LLSkipNode &operator=(const LLSkipNode &);
	};
	static BOOL defaultEquals(const DATA_TYPE& first, const DATA_TYPE& second)
	{
		return first == second;
	}
private:
	LLSkipNode					mHead;
	LLSkipNode					*mUpdate[BINARY_DEPTH];
	LLSkipNode					*mCurrentp;
	LLSkipNode					*mCurrentOperatingp;
	S32							mLevel;
	insert_func mInsertFirst;
	equals_func mEquals;
private:
	LLSkipList(const LLSkipList &);
	LLSkipList &operator=(const LLSkipList &);
};
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::init()
{
	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mHead.mForward[i] = NULL;
		mUpdate[i] = NULL;
	}
	mLevel = 1;
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipList<DATA_TYPE, BINARY_DEPTH>::LLSkipList()
:	mInsertFirst(NULL),
	mEquals(defaultEquals)
{
	init();
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipList<DATA_TYPE, BINARY_DEPTH>::LLSkipList(insert_func insert,
													   equals_func equals)
:	mInsertFirst(insert),
	mEquals(equals)
{
	init();
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipList<DATA_TYPE, BINARY_DEPTH>::~LLSkipList()
{
	removeAllNodes();
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::setInsertFirst(insert_func insert_first)
{
	mInsertFirst = insert_first;
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::setEquals(equals_func equals)
{
	mEquals = equals;
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::addData(const DATA_TYPE& data)
{
	S32			level;
	LLSkipNode	*current = &mHead;
	LLSkipNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mData, data)))
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
				   &&(temp->mData < data))
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
		if (ll_frand() < 0.5f)
			break;
	}
	LLSkipNode *snode = new LLSkipNode(data);
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
	return TRUE;
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::checkData(const DATA_TYPE& data)
{
	S32			level;
	LLSkipNode	*current = &mHead;
	LLSkipNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mData, data)))
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
				   &&(temp->mData < data))
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
		return mEquals(current->mData, data);
	}
	return FALSE;
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline S32 LLSkipList<DATA_TYPE, BINARY_DEPTH>::getLength() const
{
	U32	length = 0;
	for (LLSkipNode* temp = *(mHead.mForward); temp != NULL; temp = temp->mForward[0])
	{
		length++;
	}
	return length;
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::moveData(const DATA_TYPE& data, LLSkipList *newlist)
{
	BOOL removed = removeData(data);
	BOOL added = newlist->addData(data);
	return removed && added;
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::removeData(const DATA_TYPE& data)
{
	S32			level;
	LLSkipNode	*current = &mHead;
	LLSkipNode	*temp;
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mData, data)))
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
				   &&(temp->mData < data))
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
	if (!mEquals(current->mData, data))
	{
		return FALSE;
	}
	else
	{
		if (current == mCurrentp)
		{
			mCurrentp = current->mForward[0];
		}
		if (current == mCurrentOperatingp)
		{
			mCurrentOperatingp = current->mForward[0];
		}
		for (level = 0; level < mLevel; level++)
		{
			if (mUpdate[level]->mForward[level] != current)
			{
				break;
			}
			mUpdate[level]->mForward[level] = current->mForward[level];
		}
		delete current;
		while (  (mLevel > 1)
			   &&(!mHead.mForward[mLevel - 1]))
		{
			mLevel--;
		}
	}
	return TRUE;
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::removeAllNodes()
{
	LLSkipNode *temp;
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
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::resetList()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipList<DATA_TYPE, BINARY_DEPTH>::getCurrentData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		return (DATA_TYPE)0;
	}
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipList<DATA_TYPE, BINARY_DEPTH>::getNextData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		return (DATA_TYPE)0;
	}
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::removeCurrentData()
{
	if (mCurrentOperatingp)
	{
		removeData(mCurrentOperatingp->mData);
	}
}
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipList<DATA_TYPE, BINARY_DEPTH>::getFirstData()
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
		return (DATA_TYPE)0;
	}
}
#endif
