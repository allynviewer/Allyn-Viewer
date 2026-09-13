/** 
 * @file lldir.cpp
 * @brief implementation of directory utilities base class
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
#include "linden_common.h"
#if !LL_WINDOWS
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#else
#include <direct.h>
#endif
#include "lldir.h"
#include "llerror.h"
#include "lltimer.h"
#include "lluuid.h"
#include "lldiriterator.h"
#include "stringize.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <algorithm>
using boost::assign::list_of;
using boost::assign::map_list_of;
#if LL_WINDOWS
#include "lldir_win32.h"
LLDir_Win32 gDirUtil;
#elif LL_DARWIN
#include "lldir_mac.h"
LLDir_Mac gDirUtil;
#elif LL_SOLARIS
#include "lldir_solaris.h"
LLDir_Solaris gDirUtil;
#endif
LLDir *gDirUtilp = (LLDir *)&gDirUtil;
const char
	*LLDir::XUI      = "xui",
	*LLDir::TEXTURES = "textures",
	*LLDir::SKINBASE = "";
static const char* const empty = "";
std::string LLDir::sDumpDir = "";
LLDir::LLDir()
:	mAppName(""),
	mExecutablePathAndName(""),
	mExecutableFilename(""),
	mExecutableDir(""),
	mAppRODataDir(""),
	mOSUserDir(""),
	mOSUserAppDir(""),
	mLindenUserDir(""),
	mOSCacheDir(""),
	mCAFile(""),
	mTempDir(""),
	mDirDelimiter("/"),
	mLanguage("en"),
	mUserName("undefined")
{
}
LLDir::~LLDir()
{
}
std::vector<std::string> LLDir::getFilesInDir(const std::string &dirname)
{
#if LL_WINDOWS
    boost::filesystem::path p (utf8str_to_utf16str(dirname).c_str());
#else
    boost::filesystem::path p (dirname);
#endif
    std::vector<std::string> v;
    if (exists(p))
    {
        if (is_directory(p))
        {
            boost::filesystem::directory_iterator end_iter;
            for (boost::filesystem::directory_iterator dir_itr(p);
                 dir_itr != end_iter;
                 ++dir_itr)
            {
                if (boost::filesystem::is_regular_file(dir_itr->status()))
                {
                    v.push_back(dir_itr->path().filename().string());
                }
            }
        }
    }
    return v;
}
S32 LLDir::deleteFilesInDir(const std::string &dirname, const std::string &mask)
{
	S32 count = 0;
	std::string filename;
	std::string fullpath;
	S32 result;
	if (LLStringUtil::startsWith(mask, getDirDelimiter()))
	{
		LL_WARNS() << "Invalid file mask: " << mask << LL_ENDL;
		llassert(!"Invalid file mask");
	}
	try
	{
		LLDirIterator iter(dirname, mask);
		while (iter.next(filename))
		{
			fullpath = add(dirname, filename);
			if(LLFile::isdir(fullpath))
			{
				count++;
				continue;
			}
			S32 retry_count = 0;
			while (retry_count < 5)
			{
				if (0 != LLFile::remove(fullpath))
				{
					retry_count++;
					result = errno;
				LL_WARNS() << "Problem removing " << fullpath << " - errorcode: "
						<< result << " attempt " << retry_count << LL_ENDL;
					if(retry_count >= 5)
					{
					LL_WARNS() << "Failed to remove " << fullpath << LL_ENDL ;
						return count ;
					}
					ms_sleep(100);
				}
				else
				{
					if (retry_count)
					{
					LL_WARNS() << "Successfully removed " << fullpath << LL_ENDL;
					}
					break;
				}
			}
			count++;
		}
	}
	catch(...)
	{
		LL_WARNS() << "Unable to remove some files from " + dirname  << LL_ENDL;
	}
	return count;
}
U32 LLDir::deleteDirAndContents(const std::string& dir_name)
{
	U32 num_deleted = 0;
	try
	{
#if LL_WINDOWS
	   boost::filesystem::path dir_path(utf8str_to_utf16str(dir_name).c_str());
#else
	   boost::filesystem::path dir_path(dir_name);
#endif
	   if (boost::filesystem::exists (dir_path))
	   {
	      if (!boost::filesystem::is_empty (dir_path))
		  {
		     num_deleted = boost::filesystem::remove_all (dir_path);
		  }
		  else
		  {
		     boost::filesystem::remove (dir_path);
		  }
	   }
	}
	catch (boost::filesystem::filesystem_error &er)
	{
		LL_WARNS() << "Failed to delete " << dir_name << " with error " << er.code().message() << LL_ENDL;
	}
	return num_deleted;
}
const std::string LLDir::findFile(const std::string &filename,
						   const std::string& searchPath1,
						   const std::string& searchPath2,
						   const std::string& searchPath3) const
{
	std::vector<std::string> search_paths;
	search_paths.push_back(searchPath1);
	search_paths.push_back(searchPath2);
	search_paths.push_back(searchPath3);
	return findFile(filename, search_paths);
}
const std::string LLDir::findFile(const std::string& filename, const std::vector<std::string> search_paths) const
{
	std::vector<std::string>::const_iterator search_path_iter;
	for (search_path_iter = search_paths.begin();
		search_path_iter != search_paths.end();
		++search_path_iter)
	{
		if (!search_path_iter->empty())
		{
			std::string filename_and_path = (*search_path_iter);
			if (!filename.empty())
			{
				filename_and_path += getDirDelimiter() + filename;
			}
			if (fileExists(filename_and_path))
			{
				return filename_and_path;
			}
		}
	}
	return "";
}
const std::string &LLDir::getExecutablePathAndName() const
{
	return mExecutablePathAndName;
}
const std::string &LLDir::getExecutableFilename() const
{
	return mExecutableFilename;
}
const std::string &LLDir::getExecutableDir() const
{
	return mExecutableDir;
}
const std::string &LLDir::getWorkingDir() const
{
	return mWorkingDir;
}
const std::string &LLDir::getAppName() const
{
	return mAppName;
}
const std::string &LLDir::getAppRODataDir() const
{
	return mAppRODataDir;
}
const std::string &LLDir::getOSUserDir() const
{
	return mOSUserDir;
}
const std::string &LLDir::getOSUserAppDir() const
{
	return mOSUserAppDir;
}
const std::string &LLDir::getLindenUserDir(bool empty_ok) const
{
	if (mLindenUserDir.empty() && !empty_ok)
	{
		LL_WARNS() << "getLindenUserDir called before set, returning OSUserAppDir as fallback" << LL_ENDL;
		return mOSUserAppDir;
	}
	llassert(empty_ok || !mLindenUserDir.empty());
	return mLindenUserDir;
}
const std::string &LLDir::getChatLogsDir() const
{
	return mChatLogsDir;
}
void LLDir::setDumpDir( const std::string& path )
{
    LLDir::sDumpDir = path;
    if (! sDumpDir.empty() && sDumpDir.rbegin() == mDirDelimiter.rbegin() )
    {
        sDumpDir.erase(sDumpDir.size() -1);
    }
}
const std::string &LLDir::getDumpDir() const
{
	if (sDumpDir.empty() )
	{
		sDumpDir = getExpandedFilename(LL_PATH_LOGS, "") + "Allyn-debug";
		dir_exists_or_crash(sDumpDir);
	}
	return LLDir::sDumpDir;
}
const std::string &LLDir::getPerAccountChatLogsDir() const
{
	return mPerAccountChatLogsDir;
}
const std::string &LLDir::getTempDir() const
{
	return mTempDir;
}
const std::string  LLDir::getCacheDir(bool get_default) const
{
	if (mCacheDir.empty() || get_default)
	{
		if (!mDefaultCacheDir.empty())
		{
			return mDefaultCacheDir;
		}
		std::string res = buildSLOSCacheDir();
		return res;
	}
	else
	{
		return mCacheDir;
	}
}
#define OS_CACHE_DIR "AllynViewer"
std::string LLDir::buildSLOSCacheDir() const
{
	std::string res;
	if (getOSCacheDir().empty())
	{
		if (getOSUserAppDir().empty())
		{
			res = "data";
		}
		else
		{
			res = add(getOSUserAppDir(), "cache_sg1");
		}
	}
	else
	{
		res = add(getOSCacheDir(), OS_CACHE_DIR);
	}
	return res;
}
const std::string &LLDir::getOSCacheDir() const
{
	return mOSCacheDir;
}
const std::string &LLDir::getCAFile() const
{
	return mCAFile;
}
const std::string &LLDir::getDirDelimiter() const
{
	return mDirDelimiter;
}
const std::string& LLDir::getDefaultSkinDir() const
{
	return mDefaultSkinDir;
}
const std::string &LLDir::getSkinDir() const
{
	return mSkinDir;
}
const std::string& LLDir::getUserDefaultSkinDir() const
{
	return mUserDefaultSkinDir;
}
const std::string &LLDir::getUserSkinDir() const
{
	return mUserSkinDir;
}
const std::string LLDir::getSkinBaseDir() const
{
	return mSkinBaseDir;
}
const std::string &LLDir::getLLPluginDir() const
{
	return mLLPluginDir;
}
const std::string &LLDir::getUserName() const
{
	return mUserName;
}
static std::string ELLPathToString(ELLPath location)
{
	typedef std::map<ELLPath, const char*> ELLPathMap;
#define ENT(symbol) (symbol, #symbol)
	static const ELLPathMap sMap = map_list_of
		ENT(LL_PATH_NONE)
		ENT(LL_PATH_USER_SETTINGS)
		ENT(LL_PATH_APP_SETTINGS)
		ENT(LL_PATH_PER_SL_ACCOUNT)
		ENT(LL_PATH_CACHE)
		ENT(LL_PATH_CHARACTER)
		ENT(LL_PATH_HELP)
		ENT(LL_PATH_LOGS)
		ENT(LL_PATH_TEMP)
		ENT(LL_PATH_SKINS)
		ENT(LL_PATH_TOP_SKIN)
		ENT(LL_PATH_CHAT_LOGS)
		ENT(LL_PATH_PER_ACCOUNT_CHAT_LOGS)
		ENT(LL_PATH_USER_SKIN)
		ENT(LL_PATH_LOCAL_ASSETS)
		ENT(LL_PATH_EXECUTABLE)
		ENT(LL_PATH_DEFAULT_SKIN)
		ENT(LL_PATH_FONTS)
		ENT(LL_PATH_LAST)
	;
#undef ENT
	ELLPathMap::const_iterator found = sMap.find(location);
	if (found != sMap.end())
		return found->second;
	return STRINGIZE("Invalid ELLPath value " << location);
}
std::string LLDir::getExpandedFilename(ELLPath location, const std::string& filename) const
{
	return getExpandedFilename(location, "", filename);
}
std::string LLDir::getExpandedFilename(ELLPath location, const std::string& subdir, const std::string& filename) const
{
	return getExpandedFilename(location, "", subdir, filename);
}
std::string LLDir::getExpandedFilename(ELLPath location, const std::string& subdir1, const std::string& subdir2, const std::string& in_filename) const
{
	std::string prefix;
	switch (location)
	{
	case LL_PATH_NONE:
		break;
	case LL_PATH_APP_SETTINGS:
		prefix = add(getAppRODataDir(), "app_settings");
		break;
	case LL_PATH_CHARACTER:
		prefix = add(getAppRODataDir(), "character");
		break;
	case LL_PATH_HELP:
		prefix = "help";
		break;
	case LL_PATH_CACHE:
		prefix = getCacheDir();
		break;
    case LL_PATH_DUMP:
        prefix=getDumpDir();
        break;
	case LL_PATH_USER_SETTINGS:
		prefix = add(getOSUserAppDir(), "user_settings");
		break;
	case LL_PATH_PER_SL_ACCOUNT:
		prefix = getLindenUserDir(true);
		if (prefix.empty())
		{
			LL_DEBUGS("LLDir") << "getLindenUserDir() not yet set: "
							   << ELLPathToString(location)
							   << ", '" << subdir1 << "', '" << subdir2 << "', '" << in_filename
							   << "' => ''" << LL_ENDL;
			return std::string();
		}
		break;
	case LL_PATH_CHAT_LOGS:
		prefix = getChatLogsDir();
		break;
	case LL_PATH_PER_ACCOUNT_CHAT_LOGS:
		prefix = getPerAccountChatLogsDir();
		break;
	case LL_PATH_LOGS:
		prefix = add(getOSUserAppDir(), "logs");
		break;
	case LL_PATH_TEMP:
		prefix = getTempDir();
		break;
	case LL_PATH_TOP_SKIN:
		prefix = getSkinDir();
		break;
	case LL_PATH_DEFAULT_SKIN:
		prefix = getDefaultSkinDir();
		break;
	case LL_PATH_USER_SKIN:
		prefix = getUserSkinDir();
		break;
	case LL_PATH_SKINS:
		prefix = getSkinBaseDir();
		break;
	case LL_PATH_LOCAL_ASSETS:
		prefix = add(getAppRODataDir(), "local_assets");
		break;
	case LL_PATH_EXECUTABLE:
		prefix = getExecutableDir();
		break;
	case LL_PATH_FONTS:
		prefix = add(getAppRODataDir(), "fonts");
		break;
	default:
		llassert(0);
	}
	if (prefix.empty())
	{
		LL_WARNS() << ELLPathToString(location)
				<< ", '" << subdir1 << "', '" << subdir2 << "', '" << in_filename
				<< "': prefix is empty, possible bad filename" << LL_ENDL;
	}
	std::string expanded_filename = add(add(prefix, subdir1), subdir2);
	if (expanded_filename.empty() && in_filename.empty())
	{
		return "";
	}
	expanded_filename += mDirDelimiter;
	expanded_filename += in_filename;
	LL_DEBUGS("LLDir") << ELLPathToString(location)
					   << ", '" << subdir1 << "', '" << subdir2 << "', '" << in_filename
					   << "' => '" << expanded_filename << "'" << LL_ENDL;
	return expanded_filename;
}
std::string LLDir::getBaseFileName(const std::string& filepath, bool strip_exten) const
{
	std::size_t offset = filepath.find_last_of(getDirDelimiter());
	offset = (offset == std::string::npos) ? 0 : offset+1;
	std::string res = filepath.substr(offset, std::string::npos);
	if (strip_exten)
	{
		offset = res.find_last_of('.');
		if (offset != std::string::npos &&
		    offset != 0)
		{
			res = res.substr(0, offset);
		}
	}
	return res;
}
std::string LLDir::getDirName(const std::string& filepath) const
{
	std::size_t offset = filepath.find_last_of(getDirDelimiter());
	size_t len = (offset == std::string::npos) ? 0 : offset;
	std::string dirname = filepath.substr(0, len);
	return dirname;
}
std::string LLDir::getExtension(const std::string& filepath) const
{
	if (filepath.empty())
		return std::string();
	std::string basename = getBaseFileName(filepath, false);
	std::size_t offset = basename.find_last_of('.');
	std::string exten = (offset == std::string::npos || offset == 0) ? "" : basename.substr(offset+1);
	LLStringUtil::toLower(exten);
	return exten;
}
std::string LLDir::findSkinnedFilenameBaseLang(const std::string &subdir,
											   const std::string &filename,
											   ESkinConstraint constraint) const
{
	std::vector<std::string> found(findSkinnedFilenames(subdir, filename, constraint));
	if (found.empty())
	{
		return "";
	}
	return found.front();
}
std::string LLDir::findSkinnedFilename(const std::string &subdir,
									   const std::string &filename,
									   ESkinConstraint constraint) const
{
	std::vector<std::string> found(findSkinnedFilenames(subdir, filename, constraint));
	if (found.empty())
	{
		return "";
	}
	return found.back();
}
template <typename FUNCTION>
void LLDir::walkSearchSkinDirs(const std::string& subdir,
							   const std::vector<std::string>& subsubdirs,
							   const std::string& filename,
							   const FUNCTION& function) const
{
	BOOST_FOREACH(std::string skindir, mSearchSkinDirs)
	{
		std::string subdir_path(add(skindir, subdir));
		BOOST_FOREACH(std::string subsubdir, subsubdirs)
		{
			std::string full_path(add(add(subdir_path, subsubdir), filename));
			if (fileExists(full_path))
			{
				function(subsubdir, full_path);
			}
		}
	}
}
inline void push_back(std::vector<std::string>& vector, const std::string& value)
{
	vector.push_back(value);
}
typedef std::map<std::string, std::string> StringMap;
inline void store_in_map(StringMap& map, const std::string& key, const std::string& value)
{
	map[key] = value;
}
std::vector<std::string> LLDir::findSkinnedFilenames(const std::string& subdir,
													 const std::string& filename,
													 ESkinConstraint constraint) const
{
	static const std::set<std::string> sUnlocalized = list_of
		("")
		("textures")
	;
	LL_DEBUGS("LLDir") << "subdir '" << subdir << "', filename '" << filename
					   << "', constraint "
					   << ((constraint == CURRENT_SKIN)? "CURRENT_SKIN" : "ALL_SKINS")
					   << LL_ENDL;
	std::vector<std::string> results;
	if (filename.find("..") != std::string::npos)
	{
		LL_WARNS("LLDir") << "Ignoring potentially relative filename '" << filename << "'" << LL_ENDL;
		return results;
	}
	static StringMap sLocalized;
	StringMap::const_iterator found = sLocalized.find(subdir);
	if (found == sLocalized.end())
	{
		if (sUnlocalized.find(subdir) != sUnlocalized.end())
		{
			found = sLocalized.insert(StringMap::value_type(subdir, "")).first;
		}
		else
		{
			std::string subdir_path(add(getDefaultSkinDir(), subdir));
			if (fileExists(add(subdir_path, "en")))
			{
				found = sLocalized.insert(StringMap::value_type(subdir, "en")).first;
			}
			else if (fileExists(add(subdir_path, "en-us")))
			{
				found = sLocalized.insert(StringMap::value_type(subdir, "en-us")).first;
			}
			else
			{
				found = sLocalized.insert(StringMap::value_type(subdir, "")).first;
			}
		}
	}
	llassert(found != sLocalized.end());
	std::vector<std::string> subsubdirs;
	if (found->second.empty())
	{
		subsubdirs.push_back("");
	}
	else
	{
		subsubdirs.push_back(found->second);
		if (mLanguage != found->second)
		{
			subsubdirs.push_back(mLanguage);
		}
	}
	if (constraint != CURRENT_SKIN)
	{
		walkSearchSkinDirs(subdir, subsubdirs, filename,
						   boost::bind(push_back, boost::ref(results), _2));
	}
	else
	{
		StringMap path_for;
		walkSearchSkinDirs(subdir, subsubdirs, filename,
						   boost::bind(store_in_map, boost::ref(path_for), _1, _2));
		BOOST_FOREACH(std::string subsubdir, subsubdirs)
		{
			StringMap::const_iterator found(path_for.find(subsubdir));
			if (found != path_for.end())
			{
				results.push_back(found->second);
			}
		}
	}
	LL_DEBUGS("LLDir") << empty;
	const char* comma = "";
	BOOST_FOREACH(std::string path, results)
	{
		LL_CONT << comma << "'" << path << "'";
		comma = ", ";
	}
	LL_CONT << LL_ENDL;
	return results;
}
std::string LLDir::getTempFilename() const
{
	LLUUID random_uuid;
	std::string uuid_str;
	random_uuid.generate();
	random_uuid.toString(uuid_str);
	return add(getTempDir(), uuid_str + ".tmp");
}
std::string LLDir::getScrubbedFileName(const std::string& uncleanFileName)
{
	std::string name(uncleanFileName);
	std::string illegalChars(getForbiddenFileChars());
#if LL_SOLARIS
	illegalChars += ' ';
#endif
	for( unsigned int i = 0; i < illegalChars.length(); i++ )
	{
		size_t j = std::string::npos;
		while ((j = name.find(illegalChars[i])) != std::string::npos)
		{
			name[j] = '_';
		}
	}
	return name;
}
std::string LLDir::getForbiddenFileChars()
{
	return "\\/:*?\"<>|";
}
std::string LLDir::getGridSpecificDir( const std::string& in, const std::string& grid )
{
	std::string ret = in;
	if (!grid.empty())
	{
		std::string gridlower(grid);
		LLStringUtil::toLower(gridlower);
		ret += '@' + gridlower;
	}
	return ret;
}
void LLDir::setLindenUserDir(const std::string &grid, const std::string &first, const std::string &last)
{
	if (!first.empty() && !last.empty())
	{
		std::string userlower(first+"_"+last);
		LLStringUtil::toLower(userlower);
		mLindenUserDir = getGridSpecificDir(add(getOSUserAppDir(), userlower), grid);
	}
	else
	{
		LL_ERRS() << "Invalid name for LLDir::setLindenUserDir" << LL_ENDL;
	}
	dumpCurrentDirectories();
}
void LLDir::makePortable()
{
	std::string dir = mExecutableDir;
	dir.erase(dir.rfind(mDirDelimiter));
	dir += mDirDelimiter + "portable_viewer";
	if (LLFile::mkdir(dir) == -1)
	{
		if (errno != EEXIST)
		{
			LL_WARNS() << "Couldn't create portable_viewer directory." << LL_ENDL;
			return;
		}
	}
	mOSUserDir = dir + mDirDelimiter + "settings";
	mOSCacheDir = dir + mDirDelimiter + "cache";
	if (LLFile::mkdir(mOSUserDir) == -1 || LLFile::mkdir(mOSCacheDir) == -1)
	{
		if (errno != EEXIST)
		{
			LL_WARNS() << "Couldn't create portable_viewer cache and settings directories." << LL_ENDL;
			return;
		}
	}
	mDefaultCacheDir = buildSLOSCacheDir();
	initAppDirs(mAppName);
}
void LLDir::setChatLogsDir( const std::string& path)
{
	if (!path.empty() )
	{
		mChatLogsDir = path;
	}
	else
	{
		LL_WARNS() << "Invalid name for LLDir::setChatLogsDir" << LL_ENDL;
	}
}
void LLDir::updatePerAccountChatLogsDir(const std::string &grid)
{
	mPerAccountChatLogsDir = getGridSpecificDir(add(getChatLogsDir(), mUserName), grid);
}
void LLDir::setPerAccountChatLogsDir(const std::string &grid, const std::string &first, const std::string &last)
{
	if (!first.empty() && !last.empty())
	{
		std::string userlower(first+"_"+last);
		LLStringUtil::toLower(userlower);
		mUserName = userlower;
		updatePerAccountChatLogsDir(grid);
	}
	else
	{
		LL_WARNS() << "Invalid name for LLDir::setPerAccountChatLogsDir" << LL_ENDL;
	}
}
void LLDir::setSkinFolder(const std::string &skin_folder, const std::string& language)
{
	LL_DEBUGS("LLDir") << "Setting skin '" << skin_folder << "', language '" << language << "'"
					   << LL_ENDL;
	mSkinName = skin_folder;
	mLanguage = language;
	mSearchSkinDirs.clear();
	mDefaultSkinDir = getSkinBaseDir();
	append(mDefaultSkinDir, "default");
	addSearchSkinDir(mDefaultSkinDir);
	mSkinDir = getSkinBaseDir();
	append(mSkinDir, skin_folder);
	addSearchSkinDir(mSkinDir);
	mUserSkinDir = getOSUserAppDir();
	append(mUserSkinDir, "skins_sg1");
	mUserDefaultSkinDir = mUserSkinDir;
	append(mUserDefaultSkinDir, "default");
	append(mUserSkinDir, skin_folder);
	addSearchSkinDir(mUserDefaultSkinDir);
	addSearchSkinDir(mUserSkinDir);
}
void LLDir::addSearchSkinDir(const std::string& skindir)
{
	if (std::find(mSearchSkinDirs.begin(), mSearchSkinDirs.end(), skindir) == mSearchSkinDirs.end())
	{
		LL_DEBUGS("LLDir") << "search skin: '" << skindir << "'" << LL_ENDL;
		mSearchSkinDirs.push_back(skindir);
	}
}
std::string LLDir::getSkinFolder() const
{
	return mSkinName;
}
std::string LLDir::getLanguage() const
{
	return mLanguage;
}
bool LLDir::setCacheDir(const std::string &path)
{
	if (path.empty() )
	{
		mCacheDir = "";
		return true;
	}
	else
	{
		LLFile::mkdir(path);
		std::string tempname = add(path, "temp");
		LLFILE* file = LLFile::fopen(tempname,"wt");
		if (file)
		{
			fclose(file);
			LLFile::remove(tempname);
			mCacheDir = path;
			return true;
		}
		return false;
	}
}
void LLDir::dumpCurrentDirectories()
{
	LL_DEBUGS("AppInit","Directories") << "Current Directories:" << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  CurPath:               " << getCurPath() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  AppName:               " << getAppName() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  ExecutableFilename:    " << getExecutableFilename() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  ExecutableDir:         " << getExecutableDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  ExecutablePathAndName: " << getExecutablePathAndName() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  WorkingDir:            " << getWorkingDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  AppRODataDir:          " << getAppRODataDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  OSUserDir:             " << getOSUserDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  OSUserAppDir:          " << getOSUserAppDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  LindenUserDir:         " << getLindenUserDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  TempDir:               " << getTempDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  CAFile:				 " << getCAFile() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  SkinBaseDir:           " << getSkinBaseDir() << LL_ENDL;
	LL_DEBUGS("AppInit","Directories") << "  SkinDir:               " << getSkinDir() << LL_ENDL;
}
std::string LLDir::add(const std::string& path, const std::string& name) const
{
	std::string destpath(path);
	append(destpath, name);
	return destpath;
}
void LLDir::append(std::string& destpath, const std::string& name) const
{
	SepOff sepoff(needSep(destpath, name));
	if (sepoff.first)
	{
		destpath += mDirDelimiter;
	}
	destpath += name.substr(sepoff.second);
}
LLDir::SepOff LLDir::needSep(const std::string& path, const std::string& name) const
{
	if (path.empty() || name.empty())
	{
		return SepOff(false, 0);
	}
	std::string::size_type seplen(mDirDelimiter.length());
	bool path_ends_sep(path.substr(path.length() - seplen) == mDirDelimiter);
	bool name_starts_sep(name.substr(0, seplen) == mDirDelimiter);
	if ((! path_ends_sep) && (! name_starts_sep))
	{
		return SepOff(true, 0);
	}
	if (path_ends_sep && name_starts_sep)
	{
		return SepOff(false, static_cast<U8>(seplen));
	}
	return SepOff(false, 0);
}
void dir_exists_or_crash(const std::string &dir_name)
{
#if LL_WINDOWS
	LLFile::mkdir(dir_name, 0700);
#else
	struct stat dir_stat;
	if(0 != LLFile::stat(dir_name, &dir_stat))
	{
		S32 stat_rv = errno;
		if(ENOENT == stat_rv)
		{
		   if(0 != LLFile::mkdir(dir_name, 0700))
		   {
			   LL_ERRS() << "Unable to create directory: " << dir_name << LL_ENDL;
		   }
		}
		else
		{
			LL_ERRS() << "Unable to stat: " << dir_name << " errno = " << stat_rv
				   << LL_ENDL;
		}
	}
	else
	{
		if(!S_ISDIR(dir_stat.st_mode))
		{
			LL_ERRS() << "Data directory collision: " << dir_name << LL_ENDL;
		}
	}
#endif
}
