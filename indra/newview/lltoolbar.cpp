/** 
 * @file lltoolbar.cpp
 * @author James Cook, Richard Nelson
 * @brief Large friendly buttons at bottom of screen.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * Modifications Copyright (C) 2026 - Allyn Viewer
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
#include "lloverlaybar.h"
#include "llbutton.h"
#include "llcontrol.h"
#include "llfloatertoolbarprefs.h"
#include "llflyoutbutton.h"
#include "llfocusmgr.h"
#include "llmenugl.h"
#include "llscrolllistitem.h"
#include "llui.h"
#include "llrender2dutils.h"
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
static const S32 TOOLBAR_BUTTON_SIZE = LLFloatingBarStyle::ICON_BUTTON_SIZE;
static const S32 TOOLBAR_MIN_BUTTON_SIZE = 20;
static const S32 TOOLBAR_ROW_GAP = 4;
static const S32 TOOLBAR_ARROW_WIDTH = 20;
static const S32 TOOLBAR_BADGE_SIZE = 14;
static const S32 TOOLBAR_PILL_LEFT_PAD = 10;
static const S32 TOOLBAR_ADD_SIZE = 44;
static const S32 TOOLBAR_ADD_GAP = 6;
static const S32 TOOLBAR_ADD_OVERHANG = 4;
static const S32 TOOLBAR_ADD_RISE = (TOOLBAR_ADD_SIZE - LLFloatingBarStyle::BOTTOM_BAR_HEIGHT) / 2;
static const S32 TOOLBAR_TOP_GAP = 3;
static const F32 TOOLBAR_SLIDE_SECONDS = 0.18f;
static const S32 TOOLBAR_CHROME_WIDTH = TOOLBAR_PILL_LEFT_PAD + TOOLBAR_ADD_GAP + TOOLBAR_ADD_SIZE;
void show_floater(const std::string& floater_name);
void show_inv_floater(const LLSD& userdata, const std::string& field);
void show_web_floater(const std::string& type);
LLToolBar::LLToolBar()
:	LLLayoutPanel()
,	mToolbarStack(NULL)
,	mContextMenu(NULL)
,	mContextPanel(NULL)
,	mDragStartPanel(NULL)
,	mDragStartX(0)
,	mDragStartY(0)
,	mDragging(false)
,	mDragInsertIndex(-1)
,	mDragCaretVisible(false)
,	mOrderDirty(false)
,	mLayoutDirty(true)
,	mMenuRequested(false)
,	mAddBtn(NULL)
,	mPillBlocker(NULL)
,	mBarHeight(0)
,	mRowCount(1)
,	mCompanionReserve(0)
,	mCenterOverride(false)
,	mVisualLeft(0)
,	mFromLeft(0)
,	mTargetLeft(0)
,	mAppliedLeft(0)
,	mSlideStart(0.0)
,	mSliding(false)
,	mPlacementReady(false)
,	mWasDockLeft(false)
,	mWasCenter(false)
,	mLayoutPillRight(0)
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
	setMouseOpaque(FALSE);
	setBackgroundVisible(FALSE);
	if (mToolbarStack)
	{
		mToolbarStack->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	}
	buildSlots();
	mPillBlocker = new LLView(std::string("toolbar_pill"), LLRect(), TRUE);
	addChildInBack(mPillBlocker);
	mAddBtn = new LLButton(std::string("toolbar_add_btn"), LLRect(), LLStringUtil::null, boost::bind(&LLToolBar::onClickAddButton, this));
	mAddBtn->setLabel(LLStringExplicit(""));
	mAddBtn->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	mAddBtn->setTabStop(FALSE);
	LLFloatingBarStyle::styleRoundButton(mAddBtn);
	mAddBtn->setImageOverlay("floating_ui/plus.png", LLFontGL::HCENTER, LLColor4::white);
	mAddBtn->setToolTip(LLStringExplicit(getString("ToolbarButtons")));
	addChild(mAddBtn);
	mLayoutSlots.push_back(LLFloatingBarStyle::connectSetting("FloatingBarMargin", boost::bind(&LLToolBar::markLayoutDirty, this)));
	mLayoutSlots.push_back(LLFloatingBarStyle::connectSetting("FloatingBarIconSpacing", boost::bind(&LLToolBar::markLayoutDirty, this)));
	mLayoutSlots.push_back(LLFloatingBarStyle::connectSetting("FloatingBarBottomAlignment", boost::bind(&LLToolBar::markLayoutDirty, this)));
	mLayoutDirty = true;
	return TRUE;
}
LLToolBar::~LLToolBar()
{
	for (boost::signals2::connection& slot : mLayoutSlots)
	{
		slot.disconnect();
	}
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
	S32 button_x = 0;
	S32 button_y = 0;
	LLPanelMainInventory* active_inventory = LLPanelMainInventory::getActiveInventory();
	if (active_inventory && active_inventory->getVisible())
	{
		mInventoryAutoOpenTimer.stop();
	}
	else if (localPointToOtherView(x, y, &button_x, &button_y, inventory_btn) && inventory_btn->pointInView(button_x, button_y))
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
		show_floater("friends");
	}
	else if (selected_option.asString() == "local chat")
	{
		show_floater("chat history");
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
		show_floater("mute list");
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
static S32 scale_toolbar_dim(S32 value, S32 size)
{
	return (value * size + TOOLBAR_BUTTON_SIZE / 2) / TOOLBAR_BUTTON_SIZE;
}
void LLToolBar::buildSlots()
{
	mSlots.clear();
	if (!mToolbarStack)
	{
		return;
	}
	const S32 n = mToolbarStack->getNumPanels();
	mSlots.reserve(n);
	for (S32 i = 0; i < n; ++i)
	{
		LLLayoutPanel* panel = mToolbarStack->getPanelByIndex(i);
		if (!panel)
		{
			continue;
		}
		Slot slot;
		slot.panel = panel;
		slot.main = NULL;
		slot.action = NULL;
		slot.arrow = NULL;
		slot.badge = NULL;
		slot.natural = panel->getRect().getWidth();
		for (LLView* child : *panel->getChildList())
		{
			if (!dynamic_cast<LLButton*>(child) && !dynamic_cast<LLFlyoutButton*>(child))
			{
				continue;
			}
			if (!slot.main || child->getRect().getWidth() > slot.main->getRect().getWidth())
			{
				slot.main = child;
			}
		}
		for (LLView* child : *panel->getChildList())
		{
			if (child != slot.main && dynamic_cast<LLButton*>(child))
			{
				slot.badge = child;
				child->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
			}
		}
		if (slot.main)
		{
			slot.main->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
			if (LLFlyoutButton* flyout = dynamic_cast<LLFlyoutButton*>(slot.main))
			{
				std::string label;
				for (LLView* child : *flyout->getChildList())
				{
					LLButton* button = dynamic_cast<LLButton*>(child);
					if (!button)
					{
						continue;
					}
					if (button->getRect().mLeft == 0)
					{
						slot.action = button;
						label = button->getLabelUnselected();
					}
					else
					{
						slot.arrow = button;
					}
					button->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
					LLFloatingBarStyle::styleIconButton(button);
				}
				if (slot.action)
				{
					LLStringUtil::trim(label);
					std::string tip = slot.action->getToolTip();
					if (tip.empty())
					{
						tip = flyout->getToolTip();
					}
					if (slot.action->getToolTip().empty() && !tip.empty())
					{
						slot.action->setToolTip(LLStringExplicit(tip));
					}
					slot.action->setShowLabelInToolTip(TRUE);
					slot.action->setImageOverlay("icn_toolbar_im.tga", LLFontGL::HCENTER, LLColor4::white);
					slot.action->setImageOverlaySelectedColor(LLFloatingBarStyle::accentColor());
					if (slot.arrow)
					{
						if (!label.empty())
						{
							slot.arrow->setLabelUnselected(LLStringExplicit(label));
							slot.arrow->setLabelSelected(LLStringExplicit(slot.action->getLabelSelected().empty() ? label : slot.action->getLabelSelected()));
						}
						if (!tip.empty())
						{
							slot.arrow->setToolTip(LLStringExplicit(tip));
						}
						slot.arrow->setShowLabelInToolTip(TRUE);
					}
				}
			}
			else if (LLButton* button = dynamic_cast<LLButton*>(slot.main))
			{
				LLFloatingBarStyle::labelToToolTip(button);
				LLFloatingBarStyle::styleIconButton(button);
			}
		}
		if (LLControlVariable* control = panel->getMakeVisibleControlVariable())
		{
			mLayoutSlots.push_back(control->getSignal()->connect(boost::bind(&LLToolBar::markLayoutDirty, this)));
		}
		mSlots.push_back(slot);
	}
}
void LLToolBar::markLayoutDirty()
{
	mLayoutDirty = true;
}
S32 LLToolBar::getBarHeight() const
{
	if (mBarHeight > 0)
	{
		return mBarHeight;
	}
	return llmax(LLFloatingBarStyle::margin(), TOOLBAR_ADD_RISE) + LLFloatingBarStyle::BOTTOM_BAR_HEIGHT + TOOLBAR_TOP_GAP;
}
void LLToolBar::onClickAddButton()
{
	show_floater("floater_toolbar_prefs.xml");
}
void LLToolBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (width == getRect().getWidth() && height == getRect().getHeight())
	{
		return;
	}
	LLLayoutPanel::reshape(width, height, called_from_parent);
	mLayoutDirty = true;
}
void LLToolBar::draw()
{
	if (mLayoutDirty)
	{
		layoutToolbar();
	}
	else if (mSliding)
	{
		tickSlide();
	}
	mPlacementReady = true;
	mStyle.drawPill(mPillRect, LLFloatingBarStyle::barColor());
	mStyle.drawDisc(mAddRect, LLFloatingBarStyle::accentColor());
	LLLayoutPanel::draw();
	if (mDragging && mDragCaretVisible)
	{
		gl_rect_2d(mDragCaretRect, LLFloatingBarStyle::accentColor(), TRUE);
	}
}
S32 LLToolBar::getPreferredWidth() const
{
	return getContentWidth(TOOLBAR_BUTTON_SIZE) + TOOLBAR_CHROME_WIDTH;
}
void LLToolBar::setCompanionReserve(S32 width)
{
	if (width < 0)
	{
		width = 0;
	}
	if (mCompanionReserve == width)
	{
		return;
	}
	mCompanionReserve = width;
	mLayoutDirty = true;
}
void LLToolBar::setCenterOverride(bool center)
{
	if (mCenterOverride == center)
	{
		return;
	}
	mCenterOverride = center;
	mLayoutDirty = true;
}
void LLToolBar::layoutNow()
{
	layoutToolbar();
}
void LLToolBar::retargetLeft(S32 left, bool animate)
{
	if (!mPlacementReady)
	{
		mFromLeft = mVisualLeft = mTargetLeft = left;
		mSliding = false;
		return;
	}
	if (!animate)
	{
		if (mSliding)
		{
			mTargetLeft = left;
			if (left == mVisualLeft)
			{
				mFromLeft = left;
				mSliding = false;
			}
			return;
		}
		mFromLeft = mVisualLeft = mTargetLeft = left;
		return;
	}
	if (left == mTargetLeft)
	{
		return;
	}
	mFromLeft = mVisualLeft;
	mTargetLeft = left;
	mSlideStart = LLFrameTimer::getTotalSeconds();
	mSliding = mFromLeft != left;
}
S32 LLToolBar::sampleLeft()
{
	if (!mSliding)
	{
		return mTargetLeft;
	}
	F32 t = (F32)((LLFrameTimer::getTotalSeconds() - mSlideStart) / TOOLBAR_SLIDE_SECONDS);
	if (t >= 1.f)
	{
		mSliding = false;
		mVisualLeft = mTargetLeft;
		return mTargetLeft;
	}
	t = t * t * (3.f - 2.f * t);
	mVisualLeft = mFromLeft + (S32)ll_round((mTargetLeft - mFromLeft) * t);
	return mVisualLeft;
}
void LLToolBar::tickSlide()
{
	const S32 left = sampleLeft();
	const S32 delta = left - mAppliedLeft;
	if (!delta)
	{
		return;
	}
	if (mToolbarStack)
	{
		LLRect stack_rect = mToolbarStack->getRect();
		stack_rect.translate(delta, 0);
		mToolbarStack->setRect(stack_rect);
	}
	mAddRect.translate(delta, 0);
	mPillRect.translate(delta, 0);
	if (mAddBtn)
	{
		mAddBtn->setRect(mAddRect);
	}
	if (mPillBlocker)
	{
		mPillBlocker->setRect(mPillRect);
	}
	mAppliedLeft = left;
}
S32 LLToolBar::getContentWidth(S32 size) const
{
	const S32 spacing = scale_toolbar_dim(LLFloatingBarStyle::iconSpacing(), size);
	const S32 arrow_width = scale_toolbar_dim(TOOLBAR_ARROW_WIDTH, size);
	S32 width = 0;
	for (const Slot& slot : mSlots)
	{
		if (!slot.panel->getVisible())
		{
			continue;
		}
		if (slot.main)
		{
			width += size + spacing + (slot.arrow ? arrow_width : 0);
		}
		else
		{
			width += slot.natural;
		}
	}
	return width;
}
void LLToolBar::layoutToolbar()
{
	mLayoutDirty = false;
	mRowCount = 1;
	if (!mToolbarStack || !mAddBtn || !mPillBlocker)
	{
		return;
	}
	const S32 margin = LLFloatingBarStyle::margin();
	const S32 width = getRect().getWidth();
	S32 available = width - 2 * margin - TOOLBAR_CHROME_WIDTH - mCompanionReserve;
	if (available < TOOLBAR_MIN_BUTTON_SIZE)
	{
		available = TOOLBAR_MIN_BUTTON_SIZE;
	}
	S32 size = TOOLBAR_BUTTON_SIZE;
	while (size > TOOLBAR_MIN_BUTTON_SIZE && getContentWidth(size) > available)
	{
		--size;
	}
	const S32 spacing = scale_toolbar_dim(LLFloatingBarStyle::iconSpacing(), size);
	const S32 arrow_width = scale_toolbar_dim(TOOLBAR_ARROW_WIDTH, size);
	const S32 pad = spacing / 2;
	std::vector<std::vector<S32>> rows(1);
	S32 row_width = 0;
	S32 content = 0;
	for (S32 i = 0; i < (S32)mSlots.size(); ++i)
	{
		const Slot& slot = mSlots[i];
		if (!slot.panel->getVisible())
		{
			continue;
		}
		const S32 slot_width = slot.main ? size + spacing + (slot.arrow ? arrow_width : 0) : slot.natural;
		if (row_width > 0 && row_width + slot_width > available)
		{
			content = llmax(content, row_width);
			rows.push_back(std::vector<S32>());
			row_width = 0;
		}
		rows.back().push_back(i);
		row_width += slot_width;
	}
	content = llmax(content, row_width);
	const S32 row_count = rows.back().empty() ? 1 : (S32)rows.size();
	mRowCount = row_count;
	const S32 rows_h = row_count * size + (row_count - 1) * TOOLBAR_ROW_GAP;
	const S32 pad_y = (LLFloatingBarStyle::BOTTOM_BAR_HEIGHT - TOOLBAR_BUTTON_SIZE) / 2;
	const S32 pill_h = llmax(LLFloatingBarStyle::BOTTOM_BAR_HEIGHT, rows_h + pad_y * 2);
	const S32 bottom = llmax(margin, (TOOLBAR_ADD_SIZE - pill_h) / 2);
	const S32 add_y = bottom + (pill_h - TOOLBAR_ADD_SIZE) / 2;
	mBarHeight = llmax(bottom + pill_h, add_y + TOOLBAR_ADD_SIZE) + TOOLBAR_TOP_GAP;
	if (getRect().getHeight() != mBarHeight)
	{
		reshape(width, mBarHeight);
		mLayoutDirty = false;
	}
	for (S32 i = 0; i < (S32)mSlots.size(); ++i)
	{
		const Slot& slot = mSlots[i];
		const S32 slot_width = slot.panel->getVisible()
			? (slot.main ? size + spacing + (slot.arrow ? arrow_width : 0) : slot.natural)
			: 0;
		const S32 panel_width = llmax(slot_width, 1);
		if (slot.panel->getRect().getWidth() != panel_width || slot.panel->getRect().getHeight() != size)
		{
			slot.panel->reshape(panel_width, size);
		}
	}
	const S32 group = TOOLBAR_CHROME_WIDTH + content;
	S32 target_left = (width - group) / 2;
	if (mCompanionReserve > 0)
	{
		target_left = margin;
	}
	else if (!mCenterOverride)
	{
		switch (LLFloatingBarStyle::bottomAlignment())
		{
		case LLFloatingBarStyle::ALIGN_LEFT:
			target_left = margin;
			break;
		case LLFloatingBarStyle::ALIGN_RIGHT:
			target_left = width - margin - group;
			break;
		default:
			break;
		}
	}
	target_left = llmax(margin, target_left);
	mLayoutPillRight = target_left + TOOLBAR_PILL_LEFT_PAD + content + TOOLBAR_ADD_GAP + TOOLBAR_ADD_SIZE - TOOLBAR_ADD_OVERHANG;
	const bool dock_left = mCompanionReserve > 0;
	const bool animate = mPlacementReady && (dock_left != mWasDockLeft || mCenterOverride != mWasCenter);
	mWasDockLeft = dock_left;
	mWasCenter = mCenterOverride;
	retargetLeft(target_left, animate);
	const S32 left = sampleLeft();
	const S32 stack_left = left + TOOLBAR_PILL_LEFT_PAD;
	const S32 stack_bottom = bottom + (pill_h - rows_h) / 2;
	mToolbarStack->setOrigin(stack_left, stack_bottom);
	mToolbarStack->reshape(llmax(content, 1), rows_h);
	mToolbarStack->updateLayout();
	S32 y = 0;
	for (const std::vector<S32>& row : rows)
	{
		S32 x = 0;
		for (S32 index : row)
		{
			const Slot& slot = mSlots[index];
			const S32 slot_width = slot.main ? size + spacing + (slot.arrow ? arrow_width : 0) : slot.natural;
			slot.panel->setRect(LLRect(x, y + size, x + slot_width, y));
			if (slot.main)
			{
				const S32 main_width = size + (slot.arrow ? arrow_width : 0);
				slot.main->setShape(LLRect(pad, size, pad + main_width, 0));
				if (slot.action && slot.arrow)
				{
					slot.action->setShape(LLRect(0, size, size, 0));
					slot.arrow->setShape(LLRect(size, size, main_width, 0));
					slot.arrow->setImageOverlay("floating_ui/chevron_up.png", LLFontGL::HCENTER, LLColor4::white);
					slot.arrow->setImageOverlaySelectedColor(LLFloatingBarStyle::accentColor());
				}
				if (slot.badge)
				{
					const S32 badge = llmax(6, scale_toolbar_dim(TOOLBAR_BADGE_SIZE, size));
					slot.badge->setShape(LLRect(pad + size - badge, badge, pad + size, 0));
				}
			}
			x += slot_width;
		}
		y += size + TOOLBAR_ROW_GAP;
	}
	mAddRect.setOriginAndSize(stack_left + content + TOOLBAR_ADD_GAP, add_y, TOOLBAR_ADD_SIZE, TOOLBAR_ADD_SIZE);
	mPillRect.set(left, bottom + pill_h, mAddRect.mRight - TOOLBAR_ADD_OVERHANG, bottom);
	mAddBtn->setShape(mAddRect);
	mPillBlocker->setShape(mPillRect);
	mAppliedLeft = left;
	if (gOverlayBar)
	{
		gOverlayBar->requestLayout();
	}
}
void LLToolBar::hookToolbarButtons(LLView* view)
{
	if (!view)
	{
		return;
	}
	if (LLButton* btn = dynamic_cast<LLButton*>(view))
	{
		btn->setHandleRightMouse(true);
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
	mContextPanel = ctrl ? findLayoutPanelFromChild(ctrl) : NULL;
	S32 local_x = x;
	S32 local_y = y;
	if (ctrl)
	{
		ctrl->localPointToOtherView(x, y, &local_x, &local_y, this);
	}
	showToolbarContextMenu(local_x, local_y);
}
void LLToolBar::onRemoveContextButton(void*)
{
	if (gToolBar)
	{
		gToolBar->removeContextButton();
	}
}
void LLToolBar::removeContextButton()
{
	LLLayoutPanel* panel = mContextPanel;
	mContextPanel = NULL;
	if (!panel)
	{
		return;
	}
	if (LLControlVariable* vis = panel->getMakeVisibleControlVariable())
	{
		vis->setValue(LLSD(false));
	}
}
void LLToolBar::showToolbarContextMenu(S32 x, S32 y)
{
	if (mContextMenu)
	{
		mContextMenu->die();
		mContextMenu = NULL;
	}
	LLControlVariable* vis = mContextPanel ? mContextPanel->getMakeVisibleControlVariable() : NULL;
	if (!vis)
	{
		mMenuRequested = true;
		return;
	}
	mContextMenu = new LLMenuGL("toolbar_context");
	mContextMenu->setCanTearOff(FALSE);
	mContextMenu->addChild(new LLMenuItemCallGL(getString("RemoveToolbarButton"), onRemoveContextButton));
	mContextMenu->updateParent(LLMenuGL::sMenuContainer);
	gFocusMgr.setMouseCapture(NULL);
	mMenuRequested = true;
	LLMenuGL::showPopup(this, mContextMenu, x, y);
}
BOOL LLToolBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	mMenuRequested = false;
	mContextPanel = NULL;
	if (!childrenHandleRightMouseDown(x, y, mask))
	{
		return FALSE;
	}
	if (!mMenuRequested)
	{
		mContextPanel = findPanelAt(x, y);
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
		commitButtonDrag();
		mDragging = false;
		mDragCaretVisible = false;
		mDragStartPanel = NULL;
		mDragInsertIndex = -1;
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
	mDragCaretVisible = false;
	mDragStartPanel = NULL;
	mDragInsertIndex = -1;
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
	mDragCaretVisible = false;
	mDragInsertIndex = -1;
	if (!mDragging || !mDragStartPanel || !mToolbarStack)
	{
		return;
	}
	S32 sx = x;
	S32 sy = y;
	if (!localPointToOtherView(x, y, &sx, &sy, mToolbarStack))
	{
		return;
	}
	LLLayoutPanel* over = NULL;
	S32 best = S32_MAX;
	const S32 n = mToolbarStack->getNumPanels();
	for (S32 i = 0; i < n; ++i)
	{
		LLLayoutPanel* panel = mToolbarStack->getPanelByIndex(i);
		if (!panel || !panel->getVisible() || panel->getRect().getWidth() <= 1)
		{
			continue;
		}
		LLRect hit = panel->getRect();
		hit.stretch(6);
		if (!hit.pointInRect(sx, sy))
		{
			continue;
		}
		const S32 dx = sx - (panel->getRect().mLeft + panel->getRect().mRight) / 2;
		const S32 dy = sy - (panel->getRect().mBottom + panel->getRect().mTop) / 2;
		const S32 dist = dx * dx + dy * dy;
		if (dist < best)
		{
			best = dist;
			over = panel;
		}
	}
	if (!over)
	{
		return;
	}
	S32 dst_start = 0;
	S32 dst_count = 0;
	if (!getGroupRange(over, dst_start, dst_count))
	{
		return;
	}
	LLLayoutPanel* first_dst = mToolbarStack->getPanelByIndex(dst_start);
	LLLayoutPanel* last_dst = mToolbarStack->getPanelByIndex(dst_start + dst_count - 1);
	if (!first_dst || !last_dst)
	{
		return;
	}
	const S32 mid_x = (first_dst->getRect().mLeft + last_dst->getRect().mRight) / 2;
	const S32 insert_index = (sx < mid_x) ? dst_start : (dst_start + dst_count);
	LLLayoutPanel* edge = (insert_index == dst_start) ? first_dst : last_dst;
	const S32 ex = (insert_index == dst_start) ? edge->getRect().mLeft : edge->getRect().mRight;
	S32 caret_x = 0;
	S32 caret_bottom = 0;
	S32 caret_top = 0;
	S32 ignored = 0;
	mToolbarStack->localPointToOtherView(ex, edge->getRect().mBottom, &caret_x, &caret_bottom, this);
	mToolbarStack->localPointToOtherView(ex, edge->getRect().mTop, &ignored, &caret_top, this);
	mDragCaretRect.set(caret_x - 1, caret_top, caret_x + 2, caret_bottom);
	mDragInsertIndex = insert_index;
	mDragCaretVisible = true;
}
void LLToolBar::commitButtonDrag()
{
	if (!mDragCaretVisible || !mDragStartPanel || !mToolbarStack || mDragInsertIndex < 0)
	{
		return;
	}
	S32 src_start = 0;
	S32 src_count = 0;
	if (!getGroupRange(mDragStartPanel, src_start, src_count))
	{
		return;
	}
	if (mDragInsertIndex == src_start || (mDragInsertIndex > src_start && mDragInsertIndex <= src_start + src_count))
	{
		return;
	}
	mToolbarStack->movePanelRange(src_start, src_count, mDragInsertIndex);
	syncSlotsToStack();
	mLayoutDirty = true;
	saveButtonOrder();
}
void LLToolBar::syncSlotsToStack()
{
	if (!mToolbarStack)
	{
		return;
	}
	std::vector<Slot> ordered;
	std::vector<bool> used(mSlots.size(), false);
	ordered.reserve(mSlots.size());
	const S32 n = mToolbarStack->getNumPanels();
	for (S32 i = 0; i < n; ++i)
	{
		LLLayoutPanel* panel = mToolbarStack->getPanelByIndex(i);
		for (S32 s = 0; s < (S32)mSlots.size(); ++s)
		{
			if (!used[s] && mSlots[s].panel == panel)
			{
				ordered.push_back(mSlots[s]);
				used[s] = true;
				break;
			}
		}
	}
	for (S32 s = 0; s < (S32)mSlots.size(); ++s)
	{
		if (!used[s])
		{
			ordered.push_back(mSlots[s]);
		}
	}
	mSlots.swap(ordered);
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
