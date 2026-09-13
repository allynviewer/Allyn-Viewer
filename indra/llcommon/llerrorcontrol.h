/**
 * @file llerrorcontrol.h
 * @date   December 2006
 * @brief error message system control
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#ifndef LL_LLERRORCONTROL_H
#define LL_LLERRORCONTROL_H
#include "llerror.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "boost/function.hpp"
#include "boost/shared_ptr.hpp"
#include <string>
class LLSD;
class LLLineBuffer
{
public:
	LLLineBuffer() {};
	virtual ~LLLineBuffer() {};
	virtual void clear() = 0;
	virtual void addLine(const std::string& utf8line) = 0;
};
namespace LLError
{
	LL_COMMON_API void initForServer(const std::string& identity);
	LL_COMMON_API void initForApplication(const std::string& dir, bool log_to_stderr = true);
	LL_COMMON_API void setPrintLocation(bool);
	LL_COMMON_API void setDefaultLevel(LLError::ELevel);
	LL_COMMON_API ELevel getDefaultLevel();
	LL_COMMON_API void setFunctionLevel(const std::string& function_name, LLError::ELevel);
	LL_COMMON_API void setClassLevel(const std::string& class_name, LLError::ELevel);
	LL_COMMON_API void setFileLevel(const std::string& file_name, LLError::ELevel);
	LL_COMMON_API void setTagLevel(const std::string& file_name, LLError::ELevel);
	LL_COMMON_API LLError::ELevel decodeLevel(std::string name);
	LL_COMMON_API void configure(const LLSD&);
	typedef boost::function<void(const std::string&)> FatalFunction;
	LL_COMMON_API void crashAndLoop(const std::string& message);
	LL_COMMON_API void setFatalFunction(const FatalFunction&);
	LL_COMMON_API FatalFunction getFatalFunction();
	class LL_COMMON_API OverrideFatalFunction
	{
	public:
		OverrideFatalFunction(const FatalFunction& func):
			mPrev(getFatalFunction())
		{
			setFatalFunction(func);
		}
		~OverrideFatalFunction()
		{
			setFatalFunction(mPrev);
		}
	private:
		FatalFunction mPrev;
	};
	typedef std::string (*TimeFunction)();
	LL_COMMON_API std::string utcTime();
	LL_COMMON_API void setTimeFunction(TimeFunction);
	class LL_COMMON_API Recorder
	{
	public:
		Recorder();
		virtual ~Recorder();
		virtual void recordMessage(LLError::ELevel, const std::string& message) = 0;
		bool wantsTime();
		bool wantsTags();
		bool wantsLevel();
		bool wantsLocation();
		bool wantsFunctionName();
	protected:
		bool	mWantsTime,
				mWantsTags,
				mWantsLevel,
				mWantsLocation,
				mWantsFunctionName;
	};
	typedef boost::shared_ptr<Recorder> RecorderPtr;
	LL_COMMON_API void addRecorder(RecorderPtr);
	LL_COMMON_API void removeRecorder(RecorderPtr);
	LL_COMMON_API void logToFile(const std::string& filename);
	LL_COMMON_API void logToFixedBuffer(LLLineBuffer*);
	LL_COMMON_API std::string logFileName();
	typedef LLPointer<LLRefCount> SettingsStoragePtr;
	LL_COMMON_API SettingsStoragePtr saveAndResetSettings();
	LL_COMMON_API void restoreSettings(SettingsStoragePtr pSettingsStorage);
	LL_COMMON_API std::string abbreviateFile(const std::string& filePath);
	LL_COMMON_API int shouldLogCallCount();
};
#endif
