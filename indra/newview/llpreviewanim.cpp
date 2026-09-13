/** 
 * @file llpreviewanim.cpp
 * @brief LLPreviewAnim class implementation
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
#include "llpreviewanim.h"
#include "llbutton.h"
#include "llfloaterinventory.h"
#include "llresmgr.h"
#include "llinventory.h"
#include "llvoavatarself.h"
#include "llagent.h"
#include "llkeyframemotion.h"
#include "statemachine/aifilepicker.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
extern LLAgent gAgent;
LLPreviewAnim::LLPreviewAnim(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const e_activation_type& _activate, const LLUUID& object_uuid )	:
	LLPreview( name, rect, title, item_uuid, object_uuid)
{
	mIsCopyable = false;
	setTitle(title);
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_preview_animation.xml");
	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}
	activate(_activate);
}
void LLPreviewAnim::endAnimCallback( void *userdata )
{
	LLHandle<LLFloater>* handlep = ((LLHandle<LLFloater>*)userdata);
	LLFloater* self = handlep->get();
	delete handlep;
	if (self)
	{
		self->childSetValue("Anim play btn", FALSE);
		self->childSetValue("Anim audition btn", FALSE);
	}
}
BOOL LLPreviewAnim::postBuild()
{
	const LLInventoryItem* item = getItem();
	if(item)
	{
		gAgentAvatarp->createMotion(item->getAssetUUID());
		childSetText("desc", item->getDescription());
		const LLPermissions& perm = item->getPermissions();
		mIsCopyable = (perm.getCreator() == gAgent.getID());
	}
	childSetAction("Anim play btn",playAnim,this);
	childSetAction("Anim audition btn",auditionAnim,this);
	childSetCommitCallback("desc", LLPreview::onText, this);
	getChild<LLLineEditor>("desc")->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	return LLPreview::postBuild();
}
void LLPreviewAnim::activate(e_activation_type type)
{
	switch ( type )
	{
		case PLAY:
		{
			playAnim( (void *) this );
			break;
		}
		case AUDITION:
		{
			auditionAnim( (void *) this );
			break;
		}
		default:
		{
		}
	}
}
void LLPreviewAnim::playAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item)
	{
		LLUUID itemID=item->getAssetUUID();
		LLButton* btn = self->getChild<LLButton>("Anim play btn");
		if (btn)
		{
			btn->toggleState();
		}
		if (self->childGetValue("Anim play btn").asBoolean() )
		{
			self->mPauseRequest = NULL;
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_START);
			LLMotion* motion = gAgentAvatarp->findMotion(itemID);
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback, (void *)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgentAvatarp->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}
void LLPreviewAnim::auditionAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item)
	{
		LLUUID itemID=item->getAssetUUID();
		LLButton* btn = self->getChild<LLButton>("Anim audition btn");
		if (btn)
		{
			btn->toggleState();
		}
		if (self->childGetValue("Anim audition btn").asBoolean() )
		{
			self->mPauseRequest = NULL;
			gAgentAvatarp->startMotion(item->getAssetUUID());
			LLMotion* motion = gAgentAvatarp->findMotion(itemID);
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback, (void *)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgentAvatarp->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}
void LLPreviewAnim::copyAnimID(void *userdata)
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item)
	{
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(item->getAssetUUID().asString()));
	}
}
BOOL LLPreviewAnim::canSaveAs() const
{
	return mIsCopyable;
}
void LLPreviewAnim::saveAs()
{
	const LLInventoryItem *item = getItem();
	if(item)
	{
		bool static_vfile = false;
		LLVFile* anim_file = new LLVFile(gStaticVFS, item->getAssetUUID(), LLAssetType::AT_ANIMATION);
		if (anim_file && anim_file->getSize())
		{
			static_vfile = true;
			LLPreviewAnim::gotAssetForSave(gStaticVFS, item->getAssetUUID(), LLAssetType::AT_ANIMATION, this, 0, 0);
		}
		delete anim_file;
		anim_file = NULL;
		if(!static_vfile)
		{
			gAssetStorage->getAssetData(item->getAssetUUID(), LLAssetType::AT_ANIMATION, LLPreviewAnim::gotAssetForSave, this, TRUE);
		}
	}
}
void LLPreviewAnim::gotAssetForSave(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLPreviewAnim* self = (LLPreviewAnim*) user_data;
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
	filepicker->open(LLDir::getScrubbedFileName(self->getItem()->getName()) + ".animatn", FFSAVE_ANIMATN);
	filepicker->run(boost::bind(&LLPreviewAnim::gotAssetForSave_continued, buffer, size, filepicker));
}
void LLPreviewAnim::gotAssetForSave_continued(char* buffer, S32 size, AIFilePicker* filepicker)
{
	if (filepicker->hasFilename())
	{
		std::string filename = filepicker->getFilename();
		std::ofstream export_file(filename.c_str(), std::ofstream::binary);
		export_file.write(buffer, size);
		export_file.close();
	}
	delete[] buffer;
}
LLUUID LLPreviewAnim::getItemID()
{
	const LLViewerInventoryItem* item = getItem();
	if(item)
	{
		return item->getUUID();
	}
	return LLUUID::null;
}
void LLPreviewAnim::onClose(bool app_quitting)
{
	const LLInventoryItem *item = getItem();
	if(item)
	{
		gAgentAvatarp->stopMotion(item->getAssetUUID());
		gAgent.sendAnimationRequest(item->getAssetUUID(), ANIM_REQUEST_STOP);
		LLMotion* motion = gAgentAvatarp->findMotion(item->getAssetUUID());
		if (motion)
		{
			motion->setDeactivateCallback(NULL, (void *)NULL);
		}
	}
	destroy();
}
