/** 
 * @file llviewerassetstats.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "llviewerassetstats.h"
#include "llregionhandle.h"
#include "stdtypes.h"
LLViewerAssetStats * gViewerAssetStatsMain(0);
LLViewerAssetStats * gViewerAssetStatsThread1(0);
namespace
{
static LLViewerAssetStats::EViewerAssetCategories
asset_type_to_category(const LLViewerAssetType::EType at, bool with_http, bool is_temp);
}
void
LLViewerAssetStats::PerRegionStats::reset()
{
	for (int i(0); i < LL_ARRAY_SIZE(mRequests); ++i)
	{
		mRequests[i].mEnqueued.reset();
		mRequests[i].mDequeued.reset();
		mRequests[i].mResponse.reset();
	}
	mFPS.reset();
	mTotalTime = U64Microseconds(0);
	mStartTimestamp = LLViewerAssetStatsFF::get_timestamp();
}
void
LLViewerAssetStats::PerRegionStats::merge(const LLViewerAssetStats::PerRegionStats & src)
{
	if (src.mFPS.getCount() && mFPS.getCount())
	{
		mFPS.merge(src.mFPS);
	}
	for (int i = 0; i < LL_ARRAY_SIZE(mRequests); ++i)
	{
		mRequests[i].mEnqueued.merge(src.mRequests[i].mEnqueued);
		mRequests[i].mDequeued.merge(src.mRequests[i].mDequeued);
		mRequests[i].mResponse.merge(src.mRequests[i].mResponse);
	}
}
void
LLViewerAssetStats::PerRegionStats::accumulateTime(duration_t now)
{
	mTotalTime += (now - mStartTimestamp);
	mStartTimestamp = now;
}
LLViewerAssetStats::LLViewerAssetStats()
	: mRegionHandle(U64(0))
{
	reset();
}
LLViewerAssetStats::LLViewerAssetStats(const LLViewerAssetStats & src)
	: mRegionHandle(src.mRegionHandle),
	  mResetTimestamp(src.mResetTimestamp)
{
	const PerRegionContainer::const_iterator it_end(src.mRegionStats.end());
	for (PerRegionContainer::const_iterator it(src.mRegionStats.begin()); it_end != it; ++it)
	{
		mRegionStats[it->first] = new PerRegionStats(*it->second);
	}
	mCurRegionStats = mRegionStats[mRegionHandle];
}
void
LLViewerAssetStats::reset()
{
	mRegionStats.clear();
	if (mCurRegionStats)
	{
		mCurRegionStats->reset();
	}
	else
	{
		mCurRegionStats = new PerRegionStats(mRegionHandle);
	}
	mRegionStats[mRegionHandle] = mCurRegionStats;
	mResetTimestamp = mCurRegionStats->mStartTimestamp;
}
void
LLViewerAssetStats::setRegion(region_handle_t region_handle)
{
	if (region_handle == mRegionHandle)
	{
		return;
	}
	const duration_t now = LLViewerAssetStatsFF::get_timestamp();
	mCurRegionStats->accumulateTime(now);
	PerRegionContainer::iterator new_stats = mRegionStats.find(region_handle);
	if (mRegionStats.end() == new_stats)
	{
		mCurRegionStats = new PerRegionStats(region_handle);
		mRegionStats[region_handle] = mCurRegionStats;
	}
	else
	{
		mCurRegionStats = new_stats->second;
	}
	mCurRegionStats->mStartTimestamp = now;
	mRegionHandle = region_handle;
}
void
LLViewerAssetStats::recordGetEnqueued(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));
	++(mCurRegionStats->mRequests[int(eac)].mEnqueued);
}
void
LLViewerAssetStats::recordGetDequeued(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));
	++(mCurRegionStats->mRequests[int(eac)].mDequeued);
}
void
LLViewerAssetStats::recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, duration_t duration)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));
	mCurRegionStats->mRequests[int(eac)].mResponse.record(duration);
}
void
LLViewerAssetStats::recordFPS(F32 fps)
{
	mCurRegionStats->mFPS.record(fps);
}
LLSD
LLViewerAssetStats::asLLSD(bool compact_output)
{
	static const LLSD::String tags[EVACCount] =
		{
			LLSD::String("get_texture_temp_http"),
			LLSD::String("get_texture_temp_udp"),
			LLSD::String("get_texture_non_temp_http"),
			LLSD::String("get_texture_non_temp_udp"),
			LLSD::String("get_wearable_udp"),
			LLSD::String("get_sound_udp"),
			LLSD::String("get_gesture_udp"),
			LLSD::String("get_other")
		};
	static const LLSD::String enq_tag("enqueued");
	static const LLSD::String deq_tag("dequeued");
	static const LLSD::String rcnt_tag("resp_count");
	static const LLSD::String rmin_tag("resp_min");
	static const LLSD::String rmax_tag("resp_max");
	static const LLSD::String rmean_tag("resp_mean");
	static const LLSD::String cnt_tag("count");
	static const LLSD::String min_tag("min");
	static const LLSD::String max_tag("max");
	static const LLSD::String mean_tag("mean");
	const duration_t now = LLViewerAssetStatsFF::get_timestamp();
	mCurRegionStats->accumulateTime(now);
	LLSD regions = LLSD::emptyArray();
	for (PerRegionContainer::iterator it = mRegionStats.begin();
		 mRegionStats.end() != it;
		 ++it)
	{
		if (0 == it->first)
		{
			continue;
		}
		PerRegionStats & stats = *it->second;
		LLSD reg_stat = LLSD::emptyMap();
		for (int i = 0; i < LL_ARRAY_SIZE(tags); ++i)
		{
			PerRegionStats::prs_group & group(stats.mRequests[i]);
			if ((! compact_output) ||
				group.mEnqueued.getCount() ||
				group.mDequeued.getCount() ||
				group.mResponse.getCount())
			{
				LLSD & slot = reg_stat[tags[i]];
				slot = LLSD::emptyMap();
				slot[enq_tag] = LLSD(S32(stats.mRequests[i].mEnqueued.getCount()));
				slot[deq_tag] = LLSD(S32(stats.mRequests[i].mDequeued.getCount()));
				slot[rcnt_tag] = LLSD(S32(stats.mRequests[i].mResponse.getCount()));
				slot[rmin_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMin().valueInUnits<LLUnits::Seconds>()));
				slot[rmax_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMax().valueInUnits<LLUnits::Seconds>()));
				slot[rmean_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMean().valueInUnits<LLUnits::Seconds>()));
			}
		}
		if ((! compact_output) || stats.mFPS.getCount())
		{
			LLSD & slot = reg_stat["fps"];
			slot = LLSD::emptyMap();
			slot[cnt_tag] = LLSD(S32(stats.mFPS.getCount()));
			slot[min_tag] = LLSD(F64(stats.mFPS.getMin()));
			slot[max_tag] = LLSD(F64(stats.mFPS.getMax()));
			slot[mean_tag] = LLSD(F64(stats.mFPS.getMean()));
		}
		U32 grid_x(0), grid_y(0);
		grid_from_region_handle(it->first, &grid_x, &grid_y);
		reg_stat["grid_x"] = LLSD::Integer(grid_x);
		reg_stat["grid_y"] = LLSD::Integer(grid_y);
		reg_stat["duration"] = LLSD::Real(stats.mTotalTime.valueInUnits<LLUnits::Seconds>());
		regions.append(reg_stat);
	}
	LLSD ret = LLSD::emptyMap();
	ret["regions"] = regions;
	ret["duration"] = LLSD::Real((now - mResetTimestamp).valueInUnits<LLUnits::Seconds>());
	return ret;
}
void
LLViewerAssetStats::merge(const LLViewerAssetStats & src)
{
	const PerRegionContainer::const_iterator it_end(src.mRegionStats.end());
	for (PerRegionContainer::const_iterator it(src.mRegionStats.begin()); it_end != it; ++it)
	{
		PerRegionContainer::iterator dst(mRegionStats.find(it->first));
		if (mRegionStats.end() == dst)
		{
			mRegionStats[it->first] = new PerRegionStats(*it->second);
		}
		else
		{
			dst->second->merge(*it->second);
		}
	}
}
namespace LLViewerAssetStatsFF
{
void
set_region_main(LLViewerAssetStats::region_handle_t region_handle)
{
	if (! gViewerAssetStatsMain)
		return;
	gViewerAssetStatsMain->setRegion(region_handle);
}
void
record_enqueue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsMain)
		return;
	gViewerAssetStatsMain->recordGetEnqueued(at, with_http, is_temp);
}
void
record_dequeue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsMain)
		return;
	gViewerAssetStatsMain->recordGetDequeued(at, with_http, is_temp);
}
void
record_response_main(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration)
{
	if (! gViewerAssetStatsMain)
		return;
	gViewerAssetStatsMain->recordGetServiced(at, with_http, is_temp, duration);
}
void
record_fps_main(F32 fps)
{
	if (! gViewerAssetStatsMain)
		return;
	gViewerAssetStatsMain->recordFPS(fps);
}
void
set_region_thread1(LLViewerAssetStats::region_handle_t region_handle)
{
	if (! gViewerAssetStatsThread1)
		return;
	gViewerAssetStatsThread1->setRegion(region_handle);
}
void
record_enqueue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsThread1)
		return;
	gViewerAssetStatsThread1->recordGetEnqueued(at, with_http, is_temp);
}
void
record_dequeue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsThread1)
		return;
	gViewerAssetStatsThread1->recordGetDequeued(at, with_http, is_temp);
}
void
record_response_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration)
{
	if (! gViewerAssetStatsThread1)
		return;
	gViewerAssetStatsThread1->recordGetServiced(at, with_http, is_temp, duration);
}
void
init()
{
	if (! gViewerAssetStatsMain)
	{
		gViewerAssetStatsMain = new LLViewerAssetStats();
	}
	if (! gViewerAssetStatsThread1)
	{
		gViewerAssetStatsThread1 = new LLViewerAssetStats();
	}
}
void
cleanup()
{
	delete gViewerAssetStatsMain;
	gViewerAssetStatsMain = 0;
	delete gViewerAssetStatsThread1;
	gViewerAssetStatsThread1 = 0;
}
}
namespace
{
LLViewerAssetStats::EViewerAssetCategories
asset_type_to_category(const LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	switch (at)
	{
	case LLAssetType::AT_TEXTURE:
		return is_temp ? with_http ? LLViewerAssetStats::EVACTextureTempHTTPGet : LLViewerAssetStats::EVACTextureTempUDPGet
			: with_http ? LLViewerAssetStats::EVACTextureTempHTTPGet : LLViewerAssetStats::EVACTextureNonTempUDPGet;
		break;
	case LLAssetType::AT_SOUND:
	case LLAssetType::AT_SOUND_WAV:
		return LLViewerAssetStats::EVACSoundUDPGet;
		break;
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
		return LLViewerAssetStats::EVACWearableUDPGet;
		break;
	case LLAssetType::AT_ANIMATION:
	case LLAssetType::AT_GESTURE:
		return LLViewerAssetStats::EVACGestureUDPGet;
		break;
	case LLAssetType::AT_LANDMARK:
	default:
		return LLViewerAssetStats::EVACOtherGet;
		break;
	}
}
}
