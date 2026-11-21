/** 
 * @file llimageworker.h
 * @brief Object for managing images and their textures.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Modifications Copyright (C) 2026 - Allyn Viewer
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
#ifndef LL_LLIMAGEWORKER_H
#define LL_LLIMAGEWORKER_H
#include "llimage.h"
#include "llimagebc7.h"
#include "llpointer.h"
#include "lluuid.h"
#include "llthread.h"
#include "llworkerthread.h"
#include <vector>
class LLImageDecodeThread : public LLQueuedThread
{
public:
	class Responder : public LLThreadSafeRefCount
	{
	protected:
		virtual ~Responder();
	public:
		virtual void completed(bool success, LLImageRaw* raw, LLImageRaw* aux) = 0;
	};
	class ImageRequest : public LLQueuedThread::QueuedRequest
	{
	protected:
		virtual ~ImageRequest();
	public:
		ImageRequest(handle_t handle, LLImageFormatted* image,
					 U32 priority, S32 discard, BOOL needs_aux,
					 LLImageDecodeThread::Responder* responder,
					 const LLUUID& id = LLUUID::null, bool transcoded_ok = false);
		bool processRequest();
		void finishRequest(bool completed);
		bool tut_isOK();
	private:
		LLPointer<LLImageFormatted> mFormattedImage;
		S32 mDiscardLevel;
		BOOL mNeedsAux;
		LLPointer<LLImageRaw> mDecodedImageRaw;
		LLPointer<LLImageRaw> mDecodedImageAux;
		BOOL mDecodedRaw;
		BOOL mDecodedAux;
		LLPointer<LLImageDecodeThread::Responder> mResponder;
		LLUUID mID;
		bool mTranscodedOK;
		LLImageBC7Cache::EState mBC7State;
		LLImageBC7ChainPtr mBC7Chain;
		bool readBC7();
		void attachBC7();
	};
public:
	LLImageDecodeThread(bool threaded = true, S32 extra_workers = 0);
	virtual ~LLImageDecodeThread();
	void shutdown();
	handle_t decodeImage(LLImageFormatted* image,
						 U32 priority, S32 discard, BOOL needs_aux,
						 Responder* responder,
						 const LLUUID& id = LLUUID::null, bool transcoded_ok = false);
	S32 update(F32 max_time_ms);
	S32 tut_size();
	S32 decodeOne() { return processNextRequest(); }
	S32 getDecodeSlots() const;
private:
	class DecodeWorker : public LLThread
	{
	public:
		DecodeWorker(LLImageDecodeThread* owner, const std::string& name);
		void run();
	private:
		LLImageDecodeThread* mOwner;
	};
	std::vector<DecodeWorker*> mWorkers;
	struct creation_info
	{
		handle_t handle;
		LLPointer<LLImageFormatted> image;
		U32 priority;
		S32 discard;
		BOOL needs_aux;
		LLPointer<Responder> responder;
		LLUUID id;
		bool transcoded_ok;
		creation_info(handle_t h, LLImageFormatted* i, U32 p, S32 d, BOOL aux, Responder* r, const LLUUID& u, bool t)
			: handle(h), image(i), priority(p), discard(d), needs_aux(aux), responder(r), id(u), transcoded_ok(t)
		{}
	};
	typedef std::list<creation_info> creation_list_t;
	creation_list_t mCreationList;
	LLMutex* mCreationMutex;
};
#endif
