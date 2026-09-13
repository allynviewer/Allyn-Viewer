/** 
 * @file llextendedstatus.h
 * @date   August 2007
 * @brief extended status codes for curl/vfs/resident asset storage and delivery
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#ifndef LL_LLEXTENDEDSTATUS_H
#define LL_LLEXTENDEDSTATUS_H
typedef S32 LLExtStat;
const LLExtStat LL_EXSTAT_CURL_RESULT	= 1L<<30;
const LLExtStat LL_EXSTAT_RES_RESULT	= 2L<<30;
const LLExtStat LL_EXSTAT_VFS_RESULT	= 3L<<30;
const LLExtStat LL_EXSTAT_NONE				= 0x00000;
const LLExtStat LL_EXSTAT_NULL_UUID			= 0x10001;
const LLExtStat LL_EXSTAT_NO_UPSTREAM		= 0x10002;
const LLExtStat LL_EXSTAT_REQUEST_DROPPED	= 0x10003;
const LLExtStat LL_EXSTAT_NONEXISTENT_FILE	= 0x10004;
const LLExtStat LL_EXSTAT_BLOCKED_FILE		= 0x10005;
const LLExtStat LL_EXSTAT_VFS_CACHED	= LL_EXSTAT_VFS_RESULT | 0x0001;
const LLExtStat LL_EXSTAT_VFS_CORRUPT	= LL_EXSTAT_VFS_RESULT | 0x0002;
#endif
