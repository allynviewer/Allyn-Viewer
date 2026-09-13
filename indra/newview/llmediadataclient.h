/** 
 * @file llmediadataclient.h
 * @brief class for queueing up requests to the media service
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#ifndef LL_LLMEDIADATACLIENT_H
#define LL_LLMEDIADATACLIENT_H
#include "llhttpclient.h"
#include <set>
#include "llrefcount.h"
#include "llpointer.h"
#include "lleventtimer.h"
extern AIHTTPTimeoutPolicy mediaDataClientResponder_timeout;
class LLMediaDataClientObject : public LLRefCount
{
public:
	virtual U8 getMediaDataCount() const = 0;
	virtual LLSD getMediaDataLLSD(U8 index) const = 0;
	virtual bool isCurrentMediaUrl(U8 index, const std::string &url) const = 0;
	virtual LLUUID getID() const = 0;
	virtual void mediaNavigateBounceBack(U8 index) = 0;
	virtual bool hasMedia() const = 0;
	virtual void updateObjectMediaData(LLSD const &media_data_array, const std::string &version_string) = 0;
	virtual F64 getMediaInterest() const = 0;
	virtual std::string getCapabilityUrl(const std::string &name) const = 0;
	virtual bool isDead() const = 0;
	virtual U32 getMediaVersion() const = 0;
	virtual bool isInterestingEnough() const = 0;
	virtual bool isNew() const = 0;
	typedef LLPointer<LLMediaDataClientObject> ptr_t;
};
class LLMediaDataClient : public LLRefCount
{
public:
    LOG_CLASS(LLMediaDataClient);
    const static F32 QUEUE_TIMER_DELAY;
	const static F32 UNAVAILABLE_RETRY_TIMER_DELAY;
	const static U32 MAX_RETRIES;
	const static U32 MAX_SORTED_QUEUE_SIZE;
	const static U32 MAX_ROUND_ROBIN_QUEUE_SIZE;
	LLMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
					  F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
		              U32 max_retries = MAX_RETRIES,
					  U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
					  U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE);
	F32 getRetryTimerDelay() const { return mRetryTimerDelay; }
	virtual bool isEmpty() const;
	virtual bool isInQueue(const LLMediaDataClientObject::ptr_t &object);
	virtual void removeFromQueue(const LLMediaDataClientObject::ptr_t &object);
	virtual bool processQueueTimer();
protected:
	virtual ~LLMediaDataClient();
	class Responder;
	class Request : public LLRefCount
	{
	public:
		virtual LLSD getPayload() const = 0;
		virtual Responder *createResponder() = 0;
		virtual std::string getURL() { return ""; }
        enum Type {
            GET,
            UPDATE,
            NAVIGATE,
			ANY
        };
	protected:
		Request(Type in_type, LLMediaDataClientObject *obj, LLMediaDataClient *mdc, S32 face = -1);
	public:
		LLMediaDataClientObject *getObject() const { return mObject; }
        U32 getNum() const { return mNum; }
		U32 getRetryCount() const { return mRetryCount; }
		void incRetryCount() { mRetryCount++; }
        Type getType() const { return mType; }
		F64 getScore() const { return mScore; }
		std::string getCapability() const;
		const char *getCapName() const;
		const char *getTypeAsString() const;
		void reEnqueue();
		F32 getRetryTimerDelay() const;
		U32 getMaxNumRetries() const;
		bool isObjectValid() const { return mObject.notNull() && (!mObject->isDead()); }
		bool isNew() const { return isObjectValid() && mObject->isNew(); }
		void updateScore();
		void markDead();
		bool isDead();
		void startTracking();
		void stopTracking();
		friend std::ostream& operator<<(std::ostream &s, const Request &q);
		const LLUUID &getID() const { return mObjectID; }
		S32 getFace() const { return mFace; }
		bool isMatch (const Request* other, Type match_type = ANY) const
		{
			return ((match_type == ANY) || (mType == other->mType)) &&
					(mFace == other->mFace) &&
					(mObjectID == other->mObjectID);
		}
	protected:
		LLMediaDataClientObject::ptr_t mObject;
	private:
		Type mType;
		U32 mNum;
		static U32 sNum;
        U32 mRetryCount;
		F64 mScore;
		LLUUID mObjectID;
		S32 mFace;
		LLMediaDataClient *mMDC;
	};
	typedef LLPointer<Request> request_ptr_t;
	class Responder : public LLHTTPClient::ResponderWithResult
	{
	public:
		Responder(const request_ptr_t &request);
		virtual void httpFailure(void);
		virtual void httpSuccess(void);
		AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return mediaDataClientResponder_timeout; }
		char const* getName(void) const { return "LLMediaDataClientResponder"; }
		request_ptr_t &getRequest() { return mRequest; }
	private:
		request_ptr_t mRequest;
	};
	class RetryTimer : public LLEventTimer
	{
	public:
		RetryTimer(F32 time, request_ptr_t);
		virtual BOOL tick();
	private:
		request_ptr_t mRequest;
	};
protected:
	typedef std::list<request_ptr_t> request_queue_t;
	typedef std::set<request_ptr_t> request_set_t;
	virtual const char *getCapabilityName() const = 0;
	virtual void enqueue(Request*) = 0;
	virtual void serviceQueue();
	virtual request_queue_t *getQueue() { return &mQueue; };
	virtual request_ptr_t dequeue();
	virtual bool canServiceRequest(request_ptr_t request) { return true; };
	virtual void pushBack(request_ptr_t request);
	void trackRequest(request_ptr_t request);
	void stopTrackingRequest(request_ptr_t request);
	request_queue_t mQueue;
	const F32 mQueueTimerDelay;
	const F32 mRetryTimerDelay;
	const U32 mMaxNumRetries;
	const U32 mMaxSortedQueueSize;
	const U32 mMaxRoundRobinQueueSize;
	request_set_t mUnQueuedRequests;
	void startQueueTimer();
	void stopQueueTimer();
private:
	static F64 getObjectScore(const LLMediaDataClientObject::ptr_t &obj);
	friend std::ostream& operator<<(std::ostream &s, const Request &q);
	friend std::ostream& operator<<(std::ostream &s, const request_queue_t &q);
	class QueueTimer : public LLEventTimer
	{
	public:
		QueueTimer(F32 time, LLMediaDataClient *mdc);
		virtual BOOL tick();
	private:
		LLPointer<LLMediaDataClient> mMDC;
	};
	void setIsRunning(bool val) { mQueueTimerIsRunning = val; }
	bool mQueueTimerIsRunning;
	template <typename T> friend typename T::iterator find_matching_request(T &c, const LLMediaDataClient::Request *request, LLMediaDataClient::Request::Type match_type);
	template <typename T> friend typename T::iterator find_matching_request(T &c, const LLUUID &id, LLMediaDataClient::Request::Type match_type);
	template <typename T> friend void remove_matching_requests(T &c, const LLUUID &id, LLMediaDataClient::Request::Type match_type);
};
class LLObjectMediaDataClient : public LLMediaDataClient
{
public:
    LOG_CLASS(LLObjectMediaDataClient);
    LLObjectMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
							F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
							U32 max_retries = MAX_RETRIES,
							U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
							U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries),
		  mCurrentQueueIsTheSortedQueue(true)
		{}
	void fetchMedia(LLMediaDataClientObject *object);
    void updateMedia(LLMediaDataClientObject *object);
	class RequestGet: public Request
	{
	public:
		RequestGet(LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
		LLSD getPayload() const;
		Responder *createResponder();
	};
	class RequestUpdate: public Request
	{
	public:
		RequestUpdate(LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
		LLSD getPayload() const;
		Responder *createResponder();
	};
	virtual bool isEmpty() const;
	virtual bool isInQueue(const LLMediaDataClientObject::ptr_t &object);
	virtual void removeFromQueue(const LLMediaDataClientObject::ptr_t &object);
	virtual bool processQueueTimer();
	virtual bool canServiceRequest(request_ptr_t request);
protected:
	virtual const char *getCapabilityName() const;
	virtual request_queue_t *getQueue();
	virtual void enqueue(Request*);
    class Responder : public LLMediaDataClient::Responder
    {
    public:
        Responder(const request_ptr_t &request)
            : LLMediaDataClient::Responder(request) {}
        virtual void httpSuccess(void);
    };
private:
	void swapCurrentQueue();
	request_queue_t mRoundRobinQueue;
	bool mCurrentQueueIsTheSortedQueue;
	static bool compareRequestScores(const request_ptr_t &o1, const request_ptr_t &o2);
	void sortQueue();
};
class LLObjectMediaNavigateClient : public LLMediaDataClient
{
public:
    LOG_CLASS(LLObjectMediaNavigateClient);
	static const int ERROR_PERMISSION_DENIED_CODE = 8002;
    LLObjectMediaNavigateClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
								F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
								U32 max_retries = MAX_RETRIES,
								U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
								U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries)
		{}
    void navigate(LLMediaDataClientObject *object, U8 texture_index, const std::string &url);
	virtual void enqueue(Request*);
	class RequestNavigate: public Request
	{
	public:
		RequestNavigate(LLMediaDataClientObject *obj, LLMediaDataClient *mdc, U8 texture_index, const std::string &url);
		LLSD getPayload() const;
		Responder *createResponder();
		std::string getURL() { return mURL; }
	private:
		std::string mURL;
	};
protected:
	virtual const char *getCapabilityName() const;
    class Responder : public LLMediaDataClient::Responder
    {
    public:
        Responder(const request_ptr_t &request)
            : LLMediaDataClient::Responder(request) {}
		virtual void httpFailure(void);
        virtual void httpSuccess(void);
    private:
        void mediaNavigateBounceBack();
    };
};
#endif
