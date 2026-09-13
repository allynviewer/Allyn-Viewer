/** 
 * @file llviewerassetstorage.cpp
 * @brief Subclass capable of loading asset data to/from an external source.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#include "llviewerassetstorage.h"
#include "llvfile.h"
#include "llvfs.h"
#include "message.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "lltransfersourceasset.h"
#include "lltransfertargetvfile.h"
#include "llviewerassetstats.h"
#include "llworld.h"
class LLViewerAssetRequest : public LLAssetRequest
{
public:
	LLViewerAssetRequest(const LLUUID &uuid, const LLAssetType::EType type, bool with_http)
		: LLAssetRequest(uuid, type),
		  mMetricsStartTime(0),
		  mWithHTTP(with_http)
		{
		}
	LLViewerAssetRequest & operator=(const LLViewerAssetRequest &);
	~LLViewerAssetRequest()
		{
			recordMetrics();
		}
	LLBaseDownloadRequest* getCopy()
	{
		return new LLViewerAssetRequest(*this);
	}
	bool operator==(const LLViewerAssetRequest& rhs) const { return mUUID == rhs.mUUID && mType == rhs.mType && mWithHTTP == rhs.mWithHTTP; }
protected:
	void recordMetrics()
		{
			if (mMetricsStartTime.value())
			{
				LLViewerAssetStatsFF::record_dequeue_main(mType, mWithHTTP, false);
				LLViewerAssetStatsFF::record_response_main(mType, mWithHTTP, false,
														   (LLViewerAssetStatsFF::get_timestamp()
															- mMetricsStartTime));
				mMetricsStartTime = (U32Seconds)0;
			}
		}
public:
	LLViewerAssetStats::duration_t		mMetricsStartTime;
	bool mWithHTTP;
};
LLViewerAssetStorage::LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
										   LLVFS *vfs, LLVFS *static_vfs,
										   const LLHost &upstream_host)
		: LLAssetStorage(msg, xfer, vfs, static_vfs, upstream_host)
{
}
LLViewerAssetStorage::LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
										   LLVFS *vfs, LLVFS *static_vfs)
		: LLAssetStorage(msg, xfer, vfs, static_vfs)
{
}
void LLViewerAssetStorage::storeAssetData(
	const LLTransactionID& tid,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority,
	bool store_local,
	bool user_waiting,
	F64Seconds timeout)
{
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	LL_DEBUGS("AssetStorage") << "LLViewerAssetStorage::storeAssetData (legacy) " << tid << ":" << LLAssetType::lookup(asset_type)
			<< " ASSET_ID: " << asset_id << LL_ENDL;
	if (mUpstreamHost.isOk())
	{
		if (mVFS->getExists(asset_id, asset_type))
		{
			U8 buffer[MTUBYTES];
			buffer[0] = 0;
			LLVFile vfile(mVFS, asset_id, asset_type, LLVFile::READ);
			S32 asset_size = vfile.getSize();
			LLAssetRequest *req = new LLAssetRequest(asset_id, asset_type);
			req->mUpCallback = callback;
			req->mUserData = user_data;
			if (asset_size < 1)
			{
				LL_WARNS("AssetStorage") << "LLViewerAssetStorage::storeAssetData()  Data _should_ already be in the VFS, but it's not! " << asset_id << LL_ENDL;
				reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file didn't exist or was zero length (VFS - can't tell which)" );
				delete req;
				if (callback)
				{
					callback(asset_id, user_data, LL_ERR_ASSET_REQUEST_FAILED, LL_EXSTAT_VFS_CORRUPT);
				}
				return;
			}
			else
			{
				S32 size = mVFS->getSize(asset_id, asset_type);
				const char *message = "Added to upload queue";
				reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, size, MR_OKAY, __FILE__, __LINE__, message );
				if(is_priority)
				{
					mPendingUploads.push_front(req);
				}
				else
				{
					mPendingUploads.push_back(req);
				}
			}
			if (asset_size + 100 < MTUBYTES)
			{
				BOOL res = vfile.read(buffer, asset_size);
				S32 bytes_read = res ? vfile.getLastBytesRead() : 0;
				if( bytes_read == asset_size )
				{
					req->mDataSentInFirstPacket = TRUE;
				}
				else
				{
					LL_WARNS("AssetStorage") << "Probable corruption in VFS file, aborting store asset data" << LL_ENDL;
					reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, asset_size, MR_VFS_CORRUPTION, __FILE__, __LINE__, "VFS corruption" );
					if (callback)
					{
						callback(asset_id, user_data, LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE, LL_EXSTAT_VFS_CORRUPT);
					}
					return;
				}
			}
			else
			{
				buffer[0] = 0;
				asset_size = 0;
			}
			mMessageSys->newMessageFast(_PREHASH_AssetUploadRequest);
			mMessageSys->nextBlockFast(_PREHASH_AssetBlock);
			mMessageSys->addUUIDFast(_PREHASH_TransactionID, tid);
			mMessageSys->addS8Fast(_PREHASH_Type, (S8)asset_type);
			mMessageSys->addBOOLFast(_PREHASH_Tempfile, temp_file);
			mMessageSys->addBOOLFast(_PREHASH_StoreLocal, store_local);
			mMessageSys->addBinaryDataFast( _PREHASH_AssetData, buffer, asset_size );
			mMessageSys->sendReliable(mUpstreamHost);
		}
		else
		{
			LL_WARNS("AssetStorage") << "AssetStorage: attempt to upload non-existent vfile " << asset_id << ":" << LLAssetType::lookup(asset_type) << LL_ENDL;
			reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file didn't exist or was zero length (VFS - can't tell which)" );
			if (callback)
			{
				callback(asset_id, user_data,  LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE, LL_EXSTAT_NONEXISTENT_FILE);
			}
		}
	}
	else
	{
		LL_WARNS("AssetStorage") << "Attempt to move asset store request upstream w/o valid upstream provider" << LL_ENDL;
		reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_NO_UPSTREAM, __FILE__, __LINE__, "No upstream provider" );
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CIRCUIT_GONE, LL_EXSTAT_NO_UPSTREAM);
		}
	}
}
void LLViewerAssetStorage::storeAssetData(
	const std::string& filename,
	const LLTransactionID& tid,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority,
	bool user_waiting,
	F64Seconds timeout)
{
	if(filename.empty())
	{
		reportMetric( LLUUID::null, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_VFS_CORRUPTION, __FILE__, __LINE__, "Filename missing" );
		LL_ERRS() << "No filename specified" << LL_ENDL;
		return;
	}
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	LL_DEBUGS("AssetStorage") << "LLViewerAssetStorage::storeAssetData (legacy)" << asset_id << ":" << LLAssetType::lookup(asset_type) << LL_ENDL;
	LL_DEBUGS("AssetStorage") << "ASSET_ID: " << asset_id << LL_ENDL;
	S32 size = 0;
	LLFILE* fp = LLFile::fopen(filename, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}
	if( size )
	{
		LLLegacyAssetRequest *legacy = new LLLegacyAssetRequest;
		legacy->mUpCallback = callback;
		legacy->mUserData = user_data;
		LLVFile file(mVFS, asset_id, asset_type, LLVFile::WRITE);
		file.setMaxSize(size);
		const S32 buf_size = 65536;
		U8 copy_buf[buf_size];
		while ((size = (S32)fread(copy_buf, 1, buf_size, fp)))
		{
			file.write(copy_buf, size);
		}
		fclose(fp);
		if (temp_file)
		{
			LLFile::remove(filename);
		}
		LLViewerAssetStorage::storeAssetData(
			tid,
			asset_type,
			legacyStoreDataCallback,
			(void**)legacy,
			temp_file,
			is_priority);
	}
	else
	{
		if( fp )
		{
			reportMetric( asset_id, asset_type, filename, LLUUID::null, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file was zero length" );
			fclose(fp);
		}
		else
		{
			reportMetric( asset_id, asset_type, filename, LLUUID::null, 0, MR_FILE_NONEXIST, __FILE__, __LINE__, "The file didn't exist" );
		}
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CANNOT_OPEN_FILE, LL_EXSTAT_BLOCKED_FILE);
		}
	}
}
void LLViewerAssetStorage::_queueDataRequest(
	const LLUUID& uuid,
	LLAssetType::EType atype,
	LLGetAssetCallback callback,
	void *user_data,
	BOOL duplicate,
	BOOL is_priority)
{
	queueRequestUDP(uuid, atype, callback, user_data, duplicate, is_priority);
}
void LLViewerAssetStorage::queueRequestUDP(
	const LLUUID& uuid,
	LLAssetType::EType atype,
	LLGetAssetCallback callback,
	void *user_data,
	BOOL duplicate,
	BOOL is_priority)
{
    LL_DEBUGS("ViewerAsset") << "Request asset via HTTP " << uuid << " type " << LLAssetType::lookup(atype) << LL_ENDL;
	if (mUpstreamHost.isOk())
	{
		const auto region = gAgent.getRegion();
		bool with_http = !region || !region->capabilitiesReceived() || !region->getViewerAssetUrl().empty();
		LLViewerAssetRequest *req = new LLViewerAssetRequest(uuid, atype, with_http);
		req->mDownCallback = callback;
		req->mUserData = user_data;
		req->mIsPriority = is_priority;
		if (!duplicate)
		{
			req->mMetricsStartTime = LLViewerAssetStatsFF::get_timestamp();
		}
		mPendingDownloads.push_back(req);
		if (!duplicate)
		{
			bool is_temp = false;
			LLViewerAssetStatsFF::record_enqueue_main(atype, with_http, is_temp);
			if (!with_http)
			{
				LLTransferSourceParamsAsset spa;
				spa.setAsset(uuid, atype);
				LLTransferTargetParamsVFile tpvf;
				tpvf.setAsset(uuid, atype);
				tpvf.setCallback(downloadCompleteCallback, *req);
				LL_DEBUGS("AssetStorage") << "Starting transfer for " << uuid << LL_ENDL;
				LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(mUpstreamHost, LLTCT_ASSET);
				ttcp->requestTransfer(spa, tpvf, 100.f + (is_priority ? 1.f : 0.f));
			}
			else
			{
				LLViewerAssetStorage::assetRequestCoro(req, uuid, atype, callback, user_data);
			}
		}
	}
	else
	{
		LL_WARNS() << "Attempt to move asset data request upstream w/o valid upstream provider" << LL_ENDL;
		if (callback)
		{
			callback(mVFS, uuid, atype, user_data, LL_ERR_CIRCUIT_GONE, LL_EXSTAT_NO_UPSTREAM);
		}
	}
}
extern AIHTTPTimeoutPolicy HTTPGetResponder_timeout;
class LLViewerAssetResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	LLViewerAssetResponder(const LLUUID& id, LLAssetType::EType type) : LLHTTPClient::ResponderWithCompleted()
	, uuid(id), atype(type)
	{}
private:
	LLUUID uuid;
	LLAssetType::EType atype;
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer) override
	{
		if (LLApp::isQuitting())
		{
			return;
		}
		LL_DEBUGS("ViewerAsset") << "request succeeded, url " << mURL << LL_ENDL;
		S32 result_code = LL_ERR_NOERR;
		LLExtStat ext_status = LL_EXSTAT_NONE;
		if (!isGoodStatus(mStatus))
		{
			LL_DEBUGS("ViewerAsset") << "request failed, status " << mStatus << LL_ENDL;
			result_code = LL_ERR_ASSET_REQUEST_FAILED;
			ext_status = LL_EXSTAT_NONE;
		}
		else
		{
			std::string raw;
			decode_raw_body(channels, buffer, raw);
			S32 size = raw.size();
			if (size > 0)
			{
				LLUUID temp_id;
				temp_id.generate();
				LLVFile vf(gAssetStorage->mVFS, temp_id, atype, LLVFile::WRITE);
				vf.setMaxSize(size);
				if (!vf.write((const U8*)raw.data(), size))
				{
					LL_WARNS("ViewerAsset") << "Failure in vf.write()" << LL_ENDL;
					result_code = LL_ERR_ASSET_REQUEST_FAILED;
					ext_status = LL_EXSTAT_VFS_CORRUPT;
				}
				else if (!vf.rename(uuid, atype))
				{
					LL_WARNS("ViewerAsset") << "rename failed" << LL_ENDL;
					result_code = LL_ERR_ASSET_REQUEST_FAILED;
					ext_status = LL_EXSTAT_VFS_CORRUPT;
				}
			}
			else
			{
				LL_WARNS("ViewerAsset") << "bad size" << LL_ENDL;
				result_code = LL_ERR_ASSET_REQUEST_FAILED;
				ext_status = LL_EXSTAT_NONE;
			}
		}
		gAssetStorage->removeAndCallbackPendingDownloads(uuid, atype, uuid, atype, result_code, ext_status);
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy() const override { return HTTPGetResponder_timeout; }
	char const* getName() const override { return "assetRequestCoro"; }
};
void LLViewerAssetStorage::capsRecvForRegion(const LLUUID& uuid, LLAssetType::EType atype, const LLUUID& region_id)
{
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(region_id);
    if (!regionp)
    {
        LL_WARNS("ViewerAsset") << "region not found for region_id " << region_id << LL_ENDL;
    }
    else
    {
        mViewerAssetUrl = regionp->getViewerAssetUrl();
    }
    LL_WARNS_ONCE("ViewerAsset") << "capsRecv got event" << LL_ENDL;
    LL_WARNS_ONCE("ViewerAsset") << "region " << gAgent.getRegion() << " mViewerAssetUrl " << mViewerAssetUrl << LL_ENDL;
    if (mViewerAssetUrl.empty())
    {
        LL_WARNS_ONCE("ViewerAsset") << "asset request fails: caps received but no viewer asset cap found" << LL_ENDL;
        auto result_code = LL_ERR_ASSET_REQUEST_FAILED;
        auto ext_status = LL_EXSTAT_NONE;
        removeAndCallbackPendingDownloads(uuid, atype, uuid, atype, result_code, ext_status);
		return;
    }
    std::string url = getAssetURL(mViewerAssetUrl, uuid,atype);
    LL_DEBUGS("ViewerAsset") << "request url: " << url << LL_ENDL;
	LLHTTPClient::get(url, new LLViewerAssetResponder(uuid, atype));
}
void LLViewerAssetStorage::assetRequestCoro(
    LLViewerAssetRequest *req,
    const LLUUID uuid,
    LLAssetType::EType atype,
    LLGetAssetCallback callback,
    void *user_data)
{
    if (!gAgent.getRegion())
    {
        LL_WARNS_ONCE("ViewerAsset") << "Asset request fails: no region set" << LL_ENDL;
        auto result_code = LL_ERR_ASSET_REQUEST_FAILED;
        auto ext_status = LL_EXSTAT_NONE;
        removeAndCallbackPendingDownloads(uuid, atype, uuid, atype, result_code, ext_status);
		return;
    }
    else if (!gAgent.getRegion()->capabilitiesReceived())
    {
        LL_WARNS_ONCE("ViewerAsset") << "Waiting for capabilities" << LL_ENDL;
        gAgent.getRegion()->setCapabilitiesReceivedCallback(
            boost::bind(&LLViewerAssetStorage::capsRecvForRegion, this, uuid, atype, _1));
	}
	else capsRecvForRegion(uuid, atype, gAgent.getRegion()->getRegionID());
}
std::string LLViewerAssetStorage::getAssetURL(const std::string& cap_url, const LLUUID& uuid, LLAssetType::EType atype)
{
    std::string type_name = LLAssetType::lookup(atype);
    std::string url = cap_url + "/?" + type_name + "_id=" + uuid.asString();
    return url;
}
