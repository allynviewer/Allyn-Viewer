/** 
 * @file llpermissions.h
 * @brief Permissions structures for objects.
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
#ifndef LL_LLPERMISSIONS_H
#define LL_LLPERMISSIONS_H
#include "llpermissionsflags.h"
#include "llsd.h"
#include "lluuid.h"
#include "llxmlnode.h"
#include "llinventorytype.h"
class LLMessageSystem;
extern void mask_to_string(U32 mask, char* str);
extern std::string mask_to_string(U32 mask);
template<class T> class LLMetaClassT;
enum ExportPolicy {
  ep_creator_only,
  ep_full_perm,
  ep_export_bit
};
class LLPermissions
{
private:
	LLUUID			mCreator;
	LLUUID			mOwner;
	LLUUID			mLastOwner;
	LLUUID			mGroup;
	PermissionMask	mMaskBase;
	PermissionMask	mMaskOwner;
	PermissionMask	mMaskEveryone;
	PermissionMask	mMaskGroup;
	PermissionMask mMaskNextOwner;
	bool mIsGroupOwned;
	void fixFairUse();
	void fixOwnership();
public:
	static const LLPermissions DEFAULT;
	LLPermissions();
	void init(const LLUUID& creator, const LLUUID& owner,
			  const LLUUID& last_owner, const LLUUID& group);
	void initMasks(PermissionMask base, PermissionMask owner,
				   PermissionMask everyone, PermissionMask group,
				   PermissionMask next);
	void initMasks(LLInventoryType::EType type);
	const LLUUID&	getCreator() 		const	{ return mCreator; }
	const LLUUID&	getOwner() 			const	{ return mOwner; }
	const LLUUID&	getGroup() 			const	{ return mGroup; }
	const LLUUID&	getLastOwner() 		const	{ return mLastOwner; }
	U32				getMaskBase() 		const	{ return mMaskBase; }
	U32				getMaskOwner() 		const	{ return mMaskOwner; }
	U32				getMaskGroup() 		const	{ return mMaskGroup; }
	U32				getMaskEveryone() 	const	{ return mMaskEveryone; }
	U32 getMaskNextOwner() const { return mMaskNextOwner; }
	bool isOwned() const { return (mOwner.notNull() || mIsGroupOwned); }
	bool isGroupOwned() const { return mIsGroupOwned; }
	BOOL getOwnership(LLUUID& owner_id, BOOL& is_group_owned) const;
	LLUUID getSafeOwner() const;
	U32 getCRC32() const;
	void fix();
	void set(const LLPermissions& permissions);
	void setMaskBase(U32 mask)	   { mMaskBase = mask; }
	void setMaskOwner(U32 mask)	   { mMaskOwner = mask; }
	void setMaskEveryone(U32 mask) { mMaskEveryone = mask;}
	void setMaskGroup(U32 mask)	   { mMaskGroup = mask;}
	void setMaskNext(U32 mask) { mMaskNextOwner = mask; }
	void accumulate(const LLPermissions& perm);
	BOOL setOwnerAndGroup(const LLUUID& agent, const LLUUID& owner, const LLUUID& group, bool is_atomic);
	void yesReallySetOwner(const LLUUID& owner, bool group_owned);
	void setLastOwner(const LLUUID& last_owner);
	BOOL deedToGroup(const LLUUID& agent, const LLUUID& group);
	BOOL setBaseBits( const LLUUID& agent, BOOL set, PermissionMask bits);
	BOOL setOwnerBits( const LLUUID& agent, BOOL set, PermissionMask bits);
	BOOL setGroupBits( const LLUUID& agent, const LLUUID& group, BOOL set, PermissionMask bits);
	BOOL setEveryoneBits(const LLUUID& agent, const LLUUID& group, BOOL set, PermissionMask bits);
	BOOL setNextOwnerBits(const LLUUID& agent, const LLUUID& group, BOOL set, PermissionMask bits);
	void setCreator(const LLUUID& creator) { mCreator = creator; }
	bool allowOperationBy(PermissionBit op, const LLUUID& agent, const LLUUID& group = LLUUID::null) const;
	inline bool allowModifyBy(const LLUUID &agent_id) const;
	inline bool allowCopyBy(const LLUUID& agent_id) const;
	inline bool allowMoveBy(const LLUUID& agent_id) const;
	inline bool allowModifyBy(const LLUUID &agent_id, const LLUUID& group) const;
	inline bool allowCopyBy(const LLUUID& agent_id, const LLUUID& group) const;
	inline bool allowMoveBy(const LLUUID &agent_id, const LLUUID &group) const;
	bool allowExportBy(LLUUID const& requester, ExportPolicy export_policy) const;
	inline bool allowTransferTo(const LLUUID &agent_id) const;
	LLSD	packMessage() const;
	void	unpackMessage(const LLSD& perms);
	void	packMessage(LLMessageSystem* msg) const;
	void	unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
	BOOL	importFile(LLFILE* fp);
	BOOL	exportFile(LLFILE* fp) const;
	BOOL	importLegacyStream(std::istream& input_stream);
	BOOL	exportLegacyStream(std::ostream& output_stream) const;
	bool operator==(const LLPermissions &rhs) const;
	bool operator!=(const LLPermissions &rhs) const;
	friend std::ostream& operator<<(std::ostream &s, const LLPermissions &perm);
};
bool LLPermissions::allowModifyBy(const LLUUID& agent, const LLUUID& group) const
{
	return allowOperationBy(PERM_MODIFY, agent, group);
}
bool LLPermissions::allowCopyBy(const LLUUID& agent, const LLUUID& group) const
{
	return allowOperationBy(PERM_COPY, agent, group);
}
bool LLPermissions::allowMoveBy(const LLUUID& agent, const LLUUID& group) const
{
	return allowOperationBy(PERM_MOVE, agent, group);
}
bool LLPermissions::allowModifyBy(const LLUUID& agent) const
{
	return allowOperationBy(PERM_MODIFY, agent, LLUUID::null);
}
bool LLPermissions::allowCopyBy(const LLUUID& agent) const
{
	return allowOperationBy(PERM_COPY, agent, LLUUID::null);
}
bool LLPermissions::allowMoveBy(const LLUUID& agent) const
{
	return allowOperationBy(PERM_MOVE, agent, LLUUID::null);
}
bool LLPermissions::allowTransferTo(const LLUUID &agent_id) const
{
	if (mIsGroupOwned)
	{
		return allowOperationBy(PERM_TRANSFER, mGroup, mGroup);
	}
	else
	{
		return ((mOwner == agent_id) ? TRUE : allowOperationBy(PERM_TRANSFER, mOwner));
	}
}
class LLAggregatePermissions
{
public:
	enum EValue
	{
		AP_EMPTY = 0x00,
		AP_NONE = 0x01,
		AP_SOME = 0x02,
		AP_ALL = 0x03
	};
	LLAggregatePermissions();
	EValue getValue(PermissionBit bit) const;
	U8 getU8() const;
	BOOL isEmpty() const ;
	void aggregate(PermissionMask mask);
	void aggregate(const LLAggregatePermissions& ag);
	void packMessage(LLMessageSystem* msg, const char* field) const;
	void unpackMessage(LLMessageSystem* msg, const char* block, const char *field, S32 block_num = 0);
	static const LLAggregatePermissions empty;
	friend std::ostream& operator<<(std::ostream &s, const LLAggregatePermissions &perm);
protected:
	enum EPermIndex
	{
		PI_COPY = 0,
		PI_MODIFY = 1,
		PI_TRANSFER = 2,
		PI_END = 3,
		PI_COUNT = 3
	};
	void aggregateBit(EPermIndex idx, BOOL allowed);
	void aggregateIndex(EPermIndex idx, U8 bits);
	static EPermIndex perm2PermIndex(PermissionBit bit);
	U8 mBits[PI_COUNT];
};
LLSD ll_create_sd_from_permissions(const LLPermissions& perm);
LLPermissions ll_permissions_from_sd(const LLSD& sd_perm);
#endif
