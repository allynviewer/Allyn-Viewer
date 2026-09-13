/** 
 *
 * Copyright (c) 2009-2014, Kitty Barnett
 * 
 * The source code in this file is provided to you under the terms of the 
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 * 
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to 
 * abide by those obligations.
 * 
 */
#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "llappearancemgr.h"
#include "llstartup.h"
#include "llviewerfoldertype.h"
#include "rlvinventory.h"
#include "boost/algorithm/string.hpp"
const std::string RlvInventory::cstrSharedRoot = RLV_ROOT_FOLDER;
class RlvSharedInventoryFetcher : public LLInventoryFetchDescendentsObserver
{
public:
	RlvSharedInventoryFetcher(const uuid_vec_t& idFolders): LLInventoryFetchDescendentsObserver(idFolders) {}
	virtual ~RlvSharedInventoryFetcher() {}
	virtual void done()
	{
		RLV_INFOS << "Shared folders fetch completed" << LL_ENDL;
		RlvInventory::instance().m_fFetchComplete = true;
		RlvInventory::instance().fetchSharedLinks();
		gInventory.removeObserver(this);
		delete this;
	}
};
RlvInventory::RlvInventory()
	: m_fFetchStarted(false), m_fFetchComplete(false)
{
}
RlvInventory::~RlvInventory()
{
	if (gInventory.containsObserver(this))
		gInventory.removeObserver(this);
}
void RlvInventory::changed(U32 mask)
{
	const LLInventoryModel::changed_items_t& idsChanged = gInventory.getChangedIDs();
	if (std::find(idsChanged.begin(), idsChanged.end(), m_idRlvRoot) != idsChanged.end())
	{
		gInventory.removeObserver(this);
		LLUUID idRlvRootPrev = m_idRlvRoot;
		m_idRlvRoot.setNull();
		if (idRlvRootPrev != getSharedRootID())
			m_OnSharedRootIDChanged();
	}
}
void RlvInventory::fetchSharedInventory()
{
	const LLViewerInventoryCategory* pRlvRoot = getSharedRoot();
	if ( (m_fFetchStarted) || (!pRlvRoot) )
		return;
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(pRlvRoot->getUUID(), folders, items, FALSE);
	uuid_vec_t idFolders;
	idFolders.push_back(pRlvRoot->getUUID());
	for (S32 idxFolder = 0, cntFolder = folders.size(); idxFolder < cntFolder; idxFolder++)
		idFolders.push_back(folders.at(idxFolder)->getUUID());
	RlvSharedInventoryFetcher* pFetcher = new RlvSharedInventoryFetcher(idFolders);
	RLV_INFOS << "Starting fetch of " << idFolders.size() << " shared folders" << RLV_ENDL;
	pFetcher->startFetch();
	m_fFetchStarted = true;
	if (pFetcher->isFinished())
		pFetcher->done();
	else
		gInventory.addObserver(pFetcher);
}
void RlvInventory::fetchSharedLinks()
{
	const LLViewerInventoryCategory* pRlvRoot = getSharedRoot();
	if (!pRlvRoot)
		return;
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items; RlvIsLinkType f;
	gInventory.collectDescendentsIf(pRlvRoot->getUUID(), folders, items, FALSE, f, false);
	uuid_vec_t idFolders, idItems;
	for (S32 idxItem = 0, cntItem = items.size(); idxItem < cntItem; idxItem++)
	{
		const LLViewerInventoryItem* pItem = items.at(idxItem);
		switch (pItem->getActualType())
		{
			case LLAssetType::AT_LINK:
				idItems.push_back(pItem->getLinkedUUID());
				break;
			case LLAssetType::AT_LINK_FOLDER:
				idFolders.push_back(pItem->getLinkedUUID());
				break;
			default:
				break;;
		}
	}
	RLV_INFOS << "Starting link target fetch of " << idItems.size() << " items and " << idFolders.size() << " folders" << RLV_ENDL;
	LLInventoryFetchItemsObserver itemFetcher(idItems);
	itemFetcher.startFetch();
}
void RlvInventory::fetchWornItems()
{
	uuid_vec_t idItems;
	for (int type = 0; type < LLWearableType::WT_COUNT; type++)
	{
		const LLUUID& idItem = gAgentWearables.getWearableItemID((LLWearableType::EType)type, 0);
		if (idItem.notNull())
			idItems.push_back(idItem);
	}
	if (isAgentAvatarValid())
	{
		for (LLVOAvatar::attachment_map_t::const_iterator itAttachPt = gAgentAvatarp->mAttachmentPoints.begin();
				itAttachPt != gAgentAvatarp->mAttachmentPoints.end(); ++itAttachPt)
		{
			const LLViewerJointAttachment* pAttachPt = itAttachPt->second;
			if (pAttachPt)
			{
				for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
					 itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
				{
					const LLViewerObject* pAttachObj = (*itAttachObj);
					if ( (pAttachObj) && (pAttachObj->getAttachmentItemID().notNull()) )
						idItems.push_back(pAttachObj->getAttachmentItemID());
				}
			}
		}
	}
	LLInventoryFetchItemsObserver itemFetcher(idItems);
	itemFetcher.startFetch();
}
bool RlvInventory::findSharedFolders(const std::string& strCriteria, LLInventoryModel::cat_array_t& folders) const
{
	const LLViewerInventoryCategory* pRlvRoot = RlvInventory::instance().getSharedRoot();
	if (!pRlvRoot)
		return false;
	folders.clear();
	LLInventoryModel::item_array_t items;
	RlvCriteriaCategoryCollector f(strCriteria);
	gInventory.collectDescendentsIf(pRlvRoot->getUUID(), folders, items, FALSE, f);
	return (folders.size() != 0);
}
bool RlvInventory::getPath(const uuid_vec_t& idItems, LLInventoryModel::cat_array_t& folders) const
{
	const LLViewerInventoryCategory* pRlvRoot = RlvInventory::instance().getSharedRoot();
	if (!pRlvRoot)
		return false;
	folders.clear();
	for (uuid_vec_t::const_iterator itItem = idItems.begin(); itItem != idItems.end(); ++itItem)
	{
		const LLInventoryItem* pItem = gInventory.getItem(*itItem);
		if ( (pItem) && (gInventory.isObjectDescendentOf(pItem->getUUID(), pRlvRoot->getUUID())) )
		{
			LLViewerInventoryCategory* pFolder = gInventory.getCategory(pItem->getParentUUID());
			if (RlvInventory::instance().isFoldedFolder(pFolder, true))
				pFolder = gInventory.getCategory(pFolder->getParentUUID());
			folders.push_back(pFolder);
		}
	}
	return (folders.size() != 0);
}
const LLUUID& RlvInventory::getSharedRootID() const
{
	if ( (m_idRlvRoot.isNull()) && (gInventory.isInventoryUsable()) )
	{
		LLInventoryModel::cat_array_t* pFolders; LLInventoryModel::item_array_t* pItems;
		gInventory.getDirectDescendentsOf(gInventory.getRootFolderID(), pFolders, pItems);
		if (pFolders)
		{
			const LLViewerInventoryCategory* pFolder;
			for (S32 idxFolder = 0, cntFolder = pFolders->size(); idxFolder < cntFolder; idxFolder++)
			{
				if ( ((pFolder = pFolders->at(idxFolder)) != NULL) && (cstrSharedRoot == pFolder->getName()) )
				{
					m_idRlvRoot = pFolder->getUUID();
					if (getDirectDescendentsFolderCount(pFolder) > 0)
						break;
				}
			}
			if ( (m_idRlvRoot.notNull()) && (!gInventory.containsObserver((RlvInventory*)this)) )
				gInventory.addObserver((RlvInventory*)this);
		}
	}
	return m_idRlvRoot;
}
LLViewerInventoryCategory* RlvInventory::getSharedFolder(const LLUUID& idParent, const std::string& strFolderName, bool fMatchPartial) const
{
	LLInventoryModel::cat_array_t* pFolders; LLInventoryModel::item_array_t* pItems;
	gInventory.getDirectDescendentsOf(idParent, pFolders, pItems);
	if ( (!pFolders) || (strFolderName.empty()) )
		return NULL;
	LLViewerInventoryCategory* pPartial = NULL;
	for (LLInventoryModel::cat_array_t::const_iterator itFolder = pFolders->begin(); itFolder != pFolders->end(); ++itFolder)
	{
		LLViewerInventoryCategory* pFolder = *itFolder;
		const std::string& strName = pFolder->getName();
		if (boost::iequals(strName, strFolderName))
			return pFolder;
		else if ( (fMatchPartial) && (!pPartial) && (RLV_FOLDER_PREFIX_HIDDEN != strName[0]) && (boost::icontains(strName, strFolderName)) )
			pPartial = pFolder;
	}
	return pPartial;
}
LLViewerInventoryCategory* RlvInventory::getSharedFolder(const std::string& strPath, bool fMatchPartial) const
{
	LLViewerInventoryCategory* pRlvRoot = getSharedRoot(), *pFolder = pRlvRoot;
	if (!pRlvRoot)
		return NULL;
	boost_tokenizer tokens(strPath, boost::char_separator<char>("/", "", boost::drop_empty_tokens));
	for (boost_tokenizer::const_iterator itToken = tokens.begin(); itToken != tokens.end(); ++itToken)
	{
		pFolder = getSharedFolder(pFolder->getUUID(), *itToken, fMatchPartial);
		if (!pFolder)
			return NULL;
	}
	return pFolder;
}
std::string RlvInventory::getSharedPath(const LLViewerInventoryCategory* pFolder) const
{
	const LLViewerInventoryCategory* pRlvRoot = getSharedRoot();
	if ( (!pRlvRoot) || (!pFolder) || (pRlvRoot->getUUID() == pFolder->getUUID()) )
		return std::string();
	const LLUUID& idRLV  = pRlvRoot->getUUID();
	const LLUUID& idRoot = gInventory.getRootFolderID();
	std::string strPath;
	RLV_ASSERT(gInventory.isObjectDescendentOf(pFolder->getUUID(), pRlvRoot->getUUID()));
	while (pFolder)
	{
		strPath = "/" + pFolder->getName() + strPath;
		const LLUUID& idParent = pFolder->getParentUUID();
		if (idRLV == idParent)
			break;
		else if (idRoot == idParent)
			return std::string();
		pFolder = gInventory.getCategory(idParent);
	}
	return strPath.erase(0, 1);
}
S32 RlvInventory::getDirectDescendentsFolderCount(const LLInventoryCategory* pFolder)
{
	LLInventoryModel::cat_array_t* pFolders = NULL; LLInventoryModel::item_array_t* pItems = NULL;
	if (pFolder)
		gInventory.getDirectDescendentsOf(pFolder->getUUID(), pFolders, pItems);
	return (pFolders) ? pFolders->size() : 0;
}
S32 RlvInventory::getDirectDescendentsItemCount(const LLInventoryCategory* pFolder, LLAssetType::EType filterType)
{
	S32 cntType = 0;
	if (pFolder)
	{
		LLInventoryModel::cat_array_t* pFolders; LLInventoryModel::item_array_t* pItems;
		gInventory.getDirectDescendentsOf(pFolder->getUUID(), pFolders, pItems);
		if (pItems)
		{
			for (S32 idxItem = 0, cntItem = pItems->size(); idxItem < cntItem; idxItem++)
				if (pItems->at(idxItem)->getType() == filterType)
					cntType++;
		}
	}
	return cntType;
}
void RlvRenameOnWearObserver::done()
{
	gInventory.removeObserver(this);
	doOnIdleOneTime(boost::bind(&RlvRenameOnWearObserver::doneIdle, this));
}
void RlvRenameOnWearObserver::doneIdle()
{
	const LLViewerInventoryCategory* pRlvRoot = NULL;
	if ( (RlvSettings::getEnableSharedWear()) || (!RlvSettings::getSharedInvAutoRename()) || (LLStartUp::getStartupState() < STATE_STARTED) ||
		 (!isAgentAvatarValid()) || ((pRlvRoot = RlvInventory::instance().getSharedRoot()) == NULL) )
	{
		delete this;
		return;
	}
	const LLViewerJointAttachment* pAttachPt = NULL; S32 idxAttachPt = 0;
	for (uuid_vec_t::const_iterator itItem = mComplete.begin(); itItem != mComplete.end(); ++itItem)
	{
		const LLUUID& idAttachItem = *itItem;
		LLInventoryModel::item_array_t items;
		if (gInventory.isObjectDescendentOf(idAttachItem, pRlvRoot->getUUID()))
			items.push_back(gInventory.getItem(idAttachItem));
		if (items.empty())
			continue;
		if ( ((pAttachPt = gAgentAvatarp->getWornAttachmentPoint(idAttachItem)) == NULL) ||
			 ((idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(pAttachPt)) == 0) )
		{
			RLV_ASSERT(false);
			continue;
		}
		for (S32 idxItem = 0, cntItem = items.size(); idxItem < cntItem; idxItem++)
		{
			LLViewerInventoryItem* pItem = items.at(idxItem);
			if (!pItem)
				continue;
			S32 idxAttachPtItem = RlvAttachPtLookup::getAttachPointIndex(pItem);
			if ( (idxAttachPt == idxAttachPtItem) || (idxAttachPtItem) )
				continue;
			std::string strAttachPt = pAttachPt->getName();
			LLStringUtil::toLower(strAttachPt);
			if (pItem->getPermissions().allowModifyBy(gAgent.getID()))
			{
				std::string strName = pItem->getName();
				LLStringUtil::truncate(strName, DB_INV_ITEM_NAME_STR_LEN - strAttachPt.length() - 3);
				strName += " (" + strAttachPt + ")";
				pItem->rename(strName);
				pItem->updateServer(FALSE);
				gInventory.addChangedMask(LLInventoryObserver::LABEL, pItem->getUUID());
			}
			else
			{
				LLViewerInventoryCategory* pFolder = gInventory.getCategory(pItem->getParentUUID());
				if ( (pFolder) && (pFolder->getUUID() != pRlvRoot->getUUID()) && (!RlvInventory::isFoldedFolder(pFolder, false)) )
				{
					std::string strFolderName = ".(" + strAttachPt + ")";
					if ( (LLViewerFolderType::lookupNewCategoryName(LLFolderType::FT_NONE) == pFolder->getName()) &&
						 (pFolder->getParentUUID() != pRlvRoot->getUUID()) &&
						 (1 == RlvInventory::getDirectDescendentsItemCount(pFolder, LLAssetType::AT_OBJECT)) )
					{
						pFolder->rename(strFolderName);
						pFolder->updateServer(FALSE);
						gInventory.addChangedMask(LLInventoryObserver::LABEL, pFolder->getUUID());
					}
					else
					{
						inventory_func_type func = boost::bind(&RlvRenameOnWearObserver::onCategoryCreate, this, _1, pItem->getUUID());
						LLUUID idFolder = gInventory.createNewCategory(pFolder->getUUID(), LLFolderType::FT_NONE, strFolderName, func);
						if (idFolder.notNull())
						{
							RlvRenameOnWearObserver::onCategoryCreate(idFolder, pItem->getUUID());
						}
					}
				}
			}
		}
	}
	gInventory.notifyObservers();
	delete this;
}
void RlvRenameOnWearObserver::onCategoryCreate(const LLUUID& folder_id, const LLUUID& item_id)
{
	if (folder_id.notNull() && item_id.notNull())
		move_inventory_item(gAgent.getID(), gAgent.getSessionID(), item_id, folder_id, std::string(), NULL);
}
bool RlvGiveToRLVOffer::createDestinationFolder(const std::string& strPath)
{
	m_DestPath.clear();
	if (0 == strPath.find(RLV_PUTINV_PREFIX))
	{
		boost::split(m_DestPath, strPath, boost::is_any_of(std::string(RLV_PUTINV_SEPARATOR)));
	}
	if ( (m_DestPath.size() >= 2) && (m_DestPath.size() <= RLV_PUTINV_MAXDEPTH) )
	{
		const std::string strFolder = m_DestPath.front();
		if (RLV_ROOT_FOLDER == strFolder)
		{
			m_DestPath.pop_front();
			const LLUUID& idRlvRoot = RlvInventory::instance().getSharedRootID();
			if (idRlvRoot.notNull())
			{
				onCategoryCreate(idRlvRoot);
			}
			else
			{
				inventory_func_type func = boost::bind(&RlvGiveToRLVOffer::onCategoryCreate, this, _1);
				const LLUUID idTemp = gInventory.createNewCategory(gInventory.getRootFolderID(), LLFolderType::FT_NONE, RLV_ROOT_FOLDER, func);
				if (idTemp.notNull())
					onCategoryCreate(idTemp);
			}
			return true;
		}
	}
	m_DestPath.clear();
	return false;
}
void RlvGiveToRLVOffer::onCategoryCreate(const LLUUID& folder_id)
{
	if (folder_id.isNull())
	{
		onDestinationCreated(LLUUID::null, LLStringUtil::null);
		return;
	}
	LLUUID target_folder = folder_id;
	while (m_DestPath.size() > 1)
	{
		std::string strFolder = m_DestPath.front();
		m_DestPath.pop_front();
		const LLViewerInventoryCategory* pFolder = RlvInventory::instance().getSharedFolder(folder_id, strFolder, false);
		if (pFolder)
		{
			target_folder = pFolder->getUUID();
		}
		else
		{
			LLInventoryObject::correctInventoryName(strFolder);
			inventory_func_type func = boost::bind(&RlvGiveToRLVOffer::onCategoryCreate, this, _1);
			const LLUUID idTemp = gInventory.createNewCategory(folder_id, LLFolderType::FT_NONE, strFolder, func);
			if (idTemp.notNull())
				onCategoryCreate(idTemp);
			return;
		}
	}
	onDestinationCreated(target_folder, m_DestPath.front());
}
void RlvGiveToRLVOffer::moveAndRename(const LLUUID& idFolder, const LLUUID& idDestination, const std::string& strName)
{
	const LLViewerInventoryCategory* pDest = gInventory.getCategory(idDestination);
	const LLViewerInventoryCategory* pFolder = gInventory.getCategory(idFolder);
	if ( (pDest) && (pFolder) )
	{
		LLPointer<LLViewerInventoryCategory> pNewFolder = new LLViewerInventoryCategory(pFolder);
		if (pDest->getUUID() != pFolder->getParentUUID())
		{
			LLInventoryModel::update_list_t update;
			LLInventoryModel::LLCategoryUpdate updOldParent(pFolder->getParentUUID(), -1);
			update.push_back(updOldParent);
			LLInventoryModel::LLCategoryUpdate updNewParent(pDest->getUUID(), 1);
			update.push_back(updNewParent);
			gInventory.accountForUpdate(update);
			pNewFolder->setParent(pDest->getUUID());
			pNewFolder->updateParentOnServer(FALSE);
		}
		pNewFolder->rename(strName);
		pNewFolder->updateServer(FALSE);
		gInventory.updateCategory(pNewFolder);
		gInventory.notifyObservers();
	}
}
void RlvGiveToRLVTaskOffer::changed(U32 mask)
{
	if (mask & LLInventoryObserver::ADD)
	{
		LLMessageSystem* pMsg = gMessageSystem;
		if ( (pMsg->getMessageName()) && (0 == strcmp(pMsg->getMessageName(), "BulkUpdateInventory")) )
		{
			LLUUID idTransaction;
			pMsg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, idTransaction);
			if (m_idTransaction == idTransaction)
			{
				LLUUID idInvObject;
				for (S32 idxBlock = 0, cntBlock = pMsg->getNumberOfBlocksFast(_PREHASH_FolderData); idxBlock < cntBlock; idxBlock++)
				{
					pMsg->getUUIDFast(_PREHASH_FolderData, _PREHASH_FolderID, idInvObject, idxBlock);
					if ( (idInvObject.notNull()) && (std::find(m_Folders.begin(), m_Folders.end(), idInvObject) == m_Folders.end()) )
						m_Folders.push_back(idInvObject);
				}
				done();
			}
		}
	}
}
void RlvGiveToRLVTaskOffer::done()
{
	gInventory.removeObserver(this);
	doOnIdleOneTime(boost::bind(&RlvGiveToRLVTaskOffer::doneIdle, this));
}
void RlvGiveToRLVTaskOffer::doneIdle()
{
	const LLViewerInventoryCategory* pFolder = (m_Folders.size()) ? gInventory.getCategory(m_Folders.front()) : NULL;
	if ( (!pFolder) || (!createDestinationFolder(pFolder->getName())) )
		delete this;
}
void RlvGiveToRLVTaskOffer::onDestinationCreated(const LLUUID& idFolder, const std::string& strName)
{
	const LLViewerInventoryCategory* pTarget = (idFolder.notNull()) ? gInventory.getCategory(idFolder) : NULL;
	if (pTarget)
	{
		const LLUUID& idOfferedFolder = m_Folders.front();
		moveAndRename(idOfferedFolder, idFolder, strName);
		RlvBehaviourNotifyHandler::sendNotification("accepted_in_rlv inv_offer " + RlvInventory::instance().getSharedPath(idOfferedFolder));
	}
	delete this;
}
void RlvGiveToRLVAgentOffer::done()
{
	gInventory.removeObserver(this);
	doOnIdleOneTime(boost::bind(&RlvGiveToRLVAgentOffer::doneIdle, this));
}
void RlvGiveToRLVAgentOffer::doneIdle()
{
	const LLViewerInventoryCategory* pFolder = (mComplete.size()) ? gInventory.getCategory(mComplete.front()) : NULL;
	if ( (!pFolder) || (!createDestinationFolder(pFolder->getName())) )
		delete this;
}
void RlvGiveToRLVAgentOffer::onDestinationCreated(const LLUUID& idFolder, const std::string& strName)
{
	if ( (idFolder.notNull()) && (mComplete.size()) )
		moveAndRename(mComplete[0], idFolder, strName);
	delete this;
}
RlvWearableItemCollector::RlvWearableItemCollector(const LLInventoryCategory* pFolder, RlvForceWear::EWearAction eAction, RlvForceWear::EWearFlags eFlags)
	: m_idFolder(pFolder->getUUID()), m_eWearAction(eAction), m_eWearFlags(eFlags),
	  m_strWearAddPrefix(RlvSettings::getWearAddPrefix()), m_strWearReplacePrefix(RlvSettings::getWearReplacePrefix())
{
	m_Wearable.push_back(m_idFolder);
	if ( (m_strWearAddPrefix.length() > 1) && (RLV_FOLDER_PREFIX_HIDDEN == m_strWearAddPrefix[0]) )
		m_strWearAddPrefix.clear();
	if ( (m_strWearReplacePrefix.length() > 1) && (RLV_FOLDER_PREFIX_HIDDEN == m_strWearReplacePrefix[0]) )
		m_strWearReplacePrefix.clear();
	m_eWearAction = getWearActionNormal(pFolder);
	m_WearActionMap.insert(std::pair<LLUUID, RlvForceWear::EWearAction>(m_idFolder, m_eWearAction));
}
RlvForceWear::EWearAction RlvWearableItemCollector::getWearActionNormal(const LLInventoryCategory* pFolder)
{
	RLV_ASSERT_DBG(!RlvInventory::isFoldedFolder(pFolder, false));
	if ( (RlvForceWear::ACTION_WEAR_REPLACE == m_eWearAction) && (!m_strWearAddPrefix.empty()) &&
		 (boost::algorithm::starts_with(pFolder->getName(), m_strWearAddPrefix)))
	{
		return RlvForceWear::ACTION_WEAR_ADD;
	}
	else if ( (RlvForceWear::ACTION_WEAR_ADD == m_eWearAction) && (!m_strWearReplacePrefix.empty()) &&
		      (boost::algorithm::starts_with(pFolder->getName(), m_strWearReplacePrefix)) )
	{
		return RlvForceWear::ACTION_WEAR_REPLACE;
	}
	return (pFolder->getUUID() != m_idFolder) ? getWearAction(pFolder->getParentUUID()) : m_eWearAction;
}
const LLUUID& RlvWearableItemCollector::getFoldedParent(const LLUUID& idFolder) const
{
	std::map<LLUUID, LLUUID>::const_iterator itFolder = m_FoldingMap.end(), itCur = m_FoldingMap.find(idFolder);
	while (itCur != m_FoldingMap.end())
	{
		itFolder = itCur;
		itCur = m_FoldingMap.find(itFolder->second);
	}
	return (m_FoldingMap.end() == itFolder) ? idFolder : itFolder->second;
}
RlvForceWear::EWearAction RlvWearableItemCollector::getWearAction(const LLUUID& idFolder) const
{
	LLUUID idCurFolder(idFolder); std::map<LLUUID, RlvForceWear::EWearAction>::const_iterator itCurFolder;
	while ((itCurFolder = m_WearActionMap.find(idCurFolder)) == m_WearActionMap.end())
	{
		const LLViewerInventoryCategory* pFolder = gInventory.getCategory(idCurFolder);
		if ((!pFolder) || (gInventory.getRootFolderID() == pFolder->getParentUUID()))
			break;
		idCurFolder = pFolder->getParentUUID();
	}
	return (itCurFolder != m_WearActionMap.end()) ? itCurFolder->second : m_eWearAction;
}
bool RlvWearableItemCollector::onCollectFolder(const LLInventoryCategory* pFolder)
{
	bool fLinkedFolder = isLinkedFolder(pFolder->getUUID());
	if ( (!fLinkedFolder) && (m_Wearable.end() == std::find(m_Wearable.begin(), m_Wearable.end(), pFolder->getParentUUID())) )
		return false;
	const std::string& strFolder = pFolder->getName();
	if (strFolder.empty())
		return false;
	bool fAttach = RlvForceWear::isWearAction(m_eWearAction);
	bool fMatchAll = (!fLinkedFolder) && (m_eWearFlags & RlvForceWear::FLAG_MATCHALL);
	if ( (!fLinkedFolder) && (RlvInventory::isFoldedFolder(pFolder, false)) )
	{
		if ( (!fAttach) || (1 == RlvInventory::getDirectDescendentsItemCount(pFolder, LLAssetType::AT_OBJECT)) )
		{
			m_Folded.push_front(pFolder->getUUID());
			m_FoldingMap.insert(std::pair<LLUUID, LLUUID>(pFolder->getUUID(), pFolder->getParentUUID()));
		}
	}
	else if ( (RLV_FOLDER_PREFIX_HIDDEN != strFolder[0]) &&
		      ( (fMatchAll) || (fLinkedFolder) ) &&
			  (!isLinkedFolder(pFolder->getParentUUID())) )
	{
		#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
		if ( (!RlvSettings::getEnableComposites()) ||
			 (!gRlvHandler.isCompositeFolder(pFolder)) ||
		     ((m_fAttach) && (gRlvHandler.canWearComposite(pFolder))) ||
			 (!m_fAttach) && (gRlvHandler.canTakeOffComposite(pFolder)) )
		#endif
		{
			m_Wearable.push_front(pFolder->getUUID());
			m_WearActionMap.insert(std::pair<LLUUID, RlvForceWear::EWearAction>(pFolder->getUUID(), getWearActionNormal(pFolder)));
		}
		return (!fLinkedFolder) && (pFolder->getParentUUID() == m_idFolder);
	}
	#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	else if ( (RlvSettings::getEnableComposites()) &&
			  (RLV_FOLDER_PREFIX_HIDDEN == strFolder[0]) &&
			  (gRlvHandler.isCompositeFolder(pFolder)) &&
		      ( ((m_fAttach) && (gRlvHandler.canWearComposite(pFolder))) ||
			    (!m_fAttach) && (gRlvHandler.canTakeOffComposite(pFolder)) ) )
	{
		m_Wearable.push_front(pFolder->getUUID());
		m_FoldingMap.insert(std::pair<LLUUID, LLUUID>(pFolder->getUUID(), idParent));
	}
	#endif
	return false;
}
bool RlvWearableItemCollector::onCollectItem(const LLInventoryItem* pItem)
{
	bool fAttach = RlvForceWear::isWearAction(m_eWearAction);
	if ( (!fAttach) && (!RlvForceWear::isStrippable(pItem)) )
		return false;
	const LLUUID& idParent = pItem->getParentUUID(); bool fRet = false;
	switch (pItem->getType())
	{
		case LLAssetType::AT_BODYPART:
			if (!fAttach)
				break;
		case LLAssetType::AT_CLOTHING:
			fRet = ( (m_Wearable.end() != std::find(m_Wearable.begin(), m_Wearable.end(), idParent)) ||
					 ( (fAttach) && (m_Folded.end() != std::find(m_Folded.begin(), m_Folded.end(), idParent)) &&
					   (RlvForceWear::isStrippable(pItem)) ) );
			break;
		case LLAssetType::AT_OBJECT:
			fRet = ( (m_Wearable.end() != std::find(m_Wearable.begin(), m_Wearable.end(), idParent)) ||
				     (m_Folded.end() != std::find(m_Folded.begin(), m_Folded.end(), idParent)) ) &&
				   ( (!fAttach) || (RlvAttachPtLookup::hasAttachPointName(pItem)) || (RlvSettings::getEnableSharedWear()) );
			break;
		#ifdef RLV_EXTENSION_FORCEWEAR_GESTURES
		case LLAssetType::AT_GESTURE:
			fRet = (m_Wearable.end() != std::find(m_Wearable.begin(), m_Wearable.end(), idParent));
			break;
		#endif
		#ifdef RLV_EXTENSION_FORCEWEAR_FOLDERLINKS
		case LLAssetType::AT_CATEGORY:
			if (LLAssetType::AT_LINK_FOLDER == pItem->getActualType())
			{
				const LLUUID& idLinkedFolder = pItem->getLinkedUUID();
				LLViewerInventoryCategory* pLinkedFolder = gInventory.getCategory(idLinkedFolder);
				if ( (pLinkedFolder) && (LLFolderType::FT_OUTFIT != pLinkedFolder->getPreferredType()) &&
					 (gInventory.isObjectDescendentOf(pItem->getUUID(), m_idFolder)) &&
					 (!gInventory.isObjectDescendentOf(idLinkedFolder, m_idFolder)) )
				{
					m_FoldingMap.insert(std::pair<LLUUID, LLUUID>(idLinkedFolder, pItem->getParentUUID()));
					m_Linked.push_front(idLinkedFolder);
				}
			}
			break;
		#endif
		default:
			break;
	}
	return fRet;
}
bool RlvWearableItemCollector::operator()(LLInventoryCategory* pFolder, LLInventoryItem* pItem)
{
	return (pFolder) ? onCollectFolder(pFolder) : ( (pItem) ? onCollectItem(pItem) : false );
}
bool RlvFindAttachmentsOnPoint::operator()(LLInventoryCategory* pFolder, LLInventoryItem* pItem)
{
#ifndef RLV_DEPRECATE_ATTACHPTNAMING
	return (pItem) && (LLAssetType::AT_OBJECT == pItem->getType()) &&
		( ((m_pAttachPt) && (m_pAttachPt->getAttachedObject(pItem->getLinkedUUID()))) || (RlvAttachPtLookup::getAttachPoint(pItem) == m_pAttachPt) );
#else
	return (pItem) && (LLAssetType::AT_OBJECT == pItem->getType()) && (m_pAttachPt) && (m_pAttachPt->getAttachedObject(pItem->getLinkedUUID()));
#endif
}
