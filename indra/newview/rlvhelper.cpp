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
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "llgesturemgr.h"
#include "llnotificationsutil.h"
#include "llviewerobjectlist.h"
#include "rlvhelper.h"
#include "rlvhandler.h"
#include "rlvinventory.h"
#include <boost/algorithm/string.hpp>
RlvCommand::bhvr_map_t RlvCommand::m_BhvrMap;
RlvCommand::RlvCommand(const LLUUID& idObj, const std::string& strCommand)
	: m_fValid(false), m_idObj(idObj), m_eBehaviour(RLV_BHVR_UNKNOWN), m_fStrict(false), m_eParamType(RLV_TYPE_UNKNOWN), m_eRet(RLV_RET_UNKNOWN)
{
	if ((m_fValid = parseCommand(strCommand, m_strBehaviour, m_strOption, m_strParam)))
	{
		S32 nTemp = 0;
		if ( ("n" == m_strParam) || ("add" == m_strParam) )
			m_eParamType = RLV_TYPE_ADD;
		else if ( ("y" == m_strParam) || ("rem" == m_strParam) )
			m_eParamType = RLV_TYPE_REMOVE;
		else if (m_strBehaviour == "clear")
			m_eParamType = RLV_TYPE_CLEAR;
		else if ("force" == m_strParam)
			m_eParamType = RLV_TYPE_FORCE;
		else if (LLStringUtil::convertToS32(m_strParam, nTemp))
			m_eParamType = RLV_TYPE_REPLY;
		else
		{
			m_eParamType = RLV_TYPE_UNKNOWN;
			m_fValid = false;
		}
	}
	if (!m_fValid)
	{
		m_strBehaviour = m_strOption = m_strParam = "";
		return;
	}
	if ( (RLV_TYPE_FORCE == m_eParamType) &&
		 (m_strBehaviour.length() > 13) && (m_strBehaviour.length() - 13 == m_strBehaviour.rfind("overorreplace")) )
	{
		m_strBehaviour.erase(m_strBehaviour.length() - 13, 13);
	}
	if ( (RLV_TYPE_FORCE == m_eParamType) && (0 == m_strBehaviour.find("addoutfit")) )
	{
		m_strBehaviour.replace(0, 9, "attach");
	}
	m_eBehaviour = getBehaviourFromString(m_strBehaviour, &m_fStrict);
}
bool RlvCommand::parseCommand(const std::string& strCommand, std::string& strBehaviour, std::string& strOption, std::string& strParam)
{
	int idxParam  = strCommand.find('=');
	int idxOption = (idxParam > 0) ? strCommand.find(':') : -1;
	if (idxOption > idxParam - 1)
		idxOption = -1;
	if ( (0 == idxOption) || (0 == idxParam) )
		return false;
	strBehaviour = strCommand.substr(0, (-1 != idxOption) ? idxOption : idxParam);
	strOption = strParam = "";
	if ( (-1 == idxParam) || (static_cast<int>(strCommand.length()) - 1 == idxParam) )
	{
		if ( ("clear" == strBehaviour) && ( (!idxOption) || (idxParam) ) )
			return true;
		return false;
	}
	if ( (-1 != idxOption) && (idxOption + 1 != idxParam) )
		strOption = strCommand.substr(idxOption + 1, idxParam - idxOption - 1);
	strParam = strCommand.substr(idxParam + 1);
	return true;
}
ERlvBehaviour RlvCommand::getBehaviourFromString(const std::string& strBhvr, bool* pfStrict )
{
	std::string::size_type idxStrict = strBhvr.find("_sec");
	bool fStrict = (std::string::npos != idxStrict) && (idxStrict + 4 == strBhvr.length());
	if (pfStrict)
		*pfStrict = fStrict;
	RLV_ASSERT(m_BhvrMap.size() > 0);
	bhvr_map_t::const_iterator itBhvr = m_BhvrMap.find( (!fStrict) ? strBhvr : strBhvr.substr(0, idxStrict));
	if ( (itBhvr != m_BhvrMap.end()) && ((!fStrict) || (hasStrictVariant(itBhvr->second))) )
		return itBhvr->second;
	return RLV_BHVR_UNKNOWN;
}
bool RlvCommand::getCommands(bhvr_map_t& cmdList, const std::string &strMatch)
{
	if (strMatch.empty())
		return false;
	cmdList.clear();
	RLV_ASSERT(m_BhvrMap.size() > 0);
	for (bhvr_map_t::const_iterator itBhvr = m_BhvrMap.begin(); itBhvr != m_BhvrMap.end(); ++itBhvr)
	{
		std::string strCmd = itBhvr->first; ERlvBehaviour eBhvr = itBhvr->second;
		if (std::string::npos != strCmd.find(strMatch))
			cmdList.insert(std::pair<std::string, ERlvBehaviour>(strCmd, eBhvr));
		if ( (hasStrictVariant(eBhvr)) && (std::string::npos != strCmd.append("_sec").find(strMatch)) )
			cmdList.insert(std::pair<std::string, ERlvBehaviour>(strCmd, eBhvr));
	}
	return (0 != cmdList.size());
}
void RlvCommand::initLookupTable()
{
	static bool fInitialized = false;
	if (!fInitialized)
	{
		std::string arBehaviours[RLV_BHVR_COUNT] =
			{
				"detach", "attach", "addattach", "remattach", "addoutfit", "remoutfit", "sharedwear", "sharedunwear",
				"unsharedwear", "unsharedunwear", "emote", "sendchat", "recvchat", "recvchatfrom", "recvemote", "recvemotefrom",
				"redirchat", "rediremote", "chatwhisper", "chatnormal", "chatshout", "sendchannel", "sendim", "sendimto",
				"recvim", "recvimfrom", "startim", "startimto", "permissive", "notify", "showinv", "showminimap", "showworldmap", "showloc",
				"shownames", "shownametags", "showhovertext", "showhovertexthud", "showhovertextworld", "showhovertextall", "tplm", "tploc", "tplure", "tprequest",
				"viewnote", "viewscript", "viewtexture", "acceptpermission", "accepttp", "accepttprequest", "allowidle", "edit", "editobj", "rez",
				"fartouch", "interact", "touchthis", "touchattach", "touchattachself", "touchattachother", "touchhud", "touchworld", "touchall",
				"touchme", "fly", "setgroup", "unsit", "sit", "sittp", "standtp", "setdebug", "setenv", "alwaysrun", "temprun", "detachme",
				"attachover", "attachthis", "attachthisover", "attachthis_except", "detachthis", "detachthis_except", "attachall",
				"attachallover", "detachall", "attachallthis", "attachallthis_except", "attachallthisover", "detachallthis",
				"detachallthis_except", "adjustheight", "camzoommax", "camzoommin", "camdistmax", "camdistmin",
				"TODOcamdrawmax", "TODOcamdrawmin", "TODOcamdrawalphamax", "TODOcamdrawalphamin", "TODOcamdrawcolor", "camunlock", "camavdist", "TODOcamtextures",
				"tpto", "version", "versionnew", "versionnum", "getattach", "getattachnames",
				"getaddattachnames", "getremattachnames", "getoutfit", "getoutfitnames", "getaddoutfitnames", "getremoutfitnames",
				"findfolder", "findfolders", "getpath", "getpathnew", "getinv", "getinvworn", "getgroup", "getsitid", "getcommand",
				"getstatus", "getstatusall"
			};
		for (int idxBvhr = 0; idxBvhr < RLV_BHVR_COUNT; idxBvhr++)
			m_BhvrMap.insert(std::pair<std::string, ERlvBehaviour>(arBehaviours[idxBvhr], (ERlvBehaviour)idxBvhr));
		fInitialized = true;
	}
}
RlvCommandOptionGeneric::RlvCommandOptionGeneric(const std::string& strOption): m_fEmpty(false)
{
	LLWearableType::EType wtType(LLWearableType::WT_INVALID); LLUUID idOption; ERlvAttachGroupType eAttachGroup(RLV_ATTACHGROUP_INVALID);
	LLViewerJointAttachment* pAttachPt = NULL; LLViewerInventoryCategory* pFolder = NULL;
	if (!(m_fEmpty = strOption.empty()))
	{
		if ( ((wtType = LLWearableType::typeNameToType(strOption)) != LLWearableType::WT_INVALID) && (wtType != LLWearableType::WT_NONE) )
			m_varOption = wtType;
		else if ((pAttachPt = RlvAttachPtLookup::getAttachPoint(strOption)) != NULL)
			m_varOption = pAttachPt;
		else if ( ((UUID_STR_LENGTH - 1) == strOption.length()) && (idOption.set(strOption)) )
			m_varOption = idOption;
		else if ((pFolder = RlvInventory::instance().getSharedFolder(strOption)) != NULL)
			m_varOption = pFolder;
		else if ((eAttachGroup = rlvAttachGroupFromString(strOption)) != RLV_ATTACHGROUP_INVALID)
			m_varOption = eAttachGroup;
		else
			m_varOption = strOption;
	}
	m_fValid = true;
}
class RlvCommandOptionGetPathCallback
{
public:
	RlvCommandOptionGetPathCallback(const LLUUID& idAttachObj, RlvCommandOptionGetPath::getpath_callback_t cb)
		: mObjectId(idAttachObj), mCallback(cb)
	{
		if (isAgentAvatarValid())
			mAttachmentConnection = gAgentAvatarp->setAttachmentCallback(boost::bind(&RlvCommandOptionGetPathCallback::onAttachment, this, _1, _3));
		gIdleCallbacks.addFunction(&onIdle, this);
	}
	~RlvCommandOptionGetPathCallback()
	{
		if (mAttachmentConnection.connected())
			mAttachmentConnection.disconnect();
		gIdleCallbacks.deleteFunction(&onIdle, this);
	}
	void onAttachment(LLViewerObject* pAttachObj, LLVOAvatarSelf::EAttachAction eAction) const
	{
		if ( (LLVOAvatarSelf::ACTION_ATTACH == eAction) && (pAttachObj->getID() == mObjectId) )
		{
			uuid_vec_t idItems(1, pAttachObj->getAttachmentItemID());
			mCallback(idItems);
			delete this;
		}
	}
	static void onIdle(void* pData)
	{
		RlvCommandOptionGetPathCallback* pInstance = reinterpret_cast<RlvCommandOptionGetPathCallback*>(pData);
		if (pInstance->mExpirationTimer.getElapsedTimeF32() > 30.0f)
			delete pInstance;
	}
protected:
	LLUUID                      mObjectId;
	RlvCommandOptionGetPath::getpath_callback_t mCallback;
	boost::signals2::connection mAttachmentConnection;
	LLFrameTimer                mExpirationTimer;
};
RlvCommandOptionGetPath::RlvCommandOptionGetPath(const RlvCommand& rlvCmd, getpath_callback_t cb)
	: m_fCallback(false)
{
	m_fValid = true;
	RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
	if (rlvCmdOption.isWearableType())
	{
		getItemIDs(rlvCmdOption.getWearableType(), m_idItems);
	}
	else if (rlvCmdOption.isAttachmentPoint())
	{
		getItemIDs(rlvCmdOption.getAttachmentPoint(), m_idItems);
	}
	else if (rlvCmdOption.isUUID())
	{
		const LLViewerObject* pAttachObj = gObjectList.findObject(rlvCmdOption.getUUID());
		if ( (pAttachObj) && (pAttachObj->isAttachment()) && (pAttachObj->permYouOwner()) )
			m_idItems.push_back(pAttachObj->getAttachmentItemID());
		else
			m_fValid = false;
	}
	else if (rlvCmdOption.isEmpty())
	{
		const LLViewerObject* pObj = gObjectList.findObject(rlvCmd.getObjectID());
		if (pObj)
		{
			if (pObj->isAttachment())
				m_idItems.push_back(pObj->getAttachmentItemID());
		}
		else if (!cb.empty())
		{
			new RlvCommandOptionGetPathCallback(rlvCmd.getObjectID(), cb);
			m_fCallback = true;
			return;
		}
	}
	else
	{
		m_fValid = false;
		return;
	}
	if (!cb.empty())
	{
		cb(getItemIDs());
	}
}
bool RlvCommandOptionGetPath::getItemIDs(const LLViewerJointAttachment* pAttachPt, uuid_vec_t& idItems)
{
	uuid_vec_t::size_type cntItemsPrev = idItems.size();
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	RlvFindAttachmentsOnPoint f(pAttachPt);
	gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(), folders, items, false, f);
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		const LLViewerInventoryItem* pItem = *itItem;
		if (pItem)
			idItems.push_back(pItem->getLinkedUUID());
	}
	return (cntItemsPrev != idItems.size());
}
bool RlvCommandOptionGetPath::getItemIDs(LLWearableType::EType wtType, uuid_vec_t& idItems)
{
	uuid_vec_t::size_type cntItemsPrev = idItems.size();
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	LLFindWearablesOfType f(wtType);
	gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(), folders, items, false, f);
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		const LLViewerInventoryItem* pItem = *itItem;
		if (pItem)
			idItems.push_back(pItem->getLinkedUUID());
	}
	return (cntItemsPrev != idItems.size());
}
RlvCommandOptionAdjustHeight::RlvCommandOptionAdjustHeight(const RlvCommand& rlvCmd)
	: m_nPelvisToFoot(0.0f), m_nPelvisToFootDeltaMult(0.0f), m_nPelvisToFootOffset(0.0f)
{
	std::vector<std::string> cmdTokens;
	boost::split(cmdTokens, rlvCmd.getOption(), boost::is_any_of(std::string(";")));
	if (1 == cmdTokens.size())
	{
		m_fValid = (LLStringUtil::convertToF32(cmdTokens[0], m_nPelvisToFootOffset));
		m_nPelvisToFootOffset /= 100;
	}
	else if ( (2 <= cmdTokens.size()) && (cmdTokens.size() <= 3) )
	{
		m_fValid = (LLStringUtil::convertToF32(cmdTokens[0], m_nPelvisToFoot)) &&
			 (LLStringUtil::convertToF32(cmdTokens[1], m_nPelvisToFootDeltaMult)) &&
			 ( (2 == cmdTokens.size()) || (LLStringUtil::convertToF32(cmdTokens[2], m_nPelvisToFootOffset)) );
	}
}
RlvCommandOptionTpTo::RlvCommandOptionTpTo(const RlvCommand &rlvCmd)
{
	std::vector<std::string> cmdTokens;
	boost::split(cmdTokens, rlvCmd.getOption(), boost::is_any_of(std::string("/")));
	m_fValid = (3 == cmdTokens.size());
	for (int idxAxis = 0; (idxAxis < 3) && (m_fValid); idxAxis++)
		m_fValid &= static_cast<bool>(LLStringUtil::convertToF64(cmdTokens[idxAxis], m_posGlobal[idxAxis]));
}
RlvObject::RlvObject(const LLUUID& idObj) : m_idObj(idObj), m_nLookupMisses(0)
{
	LLViewerObject* pObj = gObjectList.findObject(idObj);
	m_fLookup = (NULL != pObj);
	m_idxAttachPt = (pObj) ? ATTACHMENT_ID_FROM_STATE(pObj->getAttachmentState()) : 0;
	m_idRoot = (pObj) ? pObj->getRootEdit()->getID() : LLUUID::null;
}
bool RlvObject::addCommand(const RlvCommand& rlvCmd)
{
	RLV_ASSERT(RLV_TYPE_ADD == rlvCmd.getParamType());
	for (rlv_command_list_t::iterator itCmd = m_Commands.begin(); itCmd != m_Commands.end(); ++itCmd)
	{
		if ( (itCmd->getBehaviour() == rlvCmd.getBehaviour()) && (itCmd->getOption() == rlvCmd.getOption()) &&
			 (itCmd->isStrict() == rlvCmd.isStrict() ) )
		{
			return false;
		}
	}
	m_Commands.push_back(rlvCmd);
	return true;
}
bool RlvObject::removeCommand(const RlvCommand& rlvCmd)
{
	RLV_ASSERT(RLV_TYPE_REMOVE == rlvCmd.getParamType());
	for (rlv_command_list_t::iterator itCmd = m_Commands.begin(); itCmd != m_Commands.end(); ++itCmd)
	{
		if ( (itCmd->getBehaviour() == rlvCmd.getBehaviour()) && (itCmd->getOption() == rlvCmd.getOption()) &&
			 (itCmd->isStrict() == rlvCmd.isStrict() ) )
		{
			m_Commands.erase(itCmd);
			return true;
		}
	}
	return false;
}
void RlvObject::setCommandRet(const RlvCommand& rlvCmd, ERlvCmdRet eRet)
{
	for (rlv_command_list_t::iterator itCmd = m_Commands.begin(); itCmd != m_Commands.end(); ++itCmd)
	{
		if (*itCmd == rlvCmd)
		{
			itCmd->m_eRet = eRet;
			break;
		}
	}
}
bool RlvObject::hasBehaviour(ERlvBehaviour eBehaviour, bool fStrictOnly) const
{
	for (rlv_command_list_t::const_iterator itCmd = m_Commands.begin(); itCmd != m_Commands.end(); ++itCmd)
		if ( (itCmd->getBehaviourType() == eBehaviour) && (itCmd->getOption().empty()) && ((!fStrictOnly) || (itCmd->isStrict())) )
			return true;
	return false;
}
bool RlvObject::hasBehaviour(ERlvBehaviour eBehaviour, const std::string& strOption, bool fStrictOnly) const
{
	for (const RlvCommand& rlvCmd : m_Commands)
	{
		if ( (rlvCmd.getBehaviourType() == eBehaviour) &&
			 ( (rlvCmd.getOption() == strOption) ) &&
			 ( (!fStrictOnly) ||(rlvCmd.isStrict()) ) )
		{
			return true;
		}
	}
	return false;
}
std::string RlvObject::getStatusString(const std::string& strFilter, const std::string& strSeparator) const
{
	std::string strStatus, strCmd;
	for (rlv_command_list_t::const_iterator itCmd = m_Commands.begin(); itCmd != m_Commands.end(); ++itCmd)
	{
		strCmd = itCmd->asString();
		if ( (strFilter.empty()) || (std::string::npos != strCmd.find(strFilter)) )
			strStatus.append(strSeparator).append(strCmd);
	}
	return strStatus;
}
bool RlvForceWear::isWearingItem(const LLInventoryItem* pItem)
{
	if (pItem)
	{
		switch (pItem->getActualType())
		{
			case LLAssetType::AT_BODYPART:
			case LLAssetType::AT_CLOTHING:
				return gAgentWearables.isWearingItem(pItem->getUUID());
			case LLAssetType::AT_OBJECT:
				return (isAgentAvatarValid()) && (gAgentAvatarp->isWearingAttachment(pItem->getUUID()));
			case LLAssetType::AT_GESTURE:
				return LLGestureMgr::instance().isGestureActive(pItem->getUUID());
			case LLAssetType::AT_LINK:
				return isWearingItem(gInventory.getItem(pItem->getLinkedUUID()));
			default:
				break;
		}
	}
	return false;
}
void RlvForceWear::forceFolder(const LLViewerInventoryCategory* pFolder, EWearAction eAction, EWearFlags eFlags)
{
	if (!gAgentWearables.areWearablesLoaded())
	{
		LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
		return;
	}
	if (!isAgentAvatarValid())
		return;
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	RlvWearableItemCollector f(pFolder, eAction, eFlags);
	gInventory.collectDescendentsIf(pFolder->getUUID(), folders, items, FALSE, f, true);
	bool fSeenWType[LLWearableType::WT_COUNT] = { false };
	EWearAction eCurAction = eAction;
	for (S32 idxItem = 0, cntItem = items.size(); idxItem < cntItem; idxItem++)
	{
		LLViewerInventoryItem* pRlvItem = items.at(idxItem);
		LLViewerInventoryItem* pItem = (LLAssetType::AT_LINK == pRlvItem->getActualType()) ? pRlvItem->getLinkedItem() : pRlvItem;
		if (isWearAction(eAction))
			eCurAction = f.getWearAction(pRlvItem->getParentUUID());
		else
			eCurAction = eAction;
		switch (pItem->getType())
		{
			case LLAssetType::AT_BODYPART:
				RLV_ASSERT(isWearAction(eAction));
			case LLAssetType::AT_CLOTHING:
				if (isWearAction(eAction))
				{
					eCurAction = (!fSeenWType[pItem->getWearableType()]) ? eCurAction : ACTION_WEAR_ADD;
					ERlvWearMask eWearMask = gRlvWearableLocks.canWear(pRlvItem);
					if ( ((ACTION_WEAR_REPLACE == eCurAction) && (eWearMask & RLV_WEAR_REPLACE)) ||
						 ((ACTION_WEAR_ADD == eCurAction) && (eWearMask & RLV_WEAR_ADD)) )
					{
						if (!isAddWearable(pItem))
							addWearable(pRlvItem, eCurAction);
						fSeenWType[pItem->getWearableType()] = true;
					}
				}
				else
				{
					const LLViewerWearable* pWearable = gAgentWearables.getWearableFromItemID(pItem->getUUID());
					if ( (pWearable) && (isForceRemovable(pWearable, false)) )
						remWearable(pWearable);
				}
				break;
			case LLAssetType::AT_OBJECT:
				if (isWearAction(eAction))
				{
					ERlvWearMask eWearMask = gRlvAttachmentLocks.canAttach(pRlvItem);
					if ( ((ACTION_WEAR_REPLACE == eCurAction) && (eWearMask & RLV_WEAR_REPLACE)) ||
						 ((ACTION_WEAR_ADD == eCurAction) && (eWearMask & RLV_WEAR_ADD)) )
					{
						if (!isAddAttachment(pRlvItem))
						{
							#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
							LLViewerInventoryCategory* pCompositeFolder = NULL;
							if ( (pAttachPt->getObject()) && (RlvSettings::getEnableComposites()) &&
								 (pAttachPt->getItemID() != pItem->getUUID()) &&
								 (gRlvHandler.getCompositeInfo(pAttachPt->getItemID(), NULL, &pCompositeFolder)) )
							{
								if (gRlvHandler.canTakeOffComposite(pCompositeFolder))
								{
									forceFolder(pCompositeFolder, ACTION_DETACH, FLAG_DEFAULT);
									addAttachment(pRlvItem);
								}
							}
							else
							#endif
							{
								addAttachment(pRlvItem, eCurAction);
							}
						}
					}
				}
				else
				{
					const LLViewerObject* pAttachObj = gAgentAvatarp->getWornAttachment(pItem->getUUID());
					if ( (pAttachObj) && (isForceDetachable(pAttachObj, false)) )
						remAttachment(pAttachObj);
				}
				break;
			#ifdef RLV_EXTENSION_FORCEWEAR_GESTURES
			case LLAssetType::AT_GESTURE:
				if (isWearAction(eAction))
				{
					if (std::find_if(m_addGestures.begin(), m_addGestures.end(), RlvPredIsEqualOrLinkedItem(pRlvItem)) == m_addGestures.end())
						m_addGestures.push_back(pRlvItem);
				}
				else
				{
					if (std::find_if(m_remGestures.begin(), m_remGestures.end(), RlvPredIsEqualOrLinkedItem(pRlvItem)) == m_remGestures.end())
						m_remGestures.push_back(pRlvItem);
				}
				break;
			#endif
			default:
				break;
		}
	}
}
bool RlvForceWear::isForceDetachable(const LLViewerObject* pAttachObj, bool fCheckComposite , const LLUUID& idExcept )
{
	#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	LLViewerInventoryCategory* pFolder = NULL;
	#endif
	return
	  (
	    (pAttachObj) && (pAttachObj->isAttachment())
		&& ( (idExcept.isNull()) ? (!gRlvAttachmentLocks.isLockedAttachment(pAttachObj))
								 : (!gRlvAttachmentLocks.isLockedAttachmentExcept(pAttachObj, idExcept)) )
		&& (isStrippable(pAttachObj->getAttachmentItemID()))
		#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
		&& ( (!fCheckComposite) || (!RlvSettings::getEnableComposites()) ||
		     (!gRlvHandler.getCompositeInfo(pAttachPt->getItemID(), NULL, &pFolder)) || (gRlvHandler.canTakeOffComposite(pFolder)) )
		#endif
	  );
}
bool RlvForceWear::isForceDetachable(const LLViewerJointAttachment* pAttachPt, bool fCheckComposite , const LLUUID& idExcept )
{
	for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
			itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
	{
		if (isForceDetachable(*itAttachObj, fCheckComposite, idExcept))
			return true;
	}
	return false;
}
void RlvForceWear::forceDetach(const LLViewerObject* pAttachObj)
{
	if ( (!pAttachObj) || (isRemAttachment(pAttachObj)) )
		return;
	if (isForceDetachable(pAttachObj))
	{
		#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
		LLViewerInventoryCategory* pFolder = NULL;
		if ( (RlvSettings::getEnableComposites()) &&
			 (gRlvHandler.getCompositeInfo(pAttachPt->getItemID(), NULL, &pFolder)) )
		{
			if (gRlvHandler.canTakeOffComposite(pFolder))
				forceFolder(pFolder, ACTION_DETACH, FLAG_DEFAULT);
		}
		else
		#endif
		{
			remAttachment(pAttachObj);
		}
	}
}
void RlvForceWear::forceDetach(const LLViewerJointAttachment* pAttachPt)
{
	for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator itAttachObj = pAttachPt->mAttachedObjects.begin();
			itAttachObj != pAttachPt->mAttachedObjects.end(); ++itAttachObj)
	{
		forceDetach(*itAttachObj);
	}
}
bool RlvForceWear::isForceRemovable(const LLViewerWearable* pWearable, bool fCheckComposite , const LLUUID& idExcept )
{
	#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	LLViewerInventoryCategory* pFolder = NULL;
	#endif
	return
	  (
		(pWearable) && (LLAssetType::AT_CLOTHING == pWearable->getAssetType())
		&& ( (idExcept.isNull()) ? !gRlvWearableLocks.isLockedWearable(pWearable)
		                         : !gRlvWearableLocks.isLockedWearableExcept(pWearable, idExcept) )
		&& (isStrippable(pWearable->getItemID()))
		#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
		&& ( (!fCheckComposite) || (!RlvSettings::getEnableComposites()) ||
		     (!gRlvHandler.getCompositeInfo(pWearable->getItemID(), NULL, &pFolder)) || (gRlvHandler.canTakeOffComposite(pFolder)) )
		#endif
	  );
}
bool RlvForceWear::isForceRemovable(LLWearableType::EType wtType, bool fCheckComposite , const LLUUID& idExcept )
{
	for (U32 idxWearable = 0, cntWearable = gAgentWearables.getWearableCount(wtType); idxWearable < cntWearable; idxWearable++)
		if (isForceRemovable(gAgentWearables.getViewerWearable(wtType, idxWearable), fCheckComposite, idExcept))
			return true;
	return false;
}
void RlvForceWear::forceRemove(const LLViewerWearable* pWearable)
{
	if ( (!pWearable) || (isRemWearable(pWearable)) )
		return;
	if (isForceRemovable(pWearable))
	{
		#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
		LLViewerInventoryCategory* pFolder = NULL;
		if ( (RlvSettings::getEnableComposites()) &&
			 (gRlvHandler.getCompositeInfo(gAgent.getWearableItem(wtType), NULL, &pFolder)) )
		{
			if (gRlvHandler.canTakeOffComposite(pFolder))
				forceFolder(pFolder, ACTION_DETACH, FLAG_DEFAULT);
		}
		else
		#endif
		{
			remWearable(pWearable);
		}
	}
}
void RlvForceWear::forceRemove(LLWearableType::EType wtType)
{
	for (U32 idxWearable = 0, cntWearable = gAgentWearables.getWearableCount(wtType); idxWearable < cntWearable; idxWearable++)
		forceRemove(gAgentWearables.getViewerWearable(wtType, idxWearable));
}
bool RlvForceWear::isStrippable(const LLInventoryItem* pItem)
{
	if (pItem)
	{
		if (LLAssetType::AT_LINK == pItem->getActualType())
		{
			if (!isStrippable(pItem->getLinkedUUID()))
				return false;
		}
		else
		{
			if (std::string::npos != pItem->getName().find(RLV_FOLDER_FLAG_NOSTRIP))
				return false;
		}
		LLViewerInventoryCategory* pFolder = gInventory.getCategory(pItem->getParentUUID());
		while (pFolder)
		{
			if (std::string::npos != pFolder->getName().find(RLV_FOLDER_FLAG_NOSTRIP))
				return false;
			if ( (gInventory.getRootFolderID() != pFolder->getParentUUID()) && (RlvInventory::isFoldedFolder(pFolder, true)) )
				pFolder = gInventory.getCategory(pFolder->getParentUUID());
			else
				pFolder = NULL;
		}
	}
	return true;
}
void RlvForceWear::addAttachment(const LLViewerInventoryItem* pItem, EWearAction eAction)
{
	const LLViewerObject* pAttachObj = (isAgentAvatarValid()) ? gAgentAvatarp->getWornAttachment(pItem->getLinkedUUID()) : NULL;
	if ( (pAttachObj) && (isRemAttachment(pAttachObj)) )
		m_remAttachments.erase(std::remove(m_remAttachments.begin(), m_remAttachments.end(), pAttachObj), m_remAttachments.end());
	S32 idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(pItem, true);
	if (ACTION_WEAR_ADD == eAction)
	{
		idxAttachPt |= ATTACHMENT_ADD;
		if (!isAddAttachment(pItem))
		{
			addattachments_map_t::iterator itAddAttachments = m_addAttachments.find(idxAttachPt);
			if (itAddAttachments == m_addAttachments.end())
			{
				m_addAttachments.insert(addattachment_pair_t(idxAttachPt, LLInventoryModel::item_array_t()));
				itAddAttachments = m_addAttachments.find(idxAttachPt);
			}
			itAddAttachments->second.push_back(const_cast<LLViewerInventoryItem*>(pItem));
		}
	}
	else if (ACTION_WEAR_REPLACE == eAction)
	{
		addattachments_map_t::iterator itAddAttachments = m_addAttachments.find(idxAttachPt | ATTACHMENT_ADD);
		if ( (0 != idxAttachPt) && (itAddAttachments != m_addAttachments.end()) )
			itAddAttachments->second.clear();
		itAddAttachments = m_addAttachments.find(idxAttachPt);
		if (itAddAttachments == m_addAttachments.end())
		{
			m_addAttachments.insert(addattachment_pair_t(idxAttachPt, LLInventoryModel::item_array_t()));
			itAddAttachments = m_addAttachments.find(idxAttachPt);
		}
		if (0 != idxAttachPt)
			itAddAttachments->second.clear();
		itAddAttachments->second.push_back(const_cast<LLViewerInventoryItem*>(pItem));
	}
}
void RlvForceWear::remAttachment(const LLViewerObject* pAttachObj)
{
	const LLViewerInventoryItem* pItem = (pAttachObj->isAttachment()) ? gInventory.getItem(pAttachObj->getAttachmentItemID()) : NULL;
	if (pItem)
	{
		addattachments_map_t::iterator itAddAttachments = m_addAttachments.begin();
		while (itAddAttachments != m_addAttachments.end())
		{
			LLInventoryModel::item_array_t& wearItems = itAddAttachments->second;
			if (std::find_if(wearItems.begin(), wearItems.end(), RlvPredIsEqualOrLinkedItem(pItem)) != wearItems.end())
				wearItems.erase(std::remove_if(wearItems.begin(), wearItems.end(), RlvPredIsEqualOrLinkedItem(pItem)), wearItems.end());
			if (wearItems.empty())
				m_addAttachments.erase(itAddAttachments++);
			else
				++itAddAttachments;
		}
	}
	if (!isRemAttachment(pAttachObj))
	{
		m_remAttachments.push_back(const_cast<LLViewerObject*>(pAttachObj));
	}
}
void RlvForceWear::addWearable(const LLViewerInventoryItem* pItem, EWearAction eAction)
{
	const LLViewerWearable* pWearable = gAgentWearables.getWearableFromItemID(pItem->getLinkedUUID());
	if ( (ACTION_WEAR_REPLACE == eAction) && (!pWearable) )
		forceRemove(pItem->getWearableType());
	if ( (pWearable) && (isRemWearable(pWearable)) )
		m_remWearables.erase(std::remove(m_remWearables.begin(), m_remWearables.end(), pWearable), m_remWearables.end());
	addwearables_map_t::iterator itAddWearables = m_addWearables.find(pItem->getWearableType());
	if (itAddWearables == m_addWearables.end())
	{
		m_addWearables.insert(addwearable_pair_t(pItem->getWearableType(), LLInventoryModel::item_array_t()));
		itAddWearables = m_addWearables.find(pItem->getWearableType());
	}
	if (ACTION_WEAR_ADD == eAction)
	{
		if (!isAddWearable(pItem))
			itAddWearables->second.push_back(const_cast<LLViewerInventoryItem*>(pItem));
	}
	else if (ACTION_WEAR_REPLACE == eAction)
	{
		itAddWearables->second.clear();
		itAddWearables->second.push_back(const_cast<LLViewerInventoryItem*>(pItem));
	}
}
void RlvForceWear::remWearable(const LLViewerWearable* pWearable)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(pWearable->getItemID());
	if ( (pItem) && (isAddWearable(pItem)) )
	{
		addwearables_map_t::iterator itAddWearables = m_addWearables.find(pItem->getWearableType());
		LLInventoryModel::item_array_t& wearItems = itAddWearables->second;
		wearItems.erase(std::remove_if(wearItems.begin(), wearItems.end(), RlvPredIsEqualOrLinkedItem(pItem)), wearItems.end());
		if (wearItems.empty())
			m_addWearables.erase(itAddWearables);
	}
	if (!isRemWearable(pWearable))
		m_remWearables.push_back(pWearable);
}
void RlvForceWear::updatePendingAttachments()
{
	if (RlvForceWear::instanceExists())
	{
		RlvForceWear* pThis = RlvForceWear::getInstance();
		for (const auto& itAttach : pThis->m_pendingAttachments)
			LLAttachmentsMgr::instance().addAttachmentRequest(itAttach.first, itAttach.second & ~ATTACHMENT_ADD, itAttach.second & ATTACHMENT_ADD);
		pThis->m_pendingAttachments.clear();
	}
}
void RlvForceWear::addPendingAttachment(const LLUUID& idItem, U8 idxPoint)
{
	auto itAttach = m_pendingAttachments.find(idItem);
	if (m_pendingAttachments.end() == itAttach)
		m_pendingAttachments.insert(std::make_pair(idItem, idxPoint));
	else
		itAttach->second = idxPoint;
}
void RlvForceWear::remPendingAttachment(const LLUUID& idItem)
{
	m_pendingAttachments.erase(idItem);
}
void RlvForceWear::done()
{
	if ( (m_remWearables.empty()) && (m_remAttachments.empty()) && (m_remGestures.empty()) &&
		 (m_addWearables.empty()) && (m_addAttachments.empty()) && (m_addGestures.empty()) )
	{
		return;
	}
	uuid_vec_t remItems;
	if (m_remWearables.size())
	{
		for (const LLViewerWearable* pWearable : m_remWearables)
			remItems.push_back(pWearable->getItemID());
		m_remWearables.clear();
	}
	if (m_remGestures.size())
	{
		for (const LLViewerInventoryItem* pItem : m_remGestures)
			LLGestureMgr::instance().deactivateGesture(pItem->getUUID());
		m_remGestures.clear();
	}
	if (m_remAttachments.size())
	{
		LLAgentWearables::userRemoveMultipleAttachments(m_remAttachments);
		for (const LLViewerObject* pAttachObj : m_remAttachments)
			remItems.push_back(pAttachObj->getAttachmentItemID());
		m_remAttachments.clear();
	}
	LLInventoryModel::item_array_t addBodyParts, addClothing;
	for (addwearables_map_t::const_iterator itAddWearables = m_addWearables.cbegin(); itAddWearables != m_addWearables.cend(); ++itAddWearables)
	{
		for (LLViewerInventoryItem* pItem : itAddWearables->second)
		{
			if (LLAssetType::AT_BODYPART == pItem->getType())
				addBodyParts.push_back(pItem);
			else
				addClothing.push_back(pItem);
		}
	}
	m_addWearables.clear();
	for (addattachments_map_t::const_iterator itAddAttachments = m_addAttachments.cbegin(); itAddAttachments != m_addAttachments.cend(); ++itAddAttachments)
	{
		for (const LLViewerInventoryItem* pItem : itAddAttachments->second)
			addPendingAttachment(pItem->getLinkedUUID(), itAddAttachments->first);
	}
	m_addAttachments.clear();
	LLPointer<LLInventoryCallback> cb = new LLUpdateAppearanceOnDestroy(false, false, boost::bind(RlvForceWear::updatePendingAttachments));
	if (!remItems.empty())
	{
		LLAppearanceMgr::instance().removeItemsFromAvatar(remItems, cb, true);
	}
	if ( (addBodyParts.empty()) && (!addClothing.empty()) && (m_addGestures.empty()) )
	{
		uuid_vec_t idClothing;
		for (const LLViewerInventoryItem* pItem : addClothing)
			idClothing.push_back(pItem->getUUID());
		LLAppearanceMgr::instance().wearItemsOnAvatar(idClothing, false, false, cb);
	}
	else if ( (!addBodyParts.empty()) || (!addClothing.empty()) || (!m_addGestures.empty()) )
	{
		LLInventoryModel::item_array_t addAttachments;
		LLAppearanceMgr::instance().updateCOF(addBodyParts, addClothing, addAttachments, m_addGestures, true, LLUUID::null, cb);
		m_addGestures.clear();
	}
	RLV_ASSERT( (m_remWearables.empty()) && (m_remAttachments.empty()) && (m_remGestures.empty()) );
	RLV_ASSERT( (m_addWearables.empty()) && (m_addAttachments.empty()) && (m_addGestures.empty()) );
}
RlvBehaviourNotifyHandler::RlvBehaviourNotifyHandler()
{
	m_ConnCommand = gRlvHandler.setCommandCallback(boost::bind(&RlvBehaviourNotifyHandler::onCommand, this, _1, _2, _3));
}
void RlvBehaviourNotifyHandler::onCommand(const RlvCommand& rlvCmd, ERlvCmdRet eRet, bool fInternal) const
{
	if (fInternal)
		return;
	switch (rlvCmd.getParamType())
	{
		case RLV_TYPE_ADD:
		case RLV_TYPE_REMOVE:
			sendNotification(rlvCmd.asString(), "=" + rlvCmd.getParam());
			break;
		case RLV_TYPE_CLEAR:
			sendNotification(rlvCmd.asString());
			break;
		default:
			break;
	}
}
void RlvBehaviourNotifyHandler::sendNotification(const std::string& strText, const std::string& strSuffix)
{
	if (instanceExists())
	{
		RlvBehaviourNotifyHandler* pThis = getInstance();
		for (std::multimap<LLUUID, notifyData>::const_iterator itNotify = pThis->m_Notifications.begin();
				itNotify != pThis->m_Notifications.end(); ++itNotify)
		{
			if ( (itNotify->second.strFilter.empty()) || (boost::icontains(strText, itNotify->second.strFilter)) )
				RlvUtil::sendChatReply(itNotify->second.nChannel, "/" + strText + strSuffix);
		}
	}
}
void RlvBehaviourNotifyHandler::onWear(LLWearableType::EType eType, bool fAllowed)
{
	sendNotification(llformat("worn %s %s", (fAllowed) ? "legally" : "illegally", LLWearableType::getTypeName(eType).c_str()));
}
void RlvBehaviourNotifyHandler::onTakeOff(LLWearableType::EType eType, bool fAllowed)
{
	sendNotification(llformat("unworn %s %s", (fAllowed) ? "legally" : "illegally", LLWearableType::getTypeName(eType).c_str()));
}
void RlvBehaviourNotifyHandler::onAttach(const LLViewerJointAttachment* pAttachPt, bool fAllowed)
{
	sendNotification(llformat("attached %s %s", (fAllowed) ? "legally" : "illegally", pAttachPt->getName().c_str()));
}
void RlvBehaviourNotifyHandler::onDetach(const LLViewerJointAttachment* pAttachPt, bool fAllowed)
{
	sendNotification(llformat("detached %s %s", (fAllowed) ? "legally" : "illegally", pAttachPt->getName().c_str()));
}
void RlvBehaviourNotifyHandler::onReattach(const LLViewerJointAttachment* pAttachPt, bool fAllowed)
{
	sendNotification(llformat("reattached %s %s", (fAllowed) ? "legally" : "illegally", pAttachPt->getName().c_str()));
}
BOOL RlvGCTimer::tick()
{
	bool fContinue = gRlvHandler.onGC();
	if (!fContinue)
		gRlvHandler.m_pGCTimer = NULL;
	return !fContinue;
}
const std::string cstrAttachGroups[RLV_ATTACHGROUP_COUNT] = { "head", "torso", "arms", "legs", "hud" };
ERlvAttachGroupType rlvAttachGroupFromIndex(S32 idxGroup)
{
	switch (idxGroup)
	{
		case 0:
		case 1:
		case 3:
		case 4:
			return RLV_ATTACHGROUP_ARMS;
		case 2:
			return RLV_ATTACHGROUP_HEAD;
		case 5:
		case 7:
			return RLV_ATTACHGROUP_LEGS;
		case 6:
			return RLV_ATTACHGROUP_TORSO;
		case 8:
			return RLV_ATTACHGROUP_HUD;
		default:
			return RLV_ATTACHGROUP_INVALID;
	}
}
ERlvAttachGroupType rlvAttachGroupFromString(const std::string& strGroup)
{
	for (int idx = 0; idx < RLV_ATTACHGROUP_COUNT; idx++)
		if (cstrAttachGroups[idx] == strGroup)
			return static_cast<ERlvAttachGroupType>(idx);
	return RLV_ATTACHGROUP_INVALID;
}
std::string rlvGetFirstParenthesisedText(const std::string& strText, std::string::size_type* pidxMatch )
{
	if (pidxMatch)
		*pidxMatch = std::string::npos;
	std::string::size_type idxIt, idxStart; int cntLevel = 1;
	if ((idxStart = strText.find_first_of('(')) == std::string::npos)
		return std::string();
	const char* pstrText = strText.c_str(); idxIt = idxStart;
	while ( (cntLevel > 0) && (++idxIt < strText.length()) )
	{
		if ('(' == pstrText[idxIt])
			cntLevel++;
		else if (')' == pstrText[idxIt])
			cntLevel--;
	}
	if (idxIt < strText.length())
	{
		if (pidxMatch)
			*pidxMatch = idxStart;
		return strText.substr(idxStart + 1, idxIt - idxStart - 1);
	}
	return std::string();
}
std::string rlvGetLastParenthesisedText(const std::string& strText, std::string::size_type* pidxStart )
{
	if (pidxStart)
		*pidxStart = std::string::npos;
	std::string::size_type idxIt, idxEnd; int cntLevel = 1;
	if ((idxEnd = strText.find_last_of(')')) == std::string::npos)
		return std::string();
	const char* pstrText = strText.c_str(); idxIt = idxEnd;
	while ( (cntLevel > 0) && (--idxIt >= 0) && (idxIt < strText.length()) )
	{
		if (')' == pstrText[idxIt])
			cntLevel++;
		else if ('(' == pstrText[idxIt])
			cntLevel--;
	}
	if ( (idxIt >= 0) && (idxIt < strText.length()) )
	{
		if (pidxStart)
			*pidxStart = idxIt;
		return strText.substr(idxIt + 1, idxEnd - idxIt - 1);
	}
	return std::string();
}
