/** 
 * @file llimageworker.cpp
 * @brief Base class for images.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llimageworker.h"
#include "llimagedxt.h"
#include "lltimer.h"
#include <string>
LLImageDecodeThread::DecodeWorker::DecodeWorker(LLImageDecodeThread* owner, const std::string& name)
	: LLThread(name),
	  mOwner(owner)
{
	start();
}
void LLImageDecodeThread::DecodeWorker::run()
{
	while (!isQuitting())
	{
		if (mOwner->isQuitting())
		{
			break;
		}
		if (mOwner->isPaused())
		{
			ms_sleep(2);
			continue;
		}
		S32 pending = mOwner->decodeOne();
		if (pending == 0)
		{
			ms_sleep(1);
		}
	}
}
LLImageDecodeThread::LLImageDecodeThread(bool threaded, S32 extra_workers)
	: LLQueuedThread("imagedecode", threaded)
{
	mCreationMutex = new LLMutex();
	if (threaded && extra_workers > 0)
	{
		mWorkers.reserve(extra_workers);
		for (S32 i = 0; i < extra_workers; ++i)
		{
			mWorkers.push_back(new DecodeWorker(this, std::string("imagedecode-") + std::to_string(i + 1)));
		}
		LL_INFOS() << "Image decode pool: 1 queue thread + " << extra_workers
				   << " extra workers" << LL_ENDL;
	}
}
void LLImageDecodeThread::shutdown()
{
	for (DecodeWorker* worker : mWorkers)
	{
		worker->setQuitting();
	}
	LLQueuedThread::shutdown();
	for (DecodeWorker* worker : mWorkers)
	{
		S32 timeout = 50;
		while (!worker->isStopped() && timeout-- > 0)
		{
			ms_sleep(100);
		}
		delete worker;
	}
	mWorkers.clear();
}
LLImageDecodeThread::~LLImageDecodeThread()
{
	if (!mWorkers.empty())
	{
		shutdown();
	}
	delete mCreationMutex ;
}
S32 LLImageDecodeThread::update(F32 max_time_ms)
{
	LLMutexLock lock(mCreationMutex);
	for (creation_list_t::iterator iter = mCreationList.begin();
		 iter != mCreationList.end(); ++iter)
	{
		creation_info& info = *iter;
		ImageRequest* req = new ImageRequest(info.handle, info.image,
						     info.priority, info.discard, info.needs_aux,
						     info.responder);
		bool res = addRequest(req);
		if (!res)
		{
			LL_ERRS() << "request added after LLLFSThread::cleanupClass()" << LL_ENDL;
		}
	}
	mCreationList.clear();
	S32 res = LLQueuedThread::update(max_time_ms);
	return res;
}
LLImageDecodeThread::handle_t LLImageDecodeThread::decodeImage(LLImageFormatted* image,
	U32 priority, S32 discard, BOOL needs_aux, Responder* responder)
{
	LLMutexLock lock(mCreationMutex);
	handle_t handle = generateHandle();
	mCreationList.push_back(creation_info(handle, image, priority, discard, needs_aux, responder));
	return handle;
}
S32 LLImageDecodeThread::tut_size()
{
	LLMutexLock lock(mCreationMutex);
	S32 res = mCreationList.size();
	return res;
}
LLImageDecodeThread::Responder::~Responder()
{
}
LLImageDecodeThread::ImageRequest::ImageRequest(handle_t handle, LLImageFormatted* image,
												U32 priority, S32 discard, BOOL needs_aux,
												LLImageDecodeThread::Responder* responder)
	: LLQueuedThread::QueuedRequest(handle, priority, FLAG_AUTO_COMPLETE),
	  mFormattedImage(image),
	  mDiscardLevel(discard),
	  mNeedsAux(needs_aux),
	  mDecodedRaw(FALSE),
	  mDecodedAux(FALSE),
	  mResponder(responder)
{
}
LLImageDecodeThread::ImageRequest::~ImageRequest()
{
	mDecodedImageRaw = NULL;
	mDecodedImageAux = NULL;
	mFormattedImage = NULL;
}
bool LLImageDecodeThread::ImageRequest::processRequest()
{
	const F32 decode_time_slice = 0.f;
	bool done = true;
	if (!mDecodedRaw && mFormattedImage.notNull())
	{
		if (mDecodedImageRaw.isNull())
		{
			if (!mFormattedImage->updateData())
			{
				return true;
			}
			if (!(mFormattedImage->getWidth() * mFormattedImage->getHeight() * mFormattedImage->getComponents()))
			{
				return true;
			}
			if (mDiscardLevel >= 0)
			{
				mFormattedImage->setDiscardLevel(mDiscardLevel);
			}
			mDecodedImageRaw = new LLImageRaw(mFormattedImage->getWidth(),
											  mFormattedImage->getHeight(),
											  mFormattedImage->getComponents());
			if (mDecodedImageRaw.isNull() || !mDecodedImageRaw->getData())
			{
				mDecodedImageRaw = NULL;
				return true;
			}
		}
		done = mFormattedImage->decode(mDecodedImageRaw, decode_time_slice);
		mDecodedRaw = done;
	}
	if (done && mNeedsAux && !mDecodedAux && mFormattedImage.notNull())
	{
		if (!mDecodedImageAux)
		{
			mDecodedImageAux = new LLImageRaw(mFormattedImage->getWidth(),
											  mFormattedImage->getHeight(),
											  1);
		}
		done = mFormattedImage->decodeChannels(mDecodedImageAux, decode_time_slice, 4, 4);
		mDecodedAux = done;
	}
	return done;
}
void LLImageDecodeThread::ImageRequest::finishRequest(bool completed)
{
	if (mResponder.notNull())
	{
		bool success = completed && mDecodedRaw && mDecodedImageRaw.notNull()
			&& mDecodedImageRaw->getDataSize() && (!mNeedsAux || mDecodedAux);
		mResponder->completed(success, mDecodedImageRaw, mDecodedImageAux);
	}
}
bool LLImageDecodeThread::ImageRequest::tut_isOK()
{
	return mResponder.notNull();
}
