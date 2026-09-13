/** 
 * @file llmodaldialog.h
 * @brief LLModalDialog base class
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
#ifndef LL_LLMODALDIALOG_H
#define LL_LLMODALDIALOG_H
#include "llfloater.h"
#include "llframetimer.h"
class LLModalDialog;
class LLModalDialog : public LLFloater
{
public:
	LLModalDialog( const std::string& title, S32 width, S32 height, BOOL modal = true );
	~LLModalDialog();
	void	open();
	void 	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void	startModal();
	void	stopModal();
	BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL	handleHover(S32 x, S32 y, MASK mask);
	BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	BOOL	handleKeyHere(KEY key, MASK mask );
	void	onClose(bool app_quitting);
	void	setVisible(BOOL visible);
	void	draw();
	BOOL isModal() const { return mModal; }
	static void		onAppFocusLost();
	static void		onAppFocusGained();
	static S32		activeCount() { return sModalStack.size(); }
	static void		shutdownModals();
protected:
	void			centerOnScreen();
private:
	LLFrameTimer 	mVisibleTime;
	const BOOL		mModal;
	static std::list<LLModalDialog*> sModalStack;
};
#endif
