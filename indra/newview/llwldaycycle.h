/**
 * @file llwlparammanager.h
 * @brief Implementation for the LLWLParamManager class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#ifndef LL_WL_DAY_CYCLE_H
#define LL_WL_DAY_CYCLE_H
class LLWLDayCycle;
#include <vector>
#include <map>
#include <string>
#include "llwlparamset.h"
#include "llwlanimator.h"
struct LLWLParamKey;
#include "llenvmanager.h"
class LLWLDayCycle
{
	LOG_CLASS(LLWLDayCycle);
public:
	std::map<F32, LLWLParamKey> mTimeMap;
	F32 mDayRate;
public:
	LLWLDayCycle();
	~LLWLDayCycle();
	void loadDayCycle(const LLSD& llsd, LLEnvKey::EScope scope);
	void loadDayCycleFromFile(const std::string & fileName);
	void saveDayCycle(const std::string & fileName);
	void clearKeys();
	void save(const std::string& file_path);
	static LLSD loadCycleDataFromFile(const std::string & fileName);
	static LLSD loadDayCycleFromPath(const std::string& file_path);
	LLSD asLLSD();
	bool getSkyRefs(std::map<LLWLParamKey, LLWLParamSet>& refs) const;
	bool getSkyMap(LLSD& sky_map) const;
	void clearKeyframes();
	bool addKey(F32 newTime, const std::string & paramName);
	bool addKeyframe(F32 newTime, LLWLParamKey key);
	bool changeKeyTime(F32 oldTime, F32 newTime);
	bool changeKeyframeTime(F32 oldTime, F32 newTime);
	bool changeKeyParam(F32 time, const std::string & paramName);
	bool changeKeyframeParam(F32 time, LLWLParamKey key);
	bool removeKeyframe(F32 time);
	bool getKey(const std::string & name, F32& key);
	bool getKeytime(LLWLParamKey keyFrame, F32& keyTime) const;
	bool getKeyedParam(F32 time, LLWLParamSet& param);
	bool getKeyedParamName(F32 time, std::string & name);
	bool hasReferencesTo(const LLWLParamKey& keyframe) const;
	void removeReferencesTo(const LLWLParamKey& keyframe);
};
#endif
