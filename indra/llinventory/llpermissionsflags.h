/** 
 * @file llpermissionsflags.h
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LL_LLPERMISSIONSFLAGS_H
#define LL_LLPERMISSIONSFLAGS_H
typedef U32 PermissionMask;
typedef U32 PermissionBit;
const PermissionBit PERM_TRANSFER           = (1 << 13);
const PermissionBit PERM_MODIFY				= (1 << 14);
const PermissionBit PERM_COPY				= (1 << 15);
const PermissionBit PERM_EXPORT				= (1 << 16);
const PermissionBit PERM_MOVE				= (1 << 19);
const PermissionBit PERM_RESERVED			= ((U32)1) << 31;
const PermissionMask PERM_NONE				= 0x00000000;
const PermissionMask PERM_ALL				= 0x7FFFFFFF;
const PermissionMask PERM_ITEM_UNRESTRICTED =  PERM_MODIFY | PERM_COPY | PERM_TRANSFER;
const U8 PERM_BASE		= 0x01;
const U8 PERM_OWNER		= 0x02;
const U8 PERM_GROUP		= 0x04;
const U8 PERM_EVERYONE	= 0x08;
const U8 PERM_NEXT_OWNER = 0x10;
const U8 PERM_SET_TRUE	= 0x1;
const U8 PERM_SET_FALSE = 0x0;
#endif
