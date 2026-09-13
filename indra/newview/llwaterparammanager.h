/**
 * @file llwaterparammanager.h
 * @brief Implementation for the LLWaterParamManager class.
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
#ifndef LL_WATER_PARAMMANAGER_H
#define LL_WATER_PARAMMANAGER_H
#include <list>
#include <map>
#include "llwaterparamset.h"
#include "llviewercamera.h"
#include "v4color.h"
#include <boost/signals2.hpp>
#include "llassettype.h"
class LLVFS;
const F32 WATER_FOG_LIGHT_CLAMP = 0.3f;
struct WaterColorControl {
	F32 mR, mG, mB, mA, mI;
	std::string mName;
	std::string mSliderName;
	bool mHasSliderName;
	inline WaterColorControl(F32 red, F32 green, F32 blue, F32 alpha,
							 F32 intensity, const std::string& n, const std::string& sliderName = LLStringUtil::null)
		: mR(red), mG(green), mB(blue), mA(alpha), mI(intensity), mName(n), mSliderName(sliderName)
	{
		mHasSliderName = false;
		if (mSliderName != "") {
			mHasSliderName = true;
		}
	}
	inline WaterColorControl & operator = (LLColor4 const & val)
	{
		mR = val.mV[0];
		mG = val.mV[1];
		mB = val.mV[2];
		mA = val.mV[3];
		return *this;
	}
	inline operator LLColor4 (void) const
	{
		return LLColor4(mR, mG, mB, mA);
	}
	inline WaterColorControl & operator = (LLVector4 const & val)
	{
		mR = val.mV[0];
		mG = val.mV[1];
		mB = val.mV[2];
		mA = val.mV[3];
		return *this;
	}
	inline operator LLVector4 (void) const
	{
		return LLVector4(mR, mG, mB, mA);
	}
	inline operator LLVector3 (void) const
	{
		return LLVector3(mR, mG, mB);
	}
	inline void update(LLWaterParamSet & params) const
	{
		params.set(mName, mR, mG, mB, mA);
	}
};
struct WaterVector3Control
{
	F32 mX;
	F32 mY;
	F32 mZ;
	std::string mName;
	inline WaterVector3Control(F32 valX, F32 valY, F32 valZ, const std::string& n)
		: mX(valX), mY(valY), mZ(valZ), mName(n)
	{
	}
	inline WaterVector3Control & operator = (LLVector3 const & val)
	{
		mX = val.mV[0];
		mY = val.mV[1];
		mZ = val.mV[2];
		return *this;
	}
	inline void update(LLWaterParamSet & params) const
	{
		params.set(mName, mX, mY, mZ);
	}
};
struct WaterVector2Control
{
	F32 mX;
	F32 mY;
	std::string mName;
	inline WaterVector2Control(F32 valX, F32 valY, const std::string& n)
		: mX(valX), mY(valY), mName(n)
	{
	}
	inline WaterVector2Control & operator = (LLVector2 const & val)
	{
		mX = val.mV[0];
		mY = val.mV[1];
		return *this;
	}
	inline void update(LLWaterParamSet & params) const
	{
		params.set(mName, mX, mY);
	}
};
struct WaterFloatControl
{
	F32 mX;
	std::string mName;
	F32 mMult;
	inline WaterFloatControl(F32 val, const std::string& n, F32 m=1.0f)
		: mX(val), mName(n), mMult(m)
	{
	}
	inline WaterFloatControl & operator = (LLVector4 const & val)
	{
		mX = val.mV[0];
		return *this;
	}
	inline operator F32 (void) const
	{
		return mX;
	}
	inline void update(LLWaterParamSet & params) const
	{
		params.set(mName, mX);
	}
};
struct WaterExpFloatControl
{
	F32 mExp;
	std::string mName;
	F32 mBase;
	inline WaterExpFloatControl(F32 val, const std::string& n, F32 b)
		: mExp(val), mName(n), mBase(b)
	{
	}
	inline WaterExpFloatControl & operator = (F32 val)
	{
		mExp = log(val) / log(mBase);
		return *this;
	}
	inline operator F32 (void) const
	{
		return pow(mBase, mExp);
	}
	inline void update(LLWaterParamSet & params) const
	{
		params.set(mName, pow(mBase, mExp));
	}
};
class LLWaterParamManager : public LLSingleton<LLWaterParamManager>
{
	LOG_CLASS(LLWaterParamManager);
public:
	typedef std::list<std::string> preset_name_list_t;
	typedef std::map<std::string, LLWaterParamSet> preset_map_t;
	typedef boost::signals2::signal<void()> preset_list_signal_t;
	void updateShaderLinks();
	void loadAllPresets(const std::string & fileName);
	bool loadPresetXML(const std::string& name, std::istream& preset_stream);
	void loadPresetNotecard(const std::string& name, const LLUUID& asset_id, const LLUUID& inv_id);
	void savePreset(const std::string & name);
	bool savePresetToNotecard(const std::string & name);
	void propagateParameters(void);
	void applyParams(const LLSD& params, bool interpolate);
	void update(LLViewerCamera * cam);
	void updateShaderUniforms(LLGLSLShader * shader);
	bool addParamSet(const std::string& name, LLWaterParamSet& param);
	BOOL addParamSet(const std::string& name, LLSD const & param);
	bool getParamSet(const std::string& name, LLWaterParamSet& param);
	bool hasParamSet(const std::string& name);
	bool setParamSet(const std::string& name, LLWaterParamSet& param);
	bool setParamSet(const std::string& name, LLSD const & param);
	bool removeParamSet(const std::string& name, bool delete_from_disk);
	bool isSystemPreset(const std::string& preset_name) const;
	const preset_map_t& getPresets() const { return mParamList; }
	void getPresetNames(preset_name_list_t& presets) const;
	void getPresetNames(preset_name_list_t& user_presets, preset_name_list_t& system_presets) const;
	void getUserPresetNames(preset_name_list_t& user_presets) const;
	boost::signals2::connection setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb);
	bool setNormalMapID(const LLUUID& img);
	void setDensitySliderValue(F32 val);
	LLUUID getNormalMapID(void);
	LLVector2 getWave1Dir(void);
	LLVector2 getWave2Dir(void);
	F32 getScaleAbove(void);
	F32 getScaleBelow(void);
	LLVector3 getNormalScale(void);
	F32 getFresnelScale(void);
	F32 getFresnelOffset(void);
	F32 getBlurMultiplier(void);
	F32 getFogDensity(void);
	LLColor4 getFogColor(void);
public:
	LLWaterParamSet mCurParams;
	WaterColorControl mFogColor;
	WaterExpFloatControl mFogDensity;
	WaterFloatControl mUnderWaterFogMod;
	WaterVector3Control mNormalScale;
	WaterVector2Control mWave1Dir;
	WaterVector2Control mWave2Dir;
	WaterFloatControl mFresnelScale;
	WaterFloatControl mFresnelOffset;
	WaterFloatControl mScaleAbove;
	WaterFloatControl mScaleBelow;
	WaterFloatControl mBlurMultiplier;
	F32 mDensitySliderValue;
private:
	friend class LLSingleton<LLWaterParamManager>;
	void initSingleton();
	LLWaterParamManager();
	~LLWaterParamManager();
	void loadAllPresets();
	void loadPresetsFromDir(const std::string& dir);
	bool loadPreset(const std::string& path);
	static std::string getSysDir();
	static std::string getUserDir();
	LLVector4 mWaterPlane;
	F32 mWaterFogKS;
	std::vector<LLGLSLShader *> mShaderList;
	preset_map_t mParamList;
	preset_list_signal_t mPresetListChangeSignal;
	static void loadWaterNotecard(LLVFS *vfs, const LLUUID& asset_id, LLAssetType::EType asset_type, void *user_data, S32 status, LLExtStat ext_status);
};
inline void LLWaterParamManager::setDensitySliderValue(F32 val)
{
	val /= 10.0f;
	val = 1.0f - val;
	val *= val * val;
	mDensitySliderValue = val;
}
inline LLUUID LLWaterParamManager::getNormalMapID()
{
	return mCurParams.mParamValues["normalMap"].asUUID();
}
inline bool LLWaterParamManager::setNormalMapID(const LLUUID& id)
{
	mCurParams.mParamValues["normalMap"] = id;
	return true;
}
inline LLVector2 LLWaterParamManager::getWave1Dir(void)
{
	bool err;
	return mCurParams.getVector2("wave1Dir", err);
}
inline LLVector2 LLWaterParamManager::getWave2Dir(void)
{
	bool err;
	return mCurParams.getVector2("wave2Dir", err);
}
inline F32 LLWaterParamManager::getScaleAbove(void)
{
	bool err;
	return mCurParams.getFloat("scaleAbove", err);
}
inline F32 LLWaterParamManager::getScaleBelow(void)
{
	bool err;
	return mCurParams.getFloat("scaleBelow", err);
}
inline LLVector3 LLWaterParamManager::getNormalScale(void)
{
	bool err;
	return mCurParams.getVector3("normScale", err);
}
inline F32 LLWaterParamManager::getFresnelScale(void)
{
	bool err;
	return mCurParams.getFloat("fresnelScale", err);
}
inline F32 LLWaterParamManager::getFresnelOffset(void)
{
	bool err;
	return mCurParams.getFloat("fresnelOffset", err);
}
inline F32 LLWaterParamManager::getBlurMultiplier(void)
{
	bool err;
	return mCurParams.getFloat("blurMultiplier", err);
}
inline LLColor4 LLWaterParamManager::getFogColor(void)
{
	bool err;
	return LLColor4(mCurParams.getVector4("waterFogColor", err));
}
#endif
