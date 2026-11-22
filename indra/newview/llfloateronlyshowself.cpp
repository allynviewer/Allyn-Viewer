/**
 * @file llfloateronlyshowself.cpp
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llfloateronlyshowself.h"
#include "llagent.h"
#include "llfloateravatarpicker.h"
#include "llnamelistctrl.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llvoavatar.h"
LLFloaterOnlyShowSelf::LLFloaterOnlyShowSelf(const LLSD&)
:	LLFloater(std::string("only show self floater"))
,	mList(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_only_show_self.xml");
}
LLFloaterOnlyShowSelf::~LLFloaterOnlyShowSelf()
{
}
BOOL LLFloaterOnlyShowSelf::postBuild()
{
	mList = getChild<LLNameListCtrl>("whitelist_list");
	mList->setCommitOnSelectionChange(TRUE);
	childSetCommitCallback("whitelist_list", boost::bind(&LLFloaterOnlyShowSelf::updateButtons, this));
	childSetAction("add_btn", boost::bind(&LLFloaterOnlyShowSelf::onAdd, this));
	childSetAction("remove_btn", boost::bind(&LLFloaterOnlyShowSelf::onRemove, this));
	refreshList();
	return TRUE;
}
void LLFloaterOnlyShowSelf::onOpen()
{
	refreshList();
}
void LLFloaterOnlyShowSelf::onClose(bool app_quitting)
{
	app_quitting ? destroy() : setVisible(FALSE);
}
void LLFloaterOnlyShowSelf::refreshIfOpen()
{
	if (LLFloaterOnlyShowSelf* floater = findInstance())
	{
		floater->refreshList();
	}
}
void LLFloaterOnlyShowSelf::refreshList()
{
	if (!mList)
	{
		return;
	}
	uuid_vec_t selected = mList->getSelectedIDs();
	S32 scrollpos = mList->getScrollPos();
	mList->deleteAllItems();
	const LLSD list = gSavedPerAccountSettings.getLLSD("AllynOnlyShowSelfWhitelist");
	const S32 count = list.isArray() ? list.size() : 0;
	for (S32 i = 0; i < count; ++i)
	{
		LLUUID id(list[i].asString());
		if (id.notNull() && id != gAgentID)
		{
			mList->addNameItem(id);
		}
	}
	mList->updateSort();
	mList->selectMultiple(selected);
	mList->setScrollPos(scrollpos);
	updateButtons();
}
void LLFloaterOnlyShowSelf::updateButtons()
{
	getChildView("remove_btn")->setEnabled(mList && mList->getFirstSelected());
}
void LLFloaterOnlyShowSelf::onAdd()
{
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterOnlyShowSelf::onPicked, this, _1, _2), TRUE, TRUE, TRUE);
	if (picker)
	{
		addDependentFloater(picker);
	}
}
void LLFloaterOnlyShowSelf::onPicked(const uuid_vec_t& ids, const std::vector<LLAvatarName>&)
{
	LLVOAvatar::setOnlyShowSelfWhitelisted(ids, true);
}
void LLFloaterOnlyShowSelf::onRemove()
{
	if (!mList)
	{
		return;
	}
	uuid_vec_t ids = mList->getSelectedIDs();
	if (ids.empty())
	{
		return;
	}
	LLVOAvatar::setOnlyShowSelfWhitelisted(ids, false);
}
