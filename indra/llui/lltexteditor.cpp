/** 
 * @file lltexteditor.cpp
 * @brief LLTextEditor base class
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
#include "lltexteditor.h"
#include "llfontgl.h"
#include "lllocalcliprect.h"
#include "llrender.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "lluiimage.h"
#include "llurlaction.h"
#include "llurlregistry.h"
#include "llrect.h"
#include "llfocusmgr.h"
#include "lltimer.h"
#include "llmath.h"
#include "llclipboard.h"
#include "llscrollbar.h"
#include "llstl.h"
#include "llstring.h"
#include "llkeyboard.h"
#include "llkeywords.h"
#include "llundo.h"
#include "llviewborder.h"
#include "llfasttimer.h"
#include "llcontrol.h"
#include "llimagegl.h"
#include "llwindow.h"
#include "lltextparser.h"
#include "lldir.h"
#include <queue>
#include "llmemberlistener.h"
#include "llmenugl.h"
#include "../newview/lgghunspell_wrapper.h"
static LLRegisterWidget<LLTextEditor> r("simple_text_editor");
BOOL gDebugTextEditorTips = FALSE;
const S32	UI_TEXTEDITOR_BUFFER_BLOCK_SIZE = 512;
const S32	UI_TEXTEDITOR_BORDER = 1;
const S32	UI_TEXTEDITOR_H_PAD = 4;
const S32	UI_TEXTEDITOR_V_PAD_TOP = 4;
const S32	UI_TEXTEDITOR_LINE_NUMBER_MARGIN = 32;
const S32	UI_TEXTEDITOR_LINE_NUMBER_DIGITS = 4;
const F32	CURSOR_FLASH_DELAY = 1.0f;
const S32	CURSOR_THICKNESS = 2;
const S32	SPACES_PER_TAB = 4;
const F32	PREEDIT_MARKER_BRIGHTNESS = 0.4f;
const S32	PREEDIT_MARKER_GAP = 1;
const S32	PREEDIT_MARKER_POSITION = 2;
const S32	PREEDIT_MARKER_THICKNESS = 1;
const F32	PREEDIT_STANDOUT_BRIGHTNESS = 0.6f;
const S32	PREEDIT_STANDOUT_GAP = 1;
const S32	PREEDIT_STANDOUT_POSITION = 2;
const S32	PREEDIT_STANDOUT_THICKNESS = 2;
class LLTextEditor::LLTextCmdInsert : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdInsert(S32 pos, BOOL group_with_next, const LLWString &ws)
		: LLTextCmd(pos, group_with_next), mWString(ws)
	{
	}
	virtual ~LLTextCmdInsert() {}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		*delta = insert(editor, getPosition(), mWString );
		LLWStringUtil::truncate(mWString, *delta);
		return (*delta != 0);
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		remove(editor, getPosition(), mWString.length() );
		return getPosition();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}
private:
	LLWString mWString;
};
class LLTextEditor::LLTextCmdAddChar : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdAddChar( S32 pos, BOOL group_with_next, llwchar wc)
		: LLTextCmd(pos, group_with_next), mWString(1, wc), mBlockExtensions(FALSE)
	{
	}
	virtual void blockExtensions()
	{
		mBlockExtensions = TRUE;
	}
	virtual BOOL canExtend(S32 pos) const
	{
		return !mBlockExtensions && (pos == getPosition() + (S32)mWString.length());
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		*delta = insert(editor, getPosition(), mWString);
		LLWStringUtil::truncate(mWString, *delta);
		return (*delta != 0);
	}
	virtual BOOL extendAndExecute( LLTextEditor* editor, S32 pos, llwchar wc, S32* delta )
	{
		LLWString ws;
		ws += wc;
		*delta = insert(editor, pos, ws);
		if( *delta > 0 )
		{
			mWString += wc;
		}
		return (*delta != 0);
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		remove(editor, getPosition(), mWString.length() );
		return getPosition();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}
private:
	LLWString	mWString;
	BOOL		mBlockExtensions;
};
class LLTextEditor::LLTextCmdOverwriteChar : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdOverwriteChar( S32 pos, BOOL group_with_next, llwchar wc)
		: LLTextCmd(pos, group_with_next), mChar(wc), mOldChar(0) {}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		mOldChar = editor->getWChar(getPosition());
		overwrite(editor, getPosition(), mChar);
		*delta = 0;
		return TRUE;
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		overwrite(editor, getPosition(), mOldChar);
		return getPosition();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		overwrite(editor, getPosition(), mChar);
		return getPosition()+1;
	}
private:
	llwchar		mChar;
	llwchar		mOldChar;
};
class LLTextEditor::LLTextCmdRemove : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdRemove( S32 pos, BOOL group_with_next, S32 len ) :
		LLTextCmd(pos, group_with_next), mLen(len)
	{
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		mWString = editor->getWSubString(getPosition(), mLen);
		*delta = remove(editor, getPosition(), mLen );
		return (*delta != 0);
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		remove(editor, getPosition(), mLen );
		return getPosition();
	}
private:
	LLWString	mWString;
	S32				mLen;
};
LLTextEditor::LLTextEditor(
	const std::string& name,
	const LLRect& rect,
	S32 max_length,
	const std::string &default_text,
	const LLFontGL* font,
	BOOL allow_embedded_items,
	bool parse_html)
	:
	LLUICtrl(name, rect, TRUE, NULL, FOLLOWS_TOP | FOLLOWS_LEFT),
	mTextIsUpToDate(TRUE),
	mMaxTextByteLength(max_length),
	mPopupMenuHandle(),
	mBaseDocIsPristine(TRUE),
	mPristineCmd(NULL),
	mLastCmd(NULL),
	mCursorPos(0),
	mIsSelecting(FALSE),
	mSelectionStart(0),
	mSelectionEnd(0),
	mScrolledToBottom(TRUE),
	mOnScrollEndCallback(NULL),
	mOnScrollEndData(NULL),
	mCursorColor(LLUI::sColorsGroup->getColor("TextCursorColor")),
	mFgColor(LLUI::sColorsGroup->getColor("TextFgColor")),
	mDefaultColor(LLUI::sColorsGroup->getColor("TextDefaultColor")),
	mReadOnlyFgColor(LLUI::sColorsGroup->getColor("TextFgReadOnlyColor")),
	mWriteableBgColor(LLUI::sColorsGroup->getColor("TextBgWriteableColor")),
	mReadOnlyBgColor(LLUI::sColorsGroup->getColor("TextBgReadOnlyColor")),
	mFocusBgColor(LLUI::sColorsGroup->getColor("TextBgFocusColor")),
	mLinkColor(nullptr),
	mReadOnly(parse_html),
	mWordWrap(FALSE),
	mShowLineNumbers(FALSE),
	mTabsToNextField(TRUE),
	mCommitOnFocusLost(FALSE),
	mHideScrollbarForShortDocs(FALSE),
	mTakesNonScrollClicks(TRUE),
	mTrackBottom(FALSE),
	mAllowEmbeddedItems(allow_embedded_items),
	mAcceptCallingCardNames(FALSE),
	mHandleEditKeysDirectly(FALSE),
	mMouseDownX(0),
	mMouseDownY(0),
	mLastSelectionX(-1),
	mLastSelectionY(-1),
	mParseHTML(parse_html),
	mParseHighlights(false),
	mLastContextMenuX(-1),
	mLastContextMenuY(-1),
	mReflowNeeded(FALSE),
	mScrollNeeded(FALSE),
	mSpellCheckable(FALSE)
{
	mSourceID.generate();
	mDesiredXPixel = -1;
	if (font)
	{
		mGLFont = font;
	}
	else
	{
		mGLFont = LLFontGL::getFontSansSerif();
	}
	updateTextRect();
	S32 line_height = ll_round(mGLFont->getLineHeight());
	S32 page_size = mTextRect.getHeight() / line_height;
	LLRect scroll_rect;
	scroll_rect.setOriginAndSize(
		getRect().getWidth() - SCROLLBAR_SIZE,
		1,
		SCROLLBAR_SIZE,
		getRect().getHeight() - 1);
	S32 lines_in_doc = getLineCount();
	mScrollbar = new LLScrollbar(std::string("Scrollbar"), scroll_rect,
		LLScrollbar::VERTICAL,
		lines_in_doc,
		0,
		page_size,
		NULL);
	mScrollbar->setFollowsRight();
	mScrollbar->setFollowsTop();
	mScrollbar->setFollowsBottom();
	mScrollbar->setEnabled(TRUE);
	mScrollbar->setVisible(TRUE);
	mScrollbar->setOnScrollEndCallback(mOnScrollEndCallback, mOnScrollEndData);
	addChild(mScrollbar);
	mBorder = new LLViewBorder(std::string("text ed border"), LLRect(0, getRect().getHeight(), getRect().getWidth(), 0), LLViewBorder::BEVEL_IN, LLViewBorder::STYLE_LINE, UI_TEXTEDITOR_BORDER);
	addChild(mBorder);
	appendText(default_text, FALSE, FALSE);
	resetDirty();
	setCommitCallback(boost::bind(&LLTextEditor::setControlValue, this, _2));
}
LLTextEditor::~LLTextEditor()
{
	gFocusMgr.releaseFocusIfNeeded( this );
	if (mLinkColor)
	{
		delete mLinkColor;
		mLinkColor = nullptr;
	}
	std::for_each(mUndoStack.begin(), mUndoStack.end(), DeletePointer());
	mSegments.clear();
	auto menu = mPopupMenuHandle.get();
	if (menu)
	{
		menu->die();
		mPopupMenuHandle.markDead();
	}
}
const std::string& LLTextEditor::getMenuSegmentUrl() const
{
	auto segment = getSegmentAtLocalPos(mLastContextMenuX, mLastContextMenuY);
	auto style = segment ? segment->getStyle() : nullptr;
	return style ? style->getLinkHREF() : LLStringUtil::null;
}
static LFIDBearer::Type get_type_from_url(const std::string& url)
{
	auto pos = url.find("/app/");
	if (pos != std::string::npos && pos + 10 <= url.size())
	{
		auto type = url.substr(pos + 5, 5);
		return type == "agent" ? LFIDBearer::AVATAR
			: type == "group" ? LFIDBearer::GROUP
			: type == "experience" ? LFIDBearer::EXPERIENCE
			: LFIDBearer::OBJECT;
	}
	return LFIDBearer::NONE;
}
LLUUID LLTextEditor::getStringUUIDSelectedItem() const
{
	const auto& url = getMenuSegmentUrl();
	const auto& type = get_type_from_url(url);
	return type == LFIDBearer::NONE ? LLUUID::null : LLUUID(type == OBJECT ? LLUrlAction::getObjectId(url) : LLUrlAction::getUserID(url));
}
LFIDBearer::Type LLTextEditor::getSelectedType() const
{
	return get_type_from_url(getMenuSegmentUrl());
}
class CopyRawText : public LLMemberListener<LLView>
{
	bool handleEvent(LLPointer<LLOldEvents::LLEvent>, const LLSD& userdata) override
	{
		LFIDBearer::getActive<LLTextEditor>()->copyRaw();
		return true;
	}
};
class TextEditorVisible : public LLMemberListener<LLView>
{
	bool handleEvent(LLPointer<LLOldEvents::LLEvent>, const LLSD& userdata) override
	{
		LLMenuGL::sMenuContainer->findControl(userdata["control"].asString())->setValue(!!dynamic_cast<LLTextEditor*>(gFocusMgr.getKeyboardFocus()));
		return true;
	}
};
class ContextUrl : public LLMemberListener<LLView>
{
	bool handleEvent(LLPointer<LLOldEvents::LLEvent>, const LLSD& userdata) override
	{
		const auto& url = LFIDBearer::getActive<LLTextEditor>()->getMenuSegmentUrl();
		const auto& op = userdata.asStringRef();
		if (op == "Open") LLUrlAction::openURL(url);
		else if (op == "OpenInternal") LLUrlAction::openURLInternal(url);
		else if (op == "OpenExternal") LLUrlAction::openURLExternal(url);
		else if (op == "Execute") LLUrlAction::executeSLURL(url, true);
		else if (op == "Block") LLUrlAction::blockObject(url);
		else if (op == "Unblock") LLUrlAction::unblockObject(url);
		else if (op == "Teleport") LLUrlAction::teleportToLocation(url);
		else if (op == "ShowOnMap") LLUrlAction::showLocationOnMap(url);
		else if (op == "CopyLabel") LLUrlAction::copyLabelToClipboard(url);
		else if (op == "CopyUrl") LLUrlAction::copyURLToClipboard(url);
		return true;
	}
};
class ContextUrlCopy : public LLMemberListener<LLView>
{
	bool handleEvent(LLPointer<LLOldEvents::LLEvent>, const LLSD& userdata) override
	{
		LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(LFIDBearer::getActiveSelectedID().asString()));
		return true;
	}
};
void LLTextEditor::spell_correct(void* data)
{
	auto self = static_cast<LLTextEditor*>(data);
	S32 wordStart = 0, wordLen = 0;
	if (self->getWordBoundriesAt(self->getCursorPosFromLocalCoord(self->mLastContextMenuX, self->mLastContextMenuY, TRUE), &wordStart, &wordLen))
	{
		const auto& word = utf8str_to_wstring(LLMenuGL::sMenuContainer->getActivatedItem()->getLabel());
		self->remove(wordStart, wordLen, TRUE);
		self->insert(wordStart, word, FALSE);
		self->mCursorPos += word.length() - wordLen;
		self->needsReflow();
	}
}
void LLTextEditor::spell_show(void* show)
{
	glggHunSpell->setSpellCheckHighlight(!!show);
}
std::vector<S32> LLTextEditor::getMisspelledWordsPositions()
{
	resetSpellDirty();
	std::vector<S32> thePosesOfBadWords;
	LLWString& text = mWText;
	S32 wordStart=0;
	S32 wordEnd=spellStart;
	while(wordEnd < spellEnd)
	{
		if( LLTextEditor::isPartOfWord( text[wordEnd] ) )
		{
			while ((wordEnd > 0) && LLTextEditor::isPartOfWord(text[wordEnd-1]))
			{
				wordEnd--;
			}
			wordStart=wordEnd;
			while ((wordEnd < (S32)text.length()) && LLTextEditor::isPartOfWord( text[wordEnd] ) )
			{
				wordEnd++;
			}
			std::string regText(text.begin(),text.end());
			std::string selectedWord(regText.substr(wordStart,wordEnd-wordStart));
			if(!glggHunSpell->isSpelledRight(selectedWord))
			{
				thePosesOfBadWords.push_back(wordStart);
				thePosesOfBadWords.push_back(wordEnd);
			}
		}
		wordEnd++;
	}
	return thePosesOfBadWords;
}
void LLTextEditor::spell_add(void* data)
{
	auto self = static_cast<LLTextEditor*>(data);
	S32 wordStart = 0, wordLen = 0;
	if (self->getWordBoundriesAt(self->getCursorPosFromLocalCoord(self->mLastContextMenuX, self->mLastContextMenuY, TRUE), &wordStart, &wordLen))
	{
		glggHunSpell->addWordToCustomDictionary(wstring_to_utf8str(self->getWText().substr(wordStart, wordLen)));
		self->mPrevSpelledText.erase();
	}
}
void LLTextEditor::addMenuListeners()
{
	(new CopyRawText)->registerListener(LLMenuGL::sMenuContainer, "CopyRawText");
	(new TextEditorVisible)->registerListener(LLMenuGL::sMenuContainer, "TextEditorVisible");
	(new ContextUrl)->registerListener(LLMenuGL::sMenuContainer, "Text.Url");
	(new ContextUrlCopy)->registerListener(LLMenuGL::sMenuContainer, "Text.Url.CopyUUID");
}
void LLTextEditor::setTrackColor( const LLColor4& color )
{
	mScrollbar->setTrackColor(color);
}
void LLTextEditor::setThumbColor( const LLColor4& color )
{
	mScrollbar->setThumbColor(color);
}
void LLTextEditor::setHighlightColor( const LLColor4& color )
{
	mScrollbar->setHighlightColor(color);
}
void LLTextEditor::setShadowColor( const LLColor4& color )
{
	mScrollbar->setShadowColor(color);
}
void LLTextEditor::updateLineStartList(S32 startpos)
{
	updateSegments();
	bindEmbeddedChars(mGLFont);
	S32 seg_num = mSegments.size();
	S32 seg_idx = 0;
	S32 seg_offset = 0;
	if (!mLineStartList.empty())
	{
		getSegmentAndOffset(startpos, &seg_idx, &seg_offset);
		line_info t(seg_idx, seg_offset);
		line_list_t::iterator iter = std::upper_bound(mLineStartList.begin(), mLineStartList.end(), t, line_info_compare());
		if (iter != mLineStartList.begin()) --iter;
		seg_idx = iter->mSegment;
		seg_offset = iter->mOffset;
		mLineStartList.erase(iter, mLineStartList.end());
	}
	while( seg_idx < seg_num )
	{
		mLineStartList.push_back(line_info(seg_idx,seg_offset));
		BOOL line_ended = FALSE;
		S32 start_x = mShowLineNumbers ? UI_TEXTEDITOR_LINE_NUMBER_MARGIN : 0;
		S32 line_width = start_x;
		while(!line_ended && seg_idx < seg_num)
		{
			LLTextSegment* segment = mSegments[seg_idx];
			S32 start_idx = segment->getStart() + seg_offset;
			S32 end_idx = start_idx;
			while (end_idx < segment->getEnd() && mWText[end_idx] != '\n')
			{
				end_idx++;
			}
			if (start_idx == end_idx)
			{
				if (end_idx >= segment->getEnd())
				{
					seg_idx++;
					seg_offset = 0;
				}
				else
				{
					line_ended = TRUE;
					seg_offset++;
				}
			}
			else
			{
				static LLWString buf;
				if(start_idx)
				{
					buf.resize(end_idx - start_idx);
					std::copy(mWText.begin() + start_idx, mWText.begin() + end_idx, buf.begin());
				}
				const LLWString& str = start_idx ? buf : mWText;
				S32 drawn = mGLFont->maxDrawableChars(str, (F32)abs(mTextRect.getWidth()) - line_width,
													  end_idx - start_idx, mWordWrap ? LLFontGL::WORD_BOUNDARY_IF_POSSIBLE : LLFontGL::ANYWHERE, mAllowEmbeddedItems );
				if( 0 == drawn && line_width == start_x)
				{
					drawn = 1;
				}
				seg_offset += drawn;
				line_width += mGLFont->getWidth(str, 0, drawn, mAllowEmbeddedItems);
				end_idx = segment->getStart() + seg_offset;
				if (end_idx < segment->getEnd())
				{
					line_ended = TRUE;
					if (mWText[end_idx] == '\n')
					{
						seg_offset++;
					}
				}
				else
				{
					seg_idx++;
					seg_offset = 0;
				}
			}
		}
	}
	unbindEmbeddedChars(mGLFont);
	mScrollbar->setDocSize( getLineCount() );
	if (mHideScrollbarForShortDocs)
	{
		BOOL short_doc = (mScrollbar->getDocSize() <= mScrollbar->getPageSize());
		mScrollbar->setVisible(!short_doc);
	}
	if (mScrolledToBottom && mTrackBottom && !hasMouseCapture())
	{
		endOfDoc();
	}
}
BOOL LLTextEditor::truncate()
{
	BOOL did_truncate = FALSE;
	if (mWText.size() >= (size_t) (mMaxTextByteLength / 4))
	{
		S32 utf8_byte_size = wstring_utf8_length( mWText );
		if ( utf8_byte_size > mMaxTextByteLength )
		{
			std::string temp_utf8_text = wstring_to_utf8str( mWText );
			temp_utf8_text = utf8str_truncate( temp_utf8_text, mMaxTextByteLength );
			mWText = utf8str_to_wstring( temp_utf8_text );
			mTextIsUpToDate = FALSE;
			did_truncate = TRUE;
		}
	}
	return did_truncate;
}
LLMenuGL* LLTextEditor::createUrlContextMenu(S32 x, S32 y, const std::string &in_url)
{
	LLUrlMatch match;
	std::string url = in_url;
	if (!LLUrlRegistry::instance().findUrl(url, match))
	{
		return nullptr;
	}
	std::string xui_file = match.getMenuName();
	if (xui_file.empty())
	{
		return nullptr;
	}
	llassert(LLMenuGL::sMenuContainer != NULL);
	auto menu = LLUICtrlFactory::instance().buildMenu(xui_file, LLMenuGL::sMenuContainer);
	return menu;
}
void LLTextEditor::setText(const LLStringExplicit &utf8str, bool force_replace_links)
{
	mWText.clear();
	clearSegments();
	deselect();
	std::string text(utf8str);
	appendText(utf8str, false, false, nullptr, force_replace_links);
	setCursorPos(0);
	resetDirty();
	mUTF8Text = text;
	mTextIsUpToDate = true;
}
void LLTextEditor::setWText(const LLWString& text, bool force_replace_links)
{
	setText(wstring_to_utf8str(text), force_replace_links);
}
void LLTextEditor::setValue(const LLSD& value)
{
	setText(value.asString());
}
const std::string& LLTextEditor::getText() const
{
	if (!mTextIsUpToDate)
	{
		if (mAllowEmbeddedItems)
		{
			LL_WARNS() << "getText() called on text with embedded items (not supported)" << LL_ENDL;
		}
		mUTF8Text = wstring_to_utf8str(mWText);
		mTextIsUpToDate = TRUE;
	}
	return mUTF8Text;
}
LLSD LLTextEditor::getValue() const
{
	return LLSD(getText());
}
void LLTextEditor::setWordWrap(BOOL b)
{
	mWordWrap = b;
	setCursorPos(0);
	deselect();
	needsReflow();
}
void LLTextEditor::setBorderVisible(BOOL b)
{
	mBorder->setVisible(b);
}
BOOL LLTextEditor::isBorderVisible() const
{
	return mBorder->getVisible();
}
void LLTextEditor::setHideScrollbarForShortDocs(BOOL b)
{
	mHideScrollbarForShortDocs = b;
	if (mHideScrollbarForShortDocs)
	{
		BOOL short_doc = (mScrollbar->getDocSize() <= mScrollbar->getPageSize());
		mScrollbar->setVisible(!short_doc);
	}
}
void LLTextEditor::selectNext(const std::string& search_text_in, BOOL case_insensitive, BOOL wrap)
{
	if (search_text_in.empty())
	{
		return;
	}
	LLWString text = getWText();
	LLWString search_text = utf8str_to_wstring(search_text_in);
	if (case_insensitive)
	{
		LLWStringUtil::toLower(text);
		LLWStringUtil::toLower(search_text);
	}
	if (mIsSelecting)
	{
		LLWString selected_text = text.substr(mSelectionEnd, mSelectionStart - mSelectionEnd);
		if (selected_text == search_text)
		{
			mCursorPos += search_text.size();
		}
	}
	S32 loc = text.find(search_text,mCursorPos);
	if (wrap && (-1 == loc))
	{
		loc = text.find(search_text);
	}
	if (-1 == loc)
	{
		mIsSelecting = FALSE;
		mSelectionEnd = 0;
		mSelectionStart = 0;
		return;
	}
	setCursorPos(loc);
	scrollToPos(mCursorPos);
	mIsSelecting = TRUE;
	mSelectionEnd = mCursorPos;
	mSelectionStart = llmin((S32)getLength(), (S32)(mCursorPos + search_text.size()));
}
BOOL LLTextEditor::replaceText(const std::string& search_text_in, const std::string& replace_text,
							   BOOL case_insensitive, BOOL wrap)
{
	BOOL replaced = FALSE;
	if (search_text_in.empty())
	{
		return replaced;
	}
	LLWString search_text = utf8str_to_wstring(search_text_in);
	if (mIsSelecting)
	{
		LLWString text = getWText();
		LLWString selected_text = text.substr(mSelectionEnd, mSelectionStart - mSelectionEnd);
		if (case_insensitive)
		{
			LLWStringUtil::toLower(selected_text);
			LLWStringUtil::toLower(search_text);
		}
		if (selected_text == search_text)
		{
			insertText(replace_text);
			replaced = TRUE;
		}
	}
	selectNext(search_text_in, case_insensitive, wrap);
	return replaced;
}
void LLTextEditor::replaceTextAll(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive)
{
	S32 cur_pos = mScrollbar->getDocPos();
	setCursorPos(0);
	selectNext(search_text, case_insensitive, FALSE);
	BOOL replaced = TRUE;
	while ( replaced )
	{
		replaced = replaceText(search_text,replace_text, case_insensitive, FALSE);
	}
	mScrollbar->setDocPos(cur_pos);
}
void LLTextEditor::setCursorAtLocalPos( S32 local_x, S32 local_y, BOOL round )
{
	setCursorPos(getCursorPosFromLocalCoord(local_x, local_y, round));
}
S32 LLTextEditor::prevWordPos(S32 cursorPos) const
{
	const LLWString& wtext = mWText;
	while( (cursorPos > 0) && (wtext[cursorPos-1] == ' ') )
	{
		cursorPos--;
	}
	while( (cursorPos > 0) && isPartOfWord( wtext[cursorPos-1] ) )
	{
		cursorPos--;
	}
	return cursorPos;
}
S32 LLTextEditor::nextWordPos(S32 cursorPos) const
{
	const LLWString& wtext = mWText;
	while( (cursorPos < getLength()) && isPartOfWord( wtext[cursorPos] ) )
	{
		cursorPos++;
	}
	while( (cursorPos < getLength()) && (wtext[cursorPos] == ' ') )
	{
		cursorPos++;
	}
	return cursorPos;
}
BOOL LLTextEditor::getWordBoundriesAt(const S32 at, S32* word_begin, S32* word_length) const
{
	S32 pos = at;
	if (isPartOfWord(mWText[pos]))
	{
		while ( (pos > 0) && isPartOfWord(mWText[pos - 1]) )
		{
			pos--;
		}
		*word_begin = pos;
		while ( (pos < getLength()) && isPartOfWord(mWText[pos]) )
		{
			pos++;
		}
		*word_length = pos - *word_begin;
		return TRUE;
	}
	return FALSE;
}
S32 LLTextEditor::getLineStart( S32 line ) const
{
	S32 num_lines = getLineCount();
	if (num_lines == 0)
	{
		return 0;
	}
	line = llclamp(line, 0, num_lines-1);
	S32 segidx = mLineStartList[line].mSegment;
	S32 segoffset = mLineStartList[line].mOffset;
	LLTextSegment* seg = mSegments[segidx];
	S32 res = seg->getStart() + segoffset;
	if (res > seg->getEnd())
	{
		LL_WARNS() << "BAD JOOJOO! Text length (" << res << ") greater than text end (" << seg->getEnd() << "). Setting line start to " << seg->getEnd() << LL_ENDL;
		res = seg->getEnd();
	}
	return res;
}
void LLTextEditor::getLineAndOffset( S32 startpos, S32* linep, S32* offsetp ) const
{
	if (mLineStartList.empty())
	{
		*linep = 0;
		*offsetp = startpos;
	}
	else
	{
		S32 seg_idx, seg_offset;
		getSegmentAndOffset( startpos, &seg_idx, &seg_offset );
		line_info tline(seg_idx, seg_offset);
		line_list_t::const_iterator iter = std::upper_bound(mLineStartList.begin(), mLineStartList.end(), tline, line_info_compare());
		if (iter != mLineStartList.begin()) --iter;
		*linep = iter - mLineStartList.begin();
		S32 line_start = mSegments[iter->mSegment]->getStart() + iter->mOffset;
		*offsetp = startpos - line_start;
	}
}
void LLTextEditor::getSegmentAndOffset( S32 startpos, S32* segidxp, S32* offsetp ) const
{
	if (mSegments.empty())
	{
		*segidxp = -1;
		*offsetp = startpos;
	}
	LLTextSegment tseg(startpos);
	segment_list_t::const_iterator seg_iter;
	seg_iter = std::upper_bound(mSegments.begin(), mSegments.end(), &tseg, LLTextSegment::compare());
	if (seg_iter != mSegments.begin()) --seg_iter;
	*segidxp = seg_iter - mSegments.begin();
	*offsetp = startpos - (*seg_iter)->getStart();
}
const LLTextSegment*	LLTextEditor::getPreviousSegment() const
{
	S32 idx = llmax(0, getSegmentIdxAtOffset(mCursorPos) - 1);
	return idx >= 0 ? mSegments[idx] : LLTextSegmentPtr();
}
void LLTextEditor::getSelectedSegments(std::vector<LLTextSegmentPtr>& segments) const
{
	S32 left = hasSelection() ? llmin(mSelectionStart, mSelectionEnd) : mCursorPos;
	S32 right = hasSelection() ? llmax(mSelectionStart, mSelectionEnd) : mCursorPos;
	S32 first_idx = llmax(0, getSegmentIdxAtOffset(left));
	S32 last_idx = llmax(0, first_idx, getSegmentIdxAtOffset(right));
	for (S32 idx = first_idx; idx <= last_idx; ++idx)
	{
		segments.push_back(mSegments[idx]);
	}
}
S32 LLTextEditor::getCursorPosFromLocalCoord( S32 local_x, S32 local_y, BOOL round ) const
{
	if(mShowLineNumbers)
	{
		local_x -= UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
	}
	S32 total_lines = getLineCount();
	S32 line_height = ll_round( mGLFont->getLineHeight() );
	S32 max_visible_lines = mTextRect.getHeight() / line_height;
	S32 scroll_lines = mScrollbar->getDocPos();
	S32 visible_lines = llmin( total_lines - scroll_lines, max_visible_lines );
	S32 line = (mTextRect.mTop - 1 - local_y) / line_height;
	if (line >= total_lines)
	{
		return getLength();
	}
	line = llclamp( line, 0, visible_lines ) + scroll_lines;
	S32 line_start = getLineStart(line);
	S32 next_start = getLineStart(line+1);
	S32	line_end = (next_start != line_start) ? next_start - 1 : getLength();
	if(line_start == -1)
	{
		return 0;
	}
	else
	{
		S32 line_len = line_end - line_start;
		S32 pos;
		if (mAllowEmbeddedItems)
		{
			bindEmbeddedChars(mGLFont);
			pos = mGLFont->charFromPixelOffset(mWText.c_str(), line_start,
											   (F32)(local_x - mTextRect.mLeft),
											   (F32)(mTextRect.getWidth()),
											   line_len,
											   round, TRUE);
			unbindEmbeddedChars(mGLFont);
		}
		else
		{
			pos = mGLFont->charFromPixelOffset(mWText.c_str(), line_start,
											   (F32)(local_x - mTextRect.mLeft),
											   (F32)mTextRect.getWidth(),
											   line_len,
											   round);
		}
		return line_start + pos;
	}
}
void LLTextEditor::setCursor(S32 row, S32 column)
{
	const llwchar* doc = mWText.c_str();
	const char CR = 10;
	if(row < 0) row = 0;
	if(column < 0) column = 0;
	while(row--)
	{
		while (CR != *doc++);
	}
	doc += column;
	setCursorPos(doc - mWText.c_str());
}
void LLTextEditor::setCursorPos(S32 offset)
{
	mCursorPos = llclamp(offset, 0, (S32)getLength());
	needsScroll();
	mDesiredXPixel = -1;
}
BOOL LLTextEditor::canDeselect() const
{
	return hasSelection();
}
void LLTextEditor::deselect()
{
	mSelectionStart = 0;
	mSelectionEnd = 0;
	mIsSelecting = FALSE;
}
void LLTextEditor::startSelection()
{
	if( !mIsSelecting )
	{
		mIsSelecting = TRUE;
		mSelectionStart = mCursorPos;
		mSelectionEnd = mCursorPos;
	}
}
void LLTextEditor::endSelection()
{
	if( mIsSelecting )
	{
		mIsSelecting = FALSE;
		mSelectionEnd = mCursorPos;
	}
}
BOOL LLTextEditor::selectionContainsLineBreaks()
{
	if (hasSelection())
	{
		S32 left = llmin(mSelectionStart, mSelectionEnd);
		S32 right = left + llabs(mSelectionStart - mSelectionEnd);
		const LLWString &wtext = mWText;
		for( S32 i = left; i < right; i++ )
		{
			if (wtext[i] == '\n')
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}
S32 LLTextEditor::indentLine( S32 pos, S32 spaces )
{
	llassert(pos >= 0);
	llassert(pos <= getLength() );
	S32 delta_spaces = 0;
	if (spaces >= 0)
	{
		for(S32 i=0; i < spaces; i++)
		{
			delta_spaces += addChar(pos, ' ');
		}
	}
	else
	{
		for(S32 i=0; i < -spaces; i++)
		{
			const LLWString &wtext = mWText;
			if (wtext[pos] == ' ')
			{
				delta_spaces += remove( pos, 1, FALSE );
			}
 		}
	}
	return delta_spaces;
}
void LLTextEditor::indentSelectedLines( S32 spaces )
{
	if( hasSelection() )
	{
		const LLWString &text = mWText;
		S32 left = llmin( mSelectionStart, mSelectionEnd );
		S32 right = left + llabs( mSelectionStart - mSelectionEnd );
		BOOL cursor_on_right = (mSelectionEnd > mSelectionStart);
		S32 cur = left;
		while( (cur > 0) && (text[cur] != '\n') )
		{
			cur--;
		}
		left = cur;
		if( cur > 0 )
		{
			left++;
		}
		if( text[right - 1] == '\n' )
		{
			right--;
		}
		else
		{
			while( (text[right] != '\n') && (right <= getLength() ) )
			{
				right++;
			}
		}
		do
		{
			if( text[cur] == '\n' )
			{
				cur++;
			}
			S32 delta_spaces = indentLine( cur, spaces );
			if( delta_spaces > 0 )
			{
				cur += delta_spaces;
			}
			right += delta_spaces;
			while( (cur < right) && (text[cur] != '\n') )
			{
				cur++;
			}
		}
		while( cur < right );
		if( (right < getLength()) && (text[right] == '\n') )
		{
			right++;
		}
		if( cursor_on_right )
		{
			mSelectionStart = left;
			mSelectionEnd = right;
		}
		else
		{
			mSelectionStart = right;
			mSelectionEnd = left;
		}
		mCursorPos = mSelectionEnd;
	}
}
BOOL LLTextEditor::canSelectAll() const
{
	return TRUE;
}
void LLTextEditor::selectAll()
{
	mSelectionStart = getLength();
	mSelectionEnd = 0;
	mCursorPos = mSelectionEnd;
}
BOOL LLTextEditor::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
{
	for ( child_list_const_iter_t child_it = getChildList()->begin();
		  child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if( viewp->handleToolTip(local_x, local_y, msg, sticky_rect_screen ) )
		{
			return TRUE;
		}
	}
	if( mSegments.empty() )
	{
		return TRUE;
	}
	const LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
	if( cur_segment )
	{
		BOOL has_tool_tip = FALSE;
		has_tool_tip = cur_segment->getToolTip( msg );
		if( has_tool_tip )
		{
			S32 SLOP = 8;
			localPointToScreen(
				x - SLOP, y - SLOP,
				&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
			sticky_rect_screen->mRight = sticky_rect_screen->mLeft + 2 * SLOP;
			sticky_rect_screen->mTop = sticky_rect_screen->mBottom + 2 * SLOP;
		}
	}
	return TRUE;
}
BOOL LLTextEditor::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	mScrollbar->handleScrollWheel( 0, 0, clicks );
	return TRUE;
}
BOOL LLTextEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->isOpen())
	{
		LLMenuGL::sMenuContainer->hideMenus();
	}
	handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;
	if( !handled && mTakesNonScrollClicks)
	{
		if (!(mask & MASK_SHIFT))
		{
			deselect();
		}
		BOOL start_select = TRUE;
		if( start_select )
		{
			if (mask & MASK_SHIFT)
			{
				S32 old_cursor_pos = mCursorPos;
				setCursorAtLocalPos( x, y, TRUE );
				if (hasSelection())
				{
					mSelectionEnd = mCursorPos;
				}
				else
				{
					mSelectionStart = old_cursor_pos;
					mSelectionEnd = mCursorPos;
				}
				mIsSelecting = TRUE;
			}
			else
			{
				setCursorAtLocalPos( x, y, TRUE );
				startSelection();
			}
			gFocusMgr.setMouseCapture( this );
		}
		handled = TRUE;
	}
	if (hasTabStop())
	{
		setFocus( TRUE );
		handled = TRUE;
	}
	resetKeystrokeTimer();
	return handled;
}
BOOL LLTextEditor::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	setFocus(TRUE);
	auto segment = getSegmentAtLocalPos(x, y);
	const LLStyleSP style = segment ? segment->getStyle() : nullptr;
	auto submenu = (style && style->isLink()) ? createUrlContextMenu(x, y, style->getLinkHREF()) : nullptr;
	auto menu = (submenu && !hasSelection()) ? submenu : LLUICtrlFactory::instance().buildMenu("menu_texteditor.xml", LLMenuGL::sMenuContainer);
	mPopupMenuHandle = menu->getHandle();
	if (menu)
	{
		if (submenu && submenu != menu)
		{
			submenu->removeChild(submenu->getChild<LLMenuItemCallGL>("Select All"));
			menu->appendMenu(submenu);
		}
		if (!mReadOnly && mSpellCheckable)
		{
			S32 wordStart = 0;
			S32 wordLen = 0;
			S32 pos = getCursorPosFromLocalCoord(x, y, TRUE);
			if (getWordBoundriesAt(pos, &wordStart, &wordLen))
			{
				const auto selectedWord = wstring_to_utf8str(getWText().substr(wordStart, wordLen));
				if (!glggHunSpell->isSpelledRight(selectedWord))
				{
					menu->addSeparator();
					for (const auto& word : glggHunSpell->getSuggestionList(selectedWord))
					{
						menu->addChild(new LLMenuItemCallGL(word, spell_correct, nullptr, this));
					}
					menu->addChild(new LLMenuItemCallGL("Add Word", spell_add, nullptr, this));
				}
			}
			bool show = !glggHunSpell->getSpellCheckHighlight();
			auto word = show ? "Show Misspellings" : "Hide Misspellings";
			menu->addChild(new LLMenuItemCallGL(word, spell_show, nullptr, show ? &show : nullptr));
		}
		mLastContextMenuX = x;
		mLastContextMenuY = y;
		showMenu(this, menu, x, y);
	}
	return TRUE;
}
BOOL LLTextEditor::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	setFocus( TRUE );
	if( canPastePrimary() )
	{
		setCursorAtLocalPos( x, y, TRUE );
		pastePrimary();
	}
	return TRUE;
}
bool always_underline_links()
{
	static const LLUICachedControl<bool> always_underline("SinguAlwaysUnderlineLinks");
	return always_underline;
}
BOOL LLTextEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	auto old_hover = mHoverSegment;
	mHoverSegment = NULL;
	if(hasMouseCapture() )
	{
		if( mIsSelecting )
		{
			if (x != mLastSelectionX || y != mLastSelectionY)
			{
				mLastSelectionX = x;
				mLastSelectionY = y;
			}
			if( y > mTextRect.mTop )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() - 1 );
			}
			else
			if( y < mTextRect.mBottom )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() + 1 );
			}
			setCursorAtLocalPos( x, y, TRUE );
			mSelectionEnd = mCursorPos;
		}
		LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (active)" << LL_ENDL;
		getWindow()->setCursor(UI_CURSOR_IBEAM);
		handled = TRUE;
	}
	if( !handled )
	{
		handled = LLView::childrenHandleHover(x, y, mask) != NULL;
	}
	if( handled )
	{
		resetKeystrokeTimer();
	}
	if( !handled && mTakesNonScrollClicks)
	{
		if( !mSegments.empty() )
		{
			LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
			if( cur_segment )
			{
				if(cur_segment->getStyle()->isLink())
				{
					LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (over link, inactive)" << LL_ENDL;
					getWindow()->setCursor(UI_CURSOR_HAND);
					handled = TRUE;
				}
				else
				if(cur_segment->getStyle()->getIsEmbeddedItem())
				{
					LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (over embedded item, inactive)" << LL_ENDL;
					getWindow()->setCursor(UI_CURSOR_HAND);
					handled = TRUE;
				}
				mHoverSegment = cur_segment;
			}
		}
		if( !handled )
		{
			LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (inactive)" << LL_ENDL;
			if (!mScrollbar->getVisible() || x < getRect().getWidth() - SCROLLBAR_SIZE)
			{
				getWindow()->setCursor(UI_CURSOR_IBEAM);
			}
			else
			{
				getWindow()->setCursor(UI_CURSOR_ARROW);
			}
			handled = TRUE;
		}
	}
	if (old_hover != mHoverSegment)
	{
		if (old_hover && !always_underline_links())
			old_hover->underlineOnHover(false);
		if (mHoverSegment)
			mHoverSegment->underlineOnHover(true);
	}
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
	return handled;
}
void LLTextEditor::onMouseLeave(S32 x, S32 y, MASK mask)
{
	if (mHoverSegment && !always_underline_links())
	{
		mHoverSegment->underlineOnHover(false);
		mHoverSegment = nullptr;
	}
}
BOOL LLTextEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;
	handled = LLView::childrenHandleMouseUp(x, y, mask) != NULL;
	if( !handled && mTakesNonScrollClicks)
	{
		if( mIsSelecting )
		{
			if( y > mTextRect.mTop )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() - 1 );
			}
			else
			if( y < mTextRect.mBottom )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() + 1 );
			}
			setCursorAtLocalPos( x, y, TRUE );
			endSelection();
		}
		if( !hasSelection() )
		{
			handleMouseUpOverSegment( x, y, mask );
		}
		updatePrimary();
		handled = TRUE;
	}
	resetKeystrokeTimer();
	if( hasMouseCapture()  )
	{
		gFocusMgr.setMouseCapture( NULL );
		handled = TRUE;
	}
	return handled;
}
BOOL LLTextEditor::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;
	handled = LLView::childrenHandleDoubleClick(x, y, mask) != NULL;
	if( !handled && mTakesNonScrollClicks)
	{
		setCursorAtLocalPos( x, y, FALSE );
		deselect();
		const LLWString &text = mWText;
		if( isPartOfWord( text[mCursorPos] ) )
		{
			while ((mCursorPos > 0) && isPartOfWord(text[mCursorPos-1]))
			{
				mCursorPos--;
			}
			startSelection();
			while ((mCursorPos < (S32)text.length()) && isPartOfWord( text[mCursorPos] ) )
			{
				mCursorPos++;
			}
			mSelectionEnd = mCursorPos;
		}
		else if ((mCursorPos < (S32)text.length()) && !iswspace( text[mCursorPos]) )
		{
			startSelection();
			mCursorPos++;
			mSelectionEnd = mCursorPos;
		}
		mIsSelecting = FALSE;
		resetKeystrokeTimer();
		updatePrimary();
		handled = TRUE;
	}
	return handled;
}
BOOL LLTextEditor::handleDragAndDrop(S32 x, S32 y, MASK mask,
					  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
					  EAcceptance *accept,
					  std::string& tooltip_msg)
{
	*accept = ACCEPT_NO;
	return TRUE;
}
S32 LLTextEditor::execute( LLTextCmd* cmd )
{
	S32 delta = 0;
	if( cmd->execute(this, &delta) )
	{
		undo_stack_t::iterator enditer = std::find(mUndoStack.begin(), mUndoStack.end(), mLastCmd);
		if (enditer != mUndoStack.begin())
		{
			--enditer;
			std::for_each(mUndoStack.begin(), enditer, DeletePointer());
			mUndoStack.erase(mUndoStack.begin(), enditer);
		}
		mUndoStack.push_front(cmd);
		mLastCmd = cmd;
	}
	else
	{
		delete cmd;
	}
	return delta;
}
S32 LLTextEditor::insert(const S32 pos, const LLWString &wstr, const BOOL group_with_next_op)
{
	return execute( new LLTextCmdInsert( pos, group_with_next_op, wstr ) );
}
S32 LLTextEditor::remove(const S32 pos, const S32 length, const BOOL group_with_next_op)
{
	return execute( new LLTextCmdRemove( pos, group_with_next_op, length ) );
}
S32 LLTextEditor::overwriteChar(S32 pos, llwchar wc)
{
	if ((S32)mWText.length() == pos)
	{
		return addChar(pos, wc);
	}
	else
	{
		return execute(new LLTextCmdOverwriteChar(pos, FALSE, wc));
	}
}
void LLTextEditor::removeCharOrTab()
{
	if( !getEnabled() )
	{
		return;
	}
	if( mCursorPos > 0 )
	{
		S32 chars_to_remove = 1;
		const LLWString &text = mWText;
		if (text[mCursorPos - 1] == ' ')
		{
			S32 line, offset;
			getLineAndOffset(mCursorPos, &line, &offset);
			if (offset > 0)
			{
				chars_to_remove = offset % SPACES_PER_TAB;
				if( chars_to_remove == 0 )
				{
					chars_to_remove = SPACES_PER_TAB;
				}
				for( S32 i = 0; i < chars_to_remove; i++ )
				{
					if (text[ mCursorPos - i - 1] != ' ')
					{
						chars_to_remove = 1;
						break;
					}
				}
			}
		}
		for (S32 i = 0; i < chars_to_remove; i++)
		{
			setCursorPos(mCursorPos - 1);
			remove( mCursorPos, 1, FALSE );
		}
	}
	else
	{
		reportBadKeystroke();
	}
}
S32 LLTextEditor::removeChar(S32 pos)
{
	return remove( pos, 1, FALSE );
}
void LLTextEditor::removeChar()
{
	if (!getEnabled())
	{
		return;
	}
	if (mCursorPos > 0)
	{
		setCursorPos(mCursorPos - 1);
		removeChar(mCursorPos);
	}
	else
	{
		reportBadKeystroke();
	}
}
void LLTextEditor::removeWord(bool prev)
{
	const S32& pos(mCursorPos);
	if (prev ? pos > 0 : pos < getLength())
	{
		S32 new_pos(prev ? prevWordPos(pos) : nextWordPos(pos));
		if (new_pos == pos)
			new_pos = prev ? prevWordPos(new_pos-1) : nextWordPos(new_pos+1);
		const U32 diff(labs(pos - new_pos));
		if (prev)
		{
			remove(new_pos, diff, false);
			setCursorPos(new_pos);
		}
		else
		{
			remove(pos, diff, false);
		}
	}
	else
	{
		reportBadKeystroke();
	}
}
S32 LLTextEditor::addChar(S32 pos, llwchar wc)
{
	if ( (wstring_utf8_length( mWText ) + wchar_utf8_length( wc ))  >= mMaxTextByteLength)
	{
		make_ui_sound("UISndBadKeystroke");
		return 0;
	}
	if (mLastCmd && mLastCmd->canExtend(pos))
	{
		S32 delta = 0;
		mLastCmd->extendAndExecute(this, pos, wc, &delta);
		return delta;
	}
	else
	{
		return execute(new LLTextCmdAddChar(pos, FALSE, wc));
	}
}
void LLTextEditor::addChar(llwchar wc)
{
	if( !getEnabled() )
	{
		return;
	}
	if( hasSelection() )
	{
		deleteSelection(TRUE);
	}
	else if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		removeChar(mCursorPos);
	}
	setCursorPos(mCursorPos + addChar( mCursorPos, wc ));
}
BOOL LLTextEditor::handleSelectionKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;
	if( mask & MASK_SHIFT )
	{
		handled = TRUE;
		switch( key )
		{
		case KEY_LEFT:
			if( 0 < mCursorPos )
			{
				startSelection();
				mCursorPos--;
				if( mask & MASK_CONTROL )
				{
					mCursorPos = prevWordPos(mCursorPos);
				}
				mSelectionEnd = mCursorPos;
			}
			break;
		case KEY_RIGHT:
			if( mCursorPos < getLength() )
			{
				startSelection();
				mCursorPos++;
				if( mask & MASK_CONTROL )
				{
					mCursorPos = nextWordPos(mCursorPos);
				}
				mSelectionEnd = mCursorPos;
			}
			break;
		case KEY_UP:
			startSelection();
			changeLine( -1 );
			mSelectionEnd = mCursorPos;
			break;
		case KEY_PAGE_UP:
			startSelection();
			changePage( -1 );
			mSelectionEnd = mCursorPos;
			break;
		case KEY_HOME:
			startSelection();
			if( mask & MASK_CONTROL )
			{
				mCursorPos = 0;
			}
			else
			{
				startOfLine();
			}
			mSelectionEnd = mCursorPos;
			break;
		case KEY_DOWN:
			startSelection();
			changeLine( 1 );
			mSelectionEnd = mCursorPos;
			break;
		case KEY_PAGE_DOWN:
			startSelection();
			changePage( 1 );
			mSelectionEnd = mCursorPos;
			break;
		case KEY_END:
			startSelection();
			if( mask & MASK_CONTROL )
			{
				mCursorPos = getLength();
			}
			else
			{
				endOfLine();
			}
			mSelectionEnd = mCursorPos;
			break;
		default:
			handled = FALSE;
			break;
		}
	}
	if( !handled && mHandleEditKeysDirectly )
	{
		if( (MASK_CONTROL & mask) && ('A' == key) )
		{
			if( canSelectAll() )
			{
				selectAll();
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
	}
	if( handled )
	{
		updatePrimary();
	}
	return handled;
}
BOOL LLTextEditor::handleNavigationKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;
	if( MASK_NONE == mask )
	{
		handled = TRUE;
		switch( key )
		{
		case KEY_UP:
			if (mReadOnly)
			{
				mScrollbar->setDocPos(mScrollbar->getDocPos() - 1);
			}
			else
			{
				changeLine( -1 );
			}
			break;
		case KEY_PAGE_UP:
			changePage( -1 );
			break;
		case KEY_HOME:
			if (mReadOnly)
			{
				mScrollbar->setDocPos(0);
			}
			else
			{
				startOfLine();
			}
			break;
		case KEY_DOWN:
			if (mReadOnly)
			{
				mScrollbar->setDocPos(mScrollbar->getDocPos() + 1);
			}
			else
			{
				changeLine( 1 );
			}
			break;
		case KEY_PAGE_DOWN:
			changePage( 1 );
			break;
		case KEY_END:
			if (mReadOnly)
			{
				mScrollbar->setDocPos(mScrollbar->getDocPosMax());
			}
			else
			{
				endOfLine();
			}
			break;
		case KEY_LEFT:
			if (mReadOnly)
			{
				break;
			}
			if( hasSelection() )
			{
				setCursorPos(llmin( mCursorPos - 1, mSelectionStart, mSelectionEnd ));
			}
			else
			{
				if( 0 < mCursorPos )
				{
					setCursorPos(mCursorPos - 1);
				}
				else
				{
					reportBadKeystroke();
				}
			}
			break;
		case KEY_RIGHT:
			if (mReadOnly)
			{
				break;
			}
			if( hasSelection() )
			{
				setCursorPos(llmax( mCursorPos + 1, mSelectionStart, mSelectionEnd ));
			}
			else
			{
				if( mCursorPos < getLength() )
				{
					setCursorPos(mCursorPos + 1);
				}
				else
				{
					reportBadKeystroke();
				}
			}
			break;
		default:
			handled = FALSE;
			break;
		}
	}
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
	return handled;
}
void LLTextEditor::deleteSelection(BOOL group_with_next_op )
{
	if( getEnabled() && hasSelection() )
	{
		S32 pos = llmin( mSelectionStart, mSelectionEnd );
		S32 length = llabs( mSelectionStart - mSelectionEnd );
		remove( pos, length, group_with_next_op );
		deselect();
		setCursorPos(pos);
	}
}
BOOL LLTextEditor::canCut() const
{
	return !mReadOnly && hasSelection();
}
void LLTextEditor::cut()
{
	if( !canCut() )
	{
		return;
	}
	S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
	S32 length = llabs( mSelectionStart - mSelectionEnd );
	gClipboard.copyFromSubstring( mWText, left_pos, length, mSourceID );
	deleteSelection( FALSE );
	needsReflow();
	onKeyStroke();
}
BOOL LLTextEditor::canCopy() const
{
	return hasSelection();
}
void LLTextEditor::copy(bool raw) const
{
	if( !canCopy() )
	{
		return;
	}
	S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
	S32 length = llabs( mSelectionStart - mSelectionEnd );
	if (mParseHTML && raw)
	{
		auto begin = std::find_if(mSegments.begin(), mSegments.end(), [left_pos](const LLTextSegmentPtr& ptr) {
			return ptr->getEnd() > left_pos;
		});
		auto last = mSegments.end();
		if (begin == last || begin->isNull())
		{
			gClipboard.copyFromSubstring(mWText, left_pos, length, mSourceID);
			return;
		}
		S32 right_pos = left_pos + length, offset = 0;
		{
			auto segment = **begin;
			if (auto style = segment.getStyle())
				if (style->isLink())
					left_pos = llmin(segment.getStart(), left_pos);
		}
		auto text = mWText.substr(left_pos, length);
		for (; begin->notNull() && begin != last && (*begin)->getStart() <= right_pos; ++begin)
		{
			auto segment = **begin;
			const auto& link = segment.getStyle()->getLinkHREF();
			if (!link.empty())
			{
				const S32 label_length = (segment.getEnd() - segment.getStart());
				const S32 start = (segment.getStart()+offset)-left_pos;
				const auto label = text.substr(start, label_length);
				const auto wlink = utf8str_to_wstring(link);
				const auto pos = wlink.find(label);
				if (pos == std::string::npos
					|| (pos != 0 && wlink[pos-1] != '/') || pos+label.size() != wlink.size())
				{
					constexpr llwchar startchar = '[', space = ' ', endchar = ']';
					const auto raw_html = startchar + wlink + space + label + endchar;
					text.replace(start, label_length, raw_html);
					offset += raw_html.size() - label_length;
				}
			}
		}
		gClipboard.copyFromSubstring(text, 0, text.length(), mSourceID);
	}
	else gClipboard.copyFromSubstring(mWText, left_pos, length, mSourceID);
}
BOOL LLTextEditor::canPaste() const
{
	return !mReadOnly && gClipboard.canPasteString();
}
void LLTextEditor::paste()
{
	bool is_primary = false;
	pasteHelper(is_primary);
}
void LLTextEditor::pastePrimary()
{
	bool is_primary = true;
	pasteHelper(is_primary);
}
void LLTextEditor::pasteHelper(bool is_primary)
{
	bool can_paste_it;
	if (is_primary)
	{
		can_paste_it = canPastePrimary();
	}
	else
	{
		can_paste_it = canPaste();
	}
	if (!can_paste_it)
	{
		return;
	}
	LLUUID source_id;
	LLWString paste;
	if (is_primary)
	{
		paste = gClipboard.getPastePrimaryWString(&source_id);
	}
	else
	{
		paste = gClipboard.getPasteWString(&source_id);
	}
	if (paste.empty())
	{
		return;
	}
	if( (!is_primary) && hasSelection() )
	{
		deleteSelection(TRUE);
	}
	LLWString clean_string(paste);
	LLWStringUtil::replaceTabsWithSpaces(clean_string, SPACES_PER_TAB);
	if( mAllowEmbeddedItems )
	{
		const llwchar LF = 10;
		S32 len = clean_string.length();
		for( S32 i = 0; i < len; i++ )
		{
			llwchar wc = clean_string[i];
			if( (wc < LLFontFreetype::FIRST_CHAR) && (wc != LF) )
			{
				clean_string[i] = LL_UNKNOWN_CHAR;
			}
			else if (wc >= FIRST_EMBEDDED_CHAR && wc <= LAST_EMBEDDED_CHAR)
			{
				clean_string[i] = pasteEmbeddedItem(wc);
			}
		}
	}
	setCursorPos(mCursorPos + insert(mCursorPos, clean_string, FALSE));
	deselect();
	needsReflow();
	onKeyStroke();
}
void LLTextEditor::copyPrimary()
{
	if( !canCopy() )
	{
		return;
	}
	S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
	S32 length = llabs( mSelectionStart - mSelectionEnd );
	gClipboard.copyFromPrimarySubstring(mWText, left_pos, length, mSourceID);
}
BOOL LLTextEditor::canPastePrimary() const
{
	return !mReadOnly && gClipboard.canPastePrimaryString();
}
void LLTextEditor::updatePrimary()
{
	if (canCopy())
	{
		copyPrimary();
	}
}
BOOL LLTextEditor::handleControlKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;
	if( mask & MASK_CONTROL )
	{
		handled = TRUE;
		switch( key )
		{
		case KEY_HOME:
			if( mask & MASK_SHIFT )
			{
				startSelection();
				mCursorPos = 0;
				mSelectionEnd = mCursorPos;
			}
			else
			{
				deselect();
				setCursorPos(0);
			}
			break;
		case KEY_END:
			{
				if( mask & MASK_SHIFT )
				{
					startSelection();
				}
				else
				{
					deselect();
				}
				endOfDoc();
				if( mask & MASK_SHIFT )
				{
					mSelectionEnd = mCursorPos;
				}
				break;
			}
		case KEY_RIGHT:
			if( mCursorPos < getLength() )
			{
				deselect();
				setCursorPos(nextWordPos(mCursorPos + 1));
			}
			break;
		case KEY_LEFT:
			if( mCursorPos > 0 )
			{
				deselect();
				setCursorPos(prevWordPos(mCursorPos - 1));
			}
			break;
		case KEY_DELETE:
			if (getEnabled())
				removeWord(false);
			else
				handled = false;
			break;
		default:
			handled = FALSE;
			break;
		}
	}
	if (handled)
	{
		updatePrimary();
	}
	return handled;
}
BOOL LLTextEditor::handleEditKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;
	if( KEY_DELETE == key )
	{
		if( canDoDelete() )
		{
			doDelete();
		}
		else
		{
			reportBadKeystroke();
		}
		handled = TRUE;
	}
	else if ((mask & (MASK_CONTROL|MASK_SHIFT)) == (MASK_CONTROL|MASK_SHIFT))
	{
		if (key == 'C')
		{
			if (canCopy())
				copy(true);
			else
				reportBadKeystroke();
			handled = TRUE;
		}
	}
	else
	if( MASK_CONTROL & mask )
	{
		if( 'C' == key )
		{
			if( canCopy() )
			{
				copy();
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
		else
		if( 'V' == key )
		{
			if( canPaste() )
			{
				paste();
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
		else
		if( 'X' == key )
		{
			if( canCut() )
			{
				cut();
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
	}
	if( handled )
	{
		updatePrimary();
	}
	return handled;
}
BOOL LLTextEditor::handleSpecialKey(const KEY key, const MASK mask, BOOL* return_key_hit)
{
	*return_key_hit = FALSE;
	BOOL handled = TRUE;
	switch( key )
	{
	case KEY_INSERT:
		if (mask == MASK_NONE)
		{
			gKeyboard->toggleInsertMode();
		}
		break;
	case KEY_BACKSPACE:
		if( hasSelection() )
		{
			deleteSelection(FALSE);
		}
		else
		if( 0 < mCursorPos )
		{
			if (mask == MASK_CONTROL)
				removeWord(true);
			else
				removeCharOrTab();
		}
		else
		{
			reportBadKeystroke();
		}
		break;
	case KEY_RETURN:
		if (mask == MASK_NONE)
		{
			if( hasSelection() )
			{
				deleteSelection(FALSE);
			}
			autoIndent();
		}
		else
		{
			handled = FALSE;
			break;
		}
		break;
	case KEY_TAB:
		if (mask & MASK_CONTROL)
		{
			handled = FALSE;
			break;
		}
		if( hasSelection() && selectionContainsLineBreaks() )
		{
			indentSelectedLines( (mask & MASK_SHIFT) ? -SPACES_PER_TAB : SPACES_PER_TAB );
		}
		else
		{
			if( hasSelection() )
			{
				deleteSelection(FALSE);
			}
			S32 line, offset;
			getLineAndOffset( mCursorPos, &line, &offset );
			S32 spaces_needed = SPACES_PER_TAB - (offset % SPACES_PER_TAB);
			for( S32 i=0; i < spaces_needed; i++ )
			{
				addChar( ' ' );
			}
		}
		break;
	default:
		handled = FALSE;
		break;
	}
	if (handled)
	{
		onKeyStroke();
	}
	return handled;
}
void LLTextEditor::unindentLineBeforeCloseBrace()
{
	if( mCursorPos >= 1 )
	{
		const LLWString &text = mWText;
		if( ' ' == text[ mCursorPos - 1 ] )
		{
			removeCharOrTab();
		}
	}
}
BOOL LLTextEditor::handleKeyHere(KEY key, MASK mask )
{
	BOOL	handled = FALSE;
	BOOL	selection_modified = FALSE;
	BOOL	return_key_hit = FALSE;
	BOOL	text_may_have_changed = TRUE;
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->isOpen())
	{
		LLMenuGL::sMenuContainer->hideMenus();
	}
	if ( gFocusMgr.getKeyboardFocus() == this )
	{
		if (KEY_TAB == key && mTabsToNextField)
		{
			return FALSE;
		}
		handled = handleNavigationKey( key, mask );
		if( handled )
		{
			text_may_have_changed = FALSE;
		}
		if( !handled )
		{
			handled = handleSelectionKey( key, mask );
			if( handled )
			{
				selection_modified = TRUE;
			}
		}
		if( !handled )
		{
			handled = handleControlKey( key, mask );
			if( handled )
			{
				selection_modified = TRUE;
			}
		}
		if( !handled && mHandleEditKeysDirectly )
		{
			handled = handleEditKey( key, mask );
			if( handled )
			{
				selection_modified = TRUE;
				text_may_have_changed = TRUE;
			}
		}
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
		if (menu && menu->isOpen())
		{
			LLMenuGL::sMenuContainer->hideMenus();
		}
		if( !mReadOnly )
		{
			if( !handled )
			{
				handled = handleSpecialKey( key, mask, &return_key_hit );
				if( handled )
				{
					selection_modified = TRUE;
					text_may_have_changed = TRUE;
				}
			}
		}
		if( handled )
		{
			resetKeystrokeTimer();
			if( !selection_modified &&
				KEY_SHIFT != key &&
				KEY_CONTROL != key &&
				KEY_ALT != key &&
				KEY_CAPSLOCK )
			{
				deselect();
			}
			if(text_may_have_changed)
			{
				needsReflow();
			}
			needsScroll();
		}
	}
	return handled;
}
BOOL LLTextEditor::handleUnicodeCharHere(llwchar uni_char)
{
	if ((uni_char < 0x20) || (uni_char == 0x7F))
	{
		return FALSE;
	}
	BOOL	handled = FALSE;
	if ( gFocusMgr.getKeyboardFocus() == this )
	{
		if( !mReadOnly )
		{
			if( '}' == uni_char )
			{
				unindentLineBeforeCloseBrace();
			}
			addChar( uni_char );
			getWindow()->hideCursorUntilMouseMove();
			handled = TRUE;
		}
		if( handled )
		{
			resetKeystrokeTimer();
			deselect();
			needsReflow();
			onKeyStroke();
		}
	}
	return handled;
}
BOOL LLTextEditor::canDoDelete() const
{
	return !mReadOnly && ( hasSelection() || (mCursorPos < getLength()) );
}
void LLTextEditor::doDelete()
{
	if( !canDoDelete() )
	{
		return;
	}
	if( hasSelection() )
	{
		deleteSelection(FALSE);
	}
	else
	if( mCursorPos < getLength() )
	{
		S32 i;
		S32 chars_to_remove = 1;
		const LLWString &text = mWText;
		if( (text[ mCursorPos ] == ' ') && (mCursorPos + SPACES_PER_TAB < getLength()) )
		{
			S32 line, offset;
			getLineAndOffset( mCursorPos, &line, &offset );
			chars_to_remove = SPACES_PER_TAB - (offset % SPACES_PER_TAB);
			if( chars_to_remove == 0 )
			{
				chars_to_remove = SPACES_PER_TAB;
			}
			for( i = 0; i < chars_to_remove; i++ )
			{
				if( text[mCursorPos + i] != ' ' )
				{
					chars_to_remove = 1;
					break;
				}
			}
		}
		for( i = 0; i < chars_to_remove; i++ )
		{
			setCursorPos(mCursorPos + 1);
			removeChar();
		}
	}
	needsReflow();
	onKeyStroke();
}
void LLTextEditor::blockUndo()
{
	mBaseDocIsPristine = FALSE;
	mLastCmd = NULL;
	std::for_each(mUndoStack.begin(), mUndoStack.end(), DeletePointer());
	mUndoStack.clear();
}
BOOL LLTextEditor::canUndo() const
{
	return !mReadOnly && mLastCmd != NULL;
}
void LLTextEditor::undo()
{
	if( !canUndo() )
	{
		return;
	}
	deselect();
	S32 pos = 0;
	do
	{
		pos = mLastCmd->undo(this);
		undo_stack_t::iterator iter = std::find(mUndoStack.begin(), mUndoStack.end(), mLastCmd);
		if (iter != mUndoStack.end())
			++iter;
		if (iter != mUndoStack.end())
			mLastCmd = *iter;
		else
			mLastCmd = NULL;
		} while( mLastCmd && mLastCmd->groupWithNext() );
		setCursorPos(pos);
	needsReflow();
	onKeyStroke();
}
BOOL LLTextEditor::canRedo() const
{
	return !mReadOnly && (mUndoStack.size() > 0) && (mLastCmd != mUndoStack.front());
}
void LLTextEditor::redo()
{
	if( !canRedo() )
	{
		return;
	}
	deselect();
	S32 pos = 0;
	do
	{
		if( !mLastCmd )
		{
			mLastCmd = mUndoStack.back();
		}
		else
		{
			undo_stack_t::iterator iter = std::find(mUndoStack.begin(), mUndoStack.end(), mLastCmd);
			if (iter != mUndoStack.begin())
				mLastCmd = *(--iter);
			else
				mLastCmd = NULL;
		}
			if( mLastCmd )
			{
				pos = mLastCmd->redo(this);
			}
		} while(
			mLastCmd &&
			mLastCmd->groupWithNext() &&
			(mLastCmd != mUndoStack.front()) );
		setCursorPos(pos);
	needsReflow();
	onKeyStroke();
}
void LLTextEditor::onFocusReceived()
{
	LLUICtrl::onFocusReceived();
	updateAllowingLanguageInput();
}
void LLTextEditor::onFocusLost()
{
	updateAllowingLanguageInput();
 	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}
	if (mCommitOnFocusLost)
	{
		onCommit();
	}
	getWindow()->showCursorFromMouseMove();
	LLUICtrl::onFocusLost();
}
void LLTextEditor::setEnabled(BOOL enabled)
{
	BOOL read_only = !enabled;
	if (read_only != mReadOnly)
	{
		mReadOnly = read_only;
		updateSegments();
		updateAllowingLanguageInput();
	}
}
void LLTextEditor::drawBackground()
{
	S32 left = 0;
	S32 top = getRect().getHeight();
	S32 right = getRect().getWidth();
	S32 bottom = 0;
	LLColor4 bg_color = mReadOnly ? mReadOnlyBgColor
		: gFocusMgr.getKeyboardFocus() == this ? mFocusBgColor : mWriteableBgColor;
	if( mShowLineNumbers ) {
		gl_rect_2d(left, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN, bottom, mReadOnlyBgColor );
		gl_rect_2d(UI_TEXTEDITOR_LINE_NUMBER_MARGIN, top, right, bottom, bg_color);
		gl_rect_2d(UI_TEXTEDITOR_LINE_NUMBER_MARGIN, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN-1, bottom, LLColor4::grey3);
	} else {
		gl_rect_2d(left, top, right, bottom, bg_color);
	}
	LLView::draw();
}
void LLTextEditor::drawSelectionBackground()
{
	if( hasSelection() )
	{
		const LLWString &text = mWText;
		const S32 text_len = getLength();
		std::queue<S32> line_endings;
		S32 line_height = ll_round( mGLFont->getLineHeight() );
		S32 selection_left		= llmin( mSelectionStart, mSelectionEnd );
		S32 selection_right		= llmax( mSelectionStart, mSelectionEnd );
		S32 selection_left_x	= mTextRect.mLeft;
		S32 selection_left_y	= mTextRect.mTop - line_height;
		S32 selection_right_x	= mTextRect.mRight;
		S32 selection_right_y	= mTextRect.mBottom;
		BOOL selection_left_visible = FALSE;
		BOOL selection_right_visible = FALSE;
		S32 cur_line = mScrollbar->getDocPos();
		S32 left_line_num = cur_line;
		S32 num_lines = getLineCount();
		S32 right_line_num = num_lines - 1;
		S32 line_start = -1;
		if (cur_line >= num_lines)
		{
			return;
		}
		line_start = getLineStart(cur_line);
		S32 left_visible_pos	= line_start;
		S32 right_visible_pos	= line_start;
		S32 text_y = mTextRect.mTop - line_height;
		while((cur_line < num_lines))
		{
			S32 next_line = -1;
			S32 line_end = text_len;
			if ((cur_line + 1) < num_lines)
			{
				next_line = getLineStart(cur_line + 1);
				line_end = next_line;
				line_end = ( (line_end - line_start)==0 || text[next_line-1] == '\n' || text[next_line-1] == '\0' || text[next_line-1] == ' ' || text[next_line-1] == '\t'  ) ? next_line-1 : next_line;
			}
			const llwchar* line = text.c_str() + line_start;
			if( line_start <= selection_left && selection_left <= line_end )
			{
				left_line_num = cur_line;
				selection_left_visible = TRUE;
				selection_left_x = mTextRect.mLeft + mGLFont->getWidth(line, 0, selection_left - line_start, mAllowEmbeddedItems);
				selection_left_y = text_y;
			}
			if( line_start <= selection_right && selection_right <= line_end )
			{
				right_line_num = cur_line;
				selection_right_visible = TRUE;
				selection_right_x = mTextRect.mLeft + mGLFont->getWidth(line, 0, selection_right - line_start, mAllowEmbeddedItems);
				if (selection_right == line_end)
				{
				}
				selection_right_y = text_y;
			}
			if (selection_left <= line_end && line_end < selection_right && selection_left != selection_right)
			{
				const LLWString nstr(utf8str_to_wstring(std::string("n")));
				line_endings.push(mTextRect.mLeft + mGLFont->getWidth(line, 0, line_end - line_start, mAllowEmbeddedItems) + mGLFont->getWidth(nstr.c_str()));
			}
			text_y -= line_height;
			right_visible_pos = line_end;
			line_start = next_line;
			cur_line++;
			if (selection_right_visible)
			{
				break;
			}
		}
		BOOL selection_visible = (left_visible_pos <= selection_right) && (selection_left <= right_visible_pos);
		if( selection_visible )
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			const LLColor4& color = mReadOnly ? mReadOnlyBgColor : mWriteableBgColor;
			F32 alpha = hasFocus() ? 1.f : 0.8f;
			gGL.color4f( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2], alpha );
			S32 margin_offset = mShowLineNumbers ? UI_TEXTEDITOR_LINE_NUMBER_MARGIN : 0;
			if( selection_left_y == selection_right_y )
			{
				gl_rect_2d( selection_left_x + margin_offset, selection_left_y + line_height + 1,
					selection_right_x + margin_offset, selection_right_y);
			}
			else
			{
				if( mTextRect.mRight == selection_left_x )
				{
					selection_left_x -= CURSOR_THICKNESS;
				}
				S32 line_end = line_endings.front();
				line_endings.pop();
				gl_rect_2d( selection_left_x + margin_offset, selection_left_y + line_height + 1,
					line_end + margin_offset, selection_left_y );
				S32 line_num = left_line_num + 1;
				while(line_endings.size())
				{
					S32 vert_offset = -(line_num - left_line_num) * line_height;
					gl_rect_2d( mTextRect.mLeft + margin_offset, selection_left_y + vert_offset + line_height + 1,
						line_endings.front() + margin_offset, selection_left_y + vert_offset);
					line_endings.pop();
					line_num++;
				}
				if( mTextRect.mLeft == selection_right_x )
				{
					selection_right_x += CURSOR_THICKNESS;
				}
				gl_rect_2d( mTextRect.mLeft + margin_offset, selection_right_y + line_height + 1,
					selection_right_x + margin_offset, selection_right_y );
			}
		}
	}
}
void LLTextEditor::drawMisspelled()
{
	if (!mReadOnly && mSpellCheckable)
	{
		if(
			( ((getLength()<400)||(false))	&&(  (S32(mSpellTimer.getElapsedTimeF32() / 1) & 1) ))
			||
			(S32(mKeystrokeTimer.getElapsedTimeF32() / 1) & 1)
			)
		{
			S32 newSpellStart = getLineStart(mScrollbar->getDocPos());
			S32 newSpellEnd = getLineStart(mScrollbar->getDocPos() + 1 + mScrollbar->getDocSize()-mScrollbar->getDocPosMax());
			if (mScrollbar->getDocPos() == mScrollbar->getDocPosMax())
			{
				newSpellEnd = (S32)mWText.length();
			}
			if (isSpellDirty() || (newSpellEnd!=spellEnd || newSpellStart!=spellStart))
			{
				spellEnd = newSpellEnd;
				spellStart = newSpellStart;
				misspellLocations = getMisspelledWordsPositions();
			}
		}
		if (glggHunSpell->getSpellCheckHighlight())
		{
			for (int i = 0; i<(int)misspellLocations.size() ;i++)
			{
				S32 wstart = misspellLocations[i];
				S32 wend = misspellLocations[++i];
				const LLWString &text = mWText;
				const S32 text_len = getLength();
				S32 search_pos = mScrollbar->getDocPos();
				S32 num_lines = getLineCount();
				if (search_pos >= num_lines)
				{
					return;
				}
				S32 line_start = getLineStart(search_pos);
				F32 line_height = mGLFont->getLineHeight();
				F32 text_y = (F32)(mTextRect.mTop) - line_height;
				F32 word_left = 0.f;
				F32 word_right = 0.f;
				F32 word_bottom = 0.f;
				BOOL word_visible = FALSE;
				S32 line_end = 0;
				while( (mTextRect.mBottom <= ll_round(text_y)) && (search_pos < num_lines))
				{
					line_end = text_len + 1;
					S32 next_line = -1;
					if ((search_pos + 1) < num_lines)
					{
						next_line = getLineStart(search_pos + 1);
						line_end = next_line - 1;
					}
					const llwchar* line = text.c_str() + line_start;
					if( line_start <= wstart && wend <= line_end )
					{
						word_visible = TRUE;
						word_left = (F32)mTextRect.mLeft + mGLFont->getWidthF32(line, 0, wstart - line_start, mAllowEmbeddedItems )-1.f;
						word_right = (F32)mTextRect.mLeft + mGLFont->getWidthF32(line, 0, wend - line_start, mAllowEmbeddedItems )+1.f;
						word_bottom = text_y;
						break;
					}
					text_y -= line_height;
					line_start = next_line;
					search_pos++;
				}
				if (mShowLineNumbers)
				{
					word_left += UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
					word_right += UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
				}
				if (word_visible)
				{
					gGL.color4ub(255,0,0,200);
					while (word_left<word_right)
					{
						gl_line_2d(word_left,word_bottom-2, word_left+3,word_bottom+1);
						gl_line_2d(word_left+3,word_bottom+1, word_left+6,word_bottom-2);
						word_left += 6;
					}
				}
			}
		}
	}
}
void LLTextEditor::drawCursor()
{
	if( gFocusMgr.getKeyboardFocus() == this
		&& gShowTextEditCursor && !mReadOnly)
	{
		const LLWString &text = mWText;
		const S32 text_len = getLength();
		S32 cur_pos = mScrollbar->getDocPos();
		S32 num_lines = getLineCount();
		if (cur_pos >= num_lines)
		{
			return;
		}
		S32 line_start = getLineStart(cur_pos);
		F32 line_height = mGLFont->getLineHeight();
		F32 text_y = (F32)(mTextRect.mTop) - line_height;
		F32 cursor_left = 0.f;
		F32 next_char_left = 0.f;
		F32 cursor_bottom = 0.f;
		BOOL cursor_visible = FALSE;
		S32 line_end = 0;
		while( (mTextRect.mBottom <= ll_round(text_y)) && (cur_pos < num_lines))
		{
			line_end = text_len + 1;
			S32 next_line = -1;
			if ((cur_pos + 1) < num_lines)
			{
				next_line = getLineStart(cur_pos + 1);
				line_end = next_line - 1;
			}
			const llwchar* line = text.c_str() + line_start;
			if( line_start <= mCursorPos && mCursorPos <= line_end )
			{
				cursor_visible = TRUE;
				next_char_left = (F32)mTextRect.mLeft + mGLFont->getWidthF32(line, 0, mCursorPos - line_start, mAllowEmbeddedItems );
				cursor_left = next_char_left - 1.f;
				cursor_bottom = text_y;
				break;
			}
			text_y -= line_height;
			line_start = next_line;
			cur_pos++;
		}
		if(mShowLineNumbers)
		{
			cursor_left += UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
		}
		if( cursor_visible )
		{
			F32 elapsed = mKeystrokeTimer.getElapsedTimeF32();
			if( (elapsed < CURSOR_FLASH_DELAY ) || (S32(elapsed * 2) & 1) )
			{
				F32 cursor_top = cursor_bottom + line_height + 1.f;
				F32 cursor_right = cursor_left + (F32)CURSOR_THICKNESS;
				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
				{
					cursor_left += CURSOR_THICKNESS;
					const LLWString space(utf8str_to_wstring(std::string(" ")));
					F32 spacew = mGLFont->getWidthF32(space.c_str());
					if (mCursorPos == line_end)
					{
						cursor_right = cursor_left + spacew;
					}
					else
					{
						F32 width = mGLFont->getWidthF32(text.c_str(), mCursorPos, 1, mAllowEmbeddedItems);
						cursor_right = cursor_left + llmax(spacew, width);
					}
				}
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gGL.color4fv( mCursorColor.mV );
				gl_rect_2d(llfloor(cursor_left), llfloor(cursor_top),
					llfloor(cursor_right), llfloor(cursor_bottom));
				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection() && text[mCursorPos] != '\n')
				{
					const LLTextSegment* segmentp = getSegmentAtOffset(mCursorPos);
					LLColor4 text_color;
					if (segmentp)
					{
						text_color = segmentp->getColor();
					}
					else if (mReadOnly)
					{
						text_color = mReadOnlyFgColor;
					}
					else
					{
						text_color = mFgColor;
					}
					mGLFont->render(text, mCursorPos, next_char_left, cursor_bottom + line_height,
						LLColor4(1.f - text_color.mV[VRED], 1.f - text_color.mV[VGREEN], 1.f - text_color.mV[VBLUE], 1.f),
						LLFontGL::LEFT, LLFontGL::TOP,
						LLFontGL::NORMAL,
						LLFontGL::NO_SHADOW,
						1);
				}
				LLRect screen_pos = calcScreenRect();
				LLCoordGL ime_pos( screen_pos.mLeft + llfloor(cursor_left), screen_pos.mBottom + llfloor(cursor_top) );
				ime_pos.mX = (S32) (ime_pos.mX * LLUI::getScaleFactor().mV[VX]);
				ime_pos.mY = (S32) (ime_pos.mY * LLUI::getScaleFactor().mV[VY]);
				getWindow()->setLanguageTextInput( ime_pos );
			}
		}
	}
}
void LLTextEditor::drawPreeditMarker()
{
	if (!hasPreeditString())
	{
		return;
	}
	const llwchar *text = mWText.c_str();
	const S32 text_len = getLength();
	const S32 num_lines = getLineCount();
	S32 cur_line = mScrollbar->getDocPos();
	if (cur_line >= num_lines)
	{
		return;
	}
	const S32 line_height = ll_round( mGLFont->getLineHeight() );
	S32 line_start = getLineStart(cur_line);
	S32 line_y = mTextRect.mTop - line_height;
	while((mTextRect.mBottom <= line_y) && (num_lines > cur_line))
	{
		S32 next_start = -1;
		S32 line_end = text_len;
		if ((cur_line + 1) < num_lines)
		{
			next_start = getLineStart(cur_line + 1);
			line_end = next_start;
		}
		if ( text[line_end-1] == '\n' )
		{
			--line_end;
		}
		if (line_start >= mPreeditPositions.back())
		{
			break;
		}
		if (line_end > mPreeditPositions.front())
		{
			for (U32 i = 0; i < mPreeditStandouts.size(); i++)
			{
				S32 left = mPreeditPositions[i];
				S32 right = mPreeditPositions[i + 1];
				if (right <= line_start || left >= line_end)
				{
					continue;
				}
				S32 preedit_left = mTextRect.mLeft;
				if (left > line_start)
				{
					preedit_left += mGLFont->getWidth(text, line_start, left - line_start, mAllowEmbeddedItems);
				}
				S32 preedit_right = mTextRect.mLeft;
				if (right < line_end)
				{
					preedit_right += mGLFont->getWidth(text, line_start, right - line_start, mAllowEmbeddedItems);
				}
				else
				{
					preedit_right += mGLFont->getWidth(text, line_start, line_end - line_start, mAllowEmbeddedItems);
				}
				if (mPreeditStandouts[i])
				{
					gl_rect_2d(preedit_left + PREEDIT_STANDOUT_GAP,
							line_y + PREEDIT_STANDOUT_POSITION,
							preedit_right - PREEDIT_STANDOUT_GAP - 1,
							line_y + PREEDIT_STANDOUT_POSITION - PREEDIT_STANDOUT_THICKNESS,
							(mCursorColor * PREEDIT_STANDOUT_BRIGHTNESS + mWriteableBgColor * (1 - PREEDIT_STANDOUT_BRIGHTNESS)).setAlpha(1.0f));
				}
				else
				{
					gl_rect_2d(preedit_left + PREEDIT_MARKER_GAP,
							line_y + PREEDIT_MARKER_POSITION,
							preedit_right - PREEDIT_MARKER_GAP - 1,
							line_y + PREEDIT_MARKER_POSITION - PREEDIT_MARKER_THICKNESS,
							(mCursorColor * PREEDIT_MARKER_BRIGHTNESS + mWriteableBgColor * (1 - PREEDIT_MARKER_BRIGHTNESS)).setAlpha(1.0f));
				}
			}
		}
		line_y -= line_height;
		line_start = next_start;
		cur_line++;
	}
}
void LLTextEditor::drawText()
{
	const LLWString &text = mWText;
	const S32 text_len = getLength();
	if( text_len <= 0 ) return;
	S32 selection_left = -1;
	S32 selection_right = -1;
	if( hasSelection())
	{
		selection_left = llmin( mSelectionStart, mSelectionEnd );
		selection_right = llmax( mSelectionStart, mSelectionEnd );
	}
	LLGLSUIDefault gls_ui;
	S32 cur_line = mScrollbar->getDocPos();
	S32 num_lines = getLineCount();
	if (cur_line >= num_lines) return;
	S32 line_start = getLineStart(cur_line);
	S32 prev_start = getLineStart(cur_line-1);
	S32 cur_line_num  = getLineForPosition(line_start);
	S32 prev_line_num = getLineForPosition(prev_start);
	BOOL cur_line_is_continuation = cur_line_num > 0 && cur_line_num == prev_line_num;
	BOOL line_wraps = FALSE;
	LLTextSegment t(line_start);
	segment_list_t::iterator seg_iter;
	seg_iter = std::upper_bound(mSegments.begin(), mSegments.end(), &t, LLTextSegment::compare());
	if (seg_iter == mSegments.end() || (*seg_iter)->getStart() > line_start) --seg_iter;
	LLTextSegment* cur_segment = *seg_iter;
	S32 line_height = ll_round( mGLFont->getLineHeight() );
	F32 text_y = (F32)(mTextRect.mTop - line_height);
	while((mTextRect.mBottom <= text_y) && (cur_line < num_lines))
	{
		S32 next_line = cur_line + 1;
		S32 next_start = -1;
		S32 line_end = text_len;
		if (next_line < num_lines)
		{
			next_start = getLineStart(next_line);
			line_end = next_start;
		}
		line_wraps = text[line_end-1] != '\n';
		if ( ! line_wraps )
		{
			--line_end;
		}
		F32 text_start = (F32)mTextRect.mLeft;
		F32 text_x = text_start + (mShowLineNumbers ? UI_TEXTEDITOR_LINE_NUMBER_MARGIN : 0);
		if( mShowLineNumbers && !cur_line_is_continuation)
		{
			const LLFontGL *num_font = LLFontGL::getFontMonospace();
			F32 y_top = text_y + ((F32)ll_round(num_font->getLineHeight()) / 2);
			BOOL is_cur_line = getCurrentLine() == cur_line_num;
			const U8 style = is_cur_line ? LLFontGL::BOLD : LLFontGL::NORMAL;
			const LLColor4 fg_color = is_cur_line ? mCursorColor : mReadOnlyFgColor;
			num_font->renderUTF8(
				llformat("%*d", UI_TEXTEDITOR_LINE_NUMBER_DIGITS, cur_line_num),
				0,
				3.,
				y_top,
				fg_color,
				LLFontGL::LEFT,
				LLFontGL::VCENTER,
				style,
				LLFontGL::NO_SHADOW,
				S32_MAX,
				UI_TEXTEDITOR_LINE_NUMBER_MARGIN,
				NULL);
		}
		S32 seg_start = line_start;
		while( seg_start < line_end )
		{
			while( cur_segment->getEnd() <= seg_start )
			{
				seg_iter++;
				if (seg_iter == mSegments.end())
				{
					LL_WARNS() << "Ran off the segmentation end!" << LL_ENDL;
					return;
				}
				cur_segment = *seg_iter;
			}
			S32 clipped_end	=	llmin( line_end, cur_segment->getEnd() );
			S32 clipped_len =	clipped_end - seg_start;
			if( clipped_len > 0 )
			{
				LLStyleSP style = cur_segment->getStyle();
				if ( style->isImage() && (cur_segment->getStart() >= seg_start) && (cur_segment->getStart() <= clipped_end))
				{
					S32 style_image_height = style->mImageHeight;
					S32 style_image_width = style->mImageWidth;
					LLUIImagePtr image = style->getImage();
					image->draw(ll_round(text_x), ll_round(text_y)+line_height-style_image_height,
						style_image_width, style_image_height);
				}
				if (cur_segment == mHoverSegment && style->getIsEmbeddedItem())
				{
					style->mUnderline = TRUE;
				}
				drawClippedSegment( text, seg_start, clipped_end, text_x, text_y, selection_left, selection_right, style, &text_x );
				if( text_x == text_start && mShowLineNumbers )
				{
					text_x += UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
				}
				seg_start += clipped_len;
			}
		}
		text_y -= (F32)line_height;
		if( line_wraps )
		{
			cur_line_num--;
		}
		cur_line_is_continuation = line_wraps;
		line_start = next_start;
		cur_line++;
		cur_line_num++;
	}
}
void LLTextEditor::drawClippedSegment(const LLWString &text, S32 seg_start, S32 seg_end, F32 x, F32 y, S32 selection_left, S32 selection_right, const LLStyleSP& style, F32* right_x )
{
	if (!style->isVisible())
	{
		return;
	}
	const LLFontGL* font = mGLFont;
	LLColor4 color = style->getColor();
	if ( style->getFontString()[0] )
	{
		font = LLResMgr::getInstance()->getRes(style->getFontID());
	}
	U8 font_flags = LLFontGL::NORMAL;
	if (style->mBold)
	{
		font_flags |= LLFontGL::BOLD;
	}
	if (style->mItalic)
	{
		font_flags |= LLFontGL::ITALIC;
	}
	if (style->mUnderline)
	{
		font_flags |= LLFontGL::UNDERLINE;
	}
	if (style->getIsEmbeddedItem())
	{
		if (mReadOnly)
		{
			color = LLUI::sColorsGroup->getColor("TextEmbeddedItemReadOnlyColor");
		}
		else
		{
			color = LLUI::sColorsGroup->getColor("TextEmbeddedItemColor");
		}
	}
	F32 y_top = y + (F32)ll_round(font->getLineHeight());
  	if( selection_left > seg_start )
	{
		S32 start = seg_start;
		S32 end = llmin( selection_left, seg_end );
		S32 length =  end - start;
		font->render(text, start, x, y_top, color, LLFontGL::LEFT, LLFontGL::TOP, font_flags, LLFontGL::NO_SHADOW, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
	x = *right_x;
	if( (selection_left < seg_end) && (selection_right > seg_start) )
	{
		S32 start = llmax( selection_left, seg_start );
		S32 end = llmin( selection_right, seg_end );
		S32 length = end - start;
		font->render(text, start, x, y_top,
					 LLColor4( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2], 1.f ),
					 LLFontGL::LEFT, LLFontGL::TOP, font_flags, LLFontGL::NO_SHADOW, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
	x = *right_x;
	if( selection_right < seg_end )
	{
		S32 start = llmax( selection_right, seg_start );
		S32 end = seg_end;
		S32 length = end - start;
		font->render(text, start, x, y_top, color, LLFontGL::LEFT, LLFontGL::TOP, font_flags, LLFontGL::NO_SHADOW, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
 }
void LLTextEditor::draw()
{
	if (mReflowNeeded)
	{
		updateLineStartList();
		mReflowNeeded = FALSE;
	}
	if (mScrollNeeded)
	{
		updateScrollFromCursor();
		mScrollNeeded = FALSE;
	}
	{
		LLLocalClipRect clip(LLRect(0, getRect().getHeight(), getRect().getWidth() - (mScrollbar->getVisible() ? SCROLLBAR_SIZE : 0), 0));
			bindEmbeddedChars(mGLFont);
			drawBackground();
			drawSelectionBackground();
			drawPreeditMarker();
			drawText();
			drawCursor();
			drawMisspelled();
			resetSpellDirty();
			unbindEmbeddedChars(mGLFont);
		mBorder->setKeyboardFocusHighlight( gFocusMgr.getKeyboardFocus() == this);
	}
	LLView::draw();
	mScrolledToBottom = isScrolledToBottom();
}
void LLTextEditor::onTabInto()
{
}
void LLTextEditor::clear()
{
	setText(LLStringUtil::null);
	mSegments.clear();
}
void LLTextEditor::setFocus( BOOL new_state )
{
	BOOL old_state = hasFocus();
	if (new_state == old_state) return;
	if (!new_state)
	{
		getWindow()->allowLanguageTextInput(this, FALSE);
	}
	LLUICtrl::setFocus( new_state );
	if( new_state )
	{
		gEditMenuHandler = this;
		resetKeystrokeTimer();
	}
	else
	{
		if( gEditMenuHandler == this )
		{
			gEditMenuHandler = NULL;
		}
		endSelection();
	}
}
BOOL LLTextEditor::acceptsTextInput() const
{
	return !mReadOnly;
}
S32 LLTextEditor::getPos( S32 line, S32 offset )
{
	S32 line_start = getLineStart(line);
	S32 next_start = getLineStart(line+1);
	if (next_start == line_start)
	{
		next_start = getLength() + 1;
	}
	S32 line_length = next_start - line_start - 1;
	line_length = llmax(line_length, 0);
	return line_start + llmin( offset, line_length );
}
void LLTextEditor::changePage( S32 delta )
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	S32 desired_x_pixel = mDesiredXPixel;
	S32 page_size = mScrollbar->getPageSize() - 1;
	if( delta == -1 )
	{
		line = llmax( line - page_size, 0);
		setCursorPos(getPos( line, offset ));
		mScrollbar->setDocPos( mScrollbar->getDocPos() - page_size );
	}
	else
	if( delta == 1 )
	{
		setCursorPos(getPos( line + page_size, offset ));
		mScrollbar->setDocPos( mScrollbar->getDocPos() + page_size );
	}
	mDesiredXPixel = desired_x_pixel;
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}
void LLTextEditor::changeLine( S32 delta )
{
	bindEmbeddedChars(mGLFont);
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	S32  line_start = getLineStart(line);
	S32 desired_x_pixel = mDesiredXPixel;
	if( desired_x_pixel == -1 )
	{
		desired_x_pixel = mGLFont->getWidth(mWText.c_str(), line_start, offset, mAllowEmbeddedItems );
	}
	S32 new_line = 0;
	if( (delta < 0) && (line > 0 ) )
	{
		new_line = line - 1;
	}
	else
	if( (delta > 0) && (line < (getLineCount() - 1)) )
	{
		new_line = line + 1;
	}
	else
	{
		unbindEmbeddedChars(mGLFont);
		return;
	}
	S32 num_lines = getLineCount();
	S32 new_line_start = getLineStart(new_line);
	S32 new_line_end = getLength();
	if (new_line + 1 < num_lines)
	{
		new_line_end = getLineStart(new_line + 1) - 1;
	}
	S32 new_line_len = new_line_end - new_line_start;
	S32 new_offset;
	new_offset = mGLFont->charFromPixelOffset(mWText.c_str(), new_line_start,
											  (F32)desired_x_pixel,
											  (F32)mTextRect.getWidth(),
											  new_line_len,
											  mAllowEmbeddedItems);
	setCursorPos (getPos( new_line, new_offset ));
	mDesiredXPixel = desired_x_pixel;
	unbindEmbeddedChars(mGLFont);
}
BOOL LLTextEditor::isScrolledToTop()
{
	return mScrollbar->isAtBeginning();
}
BOOL LLTextEditor::isScrolledToBottom()
{
	return mScrollbar->isAtEnd();
}
void LLTextEditor::startOfLine()
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	setCursorPos(mCursorPos - offset);
}
void LLTextEditor::setCursorAndScrollToEnd()
{
	deselect();
	endOfDoc();
	needsScroll();
}
void LLTextEditor::scrollToPos(S32 pos)
{
	mScrollbar->setDocSize( getLineCount() );
	S32 line, offset;
	getLineAndOffset(pos, &line, &offset );
	S32 page_size = mScrollbar->getPageSize();
	if( line < mScrollbar->getDocPos() )
	{
		mScrollbar->setDocPos( line );
	}
	else if( line >= mScrollbar->getDocPos() + page_size - 1 )
	{
		S32 new_pos = 0;
		if( line < mScrollbar->getDocSize() - 1 )
		{
			new_pos = line - page_size + 1;
		}
		else
		{
			new_pos = mScrollbar->getDocPosMax();
		}
		mScrollbar->setDocPos( new_pos );
	}
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}
void LLTextEditor::getLineAndColumnForPosition( S32 position, S32* line, S32* col, BOOL include_wordwrap )
{
	if( include_wordwrap )
	{
		getLineAndOffset( mCursorPos, line, col );
	}
	else
	{
		const LLWString &text = mWText;
		S32 line_count = 0;
		S32 line_start = 0;
		S32 i;
		for( i = 0; text[i] && (i < position); i++ )
		{
			if( '\n' == text[i] )
			{
				line_start = i + 1;
				line_count++;
			}
		}
		*line = line_count;
		*col = i - line_start;
	}
}
void LLTextEditor::getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap )
{
	getLineAndColumnForPosition(mCursorPos, line, col, include_wordwrap);
}
S32 LLTextEditor::getCurrentLine()
{
	return getLineForPosition(mCursorPos);
}
S32 LLTextEditor::getLineForPosition(S32 position)
{
	S32 line, col;
	getLineAndColumnForPosition(position, &line, &col, FALSE);
	return line;
}
void LLTextEditor::endOfLine()
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	S32 num_lines = getLineCount();
	if (line + 1 >= num_lines)
	{
		setCursorPos(getLength());
	}
	else
	{
		setCursorPos( getLineStart(line + 1) - 1 );
	}
}
void LLTextEditor::endOfDoc()
{
	mScrollbar->setDocPos(mScrollbar->getDocPosMax());
	mScrolledToBottom = true;
	S32 len = getLength();
	if( len )
	{
		setCursorPos(len);
	}
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}
void LLTextEditor::updateScrollFromCursor()
{
	if (mReadOnly)
	{
		return;
	}
	scrollToPos(mCursorPos);
}
void LLTextEditor::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape( width, height, called_from_parent );
	updateTextRect();
	needsReflow();
	mScrollbar->setDocSize( getLineCount() );
	S32 line_height = ll_round( mGLFont->getLineHeight() );
	S32 page_lines = mTextRect.getHeight() / line_height;
	mScrollbar->setPageSize( page_lines );
}
void LLTextEditor::autoIndent()
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	S32 line_start = getLineStart(line);
	S32 space_count = 0;
	S32 i;
	const LLWString &text = mWText;
	while( ' ' == text[line_start] )
	{
		space_count++;
		line_start++;
	}
	if( (mCursorPos > 0) && (text[mCursorPos -1] == '{') )
	{
		space_count += SPACES_PER_TAB;
	}
	addChar( '\n' );
	for( i = 0; i < space_count; i++ )
	{
		addChar( ' ' );
	}
}
void LLTextEditor::insertText(const std::string &new_text,BOOL deleteCurrentSelection)
{
	BOOL enabled = getEnabled();
	setEnabled( TRUE );
	if( hasSelection() && (deleteCurrentSelection))
	{
		deleteSelection(TRUE);
	}
	setCursorPos(mCursorPos + insert( mCursorPos, utf8str_to_wstring(new_text), FALSE ));
	needsReflow();
	setEnabled( enabled );
}
void LLTextEditor::appendColoredText(const std::string &new_text,
									 bool allow_undo,
									 bool prepend_newline,
									 const LLColor4 &color,
									 const std::string& font_name)
{
	LLColor4 lcolor=color;
	if (mParseHighlights && !gDirUtilp->getLindenUserDir(true).empty())
	{
		LLTextParser* highlight = LLTextParser::getInstance();
		highlight->parseFullLineHighlights(new_text, &lcolor);
	}
	LLStyleSP style(new LLStyle);
	style->setVisible(true);
	style->setColor(lcolor);
	style->setFontName(font_name);
	appendText(new_text, allow_undo, prepend_newline, style);
}
static LLTrace::BlockTimerStatHandle FTM_APPEND_TEXT("Append Text");
void LLTextEditor::appendText(const std::string &new_text, bool allow_undo, bool prepend_newline,
							  const LLStyleSP style, bool force_replace_links)
{
	LL_RECORD_BLOCK_TIME(FTM_APPEND_TEXT);
	if (new_text.empty())
		return;
	std::string text = prepend_newline && !mWText.empty() ? ('\n' + new_text) : new_text;
	appendTextImpl(text, style, force_replace_links);
	if (!allow_undo)
	{
		blockUndo();
	}
}
static LLTrace::BlockTimerStatHandle FTM_PARSE_HTML("Parse HTML");
void LLTextEditor::appendTextImpl(const std::string &new_text, const LLStyleSP style, bool force_replace_links)
{
	std::string text = new_text;
	static const LLUICachedControl<bool> replace_links("SinguReplaceLinks");
	force_replace_links = force_replace_links || replace_links;
	bool is_link = style && style->isLink();
	S32 part = (S32)LLTextParser::WHOLE;
	if (mReadOnly && mParseHTML && !is_link)
	{
		LL_RECORD_BLOCK_TIME(FTM_PARSE_HTML);
		size_t start=0, end=0;
		LLUrlMatch match;
		auto append_substr = [&](const size_t& pos, const size_t& count)
		{
			appendAndHighlightText(text.substr(pos, count), part, style);
		};
		auto append_link = [&](const std::string& link, LLStyleSP link_style)
		{
			if (!link_style->isLink())
			{
				appendAndHighlightText(link, part, style);
				return;
			}
			if (style)
			{
				const auto& text_style = *style;
				link_style->mItalic = text_style.mItalic;
				link_style->mBold = text_style.mBold;
				link_style->mUnderline = text_style.mUnderline;
			}
			if (mLinkColor) link_style->setColor(*mLinkColor);
			const auto always_underline(always_underline_links());
			if (always_underline) link_style->mUnderline = true;
			appendAndHighlightText(link, part, link_style, !always_underline);
		};
		const auto&& cb = force_replace_links ? boost::bind(&LLTextEditor::replaceUrl, this, _1, _2, _3) : static_cast<LLUrlLabelCallback>(LLUrlRegistryNullCallback);
		auto& urlr = LLUrlRegistry::instance();
		while (!text.empty() && urlr.findUrl(text, match, cb))
		{
			start = match.getStart();
			end = match.getEnd()+1;
			if (start > 0)
			{
				if (part == (S32)LLTextParser::WHOLE ||
					part == (S32)LLTextParser::START)
				{
					part = (S32)LLTextParser::START;
				}
				else
				{
					part = (S32)LLTextParser::MIDDLE;
				}
				append_substr(0, start);
			}
			auto url = match.getUrl();
			const auto& label = match.getLabel();
			if (force_replace_links || url == label)
			{
				append_link(label + match.getQuery(), match.getStyle());
				bool tooltip_required = !match.getTooltip().empty();
				if (tooltip_required)
				{
					setLastSegmentToolTip(match.getTooltip());
				}
			}
			else
			{
				const auto pos = text.find(url);
				bool fallback(pos == std::string::npos);
				bool brackets(fallback && text.find('[') == start);
				if (fallback == brackets && start < pos)
					append_substr(start, brackets ? 1 : pos-start);
				if (fallback) url = brackets ? text.substr(start+1, text.find(' ', start+2)-start) : text.substr(start, end-start);
				append_link(url, match.getStyle());
				const auto url_end = pos + url.size();
				if (fallback == brackets && end > url_end)
					append_substr(url_end, end-url_end);
			}
			if (end < (S32)text.length())
			{
				text = text.substr(end, text.length() - end);
				end=0;
				part=(S32)LLTextParser::END;
			}
			else
			{
				break;
			}
		}
		if (part != (S32)LLTextParser::WHOLE)
			part=(S32)LLTextParser::END;
		if (end < (S32)text.length())
			appendAndHighlightText(text, part, style);
	}
	else
	{
		appendAndHighlightText(text, part, style);
	}
}
void LLTextEditor::setLastSegmentToolTip(const std::string &tooltip)
{
	LLTextSegmentPtr segment = mSegments.back();
	segment->setToolTip(tooltip);
}
void LLTextEditor::appendLineBreakSegment()
{
	insertStringNoUndo(getLength(), LLWString(1, '\n'));
}
void LLTextEditor::appendAndHighlightText(const std::string& new_text, S32 highlight_part, const LLStyleSP stylep, bool underline_on_hover)
{
	if (new_text.empty()) return;
	std::string::size_type start = 0;
	std::string str = std::string(new_text,start,new_text.length()-start);
	appendAndHighlightTextImpl(str,highlight_part, stylep, underline_on_hover);
}
void LLTextEditor::replaceUrl(const std::string &url,
							const std::string &label,
							const std::string &icon)
{
	LLWString text = getWText();
	LLWString wlabel = utf8str_to_wstring(label);
	bool modified = false;
	S32 seg_start = 0;
	for (auto it = mSegments.begin(); it != mSegments.end(); ++it)
	{
		LLTextSegment *seg = *it;
		LLStyleSP style = seg->getStyle();
		S32 seg_length = seg->getEnd() - seg->getStart();
		seg->setStart(seg_start);
		seg->setEnd(seg_start + seg_length);
		if (style->getLinkHREF() == url)
		{
			S32 start = seg->getStart();
			S32 end = seg->getEnd();
			const auto& old_label = text.substr(start, end - start);
			if (wlabel != old_label)
			{
				const auto difference = std::abs((S32)wlabel.length() - (S32)old_label.length());
				if (mSelectionEnd >= end)
				{
					mSelectionEnd += difference;
					if (mSelectionStart >= end)
						mSelectionStart += difference;
				}
				if (mCursorPos >= end)
					mCursorPos += difference;
				text.replace(start, end - start, wlabel);
				seg->setEnd(start + wlabel.size());
				modified = true;
			}
		}
		seg_start = seg->getEnd();
	}
	if (modified)
	{
		mWText = text;
		mTextIsUpToDate = FALSE;
		setCursorPos(mCursorPos);
		needsReflow();
	}
}
void LLTextEditor::appendAndHighlightTextImpl(const std::string& new_text, S32 highlight_part, const LLStyleSP stylep, bool underline_on_hover)
{
	BOOL was_scrolled_to_bottom = (mScrollbar->getDocPos() == mScrollbar->getDocPosMax());
	S32 selection_start = mSelectionStart;
	S32 selection_end = mSelectionEnd;
	BOOL was_selecting = mIsSelecting;
	S32 cursor_pos = mCursorPos;
	S32 old_length = getLength();
	BOOL cursor_was_at_end = (mCursorPos == old_length);
	deselect();
	setCursorPos(old_length);
	LLTextParser* highlight = mParseHighlights && stylep && !gDirUtilp->getLindenUserDir(true).empty() ? LLTextParser::getInstance() : nullptr;
	if (highlight)
	{
		const LLSD pieces = highlight->parsePartialLineHighlights(new_text, stylep->getColor(), (LLTextParser::EHighlightPosition)highlight_part);
		auto cur_length = getLength();
		for (auto i = pieces.beginArray(), end = pieces.endArray(); i < end; ++i)
		{
			const auto& piece = *i;
			LLWString wide_text = utf8str_to_wstring(piece["text"].asString());
			insertStringNoUndo(cur_length, wide_text);
			LLStyleSP sp(new LLStyle(*stylep));
			sp->setColor(piece["color"]);
			auto wide_size = wide_text.size();
			LLTextSegmentPtr segmentp = new LLTextSegment(sp, cur_length, cur_length + wide_size);
			cur_length += wide_size;
			if (underline_on_hover) segmentp->setUnderlineOnHover(true);
			mSegments.push_back(segmentp);
		}
	}
	else
	{
		LLWString wide_text = utf8str_to_wstring(new_text);
		auto length = getLength();
		auto insert_len = length + insertStringNoUndo(length, utf8str_to_wstring(new_text));
		if (stylep)
		{
			S32 segment_start = old_length;
			S32 segment_end = old_length + wide_text.size();
			LLTextSegmentPtr segment = new LLTextSegment(stylep, segment_start, segment_end);
			if (underline_on_hover) segment->setUnderlineOnHover(true);
			mSegments.push_back(segment);
		}
		else
		{
			auto segment = mSegments.empty() ? nullptr : mSegments.back();
			if (!segment || !segment->getIsDefault())
			{
				LLColor4& text_color = (mReadOnly ? mReadOnlyFgColor : mFgColor);
				LLTextSegmentPtr segment = new LLTextSegment(text_color, length, insert_len);
				segment->setIsDefault(true);
				mSegments.push_back(segment);
			}
			else
			{
				segment->setEnd(insert_len);
			}
		}
	}
	if( was_scrolled_to_bottom && !was_selecting )
	{
		if( selection_start != selection_end )
		{
			mSelectionStart = selection_start;
			mSelectionEnd = selection_end;
		}
		endOfDoc();
	}
	else if (was_selecting || selection_start != selection_end)
	{
		mSelectionStart = selection_start;
		mSelectionEnd = selection_end;
		mIsSelecting = was_selecting;
		setCursorPos(cursor_pos);
	}
	else if( cursor_was_at_end )
	{
		setCursorPos(getLength());
	}
	else
	{
		setCursorPos(cursor_pos);
	}
}
void LLTextEditor::removeTextFromEnd(S32 num_chars)
{
	if (num_chars <= 0) return;
	num_chars = llclamp(num_chars, 0, getLength());
	remove(getLength() - num_chars, num_chars, FALSE);
	S32 len = getLength();
	mCursorPos = llclamp(mCursorPos, 0, len);
	mSelectionStart = llclamp(mSelectionStart, 0, len);
	mSelectionEnd = llclamp(mSelectionEnd, 0, len);
	pruneSegments();
	updateLineStartList();
	needsScroll();
}
S32 LLTextEditor::insertStringNoUndo(const S32 pos, const LLWString &wstr)
{
	S32 old_len = mWText.length();
	S32 insert_len = wstr.length();
	mWText.insert(pos, wstr);
	mTextIsUpToDate = FALSE;
	if (!mReadOnly && truncate())
	{
		make_ui_sound("UISndBadKeystroke");
		insert_len = mWText.length() - old_len;
	}
	needsReflow();
	return insert_len;
}
S32 LLTextEditor::removeStringNoUndo(S32 pos, S32 length)
{
	auto seg_iter = getSegIterContaining(pos);
	S32 end = pos + length;
	while(seg_iter != mSegments.end())
	{
		LLTextSegmentPtr segmentp = *seg_iter;
		if (segmentp->getStart() < pos)
		{
			if (segmentp->getEnd() > end)
			{
				segmentp->setEnd(segmentp->getEnd() - length);
			}
			else
			{
				segmentp->setEnd(pos);
			}
		}
		else if (segmentp->getStart() < end)
		{
			if (segmentp->getEnd() <= end)
			{
				seg_iter = mSegments.erase(seg_iter);
				continue;
			}
			else
			{
				segmentp->setStart(pos);
				segmentp->setEnd(segmentp->getEnd() - length);
			}
		}
		else
		{
			segmentp->setStart(segmentp->getStart() - length);
			segmentp->setEnd(segmentp->getEnd() - length);
		}
		++seg_iter;
	}
	mWText.erase(pos, length);
	createDefaultSegment();
	mTextIsUpToDate = FALSE;
	updateLineStartList();
	mReflowNeeded = false;
	return -length;
}
S32 LLTextEditor::overwriteCharNoUndo(S32 pos, llwchar wc)
{
	if (pos > (S32)mWText.length())
	{
		return 0;
	}
	mWText[pos] = wc;
	mTextIsUpToDate = FALSE;
	return 1;
}
void LLTextEditor::makePristine()
{
	mPristineCmd = mLastCmd;
	mBaseDocIsPristine = !mLastCmd;
	if( mLastCmd )
	{
		mLastCmd->blockExtensions();
	}
}
BOOL LLTextEditor::isPristine() const
{
	if( mPristineCmd )
	{
		return (mPristineCmd == mLastCmd);
	}
	else
	{
		return !mLastCmd && mBaseDocIsPristine;
	}
}
BOOL LLTextEditor::tryToRevertToPristineState()
{
	if( !isPristine() )
	{
		deselect();
		S32 i = 0;
		while( !isPristine() && canUndo() )
		{
			undo();
			i--;
		}
		while( !isPristine() && canRedo() )
		{
			redo();
			i++;
		}
		if( !isPristine() )
		{
			while( i > 0 )
			{
				undo();
				i--;
			}
		}
		needsReflow();
	}
	return isPristine();
}
void LLTextEditor::updateTextRect()
{
	mTextRect.setOriginAndSize(
		UI_TEXTEDITOR_BORDER + UI_TEXTEDITOR_H_PAD,
		UI_TEXTEDITOR_BORDER,
		getRect().getWidth() - SCROLLBAR_SIZE - 2 * (UI_TEXTEDITOR_BORDER + UI_TEXTEDITOR_H_PAD),
		getRect().getHeight() - 2 * UI_TEXTEDITOR_BORDER - UI_TEXTEDITOR_V_PAD_TOP );
}
void LLTextEditor::loadKeywords(const std::string& filename,
								const std::vector<std::string>& funcs,
								const std::vector<std::string>& tooltips,
								const LLColor3& color)
{
	if(mKeywords.loadFromFile(filename))
	{
		S32 count = llmin(funcs.size(), tooltips.size());
		for(S32 i = 0; i < count; i++)
		{
			std::string name = utf8str_trim(funcs[i]);
			mKeywords.addToken(LLKeywordToken::WORD, name, color, tooltips[i] );
		}
		segment_list_t segment_list;
		mKeywords.findSegments(&segment_list, getWText(), mDefaultColor);
		mSegments.clear();
		segment_list_t::iterator insert_it = mSegments.begin();
		for (segment_list_t::iterator list_it = segment_list.begin(); list_it != segment_list.end(); ++list_it)
		{
			insert_it = mSegments.insert(insert_it, *list_it);
		}
	}
}
static LLTrace::BlockTimerStatHandle FTM_SYNTAX_HIGHLIGHTING("Syntax Highlighting");
static LLTrace::BlockTimerStatHandle FTM_UPDATE_TEXT_SEGMENTS("Update Text Segments");
void LLTextEditor::createDefaultSegment()
{
	if (mSegments.empty())
	{
		LLColor4& text_color = (mReadOnly ? mReadOnlyFgColor : mFgColor);
		LLTextSegmentPtr default_segment = new LLTextSegment(text_color, 0, mWText.length());
		default_segment->setIsDefault(TRUE);
		mSegments.push_back(default_segment);
	}
}
void LLTextEditor::clearSegments()
{
	mLineStartList.clear();
	mSegments.clear();
	createDefaultSegment();
}
void LLTextEditor::updateSegments()
{
	{
		LL_RECORD_BLOCK_TIME(FTM_SYNTAX_HIGHLIGHTING);
		if (mKeywords.isLoaded())
		{
			mKeywords.findSegments(&mSegments, mWText, mDefaultColor);
		}
		else if (mAllowEmbeddedItems)
		{
			findEmbeddedItemSegments();
		}
	}
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_TEXT_SEGMENTS);
	if (mSegments.size() == 1 && mSegments[0]->getIsDefault())
	{
		clearSegments();
	}
	else createDefaultSegment();
}
void LLTextEditor::pruneSegments()
{
	S32 len = mWText.length();
	segment_list_t::iterator iter = mSegments.end();
	while(iter != mSegments.begin())
	{
		--iter;
		LLTextSegment* seg = *iter;
		if (seg->getStart() < len)
		{
			if (seg->getEnd() > len)
			{
				seg->setEnd(len);
			}
			break;
		}
	}
	if (iter != mSegments.end())
	{
		++iter;
		mSegments.erase(iter, mSegments.end());
	}
	else
	{
		LL_WARNS() << "Tried to erase end of empty LLTextEditor" << LL_ENDL;
	}
}
void LLTextEditor::findEmbeddedItemSegments()
{
	mHoverSegment = NULL;
	mSegments.clear();
	BOOL found_embedded_items = FALSE;
	const LLWString &text = mWText;
	S32 idx = 0;
	while( text[idx] )
	{
		if( text[idx] >= FIRST_EMBEDDED_CHAR && text[idx] <= LAST_EMBEDDED_CHAR )
 		{
			found_embedded_items = TRUE;
			break;
		}
		++idx;
	}
	if( !found_embedded_items )
	{
		return;
	}
	S32 text_len = text.length();
	BOOL in_text = FALSE;
	LLColor4& text_color = ( mReadOnly ? mReadOnlyFgColor : mFgColor  );
	if( idx > 0 )
	{
		mSegments.push_back( new LLTextSegment( text_color, 0, text_len ) );
		in_text = TRUE;
	}
	LLStyleSP embedded_style(new LLStyle);
	embedded_style->setIsEmbeddedItem( TRUE );
	while ( text[idx] )
	{
		if( text[idx] >= FIRST_EMBEDDED_CHAR && text[idx] <= LAST_EMBEDDED_CHAR )
		{
			if( in_text )
			{
				mSegments.back()->setEnd( idx );
			}
			mSegments.push_back( new LLTextSegment( embedded_style, idx, idx + 1 ) );
			in_text = FALSE;
		}
		else
		if( !in_text )
		{
			mSegments.push_back( new LLTextSegment( text_color, idx, text_len ) );
			in_text = TRUE;
		}
		++idx;
	}
}
BOOL LLTextEditor::handleMouseUpOverSegment(S32 x, S32 y, MASK mask)
{
	if ( hasMouseCapture() )
	{
		if (mParseHTML)
		{
			if (auto segment = getSegmentAtLocalPos(x, y))
			{
				if (auto style = segment->getStyle())
				{
					if (style->isLink())
						LLUrlAction::clickAction(style->getLinkHREF(), true);
				}
			}
		}
	}
	return FALSE;
}
LLTextEditor::segment_list_t::iterator LLTextEditor::getSegIterContaining(S32 index)
{
	S32 text_len = getLength();
	if (index > text_len) { return mSegments.end(); }
	if (mSegments.size() <= 1) { return mSegments.begin(); }
	LLPointer<LLTextSegment> index_segment = new LLTextSegment(index);
	auto it = std::lower_bound(mSegments.begin(), mSegments.end(), index_segment, LLTextSegment::compare());
	return it;
}
LLTextEditor::segment_list_t::const_iterator LLTextEditor::getSegIterContaining(S32 index) const
{
	S32 text_len = getLength();
	if (index > text_len) { return mSegments.end(); }
	if (mSegments.size() <= 1) { return mSegments.begin(); }
	LLPointer<LLTextSegment> index_segment = new LLTextSegment(index);
	auto it = std::lower_bound(mSegments.begin(), mSegments.end(), index_segment, LLTextSegment::compare());
	return it;
}
LLTextSegment* LLTextEditor::getSegmentAtLocalPos( S32 x, S32 y ) const
{
	S32 offset = getCursorPosFromLocalCoord( x, y, FALSE );
	S32 idx = getSegmentIdxAtOffset(offset);
	return idx >= 0 ? mSegments[idx] : LLTextSegmentPtr();
}
const LLTextSegment* LLTextEditor::getSegmentAtOffset(S32 offset) const
{
	S32 idx = getSegmentIdxAtOffset(offset);
	return idx >= 0 ? mSegments[idx] : LLTextSegmentPtr();
}
S32 LLTextEditor::getSegmentIdxAtOffset(S32 offset) const
{
	if (mSegments.empty() || offset < 0 || offset >= getLength())
	{
		return -1;
	}
	else
	{
		S32 segidx, segoff;
		getSegmentAndOffset(offset, &segidx, &segoff);
		return segidx;
	}
}
void LLTextEditor::onMouseCaptureLost()
{
	endSelection();
}
void LLTextEditor::setOnScrollEndCallback(void (*callback)(void*), void* userdata)
{
	mOnScrollEndCallback = callback;
	mOnScrollEndData = userdata;
	mScrollbar->setOnScrollEndCallback(callback, userdata);
}
LLTextSegment::LLTextSegment(S32 start) :
	mStart(start),
	mEnd(0),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLStyleSP& style, S32 start, S32 end ) :
	mStyle( style ),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor4& color, S32 start, S32 end, BOOL is_visible) :
	mStyle(new LLStyle(is_visible,color,LLStringUtil::null)),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor4& color, S32 start, S32 end ) :
	mStyle(new LLStyle(TRUE, color,LLStringUtil::null )),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor3& color, S32 start, S32 end ) :
	mStyle(new LLStyle(TRUE, color,LLStringUtil::null )),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
BOOL LLTextSegment::getToolTip(std::string& msg) const
{
	if (mToken && !mToken->getToolTip().empty())
	{
		const LLWString& wmsg = mToken->getToolTip();
		msg = wstring_to_utf8str(wmsg);
		return TRUE;
	}
	if (!mTooltip.empty())
	{
		msg = mTooltip;
		return TRUE;
	}
	return FALSE;
}
void LLTextSegment::setToolTip(const std::string& tooltip)
{
	if (mToken)
	{
		LL_WARNS() << "LLTextSegment::setToolTip: cannot replace keyword tooltip." << LL_ENDL;
		return;
	}
	mTooltip = tooltip;
}
void LLTextSegment::dump() const
{
	LL_INFOS() << "Segment [" <<
		mStart << ", " <<
		getEnd() << "]" <<
		LL_ENDL;
}
LLXMLNodePtr LLTextEditor::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();
	node->setName(LL_SIMPLE_TEXT_EDITOR_TAG);
	node->createChild("max_length", TRUE)->setIntValue(getMaxLength());
	node->createChild("embedded_items", TRUE)->setBoolValue(mAllowEmbeddedItems);
	node->createChild("font", TRUE)->setStringValue(LLFontGL::nameFromFont(mGLFont));
	node->createChild("word_wrap", TRUE)->setBoolValue(mWordWrap);
	node->createChild("hide_scrollbar", TRUE)->setBoolValue(mHideScrollbarForShortDocs);
	node->createChild("hide_border", TRUE)->setBoolValue(!mBorder->getVisible());
	addColorXML(node, mCursorColor, "cursor_color", "TextCursorColor");
	addColorXML(node, mFgColor, "text_color", "TextFgColor");
	addColorXML(node, mDefaultColor, "text_default_color", "TextDefaultColor");
	addColorXML(node, mReadOnlyFgColor, "text_readonly_color", "TextFgReadOnlyColor");
	addColorXML(node, mReadOnlyBgColor, "bg_readonly_color", "TextBgReadOnlyColor");
	addColorXML(node, mWriteableBgColor, "bg_writeable_color", "TextBgWriteableColor");
	addColorXML(node, mFocusBgColor, "bg_focus_color", "TextBgFocusColor");
 	node->setStringValue(getText());
	return node;
}
void LLTextEditor::setTextEditorParameters(LLXMLNodePtr node)
{
	BOOL word_wrap = FALSE;
	node->getAttributeBOOL("word_wrap", word_wrap);
	setWordWrap(word_wrap);
	node->getAttributeBOOL("show_line_numbers", mShowLineNumbers);
	node->getAttributeBOOL("track_bottom", mTrackBottom);
	node->getAttributeBOOL("spell_check", mSpellCheckable);
	LLColor4 color;
	if (LLUICtrlFactory::getAttributeColor(node,"cursor_color", color))
	{
		setCursorColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"text_color", color))
	{
		setFgColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"text_readonly_color", color))
	{
		setReadOnlyFgColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"bg_readonly_color", color))
	{
		setReadOnlyBgColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"bg_writeable_color", color))
	{
		setWriteableBgColor(color);
	}
}
void LLTextEditor::updateAllowingLanguageInput()
{
	if (hasFocus() && !mReadOnly)
	{
		getWindow()->allowLanguageTextInput(this, TRUE);
	}
	else
	{
		getWindow()->allowLanguageTextInput(this, FALSE);
	}
}
BOOL LLTextEditor::hasPreeditString() const
{
	return (mPreeditPositions.size() > 1);
}
void LLTextEditor::resetPreedit()
{
	if (hasPreeditString())
	{
		if (hasSelection())
		{
			LL_WARNS() << "Preedit and selection!" << LL_ENDL;
			deselect();
		}
		mCursorPos = mPreeditPositions.front();
		removeStringNoUndo(mCursorPos, mPreeditPositions.back() - mCursorPos);
		insertStringNoUndo(mCursorPos, mPreeditOverwrittenWString);
		mPreeditWString.clear();
		mPreeditOverwrittenWString.clear();
		mPreeditPositions.clear();
	}
}
void LLTextEditor::updatePreedit(const LLWString &preedit_string,
		const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position)
{
	if (mReadOnly)
	{
		return;
	}
	getWindow()->hideCursorUntilMouseMove();
	S32 insert_preedit_at = mCursorPos;
	mPreeditWString = preedit_string;
	mPreeditPositions.resize(preedit_segment_lengths.size() + 1);
	S32 position = insert_preedit_at;
	for (segment_lengths_t::size_type i = 0; i < preedit_segment_lengths.size(); i++)
	{
		mPreeditPositions[i] = position;
		position += preedit_segment_lengths[i];
	}
	mPreeditPositions.back() = position;
	if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		mPreeditOverwrittenWString = getWSubString(insert_preedit_at, mPreeditWString.length());
		removeStringNoUndo(insert_preedit_at, mPreeditWString.length());
	}
	else
	{
		mPreeditOverwrittenWString.clear();
	}
	insertStringNoUndo(insert_preedit_at, mPreeditWString);
	mPreeditStandouts = preedit_standouts;
	needsReflow();
	setCursorPos(insert_preedit_at + caret_position);
	mKeystrokeTimer.reset();
	onKeyStroke();
}
BOOL LLTextEditor::getPreeditLocation(S32 query_offset, LLCoordGL *coord, LLRect *bounds, LLRect *control) const
{
	if (control)
	{
		LLRect control_rect_screen;
		localRectToScreen(mTextRect, &control_rect_screen);
		LLUI::screenRectToGL(control_rect_screen, control);
	}
	S32 preedit_left_position, preedit_right_position;
	if (hasPreeditString())
	{
		preedit_left_position = mPreeditPositions.front();
		preedit_right_position = mPreeditPositions.back();
	}
	else
	{
		preedit_left_position = preedit_right_position = mCursorPos;
	}
	const S32 query = (query_offset >= 0 ? preedit_left_position + query_offset : mCursorPos);
	if (query < preedit_left_position || query > preedit_right_position)
	{
		return FALSE;
	}
	const S32 first_visible_line = mScrollbar->getDocPos();
	if (query < getLineStart(first_visible_line))
	{
		return FALSE;
	}
	S32 current_line = first_visible_line;
	S32 current_line_start, current_line_end;
	for (;;)
	{
		current_line_start = getLineStart(current_line);
		current_line_end = getLineStart(current_line + 1);
		if (query >= current_line_start && query < current_line_end)
		{
			break;
		}
		if (current_line_start == current_line_end)
		{
			break;
		}
		current_line++;
	}
	const llwchar * const text = mWText.c_str();
	const S32 line_height = ll_round(mGLFont->getLineHeight());
	if (coord)
	{
		const S32 query_x = mTextRect.mLeft + mGLFont->getWidth(text, current_line_start, query - current_line_start, mAllowEmbeddedItems);
		const S32 query_y = mTextRect.mTop - (current_line - first_visible_line) * line_height - line_height / 2;
		S32 query_screen_x, query_screen_y;
		localPointToScreen(query_x, query_y, &query_screen_x, &query_screen_y);
		LLUI::screenPointToGL(query_screen_x, query_screen_y, &coord->mX, &coord->mY);
	}
	if (bounds)
	{
		S32 preedit_left = mTextRect.mLeft;
		if (preedit_left_position > current_line_start)
		{
			preedit_left += mGLFont->getWidth(text, current_line_start, preedit_left_position - current_line_start, mAllowEmbeddedItems);
		}
		S32 preedit_right = mTextRect.mLeft;
		if (preedit_right_position < current_line_end)
		{
			preedit_right += mGLFont->getWidth(text, current_line_start, preedit_right_position - current_line_start, mAllowEmbeddedItems);
		}
		else
		{
			preedit_right += mGLFont->getWidth(text, current_line_start, current_line_end - current_line_start, mAllowEmbeddedItems);
		}
		const S32 preedit_top = mTextRect.mTop - (current_line - first_visible_line) * line_height;
		const S32 preedit_bottom = preedit_top - line_height;
		const LLRect preedit_rect_local(preedit_left, preedit_top, preedit_right, preedit_bottom);
		LLRect preedit_rect_screen;
		localRectToScreen(preedit_rect_local, &preedit_rect_screen);
		LLUI::screenRectToGL(preedit_rect_screen, bounds);
	}
	return TRUE;
}
void LLTextEditor::getSelectionRange(S32 *position, S32 *length) const
{
	if (hasSelection())
	{
		*position = llmin(mSelectionStart, mSelectionEnd);
		*length = llabs(mSelectionStart - mSelectionEnd);
	}
	else
	{
		*position = mCursorPos;
		*length = 0;
	}
}
void LLTextEditor::getPreeditRange(S32 *position, S32 *length) const
{
	if (hasPreeditString())
	{
		*position = mPreeditPositions.front();
		*length = mPreeditPositions.back() - mPreeditPositions.front();
	}
	else
	{
		*position = mCursorPos;
		*length = 0;
	}
}
void LLTextEditor::markAsPreedit(S32 position, S32 length)
{
	deselect();
	setCursorPos(position);
	if (hasPreeditString())
	{
		LL_WARNS() << "markAsPreedit invoked when hasPreeditString is true." << LL_ENDL;
	}
	mPreeditWString = LLWString( mWText, position, length );
	if (length > 0)
	{
		mPreeditPositions.resize(2);
		mPreeditPositions[0] = position;
		mPreeditPositions[1] = position + length;
		mPreeditStandouts.resize(1);
		mPreeditStandouts[0] = FALSE;
	}
	else
	{
		mPreeditPositions.clear();
		mPreeditStandouts.clear();
	}
	if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		mPreeditOverwrittenWString = mPreeditWString;
	}
	else
	{
		mPreeditOverwrittenWString.clear();
	}
}
S32 LLTextEditor::getPreeditFontSize() const
{
	return ll_round(mGLFont->getLineHeight() * LLUI::getScaleFactor().mV[VY]);
}
void LLTextEditor::setKeystrokeCallback(const keystroke_signal_t::slot_type& callback)
{
	mKeystrokeSignal.connect(callback);
}
void LLTextEditor::onKeyStroke()
{
	mKeystrokeSignal(this);
}
