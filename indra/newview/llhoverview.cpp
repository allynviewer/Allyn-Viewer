/** 
 * @file llhoverview.cpp
 * @brief LLHoverView class implementation
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
#include "llhoverview.h"
#include "llfontgl.h"
#include "message.h"
#include "llgl.h"
#include "llrender.h"
#include "llfontgl.h"
#include "llparcel.h"
#include "lldbstrings.h"
#include "llclickaction.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llavatarnamecache.h"
#include "llcachename.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llpermissions.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolselectland.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llglheaders.h"
#include "llviewertexturelist.h"
#include "llhudmanager.h"
#include "llviewermenu.h"
#include "llviewermedia.h"
#include "llmediaentry.h"
#include "llhudmanager.h"
#include "llhudeffect.h"
#include "hippogridmanager.h"
#include "rlvhandler.h"
const char* DEFAULT_DESC = "(No Description)";
const F32 DELAY_BEFORE_SHOW_TIP = 0.35f;
const F32 DELAY_BEFORE_REFRESH_TIP = 0.50f;
LLHoverView *gHoverView = NULL;
BOOL LLHoverView::sShowHoverTips = TRUE;
LLHoverView::LLHoverView(const std::string& name, const LLRect& rect)
:	LLView(name, rect, FALSE)
{
	mDoneHoverPick = FALSE;
	mStartHoverPickTimer = FALSE;
	mHoverActive = FALSE;
	mUseHover = TRUE;
	mTyping = FALSE;
	mHoverOffset.clearVec();
	mLastTextHoverObject = NULL;
}
LLHoverView::~LLHoverView()
{
}
void LLHoverView::updateHover(LLTool* current_tool)
{
	BOOL picking_tool = (	current_tool == LLToolPie::getInstance()
							|| current_tool == LLToolSelectLand::getInstance() );
	mUseHover = !gAgentCamera.cameraMouselook()
				&& picking_tool
				&& !mTyping;
	if (mUseHover)
	{
		if ((gViewerWindow->getMouseVelocityStat()->getPrev(0) < 0.01f)
			&& (LLViewerCamera::getInstance()->getAngularVelocityStat()->getPrev(0) < 0.01f)
			&& (LLViewerCamera::getInstance()->getVelocityStat()->getPrev(0) < 0.01f))
		{
			if (!mStartHoverPickTimer)
			{
				mStartHoverTimer.reset();
				mStartHoverPickTimer = TRUE;
				mText.clear();
				mLastTextHoverObject = NULL;
			}
			if (mDoneHoverPick)
			{
				updateText();
			}
			else if (mStartHoverTimer.getElapsedTimeF32() > DELAY_BEFORE_SHOW_TIP)
			{
				gViewerWindow->pickAsync(gViewerWindow->getCurrentMouseX(),
													 gViewerWindow->getCurrentMouseY(), 0, pickCallback );
			}
		}
		else
		{
			cancelHover();
		}
	}
}
void LLHoverView::pickCallback(const LLPickInfo& pick_info)
{
	gHoverView->mLastPickInfo = pick_info;
	LLViewerObject* hit_obj = pick_info.getObject();
	if (hit_obj)
	{
		gHoverView->setHoverActive(TRUE);
		LLSelectMgr::getInstance()->setHoverObject(hit_obj, pick_info.mObjectFace);
		gHoverView->mLastHoverObject = hit_obj;
		gHoverView->mHoverOffset = pick_info.mObjectOffset;
	}
	else
	{
		gHoverView->mLastHoverObject = NULL;
	}
	if (!hit_obj && pick_info.mPosGlobal != LLVector3d::zero)
	{
		gHoverView->setHoverActive(TRUE);
		gHoverView->mHoverLandGlobal = pick_info.mPosGlobal;
		LLViewerParcelMgr::getInstance()->setHoverParcel( gHoverView->mHoverLandGlobal );
	}
	else
	{
		gHoverView->mHoverLandGlobal = LLVector3d::zero;
	}
	gHoverView->mDoneHoverPick = TRUE;
}
void LLHoverView::setTyping(BOOL b)
{
	mTyping = b;
}
void LLHoverView::cancelHover()
{
	mStartHoverTimer.reset();
	mDoneHoverPick = FALSE;
	mStartHoverPickTimer = FALSE;
	LLSelectMgr::getInstance()->setHoverObject(NULL);
	setHoverActive(FALSE);
}
void LLHoverView::resetLastHoverObject()
{
	mLastHoverObject = NULL;
}
void LLHoverView::updateText()
{
	LLViewerObject* hit_object = getLastHoverObject();
	std::string line;
	if (hit_object == mLastTextHoverObject &&
		!(mLastTextHoverObjectTimer.getStarted() && mLastTextHoverObjectTimer.hasExpired()))
	{
		return;
	}
	mLastTextHoverObject = hit_object;
	mLastTextHoverObjectTimer.stop();
	bool retrieving_data = false;
	mText.clear();
	if ( hit_object )
	{
		if ( hit_object->isHUDAttachment() )
		{
			return;
		}
		if ( hit_object->isAttachment() )
		{
			LLViewerObject* root_edit = hit_object->getRootEdit();
			if (!root_edit)
			{
				return;
			}
			hit_object = (LLViewerObject*)root_edit->getParent();
			if (!hit_object)
			{
				return;
			}
		}
		line.clear();
		if (hit_object->isAvatar())
		{
			if (gAgentAvatarp != hit_object && gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS))
				return;
			LLNameValue* title = hit_object->getNVPair("Title");
			LLNameValue* firstname = hit_object->getNVPair("FirstName");
			LLNameValue* lastname =  hit_object->getNVPair("LastName");
			if (firstname && lastname)
			{
				if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
				{
					line = RlvStrings::getAnonym(line.append(firstname->getString()).append(1, ' ').append(lastname->getString()));
				}
				else
				{
					std::string complete_name;
					if (!LLAvatarNameCache::getNSName(hit_object->getID(), complete_name))
						complete_name = firstname->getString() + std::string(" ") + lastname->getString();
					if (title)
					{
						line.append(title->getString());
						line.append(1, ' ');
					}
					line += complete_name;
				}
			}
			else
			{
				line.append(LLTrans::getString("TooltipPerson"));
			}
			mText.push_back(line);
			mText.push_back(LLTrans::getString("Complexity", LLSD().with("NUM", static_cast<LLSD::Integer>(hit_object->asAvatar()->getVisualComplexity()))));
		}
		else
		{
			BOOL suppressObjectHoverDisplay = !gSavedSettings.getBOOL("ShowAllObjectHoverTip");
			LLSelectNode *nodep = LLSelectMgr::getInstance()->getHoverNode();
			if (nodep)
			{
				line.clear();
				bool for_copy = nodep->mValid && nodep->mPermissions->getMaskEveryone() & PERM_COPY && hit_object && hit_object->permCopy();
				bool for_sale = nodep->mValid && for_sale_selection(nodep);
				bool has_media = false;
				bool is_time_based_media = false;
				bool is_web_based_media = false;
				bool is_media_playing = false;
				bool is_media_displaying = false;
				const LLTextureEntry* tep = hit_object ? hit_object->getTE(mLastPickInfo.mObjectFace) : NULL;
				if(tep)
				{
					has_media = tep->hasMedia();
					const LLMediaEntry* mep = has_media ? tep->getMediaData() : NULL;
					if (mep)
					{
						viewer_media_t media_impl = LLViewerMedia::getMediaImplFromTextureID(mep->getMediaID());
						LLPluginClassMedia* media_plugin = NULL;
						if (media_impl.notNull() && (media_impl->hasMedia()))
						{
							is_media_displaying = true;
							media_plugin = media_impl->getMediaPlugin();
							if(media_plugin)
							{
								if(media_plugin->pluginSupportsMediaTime())
								{
									is_time_based_media = true;
									is_web_based_media = false;
									is_media_playing = media_impl->isMediaPlaying();
								}
								else
								{
									is_time_based_media = false;
									is_web_based_media = true;
								}
							}
						}
					}
				}
				if(!suppressObjectHoverDisplay || !is_media_displaying || for_sale)
				{
					if (nodep->mName.empty())
					{
						line.append(LLTrans::getString("TooltipNoName"));
					}
					else
					{
						line.append( nodep->mName );
					}
					mText.push_back(line);
					if (!nodep->mDescription.empty()
						&& nodep->mDescription != DEFAULT_DESC)
					{
						mText.push_back( nodep->mDescription );
					}
					line.clear();
					line.append(LLTrans::getString("TooltipOwner") + " ");
					if (nodep->mValid)
					{
						LLUUID owner;
						std::string name;
						if (!nodep->mPermissions->isGroupOwned())
						{
							owner = nodep->mPermissions->getOwner();
							if (LLUUID::null == owner)
							{
								line.append(LLTrans::getString("TooltipPublic"));
							}
							else if (LLAvatarNameCache::getNSName(owner, name))
							{
								if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
								{
									name = RlvStrings::getAnonym(name);
								}
								line.append(name);
							}
							else
							{
								line.append(LLTrans::getString("RetrievingData"));
								retrieving_data = true;
							}
						}
						else
						{
							std::string name;
							owner = nodep->mPermissions->getGroup();
							if (gCacheName->getGroupName(owner, name))
							{
								line.append(name);
								line.append(LLTrans::getString("TooltipIsGroup"));
							}
							else
							{
								line.append(LLTrans::getString("RetrievingData"));
								retrieving_data = true;
							}
						}
					}
					else
					{
						line.append(LLTrans::getString("RetrievingData"));
						retrieving_data = true;
					}
					mText.push_back(line);
					LLViewerObject *object = hit_object;
					LLViewerObject *parent = (LLViewerObject *)object->getParent();
					if (object &&
						(object->flagUsePhysics() ||
						 object->flagScripted() ||
						 object->flagHandleTouch() || (parent && parent->flagHandleTouch()) ||
						 object->flagTakesMoney() || (parent && parent->flagTakesMoney()) ||
						 object->flagAllowInventoryAdd() ||
						 object->flagTemporary() ||
						 object->flagPhantom()) )
					{
						line.clear();
						if (object->flagScripted())
						{
							line.append(LLTrans::getString("TooltipFlagScript") + " ");
						}
						if (object->flagUsePhysics())
						{
							line.append(LLTrans::getString("TooltipFlagPhysics") + " ");
						}
						if (object->flagHandleTouch() || (parent && parent->flagHandleTouch()) )
						{
							line.append(LLTrans::getString("TooltipFlagTouch") + " ");
							suppressObjectHoverDisplay = FALSE;
						}
						if (object->flagTakesMoney() || (parent && parent->flagTakesMoney()) )
						{
							line.append(gHippoGridManager->getConnectedGrid()->getCurrencySymbol() + " ");
							suppressObjectHoverDisplay = FALSE;
						}
						if (object->flagAllowInventoryAdd())
						{
							line.append(LLTrans::getString("TooltipFlagDropInventory") + " ");
							suppressObjectHoverDisplay = FALSE;
						}
						if (object->flagPhantom())
						{
							line.append(LLTrans::getString("TooltipFlagPhantom") + " ");
						}
						if (object->flagTemporary())
						{
							line.append(LLTrans::getString("TooltipFlagTemporary") + " ");
						}
						if (object->flagUsePhysics() ||
							object->flagHandleTouch() ||
							(parent && parent->flagHandleTouch()) )
						{
							line.append(LLTrans::getString("TooltipFlagRightClickMenu") + " ");
						}
						mText.push_back(line);
					}
					line.clear();
					if (nodep->mValid)
					{
						if (for_copy)
						{
							line.append(LLTrans::getString("TooltipFreeToCopy"));
							suppressObjectHoverDisplay = FALSE;
						}
						else if (for_sale)
						{
							LLStringUtil::format_map_t args;
							args["[AMOUNT]"] = llformat("%d", nodep->mSaleInfo.getSalePrice());
							line.append(LLTrans::getString("TooltipForSaleL$", args));
							suppressObjectHoverDisplay = FALSE;
						}
						else
						{
						}
					}
					else
					{
						LLStringUtil::format_map_t args;
						args["[MESSAGE]"] = LLTrans::getString("RetrievingData");
						retrieving_data = true;
						line.append(LLTrans::getString("TooltipForSaleMsg", args));
					}
					mText.push_back(line);
					auto objects = LLSelectMgr::getInstance()->getHoverObjects();
					line.clear();
					S32 prim_count = objects->getObjectCount();
					line.append(llformat("Prims: %d", prim_count));
					mText.push_back(line);
					mText.push_back(llformat("LI: %.2f", objects->getSelectedLinksetCost()));
					line.clear();
					line.append("Position: ");
					LLViewerRegion *region = gAgent.getRegion();
					LLVector3 position = region->getPosRegionFromGlobal(hit_object->getPositionGlobal());
					LLVector3 mypos = region->getPosRegionFromGlobal(gAgent.getPositionGlobal());
					LLVector3 delta = position - mypos;
					F32 distance = (F32)delta.magVec();
					line.append(llformat("<%.02f,%.02f,%.02f>",position.mV[0],position.mV[1],position.mV[2]));
					mText.push_back(line);
					line.clear();
					line.append(llformat("Distance: %.02fm",distance));
					mText.push_back(line);
				}
				else
				{
					suppressObjectHoverDisplay = TRUE;
				}
				if (suppressObjectHoverDisplay)
				{
					mText.clear();
				}
			}
		}
	}
	else if ( mHoverLandGlobal != LLVector3d::zero )
	{
		if (!gSavedSettings.getBOOL("ShowLandHoverTip")) return;
		LLParcel* hover_parcel = LLViewerParcelMgr::getInstance()->getHoverParcel();
		LLUUID owner;
		if ( hover_parcel )
		{
			owner = hover_parcel->getOwnerID();
		}
		line.clear();
		line.append(LLTrans::getString("TooltipLand"));
		if (hover_parcel)
		{
			line.append( (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
				? hover_parcel->getName() : RlvStrings::getString(RLV_STRING_HIDDEN_PARCEL) );
		}
		mText.push_back(line);
		line.clear();
		line.append(LLTrans::getString("TooltipOwner") + " ");
		if ( hover_parcel )
		{
			std::string name;
			if (LLUUID::null == owner)
			{
				line.append(LLTrans::getString("TooltipPublic"));
			}
			else if (hover_parcel->getIsGroupOwned())
			{
				if (gCacheName->getGroupName(owner, name))
				{
					line.append(name);
					line.append(LLTrans::getString("TooltipIsGroup"));
				}
				else
				{
					line.append(LLTrans::getString("RetrievingData"));
					retrieving_data = true;
				}
			}
			else if(gCacheName->getFullName(owner, name))
			{
				line.append( (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) ? name : RlvStrings::getAnonym(name));
			}
			else
			{
				line.append(LLTrans::getString("RetrievingData"));
				retrieving_data = true;
			}
		}
		else
		{
			line.append(LLTrans::getString("RetrievingData"));
			retrieving_data = true;
		}
		mText.push_back(line);
		if ( hover_parcel && owner != gAgent.getID() )
		{
			S32 words = 0;
			line.clear();
			if ( !hover_parcel->getAllowModify() )
			{
				if ( hover_parcel->getAllowGroupModify() )
				{
					line.append(LLTrans::getString("TooltipFlagGroupBuild"));
				}
				else
				{
					line.append(LLTrans::getString("TooltipFlagNoBuild"));
				}
				words++;
			}
			if ( !hover_parcel->getAllowTerraform() )
			{
				if (words) line.append(", ");
				line.append(LLTrans::getString("TooltipFlagNoEdit"));
				words++;
			}
			if ( hover_parcel->getAllowDamage() )
			{
				if (words) line.append(", ");
				line.append(LLTrans::getString("TooltipFlagNotSafe"));
				words++;
			}
			if ( !hover_parcel->getAllowFly() )
			{
				if (words) line.append(", ");
				line.append(LLTrans::getString("TooltipFlagNoFly"));
				words++;
			}
			if ( !hover_parcel->getAllowOtherScripts() )
			{
				if (words) line.append(", ");
				if ( hover_parcel->getAllowGroupScripts() )
				{
					line.append(LLTrans::getString("TooltipFlagGroupScripts"));
				}
				else
				{
					line.append(LLTrans::getString("TooltipFlagNoScripts"));
				}
				words++;
			}
			if (words)
			{
				mText.push_back(line);
			}
		}
		if (hover_parcel && hover_parcel->getParcelFlag(PF_FOR_SALE))
		{
			LLStringUtil::format_map_t args;
			args["[AMOUNT]"] = llformat("%d", hover_parcel->getSalePrice());
			line = LLTrans::getString("TooltipForSaleL$", args);
			mText.push_back(line);
		}
	}
	if (retrieving_data)
	{
		mLastTextHoverObjectTimer.start(DELAY_BEFORE_REFRESH_TIP);
	}
}
void LLHoverView::draw()
{
	if ( !isHovering() )
	{
		return;
	}
#ifdef RLV_EXTENSION_CMD_INTERACT
	if ( (!sShowHoverTips) || (gRlvHandler.hasBehaviour(RLV_BHVR_INTERACT)) )
#else
	if (!sShowHoverTips)
#endif
	{
		return;
	}
	const F32 MAX_HOVER_DISPLAY_SECS = 5.f;
	if (mHoverTimer.getElapsedTimeF32() > MAX_HOVER_DISPLAY_SECS)
	{
		return;
	}
	const F32 MAX_ALPHA = 0.9f;
	F32 alpha;
	if (mHoverActive)
	{
		alpha = 1.f;
		if (isHoveringObject())
		{
			LLViewerObject *hover_object = getLastHoverObject();
			if (hover_object->isAvatar())
			{
				gAgentCamera.setLookAt(LOOKAT_TARGET_HOVER, getLastHoverObject(), LLVector3::zero);
			}
			else
			{
				gAgentCamera.setLookAt(LOOKAT_TARGET_HOVER, getLastHoverObject(), mHoverOffset);
			}
		}
	}
	else
	{
		alpha = llmax(0.f, MAX_ALPHA - mHoverTimer.getElapsedTimeF32()*2.f);
	}
	if (mText.empty())
	{
		return;
	}
	if (alpha <= 0.f)
	{
		return;
	}
	LLUIImagePtr box_imagep = LLUI::getUIImage("Rounded_Square");
	LLUIImagePtr shadow_imagep = LLUI::getUIImage("Rounded_Square_Soft");
	const LLFontGL* fontp = LLResMgr::getInstance()->getRes(LLFONT_SANSSERIF_SMALL);
	LLColor4 text_color = gColors.getColor("ToolTipTextColor");
	LLColor4 bg_color = gColors.getColor("ToolTipBgColor");
	LLColor4 shadow_color = gColors.getColor("ColorDropShadow");
	S32 max_width = 0;
	S32 num_lines = mText.size();
	for (text_list_t::iterator iter = mText.begin(); iter != mText.end(); ++iter)
	{
		max_width = llmax(max_width, (S32)fontp->getWidth(*iter));
	}
	S32 left	= mHoverPos.mX + 10;
	S32 top		= mHoverPos.mY - 16;
	S32 right	= mHoverPos.mX + max_width + 30;
	S32 bottom	= mHoverPos.mY - 24 - llfloor(num_lines*fontp->getLineHeight());
	if (mHoverActive
		&& isHoveringObject()
		&& mLastHoverObject->getClickAction() != CLICK_ACTION_NONE)
	{
		const S32 CLICK_OFFSET = 10;
		top -= CLICK_OFFSET;
		bottom -= CLICK_OFFSET;
	}
	LLRect old_rect = getRect();
	setRect( LLRect(left, top, right, bottom ) );
	translateIntoRect( gViewerWindow->getVirtualWindowRect(), FALSE );
	left = getRect().mLeft;
	top = getRect().mTop;
	right = getRect().mRight;
	bottom = getRect().mBottom;
	setRect(old_rect);
	LLGLSUIDefault gls_ui;
	shadow_color.mV[VALPHA] = 0.7f * alpha;
	S32 shadow_offset = gSavedSettings.getS32("DropShadowTooltip");
	shadow_imagep->draw(LLRect(left + shadow_offset, top - shadow_offset, right + shadow_offset, bottom - shadow_offset), shadow_color);
	bg_color.mV[VALPHA] = alpha;
	box_imagep->draw(LLRect(left, top, right, bottom), bg_color);
	S32 cur_offset = top - 4;
	for (text_list_t::iterator iter = mText.begin(); iter != mText.end(); ++iter)
	{
		fontp->renderUTF8(*iter, 0, left + 10, cur_offset, text_color, LLFontGL::LEFT, LLFontGL::TOP);
		cur_offset -= llfloor(fontp->getLineHeight());
	}
}
void LLHoverView::setHoverActive(const BOOL active)
{
	if (active != mHoverActive)
	{
		mHoverTimer.reset();
	}
	mHoverActive = active;
	if (active)
	{
		mHoverPos = gViewerWindow->getCurrentMouse();
	}
}
BOOL LLHoverView::isHoveringLand() const
{
	return !mHoverLandGlobal.isExactlyZero();
}
BOOL LLHoverView::isHoveringObject() const
{
	return !mLastHoverObject.isNull() && !mLastHoverObject->isDead();
}
LLViewerObject* LLHoverView::getLastHoverObject() const
{
	if (!mLastHoverObject.isNull() && !mLastHoverObject->isDead())
	{
		return mLastHoverObject;
	}
	else
	{
		return NULL;
	}
}
