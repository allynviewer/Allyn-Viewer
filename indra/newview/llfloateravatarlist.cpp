/**
 * @file llfloateravatarlist.cpp
 * @brief Avatar list/radar floater
 *
 * @author Dale Glass <dale@daleglass.net>, (C) 2007
 */
#include "llviewerprecompiledheaders.h"
#include "llfloateravatarlist.h"
#include "llaudioengine.h"
#include "llavatarconstants.h"
#include "llavatarnamecache.h"
#include "llfiltereditor.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "llscrolllistcolumn.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsdutil.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "hippogridmanager.h"
#include "lfsimfeaturehandler.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llavataractions.h"
#include "llfloaterchat.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "llmutelist.h"
#include "llspeakers.h"
#include "lltracker.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoiceclient.h"
#include "llworld.h"
#include <boost/date_time.hpp>
#include "rlvhandler.h"
LLVector3d unpackLocalToGlobalPosition(U32 compact_local, const LLVector3d& origin);
extern U32 gFrameCount;
const S32& radar_namesystem()
{
	static const LLCachedControl<S32> namesystem("RadarNameSystem");
	return namesystem;
}
namespace
{
	void chat_avatar_status(const std::string& name, const LLUUID& key, ERadarStatType type, bool entering, const F32& dist, bool flood = false)
	{
		static LLCachedControl<bool> radar_chat_alerts(gSavedSettings, "RadarChatAlerts");
		if (!radar_chat_alerts) return;
		if (!entering && gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
		{
			return;
		}
		if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS)) return;
		static LLCachedControl<bool> radar_alert_flood(gSavedSettings, "RadarAlertFlood");
		if (flood && !radar_alert_flood) return;
		static LLCachedControl<bool> radar_alert_sim(gSavedSettings, "RadarAlertSim");
		static LLCachedControl<bool> radar_alert_draw(gSavedSettings, "RadarAlertDraw");
		static LLCachedControl<bool> radar_alert_shout_range(gSavedSettings, "RadarAlertShoutRange");
		static LLCachedControl<bool> radar_alert_chat_range(gSavedSettings, "RadarAlertChatRange");
		static LLCachedControl<bool> radar_alert_age(gSavedSettings, "RadarAlertAge");
		LLStringUtil::format_map_t args;
		LLChat chat;
		switch(type)
		{
			case STAT_TYPE_SIM:			if (radar_alert_sim)			args["[RANGE]"] = LLTrans::getString("the_sim");											break;
			case STAT_TYPE_DRAW:		if (radar_alert_draw)			args["[RANGE]"] = LLTrans::getString("draw_distance");										break;
			case STAT_TYPE_SHOUTRANGE:	if (radar_alert_shout_range)	args["[RANGE]"] = LLTrans::getString("shout_range");										break;
			case STAT_TYPE_CHATRANGE:	if (radar_alert_chat_range)		args["[RANGE]"] = LLTrans::getString("chat_range");										break;
			case STAT_TYPE_AGE:			if (radar_alert_age)			chat.mText = name + ' ' + LLTrans::getString("has_triggered_your_avatar_age_alert") + '.';	break;
			default:					llassert(type);																											break;
		}
		args["[NAME]"] = name;
		args["[ACTION]"] = LLTrans::getString((entering ? "has_entered" : "has_left") + (flood ? "_flood" : LLStringUtil::null));
		if (args.find("[RANGE]") != args.end())
			chat.mText = LLTrans::getString("radar_alert_template", args);
		else if (chat.mText.empty()) return;
		if (entering)
		{
			static const LLCachedControl<bool> radar_show_dist("RadarAlertShowDist");
			if (radar_show_dist) chat.mText += llformat(" (%.2fm)", dist);
		}
		chat.mFromName = name;
		chat.mFromID = key;
		if (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
			chat.mURL = LLAvatarActions::getSLURL(key);
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		LLFloaterChat::addChat(chat);
	}
	void send_keys_message(const int transact_num, const int num_ids, const std::string& ids)
	{
		gMessageSystem->newMessage("ScriptDialogReply");
		gMessageSystem->nextBlock("AgentData");
		gMessageSystem->addUUID("AgentID", gAgentID);
		gMessageSystem->addUUID("SessionID", gAgentSessionID);
		gMessageSystem->nextBlock("Data");
		gMessageSystem->addUUID("ObjectID", gAgentID);
		gMessageSystem->addS32("ChatChannel", -777777777);
		gMessageSystem->addS32("ButtonIndex", 1);
		gMessageSystem->addString("ButtonLabel", llformat("%d,%d", transact_num, num_ids) + ids);
		gAgent.sendReliableMessage();
	}
}
const LLColor4* mm_getMarkerColor(const LLUUID& id, bool mark_only = true);
LLAvatarListEntry::LLAvatarListEntry(const LLUUID& id, const std::string& name, const LLVector3d& position) :
		mID(id), mName(name), mPosition(position), mMarked(mm_getMarkerColor(id)), mFocused(false),
		mStats(),
		mActivityType(ACTIVITY_NEW), mActivityTimer(),
		mIsInList(false), mAge(-1), mTime(time(NULL))
{
	LLAvatarPropertiesProcessor& inst(LLAvatarPropertiesProcessor::instance());
	inst.addObserver(mID, this);
	inst.sendAvatarPropertiesRequest(mID);
	inst.sendAvatarNotesRequest(mID);
}
LLAvatarListEntry::~LLAvatarListEntry()
{
	static LLCachedControl<bool> radar_alert_flood_leaving(gSavedSettings, "RadarAlertFloodLeaving");
	bool cleanup = LLFloaterAvatarList::isCleanup();
	if (radar_alert_flood_leaving || !cleanup)
	{
		setPosition(mPosition, F32_MIN, false, cleanup);
	}
	LLAvatarPropertiesProcessor::instance().removeObserver(mID, this);
}
void LLAvatarListEntry::processProperties(void* data, EAvatarProcessorType type)
{
	switch(type)
	{
		case APT_PROPERTIES:
			if (mAge == -1)
			{
				LLAvatarPropertiesProcessor& inst(LLAvatarPropertiesProcessor::instance());
				const LLAvatarData* pAvatarData = static_cast<const LLAvatarData*>(data);
				if (pAvatarData && (pAvatarData->avatar_id.notNull()))
				{
					using namespace boost::gregorian;
					int year, month, day;
					const auto born = pAvatarData->born_on;
					if (born.empty()) return;
					if (sscanf(born.c_str(),"%d/%d/%d",&month,&day,&year) == 3)
					try
					{
						mAge = (day_clock::local_day() - date(year, month, day)).days();
					}
					catch(const std::out_of_range&) {}
					if (mAge >= 0)
					{
						static const LLCachedControl<U32> sAvatarAgeAlertDays(gSavedSettings, "AvatarAgeAlertDays");
						if (!mStats[STAT_TYPE_AGE] && (U32)mAge < sAvatarAgeAlertDays)
							chat_avatar_status(mName, mID, STAT_TYPE_AGE, mStats[STAT_TYPE_AGE] = true, (mPosition - gAgent.getPositionGlobal()).magVec());
					}
					else
					{
						LL_WARNS() << "Failed to extract age from APT_PROPERTIES for " << mID << ", received \"" << born << "\"." << LL_ENDL;
						if (!gHippoGridManager->getConnectedGrid()->isSecondLife())
						{
							LL_WARNS() << "Requesting properties again." << LL_ENDL;
							inst.sendAvatarPropertiesRequest(mID);
						}
					}
				}
			}
			break;
		case APT_NOTES:
			if (const LLAvatarNotes* pAvatarNotes = static_cast<const LLAvatarNotes*>(data))
				mNotes = !pAvatarNotes->notes.empty();
		break;
		default: break;
	}
}
void LLAvatarListEntry::setPosition(const LLVector3d& position, const F32& dist, bool drawn, bool flood)
{
	mPosition = position;
	bool here(dist != F32_MIN);
	bool this_sim(here && (gAgent.getRegion()->pointInRegionGlobal(position) || !(LLWorld::getInstance()->positionRegionValidGlobal(position))));
	if (this_sim != mStats[STAT_TYPE_SIM])			chat_avatar_status(mName, mID, STAT_TYPE_SIM, mStats[STAT_TYPE_SIM] = this_sim, dist, flood);
	if (drawn != mStats[STAT_TYPE_DRAW])			chat_avatar_status(mName, mID, STAT_TYPE_DRAW, mStats[STAT_TYPE_DRAW] = drawn, dist, flood);
	bool shoutrange(here && dist < LFSimFeatureHandler::getInstance()->shoutRange());
	if (shoutrange != mStats[STAT_TYPE_SHOUTRANGE])	chat_avatar_status(mName, mID, STAT_TYPE_SHOUTRANGE, mStats[STAT_TYPE_SHOUTRANGE] = shoutrange, dist, flood);
	bool chatrange(here && dist < LFSimFeatureHandler::getInstance()->sayRange());
	if (chatrange != mStats[STAT_TYPE_CHATRANGE])	chat_avatar_status(mName, mID, STAT_TYPE_CHATRANGE, mStats[STAT_TYPE_CHATRANGE] = chatrange, dist, flood);
}
void LLAvatarListEntry::resetName(const bool& hide_tags, const bool& anon_names, const std::string& hidden)
{
	if (hide_tags)
		mName = hidden;
	else
	{
		LLAvatarNameCache::getNSName(mID, mName, radar_namesystem());
		if (anon_names) mName = RlvStrings::getAnonym(mName);
	}
}
const F32 ACTIVITY_TIMEOUT = 1.0f;
void LLAvatarListEntry::setActivity(ACTIVITY_TYPE activity)
{
	if (activity >= mActivityType || mActivityTimer.getElapsedTimeF32() > ACTIVITY_TIMEOUT)
	{
		mActivityType = activity;
		mActivityTimer.start();
	}
}
const LLAvatarListEntry::ACTIVITY_TYPE LLAvatarListEntry::getActivity()
{
	if (mActivityTimer.getElapsedTimeF32() > ACTIVITY_TIMEOUT)
	{
		mActivityType = ACTIVITY_NONE;
	}
	return mActivityType;
}
LLFloaterAvatarList::LLFloaterAvatarList() :  LLFloater(std::string("radar")),
	mTracking(false),
	mUpdate("RadarUpdateEnabled"),
	mDirtyAvatarSorting(false),
	mAvatarList(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_radar.xml");
}
LLFloaterAvatarList::~LLFloaterAvatarList()
{
	mCleanup = true;
	LLSD sort;
	for (const auto& col : mAvatarList->getSortOrder())
	{
		sort.append(mAvatarList->getColumn(col.first)->mName);
		sort.append(col.second);
	}
	gSavedSettings.setLLSD("RadarSortOrder", sort);
	mAvatars.clear();
}
void LLFloaterAvatarList::toggleInstance(const LLSD&)
{
	if (!instanceVisible())
	{
		showInstance();
	}
	else
	{
		getInstance()->close();
	}
}
void LLFloaterAvatarList::showInstance()
{
	getInstance()->open();
}
void LLFloaterAvatarList::draw()
{
	LLFloater::draw();
}
void LLFloaterAvatarList::onOpen()
{
	refreshAvatarList();
}
void LLFloaterAvatarList::onClose(bool app_quitting)
{
	if (app_quitting || !gSavedSettings.getBOOL("RadarKeepOpen"))
		destroy();
	else
		setVisible(false);
}
BOOL LLFloaterAvatarList::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS) || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) return TRUE;
	return LLFloater::handleRightMouseDown(x, y, mask);
}
bool is_nearby(const LLUUID& id)
{
	if (id.isNull()) return false;
	if (const auto inst = LLFloaterAvatarList::getIfExists())
		return inst->getAvatarEntry(id);
	uuid_vec_t avatars;
	LLWorld::instance().getAvatars(&avatars);
	return std::find(avatars.begin(), avatars.end(), id) != avatars.end();
}
const LLVector3d& get_av_pos(const LLUUID& id)
{
	if (const auto inst = LLFloaterAvatarList::getIfExists())
		if (const auto av = inst->getAvatarEntry(id))
			return av->getPosition();
	LLWorld::pos_map_t avatars;
	LLWorld::instance().getAvatars(&avatars);
	return avatars[id];
}
void track_av(const LLUUID& id)
{
	if (auto inst = LLFloaterAvatarList::getIfExists())
		if (inst->getAvatarEntry(id))
		{
			inst->trackAvatar(id);
			return;
		}
	LLWorld::pos_map_t avatars;
	LLWorld::instance().getAvatars(&avatars);
	LLTracker::trackLocation(avatars[id], LLStringUtil::null, LLStringUtil::null);
}
static void cmd_profile(const LLAvatarListEntry* entry);
static void cmd_toggle_mark(LLAvatarListEntry* entry)
{
	bool mark = !entry->isMarked();
	void mm_setcolor(LLUUID key, LLColor4 col);
	void mm_clearMark(const LLUUID & id);
	mark ? mm_setcolor(entry->getID(), LLColor4::red) : mm_clearMark(entry->getID());
	entry->setMarked(mark);
}
static void cmd_ar(const LLAvatarListEntry* entry);
static void cmd_teleport(const LLAvatarListEntry* entry);
const LLUUID& active_owner_or_id(const LLSD& userdata);
namespace
{
	typedef LLMemberListener<LLView> view_listener_t;
	class RadarTrack final : public view_listener_t
	{
		bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata) override
		{
			LLFloaterAvatarList::instance().onClickTrack();
			return true;
		}
	};
	class RadarFocus final : public view_listener_t
	{
		bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata) override
		{
			LLFloaterAvatarList::setFocusAvatar(active_owner_or_id(userdata));
			return true;
		}
	};
	class RadarFocusPrev final : public view_listener_t
	{
		bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata) override
		{
			LLFloaterAvatarList::instance().focusOnPrev(userdata.asInteger());
			return true;
		}
	};
	class RadarFocusNext final : public view_listener_t
	{
		bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata) override
		{
			LLFloaterAvatarList::instance().focusOnNext(userdata.asInteger());
			return true;
		}
	};
	class RadarAnnounceKeys final : public view_listener_t
	{
		bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata) override
		{
			LLFloaterAvatarList::instance().sendKeys();
			return true;
		}
	};
}
void addMenu(view_listener_t* menu, const std::string& name);
void add_radar_listeners()
{
	addMenu(new RadarTrack, "Radar.Track");
	addMenu(new RadarFocus, "Radar.Focus");
	addMenu(new RadarFocusPrev, "Radar.FocusPrev");
	addMenu(new RadarFocusNext, "Radar.FocusNext");
	addMenu(new RadarAnnounceKeys, "Radar.AnnounceKeys");
}
BOOL LLFloaterAvatarList::postBuild()
{
	childSetAction("profile_btn", boost::bind(&LLFloaterAvatarList::doCommand, this, &cmd_profile, false));
	childSetAction("im_btn", boost::bind(&LLFloaterAvatarList::onClickIM, this));
	childSetAction("offer_btn", boost::bind(&LLFloaterAvatarList::onClickTeleportOffer, this));
	childSetAction("track_btn", boost::bind(&LLFloaterAvatarList::onClickTrack, this));
	childSetAction("mark_btn", boost::bind(&LLFloaterAvatarList::doCommand, this, &cmd_toggle_mark, false));
	childSetAction("focus_btn", boost::bind(&LLFloaterAvatarList::onClickFocus, this));
	childSetAction("prev_in_list_btn", boost::bind(&LLFloaterAvatarList::focusOnPrev, this, false));
	childSetAction("next_in_list_btn", boost::bind(&LLFloaterAvatarList::focusOnNext, this, false));
	childSetAction("prev_marked_btn", boost::bind(&LLFloaterAvatarList::focusOnPrev, this, true));
	childSetAction("next_marked_btn", boost::bind(&LLFloaterAvatarList::focusOnNext, this, true));
	childSetAction("get_key_btn", boost::bind(&LLFloaterAvatarList::onClickGetKey, this));
	childSetAction("freeze_btn", boost::bind(&LLFloaterAvatarList::onClickFreeze, this));
	childSetAction("eject_btn", boost::bind(&LLFloaterAvatarList::onClickEject, this));
	childSetAction("mute_btn", boost::bind(&LLFloaterAvatarList::onClickMute, this));
	childSetAction("ar_btn", boost::bind(&LLFloaterAvatarList::doCommand, this, &cmd_ar, true));
	childSetAction("teleport_btn", boost::bind(&LLFloaterAvatarList::doCommand, this, &cmd_teleport, true));
	childSetAction("estate_eject_btn", boost::bind(&LLFloaterAvatarList::onClickEjectFromEstate, this));
	childSetAction("estate_ban_btn",boost::bind(&LLFloaterAvatarList:: onClickBanFromEstate, this));
	childSetAction("send_keys_btn", boost::bind(&LLFloaterAvatarList::sendKeys, this));
	gSavedSettings.getControl("RadarColumnMarkHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	gSavedSettings.getControl("RadarColumnPositionHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	gSavedSettings.getControl("RadarColumnAltitudeHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	gSavedSettings.getControl("RadarColumnActivityHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	gSavedSettings.getControl("RadarColumnVoiceHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	gSavedSettings.getControl("RadarColumnNotesHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	gSavedSettings.getControl("RadarColumnAgeHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	gSavedSettings.getControl("RadarColumnTimeHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	if (LLFilterEditor* filter = getChild<LLFilterEditor>("filter_input"))
	{
		filter->setCommitCallback(boost::bind(&LLFloaterAvatarList::onFilterEdit, this, _2));
	}
	mAvatarList = getChild<LLScrollListCtrl>("avatar_list");
	const LLSD sort = gSavedSettings.getLLSD("RadarSortOrder");
	for (auto it = sort.beginArray(), end = sort.endArray(); it < end; ++it)
	{
		const auto& column_name = (*it);
		const auto& ascending = (*++it);
		mAvatarList->sortByColumn(column_name.asStringRef(), ascending.asBoolean());
	}
	mAvatarList->setCommitOnSelectionChange(true);
	mAvatarList->setCommitCallback(boost::bind(&LLFloaterAvatarList::onSelectName,this));
	mAvatarList->setDoubleClickCallback(boost::bind(&LLFloaterAvatarList::onClickFocus,this));
	mAvatarList->setSortChangedCallback(boost::bind(&LLFloaterAvatarList::onAvatarSortingChanged,this));
	for (LLViewerRegion* region : LLWorld::instance().getRegionList())
	{
		updateAvatarList(region, true);
	}
	assessColumns();
	if(gHippoGridManager->getConnectedGrid()->isSecondLife())
		childSetVisible("hide_client", false);
	else
		gSavedSettings.getControl("RadarColumnClientHidden")->getSignal()->connect(boost::bind(&LLFloaterAvatarList::assessColumns, this));
	return true;
}
void col_helper(const bool hide, LLCachedControl<S32> &setting, LLScrollListColumn* col)
{
	const int width = col->getWidth();
	if (hide && width)
	{
		setting = width;
		col->setWidth(0);
	}
	else if(!hide && !width)
	{
		col->setWidth(setting);
	}
}
enum AVATARS_COLUMN_ORDER
{
	LIST_MARK,
	LIST_AVATAR_NAME,
	LIST_DISTANCE,
	LIST_POSITION,
	LIST_ALTITUDE,
	LIST_ACTIVITY,
	LIST_VOICE,
	LIST_NOTES,
	LIST_AGE,
	LIST_TIME,
	LIST_CLIENT,
};
#define BIND_COLUMN_TO_SETTINGS(col, name)\
	static const LLCachedControl<bool> hide_##name(gSavedSettings, "RadarColumn"#name"Hidden");\
	static LLCachedControl<S32> width_##name(gSavedSettings, "RadarColumn"#name"Width");\
	col_helper(hide_##name, width_##name, mAvatarList->getColumn(col));
void LLFloaterAvatarList::assessColumns()
{
	BIND_COLUMN_TO_SETTINGS(LIST_MARK,Mark);
	BIND_COLUMN_TO_SETTINGS(LIST_POSITION,Position);
	BIND_COLUMN_TO_SETTINGS(LIST_ALTITUDE,Altitude);
	BIND_COLUMN_TO_SETTINGS(LIST_ACTIVITY,Activity);
	BIND_COLUMN_TO_SETTINGS(LIST_VOICE,Voice);
	BIND_COLUMN_TO_SETTINGS(LIST_NOTES,Notes);
	BIND_COLUMN_TO_SETTINGS(LIST_AGE,Age);
	BIND_COLUMN_TO_SETTINGS(LIST_TIME,Time);
	static const LLCachedControl<bool> hide_client(gSavedSettings, "RadarColumnClientHidden");
	static LLCachedControl<S32> width_name(gSavedSettings, "RadarColumnNameWidth");
	bool client_hidden = hide_client || gHippoGridManager->getConnectedGrid()->isSecondLife();
	LLScrollListColumn* name_col = mAvatarList->getColumn(LIST_AVATAR_NAME);
	LLScrollListColumn* client_col = mAvatarList->getColumn(LIST_CLIENT);
	if (client_hidden != !!name_col->mDynamicWidth)
	{
		if(client_hidden && !gHippoGridManager->getConnectedGrid()->isSecondLife() && name_col->getWidth() > 0)
			width_name = name_col->getWidth();
		client_col->setWidth(0);
		name_col->setWidth(0);
		client_col->mDynamicWidth =	!client_hidden;
		name_col->mDynamicWidth =	 client_hidden;
		mAvatarList->setNumDynamicColumns(1);
		if(!client_hidden)
		{
			name_col->setWidth(llmax(width_name.get(),10));
		}
	}
	mAvatarList->updateLayout();
}
void updateParticleActivity(LLDrawable *drawablep)
{
	if (LLFloaterAvatarList::instanceExists())
	{
		LLViewerObject *vobj = drawablep->getVObj();
		if (vobj && vobj->isParticleSource())
		{
			LLUUID id = vobj->mPartSourcep->getOwnerUUID();
			LLAvatarListEntry *ent = LLFloaterAvatarList::getInstance()->getAvatarEntry(id);
			if ( NULL != ent )
			{
				ent->setActivity(LLAvatarListEntry::ACTIVITY_PARTICLES);
			}
		}
	}
}
const F32& radar_range_radius()
{
	static const LLCachedControl<F32> radius("RadarRangeRadius", 0);
	return radius;
}
void LLFloaterAvatarList::updateAvatarList(const LLViewerRegion* region, bool first)
{
	if (!mUpdate)
	{
		refreshTracker();
		return;
	}
	{
		const std::vector<U32>& map_avs(region->mMapAvatars);
		const uuid_vec_t& map_avids(region->mMapAvatarIDs);
		const LLVector3d& mypos(gAgent.getPositionGlobal());
		const LLVector3d& origin(region->getOriginGlobal());
		const F32 max_range(radar_range_radius() * radar_range_radius());
		static LLCachedControl<bool> announce(gSavedSettings, "RadarChatKeys");
		std::queue<LLUUID> announce_keys;
		bool no_names(gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS));
		bool anon_names(!no_names && gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES));
		const std::string& rlv_hidden(RlvStrings::getString(RLV_STRING_HIDDEN));
		for (size_t i = 0, size = map_avs.size(); i < size; ++i)
		{
			const LLUUID& avid = map_avids[i];
			LLVector3d position(unpackLocalToGlobalPosition(map_avs[i], origin));
			LLVOAvatar* avatarp = gObjectList.findAvatar(avid);
			if (avatarp) position = gAgent.getPosGlobalFromAgent(avatarp->getCharacterPosition());
			if (max_range && dist_vec_squared(position, mypos) > max_range) continue;
			std::string name;
			if (no_names) name = rlv_hidden;
			else if (!LLAvatarNameCache::getNSName(avid, name, radar_namesystem())) continue;
			else if (anon_names) name = RlvStrings::getAnonym(name);
			LLAvatarListEntry* entry = getAvatarEntry(avid);
			if (!entry)
			{
				if (announce && gAgent.getRegion()->pointInRegionGlobal(position)) announce_keys.push(avid);
				mAvatars.push_back(LLAvatarListEntryPtr(entry = new LLAvatarListEntry(avid, name, position)));
			}
			entry->setPosition(position, (position - mypos).magVec(), avatarp, first);
			if (avatarp && avatarp->isTyping()) entry->setActivity(LLAvatarListEntry::ACTIVITY_TYPING);
		}
		if (gAudiop)
			for (LLAudioEngine::source_map::iterator i = gAudiop->mAllSources.begin(); i != gAudiop->mAllSources.end(); ++i)
				if (LLAvatarListEntry* entry = getAvatarEntry((i->second)->getOwnerID()))
					entry->setActivity(LLAvatarListEntry::ACTIVITY_SOUND);
		if (announce && !announce_keys.empty())
		{
			std::ostringstream ids;
			U32 transact_num = gFrameCount;
			U32 num_ids = 0;
			while(!announce_keys.empty())
			{
				ids << ',' << announce_keys.front().asString();
				++num_ids;
				if (ids.tellp() > 200)
				{
					send_keys_message(transact_num, num_ids, ids.str());
					ids.seekp(num_ids = 0);
					ids.str(LLStringUtil::null);
				}
				announce_keys.pop();
			}
			if (num_ids) send_keys_message(transact_num, num_ids, ids.str());
		}
	}
}
void LLFloaterAvatarList::expireAvatarList(const std::list<LLUUID>& ids)
{
	if (!ids.empty())
	{
		uuid_vec_t existing_avs;
		std::vector<LLViewerRegion*> neighbors;
		gAgent.getRegion()->getNeighboringRegions(neighbors);
		for (const LLViewerRegion* region : neighbors)
			existing_avs.insert(existing_avs.end(), region->mMapAvatarIDs.begin(), region->mMapAvatarIDs.end());
		for (const LLUUID& id : ids)
		{
			if (std::find(existing_avs.begin(), existing_avs.end(), id) != existing_avs.end()) continue;
			av_list_t::iterator it(std::find_if(mAvatars.begin(), mAvatars.end(), LLAvatarListEntry::uuidMatch(id)));
			if (it != mAvatars.end())
				mAvatars.erase(it);
		}
	}
	refreshAvatarList();
	refreshTracker();
}
void LLFloaterAvatarList::updateAvatarSorting()
{
	if (mDirtyAvatarSorting)
	{
		mDirtyAvatarSorting = false;
		if (mAvatars.size() <= 1) return;
		const std::vector<LLScrollListItem*> list = mAvatarList->getAllData();
		av_list_t::iterator insert_it = mAvatars.begin();
		for(std::vector<LLScrollListItem*>::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			av_list_t::iterator av_it = std::find_if(mAvatars.begin(),mAvatars.end(),LLAvatarListEntry::uuidMatch((*it)->getUUID()));
			if (av_it == mAvatars.end()) continue;
			std::iter_swap(insert_it++, av_it);
			if (insert_it+1 == mAvatars.end()) return;
		}
	}
}
static bool avatar_matches_filter(const LLAvatarListEntry& entry, const std::string& filter)
{
	if (filter.empty()) return true;
	auto contains = [&filter](const std::string& text)
	{
		return utf8str_tolower(text).find(filter) != std::string::npos;
	};
	if (contains(entry.getName())) return true;
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(entry.getID(), &av_name))
	{
		if (contains(av_name.getDisplayName())) return true;
		if (contains(av_name.getUserName())) return true;
		if (contains(av_name.getAccountName())) return true;
		if (contains(av_name.getLegacyName())) return true;
	}
	return false;
}
bool getCustomColorRLV(const LLUUID& id, LLColor4& color, LLViewerRegion* parent_estate, bool name_restricted);
void LLFloaterAvatarList::refreshAvatarList()
{
	if (!getVisible()) return;
	uuid_vec_t selected = mAvatarList->getSelectedIDs();
	S32 scrollpos = mAvatarList->getScrollPos();
	mAvatarList->deleteAllItems();
	LLVector3d mypos = gAgent.getPositionGlobal();
	LLVector3d posagent;
	posagent.setVec(gAgent.getPositionAgent());
	LLVector3d simpos = mypos - posagent;
	const S32 width(gAgent.getRegion() ? gAgent.getRegion()->getWidth() : 256);
	LLSpeakerMgr& speakermgr = LLActiveSpeakerMgr::instance();
	LLRect screen_rect;
	localRectToScreen(getLocalRect(), &screen_rect);
	speakermgr.update(!(screen_rect.pointInRect(gViewerWindow->getCurrentMouseX(), gViewerWindow->getCurrentMouseY()) && gMouseIdleTimer.getElapsedTimeF32() < 5.f));
	av_list_t dead_entries;
	bool name_restricted(gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS) || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES));
	for (auto& entry : mAvatars)
	{
		LLVector3d position = entry->getPosition();
		LLVector3d delta = position - mypos;
		bool UnknownAltitude = position.mdV[VZ] == (gHippoGridManager->getConnectedGrid()->isSecondLife() ? 1020.f : 0.f);
		F32 distance = UnknownAltitude ? 9000.0f : (F32)delta.magVec();
		delta.mdV[2] = 0.0f;
		if (delta.magVec() > 2048.0)
		{
			dead_entries.push_back(entry);
			continue;
		}
		if (!avatar_matches_filter(*entry, mFilterString))
		{
			continue;
		}
		entry->setInList();
		const LLUUID& av_id = entry->getID();
		LLScrollListItem::Params element;
		element.value = av_id;
		static const LLCachedControl<bool> hide_mark("RadarColumnMarkHidden");
		if (!hide_mark)
		{
			LLScrollListCell::Params mark;
			mark.column = "marked";
			mark.type = "text";
			if (entry->isMarked())
			{
				mark.value = "X";
				mark.color = *mm_getMarkerColor(av_id);
				mark.font_style = "BOLD";
			}
			element.columns.add(mark);
		}
		static const LLCachedControl<LLColor4> unselected_color(gColors, "ScrollUnselectedColor", LLColor4(0.f, 0.f, 0.f, 0.8f));
		static const LLCachedControl<LLColor4> sDefaultListText(gColors, "DefaultListText");
		static const LLCachedControl<LLColor4> ascent_muted_color("AscentMutedColor", LLColor4(0.7f,0.7f,0.7f,1.f));
		LLColor4 color = sDefaultListText;
		{
			LLScrollListCell::Params name;
			name.column = "avatar_name";
			name.type = "text";
			name.value = entry->getName();
			if (entry->isFocused())
			{
				name.font_style = "BOLD";
			}
			getCustomColorRLV(av_id, color, LLWorld::getInstance()->getRegionFromPosGlobal(entry->getPosition()), name_restricted);
			name.color = color*0.5f + unselected_color*0.5f;
			element.columns.add(name);
		}
		char temp[32];
		{
			color = sDefaultListText;
			LLScrollListCell::Params dist;
			dist.column = "distance";
			dist.type = "text";
			static const LLCachedControl<LLColor4> sRadarTextDrawDist(gColors, "RadarTextDrawDist");
			if (UnknownAltitude)
			{
				strcpy(temp, "?");
				if (entry->mStats[STAT_TYPE_DRAW])
				{
					color = sRadarTextDrawDist;
				}
			}
			else
			{
				if (distance <= LFSimFeatureHandler::getInstance()->shoutRange())
				{
					static const LLCachedControl<LLColor4> sRadarTextChatRange(gColors, "RadarTextChatRange");
					static const LLCachedControl<LLColor4> sRadarTextShoutRange(gColors, "RadarTextShoutRange");
					snprintf(temp, sizeof(temp), "%.1f", distance);
					color = (distance > LFSimFeatureHandler::getInstance()->sayRange()) ? sRadarTextShoutRange : sRadarTextChatRange;
				}
				else
				{
					if (entry->mStats[STAT_TYPE_DRAW]) color = sRadarTextDrawDist;
					snprintf(temp, sizeof(temp), "%d", (S32)distance);
				}
			}
			dist.value = temp;
			dist.color = color * 0.7f + unselected_color * 0.3f;
			element.columns.add(dist);
		}
		static const LLCachedControl<bool> hide_pos("RadarColumnPositionHidden");
		if (!hide_pos)
		{
			LLScrollListCell::Params pos;
			position -= simpos;
			S32 x(position.mdV[VX]);
			S32 y(position.mdV[VY]);
			if (x >= 0 && x <= width && y >= 0 && y <= width)
			{
				snprintf(temp, sizeof(temp), "%d, %d", x, y);
			}
			else
			{
				temp[0] = '\0';
				if (y < 0)
				{
					strcat(temp, "S");
				}
				else if (y > width)
				{
					strcat(temp, "N");
				}
				if (x < 0)
				{
					strcat(temp, "W");
				}
				else if (x > width)
				{
					strcat(temp, "E");
				}
			}
			pos.column = "position";
			pos.type = "text";
			pos.value = temp;
			element.columns.add(pos);
		}
		static const LLCachedControl<bool> hide_alt("RadarColumnAltitudeHidden");
		if (!hide_alt)
		{
			LLScrollListCell::Params alt;
			alt.column = "altitude";
			alt.type = "text";
			if (UnknownAltitude)
			{
				strcpy(temp, "?");
			}
			else
			{
				snprintf(temp, sizeof(temp), "%d", (S32)position.mdV[VZ]);
			}
			alt.value = temp;
			element.columns.add(alt);
		}
		static const LLCachedControl<bool> hide_act("RadarColumnActivityHidden");
		if (!hide_act)
		{
			LLScrollListCell::Params act;
			act.column = "activity";
			act.type = "icon";
			switch(entry->getActivity())
			{
			case LLAvatarListEntry::ACTIVITY_MOVING:
				act.value = "inv_item_animation.tga";
				act.tool_tip = getString("Moving");
				break;
			case LLAvatarListEntry::ACTIVITY_GESTURING:
				act.value = "inv_item_gesture.tga";
				act.tool_tip = getString("Playing a gesture");
				break;
			case LLAvatarListEntry::ACTIVITY_SOUND:
				act.value = "inv_item_sound.tga";
				act.tool_tip = getString("Playing a sound");
				break;
			case LLAvatarListEntry::ACTIVITY_REZZING:
				act.value = "ff_edit_theirs.tga";
				act.tool_tip = getString("Rezzing objects");
				break;
			case LLAvatarListEntry::ACTIVITY_PARTICLES:
				act.value = "particles_scan.tga";
				act.tool_tip = getString("Creating particles");
				break;
			case LLAvatarListEntry::ACTIVITY_NEW:
				act.value = "avatar_new.tga";
				act.tool_tip = getString("Just arrived");
				break;
			case LLAvatarListEntry::ACTIVITY_TYPING:
				act.value = "avatar_typing.tga";
				act.tool_tip = getString("Typing");
				break;
			default:
				break;
			}
			element.columns.add(act);
		}
		static const LLCachedControl<bool> hide_voice("RadarColumnVoiceHidden");
		if (!hide_voice)
		{
			LLScrollListCell::Params voice;
			voice.column("voice");
			voice.type("icon");
			if (LLPointer<LLSpeaker> speakerp = speakermgr.findSpeaker(av_id))
			{
				if (speakerp->mStatus == LLSpeaker::STATUS_MUTED)
				{
					voice.value("mute_icon.tga");
					voice.color(speakerp->mModeratorMutedVoice ? ascent_muted_color : LLColor4(1.f, 71.f / 255.f, 71.f / 255.f, 1.f));
				}
				else
				{
					switch(llmin(2, llfloor((speakerp->mSpeechVolume / LLVoiceClient::OVERDRIVEN_POWER_LEVEL) * 3.f)))
					{
						case 0:
							voice.value("icn_active-speakers-dot-lvl0.tga");
							break;
						case 1:
							voice.value("icn_active-speakers-dot-lvl1.tga");
							break;
						case 2:
							voice.value("icn_active-speakers-dot-lvl2.tga");
							break;
					}
					voice.color(speakerp->mStatus > LLSpeaker::STATUS_VOICE_ACTIVE ? LLColor4::transparent : speakerp->mDotColor);
				}
			}
			element.columns.add(voice);
		}
		static const LLCachedControl<bool> hide_notes("RadarColumnNotesHidden");
		if (!hide_notes)
		{
			element.columns.add(LLScrollListCell::Params().column("notes").type("checkbox").enabled(false).value(entry->mNotes));
		}
		static const LLCachedControl<bool> hide_age("RadarColumnAgeHidden");
		if (!hide_age)
		{
			LLScrollListCell::Params agep;
			agep.column = "age";
			agep.type = "text";
			color = sDefaultListText;
			bool age_set(entry->mAge > -1);
			if (age_set)
			{
				static const LLCachedControl<U32> sAvatarAgeAlertDays(gSavedSettings, "AvatarAgeAlertDays");
				if ((U32)entry->mAge < sAvatarAgeAlertDays)
				{
					static const LLCachedControl<LLColor4> sRadarTextYoung(gColors, "RadarTextYoung");
					color = sRadarTextYoung;
				}
			}
			agep.value = age_set ? fmt::to_string(entry->mAge) : "?";
			agep.color = color;
			element.columns.add(agep);
		}
		static const LLCachedControl<bool> hide_time("RadarColumnTimeHidden");
		if (!hide_time)
		{
			int dur = difftime(time(NULL), entry->getTime());
			int hours = dur / 3600;
			int mins = (dur % 3600) / 60;
			int secs = (dur % 3600) % 60;
			LLScrollListCell::Params time;
			time.column = "time";
			time.type = "text";
			time.value = llformat("%d:%02d:%02d", hours, mins, secs);
			element.columns.add(time);
		}
		static const LLCachedControl<bool> hide_client("RadarColumnClientHidden");
		if (!hide_client)
		{
			LLScrollListCell::Params viewer;
			viewer.column = "client";
			viewer.type = "text";
			static const LLCachedControl<LLColor4> avatar_name_color(gColors, "AvatarNameColor",LLColor4(0.98f, 0.69f, 0.36f, 1.f));
			color = avatar_name_color;
			if (LLVOAvatar* avatarp = gObjectList.findAvatar(av_id))
			{
				std::string client = SHClientTagMgr::instance().getClientName(avatarp, false);
				if (client.empty())
				{
					color = unselected_color;
					client = "?";
				}
				else SHClientTagMgr::instance().getClientColor(avatarp, false, color);
				viewer.value = client.c_str();
			}
			else
			{
				viewer.value = getString("Out Of Range");
			}
			viewer.color = color *.5f + unselected_color * .5f;
			element.columns.add(viewer);
		}
		mAvatarList->addRow(element);
	}
	for (auto& dead : dead_entries)
		mAvatars.erase(std::remove(mAvatars.begin(), mAvatars.end(), dead), mAvatars.end());
	if (mAvatars.empty())
		setTitle(getString("Title"));
	else if (mAvatars.size() == 1)
		setTitle(getString("TitleOneAvatar"));
	else
	{
		LLStringUtil::format_map_t args;
		args["[COUNT]"] = fmt::to_string(mAvatars.size());
		setTitle(getString("TitleWithCount", args));
	}
	mAvatarList->updateSort();
	mAvatarList->selectMultiple(selected);
	mAvatarList->setScrollPos(scrollpos);
	mDirtyAvatarSorting = true;
}
void LLFloaterAvatarList::resetAvatarNames()
{
	bool hide_tags(gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS));
	bool anon_names(gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES));
	const std::string& hidden(RlvStrings::getString(RLV_STRING_HIDDEN));
	for (auto& entry : mAvatars)
	{
		entry->resetName(hide_tags, anon_names, hidden);
	}
	getChildView("profile_btn")->setEnabled(!hide_tags && !anon_names);
	getChildView("im_btn")->setEnabled(!hide_tags && !anon_names);
}
void LLFloaterAvatarList::onClickIM()
{
	const uuid_vec_t ids = mAvatarList->getSelectedIDs();
	if (!ids.empty())
	{
		if (ids.size() == 1)
		{
			LLAvatarActions::startIM(ids[0]);
		}
		else
		{
			LLAvatarActions::startConference(ids);
		}
	}
}
void LLFloaterAvatarList::onClickTeleportOffer()
{
	uuid_vec_t ids = mAvatarList->getSelectedIDs();
	if (ids.size() > 0)
	{
		handle_lure(ids);
	}
}
void LLFloaterAvatarList::onClickTrack()
{
	LLScrollListItem* item = mAvatarList->getFirstSelected();
	if (!item) return;
	trackAvatar(item->getUUID());
}
void LLFloaterAvatarList::trackAvatar(const LLUUID& agent_id)
{
	if (mTracking && mTrackedAvatar == agent_id)
	{
		LLTracker::stopTracking(false);
		mTracking = false;
	}
	else
	{
		mTracking = true;
		mTrackedAvatar = agent_id;
		trackAvatar(getAvatarEntry(mTrackedAvatar));
	}
}
void LLFloaterAvatarList::refreshTracker()
{
	if (!mTracking) return;
	if (LLTracker::isTracking())
	{
		if(LLAvatarListEntry* entry = getAvatarEntry(mTrackedAvatar))
		{
			if (entry->getPosition() != LLTracker::getTrackedPositionGlobal())
			{
				trackAvatar(entry);
			}
		}
	}
	else
	{
		LLTracker::stopTracking(false);
		mTracking = false;
	}
}
void LLFloaterAvatarList::trackAvatar(const LLAvatarListEntry* entry) const
{
	if (!entry) return;
	std::string name = entry->getName();
	if (!mUpdate) name += "\n(last known position)";
	LLTracker::trackLocation(entry->getPosition(), name, name);
}
LLAvatarListEntry* LLFloaterAvatarList::getAvatarEntry(const LLUUID& avatar) const
{
	av_list_t::const_iterator iter = std::find_if(mAvatars.begin(),mAvatars.end(),LLAvatarListEntry::uuidMatch(avatar));
	return (iter != mAvatars.end()) ? iter->get() : NULL;
}
void LLFloaterAvatarList::onFilterEdit(const std::string& search_string)
{
	mFilterString = utf8str_tolower(search_string);
	LLStringUtil::trim(mFilterString);
	refreshAvatarList();
}
BOOL LLFloaterAvatarList::handleKeyHere(KEY key, MASK mask)
{
	if (LLView* filter = getChildView("filter_input", TRUE, FALSE))
	{
		if (filter->hasFocus())
		{
			return FALSE;
		}
	}
	if (const LLScrollListItem* item = mAvatarList->getFirstSelected())
	{
		LLUUID agent_id = item->getUUID();
		if (KEY_RETURN == key)
		{
			if (MASK_NONE == mask)
			{
				setFocusAvatar(agent_id);
				return true;
			}
			if (MASK_CONTROL == mask)
			{
				if (const LLAvatarListEntry* entry = getAvatarEntry(agent_id))
				{
					gAgent.teleportViaLocation(entry->getPosition());
				}
				return true;
			}
			if (MASK_SHIFT == mask)
			{
				if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS) || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
				{
					make_ui_sound("UISndInvalidOp");
					return true;
				}
				onClickIM();
				return true;
			}
		}
	}
	return LLPanel::handleKeyHere(key, mask);
}
void LLFloaterAvatarList::onClickFocus()
{
	if (LLScrollListItem* item = mAvatarList->getFirstSelected())
	{
		setFocusAvatar(item->getUUID());
	}
}
void LLFloaterAvatarList::removeFocusFromAll()
{
	for (auto& entry : mAvatars)
	{
		entry->setFocus(false);
	}
}
void LLFloaterAvatarList::setFocusAvatar(const LLUUID& id)
{
	if ( !lookAtAvatar(id)) return;
	if (auto inst = getIfExists())
		inst->setFocusAvatarInternal(id);
}
void LLFloaterAvatarList::setFocusAvatarInternal(const LLUUID& id)
{
	av_list_t::iterator iter = std::find_if(mAvatars.begin(),mAvatars.end(),LLAvatarListEntry::uuidMatch(id));
	if (iter == mAvatars.end()) return;
	removeFocusFromAll();
	(*iter)->setFocus(true);
}
template<typename T>
T prev_iter(const T& cur, const T& begin, const T& end)
{
	return ((cur == begin) ? end : cur) - 1;
}
template<typename T>
void decrement_focus_target(T begin, const T& end, bool marked_only)
{
	for (T iter = begin; iter != end; ++iter)
	{
		LLAvatarListEntry& old = *(iter->get());
		if (!old.isFocused()) continue;
		for (T prev = prev_iter(iter, begin, end); prev != iter; prev = prev_iter(prev, begin, end))
		{
			LLAvatarListEntry& entry = *(prev->get());
			if (!entry.isInList()) continue;
			if (marked_only && !entry.isMarked()) continue;
			if (gAgentCamera.lookAtObject(entry.getID(), false))
			{
				old.setFocus(false);
				entry.setFocus(true);
				return;
			}
		}
		break;
	}
}
void LLFloaterAvatarList::focusOnPrev(bool marked_only)
{
	updateAvatarSorting();
	decrement_focus_target(mAvatars.begin(), mAvatars.end(), marked_only);
}
void LLFloaterAvatarList::focusOnNext(bool marked_only)
{
	updateAvatarSorting();
	decrement_focus_target(mAvatars.rbegin(), mAvatars.rend(), marked_only);
}
bool LLFloaterAvatarList::lookAtAvatar(const LLUUID& uuid)
{
	LLVOAvatar* voavatar = gObjectList.findAvatar(uuid);
	if (voavatar && voavatar->isAvatar())
	{
		gAgentCamera.setFocusOnAvatar(false, false);
		gAgentCamera.changeCameraToThirdPerson();
		gAgentCamera.setFocusGlobal(voavatar->getPositionGlobal(),uuid);
		gAgentCamera.setCameraPosAndFocusGlobal(voavatar->getPositionGlobal()
				+ LLVector3d(3.5, 1.35, 0.75) * voavatar->getRotation(),
												voavatar->getPositionGlobal(),
												uuid);
		return true;
	}
	return false;
}
void LLFloaterAvatarList::onClickGetKey()
{
	if (LLScrollListItem* item = mAvatarList->getFirstSelected())
	{
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(item->getUUID().asString()));
	}
}
void LLFloaterAvatarList::sendKeys() const
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp) return;
	static U32 last_transact_num = 0;
	U32 transact_num(gFrameCount);
	if (transact_num > last_transact_num)
	{
		last_transact_num = transact_num;
	}
	else
	{
		return;
	}
	std::ostringstream ids;
	U32 num_ids = 0;
	for (U32 i = 0; i < regionp->mMapAvatarIDs.size(); ++i)
	{
		ids << ',' << regionp->mMapAvatarIDs.at(i);
		++num_ids;
		if (ids.tellp() > 200)
		{
			send_keys_message(transact_num, num_ids, ids.str());
			ids.seekp(num_ids = 0);
			ids.str(LLStringUtil::null);
		}
	}
	if (num_ids > 0) send_keys_message(transact_num, num_ids, ids.str());
}
void LLFloaterAvatarList::sound_trigger_hook(LLMessageSystem* msg,void **)
{
	if (!LLFloaterAvatarList::instanceExists()) return;
	LLUUID owner_id;
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_OwnerID, owner_id);
	if (owner_id != gAgentID) return;
	LLUUID sound_id;
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_SoundID, sound_id);
	if (sound_id == LLUUID("76c78607-93f9-f55a-5238-e19b1a181389"))
	{
		static LLCachedControl<bool> on("RadarChatKeys");
		static LLCachedControl<bool> do_not_ask("RadarChatKeysStopAsking");
		if (on)
			LLFloaterAvatarList::getInstance()->sendKeys();
		else if (!do_not_ask)
			LLNotificationsUtil::add("RadarChatKeysRequest", LLSD(), LLSD(), onConfirmRadarChatKeys);
	}
}
bool LLFloaterAvatarList::onConfirmRadarChatKeys(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 1)
	{
		return false;
	}
	else if (option == 0)
	{
		gSavedSettings.setBOOL("RadarChatKeys", true);
		LLFloaterAvatarList::getInstance()->sendKeys();
	}
	else if (option == 2)
	{
		gSavedSettings.setBOOL("RadarChatKeysStopAsking", true);
	}
	return false;
}
void send_freeze(const LLUUID& avatar_id, bool freeze)
{
	LLVOAvatar* avatarp = gObjectList.findAvatar(avatar_id);
	if (!avatarp) return;
	if (LLViewerRegion* region = avatarp->getRegion())
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("FreezeUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgentID);
		msg->addUUID("SessionID", gAgentSessionID);
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id);
		U32 flags = 0x0;
		if (!freeze)
		{
			flags |= 0x1;
		}
		msg->addU32("Flags", flags);
		msg->sendReliable(region->getHost());
	}
}
void send_eject(const LLUUID& avatar_id, bool ban)
{
	LLVOAvatar* avatarp = gObjectList.findAvatar(avatar_id);
	if (!avatarp) return;
	if (LLViewerRegion* region = avatarp->getRegion())
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("EjectUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgentID);
		msg->addUUID("SessionID", gAgentSessionID);
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id);
		U32 flags = 0x0;
		if (ban)
		{
			flags |= 0x1;
		}
		msg->addU32("Flags", flags);
		msg->sendReliable(region->getHost());
	}
}
void send_estate_message(const std::string request, const std::vector<std::string>& strings);
static void cmd_append_names(const LLAvatarListEntry* entry, std::string &str, std::string &sep)
															{ if(!str.empty())str.append(sep);str.append(entry->getName()); }
static void cmd_ar(const LLAvatarListEntry* entry)			{ LLFloaterReporter::showFromObject(entry->getID()); }
static void cmd_profile(const LLAvatarListEntry* entry)		{ LLAvatarActions::showProfile(entry->getID()); }
static void cmd_teleport(const LLAvatarListEntry* entry)	{ gAgent.teleportViaLocation(entry->getPosition()); }
static void cmd_freeze(const LLAvatarListEntry* entry)		{ send_freeze(entry->getID(), true); }
static void cmd_unfreeze(const LLAvatarListEntry* entry)	{ send_freeze(entry->getID(), false); }
static void cmd_eject(const LLAvatarListEntry* entry)		{ send_eject(entry->getID(), false); }
static void cmd_ban(const LLAvatarListEntry* entry)			{ send_eject(entry->getID(), true); }
static void cmd_estate_eject(const LLAvatarListEntry* entry){ send_estate_message("kickestate", {entry->getID().asString()}); }
static void cmd_estate_tp_home(const LLAvatarListEntry* entry){ send_estate_message("teleporthomeuser", {gAgentID.asString(), entry->getID().asString()}); }
static void cmd_estate_ban(const LLAvatarListEntry* entry)	{ LLPanelEstateAccess::sendEstateAccessDelta(ESTATE_ACCESS_BANNED_AGENT_ADD | ESTATE_ACCESS_ALLOWED_AGENT_REMOVE | ESTATE_ACCESS_NO_REPLY, entry->getID()); }
void LLFloaterAvatarList::doCommand(avlist_command_t func, bool single) const
{
	uuid_vec_t ids;
	if (!single)
		ids = mAvatarList->getSelectedIDs();
	else
		ids.push_back(getSelectedID());
	for (uuid_vec_t::const_iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		const LLUUID& avid = *itr;
		if (avid.isNull()) continue;
		if (LLAvatarListEntry* entry = getAvatarEntry(avid))
		{
			LL_INFOS() << "Executing command on " << entry->getName() << LL_ENDL;
			func(entry);
		}
	}
}
std::string LLFloaterAvatarList::getSelectedNames(const std::string& separator) const
{
	std::string ret;
	doCommand(boost::bind(&cmd_append_names,_1,boost::ref(ret),separator));
	return ret;
}
std::string LLFloaterAvatarList::getSelectedName() const
{
	LLAvatarListEntry* entry = getAvatarEntry(getSelectedID());
	return entry ? entry->getName() : LLStringUtil::null;
}
LLUUID LLFloaterAvatarList::getSelectedID() const
{
	LLScrollListItem* item = mAvatarList->getFirstSelected();
	return item ? item->getUUID() : LLUUID::null;
}
uuid_vec_t LLFloaterAvatarList::getSelectedIDs() const
{
	return mAvatarList->getSelectedIDs();
}
void LLFloaterAvatarList::callbackFreeze(const LLSD& notification, const LLSD& response)
{
	if (!instanceExists()) return;
	LLFloaterAvatarList& inst(instance());
	S32 option = LLNotification::getSelectedOption(notification, response);
	if		(option == 0)	inst.doCommand(cmd_freeze);
	else if	(option == 1)	inst.doCommand(cmd_unfreeze);
}
void LLFloaterAvatarList::callbackEject(const LLSD& notification, const LLSD& response)
{
	if (!instanceExists()) return;
	LLFloaterAvatarList& inst(instance());
	S32 option = LLNotification::getSelectedOption(notification, response);
	if		(option == 0)	inst.doCommand(cmd_eject);
	else if	(option == 1)	inst.doCommand(cmd_ban);
}
void LLFloaterAvatarList::callbackEjectFromEstate(const LLSD& notification, const LLSD& response)
{
	if (!instanceExists()) return;
	LLFloaterAvatarList& inst(instance());
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option != 2)
		inst.doCommand(option ? cmd_estate_tp_home : cmd_estate_eject);
}
void LLFloaterAvatarList::callbackBanFromEstate(const LLSD& notification, const LLSD& response)
{
	if (!instanceExists()) return;
	LLFloaterAvatarList& inst(instance());
	if (!LLNotification::getSelectedOption(notification, response))
	{
		inst.doCommand(cmd_estate_eject);
		inst.doCommand(cmd_estate_ban);
	}
}
void LLFloaterAvatarList::onClickFreeze()
{
	LLSD args;
	args["AVATAR_NAME"] = getSelectedNames();
	LLNotificationsUtil::add("FreezeAvatarFullname", args, LLSD(), callbackFreeze);
}
void LLFloaterAvatarList::onClickEject()
{
	LLSD args;
	args["AVATAR_NAME"] = getSelectedNames();
	LLNotificationsUtil::add("EjectAvatarFullname", args, LLSD(), callbackEject);
}
void LLFloaterAvatarList::onClickMute()
{
	uuid_vec_t ids = mAvatarList->getSelectedIDs();
	for (uuid_vec_t::const_iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		const LLUUID& agent_id = *itr;
		std::string agent_name;
		if (gCacheName->getFullName(agent_id, agent_name))
		{
			LLMute mute(agent_id, agent_name, LLMute::AGENT);
			LLMuteList::getInstance()->isMuted(agent_id) ? LLMuteList::getInstance()->remove(mute) : LLMuteList::getInstance()->add(mute);
		}
	}
}
void LLFloaterAvatarList::onClickEjectFromEstate()
{
	LLSD args;
	args["EVIL_USER"] = getSelectedNames();
	LLNotificationsUtil::add("EstateKickUser", args, LLSD(), callbackEjectFromEstate);
}
void LLFloaterAvatarList::onClickBanFromEstate()
{
	LLSD args;
	LLSD payload;
	args["EVIL_USER"] = getSelectedNames();
	LLNotificationsUtil::add("EstateBanUser", args, LLSD(), callbackBanFromEstate);
}
void LLFloaterAvatarList::onSelectName()
{
	if (LLScrollListItem* item = mAvatarList->getFirstSelected())
	{
		if (LLAvatarListEntry* entry = getAvatarEntry(item->getUUID()))
		{
			bool enabled = entry->mStats[STAT_TYPE_DRAW];
			childSetEnabled("focus_btn", enabled);
			childSetEnabled("prev_in_list_btn", enabled);
			childSetEnabled("next_in_list_btn", enabled);
		}
	}
}
