/** 
 * @file llsys.cpp
 * @brief Implementation of the basic system query functions.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#if LL_WINDOWS
#pragma warning (disable : 4355)
#endif
#include "linden_common.h"
#include "llsys.h"
#include <iostream>
#ifdef LL_STANDALONE
# include <zlib.h>
#else
# include "zlib-ng/zlib.h"
#endif
#include "llprocessor.h"
#include "llerrorcontrol.h"
#include "llevents.h"
#include "llformat.h"
#include "lltimer.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include <boost/circular_buffer.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
using namespace llsd;
#if LL_WINDOWS
#	include "llwin32headerslean.h"
#   include <psapi.h>
#	include <VersionHelpers.h>
#elif LL_DARWIN
#	include <errno.h>
#	include <sys/sysctl.h>
#	include <sys/utsname.h>
#	include <stdint.h>
#	include <Carbon/Carbon.h>
#   include <stdexcept>
#	include <mach/host_info.h>
#	include <mach/mach_host.h>
#	include <mach/task.h>
#	include <mach/task_info.h>
#elif LL_SOLARIS
#	include <stdio.h>
#	include <unistd.h>
#	include <sys/utsname.h>
#	define _STRUCTURED_PROC 1
#	include <sys/procfs.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <errno.h>
extern int errno;
#endif
LLCPUInfo gSysCPU;
static const F32 MEM_INFO_THROTTLE = 20;
static const F32 MEM_INFO_WINDOW = 10*60;
template <typename S, typename M, typename R>
static bool regex_match_no_exc(const S& string, M& match, const R& regex)
{
    try
    {
        return boost::regex_match(string, match, regex);
    }
    catch (const std::runtime_error& e)
    {
        LL_WARNS("LLMemoryInfo") << "error matching with '" << regex.str() << "': "
                                 << e.what() << ":\n'" << string << "'" << LL_ENDL;
        return false;
    }
}
template <typename S, typename M, typename R>
static bool regex_search_no_exc(const S& string, M& match, const R& regex)
{
    try
    {
        return boost::regex_search(string, match, regex);
    }
    catch (const std::runtime_error& e)
    {
        LL_WARNS("LLMemoryInfo") << "error searching with '" << regex.str() << "': "
                                 << e.what() << ":\n'" << string << "'" << LL_ENDL;
        return false;
    }
}
LLOSInfo::LLOSInfo() :
	mMajorVer(0), mMinorVer(0), mBuild(0), mOSVersionString("")
{
#if LL_WINDOWS
	bool is_server = IsWindowsServer();
	std::string service_pack;
	if (IsWindows10OrGreater())
	{
		if (is_server)
		{
			mOSStringSimple = "Microsoft Windows Server 2016 ";
		}
		else
		{
			mOSStringSimple = "Microsoft Windows 10 ";
		}
	}
	else if (IsWindows8Point1OrGreater())
	{
		if (is_server)
		{
			mOSStringSimple = "Microsoft Windows Server 2012 R2 ";
		}
		else
		{
			mOSStringSimple = "Microsoft Windows 8.1 ";
		}
	}
	else if (IsWindows8OrGreater())
	{
		if (is_server)
		{
			mOSStringSimple = "Microsoft Windows Server 2012 ";
		}
		else
		{
			mOSStringSimple = "Microsoft Windows 8 ";
		}
	}
	else if (IsWindows7OrGreater())
	{
		if (is_server)
		{
			mOSStringSimple = "Microsoft Windows Server 2008 R2 ";
		}
		else
		{
			mOSStringSimple = "Microsoft Windows 7 ";
		}
		if (IsWindows7SP1OrGreater())
		{
			service_pack = "Service Pack 1 ";
		}
	}
	else if (IsWindowsVistaOrGreater())
	{
		if (is_server)
		{
			mOSStringSimple = "Microsoft Windows Server 2008 ";
		}
		else
		{
			mOSStringSimple = "Microsoft Windows Vista ";
		}
		if (IsWindowsVistaSP2OrGreater())
		{
			service_pack = "Service Pack 2 ";
		}
		else if (IsWindowsVistaSP1OrGreater())
		{
			service_pack = "Service Pack 1 ";
		}
	}
	else
	{
		mOSStringSimple = "Microsoft Windows (unrecognized) ";
	}
	typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
	SYSTEM_INFO si;
	PGNSI pGNSI;
	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	pGNSI = (PGNSI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if (NULL != pGNSI)
		pGNSI(&si);
	else
		GetSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
	{
		mOSStringSimple += "64-bit ";
	}
	else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		mOSStringSimple += "32-bit ";
	}
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osvi)))
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osvi);
	}
	std::string tmpstr;
	if (bOsVersionInfoEx)
	{
		mMajorVer = osvi.dwMajorVersion;
		mMinorVer = osvi.dwMinorVersion;
		mBuild = osvi.dwBuildNumber;
		tmpstr = llformat("%s(Build %d)", service_pack.c_str(), mBuild);
	}
	else
	{
		mMajorVer = 0;
		mMinorVer = 0;
		mBuild = 0;
		tmpstr = llformat("%s(Build %d)", service_pack.c_str(), 0);
	}
	mOSString = mOSStringSimple + tmpstr;
	LLStringUtil::trim(mOSStringSimple);
#elif LL_DARWIN
	{
		const char * DARWIN_PRODUCT_NAME = "Mac OS X";
		SInt32 major_version, minor_version, bugfix_version;
		OSErr r1 = Gestalt(gestaltSystemVersionMajor, &major_version);
		OSErr r2 = Gestalt(gestaltSystemVersionMinor, &minor_version);
		OSErr r3 = Gestalt(gestaltSystemVersionBugFix, &bugfix_version);
		if((r1 == noErr) && (r2 == noErr) && (r3 == noErr))
		{
			mMajorVer = major_version;
			mMinorVer = minor_version;
			mBuild = bugfix_version;
			std::stringstream os_version_string;
			os_version_string << DARWIN_PRODUCT_NAME << " " << mMajorVer << "." << mMinorVer << "." << mBuild;
			mOSStringSimple.append(os_version_string.str());
		}
		else
		{
			mOSStringSimple.append("Unable to collect OS info");
		}
	}
	struct utsname un;
	if(uname(&un) != -1)
	{
		mOSString = mOSStringSimple;
		mOSString.append(" ");
		mOSString.append(un.sysname);
		mOSString.append(" ");
		mOSString.append(un.release);
		mOSString.append(" ");
		mOSString.append(un.version);
		mOSString.append(" ");
		mOSString.append(un.machine);
	}
	else
	{
		mOSString = mOSStringSimple;
	}
#else
	struct utsname un;
	if(uname(&un) != -1)
	{
		mOSStringSimple.append(un.sysname);
		mOSStringSimple.append(" ");
		mOSStringSimple.append(un.release);
		mOSString = mOSStringSimple;
		mOSString.append(" ");
		mOSString.append(un.version);
		mOSString.append(" ");
		mOSString.append(un.machine);
	}
	else
	{
		mOSStringSimple.append("Unable to collect OS info");
		mOSString = mOSStringSimple;
	}
#endif
	std::stringstream dotted_version_string;
	dotted_version_string << mMajorVer << "." << mMinorVer << "." << mBuild;
	mOSVersionString.append(dotted_version_string.str());
}
#ifndef LL_WINDOWS
S32 LLOSInfo::getMaxOpenFiles()
{
	const S32 OPEN_MAX_GUESS = 256;
#ifdef	OPEN_MAX
	static S32 open_max = OPEN_MAX;
#else
	static S32 open_max = 0;
#endif
	if (0 == open_max)
	{
		errno = 0;
		if ( (open_max = sysconf(_SC_OPEN_MAX)) < 0)
		{
			if (0 == errno)
			{
				open_max = OPEN_MAX_GUESS;
			}
			else
			{
				LL_ERRS() << "LLOSInfo::getMaxOpenFiles: sysconf error for _SC_OPEN_MAX" << LL_ENDL;
			}
		}
	}
	return open_max;
}
#endif
void LLOSInfo::stream(std::ostream& s) const
{
	s << mOSString;
}
const std::string& LLOSInfo::getOSString() const
{
	return mOSString;
}
const std::string& LLOSInfo::getOSStringSimple() const
{
	return mOSStringSimple;
}
const std::string& LLOSInfo::getOSVersionString() const
{
	return mOSVersionString;
}
U32 LLOSInfo::getProcessVirtualSizeKB()
{
	U32 virtual_size = 0;
#if LL_WINDOWS
#elif LL_SOLARIS
	char proc_ps[LL_MAX_PATH];
	sprintf(proc_ps, "/proc/%d/psinfo", (int)getpid());
	int proc_fd = -1;
	if((proc_fd = open(proc_ps, O_RDONLY)) == -1){
		LL_WARNS() << "unable to open " << proc_ps << LL_ENDL;
		return 0;
	}
	psinfo_t proc_psinfo;
	if(read(proc_fd, &proc_psinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)){
		LL_WARNS() << "Unable to read " << proc_ps << LL_ENDL;
		close(proc_fd);
		return 0;
	}
	close(proc_fd);
	virtual_size = proc_psinfo.pr_size;
#endif
	return virtual_size;
}
U32 LLOSInfo::getProcessResidentSizeKB()
{
	U32 resident_size = 0;
#if LL_WINDOWS
#elif LL_SOLARIS
	char proc_ps[LL_MAX_PATH];
	sprintf(proc_ps, "/proc/%d/psinfo", (int)getpid());
	int proc_fd = -1;
	if((proc_fd = open(proc_ps, O_RDONLY)) == -1){
		LL_WARNS() << "unable to open " << proc_ps << LL_ENDL;
		return 0;
	}
	psinfo_t proc_psinfo;
	if(read(proc_fd, &proc_psinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)){
		LL_WARNS() << "Unable to read " << proc_ps << LL_ENDL;
		close(proc_fd);
		return 0;
	}
	close(proc_fd);
	resident_size = proc_psinfo.pr_rssize;
#endif
	return resident_size;
}
LLCPUInfo::LLCPUInfo()
{
	std::ostringstream out;
	LLProcessorInfo proc;
	mHasSSE = proc.hasSSE();
	mHasSSE2 = proc.hasSSE2();
	mHasAltivec = proc.hasAltivec();
	mCPUMHz = (F64)proc.getCPUFrequency();
	mFamily = proc.getCPUFamilyName();
	mCPUString = "Unknown";
	out << proc.getCPUBrandName();
	if (200 < mCPUMHz && mCPUMHz < 10000)
	{
		out << " (" << mCPUMHz << " MHz)";
	}
	mCPUString = out.str();
	LLStringUtil::trim(mCPUString);
}
bool LLCPUInfo::hasAltivec() const
{
	return mHasAltivec;
}
bool LLCPUInfo::hasSSE() const
{
	return mHasSSE;
}
bool LLCPUInfo::hasSSE2() const
{
	return mHasSSE2;
}
F64 LLCPUInfo::getMHz() const
{
	return mCPUMHz;
}
std::string LLCPUInfo::getCPUString() const
{
	return mCPUString;
}
void LLCPUInfo::stream(std::ostream& s) const
{
	s << LLProcessorInfo().getCPUFeatureDescription();
	s << "->mHasSSE:     " << (U32)mHasSSE << std::endl;
	s << "->mHasSSE2:    " << (U32)mHasSSE2 << std::endl;
	s << "->mHasAltivec: " << (U32)mHasAltivec << std::endl;
	s << "->mCPUMHz:     " << mCPUMHz << std::endl;
	s << "->mCPUString:  " << mCPUString << std::endl;
}
class Stats
{
public:
	Stats():
		mStats(LLSD::emptyMap())
	{}
	template <class T>
	void add(const LLSD::String& name, const T& value,
			 typename std::enable_if<std::is_integral<T>::value>::type* = nullptr)
	{
		mStats[name] = LLSD::Integer(value);
	}
	template <class T>
	void add(const LLSD::String& name, const T& value,
			 typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr)
	{
		mStats[name] = LLSD::Real(value);
	}
	void add(const LLSD::String& name, const LLSD::Date& value)
	{
		mStats[name] = value;
	}
	LLSD get() const { return mStats; }
private:
	LLSD mStats;
};
LLMemoryInfo::LLMemoryInfo()
{
	refresh();
}
U32Kilobytes LLMemoryInfo::getPhysicalMemoryKB() const
{
#if LL_WINDOWS
	return U32Kilobytes(mStatsMap["Total Physical KB"].asInteger());
#elif LL_DARWIN
	uint64_t phys = 0;
	int mib[2] = { CTL_HW, HW_MEMSIZE };
	size_t len = sizeof(phys);
	sysctl(mib, 2, &phys, &len, NULL, 0);
	return U64Bytes(phys);
#else
	return 0;
#endif
}
void LLMemoryInfo::getAvailableMemoryKB(U32Kilobytes& avail_physical_mem_kb, U32Kilobytes& avail_virtual_mem_kb)
{
#if LL_WINDOWS
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);
	avail_physical_mem_kb = U64Bytes(state.ullAvailPhys);
	avail_virtual_mem_kb  = U64Bytes(state.ullAvailVirtual);
#elif LL_DARWIN
	avail_physical_mem_kb = (U32Kilobytes)-1 ;
	avail_virtual_mem_kb = (U32Kilobytes)-1 ;
#else
	avail_physical_mem_kb = (U32Kilobytes)-1 ;
	avail_virtual_mem_kb = (U32Kilobytes)-1 ;
#endif
}
void LLMemoryInfo::stream(std::ostream& s) const
{
	std::string pfx(LLError::utcTime() + " <mem> ");
	size_t key_width(0);
	for (const MapEntry& pair : inMap(mStatsMap))
	{
		size_t len(pair.first.length());
		if (len > key_width)
		{
			key_width = len;
		}
	}
	for (const MapEntry& pair : inMap(mStatsMap))
	{
		s << pfx << std::setw(key_width+1) << (pair.first + ':') << ' ';
		LLSD value(pair.second);
		if (value.isInteger())
			s << std::setw(12) << value.asInteger();
		else if (value.isReal())
			s << std::fixed << std::setprecision(1) << value.asReal();
		else if (value.isDate())
			value.asDate().toStream(s);
		else
			s << value;
		s << std::endl;
	}
}
LLSD LLMemoryInfo::getStatsMap() const
{
	return mStatsMap;
}
LLMemoryInfo& LLMemoryInfo::refresh()
{
	mStatsMap = loadStatsMap();
	LL_DEBUGS("LLMemoryInfo") << "Populated mStatsMap:\n";
	LLSDSerialize::toPrettyXML(mStatsMap, LL_CONT);
	LL_ENDL;
	return *this;
}
LLSD LLMemoryInfo::loadStatsMap()
{
	Stats stats;
	stats.add("timestamp", LLDate::now());
#if LL_WINDOWS
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);
	DWORDLONG div = 1024;
	stats.add("Percent Memory use", state.dwMemoryLoad);
	stats.add("Total Physical KB",  state.ullTotalPhys/div);
	stats.add("Avail Physical KB",  state.ullAvailPhys/div);
	stats.add("Total page KB",      state.ullTotalPageFile/div);
	stats.add("Avail page KB",      state.ullAvailPageFile/div);
	stats.add("Total Virtual KB",   state.ullTotalVirtual/div);
	stats.add("Avail Virtual KB",   state.ullAvailVirtual/div);
	PERFORMANCE_INFORMATION perf;
	perf.cb = sizeof(perf);
	GetPerformanceInfo(&perf, sizeof(perf));
	SIZE_T pagekb(perf.PageSize/1024);
	stats.add("CommitTotal KB",     perf.CommitTotal * pagekb);
	stats.add("CommitLimit KB",     perf.CommitLimit * pagekb);
	stats.add("CommitPeak KB",      perf.CommitPeak * pagekb);
	stats.add("PhysicalTotal KB",   perf.PhysicalTotal * pagekb);
	stats.add("PhysicalAvail KB",   perf.PhysicalAvailable * pagekb);
	stats.add("SystemCache KB",     perf.SystemCache * pagekb);
	stats.add("KernelTotal KB",     perf.KernelTotal * pagekb);
	stats.add("KernelPaged KB",     perf.KernelPaged * pagekb);
	stats.add("KernelNonpaged KB",  perf.KernelNonpaged * pagekb);
	stats.add("PageSize KB",        pagekb);
	stats.add("HandleCount",        perf.HandleCount);
	stats.add("ProcessCount",       perf.ProcessCount);
	stats.add("ThreadCount",        perf.ThreadCount);
	PROCESS_MEMORY_COUNTERS_EX pmem;
	pmem.cb = sizeof(pmem);
	GetProcessMemoryInfo(GetCurrentProcess(), PPROCESS_MEMORY_COUNTERS(&pmem), sizeof(pmem));
	stats.add("Page Fault Count",              pmem.PageFaultCount);
	stats.add("PeakWorkingSetSize KB",         pmem.PeakWorkingSetSize/div);
	stats.add("WorkingSetSize KB",             pmem.WorkingSetSize/div);
	stats.add("QutaPeakPagedPoolUsage KB",     pmem.QuotaPeakPagedPoolUsage/div);
	stats.add("QuotaPagedPoolUsage KB",        pmem.QuotaPagedPoolUsage/div);
	stats.add("QuotaPeakNonPagedPoolUsage KB", pmem.QuotaPeakNonPagedPoolUsage/div);
	stats.add("QuotaNonPagedPoolUsage KB",     pmem.QuotaNonPagedPoolUsage/div);
	stats.add("PagefileUsage KB",              pmem.PagefileUsage/div);
	stats.add("PeakPagefileUsage KB",          pmem.PeakPagefileUsage/div);
	stats.add("PrivateUsage KB",               pmem.PrivateUsage/div);
#elif LL_DARWIN
	vm_size_t page_size_kb;
	if (host_page_size(mach_host_self(), &page_size_kb) != KERN_SUCCESS)
	{
		LL_WARNS() << "Unable to get host page size. Using default value." << LL_ENDL;
		page_size_kb = 4096;
	}
	page_size_kb = page_size_kb / 1024;
	{
		vm_statistics64_data_t vmstat;
		mach_msg_type_number_t vmstatCount = HOST_VM_INFO64_COUNT;
		if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t) &vmstat, &vmstatCount) != KERN_SUCCESS)
	{
			LL_WARNS("LLMemoryInfo") << "Unable to collect memory information" << LL_ENDL;
		}
		else
		{
			stats.add("Pages free KB",		page_size_kb * vmstat.free_count);
			stats.add("Pages active KB",	page_size_kb * vmstat.active_count);
			stats.add("Pages inactive KB",	page_size_kb * vmstat.inactive_count);
			stats.add("Pages wired KB",		page_size_kb * vmstat.wire_count);
			stats.add("Pages zero fill",		vmstat.zero_fill_count);
			stats.add("Page reactivations",		vmstat.reactivations);
			stats.add("Page-ins",				vmstat.pageins);
			stats.add("Page-outs",				vmstat.pageouts);
			stats.add("Faults",					vmstat.faults);
			stats.add("Faults copy-on-write",	vmstat.cow_faults);
			stats.add("Cache lookups",			vmstat.lookups);
			stats.add("Cache hits",				vmstat.hits);
			stats.add("Page purgeable count",	vmstat.purgeable_count);
			stats.add("Page purges",			vmstat.purges);
			stats.add("Page speculative reads",	vmstat.speculative_count);
		}
	}
		{
		task_events_info_data_t taskinfo;
		unsigned taskinfoSize = sizeof(taskinfo);
		if (task_info(mach_task_self(), TASK_EVENTS_INFO, (task_info_t) &taskinfo, &taskinfoSize) != KERN_SUCCESS)
					{
			LL_WARNS("LLMemoryInfo") << "Unable to collect task information" << LL_ENDL;
			}
			else
			{
			stats.add("Task page-ins",					taskinfo.pageins);
			stats.add("Task copy-on-write faults",		taskinfo.cow_faults);
			stats.add("Task messages sent",				taskinfo.messages_sent);
			stats.add("Task messages received",			taskinfo.messages_received);
			stats.add("Task mach system call count",	taskinfo.syscalls_mach);
			stats.add("Task unix system call count",	taskinfo.syscalls_unix);
			stats.add("Task context switch count",		taskinfo.csw);
			}
	}
		{
			mach_task_basic_info_data_t taskinfo;
			mach_msg_type_number_t task_count = MACH_TASK_BASIC_INFO_COUNT;
			if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &taskinfo, &task_count) != KERN_SUCCESS)
			{
				LL_WARNS("LLMemoryInfo") << "Unable to collect task information" << LL_ENDL;
			}
			else
			{
				stats.add("Basic virtual memory KB", taskinfo.virtual_size / 1024);
				stats.add("Basic resident memory KB", taskinfo.resident_size / 1024);
				stats.add("Basic max resident memory KB", taskinfo.resident_size_max / 1024);
				stats.add("Basic new thread policy", taskinfo.policy);
				stats.add("Basic suspend count", taskinfo.suspend_count);
			}
	}
#elif LL_SOLARIS
	U64 phys = 0;
	phys = (U64)(sysconf(_SC_PHYS_PAGES)) * (U64)(sysconf(_SC_PAGESIZE)/1024);
	stats.add("Total Physical KB", phys);
#else
	LL_WARNS("LLMemoryInfo") << "Unknown system; unable to collect memory information" << LL_ENDL;
#endif
	return stats.get();
}
std::ostream& operator<<(std::ostream& s, const LLOSInfo& info)
{
	info.stream(s);
	return s;
}
std::ostream& operator<<(std::ostream& s, const LLCPUInfo& info)
{
	info.stream(s);
	return s;
}
std::ostream& operator<<(std::ostream& s, const LLMemoryInfo& info)
{
	info.stream(s);
	return s;
}
class FrameWatcher
{
public:
    FrameWatcher():
        mConnection(LLEventPumps::instance()
                    .obtain("mainloop")
					.listen("FrameWatcher", std::bind(&FrameWatcher::tick, this, std::placeholders::_1))),
        mSampleStart(-1),
        mSampleEnd(0),
        mFrames(0),
        mSamples(int((MEM_INFO_WINDOW / MEM_INFO_THROTTLE) + 0.7)),
        mSlowest(F32_MAX)
    {}
    bool tick(const LLSD&)
    {
        F32 timestamp(mTimer.getElapsedTimeF32());
        ++mFrames;
        if (timestamp < mSampleEnd)
        {
            return false;
        }
        U32 frames(mFrames);
        F32 sampleStart(mSampleStart);
        mFrames = 0;
        mSampleStart = timestamp;
        mSampleEnd = mSampleStart + MEM_INFO_THROTTLE;
        if (sampleStart < 0)
        {
            return false;
        }
        F32 elapsed(timestamp - sampleStart);
        F32 framerate(frames/elapsed);
        F32 slowest(mSlowest);
        boost::circular_buffer<F32>::size_type prevSize(mSamples.size());
        mSamples.push_back(framerate);
        mSlowest = framerate;
        for (boost::circular_buffer<F32>::const_iterator si(mSamples.begin()), send(mSamples.end());
             si != send; ++si)
        {
            if (*si < mSlowest)
            {
                mSlowest = *si;
            }
        }
        if (framerate >= slowest)
        {
            return false;
        }
        LL_INFOS("FrameWatcher") << ' ';
        if (! prevSize)
        {
            LL_CONT << "initial framerate ";
        }
        else
        {
            LL_CONT << "slowest framerate for last " << int(prevSize * MEM_INFO_THROTTLE)
                    << " seconds ";
        }
	std::streamsize precision = LL_CONT.precision();
        LL_CONT << std::fixed << std::setprecision(1) << framerate << '\n'
                << LLMemoryInfo();
	LL_CONT.precision(precision);
	LL_CONT << LL_ENDL;
        return false;
    }
private:
    LLTempBoundListener mConnection;
    LLTimer mTimer;
    F32 mSampleStart, mSampleEnd;
    U32 mFrames;
    boost::circular_buffer<F32> mSamples;
    F32 mSlowest;
};
static FrameWatcher sFrameWatcher;
BOOL gunzip_file(const std::string& srcfile, const std::string& dstfile)
{
	std::string tmpfile;
	const S32 UNCOMPRESS_BUFFER_SIZE = 32768;
	BOOL retval = FALSE;
	gzFile src = NULL;
	std::array<U8, UNCOMPRESS_BUFFER_SIZE> buffer;
	LLFILE *dst = NULL;
	S32 bytes = 0;
	tmpfile = dstfile + ".t";
#if LL_WINDOWS
	src = gzopen_w(utf8str_to_utf16str(srcfile).c_str(), "rb");
#else
	src = gzopen(srcfile.c_str(), "rb");
#endif
	if (! src) goto err;
	dst = LLFile::fopen(tmpfile, "wb");
	if (! dst) goto err;
	do
	{
		bytes = gzread(src, buffer.data(), buffer.size());
		size_t nwrit = fwrite(buffer.data(), sizeof(U8), bytes, dst);
		if (nwrit < (size_t) bytes)
		{
			LL_WARNS() << "Short write on " << tmpfile << ": Wrote " << nwrit << " of " << bytes << " bytes." << LL_ENDL;
			goto err;
		}
	} while(gzeof(src) == 0);
	fclose(dst);
	dst = NULL;
	if (LLFile::rename(tmpfile, dstfile) == -1) goto err;
	retval = TRUE;
err:
	if (src != NULL) gzclose(src);
	if (dst != NULL) fclose(dst);
	return retval;
}
BOOL gzip_file(const std::string& srcfile, const std::string& dstfile)
{
	const S32 COMPRESS_BUFFER_SIZE = 32768;
	std::string tmpfile;
	BOOL retval = FALSE;
	std::array<U8, COMPRESS_BUFFER_SIZE> buffer;
	gzFile dst = NULL;
	LLFILE *src = NULL;
	S32 bytes = 0;
	tmpfile = dstfile + ".t";
#if LL_WINDOWS
	dst = gzopen_w(utf8str_to_utf16str(tmpfile).c_str(), "wb");
#else
	dst = gzopen(tmpfile.c_str(), "wb");
#endif
	if (! dst) goto err;
	src = LLFile::fopen(srcfile, "rb");
	if (! src) goto err;
	while ((bytes = (S32)fread(buffer.data(), sizeof(U8), buffer.size(), src)) > 0)
	{
		if (gzwrite(dst, buffer.data(), bytes) <= 0)
		{
			LL_WARNS() << "gzwrite failed: " << gzerror(dst, NULL) << LL_ENDL;
			goto err;
		}
	}
	if (ferror(src))
	{
		LL_WARNS() << "Error reading " << srcfile << LL_ENDL;
		goto err;
	}
	gzclose(dst);
	dst = NULL;
#if LL_WINDOWS
	LLFile::remove(dstfile);
#endif
	if (LLFile::rename(tmpfile, dstfile) == -1) goto err;
	retval = TRUE;
 err:
	if (src != NULL) fclose(src);
	if (dst != NULL) gzclose(dst);
	return retval;
}
