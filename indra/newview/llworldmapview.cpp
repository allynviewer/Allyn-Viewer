/** 
 * @file llworldmapview.cpp
 * @brief LLWorldMapView class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "llworldmapview.h"
#include "indra_constants.h"
#include "llui.h"
#include "lllocalcliprect.h"
#include "llmath.h"
#include "llregionhandle.h"
#include "lleventflags.h"
#include "llrender.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llcallingcard.h"
#include "llcolorscheme.h"
#include "llviewercontrol.h"
#include "llcylinder.h"
#include "llfloaterdirectory.h"
#include "llfloatermap.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "lltextbox.h"
#include "lltextureview.h"
#include "lltracker.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworldmap.h"
#include "llworldmipmap.h"
#include "lltexturefetch.h"
#include "llappviewer.h"
#include "lltrans.h"
#include "hippogridmanager.h"
#include "rlvhandler.h"
#include "llglheaders.h"
const F32 OCEAN_RED   = (F32)(0x1D)/255.f;
const F32 OCEAN_GREEN = (F32)(0x47)/255.f;
const F32 OCEAN_BLUE  = (F32)(0x5F)/255.f;
const F32 GODLY_TELEPORT_HEIGHT = 200.f;
const S32 SCROLL_HINT_WIDTH = 65;
const F32 BIG_DOT_RADIUS = 5.f;
BOOL LLWorldMapView::sHandledLastClick = FALSE;
LLUIImagePtr LLWorldMapView::sAvatarSmallImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarYouImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarYouLargeImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarLevelImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarAboveImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarBelowImage = NULL;
LLUIImagePtr LLWorldMapView::sTelehubImage = NULL;
LLUIImagePtr LLWorldMapView::sInfohubImage = NULL;
LLUIImagePtr LLWorldMapView::sHomeImage = NULL;
LLUIImagePtr LLWorldMapView::sEventImage = NULL;
LLUIImagePtr LLWorldMapView::sEventMatureImage = NULL;
LLUIImagePtr LLWorldMapView::sEventAdultImage = NULL;
LLUIImagePtr LLWorldMapView::sTrackCircleImage = NULL;
LLUIImagePtr LLWorldMapView::sTrackArrowImage = NULL;
LLUIImagePtr LLWorldMapView::sClassifiedsImage = NULL;
LLUIImagePtr LLWorldMapView::sForSaleImage = NULL;
LLUIImagePtr LLWorldMapView::sForSaleAdultImage = NULL;
F32 LLWorldMapView::sPanX = 0.f;
F32 LLWorldMapView::sPanY = 0.f;
F32 LLWorldMapView::sTargetPanX = 0.f;
F32 LLWorldMapView::sTargetPanY = 0.f;
S32 LLWorldMapView::sTrackingArrowX = 0;
S32 LLWorldMapView::sTrackingArrowY = 0;
F32 LLWorldMapView::sPixelsPerMeter = 1.f;
bool LLWorldMapView::sVisibleTilesLoaded = false;
F32 LLWorldMapView::sMapScale = 128.f;
F32 CONE_SIZE = 0.6f;
std::map<std::string,std::string> LLWorldMapView::sStringsMap;
const F32 DRAW_TEXT_THRESHOLD = 96.f;
const S32 DRAW_SIMINFO_THRESHOLD = 3;
const int SIM_MAP_AGENT_SCALE=32;
const int SIM_DATA_SCALE=32;
const int SIM_LANDFORSALE_SCALE=32;
const int SIM_COMPOSITE_SCALE=2;
const int SIM_FETCH_SCALE=12;
#define DEBUG_DRAW_TILE 0
void LLWorldMapView::initClass()
{
	sAvatarSmallImage = 	LLUI::getUIImage("map_avatar_8.tga");
	sAvatarYouImage =		LLUI::getUIImage("map_avatar_16.tga");
	sAvatarYouLargeImage =	LLUI::getUIImage("map_avatar_you_32.tga");
	sAvatarLevelImage = 	LLUI::getUIImage("map_avatar_32.tga");
	sAvatarAboveImage = 	LLUI::getUIImage("map_avatar_above_32.tga");
	sAvatarBelowImage = 	LLUI::getUIImage("map_avatar_below_32.tga");
	sHomeImage =			LLUI::getUIImage("map_home.tga");
	sTelehubImage = 		LLUI::getUIImage("map_telehub.tga");
	sInfohubImage = 		LLUI::getUIImage("map_infohub.tga");
	sEventImage =			LLUI::getUIImage("Parcel_PG_Light");
	sEventMatureImage =		LLUI::getUIImage("Parcel_M_Light");
	sEventAdultImage =		LLUI::getUIImage("Parcel_R_Light");
	sTrackCircleImage =		LLUI::getUIImage("map_track_16.tga");
	sTrackArrowImage =		LLUI::getUIImage("direction_arrow.tga");
	sClassifiedsImage =		LLUI::getUIImage("icon_top_pick.tga");
	sForSaleImage =			LLUI::getUIImage("icon_for_sale.tga");
	sForSaleAdultImage =    LLUI::getUIImage("icon_for_sale_adult.tga");
	sStringsMap["loading"] = LLTrans::getString("texture_loading");
	sStringsMap["offline"] = LLTrans::getString("worldmap_offline");
}
void LLWorldMapView::cleanupClass()
{
	sAvatarSmallImage = NULL;
	sAvatarYouImage = NULL;
	sAvatarYouLargeImage = NULL;
	sAvatarLevelImage = NULL;
	sAvatarAboveImage = NULL;
	sAvatarBelowImage = NULL;
	sTelehubImage = NULL;
	sInfohubImage = NULL;
	sHomeImage = NULL;
	sEventImage = NULL;
	sEventMatureImage = NULL;
	sEventAdultImage = NULL;
	sTrackCircleImage = NULL;
	sTrackArrowImage = NULL;
	sClassifiedsImage = NULL;
	sForSaleImage = NULL;
	sForSaleAdultImage = NULL;
}
LLWorldMapView::LLWorldMapView(const std::string& name, const LLRect& rect )
:	LLPanel(name, rect, BORDER_NO),
	mBackgroundColor( LLColor4( OCEAN_RED, OCEAN_GREEN, OCEAN_BLUE, 1.f ) ),
	mItemPicked(FALSE),
	mPanning( FALSE ),
	mMouseDownPanX( 0 ),
	mMouseDownPanY( 0 ),
	mMouseDownX( 0 ),
	mMouseDownY( 0 ),
	mSelectIDStart(0)
{
	sPixelsPerMeter = sMapScale / REGION_WIDTH_METERS;
	clearLastClick();
	const S32 DIR_WIDTH = 10;
	const S32 DIR_HEIGHT = 10;
	LLRect major_dir_rect(  0, DIR_HEIGHT, DIR_WIDTH, 0 );
	mTextBoxNorth = new LLTextBox( std::string("N"), major_dir_rect );
	addChild( mTextBoxNorth );
	LLColor4 minor_color( 1.f, 1.f, 1.f, .7f );
	mTextBoxEast =	new LLTextBox( std::string("E"), major_dir_rect );
	mTextBoxEast->setColor( minor_color );
	addChild( mTextBoxEast );
	major_dir_rect.mRight += 1 ;
	mTextBoxWest =	new LLTextBox( std::string("W"), major_dir_rect );
	mTextBoxWest->setColor( minor_color );
	addChild( mTextBoxWest );
	major_dir_rect.mRight -= 1 ;
	mTextBoxSouth = new LLTextBox( std::string("S"), major_dir_rect );
	mTextBoxSouth->setColor( minor_color );
	addChild( mTextBoxSouth );
	LLRect minor_dir_rect(  0, DIR_HEIGHT, DIR_WIDTH * 2, 0 );
	mTextBoxSouthEast =	new LLTextBox( std::string("SE"), minor_dir_rect );
	mTextBoxSouthEast->setColor( minor_color );
	addChild( mTextBoxSouthEast );
	mTextBoxNorthEast = new LLTextBox( std::string("NE"), minor_dir_rect );
	mTextBoxNorthEast->setColor( minor_color );
	addChild( mTextBoxNorthEast );
	mTextBoxSouthWest =	new LLTextBox( std::string("SW"), minor_dir_rect );
	mTextBoxSouthWest->setColor( minor_color );
	addChild( mTextBoxSouthWest );
	mTextBoxNorthWest = new LLTextBox( std::string("NW"), minor_dir_rect );
	mTextBoxNorthWest->setColor( minor_color );
	addChild( mTextBoxNorthWest );
}
LLWorldMapView::~LLWorldMapView()
{
	cleanupTextures();
}
void LLWorldMapView::cleanupTextures()
{
}
void LLWorldMapView::setScale( F32 scale )
{
	if(!LLWorldMap::useWebMapTiles())
	{
		scale = SIM_COMPOSITE_SCALE + scale*((256.f-SIM_COMPOSITE_SCALE)/256.f);
	}
	if (scale != sMapScale)
	{
		F32 old_scale = sMapScale;
		sMapScale = scale;
		if (sMapScale <= 0.f)
		{
			sMapScale = 0.1f;
		}
		F32 ratio = (scale / old_scale);
		sPanX *= ratio;
		sPanY *= ratio;
		sTargetPanX = sPanX;
		sTargetPanY = sPanY;
		sPixelsPerMeter = sMapScale / REGION_WIDTH_METERS;
		sVisibleTilesLoaded = false;
	}
}
void LLWorldMapView::translatePan( S32 delta_x, S32 delta_y )
{
	sPanX += delta_x;
	sPanY += delta_y;
	sTargetPanX = sPanX;
	sTargetPanY = sPanY;
	sVisibleTilesLoaded = false;
}
void LLWorldMapView::setPan( S32 x, S32 y, BOOL snap )
{
	sTargetPanX = (F32)x;
	sTargetPanY = (F32)y;
	if (snap)
	{
		sPanX = sTargetPanX;
		sPanY = sTargetPanY;
	}
	sVisibleTilesLoaded = false;
}
bool LLWorldMapView::showRegionInfo()
{
	return (LLWorldMipmap::scaleToLevel(sMapScale) <= DRAW_SIMINFO_THRESHOLD ? true : false);
}
BOOL is_agent_in_region(LLViewerRegion* region, LLSimInfo* info)
{
	return (region && info && info->isName(region->getName()));
}
void LLWorldMapView::draw()
{
	LLTextureView::clearDebugImages();
	F64 current_time = LLTimer::getElapsedSeconds();
	mVisibleRegions.clear();
	sPanX = lerp(sPanX, sTargetPanX, LLSmoothInterpolation::getInterpolant(0.1f));
	sPanY = lerp(sPanY, sTargetPanY, LLSmoothInterpolation::getInterpolant(0.1f));
	const S32 width = getRect().getWidth();
	const S32 height = getRect().getHeight();
	const F32 half_width = F32(width) / 2.0f;
	const F32 half_height = F32(height) / 2.0f;
	LLVector3d camera_global = gAgentCamera.getCameraPositionGlobal();
	LLLocalClipRect clip(getLocalRect());
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.setColorMask(false, true);
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.f);
		gGL.setSceneBlendType(LLRender::BT_REPLACE);
		gGL.color4f(0.0f, 0.0f, 0.0f, 0.0f);
		gl_rect_2d(0, height, width, 0);
	}
	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	gGL.setColorMask(true, true);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	static const LLCachedControl<bool> map_show_land_for_sale("MapShowLandForSale");
	if(LLWorldMap::useWebMapTiles())
		drawMipmap(width, height);
	else
		drawLegacyBackgroundLayers(width, height);
	LLGLSUIDefault gls_ui;
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.f);
		gGL.blendFunc(LLRender::BF_ONE_MINUS_DEST_ALPHA, LLRender::BF_DEST_ALPHA);
		gGL.color4fv( mBackgroundColor.mV );
		gl_rect_2d(0, height, width, 0);
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
	}
	for (LLWorldMap::sim_info_map_t::const_iterator it = LLWorldMap::getInstance()->getRegionMap().begin();
		 it != LLWorldMap::getInstance()->getRegionMap().end(); ++it)
	{
		U64 handle = it->first;
		LLSimInfo* info = it->second;
		LLVector3d origin_global = from_region_handle(handle);
		LLVector3d rel_region_pos = origin_global - camera_global;
		F32 relative_x = (rel_region_pos.mdV[0] / REGION_WIDTH_METERS) * sMapScale;
		F32 relative_y = (rel_region_pos.mdV[1] / REGION_WIDTH_METERS) * sMapScale;
		F32 bottom =    sPanY + half_height + relative_y;
		F32 left =      sPanX + half_width + relative_x;
		F32 top =       bottom + sMapScale  * ((F32)info->getSizeY() / REGION_WIDTH_METERS);
		F32 right =     left + sMapScale  * ((F32)info->getSizeX() / REGION_WIDTH_METERS);
		if ((top < 0.f)   || (bottom > height) ||
			(right < 0.f) || (left > width)       )
		{
			info->dropImagePriority();
			continue;
		}
		mVisibleRegions.push_back(handle);
		if (sMapScale >= SIM_MAP_AGENT_SCALE)
		{
			info->updateAgentCount(current_time);
		}
		F32 alpha = !LLWorldMap::useWebMapTiles() ? drawLegacySimTile(*info,left,top,right,bottom) : 1.f;
		if (info->isDown())
		{
			gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_SOURCE_ALPHA);
			gGL.color4f(0.2f, 0.0f, 0.0f, 0.4f);
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.vertex2f(left, top);
				gGL.vertex2f(left, bottom);
				gGL.vertex2f(right, top);
				gGL.vertex2f(right, bottom);
			gGL.end();
			gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
		}
		else if (map_show_land_for_sale && (sMapScale >= SIM_LANDFORSALE_SCALE))
		{
			LLViewerFetchedTexture* overlayimage = info->getLandForSaleImage();
			if (overlayimage)
			{
				S32 draw_size = ll_round(sMapScale);
				overlayimage->setKnownDrawSize(	ll_round(draw_size * LLUI::getScaleFactor().mV[VX] * ((F32)info->getSizeX() / REGION_WIDTH_METERS)),
												ll_round(draw_size * LLUI::getScaleFactor().mV[VY] * ((F32)info->getSizeY() / REGION_WIDTH_METERS)));
				if (overlayimage->hasGLTexture() && !overlayimage->isMissingAsset() && overlayimage->getID() != IMG_DEFAULT)
				{
					gGL.getTexUnit(0)->bind(overlayimage);
					gGL.color4f(1.f, 1.f, 1.f, alpha);
					gGL.begin(LLRender::TRIANGLE_STRIP);
						gGL.texCoord2f(0.f, 1.f);
						gGL.vertex3f(left, top, -0.5f);
						gGL.texCoord2f(0.f, 0.f);
						gGL.vertex3f(left, bottom, -0.5f);
						gGL.texCoord2f(1.f, 1.f);
						gGL.vertex3f(right, top, -0.5f);
						gGL.texCoord2f(1.f, 0.f);
						gGL.vertex3f(right, bottom, -0.5f);
					gGL.end();
				}
			}
		}
		else
		{
			info->dropImagePriority(SIM_LAYER_OVERLAY);
		}
		if (sMapScale >= DRAW_TEXT_THRESHOLD)
		{
			LLFontGL* font = LLFontGL::getFontSansSerifSmall();
			std::string mesg;
			if (info->isDown())
			{
				if(info->getName().empty())
					mesg = llformat( "(%s)", sStringsMap["offline"].c_str());
				else
					mesg = llformat( "%s (%s)", info->getName().c_str(), sStringsMap["offline"].c_str());
			}
			else if(!info->getName().empty())
			{
				mesg = info->getName();
				static const LLCachedControl<bool> show_avs("LiruMapShowAvCount");
				if (show_avs) mesg += llformat(" (%d)", info->getAgentCount());
				mesg += llformat(" (%s)", info->getAccessString().c_str());
			}
			if ( (!mesg.empty()) && (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) )
			{
				font->renderUTF8(
					mesg, 0,
					llfloor(left + 3), llfloor(bottom + 2),
					LLColor4::white,
					LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);
				static const LLCachedControl<bool> show_region_positions("SFMapShowRegionPositions");
				if (show_region_positions)
					font->renderUTF8(
						llformat("(%d, %d)", llfloor(info->getGlobalOrigin().mdV[VX]/256), llfloor(info->getGlobalOrigin().mdV[VY])/256), 0,
						llfloor(left + 3), llfloor(bottom + 14),
						LLColor4::white,
						LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);
			}
		}
	}
	static const LLCachedControl<bool> map_show_requests("MapShowRequests", false);
	if(map_show_requests)
	{
		const F32 map_to_pixels =	MAP_MAX_SIZE*sMapScale;
		const F32 block_to_pixels =	MAP_BLOCK_SIZE*sMapScale;
		const F32 padding =			.25f*sMapScale;
		const F32 centerX =			-sPanX/sMapScale + camera_global.mdV[VX]/REGION_WIDTH_METERS;
		const F32 centerY =			-sPanY/sMapScale + camera_global.mdV[VY]/REGION_WIDTH_METERS;
		const F32 bottom =			centerY - half_height/sMapScale ;
		const F32 left =			centerX - half_width/sMapScale ;
		const F32 top =				centerY + half_height/sMapScale ;
		const F32 right =			centerX + half_width/sMapScale ;
		const U32 max_range =		(U16_MAX+1)/MAP_BLOCK_RES/MAP_BLOCK_SIZE - 1;
		const U32 map_block_x0 =	(U32)(llmax(left,0.f) / MAP_MAX_SIZE);
		const U32 map_block_x1 =	llclamp(U32(right / MAP_MAX_SIZE), U32(0), max_range);
		const U32 map_block_y0 =	(U32)(llmax(bottom,0.f) / MAP_MAX_SIZE);
		const U32 map_block_y1 =	llclamp(U32(top / MAP_MAX_SIZE), U32(0), max_range);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		for(U32 i = map_block_x0; i <= map_block_x1; ++i)
		{
			const F32 base_left = i*map_to_pixels-left*sMapScale;
			const F32 base_right = base_left + map_to_pixels;
			if(	base_right <= 0.f)
				continue;
			if(	base_left >= width)
				break;
			for (U32 j = map_block_y0; j <= map_block_y1; ++j)
			{
				const F32 base_bottom = j*map_to_pixels-bottom*sMapScale;
				const F32 base_top = base_bottom + map_to_pixels;
				if(	base_top <= 0.f)
					continue;
				if(	base_bottom >= height)
					break;
				std::map<U32, std::vector<bool> >::const_iterator it = LLWorldMap::getInstance()->findMapBlock(SIM_LAYER_OVERLAY,i,j);
				if(it != LLWorldMap::getInstance()->getMapBlockEnd(SIM_LAYER_OVERLAY))
					gGL.color4f(0.5f, 0.0f, 0.0f, 0.4f);
				else
					gGL.color4f(0.2f, .2f, 0.2f, 0.4f);
				gGL.begin(LLRender::TRIANGLE_STRIP);
					gGL.vertex2f(base_left+padding, base_top-padding);
					gGL.vertex2f(base_left+padding, base_bottom+padding);
					gGL.vertex2f(base_right - padding, base_top - padding);
					gGL.vertex2f(base_right-padding, base_bottom+padding);
				gGL.end();
				if(it != LLWorldMap::getInstance()->getMapBlockEnd(SIM_LAYER_OVERLAY))
				{
					for(U32 x=0;x<MAP_BLOCK_RES;++x)
					{
						const F32 block_left = base_left + x*block_to_pixels;
						const F32 block_right = block_left + block_to_pixels;
						if( block_right <= 0.f )
							continue;
						if(	block_left >= width)
							break;
						for(U32 y=0;y<MAP_BLOCK_RES;++y)
						{
							const F32 block_bottom = base_bottom + y*block_to_pixels;
							const F32 block_top = block_bottom + block_to_pixels;
							if( block_top <= 0.f )
								continue;
							if(	block_bottom >= height)
								break;
							if(it->second[x | (y * MAP_BLOCK_RES)])
								gGL.color4f(0.0f, 0.5f, 0.0f, 0.4f);
							else
								gGL.color4f(0.4f, 0.0f, 0.0f, 0.4f);
							gGL.begin(LLRender::TRIANGLE_STRIP);
								gGL.vertex2f(block_left+padding, block_top-padding);
								gGL.vertex2f(block_left+padding, block_bottom+padding);
								gGL.vertex2f(block_right - padding, block_top - padding);
								gGL.vertex2f(block_right-padding, block_bottom+padding);
							gGL.end();
							for(U32 sim_x=0;sim_x<MAP_BLOCK_SIZE;++sim_x)
							{
								for(U32 sim_y=0;sim_y<MAP_BLOCK_SIZE;++sim_y)
								{
									U64 handle = grid_to_region_handle(	sim_x+x*MAP_BLOCK_SIZE+i*MAP_MAX_SIZE,
																		sim_y+y*MAP_BLOCK_SIZE+j*MAP_MAX_SIZE );
									LLSimInfo* info = LLWorldMap::instance().simInfoFromHandle(handle);
									if(info)
									{
										const F32 sim_bottom = block_bottom + sim_y*sMapScale + (sim_y ? 0 : padding);
										const F32 sim_top = block_bottom + sim_y*sMapScale + sMapScale - (sim_y<(MAP_BLOCK_SIZE-1) ? 0 : padding);
										const F32 sim_left = block_left + sim_x*sMapScale + (sim_x ? 0 : padding);
										const F32 sim_right = block_left + sim_x*sMapScale + sMapScale - (sim_x<(MAP_BLOCK_SIZE-1) ? 0 : padding);
										gGL.color4f(0.0f, 0.0f, 0.4f, 0.4f);
										gGL.begin(LLRender::TRIANGLE_STRIP);
											gGL.vertex2f(sim_left, sim_top);
											gGL.vertex2f(sim_left, sim_bottom);
											gGL.vertex2f(sim_right, sim_top);
											gGL.vertex2f(sim_right, sim_bottom);
										gGL.end();
									}
								}
							}
						}
					}
				}
			}
		}
	}
	static const LLCachedControl<bool> map_show_infohubs("MapShowInfohubs");
	static const LLCachedControl<bool> map_show_telehubs("MapShowTelehubs");
	static const LLCachedControl<bool> map_show_mature_events("MapShowMatureEvents");
	static const LLCachedControl<bool> map_show_adult_events("MapShowAdultEvents");
	if(sMapScale >= DRAW_TEXT_THRESHOLD && (map_show_infohubs ||
											map_show_telehubs ||
											map_show_land_for_sale ||
											map_show_mature_events ||
											map_show_adult_events))
	{
		drawItems();
	}
	LLVector3d home;
	if (gAgent.getHomePosGlobal(&home))
	{
		drawImage(home, sHomeImage);
	}
	LLVector3d pos_global = gAgent.getPositionGlobal();
	drawImage(pos_global, sAvatarYouImage);
	LLVector3 pos_map = globalPosToView(pos_global);
	if (!pointInView(ll_round(pos_map.mV[VX]), ll_round(pos_map.mV[VY])))
	{
		drawTracking(pos_global,
			lerp(LLColor4::yellow, LLColor4::orange, 0.4f),
			TRUE,
			"You are here",
			"",
			ll_round(LLFontGL::getFontSansSerifSmall()->getLineHeight()));
	}
	drawFrustum();
	static const LLCachedControl<bool> map_show_people("MapShowPeople");
	if (map_show_people && sMapScale >= SIM_MAP_AGENT_SCALE)
	{
		drawAgents();
	}
	LLTracker::ETrackingStatus tracking_status = LLTracker::getTrackingStatus();
	if ( LLTracker::TRACKING_AVATAR == tracking_status )
	{
		drawTracking( LLAvatarTracker::instance().getGlobalPos(), gTrackColor, TRUE, LLTracker::getLabel(), "" );
	}
	else if ( LLTracker::TRACKING_LANDMARK == tracking_status
			 || LLTracker::TRACKING_LOCATION == tracking_status )
	{
		LLVector3d pos_global = LLTracker::getTrackedPositionGlobal();
		if (!pos_global.isExactlyZero())
		{
			drawTracking( pos_global, gTrackColor, TRUE, LLTracker::getLabel(), LLTracker::getToolTip() );
		}
	}
	else if (LLWorldMap::getInstance()->isTracking())
	{
		if (LLWorldMap::getInstance()->isTrackingInvalidLocation())
		{
			LLColor4 loading_color(0.0, 0.5, 1.0, 1.0);
			drawTracking( LLWorldMap::getInstance()->getTrackedPositionGlobal(), loading_color, TRUE, LLTrans::getString("InvalidLocation"), "");
		}
		else
		{
			double value = fmod(current_time, 2);
			value = 0.5 + 0.5*cos(value * F_PI);
			LLColor4 loading_color(0.0, F32(value/2), F32(value), 1.0);
			drawTracking( LLWorldMap::getInstance()->getTrackedPositionGlobal(), loading_color, TRUE, LLTrans::getString("LoadingData"), "");
		}
	}
	if(!LLWorldMap::useWebMapTiles() && (sMapScale < SIM_FETCH_SCALE || sMapScale < SIM_DATA_SCALE))
	{
		LLFontGL::getFontSansSerifSmall()->renderUTF8("(Zoom to resume loading)", 0,
					(F32)(width-2), LLFontGL::getFontSansSerifSmall()->getLineHeight()*1.5f,
					LLColor4::white,
					LLFontGL::RIGHT, LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);
	}
	LLGLDisable<GL_SCISSOR_TEST> no_scissor;
	updateDirections();
	LLView::draw();
	updateVisibleBlocks();
	gGL.flush();
}
void LLWorldMapView::setVisible(BOOL visible)
{
	LLPanel::setVisible(visible);
	if (!visible)
	{
		std::vector<LLWorldMapLayer>::iterator begin = LLWorldMap::getInstance()->getMapLayerBegin();
		std::vector<LLWorldMapLayer>::iterator end = LLWorldMap::getInstance()->getMapLayerEnd();
		for(std::vector<LLWorldMapLayer>::iterator it = begin; it != end; ++it)
		{
			if (it->LayerDefined)
			{
				it->LayerImage->setBoostLevel(0);
			}
		}
		LLWorldMap::getInstance()->dropImagePriorities();
	}
}
void LLWorldMapView::drawLegacyBackgroundLayers(S32 width, S32 height) {
	const F32 half_width = F32(width) / 2.0f;
	const F32 half_height = F32(height) / 2.0f;
	LLVector3d camera_global = gAgentCamera.getCameraPositionGlobal();
	std::vector<LLWorldMapLayer>::const_iterator begin = LLWorldMap::getInstance()->getMapLayerBegin();
	std::vector<LLWorldMapLayer>::const_iterator end = LLWorldMap::getInstance()->getMapLayerEnd();
	for (std::vector<LLWorldMapLayer>::const_iterator it = begin; it != end; ++it)
	{
		if (!it->LayerDefined)
		{
			continue;
		}
		const LLWorldMapLayer& layer = *it;
		LLViewerTexture *current_image = layer.LayerImage;
		if (!current_image || current_image->isMissingAsset() || current_image->getID() == IMG_DEFAULT)
		{
			continue;
		}
		LLVector3d origin_global((F64)layer.LayerExtents.mLeft * REGION_WIDTH_METERS, (F64)layer.LayerExtents.mBottom * REGION_WIDTH_METERS, 0.f);
		LLVector3d rel_region_pos = origin_global - camera_global;
		F32 relative_x = (rel_region_pos.mdV[0] / REGION_WIDTH_METERS) * sMapScale;
		F32 relative_y = (rel_region_pos.mdV[1] / REGION_WIDTH_METERS) * sMapScale;
		F32 pix_width = sMapScale*(layer.LayerExtents.getWidth() + 1);
		F32 pix_height = sMapScale*(layer.LayerExtents.getHeight() + 1);
		F32 bottom =	sPanY + half_height + relative_y;
		F32 left =		sPanX + half_width + relative_x;
		F32 top =		bottom + pix_height;
		F32 right =		left + pix_width;
		F32 pixel_area = pix_width*pix_height;
		if (top < 0.f ||
			bottom > height ||
			right < 0.f ||
			left > width ||
			(pixel_area < 4*4))
		{
			current_image->setBoostLevel(0);
			continue;
		}
		current_image->setBoostLevel(LLGLTexture::BOOST_MAP);
		current_image->setKnownDrawSize(ll_round(pix_width * LLUI::getScaleFactor().mV[VX]),
			ll_round(pix_height * LLUI::getScaleFactor().mV[VY]));
		if (!current_image->hasGLTexture())
		{
			continue;
		}
		gGL.getTexUnit(0)->bind(current_image);
		gGL.setColorMask(true, false);
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		gGL.begin(LLRender::TRIANGLE_STRIP);
			gGL.texCoord2f(0.0f, 1.0f);
			gGL.vertex3f(left, top, -1.0f);
			gGL.texCoord2f(0.0f, 0.0f);
			gGL.vertex3f(left, bottom, -1.0f);
			gGL.texCoord2f(1.0f, 1.0f);
			gGL.vertex3f(right, top, -1.0f);
			gGL.texCoord2f(1.0f, 0.0f);
			gGL.vertex3f(right, bottom, -1.0f);
		gGL.end();
		gGL.setColorMask(false, true);
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		gGL.begin(LLRender::TRIANGLE_STRIP);
			gGL.texCoord2f(0.0f, 1.0f);
			gGL.vertex2f(left, top);
			gGL.texCoord2f(0.0f, 0.0f);
			gGL.vertex2f(left, bottom);
			gGL.texCoord2f(1.0f, 1.0f);
			gGL.vertex2f(right, top);
			gGL.texCoord2f(1.0f, 0.0f);
			gGL.vertex2f(right, bottom);
		gGL.end();
		gGL.setColorMask(true, true);
	}
}
F32 LLWorldMapView::drawLegacySimTile(LLSimInfo& sim_info, S32 left, S32 top, S32 right, S32 bottom)
{
	const F32 ALPHA_CUTOFF = 0.001f;
	const S32 MAX_SIMULTANEOUS_TEX = 100;
	const S32 MAX_REQUEST_PER_TICK = 5;
	const S32 MIN_REQUEST_PER_TICK = 1;
	S32 textures_requested_this_tick = 0;
	LLViewerFetchedTexture* simimage = sim_info.mLayerImage[SIM_LAYER_COMPOSITE];
	const bool sim_visible = sMapScale >= SIM_COMPOSITE_SCALE;
	const bool sim_fetchable = sMapScale >= SIM_FETCH_SCALE;
	if(sim_fetchable)
	{
		if (!simimage && sim_info.mMapImageID[SIM_LAYER_COMPOSITE].notNull() && sim_info.mMapImageID[SIM_LAYER_COMPOSITE] != IMG_DEFAULT)
		{
			if ((textures_requested_this_tick < MIN_REQUEST_PER_TICK) ||
				((LLAppViewer::getTextureFetch()->getNumRequests() < MAX_SIMULTANEOUS_TEX) &&
				 (textures_requested_this_tick < MAX_REQUEST_PER_TICK)))
			{
				textures_requested_this_tick++;
				simimage = sim_info.mLayerImage[SIM_LAYER_COMPOSITE] = LLViewerTextureManager::getFetchedTexture(sim_info.mMapImageID[SIM_LAYER_COMPOSITE], FTT_MAP_TILE, MIPMAP_TRUE, LLGLTexture::BOOST_MAP, LLViewerTexture::LOD_TEXTURE);
	            simimage->setAddressMode(LLTexUnit::TAM_CLAMP);
			}
		}
	}
	const bool sim_drawable = simimage && !simimage->isMissingAsset() && simimage->getID() != IMG_DEFAULT;
	const bool sim_fetching = sim_drawable && !simimage->hasGLTexture();
	const F32 fade_target = sim_visible ? 1.f : 0.f;
	if (!sim_drawable || sim_fetching)
		sim_info.setAlpha( 0.f );
	else if (llabs(sim_info.getAlpha() - fade_target) > ALPHA_CUTOFF)
		sim_info.setAlpha(lerp(sim_info.getAlpha(), fade_target, LLSmoothInterpolation::getInterpolant(0.15f)));
	F32 alpha = sim_info.getAlpha();
	if(sim_fetching || alpha >= ALPHA_CUTOFF)
	{
		S32 draw_size = ll_round(sMapScale);
		simimage->setKnownDrawSize(	ll_round(draw_size * LLUI::getScaleFactor().mV[VX] * ((F32)sim_info.getSizeX() / REGION_WIDTH_METERS)),
									ll_round(draw_size * LLUI::getScaleFactor().mV[VY] * ((F32)sim_info.getSizeY() / REGION_WIDTH_METERS)));
		simimage->setBoostLevel(LLGLTexture::BOOST_MAP);
		if(alpha >= ALPHA_CUTOFF)
		{
			gGL.getTexUnit(0)->bind(simimage);
			gGL.color4f(1.f, 1.f, 1.f, alpha);
			gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.texCoord2f(0.f, 1.f);
				gGL.vertex2f(left, top);
				gGL.texCoord2f(0.f, 0.f);
				gGL.vertex2f(left, bottom);
				gGL.texCoord2f(1.f, 1.f);
				gGL.vertex2f(right, top);
				gGL.texCoord2f(1.f, 0.f);
				gGL.vertex2f(right, bottom);
			gGL.end();
		}
	}
	else
	{
		sim_info.dropImagePriority(SIM_LAYER_COMPOSITE);
	}
	return alpha;
}
void LLWorldMapView::drawMipmap(S32 width, S32 height)
{
	S32 level = LLWorldMipmap::scaleToLevel(sMapScale);
	LLWorldMap::getInstance()->equalizeBoostLevels();
	if (!sVisibleTilesLoaded)
	{
		for (S32 l = LLWorldMipmap::MAP_LEVELS; l > level; l--)
		{
			drawMipmapLevel(width, height, l, false);
		}
		if (level > 1)
		{
			drawMipmapLevel(width, height, level - 1, false);
		}
	}
	else
	{
	}
	sVisibleTilesLoaded = drawMipmapLevel(width, height, level);
	return;
}
bool LLWorldMapView::drawMipmapLevel(S32 width, S32 height, S32 level, bool load)
{
	llassert (level > 0);
	if (level <= 0)
		return false;
	S32 completed_tiles = 0;
	S32 total_tiles = 0;
	S32 tile_width = LLWorldMipmap::MAP_TILE_SIZE * (1 << (level - 1));
	LLVector3d pos_SW = viewPosToGlobal(0, 0);
	LLVector3d pos_NE = viewPosToGlobal(width, height);
	pos_NE[VX] += tile_width;
	pos_NE[VY] += tile_width;
	U32 grid_x, grid_y;
	for (F64 index_y = pos_SW[VY]; index_y < pos_NE[VY]; index_y += tile_width)
	{
		for (F64 index_x = pos_SW[VX]; index_x < pos_NE[VX]; index_x += tile_width)
		{
			LLVector3d pos_global(index_x, index_y, pos_SW[VZ]);
			LLWorldMipmap::globalToMipmap(pos_global[VX], pos_global[VY], level, &grid_x, &grid_y);
			LLPointer<LLViewerFetchedTexture> simimage = LLWorldMap::getInstance()->getObjectsTile(grid_x, grid_y, level, load);
			if (simimage)
			{
				if (simimage->hasGLTexture())
				{
					completed_tiles++;
					pos_global[VX] = grid_x * REGION_WIDTH_METERS;
					pos_global[VY] = grid_y * REGION_WIDTH_METERS;
					LLVector3 pos_screen = globalPosToView (pos_global);
					F32 left   = pos_screen[VX];
					F32 bottom = pos_screen[VY];
					{
						pos_global[VX] += tile_width;
						pos_global[VY] += tile_width;
					}
					pos_screen = globalPosToView (pos_global);
					F32 right  = pos_screen[VX];
					F32 top    = pos_screen[VY];
					LLGLSUIDefault gls_ui;
					gGL.getTexUnit(0)->bind(simimage.get());
					simimage->setAddressMode(LLTexUnit::TAM_CLAMP);
					gGL.color4f(1.f, 1.0f, 1.0f, 1.0f);
					gGL.begin(LLRender::TRIANGLE_STRIP);
						gGL.texCoord2f(0.f, 1.f);
						gGL.vertex3f(left, top, 0.f);
						gGL.texCoord2f(0.f, 0.f);
						gGL.vertex3f(left, bottom, 0.f);
						gGL.texCoord2f(1.f, 1.f);
						gGL.vertex3f(right, top, 0.f);
						gGL.texCoord2f(1.f, 0.f);
						gGL.vertex3f(right, bottom, 0.f);
					gGL.end();
#if DEBUG_DRAW_TILE
					drawTileOutline(level, top, left, bottom, right);
#endif
				}
			}
			else
			{
				completed_tiles++;
			}
			total_tiles++;
		}
	}
	return (completed_tiles == total_tiles);
}
void LLWorldMapView::drawTileOutline(S32 level, F32 top, F32 left, F32 bottom, F32 right)
{
	gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_ZERO);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	if (level == 1)
		gGL.color3f(1.f, 0.f, 0.f);
	else if (level == 2)
		gGL.color3f(0.f, 1.f, 0.f);
	else if (level == 3)
		gGL.color3f(0.f, 0.f, 1.f);
	else if (level == 4)
		gGL.color3f(1.f, 1.f, 0.f);
	else if (level == 5)
		gGL.color3f(1.f, 0.f, 1.f);
	else if (level == 6)
		gGL.color3f(0.f, 1.f, 1.f);
	else if (level == 7)
		gGL.color3f(1.f, 1.f, 1.f);
	else
		gGL.color3f(0.f, 0.f, 0.f);
	gGL.begin(LLRender::LINE_STRIP);
		gGL.vertex2f(left, top);
		gGL.vertex2f(right, bottom);
		gGL.vertex2f(left, bottom);
		gGL.vertex2f(right, top);
		gGL.vertex2f(left, top);
		gGL.vertex2f(left, bottom);
		gGL.vertex2f(right, bottom);
		gGL.vertex2f(right, top);
	gGL.end();
	gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
}
void LLWorldMapView::drawGenericItems(const LLSimInfo::item_info_list_t& items, LLUIImagePtr image)
{
	LLSimInfo::item_info_list_t::const_iterator e;
	for (e = items.begin(); e != items.end(); ++e)
	{
		drawGenericItem(*e, image);
	}
}
void LLWorldMapView::drawGenericItem(const LLItemInfo& item, LLUIImagePtr image)
{
	drawImage(item.getGlobalPosition(), image);
}
void LLWorldMapView::drawImage(const LLVector3d& global_pos, LLUIImagePtr image, const LLColor4& color)
{
	LLVector3 pos_map = globalPosToView( global_pos );
	image->draw(ll_round(pos_map.mV[VX] - image->getWidth() /2.f),
				ll_round(pos_map.mV[VY] - image->getHeight()/2.f),
				color);
}
void LLWorldMapView::drawImageStack(const LLVector3d& global_pos, LLUIImagePtr image, U32 count, F32 offset, const LLColor4& color)
{
	LLVector3 pos_map = globalPosToView( global_pos );
	for(U32 i=0; i<count; i++)
	{
		image->draw(ll_round(pos_map.mV[VX] - image->getWidth() /2.f),
					ll_round(pos_map.mV[VY] - image->getHeight()/2.f + i*offset),
					color);
	}
}
void LLWorldMapView::drawItems()
{
	bool mature_enabled = gAgent.canAccessMature();
	bool adult_enabled = gAgent.canAccessAdult();
	static const LLCachedControl<bool> map_show_infohubs("MapShowInfohubs");
	static const LLCachedControl<bool> map_show_telehubs("MapShowTelehubs");
	static const LLCachedControl<bool> map_show_pg_events("MapShowPGEvents");
	static const LLCachedControl<bool> map_show_mature_events("MapShowMatureEvents");
	static const LLCachedControl<bool> map_show_adult_events("MapShowAdultEvents");
	static const LLCachedControl<bool> map_show_land_for_sale("MapShowLandForSale");
	BOOL show_mature = mature_enabled && map_show_mature_events;
	BOOL show_adult = adult_enabled && map_show_adult_events;
	for (handle_list_t::iterator iter = mVisibleRegions.begin(); iter != mVisibleRegions.end(); ++iter)
	{
		U64 handle = *iter;
		LLSimInfo* info = LLWorldMap::getInstance()->simInfoFromHandle(handle);
		if ((info == NULL) || (info->isDown()))
		{
			continue;
		}
		if (map_show_infohubs)
		{
			drawGenericItems(info->getInfoHub(), sInfohubImage);
		}
		if (map_show_telehubs)
		{
			drawGenericItems(info->getTeleHub(), sTelehubImage);
		}
		if (map_show_land_for_sale)
		{
			drawGenericItems(info->getLandForSale(), sForSaleImage);
			if (gAgent.canAccessAdult())
			{
				drawGenericItems(info->getLandForSaleAdult(), sForSaleAdultImage);
			}
		}
		if (map_show_pg_events)
		{
			drawGenericItems(info->getPGEvent(), sEventImage);
		}
		if (show_mature)
		{
			drawGenericItems(info->getMatureEvent(), sEventMatureImage);
		}
		if (show_adult)
		{
			drawGenericItems(info->getAdultEvent(), sEventAdultImage);
		}
	}
}
void LLWorldMapView::drawAgents()
{
	F32 agents_scale = (sMapScale * 0.9f) / 256.f;
	LLColor4 avatar_color = gSavedSettings.getColor( "MapAvatar" );
	for (handle_list_t::iterator iter = mVisibleRegions.begin(); iter != mVisibleRegions.end(); ++iter)
	{
		U64 handle = *iter;
		LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
		if ((siminfo == NULL) || (siminfo->isDown()))
		{
			continue;
		}
		LLSimInfo::item_info_list_t::const_iterator it = siminfo->getAgentLocation().begin();
		while (it != siminfo->getAgentLocation().end())
		{
			S32 agent_count = (S32)(((it->getCount()-1) * agents_scale + (it->getCount()-1) * 0.1f)+.1f) + 1;
			drawImageStack(it->getGlobalPosition(), sAvatarSmallImage, agent_count, 3.f, avatar_color);
			++it;
		}
	}
}
void LLWorldMapView::drawFrustum()
{
	F32 meters_to_pixels = sMapScale/ REGION_WIDTH_METERS;
	F32 horiz_fov = LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect();
	F32 far_clip_meters = LLViewerCamera::getInstance()->getFar();
	F32 far_clip_pixels = far_clip_meters * meters_to_pixels;
	F32 half_width_meters = far_clip_meters * tan( horiz_fov / 2 );
	F32 half_width_pixels = half_width_meters * meters_to_pixels;
	LLVector2 ui_scale_factor = gViewerWindow->getUIScale();
	F32 ctr_x = (getLocalRect().getWidth() * 0.5f + sPanX)  * ui_scale_factor.mV[VX];
	F32 ctr_y = (getLocalRect().getHeight() * 0.5f + sPanY) * ui_scale_factor.mV[VY];
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.pushMatrix();
	{
		gGL.translatef( ctr_x, ctr_y, 0 );
		gGL.begin( LLRender::TRIANGLES  );
		{
			LLVector3 at_axis = LLViewerCamera::instance().getAtAxis();
			LLVector3 left_axis = LLViewerCamera::instance().getLeftAxis();
			LLVector2 cam_lookat(at_axis.mV[VX], at_axis.mV[VY]);
			LLVector2 cam_left(left_axis.mV[VX], left_axis.mV[VY]);
			if (is_approx_zero(cam_lookat.magVecSquared()))
			{
				cam_lookat = LLVector2(1.f, 0.f);
				cam_left = LLVector2(0.f, 1.f);
			}
			cam_lookat.normVec();
			cam_left.normVec();
			gGL.color4f(1.f, 1.f, 1.f, 0.25f);
			gGL.vertex2f( 0, 0 );
			gGL.color4f(1.f, 1.f, 1.f, 0.02f);
			LLVector2 vert = cam_lookat * far_clip_pixels + cam_left * half_width_pixels;
			gGL.vertex2f(vert.mV[VX], vert.mV[VY]);
			vert = cam_lookat * far_clip_pixels - cam_left * half_width_pixels;
			gGL.vertex2f(vert.mV[VX], vert.mV[VY]);
		}
		gGL.end();
	}
	gGL.popMatrix();
}
LLVector3 LLWorldMapView::globalPosToView( const LLVector3d& global_pos )
{
	LLVector3d relative_pos_global = global_pos - gAgentCamera.getCameraPositionGlobal();
	LLVector3 pos_local;
	pos_local.setVec(relative_pos_global);
	pos_local.mV[VX] *= sPixelsPerMeter;
	pos_local.mV[VY] *= sPixelsPerMeter;
	pos_local.mV[VX] += getRect().getWidth() / 2 + sPanX;
	pos_local.mV[VY] += getRect().getHeight() / 2 + sPanY;
	return pos_local;
}
void LLWorldMapView::drawTracking(const LLVector3d& pos_global, const LLColor4& color, BOOL draw_arrow,
								  const std::string& label, const std::string& tooltip, S32 vert_offset )
{
	LLVector3 pos_local = globalPosToView( pos_global );
	S32 x = ll_round( pos_local.mV[VX] );
	S32 y = ll_round( pos_local.mV[VY] );
	LLFontGL* font = LLFontGL::getFontSansSerifSmall();
	S32 text_x = x;
	S32 text_y = (S32)(y - sTrackCircleImage->getHeight()/2 - font->getLineHeight());
	if(    x < 0
		|| y < 0
		|| x >= getRect().getWidth()
		|| y >= getRect().getHeight() )
	{
		if (draw_arrow)
		{
			drawTrackingCircle( getRect(), x, y, color, 3, 15 );
			drawTrackingArrow( getRect(), x, y, color );
			text_x = sTrackingArrowX;
			text_y = sTrackingArrowY;
		}
	}
	else if (LLTracker::getTrackingStatus() == LLTracker::TRACKING_LOCATION &&
		LLTracker::getTrackedLocationType() != LLTracker::LOCATION_NOTHING)
	{
		drawTrackingCircle( getRect(), x, y, color, 3, 15 );
	}
	else
	{
		drawImage(pos_global, sTrackCircleImage, color);
	}
	const S32 TEXT_PADDING = DEFAULT_TRACKING_ARROW_SIZE + 2;
	S32 half_text_width = llfloor(font->getWidthF32(label) * 0.5f);
	text_x = llclamp(text_x, half_text_width + TEXT_PADDING, getRect().getWidth() - half_text_width - TEXT_PADDING);
	text_y = llclamp(text_y + vert_offset, TEXT_PADDING + vert_offset, getRect().getHeight() - ll_round(font->getLineHeight()) - TEXT_PADDING - vert_offset);
	if ( (label != "") && (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) )
	{
		font->renderUTF8(
			label, 0,
			text_x,
			text_y,
			LLColor4::white, LLFontGL::HCENTER,
			LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);
		if (tooltip != "")
		{
			text_y -= (S32)font->getLineHeight();
			font->renderUTF8(
				tooltip, 0,
				text_x,
				text_y,
				LLColor4::white, LLFontGL::HCENTER,
				LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);
		}
	}
}
LLVector3d LLWorldMapView::viewPosToGlobal( S32 x, S32 y )
{
	x -= llfloor((getRect().getWidth() / 2 + sPanX));
	y -= llfloor((getRect().getHeight() / 2 + sPanY));
	LLVector3 pos_local( (F32)x, (F32)y, 0.f );
	pos_local *= ( REGION_WIDTH_METERS / sMapScale );
	LLVector3d pos_global;
	pos_global.setVec( pos_local );
	pos_global += gAgentCamera.getCameraPositionGlobal();
	if(gAgent.isGodlike())
	{
		pos_global.mdV[VZ] = GODLY_TELEPORT_HEIGHT;
	}
	else
	{
		pos_global.mdV[VZ] = gAgent.getPositionAgent().mV[VZ];
	}
	return pos_global;
}
BOOL LLWorldMapView::handleToolTip( S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen )
{
	LLVector3d pos_global = viewPosToGlobal(x, y);
	U64 handle = to_region_handle(pos_global);
	LLSimInfo* info = LLWorldMap::getInstance()->simInfoFromHandle(handle);
	if (info)
	{
		LLViewerRegion *region = gAgent.getRegion();
		std::string message = llformat("%s (%s)",
			(!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) ? info->getName().c_str() : RlvStrings::getString(RLV_STRING_HIDDEN_REGION).c_str(),
				info->getAccessString().c_str());
		if (!info->isDown())
		{
			S32 agent_count = info->getAgentCount();
			if (region && (region->getHandle() == handle))
			{
				++agent_count;
			}
			if (agent_count > 0)
			{
				message += llformat("\n%d ", agent_count);
				if (agent_count == 1)
				{
					message += "avatar";
				}
				else
				{
					message += "avatars";
				}
			}
		}
		msg.assign( message );
		std::string region_flags = info->getFlagsString();
		if (!region_flags.empty())
		{
			msg += '\n';
			msg += region_flags;
		}
		S32 SLOP = 4;
		localPointToScreen(
			x - SLOP, y - SLOP,
			&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
		sticky_rect_screen->mRight = sticky_rect_screen->mLeft + 2 * SLOP;
		sticky_rect_screen->mTop = sticky_rect_screen->mBottom + 2 * SLOP;
	}
	return TRUE;
}
static void drawDot(F32 x_pixels, F32 y_pixels,
			 const LLColor4& color,
			 F32 relative_z,
			 F32 dot_radius,
			 LLUIImagePtr dot_image)
{
	const F32 HEIGHT_THRESHOLD = 7.f;
	if(-HEIGHT_THRESHOLD <= relative_z && relative_z <= HEIGHT_THRESHOLD)
	{
		dot_image->draw(ll_round(x_pixels) - dot_image->getWidth()/2,
						ll_round(y_pixels) - dot_image->getHeight()/2,
						color);
	}
	else
	{
		F32 left =		x_pixels - dot_radius;
		F32 right =		x_pixels + dot_radius;
		F32 center = (left + right) * 0.5f;
		F32 top =		y_pixels + dot_radius;
		F32 bottom =	y_pixels - dot_radius;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv( color.mV );
		LLUI::setLineWidth(3.0f);
		F32 point = relative_z > HEIGHT_THRESHOLD ? top : bottom;
		F32 back = relative_z > HEIGHT_THRESHOLD ? bottom : top;
		gGL.begin( LLRender::LINES );
			gGL.vertex2f(left, back);
			gGL.vertex2f(center, point);
			gGL.vertex2f(center, point);
			gGL.vertex2f(right, back);
		gGL.end();
		LLUI::setLineWidth(1.0f);
	}
}
void LLWorldMapView::drawAvatar(F32 x_pixels,
								F32 y_pixels,
								const LLColor4& color,
								F32 relative_z,
								F32 dot_radius)
{
	const F32 HEIGHT_THRESHOLD = 7.f;
	LLUIImagePtr dot_image = sAvatarLevelImage;
	if (relative_z == 16000.f)
	{
		dot_image = sAvatarSmallImage;
	}
	else if (relative_z < -HEIGHT_THRESHOLD)
	{
		dot_image = sAvatarBelowImage;
	}
	else if (relative_z > HEIGHT_THRESHOLD)
	{
		dot_image = sAvatarAboveImage;
	}
	S32 dot_width = ll_round(dot_radius * 2.f);
	dot_image->draw(ll_round(x_pixels - dot_radius),
		            ll_round(y_pixels - dot_radius),
		            dot_width,
		            dot_width,
		            color);
}
void LLWorldMapView::drawTrackingDot( F32 x_pixels,
									  F32 y_pixels,
									  const LLColor4& color,
									  F32 relative_z,
									  F32 dot_radius)
{
	drawDot(x_pixels, y_pixels, color, relative_z, dot_radius, sTrackCircleImage);
}
void LLWorldMapView::drawIconName(F32 x_pixels,
								  F32 y_pixels,
								  const LLColor4& color,
								  const std::string& first_line,
								  const std::string& second_line)
{
	const S32 VERT_PAD = 8;
	S32 text_x = ll_round(x_pixels);
	S32 text_y = ll_round(y_pixels
						 - BIG_DOT_RADIUS
						 - VERT_PAD);
	LLFontGL::getFontSansSerif()->renderUTF8(first_line, 0,
		text_x,
		text_y,
		color,
		LLFontGL::HCENTER,
		LLFontGL::TOP,
		LLFontGL::NORMAL,
		LLFontGL::DROP_SHADOW);
	text_y -= ll_round(LLFontGL::getFontSansSerif()->getLineHeight());
	LLFontGL::getFontSansSerif()->renderUTF8(second_line, 0,
		text_x,
		text_y,
		color,
		LLFontGL::HCENTER,
		LLFontGL::TOP,
		LLFontGL::NORMAL,
		LLFontGL::DROP_SHADOW);
}
void LLWorldMapView::drawTrackingCircle( const LLRect& rect, S32 x, S32 y, const LLColor4& color, S32 min_thickness, S32 overlap )
{
	F32 start_theta = 0.f;
	F32 end_theta = F_TWO_PI;
	F32 x_delta = 0.f;
	F32 y_delta = 0.f;
	if (x < 0)
	{
		x_delta = 0.f - (F32)x;
		start_theta = F_PI + F_PI_BY_TWO;
		end_theta = F_TWO_PI + F_PI_BY_TWO;
	}
	else if (x > rect.getWidth())
	{
		x_delta = (F32)(x - rect.getWidth());
		start_theta = F_PI_BY_TWO;
		end_theta = F_PI + F_PI_BY_TWO;
	}
	if (y < 0)
	{
		y_delta = 0.f - (F32)y;
		if (x < 0)
		{
			start_theta = 0.f;
			end_theta = F_PI_BY_TWO;
		}
		else if (x > rect.getWidth())
		{
			start_theta = F_PI_BY_TWO;
			end_theta = F_PI;
		}
		else
		{
			start_theta = 0.f;
			end_theta = F_PI;
		}
	}
	else if (y > rect.getHeight())
	{
		y_delta = (F32)(y - rect.getHeight());
		if (x < 0)
		{
			start_theta = F_PI + F_PI_BY_TWO;
			end_theta = F_TWO_PI;
		}
		else if (x > rect.getWidth())
		{
			start_theta = F_PI;
			end_theta = F_PI + F_PI_BY_TWO;
		}
		else
		{
			start_theta = F_PI;
			end_theta = F_TWO_PI;
		}
	}
	F32 distance = sqrtf(x_delta * x_delta + y_delta * y_delta);
	distance = llmax(0.1f, distance);
	F32 outer_radius = distance + (1.f + (9.f * sqrtf(x_delta * y_delta) / distance)) * (F32)overlap;
	F32 inner_radius = outer_radius - (F32)min_thickness;
	F32 angle_adjust_x = asin(x_delta / outer_radius);
	F32 angle_adjust_y = asin(y_delta / outer_radius);
	if (angle_adjust_x)
	{
		if (angle_adjust_y)
		{
			F32 angle_adjust = llmin(angle_adjust_x, angle_adjust_y);
			start_theta += angle_adjust;
			end_theta -= angle_adjust;
		}
		else
		{
			start_theta += angle_adjust_x;
			end_theta -= angle_adjust_x;
		}
	}
	else if (angle_adjust_y)
	{
		start_theta += angle_adjust_y;
		end_theta -= angle_adjust_y;
	}
	gGL.pushUIMatrix();
	gGL.translateUI(x, y, 0.f);
	gl_washer_segment_2d(inner_radius, outer_radius, start_theta, end_theta, 40, color, color);
	gGL.popUIMatrix();
}
void LLWorldMapView::drawTrackingArrow(const LLRect& rect, S32 x, S32 y,
									   const LLColor4& color,
									   S32 arrow_size)
{
	F32 x_center = (F32)rect.getWidth() / 2.f;
	F32 y_center = (F32)rect.getHeight() / 2.f;
	F32 x_clamped = (F32)llclamp( x, 0, rect.getWidth() - arrow_size );
	F32 y_clamped = (F32)llclamp( y, 0, rect.getHeight() - arrow_size );
	F32 slope = (F32)(y - y_center) / (F32)(x - x_center);
	F32 window_ratio = (F32)rect.getHeight() / (F32)rect.getWidth();
	if (llabs(slope) > window_ratio && y_clamped != (F32)y)
	{
		x_clamped = (y_clamped - y_center) / slope + x_center;
		x_clamped  = llclamp(x_clamped , 0.f, (F32)(rect.getWidth() - arrow_size) );
	}
	else if (x_clamped != (F32)x)
	{
		y_clamped = (x_clamped - x_center) * slope + y_center;
		y_clamped = llclamp( y_clamped, 0.f, (F32)(rect.getHeight() - arrow_size) );
	}
	S32 half_arrow_size = (S32) (0.5f * arrow_size);
	F32 angle = atan2( y + half_arrow_size - y_center, x + half_arrow_size - x_center);
	sTrackingArrowX = llfloor(x_clamped);
	sTrackingArrowY = llfloor(y_clamped);
	gl_draw_scaled_rotated_image(
		sTrackingArrowX,
		sTrackingArrowY,
		arrow_size, arrow_size,
		RAD_TO_DEG * angle,
		sTrackArrowImage->getImage(),
		color);
}
void LLWorldMapView::setDirectionPos( LLTextBox* text_box, F32 rotation )
{
	F32 map_half_height = getRect().getHeight() * 0.5f;
	F32 map_half_width = getRect().getWidth() * 0.5f;
	F32 text_half_height = text_box->getRect().getHeight() * 0.5f;
	F32 text_half_width = text_box->getRect().getWidth() * 0.5f;
	F32 radius = llmin( map_half_height - text_half_height, map_half_width - text_half_width );
	text_box->setOrigin(
		ll_round(map_half_width - text_half_width + radius * cos( rotation )),
		ll_round(map_half_height - text_half_height + radius * sin( rotation )) );
}
void LLWorldMapView::updateDirections()
{
	S32 width = getRect().getWidth();
	S32 height = getRect().getHeight();
	S32 text_height = mTextBoxNorth->getRect().getHeight();
	S32 text_width = mTextBoxNorth->getRect().getWidth();
	const S32 PAD = 2;
	S32 top = height - text_height - PAD;
	S32 left = PAD*2;
	S32 bottom = PAD;
	S32 right = width - text_width - PAD;
	S32 center_x = width/2 - text_width/2;
	S32 center_y = height/2 - text_height/2;
	mTextBoxNorth->setOrigin( center_x, top );
	mTextBoxEast->setOrigin( right, center_y );
	mTextBoxSouth->setOrigin( center_x, bottom );
	mTextBoxWest->setOrigin( left, center_y );
	text_width = mTextBoxNorthWest->getRect().getWidth();
	right = width - text_width - PAD;
	mTextBoxNorthWest->setOrigin(left, top);
	mTextBoxNorthEast->setOrigin(right, top);
	mTextBoxSouthWest->setOrigin(left, bottom);
	mTextBoxSouthEast->setOrigin(right, bottom);
}
void LLWorldMapView::reshape( S32 width, S32 height, BOOL called_from_parent )
{
	LLView::reshape( width, height, called_from_parent );
}
bool LLWorldMapView::checkItemHit(S32 x, S32 y, LLItemInfo& item, LLUUID* id, bool track)
{
	LLVector3 pos_view = globalPosToView(item.getGlobalPosition());
	S32 item_x = ll_round(pos_view.mV[VX]);
	S32 item_y = ll_round(pos_view.mV[VY]);
	if (x < item_x - BIG_DOT_RADIUS) return false;
	if (x > item_x + BIG_DOT_RADIUS) return false;
	if (y < item_y - BIG_DOT_RADIUS) return false;
	if (y > item_y + BIG_DOT_RADIUS) return false;
	LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromHandle(item.getRegionHandle());
	if (sim_info)
	{
		if (track)
		{
			gFloaterWorldMap->trackLocation(item.getGlobalPosition());
		}
	}
	if (track)
	{
		gFloaterWorldMap->trackGenericItem(item);
	}
	*id = item.getUUID();
	return true;
}
void LLWorldMapView::handleClick(S32 x, S32 y, MASK mask,
								 S32* hit_type,
								 LLUUID* id)
{
	LLVector3d pos_global = viewPosToGlobal(x, y);
	if(gAgent.isGodlike())
	{
		pos_global.mdV[VZ] = 200.0;
	}
	*hit_type = 0;
	LLWorldMap::getInstance()->cancelTracking();
	static const LLCachedControl<bool> map_show_pg_events("MapShowPGEvents");
	static const LLCachedControl<bool> map_show_mature_events("MapShowMatureEvents");
	static const LLCachedControl<bool> map_show_adult_events("MapShowAdultEvents");
	static const LLCachedControl<bool> map_show_land_for_sale("MapShowLandForSale");
	if(sMapScale >= DRAW_TEXT_THRESHOLD)
	{
		bool show_mature = gAgent.canAccessMature() && map_show_mature_events;
		bool show_adult = gAgent.canAccessAdult() && map_show_adult_events;
		if (map_show_pg_events || show_mature || show_adult || map_show_land_for_sale)
		{
			for (handle_list_t::iterator iter = mVisibleRegions.begin(); iter != mVisibleRegions.end(); ++iter)
			{
				U64 handle = *iter;
				LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
				if ((siminfo == NULL) || (siminfo->isDown()))
				{
					continue;
				}
				if (map_show_pg_events)
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getPGEvent().begin();
					while (it != siminfo->getPGEvent().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, false))
						{
							*hit_type = MAP_ITEM_PG_EVENT;
							mItemPicked = TRUE;
							gFloaterWorldMap->trackEvent(event);
							return;
						}
						++it;
					}
				}
				if (show_mature)
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getMatureEvent().begin();
					while (it != siminfo->getMatureEvent().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, false))
						{
							*hit_type = MAP_ITEM_MATURE_EVENT;
							mItemPicked = TRUE;
							gFloaterWorldMap->trackEvent(event);
							return;
						}
						++it;
					}
				}
				if (show_adult)
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getAdultEvent().begin();
					while (it != siminfo->getAdultEvent().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, false))
						{
							*hit_type = MAP_ITEM_ADULT_EVENT;
							mItemPicked = TRUE;
							gFloaterWorldMap->trackEvent(event);
							return;
						}
						++it;
					}
				}
				if (map_show_land_for_sale)
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getLandForSale().begin();
					while (it != siminfo->getLandForSale().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, true))
						{
							*hit_type = MAP_ITEM_LAND_FOR_SALE;
							mItemPicked = TRUE;
							return;
						}
						++it;
					}
					if (gAgent.canAccessAdult())
					{
						LLSimInfo::item_info_list_t::const_iterator it = siminfo->getLandForSaleAdult().begin();
						while (it != siminfo->getLandForSaleAdult().end())
						{
							LLItemInfo event = *it;
							if (checkItemHit(x, y, event, id, true))
							{
								*hit_type = MAP_ITEM_LAND_FOR_SALE_ADULT;
								mItemPicked = TRUE;
								return;
							}
							++it;
						}
					}
				}
			}
		}
	}
	gFloaterWorldMap->trackLocation(pos_global);
	mItemPicked = FALSE;
	*id = LLUUID::null;
	return;
}
BOOL outside_slop(S32 x, S32 y, S32 start_x, S32 start_y)
{
	S32 dx = x - start_x;
	S32 dy = y - start_y;
	return (dx <= -2 || 2 <= dx || dy <= -2 || 2 <= dy);
}
BOOL LLWorldMapView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	gFocusMgr.setMouseCapture( this );
	mMouseDownPanX = ll_round(sPanX);
	mMouseDownPanY = ll_round(sPanY);
	mMouseDownX = x;
	mMouseDownY = y;
	sHandledLastClick = TRUE;
	return TRUE;
}
BOOL LLWorldMapView::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (hasMouseCapture())
	{
		if (mPanning)
		{
			S32 local_x, local_y;
			local_x = mMouseDownX + llfloor(sPanX - mMouseDownPanX);
			local_y = mMouseDownY + llfloor(sPanY - mMouseDownPanY);
			LLRect clip_rect = getRect();
			clip_rect.stretch(-8);
			clip_rect.clipPointToRect(mMouseDownX, mMouseDownY, local_x, local_y);
			LLUI::setMousePositionLocal(this, local_x, local_y);
			mPanning = FALSE;
			mMouseDownX = 0;
			mMouseDownY = 0;
		}
		else
		{
			S32 hit_type;
			LLUUID id;
			handleClick(x, y, mask, &hit_type, &id);
		}
		gViewerWindow->showCursor();
		gFocusMgr.setMouseCapture( NULL );
		return TRUE;
	}
	return FALSE;
}
void LLWorldMapView::updateVisibleBlocks()
{
	if (sMapScale < SIM_DATA_SCALE)
	{
		return;
	}
	LLVector3d camera_global = gAgentCamera.getCameraPositionGlobal();
	const S32 width = getRect().getWidth();
	const S32 height = getRect().getHeight();
	const F32 half_width = F32(width) / 2.0f;
	const F32 half_height = F32(height) / 2.0f;
	S32 world_center_x = S32((-sPanX / sMapScale) + (camera_global.mdV[0] / REGION_WIDTH_METERS));
	S32 world_center_y = S32((-sPanY / sMapScale) + (camera_global.mdV[1] / REGION_WIDTH_METERS));
	S32 world_left   = world_center_x - S32(half_width  / sMapScale) - 1;
	S32 world_right  = world_center_x + S32(half_width  / sMapScale) + 1;
	S32 world_bottom = world_center_y - S32(half_height / sMapScale) - 1;
	S32 world_top    = world_center_y + S32(half_height / sMapScale) + 1;
	LLWorldMap::getInstance()->updateRegions(world_left, world_bottom, world_right, world_top);
}
BOOL LLWorldMapView::handleHover( S32 x, S32 y, MASK mask )
{
	if (hasMouseCapture())
	{
		if (mPanning || outside_slop(x, y, mMouseDownX, mMouseDownY))
		{
			if (!mPanning)
			{
				mPanning = TRUE;
				gViewerWindow->hideCursor();
			}
			F32 delta_x = (F32)(gViewerWindow->getCurrentMouseDX());
			F32 delta_y = (F32)(gViewerWindow->getCurrentMouseDY());
			sPanX += delta_x;
			sPanY += delta_y;
			sTargetPanX = sPanX;
			sTargetPanY = sPanY;
			gViewerWindow->moveCursorToCenter();
		}
		gViewerWindow->setCursor(UI_CURSOR_CROSS );
		return TRUE;
	}
	else
	{
		LLVector3d pos_global = LLTracker::getTrackedPositionGlobal();
		if (LLTracker::isTracking()
			&& pos_global.isExactlyZero())
		{
			gViewerWindow->setCursor( UI_CURSOR_WAIT );
		}
		else
		{
			gViewerWindow->setCursor( UI_CURSOR_CROSS );
		}
		LL_DEBUGS("UserInput") << "hover handled by LLWorldMapView" << LL_ENDL;
		return TRUE;
	}
}
BOOL LLWorldMapView::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	if( sHandledLastClick )
	{
		S32 hit_type;
		LLUUID id;
		handleClick(x, y, mask, &hit_type, &id);
		switch (hit_type)
		{
		case MAP_ITEM_PG_EVENT:
		case MAP_ITEM_MATURE_EVENT:
		case MAP_ITEM_ADULT_EVENT:
			{
				gFloaterWorldMap->close();
				std::string uuid_str;
				S32 event_id;
				id.toString(uuid_str);
				uuid_str = uuid_str.substr(28);
				sscanf(uuid_str.c_str(), "%X", &event_id);
				LLFloaterDirectory::showEvents(event_id);
				break;
			}
		case MAP_ITEM_LAND_FOR_SALE:
		case MAP_ITEM_LAND_FOR_SALE_ADULT:
			{
				gFloaterWorldMap->close();
				LLFloaterDirectory::showLandForSale(id);
				break;
			}
		case MAP_ITEM_CLASSIFIED:
			{
				gFloaterWorldMap->close();
				LLFloaterDirectory::showClassified(id);
				break;
			}
		default:
			{
				if (!gSavedSettings.getBOOL("DoubleClickTeleportMap")) return true;
				if (LLWorldMap::getInstance()->isTracking())
				{
					LLWorldMap::getInstance()->setTrackingDoubleClick();
				}
				else
				{
					LLVector3d pos_global = viewPosToGlobal(x,y);
					LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromPosGlobal(pos_global);
					if (sim_info && !sim_info->isDown())
					{
						gAgent.teleportViaLocation( pos_global );
					}
				}
			}
		};
		return TRUE;
	}
	return FALSE;
}
