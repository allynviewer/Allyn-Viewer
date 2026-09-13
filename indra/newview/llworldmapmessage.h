/** 
 * @file llworldmapmessage.h
 * @brief Handling of the messages to the DB made by and for the world map.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#ifndef LL_LLWORLDMAPMESSAGE_H
#define LL_LLWORLDMAPMESSAGE_H
class LLMessageSystem;
class LLWorldMapMessage : public LLSingleton<LLWorldMapMessage>
{
public:
	typedef boost::function<void(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)>
		url_callback_t;
	LLWorldMapMessage();
	~LLWorldMapMessage();
	static void processMapBlockReply(LLMessageSystem*, void**);
	static void processMapItemReply(LLMessageSystem*, void**);
	void sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent, S32 layer = 2);
	void sendNamedRegionRequest(std::string region_name);
	void sendNamedRegionRequest(std::string region_name,
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport);
	void sendHandleRegionRequest(U64 region_handle,
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport);
	void sendItemRequest(U32 type, U64 handle = 0);
	void handleSLURL(std::string& name, U32 x_world, U32 y_world, LLUUID image_id);
private:
	std::string		mSLURLRegionName;
	U64				mSLURLRegionHandle;
	std::string		mSLURL;
	url_callback_t	mSLURLCallback;
	bool			mSLURLTeleport;
};
#endif
