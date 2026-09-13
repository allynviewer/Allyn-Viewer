/** 
 * @file llpreviewtexture.cpp
 * @brief LLPreviewTexture class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llpreviewtexture.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "statemachine/aifilepicker.h"
#include "llfloaterinventory.h"
#include "llimage.h"
#include "llinventory.h"
#include "llnotificationsutil.h"
#include "llresmgr.h"
#include "lltrans.h"
#include "lltextbox.h"
#include "lltextureview.h"
#include "llviewertexturelist.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "lllineeditor.h"
#include "lllocalcliprect.h"
#include "llfloater.h"
const S32 PREVIEW_TEXTURE_MIN_WIDTH = 360;
const S32 PREVIEW_TEXTURE_MIN_HEIGHT = 280;
const S32 CLIENT_RECT_VPAD = 4;
const F32 SECONDS_TO_SHOW_FILE_SAVED_MSG = 8.f;
const F32 PREVIEW_TEXTURE_MAX_ASPECT = 200.f;
const F32 PREVIEW_TEXTURE_MIN_ASPECT = 0.005f;
LLPreviewTexture * LLPreviewTexture::sInstance;
LLPreviewTexture::LLPreviewTexture(const std::string& name,
								   const LLRect& rect,
								   const std::string& title,
								   const LLUUID& item_uuid,
								   const LLUUID& object_id,
								   BOOL show_keep_discard)
:	LLPreview(name, rect, title, item_uuid, object_id, TRUE, PREVIEW_TEXTURE_MIN_WIDTH, PREVIEW_TEXTURE_MIN_HEIGHT ),
	mLoadingFullImage( FALSE ),
	mShowKeepDiscard(show_keep_discard),
	mCopyToInv(FALSE),
	mIsCopyable(FALSE),
	mUpdateDimensions(TRUE),
	mLastHeight(0),
	mLastWidth(0),
	mAspectRatio(0.f),
	mImage(NULL),
	mImageOldBoostLevel(LLGLTexture::BOOST_NONE),
	mMaxAutoSize(0)
{
	const LLInventoryItem *item = getItem();
	if(item)
	{
		mImageID = item->getAssetUUID();
		const LLPermissions& perm = item->getPermissions();
		U32 mask = PERM_NONE;
		if(perm.getOwner() == gAgent.getID())
		{
			mask = perm.getMaskBase();
		}
		else if(gAgent.isInGroup(perm.getGroup()))
		{
			mask = perm.getMaskGroup();
		}
		else
		{
			mask = perm.getMaskEveryone();
		}
		if((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		{
			mIsCopyable = TRUE;
		}
	}
	init();
	setTitle(title);
	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}
}
LLPreviewTexture::LLPreviewTexture(
	const std::string& name,
	const LLRect& rect,
	const std::string& title,
	const LLUUID& asset_id,
	BOOL copy_to_inv,
	S32 max_auto_size)
	:
	LLPreview(
		name,
		rect,
		title,
		asset_id,
		LLUUID::null,
		TRUE,
		PREVIEW_TEXTURE_MIN_WIDTH,
		PREVIEW_TEXTURE_MIN_HEIGHT ),
	mImageID(asset_id),
	mLoadingFullImage( FALSE ),
	mShowKeepDiscard(FALSE),
	mCopyToInv(copy_to_inv),
	mIsCopyable(FALSE),
	mLastHeight(0),
	mLastWidth(0),
	mAspectRatio(0.f),
	mAlphaMaskResult(0),
	mMaxAutoSize(max_auto_size)
{
	init();
	setTitle(title);
	LLRect curRect = getRect();
	translate(curRect.mLeft - rect.mLeft, curRect.mTop - rect.mTop);
}
LLPreviewTexture::~LLPreviewTexture()
{
	LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;
	if( mLoadingFullImage )
	{
		getWindow()->decBusyCount();
	}
	if(mImage)
	{
		mImage->setBoostLevel(mImageOldBoostLevel);
		mImage = NULL;
	}
	sInstance = NULL;
}
void LLPreviewTexture::init()
{
	sInstance = this;
	LLUICtrlFactory::getInstance()->buildFloater(sInstance,"floater_preview_texture.xml");
	childSetVisible("desc", !mCopyToInv);
	childSetVisible("desc txt", !mCopyToInv);
	childSetVisible("Copy To Inventory", mCopyToInv);
	childSetVisible("Keep", mShowKeepDiscard);
	childSetVisible("Discard", mShowKeepDiscard);
	childSetAction("openprofile", onClickProfile, this);
	if (mCopyToInv)
	{
		childSetAction("Copy To Inventory",LLPreview::onBtnCopyToInv,this);
	}
	else if (mShowKeepDiscard)
	{
		childSetAction("Keep",onKeepBtn,this);
		childSetAction("Discard",onDiscardBtn,this);
	}
	if (!mCopyToInv)
	{
		const LLInventoryItem* item = getItem();
		if (item)
		{
			mCreatorKey = item->getCreatorUUID();
			childSetCommitCallback("desc", LLPreview::onText, this);
			childSetText("desc", item->getDescription());
			getChild<LLLineEditor>("desc")->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
			childSetText("uuid", getItemID().asString());
			childSetText("alphanote", LLTrans::getString("LoadingData"));
		}
	}
	childSetText("uploader", getItemCreatorName());
	childSetText("uploadtime", getItemCreationDate());
	childSetCommitCallback("combo_aspect_ratio", onAspectRatioCommit, this);
	LLComboBox* combo = getChild<LLComboBox>("combo_aspect_ratio");
	combo->setCurrentByIndex(0);
}
void LLPreviewTexture::draw()
{
	if (mUpdateDimensions)
	{
		updateDimensions();
	}
	LLPreview::draw();
	if (!isMinimized())
	{
		LLGLSUIDefault gls_ui;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		const LLRect& border = mClientRect;
		LLRect interior = mClientRect;
		interior.stretch( -PREVIEW_BORDER_WIDTH );
		LLLocalClipRect image_clip(mClientRect);
		gl_rect_2d( border, LLColor4(0.f, 0.f, 0.f, 1.f));
		gl_rect_2d_checkerboard( calcScreenRect(), interior );
		if ( mImage.notNull() )
		{
			gGL.diffuseColor3f( 1.f, 1.f, 1.f );
			gl_draw_scaled_image(interior.mLeft,
								interior.mBottom,
								interior.getWidth(),
								interior.getHeight(),
								mImage);
			static const LLCachedControl<bool> use_rmse_auto_mask("SHUseRMSEAutoMask",false);
			static const LLCachedControl<F32> auto_mask_max_rmse("SHAutoMaskMaxRMSE",.09f);
			static const LLCachedControl<F32> auto_mask_max_mid("SHAutoMaskMaxMid", .25f);
			if (mAlphaMaskResult != mImage->getIsAlphaMask(use_rmse_auto_mask ? auto_mask_max_rmse : -1.f, auto_mask_max_mid))
			{
				mAlphaMaskResult = !mAlphaMaskResult;
				if (!mAlphaMaskResult)
				{
					childSetColor("alphanote", LLColor4::green);
					childSetText("alphanote", getString("No Alpha"));
				}
				else
				{
					childSetColor("alphanote", LLColor4::red);
					childSetText("alphanote", getString("Has Alpha"));
				}
			}
			F32 pixel_area = mLoadingFullImage ? (F32)MAX_IMAGE_AREA  : (F32)(interior.getWidth() * interior.getHeight() );
			mImage->addTextureStats( pixel_area );
			if(pixel_area > 0.f)
			{
				mImage->setAdditionalDecodePriority(1.0f) ;
			}
			if (!mLoadingFullImage)
			{
				S32 int_width = interior.getWidth();
				S32 int_height = interior.getHeight();
				mImage->setKnownDrawSize(int_width, int_height);
			}
			else
			{
				mImage->setKnownDrawSize(0, 0);
			}
			if( mLoadingFullImage )
			{
				LLFontGL::getFontSansSerif()->renderUTF8(LLTrans::getString("Receiving"), 0,
					interior.mLeft + 4,
					interior.mBottom + 4,
					LLColor4::white, LLFontGL::LEFT, LLFontGL::BOTTOM,
					LLFontGL::NORMAL,
					LLFontGL::DROP_SHADOW);
				F32 data_progress = mImage->getDownloadProgress();
				const S32 BAR_HEIGHT = 12;
				const S32 BAR_LEFT_PAD = 80;
				S32 left = interior.mLeft + 4 + BAR_LEFT_PAD;
				S32 bar_width = getRect().getWidth() - left - RESIZE_HANDLE_WIDTH - 2;
				S32 top = interior.mBottom + 4 + BAR_HEIGHT;
				S32 right = left + bar_width;
				S32 bottom = top - BAR_HEIGHT;
				LLColor4 background_color(0.f, 0.f, 0.f, 0.75f);
				LLColor4 decoded_color(0.f, 1.f, 0.f, 1.0f);
				LLColor4 downloaded_color(0.f, 0.5f, 0.f, 1.0f);
				gl_rect_2d(left, top, right, bottom, background_color);
				if (data_progress > 0.0f)
				{
					right = left + llfloor(data_progress * (F32)bar_width);
					if (right > left)
					{
						gl_rect_2d(left, top, right, bottom, downloaded_color);
					}
				}
			}
			else if(!mSavedFileTimer.hasExpired())
			{
				LLFontGL::getFontSansSerif()->renderUTF8(LLTrans::getString("FileSaved"), 0,
					interior.mLeft + 4,
					interior.mBottom + 4,
					LLColor4::white, LLFontGL::LEFT, LLFontGL::BOTTOM,
					LLFontGL::NORMAL,
					LLFontGL::DROP_SHADOW);
			}
		}
	}
}
BOOL LLPreviewTexture::canSaveAs() const
{
	return mIsCopyable && !mLoadingFullImage && mImage.notNull() && !mImage->isMissingAsset();
}
void LLPreviewTexture::saveAs()
{
	if( mLoadingFullImage )
		return;
	const LLViewerInventoryItem* item = getItem() ;
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open(item ? LLDir::getScrubbedFileName(item->getName()) + ".png" : LLStringUtil::null, FFSAVE_IMAGE, "", "image");
	filepicker->run(boost::bind(&LLPreviewTexture::saveAs_continued, this, item, filepicker));
}
void LLPreviewTexture::saveAs_continued(LLViewerInventoryItem const* item, AIFilePicker* filepicker)
{
	if (!filepicker->hasFilename())
		return;
	mSaveFileName = filepicker->getFilename();
	mLoadingFullImage = TRUE;
	getWindow()->incBusyCount();
	mImage->forceToSaveRawImage(0) ;
	mImage->setLoadedCallback( LLPreviewTexture::onFileLoadedForSave,
								0, TRUE, FALSE, new LLUUID( mItemUUID ), &mCallbackTextureList );
}
void LLPreviewTexture::onFileLoadedForSave(BOOL success,
											LLViewerFetchedTexture *src_vi,
											LLImageRaw* src,
											LLImageRaw* aux_src,
											S32 discard_level,
											BOOL final,
											void* userdata)
{
	LLUUID* item_uuid = (LLUUID*) userdata;
	LLPreviewTexture* self = NULL;
	preview_map_t::iterator found_it = LLPreview::sInstances.find(*item_uuid);
	if(found_it != LLPreview::sInstances.end())
	{
		self = (LLPreviewTexture*) found_it->second;
	}
	if( final || !success )
	{
		delete item_uuid;
		if( self )
		{
			self->getWindow()->decBusyCount();
			self->mLoadingFullImage = FALSE;
		}
	}
	if( self && final && success )
	{
		LLPointer<LLImageFormatted> image = LLImageFormatted::createFromExtension(self->mSaveFileName);
		if (!image || !image->encode(src, 0.0))
		{
			LLSD args;
			args["FILE"] = self->mSaveFileName;
			LLNotificationsUtil::add("CannotEncodeFile", args);
		}
		else if (!image->save(self->mSaveFileName))
		{
			LLSD args;
			args["FILE"] = self->mSaveFileName;
			LLNotificationsUtil::add("CannotWriteFile", args);
		}
		else
		{
			self->mSavedFileTimer.reset(SECONDS_TO_SHOW_FILE_SAVED_MSG);
		}
		self->mSaveFileName.clear();
	}
	if( self && !success )
	{
		LLNotificationsUtil::add("CannotDownloadFile");
	}
}
LLUUID LLPreviewTexture::getItemID()
{
	const LLViewerInventoryItem* item = getItem();
	if(item)
	{
		U32 perms = item->getPermissions().getMaskOwner();
		if ((perms & PERM_TRANSFER) &&
			(perms & PERM_COPY))
		{
			return item->getAssetUUID();
		}
	}
	return LLUUID::null;
}
std::string LLPreviewTexture::getItemCreationDate()
{
	const LLViewerInventoryItem* item = getItem();
	if(item)
	{
		std::string time;
		timeToFormattedString(item->getCreationDate(), gSavedSettings.getString("TimestampFormat"), time);
		return time;
	}
	const LLDate date = mImage->getUploadTime();
	return date.notNull() ? date.toHTTPDateString(gSavedSettings.getString("TimestampFormat"))
		: getString("Unknown");
}
std::string LLPreviewTexture::getItemCreatorName()
{
	const LLViewerInventoryItem* item = getItem();
	const LLUUID& id = item ? item->getCreatorUUID() : mImage->getUploader();
	if (id.notNull())
	{
		std::string name;
		LLAvatarNameCache::getNSName(id, name);
		mCreatorKey = id;
		return name;
	}
	return getString("Unknown");
}
LLRect LLPreviewTexture::getImageSlot() const
{
	const S32 pad = CLIENT_RECT_VPAD;
	S32 header_bottom = getRect().getHeight() - LLFLOATER_HEADER_SIZE;
	static const char* const header_names[] = {
		"desc txt", "desc", "uuid txt", "uuid",
		"uploader txt", "uploader", "openprofile",
		"uploadtime txt", "uploadtime", "alphanote"
	};
	for (const char* name : header_names)
	{
		const LLView* v = getChildView(name, TRUE, FALSE);
		if (v && v->getVisible())
		{
			header_bottom = llmin(header_bottom, v->getRect().mBottom);
		}
	}
	S32 footer_top = LLFLOATER_CORNER_RADIUS;
	static const char* const footer_names[] = {
		"dimensions", "aspect_ratio", "combo_aspect_ratio",
		"Keep", "Discard", "Copy To Inventory"
	};
	for (const char* name : footer_names)
	{
		const LLView* v = getChildView(name, TRUE, FALSE);
		if (v && v->getVisible())
		{
			footer_top = llmax(footer_top, v->getRect().mTop);
		}
	}
	LLRect slot(PREVIEW_PAD, header_bottom - pad, getRect().getWidth() - PREVIEW_PAD, footer_top + pad);
	if (slot.mTop < slot.mBottom + 8)
	{
		slot.mTop = slot.mBottom + 8;
	}
	if (slot.mRight < slot.mLeft + 8)
	{
		slot.mRight = slot.mLeft + 8;
	}
	return slot;
}
void LLPreviewTexture::updateDimensions()
{
	if (!mImage) return;
	S32 image_height = llmax(1, mImage->getHeight(0));
	S32 image_width = llmax(1, mImage->getWidth(0));
	S32 client_width = image_width;
	S32 client_height = image_height;
	S32 horiz_pad = 2 * (LLPANEL_BORDER_WIDTH + PREVIEW_PAD) + PREVIEW_RESIZE_HANDLE_SIZE;
	S32 max_client_width = gViewerWindow->getWindowWidth() - horiz_pad;
	S32 max_client_height = gViewerWindow->getWindowHeight() - LLFLOATER_HEADER_SIZE - 2 * LLFLOATER_CORNER_RADIUS;
	if (mAspectRatio > 0.f) client_height = llceil((F32)client_width / mAspectRatio);
	if (mMaxAutoSize > 0 && (client_width > mMaxAutoSize || client_height > mMaxAutoSize))
	{
		if (client_width >= client_height)
		{
			client_height = llmax(1, client_height * mMaxAutoSize / client_width);
			client_width = mMaxAutoSize;
		}
		else
		{
			client_width = llmax(1, client_width * mMaxAutoSize / client_height);
			client_height = mMaxAutoSize;
		}
	}
	while ((client_width > max_client_width) ||
	       (client_height > max_client_height ))
	{
		client_width /= 2;
		client_height /= 2;
	}
	childSetTextArg("dimensions", "[WIDTH]", llformat("%d", mImage->getFullWidth()));
	childSetTextArg("dimensions", "[HEIGHT]", llformat("%d", mImage->getFullHeight()));
	const LLRect slot_before = getImageSlot();
	const S32 chrome_w = getRect().getWidth() - slot_before.getWidth();
	const S32 chrome_h = getRect().getHeight() - slot_before.getHeight();
	S32 view_width = llmax(client_width + chrome_w, getMinWidth());
	S32 view_height = llmax(client_height + chrome_h, getMinHeight());
	if (client_height != mLastHeight || client_width != mLastWidth)
	{
		mLastWidth = client_width;
		mLastHeight = client_height;
		S32 old_top = getRect().mTop;
		S32 old_left = getRect().mLeft;
		if (getHost())
		{
			getHost()->growToFit(view_width, view_height);
		}
		else
		{
			reshape( view_width, view_height );
			S32 new_bottom = old_top - getRect().getHeight();
			setOrigin( old_left, new_bottom );
			gFloaterView->adjustToFitScreen(this, FALSE);
		}
	}
	if (mImage->getUploader().notNull())
	{
		childSetText("uploader", getItemCreatorName());
		childSetText("uploadtime", getItemCreationDate());
	}
	const LLRect slot = getImageSlot();
	const S32 avail_w = llmax(1, slot.getWidth());
	const S32 avail_h = llmax(1, slot.getHeight());
	const F32 aspect = (mAspectRatio > 0.f)
		? mAspectRatio
		: (F32)client_width / (F32)llmax(1, client_height);
	if (mUserResized)
	{
		client_width = avail_w;
		client_height = llmax(1, ll_round(client_width / aspect));
	}
	if (client_width > avail_w || client_height > avail_h)
	{
		client_width = avail_w;
		client_height = llmax(1, ll_round(client_width / aspect));
		if (client_height > avail_h)
		{
			client_height = avail_h;
			client_width = llmax(1, ll_round(client_height * aspect));
		}
	}
	client_width = llclamp(client_width, 1, avail_w);
	client_height = llclamp(client_height, 1, avail_h);
	mClientRect.setLeftTopAndSize(
		slot.mLeft + (avail_w - client_width) / 2,
		slot.mTop,
		client_width,
		client_height);
	LLRect dim_rect, aspect_label_rect;
	childGetRect("aspect_ratio", aspect_label_rect);
	childGetRect("dimensions", dim_rect);
	childSetVisible("aspect_ratio", dim_rect.mRight < aspect_label_rect.mLeft);
}
bool LLPreviewTexture::setAspectRatio(const F32 width, const F32 height)
{
	mUpdateDimensions = TRUE;
	if ((width <= 0.f) || (height <= F_APPROXIMATELY_ZERO))
	{
		mAspectRatio = 0.f;
		return false;
	}
	F32 ratio = width / height;
	mAspectRatio = llclamp(ratio, PREVIEW_TEXTURE_MIN_ASPECT, PREVIEW_TEXTURE_MAX_ASPECT);
	return (ratio == mAspectRatio);
}
void LLPreviewTexture::onClickProfile(void* userdata)
{
	LLPreviewTexture* self = (LLPreviewTexture*) userdata;
	LLAvatarActions::showProfile(self->mCreatorKey);
}
void LLPreviewTexture::onAspectRatioCommit(LLUICtrl* ctrl, void* userdata)
{
	LLPreviewTexture* self = (LLPreviewTexture*) userdata;
	std::string ratio(ctrl->getValue().asString());
	std::string::size_type separator(ratio.find_first_of(":/\\"));
	if (std::string::npos == separator) {
		self->setAspectRatio(0.0f, 0.0f);
		return;
	}
	F32 width, height;
	std::istringstream numerator(ratio.substr(0, separator));
	std::istringstream denominator(ratio.substr(separator + 1));
	numerator >> width;
	denominator >> height;
	self->setAspectRatio(width, height);
}
void LLPreviewTexture::loadAsset()
{
	mImage = LLViewerTextureManager::getFetchedTexture(mImageID, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	mImageOldBoostLevel = mImage->getBoostLevel();
	mImage->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
	mImage->forceToSaveRawImage(0) ;
	mAssetStatus = PREVIEW_ASSET_LOADING;
	mUpdateDimensions = TRUE;
	updateDimensions();
}
LLPreview::EAssetStatus LLPreviewTexture::getAssetStatus()
{
	if (mImage.notNull() && (mImage->getFullWidth() * mImage->getFullHeight() > 0))
	{
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
	return mAssetStatus;
}
