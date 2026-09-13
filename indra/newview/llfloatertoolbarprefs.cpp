/**
 * @file llfloatertoolbarprefs.cpp
 * @brief Floater for choosing which toolbar buttons are visible.
 */
#include "llviewerprecompiledheaders.h"
#include "llfloatertoolbarprefs.h"
#include "llcheckboxctrl.h"
#include "llcontrol.h"
#include "llfiltereditor.h"
#include "lluictrlfactory.h"
#include <algorithm>
LLFloaterToolbarPrefs::LLFloaterToolbarPrefs(const LLSD&)
:	LLFloater(std::string("Toolbar Prefs"))
{
	mCommitCallbackRegistrar.add("FilterToolbarButtons", boost::bind(&LLFloaterToolbarPrefs::onFilter, this, _2));
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_toolbar_prefs.xml");
}
BOOL LLFloaterToolbarPrefs::postBuild()
{
	const child_list_t* children = getChildList();
	for (LLView* child : *children)
	{
		LLCheckBoxCtrl* checkbox = dynamic_cast<LLCheckBoxCtrl*>(child);
		if (!checkbox)
		{
			continue;
		}
		CheckboxInfo info;
		info.ctrl = checkbox;
		info.original_rect = checkbox->getRect();
		mCheckboxes.push_back(info);
	}
	if (LLFilterEditor* filter = findChild<LLFilterEditor>("filter_input"))
	{
		filter->setCommitCallback(boost::bind(&LLFloaterToolbarPrefs::onFilter, this, _2));
		filter->setKeystrokeCallback(boost::bind(&LLFloaterToolbarPrefs::onFilter, this, _2));
	}
	return TRUE;
}
void LLFloaterToolbarPrefs::onFilter(const LLSD& value)
{
	applyFilter(value.asString());
}
void LLFloaterToolbarPrefs::applyFilter(const std::string& filter)
{
	std::string needle = filter;
	LLStringUtil::trim(needle);
	LLStringUtil::toLower(needle);
	if (needle.empty())
	{
		for (const CheckboxInfo& info : mCheckboxes)
		{
			info.ctrl->setRect(info.original_rect);
			bool visible = true;
			if (LLControlVariable* vis = info.ctrl->getMakeVisibleControlVariable())
			{
				visible = vis->getValue().asBoolean();
			}
			info.ctrl->setVisible(visible);
		}
		return;
	}
	std::vector<CheckboxInfo> matches;
	for (const CheckboxInfo& info : mCheckboxes)
	{
		bool allowed = true;
		if (LLControlVariable* vis = info.ctrl->getMakeVisibleControlVariable())
		{
			allowed = vis->getValue().asBoolean();
		}
		std::string label = info.ctrl->getLabel();
		LLStringUtil::toLower(label);
		const bool match = allowed && label.find(needle) != std::string::npos;
		info.ctrl->setVisible(match);
		if (match)
		{
			matches.push_back(info);
		}
	}
	if (matches.empty())
	{
		return;
	}
	std::sort(matches.begin(), matches.end(),
		[](const CheckboxInfo& a, const CheckboxInfo& b)
		{
			if (a.original_rect.mLeft != b.original_rect.mLeft)
			{
				return a.original_rect.mLeft < b.original_rect.mLeft;
			}
			return a.original_rect.mBottom > b.original_rect.mBottom;
		});
	S32 start_left = mCheckboxes.front().original_rect.mLeft;
	S32 start_bottom = mCheckboxes.front().original_rect.mBottom;
	for (const CheckboxInfo& info : mCheckboxes)
	{
		start_left = llmin(start_left, info.original_rect.mLeft);
		start_bottom = llmax(start_bottom, info.original_rect.mBottom);
	}
	const S32 col_width = 170;
	const S32 row_height = 22;
	const S32 rows_per_col = llmax(1, (start_bottom - 12) / row_height + 1);
	for (S32 i = 0; i < (S32)matches.size(); ++i)
	{
		const S32 col = i / rows_per_col;
		const S32 row = i % rows_per_col;
		const LLRect& src = matches[i].original_rect;
		const S32 left = start_left + col * col_width;
		const S32 bottom = start_bottom - row * row_height;
		matches[i].ctrl->setRect(LLRect(left, bottom + src.getHeight(), left + src.getWidth(), bottom));
	}
}
