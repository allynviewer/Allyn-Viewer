/**
 * @file llfloatermarketplacelistings.cpp
 * @brief Implementation of the marketplace listings floater and panels
 * @author merov@lindenlab.com
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
#include "llviewerprecompiledheaders.h"
#include "llfloatermarketplacelistings.h"
#include "llfiltereditor.h"
#include "llfolderview.h"
#include "llinventorybridge.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventoryfunctions.h"
#include "llmarketplacefunctions.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "lltextbox.h"
#include "lltrans.h"
static LLPanelInjector<LLPanelMarketplaceListings> t_panel_status("llpanelmarketplacelistings");
LLPanelMarketplaceListings::LLPanelMarketplaceListings()
: mRootFolder(NULL)
, mAuditBtn(nullptr)
, mFilterEditor(nullptr)
, mSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME)
, mFilterListingFoldersOnly(false)
{
}
LLPanelMarketplaceListings::~LLPanelMarketplaceListings() { delete mMenu; }
void showMenu(LLView* button, LLMenuGL* menu)
{
	menu->buildDrawLabels();
	menu->updateParent(LLMenuGL::sMenuContainer);
	const auto& rect = button->getRect();
	LLMenuGL::showPopup(button->getParent(), menu, rect.mLeft, rect.mBottom);
}
BOOL LLPanelMarketplaceListings::postBuild()
{
	childSetAction("add_btn", boost::bind(&LLPanelMarketplaceListings::onAddButtonClicked, this));
	childSetAction("audit_btn", boost::bind(&LLPanelMarketplaceListings::onAuditButtonClicked, this));
	mMenu = LLUICtrlFactory::instance().buildMenu("menu_marketplace_view.xml", LLMenuGL::sMenuContainer);
	auto sort = getChild<LLUICtrl>("sort_btn");
	sort->setCommitCallback(boost::bind(showMenu, _1, mMenu));
	mFilterEditor = getChild<LLFilterEditor>("filter_editor");
	mFilterEditor->setCommitCallback(boost::bind(&LLPanelMarketplaceListings::onFilterEdit, this, _2));
	mAuditBtn = getChild<LLButton>("audit_btn");
	mAuditBtn->setEnabled(FALSE);
	return LLPanel::postBuild();
}
BOOL LLPanelMarketplaceListings::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
					   EDragAndDropType cargo_type,
					   void* cargo_data,
					   EAcceptance* accept,
					   std::string& tooltip_msg)
{
	LLView * handled_view = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
	if (!handled_view || (handled_view->getName() == "marketplace_drop_zone"))
	{
		LLFolderView* root_folder = getRootFolder();
		return root_folder->handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
	}
	return false;
}
void LLPanelMarketplaceListings::buildAllPanels()
{
	LLInventoryPanel* panel_all_items;
	panel_all_items = buildInventoryPanel("All Items", "panel_marketplace_listings_inventory.xml");
	panel_all_items->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
	panel_all_items->getFilter().markDefault();
	LLInventoryPanel* panel;
	panel = buildInventoryPanel("Active Items", "panel_marketplace_listings_listed.xml");
	panel->getFilter().setFilterMarketplaceActiveFolders();
	panel->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
	panel->getFilter().markDefault();
	panel = buildInventoryPanel("Inactive Items", "panel_marketplace_listings_unlisted.xml");
	panel->getFilter().setFilterMarketplaceInactiveFolders();
	panel->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
	panel->getFilter().markDefault();
	panel = buildInventoryPanel("Unassociated Items", "panel_marketplace_listings_unassociated.xml");
	panel->getFilter().setFilterMarketplaceUnassociatedFolders();
	panel->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
	panel->getFilter().markDefault();
	LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
	tabs_panel->setCommitCallback(boost::bind(&LLPanelMarketplaceListings::onTabChange, this));
	tabs_panel->selectTabPanel(panel_all_items);
	mRootFolder = panel_all_items->getRootFolder();
	setSortOrder(gSavedSettings.getU32("MarketplaceListingsSortOrder"));
}
LLInventoryPanel* LLPanelMarketplaceListings::buildInventoryPanel(const std::string& childname, const std::string& filename)
{
	LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
	LLInventoryPanel* panel = tabs_panel->getChild<LLInventoryPanel>(childname, false, false);
	llassert(panel != NULL);
	panel->getRootFolder()->setSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->setSelectCallback(boost::bind(&LLPanelMarketplaceListings::onSelectionChange, this, panel, _1, _2));
	return panel;
}
void LLPanelMarketplaceListings::setSortOrder(U32 sort_order)
{
	mSortOrder = sort_order;
	gSavedSettings.setU32("MarketplaceListingsSortOrder", sort_order);
	LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
	LLInventoryPanel* panel = (LLInventoryPanel*)tabs_panel->getPanelByName("All Items");
	panel->setSortOrder(mSortOrder);
	panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Active Items");
	panel->setSortOrder(mSortOrder);
	panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Inactive Items");
	panel->setSortOrder(mSortOrder);
	panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Unassociated Items");
	panel->setSortOrder(mSortOrder);
}
void LLPanelMarketplaceListings::onFilterEdit(const std::string& search_string)
{
	LLInventoryPanel* panel = (LLInventoryPanel*)getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
	if (panel)
	{
		mFilterSubString = search_string;
		panel->setFilterSubString(mFilterSubString);
	}
}
void LLPanelMarketplaceListings::draw()
{
	if (LLMarketplaceData::instance().checkDirtyCount())
	{
		update_all_marketplace_count();
	}
	if (!mAuditBtn->getEnabled())
	{
		mAuditBtn->setEnabled(LLInventoryModelBackgroundFetch::instance().isEverythingFetched());
	}
	LLPanel::draw();
}
void LLPanelMarketplaceListings::onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, BOOL user_action)
{
	panel->onSelectionChange(items, user_action);
}
bool LLPanelMarketplaceListings::allowDropOnRoot()
{
	LLInventoryPanel* panel = (LLInventoryPanel*)getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
	return (panel ? panel->getAllowDropOnRoot() : false);
}
void LLPanelMarketplaceListings::onTabChange()
{
	LLInventoryPanel* panel = (LLInventoryPanel*)getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
	if (panel)
	{
		LLButton* add_btn = getChild<LLButton>("add_btn");
		add_btn->setEnabled(panel->getAllowDropOnRoot());
		panel->setFilterSubString(mFilterSubString);
		LLPanel* drop_zone = (LLPanel*)getChild<LLPanel>("marketplace_drop_zone");
		bool drop_zone_visible = drop_zone->getVisible();
		bool allow_drop_on_root = panel->getAllowDropOnRoot();
		if (drop_zone_visible != allow_drop_on_root)
		{
			LLPanel* tabs = (LLPanel*)getChild<LLPanel>("tab_container_panel");
			S32 delta_height = drop_zone->getRect().getHeight();
			delta_height = (drop_zone_visible ? delta_height : -delta_height);
			tabs->reshape(tabs->getRect().getWidth(),tabs->getRect().getHeight() + delta_height);
			tabs->translate(0,-delta_height);
		}
		drop_zone->setVisible(allow_drop_on_root);
	}
}
void LLPanelMarketplaceListings::onAddButtonClicked()
{
	LLInventoryPanel* panel = (LLInventoryPanel*)getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
	if (panel)
	{
		LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		llassert(marketplacelistings_id.notNull());
		LLFolderType::EType preferred_type = LLFolderType::lookup("category");
		LLUUID category = gInventory.createNewCategory(marketplacelistings_id, preferred_type, LLStringUtil::null);
		gInventory.notifyObservers();
		panel->setSelectionByID(category, TRUE);
		panel->getRootFolder()->setNeedsAutoRename(TRUE);
	}
}
void LLPanelMarketplaceListings::onAuditButtonClicked()
{
	LLSD data(LLSD::emptyMap());
	new LLFloaterMarketplaceValidation(data);
}
void LLPanelMarketplaceListings::onViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();
	if ((chosen_item == "sort_by_stock_amount") || (chosen_item == "sort_by_name") || (chosen_item == "sort_by_recent"))
	{
		if (chosen_item == "sort_by_stock_amount")
		{
			setSortOrder(LLInventoryFilter::SO_FOLDERS_BY_WEIGHT);
		}
		else if (chosen_item == "sort_by_name")
		{
			setSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME);
		}
		else if (chosen_item == "sort_by_recent")
		{
			setSortOrder(LLInventoryFilter::SO_DATE);
		}
	}
	else if (chosen_item == "show_only_listing_folders")
	{
		mFilterListingFoldersOnly = !mFilterListingFoldersOnly;
		LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
		LLInventoryPanel* panel = (LLInventoryPanel*)tabs_panel->getPanelByName("All Items");
		panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
		panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Active Items");
		panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
		panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Inactive Items");
		panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
		panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Unassociated Items");
		panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
	}
}
bool LLPanelMarketplaceListings::onViewSortMenuItemCheck(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();
	if ((chosen_item == "sort_by_stock_amount") || (chosen_item == "sort_by_name") || (chosen_item == "sort_by_recent"))
	{
		if (chosen_item == "sort_by_stock_amount")
		{
			return (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_WEIGHT);
		}
		else if (chosen_item == "sort_by_name")
		{
			return (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_NAME);
		}
		else if (chosen_item == "sort_by_recent")
		{
			return (mSortOrder & LLInventoryFilter::SO_DATE);
		}
	}
	else if (chosen_item == "show_only_listing_folders")
	{
		return mFilterListingFoldersOnly;
	}
	return false;
}
class LLMarketplaceListingsAddedObserver : public LLInventoryCategoryAddedObserver
{
public:
	LLMarketplaceListingsAddedObserver(LLFloaterMarketplaceListings * marketplace_listings_floater)
	: LLInventoryCategoryAddedObserver()
	, mMarketplaceListingsFloater(marketplace_listings_floater)
	{
	}
	void done()
	{
		for (cat_vec_t::iterator it = mAddedCategories.begin(); it != mAddedCategories.end(); ++it)
		{
			LLViewerInventoryCategory* added_category = *it;
			LLFolderType::EType added_category_type = added_category->getPreferredType();
			if (added_category_type == LLFolderType::FT_MARKETPLACE_LISTINGS)
			{
				mMarketplaceListingsFloater->initializeMarketPlace();
			}
		}
	}
private:
	LLFloaterMarketplaceListings *	mMarketplaceListingsFloater;
};
LLFloaterMarketplaceListings::LLFloaterMarketplaceListings(const LLSD& key)
: LLFloater(key)
, mCategoriesObserver(NULL)
, mCategoryAddedObserver(NULL)
, mRootFolderId(LLUUID::null)
, mInventoryStatus(NULL)
, mInventoryInitializationInProgress(NULL)
, mInventoryPlaceholder(NULL)
, mInventoryText(NULL)
, mInventoryTitle(NULL)
, mPanelListings(NULL)
, mPanelListingsSet(false)
{
	mFactoryMap["panel_marketplace_listing"] = LLCallbackMap([&](void*) { return mPanelListings = new LLPanelMarketplaceListings; });
	LLUICtrlFactory::instance().buildFloater(this, "floater_marketplace_listings.xml", &getFactoryMap());
}
LLFloaterMarketplaceListings::~LLFloaterMarketplaceListings()
{
	if (mCategoriesObserver && gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;
	if (mCategoryAddedObserver && gInventory.containsObserver(mCategoryAddedObserver))
	{
		gInventory.removeObserver(mCategoryAddedObserver);
	}
	delete mCategoryAddedObserver;
}
BOOL LLFloaterMarketplaceListings::postBuild()
{
	mInventoryStatus = getChild<LLTextBox>("marketplace_status");
	mInventoryInitializationInProgress = getChild<LLView>("initialization_progress_indicator");
	mInventoryPlaceholder = getChild<LLView>("marketplace_listings_inventory_placeholder_panel");
	mInventoryText = mInventoryPlaceholder->getChild<LLTextEditor>("marketplace_listings_inventory_placeholder_text");
	mInventoryTitle = mInventoryPlaceholder->getChild<LLTextBox>("marketplace_listings_inventory_placeholder_title");
	LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLFloaterMarketplaceListings::onFocusReceived, this));
	mCategoryAddedObserver = new LLMarketplaceListingsAddedObserver(this);
	gInventory.addObserver(mCategoryAddedObserver);
	fetchContents();
	return TRUE;
}
void LLFloaterMarketplaceListings::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
}
void LLFloaterMarketplaceListings::onOpen()
{
	if (LLMarketplaceData::instance().getSLMStatus() <= MarketplaceStatusCodes::MARKET_PLACE_CONNECTION_FAILURE)
	{
		initializeMarketPlace();
	}
	else
	{
		updateView();
	}
}
void LLFloaterMarketplaceListings::onFocusReceived()
{
	updateView();
}
void LLFloaterMarketplaceListings::fetchContents()
{
	if (mRootFolderId.notNull() &&
	 (LLMarketplaceData::instance().getSLMDataFetched() != MarketplaceFetchCodes::MARKET_FETCH_LOADING) &&
	 (LLMarketplaceData::instance().getSLMDataFetched() != MarketplaceFetchCodes::MARKET_FETCH_DONE))
	{
		LLMarketplaceData::instance().setDataFetchedSignal(boost::bind(&LLFloaterMarketplaceListings::updateView, this));
		LLMarketplaceData::instance().setSLMDataFetched(MarketplaceFetchCodes::MARKET_FETCH_LOADING);
		LLInventoryModelBackgroundFetch::instance().start(mRootFolderId);
		LLMarketplaceData::instance().getSLMListings();
	}
}
void LLFloaterMarketplaceListings::setRootFolder()
{
	if ((LLMarketplaceData::instance().getSLMStatus() != MarketplaceStatusCodes::MARKET_PLACE_MERCHANT) &&
		(LLMarketplaceData::instance().getSLMStatus() != MarketplaceStatusCodes::MARKET_PLACE_MIGRATED_MERCHANT))
	{
		return;
	}
	LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, true);
	if (marketplacelistings_id.isNull())
	{
		LL_ERRS("SLM") << "Inventory problem: failure to create the marketplace listings folder for a merchant!" << LL_ENDL;
		return;
	}
	if (mCategoryAddedObserver && gInventory.containsObserver(mCategoryAddedObserver))
	{
		gInventory.removeObserver(mCategoryAddedObserver);
		delete mCategoryAddedObserver;
		mCategoryAddedObserver = NULL;
	}
	llassert(!mCategoryAddedObserver);
	if (marketplacelistings_id == mRootFolderId)
	{
		LL_WARNS("SLM") << "Inventory warning: Marketplace listings folder already set" << LL_ENDL;
		return;
	}
	mRootFolderId = marketplacelistings_id;
}
void LLFloaterMarketplaceListings::setPanels()
{
	if (mRootFolderId.isNull())
	{
		return;
	}
	gInventory.consolidateForType(mRootFolderId, LLFolderType::FT_MARKETPLACE_LISTINGS);
	mPanelListings->buildAllPanels();
	if (!mCategoriesObserver)
	{
		mCategoriesObserver = new LLInventoryCategoriesObserver();
		llassert(mCategoriesObserver);
		gInventory.addObserver(mCategoriesObserver);
		mCategoriesObserver->addCategory(mRootFolderId, boost::bind(&LLFloaterMarketplaceListings::onChanged, this));
	}
	fetchContents();
	mPanelListingsSet = true;
}
void LLFloaterMarketplaceListings::initializeMarketPlace()
{
	LLMarketplaceData::instance().initializeSLM(boost::bind(&LLFloaterMarketplaceListings::updateView, this));
}
S32 LLFloaterMarketplaceListings::getFolderCount()
{
	if (mPanelListings && mRootFolderId.notNull())
	{
		LLInventoryModel::cat_array_t * cats;
		LLInventoryModel::item_array_t * items;
		gInventory.getDirectDescendentsOf(mRootFolderId, cats, items);
		return (cats->size() + items->size());
	}
	else
	{
		return 0;
	}
}
void LLFloaterMarketplaceListings::setStatusString(const std::string& statusString)
{
	mInventoryStatus->setText(statusString);
}
void LLFloaterMarketplaceListings::updateView()
{
	U32 mkt_status = LLMarketplaceData::instance().getSLMStatus();
	bool is_merchant = (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_MERCHANT) || (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_MIGRATED_MERCHANT);
	U32 data_fetched = LLMarketplaceData::instance().getSLMDataFetched();
	if (mRootFolderId.isNull() && is_merchant)
	{
		setRootFolder();
	}
	if ((mkt_status <= MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING) || (is_merchant && (data_fetched <= MarketplaceFetchCodes::MARKET_FETCH_LOADING)) )
	{
		mInventoryInitializationInProgress->setVisible(true);
		mPanelListings->setVisible(FALSE);
		fetchContents();
		return;
	}
	else
	{
		mInventoryInitializationInProgress->setVisible(false);
	}
	if (getFolderCount() > 0)
	{
		if (!mPanelListingsSet)
		{
			setPanels();
		}
		mPanelListings->setVisible(TRUE);
		mInventoryPlaceholder->setVisible(FALSE);
	}
	else
	{
		mPanelListings->setVisible(FALSE);
		mInventoryPlaceholder->setVisible(TRUE);
		std::string text;
		std::string title;
		std::string tooltip;
		const LLSD& subs = LLMarketplaceData::instance().getMarketplaceStringSubstitutions();
		if (mRootFolderId.notNull())
		{
			text = LLTrans::getString("InventoryMarketplaceListingsNoItems", subs);
			title = LLTrans::getString("InventoryMarketplaceListingsNoItemsTitle");
			tooltip = LLTrans::getString("InventoryMarketplaceListingsNoItemsTooltip");
		}
		else if (mkt_status <= MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING)
		{
			text = LLTrans::getString("InventoryOutboxInitializing", subs);
			title = LLTrans::getString("InventoryOutboxInitializingTitle");
			tooltip = LLTrans::getString("InventoryOutboxInitializingTooltip");
		}
		else if (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_NOT_MERCHANT)
		{
			text = LLTrans::getString("InventoryOutboxNotMerchant", subs);
			title = LLTrans::getString("InventoryOutboxNotMerchantTitle");
			tooltip = LLTrans::getString("InventoryOutboxNotMerchantTooltip");
		}
		else
		{
			text = LLTrans::getString("InventoryMarketplaceError", subs);
			title = LLTrans::getString("InventoryOutboxErrorTitle");
			tooltip = LLTrans::getString("InventoryOutboxErrorTooltip");
		}
		mInventoryText->setValue(text);
		mInventoryTitle->setValue(title);
		mInventoryPlaceholder->getParent()->setToolTip(tooltip);
	}
}
bool LLFloaterMarketplaceListings::isAccepted(EAcceptance accept)
{
	return (accept >= ACCEPT_YES_COPY_SINGLE);
}
BOOL LLFloaterMarketplaceListings::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										EDragAndDropType cargo_type,
										void* cargo_data,
										EAcceptance* accept,
										std::string& tooltip_msg)
{
	if (!mPanelListings || mRootFolderId.isNull())
	{
		return FALSE;
	}
	tooltip_msg = "";
	LLView * handled_view = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
	BOOL handled = (handled_view != NULL);
	if ((!handled || !isAccepted(*accept)) && !mPanelListings->getVisible() && mRootFolderId.notNull())
	{
		if (!mPanelListingsSet)
		{
			setPanels();
		}
		LLFolderView* root_folder = mPanelListings->getRootFolder();
		handled = root_folder->handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
	}
	return handled;
}
BOOL LLFloaterMarketplaceListings::handleHover(S32 x, S32 y, MASK mask)
{
	return LLFloater::handleHover(x, y, mask);
}
void LLFloaterMarketplaceListings::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLFloater::onMouseLeave(x, y, mask);
}
void LLFloaterMarketplaceListings::onChanged()
{
	LLViewerInventoryCategory* category = gInventory.getCategory(mRootFolderId);
	if (mRootFolderId.notNull() && category)
	{
		updateView();
	}
	else
	{
		mRootFolderId.setNull();
	}
}
bool hasUniqueVersionFolder(const LLUUID& folder_id)
{
	LLInventoryModel::cat_array_t* categories;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(folder_id, categories, items);
	return (categories->size() == 1);
}
LLFloaterAssociateListing::LLFloaterAssociateListing(const LLSD& key)
: LLFloater(key)
, mUUID()
{
	LLUICtrlFactory::instance().buildFloater(this, "floater_associate_listing.xml");
}
LLFloaterAssociateListing::~LLFloaterAssociateListing()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}
BOOL LLFloaterAssociateListing::postBuild()
{
	getChild<LLButton>("OK")->setCommitCallback(boost::bind(&LLFloaterAssociateListing::apply, this, TRUE));
	getChild<LLButton>("Cancel")->setCommitCallback(boost::bind(&LLFloaterAssociateListing::cancel, this));
	getChild<LLLineEditor>("listing_id")->setPrevalidate(&LLLineEditor::prevalidateNonNegativeS32);
	center();
	return LLFloater::postBuild();
}
BOOL LLFloaterAssociateListing::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		apply();
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		cancel();
		return TRUE;
	}
	return LLFloater::handleKeyHere(key, mask);
}
void LLFloaterAssociateListing::show(LLFloaterAssociateListing* floater, const LLSD& folder_id)
{
	if (!floater) return;
	floater->mUUID = folder_id.asUUID();
	floater->open();
}
void LLFloaterAssociateListing::callback_apply(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		apply(FALSE);
	}
}
void LLFloaterAssociateListing::apply(BOOL user_confirm)
{
	if (mUUID.notNull())
	{
		S32 id = (S32)getChild<LLUICtrl>("listing_id")->getValue().asInteger();
		if (id > 0)
		{
			LLUUID listing_uuid = LLMarketplaceData::instance().getListingFolder(id);
			if (listing_uuid.notNull() && user_confirm && LLMarketplaceData::instance().getActivationState(listing_uuid) && !hasUniqueVersionFolder(mUUID))
			{
				LLNotificationsUtil::add("ConfirmMerchantUnlist", LLSD(), LLSD(), boost::bind(&LLFloaterAssociateListing::callback_apply, this, _1, _2));
				return;
			}
			LLMarketplaceData::instance().associateListing(mUUID,listing_uuid,id);
		}
		else
		{
			LLNotificationsUtil::add("AlertMerchantListingInvalidID");
		}
	}
	close();
}
void LLFloaterAssociateListing::cancel()
{
	close();
}
LLFloaterMarketplaceValidation::LLFloaterMarketplaceValidation(const LLSD& key)
:	LLFloater(),
	mKey(key),
	mEditor(NULL)
{
	LLUICtrlFactory::instance().buildFloater(this, "floater_marketplace_validation.xml");
}
BOOL LLFloaterMarketplaceValidation::postBuild()
{
	childSetAction("OK", onOK, this);
	mEditor = getChild<LLTextEditor>("validation_text");
	mEditor->setEnabled(FALSE);
	mEditor->setFocus(TRUE);
	mEditor->setValue(LLSD());
	return TRUE;
}
LLFloaterMarketplaceValidation::~LLFloaterMarketplaceValidation()
{
}
void LLFloaterMarketplaceValidation::draw()
{
	LLFloater::draw();
}
void LLFloaterMarketplaceValidation::onOpen()
{
	clearMessages();
	LLUUID cat_id(mKey.asUUID());
	if (cat_id.isNull())
	{
		cat_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	}
	if (cat_id.notNull())
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
		validate_marketplacelistings(cat, boost::bind(&LLFloaterMarketplaceValidation::appendMessage, this, _1, _2, _3), false);
	}
	handleCurrentListing();
	if (mEditor)
	{
		mEditor->setValue(LLSD());
		if (mMessages.empty())
		{
			mEditor->appendText(LLTrans::getString("Marketplace Validation No Error"), false, false);
		}
		else
		{
			message_list_t::iterator mCurrentLine = mMessages.begin();
			bool new_line = false;
			while (mCurrentLine != mMessages.end())
			{
				LLStyleSP style(new LLStyle);
				style->setColor(mEditor->getReadOnlyFgColor());
				style->mBold = mCurrentLine->mErrorLevel == LLError::LEVEL_ERROR;
				mEditor->appendText(mCurrentLine->mMessage, false, new_line, style);
				new_line = true;
				mCurrentLine++;
			}
		}
	}
	clearMessages();
}
void LLFloaterMarketplaceValidation::onOK( void* userdata )
{
	LLFloaterMarketplaceValidation* self = (LLFloaterMarketplaceValidation*) userdata;
	self->clearMessages();
	self->close();
}
void LLFloaterMarketplaceValidation::appendMessage(std::string& message, S32 depth, LLError::ELevel log_level)
{
	if (depth == 1)
	{
		handleCurrentListing();
	}
	Message current_message;
	current_message.mErrorLevel = log_level;
	current_message.mMessage = message;
	mCurrentListingMessages.push_back(current_message);
	mCurrentListingErrorLevel = (mCurrentListingErrorLevel < log_level ? log_level : mCurrentListingErrorLevel);
}
void LLFloaterMarketplaceValidation::handleCurrentListing()
{
	if (mCurrentListingErrorLevel > LLError::LEVEL_INFO)
	{
		message_list_t::iterator mCurrentLine = mCurrentListingMessages.begin();
		while (mCurrentLine != mCurrentListingMessages.end())
		{
			mMessages.push_back(*mCurrentLine);
			mCurrentLine++;
		}
	}
	mCurrentListingMessages.clear();
	mCurrentListingErrorLevel = LLError::LEVEL_INFO;
}
void LLFloaterMarketplaceValidation::clearMessages()
{
	mMessages.clear();
	mCurrentListingMessages.clear();
	mCurrentListingErrorLevel = LLError::LEVEL_INFO;
}
