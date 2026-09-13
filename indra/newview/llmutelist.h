/** 
 * @file llmutelist.h
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
#ifndef LL_MUTELIST_H
#define LL_MUTELIST_H
#include "llstring.h"
#include "lluuid.h"
class LLViewerObject;
class LLMessageSystem;
class LLMuteListObserver;
class LLMute
{
public:
	enum EType { BY_NAME = 0, AGENT = 1, OBJECT = 2, GROUP = 3, EXTERNAL = 4, COUNT = 5 };
	enum
	{
		flagTextChat		= 0x00000001,
		flagVoiceChat		= 0x00000002,
		flagParticles		= 0x00000004,
		flagObjectSounds 	= 0x00000008,
		flagAll				= 0x0000000F
	};
	LLMute(const LLUUID& id, const std::string& name = std::string(), EType type = BY_NAME, U32 flags = 0);
	std::string getDisplayType() const;
public:
	LLUUID		mID;
	std::string	mName;
	EType		mType;
	U32			mFlags;
};
class LLMuteList : public LLSingleton<LLMuteList>
{
public:
	enum EAutoReason
	{
		AR_IM = 0,
		AR_MONEY = 1,
		AR_INVENTORY = 2,
		AR_COUNT
	};
	LLMuteList();
	~LLMuteList();
	static LLMuteList* getInstance();
	void addObserver(LLMuteListObserver* observer);
	void removeObserver(LLMuteListObserver* observer);
	BOOL add(const LLMute& mute, U32 flags = 0);
	BOOL remove(const LLMute& mute, U32 flags = 0);
	BOOL autoRemove(const LLUUID& agent_id, const EAutoReason reason);
	BOOL isMuted(const LLUUID& id, const std::string& name = LLStringUtil::null, U32 flags = 0) const;
	BOOL isMuted(const LLUUID& id, U32 flags) const { return isMuted(id, LLStringUtil::null, flags); };
	BOOL isLinden(const std::string& name) const;
	bool isLinden(const LLUUID& id) const;
	bool hasMute(const LLMute& mute) const { return mMutes.find(mute) != mMutes.end(); }
	BOOL isLoaded() const { return mIsLoaded; }
	std::vector<LLMute> getMutes() const;
	void requestFromServer(const LLUUID& agent_id);
	void cache(const LLUUID& agent_id);
private:
	BOOL loadFromFile(const std::string& filename);
	BOOL saveToFile(const std::string& filename);
	void setLoaded();
	void notifyObservers();
	void notifyObserversDetailed(const LLMute &mute);
	void updateAdd(const LLMute& mute);
	void updateRemove(const LLMute& mute);
	static void processMuteListUpdate(LLMessageSystem* msg, void**);
	static void processUseCachedMuteList(LLMessageSystem* msg, void**);
	static void onFileMuteList(void** user_data, S32 code, LLExtStat ext_status);
private:
	struct compare_by_name
	{
		bool operator()(const LLMute& a, const LLMute& b) const
		{
			std::string name1 = a.mName;
			std::string name2 = b.mName;
			LLStringUtil::toUpper(name1);
			LLStringUtil::toUpper(name2);
			return name1 < name2;
		}
	};
	struct compare_by_id
	{
		bool operator()(const LLMute& a, const LLMute& b) const
		{
			return a.mID < b.mID;
		}
	};
	typedef std::set<LLMute, compare_by_id> mute_set_t;
	mute_set_t mMutes;
	typedef std::set<std::string> string_set_t;
	string_set_t mLegacyMutes;
	typedef std::set<LLMuteListObserver*> observer_set_t;
	observer_set_t mObservers;
	BOOL mIsLoaded;
	friend class LLDispatchEmptyMuteList;
	friend class LFSimFeatureHandler;
	std::set<std::string> mGodLastNames;
	std::set<std::string> mGodFullNames;
};
class LLMuteListObserver
{
public:
	virtual ~LLMuteListObserver() { }
	virtual void onChange() = 0;
	virtual void onChangeDetailed(const LLMute& ) { }
};
#endif
