/**
 * @file llmemory.h
 * @brief Memory allocation/deallocation header-stuff goes here.
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
#ifndef LLMEMORY_H
#define LLMEMORY_H
#include "linden_common.h"
#include "llunits.h"
#include "stdtypes.h"
#include <new>
#include <cstdlib>
#if !LL_WINDOWS
#include <stdint.h>
#endif
class LLMutex ;
#if LL_WINDOWS && LL_DEBUG
#define LL_CHECK_MEMORY llassert(_CrtCheckMemory());
#else
#define LL_CHECK_MEMORY
#endif
#if LL_WINDOWS
#define LL_ALIGN_OF __alignof
#else
#define LL_ALIGN_OF __align_of__
#endif
#if LL_WINDOWS
#define LL_DEFAULT_HEAP_ALIGN 8
#elif LL_DARWIN
#define LL_DEFAULT_HEAP_ALIGN 16
#endif
#ifdef SHOW_ASSERT
LL_COMMON_API void singu_alignment_check_failed(void);
#define ll_assert_aligned(ptr,alignment)											\
	do																				\
	{																				\
	  if (LL_UNLIKELY(reinterpret_cast<intptr_t>(ptr) % alignment))					\
	  {																				\
		singu_alignment_check_failed();												\
	  }																				\
	}																				\
	while(0)
#else
#define ll_assert_aligned(ptr,alignment)
#endif
#include <xmmintrin.h>
template <typename T> T* LL_NEXT_ALIGNED_ADDRESS(T* address)
{
	return reinterpret_cast<T*>(
		(reinterpret_cast<uintptr_t>(address) + 0xF) & ~0xF);
}
template <typename T> T* LL_NEXT_ALIGNED_ADDRESS_64(T* address)
{
	return reinterpret_cast<T*>(
		(reinterpret_cast<uintptr_t>(address) + 0x3F) & ~0x3F);
}
#if LL_DARWIN
#define			LL_ALIGN_PREFIX(x)
#define			LL_ALIGN_POSTFIX(x)		__attribute__((aligned(x)))
#elif LL_WINDOWS
#define			LL_ALIGN_PREFIX(x)		__declspec(align(x))
#define			LL_ALIGN_POSTFIX(x)
#else
#error "LL_ALIGN_PREFIX and LL_ALIGN_POSTFIX undefined"
#endif
#define LL_ALIGN_16(var) LL_ALIGN_PREFIX(16) var LL_ALIGN_POSTFIX(16)
#if 0 && defined(LL_WINDOWS)
	void* ll_aligned_malloc_fallback( size_t size, int align );
	void ll_aligned_free_fallback( void* ptr );
#else
	inline void* ll_aligned_malloc_fallback( size_t size, int align )
	{
	#if defined(LL_WINDOWS)
		return _aligned_malloc(size, align);
	#else
        char* aligned = NULL;
		void* mem = malloc( size + (align - 1) + sizeof(void*) );
        if (mem)
        {
            aligned = ((char*)mem) + sizeof(void*);
            aligned += align - ((uintptr_t)aligned & (align - 1));
            ((void**)aligned)[-1] = mem;
        }
		return aligned;
	#endif
	}
	inline void ll_aligned_free_fallback( void* ptr )
	{
	#if defined(LL_WINDOWS)
		_aligned_free(ptr);
	#else
		if (ptr)
		{
			free( ((void**)ptr)[-1] );
		}
	#endif
	}
#endif
#if !LL_USE_TCMALLOC
inline void* ll_aligned_malloc_16(size_t size)
{
#if defined(LL_WINDOWS)
	return _aligned_malloc(size, 16);
#elif defined(LL_DARWIN)
	return malloc(size);
#else
	void *rtn;
	if (LL_LIKELY(0 == posix_memalign(&rtn, 16, size)))
		return rtn;
	else
		return NULL;
#endif
}
inline void ll_aligned_free_16(void *p)
{
#if defined(LL_WINDOWS)
	_aligned_free(p);
#elif defined(LL_DARWIN)
	return free(p);
#else
	free(p);
#endif
}
inline void* ll_aligned_realloc_16(void* ptr, size_t size, size_t old_size)
{
#if defined(LL_WINDOWS)
	return _aligned_realloc(ptr, size, 16);
#elif defined(LL_DARWIN)
	return realloc(ptr,size);
#else
	void* ret = ll_aligned_malloc_16(size);
	if (ptr)
	{
		if (ret)
		{
			memcpy(ret, ptr, llmin(old_size, size));
		}
		ll_aligned_free_16(ptr);
	}
	return ret;
#endif
}
#else
#define ll_aligned_malloc_16 ::malloc
#define ll_aligned_realloc_16(a,b,c) ::realloc(a,b)
#define ll_aligned_free_16 ::free
#endif
inline void* ll_aligned_malloc_32(size_t size)
{
#if defined(LL_WINDOWS)
	return _aligned_malloc(size, 32);
#elif defined(LL_DARWIN)
	return ll_aligned_malloc_fallback( size, 32 );
#else
	void *rtn;
	if (LL_LIKELY(0 == posix_memalign(&rtn, 32, size)))
		return rtn;
	else
		return NULL;
#endif
}
inline void ll_aligned_free_32(void *p)
{
#if defined(LL_WINDOWS)
	_aligned_free(p);
#elif defined(LL_DARWIN)
	ll_aligned_free_fallback( p );
#else
	free(p);
#endif
}
template<size_t ALIGNMENT>
LL_FORCE_INLINE void* ll_aligned_malloc(size_t size)
{
	if (LL_DEFAULT_HEAP_ALIGN % ALIGNMENT == 0)
	{
		return malloc(size);
	}
	else if (ALIGNMENT == 16)
	{
		return ll_aligned_malloc_16(size);
	}
	else if (ALIGNMENT == 32)
	{
		return ll_aligned_malloc_32(size);
	}
	else
	{
		return ll_aligned_malloc_fallback(size, ALIGNMENT);
	}
}
template<size_t ALIGNMENT>
LL_FORCE_INLINE void ll_aligned_free(void* ptr)
{
	if (ALIGNMENT == LL_DEFAULT_HEAP_ALIGN)
	{
		free(ptr);
	}
	else if (ALIGNMENT == 16)
	{
		ll_aligned_free_16(ptr);
	}
	else if (ALIGNMENT == 32)
	{
		return ll_aligned_free_32(ptr);
	}
	else
	{
		return ll_aligned_free_fallback(ptr);
	}
}
inline void ll_memcpy_nonaliased_aligned_16(char* __restrict dst, const char* __restrict src, size_t bytes)
{
	assert(src != NULL);
	assert(dst != NULL);
	assert(bytes > 0);
	assert((bytes % sizeof(F32))== 0);
	ll_assert_aligned(src,16);
	ll_assert_aligned(dst,16);
	assert((src < dst) ? ((src + bytes) <= dst) : ((dst + bytes) <= src));
	assert(bytes%16==0);
	char* end = dst + bytes;
	if (bytes > 64)
	{
		void* begin_64 = LL_NEXT_ALIGNED_ADDRESS_64(dst);
		void* end_64 = end-64;
		_mm_prefetch((char*)begin_64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 128, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 192, _MM_HINT_NTA);
		while (dst < begin_64)
		{
			_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
			dst += 16;
			src += 16;
		}
		while (dst < end_64)
		{
			_mm_prefetch((char*)src + 512, _MM_HINT_NTA);
			_mm_prefetch((char*)dst + 512, _MM_HINT_NTA);
			_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
			_mm_store_ps((F32*)(dst + 16), _mm_load_ps((F32*)(src + 16)));
			_mm_store_ps((F32*)(dst + 32), _mm_load_ps((F32*)(src + 32)));
			_mm_store_ps((F32*)(dst + 48), _mm_load_ps((F32*)(src + 48)));
			dst += 64;
			src += 64;
		}
	}
	while (dst < end)
	{
		_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
		dst += 16;
		src += 16;
	}
}
#ifndef __DEBUG_PRIVATE_MEM__
#define __DEBUG_PRIVATE_MEM__  0
#endif
class LL_COMMON_API LLMemory
{
public:
	static void initClass();
	static void cleanupClass();
	static void freeReserve();
	static U64 getCurrentRSS();
	static void* tryToAlloc(void* address, U32 size);
	static void initMaxHeapSizeGB(F32Gigabytes max_heap_size, BOOL prevent_heap_failure);
	static void updateMemoryInfo() ;
	static void logMemoryInfo(BOOL update = FALSE);
	static bool isMemoryPoolLow();
	static U32Kilobytes getAvailableMemKB() ;
	static U32Kilobytes getMaxMemKB() ;
	static U32Kilobytes getAllocatedMemKB() ;
private:
	static char* reserveMem;
	static U32Kilobytes sAvailPhysicalMemInKB ;
	static U32Kilobytes sMaxPhysicalMemInKB ;
	static U32Kilobytes sAllocatedMemInKB;
	static U32Kilobytes sAllocatedPageSizeInKB ;
	static U32Kilobytes sMaxHeapSizeInKB;
	static BOOL sEnableMemoryFailurePrevention;
};
#if MEM_TRACK_MEM
class LLMutex ;
class LL_COMMON_API LLMemTracker
{
private:
	LLMemTracker() ;
	~LLMemTracker() ;
public:
	static void release() ;
	static LLMemTracker* getInstance() ;
	void track(const char* function, const int line) ;
	void preDraw(BOOL pause) ;
	void postDraw() ;
	const char* getNextLine() ;
private:
	static LLMemTracker* sInstance ;
	char**     mStringBuffer ;
	S32        mCapacity ;
	U32        mLastAllocatedMem ;
	S32        mCurIndex ;
	S32        mCounter;
	S32        mDrawnIndex;
	S32        mNumOfDrawn;
	BOOL       mPaused;
	LLMutex*   mMutexp ;
};
#define MEM_TRACK_RELEASE LLMemTracker::release() ;
#define MEM_TRACK         LLMemTracker::getInstance()->track(__FUNCTION__, __LINE__) ;
#else
#define MEM_TRACK_RELEASE
#define MEM_TRACK
#endif
class LL_COMMON_API LLPrivateMemoryPool
{
	friend class LLPrivateMemoryPoolManager ;
public:
	class LL_COMMON_API LLMemoryBlock
	{
	public:
		LLMemoryBlock() ;
		~LLMemoryBlock() ;
		void init(char* buffer, U32 buffer_size, U32 slot_size) ;
		void setBuffer(char* buffer, U32 buffer_size) ;
		char* allocate() ;
		void  freeMem(void* addr) ;
		bool empty() {return !mAllocatedSlots;}
		bool isFull() {return mAllocatedSlots == mTotalSlots;}
		bool isFree() {return !mTotalSlots;}
		U32  getSlotSize()const {return mSlotSize;}
		U32  getTotalSlots()const {return mTotalSlots;}
		U32  getBufferSize()const {return mBufferSize;}
		char* getBuffer() const {return mBuffer;}
		void resetBitMap() ;
	private:
		char* mBuffer;
		U32   mSlotSize ;
		U32   mBufferSize ;
		U32   mUsageBits ;
		U8    mTotalSlots ;
		U8    mAllocatedSlots ;
		U8    mDummySize ;
	public:
		LLMemoryBlock* mPrev ;
		LLMemoryBlock* mNext ;
		LLMemoryBlock* mSelf ;
		struct CompareAddress
		{
			bool operator()(const LLMemoryBlock* const& lhs, const LLMemoryBlock* const& rhs)
			{
				return (size_t)lhs->getBuffer() < (size_t)rhs->getBuffer();
			}
		};
	};
	class LL_COMMON_API LLMemoryChunk
	{
	public:
		LLMemoryChunk() ;
		~LLMemoryChunk() ;
		void init(char* buffer, U32 buffer_size, U32 min_slot_size, U32 max_slot_size, U32 min_block_size, U32 max_block_size) ;
		void setBuffer(char* buffer, U32 buffer_size) ;
		bool empty() ;
		char* allocate(U32 size) ;
		void  freeMem(void* addr) ;
		char* getBuffer() const {return mBuffer;}
		U32 getBufferSize() const {return mBufferSize;}
		U32 getAllocatedSize() const {return mAlloatedSize;}
		bool containsAddress(const char* addr) const;
		static U32 getMaxOverhead(U32 data_buffer_size, U32 min_slot_size,
													   U32 max_slot_size, U32 min_block_size, U32 max_block_size) ;
		void dump() ;
	private:
		U32 getPageIndex(char const* addr) ;
		U32 getBlockLevel(U32 size) ;
		U16 getPageLevel(U32 size) ;
		LLMemoryBlock* addBlock(U32 blk_idx) ;
		void popAvailBlockList(U32 blk_idx) ;
		void addToFreeSpace(LLMemoryBlock* blk) ;
		void removeFromFreeSpace(LLMemoryBlock* blk) ;
		void removeBlock(LLMemoryBlock* blk) ;
		void addToAvailBlockList(LLMemoryBlock* blk) ;
		U32  calcBlockSize(U32 slot_size);
		LLMemoryBlock* createNewBlock(LLMemoryBlock* blk, U32 buffer_size, U32 slot_size, U32 blk_idx) ;
	private:
		LLMemoryBlock** mAvailBlockList ;
		LLMemoryBlock** mFreeSpaceList;
		LLMemoryBlock*  mBlocks ;
		char* mBuffer ;
		U32   mBufferSize ;
		char* mDataBuffer ;
		char* mMetaBuffer ;
		U32   mMinBlockSize ;
		U32   mMinSlotSize ;
		U32   mMaxSlotSize ;
		U32   mAlloatedSize ;
		U16   mBlockLevels;
		U16   mPartitionLevels;
	public:
		LLMemoryChunk* mNext ;
		LLMemoryChunk* mPrev ;
	} ;
private:
	LLPrivateMemoryPool(S32 type, U32 max_pool_size) ;
	~LLPrivateMemoryPool() ;
	char *allocate(U32 size) ;
	void  freeMem(void* addr) ;
	void  dump() ;
	U32   getTotalAllocatedSize() ;
	U32   getTotalReservedSize() {return mReservedPoolSize;}
	S32   getType() const {return mType; }
	bool  isEmpty() const {return !mNumOfChunks; }
private:
	void lock() ;
	void unlock() ;
	S32 getChunkIndex(U32 size) ;
	LLMemoryChunk*  addChunk(S32 chunk_index) ;
	bool checkSize(U32 asked_size) ;
	void removeChunk(LLMemoryChunk* chunk) ;
	U16  findHashKey(const char* addr);
	void addToHashTable(LLMemoryChunk* chunk) ;
	void removeFromHashTable(LLMemoryChunk* chunk) ;
	void rehash() ;
	bool fillHashTable(U16 start, U16 end, LLMemoryChunk* chunk) ;
	LLMemoryChunk* findChunk(const char* addr) ;
	void destroyPool() ;
public:
	enum
	{
		SMALL_ALLOCATION = 0,
		MEDIUM_ALLOCATION,
		LARGE_ALLOCATION,
		SUPER_ALLOCATION
	};
	enum
	{
		STATIC = 0 ,
		VOLATILE,
		STATIC_THREADED,
		VOLATILE_THREADED,
		MAX_TYPES
	};
private:
	LLMutex* mMutexp ;
	U32  mMaxPoolSize;
	U32  mReservedPoolSize ;
	LLMemoryChunk* mChunkList[SUPER_ALLOCATION] ;
	U16 mNumOfChunks ;
	U16 mHashFactor ;
	S32 mType ;
	class LLChunkHashElement
	{
	public:
		LLChunkHashElement() {mFirst = NULL ; mSecond = NULL ;}
		bool add(LLMemoryChunk* chunk) ;
		void remove(LLMemoryChunk* chunk) ;
		LLMemoryChunk* findChunk(const char* addr) ;
		bool empty() {return !mFirst && !mSecond; }
		bool full()  {return mFirst && mSecond; }
		bool hasElement(LLMemoryChunk* chunk) {return mFirst == chunk || mSecond == chunk;}
	private:
		LLMemoryChunk* mFirst ;
		LLMemoryChunk* mSecond ;
	};
	std::vector<LLChunkHashElement> mChunkHashList ;
};
class LL_COMMON_API LLPrivateMemoryPoolManager
{
private:
	LLPrivateMemoryPoolManager(BOOL enabled, U32 max_pool_size) ;
	~LLPrivateMemoryPoolManager() ;
public:
	static LLPrivateMemoryPoolManager* getInstance() ;
	static void initClass(BOOL enabled, U32 pool_size) ;
	static void destroyClass() ;
	LLPrivateMemoryPool* newPool(S32 type) ;
	void deletePool(LLPrivateMemoryPool* pool) ;
private:
	std::vector<LLPrivateMemoryPool*> mPoolList ;
	U32  mMaxPrivatePoolSize;
	static LLPrivateMemoryPoolManager* sInstance ;
	static BOOL sPrivatePoolEnabled;
	static std::vector<LLPrivateMemoryPool*> sDanglingPoolList ;
public:
	void updateStatistics() ;
	U32 mTotalReservedSize ;
	U32 mTotalAllocatedSize ;
public:
#if __DEBUG_PRIVATE_MEM__
	static char* allocate(LLPrivateMemoryPool* poolp, U32 size, const char* function, const int line) ;
	typedef std::map<char*, std::string> mem_allocation_info_t ;
	static mem_allocation_info_t sMemAllocationTracker;
#else
	static char* allocate(LLPrivateMemoryPool* poolp, U32 size) ;
#endif
	static void  freeMem(LLPrivateMemoryPool* poolp, void* addr) ;
};
#if __DEBUG_PRIVATE_MEM__
#define ALLOCATE_MEM(poolp, size) LLPrivateMemoryPoolManager::allocate((poolp), (size), __FUNCTION__, __LINE__)
#else
#define ALLOCATE_MEM(poolp, size) LLPrivateMemoryPoolManager::allocate((poolp), (size))
#endif
#define FREE_MEM(poolp, addr) LLPrivateMemoryPoolManager::freeMem((poolp), (addr))
#if 0
class LL_COMMON_API LLPrivateMemoryPoolTester
{
private:
	LLPrivateMemoryPoolTester() ;
	~LLPrivateMemoryPoolTester() ;
public:
	static LLPrivateMemoryPoolTester* getInstance() ;
	static void destroy() ;
	void run(S32 type) ;
private:
	void correctnessTest() ;
	void performanceTest() ;
	void fragmentationtest() ;
	void test(U32 min_size, U32 max_size, U32 stride, U32 times, bool random_deletion, bool output_statistics) ;
	void testAndTime(U32 size, U32 times) ;
#if 0
public:
	void* operator new(size_t size)
	{
		return (void*)sPool->allocate(size) ;
	}
    void  operator delete(void* addr)
	{
		sPool->freeMem(addr) ;
	}
	void* operator new[](size_t size)
	{
		return (void*)sPool->allocate(size) ;
	}
    void  operator delete[](void* addr)
	{
		sPool->freeMem(addr) ;
	}
#endif
private:
	static LLPrivateMemoryPoolTester* sInstance;
	static LLPrivateMemoryPool* sPool ;
	static LLPrivateMemoryPool* sThreadedPool ;
};
#if 0
void* LLPrivateMemoryPoolTester::operator new(size_t size)
{
	return (void*)sPool->allocate(size) ;
}
void  LLPrivateMemoryPoolTester::operator delete(void* addr)
{
	sPool->free(addr) ;
}
void* LLPrivateMemoryPoolTester::operator new[](size_t size)
{
	return (void*)sPool->allocate(size) ;
}
void  LLPrivateMemoryPoolTester::operator delete[](void* addr)
{
	sPool->free(addr) ;
}
#endif
#endif
#include "llpointer.h"
#include "llsingleton.h"
#include "llsafehandle.h"
#endif
