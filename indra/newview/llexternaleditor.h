/** 
 * @file llexternaleditor.h
 * @brief A convenient class to run external editor.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
#ifndef LL_LLEXTERNALEDITOR_H
#define LL_LLEXTERNALEDITOR_H
#include <llprocesslauncher.h>
class LLExternalEditor
{
	typedef std::vector<std::string> string_vec_t;
public:
	typedef enum e_error_code {
		EC_SUCCESS,
		EC_NOT_SPECIFIED,
		EC_PARSE_ERROR,
		EC_BINARY_NOT_FOUND,
		EC_FAILED_TO_RUN,
	} EErrorCode;
	EErrorCode setCommand(const std::string& env_var, const std::string& override = LLStringUtil::null);
	EErrorCode run(const std::string& file_path);
	static std::string getErrorMessage(EErrorCode code);
private:
	static std::string findCommand(
		const std::string& env_var,
		const std::string& override);
	static size_t tokenize(string_vec_t& tokens, const std::string& str);
	static const std::string sFilenameMarker;
	static const std::string sSetting;
	std::string			mArgs;
	LLProcessLauncher	mProcess;
};
#endif
