/** 
 * @file llautoreplace.h
 * @brief Auto Replace Manager
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
 */
#ifndef LLAUTOREPLACE_H
#define LLAUTOREPLACE_H
#include "lllineeditor.h"
class LLAutoReplace;
class LLAutoReplaceSettings
{
  public:
	LLAutoReplaceSettings();
	~LLAutoReplaceSettings();
	LLAutoReplaceSettings(const LLAutoReplaceSettings& settings);
	void set(const LLAutoReplaceSettings& newSettings);
	bool setFromLLSD(const LLSD& settingsFromLLSD);
	LLSD getListNames();
	typedef enum
	{
		AddListOk,
		AddListDuplicateName,
		AddListInvalidList,
	} AddListResult;
	AddListResult addList(const LLSD& newList);
	AddListResult replaceList(const LLSD& newList);
	bool removeReplacementList(std::string listName);
	bool increaseListPriority(std::string listName);
	bool decreaseListPriority(std::string listName);
	const LLSD* exportList(std::string listName);
	bool listIsValid(const LLSD& listSettings);
	bool listNameIs(const LLSD& listSettings);
	bool listNameIsUnique(const LLSD& newList);
	static void createEmptyList(LLSD& emptyList);
	static void setListName(LLSD& list, const std::string& newName);
	static std::string getListName(LLSD& list);
	const LLSD* getListEntries(std::string listName);
	std::string replacementFor(std::string keyword, std::string listName);
	bool addEntryToList(LLWString keyword, LLWString replacement, std::string listName);
	bool removeEntryFromList(std::string keyword, std::string listName);
	std::string replaceWord(const std::string currentWord );
	LLSD getExampleLLSD();
	const LLSD& asLLSD();
  private:
	bool listNameMatches( const LLSD& list, const std::string name );
	LLSD mLists;
	static const std::string AUTOREPLACE_LIST_NAME;
	static const std::string AUTOREPLACE_LIST_REPLACEMENTS;
};
class LLAutoReplace : public LLSingleton<LLAutoReplace>
{
public:
    void autoreplaceCallback(S32& replacement_start, S32& replacement_length, LLWString& replacement_string, S32& cursor_pos, const LLWString& input_text);
    LLAutoReplaceSettings getSettings();
    void setSettings(const LLAutoReplaceSettings& settings);
private:
    friend class LLSingleton<LLAutoReplace>;
    LLAutoReplace();
    void initSingleton();
    LLAutoReplaceSettings mSettings;
    void loadFromSettings();
    void saveToUserSettings();
    std::string getUserSettingsFileName();
    std::string getAppSettingsFileName();
    static const char* SETTINGS_FILE_NAME;
};
#endif
