/** 
 * @file llmenucommands.cpp
 * @brief Implementations of menu commands.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
#include "llmenucommands.h"
#include "aihttpview.h"
#include "alfloaterregiontracker.h"
#include "floaterao.h"
#include "floaterlocalassetbrowse.h"
#include "hbfloatergrouptitles.h"
#include "jcfloaterareasearch.h"
#include "llagentcamera.h"
#include "llchatbar.h"
#include "llconsole.h"
#include "lldebugview.h"
#include "llfasttimerview.h"
#include "llfloaterabout.h"
#include "llfloateractivespeakers.h"
#include "llfloaterautoreplacesettings.h"
#include "llfloateravatar.h"
#include "llfloateravatarlist.h"
#include "llfloaterbeacons.h"
#include "llfloaterblacklist.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbump.h"
#include "llfloaterbuycurrency.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloatercustomize.h"
#include "llfloaterdaycycle.h"
#include "llfloaterdestinations.h"
#include "llfloaterdisplayname.h"
#include "llfloatereditui.h"
#include "llfloaterenvsettings.h"
#include "llfloaterexperiences.h"
#include "llfloaterexploreanimations.h"
#include "llfloaterexploresounds.h"
#include "llfloaterfonttest.h"
#include "llfloatergesture.h"
#include "llfloatergodtools.h"
#include "llfloaterhud.h"
#include "llfloaterinspect.h"
#include "llfloaterinventory.h"
#include "llfloaterjoystick.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloatermarketplacelistings.h"
#include "llfloatermediafilter.h"
#include "llfloatermemleak.h"
#include "llfloatermessagelog.h"
#include "llfloatermute.h"
#include "llfloaternotranslate.h"
#include "llfloaternotificationsconsole.h"
#include "llfloaterpathfindingcharacters.h"
#include "llfloaterpathfindinglinksets.h"
#include "llfloaterperms.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterregiondebugconsole.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "llfloaterscriptdebug.h"
#include "llfloaterscriptlimits.h"
#include "llfloatersettingsdebug.h"
#include "llfloatersnapshot.h"
#include "llfloaterstats.h"
#include "llfloaterteleporthistory.h"
#include "llfloatertoolbarprefs.h"
#include "llfloatertest.h"
#include "llfloatervoiceeffect.h"
#include "llfloaterwater.h"
#include "llfloaterwebcontent.h"
#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llframestatview.h"
#include "llmakeoutfitdialog.h"
#include "lltextureview.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lluictrlfactory.h"
#include "llvelocitybar.h"
#include "llviewerparcelmgr.h"
#include "rlvfloaters.h"
#include "shfloatermediaticker.h"
void handle_debug_avatar_textures(void*);
template<typename T> void handle_singleton_toggle(void*);
void show_outfit_dialog() { new LLMakeOutfitDialog(false); }
class LLFloaterExperiencePicker* show_xp_picker(const LLSD& key);
void toggle_build() { LLToolMgr::getInstance()->toggleBuildMode(); }
void toggle_control(const std::string& name) { if (LLControlVariable* control = gSavedSettings.getControl(name)) control->set(!control->get()); }
void toggle_search_floater();
void toggle_always_run() { gAgent.getAlwaysRun() ? gAgent.clearAlwaysRun() : gAgent.setAlwaysRun(); }
void toggle_sit();
void toggle_mouselook() { gAgentCamera.cameraMouselook() ? gAgentCamera.changeCameraToDefault() : gAgentCamera.changeCameraToMouselook(); }
bool is_visible_view(std::function<LLView* ()> get)
{
	if (LLView* v = get())
		return v->getVisible();
	return false;
}
struct CommWrapper
{
	static bool only_comm()
	{
		static const LLCachedControl<bool> only("CommunicateSpecificShortcut");
		return only || LLFloaterChatterBox::getInstance()->getFloaterCount();
	}
	static bool instanceVisible(const LLSD& key) { return only_comm() ? LLFloaterChatterBox::instanceVisible(key) : LLFloaterMyFriends::instanceVisible(key); }
	static void toggleInstance(const LLSD& key) { only_comm() ? LLFloaterChatterBox::toggleInstance(key) : LLFloaterMyFriends::toggleInstance(key); }
};
struct MenuFloaterDict final : public LLSingleton<MenuFloaterDict>
{
	typedef std::map<const std::string, std::pair<std::function<void ()>, std::function<bool ()>>> menu_floater_map_t;
	menu_floater_map_t mEntries;
	MenuFloaterDict()
	{
		registerConsole("debug console", gDebugView->mDebugConsolep);
		registerConsole("fast timers", gDebugView->mFastTimerView);
		registerConsole("frame console", gDebugView->mFrameStatView);
		registerConsole("http console", gHttpView);
		registerConsole("texture console", gTextureView);
		if (gAuditTexture)
		{
			registerConsole("texture category console", gTextureCategoryView);
			registerConsole("texture size console", gTextureSizeView);
		}
		registerConsole("velocity", gVelocityBar);
		registerWindow("about", boost::bind(&LLFloaterAbout::show,nullptr), "floater_about");
		registerFloater("always run", boost::bind(toggle_always_run), boost::bind(&LLAgent::getAlwaysRun, &gAgent));
		registerWindow("anims_explorer", boost::bind(LLFloaterExploreAnimations::show), "floater_explore_animations");
		registerWindow("appearance", boost::bind(LLFloaterCustomize::show), "floater customize");
		registerFloater("asset_blacklist", boost::bind(LLFloaterBlacklist::toggle), boost::bind(LLFloaterBlacklist::visible));
		registerFloater("build", boost::bind(toggle_build));
		registerWindow("buy currency", boost::bind(LLFloaterBuyCurrency::buyCurrency), "buy currency");
		registerFloater("buy land", boost::bind(&LLViewerParcelMgr::startBuyLand, boost::bind(LLViewerParcelMgr::getInstance), false));
		registerWindow("complaint reporter", boost::bind(LLFloaterReporter::showFromMenu, COMPLAINT_REPORT), "floater_report_abuse");
		registerWindow("DayCycle", boost::bind(LLFloaterDayCycle::show), "Day Cycle Floater");
		registerFloater("debug avatar", boost::bind(handle_debug_avatar_textures, nullptr));
		registerFloater("debug settings", boost::bind(handle_singleton_toggle<LLFloaterSettingsDebug>, nullptr));
		registerWindow("edit ui", boost::bind(LLFloaterEditUI::show, nullptr), "floater_ui_editor");
		registerWindow("EnvSettings", boost::bind(LLFloaterEnvSettings::show), "Environment Editor Floater");
		registerWindow("experience_search", boost::bind(show_xp_picker, LLSD()), "experiencepicker");
		registerFloater("fly", boost::bind(LLAgent::toggleFlying));
		registerWindow("font test", boost::bind(LLFloaterFontTest::show, nullptr), "contents");
		registerWindow("god tools", boost::bind(LLFloaterGodTools::show, nullptr), "godtools floater");
		registerWindow("grid options", boost::bind(LLFloaterBuildOptions::show, nullptr), "build options floater");
		registerFloater("group titles", boost::bind(HBFloaterGroupTitles::toggle));
		registerWindow("help tutorial", boost::bind(LLFloaterHUD::showHUD), "floater_hud");
		registerFloater("inventory", boost::bind(LLPanelMainInventory::toggleVisibility, nullptr), boost::bind(is_visible_view, static_cast<std::function<LLView* ()> >(LLPanelMainInventory::getActiveInventory)));
		registerWindow("local assets", boost::bind(FloaterLocalAssetBrowser::show, (void*)0), "local_bitmap_browser_floater");
		registerWindow("mean events", boost::bind(LLFloaterBump::show, nullptr), "floater_bumps");
		registerFloater("media ticker", boost::bind(handle_ticker_toggle, nullptr), boost::bind(SHFloaterMediaTicker::instanceExists));
		registerWindow("memleak", boost::bind(LLFloaterMemLeak::show, nullptr), "MemLeak");
		registerWindow("messagelog", boost::bind(LLFloaterMessageLog::show), "Message Log");
		registerFloater("mouselook", boost::bind(toggle_mouselook));
		registerWindow("my land", boost::bind(LLFloaterLandHoldings::show, nullptr), "land holdings floater");
		registerFloater("outfit", boost::bind(show_outfit_dialog));
		registerWindow("preferences", boost::bind(LLFloaterPreference::show, nullptr),
			[]() -> LLFloater* { return LLFloaterPreference::findInstance(); });
		registerFloater("quit", boost::bind(&LLAppViewer::userQuit, LLAppViewer::instance()));
		registerFloater("RegionDebugConsole", boost::bind(handle_singleton_toggle<LLFloaterRegionDebugConsole>, nullptr), boost::bind(LLFloaterRegionDebugConsole::instanceExists));
		registerWindow("script errors", boost::bind(LLFloaterScriptDebug::show, LLUUID::null), "script debug floater");
		registerFloater("search", boost::bind(toggle_search_floater));
		registerFloater("show inspect", boost::bind(LLFloaterInspect::toggleInstance, LLSD()), boost::bind(LLFloaterInspect::instanceVisible, LLSD()));
		registerFloater("sit", boost::bind(toggle_sit));
		registerWindow("snapshot", boost::bind(LLFloaterSnapshot::show, nullptr), "Snapshot");
		registerFloater("sound_explorer", boost::bind(LLFloaterExploreSounds::toggle), boost::bind(LLFloaterExploreSounds::visible));
		registerWindow("test", boost::bind(LLFloaterTest::show, nullptr), "test");
		registerWindow("WaterSettings", boost::bind(LLFloaterWater::show), "Water Floater");
		registerWindow("Windlight", boost::bind(LLFloaterWindLight::show), "WindLight floater");
		registerFloater("world map", boost::bind(LLFloaterWorldMap::toggle));
		registerFloater<LLFloaterLand>					("about land");
		registerFloater<LLFloaterRegionInfo>			("about region");
		registerFloater<LLFloaterActiveSpeakers>		("active speakers");
		registerFloater<LLFloaterAO>					("ao");
		registerFloater<JCFloaterAreaSearch>			("areasearch");
		registerFloater<LLFloaterAutoReplaceSettings>	("autoreplace");
		registerFloater<LLFloaterAvatar>				("avatar");
		registerFloater<LLFloaterBeacons>				("beacons");
		registerFloater<LLFloaterCamera>				("camera controls");
		registerFloater<LLFloaterCamera>				("movement controls");
		registerFloater<LLFloaterChat>					("chat history");
		registerFloater<LLFloaterChatterBox>			("communicate");
		registerFloater<LLFloaterDestinations>			("destinations");
		registerFloater<LLFloaterDisplayName>			("displayname");
		registerFloater<LLFloaterExperiences>				("experiences");
		registerFloater<LLFloaterMyFriends>				("friends", 0);
		registerFloater<LLFloaterGesture>				("gestures");
		registerFloater<LLFloaterMyFriends>				("groups", 1);
		registerFloater<CommWrapper>					("im");
		registerFloater<LLFloaterInspect>				("inspect");
		registerFloater<LLFloaterJoystick>				("joystick");
		registerFloater<LLFloaterMediaFilter>			("media filter");
		registerFloater<LLFloaterMap>					("mini map");
		registerFloater<LLFloaterMarketplaceListings>	("marketplace_listings");
		registerFloater<LLFloaterMute>					("mute list");
		registerFloater<LLFloaterNoTranslate>			("no translate");
		registerFloater<LLFloaterNotificationConsole>	("notifications console");
		registerFloater<LLFloaterPathfindingCharacters>	("pathfinding_characters");
		registerFloater<LLFloaterPathfindingLinksets>	("pathfinding_linksets");
		registerFloater<LLFloaterPermsDefault>			("perm prefs");
		registerFloater<LLFloaterPostProcess>			("PostProcess");
		registerFloater<LLFloaterAvatarList>			("radar");
		registerFloater<ALFloaterRegionTracker>			("region_tracker");
		registerFloater<LLFloaterScriptLimits>			("script info");
		registerFloater<LLFloaterStats>					("stat bar");
		registerFloater<LLFloaterTeleportHistory>		("teleport history");
		registerFloater<LLFloaterToolbarPrefs>			("floater_toolbar_prefs.xml");
		registerFloater<LLFloaterVoiceEffect>			("voice effect");
		registerFloater<RlvFloaterBehaviours>("rlv restrictions");
		registerFloater<RlvFloaterLocks>("rlv locks");
		registerFloater<RlvFloaterStrings>("rlv strings");
	}
public:
	template <typename T>
	void registerConsole(const std::string& name, T* console)
	{
		registerFloater(name, boost::bind(&T::setVisible, console, !boost::bind(&T::getVisible, console)), boost::bind(&T::getVisible, console));
	}
	void registerFloater(const std::string& name, std::function<void ()> show, std::function<bool ()> visible = nullptr)
	{
		mEntries.insert( std::make_pair( name, std::make_pair( show, visible ) ) );
	}
	template <typename T>
	void registerFloater(const std::string& name, const LLSD& key = LLSD())
	{
		registerFloater(name, boost::bind(&T::toggleInstance,key), boost::bind(&T::instanceVisible,key));
	}
	void registerWindow(const std::string& name, std::function<void()> show, std::function<LLFloater*()> get)
	{
		registerFloater(name,
			[show, get]() {
				if (LLFloater* floater = get())
				{
					if (floater->getVisible())
					{
						floater->close();
						return;
					}
				}
				show();
			},
			[get]() {
				LLFloater* floater = get();
				return floater && floater->getVisible();
			});
	}
	void registerWindow(const std::string& name, std::function<void()> show, const std::string& xml_name)
	{
		registerWindow(name, show, [xml_name]() -> LLFloater* {
			return LLUICtrlFactory::getInstance()->getBuiltFloater(xml_name);
		});
	}
};
void show_floater(const std::string& floater_name)
{
	if (floater_name.empty()) return;
	MenuFloaterDict::menu_floater_map_t::iterator it = MenuFloaterDict::instance().mEntries.find(floater_name);
	if (it == MenuFloaterDict::instance().mEntries.end())
	{
		if (LLFloater* floater = LLUICtrlFactory::getInstance()->getBuiltFloater(floater_name))
			floater->getVisible() ? floater->close() : gFloaterView->bringToFront(floater);
		else
			LLUICtrlFactory::getInstance()->buildFloater(new LLFloater(), floater_name);
	}
	else if (it->second.first)
	{
		it->second.first();
	}
}
bool floater_visible(const std::string& floater_name)
{
	MenuFloaterDict::menu_floater_map_t::iterator it = MenuFloaterDict::instance().mEntries.find(floater_name);
	return it != MenuFloaterDict::instance().mEntries.end() && it->second.second && it->second.second();
}
