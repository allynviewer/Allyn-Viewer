/** 
 * @file llfocusmgr.cpp
 * @brief LLFocusMgr base class
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
#include "linden_common.h"
#include "llfocusmgr.h"
#include "lluictrl.h"
#include "v4color.h"
const F32 FOCUS_FADE_TIME = 0.3f;
LLFocusableElement::LLFocusableElement()
:	mFocusLostCallback(nullptr),
	mFocusReceivedCallback(nullptr),
	mFocusChangedCallback(nullptr),
	mTopLostCallback(nullptr)
{
}
BOOL LLFocusableElement::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	return FALSE;
}
BOOL LLFocusableElement::handleKeyUp(KEY key, MASK mask, BOOL called_from_parent)
{
	return FALSE;
}
BOOL LLFocusableElement::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	return FALSE;
}
bool LLFocusableElement::wantsKeyUpKeyDown() const
{
    return false;
}
bool LLFocusableElement::wantsReturnKey() const
{
    return false;
}
LLFocusableElement::~LLFocusableElement()
{
	gFocusMgr.removeKeyboardFocusWithoutCallback(this);
	delete mFocusLostCallback;
	delete mFocusReceivedCallback;
	delete mFocusChangedCallback;
	delete mTopLostCallback;
}
void LLFocusableElement::onFocusReceived()
{
	if (mFocusReceivedCallback) (*mFocusReceivedCallback)(this);
	if (mFocusChangedCallback) (*mFocusChangedCallback)(this);
}
void LLFocusableElement::onFocusLost()
{
	if (mFocusLostCallback) (*mFocusLostCallback)(this);
	if (mFocusChangedCallback) (*mFocusChangedCallback)(this);
}
void LLFocusableElement::onTopLost()
{
	if (mTopLostCallback) (*mTopLostCallback)(this);
}
BOOL LLFocusableElement::hasFocus() const
{
	return gFocusMgr.getKeyboardFocus() == this;
}
void LLFocusableElement::setFocus(BOOL b)
{
}
boost::signals2::connection LLFocusableElement::setFocusLostCallback( const focus_signal_t::slot_type& cb)
{
	if (!mFocusLostCallback) mFocusLostCallback = new focus_signal_t();
	return mFocusLostCallback->connect(cb);
}
boost::signals2::connection	LLFocusableElement::setFocusReceivedCallback(const focus_signal_t::slot_type& cb)
{
	if (!mFocusReceivedCallback) mFocusReceivedCallback = new focus_signal_t();
	return mFocusReceivedCallback->connect(cb);
}
boost::signals2::connection	LLFocusableElement::setFocusChangedCallback(const focus_signal_t::slot_type& cb)
{
	if (!mFocusChangedCallback) mFocusChangedCallback = new focus_signal_t();
	return mFocusChangedCallback->connect(cb);
}
boost::signals2::connection	LLFocusableElement::setTopLostCallback(const focus_signal_t::slot_type& cb)
{
	if (!mTopLostCallback) mTopLostCallback = new focus_signal_t();
	return mTopLostCallback->connect(cb);
}
typedef std::list<LLHandle<LLView> > view_handle_list_t;
typedef std::map<LLHandle<LLView>, LLHandle<LLView> > focus_history_map_t;
struct LLFocusMgr::Impl
{
	view_handle_list_t mCachedKeyboardFocusList;
	focus_history_map_t mFocusHistory;
};
LLFocusMgr gFocusMgr;
LLFocusMgr::LLFocusMgr()
:	mLockedView(nullptr ),
	mMouseCaptor(nullptr ),
	mKeyboardFocus(nullptr ),
	mLastKeyboardFocus(nullptr ),
	mDefaultKeyboardFocus(nullptr ),
	mLastDefaultKeyboardFocus(nullptr ),
	mKeystrokesOnly(FALSE),
	mTopCtrl(nullptr ),
	mAppHasFocus(TRUE),
	mImpl(new LLFocusMgr::Impl)
{
}
LLFocusMgr::~LLFocusMgr()
{
	mImpl->mFocusHistory.clear();
	delete mImpl;
	mImpl = nullptr;
}
void LLFocusMgr::releaseFocusIfNeeded( const LLView* view )
{
	if( childHasMouseCapture( view ) )
	{
		setMouseCapture(nullptr );
	}
	if( childHasKeyboardFocus( view ))
	{
		if (view == mLockedView)
		{
			mLockedView = nullptr;
			setKeyboardFocus(nullptr );
		}
		else
		{
			setKeyboardFocus( mLockedView );
		}
	}
	if( childIsTopCtrl( view ) )
	{
		setTopCtrl(nullptr);
	}
}
void LLFocusMgr::restoreDefaultKeyboardFocus(LLFocusableElement* current_default_focus)
{
	if (current_default_focus && mDefaultKeyboardFocus == current_default_focus)
	{
		setDefaultKeyboardFocus(mLastDefaultKeyboardFocus);
		mLastDefaultKeyboardFocus = nullptr;
	}
}
void LLFocusMgr::restoreKeyboardFocus(LLFocusableElement* current_focus)
{
	if (current_focus && mKeyboardFocus == current_focus)
	{
		setKeyboardFocus(mLastKeyboardFocus);
		mLastKeyboardFocus = nullptr;
	}
}
void LLFocusMgr::setKeyboardFocus(LLFocusableElement* new_focus, BOOL lock, BOOL keystrokes_only)
{
	static bool focus_dirty;
	focus_dirty = false;
	if (mLockedView &&
		(new_focus == nullptr ||
			(new_focus != mLockedView
			&& dynamic_cast<LLView*>(new_focus)
			&& !dynamic_cast<LLView*>(new_focus)->hasAncestor(mLockedView))))
	{
		return;
	}
	mKeystrokesOnly = keystrokes_only;
	if( new_focus != mKeyboardFocus )
	{
		mLastKeyboardFocus = mKeyboardFocus;
		mKeyboardFocus = new_focus;
		view_handle_list_t old_focus_list = mImpl->mCachedKeyboardFocusList;
		view_handle_list_t new_focus_list;
		for (LLView* ctrl = dynamic_cast<LLView*>(mKeyboardFocus); ctrl; ctrl = ctrl->getParent())
		{
			new_focus_list.push_back(ctrl->getHandle());
		}
		while (!new_focus_list.empty() &&
			   !old_focus_list.empty() &&
			   new_focus_list.back() == old_focus_list.back())
		{
			new_focus_list.pop_back();
			old_focus_list.pop_back();
		}
		for (view_handle_list_t::iterator old_focus_iter = old_focus_list.begin();
			 old_focus_iter != old_focus_list.end() && !focus_dirty;
			 old_focus_iter++)
		{
			LLView* old_focus_view = old_focus_iter->get();
			if (old_focus_view)
			{
				mImpl->mCachedKeyboardFocusList.pop_front();
				old_focus_view->onFocusLost();
			}
		}
		for (view_handle_list_t::reverse_iterator new_focus_riter = new_focus_list.rbegin();
			 new_focus_riter != new_focus_list.rend() && !focus_dirty;
			 new_focus_riter++)
		{
			LLView* new_focus_view = new_focus_riter->get();
			if (new_focus_view)
			{
                mImpl->mCachedKeyboardFocusList.push_front(new_focus_view->getHandle());
				new_focus_view->onFocusReceived();
			}
		}
		if (focus_dirty)
		{
			return;
		}
		if (mDefaultKeyboardFocus != nullptr && mKeyboardFocus == nullptr)
		{
			mDefaultKeyboardFocus->setFocus(TRUE);
		}
		LLView* focus_subtree = dynamic_cast<LLView*>(mKeyboardFocus);
		LLView* viewp = dynamic_cast<LLView*>(mKeyboardFocus);
		while(viewp)
		{
			if (viewp->isFocusRoot())
			{
				focus_subtree = viewp;
			}
			viewp = viewp->getParent();
		}
		if (focus_subtree)
		{
			LLView* focused_view = dynamic_cast<LLView*>(mKeyboardFocus);
			mImpl->mFocusHistory[focus_subtree->getHandle()] = focused_view ? focused_view->getHandle() : LLHandle<LLView>();
		}
	}
	if (lock)
	{
		lockFocus();
	}
	focus_dirty = true;
}
BOOL LLFocusMgr::childHasKeyboardFocus(const LLView* parent ) const
{
	LLView* focus_view = dynamic_cast<LLView*>(mKeyboardFocus);
	while( focus_view )
	{
		if( focus_view == parent )
		{
			return TRUE;
		}
		focus_view = focus_view->getParent();
	}
	return FALSE;
}
BOOL LLFocusMgr::childHasMouseCapture( const LLView* parent ) const
{
	if( mMouseCaptor && dynamic_cast<LLView*>(mMouseCaptor) != nullptr )
	{
		LLView* captor_view = (LLView*)mMouseCaptor;
		while( captor_view )
		{
			if( captor_view == parent )
			{
				return TRUE;
			}
			captor_view = captor_view->getParent();
		}
	}
	return FALSE;
}
void LLFocusMgr::removeKeyboardFocusWithoutCallback( const LLFocusableElement* focus )
{
	if (focus == mLockedView)
	{
		mLockedView = nullptr;
	}
	if (mKeyboardFocus == focus)
	{
		mKeyboardFocus = nullptr;
	}
	if (mLastKeyboardFocus == focus)
	{
		mLastKeyboardFocus = nullptr;
	}
	if (mDefaultKeyboardFocus == focus)
	{
		mDefaultKeyboardFocus = nullptr;
	}
	if (mLastDefaultKeyboardFocus == focus)
	{
		mLastDefaultKeyboardFocus = nullptr;
	}
}
bool LLFocusMgr::keyboardFocusHasAccelerators() const
{
	LLView* focus_view = dynamic_cast<LLView*>(mKeyboardFocus);
	while(focus_view)
	{
		if (focus_view->hasAccelerators())
		{
			return true;
		}
		focus_view = focus_view->getParent();
	}
	return false;
}
void LLFocusMgr::setMouseCapture( LLMouseHandler* new_captor )
{
	if( new_captor != mMouseCaptor )
	{
		LLMouseHandler* old_captor = mMouseCaptor;
		mMouseCaptor = new_captor;
		if (LLView::sDebugMouseHandling)
		{
			if (new_captor)
			{
				LL_INFOS() << "New mouse captor: " << new_captor->getName() << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "New mouse captor: NULL" << LL_ENDL;
			}
		}
		if( old_captor )
		{
			old_captor->onMouseCaptureLost();
		}
	}
}
void LLFocusMgr::removeMouseCaptureWithoutCallback( const LLMouseHandler* captor )
{
	if( mMouseCaptor == captor )
	{
		mMouseCaptor = nullptr;
	}
}
BOOL LLFocusMgr::childIsTopCtrl( const LLView* parent ) const
{
	LLView* top_view = (LLView*)mTopCtrl;
	while( top_view )
	{
		if( top_view == parent )
		{
			return TRUE;
		}
		top_view = top_view->getParent();
	}
	return FALSE;
}
void LLFocusMgr::setTopCtrl( LLUICtrl* new_top  )
{
	LLUICtrl* old_top = mTopCtrl;
	if( new_top != old_top )
	{
		mTopCtrl = new_top;
		if (old_top)
		{
			old_top->onTopLost();
		}
	}
}
void LLFocusMgr::removeTopCtrlWithoutCallback( const LLUICtrl* top_view )
{
	if( mTopCtrl == top_view )
	{
		mTopCtrl = nullptr;
	}
}
void LLFocusMgr::lockFocus()
{
	mLockedView = dynamic_cast<LLUICtrl*>(mKeyboardFocus);
}
void LLFocusMgr::unlockFocus()
{
	mLockedView = nullptr;
}
F32 LLFocusMgr::getFocusFlashAmt() const
{
	return clamp_rescale(mFocusFlashTimer.getElapsedTimeF32(), 0.f, FOCUS_FADE_TIME, 1.f, 0.f);
}
LLColor4 LLFocusMgr::getFocusColor() const
{
	static LLCachedControl<LLColor4> focus_color_cached(*LLUI::sColorsGroup,"FocusColor", LLColor4::white);
	LLColor4 focus_color = lerp(focus_color_cached, LLColor4::white, getFocusFlashAmt());
	if (!mAppHasFocus)
	{
		focus_color.mV[VALPHA] *= 0.4f;
	}
	return focus_color;
}
void LLFocusMgr::triggerFocusFlash()
{
	mFocusFlashTimer.reset();
}
void LLFocusMgr::setAppHasFocus(BOOL focus)
{
	if (!mAppHasFocus && focus)
	{
		triggerFocusFlash();
	}
	if (!focus && mTopCtrl)
	{
		setTopCtrl(nullptr);
	}
	mAppHasFocus = focus;
}
LLUICtrl* LLFocusMgr::getLastFocusForGroup(LLView* subtree_root) const
{
	if (subtree_root)
	{
		focus_history_map_t::const_iterator found_it = mImpl->mFocusHistory.find(subtree_root->getHandle());
		if (found_it != mImpl->mFocusHistory.end())
		{
			return static_cast<LLUICtrl*>(found_it->second.get());
		}
	}
	return nullptr;
}
void LLFocusMgr::clearLastFocusForGroup(LLView* subtree_root)
{
	if (subtree_root)
	{
		mImpl->mFocusHistory.erase(subtree_root->getHandle());
	}
}
