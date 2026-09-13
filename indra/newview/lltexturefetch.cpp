/**
 * @file lltexturefetch.cpp
 * @brief Object which fetches textures from the cache and/or network
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"
#include <iostream>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include "llstl.h"
#include "message.h"
#include "lltexturefetch.h"
#include "aicurl.h"
#include "lldir.h"
#include "llhttpclient.h"
#include "llhttpstatuscodes.h"
#include "llimage.h"
#include "llimagej2c.h"
#include "llimageworker.h"
#include "llworkerthread.h"
#include "message.h"
#include "llagent.h"
#include "lltexturecache.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewertexture.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerstatsrecorder.h"
#include "llviewerassetstats.h"
#include "llworld.h"
#include "llsdutil.h"
#include "llstartup.h"
#include "llsdserialize.h"
#include "llbuffer.h"
#include "llhttpretrypolicy.h"
#include "hippogridmanager.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy HTTPGetResponder_timeout;
extern AIHTTPTimeoutPolicy lcl_responder_timeout;
extern AIHTTPTimeoutPolicy assetReportHandler_timeout;
LLStat LLTextureFetch::sCacheHitRate("texture_cache_hits", 128);
LLStat LLTextureFetch::sCacheReadLatency("texture_cache_read_latency", 128);
static const char * const LOG_TXT = "Texture";
class LLTextureFetchWorker : public LLWorkerClass
{
	friend class LLTextureFetch;
	friend class HTTPGetResponder;
private:
	class CacheReadResponder : public LLTextureCache::ReadResponder
	{
	public:
		CacheReadResponder(LLTextureFetch* fetcher, const LLUUID& id, LLImageFormatted* image)
			: mFetcher(fetcher), mID(id)
		{
			setImage(image);
		}
		virtual void completed(bool success)
		{
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
 				worker->callbackCacheRead(success, mFormattedImage, mImageSize, mImageLocal);
			}
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
	};
	class CacheWriteResponder : public LLTextureCache::WriteResponder
	{
	public:
		CacheWriteResponder(LLTextureFetch* fetcher, const LLUUID& id)
			: mFetcher(fetcher), mID(id)
		{
		}
		virtual void completed(bool success)
		{
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
				worker->callbackCacheWrite(success);
			}
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
	};
	class DecodeResponder : public LLImageDecodeThread::Responder
	{
	public:
		DecodeResponder(LLTextureFetch* fetcher, const LLUUID& id, LLTextureFetchWorker* worker)
			: mFetcher(fetcher), mID(id), mWorker(worker)
		{
		}
		virtual void completed(bool success, LLImageRaw* raw, LLImageRaw* aux)
		{
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
 				worker->callbackDecoded(success, raw, aux);
			}
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
		LLTextureFetchWorker* mWorker;
	};
	struct Compare
	{
		bool operator()(const LLTextureFetchWorker* lhs, const LLTextureFetchWorker* rhs) const
		{
			const F32 lpriority = lhs->mImagePriority;
			const F32 rpriority = rhs->mImagePriority;
			if (lpriority > rpriority)
				return true;
			else if (lpriority < rpriority)
				return false;
			else
				return lhs < rhs;
		}
	};
public:
	bool doWork(S32 param);
	void finishWork(S32 param, bool completed);
	bool deleteOK();
	~LLTextureFetchWorker();
	S32 callbackHttpGet(U32 offset, U32 length,
						 const LLChannelDescriptors& channels,
						 const LLHTTPClient::ResponderBase::buffer_ptr_t& buffer,
						 bool partial, bool success);
	void callbackCacheRead(bool success, LLImageFormatted* image,
						   S32 imagesize, BOOL islocal);
	void callbackCacheWrite(bool success);
	void callbackDecoded(bool success, LLImageRaw* raw, LLImageRaw* aux);
	void setGetStatus(U32 status, const std::string& reason)
	{
		LLMutexLock lock(&mWorkMutex);
		mGetStatus = status;
		mGetReason = reason;
	}
	void setCanUseHTTP(bool can_use_http) { mCanUseHTTP = can_use_http; }
	bool getCanUseHTTP() const { return mCanUseHTTP; }
	LLTextureFetch & getFetcher() { return *mFetcher; }
protected:
	LLTextureFetchWorker(LLTextureFetch* fetcher, FTType f_type,
						 const std::string& url, const LLUUID& id, const LLHost& host,
						 F32 priority, S32 discard, S32 size);
private:
	void startWork(S32 param);
	void endWork(S32 param, bool aborted);
	void resetFormattedData();
	void setImagePriority(F32 priority);
	void setDesiredDiscard(S32 discard, S32 size);
	bool insertPacket(S32 index, U8* data, S32 size);
	void clearPackets();
	void setupPacketData();
	U32 calcWorkPriority();
	void removeFromCache();
	bool processSimulatorPackets();
	bool writeToCacheComplete();
	void recordTextureStart(bool is_http);
	void recordTextureDone(bool is_http);
	void lockWorkMutex() { mWorkMutex.lock(); }
	void unlockWorkMutex() { mWorkMutex.unlock(); }
private:
	enum e_state
	{
		INVALID = 0,
		INIT,
		LOAD_FROM_TEXTURE_CACHE,
		CACHE_POST,
		LOAD_FROM_NETWORK,
		LOAD_FROM_SIMULATOR,
		SEND_UDP_REQ,
		WAIT_UDP_REQ,
		SEND_HTTP_REQ,
		WAIT_HTTP_REQ,
		DECODE_IMAGE,
		DECODE_IMAGE_UPDATE,
		WRITE_TO_CACHE,
		WAIT_ON_WRITE,
		DONE
	};
	enum e_request_state
	{
		UNSENT = 0,
		QUEUED = 1,
		SENT_SIM = 2
	};
	enum e_write_to_cache_state
	{
		NOT_WRITE = 0,
		CAN_WRITE = 1,
		SHOULD_WRITE = 2
	};
	static const char* sStateDescs[];
	e_state mState;
	void setState(e_state new_state);
	e_write_to_cache_state mWriteToCacheState;
	LLTextureFetch* mFetcher;
	LLPointer<LLImageFormatted> mFormattedImage;
	LLPointer<LLImageRaw>       mRawImage,
								mAuxImage;
	const FTType mFTType;
	LLUUID mID;
	LLHost mHost;
	std::string mUrl;
	AIPerServicePtr mPerServicePtr;
	U8 mType;
	F32 mImagePriority;
	U32 mWorkPriority;
	F32 mRequestedPriority;
	S32                         mDesiredDiscard,
								mSimRequestedDiscard,
								mRequestedDiscard,
								mLoadedDiscard,
								mDecodedDiscard;
	LLFrameTimer                mRequestedTimer,
								mFetchTimer;
	LLTimer			mCacheReadTimer;
	F32				mCacheReadTime;
	LLTextureCache::handle_t    mCacheReadHandle,
								mCacheWriteHandle;
	std::vector<U8> mHttpBuffer;
	S32                         mRequestedSize,
								mRequestedOffset,
								mDesiredSize,
								mFileSize,
								mCachedSize;
	e_request_state mSentRequest;
	handle_t mDecodeHandle;
	BOOL mLoaded;
	BOOL mDecoded;
	BOOL mWritten;
	BOOL mNeedsAux;
	BOOL mHaveAllData;
	BOOL mInLocalCache;
	bool                        mCanUseHTTP,
								mCanUseNET ;
	S32 mHTTPFailCount;
	S32 mRetryAttempt;
	S32 mActiveCount;
	U32 mGetStatus;
	std::string mGetReason;
	LLMutex mWorkMutex;
	struct PacketData
	{
		PacketData(U8* data, S32 size)
		:	mData(data), mSize(size)
		{}
		~PacketData() { clearData(); }
		void clearData() { delete[] mData; mData = NULL; }
		U8* mData;
		U32 mSize;
	};
	std::vector<PacketData*> mPackets;
	S32 mFirstPacket;
	S32 mLastPacket;
	U16 mTotalPackets;
	U8 mImageCodec;
	LLViewerAssetStats::duration_t mMetricsStartTime;
	U32						mHttpReplySize,
							mHttpReplyOffset;
	U32						mCacheReadCount,
							mCacheWriteCount;
};
class HTTPGetResponder : public LLHTTPClient::ResponderWithCompleted
{
	LOG_CLASS(HTTPGetResponder);
public:
	HTTPGetResponder( FTType f_type, LLTextureFetch* fetcher, const LLUUID& id, U64 startTime, S32 requestedSize, U32 offset)
		: mFetcher(fetcher)
		, mID(id)
		, mMetricsStartTime(startTime)
		, mRequestedSize(requestedSize)
		, mRequestedOffset(offset)
		, mReplyOffset(0)
		, mReplyLength(0)
		, mReplyFullLength(0)
		, mFTType(f_type)
	{
		mFetchRetryPolicy = new LLAdaptiveRetryPolicy(10.0,3600.0,2.0,10);
	}
	~HTTPGetResponder()
	{
	}
	bool needsHeaders(void) const { return true; }
	void completedHeaders(void) {
		LL_DEBUGS("Texture") << "HTTP HEADERS COMPLETE: " << mID << LL_ENDL;
		std::string	rangehdr;
		if (mReceivedHeaders.getFirstValue("content-range", rangehdr))
		{
			std::vector<std::string> tokens;
			boost::split(tokens,rangehdr,boost::is_any_of(" -/"));
			if(tokens.size() == 4 && !stricmp(tokens[0].c_str(),"bytes"))
			{
				U32 first(0), last(0), len(0);
				try
				{
					first = boost::lexical_cast<U32>(tokens[1].c_str());
					last = boost::lexical_cast<U32>(tokens[2].c_str());
				}
				catch( boost::bad_lexical_cast& )
				{
					return;
				}
				if(tokens[3] != "*")
				{
					try
					{
						len = boost::lexical_cast<U32>(tokens[3].c_str());
					}
					catch( boost::bad_lexical_cast& )
					{
						len = 0;
					}
				}
				if(first <= last && (!len || last < len))
				{
					mReplyOffset = first;
					mReplyLength = last - first + 1;
					mReplyFullLength = len;
					LL_DEBUGS("Texture") << " mReplyOffset=" << mReplyOffset << " mReplyLength=" << mReplyLength << " mReplyFullLength=" << mReplyFullLength << LL_ENDL;
				}
			}
		}
	}
	void completedRaw(LLChannelDescriptors const& channels,
								  buffer_ptr_t const& buffer)
	{
		static LLCachedControl<bool> log_to_viewer_log(gSavedSettings,"LogTextureDownloadsToViewerLog");
		static LLCachedControl<bool> log_to_sim(gSavedSettings,"LogTextureDownloadsToSimulator");
		static LLCachedControl<bool> log_texture_traffic(gSavedSettings,"LogTextureNetworkTraffic") ;
		if (log_to_viewer_log || log_to_sim)
		{
			mFetcher->mTextureInfo.setRequestStartTime(mID, mMetricsStartTime);
			mFetcher->mTextureInfo.setRequestType(mID, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
			mFetcher->mTextureInfo.setRequestSize(mID, mRequestedSize);
			mFetcher->mTextureInfo.setRequestOffset(mID, mRequestedOffset);
			mFetcher->mTextureInfo.setRequestCompleteTimeAndLog(mID, LLTimer::getTotalTime());
		}
		LL_DEBUGS("Texture") << "HTTP COMPLETE: " << mID << LL_ENDL;
		if (mFetcher)
		{
			mFetcher->recordHttpResult(mStatus, mFTType == FTT_MAP_TILE, (S32)mReplyLength);
		}
		LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
		if (worker)
		{
			worker->lockWorkMutex();
			bool success = false;
			bool partial = false;
			if (HTTP_OK <= mStatus &&  mStatus < HTTP_MULTIPLE_CHOICES)
			{
				mFetchRetryPolicy->onSuccess();
				success = true;
				if (HTTP_PARTIAL_CONTENT == mStatus)
				{
					partial = true;
				}
			}
			if (!success)
			{
				if(mFTType == FTT_SERVER_BAKE)
				{
					mFetchRetryPolicy->onFailure(getStatus(), getResponseHeaders());
					F32 retry_after;
					if (mFetchRetryPolicy->shouldRetry(retry_after))
					{
						LL_INFOS(LOG_TXT) << mID << " will retry after " << retry_after << " seconds, resetting state to LOAD_FROM_NETWORK" << LL_ENDL;
						mFetcher->removeFromHTTPQueue(mID, 0);
						worker->setGetStatus(mStatus, mReason);
						worker->setState(LLTextureFetchWorker::LOAD_FROM_NETWORK);
						worker->unlockWorkMutex();
						return;
					}
				}
				worker->setGetStatus(mStatus, mReason);
				if (mFTType != FTT_MAP_TILE)
				{
					LL_WARNS(LOG_TXT) << "CURL GET FAILED, status:" << mStatus
									  << " reason: " << mReason << LL_ENDL;
				}
			}
			S32BytesImplicit data_size = worker->callbackHttpGet(mReplyOffset, mReplyLength, channels, buffer, partial, success);
			if(log_texture_traffic && data_size > 0)
			{
				std::vector<LLViewerTexture*> textures;
				LLViewerTextureManager::findTextures(mID, textures);
				std::vector<LLViewerTexture*>::iterator iter = textures.begin();
				while (iter != textures.end())
				{
					LLViewerTexture* tex = *iter++;
					if (tex)
					{
						gTotalTextureBytesPerBoostLevel[tex->getBoostLevel()] += data_size;
					}
				}
			}
			mFetcher->removeFromHTTPQueue(mID, data_size);
			worker->recordTextureDone(true);
			worker->unlockWorkMutex();
		}
		else
		{
			mFetcher->removeFromHTTPQueue(mID);
 			LL_WARNS() << "Worker not found: " << mID << LL_ENDL;
		}
	}
	AICapabilityType capability_type(void) const { return cap_texture; }
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return HTTPGetResponder_timeout; }
	char const* getName(void) const { return "HTTPGetResponder"; }
private:
	LLTextureFetch* mFetcher;
	LLUUID mID;
	const FTType mFTType;
	LLPointer<LLHTTPRetryPolicy> mFetchRetryPolicy;
	U64 mMetricsStartTime;
	S32 mRequestedSize;
	U32 mRequestedOffset;
	U32 mReplyOffset;
	U32 mReplyLength;
	U32 mReplyFullLength;
};
class SGHostBlackList{
	static const int MAX_ERRORCOUNT = 20;
	struct BlackListEntry {
		std::string host;
		U64 timeUntil;
		U32 reason;
		int errorCount;
	};
	typedef std::vector<BlackListEntry> blacklist_t;
	typedef blacklist_t::iterator iter;
	static blacklist_t blacklist;
	static bool is_obsolete(BlackListEntry entry) {
			U64 now = LLTimer::getTotalTime();
			return (now > entry.timeUntil);
	}
	static void cleanup() {
		(void)std::remove_if(blacklist.begin(), blacklist.end(), is_obsolete);
	}
	static iter find(std::string host) {
		cleanup();
		for(blacklist_t::iterator i = blacklist.begin(); i != blacklist.end(); ++i) {
			if (i->host.find(host) == 0) return i;
		}
		return blacklist.end();
	}
public:
	static bool isBlacklisted(std::string url) {
		iter found = find(url);
		bool r = (found != blacklist.end()) && (found->errorCount > MAX_ERRORCOUNT);
		return r;
	}
	static void add(std::string url, float timeout, U32 reason) {
		LL_WARNS() << "Requested adding to blacklist: " << url << LL_ENDL;
		BlackListEntry entry;
		entry.host = url.substr(0, url.rfind("/"));
		if (entry.host.empty()) return;
		entry.timeUntil = LLTimer::getTotalTime() + timeout*1000;
		entry.reason = reason;
		entry.errorCount = 0;
		iter found = find(entry.host);
		if(found != blacklist.end()) {
			entry.errorCount = found->errorCount + 1;
			*found = entry;
			if (entry.errorCount > MAX_ERRORCOUNT) {
				std::string s;
				microsecondsToTimecodeString(entry.timeUntil, s);
				LL_WARNS() << "Blacklisting address " << entry.host
					<< "is blacklisted for " << timeout
					<< " seconds because of error " << reason
					<< LL_ENDL;
			}
		}
		else blacklist.push_back(entry);
	}
};
SGHostBlackList::blacklist_t SGHostBlackList::blacklist;
class LLTextureFetch::TFRequest
{
public:
	virtual ~TFRequest()
		{}
	virtual bool doWork(LLTextureFetch * fetcher) = 0;
};
namespace
{
class TFReqSetRegion : public LLTextureFetch::TFRequest
{
public:
	TFReqSetRegion(U64 region_handle)
		: LLTextureFetch::TFRequest(),
		  mRegionHandle(region_handle)
		{}
	TFReqSetRegion & operator=(const TFReqSetRegion &);
	virtual ~TFReqSetRegion()
		{}
	virtual bool doWork(LLTextureFetch * fetcher);
public:
	const U64 mRegionHandle;
};
class TFReqSendMetrics : public LLTextureFetch::TFRequest
{
public:
	TFReqSendMetrics(const std::string & caps_url,
					 const LLUUID & session_id,
					 const LLUUID & agent_id,
					 LLViewerAssetStats * main_stats)
		: LLTextureFetch::TFRequest(),
		  mCapsURL(caps_url),
		  mSessionID(session_id),
		  mAgentID(agent_id),
		  mMainStats(main_stats)
		{}
	TFReqSendMetrics & operator=(const TFReqSendMetrics &);
	virtual ~TFReqSendMetrics();
	virtual bool doWork(LLTextureFetch * fetcher);
public:
	const std::string mCapsURL;
	const LLUUID mSessionID;
	const LLUUID mAgentID;
	LLViewerAssetStats * mMainStats;
};
bool truncate_viewer_metrics(int max_regions, LLSD & metrics);
}
const char* LLTextureFetchWorker::sStateDescs[] = {
	"INVALID",
	"INIT",
	"LOAD_FROM_TEXTURE_CACHE",
	"CACHE_POST",
	"LOAD_FROM_NETWORK",
	"LOAD_FROM_SIMULATOR",
	"SEND_UDP_REQ",
	"WAIT_UDP_REQ",
	"SEND_HTTP_REQ",
	"WAIT_HTTP_REQ",
	"DECODE_IMAGE",
	"DECODE_IMAGE_UPDATE",
	"WRITE_TO_CACHE",
	"WAIT_ON_WRITE",
	"DONE"
};
volatile bool LLTextureFetch::svMetricsDataBreak(true);
LLTextureFetchWorker::LLTextureFetchWorker(LLTextureFetch* fetcher,
										   FTType f_type,
										   const std::string& url,
										   const LLUUID& id,
										   const LLHost& host,
										   F32 priority,
										   S32 discard,
										   S32 size)
	: LLWorkerClass(fetcher, "TextureFetch"),
	  mState(INIT),
	  mWriteToCacheState(NOT_WRITE),
	  mFetcher(fetcher),
	  mFTType(f_type),
	  mID(id),
	  mHost(host),
	  mUrl(url),
	  mImagePriority(priority),
	  mWorkPriority(0),
	  mRequestedPriority(0.f),
	  mDesiredDiscard(-1),
	  mSimRequestedDiscard(-1),
	  mRequestedDiscard(-1),
	  mLoadedDiscard(-1),
	  mDecodedDiscard(-1),
	  mCacheReadTime(0.f),
	  mCacheReadHandle(LLTextureCache::nullHandle()),
	  mCacheWriteHandle(LLTextureCache::nullHandle()),
	  mRequestedSize(0),
	  mRequestedOffset(0),
	  mDesiredSize(TEXTURE_CACHE_ENTRY_SIZE),
	  mFileSize(0),
	  mCachedSize(0),
	  mLoaded(FALSE),
	  mSentRequest(UNSENT),
	  mDecodeHandle(0),
	  mDecoded(FALSE),
	  mWritten(FALSE),
	  mNeedsAux(FALSE),
	  mHaveAllData(FALSE),
	  mInLocalCache(FALSE),
	  mCanUseHTTP(true),
	  mHTTPFailCount(0),
	  mRetryAttempt(0),
	  mActiveCount(0),
	  mGetStatus(0),
	  mFirstPacket(0),
	  mLastPacket(-1),
	  mTotalPackets(0),
	  mImageCodec(IMG_CODEC_INVALID),
	  mMetricsStartTime(0),
	  mHttpReplySize(0U),
	  mHttpReplyOffset(0U),
	  mCacheReadCount(0U),
	  mCacheWriteCount(0U)
{
	mCanUseNET = mUrl.empty();
	if (!mCanUseNET)
	{
	  std::string servicename = AIPerService::extract_canonical_servicename(mUrl);
	  if (!servicename.empty())
	  {
		mPerServicePtr = AIPerService::instance(servicename);
	  }
	}
	calcWorkPriority();
	mType = host.isOk() ? LLImageBase::TYPE_AVATAR_BAKE : LLImageBase::TYPE_NORMAL;
	if (!mFetcher->mDebugPause)
	{
		U32 work_priority = mWorkPriority | LLWorkerThread::PRIORITY_HIGH;
		addWork(0, work_priority );
	}
	setDesiredDiscard(discard, size);
}
LLTextureFetchWorker::~LLTextureFetchWorker()
{
	llassert_always(!haveWork());
	lockWorkMutex();
	if (mCacheReadHandle != LLTextureCache::nullHandle() && mFetcher->mTextureCache)
	{
		mFetcher->mTextureCache->readComplete(mCacheReadHandle, true);
	}
	if (mCacheWriteHandle != LLTextureCache::nullHandle() && mFetcher->mTextureCache)
	{
		mFetcher->mTextureCache->writeComplete(mCacheWriteHandle, true);
	}
	mFormattedImage = NULL;
	clearPackets();
	unlockWorkMutex();
	mFetcher->removeFromHTTPQueue(mID);
	mFetcher->updateStateStats(mCacheReadCount, mCacheWriteCount);
}
void LLTextureFetchWorker::clearPackets()
{
	for_each(mPackets.begin(), mPackets.end(), DeletePointer());
	mPackets.clear();
	mTotalPackets = 0;
	mLastPacket = -1;
	mFirstPacket = 0;
}
void LLTextureFetchWorker::setupPacketData()
{
	S32 data_size = 0;
	if (mFormattedImage.notNull())
	{
		data_size = mFormattedImage->getDataSize();
	}
	if (data_size > 0)
	{
		mFirstPacket = (data_size - FIRST_PACKET_SIZE) / MAX_IMG_PACKET_SIZE + 1;
		if (FIRST_PACKET_SIZE + (mFirstPacket-1) * MAX_IMG_PACKET_SIZE != data_size)
		{
			LL_WARNS(LOG_TXT) << "Bad CACHED TEXTURE size: " << data_size << " removing." << LL_ENDL;
			removeFromCache();
			resetFormattedData();
			clearPackets();
		}
		else if (mFileSize > 0)
		{
			mLastPacket = mFirstPacket-1;
			mTotalPackets = (mFileSize - FIRST_PACKET_SIZE + MAX_IMG_PACKET_SIZE-1) / MAX_IMG_PACKET_SIZE + 1;
		}
		else
		{
			resetFormattedData();
			clearPackets();
		}
	}
}
U32 LLTextureFetchWorker::calcWorkPriority()
{
	static const F32 PRIORITY_SCALE = (F32)LLWorkerThread::PRIORITY_LOWBITS / LLViewerFetchedTexture::maxDecodePriority();
	mWorkPriority = llmin((U32)LLWorkerThread::PRIORITY_LOWBITS, (U32)(mImagePriority * PRIORITY_SCALE));
	return mWorkPriority;
}
void LLTextureFetchWorker::setDesiredDiscard(S32 discard, S32 size)
{
	bool prioritize = false;
	if (mDesiredDiscard != discard)
	{
		if (!haveWork())
		{
			calcWorkPriority();
			if (!mFetcher->mDebugPause)
			{
				U32 work_priority = mWorkPriority | LLWorkerThread::PRIORITY_HIGH;
				addWork(0, work_priority);
			}
		}
		else if (mDesiredDiscard < discard)
		{
			prioritize = true;
		}
		mDesiredDiscard = discard;
		mDesiredSize = size;
	}
	else if (size > mDesiredSize)
	{
		mDesiredSize = size;
		prioritize = true;
	}
	mDesiredSize = llmax(mDesiredSize, TEXTURE_CACHE_ENTRY_SIZE);
	if ((prioritize && mState == INIT) || mState == DONE)
	{
		setState(INIT);
		U32 work_priority = mWorkPriority | LLWorkerThread::PRIORITY_HIGH;
		setPriority(work_priority);
	}
}
void LLTextureFetchWorker::setImagePriority(F32 priority)
{
	F32 delta = fabs(priority - mImagePriority);
	if (delta > (mImagePriority * .05f) || mState == DONE)
	{
		mImagePriority = priority;
		calcWorkPriority();
		U32 work_priority = mWorkPriority | (getPriority() & LLWorkerThread::PRIORITY_HIGHBITS);
		setPriority(work_priority);
	}
}
void LLTextureFetchWorker::resetFormattedData()
{
	std::vector<U8>().swap(mHttpBuffer);
	if (mFormattedImage.notNull())
	{
		mFormattedImage->deleteData();
	}
	mHttpReplySize = 0;
	mHttpReplyOffset = 0;
	mHaveAllData = FALSE;
}
void LLTextureFetchWorker::startWork(S32 param)
{
	llassert(mFormattedImage.isNull());
}
#include "llviewertexturelist.h"
bool LLTextureFetchWorker::doWork(S32 param)
{
	LLMutexLock lock(&mWorkMutex);
	if ((mFetcher->isQuitting() || getFlags(LLWorkerClass::WCF_DELETE_REQUESTED)))
	{
		if (mState < DECODE_IMAGE)
	{
			return true;
		}
	}
	if(mImagePriority < F_ALMOST_ZERO)
	{
		if (mState == INIT || mState == LOAD_FROM_NETWORK)
		{
			LL_DEBUGS(LOG_TXT) << mID << " abort: mImagePriority < F_ALMOST_ZERO" << LL_ENDL;
			return true;
		}
	}
	if(mState > CACHE_POST && !mCanUseNET && !mCanUseHTTP)
	{
		LL_WARNS(LOG_TXT) << mID << " abort, nowhere to get data" << LL_ENDL;
		return true ;
	}
	if (mFetcher->mDebugPause)
	{
		return false;
	}
	if (mID == mFetcher->mDebugID)
	{
		mFetcher->mDebugCount++;
	}
	if (mState != DONE)
	{
		mFetchTimer.reset();
	}
	if (mState == INIT)
	{
		if(gAssetStorage && std::find(gAssetStorage->mBlackListedAsset.begin(),
			gAssetStorage->mBlackListedAsset.end(),mID) != gAssetStorage->mBlackListedAsset.end())
		{
			LL_INFOS() << "Blacklisted asset " << mID.asString() << " was trying to be accessed!!!!!!" << LL_ENDL;
			setState(DONE);
			return true;
		}
		mRawImage = NULL ;
		mRequestedDiscard = -1;
		mLoadedDiscard = -1;
		mDecodedDiscard = -1;
		mRequestedSize = 0;
		mRequestedOffset = 0;
		mFileSize = 0;
		mCachedSize = 0;
		mLoaded = FALSE;
		mSentRequest = UNSENT;
		mDecoded  = FALSE;
		mWritten  = FALSE;
		std::vector<U8>().swap(mHttpBuffer);
		mHttpReplySize = 0;
		mHttpReplyOffset = 0;
		mHaveAllData = FALSE;
		clearPackets();
		mCacheReadHandle = LLTextureCache::nullHandle();
		mCacheWriteHandle = LLTextureCache::nullHandle();
		setState(LOAD_FROM_TEXTURE_CACHE);
		mDesiredSize = llmax(mDesiredSize, TEXTURE_CACHE_ENTRY_SIZE);
		LL_DEBUGS(LOG_TXT) << mID << ": Priority: " << llformat("%8.0f",mImagePriority)
							 << " Desired Discard: " << mDesiredDiscard << " Desired Size: " << mDesiredSize << LL_ENDL;
	}
	if (mState == LOAD_FROM_TEXTURE_CACHE)
	{
		if (mCacheReadHandle == LLTextureCache::nullHandle())
		{
			U32 cache_priority = mWorkPriority;
			S32 offset = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
			S32 size = mDesiredSize - offset;
			if (size <= 0)
			{
				setState(CACHE_POST);
				return false;
			}
			mFileSize = 0;
			mLoaded = FALSE;
			if (mUrl.compare(0, 7, "file://") == 0)
			{
				setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
				++mCacheReadCount;
				std::string filename = mUrl.substr(7, std::string::npos);
				CacheReadResponder* responder = new CacheReadResponder(mFetcher, mID, mFormattedImage);
				mCacheReadHandle = mFetcher->mTextureCache->readFromCache(filename, mID, cache_priority,
																		  offset, size, responder);
				mCacheReadTimer.reset();
			}
			else if ((mUrl.empty() || mFTType==FTT_SERVER_BAKE))
			{
				setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
				++mCacheReadCount;
				CacheReadResponder* responder = new CacheReadResponder(mFetcher, mID, mFormattedImage);
				mCacheReadHandle = mFetcher->mTextureCache->readFromCache(mID, cache_priority,
																		  offset, size, responder);
				mCacheReadTimer.reset();
			}
			else if(!mUrl.empty() && mCanUseHTTP)
			{
				if (!(mUrl.compare(0, 7, "http://") == 0))
				{
					LL_WARNS() << "Unknown URL Type: " << mUrl << LL_ENDL;
				}
				setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
				setState(SEND_HTTP_REQ);
			}
			else
			{
				setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
				setState(LOAD_FROM_NETWORK);
			}
		}
		if (mLoaded)
		{
			if (mFetcher->mTextureCache->readComplete(mCacheReadHandle, false))
			{
				mCacheReadHandle = LLTextureCache::nullHandle();
				setState(CACHE_POST);
			}
			else
			{
				LL_DEBUGS(LOG_TXT) << mID << " this should never happen" << LL_ENDL;
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	if (mState == CACHE_POST)
	{
		mCachedSize = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
		if ((mCachedSize >= mDesiredSize) || mHaveAllData)
		{
			llassert_always(mFormattedImage->getDataSize() > 0);
			mLoadedDiscard = mDesiredDiscard;
			if (mLoadedDiscard < 0)
			{
				LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
									<< ", should be >=0" << LL_ENDL;
			}
			setState(DECODE_IMAGE);
			mWriteToCacheState = NOT_WRITE ;
			LL_DEBUGS(LOG_TXT) << mID << ": Cached. Bytes: " << mFormattedImage->getDataSize()
								 << " Size: " << llformat("%dx%d",mFormattedImage->getWidth(),mFormattedImage->getHeight())
								 << " Desired Discard: " << mDesiredDiscard << " Desired Size: " << mDesiredSize << LL_ENDL;
			LLTextureFetch::sCacheHitRate.addValue(100.f);
		}
		else
		{
			if (mUrl.compare(0, 7, "file://") == 0)
			{
				LL_WARNS(LOG_TXT) << mID << ": abort, failed to load local file " << mUrl << LL_ENDL;
				return true;
			}
			else
			{
				LL_DEBUGS(LOG_TXT) << mID << ": Not in Cache" << LL_ENDL;
				setState(LOAD_FROM_NETWORK);
			}
			LLTextureFetch::sCacheHitRate.addValue(0.f);
		}
	}
	if (mState == LOAD_FROM_NETWORK)
	{
		static LLCachedControl<bool> use_http(gSavedSettings,"ImagePipelineUseHTTP");
		bool is_sl = gHippoGridManager->getConnectedGrid()->isSecondLife();
		if ((is_sl || use_http) && mCanUseHTTP && mUrl.empty())
		{
			LLViewerRegion* region = NULL;
			if (mHost == LLHost::invalid)
				region = gAgent.getRegion();
			else
				region = LLWorld::getInstance()->getRegion(mHost);
			if (region)
			{
				std::string http_url = region->getViewerAssetUrl();
				if (http_url.empty()) http_url = region->getCapability("GetTexture");
				if (!http_url.empty())
				{
					if (mFTType != FTT_DEFAULT)
					{
						LL_WARNS(LOG_TXT) << "trying to seek a non-default texture on the sim. Bad! mFTType: " << mFTType << LL_ENDL;
					}
					mUrl = http_url + "/?texture_id=" + mID.asString().c_str();
					LL_DEBUGS(LOG_TXT) << "Texture URL: " << mUrl << LL_ENDL;
					mWriteToCacheState = CAN_WRITE ;
					mPerServicePtr = AIPerService::instance(AIPerService::extract_canonical_servicename(http_url));
				}
				else
				{
					mCanUseHTTP = false ;
					LL_DEBUGS(LOG_TXT) << "Texture not available via HTTP: empty URL." << LL_ENDL;
				}
			}
			else
			{
				LL_DEBUGS(LOG_TXT) << "Texture not available via HTTP: no region " << mUrl << LL_ENDL;
				mCanUseHTTP = false;
			}
		}
		else if (mFTType == FTT_SERVER_BAKE)
		{
			mWriteToCacheState = CAN_WRITE;
		}
		if (!mUrl.empty() && SGHostBlackList::isBlacklisted(mUrl)){
			LL_DEBUGS("Texture") << mID << "Blacklisted" << LL_ENDL;
			mCanUseHTTP = false;
		}
		if (mCanUseHTTP && !mUrl.empty())
		{
			setState(SEND_HTTP_REQ);
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			if(mWriteToCacheState != NOT_WRITE)
			{
				mWriteToCacheState = CAN_WRITE ;
			}
		}
		else if (!mCanUseNET)
		{
			LL_WARNS(LOG_TXT) << mID << "Unable to retrieve texture via HTTP and UDP unavailable (probable 404): " << mUrl << LL_ENDL;
			return true;
		}
		else if (mSentRequest == UNSENT)
		{
			LL_DEBUGS("Texture") << mID << " moving to UDP fetch. mSentRequest=" << mSentRequest << " mCanUseNET = " << mCanUseNET << LL_ENDL;
			setState(SEND_UDP_REQ);
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			if(mWriteToCacheState != NOT_WRITE)
			{
				mWriteToCacheState = CAN_WRITE ;
			}
		}
		else
		{
			LL_WARNS("Texture") << mID << " does this happen? mSentRequest=" << mSentRequest << " mCanUseNET = " << mCanUseNET << LL_ENDL;
			return false;
		}
	}
	if (mState == LOAD_FROM_SIMULATOR)
	{
		if (mFormattedImage.isNull())
		{
			mFormattedImage = new LLImageJ2C;
		}
		if (processSimulatorPackets())
		{
			LL_DEBUGS(LOG_TXT) << mID << ": Loaded from Sim. Bytes: " << mFormattedImage->getDataSize() << LL_ENDL;
			mFetcher->removeFromNetworkQueue(this, false);
			if (mFormattedImage.isNull() || !mFormattedImage->getDataSize())
			{
				LL_WARNS(LOG_TXT) << mID << " processSimulatorPackets() failed to load buffer" << LL_ENDL;
				return true;
			}
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			if (mLoadedDiscard < 0)
			{
				LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
									<< ", should be >=0" << LL_ENDL;
			}
			setState(DECODE_IMAGE);
			llassert_always(strstr(mUrl.c_str(), "map.secondlife") == NULL);
			mWriteToCacheState = SHOULD_WRITE;
			recordTextureDone(false);
		}
		else
		{
			llassert(mFetcher->mNetworkQueue.find(mID) != mFetcher->mNetworkQueue.end());
		}
		return false;
	}
	if (mState == SEND_UDP_REQ)
	{
		if (! mCanUseNET)
		{
			LL_WARNS("Texture") << mID << " abort: SEND_UDP_REQ but !mCanUseNet" << LL_ENDL;
			return true ;
		}
		LL_DEBUGS("Texture") << mID << " sending to UDP fetch. mSentRequest=" << mSentRequest << " mCanUseNET = " << mCanUseNET << LL_ENDL;
		mRequestedSize = mDesiredSize;
		mRequestedDiscard = mDesiredDiscard;
		mSentRequest = QUEUED;
		mFetcher->addToNetworkQueue(this);
		recordTextureStart(false);
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		setState(WAIT_UDP_REQ);
	}
	if (mState == WAIT_UDP_REQ)
	{
		if (! mCanUseNET)
		{
			LL_WARNS("Texture") << mID << " abort: SEND_UDP_REQ but !mCanUseNet" << LL_ENDL;
			return true ;
		}
		llassert(mFetcher->mNetworkQueue.find(mID) != mFetcher->mNetworkQueue.end());
	}
	if (mState == SEND_HTTP_REQ)
	{
		if (! mCanUseHTTP)
		{
			LL_WARNS(LOG_TXT) << mID << " abort: SEND_HTTP_REQ but !mCanUseHTTP" << LL_ENDL;
			return true ;
		}
		S32 cur_size = 0;
		if (mFormattedImage.notNull())
		{
			cur_size = mFormattedImage->getDataSize();
			if (mFormattedImage->getDiscardLevel() == 0)
			{
				mFetcher->removeFromNetworkQueue(this, false);
				if (cur_size > 0)
				{
					mLoadedDiscard = mFormattedImage->getDiscardLevel();
					setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
					if (mLoadedDiscard < 0)
					{
						LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
											<< ", should be >=0" << LL_ENDL;
					}
					setState(DECODE_IMAGE);
					return false;
				}
				else
				{
					LL_WARNS(LOG_TXT) << mID << " SEND_HTTP_REQ abort: cur_size " << cur_size << " <=0" << LL_ENDL;
						return true ;
				}
			}
		}
		LLPointer<AIPerService::Approvement> approved = AIPerService::approveHTTPRequestFor(mPerServicePtr, cap_texture);
		if (!approved)
		{
			return false ;
		}
		mFetcher->removeFromNetworkQueue(this, false);
		mRequestedSize = mDesiredSize;
		mRequestedDiscard = mDesiredDiscard;
		mRequestedSize -= cur_size;
		mRequestedOffset = cur_size;
		if (mRequestedOffset)
		{
			mRequestedOffset -= 1;
			mRequestedSize += 1;
		}
		if (mUrl.empty())
		{
			LL_WARNS() << "HTTP GET request failed for " << mID << LL_ENDL;
			resetFormattedData();
			++mHTTPFailCount;
			return true;
		}
		mRequestedTimer.reset();
		mLoaded = FALSE;
		mGetStatus = 0;
		mGetReason.clear();
		LL_DEBUGS(LOG_TXT) << "HTTP GET: " << mID << " Offset: " << mRequestedOffset
									 << " Bytes: " << mRequestedSize
									 << LL_ENDL;
		AIHTTPHeaders headers("Accept", "image/x-j2c");
		if (mRequestedOffset > 0 || mRequestedSize > 0)
		{
			int const range_end = mRequestedOffset + mRequestedSize - 1;
			char const* const range_format = (range_end >= HTTP_REQUESTS_RANGE_END_MAX) ? "bytes=%d-" : "bytes=%d-%d";
			headers.addHeader("Range", llformat(range_format, mRequestedOffset, range_end));
		}
		LLHTTPClient::request(mUrl, LLHTTPClient::HTTP_GET, NULL,
			new HTTPGetResponder( mFTType, mFetcher, mID, LLTimer::getTotalTime(), mRequestedSize, mRequestedOffset),
			headers, approved DEBUG_CURLIO_PARAM(debug_off), keep_alive, no_does_authentication, allow_compressed_reply, NULL, 0, NULL);
		mFetcher->addToHTTPQueue(mID);
		recordTextureStart(true);
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		setState(WAIT_HTTP_REQ);
	}
	if (mState == WAIT_HTTP_REQ)
	{
		if (mLoaded)
		{
			S32 cur_size = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
			if (mRequestedSize < 0)
			{
				S32 max_attempts;
				switch(mGetStatus)
				{
#define HTTP_CASE(name) case name: LL_DEBUGS("TexDebug") << mID << " status = " << mGetStatus << " (" << #name << ")" << " Failcount = " << mHTTPFailCount << LL_ENDL; break;
				HTTP_CASE(HTTP_CONTINUE)
				HTTP_CASE(HTTP_SWITCHING_PROTOCOLS)
				HTTP_CASE(HTTP_OK)
				HTTP_CASE(HTTP_CREATED)
				HTTP_CASE(HTTP_ACCEPTED)
				HTTP_CASE(HTTP_NON_AUTHORITATIVE_INFORMATION)
				HTTP_CASE(HTTP_NO_CONTENT)
				HTTP_CASE(HTTP_RESET_CONTENT)
				HTTP_CASE(HTTP_PARTIAL_CONTENT)
				HTTP_CASE(HTTP_MULTIPLE_CHOICES)
				HTTP_CASE(HTTP_MOVED_PERMANENTLY)
				HTTP_CASE(HTTP_FOUND)
				HTTP_CASE(HTTP_SEE_OTHER)
				HTTP_CASE(HTTP_NOT_MODIFIED)
				HTTP_CASE(HTTP_USE_PROXY)
				HTTP_CASE(HTTP_TEMPORARY_REDIRECT)
				HTTP_CASE(HTTP_BAD_REQUEST)
				HTTP_CASE(HTTP_UNAUTHORIZED)
				HTTP_CASE(HTTP_PAYMENT_REQUIRED)
				HTTP_CASE(HTTP_FORBIDDEN)
				HTTP_CASE(HTTP_NOT_FOUND)
				HTTP_CASE(HTTP_METHOD_NOT_ALLOWED)
				HTTP_CASE(HTTP_NOT_ACCEPTABLE)
				HTTP_CASE(HTTP_PROXY_AUTHENTICATION_REQUIRED)
				HTTP_CASE(HTTP_REQUEST_TIME_OUT)
				HTTP_CASE(HTTP_CONFLICT)
				HTTP_CASE(HTTP_GONE)
				HTTP_CASE(HTTP_LENGTH_REQUIRED)
				HTTP_CASE(HTTP_PRECONDITION_FAILED)
				HTTP_CASE(HTTP_REQUEST_ENTITY_TOO_LARGE)
				HTTP_CASE(HTTP_REQUEST_URI_TOO_LARGE)
				HTTP_CASE(HTTP_UNSUPPORTED_MEDIA_TYPE)
				HTTP_CASE(HTTP_REQUESTED_RANGE_NOT_SATISFIABLE)
				HTTP_CASE(HTTP_EXPECTATION_FAILED)
				HTTP_CASE(HTTP_INTERNAL_SERVER_ERROR)
				HTTP_CASE(HTTP_NOT_IMPLEMENTED)
				HTTP_CASE(HTTP_BAD_GATEWAY)
				HTTP_CASE(HTTP_SERVICE_UNAVAILABLE)
				HTTP_CASE(HTTP_GATEWAY_TIME_OUT)
				HTTP_CASE(HTTP_VERSION_NOT_SUPPORTED)
				HTTP_CASE(HTTP_INTERNAL_ERROR_LOW_SPEED)
				HTTP_CASE(HTTP_INTERNAL_ERROR_CURL_LOCKUP)
				HTTP_CASE(HTTP_INTERNAL_ERROR_CURL_BADSOCKET)
				HTTP_CASE(HTTP_INTERNAL_ERROR_CURL_TIMEOUT)
				HTTP_CASE(HTTP_INTERNAL_ERROR_CURL_OTHER)
				HTTP_CASE(HTTP_INTERNAL_ERROR_OTHER)
				default:
					LL_DEBUGS("TexDebug") << mID << " status = " << mGetStatus << " (?)" << " Failcount = " << mHTTPFailCount << LL_ENDL; break;
				}
				if (mGetStatus == HTTP_NOT_FOUND || mGetStatus == HTTP_INTERNAL_ERROR_CURL_TIMEOUT || mGetStatus == HTTP_INTERNAL_ERROR_LOW_SPEED)
				{
					mHTTPFailCount = max_attempts = 1;
					if(mGetStatus == HTTP_NOT_FOUND)
					{
						if (mFTType != FTT_MAP_TILE)
						{
							LL_WARNS(LOG_TXT) << "Texture missing from server (404): " << mUrl << LL_ENDL;
						}
						if(mWriteToCacheState == NOT_WRITE)
						{
							resetFormattedData();
							setState(DONE);
							if (mFTType != FTT_MAP_TILE)
							{
								LL_WARNS(LOG_TXT) << mID << " abort: WAIT_HTTP_REQ not found" << LL_ENDL;
							}
							return true;
						}
					}
					else if (mGetStatus == HTTP_INTERNAL_ERROR_CURL_TIMEOUT || mGetStatus == HTTP_INTERNAL_ERROR_LOW_SPEED)
					{
						if (mGetStatus == HTTP_INTERNAL_ERROR_CURL_TIMEOUT)
						{
							LL_WARNS() << "No response from server (HTTP_INTERNAL_ERROR_CURL_TIMEOUT): " << mUrl << LL_ENDL;
						}
						else
						{
							LL_WARNS() << "Slow response from server (HTTP_INTERNAL_ERROR_LOW_SPEED): " << mUrl << LL_ENDL;
						}
						SGHostBlackList::add(mUrl, 60.0, mGetStatus);
					}
					if(mCanUseNET)
					{
						LL_DEBUGS("TexDebug") << mID << " falling back to udp mSentRequest=" << mSentRequest << " mCanUseNET = " << mCanUseNET << LL_ENDL;
						resetFormattedData();
						setState(INIT);
						mCanUseHTTP = false ;
						mUrl.clear();
						setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
						LL_DEBUGS("TexDebug") << mID << " .. mSentRequest=" << mSentRequest << " mCanUseNET = " << mCanUseNET << LL_ENDL;
						return false ;
					}
					else
					{
						LL_INFOS("Texture") << mID << " aborted. no udp fallback" << LL_ENDL;
						resetFormattedData();
						return true;
					}
				}
				else if (mGetStatus == HTTP_SERVICE_UNAVAILABLE)
				{
					++mHTTPFailCount;
					max_attempts = mHTTPFailCount+1;
					LL_INFOS_ONCE("Texture") << "Texture server busy (503): " << mUrl << LL_ENDL;
				}
				else if (mGetStatus == HTTP_REQUESTED_RANGE_NOT_SATISFIABLE)
				{
					mHaveAllData = TRUE;
					max_attempts = mHTTPFailCount+1;
				}
				else
				{
					const S32 HTTP_MAX_RETRY_COUNT = 3;
					max_attempts = HTTP_MAX_RETRY_COUNT + 1;
					++mHTTPFailCount;
					LL_INFOS() << "HTTP GET failed for: " << mUrl
							<< " Status: " << mGetStatus << " Reason: '" << mGetReason << "'"
							<< " Attempt:" << mHTTPFailCount+1 << "/" << max_attempts << LL_ENDL;
				}
				if (mHTTPFailCount >= max_attempts)
				{
					if (mFTType != FTT_SERVER_BAKE)
					{
						mUrl.clear();
					}
					if (cur_size > 0 && (mHTTPFailCount < (max_attempts+1)) )
					{
						mLoadedDiscard = mFormattedImage->getDiscardLevel();
						setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
						if (mLoadedDiscard < 0)
						{
							LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
											  << ", should be >=0" << LL_ENDL;
						}
						setState(DECODE_IMAGE);
						return false;
					}
					else
					{
    					if(mCanUseNET)
    					{
							LL_DEBUGS("TexDebug") << mID << " falling back to udp (2)" << LL_ENDL;
							resetFormattedData();
							setState(INIT);
    						mCanUseHTTP = false ;
    						setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
    						return false ;
    					}
    					else
    					{
    						resetFormattedData();
                            setState(DONE);
							LL_WARNS(LOG_TXT) <<  mID << " abort: fail harder" << LL_ENDL;
    						return true;
    					}
					}
				}
				else
				{
					setState(SEND_HTTP_REQ);
					return false;
				}
			}
			if(mWriteToCacheState != NOT_WRITE && mFTType != FTT_SERVER_BAKE)
			{
				mUrl.clear();
			}
			if(mHttpBuffer.empty())
			{
				setState(DONE);
				LL_WARNS(LOG_TXT) << mID << " abort: no data received" << LL_ENDL;
				return true;
			}
			S32 append_size(mHttpBuffer.size());
			S32 total_size(cur_size + append_size);
			S32 src_offset(0);
			llassert_always(append_size == mRequestedSize);
			if (mHttpReplyOffset && mHttpReplyOffset != cur_size)
			{
				if ((S32)mHttpReplyOffset > cur_size)
				{
					LL_WARNS(LOG_TXT) << "Partial HTTP response produces break in image data for texture "
										<< mID << ".  Aborting load."  << LL_ENDL;
					setState(DONE);
					return true;
				}
				src_offset = cur_size - mHttpReplyOffset;
				append_size -= src_offset;
				total_size -= src_offset;
				mRequestedSize -= src_offset;
				mRequestedOffset += src_offset;
			}
			if (mFormattedImage.isNull())
			{
				std::string extension = gDirUtilp->getExtension(mUrl);
				mFormattedImage = LLImageFormatted::createFromType(LLImageBase::getCodecFromExtension(extension));
				if (mFormattedImage.isNull())
				{
					mFormattedImage = new LLImageJ2C;
				}
			}
			if (mHaveAllData)
			{
				mFileSize = total_size;
			}
			else
			{
				mFileSize = total_size + 1 ;
			}
			U8* buffer = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), total_size);
			if (cur_size > 0)
			{
				memcpy(buffer, mFormattedImage->getData(), cur_size);
			}
			if (append_size > 0)
			{
				memcpy(buffer + cur_size, &mHttpBuffer[src_offset], append_size);
			}
			mFormattedImage->setData(buffer, total_size);
			std::vector<U8>().swap(mHttpBuffer);
			mHttpReplySize = 0;
			mHttpReplyOffset = 0;
			mLoadedDiscard = mRequestedDiscard;
			if (mLoadedDiscard < 0)
			{
				LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
									<< ", should be >=0" << LL_ENDL;
			}
			setState(DECODE_IMAGE);
			if(mWriteToCacheState != NOT_WRITE)
			{
				mWriteToCacheState = SHOULD_WRITE ;
			}
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			return false;
		}
		else
		{
			setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
			return false;
		}
	}
	if (mState == DECODE_IMAGE)
	{
		static LLCachedControl<bool> textures_decode_disabled(gSavedSettings,"TextureDecodeDisabled");
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		if (textures_decode_disabled)
		{
			setState(DONE);
			return true;
		}
		if (mDesiredDiscard < 0)
		{
			setState(DONE);
			LL_DEBUGS(LOG_TXT) << mID << " DECODE_IMAGE abort: desired discard " << mDesiredDiscard << "<0" << LL_ENDL;
			return true;
		}
		if (mFormattedImage->getDataSize() <= 0)
		{
			LL_WARNS(LOG_TXT) << "Decode entered with invalid mFormattedImage. ID = " << mID << LL_ENDL;
			setState(DONE);
			LL_DEBUGS(LOG_TXT) << mID << " DECODE_IMAGE abort: (mFormattedImage->getDataSize() <= 0)" << LL_ENDL;
			return true;
		}
		if (mLoadedDiscard < 0)
		{
			LL_WARNS(LOG_TXT) << "Decode entered with invalid mLoadedDiscard. ID = " << mID << LL_ENDL;
			setState(DONE);
			LL_DEBUGS(LOG_TXT) << mID << " DECODE_IMAGE abort: mLoadedDiscard < 0" << LL_ENDL;
			return true;
		}
		mRawImage = NULL;
		mAuxImage = NULL;
		llassert_always(mFormattedImage.notNull());
		S32 discard = mHaveAllData ? 0 : mLoadedDiscard;
		U32 image_priority = LLWorkerThread::PRIORITY_NORMAL | mWorkPriority;
		mDecoded  = FALSE;
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		setState(DECODE_IMAGE_UPDATE);
		LL_DEBUGS(LOG_TXT) << mID << ": Decoding. Bytes: " << mFormattedImage->getDataSize() << " Discard: " << discard
				<< " All Data: " << mHaveAllData << LL_ENDL;
		mDecodeHandle = mFetcher->mImageDecodeThread->decodeImage(mFormattedImage, image_priority, discard, mNeedsAux,
																  new DecodeResponder(mFetcher, mID, this));
	}
	if (mState == DECODE_IMAGE_UPDATE)
	{
		if (mDecoded)
		{
			if (mDecodedDiscard < 0)
			{
				LL_DEBUGS(LOG_TXT) << mID << ": Failed to Decode." << LL_ENDL;
				if (mCachedSize > 0 && !mInLocalCache && mRetryAttempt == 0)
				{
 					LL_WARNS(LOG_TXT) << mID << ": Decode of cached file failed (removed), retrying" << LL_ENDL;
					llassert_always(mDecodeHandle == 0);
					mFormattedImage = NULL;
					++mRetryAttempt;
					setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
					setState(INIT);
					return false;
				}
				else
				{
					setState(DONE);
				}
			}
			else
			{
				llassert_always(mRawImage.notNull());
				LL_DEBUGS(LOG_TXT) << mID << ": Decoded. Discard: " << mDecodedDiscard
						<< " Raw Image: " << llformat("%dx%d",mRawImage->getWidth(),mRawImage->getHeight()) << LL_ENDL;
				setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
				setState(WRITE_TO_CACHE);
			}
		}
		else
		{
			return false;
		}
	}
	if (mState == WRITE_TO_CACHE)
	{
		if (mWriteToCacheState != SHOULD_WRITE || mFormattedImage.isNull())
		{
			setState(DONE);
			return false;
		}
		S32 datasize = mFormattedImage->getDataSize();
		if (mFileSize < datasize)
		{
			if (mHaveAllData)
			{
				mFileSize = datasize;
			}
			else
			{
				mFileSize = datasize + 1;
			}
		}
		llassert_always(datasize);
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		U32 cache_priority = mWorkPriority;
		mWritten = FALSE;
		setState(WAIT_ON_WRITE);
		++mCacheWriteCount;
		CacheWriteResponder* responder = new CacheWriteResponder(mFetcher, mID);
		mCacheWriteHandle = mFetcher->mTextureCache->writeToCache(mID, cache_priority,
																  mFormattedImage->getData(), datasize,
																  mFileSize, responder, mRawImage, mDecodedDiscard);
	}
	if (mState == WAIT_ON_WRITE)
	{
		if (writeToCacheComplete())
		{
			setState(DONE);
		}
		else
		{
			if (mDesiredDiscard < mDecodedDiscard)
			{
				mFetcher->mTextureCache->prioritizeWrite(mCacheWriteHandle);
			}
			return false;
		}
	}
	if (mState == DONE)
	{
		if (mDecodedDiscard >= 0 && mDesiredDiscard < mDecodedDiscard)
		{
			setState(INIT);
			LL_DEBUGS(LOG_TXT) << mID << " more data requested, returning to INIT: "
							   << " mDecodedDiscard " << mDecodedDiscard << ">= 0 && mDesiredDiscard " << mDesiredDiscard
							   << "<" << " mDecodedDiscard " << mDecodedDiscard << LL_ENDL;
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			return false;
		}
		else
		{
			setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
			return true;
		}
	}
	return false;
}
void LLTextureFetchWorker::endWork(S32 param, bool aborted)
{
	if (mDecodeHandle != 0)
	{
		mFetcher->mImageDecodeThread->abortRequest(mDecodeHandle, false);
		mDecodeHandle = 0;
	}
	mFormattedImage = NULL;
}
void LLTextureFetchWorker::finishWork(S32 param, bool completed)
{
	if (mCacheReadHandle != LLTextureCache::nullHandle())
	{
		mFetcher->mTextureCache->readComplete(mCacheReadHandle, true);
		mCacheReadHandle = LLTextureCache::nullHandle();
	}
	if (mCacheWriteHandle != LLTextureCache::nullHandle())
	{
		mFetcher->mTextureCache->writeComplete(mCacheWriteHandle, true);
		mCacheWriteHandle = LLTextureCache::nullHandle();
	}
}
bool LLTextureFetchWorker::deleteOK()
{
	bool delete_ok = true;
	if (mCacheReadHandle != LLTextureCache::nullHandle())
	{
		if (mFetcher->mTextureCache->readComplete(mCacheReadHandle, true))
		{
			mCacheReadHandle = LLTextureCache::nullHandle();
		}
		else
		{
			delete_ok = false;
		}
	}
	if (mCacheWriteHandle != LLTextureCache::nullHandle())
	{
		if (mFetcher->mTextureCache->writeComplete(mCacheWriteHandle))
		{
			mCacheWriteHandle = LLTextureCache::nullHandle();
		}
		else
		{
			delete_ok = false;
		}
	}
	if ((haveWork() &&
		 ((mState >= WRITE_TO_CACHE && mState <= WAIT_ON_WRITE))))
	{
		delete_ok = false;
	}
	return delete_ok;
}
void LLTextureFetchWorker::removeFromCache()
{
	if (!mInLocalCache)
	{
		mFetcher->mTextureCache->removeFromCache(mID);
	}
}
bool LLTextureFetchWorker::processSimulatorPackets()
{
	if (mFormattedImage.isNull() || mRequestedSize < 0)
	{
		llassert_always(mDecodeHandle == 0);
		mFormattedImage = NULL;
		return true;
	}
	if (mLastPacket >= mFirstPacket)
	{
		S32 buffer_size = mFormattedImage->getDataSize();
		for (S32 i = mFirstPacket; i<=mLastPacket; i++)
		{
			llassert_always(mPackets[i]);
			buffer_size += mPackets[i]->mSize;
		}
		bool have_all_data = mLastPacket >= mTotalPackets-1;
		if (mRequestedSize <= 0)
		{
			return true;
		}
		if (buffer_size >= mRequestedSize || have_all_data)
		{
			if (have_all_data)
			{
				mHaveAllData = TRUE;
			}
			S32 cur_size = mFormattedImage->getDataSize();
			if (buffer_size > cur_size)
			{
				U8* buffer = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), buffer_size);
				S32 offset = 0;
				if (cur_size > 0 && mFirstPacket > 0)
				{
					memcpy(buffer, mFormattedImage->getData(), cur_size);
					offset = cur_size;
				}
				for (S32 i=mFirstPacket; i<=mLastPacket; i++)
				{
					memcpy(buffer + offset, mPackets[i]->mData, mPackets[i]->mSize);
					offset += mPackets[i]->mSize;
				}
				mFormattedImage->setData(buffer, buffer_size);
			}
			mLoadedDiscard = mRequestedDiscard;
			return true;
		}
	}
	return false;
}
S32 LLTextureFetchWorker::callbackHttpGet(U32 offset, U32 length,
										   const LLChannelDescriptors& channels,
										   const LLHTTPClient::ResponderBase::buffer_ptr_t& buffer,
										   bool partial, bool success)
{
	S32 data_size = 0 ;
	if (mState != WAIT_HTTP_REQ)
	{
		LL_WARNS(LOG_TXT) << "callbackHttpGet for unrequested fetch worker: " << mID
				<< " req=" << mSentRequest << " state= " << mState << LL_ENDL;
		return data_size;
	}
	if (mLoaded)
	{
		LL_WARNS(LOG_TXT) << "Duplicate callback for " << mID.asString() << LL_ENDL;
		return data_size;
	}
	if (success)
	{
		data_size = buffer->countAfter(channels.in(), NULL);
		LL_DEBUGS(LOG_TXT) << "HTTP RECEIVED: " << mID.asString() << " Bytes: " << data_size << LL_ENDL;
		if (data_size > 0)
		{
			LLViewerStatsRecorder::instance().textureFetch(data_size);
			llassert(mHttpBuffer.empty());
			mHttpBuffer.resize(data_size);
			buffer->readAfter(channels.in(), NULL, &mHttpBuffer[0], data_size);
			if (partial)
			{
				if (! offset && ! length)
				{
					mHttpReplySize = data_size;
					mHttpReplyOffset = mRequestedOffset;
				}
				else
				{
					mHttpReplySize = length;
					mHttpReplyOffset = offset;
				}
			}
			if (! partial)
			{
				if (data_size <= mRequestedOffset)
				{
					LL_WARNS(LOG_TXT) << "Fetched entire texture " << mID
										<< " when it was expected to be marked complete.  mImageSize:  "
										<< mFileSize << " datasize:  " << mFormattedImage->getDataSize()
										<< LL_ENDL;
				}
				mHaveAllData = TRUE;
				llassert_always(mDecodeHandle == 0);
				mFormattedImage = NULL;
			}
			else if (data_size < mRequestedSize)
			{
				mHaveAllData = TRUE;
			}
			else if (data_size > mRequestedSize)
			{
				LL_WARNS(LOG_TXT) << "data_size = " << data_size << " > requested: " << mRequestedSize << LL_ENDL;
				mHaveAllData = TRUE;
				llassert_always(mDecodeHandle == 0);
				mFormattedImage = NULL;
			}
		}
		else
		{
			mHaveAllData = TRUE;
		}
		mRequestedSize = data_size;
	}
	else
	{
		mRequestedSize = -1;
	}
	mLoaded = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
	LLViewerStatsRecorder::instance().log(0.2f);
	return data_size ;
}
void LLTextureFetchWorker::callbackCacheRead(bool success, LLImageFormatted* image,
											 S32 imagesize, BOOL islocal)
{
	LLMutexLock lock(&mWorkMutex);
	if (mState != LOAD_FROM_TEXTURE_CACHE)
	{
		return;
	}
	if (success)
	{
		llassert_always(imagesize >= 0);
		mFileSize = imagesize;
		mFormattedImage = image;
		mImageCodec = image->getCodec();
		mInLocalCache = islocal;
		if (mFileSize != 0 && mFormattedImage->getDataSize() >= mFileSize)
		{
			mHaveAllData = TRUE;
		}
	}
	mLoaded = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
}
void LLTextureFetchWorker::callbackCacheWrite(bool success)
{
	LLMutexLock lock(&mWorkMutex);
	if (mState != WAIT_ON_WRITE)
	{
		return;
	}
	mWritten = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
}
void LLTextureFetchWorker::callbackDecoded(bool success, LLImageRaw* raw, LLImageRaw* aux)
{
	LLMutexLock lock(&mWorkMutex);
	if (mDecodeHandle == 0)
	{
		return;
	}
	if (mState != DECODE_IMAGE_UPDATE)
	{
		mDecodeHandle = 0;
		return;
	}
	llassert_always(mFormattedImage.notNull());
	mDecodeHandle = 0;
	if (success)
	{
		llassert_always(raw);
		mRawImage = raw;
		mAuxImage = aux;
		mDecodedDiscard = mFormattedImage->getDiscardLevel();
 		LL_DEBUGS(LOG_TXT) << mID << ": Decode Finished. Discard: " << mDecodedDiscard
							 << " Raw Image: " << llformat("%dx%d",mRawImage->getWidth(),mRawImage->getHeight()) << LL_ENDL;
	}
	else
	{
		if (mFormattedImage.notNull())
		{
			LL_WARNS(LOG_TXT) << "DECODE FAILED: id = " << mID << ", Discard = " << (S32)mFormattedImage->getDiscardLevel() << LL_ENDL;
		}
		else
		{
			LL_WARNS(LOG_TXT) << "DECODE FAILED: id = " << mID << ", mFormattedImage is Null!" << LL_ENDL;
		}
		removeFromCache();
		mDecodedDiscard = -1;
	}
	mDecoded = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
	mCacheReadTime = mCacheReadTimer.getElapsedTimeF32();
}
bool LLTextureFetchWorker::writeToCacheComplete()
{
	if (mCacheWriteHandle != LLTextureCache::nullHandle())
	{
		if (!mWritten)
		{
			return false;
		}
		if (mFetcher->mTextureCache->writeComplete(mCacheWriteHandle))
		{
			mCacheWriteHandle = LLTextureCache::nullHandle();
		}
		else
		{
			return false;
		}
	}
	return true;
}
void LLTextureFetchWorker::recordTextureStart(bool is_http)
{
	if (! mMetricsStartTime.value())
	{
		mMetricsStartTime = LLViewerAssetStatsFF::get_timestamp();
	}
	LLViewerAssetStatsFF::record_enqueue_thread1(LLViewerAssetType::AT_TEXTURE,
												 is_http,
												 LLImageBase::TYPE_AVATAR_BAKE == mType);
}
void LLTextureFetchWorker::recordTextureDone(bool is_http)
{
	if (mMetricsStartTime.value())
	{
		LLViewerAssetStatsFF::record_response_thread1(LLViewerAssetType::AT_TEXTURE,
													  is_http,
													  LLImageBase::TYPE_AVATAR_BAKE == mType,
													  LLViewerAssetStatsFF::get_timestamp() - mMetricsStartTime);
		mMetricsStartTime = (U32Seconds)0;
	}
	LLViewerAssetStatsFF::record_dequeue_thread1(LLViewerAssetType::AT_TEXTURE,
												 is_http,
												 LLImageBase::TYPE_AVATAR_BAKE == mType);
}
LLTextureFetch::LLTextureFetch(LLTextureCache* cache, LLImageDecodeThread* imagedecodethread, bool threaded, bool qa_mode)
	: LLWorkerThread("TextureFetch", threaded, true),
	  mDebugCount(0),
	  mDebugPause(FALSE),
	  mPacketCount(0),
	  mBadPacketCount(0),
	  mTextureCache(cache),
	  mImageDecodeThread(imagedecodethread),
	  mTotalHTTPRequests(0),
	  mQAMode(qa_mode),
	  mTotalCacheReadCount(0U),
	  mTotalCacheWriteCount(0U),
	  mHttpOkCount(0),
	  mHttpPartialCount(0),
	  mHttpForbiddenCount(0),
	  mHttpNotFoundCount(0),
	  mHttpOtherFailCount(0),
	  mHttpMapFailCount(0),
	  mHttpTextureBytes(0)
{
	mTextureInfo.setUpLogging(gSavedSettings.getBOOL("LogTextureDownloadsToViewerLog"), gSavedSettings.getBOOL("LogTextureDownloadsToSimulator"), U32Bytes(gSavedSettings.getU32("TextureLoggingThreshold")));
}
LLTextureFetch::~LLTextureFetch()
{
	clearDeleteList() ;
	while (! mCommands.empty())
	{
		TFRequest * req(mCommands.front());
		mCommands.pop_front();
		delete req;
	}
}
bool LLTextureFetch::createRequest(FTType f_type, const std::string& url, const LLUUID& id, const LLHost& host, F32 priority,
								   S32 w, S32 h, S32 c, S32 desired_discard, bool needs_aux, bool can_use_http)
{
	if (mDebugPause)
	{
		return false;
	}
	if (f_type == FTT_SERVER_BAKE)
	{
		LL_DEBUGS("Avatar") << " requesting " << id << " " << w << "x" << h << " discard " << desired_discard << " type " << f_type << LL_ENDL;
	}
	LLTextureFetchWorker* worker = getWorker(id) ;
	if (worker)
	{
		if (worker->mHost != host)
		{
			LL_WARNS(LOG_TXT) << "LLTextureFetch::createRequest " << id << " called with multiple hosts: "
					<< host << " != " << worker->mHost << LL_ENDL;
			removeRequest(worker, true);
			worker = NULL;
			return false;
		}
	}
	S32 desired_size;
	std::string exten = gDirUtilp->getExtension(url);
    if ((f_type == FTT_SERVER_BAKE) && !url.empty() && !exten.empty() && (LLImageBase::getCodecFromExtension(exten) != IMG_CODEC_J2C))
	{
		llassert(!url.empty() && (!exten.empty() && LLImageBase::getCodecFromExtension(exten) != IMG_CODEC_J2C));
		LL_DEBUGS(LOG_TXT) << "full request for " << id << " texture is FTT_SERVER_BAKE" << LL_ENDL;
		desired_size = MAX_IMAGE_DATA_SIZE;
		desired_discard = 0;
	}
	else if (!url.empty() && (!exten.empty() && LLImageBase::getCodecFromExtension(exten) != IMG_CODEC_J2C))
	{
		LL_DEBUGS(LOG_TXT) << "full request for " << id << " exten is not J2C: " << exten << LL_ENDL;
		desired_size = MAX_IMAGE_DATA_SIZE;
		desired_discard = 0;
	}
	else if (desired_discard == 0)
	{
		desired_size = MAX_IMAGE_DATA_SIZE;
	}
	else if (w*h*c > 0)
	{
		desired_size = LLImageJ2C::calcDataSizeJ2C(w, h, c, desired_discard);
	}
	else
	{
		desired_size = TEXTURE_CACHE_ENTRY_SIZE;
		desired_discard = MAX_DISCARD_LEVEL;
	}
	if (worker)
	{
		if (worker->wasAborted())
		{
			return false;
		}
		worker->lockWorkMutex();
		worker->mActiveCount++;
		worker->mNeedsAux = needs_aux;
		worker->setImagePriority(priority);
		worker->setDesiredDiscard(desired_discard, desired_size);
		worker->setCanUseHTTP(can_use_http) ;
		if (!worker->haveWork())
		{
			worker->setState(LLTextureFetchWorker::INIT);
			worker->unlockWorkMutex();
			worker->addWork(0, LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
		}
		else
		{
			worker->unlockWorkMutex();
		}
	}
	else
	{
		worker = new LLTextureFetchWorker(this, f_type, url, id, host, priority, desired_discard, desired_size);
		lockQueue() ;
		mRequestMap[id] = worker;
		unlockQueue() ;
		worker->lockWorkMutex();
		worker->mActiveCount++;
		worker->mNeedsAux = needs_aux;
		worker->setCanUseHTTP(can_use_http) ;
		worker->unlockWorkMutex();
	}
 	LL_DEBUGS(LOG_TXT) << "REQUESTED: " << id << " f_type " << fttype_to_string(f_type)
					   << " Discard: " << desired_discard << " size " << desired_size << LL_ENDL;
	return true;
}
void LLTextureFetch::addToNetworkQueue(LLTextureFetchWorker* worker)
{
	lockQueue();
	bool in_request_map = (mRequestMap.find(worker->mID) != mRequestMap.end());
	unlockQueue();
	LLMutexLock lock(&mNetworkQueueMutex);
	if (in_request_map)
	{
		mNetworkQueue.insert(worker->mID);
	}
	for (cancel_queue_t::iterator iter1 = mCancelQueue.begin();
		 iter1 != mCancelQueue.end(); ++iter1)
	{
		iter1->second.erase(worker->mID);
	}
}
void LLTextureFetch::removeFromNetworkQueue(LLTextureFetchWorker* worker, bool cancel)
{
	LLMutexLock lock(&mNetworkQueueMutex);
	size_t erased = mNetworkQueue.erase(worker->mID);
	if (cancel && erased > 0)
	{
		mCancelQueue[worker->mHost].insert(worker->mID);
	}
}
void LLTextureFetch::addToHTTPQueue(const LLUUID& id)
{
	LLMutexLock lock(&mNetworkQueueMutex);
	mHTTPTextureQueue.insert(id);
	mTotalHTTPRequests++;
}
void LLTextureFetch::removeFromHTTPQueue(const LLUUID& id, S32 received_size)
{
	LLMutexLock lock(&mNetworkQueueMutex);
	mHTTPTextureQueue.erase(id);
}
void LLTextureFetch::deleteRequest(const LLUUID& id, bool cancel)
{
	lockQueue() ;
	LLTextureFetchWorker* worker = getWorkerAfterLock(id);
	removeRequest(worker, cancel, false);
}
void LLTextureFetch::removeRequest(LLTextureFetchWorker* worker, bool cancel, bool bNeedsLock)
{
	if(!worker)
	{
		if(!bNeedsLock)
			unlockQueue() ;
		return;
	}
	if(bNeedsLock)
		lockQueue() ;
	size_t erased_1 = mRequestMap.erase(worker->mID);
	unlockQueue() ;
	llassert_always(erased_1 > 0) ;
	removeFromNetworkQueue(worker, cancel);
	llassert_always(!(worker->getFlags(LLWorkerClass::WCF_DELETE_REQUESTED))) ;
	worker->scheduleDelete();
}
void LLTextureFetch::deleteAllRequests()
{
	while(1)
	{
		lockQueue();
		if(mRequestMap.empty())
		{
			unlockQueue() ;
			break;
		}
		LLTextureFetchWorker* worker = mRequestMap.begin()->second;
		removeRequest(worker, true, false);
	}
}
S32 LLTextureFetch::getNumRequests()
{
	lockQueue() ;
	S32 size = (S32)mRequestMap.size();
	unlockQueue() ;
	return size ;
}
S32 LLTextureFetch::getNumHTTPRequests()
{
	mNetworkQueueMutex.lock();
	S32 size = (S32)mHTTPTextureQueue.size();
	mNetworkQueueMutex.unlock();
	return size;
}
U32 LLTextureFetch::getTotalNumHTTPRequests()
{
	mNetworkQueueMutex.lock() ;
	U32 size = mTotalHTTPRequests ;
	mNetworkQueueMutex.unlock() ;
	return size ;
}
LLTextureFetchWorker* LLTextureFetch::getWorkerAfterLock(const LLUUID& id)
{
	LLTextureFetchWorker* res = NULL;
	map_t::iterator iter = mRequestMap.find(id);
	if (iter != mRequestMap.end())
	{
		res = iter->second;
	}
	return res;
}
LLTextureFetchWorker* LLTextureFetch::getWorker(const LLUUID& id)
{
	LLMutexLock lock(&mQueueMutex) ;
	return getWorkerAfterLock(id) ;
}
bool LLTextureFetch::getRequestFinished(const LLUUID& id, S32& discard_level,
										LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux)
{
	bool res = false;
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker)
	{
		if (worker->wasAborted())
		{
			res = true;
		}
		else if (!worker->haveWork())
		{
			if (!mDebugPause)
			{
				worker->addWork(0, LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
			}
		}
		else if (worker->checkWork())
		{
			worker->lockWorkMutex();
			discard_level = worker->mDecodedDiscard;
			raw = worker->mRawImage;
			aux = worker->mAuxImage;
			F32 cache_read_time = worker->mCacheReadTime;
			if (cache_read_time != 0.f)
			{
				sCacheReadLatency.addValue(cache_read_time * 1000.f);
			}
			res = true;
			LL_DEBUGS("Texture") << id << ": Request Finished. State: " << worker->mState << " Discard: " << discard_level << LL_ENDL;
			worker->unlockWorkMutex();
		}
		else
		{
			worker->lockWorkMutex();
			if ((worker->mDecodedDiscard >= 0) &&
				(worker->mDecodedDiscard < discard_level || discard_level < 0) &&
				(worker->mState >= LLTextureFetchWorker::WAIT_ON_WRITE))
			{
				discard_level = worker->mDecodedDiscard;
				raw = worker->mRawImage;
				aux = worker->mAuxImage;
			}
			worker->unlockWorkMutex();
		}
	}
	else
	{
		res = true;
	}
	return res;
}
bool LLTextureFetch::updateRequestPriority(const LLUUID& id, F32 priority)
{
	bool res = false;
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker)
	{
		worker->lockWorkMutex();
		worker->setImagePriority(priority);
		worker->unlockWorkMutex();
		res = true;
	}
	return res;
}
S32 LLTextureFetch::getPending()
{
	S32 res;
	lockData();
    {
        LLMutexLock lock(&mQueueMutex);
        res = mRequestQueue.size();
        res += mCommands.size();
    }
	unlockData();
	return res;
}
bool LLTextureFetch::runCondition()
{
	bool have_no_commands(false);
	{
		LLMutexLock lock(&mQueueMutex);
		have_no_commands = mCommands.empty();
	}
	return ! (have_no_commands
			  && (mRequestQueue.empty() && mIdleThread));
}
void LLTextureFetch::commonUpdate()
{
	cmdDoWork();
}
S32 LLTextureFetch::update(F32 max_time_ms)
{
	S32 res = LLWorkerThread::update(max_time_ms);
	if (!mDebugPause)
	{
		if (LLStartUp::getStartupState() > STATE_AGENT_SEND)
		{
			sendRequestListToSimulators();
		}
	}
	if (!mThreaded)
	{
		commonUpdate();
	}
	return res;
}
void LLTextureFetch::shutDownTextureCacheThread()
{
	if(mTextureCache)
	{
		llassert_always(mTextureCache->isQuitting() || mTextureCache->isStopped()) ;
		mTextureCache = NULL ;
	}
}
void LLTextureFetch::shutDownImageDecodeThread()
{
	if(mImageDecodeThread)
	{
		llassert_always(mImageDecodeThread->isQuitting() || mImageDecodeThread->isStopped()) ;
		mImageDecodeThread = NULL ;
	}
}
void LLTextureFetch::startThread()
{
}
void LLTextureFetch::endThread()
{
	LL_INFOS(LOG_TXT) << "CacheReads:  " << mTotalCacheReadCount
					  << ", CacheWrites:  " << mTotalCacheWriteCount
					  << ", TotalHTTPReq:  " << getTotalNumHTTPRequests()
					  << ", HTTP ok/206/403/404/other/mapfail: "
					  << (U32)mHttpOkCount << "/" << (U32)mHttpPartialCount << "/"
					  << (U32)mHttpForbiddenCount << "/" << (U32)mHttpNotFoundCount << "/"
					  << (U32)mHttpOtherFailCount << "/" << (U32)mHttpMapFailCount
					  << LL_ENDL;
}
void LLTextureFetch::recordHttpResult(S32 status, bool is_map_tile, S32 received_bytes)
{
	if (status == HTTP_OK)
	{
		mHttpOkCount++;
		if (received_bytes > 0 && !is_map_tile)
		{
			mHttpTextureBytes += (U32)received_bytes;
		}
		return;
	}
	if (status == HTTP_PARTIAL_CONTENT)
	{
		mHttpPartialCount++;
		if (received_bytes > 0 && !is_map_tile)
		{
			mHttpTextureBytes += (U32)received_bytes;
		}
		return;
	}
	if (is_map_tile)
	{
		mHttpMapFailCount++;
		return;
	}
	if (status == HTTP_FORBIDDEN)
	{
		mHttpForbiddenCount++;
		return;
	}
	if (status == HTTP_NOT_FOUND)
	{
		mHttpNotFoundCount++;
		return;
	}
	mHttpOtherFailCount++;
}
void LLTextureFetch::threadedUpdate()
{
	const F32 PROCESS_TIME = 0.05f;
	static LLFrameTimer process_timer;
	if (process_timer.getElapsedTimeF32() < PROCESS_TIME)
	{
		return;
	}
	process_timer.reset();
	commonUpdate();
#if 0
	const F32 INFO_TIME = 1.0f;
	static LLFrameTimer info_timer;
	if (info_timer.getElapsedTimeF32() >= INFO_TIME)
	{
		S32 q = mCurlGetRequest->getQueued();
		if (q > 0)
		{
			LL_INFOS(LOG_TXT) << "Queued gets: " << q << LL_ENDL;
			info_timer.reset();
		}
	}
#endif
}
void LLTextureFetch::sendRequestListToSimulators()
{
	const F32 REQUEST_DELTA_TIME = 0.10f;
	const S32 IMAGES_PER_REQUEST = 50;
	const F32 SIM_LAZY_FLUSH_TIMEOUT = 10.0f;
	const F32 MIN_REQUEST_TIME = 1.0f;
	const F32 MIN_DELTA_PRIORITY = 1000.f;
	static LLFrameTimer timer;
	if (timer.getElapsedTimeF32() < REQUEST_DELTA_TIME)
	{
		return;
	}
	timer.reset();
	typedef std::set<LLTextureFetchWorker*,LLTextureFetchWorker::Compare> request_list_t;
	typedef std::map< LLHost, request_list_t > work_request_map_t;
	work_request_map_t requests;
	{
	LLMutexLock lock2(&mNetworkQueueMutex);
	for (queue_t::iterator iter = mNetworkQueue.begin(); iter != mNetworkQueue.end(); )
	{
		queue_t::iterator curiter = iter++;
		LLTextureFetchWorker* req = getWorker(*curiter);
		if (!req)
		{
			mNetworkQueue.erase(curiter);
			continue;
		}
		if ((req->mState != LLTextureFetchWorker::SEND_UDP_REQ) &&
			(req->mState != LLTextureFetchWorker::WAIT_UDP_REQ) &&
			(req->mState != LLTextureFetchWorker::LOAD_FROM_SIMULATOR))
		{
				LL_WARNS(LOG_TXT) << "Worker: " << req->mID << " in mNetworkQueue but in wrong state: " << req->mState << LL_ENDL;
			mNetworkQueue.erase(curiter);
			continue;
		}
		if (req->mID == mDebugID)
		{
			mDebugCount++;
		}
		if (req->mSentRequest == LLTextureFetchWorker::SENT_SIM &&
			req->mTotalPackets > 0 &&
			req->mLastPacket >= req->mTotalPackets-1)
		{
			continue;
		}
		F32 elapsed = req->mRequestedTimer.getElapsedTimeF32();
		{
			F32 delta_priority = llabs(req->mRequestedPriority - req->mImagePriority);
			if ((req->mSimRequestedDiscard != req->mDesiredDiscard) ||
				(delta_priority > MIN_DELTA_PRIORITY && elapsed >= MIN_REQUEST_TIME) ||
				(elapsed >= SIM_LAZY_FLUSH_TIMEOUT))
			{
				requests[req->mHost].insert(req);
			}
		}
	}
	}
	for (work_request_map_t::iterator iter1 = requests.begin();
		 iter1 != requests.end(); ++iter1)
	{
		LLHost host = iter1->first;
		if (host == LLHost::invalid)
		{
			host = gAgent.getRegionHost();
		}
		S32 sim_request_count = 0;
		for (request_list_t::iterator iter2 = iter1->second.begin();
			 iter2 != iter1->second.end(); ++iter2)
		{
			LLTextureFetchWorker* req = *iter2;
			if (gMessageSystem)
			{
				if (req->mSentRequest != LLTextureFetchWorker::SENT_SIM)
				{
					req->lockWorkMutex();
					req->setupPacketData();
					req->unlockWorkMutex();
				}
				if (0 == sim_request_count)
				{
					gMessageSystem->newMessageFast(_PREHASH_RequestImage);
					gMessageSystem->nextBlockFast(_PREHASH_AgentData);
					gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				}
				S32 packet = req->mLastPacket + 1;
				gMessageSystem->nextBlockFast(_PREHASH_RequestImage);
				gMessageSystem->addUUIDFast(_PREHASH_Image, req->mID);
				gMessageSystem->addS8Fast(_PREHASH_DiscardLevel, (S8)req->mDesiredDiscard);
				gMessageSystem->addF32Fast(_PREHASH_DownloadPriority, req->mImagePriority);
				gMessageSystem->addU32Fast(_PREHASH_Packet, packet);
				gMessageSystem->addU8Fast(_PREHASH_Type, req->mType);
				static LLCachedControl<bool> log_to_viewer_log(gSavedSettings,"LogTextureDownloadsToViewerLog");
				static LLCachedControl<bool> log_to_sim(gSavedSettings,"LogTextureDownloadsToSimulator");
				if (log_to_viewer_log || log_to_sim)
				{
					mTextureInfo.setRequestStartTime(req->mID, LLTimer::getTotalTime());
					mTextureInfo.setRequestOffset(req->mID, 0);
					mTextureInfo.setRequestSize(req->mID, 0);
					mTextureInfo.setRequestType(req->mID, LLTextureInfoDetails::REQUEST_TYPE_UDP);
				}
				req->lockWorkMutex();
				req->mSentRequest = LLTextureFetchWorker::SENT_SIM;
				req->mSimRequestedDiscard = req->mDesiredDiscard;
				req->mRequestedPriority = req->mImagePriority;
				req->mRequestedTimer.reset();
				req->unlockWorkMutex();
				sim_request_count++;
				if (sim_request_count >= IMAGES_PER_REQUEST)
				{
					gMessageSystem->sendSemiReliable(host, NULL, NULL);
					sim_request_count = 0;
				}
			}
		}
		if (gMessageSystem && sim_request_count > 0 && sim_request_count < IMAGES_PER_REQUEST)
		{
			gMessageSystem->sendSemiReliable(host, NULL, NULL);
			sim_request_count = 0;
		}
	}
	{
	LLMutexLock lock2(&mNetworkQueueMutex);
	if (gMessageSystem && !mCancelQueue.empty())
	{
		for (cancel_queue_t::iterator iter1 = mCancelQueue.begin();
			 iter1 != mCancelQueue.end(); ++iter1)
		{
			LLHost host = iter1->first;
			if (host == LLHost::invalid)
			{
				host = gAgent.getRegionHost();
			}
			S32 request_count = 0;
			for (queue_t::iterator iter2 = iter1->second.begin();
				 iter2 != iter1->second.end(); ++iter2)
			{
				if (0 == request_count)
				{
					gMessageSystem->newMessageFast(_PREHASH_RequestImage);
					gMessageSystem->nextBlockFast(_PREHASH_AgentData);
					gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				}
				gMessageSystem->nextBlockFast(_PREHASH_RequestImage);
				gMessageSystem->addUUIDFast(_PREHASH_Image, *iter2);
				gMessageSystem->addS8Fast(_PREHASH_DiscardLevel, -1);
				gMessageSystem->addF32Fast(_PREHASH_DownloadPriority, 0);
				gMessageSystem->addU32Fast(_PREHASH_Packet, 0);
				gMessageSystem->addU8Fast(_PREHASH_Type, 0);
				request_count++;
				if (request_count >= IMAGES_PER_REQUEST)
				{
					gMessageSystem->sendSemiReliable(host, NULL, NULL);
					request_count = 0;
				}
			}
			if (request_count > 0 && request_count < IMAGES_PER_REQUEST)
			{
				gMessageSystem->sendSemiReliable(host, NULL, NULL);
			}
		}
		mCancelQueue.clear();
	}
	}
}
bool LLTextureFetchWorker::insertPacket(S32 index, U8* data, S32 size)
{
	mRequestedTimer.reset();
	if (index >= mTotalPackets)
	{
		return false;
	}
	if (index > 0 && index < mTotalPackets-1 && size != MAX_IMG_PACKET_SIZE)
	{
		return false;
	}
	if (index >= (S32)mPackets.size())
	{
		mPackets.resize(index+1, (PacketData*)NULL);
	}
	else if (mPackets[index] != NULL)
	{
		return false;
	}
	mPackets[index] = new PacketData(data, size);
	while (mLastPacket+1 < (S32)mPackets.size() && mPackets[mLastPacket+1] != NULL)
	{
		++mLastPacket;
	}
	return true;
}
void LLTextureFetchWorker::setState(e_state new_state)
{
	mState = new_state;
}
bool LLTextureFetch::receiveImageHeader(const LLHost& host, const LLUUID& id, U8 codec, U16 packets, U32 totalbytes,
										U16 data_size, U8* data)
{
	LLTextureFetchWorker* worker = getWorker(id);
	bool res = true;
	++mPacketCount;
	if (!worker)
	{
		res = false;
	}
	else if (worker->mState != LLTextureFetchWorker::WAIT_UDP_REQ ||
			 worker->mSentRequest != LLTextureFetchWorker::SENT_SIM)
	{
		res = false;
	}
	else if (worker->mLastPacket != -1)
	{
		res = false;
	}
	else if (!data_size)
	{
		res = false;
	}
	if (!res)
	{
		mNetworkQueueMutex.lock();
		++mBadPacketCount;
		mCancelQueue[host].insert(id);
		mNetworkQueueMutex.unlock();
		return false;
	}
	LLViewerStatsRecorder::instance().textureFetch(data_size);
	LLViewerStatsRecorder::instance().log(0.1f);
	worker->lockWorkMutex();
	worker->mImageCodec = codec;
	worker->mTotalPackets = packets;
	worker->mFileSize = (S32)totalbytes;
	llassert_always(totalbytes > 0);
	llassert_always(data_size == FIRST_PACKET_SIZE || data_size == worker->mFileSize);
	res = worker->insertPacket(0, data, data_size);
	worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
	worker->setState(LLTextureFetchWorker::LOAD_FROM_SIMULATOR);
	worker->unlockWorkMutex();
	return res;
}
bool LLTextureFetch::receiveImagePacket(const LLHost& host, const LLUUID& id, U16 packet_num, U16 data_size, U8* data)
{
	LLTextureFetchWorker* worker = getWorker(id);
	bool res = true;
	++mPacketCount;
	if (!worker)
	{
		res = false;
	}
	else if (worker->mLastPacket == -1)
	{
		res = false;
	}
	else if (!data_size)
	{
		res = false;
	}
	if (!res)
	{
		mNetworkQueueMutex.lock();
		++mBadPacketCount;
		mCancelQueue[host].insert(id);
		mNetworkQueueMutex.unlock();
		return false;
	}
	LLViewerStatsRecorder::instance().textureFetch(data_size);
	LLViewerStatsRecorder::instance().log(0.1f);
	worker->lockWorkMutex();
	res = worker->insertPacket(packet_num, data, data_size);
	if ((worker->mState == LLTextureFetchWorker::LOAD_FROM_SIMULATOR) ||
		(worker->mState == LLTextureFetchWorker::WAIT_UDP_REQ))
	{
		worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
		worker->setState(LLTextureFetchWorker::LOAD_FROM_SIMULATOR);
	}
	else
	{
		removeFromNetworkQueue(worker, true);
	}
	if(packet_num >= (worker->mTotalPackets - 1))
	{
		static LLCachedControl<bool> log_to_viewer_log(gSavedSettings,"LogTextureDownloadsToViewerLog");
		static LLCachedControl<bool> log_to_sim(gSavedSettings,"LogTextureDownloadsToSimulator");
		if (log_to_viewer_log || log_to_sim)
		{
			U64Microseconds timeNow = LLTimer::getTotalTime();
			mTextureInfo.setRequestSize(id, worker->mFileSize);
			mTextureInfo.setRequestCompleteTimeAndLog(id, timeNow);
		}
	}
	worker->unlockWorkMutex();
	return res;
}
BOOL LLTextureFetch::isFromLocalCache(const LLUUID& id)
{
	BOOL from_cache = FALSE;
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker)
	{
		worker->lockWorkMutex();
		from_cache = worker->mInLocalCache;
		worker->unlockWorkMutex();
	}
	return from_cache;
}
S32 LLTextureFetch::getFetchState(const LLUUID& id, F32& data_progress_p, F32& requested_priority_p,
								  U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p, bool& can_use_http)
{
	S32 state = LLTextureFetchWorker::INVALID;
	F32 data_progress = 0.0f;
	F32 requested_priority = 0.0f;
	F32 fetch_dtime = 999999.f;
	F32 request_dtime = 999999.f;
	U32 fetch_priority = 0;
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker && worker->haveWork())
	{
		worker->lockWorkMutex();
		state = worker->mState;
		fetch_dtime = worker->mFetchTimer.getElapsedTimeF32();
		request_dtime = worker->mRequestedTimer.getElapsedTimeF32();
		if (worker->mFileSize > 0)
		{
			if (state == LLTextureFetchWorker::LOAD_FROM_SIMULATOR)
			{
				S32 data_size = FIRST_PACKET_SIZE + (worker->mLastPacket-1) * MAX_IMG_PACKET_SIZE;
				data_size = llmax(data_size, 0);
				data_progress = (F32)data_size / (F32)worker->mFileSize;
			}
			else if (worker->mFormattedImage.notNull())
			{
				data_progress = (F32)worker->mFormattedImage->getDataSize() / (F32)worker->mFileSize;
			}
		}
		if (state >= LLTextureFetchWorker::LOAD_FROM_NETWORK && state <= LLTextureFetchWorker::WAIT_HTTP_REQ)
		{
			requested_priority = worker->mRequestedPriority;
		}
		else
		{
			requested_priority = worker->mImagePriority;
		}
		fetch_priority = worker->getPriority();
		can_use_http = worker->getCanUseHTTP() ;
		worker->unlockWorkMutex();
	}
	data_progress_p = data_progress;
	requested_priority_p = requested_priority;
	fetch_priority_p = fetch_priority;
	fetch_dtime_p = fetch_dtime;
	request_dtime_p = request_dtime;
	return state;
}
void LLTextureFetch::dump()
{
	LL_INFOS(LOG_TXT) << "LLTextureFetch REQUESTS:" << LL_ENDL;
	for (request_queue_t::iterator iter = mRequestQueue.begin();
		 iter != mRequestQueue.end(); ++iter)
	{
		LLQueuedThread::QueuedRequest* qreq = *iter;
		LLWorkerThread::WorkRequest* wreq = (LLWorkerThread::WorkRequest*)qreq;
		LLTextureFetchWorker* worker = (LLTextureFetchWorker*)wreq->getWorkerClass();
		LL_INFOS(LOG_TXT) << " ID: " << worker->mID
				<< " PRI: " << llformat("0x%08x",wreq->getPriority())
				<< " STATE: " << worker->sStateDescs[worker->mState]
				<< LL_ENDL;
	}
	LL_INFOS(LOG_TXT) << "LLTextureFetch ACTIVE_HTTP:" << LL_ENDL;
	for (queue_t::const_iterator iter(mHTTPTextureQueue.begin());
		 mHTTPTextureQueue.end() != iter;
		 ++iter)
	{
		LL_INFOS(LOG_TXT) << " ID: " << (*iter) << LL_ENDL;
	}
}
void LLTextureFetch::updateStateStats(U32 cache_read, U32 cache_write)
{
	LLMutexLock lock(&mQueueMutex);
	mTotalCacheReadCount += cache_read;
	mTotalCacheWriteCount += cache_write;
}
void LLTextureFetch::getStateStats(U32 * cache_read, U32 * cache_write)
{
	U32 ret1(0U), ret2(0U);
	{
		LLMutexLock lock(&mQueueMutex);
		ret1 = mTotalCacheReadCount;
		ret2 = mTotalCacheWriteCount;
	}
	*cache_read = ret1;
	*cache_write = ret2;
}
void LLTextureFetch::commandSetRegion(U64 region_handle)
{
	TFReqSetRegion * req = new TFReqSetRegion(region_handle);
	cmdEnqueue(req);
}
void LLTextureFetch::commandSendMetrics(const std::string & caps_url,
										const LLUUID & session_id,
										const LLUUID & agent_id,
										LLViewerAssetStats * main_stats)
{
	TFReqSendMetrics * req = new TFReqSendMetrics(caps_url, session_id, agent_id, main_stats);
	cmdEnqueue(req);
}
void LLTextureFetch::commandDataBreak()
{
	LLTextureFetch::svMetricsDataBreak = true;
}
void LLTextureFetch::cmdEnqueue(TFRequest * req)
{
	lockQueue();
	mCommands.push_back(req);
	unlockQueue();
	unpause();
}
LLTextureFetch::TFRequest * LLTextureFetch::cmdDequeue()
{
	TFRequest * ret = 0;
	lockQueue();
	if (! mCommands.empty())
	{
		ret = mCommands.front();
		mCommands.pop_front();
	}
	unlockQueue();
	return ret;
}
void LLTextureFetch::cmdDoWork()
{
	if (mDebugPause)
	{
		return;
	}
	TFRequest * req = cmdDequeue();
	if (req)
	{
		req->doWork(this);
		delete req;
	}
}
namespace
{
class AssetReportHandler : public LLHTTPClient::ResponderWithCompleted
{
public:
	virtual void httpCompleted(void)
	{
		if (mStatus)
		{
			LL_DEBUGS("Texture") << "Successfully delivered asset metrics to grid."
								<< LL_ENDL;
		}
		else
		{
			LL_WARNS("Texture") << "Error delivering asset metrics to grid.  Reason:  "
								<< mStatus << LL_ENDL;
		}
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return assetReportHandler_timeout; }
	char const* getName(void) const { return "AssetReportHandler"; }
};
bool
TFReqSetRegion::doWork(LLTextureFetch *)
{
	LLViewerAssetStatsFF::set_region_thread1(mRegionHandle);
	return true;
}
TFReqSendMetrics::~TFReqSendMetrics()
{
	delete mMainStats;
	mMainStats = 0;
}
bool
TFReqSendMetrics::doWork(LLTextureFetch * fetcher)
{
	if (! gViewerAssetStatsThread1)
		return true;
	static volatile bool reporting_started(false);
	static volatile S32 report_sequence(0);
	LLViewerAssetStats & main_stats = *mMainStats;
	main_stats.merge(*gViewerAssetStatsThread1);
	LLSD merged_llsd = main_stats.asLLSD(true);
	merged_llsd["session_id"] = mSessionID;
	merged_llsd["agent_id"] = mAgentID;
	merged_llsd["message"] = "ViewerAssetMetrics";
	merged_llsd["sequence"] = report_sequence;
	merged_llsd["initial"] = ! reporting_started;
	merged_llsd["break"] = LLTextureFetch::svMetricsDataBreak;
	if (S32_MAX == ++report_sequence)
		report_sequence = 0;
	reporting_started = true;
	merged_llsd["truncated"] = truncate_viewer_metrics(10, merged_llsd);
	if (! mCapsURL.empty())
	{
		if(fetcher->isQAMode() || true)
			LLHTTPClient::post(mCapsURL, merged_llsd, new AssetReportHandler());
		else
			LLHTTPClient::post(mCapsURL, merged_llsd, new LLHTTPClient::ResponderIgnore());
		LLTextureFetch::svMetricsDataBreak = false;
	}
	else
	{
		LLTextureFetch::svMetricsDataBreak = true;
	}
	if (fetcher->isQAMode())
	{
		LL_INFOS("Textures") << ll_pretty_print_sd(merged_llsd) << LL_ENDL;
	}
	gViewerAssetStatsThread1->reset();
	return true;
}
bool
truncate_viewer_metrics(int max_regions, LLSD & metrics)
{
	static const LLSD::String reg_tag("regions");
	static const LLSD::String duration_tag("duration");
	LLSD & reg_map(metrics[reg_tag]);
	if (reg_map.size() <= max_regions)
	{
		return false;
	}
	typedef std::multimap<LLSD::Real, int> reg_ordered_list_t;
	reg_ordered_list_t regions_by_duration;
	int ind(0);
	LLSD::array_const_iterator it_end(reg_map.endArray());
	for (LLSD::array_const_iterator it(reg_map.beginArray()); it_end != it; ++it, ++ind)
	{
		LLSD::Real duration = (*it)[duration_tag].asReal();
		regions_by_duration.insert(reg_ordered_list_t::value_type(duration, ind));
	}
	LLSD new_region(LLSD::emptyArray());
	reg_ordered_list_t::const_reverse_iterator it2_end(regions_by_duration.rend());
	reg_ordered_list_t::const_reverse_iterator it2(regions_by_duration.rbegin());
	for (int i(0); i < max_regions && it2_end != it2; ++i, ++it2)
	{
		new_region.append(reg_map[it2->second]);
	}
	reg_map = new_region;
	return true;
}
}
