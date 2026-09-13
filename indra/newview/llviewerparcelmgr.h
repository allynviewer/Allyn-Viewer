/** 
 * @file llviewerparcelmgr.h
 * @brief Viewer-side representation of owned land
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
#ifndef LL_LLVIEWERPARCELMGR_H
#define LL_LLVIEWERPARCELMGR_H
#include "v3dmath.h"
#include "llframetimer.h"
#include "llsingleton.h"
#include "llparcelselection.h"
#include "llui.h"
class LLUUID;
class LLMessageSystem;
class LLParcel;
class LLViewerTexture;
class LLViewerRegion;
const F32 DWELL_NAN = -1.0f;
class LLParcelObserver
{
public:
	virtual ~LLParcelObserver() {};
	virtual void changed() = 0;
};
class LLViewerParcelMgr : public LLSingleton<LLViewerParcelMgr>
{
	friend class LLSingleton<LLViewerParcelMgr>;
	LLViewerParcelMgr();
	~LLViewerParcelMgr();
public:
	typedef std::function<void (const LLVector3d&)> teleport_finished_callback_t;
	typedef boost::signals2::signal<void (const LLVector3d&)> teleport_finished_signal_t;
	typedef std::function<void()> parcel_changed_callback_t;
	typedef boost::signals2::signal<void()> parcel_changed_signal_t;
	void init(F32 region_size);
	static void cleanupGlobals();
	BOOL	selectionEmpty() const;
	F32		getSelectionWidth() const	{ return F32(mEastNorth.mdV[VX] - mWestSouth.mdV[VX]); }
	F32		getSelectionHeight() const	{ return F32(mEastNorth.mdV[VY] - mWestSouth.mdV[VY]); }
	BOOL	getSelection(LLVector3d &min, LLVector3d &max) { min = mWestSouth; max = mEastNorth; return !selectionEmpty();}
	LLViewerRegion* getSelectionRegion() const;
	F32		getDwelling() const { return mSelectedDwell;}
	void	getDisplayInfo(S32* area, S32* claim, S32* rent, BOOL* for_sale, F32* dwell);
	S32 getSelectedArea() const;
	void resetSegments(U8* segments);
	void writeHighlightSegments(F32 west, F32 south, F32 east, F32 north);
	void writeSegmentsFromBitmap(U8* bitmap, U8* segments);
	void writeAgentParcelFromBitmap(U8* bitmap);
	void selectCollisionParcel();
	LLParcelSelectionHandle selectParcelAt(const LLVector3d& pos_global);
	LLParcelSelectionHandle selectParcelInRectangle();
	LLParcelSelectionHandle selectLand(const LLVector3d &corner1, const LLVector3d &corner2,
					   BOOL snap_to_parcel);
	void	deselectLand();
	void	deselectUnused();
	void addObserver(LLParcelObserver* observer);
	void removeObserver(LLParcelObserver* observer);
	void notifyObservers();
	void setSelectionVisible(BOOL visible) { mRenderSelection = visible; }
	BOOL	isOwnedAt(const LLVector3d& pos_global) const;
	BOOL	isOwnedSelfAt(const LLVector3d& pos_global) const;
	BOOL	isOwnedOtherAt(const LLVector3d& pos_global) const;
	BOOL	isSoundLocal(const LLVector3d &pos_global) const;
	BOOL	canHearSound(const LLVector3d &pos_global) const;
	LLParcelSelectionHandle getParcelSelection() const;
	LLParcelSelectionHandle getFloatingParcelSelection() const;
	LLParcel *getAgentParcel() const;
	BOOL	inAgentParcel(const LLVector3d &pos_global) const;
	LLParcel*	getHoverParcel() const;
	LLParcel*	getCollisionParcel() const;
	const U8*	getCollisionBitmap() const { return mCollisionBitmap; }
	size_t		getCollisionBitmapSize() const { return mParcelsPerEdge * mParcelsPerEdge / 8; }
	U64			getCollisionRegionHandle() const { return mCollisionRegionHandle; }
	typedef boost::signals2::signal<void (const LLViewerRegion*)> collision_update_signal_t;
	boost::signals2::connection setCollisionUpdateCallback(const collision_update_signal_t::slot_type & cb);
	bool	allowAgentBuild() const;
	bool	allowAgentBuild(const LLParcel* parcel) const;
	bool	allowAgentVoice() const;
	bool	allowAgentVoice(const LLViewerRegion* region, const LLParcel* parcel) const;
	bool	allowAgentFly(const LLViewerRegion* region, const LLParcel* parcel) const;
	bool	allowAgentPush(const LLViewerRegion* region, const LLParcel* parcel) const;
	bool	allowAgentScripts(const LLViewerRegion* region, const LLParcel* parcel) const;
	bool	allowAgentDamage(const LLViewerRegion* region, const LLParcel* parcel) const;
	F32		getHoverParcelWidth() const
				{ return F32(mHoverEastNorth.mdV[VX] - mHoverWestSouth.mdV[VX]); }
	F32		getHoverParcelHeight() const
				{ return F32(mHoverEastNorth.mdV[VY] - mHoverWestSouth.mdV[VY]); }
	void	render();
	void	renderParcelCollision();
	void	renderRect(	const LLVector3d &west_south_bottom,
						const LLVector3d &east_north_top );
	void	renderOneSegment(F32 x1, F32 y1, F32 x2, F32 y2, F32 height, U8 direction, LLViewerRegion* regionp);
	void	renderHighlightSegments(const U8* segments, LLViewerRegion* regionp);
	void	renderCollisionSegments(U8* segments, BOOL use_pass, LLViewerRegion* regionp);
	void	sendParcelGodForceOwner(const LLUUID& owner_id);
	void sendParcelGodForceToContent();
	void	sendParcelPropertiesUpdate(LLParcel* parcel, bool use_agent_region = false);
	void	sendParcelAccessListUpdate(U32 which);
	void	sendParcelAccessListRequest(U32 flags);
	void	sendParcelDwellRequest();
	void	setHoverParcel(const LLVector3d& pos_global);
	bool	canAgentBuyParcel(LLParcel*, bool forGroup) const;
	void	startBuyLand(BOOL is_for_group = FALSE);
	void	startSellLand();
	void	startReleaseLand();
	void	startDivideLand();
	void	startJoinLand();
	void	startDeedLandToGroup();
	void reclaimParcel();
	void	buyPass();
	class ParcelBuyInfo;
	ParcelBuyInfo* setupParcelBuy(const LLUUID& agent_id,
								  const LLUUID& session_id,
								  const LLUUID& group_id,
								  BOOL is_group_owned,
								  BOOL is_claim,
								  BOOL remove_contribution);
	void sendParcelBuy(ParcelBuyInfo*);
	void deleteParcelBuy(ParcelBuyInfo* *info);
	void sendParcelDeed(const LLUUID& group_id);
	void sendParcelRelease();
	const std::string& getAgentParcelName() const;
	static void processParcelOverlay(LLMessageSystem *msg, void **user_data);
	static void processParcelProperties(LLMessageSystem *msg, void **user_data);
	static void processParcelAccessListReply(LLMessageSystem *msg, void **user);
	static void processParcelDwellReply(LLMessageSystem *msg, void **user);
	void dump();
	BOOL isCollisionBanned();
	boost::signals2::connection addAgentParcelChangedCallback(parcel_changed_callback_t cb);
	boost::signals2::connection setTeleportFinishedCallback(teleport_finished_callback_t cb);
	boost::signals2::connection setTeleportFailedCallback(parcel_changed_callback_t cb);
	void onTeleportFinished(bool local, const LLVector3d& new_pos);
	void onTeleportFailed();
	static BOOL isParcelOwnedByAgent(const LLParcel* parcelp, U64 group_proxy_power);
	static BOOL isParcelModifiableByAgent(const LLParcel* parcelp, U64 group_proxy_power);
private:
	static void sendParcelAccessListUpdate(U32 flags, const std::map<LLUUID, class LLAccessEntry>& entries, LLViewerRegion* region, S32 parcel_local_id);
	static void sendParcelExperienceUpdate( const U32 flags, uuid_vec_t experience_ids, LLViewerRegion* region, S32 parcel_local_id );
	static bool releaseAlertCB(const LLSD& notification, const LLSD& response);
	void deedLandToGroup();
	static bool deedAlertCB(const LLSD& notification, const LLSD& response);
	static bool callbackDivideLand(const LLSD& notification, const LLSD& response);
	static bool callbackJoinLand(const LLSD& notification, const LLSD& response);
	LLViewerTexture* getBlockedImage() const;
	LLViewerTexture* getPassImage() const;
private:
	BOOL						mSelected;
	LLParcel*					mCurrentParcel;
	LLParcelSelectionHandle		mCurrentParcelSelection;
	LLParcelSelectionHandle		mFloatingParcelSelection;
	S32							mRequestResult;
	LLVector3d					mWestSouth;
	LLVector3d					mEastNorth;
	F32							mSelectedDwell;
	LLParcel					*mAgentParcel;
	S32							mAgentParcelSequenceID;
	LLParcel*					mHoverParcel;
	S32							mHoverRequestResult;
	LLVector3d					mHoverWestSouth;
	LLVector3d					mHoverEastNorth;
	std::vector<LLParcelObserver*> mObservers;
	BOOL						mTeleportInProgress;
	teleport_finished_signal_t	mTeleportFinishedSignal;
	parcel_changed_signal_t		mTeleportFailedSignal;
	parcel_changed_signal_t		mAgentParcelChangedSignal;
	S32							mParcelsPerEdge;
	U8*							mHighlightSegments;
	U8*							mAgentParcelOverlay;
	static U8*					sPackedOverlay;
	LLParcel*					mCollisionParcel;
	U8*							mCollisionBitmap;
	U64							mCollisionRegionHandle;
	collision_update_signal_t*	mCollisionUpdateSignal;
	U8*							mCollisionSegments;
	BOOL						mRenderCollision;
	BOOL						mRenderSelection;
	S32							mCollisionBanned;
	LLFrameTimer				mCollisionTimer;
	LLViewerTexture*			mBlockedImage;
	LLViewerTexture*			mPassImage;
	S32 						mMediaParcelId;
	U64 						mMediaRegionId;
};
void sanitize_corners(const LLVector3d &corner1, const LLVector3d &corner2,
						LLVector3d &west_south_bottom, LLVector3d &east_north_top);
#endif
