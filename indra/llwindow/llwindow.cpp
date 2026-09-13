/** 
 * @file llwindow.cpp
 * @brief Basic graphical window class
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
#include "linden_common.h"
#include "llwindowheadless.h"
#if LL_MESA_HEADLESS
#include "llwindowmesaheadless.h"
#elif LL_SDL
#include "llwindowsdl.h"
#elif LL_WINDOWS
#include "llwindowwin32.h"
#elif LL_DARWIN
#include "llwindowmacosx.h"
#endif
#include "llerror.h"
#include "llkeyboard.h"
#include "llwindowcallbacks.h"
LLSplashScreen *gSplashScreenp = NULL;
BOOL gDebugClicks = FALSE;
BOOL gDebugWindowProc = FALSE;
bool isWhitelistedProtocol(const std::string& escaped_url) {
	for (const auto& protocol : { "secondlife:", "http:", "https:", "data:", "mailto:" })
		if (escaped_url.find(protocol) != std::string::npos)
			return true;
	return false;
}
S32 OSMessageBox(const std::string& text, const std::string& caption, U32 type)
{
	BOOL was_visible = FALSE;
	if (LLSplashScreen::isVisible())
	{
		was_visible = TRUE;
		LLSplashScreen::hide();
	}
	S32 result = 0;
#if LL_MESA_HEADLESS
	LL_WARNS() << "OSMessageBox: " << text << LL_ENDL;
	result = OSBTN_OK;
#elif LL_WINDOWS
	result = OSMessageBoxWin32(text, caption, type);
#elif LL_DARWIN
	result = OSMessageBoxMacOSX(text, caption, type);
#elif LL_SDL
	result = OSMessageBoxSDL(text, caption, type);
#else
#error("OSMessageBox not implemented for this platform!")
#endif
	if (was_visible)
	{
		LLSplashScreen::show();
	}
	return result;
}
LLWindow::LLWindow(LLWindowCallbacks* callbacks, BOOL fullscreen, U32 flags)
	: mCallbacks(callbacks),
	  mPostQuit(TRUE),
	  mFullscreen(fullscreen),
	  mFullscreenWidth(0),
	  mFullscreenHeight(0),
	  mFullscreenBits(0),
	  mFullscreenRefresh(0),
	  mSupportedResolutions(NULL),
	  mNumSupportedResolutions(0),
	  mCurrentCursor(UI_CURSOR_ARROW),
	  mNextCursor(UI_CURSOR_ARROW),
	  mCursorHidden(FALSE),
	  mBusyCount(0),
	  mIsMouseClipping(FALSE),
	  mMinWindowWidth(0),
	  mMinWindowHeight(0),
	  mSwapMethod(SWAP_METHOD_UNDEFINED),
	  mHideCursorPermanent(FALSE),
	  mFlags(flags),
	  mHighSurrogate(0)
{
}
LLWindow::~LLWindow()
{
}
BOOL LLWindow::isValid()
{
	return TRUE;
}
BOOL LLWindow::canDelete()
{
	return TRUE;
}
void LLWindow::incBusyCount()
{
	++mBusyCount;
}
void LLWindow::decBusyCount()
{
	if (mBusyCount > 0)
	{
		--mBusyCount;
	}
}
void LLWindow::resetBusyCount()
{
	mBusyCount = 0;
}
S32 LLWindow::getBusyCount() const
{
	return mBusyCount;
}
ECursorType LLWindow::getCursor() const
{
	return mCurrentCursor;
}
BOOL LLWindow::dialogColorPicker(F32 *r, F32 *g, F32 *b)
{
	return FALSE;
}
void *LLWindow::getMediaWindow()
{
	return getPlatformWindow();
}
BOOL LLWindow::setSize(LLCoordScreen size)
{
	if (!getMaximized())
	{
		size.mX = llmax(size.mX, mMinWindowWidth);
		size.mY = llmax(size.mY, mMinWindowHeight);
	}
	return setSizeImpl(size);
}
BOOL LLWindow::setSize(LLCoordWindow size)
{
	if (!getMaximized())
	{
		size.mX = llmax(size.mX, mMinWindowWidth);
		size.mY = llmax(size.mY, mMinWindowHeight);
	}
	return setSizeImpl(size);
}
void LLWindow::setMinSize(U32 min_width, U32 min_height, bool enforce_immediately)
{
	mMinWindowWidth = min_width;
	mMinWindowHeight = min_height;
	if (enforce_immediately)
	{
		LLCoordScreen cur_size;
		if (!getMaximized() && getSize(&cur_size))
		{
			if (cur_size.mX < mMinWindowWidth || cur_size.mY < mMinWindowHeight)
			{
				setSizeImpl(LLCoordScreen(llmin(cur_size.mX, mMinWindowWidth), llmin(cur_size.mY, mMinWindowHeight)));
			}
		}
	}
}
void LLWindow::processMiscNativeEvents()
{
}
BOOL LLWindow::isPrimaryTextAvailable()
{
	return FALSE;
}
BOOL LLWindow::pasteTextFromPrimary(LLWString &dst)
{
	return FALSE;
}
BOOL LLWindow::copyTextToPrimary(const LLWString &src)
{
	return FALSE;
}
#if LL_WINDOWS
#include <shellapi.h>
#endif
int LLWindow::ShellEx(const std::string& command)
{
#if LL_WINDOWS
	llutf16string url_utf16 = L'"' + utf8str_to_utf16str(command) + L'"';
	SHELLEXECUTEINFO sei = { sizeof( sei ) };
	sei.fMask = SEE_MASK_NOASYNC;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = L"open";
	sei.lpFile = url_utf16.c_str();
	const auto& code = ShellExecuteEx(&sei) ? 0 : GetLastError();
#elif LL_DARWIN
	CFURLRef urlRef;
	CFStringRef stringRef = CFStringCreateWithCString(NULL, command.c_str(), kCFStringEncodingUTF8);
	if (stringRef)
	{
		urlRef = CFURLCreateWithString(NULL, stringRef, NULL);
		CFRelease(stringRef);
	}
	OSStatus code;
	if (urlRef)
	{
		code = LSOpenCFURLRef(urlRef, NULL);
		CFRelease(urlRef);
	}
	else code = -1;
#else
	const auto& code = std::system(("xdg-open \"" + command + '"').c_str());
#endif
	if (code) LL_WARNS() << "Failed to open \"" << command << "\" return code: " << code << LL_ENDL;
	return code;
}
std::vector<std::string> LLWindow::getDynamicFallbackFontList()
{
#if LL_WINDOWS
	return LLWindowWin32::getDynamicFallbackFontList();
#elif LL_DARWIN
	return LLWindowMacOSX::getDynamicFallbackFontList();
#elif LL_MESA_HEADLESS
	return std::vector<std::string>();
#elif LL_SDL
	return LLWindowSDL::getDynamicFallbackFontList();
#else
	return std::vector<std::string>();
#endif
}
#define UTF16_IS_HIGH_SURROGATE(U) ((U16)((U) - 0xD800) < 0x0400)
#define UTF16_IS_LOW_SURROGATE(U)  ((U16)((U) - 0xDC00) < 0x0400)
#define UTF16_SURROGATE_PAIR_TO_UTF32(H,L) (((H) << 10) + (L) - (0xD800 << 10) - 0xDC00 + 0x00010000)
void LLWindow::handleUnicodeUTF16(U16 utf16, MASK mask)
{
	if (mHighSurrogate == 0)
	{
		if (UTF16_IS_HIGH_SURROGATE(utf16))
		{
			mHighSurrogate = utf16;
		}
		else
		{
			mCallbacks->handleUnicodeChar(utf16, mask);
		}
	}
	else
	{
		if (UTF16_IS_LOW_SURROGATE(utf16))
		{
			mCallbacks->handleUnicodeChar(UTF16_SURROGATE_PAIR_TO_UTF32(mHighSurrogate, utf16), mask);
			mHighSurrogate = 0;
		}
		else if (UTF16_IS_HIGH_SURROGATE(utf16))
		{
			mCallbacks->handleUnicodeChar(mHighSurrogate, mask);
			mHighSurrogate = utf16;
		}
		else
		{
			mCallbacks->handleUnicodeChar(mHighSurrogate, mask);
			mHighSurrogate = 0;
			mCallbacks->handleUnicodeChar(utf16, mask);
		}
	}
}
bool LLSplashScreen::isVisible()
{
	return gSplashScreenp ? true: false;
}
LLSplashScreen *LLSplashScreen::create()
{
#if LL_MESA_HEADLESS || LL_SDL
	return 0;
#elif LL_WINDOWS
	return new LLSplashScreenWin32;
#elif LL_DARWIN
	return new LLSplashScreenMacOSX;
#else
#error("LLSplashScreen not implemented on this platform!")
#endif
}
void LLSplashScreen::show()
{
	if (!gSplashScreenp)
	{
#if LL_WINDOWS && !LL_MESA_HEADLESS
		gSplashScreenp = new LLSplashScreenWin32;
#elif LL_DARWIN
		gSplashScreenp = new LLSplashScreenMacOSX;
#endif
		if (gSplashScreenp)
		{
			gSplashScreenp->showImpl();
		}
	}
}
void LLSplashScreen::update(const std::string& str)
{
	LLSplashScreen::show();
	if (gSplashScreenp)
	{
		gSplashScreenp->updateImpl(str);
	}
}
void LLSplashScreen::hide()
{
	if (gSplashScreenp)
	{
		gSplashScreenp->hideImpl();
	}
	delete gSplashScreenp;
	gSplashScreenp = NULL;
}
static std::set<LLWindow*> sWindowList;
LLWindow* LLWindowManager::createWindow(
	LLWindowCallbacks* callbacks,
	const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height, U32 flags,
	BOOL fullscreen,
	BOOL clearBg,
	const S32 vsync_mode,
	BOOL use_gl,
	BOOL ignore_pixel_depth,
	U32 fsaa_samples)
{
	LLWindow* new_window;
	if (use_gl)
	{
#if LL_MESA_HEADLESS
		new_window = new LLWindowMesaHeadless(callbacks,
			title, name, x, y, width, height, flags,
			fullscreen, clearBg, vsync_mode, ignore_pixel_depth);
#elif LL_SDL
		new_window = new LLWindowSDL(callbacks,
			title, x, y, width, height, flags,
			fullscreen, clearBg, vsync_mode, ignore_pixel_depth, fsaa_samples);
#elif LL_WINDOWS
		new_window = new LLWindowWin32(callbacks,
			title, name, x, y, width, height, flags,
			fullscreen, clearBg, vsync_mode, ignore_pixel_depth, fsaa_samples);
#elif LL_DARWIN
		new_window = new LLWindowMacOSX(callbacks,
			title, name, x, y, width, height, flags,
			fullscreen, clearBg, vsync_mode, ignore_pixel_depth, fsaa_samples);
#endif
	}
	else
	{
		new_window = new LLWindowHeadless(callbacks,
			title, name, x, y, width, height, flags,
			fullscreen, clearBg, vsync_mode, ignore_pixel_depth);
	}
	if (FALSE == new_window->isValid())
	{
		delete new_window;
		LL_WARNS() << "LLWindowManager::create() : Error creating window." << LL_ENDL;
		return NULL;
	}
	sWindowList.insert(new_window);
	return new_window;
}
BOOL LLWindowManager::destroyWindow(LLWindow* window)
{
	if (sWindowList.find(window) == sWindowList.end())
	{
		LL_ERRS() << "LLWindowManager::destroyWindow() : Window pointer not valid, this window doesn't exist!"
			<< LL_ENDL;
		return FALSE;
	}
	window->close();
	sWindowList.erase(window);
	delete window;
	return TRUE;
}
BOOL LLWindowManager::isWindowValid(LLWindow *window)
{
	return sWindowList.find(window) != sWindowList.end();
}
LLCoordCommon LL_COORD_TYPE_WINDOW::convertToCommon() const
{
	const LLCoordWindow& self = LLCoordWindow::getTypedCoords(*this);
	LLWindow* windowp = &(*LLWindow::beginInstances());
	LLCoordGL out;
	windowp->convertCoords(self, &out);
	return out.convert();
}
void LL_COORD_TYPE_WINDOW::convertFromCommon(const LLCoordCommon& from)
{
	LLCoordWindow& self = LLCoordWindow::getTypedCoords(*this);
	LLWindow* windowp = &(*LLWindow::beginInstances());
	LLCoordGL from_gl(from);
	windowp->convertCoords(from_gl, &self);
}
LLCoordCommon LL_COORD_TYPE_SCREEN::convertToCommon() const
{
	const LLCoordScreen& self = LLCoordScreen::getTypedCoords(*this);
	LLWindow* windowp = &(*LLWindow::beginInstances());
	LLCoordGL out;
	windowp->convertCoords(self, &out);
	return out.convert();
}
void LL_COORD_TYPE_SCREEN::convertFromCommon(const LLCoordCommon& from)
{
	LLCoordScreen& self = LLCoordScreen::getTypedCoords(*this);
	LLWindow* windowp = &(*LLWindow::beginInstances());
	LLCoordGL from_gl(from);
	windowp->convertCoords(from_gl, &self);
}
