/** 
 * @file llassoclist.h
 * @brief LLAssocList class header file
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
#ifndef LL_LLASSOCLIST_H
#define LL_LLASSOCLIST_H
#include <iostream>
template<class INDEX_TYPE, class VALUE_TYPE>
class LLAssocList
{
private:
	class Node
	{
	public:
		Node(const INDEX_TYPE &index, const VALUE_TYPE &value, Node *next)
		{
			mIndex = index;
			mValue = value;
			mNext = next;
		}
		~Node() { }
		INDEX_TYPE	mIndex;
		VALUE_TYPE	mValue;
		Node		*mNext;
	};
	Node *mHead;
public:
	LLAssocList()
	{
		mHead = NULL;
	}
	~LLAssocList()
	{
		removeAll();
	}
	BOOL isEmpty()
	{
		return (mHead == NULL);
	}
	U32 length()
	{
		U32 count = 0;
		for (	Node *node = mHead;
				node;
				node = node->mNext )
		{
			count++;
		}
		return count;
	}
	BOOL remove( const INDEX_TYPE &index )
	{
		if (!mHead)
			return FALSE;
		if (mHead->mIndex == index)
		{
			Node *node = mHead;
			mHead = mHead->mNext;
			delete node;
			return TRUE;
		}
		for (	Node *prev = mHead;
				prev->mNext;
				prev = prev->mNext )
		{
			if (prev->mNext->mIndex == index)
			{
				Node *node = prev->mNext;
				prev->mNext = prev->mNext->mNext;
				delete node;
				return TRUE;
			}
		}
		return FALSE;
	}
	void removeAll()
	{
		while ( mHead )
		{
			Node *node = mHead;
			mHead = mHead->mNext;
			delete node;
		}
	}
	void addToHead( const INDEX_TYPE &index, const VALUE_TYPE &value )
	{
		remove(index);
		Node *node = new Node(index, value, mHead);
		mHead = node;
	}
	void addToTail( const INDEX_TYPE &index, const VALUE_TYPE &value )
	{
		remove(index);
		Node *node = new Node(index, value, NULL);
		if (!mHead)
		{
			mHead = node;
			return;
		}
		for (	Node *prev=mHead;
				prev;
				prev=prev->mNext )
		{
			if (!prev->mNext)
			{
				prev->mNext=node;
				return;
			}
		}
	}
	BOOL setValue( const INDEX_TYPE &index, const VALUE_TYPE &value, BOOL addIfNotFound=FALSE )
	{
		VALUE_TYPE *valueP = getValue(index);
		if (valueP)
		{
			*valueP = value;
			return TRUE;
		}
		if (!addIfNotFound)
			return FALSE;
		addToTail(index, value);
		return TRUE;
	}
	BOOL setValueAt( U32 i, const VALUE_TYPE &value )
	{
		VALUE_TYPE *valueP = getValueAt(i);
		if (valueP)
		{
			*valueP = value;
			return TRUE;
		}
		return FALSE;
	}
	VALUE_TYPE *getValue( const INDEX_TYPE &index )
	{
		for (	Node *node = mHead;
				node;
				node = node->mNext )
		{
			if (node->mIndex == index)
				return &node->mValue;
		}
		return NULL;
	}
	VALUE_TYPE *getValueAt( U32 i )
	{
		U32 count = 0;
		for (	Node *node = mHead;
				node;
				node = node->mNext )
		{
			if (count == i)
				return &node->mValue;
			count++;
		}
		return NULL;
	}
	INDEX_TYPE *getIndex( const INDEX_TYPE &index )
	{
		for (	Node *node = mHead;
				node;
				node = node->mNext )
		{
			if (node->mIndex == index)
				return &node->mIndex;
		}
		return NULL;
	}
	INDEX_TYPE *getIndexAt( U32 i )
	{
		U32 count = 0;
		for (	Node *node = mHead;
				node;
				node = node->mNext )
		{
			if (count == i)
				return &node->mIndex;
			count++;
		}
		return NULL;
	}
	VALUE_TYPE *operator[](const INDEX_TYPE &index)
	{
		return getValue(index);
	}
	VALUE_TYPE *operator[](U32 i)
	{
		return getValueAt(i);
	}
	friend std::ostream &operator<<( std::ostream &os, LLAssocList &map )
	{
		os << "{";
		for (	Node *node = map.mHead;
				node;
				node = node->mNext )
		{
			os << "<" << node->mIndex << ", " << node->mValue << ">";
			if (node->mNext)
				os << ", ";
		}
		os << "}";
		return os;
	}
};
#endif
