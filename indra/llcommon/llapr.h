/** 
 * @file llapr.h
 * @author Phoenix
 * @date 2004-11-28
 * @brief Helper functions for using the apache portable runtime library.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#ifndef LL_LLAPR_H
#define LL_LLAPR_H
#if LL_SOLARIS
#include <sys/param.h>
#endif
#include "llwin32headerslean.h"
#include <boost/noncopyable.hpp>
#include "apr_thread_proc.h"
#include "apr_getopt.h"
#include "llstring.h"
class LLAPRPool;
class LLVolatileAPRPool;
#define LL_APR_R (APR_READ)
#define LL_APR_W (APR_CREATE|APR_TRUNCATE|APR_WRITE)
#define LL_APR_RB (APR_READ|APR_BINARY)
#define LL_APR_WB (APR_CREATE|APR_TRUNCATE|APR_WRITE|APR_BINARY)
#define LL_APR_RPB (APR_READ|APR_WRITE|APR_BINARY)
#define LL_APR_WPB (APR_CREATE|APR_TRUNCATE|APR_READ|APR_WRITE|APR_BINARY)
class LL_COMMON_API LLAPRFile : boost::noncopyable
{
private:
	apr_file_t* mFile ;
	LLVolatileAPRPool* mVolatileFilePoolp;
	LLAPRPool* mRegularFilePoolp;
public:
	enum access_t {
		long_lived,
		short_lived
	};
	LLAPRFile() ;
	LLAPRFile(const std::string& filename, apr_int32_t flags, S32* sizep = NULL, access_t access_type = short_lived);
	~LLAPRFile() ;
	apr_status_t open(const std::string& filename, apr_int32_t flags, access_t access_type = short_lived, S32* sizep = NULL);
	apr_status_t open(const std::string& filename, apr_int32_t flags, BOOL use_global_pool);
	apr_status_t close() ;
	S32 seek(apr_seek_where_t where, S32 offset);
	apr_status_t eof() { return apr_file_eof(mFile);}
	S32 read(void* buf, U64 nbytes);
	S32 write(const void* buf, U64 nbytes);
	apr_file_t* getFileHandle() {return mFile;}
private:
	static S32 seek(apr_file_t* file, apr_seek_where_t where, S32 offset);
public:
	static bool remove(const std::string& filename);
	static bool rename(const std::string& filename, const std::string& newname);
	static bool isExist(const std::string& filename, apr_int32_t flags = APR_READ);
	static S32 size(const std::string& filename);
	static bool makeDir(const std::string& dirname);
	static bool removeDir(const std::string& dirname);
	static S32 readEx(const std::string& filename, void *buf, S32 offset, S32 nbytes);
	static S32 writeEx(const std::string& filename, void *buf, S32 offset, S32 nbytes);
};
bool LL_COMMON_API ll_apr_warn_status(apr_status_t status);
void LL_COMMON_API ll_apr_assert_status(apr_status_t status);
#endif
