/**
 * @file   llviewmodel.h
 * @author Nat Goodspeed
 * @date   2008-08-08
 * @brief  Define "View Model" classes intended to store data values for use
 *         by LLUICtrl subclasses. The phrase is borrowed from Microsoft
 *         terminology, in which "View Model" means the storage object
 *         underlying a specific widget object -- as in our case -- rather
 *         than the business "model" object underlying the overall "view"
 *         presented by the collection of widgets.
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
#if ! defined(LL_LLVIEWMODEL_H)
#define LL_LLVIEWMODEL_H
#include "llpointer.h"
#include "llsd.h"
#include "llrefcount.h"
#include "stdenums.h"
#include "llstring.h"
#include <string>
class LLScrollListItem;
class LLViewModel;
class LLTextViewModel;
class LLListViewModel;
typedef LLPointer<LLViewModel> LLViewModelPtr;
typedef LLPointer<LLTextViewModel> LLTextViewModelPtr;
typedef LLPointer<LLListViewModel> LLListViewModelPtr;
class LLViewModel: public LLRefCount
{
public:
    LLViewModel();
    LLViewModel(const LLSD& value);
    virtual void setValue(const LLSD& value);
    virtual LLSD getValue() const;
    bool isDirty() const { return mDirty; }
    void resetDirty() { mDirty = false; }
    void setDirty() { mDirty = true; }
protected:
    LLSD mValue;
    bool mDirty;
};
class LLTextViewModel: public LLViewModel
{
public:
    LLTextViewModel();
    LLTextViewModel(const LLSD& value);
    virtual void setValue(const LLSD& value);
    virtual LLSD getValue() const;
    const LLWString& getDisplay() const { return mDisplay; }
    void setDisplay(const LLWString& value);
private:
    LLWString mDisplay;
    bool mUpdateFromDisplay;
};
class LLListViewModel: public LLViewModel
{
public:
    LLListViewModel() {}
    LLListViewModel(const LLSD& values);
    virtual void addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
    virtual void clearColumns();
    virtual void setColumnLabel(const std::string& column, const std::string& label);
    virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM,
                                         void* userdata = NULL);
    virtual LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos,
                                               const LLSD& id);
    virtual void clearRows();
    virtual void sortByColumn(const std::string& name, bool ascending);
};
#endif
