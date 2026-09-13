/** 
 * @file llloginhandler.cpp
 * @brief Handles filling in the login panel information from a SLURL
 * such as secondlife:///app/login?first=Bob&last=Dobbs
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2010, Linden Research, Inc.
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
#include "llloginhandler.h"
#include "llpanellogin.h"
#include "llslurl.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llmd5.h"
LLLoginHandler gLoginHandler;
bool LLLoginHandler::parseDirectLogin(std::string url)
{
	LLURI uri(url);
	parse(uri.queryMap());
	if (mWebLoginKey.isNull() ||
		mFirstName.empty() ||
		mLastName.empty())
	{
		return false;
	}
	else
	{
		return true;
	}
}
void LLLoginHandler::parse(const LLSD& queryMap)
{
	mWebLoginKey = queryMap["web_login_key"].asUUID();
	mFirstName = queryMap["first_name"].asString();
	mLastName = queryMap["last_name"].asString();
	std::string startLocation = queryMap["location"].asString();
	if (startLocation == "specify")
	{
	  LLStartUp::setStartSLURL(queryMap["region"].asString());
	}
	else if (startLocation == "home")
	{
	  LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_HOME));
	}
	else if (startLocation == "last")
	{
	  LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_LAST));
	}
}
bool LLLoginHandler::handle(const LLSD& tokens,
							const LLSD& query_map,
							LLMediaCtrl* web)
{
	parse(query_map);
	if (STATE_FIRST == LLStartUp::getStartupState())
	{
		return true;
	}
	std::string password = query_map["password"].asString();
	if (!password.empty())
	{
		gSavedSettings.setBOOL("RememberPassword", TRUE);
		if (password.substr(0,3) != "$1$")
		{
			LLMD5 pass((unsigned char*)password.c_str());
			char md5pass[33];
			pass.hex_digest(md5pass);
			std::string hashed_password = ll_safe_string(md5pass, 32);
			LLStartUp::savePasswordToDisk(hashed_password);
		}
	}
	if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
	{
		if (!mFirstName.empty() || !mLastName.empty())
		{
			LLPanelLogin::setFields(mFirstName, mLastName, password);
		}
		if (mWebLoginKey.isNull())
		{
			LLPanelLogin::loadLoginPage();
		}
		else
		{
			LLStartUp::setStartupState( STATE_LOGIN_CLEANUP );
		}
	}
	return true;
}
