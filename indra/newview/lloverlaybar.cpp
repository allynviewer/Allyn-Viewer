/** 
 * @file lloverlaybar.cpp
 * @brief LLOverlayBar class implementation
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
#include "lloverlaybar.h"
#include "llaudioengine.h"
#include "floaterao.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llchatbar.h"
#include "llfloaterchatterbox.h"
#include "llmediaremotectrl.h"
#include "llpanelaudiovolume.h"
#include "llparcel.h"
#include "llviewertexturelist.h"
#include "llviewerjoystick.h"
#include "llviewermedia.h"
#include "llviewermenu.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "lluictrlfactory.h"
#include "llui.h"
#include "llrender2dutils.h"
#include "llvoiceclient.h"
#include "llvoavatarself.h"
#include "llvoiceremotectrl.h"
#include "llselectmgr.h"
#include "lltoolbar.h"
#include "wlfPanel_AdvSettings.h"
#include "llpanelnearbymedia.h"
#include "rlvhandler.h"
#include <boost/range/adaptor/reversed.hpp>
LLOverlayBar *gOverlayBar = NULL;
static const S32 OVERLAY_BUTTON_HEIGHT = 29;
static const S32 OVERLAY_ICON_BUTTON_WIDTH = 30;
static const S32 OVERLAY_ICON_PANEL_WIDTH = 34;
static const S32 OVERLAY_LIGHT_PANEL_WIDTH = 31;
static const S32 OVERLAY_LIGHT_BUTTON = 29;
static const S32 OVERLAY_PILL_PAD = 3;
static const S32 OVERLAY_PILL_CAP = 8;
static const S32 OVERLAY_MOUSELOOK_SIZE = 30;
static const F32 OVERLAY_SLIDE_SECONDS = 0.18f;
static S32 overlayRowStride(S32 button_h, S32 pad)
{
	return button_h + 2 * pad + LLFloatingBarStyle::BAR_GAP;
}
static S32 overlayOuterWidth(S32 content, S32 cap)
{
	if (content <= 0)
		return 0;
	return content + cap + cap - 2;
}
static bool overlayPanelShown(const LLView* panel)
{
	return panel && panel->getVisible() && panel->getRect().getWidth() > 0;
}
void* LLOverlayBar::createVoiceRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;
	self->mVoiceRemote = new LLVoiceRemoteCtrl(std::string("voice_remote"));
	return self->mVoiceRemote;
}
void* LLOverlayBar::createChatBar(void* userdata)
{
	gChatBar = new LLChatBar();
	return gChatBar;
}
LLOverlayBar::LLOverlayBar()
	:	LLLayoutPanel(),
		mMediaRemote(NULL),
		mVoiceRemote(NULL),
		mAOButton(NULL),
		mVolumeButton(NULL),
		mEnvButton(NULL),
		mOverlayStack(NULL),
		mStackLift(0),
		mStackNaturalHeight(0),
		mOverlayLayoutDirty(true),
		mSecondaryPillValid(false),
		mVoicePillValid(false),
		mWasBeside(false),
		mVoiceRaised(false),
		mSecondaryRaised(false),
		mToolbarRowOk(false),
		mToolbarRow(0),
		mIconSize(LLFloatingBarStyle::ICON_BUTTON_SIZE),
		mSlidePrimed(false),
		mStackSliding(false),
		mFromBottom(0),
		mToBottom(0),
		mVisualBottom(0),
		mSlideTop(0),
		mSlideStart(0.0),
		mAOFloaterWasOpen(false),
		mVolumeFloaterWasOpen(false),
		mEnvFloaterWasOpen(false),
		mVolumeMutedIcon(false),
		mVolumeIconReady(false),
		mMusicState(STOPPED),
		mOriginalIMLabel("")
{
	gOverlayBar = this;
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);
	mBuilt = false;
	LLCallbackMap::map_t factory_map;
	factory_map["voice_remote"] = LLCallbackMap(LLOverlayBar::createVoiceRemote, this);
	factory_map["chat_bar"] = LLCallbackMap(LLOverlayBar::createChatBar, this);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_overlaybar.xml", &factory_map);
}
bool LLOverlayBar::updateAdvSettingsPopup(const LLSD &data)
{
	if (data.asBoolean())
	{
		LLFloaterOverlayEnv::getInstance()->open();
	}
	else if (LLFloaterOverlayEnv::instanceExists() && LLFloaterOverlayEnv::getInstance()->getVisible())
	{
		LLFloaterOverlayEnv::getInstance()->close();
	}
	return true;
}
bool LLOverlayBar::updateVolumePopup(const LLSD &data)
{
	if (!LLFloaterOverlayVolume::instanceExists())
		return true;
	LLFloaterOverlayVolume* floater = LLFloaterOverlayVolume::getInstance();
	if (data.asBoolean())
	{
		if (!floater->getVisible())
			floater->open();
	}
	else if (floater->getVisible())
	{
		floater->close();
	}
	return true;
}
void LLOverlayBar::onClickAO(void*)
{
	if (!gOverlayBar)
		return;
	const bool opening = !LLFloaterAO::instanceVisible();
	LLFloaterAO::toggleInstance();
	if (opening)
	{
		if (LLFloaterAO* floater = LLFloaterAO::findInstance())
			placeFloaterAbove(floater, gOverlayBar->mAOButton);
	}
}
void LLOverlayBar::placeFloaterAbove(LLFloater* floater, LLView* anchor)
{
	if (!floater || !gFloaterView)
		return;
	bool placed = false;
	const std::string& control = floater->getRectControl();
	if (!control.empty() && gSavedSettings.controlExists(control))
	{
		const LLRect saved = gSavedSettings.getRect(control);
		if (saved.getWidth() > 0 && saved.getHeight() > 0)
		{
			floater->applyRectControl();
			placed = true;
		}
	}
	if (!placed && anchor)
	{
		const LLRect anchor_rect = anchor->calcScreenRect();
		floater->setOrigin(anchor_rect.mRight - floater->getRect().getWidth(), anchor_rect.mTop);
	}
	gFloaterView->adjustToFitScreen(floater, FALSE);
}
void LLOverlayBar::fitFloaterToPanel(LLFloater* floater, LLView* panel)
{
	if (!floater || !panel)
		return;
	const LLRect local = panel->getLocalBoundingRect();
	const S32 pad = LLFLOATER_CONTENT_PAD;
	const S32 panel_w = llmax(panel->getRect().getWidth(), local.getWidth());
	const S32 panel_h = llmax(panel->getRect().getHeight(), local.getHeight());
	panel->setOrigin(pad, pad);
	floater->reshape(panel_w + pad * 2, panel_h + LLFLOATER_HEADER_SIZE + pad + 4);
	floater->setResizeLimits(floater->getRect().getWidth(), floater->getRect().getHeight());
}
static void stashOverlayFloater(LLFloater* floater, bool& was_open)
{
	if (floater && floater->getVisible())
	{
		was_open = true;
		floater->setVisible(FALSE);
	}
}
void LLOverlayBar::stashOverlayFloaters(bool in_mouselook)
{
	if (in_mouselook)
	{
		stashOverlayFloater(LLFloaterAO::findInstance(), mAOFloaterWasOpen);
		if (LLFloaterOverlayVolume::instanceExists())
			stashOverlayFloater(LLFloaterOverlayVolume::getInstance(), mVolumeFloaterWasOpen);
		if (LLFloaterOverlayEnv::instanceExists())
			stashOverlayFloater(LLFloaterOverlayEnv::getInstance(), mEnvFloaterWasOpen);
		return;
	}
	if (mAOFloaterWasOpen)
	{
		if (LLFloaterAO* floater = LLFloaterAO::findInstance())
			floater->setVisible(TRUE);
		mAOFloaterWasOpen = false;
	}
	if (mVolumeFloaterWasOpen)
	{
		if (gSavedSettings.getBOOL("ShowVolumeSettingsPopup") && LLFloaterOverlayVolume::instanceExists())
			LLFloaterOverlayVolume::getInstance()->setVisible(TRUE);
		mVolumeFloaterWasOpen = false;
	}
	if (mEnvFloaterWasOpen)
	{
		if (gSavedSettings.getBOOL("wlfAdvSettingsPopup") && LLFloaterOverlayEnv::instanceExists())
			LLFloaterOverlayEnv::getInstance()->setVisible(TRUE);
		mEnvFloaterWasOpen = false;
	}
}
bool LLOverlayBar::updateChatVisible(const LLSD &data)
{
	mChatBar->getParent()->setVisible(data.asBoolean());
	return true;
}
bool LLOverlayBar::updateAORemoteVisible(const LLSD &data)
{
	mAORemoteContainer->setVisible(data.asBoolean());
	mAOLightContainer->setVisible(data.asBoolean());
	return true;
}
bool updateNearbyMediaFloater(const LLSD &data)
{
	LLFloaterNearbyMedia::updateClass();
	return true;
}
BOOL LLOverlayBar::postBuild()
{
	childSetAction("New IM",onClickIMReceived,this);
	childSetAction("Set Not Busy",onClickSetNotBusy,this);
	childSetAction("Mouselook",onClickMouselook,this);
	childSetAction("Stand Up",onClickStandUp,this);
	childSetAction("Cancel TP",onClickCancelTP,this);
 	childSetAction("Flycam",onClickFlycam,this);
	mCancelBtn = getChild<LLButton>("Cancel TP");
	setFocusRoot(TRUE);
	mBuilt = true;
	mUnreadCountStringPlural = getString("unread_count_string_plural");
	mChatbarAndButtons.connect(this,"chatbar_and_buttons");
	mNewIM.connect(this,"New IM");
	mNotBusy.connect(this,"Set Not Busy");
	mMouseLook.connect(this,"Mouselook");
	mStandUp.connect(this,"Stand Up");
	mFlyCam.connect(this,"Flycam");
	mChatBar.connect(this,"chat_bar");
	mVoiceRemoteContainer.connect(this,"voice_remote_container");
	mStateManagementContainer.connect(this,"state_management_buttons_container");
	mAORemoteContainer.connect(this,"ao_remote_container");
	mAOLightContainer.connect(this,"ao_light_container");
	mAdvSettingsContainer.connect(this,"AdvSettings_container");
	mMediaRemoteContainer.connect(this,"media_remote_container");
	mAOButton = getChild<LLButton>("overlay_ao_btn");
	mVolumeButton = getChild<LLButton>("overlay_volume_btn");
	mEnvButton = getChild<LLButton>("overlay_env_btn");
	childSetAction("overlay_ao_btn", onClickAO, this);
	LLFloaterOverlayVolume::getInstance();
	mMediaRemote = LLFloaterOverlayVolume::getInstance()->getRemote();
	updateAdvSettingsPopup(gSavedSettings.getBOOL("wlfAdvSettingsPopup"));
	updateVolumePopup(gSavedSettings.getBOOL("ShowVolumeSettingsPopup"));
	updateChatVisible(gSavedSettings.getBOOL("ChatVisible"));
	updateAORemoteVisible(gSavedSettings.getBOOL("EnableAORemote"));
	mOriginalIMLabel = mNewIM->getLabelSelected();
	if (LLView* parent = mNewIM->getParent())
	{
		if (parent != this)
		{
			parent->removeChild(mNewIM);
			addChild(mNewIM);
		}
	}
	LLFloatingBarStyle::styleIconButton(mNewIM);
	mNewIM->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	mOverlayStack = findChild<LLLayoutStack>("overlay_layout_panel");
	detachVoiceBar();
	styleOverlayButton(mAOButton, mAORemoteContainer, "floating_ui/ao.png", true);
	styleOverlayButton(mVolumeButton, mMediaRemoteContainer, "floating_ui/volume.png", true);
	styleOverlayButton(mEnvButton, mAdvSettingsContainer, "floating_ui/environment.png", true);
	styleOverlayButton(mMouseLook, NULL, "floating_ui/mouselook.png", true);
	mMarginSlot = LLFloatingBarStyle::connectSetting("FloatingBarMargin", boost::bind(&LLOverlayBar::requestLayout, this));
	layoutOverlayStack();
	if (mVoiceRemote)
		mVoiceRemote->layoutControls(mIconSize);
	layoutButtons();
	sendChildToFront(mMouseLook);
	gSavedSettings.getControl("wlfAdvSettingsPopup")->getSignal()->connect(boost::bind(&LLOverlayBar::updateAdvSettingsPopup,this,_2));
	gSavedSettings.getControl("ShowVolumeSettingsPopup")->getSignal()->connect(boost::bind(&LLOverlayBar::updateVolumePopup,this,_2));
	gSavedSettings.getControl("ChatVisible")->getSignal()->connect(boost::bind(&LLOverlayBar::updateChatVisible,this,_2));
	gSavedSettings.getControl("EnableAORemote")->getSignal()->connect(boost::bind(&LLOverlayBar::updateAORemoteVisible,this,_2));
	gSavedSettings.getControl("ShowNearbyMediaFloater")->getSignal()->connect(boost::bind(&updateNearbyMediaFloater,_2));
	mAORemoteContainer->setVisible(gSavedSettings.getBOOL("EnableAORemote"));
	mAOLightContainer->setVisible(gSavedSettings.getBOOL("EnableAORemote"));
	return TRUE;
}
LLOverlayBar::~LLOverlayBar()
{
	mMarginSlot.disconnect();
}
void LLOverlayBar::styleOverlayButton(LLButton* button, LLView* container, const std::string& icon, bool label_to_tip)
{
	if (!button)
		return;
	if (label_to_tip)
		LLFloatingBarStyle::labelToToolTip(button);
	else
		button->setLabel(LLStringExplicit(""));
	LLFloatingBarStyle::styleIconButton(button);
	button->setImageOverlay(icon, LLFontGL::HCENTER, LLColor4::white);
	button->setImageOverlaySelectedColor(LLFloatingBarStyle::accentColor());
	button->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	if (LLLayoutPanel* panel = dynamic_cast<LLLayoutPanel*>(container))
	{
		panel->setMinDim(OVERLAY_ICON_PANEL_WIDTH);
		panel->reshape(OVERLAY_ICON_PANEL_WIDTH, panel->getRect().getHeight());
		const S32 left = (OVERLAY_ICON_PANEL_WIDTH - OVERLAY_ICON_BUTTON_WIDTH) / 2;
		button->setShape(LLRect(left, OVERLAY_BUTTON_HEIGHT, left + OVERLAY_ICON_BUTTON_WIDTH, 0));
	}
}
static void apply_stack_rect(LLView* stack, const LLRect& rect)
{
	if (stack && stack->getRect() != rect)
	{
		stack->setShape(rect);
	}
}
void LLOverlayBar::liftMainPanel(S32 lift)
{
	if (!mOverlayStack)
	{
		return;
	}
	if (lift < 0)
	{
		lift = 0;
	}
	for (S32 i = 0; i < mOverlayStack->getNumPanels(); ++i)
	{
		LLLayoutPanel* panel = mOverlayStack->getPanelByIndex(i);
		if (!panel || panel->getName() != "main_panel")
		{
			continue;
		}
		LLRect main_rect = panel->getRect();
		if (main_rect.mBottom != lift)
		{
			main_rect.translate(0, lift - main_rect.mBottom);
			panel->setRect(main_rect);
		}
		break;
	}
}
void LLOverlayBar::applyStackMotion(const LLRect& rect, bool reflow)
{
	if (!mOverlayStack)
	{
		return;
	}
	if (reflow)
	{
		if (mOverlayStack->getRect() != rect)
		{
			mOverlayStack->setShape(rect);
		}
		if (mOverlayStack->needsLayout())
		{
			mOverlayStack->updateLayout();
		}
	}
	else if (mOverlayStack->getRect() != rect)
	{
		mOverlayStack->setRect(rect);
	}
	S32 lift = mStackLift - rect.mBottom;
	if (lift < 0)
	{
		lift = 0;
	}
	liftMainPanel(lift);
	layoutVoiceBar(rect.mBottom);
	clipChatToVoice();
	layoutMouselookButton();
	cacheSecondaryPill();
	mVisualBottom = rect.mBottom;
}
void LLOverlayBar::presentStack(const LLRect& desired, bool animate_bottom)
{
	if (!mOverlayStack)
	{
		return;
	}
	const S32 bottom = desired.mBottom;
	if (!mSlidePrimed || !animate_bottom)
	{
		if (mStackSliding)
		{
			mToBottom = bottom;
			mSlideTop = desired.mTop;
			if (bottom == mVisualBottom)
			{
				mStackSliding = false;
				LLRect settled = desired;
				if (mOverlayStack->getRect() == settled)
				{
					mOverlayStack->reshape(settled.getWidth(), settled.getHeight());
				}
				applyStackMotion(settled, true);
				return;
			}
			LLRect shown = desired;
			shown.mBottom = mVisualBottom;
			shown.mTop = mSlideTop;
			applyStackMotion(shown, false);
			return;
		}
		mStackSliding = false;
		mFromBottom = bottom;
		mToBottom = bottom;
		mSlideTop = desired.mTop;
		applyStackMotion(desired, true);
		return;
	}
	if (mStackSliding && bottom == mToBottom)
	{
		mSlideTop = desired.mTop;
		LLRect shown = desired;
		shown.mBottom = mVisualBottom;
		shown.mTop = mSlideTop;
		applyStackMotion(shown, false);
		return;
	}
	const S32 current = mOverlayStack->getRect().mBottom;
	mFromBottom = current;
	mVisualBottom = current;
	mToBottom = bottom;
	mSlideTop = desired.mTop;
	mSlideStart = LLFrameTimer::getTotalSeconds();
	mStackSliding = current != bottom;
	if (!mStackSliding)
	{
		applyStackMotion(desired, true);
		return;
	}
	LLRect shown = desired;
	shown.mBottom = current;
	shown.mTop = mSlideTop;
	applyStackMotion(shown, false);
}
void LLOverlayBar::tickStackSlide()
{
	if (!mStackSliding || !mOverlayStack)
	{
		return;
	}
	F32 t = (F32)((LLFrameTimer::getTotalSeconds() - mSlideStart) / OVERLAY_SLIDE_SECONDS);
	const bool done = t >= 1.f;
	if (done)
	{
		t = 1.f;
	}
	else
	{
		t = t * t * (3.f - 2.f * t);
	}
	const S32 bottom = done ? mToBottom : mFromBottom + (S32)ll_round((F32)(mToBottom - mFromBottom) * t);
	if (!done && bottom == mVisualBottom)
	{
		return;
	}
	LLRect rect = mOverlayStack->getRect();
	rect.mBottom = bottom;
	rect.mTop = mSlideTop;
	if (done)
	{
		mStackSliding = false;
		if (mOverlayStack->getRect() == rect)
		{
			mOverlayStack->reshape(rect.getWidth(), rect.getHeight());
		}
		applyStackMotion(rect, true);
		return;
	}
	applyStackMotion(rect, false);
}
void LLOverlayBar::detachVoiceBar()
{
	if (mVoiceRemote && mVoiceRemote->getParent() && mVoiceRemote->getParent() != this)
	{
		mVoiceRemote->getParent()->removeChild(mVoiceRemote);
		addChild(mVoiceRemote);
		sendChildToFront(mVoiceRemote);
	}
	if (mVoiceRemote)
		mVoiceRemote->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	if (mVoiceRemoteContainer)
	{
		LLPanel* raw = mVoiceRemoteContainer;
		if (LLLayoutPanel* panel = dynamic_cast<LLLayoutPanel*>(raw))
		{
			panel->setMinDim(0);
			panel->setExpandedMinDim(0);
		}
		mVoiceRemoteContainer->setVisible(FALSE);
	}
}
void LLOverlayBar::applyToolbarReserve(S32 reserve)
{
	if (!gToolBar)
		return;
	gToolBar->setDockLeft(true);
	gToolBar->setCenterOverride(false);
	gToolBar->setCompanionReserve(reserve);
	if (gToolBar->isLayoutDirty())
		gToolBar->layoutNow();
}
bool LLOverlayBar::measureSecondary(S32& left, S32& right) const
{
	LLView* panels[4] = { mAOLightContainer, mAORemoteContainer, mMediaRemoteContainer, mAdvSettingsContainer };
	bool any = false;
	left = S32_MAX;
	right = 0;
	if (!mOverlayStack)
		return false;
	const S32 origin = mOverlayStack->getRect().mLeft;
	for (LLView* panel : panels)
	{
		if (!overlayPanelShown(panel))
			continue;
		left = llmin(left, origin + panel->getRect().mLeft);
		right = llmax(right, origin + panel->getRect().mRight);
		any = true;
	}
	return any && right > left;
}
void LLOverlayBar::layoutVoiceBar(S32 secondary_bottom)
{
	mVoicePillValid = false;
	if (!mVoiceRemote || !mVoiceRemote->getVisible() || mVoiceRemote->getRect().getWidth() <= 0)
		return;
	const S32 gap = LLFloatingBarStyle::BAR_GAP;
	const S32 margin = LLFloatingBarStyle::margin();
	const S32 width = mVoiceRemote->getRect().getWidth();
	const S32 height = llmax(mVoiceRemote->getRect().getHeight(), buttonHeight());
	S32 secondary_left = 0;
	S32 secondary_right = 0;
	const bool have_secondary = measureSecondary(secondary_left, secondary_right);
	S32 left = 0;
	S32 bottom = secondary_bottom;
	if (have_secondary && !mVoiceRaised)
	{
		const S32 voice_right = secondary_left - gap - (pillCap() * 2 - 2);
		left = voice_right - width;
		bottom = secondary_bottom;
	}
	else if (have_secondary)
	{
		left = secondary_right - width;
		bottom = secondary_bottom + overlayRowStride(buttonHeight(), pillPad());
	}
	else
	{
		const S32 right_edge = getRect().getWidth() - margin - pillCap();
		left = right_edge - width;
		bottom = mVoiceRaised ? mStackLift + overlayRowStride(buttonHeight(), pillPad()) : (mToolbarRowOk ? mToolbarRow : mStackLift);
	}
	if (left < 0)
		left = 0;
	LLRect placed = mVoiceRemote->getRect();
	placed.setOriginAndSize(left, bottom, width, height);
	if (mVoiceRemote->getRect() != placed)
		mVoiceRemote->setRect(placed);
	sendChildToFront(mVoiceRemote);
	mVoicePill.set(left - pillCap(), bottom + height + pillPad(), left + width + pillCap() - 2, bottom - pillPad());
	mVoicePillValid = true;
}
void LLOverlayBar::clipChatToVoice()
{
	if (!mVoiceRaised || !mVoiceRemote || !mVoiceRemote->getVisible() || !mOverlayStack)
		return;
	LLLayoutPanel* main = NULL;
	for (S32 i = 0; i < mOverlayStack->getNumPanels(); ++i)
	{
		LLLayoutPanel* panel = mOverlayStack->getPanelByIndex(i);
		if (panel && panel->getName() == "main_panel")
		{
			main = panel;
			break;
		}
	}
	if (!main)
		return;
	const S32 limit = mVoiceRemote->getRect().mLeft - LLFloatingBarStyle::BAR_GAP - mOverlayStack->getRect().mLeft;
	const S32 width = limit - main->getRect().mLeft;
	if (width < 80 || width >= main->getRect().getWidth())
		return;
	main->reshape(width, main->getRect().getHeight());
}
struct OverlayBarFit
{
	bool voice_raised;
	bool secondary_raised;
	S32 reserve;
};
static OverlayBarFit fitOverlayBars(S32 span, S32 main_outer, S32 secondary_outer, S32 voice_outer, bool toolbar_visible)
{
	const S32 gap = LLFloatingBarStyle::BAR_GAP;
	S32 inline_right = 0;
	if (voice_outer > 0)
		inline_right += voice_outer;
	if (secondary_outer > 0)
	{
		if (inline_right > 0)
			inline_right += gap;
		inline_right += secondary_outer;
	}
	auto fitsBeside = [&](S32 left_width, S32 right_width) -> bool
	{
		if (right_width <= 0)
			return true;
		if (left_width <= 0)
			return right_width <= span;
		return left_width + gap + right_width <= span;
	};
	OverlayBarFit fit;
	fit.voice_raised = false;
	fit.secondary_raised = false;
	if (secondary_outer > 0 && voice_outer > 0)
	{
		if (!fitsBeside(main_outer, inline_right))
		{
			fit.voice_raised = true;
			if (!fitsBeside(main_outer, secondary_outer))
				fit.secondary_raised = true;
		}
	}
	else if (secondary_outer > 0)
	{
		if (!fitsBeside(main_outer, secondary_outer))
			fit.secondary_raised = true;
	}
	else if (voice_outer > 0)
	{
		if (!fitsBeside(main_outer, voice_outer))
			fit.voice_raised = true;
	}
	fit.reserve = 0;
	if (toolbar_visible)
	{
		if (secondary_outer > 0 && !fit.secondary_raised)
			fit.reserve = ((!fit.voice_raised && voice_outer > 0) ? inline_right : secondary_outer) + gap;
		else if (!fit.voice_raised && voice_outer > 0)
			fit.reserve = voice_outer + gap;
	}
	return fit;
}
S32 LLOverlayBar::buttonHeight() const
{
	return LLFloatingBarStyle::scaleIcon(OVERLAY_BUTTON_HEIGHT, mIconSize);
}
S32 LLOverlayBar::pillPad() const
{
	return LLFloatingBarStyle::scaleIcon(OVERLAY_PILL_PAD, mIconSize);
}
S32 LLOverlayBar::pillCap() const
{
	return LLFloatingBarStyle::scaleIcon(OVERLAY_PILL_CAP, mIconSize);
}
S32 LLOverlayBar::secondaryContentWidth(S32 size) const
{
	const S32 panel_w = LLFloatingBarStyle::scaleIcon(OVERLAY_ICON_PANEL_WIDTH, size);
	const S32 light_w = LLFloatingBarStyle::scaleIcon(OVERLAY_LIGHT_PANEL_WIDTH, size);
	S32 width = 0;
	S32 count = 0;
	auto add = [&](const LLView* panel, S32 panel_width)
	{
		if (!overlayPanelShown(panel))
			return;
		width += panel_width;
		++count;
	};
	add(mAOLightContainer, light_w);
	add(mAORemoteContainer, panel_w);
	add(mMediaRemoteContainer, panel_w);
	add(mAdvSettingsContainer, panel_w);
	if (count > 1 && mOverlayStack)
		width += mOverlayStack->getPanelSpacing() * (count - 1);
	return width;
}
S32 LLOverlayBar::iconSizeForLayout() const
{
	const S32 full = LLFloatingBarStyle::ICON_BUTTON_SIZE;
	const S32 minimum = LLFloatingBarStyle::ICON_BUTTON_MIN;
	const bool toolbar_visible = gToolBar && gToolBar->getVisible() && gToolBar->getRect().getWidth() > 0;
	const S32 width = toolbar_visible ? gToolBar->getRect().getWidth() : getRect().getWidth();
	if (width <= 0)
		return full;
	const S32 margin = LLFloatingBarStyle::margin();
	const S32 span = llmax(0, width - 2 * margin);
	const S32 main_full = toolbar_visible ? gToolBar->getPreferredWidth() : 0;
	const bool voice_on = mVoiceRemote && !gAgentCamera.cameraMouselook() && LLVoiceClient::getInstance()->voiceEnabled();
	auto outerVoice = [&](S32 size) -> S32
	{
		if (!voice_on || !mVoiceRemote)
			return 0;
		return overlayOuterWidth(mVoiceRemote->contentWidthForSize(size), LLFloatingBarStyle::scaleIcon(OVERLAY_PILL_CAP, size));
	};
	if (!toolbar_visible)
	{
		for (S32 size = full; size >= minimum; --size)
		{
			const S32 secondary_outer = overlayOuterWidth(secondaryContentWidth(size), LLFloatingBarStyle::scaleIcon(OVERLAY_PILL_CAP, size));
			const S32 voice_outer = outerVoice(size);
			const OverlayBarFit fit = fitOverlayBars(span, 0, secondary_outer, voice_outer, false);
			S32 inline_right = secondary_outer;
			if (voice_outer > 0)
			{
				if (inline_right > 0)
					inline_right += LLFloatingBarStyle::BAR_GAP;
				inline_right += voice_outer;
			}
			const S32 widest = (secondary_outer > 0 && voice_outer > 0 && !fit.voice_raised && !fit.secondary_raised) ? inline_right : llmax(secondary_outer, voice_outer);
			if (widest <= span || size == minimum)
				return size;
		}
		return minimum;
	}
	S32 size = full;
	S32 previous = -1;
	const S32 chrome = gToolBar->chromeWidth();
	for (S32 pass = 0; pass < 8; ++pass)
	{
		const S32 secondary_outer = overlayOuterWidth(secondaryContentWidth(size), LLFloatingBarStyle::scaleIcon(OVERLAY_PILL_CAP, size));
		const S32 voice_outer = outerVoice(size);
		const S32 reserve = fitOverlayBars(span, main_full, secondary_outer, voice_outer, true).reserve;
		const S32 available = width - 2 * margin - chrome - reserve;
		const S32 fitted = gToolBar->buttonSizeForWidth(available);
		if (fitted == size || fitted == previous)
			return llmin(fitted, size);
		previous = size;
		size = fitted;
	}
	return llclamp(size, minimum, full);
}
void LLOverlayBar::applyIconSize(S32 size)
{
	if (size < LLFloatingBarStyle::ICON_BUTTON_MIN)
		size = LLFloatingBarStyle::ICON_BUTTON_MIN;
	if (size > LLFloatingBarStyle::ICON_BUTTON_SIZE)
		size = LLFloatingBarStyle::ICON_BUTTON_SIZE;
	mIconSize = size;
	const S32 button_w = LLFloatingBarStyle::scaleIcon(OVERLAY_ICON_BUTTON_WIDTH, size);
	const S32 button_h = LLFloatingBarStyle::scaleIcon(OVERLAY_BUTTON_HEIGHT, size);
	const S32 panel_w = LLFloatingBarStyle::scaleIcon(OVERLAY_ICON_PANEL_WIDTH, size);
	const S32 left = (panel_w - button_w) / 2;
	auto place = [&](LLButton* button, LLPanel* container)
	{
		if (LLLayoutPanel* panel = dynamic_cast<LLLayoutPanel*>(container))
		{
			if (panel->getMinDim() != panel_w)
				panel->setMinDim(panel_w);
			if (panel->getExpandedMinDim() != panel_w)
				panel->setExpandedMinDim(panel_w);
			if (panel->getRect().getWidth() != panel_w)
				panel->reshape(panel_w, llmax(panel->getRect().getHeight(), 1));
		}
		if (button)
		{
			const LLRect shape(left, button_h, left + button_w, 0);
			if (button->getRect() != shape)
				button->setShape(shape);
		}
	};
	place(mAOButton, mAORemoteContainer);
	place(mVolumeButton, mMediaRemoteContainer);
	place(mEnvButton, mAdvSettingsContainer);
	if (LLPanel* light_panel = mAOLightContainer)
	{
		const S32 light_w = LLFloatingBarStyle::scaleIcon(OVERLAY_LIGHT_PANEL_WIDTH, size);
		const S32 light_btn = LLFloatingBarStyle::scaleIcon(OVERLAY_LIGHT_BUTTON, size);
		if (LLLayoutPanel* panel = dynamic_cast<LLLayoutPanel*>(light_panel))
		{
			if (panel->getMinDim() != light_w)
				panel->setMinDim(light_w);
			if (panel->getExpandedMinDim() != light_w)
				panel->setExpandedMinDim(light_w);
			if (panel->getRect().getWidth() != light_w)
				panel->reshape(light_w, llmax(panel->getRect().getHeight(), 1));
		}
		if (LLButton* light = light_panel->getChild<LLButton>("overlay_ao_check", TRUE, FALSE))
		{
			const LLRect shape(0, light_btn, light_btn, 0);
			if (light->getRect() != shape)
				light->setShape(shape);
		}
	}
}
void LLOverlayBar::layoutOverlayStack()
{
	if (!mOverlayStack)
		return;
	applyIconSize(iconSizeForLayout());
	const S32 margin = LLFloatingBarStyle::margin();
	const bool toolbar_visible = gToolBar && gToolBar->getVisible();
	mStackLift = toolbar_visible ? 0 : llmax(margin, pillPad());
	if (mStackNaturalHeight <= 0)
	{
		const S32 height = mOverlayStack->getRect().getHeight();
		if (height > 0)
		{
			mStackNaturalHeight = height;
		}
	}
	const S32 stack_h = mStackNaturalHeight > 0 ? mStackNaturalHeight : llmax(mOverlayStack->getRect().getHeight(), 1);
	const S32 width = getRect().getWidth();
	const S32 stack_left = margin;
	const S32 stack_right = llmax(stack_left, width - margin - pillCap());
	if (mOverlayStack->getRect().mLeft != stack_left || mOverlayStack->getRect().mRight != stack_right)
	{
		LLRect width_rect = mOverlayStack->getRect();
		width_rect.mLeft = stack_left;
		width_rect.mRight = stack_right;
		apply_stack_rect(mOverlayStack, width_rect);
	}
	if (mOverlayStack->needsLayout())
	{
		mOverlayStack->updateLayout();
	}
	const S32 gap = LLFloatingBarStyle::BAR_GAP;
	const S32 span = llmax(0, width - 2 * margin);
	S32 secondary_left = 0;
	S32 secondary_right = 0;
	const bool have_secondary = measureSecondary(secondary_left, secondary_right);
	const S32 secondary_outer = have_secondary ? overlayOuterWidth(secondary_right - secondary_left, pillCap()) : 0;
	const bool voice_on = mVoiceRemote && !gAgentCamera.cameraMouselook() && LLVoiceClient::getInstance()->voiceEnabled();
	if (mVoiceRemote && (bool)mVoiceRemote->getVisible() != voice_on)
		mVoiceRemote->setVisible(voice_on);
	if (voice_on)
		mVoiceRemote->layoutControls(mIconSize);
	const S32 voice_content = voice_on ? mVoiceRemote->getRect().getWidth() : 0;
	const S32 voice_outer = overlayOuterWidth(voice_content, pillCap());
	const S32 main_outer = toolbar_visible ? gToolBar->getPreferredWidth() : 0;
	S32 inline_right = 0;
	if (voice_outer > 0)
		inline_right += voice_outer;
	if (secondary_outer > 0)
	{
		if (inline_right > 0)
			inline_right += gap;
		inline_right += secondary_outer;
	}
	auto fitsBeside = [&](S32 left_width, S32 right_width) -> bool
	{
		if (right_width <= 0)
			return true;
		if (left_width <= 0)
			return right_width <= span;
		return left_width + gap + right_width <= span;
	};
	mVoiceRaised = false;
	mSecondaryRaised = false;
	if (secondary_outer > 0 && voice_outer > 0)
	{
		if (!fitsBeside(main_outer, inline_right))
		{
			mVoiceRaised = true;
			if (!fitsBeside(main_outer, secondary_outer))
				mSecondaryRaised = true;
		}
	}
	else if (secondary_outer > 0)
	{
		if (!fitsBeside(main_outer, secondary_outer))
			mSecondaryRaised = true;
	}
	else if (voice_outer > 0)
	{
		if (!fitsBeside(main_outer, voice_outer))
			mVoiceRaised = true;
	}
	S32 reserve = 0;
	if (toolbar_visible)
	{
		if (have_secondary && !mSecondaryRaised)
			reserve = ((!mVoiceRaised && voice_outer > 0) ? inline_right : secondary_outer) + gap;
		else if (!mVoiceRaised && voice_outer > 0)
			reserve = voice_outer + gap;
	}
	if (gToolBar)
		gToolBar->setMaxButtonSize(mIconSize);
	applyToolbarReserve(reserve);
	mToolbarRow = mStackLift;
	mToolbarRowOk = false;
	auto alignToolbarRow = [&]()
	{
		mToolbarRow = mStackLift;
		mToolbarRowOk = false;
		if (!toolbar_visible || !gToolBar || gToolBar->getPillRect().getWidth() <= 0)
			return;
		const LLRect& pill = gToolBar->getPillRect();
		S32 unused = 0;
		S32 pill_bottom = 0;
		S32 pill_top = 0;
		gToolBar->localPointToOtherView(0, pill.mBottom, &unused, &pill_bottom, this);
		gToolBar->localPointToOtherView(0, pill.mTop, &unused, &pill_top, this);
		const S32 aligned = (pill_bottom + pill_top) / 2 - buttonHeight() / 2;
		if (aligned < mStackLift)
		{
			mToolbarRow = aligned;
			mToolbarRowOk = true;
		}
	};
	alignToolbarRow();
	if (!mToolbarRowOk)
	{
		if (have_secondary && !mSecondaryRaised)
			mSecondaryRaised = true;
		if (voice_outer > 0 && !mVoiceRaised)
			mVoiceRaised = true;
		if (reserve != 0)
		{
			reserve = 0;
			if (gToolBar)
				gToolBar->setMaxButtonSize(LLFloatingBarStyle::ICON_BUTTON_SIZE);
			applyToolbarReserve(reserve);
		}
	}
	if (toolbar_visible)
	{
		const S32 proportion = gToolBar->getButtonSize();
		if (proportion != mIconSize)
		{
			applyIconSize(proportion);
			if (voice_on && mVoiceRemote)
				mVoiceRemote->layoutControls(mIconSize);
			if (mOverlayStack->needsLayout())
				mOverlayStack->updateLayout();
			alignToolbarRow();
		}
	}
	const bool beside = have_secondary && !mSecondaryRaised && mToolbarRowOk;
	LLRect desired;
	desired.mLeft = stack_left;
	desired.mRight = stack_right;
	desired.mBottom = beside ? mToolbarRow : mStackLift;
	desired.mTop = mStackLift + stack_h;
	const bool animate_bottom = mSlidePrimed && beside != mWasBeside;
	mWasBeside = beside;
	presentStack(desired, animate_bottom);
	mOverlayLayoutDirty = false;
}
void LLOverlayBar::requestLayout()
{
	mOverlayLayoutDirty = true;
}
void LLOverlayBar::cacheSecondaryPill()
{
	mSecondaryPillValid = false;
	S32 left = S32_MAX;
	S32 right = 0;
	S32 panel_bottom = 0;
	bool have_panel = false;
	const LLView* panels[4] = { mAOLightContainer, mAORemoteContainer, mMediaRemoteContainer, mAdvSettingsContainer };
	for (const LLView* panel : panels)
	{
		if (!panel || !panel->getVisible() || panel->getRect().getWidth() <= 0)
		{
			continue;
		}
		LLRect screen = panel->getRect();
		if (const LLView* parent = panel->getParent())
		{
			screen.mLeft += parent->getRect().mLeft;
			screen.mRight += parent->getRect().mLeft;
			screen.mBottom += parent->getRect().mBottom;
		}
		left = llmin(left, screen.mLeft);
		right = llmax(right, screen.mRight);
		if (!have_panel || screen.mBottom < panel_bottom)
		{
			panel_bottom = screen.mBottom;
		}
		have_panel = true;
	}
	if (right > left && have_panel)
	{
		const S32 pad = pillPad();
		const S32 cap = pillCap();
		const S32 bottom = panel_bottom - pad;
		mSecondaryPill.set(left - cap, bottom + buttonHeight() + 2 * pad, right + cap - 2, bottom);
		mSecondaryPillValid = true;
	}
}
void LLOverlayBar::draw()
{
	if (mOverlayLayoutDirty || (mOverlayStack && mOverlayStack->needsLayout()) || (gToolBar && gToolBar->isLayoutDirty()))
	{
		layoutOverlayStack();
	}
	else if (mStackSliding)
	{
		tickStackSlide();
	}
	mSlidePrimed = true;
	if (mNewIM && mNewIM->getVisible())
	{
		layoutNewIM();
	}
	if (mSecondaryPillValid)
	{
		mStyle.drawPill(mSecondaryPill, LLFloatingBarStyle::barColor());
	}
	if (mMouseLook && mMouseLook->getVisible())
	{
		mStyle.drawPill(mMouseLook->getRect(), LLFloatingBarStyle::barColor());
	}
	if (mNewIM && mNewIM->getVisible())
	{
		mStyle.drawPill(mNewIMPill, LLFloatingBarStyle::barColor());
	}
	LLPanel::draw();
	if (mNewIM && mNewIM->getVisible())
	{
		LLUIImagePtr border = LLUI::getUIImage("Floating_Pill_Border");
		if (border.notNull())
		{
			const F32 wave = 0.5f + 0.5f * sinf((F32)LLFrameTimer::getElapsedSeconds() * 6.f);
			LLColor4 orange(1.f, 0.42f, 0.f, 0.2f + 0.8f * wave);
			border->draw(mNewIMPill, orange);
		}
	}
}
void LLOverlayBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	S32 delta_width = width - getRect().getWidth();
	S32 delta_height = height - getRect().getHeight();
	if (!delta_width && !delta_height && !sForceReshape)
		return;
	LLView::reshape(width, height, called_from_parent);
	if (mBuilt)
	{
		layoutOverlayStack();
		layoutButtons();
		if (mNewIM && mNewIM->getVisible())
		{
			layoutNewIM();
		}
	}
}
void LLOverlayBar::layoutMouselookButton()
{
	if (!mMouseLook)
		return;
	const S32 width = LLFloatingBarStyle::scaleIcon(OVERLAY_MOUSELOOK_SIZE, mIconSize);
	S32 right = getRect().getWidth() - LLFloatingBarStyle::margin() - pillCap();
	S32 top = mStackLift + buttonHeight();
	S32 secondary_left = 0;
	S32 secondary_right = 0;
	const bool have_secondary = measureSecondary(secondary_left, secondary_right);
	const bool voice_top = mVoicePillValid && (mVoiceRaised || !have_secondary);
	if (voice_top)
	{
		right = mVoiceRemote->getRect().mRight;
		top = mVoiceRemote->getRect().mTop;
	}
	else if (have_secondary)
	{
		right = secondary_right;
		top = mOverlayStack ? mOverlayStack->getRect().mBottom + buttonHeight() : mStackLift + buttonHeight();
	}
	const S32 height = width;
	const S32 y = top + pillPad() + LLFloatingBarStyle::BAR_GAP;
	S32 left = right - width;
	if (left < 0)
		left = 0;
	const LLRect& cur = mMouseLook->getRect();
	if (cur.mLeft != left || cur.mBottom != y || cur.getWidth() != width || cur.getHeight() != height)
	{
		LLRect r;
		r.setOriginAndSize(left, y, width, height);
		mMouseLook->setShape(r);
	}
	sendChildToFront(mMouseLook);
}
void LLOverlayBar::layoutButtons()
{
	if (mStateManagementContainer->getVisible())
	{
		U32 button_count = 0;
		const child_list_t& view_list = *(mStateManagementContainer->getChildList());
		for (LLView* viewp : view_list)
		{
			if(!viewp->getEnabled())
				continue;
			++button_count;
		}
		const S32 MAX_BAR_WIDTH = 600;
		S32 bar_width = llclamp(mStateManagementContainer->getRect().getWidth(), 0, MAX_BAR_WIDTH);
		const S32 MAX_BUTTON_WIDTH = 150;
		static LLCachedControl<S32> status_bar_pad("StatusBarPad",10);
		S32 segment_width = llclamp(lltrunc((F32)(bar_width) / (F32)button_count), 0, MAX_BUTTON_WIDTH);
		S32 btn_width = segment_width - status_bar_pad;
		S32 left = 0;
		S32 bottom = 1;
		for (LLView* viewp : boost::adaptors::reverse(view_list))
		{
			if(!viewp->getEnabled())
				continue;
			LLRect r = viewp->getRect();
			r.setOriginAndSize(left, bottom, btn_width, r.getHeight());
			viewp->setRect(r);
			left += segment_width;
		}
	}
	layoutNewIM();
}
void LLOverlayBar::layoutNewIM()
{
	if (!mNewIM)
	{
		return;
	}
	const LLFontGL* font = mNewIM->getFont();
	const S32 text_w = font ? font->getWidth(mNewIM->getLabelUnselected()) : 64;
	const S32 pill_h = 28;
	const S32 pad_y = 3;
	const S32 btn_h = pill_h - pad_y * 2;
	const S32 pad_x = 14;
	const S32 btn_w = llmax(text_w + pad_x * 2, btn_h);
	const S32 pill_w = btn_w + pad_y * 2;
	S32 center = getRect().getWidth() / 2;
	S32 bottom = LLFloatingBarStyle::BAR_GAP;
	if (gToolBar && gToolBar->getVisible() && gToolBar->getPillRect().getWidth() > 0)
	{
		const LLRect& pill = gToolBar->getPillRect();
		const LLRect& add = gToolBar->getAddRect();
		const S32 chrome_top = llmax(pill.mTop, add.mTop);
		const S32 inset = llmax(0, gToolBar->getRect().getHeight() - chrome_top);
		center = (pill.mLeft + pill.mRight) / 2;
		bottom = llmax(0, LLFloatingBarStyle::BAR_GAP - inset);
	}
	S32 left = center - pill_w / 2;
	left = llclamp(left, 0, llmax(0, getRect().getWidth() - pill_w));
	if (mChatBar && mChatBar->getVisible())
	{
		S32 chat_left = 0;
		S32 chat_bottom = 0;
		S32 chat_right = 0;
		S32 chat_top = 0;
		mChatBar->localPointToOtherView(0, 0, &chat_left, &chat_bottom, this);
		mChatBar->localPointToOtherView(mChatBar->getRect().getWidth(), mChatBar->getRect().getHeight(), &chat_right, &chat_top, this);
		const S32 btn_right = left + pill_w;
		if (btn_right > chat_left && left < chat_right && bottom < chat_top && bottom + pill_h > chat_bottom)
		{
			bottom = chat_top + LLFloatingBarStyle::BAR_GAP;
		}
	}
	mNewIMPill.setOriginAndSize(left, bottom, pill_w, pill_h);
	const LLRect button_rect(left + pad_y, bottom + pad_y + btn_h, left + pad_y + btn_w, bottom + pad_y);
	if (mNewIM->getRect() != button_rect)
	{
		mNewIM->setShape(button_rect);
	}
}
LLButton* LLOverlayBar::updateButtonVisiblity(LLButton* button, bool visible)
{
	if (button && (bool)button->getVisible() != visible)
	{
		button->setVisible(visible);
		sendChildToFront(button);
		moveChildToBackOfTabGroup(button);
	}
	return button;
}
void LLOverlayBar::refresh()
{
	bool buttons_changed = FALSE;
	int unread_count(gIMMgr->getIMUnreadCount());
	static const LLCachedControl<bool> per_conversation("NewIMsPerConversation");
	static const LLCachedControl<bool> reset_count("NewIMsPerConversationReset");
	if (per_conversation && (!reset_count || unread_count) && !LLFloaterChatterBox::instanceVisible())
	{
		unread_count = 0;
		for(std::set<LLHandle<LLFloater> >::const_iterator it = gIMMgr->getIMFloaterHandles().begin(); it != gIMMgr->getIMFloaterHandles().end(); ++it)
			if (LLFloaterIMPanel* im_floater = static_cast<LLFloaterIMPanel*>(it->get()))
				if (im_floater->getParent() != gFloaterView && im_floater->getNumUnreadMessages())
					++unread_count;
	}
	if (LLButton* button = updateButtonVisiblity(mNewIM, unread_count))
	{
		if (unread_count > 0)
		{
			if (unread_count > 1)
			{
				std::stringstream ss;
				ss << unread_count << " " << mUnreadCountStringPlural;
				button->setLabel(ss.str());
			}
			else
			{
				button->setLabel("1 " + mOriginalIMLabel);
			}
		}
		buttons_changed = true;
	}
	buttons_changed |= updateButtonVisiblity(mNotBusy, gAgent.isDoNotDisturb()) != NULL;
	buttons_changed |= updateButtonVisiblity(mFlyCam,LLViewerJoystick::getInstance()->getOverrideCamera()) != NULL;
	buttons_changed |= updateButtonVisiblity(mStandUp,isAgentAvatarValid() && gAgentAvatarp->isSitting() && !gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) != NULL;
	buttons_changed |= updateButtonVisiblity(mCancelBtn,(gAgent.getTeleportState() >= LLAgent::TELEPORT_START) &&	(gAgent.getTeleportState() <= LLAgent::TELEPORT_MOVING)) != NULL;
	if (mVoiceRemote)
		moveChildToBackOfTabGroup(mVoiceRemote);
	const bool muted = gSavedSettings.getBOOL("MuteAudio");
	if (mVolumeButton && (!mVolumeIconReady || muted != mVolumeMutedIcon))
	{
		mVolumeMutedIcon = muted;
		mVolumeIconReady = true;
		mVolumeButton->setImageOverlay(muted ? "floating_ui/volume_muted.png" : "floating_ui/volume.png", LLFontGL::HCENTER, LLColor4::white);
		mVolumeButton->setImageOverlaySelectedColor(LLFloatingBarStyle::accentColor());
	}
	static BOOL last_mouselook = FALSE;
	BOOL in_mouselook = gAgentCamera.cameraMouselook();
	if (last_mouselook != in_mouselook)
	{
		stashOverlayFloaters(in_mouselook);
		last_mouselook = in_mouselook;
		static const LLCachedControl<bool> enable_ao_remote("EnableAORemote", true);
		mMediaRemoteContainer->setVisible(!in_mouselook);
		mAdvSettingsContainer->setVisible(!in_mouselook);
		mAORemoteContainer->setVisible(!in_mouselook && enable_ao_remote);
		mAOLightContainer->setVisible(!in_mouselook && enable_ao_remote);
		mStateManagementContainer->setVisible(!in_mouselook);
	}
	else if (in_mouselook)
	{
		stashOverlayFloaters(true);
	}
	if (mMouseLook && (bool)mMouseLook->getToggleState() != (bool)in_mouselook)
		mMouseLook->setToggleState(in_mouselook);
	if (mVoiceRemote)
	{
		const bool voice_on = !in_mouselook && LLVoiceClient::getInstance()->voiceEnabled();
		if ((bool)mVoiceRemote->getVisible() != voice_on)
		{
			mVoiceRemote->setVisible(voice_on);
			requestLayout();
		}
	}
	if (buttons_changed)
	{
		layoutButtons();
	}
}
void LLOverlayBar::onClickIMReceived(void*)
{
	gIMMgr->setFloaterOpen(TRUE);
}
void LLOverlayBar::onClickSetNotBusy(void*)
{
	gAgent.setDoNotDisturb(false);
}
void LLOverlayBar::onClickFlycam(void*)
{
	LLViewerJoystick::getInstance()->toggleFlycam();
}
void LLOverlayBar::onClickResetView(void* data)
{
	handle_reset_view();
}
void LLOverlayBar::onClickMouselook(void*)
{
	if (gAgentCamera.cameraMouselook())
		gAgentCamera.changeCameraToDefault();
	else
		gAgentCamera.changeCameraToMouselook();
}
void LLOverlayBar::onClickStandUp(void*)
{
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) && (gAgentAvatarp) && (gAgentAvatarp->isSitting()) )
	{
		return;
	}
	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
}
void LLOverlayBar::onClickCancelTP(void* data)
{
	LLOverlayBar* self = (LLOverlayBar*)data;
	self->setCancelTPButtonVisible(FALSE, std::string("Cancel TP"));
	gAgent.teleportCancel();
	LL_INFOS() << "trying to cancel teleport" << LL_ENDL;
}
void LLOverlayBar::setCancelTPButtonVisible(BOOL b, const std::string& label)
{
	mCancelBtn->setVisible( b );
	mCancelBtn->setLabelSelected(label);
	mCancelBtn->setLabelUnselected(label);
}
void LLOverlayBar::audioFilterPlay()
{
	if (gOverlayBar && gOverlayBar->mMusicState != PLAYING)
	{
		gOverlayBar->mMusicState = PLAYING;
	}
}
void LLOverlayBar::audioFilterStop()
{
	if (gOverlayBar && gOverlayBar->mMusicState != STOPPED)
	{
		gOverlayBar->mMusicState = STOPPED;
	}
}
void LLOverlayBar::mediaStop(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	LLViewerParcelMedia::stop();
}
void LLOverlayBar::toggleMediaPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	if (LLViewerParcelMedia::getStatus() == LLViewerMediaImpl::MEDIA_PAUSED)
	{
		LLViewerParcelMedia::start();
	}
	else if(LLViewerParcelMedia::getStatus() == LLViewerMediaImpl::MEDIA_PLAYING)
	{
		LLViewerParcelMedia::pause();
	}
	else
	{
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		if (parcel)
		{
			LLViewerParcelMedia::play(parcel);
		}
	}
}
void LLOverlayBar::toggleMusicPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	if (gOverlayBar->mMusicState != PLAYING)
	{
		gOverlayBar->mMusicState = PLAYING;
		if (gAudiop)
		{
			LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
			if ( parcel )
			{
				{
					LLViewerParcelMedia::playStreamingMusic(parcel);
				}
			}
		}
	}
	else
	{
		gOverlayBar->mMusicState = STOPPED;
		if (gAudiop)
		{
			gAudiop->stopInternetStream();
		}
	}
}
