/** 
 * @file llurlwhitelist.cpp
 * @author Callum Prentice
 * @brief maintains a "white list" of acceptable URLS that are stored on disk
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "llurlwhitelist.h"
#include <iostream>
#include <fstream>
LLUrlWhiteList* LLUrlWhiteList::sInstance = 0;
LLUrlWhiteList::LLUrlWhiteList () :
	mLoaded ( false ),
	mFilename ( "url_whitelist.ini" ),
	mUrlList ( 0 ),
	mCurIndex ( 0 )
{
}
LLUrlWhiteList::~LLUrlWhiteList ()
{
}
void LLUrlWhiteList::initClass ()
{
    if ( ! sInstance )
	{
        sInstance = new LLUrlWhiteList ();
	}
}
void LLUrlWhiteList::cleanupClass ()
{
	delete sInstance;
	sInstance = NULL;
}
LLUrlWhiteList* LLUrlWhiteList::getInstance ()
{
	return sInstance;
}
bool LLUrlWhiteList::load ()
{
	if ( mLoaded )
		return ( true );
	clear ();
	std::string resolvedFilename = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, mFilename );
	llifstream file ( resolvedFilename );
	if ( file.is_open () )
	{
		std::string line;
		while ( std::getline ( file, line ) )
		{
			addItem ( line, false );
		};
		file.close ();
		mLoaded = true;
		return true;
	};
	return false;
}
bool LLUrlWhiteList::save ()
{
	std::string resolvedFilename = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, mFilename );
	if (resolvedFilename.empty())
	{
		LL_INFOS() << "No per-user dir for saving URL whitelist - presumably not logged in yet.  Skipping." << LL_ENDL;
		return false;
	}
	llofstream file ( resolvedFilename );
	if ( file.is_open () )
	{
		for ( string_list_t::iterator iter = mUrlList.begin (); iter != mUrlList.end (); ++iter )
		{
			file << ( *iter ) << std::endl;
		}
		file.close ();
		return true;
	};
	return false;
}
bool LLUrlWhiteList::clear ()
{
	mUrlList.clear ();
	mCurIndex = 0;
	return true;
}
std::string url_cleanup(std::string pattern)
{
	LLStringUtil::trim(pattern);
	S32 length = pattern.length();
	S32 position = 0;
	std::string::reverse_iterator it = pattern.rbegin();
	++it;
	++position;
	for (; it < pattern.rend(); ++it)
	{
		char c = *it;
		if (c == '/')
		{
			S32 desired_length = length - position;
			LLStringUtil::truncate(pattern, desired_length);
			break;
		}
		++position;
	}
	return pattern;
}
bool LLUrlWhiteList::addItem ( const std::string& itemIn, bool saveAfterAdd )
{
	std::string item = url_cleanup(itemIn);
	mUrlList.push_back ( item );
	if ( saveAfterAdd )
		save ();
	return true;
}
bool LLUrlWhiteList::getFirst ( std::string& valueOut )
{
	if ( mUrlList.size () == 0 )
		return false;
	mCurIndex = 0;
	valueOut = mUrlList[mCurIndex++];
	return true;
}
bool LLUrlWhiteList::getNext ( std::string& valueOut )
{
	if ( mCurIndex >= mUrlList.size () )
		return false;
	valueOut = mUrlList[mCurIndex++];
	return true;
}
bool LLUrlWhiteList::containsMatch ( const std::string& patternIn )
{
	return false;
}
