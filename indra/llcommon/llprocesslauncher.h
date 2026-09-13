/** 
 * @file llprocesslauncher.h
 * @brief Utility class for launching, terminating, and tracking the state of processes.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#ifndef LL_LLPROCESSLAUNCHER_H
#define LL_LLPROCESSLAUNCHER_H
#include "llwin32headerslean.h"
class LL_COMMON_API LLProcessLauncher
{
	LOG_CLASS(LLProcessLauncher);
public:
	LLProcessLauncher();
	virtual ~LLProcessLauncher();
	void setExecutable(const std::string &executable);
	void setWorkingDirectory(const std::string &dir);
	const std::string& getExecutable() const;
	void clearArguments();
	void addArgument(const std::string &arg);
	void addArgument(const char *arg);
	int launch(void);
	bool isRunning(void);
	bool kill(void);
	void orphan(void);
	static void reap(void);
#if LL_WINDOWS
	HANDLE getProcessHandle() { return mProcessHandle; };
#else
	pid_t getProcessID() { return mProcessID; };
#endif
private:
	std::string mExecutable;
	std::string mWorkingDir;
	std::vector<std::string> mLaunchArguments;
#if LL_WINDOWS
	HANDLE mProcessHandle;
#else
	pid_t mProcessID;
#endif
};
#endif
