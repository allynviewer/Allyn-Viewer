/**
 * @file llheapdiag.cpp
 * @brief Crash/heap diagnostics for intermittent ntdll ACCESS_VIOLATION.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llheapdiag.h"
#include "lldir.h"
#include "lltimer.h"
#include "llviewercontrol.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#if LL_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif
namespace
{
constexpr size_t kTrailCap = 64;
constexpr size_t kLineMax = 512;
struct TrailEntry
{
	F64 time_s = 0.0;
	char text[kLineMax];
};
std::mutex gMutex;
bool gInited = false;
bool gEnabled = true;
bool gValidate = true;
F32 gInterval = 2.f;
F64 gLastIdleCheck = 0.0;
bool gCorruptionLogged = false;
std::string gLogPath;
std::string gLastPath;
char gMainloopState[128] = "boot";
TrailEntry gTrail[kTrailCap];
size_t gTrailNext = 0;
size_t gTrailCount = 0;
U32 gValidateOkCount = 0;
U32 gMarkCount = 0;
F64 now_s()
{
	return LLTimer::getTotalSeconds();
}
void append_trail_locked(const char* text)
{
	TrailEntry& e = gTrail[gTrailNext];
	e.time_s = now_s();
	strncpy(e.text, text, kLineMax - 1);
	e.text[kLineMax - 1] = '\0';
	gTrailNext = (gTrailNext + 1) % kTrailCap;
	if (gTrailCount < kTrailCap)
		++gTrailCount;
	++gMarkCount;
}
void write_line_unlocked(FILE* fp, const char* line)
{
	if (!fp)
		return;
	std::fprintf(fp, "%.3f %s\n", now_s(), line);
	std::fflush(fp);
}
void flush_last_unlocked(const char* line)
{
	if (gLastPath.empty())
		return;
	FILE* fp = std::fopen(gLastPath.c_str(), "wb");
	if (!fp)
		return;
	std::fprintf(fp, "%.3f %s\nmainloop=%s\nmarks=%u validate_ok=%u\n",
				 now_s(), line, gMainloopState, gMarkCount, gValidateOkCount);
	std::fflush(fp);
	std::fclose(fp);
}
void flush_trail_unlocked(const char* header)
{
	if (gLogPath.empty())
		return;
	FILE* fp = std::fopen(gLogPath.c_str(), "ab");
	if (!fp)
		return;
	write_line_unlocked(fp, header);
	std::fprintf(fp, "---- trail (oldest->newest) mainloop=%s ----\n", gMainloopState);
	const size_t start = (gTrailCount == kTrailCap) ? gTrailNext : 0;
	for (size_t i = 0; i < gTrailCount; ++i)
	{
		const TrailEntry& e = gTrail[(start + i) % kTrailCap];
		std::fprintf(fp, "  %.3f %s\n", e.time_s, e.text);
	}
	std::fprintf(fp, "---- end trail ----\n");
	std::fflush(fp);
	std::fclose(fp);
	flush_last_unlocked(header);
}
#if LL_WINDOWS
LONG WINAPI heapdiag_unhandled_filter(EXCEPTION_POINTERS* info)
{
	unsigned long code = 0;
	const void* addr = nullptr;
	if (info && info->ExceptionRecord)
	{
		code = info->ExceptionRecord->ExceptionCode;
		addr = info->ExceptionRecord->ExceptionAddress;
	}
	LLHeapDiag::onFatalCrash(code, addr);
	return EXCEPTION_CONTINUE_SEARCH;
}
bool validate_heaps_unlocked(std::string& detail)
{
	HANDLE heaps[256];
	const DWORD n = GetProcessHeaps(256, heaps);
	if (n == 0 || n > 256)
	{
		detail = llformat("GetProcessHeaps failed/n=%u err=%u", (unsigned)n, (unsigned)GetLastError());
		return false;
	}
	U32 bad = 0;
	for (DWORD i = 0; i < n; ++i)
	{
		const BOOL ok = HeapValidate(heaps[i], 0, nullptr);
		if (!ok)
		{
			++bad;
			detail += llformat(" heap[%u]=CORRUPT(err=%u)", (unsigned)i, (unsigned)GetLastError());
		}
	}
	if (bad == 0)
	{
		detail = llformat("heaps=%u OK", (unsigned)n);
		return true;
	}
	detail = llformat("heaps=%u BAD=%u%s", (unsigned)n, (unsigned)bad, detail.c_str());
	return false;
}
std::string module_at(const void* addr)
{
	if (!addr)
		return "null";
	HMODULE mod = nullptr;
	char path[MAX_PATH] = {};
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						   (LPCSTR)addr, &mod)
		&& mod && GetModuleFileNameA(mod, path, MAX_PATH))
	{
		return llformat("%s+0x%llX", path, (unsigned long long)((const char*)addr - (const char*)mod));
	}
	return llformat("addr=0x%p", addr);
}
#else
bool validate_heaps_unlocked(std::string& detail)
{
	detail = "HeapValidate not available on this platform";
	return true;
}
std::string module_at(const void*) { return "n/a"; }
#endif
}
void LLHeapDiag::init()
{
	std::lock_guard<std::mutex> lock(gMutex);
	if (gInited)
		return;
	gEnabled = true;
	gValidate = true;
	gInterval = 2.f;
	if (gSavedSettings.getControl("HeapDiagEnabled"))
		gEnabled = gSavedSettings.getBOOL("HeapDiagEnabled");
	if (gSavedSettings.getControl("HeapDiagValidateHeaps"))
		gValidate = gSavedSettings.getBOOL("HeapDiagValidateHeaps");
	if (gSavedSettings.getControl("HeapDiagInterval"))
		gInterval = llmax(0.5f, gSavedSettings.getF32("HeapDiagInterval"));
	if (gDirUtilp)
	{
		gLogPath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "Allyn-heap-diag.log");
		gLastPath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "Allyn-heap-diag.last");
	}
	gInited = true;
	append_trail_locked("init");
#if LL_WINDOWS
	SetUnhandledExceptionFilter(heapdiag_unhandled_filter);
#endif
	LL_INFOS("HeapDiag") << "initialized crash-only path=" << gLogPath
						 << " enabled=" << gEnabled
						 << " validate=" << gValidate
						 << " interval=" << gInterval << LL_ENDL;
}
void LLHeapDiag::shutdown()
{
	markImportant("shutdown");
	std::lock_guard<std::mutex> lock(gMutex);
	gEnabled = false;
}
void LLHeapDiag::setMainloopState(const char* state)
{
	if (!state)
		return;
	std::lock_guard<std::mutex> lock(gMutex);
	strncpy(gMainloopState, state, sizeof(gMainloopState) - 1);
	gMainloopState[sizeof(gMainloopState) - 1] = '\0';
}
void LLHeapDiag::mark(const char* tag, bool flush)
{
	if (!tag)
		return;
	std::lock_guard<std::mutex> lock(gMutex);
	if (!gInited || !gEnabled)
		return;
	char buf[kLineMax];
	std::snprintf(buf, sizeof(buf), "MARK [%s] mainloop=%s", tag, gMainloopState);
	append_trail_locked(buf);
	(void)flush;
}
void LLHeapDiag::markImportant(const char* tag)
{
	mark(tag, true);
}
bool LLHeapDiag::validateHeaps(const char* reason)
{
	std::string detail;
	bool ok = true;
	{
		std::lock_guard<std::mutex> lock(gMutex);
		if (!gInited || !gEnabled || !gValidate)
			return true;
		ok = validate_heaps_unlocked(detail);
		char buf[kLineMax];
		std::snprintf(buf, sizeof(buf), "VALIDATE %s %s reason=%s mainloop=%s",
					  ok ? "OK" : "FAIL", detail.c_str(),
					  reason ? reason : "?", gMainloopState);
		append_trail_locked(buf);
		if (ok)
		{
			++gValidateOkCount;
		}
		else
		{
			flush_trail_unlocked(buf);
		}
	}
	if (!ok)
	{
		LL_WARNS("HeapDiag") << "HEAP CORRUPTION DETECTED reason=" << (reason ? reason : "?")
							 << " detail=" << detail
							 << " (see Allyn-heap-diag.log)" << LL_ENDL;
		if (!gCorruptionLogged)
		{
			gCorruptionLogged = true;
			markImportant("HEAP_CORRUPT_FIRST");
		}
	}
	return ok;
}
void LLHeapDiag::idle()
{
	if (!gInited)
		init();
	if (!gEnabled)
		return;
	const F64 t = now_s();
	if (gSavedSettings.getControl("HeapDiagEnabled"))
		gEnabled = gSavedSettings.getBOOL("HeapDiagEnabled");
	if (gSavedSettings.getControl("HeapDiagValidateHeaps"))
		gValidate = gSavedSettings.getBOOL("HeapDiagValidateHeaps");
	if (gSavedSettings.getControl("HeapDiagInterval"))
		gInterval = llmax(0.5f, gSavedSettings.getF32("HeapDiagInterval"));
	if (!gEnabled)
		return;
	if (t - gLastIdleCheck < gInterval)
		return;
	gLastIdleCheck = t;
	char snap[160];
	{
		std::lock_guard<std::mutex> lock(gMutex);
		std::snprintf(snap, sizeof(snap), "idle_tick mainloop=%s marks=%u", gMainloopState, gMarkCount);
	}
	mark(snap, false);
	validateHeaps("idle");
}
void LLHeapDiag::onFatalCrash(unsigned long code, const void* address)
{
	char buf[kLineMax];
	std::snprintf(buf, sizeof(buf),
				  "FATAL_CRASH code=0x%08lX module=%s mainloop=%s",
				  code, module_at(address).c_str(), gMainloopState);
	if (gMutex.try_lock())
	{
		append_trail_locked(buf);
		flush_trail_unlocked(buf);
		gMutex.unlock();
	}
	else if (!gLogPath.empty())
	{
		FILE* fp = std::fopen(gLogPath.c_str(), "ab");
		if (fp)
		{
			std::fprintf(fp, "%.3f %s (no-lock)\n", now_s(), buf);
			std::fflush(fp);
			std::fclose(fp);
		}
	}
}
