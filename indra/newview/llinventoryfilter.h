/** 
* @file llinventoryfilter.h
* @brief Support for filtering your inventory to only display a subset of the
* available items.
*
* $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#ifndef LLINVENTORYFILTER_H
#define LLINVENTORYFILTER_H
#include "llinventorytype.h"
#include "llpermissionsflags.h"
class LLFolderViewItem;
class LLFolderViewFolder;
class LLInventoryItem;
class LLInventoryFilter
{
public:
	enum EFilterModified
	{
		FILTER_NONE,
		FILTER_RESTART,
		FILTER_LESS_RESTRICTIVE,
		FILTER_MORE_RESTRICTIVE
	};
	enum EFolderShow
	{
		SHOW_ALL_FOLDERS,
		SHOW_NON_EMPTY_FOLDERS,
		SHOW_NO_FOLDERS
	};
	enum EFilterType	{
		FILTERTYPE_NONE = 0,
		FILTERTYPE_OBJECT = 0x1 << 0,
		FILTERTYPE_CATEGORY = 0x1 << 1,
		FILTERTYPE_UUID	= 0x1 << 2,
		FILTERTYPE_DATE = 0x1 << 3,
		FILTERTYPE_WEARABLE = 0x1 << 4,
		FILTERTYPE_EMPTYFOLDERS = 0x1 << 5,
		FILTERTYPE_MARKETPLACE_ACTIVE = 0x1 << 6,
		FILTERTYPE_MARKETPLACE_INACTIVE = 0x1 << 7,
		FILTERTYPE_MARKETPLACE_UNASSOCIATED = 0x1 << 8,
		FILTERTYPE_MARKETPLACE_LISTING_FOLDER = 0x1 << 9,
		FILTERTYPE_NO_MARKETPLACE_ITEMS = 0x1 << 10,
		FILTERTYPE_WORN = 0x1 << 11
	};
	enum EFilterDateDirection
	{
		FILTERDATEDIRECTION_NEWER,
		FILTERDATEDIRECTION_OLDER
	};
	enum EFilterLink
	{
		FILTERLINK_INCLUDE_LINKS,
		FILTERLINK_EXCLUDE_LINKS,
		FILTERLINK_ONLY_LINKS
	};
	enum ESortOrderType
	{
		SO_NAME = 0,
		SO_DATE = 0x1,
		SO_FOLDERS_BY_NAME = 0x1 << 1,
		SO_SYSTEM_FOLDERS_TO_TOP = 0x1 << 2,
		SO_FOLDERS_BY_WEIGHT = 0x1 << 3,
	};
	struct FilterOps
	{
		struct DateRange : public LLInitParam::Block<DateRange>
		{
			Optional<time_t>	min_date,
								max_date;
			DateRange()
			:	min_date("min_date", time_min()),
				max_date("max_date", time_max())
			{}
			bool validateBlock(bool emit_errors = true) const;
		};
		struct Params : public LLInitParam::Block<Params>
		{
			Optional<U32>				types;
			Optional<U64>				object_types,
										wearable_types,
										category_types,
										worn_items;
			Optional<EFilterLink>		links;
			Optional<LLUUID>			uuid;
			Optional<DateRange>			date_range;
			Optional<U32>				hours_ago;
			Optional<U32>				date_search_direction;
			Optional<EFolderShow>		show_folder_state;
			Optional<PermissionMask>	permissions;
			Params()
			:	types("filter_types", FILTERTYPE_OBJECT),
				object_types("object_types", 0xffffFFFFffffFFFFULL),
				wearable_types("wearable_types", 0),
				category_types("category_types", 0xffffFFFFffffFFFFULL),
				worn_items("worn_items", 0xffffFFFFffffFFFFULL),
				links("links", FILTERLINK_INCLUDE_LINKS),
				uuid("uuid"),
				date_range("date_range"),
				hours_ago("hours_ago", 0),
				date_search_direction("date_search_direction", FILTERDATEDIRECTION_NEWER),
				show_folder_state("show_folder_state", SHOW_NON_EMPTY_FOLDERS),
				permissions("permissions", PERM_NONE)
			{
				addSynonym(links, "filter_links");
			}
		};
		FilterOps(const Params& = Params());
		U32 			mFilterTypes;
		U64				mFilterObjectTypes,
						mFilterWearableTypes,
						mFilterCategoryTypes,
						mFilterWornItems;
		EFilterLink		mFilterLinks;
		LLUUID      	mFilterUUID;
		time_t			mMinDate,
						mMaxDate;
		U32				mHoursAgo;
		U32				mDateSearchDirection;
		EFolderShow		mShowFolderState;
		PermissionMask	mPermissions;
		bool			mFilterWorn;
	};
	struct Params : public LLInitParam::Block<Params>
	{
		Optional<std::string>		name;
		Optional<FilterOps::Params>	filter_ops;
		Optional<std::string>		substring;
		Optional<bool>				since_logoff;
		Params()
		:	name("name"),
			filter_ops(""),
			substring("substring"),
			since_logoff("since_logoff")
		{}
	};
	LLInventoryFilter(const Params& p = Params());
	LLInventoryFilter(const LLInventoryFilter& other) { *this = other; }
	virtual ~LLInventoryFilter() {}
	U64 				getFilterTypes() const;
	U64 				getFilterObjectTypes() const;
	U64					getFilterCategoryTypes() const;
	U64					getFilterWearableTypes() const;
	U64					getFilterWornItems() const;
	bool 				isFilterObjectTypesWith(LLInventoryType::EType t) const;
	void 				setFilterObjectTypes(U64 types);
	void 				setFilterCategoryTypes(U64 types);
	void 				setFilterUUID(const LLUUID &object_id);
	void				setFilterWearableTypes(U64 types);
	void				setFilterEmptySystemFolders();
	void				setFilterMarketplaceActiveFolders();
	void				setFilterMarketplaceInactiveFolders();
	void				setFilterMarketplaceUnassociatedFolders();
	void				setFilterMarketplaceListingFolders(bool select_only_listing_folders);
	void				setFilterNoMarketplaceFolder();
	void				updateFilterTypes(U64 types, U64& current_types);
	void				setFilterWornItems();
	void 				setFilterSubString(const std::string& string);
	const std::string& 	getFilterSubString(BOOL trim = FALSE) const;
	const std::string& 	getFilterSubStringOrig() const { return mFilterSubStringOrig; }
	bool 				hasFilterString() const;
	void setFilterPermissions(PermissionMask perms);
	PermissionMask 		getFilterPermissions() const;
	void setDateRange(time_t min_date, time_t max_date);
	void setDateRangeLastLogoff(BOOL sl);
	time_t 				getMinDate() const;
	time_t 				getMaxDate() const;
	void setHoursAgo(U32 hours);
	U32 				getHoursAgo() const;
	void				setDateSearchDirection(U32 direction);
	U32					getDateSearchDirection() const;
	void 				setFilterLinks(EFilterLink filter_link);
	EFilterLink			getFilterLinks() const;
	bool 				check(LLFolderViewItem* item);
	bool				check(const LLInventoryItem* item);
	bool				checkFolder(const LLFolderViewFolder* folder) const;
	bool				checkFolder(const LLUUID& folder_id) const;
	bool				showAllResults() const;
	std::string::size_type getStringMatchOffset() const;
	std::string::size_type getFilterStringSize() const;
	void 				setShowFolderState( EFolderShow state);
	EFolderShow 		getShowFolderState() const;
	void 				setEmptyLookupMessage(const std::string& message);
	std::string			getEmptyLookupMessage() const;
	void 				setSortOrder(U32 order);
	U32 				getSortOrder() const;
	bool 				isActive() const;
	bool 				isModified() const;
	bool				isSinceLogoff() const;
	void 				clearModified();
	const std::string& 	getName() const { return mName; }
	const std::string& 	getFilterText();
	void setModified(EFilterModified behavior = FILTER_RESTART);
	void 				setFilterCount(S32 count);
	S32 				getFilterCount() const;
	void 				decrementFilterCount();
	bool 				isDefault() const;
	bool 				isNotDefault() const;
	void 				markDefault();
	void 				resetDefault();
	S32 				getCurrentGeneration() const;
	S32 				getFirstSuccessGeneration() const;
	S32 				getFirstRequiredGeneration() const;
	void 				toParams(Params& params) const;
	void 				fromParams(const Params& p);
	LLInventoryFilter& operator =(const LLInventoryFilter& other);
private:
	bool				areDateLimitsSet();
	bool 				checkAgainstFilterType(const LLFolderViewItem* item) const;
	bool 				checkAgainstFilterType(const LLInventoryItem* item) const;
	bool 				checkAgainstPermissions(const LLFolderViewItem* item) const;
	bool 				checkAgainstPermissions(const LLInventoryItem* item) const;
	bool 				checkAgainstFilterLinks(const LLFolderViewItem* item) const;
	bool				checkAgainstClipboard(const LLUUID& object_id) const;
	U32						mOrder;
	FilterOps				mFilterOps;
	FilterOps				mDefaultFilterOps;
	std::string::size_type	mSubStringMatchOffset;
	std::string				mFilterSubString;
	std::string				mFilterSubStringOrig;
	const std::string		mName;
	S32						mCurrentGeneration;
	S32						mFirstRequiredGeneration;
	S32						mFirstSuccessGeneration;
	S32						mFilterCount;
	EFilterModified 		mFilterModified;
	std::string 			mFilterText;
	std::string 			mEmptyLookupMessage;
};
#endif
