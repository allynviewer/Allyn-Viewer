/** 
 * @file llinventoryobserver.h
 * @brief LLInventoryObserver class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLINVENTORYOBSERVERS_H
#define LL_LLINVENTORYOBSERVERS_H
#include "lluuid.h"
#include "llmd5.h"
#include <string>
#include <boost/unordered_map.hpp>
#include <vector>
class LLViewerInventoryCategory;
class LLInventoryObserver
{
public:
	enum
	{
		NONE = 0,
		LABEL = 1,
		INTERNAL = 2,
		ADD = 4,
		REMOVE = 8,
		STRUCTURE = 16,
		CALLING_CARD = 32,
		GESTURE 		= 64,
		REBUILD 		= 128,
		SORT 			= 256,
		CREATE			= 512,
		UPDATE_CREATE	= 1024,
		DESCRIPTION		= 2048,
		ALL 			= 0xffffffff
	};
	LLInventoryObserver();
	virtual ~LLInventoryObserver();
	virtual void changed(U32 mask) = 0;
};
class LLInventoryFetchObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchObserver(const LLUUID& id = LLUUID::null);
	LLInventoryFetchObserver(const uuid_vec_t& ids);
	void setFetchID(const LLUUID& id);
	void setFetchIDs(const uuid_vec_t& ids);
	BOOL isFinished() const;
	virtual void startFetch() = 0;
	virtual void changed(U32 mask) = 0;
	virtual void done() {};
protected:
	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
	uuid_vec_t mIDs;
};
class LLInventoryFetchItemsObserver : public LLInventoryFetchObserver
{
public:
	LLInventoryFetchItemsObserver(const LLUUID& item_id = LLUUID::null);
	LLInventoryFetchItemsObserver(const uuid_vec_t& item_ids);
	void startFetch();
	void changed(U32 mask);
private:
	LLTimer mFetchingPeriod;
	static const F32 FETCH_TIMER_EXPIRY;
};
class LLInventoryFetchDescendentsObserver : public LLInventoryFetchObserver
{
public:
	LLInventoryFetchDescendentsObserver(const LLUUID& cat_id = LLUUID::null);
	LLInventoryFetchDescendentsObserver(const uuid_vec_t& cat_ids);
	void startFetch();
	void changed(U32 mask);
protected:
	BOOL isCategoryComplete(const LLViewerInventoryCategory* cat) const;
};
class LLInventoryFetchComboObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchComboObserver(const uuid_vec_t& folder_ids,
								  const uuid_vec_t& item_ids);
	~LLInventoryFetchComboObserver();
	void changed(U32 mask);
	void startFetch();
	virtual void done() = 0;
protected:
	LLInventoryFetchItemsObserver *mFetchItems;
	LLInventoryFetchDescendentsObserver *mFetchDescendents;
};
class LLInventoryAddItemByAssetObserver : public LLInventoryObserver
{
public:
	LLInventoryAddItemByAssetObserver() : mIsDirty(false) {}
	virtual void changed(U32 mask);
	void watchAsset(const LLUUID& asset_id);
	bool isAssetWatched(const LLUUID& asset_id);
protected:
	virtual void onAssetAdded(const LLUUID& asset_id) {}
	virtual void done() = 0;
	typedef uuid_vec_t item_ref_t;
	item_ref_t mAddedItems;
	item_ref_t mWatchedAssets;
private:
	bool mIsDirty;
};
class LLInventoryAddedObserver : public LLInventoryObserver
{
public:
	LLInventoryAddedObserver() {}
	void changed(U32 mask);
protected:
	virtual void done() = 0;
};
class LLInventoryCategoryAddedObserver : public LLInventoryObserver
{
public:
	typedef std::vector<LLViewerInventoryCategory*>	cat_vec_t;
	LLInventoryCategoryAddedObserver() : mAddedCategories() {}
	void changed(U32 mask);
protected:
	virtual void done() = 0;
	cat_vec_t	mAddedCategories;
};
class LLInventoryCompletionObserver : public LLInventoryObserver
{
public:
	LLInventoryCompletionObserver() {}
	void changed(U32 mask);
	void watchItem(const LLUUID& id);
protected:
	virtual void done() = 0;
	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
};
class LLInventoryCategoriesObserver : public LLInventoryObserver
{
public:
	typedef std::function<void()> callback_t;
	LLInventoryCategoriesObserver() {};
	virtual void changed(U32 mask);
	bool addCategory(const LLUUID& cat_id, callback_t cb);
	void removeCategory(const LLUUID& cat_id);
protected:
	struct LLCategoryData
	{
		LLCategoryData(const LLUUID& cat_id, callback_t cb, S32 version, S32 num_descendents);
		callback_t	mCallback;
		S32			mVersion;
		S32			mDescendentsCount;
		LLMD5		mItemNameHash;
		bool		mIsNameHashInitialized;
		LLUUID		mCatID;
	};
	typedef	boost::unordered_map<LLUUID, LLCategoryData>	category_map_t;
	typedef category_map_t::value_type			category_map_value_t;
	category_map_t				mCategoryMap;
};
class LLFolderView;
class LLScrollOnRenameObserver: public LLInventoryObserver
{
public:
	LLFolderView *mView;
	LLUUID mUUID;
	LLScrollOnRenameObserver(const LLUUID& uuid, LLFolderView *view):
		mUUID(uuid),
		mView(view)
	{
	}
	void changed(U32 mask);
};
#endif
