/** 
 * @file llflatlistview.h
 * @brief LLFlatListView base class and extension to support messages for several cases of an empty list.
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
#ifndef LL_LLFLATLISTVIEW_H
#define LL_LLFLATLISTVIEW_H
#include "llpanel.h"
#include "llscrollcontainer.h"
#include "lltextbox.h"
class LLFlatListView : public LLScrollContainer, public LLEditMenuHandler
{
	LOG_CLASS(LLFlatListView);
public:
	class ItemComparator
	{
	public:
		ItemComparator() {};
		virtual ~ItemComparator() {};
		virtual bool compare(const LLPanel* item1, const LLPanel* item2) const = 0;
	};
	class ItemReverseComparator : public ItemComparator
	{
	public:
		ItemReverseComparator(const ItemComparator& comparator) : mComparator(comparator) {};
		virtual ~ItemReverseComparator() {};
		bool compare(const LLPanel* item1, const LLPanel* item2) const override
		{
			return mComparator.compare(item2, item1);
		}
	private:
		const ItemComparator& mComparator;
	};
	struct Params : public LLInitParam::Block<Params, LLScrollContainer::Params>
	{
		Optional<bool> allow_select;
		Optional<bool> multi_select;
		Optional<bool> keep_one_selected;
		Optional<bool> keep_selection_visible_on_reshape;
		Optional<U32> item_pad;
		Optional<LLTextBox::Params> no_items_text;
		Params();
	};
	BOOL canFocusChildren() const override { return FALSE; }
	boost::signals2::connection setReturnCallback( const commit_signal_t::slot_type& cb ) { return mOnReturnSignal.connect(cb); }
	void reshape(S32 width, S32 height, BOOL called_from_parent  = TRUE) override;
	const LLRect& getItemsRect() const;
	LLRect getRequiredRect() override { return getItemsRect(); }
	const S32 getItemsPad() const { return mItemPad; }
	virtual bool addItem(LLPanel * item, const LLSD& value = LLUUID::null, EAddPosition pos = ADD_BOTTOM, bool rearrange = true);
	virtual bool insertItemAfter(LLPanel* after_item, LLPanel* item_to_add, const LLSD& value = LLUUID::null);
	virtual bool removeItem(LLPanel* item, bool rearrange = true);
	virtual bool removeItemByValue(const LLSD& value, bool rearrange = true);
	virtual bool removeItemByUUID(const LLUUID& uuid, bool rearrange = true);
	virtual LLPanel* getItemByValue(const LLSD& value) const;
	virtual bool valueExists(const LLSD& value) const;
	template<class T>
	T* getTypedItemByValue(const LLSD& value) const
	{
		return dynamic_cast<T*>(getItemByValue(value));
	}
	virtual bool selectItem(LLPanel* item, bool select = true);
	virtual bool selectItemByValue(const LLSD& value, bool select = true);
	virtual bool selectItemByUUID(const LLUUID& uuid, bool select = true);
	virtual void getItems(std::vector<LLPanel*>& items) const;
	virtual void getValues(std::vector<LLSD>& values) const;
	virtual LLSD getSelectedValue() const;
	virtual void getSelectedValues(std::vector<LLSD>& selected_values) const;
	virtual LLUUID getSelectedUUID() const;
	virtual void getSelectedUUIDs(uuid_vec_t& selected_uuids) const;
	virtual LLPanel* getSelectedItem() const;
	virtual void getSelectedItems(std::vector<LLPanel*>& selected_items) const;
	virtual void resetSelection(bool no_commit_on_deselection = false);
	void setNoItemsCommentText( const std::string& comment_text);
	void setAllowMultipleSelection(bool allow) { mMultipleSelection = allow; }
	void setAllowSelection(bool can_select) { mAllowSelection = can_select; }
	void setCommitOnSelectionChange(bool b)		{ mCommitOnSelectionChange = b; }
	U32 numSelected() const {return mSelectedItemPairs.size(); }
	U32 size(const bool only_visible_items = true) const;
	void clear() override;
	void detachItems(std::vector<LLPanel*>& detached_items);
	void setComparator(const ItemComparator* comp) { mItemComparator = comp; }
	void sort();
	bool updateValue(const LLSD& old_value, const LLSD& new_value);
	void scrollToShowFirstSelectedItem();
	void selectFirstItem	();
	void selectLastItem		();
	S32	notify(const LLSD& info) override;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	virtual ~LLFlatListView();
protected:
	typedef std::pair<LLPanel*, LLSD> item_pair_t;
	typedef std::list<item_pair_t*> pairs_list_t;
	typedef pairs_list_t::iterator pairs_iterator_t;
	typedef pairs_list_t::const_iterator pairs_const_iterator_t;
	struct ComparatorAdaptor
	{
		ComparatorAdaptor(const ItemComparator& comparator) : mComparator(comparator) {};
		bool operator()(const item_pair_t* item_pair1, const item_pair_t* item_pair2) const
		{
			return mComparator.compare(item_pair1->first, item_pair2->first);
		}
		const ItemComparator& mComparator;
	};
	friend class LLUICtrlFactory;
	LLFlatListView(const std::string& name, const LLRect& rect, bool opaque, const LLColor4& color, const S32& item_pad, bool allow_select, bool multi_select, bool keep_one_selected, bool keep_selection_visible_on_reshape, const std::string& no_items_text);
	void onItemMouseClick(item_pair_t* item_pair, MASK mask);
	void onItemRightMouseClick(item_pair_t* item_pair, MASK mask);
	virtual void rearrangeItems();
	virtual item_pair_t* getItemPair(LLPanel* item) const;
	virtual item_pair_t* getItemPair(const LLSD& value) const;
	virtual bool selectItemPair(item_pair_t* item_pair, bool select);
	virtual bool selectNextItemPair(bool is_up_direction, bool reset_selection);
	BOOL canSelectAll() const override;
	void selectAll() override;
	virtual bool isSelected(item_pair_t* item_pair) const;
	virtual bool removeItemPair(item_pair_t* item_pair, bool rearrange);
	bool addItemPairs(pairs_list_t panel_list, bool rearrange = true);
	void notifyParentItemsRectChanged();
	BOOL handleKeyHere(KEY key, MASK mask) override;
	BOOL postBuild() override;
	void onFocusReceived() override;
	void onFocusLost() override;
	void draw() override;
	LLRect getLastSelectedItemRect();
	void   ensureSelectedVisible();
private:
	void setItemsNoScrollWidth(S32 new_width) {mItemsNoScrollWidth = new_width - 2 * mBorderThickness;}
	void setNoItemsCommentVisible(bool visible) const;
protected:
	const ItemComparator* mItemComparator;
private:
	LLPanel* mItemsPanel;
	S32 mItemsNoScrollWidth;
	S32 mBorderThickness;
	S32 mItemPad;
	bool mAllowSelection;
	bool mMultipleSelection;
	bool mCommitOnSelectionChange;
	bool mKeepOneItemSelected;
	bool mIsConsecutiveSelection;
	bool mKeepSelectionVisibleOnReshape;
	pairs_list_t mItemPairs;
	pairs_list_t mSelectedItemPairs;
	LLRect mPrevNotifyParentRect;
	LLTextBox* mNoItemsCommentTextbox;
	LLViewBorder* mSelectedItemsBorder;
	commit_signal_t	mOnReturnSignal;
};
class LLFlatListViewEx : public LLFlatListView
{
public:
	LOG_CLASS(LLFlatListViewEx);
	struct Params : public LLInitParam::Block<Params, LLFlatListView::Params>
	{
		Optional<std::string>	no_items_msg;
		Optional<std::string>	no_filtered_items_msg;
		Params();
	};
	void setNoItemsMsg(const std::string& msg) { mNoItemsMsg = msg; }
	void setNoFilteredItemsMsg(const std::string& msg) { mNoFilteredItemsMsg = msg; }
	bool getForceShowingUnmatchedItems();
	void setForceShowingUnmatchedItems(bool show);
	void setFilterSubString(const std::string& filter_str);
	std::string getFilterSubString() const { return mFilterSubString; }
	void filterItems();
	bool hasMatchedItems();
protected:
	LLFlatListViewEx(const Params& p);
	void updateNoItemsMessage(const std::string& filter_string);
	void updateItemVisibility(LLPanel* item, const LLSD &action);
private:
	std::string mNoFilteredItemsMsg;
	std::string mNoItemsMsg;
	std::string	mFilterSubString;
	bool mForceShowingUnmatchedItems;
	bool mHasMatchedItems;
};
#endif
