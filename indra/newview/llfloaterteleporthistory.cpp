/** 
 * @file llfloaterteleporthistory.cpp
 * @author Zi Ree
 * @brief LLFloaterTeleportHistory class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llfloaterteleporthistory.h"
#include "llappviewer.h"
#include "llfloaterworldmap.h"
#include "llscrolllistcolumn.h"
#include "llscrolllistitem.h"
#include "llsdserialize.h"
#include "llslurl.h"
#include "lluictrlfactory.h"
#include "llurlaction.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llweb.h"
#include "rlvhandler.h"
LLFloaterTeleportHistory::LLFloaterTeleportHistory(const LLSD& seed)
:	LLFloater(std::string("teleporthistory")),
	mPlacesList(NULL),
	mID(0)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_teleport_history.xml", NULL);
}
LLFloaterTeleportHistory::~LLFloaterTeleportHistory()
{
}
void LLFloaterTeleportHistory::onFocusReceived()
{
	if (mPlacesList->getFirstSelected())
	{
		setButtonsEnabled(TRUE);
	}
	else
	{
		setButtonsEnabled(FALSE);
	}
	LLFloater::onFocusReceived();
}
BOOL LLFloaterTeleportHistory::postBuild()
{
	mPlacesList=getChild<LLScrollListCtrl>("places_list");
	if (!mPlacesList)
	{
		LL_WARNS() << "coud not get pointer to places list" << LL_ENDL;
		return FALSE;
	}
	mPlacesList->setDoubleClickCallback(boost::bind(&LLFloaterTeleportHistory::onTeleport,this));
	mPlacesList->setCommitCallback(boost::bind(&LLFloaterTeleportHistory::onPlacesSelected,_1,this));
	childSetAction("teleport", onTeleport, this);
	childSetAction("show_on_map", onShowOnMap, this);
	childSetAction("copy_slurl", onCopySLURL, this);
	return TRUE;
}
void LLFloaterTeleportHistory::addPendingEntry(std::string regionName, S16 x, S16 y, S16 z)
{
	if(gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
		return;
	struct tm* internal_time = utc_to_pacific_time(time_corrected(), gPacificDaylightTime);
	timeStructToFormattedString(internal_time, gSavedSettings.getString("ShortDateFormat") + gSavedSettings.getString("LongTimeFormat"), mPendingTimeString);
	mPendingTimeString += gPacificDaylightTime ? " PDT" : " PST";
	mPendingRegionName = regionName;
	mPendingPosition = llformat("%d, %d, %d", x, y, z);
	LLSLURL slurl(regionName, LLVector3(x, y, z));
	mPendingSimString = LLWeb::escapeURL(slurl.getLocationString());
	mPendingSLURL = slurl.getSLURLString();
}
void LLFloaterTeleportHistory::addEntry(std::string parcelName)
{
	if (mPendingRegionName.empty())
	{
		return;
	}
	if (mPlacesList)
	{
		LLSD value;
		value["id"] = mID;
		value["columns"][LIST_PARCEL]["column"] = "parcel";
		value["columns"][LIST_PARCEL]["value"] = parcelName;
		value["columns"][LIST_REGION]["column"] = "region";
		value["columns"][LIST_REGION]["value"] = mPendingRegionName;
		value["columns"][LIST_POSITION]["column"] = "position";
		value["columns"][LIST_POSITION]["value"] = mPendingPosition;
		value["columns"][LIST_VISITED]["column"] = "visited";
		value["columns"][LIST_VISITED]["value"] = mPendingTimeString;
		value["columns"][LIST_SLURL]["column"] = "slurl";
		value["columns"][LIST_SLURL]["value"] = mPendingSLURL;
		value["columns"][LIST_SIMSTRING]["column"] = "simstring";
		value["columns"][LIST_SIMSTRING]["value"] = mPendingSimString;
		const S32 max_entries = gSavedSettings.getS32("TeleportHistoryMaxEntries");
		S32 num_entries = mPlacesList->getItemCount();
		if (max_entries <= 0)
		{
			if (!max_entries)
				mPlacesList->clearRows();
		}
		else while(num_entries >= max_entries)
		{
			mPlacesList->deleteItems(LLSD(mID - num_entries--));
		}
		mPlacesList->addElement(value, ADD_TOP);
		mPlacesList->deselectAllItems(TRUE);
		setButtonsEnabled(FALSE);
		mID++;
		saveFile("teleport_history.xml");
	}
	else
	{
		LL_WARNS() << "pointer to places list is NULL" << LL_ENDL;
	}
	mPendingRegionName.clear();
}
void LLFloaterTeleportHistory::setButtonsEnabled(BOOL on)
{
	childSetEnabled("teleport", on);
	childSetEnabled("show_on_map", on);
	childSetEnabled("copy_slurl", on);
}
void LLFloaterTeleportHistory::loadFile(const std::string &file_name)
{
	LLFloaterTeleportHistory *pFloaterHistory = LLFloaterTeleportHistory::findInstance();
	if(!pFloaterHistory)
		return;
	LLScrollListCtrl* pScrollList = pFloaterHistory->mPlacesList;
	if(!pScrollList)
		return;
	std::string temp_str(gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + file_name);
	llifstream file(temp_str);
	if (file.is_open())
	{
		LL_INFOS() << "Loading "<< file_name << " file at " << temp_str << LL_ENDL;
		LLSD data;
		LLSDSerialize::fromXML(data, file);
		if (data.isUndefined())
		{
			LL_INFOS() << "file missing, ill-formed, "
				"or simply undefined; not reading the"
				" file" << LL_ENDL;
		}
		else
		{
			const S32 max_entries = gSavedSettings.getS32("TeleportHistoryMaxEntries");
			const S32 num_entries = llmin(max_entries < 0 ? S32_MAX : max_entries,(const S32)data.size());
			pScrollList->clear();
			for(S32 i = 0; i < num_entries; i++)
			{
				pScrollList->addElement(data[i], ADD_BOTTOM);
			}
			pScrollList->deselectAllItems(TRUE);
			pFloaterHistory->mID = pScrollList->getItemCount();
			pFloaterHistory->setButtonsEnabled(FALSE);
		}
	}
}
struct SortByAge{
  inline bool operator() (LLScrollListItem* const i,LLScrollListItem* const j) const {
	  return (i->getValue().asInteger()>j->getValue().asInteger());
  }
};
void LLFloaterTeleportHistory::saveFile(const std::string &file_name)
{
	LLFloaterTeleportHistory *pFloaterHistory = LLFloaterTeleportHistory::findInstance();
	if(!pFloaterHistory)
		return;
	std::string temp_str = gDirUtilp->getLindenUserDir(true);
	if( temp_str.empty() )
	{
		LL_INFOS() << "Can't save teleport history - no user directory set yet." << LL_ENDL;
		return;
	}
	temp_str += gDirUtilp->getDirDelimiter() + file_name;
	llofstream export_file(temp_str);
	if (!export_file.good())
	{
		LL_WARNS() << "Unable to open " << file_name << " for output." << LL_ENDL;
		return;
	}
	LL_INFOS() << "Writing "<< file_name << " file at " << temp_str << LL_ENDL;
	LLSD elements;
	LLScrollListCtrl* pScrollList = pFloaterHistory->mPlacesList;
	if (pScrollList)
	{
		std::vector<LLScrollListItem*> data_list = pScrollList->getAllData();
		std::sort(data_list.begin(),data_list.end(),SortByAge());
		for (std::vector<LLScrollListItem*>::iterator itr = data_list.begin(); itr != data_list.end(); ++itr)
		{
			LLSD data_entry;
			data_entry["id"] = pScrollList->getItemCount() - elements.size() - 1;
			for(S32 i = 0; i < (*itr)->getNumColumns(); ++i)
			{
				data_entry["columns"][i]["column"]=pScrollList->getColumn(i)->mName;
				data_entry["columns"][i]["value"]=(*itr)->getColumn(i)->getValue();
			}
			elements.append(data_entry);
		}
	}
	LLSDSerialize::toXML(elements, export_file);
	export_file.close();
}
void LLFloaterTeleportHistory::onClose(bool app_quitting)
{
	LLFloater::setVisible(FALSE);
}
BOOL LLFloaterTeleportHistory::canClose()
{
	return !LLApp::isExiting();
}
void LLFloaterTeleportHistory::onPlacesSelected(LLUICtrl* , void* data)
{
	LLFloaterTeleportHistory* self = (LLFloaterTeleportHistory*) data;
	if (self->mPlacesList->getFirstSelected())
	{
		self->setButtonsEnabled(TRUE);
	}
	else
	{
		self->setButtonsEnabled(FALSE);
	}
}
void LLFloaterTeleportHistory::onTeleport(void* data)
{
	LLFloaterTeleportHistory* self = (LLFloaterTeleportHistory*) data;
	std::string slapp = "secondlife:///app/teleport/" + self->mPlacesList->getFirstSelected()->getColumn(LIST_SIMSTRING)->getValue().asString();
	LLUrlAction::teleportToLocation(slapp);
}
void LLFloaterTeleportHistory::onShowOnMap(void* data)
{
	LLFloaterTeleportHistory* self = (LLFloaterTeleportHistory*) data;
	std::string simString = self->mPlacesList->getFirstSelected()->getColumn(LIST_SIMSTRING)->getValue().asString();
	LLSLURL slurl(simString);
	gFloaterWorldMap->trackURL(slurl.getRegion(), slurl.getPosition().mV[VX], slurl.getPosition().mV[VY], slurl.getPosition().mV[VZ]);
	LLFloaterWorldMap::show(true);
}
void LLFloaterTeleportHistory::onCopySLURL(void* data)
{
	LLFloaterTeleportHistory* self = (LLFloaterTeleportHistory*) data;
	std::string SLURL = self->mPlacesList->getFirstSelected()->getColumn(LIST_SLURL)->getValue().asString();
	gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(SLURL));
}
