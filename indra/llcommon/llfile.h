/** 
 * @file llfile.h
 * @author Michael Schlachter
 * @date 2006-03-23
 * @brief Declaration of cross-platform POSIX file buffer and c++
 * stream classes.
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
#ifndef LL_LLFILE_H
#define LL_LLFILE_H
#include <fstream>
#include <sys/stat.h>
typedef FILE	LLFILE;
#if LL_WINDOWS
typedef struct _stat	llstat;
#else
typedef struct stat		llstat;
#include <sys/types.h>
#endif
#ifndef S_ISREG
# define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
# define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#include "llstring.h"
class LL_COMMON_API LLFile
{
public:
	static	LLFILE*	fopen(const std::string& filename,const char* accessmode);
	static	LLFILE*	_fsopen(const std::string& filename,const char* accessmode,int	sharingFlag);
	static	int		close(LLFILE * file);
	static	int		mkdir_nowarn(const std::string& filename, int perms);
	static	int		rmdir_nowarn(const std::string& filename);
	static	int		remove_nowarn(const std::string& filename);
	static	int		rename_nowarn(const std::string& filename, const std::string& newname);
	static	int		mkdir(const std::string& filename, int perms = 0700);
	static	int		rmdir(const std::string& filename);
	static	int		remove(const std::string& filename, int supress_error = 0);
	static	int		rename(const std::string& filename,const std::string& newname, int supress_error = 0);
	static	int		stat(const std::string&	filename,llstat*	file_status);
	static	bool	isdir(const std::string&	filename);
	static	bool	isfile(const std::string&	filename);
	static	LLFILE *	_Fiopen(const std::string& filename,
			std::ios::openmode mode);
	static  const char * tmpdir();
	static std::string strerr(int errn);
	static std::string strerr();
};
#if !defined(LL_WINDOWS)
typedef std::ifstream llifstream;
typedef std::ofstream llofstream;
#else
class LL_COMMON_API llifstream : public	std::ifstream
{
public:
	llifstream();
	explicit llifstream(const std::string& _Filename,
			ios_base::openmode _Mode = ios_base::in);
	explicit llifstream(const char* _Filename,
			ios_base::openmode _Mode = ios_base::in);
	void open(const std::string& _Filename,
			ios_base::openmode _Mode = ios_base::in);
	void open(const char* _Filename,
			ios_base::openmode _Mode = ios_base::in);
};
class LL_COMMON_API llofstream : public	std::ofstream
{
public:
	llofstream();
	explicit llofstream(const std::string& _Filename,
			ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
	explicit llofstream(const char* _Filename,
			ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
	void open(const std::string& _Filename,
			ios_base::openmode _Mode = ios_base::out | ios_base::trunc);
	void open(const char* _Filename,
			ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
};
#endif
std::streamsize LL_COMMON_API llifstream_size(llifstream& fstr);
std::streamsize LL_COMMON_API llofstream_size(llofstream& fstr);
#endif
