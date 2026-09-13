/** 
 * @file llpanelmaininventory.h
 * @brief llpanelmaininventory.h
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LL_LLPANELMAININVENTORY_H
#define LL_LLPANELMAININVENTORY_H
#include "llfloater.h"
#include "llinventoryobserver.h"
#include "llfolderview.h"
class LLFolderViewItem;
class LLInventoryPanel;
class LLSaveFolderState;
class LLFilterEditor;
class LLTabContainer;
class LLMenuButton;
class LLMenuGL;
class LLToggleableMenu;
class LLFloater;
class LLFilterEditor;
class LLComboBox;
class LLFloaterInventoryFinder;
class LLPanelMainInventory : public LLFloater, LLInventoryObserver
{
friend class LLFloaterInventoryFinder;
public:
	LLPanelMainInventory(const std::string& name, const std::string& rect,
			LLInventoryModel* inventory);
	LLPanelMainInventory(const std::string& name, const LLRect& rect,
					LLInventoryModel* inventory);
	~LLPanelMainInventory();
	 BOOL postBuild();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	static LLPanelMainInventory* showAgentInventory(BOOL take_keyboard_focus=FALSE);
	static LLPanelMainInventory* getActiveInventory();
	static void toggleVisibility();
	static void toggleVisibility(void*) { toggleVisibility(); }
	static void cleanup();
	virtual void onClose(bool app_quitting);
	virtual void setVisible(BOOL visible);
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);
	void changed(U32);
	void draw();
	LLInventoryPanel* getPanel() { return mActivePanel; }
	LLInventoryPanel* getActivePanel() { return mActivePanel; }
	const LLInventoryPanel* getActivePanel() const { return mActivePanel; }
	LLFolderView* getRootFolder() const;
	const std::string& getFilterText() const { return mFilterText; }
	void setSelectCallback(const LLFolderView::signal_t::slot_type& cb);
	void onFilterEdit(const std::string& search_string);
	void setFilterTextFromFilter();
	void startSearch();
	void toggleFindOptions();
	void onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, BOOL user_action);
	static BOOL filtersVisible(void* user_data);
	static void onFoldersByName(void *user_data);
	static BOOL checkFoldersByName(void *user_data);
	void onFilterSelected();
	const std::string getFilterSubString();
	void setFilterSubString(const std::string& string);
	static void onQuickFilterCommit(LLUICtrl* ctrl, void* user_data);
	static void refreshQuickFilter(LLUICtrl* ctrl);
	static void onResetAll(void* userdata);
	static void onExpandAll(void* userdata);
	static void onCollapseAll(void* userdata);
	void updateSortControls();
	void resetFilters();
	void updateItemcountText();
	void layoutInventoryChrome();
	static void closeAll()
	{
		LLPanelMainInventory* pView; U8 flagsSound;
		for (S32 idx = sActiveViews.size() - 1; idx >= 0; idx--)
		{
			pView = sActiveViews.at(idx);
			flagsSound = pView->getSoundFlags();
			pView->setSoundFlags(LLView::SILENT);
			pView->close();
			pView->setSoundFlags(flagsSound);
		}
	}
protected:
	void init(LLInventoryModel* inventory);
protected:
	LLFloaterInventoryFinder* getFinder();
	LLFilterEditor*				mFilterEditor;
	LLComboBox*					mQuickFilterCombo;
	LLTabContainer*				mFilterTabs;
	LLHandle<LLFloater>			mFinderHandle;
	LLInventoryPanel*			mActivePanel;
	bool						mResortActivePanel;
	std::string					mFilterText;
	static std::vector<LLPanelMainInventory*> sActiveViews;
};
#endif
