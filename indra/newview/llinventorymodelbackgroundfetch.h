/** 
 * @file llinventorymodelbackgroundfetch.h
 * @brief LLInventoryModelBackgroundFetch class header file
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
#ifndef LL_LLINVENTORYMODELBACKGROUNDFETCH_H
#define LL_LLINVENTORYMODELBACKGROUNDFETCH_H
#include "llsingleton.h"
#include "lluuid.h"
#include "aicurlperservice.h"
class LLInventoryModelBackgroundFetch : public LLSingleton<LLInventoryModelBackgroundFetch>
{
public:
	LLInventoryModelBackgroundFetch();
	~LLInventoryModelBackgroundFetch();
	void start(const LLUUID& cat_id = LLUUID::null, BOOL recursive = TRUE);
	BOOL folderFetchActive() const;
	bool isEverythingFetched() const;
	bool libraryFetchStarted() const;
	bool libraryFetchCompleted() const;
	bool libraryFetchInProgress() const;
	bool inventoryFetchStarted() const;
	bool inventoryFetchCompleted() const;
	bool inventoryFetchInProgress() const;
    void findLostItems();
	void incrFetchCount(S32 fetching);
	bool isBulkFetchProcessingComplete() const;
	void setAllFoldersFetched();
	void addRequestAtFront(const LLUUID & id, BOOL recursive, bool is_category);
	void addRequestAtBack(const LLUUID & id, BOOL recursive, bool is_category);
protected:
	void bulkFetch();
	void backgroundFetch();
	static void backgroundFetchCB(void*);
	bool fetchQueueContainsNoDescendentsOf(const LLUUID& cat_id) const;
private:
 	BOOL mRecursiveInventoryFetchStarted;
	BOOL mRecursiveLibraryFetchStarted;
	BOOL mAllFoldersFetched;
	BOOL mBackgroundFetchActive;
	bool mFolderFetchActive;
	S32 mFetchCount;
	BOOL mTimelyFetchPending;
	S32  mNumFetchRetries;
	AIPerServicePtr mPerServicePtr;
	LLFrameTimer mFetchTimer;
	F32 mMinTimeBetweenFetches;
	F32 mMaxTimeBetweenFetches;
	struct FetchQueueInfo
	{
		FetchQueueInfo(const LLUUID& id, BOOL recursive, bool is_category = true)
			: mUUID(id),
			  mIsCategory(is_category),
			  mRecursive(recursive)
		{}
		LLUUID mUUID;
		bool mIsCategory;
		BOOL mRecursive;
	};
	typedef std::deque<FetchQueueInfo> fetch_queue_t;
	fetch_queue_t mFetchQueue;
};
#endif
