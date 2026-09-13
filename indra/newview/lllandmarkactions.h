/** 
 * @file lllandmarkactions.h
 * @brief LLLandmark class declaration
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_LLLANDMARKACTIONS_H
#define LL_LLLANDMARKACTIONS_H
#include "llinventorymodel.h"
#include "lllandmarklist.h"
class LLLandmark;
class LLLandmarkActions
{
public:
	typedef std::function<void(std::string& slurl)> slurl_callback_t;
	typedef std::function<void(std::string& slurl, S32 x, S32 y, S32 z)> region_name_and_coords_callback_t;
	static LLInventoryModel::item_array_t fetchLandmarksByName(std::string& name, BOOL if_use_substring);
	static bool landmarkAlreadyExists();
	static bool hasParcelLandmark();
	static LLViewerInventoryItem* findLandmarkForGlobalPos(const LLVector3d &pos);
	static LLViewerInventoryItem* findLandmarkForAgentPos();
	static bool canCreateLandmarkHere();
	static void createLandmarkHere();
	static void createLandmarkHere(
		const std::string& name,
		const std::string& desc,
		const LLUUID& folder_id);
	static void getSLURLfromPosGlobal(const LLVector3d& global_pos, slurl_callback_t cb, bool escaped = true);
	static void getRegionNameAndCoordsFromPosGlobal(const LLVector3d& global_pos, region_name_and_coords_callback_t cb);
    static bool getLandmarkGlobalPos(const LLUUID& landmarkInventoryItemID, LLVector3d& posGlobal);
    static LLLandmark* getLandmark(const LLUUID& landmarkInventoryItemID, LLLandmarkList::loaded_callback_t cb = nullptr);
    static void copySLURLtoClipboard(const LLUUID& landmarkInventoryItemID);
private:
    LLLandmarkActions() = delete;
    LLLandmarkActions(const LLLandmarkActions&) = delete;
	static void onRegionResponseSLURL(slurl_callback_t cb,
								 const LLVector3d& global_pos,
								 bool escaped,
								 const std::string& url);
	static void onRegionResponseNameAndCoords(region_name_and_coords_callback_t cb,
								 const LLVector3d& global_pos,
								 U64 region_handle);
};
#endif
