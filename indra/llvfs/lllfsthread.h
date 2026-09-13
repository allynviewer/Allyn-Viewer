/** 
 * @file lllfsthread.h
 * @brief LLLFSThread base class
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_LLLFSTHREAD_H
#define LL_LLLFSTHREAD_H
#include <queue>
#include <string>
#include <map>
#include <set>
#include "llapr.h"
#include "llpointer.h"
#include "llqueuedthread.h"
class LLLFSThread : public LLQueuedThread
{
public:
	enum operation_t {
		FILE_READ,
		FILE_WRITE,
		FILE_RENAME,
		FILE_REMOVE
	};
public:
	class Responder : public LLThreadSafeRefCount
	{
	protected:
		~Responder();
	public:
		virtual void completed(S32 bytes) = 0;
	};
	class Request : public QueuedRequest
	{
	protected:
		virtual ~Request();
	public:
		Request(LLLFSThread* thread,
				handle_t handle, U32 priority,
				operation_t op, const std::string& filename,
				U8* buffer, S32 offset, S32 numbytes,
				Responder* responder);
		S32 getBytes()
		{
			return mBytes;
		}
		S32 getBytesRead()
		{
			return mBytesRead;
		}
		S32 getOperation()
		{
			return mOperation;
		}
		U8* getBuffer()
		{
			return mBuffer;
		}
		const std::string& getFilename()
		{
			return mFileName;
		}
		bool processRequest();
		void finishRequest(bool completed);
		void deleteRequest();
	private:
		LLLFSThread* mThread;
		operation_t mOperation;
		std::string mFileName;
		U8* mBuffer;
		S32 mOffset;
		S32 mBytes;
		S32 mBytesRead;
		LLPointer<Responder> mResponder;
	};
public:
	LLLFSThread(bool threaded = TRUE);
	~LLLFSThread();
	handle_t read(const std::string& filename,
				  U8* buffer, S32 offset, S32 numbytes,
				  Responder* responder, U32 pri=0);
	handle_t write(const std::string& filename,
				   U8* buffer, S32 offset, S32 numbytes,
				   Responder* responder, U32 pri=0);
	U32 priorityCounter() { return mPriorityCounter-- & PRIORITY_LOWBITS; }
	static void initClass(bool local_is_threaded = TRUE);
	static S32 updateClass(U32 ms_elapsed);
	static void cleanupClass();
private:
	U32 mPriorityCounter;
public:
	static LLLFSThread* sLocal;
};
#endif
