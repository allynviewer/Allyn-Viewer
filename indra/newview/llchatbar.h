/** 
 * @file llchatbar.h
 * @brief LLChatBar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LL_LLCHATBAR_H
#define LL_LLCHATBAR_H
#include "llframetimer.h"
#include "llchat.h"
#include "llpanel.h"
class LLLineEditor;
class LLMessageSystem;
class LLUICtrl;
class LLUUID;
class LLFrameTimer;
class LLChatBarGestureObserver;
class LLComboBox;
class LLChatBar final
:	public LLPanel
{
public:
	LLChatBar();
	BOOL		postBuild() override;
	BOOL		handleKeyHere(KEY key, MASK mask) override;
	void		onFocusLost() override;
	void		refresh() override;
	void		refreshGestures();
	void		setKeyboardFocus(BOOL b);
	void		setIgnoreArrowKeys(BOOL b);
	BOOL		inputEditorHasFocus() const;
	std::string	getCurrentChat() const;
	void		setGestureCombo(LLComboBox* combo);
	void		sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);
	void		sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);
	LLWString stripChannelNumber(const LLWString &mesg, S32* channel);
	void onClickSay(LLUICtrl* ctrl);
	void	onInputEditorKeystroke();
	static void	onInputEditorFocusLost();
	static void	onInputEditorGainFocus();
	void onCommitGesture(LLUICtrl* ctrl);
	static void startChat(const char* line);
	static void stopChat();
protected:
	~LLChatBar();
	void sendChat(EChatType type);
	void updateChat();
	LLLineEditor*	mInputEditor;
	LLFrameTimer	mGestureLabelTimer;
	S32				mLastSpecialChatChannel;
	BOOL			mIsBuilt;
	LLComboBox*		mGestureCombo;
	LLChatBarGestureObserver* mObserver;
};
extern LLChatBar *gChatBar;
#endif
