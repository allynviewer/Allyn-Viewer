/** 
 * @file llgesturemgr.cpp
 * @brief Manager for playing gestures on the viewer
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
#include "llgesturemgr.h"
#include <functional>
#include <algorithm>
#include "llanimationstates.h"
#include "llaudioengine.h"
#include "lldatapacker.h"
#include "llinventory.h"
#include "llkeyframemotion.h"
#include "llmultigesture.h"
#include "llnotificationsutil.h"
#include "llstl.h"
#include "llstring.h"
#include "llvfile.h"
#include "message.h"
#include "llagent.h"
#include "llchatbar.h"
#include "lldelayedgestureerror.h"
#include "llinventorymodel.h"
#include "llviewermessage.h"
#include "llvoavatarself.h"
#include "llviewerstats.h"
#include "llappearancemgr.h"
#include "chatbar_as_cmdline.h"
#if SHY_MOD
#include "shcommandhandler.h"
#endif
void fake_local_chat(std::string msg);
const F32 MAX_WAIT_ANIM_SECS = 30.f;
LLGestureMgr::LLGestureMgr()
:	mValid(false),
	mPlaying(),
	mActive(),
	mLoadingCount(0)
{
	gInventory.addObserver(this);
}
LLGestureMgr::~LLGestureMgr()
{
	item_map_t::iterator it;
	for (it = mActive.begin(); it != mActive.end(); ++it)
	{
		LLMultiGesture* gesture = (*it).second;
		delete gesture;
		gesture = nullptr;
	}
	gInventory.removeObserver(this);
}
void LLGestureMgr::init()
{
}
void LLGestureMgr::changed(U32 mask)
{
	LLInventoryFetchItemsObserver::changed(mask);
	if (mask & LLInventoryObserver::GESTURE)
	{
		if (mask & LLInventoryObserver::LABEL)
		{
			for(item_map_t::iterator it = mActive.begin(); it != mActive.end(); ++it)
			{
				if(it->second)
				{
					LLViewerInventoryItem* item = gInventory.getItem(it->first);
					if(item)
					{
						it->second->mName = item->getName();
					}
				}
			}
			notifyObservers();
		}
		else if(mask & LLInventoryObserver::ADD ||
				mask & LLInventoryObserver::REMOVE ||
				mask & LLInventoryObserver::STRUCTURE)
		{
			notifyObservers();
		}
	}
}
void LLGestureMgr::activateGesture(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (!item) return;
	if (item->getType() != LLAssetType::AT_GESTURE)
		return;
	LLUUID asset_id = item->getAssetUUID();
	mLoadingCount = 1;
	mDeactivateSimilarNames.clear();
	const bool inform_server = true;
	const bool deactivate_similar = false;
	activateGestureWithAsset(item_id, asset_id, inform_server, deactivate_similar);
}
void LLGestureMgr::activateGestures(LLViewerInventoryItem::item_array_t& items)
{
	mDeactivateSimilarNames.clear();
	mLoadingCount = 0;
	LLMessageSystem* msg = gMessageSystem;
	bool start_message = true;
	for (const auto& item : items)
	{
		const auto& id = item->getUUID();
		if (isGestureActive(id))
		{
			continue;
		}
		activateGesture(id);
		++mLoadingCount;
		const auto& asset_id = item->getAssetUUID();
		const bool no_inform_server = false;
		const bool deactivate_similar = true;
		activateGestureWithAsset(id, asset_id,
								 no_inform_server,
								 deactivate_similar);
		if (start_message)
		{
			msg->newMessage("ActivateGestures");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->addU32("Flags", 0x0);
			start_message = false;
		}
		msg->nextBlock("Data");
		msg->addUUID("ItemID", id);
		msg->addUUID("AssetID", asset_id);
		msg->addU32("GestureFlags", 0x0);
		if (msg->getCurrentSendTotal() > MTUBYTES)
		{
			gAgent.sendReliableMessage();
			start_message = true;
		}
	}
	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
}
struct LLLoadInfo
{
	LLUUID mItemID;
	bool mInformServer;
	bool mDeactivateSimilar;
};
void LLGestureMgr::activateGestureWithAsset(const LLUUID& item_id,
												const LLUUID& asset_id,
												bool inform_server,
												bool deactivate_similar)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	if( !gAssetStorage )
	{
		LL_WARNS() << "LLGestureMgr::activateGestureWithAsset without valid gAssetStorage" << LL_ENDL;
		return;
	}
	if (isGestureActive(base_item_id))
	{
		LL_WARNS() << "Tried to loadGesture twice " << base_item_id << LL_ENDL;
		return;
	}
	mActive[base_item_id] = nullptr;
	if (asset_id.notNull())
	{
		LLLoadInfo* info = new LLLoadInfo;
		info->mItemID = base_item_id;
		info->mInformServer = inform_server;
		info->mDeactivateSimilar = deactivate_similar;
		const bool high_priority = true;
		gAssetStorage->getAssetData(asset_id,
									LLAssetType::AT_GESTURE,
									onLoadComplete,
									(void*)info,
									high_priority);
	}
	else
	{
		notifyObservers();
	}
}
void LLGestureMgr::deactivateGesture(const LLUUID& item_id)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	const auto& it = mActive.find(base_item_id);
	if (it == mActive.end())
	{
		LL_WARNS() << "deactivateGesture for inactive gesture " << base_item_id << LL_ENDL;
		return;
	}
	LLMultiGesture* gesture = (*it).second;
	if (gesture)
	{
		stopGesture(gesture);
		delete gesture;
		gesture = nullptr;
	}
	mActive.erase(it);
	gInventory.addChangedMask(LLInventoryObserver::LABEL, base_item_id);
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("DeactivateGestures");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgentID);
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addU32("Flags", 0x0);
	msg->nextBlock("Data");
	msg->addUUID("ItemID", base_item_id);
	msg->addU32("GestureFlags", 0x0);
	gAgent.sendReliableMessage();
	LLAppearanceMgr::instance().removeCOFItemLinks(base_item_id);
	notifyObservers();
}
void LLGestureMgr::deactivateSimilarGestures(const LLMultiGesture* in, const LLUUID& in_item_id)
{
	const LLUUID& base_in_item_id = gInventory.getLinkedItemID(in_item_id);
	LLMessageSystem* msg = gMessageSystem;
	bool start_message = true;
	for (auto it = mActive.begin(), end = mActive.end(); it != end; )
	{
		const LLUUID& item_id = (*it).first;
		LLMultiGesture* gest = (*it).second;
		if (!gest || item_id == base_in_item_id)
		{
			++it;
		}
		else if ((!gest->mTrigger.empty() && gest->mTrigger == in->mTrigger)
				 || (gest->mKey != KEY_NONE && gest->mKey == in->mKey && gest->mMask == in->mMask))
		{
			stopGesture(gest);
			delete gest;
			gest = nullptr;
			it = mActive.erase(it);
			end = mActive.end();
			gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
			if (start_message)
			{
				msg->newMessage("DeactivateGestures");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID", gAgentID);
				msg->addUUID("SessionID", gAgent.getSessionID());
				msg->addU32("Flags", 0x0);
				start_message = false;
			}
			msg->nextBlock("Data");
			msg->addUUID("ItemID", item_id);
			msg->addU32("GestureFlags", 0x0);
			if (msg->getCurrentSendTotal() > MTUBYTES)
			{
				gAgent.sendReliableMessage();
				start_message = true;
			}
			if (const auto& item = gInventory.getItem(item_id))
				mDeactivateSimilarNames += item->getName() + '\n';
		}
		else
		{
			++it;
		}
	}
	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
	notifyObservers();
}
bool LLGestureMgr::isGestureActive(const LLUUID& item_id) const
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	const auto& it = mActive.find(base_item_id);
	return (it != mActive.end());
}
bool LLGestureMgr::isGesturePlaying(const LLUUID& item_id) const
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	const auto& it = mActive.find(base_item_id);
	if (it == mActive.end()) return false;
	const LLMultiGesture* gesture = (*it).second;
	if (!gesture) return false;
	return gesture->mPlaying;
}
bool LLGestureMgr::isGesturePlaying(const LLMultiGesture* gesture) const
{
	return gesture && gesture->mPlaying;
}
void LLGestureMgr::replaceGesture(const LLUUID& item_id, LLMultiGesture* new_gesture, const LLUUID& asset_id)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	const auto& it = mActive.find(base_item_id);
	if (it == mActive.end())
	{
		LL_WARNS() << "replaceGesture for inactive gesture " << base_item_id << LL_ENDL;
		return;
	}
	LLMultiGesture* old_gesture = (*it).second;
	stopGesture(old_gesture);
	mActive.erase(base_item_id);
	mActive[base_item_id] = new_gesture;
	delete old_gesture;
	old_gesture = nullptr;
	if (asset_id.notNull())
	{
		mLoadingCount = 1;
		mDeactivateSimilarNames.clear();
		LLLoadInfo* info = new LLLoadInfo;
		info->mItemID = base_item_id;
		info->mInformServer = true;
		info->mDeactivateSimilar = false;
		const bool high_priority = true;
		gAssetStorage->getAssetData(asset_id,
									LLAssetType::AT_GESTURE,
									onLoadComplete,
									(void*)info,
									high_priority);
	}
	notifyObservers();
}
void LLGestureMgr::replaceGesture(const LLUUID& item_id, const LLUUID& new_asset_id)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	auto it = LLGestureMgr::instance().mActive.find(base_item_id);
	if (it == mActive.end())
	{
		LL_WARNS() << "replaceGesture for inactive gesture " << base_item_id << LL_ENDL;
		return;
	}
	LLMultiGesture* gesture = (*it).second;
	LLGestureMgr::instance().replaceGesture(base_item_id, gesture, new_asset_id);
}
void LLGestureMgr::playGesture(LLMultiGesture* gesture, bool local)
{
	if (!gesture) return;
	gesture->mCurrentStep = 0;
	gesture->mPlaying = true;
	gesture->mLocal = local;
	mPlaying.push_back(gesture);
	for (std::vector<LLGestureStep*>::iterator steps_it = gesture->mSteps.begin();
		 steps_it != gesture->mSteps.end();
		 ++steps_it)
	{
		LLGestureStep* step = *steps_it;
		switch(step->getType())
		{
		case STEP_ANIMATION:
			{
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				const LLUUID& anim_id = anim_step->mAnimAssetID;
				if (!(anim_id.isNull()
					  || anim_step->mFlags & ANIM_FLAG_STOP
					  || gAssetStorage->hasLocalAsset(anim_id, LLAssetType::AT_ANIMATION)))
				{
					const char* emote_name = gAnimLibrary.animStateToString(anim_id);
					if(emote_name && strstr(emote_name,"express_")==emote_name)
					{
						break;
					}
					mLoadingAssets.insert(anim_id);
					LLUUID* id = new LLUUID(gAgentID);
					gAssetStorage->getAssetData(anim_id,
									LLAssetType::AT_ANIMATION,
									onAssetLoadComplete,
									(void *)id,
									true);
				}
				break;
			}
		case STEP_SOUND:
			{
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				const LLUUID& sound_id = sound_step->mSoundAssetID;
				if (!(sound_id.isNull()
					  || gAssetStorage->hasLocalAsset(sound_id, LLAssetType::AT_SOUND)))
				{
					mLoadingAssets.insert(sound_id);
					gAssetStorage->getAssetData(sound_id,
									LLAssetType::AT_SOUND,
									onAssetLoadComplete,
									nullptr,
									true);
				}
				break;
			}
		case STEP_CHAT:
		case STEP_WAIT:
		case STEP_EOF:
			{
				break;
			}
		default:
			{
				LL_WARNS() << "Unknown gesture step type: " << step->getType() << LL_ENDL;
			}
		}
	}
	stepGesture(gesture);
	notifyObservers();
}
void LLGestureMgr::playGesture(const LLUUID& item_id, bool local)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	item_map_t::iterator it = mActive.find(base_item_id);
	if (it == mActive.end()) return;
	LLMultiGesture* gesture = (*it).second;
	if (!gesture) return;
	playGesture(gesture);
}
bool LLGestureMgr::triggerAndReviseString(const std::string &utf8str, std::string* revised_string)
{
	std::string tokenized = utf8str;
	bool found_gestures = false;
	bool first_token = true;
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(tokenized, sep);
	tokenizer::iterator token_iter;
	for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		const char* cur_token = token_iter->c_str();
		LLMultiGesture* gesture = nullptr;
		if( !found_gestures )
		{
			std::vector <LLMultiGesture *> matching;
			for (const auto& pair : mActive)
			{
				gesture = pair.second;
				if (!gesture) continue;
				if (LLStringUtil::compareInsensitive(gesture->mTrigger, cur_token) == 0)
				{
					matching.push_back(gesture);
				}
				gesture = nullptr;
			}
			if (matching.size() > 0)
			{
				{
					S32 random = ll_rand(matching.size());
					gesture = matching[random];
					playGesture(gesture);
					if (revised_string && !gesture->mReplaceText.empty())
					{
						if (!first_token) revised_string->push_back(' ');
						revised_string->append(LLStringUtil::compareInsensitive(cur_token, gesture->mReplaceText) == 0 ?
							cur_token : gesture->mReplaceText);
					}
					found_gestures = true;
				}
			}
		}
		if (!gesture && revised_string)
		{
			if (!first_token) revised_string->push_back(' ');
			revised_string->append(cur_token);
		}
		first_token = false;
		gesture = nullptr;
	}
	return found_gestures;
}
bool LLGestureMgr::triggerGesture(KEY key, MASK mask)
{
	if (mActive.empty()) return false;
	std::vector<LLMultiGesture*> matching;
	for (const auto& pair : mActive)
	{
		LLMultiGesture* gesture = pair.second;
		if (!gesture) continue;
		if (gesture->mKey == key
			&& gesture->mMask == mask)
		{
			matching.push_back(gesture);
		}
	}
	const auto& count = matching.size();
	if (count > 0)
	{
		playGesture(matching[ll_rand(count)]);
		return true;
	}
	return false;
}
S32 LLGestureMgr::getPlayingCount() const
{
	return mPlaying.size();
}
void LLGestureMgr::update()
{
	bool notify = false;
	for (auto it = mPlaying.begin(), end = mPlaying.end(); it != end;)
	{
		auto& gesture = *it;
		stepGesture(gesture);
		if (!gesture->mPlaying)
		{
			if (gesture->mDoneCallback)
			{
				gesture->mDoneCallback(gesture);
				gesture = nullptr;
			}
			it = mPlaying.erase(it);
			end = mPlaying.end();
			notify = true;
		}
		else ++it;
	}
	if (notify) notifyObservers();
}
void LLGestureMgr::stepGesture(LLMultiGesture* gesture)
{
	if (!gesture)
	{
		return;
	}
	if (!isAgentAvatarValid() || hasLoadingAssets(gesture)) return;
	const auto& signaled(gAgentAvatarp->mSignaledAnimations);
	const auto& signaled_end(signaled.end());
	for (auto gest_it = gesture->mPlayingAnimIDs.begin(), end = gesture->mPlayingAnimIDs.end(); gest_it != end;)
	{
		if (gesture->mLocal)
		{
			LLMotion* motion = gAgentAvatarp->findMotion(*gest_it);
			if (!motion || motion->isStopped())
				gest_it = gesture->mPlayingAnimIDs.erase(gest_it);
			else ++gest_it;
		}
		else if (signaled.find(*gest_it) != signaled_end)
		{
			++gest_it;
		}
		else
		{
			gest_it = gesture->mPlayingAnimIDs.erase(gest_it);
		}
	}
	for (auto gest_it = gesture->mRequestedAnimIDs.begin(), end = gesture->mRequestedAnimIDs.end(); gest_it != end;)
	{
		if (signaled.find(*gest_it) != signaled_end)
		{
			gesture->mPlayingAnimIDs.insert(*gest_it);
			gest_it = gesture->mRequestedAnimIDs.erase(gest_it);
		}
		else
		{
			++gest_it;
		}
	}
	bool waiting = false;
	while (!waiting && gesture->mPlaying)
	{
		LLGestureStep* step = nullptr;
		if (gesture->mCurrentStep < (S32)gesture->mSteps.size())
		{
			step = gesture->mSteps[gesture->mCurrentStep];
			llassert(step != nullptr);
		}
		else
		{
			gesture->mWaitingAtEnd = true;
		}
		if (gesture->mWaitingAtEnd)
		{
			if ((gesture->mRequestedAnimIDs.empty()
				&& gesture->mPlayingAnimIDs.empty()))
			{
				gesture->mWaitingAtEnd = false;
				gesture->mPlaying = false;
			}
			else
			{
				waiting = true;
			}
			continue;
		}
		if (gesture->mWaitingAnimations)
		{
			if ((gesture->mRequestedAnimIDs.empty()
				&& gesture->mPlayingAnimIDs.empty()))
			{
				gesture->mWaitingAnimations = false;
				++gesture->mCurrentStep;
			}
			else if (gesture->mWaitTimer.getElapsedTimeF32() > MAX_WAIT_ANIM_SECS)
			{
				LL_INFOS() << "Waited too long for animations to stop, continuing gesture."
					<< LL_ENDL;
				gesture->mWaitingAnimations = false;
				++gesture->mCurrentStep;
			}
			else
			{
				waiting = true;
			}
			continue;
		}
		if (gesture->mWaitingTimer)
		{
			LLGestureStepWait* wait_step = (LLGestureStepWait*)step;
			F32 elapsed = gesture->mWaitTimer.getElapsedTimeF32();
			if (elapsed > wait_step->mWaitSeconds)
			{
				gesture->mWaitingTimer = false;
				++gesture->mCurrentStep;
			}
			else
			{
				waiting = true;
			}
			continue;
		}
		runStep(gesture, step);
	}
}
void LLGestureMgr::runStep(LLMultiGesture* gesture, LLGestureStep* step)
{
	switch(step->getType())
	{
	case STEP_ANIMATION:
		{
			LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
			if (anim_step->mAnimAssetID.isNull())
			{
				++gesture->mCurrentStep;
			}
			if (anim_step->mFlags & ANIM_FLAG_STOP)
			{
				if (gesture->mLocal)
				{
					gAgentAvatarp->stopMotion(anim_step->mAnimAssetID);
				}
				else
				{
					gAgent.sendAnimationRequest(anim_step->mAnimAssetID, ANIM_REQUEST_STOP);
					auto set_it = gesture->mRequestedAnimIDs.find(anim_step->mAnimAssetID);
					if (set_it != gesture->mRequestedAnimIDs.end())
					{
						gesture->mRequestedAnimIDs.erase(set_it);
					}
				}
			}
			else
			{
				if (gesture->mLocal)
				{
					gAgentAvatarp->startMotion(anim_step->mAnimAssetID);
					gesture->mPlayingAnimIDs.insert(anim_step->mAnimAssetID);
				}
				else
				{
					gAgent.sendAnimationRequest(anim_step->mAnimAssetID, ANIM_REQUEST_START);
					gesture->mRequestedAnimIDs.insert(anim_step->mAnimAssetID);
				}
			}
			++gesture->mCurrentStep;
			break;
		}
	case STEP_SOUND:
		{
			LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
			const LLUUID& sound_id = sound_step->mSoundAssetID;
			constexpr F32 volume = 1.f;
			if (gesture->mLocal)
				gAudiop->triggerSound(sound_id, gAgentID, volume, LLAudioEngine::AUDIO_TYPE_UI, gAgent.getPositionGlobal());
			else
				send_sound_trigger(sound_id, volume);
			++gesture->mCurrentStep;
			break;
		}
	case STEP_CHAT:
		{
			LLGestureStepChat* chat_step = (LLGestureStepChat*)step;
			const std::string& chat_text = chat_step->mChatText;
			constexpr bool animate = false;
			if (cmd_line_chat(chat_text, CHAT_TYPE_NORMAL))
			{
#if SHY_MOD
				if(!SHCommandHandler::handleCommand(true, chat_text, gAgentID, gAgentAvatarp))
#endif
				gesture->mLocal ? fake_local_chat(chat_text) : gChatBar->sendChatFromViewer(chat_text, CHAT_TYPE_NORMAL, animate);
			}
			++gesture->mCurrentStep;
			break;
		}
	case STEP_WAIT:
		{
			LLGestureStepWait* wait_step = (LLGestureStepWait*)step;
			if (wait_step->mFlags & WAIT_FLAG_TIME)
			{
				gesture->mWaitingTimer = true;
				gesture->mWaitTimer.reset();
			}
			else if (wait_step->mFlags & WAIT_FLAG_ALL_ANIM)
			{
				gesture->mWaitingAnimations = true;
				gesture->mWaitTimer.reset();
			}
			else
			{
				++gesture->mCurrentStep;
			}
			break;
		}
	default:
		{
			break;
		}
	}
}
void LLGestureMgr::onLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLLoadInfo* info = (LLLoadInfo*)user_data;
	const LLUUID item_id = info->mItemID;
	const bool inform_server = info->mInformServer;
	const bool deactivate_similar = info->mDeactivateSimilar;
	delete info;
	info = nullptr;
	LLGestureMgr& self = LLGestureMgr::instance();
	--self.mLoadingCount;
	if (0 == status)
	{
		LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
		S32 size = file.getSize();
		std::vector<char> buffer(size+1, '\0');
		file.read((U8*)&buffer[0], size);
		LLMultiGesture* gesture = new LLMultiGesture();
		LLDataPackerAsciiBuffer dp(&buffer[0], size+1);
		if (gesture->deserialize(dp))
		{
			if (deactivate_similar)
			{
				self.deactivateSimilarGestures(gesture, item_id);
				if (self.mLoadingCount == 0
					&& self.mDeactivateSimilarNames.length() > 0)
				{
					LLSD args;
					args["NAMES"] = self.mDeactivateSimilarNames;
					LLNotificationsUtil::add("DeactivatedGesturesTrigger", args);
				}
			}
			LLViewerInventoryItem* item = gInventory.getItem(item_id);
			if(item)
			{
				gesture->mName = item->getName();
			}
			else
			{
				self.setFetchID(item_id);
				self.startFetch();
			}
			self.mActive[item_id] = gesture;
			gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
			if (inform_server)
			{
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessage("ActivateGestures");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID", gAgent.getID());
				msg->addUUID("SessionID", gAgent.getSessionID());
				msg->addU32("Flags", 0x0);
				msg->nextBlock("Data");
				msg->addUUID("ItemID", item_id);
				msg->addUUID("AssetID", asset_uuid);
				msg->addU32("GestureFlags", 0x0);
				gAgent.sendReliableMessage();
			}
			auto i_cb = self.mCallbackMap.find(item_id);
			if(i_cb != self.mCallbackMap.end())
			{
				i_cb->second(gesture);
				self.mCallbackMap.erase(i_cb);
			}
			self.notifyObservers();
		}
		else
		{
			LL_WARNS() << "Unable to load gesture" << LL_ENDL;
			self.mActive.erase(item_id);
			delete gesture;
			gesture = nullptr;
		}
	}
	else
	{
		LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
			LL_ERR_FILE_EMPTY == status)
		{
			LLDelayedGestureError::gestureMissing( item_id );
		}
		else
		{
			LLDelayedGestureError::gestureFailedToLoad( item_id );
		}
		LL_WARNS() << "Problem loading gesture: " << status << LL_ENDL;
		LLGestureMgr::instance().mActive.erase(item_id);
	}
}
void LLGestureMgr::onAssetLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLGestureMgr& self = LLGestureMgr::instance();
	switch(type)
	{
	case LLAssetType::AT_ANIMATION:
		{
			LLKeyframeMotion::onLoadComplete(vfs, asset_uuid, type, user_data, status, ext_status);
			self.mLoadingAssets.erase(asset_uuid);
			break;
		}
	case LLAssetType::AT_SOUND:
		{
			LLAudioEngine::assetCallback(vfs, asset_uuid, type, user_data, status, ext_status);
			self.mLoadingAssets.erase(asset_uuid);
			break;
		}
	default:
		{
			LL_WARNS() << "Unexpected asset type: " << type << LL_ENDL;
			llassert(type == LLAssetType::AT_ANIMATION || type == LLAssetType::AT_SOUND);
		}
	}
}
bool LLGestureMgr::hasLoadingAssets(LLMultiGesture* gesture)
{
	LLGestureMgr& self = LLGestureMgr::instance();
	for (auto& step : gesture->mSteps)
	{
		switch(step->getType())
		{
		case STEP_ANIMATION:
			{
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				const LLUUID& anim_id = anim_step->mAnimAssetID;
				if (!(anim_id.isNull()
					  || anim_step->mFlags & ANIM_FLAG_STOP
					  || self.mLoadingAssets.find(anim_id) == self.mLoadingAssets.end()))
				{
					return true;
				}
				break;
			}
		case STEP_SOUND:
			{
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				const LLUUID& sound_id = sound_step->mSoundAssetID;
				if (!(sound_id.isNull()
					  || self.mLoadingAssets.find(sound_id) == self.mLoadingAssets.end()))
				{
					return true;
				}
				break;
			}
		case STEP_CHAT:
		case STEP_WAIT:
		case STEP_EOF:
			{
				break;
			}
		default:
			{
				LL_WARNS() << "Unknown gesture step type: " << step->getType() << LL_ENDL;
			}
		}
	}
	return false;
}
void LLGestureMgr::stopGesture(LLMultiGesture* gesture)
{
	if (!gesture) return;
	for (const auto& anim_id : gesture->mRequestedAnimIDs)
	{
		gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_STOP);
	}
	gesture->mRequestedAnimIDs.clear();
	for (const auto& anim_id : gesture->mPlayingAnimIDs)
	{
		if (gesture->mLocal)
			gAgentAvatarp->stopMotion(anim_id, true);
		else
			gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_STOP);
	}
	gesture->mPlayingAnimIDs.clear();
	mPlaying.erase(std::remove(mPlaying.begin(), mPlaying.end(), gesture), mPlaying.end());
	gesture->reset();
	if (gesture->mDoneCallback)
	{
		gesture->mDoneCallback(gesture);
		gesture = nullptr;
	}
	notifyObservers();
}
void LLGestureMgr::stopGesture(const LLUUID& item_id)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	auto it = mActive.find(base_item_id);
	if (it == mActive.end()) return;
	LLMultiGesture* gesture = (*it).second;
	if (!gesture) return;
	stopGesture(gesture);
}
void LLGestureMgr::addObserver(LLGestureManagerObserver* observer)
{
	mObservers.push_back(observer);
}
void LLGestureMgr::removeObserver(LLGestureManagerObserver* observer)
{
	const auto& end = mObservers.end();
	auto it = std::find(mObservers.begin(), end, observer);
	if (it != end)
	{
		mObservers.erase(it);
	}
}
void LLGestureMgr::notifyObservers()
{
	LL_DEBUGS() << "LLGestureMgr::notifyObservers" << LL_ENDL;
	for(auto& observer : mObservers)
	{
		observer->changed();
	}
}
bool LLGestureMgr::matchPrefix(const std::string& in_str, std::string* out_str) const
{
	S32 in_len = in_str.length();
#ifdef MATCH_COMMON_CHARS
	std::string rest_of_match;
	std::string buf;
#endif
	for (const auto& pair : mActive)
	{
		const LLMultiGesture* gesture = pair.second;
		if (gesture)
		{
			const std::string& trigger = gesture->getTrigger();
#ifdef MATCH_COMMON_CHARS
			if (!LLStringUtil::compareInsensitive(in_str, trigger))
			{
				*out_str = trigger;
				return true;
			}
#else
			if (in_len > (S32)trigger.length()) continue;
#endif
			std::string trigger_trunc = trigger;
			LLStringUtil::truncate(trigger_trunc, in_len);
			if (!LLStringUtil::compareInsensitive(in_str, trigger_trunc))
			{
#ifndef MATCH_COMMON_CHARS
				*out_str = trigger;
				return true;
#else
				if (rest_of_match.empty())
				{
					rest_of_match = trigger.substr(in_str.size());
				}
				std::string cur_rest_of_match = trigger.substr(in_str.size());
				buf.clear();
				for (U32 i = 0; i < rest_of_match.length() && i < cur_rest_of_match.length(); ++i)
				{
					const auto& rest(rest_of_match[i])
					if (rest==cur_rest_of_match[i])
					{
						buf.push_back(rest);
					}
					else
					{
						if (i==0)
						{
							rest_of_match.clear();
						}
						break;
					}
				}
				if (rest_of_match.empty())
				{
					return false;
				}
				if (!buf.empty())
				{
					rest_of_match = buf;
				}
#endif
			}
		}
	}
#ifdef MATCH_COMMON_CHARS
	if (!rest_of_match.empty())
	{
		*out_str = in_str+rest_of_match;
		return true;
	}
#endif
	return false;
}
void LLGestureMgr::getItemIDs(uuid_vec_t* ids) const
{
	for (const auto& pair : mActive)
	{
		ids->push_back(pair.first);
	}
}
void LLGestureMgr::done()
{
	bool notify = false;
	for(const auto& pair : mActive)
	{
		if (pair.second && pair.second->mName.empty())
		{
			LLViewerInventoryItem* item = gInventory.getItem(pair.first);
			if(item)
			{
				pair.second->mName = item->getName();
				notify = true;
			}
		}
	}
	if(notify)
	{
		notifyObservers();
	}
}
