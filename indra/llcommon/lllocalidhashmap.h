/** 
 * @file lllocalidhashmap.h
 * @brief Map specialized for dealing with local ids
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
#ifndef LL_LLLOCALIDHASHMAP_H
#define LL_LLLOCALIDHASHMAP_H
#include "stdtypes.h"
#include "llerror.h"
const S32 MAX_ITERS = 4;
template <class DATA, int SIZE>
class LLLocalIDHashNode
{
public:
	LLLocalIDHashNode();
public:
	S32 mCount;
	U32	mKey[SIZE];
	DATA mData[SIZE];
	LLLocalIDHashNode<DATA, SIZE> *mNextNodep;
};
template <class DATA, int SIZE>
LLLocalIDHashNode<DATA, SIZE>::LLLocalIDHashNode()
{
	mCount = 0;
	mNextNodep = NULL;
}
template <class DATA_TYPE, int SIZE>
class LLLocalIDHashMap;
template <class DATA_TYPE, int SIZE>
class LLLocalIDHashMapIter
{
public:
	LLLocalIDHashMapIter(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp);
	~LLLocalIDHashMapIter();
	void setMap(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp);
	inline void first();
	inline void next();
	inline DATA_TYPE& current();
	inline BOOL done() const;
	inline S32  currentBin() const;
	inline void setBin(S32 bin);
	DATA_TYPE& operator*() const
	{
		return mCurHashNodep->mData[mCurHashNodeKey];
	}
	DATA_TYPE* operator->() const
	{
		return &(operator*());
	}
	LLLocalIDHashMap<DATA_TYPE, SIZE> *mHashMapp;
	LLLocalIDHashNode<DATA_TYPE, SIZE> *mCurHashNodep;
	S32 mCurHashMapNodeNum;
	S32 mCurHashNodeKey;
	DATA_TYPE mNull;
	S32 mIterID;
};
template <class DATA_TYPE, int SIZE>
class LLLocalIDHashMap
{
public:
	friend class LLLocalIDHashMapIter<DATA_TYPE, SIZE>;
	LLLocalIDHashMap();
	LLLocalIDHashMap(const DATA_TYPE &null_data);
	~LLLocalIDHashMap();
	inline DATA_TYPE &get(const U32 local_id);
	inline BOOL check(const U32 local_id) const;
	inline DATA_TYPE &set(const U32 local_id, const DATA_TYPE data);
	inline BOOL remove(const U32 local_id);
	void removeAll();
	void setNull(const DATA_TYPE data) { mNull = data; }
	inline S32 getLength() const;
	void dumpIter();
	void dumpBin(U32 bin);
protected:
	void addIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter);
	void removeIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter);
	BOOL removeWithShift(const U32 local_id);
protected:
	LLLocalIDHashNode<DATA_TYPE, SIZE> mNodes[256];
	S32 mIterCount;
	LLLocalIDHashMapIter<DATA_TYPE, SIZE> *mIters[MAX_ITERS];
	DATA_TYPE mNull;
};
template <class DATA_TYPE, int SIZE>
LLLocalIDHashMap<DATA_TYPE, SIZE>::LLLocalIDHashMap()
:	mIterCount(0),
	mNull()
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		mIters[i] = NULL;
	}
}
template <class DATA_TYPE, int SIZE>
LLLocalIDHashMap<DATA_TYPE, SIZE>::LLLocalIDHashMap(const DATA_TYPE &null_data)
:	mIterCount(0),
	mNull(null_data)
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		mIters[i] = NULL;
	}
}
template <class DATA_TYPE, int SIZE>
LLLocalIDHashMap<DATA_TYPE, SIZE>::~LLLocalIDHashMap()
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i])
		{
			mIters[i]->mHashMapp = NULL;
			mIterCount--;
		}
	}
	removeAll();
}
template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::removeAll()
{
	S32 bin;
	for (bin = 0; bin < 256; bin++)
	{
		LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[bin];
		BOOL first = TRUE;
		do
		{
			S32 i;
			const S32 count = nodep->mCount;
			for (i = 0; i < count; i++)
			{
				nodep->mData[i] = mNull;
			}
			nodep->mCount = 0;
			LLLocalIDHashNode<DATA_TYPE, SIZE>* curp = nodep;
			nodep = nodep->mNextNodep;
			if (first)
			{
				first = FALSE;
				curp->mNextNodep = NULL;
			}
			else
			{
				delete curp;
			}
		} while (nodep);
	}
}
template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::dumpIter()
{
	std::cout << "Hash map with " << mIterCount << " iterators" << std::endl;
	std::cout << "Hash Map Iterators:" << std::endl;
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i])
		{
			LL_INFOS() << i << " " << mIters[i]->mCurHashNodep << " " << mIters[i]->mCurHashNodeKey << LL_ENDL;
		}
		else
		{
			LL_INFOS() << i << "null" << LL_ENDL;
		}
	}
}
template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::dumpBin(U32 bin)
{
	std::cout << "Dump bin " << bin << std::endl;
	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[bin];
	S32 node = 0;
	do
	{
		std::cout << "Bin " << bin
			<< " node " << node
			<< " count " << nodep->mCount
			<< " contains " << std::flush;
		S32 i;
		for (i = 0; i < nodep->mCount; i++)
		{
			std::cout << nodep->mData[i] << " " << std::flush;
		}
		std::cout << std::endl;
		nodep = nodep->mNextNodep;
		node++;
	} while (nodep);
}
template <class DATA_TYPE, int SIZE>
inline S32 LLLocalIDHashMap<DATA_TYPE, SIZE>::getLength() const
{
	S32 count = 0;
	S32 bin;
	for (bin = 0; bin < 256; bin++)
	{
		const LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[bin];
		while (nodep)
		{
			count += nodep->mCount;
			nodep = nodep->mNextNodep;
		}
	}
	return count;
}
template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLLocalIDHashMap<DATA_TYPE, SIZE>::get(const U32 local_id)
{
	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[local_id & 0xff];
	do
	{
		S32 i;
		const S32 count = nodep->mCount;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				return nodep->mData[i];
			}
		}
		nodep = nodep->mNextNodep;
	} while (nodep);
	return mNull;
}
template <class DATA_TYPE, int SIZE>
inline BOOL LLLocalIDHashMap<DATA_TYPE, SIZE>::check(const U32 local_id) const
{
	const LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[local_id & 0xff];
	do
	{
		S32 i;
		const S32 count = nodep->mCount;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				return TRUE;
			}
		}
		nodep = nodep->mNextNodep;
	} while (nodep);
	return FALSE;
}
template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLLocalIDHashMap<DATA_TYPE, SIZE>::set(const U32 local_id, const DATA_TYPE data)
{
	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[local_id & 0xff];
	while (1)
	{
		const S32 count = nodep->mCount;
		S32 i;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				nodep->mData[i] = data;
				return nodep->mData[i];
			}
		}
		if (!nodep->mNextNodep)
		{
			if (i < SIZE)
			{
				nodep->mKey[i] = local_id;
				nodep->mData[i] = data;
				nodep->mCount++;
				return nodep->mData[i];
			}
			else
			{
				nodep->mNextNodep = new LLLocalIDHashNode<DATA_TYPE, SIZE>;
				nodep->mNextNodep->mKey[0] = local_id;
				nodep->mNextNodep->mData[0] = data;
				nodep->mNextNodep->mCount = 1;
				return nodep->mNextNodep->mData[0];
			}
		}
		nodep = nodep->mNextNodep;
	}
}
template <class DATA_TYPE, int SIZE>
inline BOOL LLLocalIDHashMap<DATA_TYPE, SIZE>::remove(const U32 local_id)
{
	const S32 node_index = local_id & 0xff;
	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[node_index];
	do
	{
		const S32 count = nodep->mCount;
		S32 i;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				BOOL need_shift = FALSE;
				S32 cur_iter;
				if (mIterCount)
				{
					for (cur_iter = 0; cur_iter < MAX_ITERS; cur_iter++)
					{
						if (mIters[cur_iter])
						{
							if (mIters[cur_iter]->mCurHashMapNodeNum == node_index)
							{
								if ((mIters[cur_iter]->mCurHashNodep == nodep)
									&& (mIters[cur_iter]->mCurHashNodeKey == i))
								{
								}
								else
								{
									need_shift = TRUE;
								}
							}
						}
					}
				}
				if (need_shift)
				{
					return removeWithShift(local_id);
				}
				for (cur_iter = 0; cur_iter < MAX_ITERS; cur_iter++)
				{
					if (mIters[cur_iter])
					{
						if (mIters[cur_iter]->mCurHashMapNodeNum == node_index)
						{
							if ((mIters[cur_iter]->mCurHashNodep == nodep)
								&& (mIters[cur_iter]->mCurHashNodeKey == i))
							{
								if (nodep->mCount > 1)
								{
									mIters[cur_iter]->mCurHashNodeKey--;
								}
								else
								{
									mIters[cur_iter]->next();
									mIters[cur_iter]->mCurHashNodeKey--;
								}
							}
						}
					}
				}
				LLLocalIDHashNode<DATA_TYPE, SIZE> *prevp = &mNodes[node_index];
				LLLocalIDHashNode<DATA_TYPE, SIZE> *lastp = prevp;
				while (lastp->mNextNodep)
				{
					prevp = lastp;
					lastp = lastp->mNextNodep;
				}
				nodep->mKey[i] = lastp->mKey[lastp->mCount - 1];
				nodep->mData[i] = lastp->mData[lastp->mCount - 1];
				lastp->mCount--;
				lastp->mData[lastp->mCount] = mNull;
				if (!lastp->mCount)
				{
					if (lastp != &mNodes[local_id & 0xff])
					{
						prevp->mNextNodep = NULL;
						delete lastp;
					}
				}
				return TRUE;
			}
		}
		nodep = nodep->mNextNodep;
	} while (nodep);
	return FALSE;
}
template <class DATA_TYPE, int SIZE>
BOOL LLLocalIDHashMap<DATA_TYPE, SIZE>::removeWithShift(const U32 local_id)
{
	const S32 node_index = local_id & 0xFF;
	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[node_index];
	LLLocalIDHashNode<DATA_TYPE, SIZE>* prevp = NULL;
	BOOL found = FALSE;
	do
	{
		const S32 count = nodep->mCount;
		S32 i;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				found = TRUE;
			}
			if (found)
			{
				S32 cur_iter;
				for (cur_iter = 0; cur_iter <MAX_ITERS; cur_iter++)
				{
					LLLocalIDHashMapIter<DATA_TYPE, SIZE>* iter;
					iter = mIters[cur_iter];
					if (iter
						&& iter->mCurHashMapNodeNum == node_index
						&& iter->mCurHashNodep == nodep
						&& iter->mCurHashNodeKey == i)
					{
						if (i > 0)
						{
							iter->mCurHashNodeKey--;
						}
						else if (prevp)
						{
							iter->mCurHashNodep = prevp;
							iter->mCurHashNodeKey = prevp->mCount - 1;
						}
						else
						{
							iter->mCurHashNodeKey = -1;
						}
					}
				}
				if (i < count-1)
				{
					nodep->mKey[i] = nodep->mKey[i+1];
					nodep->mData[i] = nodep->mData[i+1];
				}
				else if (nodep->mNextNodep)
				{
					nodep->mKey[i] = nodep->mNextNodep->mKey[0];
					nodep->mData[i] = nodep->mNextNodep->mData[0];
				}
				else
				{
					nodep->mKey[i] = 0;
					nodep->mData[i] = mNull;
				}
			}
		}
		if (found
			&& !nodep->mNextNodep)
		{
			nodep->mCount--;
			if (nodep->mCount == 0)
			{
				if (nodep != &mNodes[node_index])
				{
					llassert(prevp);
					prevp->mNextNodep = NULL;
					delete nodep;
					nodep = NULL;
				}
			}
			return found;
		}
		prevp = nodep;
		nodep = nodep->mNextNodep;
	} while (nodep);
	return found;
}
template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::removeIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter)
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i] == iter)
		{
			mIters[i] = NULL;
			mIterCount--;
			return;
		}
	}
	LL_ERRS() << "Iterator " << iter << " not found for removal in hash map!" << LL_ENDL;
}
template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::addIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter)
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i] == NULL)
		{
			mIters[i] = iter;
			mIterCount++;
			return;
		}
	}
	LL_ERRS() << "More than " << MAX_ITERS << " iterating over a map simultaneously!" << LL_ENDL;
}
template <class DATA_TYPE, int SIZE>
LLLocalIDHashMapIter<DATA_TYPE, SIZE>::LLLocalIDHashMapIter(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp)
{
	mHashMapp = NULL;
	setMap(hash_mapp);
}
template <class DATA_TYPE, int SIZE>
LLLocalIDHashMapIter<DATA_TYPE, SIZE>::~LLLocalIDHashMapIter()
{
	if (mHashMapp)
	{
		mHashMapp->removeIter(this);
	}
}
template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::setMap(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp)
{
	if (mHashMapp)
	{
		mHashMapp->removeIter(this);
	}
	mHashMapp = hash_mapp;
	if (mHashMapp)
	{
		mHashMapp->addIter(this);
	}
	mCurHashNodep = NULL;
	mCurHashMapNodeNum = -1;
	mCurHashNodeKey = 0;
}
template <class DATA_TYPE, int SIZE>
inline void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::first()
{
	S32 i;
	for (i = 0; i < 256; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{
			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			return;
		}
	}
	mCurHashNodep = NULL;
	return;
}
template <class DATA_TYPE, int SIZE>
inline BOOL LLLocalIDHashMapIter<DATA_TYPE, SIZE>::done() const
{
	return mCurHashNodep ? FALSE : TRUE;
}
template <class DATA_TYPE, int SIZE>
inline S32 LLLocalIDHashMapIter<DATA_TYPE, SIZE>::currentBin() const
{
	if (  (mCurHashMapNodeNum > 255)
		||(mCurHashMapNodeNum < 0))
	{
		return 0;
	}
	else
	{
		return mCurHashMapNodeNum;
	}
}
template <class DATA_TYPE, int SIZE>
inline void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::setBin(S32 bin)
{
	S32 i;
	bin = llclamp(bin, 0, 255);
	for (i = bin; i < 256; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{
			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			return;
		}
	}
	for (i = 0; i < bin; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{
			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			return;
		}
	}
	mCurHashNodep = NULL;
}
template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLLocalIDHashMapIter<DATA_TYPE, SIZE>::current()
{
	if (!mCurHashNodep)
	{
		return mNull;
	}
	return mCurHashNodep->mData[mCurHashNodeKey];
}
template <class DATA_TYPE, int SIZE>
inline void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::next()
{
	if (!mCurHashNodep)
	{
		return;
	}
	mCurHashNodeKey++;
	if (mCurHashNodeKey < mCurHashNodep->mCount)
	{
		return;
	}
	mCurHashNodep = mCurHashNodep->mNextNodep;
	if (mCurHashNodep)
	{
		mCurHashNodeKey = 0;
		return;
	}
	mCurHashMapNodeNum++;
	S32 i;
	for (i = mCurHashMapNodeNum; i < 256; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{
			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			return;
		}
	}
	mCurHashNodep = NULL;
	mHashMapp->mIterCount--;
	return;
}
#endif
