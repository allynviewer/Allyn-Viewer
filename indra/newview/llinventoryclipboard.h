/** 
 * @file llinventoryclipboard.h
 * @brief LLInventoryClipboard class header file
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
#ifndef LL_LLINVENTORYCLIPBOARD_H
#define LL_LLINVENTORYCLIPBOARD_H
#include "lluuid.h"
class LLInventoryClipboard
{
public:
	static LLInventoryClipboard& instance() { return sInstance; }
	void add(const LLUUID& object);
	void store(const LLUUID& object);
	void store(const uuid_vec_t& inventory_objects);
	void cut(const LLUUID& object);
	void retrieve(uuid_vec_t& inventory_objects) const;
	void reset();
	BOOL hasContents() const;
	bool isOnClipboard(const LLUUID& object) const;
	bool isCutMode() const { return mCutMode; }
	void setCutMode(bool mode) { mCutMode = mode; }
protected:
	static LLInventoryClipboard sInstance;
	uuid_vec_t mObjects;
	bool mCutMode;
public:
	LLInventoryClipboard();
	~LLInventoryClipboard();
private:
	LLInventoryClipboard(const LLInventoryClipboard&);
	LLInventoryClipboard& operator=(const LLInventoryClipboard&);
};
#endif
