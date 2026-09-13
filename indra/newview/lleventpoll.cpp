/**
 * @file lleventpoll.cpp
 * @brief Implementation of the LLEventPoll class.
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
#include "llviewerprecompiledheaders.h"
#include "lleventpoll.h"
#include "llappviewer.h"
#include "llagent.h"
#include "llhttpclient.h"
#include "llhttpstatuscodes.h"
#include "llsdserialize.h"
#include "lleventtimer.h"
#include "llsdutil.h"
#include "llviewerregion.h"
#include "message.h"
#include "lltrans.h"
#include "aithreadid.h"
#include <mutex>
#include <sstream>
#include <vector>
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy eventPollResponder_timeout;

namespace
{
	struct PendingEQDispatch
	{
		std::string name;
		std::string sender;
		std::string body_bin;
	};
	std::mutex sPendingEQMutex;
	std::vector<PendingEQDispatch> sPendingEQ;

	bool is_teleport_eq_message(const std::string& name)
	{
		return name == "TeleportFinish" || name == "TeleportStart"
			|| name == "EnableSimulator" || name == "CrossedRegion";
	}

	std::string serialize_llsd_binary(const LLSD& src)
	{
		std::stringstream stream;
		LLSDSerialize::toBinary(src, stream);
		return stream.str();
	}

	LLSD deserialize_llsd_binary(const std::string& blob)
	{
		LLSD dst;
		std::istringstream istr(blob);
		LLSDSerialize::fromBinary(dst, istr, blob.size());
		return dst;
	}
}

void LLEventPoll::dispatchPending()
{
	std::vector<PendingEQDispatch> pending;
	{
		std::lock_guard<std::mutex> lock(sPendingEQMutex);
		if (sPendingEQ.empty())
		{
			return;
		}
		pending.swap(sPendingEQ);
	}
	for (const PendingEQDispatch& item : pending)
	{
		LLSD message;
		message["sender"] = item.sender;
		message["body"] = deserialize_llsd_binary(item.body_bin);
		if (is_teleport_eq_message(item.name))
		{
			LL_INFOS("Messaging") << "Dispatching queued EventQueue message "
				<< item.name << LL_ENDL;
		}
		LLMessageSystem::dispatch(item.name, message);
	}
}

namespace
{
	const F32 EVENT_POLL_ERROR_RETRY_SECONDS = 1.f;
	const F32 EVENT_POLL_ERROR_RETRY_SECONDS_INC = 3.f;
	const S32 MAX_EVENT_POLL_HTTP_ERRORS = 15;
	const F64 MIN_SECONDS_PASSED = 10.0;
	class LLEventPollResponder : public LLHTTPClient::ResponderWithResult
	{
	public:
		static LLHTTPClient::ResponderPtr start(const std::string& pollURL, const LLHost& sender);
		void stop();
		void makeRequest();
	private:
		LLEventPollResponder(const std::string&	pollURL, const LLHost& sender);
		~LLEventPollResponder();
		void handleMessage(const LLSD& content);
		void httpFailure(void);
		void httpSuccess(void);
		bool is_event_poll(void) const { return true; }
		AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return eventPollResponder_timeout; }
		char const* getName(void) const { return "LLEventPollResponder"; }
	private:
		bool	mDone;
		std::string			mPollURL;
		std::string			mSender;
		LLSD	mAcknowledge;
		LLTimer	mRequestTimer;
		static int sCount;
		int	mCount;
		S32 mErrorCount;
	};
	class LLEventPollEventTimer : public LLEventTimer
	{
		typedef boost::intrusive_ptr<LLEventPollResponder> EventPollResponderPtr;
	public:
		LLEventPollEventTimer(F32 period, EventPollResponderPtr responder)
			: LLEventTimer(period), mResponder(responder)
		{ }
		virtual BOOL tick()
		{
			mResponder->makeRequest();
			return TRUE;
		}
	private:
		EventPollResponderPtr mResponder;
	};
	LLHTTPClient::ResponderPtr LLEventPollResponder::start(
		const std::string& pollURL, const LLHost& sender)
	{
		LLHTTPClient::ResponderPtr result = new LLEventPollResponder(pollURL, sender);
		LL_INFOS() << "LLEventPollResponder::start <" << sCount << "> "
				<< pollURL << LL_ENDL;
		return result;
	}
	void LLEventPollResponder::stop()
	{
		LL_INFOS() << "LLEventPollResponder::stop	<" << mCount <<	"> "
				<< mPollURL	<< LL_ENDL;
		mDone =	true;
	}
	int	LLEventPollResponder::sCount =	0;
	LLEventPollResponder::LLEventPollResponder(const std::string& pollURL, const LLHost& sender)
		: mDone(false),
		  mPollURL(pollURL),
		  mCount(++sCount),
		  mErrorCount(0)
	{
		LLViewerRegion *regionp = gAgent.getRegion();
		if (!regionp)
		{
			LL_ERRS() << "LLEventPoll initialized before region is added." << LL_ENDL;
		}
		mSender = sender.getIPandPort();
		LL_INFOS() << "LLEventPoll initialized with sender " << mSender << LL_ENDL;
		makeRequest();
	}
	LLEventPollResponder::~LLEventPollResponder()
	{
		stop();
		LL_DEBUGS() <<	"LLEventPollResponder::~Impl <" <<	mCount << "> "
				 <<	mPollURL <<	LL_ENDL;
	}
	void LLEventPollResponder::makeRequest()
	{
		LLSD request;
		request["ack"] = mAcknowledge;
		request["done"]	= mDone;
		LL_DEBUGS() <<	"LLEventPollResponder::makeRequest	<" << mCount <<	"> ack = "
				 <<	LLSDXMLStreamer(mAcknowledge) << LL_ENDL;
		mRequestTimer.reset();
		LLHTTPClient::post(mPollURL, request, this);
	}
	void LLEventPollResponder::handleMessage(const	LLSD& content)
	{
		std::string	msg_name	= content["message"].asString();
		const LLSD body = content.has("body") ? content["body"] : LLSD();
		if (!AIThreadID::in_main_thread())
		{
			PendingEQDispatch item;
			item.name = msg_name;
			item.sender = mSender;
			item.body_bin = serialize_llsd_binary(body);
			{
				std::lock_guard<std::mutex> lock(sPendingEQMutex);
				sPendingEQ.push_back(item);
			}
			if (is_teleport_eq_message(msg_name))
			{
				LL_INFOS("Messaging") << "Queued EventQueue message " << msg_name
					<< " for main thread (sender " << mSender << ")" << LL_ENDL;
			}
			return;
		}
		LLSD message;
		message["sender"] = mSender;
		message["body"] = deserialize_llsd_binary(serialize_llsd_binary(body));
		LLMessageSystem::dispatch(msg_name, message);
	}
	bool is_expected_empty_poll(S32 status)
	{
		return is_internal_http_error_that_warrants_a_retry(status)
			|| status == HTTP_INTERNAL_ERROR_OTHER
			|| status == HTTP_INTERNAL_SERVER_ERROR
			|| status == HTTP_BAD_GATEWAY
			|| status == HTTP_SERVICE_UNAVAILABLE
			|| status == HTTP_GATEWAY_TIME_OUT;
	}
	void LLEventPollResponder::httpFailure(void)
	{
		if (mDone) return;
		if (is_expected_empty_poll(mStatus))
		{
			const F64 elapsed = mRequestTimer.getElapsedTimeF32();
			if (elapsed >= MIN_SECONDS_PASSED)
			{
				LL_DEBUGS("LLEventPollImpl") << "No events, status: " << mStatus
					<< ", time passed: " << elapsed << LL_ENDL;
				mErrorCount = 0;
				makeRequest();
				return;
			}
			LL_WARNS("LLEventPollImpl") << "Response arrived too early, status: "
				<< mStatus << ", time passed: " << elapsed << LL_ENDL;
		}
		else if (mStatus == HTTP_NOT_FOUND)
		{
			LL_WARNS("LLEventPollImpl") << "Canceling coroutine" << LL_ENDL;
			stop();
			return;
		}
		else if (mCode != CURLE_OK)
		{
		    LL_WARNS("LLEventPollImpl") << "Critical error from poll request returned from libraries.  Canceling coroutine." << LL_ENDL;
			stop();
			return;
		}
		if (mErrorCount < MAX_EVENT_POLL_HTTP_ERRORS)
		{
			++mErrorCount;
			new LLEventPollEventTimer(EVENT_POLL_ERROR_RETRY_SECONDS
										+ mErrorCount * EVENT_POLL_ERROR_RETRY_SECONDS_INC
									, this);
			LL_WARNS() << "Unexpected HTTP error.  status: " << mStatus << ", reason: " << mReason << LL_ENDL;
		}
		else
		{
			LL_WARNS() <<	"LLEventPollResponder::error: <" << mCount << "> got "
					<<	mStatus << ": " << mReason
					<<	(mDone ? " -- done"	: "") << LL_ENDL;
			stop();
		}
	}
	void LLEventPollResponder::httpSuccess(void)
	{
		LL_DEBUGS() <<	"LLEventPollResponder::result <" << mCount	<< ">"
				 <<	(mDone ? " -- done"	: "") << ll_pretty_print_sd(mContent)  << LL_ENDL;
		if (mDone) return;
		mErrorCount = 0;
		if (!mContent.get("events") ||
			!mContent.get("id"))
		{
			makeRequest();
			return;
		}
		mAcknowledge = mContent["id"];
		LLSD events	= mContent["events"];
		if(mAcknowledge.isUndefined())
		{
			LL_WARNS() << "LLEventPollResponder: id undefined" << LL_ENDL;
		}
		LL_DEBUGS() << "LLEventPollResponder::completed <" <<	mCount << "> " << events.size() << "events (id "
				 <<	LLSDXMLStreamer(mAcknowledge) << ")" << LL_ENDL;
		LLSD::array_const_iterator i = events.beginArray();
		LLSD::array_const_iterator end = events.endArray();
		for	(; i !=	end; ++i)
		{
			if (i->has("message"))
			{
				handleMessage(*i);
			}
		}
		makeRequest();
	}
}
LLEventPoll::LLEventPoll(const std::string&	poll_url, const LLHost& sender)
	: mImpl(LLEventPollResponder::start(poll_url, sender))
	{ }
LLEventPoll::~LLEventPoll()
{
	LLHTTPClient::ResponderBase* responderp = mImpl.get();
	LLEventPollResponder* event_poll_responder = dynamic_cast<LLEventPollResponder*>(responderp);
	if (event_poll_responder) event_poll_responder->stop();
}
