/** 
 * @file llmeshrepository.cpp
 * @brief Mesh repository implementation.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"
#include <algorithm>
#include "apr_pools.h"
#include "apr_dso.h"
#include "llhttpstatuscodes.h"
#include "llmeshrepository.h"
#include "llagent.h"
#include "llappviewer.h"
#include "llbufferstream.h"
#include "llcallbacklist.h"
#include "lldatapacker.h"
#include "llfasttimer.h"
#include "llfloaterperms.h"
#include "llimagej2c.h"
#include "llhost.h"
#include "llnotificationsutil.h"
#include "llsd.h"
#include "llsdutil_math.h"
#include "llsdserialize.h"
#include "llthread.h"
#include "llvfile.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermenufile.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llvolume.h"
#include "llvolumemgr.h"
#include "llvovolume.h"
#include "llvoavatar.h"
#include "lldrawable.h"
#include "llassetfetchrank.h"
#include "llworld.h"
#include "material_codes.h"
#include "pipeline.h"
#include "llinventorymodel.h"
#include "llfoldertype.h"
#include "llviewerparcelmgr.h"
#include "llassetuploadresponders.h"
#include "lluploadfloaterobservers.h"
#include "aicurl.h"
#include <mutex>
#ifndef LL_WINDOWS
#include "netdb.h"
#endif
#include <queue>
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy meshHeaderResponder_timeout;
extern AIHTTPTimeoutPolicy meshLODResponder_timeout;
extern AIHTTPTimeoutPolicy meshSkinInfoResponder_timeout;
extern AIHTTPTimeoutPolicy meshDecompositionResponder_timeout;
extern AIHTTPTimeoutPolicy meshPhysicsShapeResponder_timeout;
extern AIHTTPTimeoutPolicy wholeModelFeeResponder_timeout;
extern AIHTTPTimeoutPolicy wholeModelUploadResponder_timeout;
LLMeshRepository gMeshRepo;
const U32 MAX_MESH_REQUESTS_PER_SECOND = 100;
const S32 MAX_MESH_VERSION = 999;
U32 LLMeshRepository::sBytesReceived = 0;
U32 LLMeshRepository::sHTTPRequestCount = 0;
U32 LLMeshRepository::sHTTPRetryCount = 0;
U32 LLMeshRepository::sLODProcessing = 0;
U32 LLMeshRepository::sLODPending = 0;
U32 LLMeshRepository::sCacheBytesRead = 0;
U32 LLMeshRepository::sCacheBytesWritten = 0;
U32 LLMeshRepository::sPeakKbps = 0;
U32 LLMeshRepository::sSkinParseErrors = 0;
U32 LLMeshRepository::sSkinSent = 0;
U32 LLMeshRepository::sSkinNoHeader = 0;
U32 LLMeshRepository::sYieldToTexture = 0;
const U32 MAX_TEXTURE_UPLOAD_RETRIES = 5;
static S32 dump_num = 0;
std::string make_dump_name(std::string prefix, S32 num)
{
	return prefix + fmt::to_string(num) + std::string(".xml");
}
void dump_llsd_to_file(const LLSD& content, std::string filename);
LLSD llsd_from_file(std::string filename);
std::string header_lod[] =
{
	"lowest_lod",
	"low_lod",
	"medium_lod",
	"high_lod"
};
const char * const LOG_MESH = "Mesh";
U32 get_volume_memory_size(const LLVolume* volume)
{
	U32 indices = 0;
	U32 vertices = 0;
	for (U32 i = 0; i < (U32)volume->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& face = volume->getVolumeFace(i);
		indices += face.mNumIndices;
		vertices += face.mNumVertices;
	}
	return indices * 2 + vertices * 11 + sizeof(LLVolume) + sizeof(LLVolumeFace) * volume->getNumVolumeFaces();
}
void get_vertex_buffer_from_mesh(LLCDMeshData& mesh, LLModel::PhysicsMesh& res, F32 scale = 1.f)
{
	res.mPositions.clear();
	res.mNormals.clear();
	const F32* v = mesh.mVertexBase;
	if (mesh.mIndexType == LLCDMeshData::INT_16)
	{
		U16* idx = (U16*) mesh.mIndexBase;
		for (S32 j = 0; j < mesh.mNumTriangles; ++j)
		{
			F32* mp0 = (F32*) ((U8*)v+idx[0]*mesh.mVertexStrideBytes);
			F32* mp1 = (F32*) ((U8*)v+idx[1]*mesh.mVertexStrideBytes);
			F32* mp2 = (F32*) ((U8*)v+idx[2]*mesh.mVertexStrideBytes);
			idx = (U16*) (((U8*)idx)+mesh.mIndexStrideBytes);
			LLVector3 v0(mp0);
			LLVector3 v1(mp1);
			LLVector3 v2(mp2);
			LLVector3 n = (v1-v0)%(v2-v0);
			n.normalize();
			res.mPositions.push_back(v0*scale);
			res.mPositions.push_back(v1*scale);
			res.mPositions.push_back(v2*scale);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
		}
	}
	else
	{
		U32* idx = (U32*) mesh.mIndexBase;
		for (S32 j = 0; j < mesh.mNumTriangles; ++j)
		{
			F32* mp0 = (F32*) ((U8*)v+idx[0]*mesh.mVertexStrideBytes);
			F32* mp1 = (F32*) ((U8*)v+idx[1]*mesh.mVertexStrideBytes);
			F32* mp2 = (F32*) ((U8*)v+idx[2]*mesh.mVertexStrideBytes);
			idx = (U32*) (((U8*)idx)+mesh.mIndexStrideBytes);
			LLVector3 v0(mp0);
			LLVector3 v1(mp1);
			LLVector3 v2(mp2);
			LLVector3 n = (v1-v0)%(v2-v0);
			n.normalize();
			res.mPositions.push_back(v0*scale);
			res.mPositions.push_back(v1*scale);
			res.mPositions.push_back(v2*scale);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
		}
	}
}
LLViewerFetchedTexture* LLMeshUploadThread::FindViewerTexture(const LLImportMaterial& material)
{
	LLPointer< LLViewerFetchedTexture > * ppTex = static_cast< LLPointer< LLViewerFetchedTexture > * >(material.mOpaqueData);
	return ppTex ? (*ppTex).get() : NULL;
}
S32 LLMeshRepoThread::sActiveHeaderRequests = 0;
S32 LLMeshRepoThread::sActiveLODRequests = 0;
U32	LLMeshRepoThread::sMaxConcurrentRequests = 1;
class LLMeshHeaderResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	LLVolumeParams mMeshParams;
	U32 mRank;
	bool mProcessed;
	void retry();
	LLMeshHeaderResponder(const LLVolumeParams& mesh_params, U32 rank)
		: mMeshParams(mesh_params), mRank(rank)
	{
		LLMeshRepoThread::incActiveHeaderRequests();
		mProcessed = false;
	}
	~LLMeshHeaderResponder()
	{
		if (!LLApp::isQuitting())
		{
			if (!mProcessed)
			{
				LL_WARNS() << "Timeout or service unavailable, retrying." << LL_ENDL;
				LLMeshRepository::sHTTPRetryCount++;
				LLMutexLock lock(gMeshRepo.mThread->mMutex);
				U32 rank = gMeshRepo.mThread->raiseRank(mMeshParams.getSculptID(), mRank);
				gMeshRepo.mThread->pushHeaderRequest(mMeshParams, 10.f, rank);
			}
			LLMeshRepoThread::decActiveHeaderRequests();
		}
	}
	virtual void completedRaw(LLChannelDescriptors const& channels,
							  LLIOPipe::buffer_ptr_t const& buffer);
	AICapabilityType capability_type(void) const { return cap_mesh; }
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return meshHeaderResponder_timeout; }
	char const* getName(void) const { return "LLMeshHeaderResponder"; }
};
class LLMeshLODResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	LLVolumeParams mMeshParams;
	S32 mLOD;
	U32 mRank;
	U32 mRequestedBytes;
	U32 mOffset;
	bool mProcessed;
	void retry();
	LLMeshLODResponder(const LLVolumeParams& mesh_params, S32 lod, U32 offset, U32 requested_bytes, U32 rank)
		: mMeshParams(mesh_params), mLOD(lod), mOffset(offset), mRequestedBytes(requested_bytes), mRank(rank)
	{
		LLMeshRepoThread::incActiveLODRequests();
		mProcessed = false;
	}
	~LLMeshLODResponder()
	{
		if (!LLApp::isExiting())
		{
			if (!mProcessed)
			{
				LL_WARNS() << "Killed without being processed, retrying." << LL_ENDL;
				LLMeshRepository::sHTTPRetryCount++;
				gMeshRepo.mThread->loadMeshLOD(mMeshParams, mLOD, mRank);
			}
			LLMeshRepoThread::decActiveLODRequests();
		}
	}
	virtual void completedRaw(LLChannelDescriptors const& channels,
							  LLIOPipe::buffer_ptr_t const& buffer);
	AICapabilityType capability_type(void) const { return cap_mesh; }
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return meshLODResponder_timeout; }
	char const* getName(void) const { return "LLMeshLODResponder"; }
};
class LLMeshSkinInfoResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	LLUUID mMeshID;
	U32 mRequestedBytes;
	U32 mOffset;
	bool mProcessed;
	void retry();
	LLMeshSkinInfoResponder(const LLUUID& id, U32 offset, U32 size)
		: mMeshID(id), mRequestedBytes(size), mOffset(offset)
	{
		mProcessed = false;
	}
	~LLMeshSkinInfoResponder()
	{
		if (!LLApp::isQuitting() &&
			!mProcessed &&
			mMeshID.notNull())
		{
			LL_WARNS() << "Timeout or service unavailable, retrying loadMeshSkinInfo() for " << mMeshID << LL_ENDL;
			LLMeshRepository::sHTTPRetryCount++;
			gMeshRepo.mThread->loadMeshSkinInfo(mMeshID);
		}
	}
	virtual void completedRaw(LLChannelDescriptors const& channels,
							  LLIOPipe::buffer_ptr_t const& buffer);
	AICapabilityType capability_type(void) const { return cap_mesh; }
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return meshSkinInfoResponder_timeout; }
	char const* getName(void) const { return "LLMeshSkinInfoResponder"; }
};
class LLMeshDecompositionResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	LLUUID mMeshID;
	U32 mRequestedBytes;
	U32 mOffset;
	bool mProcessed;
	void retry();
	LLMeshDecompositionResponder(const LLUUID& id, U32 offset, U32 size)
		: mMeshID(id), mRequestedBytes(size), mOffset(offset)
	{
		mProcessed = false;
	}
	~LLMeshDecompositionResponder()
	{
		if (!LLApp::isQuitting() &&
			!mProcessed &&
			mMeshID.notNull())
		{
			LL_WARNS() << "Timeout or service unavailable, retrying loadMeshDecomposition() for " << mMeshID << LL_ENDL;
			LLMeshRepository::sHTTPRetryCount++;
			gMeshRepo.mThread->loadMeshDecomposition(mMeshID);
		}
	}
	virtual void completedRaw(LLChannelDescriptors const& channels,
							  LLIOPipe::buffer_ptr_t const& buffer);
	AICapabilityType capability_type(void) const { return cap_mesh; }
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return meshDecompositionResponder_timeout; }
	char const* getName(void) const { return "LLMeshDecompositionResponder"; }
};
class LLMeshPhysicsShapeResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	LLUUID mMeshID;
	U32 mRequestedBytes;
	U32 mOffset;
	bool mProcessed;
	void retry();
	LLMeshPhysicsShapeResponder(const LLUUID& id, U32 offset, U32 size)
		: mMeshID(id), mRequestedBytes(size), mOffset(offset)
	{
		mProcessed = false;
	}
	~LLMeshPhysicsShapeResponder()
	{
		if (!LLApp::isQuitting() &&
			!mProcessed &&
			mMeshID.notNull())
		{
			LL_WARNS() << "Timeout or service unavailable, retrying loadMeshPhysicsShape() for " << mMeshID << LL_ENDL;
			LLMeshRepository::sHTTPRetryCount++;
			gMeshRepo.mThread->loadMeshPhysicsShape(mMeshID);
		}
	}
	virtual void completedRaw(LLChannelDescriptors const& channels,
							  LLIOPipe::buffer_ptr_t const& buffer);
	AICapabilityType capability_type(void) const { return cap_mesh; }
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return meshPhysicsShapeResponder_timeout; }
	char const* getName(void) const { return "LLMeshPhysicsShapeResponder"; }
};
void log_upload_error(S32 status, const LLSD& content, std::string stage, std::string model_name)
{
	LLSD args;
	std::string message = content["error"]["message"];
	std::string identifier = content["error"]["identifier"];
	args["MESSAGE"] = message;
	args["IDENTIFIER"] = identifier;
	args["LABEL"] = model_name;
	gMeshRepo.uploadError(args);
	LL_WARNS() << "stage: " << stage << " http status: " << status << LL_ENDL;
	if (content.has("error"))
	{
		const LLSD& err = content["error"];
		LL_WARNS() << "err: " << err << LL_ENDL;
		LL_WARNS() << "mesh upload failed, stage '" << stage
				<< "' error '" << err["error"].asString()
				<< "', message '" << err["message"].asString()
				<< "', id '" << err["identifier"].asString()
				<< "'" << LL_ENDL;
		if (err.has("errors"))
		{
			S32 error_num = 0;
			const LLSD& err_list = err["errors"];
			for (LLSD::array_const_iterator it = err_list.beginArray();
				 it != err_list.endArray();
				 ++it)
			{
				const LLSD& err_entry = *it;
				LL_WARNS() << "error[" << error_num << "]:" << LL_ENDL;
				for (LLSD::map_const_iterator map_it = err_entry.beginMap();
					 map_it != err_entry.endMap();
					 ++map_it)
				{
					LL_WARNS() << "\t" << map_it->first << ": "
							<< map_it->second << LL_ENDL;
				}
				error_num++;
			}
		}
	}
	else
	{
		LL_WARNS() << "bad mesh, no error information available" << LL_ENDL;
	}
}
class LLWholeModelFeeResponder : public LLHTTPClient::ResponderWithCompleted
{
	std::string& mWholeModelUploadURL;
	LLSD mModelData;
	LLHandle<LLWholeModelFeeObserver> mObserverHandle;
public:
	LLWholeModelFeeResponder(LLSD& model_data, LLHandle<LLWholeModelFeeObserver> observer_handle, std::string& url_out):
		mWholeModelUploadURL(url_out),
		mModelData(model_data),
		mObserverHandle(observer_handle)
	{
	}
	~LLWholeModelFeeResponder()
	{
	}
	virtual void httpCompleted(void)
	{
		LLSD cc = mContent;
		if (gSavedSettings.getS32("MeshUploadFakeErrors")&1)
		{
			cc = llsd_from_file("fake_upload_error.xml");
		}
		dump_llsd_to_file(cc,make_dump_name("whole_model_fee_response_",dump_num));
		LLWholeModelFeeObserver* observer = mObserverHandle.get();
		if (isGoodStatus(mStatus) &&
			cc["state"].asString() == "upload")
		{
			mWholeModelUploadURL = cc["uploader"].asString();
			if (observer)
			{
				cc["data"]["upload_price"] = cc["upload_price"];
				observer->onModelPhysicsFeeReceived(cc["data"], mWholeModelUploadURL);
			}
		}
		else
		{
			LL_WARNS() << "fee request failed" << LL_ENDL;
			log_upload_error(mStatus,cc,"fee",mModelData["name"]);
			mWholeModelUploadURL = "";
			if (observer)
			{
				observer->setModelPhysicsFeeErrorStatus(mStatus, mReason);
			}
		}
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return wholeModelFeeResponder_timeout; }
	char const* getName(void) const { return "LLWholeModelFeeResponder"; }
};
class LLWholeModelUploadResponder : public LLHTTPClient::ResponderWithCompleted
{
	LLSD mModelData;
	LLHandle<LLWholeModelUploadObserver> mObserverHandle;
public:
	LLWholeModelUploadResponder(LLSD& model_data, LLHandle<LLWholeModelUploadObserver> observer_handle):
		mModelData(model_data),
		mObserverHandle(observer_handle)
	{
	}
	~LLWholeModelUploadResponder()
	{
	}
	virtual void httpCompleted(void)
	{
		LLSD cc = mContent;
		if (gSavedSettings.getS32("MeshUploadFakeErrors")&2)
		{
			cc = llsd_from_file("fake_upload_error.xml");
		}
		dump_llsd_to_file(cc,make_dump_name("whole_model_upload_response_",dump_num));
		LLWholeModelUploadObserver* observer = mObserverHandle.get();
		if (isGoodStatus(mStatus) &&
			cc["state"].asString() == "complete")
		{
			mModelData["asset_type"] = "object";
			gMeshRepo.updateInventory(LLMeshRepository::inventory_data(mModelData,cc));
			if (observer)
			{
				doOnIdleOneTime(boost::bind(&LLWholeModelUploadObserver::onModelUploadSuccess, observer));
			}
		}
		else
		{
			LL_WARNS() << "upload failed" << LL_ENDL;
			std::string model_name = mModelData["name"].asString();
			log_upload_error(mStatus,cc,"upload",model_name);
			if (observer)
			{
				doOnIdleOneTime(boost::bind(&LLWholeModelUploadObserver::onModelUploadFailure, observer));
			}
		}
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return wholeModelUploadResponder_timeout; }
	char const* getName(void) const { return "LLWholeModelUploadResponder"; }
};
LLMeshRepoThread::LLMeshRepoThread()
: LLThread("mesh repo"), mSkinSkip(0)
{
	mMutex = new LLMutex();
	mHeaderMutex = new LLMutex();
	mSignal = new LLCondition();
	mSkinInfoQMutex = new LLMutex();
	mDecompositionQMutex = new LLMutex();
}
LLMeshRepoThread::~LLMeshRepoThread()
{
	while (!mPendingLODDecodeQ.empty())
	{
		delete[] mPendingLODDecodeQ.front().mData;
		mPendingLODDecodeQ.pop();
	}
	delete mMutex;
	mMutex = NULL;
	delete mHeaderMutex;
	mHeaderMutex = NULL;
	delete mSignal;
	mSignal = NULL;
	delete mSkinInfoQMutex;
	mSkinInfoQMutex = NULL;
	delete mDecompositionQMutex;
	mDecompositionQMutex = NULL;
}
bool LLMeshRepoThread::HeaderRequest::fetch(U32& count)
{
	return gMeshRepo.mThread->fetchMeshHeader(this->mMeshParams, count, this->mRank);
}
void LLMeshRepoThread::LODRequest::preFetch()
{
	--LLMeshRepository::sLODProcessing;
}
bool LLMeshRepoThread::LODRequest::fetch(U32& count)
{
	LLUUID mesh_id = mMeshParams.getSculptID();
	uuid_set_t::iterator skin = gMeshRepo.mThread->mSkinRequests.find(mesh_id);
	if (skin != gMeshRepo.mThread->mSkinRequests.end())
	{
		if (gMeshRepo.mThread->fetchMeshSkinInfo(mesh_id))
		{
			gMeshRepo.mThread->mSkinRequests.erase(skin);
		}
	}
	if (!gMeshRepo.mThread->fetchMeshLOD(this->mMeshParams, this->mLOD, count, this->mRank))
	{
		gMeshRepo.mThread->mMutex->lock();
		++LLMeshRepository::sLODProcessing;
		gMeshRepo.mThread->mMutex->unlock();
		return false;
	}
	return true;
}
void LLMeshRepoThread::runQueue(std::deque<std::pair<std::shared_ptr<MeshRequest>, F32> >& query, U32& count, S32& active_requests)
{
	std::queue<std::pair<std::shared_ptr<MeshRequest>, F32> > incomplete;
	if (mMutex)
	{
		mMutex->lock();
	}
	std::sort(query.begin(), query.end(), [](const std::pair<std::shared_ptr<MeshRequest>, F32>& a, const std::pair<std::shared_ptr<MeshRequest>, F32>& b)
	{
		auto effective = [](const std::shared_ptr<MeshRequest>& req) -> U32
		{
			if (!req)
			{
				return 0;
			}
			U32 rank = req->mRank;
			if (rank >= AllynAssetRank::URGENT)
			{
				return rank;
			}
			LODRequest* lod_req = dynamic_cast<LODRequest*>(req.get());
			if (!lod_req)
			{
				return rank;
			}
			S32 bias = lod_req->mLOD;
			if (bias < 0)
			{
				bias = 0;
			}
			if (bias > 3)
			{
				bias = 3;
			}
			return rank + (U32)bias;
		};
		return effective(a.first) > effective(b.first);
	});
	if (mMutex)
	{
		mMutex->unlock();
	}
	while (!query.empty() && count < MAX_MESH_REQUESTS_PER_SECOND && active_requests < (S32)sMaxConcurrentRequests)
	{
		if (mMutex)
		{
			mMutex->lock();
			auto req = query.front().first;
			F32 delay = query.front().second;
			U32 tex_rank = (U32)AllynAssetRank::textureWaitRank();
			if (tex_rank > req->mRank)
			{
				LLMeshRepository::sYieldToTexture += 1;
				while (!query.empty())
				{
					incomplete.push(query.front());
					query.pop_front();
				}
				mMutex->unlock();
				break;
			}
			query.pop_front();
			req->preFetch();
			mMutex->unlock();
			F32 remainder = delay - req->mTimer.getElapsedTimeF32();
			if (remainder > 0.f)
			{
				incomplete.push(std::make_pair(req, delay));
			}
			else if (!req->fetch(count))
			{
				LL_INFOS() << req->mMeshParams.getSculptID() << " fetch failed outright. Delaying for " << (delay ? delay : 15) << "s" << LL_ENDL;
				req->mTimer.reset();
				incomplete.push(std::make_pair(req, delay ? delay : 15));
			}
			else {
			}
		}
	}
	if (!incomplete.empty())
	{
		mMutex->lock();
		while (!incomplete.empty())
		{
			query.push_back(incomplete.front());
			incomplete.pop();
		}
		mMutex->unlock();
	}
}
void LLMeshRepoThread::runSet(uuid_set_t& set, std::function<bool(const LLUUID& mesh_id)> fn)
{
	for (auto iter = set.begin(); iter != set.end();)
	{
		if (fn(*iter))
			iter = set.erase(iter);
		else ++iter;
	}
}
void LLMeshRepoThread::issueSkins(U32& count, S32 limit)
{
	S32 issued = 0;
	if (mMutex)
	{
		mMutex->lock();
		while (!mSkinReady.empty() && issued < limit && count < MAX_MESH_REQUESTS_PER_SECOND)
		{
			LLUUID id = mSkinReady.front();
			mSkinReady.pop_front();
			mMutex->unlock();
			uuid_set_t::iterator ready = mSkinRequests.find(id);
			if (ready != mSkinRequests.end())
			{
				if (!fetchMeshSkinInfo(id))
				{
					mMutex->lock();
					mSkinReady.push_front(id);
					mMutex->unlock();
					return;
				}
				mSkinRequests.erase(ready);
				++issued;
				++count;
			}
			mMutex->lock();
		}
		mMutex->unlock();
	}
	if (mSkinRequests.empty() || issued >= limit || count >= MAX_MESH_REQUESTS_PER_SECOND)
	{
		return;
	}
	uuid_set_t::iterator iter = mSkinRequests.begin();
	S32 skip = mSkinSkip;
	while (skip > 0 && iter != mSkinRequests.end())
	{
		++iter;
		--skip;
	}
	if (iter == mSkinRequests.end())
	{
		iter = mSkinRequests.begin();
		mSkinSkip = 0;
	}
	S32 looked = 0;
	while (looked < 48 && issued < limit && count < MAX_MESH_REQUESTS_PER_SECOND && iter != mSkinRequests.end())
	{
		++mSkinSkip;
		++looked;
		MeshHeaderInfo info;
		if (!getMeshHeaderInfo(*iter, "skin", info))
		{
			LLMeshRepository::sSkinNoHeader++;
			++iter;
			if (iter == mSkinRequests.end())
			{
				iter = mSkinRequests.begin();
				mSkinSkip = 0;
			}
			continue;
		}
		if (!fetchMeshSkinInfo(*iter))
		{
			return;
		}
		iter = mSkinRequests.erase(iter);
		++issued;
		++count;
		if (iter == mSkinRequests.end())
		{
			iter = mSkinRequests.begin();
			mSkinSkip = 0;
		}
	}
}
void LLMeshRepoThread::run()
{
	LLCDResult res = LLConvexDecomposition::initThread();
	if (res != LLCD_OK)
	{
		LL_WARNS() << "convex decomposition unable to be loaded" << LL_ENDL;
	}
	mSignal->lock();
	while (!LLApp::isQuitting())
	{
		if (!LLApp::isQuitting())
		{
			static U32 count = 0;
			static F32 last_hundred = gFrameTimeSeconds;
			if (gFrameTimeSeconds - last_hundred > 1.f)
			{
				last_hundred = gFrameTimeSeconds;
				count = 0;
			}
			if (mMutex)
			{
				mMutex->lock();
				publishWaitRank();
				mMutex->unlock();
			}
			runQueue(mHeaderReqQ, count, sActiveHeaderRequests);
			issueSkins(count, mLODReqQ.empty() ? 4 : 1);
			if (mMutex)
			{
				mMutex->lock();
				publishWaitRank();
				mMutex->unlock();
			}
			runQueue(mLODReqQ, count, sActiveLODRequests);
			runSet(mDecompositionRequests, std::bind(&LLMeshRepoThread::fetchMeshDecomposition, this, std::placeholders::_1));
			runSet(mPhysicsShapeRequests, std::bind(&LLMeshRepoThread::fetchMeshPhysicsShape, this, std::placeholders::_1));
		}
		mSignal->unlock();
		processPendingLODDecodes();
		ms_sleep(1000 / 60);
		mSignal->lock();
	}
	if (mSignal->isLocked())
	{
		mSignal->unlock();
	}
	res = LLConvexDecomposition::quitThread();
	if (res != LLCD_OK)
	{
		LL_WARNS() << "convex decomposition unable to be quit" << LL_ENDL;
	}
}
void LLMeshRepoThread::loadMeshSkinInfo(const LLUUID& mesh_id)
{
	mSkinRequests.insert(mesh_id);
}
void LLMeshRepoThread::loadMeshDecomposition(const LLUUID& mesh_id)
{
	mDecompositionRequests.insert(mesh_id);
}
void LLMeshRepoThread::loadMeshPhysicsShape(const LLUUID& mesh_id)
{
	mPhysicsShapeRequests.insert(mesh_id);
}
void LLMeshRepoThread::lockAndLoadMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{
	if (!LLAppViewer::isQuitting())
	{
		loadMeshLOD(mesh_params, lod);
	}
}
U32 LLMeshRepoThread::raiseRank(const LLUUID& mesh_id, U32 rank)
{
	if (mesh_id.isNull())
	{
		return rank;
	}
	std::map<LLUUID, U32>::iterator known = mFetchRank.find(mesh_id);
	if (known != mFetchRank.end() && known->second > rank)
	{
		rank = known->second;
	}
	if (rank == 0)
	{
		return 0;
	}
	if (known == mFetchRank.end() || known->second < rank)
	{
		mFetchRank[mesh_id] = rank;
	}
	for (auto& item : mHeaderReqQ)
	{
		if (item.first && item.first->mMeshParams.getSculptID() == mesh_id && item.first->mRank < rank)
		{
			item.first->mRank = rank;
		}
	}
	for (auto& item : mLODReqQ)
	{
		if (item.first && item.first->mMeshParams.getSculptID() == mesh_id && item.first->mRank < rank)
		{
			item.first->mRank = rank;
		}
	}
	return rank;
}
void LLMeshRepoThread::loadMeshLOD(const LLVolumeParams& mesh_params, S32 lod, U32 rank)
{
	if (rank == 0)
	{
		rank = AllynAssetRank::meshRank(1.f);
	}
	std::unique_lock<LLMutex> header_lock(*mHeaderMutex);
	bool exists = mMeshHeader.find(mesh_params.getSculptID()) != mMeshHeader.end();
	header_lock.unlock();
	LLMutexLock lock(mMutex);
	LLUUID mesh_id = mesh_params.getSculptID();
	rank = raiseRank(mesh_id, rank);
	if (exists)
	{
		gMeshRepo.mThread->pushLODRequest(mesh_params, lod, 0.f, rank);
		LLMeshRepository::sLODProcessing++;
	}
	else
	{
		pending_lod_map::iterator pending = mPendingLOD.find(mesh_params);
		if (pending != mPendingLOD.end())
		{
			pending->second.push_back(lod);
			llassert(pending->second.size() <= LLModel::NUM_LODS);
		}
		else
		{
			gMeshRepo.mThread->pushHeaderRequest(mesh_params, 0.f, rank);
			mPendingLOD[mesh_params].push_back(lod);
		}
	}
	publishWaitRank();
}
std::string LLMeshRepoThread::constructUrl(LLUUID mesh_id)
{
	std::string http_url;
	if (gAgent.getRegion())
	{
		http_url = gMeshRepo.mGetMeshCapability;
	}
	if (!http_url.empty())
	{
		http_url += "/?mesh_id=";
		http_url += mesh_id.asString().c_str();
	}
	else
	{
		LL_WARNS() << "Current region does not have GetMesh capability!  Cannot load " << mesh_id << ".mesh" << LL_ENDL;
	}
	return http_url;
}
bool LLMeshRepoThread::getMeshHeaderInfo(const LLUUID& mesh_id, const char* block_name, MeshHeaderInfo& info)
{
	if (!mHeaderMutex)
	{
		return false;
	}
	LLMutexLock lock(mHeaderMutex);
	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{
		return false;
	}
	if ((info.mHeaderSize = mMeshHeaderSize[mesh_id]) > 0)
	{
		const LLSD& header = mMeshHeader[mesh_id];
		const LLSD& block = header[block_name];
		info.mVersion = header["version"].asInteger();
		info.mOffset = info.mHeaderSize + block["offset"].asInteger();
		info.mSize = block["size"].asInteger();
	}
	return true;
}
bool LLMeshRepoThread::loadInfoFromVFS(const LLUUID& mesh_id, MeshHeaderInfo& info, boost::function<bool(const LLUUID&, U8*, S32)> fn)
{
	LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
	if (file.getSize() >= info.mOffset + info.mSize)
	{
		LLMeshRepository::sCacheBytesRead += info.mSize;
		file.seek(info.mOffset);
		U8* buffer = new U8[info.mSize];
		file.read(buffer, info.mSize);
		bool zero = true;
		for (S32 i = 0; i < llmin(info.mSize, S32(1024)) && zero; ++i)
		{
			zero = buffer[i] > 0 ? false : true;
		}
		if (!zero)
		{
			if (fn(mesh_id, buffer, info.mSize))
			{
				delete[] buffer;
				return true;
			}
		}
		delete[] buffer;
	}
	return false;
}
bool LLMeshRepoThread::fetchMeshSkinInfo(const LLUUID& mesh_id)
{
	MeshHeaderInfo info;
	if (!getMeshHeaderInfo(mesh_id, "skin", info))
	{
		LLMeshRepository::sSkinNoHeader++;
		return false;
	}
	if (info.mHeaderSize > 0 && info.mVersion <= MAX_MESH_VERSION && info.mOffset >= 0 && info.mSize > 0)
	{
		if (loadInfoFromVFS(mesh_id, info, boost::bind(&LLMeshRepoThread::skinInfoReceived, this, _1, _2, _3 )))
		{
			LLMeshRepository::sSkinSent++;
			return true;
		}
		AIHTTPHeaders headers("Accept", "application/octet-stream");
		std::string http_url = constructUrl(mesh_id);
		if (!http_url.empty())
		{
			if (!LLHTTPClient::getByteRange(http_url, headers, info.mOffset, info.mSize,
				new LLMeshSkinInfoResponder(mesh_id, info.mOffset, info.mSize)))
				return false;
			LLMeshRepository::sHTTPRequestCount++;
			LLMeshRepository::sSkinSent++;
		}
	}
	return true;
}
bool LLMeshRepoThread::fetchMeshDecomposition(const LLUUID& mesh_id)
{
	MeshHeaderInfo info;
	if (!getMeshHeaderInfo(mesh_id, "physics_convex", info))
	{
		return false;
	}
	if (info.mHeaderSize > 0 && info.mVersion <= MAX_MESH_VERSION && info.mOffset >= 0 && info.mSize > 0)
	{
		if (loadInfoFromVFS(mesh_id, info, boost::bind(&LLMeshRepoThread::decompositionReceived, this, _1, _2, _3 )))
			return true;
		AIHTTPHeaders headers("Accept", "application/octet-stream");
		std::string http_url = constructUrl(mesh_id);
		if (!http_url.empty())
		{
			if (!LLHTTPClient::getByteRange(http_url, headers, info.mOffset, info.mSize,
				new LLMeshDecompositionResponder(mesh_id, info.mOffset, info.mSize)))
				return false;
			LLMeshRepository::sHTTPRequestCount++;
		}
	}
	return true;
}
bool LLMeshRepoThread::fetchMeshPhysicsShape(const LLUUID& mesh_id)
{
	MeshHeaderInfo info;
	if (!getMeshHeaderInfo(mesh_id, "physics_mesh", info))
	{
		return false;
	}
	if (info.mHeaderSize > 0)
	{
		if (info.mVersion <= MAX_MESH_VERSION && info.mOffset >= 0 && info.mSize > 0)
		{
			if (loadInfoFromVFS(mesh_id, info, boost::bind(&LLMeshRepoThread::physicsShapeReceived, this, _1, _2, _3 )))
				return true;
			AIHTTPHeaders headers("Accept", "application/octet-stream");
			std::string http_url = constructUrl(mesh_id);
			if (!http_url.empty())
			{
				if (!LLHTTPClient::getByteRange(http_url, headers, info.mOffset, info.mSize,
					new LLMeshPhysicsShapeResponder(mesh_id, info.mOffset, info.mSize)))
					return false;
				LLMeshRepository::sHTTPRequestCount++;
			}
		}
		else
		{
			physicsShapeReceived(mesh_id, NULL, 0);
		}
	}
	return true;
}
void LLMeshRepoThread::incActiveLODRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	++LLMeshRepoThread::sActiveLODRequests;
}
void LLMeshRepoThread::decActiveLODRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	--LLMeshRepoThread::sActiveLODRequests;
}
void LLMeshRepoThread::incActiveHeaderRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	++LLMeshRepoThread::sActiveHeaderRequests;
}
void LLMeshRepoThread::decActiveHeaderRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	--LLMeshRepoThread::sActiveHeaderRequests;
}
bool LLMeshRepoThread::fetchMeshHeader(const LLVolumeParams& mesh_params, U32& count, U32 rank)
{
	{
		LLVFile file(gVFS, mesh_params.getSculptID(), LLAssetType::AT_MESH);
		S32 size = file.getSize();
		if (size > 0)
		{
			U8 buffer[4096];
			S32 bytes = llmin(size, 4096);
			LLMeshRepository::sCacheBytesRead += bytes;
			file.read(buffer, bytes);
			if (headerReceived(mesh_params, buffer, bytes))
			{
				return true;
			}
		}
	}
	bool retval = true;
	AIHTTPHeaders headers("Accept", "application/octet-stream");
	std::string http_url = constructUrl(mesh_params.getSculptID());
	if (!http_url.empty())
	{
		retval = LLHTTPClient::getByteRange(http_url, headers, 0, 4096, new LLMeshHeaderResponder(mesh_params, rank));
		if (retval)
		{
			LLMeshRepository::sHTTPRequestCount++;
		}
		count++;
	}
	return retval;
}
bool LLMeshRepoThread::fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod, U32& count, U32 rank)
{
	LLUUID mesh_id = mesh_params.getSculptID();
	MeshHeaderInfo info;
	if (!getMeshHeaderInfo(mesh_id, header_lod[lod].c_str(), info))
	{
		return false;
	}
	if (info.mHeaderSize > 0)
	{
		if(info.mVersion <= MAX_MESH_VERSION && info.mOffset >= 0 && info.mSize > 0)
		{
			if (loadInfoFromVFS(mesh_id, info, boost::bind(&LLMeshRepoThread::lodReceived, this, mesh_params, lod, _2, _3 )))
				return true;
			AIHTTPHeaders headers("Accept", "application/octet-stream");
			std::string http_url = constructUrl(mesh_id);
			if (!http_url.empty())
			{
				count++;
				if (!LLHTTPClient::getByteRange(constructUrl(mesh_id), headers, info.mOffset, info.mSize,
						new LLMeshLODResponder(mesh_params, lod, info.mOffset, info.mSize, rank)))
					return false;
				LLMeshRepository::sHTTPRequestCount++;
			}
			else
			{
				mUnavailableQ.push(LODRequest(mesh_params, lod));
			}
		}
		else
		{
			mUnavailableQ.push(LODRequest(mesh_params, lod));
		}
	}
	return true;
}
bool LLMeshRepoThread::headerReceived(const LLVolumeParams& mesh_params, U8* data, S32 data_size)
{
	LLSD header;
	U32 header_size = 0;
	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);
		std::string deprecated_header("<? LLSD/Binary ?>");
		if (res_str.substr(0, deprecated_header.size()) == deprecated_header)
		{
			res_str = res_str.substr(deprecated_header.size()+1, data_size);
			header_size = deprecated_header.size()+1;
		}
		data_size = res_str.size();
		std::istringstream stream(res_str);
		if (!LLSDSerialize::fromBinary(header, stream, data_size))
		{
			LL_WARNS() << "Mesh header parse error.  Not a valid mesh asset!" << LL_ENDL;
			return false;
		}
		header_size += stream.tellg();
	}
	else
	{
		LL_INFOS()
			<< "Marking header as non-existent, will not retry." << LL_ENDL;
		header["404"] = 1;
	}
	{
		LLUUID mesh_id = mesh_params.getSculptID();
		{
			LLMutexLock lock(mHeaderMutex);
			mMeshHeaderSize[mesh_id] = header_size;
			mMeshHeader[mesh_id] = header;
		}
		LLMutexLock lock(mMutex);
		if (mSkinRequests.find(mesh_id) != mSkinRequests.end())
		{
			mSkinReady.push_back(mesh_id);
		}
		pending_lod_map::iterator iter = mPendingLOD.find(mesh_params);
		if (iter != mPendingLOD.end())
		{
			U32 rank = AllynAssetRank::meshRank(1.f);
			std::map<LLUUID, U32>::iterator known = mFetchRank.find(mesh_params.getSculptID());
			if (known != mFetchRank.end())
			{
				rank = known->second;
			}
			for (U32 i = 0; i < iter->second.size(); ++i)
			{
				LLMeshRepository::sLODProcessing++;
				gMeshRepo.mThread->pushLODRequest(mesh_params, iter->second[i], 0.f, rank);
			}
			mPendingLOD.erase(iter);
			publishWaitRank();
		}
	}
	return true;
}
bool LLMeshRepoThread::lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size)
{
	AIStateMachine::StateTimer timer("lodReceived");
	LLPointer<LLVolume> volume = new LLVolume(mesh_params, LLVolumeLODGroup::getVolumeScaleFromDetail(lod));
	std::string mesh_string((char*) data, data_size);
	std::istringstream stream(mesh_string);
	AIStateMachine::StateTimer timer2("unpackVolumeFaces");
	if (volume->unpackVolumeFaces(stream, data_size))
	{
		AIStateMachine::StateTimer timer("getNumFaces");
		if (volume->getNumFaces() > 0)
		{
			AIStateMachine::StateTimer timer("LoadedMesh");
			LoadedMesh mesh(volume, mesh_params, lod);
			{
				AIStateMachine::StateTimer timer("LLMutexLock");
				LLMutexLock lock(mMutex);
				mLoadedQ.push(mesh);
			}
			return true;
		}
	}
	return false;
}
void LLMeshRepoThread::queueLODDecode(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size, S32 offset, S32 requested_bytes)
{
	PendingLODDecode pending;
	pending.mMeshParams = mesh_params;
	pending.mLOD = lod;
	pending.mData = data;
	pending.mDataSize = data_size;
	pending.mOffset = offset;
	pending.mRequestedBytes = requested_bytes;
	LLMutexLock lock(mMutex);
	mPendingLODDecodeQ.push(pending);
}
void LLMeshRepoThread::processPendingLODDecodes()
{
	while (true)
	{
		PendingLODDecode pending;
		{
			LLMutexLock lock(mMutex);
			if (mPendingLODDecodeQ.empty())
			{
				return;
			}
			pending = mPendingLODDecodeQ.front();
			mPendingLODDecodeQ.pop();
		}
		bool ok = false;
		if (pending.mData && pending.mDataSize > 0)
		{
			ok = lodReceived(pending.mMeshParams, pending.mLOD, pending.mData, pending.mDataSize);
		}
		if (ok && pending.mData && pending.mRequestedBytes > 0)
		{
			LLVFile file(gVFS, pending.mMeshParams.getSculptID(), LLAssetType::AT_MESH, LLVFile::WRITE);
			S32 offset = pending.mOffset;
			S32 size = pending.mRequestedBytes;
			if (file.getSize() >= offset + size)
			{
				file.seek(offset);
				file.write(pending.mData, size);
				LLMeshRepository::sCacheBytesWritten += size;
			}
		}
		delete[] pending.mData;
	}
}
bool LLMeshRepoThread::skinInfoReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD skin;
	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);
		std::istringstream stream(res_str);
		if (!unzip_llsd(skin, stream, data_size))
		{
			LLMeshRepository::sSkinParseErrors++;
			LL_WARNS() << "Mesh skin info parse error.  Not a valid mesh asset! id=" << mesh_id
				<< " size=" << data_size << LL_ENDL;
			return false;
		}
	}
	{
		LLMeshSkinInfo info(skin);
		info.mMeshID = mesh_id;
		mSkinInfoQMutex->lock();
		mSkinInfoQ.push(info);
		mSkinInfoQMutex->unlock();
	}
	return true;
}
bool LLMeshRepoThread::decompositionReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD decomp;
	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);
		std::istringstream stream(res_str);
		if (!unzip_llsd(decomp, stream, data_size))
		{
			LL_WARNS() << "Mesh decomposition parse error.  Not a valid mesh asset!" << LL_ENDL;
			return false;
		}
	}
	{
		LLModel::Decomposition* d = new LLModel::Decomposition(decomp);
		d->mMeshID = mesh_id;
		mDecompositionQMutex->lock();
		mDecompositionQ.push(d);
		mDecompositionQMutex->unlock();
	}
	return true;
}
bool LLMeshRepoThread::physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD physics_shape;
	LLModel::Decomposition* d = new LLModel::Decomposition();
	d->mMeshID = mesh_id;
	if (data == NULL)
	{
		d->mPhysicsShapeMesh.clear();
	}
	else
	{
		LLVolumeParams volume_params;
		volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
		volume_params.setSculptID(mesh_id, LL_SCULPT_TYPE_MESH);
		LLPointer<LLVolume> volume = new LLVolume(volume_params,0);
		std::string mesh_string((char*) data, data_size);
		std::istringstream stream(mesh_string);
		if (volume->unpackVolumeFaces(stream, data_size))
		{
			S32 vertex_count = 0;
			S32 index_count = 0;
			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
				vertex_count += face.mNumVertices;
				index_count += face.mNumIndices;
			}
			d->mPhysicsShapeMesh.clear();
			std::vector<LLVector3>& pos = d->mPhysicsShapeMesh.mPositions;
			std::vector<LLVector3>& norm = d->mPhysicsShapeMesh.mNormals;
			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
				for (S32 i = 0; i < face.mNumIndices; ++i)
				{
					U16 idx = face.mIndices[i];
					pos.push_back(LLVector3(face.mPositions[idx].getF32ptr()));
					norm.push_back(LLVector3(face.mNormals[idx].getF32ptr()));
				}
			}
		}
	}
	mDecompositionQMutex->lock();
	mDecompositionQ.push(d);
	mDecompositionQMutex->unlock();
	return true;
}
void LLMeshUploadThread::init(LLMeshUploadThread::instance_list& data, LLVector3& scale, bool upload_textures,
							  bool upload_skin, bool upload_joints, bool do_upload,
							  LLHandle<LLWholeModelFeeObserver> const& fee_observer, LLHandle<LLWholeModelUploadObserver> const& upload_observer)
{
	mDoUpload = do_upload;
	mFeeObserverHandle = fee_observer;
	mUploadObserverHandle = upload_observer;
	mInstanceList = data;
	mUploadTextures = upload_textures;
	mUploadSkin = upload_skin;
	mUploadJoints = upload_joints;
	mOrigin = gAgent.getPositionAgent();
	mHost = gAgent.getRegionHost();
	mWholeModelFeeCapability = gAgent.getRegion()->getCapability("NewFileAgentInventory");
	mOrigin += gAgent.getAtAxis() * scale.magVec();
	mMeshUploadTimeOut = gSavedSettings.getS32("MeshUploadTimeOut") ;
}
LLMeshUploadThread::~LLMeshUploadThread()
{
}
LLMeshUploadThread::DecompRequest::DecompRequest(LLModel* mdl, LLModel* base_model, LLMeshUploadThread* thread)
{
	mStage = "single_hull";
	mModel = mdl;
	mDecompID = &mdl->mDecompID;
	mBaseModel = base_model;
	mThread = thread;
	assignData(mdl) ;
	mThread->mFinalDecomp = this;
	mThread->mPhysicsComplete = false;
}
void LLMeshUploadThread::DecompRequest::completed()
{
	if (mThread->mFinalDecomp == this)
	{
		mThread->mPhysicsComplete = true;
	}
	llassert(mHull.size() == 1);
	mThread->mHullMap[mBaseModel] = mHull[0];
}
void LLMeshUploadThread::preStart()
{
	for (instance_list::iterator iter = mInstanceList.begin(); iter != mInstanceList.end(); ++iter)
	{
		mInstance[iter->mModel].push_back(*iter);
	}
}
AIMeshUpload::AIMeshUpload(LLMeshUploadThread::instance_list& data, LLVector3& scale, bool upload_textures, bool upload_skin, bool upload_joints, std::string const& upload_url, bool do_upload,
	LLHandle<LLWholeModelFeeObserver> const& fee_observer, LLHandle<LLWholeModelUploadObserver> const& upload_observer) :
#ifdef CWDEBUG
		AIStateMachine(false),
#endif
		mMeshUpload(new AIStateMachineThread<LLMeshUploadThread>(CWD_ONLY(false))), mWholeModelUploadURL(upload_url)
{
	mMeshUpload->thread_impl().init(data, scale, upload_textures, upload_skin, upload_joints, do_upload, fee_observer, upload_observer);
}
char const* AIMeshUpload::state_str_impl(state_type run_state) const
{
	switch (run_state)
	{
		AI_CASE_RETURN(AIMeshUpload_start);
		AI_CASE_RETURN(AIMeshUpload_threadFinished);
		AI_CASE_RETURN(AIMeshUpload_responderFinished);
	}
	return "UNKNOWN STATE";
}
void AIMeshUpload::initialize_impl()
{
	mMeshUpload->thread_impl().preStart();
	set_state(AIMeshUpload_start);
}
void AIMeshUpload::multiplex_impl(state_type run_state)
{
	switch (run_state)
	{
		case AIMeshUpload_start:
			mMeshUpload->run(this, AIMeshUpload_threadFinished);
			idle();
			break;
		case AIMeshUpload_threadFinished:
			mMeshUpload->thread_impl().postRequest(mWholeModelUploadURL, this);
			idle();
			break;
		case AIMeshUpload_responderFinished:
			finish();
			break;
	}
}
bool LLMeshUploadThread::run()
{
	generateHulls();
	wholeModelToLLSD(mModelData, mDoUpload);
	if (!mDoUpload)
	{
		++dump_num;
		dump_llsd_to_file(mModelData, make_dump_name("whole_model_fee_request_", dump_num));
	}
	else
	{
		mBody = mModelData["asset_resources"];
		dump_llsd_to_file(mBody, make_dump_name("whole_model_body_", dump_num));
	}
	return true;
}
void LLMeshUploadThread::postRequest(std::string& whole_model_upload_url, AIMeshUpload* state_machine)
{
	if (mDoUpload)
	{
		LLHTTPClient::post(whole_model_upload_url, mBody,
			new LLWholeModelUploadResponder(mModelData, mUploadObserverHandle)
			DEBUG_CURLIO_PARAM(debug_off), keep_alive, state_machine, AIMeshUpload_responderFinished);
	}
	else
	{
		LLHTTPClient::post(mWholeModelFeeCapability, mModelData,
			new LLWholeModelFeeResponder(mModelData, mFeeObserverHandle, whole_model_upload_url)
			DEBUG_CURLIO_PARAM(debug_on), keep_alive, state_machine, AIMeshUpload_responderFinished);
	}
}
void dump_llsd_to_file(const LLSD& content, std::string filename)
{
	if (gSavedSettings.getBOOL("MeshUploadLogXML"))
	{
		std::ofstream of(filename.c_str());
		LLSDSerialize::toPrettyXML(content,of);
	}
}
LLSD llsd_from_file(std::string filename)
{
	std::ifstream ifs(filename.c_str());
	LLSD result;
	LLSDSerialize::fromXML(result,ifs);
	return result;
}
void LLMeshUploadThread::wholeModelToLLSD(LLSD& dest, bool include_textures)
{
	LLSD result;
	LLSD res;
	result["folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
	result["texture_folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE);
	result["asset_type"] = "mesh";
	result["inventory_type"] = "object";
	result["description"] = "(No Description)";
	result["next_owner_mask"] = LLSD::Integer(LLFloaterPerms::getNextOwnerPerms("Uploads"));
	result["group_mask"] = LLSD::Integer(LLFloaterPerms::getGroupPerms("Uploads"));
	result["everyone_mask"] = LLSD::Integer(LLFloaterPerms::getEveryonePerms("Uploads"));
	res["mesh_list"] = LLSD::emptyArray();
	res["texture_list"] = LLSD::emptyArray();
	res["instance_list"] = LLSD::emptyArray();
	S32 mesh_num = 0;
	S32 texture_num = 0;
	std::set<LLViewerTexture* > textures;
	std::map<LLViewerTexture*,S32> texture_index;
	std::map<LLModel*,S32> mesh_index;
	std::string model_name;
	std::string model_metric;
	S32 instance_num = 0;
	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;
		if (data.mBaseModel->mSubmodelID)
		{
			continue;
		}
		LLModelInstance& first_instance = *(iter->second.begin());
		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = first_instance.mLOD[i];
		}
		if (mesh_index.find(data.mBaseModel) == mesh_index.end())
		{
			if (model_name.empty())
			{
				model_name = data.mBaseModel->getName();
			}
			if (model_metric.empty())
			{
				model_metric = data.mBaseModel->getMetric();
			}
			std::stringstream ostr;
			LLModel::Decomposition& decomp =
				data.mModel[LLModel::LOD_PHYSICS].notNull() ?
				data.mModel[LLModel::LOD_PHYSICS]->mPhysics :
				data.mBaseModel->mPhysics;
			decomp.mBaseHull = mHullMap[data.mBaseModel];
			LLSD mesh_header = LLModel::writeModel(
				ostr,
				data.mModel[LLModel::LOD_PHYSICS],
				data.mModel[LLModel::LOD_HIGH],
				data.mModel[LLModel::LOD_MEDIUM],
				data.mModel[LLModel::LOD_LOW],
				data.mModel[LLModel::LOD_IMPOSTOR],
				decomp,
				mUploadSkin,
				mUploadJoints,
				FALSE,
				FALSE,
				data.mBaseModel->mSubmodelID);
			data.mAssetData = ostr.str();
			std::string str = ostr.str();
			res["mesh_list"][mesh_num] = LLSD::Binary(str.begin(),str.end());
			mesh_index[data.mBaseModel] = mesh_num;
			mesh_num++;
		}
		for (instance_list::iterator instance_iter = iter->second.begin();
			 instance_iter != iter->second.end();
			 ++instance_iter)
		{
			LLModelInstance& instance = *instance_iter;
			LLSD instance_entry;
			for (S32 i = 0; i < 5; i++)
			{
				data.mModel[i] = instance.mLOD[i];
			}
			LLVector3 pos, scale;
			LLQuaternion rot;
			LLMatrix4 transformation = instance.mTransform;
			decomposeMeshMatrix(transformation,pos,rot,scale);
			instance_entry["position"] = ll_sd_from_vector3(pos);
			instance_entry["rotation"] = ll_sd_from_quaternion(rot);
			instance_entry["scale"] = ll_sd_from_vector3(scale);
			instance_entry["material"] = LL_MCODE_WOOD;
			instance_entry["physics_shape_type"] = data.mModel[LLModel::LOD_PHYSICS].notNull() ? (U8)(LLViewerObject::PHYSICS_SHAPE_PRIM) : (U8)(LLViewerObject::PHYSICS_SHAPE_CONVEX_HULL);
			instance_entry["mesh"] = mesh_index[data.mBaseModel];
			instance_entry["mesh_name"] = instance.mLabel;
			instance_entry["face_list"] = LLSD::emptyArray();
			S32 end = llmin((S32)instance.mMaterial.size(), instance.mModel->getNumVolumeFaces()) ;
			for (S32 face_num = 0; face_num < end; face_num++)
			{
				LLImportMaterial& material = instance.mMaterial[data.mBaseModel->mMaterialList[face_num]];
				LLSD face_entry = LLSD::emptyMap();
				LLViewerFetchedTexture *texture = NULL;
				if (material.mDiffuseMapFilename.size())
				{
					texture = FindViewerTexture(material);
				}
				if ((texture != NULL) &&
					(textures.find(texture) == textures.end()))
				{
					textures.insert(texture);
				}
				std::stringstream texture_str;
				if (texture != NULL && include_textures && mUploadTextures)
				{
					if(texture->hasSavedRawImage())
					{
						LLPointer<LLImageJ2C> upload_file =
							LLViewerTextureList::convertToUploadFile(texture->getSavedRawImage());
						if (!upload_file.isNull() && upload_file->getDataSize())
						{
						texture_str.write((const char*) upload_file->getData(), upload_file->getDataSize());
					}
				}
				}
				if (texture != NULL &&
					mUploadTextures &&
					texture_index.find(texture) == texture_index.end())
				{
					texture_index[texture] = texture_num;
					std::string str = texture_str.str();
					res["texture_list"][texture_num] = LLSD::Binary(str.begin(),str.end());
					texture_num++;
				}
				if (texture != NULL && mUploadTextures)
				{
					face_entry["image"] = texture_index[texture];
					face_entry["scales"] = 1.0;
					face_entry["scalet"] = 1.0;
					face_entry["offsets"] = 0.0;
					face_entry["offsett"] = 0.0;
					face_entry["imagerot"] = 0.0;
				}
				face_entry["diffuse_color"] = ll_sd_from_color4(material.mDiffuseColor);
				face_entry["fullbright"] = material.mFullbright;
				instance_entry["face_list"][face_num] = face_entry;
		    }
			res["instance_list"][instance_num] = instance_entry;
			instance_num++;
		}
	}
	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;
		if (!data.mBaseModel->mSubmodelID)
		{
			continue;
		}
		LLModelInstance& first_instance = *(iter->second.begin());
		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = first_instance.mLOD[i];
		}
		if (mesh_index.find(data.mBaseModel) == mesh_index.end())
		{
			if (model_name.empty())
			{
				model_name = data.mBaseModel->getName();
			}
			if (model_metric.empty())
			{
				model_metric = data.mBaseModel->getMetric();
			}
			std::stringstream ostr;
			LLModel::Decomposition& decomp =
				data.mModel[LLModel::LOD_PHYSICS].notNull() ?
				data.mModel[LLModel::LOD_PHYSICS]->mPhysics :
				data.mBaseModel->mPhysics;
			decomp.mBaseHull = mHullMap[data.mBaseModel];
			LLSD mesh_header = LLModel::writeModel(
				ostr,
				data.mModel[LLModel::LOD_PHYSICS],
				data.mModel[LLModel::LOD_HIGH],
				data.mModel[LLModel::LOD_MEDIUM],
				data.mModel[LLModel::LOD_LOW],
				data.mModel[LLModel::LOD_IMPOSTOR],
				decomp,
				mUploadSkin,
				mUploadJoints,
				FALSE,
				FALSE,
				data.mBaseModel->mSubmodelID);
			data.mAssetData = ostr.str();
			std::string str = ostr.str();
			res["mesh_list"][mesh_num] = LLSD::Binary(str.begin(),str.end());
			mesh_index[data.mBaseModel] = mesh_num;
			mesh_num++;
		}
		for (instance_list::iterator instance_iter = iter->second.begin();
			 instance_iter != iter->second.end();
			 ++instance_iter)
		{
			LLModelInstance& instance = *instance_iter;
			LLSD instance_entry;
			for (S32 i = 0; i < 5; i++)
			{
				data.mModel[i] = instance.mLOD[i];
			}
			LLVector3 pos, scale;
			LLQuaternion rot;
			LLMatrix4 transformation = instance.mTransform;
			decomposeMeshMatrix(transformation,pos,rot,scale);
			instance_entry["position"] = ll_sd_from_vector3(pos);
			instance_entry["rotation"] = ll_sd_from_quaternion(rot);
			instance_entry["scale"] = ll_sd_from_vector3(scale);
			instance_entry["material"] = LL_MCODE_WOOD;
			instance_entry["physics_shape_type"] = (U8)(LLViewerObject::PHYSICS_SHAPE_NONE);
			instance_entry["mesh"] = mesh_index[data.mBaseModel];
			instance_entry["face_list"] = LLSD::emptyArray();
			S32 end = llmin((S32)instance.mMaterial.size(), instance.mModel->getNumVolumeFaces()) ;
			for (S32 face_num = 0; face_num < end; face_num++)
			{
				LLImportMaterial& material = instance.mMaterial[data.mBaseModel->mMaterialList[face_num]];
				LLSD face_entry = LLSD::emptyMap();
				LLViewerFetchedTexture *texture = NULL;
				if (material.mDiffuseMapFilename.size())
				{
					texture = FindViewerTexture(material);
				}
				if ((texture != NULL) &&
					(textures.find(texture) == textures.end()))
				{
					textures.insert(texture);
				}
				std::stringstream texture_str;
				if (texture != NULL && include_textures && mUploadTextures)
				{
					if(texture->hasSavedRawImage())
					{
						LLPointer<LLImageJ2C> upload_file =
							LLViewerTextureList::convertToUploadFile(texture->getSavedRawImage());
						if (!upload_file.isNull() && upload_file->getDataSize())
						{
						texture_str.write((const char*) upload_file->getData(), upload_file->getDataSize());
					}
				}
				}
				if (texture != NULL &&
					mUploadTextures &&
					texture_index.find(texture) == texture_index.end())
				{
					texture_index[texture] = texture_num;
					std::string str = texture_str.str();
					res["texture_list"][texture_num] = LLSD::Binary(str.begin(),str.end());
					texture_num++;
				}
				if (texture != NULL && mUploadTextures)
				{
					face_entry["image"] = texture_index[texture];
					face_entry["scales"] = 1.0;
					face_entry["scalet"] = 1.0;
					face_entry["offsets"] = 0.0;
					face_entry["offsett"] = 0.0;
					face_entry["imagerot"] = 0.0;
				}
				face_entry["diffuse_color"] = ll_sd_from_color4(material.mDiffuseColor);
				face_entry["fullbright"] = material.mFullbright;
				instance_entry["face_list"][face_num] = face_entry;
		    }
			res["instance_list"][instance_num] = instance_entry;
			instance_num++;
		}
	}
	if (model_name.empty()) model_name = "mesh model";
	result["name"] = model_name;
	if (model_metric.empty()) model_metric = "MUT_Unspecified";
	res["metric"] = model_metric;
	result["asset_resources"] = res;
	dump_llsd_to_file(result,make_dump_name("whole_model_",dump_num));
	dest = result;
}
void LLMeshUploadThread::generateHulls()
{
	bool has_valid_requests = false ;
	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;
		LLModelInstance& instance = *(iter->second.begin());
		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = instance.mLOD[i];
		}
		LLModel* physics = NULL;
		if (data.mModel[LLModel::LOD_PHYSICS].notNull())
		{
			physics = data.mModel[LLModel::LOD_PHYSICS];
		}
		else if (data.mModel[LLModel::LOD_LOW].notNull())
		{
			physics = data.mModel[LLModel::LOD_LOW];
		}
		else if (data.mModel[LLModel::LOD_MEDIUM].notNull())
		{
			physics = data.mModel[LLModel::LOD_MEDIUM];
		}
		else
		{
			physics = data.mModel[LLModel::LOD_HIGH];
		}
		llassert(physics != NULL);
		DecompRequest* request = new DecompRequest(physics, data.mBaseModel, this);
		if(request->isValid())
		{
			gMeshRepo.mDecompThread->submitRequest(request);
			has_valid_requests = true ;
		}
	}
	if(has_valid_requests)
	{
		while (!mPhysicsComplete)
		{
			apr_sleep(100);
		}
	}
}
void LLMeshRepoThread::notifyLoadedMeshes()
{
	if (!mMutex)
	{
		return;
	}
	while (!mLoadedQ.empty())
	{
		mMutex->lock();
		LoadedMesh mesh = mLoadedQ.front();
		mLoadedQ.pop();
		mMutex->unlock();
		if (mesh.mVolume && mesh.mVolume->getNumVolumeFaces() > 0)
		{
			gMeshRepo.notifyMeshLoaded(mesh.mMeshParams, mesh.mVolume);
		}
		else
		{
			gMeshRepo.notifyMeshUnavailable(mesh.mMeshParams,
				LLVolumeLODGroup::getVolumeDetailFromScale(mesh.mVolume->getDetail()));
		}
	}
	while (!mUnavailableQ.empty())
	{
		mMutex->lock();
		LODRequest req = mUnavailableQ.front();
		mUnavailableQ.pop();
		mMutex->unlock();
		gMeshRepo.notifyMeshUnavailable(req.mMeshParams, req.mLOD);
	}
	while (!mSkinInfoQ.empty())
	{
		mSkinInfoQMutex->lock();
		auto req = mSkinInfoQ.front();
		mSkinInfoQ.pop();
		mSkinInfoQMutex->unlock();
		gMeshRepo.notifySkinInfoReceived(req);
	}
	while (!mDecompositionQ.empty())
	{
		mDecompositionQMutex->lock();
		auto req = mDecompositionQ.front();
		mDecompositionQ.pop();
		mDecompositionQMutex->unlock();
		gMeshRepo.notifyDecompositionReceived(req);
	}
}
S32 LLMeshRepoThread::getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{
	LLMutexLock lock(mHeaderMutex);
	mesh_header_map::iterator iter = mMeshHeader.find(mesh_params.getSculptID());
	if (iter != mMeshHeader.end())
	{
		LLSD& header = iter->second;
		return LLMeshRepository::getActualMeshLOD(header, lod);
	}
	return lod;
}
S32 LLMeshRepository::getActualMeshLOD(LLSD& header, S32 lod)
{
	lod = llclamp(lod, 0, 3);
	S32 version = header["version"].asInteger();
	if (header.has("404") || version > MAX_MESH_VERSION)
	{
		return -1;
	}
	if (header[header_lod[lod]]["size"].asInteger() > 0)
	{
		return lod;
	}
	for (S32 i = lod-1; i >= 0; --i)
	{
		if (header[header_lod[i]]["size"].asInteger() > 0)
		{
			return i;
		}
	}
	for (S32 i = lod+1; i < 4; ++i)
	{
		if (header[header_lod[i]]["size"].asInteger() > 0)
		{
			return i;
		}
	}
	header["404"] = 1;
	return -1;
}
void LLMeshRepository::cacheOutgoingMesh(LLMeshUploadData& data, LLSD& header)
{
	mThread->mMeshHeader[data.mUUID] = header;
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
	volume_params.setSculptID(data.mUUID, LL_SCULPT_TYPE_MESH);
	for (U32 i = 0; i < 4; i++)
	{
		if (data.mModel[i].notNull())
		{
			LLPointer<LLVolume> volume = new LLVolume(volume_params, LLVolumeLODGroup::getVolumeScaleFromDetail(i));
			volume->copyVolumeFaces(data.mModel[i]);
			volume->setMeshAssetLoaded(TRUE);
		}
	}
}
void LLMeshLODResponder::retry()
{
	AIStateMachine::StateTimer timer("loadMeshLOD");
	LLMeshRepository::sHTTPRetryCount++;
	gMeshRepo.mThread->loadMeshLOD(mMeshParams, mLOD, mRank);
}
void LLMeshLODResponder::completedRaw(LLChannelDescriptors const& channels,
							          LLIOPipe::buffer_ptr_t const& buffer)
{
	mProcessed = true;
	if( !gMeshRepo.mThread )
	{
		return;
	}
	S32 data_size = buffer->countAfter(channels.in(), NULL);
	if (mStatus < 200 || mStatus >= 400)
	{
		LL_WARNS() << mStatus << ": " << mReason << LL_ENDL;
	}
	if (data_size < (S32)mRequestedBytes)
	{
		if (is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE)
		{
			LL_WARNS() << "Timeout or service unavailable, retrying." << LL_ENDL;
			retry();
		}
		else
		{
			llassert(is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE);
			LL_WARNS() << "Unhandled status " << mStatus << LL_ENDL;
		}
		return;
	}
	LLMeshRepository::sBytesReceived += mRequestedBytes;
	U8* data = NULL;
	if (data_size > 0)
	{
		AIStateMachine::StateTimer timer("readAfter");
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}
	gMeshRepo.mThread->queueLODDecode(mMeshParams, mLOD, data, data_size, (S32)mOffset, (S32)mRequestedBytes);
}
void LLMeshSkinInfoResponder::retry()
{
	LLMeshRepository::sHTTPRetryCount++;
	gMeshRepo.mThread->loadMeshSkinInfo(mMeshID);
}
void LLMeshSkinInfoResponder::completedRaw(LLChannelDescriptors const& channels,
							               LLIOPipe::buffer_ptr_t const& buffer)
{
	mProcessed = true;
	if( !gMeshRepo.mThread )
	{
		return;
	}
	S32 data_size = buffer->countAfter(channels.in(), NULL);
	if (mStatus < 200 || mStatus >= 400)
	{
		LL_WARNS() << mStatus << ": " << mReason << LL_ENDL;
	}
	if (data_size < (S32)mRequestedBytes)
	{
		if (is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE)
		{
			LL_WARNS() << "Timeout or service unavailable, retrying loadMeshSkinInfo() for " << mMeshID << LL_ENDL;
			retry();
		}
		else
		{
			llassert(is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE);
			LL_WARNS() << "Unhandled status " << mStatus << LL_ENDL;
		}
		return;
	}
	LLMeshRepository::sBytesReceived += mRequestedBytes;
	U8* data = NULL;
	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}
	if (gMeshRepo.mThread->skinInfoReceived(mMeshID, data, data_size))
	{
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);
		S32 offset = mOffset;
		S32 size = mRequestedBytes;
		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			file.seek(offset);
			file.write(data, size);
		}
	}
	else if (data && data_size > 0)
	{
		U32 sig = (U32)data[0] << 24;
		if (data_size > 1) sig |= (U32)data[1] << 16;
		if (data_size > 2) sig |= (U32)data[2] << 8;
		if (data_size > 3) sig |= (U32)data[3];
		LL_WARNS() << "skin bytes id=" << mMeshID
			<< " off=" << mOffset
			<< " ask=" << mRequestedBytes
			<< " got=" << data_size
			<< " sig=" << llformat("%08X", sig)
			<< LL_ENDL;
	}
	delete [] data;
}
void LLMeshDecompositionResponder::retry()
{
	LLMeshRepository::sHTTPRetryCount++;
	gMeshRepo.mThread->loadMeshDecomposition(mMeshID);
}
void LLMeshDecompositionResponder::completedRaw(LLChannelDescriptors const& channels,
							                    LLIOPipe::buffer_ptr_t const& buffer)
{
	mProcessed = true;
	if( !gMeshRepo.mThread )
	{
		return;
	}
	S32 data_size = buffer->countAfter(channels.in(), NULL);
	if (mStatus < 200 || mStatus >= 400)
	{
		LL_WARNS() << mStatus << ": " << mReason << LL_ENDL;
	}
	if (data_size < (S32)mRequestedBytes)
	{
		if (is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE)
		{
			LL_WARNS() << "Timeout or service unavailable, retrying loadMeshDecomposition() for " << mMeshID << LL_ENDL;
			retry();
		}
		else
		{
			llassert(is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE);
			LL_WARNS() << "Unhandled status " << mStatus << LL_ENDL;
		}
		return;
	}
	LLMeshRepository::sBytesReceived += mRequestedBytes;
	U8* data = NULL;
	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}
	if (gMeshRepo.mThread->decompositionReceived(mMeshID, data, data_size))
	{
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);
		S32 offset = mOffset;
		S32 size = mRequestedBytes;
		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			file.seek(offset);
			file.write(data, size);
		}
	}
	delete [] data;
}
void LLMeshPhysicsShapeResponder::retry()
{
	LLMeshRepository::sHTTPRetryCount++;
	gMeshRepo.mThread->loadMeshPhysicsShape(mMeshID);
}
void LLMeshPhysicsShapeResponder::completedRaw(LLChannelDescriptors const& channels,
							                   LLIOPipe::buffer_ptr_t const& buffer)
{
	mProcessed = true;
	if( !gMeshRepo.mThread )
	{
		return;
	}
	S32 data_size = buffer->countAfter(channels.in(), NULL);
	if (mStatus < 200 || mStatus >= 400)
	{
		LL_WARNS() << mStatus << ": " << mReason << LL_ENDL;
	}
	if (data_size < (S32)mRequestedBytes)
	{
		if (is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE)
		{
			LL_WARNS() << "Timeout or service unavailable, retrying loadMeshPhysicsShape() for " << mMeshID << LL_ENDL;
			retry();
		}
		else
		{
			llassert(is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE);
			LL_WARNS() << "Unhandled status " << mStatus << LL_ENDL;
		}
		return;
	}
	LLMeshRepository::sBytesReceived += mRequestedBytes;
	U8* data = NULL;
	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}
	if (gMeshRepo.mThread->physicsShapeReceived(mMeshID, data, data_size))
	{
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);
		S32 offset = mOffset;
		S32 size = mRequestedBytes;
		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			file.seek(offset);
			file.write(data, size);
		}
	}
	delete [] data;
}
void LLMeshHeaderResponder::retry()
{
	AIStateMachine::StateTimer timer("Retry");
	LLMeshRepository::sHTTPRetryCount++;
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	gMeshRepo.mThread->pushHeaderRequest(mMeshParams, 10.f, mRank);
}
void LLMeshHeaderResponder::completedRaw(LLChannelDescriptors const& channels,
										 LLIOPipe::buffer_ptr_t const& buffer)
{
	mProcessed = true;
	if( !gMeshRepo.mThread )
	{
		return;
	}
	if (mStatus < 200 || mStatus >= 400)
	{
		llassert(mStatus == HTTP_NOT_FOUND || mStatus == HTTP_SERVICE_UNAVAILABLE || mStatus == HTTP_REQUEST_TIME_OUT || is_internal_http_error_that_warrants_a_retry(mStatus));
		if (is_internal_http_error_that_warrants_a_retry(mStatus) || mStatus == HTTP_SERVICE_UNAVAILABLE)
		{
			LL_WARNS() << "Timeout or service unavailable, retrying." << LL_ENDL;
			retry();
			return;
		}
		else
		{
			LL_WARNS() << "Unhandled status: " << mStatus << LL_ENDL;
		}
	}
	S32 data_size = buffer->countAfter(channels.in(), NULL);
	static const U32 BUFF_MAX_STATIC_SIZE = 16384;
	static std::vector<U8> data(BUFF_MAX_STATIC_SIZE);
	if (data_size > (S32)data.size())
		data.resize(data_size);
	else
		memset(&data[0] + data_size, 0, data.size() - data_size);
	if (data_size > 0)
	{
		AIStateMachine::StateTimer timer("readAfter");
		buffer->readAfter(channels.in(), NULL, &data[0], data_size);
	}
	LLMeshRepository::sBytesReceived += llmin(data_size, 4096);
	AIStateMachine::StateTimer timer("headerReceived");
	bool success = gMeshRepo.mThread->headerReceived(mMeshParams, &data[0], data_size);
	llassert(success);
	if (!success)
	{
		LL_WARNS()
			<< "Unable to parse mesh header: "
			<< mStatus << ": " << mReason << LL_ENDL;
	}
	else if (data_size > 0)
	{
		LLUUID mesh_id = mMeshParams.getSculptID();
		LLSD header = gMeshRepo.mThread->mMeshHeader[mesh_id];
		S32 version = header["version"].asInteger();
		if (version <= MAX_MESH_VERSION)
		{
			std::stringstream str;
			S32 lod_bytes = 0;
			for (U32 i = 0; i < LLModel::LOD_PHYSICS; ++i)
			{
				std::string lod_name = header_lod[i];
				lod_bytes = llmax(lod_bytes, header[lod_name]["offset"].asInteger()+header[lod_name]["size"].asInteger());
			}
			lod_bytes = llmax(lod_bytes, header["skin"]["offset"].asInteger() + header["skin"]["size"].asInteger());
			lod_bytes = llmax(lod_bytes, header["physics_convex"]["offset"].asInteger() + header["physics_convex"]["size"].asInteger());
			S32 header_bytes = (S32) gMeshRepo.mThread->mMeshHeaderSize[mesh_id];
			S32 bytes = lod_bytes + header_bytes;
			data_size = llmin(data_size, bytes);
			AIStateMachine::StateTimer timer("FileOpen");
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH, LLVFile::WRITE);
			if (file.getMaxSize() >= bytes || file.setMaxSize(bytes))
			{
				LLMeshRepository::sCacheBytesWritten += data_size;
				AIStateMachine::StateTimer timer("WriteData");
				S32 bytes_remaining = bytes;
				while (bytes_remaining > 0)
				{
					const S32 bytes_to_write = llmin(bytes_remaining, data_size);
					file.write(&data[0], bytes_to_write);
					if (bytes_remaining == bytes && bytes_to_write < bytes_remaining)
					{
						memset(&data[0], 0, data.size());
					}
					bytes_remaining -= llmin(bytes_remaining, bytes_to_write);
				}
			}
		}
	}
	if (data.size() > BUFF_MAX_STATIC_SIZE)
	{
		std::vector<U8>().swap(data);
		data.resize(BUFF_MAX_STATIC_SIZE);
	}
}
LLMeshRepository::LLMeshRepository()
: mMeshMutex(NULL),
  mMeshThreadCount(0),
  mThread(NULL),
  mDecompThread(nullptr)
{
}
void LLMeshRepository::init()
{
	mMeshMutex = new LLMutex();
	LLConvexDecomposition::getInstance()->initSystem();
	mDecompThread = new LLPhysicsDecomp();
	mDecompThread->start();
	while (!mDecompThread->mInited)
	{
		apr_sleep(100);
	}
	mThread = new LLMeshRepoThread();
	mThread->start();
}
void LLMeshRepository::shutdown()
{
	LL_INFOS(LOG_MESH) << "Shutting down mesh repository." << LL_ENDL;
	mThread->mSignal->signal();
	while (!mThread->isStopped())
	{
		apr_sleep(10);
	}
	delete mThread;
	mThread = NULL;
	delete mMeshMutex;
	mMeshMutex = NULL;
	LL_INFOS(LOG_MESH) << "Shutting down decomposition system." << LL_ENDL;
	if (mDecompThread)
	{
		mDecompThread->shutdown();
		delete mDecompThread;
		mDecompThread = NULL;
	}
	LLConvexDecomposition::quitSystem();
}
void LLMeshRepository::unregisterMesh(LLVOVolume* vobj)
{
	for (auto& lod : mLoadingMeshes)
	{
		for (mesh_load_map::iterator param = lod.begin(); param != lod.end(); )
		{
			vector_replace_with_last(param->second, vobj);
			if (param->second.empty())
			{
				param = lod.erase(param);
			}
			else
			{
				++param;
			}
		}
	}
}
void LLMeshRepository::dropUnreferencedCache(const LLUUID& mesh_id)
{
	if (mesh_id.isNull() || !mMeshMutex || LLApp::isQuitting())
	{
		return;
	}
	mUnreferencedMeshes.insert(mesh_id);
	bool skin_busy = false;
	bool decomp_busy = false;
	{
		LLMutexLock lock(mMeshMutex);
		skin_busy = mLoadingSkins.count(mesh_id) != 0;
		decomp_busy = mLoadingDecompositions.count(mesh_id) != 0 || mLoadingPhysicsShapes.count(mesh_id) != 0;
	}
	if (!skin_busy)
	{
		mSkinMap.erase(mesh_id);
	}
	if (!decomp_busy)
	{
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			LLModel::Decomposition* decomp = iter->second;
			mDecompositionMap.erase(iter);
			delete decomp;
		}
	}
}
S32 LLMeshRepository::loadMesh(LLVOVolume* vobj, const LLVolumeParams& mesh_params, S32 detail, S32 last_lod)
{
	if (detail < 0 || detail > 4)
	{
		return detail;
	}
	mUnreferencedMeshes.erase(mesh_params.getSculptID());
	{
		LLMutexLock lock(mMeshMutex);
		mesh_load_map::iterator iter = mLoadingMeshes[detail].find(mesh_params);
		if (iter != mLoadingMeshes[detail].end() && !iter->second.empty())
		{
			auto it = std::find(iter->second.begin(), iter->second.end(), vobj);
			if (it == iter->second.end()) {
				iter->second.push_back(vobj);
			}
		}
		else
		{
			mLoadingMeshes[detail][mesh_params].push_back(vobj);
			mPendingRequests.push_back(LLMeshRepoThread::LODRequest(mesh_params, detail));
			LLMeshRepository::sLODPending++;
		}
	}
	LLVolume* volume = vobj->getVolume();
	if (volume)
	{
		LLVolumeParams params = volume->getParams();
		LLVolumeLODGroup* group = LLPrimitive::getVolumeManager()->getGroup(params);
		if (group)
		{
			if (last_lod >= 0)
			{
				LLVolume* lod = group->refLOD(last_lod);
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return last_lod;
				}
				group->derefLOD(lod);
			}
			for (S32 i = detail-1; i >= 0; --i)
			{
				LLVolume* lod = group->refLOD(i);
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return i;
				}
				group->derefLOD(lod);
			}
			for (S32 i = detail+1; i < 4; ++i)
			{
				LLVolume* lod = group->refLOD(i);
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return i;
				}
				group->derefLOD(lod);
			}
		}
	}
	return detail;
}
void LLMeshRepository::logMeshWatch()
{
	static const char* kWatch[] = {
		"8240f362-0d25-3da1-75d4-3b84c5657e49",
		"bebda738-eaed-0d98-9197-3bc64fb42efa",
		"b7e4705c-439f-7e9d-06f5-735b8f816532",
		"d79d084a-092f-7f63-2781-26c472769ba3",
		"f645ddf5-7ed3-a464-bf57-ec6185b7da23"
	};
	for (S32 index = 0; index < (S32)(sizeof(kWatch) / sizeof(kWatch[0])); ++index)
	{
		LLUUID watch_id(kWatch[index]);
		LLViewerObject* objectp = gObjectList.findObject(watch_id);
		if (!objectp)
		{
			LL_INFOS("ObjWatch") << "root=" << watch_id.asString() << " present=0" << LL_ENDL;
			continue;
		}
		LLUUID link_root = objectp->getRootEdit() ? objectp->getRootEdit()->getID() : watch_id;
		S32 kids = objectp->numChildren();
		S32 kid_mesh = 0;
		S32 kid_sculpt = 0;
		for (LLViewerObject::child_list_t::const_iterator it = objectp->getChildren().begin(); it != objectp->getChildren().end(); ++it)
		{
			LLVOVolume* cv = dynamic_cast<LLVOVolume*>(it->get());
			if (!cv)
			{
				continue;
			}
			if (cv->isMesh())
			{
				++kid_mesh;
			}
			else if (cv->isSculpted())
			{
				++kid_sculpt;
			}
		}
		F32 dist = objectp->mDrawable.notNull() ? objectp->mDrawable->mDistanceWRTCamera : -1.f;
		LL_INFOS("ObjWatch") << "root=" << watch_id.asString()
			<< " present=1"
			<< " dead=" << (objectp->isDead() ? 1 : 0)
			<< " pcode=" << (S32)objectp->getPCode()
			<< " scale=" << objectp->getScale().mV[0] << "," << objectp->getScale().mV[1] << "," << objectp->getScale().mV[2]
			<< " z=" << objectp->getPositionAgent().mV[2]
			<< " dist=" << dist
			<< " kids=" << kids
			<< " kid_mesh=" << kid_mesh
			<< " kid_sculpt=" << kid_sculpt
			<< " link=" << link_root.asString()
			<< " draw=" << (objectp->mDrawable.notNull() ? 1 : 0)
			<< " attach=" << (objectp->isAttachment() ? 1 : 0)
			<< LL_ENDL;
		std::vector<LLVOVolume*> parts;
		LLVOVolume* self_vol = dynamic_cast<LLVOVolume*>(objectp);
		if (self_vol)
		{
			parts.push_back(self_vol);
		}
		S32 added = 0;
		for (LLViewerObject::child_list_t::const_iterator it = objectp->getChildren().begin(); it != objectp->getChildren().end() && added < 8; ++it)
		{
			LLViewerObject* child = it->get();
			if (!child)
			{
				continue;
			}
			LLVOVolume* cv = dynamic_cast<LLVOVolume*>(child);
			if (!cv)
			{
				LL_INFOS("ObjWatch") << "root=" << watch_id.asString()
					<< " part=child"
					<< " id=" << child->getID().asString()
					<< " vol=0"
					<< " pcode=" << (S32)child->getPCode()
					<< LL_ENDL;
				++added;
				continue;
			}
			parts.push_back(cv);
			++added;
		}
		for (S32 p = 0; p < (S32)parts.size(); ++p)
		{
			LLVOVolume* vol = parts[p];
			const char* role = (vol == self_vol) ? "self" : "child";
			S32 mesh = vol->isMesh() ? 1 : 0;
			S32 lod = mesh ? vol->getLOD() : -1;
			S32 loaded = 0;
			S32 vfaces = 0;
			S32 got[4] = {0, 0, 0, 0};
			LLUUID sculpt;
			if (vol->getVolume())
			{
				vfaces = vol->getVolume()->getNumVolumeFaces();
				sculpt = vol->getVolume()->getParams().getSculptID();
				loaded = (mesh && vol->getVolume()->isMeshAssetLoaded() && vfaces > 0) ? 1 : 0;
				if (mesh)
				{
					LLVolumeLODGroup* group = LLPrimitive::getVolumeManager()->getGroup(vol->getVolume()->getParams());
					if (group)
					{
						for (S32 g = 0; g < 4; ++g)
						{
							LLVolume* lod_vol = group->refLOD(g);
							if (lod_vol && lod_vol->isMeshAssetLoaded() && lod_vol->getNumVolumeFaces() > 0)
							{
								got[g] = 1;
							}
							if (lod_vol)
							{
								group->derefLOD(lod_vol);
							}
						}
					}
				}
			}
			S32 hdr = 0;
			S32 gone = 0;
			S32 qh = -1;
			S32 ql = -1;
			S32 sz[4] = {0, 0, 0, 0};
			if (mesh && sculpt.notNull() && mThread)
			{
				if (mThread->mMutex)
				{
					LLMutexLock lock(mThread->mMutex);
					for (auto& item : mThread->mHeaderReqQ)
					{
						if (item.first && item.first->mMeshParams.getSculptID() == sculpt && (S32)item.first->mRank > qh)
						{
							qh = (S32)item.first->mRank;
						}
					}
					for (auto& item : mThread->mLODReqQ)
					{
						if (!item.first || item.first->mMeshParams.getSculptID() != sculpt)
						{
							continue;
						}
						LLMeshRepoThread::LODRequest* lod_req = dynamic_cast<LLMeshRepoThread::LODRequest*>(item.first.get());
						S32 lod_i = lod_req ? lod_req->mLOD : -1;
						if (lod_i > ql)
						{
							ql = lod_i;
						}
					}
				}
				if (mThread->mHeaderMutex)
				{
					LLMutexLock lock(mThread->mHeaderMutex);
					LLMeshRepoThread::mesh_header_map::iterator iter = mThread->mMeshHeader.find(sculpt);
					if (iter != mThread->mMeshHeader.end())
					{
						hdr = 1;
						if (iter->second.has("404"))
						{
							gone = 1;
						}
						for (S32 g = 0; g < 4; ++g)
						{
							sz[g] = iter->second[header_lod[g]]["size"].asInteger();
						}
					}
				}
			}
			S32 sculpted = vol->isSculpted() ? 1 : 0;
			S32 stype = -1;
			LLUUID stex;
			S32 slevel = vol->getVolume() ? vol->getVolume()->getSculptLevel() : -99;
			S32 sraw = -2;
			S32 sready = 0;
			S32 sdisc = -2;
			S32 sgl = 0;
			S32 smiss = 0;
			const LLSculptParams* sculpt_params = vol->getSculptParams();
			if (sculpt_params)
			{
				stype = (S32)(sculpt_params->getSculptType() & LL_SCULPT_TYPE_MASK);
				stex = sculpt_params->getSculptTexture();
			}
			if (stex.notNull())
			{
				LLViewerFetchedTexture* sculpt_tex = LLViewerTextureManager::findFetchedTexture(stex, TEX_LIST_STANDARD);
				if (!sculpt_tex)
				{
					sculpt_tex = LLViewerTextureManager::findFetchedTexture(stex, TEX_LIST_SCALE);
				}
				if (sculpt_tex)
				{
					sraw = sculpt_tex->getCachedRawImageLevel();
					sready = sculpt_tex->isCachedRawImageReady() ? 1 : 0;
					sdisc = sculpt_tex->getDiscardLevel();
					sgl = sculpt_tex->hasGLTexture() ? 1 : 0;
					smiss = sculpt_tex->isMissingAsset() ? 1 : 0;
				}
			}
			F32 part_dist = vol->mDrawable.notNull() ? vol->mDrawable->mDistanceWRTCamera : -1.f;
			LL_INFOS("ObjWatch") << "root=" << watch_id.asString()
				<< " part=" << role
				<< " id=" << vol->getID().asString()
				<< " mesh=" << mesh
				<< " sculpted=" << sculpted
				<< " stype=" << stype
				<< " stex=" << stex.asString()
				<< " slevel=" << slevel
				<< " sraw=" << sraw
				<< " sready=" << sready
				<< " sdisc=" << sdisc
				<< " sgl=" << sgl
				<< " smiss=" << smiss
				<< " sculpt=" << sculpt.asString()
				<< " lod=" << lod
				<< " loaded=" << loaded
				<< " got=" << got[0] << "," << got[1] << "," << got[2] << "," << got[3]
				<< " vfaces=" << vfaces
				<< " ofaces=" << vol->getNumFaces()
				<< " tes=" << (S32)vol->getNumTEs()
				<< " dist=" << part_dist
				<< " draw=" << (vol->mDrawable.notNull() ? 1 : 0)
				<< " hdr=" << hdr
				<< " gone=" << gone
				<< " qH=" << qh
				<< " qL=" << ql
				<< " sz=" << sz[0] << "," << sz[1] << "," << sz[2] << "," << sz[3]
				<< LL_ENDL;
			S32 face_limit = (S32)vol->getNumTEs();
			if (face_limit > 4)
			{
				face_limit = 4;
			}
			for (S32 f = 0; f < face_limit; ++f)
			{
				LLTextureEntry* te = vol->getTE((U8)f);
				LLViewerTexture* img = vol->getTEImage((U8)f);
				LLUUID tex_id;
				S32 disc = -1;
				S32 gl = 0;
				S32 miss = 0;
				S32 white = 0;
				S32 full_w = 0;
				S32 want = -1;
				S32 boost = -1;
				S32 raw = -2;
				S32 fetcher = 0;
				S32 listed = 0;
				F32 decpri = 0.f;
				U32 fpri = 0;
				if (img)
				{
					tex_id = img->getID();
					disc = img->getDiscardLevel();
					gl = img->hasGLTexture() ? 1 : 0;
					miss = img->isMissingAsset() ? 1 : 0;
					white = (img == LLViewerFetchedTexture::sWhiteImagep.get()) ? 1 : 0;
					full_w = img->getFullWidth();
					LLViewerFetchedTexture* fetched = LLViewerTextureManager::staticCastToFetchedTexture(img, FALSE);
					if (fetched)
					{
						want = fetched->getDesiredDiscardLevel();
						boost = fetched->getBoostLevel();
						raw = fetched->getRawImageLevel();
						fetcher = fetched->hasFetcher() ? 1 : 0;
						listed = fetched->isInImageList() ? 1 : 0;
						decpri = fetched->getDecodePriority();
						fpri = fetched->getFetchPriority();
					}
				}
				LLUUID bound_id;
				S32 bound_disc = -1;
				if (vol->mDrawable.notNull())
				{
					LLFace* face = vol->mDrawable->getFace(f);
					LLViewerTexture* bound = face ? face->getTexture() : NULL;
					if (bound)
					{
						bound_id = bound->getID();
						bound_disc = bound->getDiscardLevel();
					}
				}
				LLUUID mat_id;
				S32 mat_ptr = 0;
				S32 mat_ready = 0;
				LLUUID base_id;
				F32 alpha = 1.f;
				if (te)
				{
					alpha = te->getColor().mV[3];
					mat_id = te->getGLTFMaterialId();
					LLGLTFMaterial* mat = te->getGLTFMaterial();
					if (mat)
					{
						mat_ptr = 1;
						mat_ready = mat->mReady ? 1 : 0;
						base_id = mat->mTextureId[LLGLTFMaterial::BASE_COLOR];
					}
				}
				LL_INFOS("ObjWatch") << "root=" << watch_id.asString()
					<< " part=" << role
					<< " id=" << vol->getID().asString()
					<< " face=" << f
					<< " tex=" << tex_id.asString()
					<< " disc=" << disc
					<< " vs=" << (img ? img->getMaxVirtualSize() : 0.f)
					<< " want=" << want
					<< " boost=" << boost
					<< " raw=" << raw
					<< " fetcher=" << fetcher
					<< " listed=" << listed
					<< " decpri=" << decpri
					<< " fpri=" << fpri
					<< " bound=" << bound_id.asString()
					<< " bound_disc=" << bound_disc
					<< " gl=" << gl
					<< " miss=" << miss
					<< " white=" << white
					<< " full=" << full_w
					<< " alpha=" << alpha
					<< " mat=" << mat_id.asString()
					<< " mat_ptr=" << mat_ptr
					<< " ready=" << mat_ready
					<< " base=" << base_id.asString()
					<< LL_ENDL;
			}
		}
	}
}
void LLMeshRepository::notifyLoadedMeshes()
{
	static const LLCachedControl<U32> max_concurrent_requests("MeshMaxConcurrentRequests");
	LLMeshRepoThread::sMaxConcurrentRequests = max_concurrent_requests;
	{
		static LLFrameTimer s_mesh_stats_timer;
		if (s_mesh_stats_timer.getElapsedTimeF32() >= 5.f)
		{
			s_mesh_stats_timer.reset();
			S32 active = LLMeshRepoThread::sActiveHeaderRequests + LLMeshRepoThread::sActiveLODRequests;
			S32 push_room = (S32)LLMeshRepoThread::sMaxConcurrentRequests - active;
			S32 header_q = 0;
			S32 lod_q = 0;
			S32 skin_q = 0;
			S32 decode_q = 0;
			S32 skin_wait = 0;
			S32 skin_ready = 0;
			S32 skin_hdr = 0;
			if (mMeshMutex)
			{
				LLMutexLock skin_lock(mMeshMutex);
				skin_wait = (S32)mPendingSkinRequests.size();
			}
			if (mThread && mThread->mMutex)
			{
				std::vector<LLUUID> skin_ids;
				{
					LLMutexLock mesh_lock(mThread->mMutex);
					header_q = (S32)mThread->mHeaderReqQ.size();
					lod_q = (S32)mThread->mLODReqQ.size();
					skin_q = (S32)mThread->mSkinRequests.size();
					decode_q = (S32)mThread->mPendingLODDecodeQ.size();
					skin_ready = (S32)mThread->mSkinReady.size();
					skin_ids.reserve(mThread->mSkinRequests.size());
					for (uuid_set_t::const_iterator it = mThread->mSkinRequests.begin(); it != mThread->mSkinRequests.end(); ++it)
					{
						skin_ids.push_back(*it);
					}
				}
				if (mThread->mHeaderMutex)
				{
					LLMutexLock header_lock(mThread->mHeaderMutex);
					for (std::vector<LLUUID>::const_iterator it = skin_ids.begin(); it != skin_ids.end(); ++it)
					{
						if (mThread->mMeshHeader.find(*it) != mThread->mMeshHeader.end())
						{
							++skin_hdr;
						}
					}
				}
			}
			static U32 last_yield_tex = 0;
			static U32 last_skin_sent = 0;
			static U32 last_skin_miss = 0;
			U32 yield_tex = sYieldToTexture;
			U32 d_yield_tex = yield_tex - last_yield_tex;
			last_yield_tex = yield_tex;
			U32 skin_sent = sSkinSent;
			U32 skin_miss = sSkinNoHeader;
			U32 d_skin_sent = skin_sent - last_skin_sent;
			U32 d_skin_miss = skin_miss - last_skin_miss;
			last_skin_sent = skin_sent;
			last_skin_miss = skin_miss;
			LL_INFOS("MESHSTATS") << "HTTP " << sHTTPRequestCount << "/" << sHTTPRetryCount
				<< " Pending/Processing " << sLODPending << "/" << sLODProcessing
				<< " ActiveH/L " << LLMeshRepoThread::sActiveHeaderRequests
				<< "/" << LLMeshRepoThread::sActiveLODRequests
				<< " MaxConc " << LLMeshRepoThread::sMaxConcurrentRequests
				<< " PushRoom " << push_room
				<< " MBRecv " << (sBytesReceived / (1024.f * 1024.f))
				<< " SkinParseErr " << sSkinParseErrors
				<< " CacheRW " << (sCacheBytesRead / (1024.f * 1024.f))
				<< "/" << (sCacheBytesWritten / (1024.f * 1024.f)) << " MB"
				<< " qH=" << header_q
				<< " qL=" << lod_q
				<< " qSkin=" << skin_q
				<< " qDec=" << decode_q
				<< " skinWait=" << skin_wait
				<< " skinHdr=" << skin_hdr
				<< " skinReady=" << skin_ready
				<< " d_skinSent=" << d_skin_sent
				<< " d_skinMiss=" << d_skin_miss
				<< " yieldTex=" << yield_tex
				<< " d_yieldTex=" << d_yield_tex
				<< LL_ENDL;
			logMeshWatch();
		}
	}
	if (!mInventoryQ.empty())
	{
		LLMutexLock lock(mMeshMutex);
		while (!mInventoryQ.empty())
		{
			inventory_data& data = mInventoryQ.front();
			LLAssetType::EType asset_type = LLAssetType::lookup(data.mPostData["asset_type"].asString());
			LLInventoryType::EType inventory_type = LLInventoryType::lookup(data.mPostData["inventory_type"].asString());
			if ( data.mResponse.has("new_texture_folder_id") )
			{
				const LLUUID& folder_id = data.mResponse["new_texture_folder_id"].asUUID();
				if ( folder_id.notNull() )
				{
					LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE);
					std::string name;
					if ( data.mResponse.has("new_texture_folder_name") )
					{
						name = data.mResponse["new_texture_folder_name"].asString();
					}
					else
					{
						name = data.mPostData["name"].asString();
					}
					LLPointer<LLViewerInventoryCategory> cat =
						new LLViewerInventoryCategory(folder_id, parent_id,
							LLFolderType::FT_NONE, name, gAgent.getID());
					cat->setVersion(LLViewerInventoryCategory::VERSION_UNKNOWN);
					LLInventoryModel::LLCategoryUpdate update(cat->getParentUUID(), 1);
					gInventory.accountForUpdate(update);
					gInventory.updateCategory(cat);
				}
			}
			on_new_single_inventory_upload_complete(
				asset_type,
				inventory_type,
				data.mPostData["asset_type"].asString(),
				data.mPostData["folder_id"].asUUID(),
				data.mPostData["name"],
				data.mPostData["description"],
				data.mResponse,
				data.mResponse["upload_price"]);
			mInventoryQ.pop();
		}
	}
	mDecompThread->notifyCompleted();
	if (!mThread->mSignal->try_lock())
	{
		return;
	}
	mThread->mSignal->unlock();
	static std::string region_name("never name a region this");
	if (gAgent.getRegion())
	{
		if (gAgent.getRegion()->getName() != region_name && gAgent.getRegion()->capabilitiesReceived())
		{
			region_name = gAgent.getRegion()->getName();
			const std::string mesh_cap(gAgent.getRegion()->getViewerAssetUrl());
			mGetMeshCapability = !mesh_cap.empty() ? mesh_cap : gAgent.getRegion()->getCapability("GetMesh2");
			if (mGetMeshCapability.empty())
			{
				mGetMeshCapability = gAgent.getRegion()->getCapability("GetMesh");
			}
		}
	}
	{
		LLMutexLock lock1(mMeshMutex);
		LLMutexLock lock2(mThread->mMutex);
		while (!mUploadErrorQ.empty())
		{
			LLNotificationsUtil::add("MeshUploadError", mUploadErrorQ.front());
			mUploadErrorQ.pop();
		}
		S32 push_count = LLMeshRepoThread::sMaxConcurrentRequests-(LLMeshRepoThread::sActiveHeaderRequests+LLMeshRepoThread::sActiveLODRequests);
		push_count = llmin(push_count, (S32)mPendingRequests.size());
		boost::unordered_map<LLUUID, F32> score_map;
		const F32 AVATAR_LOD_RADIUS = 2.2f;
		const F32 OWN_MESH_SCORE = 1000000.f;
		for (auto& lod : mLoadingMeshes)
		{
			for (auto& param : lod)
			{
				F32 max_score = 0.f;
				for (auto& vobj : param.second)
				{
					if (LLDrawable* drawable = vobj->mDrawable)
					{
						F32 dist = llmax(drawable->mDistanceWRTCamera, 0.1f);
						F32 radius = drawable->getRadius();
						bool own = false;
						if (vobj->isHUDAttachment())
						{
							own = true;
						}
						else if (vobj->isAttachment())
						{
							radius = llmax(radius, AVATAR_LOD_RADIUS);
							LLVOAvatar* avatar = vobj->getAvatarAncestor();
							if (avatar && avatar->isSelf())
							{
								own = true;
							}
						}
						F32 cur_score = radius / dist;
						if (own)
						{
							cur_score += OWN_MESH_SCORE;
						}
						max_score = llmax(max_score, cur_score);
					}
				}
				if (max_score > 0.f)
				{
					score_map[param.first.getSculptID()] = max_score;
				}
			}
		}
		for (boost::unordered_map<LLUUID, F32>::iterator it = score_map.begin(); it != score_map.end(); ++it)
		{
			bool own = it->second >= OWN_MESH_SCORE;
			F32 score = own ? it->second - OWN_MESH_SCORE : it->second;
			F32 px = score * LLDrawable::sCurPixelAngle;
			U32 live_rank = own ? (U32)AllynAssetRank::URGENT : AllynAssetRank::meshRank(px);
			std::map<LLUUID, U32>::iterator known = mThread->mFetchRank.find(it->first);
			if (known == mThread->mFetchRank.end() || known->second < live_rank)
			{
				mThread->mFetchRank[it->first] = live_rank;
			}
		}
		for (auto& item : mThread->mHeaderReqQ)
		{
			if (!item.first)
			{
				continue;
			}
			std::map<LLUUID, U32>::iterator known = mThread->mFetchRank.find(item.first->mMeshParams.getSculptID());
			if (known != mThread->mFetchRank.end() && score_map.find(item.first->mMeshParams.getSculptID()) != score_map.end() && item.first->mRank < known->second)
			{
				item.first->mRank = known->second;
			}
		}
		for (auto& item : mThread->mLODReqQ)
		{
			if (!item.first)
			{
				continue;
			}
			std::map<LLUUID, U32>::iterator known = mThread->mFetchRank.find(item.first->mMeshParams.getSculptID());
			if (known != mThread->mFetchRank.end() && score_map.find(item.first->mMeshParams.getSculptID()) != score_map.end() && item.first->mRank < known->second)
			{
				item.first->mRank = known->second;
			}
		}
		mThread->publishWaitRank();
		if (push_count > 0)
		{
			for (std::vector<LLMeshRepoThread::LODRequest>::iterator iter = mPendingRequests.begin(); iter != mPendingRequests.end(); ++iter)
			{
				boost::unordered_map<LLUUID, F32>::iterator scored = score_map.find(iter->mMeshParams.getSculptID());
				iter->mScore = scored == score_map.end() ? 0.f : scored->second;
			}
			std::partial_sort(mPendingRequests.begin(), mPendingRequests.begin() + push_count,
							  mPendingRequests.end(), LLMeshRepoThread::CompareScoreGreater());
			U32 tex_rank = (U32)AllynAssetRank::textureWaitRank();
			while (!mPendingRequests.empty() && push_count > 0)
			{
				LLMeshRepoThread::LODRequest& request = mPendingRequests.front();
				bool own = request.mScore >= OWN_MESH_SCORE;
				F32 score = own ? request.mScore - OWN_MESH_SCORE : request.mScore;
				F32 px = score * LLDrawable::sCurPixelAngle;
				U32 rank = own ? (U32)AllynAssetRank::URGENT : AllynAssetRank::meshRank(px);
				if (!own && tex_rank > rank)
				{
					LLMeshRepository::sYieldToTexture += 1;
					break;
				}
				mThread->loadMeshLOD(request.mMeshParams, request.mLOD, rank);
				mPendingRequests.erase(mPendingRequests.begin());
				LLMeshRepository::sLODPending--;
				push_count--;
			}
		}
		while (!mPendingSkinRequests.empty())
		{
			mThread->loadMeshSkinInfo(mPendingSkinRequests.front());
			mPendingSkinRequests.pop();
		}
		while (!mPendingDecompositionRequests.empty())
		{
			mThread->loadMeshDecomposition(mPendingDecompositionRequests.front());
			mPendingDecompositionRequests.pop();
		}
		while (!mPendingPhysicsShapeRequests.empty())
		{
			mThread->loadMeshPhysicsShape(mPendingPhysicsShapeRequests.front());
			mPendingPhysicsShapeRequests.pop();
		}
		mThread->notifyLoadedMeshes();
	}
	mThread->mSignal->signal();
}
void LLMeshRepository::notifySkinInfoReceived(LLMeshSkinInfo& info)
{
	mSkinMap.insert_or_assign(info.mMeshID, info);
	bool any_live = false;
	skin_load_map::iterator iter = mLoadingSkins.find(info.mMeshID);
	if (iter != mLoadingSkins.end())
	{
		for (auto obj_id = iter->second.begin(); obj_id != iter->second.end(); ++obj_id)
		{
			LLVOVolume* vobj = (LLVOVolume*) gObjectList.findObject(*obj_id);
			if (vobj)
			{
				any_live = true;
				vobj->notifyMeshLoaded();
			}
		}
		mLoadingSkins.erase(info.mMeshID);
	}
	if (mUnreferencedMeshes.count(info.mMeshID))
	{
		S32 live_volumes = gObjectList.notifyVolumesUsingMesh(info.mMeshID);
		if (!any_live && live_volumes == 0)
		{
			mSkinMap.erase(info.mMeshID);
		}
		mUnreferencedMeshes.erase(info.mMeshID);
	}
}
void LLMeshRepository::notifyDecompositionReceived(LLModel::Decomposition* decomp)
{
	LLUUID mesh_id = decomp->mMeshID;
	decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
	if (iter == mDecompositionMap.end())
	{
		mDecompositionMap[mesh_id] = decomp;
		mLoadingDecompositions.erase(mesh_id);
	}
	else
	{
		iter->second->merge(decomp);
		mLoadingDecompositions.erase(mesh_id);
		delete decomp;
	}
	if (mUnreferencedMeshes.count(mesh_id))
	{
		S32 live_volumes = gObjectList.notifyVolumesUsingMesh(mesh_id);
		if (live_volumes == 0)
		{
			iter = mDecompositionMap.find(mesh_id);
			if (iter != mDecompositionMap.end())
			{
				LLModel::Decomposition* stored = iter->second;
				mDecompositionMap.erase(iter);
				delete stored;
			}
		}
		mUnreferencedMeshes.erase(mesh_id);
	}
}
void LLMeshRepository::notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volume)
{
	S32 detail = LLVolumeLODGroup::getVolumeDetailFromScale(volume->getDetail());
	mesh_load_map::iterator obj_iter = mLoadingMeshes[detail].find(mesh_params);
	if (volume && obj_iter != mLoadingMeshes[detail].end())
	{
		if (volume->getNumVolumeFaces() <= 0)
		{
			LL_WARNS(LOG_MESH) << "Mesh loading returned empty volume.  ID:  " << mesh_params.getSculptID()
							   << LL_ENDL;
		}
		{
			LLVolume* sys_volume = LLPrimitive::getVolumeManager()->refVolume(mesh_params, detail);
			if (sys_volume)
			{
				sys_volume->copyVolumeFaces(volume);
				sys_volume->setMeshAssetLoaded(TRUE);
				LLPrimitive::getVolumeManager()->unrefVolume(sys_volume);
			}
			else
			{
				LL_WARNS(LOG_MESH) << "Couldn't find system volume for mesh " << mesh_params.getSculptID()
								   << LL_ENDL;
			}
		}
		for (auto& vobj : obj_iter->second)
		{
			vobj->notifyMeshLoaded();
		}
		mLoadingMeshes[detail].erase(mesh_params);
	}
}
void LLMeshRepository::notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod)
{
	mesh_load_map::iterator obj_iter = mLoadingMeshes[lod].find(mesh_params);
	F32 detail = LLVolumeLODGroup::getVolumeScaleFromDetail(lod);
	if (obj_iter != mLoadingMeshes[lod].end())
	{
		for (auto& vobj : obj_iter->second)
		{
			LLVolume* obj_volume = vobj->getVolume();
			if (obj_volume &&
				obj_volume->getDetail() == detail &&
				obj_volume->getParams() == mesh_params)
			{
					vobj->setVolume(obj_volume->getParams(), lod);
			}
		}
		mLoadingMeshes[lod].erase(mesh_params);
	}
}
S32 LLMeshRepository::getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{
	return mThread->getActualMeshLOD(mesh_params, lod);
}
const LLMeshSkinInfo* LLMeshRepository::getSkinInfo(const LLUUID& mesh_id, const LLVOVolume* requesting_obj)
{
	if (mesh_id.notNull())
	{
		mUnreferencedMeshes.erase(mesh_id);
		const auto iter = mSkinMap.find(mesh_id);
		if (iter != mSkinMap.cend())
		{
			return &(iter->second);
		}
		{
			LLMutexLock lock(mMeshMutex);
			skin_load_map::iterator iter = mLoadingSkins.find(mesh_id);
			if (iter == mLoadingSkins.end())
			{
				mPendingSkinRequests.push(mesh_id);
			}
			mLoadingSkins[mesh_id].insert(requesting_obj->getID());
		}
	}
	return NULL;
}
void LLMeshRepository::fetchPhysicsShape(const LLUUID& mesh_id)
{
	if (mesh_id.notNull())
	{
		mUnreferencedMeshes.erase(mesh_id);
		LLModel::Decomposition* decomp = NULL;
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			decomp = iter->second;
		}
		if (!decomp || decomp->mPhysicsShapeMesh.empty())
		{
			LLMutexLock lock(mMeshMutex);
			auto iter = mLoadingPhysicsShapes.find(mesh_id);
			if (iter == mLoadingPhysicsShapes.end())
			{
				mLoadingPhysicsShapes.insert(mesh_id);
				mPendingPhysicsShapeRequests.push(mesh_id);
			}
		}
	}
}
LLModel::Decomposition* LLMeshRepository::getDecomposition(const LLUUID& mesh_id)
{
	LLModel::Decomposition* ret = NULL;
	if (mesh_id.notNull())
	{
		mUnreferencedMeshes.erase(mesh_id);
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			ret = iter->second;
		}
		if (!ret || ret->mBaseHullMesh.empty())
		{
			LLMutexLock lock(mMeshMutex);
			auto iter = mLoadingDecompositions.find(mesh_id);
			if (iter == mLoadingDecompositions.end())
			{
				mLoadingDecompositions.insert(mesh_id);
				mPendingDecompositionRequests.push(mesh_id);
			}
		}
	}
	return ret;
}
void LLMeshRepository::buildHull(const LLVolumeParams& params, S32 detail)
{
	LLVolume* volume = LLPrimitive::getVolumeManager()->refVolume(params, detail);
	if (!volume->mHullPoints)
	{
	}
	LLPrimitive::getVolumeManager()->unrefVolume(volume);
}
bool LLMeshRepository::hasPhysicsShape(const LLUUID& mesh_id)
{
	LLSD mesh = mThread->getMeshHeader(mesh_id);
	if (mesh.has("physics_mesh") && mesh["physics_mesh"].has("size") && (mesh["physics_mesh"]["size"].asInteger() > 0))
	{
		return true;
	}
	LLModel::Decomposition* decomp = getDecomposition(mesh_id);
	if (decomp && !decomp->mHull.empty())
	{
		return true;
	}
	return false;
}
LLSD& LLMeshRepository::getMeshHeader(const LLUUID& mesh_id)
{
	return mThread->getMeshHeader(mesh_id);
}
LLSD& LLMeshRepoThread::getMeshHeader(const LLUUID& mesh_id)
{
	static LLSD dummy_ret;
	if (mesh_id.notNull())
	{
		LLMutexLock lock(mHeaderMutex);
		mesh_header_map::iterator iter = mMeshHeader.find(mesh_id);
		if (iter != mMeshHeader.end() && mMeshHeaderSize[mesh_id] > 0)
		{
			return iter->second;
		}
	}
	return dummy_ret;
}
void LLMeshRepository::uploadModel(std::vector<LLModelInstance>& data, LLVector3& scale, bool upload_textures,
									bool upload_skin, bool upload_joints, std::string upload_url, bool do_upload,
								   LLHandle<LLWholeModelFeeObserver> fee_observer, LLHandle<LLWholeModelUploadObserver> upload_observer)
{
	if (do_upload && upload_url.empty())
	{
		LL_INFOS() << "unable to upload, fee request failed" << LL_ENDL;
		return;
	}
	AIMeshUpload* thread = new AIMeshUpload(data, scale, upload_textures, upload_skin, upload_joints, upload_url,
												do_upload, fee_observer, upload_observer);
	thread->run(NULL, 0, false, true, &gMainThreadEngine);
}
S32 LLMeshRepository::getMeshSize(const LLUUID& mesh_id, S32 lod)
{
	if (mThread && mesh_id.notNull() && LLPrimitive::NO_LOD != lod)
	{
		LLMutexLock lock(mThread->mHeaderMutex);
		LLMeshRepoThread::mesh_header_map::iterator iter = mThread->mMeshHeader.find(mesh_id);
		if (iter != mThread->mMeshHeader.end() && mThread->mMeshHeaderSize[mesh_id] > 0)
		{
			LLSD& header = iter->second;
			if (header.has("404"))
			{
				return -1;
			}
			S32 size = header[header_lod[lod]]["size"].asInteger();
			return size;
		}
	}
	return -1;
}
void LLMeshUploadThread::decomposeMeshMatrix(LLMatrix4& transformation,
											 LLVector3& result_pos,
											 LLQuaternion& result_rot,
											 LLVector3& result_scale)
{
	BOOL reflected = (transformation.determinant() < 0);
	LLVector3 position = LLVector3(0, 0, 0) * transformation;
	LLVector3 x_transformed = LLVector3(1, 0, 0) * transformation - position;
	LLVector3 y_transformed = LLVector3(0, 1, 0) * transformation - position;
	LLVector3 z_transformed = LLVector3(0, 0, 1) * transformation - position;
	F32 x_length = x_transformed.normalize();
	F32 y_length = y_transformed.normalize();
	F32 z_length = z_transformed.normalize();
	LLVector3 scale = LLVector3(x_length, y_length, z_length);
	LLVector3 x_transformed_reflected = x_transformed;
	if (reflected)
	{
		x_transformed_reflected *= -1.0;
	}
	LLMatrix3 rotation_matrix;
	rotation_matrix.setRows(x_transformed_reflected, y_transformed, z_transformed);
	LLQuaternion quat_rotation = rotation_matrix.quaternion();
	quat_rotation.normalize();
	LLVector3 euler_rotation;
	quat_rotation.getEulerAngles(&euler_rotation.mV[VX], &euler_rotation.mV[VY], &euler_rotation.mV[VZ]);
	result_pos = position + mOrigin;
	result_scale = scale;
	result_rot = quat_rotation;
}
void LLMeshRepository::updateInventory(inventory_data data)
{
	LLMutexLock lock(mMeshMutex);
	dump_llsd_to_file(data.mPostData,make_dump_name("update_inventory_post_data_",dump_num));
	dump_llsd_to_file(data.mResponse,make_dump_name("update_inventory_response_",dump_num));
	mInventoryQ.push(data);
}
void LLMeshRepository::uploadError(LLSD& args)
{
	LLMutexLock lock(mMeshMutex);
	mUploadErrorQ.push(args);
}
F32 LLMeshRepository::getEstTrianglesMax(LLUUID mesh_id)
{
	LLMeshCostData costs;
	if (getCostData(mesh_id, costs))
	{
		return costs.getEstTrisMax();
	}
	else
	{
		return 0.f;
	}
}
F32 LLMeshRepository::getEstTrianglesStreamingCost(LLUUID mesh_id)
{
    LLMeshCostData costs;
    if (getCostData(mesh_id, costs))
    {
        return costs.getEstTrisForStreamingCost();
    }
    else
    {
        return 0.f;
    }
}
F32 LLMeshRepository::getStreamingCostLegacy(LLUUID mesh_id, F32 radius, S32* bytes, S32* bytes_visible, S32 lod, F32 *unscaled_value)
{
	F32 result = 0.f;
    if (mThread && mesh_id.notNull())
    {
        LLMutexLock lock(mThread->mHeaderMutex);
        LLMeshRepoThread::mesh_header_map::iterator iter = mThread->mMeshHeader.find(mesh_id);
        if (iter != mThread->mMeshHeader.end() && mThread->mMeshHeaderSize[mesh_id] > 0)
        {
            result  = getStreamingCostLegacy(iter->second, radius, bytes, bytes_visible, lod, unscaled_value);
        }
    }
    if (result > 0.f)
    {
        LLMeshCostData data;
        if (getCostData(mesh_id, data))
        {
            F32 ref_streaming_cost = data.getRadiusBasedStreamingCost(radius);
            F32 ref_weighted_tris = data.getRadiusWeightedTris(radius);
            if (!is_approx_equal(ref_streaming_cost,result))
            {
                LL_WARNS() << mesh_id << "streaming mismatch " << result << " " << ref_streaming_cost << LL_ENDL;
            }
            if (unscaled_value && !is_approx_equal(ref_weighted_tris,*unscaled_value))
            {
                LL_WARNS() << mesh_id << "weighted_tris mismatch " << *unscaled_value << " " << ref_weighted_tris << LL_ENDL;
            }
            if (bytes && (*bytes != data.getSizeTotal()))
            {
                LL_WARNS() << mesh_id << "bytes mismatch " << *bytes << " " << data.getSizeTotal() << LL_ENDL;
            }
            if (bytes_visible && (lod >=0) && (lod < 4) && (*bytes_visible != data.getSizeByLOD(lod)))
            {
                LL_WARNS() << mesh_id << "bytes_visible mismatch " << *bytes_visible << " " << data.getSizeByLOD(lod) << LL_ENDL;
            }
        }
        else
        {
            LL_WARNS() << "getCostData failed!!!" << LL_ENDL;
        }
    }
    return result;
}
F32 LLMeshRepository::getStreamingCostLegacy(LLSD& header, F32 radius, S32* bytes, S32* bytes_visible, S32 lod, F32 *unscaled_value)
{
	if (header.has("404")
		|| !header.has("lowest_lod")
		|| (header.has("version") && header["version"].asInteger() > MAX_MESH_VERSION))
	{
		return 0.f;
	}
	F32 max_distance = 512.f;
	F32 dlowest = llmin(radius/0.03f, max_distance);
	F32 dlow = llmin(radius/0.06f, max_distance);
	F32 dmid = llmin(radius/0.24f, max_distance);
	static const LLCachedControl<U32> mesh_meta_data_discount("MeshMetaDataDiscount");
	static const LLCachedControl<U32> mesh_minimum_byte_size("MeshMinimumByteSize");
	static const LLCachedControl<U32> mesh_bytes_per_triangle("MeshBytesPerTriangle");
	static const LLCachedControl<U32> mesh_triangle_budget("MeshTriangleBudget");
	F32 METADATA_DISCOUNT = (F32) mesh_meta_data_discount.get();
	F32 MINIMUM_SIZE = (F32) mesh_minimum_byte_size.get();
	F32 bytes_per_triangle = (F32) mesh_bytes_per_triangle.get();
	if (!(bytes_per_triangle > 0.f))
		bytes_per_triangle = 16.f;
	S32 bytes_lowest = header["lowest_lod"]["size"].asInteger();
	S32 bytes_low = header["low_lod"]["size"].asInteger();
	S32 bytes_mid = header["medium_lod"]["size"].asInteger();
	S32 bytes_high = header["high_lod"]["size"].asInteger();
	if (bytes_high == 0)
	{
		return 0.f;
	}
	if (bytes_mid == 0)
	{
		bytes_mid = bytes_high;
	}
	if (bytes_low == 0)
	{
		bytes_low = bytes_mid;
	}
	if (bytes_lowest == 0)
	{
		bytes_lowest = bytes_low;
	}
	F32 triangles_lowest = llmax((F32) bytes_lowest-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_low = llmax((F32) bytes_low-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_mid = llmax((F32) bytes_mid-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_high = llmax((F32) bytes_high-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	if (bytes)
	{
		*bytes = 0;
		*bytes += header["lowest_lod"]["size"].asInteger();
		*bytes += header["low_lod"]["size"].asInteger();
		*bytes += header["medium_lod"]["size"].asInteger();
		*bytes += header["high_lod"]["size"].asInteger();
	}
	if (bytes_visible)
	{
		lod = LLMeshRepository::getActualMeshLOD(header, lod);
		if (lod >= 0 && lod <= 3)
		{
			*bytes_visible = header[header_lod[lod]]["size"].asInteger();
		}
	}
	F32 max_area = 102944.f;
	F32 min_area = 1.f;
	F32 high_area = llmin(F_PI*dmid*dmid, max_area);
	F32 mid_area = llmin(F_PI*dlow*dlow, max_area);
	F32 low_area = llmin(F_PI*dlowest*dlowest, max_area);
	F32 lowest_area = max_area;
	lowest_area -= low_area;
	low_area -= mid_area;
	mid_area -= high_area;
	high_area = llclamp(high_area, min_area, max_area);
	mid_area = llclamp(mid_area, min_area, max_area);
	low_area = llclamp(low_area, min_area, max_area);
	lowest_area = llclamp(lowest_area, min_area, max_area);
	F32 total_area = high_area + mid_area + low_area + lowest_area;
	high_area /= total_area;
	mid_area /= total_area;
	low_area /= total_area;
	lowest_area /= total_area;
	F32 weighted_avg = triangles_high*high_area +
					   triangles_mid*mid_area +
					   triangles_low*low_area +
					  triangles_lowest*lowest_area;
	if (unscaled_value)
	{
		*unscaled_value = weighted_avg;
	}
	F32 triangle_budget = (F32)mesh_triangle_budget.get();
	if (!(triangle_budget > 0.f))
		triangle_budget = 250000.f;
	return weighted_avg/triangle_budget*15000.f;
}
LLMeshCostData::LLMeshCostData()
{
    mSizeByLOD.resize(4);
    mEstTrisByLOD.resize(4);
    std::fill(mSizeByLOD.begin(), mSizeByLOD.end(), 0);
    std::fill(mEstTrisByLOD.begin(), mEstTrisByLOD.end(), 0.f);
}
bool LLMeshCostData::init(const LLSD& header)
{
    mSizeByLOD.resize(4);
    mEstTrisByLOD.resize(4);
    std::fill(mSizeByLOD.begin(), mSizeByLOD.end(), 0);
    std::fill(mEstTrisByLOD.begin(), mEstTrisByLOD.end(), 0.f);
    S32 bytes_high = header["high_lod"]["size"].asInteger();
    S32 bytes_med = header["medium_lod"]["size"].asInteger();
    if (bytes_med == 0)
    {
        bytes_med = bytes_high;
    }
    S32 bytes_low = header["low_lod"]["size"].asInteger();
    if (bytes_low == 0)
    {
        bytes_low = bytes_med;
    }
    S32 bytes_lowest = header["lowest_lod"]["size"].asInteger();
    if (bytes_lowest == 0)
    {
        bytes_lowest = bytes_low;
    }
    mSizeByLOD[0] = bytes_lowest;
    mSizeByLOD[1] = bytes_low;
    mSizeByLOD[2] = bytes_med;
    mSizeByLOD[3] = bytes_high;
    F32 METADATA_DISCOUNT = (F32) gSavedSettings.getU32("MeshMetaDataDiscount");
    F32 MINIMUM_SIZE = (F32) gSavedSettings.getU32("MeshMinimumByteSize");
    F32 bytes_per_triangle = (F32) gSavedSettings.getU32("MeshBytesPerTriangle");
    if (!(bytes_per_triangle > 0.f))
        bytes_per_triangle = 16.f;
    for (S32 i=0; i<4; i++)
    {
        mEstTrisByLOD[i] = llmax((F32) mSizeByLOD[i]-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
    }
    return true;
}
S32 LLMeshCostData::getSizeByLOD(S32 lod)
{
    if (llclamp(lod,0,3) != lod)
    {
        return 0;
    }
    return mSizeByLOD[lod];
}
S32 LLMeshCostData::getSizeTotal()
{
    return mSizeByLOD[0] + mSizeByLOD[1] + mSizeByLOD[2] + mSizeByLOD[3];
}
F32 LLMeshCostData::getEstTrisByLOD(S32 lod)
{
    if (llclamp(lod,0,3) != lod)
    {
        return 0.f;
    }
    return mEstTrisByLOD[lod];
}
F32 LLMeshCostData::getEstTrisMax()
{
    return llmax(mEstTrisByLOD[0], mEstTrisByLOD[1], mEstTrisByLOD[2], mEstTrisByLOD[3]);
}
F32 LLMeshCostData::getRadiusWeightedTris(F32 radius)
{
	F32 max_distance = 512.f;
	F32 dlowest = llmin(radius/0.03f, max_distance);
	F32 dlow = llmin(radius/0.06f, max_distance);
	F32 dmid = llmin(radius/0.24f, max_distance);
	F32 triangles_lowest = mEstTrisByLOD[0];
	F32 triangles_low = mEstTrisByLOD[1];
	F32 triangles_mid = mEstTrisByLOD[2];
	F32 triangles_high = mEstTrisByLOD[3];
	F32 max_area = 102944.f;
	F32 min_area = 1.f;
	F32 high_area = llmin(F_PI*dmid*dmid, max_area);
	F32 mid_area = llmin(F_PI*dlow*dlow, max_area);
	F32 low_area = llmin(F_PI*dlowest*dlowest, max_area);
	F32 lowest_area = max_area;
	lowest_area -= low_area;
	low_area -= mid_area;
	mid_area -= high_area;
	high_area = llclamp(high_area, min_area, max_area);
	mid_area = llclamp(mid_area, min_area, max_area);
	low_area = llclamp(low_area, min_area, max_area);
	lowest_area = llclamp(lowest_area, min_area, max_area);
	F32 total_area = high_area + mid_area + low_area + lowest_area;
	high_area /= total_area;
	mid_area /= total_area;
	low_area /= total_area;
	lowest_area /= total_area;
	F32 weighted_avg = triangles_high*high_area +
					   triangles_mid*mid_area +
					   triangles_low*low_area +
					   triangles_lowest*lowest_area;
    return weighted_avg;
}
F32 LLMeshCostData::getEstTrisForStreamingCost()
{
    LL_DEBUGS("StreamingCost") << "tris_by_lod: "
                               << mEstTrisByLOD[0] << ", "
                               << mEstTrisByLOD[1] << ", "
                               << mEstTrisByLOD[2] << ", "
                               << mEstTrisByLOD[3] << LL_ENDL;
    F32 charged_tris = mEstTrisByLOD[3];
    F32 allowed_tris = mEstTrisByLOD[3];
    const F32 ENFORCE_FLOOR = 64.0f;
    for (S32 i=2; i>=0; i--)
    {
        allowed_tris = llclamp(allowed_tris/2.0f,ENFORCE_FLOOR,mEstTrisByLOD[i]);
        F32 excess_tris = mEstTrisByLOD[i]-allowed_tris;
        if (excess_tris>0.f)
        {
            LL_DEBUGS("StreamingCost") << "excess tris in lod[" << i << "] " << excess_tris << " allowed " << allowed_tris <<  LL_ENDL;
            charged_tris += excess_tris;
        }
    }
    return charged_tris;
}
F32 LLMeshCostData::getRadiusBasedStreamingCost(F32 radius)
{
	U32 budget = gSavedSettings.getU32("MeshTriangleBudget");
	if (budget == 0)
		budget = 250000;
	return getRadiusWeightedTris(radius) / (F32)budget * 15000.f;
}
F32 LLMeshCostData::getTriangleBasedStreamingCost()
{
    F32 result = ANIMATED_OBJECT_COST_PER_KTRI * 0.001 * getEstTrisForStreamingCost();
    return result;
}
bool LLMeshRepository::getCostData(LLUUID mesh_id, LLMeshCostData& data)
{
	data = LLMeshCostData();
	if (mThread && mesh_id.notNull())
	{
		LLMutexLock lock(mThread->mHeaderMutex);
		LLMeshRepoThread::mesh_header_map::iterator iter = mThread->mMeshHeader.find(mesh_id);
		if (iter != mThread->mMeshHeader.end() && mThread->mMeshHeaderSize[mesh_id] > 0)
		{
			const LLSD& header = iter->second;
			bool header_invalid = (header.has("404")
				|| !header.has("lowest_lod")
				|| (header.has("version") && header["version"].asInteger() > MAX_MESH_VERSION));
			if (!header_invalid)
			{
				return getCostData(header, data);
			}
			return true;
		}
	}
	return false;
}
bool LLMeshRepository::getCostData(LLSD& header, LLMeshCostData& data)
{
	data = LLMeshCostData();
	if (!data.init(header))
	{
		return false;
	}
	return true;
}
LLPhysicsDecomp::LLPhysicsDecomp()
:	LLThread("Physics Decomp")
{
	mInited = false;
	mQuitting = false;
	mDone = false;
	mSignal = new LLCondition;
	mMutex = new LLMutex;
}
LLPhysicsDecomp::~LLPhysicsDecomp()
{
	shutdown();
	delete mSignal;
	mSignal = NULL;
	delete mMutex;
	mMutex = NULL;
}
void LLPhysicsDecomp::shutdown()
{
	if (mSignal)
	{
		mQuitting = true;
		mSignal->signal();
		while (!isStopped())
		{
			apr_sleep(10);
		}
	}
}
void LLPhysicsDecomp::submitRequest(LLPhysicsDecomp::Request* request)
{
	LLMutexLock lock(mMutex);
	mRequestQ.push(request);
	mSignal->signal();
}
S32 LLPhysicsDecomp::llcdCallback(const char* status, S32 p1, S32 p2)
{
	if (gMeshRepo.mDecompThread && gMeshRepo.mDecompThread->mCurRequest.notNull())
	{
		return gMeshRepo.mDecompThread->mCurRequest->statusCallback(status, p1, p2);
	}
	return 1;
}
bool needTriangles( LLConvexDecomposition *aDC )
{
	if( !aDC )
		return false;
	LLCDParam const  *pParams(0);
	int nParams = aDC->getParameters( &pParams );
	if( nParams <= 0 )
		return false;
	for( int i = 0; i < nParams; ++i )
	{
		if( pParams[i].mName && strcmp( "nd_AlwaysNeedTriangles", pParams[i].mName ) == 0 )
		{
			if( LLCDParam::LLCD_BOOLEAN == pParams[i].mType && pParams[i].mDefault.mBool )
				return true;
			else
				return false;
		}
	}
	return false;
}
void LLPhysicsDecomp::setMeshData(LLCDMeshData& mesh, bool vertex_based)
{
	if (vertex_based)
	{
		if (LLConvexDecomposition* pDeComp = LLConvexDecomposition::getInstance())
			vertex_based = !needTriangles(pDeComp);
		else
			return;
	}
	mesh.mVertexBase = mCurRequest->mPositions[0].mV;
	mesh.mVertexStrideBytes = 12;
	mesh.mNumVertices = mCurRequest->mPositions.size();
	if(!vertex_based)
	{
		mesh.mIndexType = LLCDMeshData::INT_16;
		mesh.mIndexBase = &(mCurRequest->mIndices[0]);
		mesh.mIndexStrideBytes = 6;
		mesh.mNumTriangles = mCurRequest->mIndices.size()/3;
	}
	if ((vertex_based || mesh.mNumTriangles > 0) && mesh.mNumVertices > 2)
	{
		LLCDResult ret = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			ret  = LLConvexDecomposition::getInstance()->setMeshData(&mesh, vertex_based);
		}
		if (ret)
		{
			LL_ERRS(LOG_MESH) << "Convex Decomposition thread valid but could not set mesh data." << LL_ENDL;
		}
	}
}
void LLPhysicsDecomp::doDecomposition()
{
	LLCDMeshData mesh;
	S32 stage = mStageID[mCurRequest->mStage];
	if (LLConvexDecomposition::getInstance() == NULL)
	{
		return;
	}
	if (stage == 0)
	{
		setMeshData(mesh, false);
	}
	std::map<std::string, const LLCDParam*> param_map;
	static const LLCDParam* params = NULL;
	static S32 param_count = 0;
	if (!params)
	{
		param_count = LLConvexDecomposition::getInstance()->getParameters(&params);
	}
	for (S32 i = 0; i < param_count; ++i)
	{
		param_map[params[i].mName] = params+i;
	}
	LLCDResult ret = LLCD_OK;
	for (decomp_params::iterator iter = mCurRequest->mParams.begin(); iter != mCurRequest->mParams.end(); ++iter)
	{
		const std::string& name = iter->first;
		const LLSD& value = iter->second;
		const LLCDParam* param = param_map[name];
		if (param == NULL)
		{
			continue;
		}
		if (param->mType == LLCDParam::LLCD_FLOAT)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, (F32) value.asReal());
		}
		else if (param->mType == LLCDParam::LLCD_INTEGER ||
				 param->mType == LLCDParam::LLCD_ENUM)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, value.asInteger());
		}
		else if (param->mType == LLCDParam::LLCD_BOOLEAN)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, value.asBoolean());
		}
	}
	mCurRequest->setStatusMessage("Executing.");
	if (LLConvexDecomposition::getInstance() != NULL)
	{
		ret = LLConvexDecomposition::getInstance()->executeStage(stage);
	}
	if (ret)
	{
		LL_WARNS(LOG_MESH) << "Convex Decomposition thread valid but could not execute stage " << stage << "."
						   << LL_ENDL;
		LLMutexLock lock(mMutex);
		mCurRequest->mHull.clear();
		mCurRequest->mHullMesh.clear();
		mCurRequest->setStatusMessage("FAIL");
		completeCurrent();
	}
	else
	{
		mCurRequest->setStatusMessage("Reading results");
		S32 num_hulls =0;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			num_hulls = LLConvexDecomposition::getInstance()->getNumHullsFromStage(stage);
		}
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull.clear();
			mCurRequest->mHull.resize(num_hulls);
			mCurRequest->mHullMesh.clear();
			mCurRequest->mHullMesh.resize(num_hulls);
		}
		for (S32 i = 0; i < num_hulls; ++i)
		{
			std::vector<LLVector3> p;
			LLCDHull hull;
			LLConvexDecomposition::getInstance()->getHullFromStage(stage, i, &hull);
			const F32* v = hull.mVertexBase;
			for (S32 j = 0; j < hull.mNumVertices; ++j)
			{
				LLVector3 vert(v[0], v[1], v[2]);
				p.push_back(vert);
				v = (F32*) (((U8*) v) + hull.mVertexStrideBytes);
			}
			LLCDMeshData mesh;
			LLConvexDecomposition::getInstance()->getMeshFromStage(stage, i, &mesh);
			get_vertex_buffer_from_mesh(mesh, mCurRequest->mHullMesh[i]);
			{
				LLMutexLock lock(mMutex);
				mCurRequest->mHull[i] = p;
			}
		}
		{
			LLMutexLock lock(mMutex);
			mCurRequest->setStatusMessage("FAIL");
			completeCurrent();
		}
	}
}
void LLPhysicsDecomp::completeCurrent()
{
	LLMutexLock lock(mMutex);
	mCompletedQ.push(mCurRequest);
	mCurRequest = NULL;
}
void LLPhysicsDecomp::notifyCompleted()
{
	if (!mCompletedQ.empty())
	{
		LLMutexLock lock(mMutex);
		while (!mCompletedQ.empty())
		{
			Request* req = mCompletedQ.front();
			req->completed();
			mCompletedQ.pop();
		}
	}
}
void make_box(LLPhysicsDecomp::Request * request)
{
	LLVector3 min,max;
	min = request->mPositions[0];
	max = min;
	for (U32 i = 0; i < request->mPositions.size(); ++i)
	{
		update_min_max(min, max, request->mPositions[i]);
	}
	request->mHull.clear();
	LLModel::hull box;
	box.push_back(LLVector3(min[0],min[1],min[2]));
	box.push_back(LLVector3(max[0],min[1],min[2]));
	box.push_back(LLVector3(min[0],max[1],min[2]));
	box.push_back(LLVector3(max[0],max[1],min[2]));
	box.push_back(LLVector3(min[0],min[1],max[2]));
	box.push_back(LLVector3(max[0],min[1],max[2]));
	box.push_back(LLVector3(min[0],max[1],max[2]));
	box.push_back(LLVector3(max[0],max[1],max[2]));
	request->mHull.push_back(box);
}
void LLPhysicsDecomp::doDecompositionSingleHull()
{
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();
	if (decomp == NULL)
	{
		return;
	}
	LLCDMeshData mesh;
	setMeshData(mesh, true);
	LLCDResult ret = decomp->buildSingleHull() ;
	if(ret)
	{
		LL_WARNS(LOG_MESH) << "Could not execute decomposition stage when attempting to create single hull." << LL_ENDL;
		make_box(mCurRequest);
	}
	else
	{
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull.clear();
			mCurRequest->mHull.resize(1);
			mCurRequest->mHullMesh.clear();
		}
		std::vector<LLVector3> p;
		LLCDHull hull;
		decomp->getSingleHull(&hull);
		const F32* v = hull.mVertexBase;
		for (S32 j = 0; j < hull.mNumVertices; ++j)
		{
			LLVector3 vert(v[0], v[1], v[2]);
			p.push_back(vert);
			v = (F32*) (((U8*) v) + hull.mVertexStrideBytes);
		}
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull[0] = p;
		}
	}
	{
		completeCurrent();
	}
}
#ifdef ND_HASCONVEXDECOMP_TRACER
class ndDecompTracer: public ndConvexDecompositionTracer
{
	int mRefCount;
public:
	ndDecompTracer()
		: mRefCount(0)
	{
	}
	virtual ~ndDecompTracer() { }
	virtual void trace( char const *a_strMsg )
	{
		LL_INFOS() << a_strMsg << LL_ENDL;
	}
	virtual void startTraceData( char const *a_strWhat)
	{
		LL_INFOS() << a_strWhat << LL_ENDL;
	}
	virtual void traceData( char const *a_strData )
	{
		LL_INFOS() << a_strData << LL_ENDL;
	}
	virtual void endTraceData()
	{
	}
	virtual int getLevel()
	{
		return eTraceFunctions;
	}
	virtual void addref()
	{
		++mRefCount;
	}
	virtual void release()
	{
		--mRefCount;
		if( mRefCount == 0 )
			delete this;
	}
};
#endif
void LLPhysicsDecomp::run()
{
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();
	if (decomp == NULL)
	{
		mInited = true;
		return;
	}
#ifdef ND_HASCONVEXDECOMP_TRACER
	ndConvexDecompositionTracable *pTraceable = dynamic_cast< ndConvexDecompositionTracable* >( decomp );
	if( pTraceable )
		pTraceable->setTracer( new ndDecompTracer() );
#endif
	decomp->initThread();
	mInited = true;
	static const LLCDStageData* stages = NULL;
	static S32 num_stages = 0;
	if (!stages)
	{
		num_stages = decomp->getStages(&stages);
	}
	for (S32 i = 0; i < num_stages; i++)
	{
		mStageID[stages[i].mName] = i;
	}
	mSignal->lock();
	while (!mQuitting)
	{
		mSignal->wait();
		while (!mQuitting && !mRequestQ.empty())
		{
			{
				LLMutexLock lock(mMutex);
				mCurRequest = mRequestQ.front();
				mRequestQ.pop();
			}
			S32& id = *(mCurRequest->mDecompID);
			if (id == -1)
			{
				decomp->genDecomposition(id);
			}
			decomp->bindDecomposition(id);
			if (mCurRequest->mStage == "single_hull")
			{
				doDecompositionSingleHull();
			}
			else
			{
				doDecomposition();
			}
		}
	}
	decomp->quitThread();
	if (mSignal->isLocked())
	{
		mSignal->unlock();
	}
	mDone = true;
}
void LLPhysicsDecomp::Request::assignData(LLModel* mdl)
{
	if (!mdl)
	{
		return ;
	}
	U16 index_offset = 0;
	U16 tri[3] ;
	mPositions.clear();
	mIndices.clear();
	mBBox[1] = LLVector3(F32_MIN, F32_MIN, F32_MIN) ;
	mBBox[0] = LLVector3(F32_MAX, F32_MAX, F32_MAX) ;
	for (S32 i = 0; i < mdl->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& face = mdl->getVolumeFace(i);
		if (mPositions.size() + face.mNumVertices > 65535)
		{
			continue;
		}
		for (U32 j = 0; j < (U32)face.mNumVertices; ++j)
		{
			mPositions.push_back(LLVector3(face.mPositions[j].getF32ptr()));
			for(U32 k = 0 ; k < 3 ; k++)
			{
				mBBox[0].mV[k] = llmin(mBBox[0].mV[k], mPositions[j].mV[k]) ;
				mBBox[1].mV[k] = llmax(mBBox[1].mV[k], mPositions[j].mV[k]) ;
			}
		}
		updateTriangleAreaThreshold() ;
		for (U32 j = 0; j+2 < (U32)face.mNumIndices; j += 3)
		{
			tri[0] = face.mIndices[j] + index_offset ;
			tri[1] = face.mIndices[j + 1] + index_offset ;
			tri[2] = face.mIndices[j + 2] + index_offset ;
			if(isValidTriangle(tri[0], tri[1], tri[2]))
			{
				mIndices.push_back(tri[0]);
				mIndices.push_back(tri[1]);
				mIndices.push_back(tri[2]);
			}
		}
		index_offset += face.mNumVertices;
	}
	return ;
}
void LLPhysicsDecomp::Request::updateTriangleAreaThreshold()
{
	F32 range = mBBox[1].mV[0] - mBBox[0].mV[0] ;
	range = llmin(range, mBBox[1].mV[1] - mBBox[0].mV[1]) ;
	range = llmin(range, mBBox[1].mV[2] - mBBox[0].mV[2]) ;
	mTriangleAreaThreshold = llmin(0.0002f, range * 0.000002f) ;
}
bool LLPhysicsDecomp::Request::isValidTriangle(U16 idx1, U16 idx2, U16 idx3)
{
	LLVector3 a = mPositions[idx2] - mPositions[idx1] ;
	LLVector3 b = mPositions[idx3] - mPositions[idx1] ;
	F32 c = a * b ;
	return ((a*a) * (b*b) - c * c) > mTriangleAreaThreshold ;
}
void LLPhysicsDecomp::Request::setStatusMessage(const std::string& msg)
{
	mStatusMessage = msg;
}
void LLMeshRepository::buildPhysicsMesh(LLModel::Decomposition& decomp)
{
	decomp.mMesh.resize(decomp.mHull.size());
	for (U32 i = 0; i < decomp.mHull.size(); ++i)
	{
		LLCDHull hull;
		hull.mNumVertices = decomp.mHull[i].size();
		hull.mVertexBase = decomp.mHull[i][0].mV;
		hull.mVertexStrideBytes = 12;
		LLCDMeshData mesh;
		LLCDResult res = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
		}
		if (res == LLCD_OK)
		{
			get_vertex_buffer_from_mesh(mesh, decomp.mMesh[i]);
		}
	}
	if (!decomp.mBaseHull.empty() && decomp.mBaseHullMesh.empty())
	{
		LLCDHull hull;
		hull.mNumVertices = decomp.mBaseHull.size();
		hull.mVertexBase = decomp.mBaseHull[0].mV;
		hull.mVertexStrideBytes = 12;
		LLCDMeshData mesh;
		LLCDResult res = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
		}
		if (res == LLCD_OK)
		{
			get_vertex_buffer_from_mesh(mesh, decomp.mBaseHullMesh);
		}
	}
}
bool LLMeshRepository::meshUploadEnabled()
{
	LLViewerRegion *region = gAgent.getRegion();
	static const LLCachedControl<bool> mesh_enabled("MeshEnabled");
	if(mesh_enabled &&
	   region)
	{
		return region->meshUploadEnabled();
	}
	return false;
}
bool LLMeshRepository::meshRezEnabled()
{
	LLViewerRegion *region = gAgent.getRegion();
	static const LLCachedControl<bool> mesh_enabled("MeshEnabled");
	if(mesh_enabled &&
	   region)
	{
		return region->meshRezEnabled();
	}
	return false;
}
