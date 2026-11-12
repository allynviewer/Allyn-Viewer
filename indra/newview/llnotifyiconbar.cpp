/**
 * @file llnotifyiconbar.cpp
 * @brief Compact notification icon below the navigation bar.
 */
#include "llviewerprecompiledheaders.h"
#include "llnotifyiconbar.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llfloaternotifywell.h"
#include "llfontgl.h"
#include "llformat.h"
#include "llmenugl.h"
#include "llnavigationbar.h"
#include "llnotify.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llviewermenu.h"
#include "llviewerwindow.h"
LLNotifyIconBar* gNotifyIconBar = NULL;
S32 NOTIFY_ICON_BAR_HEIGHT = LLFloatingBarStyle::ROUND_BUTTON_SIZE;
S32 NOTIFY_ICON_BAR_WIDTH = LLFloatingBarStyle::ROUND_BUTTON_SIZE;
LLNotifyIconBar::LLNotifyIconBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, LLRect(), FALSE),
	mWellBtn(NULL),
	mMouselookHidden(false),
	mLastNotifyCount(-1)
{
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(FALSE);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_notify_icon_bar.xml");
	reshape(LLFloatingBarStyle::ROUND_BUTTON_SIZE, LLFloatingBarStyle::ROUND_BUTTON_SIZE, TRUE);
	setFollows(FOLLOWS_RIGHT | FOLLOWS_TOP);
	mWellBtn = getChild<LLButton>("notify_well_btn", TRUE, FALSE);
	if (mWellBtn)
	{
		mWellBtn->setClickedCallback(boost::bind(&LLNotifyIconBar::onWellClicked, this));
		mWellBtn->setFont(LLFontGL::getFontSansSerifBold());
		mWellBtn->setHAlign(LLFontGL::HCENTER);
		LLFloatingBarStyle::styleRoundButton(mWellBtn);
		mWellBtn->setUnselectedLabelColor(LLColor4::white);
		mWellBtn->setSelectedLabelColor(LLColor4::white);
		mWellBtn->setDropShadowedText(TRUE);
	}
	setRect(rect);
	mMarginSlot = LLFloatingBarStyle::connectSetting("FloatingBarMargin", boost::bind(&LLNotifyIconBar::updatePosition, this));
}
LLNotifyIconBar::~LLNotifyIconBar()
{
	mMarginSlot.disconnect();
	if (gNotifyIconBar == this)
	{
		gNotifyIconBar = NULL;
	}
}
void LLNotifyIconBar::draw()
{
	refreshBadges();
	mStyle.drawPill(getLocalRect(), mLastNotifyCount > 0 ? LLFloatingBarStyle::accentColor() : LLFloatingBarStyle::barColor());
	LLPanel::draw();
	if (mLastNotifyCount > 0)
	{
		LLUIImagePtr border = LLUI::getUIImage("Floating_Pill_Border");
		if (border.notNull())
		{
			const F32 wave = 0.5f + 0.5f * sinf((F32)LLFrameTimer::getElapsedSeconds() * 6.f);
			LLColor4 orange(1.f, 0.42f, 0.f, 0.2f + 0.8f * wave);
			border->draw(getLocalRect(), orange);
		}
	}
}
void LLNotifyIconBar::setVisibleForMouselook(bool visible)
{
	mMouselookHidden = !visible;
	bool show = visible && !gAgentCamera.cameraMouselook();
	if (getVisible() != (BOOL)show)
	{
		setVisible(show);
		if (gNotifyBoxView)
		{
			gNotifyBoxView->layoutStack();
		}
	}
}
void LLNotifyIconBar::updatePosition()
{
	LLView* parent = getParent();
	if (!parent)
	{
		return;
	}
	const S32 margin = LLFloatingBarStyle::margin();
	const S32 size = LLFloatingBarStyle::ROUND_BUTTON_SIZE;
	const S32 top = parent->getRect().getHeight() - margin;
	const S32 right = parent->getRect().getWidth() - margin;
	setShape(LLRect(right - size, top, right, top - size));
}
void LLNotifyIconBar::refreshBadges()
{
	S32 notify_count = gNotifyBoxView ? gNotifyBoxView->getOverflowCount() : 0;
	if (mWellBtn && notify_count != mLastNotifyCount)
	{
		mLastNotifyCount = notify_count;
		if (notify_count > 0)
		{
			mWellBtn->setLabel(llformat("%d", notify_count));
			mWellBtn->setImageOverlay(LLStringUtil::null);
		}
		else
		{
			mWellBtn->setLabel(LLStringExplicit(""));
			mWellBtn->setImageOverlay("floating_ui/bell.png", LLFontGL::HCENTER, LLColor4::white);
		}
	}
}
void LLNotifyIconBar::onWellClicked()
{
	LLFloaterNotifyWell::toggleInstance();
	if (LLFloaterNotifyWell::instanceVisible() && gNotifyBoxView)
	{
		gNotifyBoxView->layoutStack();
	}
}
