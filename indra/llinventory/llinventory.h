/** 
 * @file llinventory.h
 * @brief LLInventoryItem and LLInventoryCategory class declaration.
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
#ifndef LL_LLINVENTORY_H
#define LL_LLINVENTORY_H
#include "llfoldertype.h"
#include "llinventorytype.h"
#include "llpermissions.h"
#include "llrefcount.h"
#include "llsaleinfo.h"
#include "llsd.h"
#include "lluuid.h"
class LLMessageSystem;
class LLInventoryObject : public LLRefCount
{
public:
	typedef std::list<LLPointer<LLInventoryObject> > object_list_t;
	typedef std::list<LLConstPointer<LLInventoryObject> > const_object_list_t;
public:
	LLInventoryObject();
	LLInventoryObject(const LLUUID& uuid,
					  const LLUUID& parent_uuid,
					  LLAssetType::EType type,
					  const std::string& name);
	void copyObject(const LLInventoryObject* other);
protected:
	virtual ~LLInventoryObject();
public:
	virtual const LLUUID& getUUID() const;
	virtual const LLUUID& getLinkedUUID() const;
	const LLUUID& getParentUUID() const;
	virtual const std::string& getName() const;
	virtual LLAssetType::EType getType() const;
	LLAssetType::EType getActualType() const;
	BOOL getIsLinkType() const;
	virtual time_t getCreationDate() const;
public:
	void setUUID(const LLUUID& new_uuid);
	virtual void rename(const std::string& new_name);
	void setParent(const LLUUID& new_parent);
	void setType(LLAssetType::EType type);
	virtual void setCreationDate(time_t creation_date_utc);
	static void correctInventoryName(std::string& name);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;
	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;
	virtual void updateParentOnServer(BOOL) const;
	virtual void updateServer(BOOL) const;
protected:
	LLUUID mUUID;
	LLUUID mParentUUID;
	LLAssetType::EType mType;
	std::string mName;
	time_t mCreationDate;
};
class LLInventoryItem : public LLInventoryObject
{
public:
	typedef std::vector<LLPointer<LLInventoryItem> > item_array_t;
public:
	LLInventoryItem(const LLUUID& uuid,
					const LLUUID& parent_uuid,
					const LLPermissions& permissions,
					const LLUUID& asset_uuid,
					LLAssetType::EType type,
					LLInventoryType::EType inv_type,
					const std::string& name,
					const std::string& desc,
					const LLSaleInfo& sale_info,
					U32 flags,
					S32 creation_date_utc);
	LLInventoryItem();
	LLInventoryItem(const LLInventoryItem* other);
	virtual void copyItem(const LLInventoryItem* other);
	void generateUUID() { mUUID.generate(); }
protected:
	~LLInventoryItem();
public:
	virtual const LLUUID& getLinkedUUID() const;
	virtual const LLPermissions& getPermissions() const;
	virtual const LLUUID& getCreatorUUID() const;
	virtual const LLUUID& getAssetUUID() const;
	virtual const std::string& getDescription() const;
	virtual const std::string& getActualDescription() const;
	virtual const LLSaleInfo& getSaleInfo() const;
	virtual LLInventoryType::EType getInventoryType() const;
	virtual U32 getFlags() const;
	virtual time_t getCreationDate() const;
	virtual U32 getCRC32() const;
public:
	void setAssetUUID(const LLUUID& asset_id);
	static void correctInventoryDescription(std::string& name);
	void setDescription(const std::string& new_desc);
	void setSaleInfo(const LLSaleInfo& sale_info);
	void setPermissions(const LLPermissions& perm);
	void setInventoryType(LLInventoryType::EType inv_type);
	void setFlags(U32 flags);
	void setCreator(const LLUUID& creator);
	void accumulatePermissionSlamBits(const LLInventoryItem& old_item);
	virtual void packMessage(LLMessageSystem* msg) const;
	virtual BOOL unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
public:
	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;
	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;
public:
	S32 packBinaryBucket(U8* bin_bucket, LLPermissions* perm_override = NULL) const;
	void unpackBinaryBucket(U8* bin_bucket, S32 bin_bucket_size);
	LLSD asLLSD() const;
	void asLLSD( LLSD& sd ) const;
	bool fromLLSD(const LLSD& sd, bool is_new = true);
protected:
	LLPermissions mPermissions;
	LLUUID mAssetUUID;
	std::string mDescription;
	LLSaleInfo mSaleInfo;
	LLInventoryType::EType mInventoryType;
	U32 mFlags;
};
class LLInventoryCategory : public LLInventoryObject
{
public:
	typedef std::vector<LLPointer<LLInventoryCategory> > cat_array_t;
public:
	LLInventoryCategory(const LLUUID& uuid, const LLUUID& parent_uuid,
						LLFolderType::EType preferred_type,
						const std::string& name);
	LLInventoryCategory();
	LLInventoryCategory(const LLInventoryCategory* other);
	void copyCategory(const LLInventoryCategory* other);
protected:
	~LLInventoryCategory();
public:
	LLFolderType::EType getPreferredType() const;
	void setPreferredType(LLFolderType::EType type);
	LLSD asLLSD() const;
	bool fromLLSD(const LLSD& sd);
	bool isPreferredTypeRoot() const;
public:
	virtual void packMessage(LLMessageSystem* msg) const;
	virtual void unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
public:
	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;
	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;
protected:
	LLFolderType::EType	mPreferredType;
};
LLSD ll_create_sd_from_inventory_item(LLPointer<LLInventoryItem> item);
LLSD ll_create_sd_from_inventory_category(LLPointer<LLInventoryCategory> cat);
LLPointer<LLInventoryCategory> ll_create_category_from_sd(const LLSD& sd_cat);
#endif
