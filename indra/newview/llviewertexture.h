/** 
 * @file llviewertexture.h
 * @brief Object for managing images and their textures
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_LLVIEWERTEXTURE_H
#define LL_LLVIEWERTEXTURE_H
#include "llgltexture.h"
#include "lltimer.h"
#include "llframetimer.h"
#include "llhost.h"
#include "llgltypes.h"
#include "llrender.h"
#if 0
#include "llmetricperformancetester.h"
#endif
#include "llface.h"
#include <map>
#include <list>
extern const S32Megabytes gMinVideoRam;
extern const S32Megabytes gMaxVideoRam;
class LLImageGL ;
class LLImageRaw;
class LLViewerObject;
class LLViewerTexture;
class LLViewerFetchedTexture ;
class LLViewerMediaTexture ;
class LLTexturePipelineTester ;
typedef	void	(*loaded_callback_func)( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* userdata );
class LLVFile;
class LLMessageSystem;
class LLViewerMediaImpl ;
class LLVOVolume ;
class LLFace ;
struct LLTextureKey;
class LLLoadedCallbackEntry
{
public:
    typedef std::set< LLTextureKey > source_callback_list_t;
public:
	LLLoadedCallbackEntry(loaded_callback_func cb,
						  S32 discard_level,
						  BOOL need_imageraw,
						  void* userdata,
						  source_callback_list_t* src_callback_list,
						  LLViewerFetchedTexture* target,
						  BOOL pause);
	~LLLoadedCallbackEntry();
	void removeTexture(LLViewerFetchedTexture* tex) ;
	loaded_callback_func	mCallback;
	S32						mLastUsedDiscard;
	S32						mDesiredDiscard;
	BOOL					mNeedsImageRaw;
	BOOL                    mPaused;
	void*					mUserData;
	source_callback_list_t* mSourceCallbackList;
public:
	static void cleanUpCallbackList(LLLoadedCallbackEntry::source_callback_list_t* callback_list) ;
};
class LLTextureBar;
class LLViewerTexture : public LLGLTexture
{
public:
	enum
	{
		LOCAL_TEXTURE,
		MEDIA_TEXTURE,
		DYNAMIC_TEXTURE,
		FETCHED_TEXTURE,
		LOD_TEXTURE,
		INVALID_TEXTURE_TYPE
	};
	typedef std::vector<LLFace*> ll_face_list_t;
	typedef std::vector<LLVOVolume*> ll_volume_list_t;
protected:
	virtual ~LLViewerTexture();
	LOG_CLASS(LLViewerTexture);
public:
	bool mIsMediaTexture;
	static void initClass();
	static void updateClass(const F32 velocity, const F32 angular_velocity) ;
	LLViewerTexture(BOOL usemipmaps = TRUE, bool allow_compression = false);
	LLViewerTexture(const LLUUID& id, BOOL usemipmaps, bool allow_compression = false) ;
	LLViewerTexture(const LLImageRaw* raw, BOOL usemipmaps, bool allow_compression = false) ;
	LLViewerTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, bool allow_compression = false) ;
	void setNeedsAlphaAndPickMask(BOOL need_mask) { if(mGLTexturep)mGLTexturep->setNeedsAlphaAndPickMask(need_mask); }
	virtual S8 getType() const;
	virtual BOOL isMissingAsset()const ;
	virtual void dump();
	bool bindDefaultImage(const S32 stage = 0) ;
	void forceImmediateUpdate() ;
	bool isActiveFetching();
	const LLUUID& getID() const { return mID; }
	void setID(const LLUUID& id) { mID = id; }
	void setBoostLevel(S32 level);
	S32  getBoostLevel() { return mBoostLevel; }
	void setTextureListType(S32 tex_type) { mTextureListType = tex_type; }
	S32 getTextureListType() { return mTextureListType; }
	void addTextureStats(F32 virtual_size, BOOL needs_gltexture = TRUE) const;
	void resetTextureStats();
	void setMaxVirtualSizeResetInterval(S32 interval)const {mMaxVirtualSizeResetInterval = interval;}
	void resetMaxVirtualSizeResetCounter()const {mMaxVirtualSizeResetCounter = mMaxVirtualSizeResetInterval;}
	S32 getMaxVirtualSizeResetCounter() const { return mMaxVirtualSizeResetCounter; }
	virtual F32  getMaxVirtualSize() ;
	LLFrameTimer* getLastReferencedTimer() {return &mLastReferencedTimer ;}
	S32 getFullWidth() const { return mFullWidth; }
	S32 getFullHeight() const { return mFullHeight; }
	void setKnownDrawSize(S32 width, S32 height);
	virtual void addFace(U32 channel, LLFace* facep) ;
	virtual void removeFace(U32 channel, LLFace* facep) ;
	S32 getTotalNumFaces() const;
	S32 getNumFaces(U32 ch) const;
	const ll_face_list_t* getFaceList(U32 channel) const {llassert(channel < LLRender::NUM_TEXTURE_CHANNELS); return &mFaceList[channel];}
	virtual void addVolume(U32 channel, LLVOVolume* volumep);
	virtual void removeVolume(U32 channel, LLVOVolume* volumep);
	S32 getNumVolumes(U32 channel) const;
	const ll_volume_list_t* getVolumeList(U32 channel) const { return &mVolumeList[channel]; }
	virtual void setCachedRawImage(S32 discard_level, LLImageRaw* imageraw) ;
	BOOL isLargeImage() ;
	void setParcelMedia(LLViewerMediaTexture* media) {mParcelMedia = media;}
	BOOL hasParcelMedia() const { return mParcelMedia != NULL;}
	LLViewerMediaTexture* getParcelMedia() const { return mParcelMedia;}
	void updateBindStatsForTester() ;
protected:
	void cleanup() ;
	void init(bool firstinit) ;
	void reorganizeFaceList() ;
	void reorganizeVolumeList() ;
	void notifyAboutMissingAsset();
	void notifyAboutCreatingTexture();
private:
	friend class LLBumpImageList;
	friend class LLUIImageList;
	virtual void switchToCachedImage();
	static bool isMemoryForTextureLow() ;
protected:
	LLUUID mID;
	S32 mTextureListType;
	F32 mSelectedTime;
	mutable F32 mMaxVirtualSize;
	mutable S32  mMaxVirtualSizeResetCounter ;
	mutable S32  mMaxVirtualSizeResetInterval;
	mutable F32 mAdditionalDecodePriority;
	LLFrameTimer mLastReferencedTimer;
	ll_face_list_t    mFaceList[LLRender::NUM_TEXTURE_CHANNELS];
	U32               mNumFaces[LLRender::NUM_TEXTURE_CHANNELS];
	LLFrameTimer      mLastFaceListUpdateTimer ;
	ll_volume_list_t  mVolumeList[LLRender::NUM_VOLUME_TEXTURE_CHANNELS];
	U32					mNumVolumes[LLRender::NUM_VOLUME_TEXTURE_CHANNELS];
	LLFrameTimer	  mLastVolumeListUpdateTimer;
	LLViewerMediaTexture* mParcelMedia ;
	static F32 sTexelPixelRatio;
public:
	static const U32 sCurrentFileVersion;
	static S32 sImageCount;
	static S32 sRawCount;
	static S32 sAuxCount;
	static LLFrameTimer sEvaluationTimer;
	static F32 sDesiredDiscardBias;
	static F32 sDesiredDiscardScale;
	static S64Bytes sBoundTextureMemory;
	static S64Bytes sTotalTextureMemory;
	static S32Megabytes sMaxBoundTextureMemory;
	static S32Megabytes sMaxTotalTextureMem;
	static S64Bytes sMaxDesiredTextureMem ;
	static S32 sCameraMovingDiscardBias;
	static F32 sCameraMovingBias;
	static S32 sMaxSculptRez ;
	static S32 sMinLargeImageSize ;
	static S32 sMaxSmallImageSize ;
	static BOOL sFreezeImageScalingDown ;
	static F32  sCurrentTime ;
	enum EDebugTexels
	{
		DEBUG_TEXELS_OFF,
		DEBUG_TEXELS_CURRENT,
		DEBUG_TEXELS_DESIRED,
		DEBUG_TEXELS_FULL
	};
	static EDebugTexels sDebugTexelsMode;
	static LLPointer<LLViewerTexture> sNullImagep;
	static LLPointer<LLViewerTexture> sBlackImagep;
};
enum FTType
{
	FTT_UNKNOWN = -1,
	FTT_DEFAULT = 0,
	FTT_SERVER_BAKE,
	FTT_HOST_BAKE,
	FTT_MAP_TILE,
	FTT_LOCAL_FILE
};
const std::string& fttype_to_string(const FTType& fttype);
class LLViewerFetchedTexture : public LLViewerTexture
{
	friend class LLTextureBar;
	friend class LLTextureView;
protected:
	~LLViewerFetchedTexture();
public:
	LLViewerFetchedTexture(const LLUUID& id, FTType f_type, const LLHost& host = LLHost::invalid, BOOL usemipmaps = TRUE);
	LLViewerFetchedTexture(const LLImageRaw* raw, FTType f_type, BOOL usemipmaps);
	LLViewerFetchedTexture(const std::string& url, FTType f_type, const LLUUID& id, BOOL usemipmaps = TRUE);
public:
	static F32 maxDecodePriority();
	struct Compare
	{
		bool operator()(const LLPointer<LLViewerFetchedTexture> &lhs, const LLPointer<LLViewerFetchedTexture> &rhs) const
		{
			const LLViewerFetchedTexture* lhsp = (const LLViewerFetchedTexture*)lhs;
			const LLViewerFetchedTexture* rhsp = (const LLViewerFetchedTexture*)rhs;
			const F32 lpriority = lhsp->getDecodePriority();
			const F32 rpriority = rhsp->getDecodePriority();
			if (lpriority > rpriority)
				return true;
			if (lpriority < rpriority)
				return false;
			return lhsp < rhsp;
		}
	};
public:
	S8 getType() const ;
	FTType getFTType() const;
	void forceImmediateUpdate() ;
	void dump() ;
	void setLoadedCallback(loaded_callback_func cb,
						   S32 discard_level, BOOL keep_imageraw, BOOL needs_aux,
						   void* userdata, LLLoadedCallbackEntry::source_callback_list_t* src_callback_list, BOOL pause = FALSE);
	bool hasCallbacks() { return mLoadedCallbackList.empty() ? false : true; }
	void pauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
	void unpauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
	bool doLoadedCallbacks();
	void deleteCallbackEntry(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
	void clearCallbackEntryList() ;
	void addToCreateTexture();
	void loadFromFastCache();
	void setInFastCacheList(bool in_list) { mInFastCacheList = in_list; }
	bool isInFastCacheList() const { return mInFastCacheList; }
	BOOL createTexture(LLImageGL::GLTextureName* usename = nullptr);
	void destroyTexture() ;
	virtual void processTextureStats() ;
	F32  calcDecodePriority() ;
	BOOL needsAux() const { return mNeedsAux; }
	void setTargetHost(LLHost host)			{ mTargetHost = host; }
	LLHost getTargetHost() const			{ return mTargetHost; }
	void setDecodePriority(F32 priority = -1.0f);
	F32 getDecodePriority() const { return mDecodePriority; };
	F32 getAdditionalDecodePriority() const { return mAdditionalDecodePriority; };
	void setAdditionalDecodePriority(F32 priority) ;
	void updateVirtualSize() ;
	S32  getDesiredDiscardLevel()			 { return mDesiredDiscardLevel; }
	void setMinDiscardLevel(S32 discard) 	{ mMinDesiredDiscardLevel = llmin(mMinDesiredDiscardLevel,(S8)discard); }
	bool updateFetch();
	bool setDebugFetching(S32 debug_level);
	bool isInDebug() {return mInDebug;}
	void clearFetchedResults();
	void setKnownDrawSize(S32 width, S32 height);
	void setIsMissingAsset(BOOL is_missing = true);
	BOOL isMissingAsset()	const		{ return mIsMissingAsset; }
	S32 getOriginalWidth() { return mOrigWidth; }
	S32 getOriginalHeight() { return mOrigHeight; }
	BOOL isInImageList() const {return mInImageList ;}
	void setInImageList(BOOL flag) {mInImageList = flag ;}
	LLFrameTimer* getLastPacketTimer() {return &mLastPacketTimer;}
	U32 getFetchPriority() const { return mFetchPriority ;}
	F32 getDownloadProgress() const {return mDownloadProgress ;}
	LLImageRaw* reloadRawImage(S8 discard_level) ;
	void destroyRawImage();
	bool needsToSaveRawImage();
	const std::string& getUrl() const {return mUrl;}
	BOOL isDeleted() ;
	BOOL isInactive() ;
	BOOL isDeletionCandidate();
	void setDeletionCandidate() ;
	void setInactive() ;
	BOOL getUseDiscard() const { return mUseMipMaps && !mDontDiscard; }
	void setForSculpt();
	BOOL forSculpt() const {return mForSculpt;}
	BOOL isForSculptOnly() const;
	void        checkCachedRawSculptImage() ;
	LLImageRaw* getRawImage()const { return mRawImage ;}
	S32         getRawImageLevel() const {return mRawDiscardLevel;}
	LLImageRaw* getCachedRawImage() const { return mCachedRawImage ;}
	S32         getCachedRawImageLevel() const {return mCachedRawDiscardLevel;}
	BOOL        isCachedRawImageReady() const {return mCachedRawImageReady ;}
	BOOL        isRawImageValid()const { return mIsRawImageValid ; }
	void        forceToSaveRawImage(S32 desired_discard = 0, F32 kept_time = 0.f) ;
	void        forceToRefetchTexture(S32 desired_discard = 0, F32 kept_time = 60.f);
	void setCachedRawImage(S32 discard_level, LLImageRaw* imageraw) ;
	void        destroySavedRawImage() ;
	LLImageRaw* getSavedRawImage() ;
	BOOL        hasSavedRawImage() const ;
	F32         getElapsedLastReferencedSavedRawImageTime() const ;
	BOOL		isFullyLoaded() const;
	BOOL        hasFetcher() const { return mHasFetcher;}
	void        setCanUseHTTP(bool can_use_http) {mCanUseHTTP = can_use_http;}
	void        forceToDeleteRequest();
	void		forceRefetch();
	bool  isActiveFetching();
	LLUUID		getUploader();
	LLDate		getUploadTime();
	std::string getComment();
protected:
	void switchToCachedImage();
	S32 getCurrentDiscardLevelForFetching() ;
private:
	void init(bool firstinit) ;
	void cleanup() ;
	void saveRawImage() ;
	void setCachedRawImage() ;
	void setCachedRawImagePtr(LLImageRaw *pRawImage) ;
private:
	BOOL  mFullyLoaded;
	BOOL  mInDebug;
	BOOL  mForceCallbackFetch;
protected:
	std::string mLocalFileName;
	S32 mOrigWidth;
	S32 mOrigHeight;
	S32 mKnownDrawWidth;
	S32	mKnownDrawHeight;
	BOOL mKnownDrawSizeChanged ;
	std::string mUrl;
	S32 mRequestedDiscardLevel;
	F32 mRequestedDownloadPriority;
	S32 mFetchState;
	U32 mFetchPriority;
	F32 mDownloadProgress;
	F32 mFetchDeltaTime;
	F32 mRequestDeltaTime;
	F32 mDecodePriority;
	S32	mMinDiscardLevel;
	S8  mDesiredDiscardLevel;
	S8  mMinDesiredDiscardLevel;
	S8  mNeedsAux;
	S8  mHasAux;
	S8  mDecodingAux;
	S8  mIsRawImageValid;
	S8  mHasFetcher;
	S8  mIsFetching;
	bool mCanUseHTTP ;
	const FTType mFTType;
	mutable S8 mIsMissingAsset;
	typedef std::list<LLLoadedCallbackEntry*> callback_list_t;
	S8              mLoadedCallbackDesiredDiscardLevel;
	BOOL            mPauseLoadedCallBacks;
	callback_list_t mLoadedCallbackList;
	F32             mLastCallBackActiveTime;
	LLPointer<LLImageRaw> mRawImage;
	S32 mRawDiscardLevel;
	LLPointer<LLImageRaw> mAuxRawImage;
	BOOL mForceToSaveRawImage ;
	BOOL mSaveRawImage;
	LLPointer<LLImageRaw> mSavedRawImage;
	S32 mSavedRawDiscardLevel;
	S32 mDesiredSavedRawDiscardLevel;
	F32 mLastReferencedSavedRawImageTime ;
	F32 mKeptSavedRawImageTime ;
	LLPointer<LLImageRaw> mCachedRawImage;
	S32 mCachedRawDiscardLevel;
	BOOL mCachedRawImageReady;
	LLHost mTargetHost;
	LLFrameTimer mLastPacketTimer;
	LLFrameTimer mStopFetchingTimer;
	BOOL  mInImageList;
	BOOL  mNeedsCreateTexture;
	BOOL  mInFastCacheList;
	BOOL   mForSculpt ;
	BOOL   mIsFetched ;
	std::map<S8, std::string> mComment;
public:
	static LLPointer<LLViewerFetchedTexture> sMissingAssetImagep;
	static LLPointer<LLViewerFetchedTexture> sWhiteImagep;
	static LLPointer<LLViewerFetchedTexture> sDefaultImagep;
	static LLPointer<LLViewerFetchedTexture> sSmokeImagep;
	static LLPointer<LLViewerFetchedTexture> sFlatNormalImagep;
};
class LLViewerLODTexture : public LLViewerFetchedTexture
{
protected:
	~LLViewerLODTexture(){}
public:
	LLViewerLODTexture(const LLUUID& id, FTType f_type, const LLHost& host = LLHost::invalid, BOOL usemipmaps = TRUE);
	LLViewerLODTexture(const std::string& url, FTType f_type, const LLUUID& id, BOOL usemipmaps = TRUE);
	S8 getType() const;
	void processTextureStats();
	BOOL isUpdateFrozen() ;
private:
	void init(bool firstinit) ;
	bool scaleDown() ;
private:
	F32 mDiscardVirtualSize;
	F32 mCalculatedDiscardLevel;
};
class LLViewerMediaTexture : public LLViewerTexture
{
protected:
	~LLViewerMediaTexture() ;
public:
	LLViewerMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;
	S8 getType() const;
	void reinit(BOOL usemipmaps = TRUE);
	BOOL  getUseMipMaps() {return mUseMipMaps ; }
	void  setUseMipMaps(BOOL mipmap) ;
	void setPlaying(BOOL playing) ;
	BOOL isPlaying() const {return mIsPlaying;}
	void setMediaImpl() ;
	void initVirtualSize() ;
	void invalidateMediaImpl() ;
	void addMediaToFace(LLFace* facep) ;
	void removeMediaFromFace(LLFace* facep) ;
	void addFace(U32 ch, LLFace* facep) ;
	void removeFace(U32 ch, LLFace* facep) ;
	F32  getMaxVirtualSize() ;
private:
	void switchTexture(U32 ch, LLFace* facep) ;
	BOOL findFaces() ;
	void stopPlaying() ;
private:
	std::list< LLFace* > mMediaFaceList ;
	std::list< LLPointer<LLViewerTexture> > mTextureList ;
	LLViewerMediaImpl* mMediaImplp ;
	BOOL mIsPlaying ;
	U64  mUpdateVirtualSizeTime ;
public:
	static void updateClass() ;
	static void cleanUpClass() ;
	static LLViewerMediaTexture* findMediaTexture(const LLUUID& media_id) ;
	static void removeMediaImplFromTexture(const LLUUID& media_id) ;
private:
	typedef std::map< LLUUID, LLPointer<LLViewerMediaTexture> > media_map_t ;
	static media_map_t sMediaMap ;
};
class LLViewerTextureManager
{
private:
	LLViewerTextureManager(){}
public:
#if 0
	static LLTexturePipelineTester* sTesterp ;
#endif
	static LLViewerFetchedTexture*    staticCastToFetchedTexture(LLTexture* tex, BOOL report_error = FALSE) ;
	static void                       findFetchedTextures(const LLUUID& id, std::vector<LLViewerFetchedTexture*> &output);
	static void                       findTextures(const LLUUID& id, std::vector<LLViewerTexture*> &output);
	static LLViewerFetchedTexture*    findFetchedTexture(const LLUUID& id, S32 tex_type);
	static LLViewerMediaTexture*      findMediaTexture(const LLUUID& id) ;
	static LLViewerMediaTexture*      createMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;
	static LLViewerMediaTexture*      getMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;
	static LLPointer<LLViewerTexture> getLocalTexture(BOOL usemipmaps = TRUE, BOOL generate_gl_tex = TRUE);
	static LLPointer<LLViewerTexture> getLocalTexture(const LLUUID& id, BOOL usemipmaps, BOOL generate_gl_tex = TRUE) ;
	static LLPointer<LLViewerTexture> getLocalTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	static LLPointer<LLViewerTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex = TRUE) ;
	static LLViewerFetchedTexture* getFetchedTexture(const LLUUID &image_id,
									 FTType f_type = FTT_DEFAULT,
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 LLHost request_from_host = LLHost()
									 );
	static LLViewerFetchedTexture* getFetchedTextureFromFile(const std::string& filename,
									 FTType f_type = FTT_LOCAL_FILE,
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 const LLUUID& force_id = LLUUID::null
									 );
	static LLViewerFetchedTexture* getFetchedTextureFromUrl(const std::string& url,
									 FTType f_type,
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 const LLUUID& force_id = LLUUID::null
									 );
	static LLViewerFetchedTexture* getFetchedTextureFromHost(const LLUUID& image_id, FTType f_type, LLHost host) ;
	static void init() ;
	static void cleanup() ;
};
#if 0
class LLTexturePipelineTester : public LLMetricPerformanceTesterWithSession
{
	enum
	{
		MIN_LARGE_IMAGE_AREA = 262144
	};
public:
	LLTexturePipelineTester() ;
	~LLTexturePipelineTester() ;
	void update();
	void updateTextureBindingStats(const LLViewerTexture* imagep) ;
	void updateTextureLoadingStats(const LLViewerFetchedTexture* imagep, const LLImageRaw* raw_imagep, BOOL from_cache) ;
	void updateGrayTextureBinding() ;
	void setStablizingTime() ;
private:
	void reset() ;
	void updateStablizingTime() ;
	void outputTestRecord(LLSD* sd) ;
private:
	BOOL mPause ;
private:
	BOOL mUsingDefaultTexture;
	U32Bytes mTotalBytesUsed ;
	U32Bytes mTotalBytesUsedForLargeImage ;
	U32Bytes mLastTotalBytesUsed ;
	U32Bytes mLastTotalBytesUsedForLargeImage ;
	U32Bytes mTotalBytesLoaded ;
	U32Bytes mTotalBytesLoadedFromCache ;
	U32Bytes mTotalBytesLoadedForLargeImage ;
	U32Bytes mTotalBytesLoadedForSculpties ;
	F32 mStartFetchingTime ;
	F32 mTotalGrayTime ;
	F32 mTotalStablizingTime ;
	F32 mStartTimeLoadingSculpties ;
	F32 mEndTimeLoadingSculpties ;
	F32 mStartStablizingTime ;
	F32 mEndStablizingTime ;
private:
	class LLTextureTestSession : public LLTestSession
	{
	public:
		LLTextureTestSession() ;
		~LLTextureTestSession() ;
		void reset() ;
		F32 mTotalFetchingTime ;
		F32 mTotalGrayTime ;
		F32 mTotalStablizingTime ;
		F32 mStartTimeLoadingSculpties ;
		F32 mTotalTimeLoadingSculpties ;
		S32 mTotalBytesLoaded ;
		S32 mTotalBytesLoadedFromCache ;
		S32 mTotalBytesLoadedForLargeImage ;
		S32 mTotalBytesLoadedForSculpties ;
		typedef struct _texture_instant_preformance_t
		{
			S32 mAverageBytesUsedPerSecond ;
			S32 mAverageBytesUsedForLargeImagePerSecond ;
			F32 mAveragePercentageBytesUsedPerSecond ;
			F32 mTime ;
		}texture_instant_preformance_t ;
		std::vector<texture_instant_preformance_t> mInstantPerformanceList ;
		S32 mInstantPerformanceListCounter ;
	};
	LLMetricPerformanceTesterWithSession::LLTestSession* loadTestSession(LLSD* log) ;
	void compareTestSessions(std::ofstream* os) ;
};
#endif
#endif
