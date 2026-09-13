/** 
 * @file llresmgr.cpp
 * @brief Localized resource manager
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
#include "linden_common.h"
#include "llresmgr.h"
#include "llimagegl.h"
#include "llfontgl.h"
#include "llerror.h"
#include "llstring.h"
LLResMgr::LLResMgr()
{
	U32 i;
	for( i=0; i<LLFONT_COUNT; i++ )
	{
		mUSAFonts[i] = NULL;
	}
	mUSAFonts[ LLFONT_OCRA ]			= LLFontGL::getFontMonospace();
	mUSAFonts[ LLFONT_SANSSERIF ]		= LLFontGL::getFontSansSerif();
	mUSAFonts[ LLFONT_SANSSERIF_SMALL ]	= LLFontGL::getFontSansSerifSmall();
	mUSAFonts[ LLFONT_SANSSERIF_BIG ]	= LLFontGL::getFontSansSerifBig();
	mUSAFonts[ LLFONT_SMALL ]			= LLFontGL::getFontMonospace();
	for( i=0; i<LLFONT_COUNT; i++ )
	{
		mUKFonts[i] = mUSAFonts[i];
	}
	setLocale( LLLOCALE_USA );
}
void LLResMgr::setLocale( LLLOCALE_ID locale_id )
{
	mLocale = locale_id;
	switch( locale_id )
	{
	case LLLOCALE_USA:
		mFonts		= mUSAFonts;
		break;
	case LLLOCALE_UK:
		mFonts		= mUKFonts;
		break;
	default:
		llassert(0);
		setLocale(LLLOCALE_USA);
		break;
	}
}
char LLResMgr::getDecimalPoint() const
{
	char decimal = localeconv()->decimal_point[0];
	return decimal;
}
char LLResMgr::getThousandsSeparator() const
{
	char separator = localeconv()->thousands_sep[0];
	return separator;
}
char LLResMgr::getMonetaryDecimalPoint() const
{
	char decimal = localeconv()->mon_decimal_point[0];
	return decimal;
}
char LLResMgr::getMonetaryThousandsSeparator() const
{
	char separator = localeconv()->mon_thousands_sep[0];
	return separator;
}
std::string LLResMgr::getMonetaryString( S32 input ) const
{
	std::string output;
	LLLocale locale(LLLocale::USER_LOCALE);
	struct lconv *conv = localeconv();
	char* negative_sign = conv->negative_sign;
	char separator = getMonetaryThousandsSeparator();
	char* grouping = conv->mon_grouping;
	BOOL negative = (input < 0 );
	BOOL negative_before = negative && (conv->n_sign_posn != 2);
	BOOL negative_after = negative && (conv->n_sign_posn == 2);
	std::string digits = llformat("%u", abs(input));
	if( !grouping || !grouping[0] )
	{
		if( negative_before )
		{
			output.append( negative_sign );
		}
		output.append( digits );
		if( negative_after )
		{
			output.append( negative_sign );
		}
		return output;
	}
	S32 groupings[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	S32 cur_group;
	for( cur_group = 0; grouping[ cur_group ]; cur_group++ )
	{
		if( grouping[ cur_group ] != ';' )
		{
			groupings[cur_group] = grouping[ cur_group ];
		}
		cur_group++;
		if( groupings[cur_group] < 0 )
		{
			break;
		}
	}
	S32 group_count = cur_group;
	char reversed_output[20] = "";
	char forward_output[20] = "";
	S32 output_pos = 0;
	cur_group = 0;
	S32 pos = digits.size()-1;
	S32 count_within_group = 0;
	while( (pos >= 0) && (groupings[cur_group] >= 0) )
	{
		count_within_group++;
		if( count_within_group > groupings[cur_group] )
		{
			count_within_group = 1;
			reversed_output[ output_pos++ ] = separator;
			if( (cur_group + 1) >= group_count )
			{
				break;
			}
			else
			if( groupings[cur_group + 1] > 0 )
			{
				cur_group++;
			}
		}
		reversed_output[ output_pos++ ] = digits[pos--];
	}
	while( pos >= 0 )
	{
		reversed_output[ output_pos++ ] = digits[pos--];
	}
	reversed_output[ output_pos ] = '\0';
	forward_output[ output_pos ] = '\0';
	for( S32 i = 0; i < output_pos; i++ )
	{
		forward_output[ output_pos - 1 - i ] = reversed_output[ i ];
	}
	if( negative_before )
	{
		output.append( negative_sign );
	}
	output.append( forward_output );
	if( negative_after )
	{
		output.append( negative_sign );
	}
	return output;
}
void LLResMgr::getIntegerString( std::string& output, S32 input ) const
{
	if (input == 0)
	{
		output = "0";
		return;
	}
	S32 fraction = 0;
	std::string fraction_string;
	S32 remaining_count = input;
	while(remaining_count > 0)
	{
		fraction = (remaining_count) % 1000;
		if (!output.empty())
		{
			if (fraction == remaining_count)
			{
				fraction_string = llformat("%d%c", fraction, getThousandsSeparator());
			}
			else
			{
				fraction_string = llformat("%3.3d%c", fraction, getThousandsSeparator());
			}
			output = fraction_string + output;
		}
		else
		{
			if (fraction == remaining_count)
			{
				fraction_string = fmt::to_string(fraction);
			}
			else
			{
				fraction_string = llformat("%3.3d", fraction);
			}
			output = fraction_string;
		}
		remaining_count /= 1000;
	}
}
const std::string LLFONT_ID_NAMES[] =
{
	std::string("OCRA"),
	std::string("SANSSERIF"),
	std::string("SANSSERIF_SMALL"),
	std::string("SANSSERIF_BIG"),
	std::string("SMALL"),
};
const LLFontGL* LLResMgr::getRes( std::string font_id ) const
{
	for (S32 i=0; i<LLFONT_COUNT; ++i)
	{
		if (LLFONT_ID_NAMES[i] == font_id)
		{
			return getRes((LLFONT_ID)i);
		}
	}
	return NULL;
}
const std::string LLLocale::USER_LOCALE("English_United States.1252");
const std::string LLLocale::SYSTEM_LOCALE("English_United States.1252");
static std::string PrevFailedLocaleString = "";
LLLocale::LLLocale(const std::string& locale_string)
{
	mPrevLocaleString = setlocale( LC_ALL, NULL );
	char* new_locale_string = setlocale( LC_ALL, locale_string.c_str());
	if ( new_locale_string == NULL && PrevFailedLocaleString != locale_string )
	{
		LL_WARNS() << "Failed to set locale " << locale_string.c_str() << LL_ENDL;
		setlocale(LC_ALL, SYSTEM_LOCALE.c_str());
		PrevFailedLocaleString = locale_string;
	}
}
LLLocale::~LLLocale()
{
	setlocale( LC_ALL, mPrevLocaleString.c_str() );
}
