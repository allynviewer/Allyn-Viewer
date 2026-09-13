/** 
 * @file llsys.h
 * @brief System information debugging classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LL_SYS_H
#define LL_SYS_H
#include "llsd.h"
#include <iosfwd>
#include <string>
class LL_COMMON_API LLOSInfo
{
public:
	LLOSInfo();
	void stream(std::ostream& s) const;
	const std::string& getOSString() const;
	const std::string& getOSStringSimple() const;
	const std::string& getOSVersionString() const;
	S32 mMajorVer;
	S32 mMinorVer;
	S32 mBuild;
#ifndef LL_WINDOWS
	static S32 getMaxOpenFiles();
#endif
	static U32 getProcessVirtualSizeKB();
	static U32 getProcessResidentSizeKB();
private:
	std::string mOSString;
	std::string mOSStringSimple;
	std::string mOSVersionString;
};
class LL_COMMON_API LLCPUInfo
{
public:
	LLCPUInfo();
	void stream(std::ostream& s) const;
	std::string getCPUString() const;
	bool hasAltivec() const;
	bool hasSSE() const;
	bool hasSSE2() const;
	F64 getMHz() const;
	const std::string& getFamily() const { return mFamily; }
private:
	bool mHasSSE;
	bool mHasSSE2;
	bool mHasAltivec;
	F64 mCPUMHz;
	std::string mFamily;
	std::string mCPUString;
};
class LL_COMMON_API LLMemoryInfo
{
public:
	LLMemoryInfo();
	void stream(std::ostream& s) const;
	U32Kilobytes getPhysicalMemoryKB() const;
	static void getAvailableMemoryKB(U32Kilobytes& avail_physical_mem_kb, U32Kilobytes& avail_virtual_mem_kb);
	LLSD getStatsMap() const;
	LLMemoryInfo& refresh();
private:
	static LLSD loadStatsMap();
	LLSD mStatsMap;
};
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLOSInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLCPUInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLMemoryInfo& info);
BOOL LL_COMMON_API gunzip_file(const std::string& srcfile, const std::string& dstfile);
BOOL LL_COMMON_API gzip_file(const std::string& srcfile, const std::string& dstfile);
extern LL_COMMON_API LLCPUInfo gSysCPU;
#endif
