/** 
 * @file llpluginsharedmemory.h
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */
#ifndef LL_LLPLUGINSHAREDMEMORY_H
#define LL_LLPLUGINSHAREDMEMORY_H
#include "llaprpool.h"
class LLPluginSharedMemoryPlatformImpl;
class LLPluginSharedMemory
{
	LOG_CLASS(LLPluginSharedMemory);
public:
	LLPluginSharedMemory();
	~LLPluginSharedMemory();
	bool create(size_t size);
	bool destroy(void);
	bool attach(const std::string &name, size_t size);
	bool detach(void);
	bool isMapped(void) const { return (mMappedAddress != NULL); };
	void *getMappedAddress(void) const { return mMappedAddress; };
	size_t getSize(void) const { return mSize; };
	std::string getName() const { return mName; };
private:
	bool map(void);
	bool unmap(void);
	bool close(void);
	bool unlink(void);
	LLAPRPool mPool;
	std::string mName;
	size_t mSize;
	void *mMappedAddress;
	bool mNeedsDestroy;
	LLPluginSharedMemoryPlatformImpl *mImpl;
	static int sSegmentNumber;
	static std::string createName();
};
#endif
