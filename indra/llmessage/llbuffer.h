/** 
 * @file llbuffer.h
 * @author Phoenix
 * @date 2005-09-20
 * @brief Declaration of buffer and buffer arrays primarily used in I/O.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#ifndef LL_LLBUFFER_H
#define LL_LLBUFFER_H
#include <list>
#include <vector>
class LLMutex;
class LLChannelDescriptors
{
public:
	enum { E_CHANNEL_COUNT = 3 };
	LLChannelDescriptors() : mBaseChannel(0) {}
	explicit LLChannelDescriptors(S32 base) : mBaseChannel(base) {}
	S32 in() const { return mBaseChannel; }
	S32 out() const { return mBaseChannel + 1; }
protected:
	S32 mBaseChannel;
};
class LLSegment
{
public:
	LLSegment();
	LLSegment(S32 channel, U8* data, S32 data_len);
	~LLSegment();
	bool isOnChannel(S32 channel) const;
	S32 getChannel() const;
	void setChannel(S32 channel);
	U8* data() const;
	S32 size() const;
	bool operator==(const LLSegment& rhs) const;
protected:
	S32 mChannel;
	U8* mData;
	S32 mSize;
};
class LLBuffer
{
public:
	virtual ~LLBuffer() {}
	virtual bool createSegment(S32 channel, S32 size, LLSegment& segment) = 0;
	virtual bool reclaimSegment(const LLSegment& segment) = 0;
	virtual bool containsSegment(const LLSegment& segment) const = 0;
	virtual S32 capacity() const = 0;
};
class LLHeapBuffer : public LLBuffer
{
public:
	LLHeapBuffer();
	explicit LLHeapBuffer(S32 size);
	LLHeapBuffer(const U8* src, S32 len);
	virtual ~LLHeapBuffer();
	S32 bytesLeft() const;
	virtual bool createSegment(S32 channel, S32 size, LLSegment& segment);
	virtual bool reclaimSegment(const LLSegment& segment);
	virtual bool containsSegment(const LLSegment& segment) const;
	virtual S32 capacity() const { return mSize; }
protected:
	U8* mBuffer;
	S32 mSize;
	U8* mNextFree;
	S32 mReclaimedBytes;
private:
	void allocate(S32 size);
};
class LLBufferArray
{
public:
	typedef std::vector<LLBuffer*> buffer_list_t;
	typedef buffer_list_t::iterator buffer_iterator_t;
	typedef buffer_list_t::const_iterator const_buffer_iterator_t;
	typedef std::list<LLSegment> segment_list_t;
	typedef segment_list_t::const_iterator const_segment_iterator_t;
	typedef segment_list_t::iterator segment_iterator_t;
	enum { npos = 0xffffffff };
	LLBufferArray();
	~LLBufferArray();
	static LLChannelDescriptors makeChannelConsumer(
		const LLChannelDescriptors& channels);
	LLChannelDescriptors nextChannel();
	S32 capacity() const;
	bool append(S32 channel, const U8* src, S32 len);
	bool prepend(S32 channel, const U8* src, S32 len);
	bool insertAfter(
		segment_iterator_t segment,
		S32 channel,
		const U8* src,
		S32 len);
	S32 countAfter(S32 channel, U8* start) const;
	S32 count(S32 channel) const
	{
		return countAfter(channel, NULL);
	}
	U8* readAfter(S32 channel, U8* start, U8* dest, S32& len) const;
	U8* seek(S32 channel, U8* start, S32 delta) const;
	bool takeContents(LLBufferArray& source);
	segment_iterator_t splitAfter(U8* address);
	segment_iterator_t beginSegment();
	const_segment_iterator_t beginSegment() const;
	segment_iterator_t endSegment();
	const_segment_iterator_t endSegment() const;
	const_segment_iterator_t getSegment(U8* address) const;
	segment_iterator_t getSegment(U8* address);
	segment_iterator_t constructSegmentAfter(U8* address, LLSegment& segment);
	segment_iterator_t makeSegment(S32 channel, S32 length);
	bool eraseSegment(const segment_iterator_t& iter);
	void lock();
	void unlock();
	LLMutex* getMutex();
	void setThreaded(bool threaded);
	void writeChannelTo(std::ostream& ostr, S32 channel) const;
protected:
	bool copyIntoBuffers(
		S32 channel,
		const U8* src,
		S32 len,
		std::vector<LLSegment>& segments);
protected:
	S32 mNextBaseChannel;
	buffer_list_t mBuffers;
	segment_list_t mSegments;
	LLMutex* mMutexp;
};
#endif
