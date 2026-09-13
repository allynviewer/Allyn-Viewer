/** 
 * @file lluseroperation.h
 * @brief LLUserOperation class header file - used for message based
 * transaction. For example, L$ transactions.
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
#ifndef LL_LLUSEROPERATION_H
#define LL_LLUSEROPERATION_H
#include "lluuid.h"
#include "llframetimer.h"
#include <map>
class LLUserOperation
{
public:
	LLUserOperation(const LLUUID& agent_id);
	LLUserOperation(const LLUUID& agent_id, const LLUUID& transaction_id);
	virtual ~LLUserOperation();
	const LLUUID& getTransactionID() const { return mTransactionID; }
	const LLUUID& getAgentID() const { return mAgentID; }
	virtual BOOL isExpired();
	void SetNoExpireFlag(const BOOL flag);
	virtual void sendRequest() = 0;
	virtual BOOL execute(BOOL transaction_success) = 0;
	virtual void expire();
protected:
	LLUserOperation();
protected:
	LLUUID mAgentID;
	LLUUID mTransactionID;
	LLFrameTimer mTimer;
	BOOL   mNoExpire;
};
class LLUserOperationMgr
{
public:
	LLUserOperationMgr();
	~LLUserOperationMgr();
	void addOperation(LLUserOperation* op);
	LLUserOperation* findOperation(const LLUUID& transaction_id);
	BOOL deleteOperation(LLUserOperation* op);
	void deleteExpiredOperations();
private:
	typedef std::map<LLUUID, LLUserOperation*> user_operation_list_t;
	user_operation_list_t mUserOperationList;
	LLUUID mLastOperationConsidered;
};
extern LLUserOperationMgr* gUserOperationMgr;
#endif
