/** 
 * @file llviewerobject.h
 * @brief Description of LLViewerObject class, which is the base class for most objects in the viewer.
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
#ifndef LL_LLVIEWEROBJECT_H
#define LL_LLVIEWEROBJECT_H
#include <map>
#include "llassetstorage.h"
#include "llhudtext.h"
#include "llhudicon.h"
#include "llinventory.h"
#include "llrefcount.h"
#include "llprimitive.h"
#include "lluuid.h"
#include "llvoinventorylistener.h"
#include "object_flags.h"
#include "llquaternion.h"
#include "v3dmath.h"
#include "v3math.h"
#include "llvertexbuffer.h"
#include "llbbox.h"
class LLAgent;
class LLAudioSource;
class LLAudioSourceVO;
class LLBBox;
class LLColor4;
class LLControlAvatar;
class LLDataPacker;
class LLDrawable;
class LLFrameTimer;
class LLHost;
class LLMessageSystem;
class LLNameValue;
class LLNetMap;
class LLPartSysData;
class LLPipeline;
class LLPrimitive;
class LLTextureEntry;
class LLVOAvatar;
class LLVOInventoryListener;
class LLVOVolume;
class LLViewerInventoryItem;
class LLViewerObject;
class LLViewerObjectMedia;
class LLViewerPartSourceScript;
class LLViewerRegion;
class LLViewerTexture;
class LLWorld;
class LLMeshCostData;
typedef enum e_object_update_type
{
	OUT_FULL,
	OUT_TERSE_IMPROVED,
	OUT_FULL_COMPRESSED,
	OUT_FULL_CACHED,
	OUT_UNKNOWN,
} EObjectUpdateType;
typedef void (*inventory_callback)(LLViewerObject*,
								   LLInventoryObject::object_list_t*,
								   S32 serial_num,
								   void*);
struct LLMaterialExportInfo
{
public:
	LLMaterialExportInfo(S32 mat_index, S32 texture_index, LLColor4 color) :
	  mMaterialIndex(mat_index), mTextureIndex(texture_index), mColor(color) {};
	S32			mMaterialIndex;
	S32			mTextureIndex;
	LLColor4	mColor;
};
struct PotentialReturnableObject
{
	LLBBox			box;
	LLViewerRegion* pRegion;
};
class LLViewerObject
:	public LLPrimitive,
	public LLRefCount,
	public LLGLUpdate
{
protected:
	~LLViewerObject();
private:
	struct ExtraParameter
	{
		bool is_invalid = false;
		bool* in_use = nullptr;
		LLNetworkData* data = nullptr;
	};
	std::vector<ExtraParameter> mExtraParameterList;
	bool mFlexibleObjectDataInUse = false;
	bool mLightParamsInUse = false;
	bool mSculptParamsInUse = false;
	bool mLightImageParamsInUse = false;
	bool mExtendedMeshParamsInUse = false;
	LLFlexibleObjectData mFlexibleObjectData;
	LLLightParams mLightParams;
	LLSculptParams mSculptParams;
	LLLightImageParams mLightImageParams;
	LLExtendedMeshParams mExtendedMeshParams;
public:
	typedef std::list<LLPointer<LLViewerObject> > child_list_t;
	typedef std::list<LLPointer<LLViewerObject> > vobj_list_t;
	typedef const child_list_t const_child_list_t;
	LLViewerObject(const LLUUID &id, const LLPCode type, LLViewerRegion *regionp, BOOL is_global = FALSE);
	virtual void resetVertexBuffers() {}
	virtual void markDead();
	BOOL isDead() const									{return mDead;}
	BOOL isOrphaned() const								{ return mOrphaned; }
	BOOL isParticleSource() const;
	virtual LLVOAvatar* asAvatar();
	virtual LLVOVolume* asVolume();
	LLVOAvatar* getAvatarAncestor();
	static void initVOClasses();
	static void cleanupVOClasses();
	void			addNVPair(const std::string& data);
	BOOL			removeNVPair(const std::string& name);
	LLNameValue*	getNVPair(const std::string& name) const;
	virtual void	idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	enum { MEDIA_NONE = 0, MEDIA_SET = 1 };
	enum {
		MEDIA_URL_REMOVED = 0x1,
        MEDIA_URL_ADDED = 0x2,
        MEDIA_URL_UPDATED = 0x4,
        MEDIA_FLAGS_CHANGED = 0x8,
		INVALID_UPDATE = 0x80000000
	};
	static  U32     extractSpatialExtents(LLDataPackerBinaryBuffer *dp, LLVector3& pos, LLVector3& scale, LLQuaternion& rot);
	virtual U32		processUpdateMessage(LLMessageSystem *mesgsys,
										void **user_data,
										U32 block_num,
										const EObjectUpdateType update_type,
										LLDataPacker *dp);
	void processTerseData(LLMessageSystem *mesgsys, void **user_data, U32 block_num, S32& this_update_precision, LLVector3& new_pos_parent, LLQuaternion& new_rot, LLVector3& new_angv, LLVector3& test_pos_parent);
	virtual BOOL    isActive() const;
	BOOL			onActiveList() const				{return mOnActiveList;}
	void			setOnActiveList(BOOL on_active)		{ mOnActiveList = on_active; }
	virtual BOOL	isAttachment() const { return FALSE; }
	virtual LLVOAvatar* getAvatar() const;
	virtual BOOL	isHUDAttachment() const { return FALSE; }
	virtual BOOL	isTempAttachment() const;
	virtual void 	updateRadius() {};
	virtual F32 	getVObjRadius() const;
	LLViewerObject* getSubParent();
	const LLViewerObject* getSubParent() const;
	virtual void setPixelAreaAndAngle(LLAgent &agent);
	virtual U32 getNumVertices() const;
	virtual U32 getNumIndices() const;
	S32 getNumFaces() const { return mNumFaces; }
	virtual void updateTextures();
	virtual void boostTexturePriority(BOOL boost_children = TRUE);
	virtual LLDrawable* createDrawable(LLPipeline *pipeline);
	virtual BOOL		updateGeometry(LLDrawable *drawable);
	virtual void		updateGL();
	virtual void		updateFaceSize(S32 idx);
	virtual BOOL		updateLOD();
	virtual BOOL		setDrawableParent(LLDrawable* parentp);
	F32					getRotTime() { return mRotTime; }
	void				resetRot();
	void				applyAngularVelocity(F32 dt);
	void setLineWidthForWindowSize(S32 window_width);
	static void increaseArrowLength();
	static void decreaseArrowLength();
	LLViewerRegion* getRegion() const				{ return mRegionp; }
	BOOL isSelected() const							{ return mUserSelected; }
    BOOL isAnySelected() const;
	void setSelected(BOOL sel);
	const LLUUID &getID() const						{ return mID; }
	U32 getLocalID() const							{ return mLocalID; }
	U32 getCRC() const								{ return mTotalCRC; }
	S32 getListIndex() const						{ return mListIndex; }
	void setListIndex(S32 idx)						{ mListIndex = idx; }
	virtual BOOL isFlexible() const					{ return FALSE; }
	virtual BOOL isSculpted() const 				{ return FALSE; }
	virtual BOOL isMesh() const						{ return FALSE; }
	virtual BOOL isRiggedMesh() const				{ return FALSE; }
	virtual BOOL hasLightTexture() const			{ return FALSE; }
	bool isReturnable();
	void buildReturnablesForChildrenVO( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion );
	void constructAndAddReturnable( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion );
	bool crossesParcelBounds();
	virtual BOOL setParent(LLViewerObject* parent);
    virtual void onReparent(LLViewerObject *old_parent, LLViewerObject *new_parent);
    virtual void afterReparent();
	virtual void addChild(LLViewerObject *childp);
	virtual void removeChild(LLViewerObject *childp);
	const_child_list_t& getChildren() const { 	return mChildList; }
	S32 numChildren() const { return mChildList.size(); }
	void addThisAndAllChildren(std::vector<LLViewerObject*>& objects);
	void addThisAndNonJointChildren(std::vector<LLViewerObject*>& objects);
	BOOL isChild(const LLViewerObject *childp) const;
	BOOL isSeat() const;
	virtual BOOL lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
									  S32 face = -1,
									  BOOL pick_transparent = FALSE,
									  BOOL pick_rigged = FALSE,
									  S32* face_hit = NULL,
									  LLVector4a* intersection = NULL,
									  LLVector2* tex_coord = NULL,
									  LLVector4a* normal = NULL,
									  LLVector4a* tangent = NULL
		);
	virtual BOOL lineSegmentBoundingBox(const LLVector4a& start, const LLVector4a& end);
	virtual const LLVector3d getPositionGlobal() const;
	virtual const LLVector3 &getPositionRegion() const;
	virtual const LLVector3 getPositionEdit() const;
	virtual const LLVector3 &getPositionAgent() const;
	virtual const LLVector3 getRenderPosition() const;
	virtual const LLVector3 getPivotPositionAgent() const;
	LLViewerObject* getRootEdit() const;
	const LLQuaternion getRotationRegion() const;
	const LLQuaternion getRotationEdit() const;
	const LLQuaternion getRenderRotation() const;
	virtual	const LLMatrix4a& getRenderMatrix() const;
	void setPosition(const LLVector3 &pos, BOOL damped = FALSE);
	void setPositionGlobal(const LLVector3d &position, BOOL damped = FALSE);
	void setPositionRegion(const LLVector3 &position, BOOL damped = FALSE);
	void setPositionEdit(const LLVector3 &position, BOOL damped = FALSE);
	void setPositionAgent(const LLVector3 &pos_agent, BOOL damped = FALSE);
	void setPositionParent(const LLVector3 &pos_parent, BOOL damped = FALSE);
	void setPositionAbsoluteGlobal( const LLVector3d &pos_global, BOOL damped = FALSE );
	virtual const LLMatrix4a& getWorldMatrix(LLXformMatrix* xform) const		{ return xform->getWorldMatrix(); }
	inline void setRotation(const F32 x, const F32 y, const F32 z, BOOL damped = FALSE);
	inline void setRotation(const LLQuaternion& quat, BOOL damped = FALSE);
	void sendRotationUpdate() const;
	void	setNumTEs(const U8 num_tes);
	void	setTE(const U8 te, const LLTextureEntry &texture_entry);
	S32		setTETexture(const U8 te, const LLUUID &uuid);
	S32		setTENormalMap(const U8 te, const LLUUID &uuid);
	S32		setTESpecularMap(const U8 te, const LLUUID &uuid);
	S32 setTETextureCore(const U8 te, LLViewerTexture *image);
	S32 setTENormalMapCore(const U8 te, LLViewerTexture *image);
	S32 setTESpecularMapCore(const U8 te, LLViewerTexture *image);
	S32		setTEColor(const U8 te, const LLColor3 &color);
	S32		setTEColor(const U8 te, const LLColor4 &color);
	S32		setTEScale(const U8 te, const F32 s, const F32 t);
	S32		setTEScaleS(const U8 te, const F32 s);
	S32		setTEScaleT(const U8 te, const F32 t);
	S32		setTEOffset(const U8 te, const F32 s, const F32 t);
	S32		setTEOffsetS(const U8 te, const F32 s);
	S32		setTEOffsetT(const U8 te, const F32 t);
	S32		setTERotation(const U8 te, const F32 r);
	S32		setTEBumpmap(const U8 te, const U8 bump );
	S32		setTETexGen(const U8 te, const U8 texgen );
	S32		setTEMediaTexGen(const U8 te, const U8 media );
	S32		setTEShiny(const U8 te, const U8 shiny );
	S32		setTEFullbright(const U8 te, const U8 fullbright );
	S32		setTEMediaFlags(const U8 te, const U8 media_flags );
	S32     setTEGlow(const U8 te, const F32 glow);
	S32     setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID);
	S32		setTEMaterialParams(const U8 te, const LLMaterialPtr pMaterialParams);
	void refreshMaterials();
	BOOL	setMaterial(const U8 material);
	virtual		void	setTEImage(const U8 te, LLViewerTexture *imagep);
	virtual     void    changeTEImage(S32 index, LLViewerTexture* new_image)  ;
	virtual     void    changeTENormalMap(S32 index, LLViewerTexture* new_image)  ;
	virtual     void    changeTESpecularMap(S32 index, LLViewerTexture* new_image)  ;
	LLViewerTexture		*getTEImage(const U8 te) const;
	LLViewerTexture		*getTENormalMap(const U8 te) const;
	LLViewerTexture		*getTESpecularMap(const U8 te) const;
	bool 						isImageAlphaBlended(const U8 te) const;
	void fitFaceTexture(const U8 face);
	void sendTEUpdate() const;
	virtual void setScale(const LLVector3 &scale, BOOL damped = FALSE);
    S32 getAnimatedObjectMaxTris() const;
    F32 recursiveGetEstTrianglesMax() const;
    virtual F32 getEstTrianglesMax() const;
    virtual F32 getEstTrianglesStreamingCost() const;
	virtual F32 getStreamingCost() const;
    virtual bool getCostData(LLMeshCostData& costs) const;
	virtual U32 getTriangleCount(S32* vcount = NULL) const;
	virtual U32 getHighLODTriangleCount();
    F32 recursiveGetScaledSurfaceArea() const;
    U32 recursiveGetTriangleCount(S32* vcount = NULL) const;
	void setObjectCost(F32 cost);
	F32 getObjectCost();
	void setLinksetCost(F32 cost);
	F32 getLinksetCost();
	void setPhysicsCost(F32 cost);
	F32 getPhysicsCost();
	void setLinksetPhysicsCost(F32 cost);
	F32 getLinksetPhysicsCost();
	void sendShapeUpdate();
	U8 getAttachmentState() const						{ return mAttachmentState; }
	F32 getAppAngle() const					{ return mAppAngle; }
	F32 getPixelArea() const				{ return mPixelArea; }
	void setPixelArea(F32 area)				{ mPixelArea = area; }
	F32 getMaxScale() const;
	F32 getMidScale() const;
	F32 getMinScale() const;
	void setAttachedSound(const LLUUID &audio_uuid, const LLUUID& owner_id, const F32 gain, const U8 flags);
	void adjustAudioGain(const F32 gain);
	void clearAttachedSound()								{ mAudioSourcep = NULL; }
	LLAudioSource *getAudioSource(const LLUUID& owner_id);
	bool isAudioSource() {return mAudioSourcep != NULL;}
	U8 getMediaType() const;
	void setMediaType(U8 media_type);
	std::string getMediaURL() const;
	void setMediaURL(const std::string& media_url);
	BOOL getMediaPassedWhitelist() const;
	void setMediaPassedWhitelist(BOOL passed);
	void sendMaterialUpdate() const;
	void setCanSelect(BOOL canSelect);
	void setDebugText(const std::string &utf8text);
	void initHudText();
	std::string getDebugText();
	void setIcon(LLViewerTexture* icon_image);
	void clearIcon();
    void recursiveMarkForUpdate(BOOL priority);
	virtual void markForUpdate(BOOL priority);
	void markForUnload(BOOL priority);
	void updateVolume(const LLVolumeParams& volume_params);
	virtual	void updateSpatialExtents(LLVector4a& min, LLVector4a& max);
	virtual F32 getBinRadius();
	LLBBox				getBoundingBoxAgent() const;
	void updatePositionCaches() const;
	void updateText();
	virtual void updateDrawable(BOOL force_damped);
	void setDrawableState(U32 state, BOOL recursive = TRUE);
	void clearDrawableState(U32 state, BOOL recursive = TRUE);
	BOOL isDrawableState(U32 state, BOOL recursive = TRUE) const;
	virtual void onShift(const LLVector4a &shift_vector)	{ }
	void registerInventoryListener(LLVOInventoryListener* listener, void* user_data);
	void removeInventoryListener(LLVOInventoryListener* listener);
	BOOL isInventoryPending();
	void clearInventoryListeners();
	bool hasInventoryListeners();
	void requestInventory();
	static void processTaskInv(LLMessageSystem* msg, void** user_data);
	void removeInventory(const LLUUID& item_id);
	void updateInventory(LLViewerInventoryItem* item, U8 key, bool is_new);
	void updateInventoryLocal(LLInventoryItem* item, U8 key);
	void updateTextureInventory(LLViewerInventoryItem* item, U8 key, bool is_new);
	LLInventoryObject* getInventoryObject(const LLUUID& item_id);
	void getInventoryContents(LLInventoryObject::object_list_t& objects);
	LLInventoryObject* getInventoryRoot();
	LLViewerInventoryItem* getInventoryItemByAsset(const LLUUID& asset_id);
	S16 getInventorySerial() const { return mInventorySerialNum; }
	bool isTextureInInventory(LLViewerInventoryItem* item);
	void updateViewerInventoryAsset(
		const LLViewerInventoryItem* item,
		const LLUUID& new_asset);
	void dirtyInventory();
	BOOL isInventoryDirty() { return mInventoryDirty; }
	void saveScript(const LLViewerInventoryItem* item, BOOL active, bool is_new);
	void moveInventory(const LLUUID& agent_folder, const LLUUID& item_id);
	S32 countInventoryContents( LLAssetType::EType type );
	BOOL			permAnyOwner() const;
	BOOL			permYouOwner() const;
	BOOL			permGroupOwner() const;
	BOOL			permOwnerModify() const;
	BOOL			permModify() const;
	BOOL			permCopy() const;
	BOOL			permMove() const;
	BOOL			permTransfer() const;
	inline BOOL		flagUsePhysics() const			{ return ((mFlags & FLAGS_USE_PHYSICS) != 0); }
	inline BOOL		flagObjectAnyOwner() const		{ return ((mFlags & FLAGS_OBJECT_ANY_OWNER) != 0); }
	inline BOOL		flagObjectYouOwner() const		{ return ((mFlags & FLAGS_OBJECT_YOU_OWNER) != 0); }
	inline BOOL		flagObjectGroupOwned() const	{ return ((mFlags & FLAGS_OBJECT_GROUP_OWNED) != 0); }
	inline BOOL		flagObjectOwnerModify() const	{ return ((mFlags & FLAGS_OBJECT_OWNER_MODIFY) != 0); }
	inline BOOL		flagObjectModify() const		{ return ((mFlags & FLAGS_OBJECT_MODIFY) != 0); }
	inline BOOL		flagObjectCopy() const			{ return ((mFlags & FLAGS_OBJECT_COPY) != 0); }
	inline BOOL		flagObjectMove() const			{ return ((mFlags & FLAGS_OBJECT_MOVE) != 0); }
	inline BOOL		flagObjectTransfer() const		{ return ((mFlags & FLAGS_OBJECT_TRANSFER) != 0); }
	inline BOOL		flagObjectPermanent() const		{ return ((mFlags & FLAGS_AFFECTS_NAVMESH) != 0); }
	inline BOOL		flagCharacter() const			{ return ((mFlags & FLAGS_CHARACTER) != 0); }
	inline BOOL		flagVolumeDetect() const		{ return ((mFlags & FLAGS_VOLUME_DETECT) != 0); }
	inline BOOL		flagIncludeInSearch() const     { return ((mFlags & FLAGS_INCLUDE_IN_SEARCH) != 0); }
	inline BOOL		flagScripted() const			{ return ((mFlags & FLAGS_SCRIPTED) != 0); }
	inline BOOL		flagHandleTouch() const			{ return ((mFlags & FLAGS_HANDLE_TOUCH) != 0); }
	inline BOOL		flagTakesMoney() const			{ return ((mFlags & FLAGS_TAKES_MONEY) != 0); }
	inline BOOL		flagPhantom() const				{ return ((mFlags & FLAGS_PHANTOM) != 0); }
	inline BOOL		flagInventoryEmpty() const		{ return ((mFlags & FLAGS_INVENTORY_EMPTY) != 0); }
	inline BOOL		flagAllowInventoryAdd() const	{ return ((mFlags & FLAGS_ALLOW_INVENTORY_DROP) != 0); }
	inline BOOL		flagTemporary() const			{ return ((mFlags & FLAGS_TEMPORARY) != 0); }
	inline BOOL		flagTemporaryOnRez() const		{ return ((mFlags & FLAGS_TEMPORARY_ON_REZ) != 0); }
	inline BOOL		flagAnimSource() const			{ return ((mFlags & FLAGS_ANIM_SOURCE) != 0); }
	inline BOOL		flagCameraSource() const		{ return ((mFlags & FLAGS_CAMERA_SOURCE) != 0); }
	inline BOOL		flagCameraDecoupled() const		{ return ((mFlags & FLAGS_CAMERA_DECOUPLED) != 0); }
	inline bool     getPhysicsShapeUnknown() const  { return mPhysicsShapeUnknown; }
	U8       getPhysicsShapeType() const;
	inline F32      getPhysicsGravity() const       { return mPhysicsGravity; }
	inline F32      getPhysicsFriction() const      { return mPhysicsFriction; }
	inline F32      getPhysicsDensity() const       { return mPhysicsDensity; }
	inline F32      getPhysicsRestitution() const   { return mPhysicsRestitution; }
	bool            isPermanentEnforced() const;
	bool getIncludeInSearch() const;
	void setIncludeInSearch(bool include_in_search);
	BOOL allowOpen() const;
	void setClickAction(U8 action) { mClickAction = action; }
	U8 getClickAction() const { return mClickAction; }
	bool specialHoverCursor() const;
	void			setRegion(LLViewerRegion *regionp);
	virtual void	updateRegion(LLViewerRegion *regionp);
	void updateFlags(BOOL physics_changed = FALSE);
	void loadFlags(U32 flags);
	BOOL setFlags(U32 flag, BOOL state);
	BOOL setFlagsWithoutUpdate(U32 flag, BOOL state);
	void setPhysicsShapeType(U8 type);
	void setPhysicsGravity(F32 gravity);
	void setPhysicsFriction(F32 friction);
	void setPhysicsDensity(F32 density);
	void setPhysicsRestitution(F32 restitution);
	virtual void dump() const;
	static U32		getNumZombieObjects()			{ return sNumZombieObjects; }
	void printNameValuePairs() const;
	virtual S32 getLOD() const { return 3; }
	virtual U32 getPartitionType() const;
	virtual void dirtySpatialGroup(BOOL priority = FALSE) const;
	virtual void dirtyMesh();
	const LLFlexibleObjectData* getFlexibleObjectData() const { return mFlexibleObjectDataInUse ? &mFlexibleObjectData : nullptr; }
	const LLLightParams* getLightParams() const { return mLightParamsInUse ? &mLightParams : nullptr; }
	const LLSculptParams* getSculptParams() const { return mSculptParamsInUse ? &mSculptParams : nullptr; }
	const LLLightImageParams* getLightImageParams() const { return mLightImageParamsInUse ? &mLightImageParams : nullptr; }
	const LLExtendedMeshParams* getExtendedMeshParams() const { return mExtendedMeshParamsInUse ? &mExtendedMeshParams : nullptr; }
	bool setParameterEntry(U16 param_type, const LLNetworkData& new_value, bool local_origin);
	bool setParameterEntryInUse(U16 param_type, BOOL in_use, bool local_origin);
	virtual void parameterChanged(U16 param_type, bool local_origin);
	virtual void parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin);
	friend class LLViewerObjectList;
	friend class LLViewerMediaList;
public:
	LLViewerTexture* getBakedTextureForMagicId(const LLUUID& id);
	void updateAvatarMeshVisibility(const LLUUID& id, const LLUUID& old_id);
	void refreshBakeTexture();
public:
	static void unpackVector3(LLDataPackerBinaryBuffer* dp, LLVector3& value, std::string name);
	static void unpackUUID(LLDataPackerBinaryBuffer* dp, LLUUID& value, std::string name);
	static void unpackU32(LLDataPackerBinaryBuffer* dp, U32& value, std::string name);
	static void unpackU8(LLDataPackerBinaryBuffer* dp, U8& value, std::string name);
	static U32 unpackParentID(LLDataPackerBinaryBuffer* dp, U32& parent_id);
public:
	void resetChildrenPosition(const LLVector3& offset, BOOL simplified = FALSE,  BOOL skip_avatar_child = FALSE) ;
	void resetChildrenRotationAndPosition(const std::vector<LLQuaternion>& rotations,
											const std::vector<LLVector3>& positions) ;
	void saveUnselectedChildrenRotation(std::vector<LLQuaternion>& rotations) ;
	void saveUnselectedChildrenPosition(std::vector<LLVector3>& positions) ;
	std::vector<LLVector3> mUnselectedChildrenPositions ;
private:
	ExtraParameter* createNewParameterEntry(U16 param_type);
	const ExtraParameter& getExtraParameterEntry(U16 param_type) const
	{
		return mExtraParameterList[U32(param_type >> 4) - 1];
	}
	ExtraParameter& getExtraParameterEntry(U16 param_type)
	{
		return mExtraParameterList[U32(param_type >> 4) - 1];
	}
	ExtraParameter* getExtraParameterEntryCreate(U16 param_type)
	{
		if (param_type <= LLNetworkData::PARAMS_MAX)
		{
			ExtraParameter& param = getExtraParameterEntry(param_type);
			if (!param.is_invalid)
			{
				if (!param.data)
				{
					ExtraParameter* new_entry = createNewParameterEntry(param_type);
					return new_entry;
				}
				return &param;
			}
		}
		return nullptr;
	}
	bool unpackParameterEntry(U16 param_type, LLDataPacker* dp);
    U32 checkMediaURL(const std::string &media_url);
	void interpolateLinearMotion(const F64SecondsImplicit & time, const F32SecondsImplicit & dt);
	static void initObjectDataMap();
	void fetchInventoryFromServer();
public:
	void print();
	typedef enum e_vo_types
	{
		LL_VO_CLOUDS =				LL_PCODE_APP | 0x20,
		LL_VO_SURFACE_PATCH =		LL_PCODE_APP | 0x30,
		LL_VO_WL_SKY =				LL_PCODE_APP | 0x40,
		LL_VO_SQUARE_TORUS =		LL_PCODE_APP | 0x50,
		LL_VO_SKY =					LL_PCODE_APP | 0x60,
		LL_VO_VOID_WATER =			LL_PCODE_APP | 0x70,
		LL_VO_WATER =				LL_PCODE_APP | 0x80,
		LL_VO_GROUND =				LL_PCODE_APP | 0x90,
		LL_VO_PART_GROUP =			LL_PCODE_APP | 0xa0,
		LL_VO_TRIANGLE_TORUS =		LL_PCODE_APP | 0xb0,
		LL_VO_HUD_PART_GROUP =		LL_PCODE_APP | 0xc0,
	} EVOType;
	typedef enum e_physics_shape_types
	{
		PHYSICS_SHAPE_PRIM = 0,
		PHYSICS_SHAPE_NONE,
		PHYSICS_SHAPE_CONVEX_HULL,
	} EPhysicsShapeType;
	LLUUID			mID;
	LLUUID			mOwnerID;
	U32				mLocalID;
	U32				mTotalCRC;
	S32				mListIndex;
	LLPointer<LLViewerTexture> *mTEImages;
	LLPointer<LLViewerTexture> *mTENormalMaps;
	LLPointer<LLViewerTexture> *mTESpecularMaps;
	U32				mGLName;
	BOOL			mbCanSelect;
private:
	U32				mFlags;
	static std::map<std::string, U32> sObjectDataMap;
public:
	U8              mPhysicsShapeType;
	F32             mPhysicsGravity;
	F32             mPhysicsFriction;
	F32             mPhysicsDensity;
	F32             mPhysicsRestitution;
	LLPointer<LLDrawable> mDrawable;
	BOOL mCreateSelected;
	BOOL mRenderMedia;
	S32				mBestUpdatePrecision;
	LLPointer<LLHUDText> mText;
	std::string mHudTextString;
	LLColor4 mHudTextColor;
	LLPointer<LLHUDIcon> mIcon;
	bool mIsNameAttachment;
	static			BOOL		sUseSharedDrawables;
public:
    LLControlAvatar *getControlAvatar();
    LLControlAvatar *getControlAvatar() const;
    void linkControlAvatar();
    void unlinkControlAvatar();
    void updateControlAvatar();
    virtual bool isAnimatedObject() const;
    static const S32 CO_FLAG_CONTROL_AVATAR = 1 << 0;
    static const S32 CO_FLAG_UI_AVATAR = 1 << 1;
protected:
    LLPointer<LLControlAvatar> mControlAvatar;
protected:
	void deleteInventoryItem(const LLUUID& item_id);
	void doUpdateInventory(LLPointer<LLViewerInventoryItem>& item, U8 key, bool is_new);
	static LLViewerObject *createObject(const LLUUID &id, LLPCode pcode, LLViewerRegion *regionp, S32 flags = 0);
	BOOL setData(const U8 *datap, const U32 data_size);
	void	hideExtraDisplayItems( BOOL hidden );
	static void processTaskInvFile(void** user_data, S32 error_code, LLExtStat ext_status);
	BOOL loadTaskInvFile(const std::string& filename);
	void doInventoryCallback();
	BOOL isOnMap();
	void unpackParticleSource(const S32 block_num, const LLUUID& owner_id);
	void unpackParticleSource(LLDataPacker &dp, const LLUUID& owner_id, bool legacy);
	void deleteParticleSource();
	void setParticleSource(const LLPartSysData& particle_parameters, const LLUUID& owner_id);
private:
	void setNameValueList(const std::string& list);
	void deleteTEImages();
protected:
	typedef std::map<char *, LLNameValue *> name_value_map_t;
	name_value_map_t mNameValuePairs;
	child_list_t	mChildList;
	F64Seconds		mLastInterpUpdateSecs;
	F64Seconds		mLastMessageUpdateSecs;
	TPACKETID		mLatestRecvPacketID;
	U8* mData;
public:
	LLPointer<LLViewerPartSourceScript>		mPartSourcep;
protected:
	LLAudioSourceVO* mAudioSourcep;
	F32				mAudioGain;
	F32				mAppAngle;
	F32				mPixelArea;
	std::list<LLUUID> mPendingInventoryItemsIDs;
	LLInventoryObject::object_list_t* mInventory;
	class LLInventoryCallbackInfo
	{
	public:
		~LLInventoryCallbackInfo();
		LLVOInventoryListener* mListener;
		void* mInventoryData;
	};
	typedef std::list<LLInventoryCallbackInfo*> callback_list_t;
	callback_list_t mInventoryCallbacks;
	S16 mInventorySerialNum;
	enum EInventoryRequestState
	{
		INVENTORY_REQUEST_STOPPED,
		INVENTORY_REQUEST_PENDING,
		INVENTORY_XFER
	};
	EInventoryRequestState	mInvRequestState;
	U64						mInvRequestXFerId;
	BOOL					mInventoryDirty;
	LLViewerRegion	*mRegionp;
	BOOL			mDead;
	BOOL			mOrphaned;
	BOOL			mUserSelected;
	BOOL			mOnActiveList;
	BOOL			mOnMap;
	BOOL			mStatic;
	S32				mNumFaces;
	F32				mRotTime;
	LLQuaternion	mAngularVelocityRot;
	LLQuaternion	mPreviousRotation;
	U8				mAttachmentState;
	LLViewerObjectMedia* mMedia;
	U8 mClickAction;
	F32 mObjectCost;
	F32 mLinksetCost;
	F32 mPhysicsCost;
	F32 mLinksetPhysicsCost;
	bool mCostStale;
	mutable bool mPhysicsShapeUnknown;
	static			U32			sNumZombieObjects;
	static			BOOL		sMapDebug;
	static			LLColor4	sEditSelectColor;
	static			LLColor4	sNoEditSelectColor;
	static			F32			sCurrentPulse;
	static			BOOL		sPulseEnabled;
	static			S32			sAxisArrowLength;
	mutable LLVector3		mPositionRegion;
	mutable LLVector3		mPositionAgent;
	static void setPhaseOutUpdateInterpolationTime(F32 value)	{ sPhaseOutUpdateInterpolationTime = (F64Seconds) value;	}
	static void setMaxUpdateInterpolationTime(F32 value)		{ sMaxUpdateInterpolationTime = (F64Seconds) value;	}
	static void	setVelocityInterpolate(BOOL value)		{ sVelocityInterpolate = value;	}
	static void	setPingInterpolate(BOOL value)			{ sPingInterpolate = value;	}
private:
	static S32 sNumObjects;
	static F64Seconds sPhaseOutUpdateInterpolationTime;
	static F64Seconds sMaxUpdateInterpolationTime;
	static BOOL sVelocityInterpolate;
	static BOOL sPingInterpolate;
public:
	std::string getAttachmentPointName() const;
	const LLUUID &getAttachmentItemID() const;
	void setAttachmentItemID(const LLUUID &id);
	const LLUUID &extractAttachmentItemID();
	EObjectUpdateType getLastUpdateType() const;
	void setLastUpdateType(EObjectUpdateType last_update_type);
	BOOL getLastUpdateCached() const;
	void setLastUpdateCached(BOOL last_update_cached);
    virtual void updateRiggingInfo() {}
    LLJointRiggingInfoTab mJointRiggingInfoTab;
private:
	LLUUID mAttachmentItemID;
	EObjectUpdateType	mLastUpdateType;
	BOOL	mLastUpdateCached;
};
inline void LLViewerObject::setRotation(const LLQuaternion& quat, BOOL damped)
{
	LLPrimitive::setRotation(quat);
	setChanged(ROTATED | SILHOUETTE);
	updateDrawable(damped);
}
inline void LLViewerObject::setRotation(const F32 x, const F32 y, const F32 z, BOOL damped)
{
	LLPrimitive::setRotation(x, y, z);
	setChanged(ROTATED | SILHOUETTE);
	updateDrawable(damped);
}
class LLViewerObjectMedia
{
public:
	LLViewerObjectMedia() : mMediaURL(), mPassedWhitelist(FALSE), mMediaType(0) { }
	std::string mMediaURL;
	BOOL mPassedWhitelist;
	U8 mMediaType;
};
class LLAlphaObject : public LLViewerObject
{
public:
	LLAlphaObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLViewerObject(id,pcode,regionp)
	{ mDepth = 0.f; }
	virtual F32 getPartSize(S32 idx);
	virtual void getGeometry(S32 idx,
								LLStrider<LLVector4a>& verticesp,
								LLStrider<LLVector3>& normalsp,
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp,
								LLStrider<LLColor4U>& emissivep,
								LLStrider<U16>& indicesp) = 0;
	virtual void getBlendFunc(S32 face, U32& src, U32& dst);
	F32 mDepth;
};
class LLStaticViewerObject : public LLViewerObject
{
public:
	LLStaticViewerObject(const LLUUID& id, const LLPCode pcode, LLViewerRegion* regionp, BOOL is_global = FALSE)
		: LLViewerObject(id, pcode, regionp, is_global)
	{ }
	virtual void updateDrawable(BOOL force_damped);
};
#endif
