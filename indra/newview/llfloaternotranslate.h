/**
 * @file llfloaternotranslate.h
 * @brief Floater listing residents excluded from chat/IM translation
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
#ifndef LL_LLFLOATERNOTRANSLATE_H
#define LL_LLFLOATERNOTRANSLATE_H
#include "llfloater.h"
class LLNameListCtrl;
class LLFloaterNoTranslate : public LLFloater, public LLFloaterSingleton<LLFloaterNoTranslate>
{
	friend class LLUISingleton<LLFloaterNoTranslate, VisibilityPolicy<LLFloater> >;
public:
	BOOL postBuild() override;
	void onOpen() override;
	void onClose(bool app_quitting) override;
	void refreshList();
	static void refreshIfOpen();
private:
	LLFloaterNoTranslate(const LLSD& seed);
	~LLFloaterNoTranslate();
	void updateButtons();
	void onReactivate();
	LLNameListCtrl* mList;
};
#endif
