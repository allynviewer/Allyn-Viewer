/**
 * @file llfloateronlyshowself.h
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
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
 * $/LicenseInfo$
 */
#ifndef LL_LLFLOATERONLYSHOWSELF_H
#define LL_LLFLOATERONLYSHOWSELF_H
#include "llavatarname.h"
#include "llfloater.h"
class LLNameListCtrl;
class LLFloaterOnlyShowSelf : public LLFloater, public LLFloaterSingleton<LLFloaterOnlyShowSelf>
{
	friend class LLUISingleton<LLFloaterOnlyShowSelf, VisibilityPolicy<LLFloater> >;
public:
	BOOL postBuild() override;
	void onOpen() override;
	void onClose(bool app_quitting) override;
	void refreshList();
	static void refreshIfOpen();
private:
	LLFloaterOnlyShowSelf(const LLSD& seed);
	~LLFloaterOnlyShowSelf();
	void updateButtons();
	void onAdd();
	void onPicked(const uuid_vec_t& ids, const std::vector<LLAvatarName>& names);
	void onRemove();
	LLNameListCtrl* mList;
};
#endif
