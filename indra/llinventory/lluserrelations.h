/** 
 * @file lluserrelations.h
 * @author Phoenix
 * @date 2006-10-12
 * @brief Declaration of a class for handling granted rights.
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
#ifndef LL_LLUSERRELAIONS_H
#define LL_LLUSERRELAIONS_H
#include <map>
#include "lluuid.h"
class LLRelationship
{
public:
	LLRelationship();
	LLRelationship(S32 grant_to, S32 grant_from, bool is_online);
	static const LLRelationship DEFAULT_RELATIONSHIP;
	bool isOnline() const;
	void online(bool is_online);
	enum
	{
		GRANT_NONE = 0x0,
		GRANT_ONLINE_STATUS = 0x1,
		GRANT_MAP_LOCATION = 0x2,
		GRANT_MODIFY_OBJECTS = 0x4,
	};
	static const U8 GRANTED_VISIBLE_MASK;
	bool isRightGrantedTo(S32 rights) const;
	bool isRightGrantedFrom(S32 rights) const;
	S32 getRightsGrantedTo() const;
	S32 getRightsGrantedFrom() const;
	void setRightsTo(S32 to_agent) { mGrantToAgent = to_agent; mChangeSerialNum++; }
	void setRightsFrom(S32 from_agent) { mGrantFromAgent = from_agent; mChangeSerialNum++;}
	S32 getChangeSerialNum() const { return mChangeSerialNum; }
	void grantRights(S32 to_agent, S32 from_agent);
	void revokeRights(S32 to_agent, S32 from_agent);
protected:
	S32 mGrantToAgent;
	S32 mGrantFromAgent;
	S32 mChangeSerialNum;
	bool mIsOnline;
};
#endif
