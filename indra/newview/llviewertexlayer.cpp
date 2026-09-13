/** 
 * @file lltexlayer.cpp
 * @brief A texture layer. Used for avatars.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"
#include "lltexlayer.h"
#include "llviewertexlayer.h"
#include "llagent.h"
#include "llimagej2c.h"
#include "llnotificationsutil.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llviewerregion.h"
#include "llglslshader.h"
#include "llvoavatarself.h"
#include "pipeline.h"
#include "llassetuploadresponders.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
static const S32 BAKE_UPLOAD_ATTEMPTS = 7;
static const F32 BAKE_UPLOAD_RETRY_DELAY = 2.f;
extern std::string self_av_string();
LLBakedUploadData::LLBakedUploadData(const LLVOAvatarSelf* avatar,
									 LLViewerTexLayerSet* layerset,
									 const LLUUID& id,
									 bool highest_res) :
	mAvatar(avatar),
	mTexLayerSet(layerset),
	mID(id),
	mStartTime(LLFrameTimer::getTotalTime()),
	mIsHighestRes(highest_res)
{
}
S32 LLViewerTexLayerSetBuffer::sGLByteCount = 0;
LLViewerTexLayerSetBuffer::LLViewerTexLayerSetBuffer(LLTexLayerSet* const owner,
										 S32 width, S32 height) :
	LLTexLayerSetBuffer(owner),
	LLViewerDynamicTexture( width, height, 4, LLViewerDynamicTexture::ORDER_LAST, FALSE ),
	mUploadPending(FALSE),
	mNeedsUpload(FALSE),
	mNumLowresUploads(0),
	mUploadFailCount(0),
	mNeedsUpdate(TRUE),
	mNumLowresUpdates(0)
{
	LLViewerTexLayerSetBuffer::sGLByteCount += getSize();
	mNeedsUploadTimer.start();
	mNeedsUpdateTimer.start();
}
LLViewerTexLayerSetBuffer::~LLViewerTexLayerSetBuffer()
{
	LLViewerTexLayerSetBuffer::sGLByteCount -= getSize();
	destroyGLTexture();
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		LLViewerDynamicTexture::sInstances[order].erase(this);
	}
}
S8 LLViewerTexLayerSetBuffer::getType() const
{
	return LLViewerDynamicTexture::LL_TEX_LAYER_SET_BUFFER ;
}
void LLViewerTexLayerSetBuffer::restoreGLTexture()
{
	LLViewerDynamicTexture::restoreGLTexture() ;
}
void LLViewerTexLayerSetBuffer::destroyGLTexture()
{
	LLViewerDynamicTexture::destroyGLTexture() ;
}
void LLViewerTexLayerSetBuffer::dumpTotalByteCount()
{
	LL_INFOS() << "Composite System GL Buffers: " << (LLViewerTexLayerSetBuffer::sGLByteCount/1024) << "KB" << LL_ENDL;
}
void LLViewerTexLayerSetBuffer::requestUpdate()
{
	restartUpdateTimer();
	mNeedsUpdate = TRUE;
	mNumLowresUpdates = 0;
	mUploadID.setNull();
}
void LLViewerTexLayerSetBuffer::requestUpload()
{
	conditionalRestartUploadTimer();
	mNeedsUpload = TRUE;
	mNumLowresUploads = 0;
	mUploadPending = TRUE;
}
void LLViewerTexLayerSetBuffer::conditionalRestartUploadTimer()
{
	if (mNeedsUpload && (mNumLowresUploads == 0))
	{
		mNeedsUploadTimer.unpause();
	}
	else
	{
		mNeedsUploadTimer.unpause();
		mNeedsUploadTimer.reset();
		mNeedsUploadTimer.start();
	}
}
void LLViewerTexLayerSetBuffer::restartUpdateTimer()
{
	mNeedsUpdateTimer.reset();
	mNeedsUpdateTimer.start();
}
void LLViewerTexLayerSetBuffer::cancelUpload()
{
	mNeedsUpload = FALSE;
	mUploadPending = FALSE;
	mNeedsUploadTimer.pause();
	mUploadRetryTimer.reset();
}
BOOL LLViewerTexLayerSetBuffer::needsRender()
{
	llassert(mTexLayerSet->getAvatarAppearance() == gAgentAvatarp);
	if (!isAgentAvatarValid()) return FALSE;
	const BOOL upload_now = mNeedsUpload && isReadyToUpload();
	const BOOL update_now = mNeedsUpdate && isReadyToUpdate();
	if (!(update_now || upload_now))
	{
		return FALSE;
	}
	if (gAgentAvatarp->getIsAppearanceAnimating())
	{
		return FALSE;
	}
	if (gAgentAvatarp->getBakedTE(getViewerTexLayerSet()) == LLAvatarAppearanceDefines::TEX_SKIRT_BAKED &&
		!gAgentAvatarp->isWearingWearableType(LLWearableType::WT_SKIRT))
	{
		cancelUpload();
		return FALSE;
	}
	return getViewerTexLayerSet()->isLocalTextureDataAvailable();
}
void LLViewerTexLayerSetBuffer::preRenderTexLayerSet()
{
	LLTexLayerSetBuffer::preRenderTexLayerSet();
	LLViewerDynamicTexture::preRender(FALSE);
}
void LLViewerTexLayerSetBuffer::postRenderTexLayerSet(BOOL success)
{
	LLTexLayerSetBuffer::postRenderTexLayerSet(success);
	LLViewerDynamicTexture::postRender(success);
}
void LLViewerTexLayerSetBuffer::midRenderTexLayerSet(BOOL success)
{
	const BOOL upload_now = mNeedsUpload && isReadyToUpload();
	const BOOL update_now = mNeedsUpdate && isReadyToUpdate();
	if(upload_now)
	{
		if (!success)
		{
			LL_INFOS() << "Failed attempt to bake " << mTexLayerSet->getBodyRegionName() << LL_ENDL;
			mUploadPending = FALSE;
		}
		else
		{
			LLViewerTexLayerSet* layer_set = getViewerTexLayerSet();
			if (layer_set->isVisible())
			{
				layer_set->getAvatar()->debugBakedTextureUpload(layer_set->getBakedTexIndex(), FALSE);
				doUpload();
			}
			else
			{
				mUploadPending = FALSE;
				mNeedsUpload = FALSE;
				mNeedsUploadTimer.pause();
				layer_set->getAvatar()->setNewBakedTexture(layer_set->getBakedTexIndex(),IMG_INVISIBLE);
			}
		}
	}
	if (update_now)
	{
		doUpdate();
	}
	mGLTexturep->setGLTextureCreated(true);
}
BOOL LLViewerTexLayerSetBuffer::isInitialized(void) const
{
	return mGLTexturep.notNull() && mGLTexturep->isGLTextureCreated();
}
BOOL LLViewerTexLayerSetBuffer::uploadPending() const
{
	return mUploadPending;
}
BOOL LLViewerTexLayerSetBuffer::uploadNeeded() const
{
	return mNeedsUpload;
}
BOOL LLViewerTexLayerSetBuffer::uploadInProgress() const
{
	return !mUploadID.isNull();
}
BOOL LLViewerTexLayerSetBuffer::isReadyToUpload() const
{
	if (!gAgentQueryManager.hasNoPendingQueries()) return FALSE;
	if (isAgentAvatarValid() && gAgentAvatarp->isEditingAppearance()) return FALSE;
	BOOL ready = FALSE;
	if (getViewerTexLayerSet()->isLocalTextureDataFinal())
	{
		if (mUploadFailCount == 0)
		{
			ready = TRUE;
		}
		else
		{
			ready = mUploadRetryTimer.getElapsedTimeF32() >= BAKE_UPLOAD_RETRY_DELAY * (1 << (mUploadFailCount - 1));
		}
	}
	else
	{
		const U32 texture_timeout = gSavedSettings.getU32("AvatarBakedTextureUploadTimeout");
		if (texture_timeout != 0)
		{
			const U32 texture_timeout_threshold = texture_timeout*(1 << mNumLowresUploads);
			const BOOL is_upload_textures_timeout = mNeedsUploadTimer.getElapsedTimeF32() >= texture_timeout_threshold;
			const BOOL has_lower_lod = getViewerTexLayerSet()->isLocalTextureDataAvailable();
			ready = has_lower_lod && is_upload_textures_timeout;
		}
	}
	return ready;
}
BOOL LLViewerTexLayerSetBuffer::isReadyToUpdate() const
{
	if (getViewerTexLayerSet()->isLocalTextureDataFinal()) return TRUE;
	if (mNumLowresUpdates == 0) return TRUE;
	const U32 texture_timeout = gSavedSettings.getU32("AvatarBakedLocalTextureUpdateTimeout");
	if (texture_timeout != 0)
	{
		const BOOL is_update_textures_timeout = mNeedsUpdateTimer.getElapsedTimeF32() >= texture_timeout;
		const BOOL has_lower_lod = getViewerTexLayerSet()->isLocalTextureDataAvailable();
		if (has_lower_lod && is_update_textures_timeout) return TRUE;
	}
	return FALSE;
}
BOOL LLViewerTexLayerSetBuffer::requestUpdateImmediate()
{
	mNeedsUpdate = TRUE;
	BOOL result = FALSE;
	if (needsRender())
	{
		preRender(FALSE);
		result = render();
		postRender(result);
	}
	return result;
}
class LLSendTexLayerResponder : public LLAssetUploadResponder
{
	LOG_CLASS(LLSendTexLayerResponder);
public:
	LLSendTexLayerResponder(const LLSD& post_data, const LLUUID& vfile_id, LLAssetType::EType asset_type, LLBakedUploadData * baked_upload_data)
	: LLAssetUploadResponder(post_data, vfile_id, asset_type)
	, mBakedUploadData(baked_upload_data)
	{}
	~LLSendTexLayerResponder()
	{
		if (mBakedUploadData)
		{
			delete mBakedUploadData;
			mBakedUploadData = NULL;
		}
	}
	void uploadComplete(const LLSD& content)
	{
		const std::string& result = content["state"];
		const LLUUID& new_id = content["new_asset"];
		LL_INFOS() << "result: " << result << " new_id: " << new_id << LL_ENDL;
		LLViewerTexLayerSetBuffer::onTextureUploadComplete(new_id, (void*) mBakedUploadData, (result == "complete" && mBakedUploadData) ? 0 : -1, LL_EXSTAT_NONE);
		mBakedUploadData = NULL;
	}
	void httpFailure()
	{
		LL_INFOS() << dumpResponse() << LL_ENDL;
		LLViewerTexLayerSetBuffer::onTextureUploadComplete(LLUUID::null, (void*) mBakedUploadData, -1, LL_EXSTAT_NONE);
		mBakedUploadData = NULL;
	}
	char const* getName() const { return "LLSendTexLayerResponder"; }
private:
	LLBakedUploadData* mBakedUploadData;
};
void LLViewerTexLayerSetBuffer::doUpload()
{
	LLViewerTexLayerSet* layer_set = getViewerTexLayerSet();
	LL_INFOS() << "Uploading baked " << layer_set->getBodyRegionName() << LL_ENDL;
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_TEX_BAKES);
	layer_set->deleteCaches();
	U8* baked_color_data = new U8[ mFullWidth * mFullHeight * 4 ];
	glReadPixels(mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, GL_RGBA, GL_UNSIGNED_BYTE, baked_color_data );
	stop_glerror();
	LLGLSUIDefault gls_ui;
	LLPointer<LLImageRaw> baked_mask_image = new LLImageRaw(mFullWidth, mFullHeight, 1 );
	U8* baked_mask_data = baked_mask_image->getData();
	layer_set->gatherMorphMaskAlpha(baked_mask_data,
									mOrigin.mX, mOrigin.mY,
									mFullWidth, mFullHeight);
	const S32 baked_image_components = 5;
	LLPointer<LLImageRaw> baked_image = new LLImageRaw( mFullWidth, mFullHeight, baked_image_components );
	U8* baked_image_data = baked_image->getData();
	S32 i = 0;
	for (S32 u=0; u < mFullWidth; u++)
	{
		for (S32 v=0; v < mFullHeight; v++)
		{
			baked_image_data[5*i + 0] = baked_color_data[4*i + 0];
			baked_image_data[5*i + 1] = baked_color_data[4*i + 1];
			baked_image_data[5*i + 2] = baked_color_data[4*i + 2];
			baked_image_data[5*i + 3] = baked_color_data[4*i + 3];
			baked_image_data[5*i + 4] = baked_mask_data[i];
			i++;
		}
	}
	LLPointer<LLImageJ2C> compressedImage = new LLImageJ2C;
	const char* comment_text = LINDEN_J2C_COMMENT_PREFIX "RGBHM";
	if (compressedImage->encode(baked_image, comment_text))
	{
		LLTransactionID tid;
		tid.generate();
		const LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
		if (LLVFile::writeFile(compressedImage->getData(), compressedImage->getDataSize(),
							   gVFS, asset_id, LLAssetType::AT_TEXTURE))
		{
			BOOL valid = FALSE;
			LLPointer<LLImageJ2C> integrity_test = new LLImageJ2C;
			S32 file_size = 0;
			U8* data = LLVFile::readFile(gVFS, LLImageBase::getPrivatePool(), asset_id, LLAssetType::AT_TEXTURE, &file_size);
			if (data)
			{
				valid = integrity_test->validate(data, file_size);
			}
			else
			{
				integrity_test->setLastError("Unable to read entire file");
			}
			if (valid)
			{
				const bool highest_lod = layer_set->isLocalTextureDataFinal();
				LLBakedUploadData* baked_upload_data = new LLBakedUploadData(gAgentAvatarp,
																			 layer_set,
																			 asset_id,
																			 highest_lod);
				mUploadID = asset_id;
				const std::string url = gAgent.getRegion()->getCapability("UploadBakedTexture");
				if(!url.empty()
					&& !LLPipeline::sForceOldBakedUpload
					&& (mUploadFailCount < (BAKE_UPLOAD_ATTEMPTS - 1)))
				{
					LLSD body = LLSD::emptyMap();
					LLHTTPClient::post(url, body, new LLSendTexLayerResponder(body, mUploadID, LLAssetType::AT_TEXTURE, baked_upload_data));
					LL_INFOS() << "Baked texture upload via capability of " << mUploadID << " to " << url << LL_ENDL;
				}
				else
				{
					gAssetStorage->storeAssetData(tid,
												  LLAssetType::AT_TEXTURE,
												  LLViewerTexLayerSetBuffer::onTextureUploadComplete,
												  baked_upload_data,
												  TRUE,
												  TRUE,
												  TRUE);
					LL_INFOS() << "Baked texture upload via Asset Store." <<  LL_ENDL;
				}
				if (highest_lod)
				{
					mNeedsUpload = FALSE;
					mNeedsUploadTimer.pause();
				}
				else
				{
					mNumLowresUploads++;
					mNeedsUploadTimer.unpause();
					mNeedsUploadTimer.reset();
				}
				if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
				{
					const std::string lod_str = highest_lod ? "HighRes" : "LowRes";
					LLSD args;
					args["EXISTENCE"] = llformat("%d",(U32)layer_set->getAvatar()->debugGetExistenceTimeElapsedF32());
					args["TIME"] = llformat("%d",(U32)mNeedsUploadTimer.getElapsedTimeF32());
					args["BODYREGION"] = layer_set->getBodyRegionName();
					args["RESOLUTION"] = lod_str;
					LLNotificationsUtil::add("AvatarRezSelfBakedTextureUploadNotification",args);
					LL_DEBUGS("Avatar") << self_av_string() << "Uploading [ name: " << layer_set->getBodyRegionName() << " res:" << lod_str << " time:" << (U32)mNeedsUploadTimer.getElapsedTimeF32() << " ]" << LL_ENDL;
				}
			}
			else
			{
				mUploadPending = FALSE;
				LLVFile file(gVFS, asset_id, LLAssetType::AT_TEXTURE, LLVFile::WRITE);
				file.remove();
				LL_INFOS() << "Unable to create baked upload file (reason: corrupted)." << LL_ENDL;
			}
		}
	}
	else
	{
		mUploadPending = FALSE;
		LL_INFOS() << "Unable to create baked upload file (reason: failed to write file)" << LL_ENDL;
	}
	delete [] baked_color_data;
}
void LLViewerTexLayerSetBuffer::doUpdate()
{
	LLViewerTexLayerSet* layer_set = getViewerTexLayerSet();
	const BOOL highest_lod = layer_set->isLocalTextureDataFinal();
	if (highest_lod)
	{
		mNeedsUpdate = FALSE;
	}
	else
	{
		mNumLowresUpdates++;
	}
	restartUpdateTimer();
	layer_set->getAvatar()->updateMeshTextures();
	if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
	{
		const BOOL highest_lod = layer_set->isLocalTextureDataFinal();
		const std::string lod_str = highest_lod ? "HighRes" : "LowRes";
		LLSD args;
		args["EXISTENCE"] = llformat("%d",(U32)layer_set->getAvatar()->debugGetExistenceTimeElapsedF32());
		args["TIME"] = llformat("%d",(U32)mNeedsUpdateTimer.getElapsedTimeF32());
		args["BODYREGION"] = layer_set->getBodyRegionName();
		args["RESOLUTION"] = lod_str;
		LLNotificationsUtil::add("AvatarRezSelfBakedTextureUpdateNotification",args);
		LL_DEBUGS("Avatar") << self_av_string() << "Locally updating [ name: " << layer_set->getBodyRegionName() << " res:" << lod_str << " time:" << (U32)mNeedsUpdateTimer.getElapsedTimeF32() << " ]" << LL_ENDL;
	}
}
void LLViewerTexLayerSetBuffer::onTextureUploadComplete(const LLUUID& uuid,
												  void* userdata,
												  S32 result,
												  LLExtStat ext_status)
{
	LLBakedUploadData* baked_upload_data = (LLBakedUploadData*)userdata;
	if (isAgentAvatarValid() &&
		!gAgentAvatarp->isDead() &&
		(baked_upload_data->mAvatar == gAgentAvatarp) &&
		(baked_upload_data->mTexLayerSet->hasComposite()))
	{
		LLViewerTexLayerSetBuffer* layerset_buffer = baked_upload_data->mTexLayerSet->getViewerComposite();
		S32 failures = layerset_buffer->mUploadFailCount;
		layerset_buffer->mUploadFailCount = 0;
		if (layerset_buffer->mUploadID.isNull())
		{
			layerset_buffer->requestUpload();
		}
		else if (baked_upload_data->mID == layerset_buffer->mUploadID)
		{
			layerset_buffer->mUploadID.setNull();
			const std::string name(baked_upload_data->mTexLayerSet->getBodyRegionName());
			const std::string resolution = baked_upload_data->mIsHighestRes ? " full res " : " low res ";
			if (result >= 0)
			{
				layerset_buffer->mUploadPending = FALSE;
				LLAvatarAppearanceDefines::ETextureIndex baked_te = gAgentAvatarp->getBakedTE(layerset_buffer->getViewerTexLayerSet());
				U64 now = LLFrameTimer::getTotalTime();
				LL_INFOS() << "Baked" << resolution << "texture upload for " << name << " took " << (S32)((now - baked_upload_data->mStartTime) / 1000) << " ms" << LL_ENDL;
				gAgentAvatarp->setNewBakedTexture(baked_te, uuid);
			}
			else
			{
				++failures;
				S32 max_attempts = baked_upload_data->mIsHighestRes ? BAKE_UPLOAD_ATTEMPTS : 1;
				LL_WARNS() << "Baked" << resolution << "texture upload for " << name << " failed (attempt " << failures << "/" << max_attempts << ")" << LL_ENDL;
				if (failures < max_attempts)
				{
					layerset_buffer->mUploadFailCount = failures;
					layerset_buffer->mUploadRetryTimer.start();
					layerset_buffer->requestUpload();
				}
			}
		}
		else
		{
			LL_INFOS() << "Received baked texture out of date, ignored." << LL_ENDL;
		}
		gAgentAvatarp->dirtyMesh();
	}
	else
	{
		LL_WARNS() << "Baked upload failed" << LL_ENDL;
	}
	delete baked_upload_data;
}
LLViewerTexLayerSet::LLViewerTexLayerSet(LLAvatarAppearance* const appearance) :
	LLTexLayerSet(appearance),
	mUpdatesEnabled( FALSE )
{
}
LLViewerTexLayerSet::~LLViewerTexLayerSet()
{
}
BOOL LLViewerTexLayerSet::isLocalTextureDataAvailable() const
{
	if (!mAvatarAppearance->isSelf()) return FALSE;
	return getAvatar()->isLocalTextureDataAvailable(this);
}
BOOL LLViewerTexLayerSet::isLocalTextureDataFinal() const
{
	if (!mAvatarAppearance->isSelf()) return FALSE;
	return getAvatar()->isLocalTextureDataFinal(this);
}
void LLViewerTexLayerSet::requestUpdate()
{
	if( mUpdatesEnabled )
	{
		createComposite();
		getViewerComposite()->requestUpdate();
	}
}
void LLViewerTexLayerSet::requestUpload()
{
	createComposite();
	getViewerComposite()->requestUpload();
}
void LLViewerTexLayerSet::cancelUpload()
{
	if(mComposite)
	{
		getViewerComposite()->cancelUpload();
	}
}
void LLViewerTexLayerSet::updateComposite()
{
	createComposite();
	getViewerComposite()->requestUpdateImmediate();
}
void LLViewerTexLayerSet::createComposite()
{
	if(!mComposite)
	{
		S32 width = mInfo->getWidth();
		S32 height = mInfo->getHeight();
		if( !mAvatarAppearance->isSelf() )
		{
			LL_ERRS() << "composites should not be created for non-self avatars!" << LL_ENDL;
		}
		mComposite = new LLViewerTexLayerSetBuffer( this, width, height );
	}
}
void LLViewerTexLayerSet::setUpdatesEnabled( BOOL b )
{
	mUpdatesEnabled = b;
}
LLVOAvatarSelf* LLViewerTexLayerSet::getAvatar()
{
	return dynamic_cast<LLVOAvatarSelf*> (mAvatarAppearance);
}
const LLVOAvatarSelf* LLViewerTexLayerSet::getAvatar() const
{
	return dynamic_cast<const LLVOAvatarSelf*> (mAvatarAppearance);
}
LLViewerTexLayerSetBuffer* LLViewerTexLayerSet::getViewerComposite()
{
	return dynamic_cast<LLViewerTexLayerSetBuffer*> (getComposite());
}
const LLViewerTexLayerSetBuffer* LLViewerTexLayerSet::getViewerComposite() const
{
	return dynamic_cast<const LLViewerTexLayerSetBuffer*> (getComposite());
}
const std::string LLViewerTexLayerSetBuffer::dumpTextureInfo() const
{
	if (!isAgentAvatarValid()) return "";
	const BOOL is_high_res = !mNeedsUpload;
	const U32 num_low_res = mNumLowresUploads;
	const U32 upload_time = (U32)mNeedsUploadTimer.getElapsedTimeF32();
	const std::string local_texture_info = gAgentAvatarp->debugDumpLocalTextureDataInfo(getViewerTexLayerSet());
	std::string status 				= "CREATING ";
	if (!uploadNeeded()) status 	= "DONE     ";
	if (uploadInProgress()) status 	= "UPLOADING";
	std::string text = llformat("[%s] [HiRes:%d LoRes:%d] [Elapsed:%d] %s",
								status.c_str(),
								is_high_res, num_low_res,
								upload_time,
								local_texture_info.c_str());
	return text;
}
