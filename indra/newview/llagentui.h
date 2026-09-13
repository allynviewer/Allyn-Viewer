/** 
 * @file llagentui.h
 * @brief Utility methods to process agent's data as slurl's etc. before displaying
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
#ifndef LLAGENTUI_H
#define LLAGENTUI_H
class LLSLURL;
class LLAgentUI
{
public:
	enum ELocationFormat
	{
		LOCATION_FORMAT_NORMAL,
		LOCATION_FORMAT_LANDMARK,
		LOCATION_FORMAT_NO_MATURITY,
		LOCATION_FORMAT_NO_COORDS,
		LOCATION_FORMAT_FULL,
	};
	static void buildFullname(std::string &name);
	static void buildSLURL(LLSLURL& slurl, const bool escaped = true);
	static BOOL buildLocationString(std::string& str, ELocationFormat fmt = LOCATION_FORMAT_LANDMARK);
	static BOOL buildLocationString(std::string& str, ELocationFormat fmt,const LLVector3& agent_pos_region);
	static BOOL checkAgentDistance(const LLVector3& local_pole, F32 radius);
};
#endif
