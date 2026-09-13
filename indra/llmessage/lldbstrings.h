/** 
 * @file lldbstrings.h
 * @brief Database String Lengths.
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
#ifndef LL_LLDBSTRINGS_H
#define LL_LLDBSTRINGS_H
const S32 DB_INV_ITEM_NAME_STR_LEN		= 63;
const S32 DB_INV_ITEM_NAME_BUF_SIZE		= 64;
const S32 DB_INV_ITEM_DESC_STR_LEN		= 127;
const S32 DB_INV_ITEM_DESC_BUF_SIZE		= 128;
const S32 DB_GROUP_NAME_STR_LEN			= 35;
const S32 DB_GROUP_NAME_BUF_SIZE		= 36;
const S32 DB_GROUP_NAME_MIN_LEN			= 4;
const U32 DB_GROUP_ROLE_NAME_STR_LEN     = 20;
const U32 DB_GROUP_ROLE_NAME_BUF_SIZE    = DB_GROUP_ROLE_NAME_STR_LEN + 1;
const U32 DB_GROUP_ROLE_TITLE_STR_LEN    = 20;
const U32 DB_GROUP_ROLE_TITLE_BUF_SIZE   = DB_GROUP_ROLE_TITLE_STR_LEN + 1;
const S32 DB_GROUP_CHARTER_STR_LEN		= 511;
const S32 DB_GROUP_CHARTER_BUF_SIZE		= 512;
const S32 DB_GROUP_TITLE_STR_LEN		= 20;
const S32 DB_GROUP_TITLE_BUF_SIZE		= 21;
const S32 MAX_MSG_STR_LEN = 1023;
const S32 MAX_MSG_BUF_SIZE = 1024;
const S32 DB_IM_MSG_STR_LEN	 			= MAX_MSG_STR_LEN;
const S32 DB_IM_MSG_BUF_SIZE 			= MAX_MSG_BUF_SIZE;
const S32 DB_GROUP_NOTICE_SUBJ_STR_LEN	= 63;
const S32 DB_GROUP_NOTICE_SUBJ_STR_SIZE	= 64;
const S32 DB_GROUP_NOTICE_MSG_STR_LEN	= MAX_MSG_STR_LEN - DB_GROUP_NOTICE_SUBJ_STR_LEN;
const S32 DB_GROUP_NOTICE_MSG_STR_SIZE	= MAX_MSG_BUF_SIZE - DB_GROUP_NOTICE_SUBJ_STR_SIZE;
const S32 DB_CHAT_MSG_STR_LEN			= MAX_MSG_STR_LEN;
const S32 DB_CHAT_MSG_BUF_SIZE 			= MAX_MSG_BUF_SIZE;
const S32 DB_STIPEND_DESC_STR_LEN		= 254;
const S32 DB_STIPEND_DESC_BUF_SIZE 		= 255;
const S32 DB_EMAIL_FROM_STR_LEN			= 78;
const S32 DB_EMAIL_FROM_BUF_SIZE		= 79;
const S32 DB_EMAIL_SUBJECT_STR_LEN		= 72;
const S32 DB_EMAIL_SUBJECT_BUF_SIZE		= 73;
const S32 DB_MOTD_STR_LEN				= 254;
const S32 DB_MOTD_BUF_SIZE				= 255;
const S32 DB_FULL_NAME_STR_LEN			= 63;
const S32 DB_FULL_NAME_BUF_SIZE			= 64;
const S32 DB_FIRST_NAME_STR_LEN			= 31;
const S32 DB_FIRST_NAME_BUF_SIZE		= 32;
const S32 DB_LAST_NAME_STR_LEN			= 31;
const S32 DB_LAST_NAME_BUF_SIZE			= 32;
const S32 DB_USER_PASSWORD_STR_LEN		= 100;
const S32 DB_USER_PASSWORD_BUF_SIZE		= 101;
const S32 DB_USER_EMAIL_ADDR_STR_LEN	= 254;
const S32 DB_USER_EMAIL_ADDR_BUF_SIZE	= 255;
const S32 DB_USER_ABOUT_STR_LEN			= 511;
const S32 DB_USER_ABOUT_BUF_SIZE		= 512;
const S32 DB_USER_FL_ABOUT_STR_LEN		= 254;
const S32 DB_USER_FL_ABOUT_BUF_SIZE		= 255;
const S32 DB_USER_PROFILE_URL_STR_LEN	= 254;
const S32 DB_USER_PROFILE_URL_BUF_SIZE	= 255;
const S32 DB_USER_WANT_TO_STR_LEN		= 254;
const S32 DB_USER_WANT_TO_BUF_SIZE		= 255;
const S32 DB_USER_SKILLS_STR_LEN		= 254;
const S32 DB_USER_SKILLS_BUF_SIZE		= 255;
const S32 DB_NV_NAME_STR_LEN			= 128;
const S32 DB_NV_NAME_BUF_SIZE			= 129;
const S32 DB_VOTE_TEXT_STR_LEN			= 254;
const S32 DB_VOTE_TEXT_BUF_SIZE			= 255;
const S32 DB_VOTE_TYPE_STR_LEN			= 9;
const S32 DB_VOTE_TYPE_BUF_SIZE			= 10;
const S32 DB_VOTE_RESULT_BUF_LEN		= 8;
const S32 DB_VOTE_RESULT_BUF_SIZE		= 9;
const S32 DB_START_LOCATION_STR_LEN		= 254;
const S32 DB_START_LOCATION_BUF_SIZE	= 255;
const S32 DB_BORN_STR_LEN				= 15;
const S32 DB_BORN_BUF_SIZE				= 16;
const S32 DB_PLACE_NAME_LEN				= 63;
const S32 DB_PLACE_NAME_SIZE			= 64;
const S32 DB_PARCEL_NAME_LEN			= 63;
const S32 DB_PARCEL_NAME_SIZE			= 64;
const S32 DB_PLACE_DESC_LEN				= 255;
const S32 DB_PLACE_DESC_SIZE			= 256;
const S32 DB_PARCEL_DESC_LEN			= 255;
const S32 DB_PARCEL_DESC_SIZE			= 256;
const S32 DB_PARCEL_MUSIC_URL_LEN		= 255;
const S32 DB_PARCEL_MEDIA_URL_LEN		= 255;
const S32 DB_PARCEL_MUSIC_URL_SIZE		= 256;
const S32 DB_DATETIME_STR_LEN			= 35;
const S32 DB_DATETIME_BUF_SIZE			= 36;
const S32 DB_TERSE_DATETIME_STR_LEN		= 15;
const S32 DB_TERSE_DATETIME_BUF_SIZE	= 16;
const S32 DB_SIM_NAME_STR_LEN			= 35;
const S32 DB_SIM_NAME_BUF_SIZE			= 36;
const S32 DB_HOST_NAME_STR_LEN			= 100;
const S32 DB_HOST_NAME_BUF_SIZE			= 101;
const S32 DB_ESTATE_NAME_STR_LEN		= 63;
const S32 DB_ESTATE_NAME_BUF_SIZE			= DB_ESTATE_NAME_STR_LEN + 1;
const S32 DB_USER_NOTE_LEN				= 1023;
const S32 DB_USER_NOTE_SIZE				= 1024;
const S32 DB_PICK_NAME_LEN				= 63;
const S32 DB_PICK_NAME_SIZE				= 64;
const S32 DB_PICK_DESC_LEN				= 1023;
const S32 DB_PICK_DESC_SIZE				= 1024;
#endif
