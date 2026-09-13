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
#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "lloutfitobserver.h"
#include "llviewerobjectlist.h"
#include "pipeline.h"
#include "rlvlocks.h"
#include "rlvhelper.h"
#include "rlvinventory.h"
std::map<std::string, S32> RlvAttachPtLookup::m_AttachPtLookupMap;
void RlvAttachPtLookup::initLookupTable()
{
	static bool fInitialized = false;
	if (!fInitialized)
	{
		if ( (gAgentAvatarp) && (gAgentAvatarp->mAttachmentPoints.size() > 0) )
		{
			std::string strAttachPtName;
			for (LLVOAvatar::attachment_map_t::const_iterator itAttach = gAgentAvatarp->mAttachmentPoints.begin();
					 itAttach != gAgentAvatarp->mAttachmentPoints.end(); ++itAttach)
			{
				const LLViewerJointAttachment* pAttachPt = itAttach->second;
				if (pAttachPt)
				{
					strAttachPtName = pAttachPt->getName();
					LLStringUtil::toLower(strAttachPtName);
					m_AttachPtLookupMap.insert(std::pair<std::string, S32>(strAttachPtName, itAttach->first));
					if ("avatar center" == strAttachPtName)
					{
						m_AttachPtLookupMap.insert(std::pair<std::string, S32>("root", itAttach->first));
					}
				}
			}
			fInitialized = true;
		}
	}
}
S32 RlvAttachPtLookup::getAttachPointIndex(const LLViewerJointAttachment* pAttachPt)
{
	if (isAgentAvatarValid())
	{
		for (LLVOAvatar::attachment_map_t::const_iterator itAttach = gAgentAvatarp->mAttachmentPoints.begin();
				itAttach != gAgentAvatarp->mAttachmentPoints.end(); ++itAttach)
		{
			if (itAttach->second == pAttachPt)
				return itAttach->first;
		}
	}
	return 0;
}
S32 RlvAttachPtLookup::getAttachPointIndex(const LLInventoryCategory* pFolder)
{
	if (!pFolder)
		return 0;
	if (RlvSettings::getEnableLegacyNaming())
		return getAttachPointIndexLegacy(pFolder);
	std::string::size_type idxMatch;
	std::string strAttachPt = rlvGetFirstParenthesisedText(pFolder->getName(), &idxMatch);
	LLStringUtil::trim(strAttachPt);
	return ( (1 == idxMatch) && (RLV_FOLDER_PREFIX_HIDDEN == pFolder->getName().at(0)) ) ? getAttachPointIndex(strAttachPt) : 0;
}
S32 RlvAttachPtLookup::getAttachPointIndex(const LLInventoryItem* pItem, bool fFollowLinks )
{
	if ( (!pItem) || (LLAssetType::AT_OBJECT != pItem->getType()) )
		return 0;
	if ( (LLAssetType::AT_LINK == pItem->getActualType()) && (fFollowLinks) )
	{
		S32 idxAttachPt = getAttachPointIndex(gInventory.getItem(pItem->getLinkedUUID()), false);
		if (idxAttachPt)
			return idxAttachPt;
	}
	std::string strAttachPt =
		(LLAssetType::AT_LINK != pItem->getActualType()) ? rlvGetLastParenthesisedText(pItem->getName()) : LLStringUtil::null;
	LLStringUtil::trim(strAttachPt);
	S32 idxAttachPt = 0;
	if (pItem->getPermissions().allowModifyBy(gAgent.getID()))
	{
		idxAttachPt = (!strAttachPt.empty()) ? getAttachPointIndex(strAttachPt) : 0;
		if (!idxAttachPt)
			idxAttachPt = getAttachPointIndex(gInventory.getCategory(pItem->getParentUUID()));
	}
	else
	{
		idxAttachPt = getAttachPointIndex(gInventory.getCategory(pItem->getParentUUID()));
		if ( (!idxAttachPt) && (!strAttachPt.empty()) )
			idxAttachPt = getAttachPointIndex(strAttachPt);
	}
	return idxAttachPt;
}
S32 RlvAttachPtLookup::getAttachPointIndexLegacy(const LLInventoryCategory* pFolder)
{
	if ( (!pFolder) || (pFolder->getName().empty()) )
		return 0;
	std::string::size_type idxMatch;
	std::string strAttachPt = rlvGetFirstParenthesisedText(pFolder->getName(), &idxMatch);
	if (!strAttachPt.empty())
	{
		if ( (0 != idxMatch) && ((1 != idxMatch) || (RLV_FOLDER_PREFIX_HIDDEN == pFolder->getName().at(0)) ) &&
			 (idxMatch + strAttachPt.length() + 1 != pFolder->getName().length()) )
		{
			strAttachPt = rlvGetLastParenthesisedText(pFolder->getName());
		}
	}
	else
	{
		strAttachPt = pFolder->getName();
		if (RLV_FOLDER_PREFIX_HIDDEN == strAttachPt[0])
			strAttachPt.erase(0, 1);
	}
	return getAttachPointIndex(strAttachPt);
}
RlvAttachmentLocks gRlvAttachmentLocks;
void RlvAttachmentLocks::addAttachmentLock(const LLUUID& idAttachObj, const LLUUID& idRlvObj)
{
#ifndef RLV_RELEASE
	LLViewerObject* pDbgObj = gObjectList.findObject(idAttachObj);
	RLV_VERIFY( (pDbgObj) && (pDbgObj->isAttachment()) && (pDbgObj == pDbgObj->getRootEdit()) );
#endif
	m_AttachObjRem.insert(std::pair<LLUUID, LLUUID>(idAttachObj, idRlvObj));
	if(LLViewerObject *pObj = gObjectList.findObject(idAttachObj))
	{
		gInventory.addChangedMask(LLInventoryObserver::LABEL, pObj->getAttachmentItemID());
		gInventory.notifyObservers();
	}
	updateLockedHUD();
}
void RlvAttachmentLocks::addAttachmentPointLock(S32 idxAttachPt, const LLUUID& idRlvObj, ERlvLockMask eLock)
{
	if (eLock & RLV_LOCK_REMOVE)
	{
		m_AttachPtRem.insert(std::pair<S32, LLUUID>(idxAttachPt, idRlvObj));
		if (isAgentAvatarValid())
		{
			bool need_update = false;
			LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.find(idxAttachPt);
			if (iter != gAgentAvatarp->mAttachmentPoints.end())
			{
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = iter->second->mAttachedObjects.begin();
						attachment_iter != iter->second->mAttachedObjects.end();++attachment_iter)
				{
					LLViewerObject* attached_object = (*attachment_iter);
					if(attached_object)
					{
						gInventory.addChangedMask(LLInventoryObserver::LABEL, attached_object->getAttachmentItemID());
						need_update = true;
					}
				}
			}
			if(need_update)
				gInventory.notifyObservers();
		}
		updateLockedHUD();
	}
	if (eLock & RLV_LOCK_ADD)
		m_AttachPtAdd.insert(std::pair<S32, LLUUID>(idxAttachPt, idRlvObj));
}
bool RlvAttachmentLocks::canAttach() const
{
	if (isAgentAvatarValid())
	{
		for (LLVOAvatar::attachment_map_t::const_iterator itAttachPt = gAgentAvatarp->mAttachmentPoints.begin();
				itAttachPt != gAgentAvatarp->mAttachmentPoints.end(); ++itAttachPt)
		{
			if (!isLockedAttachmentPoint(itAttachPt->first, RLV_LOCK_ADD))
				return true;
		}
	}
	return false;
}
bool RlvAttachmentLocks::canDetach(const LLViewerJointAttachment* pAttachPt, bool fDetachAll ) const
{
	if (pAttachPt)
	{
		for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
				itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
		{
			if ( (fDetachAll) ^ (!isLockedAttachment(*itAttachObj)) )
				return !fDetachAll;
		}
	}
	return (pAttachPt) && (fDetachAll);
}
bool RlvAttachmentLocks::hasLockedAttachment(const LLViewerJointAttachment* pAttachPt) const
{
	if (pAttachPt)
	{
		for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
				itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
		{
			if (isLockedAttachment(*itAttachObj))
				return true;
		}
	}
	return false;
}
bool RlvAttachmentLocks::isLockedAttachmentExcept(const LLViewerObject* pObj, const LLUUID& idRlvObj) const
{
	if (idRlvObj.isNull())
		return isLockedAttachment(pObj);
	RLV_ASSERT( (!pObj) || (pObj == pObj->getRootEdit()) );
	for (rlv_attachobjlock_map_t::const_iterator itAttachObj = m_AttachObjRem.lower_bound(pObj->getID()),
			endAttachObj = m_AttachObjRem.upper_bound(pObj->getID()); itAttachObj != endAttachObj; ++itAttachObj)
	{
		if (itAttachObj->second != idRlvObj)
			return true;
	}
	return isLockedAttachmentPointExcept(RlvAttachPtLookup::getAttachPointIndex(pObj), RLV_LOCK_REMOVE, idRlvObj);
}
bool RlvAttachmentLocks::isLockedAttachmentPointExcept(S32 idxAttachPt, ERlvLockMask eLock, const LLUUID& idRlvObj) const
{
	if (idRlvObj.isNull())
		return isLockedAttachmentPoint(idxAttachPt, eLock);
	if (eLock & RLV_LOCK_REMOVE)
	{
		for (rlv_attachptlock_map_t::const_iterator itAttachPt = m_AttachPtRem.lower_bound(idxAttachPt),
				endAttachPt = m_AttachPtRem.upper_bound(idxAttachPt); itAttachPt != endAttachPt; ++itAttachPt)
		{
			if (itAttachPt->second != idRlvObj)
				return true;
		}
	}
	if (eLock & RLV_LOCK_ADD)
	{
		for (rlv_attachptlock_map_t::const_iterator itAttachPt = m_AttachPtAdd.lower_bound(idxAttachPt),
				endAttachPt = m_AttachPtAdd.upper_bound(idxAttachPt); itAttachPt != endAttachPt; ++itAttachPt)
		{
			if (itAttachPt->second != idRlvObj)
				return true;
		}
	}
	return false;
}
void RlvAttachmentLocks::removeAttachmentLock(const LLUUID& idAttachObj, const LLUUID& idRlvObj)
{
#ifndef RLV_RELEASE
	const LLViewerObject* pDbgObj = gObjectList.findObject(idAttachObj);
	RLV_VERIFY( (!pDbgObj) || ((pDbgObj->isAttachment()) && (pDbgObj == pDbgObj->getRootEdit())) );
#endif
	RLV_ASSERT( m_AttachObjRem.lower_bound(idAttachObj) != m_AttachObjRem.upper_bound(idAttachObj) );
	for (rlv_attachobjlock_map_t::iterator itAttachObj = m_AttachObjRem.lower_bound(idAttachObj),
			endAttachObj = m_AttachObjRem.upper_bound(idAttachObj); itAttachObj != endAttachObj; ++itAttachObj)
	{
		if (idRlvObj == itAttachObj->second)
		{
			m_AttachObjRem.erase(itAttachObj);
			if(LLViewerObject *pObj = gObjectList.findObject(idAttachObj))
			{
				gInventory.addChangedMask(LLInventoryObserver::LABEL, pObj->getAttachmentItemID());
				gInventory.notifyObservers();
			}
			updateLockedHUD();
			break;
		}
	}
}
void RlvAttachmentLocks::removeAttachmentPointLock(S32 idxAttachPt, const LLUUID& idRlvObj, ERlvLockMask eLock)
{
	if (eLock & RLV_LOCK_REMOVE)
	{
		bool removed_entry = false;
		RLV_ASSERT( m_AttachPtRem.lower_bound(idxAttachPt) != m_AttachPtRem.upper_bound(idxAttachPt) );
		for (rlv_attachptlock_map_t::iterator itAttachPt = m_AttachPtRem.lower_bound(idxAttachPt),
				endAttachPt = m_AttachPtRem.upper_bound(idxAttachPt); itAttachPt != endAttachPt; ++itAttachPt)
		{
			if (idRlvObj == itAttachPt->second)
			{
				m_AttachPtRem.erase(itAttachPt);
				removed_entry = true;
				updateLockedHUD();
				break;
			}
		}
		if(removed_entry)
		{
			if(m_AttachPtRem.find(idxAttachPt) == m_AttachPtRem.end())
			{
				LLVOAvatar* pAvatar = gAgentAvatarp;
				if (pAvatar)
				{
					bool need_update = false;
					LLVOAvatar::attachment_map_t::iterator iter = pAvatar->mAttachmentPoints.find(idxAttachPt);
					if (iter != pAvatar->mAttachmentPoints.end())
					{
						for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = iter->second->mAttachedObjects.begin();
								attachment_iter != iter->second->mAttachedObjects.end();++attachment_iter)
						{
							LLViewerObject* attached_object = (*attachment_iter);
							if(attached_object)
							{
								gInventory.addChangedMask(LLInventoryObserver::LABEL, attached_object->getAttachmentItemID());
								need_update = true;
							}
						}
						if(need_update)
							gInventory.notifyObservers();
					}
				}
			}
		}
	}
	if (eLock & RLV_LOCK_ADD)
	{
		RLV_ASSERT( m_AttachPtAdd.lower_bound(idxAttachPt) != m_AttachPtAdd.upper_bound(idxAttachPt) );
		for (rlv_attachptlock_map_t::iterator itAttachPt = m_AttachPtAdd.lower_bound(idxAttachPt),
				endAttachPt = m_AttachPtAdd.upper_bound(idxAttachPt); itAttachPt != endAttachPt; ++itAttachPt)
		{
			if (idRlvObj == itAttachPt->second)
			{
				m_AttachPtAdd.erase(itAttachPt);
				break;
			}
		}
	}
}
void RlvAttachmentLocks::updateLockedHUD()
{
	if (!isAgentAvatarValid())
		return;
	m_fHasLockedHUD = false;
	for (LLVOAvatar::attachment_map_t::const_iterator itAttachPt = gAgentAvatarp->mAttachmentPoints.begin();
			itAttachPt != gAgentAvatarp->mAttachmentPoints.end(); ++itAttachPt)
	{
		const LLViewerJointAttachment* pAttachPt = itAttachPt->second;
		if ( (pAttachPt) && (pAttachPt->getIsHUDAttachment()) && (hasLockedAttachment(pAttachPt)) )
		{
			m_fHasLockedHUD = true;
			break;
		}
	}
	if (m_fHasLockedHUD)
	{
		LLPipeline::sShowHUDAttachments = TRUE;
		gUseWireframe = FALSE;
	}
}
bool RlvAttachmentLocks::verifyAttachmentLocks()
{
	bool fSuccess = true;
	rlv_attachobjlock_map_t::iterator itAttachObj = m_AttachObjRem.begin(), itCurrentObj;
	while (itAttachObj != m_AttachObjRem.end())
	{
		itCurrentObj = itAttachObj++;
		const LLViewerObject* pAttachObj = gObjectList.findObject(itCurrentObj->first);
		if ( (!pAttachObj) || (!pAttachObj->isAttachment()) )
		{
			m_AttachObjRem.erase(itCurrentObj);
			fSuccess = false;
		}
	}
	return fSuccess;
}
bool RlvAttachmentLockWatchdog::RlvWearInfo::isAddLockedAttachPt(S32 idxAttachPt) const
{
	return (attachPts.find(idxAttachPt) != attachPts.end());
}
void RlvAttachmentLockWatchdog::RlvWearInfo::dumpInstance() const
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	std::string strItemId = idItem.asString();
	std::string strTemp = llformat("Wear %s '%s' (%s)",
		(RLV_WEAR_ADD == eWearAction) ? "add" : "replace", (pItem) ? pItem->getName().c_str() : "missing", strItemId.c_str());
	RLV_INFOS << strTemp.c_str() << RLV_ENDL;
	if (!attachPts.empty())
	{
		std::string strEmptyAttachPt;
		for (std::map<S32, uuid_vec_t>::const_iterator itAttachPt = attachPts.begin(); itAttachPt != attachPts.end(); ++itAttachPt)
		{
			const LLViewerJointAttachment* pAttachPt =
				get_if_there(gAgentAvatarp->mAttachmentPoints, itAttachPt->first, static_cast<LLViewerJointAttachment*>(NULL));
			if (!itAttachPt->second.empty())
			{
				for (uuid_vec_t::const_iterator itAttach = itAttachPt->second.begin(); itAttach != itAttachPt->second.end(); ++itAttach)
				{
					pItem = gInventory.getItem(*itAttach);
					strItemId = (*itAttach).asString();
					strTemp = llformat("  -> %s : %s (%s)",
						pAttachPt->getName().c_str(), (pItem) ? pItem->getName().c_str() : "missing", strItemId.c_str());
					RLV_INFOS << strTemp.c_str() << RLV_ENDL;
				}
			}
			else
			{
				if (!strEmptyAttachPt.empty())
					strEmptyAttachPt += ", ";
				strEmptyAttachPt += pAttachPt->getName();
			}
		}
		if (!strEmptyAttachPt.empty())
			RLV_INFOS << "  -> " << strEmptyAttachPt << " : empty" << RLV_ENDL;
	}
	else
	{
		RLV_INFOS << "  -> no attachment point information" << RLV_ENDL;
	}
}
void RlvAttachmentLockWatchdog::detach(const LLViewerObject* pAttachObj)
{
	if (pAttachObj)
	{
		gMessageSystem->newMessage("ObjectDetach");
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, pAttachObj->getLocalID());
		if (std::find(m_PendingDetach.begin(), m_PendingDetach.end(), pAttachObj->getAttachmentItemID()) == m_PendingDetach.end())
			m_PendingDetach.push_back(pAttachObj->getAttachmentItemID());
		gMessageSystem->sendReliable(gAgent.getRegionHost() );
	}
}
void RlvAttachmentLockWatchdog::detach(S32 idxAttachPt, const uuid_vec_t& idsAttachObjExcept)
{
	const LLViewerJointAttachment* pAttachPt = RlvAttachPtLookup::getAttachPoint(idxAttachPt);
	if (!pAttachPt)
		return;
	std::vector<const LLViewerObject*> attachObjs;
	for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
			itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
	{
		const LLViewerObject* pAttachObj = *itAttachObj;
		if (idsAttachObjExcept.end() == std::find(idsAttachObjExcept.begin(), idsAttachObjExcept.end(), pAttachObj->getID()))
			attachObjs.push_back(pAttachObj);
	}
	if (!attachObjs.empty())
	{
		gMessageSystem->newMessage("ObjectDetach");
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		for (std::vector<const LLViewerObject*>::const_iterator itAttachObj = attachObjs.begin(); itAttachObj != attachObjs.end(); ++itAttachObj)
		{
			const LLViewerObject* pAttachObj = *itAttachObj;
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, pAttachObj->getLocalID());
			if (m_PendingDetach.end() == std::find(m_PendingDetach.begin(), m_PendingDetach.end(), pAttachObj->getAttachmentItemID()))
				m_PendingDetach.push_back(pAttachObj->getAttachmentItemID());
		}
		gMessageSystem->sendReliable(gAgent.getRegionHost());
	}
}
void RlvAttachmentLockWatchdog::onAttach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt)
{
	S32 idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(pAttachObj);
	const LLUUID& idAttachItem = (pAttachObj) ? pAttachObj->getAttachmentItemID() : LLUUID::null;
	RLV_ASSERT( (!isAgentAvatarValid()) || ((idxAttachPt) && (idAttachItem.notNull())) );
	if ( (!idxAttachPt) || (idAttachItem.isNull()) )
		return;
	rlv_attach_map_t::iterator itAttach = m_PendingAttach.lower_bound(idxAttachPt), itAttachEnd = m_PendingAttach.upper_bound(idxAttachPt);
	if (itAttach != itAttachEnd)
	{
		bool fPendingReattach = false;
		for (; itAttach != itAttachEnd; ++itAttach)
		{
			if (idAttachItem == itAttach->second.idItem)
			{
				fPendingReattach = true;
				RlvBehaviourNotifyHandler::onReattach(pAttachPt, true);
				m_PendingAttach.erase(itAttach);
				break;
			}
		}
		if (!fPendingReattach)
		{
			detach(pAttachObj);
			RlvBehaviourNotifyHandler::onAttach(pAttachPt, false);
		}
		return;
	}
	rlv_wear_map_t::iterator itWear = m_PendingWear.find(idAttachItem); bool fAttachAllowed = true;
	if (itWear != m_PendingWear.end())
	{
		if (itWear->second.isAddLockedAttachPt(idxAttachPt))
		{
			std::map<S32, uuid_vec_t>::iterator itAttachPrev = itWear->second.attachPts.find(idxAttachPt);
			RLV_ASSERT(itAttachPrev != itWear->second.attachPts.end());
			if (std::find(itAttachPrev->second.begin(), itAttachPrev->second.end(), idAttachItem) == itAttachPrev->second.end())
			{
				if (itAttachPrev->second.empty())
				{
					detach(idxAttachPt);
				}
				else
				{
					for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
							itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
					{
						const LLViewerObject* p_attach_obj = *itAttachObj;
						uuid_vec_t::iterator it_attach =
							std::find(itAttachPrev->second.begin(), itAttachPrev->second.end(), p_attach_obj->getAttachmentItemID());
						if (it_attach == itAttachPrev->second.end())
							detach(p_attach_obj);
						else
							itAttachPrev->second.erase(it_attach);
					}
					for (uuid_vec_t::const_iterator it_attach = itAttachPrev->second.begin();
							it_attach != itAttachPrev->second.end(); ++it_attach)
					{
						m_PendingAttach.insert(std::pair<S32, RlvReattachInfo>(idxAttachPt, RlvReattachInfo(*it_attach)));
					}
				}
				fAttachAllowed = false;
			}
		}
		else if (RLV_WEAR_REPLACE == itWear->second.eWearAction)
		{
			uuid_vec_t idsAttachObjExcept;
			for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
					((itAttachObj != pAttachPt->mAttachedObjects.end()) && (fAttachAllowed)); ++itAttachObj)
			{
				if ( (pAttachObj != *itAttachObj) && (gRlvAttachmentLocks.isLockedAttachment(*itAttachObj)) )
				{
					if (gSavedSettings.getBOOL("RLVaWearReplaceUnlocked"))
						idsAttachObjExcept.push_back((*itAttachObj)->getID());
					else
						fAttachAllowed = false;
				}
			}
			if (fAttachAllowed)
			{
				idsAttachObjExcept.push_back(pAttachObj->getID());
				detach(idxAttachPt, idsAttachObjExcept);
			}
			else
			{
				detach(pAttachObj);
			}
		}
		m_PendingWear.erase(itWear);
	}
	RlvBehaviourNotifyHandler::onAttach(pAttachPt, fAttachAllowed);
}
void RlvAttachmentLockWatchdog::onDetach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt)
{
	S32 idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(pAttachPt);
	const LLUUID& idAttachItem = (pAttachObj) ? pAttachObj->getAttachmentItemID() : LLUUID::null;
	RLV_ASSERT( (!isAgentAvatarValid()) || ((idxAttachPt) && (idAttachItem.notNull())) );
	if ( (!idxAttachPt) || (idAttachItem.isNull()) )
		return;
	rlv_detach_map_t::iterator itDetach = std::find(m_PendingDetach.begin(), m_PendingDetach.end(), idAttachItem);
	if (itDetach != m_PendingDetach.end())
	{
		m_PendingDetach.erase(itDetach);
		RlvBehaviourNotifyHandler::onDetach(pAttachPt, true);
		return;
	}
	bool fDetachAllowed = true;
	if (gRlvAttachmentLocks.isLockedAttachment(pAttachObj))
	{
		bool fPendingAttach = false;
		for (rlv_attach_map_t::const_iterator itReattach = m_PendingAttach.lower_bound(idxAttachPt),
				itReattachEnd = m_PendingAttach.upper_bound(idxAttachPt); itReattach != itReattachEnd; ++itReattach)
		{
			if (itReattach->second.idItem == idAttachItem)
			{
				fPendingAttach = true;
				break;
			}
		}
		if (!fPendingAttach)
		{
			m_PendingAttach.insert(std::pair<S32, RlvReattachInfo>(idxAttachPt, RlvReattachInfo(idAttachItem)));
			startTimer();
		}
		fDetachAllowed = false;
	}
	RlvBehaviourNotifyHandler::onDetach(pAttachPt, fDetachAllowed);
}
void RlvAttachmentLockWatchdog::onSavedAssetIntoInventory(const LLUUID& idItem)
{
	for (rlv_attach_map_t::iterator itAttach = m_PendingAttach.begin(); itAttach != m_PendingAttach.end(); ++itAttach)
	{
		if ( (!itAttach->second.fAssetSaved) && (idItem == itAttach->second.idItem) )
		{
			LLAttachmentsMgr::instance().addAttachmentRequest(itAttach->second.idItem, itAttach->first, true, true);
			itAttach->second.tsAttach = LLFrameTimer::getElapsedSeconds();
		}
	}
}
BOOL RlvAttachmentLockWatchdog::onTimer()
{
	F64 tsCurrent = LLFrameTimer::getElapsedSeconds();
	rlv_wear_map_t::iterator itWear = m_PendingWear.begin();
	while (itWear != m_PendingWear.end())
	{
		if (itWear->second.tsWear + 60 < tsCurrent)
			m_PendingWear.erase(itWear++);
		else
			++itWear;
	}
	rlv_attach_map_t::iterator itAttach = m_PendingAttach.begin();
	while (itAttach != m_PendingAttach.end())
	{
		if (gInventory.getItem(itAttach->second.idItem) == NULL)
		{
			m_PendingAttach.erase(itAttach++);
			continue;
		}
		bool fAttach = false;
		if ( (!itAttach->second.fAssetSaved) && (itAttach->second.tsDetach + 15 < tsCurrent) )
		{
			itAttach->second.fAssetSaved = true;
			fAttach = true;
		}
		else if ( (itAttach->second.fAssetSaved) && (itAttach->second.tsAttach + 30 < tsCurrent) )
		{
			fAttach = true;
		}
		if (fAttach)
		{
			LLAttachmentsMgr::instance().addAttachmentRequest(itAttach->second.idItem, itAttach->first, true, true);
			itAttach->second.tsAttach = tsCurrent;
		}
		++itAttach;
	}
	return ( (m_PendingAttach.empty()) && (m_PendingDetach.empty()) && (m_PendingWear.empty()) );
}
void RlvAttachmentLockWatchdog::onWearAttachment(const LLUUID& idItem, ERlvWearMask eWearAction)
{
	RLV_ASSERT(idItem.notNull());
	if ( (idItem.isNull()) || (!isAgentAvatarValid()) || (!gRlvAttachmentLocks.hasLockedAttachmentPoint(RLV_LOCK_ANY)) )
		return;
	RlvWearInfo infoWear(idItem, eWearAction);
	RLV_ASSERT( (RLV_WEAR_ADD == eWearAction) || (RLV_WEAR_REPLACE == eWearAction) );
	for (LLVOAvatar::attachment_map_t::const_iterator itAttachPt = gAgentAvatarp->mAttachmentPoints.begin();
			itAttachPt != gAgentAvatarp->mAttachmentPoints.end(); ++itAttachPt)
	{
		const LLViewerJointAttachment* pAttachPt = itAttachPt->second;
		if (gRlvAttachmentLocks.isLockedAttachmentPoint(pAttachPt, RLV_LOCK_ADD))
		{
			uuid_vec_t attachObjs;
			for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
					itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
			{
				const LLViewerObject* pAttachObj = *itAttachObj;
				if (std::find(m_PendingDetach.begin(), m_PendingDetach.end(), pAttachObj->getAttachmentItemID()) != m_PendingDetach.end())
					continue;
				attachObjs.push_back(pAttachObj->getAttachmentItemID());
			}
			infoWear.attachPts.insert(std::pair<S32, uuid_vec_t>(itAttachPt->first, attachObjs));
		}
	}
	m_PendingWear.insert(std::pair<LLUUID, RlvWearInfo>(idItem, infoWear));
#ifdef RLV_DEBUG
	infoWear.dumpInstance();
#endif
	startTimer();
}
RlvWearableLocks gRlvWearableLocks;
void RlvWearableLocks::addWearableTypeLock(LLWearableType::EType eType, const LLUUID& idRlvObj, ERlvLockMask eLock)
{
	if (eLock & RLV_LOCK_REMOVE)
	{
		m_WearableTypeRem.insert(std::pair<LLWearableType::EType, LLUUID>(eType, idRlvObj));
		bool bChanged = false;
		for (U32 idxWearable = 0, cntWearable = gAgentWearables.getWearableCount(eType); idxWearable < cntWearable; idxWearable++)
		{
			const LLUUID& item_id = gAgentWearables.getWearableItemID(eType, idxWearable);
			if(item_id.notNull())
			{
				bChanged = true;
				gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
			}
		}
		if(bChanged)
			gInventory.notifyObservers();
	}
	if (eLock & RLV_LOCK_ADD)
		m_WearableTypeAdd.insert(std::pair<LLWearableType::EType, LLUUID>(eType, idRlvObj));
}
bool RlvWearableLocks::canRemove(LLWearableType::EType eType) const
{
	for (U32 idxWearable = 0, cntWearable = gAgentWearables.getWearableCount(eType); idxWearable < cntWearable; idxWearable++)
		if (!isLockedWearable(gAgentWearables.getViewerWearable(eType, idxWearable)))
			return true;
	return false;
}
bool RlvWearableLocks::hasLockedWearable(LLWearableType::EType eType) const
{
	for (U32 idxWearable = 0, cntWearable = gAgentWearables.getWearableCount(eType); idxWearable < cntWearable; idxWearable++)
		if (isLockedWearable(gAgentWearables.getViewerWearable(eType, idxWearable)))
			return true;
	return false;
}
bool RlvWearableLocks::isLockedWearableExcept(const LLViewerWearable* pWearable, const LLUUID& idRlvObj) const
{
	if (idRlvObj.isNull())
		return isLockedWearable(pWearable);
	return (pWearable) && (isLockedWearableTypeExcept(pWearable->getType(), RLV_LOCK_REMOVE, idRlvObj));
}
bool RlvWearableLocks::isLockedWearableTypeExcept(LLWearableType::EType eType, ERlvLockMask eLock, const LLUUID& idRlvObj) const
{
	if (idRlvObj.isNull())
		return isLockedWearableType(eType, eLock);
	if (eLock & RLV_LOCK_REMOVE)
	{
		for (rlv_wearabletypelock_map_t::const_iterator itWearableType = m_WearableTypeRem.lower_bound(eType),
				endWearableType = m_WearableTypeRem.upper_bound(eType); itWearableType != endWearableType; ++itWearableType)
		{
			if (itWearableType->second != idRlvObj)
				return true;
		}
	}
	if (eLock & RLV_LOCK_ADD)
	{
		for (rlv_wearabletypelock_map_t::const_iterator itWearableType = m_WearableTypeAdd.lower_bound(eType),
				endWearableType = m_WearableTypeAdd.upper_bound(eType); itWearableType != endWearableType; ++itWearableType)
		{
			if (itWearableType->second != idRlvObj)
				return true;
		}
	}
	return false;
}
void RlvWearableLocks::removeWearableTypeLock(LLWearableType::EType eType, const LLUUID& idRlvObj, ERlvLockMask eLock)
{
	if (eLock & RLV_LOCK_REMOVE)
	{
		RLV_ASSERT( m_WearableTypeRem.lower_bound(eType) != m_WearableTypeRem.upper_bound(eType) );
		bool removed_entry = false;
		for (rlv_wearabletypelock_map_t::iterator itWearableType = m_WearableTypeRem.lower_bound(eType),
				endWearableType = m_WearableTypeRem.upper_bound(eType); itWearableType != endWearableType; ++itWearableType)
		{
			if (idRlvObj == itWearableType->second)
			{
				m_WearableTypeRem.erase(itWearableType);
				removed_entry = true;
				break;
			}
		}
		if(removed_entry)
		{
			if(m_WearableTypeRem.find(eType) == m_WearableTypeRem.end())
			{
				bool bChanged = false;
				for (U32 idxWearable = 0, cntWearable = gAgentWearables.getWearableCount(eType); idxWearable < cntWearable; idxWearable++)
				{
					const LLUUID& item_id = gAgentWearables.getWearableItemID(eType, idxWearable);
					if(item_id.notNull())
					{
						bChanged = true;
						gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
					}
				}
				if(bChanged)
					gInventory.notifyObservers();
			}
		}
	}
	if (eLock & RLV_LOCK_ADD)
	{
		RLV_ASSERT( m_WearableTypeAdd.lower_bound(eType) != m_WearableTypeAdd.upper_bound(eType) );
		for (rlv_wearabletypelock_map_t::iterator itWearableType = m_WearableTypeAdd.lower_bound(eType),
				endWearableType = m_WearableTypeAdd.upper_bound(eType); itWearableType != endWearableType; ++itWearableType)
		{
			if (idRlvObj == itWearableType->second)
			{
				m_WearableTypeAdd.erase(itWearableType);
				break;
			}
		}
	}
}
class RlvLockedDescendentsCollector : public LLInventoryCollectFunctor
{
public:
	RlvLockedDescendentsCollector(int eSourceTypeMask, RlvFolderLocks::ELockPermission ePermMask, ERlvLockMask eLockTypeMask)
		: m_ePermMask(ePermMask), m_eSourceTypeMask(eSourceTypeMask), m_eLockTypeMask(eLockTypeMask) {}
	~RlvLockedDescendentsCollector() {}
	bool operator()(LLInventoryCategory* pFolder, LLInventoryItem* pItem)
	{
		return (pFolder) && (RlvFolderLocks::instance().isLockedFolderEntry(pFolder->getUUID(), m_eSourceTypeMask, m_ePermMask, m_eLockTypeMask));
	}
protected:
	RlvFolderLocks::ELockPermission m_ePermMask;
	int				m_eSourceTypeMask;
	ERlvLockMask	m_eLockTypeMask;
};
RlvFolderLocks::RlvFolderLocks()
	: m_cntLockAdd(0), m_cntLockRem(0), m_fLookupDirty(false), m_fLockedRoot(false)
{
	LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&RlvFolderLocks::onNeedsLookupRefresh, this));
	RlvInventory::instance().addSharedRootIDChangedCallback(boost::bind(&RlvFolderLocks::onNeedsLookupRefresh, this));
}
void RlvFolderLocks::addFolderLock(const folderlock_source_t& lockSource, ELockPermission ePerm, ELockScope eScope,
								   const LLUUID& idRlvObj, ERlvLockMask eLockType)
{
	RLV_ASSERT( (RLV_LOCK_ADD == eLockType) || (RLV_LOCK_REMOVE == eLockType) );
	m_FolderLocks.push_back(new folderlock_descr_t(idRlvObj, eLockType, lockSource, ePerm, eScope));
	if (PERM_DENY == ePerm)
	{
		if (RLV_LOCK_REMOVE == eLockType)
			m_cntLockRem++;
		else if (RLV_LOCK_ADD == eLockType)
			m_cntLockAdd++;
	}
	m_fLookupDirty = true;
}
bool RlvFolderLocks::getLockedFolders(const folderlock_source_t& lockSource, LLInventoryModel::cat_array_t& lockFolders) const
{
	S32 cntFolders = lockFolders.size();
	switch (lockSource.first)
	{
		case ST_ATTACHMENT:
			{
				RLV_ASSERT(typeid(LLUUID) == lockSource.second.type())
				const LLViewerObject* pObj = gObjectList.findObject(boost::get<LLUUID>(lockSource.second));
				if ( (pObj) && (pObj->isAttachment()) )
				{
					const LLViewerInventoryItem* pItem = gInventory.getItem(pObj->getAttachmentItemID());
					if ( (pItem) && (RlvInventory::instance().isSharedFolder(pItem->getParentUUID())) )
					{
						LLViewerInventoryCategory* pItemFolder = gInventory.getCategory(pItem->getParentUUID());
						if (pItemFolder)
							lockFolders.push_back(pItemFolder);
					}
				}
			}
			break;
		case ST_FOLDER:
			{
				RLV_ASSERT(typeid(LLUUID) == lockSource.second.type())
				LLViewerInventoryCategory* pFolder = gInventory.getCategory(boost::get<LLUUID>(lockSource.second));
				if (pFolder)
					lockFolders.push_back(pFolder);
			}
			break;
		case ST_ROOTFOLDER:
			{
				LLViewerInventoryCategory* pFolder = gInventory.getCategory(gInventory.getRootFolderID());
				if (pFolder)
					lockFolders.push_back(pFolder);
			}
			break;
		case ST_SHAREDPATH:
			{
				RLV_ASSERT(typeid(std::string) == lockSource.second.type())
				LLViewerInventoryCategory* pSharedFolder = RlvInventory::instance().getSharedFolder(boost::get<std::string>(lockSource.second));
				if (pSharedFolder)
					lockFolders.push_back(pSharedFolder);
			}
			break;
		case ST_ATTACHMENTPOINT:
		case ST_WEARABLETYPE:
			{
				RLV_ASSERT( ((ST_ATTACHMENTPOINT == lockSource.first) && (typeid(S32) == lockSource.second.type())) ||
					        ((ST_WEARABLETYPE == lockSource.first) && (typeid(LLWearableType::EType) == lockSource.second.type())) );
				uuid_vec_t idItems;
				if (ST_ATTACHMENTPOINT == lockSource.first)
					RlvCommandOptionGetPath::getItemIDs(RlvAttachPtLookup::getAttachPoint(boost::get<S32>(lockSource.second)), idItems);
				else if (ST_WEARABLETYPE == lockSource.first)
					RlvCommandOptionGetPath::getItemIDs(boost::get<LLWearableType::EType>(lockSource.second), idItems);
				LLInventoryModel::cat_array_t itemFolders;
				if (RlvInventory::instance().getPath(idItems, itemFolders))
					lockFolders.insert(lockFolders.end(), itemFolders.begin(), itemFolders.end());
			}
			break;
		default:
			return false;
	};
	return cntFolders != lockFolders.size();
}
bool RlvFolderLocks::getLockedItems(const LLUUID& idFolder, LLInventoryModel::item_array_t& lockItems) const
{
	S32 cntItems = lockItems.size();
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	LLFindWearablesEx f(true, true);
	gInventory.collectDescendentsIf(idFolder, folders, items, FALSE, f);
	std::map<LLUUID, bool> folderLookups; std::map<LLUUID, bool>::const_iterator itLookup;
	bool fItemLocked = false;
	for (S32 idxItem = 0, cntItem = items.size(); idxItem < cntItem; idxItem++)
	{
		LLViewerInventoryItem* pItem = items.at(idxItem);
		if (LLAssetType::AT_LINK == pItem->getActualType())
			pItem = pItem->getLinkedItem();
		if (!pItem)
			continue;
		const LLUUID& idItemParent = RlvInventory::getFoldedParent(pItem->getParentUUID(), true);
		if ((itLookup = folderLookups.find(idItemParent)) != folderLookups.end())
		{
			fItemLocked = itLookup->second;
		}
		else
		{
			fItemLocked = isLockedFolder(idItemParent, RLV_LOCK_REMOVE);
			folderLookups.insert(std::pair<LLUUID, bool>(idItemParent, fItemLocked));
		}
		if (!fItemLocked)
		{
			LLInventoryModel::item_array_t itemLinks;
			LLInventoryModel::cat_array_t cats;
			LLLinkedItemIDMatches item_id_matches(pItem->getUUID());
			gInventory.collectDescendentsIf(RlvInventory::instance().getSharedRootID(), cats, itemLinks, LLInventoryModel::EXCLUDE_TRASH, item_id_matches);
			for (LLInventoryModel::item_array_t::iterator itItemLink = itemLinks.begin();
					(itItemLink < itemLinks.end()) && (!fItemLocked); ++itItemLink)
			{
				LLViewerInventoryItem* pItemLink = *itItemLink;
				const LLUUID& idItemLinkParent = (pItemLink) ? RlvInventory::getFoldedParent(pItemLink->getParentUUID(), true) : LLUUID::null;
				if ((itLookup = folderLookups.find(idItemLinkParent)) != folderLookups.end())
				{
					fItemLocked = itLookup->second;
				}
				else
				{
					fItemLocked = isLockedFolder(idItemLinkParent, RLV_LOCK_REMOVE);
					folderLookups.insert(std::pair<LLUUID, bool>(idItemLinkParent, fItemLocked));
				}
			}
		}
		if (fItemLocked)
			lockItems.push_back(pItem);
	}
	return cntItems != lockItems.size();
}
bool RlvFolderLocks::hasLockedFolderDescendent(const LLUUID& idFolder, int eSourceTypeMask, ELockPermission ePermMask,
											   ERlvLockMask eLockTypeMask, bool fCheckSelf) const
{
	if (!hasLockedFolder(eLockTypeMask))
		return false;
	if (m_fLookupDirty)
		refreshLockedLookups();
	if ( (fCheckSelf) && (isLockedFolderEntry(idFolder, eSourceTypeMask, ePermMask, RLV_LOCK_ANY)) )
		return true;
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	RlvLockedDescendentsCollector f(eSourceTypeMask, ePermMask, eLockTypeMask);
	gInventory.collectDescendentsIf(idFolder, folders, items, FALSE, f, false);
	return !folders.empty();
}
bool RlvFolderLocks::isLockedFolderEntry(const LLUUID& idFolder, int eSourceTypeMask, ELockPermission ePermMask, ERlvLockMask eLockTypeMask) const
{
	for (folderlock_map_t::const_iterator itFolderLock = m_LockedFolderMap.lower_bound(idFolder),
			endFolderLock = m_LockedFolderMap.upper_bound(idFolder); itFolderLock != endFolderLock; ++itFolderLock)
	{
		const folderlock_descr_t* pLockDescr = itFolderLock->second;
		if ( (pLockDescr->lockSource.first & eSourceTypeMask) && (pLockDescr->eLockPermission & ePermMask) &&
			 (pLockDescr->eLockType & eLockTypeMask) )
		{
			return true;
		}
	}
	return false;
}
bool RlvFolderLocks::isLockedFolder(LLUUID idFolder, ERlvLockMask eLockTypeMask, int eSourceTypeMask, folderlock_source_t* plockSource) const
{
	if (!hasLockedFolder(eLockTypeMask))
		return false;
	idFolder = RlvInventory::getFoldedParent(idFolder, true);
	if (idFolder.isNull())
		return false;
	if (m_fLookupDirty)
		refreshLockedLookups();
	std::list<LLUUID> idsRlvObjRem, idsRlvObjAdd; const LLUUID& idFolderRoot = gInventory.getRootFolderID(); LLUUID idFolderCur = idFolder;
	while (idFolderRoot != idFolderCur)
	{
		for (folderlock_map_t::const_iterator itFolderLock = m_LockedFolderMap.lower_bound(idFolderCur),
				endFolderLock = m_LockedFolderMap.upper_bound(idFolderCur); itFolderLock != endFolderLock; ++itFolderLock)
		{
			const folderlock_descr_t* pLockDescr = itFolderLock->second;
			ERlvLockMask eCurLockType = static_cast<ERlvLockMask>(pLockDescr->eLockType & eLockTypeMask);
			std::list<LLUUID>* pidRlvObjList = (RLV_LOCK_REMOVE == eCurLockType) ? &idsRlvObjRem : &idsRlvObjAdd;
			if ( (0 == eCurLockType) || ((SCOPE_NODE == pLockDescr->eLockScope) && (idFolder != idFolderCur)) ||
				 (pidRlvObjList->end() != std::find(pidRlvObjList->begin(), pidRlvObjList->end(), pLockDescr->idRlvObj)) ||
				 (0 == (pLockDescr->eLockType & eSourceTypeMask)) )
			{
				continue;
			}
			if (PERM_DENY == pLockDescr->eLockPermission)
			{
				if (plockSource)
					*plockSource = pLockDescr->lockSource;
				return true;
			}
			else if (PERM_ALLOW == pLockDescr->eLockPermission)
			{
				pidRlvObjList->push_back(pLockDescr->idRlvObj);
			}
		}
		const LLViewerInventoryCategory* pParent = gInventory.getCategory(idFolderCur);
		idFolderCur = (pParent) ? pParent->getParentUUID() : idFolderRoot;
	}
	return (m_fLockedRoot) && (idsRlvObjRem.empty()) && (idsRlvObjAdd.empty());
}
void RlvFolderLocks::onNeedsLookupRefresh() const
{
	m_fLookupDirty |= !m_FolderLocks.empty();
}
void RlvFolderLocks::refreshLockedLookups() const
{
	m_fLockedRoot = false;
	m_LockedFolderMap.clear();
	for (folderlock_list_t::const_iterator itFolderLock = m_FolderLocks.begin(); itFolderLock != m_FolderLocks.end(); ++itFolderLock)
	{
		const folderlock_descr_t* pLockDescr = *itFolderLock;
		LLInventoryModel::cat_array_t lockedFolders; const LLUUID& idFolderRoot = gInventory.getRootFolderID();
		if (getLockedFolders(pLockDescr->lockSource, lockedFolders))
		{
			for (S32 idxFolder = 0, cntFolder = lockedFolders.size(); idxFolder < cntFolder; idxFolder++)
			{
				const LLViewerInventoryCategory* pFolder = lockedFolders.at(idxFolder);
				if (idFolderRoot != pFolder->getUUID())
					m_LockedFolderMap.insert(std::pair<LLUUID, const folderlock_descr_t*>(pFolder->getUUID(), pLockDescr));
				else
					m_fLockedRoot |= (SCOPE_SUBTREE == pLockDescr->eLockScope);
			}
		}
	}
	m_fLookupDirty = false;
	m_LockedAttachmentRem.clear();
	m_LockedWearableRem.clear();
	LLInventoryModel::item_array_t lockedItems;
	if (getLockedItems(LLAppearanceMgr::instance().getCOF(), lockedItems))
	{
		for (S32 idxItem = 0, cntItem = lockedItems.size(); idxItem < cntItem; idxItem++)
		{
			const LLViewerInventoryItem* pItem = lockedItems.at(idxItem);
			switch (pItem->getType())
			{
				case LLAssetType::AT_BODYPART:
				case LLAssetType::AT_CLOTHING:
					m_LockedWearableRem.push_back(pItem->getLinkedUUID());
					break;
				case LLAssetType::AT_OBJECT:
					m_LockedAttachmentRem.push_back(pItem->getLinkedUUID());
					break;
				default:
					RLV_ASSERT(true);
					break;
			}
		}
	}
	std::sort(m_LockedAttachmentRem.begin(), m_LockedAttachmentRem.end());
	m_LockedAttachmentRem.erase(std::unique(m_LockedAttachmentRem.begin(), m_LockedAttachmentRem.end()), m_LockedAttachmentRem.end());
	std::sort(m_LockedWearableRem.begin(), m_LockedWearableRem.end());
	m_LockedWearableRem.erase(std::unique(m_LockedWearableRem.begin(), m_LockedWearableRem.end()), m_LockedWearableRem.end());
}
void RlvFolderLocks::removeFolderLock(const folderlock_source_t& lockSource, ELockPermission ePerm, ELockScope eScope,
									  const LLUUID& idRlvObj, ERlvLockMask eLockType)
{
	RLV_ASSERT( (RLV_LOCK_ADD == eLockType) || (RLV_LOCK_REMOVE == eLockType) );
	folderlock_descr_t lockDescr(idRlvObj, eLockType, lockSource, ePerm, eScope); RlvPredValuesEqual<folderlock_descr_t> f = { &lockDescr };
	folderlock_list_t::iterator itFolderLock = std::find_if(m_FolderLocks.begin(), m_FolderLocks.end(), f);
	RLV_ASSERT( m_FolderLocks.end() != itFolderLock  );
	if (m_FolderLocks.end() != itFolderLock)
	{
		delete *itFolderLock;
		m_FolderLocks.erase(itFolderLock);
		if (PERM_DENY == ePerm)
		{
			if (RLV_LOCK_REMOVE == eLockType)
				m_cntLockRem--;
			else if (RLV_LOCK_ADD == eLockType)
				m_cntLockAdd--;
		}
		m_fLookupDirty = true;
	}
}
