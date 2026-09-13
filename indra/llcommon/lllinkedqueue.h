/** 
 * @file lllinkedqueue.h
 * @brief Declaration of linked queue classes.
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
#ifndef LL_LLLINKEDQUEUE_H
#define LL_LLLINKEDQUEUE_H
#include "llerror.h"
template <class DATA_TYPE> class LLLinkedQueueNode
{
public:
	DATA_TYPE			mData;
	LLLinkedQueueNode	*mNextp;
	LLLinkedQueueNode	*mPrevp;
public:
	LLLinkedQueueNode();
	LLLinkedQueueNode(const DATA_TYPE data);
	~LLLinkedQueueNode();
};
template <class DATA_TYPE> class LLLinkedQueue
{
public:
	LLLinkedQueue();
	~LLLinkedQueue();
	void push(const DATA_TYPE data);
	BOOL pop(DATA_TYPE &data);
	BOOL peek(DATA_TYPE &data);
	void reset();
	S32 getLength() const;
	BOOL isEmpty() const;
	BOOL remove(const DATA_TYPE data);
	BOOL checkData(const DATA_TYPE data) const;
private:
	void addNodeAtEnd(LLLinkedQueueNode<DATA_TYPE> *nodep);
private:
	LLLinkedQueueNode<DATA_TYPE> mHead;
	LLLinkedQueueNode<DATA_TYPE> mTail;
	S32 mLength;
};
template <class DATA_TYPE>
LLLinkedQueueNode<DATA_TYPE>::LLLinkedQueueNode() :
	mData(), mNextp(NULL), mPrevp(NULL)
{ }
template <class DATA_TYPE>
LLLinkedQueueNode<DATA_TYPE>::LLLinkedQueueNode(const DATA_TYPE data) :
	mData(data), mNextp(NULL), mPrevp(NULL)
{ }
template <class DATA_TYPE>
LLLinkedQueueNode<DATA_TYPE>::~LLLinkedQueueNode()
{ }
template <class DATA_TYPE>
LLLinkedQueue<DATA_TYPE>::LLLinkedQueue()
:	mHead(),
	mTail(),
	mLength(0)
{ }
template <class DATA_TYPE>
LLLinkedQueue<DATA_TYPE>::~LLLinkedQueue()
{
	reset();
}
template <class DATA_TYPE>
void LLLinkedQueue<DATA_TYPE>::push(const DATA_TYPE data)
{
	LLLinkedQueueNode<DATA_TYPE> *nodep = new LLLinkedQueueNode<DATA_TYPE>(data);
	addNodeAtEnd(nodep);
}
template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::remove(const DATA_TYPE data)
{
	BOOL b_found = FALSE;
	LLLinkedQueueNode<DATA_TYPE> *currentp = mHead.mNextp;
	while (currentp)
	{
		if (currentp->mData == data)
		{
			b_found = TRUE;
			if (currentp->mNextp)
			{
				currentp->mNextp->mPrevp = currentp->mPrevp;
			}
			else
			{
				mTail.mPrevp = currentp->mPrevp;
			}
			if (currentp->mPrevp)
			{
				currentp->mPrevp->mNextp = currentp->mNextp;
			}
			else
			{
				mHead.mNextp = currentp->mNextp;
			}
			delete currentp;
			mLength--;
			break;
		}
		currentp = currentp->mNextp;
	}
	return b_found;
}
template <class DATA_TYPE>
void LLLinkedQueue<DATA_TYPE>::reset()
{
	LLLinkedQueueNode<DATA_TYPE> *currentp;
	LLLinkedQueueNode<DATA_TYPE> *nextp;
	currentp = mHead.mNextp;
	while (currentp)
	{
		nextp = currentp->mNextp;
		delete currentp;
		currentp = nextp;
	}
	mHead.mNextp = NULL;
	mTail.mPrevp = NULL;
	mLength = 0;
}
template <class DATA_TYPE>
S32 LLLinkedQueue<DATA_TYPE>::getLength() const
{
	return mLength;
}
template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::isEmpty() const
{
	return mLength <= 0;
}
template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::checkData(const DATA_TYPE data) const
{
	LLLinkedQueueNode<DATA_TYPE> *currentp = mHead.mNextp;
	while (currentp)
	{
		if (currentp->mData == data)
		{
			return TRUE;
		}
		currentp = currentp->mNextp;
	}
	return FALSE;
}
template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::pop(DATA_TYPE &data)
{
	LLLinkedQueueNode<DATA_TYPE> *currentp;
	currentp = mHead.mNextp;
	if (!currentp)
	{
		return FALSE;
	}
	mHead.mNextp = currentp->mNextp;
	if (currentp->mNextp)
	{
		currentp->mNextp->mPrevp = currentp->mPrevp;
	}
	else
	{
		mTail.mPrevp = currentp->mPrevp;
	}
	data = currentp->mData;
	delete currentp;
	mLength--;
	return TRUE;
}
template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::peek(DATA_TYPE &data)
{
	LLLinkedQueueNode<DATA_TYPE> *currentp;
	currentp = mHead.mNextp;
	if (!currentp)
	{
		return FALSE;
	}
	data = currentp->mData;
	return TRUE;
}
template <class DATA_TYPE>
void LLLinkedQueue<DATA_TYPE>::addNodeAtEnd(LLLinkedQueueNode<DATA_TYPE> *nodep)
{
	nodep->mNextp = NULL;
	nodep->mPrevp = mTail.mPrevp;
	mTail.mPrevp = nodep;
	if (nodep->mPrevp)
	{
		nodep->mPrevp->mNextp = nodep;
	}
	else
	{
		mHead.mNextp = nodep;
	}
	mLength++;
}
#endif
