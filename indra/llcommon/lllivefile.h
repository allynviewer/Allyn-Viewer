/** 
 * @file lllivefile.h
 * @brief Automatically reloads a file whenever it changes or is removed.
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
#ifndef LL_LLLIVEFILE_H
#define LL_LLLIVEFILE_H
extern const F32 DEFAULT_CONFIG_FILE_REFRESH;
class LL_COMMON_API LLLiveFile
{
public:
	LLLiveFile(const std::string& filename, const F32 refresh_period = DEFAULT_CONFIG_FILE_REFRESH);
	virtual ~LLLiveFile();
	bool checkAndReload();
	std::string filename() const;
	void addToEventTimer();
	void setRefreshPeriod(F32 seconds);
protected:
	virtual bool loadFile() = 0;
	virtual void changed() {}
private:
	class Impl;
	Impl& impl;
};
#endif
