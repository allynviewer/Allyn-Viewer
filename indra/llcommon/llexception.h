/**
 * @file   llexception.h
 * @author Nat Goodspeed
 * @date   2016-06-29
 * @brief  Types needed for generic exception handling
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */
#if ! defined(LL_LLEXCEPTION_H)
#define LL_LLEXCEPTION_H
#include <stdexcept>
#include <boost/exception/exception.hpp>
#include <boost/throw_exception.hpp>
#include <boost/current_function.hpp>
struct LLException:
    public std::runtime_error,
    public boost::exception
{
    LLException(const std::string& what):
        std::runtime_error(what)
    {}
};
struct LLContinueError: public LLException
{
    LLContinueError(const std::string& what):
        LLException(what)
    {}
};
#define LLTHROW(x)                                                      \
do {                                                                    \
      \
      \
      \
      \
      \
      \
      \
      \
      \
      \
      \
      \
      \
      \
    auto exc{x};                                                        \
    annotate_exception_(exc);                                           \
    BOOST_THROW_EXCEPTION(exc);                                         \
      \
      \
} while (0)
void annotate_exception_(boost::exception& exc);
#define CRASH_ON_UNHANDLED_EXCEPTION(CONTEXT) \
     crash_on_unhandled_exception_(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, CONTEXT)
void crash_on_unhandled_exception_(const char*, int, const char*, const std::string&);
#define LOG_UNHANDLED_EXCEPTION(CONTEXT) \
     log_unhandled_exception_(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, CONTEXT)
void log_unhandled_exception_(const char*, int, const char*, const std::string&);
#if LL_WINDOWS
U32 msc_exception_filter(U32 code, struct _EXCEPTION_POINTERS *exception_infop);
#endif
#endif
