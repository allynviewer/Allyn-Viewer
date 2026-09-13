/** 
 * @file llscriptresource.cpp
 * @brief LLScriptResource class implementation for managing limited resources
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#include "llscriptresource.h"
#include "llerror.h"
LLScriptResource::LLScriptResource()
: mTotal(0),
  mUsed(0)
{
}
bool LLScriptResource::request(S32 amount )
{
	if (mUsed + amount <= mTotal)
	{
		mUsed += amount;
		return true;
	}
	return false;
}
bool LLScriptResource::release(S32 amount )
{
	if (mUsed >= amount)
	{
		mUsed -= amount;
		return true;
	}
	return false;
}
S32 LLScriptResource::getAvailable() const
{
	if (mUsed > mTotal)
	{
		return 0;
	}
	return (mTotal - mUsed);
}
void LLScriptResource::setTotal(S32 amount)
{
	mTotal = amount;
}
S32 LLScriptResource::getTotal() const
{
	return mTotal;
}
S32 LLScriptResource::getUsed() const
{
	return mUsed;
}
bool LLScriptResource::isOverLimit() const
{
	return (mUsed > mTotal);
}
