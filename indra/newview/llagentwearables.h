/** 
 * @file llagentwearables.h
 * @brief LLAgentWearables class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_LLAGENTWEARABLES_H
#define LL_LLAGENTWEARABLES_H
#include "llmemory.h"
#include "llui.h"
#include "lluuid.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llavatarappearancedefines.h"
#include "llwearabledata.h"
class LLInventoryItem;
class LLVOAvatarSelf;
class LLViewerWearable;
class LLViewerObject;
class LLAgentWearables : public LLInitClass<LLAgentWearables>, public LLWearableData
{
public:
	friend class LLInitialWearablesFetch;
	LLAgentWearables();
	virtual ~LLAgentWearables();
	void 			setAvatarObject(LLVOAvatarSelf *avatar);
	void			createStandardWearables();
	void			cleanup();
	void			dump();
	static void initClass();
public:
	BOOL			isWearingItem(const LLUUID& item_id) const;
	BOOL			isWearableModifiable(LLWearableType::EType type, U32 index ) const;
	BOOL			isWearableModifiable(const LLUUID& item_id) const;
	BOOL			isWearableCopyable(LLWearableType::EType type, U32 index ) const;
	BOOL			areWearablesLoaded() const;
	bool			areInitalWearablesLoaded() const { return mInitialWearablesLoaded; }
	bool			areInitialAttachmentsRequested() const { return mInitialAttachmentsRequested;  }
	bool			isCOFChangeInProgress() const { return mCOFChangeInProgress; }
	F32				getCOFChangeTime() const { return mCOFChangeTimer.getElapsedTimeF32(); }
	void			updateWearablesLoaded();
	void			checkWearablesLoaded() const;
	bool			canMoveWearable(const LLUUID& item_id, bool closer_to_body) const;
	bool			canWearableBeRemoved(const LLViewerWearable* wearable) const;
	void			animateAllWearableParams(F32 delta, bool upload_bake = false);
public:
	void				getWearableItemIDs(uuid_vec_t& idItems) const;
	void				getWearableItemIDs(LLWearableType::EType eType, uuid_vec_t& idItems) const;
	const LLUUID		getWearableItemID(LLWearableType::EType type, U32 index ) const;
	const LLUUID		getWearableAssetID(LLWearableType::EType type, U32 index ) const;
	const LLViewerWearable*	getWearableFromItemID(const LLUUID& item_id) const;
	LLViewerWearable*	getWearableFromItemID(const LLUUID& item_id);
	LLViewerWearable*	getWearableFromAssetID(const LLUUID& asset_id);
	LLViewerWearable*		getViewerWearable(const LLWearableType::EType type, U32 index );
	const LLViewerWearable*	getViewerWearable(const LLWearableType::EType type, U32 index ) const;
	LLInventoryItem*	getWearableInventoryItem(LLWearableType::EType type, U32 index );
	static BOOL			selfHasWearable(LLWearableType::EType type);
private:
	void	wearableUpdated(LLWearable *wearable, BOOL removed);
public:
	void			setWearableOutfit(const LLInventoryItem::item_array_t& items, const std::vector< LLViewerWearable* >& wearables);
	void			setWearableName(const LLUUID& item_id, const std::string& new_name);
	void			nameOrDescriptionChanged(LLUUID const& item_id);
	void			addLocalTextureObject(const LLWearableType::EType wearable_type, const LLAvatarAppearanceDefines::ETextureIndex texture_type, U32 wearable_index);
protected:
	void			addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												LLViewerWearable* wearable,
												const LLUUID& category_id = LLUUID::null,
												BOOL notify = TRUE);
	void 			addWearabletoAgentInventoryDone(const LLWearableType::EType type,
													const U32 index,
													const LLUUID& item_id,
													LLViewerWearable* wearable);
	void			recoverMissingWearable(const LLWearableType::EType type, U32 index );
	void			recoverMissingWearableDone();
public:
	static void		createWearable(LLWearableType::EType type, bool wear = false, const LLUUID& parent_id = LLUUID::null);
	static void		editWearable(const LLUUID& item_id);
	bool			moveWearable(const LLViewerInventoryItem* item, bool closer_to_body);
	void			requestEditingWearable(const LLUUID& item_id);
	void			editWearableIfRequested(const LLUUID& item_id);
private:
	LLUUID			mItemToEdit;
public:
private:
	void			removeWearable(const LLWearableType::EType type, bool do_remove_all , U32 index );
	void			removeWearableFinal(const LLWearableType::EType type, bool do_remove_all , U32 index );
protected:
	static bool		onRemoveWearableDialog(const LLSD& notification, const LLSD& response);
private:
	void			makeNewOutfitDone(S32 type, U32 index);
public:
	LLViewerWearable*			saveWearableAs(const LLWearableType::EType type, const U32 index, const std::string& new_name, const std::string& description, BOOL save_in_lost_and_found);
	void			saveWearable(const LLWearableType::EType type, const U32 index,
								 const std::string new_name = "");
	void			saveAllWearables();
	void			revertWearable(const LLWearableType::EType type, const U32 index);
	void			sendDummyAgentWearablesUpdate();
public:
	typedef std::vector<LLViewerObject*> llvo_vec_t;
	static void     findAttachmentsAddRemoveInfo(LLInventoryModel::item_array_t& obj_item_array,
												 llvo_vec_t& objects_to_remove,
												 llvo_vec_t& objects_to_retain,
												 LLInventoryModel::item_array_t& items_to_add);
	static void		userRemoveMultipleAttachments(llvo_vec_t& llvo_array);
	static void		userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array);
public:
	typedef boost::function<void()>			loading_started_callback_t;
	typedef boost::signals2::signal<void()>	loading_started_signal_t;
	boost::signals2::connection				addLoadingStartedCallback(loading_started_callback_t cb);
	typedef boost::function<void()>			loaded_callback_t;
	typedef boost::signals2::signal<void()>	loaded_signal_t;
	boost::signals2::connection				addLoadedCallback(loaded_callback_t cb);
	boost::signals2::connection				addInitialWearablesLoadedCallback(const loaded_callback_t& cb);
	bool									changeInProgress() const;
	void									notifyLoadingStarted();
	void									notifyLoadingFinished();
private:
	loading_started_signal_t				mLoadingStartedSignal;
	loaded_signal_t							mLoadedSignal;
	loaded_signal_t							mInitialWearablesLoadedSignal;
private:
	static BOOL		mInitialWearablesUpdateReceived;
	static bool		mInitialWearablesLoaded;
	static bool		mInitialAttachmentsRequested;
	BOOL			mWearablesLoaded;
	BOOL			mCOFChangeInProgress;
	LLTimer			mCOFChangeTimer;
private:
	class AddWearableToAgentInventoryCallback : public LLInventoryCallback
	{
	public:
		enum ETodo
		{
			CALL_NONE = 0,
			CALL_UPDATE = 1,
			CALL_RECOVERDONE = 2,
			CALL_CREATESTANDARDDONE = 4,
			CALL_MAKENEWOUTFITDONE = 8,
			CALL_WEARITEM = 16
		};
		AddWearableToAgentInventoryCallback(LLPointer<LLRefCount> cb,
											LLWearableType::EType type,
											U32 index,
											LLViewerWearable* wearable,
											U32 todo = CALL_NONE,
											const std::string description = "");
		virtual void fire(const LLUUID& inv_item);
	private:
		LLWearableType::EType mType;
		U32 mIndex;
		LLViewerWearable* mWearable;
		U32 mTodo;
		LLPointer<LLRefCount> mCB;
		std::string mDescription;
	};
};
extern LLAgentWearables gAgentWearables;
#endif
