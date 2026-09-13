/**
 * @file llheapdiag.h
 * @brief Crash/heap diagnostics for intermittent ntdll ACCESS_VIOLATION (AMD RX 580).
 *
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#ifndef LL_LLHEAPDIAG_H
#define LL_LLHEAPDIAG_H
#include <string>
class LLHeapDiag
{
public:
	static void init();
	static void shutdown();
	static void setMainloopState(const char* state);
	static void mark(const char* tag, bool flush = false);
	static void markImportant(const char* tag);
	static void idle();
	static bool validateHeaps(const char* reason);
	static void onFatalCrash(unsigned long code, const void* address);
private:
	LLHeapDiag() = delete;
};
#endif
