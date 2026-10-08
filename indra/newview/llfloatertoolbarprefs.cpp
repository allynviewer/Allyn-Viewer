/**
 * @file llfloatertoolbarprefs.cpp
 * @brief Floater for choosing which toolbar buttons are visible.
 */
#include "llviewerprecompiledheaders.h"
#include "llfloatertoolbarprefs.h"
#include "llcheckboxctrl.h"
#include "llcontrol.h"
#include "llfiltereditor.h"
#include "llfontgl.h"
#include "llpanel.h"
#include "llscrollcontainer.h"
#include "llscrollbar.h"
#include "lluictrlfactory.h"
#include <algorithm>
namespace
{
const S32 TOOLBAR_PREFS_TOP_INSET = 4;
const S32 TOOLBAR_PREFS_FILTER_GAP = 4;
const S32 TOOLBAR_PREFS_ROW_PITCH = 22;
struct ToolbarPrefsFlow
{
	S32 columns;
	S32 panel_width;
	S32 panel_height;
};
S32 columnsForWidth(S32 width, S32 step)
{
	if (step < 1)
	{
		step = 1;
	}
	if (width < step)
	{
		return 1;
	}
	return width / step;
}
S32 contentHeightFor(S32 count, S32 columns, S32 row_pitch, S32 item_h)
{
	if (count <= 0 || columns <= 0)
	{
		return 0;
	}
	const S32 rows = (count + columns - 1) / columns;
	return (rows - 1) * row_pitch + item_h;
}
ToolbarPrefsFlow flowToolbarPrefs(S32 inner_w, S32 inner_h, S32 step, S32 count, S32 row_pitch, S32 item_h)
{
	ToolbarPrefsFlow flow;
	flow.columns = 1;
	flow.panel_width = inner_w > 0 ? inner_w : 0;
	flow.panel_height = 0;
	if (count <= 0 || inner_w <= 0)
	{
		return flow;
	}
	if (step < 1)
	{
		step = 1;
	}
	if (row_pitch < item_h)
	{
		row_pitch = item_h;
	}
	S32 cols = columnsForWidth(inner_w, step);
	S32 content_h = contentHeightFor(count, cols, row_pitch, item_h);
	S32 panel_w = inner_w;
	if (inner_h < content_h)
	{
		S32 narrow_w = inner_w - SCROLLBAR_SIZE;
		if (narrow_w < 0)
		{
			narrow_w = 0;
		}
		cols = columnsForWidth(narrow_w, step);
		content_h = contentHeightFor(count, cols, row_pitch, item_h);
		if (inner_h < content_h)
		{
			panel_w = narrow_w;
		}
	}
	flow.columns = cols;
	flow.panel_width = panel_w;
	flow.panel_height = content_h;
	return flow;
}
}
LLFloaterToolbarPrefs::LLFloaterToolbarPrefs(const LLSD&)
:	LLFloater(std::string("Toolbar Prefs")),
	mFilterEditor(NULL),
	mButtonsPanel(NULL),
	mScroll(NULL),
	mInLayout(false)
{
	mCommitCallbackRegistrar.add("FilterToolbarButtons", boost::bind(&LLFloaterToolbarPrefs::onFilter, this, _2));
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_toolbar_prefs.xml");
}
LLFloaterToolbarPrefs::~LLFloaterToolbarPrefs()
{
	for (boost::signals2::connection& connection : mVisibilityConnections)
	{
		connection.disconnect();
	}
}
BOOL LLFloaterToolbarPrefs::postBuild()
{
	if (mScroll)
	{
		return TRUE;
	}
	mFilterEditor = findChild<LLFilterEditor>("filter_input");
	if (mFilterEditor)
	{
		mFilterEditor->setCommitCallback(boost::bind(&LLFloaterToolbarPrefs::onFilter, this, _2));
		mFilterEditor->setKeystrokeCallback(boost::bind(&LLFloaterToolbarPrefs::onFilter, this, _2));
	}
	std::vector<LLCheckBoxCtrl*> found;
	const child_list_t* children = getChildList();
	for (LLView* child : *children)
	{
		LLCheckBoxCtrl* checkbox = dynamic_cast<LLCheckBoxCtrl*>(child);
		if (checkbox)
		{
			found.push_back(checkbox);
		}
	}
	std::sort(found.begin(), found.end(),
		[](LLCheckBoxCtrl* a, LLCheckBoxCtrl* b)
		{
			const LLRect& ra = a->getRect();
			const LLRect& rb = b->getRect();
			if (ra.mLeft != rb.mLeft)
			{
				return ra.mLeft < rb.mLeft;
			}
			if (ra.mBottom != rb.mBottom)
			{
				return ra.mBottom > rb.mBottom;
			}
			return a->getName() < b->getName();
		});
	mCheckboxes.swap(found);
	mButtonsPanel = new LLPanel(std::string("toolbar_prefs_buttons"), LLRect(0, 16, 16, 0), FALSE);
	mButtonsPanel->setFollowsNone();
	mButtonsPanel->setBackgroundVisible(FALSE);
	for (LLCheckBoxCtrl* checkbox : mCheckboxes)
	{
		checkbox->setFollowsNone();
		mButtonsPanel->addChild(checkbox);
	}
	mScroll = new LLScrollContainer(std::string("toolbar_prefs_scroll"), LLRect(), mButtonsPanel, FALSE);
	mScroll->setBorderVisible(FALSE);
	mScroll->setFollowsNone();
	mScroll->setFollowsAll();
	addChild(mScroll);
	std::vector<LLControlVariable*> watched;
	for (LLCheckBoxCtrl* checkbox : mCheckboxes)
	{
		LLControlVariable* vis = checkbox->getMakeVisibleControlVariable();
		if (!vis)
		{
			continue;
		}
		if (std::find(watched.begin(), watched.end(), vis) != watched.end())
		{
			continue;
		}
		watched.push_back(vis);
		mVisibilityConnections.push_back(vis->getSignal()->connect(
			boost::bind(&LLFloaterToolbarPrefs::onVisibilityChanged, this, _1, _2)));
	}
	layoutContent();
	return TRUE;
}
void LLFloaterToolbarPrefs::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLFloater::reshape(width, height, called_from_parent);
	if (!mInLayout)
	{
		layoutContent();
	}
}
void LLFloaterToolbarPrefs::onFilter(const LLSD& value)
{
	applyFilter(value.asString());
}
void LLFloaterToolbarPrefs::onVisibilityChanged(LLControlVariable*, const LLSD&)
{
	if (!mInLayout)
	{
		layoutContent();
	}
}
void LLFloaterToolbarPrefs::applyFilter(const std::string& filter)
{
	std::string needle = filter;
	LLStringUtil::trim(needle);
	LLStringUtil::toLower(needle);
	mFilter = needle;
	layoutContent();
	if (mScroll)
	{
		mScroll->goToTop();
	}
}
S32 LLFloaterToolbarPrefs::checkboxColumnWidth(LLCheckBoxCtrl* ctrl) const
{
	const S32 fudge = 10;
	const LLFontGL* font = ctrl->getFont();
	S32 text_width = fudge;
	if (font)
	{
		text_width = font->getWidth(ctrl->getLabel()) + fudge;
	}
	return LLCHECKBOXCTRL_HPAD + LLCHECKBOXCTRL_BTN_SIZE + LLCHECKBOXCTRL_SPACING + text_width;
}
S32 LLFloaterToolbarPrefs::checkboxItemHeight(LLCheckBoxCtrl* ctrl) const
{
	const LLFontGL* font = ctrl->getFont();
	S32 text_h = font ? font->getLineHeight() : LLCHECKBOXCTRL_BTN_SIZE;
	S32 button_h = llmax(text_h, LLCHECKBOXCTRL_BTN_SIZE);
	return LLCHECKBOXCTRL_VPAD + button_h;
}
bool LLFloaterToolbarPrefs::checkboxAllowed(LLCheckBoxCtrl* ctrl) const
{
	LLControlVariable* vis = ctrl->getMakeVisibleControlVariable();
	if (!vis)
	{
		return true;
	}
	return vis->getValue().asBoolean();
}
void LLFloaterToolbarPrefs::layoutContent()
{
	if (mInLayout || isMinimized() || !mScroll || !mButtonsPanel)
	{
		return;
	}
	mInLayout = true;
	const S32 pad = LLFLOATER_CONTENT_PAD;
	S32 filter_h = 20;
	if (mFilterEditor)
	{
		filter_h = llmax(filter_h, mFilterEditor->getRect().getHeight());
	}
	S32 step_floor = LLCHECKBOXCTRL_HPAD + LLCHECKBOXCTRL_BTN_SIZE + LLCHECKBOXCTRL_SPACING + 10;
	S32 item_h = LLCHECKBOXCTRL_HEIGHT;
	S32 row_pitch = TOOLBAR_PREFS_ROW_PITCH;
	bool have_item = false;
	for (LLCheckBoxCtrl* checkbox : mCheckboxes)
	{
		if (!checkboxAllowed(checkbox))
		{
			continue;
		}
		step_floor = llmax(step_floor, checkboxColumnWidth(checkbox));
		if (!have_item)
		{
			item_h = checkboxItemHeight(checkbox);
			have_item = true;
		}
		else
		{
			item_h = llmax(item_h, checkboxItemHeight(checkbox));
		}
	}
	row_pitch = llmax(row_pitch, item_h);
	const S32 min_w = pad + step_floor + SCROLLBAR_SIZE + pad;
	const S32 min_h = LLFLOATER_HEADER_SIZE + TOOLBAR_PREFS_TOP_INSET + filter_h + TOOLBAR_PREFS_FILTER_GAP + item_h + pad;
	setResizeLimits(min_w, min_h);
	S32 width = getRect().getWidth();
	S32 height = getRect().getHeight();
	if (width < min_w || height < min_h)
	{
		width = llmax(width, min_w);
		height = llmax(height, min_h);
		LLFloater::reshape(width, height, TRUE);
	}
	const S32 top_limit = height - LLFLOATER_HEADER_SIZE - TOOLBAR_PREFS_TOP_INSET;
	if (mFilterEditor)
	{
		LLRect filter_rect;
		filter_rect.setLeftTopAndSize(pad, top_limit, width - pad * 2, filter_h);
		if (mFilterEditor->getRect() != filter_rect)
		{
			mFilterEditor->setRect(filter_rect);
			mFilterEditor->reshape(filter_rect.getWidth(), filter_rect.getHeight(), TRUE);
		}
	}
	const S32 filter_bottom = top_limit - filter_h;
	const S32 scroll_top = filter_bottom - TOOLBAR_PREFS_FILTER_GAP;
	const S32 scroll_bottom = pad;
	S32 scroll_h = scroll_top - scroll_bottom;
	if (scroll_h < 0)
	{
		scroll_h = 0;
	}
	LLRect scroll_rect;
	scroll_rect.setOriginAndSize(pad, scroll_bottom, width - pad * 2, scroll_h);
	std::vector<LLCheckBoxCtrl*> shown;
	shown.reserve(mCheckboxes.size());
	S32 step = step_floor;
	bool have_shown = false;
	for (LLCheckBoxCtrl* checkbox : mCheckboxes)
	{
		const bool allowed = checkboxAllowed(checkbox);
		bool match = allowed;
		if (match && !mFilter.empty())
		{
			std::string label = checkbox->getLabel();
			LLStringUtil::toLower(label);
			match = label.find(mFilter) != std::string::npos;
		}
		checkbox->setVisible(match);
		if (!match)
		{
			checkbox->setRect(LLRect());
			continue;
		}
		shown.push_back(checkbox);
		if (!have_shown)
		{
			step = checkboxColumnWidth(checkbox);
			have_shown = true;
		}
		else
		{
			step = llmax(step, checkboxColumnWidth(checkbox));
		}
	}
	if (!have_shown)
	{
		step = step_floor;
	}
	const ToolbarPrefsFlow flow = flowToolbarPrefs(scroll_rect.getWidth(), scroll_rect.getHeight(), step, (S32)shown.size(), row_pitch, item_h);
	mButtonsPanel->setRect(LLRect(0, flow.panel_height, flow.panel_width, 0));
	if (!shown.empty() && flow.columns > 0)
	{
		const S32 rows = ((S32)shown.size() + flow.columns - 1) / flow.columns;
		for (S32 i = 0; i < (S32)shown.size(); ++i)
		{
			const S32 col = i / rows;
			const S32 row = i % rows;
			const S32 left = col * step;
			const S32 bottom = flow.panel_height - item_h - row * row_pitch;
			const S32 item_w = checkboxColumnWidth(shown[i]);
			shown[i]->setRect(LLRect(left, bottom + item_h, left + item_w, bottom));
			shown[i]->reshape(item_w, item_h, TRUE);
		}
	}
	if (mScroll->getRect().getWidth() != scroll_rect.getWidth() || mScroll->getRect().getHeight() != scroll_rect.getHeight())
	{
		mScroll->reshape(scroll_rect.getWidth(), scroll_rect.getHeight(), TRUE);
	}
	const S32 scroll_dx = scroll_rect.mLeft - mScroll->getRect().mLeft;
	const S32 scroll_dy = scroll_rect.mBottom - mScroll->getRect().mBottom;
	if (scroll_dx != 0 || scroll_dy != 0)
	{
		mScroll->translate(scroll_dx, scroll_dy);
	}
	mScroll->reshape(scroll_rect.getWidth(), scroll_rect.getHeight(), TRUE);
	mInLayout = false;
}
