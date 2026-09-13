/** 
 * @file llvovolume.cpp
 * @brief LLVOVolume class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
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
#include "llvovolume.h"
#include <sstream>
#include "llviewercontrol.h"
#include "lldir.h"
#include "llflexibleobject.h"
#include "llfloaterinspect.h"
#include "llfloatertools.h"
#include "llmaterialid.h"
#include "llmaterialtable.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "llvolumeoctree.h"
#include "llvolumemgr.h"
#include "llvolumemessage.h"
#include "material_codes.h"
#include "message.h"
#include "llpluginclassmedia.h"
#include "object_flags.h"
#include "llagentconstants.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llspatialpartition.h"
#include "llhudmanager.h"
#include "llflexibleobject.h"
#include "llskinningutil.h"
#include "llsky.h"
#include "lltexturefetch.h"
#include "llvector4a.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertextureanim.h"
#include "llworld.h"
#include "llselectmgr.h"
#include "pipeline.h"
#include "llsdutil.h"
#include "llmatrix4a.h"
#include "llmediaentry.h"
#include "llmediadataclient.h"
#include "llmeshrepository.h"
#include "llagent.h"
#include "llviewermediafocus.h"
#include "lldatapacker.h"
#include "llviewershadermgr.h"
#include "llvoavatar.h"
#include "llcontrolavatar.h"
#include "llvoavatarself.h"
#include "llvocache.h"
#include "llmaterialmgr.h"
#include "llsculptidsize.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
const S32 MIN_QUIET_FRAMES_COALESCE = 30;
const F32 FORCE_SIMPLE_RENDER_AREA = 512.f;
const F32 FORCE_CULL_AREA = 8.f;
U32 JOINT_COUNT_REQUIRED_FOR_FULLRIG = 1;
BOOL gAnimateTextures = TRUE;
F32 LLVOVolume::sLODFactor = 1.f;
F32	LLVOVolume::sLODSlopDistanceFactor = 0.5f;
F32 LLVOVolume::sDistanceFactor = 1.0f;
S32 LLVOVolume::sNumLODChanges = 0;
S32 LLVOVolume::mRenderComplexity_last = 0;
S32 LLVOVolume::mRenderComplexity_current = 0;
LLPointer<LLObjectMediaDataClient> LLVOVolume::sObjectMediaClient = NULL;
LLPointer<LLObjectMediaNavigateClient> LLVOVolume::sObjectMediaNavigateClient = NULL;
static LLTrace::BlockTimerStatHandle FTM_GEN_TRIANGLES("Generate Triangles");
static LLTrace::BlockTimerStatHandle FTM_GEN_VOLUME("Generate Volumes");
static LLTrace::BlockTimerStatHandle FTM_VOLUME_TEXTURES("Volume Textures");
class LLMediaDataClientObjectImpl : public LLMediaDataClientObject
{
public:
	LLMediaDataClientObjectImpl(LLVOVolume *obj, bool isNew) : mObject(obj), mNew(isNew)
	{
		mObject->addMDCImpl();
	}
	~LLMediaDataClientObjectImpl()
	{
		mObject->removeMDCImpl();
	}
	virtual U8 getMediaDataCount() const
		{ return mObject->getNumTEs(); }
	virtual LLSD getMediaDataLLSD(U8 index) const
		{
			LLSD result;
			LLTextureEntry *te = mObject->getTE(index);
			if (NULL != te)
			{
				llassert((te->getMediaData() != NULL) == te->hasMedia());
				if (te->getMediaData() != NULL)
				{
					result = te->getMediaData()->asLLSD();
					if (!result.has(LLMediaEntry::WHITELIST_KEY))
					{
						result[LLMediaEntry::WHITELIST_KEY] = LLSD::emptyArray();
					}
				}
			}
			return result;
		}
	virtual bool isCurrentMediaUrl(U8 index, const std::string &url) const
		{
			LLTextureEntry *te = mObject->getTE(index);
			if (te)
			{
				if (te->getMediaData())
				{
					return (te->getMediaData()->getCurrentURL() == url);
				}
			}
			return url.empty();
		}
	virtual LLUUID getID() const
		{ return mObject->getID(); }
	virtual void mediaNavigateBounceBack(U8 index)
		{ mObject->mediaNavigateBounceBack(index); }
	virtual bool hasMedia() const
		{ return mObject->hasMedia(); }
	virtual void updateObjectMediaData(LLSD const &data, const std::string &version_string)
		{ mObject->updateObjectMediaData(data, version_string); }
	virtual F64 getMediaInterest() const
		{
			F64 interest = mObject->getTotalMediaInterest();
			if (interest < (F64)0.0)
			{
				interest = mObject->getPixelArea();
				if (interest == 1024.f)
				{
					const_cast<LLVOVolume*>(static_cast<LLVOVolume*>(mObject))->setPixelAreaAndAngle(gAgent);
					interest = mObject->getPixelArea();
				}
			}
			return interest;
		}
	virtual bool isInterestingEnough() const
		{
			return LLViewerMedia::isInterestingEnough(mObject, getMediaInterest());
		}
	virtual std::string getCapabilityUrl(const std::string &name) const
		{ return mObject->getRegion()->getCapability(name); }
	virtual bool isDead() const
		{ return mObject->isDead(); }
	virtual U32 getMediaVersion() const
		{ return LLTextureEntry::getVersionFromMediaVersionString(mObject->getMediaURL()); }
	virtual bool isNew() const
		{ return mNew; }
private:
	LLPointer<LLVOVolume> mObject;
	bool mNew;
};
LLVOVolume::LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLViewerObject(id, pcode, regionp),
	  mVolumeImpl(NULL)
{
	mTexAnimMode = 0;
	mRelativeXform.setIdentity();
	mRelativeXformInvTrans.setIdentity();
	mFaceMappingChanged = FALSE;
	mLOD = MIN_LOD;
	mLODDistance = 0.0f;
	mLODAdjustedDistance = 0.0f;
	mLODRadius = 0.0f;
	mTextureAnimp = NULL;
	mVolumeChanged = FALSE;
	mVObjRadius = LLVector3(1,1,0.5f).length();
	mNumFaces = 0;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
	mSpotLightPriority = 0.f;
	mMediaImplList.resize(getNumTEs());
	mLastFetchedMediaVersion = -1;
	memset(&mIndexInTex, 0, sizeof(S32) * LLRender::NUM_VOLUME_TEXTURE_CHANNELS);
	mMDCImplCount = 0;
	mLastRiggingInfoLOD = -1;
}
LLVOVolume::~LLVOVolume()
{
	delete mTextureAnimp;
	mTextureAnimp = NULL;
	delete mVolumeImpl;
	mVolumeImpl = NULL;
	gMeshRepo.unregisterMesh(this);
	if(!mMediaImplList.empty())
	{
		for(U32 i = 0 ; i < mMediaImplList.size() ; i++)
		{
			if(mMediaImplList[i].notNull())
			{
				mMediaImplList[i]->removeObject(this) ;
			}
		}
	}
}
LLVOVolume* LLVOVolume::asVolume()
{
	return this;
}
void LLVOVolume::markDead()
{
	if (!mDead)
	{
		LLSculptIDSize::instance().rem(getVolume()->getParams().getSculptID());
		if(getMDCImplCount() > 0)
		{
			LLMediaDataClientObject::ptr_t obj = new LLMediaDataClientObjectImpl(const_cast<LLVOVolume*>(this), false);
			if (sObjectMediaClient) sObjectMediaClient->removeFromQueue(obj);
			if (sObjectMediaNavigateClient) sObjectMediaNavigateClient->removeFromQueue(obj);
		}
		for(U32 i = 0 ; i < mMediaImplList.size() ; i++)
		{
			removeMediaImpl(i);
		}
		if (mSculptTexture.notNull())
		{
			mSculptTexture->removeVolume(LLRender::SCULPT_TEX, this);
		}
		if (mLightTexture.notNull())
		{
			mLightTexture->removeVolume(LLRender::LIGHT_TEX, this);
		}
	}
	LLViewerObject::markDead();
}
void LLVOVolume::initClass()
{
	if (gSavedSettings.getBOOL("PrimMediaMasterEnabled"))
	{
		const F32 queue_timer_delay = gSavedSettings.getF32("PrimMediaRequestQueueDelay");
		const F32 retry_timer_delay = gSavedSettings.getF32("PrimMediaRetryTimerDelay");
		const U32 max_retries = gSavedSettings.getU32("PrimMediaMaxRetries");
		const U32 max_sorted_queue_size = gSavedSettings.getU32("PrimMediaMaxSortedQueueSize");
		const U32 max_round_robin_queue_size = gSavedSettings.getU32("PrimMediaMaxRoundRobinQueueSize");
		sObjectMediaClient = new LLObjectMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries,
														 max_sorted_queue_size, max_round_robin_queue_size);
		sObjectMediaNavigateClient = new LLObjectMediaNavigateClient(queue_timer_delay, retry_timer_delay,
																	 max_retries, max_sorted_queue_size, max_round_robin_queue_size);
	}
}
void LLVOVolume::cleanupClass()
{
    sObjectMediaClient = NULL;
    sObjectMediaNavigateClient = NULL;
}
U32 LLVOVolume::processUpdateMessage(LLMessageSystem *mesgsys,
										  void **user_data,
										  U32 block_num, EObjectUpdateType update_type,
										  LLDataPacker *dp)
{
	LLColor4U color;
	const S32 teDirtyBits = (TEM_CHANGE_TEXTURE|TEM_CHANGE_COLOR|TEM_CHANGE_MEDIA);
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);
	LLUUID sculpt_id;
	U8 sculpt_type = 0;
	if (isSculpted())
	{
		const LLSculptParams *sculpt_params = getSculptParams();
		sculpt_id = sculpt_params->getSculptTexture();
		sculpt_type = sculpt_params->getSculptType();
	}
	if (!dp)
	{
		if (update_type == OUT_FULL)
		{
			if (mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_TextureAnim))
			{
				if (!mTextureAnimp)
				{
					mTextureAnimp = new LLViewerTextureAnim(this);
					mTexAnimMode = 0;
				}
				else
				{
					if (!(mTextureAnimp->mMode & LLTextureAnim::SMOOTH))
					{
						mTextureAnimp->reset();
					}
				}
				mTextureAnimp->unpackTAMessage(mesgsys, block_num);
			}
			else
			{
				if (mTextureAnimp)
				{
					delete mTextureAnimp;
					mTextureAnimp = NULL;
					gPipeline.markTextured(mDrawable);
					mFaceMappingChanged = TRUE;
					mTexAnimMode = 0;
				}
			}
			LLVolumeParams volume_params;
			LLVolumeMessage::unpackVolumeParams(&volume_params, mesgsys, _PREHASH_ObjectData, block_num);
			volume_params.setSculptID(sculpt_id, sculpt_type);
			if (setVolume(volume_params, 0))
			{
				markForUpdate(TRUE);
			}
		}
		S32 result = unpackTEMessage(mesgsys, _PREHASH_ObjectData, (S32) block_num);
		if (result & teDirtyBits)
		{
			updateTEData();
		}
		if (result & TEM_CHANGE_MEDIA)
		{
			retval |= MEDIA_FLAGS_CHANGED;
		}
	}
	else
	{
		if (update_type != OUT_TERSE_IMPROVED)
		{
			LLVolumeParams volume_params;
			BOOL res = LLVolumeMessage::unpackVolumeParams(&volume_params, *dp);
			if (!res)
			{
				LL_WARNS() << "Bogus volume parameters in object " << getID() << LL_ENDL;
				LL_WARNS() << getRegion()->getOriginGlobal() << LL_ENDL;
			}
			volume_params.setSculptID(sculpt_id, sculpt_type);
			if (setVolume(volume_params, 0))
			{
				markForUpdate(TRUE);
			}
			S32 res2 = unpackTEMessage(*dp);
			if (TEM_INVALID == res2)
			{
				dp->dumpBufferToLog();
				LL_WARNS() << "Flushing cache files" << LL_ENDL;
				if(LLVOCache::hasInstance() && getRegion())
				{
					LLVOCache::getInstance()->removeEntry(getRegion()->getHandle()) ;
				}
				LL_WARNS() << "Bogus TE data in " << getID() << LL_ENDL;
			}
			else
			{
				if (res2 & teDirtyBits)
				{
					updateTEData();
				}
				if (res2 & TEM_CHANGE_MEDIA)
				{
					retval |= MEDIA_FLAGS_CHANGED;
				}
			}
			U32 value = dp->getPassFlags();
			if (value & 0x40)
			{
				if (!mTextureAnimp)
				{
					mTextureAnimp = new LLViewerTextureAnim(this);
				}
				else
				{
					if (!(mTextureAnimp->mMode & LLTextureAnim::SMOOTH))
					{
						mTextureAnimp->reset();
					}
				}
				mTexAnimMode = 0;
				mTextureAnimp->unpackTAMessage(*dp);
			}
			else if (mTextureAnimp)
			{
				delete mTextureAnimp;
				mTextureAnimp = NULL;
				gPipeline.markTextured(mDrawable);
				mFaceMappingChanged = TRUE;
				mTexAnimMode = 0;
			}
			if (value & 0x400)
			{
				unpackParticleSource(*dp, mOwnerID, false);
			}
		}
		else
		{
			S32 texture_length = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_TextureEntry);
			if (texture_length)
			{
				U8							tdpbuffer[1024];
				LLDataPackerBinaryBuffer	tdp(tdpbuffer, 1024);
				mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextureEntry, tdpbuffer, 0, block_num, 1024);
				S32 result = unpackTEMessage(tdp);
				if (result & teDirtyBits)
				{
					updateTEData();
				}
				if (result & TEM_CHANGE_MEDIA)
				{
					retval |= MEDIA_FLAGS_CHANGED;
				}
			}
		}
	}
	if (retval & (MEDIA_URL_REMOVED | MEDIA_URL_ADDED | MEDIA_URL_UPDATED | MEDIA_FLAGS_CHANGED))
	{
		if ( ! ( retval & (MEDIA_URL_ADDED | MEDIA_URL_UPDATED) &&
				 mMedia && ! mMedia->mMediaURL.empty() &&
				 ! LLTextureEntry::isMediaVersionString(mMedia->mMediaURL) ) )
		{
			LL_DEBUGS("MediaOnAPrim") << "Media update: " << getID() << ": retval=" << retval << " Media URL: " <<
                ((mMedia) ?  mMedia->mMediaURL : std::string("")) << LL_ENDL;
			requestMediaDataUpdate(retval & MEDIA_FLAGS_CHANGED);
		}
        else {
            LL_INFOS("MediaOnAPrim") << "Ignoring media update for: " << getID() << " Media URL: " <<
                ((mMedia) ?  mMedia->mMediaURL : std::string("")) << LL_ENDL;
        }
	}
	cleanUpMediaImpls();
	return retval;
}
void LLVOVolume::animateTextures()
{
	if (!mDead)
	{
		F32 off_s = 0.f, off_t = 0.f, scale_s = 1.f, scale_t = 1.f, rot = 0.f;
		S32 result = mTextureAnimp->animateTextures(off_s, off_t, scale_s, scale_t, rot);
		if (result)
		{
			if (!mTexAnimMode)
			{
				mFaceMappingChanged = TRUE;
				gPipeline.markTextured(mDrawable);
			}
			mTexAnimMode = result | mTextureAnimp->mMode;
			S32 start=0, end=mDrawable->getNumFaces()-1;
			if (mTextureAnimp->mFace >= 0 && mTextureAnimp->mFace <= end)
			{
				start = end = mTextureAnimp->mFace;
			}
			for (S32 i = start; i <= end; i++)
			{
				LLFace* facep = mDrawable->getFace(i);
				if (!facep) continue;
				if(facep->getVirtualSize() <= MIN_TEX_ANIM_SIZE && facep->mTextureMatrix) continue;
				const LLTextureEntry* te = facep->getTextureEntry();
				if (!te)
				{
					continue;
				}
				if (!(result & LLViewerTextureAnim::ROTATE))
				{
					te->getRotation(&rot);
				}
				if (!(result & LLViewerTextureAnim::TRANSLATE))
				{
					te->getOffset(&off_s,&off_t);
				}
				if (!(result & LLViewerTextureAnim::SCALE))
				{
					te->getScale(&scale_s, &scale_t);
				}
				if (!facep->mTextureMatrix)
				{
					facep->mTextureMatrix = new LLMatrix4a();
				}
				LLMatrix4a& tex_mat = *facep->mTextureMatrix;
				tex_mat.setIdentity();
				LLVector3 trans ;
				{
					trans.set(LLVector3(off_s+0.5f, off_t+0.5f, 0.f));
					tex_mat.setTranslate_affine(LLVector3(-0.5f, -0.5f, 0.f));
				}
				LLVector3 scale(scale_s, scale_t, 1.f);
				tex_mat.setMul(gGL.genRot(rot*RAD_TO_DEG,0.f,0.f,-1.f),tex_mat);
				LLMatrix4a scale_mat;
				scale_mat.setIdentity();
				scale_mat.applyScale_affine(scale);
				tex_mat.setMul(scale_mat, tex_mat);
				tex_mat.translate_affine(trans);
			}
		}
		else
		{
			if (mTexAnimMode && mTextureAnimp->mRate == 0)
			{
				U8 start, count;
				if (mTextureAnimp->mFace == -1)
				{
					start = 0;
					count = getNumTEs();
				}
				else
				{
					start = (U8) mTextureAnimp->mFace;
					count = 1;
				}
				for (S32 i = start; i < start + count; i++)
				{
					if (mTexAnimMode & LLViewerTextureAnim::TRANSLATE)
					{
						setTEOffset(i, mTextureAnimp->mOffS, mTextureAnimp->mOffT);
					}
					if (mTexAnimMode & LLViewerTextureAnim::SCALE)
					{
						setTEScale(i, mTextureAnimp->mScaleS, mTextureAnimp->mScaleT);
					}
					if (mTexAnimMode & LLViewerTextureAnim::ROTATE)
					{
						setTERotation(i, mTextureAnimp->mRot);
					}
				}
				gPipeline.markTextured(mDrawable);
				mFaceMappingChanged = TRUE;
				mTexAnimMode = 0;
			}
		}
	}
}
void LLVOVolume::updateTextures()
{
	const F32 TEXTURE_AREA_REFRESH_TIME = 5.f;
	if (mTextureUpdateTimer.getElapsedTimeF32() > TEXTURE_AREA_REFRESH_TIME)
	{
		updateTextureVirtualSize();
		if (mDrawable.notNull() && !isVisible() && !mDrawable->isActive())
		{
			LLSpatialGroup* group  = mDrawable->getSpatialGroup();
			if (group)
			{
				group->destroyGL(true);
				group->setState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
			}
		}
	}
}
BOOL LLVOVolume::isVisible() const
{
	if(mDrawable.notNull() && mDrawable->isVisible())
	{
		return TRUE ;
	}
	if(isAttachment())
	{
		LLViewerObject* objp = (LLViewerObject*)getParent() ;
		while(objp && !objp->isAvatar())
		{
			objp = (LLViewerObject*)objp->getParent() ;
		}
		return objp && objp->mDrawable.notNull() && objp->mDrawable->isVisible() ;
	}
	return FALSE ;
}
void LLVOVolume::updateTextureVirtualSize(bool forced)
{
	LL_RECORD_BLOCK_TIME(FTM_VOLUME_TEXTURES);
	if(mDrawable.isNull())
		return;
	if(!forced)
	{
		if(!isVisible())
		{
			const S32 num_faces = mDrawable->getNumFaces();
			for (S32 i = 0; i < num_faces; i++)
			{
				LLFace* face = mDrawable->getFace(i);
				if (face)
				{
					face->setPixelArea(0.f);
					face->setVirtualSize(0.f);
				}
			}
			return ;
		}
		if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SIMPLE))
		{
			return;
		}
	}
	static LLCachedControl<bool> dont_load_textures(gSavedSettings,"TextureDisable");
	if (dont_load_textures || LLAppViewer::getTextureFetch()->mDebugPause)
	{
		return;
	}
	mTextureUpdateTimer.reset();
	F32 old_area = mPixelArea;
	mPixelArea = 0.f;
	const S32 num_faces = mDrawable->getNumFaces();
	F32 min_vsize=999999999.f, max_vsize=0.f;
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	for (S32 i = 0; i < num_faces; i++)
	{
		LLFace* face = mDrawable->getFace(i);
		if (!face) continue;
		const LLTextureEntry *te = face->getTextureEntry();
		LLViewerTexture *imagep = face->getTexture();
		if (!imagep || !te ||
			face->mExtents[0].equals3(face->mExtents[1]))
		{
			continue;
		}
		F32 vsize;
		F32 old_size = face->getVirtualSize();
		if (isHUDAttachment())
		{
			F32 area = (F32) camera->getScreenPixelArea();
			vsize = area;
			imagep->setBoostLevel(LLGLTexture::BOOST_HUD);
 			face->setPixelArea(area);
			face->setVirtualSize(vsize);
		}
		else
		{
			vsize = face->getTextureVirtualSize();
			if (isAttachment())
			{
				if (permYouOwner())
				{
					imagep->setBoostLevel(LLGLTexture::BOOST_HIGH);
				}
			}
		}
		mPixelArea = llmax(mPixelArea, face->getPixelArea());
		if (face->mTextureMatrix != NULL)
		{
			if ((vsize < MIN_TEX_ANIM_SIZE && old_size > MIN_TEX_ANIM_SIZE) ||
			        (vsize > MIN_TEX_ANIM_SIZE && old_size < MIN_TEX_ANIM_SIZE))
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD, FALSE);
			}
		}
		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
		{
			if (vsize < min_vsize) min_vsize = vsize;
			if (vsize > max_vsize) max_vsize = vsize;
		}
		else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
		{
			LLViewerFetchedTexture* img = LLViewerTextureManager::staticCastToFetchedTexture(imagep) ;
			if(img)
			{
				F32 pri = img->getDecodePriority();
				pri = llmax(pri, 0.0f);
				if (pri < min_vsize) min_vsize = pri;
				if (pri > max_vsize) max_vsize = pri;
			}
		}
		else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_AREA))
		{
			F32 pri = mPixelArea;
			if (pri < min_vsize) min_vsize = pri;
			if (pri > max_vsize) max_vsize = pri;
		}
	}
	if (isSculpted())
	{
		updateSculptTexture();
		if (mSculptTexture.notNull())
		{
			mSculptTexture->setBoostLevel(llmax((S32)mSculptTexture->getBoostLevel(),
												(S32)LLGLTexture::BOOST_SCULPTED));
			mSculptTexture->setForSculpt() ;
			if(!mSculptTexture->isCachedRawImageReady())
			{
				S32 lod = llmin(mLOD, 3);
				F32 lodf = ((F32)(lod + 1.0f)/4.f);
				F32 tex_size = lodf * LLViewerTexture::sMaxSculptRez ;
				mSculptTexture->addTextureStats(2.f * tex_size * tex_size, FALSE);
				{
					LLVector3 lookAt = getPositionAgent() - camera->getOrigin();
					F32 dist = lookAt.normVec() ;
					F32 cos_angle_to_view_dir = lookAt * camera->getXAxis() ;
					mSculptTexture->setAdditionalDecodePriority(0.8f * LLFace::calcImportanceToCamera(cos_angle_to_view_dir, dist)) ;
				}
			}
			S32 texture_discard = mSculptTexture->getCachedRawImageLevel();
			S32 current_discard = getVolume() ? getVolume()->getSculptLevel() : -2 ;
			if (texture_discard >= 0 &&
				(texture_discard < current_discard ||
				current_discard < 0))
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
				mSculptChanged = TRUE;
			}
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SCULPTED))
			{
				setDebugText(llformat("T%d C%d V%d\n%dx%d",
										  texture_discard, current_discard, getVolume()->getSculptLevel(),
										  mSculptTexture->getHeight(), mSculptTexture->getWidth()));
			}
		}
	}
	if (getLightTextureID().notNull())
	{
		const LLLightImageParams* params = getLightImageParams();
		LLUUID id = params->getLightTexture();
		mLightTexture = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM);
		if (mLightTexture.notNull())
		{
			F32 rad = getLightRadius();
			mLightTexture->addTextureStats(gPipeline.calcPixelArea(getPositionAgent(),
																	LLVector3(rad,rad,rad),
																	*camera));
		}
	}
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", (F32) sqrt(min_vsize),(F32) sqrt(max_vsize)));
	}
 	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
 	{
 		setDebugText(llformat("%.0f:%.0f", (F32) sqrt(min_vsize),(F32) sqrt(max_vsize)));
 	}
	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", (F32) sqrt(min_vsize),(F32) sqrt(max_vsize)));
	}
	if (mPixelArea == 0)
	{
		mPixelArea = old_area;
	}
}
BOOL LLVOVolume::isActive() const
{
	return !mStatic;
}
BOOL LLVOVolume::setMaterial(const U8 material)
{
	BOOL res = LLViewerObject::setMaterial(material);
	return res;
}
void LLVOVolume::setTexture(const S32 face)
{
	llassert(face < getNumTEs());
	gGL.getTexUnit(0)->bind(getTEImage(face));
}
void LLVOVolume::setScale(const LLVector3 &scale, BOOL damped)
{
	if (scale != getScale())
	{
		LLViewerObject::setScale(scale);
		if (mVolumeImpl)
		{
			mVolumeImpl->onSetScale(scale, damped);
		}
		updateRadius();
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_POSITION, TRUE);
	}
}
LLFace* LLVOVolume::addFace(S32 f)
{
	const LLTextureEntry* te = getTE(f);
	LLViewerTexture* imagep = getTEImage(f);
	if (te->getMaterialParams().notNull())
	{
		LLViewerTexture* normalp = getTENormalMap(f);
		LLViewerTexture* specularp = getTESpecularMap(f);
		return mDrawable->addFace(te, imagep, normalp, specularp);
	}
	return mDrawable->addFace(te, imagep);
}
LLDrawable *LLVOVolume::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_VOLUME);
	S32 max_tes_to_set = getNumTEs();
	for (S32 i = 0; i < max_tes_to_set; i++)
	{
		addFace(i);
	}
	mNumFaces = max_tes_to_set;
	if (isAttachment())
	{
		mDrawable->makeActive();
	}
	if (getIsLight())
	{
		gPipeline.setLight(mDrawable, TRUE);
	}
	updateRadius();
	bool force_update = true;
	mDrawable->updateDistance(*LLViewerCamera::getInstance(), force_update);
	return mDrawable;
}
BOOL LLVOVolume::setVolume(const LLVolumeParams &params_in, const S32 detail, bool unique_volume)
{
	LLVolumeParams volume_params = params_in;
	S32 last_lod = mVolumep.notNull() ? LLVolumeLODGroup::getVolumeDetailFromScale(mVolumep->getDetail()) : -1;
	S32 lod = mLOD;
	BOOL is404 = FALSE;
	if (isSculpted())
	{
		if ((volume_params.getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
		{
			if (NO_LOD != lod)
			{
				lod = gMeshRepo.getActualMeshLOD(volume_params, lod);
				if (lod == -1)
				{
					is404 = TRUE;
					lod = 0;
				}
			}
		}
	}
	bool is_flexible = (volume_params.getPathParams().getCurveType() == LL_PCODE_PATH_FLEXIBLE);
	if (is_flexible)
	{
		setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, false);
		if (!mVolumeImpl)
		{
			LLFlexibleObjectData* data = (LLFlexibleObjectData*)getFlexibleObjectData();
			mVolumeImpl = new LLVolumeImplFlexible(this, data);
		}
	}
	else
	{
		setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, FALSE, false);
		if (mVolumeImpl)
		{
			delete mVolumeImpl;
			mVolumeImpl = NULL;
			if (mDrawable.notNull())
			{
				mDrawable->updateXform(FALSE);
			}
		}
	}
	if (is404)
	{
		setIcon(LLViewerTextureManager::getFetchedTextureFromFile("inv_item_mesh.tga", FTT_LOCAL_FILE, TRUE, LLGLTexture::BOOST_UI));
		volume_params.setSculptID(LLUUID::null, LL_SCULPT_TYPE_NONE);
	}
	if ((LLPrimitive::setVolume(volume_params, lod, (mVolumeImpl && mVolumeImpl->isVolumeUnique()))) || mSculptChanged)
	{
		mFaceMappingChanged = TRUE;
		if (mVolumeImpl)
		{
			mVolumeImpl->onSetVolume(volume_params, mLOD);
		}
		updateSculptTexture();
		if (isSculpted())
		{
			updateSculptTexture();
			if ((volume_params.getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
			{
				if (!getVolume()->isMeshAssetLoaded())
				{
					LLUUID asset_id = volume_params.getSculptID();
					S32 available_lod = gMeshRepo.loadMesh(this, volume_params, lod, last_lod);
					if (available_lod != lod)
					{
						LLPrimitive::setVolume(volume_params, available_lod);
					}
				}
			}
			else
			{
				if (mSculptTexture.notNull())
				{
					sculpt();
				}
			}
		}
		static LLCachedControl<bool> use_transform_feedback("RenderUseTransformFeedback", false);
		bool cache_in_vram = use_transform_feedback && gTransformPositionProgram.mProgramObject &&
			(!mVolumeImpl || !mVolumeImpl->isVolumeUnique());
		if (cache_in_vram)
		{
			LLVolume* volume = getVolume();
			for (S32 i = 0; i < volume->getNumFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
				if (face.mVertexBuffer.notNull())
				{
					break;
				}
				volume->genTangents(i);
				LLFace::cacheFaceInVRAM(face);
			}
		}
		return TRUE;
	}
	else if (NO_LOD == lod)
	{
		LLSculptIDSize::instance().resetSizeSum(volume_params.getSculptID());
	}
	return FALSE;
}
void LLVOVolume::updateSculptTexture()
{
	LLPointer<LLViewerFetchedTexture> old_sculpt = mSculptTexture;
	if (isSculpted() && !isMesh())
	{
		const LLSculptParams *sculpt_params = getSculptParams();
		LLUUID id =  sculpt_params->getSculptTexture();
		if (id.notNull())
		{
			mSculptTexture = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
		}
	}
	else
	{
		mSculptTexture = NULL;
	}
	if (mSculptTexture != old_sculpt)
	{
		if (old_sculpt.notNull())
		{
			old_sculpt->removeVolume(LLRender::SCULPT_TEX, this);
		}
		if (mSculptTexture.notNull())
		{
			mSculptTexture->addVolume(LLRender::SCULPT_TEX, this);
		}
	}
}
void LLVOVolume::updateVisualComplexity()
{
    LLVOAvatar* avatar = getAvatarAncestor();
    if (avatar)
    {
        avatar->updateVisualComplexity();
    }
    LLVOAvatar* rigged_avatar = getAvatar();
    if(rigged_avatar && (rigged_avatar != avatar))
    {
        rigged_avatar->updateVisualComplexity();
    }
}
void LLVOVolume::notifyMeshLoaded()
{
	mSculptChanged = TRUE;
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
    if (getAvatar() && !isAnimatedObject())
    {
        getAvatar()->addAttachmentOverridesForObject(this);
    }
    if (getControlAvatar() && isAnimatedObject())
    {
        getControlAvatar()->addAttachmentOverridesForObject(this);
    }
    updateVisualComplexity();
}
void LLVOVolume::sculpt()
{
	if (mSculptTexture.notNull())
	{
		U16 sculpt_height = 0;
		U16 sculpt_width = 0;
		S8 sculpt_components = 0;
		const U8* sculpt_data = NULL;
		S32 discard_level = mSculptTexture->getCachedRawImageLevel();
		LLImageRaw* raw_image = mSculptTexture->getCachedRawImage() ;
		S32 max_discard = mSculptTexture->getMaxDiscardLevel();
		if (discard_level > max_discard)
		{
			discard_level = max_discard;
		}
		if(discard_level > MAX_DISCARD_LEVEL)
		{
			return;
		}
		S32 current_discard = getVolume()->getSculptLevel() ;
		if(current_discard < -2)
		{
			static S32 low_sculpty_discard_warning_count = 100;
			if (++low_sculpty_discard_warning_count >= 100)
			{
				LL_WARNS() << "WARNING!!: Current discard for sculpty " << mSculptTexture->getID()
					<< " at " << current_discard
					<< " is less than -2." << LL_ENDL;
				low_sculpty_discard_warning_count = 0;
			}
			return;
		}
		else if (current_discard > MAX_DISCARD_LEVEL)
		{
			static S32 high_sculpty_discard_warning_count = 100;
			if (++high_sculpty_discard_warning_count >= 100)
			{
				LL_WARNS() << "WARNING!!: Current discard for sculpty " << mSculptTexture->getID()
					<< " at " << current_discard
					<< " is more than than allowed max of " << MAX_DISCARD_LEVEL << LL_ENDL;
				high_sculpty_discard_warning_count = 0;
			}
			return;
		}
		if (current_discard == discard_level)
			return;
		if(!raw_image)
		{
			llassert(discard_level < 0) ;
			sculpt_width = 0;
			sculpt_height = 0;
			sculpt_data = NULL ;
		}
		else
		{
			sculpt_height = raw_image->getHeight();
			sculpt_width = raw_image->getWidth();
			sculpt_components = raw_image->getComponents();
			sculpt_data = raw_image->getData();
		}
		getVolume()->sculpt(sculpt_width, sculpt_height, sculpt_components, sculpt_data, discard_level, mSculptTexture->isMissingAsset());
		for (S32 i = 0; i < mSculptTexture->getNumVolumes(LLRender::SCULPT_TEX); ++i)
		{
			LLVOVolume* volume = (*(mSculptTexture->getVolumeList(LLRender::SCULPT_TEX)))[i];
			if (volume != this && volume->getVolume() == getVolume())
			{
				gPipeline.markRebuild(volume->mDrawable, LLDrawable::REBUILD_GEOMETRY, FALSE);
			}
		}
	}
}
S32	LLVOVolume::computeLODDetail(F32 distance, F32 radius, F32 lod_factor)
{
	S32	cur_detail;
	if (LLPipeline::sDynamicLOD)
	{
		F32 tan_angle = (lod_factor*radius)/distance;
		cur_detail = LLVolumeLODGroup::getDetailFromTan(ll_round(tan_angle, 0.01f));
	}
	else
	{
		cur_detail = llclamp((S32) (sqrtf(radius)*lod_factor*4.f), 0, 3);
	}
	return cur_detail;
}
BOOL LLVOVolume::calcLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}
	S32 cur_detail = 0;
	F32 radius;
	F32 distance;
	F32 lod_factor = LLVOVolume::sLODFactor;
	if (mDrawable->isState(LLDrawable::RIGGED) && getAvatar() && getAvatar()->mDrawable)
	{
		LLVOAvatar* avatar = getAvatar();
		if( !avatar || !avatar->mDrawable )
		{
			return FALSE;
		}
		distance = avatar->mDrawable->mDistanceWRTCamera;
		if (avatar->isControlAvatar())
		{
            const LLVector3* box = avatar->getLastAnimExtents();
            LLVector3 diag = box[1] - box[0];
            radius = diag.magVec() * 0.5f;
            LL_DEBUGS("DynamicBox") << avatar->getFullname() << " diag " << diag << " radius " << radius << LL_ENDL;
        }
        else
        {
            const LLVector3* box = avatar->getLastAnimExtents();
            LLVector3 diag = box[1] - box[0];
            radius = diag.magVec();
            LL_DEBUGS("DynamicBox") << avatar->getFullname() << " diag " << diag << " radius " << radius << LL_ENDL;
        }
        if (distance <= 0.f || radius <= 0.f)
        {
            LL_DEBUGS("DynamicBox","CalcLOD") << "avatar distance/radius uninitialized, skipping" << LL_ENDL;
            return FALSE;
        }
	}
	else
	{
		distance = mDrawable->mDistanceWRTCamera;
		radius = getVolume() ? getVolume()->mLODScaleBias.scaledVec(getScale()).length() : getScale().length();
        if (distance <= 0.f || radius <= 0.f)
        {
            LL_DEBUGS("DynamicBox","CalcLOD") << "non-avatar distance/radius uninitialized, skipping" << LL_ENDL;
            return FALSE;
        }
	}
    mLODDistance = distance;
    mLODRadius = radius;
	distance *= sDistanceFactor;
	F32 rampDist = LLVOVolume::sLODFactor * 2;
	if (distance < rampDist)
	{
		distance *= distance/rampDist;
	}
	distance *= F_PI/3.f;
    mLODAdjustedDistance = distance;
    if (isHUDAttachment())
    {
        cur_detail = 3;
    }
    else
    {
        cur_detail = computeLODDetail(ll_round(distance, 0.01f), ll_round(radius, 0.01f), lod_factor);
    }
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_LOD_INFO) &&
		mDrawable->getFace(0))
	{
        setDebugText(llformat("%d", cur_detail));
	}
	if (cur_detail != mLOD)
	{
		mAppAngle = ll_round((F32) atan2( mDrawable->getRadius(), mDrawable->mDistanceWRTCamera) * RAD_TO_DEG, 0.01f);
		mLOD = cur_detail;
		return TRUE;
	}
	return FALSE;
}
BOOL LLVOVolume::updateLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}
	BOOL lod_changed = FALSE;
	if (!LLSculptIDSize::instance().isUnloaded(getVolume()->getParams().getSculptID()))
	{
		lod_changed = calcLOD();
	}
	else
	{
		return FALSE;
	}
	if (lod_changed)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
		mLODChanged = TRUE;
	}
	else
	{
		F32 new_radius = getBinRadius();
		F32 old_radius = mDrawable->getBinRadius();
		if (new_radius < old_radius * 0.9f || new_radius > old_radius*1.1f)
		{
			gPipeline.markPartitionMove(mDrawable);
		}
	}
	lod_changed = lod_changed || LLViewerObject::updateLOD();
	return lod_changed;
}
BOOL LLVOVolume::setDrawableParent(LLDrawable* parentp)
{
	if (!LLViewerObject::setDrawableParent(parentp))
	{
		return FALSE;
	}
	if (!mDrawable->isRoot())
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
		if (mDrawable->isActive() && !parentp->isActive())
		{
			parentp->makeActive();
		}
		else if (mDrawable->isStatic() && parentp->isActive())
		{
			mDrawable->makeActive();
		}
	}
	return TRUE;
}
void LLVOVolume::updateFaceFlags()
{
	for (S32 i = 0; i < getVolume()->getNumFaces() && i < mDrawable->getNumFaces(); i++)
	{
		LLFace *face = mDrawable->getFace(i);
		if (face)
		{
			LLTextureEntry *entry = getTE(i);
			if(!entry)
				continue;
			BOOL fullbright = entry->getFullbright();
			face->clearState(LLFace::FULLBRIGHT | LLFace::HUD_RENDER | LLFace::LIGHT);
			if (fullbright || (mMaterial == LL_MCODE_LIGHT))
			{
				face->setState(LLFace::FULLBRIGHT);
			}
			if (mDrawable->isLight())
			{
				face->setState(LLFace::LIGHT);
			}
			if (isHUDAttachment())
			{
				face->setState(LLFace::HUD_RENDER);
			}
		}
	}
}
BOOL LLVOVolume::setParent(LLViewerObject* parent)
{
	BOOL ret = FALSE ;
    LLViewerObject *old_parent = (LLViewerObject*) getParent();
	if (parent != old_parent)
	{
		ret = LLViewerObject::setParent(parent);
		if (ret && mDrawable)
		{
			gPipeline.markMoved(mDrawable);
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
		}
        onReparent(old_parent, parent);
	}
	return ret ;
}
void LLVOVolume::regenFaces()
{
	BOOL count_changed = mNumFaces != getNumTEs();
	if (count_changed)
	{
		deleteFaces();
		mNumFaces = getNumTEs();
	}
	for (S32 i = 0; i < mNumFaces; i++)
	{
		LLFace* facep = count_changed ? addFace(i) : mDrawable->getFace(i);
		if (!facep) continue;
		facep->setTEOffset(i);
		facep->setTexture(getTEImage(i));
		if (facep->getTextureEntry()->getMaterialParams().notNull())
		{
			facep->setNormalMap(getTENormalMap(i));
			facep->setSpecularMap(getTESpecularMap(i));
		}
		facep->setViewerObject(this);
		if((int)mMediaImplList.size() > i)
		{
			if(mMediaImplList[i])
			{
				LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[i]->getMediaTextureID()) ;
				if(media_tex)
				{
					media_tex->addMediaToFace(facep) ;
				}
			}
		}
	}
	if (!count_changed)
	{
		updateFaceFlags();
	}
}
BOOL LLVOVolume::genBBoxes(BOOL force_global)
{
	bool res = true;
	LLVector4a min,max;
	min.clear();
	max.clear();
	BOOL rebuild = mDrawable->isState(LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION | LLDrawable::REBUILD_RIGGED);
    if (getRiggedVolume())
    {
        updateRiggedVolume();
    }
	LLVolume* volume = mRiggedVolume;
	if (!volume)
	{
		volume = getVolume();
	}
    bool any_valid_boxes = false;
	static LLCachedControl<bool> sh_override_rigged_bounds("SHOverrideRiggedBounds", false);
    if (sh_override_rigged_bounds && isAttachment() && (mDrawable->isState(LLDrawable::RIGGED) || isRiggedMesh()))
    {
		LLVOAvatar* root = getAvatar();
		if (root)
		{
			any_valid_boxes = true;
			static const LLVector3 PAD_SIZE(.1f, .1f, .1f);
			LLVector3 minpos = -PAD_SIZE;
			LLVector3 maxpos = PAD_SIZE;
			min.load3(minpos.mV,1.f);
			max.load3(maxpos.mV, 1.f);
		}
    }
	if (!any_valid_boxes) {
	for (S32 i = 0;
		 i < getVolume()->getNumVolumeFaces() && i < mDrawable->getNumFaces() && i < getNumTEs();
		 i++)
	{
		LLFace *face = mDrawable->getFace(i);
		if (!face)
		{
			continue;
		}
		bool face_res = face->genVolumeBBoxes(*volume, i,
										mRelativeXform,
										(mVolumeImpl && mVolumeImpl->isVolumeGlobal()) || force_global);
		res &= face_res;
        if (!face_res)
        {
            continue;
        }
		if (rebuild)
		{
            if (getRiggedVolume())
            {
                LL_DEBUGS("RiggedBox") << "rebuilding box, face " << i << " extents " << face->mExtents[0] << ", " << face->mExtents[1] << LL_ENDL;
            }
			if (!any_valid_boxes)
			{
				min = face->mExtents[0];
				max = face->mExtents[1];
				any_valid_boxes = true;
			}
			else
			{
				min.setMin(min, face->mExtents[0]);
				max.setMax(max, face->mExtents[1]);
			}
		}
	}
	}
	if (any_valid_boxes)
	{
		if (rebuild)
		{
			if (getRiggedVolume())
			{
				LL_DEBUGS("RiggedBox") << "rebuilding got extents " << min << ", " << max << LL_ENDL;
			}
			mDrawable->setSpatialExtents(min, max);
			min.add(max);
			min.mul(0.5f);
			mDrawable->setPositionGroup(min);
		}
		updateRadius();
		mDrawable->movePartition();
	}
	else
	{
		LL_DEBUGS("RiggedBox") << "genBBoxes failed to find any valid face boxes" << LL_ENDL;
	}
	return res;
}
void LLVOVolume::preRebuild()
{
	if (mVolumeImpl != NULL)
	{
		mVolumeImpl->preRebuild();
	}
}
void LLVOVolume::updateRelativeXform(bool force_identity)
{
	if (mVolumeImpl)
	{
		mVolumeImpl->updateRelativeXform(force_identity);
		return;
	}
	LLDrawable* drawable = mDrawable;
	if (drawable->isState(LLDrawable::RIGGED) && mRiggedVolume.notNull())
	{
		mRelativeXform = getRenderMatrix();
		mRelativeXformInvTrans = mRelativeXform;
		mRelativeXform.invert();
		mRelativeXformInvTrans.transpose();
	}
	else if (drawable->isActive() || force_identity)
	{
		bool use_identity = force_identity || drawable->isSpatialRoot();
		if(use_identity)
		{
			mRelativeXform.setIdentity();
			mRelativeXform.applyScale_affine(mDrawable->getScale());
		}
		else
		{
			mRelativeXform = LLQuaternion2(mDrawable->getRotation());
			mRelativeXform.applyScale_affine(mDrawable->getScale());
			mRelativeXform.setTranslate_affine(mDrawable->getPosition());
		}
		mRelativeXformInvTrans = mRelativeXform;
		mRelativeXformInvTrans.invert();
		mRelativeXformInvTrans.transpose();
	}
	else
	{
		LLVector4a pos;
		pos.load3(getPosition().mV);
		LLQuaternion2 rot(getRotation());
		if (mParent)
		{
			LLMatrix4a lrot = LLQuaternion2(mParent->getRotation());
			lrot.rotate(pos,pos);
			LLVector4a lpos;
			lpos.load3(mParent->getPosition().mV);
			pos.add(lpos);
			rot.mul(LLQuaternion2(mParent->getRotation()));
		}
		mRelativeXform = rot;
		mRelativeXform.applyScale_affine(getScale());
		mRelativeXform.setTranslate_affine(LLVector3(pos.getF32ptr()));
		mRelativeXformInvTrans = mRelativeXform;
		mRelativeXformInvTrans.invert();
		mRelativeXformInvTrans.transpose();
	}
}
static LLTrace::BlockTimerStatHandle FTM_GEN_FLEX("Generate Flexies");
static LLTrace::BlockTimerStatHandle FTM_UPDATE_PRIMITIVES("Update Primitives");
static LLTrace::BlockTimerStatHandle FTM_UPDATE_RIGGED_VOLUME("Update Rigged");
bool LLVOVolume::lodOrSculptChanged(LLDrawable *drawable, BOOL &compiled)
{
	bool regen_faces = false;
	LLVolume *old_volumep, *new_volumep;
		F32 old_lod, new_lod;
		S32 old_num_faces, new_num_faces ;
		old_volumep = getVolume();
		old_lod = old_volumep->getDetail();
		old_num_faces = old_volumep->getNumFaces() ;
		old_volumep = NULL ;
		{
			LL_RECORD_BLOCK_TIME(FTM_GEN_VOLUME);
			const LLVolumeParams &volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
		}
		new_volumep = getVolume();
		new_lod = new_volumep->getDetail();
		new_num_faces = new_volumep->getNumFaces() ;
		new_volumep = NULL ;
		if ((new_lod != old_lod) || mSculptChanged)
		{
        if (mDrawable->isState(LLDrawable::RIGGED))
        {
            updateVisualComplexity();
        }
		compiled = TRUE;
			sNumLODChanges += new_num_faces ;
			if((S32)getNumTEs() != getVolume()->getNumFaces())
			{
				setNumTEs(getVolume()->getNumFaces());
			}
			drawable->setState(LLDrawable::REBUILD_VOLUME);
			{
				LL_RECORD_BLOCK_TIME(FTM_GEN_TRIANGLES);
				regen_faces = new_num_faces != old_num_faces || mNumFaces != (S32)getNumTEs();
				if (regen_faces)
				{
					regenFaces();
				}
				if (mSculptChanged)
				{
					LLSpatialGroup* group = mDrawable->getSpatialGroup();
					if (group)
					{
						group->unbound();
					}
				}
			}
	}
	return regen_faces;
}
BOOL LLVOVolume::updateGeometry(LLDrawable *drawable)
{
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_PRIMITIVES);
	if (mDrawable->isState(LLDrawable::REBUILD_RIGGED))
	{
		{
			LL_RECORD_BLOCK_TIME(FTM_UPDATE_RIGGED_VOLUME);
			updateRiggedVolume();
		}
		genBBoxes(FALSE);
		mDrawable->clearState(LLDrawable::REBUILD_RIGGED);
	}
	if (mVolumeImpl != NULL)
	{
		BOOL res;
		{
			LL_RECORD_BLOCK_TIME(FTM_GEN_FLEX);
			res = mVolumeImpl->doUpdateGeometry(drawable);
		}
		updateFaceFlags();
		return res;
	}
	LLSpatialGroup* group = drawable->getSpatialGroup();
	if (group)
	{
		group->dirtyMesh();
	}
	BOOL compiled = FALSE;
	updateRelativeXform();
	if (mDrawable.isNull())
	{
		return TRUE;
	}
	if (mVolumeChanged || mFaceMappingChanged)
	{
		dirtySpatialGroup(drawable->isState(LLDrawable::IN_REBUILD_Q1));
		bool was_regen_faces = false;
		if (mVolumeChanged)
		{
			was_regen_faces = lodOrSculptChanged(drawable, compiled);
			drawable->setState(LLDrawable::REBUILD_VOLUME);
		}
		else if (mSculptChanged || mLODChanged)
		{
			compiled = TRUE;
			was_regen_faces = lodOrSculptChanged(drawable, compiled);
		}
		if (!was_regen_faces) {
			LL_RECORD_BLOCK_TIME(FTM_GEN_TRIANGLES);
			regenFaces();
		}
		genBBoxes(FALSE);
	}
	else if (mLODChanged || mSculptChanged)
	{
		dirtySpatialGroup(drawable->isState(LLDrawable::IN_REBUILD_Q1));
		compiled = TRUE;
		lodOrSculptChanged(drawable, compiled);
		if(drawable->isState(LLDrawable::REBUILD_RIGGED | LLDrawable::RIGGED))
		{
			updateRiggedVolume(false);
		}
		genBBoxes(FALSE);
	}
	else
	{
		compiled = TRUE;
		LL_RECORD_BLOCK_TIME(FTM_GEN_TRIANGLES);
		genBBoxes(FALSE);
	}
	updateFaceFlags();
	if(compiled)
	{
		LLPipeline::sCompiles++;
	}
	mVolumeChanged = FALSE;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
	mFaceMappingChanged = FALSE;
	return LLViewerObject::updateGeometry(drawable);
}
void LLVOVolume::updateFaceSize(S32 idx)
{
	if( mDrawable->getNumFaces() <= idx )
	{
		return;
	}
	LLFace* facep = mDrawable->getFace(idx);
	if (facep)
	{
		if (idx >= getVolume()->getNumVolumeFaces())
		{
			facep->setSize(0,0, true);
		}
		else
		{
			const LLVolumeFace& vol_face = getVolume()->getVolumeFace(idx);
			facep->setSize(vol_face.mNumVertices, vol_face.mNumIndices,
							true);
		}
	}
}
BOOL LLVOVolume::isRootEdit() const
{
	if (mParent && !((LLViewerObject*)mParent)->isAvatar())
	{
		return FALSE;
	}
	return TRUE;
}
void LLVOVolume::setNumTEs(const U8 num_tes)
{
	const U8 old_num_tes = getNumTEs() ;
	if(old_num_tes && old_num_tes < num_tes)
	{
		LLViewerObject::setNumTEs(num_tes) ;
		if(mMediaImplList.size() >= old_num_tes && mMediaImplList[old_num_tes -1].notNull())
		{
			mMediaImplList.resize(num_tes) ;
			const LLTextureEntry* te = getTE(old_num_tes - 1) ;
			for(U8 i = old_num_tes; i < num_tes ; i++)
			{
				setTE(i, *te) ;
				mMediaImplList[i] = mMediaImplList[old_num_tes -1] ;
			}
			mMediaImplList[old_num_tes -1]->setUpdated(TRUE) ;
		}
	}
	else if(old_num_tes > num_tes && mMediaImplList.size() > num_tes)
	{
		size_t end = mMediaImplList.size() ;
		for(U8 i = num_tes; i < end ; i++)
		{
			removeMediaImpl(i) ;
		}
		mMediaImplList.resize(num_tes) ;
		LLViewerObject::setNumTEs(num_tes) ;
	}
	else
	{
		LLViewerObject::setNumTEs(num_tes) ;
	}
	return ;
}
void LLVOVolume::changeTEImage(S32 index, LLViewerTexture* imagep)
{
	BOOL changed = (mTEImages[index] != imagep);
	LLViewerObject::changeTEImage(index, imagep);
	if (changed)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
}
void LLVOVolume::setTEImage(const U8 te, LLViewerTexture *imagep)
{
	BOOL changed = (mTEImages[te] != imagep);
	LLViewerObject::setTEImage(te, imagep);
	if (changed)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
}
S32 LLVOVolume::setTETexture(const U8 te, const LLUUID &uuid)
{
	S32 res = LLViewerObject::setTETexture(te, uuid);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}
S32 LLVOVolume::setTEColor(const U8 te, const LLColor3& color)
{
	return setTEColor(te, LLColor4(color));
}
S32 LLVOVolume::setTEColor(const U8 te, const LLColor4& color)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS("MaterialTEs") << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (color != tep->getColor())
	{
		F32 old_alpha = tep->getColor().mV[3];
		if ((color.mV[3] != old_alpha) && (color.mV[3] == 1.f || old_alpha == 1.f))
		{
			gPipeline.markTextured(mDrawable);
			mLODChanged = TRUE;
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
		}
		retval = LLPrimitive::setTEColor(te, color);
		if (mDrawable.notNull() && retval)
		{
			mDrawable->setState(LLDrawable::REBUILD_COLOR);
			dirtyMesh();
		}
	}
	return  retval;
}
S32 LLVOVolume::setTEBumpmap(const U8 te, const U8 bumpmap)
{
	S32 res = LLViewerObject::setTEBumpmap(te, bumpmap);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}
S32 LLVOVolume::setTETexGen(const U8 te, const U8 texgen)
{
	S32 res = LLViewerObject::setTETexGen(te, texgen);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}
S32 LLVOVolume::setTEMediaTexGen(const U8 te, const U8 media)
{
	S32 res = LLViewerObject::setTEMediaTexGen(te, media);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}
S32 LLVOVolume::setTEShiny(const U8 te, const U8 shiny)
{
	S32 res = LLViewerObject::setTEShiny(te, shiny);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}
S32 LLVOVolume::setTEFullbright(const U8 te, const U8 fullbright)
{
	S32 res = LLViewerObject::setTEFullbright(te, fullbright);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}
S32 LLVOVolume::setTEBumpShinyFullbright(const U8 te, const U8 bump)
{
	S32 res = LLViewerObject::setTEBumpShinyFullbright(te, bump);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}
S32 LLVOVolume::setTEMediaFlags(const U8 te, const U8 media_flags)
{
	S32 res = LLViewerObject::setTEMediaFlags(te, media_flags);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}
S32 LLVOVolume::setTEGlow(const U8 te, const F32 glow)
{
	S32 res = LLViewerObject::setTEGlow(te, glow);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}
void LLVOVolume::setTEMaterialParamsCallbackTE(const LLUUID& objectID, const LLMaterialID &pMaterialID, const LLMaterialPtr pMaterialParams, U32 te)
{
	LLVOVolume* pVol = (LLVOVolume*)gObjectList.findObject(objectID);
	if (pVol)
	{
		LL_DEBUGS("MaterialTEs") << "materialid " << pMaterialID.asString() << " to TE " << te << LL_ENDL;
		if (te >= pVol->getNumTEs())
			return;
		LLTextureEntry* texture_entry = pVol->getTE(te);
		if (texture_entry && (texture_entry->getMaterialID() == pMaterialID))
		{
			pVol->setTEMaterialParams(te, pMaterialParams);
		}
	}
}
S32 LLVOVolume::setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID)
{
	S32 res = LLViewerObject::setTEMaterialID(te, pMaterialID);
	LL_DEBUGS("MaterialTEs") << "te "<< (S32)te << " materialid " << pMaterialID.asString() << " res " << res
								<< ( LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(this), te) ? " selected" : " not selected" )
								<< LL_ENDL;
	LL_DEBUGS("MaterialTEs") << " " << pMaterialID.asString() << LL_ENDL;
	if (res)
	{
		LLMaterialMgr::instance().getTE(getRegion()->getRegionID(), pMaterialID, te, boost::bind(&LLVOVolume::setTEMaterialParamsCallbackTE, getID(), _1, _2, _3));
		setChanged(ALL_CHANGED);
		if (!mDrawable.isNull())
		{
			gPipeline.markTextured(mDrawable);
			gPipeline.markRebuild(mDrawable,LLDrawable::REBUILD_ALL);
		}
		mFaceMappingChanged = TRUE;
	}
	return res;
}
bool LLVOVolume::notifyAboutCreatingTexture(LLViewerTexture *texture)
{
	std::pair<mmap_UUID_MAP_t::iterator, mmap_UUID_MAP_t::iterator> range = mWaitingTextureInfo.equal_range(texture->getID());
	typedef std::map<U8, LLMaterialPtr> map_te_material;
	map_te_material new_material;
	for(mmap_UUID_MAP_t::iterator range_it = range.first; range_it != range.second; ++range_it)
	{
		LLMaterialPtr cur_material = getTEMaterialParams(range_it->second.te);
		if(cur_material.notNull() && LLRender::DIFFUSE_MAP == range_it->second.map && GL_RGBA != texture->getPrimaryFormat())
		{
			switch(cur_material->getDiffuseAlphaMode())
			{
			case LLMaterial::DIFFUSE_ALPHA_MODE_BLEND:
			case LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE:
			case LLMaterial::DIFFUSE_ALPHA_MODE_MASK:
				{
					LLMaterialPtr mat = NULL;
					map_te_material::iterator it = new_material.find(range_it->second.te);
					if(new_material.end() == it) {
						mat = new LLMaterial(cur_material->asLLSD());
						new_material.insert(map_te_material::value_type(range_it->second.te, mat));
					} else {
						mat = it->second;
					}
					mat->setDiffuseAlphaMode(LLMaterial::DIFFUSE_ALPHA_MODE_NONE);
				} break;
			}
		}
	}
	for(map_te_material::const_iterator it = new_material.begin(), end = new_material.end(); it != end; ++it)
	{
		LLMaterialMgr::getInstance()->put(getID(), it->first, *it->second);
		LLViewerObject::setTEMaterialParams(it->first, it->second);
	}
	mWaitingTextureInfo.erase(range.first, range.second);
	return 0 != new_material.size();
}
bool LLVOVolume::notifyAboutMissingAsset(LLViewerTexture *texture)
{
	std::pair<mmap_UUID_MAP_t::iterator, mmap_UUID_MAP_t::iterator> range = mWaitingTextureInfo.equal_range(texture->getID());
	if(range.first == range.second) return false;
	typedef std::map<U8, LLMaterialPtr> map_te_material;
	map_te_material new_material;
	for(mmap_UUID_MAP_t::iterator range_it = range.first; range_it != range.second; ++range_it)
	{
		LLMaterialPtr cur_material = getTEMaterialParams(range_it->second.te);
		if (cur_material.isNull())
			continue;
		switch(range_it->second.map)
		{
		case LLRender::DIFFUSE_MAP:
			{
				if(LLMaterial::DIFFUSE_ALPHA_MODE_NONE != cur_material->getDiffuseAlphaMode())
				{
					LLMaterialPtr mat = NULL;
					map_te_material::iterator it = new_material.find(range_it->second.te);
					if(new_material.end() == it) {
						mat = new LLMaterial(cur_material->asLLSD());
						new_material.insert(map_te_material::value_type(range_it->second.te, mat));
					} else {
						mat = it->second;
					}
					mat->setDiffuseAlphaMode(LLMaterial::DIFFUSE_ALPHA_MODE_NONE);
				}
			} break;
		case LLRender::NORMAL_MAP:
			{
				LLMaterialPtr mat = NULL;
				map_te_material::iterator it = new_material.find(range_it->second.te);
				if(new_material.end() == it) {
					mat = new LLMaterial(cur_material->asLLSD());
					new_material.insert(map_te_material::value_type(range_it->second.te, mat));
				} else {
					mat = it->second;
				}
				mat->setNormalID(LLUUID::null);
			} break;
		case LLRender::SPECULAR_MAP:
			{
				LLMaterialPtr mat = NULL;
				map_te_material::iterator it = new_material.find(range_it->second.te);
				if(new_material.end() == it) {
					mat = new LLMaterial(cur_material->asLLSD());
					new_material.insert(map_te_material::value_type(range_it->second.te, mat));
				} else {
					mat = it->second;
				}
				mat->setSpecularID(LLUUID::null);
			} break;
		case LLRender::NUM_TEXTURE_CHANNELS:
			break;
		}
	}
	for(map_te_material::const_iterator it = new_material.begin(), end = new_material.end(); it != end; ++it)
	{
		LLMaterialMgr::getInstance()->setLocalMaterial(getRegion()->getRegionID(), it->second);
		LLViewerObject::setTEMaterialParams(it->first, it->second);
	}
	mWaitingTextureInfo.erase(range.first, range.second);
	return 0 != new_material.size();
}
S32 LLVOVolume::setTEMaterialParams(const U8 te, const LLMaterialPtr pMaterialParams)
{
	LLMaterialPtr pMaterial = const_cast<LLMaterialPtr&>(pMaterialParams);
	if(pMaterialParams)
	{
		LLViewerTexture *img_diffuse = getTEImage(te);
		LLViewerTexture *img_normal = getTENormalMap(te);
		LLViewerTexture *img_specular = getTESpecularMap(te);
		llassert(NULL != img_diffuse);
		LLMaterialPtr new_material = NULL;
		if(NULL != img_diffuse)
		{
			if(0 == img_diffuse->getPrimaryFormat() && !img_diffuse->isMissingAsset())
			{
				mWaitingTextureInfo.insert(mmap_UUID_MAP_t::value_type(img_diffuse->getID(), material_info(LLRender::DIFFUSE_MAP, te)));
			}
			else
			{
				bool bSetDiffuseNone = false;
				if(img_diffuse->isMissingAsset())
				{
					bSetDiffuseNone = true;
				}
				else
				{
					switch(pMaterialParams->getDiffuseAlphaMode())
					{
					case LLMaterial::DIFFUSE_ALPHA_MODE_BLEND:
					case LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE:
					case LLMaterial::DIFFUSE_ALPHA_MODE_MASK:
						{
							LLTextureEntry* tex_entry = getTE(te);
							bool bIsBakedImageId = false;
							if (tex_entry && LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(tex_entry->getID()))
							{
								bIsBakedImageId = true;
							}
							if (GL_RGBA != img_diffuse->getPrimaryFormat() && !bIsBakedImageId)
							{
								bSetDiffuseNone = true;
							}
						} break;
					}
				}
				if(bSetDiffuseNone)
				{
					new_material = new LLMaterial(pMaterialParams->asLLSD());
					new_material->setDiffuseAlphaMode(LLMaterial::DIFFUSE_ALPHA_MODE_NONE);
				}
			}
		}
		if(LLUUID::null != pMaterialParams->getNormalID())
		{
			if(img_normal && img_normal->isMissingAsset() && img_normal->getID() == pMaterialParams->getNormalID())
			{
				if(!new_material) {
					new_material = new LLMaterial(pMaterialParams->asLLSD());
				}
				new_material->setNormalID(LLUUID::null);
			}
			else if(NULL == img_normal || 0 == img_normal->getPrimaryFormat())
			{
				mWaitingTextureInfo.insert(mmap_UUID_MAP_t::value_type(pMaterialParams->getNormalID(), material_info(LLRender::NORMAL_MAP,te)));
			}
		}
		if(LLUUID::null != pMaterialParams->getSpecularID())
		{
			if(img_specular && img_specular->isMissingAsset() && img_specular->getID() == pMaterialParams->getSpecularID())
			{
				if(!new_material) {
					new_material = new LLMaterial(pMaterialParams->asLLSD());
				}
				new_material->setSpecularID(LLUUID::null);
			}
			else if(NULL == img_specular || 0 == img_specular->getPrimaryFormat())
			{
				mWaitingTextureInfo.insert(mmap_UUID_MAP_t::value_type(pMaterialParams->getSpecularID(), material_info(LLRender::SPECULAR_MAP, te)));
			}
		}
		if(new_material) {
			pMaterial = new_material;
			LLMaterialMgr::getInstance()->setLocalMaterial(getRegion()->getRegionID(), pMaterial);
		}
	}
	S32 res = LLViewerObject::setTEMaterialParams(te, pMaterial);
	LL_DEBUGS("MaterialTEs") << "te " << (S32)te << " material " << ((pMaterial) ? pMaterial->asLLSD() : LLSD("null")) << " res " << res
							 << ( LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(this), te) ? " selected" : " not selected" )
							 << LL_ENDL;
	setChanged(ALL_CHANGED);
	if (!mDrawable.isNull())
	{
		gPipeline.markTextured(mDrawable);
		gPipeline.markRebuild(mDrawable,LLDrawable::REBUILD_ALL);
	}
	mFaceMappingChanged = TRUE;
	return TEM_CHANGE_TEXTURE;
}
S32 LLVOVolume::setTEScale(const U8 te, const F32 s, const F32 t)
{
	S32 res = LLViewerObject::setTEScale(te, s, t);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}
S32 LLVOVolume::setTEScaleS(const U8 te, const F32 s)
{
	S32 res = LLViewerObject::setTEScaleS(te, s);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}
S32 LLVOVolume::setTEScaleT(const U8 te, const F32 t)
{
	S32 res = LLViewerObject::setTEScaleT(te, t);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}
void LLVOVolume::updateTEData()
{
}
bool LLVOVolume::hasMedia() const
{
	bool result = false;
	const U8 numTEs = getNumTEs();
	for (U8 i = 0; i < numTEs; i++)
	{
		const LLTextureEntry* te = getTE(i);
		if(te->hasMedia())
		{
			result = true;
			break;
		}
	}
	return result;
}
LLVector3 LLVOVolume::getApproximateFaceNormal(U8 face_id)
{
	LLVolume* volume = getVolume();
	LLVector4a result;
	result.clear();
	LLVector3 ret;
	if (volume && face_id < volume->getNumVolumeFaces())
	{
		const LLVolumeFace& face = volume->getVolumeFace(face_id);
		for (S32 i = 0; i < (S32)face.mNumVertices; ++i)
		{
			result.add(face.mNormals[i]);
		}
		ret = LLVector3(result.getF32ptr());
		ret = volumeDirectionToAgent(ret);
		ret.normVec();
	}
	return ret;
}
void LLVOVolume::requestMediaDataUpdate(bool isNew)
{
    if (sObjectMediaClient)
		sObjectMediaClient->fetchMedia(new LLMediaDataClientObjectImpl(this, isNew));
}
bool LLVOVolume::isMediaDataBeingFetched() const
{
	return (sObjectMediaClient) ? sObjectMediaClient->isInQueue(new LLMediaDataClientObjectImpl(const_cast<LLVOVolume*>(this), false)) : false;
}
void LLVOVolume::cleanUpMediaImpls()
{
	const U8 numTEs = getNumTEs();
	for (U8 i = 0; i < numTEs; i++)
	{
		const LLTextureEntry* te = getTE(i);
		if( ! te->hasMedia())
		{
			removeMediaImpl(i) ;
		}
	}
}
void LLVOVolume::updateObjectMediaData(const LLSD &media_data_array, const std::string &media_version)
{
	U32 fetched_version = LLTextureEntry::getVersionFromMediaVersionString(media_version);
	if ( (S32)fetched_version > mLastFetchedMediaVersion)
	{
		mLastFetchedMediaVersion = fetched_version;
		U8 texture_index = 0;
		for (auto const& entry : media_data_array.array())
		{
			syncMediaData(texture_index++, entry, false, false);
		}
	}
}
void LLVOVolume::syncMediaData(S32 texture_index, const LLSD &media_data, bool merge, bool ignore_agent)
{
	if(mDead)
	{
		return;
	}
	LLTextureEntry *te = getTE(texture_index);
	if(!te)
	{
		return ;
	}
	LL_DEBUGS("MediaOnAPrim") << "BEFORE: texture_index = " << texture_index
		<< " hasMedia = " << te->hasMedia() << " : "
		<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << LL_ENDL;
	std::string previous_url;
	LLMediaEntry* mep = te->getMediaData();
	if(mep)
	{
		previous_url = mep->getCurrentURL();
	}
	if (merge)
	{
		te->mergeIntoMediaData(media_data);
	}
	else {
		te->updateMediaData(media_data);
	}
	mep = te->getMediaData();
	if(mep)
	{
		bool update_from_self = false;
		if (!ignore_agent)
		{
			LLUUID updating_agent = LLTextureEntry::getAgentIDFromMediaVersionString(getMediaURL());
			update_from_self = (updating_agent == gAgent.getID());
		}
		viewer_media_t media_impl = LLViewerMedia::updateMediaImpl(mep, previous_url, update_from_self);
		addMediaImpl(media_impl, texture_index) ;
	}
	else
	{
		removeMediaImpl(texture_index);
	}
	LL_DEBUGS("MediaOnAPrim") << "AFTER: texture_index = " << texture_index
		<< " hasMedia = " << te->hasMedia() << " : "
		<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << LL_ENDL;
}
void LLVOVolume::mediaNavigateBounceBack(U8 texture_index)
{
	const LLMediaEntry* mep = NULL;
	viewer_media_t impl = getMediaImpl(texture_index);
	LLTextureEntry *te = getTE(texture_index);
	if(te)
	{
		mep = te->getMediaData();
	}
	if (mep && impl)
	{
        std::string url = mep->getCurrentURL();
		if (!url.empty() && std::string::npos == url.find(':'))
		{
			url = "http://" + url;
		}
        if (url.empty() || !mep->checkCandidateUrl(url))
        {
            url = mep->getHomeURL();
			if (!url.empty() && std::string::npos == url.find(':'))
			{
				url = "http://" + url;
			}
        }
        if (url.empty() || !mep->checkCandidateUrl(url))
		{
			LL_WARNS("MediaOnAPrim") << "FAILED to bounce back URL \"" << url << "\" -- unloading impl" << LL_ENDL;
			impl->setMediaFailed(true);
		}
		else if (impl->getCurrentMediaURL() != url)
		{
            LL_INFOS("MediaOnAPrim") << "bouncing back to URL: " << url << LL_ENDL;
            impl->navigateTo(url, "", false, true);
        }
    }
}
bool LLVOVolume::hasMediaPermission(const LLMediaEntry* media_entry, MediaPermType perm_type)
{
    if (NULL == media_entry ) return false;
    U8 media_perms = (perm_type == MEDIA_PERM_INTERACT) ? media_entry->getPermsInteract() : media_entry->getPermsControl();
    if (0 != (media_perms & LLMediaEntry::PERM_ANYONE))
    {
        return true;
    }
    else if (0 != (media_perms & LLMediaEntry::PERM_GROUP))
    {
		LLPermissions* obj_perm = LLSelectMgr::getInstance()->findObjectPermissions(this);
		if (obj_perm && gAgent.isInGroup(obj_perm->getGroup()))
		{
			return true;
		}
    }
    else if (0 != (media_perms & LLMediaEntry::PERM_OWNER) && permYouOwner())
    {
        return true;
    }
    return false;
}
void LLVOVolume::mediaNavigated(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, std::string new_location)
{
	bool block_navigation = false;
	int face_index = getFaceIndexWithMediaImpl(impl, -1);
	LLMediaEntry* mep = NULL;
	LLTextureEntry *te = getTE(face_index);
	if(te)
	{
		mep = te->getMediaData();
	}
	if(mep)
	{
		if(!mep->checkCandidateUrl(new_location))
		{
			block_navigation = true;
		}
		if (!block_navigation && !hasMediaPermission(mep, MEDIA_PERM_INTERACT))
		{
			block_navigation = true;
		}
	}
	else
	{
		LL_WARNS("MediaOnAPrim") << "Couldn't find media entry!" << LL_ENDL;
	}
	if(block_navigation)
	{
		LL_INFOS("MediaOnAPrim") << "blocking navigate to URI " << new_location << LL_ENDL;
		mediaNavigateBounceBack(face_index);
	}
	else if (sObjectMediaNavigateClient)
	{
		LL_DEBUGS("MediaOnAPrim") << "broadcasting navigate with URI " << new_location << LL_ENDL;
		sObjectMediaNavigateClient->navigate(new LLMediaDataClientObjectImpl(this, false), face_index, new_location);
	}
}
void LLVOVolume::mediaEvent(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event)
{
	switch(event)
	{
		case LLViewerMediaObserver::MEDIA_EVENT_LOCATION_CHANGED:
		{
			switch(impl->getNavState())
			{
				case LLViewerMediaImpl::MEDIANAVSTATE_FIRST_LOCATION_CHANGED:
				{
					mediaNavigated(impl, plugin, plugin->getLocation());
				}
				break;
				case LLViewerMediaImpl::MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS:
					LL_DEBUGS("MediaOnAPrim") << "	NOT broadcasting navigate (spurious)" << LL_ENDL;
				break;
				case LLViewerMediaImpl::MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED:
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (server-directed)" << LL_ENDL;
				break;
				default:
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (redirect)" << LL_ENDL;
				break;
			}
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			switch(impl->getNavState())
			{
				case LLViewerMediaImpl::MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED:
				{
					mediaNavigated(impl, plugin, plugin->getNavigateURI());
				}
				break;
				case LLViewerMediaImpl::MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS:
					LL_DEBUGS("MediaOnAPrim") << "	NOT broadcasting navigate (spurious)" << LL_ENDL;
				break;
				case LLViewerMediaImpl::MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED:
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (server-directed)" << LL_ENDL;
				break;
				default:
				break;
			}
		}
		break;
		default:
		break;
	}
}
void LLVOVolume::sendMediaDataUpdate()
{
    if (sObjectMediaClient)
		sObjectMediaClient->updateMedia(new LLMediaDataClientObjectImpl(this, false));
}
void LLVOVolume::removeMediaImpl(S32 texture_index)
{
	if(mMediaImplList.size() <= (U32)texture_index || mMediaImplList[texture_index].isNull())
	{
		return ;
	}
	if(mDrawable && texture_index < mDrawable->getNumFaces())
	{
		LLFace* facep = mDrawable->getFace(texture_index) ;
		if(facep)
		{
			LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[texture_index]->getMediaTextureID()) ;
			if(media_tex)
			{
				media_tex->removeMediaFromFace(facep) ;
			}
		}
	}
	S32 i ;
	S32 end = (S32)mMediaImplList.size() ;
	for(i = 0; i < end ; i++)
	{
		if( i != texture_index && mMediaImplList[i] == mMediaImplList[texture_index])
		{
			break ;
		}
	}
	if(i == end)
	{
		mMediaImplList[texture_index]->removeObject(this) ;
	}
	mMediaImplList[texture_index] = NULL ;
	return ;
}
void LLVOVolume::addMediaImpl(LLViewerMediaImpl* media_impl, S32 texture_index)
{
	if((S32)mMediaImplList.size() < texture_index + 1)
	{
		mMediaImplList.resize(texture_index + 1) ;
	}
	if(mMediaImplList[texture_index].notNull())
	{
		if(mMediaImplList[texture_index] == media_impl)
		{
			return ;
		}
		removeMediaImpl(texture_index) ;
	}
	mMediaImplList[texture_index] = media_impl;
	media_impl->addObject(this) ;
	if(mDrawable)
	{
		LLFace* facep(NULL);
		if( texture_index < mDrawable->getNumFaces() )
		{
			facep = mDrawable->getFace(texture_index) ;
		}
		if(facep)
		{
			LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[texture_index]->getMediaTextureID()) ;
			if(media_tex)
			{
				media_tex->addMediaToFace(facep) ;
			}
		}
		else
		{
			media_impl->setUpdated(TRUE) ;
		}
	}
	return ;
}
viewer_media_t LLVOVolume::getMediaImpl(U8 face_id) const
{
	if(mMediaImplList.size() > face_id)
	{
		return mMediaImplList[face_id];
	}
	return NULL;
}
F64 LLVOVolume::getTotalMediaInterest() const
{
	if (LLViewerMediaFocus::getInstance()->getFocusedObjectID() == getID())
		return F64_MAX;
	F64 interest = (F64)-1.0;
	if (LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(this)))
		interest = F64_MAX / 2.0;
	int i = 0;
	const int end = getNumTEs();
	for ( ; i < end; ++i)
	{
		const viewer_media_t &impl = getMediaImpl(i);
		if (!impl.isNull())
		{
			if (interest == (F64)-1.0) interest = (F64)0.0;
			interest += impl->getInterest();
		}
	}
	return interest;
}
S32 LLVOVolume::getFaceIndexWithMediaImpl(const LLViewerMediaImpl* media_impl, S32 start_face_id)
{
	S32 end = (S32)mMediaImplList.size() ;
	for(S32 face_id = start_face_id + 1; face_id < end; face_id++)
	{
		if(mMediaImplList[face_id] == media_impl)
		{
			return face_id ;
		}
	}
	return -1 ;
}
void LLVOVolume::setLightTextureID(LLUUID id)
{
	LLViewerTexture* old_texturep = getLightTexture();
	if (id.notNull())
	{
		if (!hasLightTexture())
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, TRUE, true);
		}
		else if (old_texturep)
		{
			old_texturep->removeVolume(LLRender::LIGHT_TEX, this);
		}
		LLLightImageParams* param_block = (LLLightImageParams*)getLightImageParams();
		if (param_block && param_block->getLightTexture() != id)
		{
			param_block->setLightTexture(id);
			parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
		}
		LLViewerTexture* tex = getLightTexture();
		if (tex)
		{
			tex->addVolume(LLRender::LIGHT_TEX, this);
		}
		else
		{
			LL_WARNS() << "Can't get light texture for ID " << id.asString() << LL_ENDL;
		}
	}
	else if (hasLightTexture())
	{
		if (old_texturep)
		{
			old_texturep->removeVolume(LLRender::LIGHT_TEX, this);
		}
		setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, FALSE, true);
		parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
		mLightTexture = NULL;
	}
}
void LLVOVolume::setSpotLightParams(LLVector3 params)
{
	LLLightImageParams* param_block = (LLLightImageParams*)getLightImageParams();
	if (param_block && param_block->getParams() != params)
	{
		param_block->setParams(params);
		parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
	}
}
void LLVOVolume::setIsLight(BOOL is_light)
{
	BOOL was_light = getIsLight();
	if (is_light != was_light)
	{
		if (is_light)
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, TRUE, true);
		}
		else
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, FALSE, true);
		}
		if (is_light)
		{
			gPipeline.setLight(mDrawable, TRUE);
		}
		else
		{
			gPipeline.setLight(mDrawable, FALSE);
		}
	}
}
void LLVOVolume::setLightColor(const LLColor3& color)
{
	LLLightParams *param_block = (LLLightParams *)getLightParams();
	if (param_block)
	{
		if (param_block->getColor() != color)
		{
			param_block->setColor(LLColor4(color, param_block->getColor().mV[3]));
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
			gPipeline.markTextured(mDrawable);
			mFaceMappingChanged = TRUE;
		}
	}
}
void LLVOVolume::setLightIntensity(F32 intensity)
{
	LLLightParams *param_block = (LLLightParams *)getLightParams();
	if (param_block)
	{
		if (param_block->getColor().mV[3] != intensity)
		{
			param_block->setColor(LLColor4(LLColor3(param_block->getColor()), intensity));
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}
void LLVOVolume::setLightRadius(F32 radius)
{
	LLLightParams *param_block = (LLLightParams *)getLightParams();
	if (param_block)
	{
		if (param_block->getRadius() != radius)
		{
			param_block->setRadius(radius);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}
void LLVOVolume::setLightFalloff(F32 falloff)
{
	LLLightParams *param_block = (LLLightParams *)getLightParams();
	if (param_block)
	{
		if (param_block->getFalloff() != falloff)
		{
			param_block->setFalloff(falloff);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}
void LLVOVolume::setLightCutoff(F32 cutoff)
{
	LLLightParams *param_block = (LLLightParams *)getLightParams();
	if (param_block)
	{
		if (param_block->getCutoff() != cutoff)
		{
			param_block->setCutoff(cutoff);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}
BOOL LLVOVolume::getIsLight() const
{
	return getLightParams() != nullptr;
}
LLColor3 LLVOVolume::getLightBaseColor() const
{
	const LLLightParams *param_block = getLightParams();
	if (param_block)
	{
		return LLColor3(param_block->getColor());
	}
	else
	{
		return LLColor3(1,1,1);
	}
}
LLColor3 LLVOVolume::getLightColor() const
{
	const LLLightParams *param_block = getLightParams();
	if (param_block)
	{
		return LLColor3(param_block->getColor()) * param_block->getColor().mV[3];
	}
	else
	{
		return LLColor3(1,1,1);
	}
}
LLUUID LLVOVolume::getLightTextureID() const
{
	const LLLightImageParams *param_block = getLightImageParams();
	if (param_block)
	{
		return param_block->getLightTexture();
	}
	return LLUUID::null;
}
LLVector3 LLVOVolume::getSpotLightParams() const
{
	const LLLightImageParams *param_block = getLightImageParams();
	if (param_block)
	{
		return param_block->getParams();
	}
	return LLVector3();
}
F32 LLVOVolume::getSpotLightPriority() const
{
	return mSpotLightPriority;
}
void LLVOVolume::updateSpotLightPriority()
{
	LLVector3 pos = mDrawable->getPositionAgent();
	LLVector3 at(0,0,-1);
	at *= getRenderRotation();
	F32 r = getLightRadius()*0.5f;
	pos += at * r;
	at = LLViewerCamera::getInstance()->getAtAxis();
	pos -= at * r;
	mSpotLightPriority = gPipeline.calcPixelArea(pos, LLVector3(r,r,r), *LLViewerCamera::getInstance());
	if (mLightTexture.notNull())
	{
		mLightTexture->addTextureStats(mSpotLightPriority);
		mLightTexture->setBoostLevel(LLGLTexture::BOOST_CLOUDS);
	}
}
bool LLVOVolume::isLightSpotlight() const
{
	const LLLightImageParams* params = getLightImageParams();
	if (params)
	{
		return params->isLightSpotlight();
	}
	return false;
}
LLViewerTexture* LLVOVolume::getLightTexture()
{
	LLUUID id = getLightTextureID();
	if (id.notNull())
	{
		if (mLightTexture.isNull() || id != mLightTexture->getID())
		{
			mLightTexture = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM);
		}
	}
	else
	{
		mLightTexture = NULL;
	}
	return mLightTexture;
}
F32 LLVOVolume::getLightIntensity() const
{
	const LLLightParams *param_block = getLightParams();
	if (param_block)
	{
		return param_block->getColor().mV[3];
	}
	else
	{
		return 1.f;
	}
}
F32 LLVOVolume::getLightRadius() const
{
	const LLLightParams *param_block = getLightParams();
	if (param_block)
	{
		return param_block->getRadius();
	}
	else
	{
		return 0.f;
	}
}
F32 LLVOVolume::getLightFalloff() const
{
	const LLLightParams *param_block = getLightParams();
	if (param_block)
	{
		return param_block->getFalloff();
	}
	else
	{
		return 0.f;
	}
}
F32 LLVOVolume::getLightCutoff() const
{
	const LLLightParams *param_block = getLightParams();
	if (param_block)
	{
		return param_block->getCutoff();
	}
	else
	{
		return 0.f;
	}
}
U32 LLVOVolume::getVolumeInterfaceID() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getID();
	}
	return 0;
}
BOOL LLVOVolume::isFlexible() const
{
	if (getFlexibleObjectData())
	{
		LLVolume* volume = getVolume();
		if (volume && volume->getParams().getPathParams().getCurveType() != LL_PCODE_PATH_FLEXIBLE)
		{
			LLVolumeParams volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_FLEXIBLE);
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
BOOL LLVOVolume::isSculpted() const
{
	if (getSculptParams())
	{
		return TRUE;
	}
	return FALSE;
}
BOOL LLVOVolume::isMesh() const
{
	if (isSculpted())
	{
		const LLSculptParams *sculpt_params = getSculptParams();
		U8 sculpt_type = sculpt_params->getSculptType();
		if ((sculpt_type & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL LLVOVolume::hasLightTexture() const
{
	if (getLightImageParams())
	{
		return TRUE;
	}
	return FALSE;
}
BOOL LLVOVolume::isVolumeGlobal() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->isVolumeGlobal() ? TRUE : FALSE;
	}
	else if (mRiggedVolume.notNull())
	{
		return TRUE;
	}
	return FALSE;
}
BOOL LLVOVolume::canBeFlexible() const
{
	U8 path = getVolume()->getParams().getPathParams().getCurveType();
	return (path == LL_PCODE_PATH_FLEXIBLE || path == LL_PCODE_PATH_LINE);
}
BOOL LLVOVolume::setIsFlexible(BOOL is_flexible)
{
	BOOL res = FALSE;
	BOOL was_flexible = isFlexible();
	LLVolumeParams volume_params;
	if (is_flexible)
	{
		if (!was_flexible)
		{
			volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_FLEXIBLE);
			res = TRUE;
			setFlags(FLAGS_USE_PHYSICS, FALSE);
			setFlags(FLAGS_PHANTOM, TRUE);
			setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, true);
			if (mDrawable)
			{
				mDrawable->makeActive();
			}
		}
	}
	else
	{
		if (was_flexible)
		{
			volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_LINE);
			res = TRUE;
			setFlags(FLAGS_PHANTOM, FALSE);
			setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, FALSE, true);
		}
	}
	if (res)
	{
		res = setVolume(volume_params, 1);
		if (res)
		{
			markForUpdate(TRUE);
		}
	}
	return res;
}
const LLMeshSkinInfo* LLVOVolume::getSkinInfo() const
{
    if (getVolume())
    {
        return gMeshRepo.getSkinInfo(getVolume()->getParams().getSculptID(), this);
    }
    else
    {
        return NULL;
    }
}
BOOL LLVOVolume::isRiggedMesh() const
{
    return isMesh() && getSkinInfo();
}
U32 LLVOVolume::getExtendedMeshFlags() const
{
	const LLExtendedMeshParams *param_block = getExtendedMeshParams();
	if (param_block)
	{
		return param_block->getFlags();
	}
	else
	{
		return 0;
	}
}
void LLVOVolume::onSetExtendedMeshFlags(U32 flags)
{
	if ( mDrawable.notNull())
    {
        getRootEdit()->recursiveMarkForUpdate(TRUE);
    }
    if (isAttachment() && getAvatarAncestor())
    {
        updateVisualComplexity();
        if (flags & LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG)
        {
            getAvatarAncestor()->updateAttachmentOverrides();
        }
        else
        {
            getAvatarAncestor()->updateAttachmentOverrides();
        }
    }
}
void LLVOVolume::setExtendedMeshFlags(U32 flags)
{
    U32 curr_flags = getExtendedMeshFlags();
    if (curr_flags != flags)
    {
        bool in_use = true;
        setParameterEntryInUse(LLNetworkData::PARAMS_EXTENDED_MESH, in_use, true);
		LLExtendedMeshParams *param_block = (LLExtendedMeshParams *)getExtendedMeshParams();
        if (param_block)
        {
            param_block->setFlags(flags);
        }
        parameterChanged(LLNetworkData::PARAMS_EXTENDED_MESH, true);
        LL_DEBUGS("AnimatedObjects") << this
                                     << " new flags " << flags << " curr_flags " << curr_flags
                                     << ", calling onSetExtendedMeshFlags()"
                                     << LL_ENDL;
        onSetExtendedMeshFlags(flags);
    }
}
bool LLVOVolume::canBeAnimatedObject() const
{
    F32 est_tris = recursiveGetEstTrianglesMax();
    if (est_tris < 0 || est_tris > getAnimatedObjectMaxTris())
    {
        return false;
    }
    return true;
}
bool LLVOVolume::isAnimatedObject() const
{
    LLVOVolume *root_vol = (LLVOVolume*)getRootEdit();
    bool root_is_animated_flag = root_vol->getExtendedMeshFlags() & LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG;
    return root_is_animated_flag;
}
void LLVOVolume::onReparent(LLViewerObject *old_parent, LLViewerObject *new_parent)
{
    if (new_parent && !new_parent->isAvatar())
    {
        if (mControlAvatar.notNull())
        {
            LLControlAvatar *av = mControlAvatar;
            mControlAvatar = NULL;
            av->markForDeath();
        }
    }
	LLVOVolume *old_volp = old_parent ? old_parent->asVolume() : nullptr;
    if (old_volp && old_volp->isAnimatedObject())
    {
        if (old_volp->getControlAvatar())
        {
            old_volp->getControlAvatar()->updateAttachmentOverrides();
            old_volp->getControlAvatar()->updateAnimations();
        }
    }
}
void LLVOVolume::afterReparent()
{
    {
        LL_DEBUGS("AnimatedObjects") << "new child added for parent "
            << ((LLViewerObject*)getParent())->getID() << LL_ENDL;
    }
    if (isAnimatedObject() && getControlAvatar())
    {
        LL_DEBUGS("AnimatedObjects") << "adding attachment overrides, parent is animated object "
            << ((LLViewerObject*)getParent())->getID() << LL_ENDL;
        getControlAvatar()->updateAnimations();
    }
    else
    {
        LL_DEBUGS("AnimatedObjects") << "not adding overrides, parent: "
                                     << ((LLViewerObject*)getParent())->getID()
                                     << " isAnimated: "  << isAnimatedObject() << " cav "
                                     << getControlAvatar() << LL_ENDL;
    }
}
static LLTrace::BlockTimerStatHandle FTM_VOVOL_RIGGING_INFO("VOVol Rigging Info");
void LLVOVolume::updateRiggingInfo()
{
    LL_RECORD_BLOCK_TIME(FTM_VOVOL_RIGGING_INFO);
    if (isRiggedMesh())
    {
        const LLMeshSkinInfo* skin = getSkinInfo();
        LLVOAvatar *avatar = getAvatar();
        LLVolume *volume = getVolume();
        if (skin && avatar && volume)
        {
            LL_DEBUGS("RigSpammish") << "starting, vovol " << this << " lod " << getLOD() << " last " << mLastRiggingInfoLOD << LL_ENDL;
            if (getLOD()>mLastRiggingInfoLOD || getLOD()==3)
            {
                mJointRiggingInfoTab.clear();
                for (S32 f = 0; f < volume->getNumVolumeFaces(); ++f)
                {
                    LLVolumeFace& vol_face = volume->getVolumeFace(f);
                    LLSkinningUtil::updateRiggingInfo(skin, avatar, vol_face);
                    if (vol_face.mJointRiggingInfoTab.size()>0)
                    {
                        mJointRiggingInfoTab.merge(vol_face.mJointRiggingInfoTab);
                    }
                }
                mLastRiggingInfoLOD = getLOD();
                LL_DEBUGS("RigSpammish") << "updated rigging info for LLVOVolume "
                                         << this << " lod " << mLastRiggingInfoLOD
                                         << LL_ENDL;
            }
        }
    }
}
void LLVOVolume::generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point)
{
	LLVolume *volume = getVolume();
	if (volume)
	{
		LLVector3 view_vector;
		view_vector = view_point;
		view_vector -= getRenderPosition();
		LLQuaternion worldRot = getRenderRotation();
		view_vector = view_vector * ~worldRot;
		if (!isVolumeGlobal())
		{
			LLVector3 objScale = getScale();
			LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
			view_vector.scaleVec(invObjScale);
		}
		updateRelativeXform();
		LLMatrix4a trans_mat = mRelativeXform;
		if (mDrawable->isStatic())
		{
			trans_mat.translate_affine(getRegion()->getOriginAgent());
		}
		volume->generateSilhouetteVertices(nodep->mSilhouetteVertices, nodep->mSilhouetteNormals, view_vector, trans_mat, mRelativeXformInvTrans, nodep->getTESelectMask());
		nodep->mSilhouetteExists = TRUE;
	}
}
void LLVOVolume::deleteFaces()
{
	S32 face_count = mNumFaces;
	if (mDrawable.notNull())
	{
		mDrawable->deleteFaces(0, face_count);
	}
	mNumFaces = 0;
}
void LLVOVolume::updateRadius()
{
	if (mDrawable.isNull())
	{
		return;
	}
	mVObjRadius = getScale().length();
	mDrawable->setRadius(mVObjRadius);
}
BOOL LLVOVolume::isAttachment() const
{
	return mAttachmentState != 0 ;
}
BOOL LLVOVolume::isHUDAttachment() const
{
	S32 attachment_id = ATTACHMENT_ID_FROM_STATE(mAttachmentState);
	return ( attachment_id >= 31 && attachment_id <= 38 );
}
const LLMatrix4a& LLVOVolume::getRenderMatrix() const
{
	if (mDrawable->isActive() && !mDrawable->isRoot())
	{
		return mDrawable->getParent()->getWorldMatrix();
	}
	return mDrawable->getWorldMatrix();
}
U32 LLVOVolume::getRenderCost(texture_cost_t &textures) const
{
	BOOL has_volume = (getVolume() != NULL);
	LLVolumeParams volume_params;
	LLPathParams path_params;
	LLProfileParams profile_params;
	U32 num_triangles = 0;
	static const U32 ARC_PARTICLE_COST = 1;
	static const U32 ARC_PARTICLE_MAX = 2048;
	static const U32 ARC_TEXTURE_COST = 16;
	static const U32 ARC_LIGHT_COST = 500;
	static const U32 ARC_MEDIA_FACE_COST = 1500;
	static const F32 ARC_GLOW_MULT = 1.5f;
	static const F32 ARC_BUMP_MULT = 1.25f;
	static const F32 ARC_FLEXI_MULT = 5;
	static const F32 ARC_SHINY_MULT = 1.6f;
	static const F32 ARC_INVISI_COST = 1.2f;
	static const F32 ARC_WEIGHTED_MESH = 1.2f;
	static const F32 ARC_PLANAR_COST = 1.0f;
	static const F32 ARC_ANIM_TEX_COST = 4.f;
	static const F32 ARC_ALPHA_COST = 4.f;
	F32 shame = 0;
	U32 invisi = 0;
	U32 shiny = 0;
	U32 glow = 0;
	U32 alpha = 0;
	U32 flexi = 0;
	U32 animtex = 0;
	U32 particles = 0;
	U32 bump = 0;
	U32 planar = 0;
	U32 weighted_mesh = 0;
	U32 produces_light = 0;
	U32 media_faces = 0;
	const LLDrawable* drawablep = mDrawable;
	U32 num_faces = drawablep->getNumFaces();
	if (has_volume)
	{
		volume_params = getVolume()->getParams();
		path_params = volume_params.getPathParams();
		profile_params = volume_params.getProfileParams();
        LLMeshCostData costs;
		if (getCostData(costs))
		{
            if (isAnimatedObject() && isRiggedMesh())
            {
                num_triangles = (ANIMATED_OBJECT_COST_PER_KTRI * 0.001 * costs.getEstTrisForStreamingCost())/0.06;
            }
            else
            {
                F32 radius = getScale().length()*0.5f;
                num_triangles = costs.getRadiusWeightedTris(radius);
            }
		}
	}
	if (num_triangles <= 0)
	{
		num_triangles = 4;
	}
	if (isSculpted())
	{
		if (isMesh())
		{
			S32 size = gMeshRepo.getMeshSize(volume_params.getSculptID(), getLOD());
			if ( size > 0)
			{
				if (isRiggedMesh())
				{
					weighted_mesh = 1;
				}
			}
			else
			{
				return 0;
			}
		}
		else
		{
			const LLSculptParams *sculpt_params = getSculptParams();
			LLUUID sculpt_id = sculpt_params->getSculptTexture();
			if (textures.find(sculpt_id) == textures.end())
			{
				LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(sculpt_id);
				if (texture)
				{
					S32 texture_cost = 256 + (S32)(ARC_TEXTURE_COST * (texture->getFullHeight() / 128.f + texture->getFullWidth() / 128.f));
					textures.insert(texture_cost_t::value_type(sculpt_id, texture_cost));
				}
			}
		}
	}
	if (isFlexible())
	{
		flexi = 1;
	}
	if (isParticleSource())
	{
		particles = 1;
	}
	if (getIsLight())
	{
		produces_light = 1;
	}
	for (S32 i = 0; i < (S32)num_faces; ++i)
	{
		const LLFace* face = drawablep->getFace(i);
		if (!face) continue;
		const LLTextureEntry* te = face->getTextureEntry();
		const LLViewerTexture* img = face->getTexture();
		if (img)
		{
			if (textures.find(img->getID()) == textures.end())
			{
				S32 texture_cost = 256 + (S32)(ARC_TEXTURE_COST * (img->getFullHeight() / 128.f + img->getFullWidth() / 128.f));
				textures.insert(texture_cost_t::value_type(img->getID(), texture_cost));
			}
		}
		if (face->getPoolType() == LLDrawPool::POOL_ALPHA)
		{
			alpha = 1;
		}
		else if (img && img->getPrimaryFormat() == GL_ALPHA)
		{
			invisi = 1;
		}
		if (face->hasMedia())
		{
			media_faces++;
		}
		if (te)
		{
			if (te->getBumpmap())
			{
				bump = 1;
			}
			if (te->getShiny())
			{
				shiny = 1;
			}
			if (te->getGlow() > 0.f)
			{
				glow = 1;
			}
			if (face->mTextureMatrix != NULL)
			{
				animtex = 1;
			}
			if (te->getTexGen())
			{
				planar = 1;
			}
		}
	}
	shame = num_triangles  * 5.f;
	shame = shame < 2.f ? 2.f : shame;
	if (planar)
	{
		shame *= planar * ARC_PLANAR_COST;
	}
	if (animtex)
	{
		shame *= animtex * ARC_ANIM_TEX_COST;
	}
	if (alpha)
	{
		shame *= alpha * ARC_ALPHA_COST;
	}
	if(invisi)
	{
		shame *= invisi * ARC_INVISI_COST;
	}
	if (glow)
	{
		shame *= glow * ARC_GLOW_MULT;
	}
	if (bump)
	{
		shame *= bump * ARC_BUMP_MULT;
	}
	if (shiny)
	{
		shame *= shiny * ARC_SHINY_MULT;
	}
	if (weighted_mesh)
	{
		shame *= weighted_mesh * ARC_WEIGHTED_MESH;
	}
	if (flexi)
	{
		shame *= flexi * ARC_FLEXI_MULT;
	}
	if (particles)
	{
		const LLPartSysData *part_sys_data = &(mPartSourcep->mPartSysData);
		const LLPartData *part_data = &(part_sys_data->mPartData);
		U32 num_particles = (U32)(part_sys_data->mBurstPartCount * llceil( part_data->mMaxAge / part_sys_data->mBurstRate));
		num_particles = num_particles > ARC_PARTICLE_MAX ? ARC_PARTICLE_MAX : num_particles;
		F32 part_size = (llmax(part_data->mStartScale[0], part_data->mEndScale[0]) + llmax(part_data->mStartScale[1], part_data->mEndScale[1])) / 2.f;
		shame += num_particles * part_size * ARC_PARTICLE_COST;
	}
	if (produces_light)
	{
		shame += ARC_LIGHT_COST;
	}
	if (media_faces)
	{
		shame += media_faces * ARC_MEDIA_FACE_COST;
	}
    if (isAnimatedObject() && isRootEdit())
    {
        shame += (ANIMATED_OBJECT_BASE_COST/0.06) * 5.0f;
    }
	if (shame > mRenderComplexity_current)
	{
		mRenderComplexity_current = (S32)shame;
	}
	return (U32)shame;
}
F32 LLVOVolume::getEstTrianglesMax() const
{
	if (isMesh() && getVolume())
	{
		return gMeshRepo.getEstTrianglesMax(getVolume()->getParams().getSculptID());
	}
    return 0.f;
}
F32 LLVOVolume::getEstTrianglesStreamingCost() const
{
	if (isMesh() && getVolume())
	{
		return gMeshRepo.getEstTrianglesStreamingCost(getVolume()->getParams().getSculptID());
	}
    return 0.f;
}
F32 LLVOVolume::getStreamingCost() const
{
	F32 radius = getScale().length()*0.5f;
    F32 linkset_base_cost = 0.f;
    LLMeshCostData costs;
    if (getCostData(costs))
    {
        if (isAnimatedObject() && isRootEdit())
        {
            linkset_base_cost = ANIMATED_OBJECT_BASE_COST;
        }
        if (isMesh())
        {
            if (isAnimatedObject() && isRiggedMesh())
            {
                return linkset_base_cost + costs.getTriangleBasedStreamingCost();
            }
            else
            {
                return linkset_base_cost + costs.getRadiusBasedStreamingCost(radius);
            }
        }
        else
        {
            return linkset_base_cost + costs.getRadiusBasedStreamingCost(radius);
        }
    }
    else
    {
        return 0.f;
    }
}
bool LLVOVolume::getCostData(LLMeshCostData& costs) const
{
	if (isMesh())
	{
		return gMeshRepo.getCostData(getVolume()->getParams().getSculptID(), costs);
	}
	else
	{
		LLVolume* volume = getVolume();
		S32 counts[4];
		LLVolume::getLoDTriangleCounts(volume->getParams(), counts);
		LLSD header;
		header["lowest_lod"]["size"] = counts[0] * 10;
		header["low_lod"]["size"] = counts[1] * 10;
		header["medium_lod"]["size"] = counts[2] * 10;
		header["high_lod"]["size"] = counts[3] * 10;
		return LLMeshRepository::getCostData(header, costs);
	}
}
void LLVOVolume::updateRenderComplexity()
{
	mRenderComplexity_last = mRenderComplexity_current;
	mRenderComplexity_current = 0;
}
U32 LLVOVolume::getTriangleCount(S32* vcount) const
{
	U32 count = 0;
	LLVolume* volume = getVolume();
	if (volume)
	{
		count = volume->getNumTriangles(vcount);
	}
	return count;
}
U32 LLVOVolume::getHighLODTriangleCount()
{
	U32 ret = 0;
	LLVolume* volume = getVolume();
	if (!isSculpted())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(volume->getParams(), 3);
		ret = ref->getNumTriangles();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else if (isMesh())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(volume->getParams(), 3);
		if (!ref->isMeshAssetLoaded() || ref->getNumVolumeFaces() == 0)
		{
			gMeshRepo.loadMesh(this, volume->getParams(), LLModel::LOD_HIGH);
		}
		ret = ref->getNumTriangles();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else
	{
		ret = 31*2*31;
	}
	return ret;
}
void LLVOVolume::preUpdateGeom()
{
	sNumLODChanges = 0;
}
void LLVOVolume::parameterChanged(U16 param_type, bool local_origin)
{
	LLViewerObject::parameterChanged(param_type, local_origin);
}
void LLVOVolume::parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin)
{
	LLViewerObject::parameterChanged(param_type, data, in_use, local_origin);
	if (mVolumeImpl)
	{
		mVolumeImpl->onParameterChanged(param_type, data, in_use, local_origin);
	}
    if (!local_origin && param_type == LLNetworkData::PARAMS_EXTENDED_MESH)
    {
        U32 extended_mesh_flags = getExtendedMeshFlags();
        bool enabled =  (extended_mesh_flags & LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG);
        bool was_enabled = (getControlAvatar() != NULL);
        if (enabled != was_enabled)
        {
            LL_DEBUGS("AnimatedObjects") << this
                                         << " calling onSetExtendedMeshFlags, enabled " << (U32) enabled
                                         << " was_enabled " << (U32) was_enabled
                                         << " local_origin " << (U32) local_origin
                                         << LL_ENDL;
            onSetExtendedMeshFlags(extended_mesh_flags);
        }
    }
	if (mDrawable.notNull())
	{
		BOOL is_light = getIsLight();
		if (is_light != mDrawable->isState(LLDrawable::LIGHT))
		{
			gPipeline.setLight(mDrawable, is_light);
		}
	}
}
void LLVOVolume::setSelected(BOOL sel)
{
	LLViewerObject::setSelected(sel);
    if (isAnimatedObject())
    {
        getRootEdit()->recursiveMarkForUpdate(TRUE);
    }
    else
    {
        if (mDrawable.notNull())
        {
            markForUpdate(TRUE);
        }
    }
}
void LLVOVolume::updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{
}
F32 LLVOVolume::getBinRadius()
{
	F32 radius;
	F32 scale = 1.f;
	static const LLCachedControl<S32> size_factor_setting("OctreeStaticObjectSizeFactor", 4);
	static const LLCachedControl<S32> attachment_size_factor_setting("OctreeAttachmentSizeFactor", 4);
	static const LLCachedControl<LLVector3> distance_factor_setting("OctreeDistanceFactor",LLVector3::zero);
	static const LLCachedControl<LLVector3> alpha_distance_factor_setting("OctreeAlphaDistanceFactor",LLVector3(.1f,0.f,0.f));
	const S32 size_factor = llmax(size_factor_setting.get(),1);
	const S32 attachment_size_factor = llmax(size_factor_setting.get(),1);
	const LLVector3& distance_factor = distance_factor_setting;
	const LLVector3& alpha_distance_factor = alpha_distance_factor_setting;
	const LLVector4a* ext = mDrawable->getSpatialExtents();
	BOOL shrink_wrap = mDrawable->isAnimating();
	BOOL alpha_wrap = FALSE;
	if (!isHUDAttachment())
	{
		for (S32 i = 0; i < mDrawable->getNumFaces(); i++)
		{
			LLFace* face = mDrawable->getFace(i);
			if (!face) continue;
			if (face->getPoolType() == LLDrawPool::POOL_ALPHA &&
			    !face->canRenderAsMask())
			{
				alpha_wrap = TRUE;
				break;
			}
		}
	}
	else
	{
		shrink_wrap = FALSE;
	}
	if (alpha_wrap)
	{
		LLVector3 bounds = getScale();
		radius = llmin(bounds.mV[1], bounds.mV[2]);
		radius = llmin(radius, bounds.mV[0]);
		radius *= 0.5f;
		radius *= 1.f+mDrawable->mDistanceWRTCamera*alpha_distance_factor[1];
		radius += mDrawable->mDistanceWRTCamera*alpha_distance_factor[0];
	}
	else if (shrink_wrap)
	{
		LLVector4a rad;
		rad.setSub(ext[1], ext[0]);
		radius = rad.getLength3().getF32()*0.5f;
	}
	else if (mDrawable->isStatic())
	{
		F32 szf = size_factor;
		radius = llmax(mDrawable->getRadius(), szf);
		radius = powf(radius, 1.f+szf/radius);
		radius *= 1.f + mDrawable->mDistanceWRTCamera * distance_factor[1];
		radius += mDrawable->mDistanceWRTCamera * distance_factor[0];
	}
	else if (mDrawable->getVObj()->isAttachment())
	{
		radius = llmax((S32) mDrawable->getRadius(),1)*attachment_size_factor;
	}
	else
	{
		radius = mDrawable->getRadius();
		radius *= 1.f + mDrawable->mDistanceWRTCamera * distance_factor[1];
		radius += mDrawable->mDistanceWRTCamera * distance_factor[0];
	}
	return llclamp(radius*scale, 0.5f, 256.f);
}
const LLVector3 LLVOVolume::getPivotPositionAgent() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getPivotPosition();
	}
	return LLViewerObject::getPivotPositionAgent();
}
void LLVOVolume::onShift(const LLVector4a &shift_vector)
{
	if (mVolumeImpl)
	{
		mVolumeImpl->onShift(shift_vector);
	}
	updateRelativeXform();
}
const LLMatrix4a& LLVOVolume::getWorldMatrix(LLXformMatrix* xform) const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getWorldMatrix(xform);
	}
	return xform->getWorldMatrix();
}
void LLVOVolume::markForUpdate(BOOL priority)
{
    LLViewerObject::markForUpdate(priority);
    mVolumeChanged = TRUE;
}
LLVector3 LLVOVolume::agentPositionToVolume(const LLVector3& pos) const
{
	LLVector3 ret = pos - getRenderPosition();
	ret = ret * ~getRenderRotation();
	if (!isVolumeGlobal())
	{
		LLVector3 objScale = getScale();
		LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
		ret.scaleVec(invObjScale);
	}
	return ret;
}
LLVector3 LLVOVolume::agentDirectionToVolume(const LLVector3& dir) const
{
	LLVector3 ret = dir * ~getRenderRotation();
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	ret.scaleVec(objScale);
	return ret;
}
LLVector3 LLVOVolume::volumePositionToAgent(const LLVector3& dir) const
{
	LLVector3 ret = dir;
	if (!isVolumeGlobal())
	{
		LLVector3 objScale = getScale();
		ret.scaleVec(objScale);
	}
	ret = ret * getRenderRotation();
	ret += getRenderPosition();
	return ret;
}
LLVector3 LLVOVolume::volumeDirectionToAgent(const LLVector3& dir) const
{
	LLVector3 ret = dir;
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
	ret.scaleVec(invObjScale);
	ret = ret * getRenderRotation();
	return ret;
}
BOOL LLVOVolume::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end, S32 face, BOOL pick_transparent, BOOL pick_rigged, S32 *face_hitp,
									  LLVector4a* intersection,LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent)
{
	if (!mbCanSelect ||
		( (LLSelectMgr::getInstance()->mHideSelectedObjects && isSelected()) &&
		 ( (!rlv_handler_t::isEnabled()) ||
		   ( ((!isHUDAttachment()) || (!gRlvAttachmentLocks.isLockedAttachment(getRootEdit()))) &&
		     (gRlvHandler.canEdit(this)) ) ) ) ||
			mDrawable->isDead() ||
			!gPipeline.hasRenderType(mDrawable->getRenderType()))
	{
		return FALSE;
	}
	BOOL ret = FALSE;
	LLVolume* volume = getVolume();
	bool transform = true;
	if (mDrawable->isState(LLDrawable::RIGGED))
	{
		if (pick_rigged)
		{
			updateRiggedVolume(true);
			volume = mRiggedVolume;
			transform = false;
		}
		else
		{
			return FALSE;
		}
	}
	if (volume)
	{
		LLVector4a local_start = start;
		LLVector4a local_end = end;
		if (transform)
		{
			LLVector3 v_start(start.getF32ptr());
			LLVector3 v_end(end.getF32ptr());
			v_start = agentPositionToVolume(v_start);
			v_end = agentPositionToVolume(v_end);
			local_start.load3(v_start.mV);
			local_end.load3(v_end.mV);
		}
		LLVector4a p;
		LLVector4a n;
		LLVector2 tc;
		LLVector4a tn;
		if (intersection != NULL)
		{
			p = *intersection;
		}
		if (tex_coord != NULL)
		{
			tc = *tex_coord;
		}
		if (normal != NULL)
		{
			n = *normal;
		}
		if (tangent != NULL)
		{
			tn = *tangent;
		}
		S32 face_hit = -1;
		S32 start_face, end_face;
		if (face == -1)
		{
			start_face = 0;
			end_face = volume->getNumVolumeFaces();
		}
		else
		{
			start_face = face;
			end_face = face+1;
		}
		bool special_cursor = specialHoverCursor();
		for (S32 i = start_face; i < end_face; ++i)
		{
			if (!special_cursor && !pick_transparent && getTE(i) && getTE(i)->getColor().mV[3] == 0.f)
			{
				continue;
			}
			face_hit = volume->lineSegmentIntersect(local_start, local_end, i,
													&p, &tc, &n, &tn);
			if (face_hit >= 0 && mDrawable->getNumFaces() > face_hit)
			{
				LLFace* face = mDrawable->getFace(face_hit);
				bool ignore_alpha = false;
				const LLTextureEntry* te = face->getTextureEntry();
				if (te)
				{
					LLMaterial* mat = te->getMaterialParams();
					if (mat)
					{
						U8 mode = mat->getDiffuseAlphaMode();
						if (mode == LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE ||
							mode == LLMaterial::DIFFUSE_ALPHA_MODE_NONE)
						{
							ignore_alpha = true;
						}
					}
				}
				if (face &&
					(ignore_alpha ||
					pick_transparent ||
					!face->getTexture() ||
					!face->getTexture()->hasGLTexture() ||
					face->getTexture()->getMask(face->surfaceToTexture(tc, p, n))))
				{
					local_end = p;
					if (face_hitp != NULL)
					{
						*face_hitp = face_hit;
					}
					if (intersection != NULL)
					{
						if (transform)
						{
							LLVector3 v_p(p.getF32ptr());
							intersection->load3(volumePositionToAgent(v_p).mV);
						}
						else
						{
							*intersection = p;
						}
					}
					if (normal != NULL)
					{
						if (transform)
						{
							LLVector3 v_n(n.getF32ptr());
							normal->load3(volumeDirectionToAgent(v_n).mV);
						}
						else
						{
							*normal = n;
						}
						(*normal).normalize3fast();
					}
					if (tangent != NULL)
					{
						if (transform)
						{
							LLVector3 v_tn(tn.getF32ptr());
							LLVector4a trans_tangent;
							trans_tangent.load3(volumeDirectionToAgent(v_tn).mV);
							LLVector4Logical mask;
							mask.clear();
							mask.setElement<3>();
							tangent->setSelectWithMask(mask, tn, trans_tangent);
						}
						else
						{
							*tangent = tn;
						}
						(*tangent).normalize3fast();
					}
					if (tex_coord != NULL)
					{
						*tex_coord = tc;
					}
					ret = TRUE;
				}
			}
		}
	}
	return ret;
}
bool LLVOVolume::treatAsRigged()
{
	return isSelected() &&
			(isAttachment() || isAnimatedObject()) &&
			mDrawable.notNull() &&
			mDrawable->isState(LLDrawable::RIGGED);
}
LLRiggedVolume* LLVOVolume::getRiggedVolume()
{
	return mRiggedVolume;
}
void LLVOVolume::clearRiggedVolume()
{
	if (mRiggedVolume.notNull())
	{
		mRiggedVolume = NULL;
		updateRelativeXform();
	}
}
void LLVOVolume::updateRiggedVolume(bool force_update)
{
	if ((!force_update) && (!treatAsRigged()))
	{
		clearRiggedVolume();
		return;
	}
	LLVolume* volume = getVolume();
	const LLMeshSkinInfo* skin = getSkinInfo();
	if (!skin)
	{
		clearRiggedVolume();
		return;
	}
	LLVOAvatar* avatar = getAvatar();
	if (!avatar)
	{
		clearRiggedVolume();
		return;
	}
	if (!mRiggedVolume)
	{
		LLVolumeParams p;
		mRiggedVolume = new LLRiggedVolume(p);
		updateRelativeXform();
	}
	mRiggedVolume->update(skin, avatar, volume);
}
static LLTrace::BlockTimerStatHandle FTM_SKIN_RIGGED("Skin");
static LLTrace::BlockTimerStatHandle FTM_RIGGED_OCTREE("Octree");
void LLRiggedVolume::update(const LLMeshSkinInfo* skin, LLVOAvatar* avatar, const LLVolume* volume)
{
	bool copy = false;
	if (volume->getNumVolumeFaces() != getNumVolumeFaces())
	{
		copy = true;
	}
	for (S32 i = 0; i < volume->getNumVolumeFaces() && !copy; ++i)
	{
		const LLVolumeFace& src_face = volume->getVolumeFace(i);
		const LLVolumeFace& dst_face = getVolumeFace(i);
		if (src_face.mNumIndices != dst_face.mNumIndices ||
			src_face.mNumVertices != dst_face.mNumVertices)
		{
			copy = true;
		}
	}
	U64 frame = LLFrameTimer::getFrameCount();
	if (copy)
	{
		copyVolumeFaces(volume);
	}
    else if (avatar && avatar->areAnimationsPaused())
    {
		frame = avatar->getMotionController().getPausedFrame();
    }
	if (frame == mFrame)
	{
		return;
	}
	mFrame = frame;
	static const size_t kMaxJoints = LL_MAX_JOINTS_PER_MESH_OBJECT;
	LLMatrix4a mat[kMaxJoints];
	U32 maxJoints = LLSkinningUtil::getMeshJointCount(skin);
	LLSkinningUtil::initSkinningMatrixPalette(mat, maxJoints, skin, avatar, true);
	LLMatrix4a bind_shape_matrix;
	bind_shape_matrix.loadu(skin->mBindShapeMatrix);
	LLVector4a av_pos;
	av_pos.load3(avatar->getPosition().mV);
	for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& vol_face = volume->getVolumeFace(i);
		LLVolumeFace& dst_face = mVolumeFaces[i];
		LLVector4a* weight = vol_face.mWeights;
		if(!weight)
		{
			continue;
		}
		LLSkinningUtil::checkSkinWeights(weight, dst_face.mNumVertices, skin);
		LLVector4a* pos = dst_face.mPositions;
		if( pos && dst_face.mExtents )
		{
			LL_RECORD_BLOCK_TIME(FTM_SKIN_RIGGED);
			U32 max_joints = LLSkinningUtil::getMaxJointCount();
			for (U32 j = 0; j < (U32)dst_face.mNumVertices; ++j)
			{
				LLMatrix4a final_mat;
				LLSkinningUtil::getPerVertexSkinMatrix(weight[j].getF32ptr(), mat, false, final_mat, max_joints);
				LLVector4a& v = vol_face.mPositions[j];
				LLVector4a t;
				bind_shape_matrix.affineTransform(v, t);
				final_mat.affineTransform(t, pos[j]);
				pos[j].add(av_pos);
			}
			LLVector4a& min = dst_face.mExtents[0];
			LLVector4a& max = dst_face.mExtents[1];
			min = pos[0];
			max = pos[1];
			for (U32 j = 1; j < (U32)dst_face.mNumVertices; ++j)
			{
				min.setMin(min, pos[j]);
				max.setMax(max, pos[j]);
			}
			dst_face.mCenter->setAdd(dst_face.mExtents[0], dst_face.mExtents[1]);
			dst_face.mCenter->mul(0.5f);
		}
		{
			LL_RECORD_BLOCK_TIME(FTM_RIGGED_OCTREE);
			delete dst_face.mOctree;
			dst_face.mOctree = NULL;
			LLVector4a size;
			size.setSub(dst_face.mExtents[1], dst_face.mExtents[0]);
			size.splat(size.getLength3().getF32()*0.5f);
			dst_face.createOctree(1.f);
		}
	}
}
U32 LLVOVolume::getPartitionType() const
{
	if (isHUDAttachment())
	{
		return LLViewerRegion::PARTITION_HUD;
	}
	else if (isAttachment())
	{
		return LLViewerRegion::PARTITION_ATTACHMENT;
	}
	return LLViewerRegion::PARTITION_VOLUME;
}
LLVolumePartition::LLVolumePartition(LLViewerRegion* regionp)
: LLSpatialPartition(LLVOVolume::VERTEX_DATA_MASK, TRUE, GL_DYNAMIC_DRAW_ARB, regionp),
LLVolumeGeometryManager()
{
	mLODPeriod = 32;
	mDepthMask = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_VOLUME;
	mSlopRatio = 0.25f;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
}
LLVolumeBridge::LLVolumeBridge(LLDrawable* drawablep, LLViewerRegion* regionp)
: LLSpatialBridge(drawablep, TRUE, LLVOVolume::VERTEX_DATA_MASK, regionp),
LLVolumeGeometryManager()
{
	mDepthMask = FALSE;
	mLODPeriod = 32;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_BRIDGE;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
	mSlopRatio = 0.25f;
}
LLAttachmentBridge::LLAttachmentBridge(LLDrawable* drawablep, LLViewerRegion* regionp)
	: LLVolumeBridge(drawablep, regionp)
{
	mPartitionType = LLViewerRegion::PARTITION_ATTACHMENT;
}
bool can_batch_texture(const LLFace* facep)
{
	static const LLCachedControl<bool> alt_batching("SHAltBatching",true);
	if(!alt_batching)
	{
	if (facep->getTextureEntry()->getBumpmap())
	{
		return false;
	}
	if (facep->getTextureEntry()->getMaterialParams().notNull())
	{
		return false;
	}
	if (facep->isState(LLFace::TEXTURE_ANIM) && facep->getVirtualSize() > MIN_TEX_ANIM_SIZE)
	{
		return false;
	}
	}
	else
	{
	if (facep->getPoolType() == LLDrawPool::POOL_BUMP && (facep->getTextureEntry()->getBumpmap() > 0 && facep->getTextureEntry()->getBumpmap() < 18))
	{
		return false;
	}
	if (LLPipeline::sRenderDeferred && (facep->getPoolType() == LLDrawPool::POOL_ALPHA || facep->getPoolType() == LLDrawPool::POOL_MATERIALS) && facep->getTextureEntry()->getMaterialParams().notNull())
	{
		return false;
	}
	if (facep->isState(LLFace::TEXTURE_ANIM) && facep->getVirtualSize() > MIN_TEX_ANIM_SIZE)
	{
		return false;
	}
	}
	return true;
}
const static U32 MAX_FACE_COUNT = 4096U;
int32_t LLVolumeGeometryManager::sInstanceCount = 0;
LLFace** LLVolumeGeometryManager::sFullbrightFaces = NULL;
LLFace** LLVolumeGeometryManager::sBumpFaces = NULL;
LLFace** LLVolumeGeometryManager::sSimpleFaces = NULL;
LLFace** LLVolumeGeometryManager::sNormFaces = NULL;
LLFace** LLVolumeGeometryManager::sSpecFaces = NULL;
LLFace** LLVolumeGeometryManager::sNormSpecFaces = NULL;
LLFace** LLVolumeGeometryManager::sAlphaFaces = NULL;
LLVolumeGeometryManager::LLVolumeGeometryManager()
	: LLGeometryManager()
{
	llassert(sInstanceCount >= 0);
	if (sInstanceCount == 0)
	{
		allocateFaces(MAX_FACE_COUNT);
	}
	++sInstanceCount;
}
LLVolumeGeometryManager::~LLVolumeGeometryManager()
{
	llassert(sInstanceCount > 0);
	--sInstanceCount;
	if (sInstanceCount <= 0)
	{
		freeFaces();
		sInstanceCount = 0;
	}
}
void LLVolumeGeometryManager::allocateFaces(U32 pMaxFaceCount)
{
	sFullbrightFaces = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount*sizeof(LLFace*)));
	sBumpFaces = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount*sizeof(LLFace*)));
	sSimpleFaces = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount*sizeof(LLFace*)));
	sNormFaces = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount*sizeof(LLFace*)));
	sSpecFaces = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount*sizeof(LLFace*)));
	sNormSpecFaces = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount*sizeof(LLFace*)));
	sAlphaFaces = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount*sizeof(LLFace*)));
}
void LLVolumeGeometryManager::freeFaces()
{
	ll_aligned_free<64>(sFullbrightFaces);
	ll_aligned_free<64>(sBumpFaces);
	ll_aligned_free<64>(sSimpleFaces);
	ll_aligned_free<64>(sNormFaces);
	ll_aligned_free<64>(sSpecFaces);
	ll_aligned_free<64>(sNormSpecFaces);
	ll_aligned_free<64>(sAlphaFaces);
	sFullbrightFaces = NULL;
	sBumpFaces = NULL;
	sSimpleFaces = NULL;
	sNormFaces = NULL;
	sSpecFaces = NULL;
	sNormSpecFaces = NULL;
	sAlphaFaces = NULL;
}
static LLTrace::BlockTimerStatHandle FTM_REGISTER_FACE("Register Face");
void LLVolumeGeometryManager::registerFace(LLSpatialGroup* group, LLFace* facep, U32 type)
{
	LL_RECORD_BLOCK_TIME(FTM_REGISTER_FACE);
	const LLViewerObject* pObj = facep->getViewerObject();
	if ( (pObj->isSelected() && LLSelectMgr::getInstance()->mHideSelectedObjects) &&
		 ( (!rlv_handler_t::isEnabled()) ||
		   ( ((!pObj->isHUDAttachment()) || (!gRlvAttachmentLocks.isLockedAttachment(pObj->getRootEdit()))) &&
		     (gRlvHandler.canEdit(pObj)) ) ) )
	{
		return;
	}
	if(!facep->mShinyInAlpha)
		facep->mShinyInAlpha =	(type == LLRenderPass::PASS_FULLBRIGHT_SHINY) ||
								(type == LLRenderPass::PASS_SHINY) ||
								(LLPipeline::sRenderDeferred && type == LLRenderPass::PASS_BUMP) ||
								(LLPipeline::sRenderDeferred && type == LLRenderPass::PASS_SIMPLE);
	LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[type];
	S32 idx = draw_vec.size()-1;
	static const LLCachedControl<bool> alt_batching("SHAltBatching",true);
	BOOL fullbright;
	if(!alt_batching)
	{
	fullbright = (type == LLRenderPass::PASS_FULLBRIGHT) ||
		(type == LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK) ||
		(type == LLRenderPass::PASS_ALPHA && facep->isState(LLFace::FULLBRIGHT)) ||
		(facep->getTextureEntry()->getFullbright());
	}
	else
	{
	fullbright = facep->isState(LLFace::FULLBRIGHT);
	}
	if (!fullbright && type != LLRenderPass::PASS_GLOW && !facep->getVertexBuffer()->hasDataType(LLVertexBuffer::TYPE_NORMAL))
	{
		LL_WARNS() << "Non fullbright face has no normals!" << LL_ENDL;
		return;
	}
	const LLMatrix4a* tex_mat = NULL;
	if (facep->isState(LLFace::TEXTURE_ANIM) && facep->getVirtualSize() > MIN_TEX_ANIM_SIZE)
	{
		tex_mat = facep->mTextureMatrix;
	}
	const LLMatrix4a* model_mat = NULL;
	LLDrawable* drawable = facep->getDrawable();
	if (drawable->isState(LLDrawable::ANIMATED_CHILD))
	{
		model_mat = &drawable->getWorldMatrix();
	}
	else if (drawable->isActive())
	{
		model_mat = &drawable->getRenderMatrix();
	}
	else
	{
		model_mat = &(drawable->getRegion()->mRenderMatrix);
	}
	if(model_mat && model_mat->isIdentity())
	{
		model_mat = NULL;
	}
	LLMaterial* mat = facep->getTextureEntry()->getMaterialParams().get();
	U32 pool_type = facep->getPoolType();
	bool cmp_bump = (type == LLRenderPass::PASS_BUMP) || (type == LLRenderPass::PASS_POST_BUMP);
	bool cmp_mat =	(!alt_batching) || LLPipeline::sRenderDeferred &&
					((pool_type == LLDrawPool::POOL_MATERIALS) || (pool_type == LLDrawPool::POOL_ALPHA)) ||
					pool_type == LLDrawPool::POOL_ALPHA_MASK || pool_type == LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK;
	bool cmp_shiny = (!alt_batching) ? !!mat : (mat && cmp_mat);
	bool cmp_fullbright = !alt_batching || cmp_shiny || pool_type == LLDrawPool::POOL_ALPHA;
	U8 bump = facep->getTextureEntry()->getBumpmap();
	U8 shiny = facep->getTextureEntry()->getShiny();
	LLViewerTexture* tex = facep->getTexture();
	U8 index = facep->getTextureIndex();
	bool batchable = false;
	U32 shader_mask = 0xFFFFFFFF;
	if (mat && cmp_mat)
	{
		if (type == LLRenderPass::PASS_ALPHA)
		{
			shader_mask = mat->getShaderMask(LLMaterial::DIFFUSE_ALPHA_MODE_BLEND);
		}
		else
		{
			shader_mask = mat->getShaderMask();
		}
	}
	if (index < 255 && idx >= 0)
	{
		if (cmp_mat && (mat || draw_vec[idx]->mMaterial))
		{
			batchable = false;
		}
		else if (index < draw_vec[idx]->mTextureList.size())
		{
			if (draw_vec[idx]->mTextureList[index].isNull())
			{
				batchable = true;
				draw_vec[idx]->mTextureList[index] = tex;
			}
			else if (draw_vec[idx]->mTextureList[index] == tex)
			{
				batchable = true;
			}
		}
		else
		{
			batchable = true;
		}
	}
	if (idx >= 0 &&
		draw_vec[idx]->mVertexBuffer == facep->getVertexBuffer() &&
		draw_vec[idx]->mEnd == facep->getGeomIndex()-1 &&
		(LLPipeline::sTextureBindTest || draw_vec[idx]->mTexture == tex || batchable) &&
		(!cmp_mat || draw_vec[idx]->mMaterial == mat) &&
		(!cmp_fullbright || draw_vec[idx]->mFullbright == fullbright) &&
		(!cmp_bump || draw_vec[idx]->mBump == bump)  &&
		(!cmp_shiny || draw_vec[idx]->mShiny == shiny) &&
		draw_vec[idx]->mTextureMatrix == tex_mat &&
		draw_vec[idx]->mModelMatrix == model_mat &&
		(!cmp_mat || draw_vec[idx]->mShaderMask == shader_mask))
	{
		draw_vec[idx]->mCount += facep->getIndicesCount();
		draw_vec[idx]->mEnd += facep->getGeomCount();
		draw_vec[idx]->mVSize = llmax(draw_vec[idx]->mVSize, facep->getVirtualSize());
		if (index < 255 && index >= draw_vec[idx]->mTextureList.size())
		{
			draw_vec[idx]->mTextureList.resize(index+1);
			draw_vec[idx]->mTextureList[index] = tex;
		}
		update_min_max(draw_vec[idx]->mExtents[0], draw_vec[idx]->mExtents[1], facep->mExtents[0]);
		update_min_max(draw_vec[idx]->mExtents[0], draw_vec[idx]->mExtents[1], facep->mExtents[1]);
	}
	else
	{
		U32 start = facep->getGeomIndex();
		U32 end = start + facep->getGeomCount()-1;
		U32 offset = facep->getIndicesStart();
		U32 count = facep->getIndicesCount();
		LLPointer<LLDrawInfo> draw_info = new LLDrawInfo(start,end,count,offset,tex,
			facep->getVertexBuffer(), fullbright, bump);
		draw_info->mGroup = group;
		draw_info->mVSize = facep->getVirtualSize();
		draw_vec.push_back(draw_info);
		draw_info->mTextureMatrix = tex_mat;
		draw_info->mModelMatrix = model_mat;
		draw_info->mBump  = bump;
		draw_info->mShiny = shiny;
		float alpha[4] =
		{
			0.00f,
			0.25f,
			0.5f,
			0.75f
		};
		float spec = alpha[shiny & TEM_SHINY_MASK];
		LLVector4 specColor(spec, spec, spec, spec);
		draw_info->mSpecColor = specColor;
		draw_info->mEnvIntensity = spec;
		draw_info->mSpecularMap = NULL;
		draw_info->mMaterial = mat;
		draw_info->mShaderMask = shader_mask;
		if (cmp_mat && mat)
		{
				if (!mat->getSpecularID().isNull())
				{
					LLVector4 specColor;
					specColor.mV[0] = mat->getSpecularLightColor().mV[0] * (1.f / 255.f);
					specColor.mV[1] = mat->getSpecularLightColor().mV[1] * (1.f / 255.f);
					specColor.mV[2] = mat->getSpecularLightColor().mV[2] * (1.f / 255.f);
					specColor.mV[3] = llmax(0.0000f, mat->getSpecularLightExponent() * (1.f / 255.f));
					draw_info->mSpecColor = specColor;
					draw_info->mEnvIntensity = mat->getEnvironmentIntensity() * (1.f / 255.f);
					draw_info->mSpecularMap = facep->getViewerObject()->getTESpecularMap(facep->getTEOffset());
				}
				draw_info->mAlphaMaskCutoff = mat->getAlphaMaskCutoff() * (1.f / 255.f);
				draw_info->mDiffuseAlphaMode = mat->getDiffuseAlphaMode();
				draw_info->mNormalMap = facep->getViewerObject()->getTENormalMap(facep->getTEOffset());
		}
		else
		{
			if (type == LLRenderPass::PASS_GRASS)
			{
				draw_info->mAlphaMaskCutoff = 0.5f;
			}
			else
			{
				draw_info->mAlphaMaskCutoff = 0.33f;
			}
		}
		if (type == LLRenderPass::PASS_ALPHA)
		{
			facep->setDrawInfo(draw_info);
		}
		draw_info->mExtents[0] = facep->mExtents[0];
		draw_info->mExtents[1] = facep->mExtents[1];
		if (index < 255)
		{
			draw_info->mTextureList.resize(index+1);
			draw_info->mTextureList[index] = tex;
		}
	}
}
void LLVolumeGeometryManager::getGeometry(LLSpatialGroup* group)
{
}
static LLTrace::BlockTimerStatHandle FTM_REBUILD_VOLUME_VB("Volume VB");
static LLTrace::BlockTimerStatHandle FTM_REBUILD_VOLUME_FACE_LIST("Build Face List");
static LLTrace::BlockTimerStatHandle FTM_REBUILD_VOLUME_GEN_DRAW_INFO("Gen Draw Info");
static LLDrawPoolAvatar* get_avatar_drawpool(LLViewerObject* vobj)
{
	LLVOAvatar* avatar = vobj->getAvatar();
	if (avatar)
	{
		LLDrawable* drawable = avatar->mDrawable;
		if (drawable && drawable->getNumFaces() > 0)
		{
			LLFace* face = drawable->getFace(0);
			if (face)
			{
				LLDrawPool* drawpool = face->getPool();
				if (drawpool)
				{
					if (drawpool->getType() == LLDrawPool::POOL_AVATAR)
					{
						return (LLDrawPoolAvatar*) drawpool;
					}
				}
			}
		}
	}
	return NULL;
}
void handleRenderAutoMuteByteLimitChanged(const LLSD& new_value)
{
	static LLCachedControl<U32> render_auto_mute_byte_limit(gSavedSettings, "RenderAutoMuteByteLimit", 0U);
	if (0 != render_auto_mute_byte_limit)
	{
		LLSculptIDSize::container_BY_SIZE_view::iterator
			itL = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().lower_bound(render_auto_mute_byte_limit),
			itU = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().end();
		for (; itL != itU; ++itL)
		{
			const LLSculptIDSize::Info &nfo = *itL;
			LLVOVolume *pVVol = nfo.getPtrLLDrawable()->getVOVolume();
			if (pVVol
				&& !pVVol->isDead()
				&& pVVol->isAttachment()
				&& !pVVol->getAvatar()->isSelf()
				&& LLVOVolume::NO_LOD != pVVol->getLOD()
				)
			{
				pVVol->markForUnload();
				LLSculptIDSize::instance().addToUnloaded(nfo.getSculptId());
			}
		}
		itL = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().begin();
		itU = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().upper_bound(render_auto_mute_byte_limit);
		for (; itL != itU; ++itL)
		{
			const LLSculptIDSize::Info &nfo = *itL;
			LLVOVolume *pVVol = nfo.getPtrLLDrawable()->getVOVolume();
			if (pVVol
				&& !pVVol->isDead()
				&& pVVol->isAttachment()
				&& !pVVol->getAvatar()->isSelf()
				&& LLVOVolume::NO_LOD == pVVol->getLOD()
				)
			{
				LLSculptIDSize::instance().remFromUnloaded(nfo.getSculptId());
				pVVol->updateLOD();
				pVVol->markForUpdate(TRUE);
			}
		}
	}
	else
	{
		LLSculptIDSize::instance().clearUnloaded();
		LLSculptIDSize::container_BY_SIZE_view::iterator
			itL = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().begin(),
			itU = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().end();
		for (; itL != itU; ++itL)
		{
			const LLSculptIDSize::Info &nfo = *itL;
			LLVOVolume *pVVol = nfo.getPtrLLDrawable()->getVOVolume();
			if (pVVol
				&& !pVVol->isDead()
				&& pVVol->isAttachment()
				&& !pVVol->getAvatar()->isSelf()
				&& LLVOVolume::NO_LOD == pVVol->getLOD()
				)
			{
				pVVol->updateLOD();
				pVVol->markForUpdate(TRUE);
			}
		}
	}
}
void LLVolumeGeometryManager::rebuildGeom(LLSpatialGroup* group)
{
	if (LLPipeline::sSkipUpdate)
	{
		return;
	}
	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
	}
	group->mLastUpdateViewAngle = group->mViewAngle;
	if (!group->hasState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY))
	{
		if (group->hasState(LLSpatialGroup::MESH_DIRTY) && !LLPipeline::sDelayVBUpdate)
		{
			rebuildMesh(group);
		}
		return;
	}
	LL_RECORD_BLOCK_TIME(FTM_REBUILD_VOLUME_VB);
	group->mBuilt = 1.f;
	LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();
    LLViewerObject *vobj = NULL;
    LLVOVolume *vol_obj = NULL;
	if (bridge)
	{
        vobj = bridge->mDrawable->getVObj();
        vol_obj = vobj ? vobj->asVolume() : nullptr;
	}
    if (vol_obj)
    {
        vol_obj->updateVisualComplexity();
    }
	group->mGeometryBytes = 0;
	group->mSurfaceArea = 0;
	const LLVector4a* bounds = group->getObjectBounds();
	group->mObjectBoxSize = bounds[1].getLength3().getF32();
	group->clearDrawMap();
	mFaceList.clear();
	U32 fullbright_count = 0;
	U32 bump_count = 0;
	U32 simple_count = 0;
	U32 alpha_count = 0;
	U32 norm_count = 0;
	U32 spec_count = 0;
	U32 normspec_count = 0;
	U32 useage = group->getSpatialPartition()->mBufferUsage;
	static const LLCachedControl<S32> render_max_vbo_size("RenderMaxVBOSize", 512);
	static const LLCachedControl<S32> render_max_node_size("RenderMaxNodeSize",8192);
	U32 max_vertices = (render_max_vbo_size*1024)/LLVertexBuffer::calcVertexSize(group->getSpatialPartition()->mVertexDataMask);
	U32 max_total = (render_max_node_size*1024)/LLVertexBuffer::calcVertexSize(group->getSpatialPartition()->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);
	U32 cur_total = 0;
	bool emissive = false;
	{
		LL_RECORD_BLOCK_TIME(FTM_REBUILD_VOLUME_FACE_LIST);
		OctreeGuard guard(group->getOctreeNode());
		for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
		{
			LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
			if (!drawablep || drawablep->isDead() || drawablep->isState(LLDrawable::FORCE_INVISIBLE) )
			{
				continue;
			}
			if (drawablep->isAnimating())
			{
				useage = GL_STREAM_DRAW_ARB;
			}
			LLVOVolume* vobj = drawablep->getVOVolume();
			if (!vobj)
			{
				continue;
			}
			if (vobj->isMesh() &&
					((vobj->getVolume() && !vobj->getVolume()->isMeshAssetLoaded()) || !gMeshRepo.meshRezEnabled()))
			{
				continue;
			}
			LLVolume* volume = vobj->getVolume();
			if (volume)
			{
				const LLVector3& scale = vobj->getScale();
				group->mSurfaceArea += volume->getSurfaceArea() * llmax(llmax(scale.mV[0], scale.mV[1]), scale.mV[2]);
			}
			vobj->updateControlAvatar();
			llassert_always(vobj);
			vobj->updateTextureVirtualSize(true);
			vobj->preRebuild();
			drawablep->clearState(LLDrawable::HAS_ALPHA);
            if (vobj->isRiggedMesh() &&
                ((vobj->isAnimatedObject() && vobj->getControlAvatar()) ||
                 (!vobj->isAnimatedObject() && vobj->getAvatar())))
            {
                vobj->getAvatar()->addAttachmentOverridesForObject(vobj, NULL, false);
            }
			bool rigged = !vobj->isAnimatedObject() && vobj->isRiggedMesh() && vobj->isAttachment();
			rigged = rigged || (vobj->isAnimatedObject() && vobj->isRiggedMesh() &&
                vobj->getControlAvatar() && vobj->getControlAvatar()->mPlaying);
			bool any_rigged_face = false;
			static const LLCachedControl<bool> alt_batching("SHAltBatching",true);
			for (S32 i = 0; i < drawablep->getNumFaces(); i++)
			{
				LLFace* facep = drawablep->getFace(i);
				if (!facep)
				{
					continue;
				}
				facep->setVertexBuffer(NULL);
				drawablep->updateFaceSize(i);
				if (rigged)
				{
					if (!facep->isState(LLFace::RIGGED))
					{
						facep->clearVertexBuffer();
					}
					facep->setState(LLFace::RIGGED);
					any_rigged_face = true;
					LLDrawPoolAvatar* pool = get_avatar_drawpool(vobj);
					if (pool)
					{
						const LLTextureEntry* te = facep->getTextureEntry();
						LLDrawPool* old_pool = facep->getPool();
						if (old_pool && old_pool->getType() == LLDrawPool::POOL_AVATAR)
						{
							((LLDrawPoolAvatar*) old_pool)->removeRiggedFace(facep);
						}
						LLViewerTexture* tex = facep->getTexture();
						U32 type = gPipeline.getPoolTypeFromTE(te, tex);
						if (te->getGlow())
						{
							pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_GLOW);
						}
						LLMaterial* mat = te->getMaterialParams().get();
						if(!alt_batching)
						{
						if (mat && LLPipeline::sRenderDeferred)
						{
							U8 alpha_mode = mat->getDiffuseAlphaMode();
							bool is_alpha = type == LLDrawPool::POOL_ALPHA &&
								(alpha_mode == LLMaterial::DIFFUSE_ALPHA_MODE_BLEND ||
								te->getColor().mV[3] < 0.999f);
							if (is_alpha)
							{
								alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
							}
							if (!is_alpha || te->getColor().mV[3] > 0.f)
							{
								U32 mask = mat->getShaderMask(alpha_mode);
								pool->addRiggedFace(facep, mask);
							}
						}
						else if (mat)
						{
							bool fullbright = te->getFullbright();
							bool is_alpha = type == LLDrawPool::POOL_ALPHA;
							U8 mode = mat->getDiffuseAlphaMode();
							bool can_be_shiny = mode == LLMaterial::DIFFUSE_ALPHA_MODE_NONE ||
												mode == LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE;
							if (mode == LLMaterial::DIFFUSE_ALPHA_MODE_MASK && te->getColor().mV[3] >= 0.999f)
							{
								pool->addRiggedFace(facep, fullbright ? LLDrawPoolAvatar::RIGGED_FULLBRIGHT : LLDrawPoolAvatar::RIGGED_SIMPLE);
							}
							else if (is_alpha || (te->getColor().mV[3] < 0.999f))
							{
								if (te->getColor().mV[3] > 0.f)
								{
									pool->addRiggedFace(facep, fullbright ? LLDrawPoolAvatar::RIGGED_FULLBRIGHT_ALPHA : LLDrawPoolAvatar::RIGGED_ALPHA);
								}
							}
							else if (LLGLSLShader::sNoFixedFunction
								&& LLPipeline::sRenderBump
								&& te->getShiny()
								&& can_be_shiny)
							{
								pool->addRiggedFace(facep, fullbright ? LLDrawPoolAvatar::RIGGED_FULLBRIGHT_SHINY : LLDrawPoolAvatar::RIGGED_SHINY);
							}
							else
							{
								pool->addRiggedFace(facep, fullbright ? LLDrawPoolAvatar::RIGGED_FULLBRIGHT : LLDrawPoolAvatar::RIGGED_SIMPLE);
							}
						}
						else
						{
							if (type == LLDrawPool::POOL_ALPHA)
							{
								if (te->getColor().mV[3] > 0.f)
								{
									if (te->getFullbright())
									{
										pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_FULLBRIGHT_ALPHA);
									}
									else
									{
										pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_ALPHA);
									}
								}
							}
							else if (te->getShiny())
							{
								if (te->getFullbright())
								{
									pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_FULLBRIGHT_SHINY);
								}
								else
								{
									if (LLPipeline::sRenderDeferred)
									{
										pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_SIMPLE);
									}
									else
									{
										pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_SHINY);
									}
								}
							}
							else
							{
								if (te->getFullbright())
								{
									pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_FULLBRIGHT);
								}
								else
								{
									pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_SIMPLE);
								}
							}
							if (LLPipeline::sRenderDeferred)
							{
								if (type != LLDrawPool::POOL_ALPHA && !te->getFullbright())
								{
									if (te->getBumpmap())
									{
										pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_DEFERRED_BUMP);
									}
									else
									{
										pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_DEFERRED_SIMPLE);
									}
								}
							}
						}
						}
						else
						{
						if(type == LLDrawPool::POOL_ALPHA)
						{
							if(te->getColor().mV[3] > 0.f)
							{
								U32 mask = te->getFullbright() ? LLDrawPoolAvatar::RIGGED_FULLBRIGHT_ALPHA : LLDrawPoolAvatar::RIGGED_ALPHA;
								if (mat && LLPipeline::sRenderDeferred)
								{
									if(te->getColor().mV[3] < 0.999f || mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_BLEND)
										mask = mat->getShaderMask(LLMaterial::DIFFUSE_ALPHA_MODE_BLEND);
									else
										mask = mat->getShaderMask();
								}
								pool->addRiggedFace(facep, mask);
							}
						}
						else if(!LLPipeline::sRenderDeferred)
						{
							if(type == LLDrawPool::POOL_FULLBRIGHT || type == LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK)
							{
								pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_FULLBRIGHT);
							}
							else if(type == LLDrawPool::POOL_SIMPLE || type == LLDrawPool::POOL_ALPHA_MASK)
							{
								pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_SIMPLE);
							}
							else if(type == LLDrawPool::POOL_BUMP)
							{
								if(te->getShiny())
									pool->addRiggedFace(facep, te->getFullbright() ? LLDrawPoolAvatar::RIGGED_FULLBRIGHT_SHINY : LLDrawPoolAvatar::RIGGED_SHINY);
								else
									pool->addRiggedFace(facep, te->getFullbright() ? LLDrawPoolAvatar::RIGGED_FULLBRIGHT : LLDrawPoolAvatar::RIGGED_SIMPLE);
							}
							else
							{
								pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_SIMPLE);
							}
						}
						else
						{
							if (type == LLDrawPool::POOL_MATERIALS || ((type == LLDrawPool::POOL_ALPHA_MASK || type == LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK) && mat))
							{
								pool->addRiggedFace(facep, mat->getShaderMask());
							}
							else if (type == LLDrawPool::POOL_FULLBRIGHT || type == LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK)
							{
								pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_FULLBRIGHT);
							}
							else if (type == LLDrawPool::POOL_BUMP && te->getBumpmap())
							{
								pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_DEFERRED_BUMP);
							}
							else
							{
								pool->addRiggedFace(facep, LLDrawPoolAvatar::RIGGED_DEFERRED_SIMPLE);
							}
						}
						}
					}
					continue;
				}
				else
				{
					if (facep->isState(LLFace::RIGGED))
					{
						LLDrawPoolAvatar* pool = (LLDrawPoolAvatar*) facep->getPool();
						if (pool)
						{
							pool->removeRiggedFace(facep);
						}
						facep->clearState(LLFace::RIGGED);
					}
				}
				if (cur_total > max_total || facep->getIndicesCount() <= 0 || facep->getGeomCount() <= 0)
				{
					facep->clearVertexBuffer();
					continue;
				}
				cur_total += facep->getGeomCount();
				if (facep->hasGeometry() && facep->getPixelArea() > FORCE_CULL_AREA)
				{
					const LLTextureEntry* te = facep->getTextureEntry();
					LLViewerTexture* tex = facep->getTexture();
					if (te->getGlow() >= 1.f/255.f)
					{
						emissive = true;
					}
					if (facep->isState(LLFace::TEXTURE_ANIM))
					{
						if (!vobj->mTexAnimMode)
						{
							facep->clearState(LLFace::TEXTURE_ANIM);
						}
					}
					bool force_fullbright = group->isHUDGroup();
					BOOL force_simple = (facep->getPixelArea() < FORCE_SIMPLE_RENDER_AREA);
					U32 type = gPipeline.getPoolTypeFromTE(te, tex);
					if(!alt_batching)
					{
					if (type != LLDrawPool::POOL_ALPHA && force_simple)
					{
						type = LLDrawPool::POOL_SIMPLE;
					}
					}
					else
					{
					if (force_fullbright || te->getFullbright())
						facep->setState(LLFace::FULLBRIGHT);
					if ( type == LLDrawPool::POOL_ALPHA )
					{
						if(facep->canRenderAsMask())
						{
							if (facep->isState(LLFace::FULLBRIGHT))
							{
								type = LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK;
							}
							else
							{
								type = LLDrawPool::POOL_ALPHA_MASK;
							}
						}
					}
					else if (force_fullbright)
					{
						LLMaterial* mat = te->getMaterialParams().get();
						if (type == LLDrawPool::POOL_ALPHA_MASK || (mat && mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK ) )
						{
							type = LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK;
						}
						else
						{
							type = LLDrawPool::POOL_FULLBRIGHT;
						}
					}
					else if(force_simple && type != LLDrawPool::POOL_FULLBRIGHT && (!LLPipeline::sRenderDeferred && (type != LLDrawPool::POOL_ALPHA_MASK && type != LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK)))
					{
						type = LLDrawPool::POOL_SIMPLE;
					}
					}
					facep->setPoolType(type);
					if(!alt_batching)
					{
					if (vobj->isHUDAttachment())
					{
						facep->setState(LLFace::FULLBRIGHT);
					}
					}
					if (vobj->mTextureAnimp && vobj->mTexAnimMode)
					{
						if (vobj->mTextureAnimp->mFace <= -1)
						{
							S32 face;
							for (face = 0; face < vobj->getNumTEs(); face++)
							{
								LLFace * facep = drawablep->getFace(face);
								if (facep)
								{
									facep->setState(LLFace::TEXTURE_ANIM);
								}
							}
						}
						else if (vobj->mTextureAnimp->mFace < vobj->getNumTEs())
						{
							LLFace * facep = drawablep->getFace(vobj->mTextureAnimp->mFace);
							if (facep)
							{
								facep->setState(LLFace::TEXTURE_ANIM);
							}
						}
					}
					if(!alt_batching)
					{
					if (type == LLDrawPool::POOL_ALPHA)
 					{
						if (facep->canRenderAsMask())
						{
							if (simple_count < MAX_FACE_COUNT)
							{
								sSimpleFaces[simple_count++] = facep;
							}
						}
						else
						{
							if (te->getColor().mV[3] > 0.f)
							{
								drawablep->setState(LLDrawable::HAS_ALPHA);
							}
							if (alpha_count < MAX_FACE_COUNT)
							{
								sAlphaFaces[alpha_count++] = facep;
							}
						}
					}
					else
					{
						if (drawablep->isState(LLDrawable::REBUILD_VOLUME))
						{
							facep->mLastUpdateTime = gFrameTimeSeconds;
						}
						if (gPipeline.canUseWindLightShadersOnObjects()
							&& LLPipeline::sRenderBump)
						{
							if (LLPipeline::sRenderDeferred && te->getMaterialParams().notNull() )
 							{
								LLMaterial* mat = te->getMaterialParams().get();
								if (mat->getNormalID().notNull())
								{
									if (mat->getSpecularID().notNull())
									{
										if (normspec_count < MAX_FACE_COUNT)
										{
											sNormSpecFaces[normspec_count++] = facep;
										}
									}
									else
									{
										if (norm_count < MAX_FACE_COUNT)
										{
											sNormFaces[norm_count++] = facep;
										}
									}
								}
								else if (mat->getSpecularID().notNull())
								{
									if (spec_count < MAX_FACE_COUNT)
									{
										sSpecFaces[spec_count++] = facep;
									}
								}
								else
								{
									if (simple_count < MAX_FACE_COUNT)
									{
										sSimpleFaces[simple_count++] = facep;
									}
								}
							}
							else if (te->getBumpmap())
							{
								if (bump_count < MAX_FACE_COUNT)
								{
									sBumpFaces[bump_count++] = facep;
								}
							}
							else if (te->getShiny() || !te->getFullbright())
							{
								if (simple_count < MAX_FACE_COUNT)
								{
									sSimpleFaces[simple_count++] = facep;
								}
							}
							else
							{
								facep->setState(LLFace::FULLBRIGHT);
								if (fullbright_count < MAX_FACE_COUNT)
								{
									sFullbrightFaces[fullbright_count++] = facep;
								}
							}
						}
						else
						{
							if (te->getBumpmap() && LLPipeline::sRenderBump)
							{
								if (bump_count < MAX_FACE_COUNT)
								{
									sBumpFaces[bump_count++] = facep;
								}
							}
							else if ((te->getShiny() && LLPipeline::sRenderBump) ||
								!(te->getFullbright()))
							{
								if (simple_count < MAX_FACE_COUNT)
								{
									sSimpleFaces[simple_count++] = facep;
								}
							}
							else
							{
								facep->setState(LLFace::FULLBRIGHT);
								if (fullbright_count < MAX_FACE_COUNT)
								{
									sFullbrightFaces[fullbright_count++] = facep;
								}
							}
						}
					}
					}
					else
					{
					LLFace*** cur_type = NULL;
					U32* cur_count = NULL;
					if (type == LLDrawPool::POOL_ALPHA)
					{
						cur_type = &sAlphaFaces;
						cur_count = &alpha_count;
						if (te->getColor().mV[3] > 0.f)
						{
							drawablep->setState(LLDrawable::HAS_ALPHA);
						}
						LLMaterial* mat = te->getMaterialParams().get();
						if(mat && LLPipeline::sRenderDeferred)
						{
							if (mat->getNormalID().notNull())
							{
								if (mat->getSpecularID().notNull())
								{
									cur_type = &sNormSpecFaces;
									cur_count = &normspec_count;
								}
								else
								{
									cur_type = &sNormFaces;
									cur_count = &norm_count;
								}
							}
							else if (mat->getSpecularID().notNull())
							{
								cur_type = &sSpecFaces;
								cur_count = &spec_count;
							}
						}
					}
					else if(type == LLDrawPool::POOL_ALPHA_MASK)
					{
						cur_type = &sSimpleFaces;
						cur_count = &simple_count;
					}
					else if(type == LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK)
					{
						cur_type = &sFullbrightFaces;
						cur_count = &fullbright_count;
					}
					else
					{
						if (drawablep->isState(LLDrawable::REBUILD_VOLUME))
						{
							facep->mLastUpdateTime = gFrameTimeSeconds;
						}
						if (type == LLDrawPool::POOL_BUMP)
						{
							if(te->getBumpmap() > 0 && te->getBumpmap() < 18)
							{
								cur_type = &sBumpFaces;
								cur_count = &bump_count;
							}
							else if(te->getShiny())
							{
								cur_type = &sSimpleFaces;
								cur_count = &simple_count;
							}
						}
						else if (type == LLDrawPool::POOL_SIMPLE)
						{
							cur_type = &sSimpleFaces;
							cur_count = &simple_count;
						}
						else if (type == LLDrawPool::POOL_FULLBRIGHT)
						{
							if(LLPipeline::sRenderBump && te->getShiny())
							{
								cur_type = &sSimpleFaces;
								cur_count = &simple_count;
							}
							else
							{
								cur_type = &sFullbrightFaces;
								cur_count = &fullbright_count;
							}
 						}
						else if(type == LLDrawPool::POOL_MATERIALS)
						{
							LLMaterial* mat = te->getMaterialParams().get();
							if (mat->getNormalID().notNull())
							{
								if (mat->getSpecularID().notNull())
								{
									cur_type = &sNormSpecFaces;
									cur_count = &normspec_count;
								}
								else
								{
									cur_type = &sNormFaces;
									cur_count = &norm_count;
								}
							}
							else if (mat->getSpecularID().notNull())
							{
								cur_type = &sSpecFaces;
								cur_count = &spec_count;
							}
							else
							{
								cur_type = &sSimpleFaces;
								cur_count = &simple_count;
							}
						}
						else
						{
							LL_ERRS() << "Unknown pool type: " << type << LL_ENDL;
						}
					}
					llassert_always(cur_type);
					if(*cur_count < MAX_FACE_COUNT)
						(*cur_type)[(*cur_count)++] = facep;
					}
				}
				else
				{
					facep->clearVertexBuffer();
				}
			}
			if (any_rigged_face)
			{
				if (!drawablep->isState(LLDrawable::RIGGED))
				{
					drawablep->setState(LLDrawable::RIGGED);
					vobj->updateLOD();
				}
			}
			else
			{
				drawablep->clearState(LLDrawable::RIGGED);
				vobj->updateRiggedVolume();
			}
		}
	}
	group->mBufferUsage = useage;
	U32 simple_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 alpha_mask = simple_mask | 0x80000000;
	U32 bump_mask = simple_mask | LLVertexBuffer::MAP_TEXCOORD1;
	U32 fullbright_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 norm_mask = simple_mask | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TANGENT;
	U32 normspec_mask = norm_mask | LLVertexBuffer::MAP_TEXCOORD2;
	U32 spec_mask = simple_mask | LLVertexBuffer::MAP_TEXCOORD2;
	BOOL batch_textures = LLGLSLShader::sNoFixedFunction;
	U32 additional_flags = 0x0;
	if(batch_textures)
		additional_flags |= LLVertexBuffer::MAP_TEXTURE_INDEX;
	if (LLPipeline::sRenderDeferred)
		bump_mask = norm_mask;
	if(emissive)
		additional_flags |= LLVertexBuffer::MAP_EMISSIVE;
	genDrawInfo(group, simple_mask | additional_flags, sSimpleFaces, simple_count, FALSE, batch_textures);
	genDrawInfo(group, fullbright_mask | additional_flags, sFullbrightFaces, fullbright_count, FALSE, batch_textures);
	genDrawInfo(group, alpha_mask | additional_flags, sAlphaFaces, alpha_count, TRUE, batch_textures);
	genDrawInfo(group, bump_mask | additional_flags, sBumpFaces, bump_count, FALSE);
	genDrawInfo(group, norm_mask | additional_flags, sNormFaces, norm_count, FALSE);
	genDrawInfo(group, spec_mask | additional_flags, sSpecFaces, spec_count, FALSE);
	genDrawInfo(group, normspec_mask | additional_flags, sNormSpecFaces, normspec_count, FALSE);
	if (!LLPipeline::sDelayVBUpdate)
	{
		OctreeGuard guard(group->getOctreeNode());
		for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
		{
			LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
			drawablep->clearState(LLDrawable::REBUILD_ALL);
		}
	}
	group->mLastUpdateTime = gFrameTimeSeconds;
	group->mBuilt = 1.f;
	group->clearState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY);
	if (LLPipeline::sDelayVBUpdate)
	{
		group->setState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
	}
	mFaceList.clear();
}
void LLVolumeGeometryManager::rebuildMesh(LLSpatialGroup* group)
{
	llassert(group);
	static int warningsCount = 20;
	if (group && group->hasState(LLSpatialGroup::MESH_DIRTY) && !group->hasState(LLSpatialGroup::GEOM_DIRTY))
	{
		LL_RECORD_BLOCK_TIME(FTM_REBUILD_VOLUME_VB);
		LL_RECORD_BLOCK_TIME(FTM_REBUILD_VOLUME_GEN_DRAW_INFO);
		group->mBuilt = 1.f;
		OctreeGuard guard(group->getOctreeNode());
		S32 num_mapped_vertex_buffer = LLVertexBuffer::sMappedCount ;
		const U32 MAX_BUFFER_COUNT = 4096;
		static LLVertexBuffer* locked_buffer[MAX_BUFFER_COUNT];
		U32 buffer_count = 0;
		for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
		{
			LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
			if (drawablep && !drawablep->isDead() && drawablep->isState(LLDrawable::REBUILD_ALL) && !drawablep->isState(LLDrawable::RIGGED) )
			{
				LLVOVolume* vobj = drawablep->getVOVolume();
				if (vobj->isNoLOD()) continue;
				vobj->preRebuild();
				if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
				{
					vobj->updateRelativeXform(true);
				}
				LLVolume* volume = vobj->getVolume();
				for (S32 i = 0; i < drawablep->getNumFaces(); ++i)
				{
					LLFace* face = drawablep->getFace(i);
					if (face)
					{
						LLVertexBuffer* buff = face->getVertexBuffer();
						if (buff)
						{
							llassert(!face->isState(LLFace::RIGGED));
							if (!face->getGeometryVolume(*volume, face->getTEOffset(),
								vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), face->getGeomIndex()))
							{
								group->dirtyGeom();
								gPipeline.markRebuild(group, TRUE);
							}
							if (buff->isLocked() && buffer_count < MAX_BUFFER_COUNT)
							{
								locked_buffer[buffer_count++] = buff;
							}
						}
					}
				}
				if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
				{
					vobj->updateRelativeXform();
				}
				drawablep->clearState(LLDrawable::REBUILD_ALL);
			}
		}
		for (LLVertexBuffer** iter = locked_buffer, ** end_iter = locked_buffer+buffer_count; iter != end_iter; ++iter)
		{
			(*iter)->flush();
		}
		if(	group != NULL &&
			!group->mVertexBuffer.isNull() &&
			group->mVertexBuffer->isLocked())
		{
			group->mVertexBuffer->flush();
		}
		if(num_mapped_vertex_buffer != LLVertexBuffer::sMappedCount)
		{
			if (++warningsCount > 20)
			{
				LL_WARNS() << "Not all mapped vertex buffers are unmapped!" << LL_ENDL ;
				warningsCount = 1;
			}
			OctreeGuard guard(group->getOctreeNode());
			for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
			{
				LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
				if(!drawablep)
				{
					continue;
				}
				for (S32 i = 0; i < drawablep->getNumFaces(); ++i)
				{
					LLFace* face = drawablep->getFace(i);
					if (face)
					{
						LLVertexBuffer* buff = face->getVertexBuffer();
						if (buff && buff->isLocked())
						{
							buff->flush();
						}
					}
				}
			}
		}
		group->clearState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
	}
}
struct CompareBatchBreakerModified
{
	bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
	{
		static const LLCachedControl<bool> alt_batching("SHAltBatching",true);
		if(!alt_batching)
		{
		const LLTextureEntry* lte = lhs->getTextureEntry();
 		const LLTextureEntry* rte = rhs->getTextureEntry();
		if (lte->getBumpmap() != rte->getBumpmap())
 		{
			return lte->getBumpmap() < rte->getBumpmap();
 		}
		else if (lte->getFullbright() != rte->getFullbright())
 		{
			return lte->getFullbright() < rte->getFullbright();
 		}
		else if (LLPipeline::sRenderDeferred && lte->getMaterialParams() != rte->getMaterialParams())
 		{
			return lte->getMaterialParams() < rte->getMaterialParams();
 		}
		else if (LLPipeline::sRenderDeferred && (lte->getMaterialParams() == rte->getMaterialParams()) && (lte->getShiny() != rte->getShiny()))
 		{
 			return lte->getShiny() < rte->getShiny();
 		}
 		else
 		{
 			return lhs->getTexture() < rhs->getTexture();
 		}
		}
		else
		{
		const LLTextureEntry* lte = lhs->getTextureEntry();
		const LLTextureEntry* rte = rhs->getTextureEntry();
		bool batch_left = can_batch_texture(lhs);
		bool batch_right = can_batch_texture(rhs);
		if (lhs->getPoolType() != rhs->getPoolType())
		{
			return lhs->getPoolType() < rhs->getPoolType();
		}
		else if(batch_left != batch_right)
		{
			return !(batch_left < batch_right);
		}
		static const LLCachedControl<bool> sh_fullbright_deferred("SHFullbrightDeferred",true);
		bool batch_shiny = (!LLPipeline::sRenderDeferred || (sh_fullbright_deferred && lhs->isState(LLFace::FULLBRIGHT))) && lhs->getPoolType() == LLDrawPool::POOL_BUMP;
		bool batch_fullbright = sh_fullbright_deferred || !LLPipeline::sRenderDeferred && lhs->getPoolType() == LLDrawPool::POOL_ALPHA;
		if (batch_fullbright && lhs->isState(LLFace::FULLBRIGHT) != rhs->isState(LLFace::FULLBRIGHT))
		{
			return lhs->isState(LLFace::FULLBRIGHT) < rhs->isState(LLFace::FULLBRIGHT);
		}
		else if(batch_shiny && !lte->getShiny() != !rte->getShiny())
		{
			return !lte->getShiny() < !rte->getShiny();
		}
		else if(lhs->getPoolType() == LLDrawPool::POOL_MATERIALS && lte->getMaterialParams().get() != rte->getMaterialParams().get())
		{
			return lte->getMaterialParams().get() < rte->getMaterialParams().get();
		}
		else if(lhs->getPoolType() == LLDrawPool::POOL_BUMP && lte->getBumpmap() != rte->getBumpmap())
		{
			return lte->getBumpmap() < rte->getBumpmap();
		}
		else
		{
			return lhs->getTexture() < rhs->getTexture();
		}
		}
	}
};
static LLTrace::BlockTimerStatHandle FTM_GEN_DRAW_INFO_SORT("Draw Info Face Sort");
static LLTrace::BlockTimerStatHandle FTM_GEN_DRAW_INFO_FACE_SIZE("Face Sizing");
static LLTrace::BlockTimerStatHandle FTM_GEN_DRAW_INFO_ALLOCATE("Allocate VB");
static LLTrace::BlockTimerStatHandle FTM_GEN_DRAW_INFO_FIND_VB("Find VB");
static LLTrace::BlockTimerStatHandle FTM_GEN_DRAW_INFO_RESIZE_VB("Resize VB");
void LLVolumeGeometryManager::genDrawInfo(LLSpatialGroup* group, U32 mask, LLFace** faces, U32 face_count, BOOL distance_sort, BOOL batch_textures)
{
	LL_RECORD_BLOCK_TIME(FTM_REBUILD_VOLUME_GEN_DRAW_INFO);
	U32 buffer_usage = group->mBufferUsage;
	static const LLCachedControl<S32> render_max_vbo_size("RenderMaxVBOSize", 512);
	U32 max_vertices = (render_max_vbo_size*1024)/LLVertexBuffer::calcVertexSize(group->getSpatialPartition()->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);
	{
		LL_RECORD_BLOCK_TIME(FTM_GEN_DRAW_INFO_SORT);
		if (!distance_sort)
		{
			std::sort(faces, faces+face_count, CompareBatchBreakerModified());
		}
		else
		{
			std::sort(faces, faces+face_count, LLFace::CompareDistanceGreater());
		}
	}
	bool hud_group = group->isHUDGroup() ;
	LLFace** face_iter = faces;
	LLFace** end_faces = faces+face_count;
	LLSpatialGroup::buffer_vec_t buffer_vec;
	S32 texture_index_channels = llmax(LLGLSLShader::sIndexedTextureChannels-1,1);
	if (LLPipeline::sRenderDeferred && distance_sort)
	{
		texture_index_channels = gDeferredAlphaProgram.mFeatures.mIndexedTextureChannels;
	}
	bool flexi = false;
	while (face_iter != end_faces)
	{
		LLFace* facep = *face_iter;
		LLViewerTexture* tex = facep->getTexture();
		LLMaterialPtr mat = facep->getTextureEntry()->getMaterialParams();
		static const LLCachedControl<bool> alt_batching("SHAltBatching",true);
		if (!alt_batching && distance_sort)
		{
			tex = NULL;
		}
		U32 index_count = facep->getIndicesCount();
		U32 geom_count = facep->getGeomCount();
		flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();
		LLFace** i = face_iter;
		++i;
		if(!alt_batching)
		{
		const U32 MAX_TEXTURE_COUNT = 32;
		static LLViewerTexture* texture_list[MAX_TEXTURE_COUNT];
		U32 texture_count = 0;
		{
			LL_RECORD_BLOCK_TIME(FTM_GEN_DRAW_INFO_FACE_SIZE);
			if (batch_textures)
			{
				U8 cur_tex = 0;
				facep->setTextureIndex(cur_tex);
				if (texture_count < MAX_TEXTURE_COUNT)
				{
					texture_list[texture_count++] = tex;
				}
				if (can_batch_texture(facep))
				{
					while (i != end_faces)
					{
						facep = *i;
						if (!can_batch_texture(facep))
						{
							facep->setTextureIndex(0);
							break;
						}
						if (facep->getTexture() != tex)
						{
							if (distance_sort)
							{
								bool found = false;
								for (U32 tex_idx = 0; tex_idx < texture_count; ++tex_idx)
								{
									if (facep->getTexture() == texture_list[tex_idx])
									{
										cur_tex = tex_idx;
										found = true;
										break;
									}
								}
								if (!found)
								{
									cur_tex = texture_count;
								}
							}
							else
							{
								cur_tex++;
							}
							if (cur_tex >= texture_index_channels)
							{
								break;
							}
							tex = facep->getTexture();
							if (texture_count < MAX_TEXTURE_COUNT)
							{
								texture_list[texture_count++] = tex;
							}
						}
						if (geom_count + facep->getGeomCount() > max_vertices)
						{
							break;
						}
						++i;
						flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();
						index_count += facep->getIndicesCount();
						geom_count += facep->getGeomCount();
						facep->setTextureIndex(cur_tex);
					}
					tex = texture_list[0];
				}
				else
				{
					facep->setTextureIndex(0);
				}
			}
			else
			{
				while (i != end_faces &&
					(LLPipeline::sTextureBindTest ||
						(distance_sort ||
							((*i)->getTexture() == tex &&
							((*i)->getTextureEntry()->getMaterialParams() == mat)))))
				{
					facep = *i;
					facep->mDrawInfo = NULL;
					facep->setTextureIndex(255);
					if (geom_count + facep->getGeomCount() > max_vertices)
					{
						break;
					}
					++i;
					index_count += facep->getIndicesCount();
					geom_count += facep->getGeomCount();
					flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();
				}
				}
			}
		}
		else
		{
			LL_RECORD_BLOCK_TIME(FTM_GEN_DRAW_INFO_FACE_SIZE);
			if (batch_textures)
			{
				facep->setTextureIndex(0);
				if(can_batch_texture(facep))
				{
					static const U8 MAX_TEXTURE_COUNT = 32;
					static LLViewerTexture* texture_list[MAX_TEXTURE_COUNT];
					U8 texture_count = 1;
					U8 cur_tex = 0;
					texture_list[0] = tex;
					U8 pool = facep->getPoolType();
					while (i != end_faces)
					{
						facep = *i;
						if ( !can_batch_texture(facep) || !(facep) || (geom_count + facep->getGeomCount() > max_vertices) || pool != facep->getPoolType() )
						{
							break;
						}
						else
						{
							if(facep->getTexture() != tex)
							{
								bool reused = false;
								if(distance_sort)
								{
									for(U8 j = 0; j < texture_count; ++j)
									{
										if(texture_list[j] == facep->getTexture())
										{
											tex = facep->getTexture();
											cur_tex = j;
											reused = true;
											break;
										}
									}
								}
								if(!reused)
								{
									if(++texture_count > llmin((U8)texture_index_channels,MAX_TEXTURE_COUNT))
										break;
									cur_tex = texture_count - 1;
									tex = facep->getTexture();
									texture_list[cur_tex] = tex;
								}
							}
							facep->setTextureIndex(cur_tex);
							flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();
							index_count += facep->getIndicesCount();
							geom_count += facep->getGeomCount();
						}
						++i;
					}
				}
			}
			else
			{
				facep->mDrawInfo = NULL;
				facep->setTextureIndex(255);
				while (i != end_faces &&
					(LLPipeline::sTextureBindTest ||
						(distance_sort ||
							((*i)->getTexture() == tex &&
							((*i)->getTextureEntry()->getMaterialParams() == mat)))))
				{
					facep = *i;
					if (geom_count + facep->getGeomCount() > max_vertices)
					{
						break;
					}
					facep->mDrawInfo = NULL;
					facep->setTextureIndex(255);
					flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();
					index_count += facep->getIndicesCount();
					geom_count += facep->getGeomCount();
					++i;
				}
			}
		}
		if (flexi && buffer_usage && buffer_usage != GL_STREAM_DRAW_ARB)
		{
			buffer_usage = GL_STREAM_DRAW_ARB;
		}
		if(gDebugGL)
		{
			if (LLPipeline::sRenderDeferred &&
				(*face_iter)->getTextureEntry()->getMaterialParams().get() &&
				((*face_iter)->getPoolType() == LLDrawPool::POOL_ALPHA ||
				(*face_iter)->getPoolType() == LLDrawPool::POOL_MATERIALS ))
			{
				LLMaterial* mat = (*face_iter)->getTextureEntry()->getMaterialParams().get();
				U32 shader_mask;
				if((*face_iter)->getPoolType() == LLDrawPool::POOL_ALPHA)
					shader_mask = mat->getShaderMask(LLMaterial::DIFFUSE_ALPHA_MODE_BLEND);
				else
					shader_mask = mat->getShaderMask();
				if(shader_mask != 1 && shader_mask != 5 && shader_mask != 9 && shader_mask != 13)
				{
					LLGLSLShader* shader = &(gDeferredMaterialProgram[shader_mask]);
					if((mask & shader->mAttributeMask) != shader->mAttributeMask)
					{
						for (U32 i = 0; i < LLVertexBuffer::TYPE_MAX; ++i)
						{
							U32 attrib_mask = 1 << i;
							if((shader->mAttributeMask & attrib_mask) && !(mask & attrib_mask))
							{
								const std::string& type_name = LLVertexBuffer::getTypeName(i);
								LL_WARNS() << " Missing: " << type_name <<  LL_ENDL;
							}
						}
						LL_ERRS() << "Face VBO missing required materials attributes." << LL_ENDL;
					}
				}
			}
		}
		LLVertexBuffer* buffer = NULL;
		{
			LL_RECORD_BLOCK_TIME(FTM_GEN_DRAW_INFO_ALLOCATE);
			buffer = createVertexBuffer(mask, buffer_usage);
			buffer->allocateBuffer(geom_count, index_count, TRUE);
		}
		group->mGeometryBytes += buffer->getSize() + buffer->getIndicesSize();
		get_val_in_pair_vec(get_val_in_pair_vec(buffer_vec, mask),*face_iter).push_back(buffer);
		U32 indices_index = 0;
		U16 index_offset = 0;
		while (face_iter < i)
		{
			facep = *face_iter;
			facep->setIndicesIndex(indices_index);
			facep->setGeomIndex(index_offset);
			facep->setVertexBuffer(buffer);
			if (batch_textures && facep->getTextureIndex() == 255)
			{
				LL_ERRS() << "Invalid texture index." << LL_ENDL;
			}
			{
				facep->updateRebuildFlags();
			}
			facep->mShinyInAlpha = false;
			static const LLCachedControl<bool> alt_batching("SHAltBatching",true);
			if(!alt_batching)
			{
			BOOL force_simple = facep->getPixelArea() < FORCE_SIMPLE_RENDER_AREA;
			BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
			if ((mask & LLVertexBuffer::MAP_NORMAL) == 0)
			{
				fullbright = TRUE;
			}
			if (hud_group)
			{
				fullbright = TRUE;
			}
			const LLTextureEntry* te = facep->getTextureEntry();
			tex = facep->getTexture();
			BOOL is_alpha = (facep->getPoolType() == LLDrawPool::POOL_ALPHA) ? TRUE : FALSE;
			LLMaterial* mat = te->getMaterialParams().get();
			bool can_be_shiny = true;
			if (mat)
			{
				U8 mode = mat->getDiffuseAlphaMode();
				can_be_shiny = mode == LLMaterial::DIFFUSE_ALPHA_MODE_NONE ||
								mode == LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE;
			}
			bool use_legacy_bump = te->getBumpmap() && (te->getBumpmap() < 18) && (!mat || mat->getNormalID().isNull());
			bool opaque = te->getColor().mV[3] >= 0.999f;
			if (mat && LLPipeline::sRenderDeferred && !hud_group)
			{
				bool material_pass = false;
				if (te->getFullbright())
				{
					if (mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
					{
						if (opaque)
						{
							registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
						}
						else
						{
							registerFace(group, facep, LLRenderPass::PASS_ALPHA);
						}
					}
					else if (is_alpha)
					{
						registerFace(group, facep, LLRenderPass::PASS_ALPHA);
					}
					else
					{
						if (mat->getEnvironmentIntensity() > 0 ||
							te->getShiny() > 0)
						{
							material_pass = true;
						}
						else
						{
							registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT);
						}
					}
				}
				else if (!opaque)
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}
				else if (use_legacy_bump)
				{
					registerFace(group, facep, LLRenderPass::PASS_BUMP);
				}
				else
				{
					material_pass = true;
				}
				if (material_pass)
				{
					static const U32 pass[] =
					{
						LLRenderPass::PASS_MATERIAL,
						LLRenderPass::PASS_ALPHA,
						LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
						LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
						LLRenderPass::PASS_SPECMAP,
						LLRenderPass::PASS_ALPHA,
						LLRenderPass::PASS_SPECMAP_MASK,
						LLRenderPass::PASS_SPECMAP_EMISSIVE,
						LLRenderPass::PASS_NORMMAP,
						LLRenderPass::PASS_ALPHA,
						LLRenderPass::PASS_NORMMAP_MASK,
						LLRenderPass::PASS_NORMMAP_EMISSIVE,
						LLRenderPass::PASS_NORMSPEC,
						LLRenderPass::PASS_ALPHA,
						LLRenderPass::PASS_NORMSPEC_MASK,
						LLRenderPass::PASS_NORMSPEC_EMISSIVE,
					};
					U32 mask = mat->getShaderMask();
					llassert(mask < sizeof(pass)/sizeof(U32));
					mask = llmin(mask, (U32)(sizeof(pass)/sizeof(U32)-1));
					registerFace(group, facep, pass[mask]);
				}
			}
			else if (mat)
			{
				U8 mode = mat->getDiffuseAlphaMode();
				if (te->getColor().mV[3] < 0.999f)
				{
					mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
				}
				if (mode == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
				{
					registerFace(group, facep, fullbright ? LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK : LLRenderPass::PASS_ALPHA_MASK);
				}
				else if (is_alpha || (te->getColor().mV[3] < 0.999f))
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}
				else if (LLGLSLShader::sNoFixedFunction
					&& LLPipeline::sRenderBump
					&& te->getShiny()
					&& can_be_shiny)
				{
					registerFace(group, facep, fullbright ? LLRenderPass::PASS_FULLBRIGHT_SHINY : LLRenderPass::PASS_SHINY);
				}
				else
				{
					registerFace(group, facep, fullbright ? LLRenderPass::PASS_FULLBRIGHT : LLRenderPass::PASS_SIMPLE);
				}
			}
			else if (is_alpha)
			{
				if (facep->getFaceColor().mV[3] <= 0.f)
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA_INVISIBLE);
				}
				else if (facep->canRenderAsMask())
				{
					if (te->getFullbright() || fullbright || LLPipeline::sNoAlpha)
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
					}
					else
					{
						registerFace(group, facep, LLRenderPass::PASS_ALPHA_MASK);
					}
				}
				else
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}
			}
			else if (LLGLSLShader::sNoFixedFunction
				&& LLPipeline::sRenderBump
				&& te->getShiny()
				&& can_be_shiny)
			{
				if (LLPipeline::sRenderDeferred && !hud_group)
				{
					if (te->getFullbright())
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
						if (te->getBumpmap())
						{
							registerFace(group, facep, LLRenderPass::PASS_POST_BUMP);
						}
					}
					else if (use_legacy_bump)
					{
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else
					{
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
					}
				}
				else if (fullbright)
				{
					registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
				}
				else
				{
					registerFace(group, facep, LLRenderPass::PASS_SHINY);
				}
			}
			else
			{
				if (fullbright)
				{
					if (mat && mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
					}
					else
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT);
					}
					if (LLPipeline::sRenderDeferred && !hud_group && LLPipeline::sRenderBump && use_legacy_bump)
					{
						registerFace(group, facep, LLRenderPass::PASS_POST_BUMP);
					}
				}
				else
				{
					if (LLPipeline::sRenderDeferred && LLPipeline::sRenderBump && use_legacy_bump)
					{
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else
					{
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						if (mat && mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
						{
							registerFace(group, facep, LLRenderPass::PASS_ALPHA_MASK);
						}
						else
						{
							registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
						}
					}
				}
				if (!LLGLSLShader::sNoFixedFunction &&
					!is_alpha &&
					te->getShiny() &&
					LLPipeline::sRenderBump)
				{
					registerFace(group, facep, LLRenderPass::PASS_SHINY);
				}
			}
			if (!is_alpha && (hud_group || !LLPipeline::sRenderDeferred))
			{
				llassert((mask & LLVertexBuffer::MAP_NORMAL) || fullbright);
				facep->setPoolType((fullbright) ? LLDrawPool::POOL_FULLBRIGHT : LLDrawPool::POOL_SIMPLE);
				if (!force_simple && LLPipeline::sRenderBump && use_legacy_bump)
				{
					registerFace(group, facep, LLRenderPass::PASS_BUMP);
				}
			}
			if (!is_alpha && LLPipeline::sRenderGlow && te->getGlow() > 0.f)
			{
				registerFace(group, facep, LLRenderPass::PASS_GLOW);
			}
			}
			else
			{
			const LLTextureEntry* te = facep->getTextureEntry();
			tex = facep->getTexture();
			bool is_alpha =			facep->getPoolType() == LLDrawPool::POOL_ALPHA;
			bool is_shiny_shader =	facep->getPoolType() == LLDrawPool::POOL_BUMP && LLGLSLShader::sNoFixedFunction && te->getShiny();
			bool is_shiny_fixed =	facep->getPoolType() == LLDrawPool::POOL_BUMP && !LLGLSLShader::sNoFixedFunction && te->getShiny();
			bool is_fullbright =	facep->isState(LLFace::FULLBRIGHT);
			if (facep->getPoolType() == LLDrawPool::POOL_MATERIALS)
			{
				U32 pass[] =
				{
					LLRenderPass::PASS_MATERIAL,
					LLRenderPass::PASS_ALPHA,
					LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
					LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
					LLRenderPass::PASS_SPECMAP,
					LLRenderPass::PASS_ALPHA,
					LLRenderPass::PASS_SPECMAP_MASK,
					LLRenderPass::PASS_SPECMAP_EMISSIVE,
					LLRenderPass::PASS_NORMMAP,
					LLRenderPass::PASS_ALPHA,
					LLRenderPass::PASS_NORMMAP_MASK,
					LLRenderPass::PASS_NORMMAP_EMISSIVE,
					LLRenderPass::PASS_NORMSPEC,
					LLRenderPass::PASS_ALPHA,
					LLRenderPass::PASS_NORMSPEC_MASK,
					LLRenderPass::PASS_NORMSPEC_EMISSIVE,
				};
				U32 mask = mat->getShaderMask();
				llassert(mask < sizeof(pass)/sizeof(U32));
				mask = llmin(mask, (U32)(sizeof(pass)/sizeof(U32)-1));
				registerFace(group, facep, pass[mask]);
			}
			else if (facep->getPoolType() == LLDrawPool::POOL_ALPHA)
			{
				if (facep->getFaceColor().mV[3] <= 0.f)
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA_INVISIBLE);
				}
				else
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}
			}
			else if (facep->getPoolType() == LLDrawPool::POOL_ALPHA_MASK)
			{
				registerFace(group, facep, LLRenderPass::PASS_ALPHA_MASK);
			}
			else if (facep->getPoolType() == LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK)
			{
				registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
			}
			else
			{
				if (facep->getPoolType() == LLDrawPool::POOL_SIMPLE)
				{
					registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
				}
				else if (facep->getPoolType() == LLDrawPool::POOL_FULLBRIGHT)
				{
					registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT);
				}
				else if (facep->getPoolType() == LLDrawPool::POOL_BUMP)
				{
					llassert_always(mask & LLVertexBuffer::MAP_NORMAL);
					bool is_bump = te->getBumpmap() > 0 && te->getBumpmap() < 18;
					if(is_bump && LLPipeline::sRenderDeferred)
					{
						llassert_always(mask & LLVertexBuffer::MAP_TANGENT);
					}
					if(is_shiny_shader || is_shiny_fixed)
					{
						llassert_always(mask & LLVertexBuffer::MAP_NORMAL);
					}
					if(LLPipeline::sRenderDeferred)
					{
						static const LLCachedControl<bool> sh_fullbright_deferred("SHFullbrightDeferred",true);
						if(sh_fullbright_deferred && is_fullbright)
						{
							registerFace(group, facep, is_shiny_shader ? LLRenderPass::PASS_FULLBRIGHT_SHINY : LLRenderPass::PASS_FULLBRIGHT);
							if(is_bump)
							{
								registerFace(group, facep, LLRenderPass::PASS_POST_BUMP);
							}
						}
						else
						{
							registerFace(group, facep, is_bump ? LLRenderPass::PASS_BUMP : LLRenderPass::PASS_SIMPLE);
						}
					}
					else
					{
						if (is_fullbright )
						{
							registerFace(group, facep, is_shiny_shader ? LLRenderPass::PASS_FULLBRIGHT_SHINY : LLRenderPass::PASS_FULLBRIGHT);
						}
						else
						{
							registerFace(group, facep, is_shiny_shader ? LLRenderPass::PASS_SHINY : LLRenderPass::PASS_SIMPLE);
						}
						if(is_bump)
							registerFace(group, facep, LLRenderPass::PASS_BUMP);
						if(is_shiny_fixed)
							registerFace(group, facep, LLRenderPass::PASS_SHINY);
					}
				}
			}
			if (!is_alpha && LLPipeline::sRenderGlow && te->getGlow() > 0.f)
			{
				registerFace(group, facep, LLRenderPass::PASS_GLOW);
			}
			}
			if (!LLPipeline::sDelayVBUpdate)
			{
				LLDrawable* drawablep = facep->getDrawable();
				LLVOVolume* vobj = drawablep->getVOVolume();
				LLVolume* volume = vobj->getVolume();
				if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
				{
					vobj->updateRelativeXform(true);
				}
				U32 te_idx = facep->getTEOffset();
				llassert(!facep->isState(LLFace::RIGGED));
				if (!facep->getGeometryVolume(*volume, te_idx,
					vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), index_offset,true))
				{
					LL_WARNS() << "Failed to get geometry for face!" << LL_ENDL;
				}
				if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
				{
					vobj->updateRelativeXform(false);
				}
			}
			index_offset += facep->getGeomCount();
			indices_index += facep->getIndicesCount();
			++face_iter;
		}
		if(index_offset > 0)
		{
			buffer->validateRange(0,  index_offset - 1, indices_index, 0);
		}
		buffer->flush();
	}
	auto buffVec = get_val_in_pair_vec(group->mBufferVec, mask);
	buffVec.clear();
	auto map = get_val_in_pair_vec(buffer_vec, mask);
	for (LLSpatialGroup::buffer_texture_vec_t::iterator i = map.begin(); i != map.end(); ++i)
	{
		get_val_in_pair_vec(buffVec, i->first) = i->second;
	}
}
void LLGeometryManager::addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32 &index_count)
{
	U32 usage = group->getSpatialPartition()->mBufferUsage;
	mFaceList.clear();
	OctreeGuard guard(group->getOctreeNode());
	for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
	{
		LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
		if (drawablep->isDead())
		{
			continue;
		}
		if (drawablep->isAnimating())
		{
			usage = GL_STREAM_DRAW_ARB;
		}
		for (S32 i = 0; i < drawablep->getNumFaces(); i++)
		{
			drawablep->updateFaceSize(i);
			LLFace* facep = drawablep->getFace(i);
			if (facep)
			{
				if (facep->hasGeometry() && facep->getPixelArea() > FORCE_CULL_AREA &&
					facep->getGeomCount() + vertex_count <= 65536)
				{
					vertex_count += facep->getGeomCount();
					index_count += facep->getIndicesCount();
					mFaceList.push_back(facep);
				}
				else
				{
					facep->clearVertexBuffer();
				}
			}
		}
	}
	group->mBufferUsage = usage;
}
LLHUDPartition::LLHUDPartition(LLViewerRegion* regionp)
	: LLBridgePartition(regionp)
{
	mPartitionType = LLViewerRegion::PARTITION_HUD;
	mDrawableType = LLPipeline::RENDER_TYPE_HUD;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}
