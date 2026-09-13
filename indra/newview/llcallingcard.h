/** 
 * @file llcallingcard.h
 * @brief Definition of the LLPreviewCallingCard class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LL_LLCALLINGCARD_H
#define LL_LLCALLINGCARD_H
#include <map>
#include <set>
#include <string>
#include <vector>
#include "lluserrelations.h"
#include "lluuid.h"
#include "v3dmath.h"
class LLMessageSystem;
class LLTrackingData;
class LLFriendObserver
{
public:
	enum
	{
		NONE = 0,
		ADD = 1,
		REMOVE = 2,
		ONLINE = 4,
		POWERS = 8,
		ALL = 0xffffffff
	};
	virtual ~LLFriendObserver() {}
	virtual void changed(U32 mask) = 0;
};
class LLRelationshipFunctor
{
public:
	virtual ~LLRelationshipFunctor() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy) = 0;
};
class LLAvatarTracker
{
public:
	static LLAvatarTracker& instance() { return sInstance; }
	void track(const LLUUID& avatar_id, const std::string& name);
	void untrack(const LLUUID& avatar_id);
	bool isTrackedAgentValid() { return mTrackedAgentValid; }
	void setTrackedAgentValid(bool valid) { mTrackedAgentValid = valid; }
	void findAgent();
	void setTrackedCoarseLocation(const LLVector3d& global_pos);
	bool haveTrackingInfo();
	void getDegreesAndDist(F32& rot, F64& horiz_dist, F64& vert_dist);
	LLVector3d getGlobalPos();
	const std::string& getName();
	const LLUUID& getAvatarID();
	typedef std::map<LLUUID, LLRelationship*> buddy_map_t;
	S32 addBuddyList(const buddy_map_t& buddies);
	void copyBuddyList(buddy_map_t& buddies) const;
	void terminateBuddy(const LLUUID& id);
	const LLRelationship* getBuddyInfo(const LLUUID& id) const;
	bool isBuddy(const LLUUID& id) const;
	void setBuddyOnline(const LLUUID& id, bool is_online);
	bool isBuddyOnline(const LLUUID& id) const;
	void setBuddyEmpowered(const LLUUID& id, bool is_empowered);
	bool isBuddyEmpowered(const LLUUID& id) const;
	void empowerList(const buddy_map_t& list, bool grant);
	void empower(const LLUUID& id, bool grant);
	void registerCallbacks(LLMessageSystem* msg);
	void addObserver(LLFriendObserver* observer);
	void removeObserver(LLFriendObserver* observer);
	void notifyObservers();
	void addParticularFriendObserver(const LLUUID& buddy_id, LLFriendObserver* observer);
	void removeParticularFriendObserver(const LLUUID& buddy_id, LLFriendObserver* observer);
	void notifyParticularFriendObservers(const LLUUID& buddy_id);
	void addChangedMask(U32 mask, const LLUUID& referent);
	const uuid_set_t& getChangedIDs() { return mChangedBuddyIDs; }
	void applyFunctor(LLRelationshipFunctor& f);
	static void formFriendship(const LLUUID& friend_id);
	void updateFriends();
protected:
	void deleteTrackingData();
	void agentFound(const LLUUID& prey,
					const LLVector3d& estimated_global_pos);
	static void processAgentFound(LLMessageSystem* msg, void**);
	static void processOnlineNotification(LLMessageSystem* msg, void**);
	static void processOfflineNotification(LLMessageSystem* msg, void**);
	static void processTerminateFriendship(LLMessageSystem* msg, void**);
	static void processChangeUserRights(LLMessageSystem* msg, void**);
	void processNotify(LLMessageSystem* msg, bool online);
	void processChange(LLMessageSystem* msg);
protected:
	static LLAvatarTracker sInstance;
	LLTrackingData* mTrackingData;
	bool mTrackedAgentValid;
	U32 mModifyMask;
	buddy_map_t mBuddyInfo;
	typedef uuid_set_t changed_buddy_t;
	changed_buddy_t mChangedBuddyIDs;
	typedef std::vector<LLFriendObserver*> observer_list_t;
	observer_list_t mObservers;
    typedef std::set<LLFriendObserver*> observer_set_t;
    typedef std::map<LLUUID, observer_set_t> observer_map_t;
    observer_map_t mParticularFriendObserverMap;
private:
	LLAvatarTracker(const LLAvatarTracker&);
	bool operator==(const LLAvatarTracker&);
public:
	LLAvatarTracker();
	~LLAvatarTracker();
};
class LLCollectProxyBuddies : public LLRelationshipFunctor
{
public:
	LLCollectProxyBuddies() {}
	virtual ~LLCollectProxyBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef uuid_set_t buddy_list_t;
	buddy_list_t mProxy;
};
class LLCollectMappableBuddies : public LLRelationshipFunctor
{
public:
	LLCollectMappableBuddies() {}
	virtual ~LLCollectMappableBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef std::map<std::string, LLUUID, LLDictionaryLess> buddy_map_t;
	buddy_map_t mMappable;
	std::string mFullName;
};
class LLCollectOnlineBuddies : public LLRelationshipFunctor
{
public:
	LLCollectOnlineBuddies() {}
	virtual ~LLCollectOnlineBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef std::map<std::string, LLUUID, LLDictionaryLess> buddy_map_t;
	buddy_map_t mOnline;
	std::string mFullName;
};
class LLCollectAllBuddies : public LLRelationshipFunctor
{
public:
	LLCollectAllBuddies() {}
	virtual ~LLCollectAllBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef std::map<std::string, LLUUID, LLDictionaryLess> buddy_map_t;
	buddy_map_t mOnline;
	buddy_map_t mOffline;
	std::string mFullName;
};
#endif
