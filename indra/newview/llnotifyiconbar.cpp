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
S32 NOTIFY_ICON_BAR_HEIGHT = 27;
S32 NOTIFY_ICON_BAR_WIDTH = 36;
static const S32 NOTIFY_ICON_BAR_FLOAT_PAD = 3;
extern S32 MENU_BAR_HEIGHT;
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
	reshape(llmax(rect.getWidth(), NOTIFY_ICON_BAR_WIDTH),
			llmax(rect.getHeight(), NOTIFY_ICON_BAR_HEIGHT), TRUE);
	setFollows(FOLLOWS_RIGHT | FOLLOWS_TOP);
	mWellBtn = getChild<LLButton>("notify_well_btn", TRUE, FALSE);
	if (mWellBtn)
	{
		mWellBtn->setClickedCallback(boost::bind(&LLNotifyIconBar::onWellClicked, this));
		mWellBtn->setFont(LLFontGL::getFontSansSerifBold());
		mWellBtn->setHAlign(LLFontGL::HCENTER);
		mWellBtn->setScaleImage(TRUE);
		LLUIImagePtr chrome = LLUI::getUIImage("Rounded_Square");
		if (chrome.notNull())
		{
			mWellBtn->setImageUnselected(chrome);
			mWellBtn->setImageSelected(chrome);
			mWellBtn->setImageHoverUnselected(chrome);
			mWellBtn->setImageHoverSelected(chrome);
			mWellBtn->setImageDisabled(chrome);
		}
		const LLColor4 fill(18.f / 255.f, 20.f / 255.f, 43.f / 255.f, 0.62f);
		mWellBtn->setImageColor(fill);
		mWellBtn->setUnselectedLabelColor(LLColor4::white);
		mWellBtn->setSelectedLabelColor(LLColor4::white);
		mWellBtn->setDropShadowedText(TRUE);
		mWellBtn->setHoverGlowStrength(0.18f);
	}
	setRect(rect);
}
LLNotifyIconBar::~LLNotifyIconBar()
{
	if (gNotifyIconBar == this)
	{
		gNotifyIconBar = NULL;
	}
}
void LLNotifyIconBar::draw()
{
	refreshBadges();
	LLPanel::draw();
}
void LLNotifyIconBar::setVisibleForMouselook(bool visible)
{
	mMouselookHidden = !visible;
	bool show = visible && !gAgentCamera.cameraMouselook();
	if (getVisible() != (BOOL)show)
	{
		setVisible(show);
		updatePosition();
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
	S32 menu_h = (gMenuBarView) ? gMenuBarView->getRect().getHeight() : MENU_BAR_HEIGHT;
	S32 nav_h = (gNavigationBar) ? gNavigationBar->getOccupiedHeight() : 0;
	S32 top = parent->getRect().getHeight() - menu_h - nav_h - NOTIFY_ICON_BAR_FLOAT_PAD;
	S32 w = llmax(getRect().getWidth(), NOTIFY_ICON_BAR_WIDTH);
	S32 h = llmax(getRect().getHeight(), NOTIFY_ICON_BAR_HEIGHT);
	S32 right = parent->getRect().getWidth() - NOTIFY_ICON_BAR_FLOAT_PAD;
	setRect(LLRect(right - w, top, right, top - h));
}
S32 LLNotifyIconBar::getOccupiedHeight() const
{
	if (!getVisible())
	{
		return 0;
	}
	return getRect().getHeight() + NOTIFY_ICON_BAR_FLOAT_PAD;
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
			mWellBtn->setImageOverlay("notify_box_icon.tga", LLFontGL::HCENTER, LLColor4::white);
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
