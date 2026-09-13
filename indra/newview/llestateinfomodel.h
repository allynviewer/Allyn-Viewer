/** 
 * @file llestateinfomodel.h
 * @brief Estate info model
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#ifndef LL_LLESTATEINFOMODEL_H
#define LL_LLESTATEINFOMODEL_H
class LLMessageSystem;
#include "llsingleton.h"
class LLEstateInfoModel : public LLSingleton<LLEstateInfoModel>
{
	LOG_CLASS(LLEstateInfoModel);
public:
	typedef boost::signals2::signal<void()> update_signal_t;
	boost::signals2::connection setUpdateCallback(const update_signal_t::slot_type& cb);
	boost::signals2::connection setCommitCallback(const update_signal_t::slot_type& cb);
	void sendEstateInfo();
	bool				getUseFixedSun()			const;
	bool				getIsExternallyVisible()	const;
	bool				getAllowDirectTeleport()	const;
	bool				getDenyAnonymous()			const;
	bool				getDenyAgeUnverified()		const;
	bool				getAllowVoiceChat()			const;
	const std::string&	getName()					const { return mName; }
	const LLUUID&		getOwnerID()				const { return mOwnerID; }
	U32					getID()						const { return mID; }
	F32					getSunHour()				const { return mSunHour; }
	bool				getGlobalTime()				const { return !(mSunHour || getUseFixedSun()); }
	void setUseFixedSun(bool val);
	void setIsExternallyVisible(bool val);
	void setAllowDirectTeleport(bool val);
	void setDenyAnonymous(bool val);
	void setDenyAgeUnverified(bool val);
	void setAllowVoiceChat(bool val);
	void setSunHour(F32 sun_hour) { mSunHour = sun_hour; }
protected:
	typedef std::vector<std::string> strings_t;
	friend class LLSingleton<LLEstateInfoModel>;
	friend class LLDispatchEstateUpdateInfo;
	friend class LLEstateChangeInfoResponder;
	LLEstateInfoModel();
	void update(const strings_t& strings);
	void notifyCommit();
private:
	bool commitEstateInfoCaps();
	void commitEstateInfoDataserver();
	inline bool getFlag(U64 flag) const;
	inline void setFlag(U64 flag, bool val);
	U64  getFlags() const { return mFlags; }
	std::string getInfoDump();
	std::string	mName;
	LLUUID		mOwnerID;
	U32			mID;
	U64			mFlags;
	F32			mSunHour;
	update_signal_t mUpdateSignal;
	update_signal_t mCommitSignal;
};
inline bool LLEstateInfoModel::getFlag(U64 flag) const
{
	return ((mFlags & flag) != 0);
}
inline void LLEstateInfoModel::setFlag(U64 flag, bool val)
{
	if (val)
	{
		mFlags |= flag;
	}
	else
	{
		mFlags &= ~flag;
	}
}
#endif
