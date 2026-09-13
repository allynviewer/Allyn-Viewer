/** 
 * @file llurlaction.h
 * @author Martin Reddy
 * @brief A set of actions that can performed on Urls
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
#ifndef LL_LLURLACTION_H
#define LL_LLURLACTION_H
#include <functional>
class LLUrlAction
{
public:
	LLUrlAction();
	static void openURL(std::string url);
	static void openURLInternal(std::string url);
	static void openURLExternal(std::string url);
	static bool executeSLURL(std::string url, bool trusted_content = true);
	static void teleportToLocation(std::string url);
	static void showLocationOnMap(std::string url);
	static void clickAction(std::string url, bool trusted_content);
	static void copyLabelToClipboard(std::string url);
	static void copyURLToClipboard(std::string url);
	static void showProfile(std::string url);
	static std::string getUserID(std::string url);
	static std::string getObjectName(std::string url);
	static std::string getObjectId(std::string url);
	static void sendIM(std::string url);
	static void addFriend(std::string url);
	static void removeFriend(std::string url);
	static void blockObject(std::string url);
	static void unblockObject(std::string url);
	typedef std::function<void (const std::string&)> url_callback_t;
	typedef std::function<bool(const std::string& url, bool trusted_content)> execute_url_callback_t;
	static void	setOpenURLCallback(url_callback_t cb);
	static void	setOpenURLInternalCallback(url_callback_t cb);
	static void	setOpenURLExternalCallback(url_callback_t cb);
	static void	setExecuteSLURLCallback(execute_url_callback_t cb);
private:
	static url_callback_t sOpenURLCallback;
	static url_callback_t sOpenURLInternalCallback;
	static url_callback_t sOpenURLExternalCallback;
	static execute_url_callback_t sExecuteSLURLCallback;
};
#endif
