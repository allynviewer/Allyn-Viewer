/** 
 * @file llvieweroctree.cpp
 * @brief LLViewerOctreeGroup class implementation and supporting functions
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#include "llvieweroctree.h"
#include "llheapdiag.h"
#include "llviewerregion.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llglslshader.h"
#include "llviewershadermgr.h"
#include "lltexturecache.h"
#include "llimageworker.h"
#include "lltexturefetch.h"
U32 LLViewerOctreeEntryData::sCurVisible = 10;
typedef enum
{
	b000 = 0x00,
	b001 = 0x01,
	b010 = 0x02,
	b011 = 0x03,
	b100 = 0x04,
	b101 = 0x05,
	b110 = 0x06,
	b111 = 0x07,
} eLoveTheBits;
static U16 sOcclusionIndices[] =
{
		b111, b110, b010, b011, b001, b101, b100, b110,
		b011, b010, b000, b001, b101, b111, b110, b010,
		b101, b100, b110, b111, b011, b001, b000, b100,
		b001, b000, b100, b101, b111, b011, b010, b000,
		b110, b000, b010, b011, b111, b101, b100, b000,
		b010, b100, b000, b001, b011, b111, b110, b100,
		b100, b010, b110, b111, b101, b001, b000, b010,
		b000, b110, b100, b101, b001, b011, b010, b110,
};
U32 get_box_fan_indices(LLCamera* camera, const LLVector4a& center)
{
	LLVector4a origin;
	origin.load3(camera->getOrigin().mV);
	S32 cypher = center.greaterThan(origin).getGatheredBits() & 0x7;
	return cypher*8;
}
U8* get_box_fan_indices_ptr(LLCamera* camera, const LLVector4a& center)
{
	LLVector4a origin;
	origin.load3(camera->getOrigin().mV);
	S32 cypher = center.greaterThan(origin).getGatheredBits() & 0x7;
	return (U8*) (sOcclusionIndices+cypher*8);
}
LLVertexBuffer* ll_create_cube_vb(U32 type_mask, U32 usage)
{
	LLVertexBuffer* ret = new LLVertexBuffer(type_mask, usage);
	ret->allocateBuffer(8, 64, true);
	LLStrider<LLVector3> pos;
	LLStrider<U16> idx;
	ret->getVertexStrider(pos);
	ret->getIndexStrider(idx);
	pos[0] = LLVector3(-1,-1,-1);
	pos[1] = LLVector3(-1,-1, 1);
	pos[2] = LLVector3(-1, 1,-1);
	pos[3] = LLVector3(-1, 1, 1);
	pos[4] = LLVector3( 1,-1,-1);
	pos[5] = LLVector3( 1,-1, 1);
	pos[6] = LLVector3( 1, 1,-1);
	pos[7] = LLVector3( 1, 1, 1);
	for (U32 i = 0; i < 64; i++)
	{
		idx[i] = sOcclusionIndices[i];
	}
	ret->flush();
	return ret;
}
#define LL_TRACK_PENDING_OCCLUSION_QUERIES 0
const F32 SG_OCCLUSION_FUDGE = 0.25f;
#define SG_DISCARD_TOLERANCE 0.01f
S32 AABBSphereIntersect(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &rad)
{
	return AABBSphereIntersectR2(min, max, origin, rad*rad);
}
S32 AABBSphereIntersectR2(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &r)
{
	F32 d = 0.f;
	F32 t;
	if ((min-origin).magVecSquared() < r &&
		(max-origin).magVecSquared() < r)
	{
		return 2;
	}
	for (U32 i = 0; i < 3; i++)
	{
		if (origin.mV[i] < min.mV[i])
		{
			t = min.mV[i] - origin.mV[i];
			d += t*t;
		}
		else if (origin.mV[i] > max.mV[i])
		{
			t = origin.mV[i] - max.mV[i];
			d += t*t;
		}
		if (d > r)
		{
			return 0;
		}
	}
	return 1;
}
S32 AABBSphereIntersect(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &rad)
{
	return AABBSphereIntersectR2(min, max, origin, rad*rad);
}
S32 AABBSphereIntersectR2(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &r)
{
	F32 d = 0.f;
	F32 t;
	LLVector4a origina;
	origina.load3(origin.mV);
	LLVector4a v;
	v.setSub(min, origina);
	if (v.dot3(v) < r)
	{
		v.setSub(max, origina);
		if (v.dot3(v) < r)
		{
			return 2;
		}
	}
	for (U32 i = 0; i < 3; i++)
	{
		if (origin.mV[i] < min[i])
		{
			t = min[i] - origin.mV[i];
			d += t*t;
		}
		else if (origin.mV[i] > max[i])
		{
			t = origin.mV[i] - max[i];
			d += t*t;
		}
		if (d > r)
		{
			return 0;
		}
	}
	return 1;
}
LLViewerOctreeEntry::LLViewerOctreeEntry() :
	mGroup(NULL),
	mBinRadius(0.f),
	mBinIndex(-1),
	mVisible(0)
{
	mPositionGroup.clear();
	mExtents[0].clear();
	mExtents[1].clear();
	for(S32 i = 0; i < NUM_DATA_TYPE; i++)
	{
		mData[i] = NULL;
	}
}
LLViewerOctreeEntry::~LLViewerOctreeEntry()
{
	llassert(!mGroup);
}
void LLViewerOctreeEntry::addData(LLViewerOctreeEntryData* data)
{
	llassert(data != NULL);
	mData[data->getDataType()] = data;
}
void LLViewerOctreeEntry::removeData(LLViewerOctreeEntryData* data)
{
	if(!mData[data->getDataType()])
	{
		return;
	}
	if(mData[data->getDataType()] != data)
	{
		return;
	}
	mData[data->getDataType()] = NULL;
	if(!mGroup.isNull() && !mData[LLDRAWABLE])
	{
		mGroup->removeFromGroup(data);
		mGroup = NULL;
		llassert(mBinIndex == -1);
	}
}
void LLViewerOctreeEntry::nullGroup()
{
	mGroup = NULL;
}
void LLViewerOctreeEntry::setGroup(LLViewerOctreeGroup* group)
{
	if(mGroup == group)
	{
		return;
	}
	if(mGroup)
	{
		LLViewerOctreeGroup* old_group = mGroup;
		mGroup = NULL;
		old_group->removeFromGroup(this);
		llassert(mBinIndex == -1);
	}
	mGroup = group;
}
LLViewerOctreeEntryData::~LLViewerOctreeEntryData()
{
	if(mEntry)
	{
		mEntry->removeData(this);
	}
}
LLViewerOctreeEntryData::LLViewerOctreeEntryData(LLViewerOctreeEntry::eEntryDataType_t data_type)
	: mDataType(data_type),
	  mEntry(NULL)
{
}
void LLViewerOctreeEntryData::setOctreeEntry(LLViewerOctreeEntry* entry)
{
	llassert_always(mEntry.isNull());
	if(mEntry.notNull())
	{
		return;
	}
	if(!entry)
	{
		mEntry = new LLViewerOctreeEntry();
	}
	else
	{
		mEntry = entry;
	}
	mEntry->addData(this);
}
void LLViewerOctreeEntryData::removeOctreeEntry()
{
	if(mEntry)
	{
		mEntry->removeData(this);
		mEntry = NULL;
	}
}
void LLViewerOctreeEntryData::setSpatialExtents(const LLVector3& min, const LLVector3& max)
{
	mEntry->mExtents[0].load3(min.mV);
	mEntry->mExtents[1].load3(max.mV);
}
void LLViewerOctreeEntryData::setSpatialExtents(const LLVector4a& min, const LLVector4a& max)
{
	mEntry->mExtents[0] = min;
	mEntry->mExtents[1] = max;
}
void LLViewerOctreeEntryData::setPositionGroup(const LLVector4a& pos)
{
	mEntry->mPositionGroup = pos;
}
const LLVector4a* LLViewerOctreeEntryData::getSpatialExtents() const
{
	return mEntry->getSpatialExtents();
}
void LLViewerOctreeEntryData::setGroup(LLViewerOctreeGroup* group)
{
	mEntry->setGroup(group);
}
void LLViewerOctreeEntryData::shift(const LLVector4a &shift_vector)
{
	mEntry->mExtents[0].add(shift_vector);
	mEntry->mExtents[1].add(shift_vector);
	mEntry->mPositionGroup.add(shift_vector);
}
LLViewerOctreeGroup* LLViewerOctreeEntryData::getGroup()const
{
	return mEntry.notNull() ? mEntry->mGroup : LLPointer<LLViewerOctreeGroup>();
}
const LLVector4a& LLViewerOctreeEntryData::getPositionGroup() const
{
	return mEntry->getPositionGroup();
}
bool LLViewerOctreeEntryData::isVisible() const
{
	if(mEntry)
	{
		return mEntry->mVisible == sCurVisible;
	}
	return false;
}
bool LLViewerOctreeEntryData::isRecentlyVisible() const
{
	if(!mEntry)
	{
		return false;
	}
	if(isVisible())
	{
		return true;
	}
	if(getGroup() && getGroup()->isRecentlyVisible())
	{
		setVisible();
		return true;
	}
	return false;
}
void LLViewerOctreeEntryData::setVisible() const
{
	if(mEntry)
	{
		mEntry->mVisible = sCurVisible;
	}
}
void LLViewerOctreeEntryData::resetVisible() const
{
	if(mEntry)
	{
		mEntry->mVisible = 0;
	}
}
LLViewerOctreeGroup::~LLViewerOctreeGroup()
{
}
LLViewerOctreeGroup::LLViewerOctreeGroup(OctreeNode* node) :
	mOctreeNode(node),
	mAnyVisible(0),
	mState(CLEAN)
{
	LLVector4a tmp;
	tmp.splat(0.f);
	mExtents[0] = mExtents[1] = mObjectBounds[0] = mObjectBounds[1] =
		mObjectExtents[0] = mObjectExtents[1] = tmp;
	mBounds[0] = node->getCenter();
	mBounds[1] = node->getSize();
	mOctreeNode->addListener(this);
	for (U32 i = 0; i < sizeof(mVisible) / sizeof(mVisible[0]); i++)
	{
		mVisible[i] = 0;
	}
}
bool LLViewerOctreeGroup::hasElement(LLViewerOctreeEntryData* data)
{
	if(!data->getEntry())
	{
		return false;
	}
	return std::find(getDataBegin(), getDataEnd(), data->getEntry()) != getDataEnd();
}
bool LLViewerOctreeGroup::removeFromGroup(LLViewerOctreeEntryData* data)
{
	return removeFromGroup(data->getEntry());
}
bool LLViewerOctreeGroup::removeFromGroup(LLViewerOctreeEntry* entry)
{
	llassert(entry != NULL);
	llassert(!entry->getGroup());
	if(isDead())
	{
		entry->setBinIndex(-1);
		return true;
	}
	unbound();
	setState(OBJECT_DIRTY);
	if (mOctreeNode)
	{
		if (!mOctreeNode->remove(entry))
		{
			OCT_ERRS << "Could not remove LLVOCacheEntry from LLVOCacheOctreeGroup" << LL_ENDL;
			return false;
		}
	}
	return true;
}
void LLViewerOctreeGroup::unbound()
{
	if (isDirty())
	{
		return;
	}
	setState(DIRTY);
	if (mOctreeNode)
	{
		OctreeNode* parent = (OctreeNode*) mOctreeNode->getParent();
		while (parent != NULL)
		{
			LLViewerOctreeGroup* group = (LLViewerOctreeGroup*) parent->getListener(0);
			if (!group || group->isDirty())
			{
				return;
			}
			group->setState(DIRTY);
			parent = (OctreeNode*) parent->getParent();
		}
	}
}
void LLViewerOctreeGroup::rebound()
{
	if (!isDirty())
	{
		return;
	}
	if (mOctreeNode->getChildCount() == 1 && mOctreeNode->getElementCount() == 0)
	{
		LLViewerOctreeGroup* group = (LLViewerOctreeGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->rebound();
		mBounds[0] = group->mBounds[0];
		mBounds[1] = group->mBounds[1];
		mExtents[0] = group->mExtents[0];
		mExtents[1] = group->mExtents[1];
		group->setState(SKIP_FRUSTUM_CHECK);
	}
	else if (mOctreeNode->isLeaf())
	{
		boundObjects(TRUE, mExtents[0], mExtents[1]);
		mBounds[0] = mObjectBounds[0];
		mBounds[1] = mObjectBounds[1];
	}
	else
	{
		LLVector4a& newMin = mExtents[0];
		LLVector4a& newMax = mExtents[1];
		LLViewerOctreeGroup* group = (LLViewerOctreeGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->clearState(SKIP_FRUSTUM_CHECK);
		group->rebound();
		newMin = group->mExtents[0];
		newMax = group->mExtents[1];
		for (U32 i = 1; i < mOctreeNode->getChildCount(); i++)
		{
			group = (LLViewerOctreeGroup*) mOctreeNode->getChild(i)->getListener(0);
			group->clearState(SKIP_FRUSTUM_CHECK);
			group->rebound();
			const LLVector4a& max = group->mExtents[1];
			const LLVector4a& min = group->mExtents[0];
			newMax.setMax(newMax, max);
			newMin.setMin(newMin, min);
		}
		boundObjects(FALSE, newMin, newMax);
		mBounds[0].setAdd(newMin, newMax);
		mBounds[0].mul(0.5f);
		mBounds[1].setSub(newMax, newMin);
		mBounds[1].mul(0.5f);
	}
	clearState(DIRTY);
	return;
}
void LLViewerOctreeGroup::handleInsertion(const TreeNode* node, LLViewerOctreeEntry* obj)
{
	obj->setGroup(this);
	unbound();
	setState(OBJECT_DIRTY);
}
void LLViewerOctreeGroup::handleRemoval(const TreeNode* node, LLViewerOctreeEntry* obj)
{
	unbound();
	setState(OBJECT_DIRTY);
	obj->setGroup(NULL);
}
void LLViewerOctreeGroup::handleDestruction(const TreeNode* node)
{
	for (OctreeNode::element_iter i = mOctreeNode->getDataBegin(); i != mOctreeNode->getDataEnd(); ++i)
	{
		LLViewerOctreeEntry* obj = *i;
		if (obj && obj->getGroup() == this)
		{
			obj->nullGroup();
		}
	}
	mOctreeNode = NULL;
}
void LLViewerOctreeGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child)
{
	if (child->getListenerCount() == 0)
	{
		new LLViewerOctreeGroup(child);
	}
	else
	{
		OCT_ERRS << "LLViewerOctreeGroup redundancy detected." << LL_ENDL;
	}
	unbound();
	((LLViewerOctreeGroup*)child->getListener(0))->unbound();
}
void LLViewerOctreeGroup::handleChildRemoval(const OctreeNode* parent, const OctreeNode* child)
{
	unbound();
}
LLViewerOctreeGroup* LLViewerOctreeGroup::getParent()
{
	if (isDead())
	{
		return NULL;
	}
	if(!mOctreeNode)
	{
		return NULL;
	}
	OctreeNode* parent = mOctreeNode->getOctParent();
	if (parent)
	{
		return (LLViewerOctreeGroup*) parent->getListener(0);
	}
	return NULL;
}
bool LLViewerOctreeGroup::boundObjects(BOOL empty, LLVector4a& minOut, LLVector4a& maxOut)
{
	const OctreeNode* node = mOctreeNode;
	if (node->isEmpty())
	{
		if (empty && mOctreeNode->getParent())
		{
			OCT_ERRS << "Empty leaf found in octree." << LL_ENDL;
		}
		return false;
	}
	LLVector4a& newMin = mObjectExtents[0];
	LLVector4a& newMax = mObjectExtents[1];
	if (hasState(OBJECT_DIRTY))
	{
		clearState(OBJECT_DIRTY);
		OctreeNode::const_element_iter i = node->getDataBegin();
		LLViewerOctreeEntry* entry = *i;
		const LLVector4a* minMax = entry->getSpatialExtents();
		newMin = minMax[0];
		newMax = minMax[1];
		for (++i; i != node->getDataEnd(); ++i)
		{
			entry = *i;
			minMax = entry->getSpatialExtents();
			update_min_max(newMin, newMax, minMax[0]);
			update_min_max(newMin, newMax, minMax[1]);
		}
		mObjectBounds[0].setAdd(newMin, newMax);
		mObjectBounds[0].mul(0.5f);
		mObjectBounds[1].setSub(newMax, newMin);
		mObjectBounds[1].mul(0.5f);
	}
	if (empty)
	{
		minOut = newMin;
		maxOut = newMax;
	}
	else
	{
		minOut.setMin(minOut, newMin);
		maxOut.setMax(maxOut, newMax);
	}
	return TRUE;
}
BOOL LLViewerOctreeGroup::isVisible() const
{
	return mVisible[LLViewerCamera::sCurCameraID] >= LLViewerOctreeEntryData::getCurrentFrame() ? TRUE : FALSE;
}
BOOL LLViewerOctreeGroup::isRecentlyVisible() const
{
	return FALSE;
}
void LLViewerOctreeGroup::setVisible()
{
	mVisible[LLViewerCamera::sCurCameraID] = LLViewerOctreeEntryData::getCurrentFrame();
	if(LLViewerCamera::sCurCameraID < LLViewerCamera::CAMERA_WATER0)
	{
		mAnyVisible = LLViewerOctreeEntryData::getCurrentFrame();
	}
}
void LLViewerOctreeGroup::checkStates()
{
#if LL_OCTREE_PARANOIA_CHECK
	LLOctreeStateCheck checker;
	checker.traverse(mOctreeNode);
#endif
}
std::set<U32> LLOcclusionCullingGroup::sPendingQueries;
class LLOcclusionQueryPool : public LLGLNamePool
{
public:
	LLOcclusionQueryPool()
	{
	}
protected:
	virtual GLuint allocateName()
	{
		GLuint ret = 0;
		glGenQueriesARB(1, &ret);
		return ret;
	}
	virtual void releaseName(GLuint name)
	{
#if LL_TRACK_PENDING_OCCLUSION_QUERIES
		LLOcclusionCullingGroup::sPendingQueries.erase(name);
#endif
		glDeleteQueriesARB(1, &name);
	}
};
static LLOcclusionQueryPool sQueryPool;
U32 LLOcclusionCullingGroup::getNewOcclusionQueryObjectName()
{
	return sQueryPool.allocate();
}
void LLOcclusionCullingGroup::releaseOcclusionQueryObjectName(U32 name)
{
	sQueryPool.release(name);
}
class LLSpatialSetOcclusionState : public OctreeTraveler
{
public:
	LLOcclusionCullingGroup::eOcclusionState mState;
	LLSpatialSetOcclusionState(LLOcclusionCullingGroup::eOcclusionState state) : mState(state) { }
	virtual void visit(const OctreeNode* branch)
	{
		LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*)branch->getListener(0);
		if(group)
		{
			group->setOcclusionState(mState);
		}
	}
};
class LLSpatialSetOcclusionStateDiff : public LLSpatialSetOcclusionState
{
public:
	LLSpatialSetOcclusionStateDiff(LLOcclusionCullingGroup::eOcclusionState state) : LLSpatialSetOcclusionState(state) { }
	virtual void traverse(const OctreeNode* n)
	{
		LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*) n->getListener(0);
		if (group && !group->isOcclusionState(mState))
		{
			OctreeTraveler::traverse(n);
		}
	}
};
LLOcclusionCullingGroup::LLOcclusionCullingGroup(OctreeNode* node, LLSpatialPartition* part) :
	LLViewerOctreeGroup(node),
	mSpatialPartition(part)
{
	llassert(part);
	mSpatialPartition->mGroups.push_back(this);
	part->mLODSeed = (part->mLODSeed+1)%part->mLODPeriod;
	mLODHash = part->mLODSeed;
	OctreeNode* oct_parent = node->getOctParent();
	LLOcclusionCullingGroup* parent = oct_parent ? (LLOcclusionCullingGroup*) oct_parent->getListener(0) : NULL;
	for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
	{
		mOcclusionQuery[i] = 0;
		mOcclusionIssued[i] = 0;
		mOcclusionState[i] = parent ? SG_STATE_INHERIT_MASK & parent->mOcclusionState[i] : 0;
		mVisible[i] = 0;
	}
}
LLOcclusionCullingGroup::~LLOcclusionCullingGroup()
{
	if (mSpatialPartition)
	{
		auto it = std::find_if(mSpatialPartition->mGroups.begin(), mSpatialPartition->mGroups.end(), [this](LLOcclusionCullingGroup* rhs) {return rhs == this; });
		llassert(it != mSpatialPartition->mGroups.end());
		if (it != mSpatialPartition->mGroups.end())
		{
			mSpatialPartition->mGroups.erase(it);
		}
	}
	releaseOcclusionQueryObjectNames();
}
BOOL LLOcclusionCullingGroup::needsUpdate()
{
	return mSpatialPartition && (LLDrawable::getCurrentFrame() % mSpatialPartition->mLODPeriod == mLODHash) ? TRUE : FALSE;
}
BOOL LLOcclusionCullingGroup::isRecentlyVisible() const
{
	const S32 MIN_VIS_FRAME_RANGE = 2;
	return (LLDrawable::getCurrentFrame() - mVisible[LLViewerCamera::sCurCameraID]) < MIN_VIS_FRAME_RANGE ;
}
BOOL LLOcclusionCullingGroup::isAnyRecentlyVisible() const
{
	const S32 MIN_VIS_FRAME_RANGE = 2;
	return (LLDrawable::getCurrentFrame() - mAnyVisible) < MIN_VIS_FRAME_RANGE ;
}
void LLOcclusionCullingGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child)
{
	if (child->getListenerCount() == 0)
	{
		new LLOcclusionCullingGroup(child, mSpatialPartition);
	}
	else
	{
		OCT_ERRS << "LLOcclusionCullingGroup redundancy detected." << LL_ENDL;
	}
	unbound();
	((LLViewerOctreeGroup*)child->getListener(0))->unbound();
}
void LLOcclusionCullingGroup::releaseOcclusionQueryObjectNames()
{
	if (gGLManager.mHasOcclusionQuery)
	{
		for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; ++i)
		{
			if (mOcclusionQuery[i])
			{
				releaseOcclusionQueryObjectName(mOcclusionQuery[i]);
				mOcclusionQuery[i] = 0;
			}
		}
	}
}
void LLOcclusionCullingGroup::setOcclusionState(eOcclusionState state, S32 mode)
{
	if (mode > STATE_MODE_SINGLE)
	{
		if (mode == STATE_MODE_DIFF)
		{
			LLSpatialSetOcclusionStateDiff setter(state);
			setter.traverse(mOctreeNode);
		}
		else if (mode == STATE_MODE_BRANCH)
		{
			LLSpatialSetOcclusionState setter(state);
			setter.traverse(mOctreeNode);
		}
		else
		{
			for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
			{
				mOcclusionState[i] |= state;
				if ((state & DISCARD_QUERY) && mOcclusionQuery[i])
				{
					releaseOcclusionQueryObjectName(mOcclusionQuery[i]);
					mOcclusionQuery[i] = 0;
				}
			}
		}
	}
	else
	{
		mOcclusionState[LLViewerCamera::sCurCameraID] |= state;
		if ((state & DISCARD_QUERY) && mOcclusionQuery[LLViewerCamera::sCurCameraID])
		{
			releaseOcclusionQueryObjectName(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
			mOcclusionQuery[LLViewerCamera::sCurCameraID] = 0;
		}
	}
}
class LLSpatialClearOcclusionState : public OctreeTraveler
{
public:
	LLOcclusionCullingGroup::eOcclusionState mState;
	LLSpatialClearOcclusionState(LLOcclusionCullingGroup::eOcclusionState state) : mState(state) { }
	virtual void visit(const OctreeNode* branch)
	{
		LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*) branch->getListener(0);
		if(group)
		{
			group->clearOcclusionState(mState);
		}
	}
};
class LLSpatialClearOcclusionStateDiff : public LLSpatialClearOcclusionState
{
public:
	LLSpatialClearOcclusionStateDiff(LLOcclusionCullingGroup::eOcclusionState state) : LLSpatialClearOcclusionState(state) { }
	virtual void traverse(const OctreeNode* n)
	{
		LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*) n->getListener(0);
		if (group && group->isOcclusionState(mState))
		{
			OctreeTraveler::traverse(n);
		}
	}
};
void LLOcclusionCullingGroup::clearOcclusionState(eOcclusionState state, S32 mode)
{
	if (mode > STATE_MODE_SINGLE)
	{
		if (mode == STATE_MODE_DIFF)
		{
			LLSpatialClearOcclusionStateDiff clearer(state);
			clearer.traverse(mOctreeNode);
		}
		else if (mode == STATE_MODE_BRANCH)
		{
			LLSpatialClearOcclusionState clearer(state);
			clearer.traverse(mOctreeNode);
		}
		else
		{
			for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
			{
				mOcclusionState[i] &= ~state;
			}
		}
	}
	else
	{
		mOcclusionState[LLViewerCamera::sCurCameraID] &= ~state;
	}
}
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_READBACK("Readback Occlusion");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_WAIT("Occlusion Wait");
BOOL LLOcclusionCullingGroup::earlyFail(LLCamera* camera, const LLVector4a* bounds)
{
	if (camera->getOrigin().isExactlyZero())
	{
		return FALSE;
	}
	static LLCachedControl<F32> vel("SHOcclusionFudge",SG_OCCLUSION_FUDGE);
	LLVector4a fudge(vel*2.f);
	const LLVector4a& c = bounds[0];
	LLVector4a r;
	r.setAdd(bounds[1], fudge);
	LLVector4a e;
	e.load3(camera->getOrigin().mV);
	LLVector4a min;
	min.setSub(c,r);
	LLVector4a max;
	max.setAdd(c,r);
	S32 lt = e.lessThan(min).getGatheredBits() & 0x7;
	if (lt)
	{
		return FALSE;
	}
	S32 gt = e.greaterThan(max).getGatheredBits() & 0x7;
	if (gt)
	{
		return FALSE;
	}
	return TRUE;
}
U32 LLOcclusionCullingGroup::getLastOcclusionIssuedTime()
{
	return mOcclusionIssued[LLViewerCamera::sCurCameraID];
}
void LLOcclusionCullingGroup::checkOcclusion()
{
	if (mSpatialPartition == nullptr)
	{
		return;
	}
	if (LLPipeline::sUseOcclusion > 1)
	{
		LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_READBACK);
		LLOcclusionCullingGroup* parent = (LLOcclusionCullingGroup*)getParent();
		if (parent && parent->isOcclusionState(LLOcclusionCullingGroup::OCCLUDED))
		{
			clearOcclusionState(QUERY_PENDING | DISCARD_QUERY);
		}
		else if (isOcclusionState(QUERY_PENDING))
		{
			GLuint available = 0;
			if (mOcclusionQuery[LLViewerCamera::sCurCameraID])
			{
				glGetQueryObjectuivARB(mOcclusionQuery[LLViewerCamera::sCurCameraID], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
				static LLCachedControl<bool> wait_for_query("RenderSynchronousOcclusion", true);
				if (wait_for_query && mOcclusionIssued[LLViewerCamera::sCurCameraID] < gFrameCount)
				{
					S32 max_loop = 1024;
					LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_WAIT);
					while (!available && max_loop-- > 0)
					{
						F32 max_time = llmin(gFrameIntervalSeconds.value()*10.f, 1.f);
						LLAppViewer::getTextureCache()->update(max_time);
						LLAppViewer::getImageDecodeThread()->update(max_time);
						LLAppViewer::getTextureFetch()->update(max_time);
						glGetQueryObjectuivARB(mOcclusionQuery[LLViewerCamera::sCurCameraID], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
					}
				}
			}
			else
			{
				available = 1;
			}
			if (available)
			{
				GLuint res = 1;
				if (!isOcclusionState(DISCARD_QUERY) && mOcclusionQuery[LLViewerCamera::sCurCameraID])
				{
					glGetQueryObjectuivARB(mOcclusionQuery[LLViewerCamera::sCurCameraID], GL_QUERY_RESULT_ARB, &res);
#if LL_TRACK_PENDING_OCCLUSION_QUERIES
					sPendingQueries.erase(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
#endif
				}
				else if (mOcclusionQuery[LLViewerCamera::sCurCameraID])
				{
					releaseOcclusionQueryObjectName(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
					mOcclusionQuery[LLViewerCamera::sCurCameraID] = 0;
				}
				if (isOcclusionState(DISCARD_QUERY))
				{
					res = 2;
				}
				if (res > 0)
				{
					assert_states_valid(this);
					clearOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
					assert_states_valid(this);
				}
				else
				{
					assert_states_valid(this);
					setOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
					assert_states_valid(this);
				}
				clearOcclusionState(QUERY_PENDING | DISCARD_QUERY);
			}
		}
		else if (mSpatialPartition->isOcclusionEnabled() && isOcclusionState(LLOcclusionCullingGroup::OCCLUDED))
		{
			assert_states_valid(this);
			clearOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
			assert_states_valid(this);
		}
	}
}
static LLTrace::BlockTimerStatHandle FTM_PUSH_OCCLUSION_VERTS("Push Occlusion");
static LLTrace::BlockTimerStatHandle FTM_SET_OCCLUSION_STATE("Occlusion State");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_EARLY_FAIL("Occlusion Early Fail");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_ALLOCATE("Allocate");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_BUILD("Build");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_BEGIN_QUERY("Begin Query");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_END_QUERY("End Query");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_SET_BUFFER("Set Buffer");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_DRAW_WATER("Draw Water");
static LLTrace::BlockTimerStatHandle FTM_OCCLUSION_DRAW("Draw");
void LLOcclusionCullingGroup::doOcclusion(LLCamera* camera, const LLVector4a* shift)
{
	if (mSpatialPartition == nullptr)
	{
		return;
	}
	LLGLDisable<GL_STENCIL_TEST> stencil;
	if (mSpatialPartition->isOcclusionEnabled() && LLPipeline::sUseOcclusion > 1)
	{
		LLVector4a bounds[2];
		bounds[0] = mBounds[0];
		bounds[1] = mBounds[1];
		if(shift != NULL)
		{
			bounds[0].add(*shift);
		}
		if (earlyFail(camera, bounds))
		{
			LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_EARLY_FAIL);
			setOcclusionState(LLOcclusionCullingGroup::DISCARD_QUERY);
			assert_states_valid(this);
			clearOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
			assert_states_valid(this);
		}
		else
		{
			if (!isOcclusionState(QUERY_PENDING) || isOcclusionState(DISCARD_QUERY))
			{
				{
					LL_RECORD_BLOCK_TIME(FTM_RENDER_OCCLUSION);
					if (!mOcclusionQuery[LLViewerCamera::sCurCameraID])
					{
						LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_ALLOCATE);
						mOcclusionQuery[LLViewerCamera::sCurCameraID] = getNewOcclusionQueryObjectName();
					}
					bool const use_depth_clamp = gGLManager.mHasDepthClamp &&
												(mSpatialPartition->mDrawableType == LLDrawPool::POOL_WATER ||
												mSpatialPartition->mDrawableType == LLDrawPool::POOL_VOIDWATER);
					LLGLEnable<GL_DEPTH_CLAMP> clamp(use_depth_clamp);
					U32 mode = gGLManager.mHasOcclusionQuery2 ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED_ARB;
#if LL_TRACK_PENDING_OCCLUSION_QUERIES
					sPendingQueries.insert(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
#endif
					{
						LL_RECORD_BLOCK_TIME(FTM_PUSH_OCCLUSION_VERTS);
						mOcclusionIssued[LLViewerCamera::sCurCameraID] = gFrameCount;
						{
							LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_BEGIN_QUERY);
							glBeginQueryARB(mode, mOcclusionQuery[LLViewerCamera::sCurCameraID]);
						}
						LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
						llassert(shader);
						shader->uniform3fv(LLShaderMgr::BOX_CENTER, 1, bounds[0].getF32ptr());
						static LLCachedControl<F32> vel("SHOcclusionFudge",SG_OCCLUSION_FUDGE);
						LLVector4a fudge(SG_OCCLUSION_FUDGE);
						if (LLDrawPool::POOL_WATER == mSpatialPartition->mDrawableType)
						{
							fudge.getF32ptr()[2] = 1.f;
						}
						static LLVector4a fudged_bounds;
						fudged_bounds.setAdd(fudge, bounds[1]);
						shader->uniform3fv(LLShaderMgr::BOX_SIZE, 1, fudged_bounds.getF32ptr());
						if (!use_depth_clamp && mSpatialPartition->mDrawableType == LLDrawPool::POOL_VOIDWATER)
						{
							LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_DRAW_WATER);
							LLGLSquashToFarClip squash(glh_get_current_projection(), 1);
							if (camera->getOrigin().isExactlyZero())
							{
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, 0);
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, b111*8);
							}
							else
							{
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, bounds[0]));
							}
						}
						else
						{
							LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_DRAW);
							if (camera->getOrigin().isExactlyZero())
							{
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, 0);
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, b111*8);
							}
							else
							{
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, bounds[0]));
							}
						}
						{
							LL_RECORD_BLOCK_TIME(FTM_OCCLUSION_END_QUERY);
							glEndQueryARB(mode);
						}
					}
				}
				{
					LL_RECORD_BLOCK_TIME(FTM_SET_OCCLUSION_STATE);
					setOcclusionState(LLOcclusionCullingGroup::QUERY_PENDING);
					clearOcclusionState(LLOcclusionCullingGroup::DISCARD_QUERY);
				}
			}
		}
	}
}
LLViewerOctreePartition::LLViewerOctreePartition() :
	mRegionp(NULL),
	mOcclusionEnabled(TRUE),
	mDrawableType(0),
	mLODSeed(0),
	mLODPeriod(1)
{
	LLVector4a center, size;
	center.splat(0.f);
	size.splat(1.f);
	mOctree = new OctreeRoot(center,size, NULL);
}
LLViewerOctreePartition::~LLViewerOctreePartition()
{
	if (!mGroups.empty())
	{
		LL_WARNS_ONCE("OCTREE") << "LLViewerOctreePartition destroyed with " << mGroups.size() << " groups remaining" << LL_ENDL;
		static U32 sOctreeDirtyDtors = 0;
		++sOctreeDirtyDtors;
		if (sOctreeDirtyDtors == 1 || (sOctreeDirtyDtors % 100u) == 0u)
		{
			LLHeapDiag::markImportant(
				llformat("octree_dtor_groups=%u count=%u", (unsigned)mGroups.size(), (unsigned)sOctreeDirtyDtors).c_str());
			if ((sOctreeDirtyDtors % 500u) == 0u)
				LLHeapDiag::validateHeaps("octree_dirty_dtor");
		}
	}
	for (auto& entry : mGroups)
	{
		entry->mSpatialPartition = nullptr;
	}
	delete mOctree;
	mOctree = NULL;
}
BOOL LLViewerOctreePartition::isOcclusionEnabled()
{
	return mOcclusionEnabled || LLPipeline::sUseOcclusion > 2;
}
bool LLViewerOctreeCull::earlyFail(LLViewerOctreeGroup* group)
{
	return false;
}
void LLViewerOctreeCull::traverse(const OctreeNode* n)
{
	LLViewerOctreeGroup* group = (LLViewerOctreeGroup*) n->getListener(0);
	if (earlyFail(group))
	{
		return;
	}
	if (mRes == 2 ||
		(mRes && group->hasState(LLViewerOctreeGroup::SKIP_FRUSTUM_CHECK)))
	{
		OctreeTraveler::traverse(n);
	}
	else
	{
		mRes = frustumCheck(group);
		if (mRes)
		{
			OctreeTraveler::traverse(n);
		}
		mRes = 0;
	}
}
S32 LLViewerOctreeCull::AABBInFrustumNoFarClipGroupBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInFrustumNoFarClip(group->mBounds[0], group->mBounds[1]);
}
S32 LLViewerOctreeCull::AABBSphereIntersectGroupExtents(const LLViewerOctreeGroup* group)
{
	return AABBSphereIntersect(group->mExtents[0], group->mExtents[1], mCamera->getOrigin(), mCamera->mFrustumCornerDist);
}
S32 LLViewerOctreeCull::AABBInFrustumGroupBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInFrustum(group->mBounds[0], group->mBounds[1]);
}
S32 LLViewerOctreeCull::AABBInFrustumNoFarClipObjectBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInFrustumNoFarClip(group->mObjectBounds[0], group->mObjectBounds[1]);
}
S32 LLViewerOctreeCull::AABBSphereIntersectObjectExtents(const LLViewerOctreeGroup* group)
{
	return AABBSphereIntersect(group->mObjectExtents[0], group->mObjectExtents[1], mCamera->getOrigin(), mCamera->mFrustumCornerDist);
}
S32 LLViewerOctreeCull::AABBInFrustumObjectBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInFrustum(group->mObjectBounds[0], group->mObjectBounds[1]);
}
S32 LLViewerOctreeCull::AABBInRegionFrustumNoFarClipGroupBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustumNoFarClip(group->mBounds[0], group->mBounds[1]);
}
S32 LLViewerOctreeCull::AABBInRegionFrustumGroupBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustum(group->mBounds[0], group->mBounds[1]);
}
S32 LLViewerOctreeCull::AABBRegionSphereIntersectGroupExtents(const LLViewerOctreeGroup* group, const LLVector3& shift)
{
	return AABBSphereIntersect(group->mExtents[0], group->mExtents[1], mCamera->getOrigin() - shift, mCamera->mFrustumCornerDist);
}
S32 LLViewerOctreeCull::AABBInRegionFrustumObjectBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustum(group->mObjectBounds[0], group->mObjectBounds[1]);
}
S32 LLViewerOctreeCull::AABBInRegionFrustumNoFarClipObjectBounds(const LLViewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustumNoFarClip(group->mObjectBounds[0], group->mObjectBounds[1]);
}
S32 LLViewerOctreeCull::AABBRegionSphereIntersectObjectExtents(const LLViewerOctreeGroup* group, const LLVector3& shift)
{
	return AABBSphereIntersect(group->mObjectExtents[0], group->mObjectExtents[1], mCamera->getOrigin() - shift, mCamera->mFrustumCornerDist);
}
bool LLViewerOctreeCull::checkProjectionArea(const LLVector4a& center, const LLVector4a& size, const LLVector3& shift, F32 pixel_threshold, F32 near_radius)
{
	LLVector3 local_orig = mCamera->getOrigin() - shift;
	LLVector4a origin;
	origin.load3(local_orig.mV);
	LLVector4a lookAt;
	lookAt.setSub(center, origin);
	F32 distance = lookAt.getLength3().getF32();
	if(distance <= near_radius)
	{
		return true;
	}
	distance -= near_radius;
	F32 squared_rad = size.dot3(size).getF32();
	return squared_rad / distance > pixel_threshold;
}
bool LLViewerOctreeCull::checkObjects(const OctreeNode* branch, const LLViewerOctreeGroup* group)
{
	if (branch->getElementCount() == 0)
	{
		return false;
	}
	else if (branch->getChildCount() == 0)
	{
		return true;
	}
	else if (mRes == 1 && !frustumCheckObjects(group))
	{
		return false;
	}
	return true;
}
void LLViewerOctreeCull::preprocess(LLViewerOctreeGroup* group)
{
}
void LLViewerOctreeCull::processGroup(LLViewerOctreeGroup* group)
{
}
void LLViewerOctreeCull::visit(const OctreeNode* branch)
{
	LLViewerOctreeGroup* group = (LLViewerOctreeGroup*) branch->getListener(0);
	preprocess(group);
	if (checkObjects(branch, group))
	{
		processGroup(group);
	}
}
