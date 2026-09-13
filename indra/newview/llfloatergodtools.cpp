/** 
 * @file llfloatergodtools.cpp
 * @brief The on-screen rectangle with tool options.
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
#include "llviewerprecompiledheaders.h"
#include "llfloatergodtools.h"
#include "llavatarnamecache.h"
#include "llcoord.h"
#include "llfontgl.h"
#include "llframetimer.h"
#include "llgl.h"
#include "llhost.h"
#include "llnotificationsutil.h"
#include "llregionflags.h"
#include "llstring.h"
#include "message.h"
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldraghandle.h"
#include "llfloater.h"
#include "llfocusmgr.h"
#include "llfloatertopobjects.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llspinctrl.h"
#include "llstatusbar.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lluictrl.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llfloateravatarpicker.h"
#include "llxfermanager.h"
#include "llvlcomposition.h"
#include "llsurface.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "lltrans.h"
#include "lltransfertargetfile.h"
#include "lltransfersourcefile.h"
#include "hippogridmanager.h"
const F32 SECONDS_BETWEEN_UPDATE_REQUESTS = 5.0f;
void LLFloaterGodTools::onOpen()
{
	center();
	setFocus(TRUE);
	LLPanel *panel = childGetVisibleTab("GodTools Tabs");
	if (panel) panel->setFocus(TRUE);
	if (mPanelObjectTools)
		mPanelObjectTools->setTargetAvatar(LLUUID::null);
	if (gAgent.getRegionHost() != mCurrentHost)
	{
		sendRegionInfoRequest();
	}
}
void LLFloaterGodTools::refreshAll()
{
	LLFloaterGodTools* god_tools = instanceExists() ? getInstance() : NULL;
	if (god_tools)
	{
		if (gAgent.getRegionHost() != god_tools->mCurrentHost)
		{
			god_tools->sendRegionInfoRequest();
		}
	}
}
LLFloaterGodTools::LLFloaterGodTools()
:	LLFloater(std::string("godtools floater")),
	mPanelRegionTools(nullptr),
	mPanelObjectTools(nullptr),
	mCurrentHost(LLHost::invalid),
	mUpdateTimer()
{
	mFactoryMap["grid"] = LLCallbackMap(createPanelGrid, this);
	mFactoryMap["region"] = LLCallbackMap(createPanelRegion, this);
	mFactoryMap["objects"] = LLCallbackMap(createPanelObjects, this);
	mFactoryMap["request"] = LLCallbackMap(createPanelRequest, this);
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_god_tools.xml", &getFactoryMap());
}
BOOL LLFloaterGodTools::postBuild()
{
	sendRegionInfoRequest();
	getChild<LLTabContainer>("GodTools Tabs")->selectTabByName("region");
	getChild<LLUICtrl>("land cost text")->setTextArg("[CURRENCY]", gHippoGridManager->getConnectedGrid()->getCurrencySymbol());
	return TRUE;
}
void* LLFloaterGodTools::createPanelGrid(void *userdata)
{
	return new LLPanelGridTools();
}
void* LLFloaterGodTools::createPanelRegion(void *userdata)
{
	LLFloaterGodTools* self = (LLFloaterGodTools*)userdata;
	self->mPanelRegionTools = new LLPanelRegionTools();
	return self->mPanelRegionTools;
}
void* LLFloaterGodTools::createPanelObjects(void *userdata)
{
	LLFloaterGodTools* self = (LLFloaterGodTools*)userdata;
	self->mPanelObjectTools = new LLPanelObjectTools();
	return self->mPanelObjectTools;
}
void* LLFloaterGodTools::createPanelRequest(void *userdata)
{
	return new LLPanelRequestTools();
}
LLFloaterGodTools::~LLFloaterGodTools()
{
}
U64 LLFloaterGodTools::computeRegionFlags() const
{
	U64 flags = gAgent.getRegion()->getRegionFlags();
	if (mPanelRegionTools) flags = mPanelRegionTools->computeRegionFlags(flags);
	if (mPanelObjectTools) flags = mPanelObjectTools->computeRegionFlags(flags);
	return flags;
}
void LLFloaterGodTools::updatePopup(LLCoordGL center, MASK mask)
{
}
void LLFloaterGodTools::draw()
{
	if (mCurrentHost == LLHost::invalid)
	{
		if (mUpdateTimer.getElapsedTimeF32() > SECONDS_BETWEEN_UPDATE_REQUESTS)
		{
			sendRegionInfoRequest();
		}
	}
	else if (gAgent.getRegionHost() != mCurrentHost)
	{
		sendRegionInfoRequest();
	}
	LLFloater::draw();
}
void LLFloaterGodTools::show(void *)
{
	getInstance()->open();
}
void LLFloaterGodTools::hide()
{
	if(instanceExists())
		getInstance()->close();
}
void LLFloaterGodTools::showPanel(const std::string& panel_name)
{
	getChild<LLTabContainer>("GodTools Tabs")->selectTabByName(panel_name);
	open();
	LLPanel *panel = getChild<LLTabContainer>("GodTools Tabs")->getCurrentPanel();
	if (panel)
		panel->setFocus(TRUE);
}
void LLFloaterGodTools::processRegionInfo(LLMessageSystem* msg)
{
	llassert(msg);
	if (!msg) return;
	U64 region_flags;
	U8 sim_access;
	U8 agent_limit;
	std::string sim_name;
	U32 estate_id;
	U32 parent_estate_id;
	F32 water_height;
	F32 billable_factor;
	F32 object_bonus_factor;
	F32 terrain_raise_limit;
	F32 terrain_lower_limit;
	S32 price_per_meter;
	S32 redirect_grid_x;
	S32 redirect_grid_y;
	LLUUID cache_id;
	LLHost host = msg->getSender();
	msg->getStringFast(_PREHASH_RegionInfo, _PREHASH_SimName, sim_name);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_EstateID, estate_id);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_ParentEstateID, parent_estate_id);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_SimAccess, sim_access);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_MaxAgents, agent_limit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_ObjectBonusFactor, object_bonus_factor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_BillableFactor, billable_factor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_WaterHeight, water_height);
	if (msg->has(_PREHASH_RegionInfo3))
	{
		msg->getU64Fast(_PREHASH_RegionInfo3, _PREHASH_RegionFlagsExtended, region_flags);
	}
	else
	{
		U32 flags = 0;
		msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_RegionFlags, flags);
		region_flags = flags;
	}
	if (host != gAgent.getRegionHost())
	{
		LLWorld::getInstance()->waterHeightRegionInfo(sim_name, water_height);
		return;
	}
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainRaiseLimit, terrain_raise_limit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainLowerLimit, terrain_lower_limit);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_PricePerMeter, price_per_meter);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridX, redirect_grid_x);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridY, redirect_grid_y);
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->setRegionNameAndZone(sim_name);
		regionp->setRegionFlags(region_flags);
		regionp->setSimAccess(sim_access);
		regionp->setWaterHeight(water_height);
		regionp->setBillableFactor(billable_factor);
	}
	LLFloaterGodTools* god_tools = LLFloaterGodTools::instanceExists() ? LLFloaterGodTools::getInstance() : NULL;
	if (!god_tools) return;
	if ( gAgent.isGodlike()
		&& god_tools->mPanelRegionTools
		&& god_tools->mPanelObjectTools)
	{
		LLPanelRegionTools* rtool = god_tools->mPanelRegionTools;
		god_tools->mCurrentHost = host;
		rtool->setSimName(sim_name);
		rtool->setEstateID(estate_id);
		rtool->setParentEstateID(parent_estate_id);
		rtool->setCheckFlags(region_flags);
		rtool->setBillableFactor(billable_factor);
		rtool->setPricePerMeter(price_per_meter);
		rtool->setRedirectGridX(redirect_grid_x);
		rtool->setRedirectGridY(redirect_grid_y);
		rtool->enableAllWidgets();
		LLPanelObjectTools *otool = god_tools->mPanelObjectTools;
		otool->setCheckFlags(region_flags);
		otool->enableAllWidgets();
		LLViewerRegion *regionp = gAgent.getRegion();
		if ( !regionp )
		{
			rtool->setGridPosX(-1);
			rtool->setGridPosY(-1);
		}
		else
		{
			LLVector3d global_pos = regionp->getPosGlobalFromRegion(LLVector3::zero);
			S32 grid_pos_x = (S32) (global_pos.mdV[VX] / 256.0f);
			S32 grid_pos_y = (S32) (global_pos.mdV[VY] / 256.0f);
			rtool->setGridPosX(grid_pos_x);
			rtool->setGridPosY(grid_pos_y);
		}
	}
}
void LLFloaterGodTools::sendRegionInfoRequest()
{
	if (mPanelRegionTools) mPanelRegionTools->clearAllWidgets();
	if (mPanelObjectTools) mPanelObjectTools->clearAllWidgets();
	mCurrentHost = LLHost::invalid;
	mUpdateTimer.reset();
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("RequestRegionInfo");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	gAgent.sendReliableMessage();
}
void LLFloaterGodTools::sendGodUpdateRegionInfo()
{
	LLFloaterGodTools* god_tools = LLFloaterGodTools::instanceExists() ? LLFloaterGodTools::getInstance() : NULL;
	if (!god_tools) return;
	LLViewerRegion *regionp = gAgent.getRegion();
	if (gAgent.isGodlike()
		&& god_tools->mPanelRegionTools
		&& regionp
		&& gAgent.getRegionHost() == mCurrentHost)
	{
		LLMessageSystem *msg = gMessageSystem;
		LLPanelRegionTools *rtool = god_tools->mPanelRegionTools;
		U64 region_flags = computeRegionFlags();
		msg->newMessage("GodUpdateRegionInfo");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_RegionInfo);
		msg->addStringFast(_PREHASH_SimName, rtool->getSimName());
		msg->addU32Fast(_PREHASH_EstateID, rtool->getEstateID());
		msg->addU32Fast(_PREHASH_ParentEstateID, rtool->getParentEstateID());
		msg->addU32Fast(_PREHASH_RegionFlags, U32(region_flags));
		msg->addF32Fast(_PREHASH_BillableFactor, rtool->getBillableFactor());
		msg->addS32Fast(_PREHASH_PricePerMeter, rtool->getPricePerMeter());
		msg->addS32Fast(_PREHASH_RedirectGridX, rtool->getRedirectGridX());
		msg->addS32Fast(_PREHASH_RedirectGridY, rtool->getRedirectGridY());
		msg->nextBlockFast(_PREHASH_RegionInfo2);
		msg->addU64Fast(_PREHASH_RegionFlagsExtended, region_flags);
		gAgent.sendReliableMessage();
	}
}
const F32 BILLABLE_FACTOR_DEFAULT = 1;
const F32 BILLABLE_FACTOR_MIN = 0.0f;
const F32 BILLABLE_FACTOR_MAX = 4.f;
const F32 PRICE_PER_METER_DEFAULT = 1.f;
const F32 PRICE_PER_METER_MIN = 0.f;
const F32 PRICE_PER_METER_MAX = 100.f;
LLPanelRegionTools::LLPanelRegionTools()
: 	LLPanel()
{
	mCommitCallbackRegistrar.add("RegionTools.ChangeAnything",	boost::bind(&LLPanelRegionTools::onChangeAnything, this));
	mCommitCallbackRegistrar.add("RegionTools.ChangePrelude",	boost::bind(&LLPanelRegionTools::onChangePrelude, this));
	mCommitCallbackRegistrar.add("RegionTools.BakeTerrain",		boost::bind(&LLPanelRegionTools::onBakeTerrain, this));
	mCommitCallbackRegistrar.add("RegionTools.RevertTerrain",	boost::bind(&LLPanelRegionTools::onRevertTerrain, this));
	mCommitCallbackRegistrar.add("RegionTools.SwapTerrain",		boost::bind(&LLPanelRegionTools::onSwapTerrain, this));
	mCommitCallbackRegistrar.add("RegionTools.Refresh",			boost::bind(&LLPanelRegionTools::onRefresh, this));
	mCommitCallbackRegistrar.add("RegionTools.ApplyChanges",	boost::bind(&LLPanelRegionTools::onApplyChanges, this));
	mCommitCallbackRegistrar.add("RegionTools.SelectRegion",	boost::bind(&LLPanelRegionTools::onSelectRegion, this));
	mCommitCallbackRegistrar.add("RegionTools.SaveState",		boost::bind(&LLPanelRegionTools::onSaveState, this));
}
BOOL LLPanelRegionTools::postBuild()
{
	getChild<LLLineEditor>("region name")->setKeystrokeCallback(boost::bind(&LLPanelRegionTools::onChangeSimName, this));
	getChild<LLLineEditor>("region name")->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	getChild<LLLineEditor>("estate")->setPrevalidate(&LLLineEditor::prevalidatePositiveS32);
	getChild<LLLineEditor>("parentestate")->setPrevalidate(&LLLineEditor::prevalidatePositiveS32);
	getChildView("parentestate")->setEnabled(FALSE);
	getChild<LLLineEditor>("gridposx")->setPrevalidate(&LLLineEditor::prevalidatePositiveS32);
	getChildView("gridposx")->setEnabled(FALSE);
	getChild<LLLineEditor>("gridposy")->setPrevalidate(&LLLineEditor::prevalidatePositiveS32);
	getChildView("gridposy")->setEnabled(FALSE);
	getChild<LLLineEditor>("redirectx")->setPrevalidate(&LLLineEditor::prevalidatePositiveS32);
	getChild<LLLineEditor>("redirecty")->setPrevalidate(&LLLineEditor::prevalidatePositiveS32);
	getChild<LLUICtrl>("is gaming")->setVisible((gAgent.getRegion()->getGamingFlags() & REGION_GAMING_PRESENT) && !(gAgent.getRegion()->getGamingFlags() & REGION_GAMING_HIDE_GOD_FLOATER));
	getChild<LLUICtrl>("hide from search")->setVisible(!gHippoGridManager->getConnectedGrid()->isSecondLife());
	return TRUE;
}
LLPanelRegionTools::~LLPanelRegionTools()
{
}
U64 LLPanelRegionTools::computeRegionFlags(U64 flags) const
{
	flags &= getRegionFlagsMask();
	flags |= getRegionFlags();
	return flags;
}
void LLPanelRegionTools::refresh()
{
}
void LLPanelRegionTools::clearAllWidgets()
{
	getChild<LLUICtrl>("region name")->setValue("unknown");
	getChild<LLUICtrl>("region name")->setFocus( FALSE);
	getChild<LLUICtrl>("check prelude")->setValue(FALSE);
	getChildView("check prelude")->setEnabled(FALSE);
	getChild<LLUICtrl>("check fixed sun")->setValue(FALSE);
	getChildView("check fixed sun")->setEnabled(FALSE);
	getChild<LLUICtrl>("check reset home")->setValue(FALSE);
	getChildView("check reset home")->setEnabled(FALSE);
	getChild<LLUICtrl>("check damage")->setValue(FALSE);
	getChildView("check damage")->setEnabled(FALSE);
	getChild<LLUICtrl>("check visible")->setValue(FALSE);
	getChildView("check visible")->setEnabled(FALSE);
	getChild<LLUICtrl>("block terraform")->setValue(FALSE);
	getChildView("block terraform")->setEnabled(FALSE);
	getChild<LLUICtrl>("block dwell")->setValue(FALSE);
	getChildView("block dwell")->setEnabled(FALSE);
	getChild<LLUICtrl>("is sandbox")->setValue(FALSE);
	getChildView("is sandbox")->setEnabled(FALSE);
	getChild<LLUICtrl>("billable factor")->setValue(BILLABLE_FACTOR_DEFAULT);
	getChildView("billable factor")->setEnabled(FALSE);
	getChild<LLUICtrl>("land cost")->setValue(PRICE_PER_METER_DEFAULT);
	getChildView("land cost")->setEnabled(FALSE);
	getChildView("Apply")->setEnabled(FALSE);
	getChildView("Bake Terrain")->setEnabled(FALSE);
	getChildView("Autosave now")->setEnabled(FALSE);
}
void LLPanelRegionTools::enableAllWidgets()
{
	getChildView("check prelude")->setEnabled(TRUE);
	getChildView("check fixed sun")->setEnabled(TRUE);
	getChildView("check reset home")->setEnabled(TRUE);
	getChildView("check damage")->setEnabled(TRUE);
	getChildView("check visible")->setEnabled(FALSE);
	getChildView("block terraform")->setEnabled(TRUE);
	getChildView("block dwell")->setEnabled(TRUE);
	getChildView("is sandbox")->setEnabled(TRUE);
	getChildView("billable factor")->setEnabled(TRUE);
	getChildView("land cost")->setEnabled(TRUE);
	getChildView("Apply")->setEnabled(FALSE);
	getChildView("Bake Terrain")->setEnabled(TRUE);
	getChildView("Autosave now")->setEnabled(TRUE);
}
void LLPanelRegionTools::onSaveState(void* userdata)
{
	if (gAgent.isGodlike())
	{
		gMessageSystem->newMessageFast(_PREHASH_StateSave);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_DataBlock);
		gMessageSystem->addStringFast(_PREHASH_Filename, nullptr);
		gAgent.sendReliableMessage();
	}
}
const std::string LLPanelRegionTools::getSimName() const
{
	return getChild<LLUICtrl>("region name")->getValue();
}
U32 LLPanelRegionTools::getEstateID() const
{
	U32 id = (U32)getChild<LLUICtrl>("estate")->getValue().asInteger();
	return id;
}
U32 LLPanelRegionTools::getParentEstateID() const
{
	U32 id = (U32)getChild<LLUICtrl>("parentestate")->getValue().asInteger();
	return id;
}
S32 LLPanelRegionTools::getRedirectGridX() const
{
	return getChild<LLUICtrl>("redirectx")->getValue().asInteger();
}
S32 LLPanelRegionTools::getRedirectGridY() const
{
	return getChild<LLUICtrl>("redirecty")->getValue().asInteger();
}
S32 LLPanelRegionTools::getGridPosX() const
{
	return getChild<LLUICtrl>("gridposx")->getValue().asInteger();
}
S32 LLPanelRegionTools::getGridPosY() const
{
	return getChild<LLUICtrl>("gridposy")->getValue().asInteger();
}
U64 LLPanelRegionTools::getRegionFlags() const
{
	U64 flags = 0x0;
	flags = getChild<LLUICtrl>("check prelude")->getValue().asBoolean()
					? set_prelude_flags(flags)
					: unset_prelude_flags(flags);
	if (getChild<LLUICtrl>("check fixed sun")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SUN_FIXED;
	}
	if (getChild<LLUICtrl>("check reset home")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_RESET_HOME_ON_TELEPORT;
	}
	if (getChild<LLUICtrl>("check visible")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_EXTERNALLY_VISIBLE;
	}
	if (getChild<LLUICtrl>("check damage")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_DAMAGE;
	}
	if (getChild<LLUICtrl>("block terraform")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_BLOCK_TERRAFORM;
	}
	if (getChild<LLUICtrl>("block dwell")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_BLOCK_DWELL;
	}
	if (getChild<LLUICtrl>("is sandbox")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SANDBOX;
	}
	if (getChild<LLUICtrl>("is gaming")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_GAMING;
	}
	if (getChild<LLUICtrl>("hide from search")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_HIDE_FROM_SEARCH;
	}
	return flags;
}
U64 LLPanelRegionTools::getRegionFlagsMask() const
{
	U64 flags = 0xFFFFFFFFFFFFFFFFULL;
	flags = getChild<LLUICtrl>("check prelude")->getValue().asBoolean()
				? set_prelude_flags(flags)
				: unset_prelude_flags(flags);
	if (!getChild<LLUICtrl>("check fixed sun")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_SUN_FIXED;
	}
	if (!getChild<LLUICtrl>("check reset home")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_RESET_HOME_ON_TELEPORT;
	}
	if (!getChild<LLUICtrl>("check visible")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_EXTERNALLY_VISIBLE;
	}
	if (!getChild<LLUICtrl>("check damage")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_ALLOW_DAMAGE;
	}
	if (!getChild<LLUICtrl>("block terraform")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_BLOCK_TERRAFORM;
	}
	if (!getChild<LLUICtrl>("block dwell")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_BLOCK_DWELL;
	}
	if (!getChild<LLUICtrl>("is sandbox")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_SANDBOX;
	}
	if (!getChild<LLUICtrl>("is gaming")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_GAMING;
	}
	if (!getChild<LLUICtrl>("hide from search")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_HIDE_FROM_SEARCH;
	}
	return flags;
}
F32 LLPanelRegionTools::getBillableFactor() const
{
	return (F32)getChild<LLUICtrl>("billable factor")->getValue().asReal();
}
S32 LLPanelRegionTools::getPricePerMeter() const
{
	return getChild<LLUICtrl>("land cost")->getValue();
}
void LLPanelRegionTools::setSimName(const std::string& name)
{
	getChild<LLUICtrl>("region name")->setValue(name);
}
void LLPanelRegionTools::setEstateID(U32 id)
{
	getChild<LLUICtrl>("estate")->setValue((S32)id);
}
void LLPanelRegionTools::setGridPosX(S32 pos)
{
	getChild<LLUICtrl>("gridposx")->setValue(pos);
}
void LLPanelRegionTools::setGridPosY(S32 pos)
{
	getChild<LLUICtrl>("gridposy")->setValue(pos);
}
void LLPanelRegionTools::setRedirectGridX(S32 pos)
{
	getChild<LLUICtrl>("redirectx")->setValue(pos);
}
void LLPanelRegionTools::setRedirectGridY(S32 pos)
{
	getChild<LLUICtrl>("redirecty")->setValue(pos);
}
void LLPanelRegionTools::setParentEstateID(U32 id)
{
	getChild<LLUICtrl>("parentestate")->setValue((S32)id);
}
void LLPanelRegionTools::setCheckFlags(U64 flags)
{
	getChild<LLUICtrl>("check prelude")->setValue(is_prelude(flags) ? TRUE : FALSE);
	getChild<LLUICtrl>("check fixed sun")->setValue(flags & REGION_FLAGS_SUN_FIXED ? TRUE : FALSE);
	getChild<LLUICtrl>("check reset home")->setValue(flags & REGION_FLAGS_RESET_HOME_ON_TELEPORT ? TRUE : FALSE);
	getChild<LLUICtrl>("check damage")->setValue(flags & REGION_FLAGS_ALLOW_DAMAGE ? TRUE : FALSE);
	getChild<LLUICtrl>("check visible")->setValue(flags & REGION_FLAGS_EXTERNALLY_VISIBLE ? TRUE : FALSE);
	getChild<LLUICtrl>("block terraform")->setValue(flags & REGION_FLAGS_BLOCK_TERRAFORM ? TRUE : FALSE);
	getChild<LLUICtrl>("block dwell")->setValue(flags & REGION_FLAGS_BLOCK_DWELL ? TRUE : FALSE);
	getChild<LLUICtrl>("is sandbox")->setValue(flags & REGION_FLAGS_SANDBOX ? TRUE : FALSE );
	getChild<LLUICtrl>("is gaming")->setValue(flags & REGION_FLAGS_GAMING ? true : false);
	getChild<LLUICtrl>("hide from search")->setValue(flags & REGION_FLAGS_HIDE_FROM_SEARCH ? true : false);
}
void LLPanelRegionTools::setBillableFactor(F32 billable_factor)
{
	getChild<LLUICtrl>("billable factor")->setValue(billable_factor);
}
void LLPanelRegionTools::setPricePerMeter(S32 price)
{
	getChild<LLUICtrl>("land cost")->setValue(price);
}
void LLPanelRegionTools::onChangeAnything()
{
	if (gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(TRUE);
	}
}
void LLPanelRegionTools::onChangePrelude()
{
	if (getChild<LLUICtrl>("check prelude")->getValue().asBoolean())
	{
		getChild<LLUICtrl>("check fixed sun")->setValue(TRUE);
		getChild<LLUICtrl>("check reset home")->setValue(TRUE);
	}
	onChangeAnything();
}
void LLPanelRegionTools::onChangeSimName()
{
	if ( gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(TRUE);
	}
}
void LLPanelRegionTools::onRefresh()
{
	LLFloaterGodTools* god_tools = LLFloaterGodTools::instanceExists() ? LLFloaterGodTools::getInstance() : NULL;
	if(!god_tools) return;
	LLViewerRegion *region = gAgent.getRegion();
	if (region && gAgent.isGodlike())
	{
		god_tools->sendRegionInfoRequest();
	}
}
void LLPanelRegionTools::onApplyChanges()
{
	LLFloaterGodTools* god_tools = LLFloaterGodTools::instanceExists() ? LLFloaterGodTools::getInstance() : NULL;
	if(!god_tools) return;
	LLViewerRegion *region = gAgent.getRegion();
	if (region && gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(FALSE);
		god_tools->sendGodUpdateRegionInfo();
	}
}
void LLPanelRegionTools::onBakeTerrain()
{
	LLPanelRequestTools::sendRequest("terrain", "bake", gAgent.getRegionHost());
}
void LLPanelRegionTools::onRevertTerrain()
{
	LLPanelRequestTools::sendRequest("terrain", "revert", gAgent.getRegionHost());
}
void LLPanelRegionTools::onSwapTerrain()
{
	LLPanelRequestTools::sendRequest("terrain", "swap", gAgent.getRegionHost());
}
void LLPanelRegionTools::onSelectRegion()
{
	LL_INFOS() << "LLPanelRegionTools::onSelectRegion" << LL_ENDL;
	LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(gAgent.getPositionGlobal());
	if (!regionp)
	{
		return;
	}
	LLVector3d north_east(REGION_WIDTH_METERS, REGION_WIDTH_METERS, 0);
	LLViewerParcelMgr::getInstance()->selectLand(regionp->getOriginGlobal(),
						   regionp->getOriginGlobal() + north_east, FALSE);
}
const F32 HOURS_TO_RADIANS = (2.f*F_PI)/24.f;
LLPanelGridTools::LLPanelGridTools() :
	LLPanel()
{
	mCommitCallbackRegistrar.add("GridTools.KickAllUsers", boost::bind(&LLPanelGridTools::onClickKickAll, this));
	mCommitCallbackRegistrar.add("GridTools.FlushMapVisibilityCaches",		boost::bind(&LLPanelGridTools::onClickFlushMapVisibilityCaches, this));
}
LLPanelGridTools::~LLPanelGridTools()
{
}
BOOL LLPanelGridTools::postBuild()
{
	return TRUE;
}
void LLPanelGridTools::refresh()
{
}
void LLPanelGridTools::onClickKickAll()
{
	LLNotificationsUtil::add("KickAllUsers", LLSD(), LLSD(), LLPanelGridTools::confirmKick);
}
bool LLPanelGridTools::confirmKick(const LLSD& notification, const LLSD& response)
{
	if (LLNotificationsUtil::getSelectedOption(notification, response) == 0)
	{
		LLSD payload;
		payload["kick_message"] = response["message"].asString();
		LLNotificationsUtil::add("ConfirmKick", LLSD(), payload, LLPanelGridTools::finishKick);
	}
	return false;
}
bool LLPanelGridTools::finishKick(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   LL_UUID_ALL_AGENTS );
		msg->addU32("KickFlags", KICK_FLAGS_DEFAULT );
		msg->addStringFast(_PREHASH_Reason,    notification["payload"]["kick_message"].asString());
		gAgent.sendReliableMessage();
	}
	return false;
}
void LLPanelGridTools::onClickFlushMapVisibilityCaches()
{
	LLNotificationsUtil::add("FlushMapVisibilityCaches", LLSD(), LLSD(), flushMapVisibilityCachesConfirm);
}
bool LLPanelGridTools::flushMapVisibilityCachesConfirm(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0) return false;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null);
	msg->nextBlock("MethodData");
	msg->addString("Method", "refreshmapvisibility");
	msg->addUUID("Invoice", LLUUID::null);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", gAgent.getID().asString());
	gAgent.sendReliableMessage();
	return false;
}
LLPanelObjectTools::LLPanelObjectTools()
	: 	LLPanel(),
		mTargetAvatar()
{
	mCommitCallbackRegistrar.add("ObjectTools.ChangeAnything",		boost::bind(&LLPanelObjectTools::onChangeAnything, this));
	mCommitCallbackRegistrar.add("ObjectTools.DeletePublicOwnedBy",	boost::bind(&LLPanelObjectTools::onClickDeletePublicOwnedBy, this));
	mCommitCallbackRegistrar.add("ObjectTools.DeleteAllScriptedOwnedBy",		boost::bind(&LLPanelObjectTools::onClickDeleteAllScriptedOwnedBy, this));
	mCommitCallbackRegistrar.add("ObjectTools.DeleteAllOwnedBy",		boost::bind(&LLPanelObjectTools::onClickDeleteAllOwnedBy, this));
	mCommitCallbackRegistrar.add("ObjectTools.ApplyChanges",		boost::bind(&LLPanelObjectTools::onApplyChanges, this));
	mCommitCallbackRegistrar.add("ObjectTools.Set",		boost::bind(&LLPanelObjectTools::onClickSet, this));
	mCommitCallbackRegistrar.add("ObjectTools.GetTopColliders",		boost::bind(&LLPanelObjectTools::onGetTopColliders, this));
	mCommitCallbackRegistrar.add("ObjectTools.GetTopScripts",		boost::bind(&LLPanelObjectTools::onGetTopScripts, this));
	mCommitCallbackRegistrar.add("ObjectTools.GetScriptDigest",		boost::bind(&LLPanelObjectTools::onGetScriptDigest, this));
}
LLPanelObjectTools::~LLPanelObjectTools()
{
}
BOOL LLPanelObjectTools::postBuild()
{
	refresh();
	return TRUE;
}
void LLPanelObjectTools::setTargetAvatar(const LLUUID &target_id)
{
	mTargetAvatar = target_id;
	if (target_id.isNull())
	{
		getChild<LLUICtrl>("target_avatar_name")->setValue(getString("no_target"));
	}
}
void LLPanelObjectTools::refresh()
{
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		getChild<LLUICtrl>("region name")->setValue(regionp->getName());
	}
}
U64 LLPanelObjectTools::computeRegionFlags(U64 flags) const
{
	if (getChild<LLUICtrl>("disable scripts")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_SCRIPTS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_SCRIPTS;
	}
	if (getChild<LLUICtrl>("disable collisions")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_COLLISIONS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_COLLISIONS;
	}
	if (getChild<LLUICtrl>("disable physics")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_PHYSICS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_PHYSICS;
	}
	return flags;
}
void LLPanelObjectTools::setCheckFlags(U64 flags)
{
	getChild<LLUICtrl>("disable scripts")->setValue(flags & REGION_FLAGS_SKIP_SCRIPTS ? TRUE : FALSE);
	getChild<LLUICtrl>("disable collisions")->setValue(flags & REGION_FLAGS_SKIP_COLLISIONS ? TRUE : FALSE);
	getChild<LLUICtrl>("disable physics")->setValue(flags & REGION_FLAGS_SKIP_PHYSICS ? TRUE : FALSE);
}
void LLPanelObjectTools::clearAllWidgets()
{
	getChild<LLUICtrl>("disable scripts")->setValue(FALSE);
	getChildView("disable scripts")->setEnabled(FALSE);
	getChildView("Apply")->setEnabled(FALSE);
	getChildView("Set Target")->setEnabled(FALSE);
	getChildView("Delete Target's Scripted Objects On Others Land")->setEnabled(FALSE);
	getChildView("Delete Target's Scripted Objects On *Any* Land")->setEnabled(FALSE);
	getChildView("Delete *ALL* Of Target's Objects")->setEnabled(FALSE);
}
void LLPanelObjectTools::enableAllWidgets()
{
	getChildView("disable scripts")->setEnabled(TRUE);
	getChildView("Apply")->setEnabled(FALSE);
	getChildView("Set Target")->setEnabled(TRUE);
	getChildView("Delete Target's Scripted Objects On Others Land")->setEnabled(TRUE);
	getChildView("Delete Target's Scripted Objects On *Any* Land")->setEnabled(TRUE);
	getChildView("Delete *ALL* Of Target's Objects")->setEnabled(TRUE);
	getChildView("Get Top Colliders")->setEnabled(TRUE);
	getChildView("Get Top Scripts")->setEnabled(TRUE);
}
void LLPanelObjectTools::onGetTopColliders()
{
	LLFloaterTopObjects* instance = LLFloaterTopObjects::getInstance();
	if(!instance) return;
	if (gAgent.isGodlike())
	{
		instance->open();
		LLFloaterTopObjects::setMode(STAT_REPORT_TOP_COLLIDERS);
		instance->onRefresh();
	}
}
void LLPanelObjectTools::onGetTopScripts()
{
	LLFloaterTopObjects* instance = LLFloaterTopObjects::getInstance();
	if(!instance) return;
	if (gAgent.isGodlike())
	{
		instance->open();
		LLFloaterTopObjects::setMode(STAT_REPORT_TOP_SCRIPTS);
		instance->onRefresh();
	}
}
void LLPanelObjectTools::onGetScriptDigest()
{
	if (gAgent.isGodlike())
	{
		LLPanelRequestTools::sendRequest("scriptdigest","0",gAgent.getRegionHost());
	}
}
void LLPanelObjectTools::onClickDeletePublicOwnedBy()
{
	if (!mTargetAvatar.isNull())
	{
		mSimWideDeletesFlags =
			SWD_SCRIPTED_ONLY | SWD_OTHERS_LAND_ONLY;
		LLSD args;
		args["AVATAR_NAME"] = getChild<LLUICtrl>("target_avatar_name")->getValue().asString();
		LLSD payload;
		payload["avatar_id"] = mTargetAvatar;
		payload["flags"] = (S32)mSimWideDeletesFlags;
		LLNotificationsUtil::add( "GodDeleteAllScriptedPublicObjectsByUser",
								args,
								payload,
								callbackSimWideDeletes);
	}
}
void LLPanelObjectTools::onClickDeleteAllScriptedOwnedBy()
{
	if (!mTargetAvatar.isNull())
	{
		mSimWideDeletesFlags = SWD_SCRIPTED_ONLY;
		LLSD args;
		args["AVATAR_NAME"] = getChild<LLUICtrl>("target_avatar_name")->getValue().asString();
		LLSD payload;
		payload["avatar_id"] = mTargetAvatar;
		payload["flags"] = (S32)mSimWideDeletesFlags;
		LLNotificationsUtil::add( "GodDeleteAllScriptedObjectsByUser",
								args,
								payload,
								callbackSimWideDeletes);
	}
}
void LLPanelObjectTools::onClickDeleteAllOwnedBy()
{
	if (!mTargetAvatar.isNull())
	{
		mSimWideDeletesFlags = 0;
		LLSD args;
		args["AVATAR_NAME"] = getChild<LLUICtrl>("target_avatar_name")->getValue().asString();
		LLSD payload;
		payload["avatar_id"] = mTargetAvatar;
		payload["flags"] = (S32)mSimWideDeletesFlags;
		LLNotificationsUtil::add( "GodDeleteAllObjectsByUser",
								args,
								payload,
								callbackSimWideDeletes);
	}
}
bool LLPanelObjectTools::callbackSimWideDeletes( const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		if (!notification["payload"]["avatar_id"].asUUID().isNull())
		{
			send_sim_wide_deletes(notification["payload"]["avatar_id"].asUUID(),
								  notification["payload"]["flags"].asInteger());
		}
	}
	return false;
}
void LLPanelObjectTools::onClickSet()
{
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLPanelObjectTools::callbackAvatarID, this, _1,_2));
	if (picker)
	{
		gFloaterView->getParentFloater(this)->addDependentFloater(picker);
	}
}
void LLPanelObjectTools::onClickSetBySelection(void* data)
{
	LLPanelObjectTools* panelp = (LLPanelObjectTools*) data;
	if (!panelp) return;
	const BOOL non_root_ok = TRUE;
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode(NULL, non_root_ok);
	if (!node) return;
	std::string owner_name;
	LLUUID owner_id;
	LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);
	panelp->mTargetAvatar = owner_id;
	LLStringUtil::format_map_t args;
	args["[OBJECT]"] = node->mName;
	args["[OWNER]"] = owner_name;
	std::string name = LLTrans::getString("GodToolsObjectOwnedBy", args);
	panelp->getChild<LLUICtrl>("target_avatar_name")->setValue(name);
}
void LLPanelObjectTools::callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName> names)
{
	if (ids.empty() || names.empty()) return;
	mTargetAvatar = ids[0];
	getChild<LLUICtrl>("target_avatar_name")->setValue(names[0].getCompleteName());
	refresh();
}
void LLPanelObjectTools::onChangeAnything()
{
	if (gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(TRUE);
	}
}
void LLPanelObjectTools::onApplyChanges()
{
	LLFloaterGodTools* god_tools = LLFloaterGodTools::instanceExists() ? LLFloaterGodTools::getInstance() : NULL;
	if(!god_tools) return;
	LLViewerRegion *region = gAgent.getRegion();
	if (region && gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(FALSE);
		god_tools->sendGodUpdateRegionInfo();
	}
}
const std::string SELECTION = "Selection";
const std::string AGENT_REGION = "Agent Region";
LLPanelRequestTools::LLPanelRequestTools():
	LLPanel()
{
	mCommitCallbackRegistrar.add("GodTools.Request",		boost::bind(&LLPanelRequestTools::onClickRequest, this));
}
LLPanelRequestTools::~LLPanelRequestTools()
{
}
BOOL LLPanelRequestTools::postBuild()
{
	refresh();
	return TRUE;
}
void LLPanelRequestTools::refresh()
{
	std::string buffer = getChild<LLUICtrl>("destination")->getValue();
	LLCtrlListInterface *list = childGetListInterface("destination");
	if (!list) return;
	list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	list->addSimpleElement(SELECTION);
	list->addSimpleElement(AGENT_REGION);
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
		 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		std::string name = regionp->getName();
		if (!name.empty())
		{
			list->addSimpleElement(name);
		}
	}
	if(!buffer.empty())
	{
		list->selectByValue(buffer);
	}
	else
	{
		list->selectByValue(SELECTION);
	}
}
void LLPanelRequestTools::sendRequest(const std::string& request,
									  const std::string& parameter,
									  const LLHost& host)
{
	LL_INFOS() << "Sending request '" << request << "', '"
			<< parameter << "' to " << host << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GodlikeMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null);
	msg->nextBlock("MethodData");
	msg->addString("Method", request);
	msg->addUUID("Invoice", LLUUID::null);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", parameter);
	msg->sendReliable(host);
}
void LLPanelRequestTools::onClickRequest()
{
	const std::string dest = getChild<LLUICtrl>("destination")->getValue().asString();
	if(dest == SELECTION)
	{
		std::string req =getChild<LLUICtrl>("request")->getValue();
		req = req.substr(0, req.find_first_of(' '));
		std::string param = getChild<LLUICtrl>("parameter")->getValue();
		LLSelectMgr::getInstance()->sendGodlikeRequest(req, param);
	}
	else if(dest == AGENT_REGION)
	{
		sendRequest(gAgent.getRegionHost());
	}
	else
	{
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
			 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			if(dest == regionp->getName())
			{
				sendRequest(regionp->getHost());
			}
		}
	}
}
void terrain_download_done(void** data, S32 status, LLExtStat ext_status)
{
	LLNotificationsUtil::add("TerrainDownloaded");
}
void test_callback(const LLTSCode status)
{
	LL_INFOS() << "Test transfer callback returned!" << LL_ENDL;
}
void LLPanelRequestTools::sendRequest(const LLHost& host)
{
	std::string req = getChild<LLUICtrl>("request")->getValue();
	if (req == "terrain download")
	{
		gXferManager->requestFile(std::string("terrain.raw"), std::string("terrain.raw"), LL_PATH_NONE,
								  host,
								  FALSE,
								  terrain_download_done,
								  nullptr);
	}
	else
	{
		req = req.substr(0, req.find_first_of(' '));
		sendRequest(req, getChild<LLUICtrl>("parameter")->getValue().asString(), host);
	}
}
void send_sim_wide_deletes(const LLUUID& owner_id, U32 flags)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_SimWideDeletes);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_DataBlock);
	msg->addUUIDFast(_PREHASH_TargetID, owner_id);
	msg->addU32Fast(_PREHASH_Flags, flags);
	gAgent.sendReliableMessage();
}
