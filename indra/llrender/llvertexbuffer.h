/** 
 * @file llvertexbuffer.h
 * @brief LLVertexBuffer wrapper for OpengGL vertex buffer objects
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#ifndef LL_LLVERTEXBUFFER_H
#define LL_LLVERTEXBUFFER_H
#include "llgl.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "v4coloru.h"
#include "llstrider.h"
#include "llrender.h"
#include <set>
#include <vector>
#include <list>
#define LL_MAX_VERTEX_ATTRIB_LOCATION 64
class LLVBOPool
{
public:
	static U64 sBytesPooled;
	static U64 sIndexBytesPooled;
	static std::vector<U32> sPendingDeletions;
	static void deleteReleasedBuffers();
	LLVBOPool(U32 vboUsage, U32 vboType);
	const U32 mUsage;
	const U32 mType;
	volatile U8* allocate(U32& name, U32 size, U32 seed = 0);
	void release(U32 name, volatile U8* buffer, U32 size);
	void seedPool();
	void cleanup();
	U32 genBuffer();
	void deleteBuffer(U32 name);
	class Record
	{
	public:
		U32 mGLName;
		volatile U8* mClientData;
	};
	typedef std::list<Record> record_list_t;
	std::vector<record_list_t> mFreeList;
	std::vector<U32> mMissCount;
};
class LLVertexBuffer : public LLRefCount
{
public:
	class MappedRegion
	{
	public:
		S32 mType;
		U32 mOffset;
		U32 mLength;
		MappedRegion(S32 type, U32 offset, U32 length);
	};
	LLVertexBuffer(const LLVertexBuffer& rhs)
		: mUsage(rhs.mUsage)
	{
		*this = rhs;
	}
	const LLVertexBuffer& operator=(const LLVertexBuffer& rhs)
	{
		LL_ERRS() << "Illegal operation!" << LL_ENDL;
		return *this;
	}
	static const std::string& getTypeName(U8 i);
	static LLVBOPool sStreamVBOPool;
	static LLVBOPool sDynamicVBOPool;
	static LLVBOPool sStreamIBOPool;
	static LLVBOPool sDynamicIBOPool;
	static std::vector<U32> sAvailableVAOName;
	static U32 sCurVAOName;
	static bool	sUseStreamDraw;
	static bool sUseVAO;
	static bool	sPreferStreamDraw;
	static void seedPools();
	static U32 getVAOName();
	static void releaseVAOName(U32 name);
	static void initClass(bool use_vbo, bool no_vbo_mapping);
	static void cleanupClass();
	static void setupClientArrays(U32 data_mask);
	static void drawArrays(U32 mode, const std::vector<LLVector3>& pos, const std::vector<LLVector3>& norm);
	static void drawElements(U32 mode, const S32 num_vertices, const LLVector4a* pos, const LLVector2* tc, S32 num_indices, const U16* indicesp);
 	static void unbind();
	static S32 calcVertexSize(const U32& typemask);
	static S32 calcOffsets(const U32& typemask, S32* offsets, S32 num_vertices);
	enum {
		TYPE_VERTEX = 0,
		TYPE_NORMAL,
		TYPE_TEXCOORD0,
		TYPE_TEXCOORD1,
		TYPE_TEXCOORD2,
		TYPE_TEXCOORD3,
		TYPE_COLOR,
		TYPE_EMISSIVE,
		TYPE_TANGENT,
		TYPE_WEIGHT,
		TYPE_WEIGHT4,
		TYPE_CLOTHWEIGHT,
		TYPE_TEXTURE_INDEX,
		TYPE_MAX,
		TYPE_INDEX,
	};
	enum {
		MAP_VERTEX = (1<<TYPE_VERTEX),
		MAP_NORMAL = (1<<TYPE_NORMAL),
		MAP_TEXCOORD0 = (1<<TYPE_TEXCOORD0),
		MAP_TEXCOORD1 = (1<<TYPE_TEXCOORD1),
		MAP_TEXCOORD2 = (1<<TYPE_TEXCOORD2),
		MAP_TEXCOORD3 = (1<<TYPE_TEXCOORD3),
		MAP_COLOR = (1<<TYPE_COLOR),
		MAP_EMISSIVE = (1<<TYPE_EMISSIVE),
		MAP_TANGENT = (1<<TYPE_TANGENT),
		MAP_WEIGHT = (1<<TYPE_WEIGHT),
		MAP_WEIGHT4 = (1<<TYPE_WEIGHT4),
		MAP_CLOTHWEIGHT = (1<<TYPE_CLOTHWEIGHT),
		MAP_TEXTURE_INDEX = (1<<TYPE_TEXTURE_INDEX),
	};
protected:
	friend class LLRender;
	virtual ~LLVertexBuffer();
	virtual void setupVertexBuffer(U32 data_mask);
	void setupVertexArray();
	void	genBuffer(U32 size);
	void	genIndices(U32 size);
	bool	bindGLBuffer(bool force_bind = false);
	bool	bindGLIndices(bool force_bind = false);
	bool	bindGLArray();
	void	releaseBuffer();
	void	releaseIndices();
	void	createGLBuffer(U32 size);
	void	createGLIndices(U32 size);
	void 	destroyGLBuffer();
	void 	destroyGLIndices();
	void	updateNumVerts(S32 nverts);
	void	updateNumIndices(S32 nindices);
	void	unmapBuffer();
public:
	LLVertexBuffer(U32 typemask, S32 usage);
	volatile U8*		mapVertexBuffer(S32 type, S32 index, S32 count, bool map_range);
	volatile U8*		mapIndexBuffer(S32 index, S32 count, bool map_range);
	virtual void	setBuffer(U32 data_mask);
	void flush();
	void	allocateBuffer(S32 nverts, S32 nindices, bool create);
	virtual void resizeBuffer(S32 newnverts, S32 newnindices);
	bool getVertexStrider(LLStrider<LLVector3>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getVertexStrider(LLStrider<LLVector4a>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getIndexStrider(LLStrider<U16>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTexCoord0Strider(LLStrider<LLVector2>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTexCoord1Strider(LLStrider<LLVector2>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTexCoord2Strider(LLStrider<LLVector2>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getNormalStrider(LLStrider<LLVector3>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getNormalStrider(LLStrider<LLVector4a>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTangentStrider(LLStrider<LLVector3>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTangentStrider(LLStrider<LLVector4a>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getColorStrider(LLStrider<LLColor4U>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getEmissiveStrider(LLStrider<LLColor4U>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getWeightStrider(LLStrider<F32>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getWeight4Strider(LLStrider<LLVector4a>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getClothWeightStrider(LLStrider<LLVector4a>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool	useVBOs() const;
	bool isEmpty() const					{ return mEmpty; }
	bool isLocked() const					{ return mVertexLocked || mIndexLocked; }
	S32 getNumVerts() const					{ return mNumVerts; }
	S32 getNumIndices() const				{ return mNumIndices; }
	volatile U8* getIndicesPointer() const			{ return useVBOs() ? (U8*) mAlignedIndexOffset : mMappedIndexData; }
	volatile U8* getVerticesPointer() const			{ return useVBOs() ? (U8*) mAlignedOffset : mMappedData; }
	U32 getTypeMask() const					{ return mTypeMask; }
	bool hasDataType(S32 type) const		{ return ((1 << type) & getTypeMask()); }
	S32 getSize() const;
	S32 getIndicesSize() const				{ return mIndicesSize; }
	volatile U8* getMappedData() const				{ return mMappedData; }
	volatile U8* getMappedIndices() const			{ return mMappedIndexData; }
	S32 getOffset(S32 type) const			{ return mOffsets[type]; }
	S32 getUsage() const					{ return mUsage; }
	bool isWriteable() const				{ return (mMappable || mUsage == GL_STREAM_DRAW_ARB) ? true : false; }
	void draw(U32 mode, U32 count, U32 indices_offset) const;
	void drawArrays(U32 mode, U32 offset, U32 count) const;
	void drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const;
	void validateRange(U32 start, U32 end, U32 count, U32 offset) const;
protected:
	S32		mNumVerts;
	S32		mNumIndices;
	ptrdiff_t mAlignedOffset;
	ptrdiff_t mAlignedIndexOffset;
	S32		mSize;
	U32		mResidentSize;
	S32		mIndicesSize;
	U32		mResidentIndicesSize;
	U32		mTypeMask;
	const S32		mUsage;
	U32		mGLBuffer;
	U32		mGLIndices;
	U32		mGLArray;
	volatile U8* mMappedData;
	volatile U8* mMappedIndexData;
	U32		mMappedDataUsingVBOs : 1;
	U32		mMappedIndexDataUsingVBOs : 1;
	U32		mVertexLocked : 1;
	U32		mIndexLocked : 1;
	U32		mFinal : 1;
	U32		mEmpty : 1;
	mutable bool	mMappable;
	S32		mOffsets[TYPE_MAX];
	std::vector<MappedRegion> mMappedVertexRegions;
	std::vector<MappedRegion> mMappedIndexRegions;
	mutable LLGLFence* mFence;
	void placeFence() const;
	void waitFence() const;
	static S32 determineUsage(S32 usage);
public:
	static S32 sCount;
	static S32 sGLCount;
	static S32 sMappedCount;
	static bool sMapped;
	typedef std::list<LLVertexBuffer*> buffer_list_t;
	static bool sDisableVBOMapping;
	static bool sEnableVBOs;
	static S32 sTypeSize[TYPE_MAX];
	static U32 sGLMode[LLRender::NUM_MODES];
	static U32 sGLRenderBuffer;
	static U32 sGLRenderArray;
	static U32 sGLRenderIndices;
	static bool sVBOActive;
	static bool sIBOActive;
	static U32 sLastMask;
	static U64 sAllocatedBytes;
	static U64 sAllocatedIndexBytes;
	static U32 sVertexCount;
	static U32 sIndexCount;
	static U32 sBindCount;
	static U32 sSetCount;
private:
	static LLVertexBuffer* sUtilityBuffer;
};
#endif
