/** 
 * @file llgroupnotify.h
 * @brief Non-blocking notification that doesn't take keyboard focus.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#ifndef LL_LLGROUPNOTIFY_H
#define LL_LLGROUPNOTIFY_H
#include "llfontgl.h"
#include "llpanel.h"
#include "lltimer.h"
#include "llviewermessage.h"
#include "llnotifications.h"
class LLButton;
class LLGroupNotifyBox final
:	public LLPanel,
	public LLInitClass<LLGroupNotifyBox>
{
public:
	void close();
	void sinkToWell();
	void unsinkFromWell();
	void hideToWell();
	bool isSunkToWell() const { return mSunkToWell; }
	static void initClass();
	static bool onNewNotification(const LLSD& notification);
	const LLUUID& getWellID() const { return mWellID; }
	const std::string& getWellTitle() const { return mSubject; }
protected:
	LLGroupNotifyBox(const std::string& subject,
							 const std::string& message,
							 const std::string& from_name,
							 const LLUUID& group_id,
							 const LLUUID& group_insignia,
							 const std::string& group_name,
							 const LLDate& time_stamp,
							 const bool& has_inventory,
							 const std::string& inventory_name,
							 const LLSD& inventory_offer);
	~LLGroupNotifyBox();
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	void draw() override;
	void moveToBack();
	void drawBackground() const;
	static LLRect getGroupNotifyRect();
	void onClickSaveInventory();
private:
	bool mAnimating;
	bool mSunkToWell;
	LLTimer mTimer;
	LLButton* mNextBtn;
	LLButton* mHideBtn;
	LLButton* mSaveInventoryBtn;
	LLUUID mGroupID;
	LLUUID mWellID;
	std::string mSubject;
	bool mHasInventory;
	LLOfferInfo* mInventoryOffer;
};
#endif
