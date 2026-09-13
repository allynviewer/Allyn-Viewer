/** 
 * @file LLAccordionCtrlTab.h
 * @brief Collapsible box control implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#ifndef LL_ACCORDIONCTRLTAB_H_
#define LL_ACCORDIONCTRLTAB_H_
#include <string>
#include "llrect.h"
#include "lluictrl.h"
#include "lluicolor.h"
#include "llstyle.h"
class LLUICtrlFactory;
class LLUIImage;
class LLButton;
class LLTextBox;
class LLScrollbar;
class LLAccordionCtrlTab : public LLUICtrl
{
public:
	struct Params
	 : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool>			display_children,
								collapsible;
		Optional<std::string>	title;
		Optional<S32>			header_height,
								min_width,
								min_height;
		Mandatory<LLUIImage*>	header_expand_img,
								header_expand_img_pressed,
								header_collapse_img,
								header_collapse_img_pressed;
		Mandatory<LLUIImage*>	header_image,
								header_image_over,
								header_image_pressed,
								header_image_focused;
		Optional<LLUIColor>		header_bg_color,
								header_text_color,
								dropdown_bg_color;
		Optional<bool>			header_visible;
		Optional<bool>			fit_panel;
		Optional<bool>			selection_enabled;
		Optional<S32>			padding_left,
								padding_right,
								padding_top,
								padding_bottom;
		Params();
	};
	virtual ~LLAccordionCtrlTab();
	boost::signals2::connection setDropDownStateChangedCallback(commit_callback_t cb);
	virtual void setDisplayChildren(bool display);
	virtual bool getDisplayChildren() const {return mDisplayChildren;};
	void		setAccordionView(LLView* panel);
	LLView*		getAccordionView() { return mContainerPanel; };
	std::string getTitle() const;
	void setTitle(const std::string& title, const std::string& hl = LLStringUtil::null);
	void setTitleFontStyle(std::string style);
	void setTitleColor(LLUIColor color);
	boost::signals2::connection setFocusReceivedCallback(const focus_signal_t::slot_type& cb);
	boost::signals2::connection setFocusLostCallback(const focus_signal_t::slot_type& cb);
	void setSelected(bool is_selected);
	bool getCollapsible() {return mCollapsible;};
	void setCollapsible(bool collapsible) {mCollapsible = collapsible;};
	void changeOpenClose(bool is_open);
	void canOpenClose(bool can_open_close) { mCanOpenClose = can_open_close;};
	bool canOpenClose() const { return mCanOpenClose; };
	virtual BOOL postBuild();
	static LLView* fromXML(LLXMLNodePtr node, LLView* parent, class LLUICtrlFactory* factory);
	S32	notifyParent(const LLSD& info);
	S32 notify(const LLSD& info);
	bool notifyChildren(const LLSD& info);
	void draw();
	void    storeOpenCloseState		();
	void    restoreOpenCloseState	();
protected:
	LLAccordionCtrlTab(const LLAccordionCtrlTab::Params&);
	friend class LLUICtrlFactory;
public:
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect);
	virtual BOOL handleScrollWheel( S32 x, S32 y, S32 clicks );
	virtual bool addChild(LLView* child, S32 tab_group = 0 );
	bool isExpanded() const { return mDisplayChildren; }
	S32 getHeaderHeight();
	void setHeaderVisible(bool value);
	bool getHeaderVisible() { return mHeaderVisible;}
	S32 mExpandedHeight;
	S32	getPaddingLeft() const { return mPaddingLeft;}
	S32	getPaddingRight() const { return mPaddingRight;}
	S32	getPaddingTop() const { return mPaddingTop;}
	S32	getPaddingBottom() const { return mPaddingBottom;}
	void showAndFocusHeader();
	void setFitPanel( bool fit ) { mFitPanel = true; }
	bool getFitParent() const { return mFitPanel; }
protected:
	void adjustContainerPanel	(const LLRect& child_rect);
	void adjustContainerPanel	();
	S32	 getChildViewHeight		();
	void onScrollPosChangeCallback(S32, LLScrollbar*);
	void show_hide_scrollbar	(const LLRect& child_rect);
	void showScrollbar			(const LLRect& child_rect);
	void hideScrollbar			(const LLRect& child_rect);
	void updateLayout			( const LLRect& child_rect );
	void ctrlSetLeftTopAndSize	(LLView* panel, S32 left, S32 top, S32 width, S32 height);
	void drawChild(const LLRect& root_rect,LLView* child);
	LLView* findContainerView	();
	void selectOnFocusReceived();
	void deselectOnFocusLost();
private:
	class LLAccordionCtrlTabHeader;
	LLAccordionCtrlTabHeader* mHeader;
	bool mDisplayChildren;
	bool mCollapsible;
	bool mHeaderVisible;
	bool mCanOpenClose;
	bool mFitPanel;
	S32	mPaddingLeft;
	S32	mPaddingRight;
	S32	mPaddingTop;
	S32	mPaddingBottom;
	bool mStoredOpenCloseState;
	bool mWasStateStored;
	bool mSelectionEnabled;
	LLScrollbar*	mScrollbar;
	LLView*			mContainerPanel;
	LLUIColor mDropdownBGColor;
};
#endif
