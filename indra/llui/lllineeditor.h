/** 
 * @file lllineeditor.h
 * @brief Text editor widget to let users enter/edit a single line.
 *
 * Features: 
 *		Text entry of a single line (text, delete, left and right arrow, insert, return).
 *		Callbacks either on every keystroke or just on the return key.
 *		Focus (allow multiple text entry widgets)
 *		Clipboard (cut, copy, and paste)
 *		Horizontal scrolling to allow strings longer than widget size allows 
 *		Pre-validation (limit which keys can be used)
 *		Optional line history so previous entries can be recalled by CTRL UP/DOWN
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
#ifndef LL_LLLINEEDITOR_H
#define LL_LLLINEEDITOR_H
#include "v4color.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include "lleditmenuhandler.h"
#include "lluictrl.h"
#include "lluiimage.h"
#include "lluistring.h"
#include "llviewborder.h"
#include "llpreeditor.h"
class LLFontGL;
class LLLineEditorRollback;
class LLButton;
class LLMenuGL;
class LLLineEditor
: public LLUICtrl, public LLEditMenuHandler, protected LLPreeditor
{
public:
	typedef boost::function<void (LLLineEditor* caller)> keystroke_callback_t;
	typedef boost::function<void (LLFocusableElement*)> focus_lost_callback_t;
	typedef boost::function<BOOL (const LLWString &wstr)> validate_func_t;
	LLLineEditor(const std::string& name,
				 const LLRect& rect,
				 const std::string& default_text = LLStringUtil::null,
				 const LLFontGL* glfont = NULL,
				 S32 max_length_bytes = 254,
				 commit_callback_t commit_callback = NULL,
				 keystroke_callback_t keystroke_callback = NULL,
				 focus_lost_callback_t focus_lost_callback = NULL,
				 validate_func_t prevalidate_func = NULL,
				 LLViewBorder::EBevel border_bevel = LLViewBorder::BEVEL_IN,
				 LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE,
				 S32 border_thickness = 1);
protected:
	void showContextMenu(S32 x, S32 y);
public:
	virtual ~LLLineEditor();
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	void setColorParameters(LLXMLNodePtr node);
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	static void cleanupLineEditor();
	void setFont(const LLFontGL* font) { if (font) mGLFont = font; }
	const LLFontGL* getFont() const { return mGLFont; }
	void setVAlign(LLFontGL::VAlign align) { mVAlign = align; }
	LLFontGL::VAlign getVAlign() const { return mVAlign; }
	void setUIImage(LLPointer<LLUIImage> image) { mImage = image; }
	BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL	handleHover(S32 x, S32 y, MASK mask);
	BOOL	handleDoubleClick(S32 x,S32 y,MASK mask);
	BOOL	handleMiddleMouseDown(S32 x,S32 y,MASK mask);
	BOOL	handleRightMouseDown( S32 x, S32 y, MASK mask );
	BOOL	handleKeyHere(KEY key, MASK mask );
	BOOL	handleUnicodeCharHere(llwchar uni_char);
	void	onMouseCaptureLost();
	virtual void insert(std::string what,S32 wher);
	virtual void	cut();
	virtual BOOL	canCut() const;
	void			copy() const override final;
	virtual BOOL	canCopy() const;
	virtual void	paste();
	virtual BOOL	canPaste() const;
	virtual void	updatePrimary();
	virtual void	copyPrimary();
 	virtual void	pastePrimary();
	virtual BOOL	canPastePrimary() const;
	virtual void	doDelete();
	virtual BOOL	canDoDelete() const;
	virtual void	selectAll();
	virtual BOOL	canSelectAll() const;
	virtual void	deselect();
	virtual BOOL	canDeselect() const;
	static void spell_correct(void* data);
	static void spell_show(void* data);
	static void spell_add(void* data);
    std::vector<S32> getMisspelledWordsPositions();
	virtual void	draw();
    void drawMisspelled(LLRect background);
	virtual void	reshape(S32 width,S32 height,BOOL called_from_parent=TRUE);
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	virtual void	setEnabled(BOOL enabled);
	virtual void	clear();
	virtual void	onTabInto();
	virtual void	setFocus( BOOL b );
	virtual void 	setRect(const LLRect& rect);
	virtual BOOL	acceptsTextInput() const;
	virtual void	onCommit();
	virtual BOOL	isDirty() const { return mText.getString() != mPrevText; }
	virtual void	resetDirty() { mPrevText = mText.getString(); }
    virtual BOOL	isSpellDirty() const { return mText.getString() != mPrevSpelledText; }
    virtual void	resetSpellDirty() { mPrevSpelledText = mText.getString(); }
	typedef boost::function<void(S32&, S32&, LLWString&, S32&, const LLWString&)> autoreplace_callback_t;
	autoreplace_callback_t mAutoreplaceCallback;
	void setAutoreplaceCallback(autoreplace_callback_t cb) { mAutoreplaceCallback = cb; }
	virtual void	setValue(const LLSD& value ) { setText(value.asString()); }
	virtual LLSD	getValue() const { return LLSD(getText()); }
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	void			setLabel(const LLStringExplicit &new_label) { mLabel = new_label; }
	void			setText(const LLStringExplicit &new_text);
	const std::string& getText() const		{ return mText.getString(); }
	const LLWString& getWText() const	{ return mText.getWString(); }
	LLWString getConvertedText() const;
	S32				getLength() const	{ return mText.length(); }
	S32				getCursor()	const	{ return mCursorPos; }
	void			setCursor( S32 pos );
	void			setCursorToEnd();
	void			resetScrollPosition();
	void			setSelection(S32 start, S32 end);
	void			setCommitOnFocusLost( BOOL b )	{ mCommitOnFocusLost = b; }
	void			setRevertOnEsc( BOOL b )		{ mRevertOnEsc = b; }
	void setCursorColor(const LLColor4& c)			{ mCursorColor = c; }
	const LLColor4& getCursorColor() const			{ return mCursorColor; }
	void setFgColor( const LLColor4& c )			{ mFgColor = c; }
	void setReadOnlyFgColor( const LLColor4& c )	{ mReadOnlyFgColor = c; }
	void setTentativeFgColor(const LLColor4& c)		{ mTentativeFgColor = c; }
	void setWriteableBgColor( const LLColor4& c )	{ mWriteableBgColor = c; }
	void setReadOnlyBgColor( const LLColor4& c )	{ mReadOnlyBgColor = c; }
	void setFocusBgColor(const LLColor4& c)			{ mFocusBgColor = c; }
	void setSpellCheckable(BOOL b)					{ mSpellCheckable = b; }
	const LLColor4& getFgColor() const			{ return mFgColor; }
	const LLColor4& getReadOnlyFgColor() const	{ return mReadOnlyFgColor; }
	const LLColor4& getTentativeFgColor() const { return mTentativeFgColor; }
	const LLColor4& getWriteableBgColor() const	{ return mWriteableBgColor; }
	const LLColor4& getReadOnlyBgColor() const	{ return mReadOnlyBgColor; }
	const LLColor4& getFocusBgColor() const		{ return mFocusBgColor; }
	void			setIgnoreArrowKeys(BOOL b)		{ mIgnoreArrowKeys = b; }
	void			setIgnoreTab(BOOL b)			{ mIgnoreTab = b; }
	void			setPassDelete(BOOL b)			{ mPassDelete = b; }
	void			setDrawAsterixes(BOOL b);
	S32				prevWordPos(S32 cursorPos) const;
	S32				nextWordPos(S32 cursorPos) const;
	BOOL			getWordBoundriesAt(const S32 at, S32* word_begin, S32* word_length) const;
	BOOL			hasSelection() const { return (mSelectionStart != mSelectionEnd); }
	void			startSelection();
	void			endSelection();
	void			extendSelection(S32 new_cursor_pos);
	void			deleteSelection();
	void			setHandleEditKeysDirectly( BOOL b ) { mHandleEditKeysDirectly = b; }
	void			setSelectAllonFocusReceived(BOOL b);
	void			setSelectAllonCommit(BOOL b) { mSelectAllonCommit = b; }
	void			onKeystroke();
	void			setKeystrokeCallback(keystroke_callback_t callback);
	void			setMaxTextLength(S32 max_text_length);
	void			setTextPadding(S32 left, S32 right);
	void			setPrevalidate( validate_func_t func );
	static BOOL		prevalidateFloat(const LLWString &str );
	static BOOL		prevalidateInt(const LLWString &str );
	static BOOL		prevalidatePositiveS32(const LLWString &str);
	static BOOL		prevalidateNonNegativeS32(const LLWString &str);
	static BOOL		prevalidateAlphaNum(const LLWString &str );
	static BOOL		prevalidateAlphaNumSpace(const LLWString &str );
	static BOOL		prevalidatePrintableNotPipe(const LLWString &str);
	static BOOL		prevalidatePrintableNoSpace(const LLWString &str);
	static BOOL		prevalidateASCII(const LLWString &str);
	static BOOL		postvalidateFloat(const std::string &str);
	BOOL			evaluateFloat();
	void			setEnableLineHistory( BOOL enabled ) { mHaveHistory = enabled; }
	void			updateHistory();
	void			setReplaceNewlinesWithSpaces(BOOL replace);
	void			setContextMenu(LLMenuGL* new_context_menu);
private:
	void            pasteHelper(bool is_primary);
	void			removeChar();
	void			removeWord(bool prev);
	void			addChar(const llwchar c);
	void			setCursorAtLocalPos(S32 local_mouse_x);
	S32				calculateCursorFromMouse(S32 local_mouse_x);
	S32				findPixelNearestPos(S32 cursor_offset = 0) const;
	void			reportBadKeystroke();
	BOOL			handleSpecialKey(KEY key, MASK mask);
	BOOL			handleSelectionKey(KEY key, MASK mask);
	BOOL			handleControlKey(KEY key, MASK mask);
	S32				handleCommitKey(KEY key, MASK mask);
	void			updateAllowingLanguageInput();
	BOOL			hasPreeditString() const;
	virtual void	resetPreedit();
	virtual void	updatePreedit(const LLWString &preedit_string,
						const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position);
	virtual void	markAsPreedit(S32 position, S32 length);
	virtual void	getPreeditRange(S32 *position, S32 *length) const;
	virtual void	getSelectionRange(S32 *position, S32 *length) const;
	virtual BOOL	getPreeditLocation(S32 query_position, LLCoordGL *coord, LLRect *bounds, LLRect *control) const;
	virtual S32		getPreeditFontSize() const;
protected:
	LLUIString		mText;
	std::string		mPrevText;
	LLUIString		mLabel;
    std::string		 mPrevSpelledText;
    std::vector<S32> misspellLocations;
    S32				 mStartSpellHere;
    S32				 mEndSpellHere;
	BOOL			mSpellCheckable;
    LLFrameTimer     mSpellTimer;
	S32 mLastContextMenuX;
	BOOL		mHaveHistory;
	typedef	std::vector<std::string>	line_history_t;
	line_history_t				mLineHistory;
	line_history_t::iterator	mCurrentHistoryLine;
	LLViewBorder* mBorder;
	const LLFontGL*	mGLFont;
	LLFontGL::VAlign mVAlign;
	S32			mMaxLengthBytes;
	S32			mCursorPos;
	S32			mScrollHPos;
	LLFrameTimer mScrollTimer;
	S32			mTextPadLeft;
	S32			mTextPadRight;
	S32			mMinHPixels;
	S32			mMaxHPixels;
	BOOL		mCommitOnFocusLost;
	BOOL		mRevertOnEsc;
	keystroke_callback_t mKeystrokeCallback;
	BOOL		mIsSelecting;
	S32			mSelectionStart;
	S32			mSelectionEnd;
	S32			mLastSelectionX;
	S32			mLastSelectionY;
	S32			mLastSelectionStart;
	S32			mLastSelectionEnd;
	validate_func_t mPrevalidateFunc;
	LLFrameTimer mKeystrokeTimer;
	LLColor4	mCursorColor;
	LLColor4	mFgColor;
	LLColor4	mReadOnlyFgColor;
	LLColor4	mTentativeFgColor;
	LLColor4	mWriteableBgColor;
	LLColor4	mReadOnlyBgColor;
	LLColor4	mFocusBgColor;
	S32			mBorderThickness;
	BOOL		mIgnoreArrowKeys;
	BOOL		mIgnoreTab;
	BOOL		mDrawAsterixes;
	BOOL		mHandleEditKeysDirectly;
	BOOL		mSelectAllonFocusReceived;
	BOOL		mSelectAllonCommit;
	BOOL		mPassDelete;
	BOOL		mReadOnly;
	LLWString	mPreeditWString;
	LLWString	mPreeditOverwrittenWString;
	std::vector<S32> mPreeditPositions;
	LLPreeditor::standouts_t mPreeditStandouts;
	LLHandle<LLView> mContextMenuHandle;
private:
	static LLPointer<LLUIImage> parseImage(std::string name, LLXMLNodePtr from, LLPointer<LLUIImage> def);
	static LLPointer<LLUIImage> sImage;
	LLPointer<LLUIImage> mImage;
	BOOL        mReplaceNewlinesWithSpaces;
	class LLLineEditorRollback
	{
	public:
		LLLineEditorRollback( LLLineEditor* ed )
			:
			mCursorPos( ed->mCursorPos ),
			mScrollHPos( ed->mScrollHPos ),
			mIsSelecting( ed->mIsSelecting ),
			mSelectionStart( ed->mSelectionStart ),
			mSelectionEnd( ed->mSelectionEnd )
		{
			mText = ed->getText();
		}
		void doRollback( LLLineEditor* ed )
		{
			ed->mCursorPos = mCursorPos;
			ed->mScrollHPos = mScrollHPos;
			ed->mIsSelecting = mIsSelecting;
			ed->mSelectionStart = mSelectionStart;
			ed->mSelectionEnd = mSelectionEnd;
			ed->mText = mText;
			ed->mPrevText = mText;
		}
		std::string getText()   { return mText; }
	private:
		std::string mText;
		S32		mCursorPos;
		S32		mScrollHPos;
		BOOL	mIsSelecting;
		S32		mSelectionStart;
		S32		mSelectionEnd;
	};
};
#endif
