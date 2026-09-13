/** 
 * @file llworldmipmap_test.cpp
 * @author Merov Linden
 * @date 2009-02-03
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "../llviewerprecompiledheaders.h"
#include "../llworldmipmap.h"
#include "../llviewerimagelist.h"
#include "../../llxml/llcontrol.h"
#include "../test/lltut.h"
LLViewerImageList::LLViewerImageList() { }
LLViewerImageList::~LLViewerImageList() { }
LLViewerImageList gImageList;
LLViewerImage* LLViewerImageList::getImageFromUrl(const std::string& url,
												   BOOL usemipmaps,
												   BOOL level_immediate,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   const LLUUID& force_id)
{ return NULL; }
void LLViewerImage::setBoostLevel(S32 level) { }
LLControlGroup::LLControlGroup() { }
LLControlGroup::~LLControlGroup() { }
std::string	LLControlGroup::getString(const std::string& name) { return LLStringUtil::null; }
std::string	LLControlGroup::getText(const std::string& name) { return LLStringUtil::null; }
BOOL		LLControlGroup::getBOOL(const std::string& name) { return FALSE; }
S32			LLControlGroup::getS32(const std::string& name) { return 0;}
F32			LLControlGroup::getF32(const std::string& name) { return 0.f; }
U32			LLControlGroup::getU32(const std::string& name) { return 0; }
LLControlGroup gSavedSettings;
namespace tut
{
	struct worldmipmap_test
	{
		class LLTestWorldMipmap : public LLWorldMipmap
		{
		};
		LLTestWorldMipmap* mMap;
		worldmipmap_test()
		{
			mMap = new LLTestWorldMipmap;
		}
		~worldmipmap_test()
		{
			delete mMap;
		}
	};
	typedef test_group<worldmipmap_test> worldmipmap_t;
	typedef worldmipmap_t::object worldmipmap_object_t;
	tut::worldmipmap_t tut_worldmipmap("worldmipmap");
	template<> template<>
	void worldmipmap_object_t::test<1>()
	{
		S32 level = mMap->scaleToLevel(0.0);
		ensure("scaleToLevel() test 1 failed", level == LLWorldMipmap::MAP_LEVELS);
		level = mMap->scaleToLevel(LLWorldMipmap::MAP_TILE_SIZE);
		ensure("scaleToLevel() test 2 failed", level == 1);
		level = mMap->scaleToLevel(10 * LLWorldMipmap::MAP_TILE_SIZE);
		ensure("scaleToLevel() test 3 failed", level == 1);
	}
	template<> template<>
	void worldmipmap_object_t::test<2>()
	{
		U32 grid_x, grid_y;
		mMap->globalToMipmap(1000.f*REGION_WIDTH_METERS, 1000.f*REGION_WIDTH_METERS, 1, &grid_x, &grid_y);
		ensure("globalToMipmap() test 1 failed", (grid_x == 1000) && (grid_y == 1000));
		mMap->globalToMipmap(0.0, 0.0, LLWorldMipmap::MAP_LEVELS, &grid_x, &grid_y);
		ensure("globalToMipmap() test 2 failed", (grid_x == 0) && (grid_y == 0));
	}
	template<> template<>
	void worldmipmap_object_t::test<3>()
	{
	}
	template<> template<>
	void worldmipmap_object_t::test<4>()
	{
		try
		{
			mMap->equalizeBoostLevels();
		}
		catch (...)
		{
			fail("equalizeBoostLevels() test failed");
		}
	}
	template<> template<>
	void worldmipmap_object_t::test<5>()
	{
		try
		{
			mMap->dropBoostLevels();
		}
		catch (...)
		{
			fail("dropBoostLevels() test failed");
		}
	}
	template<> template<>
	void worldmipmap_object_t::test<6>()
	{
		try
		{
			mMap->reset();
		}
		catch (...)
		{
			fail("reset() test failed");
		}
	}
}
