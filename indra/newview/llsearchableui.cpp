/**
 * @file llsearchableui.cpp
 * @brief Searchable UI tree used by the preferences filter.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llsearchableui.h"

#include "llview.h"
#include "lltabcontainer.h"
#include "llpanel.h"
#include "llstring.h"

ll::prefs::SearchableItem::~SearchableItem()
{}

void ll::prefs::SearchableItem::setNotHighlighted()
{
	if (mCtrl)
		mCtrl->setHighlighted(false);
}

bool ll::prefs::SearchableItem::hightlightAndHide(LLWString const& aFilter)
{
	if (!mCtrl)
		return false;

	if (mCtrl->getHighlighted())
		return true;

	LLView const* pView = dynamic_cast<LLView const*>(mCtrl);
	if (pView && !pView->getVisible())
		return false;

	if (aFilter.empty())
	{
		mCtrl->setHighlighted(false);
		return true;
	}

	if (mLabel.find(aFilter) != LLWString::npos)
	{
		mCtrl->setHighlighted(true);
		return true;
	}

	return false;
}

ll::prefs::PanelData::~PanelData()
{}

bool ll::prefs::PanelData::hightlightAndHide(LLWString const& aFilter)
{
	for (tSearchableItemList::iterator itr = mChildren.begin(); itr != mChildren.end(); ++itr)
		(*itr)->setNotHighlighted();

	for (tPanelDataList::iterator itr = mChildPanel.begin(); itr != mChildPanel.end(); ++itr)
		(*itr)->setNotHighlighted();

	bool label_match = false;
	if (!aFilter.empty() && !mLabel.empty())
	{
		LLWString label = utf8str_to_wstring(mLabel);
		LLWStringUtil::toLower(label);
		label_match = label.find(aFilter) != LLWString::npos;
	}

	bool bVisible = aFilter.empty() || label_match;
	for (tSearchableItemList::iterator itr = mChildren.begin(); itr != mChildren.end(); ++itr)
		bVisible |= (*itr)->hightlightAndHide(aFilter);

	for (tPanelDataList::iterator itr = mChildPanel.begin(); itr != mChildPanel.end(); ++itr)
		bVisible |= (*itr)->hightlightAndHide(aFilter);

	return bVisible;
}

void ll::prefs::PanelData::setNotHighlighted()
{
	for (tSearchableItemList::iterator itr = mChildren.begin(); itr != mChildren.end(); ++itr)
		(*itr)->setNotHighlighted();

	for (tPanelDataList::iterator itr = mChildPanel.begin(); itr != mChildPanel.end(); ++itr)
		(*itr)->setNotHighlighted();
}

bool ll::prefs::TabContainerData::hightlightAndHide(LLWString const& aFilter)
{
	for (tSearchableItemList::iterator itr = mChildren.begin(); itr != mChildren.end(); ++itr)
		(*itr)->setNotHighlighted();

	bool bVisible(aFilter.empty());
	for (tSearchableItemList::iterator itr = mChildren.begin(); itr != mChildren.end(); ++itr)
		bVisible |= (*itr)->hightlightAndHide(aFilter);

	for (tPanelDataList::iterator itr = mChildPanel.begin(); itr != mChildPanel.end(); ++itr)
	{
		bool bPanelVisible = (*itr)->hightlightAndHide(aFilter);
		if (aFilter.empty())
		{
			bPanelVisible = true;
		}
		if ((*itr)->mPanel && mTabContainer)
			mTabContainer->setTabVisibility((*itr)->mPanel, bPanelVisible);
		bVisible |= bPanelVisible;
	}

	return bVisible;
}
