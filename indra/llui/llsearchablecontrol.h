/**
 * @file llsearchablecontrol.h
 * @brief Highlightable UI control mixin used by preferences search.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_SEARCHABLE_CONTROL_H
#define LL_SEARCHABLE_CONTROL_H

#include "llui.h"
#include "v4color.h"

namespace ll
{
	namespace ui
	{
		class SearchableControl
		{
			mutable bool mIsHighlighed;
		public:
			SearchableControl()
				: mIsHighlighed(false)
			{ }
			virtual ~SearchableControl()
			{ }

			const LLColor4& getHighlightBgColor() const
			{
				static LLColor4 fallback(139.f / 255.f, 92.f / 255.f, 246.f / 255.f, 70.f / 255.f);
				if (LLUI::sColorsGroup && LLUI::sColorsGroup->controlExists("SearchableControlHighlightBgColor"))
				{
					static LLColor4 named = LLUI::sColorsGroup->getColor("SearchableControlHighlightBgColor");
					return named;
				}
				return fallback;
			}

			const LLColor4& getHighlightFontColor() const
			{
				static LLColor4 fallback(1.f, 214.f / 255.f, 80.f / 255.f, 1.f);
				if (LLUI::sColorsGroup && LLUI::sColorsGroup->controlExists("SearchableControlHighlightFontColor"))
				{
					static LLColor4 named = LLUI::sColorsGroup->getColor("SearchableControlHighlightFontColor");
					return named;
				}
				return fallback;
			}

			void setHighlighted(bool aVal) const
			{
				mIsHighlighed = aVal;
				onSetHighlight();
			}
			bool getHighlighted() const
			{ return mIsHighlighed; }

			std::string getSearchText() const
			{ return _getSearchText(); }
		protected:
			virtual std::string _getSearchText() const = 0;
			virtual void onSetHighlight() const
			{ }
		};
	}
}

#endif
