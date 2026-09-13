/** 
 * @file llmutelist.cpp
 * @author Richard Nelson, James Cook
 * @brief Management of list of muted players
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llmutelist.h"
#include "pipeline.h"
#include <boost/tokenizer.hpp>
#include "lldispatcher.h"
#include "llxfermanager.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llviewergenericmessage.h"
#include "llworld.h"
#include "llfloaterchat.h"
#include "llimpanel.h"
#include "llimview.h"
#include "llnotifications.h"
#include "llviewerobjectlist.h"
#include "lltrans.h"
namespace
{
	LLViewerObject* get_object_to_mute_from_id(const LLUUID& object_id)
	{
		LLViewerObject *objectp = gObjectList.findObject(object_id);
		if ((objectp) && (!objectp->isAvatar()))
		{
			LLViewerObject *parentp = (LLViewerObject *)objectp->getParent();
			if (parentp && parentp->getID() != gAgent.getID())
			{
				objectp = parentp;
			}
		}
		return objectp;
	}
}
class LLDispatchEmptyMuteList : public LLDispatchHandler
{
public:
	bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings) override
	{
		LLMuteList::getInstance()->setLoaded();
		return true;
	}
};
static LLDispatchEmptyMuteList sDispatchEmptyMuteList;
LLMute::LLMute(const LLUUID& id, const std::string& name, EType type, U32 flags)
  : mID(id),
	mName(name),
	mType(type),
	mFlags(flags)
{
	LLViewerObject* mute_object = get_object_to_mute_from_id(id);
	if(mute_object && mute_object->getID() != id)
	{
		mID = mute_object->getID();
		LLNameValue* firstname = mute_object->getNVPair("FirstName");
		LLNameValue* lastname = mute_object->getNVPair("LastName");
		if (firstname && lastname)
		{
			mName = LLCacheName::buildFullName(
				firstname->getString(), lastname->getString());
		}
		mType = mute_object->isAvatar() ? AGENT : OBJECT;
	}
}
std::string LLMute::getDisplayType() const
{
	switch (mType)
	{
		case BY_NAME:
		default:
			return LLTrans::getString("MuteByName");
			break;
		case AGENT:
			return LLTrans::getString("MuteAgent");
			break;
		case OBJECT:
			return LLTrans::getString("MuteObject");
			break;
		case GROUP:
			return LLTrans::getString("MuteGroup");
			break;
		case EXTERNAL:
			return LLTrans::getString("MuteExternal");
			break;
	}
}
LLMuteList* LLMuteList::getInstance()
{
	static bool registered = false;
	if( !registered && gMessageSystem)
	{
		registered = true;
		gMessageSystem->setHandlerFuncFast(_PREHASH_MuteListUpdate, processMuteListUpdate);
		gMessageSystem->setHandlerFuncFast(_PREHASH_UseCachedMuteList, processUseCachedMuteList);
	}
	return LLSingleton<LLMuteList>::getInstance();
}
LLMuteList::LLMuteList() :
	mIsLoaded(FALSE)
{
	gGenericDispatcher.addHandler("emptymutelist", &sDispatchEmptyMuteList);
}
LLMuteList::~LLMuteList()
{
}
bool LLMuteList::isLinden(const LLUUID& id) const
{
	std::string name;
	gCacheName->getFullName(id, name);
	return isLinden(name);
}
BOOL LLMuteList::isLinden(const std::string& name) const
{
	if (mGodFullNames.find(name) != mGodFullNames.end()) return true;
	if (mGodLastNames.empty()) return false;
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(name, sep);
	tokenizer::iterator token_iter = tokens.begin();
	if (token_iter == tokens.end()) return FALSE;
	++token_iter;
	if (token_iter == tokens.end()) return FALSE;
	std::string last_name = *token_iter;
	return mGodLastNames.find(last_name) != mGodLastNames.end();
}
static LLVOAvatar* find_avatar(const LLUUID& id)
{
	LLViewerObject *obj = gObjectList.findObject(id);
	while (obj && obj->isAttachment())
	{
		obj = (LLViewerObject *)obj->getParent();
	}
	if (obj && obj->isAvatar())
	{
		return (LLVOAvatar*)obj;
	}
	else
	{
		return nullptr;
	}
}
BOOL LLMuteList::add(const LLMute& mute, U32 flags)
{
	if ((mute.mType == LLMute::AGENT)
		&& isLinden(mute.mName) && (flags & LLMute::flagTextChat || flags == 0))
	{
        LL_WARNS() << "Trying to mute a Linden; ignored" << LL_ENDL;
		LLNotifications::instance().add("MuteLinden", LLSD(), LLSD());
		return FALSE;
	}
	if (mute.mType == LLMute::AGENT
		&& mute.mID == gAgent.getID())
	{
        LL_WARNS() << "Trying to self; ignored" << LL_ENDL;
		return FALSE;
	}
	if (mute.mType == LLMute::BY_NAME)
	{
		if (mute.mName.empty())
		{
			LL_WARNS() << "Trying to mute empty string by-name" << LL_ENDL;
			return FALSE;
		}
		if (mute.mID.notNull())
		{
			LL_WARNS() << "Trying to add by-name mute with non-null id" << LL_ENDL;
			return FALSE;
		}
		std::pair<string_set_t::iterator, bool> result = mLegacyMutes.insert(mute.mName);
		if (result.second)
		{
			LL_INFOS() << "Muting by name " << mute.mName << LL_ENDL;
			updateAdd(mute);
			notifyObservers();
			notifyObserversDetailed(mute);
			return TRUE;
		}
		else
		{
			LL_INFOS() << "duplicate mute ignored" << LL_ENDL;
			return FALSE;
		}
	}
	LLMute localmute = mute;
	mute_set_t::iterator it = mMutes.find(localmute);
	bool duplicate = it != mMutes.end();
	if (duplicate)
	{
		localmute.mFlags = it->mFlags;
		mMutes.erase(it);
	}
	else
	{
		localmute.mFlags = LLMute::flagAll;
	}
	if(flags)
	{
		localmute.mFlags &= (~flags);
	}
	else
	{
		localmute.mFlags = 0;
	}
	{
		auto result = mMutes.insert(localmute);
		if (result.second)
		{
			LL_INFOS() << "Muting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << LL_ENDL;
			updateAdd(localmute);
			notifyObservers();
			notifyObserversDetailed(localmute);
			if(!(localmute.mFlags & LLMute::flagParticles))
			{
				if(localmute.mType == LLMute::AGENT || localmute.mType == LLMute::OBJECT)
				{
					LLViewerPartSim::getInstance()->clearParticlesByOwnerID(localmute.mID);
				}
			}
			LLVOAvatar *avatarp = find_avatar(localmute.mID);
			if (avatarp)
			{
				LLPipeline::removeMutedAVsLights(avatarp);
			}
			return !duplicate;
		}
	}
	return FALSE;
}
void LLMuteList::updateAdd(const LLMute& mute)
{
	if (mute.mType == LLMute::EXTERNAL)
	{
		return;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addStringFast(_PREHASH_MuteName, mute.mName);
	msg->addS32("MuteType", mute.mType);
	msg->addU32("MuteFlags", mute.mFlags);
	gAgent.sendReliableMessage();
	mIsLoaded = TRUE;
}
BOOL LLMuteList::remove(const LLMute& mute, U32 flags)
{
	BOOL found = FALSE;
	mute_set_t::iterator it = mMutes.find(mute);
	if (it != mMutes.end())
	{
		LLMute localmute = *it;
		bool remove = true;
		if(flags)
		{
			localmute.mFlags |= flags;
			if(localmute.mFlags == LLMute::flagAll)
			{
			}
			else
			{
				remove = false;
			}
		}
		else
		{
		}
		mMutes.erase(it);
		if(remove)
		{
			updateRemove(localmute);
			LL_INFOS() << "Unmuting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << LL_ENDL;
		}
		else
		{
			mMutes.insert(localmute);
			updateAdd(localmute);
			LL_INFOS() << "Updating mute entry " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << LL_ENDL;
		}
		notifyObserversDetailed(localmute);
		setLoaded();
	}
	else
	{
		string_set_t::iterator legacy_it = mLegacyMutes.find(mute.mName);
		if (legacy_it != mLegacyMutes.end())
		{
			LLMute mute(LLUUID::null, *legacy_it, LLMute::BY_NAME);
			updateRemove(mute);
			mLegacyMutes.erase(legacy_it);
			notifyObserversDetailed(mute);
			setLoaded();
		}
	}
	return found;
}
void LLMuteList::updateRemove(const LLMute& mute)
{
	if (mute.mType == LLMute::EXTERNAL)
	{
		return;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addString("MuteName", mute.mName);
	gAgent.sendReliableMessage();
}
static void notify_automute_callback(const LLUUID& agent_id, const LLMuteList::EAutoReason& reason)
{
	std::string notif_name;
	switch (reason)
	{
	default:
	case LLMuteList::AR_IM:
		notif_name = "AutoUnmuteByIM";
		break;
	case LLMuteList::AR_INVENTORY:
		notif_name = "AutoUnmuteByInventory";
		break;
	case LLMuteList::AR_MONEY:
		notif_name = "AutoUnmuteByMoney";
		break;
	}
	if (auto notif_ptr = LLNotifications::instance().add(notif_name, LLSD().with("NAME", LLAvatarActions::getSLURL(agent_id)), LLSD()))
	{
		std::string message = notif_ptr->getMessage();
		if (reason == LLMuteList::AR_IM)
		{
			if (LLFloaterIMPanel* timp = gIMMgr->findFloaterBySession(agent_id))
			{
				timp->addHistoryLine(message);
			}
		}
		LLFloaterChat::addChat(message, FALSE, FALSE);
	}
}
BOOL LLMuteList::autoRemove(const LLUUID& agent_id, const EAutoReason reason)
{
	BOOL removed = FALSE;
	if (isMuted(agent_id) && !(reason == AR_INVENTORY && gSavedPerAccountSettings.getBOOL("AutoresponseMutedItem")))
	{
		LLMute automute(agent_id, LLStringUtil::null, LLMute::AGENT);
		removed = TRUE;
		remove(automute);
		notify_automute_callback(agent_id, reason);
	}
	return removed;
}
std::vector<LLMute> LLMuteList::getMutes() const
{
	std::vector<LLMute> mutes;
	for (const auto& mMute : mMutes)
	{
		mutes.push_back(mMute);
	}
	for (const auto& mLegacyMute : mLegacyMutes)
	{
		LLMute legacy(LLUUID::null, mLegacyMute);
		mutes.push_back(legacy);
	}
	std::sort(mutes.begin(), mutes.end(), compare_by_name());
	return mutes;
}
BOOL LLMuteList::loadFromFile(const std::string& filename)
{
	if(filename.empty())
	{
		LL_WARNS() << "Mute List Filename is Empty!" << LL_ENDL;
		return FALSE;
	}
	LLFILE* fp = LLFile::fopen(filename, "rb");
	if (!fp)
	{
		LL_WARNS() << "Couldn't open mute list " << filename << LL_ENDL;
		return FALSE;
	}
	char id_buffer[MAX_STRING];
	char name_buffer[MAX_STRING];
	char buffer[MAX_STRING];
	while (!feof(fp)
		   && fgets(buffer, MAX_STRING, fp))
	{
		id_buffer[0] = '\0';
		name_buffer[0] = '\0';
		S32 type = 0;
		U32 flags = 0;
		sscanf(
			buffer, " %d %254s %254[^|]| %u\n", &type, id_buffer, name_buffer,
			&flags);
		LLUUID id = LLUUID(id_buffer);
		LLMute mute(id, std::string(name_buffer), (LLMute::EType)type, flags);
		if (mute.mID.isNull()
			|| mute.mType == LLMute::BY_NAME)
		{
			mLegacyMutes.insert(mute.mName);
		}
		else
		{
			mMutes.insert(mute);
		}
	}
	fclose(fp);
	setLoaded();
	return TRUE;
}
BOOL LLMuteList::saveToFile(const std::string& filename)
{
	if(filename.empty())
	{
		LL_WARNS() << "Mute List Filename is Empty!" << LL_ENDL;
		return FALSE;
	}
	LLFILE* fp = LLFile::fopen(filename, "wb");
	if (!fp)
	{
		LL_WARNS() << "Couldn't open mute list " << filename << LL_ENDL;
		return FALSE;
	}
	std::string id_string;
	LLUUID::null.toString(id_string);
	for (const auto& mLegacyMute : mLegacyMutes)
	{
		fprintf(fp, "%d %s %s|\n", (S32)LLMute::BY_NAME, id_string.c_str(), mLegacyMute.c_str());
	}
	for (const auto& mMute : mMutes)
	{
		if (mMute.mType != LLMute::EXTERNAL)
		{
            mMute.mID.toString(id_string);
			const std::string& name = mMute.mName;
			fprintf(fp, "%d %s %s|%u\n", (S32)mMute.mType, id_string.c_str(), name.c_str(), mMute.mFlags);
		}
	}
	fclose(fp);
	return TRUE;
}
BOOL LLMuteList::isMuted(const LLUUID& id, const std::string& name, U32 flags) const
{
	if (mMutes.empty() && mLegacyMutes.empty())
		return FALSE;
	LLViewerObject* mute_object = get_object_to_mute_from_id(id);
	LLUUID id_to_check  = (mute_object) ? mute_object->getID() : id;
	if (id_to_check == gAgentID) return false;
	LLMute mute(id_to_check);
	mute_set_t::const_iterator mute_it = mMutes.find(mute);
	if (mute_it != mMutes.end())
	{
		if(flags & mute_it->mFlags)
		{
			return FALSE;
		}
		return TRUE;
	}
	bool avatar = mute_object && mute_object->isAvatar();
	if (name.empty() || avatar) return FALSE;
	string_set_t::const_iterator legacy_it = mLegacyMutes.find(name);
	return legacy_it != mLegacyMutes.end();
}
void LLMuteList::requestFromServer(const LLUUID& agent_id)
{
    std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,
        llformat("%s.cached_mute", agent_id.asString().c_str()));
	LLCRC crc;
	crc.update(filename);
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MuteListRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, agent_id);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addU32Fast(_PREHASH_MuteCRC, crc.getCRC());
	gAgent.sendReliableMessage();
}
void LLMuteList::cache(const LLUUID& agent_id)
{
	if(mIsLoaded)
	{
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,
            llformat("%s.cached_mute", agent_id.asString().c_str()));
		saveToFile(filename);
	}
}
void LLMuteList::processMuteListUpdate(LLMessageSystem* msg, void**)
{
	LL_INFOS() << "LLMuteList::processMuteListUpdate()" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_MuteData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got an mute list update for the wrong agent." << LL_ENDL;
		return;
	}
	std::string unclean_filename;
	msg->getStringFast(_PREHASH_MuteData, _PREHASH_Filename, unclean_filename);
	std::string filename = LLDir::getScrubbedFileName(unclean_filename);
	std::string *local_filename_and_path = new std::string(gDirUtilp->getExpandedFilename( LL_PATH_CACHE, filename ));
	gXferManager->requestFile(*local_filename_and_path,
							  filename,
							  LL_PATH_CACHE,
							  msg->getSender(),
							  TRUE,
							  onFileMuteList,
							  (void**)local_filename_and_path,
							  LLXferManager::HIGH_PRIORITY);
}
void LLMuteList::processUseCachedMuteList(LLMessageSystem* msg, void**)
{
	LL_INFOS() << "LLMuteList::processUseCachedMuteList()" << LL_ENDL;
    std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,
        llformat("%s.cached_mute", gAgent.getID().asString().c_str()));
	LLMuteList::getInstance()->loadFromFile(filename);
}
void LLMuteList::onFileMuteList(void** user_data, S32 error_code, LLExtStat ext_status)
{
	LL_INFOS() << "LLMuteList::processMuteListFile()" << LL_ENDL;
	std::string* local_filename_and_path = (std::string*)user_data;
	if(local_filename_and_path && !local_filename_and_path->empty() && (error_code == 0))
	{
		LLMuteList::getInstance()->loadFromFile(*local_filename_and_path);
		LLFile::remove(*local_filename_and_path);
	}
	delete local_filename_and_path;
}
void LLMuteList::addObserver(LLMuteListObserver* observer)
{
	mObservers.insert(observer);
}
void LLMuteList::removeObserver(LLMuteListObserver* observer)
{
	mObservers.erase(observer);
}
void LLMuteList::setLoaded()
{
	mIsLoaded = TRUE;
	notifyObservers();
}
void LLMuteList::notifyObservers()
{
	for (observer_set_t::iterator it = mObservers.begin();
		it != mObservers.end();
		)
	{
		LLMuteListObserver* observer = *it;
		observer->onChange();
		it = mObservers.upper_bound(observer);
	}
}
void LLMuteList::notifyObserversDetailed(const LLMute& mute)
{
	for (observer_set_t::iterator it = mObservers.begin();
		it != mObservers.end();
		)
	{
		LLMuteListObserver* observer = *it;
		observer->onChangeDetailed(mute);
		it = mObservers.upper_bound(observer);
	}
}
