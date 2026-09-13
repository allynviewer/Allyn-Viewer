/** 
 * @file lllAccountingQuotaManager.h
 * @
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#ifndef LL_ACCOUNTINGQUOTAMANAGER_H
#define LL_ACCOUNTINGQUOTAMANAGER_H
#include "llhandle.h"
#include "llaccountingcost.h"
class LLAccountingCostObserver
{
public:
	LLAccountingCostObserver() { mObserverHandle.bind(this); }
	virtual ~LLAccountingCostObserver() {}
	virtual void onWeightsUpdate(const SelectionCost& selection_cost) = 0;
	virtual void setErrorStatus(U32 status, const std::string& reason) = 0;
	const LLHandle<LLAccountingCostObserver>& getObserverHandle() const { return mObserverHandle; }
	const LLUUID& getTransactionID() { return mTransactionID; }
protected:
	virtual void generateTransactionID() = 0;
	LLRootHandle<LLAccountingCostObserver> mObserverHandle;
	LLUUID		mTransactionID;
};
class LLAccountingCostManager : public LLSingleton<LLAccountingCostManager>
{
public:
	LLAccountingCostManager();
	void addObject( const LLUUID& objectID );
	void fetchCosts( eSelectionType selectionType, const std::string& url,
			const LLHandle<LLAccountingCostObserver>& observer_handle );
	void removePendingObject( const LLUUID& objectID );
private:
	uuid_set_t mObjectList;
	uuid_set_t mPendingObjectQuota;
	typedef uuid_set_t::iterator IDIt;
};
#endif
