/** 
 * @file llfavoritesbar.h
 * @brief LLFavoritesBarCtrl base class
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#ifndef LL_LLFAVORITESBARCTRL_H
#define LL_LLFAVORITESBARCTRL_H
#include "llbutton.h"
#include "lluictrl.h"
#include "lltextbox.h"
#include "llinventoryobserver.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llui.h"
class LLMenuItemCallGL;
class LLMenuGL;
class LLFavoritesBarCtrl : public LLUICtrl, public LLInventoryObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIImage*> image_drag_indication;
		Optional<LLTextBox::Params> more_button;
		Optional<LLTextBox::Params> label;
		Params();
	};
protected:
	LLFavoritesBarCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLFavoritesBarCtrl();
	BOOL postBuild() override;
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg) override;
	BOOL	handleHover(S32 x, S32 y, MASK mask) override;
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	void changed(U32 mask) override;
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE) override;
	void draw() override;
	void showDragMarker(BOOL show) { mShowDragMarker = show; }
	void setLandingTab(LLUICtrl* tab) { mLandingTab = tab; }
protected:
	void updateButtons();
	LLButton* createButton(const LLPointer<LLViewerInventoryItem> item, const LLButton::Params& button_params, S32 x_offset );
	const LLButton::Params& getButtonParams();
	BOOL collectFavoriteItems(LLInventoryModel::item_array_t &items);
	void onButtonClick(LLUUID id);
	void onButtonRightClick(LLUUID id,LLView* button,S32 x,S32 y,MASK mask);
	void onButtonMouseDown(LLUUID id, LLUICtrl* button, S32 x, S32 y, MASK mask);
	void onOverflowMenuItemMouseDown(LLUUID id, LLUICtrl* item, S32 x, S32 y, MASK mask);
	void onButtonMouseUp(LLUUID id, LLUICtrl* button, S32 x, S32 y, MASK mask);
	void onEndDrag();
	bool enableSelected(const LLSD& userdata);
	void doToSelected(const LLSD& userdata);
	BOOL isClipboardPasteable() const;
	void pasteFromClipboard() const;
	void showDropDownMenu();
	LLHandle<LLView> mOverflowMenuHandle;
	LLHandle<LLView> mContextMenuHandle;
	LLUUID mFavoriteFolderId;
	const LLFontGL *mFont;
	size_t mFirstDropDownItem;
	U32 mDropDownItemsCount;
	bool mUpdateDropDownItems;
	bool mRestoreOverflowMenu;
	LLUUID mSelectedItemID;
	LLUIImage* mImageDragIndication;
private:
	void handleExistingFavoriteDragAndDrop(S32 x, S32 y);
	void handleNewFavoriteDragAndDrop(LLInventoryItem *item, const LLUUID& favorites_id, S32 x, S32 y);
	LLUICtrl* findChildByLocalCoords(S32 x, S32 y);
	BOOL needToSaveItemsOrder(const LLInventoryModel::item_array_t& items);
	void insertItem(LLInventoryModel::item_array_t& items, const LLUUID& dest_item_id, LLViewerInventoryItem* insertedItem, bool insert_before);
	LLInventoryModel::item_array_t::iterator findItemByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id);
	void createOverflowMenu();
	void updateMenuItems(LLMenuGL* menu);
	void fitLabelWidth(LLMenuItemCallGL* menu_item);
	void addOpenLandmarksMenuItem(LLMenuGL* menu);
	void positionAndShowMenu(LLMenuGL* menu);
	BOOL mShowDragMarker;
	LLUICtrl* mLandingTab;
	LLUICtrl* mLastTab;
	LLTextBox* mMoreTextBox;
	LLTextBox* mBarLabel;
	LLUUID mDragItemId;
	BOOL mStartDrag;
	LLInventoryModel::item_array_t mItems;
	BOOL mTabsHighlightEnabled;
	boost::signals2::connection mEndDragConnection;
};
class LLFavoritesOrderStorage : public LLSingleton<LLFavoritesOrderStorage>
	, public LLDestroyClass<LLFavoritesOrderStorage>
{
	friend class LLSingleton<LLFavoritesOrderStorage>;
	LLFavoritesOrderStorage();
	~LLFavoritesOrderStorage() { save(); }
	LOG_CLASS(LLFavoritesOrderStorage);
public:
	void setSortIndex(const LLViewerInventoryItem* inv_item, S32 sort_index);
	S32 getSortIndex(const LLUUID& inv_item_id);
	void removeSortIndex(const LLUUID& inv_item_id);
	void getSLURL(const LLUUID& asset_id);
	void saveItemsOrder(const LLInventoryModel::item_array_t& items);
	void saveOrder();
	void rearrangeFavoriteLandmarks(const LLUUID& source_item_id, const LLUUID& target_item_id);
	static void destroyClass();
	const static S32 NO_INDEX;
private:
	void cleanup();
	const static std::string SORTING_DATA_FILE_NAME;
    std::string getSavedOrderFileName();
    static std::string getStoredFavoritesFilename();
	void load();
	void save();
	void saveFavoritesSLURLs();
	void removeFavoritesRecordOfUser();
	void onLandmarkLoaded(const LLUUID& asset_id, class LLLandmark* landmark);
	void storeFavoriteSLURL(const LLUUID& asset_id, std::string& slurl);
	typedef std::map<LLUUID, S32> sort_index_map_t;
	sort_index_map_t mSortIndexes;
	typedef std::map<LLUUID, std::string> slurls_map_t;
	slurls_map_t mSLURLs;
	uuid_set_t mMissingSLURLs;
	bool mIsDirty;
	struct IsNotInFavorites
	{
		IsNotInFavorites(const LLInventoryModel::item_array_t& items)
			: mFavoriteItems(items)
		{
		}
		bool operator()(const sort_index_map_t::value_type& id_index_pair) const
		{
			LLPointer<LLViewerInventoryItem> item = gInventory.getItem(id_index_pair.first);
			if (item.isNull()) return true;
			LLInventoryModel::item_array_t::const_iterator found_it =
				std::find(mFavoriteItems.begin(), mFavoriteItems.end(), item);
			return found_it == mFavoriteItems.end();
		}
	private:
		LLInventoryModel::item_array_t mFavoriteItems;
	};
};
inline
LLFavoritesOrderStorage::LLFavoritesOrderStorage() :
	mIsDirty(false)
{ load(); }
#endif
