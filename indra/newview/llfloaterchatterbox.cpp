/** 
 * @file llfloaterchatterbox.cpp
 * @author Richard
 * @date 2007-05-08
 * @brief Implementation of the chatterbox integrated conversation ui
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#include "llfloaterchatterbox.h"
#include "lluictrlfactory.h"
#include "llfloaterchat.h"
#include "llfloaterfriends.h"
#include "llfloatergroups.h"
#include "llvoicechannel.h"
#include "llimview.h"
#include "llimpanel.h"
#include "llstring.h"
#include "llviewercontrol.h"
namespace
{
	void handleLocalChatBar(LLFloaterChat* floater_chat, bool show_bar)
	{
		floater_chat->childSetVisible("chat_layout_panel", show_bar || !gSavedSettings.getBOOL("ChatHistoryTornOff"));
	}
}
LLFloaterMyFriends::LLFloaterMyFriends(const LLSD& seed)
{
	mFactoryMap["friends_panel"] = LLCallbackMap(LLFloaterMyFriends::createFriendsPanel, NULL);
	mFactoryMap["groups_panel"] = LLCallbackMap(LLFloaterMyFriends::createGroupsPanel, NULL);
	BOOL no_open = FALSE;
	static LLCachedControl<bool> horiz("ContactsUseHorizontalButtons");
	LLUICtrlFactory::getInstance()->buildFloater(this, (horiz ? "floater_my_friends_horiz.xml" : "floater_my_friends.xml"), &getFactoryMap(), no_open);
}
LLFloaterMyFriends::~LLFloaterMyFriends()
{
}
BOOL LLFloaterMyFriends::postBuild()
{
	mTabs = getChild<LLTabContainer>("friends_and_groups");
	return TRUE;
}
void LLFloaterMyFriends::onOpen()
{
	gSavedSettings.setBOOL("ShowContacts", true);
}
void LLFloaterMyFriends::onClose(bool app_quitting)
{
	if (!app_quitting) gSavedSettings.setBOOL("ShowContacts", false);
	setVisible(FALSE);
}
void* LLFloaterMyFriends::createFriendsPanel(void* data)
{
	return new LLPanelFriends();
}
void* LLFloaterMyFriends::createGroupsPanel(void* data)
{
	return new LLPanelGroups();
}
LLFloaterChatterBox::LLFloaterChatterBox(const LLSD& seed) :
	mActiveVoiceFloater(NULL)
{
	mAutoResize = FALSE;
	if(!gSavedSettings.getBOOL("WoLfVerticalIMTabs"))
	{
		LLUICtrlFactory::getInstance()->buildFloater(this, "floater_chatterbox.xml", NULL, FALSE);
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildFloater(this, "floater_chatterbox_wolf.xml", NULL, FALSE);
	}
	if (mTabContainer && mTabContainer->isVertical())
	{
		mTabContainer->setUserResize(TRUE);
		const S32 saved_width = gSavedSettings.getS32("CommunicateTabWidth");
		if (saved_width > 0)
		{
			mTabContainer->setVerticalTabWidth(saved_width);
		}
		mTabContainer->setTabWidthCallback([](S32 width) {
			gSavedSettings.setS32("CommunicateTabWidth", width);
		});
	}
	if (gSavedSettings.getBOOL("ContactsTornOff"))
	{
		LLFloaterMyFriends* floater_contacts = LLFloaterMyFriends::getInstance(0);
		addFloater(floater_contacts, FALSE);
		removeFloater(floater_contacts);
		gFloaterView->addChild(floater_contacts);
		if (gSavedSettings.getBOOL("ShowContacts")) floater_contacts->open();
	}
	else
	{
		addFloater(LLFloaterMyFriends::getInstance(0), TRUE);
	}
	LLFloaterChat* floater_chat = LLFloaterChat::getInstance();
	if (gSavedSettings.getBOOL("ChatHistoryTornOff"))
	{
		addFloater(floater_chat, FALSE);
		removeFloater(floater_chat);
		gFloaterView->addChild(floater_chat);
		if (gSavedSettings.getBOOL("ShowChatHistory")) floater_chat->open();
	}
	else
	{
		addFloater(floater_chat, FALSE);
	}
	if (gSavedSettings.getBOOL("ShowCommunicate")) open();
	gSavedSettings.getControl("ShowLocalChatFloaterBar")->getSignal()->connect(boost::bind(handleLocalChatBar, floater_chat, _2));
	mTabContainer->lockTabs();
}
LLFloaterChatterBox::~LLFloaterChatterBox()
{
}
BOOL LLFloaterChatterBox::handleKeyHere(KEY key, MASK mask)
{
	if (key == 'W' && mask == MASK_CONTROL)
	{
		LLFloater* floater = getActiveFloater();
		if (floater && floater->canClose())
		{
			if (floater->isCloseable())
			{
				floater->close();
			}
			else
			{
				close();
			}
		}
		return TRUE;
	}
	return LLMultiFloater::handleKeyHere(key, mask);
}
void LLFloaterChatterBox::draw()
{
	if (!isMinimized())
	{
		gIMMgr->clearNewIMNotification();
	}
	LLFloater* current_active_floater = getCurrentVoiceFloater();
	if(mActiveVoiceFloater != current_active_floater)
	{
		if (mActiveVoiceFloater)
		{
			mTabContainer->setTabImage(mActiveVoiceFloater, "");
		}
	}
	if (current_active_floater)
	{
		LLColor4 icon_color = LLColor4::white;
		LLVoiceChannel* channelp = LLVoiceChannel::getCurrentVoiceChannel();
		if (channelp)
		{
			if (channelp->isActive())
			{
				icon_color = LLColor4::green;
			}
			else if (channelp->getState() == LLVoiceChannel::STATE_ERROR)
			{
				icon_color = LLColor4::red;
			}
			else
			{
				icon_color = LLColor4::yellow;
			}
		}
		mTabContainer->setTabImage(current_active_floater, "active_voice_tab.tga", icon_color);
	}
	mActiveVoiceFloater = current_active_floater;
	LLMultiFloater::draw();
}
void LLFloaterChatterBox::onOpen()
{
	gSavedSettings.setBOOL("ShowCommunicate", TRUE);
}
void LLFloaterChatterBox::onClose(bool app_quitting)
{
	setVisible(FALSE);
	if (!app_quitting) gSavedSettings.setBOOL("ShowCommunicate", false);
}
void LLFloaterChatterBox::setMinimized(BOOL minimized)
{
	LLFloater::setMinimized(minimized);
	LLFloaterChat::getInstance()->updateConsoleVisibility();
}
void LLFloaterChatterBox::removeFloater(LLFloater* floaterp)
{
	if (floaterp->getName() == "chat floater")
	{
		mTabContainer->lockTabs(mTabContainer->getNumLockedTabs() - 1);
		gSavedSettings.setBOOL("ChatHistoryTornOff", TRUE);
		if (!gSavedSettings.getBOOL("ShowLocalChatFloaterBar")) floaterp->childSetVisible("chat_layout_panel", false);
		floaterp->setCanClose(TRUE);
	}
	else if (floaterp->getName() == "floater_my_friends")
	{
		mTabContainer->lockTabs(mTabContainer->getNumLockedTabs() - 1);
		gSavedSettings.setBOOL("ContactsTornOff", TRUE);
		floaterp->setCanClose(TRUE);
	}
	LLMultiFloater::removeFloater(floaterp);
}
void LLFloaterChatterBox::addFloater(LLFloater* floaterp,
									BOOL select_added_floater,
									LLTabContainer::eInsertionPoint insertion_point)
{
	S32 num_locked_tabs = mTabContainer->getNumLockedTabs();
	if (floaterp->getHost() == this) return;
	if (floaterp->getName() == "chat floater")
	{
		mTabContainer->unlockTabs();
		if (findChild<LLView>("floater_my_friends"))
		{
			mTabContainer->selectFirstTab();
			LLMultiFloater::addFloater(floaterp, select_added_floater, LLTabContainer::RIGHT_OF_CURRENT);
		}
		else
		{
			LLMultiFloater::addFloater(floaterp, select_added_floater, LLTabContainer::START);
		}
		mTabContainer->lockTabs(num_locked_tabs + 1);
		gSavedSettings.setBOOL("ChatHistoryTornOff", FALSE);
		floaterp->childSetVisible("chat_layout_panel", true);
		floaterp->setCanClose(FALSE);
	}
	else if (floaterp->getName() == "floater_my_friends")
	{
		mTabContainer->unlockTabs();
		LLMultiFloater::addFloater(floaterp, select_added_floater, LLTabContainer::START);
		mTabContainer->lockTabs(num_locked_tabs + 1);
		gSavedSettings.setBOOL("ContactsTornOff", FALSE);
		floaterp->setCanClose(FALSE);
	}
	else
	{
		LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);
	}
	if (floaterp == mActiveVoiceFloater)
	{
		mTabContainer->setTabImage(floaterp, "active_voice_tab.tga");
	}
}
LLFloater* LLFloaterChatterBox::getCurrentVoiceFloater()
{
	if (!LLVoiceClient::getInstance()->voiceEnabled())
	{
		return NULL;
	}
	if (LLVoiceChannelProximal::getInstance() == LLVoiceChannel::getCurrentVoiceChannel())
	{
		return LLFloaterChat::getInstance(LLSD());
	}
	else
	{
		LLFloaterChatterBox* floater = LLFloaterChatterBox::getInstance(LLSD());
		for (S32 i = 0; i < floater->getFloaterCount(); i++)
		{
			LLPanel* panelp = floater->mTabContainer->getPanelByIndex(i);
			if (panelp->getName() == "im_floater")
			{
				LLFloaterIMPanel* im_floaterp = (LLFloaterIMPanel*)panelp;
				if (im_floaterp->getVoiceChannel()  == LLVoiceChannel::getCurrentVoiceChannel())
				{
					return im_floaterp;
				}
			}
		}
	}
	return NULL;
}
