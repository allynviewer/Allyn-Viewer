/** 
 * @file llselectmgr.h
 * @brief A manager for selected objects and TEs.
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
#ifndef LL_LLSELECTMGR_H
#define LL_LLSELECTMGR_H
#include "llcharacter.h"
#include "lleditmenuhandler.h"
#include "llundo.h"
#include "lluuid.h"
#include "llpointer.h"
#include "llsafehandle.h"
#include "llsaleinfo.h"
#include "llcategory.h"
#include "v3dmath.h"
#include "llquaternion.h"
#include "llcoord.h"
#include "llframetimer.h"
#include "llbbox.h"
#include "llpermissions.h"
#include "llcontrol.h"
#include "llviewerobject.h"
#include "llmaterial.h"
#include <deque>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/signals2.hpp>
class LLMessageSystem;
class LLViewerTexture;
class LLColor4;
class LLVector3;
class LLSelectNode;
const S32 SELECT_ALL_TES = -1;
const S32 SELECT_MAX_TES = 32;
struct LLSelectedObjectFunctor
{
	virtual ~LLSelectedObjectFunctor() {};
	virtual bool apply(LLViewerObject* object) = 0;
};
struct LLSelectedNodeFunctor
{
	virtual ~LLSelectedNodeFunctor() {};
	virtual bool apply(LLSelectNode* node) = 0;
};
struct LLSelectedTEFunctor
{
	virtual ~LLSelectedTEFunctor() {};
	virtual bool apply(LLViewerObject* object, S32 face) = 0;
};
struct LLSelectedTEMaterialFunctor
{
	virtual ~LLSelectedTEMaterialFunctor() {};
	virtual LLMaterialPtr apply(LLViewerObject* object, S32 face, LLTextureEntry* tep, LLMaterialPtr& current_material) = 0;
};
template <typename T> struct LLSelectedTEGetFunctor
{
	virtual ~LLSelectedTEGetFunctor() {};
	virtual T get(LLViewerObject* object, S32 te) = 0;
};
typedef enum e_send_type
{
	SEND_ONLY_ROOTS,
	SEND_INDIVIDUALS,
	SEND_ROOTS_FIRST,
	SEND_CHILDREN_FIRST
} ESendType;
typedef enum e_grid_mode
{
	GRID_MODE_WORLD,
	GRID_MODE_LOCAL,
	GRID_MODE_REF_OBJECT
} EGridMode;
typedef enum e_action_type
{
	SELECT_ACTION_TYPE_BEGIN,
	SELECT_ACTION_TYPE_PICK,
	SELECT_ACTION_TYPE_MOVE,
	SELECT_ACTION_TYPE_ROTATE,
	SELECT_ACTION_TYPE_SCALE,
	NUM_ACTION_TYPES
}EActionType;
typedef enum e_selection_type
{
	SELECT_TYPE_WORLD,
	SELECT_TYPE_ATTACHMENT,
	SELECT_TYPE_HUD
}ESelectType;
class LLSelectNode
{
public:
	LLSelectNode(LLViewerObject* object, BOOL do_glow);
	LLSelectNode(const LLSelectNode& nodep);
	~LLSelectNode();
	void selectAllTEs(BOOL b);
	void selectTE(S32 te_index, BOOL selected);
	BOOL isTESelected(S32 te_index);
	S32 getLastSelectedTE();
	S32 getLastOperatedTE();
	S32 getTESelectMask() { return mTESelectMask; }
	void renderOneWireframe(const LLColor4& color);
	void renderOneSilhouette(const LLColor4 &color);
	void setTransient(BOOL transient) { mTransient = transient; }
	BOOL isTransient() { return mTransient; }
	LLViewerObject* getObject();
	void setObject(LLViewerObject* object);
	void saveColors();
	void saveShinyColors();
	void saveTextures(const uuid_vec_t& textures);
	void saveTextureScaleRatios(LLRender::eTexIndex index_to_query);
	BOOL allowOperationOnNode(PermissionBit op, U64 group_proxy_power) const;
public:
	BOOL			mIndividualSelection;
	BOOL			mTransient;
	BOOL			mValid;
	LLPermissions*	mPermissions;
	LLSaleInfo		mSaleInfo;
	LLAggregatePermissions mAggregatePerm;
	LLAggregatePermissions mAggregateTexturePerm;
	LLAggregatePermissions mAggregateTexturePermOwner;
	std::string		mName;
	std::string		mDescription;
	LLCategory		mCategory;
	S16				mInventorySerial;
	LLVector3		mSavedPositionLocal;
	LLVector3		mLastPositionLocal;
	LLVector3d		mSavedPositionGlobal;
	LLVector3		mSavedScale;
	LLVector3		mLastScale;
	LLQuaternion	mSavedRotation;
	LLQuaternion	mLastRotation;
	BOOL			mDuplicated;
	LLVector3d		mDuplicatePos;
	LLQuaternion	mDuplicateRot;
	LLUUID			mItemID;
	LLUUID			mFolderID;
	LLUUID			mFromTaskID;
	std::string		mTouchName;
	std::string		mSitName;
	U64				mCreationDate;
	std::vector<LLColor4>	mSavedColors;
	std::vector<LLColor4>	mSavedShinyColors;
	uuid_vec_t		mSavedTextures;
	std::vector<LLVector3>  mTextureScaleRatios;
	std::vector<LLVector3>	mSilhouetteVertices;
	std::vector<LLVector3>	mSilhouetteNormals;
	BOOL					mSilhouetteExists;
protected:
	LLPointer<LLViewerObject>	mObject;
	S32				mTESelectMask;
	S32				mLastTESelected;
};
class LLObjectSelection : public LLRefCount
{
	friend class LLSelectMgr;
	friend class LLSafeHandle<LLObjectSelection>;
protected:
	~LLObjectSelection();
public:
	typedef std::list<LLSelectNode*> list_t;
	struct is_non_null
	{
		bool operator()(LLSelectNode* node)
		{
			return (node->getObject() != nullptr);
		}
	};
	typedef boost::filter_iterator<is_non_null, list_t::iterator > iterator;
	iterator begin() { return iterator(mList.begin(), mList.end()); }
	iterator end() { return iterator(mList.end(), mList.end()); }
	struct is_valid
	{
		bool operator()(LLSelectNode* node)
		{
			return (node->getObject() != nullptr) && node->mValid;
		}
	};
	typedef boost::filter_iterator<is_valid, list_t::iterator > valid_iterator;
	valid_iterator valid_begin() { return valid_iterator(mList.begin(), mList.end()); }
	valid_iterator valid_end() { return valid_iterator(mList.end(), mList.end()); }
	struct is_root
	{
		bool operator()(LLSelectNode* node);
	};
	typedef boost::filter_iterator<is_root, list_t::iterator > root_iterator;
	root_iterator root_begin() { return root_iterator(mList.begin(), mList.end()); }
	root_iterator root_end() { return root_iterator(mList.end(), mList.end()); }
	struct is_valid_root
	{
		bool operator()(LLSelectNode* node);
	};
	typedef boost::filter_iterator<is_valid_root, list_t::iterator > valid_root_iterator;
	valid_root_iterator valid_root_begin() { return valid_root_iterator(mList.begin(), mList.end()); }
	valid_root_iterator valid_root_end() { return valid_root_iterator(mList.end(), mList.end()); }
	struct is_root_object
	{
		bool operator()(LLSelectNode* node);
	};
	typedef boost::filter_iterator<is_root_object, list_t::iterator > root_object_iterator;
	root_object_iterator root_object_begin() { return root_object_iterator(mList.begin(), mList.end()); }
	root_object_iterator root_object_end() { return root_object_iterator(mList.end(), mList.end()); }
public:
	LLObjectSelection();
	void updateEffects();
	BOOL isEmpty() const;
	LLSelectNode*	getFirstNode(LLSelectedNodeFunctor* func = nullptr);
	LLSelectNode*	getFirstRootNode(LLSelectedNodeFunctor* func = nullptr, BOOL non_root_ok = FALSE);
	LLViewerObject* getFirstSelectedObject(LLSelectedNodeFunctor* func, BOOL get_parent = FALSE);
	LLViewerObject*	getFirstObject();
	LLViewerObject*	getFirstRootObject(BOOL non_root_ok = FALSE);
	LLSelectNode*	getFirstMoveableNode(BOOL get_root_first = FALSE);
	LLViewerObject*	getFirstEditableObject(BOOL get_parent = FALSE);
	LLViewerObject*	getFirstCopyableObject(BOOL get_parent = FALSE);
	LLViewerObject* getFirstDeleteableObject();
	LLViewerObject*	getFirstMoveableObject(BOOL get_parent = FALSE);
	LLViewerObject* getPrimaryObject() { return mPrimaryObject; }
	template <typename T> bool getSelectedTEValue(LLSelectedTEGetFunctor<T>* func, T& res);
	template <typename T> bool isMultipleTEValue(LLSelectedTEGetFunctor<T>* func, const T& ignore_value);
	S32 getNumNodes();
	LLSelectNode* findNode(LLViewerObject* objectp);
	S32 getObjectCount();
	F32 getSelectedObjectCost();
	F32 getSelectedLinksetCost();
	F32 getSelectedPhysicsCost();
	F32 getSelectedLinksetPhysicsCost();
	S32 getSelectedObjectRenderCost();
	F32 getSelectedObjectStreamingCost(S32* total_bytes = nullptr, S32* visible_bytes = nullptr);
	U32 getSelectedObjectTriangleCount(S32* vcount = nullptr);
	S32 getTECount();
	S32 getRootObjectCount();
	BOOL isMultipleTESelected();
	BOOL contains(LLViewerObject* object);
	BOOL contains(LLViewerObject* object, S32 te);
	BOOL isAttachment();
	bool checkAnimatedObjectEstTris();
	bool checkAnimatedObjectLinkable();
	bool applyToRootObjects(LLSelectedObjectFunctor* func, bool firstonly = false);
	bool applyToObjects(LLSelectedObjectFunctor* func);
	bool applyToTEs(LLSelectedTEFunctor* func, bool firstonly = false);
	bool applyToRootNodes(LLSelectedNodeFunctor* func, bool firstonly = false);
	bool applyToNodes(LLSelectedNodeFunctor* func, bool firstonly = false);
	void applyNoCopyTextureToTEs(LLViewerInventoryItem* item);
	ESelectType getSelectType() const { return mSelectType; }
private:
	void addNode(LLSelectNode *nodep);
	void addNodeAtEnd(LLSelectNode *nodep);
	void moveNodeToFront(LLSelectNode *nodep);
	void removeNode(LLSelectNode *nodep);
	void deleteAllNodes();
	void cleanupNodes();
	list_t mList;
	const LLObjectSelection& operator=(const LLObjectSelection&) = delete;
	LLPointer<LLViewerObject> mPrimaryObject;
	std::map<LLPointer<LLViewerObject>, LLSelectNode*> mSelectNodeMap;
	ESelectType mSelectType;
};
typedef LLSafeHandle<LLObjectSelection> LLObjectSelectionHandle;
#ifndef LLSELECTMGR_CPP
extern template class LLSelectMgr* LLSingleton<class LLSelectMgr>::getInstance();
#endif
struct LLSelectGetFirstTest;
class LLSelectMgr : public LLEditMenuHandler, public LLSingleton<LLSelectMgr>
{
public:
	static BOOL					sRectSelectInclusive;
	static BOOL					sRenderHiddenSelections;
	static BOOL					sRenderLightRadius;
	static F32					sHighlightThickness;
	static F32					sHighlightUScale;
	static F32					sHighlightVScale;
	static F32					sHighlightAlpha;
	static F32					sHighlightAlphaTest;
	static F32					sHighlightUAnim;
	static F32					sHighlightVAnim;
	static LLColor4				sSilhouetteParentColor;
	static LLColor4				sSilhouetteChildColor;
	static LLColor4				sHighlightParentColor;
	static LLColor4				sHighlightChildColor;
	static LLColor4				sHighlightInspectColor;
	static LLColor4				sContextSilhouetteColor;
	LLCachedControl<bool>					mHideSelectedObjects;
	LLCachedControl<bool>					mRenderHighlightSelections;
	LLCachedControl<bool>					mAllowSelectAvatar;
	LLCachedControl<bool>					mDebugSelectMgr;
public:
	LLSelectMgr();
	~LLSelectMgr();
	static void cleanupGlobals();
	BOOL canUndo() const override;
	void undo() override;
	BOOL canRedo() const override;
	void redo() override;
	BOOL canDoDelete() const override;
	void doDelete() override;
	void selectAll() override;
	BOOL canSelectAll() const override;
	void deselect() override;
	BOOL canDeselect() const override;
	virtual void duplicate();
	virtual BOOL canDuplicate() const;
	void clearSelections();
	void update();
	void updateEffects();
	void overrideObjectUpdates();
	BOOL setForceSelection(BOOL force);
	LLObjectSelectionHandle selectObjectAndFamily(LLViewerObject* object, BOOL add_to_end = FALSE, BOOL ignore_select_owned = FALSE);
	LLObjectSelectionHandle selectObjectOnly(LLViewerObject* object, S32 face = SELECT_ALL_TES);
	LLObjectSelectionHandle selectObjectAndFamily(const std::vector<LLViewerObject*>& object_list, BOOL send_to_sim = TRUE);
	LLObjectSelectionHandle selectHighlightedObjects();
	LLObjectSelectionHandle setHoverObject(LLViewerObject *objectp, S32 face = -1);
	LLSelectNode *getHoverNode();
	LLSelectNode *getPrimaryHoverNode();
	void highlightObjectOnly(LLViewerObject *objectp);
	void highlightObjectAndFamily(LLViewerObject *objectp);
	void highlightObjectAndFamily(const std::vector<LLViewerObject*>& list);
	void deselectObjectOnly(LLViewerObject* object, BOOL send_to_sim = TRUE);
	void deselectObjectAndFamily(LLViewerObject* object, BOOL send_to_sim = TRUE, BOOL include_entire_object = FALSE);
	void deselectAll();
	void deselectAllForStandingUp();
	void deselectUnused();
	void deselectAllIfTooFar();
	void deselectHighlightedObjects();
	void unhighlightObjectOnly(LLViewerObject *objectp);
	void unhighlightObjectAndFamily(LLViewerObject *objectp);
	void unhighlightAll();
	BOOL removeObjectFromSelections(const LLUUID &id);
	bool linkObjects();
	bool unlinkObjects();
	bool enableLinkObjects();
	bool enableUnlinkObjects();
	LLObjectSelectionHandle getHoverObjects() { return mHoverObjects; }
	LLObjectSelectionHandle	getSelection() { return mSelectedObjects; }
	LLObjectSelectionHandle	getEditSelection() { convertTransient(); return mSelectedObjects; }
	LLObjectSelectionHandle	getHighlightedObjects() { return mHighlightedObjects; }
	void			addGridObject(LLViewerObject* objectp);
	void			clearGridObjects();
	void			setGridMode(EGridMode mode);
	EGridMode		getGridMode() { return mGridMode; }
	void			getGrid(LLVector3& origin, LLQuaternion& rotation, LLVector3 &scale, bool for_snap_guides = false);
	BOOL getTEMode()		{ return mTEMode; }
	void setTEMode(BOOL b)	{ mTEMode = b; }
	BOOL shouldShowSelection()	{ return mShowSelection; }
	LLBBox getBBoxOfSelection() const;
	LLBBox getSavedBBoxOfSelection() const { return mSavedSelectionBBox; }
	void dump();
	void cleanup();
	void updateSilhouettes();
	void renderSilhouettes(BOOL for_hud);
	void enableSilhouette(BOOL enable) { mRenderSilhouettes = enable; }
	void saveSelectedObjectTransform(EActionType action_type);
	void saveSelectedObjectColors();
	void saveSelectedShinyColors();
	void saveSelectedObjectTextures();
	void setTextureChannel(LLRender::eTexIndex texIndex) { mTextureChannel = texIndex; }
	LLRender::eTexIndex getTextureChannel() { return mTextureChannel; }
	void selectionUpdatePhysics(BOOL use_physics);
	void selectionUpdateTemporary(BOOL is_temporary);
	void selectionUpdatePhantom(BOOL is_ghost);
	void selectionDump();
	BOOL selectionAllPCode(LLPCode code);
	BOOL selectionGetClickAction(U8 *out_action);
	bool selectionGetIncludeInSearch(bool* include_in_search_out);
	BOOL selectionGetGlow(F32 *glow);
	void selectionSetPhysicsType(U8 type);
	void selectionSetGravity(F32 gravity);
	void selectionSetFriction(F32 friction);
	void selectionSetDensity(F32 density);
	void selectionSetRestitution(F32 restitution);
	void selectionSetMaterial(U8 material);
	void selectionSetImage(const LLUUID& imageid);
	void selectionSetColor(const LLColor4 &color);
	void selectionSetColorOnly(const LLColor4 &color);
	void selectionSetAlphaOnly(const F32 alpha);
	void selectionRevertColors();
	void selectionRevertShinyColors();
	BOOL selectionRevertTextures();
	void selectionSetBumpmap( U8 bumpmap );
	void selectionSetTexGen( U8 texgen );
	void selectionSetShiny( U8 shiny );
	void selectionSetFullbright( U8 fullbright );
	void selectionSetMedia( U8 media_type, const LLSD &media_data );
	void selectionSetClickAction(U8 action);
	void selectionSetIncludeInSearch(bool include_in_search);
	void selectionSetGlow(const F32 glow);
	void selectionSetMaterialParams(LLSelectedTEMaterialFunctor* material_func);
	void selectionRemoveMaterial();
	void selectionSetObjectPermissions(U8 perm_field, BOOL set, U32 perm_mask, BOOL override = FALSE);
	void selectionSetObjectName(const std::string& name);
	void selectionSetObjectDescription(const std::string& desc);
	void selectionSetObjectCategory(const LLCategory& category);
	void selectionSetObjectSaleInfo(const LLSaleInfo& sale_info);
	void selectionTexScaleAutofit(F32 repeats_per_meter);
	void adjustTexturesByScale(BOOL send_to_sim, BOOL stretch);
	bool selectionMove(const LLVector3& displ, F32 rx, F32 ry, F32 rz,
					   U32 update_type);
	void sendSelectionMove();
	void sendGodlikeRequest(const std::string& request, const std::string& parameter);
	void validateSelection();
	BOOL canSelectObject(LLViewerObject* object, BOOL ignore_select_owned = FALSE);
	BOOL selectGetAllRootsValid();
	BOOL selectGetAllValid();
	BOOL selectGetAllValidAndObjectsFound();
	BOOL selectGetRootsModify();
	BOOL selectGetModify();
	BOOL selectGetSameRegion();
	BOOL selectGetRootsNonPermanentEnforced();
	BOOL selectGetNonPermanentEnforced();
	BOOL selectGetRootsPermanent();
	BOOL selectGetPermanent();
	BOOL selectGetRootsCharacter();
	BOOL selectGetCharacter();
	BOOL selectGetRootsNonPathfinding();
	BOOL selectGetNonPathfinding();
	BOOL selectGetRootsNonPermanent();
	BOOL selectGetNonPermanent();
	BOOL selectGetRootsNonCharacter();
	BOOL selectGetNonCharacter();
	BOOL selectGetEditableLinksets();
	BOOL selectGetViewableCharacters();
	BOOL selectGetRootsTransfer();
	BOOL selectGetRootsCopy();
	BOOL selectGetCreator(LLUUID& id, std::string& name);
	BOOL selectGetOwner(LLUUID& id, std::string& name);
	BOOL selectGetLastOwner(LLUUID& id, std::string& name);
	BOOL selectGetGroup(LLUUID& id);
	BOOL selectGetPerm(	U8 which_perm, U32* mask_on, U32* mask_off);
	BOOL selectIsGroupOwned();
	BOOL selectGetPermissions(LLPermissions& perm);
	void selectGetAggregateSaleInfo(U32 &num_for_sale,
									BOOL &is_for_sale_mixed,
									BOOL &is_sale_price_mixed,
									S32 &total_sale_price,
									S32 &individual_sale_price);
	BOOL selectGetCategory(LLCategory& category);
	BOOL selectGetSaleInfo(LLSaleInfo& sale_info);
	BOOL selectGetAggregatePermissions(LLAggregatePermissions& ag_perm);
	BOOL selectGetAggregateTexturePermissions(LLAggregatePermissions& ag_perm);
	LLPermissions* findObjectPermissions(const LLViewerObject* object);
	void selectDelete();
	void selectForceDelete();
	void selectDuplicate(const LLVector3& offset, BOOL select_copy);
	void repeatDuplicate();
	void selectDuplicateOnRay(const LLVector3 &ray_start_region,
								const LLVector3 &ray_end_region,
								BOOL bypass_raycast,
								BOOL ray_end_is_intersection,
								const LLUUID &ray_target_id,
								BOOL copy_centers,
								BOOL copy_rotates,
								BOOL select_copy);
	void sendMultipleUpdate(U32 type);
	void sendOwner(const LLUUID& owner_id, const LLUUID& group_id, BOOL override = FALSE);
	void sendGroup(const LLUUID& group_id);
	void sendBuy(const LLUUID& buyer_id, const LLUUID& category_id, const LLSaleInfo sale_info);
	void sendAttach(U8 attachment_point, bool replace=true);
	void sendDetach();
	void sendDropAttachment();
	void sendLink();
	void sendDelink();
	void sendSelect();
	void requestObjectPropertiesFamily(LLViewerObject* object);
	static void processObjectProperties(LLMessageSystem *mesgsys, void **user_data);
	static void processObjectPropertiesFamily(LLMessageSystem *mesgsys, void **user_data);
	static void processForceObjectSelect(LLMessageSystem* msg, void**);
	void requestGodInfo();
	LLVector3d		getSelectionCenterGlobal() const	{ return mSelectionCenterGlobal; }
	void			updateSelectionCenter();
	void pauseAssociatedAvatars();
	void resetAgentHUDZoom();
	void setAgentHUDZoom(F32 target_zoom, F32 current_zoom);
	void getAgentHUDZoom(F32 &target_zoom, F32 &current_zoom) const;
	void updatePointAt();
	void remove(std::vector<LLViewerObject*>& objects);
	void remove(LLViewerObject* object, S32 te = SELECT_ALL_TES, BOOL undoable = TRUE);
	void removeAll();
	void addAsIndividual(LLViewerObject* object, S32 te = SELECT_ALL_TES, BOOL undoable = TRUE);
	void promoteSelectionToRoot();
	void demoteSelectionToIndividuals();
private:
	void convertTransient();
	ESelectType getSelectTypeForObject(LLViewerObject* object);
	void addAsFamily(std::vector<LLViewerObject*>& objects, BOOL add_to_end = FALSE);
	void generateSilhouette(LLSelectNode *nodep, const LLVector3& view_point);
	void updateSelectionSilhouette(LLObjectSelectionHandle object_handle, S32& num_sils_genned, std::vector<LLViewerObject*>& changed_objects);
	void sendListToRegions(	const std::string& message_name,
							void (*pack_header)(void *user_data),
							void (*pack_body)(LLSelectNode* node, void *user_data),
							void (*log_func)(LLSelectNode* node, void *user_data),
							void *user_data,
							ESendType send_type);
	static void packAgentID(	void *);
	static void packAgentAndSessionID(void* user_data);
	static void packAgentAndGroupID(void* user_data);
	static void packAgentAndSessionAndGroupID(void* user_data);
	static void packAgentIDAndSessionAndAttachment(void*);
	static void packAgentGroupAndCatID(void*);
	static void packDeleteHeader(void* userdata);
	static void packDeRezHeader(void* user_data);
	static void packObjectID(	LLSelectNode* node, void *);
	static void packObjectIDAsParam(LLSelectNode* node, void *);
	static void packObjectIDAndRotation(	LLSelectNode* node, void *);
	static void packObjectLocalID(LLSelectNode* node, void *);
	static void packObjectClickAction(LLSelectNode* node, void* data);
	static void packObjectIncludeInSearch(LLSelectNode* node, void* data);
	static void packObjectName(LLSelectNode* node, void* user_data);
	static void packObjectDescription(LLSelectNode* node, void* user_data);
	static void packObjectCategory(LLSelectNode* node, void* user_data);
	static void packObjectSaleInfo(LLSelectNode* node, void* user_data);
	static void packBuyObjectIDs(LLSelectNode* node, void* user_data);
	static void packDuplicate(	LLSelectNode* node, void *duplicate_data);
	static void packDuplicateHeader(void*);
	static void packDuplicateOnRayHead(void *user_data);
	static void packPermissions(LLSelectNode* node, void *user_data);
	static void packDeselect(	LLSelectNode* node, void *user_data);
	static void packMultipleUpdate(LLSelectNode* node, void *user_data);
	static void packPhysics(LLSelectNode* node, void *user_data);
	static void packShape(LLSelectNode* node, void *user_data);
	static void packOwnerHead(void *user_data);
	static void packHingeHead(void *user_data);
	static void packPermissionsHead(void* user_data);
	static void packGodlikeHead(void* user_data);
    static void logNoOp(LLSelectNode* node, void *user_data);
    static void logAttachmentRequest(LLSelectNode* node, void *user_data);
    static void logDetachRequest(LLSelectNode* node, void *user_data);
	static bool confirmDelete(const LLSD& notification, const LLSD& response, LLObjectSelectionHandle handle);
public:
	typedef boost::signals2::signal< void ()> update_signal_t;
	update_signal_t mUpdateSignal;
private:
	LLPointer<LLViewerTexture>				mSilhouetteImagep;
	LLObjectSelectionHandle					mSelectedObjects;
	LLObjectSelectionHandle					mHoverObjects;
	LLObjectSelectionHandle					mHighlightedObjects;
	std::set<LLPointer<LLViewerObject> >	mRectSelectedObjects;
	LLObjectSelection		mGridObjects;
	LLQuaternion			mGridRotation;
	LLVector3				mGridOrigin;
	LLVector3				mGridScale;
	EGridMode				mGridMode;
	BOOL					mTEMode;
	LLRender::eTexIndex	mTextureChannel;
	LLVector3d				mSelectionCenterGlobal;
	LLBBox					mSelectionBBox;
	LLVector3d				mLastSentSelectionCenterGlobal;
	BOOL					mShowSelection;
	LLVector3d				mLastCameraPos;
	BOOL					mRenderSilhouettes;
	LLBBox					mSavedSelectionBBox;
	LLFrameTimer			mEffectsTimer;
	BOOL					mForceSelection;
	std::vector<LLAnimPauseRequest> mPauseRequests;
	friend class LLObjectBackup;
};
void dialog_refresh_all();
template <typename T> bool LLObjectSelection::getSelectedTEValue(LLSelectedTEGetFunctor<T>* func, T& res)
{
	bool have_first = false;
	bool have_selected = false;
	T selected_value = T();
	bool identical = TRUE;
	for (iterator iter = begin(); iter != end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		S32 selected_te = -1;
		if (object == getPrimaryObject())
		{
			selected_te = node->getLastSelectedTE();
		}
		for (S32 te = 0; te < object->getNumTEs(); ++te)
		{
			if (!node->isTESelected(te))
			{
				continue;
			}
			T value = func->get(object, te);
			if (!have_first)
			{
				have_first = true;
				if (!have_selected)
				{
					selected_value = value;
				}
			}
			else
			{
				if ( value != selected_value )
				{
					identical = false;
				}
				if (te == selected_te)
				{
					selected_value = value;
					have_selected = true;
				}
			}
		}
		if (!identical && have_selected)
		{
			break;
		}
	}
	if (have_first || have_selected)
	{
		res = selected_value;
	}
	return identical;
}
template <typename T> bool LLObjectSelection::isMultipleTEValue(LLSelectedTEGetFunctor<T>* func, const T& ignore_value)
{
	bool have_first = false;
	T selected_value = T();
	bool unique = TRUE;
	for (iterator iter = begin(); iter != end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		for (S32 te = 0; te < object->getNumTEs(); ++te)
		{
			if (!node->isTESelected(te))
			{
				continue;
			}
			T value = func->get(object, te);
			if(value == ignore_value)
			{
				continue;
			}
			if (!have_first)
			{
				have_first = true;
			}
			else
			{
				if (value !=selected_value  )
				{
					unique = false;
					return !unique;
				}
			}
		}
	}
	return !unique;
}
#endif
