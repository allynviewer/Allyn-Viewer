/** 
 * @file llwindowwin32.h
 * @brief Windows implementation of LLWindow class
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
#ifndef LL_LLWINDOWWIN32_H
#define LL_LLWINDOWWIN32_H
#include "llwin32headerslean.h"
#include "llwindow.h"
#include "llwindowcallbacks.h"
#include "lldragdropwin32.h"
#define LL_WM_HOST_RESOLVED      (WM_APP + 1)
typedef void (*LLW32MsgCallback)(const MSG &msg);
class LLWindowWin32 : public LLWindow
{
public:
	void postInitialized();
	void show(bool focus = true);
	void hide();
	void close();
	BOOL getVisible();
	BOOL getMinimized();
	BOOL getMaximized();
	BOOL maximize();
	void minimize();
	void restore();
	BOOL getFullscreen();
	BOOL getPosition(LLCoordScreen *position);
	BOOL getSize(LLCoordScreen *size);
	BOOL getSize(LLCoordWindow *size);
	BOOL setPosition(LLCoordScreen position);
	BOOL setSizeImpl(LLCoordScreen size);
	BOOL setSizeImpl(LLCoordWindow size);
	BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, const S32 vsync_mode, std::function<void()> stopFn, std::function<void(bool)> restoreFn, const LLCoordScreen * const posp = NULL);
	BOOL setCursorPosition(LLCoordWindow position);
	BOOL getCursorPosition(LLCoordWindow *position);
	void showCursor();
	void hideCursor();
	void showCursorFromMouseMove();
	void hideCursorUntilMouseMove();
	BOOL isCursorHidden();
	void updateCursor();
	ECursorType getCursor() const;
	void captureMouse();
	void releaseMouse();
	void setMouseClipping( BOOL b );
	BOOL isClipboardTextAvailable();
	BOOL pasteTextFromClipboard(LLWString &dst);
	BOOL copyTextToClipboard(const LLWString &src);
	void flashIcon(F32 seconds);
	F32 getGamma();
	BOOL setGamma(const F32 gamma);
	void setFSAASamples(const U32 fsaa_samples);
	U32 getFSAASamples();
	void setVsyncMode(const S32 vsync_mode);
	S32	 getVsyncMode();
	BOOL restoreGamma();
	ESwapMethod getSwapMethod() { return mSwapMethod; }
	void gatherInput();
	void delayInputProcessing();
	void swapBuffers();
	BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to);
	BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to);
	BOOL convertCoords(LLCoordWindow from, LLCoordGL *to);
	BOOL convertCoords(LLCoordGL from, LLCoordWindow *to);
	BOOL convertCoords(LLCoordScreen from, LLCoordGL *to);
	BOOL convertCoords(LLCoordGL from, LLCoordScreen *to);
	LLWindowResolution* getSupportedResolutions(S32 &num_resolutions);
	F32	getNativeAspectRatio();
	F32 getPixelAspectRatio();
	void setNativeAspectRatio(F32 ratio) { mOverrideAspectRatio = ratio; }
	BOOL dialogColorPicker(F32 *r, F32 *g, F32 *b );
	void *getPlatformWindow();
	void bringToFront();
	void focusClient();
	void allowLanguageTextInput(LLPreeditor *preeditor, BOOL b);
	void setLanguageTextInput( const LLCoordGL & pos );
	void updateLanguageTextInputArea();
	void interruptLanguageTextInput();
	void spawnWebBrowser(const std::string& escaped_url, bool async);
	void setTitle(const std::string &title);
	LLWindowCallbacks::DragNDropResult completeDragNDropRequest( const LLCoordGL gl_coord, const MASK mask, LLWindowCallbacks::DragNDropAction action, const std::string url );
	static std::vector<std::string> getDynamicFallbackFontList();
protected:
	LLWindowWin32(LLWindowCallbacks* callbacks,
		const std::string& title, const std::string& name, int x, int y, int width, int height, U32 flags,
		BOOL fullscreen, BOOL clearBg, const S32 vsync_mode,
		BOOL ignore_pixel_depth, U32 fsaa_samples);
	~LLWindowWin32();
	void	initCursors();
	void	initInputDevices();
	void    initDPIAwareness();
	void    getDPIScales(float& xDPIScale, float& yDPIScale);
	HCURSOR loadColorCursor(LPCTSTR name);
	BOOL	isValid();
	void	moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);
	virtual LLSD	getNativeKeyData();
	BOOL	setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);
	BOOL	setFullscreenResolution();
	BOOL	resetDisplayResolution();
	BOOL	shouldPostQuit() { return mPostQuit; }
	void	fillCompositionForm(const LLRect& bounds, COMPOSITIONFORM *form);
	void	fillCandidateForm(const LLCoordGL& caret, const LLRect& bounds, CANDIDATEFORM *form);
	void	fillCharPosition(const LLCoordGL& caret, const LLRect& bounds, const LLRect& control, IMECHARPOSITION *char_position);
	void	fillCompositionLogfont(LOGFONT *logfont);
	U32		fillReconvertString(const LLWString &text, S32 focus, S32 focus_length, RECONVERTSTRING *reconvert_string);
	void	handleStartCompositionMessage();
	void	handleCompositionMessage(U32 indexes);
	BOOL	handleImeRequests(WPARAM request, LPARAM param, LRESULT *result);
protected:
	BOOL	getClientRectInScreenSpace(RECT* rectp);
	void 	updateJoystick( );
	static LRESULT CALLBACK mainWindowProc(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
	static BOOL CALLBACK enumChildWindows(HWND h_wnd, LPARAM l_param);
	WCHAR		*mWindowTitle;
	WCHAR		*mWindowClassName;
	HWND		mWindowHandle;
	HGLRC		mhRC;
	HDC			mhDC;
	HINSTANCE	mhInstance;
	WNDPROC		mWndProc;
	RECT		mOldMouseClip;
	WPARAM		mLastSizeWParam;
	F32			mOverrideAspectRatio;
	F32			mNativeAspectRatio;
	HCURSOR		mCursor[ UI_CURSOR_COUNT ];
	static BOOL sIsClassRegistered;
	F32			mCurrentGamma;
	U32			mFSAASamples;
	S32			mVsyncMode;
	WORD		mPrevGammaRamp[3][256];
	WORD		mCurrentGammaRamp[3][256];
	BOOL		mCustomGammaSet;
	LPWSTR		mIconResource;
	BOOL		mMousePositionModified;
	BOOL		mInputProcessingPaused;
	static BOOL		sLanguageTextInputAllowed;
	static BOOL		sWinIMEOpened;
	static HKL		sWinInputLocale;
	static DWORD	sWinIMEConversionMode;
	static DWORD	sWinIMESentenceMode;
	static LLCoordWindow sWinIMEWindowPosition;
	LLCoordGL		mLanguageTextInputPointGL;
	LLRect			mLanguageTextInputAreaGL;
	LLPreeditor		*mPreeditor;
	LLDragDropWin32* mDragDrop;
	U32				mKeyCharCode;
	U32				mKeyScanCode;
	U32				mKeyVirtualKey;
	U32				mRawMsg;
	U32				mRawWParam;
	U32				mRawLParam;
	HMODULE         mUser32Lib;
	HMODULE			mSHCoreLib;
	HMONITOR(WINAPI *MonitorFromWindowFn)(HWND, DWORD);
	HRESULT(WINAPI *GetDpiForMonitorFn)(HMONITOR, INT, UINT *, UINT *);
	friend class LLWindowManager;
};
class LLSplashScreenWin32 : public LLSplashScreen
{
public:
	LLSplashScreenWin32();
	virtual ~LLSplashScreenWin32();
	void showImpl();
	void updateImpl(const std::string& mesg);
	void hideImpl();
#if LL_WINDOWS
	static LRESULT CALLBACK windowProc(HWND h_wnd, UINT u_msg,
		WPARAM w_param, LPARAM l_param);
#endif
private:
#if LL_WINDOWS
	HWND mWindow;
#endif
};
extern LLW32MsgCallback gAsyncMsgCallback;
extern LPWSTR gIconResource;
static void	handleMessage( const MSG& msg );
S32 OSMessageBoxWin32(const std::string& text, const std::string& caption, U32 type);
#endif
