//
// C++ Interface: llfloateravatarlist
//
// Description: 
//
//
// Original author: Dale Glass <dale@daleglass.net>, (C) 2007
// Heavily modified by Henri Beauchamp 10/2009.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef LL_LLFLOATERAVATARLIST_H
#define LL_LLFLOATERAVATARLIST_H
#include "llavatarname.h"
#include "llavatarpropertiesprocessor.h"
#include "llfloater.h"
#include "llfloaterreporter.h"
#include "lluuid.h"
#include "lltimer.h"
#include "llscrolllistctrl.h"
#include <time.h>
#include <bitset>
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>
class LLFloaterAvatarList;
enum ERadarStatType
{
	STAT_TYPE_SIM,
	STAT_TYPE_DRAW,
	STAT_TYPE_SHOUTRANGE,
	STAT_TYPE_CHATRANGE,
	STAT_TYPE_AGE,
	STAT_TYPE_SIZE
};
class LLAvatarListEntry : public LLAvatarPropertiesObserver
{
public:
enum ACTIVITY_TYPE
{
	ACTIVITY_NONE,
	ACTIVITY_MOVING,
	ACTIVITY_GESTURING,
	ACTIVITY_REZZING,
	ACTIVITY_PARTICLES,
	ACTIVITY_TYPING,
	ACTIVITY_NEW,
	ACTIVITY_SOUND,
};
	LLAvatarListEntry(const LLUUID& id = LLUUID::null, const std::string &name = "", const LLVector3d &position = LLVector3d::zero);
	~LLAvatarListEntry();
	void processProperties(void* data, EAvatarProcessorType type);
	void setPosition(const LLVector3d& position, const F32& dist, bool drawn, bool flood = false);
	const LLVector3d& getPosition() const { return mPosition; }
	void resetName(const bool& hide_tags, const bool& anon_names, const std::string& hidden);
	const std::string&  getName() const { return mName; }
	const time_t& getTime() const { return mTime; }
	const LLUUID& getID() const { return mID; }
	void setActivity(ACTIVITY_TYPE activity);
	const ACTIVITY_TYPE getActivity();
	void setFocus(bool value) { mFocused = value; }
	bool isFocused() const { return mFocused; }
	bool isMarked() const { return mMarked; }
	void setInList()	{ mIsInList = true; }
	bool isInList() const { return mIsInList; }
	void setMarked(bool marked) { mMarked = marked; }
	struct uuidMatch
	{
		uuidMatch(const LLUUID& id) : mID(id) {}
		bool operator()(const boost::shared_ptr<LLAvatarListEntry>& l) { return l->getID() == mID; }
		LLUUID mID;
	};
private:
	friend class LLFloaterAvatarList;
	LLUUID mID;
	std::string mName;
	time_t mTime;
	LLVector3d mPosition;
	bool mMarked;
	bool mFocused;
	bool mIsInList;
	bool mNotes = false;
	int mAge;
	std::bitset<STAT_TYPE_SIZE> mStats;
	ACTIVITY_TYPE mActivityType;
	LLTimer mActivityTimer;
};
class LLFloaterAvatarList : public LLFloater, public LLSingleton<LLFloaterAvatarList>
{
	friend class LLSingleton<LLFloaterAvatarList>;
private:
	LLFloaterAvatarList();
public:
	~LLFloaterAvatarList();
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	void onClose(bool app_quitting);
	void onOpen();
	BOOL postBuild();
	void draw();
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	static void toggleInstance(const LLSD& = LLSD());
	static void showInstance();
	static bool instanceVisible(const LLSD& = LLSD()) { return instanceExists() && instance().getVisible(); }
	void assessColumns();
	void updateAvatarList(const class LLViewerRegion* region, bool first = false);
	void refreshAvatarList();
	void resetAvatarNames();
	LLAvatarListEntry* getAvatarEntry(const LLUUID& avatar) const;
	std::string getSelectedNames(const std::string& separator = ", ") const;
	std::string getSelectedName() const;
	LLUUID getSelectedID() const;
	uuid_vec_t getSelectedIDs() const;
	static bool lookAtAvatar(const LLUUID& uuid);
	static void sound_trigger_hook(LLMessageSystem* msg,void **);
	void sendKeys() const;
	typedef boost::shared_ptr<LLAvatarListEntry> LLAvatarListEntryPtr;
	typedef std::vector< LLAvatarListEntryPtr > av_list_t;
	typedef boost::function<void (LLAvatarListEntry*)> avlist_command_t;
	void removeFocusFromAll();
	static void setFocusAvatar(const LLUUID& id);
	void focusOnPrev(bool marked_only);
	void focusOnNext(bool marked_only);
	void refreshTracker();
	void trackAvatar(const LLUUID& agent_id);
	void trackAvatar(const LLAvatarListEntry* entry) const;
	void onClickIM();
	void onClickTeleportOffer();
	void onClickTrack();
	void onClickMute();
	void onClickFocus();
	void onClickGetKey();
	void onSelectName();
	void onFilterEdit(const std::string& search_string);
	void onClickFreeze();
	void onClickEject();
	void onClickEjectFromEstate();
	void onClickBanFromEstate();
	void onAvatarSortingChanged() { mDirtyAvatarSorting = true; }
	static void callbackFreeze(const LLSD& notification, const LLSD& response);
	static void callbackEject(const LLSD& notification, const LLSD& response);
	static void callbackEjectFromEstate(const LLSD& notification, const LLSD& response);
	static void callbackBanFromEstate(const LLSD& notification, const LLSD& response);
	static bool onConfirmRadarChatKeys(const LLSD& notification, const LLSD& response );
	void doCommand(avlist_command_t cmd, bool single = false) const;
	void expireAvatarList(const std::list<LLUUID>& ids);
	void updateAvatarSorting();
	static bool isCleanup()
	{
		const auto& inst = getIfExists();
		return inst && inst->mCleanup;
	}
private:
	void setFocusAvatarInternal(const LLUUID& id);
	LLScrollListCtrl*			mAvatarList;
	av_list_t	mAvatars;
	std::string	mFilterString;
	bool		mDirtyAvatarSorting;
	bool		mCleanup = false;
	const LLCachedControl<bool> mUpdate;
	bool mTracking;
	LLUUID mTrackedAvatar;
	LLUUID mFocusedAvatar;
};
#endif
