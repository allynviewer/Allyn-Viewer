/** 
 * @file llwindowcallbacks.h
 * @brief OS event callback class
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
#ifndef LLWINDOWCALLBACKS_H
#define LLWINDOWCALLBACKS_H
#include "llcoord.h"
class LLWindow;
class LLWindowCallbacks
{
public:
	virtual ~LLWindowCallbacks() {}
	virtual BOOL handleTranslatedKeyDown(KEY key,  MASK mask, BOOL repeated);
	virtual BOOL handleTranslatedKeyUp(KEY key,  MASK mask);
	virtual void handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);
	virtual BOOL handleUnicodeChar(llwchar uni_char, MASK mask);
	virtual BOOL handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleMouseLeave(LLWindow *window);
	virtual BOOL handleCloseRequest(LLWindow *window);
	virtual void handleQuit(LLWindow *window);
	virtual BOOL handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleActivate(LLWindow *window, BOOL activated);
	virtual BOOL handleActivateApp(LLWindow *window, BOOL activating);
	virtual void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleScrollWheel(LLWindow *window,  S32 clicks);
	virtual void handleResize(LLWindow *window,  S32 width,  S32 height);
	virtual void handleFocus(LLWindow *window);
	virtual void handleFocusLost(LLWindow *window);
	virtual void handleMenuSelect(LLWindow *window,  S32 menu_item);
	virtual BOOL handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
	virtual BOOL handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleWindowBlock(LLWindow *window);
	virtual void handleWindowUnblock(LLWindow *window);
	virtual void handleDataCopy(LLWindow *window, S32 data_type, void *data);
	virtual BOOL handleTimerEvent(LLWindow *window);
	virtual BOOL handleDeviceChange(LLWindow *window);
	enum DragNDropAction {
		DNDA_START_TRACKING = 0,
		DNDA_TRACK,
		DNDA_STOP_TRACKING,
		DNDA_DROPPED
	};
	enum DragNDropResult {
		DND_NONE = 0,
		DND_MOVE,
		DND_COPY,
		DND_LINK
	};
	virtual DragNDropResult handleDragNDrop(LLWindow *window, LLCoordGL pos, MASK mask, DragNDropAction action, std::string data);
	virtual bool handleDPIScaleChange(LLWindow *window, float xDPIScale, float yDPIScale, U32 width = 0, U32 height = 0);
	virtual void handlePingWatchdog(LLWindow *window, const char * msg);
	virtual void handlePauseWatchdog(LLWindow *window);
	virtual void handleResumeWatchdog(LLWindow *window);
    virtual std::string translateString(const char* tag);
	virtual std::string translateString(const char* tag,
		const std::map<std::string, std::string>& args);
};
#endif
