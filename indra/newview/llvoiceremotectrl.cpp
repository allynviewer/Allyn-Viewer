/* @file llvoiceremotectrl.cpp
 * @brief A remote control for voice chat
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#include "llvoiceremotectrl.h"
#include "llagent.h"
#include "llui.h"
#include "llbutton.h"
#include "lluictrlfactory.h"
#include "llvoicechannel.h"
#include "llvoiceclient.h"
#include "llfloateractivespeakers.h"
#include "llfloaterchatterbox.h"
#include "lliconctrl.h"
#include "lloverlaybar.h"
#include "lltextbox.h"
LLVoiceRemoteCtrl::LLVoiceRemoteCtrl (const std::string& name) : LLPanel(name), mTalkBlip()
{
	setIsChrome(TRUE);
	LLUICtrlFactory::getInstance()->buildPanel(this, gSavedSettings.getBOOL("ShowVoiceChannelPopup") ? "panel_voice_remote_expanded.xml" : "panel_voice_remote.xml");
	setFocusRoot(TRUE);
}
LLVoiceRemoteCtrl::~LLVoiceRemoteCtrl()
{
}
BOOL LLVoiceRemoteCtrl::postBuild()
{
	mTalkBtn = getChild<LLButton>("push_to_talk");
	mTalkBtn->setClickedCallback(boost::bind(&LLVoiceRemoteCtrl::onBtnTalkClicked));
	mTalkBtn->setHeldDownCallback(boost::bind(&LLVoiceRemoteCtrl::onBtnTalkHeld));
	mTalkBtn->setMouseUpCallback(boost::bind(&LLVoiceRemoteCtrl::onBtnTalkReleased));
	mTalkLockBtn = getChild<LLButton>("ptt_lock");
	mSpeakersBtn = getChild<LLButton>("speakers_btn");
	mSpeakersBtn->setClickedCallback(boost::bind(&LLVoiceRemoteCtrl::onClickSpeakers));
	childSetAction("show_channel", onClickPopupBtn, this);
	if (LLButton* end_call_btn = findChild<LLButton>("end_call_btn"))
		end_call_btn->setClickedCallback(boost::bind(&LLVoiceRemoteCtrl::onClickEndCall));
	LLTextBox* text = findChild<LLTextBox>("channel_label");
	if (text)
	{
		text->setUseEllipses(TRUE);
	}
	if (LLButton* voice_channel_bg = findChild<LLButton>("voice_channel_bg"))
		voice_channel_bg->setClickedCallback(boost::bind(&LLVoiceRemoteCtrl::onClickVoiceChannel));
	mVoiceVolIcon.connect(this,"voice_volume");
	layoutControls();
	return TRUE;
}
void LLVoiceRemoteCtrl::layoutControls()
{
	const S32 gap = 2;
	const S32 button_h = 29;
	const S32 speakers_w = 52;
	const S32 side_w = 36;
	S32 text_w = 40;
	if (mTalkBtn->getFont())
		text_w = mTalkBtn->getFont()->getWidth(mTalkBtn->getCurrentLabel().getWString());
	S32 talk_w = mTalkBtn->getLeftHPad() + 16 + 1 + text_w + mTalkBtn->getRightHPad();
	if (talk_w < 72)
		talk_w = 72;
	S32 x = 0;
	mSpeakersBtn->setOrigin(x, 0);
	mSpeakersBtn->reshape(speakers_w, button_h);
	x += speakers_w + gap;
	mTalkBtn->setOrigin(x, 0);
	mTalkBtn->reshape(talk_w, button_h);
	x += talk_w + gap;
	mTalkLockBtn->setOrigin(x, 0);
	mTalkLockBtn->reshape(side_w, button_h);
	x += side_w + gap;
	if (LLIconCtrl* volume = mVoiceVolIcon)
		volume->setVisible(FALSE);
	if (LLButton* show_channel = findChild<LLButton>("show_channel"))
	{
		show_channel->setOrigin(x, 0);
		show_channel->reshape(side_w, button_h);
		x += side_w;
	}
	const S32 width = llmax(x, 1);
	const bool expanded = findChild<LLButton>("voice_channel_bg") != NULL;
	const S32 height = expanded ? (button_h + gap + button_h) : button_h;
	reshape(width, height);
	if (LLPanel* controls = findChild<LLPanel>("voice_controls"))
	{
		controls->setOrigin(0, 0);
		controls->reshape(width, button_h);
	}
	if (expanded)
	{
		const S32 row_y = button_h + gap;
		if (LLButton* end_call = findChild<LLButton>("end_call_btn"))
		{
			end_call->setOrigin(0, row_y);
			end_call->reshape(side_w, button_h);
		}
		const S32 channel_x = side_w + gap;
		const S32 channel_w = llmax(80, width - channel_x);
		if (LLButton* channel = findChild<LLButton>("voice_channel_bg"))
		{
			channel->setOrigin(channel_x, row_y);
			channel->reshape(channel_w, button_h);
		}
		if (LLIconCtrl* channel_icon = findChild<LLIconCtrl>("voice_channel_icon"))
		{
			channel_icon->setOrigin(channel_x + 8, row_y + ((button_h - 16) / 2));
			channel_icon->reshape(16, 16);
		}
		if (LLTextBox* channel_label = findChild<LLTextBox>("channel_label"))
		{
			channel_label->setOrigin(channel_x + 28, row_y + ((button_h - 16) / 2));
			channel_label->reshape(llmax(20, channel_w - 36), 16);
		}
	}
	if (LLLayoutPanel* panel = dynamic_cast<LLLayoutPanel*>(getParent()))
	{
		panel->setMinDim(width);
		panel->setExpandedMinDim(width);
		S32 panel_h = panel->getRect().getHeight();
		if (panel_h < 1)
			panel_h = height;
		panel->reshape(width, panel_h);
	}
}
void LLVoiceRemoteCtrl::draw()
{
	BOOL voice_active = FALSE;
	LLVoiceChannel* channelp = LLVoiceChannel::getCurrentVoiceChannel();
	if (channelp)
	{
		voice_active = channelp->isActive();
	}
	mTalkBtn->setEnabled(voice_active);
	mTalkLockBtn->setEnabled(voice_active);
	if (!mTalkBtn->hasMouseCapture())
	{
		mTalkBtn->setToggleState(!mTalkLockBtn->getToggleState() || LLVoiceClient::getInstance()->getUserPTTState());
	}
	mSpeakersBtn->setToggleState(LLFloaterActiveSpeakers::instanceVisible(LLSD()));
	std::string talk_blip_image;
	if (LLVoiceClient::getInstance()->getIsSpeaking(gAgent.getID()))
	{
		F32 voice_power = LLVoiceClient::getInstance()->getCurrentPower(gAgent.getID());
		if (voice_power > LLVoiceClient::OVERDRIVEN_POWER_LEVEL)
		{
			talk_blip_image = "icn_voice_ptt-on-lvl3.tga";
		}
		else
		{
			F32 power = LLVoiceClient::getInstance()->getCurrentPower(gAgent.getID());
			S32 icon_image_idx = llmin(2, llfloor((power / LLVoiceClient::OVERDRIVEN_POWER_LEVEL) * 3.f));
			switch(icon_image_idx)
			{
			case 0:
				talk_blip_image = "icn_voice_ptt-on.tga";
				break;
			case 1:
				talk_blip_image = "icn_voice_ptt-on-lvl1.tga";
				break;
			case 2:
				talk_blip_image = "icn_voice_ptt-on-lvl2.tga";
				break;
			}
		}
	}
	else
	{
		talk_blip_image = "icn_voice_ptt-off.tga";
	}
	if (LLIconCtrl* icon = mVoiceVolIcon)
		icon->setVisible(FALSE);
	if (talk_blip_image != mTalkBlip)
	{
		mTalkBlip = talk_blip_image;
		mTalkBtn->setImageOverlay(talk_blip_image, LLFontGL::LEFT);
	}
	LLFloater* voice_floater = LLFloaterChatterBox::getInstance()->getCurrentVoiceFloater();
	LLVoiceChannel* current_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (!voice_floater)
	{
		voice_floater = gIMMgr->findFloaterBySession(current_channel->getSessionID());
	}
	std::string active_channel_name;
	if (voice_floater)
	{
		active_channel_name = voice_floater->getShortTitle();
	}
	if (LLButton* end_call_btn = findChild<LLButton>("end_call_btn"))
		end_call_btn->setEnabled(LLVoiceClient::getInstance()->voiceEnabled()
								&& current_channel
								&& current_channel->isActive()
								&& current_channel != LLVoiceChannelProximal::getInstance());
	if (LLTextBox* text = findChild<LLTextBox>("channel_label"))
		text->setValue(active_channel_name);
	LLButton* voice_channel_bg = findChild<LLButton>("voice_channel_bg");
	if (voice_channel_bg) voice_channel_bg->setToolTip(active_channel_name);
	if (current_channel)
	{
		LLIconCtrl* voice_channel_icon = findChild<LLIconCtrl>("voice_channel_icon");
		if (voice_channel_icon && voice_floater)
		{
			voice_channel_icon->setValue(voice_floater->getString("voice_icon"));
		}
		if (voice_channel_bg)
		{
			LLColor4 bg_color;
			if (current_channel->isActive())
			{
				bg_color = lerp(LLColor4::green, LLColor4::white, 0.7f);
			}
			else if (current_channel->getState() == LLVoiceChannel::STATE_ERROR)
			{
				bg_color = lerp(LLColor4::red, LLColor4::white, 0.7f);
			}
			else
			{
				bg_color = lerp(LLColor4::yellow, LLColor4::white, 0.7f);
			}
			voice_channel_bg->setImageColor(bg_color);
		}
	}
	LLPanel::draw();
}
void LLVoiceRemoteCtrl::onBtnTalkClicked()
{
	if (gSavedSettings.getBOOL("PushToTalkToggle"))
	{
		LLVoiceClient::getInstance()->toggleUserPTTState();
	}
}
void LLVoiceRemoteCtrl::onBtnTalkHeld()
{
	if (!gSavedSettings.getBOOL("PushToTalkToggle"))
	{
		LLVoiceClient::getInstance()->setUserPTTState(true);
	}
}
void LLVoiceRemoteCtrl::onBtnTalkReleased()
{
	if (!gSavedSettings.getBOOL("PushToTalkToggle"))
	{
		LLVoiceClient::getInstance()->setUserPTTState(false);
	}
}
void LLVoiceRemoteCtrl::onClickPopupBtn(void* user_data)
{
	LLVoiceRemoteCtrl* remotep = (LLVoiceRemoteCtrl*)user_data;
	remotep->deleteAllChildren();
	LLUICtrlFactory::getInstance()->buildPanel(remotep, gSavedSettings.getBOOL("ShowVoiceChannelPopup") ? "panel_voice_remote_expanded.xml" : "panel_voice_remote.xml");
	gOverlayBar->layoutButtons();
}
void LLVoiceRemoteCtrl::onClickEndCall()
{
	LLVoiceChannel* current_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (current_channel && current_channel != LLVoiceChannelProximal::getInstance())
	{
		current_channel->deactivate();
	}
}
void LLVoiceRemoteCtrl::onClickSpeakers()
{
	LLFloaterActiveSpeakers::toggleInstance(LLSD());
}
void LLVoiceRemoteCtrl::onClickVoiceChannel()
{
	if (LLFloater* floater = LLFloaterChatterBox::getInstance()->getCurrentVoiceFloater())
	{
		if (LLMultiFloater* mf = floater->getHost())
			mf->showFloater(floater);
		else
			floater->open();
	}
	else if (LLVoiceChannel* chan = LLVoiceChannel::getCurrentVoiceChannel())
	{
		if (LLFloaterIMPanel* floater = gIMMgr->findFloaterBySession(chan->getSessionID()))
			floater->open();
	}
}
