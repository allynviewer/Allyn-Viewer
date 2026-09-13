/** 
 * @file llwearablelist.h
 * @brief LLWearableList class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLWEARABLELIST_H
#define LL_LLWEARABLELIST_H
#include "llmemory.h"
#include "llviewerwearable.h"
#include "lluuid.h"
#include "llassetstorage.h"
class LLWearableList : public LLSingleton<LLWearableList>
{
public:
	LLWearableList()	{}
	~LLWearableList();
	void cleanup() ;
	S32					getLength() const { return mList.size(); }
	void				getAsset(const LLAssetID& assetID,
								 const std::string& wearable_name,
								 LLAvatarAppearance *avatarp,
								 LLAssetType::EType asset_type,
								 void(*asset_arrived_callback)(LLViewerWearable*, void* userdata),
								 void* userdata);
	LLViewerWearable*			createCopy(const LLViewerWearable* old_wearable, const std::string& new_name = std::string(), const std::string& new_description = std::string());
	LLViewerWearable*			createNewWearable(LLWearableType::EType type, LLAvatarAppearance *avatarp);
	static void	 	    processGetAssetReply(const char* filename, const LLAssetID& assetID, void* user_data, S32 status, LLExtStat ext_status);
protected:
	LLViewerWearable* generateNewWearable();
private:
	std::map<LLUUID, LLViewerWearable*> mList;
};
#endif
