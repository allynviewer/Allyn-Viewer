/** 
 * @file llassetstorage.h
 * @brief definition of LLAssetStorage class which allows simple
 * up/downloads of uuid,type asets
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
#ifndef LL_LLASSETSTORAGE_H
#define LL_LLASSETSTORAGE_H
#include <string>
#include <functional>
#include "lluuid.h"
#include "lltimer.h"
#include "llnamevalue.h"
#include "llhost.h"
#include "stdenums.h"
#include "lltransfermanager.h"
#include "llassettype.h"
#include "llstring.h"
#include "llextendedstatus.h"
class LLMessageSystem;
class LLXferManager;
class LLAssetStorage;
class LLVFS;
class LLSD;
const F32Minutes LL_ASSET_STORAGE_TIMEOUT(5);
const int LL_ERR_ASSET_REQUEST_FAILED = -1;
const int LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE = -3;
const int LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE = -4;
const int LL_ERR_INSUFFICIENT_PERMISSIONS = -5;
const int LL_ERR_PRICE_MISMATCH = -23018;
typedef std::function<void(LLVFS *vfs, const LLUUID &asset_id, LLAssetType::EType asset_type, void *user_data, S32 status, LLExtStat ext_status)> LLGetAssetCallback;
typedef std::function<void(const LLUUID &asset_id, void *user_data, S32 status, LLExtStat ext_status)> LLStoreAssetCallback;
class LLAssetInfo
{
protected:
	std::string		mDescription;
	std::string		mName;
public:
	LLUUID			mUuid;
	LLTransactionID	mTransactionID;
	LLUUID			mCreatorID;
	LLAssetType::EType	mType;
	LLAssetInfo( void );
	LLAssetInfo( const LLUUID& object_id, const LLUUID& creator_id,
				 LLAssetType::EType type, const char* name, const char* desc );
	LLAssetInfo( const LLNameValue& nv );
	const std::string& getName( void ) const { return mName; }
	const std::string& getDescription( void ) const { return mDescription; }
	void setName( const std::string& name );
	void setDescription( const std::string& desc );
	void setFromNameValue( const LLNameValue& nv );
};
class LLBaseDownloadRequest
{
public:
	LLBaseDownloadRequest(const LLUUID &uuid, const LLAssetType::EType at);
	virtual ~LLBaseDownloadRequest();
	LLUUID getUUID() const					{ return mUUID; }
	LLAssetType::EType getType() const		{ return mType; }
	void setUUID(const LLUUID& id) { mUUID = id; }
	void setType(LLAssetType::EType type) { mType = type; }
	virtual LLBaseDownloadRequest* getCopy();
protected:
	LLUUID	mUUID;
	LLAssetType::EType mType;
public:
    LLGetAssetCallback mDownCallback;
	void	*mUserData;
	LLHost  mHost;
	BOOL	mIsTemp;
	F64Seconds		mTime;
	BOOL    mIsPriority;
	BOOL	mDataSentInFirstPacket;
	BOOL	mDataIsInVFS;
};
class LLAssetRequest : public LLBaseDownloadRequest
{
public:
	LLAssetRequest(const LLUUID &uuid, const LLAssetType::EType at);
	virtual ~LLAssetRequest();
	void setTimeout(F64Seconds timeout) { mTimeout = timeout; }
	virtual LLBaseDownloadRequest* getCopy();
    LLStoreAssetCallback mUpCallback;
	void	(*mInfoCallback)(LLAssetInfo *, void *, S32);
	BOOL	mIsLocal;
	BOOL	mIsUserWaiting;
	F64Seconds		mTimeout;
	LLUUID	mRequestingAgentID;
	virtual LLSD getTerseDetails() const;
	virtual LLSD getFullDetails() const;
};
template <class T>
struct ll_asset_request_equal : public std::equal_to<T>
{
	bool operator()(const T& x, const T& y) const
	{
		return (	x->getType() == y->getType()
				&&	x->getUUID() == y->getUUID() );
	}
};
class LLInvItemRequest : public LLBaseDownloadRequest
{
public:
	LLInvItemRequest(const LLUUID &uuid, const LLAssetType::EType at);
	virtual ~LLInvItemRequest();
	virtual LLBaseDownloadRequest* getCopy();
};
class LLEstateAssetRequest : public LLBaseDownloadRequest
{
public:
	LLEstateAssetRequest(const LLUUID &uuid, const LLAssetType::EType at, EstateAssetType et);
	virtual ~LLEstateAssetRequest();
	LLAssetType::EType getAType() const		{ return mType; }
	virtual LLBaseDownloadRequest* getCopy();
protected:
	EstateAssetType mEstateAssetType;
};
typedef std::map<LLUUID,U64,lluuid_less> toxic_asset_map_t;
class LLAssetStorage
{
public:
	LLVFS *mVFS;
	LLVFS *mStaticVFS;
    typedef ::LLStoreAssetCallback LLStoreAssetCallback;
    typedef ::LLGetAssetCallback LLGetAssetCallback;
	enum ERequestType
	{
		RT_INVALID = -1,
		RT_DOWNLOAD = 0,
		RT_UPLOAD = 1,
		RT_LOCALUPLOAD = 2,
		RT_COUNT = 3
	};
protected:
	BOOL mShutDown;
	LLHost mUpstreamHost;
	LLMessageSystem *mMessageSys;
	LLXferManager	*mXferManager;
	typedef std::list<LLAssetRequest*> request_list_t;
	request_list_t mPendingDownloads;
	request_list_t mPendingUploads;
	request_list_t mPendingLocalUploads;
	toxic_asset_map_t	mToxicAssetMap;
public:
	LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs, const LLHost &upstream_host);
	LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs);
	virtual ~LLAssetStorage();
	void setUpstream(const LLHost &upstream_host);
	bool isUpstreamOK() const
	{
		return mUpstreamHost.isOk();
	}
	BOOL hasLocalAsset(const LLUUID &uuid, LLAssetType::EType type);
	void getAssetData(const LLUUID uuid, LLAssetType::EType atype, LLGetAssetCallback cb, void *user_data, BOOL is_priority = FALSE);
	uuid_vec_t mBlackListedAsset;
	virtual void storeAssetData(
		const LLTransactionID& tid,
		LLAssetType::EType atype,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool store_local = false,
		bool user_waiting= false,
		F64Seconds timeout=LL_ASSET_STORAGE_TIMEOUT) = 0;
	void checkForTimeouts();
	void getEstateAsset(const LLHost &object_sim, const LLUUID &agent_id, const LLUUID &session_id,
									const LLUUID &asset_id, LLAssetType::EType atype, EstateAssetType etype,
									 LLGetAssetCallback callback, void *user_data, BOOL is_priority);
	void getInvItemAsset(const LLHost &object_sim,
						 const LLUUID &agent_id, const LLUUID &session_id,
						 const LLUUID &owner_id, const LLUUID &task_id, const LLUUID &item_id,
						 const LLUUID &asset_id, LLAssetType::EType atype,
						 LLGetAssetCallback cb, void *user_data, BOOL is_priority = FALSE);
	BOOL		isAssetToxic( const LLUUID& uuid );
	void		flushOldToxicAssets( BOOL force_it );
	void		markAssetToxic( const LLUUID& uuid );
protected:
	bool findInStaticVFSAndInvokeCallback(const LLUUID& uuid, LLAssetType::EType type,
										  LLGetAssetCallback callback, void *user_data);
	LLSD getPendingDetailsImpl(const request_list_t* requests,
	 				LLAssetType::EType asset_type,
	 				const std::string& detail_prefix) const;
	LLSD getPendingRequestImpl(const request_list_t* requests,
							LLAssetType::EType asset_type,
							const LLUUID& asset_id) const;
	bool deletePendingRequestImpl(request_list_t* requests,
							LLAssetType::EType asset_type,
							const LLUUID& asset_id);
public:
	static const LLAssetRequest* findRequest(const request_list_t* requests,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id);
	static LLAssetRequest* findRequest(request_list_t* requests,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id);
	request_list_t* getRequestList(ERequestType rt);
	const request_list_t* getRequestList(ERequestType rt) const;
	static std::string getRequestName(ERequestType rt);
	S32 getNumPendingDownloads() const;
	S32 getNumPendingUploads() const;
	S32 getNumPendingLocalUploads();
	S32 getNumPending(ERequestType rt) const;
	LLSD getPendingDetails(ERequestType rt,
	 				LLAssetType::EType asset_type,
	 				const std::string& detail_prefix) const;
	LLSD getPendingRequest(ERequestType rt,
							LLAssetType::EType asset_type,
							const LLUUID& asset_id) const;
	bool deletePendingRequest(ERequestType rt,
							LLAssetType::EType asset_type,
							const LLUUID& asset_id);
	static void removeAndCallbackPendingDownloads(	const LLUUID& file_id, LLAssetType::EType file_type,
													const LLUUID& callback_id, LLAssetType::EType callback_type,
													S32 result_code, LLExtStat ext_status);
	static void downloadCompleteCallback(
		S32 result,
		const LLUUID& file_id,
		LLAssetType::EType file_type,
		LLBaseDownloadRequest* user_data, LLExtStat ext_status);
	static void downloadEstateAssetCompleteCallback(
		S32 result,
		const LLUUID& file_id,
		LLAssetType::EType file_type,
		LLBaseDownloadRequest* user_data, LLExtStat ext_status);
	static void downloadInvItemCompleteCallback(
		S32 result,
		const LLUUID& file_id,
		LLAssetType::EType file_type,
		LLBaseDownloadRequest* user_data, LLExtStat ext_status);
	static void uploadCompleteCallback(const LLUUID&, void *user_data, S32 result, LLExtStat ext_status);
	static void processUploadComplete(LLMessageSystem *msg, void **this_handle);
	static const char* getErrorString( S32 status );
	void getAssetData(const LLUUID uuid, LLAssetType::EType type, void (*callback)(const char*, const LLUUID&, void *, S32, LLExtStat), void *user_data, BOOL is_priority = FALSE);
	virtual void storeAssetData(
		const std::string& filename,
		const LLTransactionID &transaction_id,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void *user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool user_waiting = false,
		F64Seconds timeout  = LL_ASSET_STORAGE_TIMEOUT) = 0;
	static void legacyGetDataCallback(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType, void *user_data, S32 status, LLExtStat ext_status);
	static void legacyStoreDataCallback(const LLUUID &uuid, void *user_data, S32 status, LLExtStat ext_status);
protected:
	void _cleanupRequests(BOOL all, S32 error);
	void _callUploadCallbacks(const LLUUID &uuid, const LLAssetType::EType asset_type, BOOL success, LLExtStat ext_status);
	virtual void _queueDataRequest(const LLUUID& uuid, LLAssetType::EType type, LLGetAssetCallback callback,
								   void *user_data, BOOL duplicate,
								   BOOL is_priority) = 0;
private:
	void _init(LLMessageSystem *msg,
			   LLXferManager *xfer,
			   LLVFS *vfs,
			   LLVFS *static_vfs,
			   const LLHost &upstream_host);
protected:
	enum EMetricResult
	{
		MR_INVALID			= -1,
		MR_OKAY				= 0,
		MR_ZERO_SIZE		= 1,
		MR_BAD_FUNCTION		= 2,
		MR_FILE_NONEXIST	= 3,
		MR_NO_FILENAME		= 4,
		MR_NO_UPSTREAM		= 5,
		MR_VFS_CORRUPTION	= 6
	};
	static class LLMetrics *metric_recipient;
	static void reportMetric( const LLUUID& asset_id, const LLAssetType::EType asset_type, const std::string& filename,
							  const LLUUID& agent_id, S32 asset_size, EMetricResult result,
							  const char* file, const S32 line, const std::string& message );
public:
	static void setMetricRecipient( LLMetrics *recip )
	{
		metric_recipient = recip;
	}
};
class LLLegacyAssetRequest
{
public:
	void	(*mDownCallback)(const char *, const LLUUID&, void *, S32, LLExtStat);
	LLStoreAssetCallback mUpCallback;
	void	*mUserData;
};
extern LLAssetStorage *gAssetStorage;
extern const LLUUID CATEGORIZE_LOST_AND_FOUND_ID;
#endif
