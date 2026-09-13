/** 
 * @file llmeshrepository.h
 * @brief Client-side repository of mesh assets.
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
#ifndef LL_MESH_REPOSITORY_H
#define LL_MESH_REPOSITORY_H
#include "llassettype.h"
#include "llmodel.h"
#include "lluuid.h"
#include "llviewertexture.h"
#include "llvolume.h"
#define LLCONVEXDECOMPINTER_STATIC 1
#include "llconvexdecomposition.h"
#include "lluploadfloaterobservers.h"
#include "aistatemachinethread.h"
#include <absl/container/node_hash_map.h>
#ifndef BOOST_FUNCTION_HPP_INCLUDED
#include <boost/function.hpp>
#define BOOST_FUNCTION_HPP_INCLUDED
#endif
class LLVOVolume;
class LLMeshResponder;
class LLMutex;
class LLCondition;
class LLVFS;
class LLMeshRepository;
class AIMeshUpload;
class LLMeshUploadData
{
public:
	LLPointer<LLModel> mBaseModel;
	LLPointer<LLModel> mModel[5];
	LLUUID mUUID;
	U32 mRetries;
	std::string mRSVP;
	std::string mAssetData;
	LLSD mPostData;
	LLMeshUploadData()
	{
		mRetries = 0;
	}
};
class LLTextureUploadData
{
public:
	LLViewerFetchedTexture* mTexture;
	LLUUID mUUID;
	std::string mRSVP;
	std::string mLabel;
	U32 mRetries;
	std::string mAssetData;
	LLSD mPostData;
	LLTextureUploadData()
	{
		mRetries = 0;
	}
	LLTextureUploadData(LLViewerFetchedTexture* texture, std::string& label)
		: mTexture(texture), mLabel(label)
	{
		mRetries = 0;
	}
};
class LLPhysicsDecomp : public LLThread
{
public:
	typedef std::map<std::string, LLSD> decomp_params;
	class Request : public LLRefCount
	{
	public:
		S32* mDecompID;
		std::string mStage;
		std::vector<LLVector3> mPositions;
		std::vector<U16> mIndices;
		decomp_params mParams;
		std::string mStatusMessage;
		std::vector<LLModel::PhysicsMesh> mHullMesh;
		LLModel::convex_hull_decomposition mHull;
		virtual S32 statusCallback(const char* status, S32 p1, S32 p2) = 0;
		virtual void completed() = 0;
		virtual void setStatusMessage(const std::string& msg);
		bool isValid() const {return mPositions.size() > 2 && mIndices.size() > 2 ;}
	protected:
		LLVector3 mBBox[2] ;
		F32 mTriangleAreaThreshold ;
		void assignData(LLModel* mdl) ;
		void updateTriangleAreaThreshold() ;
		bool isValidTriangle(U16 idx1, U16 idx2, U16 idx3) ;
	};
	LLCondition* mSignal;
	LLMutex* mMutex;
	bool mInited;
	bool mQuitting;
	bool mDone;
	LLPhysicsDecomp();
	~LLPhysicsDecomp();
	void shutdown();
	void submitRequest(Request* request);
	static S32 llcdCallback(const char*, S32, S32);
	void cancel();
	void setMeshData(LLCDMeshData& mesh, bool vertex_based);
	void doDecomposition();
	void doDecompositionSingleHull();
	virtual void run();
	void completeCurrent();
	void notifyCompleted();
	std::map<std::string, S32> mStageID;
	typedef std::queue<LLPointer<Request> > request_queue;
	request_queue mRequestQ;
	LLPointer<Request> mCurRequest;
	std::queue<LLPointer<Request> > mCompletedQ;
};
class LLMeshRepoThread : public LLThread
{
public:
	static S32 sActiveHeaderRequests;
	static S32 sActiveLODRequests;
	static U32 sMaxConcurrentRequests;
	LLMutex*	mMutex;
	LLMutex*	mHeaderMutex;
	LLCondition*	mSignal;
	typedef std::map<LLUUID, LLSD> mesh_header_map;
	mesh_header_map mMeshHeader;
	std::map<LLUUID, U32> mMeshHeaderSize;
	struct MeshRequest
	{
		LLTimer mTimer;
		LLVolumeParams mMeshParams;
		MeshRequest(const LLVolumeParams&  mesh_params) : mMeshParams(mesh_params)
		{
			mTimer.start();
		}
		virtual ~MeshRequest() {}
		virtual void preFetch() {}
		virtual bool fetch(U32& count) = 0;
	};
	class HeaderRequest : public MeshRequest
	{
	public:
		HeaderRequest(const LLVolumeParams&  mesh_params)
			: MeshRequest(mesh_params)
		{}
		bool fetch(U32& count);
		bool operator<(const HeaderRequest& rhs) const
		{
			return mMeshParams < rhs.mMeshParams;
		}
	};
	class LODRequest : public MeshRequest
	{
	public:
		S32 mLOD;
		F32 mScore;
		LODRequest(const LLVolumeParams&  mesh_params, S32 lod)
			: MeshRequest(mesh_params), mLOD(lod), mScore(0.f)
		{}
		void preFetch();
		bool fetch(U32& count);
	};
	struct CompareScoreGreater
	{
		bool operator()(const LODRequest& lhs, const LODRequest& rhs)
		{
			return lhs.mScore > rhs.mScore;
		}
	};
	class LoadedMesh
	{
	public:
		LLPointer<LLVolume> mVolume;
		LLVolumeParams mMeshParams;
		S32 mLOD;
		LoadedMesh(LLVolume* volume, const LLVolumeParams&  mesh_params, S32 lod)
			: mVolume(volume), mMeshParams(mesh_params), mLOD(lod)
		{
		}
	};
	struct MeshHeaderInfo
	{
		MeshHeaderInfo()
			: mHeaderSize(0), mVersion(0), mOffset(-1), mSize(0) {}
		U32 mHeaderSize;
		U32 mVersion;
		S32 mOffset;
		S32 mSize;
	};
	uuid_set_t mSkinRequests;
	std::queue<LLMeshSkinInfo> mSkinInfoQ;
	LLMutex* mSkinInfoQMutex;
	uuid_set_t mDecompositionRequests;
	uuid_set_t mPhysicsShapeRequests;
	std::queue<LLModel::Decomposition*> mDecompositionQ;
	LLMutex* mDecompositionQMutex;
	std::deque<std::pair<std::shared_ptr<MeshRequest>, F32> > mHeaderReqQ;
	std::deque<std::pair<std::shared_ptr<MeshRequest>, F32> > mLODReqQ;
	std::queue<LODRequest> mUnavailableQ;
	std::queue<LoadedMesh> mLoadedQ;
	struct PendingLODDecode
	{
		LLVolumeParams mMeshParams;
		S32 mLOD;
		U8* mData;
		S32 mDataSize;
		S32 mOffset;
		S32 mRequestedBytes;
		PendingLODDecode()
			: mLOD(0), mData(NULL), mDataSize(0), mOffset(0), mRequestedBytes(0)
		{}
	};
	std::queue<PendingLODDecode> mPendingLODDecodeQ;
	typedef std::map<LLVolumeParams, std::vector<S32> > pending_lod_map;
	pending_lod_map mPendingLOD;
	static std::string constructUrl(LLUUID mesh_id);
	LLMeshRepoThread();
	~LLMeshRepoThread();
	void runQueue(std::deque<std::pair<std::shared_ptr<MeshRequest>, F32> >& queue, U32& count, S32& active_requests);
	void runSet(uuid_set_t& set, std::function<bool (const LLUUID& mesh_id)> fn);
	void pushHeaderRequest(const LLVolumeParams& mesh_params, F32 delay = 0)
	{
		std::shared_ptr<LLMeshRepoThread::MeshRequest> req;
		req.reset(new LLMeshRepoThread::HeaderRequest(mesh_params));
		mHeaderReqQ.push_back(std::make_pair(req, delay));
	}
	void pushLODRequest(const LLVolumeParams& mesh_params, S32 lod, F32 delay = 0)
	{
		std::shared_ptr<LLMeshRepoThread::MeshRequest> req;
		req.reset(new LLMeshRepoThread::LODRequest(mesh_params, lod));
		mLODReqQ.push_back(std::make_pair(req, delay));
	}
	virtual void run();
	void lockAndLoadMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	void loadMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	bool fetchMeshHeader(const LLVolumeParams& mesh_params, U32& count);
	bool fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod, U32& count);
	bool headerReceived(const LLVolumeParams& mesh_params, U8* data, S32 data_size);
	bool lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size);
	void queueLODDecode(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size, S32 offset, S32 requested_bytes);
	void processPendingLODDecodes();
	bool skinInfoReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool decompositionReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	LLSD& getMeshHeader(const LLUUID& mesh_id);
	bool getMeshHeaderInfo(const LLUUID& mesh_id, const char* block_name, MeshHeaderInfo& info);
	bool loadInfoFromVFS(const LLUUID& mesh_id, MeshHeaderInfo& info, boost::function<bool(const LLUUID&, U8*, S32)> fn);
	void notifyLoadedMeshes();
	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	void loadMeshSkinInfo(const LLUUID& mesh_id);
	void loadMeshDecomposition(const LLUUID& mesh_id);
	void loadMeshPhysicsShape(const LLUUID& mesh_id);
	bool fetchMeshSkinInfo(const LLUUID& mesh_id);
	bool fetchMeshDecomposition(const LLUUID& mesh_id);
	bool fetchMeshPhysicsShape(const LLUUID& mesh_id);
	static void incActiveLODRequests();
	static void decActiveLODRequests();
	static void incActiveHeaderRequests();
	static void decActiveHeaderRequests();
};
class LLMeshUploadThread : public AIThreadImpl
{
private:
	S32 mMeshUploadTimeOut ;
public:
	class DecompRequest : public LLPhysicsDecomp::Request
	{
	public:
		LLPointer<LLModel> mModel;
		LLPointer<LLModel> mBaseModel;
		LLMeshUploadThread* mThread;
		DecompRequest(LLModel* mdl, LLModel* base_model, LLMeshUploadThread* thread);
		S32 statusCallback(const char* status, S32 p1, S32 p2) { return 1; }
		void completed();
	};
	LLPointer<DecompRequest> mFinalDecomp;
	bool mPhysicsComplete;
	LLSD mModelData;
	LLSD mBody;
	typedef std::map<LLPointer<LLModel>, std::vector<LLVector3> > hull_map;
	hull_map mHullMap;
	typedef std::vector<LLModelInstance> instance_list;
	instance_list mInstanceList;
	typedef std::map<LLPointer<LLModel>, instance_list> instance_map;
	instance_map mInstance;
	LLVector3		mOrigin;
	bool			mUploadTextures;
	bool			mUploadSkin;
	bool			mUploadJoints;
	LLHost			mHost;
	std::string		mWholeModelFeeCapability;
#ifdef LL_DEBUG
	LLMeshUploadThread(void) : AIThreadImpl("mesh upload") { }
#endif
	void init(instance_list& data, LLVector3& scale, bool upload_textures, bool upload_skin, bool upload_joints, bool do_upload,
		LLHandle<LLWholeModelFeeObserver> const& fee_observer, LLHandle<LLWholeModelUploadObserver> const& upload_observer);
	~LLMeshUploadThread();
	void postRequest(std::string& url, AIMeshUpload* state_machine);
	virtual bool run();
	void preStart();
	void generateHulls();
	void wholeModelToLLSD(LLSD& dest, bool include_textures);
	void decomposeMeshMatrix(LLMatrix4& transformation,
							 LLVector3& result_pos,
							 LLQuaternion& result_rot,
							 LLVector3& result_scale);
	void setFeeObserverHandle(LLHandle<LLWholeModelFeeObserver> observer_handle) { mFeeObserverHandle = observer_handle; }
	void setUploadObserverHandle(LLHandle<LLWholeModelUploadObserver> observer_handle) { mUploadObserverHandle = observer_handle; }
	LLViewerFetchedTexture* FindViewerTexture(const LLImportMaterial& material);
private:
	LLHandle<LLWholeModelFeeObserver> mFeeObserverHandle;
	LLHandle<LLWholeModelUploadObserver> mUploadObserverHandle;
	bool mDoUpload;
};
enum AIMeshUpload_state_type {
	AIMeshUpload_start = AIStateMachine::max_state,
	AIMeshUpload_threadFinished,
	AIMeshUpload_responderFinished
};
class AIMeshUpload : public AIStateMachine
{
private:
	LLPointer<AIStateMachineThread<LLMeshUploadThread> > mMeshUpload;
	std::string mWholeModelUploadURL;
public:
	AIMeshUpload(LLMeshUploadThread::instance_list& data, LLVector3& scale,
		bool upload_textures, bool upload_skin, bool upload_joints, std::string const& upload_url, bool do_upload,
		LLHandle<LLWholeModelFeeObserver> const& fee_observer, LLHandle<LLWholeModelUploadObserver> const& upload_observer);
	void setWholeModelUploadURL(std::string const& whole_model_upload_url) { mWholeModelUploadURL = whole_model_upload_url; }
	const char* getName() const { return "AIMeshUpload"; }
protected:
	const char* state_str_impl(state_type run_state) const;
	void initialize_impl();
	void multiplex_impl(state_type run_state);
};
class LLMeshCostData
{
public:
    LLMeshCostData();
    bool init(const LLSD& header);
    S32 getSizeByLOD(S32 lod);
    S32 getSizeTotal();
    F32 getEstTrisByLOD(S32 lod);
    F32 getEstTrisMax();
    F32 getRadiusWeightedTris(F32 radius);
    F32 getEstTrisForStreamingCost();
    F32 getRadiusBasedStreamingCost(F32 radius);
    F32 getTriangleBasedStreamingCost();
private:
    std::vector<S32> mSizeByLOD;
    std::vector<F32> mEstTrisByLOD;
};
class LLMeshRepository
{
public:
	static U32 sBytesReceived;
	static U32 sHTTPRequestCount;
	static U32 sHTTPRetryCount;
	static U32 sLODPending;
	static U32 sLODProcessing;
	static U32 sCacheBytesRead;
	static U32 sCacheBytesWritten;
	static U32 sPeakKbps;
	static U32 sSkinParseErrors;
	F32 getEstTrianglesMax(LLUUID mesh_id);
	F32 getEstTrianglesStreamingCost(LLUUID mesh_id);
	F32 getStreamingCostLegacy(LLUUID mesh_id, F32 radius, S32* bytes = NULL, S32* visible_bytes = NULL, S32 detail = -1, F32 *unscaled_value = NULL);
	static F32 getStreamingCostLegacy(LLSD& header, F32 radius, S32* bytes = NULL, S32* visible_bytes = NULL, S32 detail = -1, F32 *unscaled_value = NULL);
	bool getCostData(LLUUID mesh_id, LLMeshCostData& data);
	static bool getCostData(LLSD& header, LLMeshCostData& data);
	LLMeshRepository();
	void init();
	void shutdown();
	void unregisterMesh(LLVOVolume* volume);
	S32 loadMesh(LLVOVolume* volume, const LLVolumeParams& mesh_params, S32 detail = 0, S32 last_lod = -1);
	void notifyLoadedMeshes();
	void notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volume);
	void notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod);
	void notifySkinInfoReceived(LLMeshSkinInfo& info);
	void notifyDecompositionReceived(LLModel::Decomposition* info);
	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	static S32 getActualMeshLOD(LLSD& header, S32 lod);
	const LLMeshSkinInfo* getSkinInfo(const LLUUID& mesh_id, const LLVOVolume* requesting_obj);
	LLModel::Decomposition* getDecomposition(const LLUUID& mesh_id);
	void fetchPhysicsShape(const LLUUID& mesh_id);
	bool hasPhysicsShape(const LLUUID& mesh_id);
	void buildHull(const LLVolumeParams& params, S32 detail);
	void buildPhysicsMesh(LLModel::Decomposition& decomp);
	bool meshUploadEnabled();
	bool meshRezEnabled();
	LLSD& getMeshHeader(const LLUUID& mesh_id);
	void uploadModel(std::vector<LLModelInstance>& data, LLVector3& scale, bool upload_textures,
					 bool upload_skin, bool upload_joints, std::string upload_url, bool do_upload = true,
					 LLHandle<LLWholeModelFeeObserver> fee_observer= (LLHandle<LLWholeModelFeeObserver>()), LLHandle<LLWholeModelUploadObserver> upload_observer = (LLHandle<LLWholeModelUploadObserver>()));
	S32 getMeshSize(const LLUUID& mesh_id, S32 lod);
	typedef std::map<LLVolumeParams, std::vector<LLVOVolume*> > mesh_load_map;
	mesh_load_map mLoadingMeshes[4];
	typedef absl::node_hash_map<LLUUID, LLMeshSkinInfo> skin_map;
	skin_map mSkinMap;
	typedef std::map<LLUUID, LLModel::Decomposition*> decomposition_map;
	decomposition_map mDecompositionMap;
	LLMutex*					mMeshMutex;
	std::vector<LLMeshRepoThread::LODRequest> mPendingRequests;
	typedef std::map<LLUUID, uuid_set_t > skin_load_map;
	skin_load_map mLoadingSkins;
	std::queue<LLUUID> mPendingSkinRequests;
	uuid_set_t mLoadingDecompositions;
	std::queue<LLUUID> mPendingDecompositionRequests;
	uuid_set_t mLoadingPhysicsShapes;
	std::queue<LLUUID> mPendingPhysicsShapeRequests;
	U32 mMeshThreadCount;
	void cacheOutgoingMesh(LLMeshUploadData& data, LLSD& header);
	LLMeshRepoThread* mThread;
	LLPhysicsDecomp* mDecompThread;
	class inventory_data
	{
	public:
		LLSD mPostData;
		LLSD mResponse;
		inventory_data(const LLSD& data, const LLSD& content)
			: mPostData(data), mResponse(content)
		{
		}
	};
	std::queue<inventory_data> mInventoryQ;
	std::queue<LLSD> mUploadErrorQ;
	void uploadError(LLSD& args);
	void updateInventory(inventory_data data);
	std::string mGetMeshCapability;
};
extern LLMeshRepository gMeshRepo;
const F32 ANIMATED_OBJECT_BASE_COST = 15.0f;
const F32 ANIMATED_OBJECT_COST_PER_KTRI = 1.5f;
#endif
