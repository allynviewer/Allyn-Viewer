/**
 * @file llenvmanager.h
 * @brief Declaration of classes managing WindLight and water settings.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#ifndef LL_LLENVMANAGER_H
#define LL_LLENVMANAGER_H
#include "llmemory.h"
#include "llsd.h"
class LLWLParamManager;
class LLWaterParamManager;
class LLWLAnimator;
struct LLEnvKey
{
public:
	typedef enum e_scope
	{
		SCOPE_LOCAL,
		SCOPE_REGION
	} EScope;
};
class LLEnvironmentSettings
{
public:
	LLEnvironmentSettings() :
		mWLDayCycle(LLSD::emptyMap()),
		mSkyMap(LLSD::emptyMap()),
		mWaterParams(LLSD::emptyMap()),
		mDayTime(0.f)
	{}
	LLEnvironmentSettings(const LLSD& dayCycle, const LLSD& skyMap, const LLSD& waterParams, F64 dayTime) :
		mWLDayCycle(dayCycle),
		mSkyMap(skyMap),
		mWaterParams(waterParams),
		mDayTime(dayTime)
	{}
	~LLEnvironmentSettings() {}
	void saveParams(const LLSD& dayCycle, const LLSD& skyMap, const LLSD& waterParams, F64 dayTime)
	{
		mWLDayCycle = dayCycle;
		mSkyMap = skyMap;
		mWaterParams = waterParams;
		mDayTime = dayTime;
	}
	const LLSD& getWLDayCycle() const
	{
		return mWLDayCycle;
	}
	const LLSD& getWaterParams() const
	{
		return mWaterParams;
	}
	const LLSD& getSkyMap() const
	{
		return mSkyMap;
	}
	F64 getDayTime() const
	{
		return mDayTime;
	}
	bool isEmpty() const
	{
		return mWLDayCycle.size() == 0;
	}
	void clear()
	{
		*this = LLEnvironmentSettings();
	}
	LLSD makePacket(const LLSD& metadata) const
	{
		LLSD full_packet = LLSD::emptyArray();
		full_packet.append(metadata);
		full_packet.append(mWLDayCycle);
		full_packet.append(mSkyMap);
		full_packet.append(mWaterParams);
		return full_packet;
	}
private:
	LLSD mWLDayCycle, mWaterParams, mSkyMap;
	F64 mDayTime;
};
class LLEnvPrefs
{
public:
	LLEnvPrefs() : mUseRegionSettings(true), mUseDayCycle(true) {}
	bool getUseRegionSettings() const { return mUseRegionSettings; }
	bool getUseDayCycle() const { return mUseDayCycle; }
	bool getUseFixedSky() const { return !getUseDayCycle(); }
	std::string getWaterPresetName() const;
	std::string getSkyPresetName() const;
	std::string getDayCycleName() const;
	void setUseRegionSettings(bool val);
	void setUseWaterPreset(const std::string& name);
	void setUseSkyPreset(const std::string& name);
	void setUseDayCycle(const std::string& name);
	bool			mUseRegionSettings;
	bool			mUseDayCycle;
	std::string		mWaterPresetName;
	std::string		mSkyPresetName;
	std::string		mDayCycleName;
};
class LLEnvManagerNew : public LLSingleton<LLEnvManagerNew>
{
	LOG_CLASS(LLEnvManagerNew);
public:
	typedef boost::signals2::signal<void()> prefs_change_signal_t;
	typedef boost::signals2::signal<void()> region_settings_change_signal_t;
	typedef boost::signals2::signal<void(bool)> region_settings_applied_signal_t;
	LLEnvManagerNew();
	bool getUseRegionSettings() const;
	bool getUseDayCycle() const;
	bool getUseFixedSky() const;
	std::string getWaterPresetName() const;
	std::string getSkyPresetName() const;
	std::string getDayCycleName() const;
	const LLEnvironmentSettings& getRegionSettings() const;
	void setRegionSettings(const LLEnvironmentSettings& new_settings);
	bool usePrefs();
	bool useDefaults();
	bool useRegionSettings();
	bool useWaterPreset(const std::string& name);
	bool useWaterParams(const LLSD& params);
	bool useSkyPreset(const std::string& name, bool interpolate = false);
	bool useSkyParams(const LLSD& params);
	bool useDayCycle(const std::string& name, LLEnvKey::EScope scope);
	bool useDayCycleParams(const LLSD& params, LLEnvKey::EScope scope, F32 time = 0.5);
	void setUseRegionSettings(bool val, bool interpolate = false);
	void setUseWaterPreset(const std::string& name, bool interpolate = false);
	void setUseSkyPreset(const std::string& name, bool interpolate = false);
	void setUseDayCycle(const std::string& name, bool interpolate = false);
	void setUserPrefs(
		const std::string& water_preset,
		const std::string& sky_preset,
		const std::string& day_cycle_preset,
		bool use_fixed_sky,
		bool use_region_settings);
	void dumpUserPrefs();
	void dumpPresets();
	void requestRegionSettings();
	bool sendRegionSettings(const LLEnvironmentSettings& new_settings);
	boost::signals2::connection setPreferencesChangeCallback(const prefs_change_signal_t::slot_type& cb);
	boost::signals2::connection setRegionSettingsChangeCallback(const region_settings_change_signal_t::slot_type& cb);
	boost::signals2::connection setRegionSettingsAppliedCallback(const region_settings_applied_signal_t::slot_type& cb);
	static bool canEditRegionSettings();
	static const std::string getScopeString(LLEnvKey::EScope scope);
	void onRegionSettingsResponse(const LLSD& content);
	void onRegionSettingsApplyResponse(bool ok);
private:
	friend class LLSingleton<LLEnvManagerNew>;
	void initSingleton();
	void loadUserPrefs();
	void saveUserPrefs();
	void updateSkyFromPrefs(bool interpolate = false);
	void updateWaterFromPrefs(bool interpolate);
	void updateManagersFromPrefs(bool interpolate);
public:
	bool useRegionSky();
	bool useRegionWater();
private:
	friend class WindLightRefresh;
	bool useDefaultSky();
	bool useDefaultWater();
	void onRegionChange();
	prefs_change_signal_t mUsePrefsChangeSignal;
	region_settings_change_signal_t	mRegionSettingsChangeSignal;
	region_settings_applied_signal_t mRegionSettingsAppliedSignal;
	LLEnvPrefs				mUserPrefs;
	LLEnvironmentSettings	mCachedRegionPrefs;
	LLEnvironmentSettings	mNewRegionPrefs;
	bool					mInterpNextChangeMessage;
	LLUUID					mCurRegionUUID;
	LLUUID					mLastReceivedID;
};
#endif
