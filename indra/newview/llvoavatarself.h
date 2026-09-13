/**
 * @file llvoavatarself.h
 * @brief Declaration of LLVOAvatar class which is a derivation fo
 * LLViewerObject
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
#ifndef LL_LLVOAVATARSELF_H
#define LL_LLVOAVATARSELF_H
#include "llavatarappearancedefines.h"
#include "llviewertexture.h"
#include "llvoavatar.h"
#include <map>
struct LocalTextureData;
class LLInventoryCallback;
class LLVOAvatarSelf :
	public LLVOAvatar
{
	LOG_CLASS(LLVOAvatarSelf);
public:
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
	LLVOAvatarSelf(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual 				~LLVOAvatarSelf();
	virtual void			markDead();
	virtual void 			initInstance();
	void					buildContextMenus();
	void					cleanup();
protected:
	BOOL		loadAvatar();
	BOOL					loadAvatarSelf();
	BOOL					buildSkeletonSelf(const LLAvatarSkeletonInfo *info);
	BOOL					buildMenus();
public:
	boost::signals2::connection                   mRegionChangedSlot;
	void					onSimulatorFeaturesReceived(const LLUUID& region_id);
	void 		updateRegion(LLViewerRegion *regionp);
	void   	 	idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
public:
	bool 		hasMotionFromSource(const LLUUID& source_id);
	void 		stopMotionFromSource(const LLUUID& source_id);
	void 		requestStopMotion(LLMotion* motion);
	LLJoint*	getJoint(const std::string &name);
	BOOL setVisualParamWeight(const LLVisualParam *which_param, F32 weight, bool upload_bake = false );
	BOOL setVisualParamWeight(const char* param_name, F32 weight, bool upload_bake = false );
	BOOL setVisualParamWeight(S32 index, F32 weight, bool upload_bake = false );
	void updateVisualParams();
	void writeWearablesToAvatar();
	void idleUpdateAppearanceAnimation();
private:
	BOOL setParamWeight(const LLViewerVisualParam *param, F32 weight, bool upload_bake = false );
private:
	LLUUID mInitialBakeIDs[LLAvatarAppearanceDefines::BAKED_NUM_INDICES];
public:
	bool 	isSelf() const { return true; }
	BOOL	isValid() const;
public:
	BOOL 	updateCharacter(LLAgent &agent);
	void 	idleUpdateTractorBeam();
	bool				checkStuckAppearance();
public:
	BOOL    getIsCloud() const;
	void			resetRegionCrossingTimer()	{ mRegionCrossingTimer.reset();	}
private:
	U64				mLastRegionHandle;
	LLFrameTimer	mRegionCrossingTimer;
	S32				mRegionCrossingCount;
protected:
	BOOL 		needsRenderBeam();
private:
	LLPointer<LLHUDEffectSpiral> mBeam;
	LLFrameTimer mBeamTimer;
public:
	LLViewerTexture::EBoostLevel 	getAvatarBoostLevel() const { return LLGLTexture::BOOST_AVATAR_SELF; }
	LLViewerTexture::EBoostLevel 	getAvatarBakedBoostLevel() const { return LLGLTexture::BOOST_AVATAR_BAKED_SELF; }
	S32 						getTexImageSize() const { return LLVOAvatar::getTexImageSize()*4; }
public:
	bool	hasPendingBakedUploads() const;
	S32					getLocalDiscardLevel(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
	bool				areTexturesCurrent() const;
	BOOL				isLocalTextureDataAvailable(const LLViewerTexLayerSet* layerset) const;
	BOOL				isLocalTextureDataFinal(const LLViewerTexLayerSet* layerset) const;
	BOOL				isBakedTextureFinal(const LLAvatarAppearanceDefines::EBakedTextureIndex index) const;
	BOOL    isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
	BOOL	isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, U32 index = 0) const;
	BOOL	isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerWearable *wearable) const;
public:
	BOOL				getLocalTextureGL(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerTexture** image_gl_pp, U32 index) const;
	LLViewerFetchedTexture*	getLocalTextureGL(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
	const LLUUID&		getLocalTextureID(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
	void				setLocalTextureTE(U8 te, LLViewerTexture* image, U32 index);
	void	setLocalTexture(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerTexture* tex, BOOL baked_version_exits, U32 index);
protected:
	void	setBakedReady(LLAvatarAppearanceDefines::ETextureIndex type, BOOL baked_version_exists, U32 index);
	void				localTextureLoaded(BOOL succcess, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	void				getLocalTextureByteCount(S32* gl_byte_count) const;
	void	addLocalTextureStats(LLAvatarAppearanceDefines::ETextureIndex i, LLViewerFetchedTexture* imagep, F32 texel_area_ratio, BOOL rendered, BOOL covered_by_baked);
	LLLocalTextureObject* getLocalTextureObject(LLAvatarAppearanceDefines::ETextureIndex i, U32 index) const;
private:
	static void			onLocalTextureLoaded(BOOL succcess, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	void	setImage(const U8 te, LLViewerTexture *imagep, const U32 index);
	LLViewerTexture* getImage(const U8 te, const U32 index) const;
public:
	LLAvatarAppearanceDefines::ETextureIndex getBakedTE(const LLViewerTexLayerSet* layerset ) const;
	void				setNewBakedTexture(LLAvatarAppearanceDefines::EBakedTextureIndex i, const LLUUID &uuid);
	void				setNewBakedTexture(LLAvatarAppearanceDefines::ETextureIndex i, const LLUUID& uuid);
	void				setCachedBakedTexture(LLAvatarAppearanceDefines::ETextureIndex i, const LLUUID& uuid);
	void				forceBakeAllTextures(bool slam_for_debug = false);
	static void			processRebakeAvatarTextures(LLMessageSystem* msg, void**);
protected:
	void	removeMissingBakedTextures();
public:
	void 				requestLayerSetUploads();
	void				requestLayerSetUpload(LLAvatarAppearanceDefines::EBakedTextureIndex i);
	void				requestLayerSetUpdate(LLAvatarAppearanceDefines::ETextureIndex i);
	LLViewerTexLayerSet* getLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;
	LLViewerTexLayerSet* getLayerSet(LLAvatarAppearanceDefines::ETextureIndex index) const;
public:
	void	invalidateComposite(LLTexLayerSet* layerset, BOOL upload_result);
	void	invalidateAll();
	void	setCompositeUpdatesEnabled(bool b);
	void  setCompositeUpdatesEnabled(U32 index, bool b);
	bool 	isCompositeUpdateEnabled(U32 index);
	void				setupComposites();
	void				updateComposites();
	const LLUUID&		grabBakedTexture(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;
	BOOL				canGrabBakedTexture(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;
protected:
	void   restoreMeshData();
public:
	void				wearableUpdated(LLWearableType::EType type, BOOL upload_result);
protected:
	U32 getNumWearables(LLAvatarAppearanceDefines::ETextureIndex i) const;
public:
	void 				updateAttachmentVisibility(U32 camera_mode);
	BOOL 				isWearingAttachment(const LLUUID& inv_item_id) const;
	LLViewerObject* 	getWornAttachment(const LLUUID& inv_item_id);
	bool				getAttachedPointName(const LLUUID& inv_item_id, std::string& name) const;
	LLViewerJointAttachment* getWornAttachmentPoint(const LLUUID& inv_item_id) const;
	const LLViewerJointAttachment *attachObject(LLViewerObject *viewer_object);
	BOOL 	detachObject(LLViewerObject *viewer_object);
	static BOOL			detachAttachmentIntoInventory(const LLUUID& item_id);
	enum EAttachAction { ACTION_ATTACH, ACTION_DETACH };
	typedef boost::signals2::signal<void (LLViewerObject*, const LLViewerJointAttachment*, EAttachAction)> attachment_signal_t;
	boost::signals2::connection setAttachmentCallback(const attachment_signal_t::slot_type& cb);
private:
	attachment_signal_t* mAttachmentSignal;
private:
	LLViewerJoint* 		mScreenp;
public:
	static void		onCustomizeStart(bool disable_camera_switch = false);
	static void		onCustomizeEnd(bool disable_camera_switch = false);
	LLPointer<LLInventoryCallback> mEndCustomizeCallback;
	bool shouldRenderRigged() const;
public:
	bool			sendAppearanceMessage(LLMessageSystem *mesgsys) const;
	void 			setHoverIfRegionEnabled(bool send_update = true);
	void			sendHoverHeight() const;
	void setHoverOffset(const LLVector3& hover_offset, bool send_update=true);
private:
	mutable LLVector3 mLastHoverOffsetSent;
public:
	LLVector3		getLegacyAvatarOffset() const;
public:
	static void		dumpTotalLocalTextureByteCount();
	void			dumpLocalTextures() const;
	void			dumpWearableInfo(LLAPRFile& outfile);
public:
	struct LLAvatarTexData
	{
		LLAvatarTexData(const LLUUID& id, LLAvatarAppearanceDefines::ETextureIndex index) :
			mAvatarID(id),
			mIndex(index)
		{}
		LLUUID			mAvatarID;
		LLAvatarAppearanceDefines::ETextureIndex	mIndex;
	};
	LLTimer					mTimeSinceLastRezMessage;
	bool					updateAvatarRezMetrics(bool force_send);
	std::vector<LLSD>		mPendingTimerRecords;
	void 					addMetricsTimerRecord(const LLSD& record);
	void 					debugWearablesLoaded() { mDebugTimeWearablesLoaded = mDebugSelfLoadTimer.getElapsedTimeF32(); }
	void 					debugAvatarVisible() { mDebugTimeAvatarVisible = mDebugSelfLoadTimer.getElapsedTimeF32(); }
	void 					outputRezDiagnostics() const;
	void					outputRezTiming(const std::string& msg) const;
	void					reportAvatarRezTime() const;
	void 					debugBakedTextureUpload(LLAvatarAppearanceDefines::EBakedTextureIndex index, BOOL finished);
	static void				debugOnTimingLocalTexLoaded(BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	BOOL					isAllLocalTextureDataFinal() const;
	const LLViewerTexLayerSet*	debugGetLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex index) const { return (LLViewerTexLayerSet*)(mBakedTextureDatas[index].mTexLayerSet); }
	const std::string		verboseDebugDumpLocalTextureDataInfo(const LLViewerTexLayerSet* layerset) const;
	void					dumpAllTextures() const;
	const std::string		debugDumpLocalTextureDataInfo(const LLViewerTexLayerSet* layerset) const;
	const std::string		debugDumpAllLocalTextureDataInfo() const;
	void					sendViewerAppearanceChangeMetrics();
	void 					checkForUnsupportedServerBakeAppearance();
private:
	LLFrameTimer    		mDebugSelfLoadTimer;
	F32						mDebugTimeWearablesLoaded;
	F32 					mDebugTimeAvatarVisible;
	F32 					mDebugTextureLoadTimes[LLAvatarAppearanceDefines::TEX_NUM_INDICES][MAX_DISCARD_LEVEL+1];
	F32 					mDebugBakedTextureTimes[LLAvatarAppearanceDefines::BAKED_NUM_INDICES][2];
	void					debugTimingLocalTexLoaded(BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
public:
	static void onChangeSelfInvisible(bool invisible);
	void setInvisible(bool invisible);
private:
	void handleTeleportFinished();
private:
	boost::signals2::connection mTeleportFinishedSlot;
};
extern LLPointer<LLVOAvatarSelf> gAgentAvatarp;
BOOL isAgentAvatarValid();
void dump_hud_teleport_state(const char* where);
void log_hud_event(const char* event, const LLViewerObject* obj);
void start_hud_teleport_logging();
bool hud_teleport_logging_active();
void hud_teleport_logging_on_teleport_none();
void selfStartPhase(const std::string& phase_name);
void selfStopPhase(const std::string& phase_name, bool err_check = true);
void selfClearPhases();
#endif
