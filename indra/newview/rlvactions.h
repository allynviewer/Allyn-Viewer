/** 
 *
 * Copyright (c) 2009-2016, Kitty Barnett
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
#ifndef RLV_ACTIONS_H
#define RLV_ACTIONS_H
#include "llchat.h"
#include "rlvdefines.h"
class RlvActions
{
public:
	static bool canReceiveIM(const LLUUID& idSender);
	static bool canSendChannel(int nChannel);
	static bool canSendIM(const LLUUID& idRecipient);
	static bool canStartIM(const LLUUID& idRecipient);
	enum EShowNamesContext { SNC_TELEPORTOFFER = 0, SNC_TELEPORTREQUEST, SNC_COUNT };
	static bool canShowName(EShowNamesContext eContext) { return (eContext < SNC_COUNT) ? !s_BlockNamesContexts[eContext] : false; }
	static void setShowName(EShowNamesContext eContext, bool fShowName) { if ( (eContext < SNC_COUNT) && (isRlvEnabled()) ) { s_BlockNamesContexts[eContext] = !fShowName; } }
	static EChatType checkChatVolume(EChatType chatType);
protected:
	static bool s_BlockNamesContexts[SNC_COUNT];
public:
	static bool canAcceptTpOffer(const LLUUID& idSender);
	static bool autoAcceptTeleportOffer(const LLUUID& idSender);
	static bool canAcceptTpRequest(const LLUUID& idSender);
	static bool autoAcceptTeleportRequest(const LLUUID& idRequester);
public:
	static bool canStand();
	static bool canShowLocation();
public:
	static bool hasBehaviour(ERlvBehaviour eBhvr);
	static bool hasOpenP2PSession(const LLUUID& idAgent);
	static bool hasOpenGroupSession(const LLUUID& idGroup);
	static bool isRlvEnabled();
};
#endif
