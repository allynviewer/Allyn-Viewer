/** 
 * @file lljoint.h
 * @brief Implementation of LLJoint class.
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
#ifndef LL_LLJOINT_H
#define LL_LLJOINT_H
#include <string>
#include <list>
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "xform.h"
const S32 LL_CHARACTER_MAX_JOINTS_PER_MESH = 15;
const U32 LL_CHARACTER_MAX_ANIMATED_JOINTS = 216;
const U32 LL_MAX_JOINTS_PER_MESH_OBJECT = 110;
const U32 LL_HAND_JOINT_NUM = (LL_CHARACTER_MAX_ANIMATED_JOINTS-1);
const U32 LL_FACE_JOINT_NUM = (LL_CHARACTER_MAX_ANIMATED_JOINTS-2);
const S32 LL_CHARACTER_MAX_PRIORITY = 7;
const F32 LL_MAX_PELVIS_OFFSET = 5.f;
class LLVector3OverrideMap
{
public:
	LLVector3OverrideMap() {}
	bool findActiveOverride(LLUUID& mesh_id, LLVector3& pos) const;
	void showJointVector3Overrides(std::ostringstream& os) const;
	U32 count() const;
	void add(const LLUUID& mesh_id, const LLVector3& pos);
	bool remove(const LLUUID& mesh_id);
	void clear();
	typedef std::map<LLUUID,LLVector3> map_type;
    const map_type& getMap() const { return m_map; }
private:
	map_type m_map;
};
inline bool operator==(const LLVector3OverrideMap& a, const LLVector3OverrideMap& b)
{
    return a.getMap() == b.getMap();
}
inline bool operator!=(const LLVector3OverrideMap& a, const LLVector3OverrideMap& b)
{
    return !(a == b);
}
class LLJoint
{
public:
	enum JointPriority
	{
		USE_MOTION_PRIORITY = -1,
		LOW_PRIORITY = 0,
		MEDIUM_PRIORITY,
		HIGH_PRIORITY,
		HIGHER_PRIORITY,
		HIGHEST_PRIORITY,
		ADDITIVE_PRIORITY = LL_CHARACTER_MAX_PRIORITY
	};
	enum DirtyFlags
	{
		MATRIX_DIRTY = 0x1 << 0,
		ROTATION_DIRTY = 0x1 << 1,
		POSITION_DIRTY = 0x1 << 2,
		ALL_DIRTY = 0x7
	};
public:
    enum SupportCategory
    {
        SUPPORT_BASE,
        SUPPORT_EXTENDED
    };
protected:
	std::string	mName;
	SupportCategory mSupport;
	LLJoint	*mParent;
	LLXformMatrix		mXform;
    LLVector3       mDefaultPosition;
    LLVector3       mDefaultScale;
public:
	U32				mDirtyFlags;
	BOOL			mUpdateXform;
	LLVector3		mSkinOffset;
    LLVector3		mEnd;
	S32				mJointNum;
	typedef std::list<LLJoint*> child_list_t;
	child_list_t mChildren;
	static S32		sNumTouches;
	static S32		sNumUpdates;
    typedef std::set<std::string> debug_joint_name_t;
    static debug_joint_name_t s_debugJointNames;
    static void setDebugJointNames(const debug_joint_name_t& names);
    static void setDebugJointNames(const std::string& names_string);
	LLVector3OverrideMap m_attachmentPosOverrides;
	LLVector3 m_posBeforeOverrides;
	LLVector3OverrideMap m_attachmentScaleOverrides;
	LLVector3 m_scaleBeforeOverrides;
	void updatePos(const std::string& av_info);
	void updateScale(const std::string& av_info);
public:
	LLJoint();
	LLJoint(S32 joint_num);
	LLJoint( const std::string &name, LLJoint *parent=NULL );
	virtual ~LLJoint();
private:
	void init();
public:
	void setup( const std::string &name, LLJoint *parent=NULL );
	void touch(U32 flags = ALL_DIRTY);
	const std::string& getName() const { return mName; }
	void setName( const std::string &name ) { mName = name; }
	S32 getJointNum() const { return mJointNum; }
	void setJointNum(S32 joint_num);
    SupportCategory getSupport() const { return mSupport; }
    void setSupport( const SupportCategory& support) { mSupport = support; }
    void setSupport( const std::string& support_string);
    void setEnd( const LLVector3& end) { mEnd = end; }
    const LLVector3& getEnd() const { return mEnd; }
	LLJoint *getParent() { return mParent; }
	LLJoint *getRoot();
	LLJoint *findJoint( const std::string &name );
	void addChild( LLJoint *joint );
	void removeChild( LLJoint *joint );
	void removeAllChildren();
	const LLVector3& getPosition();
	void setPosition( const LLVector3& pos, bool apply_attachment_overrides = false );
	void setDefaultPosition( const LLVector3& pos );
	const LLVector3& getDefaultPosition() const;
	void setDefaultScale( const LLVector3& scale );
	const LLVector3& getDefaultScale() const;
	LLVector3 getWorldPosition();
	LLVector3 getLastWorldPosition();
	void setWorldPosition( const LLVector3& pos );
	const LLQuaternion& getRotation();
	void setRotation( const LLQuaternion& rot );
	LLQuaternion getWorldRotation();
	LLQuaternion getLastWorldRotation();
	void setWorldRotation( const LLQuaternion& rot );
	const LLVector3& getScale();
	void setScale( const LLVector3& scale, bool apply_attachment_overrides = false );
	const LLMatrix4a &getWorldMatrix();
	void setWorldMatrix( const LLMatrix4& mat );
	void updateWorldMatrixChildren();
	void updateWorldMatrixParent();
	void updateWorldPRSParent();
	void updateWorldMatrix();
	const LLVector3 &getSkinOffset();
	void setSkinOffset( const LLVector3 &offset);
	LLXformMatrix	*getXform() { return &mXform; }
	void clampRotation(LLQuaternion old_rot, LLQuaternion new_rot);
	virtual BOOL isAnimatable() const { return TRUE; }
	void addAttachmentPosOverride( const LLVector3& pos, const LLUUID& mesh_id, const std::string& av_info, bool& active_override_changed );
	void removeAttachmentPosOverride( const LLUUID& mesh_id, const std::string& av_info, bool& active_override_changed );
	bool hasAttachmentPosOverride( LLVector3& pos, LLUUID& mesh_id ) const;
	void clearAttachmentPosOverrides();
    void showAttachmentPosOverrides(const std::string& av_info) const;
	void addAttachmentScaleOverride( const LLVector3& scale, const LLUUID& mesh_id, const std::string& av_info );
	void removeAttachmentScaleOverride( const LLUUID& mesh_id, const std::string& av_info );
	bool hasAttachmentScaleOverride( LLVector3& scale, LLUUID& mesh_id ) const;
	void clearAttachmentScaleOverrides();
    void showAttachmentScaleOverrides(const std::string& av_info) const;
    void getAllAttachmentPosOverrides(S32& num_pos_overrides,
                                      std::set<LLVector3>& distinct_pos_overrides);
    void getAllAttachmentScaleOverrides(S32& num_scale_overrides,
                                        std::set<LLVector3>& distinct_scale_overrides);
    bool aboveJointPosThreshold(const LLVector3& pos) const;
    bool aboveJointScaleThreshold(const LLVector3& scale) const;
	std::string exportString(U32 tabs);
};
#endif
