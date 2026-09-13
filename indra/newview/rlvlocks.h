/** 
 *
 * Copyright (c) 2009-2011, Kitty Barnett
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
#ifndef RLV_LOCKS_H
#define RLV_LOCKS_H
#include "llagentconstants.h"
#include "llagentwearables.h"
#include "lleventtimer.h"
#include "llvoavatarself.h"
#include "llviewerwearable.h"
#include "rlvdefines.h"
#include "rlvcommon.h"
class RlvAttachPtLookup
{
public:
	static LLViewerJointAttachment* getAttachPoint(S32 idxAttachPt);
	static LLViewerJointAttachment* getAttachPoint(const std::string& strText);
	static LLViewerJointAttachment* getAttachPoint(const LLInventoryItem* pItem);
	static S32 getAttachPointIndex(std::string strText);
	static S32 getAttachPointIndex(const LLViewerObject* pAttachObj);
	static S32 getAttachPointIndex(const LLViewerJointAttachment* pAttachPt);
	static S32 getAttachPointIndex(const LLInventoryCategory* pFolder);
	static S32 getAttachPointIndex(const LLInventoryItem* pItem, bool fFollowLinks = true);
	static bool hasAttachPointName(const LLInventoryItem* pItem) { return (0 != getAttachPointIndex(pItem)); }
private:
	static S32 getAttachPointIndexLegacy(const LLInventoryCategory* pFolder);
public:
	static void initLookupTable();
private:
	static std::map<std::string, S32> m_AttachPtLookupMap;
};
class RlvAttachmentLocks
{
public:
	RlvAttachmentLocks() : m_fHasLockedHUD(false) {}
public:
	void addAttachmentLock(const LLUUID& idAttachObj, const LLUUID& idRlvObj);
	void addAttachmentPointLock(S32 idxAttachPt, const LLUUID& idRlvObj, ERlvLockMask eLock);
	bool hasLockedAttachment(const LLViewerJointAttachment* pAttachPt) const;
	bool hasLockedAttachmentPoint(ERlvLockMask eLock) const;
	bool hasLockedHUD() const { return m_fHasLockedHUD; }
	bool isLockedAttachment(const LLViewerObject* pAttachObj) const;
	bool isLockedAttachmentExcept(const LLViewerObject* pObj, const LLUUID& idRlvObj) const;
	bool isLockedAttachmentPoint(S32 idxAttachPt, ERlvLockMask eLock) const;
	bool isLockedAttachmentPoint(const LLViewerJointAttachment* pAttachPt, ERlvLockMask eLock) const;
	void removeAttachmentLock(const LLUUID& idAttachObj, const LLUUID& idRlvObj);
	void removeAttachmentPointLock(S32 idxAttachPt, const LLUUID& idRlvObj, ERlvLockMask eLock);
	void updateLockedHUD();
	bool verifyAttachmentLocks();
protected:
	bool isLockedAttachmentPointExcept(S32 idxAttachPt, ERlvLockMask eLock, const LLUUID& idRlvObj) const;
public:
	bool         canAttach() const;
	ERlvWearMask canAttach(const LLInventoryItem* pItem, LLViewerJointAttachment** ppAttachPtOut = NULL) const;
	ERlvWearMask canAttach(const LLViewerJointAttachment* pAttachPt) const;
	bool canDetach(const LLInventoryItem* pItem) const;
	bool canDetach(const LLViewerJointAttachment* pAttachPt, bool fDetachAll = false) const;
public:
	typedef std::multimap<LLUUID, LLUUID> rlv_attachobjlock_map_t;
	typedef std::multimap<S32, LLUUID> rlv_attachptlock_map_t;
	const rlv_attachptlock_map_t& getAttachPtLocks(ERlvLockMask eLock) { return (RLV_LOCK_ADD == eLock) ? m_AttachPtAdd : m_AttachPtRem; }
	const rlv_attachobjlock_map_t& getAttachObjLocks() const
	{ return m_AttachObjRem; }
private:
	rlv_attachptlock_map_t	m_AttachPtAdd;
	rlv_attachptlock_map_t	m_AttachPtRem;
	rlv_attachobjlock_map_t	m_AttachObjRem;
	bool m_fHasLockedHUD;
};
extern RlvAttachmentLocks gRlvAttachmentLocks;
class RlvAttachmentLockWatchdog : public LLSingleton<RlvAttachmentLockWatchdog>
{
	friend class LLSingleton<RlvAttachmentLockWatchdog>;
protected:
	RlvAttachmentLockWatchdog() : m_pTimer(NULL) {}
	~RlvAttachmentLockWatchdog() { delete m_pTimer; }
protected:
	void detach(const LLViewerObject* pAttachObj);
	void detach(S32 idxAttachPt) { uuid_vec_t idsAttachObjExcept; detach(idxAttachPt, idsAttachObjExcept); }
	void detach(S32 idxAttachPt, const uuid_vec_t& idsAttachObjExcept);
	void startTimer() { if (!m_pTimer) m_pTimer = new RlvAttachmentLockWatchdogTimer(this); }
public:
	void onAttach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt);
	void onDetach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt);
	void onSavedAssetIntoInventory(const LLUUID& idItem);
	BOOL onTimer();
	void onWearAttachment(const LLInventoryItem* pItem, ERlvWearMask eWearAction);
	void onWearAttachment(const LLUUID& idItem, ERlvWearMask eWearAction);
protected:
	typedef std::list<LLUUID> rlv_detach_map_t;
	rlv_detach_map_t m_PendingDetach;
	struct RlvReattachInfo
	{
		RlvReattachInfo(const LLUUID& itemid) : idItem(itemid), fAssetSaved(false), tsAttach(0)
			{ tsDetach = LLFrameTimer::getElapsedSeconds(); }
		LLUUID idItem;
		bool   fAssetSaved;
		F64    tsDetach;
		F64    tsAttach;
	protected:
		RlvReattachInfo();
	};
	typedef std::multimap<S32, RlvReattachInfo> rlv_attach_map_t;
	rlv_attach_map_t m_PendingAttach;
	struct RlvWearInfo
	{
		RlvWearInfo(const LLUUID& itemid, ERlvWearMask wearaction) : idItem(itemid), eWearAction(wearaction)
			{ tsWear = LLFrameTimer::getElapsedSeconds(); }
		bool isAddLockedAttachPt(S32 idxAttachPt) const;
		void dumpInstance() const;
		LLUUID       idItem;
		ERlvWearMask eWearAction;
		F64          tsWear;
		std::map<S32, uuid_vec_t> attachPts;
	};
	typedef std::map<LLUUID, RlvWearInfo> rlv_wear_map_t;
	rlv_wear_map_t m_PendingWear;
	class RlvAttachmentLockWatchdogTimer : public LLEventTimer
	{
	public:
		RlvAttachmentLockWatchdogTimer(RlvAttachmentLockWatchdog* pWatchdog) : LLEventTimer(10), m_pWatchdog(pWatchdog) {}
		virtual ~RlvAttachmentLockWatchdogTimer() { m_pWatchdog->m_pTimer = NULL; }
		virtual BOOL tick() { return m_pWatchdog->onTimer(); }
		RlvAttachmentLockWatchdog* m_pWatchdog;
	} *m_pTimer;
};
class RlvWearableLocks
{
public:
	void addWearableTypeLock(LLWearableType::EType eType, const LLUUID& idRlvObj, ERlvLockMask eLock);
	bool hasLockedWearable(LLWearableType::EType eType) const;
	bool hasLockedWearableType(ERlvLockMask eLock) const;
	void removeWearableTypeLock(LLWearableType::EType eType, const LLUUID& idRlvObj, ERlvLockMask eLock);
	bool isLockedWearable(const LLViewerWearable* pWearable) const;
	bool isLockedWearableExcept(const LLViewerWearable* pWearable, const LLUUID& idRlvObj) const;
protected:
	bool isLockedWearableType(LLWearableType::EType eType, ERlvLockMask eLock) const;
	bool isLockedWearableTypeExcept(LLWearableType::EType eType, ERlvLockMask eLock, const LLUUID& idRlvObj) const;
public:
	ERlvWearMask canWear(const LLViewerInventoryItem* pItem) const;
	ERlvWearMask canWear(LLWearableType::EType eType) const;
	bool canRemove(const LLInventoryItem* pItem) const;
	bool canRemove(LLWearableType::EType eType) const;
public:
	typedef std::multimap<LLWearableType::EType, LLUUID> rlv_wearabletypelock_map_t;
	const rlv_wearabletypelock_map_t& getWearableTypeLocks(ERlvLockMask eLock) { return (RLV_LOCK_ADD == eLock) ? m_WearableTypeAdd : m_WearableTypeRem; }
protected:
	rlv_wearabletypelock_map_t m_WearableTypeAdd;
	rlv_wearabletypelock_map_t m_WearableTypeRem;
};
extern RlvWearableLocks gRlvWearableLocks;
class RlvFolderLocks : public LLSingleton<RlvFolderLocks>
{
	friend class RlvLockedDescendentsCollector;
public:
	RlvFolderLocks();
	enum ELockSourceType
	{
		ST_ATTACHMENT = 0x01, ST_ATTACHMENTPOINT = 0x02, ST_FOLDER = 0x04, ST_ROOTFOLDER = 0x08,
		ST_SHAREDPATH = 0x10, ST_WEARABLETYPE = 0x20, ST_NONE= 0x00, ST_MASK_ANY = 0xFF
	};
	typedef boost::variant<LLUUID, std::string, S32, LLWearableType::EType> lock_source_t;
	typedef std::pair<ELockSourceType, lock_source_t> folderlock_source_t;
	enum ELockPermission { PERM_ALLOW = 0x1, PERM_DENY = 0x2, PERM_MASK_ANY = 0x3 };
	enum ELockScope	{ SCOPE_NODE, SCOPE_SUBTREE } ;
	struct folderlock_descr_t
	{
		LLUUID				idRlvObj;
		ERlvLockMask		eLockType;
		folderlock_source_t	lockSource;
		ELockPermission		eLockPermission;
		ELockScope			eLockScope;
		folderlock_descr_t(const LLUUID& rlvObj, ERlvLockMask lockType, folderlock_source_t source, ELockPermission perm, ELockScope scope);
		bool operator ==(const folderlock_descr_t& rhs) const;
	};
public:
	void addFolderLock(const folderlock_source_t& lockSource, ELockPermission ePerm, ELockScope eScope, const LLUUID& idRlvObj, ERlvLockMask eLockType);
	bool hasLockedAttachment() const;
	bool hasLockedFolder(ERlvLockMask eLockTypeMask) const;
	bool hasLockedFolderDescendent(const LLUUID& idFolder, int eSourceTypeMask, ELockPermission ePermMask,
	                               ERlvLockMask eLockTypeMask, bool fCheckSelf) const;
	bool hasLockedWearable() const;
	bool isLockedAttachment(const LLUUID& idItem) const;
	bool isLockedFolder(LLUUID idFolder, ERlvLockMask eLock, int eSourceTypeMask = ST_MASK_ANY, folderlock_source_t* plockSource = NULL) const;
	bool isLockedWearable(const LLUUID& idItem) const;
	void removeFolderLock(const folderlock_source_t& lockSource, ELockPermission ePerm, ELockScope eScope, const LLUUID& idRlvObj, ERlvLockMask eLockType);
protected:
	bool isLockedFolderEntry(const LLUUID& idFolder, int eSourceTypeMask, ELockPermission ePermMask, ERlvLockMask eLockTypeMask) const;
public:
	bool canMoveFolder(const LLUUID& idFolder, const LLUUID& idFolderDest) const;
	bool canRemoveFolder(const LLUUID& idFolder) const;
	bool canRenameFolder(const LLUUID& idFolder) const;
	bool canMoveItem(const LLUUID& idItem, const LLUUID& idFolderDest) const;
	bool canRemoveItem(const LLUUID& idItem) const;
	bool canRenameItem(const LLUUID& idItem) const;
protected:
	bool getLockedFolders(const folderlock_source_t& lockSource, LLInventoryModel::cat_array_t& lockFolders) const;
	bool getLockedItems(const LLUUID& idFolder, LLInventoryModel::item_array_t& lockItems) const;
	void onNeedsLookupRefresh() const;
	void refreshLockedLookups() const;
public:
	typedef std::list<const folderlock_descr_t*> folderlock_list_t;
	const folderlock_list_t& getFolderLocks() const
	{ return m_FolderLocks; }
	const uuid_vec_t& getAttachmentLookups() const
	{ return m_LockedAttachmentRem; }
	const uuid_vec_t& getWearableLookups() const
	{ return m_LockedWearableRem; }
protected:
	folderlock_list_t	m_FolderLocks;
	S32					m_cntLockAdd;
	S32					m_cntLockRem;
	typedef std::multimap<LLUUID, const folderlock_descr_t*> folderlock_map_t;
	mutable bool				m_fLookupDirty;
	mutable bool				m_fLockedRoot;
	mutable uuid_vec_t			m_LockedAttachmentRem;
	mutable folderlock_map_t	m_LockedFolderMap;
	mutable uuid_vec_t			m_LockedWearableRem;
private:
	friend class LLSingleton<RlvFolderLocks>;
};
inline LLViewerJointAttachment* RlvAttachPtLookup::getAttachPoint(S32 idxAttachPt)
{
	return (isAgentAvatarValid()) ? get_if_there(gAgentAvatarp->mAttachmentPoints, idxAttachPt, static_cast<LLViewerJointAttachment*>(NULL)) : NULL;
}
inline LLViewerJointAttachment* RlvAttachPtLookup::getAttachPoint(const std::string& strText)
{
	return (isAgentAvatarValid()) ? get_if_there(gAgentAvatarp->mAttachmentPoints, getAttachPointIndex(strText), static_cast<LLViewerJointAttachment*>(NULL)) : NULL;
}
inline LLViewerJointAttachment* RlvAttachPtLookup::getAttachPoint(const LLInventoryItem* pItem)
{
	return (isAgentAvatarValid()) ? get_if_there(gAgentAvatarp->mAttachmentPoints, getAttachPointIndex(pItem), static_cast<LLViewerJointAttachment*>(NULL)) : NULL;
}
inline S32 RlvAttachPtLookup::getAttachPointIndex(std::string strText)
{
	LLStringUtil::toLower(strText);
	RLV_ASSERT(m_AttachPtLookupMap.size() > 0);
	std::map<std::string, S32>::const_iterator itAttachPt = m_AttachPtLookupMap.find(strText);
	return (itAttachPt != m_AttachPtLookupMap.end()) ? itAttachPt->second : 0;
}
inline S32 RlvAttachPtLookup::getAttachPointIndex(const LLViewerObject* pObj)
{
	return (pObj) ? ATTACHMENT_ID_FROM_STATE(pObj->getAttachmentState()) : 0;
}
inline ERlvWearMask RlvAttachmentLocks::canAttach(const LLInventoryItem* pItem, LLViewerJointAttachment** ppAttachPtOut ) const
{
	LLViewerJointAttachment* pAttachPt = RlvAttachPtLookup::getAttachPoint(pItem);
	if (ppAttachPtOut)
		*ppAttachPtOut = pAttachPt;
	return ((canAttach()) && (pItem) && (!RlvFolderLocks::instance().isLockedFolder(pItem->getParentUUID(), RLV_LOCK_ADD)))
		? ((!pAttachPt) ? RLV_WEAR : canAttach(pAttachPt)) : RLV_WEAR_LOCKED;
}
inline ERlvWearMask RlvAttachmentLocks::canAttach(const LLViewerJointAttachment* pAttachPt) const
{
	RLV_ASSERT(pAttachPt);
	return
		static_cast<ERlvWearMask>(((pAttachPt) && (!isLockedAttachmentPoint(pAttachPt, RLV_LOCK_ADD)))
			? ((canDetach(pAttachPt, true)) ? RLV_WEAR_REPLACE : 0) | RLV_WEAR_ADD
			: RLV_WEAR_LOCKED);
}
inline bool RlvAttachmentLocks::canDetach(const LLInventoryItem* pItem) const
{
	const LLViewerObject* pAttachObj =
		((pItem) && (isAgentAvatarValid())) ? gAgentAvatarp->getWornAttachment(pItem->getLinkedUUID()) : NULL;
	return (!pAttachObj) || (!isLockedAttachment(pAttachObj));
}
inline bool RlvAttachmentLocks::hasLockedAttachmentPoint(ERlvLockMask eLock) const
{
	return
		((eLock & RLV_LOCK_REMOVE) && ((!m_AttachPtRem.empty()) || (!m_AttachObjRem.empty()) || (RlvFolderLocks::instance().hasLockedAttachment()))) ||
		((eLock & RLV_LOCK_ADD) && (!m_AttachPtAdd.empty()) );
}
inline bool RlvAttachmentLocks::isLockedAttachment(const LLViewerObject* pAttachObj) const
{
	RLV_ASSERT( (!pAttachObj) || (pAttachObj == pAttachObj->getRootEdit()) );
	return
		(pAttachObj) && (pAttachObj->isAttachment()) && (!pAttachObj->isTempAttachment()) &&
		( (m_AttachObjRem.find(pAttachObj->getID()) != m_AttachObjRem.end()) ||
		  (isLockedAttachmentPoint(RlvAttachPtLookup::getAttachPointIndex(pAttachObj), RLV_LOCK_REMOVE)) ||
		  (RlvFolderLocks::instance().isLockedAttachment(pAttachObj->getAttachmentItemID())) );
}
inline bool RlvAttachmentLocks::isLockedAttachmentPoint(S32 idxAttachPt, ERlvLockMask eLock) const
{
	return
		( (eLock & RLV_LOCK_REMOVE) && (m_AttachPtRem.find(idxAttachPt) != m_AttachPtRem.end()) ) ||
		( (eLock & RLV_LOCK_ADD) && (m_AttachPtAdd.find(idxAttachPt) != m_AttachPtAdd.end()) );
}
inline bool RlvAttachmentLocks::isLockedAttachmentPoint(const LLViewerJointAttachment* pAttachPt, ERlvLockMask eLock) const
{
	return (pAttachPt) && (isLockedAttachmentPoint(RlvAttachPtLookup::getAttachPointIndex(pAttachPt), eLock));
}
inline bool RlvWearableLocks::canRemove(const LLInventoryItem* pItem) const
{
	RLV_ASSERT( (pItem) && (LLInventoryType::IT_WEARABLE == pItem->getInventoryType()) );
	const LLViewerWearable* pWearable = (pItem) ? gAgentWearables.getWearableFromItemID(pItem->getLinkedUUID()) : NULL;
	return (pWearable) && (!isLockedWearable(pWearable));
}
inline ERlvWearMask RlvWearableLocks::canWear(const LLViewerInventoryItem* pItem) const
{
	RLV_ASSERT( (pItem) && (LLInventoryType::IT_WEARABLE == pItem->getInventoryType()) );
	return ((pItem) && (!RlvFolderLocks::instance().isLockedFolder(pItem->getParentUUID(), RLV_LOCK_ADD)))
		? canWear(pItem->getWearableType()) : RLV_WEAR_LOCKED;
}
inline ERlvWearMask RlvWearableLocks::canWear(LLWearableType::EType eType) const
{
	return (!isLockedWearableType(eType, RLV_LOCK_ADD))
	  ? ((!hasLockedWearable(eType))
			? RLV_WEAR
			: (gAgentWearables.canAddWearable(eType)) ? RLV_WEAR_ADD : RLV_WEAR_LOCKED)
	  : RLV_WEAR_LOCKED;
}
inline bool RlvWearableLocks::hasLockedWearableType(ERlvLockMask eLock) const
{
	return ( (eLock & RLV_LOCK_REMOVE) && (!m_WearableTypeRem.empty()) ) || ( (eLock & RLV_LOCK_ADD) && (!m_WearableTypeAdd.empty()) );
}
inline bool RlvWearableLocks::isLockedWearable(const LLViewerWearable* pWearable) const
{
	RLV_ASSERT(pWearable);
	return
		(pWearable) &&
		( (isLockedWearableType(pWearable->getType(), RLV_LOCK_REMOVE)) || (RlvFolderLocks::instance().isLockedWearable(pWearable->getItemID())) );
}
inline bool RlvWearableLocks::isLockedWearableType(LLWearableType::EType eType, ERlvLockMask eLock) const
{
	return
		( (eLock & RLV_LOCK_REMOVE) && (m_WearableTypeRem.find(eType) != m_WearableTypeRem.end()) ) ||
		( (eLock & RLV_LOCK_ADD) && (m_WearableTypeAdd.find(eType) != m_WearableTypeAdd.end()) );
}
inline void RlvAttachmentLockWatchdog::onWearAttachment(const LLInventoryItem* pItem, ERlvWearMask eWearAction)
{
	onWearAttachment(pItem->getLinkedUUID(), eWearAction);
}
inline RlvFolderLocks::folderlock_descr_t::folderlock_descr_t(const LLUUID& rlvObj, ERlvLockMask lockType, folderlock_source_t source,
															  ELockPermission perm, ELockScope scope)
	: idRlvObj(rlvObj), eLockType(lockType), lockSource(source), eLockPermission(perm), eLockScope(scope)
{
}
inline bool RlvFolderLocks::folderlock_descr_t::operator ==(const folderlock_descr_t& rhs) const
{
	return (idRlvObj == rhs.idRlvObj) && (eLockType == rhs.eLockType) && (lockSource == rhs.lockSource) &&
		(eLockPermission == rhs.eLockPermission) && (eLockScope == rhs.eLockScope);
}
inline bool RlvFolderLocks::canMoveFolder(const LLUUID& idFolder, const LLUUID& idFolderDest) const
{
	folderlock_source_t lockSource(ST_NONE, 0), lockSourceDest(ST_NONE, 0);
	return
		(!hasLockedFolderDescendent(idFolder, ST_MASK_ANY, PERM_MASK_ANY, RLV_LOCK_ANY, true)) &&
		( (isLockedFolder(idFolder, RLV_LOCK_ANY, ST_MASK_ANY, &lockSource) == isLockedFolder(idFolderDest, RLV_LOCK_ANY, ST_MASK_ANY, &lockSourceDest)) &&
		  (lockSource == lockSourceDest) );
}
inline bool RlvFolderLocks::canRemoveFolder(const LLUUID& idFolder) const
{
	return
		(!hasLockedFolderDescendent(idFolder, ST_MASK_ANY, PERM_MASK_ANY, RLV_LOCK_ANY, true)) &&
		(!isLockedFolder(idFolder, RLV_LOCK_ANY, ST_MASK_ANY & ~ST_ROOTFOLDER));
}
inline bool RlvFolderLocks::canRenameFolder(const LLUUID& idFolder) const
{
	return !hasLockedFolderDescendent(idFolder, ST_SHAREDPATH | ST_ATTACHMENT | ST_ATTACHMENTPOINT | ST_WEARABLETYPE, PERM_MASK_ANY, RLV_LOCK_ANY, true);
}
inline bool RlvFolderLocks::canMoveItem(const LLUUID& idItem, const LLUUID& idFolderDest) const
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem); const LLUUID& idFolder = (pItem) ? pItem->getParentUUID() : LLUUID::null;
	int maskSource = ST_MASK_ANY & ~ST_ROOTFOLDER; folderlock_source_t lockSource(ST_NONE, 0), lockSourceDest(ST_NONE, 0);
	return
		(idFolder.notNull()) &&
		(isLockedFolder(idFolder, RLV_LOCK_ANY, maskSource, &lockSource) == isLockedFolder(idFolderDest, RLV_LOCK_ANY, maskSource, &lockSourceDest)) &&
		(lockSource == lockSourceDest);
}
inline bool RlvFolderLocks::canRemoveItem(const LLUUID& idItem) const
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem); const LLUUID& idFolder = (pItem) ? pItem->getParentUUID() : LLUUID::null;
	int maskSource = ST_MASK_ANY & ~ST_ROOTFOLDER;
	return (idFolder.notNull()) && (!isLockedFolder(idFolder, RLV_LOCK_ANY, maskSource));
}
inline bool RlvFolderLocks::canRenameItem(const LLUUID& idItem) const
{
	return true;
}
inline bool RlvFolderLocks::hasLockedAttachment() const
{
	if (m_fLookupDirty)
		refreshLockedLookups();
	return !m_LockedAttachmentRem.empty();
}
inline bool RlvFolderLocks::hasLockedFolder(ERlvLockMask eLock) const
{
	return ((eLock & RLV_LOCK_REMOVE) && (m_cntLockRem)) || ((eLock & RLV_LOCK_ADD) && (m_cntLockAdd));
}
inline bool RlvFolderLocks::hasLockedWearable() const
{
	if (m_fLookupDirty)
		refreshLockedLookups();
	return !m_LockedWearableRem.empty();
}
inline bool RlvFolderLocks::isLockedAttachment(const LLUUID& idItem) const
{
	if (m_fLookupDirty)
		refreshLockedLookups();
	return (std::find(m_LockedAttachmentRem.begin(), m_LockedAttachmentRem.end(), idItem) != m_LockedAttachmentRem.end());
}
inline bool RlvFolderLocks::isLockedWearable(const LLUUID& idItem) const
{
	if (m_fLookupDirty)
		refreshLockedLookups();
	return (std::find(m_LockedWearableRem.begin(), m_LockedWearableRem.end(), idItem) != m_LockedWearableRem.end());
}
#endif
