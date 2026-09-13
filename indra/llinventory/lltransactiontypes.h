/** 
 * @file lltransactiontypes.h
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
#ifndef LL_LLTRANSACTIONTYPES_H
#define LL_LLTRANSACTIONTYPES_H
const U8 TRANS_FAIL_SIMULATOR_TIMEOUT	= 1;
const U8 TRANS_FAIL_DATASERVER_TIMEOUT	= 2;
const U8 TRANS_FAIL_APPLICATION         = 3;
const S32 TRANS_NULL				= 0;
const S32 TRANS_OBJECT_CLAIM		= 1000;
const S32 TRANS_LAND_CLAIM			= 1001;
const S32 TRANS_GROUP_CREATE		= 1002;
const S32 TRANS_OBJECT_PUBLIC_CLAIM	= 1003;
const S32 TRANS_GROUP_JOIN		    = 1004;
const S32 TRANS_TELEPORT_CHARGE		= 1100;
const S32 TRANS_UPLOAD_CHARGE		= 1101;
const S32 TRANS_LAND_AUCTION		= 1102;
const S32 TRANS_CLASSIFIED_CHARGE	= 1103;
const S32 TRANS_OBJECT_TAX			= 2000;
const S32 TRANS_LAND_TAX			= 2001;
const S32 TRANS_LIGHT_TAX			= 2002;
const S32 TRANS_PARCEL_DIR_FEE		= 2003;
const S32 TRANS_GROUP_TAX		    = 2004;
const S32 TRANS_CLASSIFIED_RENEW	= 2005;
const S32 TRANS_RECURRING_GENERIC  = 2100;
const S32 TRANS_GIVE_INVENTORY		= 3000;
const S32 TRANS_OBJECT_SALE			= 5000;
const S32 TRANS_GIFT				= 5001;
const S32 TRANS_LAND_SALE			= 5002;
const S32 TRANS_REFER_BONUS			= 5003;
const S32 TRANS_INVENTORY_SALE		= 5004;
const S32 TRANS_REFUND_PURCHASE		= 5005;
const S32 TRANS_LAND_PASS_SALE		= 5006;
const S32 TRANS_DWELL_BONUS			= 5007;
const S32 TRANS_PAY_OBJECT			= 5008;
const S32 TRANS_OBJECT_PAYS			= 5009;
const S32 TRANS_RECURRING_GENERIC_USER  = 5100;
const S32 TRANS_GROUP_LAND_DEED		= 6001;
const S32 TRANS_GROUP_OBJECT_DEED	= 6002;
const S32 TRANS_GROUP_LIABILITY		= 6003;
const S32 TRANS_GROUP_DIVIDEND		= 6004;
const S32 TRANS_MEMBERSHIP_DUES		= 6005;
const S32 TRANS_OBJECT_RELEASE		= 8000;
const S32 TRANS_LAND_RELEASE		= 8001;
const S32 TRANS_OBJECT_DELETE		= 8002;
const S32 TRANS_OBJECT_PUBLIC_DECAY	= 8003;
const S32 TRANS_OBJECT_PUBLIC_DELETE= 8004;
const S32 TRANS_LINDEN_ADJUSTMENT	= 9000;
const S32 TRANS_LINDEN_GRANT		= 9001;
const S32 TRANS_LINDEN_PENALTY		= 9002;
const S32 TRANS_EVENT_FEE			= 9003;
const S32 TRANS_EVENT_PRIZE			= 9004;
const S32 TRANS_STIPEND_BASIC		= 10000;
const S32 TRANS_STIPEND_DEVELOPER	= 10001;
const S32 TRANS_STIPEND_ALWAYS		= 10002;
const S32 TRANS_STIPEND_DAILY		= 10003;
const S32 TRANS_STIPEND_RATING		= 10004;
const S32 TRANS_STIPEND_DELTA       = 10005;
#endif
