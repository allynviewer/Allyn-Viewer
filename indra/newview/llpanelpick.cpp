/** 
 * @file llpanelpick.cpp
 * @brief LLPanelPick class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llpanelpick.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llfloaterworldmap.h"
#include "llpreviewtexture.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "rlvhandler.h"
#include "statemachine/aifilepicker.h"
#include "hippogridmanager.h"
#include "llsdserialize.h"
void show_picture(const LLUUID& id, const std::string& name, S32 max_auto_size)
{
	if (id.isNull() || LLPreview::show(id)) return;
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("PreviewTextureRect");
	auto preview = new LLPreviewTexture("preview texture", rect.translate(left - rect.mLeft, rect.mTop - top), name, id, FALSE, max_auto_size);
	preview->setFocus(true);
	gFloaterView->adjustToFitScreen(preview, false);
}
void show_picture(const LLUUID& id, const std::string& name)
{
	show_picture(id, name, 0);
}
std::list<LLPanelPick*> LLPanelPick::sAllPanels;
LLPanelPick::LLPanelPick()
:	LLPanel(std::string("Top Picks Panel")),
	mPickID(),
	mCreatorID(),
	mParcelID(),
	mDataRequested(false),
	mDataReceived(false),
    mPosGlobal(),
    mSnapshotCtrl(NULL),
    mNameEditor(NULL),
    mDescEditor(NULL),
    mLocationEditor(NULL),
    mSetBtn(NULL),
	mOpenBtn(NULL),
	mImporting(0)
{
    sAllPanels.push_back(this);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_pick.xml");
}
LLPanelPick::~LLPanelPick()
{
	if(mDataRequested && !mDataReceived)
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(mCreatorID, this);
	}
    sAllPanels.remove(this);
}
void LLPanelPick::reset()
{
	if(mDataRequested && !mDataReceived)
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(mCreatorID, this);
	}
	mPickID.setNull();
	mCreatorID.setNull();
	mParcelID.setNull();
	mDataRequested = true;
	mDataReceived = false;
	mPosGlobal.clearVec();
	clearCtrls();
}
BOOL LLPanelPick::postBuild()
{
	auto show_pic = [this] { show_picture(mSnapshotCtrl->getImageAssetID(), mNameEditor->getText()); };
	auto show_pic_if_not_self = [=] { if (!mSnapshotCtrl->canChange()) show_pic(); };
    mSnapshotCtrl = getChild<LLTextureCtrl>("snapshot_ctrl");
	mSnapshotCtrl->setCommitCallback(boost::bind(&LLPanelPick::onCommitAny, this));
	mSnapshotCtrl->setMouseUpCallback(std::bind(show_pic_if_not_self));
    mNameEditor = getChild<LLLineEditor>("given_name_editor");
	mNameEditor->setCommitOnFocusLost(true);
	mNameEditor->setCommitCallback(boost::bind(&LLPanelPick::onCommitAny, this));
    mDescEditor = getChild<LLTextEditor>("desc_editor");
	mDescEditor->setCommitOnFocusLost(true);
	mDescEditor->setCommitCallback(boost::bind(&LLPanelPick::onCommitAny, this));
	mDescEditor->setTabsToNextField(true);
    mLocationEditor = getChild<LLLineEditor>("location_editor");
	mSetBtn = getChild<LLUICtrl>( "set_location_btn");
	mSetBtn->setCommitCallback(boost::bind(&LLPanelPick::onClickSet,this));
	mOpenBtn = getChild<LLUICtrl>("open_picture_btn");
	mOpenBtn->setCommitCallback(std::bind(show_pic));
	getChild<LLUICtrl>("pick_teleport_btn")->setCommitCallback(boost::bind(&LLPanelPick::onClickTeleport,this));
	getChild<LLUICtrl>("pick_map_btn")->setCommitCallback(boost::bind(&LLPanelPick::onClickMap,this));
    return TRUE;
}
void LLPanelPick::processProperties(void* data, EAvatarProcessorType type)
{
	if (!data || APT_PICK_INFO != type) return;
	LLPickData* pick_info = static_cast<LLPickData*>(data);
	if (pick_info->creator_id != mCreatorID || pick_info->pick_id != mPickID)
		return;
	LLAvatarPropertiesProcessor::getInstance()->removeObserver(mCreatorID, this);
	std::string location_text = pick_info->user_name + ", ";
	if (!pick_info->original_name.empty())
	{
		location_text += pick_info->original_name + ", ";
	}
	location_text += pick_info->sim_name + " ";
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelPick* self = *iter;
		if (!self->mImporting) self->mLocationText = location_text;
		else location_text = self->mLocationText;
		self->mImporting = false;
	}
    S32 region_x = ll_round((F32)pick_info->pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = ll_round((F32)pick_info->pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = ll_round((F32)pick_info->pos_global.mdV[VZ]);
    location_text.append(llformat("(%d, %d, %d)", region_x, region_y, region_z));
	mDataReceived = true;
	mPickID = pick_info->pick_id;
	mCreatorID = pick_info->creator_id;
	mParcelID = pick_info->parcel_id;
	mSimName = pick_info->sim_name;
	mPosGlobal = pick_info->pos_global;
    mNameEditor->setText(pick_info->name);
    mDescEditor->setText(pick_info->desc, false);
    mSnapshotCtrl->setImageAssetID(pick_info->snapshot_id);
    mLocationEditor->setText(location_text);
}
void LLPanelPick::initNewPick()
{
	mPickID.generate();
	mCreatorID = gAgentID;
	mPosGlobal = gAgent.getPositionGlobal();
	if (LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel())
	{
		mNameEditor->setText(parcel->getName());
		mDescEditor->setText(parcel->getDesc());
		mSnapshotCtrl->setImageAssetID(parcel->getSnapshotID());
	}
	sendPickInfoUpdate();
}
void LLPanelPick::importNewPick(void (*callback)(void*, bool), void* data)
{
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open(FFLOAD_XML, "", "export");
	filepicker->run(boost::bind(&LLPanelPick::importNewPick_continued, this, callback, data, filepicker));
}
void LLPanelPick::importNewPick_continued(void (*callback)(void*, bool), void* data, AIFilePicker* filepicker)
{
	bool result = false;
	if (filepicker->hasFilename())
	{
		llifstream importer(filepicker->getFilename());
		LLSD data;
		LLSDSerialize::fromXMLDocument(data, importer);
		LLSD file_data = data["Data"];
		mPickID.generate();
		mCreatorID = gAgentID;
		mPosGlobal = LLVector3d(file_data["globalPos"]);
		mNameEditor->setText(file_data["name"].asString());
		mDescEditor->setText(file_data["desc"].asString());
		mSnapshotCtrl->setImageAssetID(file_data["snapshotID"].asUUID());
		mParcelID = file_data["parcelID"].asUUID();
		mLocationText = file_data["locationText"].asString();
		mImporting = true;
		sendPickInfoUpdate();
		result = true;
	}
	(*callback)(data, result);
}
void LLPanelPick::exportPick()
{
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open("", FFSAVE_XML, "", "export");
	filepicker->run(boost::bind(&LLPanelPick::exportPick_continued, this, filepicker));
}
void LLPanelPick::exportPick_continued(AIFilePicker* filepicker)
{
	if (!filepicker->hasFilename())
		return;
	LLSD header;
	header["Version"] = 2;
	LLSD datas;
	datas["name"] = mNameEditor->getText();
	datas["desc"] = mDescEditor->getText();
	datas["parcelID"] = mParcelID;
	datas["snapshotID"] = mSnapshotCtrl->getImageAssetID();
	datas["globalPos"] = mPosGlobal.getValue();
	datas["locationText"] = mLocationText;
	LLSD file;
	file["Header"] = header;
	file["Grid"] = gHippoGridManager->getConnectedGrid()->getLoginUri();
	file["Data"] = datas;
	llofstream export_file(filepicker->getFilename());
	LLSDSerialize::toPrettyXML(file, export_file);
	export_file.close();
}
void LLPanelPick::setPickID(const LLUUID& pick_id, const LLUUID& creator_id)
{
	mPickID = pick_id;
	mCreatorID = creator_id;
}
void LLPanelPick::markForServerRequest()
{
	mDataRequested = false;
	mDataReceived = false;
}
void LLPanelPick::sendPickInfoRequest()
{
	LLAvatarPropertiesProcessor::getInstance()->addObserver(mCreatorID, this);
	LLAvatarPropertiesProcessor::getInstance()->sendPickInfoRequest(mCreatorID, mPickID);
	mDataRequested = true;
}
void LLPanelPick::sendPickInfoUpdate()
{
	LLPickData pick_data;
	if (mPickID.isNull())
	{
		mPickID.generate();
	}
	pick_data.agent_id = gAgentID;
	pick_data.session_id = gAgentSessionID;
	pick_data.pick_id = mPickID;
	pick_data.creator_id = gAgentID;
	pick_data.top_pick = false;
	pick_data.parcel_id = mParcelID;
	pick_data.name = mNameEditor->getText();
	pick_data.desc = mDescEditor->getText();
	pick_data.snapshot_id = mSnapshotCtrl->getImageAssetID();
	pick_data.pos_global = mPosGlobal;
	pick_data.sort_order = 0;
	pick_data.enabled = true;
	LLAvatarPropertiesProcessor::getInstance()->sendPickInfoUpdate(&pick_data);
}
void LLPanelPick::draw()
{
	if (!mDataRequested)
	{
		sendPickInfoRequest();
	}
	bool is_self = gAgentID == mCreatorID;
	mSnapshotCtrl->setEnabled(is_self);
	mNameEditor->setEnabled(is_self);
	mDescEditor->setEnabled(is_self);
	mSetBtn->setVisible(is_self);
	mSetBtn->setEnabled(is_self && !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC));
	mOpenBtn->setVisible(!is_self);
	LLPanel::draw();
}
void LLPanelPick::onClickTeleport()
{
    if (!mPosGlobal.isExactlyZero())
    {
        gAgent.teleportViaLocation(mPosGlobal);
        gFloaterWorldMap->trackLocation(mPosGlobal);
    }
}
void LLPanelPick::onClickMap()
{
	gFloaterWorldMap->trackLocation(mPosGlobal);
	LLFloaterWorldMap::show(true);
}
void LLPanelPick::onClickSet()
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
	{
		return;
	}
	mPosGlobal = gAgent.getPositionGlobal();
	std::string location_text("(will update after save), " + mSimName);
    S32 region_x = ll_round((F32)mPosGlobal.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = ll_round((F32)mPosGlobal.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = ll_round((F32)mPosGlobal.mdV[VZ]);
    location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));
	if (gAgent.getRegion()->getName() != mSimName)
	{
		LLNotificationsUtil::add("SetPickLocation");
	}
	mLocationEditor->setText(location_text);
	onCommitAny();
}
void LLPanelPick::onCommitAny()
{
	if (mCreatorID != gAgentID) return;
	if (mDataReceived)
	{
		sendPickInfoUpdate();
		if (LLTabContainer* tab = dynamic_cast<LLTabContainer*>(getParent()))
		{
			tab->setCurrentTabName(mNameEditor->getText());
		}
	}
}
