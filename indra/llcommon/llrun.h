/** 
 * @file llrun.h
 * @author Phoenix
 * @date 2006-02-16
 * @brief Declaration of LLRunner and LLRunnable classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#ifndef LL_LLRUN_H
#define LL_LLRUN_H
#include <vector>
#include <boost/shared_ptr.hpp>
#include "llpreprocessor.h"
#include "stdtypes.h"
class LLRunnable;
class LL_COMMON_API LLRunner
{
public:
	typedef std::shared_ptr<LLRunnable> run_ptr_t;
	typedef S64 run_handle_t;
	LLRunner();
	~LLRunner();
	enum ERunSchedule
	{
		RUN_IN,
		RUN_EVERY,
		RUN_SCHEDULE_COUNT
	};
	size_t run();
	run_handle_t addRunnable(
		run_ptr_t runnable,
		ERunSchedule schedule,
		F64 seconds);
	run_ptr_t removeRunnable(run_handle_t handle);
protected:
	struct LLRunInfo
	{
		run_handle_t mHandle;
		run_ptr_t mRunnable;
		ERunSchedule mSchedule;
		F64 mNextRunAt;
		F64 mIncrement;
		LLRunInfo(
			run_handle_t handle,
			run_ptr_t runnable,
			ERunSchedule schedule,
			F64 next_run_at,
			F64 increment);
	};
	typedef std::vector<LLRunInfo> run_list_t;
	run_list_t mRunOnce;
	run_list_t mRunEvery;
	run_handle_t mNextHandle;
};
class LL_COMMON_API LLRunnable
{
public:
	LLRunnable();
	virtual ~LLRunnable();
	virtual void run(LLRunner* runner, S64 handle) = 0;
};
#endif
