/** 
 * @file llpluginsharedmemory.cpp
 * LLPluginSharedMemory manages a shared memory segment for use by the LLPlugin API.
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
#include "linden_common.h"
#include "llpluginsharedmemory.h"
#if LL_WINDOWS
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#if LL_WINDOWS
	#define USE_WIN32_SHARED_MEMORY 1
#elif LL_DARWIN
	#define USE_SHM_OPEN_SHARED_MEMORY 1
#endif
#if LL_WINDOWS
	#define APR_SHARED_MEMORY_PREFIX_STRING "C:\\LLPlugin_"
	#define WIN32_SHARED_MEMORY_PREFIX_STRING "Local\\LL_"
#else
	#define APR_SHARED_MEMORY_PREFIX_STRING "/tmp/LLPlugin_"
	#define SHM_OPEN_SHARED_MEMORY_PREFIX_STRING "/LL"
#endif
#if USE_APR_SHARED_MEMORY
	#include "llapr.h"
	#include "apr_shm.h"
#elif USE_SHM_OPEN_SHARED_MEMORY
	#include <sys/fcntl.h>
	#include <sys/mman.h>
	#include <errno.h>
#elif USE_WIN32_SHARED_MEMORY
#include <windows.h>
#endif
int LLPluginSharedMemory::sSegmentNumber = 0;
std::string LLPluginSharedMemory::createName(void)
{
	std::stringstream newname;
#if LL_WINDOWS
	newname << GetCurrentProcessId();
#else
	newname << getpid();
#endif
	newname << "_" << sSegmentNumber++;
	return newname.str();
}
class LLPluginSharedMemoryPlatformImpl
{
public:
	LLPluginSharedMemoryPlatformImpl();
	~LLPluginSharedMemoryPlatformImpl();
#if USE_APR_SHARED_MEMORY
	apr_shm_t* mAprSharedMemory;
#elif USE_SHM_OPEN_SHARED_MEMORY
	int mSharedMemoryFD;
#elif USE_WIN32_SHARED_MEMORY
	HANDLE mMapFile;
#endif
};
LLPluginSharedMemory::LLPluginSharedMemory()
{
	mSize = 0;
	mMappedAddress = NULL;
	mNeedsDestroy = false;
	mImpl = new LLPluginSharedMemoryPlatformImpl;
}
LLPluginSharedMemory::~LLPluginSharedMemory()
{
	if(mNeedsDestroy)
		destroy();
	else
		detach();
	unlink();
	delete mImpl;
}
#if USE_APR_SHARED_MEMORY
LLPluginSharedMemoryPlatformImpl::LLPluginSharedMemoryPlatformImpl()
{
	mAprSharedMemory = NULL;
}
LLPluginSharedMemoryPlatformImpl::~LLPluginSharedMemoryPlatformImpl()
{
}
bool LLPluginSharedMemory::map(void)
{
	mMappedAddress = apr_shm_baseaddr_get(mImpl->mAprSharedMemory);
	if(mMappedAddress == NULL)
	{
		return false;
	}
	return true;
}
bool LLPluginSharedMemory::unmap(void)
{
	return true;
}
bool LLPluginSharedMemory::close(void)
{
	return true;
}
bool LLPluginSharedMemory::unlink(void)
{
	return true;
}
bool LLPluginSharedMemory::create(size_t size)
{
	mName = APR_SHARED_MEMORY_PREFIX_STRING;
	mName += createName();
	mSize = size;
	mPool.create();
	apr_status_t status = apr_shm_create( &(mImpl->mAprSharedMemory), mSize, mName.c_str(), mPool());
	if(ll_apr_warn_status(status))
	{
		return false;
	}
	mNeedsDestroy = true;
	return map();
}
bool LLPluginSharedMemory::destroy(void)
{
	if(mImpl->mAprSharedMemory)
	{
		apr_status_t status = apr_shm_destroy(mImpl->mAprSharedMemory);
		if(ll_apr_warn_status(status))
		{
		}
		mImpl->mAprSharedMemory = NULL;
	}
	mPool.destroy();
	return true;
}
bool LLPluginSharedMemory::attach(const std::string &name, size_t size)
{
	mName = name;
	mSize = size;
	mPool.create();
	apr_status_t status = apr_shm_attach( &(mImpl->mAprSharedMemory), mName.c_str(), mPool() );
	if(ll_apr_warn_status(status))
	{
		return false;
	}
	return map();
}
bool LLPluginSharedMemory::detach(void)
{
	if(mImpl->mAprSharedMemory)
	{
		apr_status_t status = apr_shm_detach(mImpl->mAprSharedMemory);
		if(ll_apr_warn_status(status))
		{
		}
		mImpl->mAprSharedMemory = NULL;
	}
	mPool.destroy();
	return true;
}
#elif USE_SHM_OPEN_SHARED_MEMORY
LLPluginSharedMemoryPlatformImpl::LLPluginSharedMemoryPlatformImpl()
{
	mSharedMemoryFD = -1;
}
LLPluginSharedMemoryPlatformImpl::~LLPluginSharedMemoryPlatformImpl()
{
}
bool LLPluginSharedMemory::map(void)
{
    llassert(mSize);
    if (!mSize)
    {
        LL_DEBUGS("Plugin") << "Tried to mmap zero length" << LL_ENDL;
        return false;
    }
    llassert(mImpl->mSharedMemoryFD != -1);
    llassert(fcntl(mImpl->mSharedMemoryFD, F_GETFL) != -1);
	mMappedAddress = ::mmap(NULL, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, mImpl->mSharedMemoryFD, 0);
	if(mMappedAddress == NULL)
	{
		return false;
	}
	LL_DEBUGS("Plugin") << "memory mapped at " << mMappedAddress << LL_ENDL;
	return true;
}
bool LLPluginSharedMemory::unmap(void)
{
	if(mMappedAddress != NULL)
	{
		LL_DEBUGS("Plugin") << "calling munmap(" << mMappedAddress << ", " << mSize << ")" << LL_ENDL;
		if(::munmap(mMappedAddress, mSize) == -1)
		{
		}
		mMappedAddress = NULL;
	}
	return true;
}
bool LLPluginSharedMemory::close(void)
{
	if(mImpl->mSharedMemoryFD != -1)
	{
		LL_DEBUGS("Plugin") << "calling close(" << mImpl->mSharedMemoryFD << ")" << LL_ENDL;
		if(::close(mImpl->mSharedMemoryFD) == -1)
		{
		}
		mImpl->mSharedMemoryFD = -1;
	}
	return true;
}
bool LLPluginSharedMemory::unlink(void)
{
	if(!mName.empty())
	{
		if(::shm_unlink(mName.c_str()) == -1)
		{
			return false;
		}
	}
	return true;
}
bool LLPluginSharedMemory::create(size_t size)
{
	mName = SHM_OPEN_SHARED_MEMORY_PREFIX_STRING;
	mName += createName();
	mSize = size;
	unlink();
	mImpl->mSharedMemoryFD = ::shm_open(mName.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if(mImpl->mSharedMemoryFD == -1)
	{
		return false;
	}
	mNeedsDestroy = true;
	if(::ftruncate(mImpl->mSharedMemoryFD, mSize) == -1)
	{
		return false;
	}
	return map();
}
bool LLPluginSharedMemory::destroy(void)
{
	unmap();
	close();
	return true;
}
bool LLPluginSharedMemory::attach(const std::string &name, size_t size)
{
	mName = name;
	mSize = size;
	mImpl->mSharedMemoryFD = ::shm_open(mName.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
	if(mImpl->mSharedMemoryFD == -1)
	{
		return false;
	}
	unlink();
	return map();
}
bool LLPluginSharedMemory::detach(void)
{
	unmap();
	close();
	return true;
}
#elif USE_WIN32_SHARED_MEMORY
LLPluginSharedMemoryPlatformImpl::LLPluginSharedMemoryPlatformImpl()
{
	mMapFile = NULL;
}
LLPluginSharedMemoryPlatformImpl::~LLPluginSharedMemoryPlatformImpl()
{
}
bool LLPluginSharedMemory::map(void)
{
	mMappedAddress = MapViewOfFile(
		mImpl->mMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		mSize);
	if(mMappedAddress == NULL)
	{
		LL_WARNS("Plugin") << "MapViewOfFile failed: " << GetLastError() << LL_ENDL;
		return false;
	}
	LL_DEBUGS("Plugin") << "memory mapped at " << mMappedAddress << LL_ENDL;
	return true;
}
bool LLPluginSharedMemory::unmap(void)
{
	if(mMappedAddress != NULL)
	{
		UnmapViewOfFile(mMappedAddress);
		mMappedAddress = NULL;
	}
	return true;
}
bool LLPluginSharedMemory::close(void)
{
	if(mImpl->mMapFile != NULL)
	{
		CloseHandle(mImpl->mMapFile);
		mImpl->mMapFile = NULL;
	}
	return true;
}
bool LLPluginSharedMemory::unlink(void)
{
	return true;
}
bool LLPluginSharedMemory::create(size_t size)
{
	mName = WIN32_SHARED_MEMORY_PREFIX_STRING;
	mName += createName();
	mSize = size;
	mImpl->mMapFile = CreateFileMappingA(
                 INVALID_HANDLE_VALUE,
                 NULL,
                 PAGE_READWRITE,
                 0,
                 mSize,
                 mName.c_str());
	if(mImpl->mMapFile == NULL)
	{
		LL_WARNS("Plugin") << "CreateFileMapping failed: " << GetLastError() << LL_ENDL;
		return false;
	}
	mNeedsDestroy = true;
	return map();
}
bool LLPluginSharedMemory::destroy(void)
{
	unmap();
	close();
	return true;
}
bool LLPluginSharedMemory::attach(const std::string &name, size_t size)
{
	mName = name;
	mSize = size;
	mImpl->mMapFile = OpenFileMappingA(
				FILE_MAP_ALL_ACCESS,
				FALSE,
				mName.c_str());
	if(mImpl->mMapFile == NULL)
	{
		LL_WARNS("Plugin") << "OpenFileMapping failed: " << GetLastError() << LL_ENDL;
		return false;
	}
	return map();
}
bool LLPluginSharedMemory::detach(void)
{
	unmap();
	close();
	return true;
}
#endif
