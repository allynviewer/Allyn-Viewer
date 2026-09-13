/** 
 * @file llvovolume.h
 * @brief LLVOVolume class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#ifndef LL_LLVOVOLUME_H
#define LL_LLVOVOLUME_H
#include "llviewerobject.h"
#include "llviewertexture.h"
#include "llviewermedia.h"
#include "llframetimer.h"
#include "llapr.h"
#include "m3math.h"
#include "m4math.h"
#include <map>
class LLViewerTextureAnim;
class LLDrawPool;
class LLMaterialID;
class LLSelectNode;
class LLObjectMediaDataClient;
class LLObjectMediaNavigateClient;
class LLVOAvatar;
class LLMeshSkinInfo;
typedef std::vector<viewer_media_t> media_list_t;
enum LLVolumeInterfaceType
{
	INTERFACE_FLEXIBLE = 1,
};
class LLRiggedVolume : public LLVolume
{
	U64 mFrame;
public:
	LLRiggedVolume(const LLVolumeParams& params)
		: LLVolume(params, 0.f), mFrame(-1)
	{
	}
	void update(const LLMeshSkinInfo* skin, LLVOAvatar* avatar, const LLVolume* src_volume);
};
class LLVolumeInterface
{
public:
	virtual ~LLVolumeInterface() { }
	virtual LLVolumeInterfaceType getInterfaceType() const = 0;
	virtual void doIdleUpdate() = 0;
	virtual BOOL doUpdateGeometry(LLDrawable *drawable) = 0;
	virtual LLVector3 getPivotPosition() const = 0;
	virtual void onSetVolume(const LLVolumeParams &volume_params, const S32 detail) = 0;
	virtual void onSetScale(const LLVector3 &scale, BOOL damped) = 0;
	virtual void onParameterChanged(U16 param_type, LLNetworkData *data, BOOL in_use, bool local_origin) = 0;
	virtual void onShift(const LLVector4a &shift_vector) = 0;
	virtual bool isVolumeUnique() const = 0;
	virtual bool isVolumeGlobal() const = 0;
	virtual bool isActive() const = 0;
	virtual const LLMatrix4a& getWorldMatrix(LLXformMatrix* xform) const = 0;
	virtual void updateRelativeXform(bool force_identity = false) = 0;
	virtual U32 getID() const = 0;
	virtual void preRebuild() = 0;
};
class LLVOVolume final : public LLViewerObject
{
	LOG_CLASS(LLVOVolume);
protected:
	virtual				~LLVOVolume();
public:
	static		void	initClass();
	static		void	cleanupClass();
	static 		void 	preUpdateGeom();
	enum
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD1) |
							(1 << LLVertexBuffer::TYPE_COLOR)
	};
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
public:
						LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
				LLVOVolume* asVolume() final;
	void markDead() override;
	LLDrawable* createDrawable(LLPipeline *pipeline);
				void	deleteFaces();
				void	animateTextures();
	            BOOL    isVisible() const ;
	BOOL	isActive() const;
	BOOL	isAttachment() const;
	BOOL	isRootEdit() const;
	BOOL	isHUDAttachment() const;
				void	generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point);
	BOOL	setParent(LLViewerObject* parent);
				S32		getLOD() const							{ return mLOD; }
				void	setNoLOD()							{ mLOD = NO_LOD; mLODChanged = TRUE; }
				bool	isNoLOD() const						{ return NO_LOD == mLOD; }
	const LLVector3		getPivotPositionAgent() const;
	const LLMatrix4a&	getRelativeXform() const				{ return mRelativeXform; }
	const LLMatrix4a&	getRelativeXformInvTrans() const		{ return mRelativeXformInvTrans; }
	const LLMatrix4a&	getRenderMatrix() const;
	typedef std::map<LLUUID, S32> texture_cost_t;
				U32 	getRenderCost(texture_cost_t &textures) const;
	F32		getEstTrianglesMax() const;
	F32		getEstTrianglesStreamingCost() const;
	F32	getStreamingCost() const;
	bool getCostData(LLMeshCostData& costs) const;
	U32		getTriangleCount(S32* vcount = NULL) const;
	U32		getHighLODTriangleCount();
	BOOL lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
										  S32 face = -1,
										  BOOL pick_transparent = FALSE,
										  BOOL pick_rigged = FALSE,
										  S32* face_hit = NULL,
										  LLVector4a* intersection = NULL,
										  LLVector2* tex_coord = NULL,
										  LLVector4a* normal = NULL,
										  LLVector4a* tangent = NULL
		);
				LLVector3 agentPositionToVolume(const LLVector3& pos) const;
				LLVector3 agentDirectionToVolume(const LLVector3& dir) const;
				LLVector3 volumePositionToAgent(const LLVector3& dir) const;
				LLVector3 volumeDirectionToAgent(const LLVector3& dir) const;
				BOOL	getVolumeChanged() const				{ return mVolumeChanged; }
	F32  	getRadius() const						{ return mVObjRadius; };
				const LLMatrix4a& getWorldMatrix(LLXformMatrix* xform) const;
				void	markForUpdate(BOOL priority);
				void	markForUnload()							{ LLViewerObject::markForUnload(TRUE); mVolumeChanged = TRUE; }
				void	faceMappingChanged()					{ mFaceMappingChanged=TRUE; };
	void	onShift(const LLVector4a &shift_vector);
	void	parameterChanged(U16 param_type, bool local_origin);
	void	parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin);
	U32		processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);
	void	setSelected(BOOL sel);
	BOOL	setDrawableParent(LLDrawable* parentp);
	void	setScale(const LLVector3 &scale, BOOL damped);
	void    changeTEImage(S32 index, LLViewerTexture* new_image)  ;
	void	setNumTEs(const U8 num_tes);
	void	setTEImage(const U8 te, LLViewerTexture *imagep);
	S32		setTETexture(const U8 te, const LLUUID &uuid);
	S32		setTEColor(const U8 te, const LLColor3 &color);
	S32		setTEColor(const U8 te, const LLColor4 &color);
	S32		setTEBumpmap(const U8 te, const U8 bump);
	S32		setTEShiny(const U8 te, const U8 shiny);
	S32		setTEFullbright(const U8 te, const U8 fullbright);
	S32		setTEBumpShinyFullbright(const U8 te, const U8 bump);
	S32		setTEMediaFlags(const U8 te, const U8 media_flags);
	S32		setTEGlow(const U8 te, const F32 glow);
	S32		setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID);
	static void	setTEMaterialParamsCallbackTE(const LLUUID& objectID, const LLMaterialID& pMaterialID, const LLMaterialPtr pMaterialParams, U32 te);
	S32		setTEMaterialParams(const U8 te, const LLMaterialPtr pMaterialParams);
	S32		setTEScale(const U8 te, const F32 s, const F32 t);
	S32		setTEScaleS(const U8 te, const F32 s);
	S32		setTEScaleT(const U8 te, const F32 t);
	S32		setTETexGen(const U8 te, const U8 texgen);
	S32		setTEMediaTexGen(const U8 te, const U8 media);
	BOOL 	setMaterial(const U8 material);
				void	setTexture(const S32 face);
				S32     getIndexInTex(U32 ch) const {return mIndexInTex[ch];}
	BOOL	setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);
				void	updateSculptTexture();
				void    setIndexInTex(U32 ch, S32 index) { mIndexInTex[ch] = index ;}
				void	sculpt();
	 static     void    rebuildMeshAssetCallback(LLVFS *vfs,
														  const LLUUID& asset_uuid,
														  LLAssetType::EType type,
														  void* user_data, S32 status, LLExtStat ext_status);
				void	updateRelativeXform(bool force_identity = false);
	BOOL	updateGeometry(LLDrawable *drawable);
	void	updateFaceSize(S32 idx);
	BOOL	updateLOD();
				void	updateRadius();
	void	updateTextures();
				void	updateTextureVirtualSize(bool forced = false);
				void	updateFaceFlags();
				void	regenFaces();
				BOOL	genBBoxes(BOOL force_global);
				void	preRebuild();
	virtual		void	updateSpatialExtents(LLVector4a& min, LLVector4a& max);
	virtual		F32		getBinRadius();
	virtual U32 getPartitionType() const;
	void setIsLight(BOOL is_light);
	void setLightColor(const LLColor3& color);
	void setLightIntensity(F32 intensity);
	void setLightRadius(F32 radius);
	void setLightFalloff(F32 falloff);
	void setLightCutoff(F32 cutoff);
	void setLightTextureID(LLUUID id);
	void setSpotLightParams(LLVector3 params);
	BOOL getIsLight() const;
	LLColor3 getLightBaseColor() const;
	LLColor3 getLightColor() const;
	LLUUID	getLightTextureID() const;
	bool isLightSpotlight() const;
	LLVector3 getSpotLightParams() const;
	void	updateSpotLightPriority();
	F32		getSpotLightPriority() const;
	LLViewerTexture* getLightTexture();
	F32 getLightIntensity() const;
	F32 getLightRadius() const;
	F32 getLightFalloff() const;
	F32 getLightCutoff() const;
	U32 getVolumeInterfaceID() const;
	virtual BOOL isFlexible() const;
	virtual BOOL isSculpted() const;
	virtual BOOL isMesh() const;
	virtual BOOL isRiggedMesh() const;
	virtual BOOL hasLightTexture() const;
	BOOL isVolumeGlobal() const;
	BOOL canBeFlexible() const;
	BOOL setIsFlexible(BOOL is_flexible);
    const LLMeshSkinInfo* getSkinInfo() const;
    U32 getExtendedMeshFlags() const;
    void onSetExtendedMeshFlags(U32 flags);
    void setExtendedMeshFlags(U32 flags);
    bool canBeAnimatedObject() const;
    bool isAnimatedObject() const;
    virtual void onReparent(LLViewerObject *old_parent, LLViewerObject *new_parent);
    virtual void afterReparent();
    void updateRiggingInfo();
    S32 mLastRiggingInfoLOD;
	void updateObjectMediaData(const LLSD &media_data_array, const std::string &media_version);
	void mediaNavigateBounceBack(U8 texture_index);
	enum MediaPermType {
		MEDIA_PERM_INTERACT, MEDIA_PERM_CONTROL
	};
    bool hasMediaPermission(const LLMediaEntry* media_entry, MediaPermType perm_type);
	void mediaNavigated(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, std::string new_location);
	void mediaEvent(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event);
	void syncMediaData(S32 te, const LLSD &media_data, bool merge, bool ignore_agent);
	void sendMediaDataUpdate();
	viewer_media_t getMediaImpl(U8 face_id) const;
	S32 getFaceIndexWithMediaImpl(const LLViewerMediaImpl* media_impl, S32 start_face_id);
	F64 getTotalMediaInterest() const;
	bool hasMedia() const;
	LLVector3 getApproximateFaceNormal(U8 face_id);
	void setSculptChanged(BOOL has_changed) { mSculptChanged = has_changed; }
	void updateVisualComplexity();
	void notifyMeshLoaded();
	bool isMediaDataBeingFetched() const;
	S32 getLastFetchedMediaVersion() const { return mLastFetchedMediaVersion; }
	void addMDCImpl() { ++mMDCImplCount; }
	void removeMDCImpl() { --mMDCImplCount; }
	S32 getMDCImplCount() { return mMDCImplCount; }
	void updateRiggedVolume(bool force_update = false);
	LLRiggedVolume* getRiggedVolume();
	bool treatAsRigged();
	void clearRiggedVolume();
protected:
	S32	computeLODDetail(F32 distance, F32 radius, F32 lod_factor);
	BOOL calcLOD();
	LLFace* addFace(S32 face_index);
	void updateTEData();
	static S32 mRenderComplexity_last;
	static S32 mRenderComplexity_current;
	void requestMediaDataUpdate(bool isNew);
	void cleanUpMediaImpls();
	void addMediaImpl(LLViewerMediaImpl* media_impl, S32 texture_index) ;
	void removeMediaImpl(S32 texture_index) ;
private:
	bool lodOrSculptChanged(LLDrawable *drawable, BOOL &compiled);
public:
	static S32 getRenderComplexityMax() {return mRenderComplexity_last;}
	static void updateRenderComplexity();
	LLViewerTextureAnim *mTextureAnimp;
	U8 mTexAnimMode;
    F32 mLODDistance;
    F32 mLODAdjustedDistance;
    F32 mLODRadius;
private:
	friend class LLDrawable;
	friend class LLFace;
	BOOL		mFaceMappingChanged;
	LLFrameTimer mTextureUpdateTimer;
	S32			mLOD;
	BOOL		mLODChanged;
	BOOL		mSculptChanged;
	F32			mSpotLightPriority;
	LL_ALIGN_16(LLMatrix4a	mRelativeXform);
	LL_ALIGN_16(LLMatrix4a	mRelativeXformInvTrans);
	BOOL		mVolumeChanged;
	F32			mVObjRadius;
	LLVolumeInterface *mVolumeImpl;
	LLPointer<LLViewerFetchedTexture> mSculptTexture;
	LLPointer<LLViewerFetchedTexture> mLightTexture;
	media_list_t mMediaImplList;
	S32			mLastFetchedMediaVersion;
	S32 mIndexInTex[LLRender::NUM_VOLUME_TEXTURE_CHANNELS];
	S32 mMDCImplCount;
	LLPointer<LLRiggedVolume> mRiggedVolume;
public:
	static F32 sLODSlopDistanceFactor;
	static F32 sLODFactor;
	static F32 sDistanceFactor;
	static LLPointer<LLObjectMediaDataClient> sObjectMediaClient;
	static LLPointer<LLObjectMediaNavigateClient> sObjectMediaNavigateClient;
protected:
	static S32 sNumLODChanges;
	friend class LLVolumeImplFlexible;
public:
	bool notifyAboutCreatingTexture(LLViewerTexture *texture);
	bool notifyAboutMissingAsset(LLViewerTexture *texture);
private:
	struct material_info
	{
		LLRender::eTexIndex map;
		U8 te;
		material_info(LLRender::eTexIndex map_, U8 te_)
			: map(map_)
			, te(te_)
		{}
	};
	typedef std::multimap<LLUUID, material_info> mmap_UUID_MAP_t;
	mmap_UUID_MAP_t	mWaitingTextureInfo;
};
#endif
