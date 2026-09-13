/**  
 * @file lldir.h
 * @brief Definition of directory utilities class
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_LLDIR_H
#define LL_LLDIR_H
#if LL_SOLARIS
#include <sys/param.h>
#define MAX_PATH MAXPATHLEN
#endif
typedef enum ELLPath
{
	LL_PATH_NONE = 0,
	LL_PATH_USER_SETTINGS = 1,
	LL_PATH_APP_SETTINGS = 2,
	LL_PATH_PER_SL_ACCOUNT = 3,
	LL_PATH_CACHE = 4,
	LL_PATH_CHARACTER = 5,
	LL_PATH_HELP = 6,
	LL_PATH_LOGS = 7,
	LL_PATH_TEMP = 8,
	LL_PATH_SKINS = 9,
	LL_PATH_TOP_SKIN = 10,
	LL_PATH_CHAT_LOGS = 11,
	LL_PATH_PER_ACCOUNT_CHAT_LOGS = 12,
	LL_PATH_USER_SKIN = 14,
	LL_PATH_LOCAL_ASSETS = 15,
	LL_PATH_EXECUTABLE = 16,
	LL_PATH_DEFAULT_SKIN = 17,
	LL_PATH_FONTS = 18,
	LL_PATH_DUMP = 19,
	LL_PATH_LAST
} ELLPath;
class LLDir
{
 public:
	LLDir();
	virtual ~LLDir();
	virtual void initAppDirs(const std::string &app_name,
		const std::string& app_read_only_data_dir = "") = 0;
	virtual S32 deleteFilesInDir(const std::string &dirname, const std::string &mask);
    U32 deleteDirAndContents(const std::string& dir_name);
    std::vector<std::string> getFilesInDir(const std::string &dirname);
	virtual U32 countFilesInDir(const std::string &dirname, const std::string &mask) = 0;
	virtual std::string getCurPath() = 0;
	virtual bool fileExists(const std::string &filename) const = 0;
	const std::string findFile(const std::string& filename, const std::vector<std::string> filenames) const;
	const std::string findFile(const std::string& filename, const std::string& searchPath1 = "", const std::string& searchPath2 = "", const std::string& searchPath3 = "") const;
	virtual std::string getLLPluginLauncher() = 0;
	virtual std::string getLLPluginFilename(std::string base_name) = 0;
	const std::string &getExecutablePathAndName() const;
	const std::string &getAppName() const;
	const std::string &getExecutableDir() const;
	const std::string &getExecutableFilename() const;
	const std::string &getWorkingDir() const;
	const std::string &getAppRODataDir() const;
	const std::string &getOSUserDir() const;
	const std::string &getOSUserAppDir() const;
	const std::string &getLindenUserDir(bool empty_ok = false) const;
	const std::string &getChatLogsDir() const;
	const std::string &getDumpDir() const;
	const std::string &getPerAccountChatLogsDir() const;
	const std::string &getTempDir() const;
	const std::string  getCacheDir(bool get_default = false) const;
	const std::string &getOSCacheDir() const;
	const std::string &getCAFile() const;
	const std::string &getDirDelimiter() const;
	const std::string &getDefaultSkinDir() const;
	const std::string &getSkinDir() const;
	const std::string &getUserDefaultSkinDir() const;
	const std::string &getUserSkinDir() const;
	const std::string getSkinBaseDir() const;
	const std::string &getLLPluginDir() const;
	const std::string &getUserName() const;
	std::string getExpandedFilename(ELLPath location, const std::string &filename) const;
	std::string getExpandedFilename(ELLPath location, const std::string &subdir, const std::string &filename) const;
	std::string getExpandedFilename(ELLPath location, const std::string &subdir1, const std::string &subdir2, const std::string &filename) const;
	std::string getBaseFileName(const std::string& filepath, bool strip_exten = false) const;
	std::string getDirName(const std::string& filepath) const;
	std::string getExtension(const std::string& filepath) const;
	enum ESkinConstraint { CURRENT_SKIN, ALL_SKINS };
	std::vector<std::string> findSkinnedFilenames(const std::string& subdir,
												  const std::string& filename,
												  ESkinConstraint constraint=CURRENT_SKIN) const;
	static const char *XUI, *TEXTURES, *SKINBASE;
	std::string findSkinnedFilenameBaseLang(const std::string &subdir,
											const std::string &filename,
											ESkinConstraint constraint=CURRENT_SKIN) const;
	std::string findSkinnedFilename(const std::string &subdir,
									const std::string &filename,
									ESkinConstraint constraint=CURRENT_SKIN) const;
	std::string getTempFilename() const;
	static std::string getScrubbedFileName(const std::string& uncleanFileName);
	static std::string getForbiddenFileChars();
	static std::string getGridSpecificDir( const std::string& in, const std::string& grid );
    void setDumpDir( const std::string& path );
	void makePortable();
	virtual void setChatLogsDir(const std::string &path);
	virtual void setPerAccountChatLogsDir(const std::string &grid, const std::string &first, const std::string& last);
	virtual void setLindenUserDir(const std::string& grid, const std::string& first, const std::string& last);
	virtual void setSkinFolder(const std::string &skin_folder, const std::string& language);
	virtual std::string getSkinFolder() const;
	virtual std::string getLanguage() const;
	virtual bool setCacheDir(const std::string &path);
	virtual void updatePerAccountChatLogsDir(const std::string &grid);
	virtual void dumpCurrentDirectories();
	std::string buildSLOSCacheDir() const;
	void append(std::string& destpath, const std::string& name) const;
	std::string add(const std::string& path, const std::string& name) const;
protected:
	typedef std::pair<bool, unsigned short> SepOff;
	SepOff needSep(const std::string& path, const std::string& name) const;
	void addSearchSkinDir(const std::string& skindir);
	template <typename FUNCTION>
	void walkSearchSkinDirs(const std::string& subdir,
							const std::vector<std::string>& subsubdirs,
							const std::string& filename,
							const FUNCTION& function) const;
	std::string mAppName;
	std::string mExecutablePathAndName;
	std::string mExecutableFilename;
	std::string mExecutableDir;
	std::string mWorkingDir;
	std::string mAppRODataDir;
	std::string mOSUserDir;
	std::string mOSUserAppDir;
	std::string mLindenUserDir;
	std::string mPerAccountChatLogsDir;
	std::string mChatLogsDir;
	std::string mCAFile;
	std::string mTempDir;
	std::string mCacheDir;
	std::string mDefaultCacheDir;
	std::string mOSCacheDir;
	std::string mDirDelimiter;
	std::string mSkinName;
	std::string mSkinBaseDir;
	std::string mDefaultSkinDir;
	std::string mSkinDir;
	std::string mUserDefaultSkinDir;
	std::string mUserSkinDir;
	std::vector<std::string> mSearchSkinDirs;
	std::string mLanguage;
	std::string mLLPluginDir;
    static std::string sDumpDir;
	std::string mUserName;
};
void dir_exists_or_crash(const std::string &dir_name);
extern LLDir *gDirUtilp;
#endif
