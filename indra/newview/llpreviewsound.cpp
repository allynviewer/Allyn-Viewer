/** 
 * @file llpreviewsound.cpp
 * @brief LLPreviewSound class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llaudioengine.h"
#include "llagent.h"
#include "llbutton.h"
#include "llfloaterinventory.h"
#include "llinventory.h"
#include "lllineeditor.h"
#include "llpreviewsound.h"
#include "llresmgr.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"
#include "lluictrlfactory.h"
#include "llvoavatarself.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llviewerwindow.h"
#include "statemachine/aifilepicker.h"
#include "llviewerregion.h"
extern LLAudioEngine* gAudiop;
extern LLAgent gAgent;
const F32 SOUND_GAIN = 1.0f;
LLPreviewSound::LLPreviewSound(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const LLUUID& object_uuid)	:
	LLPreview( name, rect, title, item_uuid, object_uuid)
,	mIsCopyable(false)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_preview_sound.xml");
	setTitle(title);
	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}
}
BOOL LLPreviewSound::postBuild()
{
	const LLInventoryItem* item = getItem();
	if (item)
	{
		getChild<LLUICtrl>("desc")->setValue(item->getDescription());
		mIsCopyable = (item->getPermissions().getCreator() == gAgentID);
		if (gAudiop)
			if (LLAudioSource* asp = gAgentAvatarp->getAudioSource(gAgentID))
				asp->preload(item->getAssetUUID());
	}
	childSetAction("Sound play btn",&LLPreviewSound::playSound,this);
	childSetAction("Sound audition btn",&LLPreviewSound::auditionSound,this);
	childSetAction("Sound copy uuid btn", &LLPreviewSound::copyUUID, this);
	childSetAction("Play ambient btn", &LLPreviewSound::playAmbient, this);
	LLButton* button = getChild<LLButton>("Sound play btn");
	button->setSoundFlags(LLView::SILENT);
	button = getChild<LLButton>("Sound audition btn");
	button->setSoundFlags(LLView::SILENT);
	childSetCommitCallback("desc", LLPreview::onText, this);
	getChild<LLLineEditor>("desc")->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	return LLPreview::postBuild();
}
void LLPreviewSound::playSound( void *userdata )
{
	LLPreviewSound* self = (LLPreviewSound*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item && gAudiop)
	{
		send_sound_trigger(item->getAssetUUID(), SOUND_GAIN);
	}
}
void LLPreviewSound::auditionSound( void *userdata )
{
	LLPreviewSound* self = (LLPreviewSound*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item && gAudiop)
	{
		LLVector3d lpos_global = gAgent.getPositionGlobal();
		gAudiop->triggerSound(item->getAssetUUID(), gAgent.getID(), SOUND_GAIN, LLAudioEngine::AUDIO_TYPE_UI, lpos_global);
	}
}
void LLPreviewSound::playAmbient( void* userdata )
{
	LLPreviewSound* self = (LLPreviewSound*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item && gAudiop)
	{
		F32 gain = 0.01f;
		for(int i = 0; i < 2; i++)
		{
			gMessageSystem->newMessageFast(_PREHASH_SoundTrigger);
			gMessageSystem->nextBlockFast(_PREHASH_SoundData);
			gMessageSystem->addUUIDFast(_PREHASH_SoundID, LLUUID(item->getAssetUUID()));
			gMessageSystem->addUUIDFast(_PREHASH_OwnerID, LLUUID::null);
			gMessageSystem->addUUIDFast(_PREHASH_ObjectID, LLUUID::null);
			gMessageSystem->addUUIDFast(_PREHASH_ParentID, LLUUID::null);
			gMessageSystem->addU64Fast(_PREHASH_Handle, gAgent.getRegion()->getHandle());
			LLVector3d	pos = -from_region_handle(gAgent.getRegion()->getHandle());
			gMessageSystem->addVector3Fast(_PREHASH_Position, (LLVector3)pos);
			gMessageSystem->addF32Fast(_PREHASH_Gain, gain);
			gMessageSystem->sendReliable(gAgent.getRegionHost());
			gain = 1.0f;
		}
	}
}
void LLPreviewSound::copyUUID( void *userdata )
{
	LLPreviewSound* self = (LLPreviewSound*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item )
	{
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(item->getAssetUUID().asString()));
	}
}
BOOL LLPreviewSound::canSaveAs() const
{
	return mIsCopyable;
}
void LLPreviewSound::saveAs()
{
	const LLInventoryItem *item = getItem();
	if(item)
	{
		gAssetStorage->getAssetData(item->getAssetUUID(), LLAssetType::AT_SOUND, LLPreviewSound::gotAssetForSave, this, TRUE);
	}
}
void LLPreviewSound::gotAssetForSave(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLPreviewSound* self = (LLPreviewSound*) user_data;
	LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
	S32 size = file.getSize();
	char* buffer = new char[size];
	if (buffer == NULL)
	{
		LL_ERRS() << "Memory Allocation Failed" << LL_ENDL;
		return;
	}
	file.read((U8*)buffer, size);
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open(LLDir::getScrubbedFileName(self->getItem()->getName()) + ".ogg", FFSAVE_OGG);
	filepicker->run(boost::bind(&LLPreviewSound::gotAssetForSave_continued, buffer, size, filepicker));
}
void LLPreviewSound::gotAssetForSave_continued(char* buffer, S32 size, AIFilePicker* filepicker)
{
	if (filepicker->hasFilename())
	{
		std::string filename = filepicker->getFilename();
		std::ofstream export_file(filename.c_str(), std::ofstream::binary);
		export_file.write(buffer, size);
		export_file.close();
	}
	delete [] buffer;
}
LLUUID LLPreviewSound::getItemID()
{
	const LLViewerInventoryItem* item = getItem();
	if(item)
	{
		return item->getUUID();
	}
	return LLUUID::null;
}
