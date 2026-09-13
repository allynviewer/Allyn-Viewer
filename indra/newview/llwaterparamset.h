/**
 * @file llwlparamset.h
 * @brief Interface for the LLWaterParamSet class.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
#ifndef LL_WATER_PARAM_SET_H
#define LL_WATER_PARAM_SET_H
#include <string>
#include <map>
#include "v4math.h"
#include "v4color.h"
#include "llviewershadermgr.h"
class LLWaterParamSet;
class LLWaterParamSet
{
	friend class LLWaterParamManager;
public:
	std::string mName;
	LLUUID mInventoryID;
private:
	LLSD mParamValues;
public:
	LLWaterParamSet();
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
	LLVector4 getVector4(const std::string& paramName, bool& error);
	LLVector3 getVector3(const std::string& paramName, bool& error);
	LLVector2 getVector2(const std::string& paramName, bool& error);
	F32 getFloat(const std::string& paramName, bool& error);
	void mix(LLWaterParamSet& src, LLWaterParamSet& dest,
		F32 weight);
};
inline void LLWaterParamSet::setAll(const LLSD& val)
{
	if(val.isMap()) {
		LLSD::map_const_iterator mIt = val.beginMap();
		for(; mIt != val.endMap(); mIt++)
		{
			mParamValues[mIt->first] = mIt->second;
		}
	}
}
inline const LLSD& LLWaterParamSet::getAll()
{
	return mParamValues;
}
#endif
