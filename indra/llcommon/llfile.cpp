/**
 * @file llfile.cpp
 * @author Michael Schlachter
 * @date 2006-03-23
 * @brief Implementation of cross-platform POSIX file buffer and c++
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
#if LL_WINDOWS
#include "llwin32headerslean.h"
#include <stdlib.h>
#else
#include <errno.h>
#endif
#include "linden_common.h"
#include "llfile.h"
#include "llstring.h"
#include "llerror.h"
#include "stringize.h"
using namespace std;
static std::string empty;
#if LL_WINDOWS
std::string LLFile::strerr(int errn)
{
	char buffer[256];
	strerror_s(buffer, errn);
	buffer[255] = 0;
	return buffer;
}
typedef std::basic_ios<char,std::char_traits < char > > _Myios;
#else
std::string message_from(int , const char* , size_t ,
						 const char* strerror_ret)
{
	return strerror_ret;
}
std::string message_from(int orig_errno, const char* buffer, size_t bufflen,
						 int strerror_ret)
{
	if (strerror_ret == 0)
	{
		return buffer;
	}
	int stre_errno = errno;
	if (stre_errno == ERANGE)
	{
		return STRINGIZE("strerror_r() can't explain errno " << orig_errno
						 << " (" << bufflen << "-byte buffer too small)");
	}
	if (stre_errno == EINVAL)
	{
		return STRINGIZE("unknown errno " << orig_errno);
	}
	return STRINGIZE("strerror_r() can't explain errno " << orig_errno
					 << " (error " << stre_errno << ')');
}
std::string LLFile::strerr(int errn)
{
	char buffer[256];
	return message_from(errn, buffer, sizeof(buffer),
						strerror_r(errn, buffer, sizeof(buffer)));
}
#endif
std::string LLFile::strerr()
{
	return strerr(errno);
}
int warnif(const std::string& desc, const std::string& filename, int rc, int accept=0)
{
	if (rc < 0)
	{
		int errn = errno;
		if (errn != accept)
		{
			LL_WARNS("LLFile") << "Couldn't " << desc << " '" << filename
							   << "' (errno " << errn << "): " << LLFile::strerr(errn) << LL_ENDL;
		}
#if 0 && LL_WINDOWS
		if (errn == EACCES)
		{
			LL_DEBUGS("LLFile") << empty;
			const char* TEMP = getenv("TEMP");
			if (! TEMP)
			{
				LL_CONT << "No $TEMP, not running 'handle'";
			}
			else
			{
				std::string tf(TEMP);
				tf += "\\handle.tmp";
				std::string cmd(STRINGIZE("handle \"" << filename
										  << "\" > \"" << tf << '"'));
				LL_CONT << cmd;
				if (system(cmd.c_str()) != 0)
				{
					LL_CONT << "\nDownload 'handle.exe' from http://technet.microsoft.com/en-us/sysinternals/bb896655";
				}
				else
				{
					std::ifstream inf(tf);
					std::string line;
					while (std::getline(inf, line))
					{
						LL_CONT << '\n' << line;
					}
				}
				LLFile::remove(tf);
			}
			LL_CONT << LL_ENDL;
		}
#endif
	}
	return rc;
}
int	LLFile::mkdir_nowarn(const std::string& dirname, int perms)
{
#if LL_WINDOWS
	std::string utf8dirname = dirname;
	llutf16string utf16dirname = utf8str_to_utf16str(utf8dirname);
	int rc = _wmkdir(utf16dirname.c_str());
#else
	int rc = ::mkdir(dirname.c_str(), (mode_t)perms);
#endif
	return rc;
}
int LLFile::mkdir(const std::string& dirname, int perms)
{
	int rc = LLFile::mkdir_nowarn(dirname, perms);
	return warnif("mkdir", dirname, rc, EEXIST);
}
int	LLFile::rmdir_nowarn(const std::string& dirname)
{
#if LL_WINDOWS
	std::string utf8dirname = dirname;
	llutf16string utf16dirname = utf8str_to_utf16str(utf8dirname);
	int rc = _wrmdir(utf16dirname.c_str());
#else
	int rc = ::rmdir(dirname.c_str());
#endif
	return rc;
}
int	LLFile::rmdir(const std::string& dirname)
{
	int rc = LLFile::rmdir_nowarn(dirname);
	return warnif("rmdir", dirname, rc);
}
LLFILE*	LLFile::fopen(const std::string& filename, const char* mode)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8mode = std::string(mode);
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16mode = utf8str_to_utf16str(utf8mode);
	return _wfopen(utf16filename.c_str(),utf16mode.c_str());
#else
	return ::fopen(filename.c_str(),mode);
#endif
}
LLFILE*	LLFile::_fsopen(const std::string& filename, const char* mode, int sharingFlag)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8mode = std::string(mode);
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16mode = utf8str_to_utf16str(utf8mode);
	return _wfsopen(utf16filename.c_str(),utf16mode.c_str(),sharingFlag);
#else
	llassert(0);
	return NULL;
#endif
}
int	LLFile::close(LLFILE * file)
{
	int ret_value = 0;
	if (file)
	{
		ret_value = fclose(file);
	}
	return ret_value;
}
int	LLFile::remove_nowarn(const std::string& filename)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	int rc = _wremove(utf16filename.c_str());
#else
	int rc = ::remove(filename.c_str());
#endif
	return rc;
}
int	LLFile::remove(const std::string& filename, int supress_error)
{
	int rc = LLFile::remove_nowarn(filename);
	return warnif("remove", filename, rc, supress_error);
}
int	LLFile::rename_nowarn(const std::string& filename, const std::string& newname)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8newname = newname;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16newname = utf8str_to_utf16str(utf8newname);
	int rc = _wrename(utf16filename.c_str(),utf16newname.c_str());
#else
	int rc = ::rename(filename.c_str(),newname.c_str());
	if (rc == -1 && errno == EXDEV)
	{
		rc = std::system(("mv '" + filename + "' '" + newname + '\'').data());
		errno = 0;
	}
#endif
	return rc;
}
int	LLFile::rename(const std::string& filename, const std::string& newname, int supress_error)
{
	int rc = LLFile::rename_nowarn(filename, newname);
	return warnif(STRINGIZE("rename to '" << newname << "' from"), filename, rc, supress_error);
}
int	LLFile::stat(const std::string& filename, llstat* filestatus)
{
#if LL_WINDOWS
	std::string utf8filename = filename;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	int rc = _wstat(utf16filename.c_str(),filestatus);
#else
	int rc = ::stat(filename.c_str(),filestatus);
#endif
	return warnif("stat", filename, rc, ENOENT);
}
bool LLFile::isdir(const std::string& filename)
{
	llstat st;
	return stat(filename, &st) == 0 && S_ISDIR(st.st_mode);
}
bool LLFile::isfile(const std::string& filename)
{
	llstat st;
	return stat(filename, &st) == 0 && S_ISREG(st.st_mode);
}
const char *LLFile::tmpdir()
{
	static std::string utf8path;
	if (utf8path.empty())
	{
		char sep;
#if LL_WINDOWS
		sep = '\\';
		DWORD len = GetTempPathW(0, L"");
		llutf16string utf16path;
		utf16path.resize(len + 1);
		len = GetTempPathW(static_cast<DWORD>(utf16path.size()), &utf16path[0]);
		utf8path = utf16str_to_utf8str(utf16path);
#else
		sep = '/';
		char *env = getenv("TMPDIR");
		utf8path = env ? env : "/tmp/";
#endif
		if (utf8path[utf8path.size() - 1] != sep)
		{
			utf8path += sep;
		}
	}
	return utf8path.c_str();
}
#if LL_WINDOWS
LLFILE *	LLFile::_Fiopen(const std::string& filename,
		std::ios::openmode mode)
{
	static const char *mods[] =
	{
	"r", "w", "w", "a", "rb", "wb", "wb", "ab",
	"r+", "w+", "a+", "r+b", "w+b", "a+b",
	0};
	static const int valid[] =
	{
		ios_base::in,
		ios_base::out,
		ios_base::out | ios_base::trunc,
		ios_base::out | ios_base::app,
		ios_base::in | ios_base::binary,
		ios_base::out | ios_base::binary,
		ios_base::out | ios_base::trunc | ios_base::binary,
		ios_base::out | ios_base::app | ios_base::binary,
		ios_base::in | ios_base::out,
		ios_base::in | ios_base::out | ios_base::trunc,
		ios_base::in | ios_base::out | ios_base::app,
		ios_base::in | ios_base::out | ios_base::binary,
		ios_base::in | ios_base::out | ios_base::trunc
			| ios_base::binary,
		ios_base::in | ios_base::out | ios_base::app
			| ios_base::binary,
	0};
	LLFILE *fp = 0;
	int n;
	ios_base::openmode atendflag = mode & ios_base::ate;
	ios_base::openmode norepflag = mode & ios_base::_Noreplace;
	if (mode & ios_base::_Nocreate)
		mode |= ios_base::in;
	mode &= ~(ios_base::ate | ios_base::_Nocreate | ios_base::_Noreplace);
	for (n = 0; valid[n] != 0 && valid[n] != mode; ++n)
		;
	if (valid[n] == 0)
		return (0);
	else if (norepflag && mode & (ios_base::out || ios_base::app)
		&& (fp = LLFile::fopen(filename, "r")) != 0)
		{
		fclose(fp);
		return (0);
		}
	else if (fp != 0 && fclose(fp) != 0)
		return (0);
	else if ((fp = LLFile::fopen(filename, mods[n])) == 0)
		return (0);
	if (!atendflag || fseek(fp, 0, SEEK_END) == 0)
		return (fp);
	fclose(fp);
	return (0);
}
#endif
#if LL_WINDOWS
llifstream::llifstream() : std::ifstream()
{
}
llifstream::llifstream(const std::string& _Filename, ios_base::openmode _Mode) :
	std::ifstream(utf8str_to_utf16str(_Filename),
				 _Mode | ios_base::in)
{
}
llifstream::llifstream(const char* _Filename, ios_base::openmode _Mode) :
	std::ifstream(utf8str_to_utf16str(_Filename).c_str(),
				 _Mode | ios_base::in)
{
}
void llifstream::open(const std::string& _Filename, ios_base::openmode _Mode)
{
	std::ifstream::open(utf8str_to_utf16str(_Filename),
		_Mode | ios_base::in);
}
void llifstream::open(const char* _Filename, ios_base::openmode _Mode)
{
	std::ifstream::open(utf8str_to_utf16str(_Filename).c_str(),
		_Mode | ios_base::in);
}
llofstream::llofstream() : std::ofstream()
{
}
llofstream::llofstream(const std::string& _Filename, ios_base::openmode _Mode) :
	std::ofstream(utf8str_to_utf16str(_Filename),
				  _Mode | ios_base::out)
{
}
llofstream::llofstream(const char* _Filename, ios_base::openmode _Mode) :
	std::ofstream(utf8str_to_utf16str(_Filename).c_str(),
				  _Mode | ios_base::out)
{
}
void llofstream::open(const std::string& _Filename, ios_base::openmode _Mode)
{
	std::ofstream::open(utf8str_to_utf16str(_Filename),
		_Mode | ios_base::out);
}
void llofstream::open(const char* _Filename, ios_base::openmode _Mode)
{
	std::ofstream::open(utf8str_to_utf16str(_Filename).c_str(),
		_Mode | ios_base::out);
}
#endif
std::streamsize llifstream_size(llifstream& ifstr)
{
	if(!ifstr.is_open()) return 0;
	std::streampos pos_old = ifstr.tellg();
	ifstr.seekg(0, ios_base::beg);
	std::streampos pos_beg = ifstr.tellg();
	ifstr.seekg(0, ios_base::end);
	std::streampos pos_end = ifstr.tellg();
	ifstr.seekg(pos_old, ios_base::beg);
	return pos_end - pos_beg;
}
std::streamsize llofstream_size(llofstream& ofstr)
{
	if(!ofstr.is_open()) return 0;
	std::streampos pos_old = ofstr.tellp();
	ofstr.seekp(0, ios_base::beg);
	std::streampos pos_beg = ofstr.tellp();
	ofstr.seekp(0, ios_base::end);
	std::streampos pos_end = ofstr.tellp();
	ofstr.seekp(pos_old, ios_base::beg);
	return pos_end - pos_beg;
}
