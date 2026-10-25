/** 
 * @file llnotify.cpp
 * @brief Non-blocking notification that doesn't take keyboard focus.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
#include <algorithm>
#include <vector>
#include "llnotify.h"
#include "llchat.h"
#include "lliconctrl.h"
#include "llmenugl.h"
#include "llscrollcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltrans.h"
#include "lluiconstants.h"
#include "llviewerdisplay.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llfloaterchat.h"
#include "llfloaternotifywell.h"
#include "llgroupnotify.h"
#include "llnavigationbar.h"
#include "llnotifyiconbar.h"
#include "lluictrlfactory.h"
#include "llcheckboxctrl.h"
#include "llviewercontrol.h"
#include "llstring.h"
#include "llurlmatch.h"
#include "llurlregistry.h"
#include <boost/function.hpp>
#include "hippogridmanager.h"
#include "llstartup.h"
#include "rlvhandler.h"
LLNotifyBoxView* gNotifyBoxView = NULL;
const F32 ANIMATION_TIME = 0.333f;
const F32 STACK_SLIDE_PIXELS = 12.f;
const F32 TOAST_FADE_TIME = 2.f;
const S32 BOTTOM_PAD = VPAD * 3;
const S32 STACK_GAP = 7;
const S32 STACK_TOP_PAD = 7;
const S32 TOAST_PAD = 10;
const S32 TOAST_CLOSE = 16;
const S32 TOAST_BTN_WIDTH = 90;
const S32 TOAST_MIN_TIP_HEIGHT = 40;
const S32 TOAST_MIN_HEIGHT = 72;
const S32 TOAST_MAX_HEIGHT = 420;
constexpr F32 LLNotifyBox::MIN_TOAST_DURATION;
constexpr F32 LLNotifyBox::MAX_TOAST_DURATION;
const F64 STARTUP_SUMMARY_MAX_WAIT = 15.0;
static S32 sStartupSyncDepth = 0;
static bool sOfflineMessagesPending = false;
static bool sStartupSummaryDirty = false;
static bool sStartupSummaryNewItems = false;
static F64 sStartupSummaryWaitStart = -1.0;
static LLUUID sStartupSummaryID;
static S32 sStartupSummaryCount = 0;
static std::vector<LLHandle<LLPanel> > sStartupArchived;
const std::string STARTUP_SUMMARY_REPOST = "STARTUP_SUMMARY_REPOST";
static F32 notify_toast_duration()
{
	return LLNotifyBox::getToastDuration();
}
F32 LLNotifyBox::getToastDuration()
{
	static const LLCachedControl<F32> duration(gSavedSettings, "NotifyBoxToastDuration", 30.f);
	return llclamp((F32)duration, MIN_TOAST_DURATION, MAX_TOAST_DURATION);
}
bool LLNotifyBox::isStartupSync()
{
	if (sStartupSyncDepth > 0)
	{
		return true;
	}
	const EStartupState state = LLStartUp::getStartupState();
	return state >= STATE_WORLD_INIT && state < STATE_STARTED;
}
bool LLNotifyBox::isStartupSync(const LLNotificationPtr& notification)
{
	if (notification && notification->getPayload()["STARTUP_SYNC"].asBoolean())
	{
		return true;
	}
	return isStartupSync();
}
bool LLNotifyBox::archiveDuringStartup(const LLNotificationPtr& notification, LLPanel* box)
{
	if (!notification || !box || notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH)
	{
		return false;
	}
	if (!isStartupSync(notification))
	{
		return false;
	}
	sStartupArchived.push_back(box->getHandle());
	sStartupSummaryDirty = true;
	sStartupSummaryNewItems = true;
	return true;
}
static LLNotifyBox* live_startup_summary_box()
{
	if (sStartupSummaryID.isNull())
	{
		return NULL;
	}
	LLNotifyBox* box = LLInstanceTracker<LLNotifyBox, LLUUID>::getInstance(sStartupSummaryID);
	if (!box || box->isDead())
	{
		return NULL;
	}
	LLNotificationPtr notification = box->getNotification();
	if (!notification || notification->isCancelled() || notification->isRespondedTo())
	{
		return NULL;
	}
	return box;
}
void LLNotifyBox::setOfflineMessagesPending(bool pending)
{
	sOfflineMessagesPending = pending;
}
void LLNotifyBox::flushStartupSummary()
{
	if (isStartupSync())
	{
		return;
	}
	LLNotifyBox* summary_box = live_startup_summary_box();
	if (!sStartupSummaryDirty && !summary_box)
	{
		return;
	}
	if (!summary_box && sOfflineMessagesPending)
	{
		const F64 now = LLTimer::getTotalSeconds();
		if (sStartupSummaryWaitStart < 0.0)
		{
			sStartupSummaryWaitStart = now;
		}
		if (now - sStartupSummaryWaitStart < STARTUP_SUMMARY_MAX_WAIT)
		{
			return;
		}
	}
	sStartupSummaryWaitStart = -1.0;
	const bool new_items = sStartupSummaryNewItems;
	sStartupSummaryDirty = false;
	sStartupSummaryNewItems = false;
	sStartupArchived.erase(std::remove_if(sStartupArchived.begin(), sStartupArchived.end(),
		[](const LLHandle<LLPanel>& handle)
		{
			LLPanel* box = handle.get();
			return !box || box->isDead();
		}),
		sStartupArchived.end());
	const S32 count = static_cast<S32>(sStartupArchived.size());
	if (summary_box)
	{
		LLNotificationPtr notification = summary_box->getNotification();
		if (count <= 0)
		{
			sStartupSummaryCount = 0;
			LLNotifications::instance().cancel(notification);
			return;
		}
		if (count != sStartupSummaryCount)
		{
			sStartupSummaryCount = count;
			LLSD updated = notification->asLLSD();
			updated["substitutions"]["COUNT"] = count;
			notification->updateFrom(LLNotificationPtr(new LLNotification(updated)));
			summary_box->refreshMessage();
		}
		return;
	}
	if (count <= 0 || !new_items)
	{
		sStartupSummaryCount = 0;
		return;
	}
	LLSD payload = LLSD::emptyMap();
	if (sStartupSummaryID.notNull())
	{
		payload[STARTUP_SUMMARY_REPOST] = true;
	}
	LL_INFOS("Notifications") << count << " notifications received during login are archived in the Notifications well" << LL_ENDL;
	LLNotificationPtr summary = LLNotifications::instance().add("LoginNotificationsArchived", LLSD().with("COUNT", count), payload);
	if (summary)
	{
		sStartupSummaryID = summary->getID();
		sStartupSummaryCount = count;
	}
}
LLNotifyBox::StartupSyncScope::StartupSyncScope(bool active)
	: mActive(active)
{
	if (mActive)
	{
		++sStartupSyncDepth;
	}
}
LLNotifyBox::StartupSyncScope::~StartupSyncScope()
{
	if (mActive)
	{
		--sStartupSyncDepth;
	}
}
static bool is_friend_presence_notification(const LLNotificationPtr& notification)
{
	if (!notification || notification->getType() != "notifytip")
	{
		return false;
	}
	const std::string& name = notification->getName();
	return name == "FriendOnlineOffline" || name == "FriendOnline" || name == "FriendOffline";
}
static F32 notify_toast_fade_time()
{
	return llmax(0.35f, TOAST_FADE_TIME);
}
static S32 notify_toast_width()
{
	return llclamp(gSavedSettings.getS32("NotifyBoxWidth"), 260, 400);
}
static bool notify_stack_enabled()
{
	return gSavedSettings.getBOOL("NotifyBoxStack");
}
static S32 count_wrapped_lines(const std::string& utf8message, S32 text_width, const LLFontGL* font)
{
	if (!font || text_width <= 8 || utf8message.empty())
	{
		return 1;
	}
	LLWString message = utf8str_to_wstring(utf8message);
	const S32 message_len = static_cast<S32>(message.length());
	const llwchar* wchars = message.c_str();
	const llwchar* start = wchars;
	bool done = false;
	S32 line_count = 0;
	while (!done)
	{
		const llwchar* end = start;
		while (*end != 0 && *end != '\n')
		{
			++end;
		}
		if (*end == 0)
		{
			end = wchars + message_len;
			done = true;
		}
		S32 remaining = static_cast<S32>(end - start);
		if (remaining <= 0)
		{
			++line_count;
		}
		while (remaining > 0)
		{
			S32 drawn = font->maxDrawableChars(start, (F32)text_width, remaining, LLFontGL::WORD_BOUNDARY_IF_POSSIBLE);
			if (drawn <= 0)
			{
				drawn = 1;
			}
			++line_count;
			start += drawn;
			remaining -= drawn;
		}
		start = end + 1;
		if (start > wchars + message_len)
		{
			done = true;
		}
	}
	return llmax(1, line_count);
}
void drawAllynToastChrome(S32 width, S32 height, const LLColor4& fill, bool focused)
{
	LLUIImagePtr chrome = LLUI::getUIImage("Rounded_Square");
	LLColor4 edge = fill;
	edge.mV[VW] = llmin(1.f, fill.mV[VW] + (focused ? 0.18f : 0.08f));
	if (chrome.notNull())
	{
		chrome->drawSolid(0, 0, width, height, edge);
		chrome->drawSolid(1, 1, width - 2, height - 2, fill);
	}
	else
	{
		gl_rect_2d(0, height, width, 0, fill);
	}
}
static bool is_object_offer_notification(const LLNotificationPtr& notification)
{
	return notification && notification->getName() == "ObjectGiveItem";
}
static bool is_object_offer_box(LLView* view)
{
	LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view);
	return box && is_object_offer_notification(box->getNotification());
}
static LLButton* create_toast_hide_button(LLView* parent, const std::string& tool_tip, const boost::function<void()>& on_hide)
{
	if (!parent)
	{
		return NULL;
	}
	const S32 width = parent->getRect().getWidth();
	const S32 height = parent->getRect().getHeight();
	const S32 right = width - 4;
	const S32 top = height - 3;
	LLRect hide_rect(right - TOAST_CLOSE, top, right, top - TOAST_CLOSE);
	LLButton* hide = new LLButton(std::string("hide"),
								  hide_rect,
								  std::string("UIImgBtnCloseActiveUUID"),
								  std::string("UIImgBtnCloseActiveUUID"),
								  LLStringUtil::null,
								  [on_hide](LLUICtrl*, const LLSD&)
								  {
									  if (on_hide)
									  {
										  on_hide();
									  }
								  });
	hide->setScaleImage(TRUE);
	hide->setToolTip(tool_tip);
	hide->setTabStop(FALSE);
	hide->setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
	parent->addChild(hide);
	return hide;
}
S32 sNotifyBoxCount = 0;
static const LLFontGL* sFont = NULL;
void chat_notification(const LLNotificationPtr notification)
{
	if (gSavedSettings.getBOOL("HideNotificationsInChat")) return;
	LLChat chat(notification->getMessage());
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	if (rlv_handler_t::isEnabled())
		chat.mRlvLocFiltered = chat.mRlvNamesFiltered = true;
	LLFloaterChat::getInstance()->addChatHistory(chat);
}
void LLNotifyBox::initClass()
{
	sFont = LLFontGL::getFontSansSerif();
	LLNotificationChannel::buildChannel("Notifications", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notify"));
	LLNotificationChannel::buildChannel("NotificationTips", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notifytip"));
	LLNotifications::instance().getChannel("Notifications")->connectChanged(&LLNotifyBox::onNotification);
	LLNotifications::instance().getChannel("NotificationTips")->connectChanged(&LLNotifyBox::onNotification);
	if (gSavedSettings.getS32("NotifyBoxWidth") == 350)
	{
		gSavedSettings.setS32("NotifyBoxWidth", 305);
	}
}
bool LLNotifyBox::onNotification(const LLSD& notify)
{
	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
	if (!notification) return false;
	if (notify["sigtype"].asString() == "add" || notify["sigtype"].asString() == "change")
	{
		if (notification->getPayload().has("SUPPRESS_TOAST"))
		{
			chat_notification(notification);
			return false;
		}
		const bool startup_sync = isStartupSync(notification);
		if (startup_sync && is_friend_presence_notification(notification))
		{
			chat_notification(notification);
			return false;
		}
		LLNotifyBox* boxp = LLInstanceTracker<LLNotifyBox, LLUUID>::getInstance(notification->getID());
		if (boxp && !boxp->isDead())
		{
			const bool summary_update = notification->getID() == sStartupSummaryID && notify["sigtype"].asString() == "change";
			if (!summary_update && !(startup_sync && boxp->isSunkToWell()))
			{
				gNotifyBoxView->bringToFront(notification->getID());
			}
		}
		else
		{
			LLNotifyBox* box = new LLNotifyBox(notification);
			if (archiveDuringStartup(notification, box))
			{
				box->sinkToWell();
			}
			gNotifyBoxView->addChild(box);
		}
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		LLNotifyBox* boxp = LLInstanceTracker<LLNotifyBox, LLUUID>::getInstance(notification->getID());
		if (boxp && !boxp->isDead())
		{
			boxp->close();
		}
	}
	return false;
}
LLNotifyBox::LLNotifyBox(LLNotificationPtr notification)
	:	LLPanel(notification->getName(), LLRect(), BORDER_NO),
		LLEventTimer(notify_toast_duration()),
		LLInstanceTracker<LLNotifyBox, LLUUID>(notification->getID()),
	  mNotification(notification),
	  mIsTip(notification->getType() == "notifytip"),
	  mAnimating(true),
	  mSunkToWell(false),
	  mFading(false),
	  mNextBtn(NULL),
	  mHideBtn(NULL),
	  mNumOptions(0),
	  mNumButtons(0),
	  mAddedDefaultBtn(false),
	  mUserInputBox(NULL)
{
	std::string edit_text_name;
	std::string edit_text_contents;
	const std::string& message(notification->getMessage());
	setFocusRoot(!mIsTip);
	mIsCaution = notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH;
	LLNotificationFormPtr form(notification->getForm());
	mNumOptions = form->getNumElements();
	bool is_textbox = form->getElement("message").isDefined();
	bool layout_script_dialog(notification->getName() == "ScriptDialog" || notification->getName() == "ScriptDialogGroup");
	const bool friend_presence = isFriendPresenceTip();
	const bool has_ignore = (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE
		|| form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE);
	std::string full_message = message;
	if (is_textbox || layout_script_dialog)
	{
		const std::string script_message = notification->getSubstitutions()["SCRIPT_MESSAGE"].asString();
		if (!script_message.empty())
		{
			if (!full_message.empty())
			{
				full_message += "\n";
			}
			full_message += script_message;
		}
	}
	S32 option_slots = is_textbox ? 10 : mNumOptions;
	if (option_slots < 1)
	{
		option_slots = 1;
	}
	if (layout_script_dialog)
	{
		option_slots += 1;
	}
	S32 button_rows = mIsTip ? 0 : llmax(1, (option_slots + 2) / 3);
	S32 button_area = mIsTip ? 0 : (BOTTOM_PAD + button_rows * (BTN_HEIGHT + VPAD));
	if (has_ignore)
	{
		button_area += BTN_HEIGHT;
	}
	if (is_textbox)
	{
		S32 input_rows = layout_script_dialog ? 2 : 1;
		button_area += input_rows * (BTN_HEIGHT + VPAD) + BTN_HEIGHT;
	}
	LLRect rect;
	if (friend_presence)
	{
		rect = getFriendPresenceTipRect(message);
	}
	else
	{
		const S32 width = notify_toast_width();
		const S32 text_width = width - TOAST_PAD * 2 - TOAST_CLOSE;
		const S32 line_h = sFont ? llceil(sFont->getLineHeight()) : LINE;
		S32 text_h = count_wrapped_lines(full_message, text_width, sFont) * line_h;
		S32 height = TOAST_PAD + text_h + VPAD + button_area;
		if (mIsTip)
		{
			height = llclamp(height, TOAST_MIN_TIP_HEIGHT, TOAST_MAX_HEIGHT);
		}
		else
		{
			height = llclamp(height, TOAST_MIN_HEIGHT, TOAST_MAX_HEIGHT);
		}
		const S32 top = gNotifyBoxView->getRect().getHeight();
		const S32 right = gNotifyBoxView->getRect().getWidth();
		rect = LLRect(right - width, top, right, top - height);
	}
	setRect(rect);
	mNaturalWidth = rect.getWidth();
	setFollows(friend_presence ? (FOLLOWS_TOP | FOLLOWS_LEFT) : (FOLLOWS_TOP | FOLLOWS_RIGHT));
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);
	if (friend_presence)
	{
		const LLFontGL* font = LLFontGL::getFontSansSerifSmall();
		const S32 pad_x = 10;
		const S32 pad_y = 4;
		LLRect text_rect(pad_x, getRect().getHeight() - pad_y, getRect().getWidth() - pad_x - TOAST_CLOSE, pad_y);
		LLTextBox* label = new LLTextBox(std::string("presence"), text_rect, message, font, FALSE);
		label->setColor(gColors.getColor("NotifyTextColor"));
		label->setUseEllipses(TRUE);
		label->setMouseOpaque(FALSE);
		addChild(label);
		chat_notification(mNotification);
	}
	else
	{
	const S32 TOP = getRect().getHeight() - TOAST_PAD;
	S32 x = TOAST_PAD;
	S32 y = TOP;
	{
		const S32 BTN_TOP = mIsTip ? TOAST_PAD : button_area;
		const S32 MAX_LENGTH = 512 + 20 + DB_FIRST_NAME_BUF_SIZE + DB_LAST_NAME_BUF_SIZE + DB_INV_ITEM_NAME_BUF_SIZE;
		const S32 text_right = getRect().getWidth() - TOAST_PAD;
		const S32 text_bottom = mIsTip ? TOAST_PAD : (BTN_TOP + 4);
		mText = new LLTextEditor(std::string("box"), LLRect(x, y, text_right, text_bottom), MAX_LENGTH, LLStringUtil::null, sFont, FALSE, true);
		mText->setWordWrap(TRUE);
		mText->setMouseOpaque(TRUE);
		mText->setBorderVisible(FALSE);
		mText->setTakesNonScrollClicks(TRUE);
		mText->setHideScrollbarForShortDocs(TRUE);
		mText->setReadOnlyBgColor(LLColor4::transparent);
		mText->setWriteableBgColor(LLColor4::transparent);
		auto text_color = gColors.getColor(mIsCaution ? "NotifyCautionWarnColor" : "NotifyTextColor");
		LLStyleSP style = new LLStyle(true, text_color, LLStringUtil::null);
		style->mBold = mIsCaution && !mIsTip;
		mText->setReadOnlyFgColor(text_color);
		if (!mIsCaution || !mIsTip)
			mText->setLinkColor(new LLColor4(lerp(text_color, gSavedSettings.getColor4("HTMLLinkColor"), 0.4f)));
		mText->setTabStop(FALSE);
		mTextStyle = style;
		mText->appendText(message,false,false,style);
		if (is_textbox || layout_script_dialog)
			mText->appendText(notification->getSubstitutions()["SCRIPT_MESSAGE"], false, true, style, false);
		addChild(mText);
	}
	if (mIsTip)
	{
		if (!notification->getPayload().has(STARTUP_SUMMARY_REPOST))
		{
			chat_notification(mNotification);
		}
	}
	else
	{
		mNextBtn = new LLButton(std::string("next"),
						   LLRect(getRect().getWidth()-26, BOTTOM_PAD + 20, getRect().getWidth()-2, BOTTOM_PAD),
						   std::string("notify_next.png"),
						   std::string("notify_next.png"),
						   LLStringUtil::null,
						   boost::bind(&LLNotifyBox::moveToBack, this, true),
						   sFont);
		mNextBtn->setScaleImage(TRUE);
		mNextBtn->setToolTip(LLTrans::getString("next"));
		addChild(mNextBtn);
		if (notify_stack_enabled())
		{
			mNextBtn->setVisible(FALSE);
		}
		for (S32 i = 0; i < mNumOptions; i++)
		{
			LLSD form_element = form->getElement(i);
			std::string element_type = form_element["type"].asString();
			if (element_type == "button")
			{
				addButton(form_element["name"].asString(), form_element["text"].asString(), TRUE, form_element["default"].asBoolean(), layout_script_dialog);
			}
			else if (element_type == "input")
			{
				edit_text_contents = form_element["value"].asString();
				edit_text_name = form_element["name"].asString();
			}
		}
		if (is_textbox)
		{
			S32 input_rows = layout_script_dialog ? 2 : 1;
			LLRect input_rect;
			input_rect.setOriginAndSize(x, BOTTOM_PAD + input_rows * (BTN_HEIGHT + VPAD),
										getRect().getWidth() - TOAST_PAD * 2, input_rows * (BTN_HEIGHT + VPAD) + BTN_HEIGHT);
			mUserInputBox = new LLTextEditor(edit_text_name, input_rect, 254,
											 edit_text_contents, sFont, FALSE);
			mUserInputBox->setBorderVisible(TRUE);
			mUserInputBox->setTakesNonScrollClicks(TRUE);
			mUserInputBox->setHideScrollbarForShortDocs(TRUE);
			mUserInputBox->setWordWrap(TRUE);
			mUserInputBox->setTabsToNextField(FALSE);
			mUserInputBox->setCommitOnFocusLost(FALSE);
			mUserInputBox->setAcceptCallingCardNames(FALSE);
			mUserInputBox->setHandleEditKeysDirectly(TRUE);
			addChild(mUserInputBox, -1);
		}
		else
		{
			setIsChrome(TRUE);
		}
		if (mNumButtons == 0)
		{
			addButton("OK", "OK", false, true, layout_script_dialog);
			mAddedDefaultBtn = true;
		}
		std::string check_title;
		if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE)
		{
			check_title = LLNotificationTemplates::instance().getGlobalString("skipnexttime");
		}
		else if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
		{
			check_title = LLNotificationTemplates::instance().getGlobalString("alwayschoose");
		}
		if (!check_title.empty())
		{
			const LLFontGL* font = LLResMgr::getInstance()->getRes(LLFONT_SANSSERIF);
			S32 line_height = llfloor(font->getLineHeight() + 0.99f);
			S32 max_msg_width = getRect().getWidth() - TOAST_PAD * 2;
			S32 check_width = S32(font->getWidth(check_title) + 0.99f) + 16;
			max_msg_width = llmax(max_msg_width, check_width);
			S32 msg_x = TOAST_PAD;
			LLRect check_rect;
			check_rect.setOriginAndSize(msg_x, BOTTOM_PAD + BTN_HEIGHT + VPAD*2 + (BTN_HEIGHT + VPAD) * (mNumButtons / 3),
				max_msg_width, line_height);
			LLCheckboxCtrl* check = new LLCheckboxCtrl(std::string("check"), check_rect, check_title, font,
				[this](LLUICtrl* ctrl, const LLSD& param)
				{
						this->mNotification->setIgnored(ctrl->getValue());
				});
			check->setEnabledColor(LLUI::sColorsGroup->getColor(mIsCaution ? "AlertCautionTextColor" : "AlertTextColor"));
			if (mIsCaution)
			{
				check->setButtonColor(LLUI::sColorsGroup->getColor("ButtonCautionImageColor"));
			}
			addChild(check);
		}
		if (++sNotifyBoxCount <= 0)
			LL_WARNS() << "A notification was mishandled. sNotifyBoxCount = " << sNotifyBoxCount << LL_ENDL;
		else if (mNextBtn && (sNotifyBoxCount == 1 || notify_stack_enabled()))
			mNextBtn->setVisible(false);
	}
	fitTextToContent(true);
	}
	if (is_object_offer_notification(mNotification))
	{
		mHideBtn = create_toast_hide_button(this, LLTrans::getString("NotifyToastHide"), boost::bind(&LLNotifyBox::hideToWell, this));
	}
	else
	{
		const LLUUID notification_id = mNotification->getID();
		mHideBtn = create_toast_hide_button(this, LLTrans::getString("NotifyWellClose"),
			[notification_id]()
			{
				if (gNotifyBoxView)
				{
					gNotifyBoxView->closeOne(notification_id);
				}
			});
	}
}
LLNotifyBox::~LLNotifyBox()
{
}
bool LLNotifyBox::fitTextToContent(bool force)
{
	if (!mText || !gNotifyBoxView)
	{
		return false;
	}
	const U32 revision = mText->getContentRevision();
	if (!force && mTextFitted && revision == mFittedTextRevision)
	{
		return false;
	}
	mTextFitted = true;
	mFittedTextRevision = revision;
	const S32 available = llmax(0, gNotifyBoxView->getAvailableStackHeight());
	const S32 available_width = llmax(1, gNotifyBoxView->getRect().getWidth());
	bool width_changed = false;
	if (mIsTip)
	{
		const S32 natural_width = mNaturalWidth > 0 ? mNaturalWidth : getRect().getWidth();
		const S32 tip_width = llmin(natural_width, available_width);
		if (tip_width != getRect().getWidth())
		{
			LLRect tip_rect = getRect();
			tip_rect.mLeft = tip_rect.mRight - tip_width;
			setRect(tip_rect);
			const LLRect old_text_rect = mText->getRect();
			mText->reshape(llmax(1, tip_width - TOAST_PAD - old_text_rect.mLeft), old_text_rect.getHeight());
			mText->setOrigin(old_text_rect.mLeft, old_text_rect.mBottom);
			width_changed = true;
		}
	}
	const LLRect text_rect = mText->getRect();
	const S32 text_bottom = text_rect.mBottom;
	const S32 min_height = mIsTip ? TOAST_MIN_TIP_HEIGHT : TOAST_MIN_HEIGHT;
	const S32 max_height = llmax(min_height, available);
	const S32 content_height = mText->getContentHeight();
	S32 height = llclamp(TOAST_PAD + text_bottom + content_height, min_height, max_height);
	S32 top_pad = TOAST_PAD;
	if (height - top_pad - text_bottom < content_height)
	{
		top_pad = llmax(top_pad, TOAST_CLOSE + 4);
	}
	S32 text_height = height - top_pad - text_bottom;
	const S32 min_text_height = mText->getMinContentHeight();
	if (text_height < min_text_height)
	{
		text_height = min_text_height;
		height = top_pad + text_bottom + text_height;
	}
	const bool fits = mIsTip || (height <= available && mScrollArea.getNaturalWidth(this) <= available_width);
	if (!fits)
	{
		if (!mScrollArea.isActive())
		{
			mScrollArea.activate(this, mHideBtn, text_bottom + text_rect.getHeight(), getDefaultButton());
		}
		return fitScrollArea(available, available_width);
	}
	bool restored = false;
	if (mScrollArea.isActive())
	{
		mScrollArea.deactivate(this, mHideBtn);
		restored = true;
	}
	const LLRect current_text_rect = mText->getRect();
	if (!restored && !width_changed && height == getRect().getHeight() && text_height == current_text_rect.getHeight())
	{
		return false;
	}
	LLRect rect = getRect();
	rect.mBottom = rect.mTop - height;
	setRect(rect);
	mText->reshape(current_text_rect.getWidth(), text_height);
	mText->setOrigin(current_text_rect.mLeft, text_bottom);
	placeHideButton();
	return true;
}
bool LLNotifyBox::fitScrollArea(S32 available, S32 available_width)
{
	const LLRect text_rect = mText->getRect();
	const S32 text_height = llmax(mText->getMinContentHeight(), mText->getContentHeight());
	if (text_height != text_rect.getHeight())
	{
		mText->reshape(text_rect.getWidth(), text_height);
	}
	mText->setOrigin(text_rect.mLeft, text_rect.mBottom);
	const bool changed = mScrollArea.layout(this, text_rect.mBottom + text_height, available_width, available);
	placeHideButton();
	return changed;
}
void LLNotifyBox::placeHideButton()
{
	if (!mHideBtn)
	{
		return;
	}
	const LLRect rect = getRect();
	const S32 right = rect.getWidth() - 4;
	const S32 top = rect.getHeight() - 3;
	mHideBtn->setRect(LLRect(right - TOAST_CLOSE, top, right, top - TOAST_CLOSE));
}
void LLNotifyBox::refreshMessage()
{
	if (!mText || !mNotification || isDead())
	{
		return;
	}
	mText->clear();
	mText->appendText(mNotification->getMessage(), false, false, mTextStyle);
	fitTextToContent(true);
	if (gNotifyBoxView)
	{
		gNotifyBoxView->layoutStack();
	}
}
const S32 LLToastScrollArea::TOP_RESERVE = TOAST_CLOSE + 4;
const S32 LLToastScrollArea::MIN_HEIGHT = TOAST_CLOSE + 4 + 2 * SCROLLBAR_SIZE;
S32 LLToastScrollArea::getNaturalWidth(const LLPanel* toast) const
{
	if (mScroller)
	{
		return mNaturalWidth;
	}
	return toast ? toast->getRect().getWidth() : 0;
}
void LLToastScrollArea::moveSavedChildren(LLView* from, LLView* to)
{
	std::vector<const SavedChild*> by_tab_order;
	for (const SavedChild& saved : mSaved)
	{
		LLView* child = saved.mView.get();
		if (child && child->getParent() == from)
		{
			by_tab_order.push_back(&saved);
		}
	}
	std::stable_sort(by_tab_order.begin(), by_tab_order.end(),
		[](const SavedChild* a, const SavedChild* b)
		{
			if (a->mIsCtrl != b->mIsCtrl)
			{
				return !a->mIsCtrl;
			}
			return a->mIsCtrl && a->mOrdinal < b->mOrdinal;
		});
	for (const SavedChild* saved : by_tab_order)
	{
		LLView* child = saved->mView.get();
		from->removeChild(child);
		to->addChild(child, saved->mTabGroup);
	}
	for (const SavedChild& saved : mSaved)
	{
		LLView* child = saved.mView.get();
		if (child && child->getParent() == to)
		{
			to->sendChildToFront(child);
		}
	}
}
void LLToastScrollArea::activate(LLPanel* toast, LLView* keep_on_toast, S32 content_height, LLButton* default_btn)
{
	if (mScroller || !toast)
	{
		return;
	}
	mNaturalWidth = toast->getRect().getWidth();
	mSaved.clear();
	const LLView::child_tab_order_t& order = toast->getCtrlOrder();
	for (LLView::child_list_const_reverse_iter_t it = toast->getChildList()->rbegin(); it != toast->getChildList()->rend(); ++it)
	{
		LLView* child = *it;
		if (!child || child == keep_on_toast)
		{
			continue;
		}
		SavedChild saved;
		saved.mView = child->getHandle();
		saved.mRect = child->getRect();
		if (child->isCtrl())
		{
			LLView::child_tab_order_const_iter_t found = order.find(static_cast<LLUICtrl*>(child));
			if (found != order.end())
			{
				saved.mIsCtrl = true;
				saved.mTabGroup = found->second.first;
				saved.mOrdinal = found->second.second;
			}
		}
		mSaved.push_back(saved);
	}
	mContent = new LLPanel(std::string("toast_content"), LLRect(0, llmax(1, content_height), llmax(1, mNaturalWidth), 0), BORDER_NO);
	mContent->setBackgroundVisible(FALSE);
	mContent->setMouseOpaque(FALSE);
	moveSavedChildren(toast, mContent);
	if (default_btn)
	{
		mContent->setDefaultBtn(default_btn);
	}
	mScroller = new LLScrollContainer(std::string("toast_scroll"), LLRect(0, llmax(1, toast->getRect().getHeight() - TOP_RESERVE), llmax(1, mNaturalWidth), 0), mContent, FALSE);
	mScroller->setBorderVisible(FALSE);
	mScroller->setPassBackToChildren(false);
	mScroller->setReserveScrollCorner(FALSE);
	mScroller->setFollows(FOLLOWS_NONE);
	toast->addChild(mScroller);
	if (keep_on_toast)
	{
		toast->sendChildToFront(keep_on_toast);
	}
	mLastFocus = LLHandle<LLView>();
}
void LLToastScrollArea::deactivate(LLPanel* toast, LLView* keep_on_toast)
{
	if (!mScroller || !toast)
	{
		return;
	}
	if (mContent)
	{
		moveSavedChildren(mContent, toast);
	}
	for (const SavedChild& saved : mSaved)
	{
		LLView* child = saved.mView.get();
		if (!child || child->getParent() != toast || child->getRect() == saved.mRect)
		{
			continue;
		}
		if (child->getRect().getWidth() != saved.mRect.getWidth() || child->getRect().getHeight() != saved.mRect.getHeight())
		{
			child->reshape(saved.mRect.getWidth(), saved.mRect.getHeight());
		}
		child->setOrigin(saved.mRect.mLeft, saved.mRect.mBottom);
	}
	toast->removeChild(mScroller);
	mScroller->die();
	mScroller = nullptr;
	mContent = nullptr;
	mSaved.clear();
	mLastFocus = LLHandle<LLView>();
	if (keep_on_toast)
	{
		toast->sendChildToFront(keep_on_toast);
	}
	const LLRect rect = toast->getRect();
	toast->setRect(LLRect(rect.mRight - mNaturalWidth, rect.mTop, rect.mRight, rect.mBottom));
}
bool LLToastScrollArea::layout(LLPanel* toast, S32 content_height, S32 available_width, S32 available_height)
{
	if (!mScroller || !mContent || !toast)
	{
		return false;
	}
	content_height = llmax(1, content_height);
	const S32 max_width = llmax(1, available_width);
	const S32 max_height = llmax(0, available_height);
	S32 height = content_height + TOP_RESERVE;
	const bool vertical = height > max_height;
	S32 width = mNaturalWidth + (vertical ? SCROLLBAR_SIZE : 0);
	if (width > max_width)
	{
		width = max_width;
		height += SCROLLBAR_SIZE;
	}
	height = llmax(MIN_HEIGHT, llmin(height, max_height));
	width = llmax(1, width);
	const LLRect old_rect = toast->getRect();
	toast->setRect(LLRect(old_rect.mRight - width, old_rect.mTop, old_rect.mRight, old_rect.mTop - height));
	mContent->setRect(LLRect(0, content_height, llmax(1, mNaturalWidth), 0));
	mScroller->reshape(width, llmax(1, height - TOP_RESERVE));
	mScroller->setOrigin(0, 0);
	return width != old_rect.getWidth() || height != old_rect.getHeight();
}
void LLToastScrollArea::revealFocus()
{
	if (!mScroller || !mContent)
	{
		return;
	}
	LLView* focus = dynamic_cast<LLView*>(gFocusMgr.getKeyboardFocus());
	if (!focus || !focus->hasAncestor(mContent))
	{
		mLastFocus = LLHandle<LLView>();
		return;
	}
	LLView* previous = mLastFocus.get();
	if (focus == previous)
	{
		return;
	}
	mLastFocus = focus->getHandle();
	if (!previous)
	{
		return;
	}
	LLRect focus_rect;
	if (!focus->localRectToOtherView(focus->getLocalRect(), &focus_rect, mContent))
	{
		return;
	}
	const LLRect visible_rect = mScroller->getVisibleContentRect();
	LLRect target = focus_rect;
	if (focus_rect.getHeight() > visible_rect.getHeight()
		&& focus_rect.mTop > visible_rect.mBottom && focus_rect.mBottom < visible_rect.mTop)
	{
		target.mTop = llmin(focus_rect.mTop, visible_rect.mTop);
		target.mBottom = llmax(focus_rect.mBottom, visible_rect.mBottom);
	}
	if (focus_rect.getWidth() > visible_rect.getWidth()
		&& focus_rect.mRight > visible_rect.mLeft && focus_rect.mLeft < visible_rect.mRight)
	{
		target.mLeft = llmax(focus_rect.mLeft, visible_rect.mLeft);
		target.mRight = llmin(focus_rect.mRight, visible_rect.mRight);
	}
	mScroller->scrollToShowRect(target);
}
LLButton* LLNotifyBox::addButton(const std::string& name, const std::string& label, bool is_option, bool is_default, bool layout_script_dialog)
{
	S32 btn_width = (mIsCaution || mNumOptions >= 3) ? 84 : TOAST_BTN_WIDTH;
	LLRect btn_rect;
	S32 btn_height= BTN_HEIGHT;
	const LLFontGL* font = sFont;
	S32 ignore_pad = 0;
	S32 button_index = mNumButtons;
	S32 index = button_index;
	S32 x = TOAST_PAD;
	if (layout_script_dialog)
	{
		index = button_index + 1;
		if (button_index == 0 || button_index == 1)
		{
			btn_height = BTN_HEIGHT_SMALL;
			static const LLFontGL* sFontSmall = LLFontGL::getFontSansSerifSmall();
			font = sFontSmall;
			ignore_pad = 10;
		}
	}
	btn_rect.setOriginAndSize(x + (index % 3) * (btn_width+HPAD+HPAD) + ignore_pad,
		BOTTOM_PAD + (index / 3) * (BTN_HEIGHT+VPAD),
		btn_width - 2*ignore_pad,
		btn_height);
	LLButton* btn = new LLButton(name, btn_rect, "", boost::bind(&LLNotifyBox::onClickButton, this, is_option ? name : ""));
	btn->setLabel(label);
	btn->setToolTip(label);
	btn->setFont(font);
	if (mIsCaution)
	{
		btn->setImageColor(LLUI::sColorsGroup->getColor("ButtonCautionImageColor"));
		btn->setDisabledImageColor(LLUI::sColorsGroup->getColor("ButtonCautionImageColor"));
	}
	addChild(btn, -1);
	if (is_default)
		setDefaultBtn(btn);
	mNumButtons++;
	if (!layout_script_dialog)
	{
		std::vector<LLButton*> action_btns;
		for (child_list_const_iter_t iter = getChildList()->begin();
			 iter != getChildList()->end();
			 ++iter)
		{
			LLButton* child_btn = dynamic_cast<LLButton*>(*iter);
			if (!child_btn || child_btn == mHideBtn || child_btn == mNextBtn)
			{
				continue;
			}
			action_btns.push_back(child_btn);
		}
		std::reverse(action_btns.begin(), action_btns.end());

		const S32 count = static_cast<S32>(action_btns.size());
		if (count > 0)
		{
			const S32 gap = 6;
			const S32 cols = llmin(3, count);
			const S32 avail = getRect().getWidth() - TOAST_PAD * 2;
			S32 fitted_width = (avail - (cols - 1) * gap) / cols;
			fitted_width = llclamp(fitted_width, 70, TOAST_BTN_WIDTH);
			S32 left = TOAST_PAD;
			if (count == 1)
			{
				left = (getRect().getWidth() - fitted_width) / 2;
			}
			for (S32 i = 0; i < count; ++i)
			{
				const S32 col = i % 3;
				const S32 row = i / 3;
				const S32 bx = left + col * (fitted_width + gap);
				const S32 by = BOTTOM_PAD + row * (BTN_HEIGHT + VPAD);
				action_btns[i]->setRect(LLRect(bx, by + BTN_HEIGHT, bx + fitted_width, by));
			}
		}
	}
	return btn;
}
BOOL LLNotifyBox::handleMouseUp(S32 x, S32 y, MASK mask)
{
	bool done = LLPanel::handleMouseUp(x, y, mask);
	if (!done && mIsTip)
	{
		mNotification->respond(mNotification->getResponseTemplate(LLNotification::WITH_DEFAULT_BUTTON));
		close();
		return TRUE;
	}
	setFocus(TRUE);
	return done;
}
BOOL LLNotifyBox::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (!LLPanel::handleRightMouseDown(x, y, mask))
		moveToBack(true);
	return true;
}
BOOL LLNotifyBox::handleHover(S32 x, S32 y, MASK mask)
{
	if (mFading)
	{
		mFading = false;
		mPeriod = notify_toast_duration();
		mEventTimer.reset();
	}
	mEventTimer.stop();
	return LLPanel::handleHover(x, y, mask);
}
bool LLNotifyBox::userIsInteracting() const
{
	S32 local_x;
	S32 local_y;
	screenPointToLocal(gViewerWindow->getCurrentMouseX(), gViewerWindow->getCurrentMouseY(), &local_x, &local_y);
	return pointInView(local_x, local_y) ||
		(mText && mText->getActive<LLTextEditor>() == mText && LLMenuGL::sMenuContainer->getVisibleMenu());
}
void LLNotifyBox::draw()
{
	if (!mFading)
	{
		mPeriod = notify_toast_duration();
	}
	if (gTeleportDisplay)
		mEventTimer.stop();
	else if (!mEventTimer.getStarted() && !mSunkToWell && !userIsInteracting())
		mEventTimer.start();
	mScrollArea.revealFocus();
	F32 display_time = mAnimateTimer.getElapsedTimeF32();
	if (mAnimating && display_time < ANIMATION_TIME)
	{
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		LLUI::pushMatrix();
		F32 fraction = display_time / ANIMATION_TIME;
		F32 voffset = (1.f - fraction) * STACK_SLIDE_PIXELS;
		LLUI::translate(0.f, voffset, 0.f);
		drawBackground();
		LLPanel::draw();
		LLUI::popMatrix();
	}
	else
	{
		if (mAnimating)
		{
			mAnimating = false;
			if (!mIsTip && gNotifyBoxView && !gSavedSettings.getBOOL("NotifyBoxStack"))
				gNotifyBoxView->showOnly(this);
		}
		drawBackground();
		LLPanel::draw();
	}
}
void LLNotifyBox::drawBackground() const
{
	static const LLCachedControl<LLColor4> sCautionColor(gColors, "NotifyCautionBoxColor");
	static const LLCachedControl<LLColor4> sColor(gColors, "NotifyBoxColor");
	const bool focused = gFocusMgr.childHasKeyboardFocus(this);
	LLColor4 fill = mIsCaution ? sCautionColor : sColor;
	fill.mV[VW] *= fadeAlpha();
	drawAllynToastChrome(
		getRect().getWidth(),
		getRect().getHeight(),
		fill,
		focused && !mFading);
}
F32 LLNotifyBox::fadeAlpha() const
{
	if (!mFading || mPeriod <= 0.f)
	{
		return 1.f;
	}
	const F32 elapsed = mEventTimer.getElapsedTimeF32();
	const F32 t = 1.f - llclamp(elapsed / mPeriod, 0.f, 1.f);
	return 0.22f + 0.78f * t;
}
void LLNotifyBox::hideToWell()
{
	if (isFriendPresenceTip())
	{
		if (mNotification)
		{
			LLNotifications::instance().cancel(mNotification);
		}
		if (!isDead())
		{
			close();
		}
		return;
	}
	if (!mSunkToWell)
	{
		sinkToWell();
		if (gNotifyBoxView)
		{
			gNotifyBoxView->layoutStack();
		}
	}
}
void LLNotifyBox::close()
{
	bool not_tip = !mIsTip;
	die();
	if (gNotifyBoxView)
	{
		gNotifyBoxView->layoutStack();
	}
	if (not_tip)
	{
		--sNotifyBoxCount;
		if (gNotifyBoxView)
		{
			if (LLNotifyBox* front = gNotifyBoxView->getFirstNontipBox())
			{
				if (!gSavedSettings.getBOOL("NotifyBoxStack"))
				{
					gNotifyBoxView->showOnly(front);
				}
				if (front->getVisible())
				{
					if (LLView* view = front->getDefaultButton())
						view->setFocus(true);
					gFocusMgr.triggerFocusFlash();
				}
			}
		}
	}
}
void LLNotifyBox::format(std::string& msg, const LLStringUtil::format_map_t& args)
{
	LLStringUtil::format_map_t targs = args;
	const LLStringUtil::format_map_t& default_args = LLTrans::getDefaultArgs();
	for (LLStringUtil::format_map_t::const_iterator iter = default_args.begin();
		 iter != default_args.end(); ++iter)
	{
		targs[iter->first] = iter->second;
	}
	LLStringUtil::format(msg, targs);
}
void LLNotifyBox::sinkToWell()
{
	mSunkToWell = true;
	mFading = false;
	mEventTimer.stop();
	setVisible(FALSE);
}
void LLNotifyBox::unsinkFromWell()
{
	mSunkToWell = false;
	mFading = false;
	mPeriod = notify_toast_duration();
	mEventTimer.reset();
	mEventTimer.start();
	setVisible(TRUE);
}
bool LLNotifyBox::isFriendPresenceTip() const
{
	if (!mIsTip || !mNotification)
	{
		return false;
	}
	const std::string& name = mNotification->getName();
	return name == "FriendOnlineOffline" || name == "FriendOnline" || name == "FriendOffline";
}
BOOL LLNotifyBox::tick()
{
	if (isFriendPresenceTip())
	{
		if (!getVisible())
		{
			return FALSE;
		}
		if (mNotification)
		{
			LLNotifications::instance().cancel(mNotification);
		}
		if (!isDead())
		{
			close();
		}
		return FALSE;
	}
	if (getVisible() && !mSunkToWell)
	{
		if (!mFading)
		{
			mFading = true;
			mPeriod = notify_toast_fade_time();
			mEventTimer.reset();
			mEventTimer.start();
			return FALSE;
		}
		sinkToWell();
		if (gNotifyBoxView)
		{
			gNotifyBoxView->layoutStack();
		}
	}
	return FALSE;
}
void LLNotifyBox::setVisible(BOOL visible)
{
	if (visible && isFriendPresenceTip() && !getVisible())
	{
		mPeriod = notify_toast_duration();
		mEventTimer.reset();
		mEventTimer.start();
	}
	if (visible && !mIsTip && mNextBtn)
		mNextBtn->setVisible(sNotifyBoxCount > 1 && !notify_stack_enabled());
	LLPanel::setVisible(visible);
}
void LLNotifyBox::moveToBack(bool getfocus)
{
	gNotifyBoxView->sendChildToBack(this);
	if (!mIsTip && mNextBtn)
	{
		mNextBtn->setVisible(false);
		gNotifyBoxView->layoutStack();
		if (gNotifyBoxView->getChildCount())
			if (LLNotifyBox* front = gNotifyBoxView->getFirstNontipBox())
			{
				if (!gSavedSettings.getBOOL("NotifyBoxStack"))
				{
					gNotifyBoxView->showOnly(front);
				}
				if (getfocus && front->getVisible())
				{
					if (front->mNextBtn)
						front->mNextBtn->setFocus(true);
					gFocusMgr.triggerFocusFlash();
				}
			}
	}
}
LLRect LLNotifyBox::getNotifyRect(S32 num_options, bool layout_script_dialog, bool is_caution)
{
	S32 notify_height = gSavedSettings.getS32("NotifyBoxHeight");
	if (is_caution)
	{
		notify_height = gSavedSettings.getS32("PermissionsCautionNotifyBoxHeight");
	}
	const S32 NOTIFY_WIDTH = notify_toast_width();
	const S32 TOP = gNotifyBoxView->getRect().getHeight();
	const S32 RIGHT = gNotifyBoxView->getRect().getWidth();
	const S32 LEFT = RIGHT - NOTIFY_WIDTH;
	if (num_options < 1)
		num_options = 1;
	if (layout_script_dialog)
		num_options += 1;
	S32 additional_lines = (num_options-1) / 3;
	notify_height += additional_lines * (BTN_HEIGHT + VPAD);
	return LLRect(LEFT, TOP, RIGHT, TOP-notify_height);
}
LLRect LLNotifyBox::getNotifyTipRect(const std::string &utf8message)
{
	LLWString message = utf8str_to_wstring(utf8message);
	S32 message_len = message.length();
	const S32 NOTIFY_WIDTH = notify_toast_width();
	const S32 text_area_width = NOTIFY_WIDTH - TOAST_PAD * 2 - TOAST_CLOSE;
	const llwchar* wchars = message.c_str();
	const llwchar* start = wchars;
	const llwchar* end;
	S32 total_drawn = 0;
	bool done = false;
	S32 line_count;
	for (line_count = 2; !done; ++line_count)
	{
		for (end = start; *end != 0 && *end != '\n'; end++);
		if (*end == 0)
		{
			end = wchars + message_len;
			done = true;
		}
		for (S32 remaining = end - start; remaining;)
		{
			S32 drawn = sFont->maxDrawableChars(start, (F32)text_area_width, remaining, LLFontGL::WORD_BOUNDARY_IF_POSSIBLE);
			if (0 == drawn)
			{
				drawn = 1;
			}
			total_drawn += drawn;
			start += drawn;
			remaining -= drawn;
			if (total_drawn < message_len)
			{
				if (wchars[ total_drawn ] != '\n')
				{
					line_count++;
				}
			}
			else
			{
				done = true;
			}
		}
		total_drawn++;
		start = ++end;
	}
	const S32 MIN_NOTIFY_HEIGHT = TOAST_MIN_TIP_HEIGHT;
	const S32 MAX_NOTIFY_HEIGHT = TOAST_MAX_HEIGHT;
	S32 notify_height = llceil((F32) (line_count+1) * sFont->getLineHeight());
	notify_height += TOAST_PAD * 2;
	notify_height = llclamp(notify_height, MIN_NOTIFY_HEIGHT, MAX_NOTIFY_HEIGHT);
	const S32 TOP = gNotifyBoxView->getRect().getHeight();
	const S32 RIGHT = gNotifyBoxView->getRect().getWidth();
	const S32 LEFT = RIGHT - NOTIFY_WIDTH;
	return LLRect(LEFT, TOP, RIGHT, TOP-notify_height);
}
LLRect LLNotifyBox::getFriendPresenceTipRect(const std::string &utf8message)
{
	const LLFontGL* font = LLFontGL::getFontSansSerifSmall();
	const S32 pad_x = 10;
	const S32 pad_y = 5;
	const S32 text_w = font ? font->getWidth(utf8message) : 80;
	const S32 text_h = font ? font->getLineHeight() : 14;
	const S32 width = llclamp(text_w + pad_x * 2 + TOAST_CLOSE, 160, 320);
	const S32 height = text_h + pad_y * 2;
	const S32 TOP = gNotifyBoxView->getRect().getHeight();
	const S32 LEFT = 8;
	return LLRect(LEFT, TOP, LEFT + width, TOP - height);
}
void LLNotifyBox::onClickButton(const std::string name)
{
	LLSD response = mNotification->getResponseTemplate();
	if (!mAddedDefaultBtn && !name.empty())
	{
		response[name] = true;
	}
	if (mUserInputBox)
	{
		response[mUserInputBox->getName()] = mUserInputBox->getValue();
	}
	mNotification->respond(response);
}
LLNotifyBoxView::LLNotifyBoxView(const std::string& name, const LLRect& rect, BOOL mouse_opaque, U32 follows)
	: LLUICtrl(name,rect,mouse_opaque,NULL,follows)
{
}
bool LLNotifyBoxView::addChild(LLView* view, S32 tab_group)
{
	bool ok = LLUICtrl::addChild(view, tab_group);
	layoutStack();
	return ok;
}
void LLNotifyBoxView::removeChild(LLView* view)
{
	LLUICtrl::removeChild(view);
	if (!mInDeleteAll)
	{
		layoutStack();
	}
}
void LLNotifyBoxView::draw()
{
	LLNotifyBox::flushStartupSummary();
	const S32 available = getAvailableStackHeight();
	const S32 available_width = getRect().getWidth();
	const bool space_changed = available != mLastAvailableHeight || available_width != mLastAvailableWidth;
	mLastAvailableHeight = available;
	mLastAvailableWidth = available_width;
	bool relayout = false;
	LLView::child_list_t boxes(*getChildList());
	for (LLView* view : boxes)
	{
		if (!view || view->isDead())
		{
			continue;
		}
		if (isGroupNotifyBox(view))
		{
			LLGroupNotifyBox* group = dynamic_cast<LLGroupNotifyBox*>(view);
			if (space_changed && group && group->fitToArea(available))
			{
				relayout = true;
			}
			continue;
		}
		if (LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view))
		{
			if (box->fitTextToContent(space_changed))
			{
				relayout = true;
			}
		}
	}
	if (relayout || space_changed)
	{
		layoutStack();
	}
	LLUICtrl::draw();
}
void LLNotifyBoxView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent);
	layoutStack();
}
LLNotifyBox* LLNotifyBoxView::getFirstNontipBox() const
{
	for(child_list_const_iter_t iter = getChildList()->begin();
			iter != getChildList()->end();
			iter++)
	{
		if (isGroupNotifyBox(*iter))
			continue;
		LLNotifyBox* box = static_cast<LLNotifyBox*>(*iter);
		if (!box->isTip() && !box->isDead())
			return box;
	}
	return NULL;
}
void LLNotifyBoxView::showOnly(LLView* view)
{
	if (!dynamic_cast<LLNotifyBox*>(view)) return;
	for(child_list_const_iter_t iter = getChildList()->begin();
			iter != getChildList()->end();
			iter++)
	{
		if (view == (*iter)) continue;
		LLView* other(*iter);
		if (isGroupNotifyBox(other) || !other->getVisible())
			continue;
		if (!static_cast<LLNotifyBox*>(other)->isTip())
			other->setVisible(false);
	}
	for (LLView* other : *getChildList())
	{
		if (other != view && !isGroupNotifyBox(other) && !static_cast<LLNotifyBox*>(other)->isTip())
		{
			takeSpaceHidden(other);
		}
	}
	takeSpaceHidden(view);
	view->setVisible(true);
	sendChildToFront(view);
	layoutStack();
}
bool LLNotifyBoxView::takeSpaceHidden(LLView* view)
{
	for (std::vector<LLHandle<LLView> >::iterator it = mSpaceHidden.begin(); it != mSpaceHidden.end(); ++it)
	{
		if (it->get() == view)
		{
			mSpaceHidden.erase(it);
			return true;
		}
	}
	return false;
}
void LLNotifyBoxView::deleteAllChildren()
{
	mInDeleteAll = true;
	LLUICtrl::deleteAllChildren();
	mInDeleteAll = false;
	sNotifyBoxCount = 0;
	mOverflowCount = 0;
	refreshWell();
}
static void close_notify_box(LLView* view);
void LLNotifyBoxView::purgeMessagesMatching(const Matcher& matcher)
{
	LLView::child_list_t notification_queue(*getChildList());
	for(LLView::child_list_iter_t iter = notification_queue.begin();
		iter != notification_queue.end();
		iter++)
	{
		if (isGroupNotifyBox(*iter))
			continue;
		LLNotifyBox* notification = static_cast<LLNotifyBox*>(*iter);
		if (!notification->isDead() && matcher.matches(notification->getNotification()))
		{
			close_notify_box(notification);
		}
	}
	layoutStack();
}
bool LLNotifyBoxView::isGroupNotifyBox(const LLView* view) const
{
	return view && view->getName() == "groupnotify";
}
	bool LLNotifyBoxView::stackEnabled() const
{
	return notify_stack_enabled();
}
bool LLNotifyBoxView::isStackable(LLView* view) const
{
	if (!view || view->isDead())
	{
		return false;
	}
	if (isGroupNotifyBox(view))
	{
		return true;
	}
	if (dynamic_cast<LLNotifyBox*>(view))
	{
		return true;
	}
	return false;
}
bool LLNotifyBoxView::isSunkToWell(LLView* view) const
{
	if (LLGroupNotifyBox* group = dynamic_cast<LLGroupNotifyBox*>(view))
	{
		return group->isSunkToWell();
	}
	if (LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view))
	{
		return box->isSunkToWell();
	}
	return false;
}
bool LLNotifyBoxView::belongsInWell(LLView* view) const
{
	if (LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view))
	{
		return !box->isFriendPresenceTip();
	}
	return true;
}
LLUUID LLNotifyBoxView::getBoxID(LLView* view) const
{
	if (LLGroupNotifyBox* group = dynamic_cast<LLGroupNotifyBox*>(view))
	{
		return group->getWellID();
	}
	if (LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view))
	{
		if (box->getNotification())
		{
			return box->getNotification()->getID();
		}
	}
	return LLUUID::null;
}
static std::string well_first_line(const std::string& text)
{
	std::string t = text;
	LLStringUtil::replaceChar(t, '\r', '\n');
	size_t start = 0;
	while (start < t.size())
	{
		const size_t nl = t.find('\n', start);
		std::string line = (nl == std::string::npos) ? t.substr(start) : t.substr(start, nl - start);
		LLStringUtil::trim(line);
		if (!line.empty())
		{
			return line;
		}
		if (nl == std::string::npos)
		{
			break;
		}
		start = nl + 1;
	}
	return LLStringUtil::null;
}
static bool well_is_unreadable_id(const std::string& s)
{
	std::string t = s;
	LLStringUtil::trim(t);
	if (t.empty() || t.find("://") != std::string::npos)
	{
		return true;
	}
	return LLUUID::validate(t);
}
static std::string well_replace_urls_with_labels(const std::string& text)
{
	std::string src = text;
	std::string out;
	LLUrlMatch match;
	const std::string loading = LLTrans::getString("LoadingData");
	while (!src.empty() && LLUrlRegistry::instance().findUrl(src, match))
	{
		const U32 start = match.getStart();
		const U32 length = match.getEnd() + 1 - start;
		if (start > 0)
		{
			out += src.substr(0, start);
		}
		const std::string label = match.getLabel();
		if (!label.empty()
			&& label != loading
			&& label.find("://") == std::string::npos
			&& !LLUUID::validate(label))
		{
			out += label;
		}
		src.erase(0, start + length);
	}
	out += src;
	return out;
}
static std::string well_readable_sub(const LLSD& subs, const char* key)
{
	if (!subs.has(key))
	{
		return LLStringUtil::null;
	}
	std::string value = well_replace_urls_with_labels(subs[key].asString());
	LLStringUtil::trim(value);
	if (well_is_unreadable_id(value))
	{
		return LLStringUtil::null;
	}
	return value;
}
static std::string well_cleanup_title(std::string title)
{
	LLStringUtil::replaceChar(title, '\n', ' ');
	LLStringUtil::replaceChar(title, '\r', ' ');
	LLStringUtil::replaceChar(title, '\t', ' ');
	std::string collapsed;
	collapsed.reserve(title.size());
	bool prev_space = false;
	for (const char c : title)
	{
		if (c == ' ')
		{
			if (!prev_space)
			{
				collapsed += c;
			}
			prev_space = true;
		}
		else
		{
			collapsed += c;
			prev_space = false;
		}
	}
	LLStringUtil::trim(collapsed);
	if (collapsed.size() > 80)
	{
		collapsed = utf8str_truncate(collapsed, 77) + "...";
	}
	return collapsed;
}
std::string LLNotifyBoxView::getBoxTitle(LLView* view) const
{
	std::string title;
	if (LLGroupNotifyBox* group = dynamic_cast<LLGroupNotifyBox*>(view))
	{
		title = group->getWellTitle();
	}
	else if (LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view))
	{
		if (LLNotificationPtr notification = box->getNotification())
		{
			const LLSD& payload = notification->getPayload();
			const LLSD& subs = notification->getSubstitutions();
			std::string object_name;
			if (payload.has("object_name"))
			{
				object_name = payload["object_name"].asString();
				LLStringUtil::trim(object_name);
			}
			if (well_is_unreadable_id(object_name))
			{
				object_name = well_readable_sub(subs, "TITLE");
			}
			if (well_is_unreadable_id(object_name))
			{
				object_name = well_readable_sub(subs, "OBJECTNAME");
			}
			if (well_is_unreadable_id(object_name))
			{
				object_name = well_readable_sub(subs, "OBJECTFROMNAME");
			}
			std::string detail;
			if (subs.has("SCRIPT_MESSAGE"))
			{
				detail = well_first_line(subs["SCRIPT_MESSAGE"].asString());
			}
			if (detail.empty())
			{
				const std::string label = well_replace_urls_with_labels(notification->getLabel());
				if (!well_is_unreadable_id(label))
				{
					detail = label;
				}
			}
			if (!well_is_unreadable_id(object_name))
			{
				title = object_name;
				if (!detail.empty() && detail != object_name && !well_is_unreadable_id(detail))
				{
					title += ": ";
					title += detail;
				}
			}
			else if (!well_is_unreadable_id(detail))
			{
				title = detail;
			}
			else
			{
				title = well_replace_urls_with_labels(notification->getMessage());
				if (well_is_unreadable_id(title))
				{
					title = well_replace_urls_with_labels(notification->getLabel());
				}
			}
		}
	}
	return well_cleanup_title(title);
}
S32 LLNotifyBoxView::getStackTop() const
{
	S32 top = getRect().getHeight();
	if (gNavigationBar)
	{
		top -= gNavigationBar->getOccupiedHeight();
	}
	if (gNotifyIconBar)
	{
		top -= gNotifyIconBar->getOccupiedHeight();
	}
	return top - STACK_TOP_PAD;
}
S32 LLNotifyBoxView::getStackBottom() const
{
	return 8;
}
void LLNotifyBoxView::layoutStack()
{
	if (mInLayout || mInDeleteAll)
	{
		return;
	}
	mInLayout = true;
	const bool stack = stackEnabled();
	if (stack)
	{
		mSpaceHidden.clear();
	}
	else
	{
		mSpaceHidden.erase(std::remove_if(mSpaceHidden.begin(), mSpaceHidden.end(),
			[](const LLHandle<LLView>& handle)
			{
				LLView* view = handle.get();
				return !view || view->isDead();
			}),
			mSpaceHidden.end());
	}
	const S32 min_bottom = getStackBottom();
	const S32 stack_top = getStackTop();
	S32 cursor_right = stack_top;
	S32 cursor_left = stack_top;
	S32 overflow = 0;
	for (child_list_const_iter_t iter = getChildList()->begin();
		 iter != getChildList()->end();
		 ++iter)
	{
		LLView* view = *iter;
		if (!isStackable(view))
		{
			continue;
		}
		if (isSunkToWell(view))
		{
			takeSpaceHidden(view);
			view->setVisible(FALSE);
			if (belongsInWell(view))
			{
				++overflow;
			}
			continue;
		}
		const bool left_stack = dynamic_cast<LLNotifyBox*>(view)
			&& static_cast<LLNotifyBox*>(view)->isFriendPresenceTip();
		S32& cursor = left_stack ? cursor_left : cursor_right;
		const S32 height = view->getRect().getHeight();
		const S32 width = view->getRect().getWidth();
		const bool fits = (cursor - height) >= min_bottom;
		if (stack)
		{
			if (!fits && view->getVisible())
			{
				view->setVisible(FALSE);
			}
			else if (fits)
			{
				view->setVisible(TRUE);
			}
		}
		else if (fits)
		{
			if (!view->getVisible() && takeSpaceHidden(view))
			{
				view->setVisible(TRUE);
			}
		}
		else if (view->getVisible())
		{
			view->setVisible(FALSE);
			mSpaceHidden.push_back(view->getHandle());
		}
		if (!view->getVisible())
		{
			if (belongsInWell(view))
			{
				++overflow;
			}
			continue;
		}
		if (!fits && stack)
		{
			view->setVisible(FALSE);
			if (belongsInWell(view))
			{
				++overflow;
			}
			continue;
		}
		const S32 top = cursor;
		const S32 bottom = top - height;
		if (left_stack)
		{
			const S32 left = 8;
			view->setRect(LLRect(left, top, left + width, bottom));
		}
		else
		{
			const S32 right = getRect().getWidth();
			view->setRect(LLRect(right - width, top, right, bottom));
		}
		cursor = bottom - STACK_GAP;
	}
	mOverflowCount = overflow;
	refreshWell();
	mInLayout = false;
}
void LLNotifyBoxView::bringToFront(const LLUUID& id)
{
	if (id.isNull())
	{
		return;
	}
	for (child_list_const_iter_t iter = getChildList()->begin();
		 iter != getChildList()->end();
		 ++iter)
	{
		LLView* view = *iter;
		if (!isStackable(view) || getBoxID(view) != id)
		{
			continue;
		}
		if (LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view))
		{
			box->unsinkFromWell();
		}
		else if (LLGroupNotifyBox* group = dynamic_cast<LLGroupNotifyBox*>(view))
		{
			group->unsinkFromWell();
		}
		if (stackEnabled())
		{
			sendChildToFront(view);
			view->setVisible(TRUE);
			layoutStack();
		}
		else if (dynamic_cast<LLNotifyBox*>(view))
		{
			showOnly(view);
		}
		else
		{
			sendChildToFront(view);
			view->setVisible(TRUE);
			layoutStack();
		}
		return;
	}
}
S32 LLNotifyBoxView::getOverflowCount() const
{
	return mOverflowCount;
}
S32 LLNotifyBoxView::getAvailableStackHeight() const
{
	return getStackTop() - getStackBottom();
}
static void close_notify_box(LLView* view)
{
	if (LLGroupNotifyBox* group = dynamic_cast<LLGroupNotifyBox*>(view))
	{
		group->close();
		return;
	}
	if (LLNotifyBox* box = dynamic_cast<LLNotifyBox*>(view))
	{
		if (box->isDead() || is_object_offer_box(box))
		{
			return;
		}
		if (LLNotificationPtr notification = box->getNotification())
		{
			LLNotifications::instance().cancel(notification);
		}
		if (!box->isDead())
		{
			box->close();
		}
	}
}
void LLNotifyBoxView::closeAll()
{
	LLView::child_list_t boxes(*getChildList());
	for (LLView* view : boxes)
	{
		if (isStackable(view) && !is_object_offer_box(view))
		{
			close_notify_box(view);
		}
	}
	layoutStack();
}
void LLNotifyBoxView::closeOne(const LLUUID& id)
{
	if (id.isNull())
	{
		return;
	}
	LLView::child_list_t boxes(*getChildList());
	for (LLView* view : boxes)
	{
		if (!isStackable(view) || getBoxID(view) != id)
		{
			continue;
		}
		if (is_object_offer_box(view))
		{
			bringToFront(id);
			return;
		}
		close_notify_box(view);
		break;
	}
	layoutStack();
}
void LLNotifyBoxView::refreshWell()
{
	if (LLFloaterNotifyWell* well = LLFloaterNotifyWell::findInstance())
	{
		std::vector<std::pair<LLUUID, std::string> > items;
		for (child_list_const_iter_t iter = getChildList()->begin();
			 iter != getChildList()->end();
			 ++iter)
		{
			LLView* view = *iter;
			if (!isStackable(view) || view->getVisible() || !belongsInWell(view))
			{
				continue;
			}
			items.push_back(std::make_pair(getBoxID(view), getBoxTitle(view)));
		}
		well->refreshItems(items);
	}
}
