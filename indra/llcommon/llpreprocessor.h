/** 
 * @file llpreprocessor.h
 * @brief This file should be included in all Linden Lab files and
 * should only contain special preprocessor directives
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
#ifndef LLPREPROCESSOR_H
#define LLPREPROCESSOR_H
#if defined(LL_WINDOWS)
#define LL_LITTLE_ENDIAN 1
#else
#define LL_BIG_ENDIAN 1
#endif
#ifdef __GNUC__
#define LL_FORCE_INLINE inline __attribute__((always_inline))
#else
#define LL_FORCE_INLINE __forceinline
#endif
#if __GNUC__ >= 3
# define LL_LIKELY(EXPR) __builtin_expect (!!(EXPR), true)
# define LL_UNLIKELY(EXPR) __builtin_expect (!!(EXPR), false)
#else
# define LL_LIKELY(EXPR) (EXPR)
# define LL_UNLIKELY(EXPR) (EXPR)
#endif
#if defined(__clang__)
	#define CLANG_VERSION (__clang_major__ * 10000 \
						+ __clang_minor__ * 100 \
						+ __clang_patchlevel__)
	#ifndef LL_CLANG
		#define LL_CLANG 1
	#endif
#elif defined (__ICC)
	#ifndef LL_INTELC
		#define LL_INTELC 1
	#endif
#elif defined(__GNUC__)
	#define GCC_VERSION (__GNUC__ * 10000 \
						+ __GNUC_MINOR__ * 100 \
						+ __GNUC_PATCHLEVEL__)
	#ifndef LL_GNUC
		#define LL_GNUC 1
	#endif
#elif defined(__MSVC_VER__) || defined(_MSC_VER)
	#ifndef LL_MSVC
		#define LL_MSVC 1
	#endif
#endif
#if __cplusplus < 201100L && _MSC_VER < 1800
#error C++11 support is required to build this project.
#endif
#define LL_THREAD_LOCAL __declspec(thread)
#if LL_WINDOWS && !LL_COMMON_LINK_SHARED
#ifndef APR_DECLARE_STATIC
#define APR_DECLARE_STATIC
#endif
#ifndef APU_DECLARE_STATIC
#define APU_DECLARE_STATIC
#endif
#endif
#if defined(LL_WINDOWS)
#define BOOST_REGEX_NO_LIB 1
#define CURL_STATICLIB 1
#ifndef XML_STATIC
#define XML_STATIC
#endif
#endif
#if LL_MSVC
#pragma warning( 3	     : 4701 )
#pragma warning( 3	     : 4702 )
#pragma warning( 3	     : 4189 )
#pragma warning( 3      :  4263 )
#pragma warning( 3      :  4264 )
#pragma warning( 3       : 4265 )
#pragma warning( 3      :  4266 )
#pragma warning (disable : 4180)
#pragma warning( disable : 4503 )
#pragma warning( disable : 4800 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4231 )
#pragma warning( disable : 4506 )
#pragma warning (disable : 4100)
#pragma warning (disable : 4127)
#pragma warning (disable : 4244)
#pragma warning (disable : 4396)
#pragma warning (disable : 4512)
#pragma warning (disable : 4706)
#pragma warning (disable : 4251)
#pragma warning (disable : 4275)
#if _WIN64
#pragma warning (disable : 4267)
#endif
#endif
#define LL_DLLEXPORT __declspec(dllexport)
#define LL_DLLIMPORT __declspec(dllimport)
#if LL_COMMON_LINK_SHARED
# if defined(llcommon_EXPORTS)
#   define LL_COMMON_API LL_DLLEXPORT
# else
#   define LL_COMMON_API LL_DLLIMPORT
# endif
#else
# define LL_COMMON_API
#endif
#define ll_thread_local __declspec(thread)
#define LL_TYPEOF(exp) decltype(exp)
#define LL_TO_STRING_HELPER(x) #x
#define LL_TO_STRING(x) LL_TO_STRING_HELPER(x)
#define LL_FILE_LINENO_MSG(msg) __FILE__ "(" LL_TO_STRING(__LINE__) ") : " msg
#define LL_GLUE_IMPL(x, y) x##y
#define LL_GLUE_TOKENS(x, y) LL_GLUE_IMPL(x, y)
#define LL_COMPILE_TIME_MESSAGE(msg) __pragma(message(LL_FILE_LINENO_MSG(msg)))
#ifndef LL_PROFILE_ZONE_SCOPED
#define LL_PROFILE_ZONE_SCOPED
#endif
#ifndef LL_PROFILE_ZONE_SCOPED_CATEGORY_UI
#define LL_PROFILE_ZONE_SCOPED_CATEGORY_UI
#endif
#endif
