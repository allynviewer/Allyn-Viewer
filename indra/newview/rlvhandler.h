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
#ifndef RLV_HANDLER_H
#define RLV_HANDLER_H
#include <stack>
#include "rlvcommon.h"
#include "rlvhelper.h"
class RlvHandler : public LLOldEvents::LLSimpleListener
{
public:
	RlvHandler();
	~RlvHandler();
public:
	bool hasBehaviour(ERlvBehaviour eBhvr) const { return (eBhvr < RLV_BHVR_COUNT) ? (0 != m_Behaviours[eBhvr]) : false; }
	bool hasBehaviour(ERlvBehaviour eBhvr, const std::string& strOption) const;
	bool hasBehaviourExcept(ERlvBehaviour eBhvr, const LLUUID& idObj) const;
	bool hasBehaviourExcept(ERlvBehaviour eBhvr, const std::string& strOption, const LLUUID& idObj) const;
	bool hasBehaviourRoot(const LLUUID& idObjRoot, ERlvBehaviour eBhvr, const std::string& strOption = LLStringUtil::null) const;
	void addException(const LLUUID& idObj, ERlvBehaviour eBhvr, const RlvExceptionOption& varOption);
	void removeException(const LLUUID& idObj, ERlvBehaviour eBhvr, const RlvExceptionOption& varOption);
	bool hasException(ERlvBehaviour eBhvr) const;
	bool isException(ERlvBehaviour eBhvr, const RlvExceptionOption& varOption, ERlvExceptionCheck typeCheck = RLV_CHECK_DEFAULT) const;
	bool isPermissive(ERlvBehaviour eBhvr) const;
	#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	bool canTakeOffComposite(const LLInventoryCategory* pFolder) const;
	bool canWearComposite(const LLInventoryCategory* pFolder) const;
	bool getCompositeInfo(const LLInventoryCategory* pFolder, std::string* pstrName) const;
	bool getCompositeInfo(const LLUUID& idItem, std::string* pstrName, LLViewerInventoryCategory** ppFolder) const;
	bool isCompositeFolder(const LLInventoryCategory* pFolder) const { return getCompositeInfo(pFolder, NULL); }
	bool isCompositeDescendent(const LLUUID& idItem) const { return getCompositeInfo(idItem, NULL, NULL); }
	bool isHiddenCompositeItem(const LLUUID& idItem, const std::string& strItemType) const;
	#endif
public:
	const LLUUID&     getAgentGroup() const			{ return m_idAgentGroup; }
	bool              getCanCancelTp() const		{ return m_fCanCancelTp; }
	void              setCanCancelTp(bool fAllow)	{ m_fCanCancelTp = fAllow; }
	const LLVector3d& getSitSource() const						{ return m_posSitSource; }
	void              setSitSource(const LLVector3d& posSource)	{ m_posSitSource = posSource; }
	bool canEdit(const LLViewerObject* pObj) const;
	bool canShowHoverText(const LLViewerObject* pObj) const;
	bool canSit(LLViewerObject* pObj, const LLVector3& posOffset = LLVector3::zero) const;
	bool canTouch(const LLViewerObject* pObj, const LLVector3& posOffset = LLVector3::zero) const;
	bool filterChat(std::string& strUTF8Text, bool fFilterEmote) const;
	bool redirectChatOrEmote(const std::string& strUTF8Test) const;
	void updatePole(const ERlvBehaviour& bhvr, bool max);
	F32 camPole(const ERlvBehaviour& bhvr)			{ return m_Poles[bhvr]; }
	LLColor3 camDrawColor() const;
	ERlvCmdRet processCommand(const LLUUID& idObj, const std::string& strCommand, bool fFromObj);
	void       processRetainedCommands(ERlvBehaviour eBhvrFilter = RLV_BHVR_UNKNOWN, ERlvParamType eTypeFilter = RLV_TYPE_UNKNOWN);
	const RlvCommand* getCurrentCommand() const { return (!m_CurCommandStack.empty()) ? m_CurCommandStack.top() : NULL; }
	const LLUUID&     getCurrentObject() const	{ return (!m_CurObjectStack.empty()) ? m_CurObjectStack.top() : LLUUID::null; }
	static BOOL canDisable();
	static BOOL isEnabled()	{ return m_fEnabled; }
	static BOOL setEnabled(BOOL fEnable);
protected:
	void clearState();
public:
	typedef boost::signals2::signal<void (ERlvBehaviour, ERlvParamType)> rlv_behaviour_signal_t;
	boost::signals2::connection setBehaviourCallback(const rlv_behaviour_signal_t::slot_type& cb )		 { return m_OnBehaviour.connect(cb); }
	boost::signals2::connection setBehaviourToggleCallback(const rlv_behaviour_signal_t::slot_type& cb ) { return m_OnBehaviourToggle.connect(cb); }
	typedef boost::signals2::signal<void (const RlvCommand&, ERlvCmdRet, bool)> rlv_command_signal_t;
	boost::signals2::connection setCommandCallback(const rlv_command_signal_t::slot_type& cb )			 { return m_OnCommand.connect(cb); }
	void addCommandHandler(RlvCommandHandler* pHandler);
	void removeCommandHandler(RlvCommandHandler* pHandler);
protected:
	void clearCommandHandlers();
	bool notifyCommandHandlers(rlvCommandHandler f, const RlvCommand& rlvCmd, ERlvCmdRet& eRet, bool fNotifyAll) const;
public:
	bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& sdUserdata);
	void onAttach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt);
	void onDetach(const LLViewerObject* pAttachObj, const LLViewerJointAttachment* pAttachPt);
	bool onGC();
	void onLoginComplete();
	void onSitOrStand(bool fSitting);
	void onTeleportFailed();
	void onTeleportFinished(const LLVector3d& posArrival);
	static void onIdleStartup(void* pParam);
protected:
	ERlvCmdRet processCommand(const RlvCommand& rlvCmd, bool fFromObj);
	ERlvCmdRet processClearCommand(const RlvCommand& rlvCmd);
	ERlvCmdRet processAddRemCommand(const RlvCommand& rlvCmd);
	ERlvCmdRet onAddRemAttach(const RlvCommand& rlvCmd, bool& fRefCount);
	ERlvCmdRet onAddRemDetach(const RlvCommand& rlvCmd, bool& fRefCount);
	ERlvCmdRet onAddRemFolderLock(const RlvCommand& rlvCmd, bool& fRefCount);
	ERlvCmdRet onAddRemFolderLockException(const RlvCommand& rlvCmd, bool& fRefCount);
	ERlvCmdRet processForceCommand(const RlvCommand& rlvCmd) const;
	ERlvCmdRet onForceRemAttach(const RlvCommand& rlvCmd) const;
	ERlvCmdRet onForceRemOutfit(const RlvCommand& rlvCmd) const;
	ERlvCmdRet onForceGroup(const RlvCommand& rlvCmd) const;
	ERlvCmdRet onForceSit(const RlvCommand& rlvCmd) const;
	ERlvCmdRet onForceWear(const LLViewerInventoryCategory* pFolder, ERlvBehaviour eBhvr) const;
	void       onForceWearCallback(const uuid_vec_t& idItems, ERlvBehaviour eBhvr) const;
	ERlvCmdRet processReplyCommand(const RlvCommand& rlvCmd) const;
	ERlvCmdRet onFindFolder(const RlvCommand& rlvCmd, std::string& strReply) const;
	ERlvCmdRet onGetAttach(const RlvCommand& rlvCmd, std::string& strReply) const;
	ERlvCmdRet onGetAttachNames(const RlvCommand& rlvCmd, std::string& strReply) const;
	ERlvCmdRet onGetInv(const RlvCommand& rlvCmd, std::string& strReply) const;
	ERlvCmdRet onGetInvWorn(const RlvCommand& rlvCmd, std::string& strReply) const;
	ERlvCmdRet onGetOutfit(const RlvCommand& rlvCmd, std::string& strReply) const;
	ERlvCmdRet onGetOutfitNames(const RlvCommand& rlvCmd, std::string& strReply) const;
	ERlvCmdRet onGetPath(const RlvCommand& rlvCmd, std::string& strReply) const;
public:
	typedef std::map<LLUUID, RlvObject> rlv_object_map_t;
	typedef std::multimap<ERlvBehaviour, RlvException> rlv_exception_map_t;
protected:
	rlv_object_map_t      m_Objects;
	rlv_exception_map_t   m_Exceptions;
	std::map<ERlvBehaviour, F32> m_Poles;
	S16                   m_Behaviours[RLV_BHVR_COUNT];
	rlv_command_list_t    m_Retained;
	RlvGCTimer*           m_pGCTimer;
	std::stack<const RlvCommand*> m_CurCommandStack;
	std::stack<LLUUID>    m_CurObjectStack;
	rlv_behaviour_signal_t m_OnBehaviour;
	rlv_behaviour_signal_t m_OnBehaviourToggle;
	rlv_command_signal_t   m_OnCommand;
	mutable std::list<RlvCommandHandler*> m_CommandHandlers;
	static BOOL			  m_fEnabled;
	bool				m_fCanCancelTp;
	mutable LLVector3d	m_posSitSource;
	mutable LLUUID		m_idAgentGroup;
	friend class RlvSharedRootFetcher;
	friend class RlvGCTimer;
public:
	const rlv_object_map_t*    getObjectMap() const		{ return &m_Objects; }
};
typedef RlvHandler rlv_handler_t;
extern rlv_handler_t gRlvHandler;
inline bool RlvHandler::canEdit(const LLViewerObject* pObj) const
{
	return
		(pObj) &&
		((!hasBehaviour(RLV_BHVR_EDIT)) || (isException(RLV_BHVR_EDIT, pObj->getRootEdit()->getID()))) &&
		((!hasBehaviour(RLV_BHVR_EDITOBJ)) || (!isException(RLV_BHVR_EDITOBJ, pObj->getRootEdit()->getID())));
}
inline bool RlvHandler::canShowHoverText(const LLViewerObject *pObj) const
{
	return ( (!pObj) || (LL_PCODE_VOLUME != pObj->getPCode()) ||
		    !( (hasBehaviour(RLV_BHVR_SHOWHOVERTEXTALL)) ||
			   ( (hasBehaviour(RLV_BHVR_SHOWHOVERTEXTWORLD)) && (!pObj->isHUDAttachment()) ) ||
			   ( (hasBehaviour(RLV_BHVR_SHOWHOVERTEXTHUD)) && (pObj->isHUDAttachment()) ) ||
			   (isException(RLV_BHVR_SHOWHOVERTEXT, pObj->getID(), RLV_CHECK_PERMISSIVE)) ) );
}
inline bool RlvHandler::hasBehaviour(ERlvBehaviour eBhvr, const std::string& strOption) const
{
	return hasBehaviourExcept(eBhvr, strOption, LLUUID::null);
}
inline bool RlvHandler::hasBehaviourExcept(ERlvBehaviour eBhvr, const LLUUID& idObj) const
{
	return hasBehaviourExcept(eBhvr, LLStringUtil::null, idObj);
}
#endif
