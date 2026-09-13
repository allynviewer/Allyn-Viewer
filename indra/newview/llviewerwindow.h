/** 
 * @file llviewerwindow.h
 * @brief Description of the LLViewerWindow class.
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
#ifndef LL_LLVIEWERWINDOW_H
#define LL_LLVIEWERWINDOW_H
#include "v3dmath.h"
#include "v2math.h"
#include "llcursortypes.h"
#include "llpanel.h"
#include "llwindowcallbacks.h"
#include "lltimer.h"
#include "llstat.h"
#include "llmousehandler.h"
#include "llmousehandler.h"
#include "llhandle.h"
#include "llnotifications.h"
class LLView;
class LLViewerObject;
class LLUUID;
class LLProgressView;
class LLTool;
class LLVelocityBar;
class LLTextBox;
class LLImageRaw;
class LLImageFormatted;
class LLHUDIcon;
class LLWindow;
class AIFilePicker;
#define PICK_HALF_WIDTH 5
#define PICK_DIAMETER (2 * PICK_HALF_WIDTH + 1)
class LLPickInfo
{
public:
	typedef enum e_pick_type
	{
		PICK_OBJECT,
		PICK_FLORA,
		PICK_LAND,
		PICK_ICON,
		PICK_PARCEL_WALL,
		PICK_INVALID
	} EPickType;
public:
	LLPickInfo();
	LLPickInfo(const LLCoordGL& mouse_pos,
		MASK keyboard_mask,
		BOOL pick_transparent,
		BOOL pick_rigged,
		BOOL pick_particle,
		BOOL pick_surface_info,
		BOOL pick_unselectable,
		void (*pick_callback)(const LLPickInfo& pick_info));
	void fetchResults();
	LLPointer<LLViewerObject> getObject() const;
	LLUUID getObjectID() const { return mObjectID; }
	bool isValid() const { return mPickType != PICK_INVALID; }
	static bool isFlora(LLViewerObject* object);
public:
	LLCoordGL		mMousePt;
	MASK			mKeyMask;
	void			(*mPickCallback)(const LLPickInfo& pick_info);
	EPickType		mPickType;
	LLCoordGL		mPickPt;
	LLVector3d		mPosGlobal;
	LLVector3		mObjectOffset;
	LLUUID			mObjectID;
	LLUUID			mParticleOwnerID;
	LLUUID			mParticleSourceID;
	S32				mObjectFace;
	LLHUDIcon*		mHUDIcon;
	LLVector3       mIntersection;
	LLVector2		mUVCoords;
	LLVector2       mSTCoords;
	LLCoordScreen	mXYCoords;
	LLVector3		mNormal;
	LLVector4		mTangent;
	LLVector3		mBinormal;
	BOOL			mPickTransparent;
	BOOL			mPickRigged;
	BOOL			mPickParticle;
	BOOL			mPickUnselectable;
	void		    getSurfaceInfo();
private:
	void			updateXYCoords();
	BOOL			mWantSurfaceInfo;
};
static const U32 MAX_SNAPSHOT_IMAGE_SIZE = 6 * 1024;
class LLViewerWindow : public LLWindowCallbacks
{
public:
	LLViewerWindow(const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height, BOOL fullscreen, BOOL ignore_pixel_depth);
	virtual ~LLViewerWindow();
	void			shutdownViews();
	void			shutdownGL();
	void			initGLDefaults();
	void			initBase();
	void			adjustRectanglesForFirstUse(const LLRect& window);
	void            adjustControlRectanglesForFirstUse(const LLRect& window);
	void			initWorldUI();
	void			initWorldUI_postLogin();
	BOOL handleTranslatedKeyDown(KEY key,  MASK mask, BOOL repeated);
	BOOL handleTranslatedKeyUp(KEY key,  MASK mask);
	void handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);
	BOOL handleUnicodeChar(llwchar uni_char, MASK mask);
	BOOL handleAnyMouseClick(LLWindow *window,  LLCoordGL pos, MASK mask, LLMouseHandler::EClickType clicktype, BOOL down);
	BOOL handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	BOOL handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	BOOL handleCloseRequest(LLWindow *window);
	void handleQuit(LLWindow *window);
	BOOL handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	BOOL handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	BOOL handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	BOOL handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	LLWindowCallbacks::DragNDropResult handleDragNDrop(LLWindow *window, LLCoordGL pos, MASK mask, LLWindowCallbacks::DragNDropAction action, std::string data);
				void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
	void handleMouseLeave(LLWindow *window);
	void handleResize(LLWindow *window,  S32 x,  S32 y);
	void handleFocus(LLWindow *window);
	void handleFocusLost(LLWindow *window);
	BOOL handleActivate(LLWindow *window, BOOL activated);
	BOOL handleActivateApp(LLWindow *window, BOOL activating);
	void handleMenuSelect(LLWindow *window,  S32 menu_item);
	BOOL handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
	void handleScrollWheel(LLWindow *window,  S32 clicks);
	BOOL handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);
	void handleWindowBlock(LLWindow *window);
	void handleWindowUnblock(LLWindow *window);
	void handleDataCopy(LLWindow *window, S32 data_type, void *data);
	BOOL handleTimerEvent(LLWindow *window);
	BOOL handleDeviceChange(LLWindow *window);
	bool handleDPIScaleChange(LLWindow *window, float xDPIScale, float yDPIScale, U32 width = 0, U32 height = 0);
	void handlePingWatchdog(LLWindow *window, const char * msg);
	void handlePauseWatchdog(LLWindow *window);
	void handleResumeWatchdog(LLWindow *window);
	std::string translateString(const char* tag);
	std::string translateString(const char* tag,
					const std::map<std::string, std::string>& args);
	LLView*			getRootView()		const	{ return mRootView; }
	const LLRect&	getWindowRect()		const	{ return mWindowRectRaw; };
	S32				getWindowDisplayHeight()	const;
	S32				getWindowDisplayWidth()	const;
	const LLRect&	getWindowRectRaw()			const	{ return getWindowRect(); }
	S32				getWindowHeightRaw()		const	{ return getWindowDisplayHeight(); }
	S32				getWindowWidthRaw()			const	{ return getWindowDisplayWidth(); }
	const LLRect&	getWorldViewRectRaw()		const	{ return getWindowRect(); }
	S32				getWorldViewHeightRaw()		const	{ return getWindowDisplayHeight(); }
	S32				getWorldViewWidthRaw()		const	{ return getWindowDisplayWidth(); }
	const LLRect&	getVirtualWindowRect()		const	{ return mWindowRectScaled; };
	S32				getWindowHeight()	const;
	S32				getWindowWidth()	const;
	const LLRect&	getWindowRectScaled()		const	{ return getVirtualWindowRect(); }
	S32				getWindowHeightScaled()		const	{ return getWindowHeight(); };
	S32				getWindowWidthScaled()		const	{ return getWindowWidth(); };
	const LLRect&	getWorldViewRectScaled()	const	{ return getVirtualWindowRect(); }
	S32				getWorldViewHeightScaled()	const	{ return getWindowHeight(); };
	S32				getWorldViewWidthScaled()	const	{ return getWindowWidth(); };
	LLWindow*		getWindow()			const	{ return mWindow; }
	void*			getPlatformWindow() const;
	void*			getMediaWindow() 	const;
	void			focusClient()		const;
	LLCoordGL		getLastMouse()		const	{ return mLastMousePoint; }
	S32				getLastMouseX()		const	{ return mLastMousePoint.mX; }
	S32				getLastMouseY()		const	{ return mLastMousePoint.mY; }
	LLCoordGL		getCurrentMouse()		const	{ return mCurrentMousePoint; }
	S32				getCurrentMouseX()		const	{ return mCurrentMousePoint.mX; }
	S32				getCurrentMouseY()		const	{ return mCurrentMousePoint.mY; }
	S32				getCurrentMouseDX()		const	{ return mCurrentMouseDelta.mX; }
	S32				getCurrentMouseDY()		const	{ return mCurrentMouseDelta.mY; }
	LLCoordGL		getCurrentMouseDelta()	const	{ return mCurrentMouseDelta; }
	LLStat*			getMouseVelocityStat()		{ return &mMouseVelocityStat; }
	BOOL			getLeftMouseDown()	const	{ return mLeftMouseDown; }
	BOOL			getMiddleMouseDown()	const	{ return mMiddleMouseDown; }
	BOOL			getRightMouseDown()	const	{ return mRightMouseDown; }
	const LLPickInfo&	getLastPick() const { return mLastPick; }
	void			setup2DViewport(S32 x_offset = 0, S32 y_offset = 0);
	void			setup3DViewport(S32 x_offset = 0, S32 y_offset = 0);
	void			setup3DRender();
	void			setup2DRender();
	LLVector3		mouseDirectionGlobal(const S32 x, const S32 y) const;
	LLVector3		mouseDirectionCamera(const S32 x, const S32 y) const;
	LLVector3       mousePointHUD(const S32 x, const S32 y) const;
	BOOL			getActive() const			{ return mActive; }
	void			getTargetWindow(BOOL& fullscreen, S32& width, S32& height) const;
	const std::string&	getInitAlert() { return mInitAlert; }
	void			saveLastMouse(const LLCoordGL &point);
	void			setCursor( ECursorType c );
	void			showCursor();
	void			hideCursor();
	BOOL            getCursorHidden() { return mCursorHidden; }
	void			moveCursorToCenter();
	void			setShowProgress(const BOOL show);
	BOOL			getShowProgress() const;
	void			setProgressString(const std::string& string);
	void			setProgressPercent(const F32 percent);
	void			setProgressMessage(const std::string& msg);
	void			setProgressCancelButtonVisible( BOOL b, const std::string& label = LLStringUtil::null );
	LLProgressView *getProgressView() const;
	void			revealIntroPanel();
	void			abortShowProgress();
	void			setStartupComplete();
	void			updateObjectUnderCursor();
	void			updateUI();
	void				updateLayout();
	void				updateMouseDelta();
	void				updateKeyboardFocus();
	BOOL			handleKey(KEY key, MASK mask);
	BOOL			handleKeyUp(KEY key, MASK mask);
	void			handleScrollWheel	(S32 clicks);
	void			setNormalControlsVisible( BOOL visible );
	void			setMenuBackgroundColor(bool god_mode = false, bool dev_grid = false);
	void			reshape(S32 width, S32 height);
	void			sendShapeToSim();
	void			draw();
	void			updateDebugText();
	void			drawDebugText();
	static void		loadUserImage(void **cb_data, const LLUUID &uuid);
	static void		movieSize(S32 new_width, S32 new_height);
	enum ESnapshotType
	{
		SNAPSHOT_TYPE_COLOR,
		SNAPSHOT_TYPE_DEPTH
	};
	BOOL			saveSnapshot(const std::string&  filename, S32 image_width, S32 image_height, BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR);
	bool			rawRawSnapshot(LLImageRaw* raw, S32 image_width, S32 image_height, F32 aspect,
								BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR,
								S32 max_size = MAX_SNAPSHOT_IMAGE_SIZE, F32 supersample = 1.f, bool uncrop = false);
	bool			rawSnapshot(LLImageRaw* raw, S32 image_width, S32 image_height, F32 aspect,
								BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR,
								S32 max_size = MAX_SNAPSHOT_IMAGE_SIZE, F32 supersample = 1.f);
	BOOL			thumbnailSnapshot(LLImageRaw *raw, S32 preview_width, S32 preview_height, BOOL show_ui, BOOL do_rebuild, ESnapshotType type) ;
	BOOL			isSnapshotLocSet() const { return ! sSnapshotDir.empty(); }
	void			resetSnapshotLoc() const { sSnapshotDir.clear(); }
	void saveImageNumbered(LLPointer<LLImageFormatted> image, int index);
	void saveImageNumbered_continued1(LLPointer<LLImageFormatted> image, std::string const& extension, AIFilePicker* filepicker, int index);
	void saveImageNumbered_continued2(LLPointer<LLImageFormatted> image, std::string const& extension, int index);
	void resetSnapshotLoc();
	void			playSnapshotAnimAndSound();
	void			renderSelections( BOOL for_gl_pick, BOOL pick_parcel_walls, BOOL for_hud );
	void			performPick();
	void			returnEmptyPicks();
	void			pickAsync(	S32 x,
								S32 y_from_bot,
								MASK mask,
								void (*callback)(const LLPickInfo& pick_info),
								BOOL pick_transparent = FALSE,
								BOOL pick_rigged = FALSE,
								BOOL pick_unselectable = FALSE,
								BOOL get_surface_info = FALSE);
	LLPickInfo		pickImmediate(S32 x, S32 y, BOOL pick_transparent, BOOL pick_rigged = FALSE, BOOL pick_particle = FALSE);
	static void     hoverPickCallback(const LLPickInfo& pick_info);
	LLHUDIcon* cursorIntersectIcon(S32 mouse_x, S32 mouse_y, F32 depth,
										   LLVector4a* intersection);
	LLViewerObject* cursorIntersect(S32 mouse_x = -1, S32 mouse_y = -1, F32 depth = 512.f,
									LLViewerObject *this_object = NULL,
									S32 this_face = -1,
									BOOL pick_transparent = FALSE,
									BOOL pick_rigged = FALSE,
									S32* face_hit = NULL,
									LLVector4a *intersection = NULL,
									LLVector2 *uv = NULL,
									LLVector4a *normal = NULL,
									LLVector4a *tangent = NULL,
									LLVector4a* start = NULL,
									LLVector4a* end = NULL);
	BOOL			mousePointOnLandGlobal(const S32 x, const S32 y, LLVector3d *land_pos_global, BOOL ignore_distance = FALSE);
	BOOL			mousePointOnPlaneGlobal(LLVector3d& point, const S32 x, const S32 y, const LLVector3d &plane_point, const LLVector3 &plane_normal);
	LLVector3d		clickPointInWorldGlobal(const S32 x, const S32 y_from_bot, LLViewerObject* clicked_object) const;
	BOOL			clickPointOnSurfaceGlobal(const S32 x, const S32 y, LLViewerObject *objectp, LLVector3d &point_global) const;
	void			dumpState();
	void			toggleFullscreen(BOOL show_progress);
	void			requestResolutionUpdate(bool fullscreen_checked);
	BOOL			checkSettings();
	void			restartDisplay(BOOL show_progress_bar);
	BOOL			changeDisplaySettings(BOOL fullscreen, LLCoordScreen size, const S32 vsync_mode, BOOL show_progress_bar);
	BOOL			getIgnoreDestroyWindow() { return mIgnoreActivate; }
	F32				getDisplayAspectRatio() const;
	const LLVector2& getDisplayScale() const { return mDisplayScale; }
	void			calcDisplayScale();
	LLVector2		getUIScale() const;
private:
	bool                    shouldShowToolTipFor(LLMouseHandler *mh);
	static bool onAlert(const LLSD& notify);
	void			destroyWindow();
	void			drawMouselookInstructions();
	void			stopGL(BOOL save_state = TRUE);
	void			restoreGLState();
	void			restoreGL(bool full_restore, const std::string& progress_message = LLStringUtil::null);
	void			initFonts(F32 zoom_factor = 1.f);
	void			schedulePick(LLPickInfo& pick_info);
	S32				getChatConsoleBottomPad();
	LLRect			getChatConsoleRect();
public:
	void			unblockToolTips(){mToolTipBlocked = FALSE;}
protected:
	LLWindow*		mWindow;
	bool			mActive;
	BOOL			mWantFullscreen;
	BOOL			mShowFullscreenProgress;
	LLRect			mWindowRectRaw;
	LLRect			mWindowRectScaled;
	LLView*			mRootView;
	LLVector2		mDisplayScale;
	LLCoordGL		mCurrentMousePoint;
	LLCoordGL		mLastMousePoint;
	LLCoordGL		mCurrentMouseDelta;
	LLStat			mMouseVelocityStat;
	BOOL			mLeftMouseDown;
	BOOL			mMiddleMouseDown;
	BOOL			mRightMouseDown;
	LLProgressView	*mProgressView;
	LLTextBox*		mToolTip;
	BOOL			mToolTipBlocked;
	LLRect			mToolTipStickyRect;
	BOOL			mMouseInWindow;
	BOOL			mFocusCycleMode;
	typedef std::set<LLHandle<LLView> > view_handle_set_t;
	view_handle_set_t mMouseHoverViews;
	MASK			mLastMask;
	LLTool*			mToolStored;
	BOOL			mHideCursorPermanent;
	BOOL            mCursorHidden;
	LLPickInfo		mLastPick;
	std::vector<LLPickInfo> mPicks;
	LLRect			mPickScreenRegion;
	LLTimer         mPickTimer;
	std::string		mOverlayTitle;
	BOOL			mIgnoreActivate;
	std::string		mInitAlert;
	class LLDebugText* mDebugText;
	bool			mResDirty;
	bool			mIsFullscreenChecked;
	U32			mCurrResolutionIndex;
	float       mDPIScaleX;
	float		mDPIScaleY;
protected:
	static std::string sSnapshotBaseName;
	static std::string sSnapshotDir;
	static std::string sMovieBaseName;
	LLPointer<LLViewerObject>	mDragHoveredObject;
};
class LLBottomPanel : public LLPanel
{
public:
	LLBottomPanel(const LLRect& rect);
	void setFocusIndicator(LLView * indicator);
	LLView * getFocusIndicator() { return mIndicator; }
	void draw();
	static void* createHUD(void* data);
	static void* createOverlayBar(void* data);
	static void* createToolBar(void* data);
protected:
	LLView * mIndicator;
};
extern LLBottomPanel * gBottomPanel;
void toggle_flying(void*);
void toggle_first_person();
void toggle_build(void*);
void reset_viewer_state_on_sim(void);
void update_saved_window_size(const std::string& control,S32 delta_width, S32 delta_height);
extern LLVelocityBar*	gVelocityBar;
extern LLViewerWindow*	gViewerWindow;
extern LLFrameTimer		gMouseIdleTimer;
extern LLFrameTimer		gAwayTimer;
extern LLFrameTimer		gAwayTriggerTimer;
extern LLViewerObject*  gDebugRaycastObject;
extern LLVector4a       gDebugRaycastIntersection;
extern LLVector2        gDebugRaycastTexCoord;
extern LLVector4a       gDebugRaycastNormal;
extern LLVector4a       gDebugRaycastTangent;
extern S32				gDebugRaycastFaceHit;
extern LLVector4a		gDebugRaycastStart;
extern LLVector4a		gDebugRaycastEnd;
extern BOOL			gDisplayCameraPos;
extern BOOL			gDisplayWindInfo;
extern BOOL			gDisplayNearestWater;
extern BOOL			gDisplayFOV;
extern S32 CHAT_BAR_HEIGHT;
#endif
