/** 
 * @file llfloatersnapshot.cpp
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "llfloatersnapshot.h"
#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "llrender.h"
#include "v3dmath.h"
#include "llmath.h"
#include "lldir.h"
#include "lllocalcliprect.h"
#include "llsdserialize.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llcallbacklist.h"
#include "llcriticaldamp.h"
#include "llfloaterperms.h"
#include "llui.h"
#include "llviewertexteditor.h"
#include "llfocusmgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llagentbenefits.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerstats.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llviewermenufile.h"
#include "llresourcedata.h"
#include "llfloaterpostcard.h"
#include "llfloaterfeed.h"
#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "llworld.h"
#include "llagentui.h"
#include "llvoavatar.h"
#include "lluploaddialog.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llnotificationsutil.h"
#include "llvfile.h"
#include "llvfs.h"
#include "hippogridmanager.h"
S32 LLFloaterSnapshot::sUIWinHeightLong = 566;
S32 LLFloaterSnapshot::sUIWinHeightShort = LLFloaterSnapshot::sUIWinHeightLong - 266;
S32 LLFloaterSnapshot::sUIWinWidth = 219;
S32 const THUMBHEIGHT = 159;
LLSnapshotFloaterView* gSnapshotFloaterView = NULL;
LLFloaterSnapshot* LLFloaterSnapshot::sInstance = NULL;
const F32 AUTO_SNAPSHOT_TIME_DELAY = 1.f;
F32 SHINE_TIME = 0.5f;
F32 SHINE_WIDTH = 0.6f;
F32 SHINE_OPACITY = 0.3f;
F32 FALL_TIME = 0.6f;
S32 BORDER_WIDTH = 6;
const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024;
const S32 MAX_TEXTURE_SIZE = 1024;
static std::string snapshotKeepAspectName();
class LLSnapshotLivePreview : public LLView
{
public:
	enum ESnapshotType
	{
		SNAPSHOT_FEED,
		SNAPSHOT_POSTCARD,
		SNAPSHOT_TEXTURE,
		SNAPSHOT_LOCAL
	};
	enum EAspectSizeProblem
	{
		ASPECTSIZE_OK,
		CANNOT_CROP_HORIZONTALLY,
		CANNOT_CROP_VERTICALLY,
		SIZE_TOO_LARGE,
		CANNOT_RESIZE,
		DELAYED,
		NO_RAW_IMAGE,
		ENCODING_FAILED
	};
	U32 typeToMask(ESnapshotType type) const { return 1 << type; }
	void addUsedBy(ESnapshotType type) { mUsedBy |= typeToMask(type); }
	void delUsedBy(ESnapshotType type) { mUsedBy &= ~typeToMask(type); }
	bool isUsedBy(ESnapshotType type) const { return (mUsedBy & typeToMask(type)) != 0; }
	bool isUsed(void) const { return mUsedBy; }
	void addManualOverride(ESnapshotType type) { mManualSizeOverride |= typeToMask(type); }
	bool isOverriddenBy(ESnapshotType type) const { return (mManualSizeOverride & typeToMask(type)) != 0; }
	LLSnapshotLivePreview(const LLRect& rect);
	~LLSnapshotLivePreview();
	void draw();
	void reshape(S32 width, S32 height, BOOL called_from_parent);
	void setSize(S32 w, S32 h);
	void getSize(S32& w, S32& h) const;
	void setAspect(F32 a);
	F32 getAspect() const;
	void getRawSize(S32& w, S32& h) const;
	S32 getDataSize() const { return mFormattedDataSize; }
	void setMaxImageSize(S32 size) ;
	S32  getMaxImageSize() {return mMaxImageSize ;}
	ESnapshotType getSnapshotType() const { return mSnapshotType; }
	LLFloaterSnapshot::ESnapshotFormat getSnapshotFormat() const { return mSnapshotFormat; }
	BOOL getRawSnapshotUpToDate() const;
	BOOL getSnapshotUpToDate() const;
	BOOL isSnapshotActive() { return mSnapshotActive; }
	LLViewerTexture* getThumbnailImage() const { return mThumbnailImage ; }
	S32  getThumbnailWidth() const { return mThumbnailWidth ; }
	S32  getThumbnailHeight() const { return mThumbnailHeight ; }
	BOOL getThumbnailLock() const { return mThumbnailUpdateLock ; }
	BOOL getThumbnailUpToDate() const { return mThumbnailUpToDate ;}
	bool getShowFreezeFrameSnapshot() const { return mShowFreezeFrameSnapshot; }
	LLViewerTexture* getCurrentImage();
	char const* resolutionComboName() const;
	char const* aspectComboName() const;
	void setSnapshotType(ESnapshotType type) { mSnapshotType = type; }
	void setSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat type) { mSnapshotFormat = type; }
	void setSnapshotQuality(S32 quality);
	void setSnapshotBufferType(LLFloaterSnapshot* floater, LLViewerWindow::ESnapshotType type);
	void showFreezeFrameSnapshot(bool show);
	void updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail = FALSE, F32 delay = 0.f);
	LLFloaterFeed* getCaptionAndSaveFeed();
	LLFloaterPostcard* savePostcard();
	void saveTexture();
	static void saveTextureDone(LLUUID const& asset_id, void* user_data, S32 status,  LLExtStat ext_status);
	static void saveTextureDone2(bool success, void* user_data);
	void saveLocal();
	void saveStart(int index);
	void saveDone(ESnapshotType type, bool success, int index);
	void close(LLFloaterSnapshot* view);
	void doCloseAfterSave();
	BOOL setThumbnailImageSize();
	void generateThumbnailImage();
	EAspectSizeProblem getAspectSizeProblem(S32& width_out, S32& height_out, bool& crop_vertically_out, S32& crop_offset_out);
	EAspectSizeProblem generateFormattedAndFullscreenPreview(bool delayed_formatted = false);
	void drawPreviewRect(S32 offset_x, S32 offset_y) ;
	static BOOL onIdle(LLSnapshotLivePreview* previewp);
private:
	LLColor4					mColor;
	LLPointer<LLViewerTexture>	mFullScreenPreviewTexture;
	LLRect						mFullScreenImageRect;
	S32							mWidth;
	S32							mHeight;
	BOOL						mFullScreenPreviewTextureNeedsScaling;
	LLPointer<LLViewerTexture>	mFallFullScreenPreviewTexture;
	LLRect						mFallFullScreenImageRect;
	S32							mFallWidth;
	S32							mFallHeight;
	BOOL						mFallFullScreenPreviewTextureNeedsScaling;
	S32                         mMaxImageSize;
	F32							mAspectRatio;
	LLPointer<LLViewerTexture>	mThumbnailImage ;
	LLRect                      mThumbnailPreviewRect ;
	S32                         mThumbnailWidth ;
	S32                         mThumbnailHeight ;
	BOOL                        mThumbnailUpdateLock ;
	BOOL                        mThumbnailUpToDate ;
	LLPointer<LLImageRaw>		mRawSnapshot;
	S32							mRawSnapshotWidth;
	S32							mRawSnapshotHeight;
	BOOL						mRawSnapshotRenderUI;
	BOOL						mRawSnapshotRenderHUD;
	LLViewerWindow::ESnapshotType mRawSnapshotBufferType;
	LLPointer<LLImageFormatted>	mFormattedImage;
	S32							mFormattedWidth;
	S32							mFormattedHeight;
	S32							mFormattedRawWidth;
	S32							mFormattedRawHeight;
	S32							mFormattedCropOffset;
	bool						mFormattedCropVertically;
	LLFloaterSnapshot::ESnapshotFormat	mFormattedSnapshotFormat;
	S32							mFormattedSnapshotQuality;
	bool						mFormattedUpToDate;
	LLFrameTimer				mSnapshotDelayTimer;
	U32							mUsedBy;
	U32							mManualSizeOverride;
	S32							mShineCountdown;
	LLFrameTimer				mShineAnimTimer;
	F32							mFlashAlpha;
	BOOL						mNeedsFlash;
	LLVector3d					mPosTakenGlobal;
	S32							mSnapshotQuality;
	S32							mFormattedDataSize;
	ESnapshotType				mSnapshotType;
	LLFloaterSnapshot::ESnapshotFormat	mSnapshotFormat;
	BOOL						mShowFreezeFrameSnapshot;
	LLFrameTimer				mFallAnimTimer;
	LLVector3					mCameraPos;
	LLQuaternion				mCameraRot;
	BOOL						mSnapshotActive;
	LLViewerWindow::ESnapshotType mSnapshotBufferType;
	int							mOutstandingCallbacks;
	int							mSaveFailures;
	LLFloaterSnapshot*			mCloseCalled;
	static int					sSnapshotIndex;
public:
	static std::set<LLSnapshotLivePreview*> sList;
	LLFrameTimer				mFormattedDelayTimer;
};
class LLFloaterSnapshot::Impl
{
public:
	Impl()
	:	mAvatarPauseHandles(),
		mLastToolset(NULL)
	{
	}
	~Impl()
	{
		mAvatarPauseHandles.clear();
		mQualityMouseUpConnection.disconnect();
	}
	static void onClickDiscard(void* data);
	static void onClickKeep(void* data);
	static void onCommitSave(LLUICtrl* ctrl, void* data);
	static void onClickNewSnapshot(void* data);
	static void onClickFreezeTime(void* data);
	static void onClickAutoSnap(LLUICtrl *ctrl, void* data);
	static void onClickTemporaryImage(LLUICtrl *ctrl, void* data);
	static void onClickLess(void* data) ;
	static void onClickMore(void* data) ;
	static void onClickUICheck(LLUICtrl *ctrl, void* data);
	static void onClickHUDCheck(LLUICtrl *ctrl, void* data);
	static void onClickKeepOpenCheck(LLUICtrl *ctrl, void* data);
	static void onClickKeepAspect(LLUICtrl* ctrl, void* data);
	static void onCommitQuality(LLUICtrl* ctrl, void* data);
	static void onCommitFeedResolution(LLUICtrl* ctrl, void* data);
	static void onCommitPostcardResolution(LLUICtrl* ctrl, void* data);
	static void onCommitTextureResolution(LLUICtrl* ctrl, void* data);
	static void onCommitLocalResolution(LLUICtrl* ctrl, void* data);
	static void onCommitFeedAspect(LLUICtrl* ctrl, void* data);
	static void onCommitPostcardAspect(LLUICtrl* ctrl, void* data);
	static void onCommitTextureAspect(LLUICtrl* ctrl, void* data);
	static void onCommitLocalAspect(LLUICtrl* ctrl, void* data);
	static void updateResolution(LLUICtrl* ctrl, void* data, bool update_controls = true);
	static void updateAspect(LLUICtrl* ctrl, void* data, bool update_controls = true);
	static void onCommitFreezeTime(LLUICtrl* ctrl, void* data);
	static void onCommitLayerTypes(LLUICtrl* ctrl, void*data);
	static void onCommitSnapshotType(LLUICtrl* ctrl, void* data);
	static void onCommitSnapshotFormat(LLUICtrl* ctrl, void* data);
	static void onCommitCustomResolution(LLUICtrl *ctrl, void* data);
	static void onCommitCustomAspect(LLUICtrl *ctrl, void* data);
	static void onQualityMouseUp(void* data);
	static LLSnapshotLivePreview* getPreviewView(void);
	static void setResolution(LLFloaterSnapshot* floater, const std::string& comboname, bool visible, bool update_controls = true);
	static void setAspect(LLFloaterSnapshot* floater, const std::string& comboname, bool update_controls = true);
	static void storeAspectSetting(LLComboBox* combo, const std::string& comboname);
	static void enforceAspect(LLFloaterSnapshot* floater, F32 new_aspect);
	static void enforceResolution(LLFloaterSnapshot* floater, F32 new_aspect);
	static void updateControls(LLFloaterSnapshot* floater, bool delayed_formatted = false);
	static void resetFeedAndPostcardAspect(LLFloaterSnapshot* floater);
	static void updateLayout(LLFloaterSnapshot* floater);
	static void freezeTime(bool on);
	static void keepAspect(LLFloaterSnapshot* view, bool on, bool force = false);
	static LLHandle<LLView> sPreviewHandle;
private:
	static LLSnapshotLivePreview::ESnapshotType getTypeIndex(LLFloaterSnapshot* floater);
	static ESnapshotFormat getFormatIndex(LLFloaterSnapshot* floater);
	static void comboSetCustom(LLFloaterSnapshot *floater, const std::string& comboname);
	static void checkAutoSnapshot(LLSnapshotLivePreview* floater, BOOL update_thumbnail = FALSE);
public:
	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;
	LLToolset*	mLastToolset;
	boost::signals2::connection mQualityMouseUpConnection;
};
int LLSnapshotLivePreview::sSnapshotIndex;
void LLSnapshotLivePreview::setSnapshotBufferType(LLFloaterSnapshot* floater, LLViewerWindow::ESnapshotType type)
{
	mSnapshotBufferType = type;
	switch(type)
	{
	  case LLViewerWindow::SNAPSHOT_TYPE_COLOR:
		floater->childSetValue("layer_types", "colors");
		break;
	  case LLViewerWindow::SNAPSHOT_TYPE_DEPTH:
		floater->childSetValue("layer_types", "depth");
		break;
	}
}
BOOL LLSnapshotLivePreview::getRawSnapshotUpToDate() const
{
	return mRawSnapshotRenderUI == gSavedSettings.getBOOL("RenderUIInSnapshot") &&
		mRawSnapshotRenderHUD == gSavedSettings.getBOOL("RenderHUDInSnapshot") &&
		mRawSnapshotBufferType == mSnapshotBufferType;
}
BOOL LLSnapshotLivePreview::getSnapshotUpToDate() const
{
	return mFormattedUpToDate && getRawSnapshotUpToDate();
}
std::set<LLSnapshotLivePreview*> LLSnapshotLivePreview::sList;
LLSnapshotLivePreview::LLSnapshotLivePreview (const LLRect& rect) :
	LLView(std::string("snapshot_live_preview"), rect, FALSE),
	mColor(1.f, 0.f, 0.f, 0.5f),
	mRawSnapshot(NULL),
	mRawSnapshotWidth(0),
	mRawSnapshotHeight(1),
	mRawSnapshotRenderUI(FALSE),
	mRawSnapshotRenderHUD(FALSE),
	mRawSnapshotBufferType(LLViewerWindow::SNAPSHOT_TYPE_COLOR),
	mThumbnailImage(NULL) ,
	mThumbnailWidth(0),
	mThumbnailHeight(0),
	mFormattedImage(NULL),
	mFormattedUpToDate(false),
	mUsedBy(0),
	mManualSizeOverride(0),
	mShineCountdown(0),
	mFlashAlpha(0.f),
	mNeedsFlash(TRUE),
	mSnapshotQuality(gSavedSettings.getS32("SnapshotQuality")),
	mFormattedDataSize(0),
	mSnapshotType((ESnapshotType)gSavedSettings.getS32("LastSnapshotType")),
	mSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat(gSavedSettings.getS32("SnapshotFormat"))),
	mShowFreezeFrameSnapshot(FALSE),
	mCameraPos(LLViewerCamera::getInstance()->getOrigin()),
	mCameraRot(LLViewerCamera::getInstance()->getQuaternion()),
	mSnapshotActive(FALSE),
	mSnapshotBufferType(LLViewerWindow::SNAPSHOT_TYPE_COLOR),
	mCloseCalled(NULL)
{
	DoutEntering(dc::snapshot, "LLSnapshotLivePreview() [" << (void*)this << "]");
	setSnapshotQuality(gSavedSettings.getS32("SnapshotQuality"));
	mSnapshotDelayTimer.start();
	sList.insert(this);
	setFollowsAll();
	mWidth = gViewerWindow->getWindowDisplayWidth();
	mHeight = gViewerWindow->getWindowDisplayHeight();
	mAspectRatio = (F32)mWidth / mHeight;
	mFallWidth = mWidth;
	mFallHeight = mHeight;
	mFullScreenPreviewTextureNeedsScaling = FALSE;
	mFallFullScreenPreviewTextureNeedsScaling = FALSE;
	mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
	mThumbnailUpdateLock = FALSE ;
	mThumbnailUpToDate   = FALSE ;
	updateSnapshot(TRUE,TRUE);
}
LLSnapshotLivePreview::~LLSnapshotLivePreview()
{
	DoutEntering(dc::snapshot, "~LLSnapshotLivePreview() [" << (void*)this << "]");
	sList.erase(this);
	++sSnapshotIndex;
}
void LLSnapshotLivePreview::setMaxImageSize(S32 size)
{
	if(size < MAX_SNAPSHOT_IMAGE_SIZE)
	{
		mMaxImageSize = size;
	}
	else
	{
		mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
	}
}
LLViewerTexture* LLSnapshotLivePreview::getCurrentImage()
{
	return mFullScreenPreviewTexture;
}
void LLSnapshotLivePreview::showFreezeFrameSnapshot(bool show)
{
	DoutEntering(dc::snapshot, "LLSnapshotLivePreview::showFreezeFrameSnapshot(" << show << ")");
	if (mShowFreezeFrameSnapshot && !show)
	{
		mFallFullScreenPreviewTexture = mFullScreenPreviewTexture;
		mFallFullScreenPreviewTextureNeedsScaling = mFullScreenPreviewTextureNeedsScaling;
		mFallFullScreenImageRect = mFullScreenImageRect;
		mFallWidth = mFormattedWidth;
		mFallHeight = mFormattedHeight;
		mFallAnimTimer.start();
	}
	mShowFreezeFrameSnapshot = show;
}
void LLSnapshotLivePreview::updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail, F32 delay)
{
	DoutEntering(dc::snapshot, "LLSnapshotLivePreview::updateSnapshot(" << new_snapshot << ", " << new_thumbnail << ", " << delay << ")");
	LLRect& rect = mFullScreenImageRect;
	rect.set(0, getRect().getHeight(), getRect().getWidth(), 0);
	S32 window_width = gViewerWindow->getWindowWidthRaw() ;
	S32 window_height = gViewerWindow->getWindowHeightRaw() ;
	F32 window_aspect_ratio = ((F32)window_width) / ((F32)window_height);
	if (mAspectRatio > window_aspect_ratio)
	{
		S32 height_diff = ll_round(getRect().getHeight() - (F32)getRect().getWidth() / mAspectRatio);
		S32 half_height_diff = ll_round((getRect().getHeight() - (F32)getRect().getWidth() / mAspectRatio) * 0.5);
		rect.mBottom += half_height_diff;
		rect.mTop -= height_diff - half_height_diff;
	}
	else if (mAspectRatio < window_aspect_ratio)
	{
		S32 width_diff = ll_round(getRect().getWidth() - (F32)getRect().getHeight() * mAspectRatio);
		S32 half_width_diff = ll_round((getRect().getWidth() - (F32)getRect().getHeight() * mAspectRatio) * 0.5f);
		rect.mLeft += half_width_diff;
		rect.mRight -= width_diff - half_width_diff;
	}
	mShineAnimTimer.stop();
	if (new_snapshot)
	{
		mSnapshotDelayTimer.start(delay);
	}
	if(new_thumbnail)
	{
		mThumbnailUpToDate = FALSE ;
	}
	setThumbnailImageSize();
}
void LLSnapshotLivePreview::setSnapshotQuality(S32 quality)
{
	llclamp(quality, 0, 100);
	if (quality != mSnapshotQuality)
	{
		mSnapshotQuality = quality;
		gSavedSettings.setS32("SnapshotQuality", quality);
	}
}
void LLSnapshotLivePreview::drawPreviewRect(S32 offset_x, S32 offset_y)
{
	LLColor4 alpha_color(0.5f, 0.5f, 0.5f, 0.8f);
	if (mThumbnailWidth > mThumbnailPreviewRect.getWidth())
	{
		gl_rect_2d(                      offset_x,                        offset_y,
				   mThumbnailPreviewRect.mLeft  + offset_x, mThumbnailHeight     + offset_y,
				   alpha_color, TRUE);
		gl_rect_2d(mThumbnailPreviewRect.mRight + offset_x,                        offset_y,
				   mThumbnailWidth     + offset_x, mThumbnailHeight     + offset_y,
				   alpha_color, TRUE);
	}
	if (mThumbnailHeight > mThumbnailPreviewRect.getHeight())
	{
		gl_rect_2d(                      offset_x,                        offset_y,
				   mThumbnailWidth     + offset_x, mThumbnailPreviewRect.mBottom + offset_y,
				   alpha_color, TRUE);
		gl_rect_2d(                      offset_x, mThumbnailPreviewRect.mTop    + offset_y,
				   mThumbnailWidth     + offset_x, mThumbnailHeight     + offset_y,
				   alpha_color, TRUE);
	}
	F32 line_width = gGL.getLineWidth();
	gGL.setLineWidth(2.0f * line_width);
	gl_rect_2d( mThumbnailPreviewRect.mLeft  + offset_x, mThumbnailPreviewRect.mTop    + offset_y,
				mThumbnailPreviewRect.mRight + offset_x, mThumbnailPreviewRect.mBottom + offset_y,
				LLColor4::black, FALSE);
	gGL.setLineWidth(line_width);
}
void LLSnapshotLivePreview::draw()
{
	if (mFullScreenPreviewTexture.notNull() &&
	    mShowFreezeFrameSnapshot)
	{
		LLColor4 bg_color(0.f, 0.f, 0.3f, 0.4f);
		gl_rect_2d(getRect(), bg_color);
		LLRect const& rect = mFullScreenImageRect;
		LLRect shadow_rect = mFullScreenImageRect;
		shadow_rect.stretch(BORDER_WIDTH);
		gl_drop_shadow(shadow_rect.mLeft, shadow_rect.mTop, shadow_rect.mRight, shadow_rect.mBottom, LLColor4(0.f, 0.f, 0.f, mNeedsFlash ? 0.f :0.5f), 10);
		LLColor4 image_color(1.f, 1.f, 1.f, 1.f);
		gGL.color4fv(image_color.mV);
		gGL.getTexUnit(0)->bind(mFullScreenPreviewTexture);
		F32 uv_width = mFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFormattedWidth / (F32)mFullScreenPreviewTexture->getWidth(), 1.f) : 1.f;
		F32 uv_height = mFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFormattedHeight / (F32)mFullScreenPreviewTexture->getHeight(), 1.f) : 1.f;
		gGL.pushMatrix();
		{
			gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom, 0.f);
			gGL.begin(LLRender::TRIANGLE_STRIP);
			{
				gGL.texCoord2f(uv_width, uv_height);
				gGL.vertex2i(rect.getWidth(), rect.getHeight() );
				gGL.texCoord2f(0.f, uv_height);
				gGL.vertex2i(0, rect.getHeight() );
				gGL.texCoord2f(uv_width, 0.f);
				gGL.vertex2i(rect.getWidth(), 0);
				gGL.texCoord2f(0.f, 0.f);
				gGL.vertex2i(0, 0);
			}
			gGL.end();
		}
		gGL.popMatrix();
		gGL.color4f(1.f, 1.f, 1.f, mFlashAlpha);
		gl_rect_2d(getRect());
		if (mNeedsFlash)
		{
			if (mFlashAlpha < 1.f)
			{
				mFlashAlpha = lerp(mFlashAlpha, 1.f, LLSmoothInterpolation::getInterpolant(0.02f));
			}
			else
			{
				mNeedsFlash = FALSE;
			}
		}
		else
		{
			mFlashAlpha = lerp(mFlashAlpha, 0.f, LLSmoothInterpolation::getInterpolant(0.15f));
		}
		if (mShineCountdown > 0)
		{
			mShineCountdown--;
			if (mShineCountdown == 0)
			{
				mShineAnimTimer.start();
			}
		}
		else if (mShineAnimTimer.getStarted())
		{
			F32 shine_interp = llmin(1.f, mShineAnimTimer.getElapsedTimeF32() / SHINE_TIME);
			LLLocalClipRect clip(getLocalRect());
			{
				S32 x1 = gViewerWindow->getWindowWidthScaled() * ll_round((clamp_rescale(shine_interp, 0.f, 1.f, -1.f - SHINE_WIDTH, 1.f)));
				S32 x2 = x1 + ll_round(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 x3 = x2 + ll_round(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 y1 = 0;
				S32 y2 = gViewerWindow->getWindowHeightScaled();
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gGL.begin(LLRender::TRIANGLE_STRIP);
				{
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x1 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x1, y1);
					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x2, y1);
					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x2, y1);
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x3 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x3, y1);
				}
				gGL.end();
			}
			if (shine_interp >= 1.f)
			{
				mShineAnimTimer.stop();
			}
		}
	}
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		LLRect outline_rect = mFullScreenImageRect;
		gGL.begin(LLRender::TRIANGLE_STRIP);
		{
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);
		}
		gGL.end();
	}
	if (mFallAnimTimer.getStarted())
	{
		if (mFallFullScreenPreviewTexture.notNull() && mFallAnimTimer.getElapsedTimeF32() < FALL_TIME)
		{
			F32 fall_interp = mFallAnimTimer.getElapsedTimeF32() / FALL_TIME;
			F32 alpha = clamp_rescale(fall_interp, 0.f, 1.f, 0.8f, 0.4f);
			LLColor4 image_color(1.f, 1.f, 1.f, alpha);
			gGL.color4fv(image_color.mV);
			gGL.getTexUnit(0)->bind(mFallFullScreenPreviewTexture);
			F32 uv_width = mFallFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFallWidth / (F32)mFallFullScreenPreviewTexture->getWidth(), 1.f) : 1.f;
			F32 uv_height = mFallFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFallHeight / (F32)mFallFullScreenPreviewTexture->getHeight(), 1.f) : 1.f;
			gGL.pushMatrix();
			{
				LLRect const& rect = mFallFullScreenImageRect;
				gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom - ll_round(getRect().getHeight() * 2.f * (fall_interp * fall_interp)), 0.f);
				gGL.rotatef(-45.f * fall_interp, 0.f, 0.f, 1.f);
				gGL.begin(LLRender::TRIANGLE_STRIP);
				{
					gGL.texCoord2f(uv_width, uv_height);
					gGL.vertex2i(rect.getWidth(), rect.getHeight() );
					gGL.texCoord2f(0.f, uv_height);
					gGL.vertex2i(0, rect.getHeight() );
					gGL.texCoord2f(uv_width, 0.f);
					gGL.vertex2i(rect.getWidth(), 0);
					gGL.texCoord2f(0.f, 0.f);
					gGL.vertex2i(0, 0);
				}
				gGL.end();
			}
			gGL.popMatrix();
		}
	}
}
void LLSnapshotLivePreview::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect old_rect = getRect();
	LLView::reshape(width, height, called_from_parent);
	if (old_rect.getWidth() != width || old_rect.getHeight() != height)
	{
		updateSnapshot(FALSE, TRUE);
	}
}
BOOL LLSnapshotLivePreview::setThumbnailImageSize()
{
	if(mWidth < 10 || mHeight < 10)
	{
		return FALSE ;
	}
	S32 window_width = gViewerWindow->getWindowWidthRaw() ;
	S32 window_height = gViewerWindow->getWindowHeightRaw() ;
	F32 window_aspect_ratio = ((F32)window_width) / ((F32)window_height);
	S32 max_width =  THUMBHEIGHT * 4 / 3;
	S32 max_height = THUMBHEIGHT;
	if (window_aspect_ratio > (F32)max_width / max_height)
	{
		mThumbnailWidth = max_width;
		mThumbnailHeight = ll_round((F32)max_width / window_aspect_ratio);
	}
	else
	{
		mThumbnailHeight = max_height;
		mThumbnailWidth = ll_round((F32)max_height * window_aspect_ratio);
	}
	if(mThumbnailWidth > window_width || mThumbnailHeight > window_height)
	{
		return FALSE ;
	}
	S32 left = 0 , top = mThumbnailHeight, right = mThumbnailWidth, bottom = 0 ;
	F32 ratio = mAspectRatio * window_height / window_width;
	if(ratio > 1.f)
	{
		top = ll_round(top / ratio);
	}
	else
	{
		right = ll_round(right * ratio);
	}
	left = (mThumbnailWidth - right + 1) / 2;
	bottom = (mThumbnailHeight - top + 1) / 2;
	top += bottom ;
	right += left ;
	mThumbnailPreviewRect.set(left - 1, top + 1, right + 1, bottom - 1) ;
	return TRUE ;
}
void LLSnapshotLivePreview::generateThumbnailImage(void)
{
	if(mThumbnailUpdateLock)
	{
		return ;
	}
	if(mThumbnailUpToDate)
	{
		return ;
	}
	if(mWidth < 10 || mHeight < 10)
	{
		return ;
	}
	mThumbnailUpdateLock = TRUE ;
	if(!setThumbnailImageSize())
	{
		mThumbnailUpdateLock = FALSE ;
		mThumbnailUpToDate = TRUE ;
		return ;
	}
	Dout(dc::snapshot, "LLSnapshotLivePreview::generateThumbnailImage: Actually making a new thumbnail!");
	mThumbnailImage = NULL;
	S32 w , h ;
	w = get_lower_power_two(mThumbnailWidth - 1, 512) * 2 ;
	h = get_lower_power_two(mThumbnailHeight - 1, 512) * 2 ;
	LLPointer<LLImageRaw> raw = new LLImageRaw ;
	if(!gViewerWindow->thumbnailSnapshot(raw,
							w, h,
							gSavedSettings.getBOOL("RenderUIInSnapshot"),
							FALSE,
							mSnapshotBufferType) )
	{
		raw = NULL ;
	}
	if(raw)
	{
		mThumbnailImage = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
		mThumbnailUpToDate = TRUE ;
		Dout(dc::snapshot, "thumbnailSnapshot(" << w << ", " << h << ", ...) returns " << raw->getWidth() << ", " << raw->getHeight());
	}
	mThumbnailUpdateLock = FALSE ;
}
BOOL LLSnapshotLivePreview::onIdle(LLSnapshotLivePreview* previewp)
{
	LLVector3 new_camera_pos = LLViewerCamera::getInstance()->getOrigin();
	LLQuaternion new_camera_rot = LLViewerCamera::getInstance()->getQuaternion();
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if (freeze_time &&
		(new_camera_pos != previewp->mCameraPos || dot(new_camera_rot, previewp->mCameraRot) < 0.995f))
	{
		previewp->mCameraPos = new_camera_pos;
		previewp->mCameraRot = new_camera_rot;
		previewp->showFreezeFrameSnapshot(false);
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		previewp->updateSnapshot(
			autosnap,
			FALSE,
			autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f);
	}
	previewp->mSnapshotActive =
		(previewp->mSnapshotDelayTimer.getStarted() && previewp->mSnapshotDelayTimer.hasExpired())
		&& !LLToolCamera::getInstance()->hasMouseCapture();
	if (!previewp->mSnapshotActive)
	{
		if (previewp->mFormattedDelayTimer.getStarted() && previewp->mFormattedDelayTimer.hasExpired())
		{
			previewp->mFormattedDelayTimer.stop();
			LLFloaterSnapshot::updateControls();
		}
		return FALSE;
	}
	LLFloaterSnapshot::resetFeedAndPostcardAspect();
	if (!previewp->mRawSnapshot)
	{
		previewp->mRawSnapshot = new LLImageRaw;
	}
	previewp->setVisible(FALSE);
	previewp->setEnabled(FALSE);
	previewp->getWindow()->incBusyCount();
	Dout(dc::snapshot, "LLSnapshotLivePreview::onIdle: Actually making a new snapshot!");
	if (previewp->mCloseCalled)
	{
		previewp->mCloseCalled->setEnabled(TRUE);
		previewp->mCloseCalled->setVisible(TRUE);
	}
	previewp->sSnapshotIndex++;
	Dout(dc::snapshot, "sSnapshotIndex is now " << previewp->sSnapshotIndex << "; mOutstandingCallbacks reset to 0.");
	previewp->mOutstandingCallbacks = 0;
	previewp->mSaveFailures = 0;
	previewp->mFormattedImage = NULL;
	previewp->mFormattedUpToDate = false;
	previewp->mUsedBy = 0;
	previewp->mManualSizeOverride = 0;
	previewp->mThumbnailUpToDate = FALSE;
	previewp->mRawSnapshotRenderUI = gSavedSettings.getBOOL("RenderUIInSnapshot");
	previewp->mRawSnapshotRenderHUD = gSavedSettings.getBOOL("RenderHUDInSnapshot");
	previewp->mRawSnapshotBufferType = previewp->mSnapshotBufferType;
	previewp->mRawSnapshotWidth = 0;
	previewp->mRawSnapshotHeight = 1;
	if (gViewerWindow->rawRawSnapshot(
							previewp->mRawSnapshot,
							previewp->mWidth,
							previewp->mHeight,
							previewp->mAspectRatio,
							previewp->mRawSnapshotRenderUI,
							FALSE,
							previewp->mRawSnapshotBufferType,
							previewp->getMaxImageSize(), 1.f, true))
	{
		previewp->mRawSnapshotWidth = previewp->mRawSnapshot->getWidth();
		previewp->mRawSnapshotHeight = previewp->mRawSnapshot->getHeight();
		Dout(dc::snapshot, "Created a new raw snapshot of size " << previewp->mRawSnapshotWidth << "x" << previewp->mRawSnapshotHeight);
		previewp->mPosTakenGlobal = gAgentCamera.getCameraPositionGlobal();
		EAspectSizeProblem ret = previewp->generateFormattedAndFullscreenPreview();
		llassert(previewp->mFormattedUpToDate || ret == SIZE_TOO_LARGE || ret == ENCODING_FAILED);
		if (!previewp->mFormattedUpToDate && ret == SIZE_TOO_LARGE)
		{
			LLNotificationsUtil::add("ErrorSizeAspectSnapshot");
		}
    }
	previewp->getWindow()->decBusyCount();
	previewp->setVisible(gSavedSettings.getBOOL("FreezeTime"));
	previewp->mSnapshotDelayTimer.stop();
	previewp->mSnapshotActive = FALSE;
	previewp->generateThumbnailImage();
	return TRUE;
}
LLSnapshotLivePreview::EAspectSizeProblem LLSnapshotLivePreview::getAspectSizeProblem(S32& width_out, S32& height_out, bool& crop_vertically_out, S32& crop_offset_out)
{
	S32 const window_width = gViewerWindow->getWindowWidthRaw();
	S32 const window_height = gViewerWindow->getWindowHeightRaw();
	F32 const window_aspect = (F32)window_width / window_height;
	F32 raw_aspect = (F32)mRawSnapshotWidth / mRawSnapshotHeight;
	F32 lower_raw_aspect = (mRawSnapshotWidth - 0.5) / (mRawSnapshotHeight + 0.5);
	F32 upper_raw_aspect = (mRawSnapshotWidth + 0.5) / (mRawSnapshotHeight - 0.5);
	bool const allow_vertical_crop = window_height * upper_raw_aspect >= window_width;
	bool const allow_horizontal_crop = window_width / lower_raw_aspect >= window_height;
	if (lower_raw_aspect <= window_aspect && window_aspect <= upper_raw_aspect)
	{
	  llassert(allow_vertical_crop && allow_horizontal_crop);
	  raw_aspect = window_aspect;
	}
	crop_vertically_out = true;
	crop_offset_out = 0;
	width_out = mRawSnapshotWidth;
	height_out = mRawSnapshotHeight;
	if (mAspectRatio < lower_raw_aspect)
	{
		width_out = ll_round(width_out * mAspectRatio / raw_aspect);
		if (width_out < mRawSnapshotWidth)
		{
			crop_vertically_out = false;
			if (!allow_horizontal_crop)
			{
				Dout(dc::snapshot, "NOT up to date: required aspect " << mAspectRatio <<
					" is less than the (lower) raw aspect " << lower_raw_aspect << " which is already vertically cropped.");
				return CANNOT_CROP_HORIZONTALLY;
			}
			crop_offset_out = (mRawSnapshotWidth - width_out) / 2;
		}
	}
	else if (mAspectRatio > upper_raw_aspect)
	{
		height_out = ll_round(height_out * raw_aspect / mAspectRatio);
		if (height_out < mRawSnapshotHeight)
		{
			if (!allow_vertical_crop)
			{
				Dout(dc::snapshot, "NOT up to date: required aspect " << mAspectRatio <<
					" is larger than the (upper) raw aspect " << upper_raw_aspect << " which is already horizontally cropped.");
				return CANNOT_CROP_VERTICALLY;
			}
			crop_offset_out = (mRawSnapshotHeight - height_out) / 2;
		}
	}
	if (mWidth > width_out || mHeight > height_out)
	{
		Dout(dc::snapshot, "NOT up to date: required target size " << mWidth << "x" << mHeight <<
			" is larger than the raw snapshot with size " << width_out << "x" << height_out << "!");
		return SIZE_TOO_LARGE;
	}
	if (mSnapshotType == SNAPSHOT_LOCAL && (mWidth >= window_width || mHeight >= window_height) && mWidth != width_out && mHeight != height_out)
	{
		Dout(dc::snapshot, "NOT up to date: required target size " << mWidth << "x" << mHeight <<
			" is larger or equal the window size (" << window_width << "x" << window_height << ")"
			" but unequal the the raw snapshot size (" << width_out << "x" << height_out << ")"
			" and target is disk!");
		return CANNOT_RESIZE;
	}
	return ASPECTSIZE_OK;
}
LLSnapshotLivePreview::EAspectSizeProblem LLSnapshotLivePreview::generateFormattedAndFullscreenPreview(bool delayed)
{
	DoutEntering(dc::snapshot, "LLSnapshotLivePreview::generateFormattedAndFullscreenPreview(" << delayed << ")");
	mFormattedUpToDate = false;
	S32 w, h, crop_offset;
	bool crop_vertically;
	EAspectSizeProblem ret = getAspectSizeProblem(w, h, crop_vertically, crop_offset);
	if (ret != ASPECTSIZE_OK)
	{
		return ret;
	}
	LLFloaterSnapshot::ESnapshotFormat format;
	switch (mSnapshotType)
	{
	  case SNAPSHOT_FEED:
		format = LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG;
		break;
	  case SNAPSHOT_POSTCARD:
		format = LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG;
		break;
	  case SNAPSHOT_TEXTURE:
		format = LLFloaterSnapshot::SNAPSHOT_FORMAT_J2C;
		break;
	  case SNAPSHOT_LOCAL:
		format = mSnapshotFormat;
		break;
	  default:
		format = mSnapshotFormat;
		break;
	}
	if (mFormattedImage &&
		mFormattedWidth == mWidth &&
		mFormattedHeight == mHeight &&
		mFormattedRawWidth == w &&
		mFormattedRawHeight == h &&
		mFormattedCropOffset == crop_offset &&
		mFormattedCropVertically == crop_vertically &&
		mFormattedSnapshotFormat == format &&
		(mFormattedSnapshotQuality == mSnapshotQuality ||
		 format != LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG))
	{
		Dout(dc::snapshot, "Already up to date.");
		mFormattedUpToDate = true;
		return ret;
	}
#ifdef CWDEBUG
	if (!mFormattedImage)
	{
		Dout(dc::snapshot, "mFormattedImage == NULL!");
	}
	else
	{
		if (mFormattedWidth != mWidth)
		  Dout(dc::snapshot, "Target width changed from " << mFormattedWidth << " to " << mWidth);
		if (mFormattedHeight != mHeight)
		  Dout(dc::snapshot, "Target height changed from " << mFormattedHeight << " to " << mHeight);
		if (mFormattedRawWidth != w)
		  Dout(dc::snapshot, "Cropped (raw) width changed from " << mFormattedRawWidth << " to " << w);
		if (mFormattedRawHeight != h)
		  Dout(dc::snapshot, "Cropped (raw) height changed from " << mFormattedRawHeight << " to " << h);
		if (mFormattedCropOffset != crop_offset)
		  Dout(dc::snapshot, "Crop offset changed from " << mFormattedCropOffset << " to " << crop_offset);
		if (mFormattedCropVertically != crop_vertically)
		  Dout(dc::snapshot, "Crop direction changed from " << (mFormattedCropVertically ? "vertical" : "horizontal") << " to " << (crop_vertically ? "vertical" : "horizontal"));
		if (mFormattedSnapshotFormat != format)
		  Dout(dc::snapshot, "Format changed from " << mFormattedSnapshotFormat << " to " << format);
		if (!(mFormattedSnapshotQuality == mSnapshotQuality || format != LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG))
		  Dout(dc::snapshot, "Format is JPEG and quality changed from " << mFormattedSnapshotQuality << " to " << mSnapshotQuality);
	}
#endif
	if (delayed)
	{
		Dout(dc::snapshot, "NOT up to date, but delayed. Returning.");
		return DELAYED;
	}
	if (!mRawSnapshot)
	{
		Dout(dc::snapshot, "No raw snapshot exists.");
		return NO_RAW_IMAGE;
	}
	Dout(dc::snapshot, "Generating a new formatted image!");
	mFormattedWidth = mWidth;
	mFormattedHeight = mHeight;
	mFormattedRawWidth = w;
	mFormattedRawHeight = h;
	mFormattedCropOffset = crop_offset;
	mFormattedCropVertically = crop_vertically;
	mFormattedSnapshotFormat = format;
	mFormattedSnapshotQuality = mSnapshotQuality;
	mFormattedDelayTimer.stop();
	LLPointer<LLImageRaw> scaled = new LLImageRaw(mRawSnapshot, w, h, crop_offset, crop_vertically);
	LLPointer<LLImageRaw> decoded = new LLImageRaw;
	if (mSnapshotType == SNAPSHOT_TEXTURE)
	{
		scaled->biasedScaleToPowerOfTwo(mWidth, mHeight, 1024);
	}
	else
	{
		scaled->scale(mWidth, mHeight);
	}
	bool lossless = false;
	switch(format)
	{
	  case LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG:
		mFormattedImage = new LLImagePNG;
		lossless = true;
		break;
	  case LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG:
		mFormattedImage = new LLImageJPEG(mSnapshotQuality);
		break;
	  case LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP:
		mFormattedImage = new LLImageBMP;
		lossless = true;
		break;
	  case LLFloaterSnapshot::SNAPSHOT_FORMAT_J2C:
		mFormattedImage = new LLImageJ2C;
		break;
	}
	if (mFormattedImage->encode(scaled, 0))
	{
		mFormattedDataSize = mFormattedImage->getDataSize();
		if (!lossless)
		{
			decoded->resize(
				mFormattedImage->getWidth(),
				mFormattedImage->getHeight(),
				mFormattedImage->getComponents());
			mFormattedImage->decode(decoded, 0);
		}
	}
	else
	{
		mFormattedDataSize = 0;
		mFormattedImage = NULL;
		ret = ENCODING_FAILED;
		LLNotificationsUtil::add("ErrorEncodingSnapshot");
	}
	if (!lossless)
	{
		scaled = NULL;
		scaled = new LLImageRaw(
			decoded->getData(),
			decoded->getWidth(),
			decoded->getHeight(),
			decoded->getComponents());
	}
	if (!scaled->isBufferInvalid())
	{
		if (scaled->getWidth() > 1024 || scaled->getHeight() > 1024)
		{
			scaled->biasedScaleToPowerOfTwo(1024);
			mFullScreenPreviewTextureNeedsScaling = FALSE;
		}
		else
		{
			scaled->expandToPowerOfTwo(1024, FALSE);
			mFullScreenPreviewTextureNeedsScaling = TRUE;
		}
		mFullScreenPreviewTexture = LLViewerTextureManager::getLocalTexture(scaled.get(), FALSE);
		LLPointer<LLViewerTexture> curr_preview_texture = mFullScreenPreviewTexture;
		gGL.getTexUnit(0)->bind(curr_preview_texture);
		if (mSnapshotType != SNAPSHOT_TEXTURE)
		{
			curr_preview_texture->setFilteringOption(LLTexUnit::TFO_POINT);
		}
		else
		{
			curr_preview_texture->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
		}
		curr_preview_texture->setAddressMode(LLTexUnit::TAM_CLAMP);
		showFreezeFrameSnapshot(TRUE);
		mShineCountdown = 4;
	}
	mFormattedUpToDate = mFormattedImage;
	return ret;
}
void LLSnapshotLivePreview::setSize(S32 w, S32 h)
{
	mWidth = w;
	mHeight = h;
}
void LLSnapshotLivePreview::getSize(S32& w, S32& h) const
{
	w = mWidth;
	h = mHeight;
}
void LLSnapshotLivePreview::setAspect(F32 a)
{
	mAspectRatio = a;
}
F32 LLSnapshotLivePreview::getAspect() const
{
	return mAspectRatio;
}
void LLSnapshotLivePreview::getRawSize(S32& w, S32& h) const
{
	w = mRawSnapshotWidth;
	h = mRawSnapshotHeight;
}
LLFloaterFeed* LLSnapshotLivePreview::getCaptionAndSaveFeed()
{
	if (mCloseCalled)
	{
		return NULL;
	}
	++mOutstandingCallbacks;
	mSaveFailures = 0;
	addUsedBy(SNAPSHOT_FEED);
	Dout(dc::snapshot, "LLSnapshotLivePreview::getCaptionAndSaveFeed: sSnapshotIndex = " << sSnapshotIndex << "; mOutstandingCallbacks = " << mOutstandingCallbacks << ".");
	if (mFullScreenPreviewTexture.isNull())
	{
		LL_WARNS() << "The snapshot image has not been generated!" << LL_ENDL;
		saveDone(SNAPSHOT_FEED, false, sSnapshotIndex);
		return NULL;
	}
	F32 uv_width = mFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFormattedWidth / (F32)mFullScreenPreviewTexture->getWidth(), 1.f) : 1.f;
	F32 uv_height = mFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFormattedHeight / (F32)mFullScreenPreviewTexture->getHeight(), 1.f) : 1.f;
	LLVector2 image_scale(uv_width, uv_height);
	LLImagePNG* png = dynamic_cast<LLImagePNG*>(mFormattedImage.get());
	if (!png)
	{
		LL_WARNS() << "Formatted image not a PNG" << LL_ENDL;
		saveDone(SNAPSHOT_FEED, false, sSnapshotIndex);
		return NULL;
	}
	LLFloaterFeed* floater = LLFloaterFeed::showFromSnapshot(png, mFullScreenPreviewTexture, image_scale, sSnapshotIndex);
	return floater;
}
LLFloaterPostcard* LLSnapshotLivePreview::savePostcard()
{
	if (mCloseCalled)
	{
		return NULL;
	}
	++mOutstandingCallbacks;
	mSaveFailures = 0;
	addUsedBy(SNAPSHOT_POSTCARD);
	Dout(dc::snapshot, "LLSnapshotLivePreview::savePostcard: sSnapshotIndex = " << sSnapshotIndex << "; mOutstandingCallbacks = " << mOutstandingCallbacks << ".");
	if(mFullScreenPreviewTexture.isNull())
	{
		LL_WARNS() << "The snapshot image has not been generated!" << LL_ENDL ;
		saveDone(SNAPSHOT_POSTCARD, false, sSnapshotIndex);
		return NULL ;
	}
	F32 uv_width = mFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFormattedWidth / (F32)mFullScreenPreviewTexture->getWidth(), 1.f) : 1.f;
	F32 uv_height = mFullScreenPreviewTextureNeedsScaling ? llmin((F32)mFormattedHeight / (F32)mFullScreenPreviewTexture->getHeight(), 1.f) : 1.f;
	LLVector2 image_scale(uv_width, uv_height);
	LLImageJPEG* jpg = dynamic_cast<LLImageJPEG*>(mFormattedImage.get());
	if(!jpg)
	{
		LL_WARNS() << "Formatted image not a JPEG" << LL_ENDL;
		saveDone(SNAPSHOT_POSTCARD, false, sSnapshotIndex);
		return NULL;
	}
	LLFloaterPostcard* floater = LLFloaterPostcard::showFromSnapshot(jpg, mFullScreenPreviewTexture, image_scale, mPosTakenGlobal, sSnapshotIndex);
	return floater;
}
class saveTextureUserData {
public:
	saveTextureUserData(LLSnapshotLivePreview* self, int index, bool temporary) : mSelf(self), mSnapshotIndex(index), mTemporary(temporary) { }
	LLSnapshotLivePreview* mSelf;
	int mSnapshotIndex;
	bool mTemporary;
};
void LLSnapshotLivePreview::saveTexture()
{
	if (mCloseCalled)
	{
		return;
	}
	++mOutstandingCallbacks;
	mSaveFailures = 0;
	addUsedBy(SNAPSHOT_TEXTURE);
	Dout(dc::snapshot, "LLSnapshotLivePreview::saveTexture: sSnapshotIndex = " << sSnapshotIndex << "; mOutstandingCallbacks = " << mOutstandingCallbacks << ".");
	saveStart(sSnapshotIndex);
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	LLVFile::writeFile(mFormattedImage->getData(), mFormattedImage->getDataSize(), gVFS, new_asset_id, LLAssetType::AT_TEXTURE);
	std::string pos_string;
	LLAgentUI::buildLocationString(pos_string, LLAgentUI::LOCATION_FORMAT_FULL);
	std::string who_took_it;
	LLAgentUI::buildFullname(who_took_it);
	LLAssetStorage::LLStoreAssetCallback callback = &LLSnapshotLivePreview::saveTextureDone;
	S32 expected_upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
	saveTextureUserData* user_data = new saveTextureUserData(this, sSnapshotIndex, gSavedSettings.getBOOL("TemporaryUpload"));
	if (upload_new_resource(tid,
				LLAssetType::AT_TEXTURE,
				"Snapshot : " + pos_string,
				"Taken by " + who_took_it + " at " + pos_string,
				0,
				LLFolderType::FT_SNAPSHOT_CATEGORY,
				LLInventoryType::IT_SNAPSHOT,
				PERM_ALL,
				LLFloaterPerms::getGroupPerms("Uploads"),
				LLFloaterPerms::getEveryonePerms("Uploads"),
				"Snapshot : " + pos_string,
				callback, expected_upload_cost, user_data))
	{
		saveTextureDone2(true, user_data);
	}
	else
	{
		delete user_data;
		saveDone(SNAPSHOT_TEXTURE, false, sSnapshotIndex);
	}
	gViewerWindow->playSnapshotAnimAndSound();
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_SNAPSHOT_COUNT );
}
void LLSnapshotLivePreview::saveLocal()
{
	if (mCloseCalled)
	{
		return;
	}
	++mOutstandingCallbacks;
	mSaveFailures = 0;
	addUsedBy(SNAPSHOT_LOCAL);
	Dout(dc::snapshot, "LLSnapshotLivePreview::saveLocal: sSnapshotIndex = " << sSnapshotIndex << "; mOutstandingCallbacks = " << mOutstandingCallbacks << ".");
	saveStart(sSnapshotIndex);
	gViewerWindow->saveImageNumbered(mFormattedImage, sSnapshotIndex);
}
void LLSnapshotLivePreview::close(LLFloaterSnapshot* view)
{
	DoutEntering(dc::snapshot, "LLSnapshotLivePreview::close(" << (void*)view << ") [mOutstandingCallbacks = " << mOutstandingCallbacks << "]");
	mCloseCalled = view;
	if (!mOutstandingCallbacks)
	{
		doCloseAfterSave();
	}
	else
	{
		view->setVisible(FALSE);
		view->setEnabled(FALSE);
	}
}
void LLSnapshotLivePreview::saveStart(int index)
{
	if (index == sSnapshotIndex && gSavedSettings.getBOOL("CloseSnapshotOnKeep") && gSavedSettings.getBOOL("FreezeTime"))
	{
		LLFloaterSnapshot::Impl::freezeTime(false);
	}
}
void LLSnapshotLivePreview::saveDone(ESnapshotType type, bool success, int index)
{
	DoutEntering(dc::snapshot, "LLSnapshotLivePreview::saveDone(" << type << ", " << success << ", " << index << ")");
	if (sSnapshotIndex != index)
	{
		Dout(dc::snapshot, "sSnapshotIndex (" << sSnapshotIndex << ") != index (" << index << ")");
		if (!success)
		{
			LL_WARNS() << "Permanent failure to upload or save snapshot" << LL_ENDL;
		}
		return;
	}
	--mOutstandingCallbacks;
	Dout(dc::snapshot, "sSnapshotIndex = " << sSnapshotIndex << "; mOutstandingCallbacks = " << mOutstandingCallbacks << ".");
	if (!success)
	{
		++mSaveFailures;
		delUsedBy(type);
		LLFloaterSnapshot::updateControls();
	}
	if (!mOutstandingCallbacks)
	{
		doCloseAfterSave();
	}
}
void LLSnapshotLivePreview::saveTextureDone(LLUUID const& asset_id, void* user_data, S32 status, LLExtStat ext_status)
{
	LLResourceData* resource_data = (LLResourceData*)user_data;
	bool success = status == LL_ERR_NOERR;
	if (!success)
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("UploadSnapshotFail", args);
	}
	saveTextureUserData* data = (saveTextureUserData*)resource_data->mUserData;
	bool temporary = data->mTemporary;
	data->mSelf->saveDone(SNAPSHOT_TEXTURE, success, data->mSnapshotIndex);
	delete data;
	LLAssetStorage::LLStoreAssetCallback asset_callback = temporary ? &temp_upload_callback : &upload_done_callback;
	asset_callback(asset_id, user_data, status, ext_status);
}
void LLSnapshotLivePreview::saveTextureDone2(bool success, void* user_data)
{
	saveTextureUserData* data = (saveTextureUserData*)user_data;
	data->mSelf->saveDone(SNAPSHOT_TEXTURE, success, data->mSnapshotIndex);
	delete data;
}
void LLSnapshotLivePreview::doCloseAfterSave()
{
	if (!mCloseCalled)
	{
		return;
	}
	if (!mSaveFailures && gSavedSettings.getBOOL("CloseSnapshotOnKeep"))
	{
		mFormattedImage = NULL;
		mFormattedUpToDate = false;
		mFormattedDataSize = 0;
		updateSnapshot(FALSE, FALSE);
		mCloseCalled->close();
	}
	else
	{
		mCloseCalled->setEnabled(TRUE);
		mCloseCalled->setVisible(TRUE);
		gFloaterView->bringToFront(mCloseCalled);
		mCloseCalled = NULL;
	}
}
LLHandle<LLView> LLFloaterSnapshot::Impl::sPreviewHandle;
LLSnapshotLivePreview* LLFloaterSnapshot::Impl::getPreviewView(void)
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)sPreviewHandle.get();
	return previewp;
}
LLSnapshotLivePreview::ESnapshotType LLFloaterSnapshot::Impl::getTypeIndex(LLFloaterSnapshot* floater)
{
	return (LLSnapshotLivePreview::ESnapshotType)floater->childGetValue("snapshot_type_radio").asInteger();
}
LLFloaterSnapshot::ESnapshotFormat LLFloaterSnapshot::Impl::getFormatIndex(LLFloaterSnapshot* floater)
{
	ESnapshotFormat index = SNAPSHOT_FORMAT_PNG;
	LLSD value = floater->childGetValue("local_format_combo");
	const std::string id = value.asString();
	if (id == "PNG")
		index = SNAPSHOT_FORMAT_PNG;
	else if (id == "JPEG")
		index = SNAPSHOT_FORMAT_JPEG;
	else if (id == "BMP")
		index = SNAPSHOT_FORMAT_BMP;
	return index;
}
void LLFloaterSnapshot::Impl::setResolution(LLFloaterSnapshot* floater, const std::string& comboname, bool visible, bool update_controls)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
	combo->setVisible(visible);
	updateResolution(combo, floater, update_controls);
}
void LLFloaterSnapshot::Impl::setAspect(LLFloaterSnapshot* floater, const std::string& comboname, bool update_controls)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
	combo->setVisible(TRUE);
	updateAspect(combo, floater, update_controls);
}
void LLFloaterSnapshot::Impl::resetFeedAndPostcardAspect(LLFloaterSnapshot* floaterp)
{
	floaterp->getChild<LLComboBox>("feed_aspect_combo")->setCurrentByIndex(0);
	gSavedSettings.setS32("SnapshotFeedLastAspect", 0);
	floaterp->getChild<LLComboBox>("postcard_aspect_combo")->setCurrentByIndex(0);
	gSavedSettings.setS32("SnapshotPostcardLastAspect", 0);
}
void LLFloaterSnapshot::Impl::updateLayout(LLFloaterSnapshot* floaterp)
{
	S32 delta_height = 0;
	if (!gSavedSettings.getBOOL("AdvanceSnapshot"))
	{
		floaterp->getChild<LLComboBox>("feed_size_combo")->setCurrentByIndex(2);
		gSavedSettings.setS32("SnapshotFeedLastResolution", 2);
		floaterp->getChild<LLComboBox>("postcard_size_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotPostcardLastResolution", 0);
		resetFeedAndPostcardAspect(floaterp);
		floaterp->getChild<LLComboBox>("texture_size_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotTextureLastResolution", 0);
		floaterp->getChild<LLComboBox>("texture_aspect_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotTextureLastAspect", 0);
		floaterp->getChild<LLComboBox>("local_size_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotLocalLastResolution", 0);
		floaterp->getChild<LLComboBox>("local_aspect_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotLocalLastAspect", 0);
		updateControls(floaterp);
		delta_height = floaterp->getUIWinHeightShort() - floaterp->getUIWinHeightLong();
	}
	floaterp->reshape(floaterp->getRect().getWidth(), floaterp->getUIWinHeightLong() + delta_height);
}
void LLFloaterSnapshot::Impl::freezeTime(bool on)
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (on)
	{
		gSnapshotFloaterView->setMouseOpaque(TRUE);
		if (previewp)
		{
			previewp->setEnabled(TRUE);
			previewp->setVisible(TRUE);
		}
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			iter != LLCharacter::sInstances.end(); ++iter)
		{
			sInstance->impl.mAvatarPauseHandles.push_back((*iter)->requestPause());
		}
		gSavedSettings.setBOOL("FreezeTime", TRUE);
		if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset)
		{
			sInstance->impl.mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
			LLToolMgr::getInstance()->setCurrentToolset(gCameraToolset);
		}
		gFocusMgr.setDefaultKeyboardFocus(sInstance);
	}
	else if (gSavedSettings.getBOOL("FreezeTime"))
	{
		gFocusMgr.restoreDefaultKeyboardFocus(sInstance);
		gSnapshotFloaterView->setMouseOpaque(FALSE);
		if (previewp)
		{
			previewp->setVisible(FALSE);
			previewp->setEnabled(FALSE);
		}
		gSavedSettings.setBOOL("FreezeTime", FALSE);
		LLVOAvatar* avatarp;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin(); iter != LLCharacter::sInstances.end(); ++iter)
		{
			avatarp = static_cast<LLVOAvatar*>(*iter);
			avatarp->resetFreezeTime();
		}
		sInstance->impl.mAvatarPauseHandles.clear();
		if (sInstance->impl.mLastToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(sInstance->impl.mLastToolset);
		}
	}
}
void LLFloaterSnapshot::Impl::updateControls(LLFloaterSnapshot* floater, bool delayed_formatted)
{
	DoutEntering(dc::snapshot, "LLFloaterSnapshot::Impl::updateControls()");
	const HippoGridInfo& grid(*gHippoGridManager->getConnectedGrid());
	floater->childSetLabelArg("upload_btn", "[UPLOADFEE]", grid.formatFee(gSavedSettings.getBOOL("TemporaryUpload") ? 0 : LLAgentBenefitsMgr::current().getTextureUploadCost()));
	LLSnapshotLivePreview::ESnapshotType shot_type = (LLSnapshotLivePreview::ESnapshotType)gSavedSettings.getS32("LastSnapshotType");
	ESnapshotFormat shot_format = (ESnapshotFormat)gSavedSettings.getS32("SnapshotFormat");
	floater->childSetVisible("feed_size_combo", FALSE);
	floater->childSetVisible("feed_aspect_combo", FALSE);
	floater->childSetVisible("postcard_size_combo", FALSE);
	floater->childSetVisible("postcard_aspect_combo", FALSE);
	floater->childSetVisible("texture_size_combo", FALSE);
	floater->childSetVisible("texture_aspect_combo", FALSE);
	floater->childSetVisible("local_size_combo", FALSE);
	floater->childSetVisible("local_aspect_combo", FALSE);
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		LLViewerWindow::ESnapshotType layer_type =
			(shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL) ?
			(LLViewerWindow::ESnapshotType)gSavedSettings.getS32("SnapshotLayerType") :
			LLViewerWindow::SNAPSHOT_TYPE_COLOR;
		previewp->setSnapshotBufferType(floater, layer_type);
	}
	BOOL is_advance = gSavedSettings.getBOOL("AdvanceSnapshot");
	switch(shot_type)
	{
	  case LLSnapshotLivePreview::SNAPSHOT_FEED:
		setResolution(floater, "feed_size_combo", is_advance, false);
		break;
	  case LLSnapshotLivePreview::SNAPSHOT_POSTCARD:
		setResolution(floater, "postcard_size_combo", is_advance, false);
		break;
	  case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:
		setResolution(floater, "texture_size_combo", is_advance, false);
		break;
	  case  LLSnapshotLivePreview::SNAPSHOT_LOCAL:
		setResolution(floater, "local_size_combo", is_advance, false);
		break;
	}
	floater->getChild<LLComboBox>("local_format_combo")->selectNthItem(shot_format);
	floater->childSetVisible("upload_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_TEXTURE);
	floater->childSetVisible("send_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD);
	floater->childSetVisible("feed_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_FEED);
	floater->childSetVisible("save_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	floater->childSetEnabled("layer_types",			shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	BOOL is_local = shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL;
	BOOL show_slider =
		shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD ||
		(is_local && shot_format == LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG);
	floater->getChild<LLComboBox>(previewp->aspectComboName())->setVisible(is_advance);
	floater->childSetVisible("more_btn", !is_advance);
	floater->childSetVisible("less_btn",				is_advance);
	floater->childSetVisible("type_label2",				is_advance);
	floater->childSetVisible("keep_aspect",				is_advance);
	floater->childSetVisible("type_label3",				is_advance);
	floater->childSetVisible("format_label",			is_advance && is_local);
	floater->childSetVisible("local_format_combo",		is_advance && is_local);
	floater->childSetVisible("layer_types",				is_advance);
	floater->childSetVisible("layer_type_label",		is_advance);
	floater->childSetVisible("aspect_one_label",		is_advance);
	floater->childSetVisible("snapshot_width",			is_advance);
	floater->childSetVisible("snapshot_height",			is_advance);
	floater->childSetVisible("aspect_ratio",			is_advance);
	floater->childSetVisible("ui_check",				is_advance);
	floater->childSetVisible("hud_check",				is_advance);
	floater->childSetVisible("keep_open_check",			is_advance);
	floater->childSetVisible("freeze_time_check",		is_advance);
	floater->childSetVisible("auto_snapshot_check",		is_advance);
	floater->childSetVisible("image_quality_slider",	is_advance && show_slider);
	floater->childSetVisible("temp_check",				is_advance);
	BOOL got_bytes = previewp && previewp->getDataSize() > 0;
	BOOL is_texture = shot_type == LLSnapshotLivePreview::SNAPSHOT_TEXTURE;
	if (previewp)
	{
		previewp->setSnapshotFormat(shot_format);
		if (previewp->getRawSnapshotUpToDate())
		{
			if (delayed_formatted)
			{
				previewp->mFormattedDelayTimer.start(0.5);
			}
			previewp->generateFormattedAndFullscreenPreview(delayed_formatted);
		}
		else
		{
			previewp->mFormattedDelayTimer.stop();
		}
	}
	LLLocale locale(LLLocale::USER_LOCALE);
	std::string bytes_string;
	if (got_bytes)
	{
		LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10 );
	}
	else
	{
		bytes_string = floater->getString("unknown");
	}
	S32 upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
	floater->childSetLabelArg("texture", "[AMOUNT]", llformat("%d",upload_cost));
	floater->childSetLabelArg("upload_btn", "[AMOUNT]", llformat("%d",upload_cost));
	floater->childSetTextArg("file_size_label", "[SIZE]", bytes_string);
	floater->childSetColor("file_size_label",
		shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD
		&& got_bytes
		&& previewp->getDataSize() > MAX_POSTCARD_DATASIZE ? LLColor4::red : gColors.getColor( "LabelTextColor" ));
	std::string target_size_str = gSavedSettings.getBOOL(snapshotKeepAspectName()) ? floater->getString("sourceAR") : floater->getString("targetAR");
	floater->childSetValue("type_label3", target_size_str);
	bool up_to_date = previewp && previewp->getSnapshotUpToDate();
	bool can_upload = up_to_date && !previewp->isUsedBy(shot_type);
	floater->childSetEnabled("feed_btn",   shot_type == LLSnapshotLivePreview::SNAPSHOT_FEED     && can_upload);
	floater->childSetEnabled("send_btn",   shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD && can_upload && previewp->getDataSize() <= MAX_POSTCARD_DATASIZE);
	floater->childSetEnabled("upload_btn", shot_type == LLSnapshotLivePreview::SNAPSHOT_TEXTURE  && can_upload);
	floater->childSetEnabled("save_btn",   shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL    && can_upload);
	floater->childSetEnabled("temp_check", is_advance && is_texture);
	if (previewp)
	{
		previewp->showFreezeFrameSnapshot(up_to_date);
	}
}
void LLFloaterSnapshot::Impl::checkAutoSnapshot(LLSnapshotLivePreview* previewp, BOOL update_thumbnail)
{
	if (previewp)
	{
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		previewp->updateSnapshot(autosnap, update_thumbnail, autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f);
	}
}
void LLFloaterSnapshot::Impl::onClickDiscard(void* data)
{
	LLFloaterSnapshot* view = static_cast<LLFloaterSnapshot*>(data);
	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		LLSnapshotLivePreview* previewp = view->impl.getPreviewView();
		if (previewp && previewp->getShowFreezeFrameSnapshot())
			previewp->showFreezeFrameSnapshot(false);
		view->impl.freezeTime(false);
	}
	else
	{
		view->close();
	}
}
void LLFloaterSnapshot::Impl::onCommitFeedResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotFeedLastResolution", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_FEED);
	}
	updateResolution(ctrl, data);
}
void LLFloaterSnapshot::Impl::onCommitPostcardResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotPostcardLastResolution", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_POSTCARD);
	}
	updateResolution(ctrl, data);
}
void LLFloaterSnapshot::Impl::onCommitTextureResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotTextureLastResolution", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_TEXTURE);
	}
	updateResolution(ctrl, data);
}
void LLFloaterSnapshot::Impl::onCommitLocalResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotLocalLastResolution", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	}
	updateResolution(ctrl, data);
}
void LLFloaterSnapshot::Impl::onCommitFeedAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotFeedLastAspect", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_FEED);
	}
	updateAspect(ctrl, data);
	LLFloaterSnapshot* floater = (LLFloaterSnapshot*)data;
	if (floater && previewp && gSavedSettings.getBOOL(snapshotKeepAspectName()))
	{
		enforceResolution(floater, previewp->getAspect());
	}
}
void LLFloaterSnapshot::Impl::onCommitPostcardAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotPostcardLastAspect", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_POSTCARD);
	}
	updateAspect(ctrl, data);
	LLFloaterSnapshot* floater = (LLFloaterSnapshot*)data;
	if (floater && previewp && gSavedSettings.getBOOL(snapshotKeepAspectName()))
	{
		enforceResolution(floater, previewp->getAspect());
	}
}
void LLFloaterSnapshot::Impl::onCommitTextureAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotTextureLastAspect", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_TEXTURE);
	}
	updateAspect(ctrl, data);
}
void LLFloaterSnapshot::Impl::onCommitLocalAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotLocalLastAspect", combobox->getCurrentIndex());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	}
	updateAspect(ctrl, data);
	LLFloaterSnapshot* floater = (LLFloaterSnapshot*)data;
	if (floater && previewp && gSavedSettings.getBOOL(snapshotKeepAspectName()))
	{
		enforceResolution(floater, previewp->getAspect());
	}
}
void LLFloaterSnapshot::Impl::onCommitSave(LLUICtrl* ctrl, void* data)
{
	if (ctrl->getValue().asString() == "saveas")
	{
		gViewerWindow->resetSnapshotLoc();
	}
	onClickKeep(data);
}
void LLFloaterSnapshot::saveStart(int index)
{
	LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView();
	if (previewp)
	{
		previewp->saveStart(index);
	}
}
void LLFloaterSnapshot::saveLocalDone(bool success, int index)
{
	LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView();
	if (previewp)
	{
		previewp->saveDone(LLSnapshotLivePreview::SNAPSHOT_LOCAL, success, index);
	}
}
void LLFloaterSnapshot::saveFeedDone(bool success, int index)
{
	LLUploadDialog::modalUploadFinished();
	LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView();
	if (previewp)
	{
		previewp->saveDone(LLSnapshotLivePreview::SNAPSHOT_FEED, success, index);
	}
}
void LLFloaterSnapshot::savePostcardDone(bool success, int index)
{
	LLUploadDialog::modalUploadFinished();
	LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView();
	if (previewp)
	{
		previewp->saveDone(LLSnapshotLivePreview::SNAPSHOT_POSTCARD, success, index);
	}
}
void LLFloaterSnapshot::Impl::onClickKeep(void* data)
{
	LLFloaterSnapshot* floater = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_FEED)
		{
			LLFloaterFeed* floater = previewp->getCaptionAndSaveFeed();
			if (floater)
			{
				gSnapshotFloaterView->addChild(floater);
			}
		}
		else if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_POSTCARD)
		{
			LLFloaterPostcard* floater = previewp->savePostcard();
			if (floater)
			{
				gSnapshotFloaterView->addChild(floater);
			}
		}
		else if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_TEXTURE)
		{
			previewp->saveTexture();
		}
		else
		{
			previewp->saveLocal();
		}
		if (gSavedSettings.getBOOL("CloseSnapshotOnKeep"))
		{
			previewp->close(floater);
		}
		else
		{
			checkAutoSnapshot(previewp);
		}
		updateControls(floater);
	}
}
void LLFloaterSnapshot::Impl::onClickNewSnapshot(void* data)
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->updateSnapshot(TRUE);
	}
}
void LLFloaterSnapshot::Impl::onClickFreezeTime(void*)
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		freezeTime(true);
	}
}
void LLFloaterSnapshot::Impl::onClickAutoSnap(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "AutoSnapshot", check->get() );
	LLFloaterSnapshot* floater = (LLFloaterSnapshot*)data;
	if (floater)
	{
		checkAutoSnapshot(getPreviewView());
		updateControls(floater);
	}
}
void LLFloaterSnapshot::Impl::onClickTemporaryImage(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::onClickMore(void* data)
{
	gSavedSettings.setBOOL( "AdvanceSnapshot", TRUE );
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		view->translate( 0, view->getUIWinHeightShort() - view->getUIWinHeightLong() );
		view->reshape(view->getRect().getWidth(), view->getUIWinHeightLong());
		updateControls(view) ;
		updateLayout(view) ;
		if (getPreviewView())
		{
			getPreviewView()->setThumbnailImageSize() ;
		}
	}
}
void LLFloaterSnapshot::Impl::onClickLess(void* data)
{
	gSavedSettings.setBOOL( "AdvanceSnapshot", FALSE );
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		view->translate( 0, view->getUIWinHeightLong() - view->getUIWinHeightShort() );
		view->reshape(view->getRect().getWidth(), view->getUIWinHeightShort());
		updateControls(view) ;
		updateLayout(view) ;
		if (getPreviewView())
		{
			getPreviewView()->setThumbnailImageSize();
		}
	}
}
void LLFloaterSnapshot::Impl::onClickUICheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderUIInSnapshot", check->get() );
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		checkAutoSnapshot(getPreviewView(), TRUE);
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::onClickHUDCheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderHUDInSnapshot", check->get() );
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		checkAutoSnapshot(getPreviewView(), TRUE);
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::onClickKeepOpenCheck(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "CloseSnapshotOnKeep", !check->get() );
}
void LLFloaterSnapshot::Impl::onClickKeepAspect(LLUICtrl* ctrl, void* data)
{
	LLFloaterSnapshot* view = (LLFloaterSnapshot*)data;
	if (view)
	{
		LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
		keepAspect(view, check->get());
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::onCommitQuality(LLUICtrl* ctrl, void* data)
{
	LLSliderCtrl* slider = (LLSliderCtrl*)ctrl;
	S32 quality_val = llfloor((F32)slider->getValue().asReal());
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->setSnapshotQuality(quality_val);
		checkAutoSnapshot(previewp, TRUE);
	}
}
void LLFloaterSnapshot::Impl::onQualityMouseUp(void* data)
{
	Dout(dc::snapshot, "Calling LLFloaterSnapshot::Impl::QualityMouseUp()");
	LLFloaterSnapshot* view = (LLFloaterSnapshot *)data;
	updateControls(view);
}
void LLFloaterSnapshot::Impl::onCommitFreezeTime(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (!view || !check_box)
	{
		return;
	}
	gSavedSettings.setBOOL("SnapshotOpenFreezeTime", check_box->get());
}
static std::string lastSnapshotWidthName()
{
	switch(gSavedSettings.getS32("LastSnapshotType"))
	{
	case LLSnapshotLivePreview::SNAPSHOT_FEED:     return "LastSnapshotToFeedWidth";
	case LLSnapshotLivePreview::SNAPSHOT_POSTCARD: return "LastSnapshotToEmailWidth";
	case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:  return "LastSnapshotToInventoryWidth";
	default:                                       return "LastSnapshotToDiskWidth";
	}
}
static std::string lastSnapshotHeightName()
{
	switch(gSavedSettings.getS32("LastSnapshotType"))
	{
	case LLSnapshotLivePreview::SNAPSHOT_FEED:     return "LastSnapshotToFeedHeight";
	case LLSnapshotLivePreview::SNAPSHOT_POSTCARD: return "LastSnapshotToEmailHeight";
	case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:  return "LastSnapshotToInventoryHeight";
	default:                                       return "LastSnapshotToDiskHeight";
	}
}
static std::string lastSnapshotAspectName()
{
	switch(gSavedSettings.getS32("LastSnapshotType"))
	{
	case LLSnapshotLivePreview::SNAPSHOT_FEED:     return "LastSnapshotToFeedAspect";
	case LLSnapshotLivePreview::SNAPSHOT_POSTCARD: return "LastSnapshotToEmailAspect";
	case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:  return "LastSnapshotToInventoryAspect";
	default:                                       return "LastSnapshotToDiskAspect";
	}
}
static std::string snapshotKeepAspectName()
{
	switch(gSavedSettings.getS32("LastSnapshotType"))
	{
	case LLSnapshotLivePreview::SNAPSHOT_FEED:     return "SnapshotFeedKeepAspect";
	case LLSnapshotLivePreview::SNAPSHOT_POSTCARD: return "SnapshotPostcardKeepAspect";
	case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:  return "SnapshotTextureKeepAspect";
	default:                                       return "SnapshotLocalKeepAspect";
	}
}
void LLFloaterSnapshot::Impl::keepAspect(LLFloaterSnapshot* view, bool on, bool force)
{
	DoutEntering(dc::snapshot, "LLFloaterSnapshot::Impl::keepAspect(view, " << on << ", " << force << ")");
	bool cur_on = gSavedSettings.getBOOL(snapshotKeepAspectName());
	if ((!force && cur_on == on) ||
		gSavedSettings.getBOOL("RenderUIInSnapshot") ||
		gSavedSettings.getBOOL("RenderHUDInSnapshot"))
	{
		return;
	}
	view->childSetValue("keep_aspect", on);
	gSavedSettings.setBOOL(snapshotKeepAspectName(), on);
	if (on)
	{
		LLSnapshotLivePreview* previewp = getPreviewView();
		if (previewp)
		{
			S32 w = ll_round(view->childGetValue("snapshot_width").asReal(), 1.0);
			S32 h = ll_round(view->childGetValue("snapshot_height").asReal(), 1.0);
			gSavedSettings.setS32(lastSnapshotWidthName(), w);
			gSavedSettings.setS32(lastSnapshotHeightName(), h);
			comboSetCustom(view, previewp->resolutionComboName());
			enforceAspect(view, (F32)w / h);
		}
	}
}
void LLFloaterSnapshot::Impl::updateResolution(LLUICtrl* ctrl, void* data, bool update_controls)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (!view || !combobox)
	{
		return;
	}
	LLSnapshotLivePreview* previewp = getPreviewView();
	view->getChild<LLComboBox>("feed_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotFeedLastResolution"));
	view->getChild<LLComboBox>("postcard_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotPostcardLastResolution"));
	view->getChild<LLComboBox>("texture_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotTextureLastResolution"));
	view->getChild<LLComboBox>("local_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotLocalLastResolution"));
	std::string sdstring = combobox->getSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());
	S32 width = sdres[0];
	S32 height = sdres[1];
	if (width != -1 && height != -1)
	{
		keepAspect(view, false);
	}
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;
		if (width == 0 || height == 0 || gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{
			previewp->setSize(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
		}
		else if (width == -1 || height == -1)
		{
			previewp->setSize(gSavedSettings.getS32(lastSnapshotWidthName()), gSavedSettings.getS32(lastSnapshotHeightName()));
		}
		else if (height == -2)
		{
		  F32 source_aspect = width / 630.f;
		  width = llmin(gViewerWindow->getWindowDisplayWidth(), ll_round(gViewerWindow->getWindowDisplayHeight() * source_aspect));
		  height = llmin(gViewerWindow->getWindowDisplayHeight(), ll_round(gViewerWindow->getWindowDisplayWidth() / source_aspect));
		  previewp->setSize(width, height);
		}
		else
		{
			previewp->setSize(width, height);
		}
		previewp->getSize(width, height);
		if(view->childGetValue("snapshot_width").asInteger() != width || view->childGetValue("snapshot_height").asInteger() != height)
		{
			view->childSetValue("snapshot_width", width);
			view->childSetValue("snapshot_height", height);
		}
	}
	LLSpinCtrl* width_spinner = view->getChild<LLSpinCtrl>("snapshot_width");
	LLSpinCtrl* height_spinner = view->getChild<LLSpinCtrl>("snapshot_height");
	if (	gSavedSettings.getBOOL("RenderUIInSnapshot") ||
		gSavedSettings.getBOOL("RenderHUDInSnapshot"))
	{
		width_spinner->setAllowEdit(FALSE);
		width_spinner->setIncrement(0);
		height_spinner->setAllowEdit(FALSE);
		height_spinner->setIncrement(0);
	}
	else
	{
		width_spinner->setAllowEdit(TRUE);
		width_spinner->setIncrement(32);
		height_spinner->setAllowEdit(TRUE);
		height_spinner->setIncrement(32);
	}
	if (previewp)
	{
		setAspect(view, previewp->aspectComboName(), update_controls);
	}
}
void LLFloaterSnapshot::Impl::updateAspect(LLUICtrl* ctrl, void* data, bool update_controls)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (!view || !combobox)
	{
		return;
	}
	LLSnapshotLivePreview* previewp = getPreviewView();
	view->getChild<LLComboBox>("feed_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotFeedLastAspect"));
	view->getChild<LLComboBox>("postcard_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotPostcardLastAspect"));
	view->getChild<LLComboBox>("texture_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotTextureLastAspect"));
	view->getChild<LLComboBox>("local_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotLocalLastAspect"));
	std::string sdstring = combobox->getSelectedValue();
	std::stringstream sstream;
	sstream << sdstring;
	F32 aspect;
	sstream >> aspect;
	if (aspect == -2)
	{
		S32 width, height;
		previewp->getSize(width, height);
		aspect = (F32)width / height;
		keepAspect(view, false);
	}
	else if (aspect == -1)
	{
		aspect = gSavedSettings.getF32(lastSnapshotAspectName());
	}
	if (aspect == 0)
	{
		aspect = (F32)gViewerWindow->getWindowDisplayWidth() / gViewerWindow->getWindowDisplayHeight();
	}
	LLSpinCtrl* aspect_spinner = view->getChild<LLSpinCtrl>("aspect_ratio");
	LLCheckBoxCtrl* keep_aspect = view->getChild<LLCheckBoxCtrl>("keep_aspect");
	if (gSavedSettings.getBOOL("RenderUIInSnapshot") ||
		gSavedSettings.getBOOL("RenderHUDInSnapshot"))
	{
		aspect = (F32)gViewerWindow->getWindowDisplayWidth() / gViewerWindow->getWindowDisplayHeight();
		aspect_spinner->setAllowEdit(FALSE);
		aspect_spinner->setIncrement(0);
		keep_aspect->setEnabled(FALSE);
	}
	else
	{
		aspect_spinner->setAllowEdit(TRUE);
		aspect_spinner->setIncrement(llmax(0.01f, lltrunc(aspect) / 100.0f));
		keep_aspect->setEnabled(TRUE);
	}
	aspect_spinner->set(aspect);
	if (previewp)
	{
		F32 old_aspect = previewp->getAspect();
		previewp->setAspect(aspect);
		if (old_aspect != aspect)
		{
			checkAutoSnapshot(previewp, TRUE);
		}
	}
	if (update_controls)
	{
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::enforceAspect(LLFloaterSnapshot* floater, F32 new_aspect)
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		LLComboBox* combo = floater->getChild<LLComboBox>(previewp->aspectComboName());
		S32 const aspect_custom = combo->getItemCount() - 1;
		for (S32 index = 0; index <= aspect_custom; ++index)
		{
			combo->setCurrentByIndex(index);
			if (index == aspect_custom)
			{
				gSavedSettings.setF32(lastSnapshotAspectName(), new_aspect);
				break;
			}
			std::string sdstring = combo->getSelectedValue();
			std::stringstream sstream;
			sstream << sdstring;
			F32 aspect;
			sstream >> aspect;
			if (aspect == -2)
			{
				continue;
			}
			if (aspect == 0)
			{
				aspect = (F32)gViewerWindow->getWindowDisplayWidth() / gViewerWindow->getWindowDisplayHeight();
			}
			if (llabs(aspect - new_aspect) < 0.0001)
			{
				break;
			}
		}
		storeAspectSetting(combo, previewp->aspectComboName());
		updateAspect(combo, floater, true);
	}
}
void LLFloaterSnapshot::Impl::enforceResolution(LLFloaterSnapshot* floater, F32 new_aspect)
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		S32 w, h;
		previewp->getSize(w, h);
		F32 cw = w;
		F32 ch = h;
		previewp->getRawSize(w, h);
		F32 rw = w;
		F32 rh = h;
		F32 nw = llmin(llmax(cw, ch * new_aspect), rw);
		F32 nh = llmin(llmax(ch, cw / new_aspect), rh);
		nw = llmin(nw, nh * new_aspect);
		nh = llmin(nh, nw / new_aspect);
		S32 new_width = ll_round(nw);
		S32 new_height = ll_round(nh);
		gSavedSettings.setS32(lastSnapshotWidthName(), new_width);
		gSavedSettings.setS32(lastSnapshotHeightName(), new_height);
		comboSetCustom(floater, previewp->resolutionComboName());
		updateResolution(floater->getChild<LLComboBox>(previewp->resolutionComboName()), floater, false);
	}
}
void LLFloaterSnapshot::Impl::onCommitLayerTypes(LLUICtrl* ctrl, void*data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		LLComboBox* combobox = (LLComboBox*)ctrl;
		gSavedSettings.setS32("SnapshotLayerType", combobox->getCurrentIndex());
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::onCommitSnapshotType(LLUICtrl* ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		LLSnapshotLivePreview::ESnapshotType snapshot_type = getTypeIndex(view);
		gSavedSettings.setS32("LastSnapshotType", snapshot_type);
		LLSnapshotLivePreview* previewp = getPreviewView();
		if (previewp)
		{
			previewp->setSnapshotType(snapshot_type);
		}
		keepAspect(view, gSavedSettings.getBOOL(snapshotKeepAspectName()), true);
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::onCommitSnapshotFormat(LLUICtrl* ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		gSavedSettings.setS32("SnapshotFormat", getFormatIndex(view));
		updateControls(view);
	}
}
void LLFloaterSnapshot::Impl::comboSetCustom(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
	combo->setCurrentByIndex(combo->getItemCount() - 1);
	storeAspectSetting(combo, comboname);
}
void LLFloaterSnapshot::Impl::storeAspectSetting(LLComboBox* combo, const std::string& comboname)
{
	if(comboname == "feed_size_combo")
	{
		gSavedSettings.setS32("SnapshotFeedLastResolution", combo->getCurrentIndex());
	}
	else if(comboname == "postcard_size_combo")
	{
		gSavedSettings.setS32("SnapshotPostcardLastResolution", combo->getCurrentIndex());
	}
	else if(comboname == "local_size_combo")
	{
		gSavedSettings.setS32("SnapshotLocalLastResolution", combo->getCurrentIndex());
	}
	else if(comboname == "feed_aspect_combo")
	{
		gSavedSettings.setS32("SnapshotFeedLastAspect", combo->getCurrentIndex());
	}
	else if(comboname == "postcard_aspect_combo")
	{
		gSavedSettings.setS32("SnapshotPostcardLastAspect", combo->getCurrentIndex());
	}
	else if(comboname == "texture_aspect_combo")
	{
		gSavedSettings.setS32("SnapshotTextureLastAspect", combo->getCurrentIndex());
	}
	else if(comboname == "local_aspect_combo")
	{
		gSavedSettings.setS32("SnapshotLocalLastAspect", combo->getCurrentIndex());
	}
}
void LLFloaterSnapshot::Impl::onCommitCustomResolution(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		LLSpinCtrl* width_spinner = view->getChild<LLSpinCtrl>("snapshot_width");
		LLSpinCtrl* height_spinner = view->getChild<LLSpinCtrl>("snapshot_height");
		S32 w = ll_round((F32)width_spinner->getValue().asReal(), 1.0f);
		S32 h = ll_round((F32)height_spinner->getValue().asReal(), 1.0f);
		LLSnapshotLivePreview* previewp = getPreviewView();
		if (previewp)
		{
			S32 curw,curh;
			previewp->getSize(curw, curh);
			if (w != curw || h != curh)
			{
				Dout(dc::snapshot, "w = " << w << "; curw = " << curw);
				if (llabs(w - curw) == 32)
				{
					w = (w + 16) & -32;
					Dout(dc::snapshot, "w = (w + 16) & -32 = " << w);
				}
				if (llabs(h - curh) == 32)
				{
					h = (h + 16) & -32;
				}
				if (gSavedSettings.getBOOL(snapshotKeepAspectName()))
				{
					F32 aspect = previewp->getAspect();
					if (h == curh)
					{
						h = ll_round(w / aspect);
					}
					else
					{
						w = ll_round(h * aspect);
					}
				}
				width_spinner->forceSetValue(LLSD::Real(w));
				height_spinner->forceSetValue(LLSD::Real(h));
				previewp->setMaxImageSize((S32)((LLSpinCtrl *)ctrl)->getMaxValue()) ;
				previewp->setSize(w,h);
				checkAutoSnapshot(previewp, FALSE);
				previewp->updateSnapshot(FALSE, TRUE);
				comboSetCustom(view, "feed_size_combo");
				comboSetCustom(view, "postcard_size_combo");
				comboSetCustom(view, "local_size_combo");
			}
		}
		gSavedSettings.setS32(lastSnapshotWidthName(), w);
		gSavedSettings.setS32(lastSnapshotHeightName(), h);
		updateControls(view, true);
	}
}
char const* LLSnapshotLivePreview::resolutionComboName() const
{
	char const* result;
	switch(mSnapshotType)
	{
		case SNAPSHOT_FEED:
			result = "feed_size_combo";
			break;
		case SNAPSHOT_POSTCARD:
			result = "postcard_size_combo";
			break;
		case SNAPSHOT_TEXTURE:
			result = "texture_size_combo";
			break;
		case SNAPSHOT_LOCAL:
			result = "local_size_combo";
			break;
		default:
			result= "";
			break;
	}
	return result;
}
char const* LLSnapshotLivePreview::aspectComboName() const
{
	char const* result;
	switch(mSnapshotType)
	{
		case SNAPSHOT_FEED:
			result = "feed_aspect_combo";
			break;
		case SNAPSHOT_POSTCARD:
			result = "postcard_aspect_combo";
			break;
		case SNAPSHOT_TEXTURE:
			result = "texture_aspect_combo";
			break;
		case SNAPSHOT_LOCAL:
			result = "local_aspect_combo";
			break;
		default:
			result= "";
			break;
	}
	return result;
}
void LLFloaterSnapshot::Impl::onCommitCustomAspect(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		F32 a = view->childGetValue("aspect_ratio").asReal();
		LLSnapshotLivePreview* previewp = getPreviewView();
		if (previewp)
		{
			F32 cura = previewp->getAspect();
			if (a != cura)
			{
				previewp->setAspect(a);
				previewp->updateSnapshot(FALSE, TRUE);
				comboSetCustom(view, previewp->aspectComboName());
			}
		}
		gSavedSettings.setF32(lastSnapshotAspectName(), a);
		if (gSavedSettings.getBOOL(snapshotKeepAspectName()))
		{
			enforceResolution(view, a);
		}
		updateControls(view, true);
	}
}
LLFloaterSnapshot::LLFloaterSnapshot()
	: LLFloater(std::string("Snapshot Floater")),
	  impl (*(new Impl))
{
}
LLFloaterSnapshot::~LLFloaterSnapshot()
{
	if (sInstance == this)
	{
		LLView::deleteViewByHandle(Impl::sPreviewHandle);
		Impl::sPreviewHandle = LLHandle<LLView>();
		sInstance = NULL;
	}
	gSavedSettings.setBOOL("FreezeTime", FALSE);
	if (impl.mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl.mLastToolset);
	}
	delete &impl;
}
BOOL LLFloaterSnapshot::postBuild()
{
	sUIWinHeightLong = getRect().getHeight();
	sUIWinHeightShort = sUIWinHeightLong - 266;
	if (LLUICtrl* snapshot_type_radio = getChild<LLUICtrl>("snapshot_type_radio"))
	{
		snapshot_type_radio->setCommitCallback(Impl::onCommitSnapshotType, this);
		snapshot_type_radio->setValue(gSavedSettings.getS32("LastSnapshotType"));
	}
	childSetCommitCallback("local_format_combo", Impl::onCommitSnapshotFormat, this);
	childSetAction("new_snapshot_btn", Impl::onClickNewSnapshot, this);
	childSetAction("freeze_time_btn", Impl::onClickFreezeTime, this);
	childSetAction("more_btn", Impl::onClickMore, this);
	childSetAction("less_btn", Impl::onClickLess, this);
	childSetAction("upload_btn", Impl::onClickKeep, this);
	childSetAction("send_btn", Impl::onClickKeep, this);
	childSetAction("feed_btn", Impl::onClickKeep, this);
	childSetCommitCallback("save_btn", Impl::onCommitSave, this);
	childSetAction("discard_btn", Impl::onClickDiscard, this);
	childSetCommitCallback("image_quality_slider", Impl::onCommitQuality, this);
	childSetValue("image_quality_slider", gSavedSettings.getS32("SnapshotQuality"));
	impl.mQualityMouseUpConnection = getChild<LLSliderCtrl>("image_quality_slider")->setSliderMouseUpCallback(boost::bind(&Impl::onQualityMouseUp, this));
	childSetCommitCallback("snapshot_width", Impl::onCommitCustomResolution, this);
	childSetCommitCallback("snapshot_height", Impl::onCommitCustomResolution, this);
	childSetCommitCallback("aspect_ratio", Impl::onCommitCustomAspect, this);
	childSetCommitCallback("keep_aspect", Impl::onClickKeepAspect, this);
	childSetCommitCallback("ui_check", Impl::onClickUICheck, this);
	getChild<LLUICtrl>("ui_check")->setValue(gSavedSettings.getBOOL("RenderUIInSnapshot"));
	childSetCommitCallback("hud_check", Impl::onClickHUDCheck, this);
	getChild<LLUICtrl>("hud_check")->setValue(gSavedSettings.getBOOL("RenderHUDInSnapshot"));
	childSetCommitCallback("keep_open_check", Impl::onClickKeepOpenCheck, this);
	childSetValue("keep_open_check", !gSavedSettings.getBOOL("CloseSnapshotOnKeep"));
	childSetCommitCallback("layer_types", Impl::onCommitLayerTypes, this);
	getChild<LLUICtrl>("layer_types")->setValue("colors");
	getChildView("layer_types")->setEnabled(FALSE);
	childSetValue("snapshot_width", gSavedSettings.getS32(lastSnapshotWidthName()));
	childSetValue("snapshot_height", gSavedSettings.getS32(lastSnapshotHeightName()));
	getChild<LLUICtrl>("freeze_time_check")->setValue(gSavedSettings.getBOOL("SnapshotOpenFreezeTime"));
	childSetCommitCallback("freeze_time_check", Impl::onCommitFreezeTime, this);
	getChild<LLUICtrl>("auto_snapshot_check")->setValue(gSavedSettings.getBOOL("AutoSnapshot"));
	childSetCommitCallback("auto_snapshot_check", Impl::onClickAutoSnap, this);
	childSetCommitCallback("temp_check", Impl::onClickTemporaryImage, this);
	childSetCommitCallback("feed_size_combo", Impl::onCommitFeedResolution, this);
	childSetCommitCallback("postcard_size_combo", Impl::onCommitPostcardResolution, this);
	childSetCommitCallback("texture_size_combo", Impl::onCommitTextureResolution, this);
	childSetCommitCallback("local_size_combo", Impl::onCommitLocalResolution, this);
	childSetCommitCallback("feed_aspect_combo", Impl::onCommitFeedAspect, this);
	childSetCommitCallback("postcard_aspect_combo", Impl::onCommitPostcardAspect, this);
	childSetCommitCallback("texture_aspect_combo", Impl::onCommitTextureAspect, this);
	childSetCommitCallback("local_aspect_combo", Impl::onCommitLocalAspect, this);
	LLRect full_screen_rect = sInstance->getRootView()->getRect();
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(full_screen_rect);
	sInstance->getRootView()->addChild(previewp);
	sInstance->getRootView()->addChild(gSnapshotFloaterView);
	gSavedSettings.setBOOL("TemporaryUpload",FALSE);
	childSetValue("temp_check",FALSE);
	Impl::sPreviewHandle = previewp->getHandle();
	impl.keepAspect(sInstance, gSavedSettings.getBOOL(snapshotKeepAspectName()), true);
	impl.freezeTime(gSavedSettings.getBOOL("SnapshotOpenFreezeTime"));
	impl.updateControls(this);
	return TRUE;
}
LLRect LLFloaterSnapshot::getThumbnailAreaRect()
{
	return LLRect(1, getRect().getHeight() - 17, getRect().getWidth() - 1, getRect().getHeight() - 17 - THUMBHEIGHT);
}
void LLFloaterSnapshot::draw()
{
	LLSnapshotLivePreview* previewp = impl.getPreviewView();
	if (previewp && (previewp->isSnapshotActive() || previewp->getThumbnailLock()))
	{
		return;
	}
	LLFloater::draw();
	if (previewp)
	{
		if(previewp->getThumbnailImage())
		{
			LLRect const thumb_area = getThumbnailAreaRect();
			S32 offset_x = (thumb_area.mLeft + thumb_area.mRight - previewp->getThumbnailWidth()) / 2;
			S32 offset_y = (thumb_area.mBottom + thumb_area.mTop - previewp->getThumbnailHeight()) / 2;
			gGL.matrixMode(LLRender::MM_MODELVIEW);
			gl_rect_2d(thumb_area, LLColor4::transparent, true);
			gl_draw_scaled_image(offset_x, offset_y,
					previewp->getThumbnailWidth(), previewp->getThumbnailHeight(),
					previewp->getThumbnailImage(), LLColor4::white);
			previewp->drawPreviewRect(offset_x, offset_y) ;
		}
	}
}
void LLFloaterSnapshot::onOpen()
{
	gSavedSettings.setBOOL("SnapshotBtnState", TRUE);
}
void LLFloaterSnapshot::onClose(bool app_quitting)
{
	gSnapshotFloaterView->setVisible(FALSE);
	gSnapshotFloaterView->setEnabled(FALSE);
	gSavedSettings.setBOOL("SnapshotBtnState", FALSE);
	impl.freezeTime(false);
	destroy();
}
void LLFloaterSnapshot::show(void*)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterSnapshot();
		LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_snapshot.xml", NULL, FALSE);
		gSnapshotFloaterView->addChild(sInstance);
		sInstance->impl.updateLayout(sInstance);
	}
	else
	{
		LLSnapshotLivePreview* preview = LLFloaterSnapshot::Impl::getPreviewView();
		if(preview)
		{
			preview->updateSnapshot(TRUE);
		}
	}
	sInstance->open();
	sInstance->focusFirstItem(FALSE);
	sInstance->setEnabled(TRUE);
	sInstance->setVisible(TRUE);
	gSnapshotFloaterView->setEnabled(TRUE);
	gSnapshotFloaterView->setVisible(TRUE);
	gSnapshotFloaterView->adjustToFitScreen(sInstance, FALSE);
}
void LLFloaterSnapshot::hide(void*)
{
	if (sInstance && !sInstance->isDead())
	{
		sInstance->close();
	}
}
void LLFloaterSnapshot::update()
{
	BOOL changed = FALSE;
	for (std::set<LLSnapshotLivePreview*>::iterator iter = LLSnapshotLivePreview::sList.begin();
		 iter != LLSnapshotLivePreview::sList.end(); ++iter)
	{
		changed |= LLSnapshotLivePreview::onIdle(*iter);
	}
	if(changed)
	{
		sInstance->impl.updateControls(sInstance);
	}
}
void LLFloaterSnapshot::updateControls()
{
	Impl::updateControls(sInstance);
}
void LLFloaterSnapshot::resetFeedAndPostcardAspect()
{
	Impl::resetFeedAndPostcardAspect(sInstance);
}
BOOL LLFloaterSnapshot::handleKeyHere(KEY key, MASK mask)
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if (freeze_time && key == KEY_ESCAPE && mask == MASK_NONE)
	{
		LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView();
		if (previewp && previewp->getShowFreezeFrameSnapshot())
		{
			previewp->showFreezeFrameSnapshot(false);
		}
		else
		{
			impl.freezeTime(false);
		}
		return TRUE;
	}
	else if (key == 'Q' && mask == MASK_CONTROL)
	{
		LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView();
		if (previewp && previewp->getShowFreezeFrameSnapshot())
		{
			previewp->showFreezeFrameSnapshot(false);
		}
		impl.freezeTime(false);
		gFocusMgr.removeKeyboardFocusWithoutCallback(gFocusMgr.getKeyboardFocus());
		return FALSE;
	}
	return LLFloater::handleKeyHere(key, mask);
}
BOOL LLFloaterSnapshot::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mask == MASK_NONE)
	{
		LLRect thumb_area = getThumbnailAreaRect();
		if (thumb_area.pointInRect(x, y))
		{
			return TRUE;
		}
	}
	return LLFloater::handleMouseDown(x, y, mask);
}
BOOL LLFloaterSnapshot::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (mask == MASK_NONE)
	{
		LLRect thumb_area = getThumbnailAreaRect();
		if (thumb_area.pointInRect(x, y))
		{
			impl.updateControls(this);
			return TRUE;
		}
	}
	return LLFloater::handleMouseUp(x, y, mask);
}
LLSnapshotFloaterView::LLSnapshotFloaterView( const std::string& name, const LLRect& rect ) : LLFloaterView(name, rect)
{
	setMouseOpaque(FALSE);
	setEnabled(FALSE);
}
LLSnapshotFloaterView::~LLSnapshotFloaterView()
{
}
BOOL LLSnapshotFloaterView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleKey(key, mask, called_from_parent);
	}
	LLFloaterView::handleKey(key, mask, TRUE);
	return TRUE;
}
BOOL LLSnapshotFloaterView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleMouseDown(x, y, mask);
	}
	if (childrenHandleMouseDown(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseDown( x, y, mask );
	}
	return TRUE;
}
BOOL LLSnapshotFloaterView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleMouseUp(x, y, mask);
	}
	if (childrenHandleMouseUp(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseUp( x, y, mask );
	}
	return TRUE;
}
BOOL LLSnapshotFloaterView::handleHover(S32 x, S32 y, MASK mask)
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleHover(x, y, mask);
	}
	if (childrenHandleHover(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleHover( x, y, mask );
	}
	return TRUE;
}
