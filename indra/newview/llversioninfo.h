/** 
 * @file llversioninfo.h
 * @brief Routines to access the viewer version and build information
 * @author Martin Reddy
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#ifndef LL_LLVERSIONINFO_H
#define LL_LLVERSIONINFO_H
#include <string>
#include "stdtypes.h"
class LLVersionInfo
{
public:
	static S32 getMajor();
	static S32 getMinor();
	static S32 getPatch();
	static S32 getBuild();
	static const std::string &getVersion();
	static const std::string &getShortVersion();
	static const std::string &getChannelAndVersion();
	static const std::string &getChannel();
	static void resetChannel(const std::string& channel);
    typedef enum
    {
        TEST_VIEWER,
        PROJECT_VIEWER,
		ALPHA_VIEWER,
        BETA_VIEWER,
        RELEASE_VIEWER
    } ViewerMaturity;
    static ViewerMaturity getViewerMaturity();
};
#endif
