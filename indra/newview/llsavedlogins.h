/**
 * @file llsavedlogins.h
 * @brief Manages a list of previous successful logins
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
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
#ifndef LLLOGINHISTORY_H
#define LLLOGINHISTORY_H
#include "llviewernetwork.h"
class LLSD;
class LLSavedLoginEntry
{
public:
	LLSavedLoginEntry(const LLSD& entry_data);
	LLSavedLoginEntry(const std::string& firstname,
					  const std::string& lastname,
	                  const std::string& password,
	                  const std::string& grid);
	const std::string getFirstName() const
	{
		return (mEntry.has("firstname") ? mEntry.get("firstname").asString() : std::string());
	}
	void setFirstName(std::string& value)
	{
		mEntry.insert("firstname", LLSD(value));
	}
	const std::string getLastName() const
	{
		return (mEntry.has("lastname") ? mEntry.get("lastname").asString() : std::string());
	}
	void setLastName(std::string& value)
	{
		mEntry.insert("lastname", LLSD(value));
	}
	const std::string getPassword() const;
	void setPassword(const std::string& value);
	const std::string getGrid() const
	{
		return (mEntry.has("grid") ? mEntry.get("grid").asString() : std::string());
	}
	void setGrid(const std::string& value){
		mEntry.insert("grid", LLSD(value));
	}
	bool isSecondLife() const;
	LLSD asLLSD() const;
	static const size_t PASSWORD_HASH_LENGTH = 32;
private:
	static const std::string decryptPassword(const LLSD& pwdata);
	static const LLSD encryptPassword(const std::string& password);
	LLSD mEntry;
};
typedef std::list<LLSavedLoginEntry> LLSavedLoginsList;
class LLSavedLogins
{
public:
	LLSavedLogins();
	LLSavedLogins(const LLSD& history_data);
	void addEntry(const LLSavedLoginEntry& entry);
	void deleteEntry(const std::string& firstname, const std::string& lastname, const std::string& grid);
	const LLSavedLoginsList& getEntries() const { return mEntries; }
	LLSD asLLSD() const;
	const size_t size() const {	return mEntries.size(); }
	static LLSavedLogins loadFile(const std::string& filepath);
	static bool saveFile(const LLSavedLogins& history, const std::string& filepath);
private:
	LLSavedLoginsList mEntries;
};
#endif
