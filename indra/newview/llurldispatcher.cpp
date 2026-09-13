/**
 * @file llurldispatcher.cpp
 * @brief Central registry for all URL handlers
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "llurldispatcher.h"
#include "llagent.h"
#include "llcommandhandler.h"
#include "llfloaterurldisplay.h"
#include "llfloaterdirectory.h"
#include "llfloaterworldmap.h"
#include "llpanellogin.h"
#include "llregionhandle.h"
#include "llslurl.h"
#include "llstartup.h"
#include "llweb.h"
#include "llworldmap.h"
#include "llworldmapmessage.h"
#include "llviewernetwork.h"
#include "hippogridmanager.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsd.h"
class LLURLDispatcherImpl
{
public:
	static bool dispatch(const LLSLURL& slurl,
						 const std::string& nav_type,
						 LLMediaCtrl* web,
						 bool trusted_browser);
	static bool dispatchRightClick(const LLSLURL& slurl);
private:
	static bool dispatchCore(const LLSLURL& slurl,
							 const std::string& nav_type,
							 bool right_mouse,
							 LLMediaCtrl* web,
							 bool trusted_browser);
	static bool dispatchHelp(const LLSLURL& slurl, bool right_mouse);
	static bool dispatchApp(const LLSLURL& slurl,
							const std::string& nav_type,
							bool right_mouse,
							LLMediaCtrl* web,
							bool trusted_browser);
	static bool dispatchRegion(const LLSLURL& slurl, const std::string& nav_type, bool right_mouse);
	static void regionHandleCallback(U64 handle, const LLSLURL& slurl,
		const LLUUID& snapshot_id, bool teleport);
	static void regionNameCallback(U64 handle, const LLSLURL& slurl,
		const LLUUID& snapshot_id, bool teleport);
	friend class LLTeleportHandler;
};
bool LLURLDispatcherImpl::dispatchCore(const LLSLURL& slurl,
									   const std::string& nav_type,
									   bool right_mouse,
									   LLMediaCtrl* web,
									   bool trusted_browser)
{
	switch(slurl.getType())
	{
		case LLSLURL::APP:
			return dispatchApp(slurl, nav_type, right_mouse, web, trusted_browser);
		case LLSLURL::LOCATION:
			return dispatchRegion(slurl, nav_type, right_mouse);
		default:
			return false;
	}
}
bool LLURLDispatcherImpl::dispatch(const LLSLURL& slurl,
								   const std::string& nav_type,
								   LLMediaCtrl* web,
								   bool trusted_browser)
{
	const bool right_click = false;
	return dispatchCore(slurl, nav_type, right_click, web, trusted_browser);
}
bool LLURLDispatcherImpl::dispatchRightClick(const LLSLURL& slurl)
{
	const bool right_click = true;
	LLMediaCtrl* web = nullptr;
	const bool trusted_browser = false;
	return dispatchCore(slurl, "clicked", right_click, web, trusted_browser);
}
bool LLURLDispatcherImpl::dispatchApp(const LLSLURL& slurl,
									  const std::string& nav_type,
									  bool right_mouse,
									  LLMediaCtrl* web,
									  bool trusted_browser)
{
	LL_INFOS() << "cmd: " << slurl.getAppCmd() << " path: " << slurl.getAppPath() << " query: " << slurl.getAppQuery() << LL_ENDL;
	const LLSD& query_map = LLURI::queryMap(slurl.getAppQuery());
	bool handled = LLCommandDispatcher::dispatch(
			slurl.getAppCmd(), slurl.getAppPath(), query_map, web, nav_type, trusted_browser);
	if (! handled)
	{
		LLNotificationsUtil::add("UnsupportedCommandSLURL");
	}
	return true;
}
bool LLURLDispatcherImpl::dispatchRegion(const LLSLURL& slurl, const std::string& nav_type, bool right_mouse)
{
	if(slurl.getType() != LLSLURL::LOCATION)
    {
		return false;
    }
	if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
	{
		LLPanelLogin::setLocation(slurl);
		return true;
	}
	LLSLURL _slurl = slurl;
	const std::string& grid = slurl.getGrid();
	const std::string& current_grid = gHippoGridManager->getCurrentGrid()->getGridName();
	if (grid != current_grid)
	{
		_slurl = LLSLURL(llformat("%s:%s", grid.c_str(), slurl.getRegion().c_str()), slurl.getPosition());
	}
	LLWorldMapMessage::getInstance()->sendNamedRegionRequest(slurl.getRegion(),
									  LLURLDispatcherImpl::regionNameCallback,
									  _slurl.getSLURLString(),
									  LLUI::sConfigGroup->getBOOL("SLURLTeleportDirectly"));
	return true;
}
void LLURLDispatcherImpl::regionNameCallback(U64 region_handle, const LLSLURL& slurl, const LLUUID& snapshot_id, bool teleport)
{
  if(slurl.getType() == LLSLURL::LOCATION)
    {
      regionHandleCallback(region_handle, slurl, snapshot_id, teleport);
    }
}
void LLURLDispatcherImpl::regionHandleCallback(U64 region_handle, const LLSLURL& slurl, const LLUUID& snapshot_id, bool teleport)
{
	HippoGridInfo* new_grid = gHippoGridManager->getGrid(slurl.getGrid());
	if(   new_grid
	   != gHippoGridManager->getCurrentGrid())
	{
		LLSD args;
		args["SLURL"] = slurl.getLocationString();
		args["CURRENT_GRID"] = gHippoGridManager->getCurrentGrid()->getGridName();
		std::string grid_label = new_grid ? new_grid->getGridName() : "";
		if(!grid_label.empty())
		{
			args["GRID"] = grid_label;
		}
		else
		{
			args["GRID"] = slurl.getGrid() + " (Unrecognized)";
		}
		LLNotificationsUtil::add("CantTeleportToGrid", args);
		return;
	}
	LLVector3d global_pos = from_region_handle(region_handle);
	LLVector3 local_pos = slurl.getPosition();
	global_pos += LLVector3d(local_pos);
	if (teleport)
	{
		gAgent.teleportViaLocation(global_pos);
		if(gFloaterWorldMap)
		{
			gFloaterWorldMap->trackLocation(global_pos);
		}
	}
	else
	{
		LLFloaterURLDisplay* url_displayp = LLFloaterURLDisplay::getInstance(LLSD());
		url_displayp->displayParcelInfo(region_handle, local_pos);
		if(snapshot_id.notNull())
		{
			url_displayp->setSnapshotDisplay(snapshot_id);
		}
		std::string locationString = llformat("%s %i, %i, %i", slurl.getRegion().c_str(), (S32)local_pos.mV[VX],(S32)local_pos.mV[VY],(S32)local_pos.mV[VZ]);
		url_displayp->setLocationString(locationString);
	}
}
class LLTeleportHandler : public LLCommandHandler
{
public:
	LLTeleportHandler() : LLCommandHandler("teleport", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web) override
	{
		if (tokens.size() < 1) return false;
		LLVector3 coords(128, 128, 0);
		if (tokens.size() <= 4)
		{
			coords = LLVector3(tokens[1].asReal(),
							   tokens[2].asReal(),
							   tokens[3].asReal());
		}
		std::string region_name = LLURI::unescape(tokens[0]);
		LLSD args;
		args["LOCATION"] = region_name;
		LLSD payload;
		payload["region_name"] = region_name;
		payload["callback_url"] = LLSLURL(region_name, coords).getSLURLString();
		LLNotificationsUtil::add("TeleportViaSLAPP", args, payload);
		return true;
	}
	static void teleport_via_slapp(std::string region_name, std::string callback_url)
	{
		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name,
			LLURLDispatcherImpl::regionHandleCallback,
			callback_url,
			true);
	}
	static bool teleport_via_slapp_callback(const LLSD& notification, const LLSD& response)
	{
		S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
		std::string region_name = notification["payload"]["region_name"].asString();
		std::string callback_url = notification["payload"]["callback_url"].asString();
		if (option == 0)
		{
			teleport_via_slapp(region_name, callback_url);
			return true;
		}
		return false;
	}
};
LLTeleportHandler gTeleportHandler;
static LLNotificationFunctorRegistration open_landmark_callback_reg("TeleportViaSLAPP", LLTeleportHandler::teleport_via_slapp_callback);
bool LLURLDispatcher::dispatch(const std::string& slurl,
							   const std::string& nav_type,
							   LLMediaCtrl* web,
							   bool trusted_browser)
{
	return LLURLDispatcherImpl::dispatch(LLSLURL(slurl), nav_type, web, trusted_browser);
}
bool LLURLDispatcher::dispatchRightClick(const std::string& slurl)
{
	return LLURLDispatcherImpl::dispatchRightClick(LLSLURL(slurl));
}
bool LLURLDispatcher::dispatchFromTextEditor(const std::string& slurl, bool trusted_content)
{
	LLMediaCtrl* web = nullptr;
	return LLURLDispatcherImpl::dispatch(LLSLURL(slurl), "clicked", web, trusted_content);
}
