/** 
 * @file llpluginclassmediaowner.h
 * @brief LLPluginClassMedia handles interaction with a plugin which knows about the "media" message class.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */
#ifndef LL_LLPLUGINCLASSMEDIAOWNER_H
#define LL_LLPLUGINCLASSMEDIAOWNER_H
#include "llpluginprocessparent.h"
#include "llrect.h"
#include <queue>
class LLPluginClassMedia;
class LLPluginClassMediaOwner
{
public:
	typedef enum
	{
		MEDIA_EVENT_CONTENT_UPDATED,
		MEDIA_EVENT_TIME_DURATION_UPDATED,
		MEDIA_EVENT_SIZE_CHANGED,
		MEDIA_EVENT_CURSOR_CHANGED,
		MEDIA_EVENT_NAVIGATE_BEGIN,
		MEDIA_EVENT_NAVIGATE_COMPLETE,
		MEDIA_EVENT_PROGRESS_UPDATED,
		MEDIA_EVENT_STATUS_TEXT_CHANGED,
		MEDIA_EVENT_NAME_CHANGED,
		MEDIA_EVENT_LOCATION_CHANGED,
		MEDIA_EVENT_NAVIGATE_ERROR_PAGE,
		MEDIA_EVENT_CLICK_LINK_HREF,
		MEDIA_EVENT_CLICK_LINK_NOFOLLOW,
		MEDIA_EVENT_CLOSE_REQUEST,
		MEDIA_EVENT_PICK_FILE_REQUEST,
		MEDIA_EVENT_GEOMETRY_CHANGE,
		MEDIA_EVENT_PLUGIN_FAILED_LAUNCH,
		MEDIA_EVENT_PLUGIN_FAILED,
		MEDIA_EVENT_AUTH_REQUEST,
		MEDIA_EVENT_FILE_DOWNLOAD,
		MEDIA_EVENT_DEBUG_MESSAGE,
		MEDIA_EVENT_LINK_HOVERED
	} EMediaEvent;
	typedef enum
	{
		MEDIA_NONE,
		MEDIA_LOADING,
		MEDIA_LOADED,
		MEDIA_ERROR,
		MEDIA_PLAYING,
		MEDIA_PAUSED,
		MEDIA_DONE
	} EMediaStatus;
	virtual ~LLPluginClassMediaOwner() {};
	virtual void handleMediaEvent(LLPluginClassMedia* , EMediaEvent ) {};
};
#endif
