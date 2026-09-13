/**
 * @file llwlparammanager.h
 * @brief Implementation for the LLWLParamManager class.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#ifndef LL_WLPARAMMANAGER_H
#define LL_WLPARAMMANAGER_H
#include <list>
#include <map>
#include "llenvmanager.h"
#include "llwlparamset.h"
#include "llwlanimator.h"
#include "llwldaycycle.h"
#include "llviewercamera.h"
#include "lltrans.h"
#include "llassettype.h"
class LLVFS;
class LLGLSLShader;
class LLWLAnimator;
struct WLColorControl {
	F32 r, g, b, i;
	std::string mName;
	std::string mSliderName;
	bool hasSliderName;
	bool isSunOrAmbientColor;
	bool isBlueHorizonOrDensity;
	inline WLColorControl(F32 red, F32 green, F32 blue, F32 intensity,
						  const std::string& n, const std::string& sliderName = LLStringUtil::null)
		: r(red), g(green), b(blue), i(intensity), mName(n), mSliderName(sliderName)
	{
		hasSliderName = false;
		if (mSliderName != "") {
			hasSliderName = true;
		}
		isSunOrAmbientColor = false;
		if (mSliderName == "WLSunlight" || mSliderName == "WLAmbient") {
			isSunOrAmbientColor = true;
		}
		isBlueHorizonOrDensity = false;
		if (mSliderName == "WLBlueHorizon" || mSliderName == "WLBlueDensity") {
			isBlueHorizonOrDensity = true;
		}
	}
	inline WLColorControl & operator = (LLVector4 const & val) {
		r = val.mV[0];
		g = val.mV[1];
		b = val.mV[2];
		i = val.mV[3];
		return *this;
	}
	inline operator LLVector4 (void) const {
		return LLVector4(r, g, b, i);
	}
	inline operator LLVector3 (void) const {
		return LLVector3(r, g, b);
	}
	inline void update(LLWLParamSet & params) const {
		params.set(mName, r, g, b, i);
	}
};
struct WLFloatControl {
	F32 x;
	std::string mName;
	F32 mult;
	inline WLFloatControl(F32 val, const std::string& n, F32 m=1.0f)
		: x(val), mName(n), mult(m)
	{
	}
	inline WLFloatControl & operator = (F32 val) {
		x = val;
		return *this;
	}
	inline operator F32 (void) const {
		return x;
	}
	inline void update(LLWLParamSet & params) const {
		params.set(mName, x);
	}
};
class LLWLParamManager : public LLSingleton<LLWLParamManager>
{
	LOG_CLASS(LLWLParamManager);
public:
	typedef std::list<std::string> preset_name_list_t;
	typedef std::list<LLWLParamKey> preset_key_list_t;
	typedef boost::signals2::signal<void()> preset_list_signal_t;
	void updateShaderLinks();
	bool loadPresetXML(const LLWLParamKey& key, std::istream& preset_stream);
	void loadPresetNotecard(const std::string& name, const LLUUID& asset_id, const LLUUID& inv_id);
	void savePreset(const LLWLParamKey key);
	bool savePresetToNotecard(const std::string & name);
	void propagateParameters(void);
	void updateShaderUniforms(LLGLSLShader * shader);
	void resetAnimator(F32 curTime, bool run);
	void update(LLViewerCamera * cam);
	bool applyDayCycleParams(const LLSD& params, LLEnvKey::EScope scope, F32 time = 0.5);
	bool applySkyParams(const LLSD& params, bool interpolate = false);
	inline LLVector4 getLightDir(void) const;
	inline LLVector4 getClampedLightDir(void) const;
	inline LLVector4 getRotatedLightDir(void) const;
	inline F32 getDomeOffset(void) const;
	inline F32 getDomeRadius(void) const;
	bool addParamSet(const LLWLParamKey& key, LLWLParamSet& param);
	BOOL addParamSet(const LLWLParamKey& key, LLSD const & param);
	bool getParamSet(const LLWLParamKey& key, LLWLParamSet& param);
	bool hasParamSet(const LLWLParamKey& key);
	bool setParamSet(const std::string& name, LLWLParamSet& param,  LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL);
	bool setParamSet(const LLWLParamKey& key, LLWLParamSet& param);
	bool setParamSet(const std::string& name, LLSD const & param, LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL);
	bool setParamSet(const LLWLParamKey& key, LLSD const & param);
	bool removeParamSet(const std::string& name, bool delete_from_disk);
	bool removeParamSet(const LLWLParamKey& key, bool delete_from_disk);
	void clearParamSetsOfScope(LLEnvKey::EScope scope);
	bool isSystemPreset(const std::string& preset_name) const;
	void getPresetNames(preset_name_list_t& region, preset_name_list_t& user, preset_name_list_t& sys) const;
	const std::string& findPreset(const std::string& strPresetName, LLEnvKey::EScope eScope);
	void getUserPresetNames(preset_name_list_t& user) const;
	void getLocalPresetNames(preset_name_list_t& local) const;
	void getPresetKeys(preset_key_list_t& keys) const;
	boost::signals2::connection setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb);
	void addAllSkies(LLEnvKey::EScope scope, const LLSD& preset_map);
	void refreshRegionPresets();
	std::map<LLWLParamKey, LLWLParamSet> finalizeFromDayCycle(LLWLParamKey::EScope scope);
	static LLSD createSkyMap(std::map<LLWLParamKey, LLWLParamSet> map);
	const std::map<LLWLParamKey, LLWLParamSet>& getPresets() const { return mParamList; }
	static void initClass(void);
	static void cleanupClass();
	LLWLAnimator mAnimator;
	LLVector4 mLightDir;
	LLVector4 mRotatedLightDir;
	LLVector4 mClampedLightDir;
	LLWLDayCycle mDay;
	LLWLParamSet mCurParams;
	F32 mSunDeltaYaw;
	WLFloatControl mWLGamma;
	F32 mSceneLightStrength;
	WLColorControl mBlueHorizon;
	WLFloatControl mHazeDensity;
	WLColorControl mBlueDensity;
	WLFloatControl mDensityMult;
	WLFloatControl mHazeHorizon;
	WLFloatControl mMaxAlt;
	WLColorControl mLightnorm;
	WLColorControl mSunlight;
	WLColorControl mAmbient;
	WLColorControl mGlow;
	WLColorControl mCloudColor;
	WLColorControl mCloudMain;
	WLFloatControl mCloudCoverage;
	WLColorControl mCloudDetail;
	WLFloatControl mDistanceMult;
	WLFloatControl mCloudScale;
	F32 mDomeOffset;
	F32 mDomeRadius;
private:
	friend class LLWLAnimator;
	std::vector<LLGLSLShader *> mShaderList;
	static void loadWindlightNotecard(LLVFS *vfs, const LLUUID& asset_id, LLAssetType::EType asset_type, void *user_data, S32 status, LLExtStat ext_status);
	void loadAllPresets();
	void loadPresetsFromDir(const std::string& dir);
	bool loadPreset(const std::string& path);
	static std::string getSysDir();
	static std::string getUserDir();
	friend class LLSingleton<LLWLParamManager>;
	void initSingleton();
	LLWLParamManager();
	~LLWLParamManager();
	std::map<LLWLParamKey, LLWLParamSet> mParamList;
	preset_list_signal_t mPresetListChangeSignal;
public:
	void initHack();
};
inline F32 LLWLParamManager::getDomeOffset(void) const
{
	return mDomeOffset;
}
inline F32 LLWLParamManager::getDomeRadius(void) const
{
	return mDomeRadius;
}
inline LLVector4 LLWLParamManager::getLightDir(void) const
{
	return mLightDir;
}
inline LLVector4 LLWLParamManager::getClampedLightDir(void) const
{
	return mClampedLightDir;
}
inline LLVector4 LLWLParamManager::getRotatedLightDir(void) const
{
	return mRotatedLightDir;
}
#endif
