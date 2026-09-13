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
#ifndef RLV_INVENTORY_H
#define RLV_INVENTORY_H
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llsingleton.h"
#include "rlvdefines.h"
#include "rlvhelper.h"
#include "rlvlocks.h"
#include <boost/format.hpp>
class RlvInventory : public LLSingleton<RlvInventory>, public LLInventoryObserver
{
protected:
	RlvInventory();
public:
	~RlvInventory();
	void changed(U32 mask);
public:
	typedef boost::signals2::signal<void (void)> callback_signal_t;
	void						addSharedRootIDChangedCallback(const callback_signal_t::slot_type& cb) { m_OnSharedRootIDChanged.connect(cb); }
	bool						findSharedFolders(const std::string& strCriteria, LLInventoryModel::cat_array_t& folders) const;
	bool						getPath(const uuid_vec_t& idItems, LLInventoryModel::cat_array_t& folders) const;
	LLViewerInventoryCategory*	getSharedRoot() const;
	const LLUUID&				getSharedRootID() const;
	LLViewerInventoryCategory*	getSharedFolder(const LLUUID& idParent, const std::string& strFolderName, bool fMatchPartial = true) const;
	LLViewerInventoryCategory*	getSharedFolder(const std::string& strPath, bool fMatchPartial = true) const;
	std::string					getSharedPath(const LLViewerInventoryCategory* pFolder) const;
	std::string					getSharedPath(const LLUUID& idFolder) const;
	bool						isSharedFolder(const LLUUID& idFolder);
public:
	void fetchSharedInventory();
	void fetchWornItems();
protected:
	void fetchSharedLinks();
public:
	static S32 getDirectDescendentsFolderCount(const LLInventoryCategory* pFolder);
	static S32 getDirectDescendentsItemCount(const LLInventoryCategory* pFolder, LLAssetType::EType filterType);
	static const LLUUID& getFoldedParent(const LLUUID& idFolder, bool fCheckComposite);
	static bool isFoldedFolder(const LLInventoryCategory* pFolder, bool fCheckComposite);
protected:
	bool				m_fFetchStarted;
	bool				m_fFetchComplete;
	mutable LLUUID		m_idRlvRoot;
	callback_signal_t	m_OnSharedRootIDChanged;
private:
	static const std::string cstrSharedRoot;
	friend class RlvSharedInventoryFetcher;
	friend class LLSingleton<RlvInventory>;
};
class RlvRenameOnWearObserver : public LLInventoryFetchItemsObserver
{
public:
	RlvRenameOnWearObserver(const LLUUID& idItem) : LLInventoryFetchItemsObserver(idItem) {}
	virtual ~RlvRenameOnWearObserver() {}
	virtual void done();
protected:
	void doneIdle();
	void onCategoryCreate(const LLUUID& folder_id, const LLUUID& item_id);
};
class RlvGiveToRLVOffer
{
protected:
	RlvGiveToRLVOffer(){}
	virtual ~RlvGiveToRLVOffer() {}
protected:
	bool	createDestinationFolder(const std::string& strPath);
	virtual void onDestinationCreated(const LLUUID& idFolder, const std::string& strName) = 0;
	void	moveAndRename(const LLUUID& idFolder, const LLUUID& idDestination, const std::string& strName);
private:
	void onCategoryCreate(const LLUUID& folder_id);
private:
	std::list<std::string> m_DestPath;
};
class RlvGiveToRLVTaskOffer : public LLInventoryObserver, public RlvGiveToRLVOffer
{
public:
	RlvGiveToRLVTaskOffer(const LLUUID& idTransaction) : RlvGiveToRLVOffer(), m_idTransaction(idTransaction) {}
	void changed(U32 mask);
protected:
	void done();
	void doneIdle();
	void onDestinationCreated(const LLUUID& idFolder, const std::string& strName);
protected:
	typedef uuid_vec_t folder_ref_t;
	folder_ref_t m_Folders;
	LLUUID       m_idTransaction;
};
class RlvGiveToRLVAgentOffer : public LLInventoryFetchDescendentsObserver, public RlvGiveToRLVOffer
{
public:
	RlvGiveToRLVAgentOffer(const LLUUID& idFolder) : LLInventoryFetchDescendentsObserver(idFolder), RlvGiveToRLVOffer() {}
	~RlvGiveToRLVAgentOffer() {}
public:
	void done();
protected:
	void doneIdle();
	void onDestinationCreated(const LLUUID& idFolder, const std::string& strName);
};
class RlvCriteriaCategoryCollector : public LLInventoryCollectFunctor
{
public:
	RlvCriteriaCategoryCollector(const std::string& strCriteria)
	{
		std::string::size_type idxIt, idxLast = 0;
		while (idxLast < strCriteria.length())
		{
			idxIt = strCriteria.find("&&", idxLast);
			if (std::string::npos == idxIt)
				idxIt = strCriteria.length();
			if (idxIt != idxLast)
				m_Criteria.push_back(strCriteria.substr(idxLast, idxIt - idxLast));
			idxLast = idxIt + 2;
		}
	}
	virtual ~RlvCriteriaCategoryCollector() {}
	virtual bool operator()(LLInventoryCategory* pFolder, LLInventoryItem* pItem)
	{
		if ( (!pFolder) || (m_Criteria.empty()) )
			return false;
		std::string strFolderName = pFolder->getName();
		LLStringUtil::toLower(strFolderName);
		if ( (strFolderName.empty()) ||
			 (RLV_FOLDER_PREFIX_HIDDEN == strFolderName[0]) || (RLV_FOLDER_PREFIX_PUTINV == strFolderName[0]) )
		{
			return false;
		}
		for (std::list<std::string>::const_iterator itCrit = m_Criteria.begin(); itCrit != m_Criteria.end(); ++itCrit)
			if (std::string::npos == strFolderName.find(*itCrit))
				return false;
		return true;
	}
protected:
	std::list<std::string> m_Criteria;
};
class RlvWearableItemCollector : public LLInventoryCollectFunctor
{
public:
	RlvWearableItemCollector(const LLInventoryCategory* pFolder, RlvForceWear::EWearAction eAction, RlvForceWear::EWearFlags eFlags);
	virtual ~RlvWearableItemCollector() {}
	virtual bool operator()(LLInventoryCategory* pFolder, LLInventoryItem* pItem);
	const LLUUID&             getFoldedParent(const LLUUID& idFolder) const;
	RlvForceWear::EWearAction getWearAction(const LLUUID& idFolder) const;
	RlvForceWear::EWearAction getWearActionNormal(const LLInventoryCategory* pFolder);
	RlvForceWear::EWearAction getWearActionFolded(const LLInventoryCategory* pFolder);
	bool                      isLinkedFolder(const LLUUID& idFolder);
protected:
	const LLUUID              m_idFolder;
	RlvForceWear::EWearAction m_eWearAction;
	RlvForceWear::EWearFlags  m_eWearFlags;
	bool onCollectFolder(const LLInventoryCategory* pFolder);
	bool onCollectItem(const LLInventoryItem* pItem);
	std::list<LLUUID> m_Folded;
	std::list<LLUUID> m_Linked;
	std::list<LLUUID> m_Wearable;
	std::map<LLUUID, LLUUID>                    m_FoldingMap;
	std::map<LLUUID, RlvForceWear::EWearAction> m_WearActionMap;
	std::string m_strWearAddPrefix;
	std::string m_strWearReplacePrefix;
};
class RlvIsLinkType : public LLInventoryCollectFunctor
{
public:
	RlvIsLinkType() {}
	~RlvIsLinkType() {}
	virtual bool operator()(LLInventoryCategory* pFolder, LLInventoryItem* pItem) { return (pItem) && (pItem->getIsLinkType()); }
};
class RlvFindAttachmentsOnPoint : public LLInventoryCollectFunctor
{
public:
	RlvFindAttachmentsOnPoint(const LLViewerJointAttachment* pAttachPt) : m_pAttachPt(pAttachPt) {}
	~RlvFindAttachmentsOnPoint() {}
	virtual bool operator()(LLInventoryCategory* pFolder, LLInventoryItem* pItem);
protected:
	const LLViewerJointAttachment* m_pAttachPt;
};
inline LLViewerInventoryCategory* RlvInventory::getSharedRoot() const
{
	const LLUUID& idRlvRoot = getSharedRootID();
	return (idRlvRoot.notNull()) ? gInventory.getCategory(idRlvRoot) : NULL;
}
inline const LLUUID& RlvInventory::getFoldedParent(const LLUUID& idFolder, bool fCheckComposite)
{
	LLViewerInventoryCategory* pFolder = gInventory.getCategory(idFolder);
	while ((pFolder) && (isFoldedFolder(pFolder, fCheckComposite)))
		pFolder = gInventory.getCategory(pFolder->getParentUUID());
	return (pFolder) ? pFolder->getUUID() : LLUUID::null;
}
inline std::string RlvInventory::getSharedPath(const LLUUID& idFolder) const
{
	return getSharedPath(gInventory.getCategory(idFolder));
}
inline bool RlvInventory::isFoldedFolder(const LLInventoryCategory* pFolder, bool fCheckComposite)
{
	return
	  (pFolder) && ( (RlvSettings::getEnableLegacyNaming()) || (RLV_FOLDER_PREFIX_HIDDEN == pFolder->getName().at(0)) ) &&
	  (
		(0 != RlvAttachPtLookup::getAttachPointIndex(pFolder))
		  || ((pFolder) && ((boost::format("%1%%2%%3%") % ".(" % RLV_FOLDER_FLAG_NOSTRIP % ")").str() == pFolder->getName()))
		#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
		|| ( (fCheckComposite) && (RlvSettings::getEnableComposites()) &&
		     (pFolder) && (RLV_FOLDER_PREFIX_HIDDEN == pFolder->getName().at(0)) && (isCompositeFolder(pFolder)) )
		#endif
	  );
}
inline bool RlvInventory::isSharedFolder(const LLUUID& idFolder)
{
	const LLViewerInventoryCategory* pRlvRoot = getSharedRoot();
	return (pRlvRoot) ? (pRlvRoot->getUUID() != idFolder) && (gInventory.isObjectDescendentOf(idFolder, pRlvRoot->getUUID())) : false;
}
inline bool RlvWearableItemCollector::isLinkedFolder(const LLUUID& idFolder)
{
	return (!m_Linked.empty()) && (m_Linked.end() != std::find(m_Linked.begin(), m_Linked.end(), idFolder));
}
#endif
