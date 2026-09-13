/**
 * @file   llexception.cpp
 * @brief  Simplified exception helpers for Allyn (adapted from SL viewer).
 */
#include "linden_common.h"
#include "llexception.h"
#include <boost/exception/diagnostic_information.hpp>
#include "llerror.h"
void annotate_exception_(boost::exception& exc)
{
    (void)exc;
}
void crash_on_unhandled_exception_(const char* file, int line, const char* pretty_function,
                                   const std::string& context)
{
    LL_ERRS("LLException") << file << "(" << line << "): Unhandled exception in "
        << pretty_function;
    if (!context.empty())
    {
        LL_CONT << ": " << context;
    }
    LL_CONT << ":\n" << boost::current_exception_diagnostic_information() << LL_ENDL;
}
void log_unhandled_exception_(const char* file, int line, const char* pretty_function,
                              const std::string& context)
{
    LL_WARNS("LLException") << file << "(" << line << "): Unhandled exception in "
        << pretty_function;
    if (!context.empty())
    {
        LL_CONT << ": " << context;
    }
    LL_CONT << ":\n" << boost::current_exception_diagnostic_information() << LL_ENDL;
}
#if LL_WINDOWS
#include <excpt.h>
U32 msc_exception_filter(U32 code, struct _EXCEPTION_POINTERS*)
{
    if (code == 0xE06D7363)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
