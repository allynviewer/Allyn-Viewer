/** 
 * @file llregioninfomodel.cpp
 * @brief Region info model
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "llregioninfomodel.h"
#include "message.h"
#include "llregionflags.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llestateinfomodel.h"
void LLRegionInfoModel::reset()
{
	mSimAccess			= 0;
	mAgentLimit			= 0;
	mRegionFlags		= 0;
	mEstateID			= 0;
	mParentEstateID		= 0;
	mPricePerMeter		= 0;
	mRedirectGridX		= 0;
	mRedirectGridY		= 0;
	mBillableFactor		= 0.0f;
	mObjectBonusFactor	= 0.0f;
	mWaterHeight		= 0.0f;
	mTerrainRaiseLimit	= 0.0f;
	mTerrainLowerLimit	= 0.0f;
	mSunHour			= 0.0f;
	mUseEstateSun		= false;
	mSimType.clear();
	mSimName.clear();
}
LLRegionInfoModel::LLRegionInfoModel()
{
	reset();
}
boost::signals2::connection LLRegionInfoModel::setUpdateCallback(const update_signal_t::slot_type& cb)
{
	return mUpdateSignal.connect(cb);
}
void LLRegionInfoModel::sendRegionTerrain(const LLUUID& invoice) const
{
	std::string buffer;
	std::vector<std::string> strings;
	BOOL estate_global_time = true;
	BOOL estate_fixed_sun = false;
	F32 estate_sun_hour = 0.f;
	buffer = llformat("%f", mWaterHeight);
	strings.push_back(buffer);
	buffer = llformat("%f", mTerrainRaiseLimit);
	strings.push_back(buffer);
	buffer = llformat("%f", mTerrainLowerLimit);
	strings.push_back(buffer);
	buffer = llformat("%s", (mUseEstateSun ? "Y" : "N"));
	strings.push_back(buffer);
	buffer = llformat("%s", (getUseFixedSun() ? "Y" : "N"));
	strings.push_back(buffer);
	buffer = llformat("%f", mSunHour);
	strings.push_back(buffer);
	if (mUseEstateSun)
	{
		LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();
		estate_global_time = estate_info.getGlobalTime();
		if (!estate_global_time)
		{
			estate_fixed_sun = estate_info.getUseFixedSun();
			estate_sun_hour = estate_info.getSunHour();
		}
	}
	buffer = llformat("%s", (estate_global_time ? "Y" : "N") );
	strings.push_back(buffer);
	buffer = llformat("%s", (estate_fixed_sun ? "Y" : "N") );
	strings.push_back(buffer);
	buffer = llformat("%f", estate_sun_hour);
	strings.push_back(buffer);
	sendEstateOwnerMessage(gMessageSystem, "setregionterrain", invoice, strings);
}
bool LLRegionInfoModel::getUseFixedSun() const
{
	return ((mRegionFlags & REGION_FLAGS_SUN_FIXED) != 0);
}
void LLRegionInfoModel::setUseFixedSun(bool fixed)
{
	if (fixed)
	{
		mRegionFlags |= REGION_FLAGS_SUN_FIXED;
	}
	else
	{
		mRegionFlags &= ~REGION_FLAGS_SUN_FIXED;
	}
}
void LLRegionInfoModel::update(LLMessageSystem* msg)
{
	reset();
	msg->getStringFast(_PREHASH_RegionInfo, _PREHASH_SimName, mSimName);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_EstateID, mEstateID);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_ParentEstateID, mParentEstateID);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_SimAccess, mSimAccess);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_MaxAgents, mAgentLimit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_ObjectBonusFactor, mObjectBonusFactor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_BillableFactor, mBillableFactor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_WaterHeight, mWaterHeight);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainRaiseLimit, mTerrainRaiseLimit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainLowerLimit, mTerrainLowerLimit);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_PricePerMeter, mPricePerMeter);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridX, mRedirectGridX);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridY, mRedirectGridY);
	msg->getBOOL(_PREHASH_RegionInfo, _PREHASH_UseEstateSun, mUseEstateSun);
	msg->getF32(_PREHASH_RegionInfo, _PREHASH_SunHour, mSunHour);
	LL_DEBUGS("Windlight Sync") << "Got region sun hour: " << mSunHour << LL_ENDL;
	if (msg->has(_PREHASH_RegionInfo3))
	{
		msg->getU64Fast(_PREHASH_RegionInfo3, _PREHASH_RegionFlagsExtended, mRegionFlags);
	}
	else
	{
		U32 flags = 0;
		msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_RegionFlags, flags);
		mRegionFlags = flags;
	}
	if (msg->getSize(_PREHASH_RegionInfo2, _PREHASH_ProductSKU) > 0 ||
		msg->getSize(_PREHASH_RegionInfo2, "ProductName") > 0)
	{
		msg->getString(_PREHASH_RegionInfo2, "ProductName", mSimType);
	}
	mUpdateSignal();
}
void LLRegionInfoModel::sendEstateOwnerMessage(
	LLMessageSystem* msg,
	const std::string& request,
	const LLUUID& invoice,
	const std::vector<std::string>& strings)
{
	LLViewerRegion* cur_region = gAgent.getRegion();
	if (!cur_region)
	{
		LL_WARNS() << "Agent region not set" << LL_ENDL;
		return;
	}
	LL_INFOS() << "Sending estate request '" << request << "'" << LL_ENDL;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null);
	msg->nextBlock("MethodData");
	msg->addString("Method", request);
	msg->addUUID("Invoice", invoice);
	if (strings.empty())
	{
		msg->nextBlock("ParamList");
		msg->addString("Parameter", NULL);
	}
	else
	{
		std::vector<std::string>::const_iterator it = strings.begin();
		std::vector<std::string>::const_iterator end = strings.end();
		for (unsigned i = 0; it != end; ++it, ++i)
		{
			LL_DEBUGS() << "- [" << i << "] " << (*it) << LL_ENDL;
			msg->nextBlock("ParamList");
			msg->addString("Parameter", *it);
		}
	}
	msg->sendReliable(cur_region->getHost());
}
