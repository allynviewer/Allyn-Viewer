/** 
 * @file llpluginclassmedia.h
 * @brief LLPluginClassMedia handles interaction with a plugin which knows about the "media" message class.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */
#ifndef LL_LLPLUGINCLASSMEDIA_H
#define LL_LLPLUGINCLASSMEDIA_H
#include "llgltypes.h"
#include "llpluginclassbasic.h"
#include "llrect.h"
#include "v4color.h"
#include <boost/signals2.hpp>
class LLPluginClassMedia : public LLPluginClassBasic, public boost::signals2::trackable
{
	LOG_CLASS(LLPluginClassMedia);
public:
	LLPluginClassMedia(LLPluginClassMediaOwner *owner);
	int getWidth() const { return (mMediaWidth > 0) ? mMediaWidth : 0; };
	int getHeight() const { return (mMediaHeight > 0) ? mMediaHeight : 0; };
	int getNaturalWidth() const { return mNaturalMediaWidth; };
	int getNaturalHeight() const { return mNaturalMediaHeight; };
	int getSetWidth() const { return mSetMediaWidth; };
	int getSetHeight() const { return mSetMediaHeight; };
	int getBitsWidth() const { return (mTextureWidth > 0) ? mTextureWidth : 0; };
	int getBitsHeight() const { return (mTextureHeight > 0) ? mTextureHeight : 0; };
	int getTextureWidth() const;
	int getTextureHeight() const;
	int getFullWidth() const { return mFullMediaWidth; };
	int getFullHeight() const { return mFullMediaHeight; };
	F64 getZoomFactor() const { return mZoomFactor; };
	unsigned char* getBitsData();
	int getTextureDepth() const { return mRequestedTextureDepth; };
	int getTextureFormatInternal() const { return mRequestedTextureInternalFormat; };
	int getTextureFormatPrimary() const { return mRequestedTextureFormat; };
	int getTextureFormatType() const { return mRequestedTextureType; };
	bool getTextureFormatSwapBytes() const { return mRequestedTextureSwapBytes; };
	bool getTextureCoordsOpenGL() const { return mRequestedTextureCoordsOpenGL; };
	void setSize(int width, int height);
	void setAutoScale(bool auto_scale);
	void setZoomFactor(F64 zoom_factor) { mZoomFactor = zoom_factor; }
	void setBackgroundColor(const LLColor4& color) { mBackgroundColor = color; };
	void setOwner(LLPluginClassMediaOwner *owner) { mOwner = owner; };
	bool textureValid(void);
	bool getDirty(LLRect *dirty_rect = nullptr);
	void resetDirty(void);
	typedef enum
	{
		MOUSE_EVENT_DOWN,
		MOUSE_EVENT_UP,
		MOUSE_EVENT_MOVE,
		MOUSE_EVENT_DOUBLE_CLICK
	}EMouseEventType;
	void mouseEvent(EMouseEventType type, int button, int x, int y, MASK modifiers);
	typedef enum
	{
		KEY_EVENT_DOWN,
		KEY_EVENT_UP,
		KEY_EVENT_REPEAT
	}EKeyEventType;
	bool keyEvent(EKeyEventType type, int key_code, MASK modifiers, LLSD native_key_data);
	void scrollEvent(int x, int y, int clicks_x, int clicks_y, MASK modifiers);
	void enableMediaPluginDebugging( bool enable );
	void jsEnableObject( bool enable );
	void jsAgentLocationEvent( double x, double y, double z );
	void jsAgentGlobalLocationEvent( double x, double y, double z );
	void jsAgentOrientationEvent( double angle );
	void jsAgentLanguageEvent( const std::string& language );
	void jsAgentRegionEvent( const std::string& region_name );
	void jsAgentMaturityEvent( const std::string& maturity );
	bool textInput(const std::string &text, MASK modifiers, LLSD native_key_data);
	void setCookie(std::string uri, std::string name, std::string value, std::string domain, std::string path, bool httponly, bool secure);
	void loadURI(const std::string &uri);
	void receivePluginMessage(const LLPluginMessage &message) override;
	void pluginLaunchFailed() override;
	void pluginDied() override;
	void priorityChanged(EPriority priority);
	void setLowPrioritySizeLimit(int size);
	F64 getCPUUsage();
	void sendPickFileResponse(const std::vector<std::string> files);
	void sendAuthResponse(bool ok, const std::string &username, const std::string &password);
	std::string getCursorName() const { return mCursorName; };
	LLPluginClassMediaOwner::EMediaStatus getStatus() const { return mStatus; }
	void	undo();
	bool	canUndo() const { return mCanUndo; };
	void	redo();
	bool	canRedo() const { return mCanRedo; };
	void	cut();
	bool	canCut() const { return mCanCut; };
	void	copy();
	bool	canCopy() const { return mCanCopy; };
	void	paste();
	bool	canPaste() const { return mCanPaste; };
	void	doDelete();
	bool	canDoDelete() const { return mCanDoDelete; };
	void	selectAll();
	bool	canSelectAll() const { return mCanSelectAll; };
	void	showPageSource();
	void	setUserDataPath(const std::string &user_data_path_cache, const std::string &user_data_path_cookies, const std::string &user_data_path_cef_log);
	void	setLanguageCode(const std::string &language_code);
	void	setPluginsEnabled(const bool enabled);
	void	setJavascriptEnabled(const bool enabled);
	void	setTarget(const std::string &target);
	bool pluginSupportsMediaBrowser(void);
	void focus(bool focused);
	void set_page_zoom_factor( F64 factor );
	void clear_cache();
	void clear_cookies();
	void cookies_enabled(bool enable);
	void proxy_setup(bool enable, int type = 0, const std::string &host = LLStringUtil::null, int port = 0, const std::string &user = LLStringUtil::null, const std::string &pass = LLStringUtil::null);
	void browse_stop();
	void browse_reload(bool ignore_cache = false);
	void browse_forward();
	void browse_back();
	void setBrowserUserAgent(const std::string& user_agent);
	void showWebInspector( bool show );
	void proxyWindowOpened(const std::string &target, const std::string &uuid);
	void proxyWindowClosed(const std::string &uuid);
	void ignore_ssl_cert_errors(bool ignore);
	void addCertificateFilePath(const std::string& path);
	std::string	getNavigateURI() const { return mNavigateURI; };
	S32			getNavigateResultCode() const { return mNavigateResultCode; };
	std::string getNavigateResultString() const { return mNavigateResultString; };
	bool		getHistoryBackAvailable() const { return mHistoryBackAvailable; };
	bool		getHistoryForwardAvailable() const { return mHistoryForwardAvailable; };
	int			getProgressPercent() const { return mProgressPercent; };
	std::string getStatusText() const { return mStatusText; };
	std::string getLocation() const { return mLocation; };
	std::string getClickURL() const { return mClickURL; };
	std::string getClickNavType() const { return mClickNavType; };
	std::string getClickTarget() const { return mClickTarget; };
	std::string getClickUUID() const { return mClickUUID; };
	void setOverrideClickTarget(const std::string &target);
	void resetOverrideClickTarget() { mClickEnforceTarget = false; };
	bool isOverrideClickTarget() const { return mClickEnforceTarget; }
	std::string getOverrideClickTarget() const { return mOverrideClickTarget; };
	std::string getDebugMessageText() const { return mDebugMessageText; };
	std::string getDebugMessageLevel() const { return mDebugMessageLevel; };
	S32 getStatusCode() const { return mStatusCode; };
	S32 getGeometryX() const { return mGeometryX; };
	S32 getGeometryY() const { return mGeometryY; };
	S32 getGeometryWidth() const { return mGeometryWidth; };
	S32 getGeometryHeight() const { return mGeometryHeight; };
	std::string	getAuthURL() const { return mAuthURL; };
	std::string	getAuthRealm() const { return mAuthRealm; };
	bool getIsMultipleFilePick() const { return mIsMultipleFilePick; }
	std::string	getHoverText() const { return mHoverText; };
	std::string	getHoverLink() const { return mHoverLink; };
	std::string getFileDownloadFilename() const { return mFileDownloadFilename; }
	const std::string& getMediaName() const { return mMediaName; };
	std::string getMediaDescription() const { return mMediaDescription; };
	void		crashPlugin();
	void		hangPlugin();
	bool pluginSupportsMediaTime(void);
	void stop();
	void start(float rate = 0.0f);
	void pause();
	void seek(float time);
	void setLoop(bool loop);
	void setVolume(float volume);
	float getVolume();
	F64 getCurrentTime(void) const { return mCurrentTime; };
	F64 getDuration(void) const { return mDuration; };
	F64 getCurrentPlayRate(void) { return mCurrentRate; };
	F64 getLoadedDuration(void) const { return mLoadedDuration; };
	void initializeUrlHistory(const LLSD& url_history);
protected:
	virtual bool init_impl(void);
	virtual void reset_impl(void);
	virtual void idle_impl(void);
	void mediaEvent(LLPluginClassMediaOwner::EMediaEvent event);
	void setSizeInternal(void);
protected:
	LLPluginClassMediaOwner *mOwner;
	bool		mTextureParamsReceived;
	S32 		mRequestedTextureDepth;
	LLGLenum	mRequestedTextureInternalFormat;
	LLGLenum	mRequestedTextureFormat;
	LLGLenum	mRequestedTextureType;
	bool		mRequestedTextureSwapBytes;
	bool		mRequestedTextureCoordsOpenGL;
	std::string mTextureSharedMemoryName;
	size_t		mTextureSharedMemorySize;
	bool		mAutoScaleMedia;
	int			mDefaultMediaWidth;
	int			mDefaultMediaHeight;
	int			mNaturalMediaWidth;
	int			mNaturalMediaHeight;
	int			mSetMediaWidth;
	int			mSetMediaHeight;
	int			mFullMediaWidth;
	int			mFullMediaHeight;
	int			mRequestedMediaWidth;
	int			mRequestedMediaHeight;
	int			mRequestedTextureWidth;
	int			mRequestedTextureHeight;
	int			mTextureWidth;
	int			mTextureHeight;
	int			mMediaWidth;
	int			mMediaHeight;
	F64			mZoomFactor;
	float		mRequestedVolume;
	EPriority	mPriority;
	int			mLowPrioritySizeLimit;
	bool		mAllowDownsample;
	int			mPadding;
	LLRect mDirtyRect;
	std::string translateModifiers(MASK modifiers);
	std::string mCursorName;
	int			mLastMouseX;
	int			mLastMouseY;
	LLPluginClassMediaOwner::EMediaStatus mStatus;
	F64				mSleepTime;
	bool			mCanUndo;
	bool			mCanRedo;
	bool			mCanCut;
	bool			mCanCopy;
	bool			mCanPaste;
	bool			mCanDoDelete;
	bool			mCanSelectAll;
	std::string		mMediaName;
	std::string		mMediaDescription;
	LLColor4		mBackgroundColor;
	std::string		mTarget;
	std::string		mNavigateURI;
	S32				mNavigateResultCode;
	std::string		mNavigateResultString;
	bool			mHistoryBackAvailable;
	bool			mHistoryForwardAvailable;
	std::string		mStatusText;
	int				mProgressPercent;
	std::string		mLocation;
	std::string		mClickURL;
	std::string		mClickNavType;
	std::string		mClickTarget;
	std::string		mClickUUID;
	bool			mClickEnforceTarget;
	std::string		mOverrideClickTarget;
	std::string		mDebugMessageText;
	std::string		mDebugMessageLevel;
	S32				mGeometryX;
	S32				mGeometryY;
	S32				mGeometryWidth;
	S32				mGeometryHeight;
	S32				mStatusCode;
	std::string		mAuthURL;
	std::string		mAuthRealm;
	std::string		mHoverText;
	std::string		mHoverLink;
	std::string     mFileDownloadFilename;
	bool			mIsMultipleFilePick;
	F64				mCurrentTime;
	F64				mDuration;
	F64				mCurrentRate;
	F64				mLoadedDuration;
};
#endif
