/** 
 * @file llstartup.h
 * @brief startup routines and logic declaration
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#ifndef LL_LLSTARTUP_H
#define LL_LLSTARTUP_H
#include <boost/scoped_ptr.hpp>
class LLViewerTexture ;
class LLEventPump;
class LLSLURL;
#include "llviewerstats.h"
bool idle_startup();
void release_start_screen();
bool login_alert_done(const LLSD& notification, const LLSD& response);
extern std::string SCREEN_HOME_FILENAME;
extern std::string SCREEN_LAST_FILENAME;
typedef enum {
	STATE_FIRST,
	STATE_BROWSER_INIT,
	STATE_LOGIN_SHOW,
	STATE_LOGIN_WAIT,
	STATE_LOGIN_CLEANUP,
	STATE_LOGIN_VOICE_LICENSE,
	STATE_UPDATE_CHECK,
	STATE_LOGIN_AUTH_INIT,
	STATE_XMLRPC_LEGACY_LOGIN,
	STATE_LOGIN_NO_DATA_YET,
	STATE_LOGIN_DOWNLOADING,
	STATE_LOGIN_PROCESS_RESPONSE,
	STATE_WORLD_INIT,
	STATE_MULTIMEDIA_INIT,
	STATE_FONT_INIT,
	STATE_SEED_GRANTED_WAIT,
	STATE_SEED_CAP_GRANTED,
	STATE_WORLD_WAIT,
	STATE_AGENT_SEND,
	STATE_AGENT_WAIT,
	STATE_INVENTORY_SEND,
	STATE_MISC,
	STATE_PRECACHE,
	STATE_WEARABLES_WAIT,
	STATE_CLEANUP,
	STATE_STARTED
} EStartupState;
extern bool gAgentMovementCompleted;
extern LLPointer<LLViewerTexture> gStartTexture;
extern std::string gInitialOutfit;
extern std::string gInitialOutfitGender;
class LLStartUp
{
public:
	static bool canGoFullscreen();
	static void setStartupState( EStartupState state );
	static EStartupState getStartupState() { return gStartupState; };
	static std::string getStartupStateString() { return startupStateToString(gStartupState); };
	static void multimediaInit();
	static void fontInit();
	static void initNameCache();
	static void initExperiences();
	static void cleanupNameCache();
	static void loadInitialOutfit( const std::string& outfit_folder_name,
								   const std::string& gender_name );
	static void saveInitialOutfit();
	static std::string& getInitialOutfitName();
	static std::string loadPasswordFromDisk();
	static void savePasswordToDisk(const std::string& hashed_password);
	static void deletePasswordFromDisk();
	static bool dispatchURL();
	static void postStartupState();
	static void setStartSLURL(const LLSLURL& slurl);
	static LLSLURL& getStartSLURL();
	static bool startLLProxy();
	static LLViewerStats::PhaseMap& getPhases() { return *sPhases; }
	static LLEventPump& getStateEventPump() { return *sStateWatcher; }
private:
	static LLSLURL sStartSLURL;
	static std::string startupStateToString(EStartupState state);
	static EStartupState gStartupState;
	static boost::scoped_ptr<LLEventPump> sStateWatcher;
	static boost::scoped_ptr<LLViewerStats::PhaseMap> sPhases;
};
#endif
