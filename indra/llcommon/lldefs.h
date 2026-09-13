/** 
 * @file lldefs.h
 * @brief Various generic constant definitions.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#ifndef LL_LLDEFS_H
#define LL_LLDEFS_H
#include "stdtypes.h"
const U32	VX			= 0;
const U32	VY			= 1;
const U32	VZ			= 2;
const U32	VW			= 3;
const U32	VS			= 3;
const U32	VRED		= 0;
const U32	VGREEN		= 1;
const U32	VBLUE		= 2;
const U32	VALPHA		= 3;
const U32	INVALID_DIRECTION = 0xFFFFFFFF;
const U32	EAST		= 0;
const U32	NORTH		= 1;
const U32	WEST		= 2;
const U32	SOUTH		= 3;
const U32	NORTHEAST	= 4;
const U32	NORTHWEST	= 5;
const U32	SOUTHWEST	= 6;
const U32	SOUTHEAST	= 7;
const U32	MIDDLE		= 8;
const U8	EAST_MASK		= 0x1<<EAST;
const U8	NORTH_MASK		= 0x1<<NORTH;
const U8	WEST_MASK		= 0x1<<WEST;
const U8	SOUTH_MASK		= 0x1<<SOUTH;
const U8	NORTHEAST_MASK	= NORTH_MASK | EAST_MASK;
const U8	NORTHWEST_MASK	= NORTH_MASK | WEST_MASK;
const U8	SOUTHWEST_MASK	= SOUTH_MASK | WEST_MASK;
const U8	SOUTHEAST_MASK	= SOUTH_MASK | EAST_MASK;
const U32 gDirOpposite[8] = {2, 3, 0, 1, 6, 7, 4, 5};
const U32 gDirAdjacent[8][2] =  {
								{4, 7},
								{4, 5},
								{5, 6},
								{6, 7},
								{0, 1},
								{1, 2},
								{2, 3},
								{0, 3}
								};
const S32 gDirAxes[8][2] = {
							{ 1, 0},
							{ 0, 1},
							{-1, 0},
							{ 0,-1},
							{ 1, 1},
							{-1, 1},
							{-1,-1},
							{ 1,-1},
							};
const S32 gDirMasks[8] = {
							EAST_MASK,
							NORTH_MASK,
							WEST_MASK,
							SOUTH_MASK,
							NORTHEAST_MASK,
							NORTHWEST_MASK,
							SOUTHWEST_MASK,
							SOUTHEAST_MASK
							};
const U32 NO_SIDE 		= 0;
const U32 FRONT_SIDE 	= 1;
const U32 BACK_SIDE 	= 2;
const U32 LEFT_SIDE 	= 3;
const U32 RIGHT_SIDE 	= 4;
const U32 TOP_SIDE 		= 5;
const U32 BOTTOM_SIDE 	= 6;
const U8 LL_SOUND_FLAG_NONE =         0x0;
const U8 LL_SOUND_FLAG_LOOP =         1<<0;
const U8 LL_SOUND_FLAG_SYNC_MASTER =  1<<1;
const U8 LL_SOUND_FLAG_SYNC_SLAVE =   1<<2;
const U8 LL_SOUND_FLAG_SYNC_PENDING = 1<<3;
const U8 LL_SOUND_FLAG_QUEUE =        1<<4;
const U8 LL_SOUND_FLAG_STOP =         1<<5;
const U8 LL_SOUND_FLAG_SYNC_MASK = LL_SOUND_FLAG_SYNC_MASTER | LL_SOUND_FLAG_SYNC_SLAVE | LL_SOUND_FLAG_SYNC_PENDING;
const U32	LL_MAX_PATH		= 1024;
const U32	STD_STRING_BUF_SIZE	= 255;
const U32	STD_STRING_STR_LEN	= 254;
const U32	MAX_STRING			= STD_STRING_BUF_SIZE;
const U32	MAXADDRSTR		= 17;
template <class LLDATATYPE>
inline LLDATATYPE llmax(const LLDATATYPE& d1, const LLDATATYPE& d2)
{
	return (d1 > d2) ? d1 : d2;
}
template <class LLDATATYPE>
inline LLDATATYPE llmax(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3)
{
	LLDATATYPE r = llmax(d1,d2);
	return llmax(r, d3);
}
template <class LLDATATYPE>
inline LLDATATYPE llmax(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3, const LLDATATYPE& d4)
{
	LLDATATYPE r1 = llmax(d1,d2);
	LLDATATYPE r2 = llmax(d3,d4);
	return llmax(r1, r2);
}
template <class LLDATATYPE>
inline LLDATATYPE llmin(const LLDATATYPE& d1, const LLDATATYPE& d2)
{
	return (d1 < d2) ? d1 : d2;
}
template <class LLDATATYPE>
inline LLDATATYPE llmin(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3)
{
	LLDATATYPE r = llmin(d1,d2);
	return (r < d3 ? r : d3);
}
template <class LLDATATYPE>
inline LLDATATYPE llmin(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3, const LLDATATYPE& d4)
{
	LLDATATYPE r1 = llmin(d1,d2);
	LLDATATYPE r2 = llmin(d3,d4);
	return llmin(r1, r2);
}
template <class LLDATATYPE>
inline LLDATATYPE llclamp(const LLDATATYPE& a, const LLDATATYPE& minval, const LLDATATYPE& maxval)
{
	if ( a < minval )
	{
		return minval;
	}
	else if ( a > maxval )
	{
		return maxval;
	}
	return a;
}
template <class LLDATATYPE>
inline LLDATATYPE llclampf(const LLDATATYPE& a)
{
	return llmin(llmax(a, (LLDATATYPE)0), (LLDATATYPE)1);
}
template <class LLDATATYPE>
inline LLDATATYPE llclampb(const LLDATATYPE& a)
{
	return llmin(llmax(a, (LLDATATYPE)0), (LLDATATYPE)255);
}
template <class LLDATATYPE>
inline void llswap(LLDATATYPE& lhs, LLDATATYPE& rhs)
{
	LLDATATYPE tmp = lhs;
	lhs = rhs;
	rhs = tmp;
}
#endif
