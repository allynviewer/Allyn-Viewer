/**
 * @file llfloaternotifywell.cpp
 * @brief Overflow list for stacked notify boxes.
 */
#include "llviewerprecompiledheaders.h"
#include "llfloaternotifywell.h"
#include "llbutton.h"
#include "llfontgl.h"
#include "llnotify.h"
#include "llscrolllistcolumn.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lltextbox.h"
#include "llframetimer.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
namespace
{
	const F64 NOTIFY_WELL_CLOSE_CLICK_GUARD = 0.4;
}
LLFloaterNotifyWell::LLFloaterNotifyWell(const LLSD& seed)
:	mList(NULL),
	mLastCloseTime(0.0)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_notify_well.xml");
}
BOOL LLFloaterNotifyWell::postBuild()
{
	mList = getChild<LLScrollListCtrl>("notification_list");
	if (mList)
	{
		mList->setDoubleClickCallback(boost::bind(&LLFloaterNotifyWell::onListDoubleClick, this));
		mList->setMouseUpCallback(boost::bind(&LLFloaterNotifyWell::onListMouseUp, this, _2, _3));
		mList->setCommentText(LLTrans::getString("NotifyWellEmpty"));
		if (LLTextBox* empty_label = mList->getChild<LLTextBox>("comment_text", TRUE, FALSE))
		{
			empty_label->setHAlign(LLFontGL::HCENTER);
		}
	}
	if (LLButton* open_btn = getChild<LLButton>("open_btn", TRUE, FALSE))
	{
		open_btn->setClickedCallback(boost::bind(&LLFloaterNotifyWell::onOpenSelected, this));
	}
	if (LLButton* close_all_btn = getChild<LLButton>("close_all_btn", TRUE, FALSE))
	{
		close_all_btn->setClickedCallback(boost::bind(&LLFloaterNotifyWell::onCloseAll, this));
	}
	return TRUE;
}
void LLFloaterNotifyWell::refreshItems(const std::vector<std::pair<LLUUID, std::string> >& items)
{
	if (!mList)
	{
		return;
	}
	LLUUID selected = mList->getCurrentID();
	mList->deleteAllItems();
	if (items.empty())
	{
		return;
	}
	const std::string close_tip = LLTrans::getString("NotifyWellClose");
	for (const auto& item : items)
	{
		LLScrollListItem::Params row;
		row.value(item.first);
		row.columns.add()
			.column("title")
			.value(item.second)
			.font_halign(LLFontGL::LEFT);
		row.columns.add()
			.column("close")
			.value("X")
			.font_halign(LLFontGL::HCENTER)
			.font_style("BOLD")
			.tool_tip(close_tip);
		mList->addRow(row, ADD_BOTTOM);
	}
	if (selected.notNull())
	{
		mList->selectByID(selected);
	}
}
void LLFloaterNotifyWell::onOpenSelected()
{
	if (!mList || !gNotifyBoxView)
	{
		return;
	}
	const LLUUID id = mList->getCurrentID();
	if (id.isNull())
	{
		return;
	}
	gNotifyBoxView->bringToFront(id);
}
void LLFloaterNotifyWell::onListDoubleClick()
{
	if (!wasRecentCloseClick())
	{
		onOpenSelected();
	}
}
void LLFloaterNotifyWell::onCloseAll()
{
	if (gNotifyBoxView)
	{
		gNotifyBoxView->closeAll();
	}
}
void LLFloaterNotifyWell::onListMouseUp(S32 x, S32 y)
{
	if (!mList || !gNotifyBoxView || wasRecentCloseClick())
	{
		return;
	}
	const LLRect list_rect = mList->getItemListRect();
	if (!list_rect.pointInRect(x, y))
	{
		return;
	}
	LLScrollListColumn* col = mList->getColumn(mList->getColumnIndexFromOffset(x - list_rect.mLeft));
	if (!col || col->mName != "close")
	{
		return;
	}
	LLScrollListItem* hit = mList->hitItem(x, y);
	if (!hit)
	{
		return;
	}
	const LLUUID id = hit->getUUID();
	if (id.notNull())
	{
		mLastCloseTime = (F64)LLFrameTimer::getElapsedSeconds();
		gNotifyBoxView->closeOne(id);
	}
}
bool LLFloaterNotifyWell::wasRecentCloseClick() const
{
	return ((F64)LLFrameTimer::getElapsedSeconds() - mLastCloseTime) < NOTIFY_WELL_CLOSE_CLICK_GUARD;
}
