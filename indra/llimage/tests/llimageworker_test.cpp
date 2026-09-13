/** 
 * @file llimageworker_test.cpp
 * @author Merov Linden
 * @date 2009-04-28
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "../llcommon/linden_common.h"
#include <list>
#include <map>
#include <algorithm>
#include "../llimageworker.h"
#include "../llcommon/lltimer.h"
#include "../test/lltut.h"
LLImageBase::LLImageBase() {}
LLImageBase::~LLImageBase() {}
void LLImageBase::dump() { }
void LLImageBase::sanityCheck() { }
void LLImageBase::deleteData() { }
U8* LLImageBase::allocateData(S32 size) { return NULL; }
U8* LLImageBase::reallocateData(S32 size) { return NULL; }
LLImageRaw::LLImageRaw(U16 width, U16 height, S8 components) { }
LLImageRaw::~LLImageRaw() { }
void LLImageRaw::deleteData() { }
U8* LLImageRaw::allocateData(S32 size) { return NULL; }
U8* LLImageRaw::reallocateData(S32 size) { return NULL; }
namespace tut
{
	class responder_test : public LLImageDecodeThread::Responder
	{
		public:
			responder_test(bool* res)
			{
				done = res;
				*done = false;
			}
			virtual void completed(bool success, LLImageRaw* raw, LLImageRaw* aux)
			{
				*done = true;
			}
		private:
			bool* done;
	};
	struct imagedecodethread_test
	{
		LLImageDecodeThread* mThread;
		imagedecodethread_test()
		{
			mThread = NULL;
		}
		~imagedecodethread_test()
		{
			delete mThread;
		}
	};
	struct imagerequest_test
	{
		LLImageDecodeThread::ImageRequest* mRequest;
		bool done;
		imagerequest_test()
		{
			done = false;
			mRequest = new LLImageDecodeThread::ImageRequest(0, 0,
											 LLQueuedThread::PRIORITY_NORMAL, 0, FALSE,
											 new responder_test(&done));
		}
		~imagerequest_test()
		{
		}
	};
	typedef test_group<imagedecodethread_test> imagedecodethread_t;
	typedef imagedecodethread_t::object imagedecodethread_object_t;
	tut::imagedecodethread_t tut_imagedecodethread("imagedecodethread");
	typedef test_group<imagerequest_test> imagerequest_t;
	typedef imagerequest_t::object imagerequest_object_t;
	tut::imagerequest_t tut_imagerequest("imagerequest");
	template<> template<>
	void imagedecodethread_object_t::test<1>()
	{
		mThread = new LLImageDecodeThread(false);
		ensure("LLImageDecodeThread: non threaded constructor failed", mThread != NULL);
		ensure("LLImageDecodeThread: non threaded init state incorrect", mThread->tut_size() == 0);
		bool done = false;
		LLImageDecodeThread::handle_t decodeHandle = mThread->decodeImage(NULL, LLQueuedThread::PRIORITY_NORMAL, 0, FALSE, new responder_test(&done));
		ensure("LLImageDecodeThread: non threaded decodeImage(), returned handle is null", decodeHandle != 0);
		ensure("LLImageDecodeThread: non threaded decodeImage() insertion in threaded list failed", mThread->tut_size() == 1);
		S32 res = mThread->update(0);
		ensure("LLImageDecodeThread: non threaded update() list handling test failed", res == 0);
		ensure("LLImageDecodeThread: non threaded update() list emptying test failed", mThread->tut_size() == 0);
	}
	template<> template<>
	void imagedecodethread_object_t::test<2>()
	{
		mThread = new LLImageDecodeThread(true);
		ensure("LLImageDecodeThread: threaded constructor failed", mThread != NULL);
		ensure("LLImageDecodeThread: threaded init state incorrect", mThread->tut_size() == 0);
		bool done = false;
		LLImageDecodeThread::handle_t decodeHandle = mThread->decodeImage(NULL, LLQueuedThread::PRIORITY_NORMAL, 0, FALSE, new responder_test(&done));
		ensure("LLImageDecodeThread:  threaded decodeImage(), returned handle is null", decodeHandle != 0);
		ms_sleep(500);
		ensure("LLImageDecodeThread: responder creation failed", done == false);
		mThread->update(1);
		const U32 INCREMENT_TIME = 500;
		const U32 MAX_TIME = 20 * INCREMENT_TIME;
		U32 total_time = 0;
		while ((done == false) && (total_time < MAX_TIME))
		{
			ms_sleep(INCREMENT_TIME);
			total_time += INCREMENT_TIME;
		}
		ensure("LLImageDecodeThread: threaded work unit not processed", done == true);
	}
	template<> template<>
	void imagerequest_object_t::test<1>()
	{
		ensure("LLImageDecodeThread::ImageRequest::ImageRequest() constructor test failed", mRequest->tut_isOK());
		bool res = mRequest->processRequest();
		ensure("LLImageDecodeThread::ImageRequest::processRequest() processing request test failed", res == true);
		try {
			mRequest->finishRequest(false);
		} catch (...) {
			fail("LLImageDecodeThread::ImageRequest::finishRequest() test failed");
		}
	}
}
