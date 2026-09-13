/** 
 * @file lliobuffer.h
 * @author Phoenix
 * @date 2005-05-04
 * @brief Declaration of buffers for use in IO Pipes.
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
#ifndef LL_LLIOBUFFER_H
#define LL_LLIOBUFFER_H
#include "lliopipe.h"
class LLIOBuffer : public LLIOPipe
{
public:
	LLIOBuffer();
	virtual ~LLIOBuffer();
	U8* data() const;
	S64 size() const;
	U8* current() const;
	S64 bytesLeft() const;
	void clear();
	enum EHead
	{
		READ,
		WRITE
	};
	EStatus seek(EHead head, S64 delta);
public:
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
	U8* mBuffer;
	S64 mBufferSize;
	U8* mReadHead;
	U8* mWriteHead;
};
#endif
