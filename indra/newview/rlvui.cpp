/** 
 *
 * Copyright (c) 2009-2011, Kitty Barnett
 * 
 * The source code in this file is provided to you under the terms of the 
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt 
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 * 
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to 
 * abide by those obligations.
 * 
 */
#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llenvmanager.h"
#include "llhudtext.h"
#include "llimview.h"
#include "llmoveview.h"
#include "llparcel.h"
#include "lltabcontainer.h"
#include "lltoolmgr.h"
#include "llviewerparcelmgr.h"
#include "llvoavatar.h"
#include "roles_constants.h"
#include "lffloaterinvpanel.h"
#include "llfloaterbeacons.h"
#include "llfloatertools.h"
#include "llfloaterenvsettings.h"
#include "llfloaterwindlight.h"
#include "llfloaterwater.h"
#include "llfloaterdaycycle.h"
#include "llagentcamera.h"
#include "llviewerwindow.h"
#include "llpanelmaininventory.h"
#include "llappviewer.h"
#include "llfloaterland.h"
#include "llfloatergodtools.h"
#include "llfloaterregioninfo.h"
#include "llfloatermap.h"
#include "llfloaterchat.h"
#include "llfloateravatarlist.h"
#include "llfloaterworldmap.h"
#include "llmenugl.h"
#include "lltoolbar.h"
#include "llnavigationbar.h"
#include "lluictrlfactory.h"
#include "llviewerregion.h"
#include "rlvui.h"
#include "rlvhandler.h"
#include "rlvextensions.h"
RlvUIEnabler::RlvUIEnabler()
{
	gRlvHandler.setBehaviourToggleCallback(boost::bind(&RlvUIEnabler::onBehaviourToggle, this, _1, _2));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWLOC, boost::bind(&RlvUIEnabler::onRefreshHoverText, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWNAMES, boost::bind(&RlvUIEnabler::onRefreshHoverText, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWNAMETAGS, boost::bind(&RlvUIEnabler::onRefreshHoverText, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWHOVERTEXTALL, boost::bind(&RlvUIEnabler::onRefreshHoverText, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWHOVERTEXTWORLD, boost::bind(&RlvUIEnabler::onRefreshHoverText, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWHOVERTEXTHUD, boost::bind(&RlvUIEnabler::onRefreshHoverText, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_FLY, boost::bind(&RlvUIEnabler::onToggleMovement, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_ALWAYSRUN, boost::bind(&RlvUIEnabler::onToggleMovement, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_TEMPRUN, boost::bind(&RlvUIEnabler::onToggleMovement, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_VIEWNOTE, boost::bind(&RlvUIEnabler::onToggleViewXXX, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_VIEWSCRIPT, boost::bind(&RlvUIEnabler::onToggleViewXXX, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_VIEWTEXTURE, boost::bind(&RlvUIEnabler::onToggleViewXXX, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_EDIT, boost::bind(&RlvUIEnabler::onToggleEdit, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SENDIM, boost::bind(&RlvUIEnabler::onToggleSendIM, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SETDEBUG, boost::bind(&RlvUIEnabler::onToggleSetDebug, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SETENV, boost::bind(&RlvUIEnabler::onToggleSetEnv, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWINV, boost::bind(&RlvUIEnabler::onToggleShowInv, this, _1)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWLOC, boost::bind(&RlvUIEnabler::onToggleShowLoc, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWMINIMAP, boost::bind(&RlvUIEnabler::onToggleShowMinimap, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWNAMES, boost::bind(&RlvUIEnabler::onToggleShowNames, this, _1)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWNAMETAGS, boost::bind(&RlvUIEnabler::onToggleShowNameTags, this, _1)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_SHOWWORLDMAP, boost::bind(&RlvUIEnabler::onToggleShowWorldMap, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_UNSIT, boost::bind(&RlvUIEnabler::onToggleUnsit, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_CAMUNLOCK, boost::bind(&RlvUIEnabler::onToggleCamUnlock, this)));
	void onToggleCamZoom(const ERlvBehaviour& eBhvr);
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_CAMZOOMMAX, boost::bind(onToggleCamZoom, RLV_BHVR_CAMZOOMMAX)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_CAMZOOMMIN, boost::bind(onToggleCamZoom, RLV_BHVR_CAMZOOMMIN)));
	void onToggleCamDist(const ERlvBehaviour& eBhvr);
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_CAMDISTMAX, boost::bind(onToggleCamDist, RLV_BHVR_CAMDISTMAX)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_CAMDISTMIN, boost::bind(onToggleCamDist, RLV_BHVR_CAMDISTMIN)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_TPLOC, boost::bind(&RlvUIEnabler::onToggleTp, this)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_TPLM, boost::bind(&RlvUIEnabler::onToggleTp, this)));
	#ifdef RLV_EXTENSION_STARTLOCATION
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_TPLOC, boost::bind(&RlvUIEnabler::onUpdateLoginLastLocation, this, _1)));
	m_Handlers.insert(std::pair<ERlvBehaviour, behaviour_handler_t>(RLV_BHVR_UNSIT, boost::bind(&RlvUIEnabler::onUpdateLoginLastLocation, this, _1)));
	#endif
}
void RlvUIEnabler::onBehaviourToggle(ERlvBehaviour eBhvr, ERlvParamType eType)
{
	bool fQuitting = LLApp::isQuitting();
	for (behaviour_handler_map_t::const_iterator itHandler = m_Handlers.lower_bound(eBhvr), endHandler = m_Handlers.upper_bound(eBhvr);
			itHandler != endHandler; ++itHandler)
	{
		itHandler->second(fQuitting);
	}
}
void RlvUIEnabler::onRefreshHoverText()
{
	LLHUDText::refreshAllObjectText();
}
void RlvUIEnabler::onToggleEdit()
{
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_EDIT);
	if (!fEnable)
	{
		LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
		if (LLFloaterBeacons::instanceVisible())
			LLFloaterBeacons::toggleInstance();
		if (gFloaterTools->getVisible())
		{
			gAgentCamera.resetView(FALSE);
			gFloaterTools->close();
			gViewerWindow->showCursor();
		}
	}
}
void RlvUIEnabler::onToggleMovement()
{
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_FLY)) && (gAgent.getFlying()) )
		gAgent.setFlying(FALSE);
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_ALWAYSRUN)) && (gAgent.getAlwaysRun()) )
		gAgent.clearAlwaysRun();
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_TEMPRUN)) && (gAgent.getTempRun()) )
		gAgent.clearTempRun();
}
void RlvUIEnabler::onToggleSendIM()
{
}
void RlvUIEnabler::onToggleSetDebug()
{
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_SETDEBUG);
	for (std::map<std::string, S16>::const_iterator itSetting = RlvExtGetSet::m_DbgAllowed.begin();
			itSetting != RlvExtGetSet::m_DbgAllowed.end(); ++itSetting)
	{
		if (itSetting->second & RlvExtGetSet::DBG_WRITE)
			gSavedSettings.getControl(itSetting->first)->setHiddenFromSettingsEditor(!fEnable);
	}
}
void RlvUIEnabler::onToggleSetEnv()
{
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_SETENV);
	if (!fEnable)
	{
		if ( (LLFloaterEnvSettings::isOpen()) )
			LLFloaterEnvSettings::instance()->close();
		if ( (LLFloaterWindLight::isOpen()) )
			LLFloaterWindLight::instance()->close();
		if ( (LLFloaterWater::isOpen()) )
			LLFloaterWater::instance()->close();
		if ( (LLFloaterDayCycle::isOpen()) )
			LLFloaterDayCycle::instance()->close();
	}
	gSavedSettings.getControl("VertexShaderEnable")->setHiddenFromSettingsEditor(!fEnable);
	gSavedSettings.getControl("WindLightUseAtmosShaders")->setHiddenFromSettingsEditor(!fEnable);
	if (fEnable)
		LLEnvManagerNew::instance().usePrefs();
}
void RlvUIEnabler::onToggleShowInv(bool fQuitting)
{
	if (fQuitting)
		return;
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWINV);
	if (!fEnable)
	{
		LLPanelMainInventory::closeAll();
		LFFloaterInvPanel::closeAll();
		if (LLFloater* floater = LLUICtrlFactory::getInstance()->getBuiltFloater("floater_inventory_favs.xml"))
			floater->close();
	}
	LLFloater* pAppearancePanel = LLUICtrlFactory::getInstance()->getBuiltFloater("floater_my_outfits.xml");
	if (pAppearancePanel)
	{
		if (!fEnable) pAppearancePanel->close();
	}
	if (!fEnable)
	{
		LLMenuGL::sMenuContainer->childSetEnabled("My Outfits", false);
		LLMenuGL::sMenuContainer->childSetEnabled("Favorites", false);
	}
	else
	{
		LLMenuGL::sMenuContainer->childSetEnabled("My Outfits", true);
		LLMenuGL::sMenuContainer->childSetEnabled("Favorites", true);
	}
	gToolBar->childSetEnabled("outfits_btn", fEnable);
	gToolBar->childSetEnabled("favs_btn", fEnable);
}
void RlvUIEnabler::onToggleShowLoc()
{
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC);
	if (gNavigationBar)
	{
		gNavigationBar->refreshLocation();
	}
	if (!fEnable)
	{
		if (LLFloaterLand::instanceVisible())
			LLFloaterLand::hideInstance();
		if (LLFloaterRegionInfo::instanceVisible())
			LLFloaterRegionInfo::hideInstance();
		LLFloaterGodTools::hide();
	}
}
void RlvUIEnabler::onToggleShowMinimap()
{
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWMINIMAP);
	if ((!fEnable) && LLFloaterMap::instanceVisible())
		LLFloaterMap::hideInstance();
}
void RlvUIEnabler::onToggleShowNames(bool fQuitting)
{
	if (fQuitting)
		return;
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES);
	if (!fEnable)
	{
		LLFloaterChat::getInstance()->childSetVisible("active_speakers_panel", false);
		LLAvatarNameCache::setForceDisplayNames(true);
	}
	else
	{
		LLAvatarNameCache::setForceDisplayNames(false);
		const S32 namesys = gSavedSettings.getS32("PhoenixNameSystem");
		LLAvatarNameCache::setUseDisplayNames(namesys > 0 && namesys < 4);
	}
	if (LLFloaterAvatarList::instanceExists())
		LLFloaterAvatarList::instance().resetAvatarNames();
	LLVOAvatar::invalidateNameTags();
}
void RlvUIEnabler::onToggleShowNameTags(bool fQuitting)
{
	if (fQuitting) return;
	if (LLFloaterAvatarList::instanceExists())
		LLFloaterAvatarList::instance().resetAvatarNames();
	LLVOAvatar::invalidateNameTags();
}
void RlvUIEnabler::onToggleShowWorldMap()
{
	bool fEnable = !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWWORLDMAP);
	if ((!fEnable) && gFloaterWorldMap->getVisible())
		LLFloaterWorldMap::toggle();
}
void RlvUIEnabler::onToggleCamUnlock()
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMUNLOCK))
		gAgentCamera.resetView(true, true);
}
void onToggleCamZoom(const ERlvBehaviour& eBhvr)
{
	if (gRlvHandler.hasBehaviour(eBhvr))
	{
		LLViewerCamera& inst(LLViewerCamera::instance());
		inst.mSavedFOVLoaded ? inst.loadDefaultFOV() : inst.setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
	}
}
void onToggleCamDist(const ERlvBehaviour& eBhvr)
{
	if (gRlvHandler.hasBehaviour(eBhvr))
	{
		if (eBhvr == RLV_BHVR_CAMDISTMAX && gRlvHandler.camPole(eBhvr) <= 0)
		{
			if (!gAgentCamera.cameraMouselook())
				gAgentCamera.changeCameraToMouselook();
		}
		else
		{
			if (eBhvr == RLV_BHVR_CAMDISTMIN && gRlvHandler.camPole(eBhvr) > 0 && gAgentCamera.cameraMouselook())
				gAgentCamera.changeCameraToDefault();
			gAgentCamera.cameraPanUp(0);
		}
	}
}
void RlvUIEnabler::onToggleTp()
{
	if (gNavigationBar)
	{
		gNavigationBar->refreshHomeButton();
	}
}
void RlvUIEnabler::onToggleUnsit()
{
}
void RlvUIEnabler::onToggleViewXXX()
{
}
void RlvUIEnabler::onUpdateLoginLastLocation(bool fQuitting)
{
	if (!fQuitting)
		RlvSettings::updateLoginLastLocation();
}
bool RlvUIEnabler::canViewParcelProperties()
{
	bool fShow = !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC);
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
	{
		const LLParcel* pParcel = NULL;
		if (LLViewerParcelMgr::getInstance()->selectionEmpty())
		{
			pParcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		}
		else
		{
			LLParcelSelection* pParcelSel = LLViewerParcelMgr::getInstance()->getFloatingParcelSelection();
			if (pParcelSel->hasOthersSelected())
				return false;
			pParcel = pParcelSel->getParcel();
		}
		if (pParcel)
		{
			const LLUUID& idOwner = pParcel->getOwnerID();
			if ( (idOwner != gAgent.getID()) )
			{
				S32 count = gAgent.mGroups.size();
				for (S32 i = 0; i < count; ++i)
				{
					if (gAgent.mGroups[i].mID == idOwner)
					{
						fShow = ((gAgent.mGroups[i].mPowers & GP_LAND_RETURN) > 0);
						break;
					}
				}
			}
			else
			{
				fShow = true;
			}
		}
	}
	return fShow;
}
bool RlvUIEnabler::canViewRegionProperties()
{
	bool fShow = !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC);
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
	{
		const LLViewerRegion* pRegion = gAgent.getRegion();
		if (pRegion)
			fShow = (pRegion->isEstateManager()) || (pRegion->getOwner() == gAgent.getID());
	}
	return fShow;
}
bool RlvUIEnabler::hasOpenIM(const LLUUID& idAgent)
{
	LLUUID idSession = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, idAgent);
	return gIMMgr->hasSession(idSession);
}
bool RlvUIEnabler::hasOpenProfile(const LLUUID& idAgent)
{
	return LLAvatarActions::profileVisible(idAgent);
}
bool RlvUIEnabler::isBuildEnabled()
{
	return (gAgent.canEditParcel()) && ((!gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)) || (!gRlvHandler.hasBehaviour(RLV_BHVR_REZ)));
}
