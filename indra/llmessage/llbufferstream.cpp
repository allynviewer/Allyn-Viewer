/** 
 * @file llbufferstream.cpp
 * @author Phoenix
 * @date 2005-10-10
 * @brief Implementation of the buffer iostream classes
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
#include "linden_common.h"
#include "llbufferstream.h"
#include "llbuffer.h"
#include "llthread.h"
static const S32 DEFAULT_OUTPUT_SEGMENT_SIZE = 1024 * 4;
LLBufferStreamBuf::LLBufferStreamBuf(
	const LLChannelDescriptors& channels,
	LLBufferArray* buffer) :
	mChannels(channels),
	mBuffer(buffer)
{
}
LLBufferStreamBuf::~LLBufferStreamBuf()
{
	sync();
}
int LLBufferStreamBuf::underflow()
{
	if(!mBuffer)
	{
		return EOF;
	}
	LLMutexLock lock(mBuffer->getMutex());
	LLBufferArray::segment_iterator_t iter;
	LLBufferArray::segment_iterator_t end = mBuffer->endSegment();
	U8* last_pos = (U8*)gptr();
	LLSegment segment;
	if(last_pos)
	{
		--last_pos;
		iter = mBuffer->splitAfter(last_pos);
		if(iter != end)
		{
			mBuffer->eraseSegment(iter++);
			if(iter != end) segment = (*iter);
		}
		else
		{
			return EOF;
		}
	}
	else
	{
		iter = mBuffer->constructSegmentAfter(last_pos, segment);
	}
	if(iter == end)
	{
		return EOF;
	}
	while((!segment.isOnChannel(mChannels.in()) || (segment.size() == 0)))
	{
		++iter;
		if(iter == end)
		{
			return EOF;
		}
		segment = *(iter);
	}
	char* start = (char*)segment.data();
	setg(start, start, start + segment.size());
	return *gptr();
}
int LLBufferStreamBuf::overflow(int c)
{
	if(!mBuffer)
	{
		return EOF;
	}
	if(EOF == c)
	{
		if(0 == sync())
		{
			return 1;
		}
		else
		{
			return EOF;
		}
	}
	LLBufferArray::segment_iterator_t it;
	LLMutexLock lock(mBuffer->getMutex());
	it = mBuffer->makeSegment(mChannels.out(), DEFAULT_OUTPUT_SEGMENT_SIZE);
	if(it != mBuffer->endSegment())
	{
		char* start = (char*)(*it).data();
		(*start) = (char)(c);
		setp(start + 1, start + (*it).size());
		return c;
	}
	else
	{
		return EOF;
	}
}
int LLBufferStreamBuf::sync()
{
	int return_value = -1;
	if(!mBuffer)
	{
		return return_value;
	}
	U8* address = (U8*)pptr();
	setp(NULL, NULL);
	LLMutexLock lock(mBuffer->getMutex());
	address = mBuffer->seek(mChannels.out(), address, -1);
	if(address)
	{
		LLBufferArray::segment_iterator_t it;
		it = mBuffer->splitAfter(address);
		LLBufferArray::segment_iterator_t end = mBuffer->endSegment();
		if(it != end)
		{
			++it;
			if(it != end)
			{
				mBuffer->eraseSegment(it);
			}
			return_value = 0;
		}
	}
	else
	{
		return_value = 0;
	}
	return return_value;
}
LLBufferStreamBuf::pos_type LLBufferStreamBuf::seekoff(
	LLBufferStreamBuf::off_type off,
	std::ios::seekdir way,
	std::ios::openmode which)
{
	if(!mBuffer
	   || ((way == std::ios::beg) && (off < 0))
	   || ((way == std::ios::end) && (off > 0)))
	{
		return -1;
	}
	U8* address = NULL;
	if(which & std::ios::in)
	{
		U8* base_addr = NULL;
		switch(way)
		{
		case std::ios::end:
			base_addr = (U8*)LLBufferArray::npos;
			break;
		case std::ios::cur:
			base_addr = (U8*)gptr();
			break;
		case std::ios::beg:
		default:
			break;
		}
		LLMutexLock lock(mBuffer->getMutex());
		address = mBuffer->seek(mChannels.in(), base_addr, off);
		if(address)
		{
			LLBufferArray::segment_iterator_t iter;
			iter = mBuffer->getSegment(address);
			char* start = (char*)(*iter).data();
			setg(start, (char*)address, start + (*iter).size());
		}
		else
		{
			address = (U8*)(-1);
		}
	}
	if(which & std::ios::out)
	{
		U8* base_addr = NULL;
		switch(way)
		{
		case std::ios::end:
			base_addr = (U8*)LLBufferArray::npos;
			break;
		case std::ios::cur:
			base_addr = (U8*)pptr();
			break;
		case std::ios::beg:
		default:
			break;
		}
		LLMutexLock lock(mBuffer->getMutex());
		address = mBuffer->seek(mChannels.out(), base_addr, off);
		if(address)
		{
			LLBufferArray::segment_iterator_t iter;
			iter = mBuffer->getSegment(address);
			setp((char*)address, (char*)(*iter).data() + (*iter).size());
		}
		else
		{
			address = (U8*)(-1);
		}
	}
	S32 rv = (S32)(intptr_t)address;
	return (pos_type)rv;
}
LLBufferStream::LLBufferStream(
	const LLChannelDescriptors& channels,
	LLBufferArray* buffer) :
	std::iostream(&mStreamBuf),
	mStreamBuf(channels, buffer)
{
}
LLBufferStream::~LLBufferStream()
{
}
