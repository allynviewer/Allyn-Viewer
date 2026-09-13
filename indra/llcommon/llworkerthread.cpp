/** 
 * @file llworkerthread.cpp
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llworkerthread.h"
#include "llstl.h"
#if USE_FRAME_CALLBACK_MANAGER
#include "llframecallbackmanager.h"
#endif
LLWorkerThread::LLWorkerThread(const std::string& name, bool threaded, bool should_pause) :
	LLQueuedThread(name, threaded, should_pause)
{
	mDeleteMutex = new LLMutex();
}
LLWorkerThread::~LLWorkerThread()
{
	if (!mDeleteList.empty())
	{
		LL_WARNS() << "Worker Thread: " << mName << " destroyed with " << mDeleteList.size()
				<< " entries in delete list." << LL_ENDL;
	}
	delete mDeleteMutex;
}
void LLWorkerThread::clearDeleteList()
{
	if (!mDeleteList.empty())
	{
		LL_WARNS() << "Worker Thread: " << mName << " destroyed with " << mDeleteList.size()
				<< " entries in delete list." << LL_ENDL;
		mDeleteMutex->lock();
		for (delete_list_t::iterator iter = mDeleteList.begin(); iter != mDeleteList.end(); ++iter)
		{
			(*iter)->mRequestHandle = LLWorkerThread::nullHandle();
			(*iter)->clearFlags(LLWorkerClass::WCF_HAVE_WORK);
			delete *iter ;
		}
		mDeleteList.clear() ;
		mDeleteMutex->unlock() ;
	}
}
S32 LLWorkerThread::update(F32 max_time_ms)
{
	S32 res = LLQueuedThread::update(max_time_ms);
	std::vector<LLWorkerClass*> delete_list;
	std::vector<LLWorkerClass*> abort_list;
	mDeleteMutex->lock();
	for (delete_list_t::iterator iter = mDeleteList.begin();
		 iter != mDeleteList.end(); )
	{
		delete_list_t::iterator curiter = iter++;
		LLWorkerClass* worker = *curiter;
		if (worker->deleteOK())
		{
			if (worker->getFlags(LLWorkerClass::WCF_WORK_FINISHED))
			{
				delete_list.push_back(worker);
				mDeleteList.erase(curiter);
			}
			else if (!worker->getFlags(LLWorkerClass::WCF_ABORT_REQUESTED))
			{
				abort_list.push_back(worker);
			}
		}
	}
	mDeleteMutex->unlock();
	for (std::vector<LLWorkerClass*>::iterator iter = abort_list.begin();
		 iter != abort_list.end(); ++iter)
	{
		(*iter)->abortWork(false);
	}
	for (std::vector<LLWorkerClass*>::iterator iter = delete_list.begin();
		 iter != delete_list.end(); ++iter)
	{
		LLWorkerClass* worker = *iter;
		if (worker->mRequestHandle)
		{
			completeRequest(worker->mRequestHandle);
			worker->mRequestHandle = LLWorkerThread::nullHandle();
			worker->clearFlags(LLWorkerClass::WCF_HAVE_WORK);
		}
		delete *iter;
	}
	res += delete_list.size() + abort_list.size();
	return res;
}
LLWorkerThread::handle_t LLWorkerThread::addWorkRequest(LLWorkerClass* workerclass, S32 param, U32 priority)
{
	handle_t handle = generateHandle();
	WorkRequest* req = new WorkRequest(handle, priority, workerclass, param);
	bool res = addRequest(req);
	if (!res)
	{
		LL_ERRS() << "add called after LLWorkerThread::cleanupClass()" << LL_ENDL;
		req->deleteRequest();
		handle = nullHandle();
	}
	return handle;
}
void LLWorkerThread::deleteWorker(LLWorkerClass* workerclass)
{
	mDeleteMutex->lock();
	mDeleteList.push_back(workerclass);
	mDeleteMutex->unlock();
}
LLWorkerThread::WorkRequest::WorkRequest(handle_t handle, U32 priority, LLWorkerClass* workerclass, S32 param) :
	LLQueuedThread::QueuedRequest(handle, priority),
	mWorkerClass(workerclass),
	mParam(param)
{
}
LLWorkerThread::WorkRequest::~WorkRequest()
{
}
void LLWorkerThread::WorkRequest::deleteRequest()
{
	LLQueuedThread::QueuedRequest::deleteRequest();
}
bool LLWorkerThread::WorkRequest::processRequest()
{
	LLWorkerClass* workerclass = getWorkerClass();
	workerclass->setWorking(true);
	bool complete = workerclass->doWork(getParam());
	workerclass->setWorking(false);
	return complete;
}
void LLWorkerThread::WorkRequest::finishRequest(bool completed)
{
	LLWorkerClass* workerclass = getWorkerClass();
	workerclass->finishWork(getParam(), completed);
	U32 flags = LLWorkerClass::WCF_WORK_FINISHED | (completed ? 0 : LLWorkerClass::WCF_WORK_ABORTED);
	workerclass->setFlags(flags);
}
LLWorkerClass::LLWorkerClass(LLWorkerThread* workerthread, const std::string& name)
	: mWorkerThread(workerthread),
	  mWorkerClassName(name),
	  mRequestHandle(LLWorkerThread::nullHandle()),
	  mRequestPriority(LLWorkerThread::PRIORITY_NORMAL),
	  mMutex(),
	  mWorkFlags(0)
{
	if (!mWorkerThread)
	{
		LL_ERRS() << "LLWorkerClass() called with NULL workerthread: " << name << LL_ENDL;
	}
}
LLWorkerClass::~LLWorkerClass()
{
	llassert_always(!(mWorkFlags & WCF_WORKING));
	llassert_always(mWorkFlags & WCF_DELETE_REQUESTED);
	llassert_always(!mMutex.isLocked());
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		LLWorkerThread::WorkRequest* workreq = (LLWorkerThread::WorkRequest*)mWorkerThread->getRequest(mRequestHandle);
		if (!workreq)
		{
			LL_ERRS() << "LLWorkerClass destroyed with stale work handle" << LL_ENDL;
		}
		if (workreq->getStatus() != LLWorkerThread::STATUS_ABORTED &&
			workreq->getStatus() != LLWorkerThread::STATUS_COMPLETE &&
			!(workreq->getFlags() & LLWorkerThread::FLAG_LOCKED))
		{
			LL_ERRS() << "LLWorkerClass destroyed with active worker! Worker Status: " << workreq->getStatus() << LL_ENDL;
		}
	}
}
void LLWorkerClass::setWorkerThread(LLWorkerThread* workerthread)
{
	mMutex.lock();
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		LL_ERRS() << "LLWorkerClass attempt to change WorkerThread with active worker!" << LL_ENDL;
	}
	mWorkerThread = workerthread;
	mMutex.unlock();
}
void LLWorkerClass::finishWork(S32 param, bool success)
{
}
bool LLWorkerClass::deleteOK()
{
	return true;
}
void LLWorkerClass::setWorking(bool working)
{
	mMutex.lock();
	if (working)
	{
		llassert_always(!(mWorkFlags & WCF_WORKING));
		setFlags(WCF_WORKING);
	}
	else
	{
		llassert_always((mWorkFlags & WCF_WORKING));
		clearFlags(WCF_WORKING);
	}
	mMutex.unlock();
}
bool LLWorkerClass::yield()
{
	LLThread::yield();
	mWorkerThread->checkPause();
	bool res;
	mMutex.lock();
	res = (getFlags() & WCF_ABORT_REQUESTED) ? true : false;
	mMutex.unlock();
	return res;
}
void LLWorkerClass::addWork(S32 param, U32 priority)
{
	mMutex.lock();
	llassert_always(!(mWorkFlags & (WCF_WORKING|WCF_HAVE_WORK)));
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		LL_ERRS() << "LLWorkerClass attempt to add work with active worker!" << LL_ENDL;
	}
#if _DEBUG
#endif
	startWork(param);
	clearFlags(WCF_WORK_FINISHED|WCF_WORK_ABORTED);
	setFlags(WCF_HAVE_WORK);
	mRequestHandle = mWorkerThread->addWorkRequest(this, param, priority);
	mMutex.unlock();
}
void LLWorkerClass::abortWork(bool autocomplete)
{
	mMutex.lock();
#if _DEBUG
#endif
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		mWorkerThread->abortRequest(mRequestHandle, autocomplete);
		mWorkerThread->setPriority(mRequestHandle, LLQueuedThread::PRIORITY_IMMEDIATE);
		setFlags(WCF_ABORT_REQUESTED);
	}
	mMutex.unlock();
}
bool LLWorkerClass::checkWork(bool aborting)
{
	LLMutexLock lock(&mMutex);
	bool complete = false, abort = false;
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		LLWorkerThread::WorkRequest* workreq = (LLWorkerThread::WorkRequest*)mWorkerThread->getRequest(mRequestHandle);
		if(!workreq)
		{
			if(mWorkerThread->isQuitting() || mWorkerThread->isStopped())
			{
				mRequestHandle = LLWorkerThread::nullHandle();
				clearFlags(WCF_HAVE_WORK);
			}
			else
			{
				llassert_always(workreq);
			}
			return true ;
		}
		LLQueuedThread::status_t status = workreq->getStatus();
		if (status == LLWorkerThread::STATUS_ABORTED)
		{
			complete = !(workreq->getFlags() & LLWorkerThread::FLAG_LOCKED);
			abort = true;
		}
		else if (status == LLWorkerThread::STATUS_COMPLETE)
		{
			complete = !(workreq->getFlags() & LLWorkerThread::FLAG_LOCKED);
		}
		else
		{
			llassert_always(!aborting || (workreq->getFlags() & LLQueuedThread::FLAG_ABORT));
		}
		if (complete)
		{
			llassert_always(!(getFlags(WCF_WORKING)));
			endWork(workreq->getParam(), abort);
			mWorkerThread->completeRequest(mRequestHandle);
			mRequestHandle = LLWorkerThread::nullHandle();
			clearFlags(WCF_HAVE_WORK);
		}
	}
	else
	{
		complete = true;
	}
	return complete;
}
void LLWorkerClass::scheduleDelete()
{
	bool do_delete = false;
	mMutex.lock();
	if (!(getFlags(WCF_DELETE_REQUESTED)))
	{
		setFlags(WCF_DELETE_REQUESTED);
		do_delete = true;
	}
	mMutex.unlock();
	if (do_delete)
	{
		mWorkerThread->deleteWorker(this);
	}
}
void LLWorkerClass::setPriority(U32 priority)
{
	mMutex.lock();
	if (mRequestHandle != LLWorkerThread::nullHandle() && mRequestPriority != priority)
	{
		mRequestPriority = priority;
		mWorkerThread->setPriority(mRequestHandle, priority);
	}
	mMutex.unlock();
}
