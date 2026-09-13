/** 
 * @file llgesturemgr.h
 * @brief Manager for playing gestures on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#ifndef LL_LLGESTUREMGR_H
#define LL_LLGESTUREMGR_H
#include <string>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <vector>
#include "llassetstorage.h"
#include "llinventoryobserver.h"
#include "llsingleton.h"
#include "llviewerinventory.h"
class LLMultiGesture;
class LLGestureListener;
class LLGestureStep;
class LLUUID;
class LLVFS;
class LLGestureManagerObserver
{
public:
	virtual ~LLGestureManagerObserver() { };
	virtual void changed() = 0;
};
class LLGestureMgr : public LLSingleton<LLGestureMgr>, public LLInventoryFetchItemsObserver
{
public:
	typedef std::function<void (LLMultiGesture* loaded_gesture)> gesture_loaded_callback_t;
	typedef boost::unordered_map<LLUUID, LLMultiGesture*> item_map_t;
	typedef boost::unordered_map<LLUUID, gesture_loaded_callback_t> callback_map_t;
	LLGestureMgr();
	~LLGestureMgr();
	void init();
	void update();
	void activateGesture(const LLUUID& item_id);
	void activateGestures(LLViewerInventoryItem::item_array_t& items);
	void replaceGesture(const LLUUID& item_id, LLMultiGesture* new_gesture, const LLUUID& asset_id);
	void replaceGesture(const LLUUID& item_id, const LLUUID& asset_id);
	void activateGestureWithAsset(const LLUUID& item_id, const LLUUID& asset_id, bool inform_server, bool deactivate_similar);
	void deactivateGesture(const LLUUID& item_id);
	void deactivateSimilarGestures(const LLMultiGesture* gesture, const LLUUID& in_item_id);
	bool isGestureActive(const LLUUID& item_id) const;
	bool isGesturePlaying(const LLUUID& item_id) const;
	bool isGesturePlaying(const LLMultiGesture* gesture) const;
	const item_map_t& getActiveGestures() const { return mActive; }
	void playGesture(LLMultiGesture* gesture, bool local = false);
	void playGesture(const LLUUID& item_id, bool local = false);
	void stopGesture(LLMultiGesture* gesture);
	void stopGesture(const LLUUID& item_id);
	void setGestureLoadedCallback(const LLUUID& inv_item_id, gesture_loaded_callback_t cb)
	{
		mCallbackMap[inv_item_id] = cb;
	}
	bool triggerGesture(KEY key, MASK mask);
	bool triggerAndReviseString(const std::string &str, std::string *revised_string = nullptr);
	S32 getPlayingCount() const;
	void addObserver(LLGestureManagerObserver* observer);
	void removeObserver(LLGestureManagerObserver* observer);
	void notifyObservers();
	void changed(U32 mask);
	bool matchPrefix(const std::string& in_str, std::string* out_str) const;
	void getItemIDs(uuid_vec_t* ids) const;
protected:
	void stepGesture(LLMultiGesture* gesture);
	void runStep(LLMultiGesture* gesture, LLGestureStep* step);
	void done();
	static void onLoadComplete(LLVFS *vfs,
						   const LLUUID& asset_uuid,
						   LLAssetType::EType type,
						   void* user_data, S32 status, LLExtStat ext_status);
	static void onAssetLoadComplete(LLVFS *vfs,
									const LLUUID& asset_uuid,
									LLAssetType::EType type,
									void* user_data, S32 status, LLExtStat ext_status);
	static bool hasLoadingAssets(LLMultiGesture* gesture);
private:
	item_map_t mActive;
	S32 mLoadingCount;
	std::string mDeactivateSimilarNames;
	std::vector<LLGestureManagerObserver*> mObservers;
	callback_map_t mCallbackMap;
	std::vector<LLMultiGesture*> mPlaying;
	bool mValid;
	uuid_set_t mLoadingAssets;
};
#endif
