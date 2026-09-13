/** 
 * @file llerror.h
 * @date   December 2006
 * @brief error message system
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#ifndef LL_LLERROR_H
#define LL_LLERROR_H
#include <sstream>
#include <typeinfo>
#include "stdtypes.h"
#include "llpreprocessor.h"
#include <boost/static_assert.hpp>
const int LL_ERR_NOERR = 0;
#define RELEASE_SHOW_INFO
#define RELEASE_SHOW_WARN
#ifdef _DEBUG
#define SHOW_DEBUG
#define SHOW_WARN
#define SHOW_INFO
#define SHOW_ASSERT
#else
#ifdef LL_RELEASE_WITH_DEBUG_INFO
#define SHOW_ASSERT
#endif
#ifdef RELEASE_SHOW_DEBUG
#define SHOW_DEBUG
#endif
#ifdef RELEASE_SHOW_WARN
#define SHOW_WARN
#endif
#ifdef RELEASE_SHOW_INFO
#define SHOW_INFO
#endif
#ifdef RELEASE_SHOW_ASSERT
#define SHOW_ASSERT
#endif
#endif
inline const std::string liru_assert_strip(const std::string& file) { return file.substr(1+file.substr(0, file.find_last_of("/\\")).find_last_of("/\\")); }
#define llassert_always_msg(func, msg) if (LL_UNLIKELY(!(func))) LL_ERRS() << "ASSERT (" << msg << ")\nfile:" << liru_assert_strip(__FILE__) << " line:" << std::dec << __LINE__ << LL_ENDL
#define llassert_always(func)	llassert_always_msg(func, #func)
#ifdef SHOW_ASSERT
#define llassert(func)			llassert_always_msg(func, #func)
#define llverify(func)			llassert_always_msg(func, #func)
#define ASSERT_ONLY(...)		__VA_ARGS__
#define ASSERT_ONLY_COMMA(...)	, __VA_ARGS__
#else
#define llassert(func)
#define llverify(func)			do {if (func) {}} while(0)
#define ASSERT_ONLY(...)
#define ASSERT_ONLY_COMMA(...)
#endif
#ifdef LL_WINDOWS
#define LL_STATIC_ASSERT(func, msg) static_assert(func, msg)
#define LL_BAD_TEMPLATE_INSTANTIATION(type, msg) static_assert(false, msg)
#else
#define LL_STATIC_ASSERT(func, msg) BOOST_STATIC_ASSERT(func)
#define LL_BAD_TEMPLATE_INSTANTIATION(type, msg) BOOST_STATIC_ASSERT(sizeof(type) != 0 && false);
#endif
namespace LLError
{
	enum ELevel
	{
		LEVEL_ALL = 0,
		LEVEL_DEBUG = 0,
		LEVEL_INFO = 1,
		LEVEL_WARN = 2,
		LEVEL_ERROR = 3,
		LEVEL_NONE = 4
	};
	struct CallSite;
	class LL_COMMON_API Log
	{
	public:
		static bool shouldLog(CallSite&);
		static std::ostringstream* out();
		static void flush(std::ostringstream* out, char* message);
		static void flush(std::ostringstream*, const CallSite&);
	};
	struct LL_COMMON_API CallSite
	{
		CallSite(ELevel level,
				const char* file,
				int line,
				const std::type_info& class_info,
				const char* function,
				bool print_once,
				const char** tags,
				size_t tag_count);
		~CallSite();
#ifdef LL_LIBRARY_INCLUDE
		bool shouldLog();
#else
		bool shouldLog()
		{
			return mCached
					? mShouldLog
					: Log::shouldLog(*this);
		}
#endif
		void invalidate();
		const ELevel			mLevel;
		const char* const		mFile;
		const int				mLine;
		const std::type_info&   mClassInfo;
		const char* const		mFunction;
		const char**			mTags;
		size_t					mTagCount;
		const bool				mPrintOnce;
		const char*				mLevelString;
		std::string				mLocationString,
								mFunctionString,
								mTagString;
		bool					mCached,
								mShouldLog;
		friend class Log;
	};
	class End { };
	inline std::ostream& operator<<(std::ostream& s, const End&)
		{ return s; }
	class LL_COMMON_API NoClassInfo { };
   class LL_COMMON_API LLCallStacks
   {
   private:
       static char**  sBuffer ;
	   static S32     sIndex ;
	   static void allocateStackBuffer();
	   static void freeStackBuffer();
   public:
	   static void push(const char* function, const int line) ;
	   static std::ostringstream* insert(const char* function, const int line) ;
       static void print() ;
       static void clear() ;
	   static void end(std::ostringstream* _out) ;
	   static void cleanup();
   };
}
#define LL_PUSH_CALLSTACKS() LLError::LLCallStacks::push(__FUNCTION__, __LINE__)
#define llcallstacks                                                                      \
	{                                                                                     \
       std::ostringstream* _out = LLError::LLCallStacks::insert(__FUNCTION__, __LINE__) ; \
       (*_out)
#define llcallstacksendl                   \
		LLError::End();                    \
		LLError::LLCallStacks::end(_out) ; \
	}
#define LL_CLEAR_CALLSTACKS() LLError::LLCallStacks::clear()
#define LL_PRINT_CALLSTACKS() LLError::LLCallStacks::print()
#define LOG_CLASS(s)	typedef s _LL_CLASS_TO_LOG
typedef LLError::NoClassInfo _LL_CLASS_TO_LOG;
#define lllog(level, once, nofunction, ...)																	          \
	do {                                                                                                  \
		const char* tags[] = {"", ##__VA_ARGS__};													      \
		::size_t tag_count = LL_ARRAY_SIZE(tags) - 1;													  \
		static LLError::CallSite _site(                                                                   \
		    level, __FILE__, __LINE__, typeid(_LL_CLASS_TO_LOG), nofunction ? NULL : __FUNCTION__, once, &tags[1], tag_count);\
		if (LL_UNLIKELY(_site.shouldLog()))			                                                      \
		{                                                                                                 \
			std::ostringstream* _out = LLError::Log::out();                                               \
			(*_out)
#define LL_CONT	(*_out)
#define LL_NEWLINE '\n'
#define LL_ENDL                               \
			LLError::End();                   \
			LLError::Log::flush(_out, _site); \
		}                                     \
	} while(0)
#ifdef SHOW_DEBUG
#define DO_DEBUG_LOG
#else
#define DO_DEBUG_LOG if (false)
#endif
#define LL_DEBUGS(...)	DO_DEBUG_LOG lllog(LLError::LEVEL_DEBUG, false, false, ##__VA_ARGS__)
#define LL_INFOS(...)	lllog(LLError::LEVEL_INFO, false, false, ##__VA_ARGS__)
#define LL_WARNS(...)	lllog(LLError::LEVEL_WARN, false, false, ##__VA_ARGS__)
#define LL_ERRS(...)	lllog(LLError::LEVEL_ERROR, false, false, ##__VA_ARGS__)
#define LL_ERRS_IF(exp, ...)	if (exp) LL_ERRS(##__VA_ARGS__) << "(" #exp ")"
#define LL_DEBUGS_ONCE(...)	DO_DEBUG_LOG lllog(LLError::LEVEL_DEBUG, true, false, ##__VA_ARGS__)
#define LL_INFOS_ONCE(...)	lllog(LLError::LEVEL_INFO, true, false, ##__VA_ARGS__)
#define LL_WARNS_ONCE(...)	lllog(LLError::LEVEL_WARN, true, false, ##__VA_ARGS__)
#define LL_DEBUGS_NF(...)	DO_DEBUG_LOG {lllog(LLError::LEVEL_DEBUG, false, true, ##__VA_ARGS__)
#define LL_INFOS_NF(...)	lllog(LLError::LEVEL_INFO, false, true, ##__VA_ARGS__)
#define LL_WARNS_NF(...)	lllog(LLError::LEVEL_WARN, false, true, ##__VA_ARGS__)
#define LL_ERRS_NF(...)	lllog(LLError::LEVEL_ERROR, false, true, ##__VA_ARGS__)
#define lldebugs	LL_COMPILE_TIME_MESSAGE("Warning: lldebugs deprecated, use LL_DEBUGS() instead") LL_DEBUGS()
#define llinfos		LL_COMPILE_TIME_MESSAGE("Warning: llinfos deprecated, use LL_INFOS() instead") LL_INFOS()
#define llwarns		LL_COMPILE_TIME_MESSAGE("Warning: llwarns deprecated, use LL_WARNS() instead") LL_WARNS()
#define llerrs		LL_COMPILE_TIME_MESSAGE("Warning: llerrs deprecated, use LL_ERRS() instead") LL_ERRS()
#define llcont		LL_COMPILE_TIME_MESSAGE("Warning: llcont deprecated, use LL_CONT instead") LL_CONT
#define llendl		LL_COMPILE_TIME_MESSAGE("Warning: llendl deprecated, use LL_ENDL instead") LL_ENDL
#endif
