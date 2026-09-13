/** 
 * @file llfloaterautoreplacesettings.h
 * @brief Auto Replace List floater
 * @copyright Copyright (c) 2011 LordGregGreg Back
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
#ifndef LLFLOATERAUTOREPLACESETTINGS_H
#define LLFLOATERAUTOREPLACESETTINGS_H
#include "llfloater.h"
#include "llautoreplace.h"
class AIFilePicker;
class LLLineEditor;
class LLScrollListCtrl;
class LLFloaterAutoReplaceSettings : public LLFloater, public LLFloaterSingleton<LLFloaterAutoReplaceSettings>
{
public:
	LLFloaterAutoReplaceSettings(const LLSD& key = LLSD());
	BOOL postBuild();
private:
	bool mEnabled;
	LLAutoReplaceSettings mSettings;
	std::string       mSelectedListName;
	LLScrollListCtrl* mListNames;
	LLScrollListCtrl* mReplacementsList;
	LLLineEditor*     mKeyword;
	std::string       mPreviousKeyword;
	LLLineEditor*     mReplacement;
	void onAutoReplaceToggled();
	void onSelectList();
	void onImportList();
	void onImportList_continued(AIFilePicker* picker);
	void onExportList();
	void onExportList_continued(AIFilePicker* picker, const LLSD* list);
	void onNewList();
	void onDeleteList();
	void onListUp();
	void onListDown();
	void onSelectEntry();
	void onAddEntry();
	void onDeleteEntry();
	void onSaveEntry();
	void onSaveChanges();
	void updateListNames();
	void updateListNamesControls();
	void updateReplacementsList();
	void enableReplacementEntry();
	void disableReplacementEntry();
	bool callbackNewListName(const LLSD& notification, const LLSD& response);
	bool callbackListNameConflict(const LLSD& notification, const LLSD& response);
	bool selectedListIsFirst();
	bool selectedListIsLast();
};
#endif
