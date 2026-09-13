/** 
 * @file llinventorydefines.h
 * @brief LLInventoryDefines
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
#ifndef LL_LLINVENTORYDEFINES_H
#define LL_LLINVENTORYDEFINES_H
extern const U8 TASK_INVENTORY_ITEM_KEY;
extern const U8 TASK_INVENTORY_ASSET_KEY;
enum
{
	MAX_INVENTORY_BUFFER_SIZE = 1024
};
class LLInventoryItemFlags
{
public:
	enum EType
	{
		II_FLAGS_NONE 								= 0,
		II_FLAGS_SHARED_SINGLE_REFERENCE 			= 0x40000000,
		II_FLAGS_LANDMARK_VISITED 					= 1,
		II_FLAGS_OBJECT_SLAM_PERM 					= 0x100,
		II_FLAGS_OBJECT_SLAM_SALE 					= 0x1000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_BASE			= 0x010000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_OWNER		= 0x020000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP		= 0x040000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE		= 0x080000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_NEXT_OWNER	= 0x100000,
		II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS			= 0x200000,
		II_FLAGS_SUBTYPE_MASK                       = 0x0000ff,
		II_FLAGS_PERM_OVERWRITE_MASK = 				(II_FLAGS_OBJECT_SLAM_PERM |
													 II_FLAGS_OBJECT_SLAM_SALE |
													 II_FLAGS_OBJECT_PERM_OVERWRITE_BASE |
													 II_FLAGS_OBJECT_PERM_OVERWRITE_OWNER |
													 II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP |
													 II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE |
													 II_FLAGS_OBJECT_PERM_OVERWRITE_NEXT_OWNER),
	};
};
#endif
