/** 
 * @file lltoolface.cpp
 * @brief A tool to manipulate faces
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "lltoolface.h"
#include "v3math.h"
#include "llagent.h"
#include "llviewercontrol.h"
#include "llselectmgr.h"
#include "lltoolview.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llfloatertools.h"
#include "rlvhandler.h"
LLToolFace::LLToolFace()
:	LLTool(std::string("Texture"))
{ }
LLToolFace::~LLToolFace()
{ }
BOOL LLToolFace::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		gFloaterTools->showPanel( LLFloaterTools::PANEL_FACE );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
BOOL LLToolFace::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gViewerWindow->pickAsync(x, y, mask, pickCallback, TRUE, TRUE);
	return TRUE;
}
void LLToolFace::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj	= pick_info.getObject();
	if (hit_obj)
	{
		S32 hit_face = pick_info.mObjectFace;
		if (hit_obj->isAvatar())
		{
			return;
		}
		if ( (rlv_handler_t::isEnabled()) &&
			 ( (!gRlvHandler.canEdit(hit_obj)) ||
 			   ((gRlvHandler.hasBehaviour(RLV_BHVR_FARTOUCH)) && (!gRlvHandler.canTouch(hit_obj, pick_info.mObjectOffset))) ) )
		{
			return;
		}
		if (pick_info.mKeyMask & MASK_SHIFT)
		{
			if ( !hit_obj->isSelected() )
			{
				LLSelectMgr::getInstance()->selectObjectOnly(hit_obj, hit_face);
			}
			else if (!LLSelectMgr::getInstance()->getSelection()->contains(hit_obj, hit_face) )
			{
				LLSelectMgr::getInstance()->addAsIndividual(hit_obj, hit_face);
			}
			else
			{
				LLSelectMgr::getInstance()->remove(hit_obj, hit_face);
			}
		}
		else
		{
			LLSelectMgr::getInstance()->deselectAll();
			LLSelectMgr::getInstance()->selectObjectOnly(hit_obj, hit_face);
		}
	}
	else
	{
		if (!(pick_info.mKeyMask == MASK_SHIFT))
		{
			LLSelectMgr::getInstance()->deselectAll();
		}
	}
}
void LLToolFace::handleSelect()
{
	LLSelectMgr::getInstance()->setTEMode(TRUE);
}
void LLToolFace::handleDeselect()
{
	LLSelectMgr::getInstance()->setTEMode(FALSE);
}
void LLToolFace::render()
{
}
