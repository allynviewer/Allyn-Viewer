/** 
 * @file lltoolbar.cpp
 * @author James Cook, Richard Nelson
 * @brief Large friendly buttons at bottom of screen.
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
#include "lltoolbar.h"
#include "llbutton.h"
#include "llcontrol.h"
#include "llfloatertoolbarprefs.h"
#include "llflyoutbutton.h"
#include "llfocusmgr.h"
#include "llmenugl.h"
#include "llscrolllistitem.h"
#include "llui.h"
#include "llwindow.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloaterfriends.h"
#include "llfloaterinventory.h"
#include "llfloatermute.h"
#include "llimpanel.h"
#include "llimview.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "llviewerparcelmgr.h"
#include "llvoavatarself.h"
#include "rlvhandler.h"
LLToolBar *gToolBar = NULL;
S32 TOOL_BAR_HEIGHT = 25;
F32	LLToolBar::sInventoryAutoOpenTime = 1.f;
void show_floater(const std::string& floater_name);
void show_inv_floater(const LLSD& userdata, const std::string& field);
void show_web_floater(const std::string& type);
LLToolBar::LLToolBar()
:	LLLayoutPanel()
,	mToolbarStack(NULL)
,	mContextMenu(NULL)
,	mDragStartPanel(NULL)
,	mDragStartX(0)
,	mDragStartY(0)
,	mDragging(false)
,	mOrderDirty(false)
{
	setIsChrome(TRUE);
	setFocusRoot(TRUE);
	mCommitCallbackRegistrar.add("ShowFloater", boost::bind(show_floater, _2));
	mCommitCallbackRegistrar.add("ShowInvFloater.ID", boost::bind(show_inv_floater, _2, "id"));
	mCommitCallbackRegistrar.add("ShowInvFloater.Name", boost::bind(show_inv_floater, _2, "name"));
	mCommitCallbackRegistrar.add("ShowInvFloater.Type", boost::bind(show_inv_floater, _2, "type"));
	mCommitCallbackRegistrar.add("ShowWebFloater", boost::bind(show_web_floater, _2));
}
BOOL LLToolBar::postBuild()
{
	mCommunicateBtn.connect(this, "communicate_btn");
	mCommunicateBtn->setCommitCallback(boost::bind(&LLToolBar::onClickCommunicate, this, _2));
	mFlyBtn.connect(this, "fly_btn");
	mBuildBtn.connect(this, "build_btn");
	mMapBtn.connect(this, "map_btn");
	mRadarBtn.connect(this, "radar_btn");
	mInventoryBtn.connect(this, "inventory_btn");
	mToolbarStack = findChild<LLLayoutStack>("toolbar_stack");
	hookToolbarButtons(this);
	applySavedButtonOrder();
	return TRUE;
}
LLToolBar::~LLToolBar()
{
	if (mContextMenu)
	{
		mContextMenu->die();
		mContextMenu = NULL;
	}
}
BOOL LLToolBar::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	LLButton* inventory_btn = mInventoryBtn;
	if (!inventory_btn || !inventory_btn->getVisible()) return FALSE;
	LLPanelMainInventory* active_inventory = LLPanelMainInventory::getActiveInventory();
	if (active_inventory && active_inventory->getVisible())
	{
		mInventoryAutoOpenTimer.stop();
	}
	else if (inventory_btn->getRect().pointInRect(x, y))
	{
		if (mInventoryAutoOpenTimer.getStarted())
		{
			if (!(active_inventory && active_inventory->getVisible()) &&
			mInventoryAutoOpenTimer.getElapsedTimeF32() > sInventoryAutoOpenTime)
			{
				LLPanelMainInventory::showAgentInventory();
			}
		}
		else
		{
			mInventoryAutoOpenTimer.start();
		}
	}
	return LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
}
void LLToolBar::refresh()
{
	static const LLCachedControl<bool> show_toolbar("ShowToolBar", true);
	bool show = show_toolbar;
	if (show && gAgentCamera.cameraMouselook())
	{
		static const LLCachedControl<bool> hidden("LiruMouselookHidesToolbar");
		show = !hidden;
	}
	setVisible(show);
	if (!show) return;
	updateCommunicateList();
	pollToolbarDrag();
	if (!isAgentAvatarValid()) return;
	static const LLCachedControl<bool> continue_flying_on_unsit("LiruContinueFlyingOnUnsit");
	mFlyBtn->setEnabled((gAgent.canFly() || gAgent.getFlying()) && (continue_flying_on_unsit || !gAgentAvatarp->isSitting()));
	static const LLCachedControl<bool> ascent_build_always_enabled("AscentBuildAlwaysEnabled", true);
	mBuildBtn->setEnabled(ascent_build_always_enabled || LLViewerParcelMgr::getInstance()->allowAgentBuild());
	bool build_mode = LLToolMgr::getInstance()->inEdit() && !LLToolGrab::getInstance()->getHideBuildHighlight();
	static LLCachedControl<bool> build_btn_state("BuildBtnState",false);
	if (build_btn_state != build_mode)
		build_btn_state = build_mode;
	if (rlv_handler_t::isEnabled())
	{
		mBuildBtn->setEnabled(!(gRlvHandler.hasBehaviour(RLV_BHVR_REZ) && gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)));
		mMapBtn->setEnabled(!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWWORLDMAP));
		mRadarBtn->setEnabled(!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWMINIMAP));
		mInventoryBtn->setEnabled(!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWINV));
	}
}
void bold_if_equal(const LLFloater* f1, const LLFloater* f2, LLScrollListItem* itemp)
{
	if (f1 != f2) return;
	static_cast<LLScrollListText*>(itemp->getColumn(0))->setFontStyle(LLFontGL::BOLD);
}
void LLToolBar::updateCommunicateList()
{
	if (!mCommunicateBtn->getVisible()) return;
	LLSD selected = mCommunicateBtn->getValue();
	mCommunicateBtn->removeall();
	const LLFloater* frontmost_floater = LLFloaterChatterBox::getInstance()->getActiveFloater();
	bold_if_equal(LLFloaterMyFriends::getInstance(), frontmost_floater, mCommunicateBtn->add(LLFloaterMyFriends::getInstance()->getShortTitle(), LLSD("contacts"), ADD_TOP));
	bold_if_equal(LLFloaterChat::getInstance(), frontmost_floater, mCommunicateBtn->add(LLFloaterChat::getInstance()->getShortTitle(), LLSD("local chat"), ADD_TOP));
	mCommunicateBtn->addSeparator(ADD_TOP);
	static const auto redock = getString("Redock Windows");
	mCommunicateBtn->add(redock, LLSD("redock"), ADD_TOP);
	mCommunicateBtn->addSeparator(ADD_TOP);
	bold_if_equal(LLFloaterMute::getInstance(), frontmost_floater, mCommunicateBtn->add(LLFloaterMute::getInstance()->getShortTitle(), LLSD("mute list"), ADD_TOP));
	if (gIMMgr->getIMFloaterHandles().size() > 0) mCommunicateBtn->addSeparator(ADD_TOP);
	for(const auto& handle : gIMMgr->getIMFloaterHandles())
	{
		if (LLFloaterIMPanel* im_floaterp = (LLFloaterIMPanel*)handle.get())
		{
			const S32 count = im_floaterp->getNumUnreadMessages();
			std::string floater_title;
			if (count > 0) floater_title = '*';
			floater_title.append(im_floaterp->getShortTitle());
			static const LLCachedControl<bool> show_counts("ShowUnreadIMsCounts", true);
			if (show_counts && count > 0)
			{
				floater_title += " - ";
				if (count > 1)
				{
					LLStringUtil::format_map_t args;
					args["COUNT"] = llformat("%d", count);
					static LLUIString ims = getString("IMs");
					ims.setArgList(args);
					floater_title += ims.getString();
				}
				else
				{
					static const auto im = getString("IM");
					floater_title += im;
				}
			}
			bold_if_equal(im_floaterp, frontmost_floater, mCommunicateBtn->add(floater_title, im_floaterp->getSessionID(), ADD_TOP));
		}
	}
	static const LLCachedControl<bool> show_comm("ShowCommunicate", true);
	mCommunicateBtn->setToggleState(show_comm);
	if (!selected.isUndefined()) mCommunicateBtn->setValue(selected);
}
void LLToolBar::onClickCommunicate(const LLSD& selected_option)
{
	if (selected_option.asString() == "contacts")
	{
		LLFloaterMyFriends::showInstance();
	}
	else if (selected_option.asString() == "local chat")
	{
		LLFloaterChat::showInstance();
	}
	else if (selected_option.asString() == "redock")
	{
		LLFloaterChatterBox::getInstance()->addFloater(LLFloaterMyFriends::getInstance(), FALSE);
		LLFloaterChatterBox::getInstance()->addFloater(LLFloaterChat::getInstance(), FALSE);
		LLUUID session_to_show;
		std::set<LLHandle<LLFloater> >::const_iterator floater_handle_it;
		for(floater_handle_it = gIMMgr->getIMFloaterHandles().begin(); floater_handle_it != gIMMgr->getIMFloaterHandles().end(); ++floater_handle_it)
		{
			LLFloater* im_floaterp = floater_handle_it->get();
			if (im_floaterp)
			{
				if (im_floaterp->isFrontmost())
				{
					session_to_show = ((LLFloaterIMPanel*)im_floaterp)->getSessionID();
				}
				LLFloaterChatterBox::getInstance()->addFloater(im_floaterp, FALSE);
			}
		}
		LLFloaterChatterBox::showInstance(session_to_show);
	}
	else if (selected_option.asString() == "mute list")
	{
		LLFloaterMute::showInstance();
	}
	else if (selected_option.isUndefined())
	{
		if (LLFloaterChatterBox::getInstance()->getFloaterCount() == 0)
		{
			LLFloaterMyFriends::toggleInstance();
		}
		else
		{
			LLFloaterChatterBox::toggleInstance();
		}
	}
	else
	{
		LLFloaterChatterBox::showInstance(selected_option);
	}
}
static void open_toolbar_prefs(void*)
{
	LLFloaterToolbarPrefs::showInstance();
}
void LLToolBar::hookToolbarButtons(LLView* view)
{
	if (!view)
	{
		return;
	}
	if (LLButton* btn = dynamic_cast<LLButton*>(view))
	{
		btn->LLUICtrl::setMouseDownCallback(boost::bind(&LLToolBar::onToolbarButtonMouseDown, this, _1, _2, _3, _4));
		btn->setRightMouseDownCallback(boost::bind(&LLToolBar::onToolbarButtonRightClick, this, _1, _2, _3, _4));
	}
	const child_list_t* children = view->getChildList();
	for (LLView* child : *children)
	{
		hookToolbarButtons(child);
	}
}
void LLToolBar::onToolbarButtonMouseDown(LLUICtrl* ctrl, S32 x, S32 y, MASK mask)
{
	if (!ctrl)
	{
		return;
	}
	S32 local_x = x;
	S32 local_y = y;
	ctrl->localPointToOtherView(x, y, &local_x, &local_y, this);
	mDragStartPanel = findLayoutPanelFromChild(ctrl);
	mDragStartX = local_x;
	mDragStartY = local_y;
	mDragging = false;
	mOrderDirty = false;
}
void LLToolBar::onToolbarButtonRightClick(LLUICtrl* ctrl, S32 x, S32 y, MASK mask)
{
	gFocusMgr.setMouseCapture(NULL);
	S32 local_x = x;
	S32 local_y = y;
	if (ctrl)
	{
		ctrl->localPointToOtherView(x, y, &local_x, &local_y, this);
	}
	showToolbarContextMenu(local_x, local_y);
}
void LLToolBar::showToolbarContextMenu(S32 x, S32 y)
{
	if (!mContextMenu)
	{
		mContextMenu = new LLMenuGL("toolbar_context");
		mContextMenu->setCanTearOff(FALSE);
		mContextMenu->addChild(new LLMenuItemCallGL(getString("ToolbarButtons"), open_toolbar_prefs));
		mContextMenu->updateParent(LLMenuGL::sMenuContainer);
	}
	gFocusMgr.setMouseCapture(NULL);
	LLMenuGL::showPopup(this, mContextMenu, x, y);
}
BOOL LLToolBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (!childrenHandleRightMouseDown(x, y, mask))
	{
		showToolbarContextMenu(x, y);
	}
	return TRUE;
}
BOOL LLToolBar::handleHover(S32 x, S32 y, MASK mask)
{
	if (mDragging)
	{
		updateButtonDrag(x, y);
		getWindow()->setCursor(UI_CURSOR_ARROWDRAG);
		return TRUE;
	}
	return LLLayoutPanel::handleHover(x, y, mask);
}
BOOL LLToolBar::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (mDragging)
	{
		updateButtonDrag(x, y);
		if (mOrderDirty)
		{
			saveButtonOrder();
		}
		mDragging = false;
		mDragStartPanel = NULL;
		mOrderDirty = false;
		if (hasMouseCapture())
		{
			gFocusMgr.setMouseCapture(NULL);
		}
		return TRUE;
	}
	mDragStartPanel = NULL;
	return LLLayoutPanel::handleMouseUp(x, y, mask);
}
void LLToolBar::onMouseCaptureLost()
{
	mDragging = false;
	mDragStartPanel = NULL;
	LLLayoutPanel::onMouseCaptureLost();
}
void LLToolBar::pollToolbarDrag()
{
	if (mDragging || !mDragStartPanel)
	{
		return;
	}
	LLMouseHandler* captor = gFocusMgr.getMouseCapture();
	if (!captor)
	{
		mDragStartPanel = NULL;
		return;
	}
	LLView* captor_view = dynamic_cast<LLView*>(captor);
	if (!captor_view || (captor_view != this && !captor_view->hasAncestor(this)))
	{
		mDragStartPanel = NULL;
		return;
	}
	S32 x = 0;
	S32 y = 0;
	LLUI::getMousePositionLocal(this, &x, &y);
	const S32 dx = x - mDragStartX;
	const S32 dy = y - mDragStartY;
	if (dx * dx + dy * dy >= 64)
	{
		mDragging = true;
		gFocusMgr.setMouseCapture(this);
		updateButtonDrag(x, y);
	}
}
std::string LLToolBar::getPanelGroupKey(LLLayoutPanel* panel) const
{
	if (!panel)
	{
		return LLStringUtil::null;
	}
	if (LLControlVariable* vis = panel->getMakeVisibleControlVariable())
	{
		return vis->getName();
	}
	return panel->getName();
}
bool LLToolBar::getGroupRange(LLLayoutPanel* panel, S32& start, S32& count) const
{
	if (!mToolbarStack || !panel)
	{
		return false;
	}
	const S32 idx = mToolbarStack->getPanelIndex(panel);
	if (idx < 0)
	{
		return false;
	}
	const std::string key = getPanelGroupKey(panel);
	start = idx;
	while (start > 0 && getPanelGroupKey(mToolbarStack->getPanelByIndex(start - 1)) == key)
	{
		--start;
	}
	S32 end = idx;
	const S32 n = mToolbarStack->getNumPanels();
	while (end + 1 < n && getPanelGroupKey(mToolbarStack->getPanelByIndex(end + 1)) == key)
	{
		++end;
	}
	count = end - start + 1;
	return true;
}
LLLayoutPanel* LLToolBar::findLayoutPanelFromChild(LLView* view) const
{
	while (view)
	{
		if (LLLayoutPanel* panel = dynamic_cast<LLLayoutPanel*>(view))
		{
			if (mToolbarStack && mToolbarStack->getPanelIndex(panel) >= 0)
			{
				return panel;
			}
		}
		view = view->getParent();
	}
	return NULL;
}
LLLayoutPanel* LLToolBar::findPanelAt(S32 x, S32 y) const
{
	if (!mToolbarStack)
	{
		return NULL;
	}
	S32 sx = x;
	S32 sy = y;
	if (!localPointToOtherView(x, y, &sx, &sy, mToolbarStack))
	{
		return NULL;
	}
	const S32 n = mToolbarStack->getNumPanels();
	for (S32 i = 0; i < n; ++i)
	{
		LLLayoutPanel* panel = mToolbarStack->getPanelByIndex(i);
		if (panel && panel->getVisible() && panel->getRect().pointInRect(sx, sy))
		{
			return panel;
		}
	}
	return NULL;
}
void LLToolBar::updateButtonDrag(S32 x, S32 y)
{
	if (!mDragging || !mDragStartPanel || !mToolbarStack)
	{
		return;
	}
	LLLayoutPanel* over = findPanelAt(x, y);
	if (!over || over == mDragStartPanel)
	{
		return;
	}
	if (getPanelGroupKey(over) == getPanelGroupKey(mDragStartPanel))
	{
		return;
	}
	S32 src_start = 0;
	S32 src_count = 0;
	S32 dst_start = 0;
	S32 dst_count = 0;
	if (!getGroupRange(mDragStartPanel, src_start, src_count) || !getGroupRange(over, dst_start, dst_count))
	{
		return;
	}
	S32 sx = x;
	S32 sy = y;
	localPointToOtherView(x, y, &sx, &sy, mToolbarStack);
	LLLayoutPanel* first_dst = mToolbarStack->getPanelByIndex(dst_start);
	LLLayoutPanel* last_dst = mToolbarStack->getPanelByIndex(dst_start + dst_count - 1);
	if (!first_dst || !last_dst)
	{
		return;
	}
	const S32 mid_x = (first_dst->getRect().mLeft + last_dst->getRect().mRight) / 2;
	const S32 insert_index = (sx < mid_x) ? dst_start : (dst_start + dst_count);
	if (insert_index == src_start || (insert_index > src_start && insert_index <= src_start + src_count))
	{
		return;
	}
	mToolbarStack->movePanelRange(src_start, src_count, insert_index);
	mToolbarStack->updateLayout();
	mOrderDirty = true;
}
void LLToolBar::applySavedButtonOrder()
{
	if (!mToolbarStack)
	{
		return;
	}
	const LLSD order = gSavedSettings.getLLSD("ToolbarButtonOrder");
	if (!order.isArray() || order.size() == 0)
	{
		return;
	}
	std::vector<std::string> desired;
	desired.reserve(order.size());
	for (LLSD::array_const_iterator it = order.beginArray(); it != order.endArray(); ++it)
	{
		const std::string key = it->asString();
		if (!key.empty())
		{
			desired.push_back(key);
		}
	}
	if (desired.empty())
	{
		return;
	}
	std::set<std::string> seen;
	S32 insert_at = 0;
	const S32 n = mToolbarStack->getNumPanels();
	for (const std::string& key : desired)
	{
		if (!seen.insert(key).second)
		{
			continue;
		}
		for (S32 i = insert_at; i < n; )
		{
			LLLayoutPanel* panel = mToolbarStack->getPanelByIndex(i);
			if (!panel || getPanelGroupKey(panel) != key)
			{
				++i;
				continue;
			}
			S32 start = 0;
			S32 count = 0;
			if (!getGroupRange(panel, start, count))
			{
				break;
			}
			if (start != insert_at)
			{
				mToolbarStack->movePanelRange(start, count, insert_at);
			}
			insert_at += count;
			break;
		}
	}
	mToolbarStack->updateLayout();
}
void LLToolBar::saveButtonOrder()
{
	if (!mToolbarStack)
	{
		return;
	}
	LLSD order = LLSD::emptyArray();
	std::string last_key;
	const S32 n = mToolbarStack->getNumPanels();
	for (S32 i = 0; i < n; ++i)
	{
		LLLayoutPanel* panel = mToolbarStack->getPanelByIndex(i);
		if (!panel)
		{
			continue;
		}
		const std::string key = getPanelGroupKey(panel);
		if (key.empty() || key == last_key)
		{
			continue;
		}
		order.append(key);
		last_key = key;
	}
	gSavedSettings.setLLSD("ToolbarButtonOrder", order);
}
