/**
 * @file llmarketplacefunctions.h
 * @brief Miscellaneous marketplace-related functions and classes
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#ifndef LL_LLMARKETPLACEFUNCTIONS_H
#define LL_LLMARKETPLACEFUNCTIONS_H
#include <llsd.h>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include "llsingleton.h"
#include "llstring.h"
#include "llhttpstatuscodes.h"
namespace MarketplaceErrorCodes
{
	enum eCode
	{
		IMPORT_DONE = 200,
		IMPORT_PROCESSING = 202,
		IMPORT_REDIRECT = 302,
		IMPORT_BAD_REQUEST = 400,
		IMPORT_AUTHENTICATION_ERROR = 401,
		IMPORT_FORBIDDEN = 403,
		IMPORT_NOT_FOUND = 404,
		IMPORT_DONE_WITH_ERRORS = 409,
		IMPORT_JOB_FAILED = 410,
		IMPORT_JOB_LOW_SPEED = HTTP_INTERNAL_ERROR_LOW_SPEED,
		IMPORT_JOB_TIMEOUT = 499,
		IMPORT_SERVER_SITE_DOWN = 500,
		IMPORT_SERVER_API_DISABLED = 503,
	};
}
namespace MarketplaceStatusCodes
{
	enum sCode
	{
		MARKET_PLACE_NOT_INITIALIZED = 0,
		MARKET_PLACE_INITIALIZING = 1,
		MARKET_PLACE_CONNECTION_FAILURE = 2,
		MARKET_PLACE_NOT_MERCHANT = 3,
		MARKET_PLACE_MERCHANT = 4,
		MARKET_PLACE_NOT_MIGRATED_MERCHANT = 5,
		MARKET_PLACE_MIGRATED_MERCHANT = 6
	};
}
namespace MarketplaceFetchCodes
{
	enum sCode
	{
		MARKET_FETCH_NOT_DONE = 0,
		MARKET_FETCH_LOADING = 1,
		MARKET_FETCH_FAILED = 2,
		MARKET_FETCH_DONE = 3
	};
}
class LLMarketplaceInventoryImporter
	: public LLSingleton<LLMarketplaceInventoryImporter>
{
public:
	static void update();
	LLMarketplaceInventoryImporter();
	typedef boost::signals2::signal<void (bool)> status_changed_signal_t;
	typedef boost::signals2::signal<void (U32, const LLSD&)> status_report_signal_t;
	boost::signals2::connection setInitializationErrorCallback(const status_report_signal_t::slot_type& cb);
	boost::signals2::connection setStatusChangedCallback(const status_changed_signal_t::slot_type& cb);
	boost::signals2::connection setStatusReportCallback(const status_report_signal_t::slot_type& cb);
	void initialize();
	bool triggerImport();
	bool isImportInProgress() const { return mImportInProgress; }
	bool isInitialized() const { return mInitialized; }
	U32 getMarketPlaceStatus() const { return mMarketPlaceStatus; }
protected:
	void reinitializeAndTriggerImport();
	void updateImport();
private:
	bool mAutoTriggerImport;
	bool mImportInProgress;
	bool mInitialized;
	U32 mMarketPlaceStatus;
	status_report_signal_t *	mErrorInitSignal;
	status_changed_signal_t *	mStatusChangedSignal;
	status_report_signal_t *	mStatusReportSignal;
};
namespace SLMErrorCodes
{
	enum eCode
	{
		SLM_SUCCESS = 200,
		SLM_RECORD_CREATED = 201,
		SLM_MALFORMED_PAYLOAD = 400,
		SLM_NOT_FOUND = 404,
	};
}
class LLMarketplaceData;
class LLInventoryObserver;
class LLMarketplaceTuple
{
public:
	friend class LLMarketplaceData;
	LLMarketplaceTuple();
	LLMarketplaceTuple(const LLUUID& folder_id);
	LLMarketplaceTuple(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed = false);
private:
	LLUUID mListingFolderId;
	S32 mListingId;
	LLUUID mVersionFolderId;
	bool mIsActive;
	S32 mCountOnHand;
	std::string mEditURL;
};
typedef std::map<LLUUID, LLMarketplaceTuple> marketplace_items_list_t;
typedef std::map<LLUUID, LLUUID> version_folders_list_t;
class LLSLMGetMerchantResponder;
class LLSLMGetListingsResponder;
class LLSLMCreateListingsResponder;
class LLSLMGetListingResponder;
class LLSLMUpdateListingsResponder;
class LLSLMAssociateListingsResponder;
class LLSLMDeleteListingsResponder;
class LLMarketplaceData
	: public LLSingleton<LLMarketplaceData>
{
public:
	friend class LLSLMGetMerchantResponder;
	friend class LLSLMGetListingsResponder;
	friend class LLSLMCreateListingsResponder;
	friend class LLSLMGetListingResponder;
	friend class LLSLMUpdateListingsResponder;
	friend class LLSLMAssociateListingsResponder;
	friend class LLSLMDeleteListingsResponder;
    static LLSD getMarketplaceStringSubstitutions();
	LLMarketplaceData();
	virtual ~LLMarketplaceData();
	typedef boost::signals2::signal<void()> status_updated_signal_t;
	void initializeSLM(const status_updated_signal_t::slot_type& cb);
	U32 getSLMStatus() const { return mMarketPlaceStatus; }
	void setSLMStatus(U32 status);
	void getSLMListings();
	bool isEmpty() const { return (mMarketplaceItems.size() == 0); }
	void setDataFetchedSignal(const status_updated_signal_t::slot_type& cb);
	void setSLMDataFetched(U32 status);
	U32 getSLMDataFetched() { return mMarketPlaceDataFetched; }
	bool createListing(const LLUUID& folder_id);
	bool activateListing(const LLUUID& folder_id, bool activate, S32 depth = -1);
	bool clearListing(const LLUUID& folder_id, S32 depth = -1);
	bool setVersionFolder(const LLUUID& folder_id, const LLUUID& version_id, S32 depth = -1);
	bool associateListing(const LLUUID& folder_id, const LLUUID& source_folder_id, S32 listing_id);
	bool updateCountOnHand(const LLUUID& folder_id, S32 depth = -1);
	bool getListing(const LLUUID& folder_id, S32 depth = -1);
	bool getListing(S32 listing_id);
	bool deleteListing(S32 listing_id, bool update = true);
	bool isListed(const LLUUID& folder_id);
	bool isListedAndActive(const LLUUID& folder_id);
	bool isVersionFolder(const LLUUID& folder_id);
	bool isInActiveFolder(const LLUUID& obj_id, S32 depth = -1);
	LLUUID getActiveFolder(const LLUUID& obj_id, S32 depth = -1);
	bool isUpdating(const LLUUID& folder_id, S32 depth = -1);
	bool getActivationState(const LLUUID& folder_id);
	S32 getListingID(const LLUUID& folder_id);
	LLUUID getVersionFolder(const LLUUID& folder_id);
	std::string getListingURL(const LLUUID& folder_id, S32 depth = -1);
	LLUUID getListingFolder(S32 listing_id);
	S32 getCountOnHand(const LLUUID& folder_id);
	bool checkDirtyCount() { if (mDirtyCount) { mDirtyCount = false; return true; } else { return false; } }
	void setDirtyCount() { mDirtyCount = true; }
	void setUpdating(const LLUUID& folder_id, bool isUpdating);
	void setValidationWaiting(const LLUUID& folder_id, S32 count);
	void decrementValidationWaiting(const LLUUID& folder_id, S32 count = 1);
private:
	bool addListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed, const std::string& edit_url, S32 count);
	bool deleteListing(const LLUUID& folder_id, bool update = true);
	bool setListingID(const LLUUID& folder_id, S32 listing_id, bool update = true);
	bool setVersionFolderID(const LLUUID& folder_id, const LLUUID& version_id, bool update = true);
	bool setActivationState(const LLUUID& folder_id, bool activate, bool update = true);
	bool setListingURL(const LLUUID& folder_id, const std::string& edit_url, bool update = true);
	bool setCountOnHand(const LLUUID& folder_id, S32 count, bool update = true);
	void createSLMListing(const LLUUID& folder_id, const LLUUID& version_id, S32 count);
	void getSLMListing(S32 listing_id);
	void updateSLMListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed, S32 count);
	void associateSLMListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, const LLUUID& source_folder_id);
	void deleteSLMListing(S32 listing_id);
	std::string getSLMConnectURL(const std::string& route);
	U32 mMarketPlaceStatus;
	status_updated_signal_t* mStatusUpdatedSignal;
	LLInventoryObserver* mInventoryObserver;
	bool mDirtyCount;
	U32 mMarketPlaceDataFetched;
	status_updated_signal_t* mDataFetchedSignal;
	uuid_set_t mPendingUpdateSet;
	typedef std::map<LLUUID,S32> waiting_list_t;
	waiting_list_t mValidationWaitingList;
	marketplace_items_list_t mMarketplaceItems;
	version_folders_list_t mVersionFolders;
};
#endif
