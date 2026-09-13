/** 
 * @file llviewerdisplay.cpp
 * @brief LLViewerDisplay class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llviewerdisplay.h"
#include "llgl.h"
#include "llrender.h"
#include "llglheaders.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "lldir.h"
#include "lldynamictexture.h"
#include "lldrawpoolalpha.h"
#include "llfeaturemanager.h"
#include "llfirstuse.h"
#include "llframestats.h"
#include "llhudmanager.h"
#include "llimagebmp.h"
#include "llimagegl.h"
#include "lloctree.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llstartup.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "lltooldraganddrop.h"
#include "lltoolpie.h"
#include "lltracker.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvograss.h"
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llappviewer.h"
#include "llheapdiag.h"
#include "llstartup.h"
#include "llviewershadermgr.h"
#include "llfasttimer.h"
#include "llfloatertools.h"
#include "llviewertexturelist.h"
#include "llfocusmgr.h"
#include "llcubemap.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolavatar.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llpostprocess.h"
#include "sgmemstat.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
extern bool gShiftFrame;
LLPointer<LLViewerTexture> gDisconnectedImagep = NULL;
const F32 TELEPORT_RENDER_DELAY = 20.f;
const F32 TELEPORT_ARRIVAL_DELAY = 2.f;
const F32 TELEPORT_LOCAL_DELAY = 1.0f;
BOOL		 gTeleportDisplay = FALSE;
LLFrameTimer gTeleportDisplayTimer;
LLFrameTimer gTeleportArrivalTimer;
LLFrameTimer gPostTeleportFinishKillObjectDelayTimer;
F32			 gSavedDrawDistance = 0.0f;
const F32		RESTORE_GL_TIME = 5.f;
BOOL gForceRenderLandFence = FALSE;
BOOL gDisplaySwapBuffers = FALSE;
BOOL gDepthDirty = FALSE;
BOOL gResizeScreenTexture = FALSE;
BOOL gWindowResized = FALSE;
BOOL gSnapshot = FALSE;
U32 gRecentFrameCount = 0;
LLFrameTimer gRecentFPSTime;
LLFrameTimer gRecentMemoryTime;
void pre_show_depth_buffer();
void post_show_depth_buffer();
void render_ui(F32 zoom_factor = 1.f, int subfield = 0, bool tiling = false);
void render_hud_attachments();
void render_ui_3d();
void render_ui_2d();
void render_disconnected_background();
void display_startup()
{
	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->getWindow()->getVisible()
		|| gViewerWindow->getWindow()->getMinimized()
		|| gNoRender )
	{
		return;
	}
	gPipeline.updateGL();
	LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
	LLGLSDefault gls_default;
	static S32 frame_count = 0;
	LLGLStateValidator::checkStates();
	LLGLStateValidator::checkTextureChannels();
	if (frame_count++ > 1)
	{
		LLViewerDynamicTexture::updateAllInstances();
	}
	LLGLStateValidator::checkStates();
	LLGLStateValidator::checkTextureChannels();
	gViewerWindow->updateUI();
	gGL.syncContextState();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	LLGLSUIDefault gls_ui;
	gViewerWindow->setup2DRender();
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	gGL.color4f(1,1,1,1);
	gViewerWindow->draw();
	gGL.flush();
	LLVertexBuffer::unbind();
	LLGLStateValidator::checkStates();
	LLGLStateValidator::checkTextureChannels();
	gViewerWindow->getWindow()->swapBuffers();
	gGL.syncContextState();
	glClear(GL_DEPTH_BUFFER_BIT);
}
void display_update_camera(bool tiling=false)
{
	LL_PUSH_CALLSTACKS();
	F32 final_far = gAgentCamera.mDrawDistance;
    static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(freeze_time || tiling)
	{
		LLViewerCamera::getInstance()->setFar(final_far);
		gViewerWindow->setup3DRender();
		LLWorld::getInstance()->setLandFarClip(final_far);
		return;
	}
	if (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode())
	{
		final_far *= 0.5f;
	}
	LLViewerCamera::getInstance()->setFar(final_far);
	gViewerWindow->setup3DRender();
	LLWLParamManager::getInstance()->update(LLViewerCamera::getInstance());
	LLWaterParamManager::getInstance()->update(LLViewerCamera::getInstance());
	LLWorld::getInstance()->setLandFarClip(final_far);
}
void display_stats()
{
	if (gNoRender || !gViewerWindow->getWindow()->getVisible() || !gFocusMgr.getAppHasFocus())
	{
		gRecentFrameCount = 0;
		gRecentFPSTime.reset();
	}
	F32 fps_log_freq = gSavedSettings.getF32("FPSLogFrequency");
	if (fps_log_freq > 0.f && gRecentFPSTime.getElapsedTimeF32() >= fps_log_freq)
	{
		F32 fps = gRecentFrameCount / fps_log_freq;
		LL_INFOS() << llformat("FPS: %.02f", fps) << LL_ENDL;
		LL_INFOS() << llformat("VBO: %d  glVBO: %d", LLVertexBuffer::sCount, LLVertexBuffer::sGLCount) << LL_ENDL;
#ifdef LL_OCTREE_STATS
		OctreeStats::getInstance()->dump();
#endif
		gRecentFrameCount = 0;
		gRecentFPSTime.reset();
	}
	F32 mem_log_freq = gSavedSettings.getF32("MemoryLogFrequency");
	if (mem_log_freq > 0.f && gRecentMemoryTime.getElapsedTimeF32() >= mem_log_freq)
	{
		gMemoryAllocated = U64Bytes(LLMemory::getCurrentRSS());
		U32Megabytes memory = gMemoryAllocated;
		LL_INFOS() << "MEMORY: " << memory << LL_ENDL;
		LL_INFOS() << "THREADS: "<< LLThread::getCount() << LL_ENDL;
		LL_INFOS() << "MALLOC: " << SGMemStat::getPrintableStat() <<LL_ENDL;
		LLMemory::logMemoryInfo(TRUE) ;
		gRecentMemoryTime.reset();
	}
}
static LLTrace::BlockTimerStatHandle FTM_PICK("Picking");
static LLTrace::BlockTimerStatHandle FTM_RENDER("Render", true);
static LLTrace::BlockTimerStatHandle FTM_UPDATE_SKY("Update Sky");
static LLTrace::BlockTimerStatHandle FTM_UPDATE_TEXTURES("Update Textures");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE("Update Images");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_CLASS("Class");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_BUMP("Bump");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_LIST("List");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_DELETE("Delete");
int sMaxCacheHit = 0;
int sCurCacheHit = 0;
void display(BOOL rebuild, F32 zoom_factor, int subfield, BOOL for_snapshot, bool tiling)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER);
	gViewerWindow->checkSettings();
	LLVBOPool::deleteReleasedBuffers();
	for (auto avatar : LLCharacter::sInstances)
	{
		LLVOAvatar* avatarp = dynamic_cast<LLVOAvatar*>(avatar);
		if (!avatarp) continue;
		if (avatarp->isDead()) continue;
		avatarp->clearRiggedMatrixCache();
	}
	sCurCacheHit = 0;
	if (gWindowResized)
	{
		gGL.flush();
		gGL.syncContextState();
		glClear(GL_COLOR_BUFFER_BIT);
		gViewerWindow->getWindow()->swapBuffers();
		LLPipeline::refreshCachedSettings();
		gPipeline.resizeScreenTexture();
		gResizeScreenTexture = FALSE;
		gWindowResized = FALSE;
		return;
	}
	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_pause();
	}
	gSnapshot = for_snapshot;
	LLGLSDefault gls_default;
	LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE, GL_LEQUAL);
	LLVertexBuffer::unbind();
	LLGLStateValidator::checkStates();
	LLGLStateValidator::checkTextureChannels();
	stop_glerror();
	LLGLState<GL_LIGHTING> light_state;
	gPipeline.disableLights(light_state);
	gPipeline.doResetVertexBuffers();
	stop_glerror();
	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->getWindow()->getVisible()
		|| gViewerWindow->getWindow()->getMinimized() )
	{
		if (rebuild)
		{
			gFrameStats.start(LLFrameStats::REBUILD);
			stop_glerror();
			gPipeline.rebuildPools();
			stop_glerror();
		}
		stop_glerror();
		gViewerWindow->returnEmptyPicks();
		stop_glerror();
		return;
	}
	{
		LL_RECORD_BLOCK_TIME(FTM_PICK);
		LLAppViewer::instance()->pingMainloopTimeout("Display:Pick");
		gViewerWindow->performPick();
	}
	LLAppViewer::instance()->pingMainloopTimeout("Display:CheckStates");
	LLGLStateValidator::checkStates();
	LLGLStateValidator::checkTextureChannels();
	if (gNoRender)
	{
#if LL_WINDOWS
		static F32 last_update_time = 0.f;
		if ((gFrameTimeSeconds - last_update_time) > 1.f)
		{
			InvalidateRect((HWND)gViewerWindow->getPlatformWindow(), NULL, FALSE);
			last_update_time = gFrameTimeSeconds;
		}
#endif
		return;
	}
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Startup");
		display_startup();
		return;
	}
	LLAppViewer::instance()->pingMainloopTimeout("Display:TextureStats");
	gFrameStats.start(LLFrameStats::UPDATE_TEX_STATS);
	stop_glerror();
	LLImageGL::updateStats(gFrameTimeSeconds);
	LLVOAvatar::sRenderName = gSavedSettings.getS32("RenderName");
	LLVOAvatar::sRenderGroupTitles = !gSavedSettings.getBOOL("RenderHideGroupTitleAll");
	gPipeline.mBackfaceCull = TRUE;
	gFrameCount++;
	gRecentFrameCount++;
	if (gFocusMgr.getAppHasFocus())
	{
		gForegroundFrameCount++;
	}
	if (gTeleportDisplay)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Teleport");
		const F32 TELEPORT_ARRIVAL_DELAY = 2.f;
		S32 attach_count = 0;
		if (isAgentAvatarValid())
		{
			attach_count = gAgentAvatarp->getAttachmentCount();
		}
		F32 teleport_save_time = TELEPORT_EXPIRY + TELEPORT_EXPIRY_PER_ATTACHMENT * attach_count;
		F32 teleport_elapsed = gTeleportDisplayTimer.getElapsedTimeF32();
		F32 teleport_percent = teleport_elapsed * (100.f / teleport_save_time);
		if( (gAgent.getTeleportState() != LLAgent::TELEPORT_START) && (teleport_percent > 100.f) )
		{
			gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
			gAgent.setTeleportMessage(std::string());
		}
		static const LLCachedControl<bool> hide_tp_screen("AscentDisableTeleportScreens",false);
		const std::string& message = gAgent.getTeleportMessage();
		switch( gAgent.getTeleportState() )
		{
		case LLAgent::TELEPORT_PENDING:
			gTeleportDisplayTimer.reset();
			if(!hide_tp_screen)
				gViewerWindow->setShowProgress(TRUE);
			gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
			gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["pending"]);
			gViewerWindow->setProgressString(LLAgent::sTeleportProgressMessages["pending"]);
			break;
		case LLAgent::TELEPORT_START:
			gTeleportDisplayTimer.reset();
			gTextureList.clearFetchingRequests();
			if(!hide_tp_screen)
				gViewerWindow->setShowProgress(TRUE);
			gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
			gAgent.setTeleportState( LLAgent::TELEPORT_REQUESTED );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["requesting"]);
			gViewerWindow->setProgressString(LLAgent::sTeleportProgressMessages["requesting"]);
			break;
		case LLAgent::TELEPORT_REQUESTED:
			gViewerWindow->setProgressPercent( llmin(teleport_percent, 37.5f) );
			gViewerWindow->setProgressString(message);
			break;
		case LLAgent::TELEPORT_MOVING:
			gViewerWindow->setProgressPercent( llmin(teleport_percent, 75.f) );
			gViewerWindow->setProgressString(message);
			break;
		case LLAgent::TELEPORT_START_ARRIVAL:
			gTeleportArrivalTimer.reset();
				gViewerWindow->setProgressCancelButtonVisible(FALSE, LLTrans::getString("Cancel"));
			gViewerWindow->setProgressPercent(75.f);
			gAgent.setTeleportState( LLAgent::TELEPORT_ARRIVING );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["arriving"]);
			gTextureList.mForceResetTextureStats = TRUE;
			gPostTeleportFinishKillObjectDelayTimer.reset();
			if(!hide_tp_screen)
				gAgentCamera.resetView(TRUE, TRUE);
			break;
		case LLAgent::TELEPORT_ARRIVING:
			{
				gTextureList.updateImages(0.005f);
				F32 arrival_fraction = (gTeleportArrivalTimer.getElapsedTimeF32() / TELEPORT_ARRIVAL_DELAY);
				if( arrival_fraction > 1.f || hide_tp_screen)
				{
					arrival_fraction = 1.f;
					LLFirstUse::useTeleport();
					gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
				}
				gViewerWindow->setProgressCancelButtonVisible(FALSE, LLTrans::getString("Cancel"));
				gViewerWindow->setProgressPercent(  arrival_fraction * 25.f + 75.f);
				gViewerWindow->setProgressString(message);
			}
			break;
		case LLAgent::TELEPORT_LOCAL:
			{
				{
					LLFirstUse::useTeleport();
					gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
				}
			}
			break;
		case LLAgent::TELEPORT_NONE:
			dump_hud_teleport_state("display_TELEPORT_NONE");
			hud_teleport_logging_on_teleport_none();
			gViewerWindow->setShowProgress(FALSE);
			gTeleportDisplay = FALSE;
			gTeleportArrivalTimer.reset();
			break;
		default:
			 break;
		}
	}
    else if(LLAppViewer::instance()->logoutRequestSent())
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Logout");
		F32 percent_done = gLogoutTimer.getElapsedTimeF32() * 100.f / gLogoutMaxTime;
		if (percent_done > 100.f)
		{
			percent_done = 100.f;
		}
		if( LLApp::isExiting() )
		{
			percent_done = 100.f;
		}
		gViewerWindow->setProgressPercent( percent_done );
	}
	else
	if (gRestoreGL)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:RestoreGL");
		F32 percent_done = gRestoreGLTimer.getElapsedTimeF32() * 100.f / RESTORE_GL_TIME;
		if( percent_done > 100.f )
		{
			gViewerWindow->setShowProgress(FALSE);
			gRestoreGL = FALSE;
		}
		else
		{
			if( LLApp::isExiting() )
			{
				percent_done = 100.f;
			}
			gViewerWindow->setProgressPercent( percent_done );
		}
	}
	if (gSavedDrawDistance > 0.0f && gAgent.getTeleportState() == LLAgent::TELEPORT_NONE)
	{
		if (gTeleportArrivalTimer.getElapsedTimeF32() >=
			(F32)gSavedSettings.getU32("SpeedRezInterval"))
		{
			gTeleportArrivalTimer.reset();
			F32 current = gSavedSettings.getF32("RenderFarClip");
			if (gSavedDrawDistance > current)
			{
				current *= 2.0;
				if (current > gSavedDrawDistance)
				{
					current = gSavedDrawDistance;
				}
				gSavedSettings.setF32("RenderFarClip", current);
			}
			if (current >= gSavedDrawDistance)
			{
				gSavedDrawDistance = 0.0f;
				gSavedSettings.setF32("SavedRenderFarClip", 0.0f);
			}
		}
	}
	LLAppViewer::instance()->pingMainloopTimeout("Display:Camera");
	LLViewerCamera::getInstance()->setZoomParameters(zoom_factor, subfield);
	LLViewerCamera::getInstance()->setNear(MIN_NEAR_PLANE);
	if (gDisconnected)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Disconnected");
		render_ui();
	}
	LLAppViewer::instance()->pingMainloopTimeout("Display:RenderSetup");
	stop_glerror();
	stop_glerror();
	gGL.setAmbientLightColor(LLColor4::white);
	stop_glerror();
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:DynamicTextures");
		LL_RECORD_BLOCK_TIME(FTM_UPDATE_TEXTURES);
		if (LLViewerDynamicTexture::updateAllInstances())
		{
			gGL.setColorMask(true, true);
			gGL.syncContextState();
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	}
	gViewerWindow->setup3DViewport();
	gPipeline.resetFrameStats();
	if (!gDisconnected)
	{
		LLAppViewer::instance()->pingMainloopTimeout("Display:Update");
		if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
		}
		if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES))
		{
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
		}
		LLGLNamePool::upkeepPools();
		stop_glerror();
		display_update_camera(tiling);
		stop_glerror();
		LLHUDManager::getInstance()->updateEffects();
		LLHUDObject::updateAll();
		stop_glerror();
		if(!tiling)
		{
			gFrameStats.start(LLFrameStats::UPDATE_GEOM);
			const F32 max_geom_update_time = 0.005f*10.f*gFrameIntervalSeconds;
			gPipeline.createObjects(max_geom_update_time);
			gPipeline.processPartitionQ();
			gPipeline.updateGeom(max_geom_update_time, *LLViewerCamera::getInstance());
			stop_glerror();
			gPipeline.updateGL();
			stop_glerror();
		}
		gFrameStats.start(LLFrameStats::UPDATE_CULL);
		S32 water_clip = 0;
		if ((LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT) > 1) &&
			 (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_WATER) ||
			  gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_VOIDWATER)))
		{
			if (LLViewerCamera::getInstance()->cameraUnderWater())
			{
				water_clip = -1;
			}
			else
			{
				water_clip = 1;
			}
		}
		LLAppViewer::instance()->pingMainloopTimeout("Display:Cull");
		LLDrawable::incrementVisible();
		LLSpatialGroup::sNoDelete = TRUE;
		LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
		S32 occlusion = LLPipeline::sUseOcclusion;
		if (gDepthDirty)
		{
			LLPipeline::sUseOcclusion = llmin(occlusion, 1);
		}
		gDepthDirty = FALSE;
		LLGLStateValidator::checkStates();
		LLGLStateValidator::checkTextureChannels();
		LLGLStateValidator::checkClientArrays();
		static LLCullResult result;
		LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
		LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater() ? TRUE : FALSE;
		gPipeline.updateCull(*LLViewerCamera::getInstance(), result, water_clip);
		stop_glerror();
		LLGLStateValidator::checkStates();
		LLGLStateValidator::checkTextureChannels();
		LLGLStateValidator::checkClientArrays();
		BOOL to_texture = LLGLSLShader::sNoFixedFunction &&
						LLPipeline::sRenderGlow;
		LLAppViewer::instance()->pingMainloopTimeout("Display:Swap");
		{
			if (gResizeScreenTexture)
			{
				gResizeScreenTexture = FALSE;
				gPipeline.resizeScreenTexture();
			}
			gGL.setColorMask(true, true);
			glClearColor(0,0,0,0);
			LLGLStateValidator::checkStates();
			LLGLStateValidator::checkTextureChannels();
			LLGLStateValidator::checkClientArrays();
			if (!for_snapshot || LLPipeline::sRenderDeferred)
			{
				if (gFrameCount > 1)
				{
					gPipeline.generateSunShadow(*LLViewerCamera::getInstance());
				}
				LLVertexBuffer::unbind();
				LLGLStateValidator::checkStates();
				LLGLStateValidator::checkTextureChannels();
				LLGLStateValidator::checkClientArrays();
				const LLMatrix4a saved_proj = glh_get_current_projection();
				const LLMatrix4a saved_mod = glh_get_current_modelview();
				gGL.setViewport(0,0,512,512);
				LLVOAvatar::updateFreezeCounter() ;
				if(!LLPipeline::sMemAllocationThrottled)
				{
					LLVOAvatar::updateImpostors();
				}
				glh_set_current_projection(saved_proj);
				glh_set_current_modelview(saved_mod);
				gGL.matrixMode(LLRender::MM_PROJECTION);
				gGL.loadMatrix(saved_proj);
				gGL.matrixMode(LLRender::MM_MODELVIEW);
				gGL.loadMatrix(saved_mod);
				gViewerWindow->setup3DViewport();
				LLGLStateValidator::checkStates();
				LLGLStateValidator::checkTextureChannels();
				LLGLStateValidator::checkClientArrays();
			}
			gGL.syncContextState();
			glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}
		LLGLStateValidator::checkStates();
		LLGLStateValidator::checkClientArrays();
		{
			LLAppViewer::instance()->pingMainloopTimeout("Display:Imagery");
			gPipeline.generateWaterReflection(*LLViewerCamera::getInstance());
			gPipeline.renderPhysicsDisplay();
		}
		LLGLStateValidator::checkStates();
		LLGLStateValidator::checkClientArrays();
		LLAppViewer::instance()->pingMainloopTimeout("Display:UpdateImages");
		LLError::LLCallStacks::clear() ;
		LL_PUSH_CALLSTACKS();
		gFrameStats.start(LLFrameStats::IMAGE_UPDATE);
		{
			LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE);
			{
				LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_CLASS);
				LLViewerTexture::updateClass(LLViewerCamera::getInstance()->getVelocityStat()->getMean(),
											LLViewerCamera::getInstance()->getAngularVelocityStat()->getMean());
			}
			{
				LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_BUMP);
				gBumpImageList.updateImages();
			}
			{
				LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_LIST);
				F32 max_image_decode_time = 0.080f*gFrameIntervalSeconds;
				max_image_decode_time = llclamp(max_image_decode_time, 0.004f, 0.012f );
				gTextureList.updateImages(max_image_decode_time);
			}
		}
		LL_PUSH_CALLSTACKS();
		LLGLStateValidator::checkStates();
		LLGLStateValidator::checkClientArrays();
		LLAppViewer::instance()->pingMainloopTimeout("Display:StateSort");
		{
			LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
			gFrameStats.start(LLFrameStats::STATE_SORT);
			gPipeline.stateSort(*LLViewerCamera::getInstance(), result);
			stop_glerror();
			if (rebuild)
			{
				gFrameStats.start(LLFrameStats::REBUILD);
				gPipeline.rebuildPools();
				stop_glerror();
			}
		}
		LLGLStateValidator::checkStates();
		LLGLStateValidator::checkClientArrays();
		LLPipeline::sUseOcclusion = occlusion;
		{
			LLAppViewer::instance()->pingMainloopTimeout("Display:Sky");
			LL_RECORD_BLOCK_TIME(FTM_UPDATE_SKY);
			gSky.updateSky();
		}
		if ( (gUseWireframe) && ((!rlv_handler_t::isEnabled()) || (!gRlvAttachmentLocks.hasLockedHUD())) )
		{
			glClearColor(0.5f, 0.5f, 0.5f, 0.f);
			gGL.syncContextState();
			glClear(GL_COLOR_BUFFER_BIT);
			gGL.setPolygonMode(LLRender::PF_FRONT_AND_BACK, LLRender::PM_LINE);
		}
		LLVBOPool::deleteReleasedBuffers();
		LLAppViewer::instance()->pingMainloopTimeout("Display:RenderStart");
		LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater() ? TRUE : FALSE;
		LLGLStateValidator::checkStates();
		LLGLStateValidator::checkClientArrays();
		stop_glerror();
		if (to_texture)
		{
			gGL.setColorMask(true, true);
			if (LLPipeline::sRenderDeferred)
			{
				gPipeline.mDeferredScreen.bindTarget();
				glClearColor(1,0,1,1);
				gPipeline.mDeferredScreen.clear();
			}
			else
			{
				gPipeline.mScreen.bindTarget();
				if (LLPipeline::sUnderWaterRender)
				{
					const LLColor4 &col = LLDrawPoolWater::sWaterFogColor;
					glClearColor(col.mV[0], col.mV[1], col.mV[2], 0.f);
				}
				gPipeline.mScreen.clear();
			}
			gGL.setColorMask(true, false);
		}
		LLAppViewer::instance()->pingMainloopTimeout("Display:RenderGeom");
		if (!(LLAppViewer::instance()->logoutRequestSent() && LLAppViewer::instance()->hasSavedFinalSnapshot())
				&& !gRestoreGL)
		{
			LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
			static LLCachedControl<bool> render_ui_occlusion("RenderUIOcclusion", false);
			if(render_ui_occlusion && LLGLSLShader::sNoFixedFunction)
			{
				LLFloater* floaterp = gFloaterView->getFrontmost();
				if(floaterp && floaterp->getVisible() && floaterp->isBackgroundVisible() && floaterp->isBackgroundOpaque())
				{
					LLGLDepthTest depth(GL_TRUE, GL_TRUE);
					gGL.setColorMask(false, false);
					gOcclusionProgram.bind();
					LLRect rect = floaterp->calcScreenRect();
					rect.stretch(-1);
					gGL.matrixMode(LLRender::MM_PROJECTION);
					gGL.pushMatrix();
					gGL.loadIdentity();
					gGL.ortho(0.0f, gViewerWindow->getWindowWidth(), 0.0f, gViewerWindow->getWindowHeight(), 0.f, 1.0f);
					gGL.matrixMode(LLRender::MM_MODELVIEW);
					gGL.pushMatrix();
					gGL.loadIdentity();
					gGL.color4fv( LLColor4::white.mV );
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					gGL.begin( LLRender::TRIANGLE_STRIP );
						gGL.vertex3f(rect.mLeft, rect.mTop,0.f);
						gGL.vertex3f(rect.mLeft, rect.mBottom,0.f);
						gGL.vertex3f(rect.mRight, rect.mTop, 0.f);
						gGL.vertex3f(rect.mRight, rect.mBottom,0.f);
					gGL.end();
					gGL.matrixMode(LLRender::MM_PROJECTION);
					gGL.popMatrix();
					gGL.matrixMode(LLRender::MM_MODELVIEW);
					gGL.popMatrix();
					gOcclusionProgram.unbind();
				}
			}
			static LLCachedControl<bool> render_depth_pre_pass("RenderDepthPrePass", false);
			if (render_depth_pre_pass && LLGLSLShader::sNoFixedFunction)
			{
				LLGLDepthTest depth(GL_TRUE, GL_TRUE);
				LLGLEnable<GL_CULL_FACE> cull_face;
				gGL.setColorMask(false, false);
				U32 types[] = {
					LLRenderPass::PASS_SIMPLE,
					LLRenderPass::PASS_FULLBRIGHT,
					LLRenderPass::PASS_SHINY
				};
				U32 num_types = LL_ARRAY_SIZE(types);
				gOcclusionProgram.bind();
				for (U32 i = 0; i < num_types; i++)
				{
					gPipeline.renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, FALSE);
				}
				gOcclusionProgram.unbind();
			}
			gGL.setColorMask(true, false);
			LLGLEnable<GL_LIGHTING> lighting;
			LLGLEnable<GL_NORMALIZE> normalize;
			if (LLPipeline::sRenderDeferred)
			{
				gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance());
			}
			else
			{
				gPipeline.renderGeom(*LLViewerCamera::getInstance(), TRUE);
			}
			gGL.setColorMask(true, true);
			gGLPreviousModelView = gGLLastModelView;
			gGLLastModelView = gGLModelView;
			gGLLastProjection = gGLProjection;
			stop_glerror();
		}
		for (S32 i = gGLManager.mNumTextureImageUnits-1; i >= 0; --i)
		{
			if (gGL.getTexUnit((U32)i)->getCurrType() != LLTexUnit::TT_NONE)
			{
				gGL.getTexUnit((U32)i)->unbind(gGL.getTexUnit((U32)i)->getCurrType());
				gGL.getTexUnit((U32)i)->disable();
			}
		}
		LLAppViewer::instance()->pingMainloopTimeout("Display:RenderFlush");
		if (to_texture)
		{
			if (LLPipeline::sRenderDeferred)
			{
				gPipeline.mDeferredScreen.flush();
				if(gPipeline.mDeferredScreen.getFBO())
				{
					LLRenderTarget::copyContentsToFramebuffer(gPipeline.mDeferredScreen, 0, 0, gPipeline.mDeferredScreen.getWidth(),
															  gPipeline.mDeferredScreen.getHeight(), 0, 0,
															  gPipeline.mDeferredScreen.getWidth(),
															  gPipeline.mDeferredScreen.getHeight(),
															  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
				}
			}
			else
			{
				gPipeline.mScreen.flush();
				if(gPipeline.mScreen.getFBO())
				{
					LLRenderTarget::copyContentsToFramebuffer(gPipeline.mScreen, 0, 0, gPipeline.mScreen.getWidth(),
															  gPipeline.mScreen.getHeight(), 0, 0,
															  gPipeline.mScreen.getWidth(),
															  gPipeline.mScreen.getHeight(),
															  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
				}
			}
		}
		if (LLPipeline::sRenderDeferred)
		{
			gPipeline.renderDeferredLighting();
		}
		LLPipeline::sUnderWaterRender = FALSE;
		LLAppViewer::instance()->pingMainloopTimeout("Display:RenderUI");
		if (!for_snapshot || LLPipeline::sRenderDeferred)
		{
			LL_RECORD_BLOCK_TIME(FTM_RENDER_UI);
			gFrameStats.start(LLFrameStats::RENDER_UI);
			render_ui();
		}
		LLSpatialGroup::sNoDelete = FALSE;
		gPipeline.clearReferences();
		gPipeline.rebuildGroups();
	}
	LLAppViewer::instance()->pingMainloopTimeout("Display:FrameStats");
	gFrameStats.start(LLFrameStats::MISC_END);
	stop_glerror();
	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_resume();
		LLPipeline::sRenderFrameTest = FALSE;
	}
	display_stats();
	LLAppViewer::instance()->pingMainloopTimeout("Display:Done");
	static U32 sDisplayDoneFrames = 0;
	if ((++sDisplayDoneFrames % 600u) == 0u)
	{
		LLHeapDiag::markImportant("display_done_sample");
		LLHeapDiag::validateHeaps("display_done_sample");
	}
	gShiftFrame = false;
	LLVBOPool::deleteReleasedBuffers();
}
void render_hud_attachments()
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	const LLMatrix4a saved_proj = glh_get_current_projection();
	const LLMatrix4a saved_mod = glh_get_current_modelview();
	gAgentCamera.mHUDTargetZoom = llclamp(gAgentCamera.mHUDTargetZoom, (!gRlvAttachmentLocks.hasLockedHUD()) ? 0.1f : 0.85f, 1.f);
	gAgentCamera.mHUDCurZoom = lerp(gAgentCamera.mHUDCurZoom, gAgentCamera.mHUDTargetZoom, LLSmoothInterpolation::getInterpolant(0.03f));
	const bool show_hud = LLPipeline::sShowHUDAttachments;
	const bool disconnected = gDisconnected;
	if (show_hud && !disconnected && setup_hud_matrices())
	{
		LLPipeline::sRenderingHUDs = TRUE;
		LLCamera hud_cam = *LLViewerCamera::getInstance();
		hud_cam.setOrigin(-1.f,0,0);
		hud_cam.setAxes(LLVector3(1,0,0), LLVector3(0,1,0), LLVector3(0,0,1));
		LLViewerCamera::updateFrustumPlanes(hud_cam, TRUE);
		static const LLCachedControl<bool> render_hud_particles("RenderHUDParticles");
		bool render_particles = gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) && render_hud_particles;
		gPipeline.pushRenderTypeMask();
		gPipeline.clearAllRenderTypes();
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
		if (!render_particles)
		{
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
		}
		bool has_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
		S32 use_occlusion = LLPipeline::sUseOcclusion;
		LLPipeline::sUseOcclusion = 0;
		static LLCullResult result;
		LLSpatialGroup::sNoDelete = TRUE;
		LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
		gPipeline.updateCull(hud_cam, result);
		if (hud_teleport_logging_active())
		{
			static LLFrameTimer sHUDRenderLogTimer;
			if (sHUDRenderLogTimer.getElapsedTimeF32() >= 2.f)
			{
				sHUDRenderLogTimer.reset();
				S32 vis_bridges = 0;
				S32 vis_hud_bridges = 0;
				for (LLCullResult::bridge_iterator it = result.beginVisibleBridge();
					 it != result.endVisibleBridge(); ++it)
				{
					++vis_bridges;
					LLSpatialBridge* bridge = *it;
					if (bridge && bridge->asPartition() &&
						bridge->asPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD)
					{
						++vis_hud_bridges;
					}
				}
				LL_INFOS("HUDTeleport") << "render_hud DRAW vis_bridges=" << vis_bridges
					<< " vis_hud_bridges=" << vis_hud_bridges
					<< " tp_state=" << (S32)gAgent.getTeleportState()
					<< LL_ENDL;
				dump_hud_teleport_state("render_hud_draw");
			}
		}
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_SIMPLE);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_VOLUME);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_MATERIAL);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_SHINY);
		gPipeline.stateSort(hud_cam, result);
		gPipeline.renderGeom(hud_cam);
		LLSpatialGroup::sNoDelete = FALSE;
		render_hud_elements();
		gPipeline.popRenderTypeMask();
		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
		LLPipeline::sUseOcclusion = use_occlusion;
		LLPipeline::sRenderingHUDs = FALSE;
	}
	else if (hud_teleport_logging_active())
	{
		static LLFrameTimer sHUDSkipLogTimer;
		if (sHUDSkipLogTimer.getElapsedTimeF32() >= 2.f)
		{
			sHUDSkipLogTimer.reset();
			LL_INFOS("HUDTeleport") << "render_hud SKIP"
				<< " show=" << (show_hud ? 1 : 0)
				<< " disconnected=" << (disconnected ? 1 : 0)
				<< " hasHUD=" << ((isAgentAvatarValid() && gAgentAvatarp->hasHUDAttachment()) ? 1 : 0)
				<< " tp_state=" << (S32)gAgent.getTeleportState()
				<< LL_ENDL;
			dump_hud_teleport_state("render_hud_skip");
		}
	}
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();
	glh_set_current_projection(saved_proj);
	glh_set_current_modelview(saved_mod);
}
LLRect get_whole_screen_region()
{
	LLRect whole_screen = gViewerWindow->getWorldViewRectScaled();
	F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
	S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();
	if (zoom_factor > 1.f)
	{
		S32 num_horizontal_tiles = llceil(zoom_factor);
		S32 tile_width = ll_round((F32)gViewerWindow->getWorldViewWidthScaled() / zoom_factor);
		S32 tile_height = ll_round((F32)gViewerWindow->getWorldViewHeightScaled() / zoom_factor);
		int tile_y = sub_region / num_horizontal_tiles;
		int tile_x = sub_region - (tile_y * num_horizontal_tiles);
		whole_screen.setLeftTopAndSize(tile_x * tile_width, gViewerWindow->getWorldViewHeightScaled() - (tile_y * tile_height), tile_width, tile_height);
	}
	return whole_screen;
}
bool get_hud_matrices(const LLRect& screen_region, LLMatrix4a &proj, LLMatrix4a &model)
{
	if (isAgentAvatarValid() && gAgentAvatarp->hasHUDAttachment())
	{
		F32 zoom_level = gAgentCamera.mHUDCurZoom;
		LLBBox hud_bbox = gAgentAvatarp->getHUDBBox();
		F32 hud_depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
		proj = gGL.genOrtho(-0.5f * LLViewerCamera::getInstance()->getAspect(), 0.5f * LLViewerCamera::getInstance()->getAspect(), -0.5f, 0.5f, 0.f, hud_depth);
		proj.getRow<2>().copyComponent<2>(LLVector4a(-0.01f));
		F32 aspect_ratio = LLViewerCamera::getInstance()->getAspect();
		F32 scale_x = (F32)gViewerWindow->getWorldViewWidthScaled() / (F32)screen_region.getWidth();
		F32 scale_y = (F32)gViewerWindow->getWorldViewHeightScaled() / (F32)screen_region.getHeight();
		proj.applyTranslation_affine(
			clamp_rescale((F32)(screen_region.getCenterX() - screen_region.mLeft), 0.f, (F32)gViewerWindow->getWorldViewWidthScaled(), 0.5f * scale_x * aspect_ratio, -0.5f * scale_x * aspect_ratio),
			clamp_rescale((F32)(screen_region.getCenterY() - screen_region.mBottom), 0.f, (F32)gViewerWindow->getWorldViewHeightScaled(), 0.5f * scale_y, -0.5f * scale_y),
			0.f);
		proj.applyScale_affine(scale_x, scale_y, 1.f);
		model = OGL_TO_CFR_ROTATION;
		model.applyTranslation_affine(LLVector3(-hud_bbox.getCenterLocal().mV[VX] + (hud_depth * 0.5f), 0.f, 0.f));
		model.applyScale_affine(zoom_level);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
bool get_hud_matrices(LLMatrix4a &proj, LLMatrix4a &model)
{
	LLRect whole_screen = get_whole_screen_region();
	return get_hud_matrices(whole_screen, proj, model);
}
BOOL setup_hud_matrices()
{
	LLRect whole_screen = get_whole_screen_region();
	return setup_hud_matrices(whole_screen);
}
BOOL setup_hud_matrices(const LLRect& screen_region)
{
	LLMatrix4a proj, model;
	bool result = get_hud_matrices(screen_region, proj, model);
	if (!result) return result;
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.loadMatrix(proj);
	glh_set_current_projection(proj);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadMatrix(model);
	glh_set_current_modelview(model);
	return TRUE;
}
static LLTrace::BlockTimerStatHandle FTM_SWAP("Swap");
void render_ui(F32 zoom_factor, int subfield, bool tiling)
{
	LLGLStateValidator::checkStates();
	const LLMatrix4a saved_view = glh_get_current_modelview();
	if (!gSnapshot)
	{
		gGL.pushMatrix();
		gGL.loadMatrix(gGLLastModelView);
		glh_set_current_modelview(gGLLastModelView);
	}
	{
		BOOL to_texture = LLGLSLShader::sNoFixedFunction &&
							LLPipeline::sRenderGlow;
		if (to_texture)
		{
			gPipeline.renderBloom(gSnapshot, zoom_factor, subfield, tiling);
		}
		if (LLGLSLShader::sNoFixedFunction)
		{
			LLPostProcess::getInstance()->renderEffects(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
		}
		render_hud_elements();
		render_hud_attachments();
	}
	LLGLSDefault gls_default;
	LLGLSUIDefault gls_ui;
	{
		gGL.color4f(1,1,1,1);
		if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
		{
			LL_RECORD_BLOCK_TIME(FTM_RENDER_UI);
			if (!gDisconnected)
			{
				render_ui_3d();
				LLGLStateValidator::checkStates();
			}
			else
			{
				render_disconnected_background();
			}
			render_ui_2d();
			LLGLStateValidator::checkStates();
		}
		gGL.flush();
		{
			gViewerWindow->setup2DRender();
			gViewerWindow->updateDebugText();
			gViewerWindow->drawDebugText();
		}
		LLVertexBuffer::unbind();
	}
	if (!gSnapshot)
	{
		glh_set_current_modelview(saved_view);
		gGL.popMatrix();
	}
	if (gDisplaySwapBuffers)
	{
		LL_RECORD_BLOCK_TIME(FTM_SWAP);
		gViewerWindow->getWindow()->swapBuffers();
	}
	gDisplaySwapBuffers = TRUE;
}
void renderCoordinateAxes()
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.begin(LLRender::LINES);
		gGL.color3f(1.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(2.0f, 0.0f, 0.0f);
		gGL.vertex3f(3.0f, 0.0f, 0.0f);
		gGL.vertex3f(5.0f, 0.0f, 0.0f);
		gGL.vertex3f(6.0f, 0.0f, 0.0f);
		gGL.vertex3f(8.0f, 0.0f, 0.0f);
		gGL.vertex3f(11.0f, 1.0f, 1.0f);
		gGL.vertex3f(11.0f, -1.0f, -1.0f);
		gGL.vertex3f(11.0f, 1.0f, -1.0f);
		gGL.vertex3f(11.0f, -1.0f, 1.0f);
		gGL.color3f(0.0f, 1.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 2.0f, 0.0f);
		gGL.vertex3f(0.0f, 3.0f, 0.0f);
		gGL.vertex3f(0.0f, 5.0f, 0.0f);
		gGL.vertex3f(0.0f, 6.0f, 0.0f);
		gGL.vertex3f(0.0f, 8.0f, 0.0f);
		gGL.vertex3f(1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(-1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, -1.0f);
		gGL.color3f(0.0f, 0.0f, 1.0f);
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 2.0f);
		gGL.vertex3f(0.0f, 0.0f, 3.0f);
		gGL.vertex3f(0.0f, 0.0f, 5.0f);
		gGL.vertex3f(0.0f, 0.0f, 6.0f);
		gGL.vertex3f(0.0f, 0.0f, 8.0f);
		gGL.vertex3f(-1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(1.0f, -1.0f, 11.0f);
	gGL.end();
}
void draw_axes()
{
	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLVector3 v = gAgent.getPositionAgent();
	gGL.begin(LLRender::LINES);
		gGL.color3f(1.0f, 1.0f, 1.0f);
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 40.0f);
	gGL.end();
	gGL.pushMatrix();
		gGL.translatef( v.mV[VX], v.mV[VY], v.mV[VZ] );
		renderCoordinateAxes();
	gGL.popMatrix();
}
void render_ui_3d()
{
	LLGLSPipeline gls_pipeline;
	stop_glerror();
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}
	static const LLCachedControl<bool> show_axes("ShowAxes");
	if (show_axes)
	{
		draw_axes();
	}
	gViewerWindow->renderSelections(FALSE, FALSE, TRUE);
	stop_glerror();
}
extern void check_blend_funcs();
void render_ui_2d()
{
	LLGLSUIDefault gls_ui;
	gGL.setPolygonMode(LLRender::PF_FRONT_AND_BACK, LLRender::PM_FILL);
	gViewerWindow->setup2DRender();
	F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
	S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();
	if (zoom_factor > 1.f)
	{
		int pos_y = sub_region / llceil(zoom_factor);
		int pos_x = sub_region - (pos_y*llceil(zoom_factor));
		LLFontGL::sCurOrigin.mX -= ll_round((F32)gViewerWindow->getWindowWidthScaled() * (F32)pos_x / zoom_factor);
		LLFontGL::sCurOrigin.mY -= ll_round((F32)gViewerWindow->getWindowHeightScaled() * (F32)pos_y / zoom_factor);
	}
	stop_glerror();
	if (isAgentAvatarValid() && gAgentCamera.mHUDCurZoom < 0.98f)
	{
		gGL.pushMatrix();
		S32 half_width = (gViewerWindow->getWorldViewWidthScaled() / 2);
		S32 half_height = (gViewerWindow->getWorldViewHeightScaled() / 2);
		gGL.scalef(LLUI::getScaleFactor().mV[0], LLUI::getScaleFactor().mV[1], 1.f);
		gGL.translatef((F32)half_width, (F32)half_height, 0.f);
		F32 zoom = gAgentCamera.mHUDCurZoom;
		gGL.scalef(zoom,zoom,1.f);
		gGL.color4fv(LLColor4::white.mV);
		gl_rect_2d(-half_width, half_height, half_width, -half_height, FALSE);
		gGL.popMatrix();
		stop_glerror();
	}
	if(gDebugGL)check_blend_funcs();
	gViewerWindow->draw();
	if(gDebugGL)check_blend_funcs();
	LLFontGL::sCurOrigin.set(0, 0);
}
void render_disconnected_background()
{
	if (!gDisconnectedImagep && gDisconnected)
	{
		LL_INFOS() << "Loading last bitmap..." << LL_ENDL;
		std::string temp_str;
		temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + SCREEN_LAST_FILENAME;
		LLPointer<LLImageBMP> image_bmp = new LLImageBMP;
		if( !image_bmp->load(temp_str) )
		{
			return;
		}
		LLPointer<LLImageRaw> raw = new LLImageRaw;
		if (!image_bmp->decode(raw, 0.0f))
		{
			LL_INFOS() << "Bitmap decode failed" << LL_ENDL;
			gDisconnectedImagep = NULL;
			return;
		}
		U8 *rawp = raw->getData();
		S32 npixels = (S32)image_bmp->getWidth()*(S32)image_bmp->getHeight();
		for (S32 i = 0; i < npixels; i++)
		{
			S32 sum = 0;
			sum = *rawp + *(rawp+1) + *(rawp+2);
			sum /= 3;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
		}
		raw->expandToPowerOfTwo();
		gDisconnectedImagep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE );
		gStartTexture = gDisconnectedImagep;
	}
	S32 width = gViewerWindow->getWindowWidthScaled();
	S32 height = gViewerWindow->getWindowHeightScaled();
	if (gDisconnectedImagep)
	{
		if (LLGLSLShader::sNoFixedFunction)
		{
			gUIProgram.bind();
		}
		LLGLSUIDefault gls_ui;
		gViewerWindow->setup2DRender();
		gGL.pushMatrix();
		{
			const LLVector2& display_scale = gViewerWindow->getDisplayScale();
			gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);
			gGL.getTexUnit(0)->bind(gDisconnectedImagep);
			gGL.color4f(1.f, 1.f, 1.f, 1.f);
			gl_rect_2d_simple_tex(width, height);
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		}
		gGL.popMatrix();
		gGL.flush();
		if (LLGLSLShader::sNoFixedFunction)
		{
			gUIProgram.unbind();
		}
	}
}
void display_cleanup()
{
	gDisconnectedImagep = NULL;
}
