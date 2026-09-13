/** 
 * @file lllogchat.cpp
 * @brief LLLogChat class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include <ctime>
#include "lllogchat.h"
#include "llappviewer.h"
#include "llfloaterchat.h"
#include "llsdserialize.h"
static std::string get_log_dir_file(const std::string& filename)
{
	return gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS, filename);
}
std::string LLLogChat::makeLogFileNameInternal(std::string filename)
{
	static const LLCachedControl<bool> with_date(gSavedPerAccountSettings, "LogFileNamewithDate");
	if (with_date)
	{
		time_t now;
		time(&now);
		std::array<char, 100> dbuffer;
		static const LLCachedControl<std::string> local_chat_date_format(gSavedPerAccountSettings, "LogFileLocalChatDateFormat", "-%Y-%m-%d");
		static const LLCachedControl<std::string> ims_date_format(gSavedPerAccountSettings, "LogFileIMsDateFormat", "-%Y-%m");
		strftime(dbuffer.data(), dbuffer.size(), (filename == "chat" ? local_chat_date_format : ims_date_format)().c_str(), localtime(&now));
		filename += dbuffer.data();
	}
	cleanFileName(filename);
	return get_log_dir_file(filename + ".txt");
}
bool LLLogChat::migrateFile(const std::string& old_name, const std::string& filename)
{
	std::string oldfile = makeLogFileNameInternal(old_name);
	if (!LLFile::isfile(oldfile)) return false;
	if (LLFile::isfile(filename))
	{
		auto&& new_untracked_log = llifstream(filename);
		auto&& tracked_log = llofstream(oldfile, llofstream::out|llofstream::app);
		bool failed = !(tracked_log << new_untracked_log.rdbuf());
		new_untracked_log.close();
		tracked_log.close();
		if (failed || LLFile::remove(filename))
			return true;
	}
	LLFile::rename(oldfile, filename);
	return true;
}
static LLSD sIDMap;
static std::string get_ids_map_file() { return get_log_dir_file("ids_to_names.json"); }
void LLLogChat::initializeIDMap()
{
	const auto map_file = get_ids_map_file();
	bool write = true;
	if (LLFile::isfile(map_file))
	{
		if (auto&& fstr = llifstream(map_file))
		{
			LLSDSerialize::fromNotation(sIDMap, fstr, LLSDSerialize::SIZE_UNLIMITED);
			fstr.close();
		}
		write = false;
	}
	if (gCacheName)
	{
		bool empty = sIDMap.size() == 0;
		for (const auto& r : gCacheName->getReverseMap())
		{
			const auto id = r.second.asString();
			const auto& name = r.first;
			const auto filename = makeLogFileNameInternal(name);
			bool id_known = !empty && sIDMap.has(id);
			if (id_known ? name != sIDMap[id].asStringRef()
					&& migrateFile(sIDMap[id].asStringRef(), filename)
				: LLFile::isfile(filename))
			{
				if (id_known) write = true;
				sIDMap[id] = name;
			}
		}
		if (write)
		if (auto&& fstr = llofstream(map_file))
		{
			LLSDSerialize::toPrettyNotation(sIDMap, fstr);
			fstr.close();
		}
	}
}
std::string LLLogChat::makeLogFileName(const std::string& username, const LLUUID& id)
{
	const auto name = username.empty() ? id.asString() : username;
	std::string filename = makeLogFileNameInternal(name);
	if (id.notNull() && !LLFile::isfile(filename))
	{
		auto& entry = sIDMap[id.asString()];
		const bool empty = !entry.size();
		if (empty || entry != name)
		{
			if (empty)
			{
				for (const auto& r : gCacheName->getReverseMap())
					if (r.second == id && migrateFile(r.first, filename))
						break;
			}
			else migrateFile(entry.asStringRef(), filename);
			entry = name;
			if (auto&& fstr = llofstream(get_ids_map_file()))
			{
				LLSDSerialize::toPrettyNotation(sIDMap, fstr);
				fstr.close();
			}
		}
	}
	return filename;
}
void LLLogChat::cleanFileName(std::string& filename)
{
	std::string invalidChars = "\"\'\\/?*:<>|[]{}~";
	S32 position = filename.find_first_of(invalidChars);
	while (position != filename.npos)
	{
		filename[position] = '_';
		position = filename.find_first_of(invalidChars, position);
	}
}
static void time_format(std::string& out, const char* fmt, const std::tm* time)
{
	typedef typename std::vector<char, boost::alignment::aligned_allocator<char, 1>> vec_t;
	static thread_local vec_t charvector(1024);
	#define format_the_time() std::strftime(charvector.data(), charvector.capacity(), fmt, time)
	const auto smallsize(charvector.capacity());
	const auto size = format_the_time();
	if (size < 0)
	{
		LL_ERRS() << "Formatting time failed, code " << size << ". String hint: " << out << '/' << fmt << LL_ENDL;
	}
	else if (static_cast<vec_t::size_type>(size) >= smallsize)
	{
		charvector.resize(1+size);
		format_the_time();
	}
	#undef format_the_time
	out.assign(charvector.data());
}
std::string LLLogChat::timestamp(bool withdate)
{
	auto time = utc_to_pacific_time(time_corrected(), gPacificDaylightTime);
	static const LLCachedControl<bool> withseconds("SecondsInLog");
	static const LLCachedControl<std::string> date("ShortDateFormat");
	static const LLCachedControl<std::string> shorttime("ShortTimeFormat");
	static const LLCachedControl<std::string> longtime("LongTimeFormat");
	std::string text = "[";
	if (withdate) text += date() + ' ';
	text += (withseconds ? longtime : shorttime)() + "]  ";
	time_format(text, text.data(), time);
	return text;
}
void LLLogChat::saveHistory(const std::string& name, const LLUUID& id, const std::string& line)
{
	if(name.empty() && id.isNull())
	{
		LL_INFOS() << "Filename is Empty!" << LL_ENDL;
		return;
	}
	LLFILE* fp = LLFile::fopen(LLLogChat::makeLogFileName(name, id), "a");
	if (!fp)
	{
		LL_INFOS() << "Couldn't open chat history log!" << LL_ENDL;
	}
	else
	{
		fprintf(fp, "%s\n", line.c_str());
		fclose (fp);
	}
}
static long const LOG_RECALL_BUFSIZ = 2048;
void LLLogChat::loadHistory(const std::string& name, const LLUUID& id, std::function<void (ELogLineType, const std::string&)> callback)
{
	if (name.empty() && id.isNull())
	{
		LL_WARNS() << "filename is empty!" << LL_ENDL;
	}
	else while(1)
	{
		static const LLCachedControl<U32> lines("LogShowHistoryLines", 32);
		if (lines == 0) break;
		LLFILE* fptr = LLFile::fopen(makeLogFileName(name, id), "rb");
		if (!fptr) break;
		if (fseek(fptr, 0, SEEK_END)) break;
		long pos = ftell(fptr) - 1;
		if (pos < 0) break;
		char buffer[LOG_RECALL_BUFSIZ];
		bool error = false;
		U32 nlines = 0;
		while (pos > 0 && nlines < lines)
		{
			size_t size = llmin(LOG_RECALL_BUFSIZ, pos);
			pos -= size;
			fseek(fptr, pos, SEEK_SET);
			size_t len = fread(buffer, 1, size, fptr);
			error = len != size;
			if (error) break;
			for (char const* p = buffer + size - 1; p >= buffer; --p)
			{
				if (*p == '\n')
				{
					if (++nlines == lines)
					{
						pos += p - buffer + 1;
						break;
					}
				}
			}
		}
		if (error)
		{
			fclose(fptr);
			break;
		}
		fseek(fptr, pos, SEEK_SET);
		while (fgets(buffer, LOG_RECALL_BUFSIZ, fptr))
		{
			for (S32 i = strlen(buffer) - 1; i >= 0 && (buffer[i] == '\r' || buffer[i] == '\n'); --i)
				buffer[i] = '\0';
			callback(LOG_LINE, buffer);
		}
		fclose(fptr);
		callback(LOG_END, LLStringUtil::null);
		return;
	}
	callback(LOG_EMPTY, LLStringUtil::null);
}
