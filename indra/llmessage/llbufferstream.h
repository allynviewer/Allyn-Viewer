/** 
 * @file llbufferstream.h
 * @author Phoenix
 * @date 2005-10-10
 * @brief Classes to treat an LLBufferArray as a c++ iostream.
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
#ifndef LL_LLBUFFERSTREAM_H
#define LL_LLBUFFERSTREAM_H
#include <iosfwd>
#include <iostream>
#include "llbuffer.h"
class LLBufferStreamBuf : public std::streambuf
{
public:
	LLBufferStreamBuf(
		const LLChannelDescriptors& channels,
		LLBufferArray* buffer);
	virtual ~LLBufferStreamBuf();
protected:
	typedef std::streambuf::pos_type pos_type;
	typedef std::streambuf::off_type off_type;
	virtual int underflow();
	virtual int overflow(int c);
	virtual int sync();
	virtual pos_type seekoff(
		off_type off,
		std::ios::seekdir way,
		std::ios::openmode which);
public:
	S32 count_in(void) const { return mBuffer->count(mChannels.in()); }
	S32 count_out(void) const { return mBuffer->count(mChannels.out()); }
protected:
	LLChannelDescriptors mChannels;
	LLBufferArray* mBuffer;
};
class LLBufferStream : public std::iostream
{
public:
	LLBufferStream(
		const LLChannelDescriptors& channels,
		LLBufferArray* buffer);
	~LLBufferStream();
	S32 count_in(void) const { return mStreamBuf.count_in(); }
	S32 count_out(void) const { return mStreamBuf.count_out(); }
protected:
	LLBufferStreamBuf mStreamBuf;
};
#endif
