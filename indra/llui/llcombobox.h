/** 
 * @file llcombobox.h
 * @brief LLComboBox base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LL_LLCOMBOBOX_H
#define LL_LLCOMBOBOX_H
#include "llbutton.h"
#include "lluictrl.h"
#include "llctrlselectioninterface.h"
#include "llrect.h"
#include "llscrolllistctrl.h"
#include "llsearchablecontrol.h"
class LLFontGL;
class LLLineEditor;
class LLViewBorder;
extern S32 LLCOMBOBOX_HEIGHT;
extern S32 LLCOMBOBOX_WIDTH;
class LLComboBox
:	public LLUICtrl, public LLCtrlListInterface, public ll::ui::SearchableControl
{
public:
	typedef enum e_preferred_position
	{
		ABOVE,
		BELOW
	} EPreferredPosition;
	virtual ~LLComboBox();
protected:
	friend class LLFloaterTestImpl;
	friend class LLUICtrlFactory;
	LLComboBox(const std::string& name, const LLRect& rect, const std::string& label, commit_callback_t commit_callback = NULL);
	void	prearrangeList(std::string filter = "");
public:
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	onFocusLost();
	virtual void	setEnabled(BOOL enabled);
	virtual BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void	clear();
	virtual void	onCommit();
	virtual BOOL	acceptsTextInput() const		{ return mAllowTextEntry; }
	virtual BOOL	isDirty() const;
	virtual void	resetDirty();
	virtual void	setFocus(BOOL b);
	void			setPrevalidate( BOOL (*func)(const LLWString &) );
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;
	void			setAllowTextEntry(BOOL allow, S32 max_chars = 50, BOOL make_tentative = TRUE);
	void			setTextEntry(const LLStringExplicit& text);
	const std::string	getTextEntry() const;
	void			setFocusText(BOOL b);
	BOOL			isTextDirty() const;
	void			resetTextDirty();
	LLScrollListItem*	add(const std::string& name, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	LLScrollListItem*	add(const std::string& name, const LLUUID& id, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	LLScrollListItem*	add(const std::string& name, void* userdata, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	LLScrollListItem*	add(const std::string& name, LLSD value, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	LLScrollListItem*	addSeparator(EAddPosition pos = ADD_BOTTOM);
	BOOL			remove( S32 index );
	void			removeall() { clearRows(); }
	bool			itemExists(const std::string& name);
	void			sortByName(BOOL ascending = TRUE);
	BOOL			setSimple(const LLStringExplicit& name);
	const std::string	getSimple() const;
	virtual const std::string getSelectedItemLabel(S32 column = 0) const;
	void			setLabel(const LLStringExplicit& name);
	void			updateLabel();
	BOOL			remove(const std::string& name);
	BOOL			setCurrentByIndex( S32 index );
	S32				getCurrentIndex() const;
	virtual void	updateLayout();
	LLCtrlSelectionInterface* getSelectionInterface()	{ return (LLCtrlSelectionInterface*)this; };
	LLCtrlListInterface* getListInterface()				{ return (LLCtrlListInterface*)this; };
	virtual S32		getItemCount() const;
	virtual void 	addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
	virtual void 	clearColumns();
	virtual void	setColumnLabel(const std::string& column, const std::string& label);
	virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	virtual LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos = ADD_BOTTOM, const LLSD& id = LLSD());
	virtual void 	clearRows();
	virtual void 	sortByColumn(const std::string& name, BOOL ascending);
	virtual BOOL	getCanSelect() const				{ return TRUE; }
	virtual BOOL	selectFirstItem()					{ return setCurrentByIndex(0); }
	virtual BOOL	selectNthItem( S32 index )			{ return setCurrentByIndex(index); }
	virtual BOOL	selectItemRange( S32 first, S32 last );
	virtual S32		getFirstSelectedIndex() const		{ return getCurrentIndex(); }
	virtual BOOL	setCurrentByID( const LLUUID& id );
	virtual LLUUID	getCurrentID() const;
	virtual BOOL	setSelectedByValue(const LLSD& value, BOOL selected);
	virtual LLSD	getSelectedValue();
	virtual BOOL	isSelected(const LLSD& value) const;
	virtual BOOL	operateOnSelection(EOperation op);
	virtual BOOL	operateOnAll(EOperation op);
	void*			getCurrentUserdata();
	void			setPrearrangeCallback( commit_callback_t cb ) { mPrearrangeCallback = cb; }
	void			setTextEntryCallback( commit_callback_t cb ) { mTextEntryCallback = cb; }
	void			setButtonVisible(BOOL visible);
	void			setButtonImages(const std::string& unselected, const std::string& selected);
	void			onButtonMouseDown();
	void			onListMouseUp();
	void			onItemSelected(const LLSD& data);
	void			onTextCommit(const LLSD& data);
	void			setSuppressTentative(bool suppress);
	void			setSuppressAutoComplete(bool suppress);
	void			updateSelection();
	virtual void	showList();
	virtual void	hideList();
	virtual void	onTextEntry(LLLineEditor* line_editor);
protected:
	LLButton*			mButton;
	LLLineEditor*		mTextEntry;
	LLScrollListCtrl*	mList;
	EPreferredPosition	mListPosition;
	LLPointer<LLUIImage>	mArrowImage;
	std::string			mLabel;
	BOOL				mHasAutocompletedText;
	LLColor4				mListColor;
private:
	BOOL				mAllowTextEntry;
	BOOL				mAllowNewValues;
	S32					mMaxChars;
	BOOL				mTextEntryTentative;
	bool				mSuppressAutoComplete;
	bool				mSuppressTentative;
	commit_callback_t	mPrearrangeCallback;
	commit_callback_t	mTextEntryCallback;
	boost::signals2::connection mTopLostSignalConnection;
	S32                 mLastSelectedIndex;
protected:
	virtual std::string _getSearchText() const;
	virtual void onSetHighlight() const;
};
#endif
