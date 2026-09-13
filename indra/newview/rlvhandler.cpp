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
#include "llagentcamera.h"
#include "llappearancemgr.h"
#include "llappviewer.h"
#include "llgroupactions.h"
#include "llhudtext.h"
#include "llstartup.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "rlvhandler.h"
#include "rlvhelper.h"
#include "rlvinventory.h"
#include "rlvlocks.h"
#include "rlvui.h"
#include "rlvextensions.h"
#include <boost/algorithm/string.hpp>
BOOL RlvHandler::m_fEnabled = FALSE;
rlv_handler_t gRlvHandler;
static bool rlvParseNotifyOption(const std::string& strOption, S32& nChannel, std::string& strFilter)
{
	boost_tokenizer tokens(strOption, boost::char_separator<char>(";", "", boost::keep_empty_tokens));
	boost_tokenizer::const_iterator itTok = tokens.begin();
	if ( (itTok == tokens.end()) || (!LLStringUtil::convertToS32(*itTok, nChannel)) || (!RlvUtil::isValidReplyChannel(nChannel)) )
		return false;
	strFilter.clear();
	if (++itTok != tokens.end())
	{
		strFilter = *itTok;
		++itTok;
	}
	return (itTok == tokens.end());
}
static bool rlvParseGetStatusOption(const std::string& strOption, std::string& strFilter, std::string& strSeparator)
{
	boost_tokenizer tokens(strOption, boost::char_separator<char>(";", "", boost::keep_empty_tokens));
	boost_tokenizer::const_iterator itTok = tokens.begin();
	strSeparator = "/";
	strFilter.clear();
	if (itTok != tokens.end())
		strFilter = *itTok++;
	else
		strFilter.clear();
	if ( (itTok != tokens.end()) && (!(*itTok).empty()) )
		strSeparator = *itTok++;
	else
		strSeparator = "/";
	return true;
}
RlvHandler::RlvHandler() : m_fCanCancelTp(true), m_posSitSource(), m_pGCTimer(NULL)
{
	gAgent.addListener(this, "new group");
	memset(m_Behaviours, 0, sizeof(S16) * RLV_BHVR_COUNT);
}
RlvHandler::~RlvHandler()
{
	gAgent.removeListener(this);
}
bool RlvHandler::hasBehaviourExcept(ERlvBehaviour eBhvr, const std::string& strOption, const LLUUID& idObj) const
{
	for (rlv_object_map_t::const_iterator itObj = m_Objects.begin(); itObj != m_Objects.end(); ++itObj)
		if ( (idObj != itObj->second.getObjectID()) && (itObj->second.hasBehaviour(eBhvr, strOption, false)) )
			return true;
	return false;
}
bool RlvHandler::hasBehaviourRoot(const LLUUID& idObjRoot, ERlvBehaviour eBhvr, const std::string& strOption) const
{
	for (rlv_object_map_t::const_iterator itObj = m_Objects.begin(); itObj != m_Objects.end(); ++itObj)
		if ( (idObjRoot == itObj->second.getRootID()) && (itObj->second.hasBehaviour(eBhvr, strOption, false)) )
			return true;
	return false;
}
void RlvHandler::addException(const LLUUID& idObj, ERlvBehaviour eBhvr, const RlvExceptionOption& varOption)
{
	m_Exceptions.insert(std::pair<ERlvBehaviour, RlvException>(eBhvr, RlvException(idObj, eBhvr, varOption)));
}
bool RlvHandler::isException(ERlvBehaviour eBhvr, const RlvExceptionOption& varOption, ERlvExceptionCheck typeCheck) const
{
	if (RLV_CHECK_DEFAULT == typeCheck)
		typeCheck = ( (hasBehaviour(eBhvr)) && (!isPermissive(eBhvr)) ) ? RLV_CHECK_STRICT : RLV_CHECK_PERMISSIVE;
	uuid_vec_t objList;
	if (RLV_CHECK_STRICT == typeCheck)
	{
		for (rlv_object_map_t::const_iterator itObj = m_Objects.begin(); itObj != m_Objects.end(); ++itObj)
			if (itObj->second.hasBehaviour(eBhvr, !hasBehaviour(RLV_BHVR_PERMISSIVE)))
				objList.push_back(itObj->first);
	}
	for (rlv_exception_map_t::const_iterator itException = m_Exceptions.lower_bound(eBhvr),
			endException = m_Exceptions.upper_bound(eBhvr); itException != endException; ++itException)
	{
		if (itException->second.varOption == varOption)
		{
			if (RLV_CHECK_PERMISSIVE == typeCheck)
				return true;
			uuid_vec_t::iterator itList = std::find(objList.begin(), objList.end(), itException->second.idObject);
			if (itList != objList.end())
				objList.erase(itList);
			if (objList.empty())
				return true;
		}
	}
	return false;
}
bool RlvHandler::isPermissive(ERlvBehaviour eBhvr) const
{
	return (RlvCommand::hasStrictVariant(eBhvr))
		? !((hasBehaviour(RLV_BHVR_PERMISSIVE)) || (isException(RLV_BHVR_PERMISSIVE, eBhvr, RLV_CHECK_PERMISSIVE)))
		: true;
}
void RlvHandler::removeException(const LLUUID& idObj, ERlvBehaviour eBhvr, const RlvExceptionOption& varOption)
{
	for (rlv_exception_map_t::iterator itException = m_Exceptions.lower_bound(eBhvr),
			endException = m_Exceptions.upper_bound(eBhvr); itException != endException; ++itException)
	{
		if ( (itException->second.idObject == idObj) && (itException->second.varOption == varOption) )
		{
			m_Exceptions.erase(itException);
			break;
		}
	}
}
void RlvHandler::addCommandHandler(RlvCommandHandler* pCmdHandler)
{
	if ( (pCmdHandler) && (std::find(m_CommandHandlers.begin(), m_CommandHandlers.end(), pCmdHandler) == m_CommandHandlers.end()) )
		m_CommandHandlers.push_back(pCmdHandler);
}
void RlvHandler::removeCommandHandler(RlvCommandHandler* pCmdHandler)
{
	if (pCmdHandler)
		m_CommandHandlers.remove(pCmdHandler);
}
void RlvHandler::clearCommandHandlers()
{
	std::list<RlvCommandHandler*>::const_iterator itHandler = m_CommandHandlers.begin();
	while (itHandler != m_CommandHandlers.end())
	{
		delete *itHandler;
		++itHandler;
	}
	m_CommandHandlers.clear();
}
bool RlvHandler::notifyCommandHandlers(rlvCommandHandler f, const RlvCommand& rlvCmd, ERlvCmdRet& eRet, bool fNotifyAll) const
{
	std::list<RlvCommandHandler*>::const_iterator itHandler = m_CommandHandlers.begin(); bool fContinue = true; eRet = RLV_RET_UNKNOWN;
	while ( (itHandler != m_CommandHandlers.end()) && ((fContinue) || (fNotifyAll)) )
	{
		ERlvCmdRet eCmdRet = RLV_RET_UNKNOWN;
		if ((fContinue = !((*itHandler)->*f)(rlvCmd, eCmdRet)) == false)
			eRet = eCmdRet;
		++itHandler;
	}
	RLV_ASSERT( (fContinue) || (eRet != RLV_RET_UNKNOWN) );
	return !fContinue;
}
ERlvCmdRet RlvHandler::processCommand(const RlvCommand& rlvCmd, bool fFromObj)
{
	#ifdef RLV_DEBUG
		RLV_INFOS << "[" << rlvCmd.getObjectID() << "]: " << rlvCmd.asString() << RLV_ENDL;
	#endif
	if (!rlvCmd.isValid())
	{
		#ifdef RLV_DEBUG
			RLV_INFOS << "\t-> invalid syntax" << RLV_ENDL;
		#endif
		return RLV_RET_FAILED_SYNTAX;
	}
	m_CurCommandStack.push(&rlvCmd); m_CurObjectStack.push(rlvCmd.getObjectID());
	const LLUUID& idCurObj = m_CurObjectStack.top();
	ERlvCmdRet eRet = RLV_RET_UNKNOWN;
	switch (rlvCmd.getParamType())
	{
		case RLV_TYPE_ADD:
			{
				if ( (m_Behaviours[rlvCmd.getBehaviourType()]) &&
					 ( (RLV_BHVR_SETDEBUG == rlvCmd.getBehaviourType()) || (RLV_BHVR_SETENV == rlvCmd.getBehaviourType()) ) )
				{
					#ifdef RLV_DEBUG
						RLV_INFOS << "\t- " << rlvCmd.getBehaviour() << " is already set by another object => discarding" << RLV_ENDL;
					#endif
					eRet = RLV_RET_FAILED_LOCK;
					break;
				}
				rlv_object_map_t::iterator itObj = m_Objects.find(idCurObj); bool fAdded = false;
				if (itObj != m_Objects.end())
				{
					RlvObject& rlvObj = itObj->second;
					fAdded = rlvObj.addCommand(rlvCmd);
				}
				else
				{
					RlvObject rlvObj(idCurObj);
					fAdded = rlvObj.addCommand(rlvCmd);
					m_Objects.insert(std::pair<LLUUID, RlvObject>(idCurObj, rlvObj));
				}
				#ifdef RLV_DEBUG
					RLV_INFOS << "\t- " << ( (fAdded) ? "adding behaviour" : "skipping duplicate" ) << RLV_ENDL;
				#endif
				if (fAdded) {
					if (!m_pGCTimer)
						m_pGCTimer = new RlvGCTimer();
					eRet = processAddRemCommand(rlvCmd);
					m_Objects.find(idCurObj)->second.setCommandRet(rlvCmd, eRet);
				}
				else
				{
					eRet = RLV_RET_SUCCESS_DUPLICATE;
				}
			}
			break;
		case RLV_TYPE_REMOVE:
			{
				rlv_object_map_t::iterator itObj = m_Objects.find(idCurObj); bool fRemoved = false;
				if (itObj != m_Objects.end())
					fRemoved = itObj->second.removeCommand(rlvCmd);
				#ifdef RLV_DEBUG
					RLV_INFOS << "\t- " << ( (fRemoved)	? "removing behaviour"
														: "skipping remove (unset behaviour or unknown object)") << RLV_ENDL;
				#endif
				if (fRemoved) {
					eRet = processAddRemCommand(rlvCmd);
					if (0 == itObj->second.m_Commands.size())
					{
						#ifdef RLV_DEBUG
							RLV_INFOS << "\t- command list empty => removing " << idCurObj << RLV_ENDL;
						#endif
						m_Objects.erase(itObj);
					}
				}
				else
				{
					eRet = RLV_RET_SUCCESS_UNSET;
				}
			}
			break;
		case RLV_TYPE_CLEAR:
			eRet = processClearCommand(rlvCmd);
			break;
		case RLV_TYPE_FORCE:
			eRet = processForceCommand(rlvCmd);
			break;
		case RLV_TYPE_REPLY:
			eRet = processReplyCommand(rlvCmd);
			break;
		case RLV_TYPE_UNKNOWN:
		default:
			eRet = RLV_RET_FAILED_PARAM;
			break;
	}
	RLV_ASSERT(RLV_RET_UNKNOWN != eRet);
	m_OnCommand(rlvCmd, eRet, !fFromObj);
	#ifdef RLV_DEBUG
		RLV_INFOS << "\t--> command " << ((eRet & RLV_RET_SUCCESS) ? "succeeded" : "failed") << RLV_ENDL;
	#endif
	m_CurCommandStack.pop(); m_CurObjectStack.pop();
	return eRet;
}
ERlvCmdRet RlvHandler::processCommand(const LLUUID& idObj, const std::string& strCommand, bool fFromObj)
{
	if (STATE_STARTED != LLStartUp::getStartupState())
	{
		m_Retained.push_back(RlvCommand(idObj, strCommand));
		return RLV_RET_RETAINED;
	}
	return processCommand(RlvCommand(idObj, strCommand), fFromObj);
}
void RlvHandler::processRetainedCommands(ERlvBehaviour eBhvrFilter , ERlvParamType eTypeFilter )
{
	rlv_command_list_t::iterator itCmd = m_Retained.begin(), itCurCmd;
	while (itCmd != m_Retained.end())
	{
		itCurCmd = itCmd++;
		const RlvCommand& rlvCmd = *itCurCmd;
		if ( ((RLV_BHVR_UNKNOWN == eBhvrFilter) || (rlvCmd.getBehaviourType() == eBhvrFilter)) &&
		     ((RLV_TYPE_UNKNOWN == eTypeFilter) || (rlvCmd.getParamType() == eTypeFilter)) )
		{
			processCommand(rlvCmd, true);
			m_Retained.erase(itCurCmd);
		}
	}
}
ERlvCmdRet RlvHandler::processClearCommand(const RlvCommand& rlvCmd)
{
	const std::string& strFilter = rlvCmd.getParam(); std::string strCmdRem;
	rlv_object_map_t::const_iterator itObj = m_Objects.find(rlvCmd.getObjectID());
	if (itObj != m_Objects.end())
	{
		const RlvObject& rlvObj = itObj->second; bool fContinue = true;
		for (rlv_command_list_t::const_iterator itCmd = rlvObj.m_Commands.begin(), itCurCmd;
				((fContinue) && (itCmd != rlvObj.m_Commands.end())); )
		{
			itCurCmd = itCmd++;
			const RlvCommand& rlvCmdRem = *itCurCmd; strCmdRem = rlvCmdRem.asString();
			if ( (strFilter.empty()) || (std::string::npos != strCmdRem.find(strFilter)) )
			{
				fContinue = (rlvObj.m_Commands.size() > 1);
				processCommand(rlvCmd.getObjectID(), strCmdRem.append("=y"), false);
			}
		}
	}
	ERlvCmdRet eRet = RLV_RET_SUCCESS;
	notifyCommandHandlers(&RlvCommandHandler::onClearCommand, rlvCmd, eRet, true);
	return RLV_RET_SUCCESS;
}
bool RlvHandler::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& sdUserdata)
{
	if ( (hasBehaviour(RLV_BHVR_SETGROUP)) && ("new group" == event->desc()) && (m_idAgentGroup != gAgent.getGroupID()) )
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ActivateGroup);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_GroupID, m_idAgentGroup);
		gAgent.sendReliableMessage();
		return true;
	}
	else
	{
		m_idAgentGroup = gAgent.getGroupID();
	}
	return false;
}
void RlvHandler::onSitOrStand(bool fSitting)
{
	#ifdef RLV_EXTENSION_STARTLOCATION
	if (rlv_handler_t::isEnabled())
	{
		RlvSettings::updateLoginLastLocation();
	}
	#endif
	if ( (hasBehaviour(RLV_BHVR_STANDTP)) && (!fSitting) && (!m_posSitSource.isExactlyZero()) )
	{
		doOnIdleOneTime(boost::bind(RlvUtil::forceTp, m_posSitSource));
		m_posSitSource.setZero();
	}
}
void RlvHandler::onAttach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt)
{
	RLV_ASSERT( (pAttachObj) && (pAttachObj == pAttachObj->getRootEdit()) );
	RLV_ASSERT( (pAttachPt) && (pAttachPt->isObjectAttached(pAttachObj)) );
	if ( (!pAttachObj) || (!pAttachPt) || (!pAttachPt->isObjectAttached(pAttachObj)) )
		return;
	for (rlv_object_map_t::iterator itObj = m_Objects.begin(); itObj != m_Objects.end(); ++itObj)
	{
		if ( (!itObj->second.m_fLookup) || (!itObj->second.m_idxAttachPt) )
		{
			const LLViewerObject* pObj = gObjectList.findObject(itObj->first);
			if ( (pObj) && (pObj->getRootEdit()->getID() == pAttachObj->getID()) )
			{
				itObj->second.m_fLookup = true;
				itObj->second.m_idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(pAttachObj);
				itObj->second.m_idRoot = pAttachObj->getID();
				if (itObj->second.hasBehaviour(RLV_BHVR_DETACH, false))
					gRlvAttachmentLocks.addAttachmentLock(pAttachObj->getID(), itObj->second.getObjectID());
			}
		}
	}
	if ( (STATE_STARTED == LLStartUp::getStartupState()) && (gInventory.isInventoryUsable()) )
	{
		RlvRenameOnWearObserver* pFetcher = new RlvRenameOnWearObserver(pAttachObj->getAttachmentItemID());
		pFetcher->startFetch();
		if (pFetcher->isFinished())
			pFetcher->done();
		else
			gInventory.addObserver(pFetcher);
	}
}
void RlvHandler::onDetach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt)
{
	RLV_ASSERT( (pAttachObj) && (pAttachObj == pAttachObj->getRootEdit()) );
	RLV_ASSERT( (pAttachPt) && (pAttachPt->isObjectAttached(pAttachObj)) );
	if ( (!pAttachObj) || (!pAttachPt) || (!pAttachPt->isObjectAttached(pAttachObj)) )
		return;
	if (!pAttachObj->isAttachment())
	{
		for (rlv_object_map_t::iterator itObj = m_Objects.begin(); itObj != m_Objects.end(); ++itObj)
		{
			if ( (itObj->second.m_fLookup) && (itObj->second.m_idRoot == pAttachObj->getID()) )
			{
				itObj->second.m_idxAttachPt = false;
				if (itObj->second.hasBehaviour(RLV_BHVR_DETACH, false))
					gRlvAttachmentLocks.removeAttachmentLock(pAttachObj->getID(), itObj->second.getObjectID());
			}
		}
	}
	else
	{
		rlv_object_map_t::iterator itObj = m_Objects.begin(), itCurObj;
		while (itObj != m_Objects.end())
		{
			itCurObj = itObj++;
#ifdef RLV_DEBUG
			bool itObj = true;
			RLV_ASSERT(itObj);
#endif
			if (itCurObj->second.m_idRoot == pAttachObj->getID())
			{
				RLV_INFOS << "Clearing " << itCurObj->first.asString() << ":" << RLV_ENDL;
				processCommand(itCurObj->second.getObjectID(), "clear", true);
				RLV_INFOS << "\t-> done" << RLV_ENDL;
			}
		}
	}
}
bool RlvHandler::onGC()
{
	rlv_object_map_t::iterator itObj = m_Objects.begin(), itCurObj;
	while (itObj != m_Objects.end())
	{
		itCurObj = itObj++;
#ifdef RLV_DEBUG
		bool itObj = true;
		RLV_ASSERT(itObj);
#endif
		RLV_ASSERT(itCurObj->first == itCurObj->second.getObjectID());
		const LLViewerObject* pObj = gObjectList.findObject(itCurObj->second.getObjectID());
		if (!pObj)
		{
			if ( (itCurObj->second.m_fLookup) || (++itCurObj->second.m_nLookupMisses > 20) )
			{
				RLV_INFOS << "Garbage collecting " << itCurObj->first.asString() << ":" << RLV_ENDL;
				processCommand(itCurObj->first, "clear", true);
				RLV_INFOS << "\t-> done" << RLV_ENDL;
			}
		}
		else
		{
			RLV_ASSERT( (itCurObj->second.m_fLookup) || (!pObj->isAttachment()) );
			if (!itCurObj->second.m_fLookup)
			{
				RLV_INFOS << "Resolved missing object " << itCurObj->first.asString() << RLV_ENDL;
				itCurObj->second.m_fLookup = true;
				itCurObj->second.m_idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(pObj);
				itCurObj->second.m_idRoot = pObj->getRootEdit()->getID();
				if ( (pObj->isAttachment()) && (itCurObj->second.hasBehaviour(RLV_BHVR_DETACH, false)) )
					gRlvAttachmentLocks.addAttachmentLock(pObj->getID(), itCurObj->second.getObjectID());
			}
		}
	}
	RLV_ASSERT(gRlvAttachmentLocks.verifyAttachmentLocks());
	return (0 != m_Objects.size());
}
void RlvHandler::onIdleStartup(void* pParam)
{
	LLTimer* pTimer = (LLTimer*)pParam;
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		if ( (LLStartUp::getStartupState() >= STATE_MISC) && (pTimer->getElapsedTimeF32() >= 2.0) )
		{
			gRlvHandler.processRetainedCommands(RLV_BHVR_VERSION, RLV_TYPE_REPLY);
			gRlvHandler.processRetainedCommands(RLV_BHVR_VERSIONNEW, RLV_TYPE_REPLY);
			gRlvHandler.processRetainedCommands(RLV_BHVR_VERSIONNUM, RLV_TYPE_REPLY);
			pTimer->reset();
		}
	}
	else
	{
		gIdleCallbacks.deleteFunction(onIdleStartup, pParam);
		delete pTimer;
	}
}
void RlvHandler::onLoginComplete()
{
	RlvInventory::instance().fetchWornItems();
	RlvInventory::instance().fetchSharedInventory();
	#ifdef RLV_EXTENSION_STARTLOCATION
	RlvSettings::updateLoginLastLocation();
	#endif
	LLViewerParcelMgr::getInstance()->setTeleportFailedCallback(boost::bind(&RlvHandler::onTeleportFailed, this));
	LLViewerParcelMgr::getInstance()->setTeleportFinishedCallback(boost::bind(&RlvHandler::onTeleportFinished, this, _1));
	processRetainedCommands();
}
void RlvHandler::onTeleportFailed()
{
	setCanCancelTp(true);
}
void RlvHandler::onTeleportFinished(const LLVector3d& posArrival)
{
	setCanCancelTp(true);
}
bool RlvHandler::canSit(LLViewerObject* pObj, const LLVector3& posOffset ) const
{
	return
		( (pObj) && (LL_PCODE_VOLUME == pObj->getPCode()) ) &&
		(!hasBehaviour(RLV_BHVR_SIT)) &&
		( ((!hasBehaviour(RLV_BHVR_UNSIT)) && (!hasBehaviour(RLV_BHVR_STANDTP))) ||
		  ((isAgentAvatarValid()) && (!gAgentAvatarp->isSitting())) ) &&
		( ( (NULL != getCurrentCommand()) && (RLV_BHVR_SIT == getCurrentCommand()->getBehaviourType()) ) ||
		  ( (!hasBehaviour(RLV_BHVR_SITTP)) && (!hasBehaviour(RLV_BHVR_FARTOUCH)) ) ||
		  (dist_vec_squared(gAgent.getPositionGlobal(), pObj->getPositionGlobal() + LLVector3d(posOffset)) < 1.5f * 1.5f) );
}
bool RlvHandler::canTouch(const LLViewerObject* pObj, const LLVector3& posOffset ) const
{
	const LLUUID& idRoot = (pObj) ? pObj->getRootEdit()->getID() : LLUUID::null;
	bool fCanTouch = (idRoot.notNull()) && ((pObj->isHUDAttachment()) || (!hasBehaviour(RLV_BHVR_TOUCHALL))) &&
		((!hasBehaviour(RLV_BHVR_TOUCHTHIS)) || (!isException(RLV_BHVR_TOUCHTHIS, idRoot, RLV_CHECK_PERMISSIVE)));
	if (fCanTouch)
	{
		if ( (!pObj->isAttachment()) || (!pObj->permYouOwner()) )
		{
			fCanTouch =
				( (!hasBehaviour(RLV_BHVR_TOUCHWORLD)) || (isException(RLV_BHVR_TOUCHWORLD, idRoot, RLV_CHECK_PERMISSIVE)) ) &&
				( (!pObj->isAttachment()) || (!hasBehaviour(RLV_BHVR_TOUCHATTACHOTHER)) ) &&
				( (!hasBehaviour(RLV_BHVR_FARTOUCH)) ||
				  (dist_vec_squared(gAgent.getPositionGlobal(), pObj->getPositionGlobal() + LLVector3d(posOffset)) <= 1.5f * 1.5f) );
		}
		else if (!pObj->isHUDAttachment())
		{
			fCanTouch =
				((!hasBehaviour(RLV_BHVR_TOUCHATTACH)) || (isException(RLV_BHVR_TOUCHATTACH, idRoot, RLV_CHECK_PERMISSIVE))) &&
				((!hasBehaviour(RLV_BHVR_TOUCHATTACHSELF)) || (isException(RLV_BHVR_TOUCHATTACH, idRoot, RLV_CHECK_PERMISSIVE)));
		}
#ifdef RLV_EXTENSION_CMD_TOUCHXXX
		else
		{
			fCanTouch = (!hasBehaviour(RLV_BHVR_TOUCHHUD)) || (isException(RLV_BHVR_TOUCHHUD, idRoot, RLV_CHECK_PERMISSIVE));
		}
#endif
	}
	if ( (!fCanTouch) && (hasBehaviour(RLV_BHVR_TOUCHME)) )
		fCanTouch = hasBehaviourRoot(idRoot, RLV_BHVR_TOUCHME);
	return fCanTouch;
}
size_t utf8str_strlen(const std::string& utf8)
{
	const char* pUTF8 = utf8.c_str(); size_t length = 0;
	for (int idx = 0, cnt = utf8.length(); idx < cnt ;idx++)
	{
		if ((pUTF8[idx] & 0xC0) != 0x80)
			length++;
	}
	return length;
}
std::string utf8str_chtruncate(const std::string& utf8, size_t length)
{
	if (0 == length)
		return std::string();
	if (utf8.length() <= length)
		return utf8;
	const char* pUTF8 = utf8.c_str(); int idx = 0;
	while ( (pUTF8[idx]) && (length > 0) )
	{
		if ((pUTF8[idx] & 0xC0) != 0x80)
			length--;
		idx++;
	}
	return utf8.substr(0, idx);
}
bool RlvHandler::filterChat(std::string& strUTF8Text, bool fFilterEmote) const
{
	if (strUTF8Text.empty())
		return false;
	bool fFilter = false;
	if (RlvUtil::isEmote(strUTF8Text))
	{
		if (fFilterEmote)
		{
			if ( (strUTF8Text.find_first_of("\"()*=^_?~") != std::string::npos) ||
				 (strUTF8Text.find(" -") != std::string::npos) || (strUTF8Text.find("- ") != std::string::npos) ||
				 (strUTF8Text.find("''") != std::string::npos) )
			{
				fFilter = true;
			}
			else if (!hasBehaviour(RLV_BHVR_EMOTE))
			{
				int idx = strUTF8Text.find('.');
				strUTF8Text = utf8str_chtruncate(strUTF8Text, ( (idx > 0) && (idx < 20) ) ? idx + 1 : 20);
			}
		}
	}
	else if (strUTF8Text[0] == '/')
	{
		fFilter = (utf8str_strlen(strUTF8Text) > 7);
	}
	else if ( (!RlvSettings::getCanOOC()) ||
			  (strUTF8Text.length() < 4) || (strUTF8Text.compare(0, 2, "((")) || (strUTF8Text.compare(strUTF8Text.length() - 2, 2, "))")) )
	{
		fFilter = true;
	}
	if (fFilter)
		strUTF8Text = (gSavedSettings.getBOOL("RestrainedLoveShowEllipsis")) ? "..." : "";
	return fFilter;
}
bool RlvHandler::hasException(ERlvBehaviour eBhvr) const
{
	return (m_Exceptions.find(eBhvr) != m_Exceptions.end());
}
bool RlvHandler::redirectChatOrEmote(const std::string& strUTF8Text) const
{
	ERlvBehaviour eBhvr = (!RlvUtil::isEmote(strUTF8Text)) ? RLV_BHVR_REDIRCHAT : RLV_BHVR_REDIREMOTE;
	if ( (strUTF8Text.empty()) || (!hasBehaviour(eBhvr)) )
		return false;
	if (RLV_BHVR_REDIRCHAT == eBhvr)
	{
		std::string strText = strUTF8Text;
		if (!filterChat(strText, false))
			return false;
	}
	for (rlv_exception_map_t::const_iterator itRedir = m_Exceptions.lower_bound(eBhvr),
			endRedir = m_Exceptions.upper_bound(eBhvr); itRedir != endRedir; ++itRedir)
	{
		S32 nChannel = boost::get<S32>(itRedir->second.varOption);
		if ( (!hasBehaviour(RLV_BHVR_SENDCHANNEL)) || (isException(RLV_BHVR_SENDCHANNEL, nChannel)) )
			RlvUtil::sendChatReply(nChannel, strUTF8Text);
	}
	return true;
}
void RlvHandler::updatePole(const ERlvBehaviour& bhvr, bool max)
{
	F32 pole(max ? F32_MAX : F32_MIN);
	for (rlv_exception_map_t::const_iterator i = m_Exceptions.lower_bound(bhvr),
			end = m_Exceptions.upper_bound(bhvr); i != end; ++i)
	{
		F32 val(boost::get<F32>(i->second.varOption));
		if (max ? val < pole : val > pole) pole = val;
	}
	m_Poles[bhvr] = pole;
}
LLColor3 RlvHandler::camDrawColor() const
{
	LLColor3 ret;
	U32 count(0);
	for (rlv_exception_map_t::const_iterator i = m_Exceptions.lower_bound(RLV_BHVR_CAMDRAWCOLOR),
			end = m_Exceptions.upper_bound(RLV_BHVR_CAMDRAWCOLOR); i != end; ++i, ++count)
		ret += boost::get<LLColor3>(i->second.varOption);
	ret.mV[0]/=count;
	ret.mV[1]/=count;
	ret.mV[2]/=count;
	return ret;
}
#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	bool RlvHandler::getCompositeInfo(const LLInventoryCategory* pFolder, std::string* pstrName) const
	{
		if (pFolder)
		{
			const std::string& cstrFolder = pFolder->getName();
			std::string::size_type idxStart = cstrFolder.find('['), idxEnd = cstrFolder.find(']', idxStart);
			if ( ((0 == idxStart) || (1 == idxStart)) && (idxEnd - idxStart > 1) )
			{
				if (pstrName)
					pstrName->assign(cstrFolder.substr(idxStart + 1, idxEnd - idxStart - 1));
				return true;
			}
		}
		return false;
	}
	bool RlvHandler::getCompositeInfo(const LLUUID& idItem, std::string* pstrName, LLViewerInventoryCategory** ppFolder) const
	{
		return false;
	}
#endif
#ifdef RLV_EXPERIMENTAL_COMPOSITE_FOLDING
	inline bool RlvHandler::isHiddenCompositeItem(const LLUUID& idItem, const std::string& cstrItemType) const
	{
		std::string strComposite; LLViewerInventoryCategory* pFolder;
		LLWearableType::EType type; S32 idxAttachPt;
		if ( (getCompositeInfo(idItem, &strComposite, &pFolder)) && (cstrItemType != strComposite) )
		{
			LLUUID idCompositeItem;
			if ((type = LLWearable::typeNameToType(strComposite)) != WT_INVALID)
			{
				idCompositeItem = gAgent.getWearableItem(type);
			}
			else if ((idxAttachPt = getAttachPointIndex(strComposite, true)) != 0)
			{
				LLVOAvatar* pAvatar; LLViewerJointAttachment* pAttachmentPt;
				if ( ((pAvatar = gAgent.getAvatarObject()) != NULL) &&
					 ((pAttachmentPt = get_if_there(pAvatar->mAttachmentPoints, idxAttachPt, (LLViewerJointAttachment*)NULL)) != NULL) )
				{
					idCompositeItem = pAttachmentPt->getItemID();
				}
			}
			if ( (idCompositeItem.notNull()) && (gInventory.isObjectDescendentOf(idCompositeItem, pFolder->getUUID())) )
				return true;
		}
		return false;
	}
#endif
#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	bool RlvHandler::canTakeOffComposite(const LLInventoryCategory* pFolder) const
	{
		LLVOAvatarSelf* pAvatar = gAgent.getAvatarObject();
		if ( (!pFolder) || (!pAvatar) )
			return false;
		if ( (!gRlvAttachmentLocks.hasLockedAttachmentPoint(RLV_LOCK_REMOVE)) ||
			 (!gRlvWearableLocks.hasLockedWearableType(RLV_LOCK_REMOVE)) )
		{
			return true;
		}
		return true;
	}
	bool RlvHandler::canWearComposite(const LLInventoryCategory* pFolder) const
	{
		LLVOAvatar* pAvatar = gAgent.getAvatarObject();
		if ( (!pFolder) || (!pAvatar) )
			return false;
		if ( (!gRlvAttachmentLocks.hasLockedAttachmentPoint(RLV_LOCK_ANY)) || (!gRlvWearableLocks.hacLockedWearableType(RLV_LOCK_ANY)) )
			return true;
		return true;
	}
#endif
BOOL RlvHandler::setEnabled(BOOL fEnable)
{
	if (m_fEnabled == fEnable)
		return fEnable;
	if (fEnable)
	{
		RLV_INFOS << "Enabling Restrained Love API support - " << RlvStrings::getVersion() << RLV_ENDL;
		m_fEnabled = TRUE;
		RlvCommand::initLookupTable();
		RlvSettings::initClass();
		RlvStrings::initClass();
		gRlvHandler.addCommandHandler(new RlvExtGetSet());
		if (LLStartUp::getStartupState() < STATE_STARTED)
			LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&RlvHandler::onLoginComplete, &gRlvHandler));
		else
			gRlvHandler.onLoginComplete();
		RlvUIEnabler::getInstance();
	}
	return m_fEnabled;
}
BOOL RlvHandler::canDisable()
{
	return FALSE;
}
void RlvHandler::clearState()
{
}
#define VERIFY_OPTION(x)		{ if (!(x)) { eRet = RLV_RET_FAILED_OPTION; break; } }
#define VERIFY_OPTION_REF(x)	{ if (!(x)) { eRet = RLV_RET_FAILED_OPTION; break; } fRefCount = true; }
ERlvCmdRet RlvHandler::processAddRemCommand(const RlvCommand& rlvCmd)
{
	ERlvBehaviour eBhvr = rlvCmd.getBehaviourType(); ERlvParamType eType = rlvCmd.getParamType();
	ERlvCmdRet eRet = RLV_RET_SUCCESS; bool fRefCount = false; const std::string& strOption = rlvCmd.getOption();
	switch (eBhvr)
	{
		case RLV_BHVR_DETACH:
			eRet = onAddRemDetach(rlvCmd, fRefCount);
			break;
		case RLV_BHVR_ADDATTACH:
		case RLV_BHVR_REMATTACH:
			eRet = onAddRemAttach(rlvCmd, fRefCount);
			break;
		case RLV_BHVR_ATTACHTHIS:
		case RLV_BHVR_DETACHTHIS:
		case RLV_BHVR_ATTACHALLTHIS:
		case RLV_BHVR_DETACHALLTHIS:
			eRet = onAddRemFolderLock(rlvCmd, fRefCount);
			break;
		case RLV_BHVR_ATTACHTHISEXCEPT:
		case RLV_BHVR_DETACHTHISEXCEPT:
		case RLV_BHVR_ATTACHALLTHISEXCEPT:
		case RLV_BHVR_DETACHALLTHISEXCEPT:
			eRet = onAddRemFolderLockException(rlvCmd, fRefCount);
			break;
		case RLV_BHVR_SETENV:
			{
				if (RlvSettings::getNoSetEnv())
				{
					eRet = RLV_RET_FAILED_DISABLED;
					break;
				}
				VERIFY_OPTION_REF(strOption.empty());
			}
			break;
		case RLV_BHVR_ADDOUTFIT:
		case RLV_BHVR_REMOUTFIT:
			{
				RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
				VERIFY_OPTION_REF( (rlvCmdOption.isEmpty()) || (rlvCmdOption.isWearableType()) );
				if (RlvForceWear::instanceExists())
					RlvForceWear::instance().done();
				ERlvLockMask eLock = (RLV_BHVR_ADDOUTFIT == eBhvr) ? RLV_LOCK_ADD : RLV_LOCK_REMOVE;
				for (int idxType = 0; idxType < LLWearableType::WT_COUNT; idxType++)
				{
					if ( (rlvCmdOption.isEmpty()) || ((LLWearableType::EType)idxType == rlvCmdOption.getWearableType()) )
					{
						if (RLV_TYPE_ADD == eType)
							gRlvWearableLocks.addWearableTypeLock((LLWearableType::EType)idxType, rlvCmd.getObjectID(), eLock);
						else
							gRlvWearableLocks.removeWearableTypeLock((LLWearableType::EType)idxType, rlvCmd.getObjectID(), eLock);
					}
				}
			}
			break;
		case RLV_BHVR_SHAREDWEAR:
		case RLV_BHVR_SHAREDUNWEAR:
			{
				VERIFY_OPTION_REF(strOption.empty());
				RlvFolderLocks::folderlock_source_t lockSource(RlvFolderLocks::ST_SHAREDPATH, LLStringUtil::null);
				RlvFolderLocks::ELockScope eLockScope = RlvFolderLocks::SCOPE_SUBTREE;
				ERlvLockMask eLockType = (RLV_BHVR_SHAREDUNWEAR == eBhvr) ? RLV_LOCK_REMOVE : RLV_LOCK_ADD;
				if (RLV_TYPE_ADD == eType)
					RlvFolderLocks::instance().addFolderLock(lockSource, RlvFolderLocks::PERM_DENY, eLockScope, rlvCmd.getObjectID(), eLockType);
				else
					RlvFolderLocks::instance().removeFolderLock(lockSource, RlvFolderLocks::PERM_DENY, eLockScope, rlvCmd.getObjectID(), eLockType);
			}
			break;
		case RLV_BHVR_UNSHAREDWEAR:
		case RLV_BHVR_UNSHAREDUNWEAR:
			{
				VERIFY_OPTION_REF(strOption.empty());
				RlvFolderLocks::folderlock_source_t lockSource(RlvFolderLocks::ST_ROOTFOLDER, 0);
				RlvFolderLocks::ELockScope eLockScope = RlvFolderLocks::SCOPE_SUBTREE;
				ERlvLockMask eLockType = (RLV_BHVR_UNSHAREDUNWEAR == eBhvr) ? RLV_LOCK_REMOVE : RLV_LOCK_ADD;
				if (RLV_TYPE_ADD == eType)
					RlvFolderLocks::instance().addFolderLock(lockSource, RlvFolderLocks::PERM_DENY, eLockScope, rlvCmd.getObjectID(), eLockType);
				else
					RlvFolderLocks::instance().removeFolderLock(lockSource, RlvFolderLocks::PERM_DENY, eLockScope, rlvCmd.getObjectID(), eLockType);
				lockSource = RlvFolderLocks::folderlock_source_t(RlvFolderLocks::ST_SHAREDPATH, LLStringUtil::null);
				if (RLV_TYPE_ADD == eType)
					RlvFolderLocks::instance().addFolderLock(lockSource, RlvFolderLocks::PERM_ALLOW, eLockScope, rlvCmd.getObjectID(), eLockType);
				else
					RlvFolderLocks::instance().removeFolderLock(lockSource, RlvFolderLocks::PERM_ALLOW, eLockScope, rlvCmd.getObjectID(), eLockType);
			}
			break;
		case RLV_BHVR_REDIRCHAT:
		case RLV_BHVR_REDIREMOTE:
			{
				S32 nChannel = 0;
				VERIFY_OPTION_REF( (LLStringUtil::convertToS32(strOption, nChannel)) && (RlvUtil::isValidReplyChannel(nChannel)) );
				if (RLV_TYPE_ADD == eType)
					addException(rlvCmd.getObjectID(), eBhvr, nChannel);
				else
					removeException(rlvCmd.getObjectID(), eBhvr, nChannel);
			}
			break;
		case RLV_BHVR_SENDCHANNEL:
			{
				S32 nChannel = 0;
				if ( (LLStringUtil::convertToS32(strOption, nChannel)) && (nChannel > 0) )
				{
					if (RLV_TYPE_ADD == eType)
						addException(rlvCmd.getObjectID(), eBhvr, nChannel);
					else
						removeException(rlvCmd.getObjectID(), eBhvr, nChannel);
					break;
				}
				VERIFY_OPTION_REF(strOption.empty());
			}
			break;
		case RLV_BHVR_NOTIFY:
			{
				S32 nChannel; std::string strFilter;
				VERIFY_OPTION_REF( (!strOption.empty()) && (rlvParseNotifyOption(strOption, nChannel, strFilter)) );
				if (RLV_TYPE_ADD == eType)
					RlvBehaviourNotifyHandler::getInstance()->addNotify(rlvCmd.getObjectID(), nChannel, strFilter);
				else
					RlvBehaviourNotifyHandler::getInstance()->removeNotify(rlvCmd.getObjectID(), nChannel, strFilter);
			}
			break;
		case RLV_BHVR_SHOWHOVERTEXT:
			{
				LLUUID idException(strOption);
				VERIFY_OPTION_REF(idException.notNull());
				if (RLV_TYPE_ADD == eType)
					addException(rlvCmd.getObjectID(), eBhvr, idException);
				else
					removeException(rlvCmd.getObjectID(), eBhvr, idException);
				LLViewerObject* pObj = gObjectList.findObject(idException);
				if ( (pObj) && (pObj->mText.notNull()) && (!pObj->mText->getObjectText().empty()) )
					pObj->mText->setString( (RLV_TYPE_ADD == eType) ? "" : pObj->mText->getObjectText());
			}
			break;
		case RLV_BHVR_CAMZOOMMAX:
		case RLV_BHVR_CAMZOOMMIN:
		case RLV_BHVR_CAMDISTMAX:
		case RLV_BHVR_CAMDISTMIN:
		case RLV_BHVR_CAMDRAWMAX:
		case RLV_BHVR_CAMDRAWMIN:
		case RLV_BHVR_CAMDRAWALPHAMAX:
		case RLV_BHVR_CAMDRAWALPHAMIN:
		case RLV_BHVR_CAMAVDIST:
		{
			F32 param;
			VERIFY_OPTION_REF(LLStringUtil::convertToF32(strOption, param));
			if (RLV_TYPE_ADD == eType)
				addException(rlvCmd.getObjectID(), eBhvr, param);
			else
				removeException(rlvCmd.getObjectID(), eBhvr, param);
			updatePole(eBhvr, eBhvr == RLV_BHVR_CAMDISTMAX || eBhvr == RLV_BHVR_CAMZOOMMAX || eBhvr == RLV_BHVR_CAMDRAWMAX || eBhvr == RLV_BHVR_CAMDRAWALPHAMAX);
			break;
		}
		case RLV_BHVR_CAMDRAWCOLOR:
		{
			LLColor3 color;
			if (!strOption.empty())
			{
				boost_tokenizer tokens(strOption, boost::char_separator<char>(";", "", boost::keep_empty_tokens));
				boost_tokenizer::const_iterator it = tokens.begin();
				for (U8 i = 0; i < LENGTHOFCOLOR3 && it != tokens.end(); ++it, ++i)
					LLStringUtil::convertToF32(*it, color.mV[i]);
				color.clamp();
			}
			if (RLV_TYPE_ADD == eType)
				addException(rlvCmd.getObjectID(), eBhvr, color);
			else
				removeException(rlvCmd.getObjectID(), eBhvr, color);
			eRet = RLV_RET_FAILED_UNKNOWN;
			break;
		}
		case RLV_BHVR_SHOWLOC:
		case RLV_BHVR_SHOWNAMES:
		case RLV_BHVR_SHOWNAMETAGS:
		case RLV_BHVR_CAMUNLOCK:
		case RLV_BHVR_CAMTEXTURES:
		case RLV_BHVR_EMOTE:
		case RLV_BHVR_SENDCHAT:
		case RLV_BHVR_CHATWHISPER:
		case RLV_BHVR_CHATNORMAL:
		case RLV_BHVR_CHATSHOUT:
		case RLV_BHVR_PERMISSIVE:
		case RLV_BHVR_SHOWINV:
		case RLV_BHVR_SHOWMINIMAP:
		case RLV_BHVR_SHOWWORLDMAP:
		case RLV_BHVR_SHOWHOVERTEXTHUD:
		case RLV_BHVR_SHOWHOVERTEXTWORLD:
		case RLV_BHVR_SHOWHOVERTEXTALL:
		case RLV_BHVR_STANDTP:
		case RLV_BHVR_TPLM:
		case RLV_BHVR_TPLOC:
		case RLV_BHVR_VIEWNOTE:
		case RLV_BHVR_VIEWSCRIPT:
		case RLV_BHVR_VIEWTEXTURE:
		case RLV_BHVR_ACCEPTPERMISSION:
#ifdef RLV_EXTENSION_CMD_ALLOWIDLE
		case RLV_BHVR_ALLOWIDLE:
#endif
		case RLV_BHVR_REZ:
		case RLV_BHVR_FARTOUCH:
#ifdef RLV_EXTENSION_CMD_INTERACT
		case RLV_BHVR_INTERACT:
#endif
		case RLV_BHVR_TOUCHATTACHSELF:
		case RLV_BHVR_TOUCHATTACHOTHER:
		case RLV_BHVR_TOUCHALL:
		case RLV_BHVR_TOUCHME:
		case RLV_BHVR_FLY:
		case RLV_BHVR_SETGROUP:
		case RLV_BHVR_ALWAYSRUN:
		case RLV_BHVR_TEMPRUN:
		case RLV_BHVR_UNSIT:
		case RLV_BHVR_SIT:
		case RLV_BHVR_SITTP:
		case RLV_BHVR_SETDEBUG:
			VERIFY_OPTION_REF(strOption.empty());
			break;
		case RLV_BHVR_RECVCHAT:
		case RLV_BHVR_RECVEMOTE:
		case RLV_BHVR_SENDIM:
		case RLV_BHVR_RECVIM:
		case RLV_BHVR_STARTIM:
		case RLV_BHVR_TPLURE:
		case RLV_BHVR_TPREQUEST:
		case RLV_BHVR_ACCEPTTP:
		case RLV_BHVR_ACCEPTTPREQUEST:
		case RLV_BHVR_TOUCHATTACH:
#ifdef RLV_EXTENSION_CMD_TOUCHXXX
		case RLV_BHVR_TOUCHHUD:
#endif
		case RLV_BHVR_TOUCHWORLD:
		case RLV_BHVR_EDIT:
			{
				LLUUID idException(strOption);
				if (idException.notNull())
				{
					if (RLV_TYPE_ADD == eType)
						addException(rlvCmd.getObjectID(), eBhvr, idException);
					else
						removeException(rlvCmd.getObjectID(), eBhvr, idException);
					break;
				}
				VERIFY_OPTION_REF(strOption.empty());
			}
			break;
		case RLV_BHVR_RECVCHATFROM:
		case RLV_BHVR_RECVEMOTEFROM:
		case RLV_BHVR_SENDIMTO:
		case RLV_BHVR_RECVIMFROM:
		case RLV_BHVR_STARTIMTO:
		case RLV_BHVR_EDITOBJ:
		case RLV_BHVR_TOUCHTHIS:
			{
				LLUUID idException(strOption);
				VERIFY_OPTION_REF(idException.notNull());
				if (RLV_TYPE_ADD == eType)
					addException(rlvCmd.getObjectID(), eBhvr, idException);
				else
					removeException(rlvCmd.getObjectID(), eBhvr, idException);
			}
			break;
		case RLV_BHVR_UNKNOWN:
			return (notifyCommandHandlers(&RlvCommandHandler::onAddRemCommand, rlvCmd, eRet, false)) ? eRet : RLV_RET_FAILED_UNKNOWN;
		default:
			eRet = RLV_RET_FAILED_PARAM;
			break;
	}
	if ( (RLV_RET_SUCCESS == eRet) && (fRefCount) )
	{
		if (RLV_TYPE_ADD == eType)
		{
			if (rlvCmd.isStrict())
				addException(rlvCmd.getObjectID(), RLV_BHVR_PERMISSIVE, eBhvr);
			m_Behaviours[eBhvr]++;
		}
		else
		{
			if (rlvCmd.isStrict())
				removeException(rlvCmd.getObjectID(), RLV_BHVR_PERMISSIVE, eBhvr);
			m_Behaviours[eBhvr]--;
		}
		m_OnBehaviour(eBhvr, eType);
		if ( ((RLV_TYPE_ADD == eType) && (1 == m_Behaviours[eBhvr])) || ((RLV_TYPE_REMOVE == eType) && (0 == m_Behaviours[eBhvr])) )
			m_OnBehaviourToggle(eBhvr, eType);
	}
	return eRet;
}
ERlvCmdRet RlvHandler::onAddRemAttach(const RlvCommand& rlvCmd, bool& fRefCount)
{
	RLV_ASSERT( (RLV_TYPE_ADD == rlvCmd.getParamType()) || (RLV_TYPE_REMOVE == rlvCmd.getParamType()) );
	RLV_ASSERT( (RLV_BHVR_ADDATTACH == rlvCmd.getBehaviourType()) || (RLV_BHVR_REMATTACH == rlvCmd.getBehaviourType()) );
	S32 idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(rlvCmd.getOption());
	if ( (!idxAttachPt) && (!rlvCmd.getOption().empty())  )
		return RLV_RET_FAILED_OPTION;
	if (!isAgentAvatarValid())
		return RLV_RET_FAILED;
	if (RlvForceWear::instanceExists())
		RlvForceWear::instance().done();
	ERlvLockMask eLock = (RLV_BHVR_REMATTACH == rlvCmd.getBehaviourType()) ? RLV_LOCK_REMOVE : RLV_LOCK_ADD;
	for (LLVOAvatar::attachment_map_t::const_iterator itAttach = gAgentAvatarp->mAttachmentPoints.begin();
			itAttach != gAgentAvatarp->mAttachmentPoints.end(); ++itAttach)
	{
		if ( (0 == idxAttachPt) || (itAttach->first == idxAttachPt) )
		{
			if (RLV_TYPE_ADD == rlvCmd.getParamType())
				gRlvAttachmentLocks.addAttachmentPointLock(itAttach->first, rlvCmd.getObjectID(), eLock);
			else
				gRlvAttachmentLocks.removeAttachmentPointLock(itAttach->first, rlvCmd.getObjectID(), eLock);
		}
	}
	fRefCount = rlvCmd.getOption().empty();
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onAddRemDetach(const RlvCommand& rlvCmd, bool& fRefCount)
{
	RLV_ASSERT( (RLV_TYPE_ADD == rlvCmd.getParamType()) || (RLV_TYPE_REMOVE == rlvCmd.getParamType()) );
	RLV_ASSERT(RLV_BHVR_DETACH == rlvCmd.getBehaviourType());
	if (RlvForceWear::instanceExists())
		RlvForceWear::instance().done();
	if (rlvCmd.getOption().empty())
	{
		rlv_object_map_t::const_iterator itObj = m_Objects.find(rlvCmd.getObjectID());
		if ( (itObj != m_Objects.end()) && (itObj->second.m_fLookup) && (itObj->second.m_idxAttachPt) )
		{
			if (RLV_TYPE_ADD == rlvCmd.getParamType())
				gRlvAttachmentLocks.addAttachmentLock(itObj->second.m_idRoot, itObj->first);
			else
				gRlvAttachmentLocks.removeAttachmentLock(itObj->second.m_idRoot, itObj->first);
		}
	}
	else
	{
		S32 idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(rlvCmd.getOption());
		if (0 == idxAttachPt)
			return RLV_RET_FAILED_OPTION;
		if (RLV_TYPE_ADD == rlvCmd.getParamType())
			gRlvAttachmentLocks.addAttachmentPointLock(idxAttachPt, rlvCmd.getObjectID(), (ERlvLockMask)(RLV_LOCK_ADD | RLV_LOCK_REMOVE));
		else
			gRlvAttachmentLocks.removeAttachmentPointLock(idxAttachPt, rlvCmd.getObjectID(), (ERlvLockMask)(RLV_LOCK_ADD | RLV_LOCK_REMOVE));
	}
	fRefCount = false;
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onAddRemFolderLock(const RlvCommand& rlvCmd, bool& fRefCount)
{
	RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
	RlvFolderLocks::folderlock_source_t lockSource;
	if (rlvCmdOption.isEmpty())
	{
		lockSource = RlvFolderLocks::folderlock_source_t(RlvFolderLocks::ST_ATTACHMENT, rlvCmd.getObjectID());
	}
	else if (rlvCmdOption.isSharedFolder())
	{
		lockSource = RlvFolderLocks::folderlock_source_t(RlvFolderLocks::ST_SHAREDPATH, rlvCmd.getOption());
	}
	else if (rlvCmdOption.isAttachmentPoint())
	{
		lockSource = RlvFolderLocks::folderlock_source_t(RlvFolderLocks::ST_ATTACHMENTPOINT, RlvAttachPtLookup::getAttachPointIndex(rlvCmdOption.getAttachmentPoint()));
	}
	else if (rlvCmdOption.isWearableType())
	{
		lockSource = RlvFolderLocks::folderlock_source_t(RlvFolderLocks::ST_WEARABLETYPE, rlvCmdOption.getWearableType());
	}
	else
	{
		fRefCount = false;
		return RLV_RET_FAILED_OPTION;
	}
	ERlvBehaviour eBhvr = rlvCmd.getBehaviourType();
 	ERlvLockMask eLockType = ((RLV_BHVR_ATTACHTHIS == eBhvr) || (RLV_BHVR_ATTACHALLTHIS == eBhvr)) ? RLV_LOCK_ADD : RLV_LOCK_REMOVE;
	RlvFolderLocks::ELockPermission eLockPermission = RlvFolderLocks::PERM_DENY;
	RlvFolderLocks::ELockScope eLockScope =
		((RLV_BHVR_ATTACHALLTHIS == eBhvr) || (RLV_BHVR_DETACHALLTHIS == eBhvr)) ? RlvFolderLocks::SCOPE_SUBTREE : RlvFolderLocks::SCOPE_NODE;
	if (RLV_TYPE_ADD == rlvCmd.getParamType())
		RlvFolderLocks::instance().addFolderLock(lockSource, eLockPermission, eLockScope, rlvCmd.getObjectID(), eLockType);
	else
		RlvFolderLocks::instance().removeFolderLock(lockSource, eLockPermission, eLockScope, rlvCmd.getObjectID(), eLockType);
	fRefCount = true;
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onAddRemFolderLockException(const RlvCommand& rlvCmd, bool& fRefCount)
{
	RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
	if (!rlvCmdOption.isSharedFolder())
		return RLV_RET_FAILED_OPTION;
	ERlvBehaviour eBhvr = rlvCmd.getBehaviourType();
 	ERlvLockMask eLockType =
		((RLV_BHVR_ATTACHTHISEXCEPT == eBhvr) || (RLV_BHVR_ATTACHALLTHISEXCEPT == eBhvr)) ? RLV_LOCK_ADD : RLV_LOCK_REMOVE;
	RlvFolderLocks::ELockPermission eLockPermission = RlvFolderLocks::PERM_ALLOW;
	RlvFolderLocks::ELockScope eLockScope =
		((RLV_BHVR_ATTACHALLTHISEXCEPT == eBhvr) || (RLV_BHVR_DETACHALLTHISEXCEPT == eBhvr)) ? RlvFolderLocks::SCOPE_SUBTREE : RlvFolderLocks::SCOPE_NODE;
	RlvFolderLocks::folderlock_source_t lockSource(RlvFolderLocks::ST_SHAREDPATH, rlvCmd.getOption());
	if (RLV_TYPE_ADD == rlvCmd.getParamType())
		RlvFolderLocks::instance().addFolderLock(lockSource, eLockPermission, eLockScope, rlvCmd.getObjectID(), eLockType);
	else
		RlvFolderLocks::instance().removeFolderLock(lockSource, eLockPermission, eLockScope, rlvCmd.getObjectID(), eLockType);
	fRefCount = true;
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::processForceCommand(const RlvCommand& rlvCmd) const
{
	RLV_ASSERT(RLV_TYPE_FORCE == rlvCmd.getParamType());
	ERlvCmdRet eRet = RLV_RET_SUCCESS;
	switch (rlvCmd.getBehaviourType())
	{
		case RLV_BHVR_DETACH:
		case RLV_BHVR_REMATTACH:
			{
				RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
				if (rlvCmdOption.isSharedFolder())
					eRet = onForceWear(rlvCmdOption.getSharedFolder(), RLV_BHVR_DETACH);
				else
					eRet = onForceRemAttach(rlvCmd);
			}
			break;
		case RLV_BHVR_REMOUTFIT:
			{
				RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
				if (rlvCmdOption.isSharedFolder())
					eRet = onForceWear(rlvCmdOption.getSharedFolder(), RLV_BHVR_DETACH);
				else
					eRet = onForceRemOutfit(rlvCmd);
			}
			break;
		case RLV_BHVR_SETGROUP:
			eRet = onForceGroup(rlvCmd);
			break;
		case RLV_BHVR_UNSIT:
			{
				VERIFY_OPTION(rlvCmd.getOption().empty());
				if ( (isAgentAvatarValid()) && (gAgentAvatarp->isSitting()) && (!hasBehaviourExcept(RLV_BHVR_UNSIT, rlvCmd.getObjectID())) )
				{
					gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
					send_agent_update(TRUE, TRUE);
				}
			}
			break;
		case RLV_BHVR_SIT:
			eRet = onForceSit(rlvCmd);
			break;
		case RLV_BHVR_ADJUSTHEIGHT:
			{
				RlvCommandOptionAdjustHeight rlvCmdOption(rlvCmd);
				VERIFY_OPTION(rlvCmdOption.isValid());
				if (isAgentAvatarValid())
				{
					F32 nValue = (rlvCmdOption.m_nPelvisToFoot - gAgentAvatarp->getPelvisToFoot()) * rlvCmdOption.m_nPelvisToFootDeltaMult;
					nValue += rlvCmdOption.m_nPelvisToFootOffset;
					if (gAgentAvatarp->getRegion()->avatarHoverHeightEnabled())
					{
						LLVector3 avOffset(0.0, 0.0, llclamp<F32>(nValue, MIN_HOVER_Z, MAX_HOVER_Z));
						gSavedPerAccountSettings.setF32("AvatarHoverOffsetZ", avOffset.mV[VZ]);
						gAgentAvatarp->setHoverOffset(avOffset, true);
					}
					else
					{
						eRet = RLV_RET_FAILED_DISABLED;
					}
				}
			}
			break;
		case RLV_BHVR_TPTO:
			{
				RlvCommandOptionTpTo rlvCmdOption(rlvCmd);
				VERIFY_OPTION( (rlvCmdOption.isValid()) && (!rlvCmdOption.m_posGlobal.isNull()) );
				gAgent.teleportViaLocation(rlvCmdOption.m_posGlobal);
			}
			break;
		case RLV_BHVR_ATTACH:
		case RLV_BHVR_ATTACHOVER:
		case RLV_BHVR_ATTACHALL:
		case RLV_BHVR_ATTACHALLOVER:
		case RLV_BHVR_DETACHALL:
			{
				RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
				VERIFY_OPTION(rlvCmdOption.isSharedFolder());
				eRet = onForceWear(rlvCmdOption.getSharedFolder(), rlvCmd.getBehaviourType());
			}
			break;
		case RLV_BHVR_ATTACHTHIS:
		case RLV_BHVR_ATTACHTHISOVER:
		case RLV_BHVR_DETACHTHIS:
		case RLV_BHVR_ATTACHALLTHIS:
		case RLV_BHVR_ATTACHALLTHISOVER:
		case RLV_BHVR_DETACHALLTHIS:
			{
				RlvCommandOptionGetPath rlvGetPathOption(rlvCmd, boost::bind(&RlvHandler::onForceWearCallback, this, _1, rlvCmd.getBehaviourType()));
				VERIFY_OPTION(rlvGetPathOption.isValid());
				eRet = (!rlvGetPathOption.isCallback()) ? RLV_RET_SUCCESS : RLV_RET_SUCCESS_DELAYED;
			}
			break;
		case RLV_BHVR_DETACHME:
			{
				VERIFY_OPTION(rlvCmd.getOption().empty());
				const LLViewerObject* pAttachObj = gObjectList.findObject(rlvCmd.getObjectID());
				if ( (pAttachObj) && (pAttachObj->isAttachment()) )
				{
					LLVOAvatarSelf::detachAttachmentIntoInventory(pAttachObj->getAttachmentItemID());
				}
			}
			break;
		case RLV_BHVR_UNKNOWN:
			return (notifyCommandHandlers(&RlvCommandHandler::onForceCommand, rlvCmd, eRet, false)) ? eRet : RLV_RET_FAILED_UNKNOWN;
		default:
			eRet = RLV_RET_FAILED_PARAM;
			break;
	}
	return eRet;
}
ERlvCmdRet RlvHandler::onForceRemAttach(const RlvCommand& rlvCmd) const
{
	RLV_ASSERT(RLV_TYPE_FORCE == rlvCmd.getParamType());
	RLV_ASSERT( (RLV_BHVR_REMATTACH == rlvCmd.getBehaviourType()) || (RLV_BHVR_DETACH == rlvCmd.getBehaviourType()) );
	if (!isAgentAvatarValid())
		return RLV_RET_FAILED;
	RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
	if (rlvCmdOption.isAttachmentPoint())
	{
		RlvForceWear::instance().forceDetach(rlvCmdOption.getAttachmentPoint());
		return RLV_RET_SUCCESS;
	}
	else if ( (rlvCmdOption.isAttachmentPointGroup()) || (rlvCmdOption.isEmpty()) )
	{
		for (const auto& entryAttachPt : gAgentAvatarp->mAttachmentPoints)
		{
			const LLViewerJointAttachment* pAttachPt = entryAttachPt.second;
			if ( (pAttachPt) && (pAttachPt->getNumObjects()) && ((rlvCmdOption.isEmpty()) || (rlvAttachGroupFromIndex(pAttachPt->getGroup()) == rlvCmdOption.getAttachmentPointGroup())) )
			{
				RlvForceWear::instance().forceDetach(pAttachPt);
			}
		}
		return RLV_RET_SUCCESS;
	}
	else if (rlvCmdOption.isUUID())
	{
		const LLViewerObject* pAttachObj = gObjectList.findObject(rlvCmdOption.getUUID());
		if ( (pAttachObj) && (pAttachObj->isAttachment()) && (pAttachObj->permYouOwner()) )
			RlvForceWear::instance().forceDetach(pAttachObj);
		return RLV_RET_SUCCESS;
	}
	return RLV_RET_FAILED_OPTION;
}
ERlvCmdRet RlvHandler::onForceRemOutfit(const RlvCommand& rlvCmd) const
{
	RlvCommandOptionGeneric rlvCmdOption(rlvCmd.getOption());
	if ( (!rlvCmdOption.isWearableType()) && (!rlvCmdOption.isEmpty()) )
		return RLV_RET_FAILED_OPTION;
	for (int idxType = 0; idxType < LLWearableType::WT_COUNT; idxType++)
	{
		if ( (rlvCmdOption.isEmpty()) || ((LLWearableType::EType)idxType == rlvCmdOption.getWearableType()))
			RlvForceWear::instance().forceRemove((LLWearableType::EType)idxType);
	}
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onForceGroup(const RlvCommand& rlvCmd) const
{
	if (hasBehaviourExcept(RLV_BHVR_SETGROUP, rlvCmd.getObjectID()))
	{
		return RLV_RET_FAILED_LOCK;
	}
	LLUUID idGroup; bool fValid = false;
	if ("none" == rlvCmd.getOption())
	{
		idGroup.setNull();
		fValid = true;
	}
	else if (idGroup.set(rlvCmd.getOption()))
	{
		fValid = (idGroup.isNull()) || (gAgent.isInGroup(idGroup, true));
	}
	else
	{
		bool fExactMatch = false;
		for (const auto& groupData : gAgent.mGroups)
		{
			if (boost::istarts_with(groupData.mName, rlvCmd.getOption()))
			{
				idGroup = groupData.mID;
				fExactMatch = groupData.mName.length() == rlvCmd.getOption().length();
				if (fExactMatch)
					break;
			}
		}
		fValid = idGroup.notNull();
	}
	if (fValid)
	{
		m_idAgentGroup = idGroup;
		LLGroupActions::activate(idGroup);
	}
	return (fValid) ? RLV_RET_SUCCESS : RLV_RET_FAILED_OPTION;
}
ERlvCmdRet RlvHandler::onForceSit(const RlvCommand& rlvCmd) const
{
	LLViewerObject* pObj = NULL; LLUUID idTarget(rlvCmd.getOption());
	if ( (idTarget.isNull()) || ((pObj = gObjectList.findObject(idTarget)) == NULL) || (LL_PCODE_VOLUME != pObj->getPCode()) )
		return RLV_RET_FAILED_OPTION;
	if (!canSit(pObj))
		return RLV_RET_FAILED_LOCK;
	else if ( (hasBehaviour(RLV_BHVR_STANDTP)) && (isAgentAvatarValid()) )
	{
		if (gAgentAvatarp->isSitting())
			return RLV_RET_FAILED_LOCK;
		m_posSitSource = gAgent.getPositionGlobal();
	}
	gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
	gMessageSystem->addUUIDFast(_PREHASH_TargetID, pObj->mID);
	gMessageSystem->addVector3Fast(_PREHASH_Offset, LLVector3::zero);
	pObj->getRegion()->sendReliableMessage();
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onForceWear(const LLViewerInventoryCategory* pFolder, ERlvBehaviour eBhvr) const
{
	if ( (pFolder) && (!RlvInventory::instance().isSharedFolder(pFolder->getUUID())) )
		return RLV_RET_FAILED_OPTION;
	RlvForceWear::EWearAction eAction = RlvForceWear::ACTION_WEAR_REPLACE;
	if ( (RLV_BHVR_ATTACHOVER == eBhvr) || (RLV_BHVR_ATTACHTHISOVER == eBhvr) ||
		 (RLV_BHVR_ATTACHALLOVER == eBhvr) || (RLV_BHVR_ATTACHALLTHISOVER == eBhvr) )
	{
		eAction = RlvForceWear::ACTION_WEAR_ADD;
	}
	else if ( (RLV_BHVR_DETACH == eBhvr) || (RLV_BHVR_DETACHTHIS == eBhvr) ||
		      (RLV_BHVR_DETACHALL == eBhvr) || (RLV_BHVR_DETACHALLTHIS == eBhvr) )
	{
		eAction = RlvForceWear::ACTION_REMOVE;
	}
	RlvForceWear::EWearFlags eFlags = RlvForceWear::FLAG_DEFAULT;
	if ( (RLV_BHVR_ATTACHALL == eBhvr) || (RLV_BHVR_ATTACHALLOVER == eBhvr) || (RLV_BHVR_DETACHALL == eBhvr) ||
		 (RLV_BHVR_ATTACHALLTHIS == eBhvr) || (RLV_BHVR_ATTACHALLTHISOVER == eBhvr) || (RLV_BHVR_DETACHALLTHIS == eBhvr) )
	{
		eFlags = (RlvForceWear::EWearFlags)(eFlags | RlvForceWear::FLAG_MATCHALL);
	}
	RlvForceWear::instance().forceFolder(pFolder, eAction, eFlags);
	return RLV_RET_SUCCESS;
}
void RlvHandler::onForceWearCallback(const uuid_vec_t& idItems, ERlvBehaviour eBhvr) const
{
	LLInventoryModel::cat_array_t folders;
	if (RlvInventory::instance().getPath(idItems, folders))
	{
		for (S32 idxFolder = 0, cntFolder = folders.size(); idxFolder < cntFolder; idxFolder++)
			onForceWear(folders.at(idxFolder), eBhvr);
		if ( (!getCurrentCommand()) && (RlvForceWear::instanceExists()) )
			RlvForceWear::instance().done();
	}
}
ERlvCmdRet RlvHandler::processReplyCommand(const RlvCommand& rlvCmd) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	S32 nChannel;
	if ( (!LLStringUtil::convertToS32(rlvCmd.getParam(), nChannel)) || (!RlvUtil::isValidReplyChannel(nChannel)) )
		return RLV_RET_FAILED_PARAM;
	ERlvCmdRet eRet = RLV_RET_SUCCESS; std::string strReply;
	switch (rlvCmd.getBehaviourType())
	{
		case RLV_BHVR_VERSION:
		case RLV_BHVR_VERSIONNEW:
			strReply = RlvStrings::getVersion(RLV_BHVR_VERSION == rlvCmd.getBehaviourType());
			break;
		case RLV_BHVR_VERSIONNUM:
			strReply = RlvStrings::getVersionNum();
			break;
		case RLV_BHVR_GETATTACH:
			eRet = onGetAttach(rlvCmd, strReply);
			break;
		case RLV_BHVR_GETATTACHNAMES:
		case RLV_BHVR_GETADDATTACHNAMES:
		case RLV_BHVR_GETREMATTACHNAMES:
			eRet = onGetAttachNames(rlvCmd, strReply);
			break;
		case RLV_BHVR_GETOUTFIT:
			eRet = onGetOutfit(rlvCmd, strReply);
			break;
		case RLV_BHVR_GETOUTFITNAMES:
		case RLV_BHVR_GETADDOUTFITNAMES:
		case RLV_BHVR_GETREMOUTFITNAMES:
			eRet = onGetOutfitNames(rlvCmd, strReply);
			break;
		case RLV_BHVR_FINDFOLDER:
		case RLV_BHVR_FINDFOLDERS:
			eRet = onFindFolder(rlvCmd, strReply);
			break;
		case RLV_BHVR_GETPATH:
		case RLV_BHVR_GETPATHNEW:
			eRet = onGetPath(rlvCmd, strReply);
			break;
		case RLV_BHVR_GETINV:
			eRet = onGetInv(rlvCmd, strReply);
			break;
		case RLV_BHVR_GETINVWORN:
			eRet = onGetInvWorn(rlvCmd, strReply);
			break;
		case RLV_BHVR_GETGROUP:
			strReply = (gAgent.getGroupID().notNull()) ? gAgent.getGroupName() : "none";
			break;
		case RLV_BHVR_GETSITID:
			{
				LLUUID idSitObj;
				if ( (isAgentAvatarValid()) && (gAgentAvatarp->isSitting()) )
				{
					const LLViewerObject* pSeatObj = dynamic_cast<LLViewerObject*>(gAgentAvatarp->getRoot());
					if (pSeatObj)
						idSitObj = pSeatObj->getID();
				}
				strReply = idSitObj.asString();
			}
			break;
#ifdef RLV_EXTENSION_CMD_GETCOMMAND
		case RLV_BHVR_GETCOMMAND:
			{
				RlvCommand::bhvr_map_t cmdList;
				if (RlvCommand::getCommands(cmdList, rlvCmd.getOption()))
					for (RlvCommand::bhvr_map_t::const_iterator itCmd = cmdList.begin(); itCmd != cmdList.end(); ++itCmd)
						strReply.append("/").append(itCmd->first);
			}
			break;
#endif
		case RLV_BHVR_GETSTATUS:
			{
				std::string strFilter, strSeparator;
				if (rlvParseGetStatusOption(rlvCmd.getOption(), strFilter, strSeparator))
				{
					rlv_object_map_t::const_iterator itObj = m_Objects.find(rlvCmd.getObjectID());
					if (itObj != m_Objects.end())
						strReply = itObj->second.getStatusString(strFilter, strSeparator);
				}
			}
			break;
		case RLV_BHVR_GETSTATUSALL:
			{
				std::string strFilter, strSeparator;
				if (rlvParseGetStatusOption(rlvCmd.getOption(), strFilter, strSeparator))
				{
					for (rlv_object_map_t::const_iterator itObj = m_Objects.begin(); itObj != m_Objects.end(); ++itObj)
						strReply += itObj->second.getStatusString(strFilter, strSeparator);
				}
			}
			break;
		case RLV_BHVR_UNKNOWN:
			return (notifyCommandHandlers(&RlvCommandHandler::onReplyCommand, rlvCmd, eRet, false)) ? eRet : RLV_RET_FAILED_UNKNOWN;
		default:
			return RLV_RET_FAILED_PARAM;
	}
	RlvUtil::sendChatReply(nChannel, strReply);
	return eRet;
}
ERlvCmdRet RlvHandler::onFindFolder(const RlvCommand& rlvCmd, std::string& strReply) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	RLV_ASSERT( (RLV_BHVR_FINDFOLDER == rlvCmd.getBehaviourType()) || (RLV_BHVR_FINDFOLDERS == rlvCmd.getBehaviourType()) );
	if (rlvCmd.getOption().empty())
		return RLV_RET_FAILED_OPTION;
	LLInventoryModel::cat_array_t folders;
	if (RlvInventory::instance().findSharedFolders(rlvCmd.getOption(), folders))
	{
		if (RLV_BHVR_FINDFOLDER == rlvCmd.getBehaviourType())
		{
			int maxSlashes = -1, curSlashes; std::string strFolderName;
			for (S32 idxFolder = 0, cntFolder = folders.size(); idxFolder < cntFolder; idxFolder++)
			{
				strFolderName = RlvInventory::instance().getSharedPath(folders.at(idxFolder));
				curSlashes = std::count(strFolderName.begin(), strFolderName.end(), '/');
				if (curSlashes > maxSlashes)
				{
					maxSlashes = curSlashes;
					strReply = strFolderName;
				}
			}
		}
		else if (RLV_BHVR_FINDFOLDERS == rlvCmd.getBehaviourType())
		{
			for (S32 idxFolder = 0, cntFolder = folders.size(); idxFolder < cntFolder; idxFolder++)
			{
				if (!strReply.empty())
					strReply.push_back(',');
				strReply += RlvInventory::instance().getSharedPath(folders.at(idxFolder));
			}
		}
	}
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onGetAttach(const RlvCommand& rlvCmd, std::string& strReply) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	RLV_ASSERT(RLV_BHVR_GETATTACH == rlvCmd.getBehaviourType());
	if (!isAgentAvatarValid())
		return RLV_RET_FAILED;
	S32 idxAttachPt = 0;
	if ( (rlvCmd.hasOption()) && ((idxAttachPt = RlvAttachPtLookup::getAttachPointIndex(rlvCmd.getOption())) == 0) )
		return RLV_RET_FAILED_OPTION;
	if (0 == idxAttachPt)
		strReply.push_back('0');
	for (LLVOAvatar::attachment_map_t::const_iterator itAttach = gAgentAvatarp->mAttachmentPoints.begin();
			itAttach != gAgentAvatarp->mAttachmentPoints.end(); ++itAttach)
	{
		const LLViewerJointAttachment* pAttachPt = itAttach->second;
		if ( (0 == idxAttachPt) || (itAttach->first == idxAttachPt) )
		{
			bool fWorn = (pAttachPt->getNumObjects() > 0) &&
				( (!RlvSettings::getHideLockedAttach()) || (RlvForceWear::isForceDetachable(pAttachPt, true, rlvCmd.getObjectID())) );
			strReply.push_back( (fWorn) ? '1' : '0' );
		}
	}
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onGetAttachNames(const RlvCommand& rlvCmd, std::string& strReply) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	RLV_ASSERT( (RLV_BHVR_GETATTACHNAMES == rlvCmd.getBehaviourType()) || (RLV_BHVR_GETADDATTACHNAMES == rlvCmd.getBehaviourType()) ||
		        (RLV_BHVR_GETREMATTACHNAMES == rlvCmd.getBehaviourType()) );
	if (!isAgentAvatarValid())
		return RLV_RET_FAILED;
	ERlvAttachGroupType eAttachGroup = rlvAttachGroupFromString(rlvCmd.getOption());
	for (LLVOAvatar::attachment_map_t::const_iterator itAttach = gAgentAvatarp->mAttachmentPoints.begin();
			itAttach != gAgentAvatarp->mAttachmentPoints.end(); ++itAttach)
	{
		const LLViewerJointAttachment* pAttachPt = itAttach->second;
		if ( (RLV_ATTACHGROUP_INVALID == eAttachGroup) || (rlvAttachGroupFromIndex(pAttachPt->getGroup()) == eAttachGroup) )
		{
			bool fAdd = false;
			switch (rlvCmd.getBehaviourType())
			{
				case RLV_BHVR_GETATTACHNAMES:
					fAdd = (pAttachPt->getNumObjects() > 0);
					break;
				case RLV_BHVR_GETADDATTACHNAMES:
					fAdd = (gRlvAttachmentLocks.canAttach(pAttachPt) & RLV_WEAR);
					break;
				case RLV_BHVR_GETREMATTACHNAMES:
					fAdd = RlvForceWear::isForceDetachable(pAttachPt);
					break;
				default:
					break;
			}
			if (fAdd)
			{
				if (!strReply.empty())
					strReply.push_back(',');
				strReply.append(pAttachPt->getName());
			}
		}
	}
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onGetInv(const RlvCommand& rlvCmd, std::string& strReply) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	RLV_ASSERT(RLV_BHVR_GETINV == rlvCmd.getBehaviourType());
	const LLViewerInventoryCategory* pFolder = RlvInventory::instance().getSharedFolder(rlvCmd.getOption());
	if (!pFolder)
		return (RlvInventory::instance().getSharedRoot() != NULL) ? RLV_RET_FAILED_OPTION : RLV_RET_FAILED_NOSHAREDROOT;
	LLInventoryModel::cat_array_t* pFolders; LLInventoryModel::item_array_t* pItems;
	gInventory.getDirectDescendentsOf(pFolder->getUUID(), pFolders, pItems);
	if (!pFolders)
		return RLV_RET_FAILED;
	for (S32 idxFolder = 0, cntFolder = pFolders->size(); idxFolder < cntFolder; idxFolder++)
	{
		const std::string& strFolder = pFolders->at(idxFolder)->getName();
		if ( (!strFolder.empty()) && (RLV_FOLDER_PREFIX_HIDDEN != strFolder[0]) &&
			 (!RlvInventory::isFoldedFolder(pFolders->at(idxFolder).get(), false)) )
		{
			if (!strReply.empty())
				strReply.push_back(',');
			strReply += strFolder;
		}
	}
	return RLV_RET_SUCCESS;
}
struct rlv_wear_info { U32 cntWorn, cntTotal, cntChildWorn, cntChildTotal; };
ERlvCmdRet RlvHandler::onGetInvWorn(const RlvCommand& rlvCmd, std::string& strReply) const
{
	if (!isAgentAvatarValid())
		return RLV_RET_FAILED;
	LLViewerInventoryCategory* pFolder = RlvInventory::instance().getSharedFolder(rlvCmd.getOption());
	if (!pFolder)
		return (RlvInventory::instance().getSharedRoot() != NULL) ? RLV_RET_FAILED_OPTION : RLV_RET_FAILED_NOSHAREDROOT;
	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	RlvWearableItemCollector f(pFolder, RlvForceWear::ACTION_WEAR_REPLACE, RlvForceWear::FLAG_MATCHALL);
	gInventory.collectDescendentsIf(pFolder->getUUID(), folders, items, FALSE, f, true);
	rlv_wear_info wi = {};
	std::map<LLUUID, rlv_wear_info> mapFolders;
	mapFolders.insert(std::pair<LLUUID, rlv_wear_info>(pFolder->getUUID(), wi));
	for (S32 idxFolder = 0, cntFolder = folders.size(); idxFolder < cntFolder; idxFolder++)
		mapFolders.insert(std::pair<LLUUID, rlv_wear_info>(folders.at(idxFolder)->getUUID(), wi));
	LLViewerInventoryItem* pItem; std::map<LLUUID, rlv_wear_info>::iterator itFolder;
	for (S32 idxItem = 0, cntItem = items.size(); idxItem < cntItem; idxItem++)
	{
		pItem = items.at(idxItem);
		if (!RlvForceWear::isWearableItem(pItem))
			continue;
		const LLUUID& idParent = f.getFoldedParent(pItem->getParentUUID());
		LLViewerInventoryCategory* pParent = gInventory.getCategory(idParent);
		while ( (itFolder = mapFolders.find(pParent->getUUID())) == mapFolders.end() )
			pParent = gInventory.getCategory(pParent->getParentUUID());
		U32 &cntWorn  = (idParent == pParent->getUUID()) ? itFolder->second.cntWorn : itFolder->second.cntChildWorn,
			&cntTotal = (idParent == pParent->getUUID()) ? itFolder->second.cntTotal : itFolder->second.cntChildTotal;
		if (RlvForceWear::isWearingItem(pItem))
			cntWorn++;
		cntTotal++;
	}
	itFolder = mapFolders.find(pFolder->getUUID());
	wi.cntWorn = itFolder->second.cntWorn;
	wi.cntTotal = itFolder->second.cntTotal;
	mapFolders.erase(itFolder);
	for (itFolder = mapFolders.begin(); itFolder != mapFolders.end(); ++itFolder)
	{
		rlv_wear_info& wiFolder = itFolder->second;
		wi.cntChildWorn += wiFolder.cntWorn + wiFolder.cntChildWorn;
		wi.cntChildTotal += wiFolder.cntTotal + wiFolder.cntChildTotal;
		strReply += llformat(",%s|%d%d", gInventory.getCategory(itFolder->first)->getName().c_str(),
		 (0 == wiFolder.cntTotal) ? 0 : (0 == wiFolder.cntWorn) ? 1 : (wiFolder.cntWorn != wiFolder.cntTotal) ? 2 : 3,
		 (0 == wiFolder.cntChildTotal) ? 0 : (0 == wiFolder.cntChildWorn) ? 1 : (wiFolder.cntChildWorn != wiFolder.cntChildTotal) ? 2 : 3
		);
	}
	strReply = llformat("|%d%d", (0 == wi.cntTotal) ? 0 : (0 == wi.cntWorn) ? 1 : (wi.cntWorn != wi.cntTotal) ? 2 : 3,
		(0 == wi.cntChildTotal) ? 0 : (0 == wi.cntChildWorn) ? 1 : (wi.cntChildWorn != wi.cntChildTotal) ? 2: 3) + strReply;
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onGetOutfit(const RlvCommand& rlvCmd, std::string& strReply) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	RLV_ASSERT(RLV_BHVR_GETOUTFIT == rlvCmd.getBehaviourType());
	LLWearableType::EType wtType = LLWearableType::WT_INVALID;
	if ( (rlvCmd.hasOption()) && ((wtType = LLWearableType::typeNameToType(rlvCmd.getOption())) == LLWearableType::WT_INVALID) )
		return RLV_RET_FAILED_OPTION;
	const LLWearableType::EType wtRlvTypes[] =
		{
			LLWearableType::WT_GLOVES, LLWearableType::WT_JACKET, LLWearableType::WT_PANTS, LLWearableType::WT_SHIRT,
			LLWearableType::WT_SHOES, LLWearableType::WT_SKIRT, LLWearableType::WT_SOCKS, LLWearableType::WT_UNDERPANTS,
			LLWearableType::WT_UNDERSHIRT, LLWearableType::WT_SKIN, LLWearableType::WT_EYES, LLWearableType::WT_HAIR,
			LLWearableType::WT_SHAPE, LLWearableType::WT_ALPHA, LLWearableType::WT_TATTOO, LLWearableType::WT_PHYSICS
		};
	for (int idxType = 0, cntType = sizeof(wtRlvTypes) / sizeof(LLWearableType::EType); idxType < cntType; idxType++)
	{
		if ( (LLWearableType::WT_INVALID == wtType) || (wtRlvTypes[idxType] == wtType) )
		{
			bool fWorn = (gAgentWearables.getWearableCount(wtRlvTypes[idxType]) > 0) &&
				( (!RlvSettings::getHideLockedLayers()) ||
				  (LLAssetType::AT_BODYPART == LLWearableType::getAssetType(wtRlvTypes[idxType])) ||
				  (RlvForceWear::isForceRemovable(wtRlvTypes[idxType], true, rlvCmd.getObjectID())) );
			strReply.push_back( (fWorn) ? '1' : '0' );
		}
	}
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onGetOutfitNames(const RlvCommand& rlvCmd, std::string& strReply) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	RLV_ASSERT( (RLV_BHVR_GETOUTFITNAMES == rlvCmd.getBehaviourType()) || (RLV_BHVR_GETADDOUTFITNAMES == rlvCmd.getBehaviourType()) ||
		        (RLV_BHVR_GETREMOUTFITNAMES == rlvCmd.getBehaviourType()) );
	if (rlvCmd.hasOption())
		return RLV_RET_FAILED_OPTION;
	for (int idxType = 0; idxType < LLWearableType::WT_COUNT; idxType++)
	{
		bool fAdd = false; LLWearableType::EType wtType = (LLWearableType::EType)idxType;
		switch (rlvCmd.getBehaviourType())
		{
			case RLV_BHVR_GETOUTFITNAMES:
				fAdd = (gAgentWearables.getWearableCount(wtType) > 0);
				break;
			case RLV_BHVR_GETADDOUTFITNAMES:
				fAdd = (gRlvWearableLocks.canWear(wtType) & RLV_WEAR);
				break;
			case RLV_BHVR_GETREMOUTFITNAMES:
				fAdd = RlvForceWear::isForceRemovable(wtType);
				break;
			default:
				break;
		}
		if (fAdd)
		{
			if (!strReply.empty())
				strReply.push_back(',');
			strReply.append(LLWearableType::getTypeName(wtType));
		}
	}
	return RLV_RET_SUCCESS;
}
ERlvCmdRet RlvHandler::onGetPath(const RlvCommand& rlvCmd, std::string& strReply) const
{
	RLV_ASSERT(RLV_TYPE_REPLY == rlvCmd.getParamType());
	RLV_ASSERT( (RLV_BHVR_GETPATH == rlvCmd.getBehaviourType()) || (RLV_BHVR_GETPATHNEW == rlvCmd.getBehaviourType()) );
	RlvCommandOptionGetPath rlvGetPathOption(rlvCmd);
	if (!rlvGetPathOption.isValid())
		return RLV_RET_FAILED_OPTION;
	LLInventoryModel::cat_array_t folders;
	if (RlvInventory::instance().getPath(rlvGetPathOption.getItemIDs(), folders))
	{
		if (RLV_BHVR_GETPATH == rlvCmd.getBehaviourType())
		{
			strReply = RlvInventory::instance().getSharedPath(folders.at(0));
		}
		else if (RLV_BHVR_GETPATHNEW == rlvCmd.getBehaviourType())
		{
			for (S32 idxFolder = 0, cntFolder = folders.size(); idxFolder < cntFolder; idxFolder++)
			{
				if (!strReply.empty())
					strReply.push_back(',');
				strReply += RlvInventory::instance().getSharedPath(folders.at(idxFolder));
			}
		}
	}
	return RLV_RET_SUCCESS;
}
