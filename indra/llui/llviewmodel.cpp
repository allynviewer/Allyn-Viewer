/**
 * @file   llviewmodel.cpp
 * @author Nat Goodspeed
 * @date   2008-08-08
 * @brief  Implementation for llviewmodel.
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#include "linden_common.h"
#include "llviewmodel.h"
LLViewModel::LLViewModel()
 : mDirty(false)
{
}
LLViewModel::LLViewModel(const LLSD& value)
  : mDirty(false)
{
    setValue(value);
}
void LLViewModel::setValue(const LLSD& value)
{
    mValue = value;
    mDirty = true;
}
LLSD LLViewModel::getValue() const
{
    return mValue;
}
LLTextViewModel::LLTextViewModel()
  : LLViewModel(false),
	mUpdateFromDisplay(false)
{
}
LLTextViewModel::LLTextViewModel(const LLSD& value)
  : LLViewModel(value),
	mUpdateFromDisplay(false)
{
}
void LLTextViewModel::setValue(const LLSD& value)
{
	LLViewModel::setValue(value);
    mDisplay = utf8str_to_wstring(value.asString());
    mUpdateFromDisplay = false;
}
void LLTextViewModel::setDisplay(const LLWString& value)
{
    mDisplay = value;
    mDirty = true;
    mUpdateFromDisplay = true;
}
LLSD LLTextViewModel::getValue() const
{
    if (mUpdateFromDisplay)
    {
        LLTextViewModel* nthis = const_cast<LLTextViewModel*>(this);
        nthis->mUpdateFromDisplay = false;
        nthis->mValue = wstring_to_utf8str(mDisplay);
    }
    return LLViewModel::getValue();
}
LLListViewModel::LLListViewModel(const LLSD& values)
  : LLViewModel()
{
}
void LLListViewModel::addColumn(const LLSD& column, EAddPosition pos)
{
}
void LLListViewModel::clearColumns()
{
}
void LLListViewModel::setColumnLabel(const std::string& column, const std::string& label)
{
}
LLScrollListItem* LLListViewModel::addElement(const LLSD& value, EAddPosition pos,
                                         void* userdata)
{
    return NULL;
}
LLScrollListItem* LLListViewModel::addSimpleElement(const std::string& value, EAddPosition pos,
                                               const LLSD& id)
{
    return NULL;
}
void LLListViewModel::clearRows()
{
}
void LLListViewModel::sortByColumn(const std::string& name, bool ascending)
{
}
