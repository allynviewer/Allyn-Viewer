/* Copyright (C) 2013 Liru Færs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA */
#include "llviewerprecompiledheaders.h"
#include "lfsimfeaturehandler.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llmutelist.h"
#include "hippogridmanager.h"
LFSimFeatureHandler::LFSimFeatureHandler()
: mSupportsExport(false)
, mDestinationGuideURL(gSavedSettings.getString("DestinationGuideURL"))
, mSearchURL(gSavedSettings.getString("SearchURL"))
, mSayRange(20)
, mShoutRange(100)
, mWhisperRange(10)
{
	if (!gHippoGridManager->getCurrentGrid()->isSecondLife())
		gAgent.addRegionChangedCallback(boost::bind(&LFSimFeatureHandler::handleRegionChange, this));
	LLMuteList::instance().mGodLastNames = { "Linden", "ProductEngine" };
}
ExportPolicy LFSimFeatureHandler::exportPolicy() const
{
	return gHippoGridManager->getCurrentGrid()->isSecondLife() ? ep_creator_only : (mSupportsExport ? ep_export_bit : ep_full_perm);
}
void LFSimFeatureHandler::handleRegionChange()
{
	if (LLViewerRegion* region = gAgent.getRegion())
	{
		if (region->simulatorFeaturesReceived())
		{
			setSupportedFeatures();
		}
		else
		{
			region->setSimulatorFeaturesReceivedCallback(boost::bind(&LFSimFeatureHandler::setSupportedFeatures, this));
		}
	}
}
template<typename T>
void has_feature_or_default(SignaledType<T>& type, const LLSD& features, const std::string& feature)
{
	type = (features.has(feature)) ? static_cast<T>(features[feature]) : type.getDefault();
}
template<>
void has_feature_or_default(SignaledType<U32>& type, const LLSD& features, const std::string& feature)
{
	type = (features.has(feature)) ? features[feature].asInteger() : type.getDefault();
}
void LFSimFeatureHandler::setSupportedFeatures()
{
	if (LLViewerRegion* region = gAgent.getRegion())
	{
		LLSD info;
		region->getSimulatorFeatures(info);
		if (info.has("OpenSimExtras"))
		{
			const LLSD& extras(info["OpenSimExtras"]);
			has_feature_or_default(mSupportsExport, extras, "ExportSupported");
			{
				has_feature_or_default(mDestinationGuideURL, extras, "destination-guide-url");
				mMapServerURL = extras.has("map-server-url") ? extras["map-server-url"].asString() : LLStringUtil::null;
				has_feature_or_default(mSearchURL, extras, "search-server-url");
				if (extras.has("GridName"))
				{
					const std::string& grid_name(extras["GridName"]);
					mGridName = gHippoGridManager->getConnectedGrid()->getGridName() != grid_name ? grid_name : LLStringUtil::null;
				}
			}
			has_feature_or_default(mEventsURL, extras, "EventsURL");
			has_feature_or_default(mSayRange, extras, "say-range");
			has_feature_or_default(mShoutRange, extras, "shout-range");
			has_feature_or_default(mWhisperRange, extras, "whisper-range");
		}
		else
		{
			mSupportsExport.reset();
			{
				mDestinationGuideURL.reset();
				mMapServerURL = LLStringUtil::null;
				mSearchURL.reset();
				mGridName.reset();
				mEventsURL.reset();
			}
			mSayRange.reset();
			mShoutRange.reset();
			mWhisperRange.reset();
		}
		LLMuteList& mute_list(LLMuteList::instance());
		mute_list.mGodLastNames.clear();
		mute_list.mGodFullNames.clear();
		if (info.has("god_names"))
		{
			const LLSD& god_names(info["god_names"]);
			if (god_names.has("last_names"))
			{
				const LLSD& last_names(god_names["last_names"]);
				for (LLSD::array_const_iterator it = last_names.beginArray(); it != last_names.endArray(); ++it)
					mute_list.mGodLastNames.insert((*it).asString());
			}
			if (god_names.has("full_names"))
			{
				const LLSD& full_names(god_names["full_names"]);
				for (LLSD::array_const_iterator it = full_names.beginArray(); it != full_names.endArray(); ++it)
					mute_list.mGodFullNames.insert((*it).asString());
			}
		}
		else
		{
			mute_list.mGodLastNames = { "Linden", "ProductEngine" };
		}
	}
}
