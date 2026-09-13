/** 
 * @file llpanelpick.h
 * @brief LLPanelPick class definition
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
#ifndef LL_LLPANELPICK_H
#define LL_LLPANELPICK_H
#include "llpanel.h"
#include "llavatarpropertiesprocessor.h"
class LLLineEditor;
class LLTextEditor;
class LLTextureCtrl;
class AIFilePicker;
class LLPanelPick : public LLPanel, public LLAvatarPropertiesObserver
{
public:
    LLPanelPick();
    ~LLPanelPick();
	void reset();
    BOOL postBuild();
    void draw();
	void processProperties(void* data, EAvatarProcessorType type);
	void initNewPick();
	void importNewPick(void (*callback)(void*, bool), void* data);
	void importNewPick_continued(void (*callback)(void*, bool), void* data, AIFilePicker* filepicker);
	void exportPick();
	void exportPick_continued(AIFilePicker* filepicker);
	void setPickID(const LLUUID& pick_id, const LLUUID& creator_id);
	void markForServerRequest();
	const std::string& getPickName() const { return mNameEditor->getText(); }
	const LLUUID& getPickID() const { return mPickID; }
	const LLUUID& getPickCreatorID() const { return mCreatorID; }
    void sendPickInfoRequest();
	void sendPickInfoUpdate();
protected:
	void onClickTeleport();
	void onClickMap();
	void onClickSet();
	void onCommitAny();
	bool mImporting;
	std::string mLocationText;
    LLUUID mPickID;
	LLUUID mCreatorID;
	LLUUID mParcelID;
	bool mDataRequested;
	bool mDataReceived;
	std::string mSimName;
    LLVector3d mPosGlobal;
    LLTextureCtrl*	mSnapshotCtrl;
    LLLineEditor*	mNameEditor;
    LLTextEditor*	mDescEditor;
    LLLineEditor*	mLocationEditor;
	LLUICtrl* mTeleportBtn;
	LLUICtrl* mSetBtn;
	LLUICtrl* mOpenBtn;
    typedef std::list<LLPanelPick*> panel_list_t;
	static panel_list_t sAllPanels;
};
#endif
