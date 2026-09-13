/** 
 * @file llslurl.cpp (was llsimurlstring.cpp)
 * @brief Handles "SLURL fragments" like Ahern/123/45 for
 * startup processing, login screen, prefs, etc.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "llslurl.h"
#include "llpanellogin.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llfiltersd2xmlrpc.h"
#include "curl/curl.h"
#include "hippogridmanager.h"
#include "llworldmap.h"
const char* LLSLURL::SLURL_HTTP_SCHEME		 = "http";
const char* LLSLURL::SLURL_HTTPS_SCHEME		 = "https";
const char* LLSLURL::SLURL_SECONDLIFE_SCHEME	 = "secondlife";
const char* LLSLURL::SLURL_SECONDLIFE_PATH	 = "secondlife";
const char* LLSLURL::SLURL_COM		         = "slurl.com";
const char* LLSLURL::WWW_SLURL_COM				 = "www.slurl.com";
const char* LLSLURL::MAPS_SECONDLIFE_COM		 = "maps.secondlife.com";
const char* LLSLURL::SLURL_X_GRID_INFO_SCHEME	 = "x-grid-info";
const char* LLSLURL::SLURL_X_GRID_LOCATION_INFO_SCHEME = "x-grid-location-info";
const char* LLSLURL::SLURL_APP_PATH              = "app";
const char* LLSLURL::SLURL_REGION_PATH           = "region";
const char* LLSLURL::SIM_LOCATION_HOME           = "home";
const char* LLSLURL::SIM_LOCATION_LAST           = "last";
const std::string MAIN_GRID_SLURL_BASE = "http://maps.secondlife.com/secondlife/";
const std::string SYSTEM_GRID_APP_SLURL_BASE = "secondlife:///app";
const char* SYSTEM_GRID_SLURL_BASE = "secondlife://%s/secondlife/";
const char* DEFAULT_SLURL_BASE = "x-grid-info://%s/region/";
const char* DEFAULT_APP_SLURL_BASE = "x-grid-info://%s/app";
#define MAINGRID "secondlife"
LLSLURL::LLSLURL(const std::string& slurl)
{
	mType = INVALID;
	if(slurl == SIM_LOCATION_HOME)
	{
		mType = HOME_LOCATION;
	}
	else if(slurl.empty() || (slurl == SIM_LOCATION_LAST))
	{
		mType = LAST_LOCATION;
	}
	else
	{
		LLURI slurl_uri;
		if(slurl.find(':') == std::string::npos)
		{
			std::string fixed_slurl;
			if(gHippoGridManager->getCurrentGrid()->isSecondLife())
			{
				if(gHippoGridManager->getCurrentGrid()->isInProductionGrid())
					fixed_slurl = MAIN_GRID_SLURL_BASE;
				else
					fixed_slurl = llformat(SYSTEM_GRID_SLURL_BASE, gHippoGridManager->getCurrentGridNick().c_str());
			}
			else
				fixed_slurl = llformat(DEFAULT_SLURL_BASE, gHippoGridManager->getCurrentGridNick().c_str());
			if(slurl[0] == '/')
		    {
				fixed_slurl += slurl.substr(1);
		    }
			else
		    {
				fixed_slurl += slurl;
		    }
			slurl_uri = LLURI(fixed_slurl);
		}
		else
		{
		    slurl_uri = LLURI(slurl);
		}
		LLSD path_array = slurl_uri.pathArray();
		if(slurl_uri.scheme() == LLSLURL::SLURL_SECONDLIFE_SCHEME)
		{
			mGrid = MAINGRID;
			if ((path_array[0].asString() == LLSLURL::SLURL_SECONDLIFE_PATH) ||
				(path_array[0].asString() == LLSLURL::SLURL_APP_PATH))
		    {
				if (!slurl_uri.hostName().empty())
				{
					if(slurl_uri.hostName() == "util.agni.lindenlab.com")
						mGrid = MAINGRID;
					else if(slurl_uri.hostName() == "util.aditi.lindenlab.com")
						mGrid = "secondlife_beta";
					else
					{
						HippoGridInfo* grid = gHippoGridManager->getGrid(slurl_uri.hostName());
						mGrid = grid ? grid->getGridNick() : gHippoGridManager->getDefaultGridNick();
					}
				}
				else if(path_array[0].asString() == LLSLURL::SLURL_SECONDLIFE_PATH)
				{
					mGrid = MAINGRID;
				}
				else if(path_array[0].asString() == LLSLURL::SLURL_APP_PATH)
				{
					mGrid = gHippoGridManager->getCurrentGridNick();
				}
				if(mGrid.empty())
				{
					LL_WARNS("AppInit")<<"unable to find grid"<<LL_ENDL;
					return;
				}
				if(path_array[0].asString() == LLSLURL::SLURL_SECONDLIFE_PATH)
				{
					mType = LOCATION;
				}
				else
				{
					mType = APP;
				}
				path_array.erase(0);
		    }
			else
		    {
				mType = LOCATION;
				path_array.insert(0, slurl_uri.hostName());
		    }
		}
		else if((slurl_uri.scheme() == LLSLURL::SLURL_HTTP_SCHEME) ||
		   (slurl_uri.scheme() == LLSLURL::SLURL_HTTPS_SCHEME) ||
				 (slurl_uri.scheme() == LLSLURL::SLURL_X_GRID_INFO_SCHEME) ||
				 (slurl_uri.scheme() == LLSLURL::SLURL_X_GRID_LOCATION_INFO_SCHEME))
		{
		  if ((slurl_uri.hostName() == LLSLURL::SLURL_COM) ||
		      (slurl_uri.hostName() == LLSLURL::WWW_SLURL_COM) ||
		      (slurl_uri.hostName() == LLSLURL::MAPS_SECONDLIFE_COM))
			{
				mGrid = MAINGRID;
			}
		    else
			{
				if ((slurl_uri.scheme() == LLSLURL::SLURL_HTTP_SCHEME ||
					 slurl_uri.scheme() == LLSLURL::SLURL_HTTPS_SCHEME) &&
					slurl_uri.hostName() != gHippoGridManager->getCurrentGridNick())
				{
					return;
				}
				mGrid = slurl_uri.hostNameAndPort();
			}
		    if (path_array.size() == 0)
			{
				return;
			}
		    if ((path_array[0].asString() == LLSLURL::SLURL_REGION_PATH) ||
				(path_array[0].asString() == LLSLURL::SLURL_SECONDLIFE_PATH))
			{
				path_array.erase(0);
				mType = LOCATION;
			}
			else if (path_array[0].asString() == LLSLURL::SLURL_APP_PATH)
			{
				mType = APP;
				path_array.erase(0);
			}
			else
			{
				return;
			}
		}
		else
		{
		    return;
		}
		if(path_array.size() == 0)
		{
			return;
		}
		if(mType == APP)
		{
			mAppCmd = path_array[0].asString();
			path_array.erase(0);
			mAppPath = path_array;
			mAppQuery = slurl_uri.query();
			mAppQueryMap = slurl_uri.queryMap();
			return;
		}
		else if(mType == LOCATION)
		{
			mRegion = LLURI::unescape(path_array[0].asString());
			if(LLStringUtil::containsNonprintable(mRegion))
			{
				LLStringUtil::stripNonprintable(mRegion);
			}
			path_array.erase(0);
			if(path_array.size() >= 2)
			{
			  mPosition = LLVector3(path_array);
			  if((F32(mPosition[VX]) < 0.f) ||
                             (mPosition[VX] > 8192.f) ||
			     (F32(mPosition[VY]) < 0.f) ||
                             (mPosition[VY] > 8192.f) ||
			     (F32(mPosition[VZ]) < 0.f) ||
                             (mPosition[VZ] > 8192.f))
			    {
			      mType = INVALID;
			      return;
			    }
			}
			else
			{
				mPosition = LLVector3(REGION_WIDTH_METERS/2, REGION_WIDTH_METERS/2, 0);
			}
		}
	}
}
LLSLURL::LLSLURL(const std::string& grid,
				 const std::string& region)
{
	mGrid = grid;
	mRegion = region;
	mType = LOCATION;
	mPosition = LLVector3((F64)REGION_WIDTH_METERS/2, (F64)REGION_WIDTH_METERS/2, 0);
}
LLSLURL::LLSLURL(const std::string& grid,
		 const std::string& region,
		 const LLVector3& position)
{
	mGrid = grid;
	mRegion = region;
	S32 x = ll_round( (F32)position[VX] );
	S32 y = ll_round( (F32)position[VY] );
	S32 z = ll_round( (F32)position[VZ] );
	mType = LOCATION;
	mPosition = LLVector3(x, y, z);
}
LLSLURL::LLSLURL(const std::string& region,
		 const LLVector3& position)
{
  *this = LLSLURL(gHippoGridManager->getCurrentGridNick(),
		  region, position);
}
LLSLURL::LLSLURL(const std::string& grid,
		 const std::string& region,
		 const LLVector3d& global_position)
{
	HippoGridInfo* gridp = gHippoGridManager->getGrid(grid);
	LLVector3 pos(global_position);
	if (LLSimInfo* sim = LLWorldMap::getInstance()->simInfoFromPosGlobal(global_position))
	{
		pos[VX] = fmod(pos[VX], sim->getSizeX());
		pos[VY] = fmod(pos[VY], sim->getSizeY());
	}
	*this = LLSLURL(gridp ? gridp->getGridNick() : gHippoGridManager->getDefaultGridNick(),
		  region, pos);
}
LLSLURL::LLSLURL(const std::string& region,
		 const LLVector3d& global_position)
{
  *this = LLSLURL(gHippoGridManager->getCurrentGridNick(),
		  region, global_position);
}
LLSLURL::LLSLURL(const std::string& command, const LLUUID&id, const std::string& verb)
{
  mType = APP;
  mAppCmd = command;
  mAppPath = LLSD::emptyArray();
  mAppPath.append(LLSD(id));
  mAppPath.append(LLSD(verb));
}
std::string LLSLURL::getSLURLString() const
{
	switch(mType)
	{
		case HOME_LOCATION:
			return SIM_LOCATION_HOME;
		case LAST_LOCATION:
			return SIM_LOCATION_LAST;
		case LOCATION:
			{
				S32 x = ll_round( (F32)mPosition[VX] );
				S32 y = ll_round( (F32)mPosition[VY] );
				S32 z = ll_round( (F32)mPosition[VZ] );
				std::string fixed_slurl;
				if(gHippoGridManager->getCurrentGrid()->isSecondLife())
				{
					if(gHippoGridManager->getCurrentGrid()->isInProductionGrid())
						fixed_slurl = MAIN_GRID_SLURL_BASE;
					else
						fixed_slurl = llformat(SYSTEM_GRID_SLURL_BASE, gHippoGridManager->getCurrentGridNick().c_str());
				}
				else
					fixed_slurl = llformat(DEFAULT_SLURL_BASE, gHippoGridManager->getCurrentGridNick().c_str());
				return fixed_slurl +
				LLURI::escape(mRegion) + llformat("/%d/%d/%d",x,y,z);
			}
		case APP:
		{
			std::ostringstream app_url;
			if(gHippoGridManager->getCurrentGrid()->isSecondLife())
				app_url << SYSTEM_GRID_APP_SLURL_BASE << "/" << mAppCmd;
			else
				app_url << llformat(DEFAULT_APP_SLURL_BASE, gHippoGridManager->getCurrentGridNick().c_str()) << "/" << mAppCmd;
			for(auto i = mAppPath.beginArray();
				i != mAppPath.endArray();
			    ++i)
			{
				app_url << "/" << i->asString();
			}
			if(mAppQuery.length() > 0)
			{
				app_url << "?" << mAppQuery;
			}
			return app_url.str();
		}
		default:
			LL_WARNS("AppInit") << "Unexpected SLURL type for SLURL string" << (int)mType << LL_ENDL;
			return std::string();
	}
}
std::string LLSLURL::getLoginString() const
{
	std::stringstream unescaped_start;
	switch(mType)
	{
		case LOCATION:
			unescaped_start << "uri:"
			<< mRegion << "&"
			<< ll_round(mPosition[0]) << "&"
			<< ll_round(mPosition[1]) << "&"
			<< ll_round(mPosition[2]);
			break;
		case HOME_LOCATION:
			unescaped_start << "home";
			break;
		case LAST_LOCATION:
			unescaped_start << "last";
			break;
		default:
			LL_WARNS("AppInit") << "Unexpected SLURL type ("<<(int)mType <<")for login string"<< LL_ENDL;
			break;
	}
	return  xml_escape_string(unescaped_start.str());
}
bool LLSLURL::operator==(const LLSLURL& rhs)
{
	if(rhs.mType != mType) return false;
	switch(mType)
	{
		case LOCATION:
			return ((mGrid == rhs.mGrid) &&
					(mRegion == rhs.mRegion) &&
					(mPosition == rhs.mPosition));
		case APP:
			return getSLURLString() == rhs.getSLURLString();
		case HOME_LOCATION:
		case LAST_LOCATION:
			return true;
		default:
			return false;
	}
}
bool LLSLURL::operator !=(const LLSLURL& rhs)
{
	return !(*this == rhs);
}
std::string LLSLURL::getLocationString() const
{
	return llformat("%s/%d/%d/%d",
					mRegion.c_str(),
					(int)ll_round(mPosition[0]),
					(int)ll_round(mPosition[1]),
					(int)ll_round(mPosition[2]));
}
const std::string LLSLURL::typeName[NUM_SLURL_TYPES] =
{
	"INVALID",
	"LOCATION",
	"HOME_LOCATION",
	"LAST_LOCATION",
	"APP",
	"HELP"
};
std::string LLSLURL::getTypeString(SLURL_TYPE type)
{
	std::string name;
	if ( type >= INVALID && type < NUM_SLURL_TYPES )
	{
		name = LLSLURL::typeName[type];
	}
	else
	{
		name = llformat("Out of Range (%d)",type);
	}
	return name;
}
std::string LLSLURL::asString() const
{
    std::ostringstream result;
    result
		<< "   mType: " << LLSLURL::getTypeString(mType)
		<< "   mGrid: " + getGrid()
		<< "   mRegion: " + getRegion()
		<< "   mPosition: " << mPosition
		<< "   mAppCmd:"  << getAppCmd()
		<< "   mAppPath:" + getAppPath().asString()
		<< "   mAppQueryMap:" + getAppQueryMap().asString()
		<< "   mAppQuery: " + getAppQuery()
		;
    return result.str();
}
