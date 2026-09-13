/** 
 * @file llfloaterauction.cpp
 * @author James Cook, Ian Wilkes
 * @brief Implementation of the auction floater.
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
#include "llviewerprecompiledheaders.h"
#include "llfloaterauction.h"
#include "llgl.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llparcel.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llwindow.h"
#include "message.h"
#include "llagent.h"
#include "llcombobox.h"
#include "llmimetypes.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llviewertexturelist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llrender.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "lltrans.h"
void auction_j2c_upload_done(const LLUUID& asset_id,
							   void* user_data, S32 status, LLExtStat ext_status);
void auction_tga_upload_done(const LLUUID& asset_id,
							   void* user_data, S32 status, LLExtStat ext_status);
LLFloaterAuction* LLFloaterAuction::sInstance = NULL;
LLFloaterAuction::LLFloaterAuction()
  : LLFloater(std::string("floater_auction")),
	mParcelID(-1)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_auction.xml");
	childSetValue("fence_check",
		LLSD( gSavedSettings.getBOOL("AuctionShowFence") ) );
	childSetCommitCallback("fence_check",
		onCommitControlSetting(gSavedSettings), (void*)"AuctionShowFence");
	childSetAction("snapshot_btn", onClickSnapshot, this);
	childSetAction("ok_btn", onClickStartAuction, this);
}
LLFloaterAuction::~LLFloaterAuction()
{
	sInstance = nullptr;
}
void LLFloaterAuction::show()
{
	if(!sInstance)
	{
		sInstance = new LLFloaterAuction();
		sInstance->center();
		sInstance->setFocus(TRUE);
	}
	sInstance->open();
}
BOOL LLFloaterAuction::postBuild()
{
	return TRUE;
}
void LLFloaterAuction::onOpen()
{
	initialize();
}
void LLFloaterAuction::initialize()
{
	mParcelUpdateCapUrl.clear();
	mParcelp = LLViewerParcelMgr::getInstance()->getParcelSelection();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	LLParcel* parcelp = mParcelp->getParcel();
	if(parcelp && region && !parcelp->getForSale())
	{
		mParcelHost = region->getHost();
		mParcelID = parcelp->getLocalID();
		mParcelUpdateCapUrl = region->getCapability("ParcelPropertiesUpdate");
		getChild<LLUICtrl>("parcel_text")->setValue(parcelp->getName());
		getChildView("snapshot_btn")->setEnabled(TRUE);
		getChildView("ok_btn")->setEnabled(true);
	}
	else
	{
		mParcelHost.invalidate();
		if(parcelp && parcelp->getForSale())
		{
			getChild<LLUICtrl>("parcel_text")->setValue(getString("already for sale"));
		}
		else
		{
			getChild<LLUICtrl>("parcel_text")->setValue(LLStringUtil::null);
		}
		mParcelID = -1;
		getChildView("snapshot_btn")->setEnabled(false);
		getChildView("ok_btn")->setEnabled(false);
	}
	mImageID.setNull();
	mImage = nullptr;
}
void LLFloaterAuction::draw()
{
	LLFloater::draw();
	if(!isMinimized() && mImage.notNull())
	{
		LLView* snapshot_icon = findChild<LLView>("snapshot_icon");
		if (snapshot_icon)
		{
			LLRect rect = snapshot_icon->getRect();
			{
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gl_rect_2d(rect, LLColor4(0.f, 0.f, 0.f, 1.f));
				rect.stretch(-1);
			}
			{
				LLGLSUIDefault gls_ui;
				gGL.color3f(1.f, 1.f, 1.f);
				gl_draw_scaled_image(rect.mLeft,
									 rect.mBottom,
									 rect.getWidth(),
									 rect.getHeight(),
									 mImage);
			}
		}
	}
}
void LLFloaterAuction::onClickSnapshot(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);
	LLPointer<LLImageRaw> raw = new LLImageRaw;
	gForceRenderLandFence = self->getChild<LLUICtrl>("fence_check")->getValue().asBoolean();
	BOOL success = gViewerWindow->rawSnapshot(raw,
											  gViewerWindow->getWindowWidth(),
											  gViewerWindow->getWindowHeight(),
											  (F32)gViewerWindow->getWindowWidth() / gViewerWindow->getWindowHeight(),
											  FALSE, FALSE);
	gForceRenderLandFence = FALSE;
	if (success)
	{
		self->mTransactionID.generate();
		self->mImageID = self->mTransactionID.makeAssetID(gAgent.getSecureSessionID());
		if(!gSavedSettings.getBOOL("QuietSnapshotsToDisk"))
		{
			gViewerWindow->playSnapshotAnimAndSound();
		}
		LL_INFOS() << "Writing TGA..." << LL_ENDL;
		LLPointer<LLImageTGA> tga = new LLImageTGA;
		tga->encode(raw);
		LLVFile::writeFile(tga->getData(), tga->getDataSize(), gVFS, self->mImageID, LLAssetType::AT_IMAGE_TGA);
		raw->biasedScaleToPowerOfTwo(LLViewerTexture::MAX_IMAGE_SIZE_DEFAULT);
		LL_INFOS() << "Writing J2C..." << LL_ENDL;
		LLPointer<LLImageJ2C> j2c = new LLImageJ2C;
		j2c->encode(raw, 0.0f);
		LLVFile::writeFile(j2c->getData(), j2c->getDataSize(), gVFS, self->mImageID, LLAssetType::AT_TEXTURE);
		self->mImage = LLViewerTextureManager::getLocalTexture((LLImageRaw*)raw, FALSE);
		gGL.getTexUnit(0)->bind(self->mImage);
		self->mImage->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
	else
	{
		LL_WARNS() << "Unable to take snapshot" << LL_ENDL;
	}
}
void LLFloaterAuction::onClickStartAuction(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);
	if(self->mImageID.notNull())
	{
		LLSD parcel_name = self->getChild<LLUICtrl>("parcel_text")->getValue();
		std::string* name = new std::string(parcel_name.asString());
		gAssetStorage->storeAssetData(self->mTransactionID, LLAssetType::AT_IMAGE_TGA,
									&auction_tga_upload_done,
									(void*)name,
									FALSE);
		self->getWindow()->incBusyCount();
		std::string* j2c_name = new std::string(parcel_name.asString());
		gAssetStorage->storeAssetData(self->mTransactionID, LLAssetType::AT_TEXTURE,
								   &auction_j2c_upload_done,
								   (void*)j2c_name,
								   FALSE);
		self->getWindow()->incBusyCount();
		LLNotificationsUtil::add("UploadingAuctionSnapshot");
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("ViewerStartAuction");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("ParcelData");
	msg->addS32("LocalID", self->mParcelID);
	msg->addUUID("SnapshotID", self->mImageID);
	msg->sendReliable(self->mParcelHost);
	self->cleanupAndClose();
}
void LLFloaterAuction::cleanupAndClose()
{
	mImageID.setNull();
	mImage = nullptr;
	mParcelID = -1;
	mParcelHost.invalidate();
	close();
}
void LLFloaterAuction::onClickResetParcel(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);
	if (self)
	{
		self->doResetParcel();
	}
}
void LLFloaterAuction::doResetParcel()
{
	LLParcel* parcelp = mParcelp->getParcel();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if (parcelp
		&& region
		&& !mParcelUpdateCapUrl.empty())
	{
		LLSD body;
		std::string empty;
		U32 message_flags = 0x01;
		body["flags"] = ll_sd_from_U32(message_flags);
		body["local_id"] = parcelp->getLocalID();
		U32 parcel_flags = PF_ALLOW_LANDMARK |
						   PF_ALLOW_FLY	|
						   PF_CREATE_GROUP_OBJECTS |
						   PF_ALLOW_ALL_OBJECT_ENTRY |
						   PF_ALLOW_GROUP_OBJECT_ENTRY |
						   PF_ALLOW_GROUP_SCRIPTS |
						   PF_RESTRICT_PUSHOBJECT |
						   PF_SOUND_LOCAL |
						   PF_ALLOW_VOICE_CHAT |
						   PF_USE_ESTATE_VOICE_CHAN;
		body["parcel_flags"] = ll_sd_from_U32(parcel_flags);
		std::ostringstream parcel_name;
		LLVector3 center_point( parcelp->getCenterpoint() );
		center_point.snap(0);
		parcel_name << region->getName()
					<< " ("
					<< (S32) center_point.mV[VX]
					<< ","
					<< (S32) center_point.mV[VY]
					<< ") "
					<< region->getSimAccessString()
					<< " "
					<< parcelp->getArea()
					<< "m";
		std::string new_name(parcel_name.str());
		body["name"] = new_name;
		getChild<LLUICtrl>("parcel_text")->setValue(new_name);
		body["sale_price"] = (S32) 0;
		body["description"] = empty;
		body["music_url"] = empty;
		body["media_url"] = empty;
		body["media_desc"] = empty;
		body["media_type"] = LLMIMETypes::getDefaultMimeType();
		body["media_width"] = (S32) 0;
		body["media_height"] = (S32) 0;
		body["auto_scale"] = (S32) 0;
		body["media_loop"] = (S32) 0;
		body["obscure_media"] = (S32) 0;
		body["obscure_music"] = (S32) 0;
		body["media_id"] = LLUUID::null;
		body["group_id"] = MAINTENANCE_GROUP_ID;
		body["pass_price"] = (S32) 10;
		body["pass_hours"] = 0.0f;
		body["category"] = (U8) LLParcel::C_NONE;
		body["auth_buyer_id"] = LLUUID::null;
		body["snapshot_id"] = LLUUID::null;
		body["user_location"] = ll_sd_from_vector3( LLVector3::zero );
		body["user_look_at"] = ll_sd_from_vector3( LLVector3::zero );
		body["landing_type"] = (U8) LLParcel::L_DIRECT;
		LL_INFOS() << "Sending parcel update to reset for auction via capability to: "
			<< mParcelUpdateCapUrl << LL_ENDL;
		LLHTTPClient::post(mParcelUpdateCapUrl, body, new LLHTTPClient::ResponderIgnore());
		LLMessageSystem *msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ParcelSetOtherCleanTime);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ParcelData);
		msg->addS32Fast(_PREHASH_LocalID, parcelp->getLocalID());
		msg->addS32Fast(_PREHASH_OtherCleanTime, 5);
		msg->sendReliable(region->getHost());
		clearParcelAccessList(parcelp, region, AL_ACCESS);
		clearParcelAccessList(parcelp, region, AL_BAN);
		clearParcelAccessList(parcelp, region, AL_ALLOW_EXPERIENCE);
		clearParcelAccessList(parcelp, region, AL_BLOCK_EXPERIENCE);
	}
}
void LLFloaterAuction::clearParcelAccessList(LLParcel* parcel, LLViewerRegion* region, U32 list)
{
	if (!region || !parcel) return;
	LLUUID transactionUUID;
	transactionUUID.generate();
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelAccessListUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_Data);
	msg->addU32Fast(_PREHASH_Flags, list);
	msg->addS32(_PREHASH_LocalID, parcel->getLocalID() );
	msg->addUUIDFast(_PREHASH_TransactionID, transactionUUID);
	msg->addS32Fast(_PREHASH_SequenceID, 1);
	msg->addS32Fast(_PREHASH_Sections, 0);
	msg->nextBlockFast(_PREHASH_List);
	msg->addUUIDFast(_PREHASH_ID,  LLUUID::null );
	msg->addS32Fast(_PREHASH_Time, 0 );
	msg->addU32Fast(_PREHASH_Flags,	0 );
	msg->sendReliable( region->getHost() );
}
void LLFloaterAuction::onClickSellToAnyone(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);
	if (self)
	{
		LLParcel* parcelp = self->mParcelp->getParcel();
		S32 sale_price = parcelp->getArea();
		S32 area = parcelp->getArea();
		LLSD args;
		args["LAND_SIZE"] = llformat("%d", area);
		args["SALE_PRICE"] = llformat("%d", sale_price);
		args["NAME"] = LLTrans::getString("Anyone");
		LLNotification::Params params("ConfirmLandSaleChange");
		params.substitutions(args)
			.functor(boost::bind(&LLFloaterAuction::onSellToAnyoneConfirmed, self, _1, _2));
		params.name("ConfirmLandSaleToAnyoneChange");
		LLNotifications::instance().add(params);
	}
}
bool LLFloaterAuction::onSellToAnyoneConfirmed(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		doSellToAnyone();
	}
	return false;
}
void LLFloaterAuction::doSellToAnyone()
{
	LLParcel* parcelp = mParcelp->getParcel();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if (parcelp
		&& region
		&& !mParcelUpdateCapUrl.empty())
	{
		LLSD body;
		U32 message_flags = 0x01;
		body["flags"] = ll_sd_from_U32(message_flags);
		body["local_id"] = parcelp->getLocalID();
		U32 parcel_flags = parcelp->getParcelFlags() | PF_FOR_SALE;
		parcel_flags &= ~PF_FOR_SALE_OBJECTS;
		body["parcel_flags"] = ll_sd_from_U32(parcel_flags);
		body["sale_price"] = parcelp->getArea();
		body["auth_buyer_id"] = LLUUID::null;
		LL_INFOS() << "Sending parcel update to sell to anyone for L$1 via capability to: "
			<< mParcelUpdateCapUrl << LL_ENDL;
		LLHTTPClient::post(mParcelUpdateCapUrl, body, new LLHTTPClient::ResponderIgnore());
		cleanupAndClose();
	}
}
void auction_tga_upload_done(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status)
{
	std::string* name = (std::string*)(user_data);
	LL_INFOS() << "Upload of asset '" << *name << "' " << asset_id
			<< " returned " << status << LL_ENDL;
	delete name;
	gViewerWindow->getWindow()->decBusyCount();
	if (0 == status)
	{
		LLNotificationsUtil::add("UploadWebSnapshotDone");
	}
	else
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("UploadAuctionSnapshotFail", args);
	}
}
void auction_j2c_upload_done(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status)
{
	std::string* name = (std::string*)(user_data);
	LL_INFOS() << "Upload of asset '" << *name << "' " << asset_id
			<< " returned " << status << LL_ENDL;
	delete name;
	gViewerWindow->getWindow()->decBusyCount();
	if (0 == status)
	{
		LLNotificationsUtil::add("UploadSnapshotDone");
	}
	else
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("UploadAuctionSnapshotFail", args);
	}
}
