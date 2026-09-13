/**
 * @file llwlparamset.h
 * @brief Interface for the LLWLParamSet class.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#ifndef LL_WLPARAM_SET_H
#define LL_WLPARAM_SET_H
#include <string>
#include <map>
#include "v4math.h"
#include "v4color.h"
#include "llstaticstringtable.h"
class LLWLParamSet;
class LLGLSLShader;
class LLWLParamSet {
	friend class LLWLParamManager;
public:
	std::string mName;
	LLUUID mInventoryID;
private:
	LLSD mParamValues;
	std::vector<LLStaticHashedString> mParamHashedNames;
	float mCloudScrollXOffset, mCloudScrollYOffset;
	void updateHashedNames();
public:
	LLWLParamSet();
	void update(LLGLSLShader * shader) const;
	void setAll(const LLSD& val);
	const LLSD& getAll();
	void set(const std::string& paramName, float x);
	void set(const std::string& paramName, float x, float y);
	void set(const std::string& paramName, float x, float y, float z);
	void set(const std::string& paramName, float x, float y, float z, float w);
	void set(const std::string& paramName, const float * val);
	void set(const std::string& paramName, const LLVector4 & val);
	void set(const std::string& paramName, const LLColor4 & val);
	LLVector4 getVector(const std::string& paramName, bool& error);
	F32 getFloat(const std::string& paramName, bool& error);
	void setStarBrightness(F32 val);
	F32 getStarBrightness();
	void setSunAngle(F32 val);
	F32 getSunAngle();
	void setEastAngle(F32 val);
	F32 getEastAngle();
	void setEnableCloudScrollX(bool val);
	bool getEnableCloudScrollX();
	void setEnableCloudScrollY(bool val);
	bool getEnableCloudScrollY();
	void setCloudScrollX(F32 val);
	F32 getCloudScrollX();
	void setCloudScrollY(F32 val);
	F32 getCloudScrollY();
	void mix(LLWLParamSet& src, LLWLParamSet& dest,
		F32 weight);
	void updateCloudScrolling(void);
};
inline void LLWLParamSet::setAll(const LLSD& val)
{
	if(val.isMap()) {
		mParamValues = val;
	}
	updateHashedNames();
}
inline const LLSD& LLWLParamSet::getAll()
{
	return mParamValues;
}
inline void LLWLParamSet::setStarBrightness(float val) {
	set("star_brightness", val);
}
inline F32 LLWLParamSet::getStarBrightness() {
	return (F32) mParamValues["star_brightness"].asReal();
}
inline F32 LLWLParamSet::getSunAngle() {
	return (F32) mParamValues["sun_angle"].asReal();
}
inline F32 LLWLParamSet::getEastAngle() {
	return (F32) mParamValues["east_angle"].asReal();
}
inline void LLWLParamSet::setEnableCloudScrollX(bool val) {
	mParamValues["enable_cloud_scroll"][0] = val;
}
inline bool LLWLParamSet::getEnableCloudScrollX() {
	return mParamValues["enable_cloud_scroll"][0].asBoolean();
}
inline void LLWLParamSet::setEnableCloudScrollY(bool val) {
	mParamValues["enable_cloud_scroll"][1] = val;
}
inline bool LLWLParamSet::getEnableCloudScrollY() {
	return mParamValues["enable_cloud_scroll"][1].asBoolean();
}
inline void LLWLParamSet::setCloudScrollX(F32 val) {
	mParamValues["cloud_scroll_rate"][0] = val;
}
inline F32 LLWLParamSet::getCloudScrollX() {
	return (F32) mParamValues["cloud_scroll_rate"][0].asReal();
}
inline void LLWLParamSet::setCloudScrollY(F32 val) {
	mParamValues["cloud_scroll_rate"][1] = val;
}
inline F32 LLWLParamSet::getCloudScrollY() {
	return (F32) mParamValues["cloud_scroll_rate"][1].asReal();
}
#endif
