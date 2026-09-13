/**
 * @file llvoavatar.h
 * @brief Declaration of LLVOAvatar class which is a derivation of
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
#ifndef LL_VOAVATAR_H
#define LL_VOAVATAR_H
#include <map>
#include <deque>
#include <string>
#include <vector>
#include <boost/signals2.hpp>
#include "imageids.h"
#include "llavatarappearance.h"
#include "llchat.h"
#include "lldrawpoolalpha.h"
#include "llviewerobject.h"
#include "llcharacter.h"
#include "llcontrol.h"
#include "llviewerjointmesh.h"
#include "llviewerjointattachment.h"
#include "llrendertarget.h"
#include "llavatarappearancedefines.h"
#include "lltexglobalcolor.h"
#include "lldriverparam.h"
#include "llviewertexlayer.h"
#include "material_codes.h"
#include "llrigginginfo.h"
#include "llviewerstats.h"
#include "llvovolume.h"
#include "llavatarname.h"
#if 0
extern const LLUUID ANIM_AGENT_BODY_NOISE;
extern const LLUUID ANIM_AGENT_BREATHE_ROT;
extern const LLUUID ANIM_AGENT_PHYSICS_MOTION;
extern const LLUUID ANIM_AGENT_EDITING;
extern const LLUUID ANIM_AGENT_EYE;
extern const LLUUID ANIM_AGENT_FLY_ADJUST;
extern const LLUUID ANIM_AGENT_HAND_MOTION;
extern const LLUUID ANIM_AGENT_HEAD_ROT;
extern const LLUUID ANIM_AGENT_PELVIS_FIX;
extern const LLUUID ANIM_AGENT_TARGET;
extern const LLUUID ANIM_AGENT_WALK_ADJUST;
#endif
class LLAPRFile;
class LLViewerWearable;
class LLVoiceVisualizer;
class LLHUDNameTag;
class LLHUDEffectSpiral;
class LLTexGlobalColor;
class LLViewerJoint;
struct LLAppearanceMessageContents;
class LLMeshSkinInfo;
class LLViewerJointMesh;
class LLControlAvatar;
class SHClientTagMgr : public LLSingleton<SHClientTagMgr>, public boost::signals2::trackable
{
public:
	SHClientTagMgr();
	bool fetchDefinitions() const;
	bool parseDefinitions();
private:
	void updateAgentAvatarTag();
	const LLSD generateClientTag(const LLVOAvatar* pAvatar) const;
public:
	void updateAvatarTag(LLVOAvatar* pAvatar);
	void clearAvatarTag(const LLVOAvatar* pAvatar);
	bool getIsEnabled() const;
	const std::string getClientName(const LLVOAvatar* pAvatar, bool is_friend) const;
	const LLUUID getClientID(const LLVOAvatar* pAvatar) const;
	bool getClientColor(const LLVOAvatar* pAvatar, bool check_status, LLColor4& color) const;
private:
	std::map<LLUUID, LLSD> mClientDefinitions;
	std::map<LLUUID, LLSD> mAvatarTags;
};
class LLVOAvatar :
	public LLAvatarAppearance,
	public LLViewerObject,
	public boost::signals2::trackable
{
	LOG_CLASS(LLVOAvatar);
public:
	friend class LLVOAvatarSelf;
public:
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
	LLVOAvatar(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual void		markDead();
	static void			initClass();
	static void			cleanupClass();
	virtual void 		initInstance();
protected:
	virtual				~LLVOAvatar();
public:
	void			updateGL();
	LLVOAvatar*		asAvatar();
	virtual U32    	 	 	processUpdateMessage(LLMessageSystem *mesgsys,
													 void **user_data,
													 U32 block_num,
													 const EObjectUpdateType update_type,
													 LLDataPacker *dp);
	virtual void   	 	 	idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	BOOL   	 	 	updateLOD();
	BOOL  	 	 	 	 	updateJointLODs();
	void						updateLODRiggedAttachments( void );
	void						updateSoftwareSkinnedVertices(const LLMeshSkinInfo* skin, const LLVector4a* weight, const LLVolumeFace& vol_face, LLVertexBuffer *buffer);
	BOOL   	 	 	isActive() const;
	S32Bytes					totalTextureMemForUUIDS(uuid_set_t& ids);
	bool 						allTexturesCompletelyDownloaded(uuid_set_t& ids) const;
	bool 						allLocalTexturesCompletelyDownloaded() const;
	bool 						allBakedTexturesCompletelyDownloaded() const;
	void 						bakedTextureOriginCounts(S32 &sb_count, S32 &host_count,
														 S32 &both_count, S32 &neither_count);
	std::string 				bakedTextureOriginInfo();
	void 						collectLocalTextureUUIDs(uuid_set_t& ids) const;
	void 						collectBakedTextureUUIDs(uuid_set_t& ids) const;
	void 						collectTextureUUIDs(uuid_set_t& ids);
	void						releaseOldTextures();
	void   	 	 	updateTextures();
	LLViewerFetchedTexture*		getBakedTextureImage(const U8 te, const LLUUID& uuid);
	LLViewerTexture*			getBakedTextureForAttachment(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index);
	S32    	 	 	setTETexture(const U8 te, const LLUUID& uuid);
	void   	 	 	onShift(const LLVector4a& shift_vector);
	U32    	 	 	getPartitionType() const;
	const  	 	 	LLVector3 getRenderPosition() const;
	void   	 	 	updateDrawable(BOOL force_damped);
	LLDrawable* 	createDrawable(LLPipeline *pipeline);
	BOOL   	 	 	updateGeometry(LLDrawable *drawable);
	void   	 	 	setPixelAreaAndAngle(LLAgent &agent);
	void   	 	 	updateRegion(LLViewerRegion *regionp);
	void   	 	 	updateSpatialExtents(LLVector4a& newMin, LLVector4a &newMax);
	void			   	 	 	calculateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax);
	BOOL   	 	 	lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
												 S32 face = -1,
												 BOOL pick_transparent = FALSE,
												 BOOL pick_rigged = FALSE,
												 S32* face_hit = NULL,
												 LLVector4a* intersection = NULL,
												 LLVector2* tex_coord = NULL,
												 LLVector4a* normal = NULL,
												 LLVector4a* tangent = NULL);
	virtual LLViewerObject*	lineSegmentIntersectRiggedAttachments(
                                                 const LLVector4a& start, const LLVector4a& end,
												 S32 face = -1,
												 BOOL pick_transparent = FALSE,
												 BOOL pick_rigged = FALSE,
												 S32* face_hit = NULL,
												 LLVector4a* intersection = NULL,
												 LLVector2* tex_coord = NULL,
												 LLVector4a* normal = NULL,
												 LLVector4a* tangent = NULL);
public:
	LLVector3    	getCharacterPosition();
	LLQuaternion 	getCharacterRotation();
	LLVector3    	getCharacterVelocity();
	LLVector3    	getCharacterAngularVelocity();
	LLUUID			remapMotionID(const LLUUID& id);
	BOOL			startMotion(const LLUUID& id, F32 time_offset = 0.f);
	BOOL			stopMotion(const LLUUID& id, BOOL stop_immediate = FALSE);
	void						startMotion(U32 bit, F32 start_offset = 0.f);
	void						stopMotion(U32 bit, BOOL stop_immediate = FALSE);
	virtual bool			hasMotionFromSource(const LLUUID& source_id);
	virtual void			stopMotionFromSource(const LLUUID& source_id);
	virtual void			requestStopMotion(LLMotion* motion);
	LLMotion*				findMotion(const LLUUID& id) const;
	void					startDefaultMotions();
	void					dumpAnimationState();
	virtual LLJoint*		getJoint(const std::string &name);
	LLJoint*				getJoint(S32 num);
	void 					addAttachmentOverridesForObject(LLViewerObject *vo, uuid_set_t* meshes_seen = NULL, bool recursive = true);
	void					removeAttachmentOverridesForObject(const LLUUID& mesh_id);
	void					removeAttachmentOverridesForObject(LLViewerObject *vo);
	bool					jointIsRiggedTo(const LLJoint *joint) const;
	void					clearAttachmentOverrides();
	void					rebuildAttachmentOverrides();
	void					updateAttachmentOverrides();
	void					showAttachmentOverrides(bool verbose = false) const;
	void					getAttachmentOverrideNames(	std::set<std::string>& pos_names,
														std::set<std::string>& scale_names) const;
    void 					getAssociatedVolumes(std::vector<LLVOVolume*>& volumes);
    void 					updateRiggingInfo();
	std::map<LLUUID,S32>	mLastRiggingInfoKey;
    uuid_set_t		mActiveOverrideMeshes;
    virtual void			onActiveOverrideMeshesChanged();
	const LLUUID&	getID() const;
	void			addDebugText(const std::string& text);
	F32				getTimeDilation();
	void			getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm);
	F32				getPixelArea() const;
	LLVector3d		getPosGlobalFromAgent(const LLVector3 &position);
	LLVector3		getPosAgentFromGlobal(const LLVector3d &position);
	virtual void			updateVisualParams();
public:
	virtual bool 	isSelf() const { return false; }
	bool 	isControlAvatar() const { return mIsControlAvatar; }
	virtual LLControlAvatar* asControlAvatar() { return nullptr; }
	bool 	isUIAvatar() const { return mIsUIAvatar; }
private:
	LL_ALIGN_16(LLVector4a	mImpostorExtents[2]);
private:
	BOOL			mSupportsAlphaLayers;
public:
    void			updateAppearanceMessageDebugText();
	void 			updateAnimationDebugText();
	virtual void	updateDebugText();
	virtual BOOL 	updateCharacter(LLAgent &agent);
    void			updateFootstepSounds();
    void			computeUpdatePeriod();
    void			updateOrientation(LLAgent &agent, F32 speed, F32 delta_time);
    void			updateTimeStep();
    void			updateRootPositionAndRotation(LLAgent &agent, F32 speed, bool was_sit_ground_constrained);
	void 			idleUpdateVoiceVisualizer(bool voice_enabled);
	void 			idleUpdateMisc(bool detailed_update);
	virtual void	idleUpdateAppearanceAnimation();
	void 			idleUpdateLipSync(bool voice_enabled);
	void 			idleUpdateLoadingEffect();
	void 			idleUpdateWindEffect();
	void 			idleUpdateNameTag(const LLVector3& root_pos_last);
	void			idleUpdateNameTagText(BOOL new_name);
	LLVector3		idleUpdateNameTagPosition(const LLVector3& root_pos_last);
	void			idleUpdateNameTagAlpha(BOOL new_name, F32 alpha);
	LLColor4		getNameTagColor(bool is_friend);
	void			clearNameTag();
	static void		invalidateNameTag(const LLUUID& agent_id);
	static void		invalidateNameTags();
	void			addNameTagLine(const std::string& line, const LLColor4& color, S32 style, const LLFontGL* font);
	void 			idleUpdateRenderComplexity();
    void 			accountRenderComplexityForObject(const LLViewerObject *attached_object,
                                                     const F32 max_attachment_complexity,
                                                     LLVOVolume::texture_cost_t& textures,
                                                     U32& cost
);
	void			calculateUpdateRenderComplexity();
	static const U32 VISUAL_COMPLEXITY_UNKNOWN;
	void			updateVisualComplexity();
	U32				getVisualComplexity()			{ return mVisualComplexity;				};
	F32				getAttachmentSurfaceArea()		{ return mAttachmentSurfaceArea;		};
	U32				getReportedVisualComplexity()					{ return mReportedVisualComplexity;				};
	void			setReportedVisualComplexity(S32 value)			{ mReportedVisualComplexity = value;			};
	S32				getUpdatePeriod()				{ return mUpdatePeriod;			};
	static void		updateImpostorRendering(U32 newMaxNonImpostorsValue);
	void 			idleUpdateBelowWater();
public:
	static S32		sRenderName;
	static bool		sRenderGroupTitles;
	static U32		sMaxVisible;
	static F32		sRenderDistance;
	static BOOL		sShowAnimationDebug;
	static bool		sUseImpostors;
	static bool		sShowFootPlane;
	static bool		sVisibleInFirstPerson;
	static S32		sNumLODChangesThisFrame;
	static S32		sNumVisibleChatBubbles;
	static BOOL		sDebugInvisible;
	static bool		sShowAttachmentPoints;
	static F32		sLODFactor;
	static F32		sPhysicsLODFactor;
	static BOOL		sJointDebug;
	static BOOL		sDebugAvatarRotation;
public:
	LLHost			getObjectHost() const;
public:
	BOOL			isFullyLoaded() const;
	bool			isTooComplex() const;
	bool 			visualParamWeightsAreDefault();
	virtual BOOL	getIsCloud() const;
	BOOL			isFullyTextured() const;
	BOOL			hasGray() const;
	S32				getRezzedStatus() const;
	void			updateRezzedStatusTimers();
	bool			isLangolier() const { return mFreezeTimeLangolier; }
	bool			isFrozenDead() const { return mFreezeTimeDead; }
	S32				mLastRezzedStatus;
	void 			startPhase(const std::string& phase_name);
	void 			stopPhase(const std::string& phase_name, bool err_check = true);
	void			clearPhases();
	void 			logPendingPhases();
	static void 	logPendingPhasesAllAvatars();
	void 			logMetricsTimerRecord(const std::string& phase_name, F32 elapsed, bool completed);
protected:
	LLViewerStats::PhaseMap& getPhases() { return mPhases; }
	BOOL			updateIsFullyLoaded();
	BOOL			processFullyLoadedChange(bool loading);
	void			updateRuthTimer(bool loading);
	F32 			calcMorphAmount();
private:
	BOOL			mFirstFullyVisible;
	BOOL			mFullyLoaded;
	BOOL			mPreviousFullyLoaded;
	BOOL			mFullyLoadedInitialized;
	S32				mFullyLoadedFrameCounter;
	LLFrameTimer	mFullyLoadedTimer;
	LLFrameTimer	mRuthTimer;
	bool			mFreezeTimeLangolier;
	bool			mFreezeTimeDead;
private:
	LLViewerStats::PhaseMap mPhases;
protected:
	LLFrameTimer    mInvisibleTimer;
protected:
	LLAvatarJoint*	createAvatarJoint();
	LLAvatarJoint*	createAvatarJoint(S32 joint_num);
	LLAvatarJointMesh*	createAvatarJointMesh();
public:
	void				updateHeadOffset();
	void				postPelvisSetRecalc( void );
	BOOL	loadSkeletonNode();
    void                initAttachmentPoints(bool ignore_hud_joints = false);
	void	buildCharacter();
	void				resetVisualParams();
	void				resetSkeleton(bool reset_animations);
	LLVector3			mCurRootToHeadOffset;
	LLVector3			mTargetRootToHeadOffset;
	S32					mLastSkeletonSerialNum;
public:
	U32 		renderImpostor(LLColor4U color = LLColor4U(255,255,255,255), S32 diffuse_channel = 0);
	bool		isVisuallyMuted() const;
	void		resetFreezeTime();
	U32 		renderRigid();
	U32 		renderSkinned(EAvatarRenderPass pass);
	F32			getLastSkinTime() { return mLastSkinTime; }
	U32			renderSkinnedAttachments();
	U32 		renderTransparent(BOOL first_pass);
	void 		renderCollisionVolumes();
	void		renderBones();
	void		renderJoints();
	static void	deleteCachedImages(bool clearAll=true);
	static void	destroyGL();
	static void	restoreGL();
	S32			mSpecialRenderMode;
private:
	F32			mAttachmentSurfaceArea;
	U32			mAttachmentVisibleTriangleCount;
	F32			mAttachmentEstTriangleCount;
	bool		shouldAlphaMask();
	BOOL 		mNeedsSkin;
	F32			mLastSkinTime;
	S32	 		mUpdatePeriod;
	S32  		mNumInitFaces;
	mutable U32  mVisualComplexity;
	mutable bool mVisualComplexityStale;
	U32			 mReportedVisualComplexity;
public:
    bool mIsControlAvatar;
    bool mIsUIAvatar;
    bool mEnableDefaultMotions;
public:
	void	applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components, LLAvatarAppearanceDefines::EBakedTextureIndex index = LLAvatarAppearanceDefines::BAKED_NUM_INDICES);
	BOOL 		morphMaskNeedsUpdate(LLAvatarAppearanceDefines::EBakedTextureIndex index = LLAvatarAppearanceDefines::BAKED_NUM_INDICES);
public:
	void onGlobalColorChanged(const LLTexGlobalColor* global_color, bool upload_bake = false);
protected:
	void 		updateVisibility();
private:
	U32	 		mVisibilityRank;
	BOOL 		mVisible;
public:
	void 		updateShadowFaces();
	LLDrawable*	mShadow;
private:
	LLFace* 	mShadow0Facep;
	LLFace* 	mShadow1Facep;
	LLPointer<LLViewerTexture> mShadowImagep;
public:
	virtual BOOL isImpostor() const;
	BOOL 		shouldImpostor(const U32 rank_factor = 1) const;
	BOOL 		needsImpostorUpdate() const;
	const LLVector3& getImpostorOffset() const;
	const LLVector2& getImpostorDim() const;
	void 		getImpostorValues(LLVector4a* extents, LLVector3& angle, F32& distance) const;
	void 		cacheImpostorValues();
	void 		setImpostorDim(const LLVector2& dim);
	static void	resetImpostors();
	static void updateImpostors();
	LLRenderTarget mImpostor;
	BOOL		mNeedsImpostorUpdate;
	F32SecondsImplicit mLastImpostorUpdateFrameTime;
	const LLVector3*  getLastAnimExtents() const { return mLastAnimExtents; }
	void		setNeedsExtentUpdate(bool val) { mNeedsExtentUpdate = val; }
private:
	LLVector3	mImpostorOffset;
	LLVector2	mImpostorDim;
	BOOL		mNeedsAnimUpdate;
	bool		mNeedsExtentUpdate;
	LLVector3	mImpostorAngle;
	F32			mImpostorDistance;
	F32			mImpostorPixelArea;
	LLVector3	mLastAnimExtents[2];
	LLVector3	mLastAnimBasePos;
public:
	LLVector4	mWindVec;
	F32			mRipplePhase;
	BOOL		mBelowWater;
private:
	F32			mWindFreq;
	LLFrameTimer mRippleTimer;
	F32			mRippleTimeLast;
	LLVector3	mRippleAccel;
	LLVector3	mLastVel;
public:
	static void	cullAvatarsByPixelArea();
	BOOL		isCulled() const { return mCulled; }
private:
	BOOL		mCulled;
public:
	static void updateFreezeCounter(S32 counter = 0);
private:
	static S32  sFreezeCounter;
public:
	virtual LLViewerTexture::EBoostLevel 	getAvatarBoostLevel() const { return LLGLTexture::BOOST_AVATAR; }
	virtual LLViewerTexture::EBoostLevel 	getAvatarBakedBoostLevel() const { return LLGLTexture::BOOST_AVATAR_BAKED; }
	virtual S32 						getTexImageSize() const;
	S32						getTexImageArea() const { return getTexImageSize()*getTexImageSize(); }
public:
	virtual BOOL    isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex type, U32 index = 0) const;
	virtual BOOL	isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, U32 index = 0) const;
	virtual BOOL	isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerWearable *wearable) const;
	BOOL			isFullyBaked();
	static BOOL		areAllNearbyInstancesBaked(S32& grey_avatars);
	static void		getNearbyRezzedStats(std::vector<S32>& counts);
	static std::string rezStatusToString(S32 status);
public:
	LLTexLayerSet*	createTexLayerSet();
	void			releaseComponentTextures();
protected:
	static void		onBakedTextureMasksLoaded(BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	static void		onInitialBakedTextureLoaded(BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	static void		onBakedTextureLoaded(BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	virtual void	removeMissingBakedTextures();
	void			useBakedTexture(const LLUUID& id);
	LLViewerTexLayerSet*  getTexLayerSet(const U32 index) const { return dynamic_cast<LLViewerTexLayerSet*>(mBakedTextureDatas[index].mTexLayerSet);	}
	LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList ;
	BOOL mLoadedCallbacksPaused;
	uuid_set_t	mTextureIDs;
protected:
	virtual void	setLocalTexture(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerTexture* tex, BOOL baked_version_exits, U32 index = 0);
	virtual void	addLocalTextureStats(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerFetchedTexture* imagep, F32 texel_area_ratio, BOOL rendered, BOOL covered_by_baked);
	virtual void	setBakedReady(LLAvatarAppearanceDefines::ETextureIndex type, BOOL baked_version_exists, U32 index = 0);
private:
	virtual	void				setImage(const U8 te, LLViewerTexture *imagep, const U32 index);
	virtual LLViewerTexture*	getImage(const U8 te, const U32 index) const;
	const std::string 			getImageURL(const U8 te, const LLUUID &uuid);
	virtual const LLTextureEntry* getTexEntry(const U8 te_num) const;
	virtual void setTexEntry(const U8 index, const LLTextureEntry &te);
	void checkTextureLoading() ;
protected:
	void			deleteLayerSetCaches(bool clearAll = true);
	void			addBakedTextureStats(LLViewerFetchedTexture* imagep, F32 pixel_area, F32 texel_area_ratio, S32 boost_level);
public:
	virtual void	invalidateComposite(LLTexLayerSet* layerset, BOOL upload_result);
	virtual void	invalidateAll();
	virtual void	setCompositeUpdatesEnabled(bool b) {}
	virtual void 	setCompositeUpdatesEnabled(U32 index, bool b) {}
	virtual bool 	isCompositeUpdateEnabled(U32 index) { return false; }
public:
	static BOOL 	isIndexLocalTexture(LLAvatarAppearanceDefines::ETextureIndex i);
	static BOOL 	isIndexBakedTexture(LLAvatarAppearanceDefines::ETextureIndex i);
private:
	static const LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary *getDictionary() { return sAvatarDictionary; }
	static LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary* sAvatarDictionary;
public:
	void 			onFirstTEMessageReceived();
private:
	BOOL			mFirstTEMessageReceived;
	BOOL			mFirstAppearanceMessageReceived;
public:
	void			debugColorizeSubMeshes(U32 i, const LLColor4& color);
	virtual void 	updateMeshTextures();
	void 			updateSexDependentLayerSets(bool upload_bake = false);
	virtual void	dirtyMesh();
	S32				getDirtyMesh() const { return mDirtyMesh; }
	void 			updateMeshData();
	void			updateMeshVisibility();
	LLViewerTexture*		getBakedTexture(const U8 te);
protected:
	void 			releaseMeshData();
	virtual void restoreMeshData();
private:
	virtual void	dirtyMesh(S32 priority);
	LLViewerJoint*	getViewerJoint(S32 idx);
	S32 			mDirtyMesh;
	BOOL			mMeshTexturesDirty;
protected:
	BOOL			mMeshValid;
	LLFrameTimer	mMeshInvisibleTime;
   LLPointer<LLAppearanceMessageContents> 	mLastProcessedAppearance;
public:
	void 			parseAppearanceMessage(LLMessageSystem* mesgsys, LLAppearanceMessageContents& msg);
	void 			processAvatarAppearance(LLMessageSystem* mesgsys);
    void            applyParsedAppearanceMessage(LLAppearanceMessageContents& contents, bool slam_params);
	void 			hideSkirt();
	void			startAppearanceAnimation();
public:
	BOOL			getIsAppearanceAnimating() const { return mAppearanceAnimating; }
	BOOL	isUsingLocalAppearance() const { return mUseLocalAppearance; }
	BOOL				isUsingServerBakes() const;
	void 				setIsUsingServerBakes(BOOL newval);
	BOOL	isEditingAppearance() const { return mIsEditingAppearance; }
private:
	BOOL			mAppearanceAnimating;
	LLFrameTimer	mAppearanceMorphTimer;
	F32				mLastAppearanceBlendTime;
	BOOL			mIsEditingAppearance;
	BOOL			mUseLocalAppearance;
	BOOL			mUseServerBakes;
public:
	BOOL			isVisible() const;
	virtual bool    shouldRenderRigged() const;
	void			setVisibilityRank(U32 rank);
	U32				getVisibilityRank()  const { return mVisibilityRank; }
	static S32 		sNumVisibleAvatars;
public:
	void 				clampAttachmentPositions();
	virtual const LLViewerJointAttachment* attachObject(LLViewerObject *viewer_object);
	virtual BOOL 		detachObject(LLViewerObject *viewer_object);
	static bool		    getRiggedMeshID( LLViewerObject* pVO, LLUUID& mesh_id );
	void				cleanupAttachedMesh( LLViewerObject* pVO );
	static LLVOAvatar*  findAvatarFromAttachment(LLViewerObject* obj);
	BOOL	isWearingWearableType(LLWearableType::EType type ) const;
	LLViewerObject *	findAttachmentByID( const LLUUID & target_id ) const;
	LLViewerJointAttachment* getTargetAttachmentPoint(LLViewerObject* viewer_object);
protected:
	void 				lazyAttach();
	void				rebuildRiggedAttachments( void );
public:
	S32 				getAttachmentCount();
	typedef LLSortedVector<S32, LLViewerJointAttachment*> attachment_map_t;
	attachment_map_t 								mAttachmentPoints;
	std::vector<LLPointer<LLViewerObject> > 		mPendingAttachment;
	std::vector<std::pair<LLViewerObject*,LLViewerJointAttachment*> >	mAttachedObjectsVector;
public:
	BOOL 				hasHUDAttachment() const;
	LLBBox 				getHUDBBox() const;
	void 				resetHUDAttachments();
	BOOL				canAttachMoreObjects(U32 n=1) const;
    U32					getMaxAnimatedObjectAttachments() const;
    BOOL				canAttachMoreAnimatedObjects(U32 n=1) const;
protected:
	U32					getNumAttachments() const;
	U32					getNumAnimatedObjectAttachments() const;
public:
	BOOL 			isWearingAttachment( const LLUUID& inv_item_id );
	LLViewerObject* getWornAttachment( const LLUUID& inv_item_id );
public:
	BOOL 			isAnyAnimationSignaled(const LLUUID *anim_array, const S32 num_anims) const;
	void 			processAnimationStateChanges();
protected:
	BOOL 			processSingleAnimationStateChange(const LLUUID &anim_id, BOOL start);
	void 			resetAnimations();
private:
	LLTimer			mAnimTimer;
	F32				mTimeLast;
public:
	typedef std::map<LLUUID, S32>::iterator AnimIterator;
	std::map<LLUUID, S32> 					mSignaledAnimations;
	std::map<LLUUID, S32> 					mPlayingAnimations;
	typedef std::multimap<LLUUID, LLUUID> 	AnimationSourceMap;
	typedef AnimationSourceMap::iterator 	AnimSourceIterator;
	AnimationSourceMap 						mAnimationSources;
public:
	void			addChat(const LLChat& chat);
	void	   		clearChat();
	void			startTyping() { mTyping = TRUE; mTypingTimer.reset(); mIdleTimer.reset();}
	void			stopTyping() { mTyping = FALSE; mIdleTimer.reset();}
private:
	BOOL			mVisibleChat;
	bool			mVisibleTyping;
private:
	bool 		   	mLipSyncActive;
	LLVisualParam* 	mOohMorph;
	LLVisualParam* 	mAahMorph;
public:
	BOOL			mInAir;
	LLFrameTimer	mTimeInAir;
private:
	F32 		mSpeedAccum;
	BOOL 		mTurning;
	F32			mSpeed;
public:
	void 		resolveHeightGlobal(const LLVector3d &inPos, LLVector3d &outPos, LLVector3 &outNorm);
	void 		resolveHeightAgent(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm);
	void 		resolveRayCollisionAgent(const LLVector3d start_pt, const LLVector3d end_pt, LLVector3d &out_pos, LLVector3 &out_norm);
	void 		slamPosition();
protected:
private:
	BOOL		mStepOnLand;
	U8			mStepMaterial;
	LLVector3	mStepObjectVelocity;
public:
	bool		mHasPhysicsParameters;
public:
	BOOL 	setParent(LLViewerObject* parent);
	void 	addChild(LLViewerObject *childp);
	void 	removeChild(LLViewerObject *childp);
public:
	void			sitDown(BOOL bSitting);
	BOOL			isSitting() const {return mIsSitting;}
	void 			sitOnObject(LLViewerObject *sit_object);
	void 			getOffObject();
private:
	BOOL 			mIsSitting;
	LLVector3		mLastRootPos;
public:
	std::string		getFullname() const;
	std::string		avString() const;
protected:
	static void		getAnimLabels(std::vector<std::string>* labels);
	static void		getAnimNames(std::vector<std::string>* names);
private:
	std::string		mNameString;
	std::string  	mTitle;
	bool	  		mNameAway;
	bool	  		mNameBusy;
	bool	  		mNameMute;
	bool      		mNameAppearance;
	bool			mNameFriend;
	bool			mNameCloud;
	bool			mNameLangolier;
	F32				mNameAlpha;
	BOOL      		mRenderGroupTitles;
public:
	LLFrameTimer	mChatTimer;
	LLPointer<LLHUDNameTag> mNameText;
private:
	LLFrameTimer	mTimeVisible;
	std::deque<LLChat> mChats;
	BOOL			mTyping;
public:
	BOOL isTyping(){ return mTyping; }
	LLFrameTimer	mTypingTimer;
public:
	LLPointer<LLVoiceVisualizer>  mVoiceVisualizer;
	int					mCurrentGesticulationLevel;
protected:
	const LLUUID& 		getStepSound() const;
private:
	const static LLUUID	sStepSounds[LL_MCODE_END];
	const static LLUUID	sStepSoundOnLand;
public:
	void 				setFootPlane(const LLVector4 &plane) { mFootPlane = plane; }
	LLVector4			mFootPlane;
private:
	BOOL				mWasOnGroundLeft;
	BOOL				mWasOnGroundRight;
public:
	void				getSortedJointNames(S32 joint_type, std::vector<std::string>& result) const;
	static void			dumpArchetypeXML_header(LLAPRFile& file, std::string const& archetype_name = "???");
	static void			dumpArchetypeXML_footer(LLAPRFile& file);
	void				dumpArchetypeXML(const std::string& prefix, bool group_by_wearables = false);
	void				dumpArchetypeXML_cont(std::string const& fullpath, bool group_by_wearables);
	void 				dumpAppearanceMsgParams( const std::string& dump_prefix,
												 const LLAppearanceMessageContents& contents);
	static void			dumpBakedStatus();
	const std::string 	getBakedStatusForPrintout() const;
	void				dumpAvatarTEs(const std::string& context) const;
	static F32 			sUnbakedTime;
	static F32 			sUnbakedUpdateTime;
	static F32 			sGreyTime;
	static F32 			sGreyUpdateTime;
protected:
	S32					getUnbakedPixelAreaRank();
	BOOL				mHasGrey;
private:
	F32					mMinPixelArea;
	F32					mMaxPixelArea;
	F32					mAdjustedPixelArea;
	std::string  		mDebugText;
	std::string			mBakedTextureDebugText;
public:
	void 			debugAvatarRezTime(std::string notification_name, std::string comment = "");
	F32				debugGetExistenceTimeElapsedF32() const { return mDebugExistenceTimer.getElapsedTimeF32(); }
protected:
	LLFrameTimer	mRuthDebugTimer;
	LLFrameTimer	mDebugExistenceTimer;
	LLFrameTimer	mLastAppearanceMessageTimer;
public:
	S32 mLastUpdateRequestCOFVersion;
	S32 mLastUpdateReceivedCOFVersion;
protected:
public:
	typedef std::array<F32, LL_MAX_JOINTS_PER_MESH_OBJECT * 12> rigged_matrix_array_t;
	typedef std::vector<std::pair<LLUUID, std::pair<U32, rigged_matrix_array_t> > > rigged_transformation_cache_t;
	rigged_transformation_cache_t& getRiggedMatrixCache()
	{
		return mRiggedMatrixDataCache;
	}
	void clearRiggedMatrixCache()
	{
		mRiggedMatrixDataCache.clear();
	}
private:
	rigged_transformation_cache_t mRiggedMatrixDataCache;
private:
	std::string		getIdleTime(bool is_away, bool is_busy, bool is_appearance);
public:
	LLFrameTimer 	mIdleTimer;
private:
	S32				mIdleMinute;
	LLTimer			mComplexityTimer;
public:
	void setNameFromChat(const std::string &text);
	void clearNameFromChat();
private:
	void			idleCCSUpdateAttachmentText(bool render_name);
	LLFrameTimer	mCCSUpdateAttachmentTimer;
	std::string		mCCSAttachmentText;
	bool			mCCSChatTextOverride;
	std::string		mCCSChatText;
};
extern const F32 SELF_ADDITIONAL_PRI;
extern const S32 MAX_TEXTURE_VIRTUAL_SIZE_RESET_INTERVAL;
extern const F32 MAX_HOVER_Z;
extern const F32 MIN_HOVER_Z;
constexpr U32 NUM_ATTACHMENT_GROUPS = 24;
void dump_sequential_xml(const std::string outprefix, const LLSD& content);
#endif
