/** 
 * @file LLAccordionCtrl.h
 * @brief Accordion Panel implementation
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
#ifndef LL_ACCORDIONCTRL_H
#define LL_ACCORDIONCTRL_H
#include "llpanel.h"
#include "lltextbox.h"
#include "llscrollbar.h"
#include <vector>
#include <algorithm>
#include <string>
class LLAccordionCtrlTab;
class LLAccordionCtrl: public LLPanel
{
private:
	std::vector<LLAccordionCtrlTab*> mAccordionTabs;
	void ctrlSetLeftTopAndSize(LLView* panel, S32 left, S32 top, S32 width, S32 height);
	void ctrlShiftVertical(LLView* panel,S32 delta);
	void onCollapseCtrlCloseOpen(S16 panel_num);
	void shiftAccordionTabs(S16 panel_num, S32 delta);
public:
	class LLTabComparator
	{
	public:
		LLTabComparator() {};
		virtual ~LLTabComparator() {};
		virtual bool compare(const LLAccordionCtrlTab* tab1, const LLAccordionCtrlTab* tab2) const = 0;
	};
	#if 0
	struct Params
		: public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<bool>			single_expansion,
								fit_parent;
		Optional<LLTextBox::Params>	no_matched_tabs_text;
		Optional<LLTextBox::Params>	no_visible_tabs_text;
		Params()
			: single_expansion("single_expansion",false)
			, fit_parent("fit_parent", false)
			, no_matched_tabs_text("no_matched_tabs_text")
			, no_visible_tabs_text("no_visible_tabs_text")
		{};
	};
	LLAccordionCtrl(const Params& params);
	#endif
    LLAccordionCtrl();
    virtual ~LLAccordionCtrl();
	virtual BOOL postBuild();
	static LLView* fromXML(LLXMLNodePtr node, LLView* parent, class LLUICtrlFactory* factory);
	virtual void initFromXML(LLXMLNodePtr node, LLView* parent);
	virtual BOOL handleRightMouseDown	( S32 x, S32 y, MASK mask);
	virtual BOOL handleScrollWheel		( S32 x, S32 y, S32 clicks );
	virtual BOOL handleKeyHere			(KEY key, MASK mask);
	virtual BOOL handleDragAndDrop		(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg);
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void addCollapsibleCtrl(LLView* view);
	void removeCollapsibleCtrl(LLView* view);
	void arrange();
	void	draw();
	void	onScrollPosChangeCallback(S32, LLScrollbar*);
	S32		notifyParent(const LLSD& info);
	void	reset		();
	void	expandDefaultTab();
	void	setComparator(const LLTabComparator* comp) { mTabComparator = comp; }
	void	sort();
	void	setFilterSubString(const std::string& filter_string);
	const LLAccordionCtrlTab* getExpandedTab() const;
	LLAccordionCtrlTab* getSelectedTab() const { return mSelectedTab; }
	bool getFitParent() const {return mFitParent;}
private:
	void	initNoTabsWidget(const std::string& tb_params);
	void	updateNoTabsHelpTextVisibility();
	void	arrangeSinge();
	void	arrangeMultiple();
	S32		calcRecuiredHeight();
	S32		getRecuiredHeight() const { return mInnerRect.getHeight(); }
	S32		calcExpandedTabHeight(S32 tab_index = 0, S32 available_height = 0);
	void	updateLayout			(S32 width, S32 height);
	void	show_hide_scrollbar		(S32 width, S32 height);
	void	showScrollbar			(S32 width, S32 height);
	void	hideScrollbar			(S32 width, S32 height);
	BOOL	autoScroll				(S32 x, S32 y);
	struct LLComparatorAdaptor
	{
		LLComparatorAdaptor(const LLTabComparator& comparator) : mComparator(comparator) {};
		bool operator()(const LLAccordionCtrlTab* tab1, const LLAccordionCtrlTab* tab2)
		{
			return mComparator.compare(tab1, tab2);
		}
		const LLTabComparator& mComparator;
	};
private:
	LLRect			mInnerRect;
	LLScrollbar*	mScrollbar;
	bool			mSingleExpansion;
	bool			mFitParent;
	bool			mAutoScrolling;
	F32				mAutoScrollRate;
	LLTextBox*		mNoVisibleTabsHelpText;
	std::string		mNoMatchedTabsOrigString;
	std::string		mNoVisibleTabsOrigString;
	LLAccordionCtrlTab*		mSelectedTab;
	const LLTabComparator*	mTabComparator;
};
#endif
