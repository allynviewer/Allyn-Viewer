/** 
 * @file llassettype.h
 * @brief Declaration of LLAssetType.
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
#ifndef LL_LLASSETTYPE_H
#define LL_LLASSETTYPE_H
#include <string>
#include "stdenums.h"
class LL_COMMON_API LLAssetType
{
public:
	enum EType
	{
		AT_TEXTURE = 0,
		AT_SOUND = 1,
		AT_CALLINGCARD = 2,
		AT_LANDMARK = 3,
		AT_SCRIPT = 4,
		AT_CLOTHING = 5,
		AT_OBJECT = 6,
		AT_NOTECARD = 7,
		AT_CATEGORY = 8,
		AT_LSL_TEXT = 10,
		AT_LSL_BYTECODE = 11,
		AT_TEXTURE_TGA = 12,
		AT_BODYPART = 13,
		AT_SOUND_WAV = 17,
		AT_IMAGE_TGA = 18,
		AT_IMAGE_JPEG = 19,
		AT_ANIMATION = 20,
		AT_GESTURE = 21,
		AT_SIMSTATE = 22,
		AT_LINK = 24,
		AT_LINK_FOLDER = 25,
		AT_MARKETPLACE_FOLDER = 26,
		AT_CURRENT_OUTFIT = 46,
		AT_OUTFIT = 47,
		AT_MY_OUTFITS = 48,
		AT_MESH = 49,
		AT_RESERVED_1 = 50,
		AT_RESERVED_2 = 51,
		AT_RESERVED_3 = 52,
		AT_RESERVED_4 = 53,
		AT_RESERVED_5 = 54,
		AT_RESERVED_6 = 55,
		AT_SETTINGS = 56,
		AT_COUNT = 57,
		AT_UNKNOWN = 255,
		AT_NONE = -1
	};
	static EType 				lookup(const char* name);
	static EType 				lookup(const std::string& type_name);
	static const char*			lookup(EType asset_type);
	static EType 				lookupHumanReadable(const char* desc_name);
	static EType 				lookupHumanReadable(const std::string& readable_name);
	static const char*			lookupHumanReadable(EType asset_type);
	static EType 				getType(const std::string& desc_name);
	static const std::string&	getDesc(EType asset_type);
	static bool 				lookupCanLink(EType asset_type);
	static bool 				lookupIsLinkType(EType asset_type);
	static bool 				lookupIsAssetFetchByIDAllowed(EType asset_type);
	static bool 				lookupIsAssetIDKnowable(EType asset_type);
	static const std::string&	badLookup();
protected:
	LLAssetType() {}
	~LLAssetType() {}
};
#ifdef CWDEBUG
#include <iosfwd>
inline std::ostream& operator<<(std::ostream& os, LLAssetType::EType type) { return os << LLAssetType::getDesc(type); }
#endif
#endif
