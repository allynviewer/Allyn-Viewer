/** 
 * @file llwindowheadless.h
 * @brief Headless definition of LLWindow class
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
#ifndef LL_LLWINDOWHEADLESS_H
#define LL_LLWINDOWHEADLESS_H
#include "llwindow.h"
class LLWindowHeadless : public LLWindow
{
public:
	void show(bool) {};
	void hide() {};
	void close() {};
	BOOL getVisible() {return FALSE;};
	BOOL getMinimized() {return FALSE;};
	BOOL getMaximized() {return FALSE;};
	BOOL maximize() {return FALSE;};
	void minimize() {};
	void restore() {};
	BOOL getFullscreen() {return FALSE;};
	BOOL getPosition(LLCoordScreen *position) {return FALSE;};
	BOOL getSize(LLCoordScreen *size) {return FALSE;};
	BOOL getSize(LLCoordWindow *size) {return FALSE;};
	BOOL setPosition(LLCoordScreen position) {return FALSE;};
	BOOL setSizeImpl(LLCoordScreen size) {return FALSE;};
	BOOL setSizeImpl(LLCoordWindow size) {return FALSE;};
	BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, const S32 vsync_mode, std::function<void()> stopFn, std::function<void(bool)> restoreFn, const LLCoordScreen * const posp = NULL) {return FALSE;};
	BOOL setCursorPosition(LLCoordWindow position) {return FALSE;};
	BOOL getCursorPosition(LLCoordWindow *position) {return FALSE;};
	void showCursor() {};
	void hideCursor() {};
	void showCursorFromMouseMove() {};
	void hideCursorUntilMouseMove() {};
	BOOL isCursorHidden() {return FALSE;};
	void updateCursor() {};
	void captureMouse() {};
	void releaseMouse() {};
	void setMouseClipping( BOOL b ) {};
	BOOL isClipboardTextAvailable() {return FALSE; };
	BOOL pasteTextFromClipboard(LLWString &dst) {return FALSE; };
	BOOL copyTextToClipboard(const LLWString &src) {return FALSE; };
	void flashIcon(F32 seconds) {};
	F32 getGamma() {return 1.0f; };
	BOOL setGamma(const F32 gamma) {return FALSE; };
	void setFSAASamples(const U32 fsaa_samples) { }
	U32 getFSAASamples() { return 0; }
	void setVsyncMode(const S32 vsync_mode) {}
	S32	 getVsyncMode() { return 0; }
	BOOL restoreGamma() {return FALSE; };
	void gatherInput() {};
	void delayInputProcessing() {};
	void swapBuffers();
	BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to) { return FALSE; };
	BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to) { return FALSE; };
	BOOL convertCoords(LLCoordWindow from, LLCoordGL *to) { return FALSE; };
	BOOL convertCoords(LLCoordGL from, LLCoordWindow *to) { return FALSE; };
	BOOL convertCoords(LLCoordScreen from, LLCoordGL *to) { return FALSE; };
	BOOL convertCoords(LLCoordGL from, LLCoordScreen *to) { return FALSE; };
	LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) { return NULL; };
	F32	getNativeAspectRatio() { return 1.0f; };
	F32 getPixelAspectRatio() { return 1.0f; };
	void setNativeAspectRatio(F32 ratio) {}
	void *getPlatformWindow() { return 0; };
	void bringToFront() {};
	LLWindowHeadless(LLWindowCallbacks* callbacks,
		const std::string& title, const std::string& name,
		S32 x, S32 y,
		S32 width, S32 height,
		U32 flags,  BOOL fullscreen, BOOL clear_background,
		const S32 vsync_mode, BOOL ignore_pixel_depth);
	virtual ~LLWindowHeadless();
private:
};
class LLSplashScreenHeadless : public LLSplashScreen
{
public:
	LLSplashScreenHeadless() {};
	virtual ~LLSplashScreenHeadless() {};
	void showImpl() {};
	void updateImpl(const std::string& mesg) {};
	void hideImpl() {};
};
#endif
