/**
 * @file llassetuploadresponders.cpp
 * @brief Processes responses received for asset upload requests.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
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
#include "llassetuploadresponders.h"
#include "llagent.h"
#include "llagentbenefits.h"
#include "llcompilequeue.h"
#include "llfloaterbuycurrency.h"
#include "statemachine/aifilepicker.h"
#include "llinventorydefines.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llpanelmaininventory.h"
#include "llpermissionsflags.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewgesture.h"
#include "llgesturemgr.h"
#include "llstatusbar.h"
#include "llsdserialize.h"
#include "lluploaddialog.h"
#include "llviewerobject.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewertexlayer.h"
#include "llviewerwindow.h"
#include "lltrans.h"
#include "lldir.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llvfs.h"
static const S32 FILE_COUNT_DISPLAY_THRESHOLD = 5;
void dialog_refresh_all();
void on_new_single_inventory_upload_complete(
	LLAssetType::EType asset_type,
	LLInventoryType::EType inventory_type,
	const std::string inventory_type_string,
	const LLUUID& item_folder_id,
	const std::string& item_name,
	const std::string& item_description,
	const LLSD& server_response,
	S32 upload_price)
{
	if (upload_price > 0)
	{
		LLStatusBar::sendMoneyBalanceRequest();
		LLSD args;
		args["AMOUNT"] = llformat("%d", upload_price);
		LLNotificationsUtil::add("UploadPayment", args);
	}
	if (item_folder_id.notNull())
	{
		U32 everyone_perms = PERM_NONE;
		U32 group_perms = PERM_NONE;
		U32 next_owner_perms = PERM_ALL;
		if (server_response.has("new_next_owner_mask"))
		{
			everyone_perms = server_response["new_everyone_mask"].asInteger();
			group_perms = server_response["new_group_mask"].asInteger();
			next_owner_perms = server_response["new_next_owner_mask"].asInteger();
		}
		else
		{
			if (inventory_type_string != "snapshot")
			{
				next_owner_perms = PERM_MOVE | PERM_TRANSFER;
			}
		}
		LLPermissions new_perms;
		new_perms.init(
			gAgent.getID(),
			gAgent.getID(),
			LLUUID::null,
			LLUUID::null);
		new_perms.initMasks(
			PERM_ALL,
			PERM_ALL,
			everyone_perms,
			group_perms,
			next_owner_perms);
		U32 inventory_item_flags = 0;
		if (server_response.has("inventory_flags"))
		{
			inventory_item_flags = (U32) server_response["inventory_flags"].asInteger();
			if (inventory_item_flags != 0)
			{
				LL_INFOS() << "inventory_item_flags " << inventory_item_flags << LL_ENDL;
			}
		}
		S32 creation_date_now = time_corrected();
		LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem(
			server_response["new_inventory_item"].asUUID(),
			item_folder_id,
			new_perms,
			server_response["new_asset"].asUUID(),
			asset_type,
			inventory_type,
			item_name,
			item_description,
			LLSaleInfo::DEFAULT,
			inventory_item_flags,
			creation_date_now);
		gInventory.updateItem(item);
		gInventory.notifyObservers();
		LLInventoryPanel* panel = LLInventoryPanel::getActiveInventoryPanel();
		if ( panel )
		{
			LLFocusableElement* focus = gFocusMgr.getKeyboardFocus();
			panel->setSelection(
				server_response["new_inventory_item"].asUUID(),
				TAKE_FOCUS_NO);
			if ((LLAssetType::AT_TEXTURE == asset_type || LLAssetType::AT_SOUND == asset_type)
				)
			{
				panel->openSelected();
			}
			gFocusMgr.setKeyboardFocus(focus);
		}
	}
	else
	{
		LL_WARNS() << "Can't find a folder to put it in" << LL_ENDL;
	}
	LLUploadDialog::modalUploadFinished();
}
LLAssetUploadResponder::LLAssetUploadResponder(const LLSD &post_data,
											   const LLUUID& vfile_id,
											   LLAssetType::EType asset_type)
:
	  mPostData(post_data),
	  mVFileID(vfile_id),
	  mAssetType(asset_type)
{
	if (!gVFS->getExists(vfile_id, asset_type))
	{
		LL_WARNS() << "LLAssetUploadResponder called with nonexistant vfile_id" << LL_ENDL;
		mVFileID.setNull();
		mAssetType = LLAssetType::AT_NONE;
		return;
	}
}
LLAssetUploadResponder::LLAssetUploadResponder(
	const LLSD &post_data,
											   const std::string& file_name,
											   LLAssetType::EType asset_type)
:
	  mPostData(post_data),
	  mFileName(file_name),
	  mAssetType(asset_type)
{
}
LLAssetUploadResponder::~LLAssetUploadResponder()
{
	if (!mFileName.empty())
	{
		LLFile::remove(mFileName);
	}
}
void on_failure(const LLAssetType::EType& mAssetType, const LLSD& mPostData, const LLSD& args)
{
	switch (mAssetType)
	{
	case LLAssetType::AT_NOTECARD:
	{
		if (LLPreviewNotecard* nc = (LLPreviewNotecard*)LLPreview::find(mPostData["item_id"]))
			nc->setEnabled(true);
		break;
	}
	case LLAssetType::AT_SCRIPT:
	case LLAssetType::AT_LSL_TEXT:
	case LLAssetType::AT_LSL_BYTECODE:
	{
		if (LLPreviewLSL* lsl = (LLPreviewLSL*)LLPreview::find(mPostData["item_id"]))
			lsl->callbackLSLCompileFailed(LLSD().with(0, "Upload Failure:").with(1, args["REASON"]));
		break;
	}
	default: break;
	}
}
void LLAssetUploadResponder::httpFailure()
{
	LL_INFOS() << "LLAssetUploadResponder::error " << mStatus
			<< " reason: " << mReason << LL_ENDL;
	LLSD args;
	switch(mStatus)
	{
		case 400:
			args["FILE"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
			args["REASON"] = "Error in upload request.  Please visit "
				"http://secondlife.com/support for help fixing this problem.";
			LLNotificationsUtil::add("CannotUploadReason", args);
			break;
		case 500:
		default:
			args["FILE"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
			args["REASON"] = "The server is experiencing unexpected "
				"difficulties.";
			LLNotificationsUtil::add("CannotUploadReason", args);
			break;
	}
	on_failure(mAssetType, mPostData, args);
	LLUploadDialog::modalUploadFinished();
}
void LLAssetUploadResponder::httpSuccess()
{
	const LLSD& content = getContent();
	if (!content.isMap())
	{
		failureResult(HTTP_INTERNAL_ERROR_OTHER, "Malformed response contents", content);
		return;
	}
	LL_DEBUGS() << "LLAssetUploadResponder::result from capabilities" << LL_ENDL;
	const std::string& state = content["state"].asString();
	if (state == "upload")
	{
		uploadUpload(content);
	}
	else if (state == "complete")
	{
		if (mFileName.empty())
		{
			gVFS->renameFile(mVFileID, mAssetType, content["new_asset"].asUUID(), mAssetType);
		}
		uploadComplete(content);
	}
	else
	{
		uploadFailure(content);
	}
}
void LLAssetUploadResponder::uploadUpload(const LLSD& content)
{
	const std::string& uploader = content["uploader"].asString();
	if (mFileName.empty())
	{
		LLHTTPClient::postFile(uploader, mVFileID, mAssetType, this);
	}
	else
	{
		LLHTTPClient::postFile(uploader, mFileName, this);
	}
}
void LLAssetUploadResponder::uploadFailure(const LLSD& content)
{
	LLUploadDialog::modalUploadFinished();
	const std::string& reason = content["state"].asString();
	if (reason == "insufficient funds")
	{
		S32 price;
		if (content.has("upload_price"))
			price = content["upload_price"];
		else
			LLAgentBenefitsMgr::current().findUploadCost(mAssetType, price);
		LLFloaterBuyCurrency::buyCurrency("Uploading costs", price);
	}
	else
	{
		LLSD args;
		args["FILE"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
		args["REASON"] = content["message"].asString();
		LLNotificationsUtil::add("CannotUploadReason", args);
		on_failure(mAssetType, mPostData, args);
	}
}
void LLAssetUploadResponder::uploadComplete(const LLSD& content)
{
}
LLNewAgentInventoryResponder::LLNewAgentInventoryResponder(
	const LLSD& post_data,
	const LLUUID& vfile_id,
	LLAssetType::EType asset_type)
	: LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}
LLNewAgentInventoryResponder::LLNewAgentInventoryResponder(
	const LLSD& post_data,
	const std::string& file_name,
	LLAssetType::EType asset_type)
	: LLAssetUploadResponder(post_data, file_name, asset_type)
{
}
void LLNewAgentInventoryResponder::httpFailure()
{
	LLAssetUploadResponder::httpFailure();
}
void LLNewAgentInventoryResponder::uploadFailure(const LLSD& content)
{
	LLAssetUploadResponder::uploadFailure(content);
}
void LLNewAgentInventoryResponder::uploadComplete(const LLSD& content)
{
	LL_DEBUGS() << "LLNewAgentInventoryResponder::result from capabilities" << LL_ENDL;
	LLAssetType::EType asset_type = LLAssetType::lookup(mPostData["asset_type"].asString());
	LLInventoryType::EType inventory_type = LLInventoryType::lookup(mPostData["inventory_type"].asString());
	S32 expected_upload_cost = 0;
	if (content.has("upload_price"))
		expected_upload_cost = content["upload_price"];
	else if (asset_type == LLAssetType::AT_TEXTURE ||
		asset_type == LLAssetType::AT_SOUND ||
		asset_type == LLAssetType::AT_ANIMATION ||
		asset_type == LLAssetType::AT_MESH)
	{
		LLAgentBenefitsMgr::current().findUploadCost(asset_type, expected_upload_cost);
	}
	LL_INFOS() << "Adding " << content["new_inventory_item"].asUUID() << " "
			<< content["new_asset"].asUUID() << " to inventory." << LL_ENDL;
	on_new_single_inventory_upload_complete(
		asset_type,
		inventory_type,
		mPostData["asset_type"].asString(),
		mPostData["folder_id"].asUUID(),
		mPostData["name"],
		mPostData["description"],
		content,
		expected_upload_cost);
}
LLUpdateAgentInventoryResponder::LLUpdateAgentInventoryResponder(
	const LLSD& post_data,
	const LLUUID& vfile_id,
	LLAssetType::EType asset_type)
	: LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}
LLUpdateAgentInventoryResponder::LLUpdateAgentInventoryResponder(
	const LLSD& post_data,
	const std::string& file_name,
	LLAssetType::EType asset_type)
	: LLAssetUploadResponder(post_data, file_name, asset_type)
{
}
void LLUpdateAgentInventoryResponder::uploadComplete(const LLSD& content)
{
	LL_INFOS() << "LLUpdateAgentInventoryResponder::result from capabilities" << LL_ENDL;
	LLUUID item_id = mPostData["item_id"];
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(item_id);
	if(!item)
	{
		LL_WARNS() << "Inventory item for " << mVFileID
			<< " is no longer in agent inventory." << LL_ENDL;
		return;
	}
	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	new_item->setAssetUUID(content["new_asset"].asUUID());
	gInventory.updateItem(new_item);
	gInventory.notifyObservers();
	LL_INFOS() << "Inventory item " << item->getName() << " saved into "
		<< content["new_asset"].asString() << LL_ENDL;
	LLInventoryType::EType inventory_type = new_item->getInventoryType();
	switch(inventory_type)
	{
		case LLInventoryType::IT_NOTECARD:
		{
			LLPreviewNotecard* nc = (LLPreviewNotecard*)LLPreview::find(new_item->getUUID());
			if (nc)
			{
				if(nc->hasEmbeddedInventory())
				{
					gVFS->removeFile(content["new_asset"].asUUID(), LLAssetType::AT_NOTECARD);
				}
				nc->refreshFromInventory();
			}
			break;
		}
		case LLInventoryType::IT_LSL:
		{
			LLPreviewLSL* preview = (LLPreviewLSL*)LLPreview::find(item_id);
			if (preview)
			{
				if (content["compiled"])
				{
					preview->callbackLSLCompileSucceeded();
				}
				else
				{
					preview->callbackLSLCompileFailed(content["errors"]);
				}
			}
			break;
		}
		case LLInventoryType::IT_GESTURE:
		{
		  if (LLGestureMgr::instance().isGestureActive(item_id))
		  {
			  LLUUID asset_id = new_item->getAssetUUID();
			  LLGestureMgr::instance().replaceGesture(item_id, asset_id);
			  gInventory.notifyObservers();
			}
			LLPreviewGesture* previewp = (LLPreviewGesture*)LLPreview::find(item_id);
			if (previewp)
			{
				previewp->onUpdateSucceeded();
			}
			break;
		}
		case LLInventoryType::IT_WEARABLE:
		default:
			break;
	}
}
LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
															   const LLUUID& vfile_id,
															   LLAssetType::EType asset_type)
:	LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}
LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
															   const std::string& file_name,
															   LLAssetType::EType asset_type)
:	LLAssetUploadResponder(post_data, file_name, asset_type)
{
}
LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
															   const std::string& file_name,
															   const LLUUID& queue_id,
															   LLAssetType::EType asset_type)
:	LLAssetUploadResponder(post_data, file_name, asset_type), mQueueId(queue_id)
{
}
void LLUpdateTaskInventoryResponder::uploadComplete(const LLSD& content)
{
	LL_INFOS() << "LLUpdateTaskInventoryResponder::result from capabilities" << LL_ENDL;
	LLUUID item_id = mPostData["item_id"];
	LLUUID task_id = mPostData["task_id"];
	dialog_refresh_all();
	switch(mAssetType)
	{
		case LLAssetType::AT_NOTECARD:
		{
			LLPreviewNotecard* nc = (LLPreviewNotecard*)LLPreview::find(item_id);
			if (nc)
			{
				if (nc->hasEmbeddedInventory())
				{
					gVFS->removeFile(content["new_asset"].asUUID(),
									 LLAssetType::AT_NOTECARD);
				}
				nc->setAssetId(content["new_asset"].asUUID());
				nc->refreshFromInventory();
			}
			break;
		}
		case LLAssetType::AT_LSL_TEXT:
		{
			if (mQueueId.notNull())
			{
				LLFloaterCompileQueue* queue = (LLFloaterCompileQueue*) LLFloaterScriptQueue::findInstance(mQueueId);
				if (NULL != queue)
				{
					queue->removeItemByItemID(item_id);
				}
			}
			else
			{
				LLLiveLSLEditor* preview = static_cast<LLLiveLSLEditor*>(LLPreview::find(item_id));
				if (preview)
				{
					if (content["compiled"])
					{
						preview->callbackLSLCompileSucceeded(task_id, item_id, mPostData["is_script_running"]);
					}
					else
					{
						preview->callbackLSLCompileFailed(content["errors"]);
					}
				}
			}
			break;
		}
		default:
			break;
	}
}
class LLNewAgentInventoryVariablePriceResponder::Impl
{
public:
	Impl(
		const LLUUID& vfile_id,
		LLAssetType::EType asset_type,
		const LLSD& inventory_data) :
		mVFileID(vfile_id),
		mAssetType(asset_type),
		mInventoryData(inventory_data),
		mFileName("")
	{
		if (!gVFS->getExists(vfile_id, asset_type))
		{
			LL_WARNS()
				<< "LLAssetUploadResponder called with nonexistant "
				<< "vfile_id " << vfile_id << LL_ENDL;
			mVFileID.setNull();
			mAssetType = LLAssetType::AT_NONE;
		}
	}
	Impl(
		const std::string& file_name,
		LLAssetType::EType asset_type,
		const LLSD& inventory_data) :
		mFileName(file_name),
		mAssetType(asset_type),
		mInventoryData(inventory_data)
	{
		mVFileID.setNull();
	}
	std::string getFilenameOrIDString() const
	{
		return (mFileName.empty() ? mVFileID.asString() : mFileName);
	}
	LLUUID getVFileID() const
	{
		return mVFileID;
	}
	std::string getFilename() const
	{
		return mFileName;
	}
	LLAssetType::EType getAssetType() const
	{
		return mAssetType;
	}
	LLInventoryType::EType getInventoryType() const
	{
		return LLInventoryType::lookup(
			mInventoryData["inventory_type"].asString());
	}
	std::string getInventoryTypeString() const
	{
		return mInventoryData["inventory_type"].asString();
	}
	LLUUID getFolderID() const
	{
		return mInventoryData["folder_id"].asUUID();
	}
	std::string getItemName() const
	{
		return mInventoryData["name"].asString();
	}
	std::string getItemDescription() const
	{
		return mInventoryData["description"].asString();
	}
	void displayCannotUploadReason(const std::string& reason)
	{
		LLSD args;
		args["FILE"] = getFilenameOrIDString();
		args["REASON"] = reason;
		LLNotificationsUtil::add("CannotUploadReason", args);
		LLUploadDialog::modalUploadFinished();
	}
	void onApplicationLevelError(const LLSD& error)
	{
		static const std::string _IDENTIFIER = "identifier";
		static const std::string _INSUFFICIENT_FUNDS =
			"NewAgentInventory_InsufficientLindenDollarBalance";
		static const std::string _MISSING_REQUIRED_PARAMETER =
			"NewAgentInventory_MissingRequiredParamater";
		static const std::string _INVALID_REQUEST_BODY =
			"NewAgentInventory_InvalidRequestBody";
		static const std::string _RESOURCE_COST_DIFFERS =
			"NewAgentInventory_ResourceCostDiffers";
		static const std::string _MISSING_PARAMETER = "missing_parameter";
		static const std::string _INVALID_PARAMETER = "invalid_parameter";
		static const std::string _MISSING_RESOURCE = "missing_resource";
		static const std::string _INVALID_RESOURCE = "invalid_resource";
		std::string error_identifier = error[_IDENTIFIER].asString();
		if (_INSUFFICIENT_FUNDS == error_identifier)
		{
			displayCannotUploadReason("You do not have a sufficient L$ balance to complete this upload.");
		}
		else if (_MISSING_REQUIRED_PARAMETER == error_identifier)
		{
			if (error.has(_MISSING_PARAMETER) )
			{
				std::string message =
					"Upload request was missing required parameter '[P]'";
				LLStringUtil::replaceString(
					message,
					"[P]",
					error[_MISSING_PARAMETER].asString());
				displayCannotUploadReason(message);
			}
			else
			{
				std::string message =
					"Upload request was missing a required parameter";
				displayCannotUploadReason(message);
			}
		}
		else if ( _INVALID_REQUEST_BODY == error_identifier )
		{
			if ( error.has(_INVALID_PARAMETER) )
			{
				std::string message = "Upload parameter '[P]' is invalid.";
				LLStringUtil::replaceString(
					message,
					"[P]",
					error[_INVALID_PARAMETER].asString());
				if ( error.has(_MISSING_RESOURCE) )
				{
					message += "\nMissing resource '[R]'.";
					LLStringUtil::replaceString(
						message,
						"[R]",
						error[_MISSING_RESOURCE].asString());
				}
				else if ( error.has(_INVALID_RESOURCE) )
				{
					message += "\nInvalid resource '[R]'.";
					LLStringUtil::replaceString(
						message,
						"[R]",
						error[_INVALID_RESOURCE].asString());
				}
				displayCannotUploadReason(message);
			}
			else
			{
				std::string message = "Upload request was malformed";
				displayCannotUploadReason(message);
			}
		}
		else if (_RESOURCE_COST_DIFFERS == error_identifier)
		{
			displayCannotUploadReason("The resource cost associated with this upload is not consistent with the server.");
		}
		else
		{
			displayCannotUploadReason("Unknown Error");
		}
	}
	void onTransportError()
	{
		displayCannotUploadReason(
				"The server is experiencing unexpected difficulties.");
	}
	void onTransportError(const LLSD& error)
	{
		static const std::string _IDENTIFIER = "identifier";
		static const std::string _SERVER_ERROR_AFTER_CHARGE =
			"NewAgentInventory_ServerErrorAfterCharge";
		std::string error_identifier = error[_IDENTIFIER].asString();
		if ( _SERVER_ERROR_AFTER_CHARGE == error_identifier )
		{
			displayCannotUploadReason(
				"The server is experiencing unexpected difficulties.  You may have been charged for the upload.");
		}
		else
		{
			displayCannotUploadReason(
				"The server is experiencing unexpected difficulties.");
		}
	}
	bool uploadConfirmationCallback(
		const LLSD& notification,
		const LLSD& response,
		boost::intrusive_ptr<LLNewAgentInventoryVariablePriceResponder> responder)
	{
		S32 option;
		std::string confirmation_url;
		option = LLNotificationsUtil::getSelectedOption(
			notification,
			response);
		confirmation_url =
			notification["payload"]["confirmation_url"].asString();
		switch(option)
		{
		case 0:
		    {
				confirmUpload(confirmation_url, responder);
			}
			break;
		case 1:
		default:
			break;
		}
		return false;
	}
	void confirmUpload(
		const std::string& confirmation_url,
		boost::intrusive_ptr<LLNewAgentInventoryVariablePriceResponder> responder)
	{
		if ( getFilename().empty() )
		{
			LLHTTPClient::postFile(
				confirmation_url,
				getVFileID(),
				getAssetType(),
				responder);
		}
		else
		{
			LLHTTPClient::postFile(
				confirmation_url,
				getFilename(),
				responder);
		}
	}
private:
	std::string mFileName;
	LLSD mInventoryData;
	LLAssetType::EType mAssetType;
	LLUUID mVFileID;
};
LLNewAgentInventoryVariablePriceResponder::LLNewAgentInventoryVariablePriceResponder(
	const LLUUID& vfile_id,
	LLAssetType::EType asset_type,
	const LLSD& inventory_info)
{
	mImpl = new Impl(
		vfile_id,
		asset_type,
		inventory_info);
}
LLNewAgentInventoryVariablePriceResponder::LLNewAgentInventoryVariablePriceResponder(
	const std::string& file_name,
	LLAssetType::EType asset_type,
	const LLSD& inventory_info)
{
	mImpl = new Impl(
		file_name,
		asset_type,
		inventory_info);
}
LLNewAgentInventoryVariablePriceResponder::~LLNewAgentInventoryVariablePriceResponder()
{
	delete mImpl;
}
void LLNewAgentInventoryVariablePriceResponder::httpFailure()
{
	const LLSD& content = getContent();
	LL_WARNS("Upload") << dumpResponse() << LL_ENDL;
		static const std::string _ERROR = "error";
	if ( content.has(_ERROR) )
	{
		mImpl->onTransportError(content[_ERROR]);
	}
	else
	{
		mImpl->onTransportError();
	}
}
void LLNewAgentInventoryVariablePriceResponder::httpSuccess()
{
	const LLSD& content = getContent();
	if (!content.isMap())
	{
		failureResult(HTTP_INTERNAL_ERROR_OTHER, "Malformed response contents", content);
		return;
	}
	static const std::string _ERROR = "error";
	static const std::string _STATE = "state";
	static const std::string _COMPLETE = "complete";
	static const std::string _CONFIRM_UPLOAD = "confirm_upload";
	static const std::string _UPLOAD_PRICE = "upload_price";
	static const std::string _RESOURCE_COST = "resource_cost";
	static const std::string _RSVP = "rsvp";
	if ( content.has(_ERROR) )
	{
		LL_WARNS("Upload") << dumpResponse() << LL_ENDL;
		onApplicationLevelError(content[_ERROR]);
		return;
	}
	std::string state = content[_STATE];
	LLAssetType::EType asset_type = mImpl->getAssetType();
	if (_COMPLETE == state)
	{
		if (mImpl->getFilename().empty())
		{
			gVFS->renameFile(
				mImpl->getVFileID(),
				asset_type,
				content["new_asset"].asUUID(),
				asset_type);
		}
 		on_new_single_inventory_upload_complete(
 			asset_type,
 			mImpl->getInventoryType(),
			mImpl->getInventoryTypeString(),
			mImpl->getFolderID(),
			mImpl->getItemName(),
			mImpl->getItemDescription(),
			content,
			content[_UPLOAD_PRICE].asInteger());
	}
	else if ( _CONFIRM_UPLOAD == state )
	{
		showConfirmationDialog(
			content[_UPLOAD_PRICE].asInteger(),
			content[_RESOURCE_COST].asInteger(),
			content[_RSVP].asString());
	}
	else
	{
		LL_WARNS("Upload") << dumpResponse() << LL_ENDL;
		onApplicationLevelError("");
	}
}
void LLNewAgentInventoryVariablePriceResponder::onApplicationLevelError(
	const LLSD& error)
{
	mImpl->onApplicationLevelError(error);
}
void LLNewAgentInventoryVariablePriceResponder::showConfirmationDialog(
	S32 upload_price,
	S32 resource_cost,
	const std::string& confirmation_url)
{
	if (0 == upload_price)
	{
		mImpl->confirmUpload(
			confirmation_url,
			boost::intrusive_ptr<LLNewAgentInventoryVariablePriceResponder>(this));
	}
	else
	{
		LLSD substitutions;
		LLSD payload;
		substitutions["PRICE"] = upload_price;
		payload["confirmation_url"] = confirmation_url;
		LLNotificationsUtil::add(
			"UploadCostConfirmation",
			substitutions,
			payload,
			boost::bind(
				&LLNewAgentInventoryVariablePriceResponder::Impl::uploadConfirmationCallback,
				mImpl,
				_1,
				_2,
				boost::intrusive_ptr<LLNewAgentInventoryVariablePriceResponder>(this)));
	}
}
