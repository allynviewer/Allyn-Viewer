/** 
 * @file llview.cpp
 * @author James Cook
 * @brief Container for other views, anything that draws.
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
#include "llview.h"
#include <cassert>
#include <boost/tokenizer.hpp>
#include "llrender.h"
#include "llevent.h"
#include "llfontgl.h"
#include "llfocusmgr.h"
#include "llrect.h"
#include "llstl.h"
#include "llui.h"
#include "lluictrl.h"
#include "llwindow.h"
#include "v3color.h"
#include "lluictrlfactory.h"
#include "llbutton.h"
#include "lllineeditor.h"
#include "lltexteditor.h"
#include "lltextbox.h"
#include "llfasttimer.h"
using namespace LLOldEvents;
static LLRegisterWidget<LLView> r("view");
BOOL	LLView::sDebugRects = FALSE;
BOOL	LLView::sDebugKeys = FALSE;
S32		LLView::sDepth = 0;
BOOL	LLView::sDebugMouseHandling = FALSE;
std::string LLView::sMouseHandlerMessage;
BOOL	LLView::sEditingUI = FALSE;
BOOL	LLView::sForceReshape = FALSE;
LLView*	LLView::sEditingUIView = NULL;
S32		LLView::sLastLeftXML = S32_MIN;
S32		LLView::sLastBottomXML = S32_MIN;
std::vector<LLViewDrawContext*> LLViewDrawContext::sDrawContextStack;
LLView::DrilldownFunc LLView::sDrilldown =
	boost::bind(&LLView::pointInView, _1, _2, _3, HIT_TEST_USE_BOUNDING_RECT);
#if LL_DEBUG
BOOL LLView::sIsDrawing = FALSE;
#endif
LLView::Follows::Follows()
:   string(""),
	flags("flags", FOLLOWS_NONE)
{}
LLView::Params::Params()
:	name("name", std::string("unnamed")),
	enabled("enabled", true),
	visible("visible", true),
	mouse_opaque("mouse_opaque", true),
	follows("follows"),
	hover_cursor("hover_cursor", "UI_CURSOR_ARROW"),
	use_bounding_rect("use_bounding_rect", false),
	tab_group("tab_group", 0),
	default_tab_group("default_tab_group"),
	sound_flags("sound_flags", MOUSE_UP),
	layout("layout"),
	rect("rect"),
	bottom_delta("bottom_delta", S32_MAX),
	top_pad("top_pad"),
	top_delta("top_delta", S32_MAX),
	left_pad("left_pad"),
	left_delta("left_delta", S32_MAX),
	from_xui("from_xui", true),
	focus_root("focus_root", false),
	needs_translate("translate"),
	xmlns("xmlns"),
	xmlns_xsi("xmlns:xsi"),
	xsi_schemaLocation("xsi:schemaLocation"),
	xsi_type("xsi:type")
{
	addSynonym(rect, "");
}
LLView::LLView(const LLView::Params& p)
{
	init(p);
}
void LLView::init(const LLView::Params& p)
{
	mVisible = p.visible;
	mInDraw = false;
	mName = p.name;
	mParentView = NULL;
	mReshapeFlags = FOLLOWS_NONE;
	mSaveToXML = p.from_xui;
	mIsFocusRoot = p.focus_root;
	mLastVisible = FALSE;
	mNextInsertionOrdinal = 0;
	mHoverCursor = getCursorFromString(p.hover_cursor);
	mEnabled = p.enabled;
	mMouseOpaque = p.mouse_opaque;
	mSoundFlags = p.sound_flags;
	mUseBoundingRect = p.use_bounding_rect;
	mDefaultTabGroup = p.default_tab_group;
	mLastTabGroup = 0;
	setShape(p.rect);
	parseFollowsFlags(p);
}
LLView::LLView()
{
	init(LLView::Params());
}
LLView::LLView(const std::string& name, BOOL mouse_opaque)
{
	LLView::Params p;
	p.name = name;
	p.mouse_opaque = mouse_opaque;
	init(p);
}
LLView::LLView(	const std::string& name, const LLRect& rect, BOOL mouse_opaque, U32 reshape)
{
	LLView::Params p;
	p.name = name;
	p.mouse_opaque = mouse_opaque;
	p.rect = rect;
	p.follows.flags = reshape;
	init(p);
}
LLView::~LLView()
{
	if( hasMouseCapture() )
	{
		gFocusMgr.removeMouseCaptureWithoutCallback( this );
	}
	deleteAllChildren();
	if (mParentView != NULL)
	{
		mParentView->removeChild(this);
	}
	dispatch_list_t::iterator itor;
	for (itor = mDispatchList.begin(); itor != mDispatchList.end(); ++itor)
	{
		(*itor).second->clearDispatchers();
	}
	std::for_each(mFloaterControls.begin(), mFloaterControls.end(),
				  DeletePairedPointer());
	std::for_each(mDummyWidgets.begin(), mDummyWidgets.end(),
				  DeletePairedPointer());
}
BOOL LLView::isView() const
{
	return TRUE;
}
BOOL LLView::isCtrl() const
{
	return FALSE;
}
BOOL LLView::isPanel() const
{
	return FALSE;
}
void LLView::setToolTip(const LLStringExplicit& msg)
{
	mToolTipMsg = msg;
}
BOOL LLView::setToolTipArg(const LLStringExplicit& key, const LLStringExplicit& text)
{
	mToolTipMsg.setArg(key, text);
	return TRUE;
}
void LLView::setToolTipArgs( const LLStringUtil::format_map_t& args )
{
	mToolTipMsg.setArgList(args);
}
void LLView::setRect(const LLRect& rect)
{
	mRect = rect;
	updateBoundingRect();
}
void LLView::setUseBoundingRect( BOOL use_bounding_rect )
{
	if (mUseBoundingRect != use_bounding_rect)
	{
        mUseBoundingRect = use_bounding_rect;
		updateBoundingRect();
	}
}
BOOL LLView::getUseBoundingRect() const
{
	return mUseBoundingRect;
}
const std::string& LLView::getName() const
{
	static std::string no_name("(no name)");
	return mName.empty() ? no_name : mName;
}
void LLView::sendChildToFront(LLView* child)
{
	if (child && child->getParent() == this)
	{
		if (child != mChildList.front())
		{
			mChildList.remove( child );
			mChildList.push_front(child);
		}
	}
}
void LLView::sendChildToBack(LLView* child)
{
	if (child && child->getParent() == this)
	{
		if (child != mChildList.back())
		{
			mChildList.remove( child );
			mChildList.push_back(child);
		}
	}
}
void LLView::moveChildToFrontOfTabGroup(LLUICtrl* child)
{
	if(mCtrlOrder.find(child) != mCtrlOrder.end())
	{
		mCtrlOrder[child].second = -1 * mNextInsertionOrdinal++;
	}
}
void LLView::moveChildToBackOfTabGroup(LLUICtrl* child)
{
	if(mCtrlOrder.find(child) != mCtrlOrder.end())
	{
		mCtrlOrder[child].second = mNextInsertionOrdinal++;
	}
}
bool LLView::addChild(LLView* child, S32 tab_group)
{
	if (!child)
	{
		return false;
	}
	if (mParentView == child)
	{
		LL_ERRS() << "Adding view " << child->getName() << " as child of itself" << LL_ENDL;
	}
	if (child->mParentView)
	{
		child->mParentView->removeChild(child);
	}
	mChildList.push_front(child);
	mChildHashMap[child->getName()]=child;
	if (child->isCtrl())
	{
		LLUICtrl* ctrl = static_cast<LLUICtrl*>(child);
		mCtrlOrder.insert(tab_order_pair_t(ctrl,
							tab_order_t(tab_group, mNextInsertionOrdinal)));
		mNextInsertionOrdinal++;
	}
	child->mParentView = this;
	updateBoundingRect();
	mLastTabGroup = tab_group;
	return true;
}
bool LLView::addChildInBack(LLView* child, S32 tab_group)
{
	if(addChild(child, tab_group))
	{
		sendChildToBack(child);
		return true;
	}
	return false;
}
void LLView::removeChild(LLView* child)
{
	if (child->mParentView == this)
	{
		llassert(child->mInDraw == false);
		mChildList.remove( child );
		for(boost::container::flat_map<std::string, LLView*>::iterator it=mChildHashMap.begin(); it != mChildHashMap.end(); ++it)
		{
			if(it->second == child)
			{
				mChildHashMap.erase(it);
				break;
			}
		}
		child->mParentView = NULL;
		if (child->isCtrl())
		{
			child_tab_order_t::iterator found = mCtrlOrder.find(static_cast<LLUICtrl*>(child));
			if(found != mCtrlOrder.end())
			{
				mCtrlOrder.erase(found);
			}
		}
	}
	else
	{
		LL_WARNS() << child->getName() << "is not a child of " << getName() << LL_ENDL;
	}
	updateBoundingRect();
}
LLView::ctrl_list_t LLView::getCtrlList() const
{
	ctrl_list_t controls;
	for (LLView* viewp : mChildList)
	{
		if(viewp->isCtrl())
		{
			controls.push_back(static_cast<LLUICtrl*>(viewp));
		}
	}
	return controls;
}
LLView::ctrl_list_t LLView::getCtrlListSorted() const
{
	ctrl_list_t controls = getCtrlList();
	std::sort(controls.begin(), controls.end(), LLCompareByTabOrder(mCtrlOrder));
	return controls;
}
bool LLCompareByTabOrder::operator() (const LLView* const a, const LLView* const b) const
{
	S32 a_score = 0, b_score = 0;
	if(a) a_score--;
	if(b) b_score--;
	if(a && a->isCtrl()) a_score--;
	if(b && b->isCtrl()) b_score--;
	if(a_score == -2 && b_score == -2)
	{
		const LLUICtrl * const a_ctrl = static_cast<const LLUICtrl*>(a);
		const LLUICtrl * const b_ctrl = static_cast<const LLUICtrl*>(b);
		LLView::child_tab_order_const_iter_t a_found = mTabOrder.find(a_ctrl), b_found = mTabOrder.find(b_ctrl);
		if(a_found != mTabOrder.end()) a_score--;
		if(b_found != mTabOrder.end()) b_score--;
		if(a_score == -3 && b_score == -3)
		{
			return compareTabOrders(a_found->second, b_found->second);
		}
	}
	return (a_score == b_score) ? a < b : a_score < b_score;
}
BOOL LLView::isInVisibleChain() const
{
	const LLView* cur_view = this;
	while(cur_view)
	{
		if (!cur_view->getVisible())
		{
			return FALSE;
		}
		cur_view = cur_view->getParent();
	}
	return TRUE;
}
BOOL LLView::isInEnabledChain() const
{
	const LLView* cur_view = this;
	while(cur_view)
	{
		if (!cur_view->getEnabled())
		{
			return FALSE;
		}
		cur_view = cur_view->getParent();
	}
	return TRUE;
}
BOOL LLView::canFocusChildren() const
{
	return TRUE;
}
void LLView::setTentative(BOOL b)
{
}
BOOL LLView::getTentative() const
{
	return FALSE;
}
void LLView::setEnabled(BOOL enabled)
{
	mEnabled = enabled;
}
BOOL LLView::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	return FALSE;
}
LLRect LLView::getSnapRect() const
{
	return mRect;
}
LLRect LLView::getRequiredRect()
{
	return mRect;
}
BOOL LLView::focusNextRoot()
{
	viewList_t result = LLView::getFocusRootsQuery().run(this);
	return LLView::focusNext(result);
}
BOOL LLView::focusPrevRoot()
{
	viewList_t result = LLView::getFocusRootsQuery().run(this);
	return LLView::focusPrev(result);
}
BOOL LLView::focusNext(viewList_t& result)
{
	viewList_t::iterator focused = result.end();
	for(viewList_t::iterator iter = result.begin();
		iter != result.end();
		++iter)
	{
		if(gFocusMgr.childHasKeyboardFocus(*iter))
		{
			focused = iter;
			break;
		}
	}
	viewList_t::iterator next = focused;
	next = (next == result.end()) ? result.begin() : ++next;
	while(next != focused)
	{
		if(next == result.end())
		{
			next = result.begin();
		}
		if((*next)->isCtrl())
		{
			LLUICtrl * ctrl = static_cast<LLUICtrl*>(*next);
			ctrl->setFocus(TRUE);
			ctrl->onTabInto();
			gFocusMgr.triggerFocusFlash();
			return TRUE;
		}
		++next;
	}
	return FALSE;
}
BOOL LLView::focusPrev(viewList_t& result)
{
	viewList_t::reverse_iterator focused = result.rend();
	for(viewList_t::reverse_iterator iter = result.rbegin();
		iter != result.rend();
		++iter)
	{
		if(gFocusMgr.childHasKeyboardFocus(*iter))
		{
			focused = iter;
			break;
		}
	}
	viewList_t::reverse_iterator next = focused;
	next = (next == result.rend()) ? result.rbegin() : ++next;
	while(next != focused)
	{
		if(next == result.rend())
		{
			next = result.rbegin();
		}
		if((*next)->isCtrl())
		{
			LLUICtrl * ctrl = static_cast<LLUICtrl*>(*next);
			if (!ctrl->hasFocus())
			{
				ctrl->setFocus(TRUE);
				ctrl->onTabInto();
				gFocusMgr.triggerFocusFlash();
			}
			return TRUE;
		}
		++next;
	}
	return FALSE;
}
void LLView::deleteAllChildren()
{
	mCtrlOrder.clear();
	while (!mChildList.empty())
	{
		LLView* viewp = mChildList.front();
		delete viewp;
	}
	mChildHashMap.clear();
}
void LLView::setAllChildrenEnabled(BOOL b)
{
	for (LLView* viewp : mChildList)
	{
		viewp->setEnabled(b);
	}
}
void LLView::setVisible(BOOL visible)
{
	if ( mVisible != visible )
	{
		mVisible = visible;
		if (!getParent() || getParent()->isInVisibleChain())
		{
			handleVisibilityChange( visible );
		}
		updateBoundingRect();
	}
}
void LLView::handleVisibilityChange ( BOOL new_visibility )
{
	for (LLView* viewp : mChildList)
	{
		if (viewp->getVisible())
		{
			viewp->handleVisibilityChange ( new_visibility );
		}
	}
}
void LLView::translate(S32 x, S32 y)
{
	mRect.translate(x, y);
	updateBoundingRect();
}
BOOL LLView::canSnapTo(const LLView* other_view)
{
	return other_view != this && other_view->getVisible();
}
void LLView::setSnappedTo(const LLView* snap_view)
{
}
BOOL LLView::handleHover(S32 x, S32 y, MASK mask)
{
	return childrenHandleHover( x, y, mask ) != NULL;
}
void LLView::onMouseEnter(S32 x, S32 y, MASK mask)
{
}
void LLView::onMouseLeave(S32 x, S32 y, MASK mask)
{
}
std::string LLView::getShowNamesToolTip()
{
	LLView* view = getParent();
	std::string name;
	std::string tool_tip = mName;
	while (view)
	{
		name = view->getName();
		if (name == "root") break;
		if (view->getToolTip().find(".xml") != std::string::npos)
		{
			tool_tip = view->getToolTip() + "/" +  tool_tip;
			break;
		}
		else
		{
			tool_tip = view->getName() + "/" +  tool_tip;
		}
		view = view->getParent();
	}
	return "/" + tool_tip;
}
BOOL LLView::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
{
	BOOL handled = FALSE;
	std::string tool_tip;
	for(LLView* viewp : mChildList)
	{
		S32 local_x = x - viewp->mRect.mLeft;
		S32 local_y = y - viewp->mRect.mBottom;
		if( viewp->pointInView(local_x, local_y)
			&& viewp->getVisible()
			&& viewp->handleToolTip(local_x, local_y, msg, sticky_rect_screen ))
		{
			if (!msg.empty()) return TRUE;
			handled = TRUE;
			break;
		}
	}
	tool_tip = mToolTipMsg.getString();
	if (
		LLUI::sShowXUINames
		&& (tool_tip.find(".xml", 0) == std::string::npos)
		&& (mName.find("Drag", 0) == std::string::npos))
	{
		tool_tip = getShowNamesToolTip();
	}
	BOOL show_names_text_box = LLUI::sShowXUINames && dynamic_cast<LLTextBox*>(this) != NULL;
	if (blockMouseEvent(x, y) || show_names_text_box)
	{
		if(!tool_tip.empty())
		{
			msg = tool_tip;
			localPointToScreen(
				0, 0,
				&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
			localPointToScreen(
				mRect.getWidth(), mRect.getHeight(),
				&(sticky_rect_screen->mRight), &(sticky_rect_screen->mTop) );
		}
		handled = TRUE;
	}
	return handled;
}
bool LLView::visibleAndContains(S32 local_x, S32 local_y)
{
	return sDrilldown(this, local_x, local_y)
		&& getVisible();
}
bool LLView::visibleEnabledAndContains(S32 local_x, S32 local_y)
{
	return visibleAndContains(local_x, local_y)
		&& getEnabled();
}
void LLView::logMouseEvent()
{
	if (sDebugMouseHandling)
	{
		sMouseHandlerMessage = std::string("/") + mName + sMouseHandlerMessage;
	}
}
template <typename METHOD, typename CHARTYPE>
LLView* LLView::childrenHandleCharEvent(const std::string& desc, const METHOD& method,
										CHARTYPE c, MASK mask)
{
	if ( getVisible() && getEnabled() )
	{
		for (LLView* viewp : mChildList)
		{
			if ((viewp->*method)(c, mask, TRUE))
			{
				if (LLView::sDebugKeys)
				{
					LL_INFOS() << desc << " handled by " << viewp->getName() << LL_ENDL;
				}
				return viewp;
			}
		}
	}
    return NULL;
}
template <typename METHOD, typename XDATA>
LLView* LLView::childrenHandleMouseEvent(const METHOD& method, S32 x, S32 y, XDATA extra, bool allow_mouse_block)
{
	for (LLView* viewp : mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if (!viewp->visibleEnabledAndContains(local_x, local_y))
		{
			continue;
		}
		if ((viewp->*method)( local_x, local_y, extra )
			|| (allow_mouse_block && viewp->blockMouseEvent( local_x, local_y )))
		{
			viewp->logMouseEvent();
			return viewp;
		}
	}
	return NULL;
}
LLView* LLView::childrenHandleDragAndDrop(S32 x, S32 y, MASK mask,
									   BOOL drop,
									   EDragAndDropType cargo_type,
									   void* cargo_data,
									   EAcceptance* accept,
									   std::string& tooltip_msg)
{
	*accept = ACCEPT_NO;
	for (LLView* viewp : mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if( !viewp->visibleEnabledAndContains(local_x, local_y))
		{
			continue;
		}
		if (viewp->handleDragAndDrop(local_x, local_y, mask, drop,
									 cargo_type,
									 cargo_data,
									 accept,
									 tooltip_msg)
			|| viewp->blockMouseEvent(local_x, local_y))
		{
			return viewp;
		}
	}
	return NULL;
}
LLView* LLView::childrenHandleHover(S32 x, S32 y, MASK mask)
{
	for (LLView* viewp : mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if(!viewp->visibleEnabledAndContains(local_x, local_y))
		{
			continue;
		}
		LLUI::sWindow->setCursor(viewp->getHoverCursor());
		if (viewp->handleHover(local_x, local_y, mask)
			|| viewp->blockMouseEvent(local_x, local_y))
		{
			viewp->logMouseEvent();
			return viewp;
		}
	}
	return NULL;
}
LLView*	LLView::childFromPoint(S32 x, S32 y, bool recur)
{
	if (!getVisible())
		return NULL;
	for (LLView* viewp : mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if (!viewp->visibleAndContains(local_x, local_y))
		{
			continue;
		}
		if (recur)
		{
			LLView* leaf(viewp->childFromPoint(local_x, local_y, recur));
			return leaf? leaf : viewp;
		}
		return viewp;
	}
	return NULL;
}
BOOL LLView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if (getVisible() && getEnabled())
	{
		if( called_from_parent )
		{
			handled = childrenHandleKey( key, mask ) != NULL;
		}
		if (!handled)
		{
			handled = handleKeyHere( key, mask );
			if (handled && LLView::sDebugKeys)
			{
				LL_INFOS() << "Key handled by " << getName() << LL_ENDL;
			}
		}
	}
	if( !handled && !called_from_parent && mParentView)
	{
		handled = mParentView->handleKey( key, mask, FALSE );
	}
	return handled;
}
BOOL LLView::handleKeyUp(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if (getVisible() && getEnabled())
	{
		if (called_from_parent)
		{
			handled = childrenHandleKeyUp(key, mask) != NULL;
		}
		if (!handled)
		{
			handled = handleKeyUpHere(key, mask);
			if (handled)
			{
				LL_DEBUGS() << "Key handled by " << getName() << LL_ENDL;
			}
		}
	}
	if (!handled && !called_from_parent && mParentView)
	{
		handled = mParentView->handleKeyUp(key, mask, FALSE);
	}
	return handled;
}
BOOL LLView::handleKeyHere(KEY key, MASK mask)
{
	return FALSE;
}
BOOL LLView::handleKeyUpHere(KEY key, MASK mask)
{
	return FALSE;
}
BOOL LLView::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if (getVisible() && getEnabled())
	{
		if( called_from_parent )
		{
			handled = childrenHandleUnicodeChar( uni_char ) != NULL;
		}
		if (!handled)
		{
			handled = handleUnicodeCharHere(uni_char);
			if (handled && LLView::sDebugKeys)
			{
				LL_INFOS() << "Unicode key handled by " << getName() << LL_ENDL;
			}
		}
	}
	if (!handled && !called_from_parent && mParentView)
	{
		handled = mParentView->handleUnicodeChar(uni_char, FALSE);
	}
	return handled;
}
BOOL LLView::handleUnicodeCharHere(llwchar uni_char )
{
	return FALSE;
}
BOOL LLView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
							   EDragAndDropType cargo_type, void* cargo_data,
							   EAcceptance* accept,
							   std::string& tooltip_msg)
{
	return childrenHandleDragAndDrop( x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg) != NULL;
}
void LLView::onMouseCaptureLost()
{
}
BOOL LLView::hasMouseCapture()
{
	return gFocusMgr.getMouseCapture() == this;
}
BOOL LLView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseUp( x, y, mask ) != NULL;
}
BOOL LLView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = childrenHandleMouseDown( x, y, mask );
	if (sEditingUI && handled_view)
	{
		LLButton* buttonp = dynamic_cast<LLButton*>(handled_view);
		LLLineEditor* line_editorp = dynamic_cast<LLLineEditor*>(handled_view);
		LLTextEditor* text_editorp = dynamic_cast<LLTextEditor*>(handled_view);
		LLTextBox* text_boxp = dynamic_cast<LLTextBox*>(handled_view);
		if (buttonp
			|| line_editorp
			|| text_editorp
			|| text_boxp)
		{
			sEditingUIView = handled_view;
		}
	}
	return handled_view != NULL;
}
BOOL LLView::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return childrenHandleDoubleClick( x, y, mask ) != NULL;
}
BOOL LLView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return childrenHandleScrollWheel( x, y, clicks ) != NULL;
}
BOOL LLView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleRightMouseDown( x, y, mask ) != NULL;
}
BOOL LLView::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleRightMouseUp( x, y, mask ) != NULL;
}
BOOL LLView::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMiddleMouseDown( x, y, mask ) != NULL;
}
BOOL LLView::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMiddleMouseUp( x, y, mask ) != NULL;
}
LLView* LLView::childrenHandleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return childrenHandleMouseEvent(&LLView::handleScrollWheel, x, y, clicks, false);
}
LLView* LLView::childrenHandleKey(KEY key, MASK mask)
{
	return childrenHandleCharEvent("Key", &LLView::handleKey, key, mask);
}
LLView* LLView::childrenHandleKeyUp(KEY key, MASK mask)
{
	return childrenHandleCharEvent("Key Up", &LLView::handleKeyUp, key, mask);
}
LLView* LLView::childrenHandleUnicodeChar(llwchar uni_char)
{
	return childrenHandleCharEvent("Unicode character", &LLView::handleUnicodeCharWithDummyMask,
								   uni_char, MASK_NONE);
}
LLView* LLView::childrenHandleMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleMouseDown, x, y, mask);
}
LLView* LLView::childrenHandleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleRightMouseDown, x, y, mask);
}
LLView* LLView::childrenHandleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleMiddleMouseDown, x, y, mask);
}
LLView* LLView::childrenHandleDoubleClick(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleDoubleClick, x, y, mask);
}
LLView* LLView::childrenHandleMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleMouseUp, x, y, mask);
}
LLView* LLView::childrenHandleRightMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleRightMouseUp, x, y, mask);
}
LLView* LLView::childrenHandleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = NULL;
	if( getVisible() && getEnabled() )
	{
		for (LLView* viewp : mChildList)
		{
			S32 local_x = x - viewp->getRect().mLeft;
			S32 local_y = y - viewp->getRect().mBottom;
			if (viewp->pointInView(local_x, local_y) &&
				viewp->getVisible() &&
				viewp->getEnabled() &&
				viewp->handleMiddleMouseUp( local_x, local_y, mask ))
			{
				if (sDebugMouseHandling)
				{
					sMouseHandlerMessage = std::string("->") + viewp->mName + sMouseHandlerMessage;
				}
				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}
void LLView::draw()
{
	drawChildren();
}
extern void check_blend_funcs();
void LLView::drawChildren()
{
	if (!mChildList.empty())
	{
		LLRect clip_rect = LLUI::getRootView()->getRect();
		if (LLGLState<GL_SCISSOR_TEST>::isEnabled())
		{
			LLRect scissor = gGL.getScissor();
			scissor.mLeft /= LLUI::getScaleFactor().mV[VX];
			scissor.mTop /= LLUI::getScaleFactor().mV[VY];
			scissor.mRight /= LLUI::getScaleFactor().mV[VX];
			scissor.mBottom /= LLUI::getScaleFactor().mV[VY];
			clip_rect.intersectWith(scissor);
		}
		++sDepth;
		for (child_list_const_reverse_iter_t child_iter = mChildList.rbegin(); child_iter != mChildList.rend(); ++child_iter)
		{
			LLView *viewp = *child_iter;
			if (viewp == NULL)
			{
				continue;
			}
			if (viewp->getVisible() && viewp->getRect().isValid())
			{
				LLRect screen_rect = viewp->getRect();
				screen_rect.translate(LLFontGL::sCurOrigin.mX, LLFontGL::sCurOrigin.mY);
				if ( clip_rect.overlaps(screen_rect) )
				{
					LLUI::pushMatrix();
					{
						LLUI::translate((F32)viewp->getRect().mLeft, (F32)viewp->getRect().mBottom, 0.f);
						viewp->mInDraw = true;
						if(gDebugGL)check_blend_funcs();
						viewp->draw();
						if(gDebugGL)check_blend_funcs();
						viewp->mInDraw = false;
						if (sDebugRects)
						{
							viewp->drawDebugRect();
							if (!getRect().isValid())
							{
								LL_WARNS() << "Bogus rectangle for " << getName() << " with " << mRect << LL_ENDL;
							}
						}
					}
					LLUI::popMatrix();
				}
			}
		}
		--sDepth;
	}
	if (sEditingUI && this == sEditingUIView)
	{
		drawDebugRect();
	}
}
void LLView::drawDebugRect()
{
	LLUI::pushMatrix();
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		if (getUseBoundingRect())
		{
			LLUI::translate((F32)mBoundingRect.mLeft - (F32)mRect.mLeft, (F32)mBoundingRect.mBottom - (F32)mRect.mBottom, 0.f);
		}
		LLRect debug_rect = getUseBoundingRect() ? mBoundingRect : mRect;
		LLColor4 border_color(0.f, 0.f, 0.f, 1.f);
		if (sEditingUI)
		{
			border_color.mV[0] = 1.f;
		}
		else
		{
			border_color.mV[sDepth%3] = 1.f;
		}
		gGL.color4fv( border_color.mV );
		gGL.begin(LLRender::LINES);
			gGL.vertex2i(0, debug_rect.getHeight() - 1);
			gGL.vertex2i(0, 0);
			gGL.vertex2i(0, 0);
			gGL.vertex2i(debug_rect.getWidth() - 1, 0);
			gGL.vertex2i(debug_rect.getWidth() - 1, 0);
			gGL.vertex2i(debug_rect.getWidth() - 1, debug_rect.getHeight() - 1);
			gGL.vertex2i(debug_rect.getWidth() - 1, debug_rect.getHeight() - 1);
			gGL.vertex2i(0, debug_rect.getHeight() - 1);
		gGL.end();
		if (mChildList.size() && !sEditingUI)
		{
			S32 x, y;
			gGL.color4fv( border_color.mV );
			x = debug_rect.getWidth()/2;
			y = debug_rect.getHeight()/2;
			std::string debug_text = llformat("%s (%d x %d)", getName().c_str(),
										debug_rect.getWidth(), debug_rect.getHeight());
			LLFontGL::getFontSansSerifSmall()->renderUTF8(debug_text, 0, (F32)x, (F32)y, border_color,
												LLFontGL::HCENTER, LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
												S32_MAX, S32_MAX, NULL, FALSE);
		}
	}
	LLUI::popMatrix();
}
void LLView::drawChild(LLView* childp, S32 x_offset, S32 y_offset, BOOL force_draw)
{
	if (childp && childp->getParent() == this)
	{
		++sDepth;
		if ((childp->getVisible() && childp->getRect().isValid())
			|| force_draw)
		{
			gGL.matrixMode(LLRender::MM_MODELVIEW);
			LLUI::pushMatrix();
			{
				LLUI::translate((F32)childp->getRect().mLeft + x_offset, (F32)childp->getRect().mBottom + y_offset, 0.f);
				if(gDebugGL)check_blend_funcs();
				childp->draw();
				if(gDebugGL)check_blend_funcs();
			}
			LLUI::popMatrix();
		}
		--sDepth;
	}
}
void LLView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	S32 delta_width = width - getRect().getWidth();
	S32 delta_height = height - getRect().getHeight();
	if (delta_width || delta_height || sForceReshape)
	{
		mRect.mRight = getRect().mLeft + width;
		mRect.mTop = getRect().mBottom + height;
		for (LLView* viewp : mChildList)
		{
			if (viewp != NULL)
			{
			LLRect child_rect( viewp->mRect );
			if (viewp->followsRight() && viewp->followsLeft())
			{
				child_rect.mRight += delta_width;
			}
			else if (viewp->followsRight())
			{
				child_rect.mLeft += delta_width;
				child_rect.mRight += delta_width;
			}
			else if (viewp->followsLeft())
			{
			}
			else
			{
			}
			if (viewp->followsTop() && viewp->followsBottom())
			{
				child_rect.mTop += delta_height;
			}
			else if (viewp->followsTop())
			{
				child_rect.mTop += delta_height;
				child_rect.mBottom += delta_height;
			}
			else if (viewp->followsBottom())
			{
			}
			else
			{
			}
			S32 delta_x = child_rect.mLeft - viewp->getRect().mLeft;
			S32 delta_y = child_rect.mBottom - viewp->getRect().mBottom;
			viewp->translate( delta_x, delta_y );
			if (child_rect.getWidth() != viewp->getRect().getWidth() || child_rect.getHeight() != viewp->getRect().getHeight())
			{
				viewp->reshape(child_rect.getWidth(), child_rect.getHeight());
			}
		}
	}
	}
	if (!called_from_parent)
	{
		if (mParentView)
		{
			mParentView->reshape(mParentView->getRect().getWidth(), mParentView->getRect().getHeight(), FALSE);
		}
	}
	updateBoundingRect();
}
LLRect LLView::calcBoundingRect()
{
	LLRect local_bounding_rect = LLRect::null;
	for (LLView* childp : mChildList)
	{
		if (!childp->getVisible() || childp == gFocusMgr.getTopCtrl())
		{
			continue;
		}
		LLRect child_bounding_rect = childp->getBoundingRect();
		if (local_bounding_rect.isEmpty())
		{
			local_bounding_rect = child_bounding_rect;
		}
		else
		{
			if (!child_bounding_rect.isEmpty())
			{
				local_bounding_rect.unionWith(child_bounding_rect);
			}
		}
	}
	local_bounding_rect.translate(mRect.mLeft, mRect.mBottom);
	return local_bounding_rect;
}
void LLView::updateBoundingRect()
{
	if (isDead()) return;
	if (getUseBoundingRect())
	{
		mBoundingRect = calcBoundingRect();
	}
	else
	{
		mBoundingRect = mRect;
	}
	if (getParent() && getParent()->getUseBoundingRect())
	{
		getParent()->updateBoundingRect();
	}
}
LLRect LLView::calcScreenRect() const
{
	LLRect screen_rect;
	localPointToScreen(0, 0, &screen_rect.mLeft, &screen_rect.mBottom);
	localPointToScreen(getRect().getWidth(), getRect().getHeight(), &screen_rect.mRight, &screen_rect.mTop);
	return screen_rect;
}
LLRect LLView::calcScreenBoundingRect() const
{
	LLRect screen_rect;
	LLRect bounding_rect = getUseBoundingRect() ? mBoundingRect : mRect;
	bounding_rect.translate(-mRect.mLeft, -mRect.mBottom);
	localPointToScreen(bounding_rect.mLeft, bounding_rect.mBottom, &screen_rect.mLeft, &screen_rect.mBottom);
	localPointToScreen(bounding_rect.mRight, bounding_rect.mTop, &screen_rect.mRight, &screen_rect.mTop);
	return screen_rect;
}
LLRect LLView::getLocalBoundingRect() const
{
	LLRect local_bounding_rect = getBoundingRect();
	local_bounding_rect.translate(-mRect.mLeft, -mRect.mBottom);
	return local_bounding_rect;
}
LLRect LLView::getLocalRect() const
{
	LLRect local_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	return local_rect;
}
LLRect LLView::getLocalSnapRect() const
{
	LLRect local_snap_rect = getSnapRect();
	local_snap_rect.translate(-getRect().mLeft, -getRect().mBottom);
	return local_snap_rect;
}
BOOL LLView::hasAncestor(const LLView* parentp) const
{
	if (!parentp)
	{
		return FALSE;
	}
	LLView* viewp = getParent();
	while(viewp)
	{
		if (viewp == parentp)
		{
			return TRUE;
		}
		viewp = viewp->getParent();
	}
	return FALSE;
}
BOOL LLView::childHasKeyboardFocus( const std::string& childname ) const
{
	LLView *focus = dynamic_cast<LLView *>(gFocusMgr.getKeyboardFocus());
	while (focus != NULL)
	{
		if (focus->getName() == childname)
		{
			return TRUE;
		}
		focus = focus->getParent();
	}
	return FALSE;
}
BOOL LLView::hasChild(const std::string& childname, BOOL recurse) const
{
	return getChildView(childname, recurse, FALSE) != NULL;
}
static LLTrace::BlockTimerStatHandle FTM_FIND_VIEWS("Find Widgets");
LLView* LLView::getChildView(const std::string& name, BOOL recurse, BOOL create_if_missing) const
{
	LL_RECORD_BLOCK_TIME(FTM_FIND_VIEWS);
	boost::container::flat_map<std::string, LLView*>::const_iterator it = mChildHashMap.find(name);
	if(it != mChildHashMap.end())
	{
		return it->second;
	}
	if (recurse)
	{
		for (LLView* childp : mChildList)
		{
			llassert(childp);
			LLView* viewp = childp->getChildView(name, recurse, FALSE);
			if ( viewp )
			{
				return viewp;
			}
		}
	}
	if (create_if_missing)
	{
		return createDummyWidget<LLView>(name);
	}
	return NULL;
}
BOOL LLView::parentPointInView(S32 x, S32 y, EHitTestType type) const
{
	return (getUseBoundingRect() && type == HIT_TEST_USE_BOUNDING_RECT)
		? mBoundingRect.pointInRect( x, y )
		: mRect.pointInRect( x, y );
}
BOOL LLView::pointInView(S32 x, S32 y, EHitTestType type) const
{
	return (getUseBoundingRect() && type == HIT_TEST_USE_BOUNDING_RECT)
		? mBoundingRect.pointInRect( x + mRect.mLeft, y + mRect.mBottom )
		: mRect.localPointInRect( x, y );
}
BOOL LLView::blockMouseEvent(S32 x, S32 y) const
{
	return mMouseOpaque && pointInView(x, y, HIT_TEST_IGNORE_BOUNDING_RECT);
}
void LLView::screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
{
	*local_x = screen_x - getRect().mLeft;
	*local_y = screen_y - getRect().mBottom;
	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		*local_x -= cur->getRect().mLeft;
		*local_y -= cur->getRect().mBottom;
	}
}
void LLView::localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
{
	*screen_x = local_x + getRect().mLeft;
	*screen_y = local_y + getRect().mBottom;
	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		*screen_x += cur->getRect().mLeft;
		*screen_y += cur->getRect().mBottom;
	}
}
void LLView::screenRectToLocal(const LLRect& screen, LLRect* local) const
{
	*local = screen;
	local->translate( -getRect().mLeft, -getRect().mBottom );
	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		local->translate( -cur->getRect().mLeft, -cur->getRect().mBottom );
	}
}
void LLView::localRectToScreen(const LLRect& local, LLRect* screen) const
{
	*screen = local;
	screen->translate( getRect().mLeft, getRect().mBottom );
	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		screen->translate( cur->getRect().mLeft, cur->getRect().mBottom );
	}
}
LLView* LLView::getRootView()
{
	LLView* view = this;
	while( view->mParentView )
	{
		view = view->mParentView;
	}
	return view;
}
BOOL LLView::deleteViewByHandle(LLHandle<LLView> handle)
{
	LLView* viewp = handle.get();
	if(viewp)
		delete viewp;
	return viewp != NULL;
}
LLView* LLView::findPrevSibling(LLView* child)
{
	child_list_t::iterator prev_it = std::find(mChildList.begin(), mChildList.end(), child);
	if (prev_it != mChildList.end() && prev_it != mChildList.begin())
	{
		return *(--prev_it);
	}
	return NULL;
}
LLView* LLView::findNextSibling(LLView* child)
{
	child_list_t::iterator next_it = std::find(mChildList.begin(), mChildList.end(), child);
	if (next_it != mChildList.end())
	{
		next_it++;
	}
	return (next_it != mChildList.end()) ? *next_it : NULL;
}
LLCoordGL getNeededTranslation(const LLRect& input, const LLRect& constraint, BOOL allow_partial_outside)
{
	LLCoordGL delta;
	if (allow_partial_outside)
	{
		const S32 KEEP_ONSCREEN_PIXELS = 16;
		if( input.mRight - KEEP_ONSCREEN_PIXELS < constraint.mLeft )
		{
			delta.mX = constraint.mLeft - (input.mRight - KEEP_ONSCREEN_PIXELS);
		}
		else
		if( input.mLeft + KEEP_ONSCREEN_PIXELS > constraint.mRight )
		{
			delta.mX = constraint.mRight - (input.mLeft + KEEP_ONSCREEN_PIXELS);
		}
		if( input.mTop > constraint.mTop )
		{
			delta.mY = constraint.mTop - input.mTop;
		}
		else
		if( input.mTop - KEEP_ONSCREEN_PIXELS < constraint.mBottom )
		{
			delta.mY = constraint.mBottom - (input.mTop - KEEP_ONSCREEN_PIXELS);
		}
	}
	else
	{
		if( input.mLeft < constraint.mLeft )
		{
			delta.mX = constraint.mLeft - input.mLeft;
		}
		else
		if( input.mRight > constraint.mRight )
		{
			delta.mX = constraint.mRight - input.mRight;
			delta.mX += llmax( 0, input.getWidth() - constraint.getWidth() );
		}
		if( input.mTop > constraint.mTop )
		{
			delta.mY = constraint.mTop - input.mTop;
		}
		else
		if( input.mBottom < constraint.mBottom )
		{
			delta.mY = constraint.mBottom - input.mBottom;
			delta.mY -= llmax( 0, input.getHeight() - constraint.getHeight() );
		}
	}
	return delta;
}
BOOL LLView::translateIntoRect(const LLRect& constraint, BOOL allow_partial_outside )
{
	LLCoordGL translation = getNeededTranslation(getRect(), constraint, allow_partial_outside);
	if (translation.mX != 0 || translation.mY != 0)
	{
		translate(translation.mX, translation.mY);
		return TRUE;
	}
	return FALSE;
}
BOOL LLView::translateIntoRectWithExclusion( const LLRect& inside, const LLRect& exclude, BOOL allow_partial_outside )
{
	LLCoordGL translation = getNeededTranslation(getRect(), inside, allow_partial_outside);
	if (translation.mX != 0 || translation.mY != 0)
	{
		translate(translation.mX, translation.mY);
		if (exclude.overlaps(getRect()))
		{
			if (translation.mX > 0)
			{
				translate(exclude.mRight - getRect().mLeft, 0);
			}
			else if (translation.mX < 0)
			{
				translate(exclude.mLeft - getRect().mRight, 0);
			}
			if (translation.mY > 0)
			{
				translate(0, exclude.mTop - getRect().mBottom);
			}
			else if (translation.mY < 0)
			{
				translate(0, exclude.mBottom - getRect().mTop);
			}
		}
		return TRUE;
	}
	return FALSE;
}
void LLView::centerWithin(const LLRect& bounds)
{
	S32 left   = bounds.mLeft + (bounds.getWidth() - getRect().getWidth()) / 2;
	S32 bottom = bounds.mBottom + (bounds.getHeight() - getRect().getHeight()) / 2;
	translate( left - getRect().mLeft, bottom - getRect().mBottom );
}
BOOL LLView::localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, LLView* other_view) const
{
	const LLView* cur_view = this;
	const LLView* root_view = NULL;
	while (cur_view)
	{
		if (cur_view == other_view)
		{
			*other_x = x;
			*other_y = y;
			return TRUE;
		}
		x += cur_view->getRect().mLeft;
		y += cur_view->getRect().mBottom;
		cur_view = cur_view->getParent();
		root_view = cur_view;
	}
	cur_view = other_view;
	while (cur_view)
	{
		x -= cur_view->getRect().mLeft;
		y -= cur_view->getRect().mBottom;
		cur_view = cur_view->getParent();
		if (cur_view == root_view)
		{
			*other_x = x;
			*other_y = y;
			return TRUE;
		}
	}
	*other_x = x;
	*other_y = y;
	return FALSE;
}
BOOL LLView::localRectToOtherView( const LLRect& local, LLRect* other, LLView* other_view ) const
{
	LLRect cur_rect = local;
	const LLView* cur_view = this;
	const LLView* root_view = NULL;
	while (cur_view)
	{
		if (cur_view == other_view)
		{
			*other = cur_rect;
			return TRUE;
		}
		cur_rect.translate(cur_view->getRect().mLeft, cur_view->getRect().mBottom);
		cur_view = cur_view->getParent();
		root_view = cur_view;
	}
	cur_view = other_view;
	while (cur_view)
	{
		cur_rect.translate(-cur_view->getRect().mLeft, -cur_view->getRect().mBottom);
		cur_view = cur_view->getParent();
		if (cur_view == root_view)
		{
			*other = cur_rect;
			return TRUE;
		}
	}
	*other = cur_rect;
	return FALSE;
}
LLXMLNodePtr LLView::getXML(bool save_children) const
{
	LLXMLNodePtr node = new LLXMLNode("view", FALSE);
	node->createChild("name", TRUE)->setStringValue(getName());
	node->createChild("width", TRUE)->setIntValue(getRect().getWidth());
	node->createChild("height", TRUE)->setIntValue(getRect().getHeight());
	LLView* parent = getParent();
	S32 left = getRect().mLeft;
	S32 bottom = getRect().mBottom;
	if (parent) bottom -= parent->getRect().getHeight();
	node->createChild("left", TRUE)->setIntValue(left);
	node->createChild("bottom", TRUE)->setIntValue(bottom);
	U32 follows_flags = getFollows();
	if (follows_flags)
	{
		std::stringstream buffer;
		bool pipe = false;
		if (followsLeft())
		{
			buffer << "left";
			pipe = true;
		}
		if (followsTop())
		{
			if (pipe) buffer << "|";
			buffer << "top";
			pipe = true;
		}
		if (followsRight())
		{
			if (pipe) buffer << "|";
			buffer << "right";
			pipe = true;
		}
		if (followsBottom())
		{
			if (pipe) buffer << "|";
			buffer << "bottom";
		}
		node->createChild("follows", TRUE)->setStringValue(buffer.str());
	}
	node->createChild("mouse_opaque", TRUE)->setBoolValue(mMouseOpaque );
	if (!mToolTipMsg.getString().empty())
	{
		node->createChild("tool_tip", TRUE)->setStringValue(mToolTipMsg.getString());
	}
	if (mSoundFlags != MOUSE_UP)
	{
		node->createChild("sound_flags", TRUE)->setIntValue((S32)mSoundFlags);
	}
	node->createChild("enabled", TRUE)->setBoolValue(getEnabled());
	if (!mControlName.empty())
	{
		node->createChild("control_name", TRUE)->setStringValue(mControlName);
	}
	return node;
}
LLView* LLView::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLView* viewp = new LLView();
	viewp->initFromXML(node, parent);
	return viewp;
}
void LLView::addColorXML(LLXMLNodePtr node, const LLColor4& color,
							const char* xml_name, const char* control_name)
{
	if (color != LLUI::sColorsGroup->getColor(ll_safe_string(control_name)))
	{
		node->createChild(xml_name, TRUE)->setFloatValue(4, color.mV);
	}
}
std::string LLView::escapeXML(const std::string& xml, std::string& indent)
{
	std::string ret = indent + "\"" + LLXMLNode::escapeXML(xml);
	size_t index = ret.size()-1;
	size_t fnd;
	while ((fnd = ret.rfind("\n", index)) != std::string::npos)
	{
		ret.replace(fnd, 1, "\"\n" + indent + "\"");
		index = fnd-1;
	}
	ret.append("\"");
	return ret;
}
LLWString LLView::escapeXML(const LLWString& xml)
{
	LLWString out;
	for (LLWString::size_type i = 0; i < xml.size(); ++i)
	{
		llwchar c = xml[i];
		switch(c)
		{
		case '"':	out.append(utf8string_to_wstring("&quot;"));	break;
		case '\'':	out.append(utf8string_to_wstring("&apos;"));	break;
		case '&':	out.append(utf8string_to_wstring("&amp;"));		break;
		case '<':	out.append(utf8string_to_wstring("&lt;"));		break;
		case '>':	out.append(utf8string_to_wstring("&gt;"));		break;
		default:	out.push_back(c); break;
		}
	}
	return out;
}
const LLCtrlQuery & LLView::getTabOrderQuery()
{
	static LLCtrlQuery query;
	if(query.getPreFilters().size() == 0) {
		query.addPreFilter(LLVisibleFilter::getInstance());
		query.addPreFilter(LLEnabledFilter::getInstance());
		query.addPreFilter(LLTabStopFilter::getInstance());
		query.addPostFilter(LLLeavesFilter::getInstance());
	}
	return query;
}
class LLFocusRootsFilter : public LLQueryFilter, public LLSingleton<LLFocusRootsFilter>
{
	filterResult_t operator() (const LLView* const view, const viewList_t & children) const
	{
		return filterResult_t(view->isCtrl() && view->isFocusRoot(), !view->isFocusRoot());
	}
};
const LLCtrlQuery & LLView::getFocusRootsQuery()
{
	static LLCtrlQuery query;
	if(query.getPreFilters().size() == 0) {
		query.addPreFilter(LLVisibleFilter::getInstance());
		query.addPreFilter(LLEnabledFilter::getInstance());
		query.addPreFilter(LLFocusRootsFilter::getInstance());
		query.addPostFilter(LLRootsFilter::getInstance());
	}
	return query;
}
void	LLView::setShape(const LLRect& new_rect, bool by_user)
{
	handleReshape(new_rect, by_user);
}
void	LLView::handleReshape(const LLRect& new_rect, bool by_user)
{
	reshape(new_rect.getWidth(), new_rect.getHeight());
	translate(new_rect.mLeft - getRect().mLeft, new_rect.mBottom - getRect().mBottom);
}
LLView* LLView::findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir,
							 LLView::ESnapType snap_type, S32 threshold, S32 padding)
{
	new_rect = mRect;
	LLView* snap_view = NULL;
	if (!mParentView)
	{
		return NULL;
	}
	S32 delta_x = 0;
	S32 delta_y = 0;
	if (mouse_dir.mX >= 0)
	{
		S32 new_right = mRect.mRight;
		LLView* view = findSnapEdge(new_right, mouse_dir, SNAP_RIGHT, snap_type, threshold, padding);
		delta_x = new_right - mRect.mRight;
		snap_view = view ? view : snap_view;
	}
	if (mouse_dir.mX <= 0)
	{
		S32 new_left = mRect.mLeft;
		LLView* view = findSnapEdge(new_left, mouse_dir, SNAP_LEFT, snap_type, threshold, padding);
		delta_x = new_left - mRect.mLeft;
		snap_view = view ? view : snap_view;
	}
	if (mouse_dir.mY >= 0)
	{
		S32 new_top = mRect.mTop;
		LLView* view = findSnapEdge(new_top, mouse_dir, SNAP_TOP, snap_type, threshold, padding);
		delta_y = new_top - mRect.mTop;
		snap_view = view ? view : snap_view;
	}
	if (mouse_dir.mY <= 0)
	{
		S32 new_bottom = mRect.mBottom;
		LLView* view = findSnapEdge(new_bottom, mouse_dir, SNAP_BOTTOM, snap_type, threshold, padding);
		delta_y = new_bottom - mRect.mBottom;
		snap_view = view ? view : snap_view;
	}
	new_rect.translate(delta_x, delta_y);
	return snap_view;
}
LLView*	LLView::findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding)
{
	LLRect snap_rect = getSnapRect();
	S32 snap_pos = 0;
	switch(snap_edge)
	{
	case SNAP_LEFT:
		snap_pos = snap_rect.mLeft;
		break;
	case SNAP_RIGHT:
		snap_pos = snap_rect.mRight;
		break;
	case SNAP_TOP:
		snap_pos = snap_rect.mTop;
		break;
	case SNAP_BOTTOM:
		snap_pos = snap_rect.mBottom;
		break;
	}
	if (!mParentView)
	{
		new_edge_val = snap_pos;
		return NULL;
	}
	LLView* snap_view = NULL;
	LLRect test_rect = snap_rect;
	test_rect.stretch(padding);
	S32 x_threshold = threshold;
	S32 y_threshold = threshold;
	LLRect parent_local_snap_rect = mParentView->getLocalSnapRect();
	if (snap_type == SNAP_PARENT || snap_type == SNAP_PARENT_AND_SIBLINGS)
	{
		switch(snap_edge)
		{
		case SNAP_RIGHT:
			if (llabs(parent_local_snap_rect.mRight - test_rect.mRight) <= x_threshold
				&& (parent_local_snap_rect.mRight - test_rect.mRight) * mouse_dir.mX >= 0)
			{
				snap_pos = parent_local_snap_rect.mRight - padding;
				snap_view = mParentView;
				x_threshold = llabs(parent_local_snap_rect.mRight - test_rect.mRight);
			}
			break;
		case SNAP_LEFT:
			if (llabs(test_rect.mLeft - parent_local_snap_rect.mLeft) <= x_threshold
				&& test_rect.mLeft * mouse_dir.mX <= 0)
			{
				snap_pos = parent_local_snap_rect.mLeft + padding;
				snap_view = mParentView;
				x_threshold = llabs(test_rect.mLeft - parent_local_snap_rect.mLeft);
			}
			break;
		case SNAP_BOTTOM:
			if (llabs(test_rect.mBottom - parent_local_snap_rect.mBottom) <= y_threshold
				&& test_rect.mBottom * mouse_dir.mY <= 0)
			{
				snap_pos = parent_local_snap_rect.mBottom + padding;
				snap_view = mParentView;
				y_threshold = llabs(test_rect.mBottom - parent_local_snap_rect.mBottom);
			}
			break;
		case SNAP_TOP:
			if (llabs(parent_local_snap_rect.mTop - test_rect.mTop) <= y_threshold && (parent_local_snap_rect.mTop - test_rect.mTop) * mouse_dir.mY >= 0)
			{
				snap_pos = parent_local_snap_rect.mTop - padding;
				snap_view = mParentView;
				y_threshold = llabs(parent_local_snap_rect.mTop - test_rect.mTop);
			}
			break;
		default:
			LL_ERRS() << "Invalid snap edge" << LL_ENDL;
		}
	}
	if (snap_type == SNAP_SIBLINGS || snap_type == SNAP_PARENT_AND_SIBLINGS)
	{
		for ( child_list_const_iter_t child_it = mParentView->getChildList()->begin();
			  child_it != mParentView->getChildList()->end(); ++child_it)
		{
			LLView* siblingp = *child_it;
			if (!canSnapTo(siblingp)) continue;
			LLRect sibling_rect = siblingp->getSnapRect();
			switch(snap_edge)
			{
			case SNAP_RIGHT:
				if (llabs(test_rect.mRight - sibling_rect.mLeft) <= x_threshold
					&& (test_rect.mRight - sibling_rect.mLeft) * mouse_dir.mX <= 0)
				{
					snap_pos = sibling_rect.mLeft - padding;
					snap_view = siblingp;
					x_threshold = llabs(test_rect.mRight - sibling_rect.mLeft);
				}
				else if (llabs(sibling_rect.mTop - (test_rect.mBottom - padding)) <= y_threshold
					|| llabs(sibling_rect.mBottom - (test_rect.mTop + padding)) <= x_threshold)
				{
					if (llabs(test_rect.mRight - sibling_rect.mRight) <= x_threshold
						&& (test_rect.mRight - sibling_rect.mRight) * mouse_dir.mX <= 0)
					{
						snap_pos = sibling_rect.mRight;
						snap_view = siblingp;
						x_threshold = llabs(test_rect.mRight - sibling_rect.mRight);
					}
				}
				break;
			case SNAP_LEFT:
				if (llabs(test_rect.mLeft - sibling_rect.mRight) <= x_threshold
					&& (test_rect.mLeft - sibling_rect.mRight) * mouse_dir.mX <= 0)
				{
					snap_pos = sibling_rect.mRight + padding;
					snap_view = siblingp;
					x_threshold = llabs(test_rect.mLeft - sibling_rect.mRight);
				}
				else if (llabs(sibling_rect.mTop - (test_rect.mBottom - padding)) <= y_threshold
					|| llabs(sibling_rect.mBottom - (test_rect.mTop + padding)) <= y_threshold)
				{
					if (llabs(test_rect.mLeft - sibling_rect.mLeft) <= x_threshold
						&& (test_rect.mLeft - sibling_rect.mLeft) * mouse_dir.mX <= 0)
					{
						snap_pos = sibling_rect.mLeft;
						snap_view = siblingp;
						x_threshold = llabs(test_rect.mLeft - sibling_rect.mLeft);
					}
				}
				break;
			case SNAP_BOTTOM:
				if (llabs(test_rect.mBottom - sibling_rect.mTop) <= y_threshold
					&& (test_rect.mBottom - sibling_rect.mTop) * mouse_dir.mY <= 0)
				{
					snap_pos = sibling_rect.mTop + padding;
					snap_view = siblingp;
					y_threshold = llabs(test_rect.mBottom - sibling_rect.mTop);
				}
				else if (llabs(sibling_rect.mRight - (test_rect.mLeft - padding)) <= x_threshold
					|| llabs(sibling_rect.mLeft - (test_rect.mRight + padding)) <= x_threshold)
				{
					if (llabs(test_rect.mBottom - sibling_rect.mBottom) <= y_threshold
						&& (test_rect.mBottom - sibling_rect.mBottom) * mouse_dir.mY <= 0)
					{
						snap_pos = sibling_rect.mBottom;
						snap_view = siblingp;
						y_threshold = llabs(test_rect.mBottom - sibling_rect.mBottom);
					}
				}
				break;
			case SNAP_TOP:
				if (llabs(test_rect.mTop - sibling_rect.mBottom) <= y_threshold
					&& (test_rect.mTop - sibling_rect.mBottom) * mouse_dir.mY <= 0)
				{
					snap_pos = sibling_rect.mBottom - padding;
					snap_view = siblingp;
					y_threshold = llabs(test_rect.mTop - sibling_rect.mBottom);
				}
				else if (llabs(sibling_rect.mRight - (test_rect.mLeft - padding)) <= x_threshold
					|| llabs(sibling_rect.mLeft - (test_rect.mRight + padding)) <= x_threshold)
				{
					if (llabs(test_rect.mTop - sibling_rect.mTop) <= y_threshold
						&& (test_rect.mTop - sibling_rect.mTop) * mouse_dir.mY <= 0)
					{
						snap_pos = sibling_rect.mTop;
						snap_view = siblingp;
						y_threshold = llabs(test_rect.mTop - sibling_rect.mTop);
					}
				}
				break;
			default:
				LL_ERRS() << "Invalid snap edge" << LL_ENDL;
			}
		}
	}
	new_edge_val = snap_pos;
	return snap_view;
}
void LLView::registerEventListener(std::string name, LLSimpleListener* function)
{
	mDispatchList.insert(std::pair<std::string, LLSimpleListener*>(name, function));
	LL_DEBUGS() << getName() << " registered " << name << LL_ENDL;
}
struct LLSignalListener : LLSimpleListener
{
	LLSignalListener(LLView::event_signal_t::slot_type &cb)
	{
		mSignal.connect(cb);
	}
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		mSignal(event, userdata);
		return true;
	}
	LLView::event_signal_t mSignal;
};
void LLView::registerEventListener(std::string name, event_signal_t::slot_type &cb)
{
	registerEventListener(name, new LLSignalListener(cb));
}
void LLView::deregisterEventListener(std::string name)
{
	dispatch_list_t::iterator itor = mDispatchList.find(name);
	if (itor != mDispatchList.end())
	{
		mDispatchList.erase(itor);
	}
}
std::string LLView::findEventListener(LLSimpleListener *listener) const
{
	dispatch_list_t::const_iterator itor;
	for (itor = mDispatchList.begin(); itor != mDispatchList.end(); ++itor)
	{
		if (itor->second == listener)
		{
			return itor->first;
		}
	}
	if (mParentView)
	{
		return mParentView->findEventListener(listener);
	}
	return LLStringUtil::null;
}
LLSimpleListener* LLView::getListenerByName(const std::string& callback_name)
{
	LLSimpleListener* callback = NULL;
	dispatch_list_t::iterator itor = mDispatchList.find(callback_name);
	if (itor != mDispatchList.end())
	{
		callback = itor->second;
	}
	else if (mParentView)
	{
		callback = mParentView->getListenerByName(callback_name);
	}
	return callback;
}
LLControlVariable *LLView::findControl(const std::string& name)
{
	control_map_t::iterator itor = mFloaterControls.find(name);
	if (itor != mFloaterControls.end())
	{
		return itor->second;
	}
	if (mParentView)
	{
		return mParentView->findControl(name);
	}
	if (LLControlVariable* control = LLUI::sConfigGroup->getControl(name))
		return control;
	return LLUI::sAccountGroup->getControl(name);
}
const S32 FLOATER_H_MARGIN = 15;
const S32 MIN_WIDGET_HEIGHT = 10;
const S32 VPAD = 4;
void LLView::initFromParams(const LLView::Params& params)
{
	LLRect required_rect = getRequiredRect();
	S32 width = llmax(getRect().getWidth(), required_rect.getWidth());
	S32 height = llmax(getRect().getHeight(), required_rect.getHeight());
	reshape(width, height);
	setEnabled(getEnabled());
	setVisible(getVisible());
	if (!params.name().empty())
	{
		setName(params.name());
	}
}
void LLView::parseFollowsFlags(const LLView::Params& params)
{
	if (params.follows.string.isChosen())
	{
		setFollows(FOLLOWS_NONE);
		std::string follows = params.follows.string;
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("|");
		tokenizer tokens(follows, sep);
		tokenizer::iterator token_iter = tokens.begin();
		while(token_iter != tokens.end())
		{
			const std::string& token_str = *token_iter;
			if (token_str == "left")
			{
				setFollowsLeft();
			}
			else if (token_str == "right")
			{
				setFollowsRight();
			}
			else if (token_str == "top")
			{
				setFollowsTop();
			}
			else if (token_str == "bottom")
			{
				setFollowsBottom();
			}
			else if (token_str == "all")
			{
				setFollowsAll();
			}
			++token_iter;
		}
	}
	else if (params.follows.flags.isChosen())
	{
		setFollows(params.follows.flags);
	}
}
LLView::tree_iterator_t LLView::beginTreeDFS()
{
	return tree_iterator_t(this,
							boost::bind(boost::mem_fn(&LLView::beginChild), _1),
							boost::bind(boost::mem_fn(&LLView::endChild), _1));
}
LLView::tree_iterator_t LLView::endTreeDFS()
{
	return tree_iterator_t();
}
LLView::tree_post_iterator_t LLView::beginTreeDFSPost()
{
	return tree_post_iterator_t(this,
							boost::bind(boost::mem_fn(&LLView::beginChild), _1),
							boost::bind(boost::mem_fn(&LLView::endChild), _1));
}
LLView::tree_post_iterator_t LLView::endTreeDFSPost()
{
	return tree_post_iterator_t();
}
LLView::bfs_tree_iterator_t LLView::beginTreeBFS()
{
	return bfs_tree_iterator_t(this,
							boost::bind(boost::mem_fn(&LLView::beginChild), _1),
							boost::bind(boost::mem_fn(&LLView::endChild), _1));
}
LLView::bfs_tree_iterator_t LLView::endTreeBFS()
{
	return bfs_tree_iterator_t();
}
LLView::root_to_view_iterator_t LLView::beginRootToView()
{
	return root_to_view_iterator_t(this, boost::bind(&LLView::getParent, _1));
}
LLView::root_to_view_iterator_t LLView::endRootToView()
{
	return root_to_view_iterator_t();
}
U32 LLView::createRect(LLXMLNodePtr node, LLRect &rect, LLView* parent_view, const LLRect &required_rect)
{
	U32 follows = 0;
	S32 x = rect.mLeft;
	S32 y = rect.mBottom;
	S32 w = rect.getWidth();
	S32 h = rect.getHeight();
	U32 last_x = 0;
	U32 last_y = 0;
	if (parent_view)
	{
		last_y = parent_view->getRect().getHeight();
		child_list_t::const_iterator itor = parent_view->getChildList()->begin();
		if (itor != parent_view->getChildList()->end())
		{
			LLView *last_view = (*itor);
			if (last_view->getSaveToXML())
			{
				last_x = last_view->getRect().mLeft;
				last_y = last_view->getRect().mBottom;
			}
		}
	}
	std::string rect_control;
	node->getAttributeString("rect_control", rect_control);
	if (! rect_control.empty())
	{
		LLRect rect = LLUI::sConfigGroup->getRect(rect_control);
		x = rect.mLeft;
		y = rect.mBottom;
		w = rect.getWidth();
		h = rect.getHeight();
	}
	if (node->hasAttribute("left"))
	{
		node->getAttributeS32("left", x);
	}
	if (node->hasAttribute("bottom"))
	{
		node->getAttributeS32("bottom", y);
	}
	if (parent_view)
	{
		if(w == 0)
		{
			w = llmax(required_rect.getWidth(), parent_view->getRect().getWidth() - (FLOATER_H_MARGIN) - x);
		}
		if(h == 0)
		{
			h = llmax(MIN_WIDGET_HEIGHT, required_rect.getHeight());
		}
	}
	if (node->hasAttribute("width"))
	{
		node->getAttributeS32("width", w);
	}
	if (node->hasAttribute("height"))
	{
		node->getAttributeS32("height", h);
	}
	if (parent_view)
	{
		if (node->hasAttribute("left_delta"))
		{
			S32 left_delta = 0;
			node->getAttributeS32("left_delta", left_delta);
			x = last_x + left_delta;
		}
		else if (node->hasAttribute("left") && node->hasAttribute("right"))
		{
			S32 right = 0;
			node->getAttributeS32("right", right);
			if (right < 0)
			{
				right = parent_view->getRect().getWidth() + right;
			}
			w = right - x;
		}
		else if (node->hasAttribute("left"))
		{
			if (x < 0)
			{
				x = parent_view->getRect().getWidth() + x;
				follows |= FOLLOWS_RIGHT;
			}
			else
			{
				follows |= FOLLOWS_LEFT;
			}
		}
		else if (node->hasAttribute("width") && node->hasAttribute("right"))
		{
			S32 right = 0;
			node->getAttributeS32("right", right);
			if (right < 0)
			{
				right = parent_view->getRect().getWidth() + right;
			}
			x = right - w;
		}
		else
		{
			x = last_x;
		}
		if (node->hasAttribute("bottom_delta"))
		{
			S32 bottom_delta = 0;
			node->getAttributeS32("bottom_delta", bottom_delta);
			y = last_y + bottom_delta;
		}
		else if (node->hasAttribute("top"))
		{
			S32 top = 0;
			node->getAttributeS32("top", top);
			if (top < 0)
			{
				top = parent_view->getRect().getHeight() + top;
			}
			h = top - y;
		}
		else if (node->hasAttribute("bottom"))
		{
			if (y < 0)
			{
				y = parent_view->getRect().getHeight() + y;
				follows |= FOLLOWS_TOP;
			}
			else
			{
				follows |= FOLLOWS_BOTTOM;
			}
		}
		else
		{
			if (last_y == 0)
			{
				y = parent_view->getRect().getHeight() - (h + VPAD);
				follows |= FOLLOWS_TOP;
			}
			else
			{
				y = last_y - (h + VPAD);
			}
		}
	}
	else
	{
		x = llmax(x, 0);
		y = llmax(y, 0);
		follows = FOLLOWS_LEFT | FOLLOWS_TOP;
	}
	rect.setOriginAndSize(x, y, w, h);
	return follows;
}
void LLView::initFromXML(LLXMLNodePtr node, LLView* parent)
{
	LLRect view_rect;
	U32 follows_flags = createRect(node, view_rect, parent, getRequiredRect());
	reshape(view_rect.getWidth(), view_rect.getHeight());
	setRect(view_rect);
	setFollows(follows_flags);
	parseFollowsFlags(node);
	if (node->hasAttribute("control_name"))
	{
		std::string control_name;
		node->getAttributeString("control_name", control_name);
		setControlName(control_name, NULL);
	}
	if (node->hasAttribute("tool_tip"))
	{
		std::string tool_tip_msg;
		node->getAttributeString("tool_tip", tool_tip_msg);
		setToolTip(tool_tip_msg);
	}
	if (node->hasAttribute("enabled"))
	{
		BOOL enabled;
		node->getAttributeBOOL("enabled", enabled);
		setEnabled(enabled);
	}
	if (node->hasAttribute("visible"))
	{
		BOOL visible;
		node->getAttributeBOOL("visible", visible);
		setVisible(visible);
	}
	if (node->hasAttribute("hover_cursor"))
	{
		std::string cursor_string;
		node->getAttributeString("hover_cursor", cursor_string);
		mHoverCursor = getCursorFromString(cursor_string);
	}
	node->getAttributeBOOL("use_bounding_rect", mUseBoundingRect);
	node->getAttributeBOOL("mouse_opaque", mMouseOpaque);
	node->getAttributeS32("default_tab_group", mDefaultTabGroup);
	if (node->hasAttribute("sound_flags"))
	{
		node->getAttributeU8("sound_flags", mSoundFlags);
	}
	reshape(view_rect.getWidth(), view_rect.getHeight());
}
void LLView::parseFollowsFlags(LLXMLNodePtr node)
{
	if (node->hasAttribute("follows"))
	{
		setFollowsNone();
		std::string follows;
		node->getAttributeString("follows", follows);
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("|");
		tokenizer tokens(follows, sep);
		tokenizer::iterator token_iter = tokens.begin();
		while(token_iter != tokens.end())
		{
			const std::string& token_str = *token_iter;
			if (token_str == "left")
			{
				setFollowsLeft();
			}
			else if (token_str == "right")
			{
				setFollowsRight();
			}
			else if (token_str == "top")
			{
				setFollowsTop();
			}
			else if (token_str == "bottom")
			{
				setFollowsBottom();
			}
			else if (token_str == "all")
			{
				setFollowsAll();
			}
			++token_iter;
		}
	}
}
LLFontGL* LLView::selectFont(LLXMLNodePtr node)
{
	std::string font_name, font_size, font_style;
	U8 style = 0;
	if (node->hasAttribute("font"))
	{
		node->getAttributeString("font", font_name);
	}
	if (node->hasAttribute("font_size"))
	{
		node->getAttributeString("font_size", font_size);
	}
	if (node->hasAttribute("font_style"))
	{
		node->getAttributeString("font_style", font_style);
		style = LLFontGL::getStyleFromString(font_style);
	}
	if (node->hasAttribute("font-style"))
	{
		node->getAttributeString("font-style", font_style);
		style = LLFontGL::getStyleFromString(font_style);
	}
	if (font_name.empty())
		return NULL;
	LLFontDescriptor desc(font_name, font_size, style);
	LLFontGL* gl_font = LLFontGL::getFont(desc);
	return gl_font;
}
LLFontGL::HAlign LLView::selectFontHAlign(LLXMLNodePtr node)
{
	LLFontGL::HAlign gl_hfont_align = LLFontGL::LEFT;
	if (node->hasAttribute("halign"))
	{
		std::string horizontal_align_name;
		node->getAttributeString("halign", horizontal_align_name);
		gl_hfont_align = LLFontGL::hAlignFromName(horizontal_align_name);
	}
	return gl_hfont_align;
}
LLFontGL::VAlign LLView::selectFontVAlign(LLXMLNodePtr node)
{
	LLFontGL::VAlign gl_vfont_align = LLFontGL::BASELINE;
	if (node->hasAttribute("valign"))
	{
		std::string vert_align_name;
		node->getAttributeString("valign", vert_align_name);
		gl_vfont_align = LLFontGL::vAlignFromName(vert_align_name);
	}
	return gl_vfont_align;
}
LLFontGL::StyleFlags LLView::selectFontStyle(LLXMLNodePtr node)
{
	LLFontGL::StyleFlags gl_font_style = LLFontGL::NORMAL;
	if (node->hasAttribute("style"))
	{
		std::string style_flags_name;
		node->getAttributeString("style", style_flags_name);
		if (style_flags_name == "normal")
		{
			gl_font_style = LLFontGL::NORMAL;
		}
		else if (style_flags_name == "bold")
		{
			gl_font_style = LLFontGL::BOLD;
		}
		else if (style_flags_name == "italic")
		{
			gl_font_style = LLFontGL::ITALIC;
		}
		else if (style_flags_name == "underline")
		{
			gl_font_style = LLFontGL::UNDERLINE;
		}
	}
	return gl_font_style;
}
bool LLView::setControlValue(const LLSD& value)
{
	std::string ctrlname = getControlName();
	if (!ctrlname.empty())
	{
		LLUI::getControlControlGroup(ctrlname).setUntypedValue(ctrlname, value);
		return true;
	}
	return false;
}
void LLView::setControlName(const std::string& control_name, LLView *context)
{
	if (context == NULL)
	{
		context = this;
	}
	if (!mControlName.empty())
	{
		LL_WARNS() << "setControlName called twice on same control!" << LL_ENDL;
		mControlConnection.disconnect();
		mControlName.clear();
	}
	if (!control_name.empty())
	{
		LLControlVariable *control = context->findControl(control_name);
		if (control)
		{
			mControlName = control_name;
			mControlConnection = control->getSignal()->connect(boost::bind(&controlListener, _2, getHandle(), std::string("value")));
			setValue(control->getValue());
		}
	}
}
bool LLView::controlListener(const LLSD& newvalue, LLHandle<LLView> handle, std::string type)
{
	LLView* view = handle.get();
	if (view)
	{
		if (type == "value")
		{
			view->setValue(newvalue);
			return true;
		}
		else if (type == "enabled")
		{
			view->setEnabled(newvalue.asBoolean());
			return true;
		}
		else if (type == "visible")
		{
			view->setVisible(newvalue.asBoolean());
			return true;
		}
	}
	return false;
}
void LLView::addBoolControl(const std::string& name, bool initial_value)
{
	mFloaterControls[name] = new LLControlVariable(name, TYPE_BOOLEAN, initial_value, std::string("Internal floater control"));
}
LLControlVariable *LLView::getControl(const std::string& name)
{
	control_map_t::iterator itor = mFloaterControls.find(name);
	if (itor != mFloaterControls.end())
	{
		return itor->second;
	}
	return NULL;
}
void	LLView::setValue(const LLSD& value)
{
}
LLSD	LLView::getValue() const
{
	return LLSD();
}
S32	LLView::notifyParent(const LLSD& info)
{
	LLView* parent = getParent();
	if(parent)
		return parent->notifyParent(info);
	return 0;
}
bool	LLView::notifyChildren(const LLSD& info)
{
	bool ret = false;
	for (LLView* childp : mChildList)
	{
		ret = ret || childp->notifyChildren(info);
	}
	return ret;
}
const LLViewDrawContext& LLView::getDrawContext()
{
	return LLViewDrawContext::getCurrentContext();
}
const LLViewDrawContext& LLViewDrawContext::getCurrentContext()
{
	static LLViewDrawContext default_context;
	if (sDrawContextStack.empty())
		return default_context;
	return *sDrawContextStack.back();
}
LLView* LLView::createWidget(LLXMLNodePtr xml_node) const
{
	return LLUICtrlFactory::getInstance()->createCtrlWidget(NULL, xml_node);
}
LLWidgetClassRegistry::LLWidgetClassRegistry()
{
}
void LLWidgetClassRegistry::registerCtrl(const std::string& tag, LLWidgetClassRegistry::factory_func_t function)
{
	std::string lower_case_tag = tag;
	LLStringUtil::toLower(lower_case_tag);
	mCreatorFunctions[lower_case_tag] = function;
}
BOOL LLWidgetClassRegistry::isTagRegistered(const std::string &tag)
{
	return mCreatorFunctions.find(tag) != mCreatorFunctions.end();
}
LLWidgetClassRegistry::factory_func_t LLWidgetClassRegistry::getCreatorFunc(const std::string& ctrl_type)
{
	factory_map_t::const_iterator found_it = mCreatorFunctions.find(ctrl_type);
	if (found_it == mCreatorFunctions.end())
	{
		return NULL;
	}
	return found_it->second;
}
