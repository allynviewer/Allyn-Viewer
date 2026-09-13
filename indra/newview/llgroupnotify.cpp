/** 
 * @file llgroupnotify.cpp
 * @brief Non-blocking notification that doesn't take keyboard focus.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "llgroupnotify.h"
#include "llfocusmgr.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "lliconctrl.h"
#include "llfloaterchat.h"
#include "llgroupactions.h"
#include "llnotify.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "lluiconstants.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llinventoryicon.h"
#include "llinventory.h"
#include "lltrans.h"
#include "llviewerwindow.h"
#include "llglheaders.h"
#include "llagent.h"
const F32 ANIMATION_TIME = 0.333f;
static S32 sGroupNotifyBoxCount = 0;
bool is_openable(LLAssetType::EType type)
{
	switch(type)
	{
	case LLAssetType::AT_LANDMARK:
	case LLAssetType::AT_NOTECARD:
	case LLAssetType::AT_IMAGE_JPEG:
	case LLAssetType::AT_IMAGE_TGA:
	case LLAssetType::AT_TEXTURE:
	case LLAssetType::AT_TEXTURE_TGA:
		return true;
	default:
		return false;
	}
}
LLGroupNotifyBox::LLGroupNotifyBox(const std::string& subject,
							 const std::string& message,
							 const std::string& from_name,
							 const LLUUID& group_id,
							 const LLUUID& group_insignia,
							 const std::string& group_name,
							 const LLDate& time_stamp,
							 const bool& has_inventory,
							 const std::string& inventory_name,
							 const LLSD& inventory_offer)
:	LLPanel("groupnotify", LLGroupNotifyBox::getGroupNotifyRect(), BORDER_NO),
	mAnimating(true),
	mSunkToWell(false),
	mTimer(),
	mNextBtn(NULL),
	mHideBtn(NULL),
	mSaveInventoryBtn(NULL),
	mGroupID(group_id),
	mWellID(),
	mSubject(subject),
	mHasInventory(has_inventory),
	mInventoryOffer(NULL)
{
	mWellID.generate();
	constexpr S32 PAD = 2;
	const S32 TOP = getRect().getHeight() - 28;
	const S32 BOTTOM_PAD = PAD * 2;
	const S32 BTN_TOP = BOTTOM_PAD + BTN_HEIGHT + PAD;
	const S32 RIGHT = getRect().getWidth() - HPAD - 18;
	constexpr S32 LINE_HEIGHT = 16;
	constexpr S32 LABEL_WIDTH = 64;
	constexpr S32 ICON_WIDTH = 64;
  	time_t timestamp = (time_t)time_stamp.secondsSinceEpoch();
 	if (!timestamp) time(&timestamp);
	std::string time_buf;
	timeToFormattedString(timestamp, gSavedSettings.getString("TimestampFormat"), time_buf);
	if (mHasInventory)
	{
		mInventoryOffer = new LLOfferInfo(inventory_offer);
	}
	setFocusRoot(TRUE);
	setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);
	setBackgroundColor( gColors.getColor("GroupNotifyBoxColor") );
	S32 y = TOP;
	S32 x = ICON_WIDTH + HPAD + PAD;
	static const auto text_color = gColors.getColor("GroupNotifyTextColor");
	class NoticeText : public LLTextBox
	{
	public:
		NoticeText(const std::string& name, const LLRect& rect, const std::string& text = LLStringUtil::null, const LLFontGL* font = NULL)
			: LLTextBox(name, rect, text, font)
		{
			setHAlign(LLFontGL::RIGHT);
			setFontShadow(LLFontGL::DROP_SHADOW_SOFT);
			setBorderVisible(FALSE);
			setColor(text_color);
			setBackgroundColor(LLColor4::transparent);
		}
	};
	static LLStyleSP headerstyle = nullptr;
	if (!headerstyle)
	{
		headerstyle = new LLStyle(true, text_color, LLStringUtil::null);
		headerstyle->mBold = true;
	}
	const auto bottom = y - ICON_WIDTH;
	auto links = new LLTextEditor(std::string("group"), LLRect(x, y + 5, RIGHT, bottom), S32_MAX, LLStringUtil::null, nullptr, false, true);
	links->setBorderVisible(FALSE);
	links->setReadOnlyFgColor(text_color);
	links->setReadOnlyBgColor(LLColor4::transparent);
	links->setWriteableBgColor(LLColor4::transparent);
	links->setEnabled(false);
	links->setTakesNonScrollClicks(TRUE);
	links->setHideScrollbarForShortDocs(TRUE);
	links->setWordWrap(TRUE);
	links->appendText(subject, false, false, headerstyle, false);
	LLStyleSP meta_style(new LLStyle(true, text_color, LLStringUtil::null));
	links->appendText(time_buf, false, true, meta_style, true);
	links->appendText(LLTrans::getString("GroupNotifyBy", LLSD().with("NAME", from_name).with("GROUP", group_name)), false, true, meta_style, true);
	addChild(links);
	x = HPAD;
	LLIconCtrl* icon = new LLIconCtrl(std::string("icon"),
						  LLRect(x, y, x+ICON_WIDTH, bottom),
						  group_insignia.isNull() ? "icon_groupnotice.tga" : group_insignia.asString());
	icon->setMouseOpaque(FALSE);
	addChild(icon);
	y = bottom - PAD*2;
	S32 box_bottom = BTN_TOP + (mHasInventory ? (LINE_HEIGHT + 2*PAD) : 0);
	LLTextEditor* text = new LLViewerTextEditor(std::string("box"),
		LLRect(x, y, RIGHT, box_bottom),
		DB_GROUP_NOTICE_MSG_STR_LEN,
		LLStringUtil::null,
		LLFontGL::getFontSansSerif(),
		FALSE,
		true);
	text->setBorderVisible(false);
	LLStyleSP msgstyle(new LLStyle(true, text_color, LLStringUtil::null));
	text->appendText(message, false, false, msgstyle, false);
	text->setCursor(0,0);
	text->setWordWrap(TRUE);
	text->setTabsToNextField(TRUE);
	text->setMouseOpaque(TRUE);
	text->setTakesNonScrollClicks(TRUE);
	text->setHideScrollbarForShortDocs(TRUE);
	text->setReadOnlyFgColor(text_color);
	text->setReadOnlyBgColor(LLColor4::transparent);
	text->setWriteableBgColor(LLColor4::transparent);
	addChild(text);
	y = box_bottom - PAD;
	if (mHasInventory)
	{
		addChild(new NoticeText(std::string("subjecttitle"), LLRect(x,y,x + LABEL_WIDTH,y - LINE_HEIGHT), LLTrans::getString("GroupNotifyAttached") + ' ', LLFontGL::getFontSansSerif()));
		LLUIImagePtr item_icon = LLInventoryIcon::getIcon(mInventoryOffer->mType,
												LLInventoryType::IT_TEXTURE,
												0, FALSE);
		x += LABEL_WIDTH + HPAD;
		std::stringstream ss;
		ss << "        " << inventory_name;
		LLTextBox *line = new LLTextBox(std::string("object_name"),LLRect(x,y,RIGHT - HPAD,y - LINE_HEIGHT),ss.str(),LLFontGL::getFontSansSerif());
		line->setEnabled(FALSE);
		line->setBorderVisible(FALSE);
		line->setDisabledColor(text_color);
		line->setFontStyle(LLFontGL::NORMAL);
		line->setBackgroundVisible(FALSE);
		addChild(line);
		icon = new LLIconCtrl(std::string("icon"),
				LLRect(x, y, x+16, y-16),
				item_icon->getName());
		icon->setMouseOpaque(FALSE);
		addChild(icon);
	}
	mNextBtn = new LLButton(LLTrans::getString("next"),
				LLRect(getRect().getWidth()-26, BOTTOM_PAD + 20, getRect().getWidth()-2, BOTTOM_PAD),
				std::string("notify_next.png"),
				std::string("notify_next.png"),
				LLStringUtil::null,
				boost::bind(&LLGroupNotifyBox::moveToBack, this),
				LLFontGL::getFontSansSerif());
	mNextBtn->setToolTip(LLTrans::getString("next"));
	mNextBtn->setScaleImage(TRUE);
	addChild(mNextBtn);
	S32 btn_width = 80;
	S32 wide_btn_width = 136;
	LLRect btn_rect;
	x = 2 * HPAD;
	btn_rect.setOriginAndSize(x,
								BOTTOM_PAD,
								btn_width,
								BTN_HEIGHT);
	LLButton* btn = new LLButton(LLTrans::getString("ok"), btn_rect, LLStringUtil::null, boost::bind(&LLGroupNotifyBox::close, this));
	addChild(btn, -1);
	setDefaultBtn(btn);
	x += btn_width + HPAD;
	btn_rect.setOriginAndSize(x,
								BOTTOM_PAD,
								wide_btn_width,
								BTN_HEIGHT);
	btn = new LLButton(LLTrans::getString("GroupNotifyGroupNotices"), btn_rect, LLStringUtil::null, boost::bind(LLGroupActions::showTab, mGroupID, "notices_tab"));
	btn->setToolTip(LLTrans::getString("GroupNotifyViewPastNotices"));
	addChild(btn, -1);
	if (mHasInventory)
	{
		x += wide_btn_width + HPAD;
		btn_rect.setOriginAndSize(x,
									BOTTOM_PAD,
									wide_btn_width,
									BTN_HEIGHT);
		mSaveInventoryBtn = new LLButton(LLTrans::getString(is_openable(mInventoryOffer->mType) ? "GroupNotifyOpenAttachment" : "GroupNotifySaveAttachment"), btn_rect, LLStringUtil::null, boost::bind(&LLGroupNotifyBox::onClickSaveInventory,this));
		mSaveInventoryBtn->setVisible(mHasInventory);
		addChild(mSaveInventoryBtn);
	}
	if (++sGroupNotifyBoxCount == 1 || gSavedSettings.getBOOL("NotifyBoxStack"))
		mNextBtn->setVisible(false);
	{
		const S32 width = getRect().getWidth();
		const S32 height = getRect().getHeight();
		LLRect hide_rect(width - 20, height - 3, width - 4, height - 19);
		mHideBtn = new LLButton(std::string("hide"),
								hide_rect,
								std::string("UIImgBtnCloseActiveUUID"),
								std::string("UIImgBtnCloseActiveUUID"),
								LLStringUtil::null,
								boost::bind(&LLGroupNotifyBox::hideToWell, this));
		mHideBtn->setScaleImage(TRUE);
		mHideBtn->setToolTip(LLTrans::getString("NotifyToastHide"));
		mHideBtn->setTabStop(FALSE);
		mHideBtn->setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
		addChild(mHideBtn);
	}
}
LLGroupNotifyBox::~LLGroupNotifyBox()
{
	sGroupNotifyBoxCount--;
}
BOOL LLGroupNotifyBox::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (!LLPanel::handleRightMouseDown(x, y, mask))
		moveToBack();
	return TRUE;
}
void LLGroupNotifyBox::drawBackground() const
{
	drawAllynToastChrome(
		getRect().getWidth(),
		getRect().getHeight(),
		gColors.getColor("GroupNotifyBoxColor"),
		gFocusMgr.childHasKeyboardFocus(this));
}
void LLGroupNotifyBox::draw()
{
	F32 display_time = mTimer.getElapsedTimeF32();
	if (mAnimating && display_time < ANIMATION_TIME)
	{
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();
		F32 fraction = display_time / ANIMATION_TIME;
		F32 voffset = (1.f - fraction) * 12.f;
		gGL.translatef(0.f, voffset, 0.f);
		drawBackground();
		LLPanel::draw();
		gGL.popMatrix();
	}
	else
	{
		mAnimating = false;
		drawBackground();
		LLPanel::draw();
	}
	if (getVisible() && !mSunkToWell && !mAnimating)
	{
		S32 local_x = 0;
		S32 local_y = 0;
		screenPointToLocal(gViewerWindow->getCurrentMouseX(), gViewerWindow->getCurrentMouseY(), &local_x, &local_y);
		if (pointInView(local_x, local_y))
		{
			mTimer.reset();
		}
		else if (mTimer.getElapsedTimeF32() >= llmax(0.5f, gSavedSettings.getF32("NotifyBoxToastDuration")))
		{
			sinkToWell();
			if (gNotifyBoxView)
			{
				gNotifyBoxView->layoutStack();
			}
		}
	}
}
void LLGroupNotifyBox::sinkToWell()
{
	mSunkToWell = true;
	setVisible(FALSE);
}
void LLGroupNotifyBox::hideToWell()
{
	if (!mSunkToWell)
	{
		sinkToWell();
		if (gNotifyBoxView)
		{
			gNotifyBoxView->layoutStack();
		}
	}
}
void LLGroupNotifyBox::unsinkFromWell()
{
	mSunkToWell = false;
	mTimer.reset();
	setVisible(TRUE);
}
void LLGroupNotifyBox::close()
{
	if (mHasInventory)
	{
		mInventoryOffer->forceResponse(IOR_DECLINE);
		mInventoryOffer = NULL;
		mHasInventory = false;
	}
	gNotifyBoxView->removeChild(this);
	die();
	if (gNotifyBoxView)
	{
		gNotifyBoxView->layoutStack();
	}
}
void LLGroupNotifyBox::initClass()
{
	LLNotificationChannel::buildChannel("Group Notifications", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "groupnotify"));
	LLNotifications::instance().getChannel("Group Notifications")->connectChanged(&LLGroupNotifyBox::onNewNotification);
}
bool LLGroupNotifyBox::onNewNotification(const LLSD& notify)
{
	if (LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID()))
	{
		const LLSD& payload = notification->getPayload();
		LLGroupData group_data;
		if (!gAgent.getGroupData(payload["group_id"].asUUID(),group_data))
		{
			LL_WARNS() << "Group notice for unkown group: " << payload["group_id"].asUUID() << LL_ENDL;
			return false;
		}
		gNotifyBoxView->addChild(new LLGroupNotifyBox(payload["subject"].asString(),
									payload["message"].asString(),
									payload.has("sender_id") ? LLAvatarActions::getSLURL(payload["sender_id"]) : payload["sender_name"].asString(),
									payload["group_id"].asUUID(),
									group_data.mInsigniaID,
									LLGroupActions::getSLURL(payload["group_id"].asUUID()),
									notification->getDate(),
									payload["inventory_offer"].isDefined(),
									payload["inventory_name"].asString(),
									payload["inventory_offer"]));
	}
	return false;
}
void LLGroupNotifyBox::moveToBack()
{
	gNotifyBoxView->removeChild(this);
	gNotifyBoxView->addChildInBack(this);
	mNextBtn->setVisible(FALSE);
	gNotifyBoxView->layoutStack();
	if (sGroupNotifyBoxCount > 1)
	{
		LLView* view = gNotifyBoxView->getFirstChild();
		if (view && "groupnotify" == view->getName())
		{
			LLGroupNotifyBox* front = static_cast<LLGroupNotifyBox*>(view);
			if (front->mNextBtn)
			{
				front->mNextBtn->setVisible(true);
			}
		}
	}
}
LLRect LLGroupNotifyBox::getGroupNotifyRect()
{
	S32 notify_height = gSavedSettings.getS32("GroupNotifyBoxHeight");
	const S32 NOTIFY_WIDTH = gSavedSettings.getS32("GroupNotifyBoxWidth");
	const S32 TOP = gNotifyBoxView->getRect().getHeight();
	const S32 RIGHT = gNotifyBoxView->getRect().getWidth();
	const S32 LEFT = RIGHT - NOTIFY_WIDTH;
	return LLRect(LEFT, TOP, RIGHT, TOP-notify_height);
}
void LLGroupNotifyBox::onClickSaveInventory()
{
	mInventoryOffer->forceResponse(IOR_ACCEPT);
	mInventoryOffer = NULL;
	mHasInventory = false;
	mSaveInventoryBtn->setEnabled(FALSE);
}
