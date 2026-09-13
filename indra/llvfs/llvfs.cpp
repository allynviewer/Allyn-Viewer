/** 
 * @file llvfs.cpp
 * @brief Implementation of virtual file system
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
#include "llvfs.h"
#include <sys/stat.h>
#include <set>
#include <map>
#if LL_WINDOWS
#include <share.h>
#elif LL_SOLARIS
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <sys/file.h>
#endif
#include "llstl.h"
#include "lltimer.h"
const S32 FILE_BLOCK_MASK = 0x000003FF;
const S32 VFS_CLEANUP_SIZE = 5242880;
const S32 BLOCK_LENGTH_INVALID = -1;
LLVFS *gVFS = NULL;
LLVFSBlock::LLVFSBlock()
{
	mLocation = 0;
	mLength = 0;
}
LLVFSBlock::LLVFSBlock(U32 loc, S32 size)
{
	mLocation = loc;
	mLength = size;
}
bool LLVFSBlock::locationSortPredicate(
	const LLVFSBlock* lhs,
	const LLVFSBlock* rhs)
{
	return lhs->mLocation < rhs->mLocation;
}
LLVFSFileSpecifier::LLVFSFileSpecifier()
:	mFileID(),
	mFileType( LLAssetType::AT_NONE )
{
}
LLVFSFileSpecifier::LLVFSFileSpecifier(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	mFileID = file_id;
	mFileType = file_type;
}
bool LLVFSFileSpecifier::operator<(const LLVFSFileSpecifier &rhs) const
{
	return (mFileID == rhs.mFileID)
		? mFileType < rhs.mFileType
		: mFileID < rhs.mFileID;
}
bool LLVFSFileSpecifier::operator==(const LLVFSFileSpecifier &rhs) const
{
	return (mFileID == rhs.mFileID &&
			mFileType == rhs.mFileType);
}
LLVFSFileBlock::LLVFSFileBlock() : LLVFSBlock(),  LLVFSFileSpecifier()
{
	init();
}
LLVFSFileBlock::LLVFSFileBlock(const LLUUID &file_id, LLAssetType::EType file_type, U32 loc, S32 size) :
		LLVFSBlock(loc, size), LLVFSFileSpecifier( file_id, file_type )
{
	init();
}
void LLVFSFileBlock::init()
{
	mSize = 0;
	mIndexLocation = -1;
	mAccessTime = (U32)time(NULL);
	for (S32 i = 0; i < (S32)VFSLOCK_COUNT; i++)
	{
		mLocks[(EVFSLock)i] = 0;
	}
}
#ifdef LL_LITTLE_ENDIAN
void LLVFSFileBlock::swizzleCopy(void *dst, void *src, int size) { memcpy(dst, src, size); }
#else
U32 LLVFSFileBlock::swizzle32(U32 x)
{
	return(((x >> 24) & 0x000000FF) | ((x >> 8)  & 0x0000FF00) | ((x << 8)  & 0x00FF0000) |((x << 24) & 0xFF000000));
}
U16 LLVFSFileBlock::swizzle16(U16 x)
{
	return(	((x >> 8)  & 0x000000FF) | ((x << 8)  & 0x0000FF00) );
}
void LLVFSFileBlock::swizzleCopy(void *dst, void *src, int size)
{
	if(size == 4)
	{
		((U32*)dst)[0] = swizzle32(((U32*)src)[0]);
	}
	else if(size == 2)
	{
		((U16*)dst)[0] = swizzle16(((U16*)src)[0]);
	}
	else
	{
		memcpy(dst, src, size);
	}
}
#endif
void LLVFSFileBlock::serialize(U8 *buffer)
{
	swizzleCopy(buffer, &mLocation, 4);
	buffer += 4;
	swizzleCopy(buffer, &mLength, 4);
	buffer +=4;
	swizzleCopy(buffer, &mAccessTime, 4);
	buffer +=4;
	memcpy(buffer, &mFileID.mData, 16);
	buffer += 16;
	S16 temp_type = mFileType;
	swizzleCopy(buffer, &temp_type, 2);
	buffer += 2;
	swizzleCopy(buffer, &mSize, 4);
}
void LLVFSFileBlock::deserialize(U8 *buffer, const S32 index_loc)
{
	mIndexLocation = index_loc;
	swizzleCopy(&mLocation, buffer, 4);
	buffer += 4;
	swizzleCopy(&mLength, buffer, 4);
	buffer += 4;
	swizzleCopy(&mAccessTime, buffer, 4);
	buffer += 4;
	memcpy(&mFileID.mData, buffer, 16);
	buffer += 16;
	S16 temp_type;
	swizzleCopy(&temp_type, buffer, 2);
	mFileType = (LLAssetType::EType)temp_type;
	buffer += 2;
	swizzleCopy(&mSize, buffer, 4);
}
BOOL LLVFSFileBlock::insertLRU(LLVFSFileBlock* const& first,
					  LLVFSFileBlock* const& second)
{
	return (first->mAccessTime == second->mAccessTime)
		? *first < *second
		: first->mAccessTime < second->mAccessTime;
}
struct LLVFSFileBlock_less
{
	bool operator()(LLVFSFileBlock* const& lhs, LLVFSFileBlock* const& rhs) const
	{
		return (LLVFSFileBlock::insertLRU(lhs, rhs)) ? true : false;
	}
};
const S32 LLVFSFileBlock::SERIAL_SIZE = 34;
LLVFS::LLVFS(const std::string& index_filename, const std::string& data_filename, const BOOL read_only, const U32 presize, const BOOL remove_after_crash)
:	mRemoveAfterCrash(remove_after_crash),
	mDataFP(NULL),
	mIndexFP(NULL)
{
	mDataMutex = new LLMutex;
	S32 i;
	for (i = 0; i < VFSLOCK_COUNT; i++)
	{
		mLockCounts[i] = 0;
	}
	mValid = VFSVALID_OK;
	mReadOnly = read_only;
	mIndexFilename = index_filename;
	mDataFilename = data_filename;
	const char *file_mode = mReadOnly ? "rb" : "r+b";
	LL_INFOS("VFS") << "Attempting to open VFS index file " << mIndexFilename << LL_ENDL;
	LL_INFOS("VFS") << "Attempting to open VFS data file " << mDataFilename << LL_ENDL;
	mDataFP = openAndLock(mDataFilename, file_mode, mReadOnly);
	if (!mDataFP)
	{
		if (mReadOnly)
		{
			LL_WARNS("VFS") << "Can't find " << mDataFilename << " to open read-only VFS" << LL_ENDL;
			mValid = VFSVALID_BAD_CANNOT_OPEN_READONLY;
			return;
		}
		mDataFP = openAndLock(mDataFilename, "w+b", FALSE);
		if (mDataFP)
		{
			LLFile::remove(mIndexFilename);
		}
		else
		{
			LL_WARNS("VFS") << "Couldn't open vfs data file "
				<< mDataFilename << LL_ENDL;
			mValid = VFSVALID_BAD_CANNOT_CREATE;
			return;
		}
		if (presize)
		{
			presizeDataFile(presize);
		}
	}
	if (!mReadOnly && mRemoveAfterCrash)
	{
		llstat marker_info;
		std::string marker = mDataFilename + ".open";
		if (!LLFile::stat(marker, &marker_info))
		{
			unlockAndClose(mDataFP);
			mDataFP = NULL;
			LL_WARNS("VFS") << "VFS: File left open on last run, removing old VFS file " << mDataFilename << LL_ENDL;
			LLFile::remove(mIndexFilename);
			LLFile::remove(mDataFilename);
			LLFile::remove(marker);
			mDataFP = openAndLock(mDataFilename, "w+b", FALSE);
			if (!mDataFP)
			{
				LL_WARNS("VFS") << "Can't open VFS data file in crash recovery" << LL_ENDL;
				mValid = VFSVALID_BAD_CANNOT_CREATE;
				return;
			}
			if (presize)
			{
				presizeDataFile(presize);
			}
		}
	}
	fseek(mDataFP, 0, SEEK_END);
	U32 data_size = ftell(mDataFP);
	llstat fbuf;
	if (! LLFile::stat(mIndexFilename, &fbuf) &&
		fbuf.st_size >= LLVFSFileBlock::SERIAL_SIZE &&
		(mIndexFP = openAndLock(mIndexFilename, file_mode, mReadOnly))
		)
	{
		std::vector<U8> buffer(fbuf.st_size);
    		size_t buf_offset = 0;
		size_t nread = fread(&buffer[0], 1, fbuf.st_size, mIndexFP);
		std::vector<LLVFSFileBlock*> files_by_loc;
		while (buf_offset < nread)
		{
			LLVFSFileBlock *block = new LLVFSFileBlock();
			block->deserialize(&buffer[buf_offset], (S32)buf_offset);
			if (block->mLength > 0 &&
				(U32)block->mLength <= data_size &&
				block->mLocation < data_size &&
				block->mSize > 0 &&
				block->mSize <= block->mLength &&
				block->mFileType >= LLAssetType::AT_NONE &&
				block->mFileType < LLAssetType::AT_COUNT)
			{
				mFileBlocks.insert(fileblock_map::value_type(*block, block));
				files_by_loc.push_back(block);
			}
			else
			if (block->mLength && block->mSize > 0)
			{
				LL_WARNS("VFS") << "VFS corruption: " << block->mFileID << " (" << block->mFileType << ") at index " << block->mIndexLocation << " DS: " << data_size << LL_ENDL;
				LL_WARNS("VFS") << "Length: " << block->mLength << "\tLocation: " << block->mLocation << "\tSize: " << block->mSize << LL_ENDL;
				LL_WARNS("VFS") << "File has bad data - VFS removed" << LL_ENDL;
				delete block;
				unlockAndClose( mIndexFP );
				mIndexFP = NULL;
				LLFile::remove( mIndexFilename );
				unlockAndClose( mDataFP );
				mDataFP = NULL;
				LLFile::remove( mDataFilename );
				LL_WARNS("VFS") << "Deleted corrupt VFS files "
					<< mDataFilename
					<< " and "
					<< mIndexFilename
					<< LL_ENDL;
				mValid = VFSVALID_BAD_CORRUPT;
				return;
			}
			else
			{
				mIndexHoles.push_back(buf_offset);
				delete block;
			}
			buf_offset += LLVFSFileBlock::SERIAL_SIZE;
		}
		std::sort(
			files_by_loc.begin(),
			files_by_loc.end(),
			LLVFSFileBlock::locationSortPredicate);
		if (!files_by_loc.empty())
		{
			std::vector<LLVFSFileBlock*>::iterator cur = files_by_loc.begin();
			std::vector<LLVFSFileBlock*>::iterator end = files_by_loc.end();
			LLVFSFileBlock* last_file_block = *cur;
			if (last_file_block->mLocation > 0)
			{
				addFreeBlock(new LLVFSBlock(0, last_file_block->mLocation));
			}
			++cur;
			while( cur != end )
			{
				LLVFSFileBlock* cur_file_block = *cur;
				if (cur_file_block->mLocation == last_file_block->mLocation
					&& cur_file_block->mLength == last_file_block->mLength)
				{
					LL_WARNS("VFS") << "VFS: removing duplicate entry"
						<< " at " << cur_file_block->mLocation
						<< " length " << cur_file_block->mLength
						<< " size " << cur_file_block->mSize
						<< " ID " << cur_file_block->mFileID
						<< " type " << cur_file_block->mFileType
						<< LL_ENDL;
					mFileBlocks.erase(*cur_file_block);
					if (cur_file_block->mLength > 0)
					{
						addFreeBlock(
							new LLVFSBlock(
								cur_file_block->mLocation,
								cur_file_block->mLength));
					}
					lockData();
					sync(cur_file_block, TRUE);
					sync(last_file_block, TRUE);
					unlockData();
					last_file_block = cur_file_block;
					++cur;
					continue;
				}
				S32 loc = last_file_block->mLocation+last_file_block->mLength;
				S32 length = cur_file_block->mLocation - loc;
				if (length < 0 || loc < 0 || (U32)loc > data_size)
				{
					unlockAndClose( mIndexFP );
					mIndexFP = NULL;
					LLFile::remove( mIndexFilename );
					unlockAndClose( mDataFP );
					mDataFP = NULL;
					LLFile::remove( mDataFilename );
					LL_WARNS("VFS") << "VFS: overlapping entries"
						<< " at " << cur_file_block->mLocation
						<< " length " << cur_file_block->mLength
						<< " ID " << cur_file_block->mFileID
						<< " type " << cur_file_block->mFileType
						<< LL_ENDL;
					LL_WARNS("VFS") << "Deleted corrupt VFS files "
						<< mDataFilename
						<< " and "
						<< mIndexFilename
						<< LL_ENDL;
					mValid = VFSVALID_BAD_CORRUPT;
					return;
				}
				if (length > 0)
				{
					addFreeBlock(new LLVFSBlock(loc, length));
				}
				last_file_block = cur_file_block;
				++cur;
			}
			U32 loc = last_file_block->mLocation + last_file_block->mLength;
			if (loc < data_size)
			{
				addFreeBlock(new LLVFSBlock(loc, data_size - loc));
			}
		}
		else
		{
			addFreeBlock(new LLVFSBlock(0, data_size));
		}
	}
	else
	{
		if (mReadOnly)
		{
			LL_WARNS("VFS") << "Can't find " << mIndexFilename << " to open read-only VFS" << LL_ENDL;
			mValid = VFSVALID_BAD_CANNOT_OPEN_READONLY;
			return;
		}
		mIndexFP = openAndLock(mIndexFilename, "w+b", FALSE);
		if (!mIndexFP)
		{
			LL_WARNS("VFS") << "Couldn't open an index file for the VFS, probably a sharing violation!" << LL_ENDL;
			unlockAndClose( mDataFP );
			mDataFP = NULL;
			LLFile::remove( mDataFilename );
			mValid = VFSVALID_BAD_CANNOT_CREATE;
			return;
		}
		LLVFSBlock *first_block = new LLVFSBlock(0, data_size ? data_size : 0x40000000);
		addFreeBlock(first_block);
	}
	if (!mReadOnly && mRemoveAfterCrash)
	{
		std::string marker = mDataFilename + ".open";
		LLFILE* marker_fp = LLFile::fopen(marker, "w");
		if (marker_fp)
		{
			fclose(marker_fp);
			marker_fp = NULL;
		}
	}
	LL_INFOS("VFS") << "Using VFS index file " << mIndexFilename << LL_ENDL;
	LL_INFOS("VFS") << "Using VFS data file " << mDataFilename << LL_ENDL;
	mValid = VFSVALID_OK;
}
LLVFS::~LLVFS()
{
	if (mDataMutex->isLocked())
	{
		LL_ERRS("VFS") << "LLVFS destroyed with mutex locked" << LL_ENDL;
	}
	unlockAndClose(mIndexFP);
	mIndexFP = NULL;
	fileblock_map::const_iterator it;
	for (it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		delete (*it).second;
	}
	mFileBlocks.clear();
	mFreeBlocksByLength.clear();
	for_each(mFreeBlocksByLocation.begin(), mFreeBlocksByLocation.end(), DeletePairedPointer());
	unlockAndClose(mDataFP);
	mDataFP = NULL;
	if (!mReadOnly && mRemoveAfterCrash)
	{
		std::string marker = mDataFilename + ".open";
		LLFile::remove(marker);
	}
	delete mDataMutex;
}
LLVFS * LLVFS::createLLVFS(const std::string& index_filename,
		const std::string& data_filename,
		const BOOL read_only,
		const U32 presize,
		const BOOL remove_after_crash)
{
	LLVFS * new_vfs = new LLVFS(index_filename, data_filename, read_only, presize, remove_after_crash);
	if( !new_vfs->isValid() )
	{
		std::string retry_vfs_index_name;
		std::string retry_vfs_data_name;
		S32 count = 0;
		while (!new_vfs->isValid() &&
				count < 256)
		{
			retry_vfs_index_name = index_filename + llformat(".%u",count);
			retry_vfs_data_name = data_filename + llformat(".%u", count);
			delete new_vfs;
			new_vfs = new LLVFS(retry_vfs_index_name, retry_vfs_data_name, read_only, presize, remove_after_crash);
			count++;
		}
	}
	if( !new_vfs->isValid() )
	{
		delete new_vfs;
		new_vfs = NULL;
	}
	return new_vfs;
}
void LLVFS::presizeDataFile(const U32 size)
{
	if (!mDataFP)
	{
		LL_ERRS() << "LLVFS::presizeDataFile() with no data file open" << LL_ENDL;
		return;
	}
	fseek(mDataFP, size-1, SEEK_SET);
	S32 tmp = 0;
	tmp = (S32)fwrite(&tmp, 1, 1, mDataFP);
	LLFile::remove(mIndexFilename);
	if (tmp)
	{
		LL_INFOS() << "Pre-sized VFS data file to " << ftell(mDataFP) << " bytes" << LL_ENDL;
	}
	else
	{
		LL_WARNS() << "Failed to pre-size VFS data file" << LL_ENDL;
	}
}
BOOL LLVFS::getExists(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	LLVFSFileBlock *block = NULL;
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		block = (*it).second;
		block->mAccessTime = (U32)time(NULL);
	}
	BOOL res = (block && block->mLength > 0) ? TRUE : FALSE;
	unlockData();
	return res;
}
S32	 LLVFS::getSize(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	S32 size = 0;
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		block->mAccessTime = (U32)time(NULL);
		size = block->mSize;
	}
	unlockData();
	return size;
}
S32  LLVFS::getMaxSize(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	S32 size = 0;
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		block->mAccessTime = (U32)time(NULL);
		size = block->mLength;
	}
	unlockData();
	return size;
}
BOOL LLVFS::checkAvailable(S32 max_size)
{
	lockData();
	blocks_length_map_t::iterator iter = mFreeBlocksByLength.lower_bound(max_size);
	const BOOL res(iter == mFreeBlocksByLength.end() ? FALSE : TRUE);
	unlockData();
	return res;
}
BOOL LLVFS::setMaxSize(const LLUUID &file_id, const LLAssetType::EType file_type, S32 max_size)
{
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	if (mReadOnly)
	{
		LL_ERRS() << "Attempt to write to read-only VFS" << LL_ENDL;
	}
	if (max_size <= 0)
	{
		LL_WARNS() << "VFS: Attempt to assign size " << max_size << " to vfile " << file_id << LL_ENDL;
		return FALSE;
	}
	lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	LLVFSFileBlock *block = NULL;
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		block = (*it).second;
	}
	if (file_type != LLAssetType::AT_TEXTURE)
	{
		if (max_size & FILE_BLOCK_MASK)
		{
			max_size += FILE_BLOCK_MASK;
			max_size &= ~FILE_BLOCK_MASK;
		}
    }
	if (block && block->mLength > 0)
	{
		block->mAccessTime = (U32)time(NULL);
		if (max_size == block->mLength)
		{
			unlockData();
			return TRUE;
		}
		else if (max_size < block->mLength)
		{
			LLVFSBlock *free_block = new LLVFSBlock(block->mLocation + max_size, block->mLength - max_size);
			addFreeBlock(free_block);
			block->mLength = max_size;
			if (block->mLength < block->mSize)
			{
				LL_ERRS() << "Truncating virtual file " << file_id << " to " << block->mLength << " bytes" << LL_ENDL;
				block->mSize = block->mLength;
			}
			sync(block);
			unlockData();
			return TRUE;
		}
		else if (max_size > block->mLength)
		{
			S32 size_increase = max_size - block->mLength;
			LLVFSBlock *free_block;
			blocks_location_map_t::iterator iter = mFreeBlocksByLocation.upper_bound(block->mLocation);
			if (iter != mFreeBlocksByLocation.end())
			{
				free_block = iter->second;
				if (free_block->mLocation == block->mLocation + block->mLength &&
					free_block->mLength >= size_increase)
				{
					useFreeSpace(free_block, size_increase);
					block->mLength += size_increase;
					sync(block);
					unlockData();
					return TRUE;
				}
			}
			free_block = findFreeBlock(max_size, block);
			if (free_block)
			{
				U32 new_data_location = free_block->mLocation;
				useFreeSpace(free_block, max_size);
				if (block->mLength > 0)
				{
					LLVFSBlock *new_free_block = new LLVFSBlock(block->mLocation, block->mLength);
					addFreeBlock(new_free_block);
					if (block->mSize > 0)
					{
						std::vector<U8> buffer(block->mSize);
						fseek(mDataFP, block->mLocation, SEEK_SET);
						if (fread(&buffer[0], block->mSize, 1, mDataFP) == 1)
						{
							fseek(mDataFP, new_data_location, SEEK_SET);
							if (fwrite(&buffer[0], block->mSize, 1, mDataFP) != 1)
							{
								LL_WARNS() << "Short write" << LL_ENDL;
							}
						} else {
							LL_WARNS() << "Short read" << LL_ENDL;
						}
					}
				}
				block->mLocation = new_data_location;
				block->mLength = max_size;
				sync(block);
				unlockData();
				return TRUE;
			}
			else
			{
				LL_WARNS() << "VFS: No space (" << max_size << ") to resize existing vfile " << file_id << LL_ENDL;
				unlockData();
				dumpStatistics();
				return FALSE;
			}
		}
	}
	else
	{
		LLVFSBlock *free_block = findFreeBlock(max_size);
		if (free_block)
		{
			if (block)
			{
				block->mLocation = free_block->mLocation;
				block->mLength = max_size;
			}
			else
			{
				block = new LLVFSFileBlock(file_id, file_type, free_block->mLocation, max_size);
				mFileBlocks.insert(fileblock_map::value_type(spec, block));
			}
			useFreeSpace(free_block, max_size);
			block->mAccessTime = (U32)time(NULL);
			sync(block);
		}
		else
		{
			LL_WARNS() << "VFS: No space (" << max_size << ") for new virtual file " << file_id << LL_ENDL;
			unlockData();
			dumpStatistics();
			return FALSE;
		}
	}
	unlockData();
	return TRUE;
}
void LLVFS::renameFile(const LLUUID &file_id, const LLAssetType::EType file_type,
					   const LLUUID &new_id, const LLAssetType::EType &new_type)
{
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	if (mReadOnly)
	{
		LL_ERRS() << "Attempt to write to read-only VFS" << LL_ENDL;
	}
	lockData();
	LLVFSFileSpecifier new_spec(new_id, new_type);
	LLVFSFileSpecifier old_spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(old_spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *src_block = (*it).second;
		fileblock_map::iterator new_it = mFileBlocks.find(new_spec);
		if (new_it != mFileBlocks.end())
		{
			LLVFSFileBlock *new_block = (*new_it).second;
			removeFileBlock(new_block);
		}
		it = mFileBlocks.find(new_spec);
		if (it != mFileBlocks.end())
		{
			LLVFSFileBlock *dest_block = (*it).second;
			for (S32 i = 0; i < (S32)VFSLOCK_COUNT; i++)
			{
				if(dest_block->mLocks[i])
				{
					LL_ERRS() << "Renaming VFS block to a locked file." << LL_ENDL;
				}
				dest_block->mLocks[i] = src_block->mLocks[i];
			}
			mFileBlocks.erase(new_spec);
			delete dest_block;
		}
		src_block->mFileID = new_id;
		src_block->mFileType = new_type;
		src_block->mAccessTime = (U32)time(NULL);
		mFileBlocks.erase(old_spec);
		mFileBlocks.insert(fileblock_map::value_type(new_spec, src_block));
		sync(src_block);
	}
	else
	{
		LL_WARNS() << "VFS: Attempt to rename nonexistent vfile " << file_id << ":" << file_type << LL_ENDL;
	}
	unlockData();
}
void LLVFS::removeFileBlock(LLVFSFileBlock *fileblock)
{
	sync(fileblock, TRUE);
	if (fileblock->mLength > 0)
	{
		LLVFSBlock *free_block = new LLVFSBlock(fileblock->mLocation, fileblock->mLength);
		addFreeBlock(free_block);
	}
	fileblock->mLocation = 0;
	fileblock->mSize = 0;
	fileblock->mLength = BLOCK_LENGTH_INVALID;
	fileblock->mIndexLocation = -1;
}
void LLVFS::removeFile(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	if (mReadOnly)
	{
		LL_ERRS() << "Attempt to write to read-only VFS" << LL_ENDL;
	}
    lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		removeFileBlock(block);
	}
	else
	{
		LL_WARNS() << "VFS: attempting to remove nonexistent file " << file_id << " type " << file_type << LL_ENDL;
	}
	unlockData();
}
S32 LLVFS::getData(const LLUUID &file_id, const LLAssetType::EType file_type, U8 *buffer, S32 location, S32 length)
{
	S32 bytesread = 0;
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	llassert(location >= 0);
	llassert(length >= 0);
	BOOL do_read = FALSE;
    lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		block->mAccessTime = (U32)time(NULL);
		if (location > block->mSize)
		{
			LL_WARNS() << "VFS: Attempt to read location " << location << " in file " << file_id << " of length " << block->mSize << LL_ENDL;
		}
		else
		{
			if (length > block->mSize - location)
			{
				length = block->mSize - location;
			}
			location += block->mLocation;
			do_read = TRUE;
		}
	}
	if (do_read)
	{
		fseek(mDataFP, location, SEEK_SET);
		bytesread = (S32)fread(buffer, 1, length, mDataFP);
	}
	unlockData();
	return bytesread;
}
S32 LLVFS::storeData(const LLUUID &file_id, const LLAssetType::EType file_type, const U8 *buffer, S32 location, S32 length)
{
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	if (mReadOnly)
	{
		LL_ERRS() << "Attempt to write to read-only VFS" << LL_ENDL;
	}
	llassert(length > 0);
    lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		S32 in_loc = location;
		if (location == -1)
		{
			location = block->mSize;
		}
		llassert(location >= 0);
		block->mAccessTime = (U32)time(NULL);
		if (block->mLength == BLOCK_LENGTH_INVALID)
		{
			LL_WARNS() << "VFS: Attempt to write to invalid block"
					<< " in file " << file_id
					<< " location: " << in_loc
					<< " bytes: " << length
					<< LL_ENDL;
			unlockData();
			return length;
		}
		else if (location > block->mLength)
		{
			LL_WARNS() << "VFS: Attempt to write to location " << location
					<< " in file " << file_id
					<< " type " << S32(file_type)
					<< " of size " << block->mSize
					<< " block length " << block->mLength
					<< LL_ENDL;
			unlockData();
			return length;
		}
		else
		{
			if (length > block->mLength - location )
			{
				LL_WARNS() << "VFS: Truncating write to virtual file " << file_id << " type " << S32(file_type) << LL_ENDL;
				length = block->mLength - location;
			}
			U32 file_location = location + block->mLocation;
			fseek(mDataFP, file_location, SEEK_SET);
			S32 write_len = (S32)fwrite(buffer, 1, length, mDataFP);
			if (write_len != length)
			{
				LL_WARNS() << llformat("VFS Write Error: %d != %d",write_len,length) << LL_ENDL;
			}
			if (location + length > block->mSize)
			{
				block->mSize = location + write_len;
				sync(block);
			}
			unlockData();
			return write_len;
		}
	}
	else
	{
		unlockData();
		return 0;
	}
}
void LLVFS::incLock(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock)
{
	lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
	LLVFSFileBlock *block;
 	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		block = (*it).second;
	}
	else
	{
		block = new LLVFSFileBlock(file_id, file_type, 0, BLOCK_LENGTH_INVALID);
    	block->mAccessTime = (U32)time(NULL);
		mFileBlocks.insert(fileblock_map::value_type(spec, block));
	}
	block->mLocks[lock]++;
	mLockCounts[lock]++;
	unlockData();
}
void LLVFS::decLock(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock)
{
	lockData();
	LLVFSFileSpecifier spec(file_id, file_type);
 	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		if (block->mLocks[lock] > 0)
		{
			block->mLocks[lock]--;
		}
		else
		{
			LL_WARNS() << "VFS: Decrementing zero-value lock " << lock << LL_ENDL;
		}
		mLockCounts[lock]--;
	}
	unlockData();
}
BOOL LLVFS::isLocked(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock)
{
	lockData();
	BOOL res = FALSE;
	LLVFSFileSpecifier spec(file_id, file_type);
 	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		res = (block->mLocks[lock] > 0);
	}
	unlockData();
	return res;
}
void LLVFS::eraseBlockLength(LLVFSBlock *block)
{
	S32 length = block->mLength;
	blocks_length_map_t::iterator iter = mFreeBlocksByLength.lower_bound(length);
	blocks_length_map_t::iterator end = mFreeBlocksByLength.end();
	bool found_block = false;
	while(iter != end)
	{
		LLVFSBlock *tblock = iter->second;
		llassert(tblock->mLength == length);
		if (tblock == block)
		{
			mFreeBlocksByLength.erase(iter);
			found_block = true;
			break;
		}
		++iter;
	}
	if(!found_block)
	{
		LL_ERRS() << "eraseBlock could not find block" << LL_ENDL;
	}
}
void LLVFS::eraseBlock(LLVFSBlock *block)
{
	eraseBlockLength(block);
	U32 location = block->mLocation;
	llverify(mFreeBlocksByLocation.erase(location) == 1);
}
void LLVFS::addFreeBlock(LLVFSBlock *block)
{
#if LL_DEBUG
	size_t dbgcount = mFreeBlocksByLocation.count(block->mLocation);
	if(dbgcount > 0)
	{
		LL_ERRS() << "addFreeBlock called with block already in list" << LL_ENDL;
	}
#endif
	blocks_location_map_t::iterator next_free_it = mFreeBlocksByLocation.lower_bound(block->mLocation);
	LLVFSBlock* prev_block = NULL;
	bool merge_prev = false;
	if (next_free_it != mFreeBlocksByLocation.begin())
	{
		blocks_location_map_t::iterator prev_free_it = next_free_it;
		--prev_free_it;
		prev_block = prev_free_it->second;
		merge_prev = (prev_block->mLocation + prev_block->mLength == block->mLocation);
	}
	LLVFSBlock* next_block = NULL;
	bool merge_next = false;
	if (next_free_it != mFreeBlocksByLocation.end())
	{
		next_block = next_free_it->second;
		merge_next = (block->mLocation + block->mLength == next_block->mLocation);
	}
	if (merge_prev && merge_next)
	{
		eraseBlockLength(prev_block);
		eraseBlock(next_block);
		prev_block->mLength += block->mLength + next_block->mLength;
		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(prev_block->mLength, prev_block));
		delete block;
		block = NULL;
		delete next_block;
		next_block = NULL;
	}
	else if (merge_prev)
	{
		eraseBlockLength(prev_block);
		prev_block->mLength += block->mLength;
		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(prev_block->mLength, prev_block));
		delete block;
		block = NULL;
	}
	else if (merge_next)
	{
		eraseBlock(next_block);
		next_block->mLocation = block->mLocation;
		next_block->mLength += block->mLength;
		mFreeBlocksByLocation.insert(blocks_location_map_t::value_type(next_block->mLocation, next_block));
		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(next_block->mLength, next_block));
		delete block;
		block = NULL;
	}
	else
	{
 		mFreeBlocksByLocation.insert(next_free_it, blocks_location_map_t::value_type(block->mLocation, block));
 		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(block->mLength, block));
	}
}
void LLVFS::useFreeSpace(LLVFSBlock *free_block, S32 length)
{
	if (free_block->mLength == length)
	{
		eraseBlock(free_block);
		delete free_block;
	}
	else
	{
		eraseBlock(free_block);
		free_block->mLocation += length;
		free_block->mLength -= length;
		addFreeBlock(free_block);
	}
}
void LLVFS::sync(LLVFSFileBlock *block, BOOL remove)
{
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	if (mReadOnly)
	{
		LL_WARNS() << "Attempt to sync read-only VFS" << LL_ENDL;
		return;
	}
	if (block->mLength == BLOCK_LENGTH_INVALID)
	{
		return;
	}
	if (block->mLength == 0)
	{
		LL_ERRS() << "VFS syncing zero-length block" << LL_ENDL;
	}
    BOOL set_index_to_end = FALSE;
	long seek_pos = block->mIndexLocation;
	if (-1 == seek_pos)
	{
		if (!mIndexHoles.empty())
		{
			seek_pos = mIndexHoles.front();
			mIndexHoles.pop_front();
		}
		else
		{
			set_index_to_end = TRUE;
		}
	}
    if (set_index_to_end)
	{
		fseek(mIndexFP, 0, SEEK_END);
		seek_pos = ftell(mIndexFP);
	}
	block->mIndexLocation = seek_pos;
	if (remove)
	{
		mIndexHoles.push_back(seek_pos);
	}
	U8 buffer[LLVFSFileBlock::SERIAL_SIZE];
	if (remove)
	{
		memset(buffer, 0, LLVFSFileBlock::SERIAL_SIZE);
	}
	else
	{
		block->serialize(buffer);
	}
	if (!set_index_to_end)
	{
		fseek(mIndexFP, seek_pos, SEEK_SET);
	}
	if (fwrite(buffer, LLVFSFileBlock::SERIAL_SIZE, 1, mIndexFP) != 1)
	{
		LL_WARNS() << "Short write" << LL_ENDL;
	}
	return;
}
LLVFSBlock *LLVFS::findFreeBlock(S32 size, LLVFSFileBlock *immune)
{
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	LLVFSBlock *block = NULL;
	BOOL have_lru_list = FALSE;
	typedef std::set<LLVFSFileBlock*, LLVFSFileBlock_less> lru_set;
	lru_set lru_list;
	LLTimer timer;
	while (! block)
	{
		blocks_length_map_t::iterator iter = mFreeBlocksByLength.lower_bound(size);
		if (iter != mFreeBlocksByLength.end())
			block = iter->second;
		if (! block)
		{
			if (! have_lru_list)
			{
				for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
				{
					LLVFSFileBlock *tmp = (*it).second;
					if (tmp != immune &&
						tmp->mLength > 0 &&
						! tmp->mLocks[VFSLOCK_READ] &&
						! tmp->mLocks[VFSLOCK_APPEND] &&
						! tmp->mLocks[VFSLOCK_OPEN])
					{
						lru_list.insert(tmp);
					}
				}
				have_lru_list = TRUE;
			}
			if (lru_list.size() == 0)
			{
				LL_WARNS() << "VFS: Can't make " << size << " bytes of free space in VFS, giving up" << LL_ENDL;
				break;
			}
			lru_set::iterator it = lru_list.begin();
			LLVFSFileBlock *file_block = *it;
			if (file_block->mLength >= size && file_block != immune)
			{
				LL_INFOS() << "LRU: Removing " << file_block->mFileID << ":" << file_block->mFileType << LL_ENDL;
				lru_list.erase(it);
				removeFileBlock(file_block);
				file_block = NULL;
				continue;
			}
			U32 cleanup_target = (size > VFS_CLEANUP_SIZE) ? size : VFS_CLEANUP_SIZE;
			U32 cleaned_up = 0;
		   	for (it = lru_list.begin();
				 it != lru_list.end() && cleaned_up < cleanup_target;
				 )
			{
				file_block = *it;
				cleaned_up += file_block->mLength;
				lru_list.erase(it++);
				removeFileBlock(file_block);
				file_block = NULL;
			}
		}
	}
	F32 time = timer.getElapsedTimeF32();
	if (time > 0.5f)
	{
		LL_WARNS() << "VFS: Spent " << time << " seconds in findFreeBlock!" << LL_ENDL;
	}
	return block;
}
void LLVFS::pokeFiles()
{
	if (!isValid())
	{
		LL_ERRS() << "Attempting to use invalid VFS!" << LL_ENDL;
	}
	U32 word;
	fseek(mDataFP, 0, SEEK_SET);
	if (fread(&word, sizeof(word), 1, mDataFP) == 1)
	{
		fseek(mDataFP, 0, SEEK_SET);
		if (fwrite(&word, sizeof(word), 1, mDataFP) != 1)
		{
			LL_WARNS() << "Could not write to data file" << LL_ENDL;
		}
		fflush(mDataFP);
	}
	fseek(mIndexFP, 0, SEEK_SET);
	if (fread(&word, sizeof(word), 1, mIndexFP) == 1)
	{
		fseek(mIndexFP, 0, SEEK_SET);
		if (fwrite(&word, sizeof(word), 1, mIndexFP) != 1)
		{
			LL_WARNS() << "Could not write to index file" << LL_ENDL;
		}
		fflush(mIndexFP);
	}
}
void LLVFS::dumpMap()
{
	LL_INFOS() << "Files:" << LL_ENDL;
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileBlock *file_block = (*it).second;
		LL_INFOS() << "Location: " << file_block->mLocation << "\tLength: " << file_block->mLength << "\t" << file_block->mFileID << "\t" << file_block->mFileType << LL_ENDL;
	}
	LL_INFOS() << "Free Blocks:" << LL_ENDL;
	for (blocks_location_map_t::iterator iter = mFreeBlocksByLocation.begin(),
			 end = mFreeBlocksByLocation.end();
		 iter != end; iter++)
	{
		LLVFSBlock *free_block = iter->second;
		LL_INFOS() << "Location: " << free_block->mLocation << "\tLength: " << free_block->mLength << LL_ENDL;
	}
}
void LLVFS::audit()
{
	LLMutexLock lock_data(mDataMutex);
	fflush(mIndexFP);
	fseek(mIndexFP, 0, SEEK_END);
	size_t index_size = ftell(mIndexFP);
	fseek(mIndexFP, 0, SEEK_SET);
	BOOL vfs_corrupt = FALSE;
	std::vector<U8> buffer(llmax<size_t>(index_size,1U));
	if (fread(&buffer[0], 1, index_size, mIndexFP) != index_size)
	{
		LL_WARNS() << "Index truncated" << LL_ENDL;
		vfs_corrupt = TRUE;
	}
	size_t buf_offset = 0;
	std::map<LLVFSFileSpecifier, LLVFSFileBlock*>	found_files;
	U32 cur_time = (U32)time(NULL);
	std::vector<LLVFSFileBlock*> audit_blocks;
	while (!vfs_corrupt && buf_offset < index_size)
	{
		LLVFSFileBlock *block = new LLVFSFileBlock();
		audit_blocks.push_back(block);
		block->deserialize(&buffer[buf_offset], (S32)buf_offset);
		buf_offset += block->SERIAL_SIZE;
		if (block->mLength >= 0 &&
			block->mSize >= 0 &&
			block->mSize <= block->mLength &&
			block->mFileType >= LLAssetType::AT_NONE &&
			block->mFileType < LLAssetType::AT_COUNT &&
			block->mAccessTime <= cur_time &&
			block->mFileID != LLUUID::null)
		{
			if (mFileBlocks.find(*block) == mFileBlocks.end())
			{
				LL_WARNS() << "VFile " << block->mFileID << ":" << block->mFileType << " on disk, not in memory, loc " << block->mIndexLocation << LL_ENDL;
			}
			else if (found_files.find(*block) != found_files.end())
			{
				std::map<LLVFSFileSpecifier, LLVFSFileBlock*>::iterator it;
				it = found_files.find(*block);
				LLVFSFileBlock* dupe = it->second;
				unlockAndClose(mIndexFP);
				mIndexFP = NULL;
				unlockAndClose(mDataFP);
				mDataFP = NULL;
				LL_WARNS() << "VFS: Original block index " << block->mIndexLocation
					<< " location " << block->mLocation
					<< " length " << block->mLength
					<< " size " << block->mSize
					<< " id " << block->mFileID
					<< " type " << block->mFileType
					<< LL_ENDL;
				LL_WARNS() << "VFS: Duplicate block index " << dupe->mIndexLocation
					<< " location " << dupe->mLocation
					<< " length " << dupe->mLength
					<< " size " << dupe->mSize
					<< " id " << dupe->mFileID
					<< " type " << dupe->mFileType
					<< LL_ENDL;
				LL_WARNS() << "VFS: Index size " << index_size << LL_ENDL;
				LL_WARNS() << "VFS: INDEX CORRUPT" << LL_ENDL;
				vfs_corrupt = TRUE;
				break;
			}
			else
			{
				found_files[*block] = block;
			}
		}
		else
		{
			if (block->mLength)
			{
				LL_WARNS() << "VFile " << block->mFileID << ":" << block->mFileType << " corrupt on disk" << LL_ENDL;
			}
		}
	}
	if (!vfs_corrupt)
	{
		for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
		{
			LLVFSFileBlock* block = (*it).second;
			if (block->mSize > 0)
			{
				if (! found_files.count(*block))
				{
					LL_WARNS() << "VFile " << block->mFileID << ":" << block->mFileType << " in memory, not on disk, loc " << block->mIndexLocation<< LL_ENDL;
					fseek(mIndexFP, block->mIndexLocation, SEEK_SET);
					U8 buf[LLVFSFileBlock::SERIAL_SIZE];
					if (fread(buf, LLVFSFileBlock::SERIAL_SIZE, 1, mIndexFP) != 1)
					{
						LL_WARNS() << "VFile " << block->mFileID
								<< " gave short read" << LL_ENDL;
					}
					LLVFSFileBlock disk_block;
					disk_block.deserialize(buf, block->mIndexLocation);
					LL_WARNS() << "Instead found " << disk_block.mFileID << ":" << block->mFileType << LL_ENDL;
				}
				else
				{
					block = found_files.find(*block)->second;
					found_files.erase(*block);
				}
			}
		}
		for (std::map<LLVFSFileSpecifier, LLVFSFileBlock*>::iterator iter = found_files.begin();
			 iter != found_files.end(); iter++)
		{
			LLVFSFileBlock* block = iter->second;
			LL_WARNS() << "VFile " << block->mFileID << ":" << block->mFileType << " szie:" << block->mSize << " leftover" << LL_ENDL;
		}
		LL_INFOS() << "VFS: audit OK" << LL_ENDL;
	}
	for_each(audit_blocks.begin(), audit_blocks.end(), DeletePointer());
}
void LLVFS::checkMem()
{
	lockData();
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileBlock *block = (*it).second;
		llassert(block->mFileType >= LLAssetType::AT_NONE &&
				 block->mFileType < LLAssetType::AT_COUNT &&
				 block->mFileID != LLUUID::null);
		for (std::deque<S32>::iterator iter = mIndexHoles.begin();
			 iter != mIndexHoles.end(); ++iter)
		{
			S32 index_loc = *iter;
			if (index_loc == block->mIndexLocation)
			{
				LL_WARNS() << "VFile block " << block->mFileID << ":" << block->mFileType << " is marked as a hole" << LL_ENDL;
			}
		}
	}
	LL_INFOS() << "VFS: mem check OK" << LL_ENDL;
	unlockData();
}
void LLVFS::dumpLockCounts()
{
	S32 i;
	for (i = 0; i < VFSLOCK_COUNT; i++)
	{
		LL_INFOS() << "LockType: " << i << ": " << mLockCounts[i] << LL_ENDL;
	}
}
void LLVFS::dumpStatistics()
{
	lockData();
	std::map<S32, S32> size_counts;
	std::map<U32, S32> location_counts;
	std::map<LLAssetType::EType, std::pair<S32,S32> > filetype_counts;
	S32 max_file_size = 0;
	S32 total_file_size = 0;
	S32 invalid_file_count = 0;
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileBlock *file_block = (*it).second;
		if (file_block->mLength == BLOCK_LENGTH_INVALID)
		{
			invalid_file_count++;
		}
		else if (file_block->mLength <= 0)
		{
			LL_INFOS() << "Bad file block at: " << file_block->mLocation << "\tLength: " << file_block->mLength << "\t" << file_block->mFileID << "\t" << file_block->mFileType << LL_ENDL;
			size_counts[file_block->mLength]++;
			location_counts[file_block->mLocation]++;
		}
		else
		{
			total_file_size += file_block->mLength;
		}
		if (file_block->mLength > max_file_size)
		{
			max_file_size = file_block->mLength;
		}
		filetype_counts[file_block->mFileType].first++;
		filetype_counts[file_block->mFileType].second += file_block->mLength;
	}
	for (std::map<S32,S32>::iterator it = size_counts.begin(); it != size_counts.end(); ++it)
	{
		S32 size = it->first;
		S32 size_count = it->second;
		LL_INFOS() << "Bad files size " << size << " count " << size_count << LL_ENDL;
	}
	for (std::map<U32,S32>::iterator it = location_counts.begin(); it != location_counts.end(); ++it)
	{
		U32 location = it->first;
		S32 location_count = it->second;
		LL_INFOS() << "Bad files location " << location << " count " << location_count << LL_ENDL;
	}
	S32 max_free_size = 0;
	S32 total_free_size = 0;
	std::map<S32, S32> free_length_counts;
	for (blocks_location_map_t::iterator iter = mFreeBlocksByLocation.begin(),
			 end = mFreeBlocksByLocation.end();
		 iter != end; iter++)
	{
		LLVFSBlock *free_block = iter->second;
		if (free_block->mLength <= 0)
		{
			LL_INFOS() << "Bad free block at: " << free_block->mLocation << "\tLength: " << free_block->mLength << LL_ENDL;
		}
		else
		{
			LL_INFOS() << "Block: " << free_block->mLocation
					<< "\tLength: " << free_block->mLength
					<< "\tEnd: " << free_block->mLocation + free_block->mLength
					<< LL_ENDL;
			total_free_size += free_block->mLength;
		}
		if (free_block->mLength > max_free_size)
		{
			max_free_size = free_block->mLength;
		}
		free_length_counts[free_block->mLength]++;
	}
	for (std::map<S32,S32>::iterator it = free_length_counts.begin(); it != free_length_counts.end(); ++it)
	{
		LL_INFOS() << "Free length " << it->first << " count " << it->second << LL_ENDL;
	}
	LL_INFOS() << "Invalid blocks: " << invalid_file_count << LL_ENDL;
	LL_INFOS() << "File blocks:    " << mFileBlocks.size() << LL_ENDL;
	S32 length_list_count = (S32)mFreeBlocksByLength.size();
	S32 location_list_count = (S32)mFreeBlocksByLocation.size();
	if (length_list_count == location_list_count)
	{
		LL_INFOS() << "Free list lengths match, free blocks: " << location_list_count << LL_ENDL;
	}
	else
	{
		LL_WARNS() << "Free list lengths do not match!" << LL_ENDL;
		LL_WARNS() << "By length: " << length_list_count << LL_ENDL;
		LL_WARNS() << "By location: " << location_list_count << LL_ENDL;
	}
	LL_INFOS() << "Max file: " << max_file_size/1024 << "K" << LL_ENDL;
	LL_INFOS() << "Max free: " << max_free_size/1024 << "K" << LL_ENDL;
	LL_INFOS() << "Total file size: " << total_file_size/1024 << "K" << LL_ENDL;
	LL_INFOS() << "Total free size: " << total_free_size/1024 << "K" << LL_ENDL;
	LL_INFOS() << "Sum: " << (total_file_size + total_free_size) << " bytes" << LL_ENDL;
	LL_INFOS() << llformat("%.0f%% full",((F32)(total_file_size)/(F32)(total_file_size+total_free_size))*100.f) << LL_ENDL;
	LL_INFOS() << " " << LL_ENDL;
	for (std::map<LLAssetType::EType, std::pair<S32,S32> >::iterator iter = filetype_counts.begin();
		 iter != filetype_counts.end(); ++iter)
	{
		LL_INFOS() << "Type: " << LLAssetType::getDesc(iter->first)
				<< " Count: " << iter->second.first
				<< " Bytes: " << (iter->second.second>>20) << " MB" << LL_ENDL;
	}
	{
 		blocks_location_map_t::iterator iter = mFreeBlocksByLocation.begin();
 		blocks_location_map_t::iterator end = mFreeBlocksByLocation.end();
 		LLVFSBlock *first_block = iter->second;
 		while(iter != end)
 		{
 			if (++iter == end)
 				break;
 			LLVFSBlock *second_block = iter->second;
 			if (first_block->mLocation + first_block->mLength == second_block->mLocation)
 			{
				LL_INFOS() << "Potential merge at " << first_block->mLocation << LL_ENDL;
 			}
 			first_block = second_block;
 		}
	}
	unlockData();
}
std::string get_extension(LLAssetType::EType type)
{
	std::string extension;
	switch(type)
	{
	case LLAssetType::AT_TEXTURE:
		extension = ".jp2";
		break;
	case LLAssetType::AT_SOUND:
		extension = ".ogg";
		break;
	case LLAssetType::AT_SOUND_WAV:
		extension = ".wav";
		break;
	case LLAssetType::AT_TEXTURE_TGA:
		extension = ".tga";
		break;
	case LLAssetType::AT_ANIMATION:
		extension = ".lla";
		break;
#if 0
	case LLAssetType::AT_MESH:
		extension = ".slm";
		break;
#endif
	default:
		extension += ".";
		extension += LLAssetType::lookup(type);
		break;
	}
	return extension;
}
void LLVFS::listFiles()
{
	lockData();
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileSpecifier file_spec = it->first;
		LLVFSFileBlock *file_block = it->second;
		S32 length = file_block->mLength;
		S32 size = file_block->mSize;
		if (length != BLOCK_LENGTH_INVALID && size > 0)
		{
			LLUUID id = file_spec.mFileID;
			std::string extension = get_extension(file_spec.mFileType);
			LL_INFOS() << " File: " << id
					<< " Type: " << LLAssetType::getDesc(file_spec.mFileType)
					<< " Size: " << size
					<< LL_ENDL;
		}
	}
	unlockData();
}
std::map<LLVFSFileSpecifier, LLVFSFileBlock*> LLVFS::getFileList()
{
	lockData();
	fileblock_map mFileList = mFileBlocks;
	unlockData();
	return mFileList;
}
#include "llapr.h"
void LLVFS::dumpFiles()
{
	lockData();
	S32 files_extracted = 0;
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileSpecifier file_spec = it->first;
		LLVFSFileBlock *file_block = it->second;
		S32 length = file_block->mLength;
		S32 size = file_block->mSize;
		if (length != BLOCK_LENGTH_INVALID && size > 0)
		{
			LLUUID id = file_spec.mFileID;
			LLAssetType::EType type = file_spec.mFileType;
			std::vector<U8> buffer(size);
			unlockData();
			getData(id, type, &buffer[0], 0, size);
			lockData();
			std::string extension = get_extension(type);
			std::string filename = id.asString() + extension;
			LL_INFOS() << " Writing " << filename << LL_ENDL;
			LLAPRFile outfile(filename, LL_APR_WB);
			outfile.write(&buffer[0], size);
			outfile.close();
			files_extracted++;
		}
	}
	unlockData();
	LL_INFOS() << "Extracted " << files_extracted << " files out of " << mFileBlocks.size() << LL_ENDL;
}
LLFILE *LLVFS::openAndLock(const std::string& filename, const char* mode, BOOL read_lock)
{
#if LL_WINDOWS
	return LLFile::_fsopen(filename, mode, (read_lock ? _SH_DENYWR : _SH_DENYRW));
#else
	LLFILE *fp;
	int fd;
#if LL_SOLARIS
        struct flock fl;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 1;
#else
	if (strchr(mode, 'w') != NULL)
	{
		fp = LLFile::fopen(filename, "rb");
		if (fp)
		{
			fd = fileno(fp);
			if (flock(fd, (read_lock ? LOCK_SH : LOCK_EX) | LOCK_NB) == -1)
			{
				fclose(fp);
				return NULL;
			}
			fclose(fp);
		}
	}
#endif
	fp = LLFile::fopen(filename, mode);
	if (fp)
	{
		fd = fileno(fp);
#if LL_SOLARIS
                fl.l_type = read_lock ? F_RDLCK : F_WRLCK;
                if (fcntl(fd, F_SETLK, &fl) == -1)
#else
		if (flock(fd, (read_lock ? LOCK_SH : LOCK_EX) | LOCK_NB) == -1)
#endif
		{
			fclose(fp);
			fp = NULL;
		}
	}
	return fp;
#endif
}
void LLVFS::unlockAndClose(LLFILE *fp)
{
	if (fp)
	{
#if LL_SOLARIS
	        struct flock fl;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 1;
		fl.l_type = F_UNLCK;
		fcntl(fileno(fp), F_SETLK, &fl);
#endif
		fclose(fp);
	}
}
