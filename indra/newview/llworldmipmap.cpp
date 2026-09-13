/** 
 * @file llworldmipmap.cpp
 * @brief Data storage for the S3 mipmap of the entire world.
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
#include "llworldmipmap.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "math.h"
#include "lfsimfeaturehandler.h"
#define DEBUG_TILES_STAT 0
LLWorldMipmap::LLWorldMipmap() :
	mCurrentLevel(0)
{
}
LLWorldMipmap::~LLWorldMipmap()
{
	reset();
}
void LLWorldMipmap::reset()
{
	for (int level = 0; level < MAP_LEVELS; level++)
	{
		mWorldObjectsMipMap[level].clear();
	}
}
void LLWorldMipmap::equalizeBoostLevels()
{
#if DEBUG_TILES_STAT
	S32 nb_missing = 0;
	S32 nb_tiles = 0;
	S32 nb_visible = 0;
#endif
	for (S32 level = 0; level < MAP_LEVELS; level++)
	{
		sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level];
		for (sublevel_tiles_t::iterator iter = level_mipmap.begin(); iter != level_mipmap.end(); iter++)
		{
			LLPointer<LLViewerFetchedTexture> img = iter->second;
			S32 current_boost_level = img->getBoostLevel();
			if (current_boost_level == LLGLTexture::BOOST_MAP_VISIBLE)
			{
				img->setBoostLevel(LLGLTexture::BOOST_MAP);
			}
			else
			{
				img->setBoostLevel(LLGLTexture::BOOST_NONE);
			}
#if DEBUG_TILES_STAT
			nb_tiles++;
			if (current_boost_level == LLGLTexture::BOOST_MAP_VISIBLE)
			{
				nb_visible++;
			}
			if (img->isMissingAsset())
			{
				nb_missing++;
			}
#endif
		}
	}
#if DEBUG_TILES_STAT
	LL_INFOS("World Map") << "LLWorldMipmap tile stats : total requested = " << nb_tiles << ", visible = " << nb_visible << ", missing = " << nb_missing << LL_ENDL;
#endif
}
void LLWorldMipmap::dropBoostLevels()
{
	for (S32 level = 0; level < MAP_LEVELS; level++)
	{
		sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level];
		for (sublevel_tiles_t::iterator iter = level_mipmap.begin(); iter != level_mipmap.end(); iter++)
		{
			LLPointer<LLViewerFetchedTexture> img = iter->second;
			img->setBoostLevel(LLGLTexture::BOOST_NONE);
		}
	}
}
LLPointer<LLViewerFetchedTexture> LLWorldMipmap::getObjectsTile(U32 grid_x, U32 grid_y, S32 level, bool load)
{
	llassert(level <= MAP_LEVELS);
	llassert(level >= 1);
	if (load && (level != mCurrentLevel))
	{
		cleanMissedTilesFromLevel(level);
		mCurrentLevel = level;
	}
	U64 handle = convertGridToHandle(grid_x, grid_y);
	sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level-1];
	sublevel_tiles_t::iterator found = level_mipmap.find(handle);
	if (found == level_mipmap.end())
	{
		if (load)
		{
			LLPointer<LLViewerFetchedTexture> img = loadObjectsTile(grid_x, grid_y, level);
			level_mipmap.insert(sublevel_tiles_t::value_type( handle, img ));
			found = level_mipmap.find(handle);
		}
		else
		{
			return NULL;
		}
	}
	LLPointer<LLViewerFetchedTexture> img = found->second;
	if (img->isMissingAsset())
	{
		return NULL;
	}
	else
	{
		if (load)
		{
			img->setBoostLevel(LLGLTexture::BOOST_MAP_VISIBLE);
		}
		return img;
	}
}
LLPointer<LLViewerFetchedTexture> LLWorldMipmap::loadObjectsTile(U32 grid_x, U32 grid_y, S32 level)
{
	std::string simOverrideMap = LFSimFeatureHandler::instance().mapServerURL();
	std::string imageurl = (simOverrideMap.empty() ? gSavedSettings.getString("MapServerURL") : simOverrideMap)
		+ llformat("map-%d-%d-%d-objects.jpg", level, grid_x, grid_y);
	LLPointer<LLViewerFetchedTexture> img = LLViewerTextureManager::getFetchedTextureFromUrl(imageurl, FTT_MAP_TILE, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	img->setBoostLevel(LLGLTexture::BOOST_MAP);
	return img;
}
void LLWorldMipmap::cleanMissedTilesFromLevel(S32 level)
{
	llassert(level <= MAP_LEVELS);
	llassert(level >= 0);
	if (level == 0)
	{
		return;
	}
	sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level-1];
	sublevel_tiles_t::iterator it = level_mipmap.begin();
	while (it != level_mipmap.end())
	{
		LLPointer<LLViewerFetchedTexture> img = it->second;
		if (img->isMissingAsset())
		{
			level_mipmap.erase(it++);
		}
		else
		{
			++it;
		}
	}
	return;
}
S32 LLWorldMipmap::scaleToLevel(F32 scale)
{
	if (scale <= F32_MIN)
		return MAP_LEVELS;
	S32 level = llfloor((log(REGION_WIDTH_METERS/scale)/log(2.0f)) + 1.0f);
	if (level > MAP_LEVELS)
		return MAP_LEVELS;
	else if (level < 1)
		return 1;
	else
		return level;
}
void LLWorldMipmap::globalToMipmap(F64 global_x, F64 global_y, S32 level, U32* grid_x, U32* grid_y)
{
	llassert(level <= MAP_LEVELS);
	llassert(level >= 1);
	*grid_x = lltrunc(global_x/REGION_WIDTH_METERS);
	*grid_y = lltrunc(global_y/REGION_WIDTH_METERS);
	S32 regions_in_tile = 1 << (level - 1);
	*grid_x = *grid_x - (*grid_x % regions_in_tile);
	*grid_y = *grid_y - (*grid_y % regions_in_tile);
}
