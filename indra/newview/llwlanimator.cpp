/**
 * @file llwlanimator.cpp
 * @brief Implementation for the LLWLAnimator class.
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
#include "llviewerprecompiledheaders.h"
#include "llwlanimator.h"
#include "llsky.h"
#include "pipeline.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
F64 LLWLAnimator::INTERP_TOTAL_SECONDS = 3.f;
LLWLAnimator::LLWLAnimator() : mStartTime(0.f), mDayRate(1.f), mDayTime(0.f),
							mIsRunning(FALSE), mIsInterpolating(FALSE), mIsInterpolatingSky(FALSE),
							mTimeType(TIME_LINDEN), mInterpStartTime(), mInterpEndTime()
{
	mInterpBeginWL = new LLWLParamSet();
	mInterpEndWL = new LLWLParamSet();
	mInterpBeginWater = new LLWaterParamSet();
	mInterpEndWater = new LLWaterParamSet();
}
void LLWLAnimator::update(LLWLParamSet& curParams)
{
	F64 curTime;
	curTime = getDayTime();
	if(mTimeTrack.size() == 0)
	{
		return;
	}
	mFirstIt = mTimeTrack.begin();
	mSecondIt = mTimeTrack.begin();
	mSecondIt++;
	while(mSecondIt != mTimeTrack.end() && curTime > mSecondIt->first)
	{
		mFirstIt++;
		mSecondIt++;
	}
	if(mSecondIt == mTimeTrack.end() || mFirstIt->first > curTime)
	{
		mSecondIt = mTimeTrack.begin();
		mFirstIt = mTimeTrack.end();
		mFirstIt--;
	}
	F32 weight = 0;
	if(mFirstIt->first < mSecondIt->first)
	{
		weight = F32 (curTime - mFirstIt->first) /
			(mSecondIt->first - mFirstIt->first);
	}
	else if(mFirstIt->first > mSecondIt->first)
	{
		if(curTime >= mFirstIt->first)
		{
			weight = F32 (curTime - mFirstIt->first) /
			((1 + mSecondIt->first) - mFirstIt->first);
		}
		else
		{
			weight = F32 ((1 + curTime) - mFirstIt->first) /
			((1 + mSecondIt->first) - mFirstIt->first);
		}
	}
	else
	{
		weight = 1;
	}
	if(mIsInterpolating)
	{
		clock_t current = clock();
		if(current >= mInterpEndTime)
		{
			if (mIsInterpolatingSky)
			{
				deactivate();
				curParams.setAll(mInterpEndWL->getAll());
			}
			LLWaterParamManager::getInstance()->mCurParams.setAll(mInterpEndWater->getAll());
			mIsInterpolating = false;
			mIsInterpolatingSky = false;
			return;
		}
		if (mIsInterpolatingSky)
		{
			weight = (current - mInterpStartTime) / (INTERP_TOTAL_SECONDS * CLOCKS_PER_SEC);
			curParams.mix(*mInterpBeginWL, *mInterpEndWL, weight);
		}
		else
		{
			LLWLParamSet buf = LLWLParamSet();
			buf.setAll(LLWLParamManager::getInstance()->mParamList[mFirstIt->second].getAll());
			buf.mix(LLWLParamManager::getInstance()->mParamList[mFirstIt->second], LLWLParamManager::getInstance()->mParamList[mSecondIt->second], weight);
			weight = (current - mInterpStartTime) / (INTERP_TOTAL_SECONDS * CLOCKS_PER_SEC);
			curParams.mix(*mInterpBeginWL, buf, weight);
		}
		LLWaterParamManager::getInstance()->mCurParams.mix(*mInterpBeginWater, *mInterpEndWater, weight);
	}
	else
	{
		curParams.mix(LLWLParamManager::getInstance()->mParamList[mFirstIt->second], LLWLParamManager::getInstance()->mParamList[mSecondIt->second], weight);
	}
}
F64 LLWLAnimator::getDayTime()
{
	if(!mIsRunning)
	{
		return mDayTime;
	}
	else if(mTimeType == TIME_LINDEN)
	{
		F32 phase = gSky.getSunPhase() / F_PI;
		if (phase <= 5.0 / 4.0) {
			mDayTime = (1.0 / 3.0) * phase + (1.0 / 3.0);
		}
		else
		{
			mDayTime = phase - (1.0 / 2.0);
		}
		if(mDayTime > 1)
		{
			mDayTime--;
		}
		return mDayTime;
	}
	else if(mTimeType == TIME_LOCAL)
	{
		return getLocalTime();
	}
	mDayTime = (LLTimer::getElapsedSeconds() - mStartTime) / mDayRate;
	if(mDayTime < 0)
	{
		mDayTime = 0;
	}
	while(mDayTime > 1)
	{
		mDayTime--;
	}
	return (F32)mDayTime;
}
void LLWLAnimator::setDayTime(F64 dayTime)
{
	mStartTime = LLTimer::getElapsedSeconds() - dayTime * mDayRate;
	mDayTime = dayTime;
	if(mDayTime < 0)
	{
		mDayTime = 0;
	}
	else if(mDayTime > 1)
	{
		mDayTime = 1;
	}
}
void LLWLAnimator::setTrack(std::map<F32, LLWLParamKey>& curTrack,
							F32 dayRate, F64 dayTime, bool run)
{
	mTimeTrack = curTrack;
	mDayRate = dayRate;
	setDayTime(dayTime);
	mIsRunning = run;
}
void LLWLAnimator::startInterpolation(const LLSD& targetWater)
{
	mInterpBeginWL->setAll(LLWLParamManager::getInstance()->mCurParams.getAll());
	mInterpBeginWater->setAll(LLWaterParamManager::getInstance()->mCurParams.getAll());
	mInterpStartTime = clock();
	mInterpEndTime = mInterpStartTime + clock_t(INTERP_TOTAL_SECONDS) * CLOCKS_PER_SEC;
	mInterpEndWater->setAll(targetWater);
	mIsInterpolating = true;
}
void LLWLAnimator::startInterpolationSky(const LLSD& targetSky)
{
	mInterpEndWL->setAll(targetSky);
	mIsInterpolatingSky = true;
}
std::string LLWLAnimator::timeToString(F32 curTime)
{
	S32 hours;
	S32 min;
	bool isPM = false;
	hours = (S32) (24.0 * curTime);
	curTime -= ((F32) hours / 24.0f);
	min = ll_pos_round(24.0f * 60.0f * curTime);
	if(min == 60)
	{
		hours++;
		min = 0;
	}
	if(hours >= 12 && hours < 24)
	{
		isPM = true;
	}
	if(hours >= 24)
	{
		hours = 12;
	}
	else if(hours > 12)
	{
		hours -= 12;
	}
	else if(hours == 0)
	{
		hours = 12;
	}
	std::stringstream newTime;
	newTime << hours << ":";
	if(min < 10)
	{
		newTime << 0;
	}
	newTime << min << " ";
	if(isPM)
	{
		newTime << "PM";
	}
	else
	{
		newTime << "AM";
	}
	return newTime.str();
}
F64 LLWLAnimator::getLocalTime()
{
	char buffer[9];
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 9, "%H:%M:%S", timeinfo);
	std::string timeStr(buffer);
	F64 tod = ((F64)atoi(timeStr.substr(0,2).c_str())) / 24.f +
			  ((F64)atoi(timeStr.substr(3,2).c_str())) / 1440.f +
			  ((F64)atoi(timeStr.substr(6,2).c_str())) / 86400.f;
	return tod;
}
