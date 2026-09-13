/** 
 * @file llviewertexlayer.h
 * @brief Viewer Texture layer classes. Used for avatars.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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
#ifndef LL_VIEWER_TEXLAYER_H
#define LL_VIEWER_TEXLAYER_H
#include "lldynamictexture.h"
#include "llextendedstatus.h"
#include "lltexlayer.h"
class LLVOAvatarSelf;
class LLViewerTexLayerSetBuffer;
class LLViewerTexLayerSet : public LLTexLayerSet
{
public:
	LLViewerTexLayerSet(LLAvatarAppearance* const appearance);
	virtual ~LLViewerTexLayerSet();
	void				requestUpdate();
	void						requestUpload();
	void						cancelUpload();
	BOOL						isLocalTextureDataAvailable() const;
	BOOL						isLocalTextureDataFinal() const;
	void						updateComposite();
	void				createComposite();
	void						setUpdatesEnabled(BOOL b);
	BOOL						getUpdatesEnabled()	const 	{ return mUpdatesEnabled; }
	LLVOAvatarSelf*				getAvatar();
	const LLVOAvatarSelf*		getAvatar()	const;
	LLViewerTexLayerSetBuffer*	getViewerComposite();
	const LLViewerTexLayerSetBuffer*	getViewerComposite() const;
private:
	BOOL						mUpdatesEnabled;
};
class LLViewerTexLayerSetBuffer : public LLTexLayerSetBuffer, public LLViewerDynamicTexture
{
	LOG_CLASS(LLViewerTexLayerSetBuffer);
public:
	LLViewerTexLayerSetBuffer(LLTexLayerSet* const owner, S32 width, S32 height);
	virtual ~LLViewerTexLayerSetBuffer();
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
public:
	S8          getType() const;
	BOOL					isInitialized(void) const;
	static void				dumpTotalByteCount();
	const std::string		dumpTextureInfo() const;
	virtual void 			restoreGLTexture();
	virtual void 			destroyGLTexture();
private:
	LLViewerTexLayerSet*	getViewerTexLayerSet()
		{ return dynamic_cast<LLViewerTexLayerSet*> (mTexLayerSet); }
	const LLViewerTexLayerSet*	getViewerTexLayerSet() const
		{ return dynamic_cast<const LLViewerTexLayerSet*> (mTexLayerSet); }
	static S32				sGLByteCount;
	virtual void			preRenderTexLayerSet();
	virtual void			midRenderTexLayerSet(BOOL success);
	virtual void			postRenderTexLayerSet(BOOL success);
	virtual S32				getCompositeOriginX() const { return getOriginX(); }
	virtual S32				getCompositeOriginY() const { return getOriginY(); }
	virtual S32				getCompositeWidth() const { return getFullWidth(); }
	virtual S32				getCompositeHeight() const { return getFullHeight(); }
public:
	BOOL		needsRender();
protected:
	virtual void			preRender(BOOL clear_depth) { preRenderTexLayerSet(); }
	virtual void			postRender(BOOL success) { postRenderTexLayerSet(success); }
	virtual BOOL			render() { return renderTexLayerSet(); }
public:
	void					requestUpload();
	void					cancelUpload();
	BOOL					uploadNeeded() const;
	BOOL					uploadInProgress() const;
	BOOL					uploadPending() const;
	static void				onTextureUploadComplete(const LLUUID& uuid,
													void* userdata,
													S32 result, LLExtStat ext_status);
protected:
	BOOL					isReadyToUpload() const;
	void					doUpload();
	void					conditionalRestartUploadTimer();
private:
	BOOL					mNeedsUpload;
	U32						mNumLowresUploads;
	BOOL					mUploadPending;
	LLUUID					mUploadID;
	LLFrameTimer    		mNeedsUploadTimer;
	S32						mUploadFailCount;
	LLFrameTimer    		mUploadRetryTimer;
public:
	void					requestUpdate();
	BOOL					requestUpdateImmediate();
protected:
	BOOL					isReadyToUpdate() const;
	void					doUpdate();
	void					restartUpdateTimer();
private:
	BOOL					mNeedsUpdate;
	U32						mNumLowresUpdates;
	LLFrameTimer    		mNeedsUpdateTimer;
};
struct LLBakedUploadData
{
	LLBakedUploadData(const LLVOAvatarSelf* avatar,
					  LLViewerTexLayerSet* layerset,
					  const LLUUID& id,
					  bool highest_res);
	~LLBakedUploadData() {}
	const LLUUID				mID;
	const LLVOAvatarSelf*		mAvatar;
	LLViewerTexLayerSet*		mTexLayerSet;
   	const U64					mStartTime;
   	const bool					mIsHighestRes;
};
#endif
