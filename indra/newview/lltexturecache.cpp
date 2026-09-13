/** 
 * @file lltexturecache.cpp
 * @brief Object which handles local texture caching
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
#include "llviewerprecompiledheaders.h"
#include "lltexturecache.h"
#include "llapr.h"
#include "lldir.h"
#include "llimage.h"
#include "lllfsthread.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llmemory.h"
const S32 TEXTURE_CACHE_ENTRY_SIZE = FIRST_PACKET_SIZE;
const S32 TEXTURE_FAST_CACHE_ENTRY_OVERHEAD = sizeof(S32) * 4;
const S32 TEXTURE_FAST_CACHE_DATA_SIZE = 16 * 16 * 4;
const S32 TEXTURE_FAST_CACHE_ENTRY_SIZE = TEXTURE_FAST_CACHE_DATA_SIZE + TEXTURE_FAST_CACHE_ENTRY_OVERHEAD;
const F32 TEXTURE_CACHE_PURGE_AMOUNT = .20f;
const F32 TEXTURE_CACHE_LRU_SIZE = .10f;
static std::queue<LLUUID> sgDelayedPurgeQueue;
class LLTextureCacheWorker : public LLWorkerClass
{
	friend class LLTextureCache;
private:
	class ReadResponder : public LLLFSThread::Responder
	{
	public:
		ReadResponder(LLTextureCache* cache, handle_t handle) : mCache(cache), mHandle(handle) {}
		~ReadResponder() {}
		void completed(S32 bytes)
		{
			mCache->lockWorkers();
			LLTextureCacheWorker* reader = mCache->getReader(mHandle);
			if (reader) reader->ioComplete(bytes);
			mCache->unlockWorkers();
		}
		LLTextureCache* mCache;
		LLTextureCacheWorker::handle_t mHandle;
	};
	class WriteResponder : public LLLFSThread::Responder
	{
	public:
		WriteResponder(LLTextureCache* cache, handle_t handle) : mCache(cache), mHandle(handle) {}
		~WriteResponder() {}
		void completed(S32 bytes)
		{
			mCache->lockWorkers();
			LLTextureCacheWorker* writer = mCache->getWriter(mHandle);
			if (writer) writer->ioComplete(bytes);
			mCache->unlockWorkers();
		}
		LLTextureCache* mCache;
		LLTextureCacheWorker::handle_t mHandle;
	};
public:
	LLTextureCacheWorker(LLTextureCache* cache, U32 priority, const LLUUID& id,
						 U8* data, S32 datasize, S32 offset,
						 S32 imagesize,
						 LLTextureCache::Responder* responder,
						 LLImageRaw* rawimage = NULL, S32 discardlevel = -1)
		: LLWorkerClass(cache, "LLTextureCacheWorker"),
		  mID(id),
		  mCache(cache),
		  mPriority(priority),
		  mReadData(NULL),
		  mWriteData(data),
		  mDataSize(datasize),
		  mOffset(offset),
		  mImageSize(imagesize),
		  mImageFormat(IMG_CODEC_J2C),
		  mImageLocal(FALSE),
		  mResponder(responder),
		  mFileHandle(LLLFSThread::nullHandle()),
		  mBytesToRead(0),
		  mBytesRead(0),
		  mRawImage(rawimage),
		  mRawDiscardLevel(discardlevel)
	{
		mPriority &= LLWorkerThread::PRIORITY_LOWBITS;
	}
	~LLTextureCacheWorker()
	{
		llassert_always(!haveWork());
		FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
	}
	virtual bool doRead() = 0;
	virtual bool doWrite() = 0;
	virtual bool doWork(S32 param);
	handle_t read() { addWork(0, LLWorkerThread::PRIORITY_HIGH | mPriority); return mRequestHandle; }
	handle_t write() { addWork(1, LLWorkerThread::PRIORITY_HIGH | mPriority); return mRequestHandle; }
	bool complete() { return checkWork(); }
	void ioComplete(S32 bytes)
	{
		mBytesRead = bytes;
		setPriority(LLWorkerThread::PRIORITY_HIGH | mPriority);
	}
private:
	virtual void startWork(S32 param);
	virtual void finishWork(S32 param, bool completed);
	virtual void endWork(S32 param, bool aborted);
protected:
	LLTextureCache* mCache;
	U32 mPriority;
	LLUUID	mID;
	U8* mReadData;
	U8* mWriteData;
	S32 mDataSize;
	S32 mOffset;
	S32 mImageSize;
	EImageCodec mImageFormat;
	BOOL mImageLocal;
	LLPointer<LLTextureCache::Responder> mResponder;
	LLLFSThread::handle_t mFileHandle;
	S32 mBytesToRead;
	LLAtomicS32 mBytesRead;
	LLPointer<LLImageRaw> mRawImage;
	S32 mRawDiscardLevel;
};
class LLTextureCacheLocalFileWorker : public LLTextureCacheWorker
{
public:
	LLTextureCacheLocalFileWorker(LLTextureCache* cache, U32 priority, const std::string& filename, const LLUUID& id,
						 U8* data, S32 datasize, S32 offset,
						 S32 imagesize,
						 LLTextureCache::Responder* responder)
			: LLTextureCacheWorker(cache, priority, id, data, datasize, offset, imagesize, responder),
			mFileName(filename)
	{
	}
	virtual bool doRead();
	virtual bool doWrite();
private:
	std::string	mFileName;
};
bool LLTextureCacheLocalFileWorker::doRead()
{
	S32 local_size = LLAPRFile::size(mFileName);
	if (local_size > 0 && mFileName.size() > 4)
	{
		mDataSize = local_size;
		std::string extension = mFileName.substr(mFileName.size() - 3, 3);
		mImageFormat = LLImageBase::getCodecFromExtension(extension);
		if (mImageFormat == IMG_CODEC_INVALID)
		{
			mDataSize = 0;
			return true;
		}
	}
	else
	{
		mDataSize = 0;
		return true;
	}
#if USE_LFS_READ
	if (mFileHandle == LLLFSThread::nullHandle())
	{
		mImageLocal = TRUE;
		mImageSize = local_size;
		if (!mDataSize || mDataSize + mOffset > local_size)
		{
			mDataSize = local_size - mOffset;
		}
		if (mDataSize <= 0)
		{
			mDataSize = 0;
			return true;
		}
		mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);
		mBytesRead = -1;
		mBytesToRead = mDataSize;
		setPriority(LLWorkerThread::PRIORITY_LOW | mPriority);
		mFileHandle = LLLFSThread::sLocal->read(local_filename, mReadData, mOffset, mDataSize,
												new ReadResponder(mCache, mRequestHandle));
		return false;
	}
	else
	{
		if (mBytesRead >= 0)
		{
			if (mBytesRead != mBytesToRead)
			{
				mDataSize = 0;
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
			}
			return true;
		}
		else
		{
			return false;
		}
	}
#else
	if (!mDataSize || mDataSize > local_size)
	{
		mDataSize = local_size;
	}
	mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);
	S32 bytes_read = LLAPRFile::readEx(mFileName, mReadData, mOffset, mDataSize);
	if (bytes_read != mDataSize)
	{
		mDataSize = 0;
		FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
		mReadData = NULL;
	}
	else
	{
		mImageSize = local_size;
		mImageLocal = TRUE;
	}
	return true;
#endif
}
bool LLTextureCacheLocalFileWorker::doWrite()
{
	return false;
}
class LLTextureCacheRemoteWorker : public LLTextureCacheWorker
{
public:
	LLTextureCacheRemoteWorker(LLTextureCache* cache, U32 priority, const LLUUID& id,
						 U8* data, S32 datasize, S32 offset,
						 S32 imagesize,
						 LLTextureCache::Responder* responder,
						 LLImageRaw* rawimage = NULL, S32 discardlevel = -1)
			: LLTextureCacheWorker(cache, priority, id, data, datasize, offset, imagesize, responder, rawimage, discardlevel),
			mState(INIT)
	{
	}
	virtual bool doRead();
	virtual bool doWrite();
private:
	enum e_state
	{
		INIT = 0,
		LOCAL = 1,
		CACHE = 2,
		HEADER = 3,
		BODY = 4
	};
	e_state mState;
};
void LLTextureCacheWorker::startWork(S32 param)
{
}
bool LLTextureCacheRemoteWorker::doRead()
{
	bool done = false;
	S32 idx = -1;
	S32 local_size = 0;
	std::string local_filename;
	if (mState == INIT)
	{
#if 0
		std::string filename = mCache->getLocalFileName(mID);
		{
			local_filename = filename + ".j2c";
			local_size = LLAPRFile::size(local_filename);
			if (local_size > 0)
			{
				mImageFormat = IMG_CODEC_J2C;
			}
		}
		if (local_size == 0)
		{
			local_filename = filename + ".jpg";
			local_size = LLAPRFile::size(local_filename);
			if (local_size > 0)
			{
				mImageFormat = IMG_CODEC_JPEG;
				mDataSize = local_size;
			}
		}
		if (local_size == 0)
		{
			local_filename = filename + ".tga";
			local_size = LLAPRFile::size(local_filename);
			if (local_size > 0)
			{
				mImageFormat = IMG_CODEC_TGA;
				mDataSize = local_size;
			}
		}
		mState = (local_size > 0 ? LOCAL : CACHE);
		llassert_always(mState == CACHE) ;
#else
		mState = CACHE;
#endif
	}
	if (!done && (mState == LOCAL))
	{
		llassert(local_size != 0);
		if (!mDataSize || mDataSize > local_size)
		{
			mDataSize = local_size;
		}
		mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);
		S32 bytes_read = LLAPRFile::readEx(local_filename, mReadData, mOffset, mDataSize);
		if (bytes_read != mDataSize)
		{
 			LL_WARNS() << "Error reading file from local cache: " << local_filename
 					<< " Bytes: " << mDataSize << " Offset: " << mOffset
 					<< " / " << mDataSize << LL_ENDL;
			mDataSize = 0;
			FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
			mReadData = NULL;
		}
		else
		{
			mImageSize = local_size;
			mImageLocal = TRUE;
		}
		done = true;
	}
	if (!done && (mState == CACHE))
	{
		LLTextureCache::Entry entry;
		idx = mCache->getHeaderCacheEntry(mID, entry);
		if (idx < 0)
		{
			mDataSize = 0;
			done = true;
		}
		else
		{
			mImageSize = entry.mImageSize;
			mState = mOffset < TEXTURE_CACHE_ENTRY_SIZE ? HEADER : BODY;
		}
	}
	if (!done && (mState == HEADER))
	{
		llassert_always(idx >= 0);
		llassert_always(mOffset < TEXTURE_CACHE_ENTRY_SIZE);
		S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE + mOffset;
		S32 size = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
		size = llmin(size, mDataSize);
		mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), size);
		S32 bytes_read = LLAPRFile::readEx(mCache->mHeaderDataFileName, mReadData, offset, size);
		if (bytes_read != size)
		{
			LL_WARNS() << "LLTextureCacheWorker: "  << mID
					<< " incorrect number of bytes read from header: " << bytes_read
					<< " / " << size << LL_ENDL;
			FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
			mReadData = NULL;
			mDataSize = -1;
			done = true;
		}
		if (mDataSize <= bytes_read)
		{
			done = true;
		}
		else
		{
			mState = BODY;
		}
	}
	if (!done && (mState == BODY))
	{
		std::string filename = mCache->getTextureFileName(mID);
		S32 filesize = LLAPRFile::size(filename);
		if (filesize && (filesize + TEXTURE_CACHE_ENTRY_SIZE) > mOffset)
		{
			S32 max_datasize = TEXTURE_CACHE_ENTRY_SIZE + filesize - mOffset;
			mDataSize = llmin(max_datasize, mDataSize);
			S32 data_offset, file_size, file_offset;
			U8* data = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);
			if (mOffset < TEXTURE_CACHE_ENTRY_SIZE)
			{
				data_offset = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
				file_offset = 0;
				file_size = mDataSize - data_offset;
				llassert_always(mReadData);
				memcpy(data, mReadData, data_offset);
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
			}
			else
			{
				data_offset = 0;
				file_offset = mOffset - TEXTURE_CACHE_ENTRY_SIZE;
				file_size = mDataSize;
			}
			llassert_always(mReadData == NULL);
			mReadData = data;
			S32 bytes_read = LLAPRFile::readEx(filename,
											 mReadData + data_offset,
											 file_offset, file_size);
			if (bytes_read != file_size)
			{
				LL_DEBUGS("TextureCache") << "LLTextureCacheWorker: "  << mID
						<< " incorrect number of bytes read from body: " << bytes_read
						<< " / " << file_size << LL_ENDL;
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
				mDataSize = -1;
				done = true;
			}
		}
		else
		{
			mDataSize = llmax(TEXTURE_CACHE_ENTRY_SIZE - mOffset, 0);
			LL_DEBUGS() << "No body file for: " << filename << LL_ENDL;
		}
		done = true;
	}
	return done;
}
bool LLTextureCacheRemoteWorker::doWrite()
{
	bool done = false;
	S32 idx = -1;
	if (mState == INIT)
	{
		llassert_always(mOffset == 0);
		llassert_always(mDataSize > 0);
		llassert_always(mImageSize >= mDataSize);
		mState = CACHE;
	}
	if (!done && (mState == CACHE))
	{
		bool alreadyCached = false;
		LLTextureCache::Entry entry;
		idx = mCache->getHeaderCacheEntry(mID, entry);
		if(idx < 0)
		{
			idx = mCache->setHeaderCacheEntry(mID, entry, mImageSize, mDataSize);
		}
		else
		{
			alreadyCached = mCache->updateEntry(idx, entry, mImageSize, mDataSize);
		}
		if (idx < 0)
		{
			LL_WARNS() << "LLTextureCacheWorker: "  << mID
					<< " Unable to create header entry for writing!" << LL_ENDL;
			mDataSize = -1;
			done = true;
		}
		else
		{
			if (alreadyCached && (mDataSize <= TEXTURE_CACHE_ENTRY_SIZE))
			{
				done = true;
			}
			else
			{
				mState = alreadyCached ? BODY : HEADER;
			}
		}
	}
	if (!done && (mState == HEADER))
	{
		llassert_always(idx >= 0);
		S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE;
		S32 size = TEXTURE_CACHE_ENTRY_SIZE;
		S32 bytes_written;
		if (mDataSize < TEXTURE_CACHE_ENTRY_SIZE)
		{
			U8* padBuffer = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), TEXTURE_CACHE_ENTRY_SIZE);
			memset(padBuffer, 0, TEXTURE_CACHE_ENTRY_SIZE);
			memcpy(padBuffer, mWriteData, mDataSize);
			bytes_written = LLAPRFile::writeEx(mCache->mHeaderDataFileName, padBuffer, offset, size);
			FREE_MEM(LLImageBase::getPrivatePool(), padBuffer);
		}
		else
		{
			bytes_written = LLAPRFile::writeEx(mCache->mHeaderDataFileName, mWriteData, offset, size);
		}
		if (bytes_written <= 0)
		{
			LL_WARNS() << "LLTextureCacheWorker: "  << mID
					<< " Unable to write header entry!" << LL_ENDL;
			mDataSize = -1;
			done = true;
		}
		if (mDataSize <= bytes_written)
		{
			done = true;
		}
		else
		{
			mState = BODY;
		}
	}
	if (!done && (mState == BODY))
	{
		llassert(mDataSize > TEXTURE_CACHE_ENTRY_SIZE);
		S32 file_size = mDataSize - TEXTURE_CACHE_ENTRY_SIZE;
		{
			std::string filename = mCache->getTextureFileName(mID);
			S32 bytes_written = LLAPRFile::writeEx(	filename,
													mWriteData + TEXTURE_CACHE_ENTRY_SIZE,
													0, file_size);
			if (bytes_written <= 0)
			{
				LL_WARNS() << "LLTextureCacheWorker: "  << mID
						<< " incorrect number of bytes written to body: " << bytes_written
						<< " / " << file_size << LL_ENDL;
				mDataSize = -1;
				done = true;
			}
		}
		done = true;
	}
	if (done && mDataSize > 0 && idx >= 0 && mRawImage.notNull())
	{
		mCache->writeToFastCache(mID, idx, mRawImage, mRawDiscardLevel);
	}
	return done;
}
bool LLTextureCacheWorker::doWork(S32 param)
{
	bool res = false;
	if (param == 0)
	{
		res = doRead();
	}
	else if (param == 1)
	{
		res = doWrite();
	}
	else
	{
		llassert_always(0);
	}
	return res;
}
void LLTextureCacheWorker::finishWork(S32 param, bool completed)
{
	if (mResponder.notNull())
	{
		bool success = (completed && mDataSize > 0);
		if (param == 0)
		{
			if (success)
			{
				mResponder->setData(mReadData, mDataSize, mImageSize, mImageFormat, mImageLocal);
				mReadData = NULL;
				mDataSize = 0;
			}
			else
			{
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
			}
		}
		else
		{
			mWriteData = NULL;
			mDataSize = 0;
		}
		mCache->addCompleted(mResponder, success);
	}
}
void LLTextureCacheWorker::endWork(S32 param, bool aborted)
{
	if (aborted)
	{
		return;
	}
	switch(param)
	{
	  default:
	  case 0:
	  case 1:
	  {
		  if (mDataSize < 0)
		  {
			  mCache->removeFromCache(mID);
		  }
		  break;
	  }
	}
}
LLTextureCache::LLTextureCache(bool threaded)
	: LLWorkerThread("TextureCache", threaded),
	  mHeaderAPRFile(NULL),
	  mReadOnly(TRUE),
	  mTexturesSizeTotal(0),
	  mDoPurge(FALSE),
	  mFastCachePadBuffer(NULL)
{
}
LLTextureCache::~LLTextureCache()
{
	clearDeleteList();
	writeUpdatedEntries();
	if (mFastCachePadBuffer)
	{
		ll_aligned_free_16(mFastCachePadBuffer);
		mFastCachePadBuffer = NULL;
	}
}
S32 LLTextureCache::update(F32 max_time_ms)
{
	static LLFrameTimer timer;
	static const F32 MAX_TIME_INTERVAL = 300.f;
	S32 res;
	res = LLWorkerThread::update(max_time_ms);
	mListMutex.lock();
	handle_list_t priorty_list = mPrioritizeWriteList;
	mPrioritizeWriteList.clear();
	responder_list_t completed_list = mCompletedList;
	mCompletedList.clear();
	mListMutex.unlock();
	lockWorkers();
	for (handle_list_t::iterator iter1 = priorty_list.begin();
		 iter1 != priorty_list.end(); ++iter1)
	{
		handle_t handle = *iter1;
		handle_map_t::iterator iter2 = mWriters.find(handle);
		if(iter2 != mWriters.end())
		{
			LLTextureCacheWorker* worker = iter2->second;
			worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mPriority);
		}
	}
	unlockWorkers();
	for (responder_list_t::iterator iter1 = completed_list.begin();
		 iter1 != completed_list.end(); ++iter1)
	{
		Responder *responder = iter1->first;
		bool success = iter1->second;
		responder->completed(success);
	}
	if(!res && timer.getElapsedTimeF32() > MAX_TIME_INTERVAL)
	{
		timer.reset();
		writeUpdatedEntries();
	}
	return res;
}
std::string LLTextureCache::getLocalFileName(const LLUUID& id)
{
	std::string idstr = id.asString();
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_LOCAL_ASSETS, idstr);
	return filename;
}
std::string LLTextureCache::getTextureFileName(const LLUUID& id)
{
	std::string idstr = id.asString();
	std::string delem = gDirUtilp->getDirDelimiter();
	std::string filename = mTexturesDirName + delem + idstr[0] + delem + idstr + ".texture";
	return filename;
}
BOOL LLTextureCache::isInCache(const LLUUID& id)
{
	LLMutexLock lock(&mHeaderMutex);
	id_map_t::const_iterator iter = mHeaderIDMap.find(id);
	return (iter != mHeaderIDMap.end()) ;
}
BOOL LLTextureCache::isInLocal(const LLUUID& id)
{
	S32 local_size = 0;
	std::string local_filename;
	std::string filename = getLocalFileName(id);
			{
		local_filename = filename + ".j2c";
		local_size = LLAPRFile::size(local_filename);
		if (local_size > 0)
			{
			return TRUE;
		}
			}
			{
		local_filename = filename + ".jpg";
		local_size = LLAPRFile::size(local_filename);
		if (local_size > 0)
		{
			return TRUE;
		}
	}
	{
		local_filename = filename + ".tga";
		local_size = LLAPRFile::size(local_filename);
		if (local_size > 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}
const S32 MAX_REASONABLE_FILE_SIZE = 512*1024*1024;
F32 LLTextureCache::sHeaderCacheVersion = 1.8f;
U32 LLTextureCache::sCacheMaxEntries = MAX_REASONABLE_FILE_SIZE / TEXTURE_CACHE_ENTRY_SIZE;
S64 LLTextureCache::sCacheMaxTexturesSize = 0;
const char* entries_filename = "texture.entries";
const char* cache_filename = "texture.cache";
const char* fast_cache_filename = "texture.fastcache";
const char* old_textures_dirname = "textures";
const char* textures_dirname = "texturecache";
void LLTextureCache::setDirNames(ELLPath location)
{
	std::string delem = gDirUtilp->getDirDelimiter();
	mHeaderEntriesFileName = gDirUtilp->getExpandedFilename(location, textures_dirname, entries_filename);
	mHeaderDataFileName = gDirUtilp->getExpandedFilename(location, textures_dirname, cache_filename);
	mFastCacheFileName = gDirUtilp->getExpandedFilename(location, textures_dirname, fast_cache_filename);
	mTexturesDirName = gDirUtilp->getExpandedFilename(location, textures_dirname);
}
void LLTextureCache::purgeCache(ELLPath location)
{
	LLMutexLock lock(&mHeaderMutex);
	if (!mReadOnly)
	{
		setDirNames(location);
		llassert_always(mHeaderAPRFile == NULL);
		std::string texture_dir = mTexturesDirName;
		mTexturesDirName = gDirUtilp->getExpandedFilename(location, old_textures_dirname);
		if(LLFile::isdir(mTexturesDirName))
		{
			std::string file_name = gDirUtilp->getExpandedFilename(location, entries_filename);
			if(LLAPRFile::isExist(file_name))
				LLAPRFile::remove(file_name);
			file_name = gDirUtilp->getExpandedFilename(location, cache_filename);
			if(LLAPRFile::isExist(file_name))
				LLAPRFile::remove(file_name);
			purgeAllTextures(true);
	}
		mTexturesDirName = texture_dir;
	}
	purgeAllTextures(true);
}
void LLTextureCache::setReadOnly(BOOL read_only)
{
	mReadOnly = read_only;
}
U64 LLTextureCache::initCache(ELLPath location, U64 max_size, BOOL texture_cache_mismatch)
{
	llassert_always(getPending() == 0);
	U64 header_size = (max_size * 36) / 100;
	U32 max_entries = (U32)(header_size / (TEXTURE_CACHE_ENTRY_SIZE + TEXTURE_FAST_CACHE_ENTRY_SIZE));
	sCacheMaxEntries = (llmin(sCacheMaxEntries, max_entries));
	header_size = sCacheMaxEntries * (TEXTURE_CACHE_ENTRY_SIZE + TEXTURE_FAST_CACHE_ENTRY_SIZE);
	max_size -= header_size;
	if (sCacheMaxTexturesSize > 0)
		sCacheMaxTexturesSize = (U32)llmin((U64)sCacheMaxTexturesSize, max_size);
	else
		sCacheMaxTexturesSize = max_size;
	max_size -= sCacheMaxTexturesSize;
	LL_INFOS("TextureCache") << "Headers: " << sCacheMaxEntries
			<< " Textures size: " << sCacheMaxTexturesSize / (1024 * 1024) << " MB" << LL_ENDL;
	setDirNames(location);
	if(texture_cache_mismatch)
	{
		purgeAllTextures(true);
		if(mReadOnly)
		{
			return max_size;
		}
	}
	if (!mReadOnly)
	{
		LLFile::mkdir(mTexturesDirName);
		const char* subdirs = "0123456789abcdef";
		for (S32 i=0; i<16; i++)
		{
			std::string dirname = mTexturesDirName + gDirUtilp->getDirDelimiter() + subdirs[i];
			LLFile::mkdir(dirname);
		}
	}
	readHeaderCache();
	purgeTextures(true);
	llassert_always(getPending() == 0);
	return max_size;
}
LLAPRFile* LLTextureCache::openHeaderEntriesFile(bool readonly, S32 offset)
{
	llassert_always(mHeaderAPRFile == NULL);
	apr_int32_t flags = readonly ? APR_READ|APR_BINARY : APR_READ|APR_WRITE|APR_BINARY;
	mHeaderAPRFile = new LLAPRFile(mHeaderEntriesFileName, flags);
	if(offset > 0)
	{
	mHeaderAPRFile->seek(APR_SET, offset);
	}
	return mHeaderAPRFile;
}
void LLTextureCache::closeHeaderEntriesFile()
{
	if(!mHeaderAPRFile)
	{
		return;
	}
	delete mHeaderAPRFile;
	mHeaderAPRFile = NULL;
}
void LLTextureCache::readEntriesHeader()
{
		llassert_always(mHeaderAPRFile == NULL);
	if (LLAPRFile::isExist(mHeaderEntriesFileName))
	{
		LLAPRFile::readEx(mHeaderEntriesFileName, (U8*)&mHeaderEntriesInfo, 0, sizeof(EntriesInfo));
	}
	else
	{
		mHeaderEntriesInfo.mVersion = sHeaderCacheVersion;
		mHeaderEntriesInfo.mEntries = 0;
		writeEntriesHeader();
	}
}
void LLTextureCache::writeEntriesHeader()
{
	llassert_always(mHeaderAPRFile == NULL);
	if (!mReadOnly)
	{
		LLAPRFile::writeEx(mHeaderEntriesFileName, (U8*)&mHeaderEntriesInfo, 0, sizeof(EntriesInfo));
	}
}
S32 LLTextureCache::openAndReadEntry(const LLUUID& id, Entry& entry, bool create)
{
	S32 idx = -1;
	id_map_t::iterator iter1 = mHeaderIDMap.find(id);
	if (iter1 != mHeaderIDMap.end())
	{
		idx = iter1->second;
	}
	if (idx < 0)
	{
		if (create && !mReadOnly)
		{
			if (mHeaderEntriesInfo.mEntries < sCacheMaxEntries)
			{
				idx = mHeaderEntriesInfo.mEntries++;
			}
			else if (!mFreeList.empty())
			{
				idx = *(mFreeList.begin());
				mFreeList.erase(mFreeList.begin());
			}
			else
			{
				for (auto iter2 = mLRU.begin(); iter2 != mLRU.end();)
				{
					auto curiter2 = iter2++;
					LLUUID oldid = *curiter2;
					mLRU.erase(curiter2);
					id_map_t::iterator iter3 = mHeaderIDMap.find(oldid);
					if (iter3 != mHeaderIDMap.end() && iter3->second >= 0)
					{
						idx = iter3->second;
						removeCachedTexture(oldid);
						break;
					}
				}
			}
			if (idx >= 0)
			{
				entry.mID = id;
				entry.mImageSize = -1;
				entry.mBodySize = 0;
			}
		}
	}
	else
	{
		mLRU.erase(id);
		idx_entry_map_t::iterator iter = mUpdatedEntryMap.find(idx);
		if(iter != mUpdatedEntryMap.end())
		{
			entry = iter->second;
		}
		else
		{
			readEntryFromHeaderImmediately(idx, entry);
		}
		if(entry.mImageSize <= entry.mBodySize)
		{
			LL_WARNS() << "corrupted entry: " << id << " entry image size: " << entry.mImageSize << " entry body size: " << entry.mBodySize << LL_ENDL;
			std::string tex_filename = getTextureFileName(id);
			removeEntry(idx, entry, tex_filename);
			mUpdatedEntryMap.erase(idx);
			idx = -1;
		}
	}
	return idx;
}
void LLTextureCache::writeEntryToHeaderImmediately(S32& idx, Entry& entry, bool write_header)
{
	LLAPRFile* aprfile;
	S32 bytes_written;
	S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);
	if(write_header)
	{
		aprfile = openHeaderEntriesFile(false, 0);
		bytes_written = aprfile->write((U8*)&mHeaderEntriesInfo, sizeof(EntriesInfo));
		if(bytes_written != sizeof(EntriesInfo))
		{
			clearCorruptedCache();
			idx = -1;
			return;
		}
		mHeaderAPRFile->seek(APR_SET, offset);
	}
	else
	{
		aprfile = openHeaderEntriesFile(false, offset);
	}
	bytes_written = aprfile->write((void*)&entry, (S32)sizeof(Entry));
	if(bytes_written != sizeof(Entry))
	{
		clearCorruptedCache();
		idx = -1;
		return;
	}
	closeHeaderEntriesFile();
	mUpdatedEntryMap.erase(idx);
}
void LLTextureCache::readEntryFromHeaderImmediately(S32& idx, Entry& entry)
{
		S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);
		LLAPRFile* aprfile = openHeaderEntriesFile(true, offset);
		S32 bytes_read = aprfile->read((void*)&entry, (S32)sizeof(Entry));
		closeHeaderEntriesFile();
	if(bytes_read != sizeof(Entry))
	{
		clearCorruptedCache();
		idx = -1;
	}
}
void LLTextureCache::updateEntryTimeStamp(S32 idx, Entry& entry)
{
	static const U32 MAX_ENTRIES_WITHOUT_TIME_STAMP = (U32)(LLTextureCache::sCacheMaxEntries * 0.75f);
	if(mHeaderEntriesInfo.mEntries < MAX_ENTRIES_WITHOUT_TIME_STAMP)
	{
		return;
	}
	if (idx >= 0)
	{
		if (!mReadOnly)
		{
			entry.mTime = time(NULL);
			mUpdatedEntryMap[idx] = entry;
		}
	}
}
bool LLTextureCache::updateEntry(S32& idx, Entry& entry, S32 new_image_size, S32 new_data_size)
{
	S32 new_body_size = llmax(0, new_data_size - TEXTURE_CACHE_ENTRY_SIZE);
	if(new_image_size == entry.mImageSize && new_body_size == entry.mBodySize)
			{
		return true;
			}
	else
	{
		bool purge = false;
		lockHeaders();
		bool update_header = false;
		if(entry.mImageSize < 0)
			{
			mHeaderIDMap[entry.mID] = idx;
			mTexturesSizeMap[entry.mID] = new_body_size;
			mTexturesSizeTotal += new_body_size;
			update_header = true;
			}
		else if (entry.mBodySize != new_body_size)
		{
			mTexturesSizeMap[entry.mID] = new_body_size;
			mTexturesSizeTotal -= entry.mBodySize;
			mTexturesSizeTotal += new_body_size;
		}
		entry.mTime = time(NULL);
		entry.mImageSize = new_image_size;
		entry.mBodySize = new_body_size;
		writeEntryToHeaderImmediately(idx, entry, update_header);
		if (mTexturesSizeTotal > sCacheMaxTexturesSize)
		{
			purge = true;
		}
		unlockHeaders();
		if (purge)
		{
			mDoPurge = TRUE;
		}
	}
	return false;
}
U32 LLTextureCache::openAndReadEntries(std::vector<Entry>& entries)
{
	U32 num_entries = mHeaderEntriesInfo.mEntries;
	mHeaderIDMap.clear();
	mTexturesSizeMap.clear();
	mFreeList.clear();
	mTexturesSizeTotal = 0;
	LLAPRFile* aprfile = NULL;
	if(mUpdatedEntryMap.empty())
	{
		aprfile = openHeaderEntriesFile(true, (S32)sizeof(EntriesInfo));
	}
	else
	{
		aprfile = openHeaderEntriesFile(false, 0);
		updatedHeaderEntriesFile();
		if(!aprfile)
		{
			return 0;
		}
		aprfile->seek(APR_SET, (S32)sizeof(EntriesInfo));
	}
	for (U32 idx=0; idx<num_entries; idx++)
	{
		Entry entry;
		S32 bytes_read = aprfile->read((void*)(&entry), (S32)sizeof(Entry));
		if (bytes_read < sizeof(Entry))
		{
			LL_WARNS() << "Corrupted header entries, failed at " << idx << " / " << num_entries << LL_ENDL;
			closeHeaderEntriesFile();
			purgeAllTextures(false);
			return 0;
		}
		entries.push_back(entry);
		if(entry.mImageSize > entry.mBodySize)
		{
			mHeaderIDMap[entry.mID] = idx;
				mTexturesSizeMap[entry.mID] = entry.mBodySize;
				mTexturesSizeTotal += entry.mBodySize;
			}
		else
		{
			mFreeList.insert(idx);
		}
	}
	closeHeaderEntriesFile();
	return num_entries;
}
void LLTextureCache::writeEntriesAndClose(const std::vector<Entry>& entries)
{
	S32 num_entries = entries.size();
	llassert_always(num_entries == mHeaderEntriesInfo.mEntries);
	if (!mReadOnly)
	{
		LLAPRFile* aprfile = openHeaderEntriesFile(false, (S32)sizeof(EntriesInfo));
		U64 write_size = U64(sizeof(Entry)) * num_entries;
		U64 bytes_written = aprfile->write((void*)(entries.data()), write_size);
		if (bytes_written != write_size)
		{
			clearCorruptedCache();
			return;
		}
		closeHeaderEntriesFile();
	}
}
void LLTextureCache::writeUpdatedEntries()
{
	lockHeaders();
	if (!mReadOnly && !mUpdatedEntryMap.empty())
	{
		openHeaderEntriesFile(false, 0);
		updatedHeaderEntriesFile();
		closeHeaderEntriesFile();
	}
	unlockHeaders();
}
void LLTextureCache::updatedHeaderEntriesFile()
{
	if (!mReadOnly && !mUpdatedEntryMap.empty() && mHeaderAPRFile)
	{
		mHeaderAPRFile->seek(APR_SET, 0);
		S32 bytes_written = mHeaderAPRFile->write((U8*)&mHeaderEntriesInfo, sizeof(EntriesInfo));
		if(bytes_written != sizeof(EntriesInfo))
		{
			clearCorruptedCache();
			return;
		}
		S32 entry_size = (S32)sizeof(Entry);
		S32 prev_idx = -1;
		S32 delta_idx;
		for (idx_entry_map_t::iterator iter = mUpdatedEntryMap.begin(); iter != mUpdatedEntryMap.end(); ++iter)
		{
			delta_idx = iter->first - prev_idx - 1;
			prev_idx = iter->first;
			if(delta_idx)
			{
				mHeaderAPRFile->seek(APR_CUR, delta_idx * entry_size);
			}
			bytes_written = mHeaderAPRFile->write((void*)(&iter->second), entry_size);
			if(bytes_written != entry_size)
			{
				clearCorruptedCache();
				return;
			}
		}
		mUpdatedEntryMap.clear();
	}
}
void LLTextureCache::readHeaderCache()
{
	mHeaderMutex.lock();
	mLRU.clear();
	readEntriesHeader();
	if (mHeaderEntriesInfo.mVersion != sHeaderCacheVersion)
	{
		if (!mReadOnly)
		{
			purgeAllTextures(false);
		}
	}
	else
	{
		std::vector<Entry> entries;
		U32 num_entries = openAndReadEntries(entries);
		if (num_entries)
		{
			U32 empty_entries = 0;
			typedef std::pair<U32, S32> lru_data_t;
			std::set<lru_data_t> lru;
			std::set<U32> purge_list;
			for (U32 i=0; i<num_entries; i++)
			{
				Entry& entry = entries[i];
				if (entry.mImageSize <= 0)
				{
					++empty_entries;
				}
				else
				{
					lru.insert(std::make_pair(entry.mTime, i));
					if (entry.mBodySize > 0)
					{
						if (entry.mBodySize > entry.mImageSize)
						{
							LL_WARNS() << "Bad entry: " << i << ": " << entry.mID << ": BodySize: " << entry.mBodySize << LL_ENDL;
							purge_list.insert(i);
						}
					}
				}
			}
			if (num_entries - empty_entries > sCacheMaxEntries)
			{
				U32 entries_to_purge = (num_entries - empty_entries) - sCacheMaxEntries;
				LL_INFOS() << "Texture Cache Entries: " << num_entries << " Max: " << sCacheMaxEntries << " Empty: " << empty_entries << " Purging: " << entries_to_purge << LL_ENDL;
				std::set<lru_data_t>::iterator iter = lru.begin();
				while (purge_list.size() < entries_to_purge)
				{
					purge_list.insert(iter->second);
					++iter;
				}
			}
			else
			{
				S32 lru_entries = (S32)((F32)sCacheMaxEntries * TEXTURE_CACHE_LRU_SIZE);
				for (std::set<lru_data_t>::iterator iter = lru.begin(); iter != lru.end(); ++iter)
				{
					mLRU.insert(entries[iter->second].mID);
					if (--lru_entries <= 0)
						break;
				}
			}
			if (purge_list.size() > 0)
			{
				for (std::set<U32>::iterator iter = purge_list.begin(); iter != purge_list.end(); ++iter)
				{
					std::string tex_filename = getTextureFileName(entries[*iter].mID);
					removeEntry((S32)*iter, entries[*iter], tex_filename);
				}
				std::vector<Entry> new_entries;
				for (U32 i=0; i<num_entries; i++)
				{
					const Entry& entry = entries[i];
					if (entry.mImageSize > 0)
					{
						new_entries.push_back(entry);
					}
				}
				llassert_always(new_entries.size() <= sCacheMaxEntries);
				mHeaderEntriesInfo.mEntries = new_entries.size();
				writeEntriesHeader();
				writeEntriesAndClose(new_entries);
				mHeaderMutex.unlock();
				readHeaderCache();
				mHeaderMutex.lock();
			}
			else
			{
			}
		}
	}
	mHeaderMutex.unlock();
}
void LLTextureCache::clearCorruptedCache()
{
	LL_WARNS() << "the texture cache is corrupted, need to be cleared." << LL_ENDL;
	closeHeaderEntriesFile();
	purgeAllTextures(false);
	if (!mReadOnly)
	{
		LLFile::mkdir(mTexturesDirName);
		const char* subdirs = "0123456789abcdef";
		for (S32 i=0; i<16; i++)
		{
			std::string dirname = mTexturesDirName + gDirUtilp->getDirDelimiter() + subdirs[i];
			LLFile::mkdir(dirname);
		}
	}
	return;
}
void LLTextureCache::purgeAllTextures(bool purge_directories)
{
	if (!mReadOnly)
	{
		const char* subdirs = "0123456789abcdef";
		std::string delem = gDirUtilp->getDirDelimiter();
		std::string mask = "*";
		for (S32 i=0; i<16; i++)
		{
			std::string dirname = mTexturesDirName + delem + subdirs[i];
			LL_INFOS() << "Deleting files in directory: " << dirname << LL_ENDL;
			gDirUtilp->deleteFilesInDir(dirname, mask);
			if (purge_directories)
			{
				LLFile::rmdir(dirname);
			}
		}
		if (purge_directories)
		{
			gDirUtilp->deleteFilesInDir(mTexturesDirName, mask);
			LLFile::rmdir(mTexturesDirName);
		}
	}
	mHeaderIDMap.clear();
	mTexturesSizeMap.clear();
	mTexturesSizeTotal = 0;
	mFreeList.clear();
	mTexturesSizeTotal = 0;
	mUpdatedEntryMap.clear();
	mHeaderEntriesInfo.mVersion = sHeaderCacheVersion;
	mHeaderEntriesInfo.mEntries = 0;
	writeEntriesHeader();
	LL_INFOS() << "The entire texture cache is cleared." << LL_ENDL;
}
void LLTextureCache::performDelayedPurge()
{
	LLMutexLock lock(&mHeaderMutex);
	while(!sgDelayedPurgeQueue.empty())
	{
		removeFromCache(sgDelayedPurgeQueue.front());
		sgDelayedPurgeQueue.pop();
		if (mTexturesSizeTotal < sCacheMaxTexturesSize)
		{
			break;
		}
	}
}
void LLTextureCache::purgeTextures(bool validate)
{
	if (mReadOnly)
	{
		return;
	}
	if (!mThreaded)
	{
		LLAppViewer::instance()->pauseMainloopTimeout();
	}
	LLMutexLock lock(&mHeaderMutex);
	LL_INFOS() << "TEXTURE CACHE: Purging." << LL_ENDL;
	std::queue<LLUUID> empty;
	std::swap(sgDelayedPurgeQueue, empty);
	std::vector<Entry> entries;
	U32 num_entries = openAndReadEntries(entries);
	if (!num_entries)
	{
		return;
	}
	typedef std::vector<std::pair<U32,S32> > time_idx_set_t;
	time_idx_set_t time_idx_set;
	for (size_map_t::iterator iter1 = mTexturesSizeMap.begin();
		 iter1 != mTexturesSizeMap.end(); ++iter1)
	{
		if (iter1->second > 0)
		{
			id_map_t::iterator iter2 = mHeaderIDMap.find(iter1->first);
			if (iter2 != mHeaderIDMap.end())
			{
				S32 idx = iter2->second;
				time_idx_set.push_back(std::make_pair(entries[idx].mTime, idx));
			}
			else
			{
				LL_ERRS() << "mTexturesSizeMap / mHeaderIDMap corrupted." << LL_ENDL ;
			}
		}
	}
	std::sort(time_idx_set.begin(), time_idx_set.end());
	U32 validate_idx = 0;
	if (validate)
	{
		validate_idx = gSavedSettings.getU32("CacheValidateCounter");
		U32 next_idx = (validate_idx + 1) % 256;
		gSavedSettings.setU32("CacheValidateCounter", next_idx);
		LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Validating: " << validate_idx << LL_ENDL;
	}
	S64 cache_size = mTexturesSizeTotal;
	S64 purged_cache_size = (sCacheMaxTexturesSize * (S64)((1.f-TEXTURE_CACHE_PURGE_AMOUNT)*100)) / 100;
	S32 purge_count = 0;
	for (time_idx_set_t::iterator iter = time_idx_set.begin();
		 iter != time_idx_set.end(); ++iter)
	{
		S32 idx = iter->second;
		bool purge_entry = false;
		std::string filename = getTextureFileName(entries[idx].mID);
		if (cache_size >= purged_cache_size)
		{
			purge_entry = true;
		}
		else if (validate)
		{
			U32 uuididx = entries[idx].mID.mData[0];
			if (uuididx == validate_idx)
			{
 				LL_DEBUGS("TextureCache") << "Validating: " << filename << "Size: " << entries[idx].mBodySize << LL_ENDL;
				S32 bodysize = LLAPRFile::size(filename);
				if (bodysize != entries[idx].mBodySize)
				{
					LL_WARNS("TextureCache") << "TEXTURE CACHE BODY HAS BAD SIZE: " << bodysize << " != " << entries[idx].mBodySize
							<< filename << LL_ENDL;
					purge_entry = true;
				}
			}
		}
		else
		{
			break;
		}
		if (purge_entry)
		{
			purge_count++;
	 		LL_DEBUGS("TextureCache") << "PURGING: " << filename << LL_ENDL;
			cache_size -= entries[idx].mBodySize;
			if(validate)
			{
				removeEntry(idx, entries[idx], filename);
			}
			else
			{
				sgDelayedPurgeQueue.push(entries[idx].mID);
			}
		}
	}
	LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Writing Entries: " << num_entries << LL_ENDL;
	writeEntriesAndClose(entries);
	LLAppViewer::instance()->resumeMainloopTimeout();
	LL_INFOS("TextureCache") << "TEXTURE CACHE:"
			<< " PURGED: " << purge_count
			<< " ENTRIES: " << num_entries
			<< " CACHE SIZE: " << mTexturesSizeTotal / (1024 * 1024) << " MB"
			<< LL_ENDL;
}
LLTextureCacheWorker* LLTextureCache::getReader(handle_t handle)
{
	LLTextureCacheWorker* res = NULL;
	handle_map_t::iterator iter = mReaders.find(handle);
	if (iter != mReaders.end())
	{
		res = iter->second;
	}
	return res;
}
LLTextureCacheWorker* LLTextureCache::getWriter(handle_t handle)
{
	LLTextureCacheWorker* res = NULL;
	handle_map_t::iterator iter = mWriters.find(handle);
	if (iter != mWriters.end())
	{
		res = iter->second;
	}
	return res;
}
S32 LLTextureCache::getHeaderCacheEntry(const LLUUID& id, Entry& entry)
{
	LLMutexLock lock(&mHeaderMutex);
	S32 idx = openAndReadEntry(id, entry, false);
	if (idx >= 0)
	{
		updateEntryTimeStamp(idx, entry);
	}
	return idx;
}
S32 LLTextureCache::setHeaderCacheEntry(const LLUUID& id, Entry& entry, S32 imagesize, S32 datasize)
{
	mHeaderMutex.lock();
	S32 idx = openAndReadEntry(id, entry, true);
	mHeaderMutex.unlock();
	if (idx >= 0)
	{
		updateEntry(idx, entry, imagesize, datasize);
	}
	if(idx < 0)
	{
		readHeaderCache();
		mHeaderMutex.lock();
		llassert_always(!mLRU.empty() || mHeaderEntriesInfo.mEntries < sCacheMaxEntries);
		mHeaderMutex.unlock();
		idx = setHeaderCacheEntry(id, entry, imagesize, datasize);
	}
	return idx;
}
LLTextureCache::handle_t LLTextureCache::readFromCache(const std::string& filename, const LLUUID& id, U32 priority,
													   S32 offset, S32 size, ReadResponder* responder)
{
	LLMutexLock lock(&mWorkersMutex);
	LLTextureCacheWorker* worker = new LLTextureCacheLocalFileWorker(this, priority, filename, id,
																	 NULL, size, offset, 0,
																	 responder);
	handle_t handle = worker->read();
	mReaders[handle] = worker;
	return handle;
}
LLTextureCache::handle_t LLTextureCache::readFromCache(const LLUUID& id, U32 priority,
													   S32 offset, S32 size, ReadResponder* responder)
{
	LLMutexLock lock(&mWorkersMutex);
	LLTextureCacheWorker* worker = new LLTextureCacheRemoteWorker(this, priority, id,
																  NULL, size, offset,
																  0, responder);
	handle_t handle = worker->read();
	mReaders[handle] = worker;
	return handle;
}
bool LLTextureCache::readComplete(handle_t handle, bool abort)
{
	lockWorkers();
	handle_map_t::iterator iter = mReaders.find(handle);
	LLTextureCacheWorker* worker = NULL;
	bool complete = false;
	if (iter != mReaders.end())
	{
		worker = iter->second;
		complete = worker->complete();
		if(!complete && abort)
		{
			abortRequest(handle, true);
		}
	}
	if (worker && (complete || abort))
	{
		mReaders.erase(iter);
      	unlockWorkers();
		worker->scheduleDelete();
	}
	else
	{
		unlockWorkers();
	}
	return (complete || abort);
}
LLTextureCache::handle_t LLTextureCache::writeToCache(const LLUUID& id, U32 priority,
													  U8* data, S32 datasize, S32 imagesize,
													  WriteResponder* responder, LLImageRaw* rawimage, S32 discardlevel)
{
	if (mReadOnly)
	{
		delete responder;
		return LLWorkerThread::nullHandle();
	}
	if (sgDelayedPurgeQueue.empty() && mDoPurge)
	{
		purgeTextures(false);
		mDoPurge = FALSE;
	}
	performDelayedPurge();
	LLMutexLock lock(&mWorkersMutex);
	LLTextureCacheWorker* worker = new LLTextureCacheRemoteWorker(this, priority, id,
																  data, datasize, 0,
																  imagesize, responder,
																  rawimage, discardlevel);
	handle_t handle = worker->write();
	mWriters[handle] = worker;
	return handle;
}
bool LLTextureCache::writeComplete(handle_t handle, bool abort)
{
	lockWorkers();
	handle_map_t::iterator iter = mWriters.find(handle);
	llassert(iter != mWriters.end());
	if (iter != mWriters.end())
	{
	LLTextureCacheWorker* worker = iter->second;
	if (worker->complete() || abort)
	{
		mWriters.erase(handle);
		unlockWorkers();
		worker->scheduleDelete();
		return true;
	}
	}
		unlockWorkers();
		return false;
}
void LLTextureCache::prioritizeWrite(handle_t handle)
{
	LLMutexLock lock(&mListMutex);
	mPrioritizeWriteList.push_back(handle);
}
void LLTextureCache::addCompleted(Responder* responder, bool success)
{
	LLMutexLock lock(&mListMutex);
	mCompletedList.push_back(std::make_pair(responder,success));
}
void LLTextureCache::removeCachedTexture(const LLUUID& id)
{
	if (mTexturesSizeMap.find(id) != mTexturesSizeMap.end())
	{
		mTexturesSizeTotal -= mTexturesSizeMap[id];
		mTexturesSizeMap.erase(id);
	}
	mHeaderIDMap.erase(id);
	LLAPRFile::remove(getTextureFileName(id));
}
void LLTextureCache::removeEntry(S32 idx, Entry& entry, std::string& filename)
{
 	bool file_maybe_exists = true;
	if(idx >= 0)
	{
		if (entry.mBodySize == 0)
		{
		  if (LLAPRFile::isExist(filename))
		  {
			  LL_WARNS("TextureCache") << "Entry has body size of zero but file " << filename << " exists. Deleting this file, too." << LL_ENDL;
		  }
		  else
		  {
			  file_maybe_exists = false;
		  }
		}
		mTexturesSizeTotal -= entry.mBodySize;
		entry.mImageSize = -1;
		entry.mBodySize = 0;
		mHeaderIDMap.erase(entry.mID);
		mTexturesSizeMap.erase(entry.mID);
		mFreeList.insert(idx);
	}
	if (file_maybe_exists)
	{
		LLAPRFile::remove(filename);
	}
}
bool LLTextureCache::removeFromCache(const LLUUID& id)
{
	bool ret = false;
	if (!mReadOnly)
	{
		lockHeaders();
		Entry entry;
		S32 idx = openAndReadEntry(id, entry, false);
		std::string tex_filename = getTextureFileName(id);
		removeEntry(idx, entry, tex_filename);
		if (idx >= 0)
		{
			writeEntryToHeaderImmediately(idx, entry);
			ret = true;
		}
		unlockHeaders();
	}
	return ret;
}
LLTextureCache::ReadResponder::ReadResponder()
	: mImageSize(0),
	  mImageLocal(FALSE)
{
}
void LLTextureCache::ReadResponder::setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, BOOL imagelocal)
{
	if (mFormattedImage.notNull())
	{
		llassert_always(mFormattedImage->getCodec() == imageformat);
		mFormattedImage->appendData(data, datasize);
	}
	else
	{
		mFormattedImage = LLImageFormatted::createFromType(imageformat);
		mFormattedImage->setData(data,datasize);
	}
	mImageSize = imagesize;
	mImageLocal = imagelocal;
}
LLPointer<LLImageRaw> LLTextureCache::readFromFastCache(const LLUUID& id, S32& discardlevel)
{
	S32 idx = -1;
	{
		LLMutexLock lock(&mHeaderMutex);
		id_map_t::const_iterator iter = mHeaderIDMap.find(id);
		if (iter == mHeaderIDMap.end())
		{
			return NULL;
		}
		idx = iter->second;
	}
	if (idx < 0)
	{
		return NULL;
	}
	S32 offset = idx * TEXTURE_FAST_CACHE_ENTRY_SIZE;
	S32 head[4] = { 0, 0, 0, 0 };
	{
		LLMutexLock lock(&mFastCacheMutex);
		S32 bytes = LLAPRFile::readEx(mFastCacheFileName, head, offset, TEXTURE_FAST_CACHE_ENTRY_OVERHEAD);
		if (bytes != TEXTURE_FAST_CACHE_ENTRY_OVERHEAD)
		{
			return NULL;
		}
		S32 image_size = head[0] * head[1] * head[2];
		if (image_size <= 0 || image_size > TEXTURE_FAST_CACHE_DATA_SIZE || head[3] < 0)
		{
			return NULL;
		}
		discardlevel = head[3];
		U8* data = (U8*)ll_aligned_malloc_16(image_size);
		if (!data)
		{
			return NULL;
		}
		bytes = LLAPRFile::readEx(mFastCacheFileName, data, offset + TEXTURE_FAST_CACHE_ENTRY_OVERHEAD, image_size);
		if (bytes != image_size)
		{
			ll_aligned_free_16(data);
			return NULL;
		}
		LLPointer<LLImageRaw> raw = new LLImageRaw(data, (U16)head[0], (U16)head[1], (S8)head[2], true);
		if (discardlevel > MAX_DISCARD_LEVEL)
		{
			S32 w = head[0] << (discardlevel - MAX_DISCARD_LEVEL);
			S32 h = head[1] << (discardlevel - MAX_DISCARD_LEVEL);
			discardlevel = MAX_DISCARD_LEVEL;
			raw->scale(w, h, TRUE);
		}
		return raw;
	}
}
bool LLTextureCache::writeToFastCache(LLUUID image_id, S32 id, LLPointer<LLImageRaw> raw, S32 discardlevel)
{
	if (raw.isNull() || !raw->getData() || id < 0)
	{
		return false;
	}
	S32 w = raw->getWidth();
	S32 h = raw->getHeight();
	S32 c = raw->getComponents();
	S32 i = 0;
	while (((w >> i) * (h >> i) * c) > TEXTURE_FAST_CACHE_DATA_SIZE)
	{
		++i;
	}
	if (i)
	{
		w >>= i;
		h >>= i;
		if (w * h * c > 0)
		{
			raw = raw->duplicate();
			if (raw.isNull() || !raw->getData())
			{
				return false;
			}
			raw->scale(w, h);
			discardlevel += i;
		}
	}
	if (!mFastCachePadBuffer)
	{
		mFastCachePadBuffer = (U8*)ll_aligned_malloc_16(TEXTURE_FAST_CACHE_ENTRY_SIZE);
		if (!mFastCachePadBuffer)
		{
			return false;
		}
	}
	memset(mFastCachePadBuffer, 0, TEXTURE_FAST_CACHE_ENTRY_SIZE);
	memcpy(mFastCachePadBuffer, &w, sizeof(S32));
	memcpy(mFastCachePadBuffer + sizeof(S32), &h, sizeof(S32));
	memcpy(mFastCachePadBuffer + sizeof(S32) * 2, &c, sizeof(S32));
	memcpy(mFastCachePadBuffer + sizeof(S32) * 3, &discardlevel, sizeof(S32));
	S32 copy_size = w * h * c;
	if (copy_size > 0)
	{
		copy_size = llmin(copy_size, TEXTURE_FAST_CACHE_DATA_SIZE);
		memcpy(mFastCachePadBuffer + TEXTURE_FAST_CACHE_ENTRY_OVERHEAD, raw->getData(), copy_size);
	}
	S32 offset = id * TEXTURE_FAST_CACHE_ENTRY_SIZE;
	LLMutexLock lock(&mFastCacheMutex);
	S32 written = LLAPRFile::writeEx(mFastCacheFileName, mFastCachePadBuffer, offset, TEXTURE_FAST_CACHE_ENTRY_SIZE);
	return written == TEXTURE_FAST_CACHE_ENTRY_SIZE;
}
