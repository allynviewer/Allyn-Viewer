/** 
 * @file llinventorytype.cpp
 * @brief Inventory item type, more specific than an asset type.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "linden_common.h"
#include "llinventorytype.h"
#include "lldictionary.h"
#include "llmemory.h"
#include "llsingleton.h"
static const std::string empty_string;
struct InventoryEntry : public LLDictionaryEntry
{
	InventoryEntry(const std::string &name,
				   const std::string &human_name,
				   int num_asset_types = 0, ...)
		:
		LLDictionaryEntry(name),
		mHumanName(human_name)
	{
		va_list argp;
		va_start(argp, num_asset_types);
		for (U8 i=0; i < num_asset_types; i++)
		{
			LLAssetType::EType t = (LLAssetType::EType)va_arg(argp,int);
			mAssetTypes.push_back(t);
		}
		va_end(argp);
	}
	const std::string mHumanName;
	typedef std::vector<LLAssetType::EType> asset_vec_t;
	asset_vec_t mAssetTypes;
};
class LLInventoryDictionary : public LLSingleton<LLInventoryDictionary>,
							  public LLDictionary<LLInventoryType::EType, InventoryEntry>
{
public:
	LLInventoryDictionary();
};
LLInventoryDictionary::LLInventoryDictionary()
{
	addEntry(LLInventoryType::IT_TEXTURE,             new InventoryEntry("texture",   "texture",       1, LLAssetType::AT_TEXTURE));
	addEntry(LLInventoryType::IT_SOUND,               new InventoryEntry("sound",     "sound",         1, LLAssetType::AT_SOUND));
	addEntry(LLInventoryType::IT_CALLINGCARD,         new InventoryEntry("callcard",  "calling card",  1, LLAssetType::AT_CALLINGCARD));
	addEntry(LLInventoryType::IT_LANDMARK,            new InventoryEntry("landmark",  "landmark",      1, LLAssetType::AT_LANDMARK));
	addEntry(LLInventoryType::IT_OBJECT,              new InventoryEntry("object",    "object",        1, LLAssetType::AT_OBJECT));
	addEntry(LLInventoryType::IT_NOTECARD,            new InventoryEntry("notecard",  "note card",     1, LLAssetType::AT_NOTECARD));
	addEntry(LLInventoryType::IT_CATEGORY,            new InventoryEntry("category",  "folder"         ));
	addEntry(LLInventoryType::IT_ROOT_CATEGORY,       new InventoryEntry("root",      "root"           ));
	addEntry(LLInventoryType::IT_LSL,                 new InventoryEntry("script",    "script",        2, LLAssetType::AT_LSL_TEXT, LLAssetType::AT_LSL_BYTECODE));
	addEntry(LLInventoryType::IT_SNAPSHOT,            new InventoryEntry("snapshot",  "snapshot",      1, LLAssetType::AT_TEXTURE));
	addEntry(LLInventoryType::IT_ATTACHMENT,          new InventoryEntry("attach",    "attachment",    1, LLAssetType::AT_OBJECT));
	addEntry(LLInventoryType::IT_WEARABLE,            new InventoryEntry("wearable",  "wearable",      2, LLAssetType::AT_CLOTHING, LLAssetType::AT_BODYPART));
	addEntry(LLInventoryType::IT_ANIMATION,           new InventoryEntry("animation", "animation",     1, LLAssetType::AT_ANIMATION));
	addEntry(LLInventoryType::IT_GESTURE,             new InventoryEntry("gesture",   "gesture",       1, LLAssetType::AT_GESTURE));
	addEntry(LLInventoryType::IT_MESH,                new InventoryEntry("mesh",      "mesh",          1, LLAssetType::AT_MESH));
	addEntry(LLInventoryType::IT_SETTINGS,            new InventoryEntry("settings",  "settings",      1, LLAssetType::AT_SETTINGS));
}
static const LLInventoryType::EType
DEFAULT_ASSET_FOR_INV_TYPE[LLAssetType::AT_COUNT] =
{
	LLInventoryType::IT_TEXTURE,
	LLInventoryType::IT_SOUND,
	LLInventoryType::IT_CALLINGCARD,
	LLInventoryType::IT_LANDMARK,
	LLInventoryType::IT_LSL,
	LLInventoryType::IT_WEARABLE,
	LLInventoryType::IT_OBJECT,
	LLInventoryType::IT_NOTECARD,
	LLInventoryType::IT_CATEGORY,
	LLInventoryType::IT_ROOT_CATEGORY,
	LLInventoryType::IT_LSL,
	LLInventoryType::IT_LSL,
	LLInventoryType::IT_TEXTURE,
	LLInventoryType::IT_WEARABLE,
	LLInventoryType::IT_CATEGORY,
	LLInventoryType::IT_CATEGORY,
	LLInventoryType::IT_CATEGORY,
	LLInventoryType::IT_SOUND,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_ANIMATION,
	LLInventoryType::IT_GESTURE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_MESH,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_NONE,
	LLInventoryType::IT_SETTINGS,
};
const std::string &LLInventoryType::lookup(EType type)
{
	const InventoryEntry *entry = LLInventoryDictionary::getInstance()->lookup(type);
	if (!entry) return empty_string;
	return entry->mName;
}
LLInventoryType::EType LLInventoryType::lookup(const std::string& name)
{
	return LLInventoryDictionary::getInstance()->lookup(name);
}
const std::string &LLInventoryType::lookupHumanReadable(EType type)
{
	const InventoryEntry *entry = LLInventoryDictionary::getInstance()->lookup(type);
	if (!entry) return empty_string;
	return entry->mHumanName;
}
LLInventoryType::EType LLInventoryType::defaultForAssetType(LLAssetType::EType asset_type)
{
	if((asset_type >= 0) && (asset_type < LLAssetType::AT_COUNT))
	{
		return DEFAULT_ASSET_FOR_INV_TYPE[S32(asset_type)];
	}
	else
	{
		return IT_UNKNOWN;
	}
}
bool LLInventoryType::cannotRestrictPermissions(LLInventoryType::EType type)
{
	switch(type)
	{
		case IT_CALLINGCARD:
		case IT_LANDMARK:
			return true;
		default:
			return false;
	}
}
bool LLInventoryType::showInWorldPermissions(LLInventoryType::EType type)
{
    return (type != IT_SETTINGS);
}
bool inventory_and_asset_types_match(LLInventoryType::EType inventory_type,
									 LLAssetType::EType asset_type)
{
	if (LLAssetType::lookupIsLinkType(asset_type))
		return true;
	const InventoryEntry *entry = LLInventoryDictionary::getInstance()->lookup(inventory_type);
	if (!entry) return false;
	for (InventoryEntry::asset_vec_t::const_iterator iter = entry->mAssetTypes.begin();
		 iter != entry->mAssetTypes.end();
		 iter++)
	{
		const LLAssetType::EType type = (*iter);
		if(type == asset_type)
		{
			return true;
		}
	}
	return false;
}
