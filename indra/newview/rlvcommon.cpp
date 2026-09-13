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
#include "llagentui.h"
#include "llavatarnamecache.h"
#include "llinstantmessage.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "lluictrlfactory.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewermenu.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "rlvactions.h"
#include "rlvcommon.h"
#include "rlvhelper.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
#include "../lscript/lscript_byteformat.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/regex.hpp>
#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
bool RlvSettings::fCompositeFolders = false;
#endif
bool RlvSettings::fCanOOC = true;
bool RlvSettings::fLegacyNaming = true;
bool RlvSettings::fNoSetEnv = false;
bool RlvSettings::fShowNameTags = false;
void RlvSettings::initClass()
{
	static bool fInitialized = false;
	if (!fInitialized)
	{
		#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
		fCompositeFolders = rlvGetSetting<bool>(RLV_SETTING_ENABLECOMPOSITES, false);
		if (gSavedSettings.controlExists(RLV_SETTING_ENABLECOMPOSITES))
			gSavedSettings.getControl(RLV_SETTING_ENABLECOMPOSITES)->getSignal()->connect(boost::bind(&onChangedSettingBOOL, _2, &fCompositeFolders));
		#endif
		fLegacyNaming = rlvGetSetting<bool>(RLV_SETTING_ENABLELEGACYNAMING, true);
		if (gSavedSettings.controlExists(RLV_SETTING_ENABLELEGACYNAMING))
			gSavedSettings.getControl(RLV_SETTING_ENABLELEGACYNAMING)->getSignal()->connect(boost::bind(&onChangedSettingBOOL, _2, &fLegacyNaming));
		fCanOOC = rlvGetSetting<bool>(RLV_SETTING_CANOOC, true);
		fNoSetEnv = rlvGetSetting<bool>(RLV_SETTING_NOSETENV, false);
		fShowNameTags = rlvGetSetting<bool>(RLV_SETTING_SHOWNAMETAGS, false);
		if (gSavedSettings.controlExists(RLV_SETTING_SHOWNAMETAGS))
			gSavedSettings.getControl(RLV_SETTING_SHOWNAMETAGS)->getSignal()->connect(boost::bind(&onChangedSettingBOOL, _2, &fShowNameTags));
#ifdef RLV_EXTENSION_STARTLOCATION
		if (gSavedPerAccountSettings.controlExists(RLV_SETTING_LOGINLASTLOCATION))
			gSavedPerAccountSettings.getControl(RLV_SETTING_LOGINLASTLOCATION)->setHiddenFromSettingsEditor(true);
#endif
		if (gSavedSettings.controlExists(RLV_SETTING_TOPLEVELMENU))
			gSavedSettings.getControl(RLV_SETTING_TOPLEVELMENU)->getSignal()->connect(boost::bind(&onChangedMenuLevel));
		fInitialized = true;
	}
}
#ifdef RLV_EXTENSION_STARTLOCATION
	void RlvSettings::updateLoginLastLocation()
	{
		if ( (!LLApp::isQuitting()) && (gSavedPerAccountSettings.controlExists(RLV_SETTING_LOGINLASTLOCATION)) )
		{
			BOOL fValue = (gRlvHandler.hasBehaviour(RLV_BHVR_TPLOC)) || (!RlvActions::canStand());
			if (gSavedPerAccountSettings.getBOOL(RLV_SETTING_LOGINLASTLOCATION) != fValue)
			{
				gSavedPerAccountSettings.setBOOL(RLV_SETTING_LOGINLASTLOCATION, fValue);
				gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
			}
		}
	}
#endif
bool RlvSettings::onChangedAvatarOffset(const LLSD& sdValue)
{
	if ( (isAgentAvatarValid()) && (!gAgentAvatarp->isUsingServerBakes()) )
	{
		gAgentAvatarp->computeBodySize();
	}
	return true;
}
bool RlvSettings::onChangedMenuLevel()
{
	rlvMenuToggleVisible();
	return true;
}
bool RlvSettings::onChangedSettingBOOL(const LLSD& sdValue, bool* pfSetting)
{
	if (pfSetting)
		*pfSetting = sdValue.asBoolean();
	return true;
}
std::vector<std::string> RlvStrings::m_Anonyms;
RlvStrings::string_map_t RlvStrings::m_StringMap;
std::string RlvStrings::m_StringMapPath;
void RlvStrings::initClass()
{
	static bool fInitialized = false;
	if (!fInitialized)
	{
		std::string itFile = gDirUtilp->findSkinnedFilename(LLUICtrlFactory::getXUIPaths().front(), RLV_STRINGS_FILE);
		if (!itFile.empty())
		{
			loadFromFile(itFile, false);
		}
		m_StringMapPath = itFile;
		loadFromFile(gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, RLV_STRINGS_FILE), true);
		if ( (m_StringMap.empty()) || (m_Anonyms.empty()) )
		{
			RLV_ERRS << "Problem parsing RLVa string XML file" << RLV_ENDL;
			return;
		}
		fInitialized = true;
	}
}
void RlvStrings::loadFromFile(const std::string& strFilePath, bool fUserOverride)
{
	llifstream fileStream(strFilePath, std::ios::binary); LLSD sdFileData;
	if ( (!fileStream.is_open()) || (!LLSDSerialize::fromXMLDocument(sdFileData, fileStream)) )
		return;
	fileStream.close();
	if (sdFileData.has("strings"))
	{
		const LLSD& sdStrings = sdFileData["strings"];
		for (LLSD::map_const_iterator itString = sdStrings.beginMap(); itString != sdStrings.endMap(); ++itString)
		{
			if ( (!itString->second.has("value")) || ((fUserOverride) && (!hasString(itString->first))) )
				continue;
			std::list<std::string>& listValues = m_StringMap[itString->first];
			if (!fUserOverride)
			{
				if (listValues.size() > 0)
					listValues.pop_front();
				listValues.push_front(itString->second["value"].asString());
			}
			else
			{
				while (listValues.size() > 1)
					listValues.pop_back();
				listValues.push_back(itString->second["value"].asString());
			}
		}
	}
	if (sdFileData.has("anonyms"))
	{
		const LLSD& sdAnonyms = sdFileData["anonyms"];
		for (LLSD::array_const_iterator itAnonym = sdAnonyms.beginArray(); itAnonym != sdAnonyms.endArray(); ++itAnonym)
		{
			m_Anonyms.push_back((*itAnonym).asString());
		}
	}
}
void RlvStrings::saveToFile(const std::string& strFilePath)
{
	LLSD sdFileData;
	LLSD& sdStrings = sdFileData["strings"];
	for (string_map_t::const_iterator itString = m_StringMap.begin(); itString != m_StringMap.end(); ++itString)
	{
		const std::list<std::string>& listValues = itString->second;
		if (listValues.size() > 1)
			sdStrings[itString->first]["value"] = listValues.back();
	}
	llofstream fileStream(strFilePath);
	if (!fileStream.good())
		return;
	LLSDSerialize::toPrettyXML(sdFileData, fileStream);
	fileStream.close();
}
const std::string& RlvStrings::getAnonym(const std::string& strName)
{
	const char* pszName = strName.c_str(); U32 nHash = 0;
	for (int idx = 0, cnt = strName.length(); idx < cnt; idx++)
		nHash += pszName[idx];
	return m_Anonyms[nHash % m_Anonyms.size()];
}
const std::string& RlvStrings::getString(const std::string& strStringName)
{
	static const std::string strMissing = "(Missing RLVa string)";
	string_map_t::const_iterator itString = m_StringMap.find(strStringName);
	return (itString != m_StringMap.end()) ? itString->second.back() : strMissing;
}
const char* RlvStrings::getStringFromReturnCode(ERlvCmdRet eRet)
{
	switch (eRet)
	{
		case RLV_RET_SUCCESS_UNSET:
			return "unset";
		case RLV_RET_SUCCESS_DUPLICATE:
			return "duplicate";
		case RLV_RET_SUCCESS_DELAYED:
			return "delayed";
		case RLV_RET_FAILED_SYNTAX:
			return "syntax error";
		case RLV_RET_FAILED_OPTION:
			return "invalid option";
		case RLV_RET_FAILED_PARAM:
			return "invalid param";
		case RLV_RET_FAILED_LOCK:
			return "locked command";
		case RLV_RET_FAILED_DISABLED:
			return "disabled command";
		case RLV_RET_FAILED_UNKNOWN:
			return "unknown command";
		case RLV_RET_FAILED_NOSHAREDROOT:
			return "missing #RLV";
		case RLV_RET_DEPRECATED:
			return "deprecated";
		case RLV_RET_RETAINED:
		case RLV_RET_SUCCESS:
		case RLV_RET_FAILED:
			break;
		case RLV_RET_UNKNOWN:
		default:
			RLV_ASSERT(false);
			break;
	};
	return NULL;
}
std::string RlvStrings::getVersion(bool fLegacy)
{
	return llformat("%s viewer v%d.%d.%d (RLVa %d.%d.%d)",
		( (!fLegacy) ? "RestrainedLove" : "RestrainedLife" ),
		RLV_VERSION_MAJOR, RLV_VERSION_MINOR, RLV_VERSION_PATCH,
		RLVa_VERSION_MAJOR, RLVa_VERSION_MINOR, RLVa_VERSION_PATCH);
}
std::string RlvStrings::getVersionAbout()
{
	return llformat("RLV v%d.%d.%d / RLVa v%d.%d.%d%c" ,
		RLV_VERSION_MAJOR, RLV_VERSION_MINOR, RLV_VERSION_PATCH,
		RLVa_VERSION_MAJOR, RLVa_VERSION_MINOR, RLVa_VERSION_PATCH, 'a' + RLVa_VERSION_BUILD);
}
std::string RlvStrings::getVersionNum()
{
	return llformat("%d%02d%02d%02d", RLV_VERSION_MAJOR, RLV_VERSION_MINOR, RLV_VERSION_PATCH, RLV_VERSION_BUILD);
}
bool RlvStrings::hasString(const std::string& strStringName, bool fCheckCustom)
{
	string_map_t::const_iterator itString = m_StringMap.find(strStringName);
	return (itString != m_StringMap.end()) && ((!fCheckCustom) || (itString->second.size() > 0));
}
void RlvStrings::setCustomString(const std::string& strStringName, const std::string& strStringValue)
{
	if (!hasString(strStringName))
		return;
	std::list<std::string>& listValues = m_StringMap[strStringName];
	while (listValues.size() > 1)
		listValues.pop_back();
	if (!strStringValue.empty())
		listValues.push_back(strStringValue);
}
bool RlvUtil::m_fForceTp = false;
std::string escape_for_regex(const std::string& str)
{
	using namespace boost;
	return regex_replace(str, regex("[.^$|()\\[\\]{}*+?\\\\]"), "\\\\&", match_default|format_sed);
}
void RlvUtil::filterLocation(std::string& strUTF8Text)
{
	LLWorld::region_list_t regions = LLWorld::getInstance()->getRegionList();
	const std::string& strHiddenRegion = RlvStrings::getString(RLV_STRING_HIDDEN_REGION);
	for (LLWorld::region_list_t::const_iterator itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
		boost::replace_all_regex(strUTF8Text, boost::regex("\\b" + escape_for_regex((*itRegion)->getName()) + "\\b", boost::regex::icase), strHiddenRegion);
	LLViewerParcelMgr* pParcelMgr = LLViewerParcelMgr::getInstance();
	if (pParcelMgr)
		boost::replace_all_regex(strUTF8Text, boost::regex("\\b" + escape_for_regex(pParcelMgr->getAgentParcelName()) + "\\b", boost::regex::icase), RlvStrings::getString(RLV_STRING_HIDDEN_PARCEL));
}
void RlvUtil::filterNames(std::string& strUTF8Text, bool fFilterLegacy)
{
	uuid_vec_t idAgents;
	LLWorld::getInstance()->getAvatars(&idAgents, NULL);
	for (int idxAgent = 0, cntAgent = idAgents.size(); idxAgent < cntAgent; idxAgent++)
	{
		LLAvatarName avName;
		if (LLAvatarNameCache::get(idAgents[idxAgent], &avName))
		{
			const std::string& strDisplayName = escape_for_regex(avName.getDisplayName());
			bool fFilterDisplay = (strDisplayName.length() > 2);
			const std::string& strLegacyName = avName.getLegacyName();
			fFilterLegacy &= (strLegacyName.length() > 2);
			const std::string& strAnonym = RlvStrings::getAnonym(avName);
			if (boost::icontains(strLegacyName, strDisplayName))
			{
				if (fFilterLegacy)
					boost::replace_all_regex(strUTF8Text, boost::regex("\\b" + strLegacyName + "\\b", boost::regex::icase), strAnonym);
				if (fFilterDisplay)
					boost::replace_all_regex(strUTF8Text, boost::regex("\\b" + strDisplayName + "\\b", boost::regex::icase), strAnonym);
			}
			else
			{
				if (fFilterDisplay)
					boost::replace_all_regex(strUTF8Text, boost::regex("\\b" + strDisplayName + "\\b", boost::regex::icase), strAnonym);
				if (fFilterLegacy)
					boost::replace_all_regex(strUTF8Text, boost::regex("\\b" + strLegacyName + "\\b", boost::regex::icase), strAnonym);
			}
		}
	}
}
void RlvUtil::filterScriptQuestions(S32& nQuestions, LLSD& sdPayload)
{
	if ( (!gRlvAttachmentLocks.canAttach()) && (LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_ATTACH] & nQuestions) )
	{
		sdPayload["rlv_blocked"] = RLV_STRING_BLOCKED_PERMATTACH;
		nQuestions &= ~LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_ATTACH];
	}
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_TPLOC)) && (LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TELEPORT] & nQuestions) )
	{
		sdPayload["rlv_blocked"] = RLV_STRING_BLOCKED_PERMTELEPORT;
		nQuestions &= ~LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TELEPORT];
	}
	sdPayload["questions"] = nQuestions;
}
void RlvUtil::forceTp(const LLVector3d& posDest)
{
	m_fForceTp = true;
	gAgent.teleportViaLocationLookAt(posDest);
	m_fForceTp = false;
}
bool RlvUtil::isNearbyAgent(const LLUUID& idAgent)
{
	RLV_ASSERT(idAgent.notNull());
	if ( (idAgent.notNull()) && (gAgent.getID() != idAgent) )
	{
		uuid_vec_t idAgents;
		LLWorld::getInstance()->getAvatars(&idAgents, NULL);
		for (const auto& id : idAgents)
			if (id == idAgent)
				return true;
	}
	return false;
}
bool RlvUtil::isNearbyRegion(const std::string& strRegion)
{
	LLWorld::region_list_t regions = LLWorld::getInstance()->getRegionList();
	for (LLWorld::region_list_t::const_iterator itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
		if ((*itRegion)->getName() == strRegion)
			return true;
	return false;
}
void RlvUtil::notifyBlocked(const std::string& strNotifcation, const LLSD& sdArgs)
{
	std::string strMsg = RlvStrings::getString(strNotifcation);
	LLStringUtil::format(strMsg, sdArgs);
	LLSD sdNotify;
	sdNotify["MESSAGE"] = strMsg;
	LLNotificationsUtil::add("SystemMessageTip", sdNotify);
}
void RlvUtil::notifyFailedAssertion(const std::string& strAssert, const std::string& strFile, int nLine)
{
	static std::string strAssertPrev, strFilePrev; static int nLinePrev;
	if ( ((strAssertPrev == strAssert) && (strFile == strFilePrev) && (nLine == nLinePrev)) ||
		 (!rlvGetSetting<bool>(RLV_SETTING_SHOWASSERTIONFAIL, true)) )
	{
		return;
	}
	strAssertPrev = strAssert;
	strFilePrev = strFile;
	nLinePrev = nLine;
	LLSD argsNotify;
	argsNotify["MESSAGE"] = llformat("RLVa assertion failure: %s (%s - %d)", strAssert.c_str(), strFile.c_str(), nLine);
	LLNotificationsUtil::add("SystemMessageTip", argsNotify);
}
void RlvUtil::sendBusyMessage(const LLUUID& idTo, const std::string& strMsg, const LLUUID& idSession)
{
	std::string strFullName;
	LLAgentUI::buildFullname(strFullName);
	pack_instant_message(gMessageSystem, gAgent.getID(), FALSE, gAgent.getSessionID(), idTo, strFullName,
		strMsg, IM_ONLINE, IM_BUSY_AUTO_RESPONSE, idSession);
	gAgent.sendReliableMessage();
}
bool RlvUtil::sendChatReply(S32 nChannel, const std::string& strUTF8Text)
{
	if (!isValidReplyChannel(nChannel))
		return false;
	gMessageSystem->newMessageFast(_PREHASH_ChatFromViewer);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ChatData);
	gMessageSystem->addStringFast(_PREHASH_Message, utf8str_truncate(strUTF8Text, MAX_MSG_STR_LEN));
	gMessageSystem->addU8Fast(_PREHASH_Type, CHAT_TYPE_SHOUT);
	gMessageSystem->addS32("Channel", nChannel);
	gAgent.sendReliableMessage();
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CHAT_COUNT);
	return true;
}
BOOL rlvMenuCheckEnabled(void*)
{
	return rlv_handler_t::isEnabled();
}
void rlvMenuToggleEnabled(void*)
{
	gSavedSettings.setBOOL(RLV_SETTING_MAIN, !rlv_handler_t::isEnabled());
	LLSD args;
	args["MESSAGE"] =
		llformat("RestrainedLove Support will be %s after you restart", (rlv_handler_t::isEnabled()) ? "disabled" : "enabled" );
	LLNotificationsUtil::add("GenericAlert", args);
	return;
}
void rlvMenuToggleVisible()
{
	bool fTopLevel = rlvGetSetting(RLV_SETTING_TOPLEVELMENU, true);
	bool fRlvEnabled = rlv_handler_t::isEnabled();
	LLMenuGL* pAdvancedMenu = gMenuBarView->getChildMenuByName("Advanced", FALSE);
	gMenuBarView->setItemVisible("RLVa Main", (fRlvEnabled) && (fTopLevel));
	if (!pAdvancedMenu) return;
	pAdvancedMenu->setItemVisible("RLVa Embedded", (fRlvEnabled) && (!fTopLevel));
}
bool RlvEnableIfNot::handleEvent(LLPointer<LLEvent>, const LLSD& sdParam)
{
	bool fEnable = true;
	if (rlv_handler_t::isEnabled())
	{
		ERlvBehaviour eBhvr = RlvCommand::getBehaviourFromString(sdParam["data"].asString());
		fEnable = (eBhvr != RLV_BHVR_UNKNOWN) ? !gRlvHandler.hasBehaviour(eBhvr) : true;
	}
	gMenuHolder->findControl(sdParam["control"].asString())->setValue(fEnable);
	return true;
}
bool rlvCanDeleteOrReturn(const LLViewerObject* pObj)
{
	return
		( (!gRlvHandler.hasBehaviour(RLV_BHVR_REZ)) || ((!pObj->permYouOwner()) && (!pObj->permGroupOwner())) ) &&
		( (!gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) || (!isAgentAvatarValid()) || (!pObj->getRootEdit()->isChild(gAgentAvatarp)) );
}
bool rlvCanDeleteOrReturn()
{
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_REZ)) || (gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) )
	{
		struct RlvCanDeleteOrReturn : public LLSelectedObjectFunctor
		{
			bool apply(LLViewerObject* pObj) { return rlvCanDeleteOrReturn(pObj); }
		} f;
		LLObjectSelectionHandle hSel = LLSelectMgr::getInstance()->getSelection();
		return (hSel.notNull()) && (0 != hSel->getRootObjectCount()) && (hSel->applyToRootObjects(&f, false));
	}
	return true;
}
bool RlvSelectHasLockedAttach::apply(LLSelectNode* pNode)
{
	return (pNode->getObject()) ? gRlvAttachmentLocks.isLockedAttachment(pNode->getObject()->getRootEdit()) : false;
}
bool RlvSelectIsEditable::apply(LLSelectNode* pNode)
{
	const LLViewerObject* pObj = pNode->getObject();
	return (pObj) && (!gRlvHandler.canEdit(pObj));
}
bool RlvSelectIsSittingOn::apply(LLSelectNode* pNode)
{
	return (pNode->getObject()) && (pNode->getObject()->getRootEdit()->isChild(m_pAvatar));
}
bool rlvPredCanWearItem(const LLViewerInventoryItem* pItem, ERlvWearMask eWearMask)
{
	if ( (pItem) && (RlvForceWear::isWearableItem(pItem)) )
	{
		if (RlvForceWear::isWearingItem(pItem))
			return true;
		switch (pItem->getType())
		{
			case LLAssetType::AT_BODYPART:
				return (RLV_WEAR_LOCKED != (gRlvWearableLocks.canWear(pItem) & RLV_WEAR_REPLACE & eWearMask));
			case LLAssetType::AT_CLOTHING:
				return (RLV_WEAR_LOCKED != (gRlvWearableLocks.canWear(pItem) & eWearMask));
			case LLAssetType::AT_OBJECT:
				return (RLV_WEAR_LOCKED != (gRlvAttachmentLocks.canAttach(pItem) & eWearMask));
			case LLAssetType::AT_GESTURE:
				return true;
			default:
				RLV_ASSERT(false);
		}
	}
	return false;
}
bool rlvPredCanNotWearItem(const LLViewerInventoryItem* pItem, ERlvWearMask eWearMask)
{
	return !rlvPredCanWearItem(pItem, eWearMask);
}
bool rlvPredCanRemoveItem(const LLUUID& idItem)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	if (pItem)
	{
		return rlvPredCanRemoveItem(pItem);
	}
	if (isAgentAvatarValid())
	{
		const LLViewerObject* pAttachObj = gAgentAvatarp->getWornAttachment(idItem);
		return (pAttachObj) && (!gRlvAttachmentLocks.isLockedAttachment(pAttachObj));
	}
	return false;
}
bool rlvPredCanRemoveItem(const LLViewerInventoryItem* pItem)
{
	if (pItem)
	{
		switch (pItem->getType())
		{
			case LLAssetType::AT_BODYPART:
			case LLAssetType::AT_CLOTHING:
				return gRlvWearableLocks.canRemove(pItem);
			case LLAssetType::AT_OBJECT:
				return gRlvAttachmentLocks.canDetach(pItem);
			case LLAssetType::AT_GESTURE:
				return true;
			default:
				RLV_ASSERT(!RlvForceWear::isWearableItem(pItem));
		}
	}
	return false;
}
bool rlvPredCanNotRemoveItem(const LLViewerInventoryItem* pItem)
{
	return !rlvPredCanRemoveItem(pItem);
}
RlvPredIsEqualOrLinkedItem::RlvPredIsEqualOrLinkedItem(const LLUUID& idItem)
{
	m_pItem = gInventory.getItem(idItem);
}
bool RlvPredIsEqualOrLinkedItem::operator()(const LLViewerInventoryItem* pItem) const
{
	return (m_pItem) && (pItem) && (m_pItem->getLinkedUUID() == pItem->getLinkedUUID());
}
const std::string& rlvGetAnonym(const LLAvatarName& avName)
{
	return RlvStrings::getAnonym(avName);
}
