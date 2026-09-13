/** 
 * @file llattachmentsmgr.cpp
 * @brief Manager for initiating attachments changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llattachmentsmgr.h"
#include "llvoavatarself.h"
#include "llviewerjointattachment.h"
#include "llagent.h"
#include "llappearancemgr.h"
#include "llinventorymodel.h"
#include "lltooldraganddrop.h"
#include "llviewerinventory.h"
#include "llviewerregion.h"
#include "message.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
#include <boost/algorithm/string/predicate.hpp>
const F32 COF_LINK_BATCH_TIME = 5.0F;
const F32 MAX_ATTACHMENT_REQUEST_LIFETIME = 30.0F;
const F32 MIN_RETRY_REQUEST_TIME = 5.0F;
const F32 MAX_BAD_COF_TIME = 30.0F;
class LLRegisterAttachmentCallback : public LLRequestServerAppearanceUpdateOnDestroy
{
public:
	LLRegisterAttachmentCallback()
		: LLRequestServerAppearanceUpdateOnDestroy()
	{
	}
	~LLRegisterAttachmentCallback()
	{
	}
	void fire(const LLUUID& idItem)
	{
		LLAttachmentsMgr::instance().onRegisterAttachmentComplete(idItem);
	}
};
LLAttachmentsMgr::LLAttachmentsMgr():
    mAttachmentRequests("attach",MIN_RETRY_REQUEST_TIME),
    mDetachRequests("detach",MIN_RETRY_REQUEST_TIME)
{
}
LLAttachmentsMgr::~LLAttachmentsMgr()
{
}
void LLAttachmentsMgr::addAttachmentRequest(const LLUUID& item_id,
                                            const U8 attachment_pt,
									 const BOOL add, const BOOL fRlvForce )
{
	LLViewerInventoryItem *item = gInventory.getItem(item_id);
    if (mAttachmentRequests.wasRequestedRecently(item_id))
    {
        LL_DEBUGS("Avatar") << "ATT not adding attachment to mPendingAttachments, recent request is already pending: "
                            << (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
        return;
    }
	LL_DEBUGS("Avatar") << "ATT adding attachment to mPendingAttachments "
						<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
	AttachmentsInfo attachment;
	attachment.mItemID = item_id;
	attachment.mAttachmentPt = attachment_pt;
	attachment.mAdd = add;
	if ( (rlv_handler_t::isEnabled()) && (!fRlvForce) && (gRlvAttachmentLocks.hasLockedAttachmentPoint(RLV_LOCK_ANY)) && (gAgentWearables.areInitialAttachmentsRequested()) )
	{
		const LLInventoryItem* pItem = gInventory.getItem(item_id);
		if (!pItem)
			return;
		LLViewerJointAttachment* pAttachPt = NULL;
		ERlvWearMask eWearMask = gRlvAttachmentLocks.canAttach(pItem, &pAttachPt);
		if ( ((add) && ((RLV_WEAR_ADD & eWearMask) == 0)) || ((!add) && ((RLV_WEAR_REPLACE & eWearMask) == 0)) )
			return;
		if ( (0 == attachment_pt) && (NULL != pAttachPt) )
			attachment.mAttachmentPt = RlvAttachPtLookup::getAttachPointIndex(pAttachPt);
		RlvAttachmentLockWatchdog::instance().onWearAttachment(pItem, (add) ? RLV_WEAR_ADD : RLV_WEAR_REPLACE);
		attachment.mAdd = true;
	}
	mPendingAttachments.push_back(attachment);
    mAttachmentRequests.addTime(item_id);
}
void LLAttachmentsMgr::onAttachmentRequested(const LLUUID& item_id)
{
	if (item_id.isNull())
		return;
	LLViewerInventoryItem *item = gInventory.getItem(item_id);
	LL_DEBUGS("Avatar") << "ATT attachment was requested "
						<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
    mAttachmentRequests.addTime(item_id);
}
void LLAttachmentsMgr::onIdle(void *)
{
	LLAttachmentsMgr::instance().onIdle();
}
void LLAttachmentsMgr::onIdle()
{
	if( !gAgent.getRegion() )
	{
		return;
	}
    if (LLApp::isExiting())
    {
		return;
    }
	requestPendingAttachments();
    linkRecentlyArrivedAttachments();
    expireOldAttachmentRequests();
    expireOldDetachRequests();
    spamStatusInfo();
}
void LLAttachmentsMgr::requestPendingAttachments()
{
	if (mPendingAttachments.size())
	{
		requestAttachments(mPendingAttachments);
	}
}
void LLAttachmentsMgr::requestAttachments(attachments_vec_t& attachment_requests)
{
	if( !gAgent.getRegion() )
	{
		return;
	}
    const S32 max_objects_per_request = 5;
	S32 obj_count = llmin((S32)attachment_requests.size(),max_objects_per_request);
	if (obj_count == 0)
	{
		return;
	}
	const S32 MAX_PACKETS_TO_SEND = 10;
	const S32 OBJECTS_PER_PACKET = 4;
	const S32 MAX_OBJECTS_TO_SEND = MAX_PACKETS_TO_SEND * OBJECTS_PER_PACKET;
	if( obj_count > MAX_OBJECTS_TO_SEND )
	{
        LL_WARNS() << "ATT Too many attachments requested: " << obj_count
                   << " exceeds limit of " << MAX_OBJECTS_TO_SEND << LL_ENDL;
		obj_count = MAX_OBJECTS_TO_SEND;
	}
	LL_DEBUGS("Avatar") << "ATT [RezMultipleAttachmentsFromInv] attaching multiple from attachment_requests,"
		" total obj_count " << obj_count << LL_ENDL;
	LLUUID compound_msg_id;
	compound_msg_id.generate();
	LLMessageSystem* msg = gMessageSystem;
    llassert(obj_count <= attachment_requests.size());
    for (S32 i=0; i<obj_count; i++)
	{
		if( 0 == (i % OBJECTS_PER_PACKET) )
		{
			msg->newMessageFast(_PREHASH_RezMultipleAttachmentsFromInv);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_HeaderData);
			msg->addUUIDFast(_PREHASH_CompoundMsgID, compound_msg_id );
			msg->addU8Fast(_PREHASH_TotalObjects, obj_count );
			msg->addBOOLFast(_PREHASH_FirstDetachAll, false );
		}
		const AttachmentsInfo& attachment = attachment_requests.front();
		LLViewerInventoryItem* item = gInventory.getItem(attachment.mItemID);
		if (item)
		{
            LL_DEBUGS("Avatar") << "ATT requesting from attachment_requests " << item->getName()
                                << " " << item->getLinkedUUID() << LL_ENDL;
			S32 attachment_pt = attachment.mAttachmentPt;
			if (attachment.mAdd)
				attachment_pt |= ATTACHMENT_ADD;
			msg->nextBlockFast(_PREHASH_ObjectData );
			msg->addUUIDFast(_PREHASH_ItemID, item->getLinkedUUID());
			msg->addUUIDFast(_PREHASH_OwnerID, item->getPermissions().getOwner());
			msg->addU8Fast(_PREHASH_AttachmentPt, attachment_pt);
			pack_permissions_slam(msg, item->getFlags(), item->getPermissions());
			msg->addStringFast(_PREHASH_Name, item->getName());
			msg->addStringFast(_PREHASH_Description, item->getDescription());
        }
        else
		{
			LL_WARNS("Avatar") << "ATT Attempted to add non-existent item ID:" << attachment.mItemID << LL_ENDL;
		}
		if( (i+1 == obj_count) || ((OBJECTS_PER_PACKET-1) == (i % OBJECTS_PER_PACKET)) )
		{
			msg->sendReliable( gAgent.getRegion()->getHost() );
		}
        attachment_requests.pop_front();
	}
}
void LLAttachmentsMgr::linkRecentlyArrivedAttachments()
{
    if (mRecentlyArrivedAttachments.size())
    {
		if (!LLAppearanceMgr::instance().getAttachmentInvLinkEnable())
		{
			return;
		}
        if (mAttachmentRequests.empty())
        {
            LL_DEBUGS("Avatar") << "ATT all pending attachments have arrived after "
                                << mCOFLinkBatchTimer.getElapsedTimeF32() << " seconds" << LL_ENDL;
        }
        else if (mCOFLinkBatchTimer.getElapsedTimeF32() > COF_LINK_BATCH_TIME)
        {
            LL_DEBUGS("Avatar") << "ATT " << mAttachmentRequests.size()
                                << " pending attachments have not arrived, but wait time exceeded" << LL_ENDL;
        }
        else
        {
            return;
        }
        LL_DEBUGS("Avatar") << "ATT checking COF linkability for " << mRecentlyArrivedAttachments.size()
                            << " recently arrived items" << LL_ENDL;
        uuid_vec_t ids_to_link;
        for (auto it = mRecentlyArrivedAttachments.begin();
             it != mRecentlyArrivedAttachments.end(); ++it)
        {
            if (isAgentAvatarValid() &&
                gAgentAvatarp->isWearingAttachment(*it) &&
                !LLAppearanceMgr::instance().isLinkedInCOF(*it))
            {
                LLUUID item_id = *it;
                LLViewerInventoryItem *item = gInventory.getItem(item_id);
                LL_DEBUGS("Avatar") << "ATT adding COF link for attachment "
                                    << (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
                ids_to_link.push_back(item_id);
            }
        }
        if (ids_to_link.size())
        {
			LLPointer<LLInventoryCallback> cb = new LLRegisterAttachmentCallback();
			for (const LLUUID& idAttach : ids_to_link)
			{
				if (std::find(mPendingAttachLinks.begin(), mPendingAttachLinks.end(), idAttach) == mPendingAttachLinks.end())
				{
					LLAppearanceMgr::instance().addCOFItemLink(idAttach, cb);
					mPendingAttachLinks.insert(idAttach);
				}
			}
        }
        mRecentlyArrivedAttachments.clear();
    }
}
bool LLAttachmentsMgr::getPendingAttachments(uuid_set_t& ids) const
{
	ids.clear();
	ids.insert(mRecentlyArrivedAttachments.begin(), mRecentlyArrivedAttachments.end());
	ids.insert(mPendingAttachLinks.begin(), mPendingAttachLinks.end());
	return !ids.empty();
}
void LLAttachmentsMgr::clearPendingAttachmentLink(const LLUUID& idItem)
{
	mPendingAttachLinks.erase(idItem);
}
void LLAttachmentsMgr::onRegisterAttachmentComplete(const LLUUID& idAttachLink)
{
	const LLViewerInventoryItem* pAttachLink = gInventory.getItem(idAttachLink);
	if (!pAttachLink)
		return;
	const LLUUID& idAttachBase = pAttachLink->getLinkedUUID();
	clearPendingAttachmentLink(idAttachBase);
	if ( (isAgentAvatarValid()) && (!gAgentAvatarp->isWearingAttachment(idAttachBase)) )
	{
		LLAppearanceMgr::instance().removeCOFItemLinks(idAttachBase, NULL, true);
	}
}
LLAttachmentsMgr::LLItemRequestTimes::LLItemRequestTimes(const std::string& op_name, F32 timeout):
    mOpName(op_name),
    mTimeout(timeout)
{
}
void LLAttachmentsMgr::LLItemRequestTimes::addTime(const LLUUID& inv_item_id)
{
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
    LL_DEBUGS("Avatar") << "ATT " << mOpName << " adding request time " << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
	LLTimer current_time;
	(*this)[inv_item_id] = current_time;
}
void LLAttachmentsMgr::LLItemRequestTimes::removeTime(const LLUUID& inv_item_id)
{
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
	S32 remove_count = (*this).erase(inv_item_id);
    if (remove_count)
    {
        LL_DEBUGS("Avatar") << "ATT " << mOpName << " removing request time "
                            << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
    }
}
BOOL LLAttachmentsMgr::LLItemRequestTimes::getTime(const LLUUID& inv_item_id, LLTimer& timer) const
{
	std::map<LLUUID,LLTimer>::const_iterator it = (*this).find(inv_item_id);
	if (it != (*this).end())
	{
        timer = it->second;
        return TRUE;
    }
    return FALSE;
}
BOOL LLAttachmentsMgr::LLItemRequestTimes::wasRequestedRecently(const LLUUID& inv_item_id) const
{
    LLTimer request_time;
    if (getTime(inv_item_id, request_time))
    {
		F32 request_time_elapsed = request_time.getElapsedTimeF32();
        return request_time_elapsed < mTimeout;
    }
    else
    {
        return FALSE;
    }
}
void LLAttachmentsMgr::expireOldAttachmentRequests()
{
	for (std::map<LLUUID,LLTimer>::iterator it = mAttachmentRequests.begin();
         it != mAttachmentRequests.end(); )
    {
        std::map<LLUUID,LLTimer>::iterator curr_it = it;
        ++it;
        if (curr_it->second.getElapsedTimeF32() > MAX_ATTACHMENT_REQUEST_LIFETIME)
        {
            LLInventoryItem *item = gInventory.getItem(curr_it->first);
            LL_WARNS("Avatar") << "ATT expiring request for attachment "
                                << (item ? item->getName() : "UNKNOWN") << " item_id " << curr_it->first
                                << " after " << MAX_ATTACHMENT_REQUEST_LIFETIME << " seconds" << LL_ENDL;
            mAttachmentRequests.erase(curr_it);
        }
    }
}
void LLAttachmentsMgr::expireOldDetachRequests()
{
	for (std::map<LLUUID,LLTimer>::iterator it = mDetachRequests.begin();
         it != mDetachRequests.end(); )
    {
        std::map<LLUUID,LLTimer>::iterator curr_it = it;
        ++it;
        if (curr_it->second.getElapsedTimeF32() > MAX_ATTACHMENT_REQUEST_LIFETIME)
        {
            LLInventoryItem *item = gInventory.getItem(curr_it->first);
            LL_WARNS("Avatar") << "ATT expiring request for detach "
                                << (item ? item->getName() : "UNKNOWN") << " item_id " << curr_it->first
                                << " after " << MAX_ATTACHMENT_REQUEST_LIFETIME << " seconds" << LL_ENDL;
            mDetachRequests.erase(curr_it);
        }
    }
}
void LLAttachmentsMgr::refreshAttachments()
{
	if (!isAgentAvatarValid())
	{
		LL_INFOS("HUDTeleport") << "refreshAttachments skipped: avatar invalid" << LL_ENDL;
		return;
	}

	dump_hud_teleport_state("refreshAttachments_begin");
	S32 queued = 0;
	for (LLVOAvatar::attachment_map_t::const_iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
		 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (!attachment)
		{
			continue;
		}
		for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end(); ++attachment_iter)
		{
			const LLViewerObject* pAttachObj = *attachment_iter;
			if (!pAttachObj)
			{
				continue;
			}
			const LLUUID& idItem = pAttachObj->getAttachmentItemID();
			if (mAttachmentRequests.wasRequestedRecently(idItem) || pAttachObj->isTempAttachment())
			{
				if (pAttachObj->isHUDAttachment())
				{
					log_hud_event("refreshAttachments_skip_recent_or_temp", pAttachObj);
				}
				continue;
			}

			AttachmentsInfo info;
			info.mItemID = idItem;
			info.mAttachmentPt = iter->first;
			info.mAdd = TRUE;
			mPendingAttachments.push_back(info);
			mAttachmentRequests.addTime(idItem);
			++queued;
			if (pAttachObj->isHUDAttachment())
			{
				log_hud_event("refreshAttachments_queue", pAttachObj);
			}
		}
	}
	LL_INFOS("HUDTeleport") << "refreshAttachments queued=" << queued << LL_ENDL;
}

void LLAttachmentsMgr::onAttachmentArrived(const LLUUID& inv_item_id)
{
    LLTimer timer;
    bool expected = mAttachmentRequests.getTime(inv_item_id, timer);
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
    if (!expected)
    {
        LL_WARNS() << "ATT Attachment was unexpected or arrived after " << MAX_ATTACHMENT_REQUEST_LIFETIME << " seconds: "
                   << (item ? item->getName() : "UNKNOWN") << " id " << inv_item_id << LL_ENDL;
    }
    mAttachmentRequests.removeTime(inv_item_id);
    if (expected && mAttachmentRequests.empty())
    {
        LL_DEBUGS("Avatar") << "ATT all active attachment requests have completed" << LL_ENDL;
    }
    if (mRecentlyArrivedAttachments.empty())
    {
        mCOFLinkBatchTimer.reset();
    }
    mRecentlyArrivedAttachments.insert(inv_item_id);
    static const LLCachedControl<bool> detach_bridge("SGDetachBridge");
    if (detach_bridge && item && boost::algorithm::contains(item->getName(), " Bridge v"))
    {
        LL_INFOS() << "Bridge detected! detaching" << LL_ENDL;
        LLVOAvatarSelf::detachAttachmentIntoInventory(item->getUUID());
    }
}
void LLAttachmentsMgr::onDetachRequested(const LLUUID& inv_item_id)
{
    mDetachRequests.addTime(inv_item_id);
}
void LLAttachmentsMgr::onDetachCompleted(const LLUUID& inv_item_id)
{
	clearPendingAttachmentLink(inv_item_id);
    LLTimer timer;
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
    if (mDetachRequests.getTime(inv_item_id, timer))
    {
        LL_DEBUGS("Avatar") << "ATT detach completed after " << timer.getElapsedTimeF32()
                            << " seconds for " << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
        mDetachRequests.removeTime(inv_item_id);
        if (mDetachRequests.empty())
        {
            LL_DEBUGS("Avatar") << "ATT all detach requests have completed" << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS() << "ATT unexpected detach for "
                   << (item ? item->getName() : "UNKNOWN") << " id " << inv_item_id << LL_ENDL;
    }
}
void LLAttachmentsMgr::spamStatusInfo()
{
#if 0
    static LLTimer spam_timer;
    const F32 spam_frequency = 100.0F;
    if (spam_timer.getElapsedTimeF32() > spam_frequency)
    {
        spam_timer.reset();
        LLInventoryModel::cat_array_t cat_array;
        LLInventoryModel::item_array_t item_array;
        gInventory.collectDescendents(LLAppearanceMgr::instance().getCOF(),
                                      cat_array,item_array,LLInventoryModel::EXCLUDE_TRASH);
        for (S32 i=0; i<item_array.size(); i++)
        {
            const LLViewerInventoryItem* inv_item = item_array.at(i).get();
            if (inv_item->getType() == LLAssetType::AT_OBJECT)
            {
                LL_DEBUGS("Avatar") << "item_id: " << inv_item->getUUID()
                                    << " linked_item_id: " << inv_item->getLinkedUUID()
                                    << " name: " << inv_item->getName()
                                    << " parent: " << inv_item->getParentUUID()
                                    << LL_ENDL;
            }
        }
    }
#endif
}
