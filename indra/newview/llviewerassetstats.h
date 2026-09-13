/** 
 * @file llviewerassetstats.h
 * @brief Client-side collection of asset request statistics
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
#ifndef LL_LLVIEWERASSETSTATUS_H
#define	LL_LLVIEWERASSETSTATUS_H
#include "linden_common.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llviewerassettype.h"
#include "llviewerassetstorage.h"
#include "llsimplestat.h"
#include "llsd.h"
class LLViewerAssetStats
{
public:
	enum EViewerAssetCategories
	{
		EVACTextureTempHTTPGet,
		EVACTextureTempUDPGet,
		EVACTextureNonTempHTTPGet,
		EVACTextureNonTempUDPGet,
		EVACWearableUDPGet,
		EVACSoundUDPGet,
		EVACGestureUDPGet,
		EVACOtherGet,
		EVACCount
	};
typedef U64Microseconds duration_t;
	typedef U64 region_handle_t;
	class PerRegionStats : public LLRefCount
	{
	public:
		PerRegionStats(const region_handle_t region_handle)
			: LLRefCount(),
			  mRegionHandle(region_handle)
			{
				reset();
			}
		PerRegionStats(const PerRegionStats & src)
			: LLRefCount(),
			  mRegionHandle(src.mRegionHandle),
			  mTotalTime(src.mTotalTime),
			  mStartTimestamp(src.mStartTimestamp),
			  mFPS(src.mFPS)
			{
				for (int i = 0; i < LL_ARRAY_SIZE(mRequests); ++i)
				{
					mRequests[i] = src.mRequests[i];
				}
			}
		void reset();
		void merge(const PerRegionStats & src);
		void accumulateTime(duration_t now);
	public:
		region_handle_t		mRegionHandle;
		duration_t			mTotalTime;
		duration_t			mStartTimestamp;
		LLSimpleStatMMM<>	mFPS;
		struct prs_group
		{
			LLSimpleStatCounter			mEnqueued;
			LLSimpleStatCounter			mDequeued;
			LLSimpleStatMMM<duration_t>	mResponse;
		}
		mRequests [EVACCount];
	};
public:
	LLViewerAssetStats();
	LLViewerAssetStats(const LLViewerAssetStats &);
	LLViewerAssetStats & operator=(const LLViewerAssetStats &);
	void reset();
	void setRegion(region_handle_t region_handle);
	void recordGetEnqueued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetDequeued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, duration_t duration);
	void recordFPS(F32 fps);
	void merge(const LLViewerAssetStats & src);
	LLSD asLLSD(bool compact_output);
protected:
	typedef std::map<region_handle_t, LLPointer<PerRegionStats> > PerRegionContainer;
	region_handle_t mRegionHandle;
	LLPointer<PerRegionStats> mCurRegionStats;
	PerRegionContainer mRegionStats;
	duration_t mResetTimestamp;
};
extern LLViewerAssetStats * gViewerAssetStatsMain;
extern LLViewerAssetStats * gViewerAssetStatsThread1;
namespace LLViewerAssetStatsFF
{
void init();
void cleanup();
inline LLViewerAssetStats::duration_t get_timestamp()
{
	return LLTimer::getTotalTime();
}
void set_region_main(LLViewerAssetStats::region_handle_t region_handle);
void record_enqueue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp);
void record_dequeue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp);
void record_response_main(LLViewerAssetType::EType at, bool with_http, bool is_temp,
						  LLViewerAssetStats::duration_t duration);
void record_fps_main(F32 fps);
void set_region_thread1(LLViewerAssetStats::region_handle_t region_handle);
void record_enqueue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp);
void record_dequeue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp);
void record_response_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp,
						  LLViewerAssetStats::duration_t duration);
}
#endif
