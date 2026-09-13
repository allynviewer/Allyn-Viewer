/**
 * @file llviewermedia.h
 * @brief Client interface to the media engine
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#ifndef LLVIEWERMEDIA_H
#define LLVIEWERMEDIA_H
#include "llfocusmgr.h"
#include "lleditmenuhandler.h"
#include "llpanel.h"
#include "llpluginclassmediaowner.h"
#include "llviewermediaobserver.h"
#include "llpluginclassmedia.h"
#include "v4color.h"
#include "llnotificationptr.h"
#include "llurl.h"
#include "llviewerpluginmanager.h"
class LLViewerMediaImpl;
class LLUUID;
class LLViewerMediaTexture;
class LLMediaEntry;
class LLVOVolume;
class LLMimeDiscoveryResponder;
typedef LLPointer<LLViewerMediaImpl> viewer_media_t;
class LLViewerMediaEventEmitter
{
public:
	virtual ~LLViewerMediaEventEmitter();
	bool addObserver( LLViewerMediaObserver* subject );
	bool remObserver( LLViewerMediaObserver* subject );
	virtual void emitEvent(LLPluginClassMedia* self, LLViewerMediaObserver::EMediaEvent event);
private:
	typedef std::list< LLViewerMediaObserver* > observerListType;
	observerListType mObservers;
};
class LLViewerMediaImpl;
class LLViewerMedia
{
	LOG_CLASS(LLViewerMedia);
public:
	static const char* AUTO_PLAY_MEDIA_SETTING;
	static const char* AUTO_PLAY_PRIM_MEDIA_SETTING;
	static const char* SHOW_MEDIA_ON_OTHERS_SETTING;
	static const char* SHOW_MEDIA_WITHIN_PARCEL_SETTING;
	static const char* SHOW_MEDIA_OUTSIDE_PARCEL_SETTING;
	typedef std::list<LLViewerMediaImpl*> impl_list;
	typedef std::map<LLUUID, LLViewerMediaImpl*> impl_id_map;
	static viewer_media_t newMediaImpl(const LLUUID& texture_id,
									   S32 media_width = 0,
									   S32 media_height = 0,
									   U8 media_auto_scale = false,
									   U8 media_loop = false);
	static viewer_media_t updateMediaImpl(LLMediaEntry* media_entry, const std::string& previous_url, bool update_from_self);
	static LLViewerMediaImpl* getMediaImplFromTextureID(const LLUUID& texture_id);
	static std::string getCurrentUserAgent();
	static void updateBrowserUserAgent();
	static bool handleSkinCurrentChanged(const LLSD& );
	static bool textureHasMedia(const LLUUID& texture_id);
	static void setVolume(F32 volume);
	static bool isAnyMediaShowing();
	static bool isAnyMediaPlaying();
	static void setAllMediaEnabled(bool val);
	static void setAllMediaPaused(bool val);
	static void updateMedia(void* dummy_arg = nullptr);
	static void initClass();
	static void cleanupClass();
	static F32 getVolume();
	static void muteListChanged();
	static bool isInterestingEnough(const LLVOVolume* object, const F64 &object_interest);
	static impl_list &getPriorityList();
	static bool priorityComparitor(const LLViewerMediaImpl* i1, const LLViewerMediaImpl* i2);
	static bool hasInWorldMedia();
	static std::string getParcelAudioURL();
	static bool hasParcelMedia();
	static bool hasParcelAudio();
	static bool isParcelMediaPlaying();
	static bool isParcelAudioPlaying();
	static void onAuthSubmit(const LLSD& notification, const LLSD& response);
	static void clearAllCookies();
	static void clearAllCaches();
	static void setCookiesEnabled(bool enabled);
	static void setProxyConfig(bool enable, const std::string &host, int port);
	static void openIDSetup(const std::string &openid_url, const std::string &openid_token);
	static void openIDCookieResponse(const std::string &cookie);
	static void proxyWindowOpened(const std::string &target, const std::string &uuid);
	static void proxyWindowClosed(const std::string &uuid);
	static void createSpareBrowserMediaSource();
	static LLPluginClassMedia* getSpareBrowserMediaSource();
	static void setOnlyAudibleMediaTextureID(const LLUUID& texture_id);
	static class AIHTTPHeaders getHeaders();
private:
	static bool parseRawCookie(const std::string raw_cookie, std::string& name, std::string& value, std::string& path, bool& httponly, bool& secure);
	static void setOpenIDCookie();
	static void onTeleportFinished();
    static void getOpenIDCookieCoro(std::string url);
	static LLURL sOpenIDURL;
	static std::string sOpenIDCookie;
	static LLPluginClassMedia* sSpareBrowserMediaSource;
};
class LLViewerMediaImpl
	:	public LLViewerPluginManager, public LLMouseHandler, public LLPluginClassMediaOwner, public LLViewerMediaEventEmitter, public LLEditMenuHandler
{
	LOG_CLASS(LLViewerMediaImpl);
public:
	friend class LLViewerMedia;
	friend class LLMimeDiscoveryResponder;
	LLViewerMediaImpl(
		const LLUUID& texture_id,
		S32 media_width,
		S32 media_height,
		U8 media_auto_scale,
		U8 media_loop);
	~LLViewerMediaImpl();
	void emitEvent(LLPluginClassMedia* self, LLViewerMediaObserver::EMediaEvent event) override;
	void createMediaSource();
	void destroyMediaSource();
	void setMediaType(const std::string& media_type);
	bool initializeMedia(const std::string& mime_type);
	bool initializePlugin(const std::string& media_type);
	void loadURI();
	LLPluginClassMedia* getMediaPlugin() const { return (LLPluginClassMedia*)mPluginBase; }
	void setSize(int width, int height);
	void showNotification(LLNotificationPtr notify);
	void hideNotification();
	void play();
	void stop();
	void pause();
	void start();
	void seek(F32 time);
	void skipBack(F32 step_scale);
	void skipForward(F32 step_scale);
	void setVolume(F32 volume);
	void setMute(bool mute);
	void updateVolume();
	F32 getVolume();
	void focus(bool focus);
	bool hasFocus() const;
	void mouseDown(S32 x, S32 y, MASK mask, S32 button = 0);
	void mouseUp(S32 x, S32 y, MASK mask, S32 button = 0);
	void mouseMove(S32 x, S32 y, MASK mask);
	void mouseDown(const LLVector2& texture_coords, MASK mask, S32 button = 0);
	void mouseUp(const LLVector2& texture_coords, MASK mask, S32 button = 0);
	void mouseMove(const LLVector2& texture_coords, MASK mask);
    void mouseDoubleClick(const LLVector2& texture_coords, MASK mask);
    void mouseDoubleClick(S32 x, S32 y, MASK mask, S32 button = 0);
	void scrollWheel(S32 x, S32 y, S32 scroll_x, S32 scroll_y, MASK mask);
	void mouseCapture();
	void navigateBack();
	void navigateForward();
	void navigateReload();
	void navigateHome();
	void unload();
	void navigateTo(const std::string& url, const std::string& mime_type = "", bool rediscover_type = false, bool server_request = false, bool clean_browser = false);
	void navigateInternal();
	void navigateStop();
	bool handleKeyHere(KEY key, MASK mask);
	bool handleKeyUpHere(KEY key, MASK mask);
	bool handleUnicodeCharHere(llwchar uni_char);
	bool canNavigateForward();
	bool canNavigateBack();
	std::string getMediaURL() const { return mMediaURL; }
	std::string getCurrentMediaURL();
	std::string getHomeURL() { return mHomeURL; }
	std::string getMediaEntryURL() { return mMediaEntryURL; }
	void setHomeURL(const std::string& home_url, const std::string& mime_type = LLStringUtil::null) { mHomeURL = home_url; mHomeMimeType = mime_type;};
	void clearCache();
	void setPageZoomFactor( double factor );
	double getPageZoomFactor() {return mZoomFactor;}
	std::string getMimeType() { return mMimeType; }
	void scaleMouse(S32 *mouse_x, S32 *mouse_y);
	void scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y);
	void update();
	void updateImagesMediaStreams();
	LLUUID getMediaTextureID() const;
	void suspendUpdates(bool suspend) { mSuspendUpdates = suspend; }
	void setVisible(bool visible);
	bool getVisible() const { return mVisible; }
	bool isVisible() const { return mVisible; }
	bool isMediaTimeBased();
	bool isMediaPlaying();
	bool isMediaPaused();
	bool hasMedia() const;
	bool isMediaFailed() const { return mMediaSourceFailed; }
	void setMediaFailed(bool val) { mMediaSourceFailed = val; }
	void resetPreviousMediaState();
	void setDisabled(bool disabled, bool forcePlayOnEnable = false);
	bool isMediaDisabled() const { return mIsDisabled; };
	void setInNearbyMediaList(bool in_list) { mInNearbyMediaList = in_list; }
	bool getInNearbyMediaList() { return mInNearbyMediaList; }
	bool isForcedUnloaded() const;
	bool isPlayable() const;
	void setIsParcelMedia(bool is_parcel_media) { mIsParcelMedia = is_parcel_media; }
	bool isParcelMedia() const { return mIsParcelMedia; }
	ECursorType getLastSetCursor() { return mLastSetCursor; }
	void setTarget(const std::string& target) { mTarget = target; }
	static LLPluginClassMedia* newSourceFromMediaType(std::string media_type, LLPluginClassMediaOwner *owner , S32 default_width, S32 default_height, F64 zoom_factor, const std::string target = LLStringUtil::null, bool clean_browser = false);
	static void updateBrowserUserAgent();
	static bool handleSkinCurrentChanged(const LLSD& newvalue);
	void	onMouseCaptureLost() override;
	BOOL	handleMouseUp(S32 x, S32 y, MASK mask) override;
	BOOL	handleMouseDown(S32 x, S32 y, MASK mask) override { return FALSE; };
	BOOL	handleHover(S32 x, S32 y, MASK mask) override { return FALSE; };
	BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks) override { return FALSE; };
	BOOL	handleDoubleClick(S32 x, S32 y, MASK mask) override { return FALSE; };
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) override { return FALSE; };
	BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask) override { return FALSE; };
	BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen) override { return FALSE; };
	BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask) override { return FALSE; };
	BOOL	handleMiddleMouseUp(S32 x, S32 y, MASK mask) override {return FALSE; };
	const std::string& getName() const override;
	BOOL isView() const  override { return FALSE; };
	void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const override {};
	void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const override {};
	BOOL hasMouseCapture() override { return gFocusMgr.getMouseCapture() == this; };
	void handleMediaEvent(LLPluginClassMedia* plugin, LLPluginClassMediaOwner::EMediaEvent) override;
	void	undo() override;
	BOOL	canUndo() const override;
	void	redo() override;
	BOOL	canRedo() const override;
	void	cut() override;
	BOOL	canCut() const override;
	void	copy() const override final;
	BOOL	canCopy() const override;
	void	paste() override;
	BOOL	canPaste() const override;
	void	doDelete() override;
	BOOL	canDoDelete() const override;
	void	selectAll() override;
	BOOL	canSelectAll() const override;
	void addObject(LLVOVolume* obj) ;
	void removeObject(LLVOVolume* obj) ;
	const std::list< LLVOVolume* >* getObjectList() const ;
	LLVOVolume *getSomeObject();
	void setUpdated(BOOL updated) ;
	BOOL isUpdated() ;
	void updateJavascriptObject();
	void calculateInterest();
	F64 getInterest() const { return mInterest; };
	F64 getApproximateTextureInterest();
	S32 getProximity() const { return mProximity; };
	F64 getProximityDistance() const { return mProximityDistance; };
	void setUsedInUI(bool used_in_ui);
	bool getUsedInUI() const { return mUsedInUI; };
	void setBackgroundColor(LLColor4 color);
	F64 getCPUUsage() const;
	typedef enum
	{
		PRIORITY_UNLOADED,
		PRIORITY_HIDDEN,
		PRIORITY_SLIDESHOW,
		PRIORITY_LOW,
		PRIORITY_NORMAL,
		PRIORITY_HIGH
	}EPriority;
	void setPriority(EPriority priority);
	EPriority getPriority() { return mPriority; };
	void setLowPrioritySizeLimit(int size);
	void setTextureID(LLUUID id = LLUUID::null);
	bool isTrustedBrowser() { return mTrustedBrowser; }
	void setTrustedBrowser(bool trusted) { mTrustedBrowser = trusted; }
	typedef enum
	{
		MEDIANAVSTATE_NONE,
		MEDIANAVSTATE_BEGUN,
		MEDIANAVSTATE_FIRST_LOCATION_CHANGED,
		MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS,
		MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED,
		MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS,
		MEDIANAVSTATE_SERVER_SENT,
		MEDIANAVSTATE_SERVER_BEGUN,
		MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED,
		MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED
	}EMediaNavState;
	EMediaNavState getNavState() { return mMediaNavState; }
	void setNavState(EMediaNavState state);
	void setNavigateSuspended(bool suspend);
	bool isNavigateSuspended() { return mNavigateSuspended; };
	void cancelMimeTypeProbe();
	bool isAttachedToAnotherAvatar() const;
	bool isInAgentParcel() const;
	LLNotificationPtr getCurrentNotification() const;
private:
	bool isAutoPlayable() const;
	bool shouldShowBasedOnClass() const;
	static bool isObjectAttachedToAnotherAvatar(LLVOVolume *obj);
	static bool isObjectInAgentParcel(LLVOVolume *obj);
private:
	F64		mZoomFactor;
	LLUUID mTextureId;
	bool  mMovieImageHasMips;
	std::string mMediaURL;
	std::string mHomeURL;
	std::string mHomeMimeType;
	std::string mMimeType;
	std::string mCurrentMediaURL;
	std::string mCurrentMimeType;
	S32 mLastMouseX;
	S32 mLastMouseY;
	S32 mMediaWidth;
	S32 mMediaHeight;
	bool mMediaAutoScale;
	bool mMediaLoop;
	bool mNeedsNewTexture;
	S32 mTextureUsedWidth;
	S32 mTextureUsedHeight;
	bool mSuspendUpdates;
	bool mVisible;
	ECursorType mLastSetCursor;
	EMediaNavState mMediaNavState;
	F64 mInterest;
	bool mUsedInUI;
	bool mHasFocus;
	EPriority mPriority;
	bool mNavigateRediscoverType;
	bool mNavigateServerRequest;
	bool mMediaSourceFailed;
	F32 mRequestedVolume;
	F32 mPreviousVolume;
	bool mIsMuted;
	bool mNeedsMuteCheck;
	int mPreviousMediaState;
	F64 mPreviousMediaTime;
	bool mIsDisabled;
	bool mIsParcelMedia;
	S32 mProximity;
	F64 mProximityDistance;
	F64 mProximityCamera;
	bool mMediaAutoPlay;
	std::string mMediaEntryURL;
	bool mInNearbyMediaList;
	bool mClearCache;
	LLColor4 mBackgroundColor;
	bool mNavigateSuspended;
	bool mNavigateSuspendedDeferred;
	bool mTrustedBrowser;
	std::string mTarget;
	LLNotificationPtr mNotification;
    bool mCleanBrowser;
    static std::vector<std::string> sMimeTypesFailed;
private:
	BOOL mIsUpdated ;
	std::list< LLVOVolume* > mObjectList ;
	LLMimeDiscoveryResponder* mMimeProbe;
private:
	LLViewerMediaTexture *updatePlaceholderImage();
};
#endif
