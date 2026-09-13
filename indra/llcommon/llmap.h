/** 
 * @file llmap.h
 * @brief LLMap class header file
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
#ifndef LL_LLMAP_H
#define LL_LLMAP_H
template<class INDEX_TYPE, class MAPPED_TYPE> class LLMap
{
private:
	typedef typename std::map<INDEX_TYPE, MAPPED_TYPE> stl_map_t;
	typedef typename stl_map_t::iterator stl_iter_t;
	typedef typename stl_map_t::value_type stl_value_t;
	stl_map_t mStlMap;
	stl_iter_t mCurIter;
	MAPPED_TYPE dummy_data;
	INDEX_TYPE dummy_index;
public:
	LLMap() : mStlMap()
	{
		memset((void*)(&dummy_data),  0x0, sizeof(MAPPED_TYPE));
		memset((void*)(&dummy_index), 0x0, sizeof(INDEX_TYPE));
		mCurIter = mStlMap.begin();
	}
	~LLMap()
	{
		mStlMap.clear();
	}
	void resetMap()
	{
		mCurIter = mStlMap.begin();
	}
	MAPPED_TYPE &getNextData()
	{
		if (mCurIter == mStlMap.end())
		{
			return dummy_data;
		}
		else
		{
			return (*mCurIter++).second;
		}
	}
	const INDEX_TYPE &getNextKey()
	{
		if (mCurIter == mStlMap.end())
		{
			return dummy_index;
		}
		else
		{
			return (*mCurIter++).first;
		}
	}
	MAPPED_TYPE &getFirstData()
	{
		resetMap();
		return getNextData();
	}
	const INDEX_TYPE &getFirstKey()
	{
		resetMap();
		return getNextKey();
	}
	S32 getLength()
	{
		return mStlMap.size();
	}
	void addData(const INDEX_TYPE &index, MAPPED_TYPE pointed_to)
	{
		mStlMap.insert(stl_value_t(index, pointed_to));
	}
	void addData(const INDEX_TYPE &index)
	{
		mStlMap.insert(stl_value_t(index, dummy_data));
	}
	MAPPED_TYPE &getData(const INDEX_TYPE &index)
	{
		std::pair<stl_iter_t, bool> res;
		res = mStlMap.insert(stl_value_t(index, dummy_data));
		return res.first->second;
	}
	MAPPED_TYPE &getData(const INDEX_TYPE &index, BOOL &b_new_entry)
	{
		std::pair<stl_iter_t, bool> res;
		res = mStlMap.insert(stl_value_t(index, dummy_data));
		b_new_entry = res.second;
		return res.first->second;
	}
	MAPPED_TYPE getIfThere(const INDEX_TYPE &index)
	{
		stl_iter_t iter;
		iter = mStlMap.find(index);
		if (iter == mStlMap.end())
		{
			return (MAPPED_TYPE)0;
		}
		else
		{
			return (*iter).second;
		}
	}
	MAPPED_TYPE &operator[](const INDEX_TYPE &index)
	{
		return getData(index);
	}
	INDEX_TYPE reverseLookup(const MAPPED_TYPE data)
	{
		stl_iter_t iter;
		stl_iter_t end_iter;
		iter = mStlMap.begin();
		end_iter = mStlMap.end();
		while (iter != end_iter)
		{
			if ((*iter).second == data)
				return (*iter).first;
			iter++;
		}
		return (INDEX_TYPE)0;
	}
	BOOL removeData(const INDEX_TYPE &index)
	{
		mCurIter = mStlMap.find(index);
		if (mCurIter == mStlMap.end())
		{
			return FALSE;
		}
		else
		{
			stl_iter_t iter = mCurIter++;
			mStlMap.erase(iter);
			return TRUE;
		}
	}
	BOOL checkData(const INDEX_TYPE &index)
	{
		stl_iter_t iter;
		iter = mStlMap.find(index);
		if (iter == mStlMap.end())
		{
			return FALSE;
		}
		else
		{
			mCurIter = iter;
			return TRUE;
		}
	}
	BOOL deleteData(const INDEX_TYPE &index)
	{
		mCurIter = mStlMap.find(index);
		if (mCurIter == mStlMap.end())
		{
			return FALSE;
		}
		else
		{
			stl_iter_t iter = mCurIter++;
			delete (*iter).second;
			mStlMap.erase(iter);
			return TRUE;
		}
	}
	void deleteAllData()
	{
		stl_iter_t iter;
		stl_iter_t end_iter;
		iter = mStlMap.begin();
		end_iter = mStlMap.end();
		while (iter != end_iter)
		{
			delete (*iter).second;
			iter++;
		}
		mStlMap.clear();
		mCurIter = mStlMap.end();
	}
	void removeAllData()
	{
		mStlMap.clear();
	}
};
#endif
