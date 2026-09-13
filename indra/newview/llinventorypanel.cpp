/* 
 * @file llinventorypanel.cpp
 * @brief Implementation of the inventory panel and associated stuff.
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
#include "llinventorypanel.h"
#include <utility>
#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "lluictrlfactory.h"
#include "llfloatercustomize.h"
#include "llfolderview.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventoryfilter.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llviewerfoldertype.h"
#include "llvoavatarself.h"
#include "llscrollcontainer.h"
#include "llviewerassettype.h"
#include "llpanelmaininventory.h"
#include "llsdserialize.h"
#include "rlvhandler.h"
static LLRegisterWidget<LLInventoryPanel> r("inventory_panel");
const std::string LLInventoryPanel::DEFAULT_SORT_ORDER = std::string("InventorySortOrder");
const std::string LLInventoryPanel::RECENTITEMS_SORT_ORDER = std::string("RecentItemsSortOrder");
const std::string LLInventoryPanel::WORNITEMS_SORT_ORDER = std::string("WornItemsSortOrder");
const std::string LLInventoryPanel::INHERIT_SORT_ORDER = std::string("");
static const LLInventoryFolderViewModelBuilder INVENTORY_BRIDGE_BUILDER;
class LLInventoryPanelObserver : public LLInventoryObserver
{
public:
	LLInventoryPanelObserver(LLInventoryPanel* ip) : mIP(ip) {}
	virtual ~LLInventoryPanelObserver() {}
	void changed(U32 mask) override
	{
		mIP->modelChanged(mask);
	}
protected:
	LLInventoryPanel* mIP;
};
class LLInvPanelComplObserver : public LLInventoryCompletionObserver
{
public:
	typedef std::function<void()> callback_t;
	LLInvPanelComplObserver(callback_t cb)
	:	mCallback(cb)
	{
	}
	void reset();
private:
	void done() override;
	callback_t	mCallback;
};
void LLInvPanelComplObserver::reset()
{
	mIncomplete.clear();
	mComplete.clear();
}
void LLInvPanelComplObserver::done()
{
	mCallback();
}
LLInventoryPanel::LLInventoryPanel(const std::string& name,
								    const std::string& sort_order_setting,
									const LLSD& start_folder,
									const LLRect& rect,
									LLInventoryModel* inventory,
									BOOL allow_multi_select,
									LLView *parent_view) :
	LLPanel(name, rect, TRUE),
	mInventoryObserver(NULL),
	mCompletionObserver(NULL),
	mScroller(NULL),
	mSortOrderSetting(sort_order_setting),
	mStartFolder(start_folder),
	mShowRootFolder(false),
	mShowEmptyMessage(true),
	mAllowDropOnRoot(true),
	mAllowWear(true),
	mUseMarketplaceFolders(false),
	mInventory(inventory),
	mAllowMultiSelect(allow_multi_select),
	mViewsInitialized(false),
	mInvFVBridgeBuilder(NULL),
	mGroupedItemBridge(new LLFolderViewGroupedItemBridge)
{
	mInvFVBridgeBuilder = &INVENTORY_BRIDGE_BUILDER;
	setBackgroundColor(gColors.getColor("InventoryBackgroundColor"));
	setBackgroundVisible(TRUE);
	setBackgroundOpaque(TRUE);
}
void LLInventoryPanel::buildFolderView()
{
	LLUUID root_id = getRootFolderID();
	if (!mStartFolder.has("id"))
		mStartFolder["id"] = root_id;
	LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(LLAssetType::AT_CATEGORY,
													(mUseMarketplaceFolders ? LLAssetType::AT_MARKETPLACE_FOLDER : LLAssetType::AT_CATEGORY),
													LLInventoryType::IT_CATEGORY,
													this,
													NULL,
													root_id);
	LLFolderView* folder_view = createFolderView(new_listener, true);
	mFolderRoot = folder_view->getHandle();
	addItemID(root_id, mFolderRoot.get());
}
BOOL LLInventoryPanel::postBuild()
{
	init_inventory_panel_actions(this);
	buildFolderView();
	{
		LLRect scroller_view_rect = getRect();
		scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
		mScroller = new LLScrollContainer(std::string("Inventory Scroller"),
												   scroller_view_rect,
												  mFolderRoot.get());
		mScroller->setFollowsAll();
		mScroller->setReserveScrollCorner(TRUE);
		addChild(mScroller);
		mFolderRoot.get()->setScrollContainer(mScroller);
	}
	mInventoryObserver = new LLInventoryPanelObserver(this);
	mInventory->addObserver(mInventoryObserver);
	mCompletionObserver = new LLInvPanelComplObserver(boost::bind(&LLInventoryPanel::onItemsCompletion, this));
	mInventory->addObserver(mCompletionObserver);
	if (mInventory->isInventoryUsable() && !mViewsInitialized)
	{
		initializeViews();
	}
	gIdleCallbacks.addFunction(onIdle, (void*)this);
	if (mSortOrderSetting != INHERIT_SORT_ORDER)
	{
		setSortOrder(gSavedSettings.getU32(mSortOrderSetting));
	}
	else
	{
		setSortOrder(gSavedSettings.getU32(DEFAULT_SORT_ORDER));
	}
	if (!gSavedSettings.getBOOL("InventoryOutboxMakeVisible"))
	{
		getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() & ~(1ULL << LLFolderType::FT_OUTBOX));
	}
	if (!gSavedSettings.getBOOL("InventoryOutboxMakeVisible") && !mUseMarketplaceFolders)
	{
		getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() & ~(1ULL << LLFolderType::FT_MARKETPLACE_LISTINGS));
	}
	if (gSavedSettings.getBOOL("DebugHideEmptySystemFolders"))
	{
		getFilter().setFilterEmptySystemFolders();
	}
	return LLPanel::postBuild();
}
LLInventoryPanel::~LLInventoryPanel()
{
	if (mFolderRoot.get())
	{
		U32 sort_order = mFolderRoot.get()->getSortOrder();
		if (mSortOrderSetting != INHERIT_SORT_ORDER)
		{
			gSavedSettings.setU32(mSortOrderSetting, sort_order);
		}
	}
	clearFolderRoot();
}
void LLInventoryPanel::clearFolderRoot()
{
	gIdleCallbacks.deleteFunction(onIdle, this);
	if (mInventoryObserver)
	{
		mInventory->removeObserver(mInventoryObserver);
		delete mInventoryObserver;
		mInventoryObserver = NULL;
	}
	if (mCompletionObserver)
	{
		mInventory->removeObserver(mCompletionObserver);
		delete mCompletionObserver;
		mCompletionObserver = NULL;
	}
	if (mScroller)
	{
		removeChild(mScroller);
		delete mScroller;
		mScroller = NULL;
	}
}
LLXMLNodePtr LLInventoryPanel::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLPanel::getXML(false);
	node->setName(LL_INVENTORY_PANEL_TAG);
		node->createChild("allow_multi_select", TRUE)->setBoolValue(mFolderRoot.get()->getAllowMultiSelect());
	return node;
}
class LLInventoryRecentItemsPanel : public LLInventoryPanel
{
public:
	LLInventoryRecentItemsPanel(const std::string& name,
								    const std::string& sort_order_setting,
								    const LLSD& start_folder,
									const LLRect& rect,
									LLInventoryModel* inventory,
									BOOL allow_multi_select,
									LLView *parent_view);
};
LLView* LLInventoryPanel::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string filename;
	if (node->getAttributeString("filename", filename) && !filename.empty())
	{
		LLXMLNodePtr node;
		if (LLUICtrlFactory::getLayeredXMLNode(filename, node))
			return factory->createCtrlWidget(static_cast<LLPanel*>(parent), node);
	}
	LLInventoryPanel* panel;
	std::string name("inventory_panel");
	node->getAttributeString("name", name);
	BOOL allow_multi_select = TRUE;
	node->getAttributeBOOL("allow_multi_select", allow_multi_select);
	LLRect rect;
	createRect(node, rect, parent, LLRect());
	std::string sort_order(INHERIT_SORT_ORDER);
	node->getAttributeString("sort_order", sort_order);
	LLSD start_folder;
	std::string start;
	if (node->getAttributeString("start_folder.name", start))
		start_folder["name"] = start;
	if (node->getAttributeString("start_folder.id", start))
		start_folder["id"] = LLUUID(start);
	if (node->getAttributeString("start_folder.type", start))
		start_folder["type"] = start;
	if (name == "Recent Items")
		panel = new LLInventoryRecentItemsPanel(name, sort_order, start_folder,
								 rect, &gInventory,
								 allow_multi_select, parent);
	else
		panel = new LLInventoryPanel(name, sort_order, start_folder,
								 rect, &gInventory,
								 allow_multi_select, parent);
	node->getAttribute_bool("show_root_folder", panel->mShowRootFolder);
	node->getAttribute_bool("show_empty_message", panel->mShowEmptyMessage);
	node->getAttribute_bool("allow_drop_on_root", panel->mAllowDropOnRoot);
	node->getAttribute_bool("allow_wear", panel->mAllowWear);
	node->getAttribute_bool("use_marketplace_folders", panel->mUseMarketplaceFolders);
	panel->initFromXML(node, parent);
	panel->postBuild();
	return panel;
}
void LLInventoryPanel::draw()
{
	updateSelection();
	LLPanel::draw();
}
LLInventoryFilter& LLInventoryPanel::getFilter()
{
	return mFolderRoot.get()->getFilter();
}
const LLInventoryFilter& LLInventoryPanel::getFilter() const
{
	return mFolderRoot.get()->getFilter();
}
void LLInventoryPanel::setFilterTypes(U64 types, LLInventoryFilter::EFilterType filter_type)
{
	if (filter_type == LLInventoryFilter::FILTERTYPE_OBJECT)
	{
		getFilter().setFilterObjectTypes(types);
	}
	if (filter_type == LLInventoryFilter::FILTERTYPE_CATEGORY)
		getFilter().setFilterCategoryTypes(types);
}
U32 LLInventoryPanel::getFilterObjectTypes() const
{
	return getFilter().getFilterObjectTypes();
}
U32 LLInventoryPanel::getFilterPermMask() const
{
	return getFilter().getFilterPermissions();
}
void LLInventoryPanel::setFilterPermMask(PermissionMask filter_perm_mask)
{
	getFilter().setFilterPermissions(filter_perm_mask);
}
void LLInventoryPanel::setFilterWearableTypes(U64 types)
{
	getFilter().setFilterWearableTypes(types);
}
void LLInventoryPanel::setFilterWornItems()
{
	getFilter().setFilterWornItems();
}
void LLInventoryPanel::setFilterSubString(const std::string& string)
{
	if (!getRootFolder())
		return;
	std::string uppercase_search_string = string;
	LLStringUtil::toUpper(uppercase_search_string);
	const std::string prior_search_string = getFilterSubString();
	if (prior_search_string == uppercase_search_string)
	{
		return;
	}
	if (string.empty())
	{
		getFilter().setFilterSubString(LLStringUtil::null);
		getRootFolder()->restoreFolderState();
		LLOpenFoldersWithSelection opener;
		getRootFolder()->applyFunctorRecursively(opener);
		getRootFolder()->scrollToShowSelection();
	}
	else
	{
		if (prior_search_string.empty())
		{
			getRootFolder()->saveFolderState();
		}
		getFilter().setFilterSubString(string);
	}
}
const std::string LLInventoryPanel::getFilterSubString()
{
	return getFilter().getFilterSubString();
}
void LLInventoryPanel::setSortOrder(U32 order)
{
	getFilter().setSortOrder(order);
	if (getFilter().isModified())
	{
		mFolderRoot.get()->setSortOrder(order);
		mFolderRoot.get()->scrollToShowSelection();
	}
}
U32 LLInventoryPanel::getSortOrder() const
{
	return mFolderRoot.get()->getSortOrder();
}
void LLInventoryPanel::requestSort()
{
	mFolderRoot.get()->requestSort();
}
void LLInventoryPanel::setSinceLogoff(BOOL sl)
{
	getFilter().setDateRangeLastLogoff(sl);
}
void LLInventoryPanel::setHoursAgo(U32 hours)
{
	getFilter().setHoursAgo(hours);
}
void LLInventoryPanel::setDateSearchDirection(U32 direction)
{
	getFilter().setDateSearchDirection(direction);
}
void LLInventoryPanel::setFilterLinks(LLInventoryFilter::EFilterLink filter_links)
{
	getFilter().setFilterLinks(filter_links);
}
void LLInventoryPanel::setShowFolderState(LLInventoryFilter::EFolderShow show)
{
	getFilter().setShowFolderState(show);
}
LLInventoryFilter::EFolderShow LLInventoryPanel::getShowFolderState()
{
	return getFilter().getShowFolderState();
}
void LLInventoryPanel::modelChanged(U32 mask)
{
	static LLTrace::BlockTimerStatHandle FTM_REFRESH("Inventory Refresh");
	LL_RECORD_BLOCK_TIME(FTM_REFRESH);
	if (!mViewsInitialized) return;
	const LLInventoryModel* model = getModel();
	if (!model) return;
	const LLInventoryModel::changed_items_t& changed_items = model->getChangedIDs();
	if (changed_items.empty()) return;
	for (LLInventoryModel::changed_items_t::const_iterator items_iter = changed_items.begin();
		 items_iter != changed_items.end();
		 ++items_iter)
	{
		const LLUUID& item_id = (*items_iter);
		const LLInventoryObject* model_item = model->getObject(item_id);
		LLFolderViewItem* view_item = getItemByID(item_id);
		LLFolderViewFolder* view_folder = NULL;
		if (model_item && view_item)
		{
			view_folder = dynamic_cast<LLFolderViewFolder*>(view_item);
		}
		if (mask & LLInventoryObserver::LABEL)
		{
			if (view_item)
			{
				LLInvFVBridge* bridge = (LLInvFVBridge*)view_item->getListener();
				if(bridge)
				{
					bridge->clearDisplayName();
					view_item->refresh();
				}
				view_item->nameOrDescriptionChanged();
				LLFolderViewFolder* parent = view_item->getParentFolder();
				if(parent)
				{
					parent->requestSort();
				}
			}
		}
		if ((mask & LLInventoryObserver::DESCRIPTION))
		{
			if (view_item)
			{
				view_item->nameOrDescriptionChanged();
			}
		}
		if (mask & LLInventoryObserver::REBUILD)
		{
			if (model_item && view_item)
			{
				const LLUUID& idp = view_item->getListener()->getUUID();
				view_item->destroyView();
				removeItemID(idp);
			}
			view_item = buildNewViews(item_id);
			view_folder = dynamic_cast<LLFolderViewFolder *>(view_item);
		}
		if (mask & LLInventoryObserver::INTERNAL)
		{
			if (view_item)
			{
				view_item->refresh();
			}
		}
		if (mask & LLInventoryObserver::SORT)
		{
			if (view_folder)
			{
				view_folder->requestSort();
			}
		}
		if (mask & (LLInventoryObserver::STRUCTURE |
					LLInventoryObserver::ADD |
					LLInventoryObserver::REMOVE))
		{
			if (model_item && !view_item)
			{
				buildNewViews(item_id);
				if (auto parent = getFolderByID(model_item->getParentUUID())) parent->requestSort();
				if(mFolderRoot.get()->getRoot()->needsAutoRename())
				{
					setSelection(item_id, FALSE);
				}
			}
			else if (model_item && view_item)
			{
				LLFolderViewFolder* old_parent = view_item->getParentFolder();
				if (view_item->getRoot() != view_item)
				{
					LLFolderViewFolder* new_parent = (LLFolderViewFolder*)getItemByID(model_item->getParentUUID());
					if (old_parent != new_parent)
					{
						if (new_parent != NULL)
						{
							view_item->getParentFolder()->extractItem(view_item);
							view_item->addToFolder(new_parent, mFolderRoot.get());
							addItemID(view_item->getListener()->getUUID(), view_item);
							if (mInventory)
							{
								const LLUUID trash_id = mInventory->findCategoryUUIDForType(LLFolderType::FT_TRASH);
								if (trash_id != model_item->getParentUUID() && (mask & LLInventoryObserver::INTERNAL) && new_parent->isOpen())
								{
									setSelection(item_id, FALSE);
								}
							}
						}
						else
						{
							removeItemID(view_item->getListener()->getUUID());
							view_item->destroyView();
						}
						old_parent->requestSort();
					}
				}
			}
			else if (!model_item && view_item)
			{
				LLFolderViewFolder* parent = view_item->getParentFolder();
				removeItemID(view_item->getListener()->getUUID());
				view_item->destroyView();
				if(parent)
				{
					parent->requestSort();
				}
			}
		}
	}
}
const LLUUID LLInventoryPanel::getRootFolderID() const
{
	LLUUID root_id;
	if (mFolderRoot.get() && mFolderRoot.get()->getListener())
	{
		root_id = mFolderRoot.get()->getListener()->getUUID();
	}
	else
	{
		if (mStartFolder.has("id"))
		{
			root_id = mStartFolder["id"];
		}
		else
		{
			LLStringExplicit label(mStartFolder["name"]);
			const LLFolderType::EType preferred_type = mStartFolder.has("type")
				? LLFolderType::lookup(mStartFolder["type"])
				: LLViewerFolderType::lookupTypeFromNewCategoryName(label);
			if ("LIBRARY" == label)
			{
				root_id = gInventory.getLibraryRootFolderID();
			}
			else if (preferred_type != LLFolderType::FT_NONE)
			{
				root_id = gInventory.findCategoryUUIDForType(preferred_type, false);
				if (root_id.isNull())
				{
					LL_WARNS() << "Could not find folder for " << mStartFolder << LL_ENDL;
					root_id.generateNewID();
				}
			}
		}
	}
	return root_id;
}
void LLInventoryPanel::onIdle(void *userdata)
{
	if (!gInventory.isInventoryUsable())
		return;
	LLInventoryPanel *self = (LLInventoryPanel*)userdata;
	if (!self->mViewsInitialized)
	{
		self->initializeViews();
	}
	if (self->mViewsInitialized)
	{
		gIdleCallbacks.deleteFunction(onIdle, (void*)self);
	}
}
void LLInventoryPanel::initializeViews()
{
	if (!gInventory.isInventoryUsable()) return;
	rebuildViewsFor(getRootFolderID());
	mViewsInitialized = true;
	openStartFolderOrMyInventory();
	if (gAgent.isFirstLogin())
	{
		LLFolderViewFolder* lib_folder = getFolderByID(gInventory.getLibraryRootFolderID());
		if (lib_folder)
		{
			lib_folder->setOpen(TRUE);
		}
		LLFolderViewFolder* my_inv_folder = getFolderByID(gInventory.getRootFolderID());
		if (my_inv_folder)
		{
			my_inv_folder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);
		}
	}
}
LLFolderViewItem* LLInventoryPanel::rebuildViewsFor(const LLUUID& id)
{
	LLFolderViewItem* old_view = mFolderRoot.get()->getItemByID(id);
	if (old_view)
	{
		old_view->destroyView();
	}
	return buildNewViews(id);
}
LLFolderView * LLInventoryPanel::createFolderView(LLInvFVBridge * bridge, bool useLabelSuffix)
{
	LLRect folder_rect(0,
					   0,
					   getRect().getWidth(),
					   0);
	LLFolderView* ret = new LLFolderView(
	getName(),
	folder_rect,
	LLUUID::null,
	this,
	bridge,
	mGroupedItemBridge);
	ret->setAllowMultiSelect(mAllowMultiSelect);
	ret->setShowEmptyMessage(mShowEmptyMessage);
	return ret;
}
LLFolderViewFolder * LLInventoryPanel::createFolderViewFolder(LLInvFVBridge* bridge)
{
	return new LLFolderViewFolder(
		bridge->getDisplayName(),
		bridge->getIcon(),
		bridge->getIconOpen(),
		LLUI::getUIImage("inv_link_overlay.tga"),
		mFolderRoot.get(),
		bridge);
}
LLFolderViewItem * LLInventoryPanel::createFolderViewItem(LLInvFVBridge * bridge)
{
	return new LLFolderViewItem(
		bridge->getDisplayName(),
		bridge->getIcon(),
		bridge->getIconOpen(),
		LLUI::getUIImage("inv_link_overlay.tga"),
		bridge->getCreationDate(),
		mFolderRoot.get(),
		bridge);
}
LLFolderViewItem* LLInventoryPanel::buildNewViews(const LLUUID& id)
{
	LLInventoryObject const* objectp = gInventory.getObject(id);
	LLFolderViewFolder* parent_folder = mFolderRoot.get();
	LLUUID root_id = parent_folder->getListener()->getUUID();
	LLFolderViewItem* folder_view_item = nullptr;
	if (id != root_id && !objectp)
	{
		return NULL;
	}
	if (objectp)
	{
	folder_view_item = getItemByID(id);
	const LLUUID& parent_id = objectp->getParentUUID();
	parent_folder = (LLFolderViewFolder*)getItemByID(parent_id);
	bool allow_drop = true;
	bool create_root = false;
	if (mShowRootFolder)
	{
		LLUUID root_id = getRootFolderID();
		if (root_id == id)
		{
			parent_folder = dynamic_cast<LLFolderViewFolder*>(folder_view_item);
			folder_view_item = NULL;
			allow_drop = mAllowDropOnRoot;
			create_root = true;
		}
	}
	if (!folder_view_item && parent_folder)
	{
		if (objectp->getType() <= LLAssetType::AT_NONE ||
			objectp->getType() >= LLAssetType::AT_COUNT)
		{
			LL_WARNS() << "LLInventoryPanel::buildNewViews called with invalid objectp->mType : "
					<< ((S32) objectp->getType()) << " name " << objectp->getName() << " UUID " << objectp->getUUID()
					<< LL_ENDL;
			return NULL;
		}
		if ((objectp->getType() == LLAssetType::AT_CATEGORY) &&
			(objectp->getActualType() != LLAssetType::AT_LINK_FOLDER))
		{
			LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(LLAssetType::AT_CATEGORY,
											(mUseMarketplaceFolders ? LLAssetType::AT_MARKETPLACE_FOLDER : LLAssetType::AT_CATEGORY),
											LLInventoryType::IT_CATEGORY,
											this,
											mFolderRoot.get(),
											objectp->getUUID());
			if (new_listener)
			{
				folder_view_item = createFolderViewFolder(new_listener);
				if (folder_view_item)
				{
					static_cast<LLFolderViewFolder*>(folder_view_item)->setItemSortOrder(mFolderRoot.get()->getSortOrder());
					folder_view_item->setAllowDrop(allow_drop);
					folder_view_item->setAllowWear(mAllowWear);
				}
			}
		}
		else
		{
			LLInventoryItem* item = (LLInventoryItem*)objectp;
			LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(item->getType(),
			item->getActualType(),
			item->getInventoryType(),
																			this,
																			mFolderRoot.get(),
																			item->getUUID(),
																			item->getFlags());
			if (new_listener)
			{
				folder_view_item = createFolderViewItem(new_listener);
			}
		}
		if (folder_view_item)
		{
			llassert(parent_folder != NULL);
			folder_view_item->addToFolder(parent_folder, mFolderRoot.get());
			addItemID(id, folder_view_item);
			if (create_root)
			{
				folder_view_item->setOpen(TRUE);
			}
		}
	}
	}
	if (id.isNull()
		|| (folder_view_item && objectp->getType() == LLAssetType::AT_CATEGORY))
	{
		LLViewerInventoryCategory::cat_array_t* categories;
		LLViewerInventoryItem::item_array_t* items;
		mInventory->lockDirectDescendentArrays(id, categories, items);
		if(categories)
		{
			for (LLViewerInventoryCategory::cat_array_t::const_iterator cat_iter = categories->begin();
				 cat_iter != categories->end();
				 ++cat_iter)
			{
				const LLViewerInventoryCategory* cat = (*cat_iter);
				buildNewViews(cat->getUUID());
			}
		}
		if(items)
		{
			for (LLViewerInventoryItem::item_array_t::const_iterator item_iter = items->begin();
				 item_iter != items->end();
				 ++item_iter)
			{
				const LLViewerInventoryItem* item = (*item_iter);
				buildNewViews(item->getUUID());
			}
		}
		mInventory->unlockDirectDescendentArrays(id);
	}
	return folder_view_item;
}
void LLInventoryPanel::openStartFolderOrMyInventory()
{
	for (LLView *child = mFolderRoot.get()->getFirstChild(); child; child = mFolderRoot.get()->findNextSibling(child))
	{
		LLFolderViewFolder *fchild = dynamic_cast<LLFolderViewFolder*>(child);
		if (fchild
			&& fchild->getListener()
				&& fchild->getListener()->getUUID() == gInventory.getRootFolderID())
		{
			fchild->setOpen(TRUE);
			break;
		}
	}
}
class LLOpenFolderByID : public LLFolderViewFunctor
{
public:
	LLOpenFolderByID(const LLUUID& id) : mID(id) {}
	virtual ~LLOpenFolderByID() {}
	virtual void doFolder(LLFolderViewFolder* folder)
		{
			if (folder->getListener() && folder->getListener()->getUUID() == mID) folder->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
	virtual void doItem(LLFolderViewItem* item) {}
protected:
	const LLUUID& mID;
};
void LLInventoryPanel::onItemsCompletion()
{
	if (mFolderRoot.get()) mFolderRoot.get()->updateMenu();
}
void LLInventoryPanel::openSelected()
{
	LLFolderViewItem* folder_item = mFolderRoot.get()->getCurSelectedItem();
	if(!folder_item) return;
	LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getListener();
	if(!bridge) return;
	bridge->openItem();
}
void LLInventoryPanel::unSelectAll()
{
	mFolderRoot.get()->setSelection(NULL, FALSE, FALSE);
}
BOOL LLInventoryPanel::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleHover(x, y, mask);
	if(handled)
	{
		ECursorType cursor = getWindow()->getCursor();
		if (LLInventoryModelBackgroundFetch::instance().folderFetchActive() && cursor == UI_CURSOR_ARROW)
		{
			getWindow()->setCursor(UI_CURSOR_WORKING);
		}
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
	}
	return TRUE;
}
BOOL LLInventoryPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	{
		handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		if (!handled && mAllowDropOnRoot && !mFolderRoot.get()->hasVisibleChildren())
		{
			handled = mFolderRoot.get()->handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		}
		if (handled)
		{
			mFolderRoot.get()->setDragAndDropThisFrame();
		}
	}
	return handled;
}
void LLInventoryPanel::onFocusLost()
{
	if (LLEditMenuHandler::gEditMenuHandler == mFolderRoot.get())
	{
		LLEditMenuHandler::gEditMenuHandler = NULL;
	}
	LLPanel::onFocusLost();
}
void LLInventoryPanel::onFocusReceived()
{
	LLEditMenuHandler::gEditMenuHandler = mFolderRoot.get();
	LLPanel::onFocusReceived();
}
void LLInventoryPanel::openAllFolders()
{
	mFolderRoot.get()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_DOWN);
	mFolderRoot.get()->arrangeAll();
}
void LLInventoryPanel::closeAllFolders()
{
	mFolderRoot.get()->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);
	mFolderRoot.get()->arrangeAll();
}
void LLInventoryPanel::openDefaultFolderForType(LLAssetType::EType type)
{
	LLUUID category_id = mInventory->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(type));
	LLOpenFolderByID opener(category_id);
	mFolderRoot.get()->applyFunctorRecursively(opener);
}
void LLInventoryPanel::setSelection(const LLUUID& obj_id, BOOL take_keyboard_focus)
{
	const LLInventoryObject *obj = gInventory.getObject(obj_id);
	if (obj && obj->getParentUUID() == LLAppearanceMgr::instance().getCOF())
	{
		return;
	}
	setSelectionByID(obj_id, take_keyboard_focus);
}
void LLInventoryPanel::setSelectCallback(const std::function<void (const std::deque<LLFolderViewItem*>& items, BOOL user_action)>& cb)
{
	if (mFolderRoot.get())
	{
		mFolderRoot.get()->setSelectCallback(cb);
	}
}
void LLInventoryPanel::clearSelection()
{
	mFolderRoot.get()->clearSelection();
	mSelectThisID.setNull();
}
void LLInventoryPanel::onSelectionChange(const std::deque<LLFolderViewItem*>& items, BOOL user_action)
{
	mCompletionObserver->reset();
	for (std::deque<LLFolderViewItem*>::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		LLUUID id = (*it)->getListener()->getUUID();
		LLViewerInventoryItem* inv_item = mInventory->getItem(id);
		if (inv_item && !inv_item->isFinished())
		{
			mCompletionObserver->watchItem(id);
		}
	}
	LLFolderView* fv = mFolderRoot.get();
	if (fv->needsAutoRename())
	{
		fv->setNeedsAutoRename(FALSE);
		if (items.size())
		{
			fv->startRenamingSelectedItem();
		}
	}
}
void LLInventoryPanel::createNewItem(const std::string& name,
									const LLUUID& parent_id,
									LLAssetType::EType asset_type,
									LLInventoryType::EType inv_type,
									U32 next_owner_perm)
{
	std::string desc;
	LLViewerAssetType::generateDescriptionFor(asset_type, desc);
	next_owner_perm = (next_owner_perm) ? next_owner_perm : PERM_MOVE | PERM_TRANSFER;
	if (inv_type == LLInventoryType::IT_GESTURE)
	{
		LLPointer<LLInventoryCallback> cb =  new LLBoostFuncInventoryCallback(create_gesture_cb);
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							  parent_id, LLTransactionID::tnull, name, desc, asset_type, inv_type,
							  NOT_WEARABLE, next_owner_perm, cb);
	}
	else
	{
		LLPointer<LLInventoryCallback> cb = NULL;
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							  parent_id, LLTransactionID::tnull, name, desc, asset_type, inv_type,
							  NOT_WEARABLE, next_owner_perm, cb);
	}
}
BOOL LLInventoryPanel::getSinceLogoff()
{
	return getFilter().isSinceLogoff();
}
void LLInventoryPanel::dumpSelectionInformation(void* user_data)
{
	LLInventoryPanel* iv = (LLInventoryPanel*)user_data;
	iv->mFolderRoot.get()->dumpSelectionInformation();
}
LLInventoryPanel* LLInventoryPanel::getActiveInventoryPanel(BOOL auto_open)
{
	LLInventoryPanel* res = NULL;
	LLPanelMainInventory* floater_inventory = LLPanelMainInventory::getActiveInventory();
	if (!floater_inventory)
	{
		LL_WARNS() << "Could not find My Inventory floater" << LL_ENDL;
		return FALSE;
	}
	res = floater_inventory ? floater_inventory->getActivePanel() : NULL;
	if (res)
	{
		if (floater_inventory && floater_inventory->isMinimized())
		{
			floater_inventory->setMinimized(FALSE);
		}
	}
	else if (auto_open)
	{
		floater_inventory->open();
		res = floater_inventory->getActivePanel();
	}
	return res;
}
void LLInventoryPanel::addHideFolderType(LLFolderType::EType folder_type)
{
	getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() & ~(1ULL << folder_type));
}
BOOL LLInventoryPanel::getIsHiddenFolderType(LLFolderType::EType folder_type) const
{
	return !(getFilter().getFilterCategoryTypes() & (1ULL << folder_type));
}
void LLInventoryPanel::addItemID( const LLUUID& id, LLFolderViewItem*   itemp )
{
	mItemMap[id] = itemp;
}
void LLInventoryPanel::removeItemID(const LLUUID& id)
{
	LLInventoryModel::cat_array_t categories;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(id, categories, items, TRUE);
	mItemMap.erase(id);
	for (LLInventoryModel::cat_array_t::iterator it = categories.begin(),    end_it = categories.end();
		it != end_it;
		++it)
	{
		mItemMap.erase((*it)->getUUID());
}
	for (LLInventoryModel::item_array_t::iterator it = items.begin(),   end_it  = items.end();
		it != end_it;
		++it)
	{
		mItemMap.erase((*it)->getUUID());
	}
}
LLFolderViewItem* LLInventoryPanel::getItemByID(const LLUUID& id)
{
	return mFolderRoot.get()->getItemByID(id);
}
LLFolderViewFolder* LLInventoryPanel::getFolderByID(const LLUUID& id)
{
	return mFolderRoot.get()->getFolderByID(id);
}
void LLInventoryPanel::setSelectionByID( const LLUUID& obj_id, BOOL    take_keyboard_focus )
{
	mFolderRoot.get()->setSelectionByID(obj_id, take_keyboard_focus);
}
void LLInventoryPanel::updateSelection()
{
	mFolderRoot.get()->updateSelection();
}
namespace LLInventoryAction
{
	bool doToSelected(LLFolderView* root, std::string action, BOOL user_confirm = TRUE);
}
void LLInventoryPanel::doToSelected(const LLSD& userdata)
{
	LLInventoryAction::doToSelected(mFolderRoot.get(), userdata.asString());
	return;
}
BOOL LLInventoryPanel::handleKeyHere( KEY key, MASK mask )
{
	return LLPanel::handleKeyHere(key, mask);
}
bool LLInventoryPanel::isSelectionRemovable()
{
	bool can_delete = false;
	if (mFolderRoot.get())
	{
		auto selection_set = mFolderRoot.get()->getSelectionList();
		if (!selection_set.empty())
		{
			can_delete = true;
			for (const auto& id : selection_set)
			{
				LLFolderViewItem *item = getItemByID(id);
				const LLFolderViewEventListener* listener =item->getListener();
				if (!listener)
				{
					can_delete = false;
				}
				else
				{
					can_delete &= listener->isItemRemovable() && !listener->isItemInTrash();
				}
			}
		}
	}
	return can_delete;
}
class LLInventoryRecentItemsPanel;
static const LLRecentInventoryBridgeBuilder RECENT_ITEMS_BUILDER;
LLInventoryRecentItemsPanel:: LLInventoryRecentItemsPanel(const std::string& name,
						    		const std::string& sort_order_setting,
									const LLSD& start_folder,
									const LLRect& rect,
									LLInventoryModel* inventory,
									BOOL allow_multi_select,
									LLView *parent_view) :
									LLInventoryPanel(name, sort_order_setting,start_folder,rect,inventory,allow_multi_select,parent_view)
{
	mInvFVBridgeBuilder = &RECENT_ITEMS_BUILDER;
}
