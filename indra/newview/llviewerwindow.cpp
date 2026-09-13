/** 
 * @file llviewerwindow.cpp
 * @brief Implementation of the LLViewerWindow class.
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
#include "llviewerwindow.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "llagent.h"
#include "llagentcamera.h"
#include "llmeshrepository.h"
#include "llpanellogin.h"
#include "llviewerkeyboard.h"
#include "llviewquery.h"
#include "llxmltree.h"
#include "llrender.h"
#include "llvoiceclient.h"
#include "llaudioengine.h"
#include "llassetstorage.h"
#include "llfontgl.h"
#include "llfontfreetype.h"
#include "llmousehandler.h"
#include "llrect.h"
#include "llsky.h"
#include "llstring.h"
#include "llui.h"
#include "lluuid.h"
#include "llview.h"
#include "llxfermanager.h"
#include "message.h"
#include "object_flags.h"
#include "lltimer.h"
#include "timing.h"
#include "llviewermenu.h"
#include "llmediaentry.h"
#include "raytrace.h"
#include "llbox.h"
#include "llchatbar.h"
#include "llconsole.h"
#include "lldebugview.h"
#include "lldir.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolwater.h"
#include "llmaniptranslate.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "statemachine/aifilepicker.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloatercustomize.h"
#include "llfloatereditui.h"
#include "llfloatersnapshot.h"
#include "llfloaterteleporthistory.h"
#include "llfloatertools.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llframestatview.h"
#include "llgesturemgr.h"
#include "llglheaders.h"
#include "llhoverview.h"
#include "llhudmanager.h"
#include "llhudview.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimageworker.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llmenuoptionpathfindingrebakenavmesh.h"
#include "llmodaldialog.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llnotify.h"
#include "lloverlaybar.h"
#include "llpreviewtexture.h"
#include "llprogressview.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llrootview.h"
#include "llrendersphere.h"
#include "llstartup.h"
#include "llstatusbar.h"
#include "llnavigationbar.h"
#include "llnotifyiconbar.h"
#include "llstatview.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llimview.h"
#include "lltexlayer.h"
#include "lltextbox.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "lltextureview.h"
#include "lltool.h"
#include "lltoolbar.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolmorph.h"
#include "lltoolpie.h"
#include "lltoolplacer.h"
#include "lltoolselectland.h"
#include "lltoolview.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llurldispatcher.h"
#include "llvieweraudio.h"
#include "llviewercamera.h"
#include "llviewergesture.h"
#include "llviewertexturelist.h"
#include "llviewerinventory.h"
#include "llviewerkeyboard.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewerstats.h"
#include "llvoavatarself.h"
#include "llvopartgroup.h"
#include "llvovolume.h"
#include "llworld.h"
#include "llworldmapview.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llviewerdisplay.h"
#include "llspatialpartition.h"
#include "llviewerjoystick.h"
#include "llviewernetwork.h"
#include "llpostprocess.h"
#include "llwearablelist.h"
#include "llfloaterinspect.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llfloaternotificationsconsole.h"
#include "llpanelnearbymedia.h"
#include "llmessagetemplate.h"
#include "rlvhandler.h"
#if LL_WINDOWS
#include <tchar.h>
#endif
void render_ui(F32 zoom_factor = 1.f, int subfield = 0, bool tiling = false);
LLBottomPanel* gBottomPanel = NULL;
extern BOOL gDebugClicks;
extern BOOL gDisplaySwapBuffers;
extern BOOL gDepthDirty;
extern BOOL gResizeScreenTexture;
LLViewerWindow	*gViewerWindow = NULL;
LLVelocityBar	*gVelocityBar = NULL;
LLFrameTimer	gMouseIdleTimer;
LLFrameTimer	gAwayTimer;
LLFrameTimer	gAwayTriggerTimer;
LLFrameTimer	gAlphaFadeTimer;
BOOL			gShowOverlayTitle = FALSE;
BOOL			gPickTransparent = TRUE;
LLViewerObject*  gDebugRaycastObject = NULL;
LLVOPartGroup* gDebugRaycastParticle = NULL;
LLVector4a       gDebugRaycastIntersection;
LLVector4a		gDebugRaycastParticleIntersection;
LLVector2        gDebugRaycastTexCoord;
LLVector4a       gDebugRaycastNormal;
LLVector4a       gDebugRaycastTangent;
S32				 gDebugRaycastFaceHit;
LLVector4a		 gDebugRaycastStart;
LLVector4a		 gDebugRaycastEnd;
BOOL				gDisplayWindInfo = FALSE;
BOOL				gDisplayCameraPos = FALSE;
BOOL				gDisplayNearestWater = FALSE;
BOOL				gDisplayFOV = FALSE;
S32 CHAT_BAR_HEIGHT = 28;
S32 OVERLAY_BAR_HEIGHT = 20;
const U8 NO_FACE = 255;
BOOL gQuietSnapshot = FALSE;
const F32 MIN_AFK_TIME = 2.f;
const F32 MAX_FAST_FRAME_TIME = 0.5f;
const F32 FAST_FRAME_INCREMENT = 0.1f;
const F32 MIN_DISPLAY_SCALE = 0.5f;
std::string	LLViewerWindow::sSnapshotBaseName;
std::string	LLViewerWindow::sSnapshotDir;
std::string	LLViewerWindow::sMovieBaseName;
extern void toggle_debug_menus(void*);
class LLDebugText
{
private:
	struct Line
	{
		Line(const std::string& in_text, S32 in_x, S32 in_y) : text(in_text), x(in_x), y(in_y) {}
		std::string text;
		S32 x,y;
	};
	LLViewerWindow *mWindow;
	typedef std::vector<Line> line_list_t;
	line_list_t mLineList;
	LLColor4 mTextColor;
	void addText(S32 x, S32 y, const std::string &text)
	{
		mLineList.push_back(Line(text, x, y));
	}
public:
	LLDebugText(LLViewerWindow* window) : mWindow(window) {}
	void update()
	{
		std::string wind_vel_text;
		std::string wind_vector_text;
		std::string rwind_vel_text;
		std::string rwind_vector_text;
		std::string audio_text;
		static const std::string beacon_particle = LLTrans::getString("BeaconParticle");
		static const std::string beacon_physical = LLTrans::getString("BeaconPhysical");
		static const std::string beacon_scripted = LLTrans::getString("BeaconScripted");
		static const std::string beacon_scripted_touch = LLTrans::getString("BeaconScriptedTouch");
		static const std::string beacon_sound = LLTrans::getString("BeaconSound");
		static const std::string beacon_media = LLTrans::getString("BeaconMedia");
		static const std::string particle_hiding = LLTrans::getString("ParticleHiding");
		mTextColor = LLColor4( 0.86f, 0.86f, 0.86f, 1.f );
		U32 xpos = mWindow->getWorldViewWidthScaled() - 350;
		U32 ypos = 64;
		const U32 y_inc = 20;
		static const LLCachedControl<bool> slb_show_fps("SLBShowFPS");
		if (slb_show_fps)
		{
			addText(xpos+280, ypos+5, llformat("FPS %3.1f", LLViewerStats::getInstance()->mFPSStat.getMeanPerSec()));
			ypos += y_inc;
		}
		static const LLCachedControl<bool> debug_show_time("DebugShowTime");
		if (debug_show_time)
		{
			{
			const U32 y_inc2 = 15;
				LLFrameTimer& timer = gTextureTimer;
				F32 time = timer.getElapsedTimeF32();
				S32 hours = (S32)(time / (60*60));
				S32 mins = (S32)((time - hours*(60*60)) / 60);
				S32 secs = (S32)((time - hours*(60*60) - mins*60));
				addText(xpos, ypos, llformat("Texture: %d:%02d:%02d", hours,mins,secs)); ypos += y_inc2;
			}
			F32 time = gFrameTimeSeconds;
			S32 hours = (S32)(time / (60*60));
			S32 mins = (S32)((time - hours*(60*60)) / 60);
			S32 secs = (S32)((time - hours*(60*60) - mins*60));
			addText(xpos, ypos, llformat("Time: %d:%02d:%02d", hours,mins,secs)); ypos += y_inc;
		}
		static const LLCachedControl<bool> analyze_target_texture("AnalyzeTargetTexture", false);
		if(analyze_target_texture)
		{
			LLSelectNode* nodep = LLSelectMgr::instance().getPrimaryHoverNode();
			LLObjectSelectionHandle handle = LLSelectMgr::instance().getHoverObjects();
			if(nodep || handle.notNull())
			{
				LLViewerObject* obj1 = nodep ? nodep->getObject() : NULL;
				LLViewerObject* obj2 = handle ? handle->getPrimaryObject() : NULL;
				LLViewerObject* obj = obj1 ? obj1 : obj2;
				if(obj)
				{
					S32 te = nodep ? nodep->getLastSelectedTE() : -1;
					if(te >= 0)
					{
						LLViewerTexture* imagep = obj->getTEImage(te);
						if(imagep && imagep != (LLViewerTexture*)LLViewerFetchedTexture::sDefaultImagep.get())
						{
							static const LLCachedControl<bool> use_rmse_auto_mask("SHUseRMSEAutoMask",false);
							static const LLCachedControl<F32> auto_mask_max_rmse("SHAutoMaskMaxRMSE",.09f);
							static const LLCachedControl<F32> auto_mask_max_mid("SHAutoMaskMaxMid", .25f);
							addText(xpos, ypos, llformat("Mask: %s", imagep->getIsAlphaMask(use_rmse_auto_mask ? auto_mask_max_rmse : -1.f, auto_mask_max_mid) ? "TRUE":"FALSE")); ypos += y_inc;
							addText(xpos, ypos, llformat("ID: %s", imagep->getID().asString().c_str())); ypos += y_inc;
						}
					}
				}
			}
		}
#if LL_WINDOWS
		static const LLCachedControl<bool> debug_show_memory("DebugShowMemory");
		if (debug_show_memory)
		{
			addText(xpos, ypos, llformat("Memory: %d (KB)", LLMemory::getCurrentRSS() / 1024));
			ypos += y_inc;
		}
#endif
		if (gDisplayCameraPos)
		{
			std::string camera_view_text;
			std::string camera_center_text;
			std::string agent_view_text;
			std::string agent_left_text;
			std::string agent_center_text;
			std::string agent_root_center_text;
			LLVector3d tvector;
			tvector = gAgent.getPositionGlobal();
			agent_center_text = llformat("AgentCenter  %f %f %f",
										 (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
			if (isAgentAvatarValid())
			{
				tvector = gAgent.getPosGlobalFromAgent(gAgentAvatarp->mRoot->getWorldPosition());
				agent_root_center_text = llformat("AgentRootCenter %f %f %f",
												  (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
			}
			else
			{
				agent_root_center_text = "---";
			}
			tvector = LLVector4(gAgent.getFrameAgent().getAtAxis());
			agent_view_text = llformat("AgentAtAxis  %f %f %f",
									   (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
			tvector = LLVector4(gAgent.getFrameAgent().getLeftAxis());
			agent_left_text = llformat("AgentLeftAxis  %f %f %f",
									   (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
			tvector = gAgentCamera.getCameraPositionGlobal();
			camera_center_text = llformat("CameraCenter %f %f %f",
										  (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
			tvector = LLVector4(LLViewerCamera::getInstance()->getAtAxis());
			camera_view_text = llformat("CameraAtAxis    %f %f %f",
										(F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
			addText(xpos, ypos, agent_center_text);  ypos += y_inc;
			addText(xpos, ypos, agent_root_center_text);  ypos += y_inc;
			addText(xpos, ypos, agent_view_text);  ypos += y_inc;
			addText(xpos, ypos, agent_left_text);  ypos += y_inc;
			addText(xpos, ypos, camera_center_text);  ypos += y_inc;
			addText(xpos, ypos, camera_view_text);  ypos += y_inc;
		}
		if (gDisplayWindInfo)
		{
			wind_vel_text = llformat("Wind velocity %.2f m/s", gWindVec.magVec());
			wind_vector_text = llformat("Wind vector   %.2f %.2f %.2f", gWindVec.mV[0], gWindVec.mV[1], gWindVec.mV[2]);
			rwind_vel_text = llformat("RWind vel %.2f m/s", gRelativeWindVec.magVec());
			rwind_vector_text = llformat("RWind vec   %.2f %.2f %.2f", gRelativeWindVec.mV[0], gRelativeWindVec.mV[1], gRelativeWindVec.mV[2]);
			addText(xpos, ypos, wind_vel_text);  ypos += y_inc;
			addText(xpos, ypos, wind_vector_text);  ypos += y_inc;
			addText(xpos, ypos, rwind_vel_text);  ypos += y_inc;
			addText(xpos, ypos, rwind_vector_text);  ypos += y_inc;
		}
		if (gDisplayWindInfo)
		{
			if (gAudiop)
			{
				audio_text= llformat("Audio for wind: %d", gAudiop->isWindEnabled());
			}
			addText(xpos, ypos, audio_text);  ypos += y_inc;
		}
		if (gDisplayFOV)
		{
			addText(xpos, ypos, llformat("FOV: %2.1f deg", RAD_TO_DEG * LLViewerCamera::getInstance()->getView()));
			ypos += y_inc;
		}
		static const LLCachedControl<bool> debug_show_render_info("DebugShowRenderInfo");
		if (debug_show_render_info)
		{
			if (!LLGLSLShader::sNoFixedFunction)
			{
				addText(xpos, ypos, "Shaders Disabled");
				ypos += y_inc;
			}
			if (gGLManager.mHasATIMemInfo)
			{
				S32 meminfo[4];
				glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, meminfo);
				addText(xpos, ypos, llformat("%.2f MB Texture Memory Free", meminfo[0]/1024.f));
				ypos += y_inc;
				if (gGLManager.mHasVertexBufferObject)
				{
					glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, meminfo);
					addText(xpos, ypos, llformat("%.2f MB VBO Memory Free", meminfo[0]/1024.f));
					ypos += y_inc;
				}
			}
			else if (gGLManager.mHasNVXMemInfo)
			{
				S32 free_memory;
				glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &free_memory);
				addText(xpos, ypos, llformat("%.2f MB Video Memory Free", free_memory/1024.f));
				ypos += y_inc;
			}
			addText(xpos, ypos, llformat("%d MB Index Data (%d MB Pooled, %d KIndices)", LLVertexBuffer::sAllocatedIndexBytes/(1024*1024), LLVBOPool::sIndexBytesPooled/(1024*1024), LLVertexBuffer::sIndexCount/1024));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d MB Vertex Data (%d MB Pooled, %d KVerts)", LLVertexBuffer::sAllocatedBytes/(1024*1024), LLVBOPool::sBytesPooled/(1024*1024), LLVertexBuffer::sVertexCount/1024));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Vertex Buffers", LLVertexBuffer::sGLCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Mapped Buffers", LLVertexBuffer::sMappedCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Vertex Buffer Binds", LLVertexBuffer::sBindCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Vertex Buffer Sets", LLVertexBuffer::sSetCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Texture Binds", LLImageGL::sBindCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Unique Textures", LLImageGL::sUniqueCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Render Calls", gPipeline.mBatchCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Matrix Ops", gPipeline.mMatrixOpCount));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%d Texture Matrix Ops", gPipeline.mTextureMatrixOps));
			ypos += y_inc;
			gPipeline.mTextureMatrixOps = 0;
			gPipeline.mMatrixOpCount = 0;
			if (gPipeline.mBatchCount > 0)
			{
				addText(xpos, ypos, llformat("Batch min/max/mean: %d/%d/%d", gPipeline.mMinBatchSize, gPipeline.mMaxBatchSize,
					gPipeline.mTrianglesDrawn/gPipeline.mBatchCount));
				gPipeline.mMinBatchSize = gPipeline.mMaxBatchSize;
				gPipeline.mMaxBatchSize = 0;
				gPipeline.mBatchCount = 0;
			}
			ypos += y_inc;
			addText(xpos,ypos, llformat("%d/%d Nodes visible", gPipeline.mNumVisibleNodes, LLSpatialGroup::sNodeCount));
			ypos += y_inc;
			if (!LLSpatialGroup::sPendingQueries.empty())
			{
				addText(xpos,ypos, llformat("%d Queries pending", LLSpatialGroup::sPendingQueries.size()));
				ypos += y_inc;
			}
			addText(xpos,ypos, llformat("%d Avatars visible", LLVOAvatar::sNumVisibleAvatars));
			ypos += y_inc;
			addText(xpos,ypos, llformat("%d Lights visible", LLPipeline::sVisibleLightCount));
			ypos += y_inc;
			S32 total_objects = gObjectList.getNumObjects();
			S32 ID_objects = gObjectList.mUUIDObjectMap.size();
			S32 dead_objects = gObjectList.mNumDeadObjects;
			S32 dead_object_list = gObjectList.mDeadObjects.size();
			S32 dead_object_check = 0;
			S32 total_avatars = 0;
			S32 ID_avatars = gObjectList.mUUIDAvatarMap.size();
			S32 dead_avatar_list = 0;
			S32 dead_avatar_check = 0;
			S32 orphan_parents = gObjectList.getOrphanParentCount();
			S32 orphan_parents_check = gObjectList.mOrphanParents.size();
			S32 orphan_children = gObjectList.mOrphanChildren.size();
			S32 orphan_total = gObjectList.getOrphanCount();
			S32 orphan_child_attachments = 0;
			for(U32 i = 0;i<gObjectList.mObjects.size();++i)
			{
				LLViewerObject *obj = gObjectList.mObjects[i];
				if(obj)
				{
					if(obj->isAvatar())
						++total_avatars;
					if(obj->isDead())
					{
						++dead_object_check;
						if(obj->isAvatar())
							++dead_avatar_check;
					}
				}
			}
			for(auto it = gObjectList.mDeadObjects.begin();it!=gObjectList.mDeadObjects.end();++it)
			{
				LLViewerObject *obj = gObjectList.findObject(*it);
				if(obj && obj->isAvatar())
					++dead_avatar_list;
			}
			for(std::vector<LLViewerObjectList::OrphanInfo>::iterator it = gObjectList.mOrphanChildren.begin();it!=gObjectList.mOrphanChildren.end();++it)
			{
				LLViewerObject *obj = gObjectList.findObject(it->mChildInfo);
				if(obj && obj->isAttachment())
					++orphan_child_attachments;
			}
			addText(xpos,ypos, llformat("%d|%d (%d|%d|%d) Objects", total_objects, ID_objects, dead_objects, dead_object_list,dead_object_check));
			ypos += y_inc;
			addText(xpos,ypos, llformat("%d|%d (%d|%d) Avatars", total_avatars, ID_avatars, dead_avatar_list,dead_avatar_check));
			ypos += y_inc;
			addText(xpos,ypos, llformat("%d (%d|%d %d %d) Orphans", orphan_total, orphan_parents, orphan_parents_check,orphan_children, orphan_child_attachments));
			ypos += y_inc;
			if (gMeshRepo.meshRezEnabled())
			{
				addText(xpos, ypos, llformat("%.3f MB Mesh Data Received", LLMeshRepository::sBytesReceived/(1024.f*1024.f)));
				ypos += y_inc;
				addText(xpos, ypos, llformat("%d/%d Mesh HTTP Requests/Retries", LLMeshRepository::sHTTPRequestCount,
					LLMeshRepository::sHTTPRetryCount));
				ypos += y_inc;
				addText(xpos, ypos, llformat("%d/%d Mesh LOD Pending/Processing", LLMeshRepository::sLODPending, (U32)LLMeshRepository::sLODProcessing));
				ypos += y_inc;
				addText(xpos, ypos, llformat("%.3f/%.3f MB Mesh Cache Read/Write ", LLMeshRepository::sCacheBytesRead/(1024.f*1024.f), LLMeshRepository::sCacheBytesWritten/(1024.f*1024.f)));
				ypos += y_inc;
			}
			addText(xpos, ypos, llformat("%d/%d bytes allocted to messages", sMsgDataAllocSize, sMsgdataAllocCount));
			LLVertexBuffer::sBindCount = LLImageGL::sBindCount =
				LLVertexBuffer::sSetCount = LLImageGL::sUniqueCount =
				gPipeline.mNumVisibleNodes = LLPipeline::sVisibleLightCount = 0;
		}
		sMsgDataAllocSize = 0;
		sMsgdataAllocCount = 0;
		static const LLCachedControl<bool> debug_show_render_matrices("DebugShowRenderMatrices");
		if (debug_show_render_matrices)
		{
			F32* m = gGLProjection.getF32ptr();
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[12], m[13], m[14], m[15]));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[8], m[9], m[10], m[11]));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[4], m[5], m[6], m[7]));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[0], m[1], m[2], m[3]));
			ypos += y_inc;
			addText(xpos, ypos, "Projection Matrix");
			ypos += y_inc;
			m = gGLModelView.getF32ptr();
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[12], m[13], m[14], m[15]));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[8], m[9], m[10], m[11]));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[4], m[5], m[6], m[7]));
			ypos += y_inc;
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", m[0], m[1], m[2], m[3]));
			ypos += y_inc;
			addText(xpos, ypos, "View Matrix");
			ypos += y_inc;
		}
		static const LLCachedControl<bool> debug_show_color("DebugShowColor");
		if (debug_show_color)
		{
			U8 color[4];
			LLCoordGL coord = gViewerWindow->getCurrentMouse();
			glReadPixels(coord.mX, coord.mY, 1,1,GL_RGBA, GL_UNSIGNED_BYTE, color);
			addText(xpos, ypos, llformat("%d %d %d %d", color[0], color[1], color[2], color[3]));
			ypos += y_inc;
		}
		static const LLCachedControl<bool> DebugShowPrivateMem("DebugShowPrivateMem",false);
		if (DebugShowPrivateMem)
		{
			LLPrivateMemoryPoolManager::getInstance()->updateStatistics() ;
			addText(xpos, ypos, llformat("Total Reserved(KB): %d", LLPrivateMemoryPoolManager::getInstance()->mTotalReservedSize / 1024));
			ypos += y_inc;
			addText(xpos, ypos, llformat("Total Allocated(KB): %d", LLPrivateMemoryPoolManager::getInstance()->mTotalAllocatedSize / 1024));
			ypos += y_inc;
		}
		static const LLCachedControl<bool> beacons_visible("BeaconsVisible",false);
		if (LLPipeline::getRenderBeacons(NULL) && beacons_visible)
		{
			if (LLPipeline::getRenderMOAPBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing media beacons (white)");
				ypos += y_inc;
			}
			if (LLPipeline::toggleRenderTypeControlNegated((void*)LLPipeline::RENDER_TYPE_PARTICLES))
			{
				addText(xpos, ypos, particle_hiding);
				ypos += y_inc;
			}
			if (LLPipeline::getRenderParticleBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing particle beacons (blue)");
				ypos += y_inc;
			}
			if (LLPipeline::getRenderSoundBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing sound beacons (blue/cyan/green/yellow/red)");
				ypos += y_inc;
			}
			if (LLPipeline::getRenderScriptedBeacons(NULL))
			{
				addText(xpos, ypos, beacon_scripted);
				ypos += y_inc;
			}
			else
				if (LLPipeline::getRenderScriptedTouchBeacons(NULL))
				{
					addText(xpos, ypos, beacon_scripted_touch);
					ypos += y_inc;
				}
			if (LLPipeline::getRenderPhysicalBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing physical object beacons (green)");
				ypos += y_inc;
			}
		}
	}
	void draw()
	{
		for (line_list_t::iterator iter = mLineList.begin();
			 iter != mLineList.end(); ++iter)
		{
			const Line& line = *iter;
			LLFontGL::getFontMonospace()->renderUTF8(line.text, 0, (F32)line.x, (F32)line.y, mTextColor,
											 LLFontGL::LEFT, LLFontGL::TOP,
											 LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, S32_MAX, NULL, FALSE);
		}
		mLineList.clear();
	}
};
void LLViewerWindow::updateDebugText()
{
	mDebugText->update();
}
bool LLViewerWindow::shouldShowToolTipFor(LLMouseHandler *mh)
{
	if (mToolTip && mh)
	{
		LLMouseHandler::EShowToolTip showlevel = mh->getShowToolTip();
		return (
			showlevel == LLMouseHandler::SHOW_ALWAYS ||
			(showlevel == LLMouseHandler::SHOW_IF_NOT_BLOCKED &&
			 !mToolTipBlocked)
			);
	}
	return false;
}
BOOL LLViewerWindow::handleAnyMouseClick(LLWindow *window,  LLCoordGL pos, MASK mask, LLMouseHandler::EClickType clicktype, BOOL down)
{
	std::string buttonname;
	std::string buttonstatestr;
	BOOL handled = FALSE;
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = ll_round((F32)x / mDisplayScale.mV[VX]);
	y = ll_round((F32)y / mDisplayScale.mV[VY]);
	if (down)
		{
		buttonstatestr = "down";
		}
	else
		{
		buttonstatestr = "up";
		}
	switch (clicktype)
	{
	case LLMouseHandler::CLICK_LEFT:
		mLeftMouseDown = down;
		buttonname = "Left";
		break;
	case LLMouseHandler::CLICK_RIGHT:
		mRightMouseDown = down;
		buttonname = "Right";
		break;
	case LLMouseHandler::CLICK_MIDDLE:
		mMiddleMouseDown = down;
		buttonname = "Middle";
		break;
	case LLMouseHandler::CLICK_DOUBLELEFT:
		mLeftMouseDown = down;
		buttonname = "Left Double Click";
		break;
	}
	LLView::sMouseHandlerMessage.clear();
	if (gMenuBarView)
	{
		gMenuBarView->resetMenuTrigger();
	}
	if (gDebugClicks)
	{
		LL_INFOS() << "ViewerWindow " << buttonname << " mouse " << buttonstatestr << " at " << x << "," << y << LL_ENDL;
	}
	if (down)
	{
		mWindow->captureMouse();
	}
	else
	{
		mWindow->releaseMouse();
	}
	gMouseIdleTimer.reset();
	if (down)
	{
		mToolTipBlocked = TRUE;
		mToolTip->setVisible(FALSE);
	}
	if (gHoverView)
	{
		gHoverView->cancelHover();
	}
	if (LLToolMgr::getInstance()->getCurrentTool()->clipMouseWhenDown())
	{
		mWindow->setMouseClipping(down);
	}
	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		if (LLView::sDebugMouseHandling)
		{
			LL_INFOS() << buttonname << " Mouse " << buttonstatestr << " handled by captor " << mouse_captor->getName() << LL_ENDL;
		}
		return mouse_captor->handleAnyMouseClick(local_x, local_y, mask, clicktype, down);
	}
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x, local_y;
		top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
		if (down)
		{
			if (top_ctrl->pointInView(local_x, local_y))
			{
				return top_ctrl->handleAnyMouseClick(local_x, local_y, mask, clicktype, down)	;
			}
			else
			{
				gFocusMgr.setTopCtrl(NULL);
			}
		}
		else
			handled = top_ctrl->pointInView(local_x, local_y) && top_ctrl->handleMouseUp(local_x, local_y, mask);
	}
		if( !mRootView->pointInView(x, y) )
		{
			return TRUE;
		}
	if( mRootView->handleAnyMouseClick(x, y, mask, clicktype, down) )
	{
		if (LLView::sDebugMouseHandling)
		{
			LL_INFOS() << buttonname << " Mouse " << buttonstatestr << " " << LLView::sMouseHandlerMessage << LL_ENDL;
		}
		return TRUE;
	}
	else if (LLView::sDebugMouseHandling)
	{
		LL_INFOS() << buttonname << " Mouse " << buttonstatestr << " not handled by view" << LL_ENDL;
	}
	if(!gDisconnected && LLToolMgr::getInstance()->getCurrentTool()->handleAnyMouseClick( x, y, mask, clicktype, down ) )
	{
		return TRUE;
	}
	BOOL default_rtn = !down;
	return default_rtn;
}
BOOL LLViewerWindow::handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = TRUE;
	return handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_LEFT,down);
}
BOOL LLViewerWindow::handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = TRUE;
	if (handleAnyMouseClick(window, pos, mask,
				LLMouseHandler::CLICK_DOUBLELEFT, down))
	{
		return TRUE;
	}
	return handleMouseDown(window, pos, mask);
}
BOOL LLViewerWindow::handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = FALSE;
	return handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_LEFT,down);
}
BOOL LLViewerWindow::handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = ll_round((F32)x / mDisplayScale.mV[VX]);
	y = ll_round((F32)y / mDisplayScale.mV[VY]);
	LLView::sMouseHandlerMessage.clear();
	BOOL down = TRUE;
	BOOL handle = handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_RIGHT,down);
	if (handle)
		return handle;
	if (CAMERA_MODE_CUSTOMIZE_AVATAR != gAgentCamera.getCameraMode() && LLToolMgr::getInstance()->getCurrentTool() != LLToolPie::getInstance() && gAgent.isInitialized())
	{
		LLToolPie::getInstance()->handleRightMouseDown(x, y, mask);
	}
	return TRUE;
}
BOOL LLViewerWindow::handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = FALSE;
	return handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_RIGHT,down);
}
BOOL LLViewerWindow::handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = TRUE;
	LLVoiceClient::getInstance()->middleMouseState(true);
	handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_MIDDLE,down);
	return TRUE;
}
LLWindowCallbacks::DragNDropResult LLViewerWindow::handleDragNDrop( LLWindow *window, LLCoordGL pos, MASK mask, LLWindowCallbacks::DragNDropAction action, std::string data)
{
	LLWindowCallbacks::DragNDropResult result = LLWindowCallbacks::DND_NONE;
	const bool prim_media_dnd_enabled = gSavedSettings.getBOOL("PrimMediaDragNDrop");
	const bool slurl_dnd_enabled = gSavedSettings.getBOOL("SLURLDragNDrop");
	if ( prim_media_dnd_enabled || slurl_dnd_enabled )
	{
		switch(action)
		{
			case LLWindowCallbacks::DNDA_TRACK:
			case LLWindowCallbacks::DNDA_DROPPED:
			case LLWindowCallbacks::DNDA_START_TRACKING:
			{
				bool drop = (LLWindowCallbacks::DNDA_DROPPED == action);
				if (slurl_dnd_enabled)
				{
					LLSLURL dropped_slurl(data);
					if(dropped_slurl.isSpatial())
					{
						if (drop)
						{
							LLURLDispatcher::dispatch( dropped_slurl.getSLURLString(), "clicked", NULL, true );
							return LLWindowCallbacks::DND_MOVE;
						}
						return LLWindowCallbacks::DND_COPY;
					}
				}
				if (prim_media_dnd_enabled)
				{
					LLPickInfo pick_info = pickImmediate( pos.mX, pos.mY,
                                                          TRUE ,
                                                          FALSE );
					LLUUID object_id = pick_info.getObjectID();
					S32 object_face = pick_info.mObjectFace;
					std::string url = data;
					LL_DEBUGS() << "Object: picked at " << pos.mX << ", " << pos.mY << " - face = " << object_face << " - URL = " << url << LL_ENDL;
					LLViewerObject* vobjp = static_cast<LLViewerObject*>(pick_info.getObject());
					LLVOVolume *obj = vobjp ? vobjp->asVolume() : nullptr;
					if (obj && !obj->getRegion()->getCapability("ObjectMedia").empty())
					{
						LLTextureEntry *te = obj->getTE(object_face);
						bool allow_modify_url = obj->permModify() || obj->hasMediaPermission( te->getMediaData(), LLVOVolume::MEDIA_PERM_INTERACT );
						if (te && allow_modify_url )
						{
							if (drop)
							{
								if ( ! te->hasMedia() )
								{
									if ( obj->permModify() )
									{
										LLSD media_data;
										media_data[LLMediaEntry::HOME_URL_KEY] = url;
										media_data[LLMediaEntry::CURRENT_URL_KEY] = url;
										media_data[LLMediaEntry::AUTO_PLAY_KEY] = true;
										obj->syncMediaData(object_face, media_data, true, true);
										if (obj->getMediaImpl(object_face))
											obj->getMediaImpl(object_face)->navigateReload();
										obj->sendMediaDataUpdate();
										result = LLWindowCallbacks::DND_COPY;
									}
								}
								else
								{
									if (te->getMediaData()->checkCandidateUrl( url ) )
									{
										if (obj->getMediaImpl(object_face))
										{
											obj->getMediaImpl(object_face)->navigateTo(url);
										}
										else
										{
											LLSD media_data;
											media_data[LLMediaEntry::CURRENT_URL_KEY] = url;
											obj->syncMediaData(object_face, media_data, true, true);
											obj->sendMediaDataUpdate();
										}
										result = LLWindowCallbacks::DND_LINK;
									}
								}
								LLSelectMgr::getInstance()->unhighlightObjectOnly(mDragHoveredObject);
								mDragHoveredObject = NULL;
							}
							else
							{
								if (te->getMediaData() == NULL || te->getMediaData()->checkCandidateUrl(url))
								{
									if ( obj != mDragHoveredObject)
									{
										LLSelectMgr::getInstance()->unhighlightObjectOnly(mDragHoveredObject);
										mDragHoveredObject = obj;
										LLSelectMgr::getInstance()->highlightObjectOnly(mDragHoveredObject);
									}
									result = (! te->hasMedia()) ? LLWindowCallbacks::DND_COPY : LLWindowCallbacks::DND_LINK;
								}
							}
						}
					}
				}
			}
			break;
			case LLWindowCallbacks::DNDA_STOP_TRACKING:
			break;
		}
		if (prim_media_dnd_enabled &&
			result == LLWindowCallbacks::DND_NONE && !mDragHoveredObject.isNull())
		{
			LLSelectMgr::getInstance()->unhighlightObjectOnly(mDragHoveredObject);
			mDragHoveredObject = NULL;
		}
	}
	return result;
}
BOOL LLViewerWindow::handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = FALSE;
	LLVoiceClient::getInstance()->middleMouseState(false);
	handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_MIDDLE,down);
	return TRUE;
}
void LLViewerWindow::handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = ll_round((F32)x / mDisplayScale.mV[VX]);
	y = ll_round((F32)y / mDisplayScale.mV[VY]);
	mMouseInWindow = TRUE;
	LLCoordGL prev_saved_mouse_point = mCurrentMousePoint;
	LLCoordGL mouse_point(x, y);
	if (mouse_point != mCurrentMousePoint)
	{
		gMouseIdleTimer.reset();
	}
	saveLastMouse(mouse_point);
	BOOL mouse_actually_moved = !gFocusMgr.getMouseCapture() &&
			((prev_saved_mouse_point.mX != mCurrentMousePoint.mX) || (prev_saved_mouse_point.mY != mCurrentMousePoint.mY));
	mWindow->showCursorFromMouseMove();
	if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME
		&& !gDisconnected)
	{
		gAgent.clearAFK();
	}
	if(mouse_actually_moved)
	{
		mToolTipBlocked = FALSE;
	}
	if (gHoverView)
	{
		gHoverView->setTyping(FALSE);
	}
}
void LLViewerWindow::handleMouseLeave(LLWindow *window)
{
	llassert( gFocusMgr.getMouseCapture() == NULL );
	mMouseInWindow = FALSE;
	if (mToolTip)
	{
		mToolTip->setVisible( FALSE );
	}
}
BOOL LLViewerWindow::handleCloseRequest(LLWindow *window)
{
	LLAppViewer::instance()->userQuit();
	return FALSE;
}
void LLViewerWindow::handleQuit(LLWindow *window)
{
	LLAppViewer::instance()->forceQuit();
}
void LLViewerWindow::handleResize(LLWindow *window,  S32 width,  S32 height)
{
	reshape(width, height);
	mResDirty = true;
}
void LLViewerWindow::handleFocus(LLWindow *window)
{
	gFocusMgr.setAppHasFocus(TRUE);
	LLModalDialog::onAppFocusGained();
	gAgent.onAppFocusGained();
	LLToolMgr::getInstance()->onAppFocusGained();
	gShowTextEditCursor = TRUE;
	if (gKeyboard)
	{
		gKeyboard->resetMaskKeys();
	}
	gForegroundTime.unpause();
}
void LLViewerWindow::handleFocusLost(LLWindow *window)
{
	gFocusMgr.setAppHasFocus(FALSE);
	LLToolMgr::getInstance()->onAppFocusLost();
	gFocusMgr.setMouseCapture( NULL );
	if (gMenuBarView)
	{
		gMenuBarView->resetMenuTrigger();
	}
	showCursor();
	getWindow()->setMouseClipping(FALSE);
	gShowTextEditCursor = FALSE;
	if (gKeyboard)
	{
		gKeyboard->resetKeys();
	}
	gForegroundTime.pause();
}
BOOL LLViewerWindow::handleTranslatedKeyDown(KEY key,  MASK mask, BOOL repeated)
{
	LLVoiceClient::getInstance()->keyDown(key, mask);
	if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
        LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
        if (keyboard_focus && !keyboard_focus->wantsReturnKey())
    		return FALSE;
	}
	return gViewerKeyboard.handleKey(key, mask, repeated);
}
BOOL LLViewerWindow::handleTranslatedKeyUp(KEY key,  MASK mask)
{
	LLVoiceClient::getInstance()->keyUp(key, mask);
	LLToolCompInspect * tool_inspectp = LLToolCompInspect::getInstance();
	if (LLToolMgr::getInstance()->getCurrentTool() == tool_inspectp)
	{
		tool_inspectp->keyUp(key, mask);
	}
	return gViewerKeyboard.handleKeyUp(key, mask);
}
void LLViewerWindow::handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level)
{
	LLViewerJoystick::getInstance()->setCameraNeedsUpdate(true);
	gViewerKeyboard.scanKey(key, key_down, key_up, key_level);
}
BOOL LLViewerWindow::handleActivate(LLWindow *window, BOOL activated)
{
	if (mActive == (bool)activated) {
		return TRUE;
	}
	if (activated)
	{
		mActive = true;
		send_agent_resume();
		gAgent.clearAFK();
		if (mWindow->getFullscreen() && !mIgnoreActivate)
		{
			{
				LL_WARNS() << "Activating while quitting" << LL_ENDL;
			}
		}
		audio_update_volume(false);
	}
	else
	{
		mActive = false;
		if (gSavedSettings.getBOOL("AllowIdleAFK"))
		{
			gAgent.setAFK();
		}
		if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
		{
			gAgentCamera.changeCameraToDefault();
		}
		send_agent_pause();
		audio_update_volume(false);
	}
	return TRUE;
}
BOOL LLViewerWindow::handleActivateApp(LLWindow *window, BOOL activating)
{
	LLViewerJoystick::getInstance()->setNeedsReset(true);
	return FALSE;
}
void LLViewerWindow::handleMenuSelect(LLWindow *window,  S32 menu_item)
{
}
BOOL LLViewerWindow::handlePaint(LLWindow *window,  S32 x,  S32 y, S32 width,  S32 height)
{
#if LL_WINDOWS
	if (gNoRender)
	{
		HWND window_handle = (HWND)window->getPlatformWindow();
		PAINTSTRUCT ps;
		HDC hdc;
		RECT wnd_rect;
		wnd_rect.left = 0;
		wnd_rect.top = 0;
		wnd_rect.bottom = 200;
		wnd_rect.right = 500;
		hdc = BeginPaint(window_handle, &ps);
		FillRect(hdc, &wnd_rect, CreateSolidBrush(RGB(255, 255, 255)));
		std::string temp_str;
		temp_str = llformat( "FPS %3.1f Phy FPS %2.1f Time Dil %1.3f",
				LLViewerStats::getInstance()->mFPSStat.getMeanPerSec(),
				LLViewerStats::getInstance()->mSimPhysicsFPS.getPrev(0),
				LLViewerStats::getInstance()->mSimTimeDilation.getPrev(0));
		S32 len = temp_str.length();
		TextOutA(hdc, 0, 0, temp_str.c_str(), len);
		LLVector3d pos_global = gAgent.getPositionGlobal();
		temp_str = llformat( "Avatar pos %6.1lf %6.1lf %6.1lf", pos_global.mdV[0], pos_global.mdV[1], pos_global.mdV[2]);
		len = temp_str.length();
		TextOutA(hdc, 0, 25, temp_str.c_str(), len);
		TextOutA(hdc, 0, 50, "Set \"DisableRendering FALSE\" in settings.ini file to reenable", 61);
		EndPaint(window_handle, &ps);
		return TRUE;
	}
#endif
	return FALSE;
}
void LLViewerWindow::handleScrollWheel(LLWindow *window,  S32 clicks)
{
	handleScrollWheel( clicks );
}
void LLViewerWindow::handleWindowBlock(LLWindow *window)
{
	send_agent_pause();
}
void LLViewerWindow::handleWindowUnblock(LLWindow *window)
{
	send_agent_resume();
}
void LLViewerWindow::handleDataCopy(LLWindow *window, S32 data_type, void *data)
{
	const S32 SLURL_MESSAGE_TYPE = 0;
	switch (data_type)
	{
	case SLURL_MESSAGE_TYPE:
		std::string url = (const char*)data;
		LLMediaCtrl* web = NULL;
		const bool trusted_browser = false;
		if (LLURLDispatcher::dispatch(url, "", web, trusted_browser))
		{
			mWindow->bringToFront();
		}
		break;
	}
}
BOOL LLViewerWindow::handleTimerEvent(LLWindow *window)
{
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		LLViewerJoystick::getInstance()->updateStatus();
		return TRUE;
	}
	return FALSE;
}
BOOL LLViewerWindow::handleDeviceChange(LLWindow *window)
{
	if (!LLViewerJoystick::getInstance()->isJoystickInitialized() )
	{
		LLViewerJoystick::getInstance()->init(true);
		return TRUE;
	}
	return FALSE;
}
bool LLViewerWindow::handleDPIScaleChange(LLWindow *window, float xDPIScale, float yDPIScale, U32 width, U32 height)
{
	LL_INFOS() << "handleDPIScaleChange" << LL_ENDL;
	if (mDPIScaleX != xDPIScale || mDPIScaleY != yDPIScale)
	{
		LL_INFOS() << "handleDPIScaleChange APPLY" << LL_ENDL;
		mDPIScaleX = xDPIScale;
		mDPIScaleY = yDPIScale;
		if (!mWindow->getFullscreen()) {
			reshape(width ? width : getWindowWidthRaw(), height ? height : getWindowHeightRaw());
			return true;
		}
	}
	return false;
}
void LLViewerWindow::handlePingWatchdog(LLWindow *window, const char * msg)
{
	LLAppViewer::instance()->pingMainloopTimeout(msg);
}
void LLViewerWindow::handleResumeWatchdog(LLWindow *window)
{
	LLAppViewer::instance()->resumeMainloopTimeout();
}
void LLViewerWindow::handlePauseWatchdog(LLWindow *window)
{
	LLAppViewer::instance()->pauseMainloopTimeout();
}
std::string LLViewerWindow::translateString(const char* tag)
{
	return LLTrans::getString( std::string(tag) );
}
std::string LLViewerWindow::translateString(const char* tag,
		const std::map<std::string, std::string>& args)
{
	LLStringUtil::format_map_t args_copy;
	std::map<std::string,std::string>::const_iterator it = args.begin();
	for ( ; it != args.end(); ++it)
	{
		args_copy[it->first] = it->second;
	}
	return LLTrans::getString( std::string(tag), args_copy);
}
static const std::string font_dir()
{
	return gDirUtilp->getExecutableDir()
		;
}
LLViewerWindow::LLViewerWindow(
	const std::string& title, const std::string& name,
	S32 x, S32 y,
	S32 width, S32 height,
	BOOL fullscreen, BOOL ignore_pixel_depth)
:	mWindow(NULL),
	mActive(true),
	mWantFullscreen(fullscreen),
	mShowFullscreenProgress(FALSE),
	mWindowRectRaw(0, height, width, 0),
	mWindowRectScaled(0, height, width, 0),
	mLeftMouseDown(FALSE),
	mMiddleMouseDown(FALSE),
	mRightMouseDown(FALSE),
	mToolTip(NULL),
	mToolTipBlocked(FALSE),
	mMouseInWindow( FALSE ),
	mLastMask( MASK_NONE ),
	mToolStored( NULL ),
	mHideCursorPermanent( FALSE ),
	mCursorHidden(FALSE),
	mIgnoreActivate( FALSE ),
	mResDirty(false),
	mIsFullscreenChecked(false),
	mCurrResolutionIndex(0),
	mProgressView(NULL),
	mDPIScaleX(1.f),
	mDPIScaleY(1.f)
{
	LLNotificationChannel::buildChannel("VW_alerts", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alert"));
	LLNotificationChannel::buildChannel("VW_alertmodal", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alertmodal"));
	LLNotifications::instance().getChannel("VW_alerts")->connectChanged(&LLViewerWindow::onAlert);
	LLNotifications::instance().getChannel("VW_alertmodal")->connectChanged(&LLViewerWindow::onAlert);
	LLViewerWindow::sSnapshotBaseName = "Snapshot";
	LLViewerWindow::sMovieBaseName = "SLmovie";
	resetSnapshotLoc();
	S32 vsync_mode = gSavedSettings.getS32("SHRenderVsyncMode");
	mWindow = LLWindowManager::createWindow(this,
		title, name, x, y, width, height, 0,
		fullscreen,
		gNoRender,
		vsync_mode,
		!gNoRender,
		ignore_pixel_depth,
		LLRenderTarget::sUseFBO ? 0 : gSavedSettings.getU32("RenderFSAASamples"));
	if (!LLViewerShaderMgr::sInitialized)
	{
		LLViewerShaderMgr::sInitialized = TRUE;
		LLViewerShaderMgr::instance()->setShaders();
	}
	if (NULL == mWindow)
	{
		LLSplashScreen::update(LLTrans::getString("StartupRequireDriverUpdate"));
		LL_WARNS("Window") << "Failed to create window, to be shutting Down, be sure your graphics driver is updated." << LL_ENDL ;
		ms_sleep(5000) ;
		LLSplashScreen::update(LLTrans::getString("ShuttingDown"));
		LL_WARNS("Window") << "Unable to create window, be sure screen is set at 32-bit color in Control Panels->Display->Settings"
				<< LL_ENDL;
		LLAppViewer::instance()->fastQuit(1);
	}
	if (!LLAppViewer::instance()->restoreErrorTrap())
	{
		LL_WARNS("Window") << " Someone took over my signal/exception handler (post createWindow)!" << LL_ENDL;
	}
	const bool do_not_enforce = false;
	mWindow->setMinSize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT, do_not_enforce);
	LLCoordScreen scr;
    mWindow->getSize(&scr);
	if(fullscreen && ( scr.mX!=width || scr.mY!=height))
	{
		LL_WARNS() << "Fullscreen has forced us in to a different resolution now using "<<scr.mX<<" x "<<scr.mY<<LL_ENDL;
		gSavedSettings.setS32("FullScreenWidth",scr.mX);
		gSavedSettings.setS32("FullScreenHeight",scr.mY);
	}
	mDisplayScale.setVec(llmax(1.f / mWindow->getPixelAspectRatio(), 1.f), llmax(mWindow->getPixelAspectRatio(), 1.f));
	mDisplayScale.scaleVec(getUIScale());
	LLUI::setScaleFactor(mDisplayScale);
	{
		LLCoordWindow size;
		mWindow->getSize(&size);
		mWindowRectRaw.set(0, size.mY, size.mX, 0);
		mWindowRectScaled.set(0, ll_round((F32)size.mY / mDisplayScale.mV[VY]), ll_round((F32)size.mX / mDisplayScale.mV[VX]), 0);
	}
	LLFontManager::initClass();
	LL_DEBUGS("Window") << "Loading feature tables." << LL_ENDL;
	LLFeatureManager::getInstance()->init();
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderVBOEnable") ||
		!gGLManager.mHasVertexBufferObject)
	{
		gSavedSettings.setBOOL("RenderVBOEnable", FALSE);
	}
	LLVertexBuffer::initClass(gSavedSettings.getBOOL("RenderVBOEnable"), gSavedSettings.getBOOL("RenderVBOMappingDisable"));
	LL_INFOS("RenderInit") << "LLVertexBuffer initialization done." << LL_ENDL ;
	LLImageGL::initClass(LLViewerTexture::MAX_GL_IMAGE_CATEGORY) ;
	if (LLFeatureManager::getInstance()->isSafe()
		|| (gSavedSettings.getS32("LastFeatureVersion") != LLFeatureManager::getInstance()->getVersion())
		|| (gSavedSettings.getBOOL("ProbeHardwareOnStartup")))
	{
		LLFeatureManager::getInstance()->applyRecommendedSettings();
		gSavedSettings.setBOOL("ProbeHardwareOnStartup", FALSE);
	}
	if (!gGLManager.mHasDepthClamp)
	{
		LL_INFOS("RenderInit") << "Missing feature GL_ARB_depth_clamp. Void water might disappear in rare cases." << LL_ENDL;
	}
	if (gSavedSettings.getBOOL("RenderInitError"))
	{
		mInitAlert = "DisplaySettingsNoShaders";
		LLFeatureManager::getInstance()->setGraphicsLevel(0, false);
		gSavedSettings.setU32("RenderQualityPerformance", 0);
	}
	gTextureList.init();
	LLViewerTextureManager::init() ;
	gBumpImageList.init();
	if (!gNoRender)
	{
	LLFontGL::initClass( gSavedSettings.getF32("FontScreenDPI"),
								mDisplayScale.mV[VX],
								mDisplayScale.mV[VY],
								font_dir());
	}
	LLView::Params rvp;
	rvp.name("root");
	rvp.rect(mWindowRectScaled);
	rvp.mouse_opaque(false);
	rvp.follows.flags(FOLLOWS_NONE);
	mRootView = LLUICtrlFactory::create<LLRootView>(rvp);
	LLUI::setRootView(mRootView);
	mCurrentMousePoint.mX = getWindowWidthScaled() / 2;
	mCurrentMousePoint.mY = getWindowHeightScaled() / 2;
	gShowOverlayTitle = gSavedSettings.getBOOL("ShowOverlayTitle");
	mOverlayTitle = gSavedSettings.getString("OverlayTitle");
	LLStringUtil::replaceChar(mOverlayTitle, '_', ' ');
	gSavedSettings.getControl("NumpadControl")->firePropertyChanged();
	mDebugText = new LLDebugText(this);
	mWindow->postInitialized();
}
void LLViewerWindow::initGLDefaults()
{
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	if (!LLGLSLShader::sNoFixedFunction)
	{
		glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,LLColor4::black.mV);
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,LLColor4::white.mV);
		glShadeModel( GL_SMOOTH );
		gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	gGL.setAmbientLightColor(LLColor4::black);
	glCullFace(GL_BACK);
	gBox.prerender();
}
void LLViewerWindow::initBase()
{
	S32 height = getWindowHeightScaled();
	S32 width = getWindowWidthScaled();
	LLRect full_window(0, height, width, 0);
	adjustRectanglesForFirstUse(full_window);
	F32 gamma = gSavedSettings.getF32("RenderGamma");
	if (gamma != 0.0f)
	{
		getWindow()->setGamma(gamma);
	}
	LLRect floater_view_rect = full_window;
	floater_view_rect.mTop -= MENU_BAR_HEIGHT;
	floater_view_rect.mBottom += STATUS_BAR_HEIGHT + 12 + 16 + 2;
	S32 floater_view_bottom = gSavedSettings.getS32("FloaterViewBottom");
	if (floater_view_bottom >= 0)
	{
		floater_view_rect.mBottom = floater_view_bottom;
	}
	gFloaterView = new LLFloaterView("Floater View", floater_view_rect );
	gFloaterView->setVisible(TRUE);
	gSnapshotFloaterView = new LLSnapshotFloaterView("Snapshot Floater View", full_window);
	gSnapshotFloaterView->setVisible(FALSE);
	llassert( !gConsole );
	gConsole = new LLConsole(
		"console",
		getChatConsoleRect(),
		gSavedSettings.getS32("ChatFontSize"),
		gSavedSettings.getF32("ChatPersistTime") );
	gConsole->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
	getRootView()->addChild(gConsole);
	gDebugView = new LLDebugView("gDebugView", full_window);
	gDebugView->setFollowsAll();
	gDebugView->setVisible(TRUE);
	mRootView->addChild(gDebugView);
	mRootView->addChild(gFloaterView, -1);
	mRootView->addChild(gSnapshotFloaterView);
	LLRect notify_rect = full_window;
	notify_rect.mTop -= MENU_BAR_HEIGHT;
	notify_rect.mBottom += STATUS_BAR_HEIGHT;
	gNotifyBoxView = new LLNotifyBoxView("notify_container", notify_rect, FALSE, FOLLOWS_ALL);
	mRootView->addChild(gNotifyBoxView, -2);
	mToolTip = new LLTextBox( std::string("tool tip"), LLRect(0, 1, 1, 0 ) );
	mToolTip->setHPad( 4 );
	mToolTip->setVPad( 2 );
	mToolTip->setColor( gColors.getColor( "ToolTipTextColor" ) );
	mToolTip->setBorderColor( gColors.getColor( "ToolTipBorderColor" ) );
	mToolTip->setBorderVisible( FALSE );
	mToolTip->setBackgroundColor( gColors.getColor( "ToolTipBgColor" ) );
	mToolTip->setBackgroundVisible( TRUE );
	mToolTip->setFontStyle(LLFontGL::NORMAL);
	mToolTip->setBorderDropshadowVisible( TRUE );
	mToolTip->setVisible( FALSE );
	mProgressView = new LLProgressView(std::string("ProgressView"), full_window);
	mRootView->addChild(mProgressView);
	setShowProgress(FALSE);
	setProgressCancelButtonVisible(FALSE);
}
void adjust_rect_top_left(const std::string& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setLeftTopAndSize(0, window.getHeight(), r.getWidth(), r.getHeight());
		gSavedSettings.setRect(control, r);
	}
}
void adjust_rect_top_center(const std::string& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setLeftTopAndSize( window.getWidth()/2 - r.getWidth()/2,
			window.getHeight(),
			r.getWidth(),
			r.getHeight() );
		gSavedSettings.setRect(control, r);
	}
}
void adjust_rect_top_right(const std::string& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setLeftTopAndSize(window.getWidth() - r.getWidth(),
			window.getHeight(),
			r.getWidth(),
			r.getHeight());
		gSavedSettings.setRect(control, r);
	}
}
const S32 TOOLBAR_HEIGHT = 64;
void adjust_rect_bottom_left(const std::string& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setOriginAndSize(0, TOOLBAR_HEIGHT, r.getWidth(), r.getHeight());
		gSavedSettings.setRect(control, r);
	}
}
void adjust_rect_bottom_center(const std::string& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setOriginAndSize(
			window.getWidth()/2 - r.getWidth()/2,
			TOOLBAR_HEIGHT,
			r.getWidth(),
			r.getHeight());
		gSavedSettings.setRect(control, r);
	}
}
void adjust_rect_centered_partial_zoom(const std::string& control,
									   const LLRect& window)
{
	LLRect rect = gSavedSettings.getRect(control);
	if (rect.mLeft == 0 && rect.mBottom == 0)
	{
		S32 width = window.getWidth();
		S32 height = window.getHeight();
		rect.set(0, height-STATUS_BAR_HEIGHT, width, TOOL_BAR_HEIGHT);
		const F32 ZOOM_FRACTION = 0.8f;
		S32 dx = (S32)(width * (1.f - ZOOM_FRACTION));
		S32 dy = (S32)(height * (1.f - ZOOM_FRACTION));
		rect.stretch(-dx/2, -dy/2);
		gSavedSettings.setRect(control, rect);
	}
}
void LLViewerWindow::adjustRectanglesForFirstUse(const LLRect& window)
{
	LLRect r;
	adjust_rect_bottom_center("FloaterMoveRect2", window);
	adjust_rect_top_center("FloaterCameraRect3", window);
	adjust_rect_top_left("FloaterCustomizeAppearanceRect", window);
	adjust_rect_top_left("FloaterLandRect6", window);
	adjust_rect_top_left("FloaterFindRect2", window);
	adjust_rect_top_left("FloaterGestureRect3", window);
	adjust_rect_top_right("FloaterMiniMapRect", window);
	adjust_rect_top_left("FloaterBuildOptionsRect", window);
	adjust_rect_bottom_left("FloaterActiveSpeakersRect", window);
	adjust_rect_bottom_left("FloaterBumpRect", window);
	adjust_rect_bottom_left("FloaterRegionInfo", window);
	adjust_rect_bottom_left("FloaterEnvRect", window);
	adjust_rect_bottom_left("FloaterAdvancedSkyRect", window);
	adjust_rect_bottom_left("FloaterAdvancedWaterRect", window);
	adjust_rect_bottom_left("FloaterDayCycleRect", window);
	adjust_rect_top_right("FloaterStatisticsRect", window);
	r = gSavedSettings.getRect("FloaterInventoryRect");
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setOriginAndSize(
			window.getWidth() - r.getWidth(),
			0,
			r.getWidth(),
			r.getHeight());
		gSavedSettings.setRect("FloaterInventoryRect", r);
	}
	r = gSavedSettings.getRect("FloaterHUDRect2");
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setOriginAndSize(
			window.getWidth()/4 - r.getWidth()/2,
			2*window.getHeight()/3 - r.getHeight()/2,
			r.getWidth(),
			r.getHeight());
		gSavedSettings.setRect("FloaterHUDRect2", r);
	}
}
void LLViewerWindow::adjustControlRectanglesForFirstUse(const LLRect& window)
{
	adjust_rect_bottom_center("FloaterMoveRect2", window);
	adjust_rect_top_center("FloaterCameraRect3", window);
}
void LLViewerWindow::initWorldUI()
{
	pre_init_menus();
	if(!gMenuHolder)
	{
		init_menus();
	}
}
void LLViewerWindow::initWorldUI_postLogin()
{
	S32 height = mRootView->getRect().getHeight();
	S32 width = mRootView->getRect().getWidth();
	LLRect full_window(0, height, width, 0);
	if (gBottomPanel == NULL)
	{
		gBottomPanel = new LLBottomPanel(mRootView->getRect());
		mRootView->addChild(gBottomPanel);
		LLFloaterNearbyMedia::updateClass();
		gHoverView = new LLHoverView(std::string("gHoverView"), full_window);
		gHoverView->setVisible(TRUE);
		mRootView->addChild(gHoverView);
		gIMMgr = LLIMMgr::getInstance();
		gIMMgr->loadIgnoreGroup();
		gFloaterTools = new LLFloaterTools();
		gFloaterTools->setVisible(FALSE);
	}
	if ( gHUDView == NULL )
	{
		LLRect hud_rect = full_window;
		hud_rect.mBottom += 50;
		if (gMenuBarView)
		{
			hud_rect.mTop -= gMenuBarView->getRect().getHeight();
		}
		gHUDView = new LLHUDView(hud_rect);
		mRootView->addChildInBack(gHUDView);
	}
	LLPanel* panel_ssf_container = getRootView()->getChild<LLPanel>("state_management_buttons_container");
	panel_ssf_container->setVisible(TRUE);
	LLMenuOptionPathfindingRebakeNavmesh::getInstance()->initialize();
	if (!gStatusBar)
	{
		S32 menu_bar_height = gMenuBarView->getRect().getHeight();
		LLRect root_rect = getRootView()->getRect();
		LLRect status_rect(0, root_rect.getHeight(), root_rect.getWidth(), root_rect.getHeight() - menu_bar_height);
		gStatusBar = new LLStatusBar(std::string("status"), status_rect);
		gStatusBar->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_TOP);
		gStatusBar->reshape(root_rect.getWidth(), gStatusBar->getRect().getHeight(), TRUE);
		gStatusBar->translate(0, root_rect.getHeight() - gStatusBar->getRect().getHeight());
		gStatusBar->setBackgroundColor( gMenuBarView->getBackgroundColor() );
		getRootView()->addChild(gStatusBar);
		S32 nav_h = NAV_BAR_HEIGHT;
		if (nav_h <= 0)
		{
			nav_h = 29;
		}
		LLRect nav_rect(0, root_rect.getHeight() - menu_bar_height,
						root_rect.getWidth(), root_rect.getHeight() - menu_bar_height - nav_h);
		gNavigationBar = new LLNavigationBar(std::string("navigation_bar"), nav_rect);
		gNavigationBar->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_TOP);
		gNavigationBar->setBackgroundColor(gMenuBarView->getBackgroundColor());
		getRootView()->addChild(gNavigationBar);
		getRootView()->sendChildToFront(gNavigationBar);
		S32 icon_h = NOTIFY_ICON_BAR_HEIGHT;
		S32 icon_w = NOTIFY_ICON_BAR_WIDTH;
		if (icon_h <= 0)
		{
			icon_h = 27;
		}
		if (icon_w <= 0)
		{
			icon_w = 36;
		}
		const S32 icon_pad = 3;
		LLRect icon_rect(root_rect.getWidth() - icon_w - icon_pad,
						 root_rect.getHeight() - menu_bar_height - nav_h - icon_pad,
						 root_rect.getWidth() - icon_pad,
						 root_rect.getHeight() - menu_bar_height - nav_h - icon_pad - icon_h);
		gNotifyIconBar = new LLNotifyIconBar(std::string("notify_icon_bar"), icon_rect);
		getRootView()->addChild(gNotifyIconBar);
		getRootView()->sendChildToFront(gNotifyIconBar);
		gNavigationBar->updateFloaterViewTop();
		getRootView()->sendChildToFront(gMenuHolder);
		if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
		{
			LLFloaterChat::getInstance(LLSD())->loadHistory();
		}
		LLRect morph_view_rect = full_window;
		morph_view_rect.stretch( -STATUS_BAR_HEIGHT );
		morph_view_rect.mTop = full_window.mTop - 32;
		gMorphView = new LLMorphView(std::string("gMorphView"), morph_view_rect );
		mRootView->addChild(gMorphView);
		gMorphView->setVisible(FALSE);
		LLWorldMapView::initClass();
		adjust_rect_centered_partial_zoom("FloaterWorldMapRect2", full_window);
		gFloaterWorldMap = new LLFloaterWorldMap();
		gFloaterWorldMap->setVisible(FALSE);
		LLFloaterTeleportHistory::getInstance()->setVisible(FALSE);
		LLFloaterTeleportHistory::loadFile("teleport_history.xml");
		LLFloaterChatterBox::createInstance(LLSD());
	}
	mRootView->sendChildToFront(mProgressView);
}
void LLViewerWindow::shutdownViews()
{
	delete mDebugText;
	mDebugText = NULL;
	gSavedSettings.setS32("FloaterViewBottom", gFloaterView->getRect().mBottom);
	if (gMorphView)
	{
		gMorphView->setVisible(FALSE);
	}
	LL_INFOS() << "Global views cleaned." << LL_ENDL ;
	LLModalDialog::shutdownModals();
	LL_INFOS() << "LLModalDialog shut down." << LL_ENDL;
	delete mRootView;
	mRootView = NULL;
	LL_INFOS() << "RootView deleted." << LL_ENDL ;
	if(LLMenuOptionPathfindingRebakeNavmesh::instanceExists())
		LLMenuOptionPathfindingRebakeNavmesh::getInstance()->quit();
	gFloaterTools = NULL;
	gStatusBar = NULL;
	gNavigationBar = NULL;
	gNotifyIconBar = NULL;
	gIMMgr = NULL;
	gHoverView = NULL;
	gFloaterView		= NULL;
	gMorphView			= NULL;
	gHUDView = NULL;
	gNotifyBoxView = NULL;
	delete mToolTip;
	mToolTip = NULL;
}
void LLViewerWindow::shutdownGL()
{
	LLFontGL::destroyDefaultFonts();
	LLFontManager::cleanupClass();
	stop_glerror();
	gSky.cleanup();
	stop_glerror();
	LL_INFOS() << "Cleaning up pipeline" << LL_ENDL;
	gPipeline.cleanup();
	stop_glerror();
	LL_INFOS() << "Cleaning up wearables" << LL_ENDL;
	LLWearableList::instance().cleanup() ;
	gTextureList.shutdown();
	stop_glerror();
	gBumpImageList.shutdown();
	stop_glerror();
	LLWorldMapView::cleanupTextures();
	LLViewerTextureManager::cleanup() ;
	LLImageGL::cleanupClass() ;
	LL_INFOS() << "All textures and llimagegl images are destroyed!" << LL_ENDL ;
	LL_INFOS() << "Cleaning up select manager" << LL_ENDL;
	LLSelectMgr::getInstance()->cleanup();
	LL_INFOS() << "Stopping GL during shutdown" << LL_ENDL;
	if (!gNoRender)
	{
		stopGL(FALSE);
		stop_glerror();
	}
	gGL.shutdown();
	LLVertexBuffer::cleanupClass();
	LL_INFOS() << "LLVertexBuffer cleaned." << LL_ENDL ;
}
LLViewerWindow::~LLViewerWindow()
{
	LL_INFOS() << "Destroying Window" << LL_ENDL;
	destroyWindow();
	delete mDebugText;
	mDebugText = NULL;
}
void LLViewerWindow::setCursor( ECursorType c )
{
	mWindow->setCursor( c );
}
void LLViewerWindow::showCursor()
{
	mWindow->showCursor();
	mCursorHidden = FALSE;
}
void LLViewerWindow::hideCursor()
{
	if(mToolTip ) mToolTip->setVisible( FALSE );
	if (gHoverView)	gHoverView->cancelHover();
	mWindow->hideCursor();
	mCursorHidden = TRUE;
}
void LLViewerWindow::sendShapeToSim()
{
	LLMessageSystem* msg = gMessageSystem;
	if(!msg) return;
	msg->newMessageFast(_PREHASH_AgentHeightWidth);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_CircuitCode, gMessageSystem->mOurCircuitCode);
	msg->nextBlockFast(_PREHASH_HeightWidthBlock);
	msg->addU32Fast(_PREHASH_GenCounter, 0);
	U16 height16 = (U16) mWindowRectRaw.getHeight();
	U16 width16 = (U16) mWindowRectRaw.getWidth();
	msg->addU16Fast(_PREHASH_Height, height16);
	msg->addU16Fast(_PREHASH_Width, width16);
	gAgent.sendReliableMessage();
}
void LLViewerWindow::reshape(S32 width, S32 height)
{
	if (!LLApp::isExiting())
	{
		if (gNoRender)
		{
			return;
		}
		gWindowResized = TRUE;
		gGL.setViewport(0, 0, width, height );
		if (height > 0)
		{
			LLViewerCamera::getInstance()->setViewHeightInPixels( height );
			if (mWindow->getFullscreen())
			{
				LLViewerCamera::getInstance()->setAspect( getDisplayAspectRatio() );
			}
			else
			{
				LLViewerCamera::getInstance()->setAspect( width / (F32) height);
			}
		}
		mWindowRectRaw.mRight = mWindowRectRaw.mLeft + width;
		mWindowRectRaw.mTop = mWindowRectRaw.mBottom + height;
		calcDisplayScale();
		BOOL display_scale_changed = mDisplayScale != LLUI::getScaleFactor();
		LLUI::setScaleFactor(mDisplayScale);
		mWindowRectScaled.mRight = mWindowRectScaled.mLeft + ll_round((F32)width / mDisplayScale.mV[VX]);
		mWindowRectScaled.mTop = mWindowRectScaled.mBottom + ll_round((F32)height / mDisplayScale.mV[VY]);
		setup2DViewport();
		LLView::sForceReshape = display_scale_changed;
		if (gSavedSettings.getBOOL("LiruResizeRootWithScreen"))
		mRootView->reshape(llceil((F32)width / mDisplayScale.mV[VX]), llceil((F32)height / mDisplayScale.mV[VY]));
		LLView::sForceReshape = FALSE;
		if (display_scale_changed)
		{
			LLHUDObject::reshapeAll();
		}
		sendShapeToSim();
		gSavedSettings.setBOOL("FullScreen", mWantFullscreen);
		if (!mWindow->getFullscreen())
		{
			BOOL maximized = mWindow->getMaximized();
			gSavedSettings.setBOOL("WindowMaximized", maximized);
			LLCoordScreen window_size;
			if (!maximized
				&& mWindow->getSize(&window_size))
			{
				gSavedSettings.setS32("WindowWidth", window_size.mX);
				gSavedSettings.setS32("WindowHeight", window_size.mY);
			}
		}
		LLViewerStats::getInstance()->setStat(LLViewerStats::ST_WINDOW_WIDTH, (F64)width);
		LLViewerStats::getInstance()->setStat(LLViewerStats::ST_WINDOW_HEIGHT, (F64)height);
		gResizeScreenTexture = TRUE;
		LLLayoutStack::updateClass();
	}
}
void LLViewerWindow::setNormalControlsVisible( BOOL visible )
{
	if (gBottomPanel)
	{
		gBottomPanel->setVisible(visible);
		gBottomPanel->setEnabled(visible);
	}
	if ( gMenuBarView )
	{
		gMenuBarView->setVisible( visible );
		gMenuBarView->setEnabled( visible );
		setMenuBackgroundColor(gAgent.getGodLevel() > GOD_NOT,
			LLViewerLogin::getInstance()->isInProductionGrid());
	}
	if ( gStatusBar )
	{
		gStatusBar->setVisible( visible );
		gStatusBar->setEnabled( visible );
	}
}
void LLViewerWindow::setMenuBackgroundColor(bool god_mode, bool dev_grid)
{
	LLSD args;
	LLColor4 new_bg_color;
	if(god_mode && LLViewerLogin::getInstance()->isInProductionGrid())
	{
		new_bg_color = gColors.getColor( "MenuBarGodBgColor" );
	}
	else if(god_mode && !LLViewerLogin::getInstance()->isInProductionGrid())
	{
		new_bg_color = gColors.getColor( "MenuNonProductionGodBgColor" );
	}
	else if(!god_mode && !LLViewerLogin::getInstance()->isInProductionGrid())
	{
		new_bg_color = gColors.getColor( "MenuNonProductionBgColor" );
	}
	else
	{
		new_bg_color = gColors.getColor( "MenuBarBgColor" );
	}
	if(gMenuBarView)
	{
		gMenuBarView->setBackgroundColor( new_bg_color );
	}
	if(gStatusBar)
	{
		gStatusBar->setBackgroundColor( new_bg_color );
	}
}
void LLViewerWindow::drawDebugText()
{
	gGL.color4f(1,1,1,1);
	gGL.pushMatrix();
	gGL.pushUIMatrix();
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}
	{
		gGL.scaleUI(mDisplayScale.mV[VX], mDisplayScale.mV[VY], 1.f);
		mDebugText->draw();
	}
	gGL.popUIMatrix();
	gGL.popMatrix();
	gGL.flush();
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.unbind();
	}
}
extern void check_blend_funcs();
void LLViewerWindow::draw()
{
#if LL_DEBUG
	LLView::sIsDrawing = TRUE;
#endif
	stop_glerror();
	LLUI::setLineWidth(1.f);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadIdentity();
	static const  LLCachedControl<bool> display_timecode("DisplayTimecode",false);
	if (display_timecode)
	{
		std::string text;
		gGL.loadIdentity();
		microsecondsToTimecodeString(gFrameTime,text);
		const LLFontGL* font = LLFontGL::getFontSansSerif();
		font->renderUTF8(text, 0,
						ll_round((getWindowWidthScaled()/2)-100.f),
						ll_round((getWindowHeightScaled()-60.f)),
			LLColor4( 1.f, 1.f, 1.f, 1.f ),
			LLFontGL::LEFT, LLFontGL::TOP);
	}
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}
	gGL.pushMatrix();
	LLUI::pushMatrix();
	{
		gGL.scaleUI(mDisplayScale.mV[VX], mDisplayScale.mV[VY], 1.f);
		LLVector2 old_scale_factor = LLUI::getScaleFactor();
		F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
		S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();
		if (zoom_factor > 1.f)
		{
			int pos_y = sub_region / llceil(zoom_factor);
			int pos_x = sub_region - (pos_y*llceil(zoom_factor));
			gGL.translatef((F32)getWindowWidthScaled() * -(F32)pos_x,
						(F32)getWindowHeightScaled() * -(F32)pos_y,
						0.f);
			gGL.scalef(zoom_factor, zoom_factor, 1.f);
			LLUI::getScaleFactor() *= zoom_factor;
		}
		LLToolMgr::getInstance()->getCurrentTool()->draw();
		static LLCachedControl<bool> drawMouselookInst(gSavedSettings, "AlchemyMouselookInstructions", true);
		if (drawMouselookInst && (gAgentCamera.cameraMouselook()))
		{
			drawMouselookInstructions();
			stop_glerror();
		}
		if(gDebugGL)check_blend_funcs();
		mRootView->draw();
		if(gDebugGL)check_blend_funcs();
		LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
		if (top_ctrl && top_ctrl->getVisible())
		{
			S32 screen_x, screen_y;
			top_ctrl->localPointToScreen(0, 0, &screen_x, &screen_y);
			gGL.matrixMode(LLRender::MM_MODELVIEW);
			LLUI::pushMatrix();
			LLUI::translate( (F32) screen_x, (F32) screen_y);
			if(gDebugGL)check_blend_funcs();
			top_ctrl->draw();
			if(gDebugGL)check_blend_funcs();
			LLUI::popMatrix();
		}
		if( mToolTip && mToolTip->getVisible() && !mToolTipBlocked )
		{
			gGL.matrixMode(LLRender::MM_MODELVIEW);
			LLUI::pushMatrix();
			{
				S32 tip_height = mToolTip->getRect().getHeight();
				S32 screen_x, screen_y;
				mToolTip->localPointToScreen(0, -24 - tip_height,
											 &screen_x, &screen_y);
				if (screen_y < tip_height)
				{
					mToolTip->localPointToScreen(0, 0, &screen_x, &screen_y);
				}
				LLUI::translate( (F32) screen_x, (F32) screen_y, 0);
				mToolTip->draw();
			}
			LLUI::popMatrix();
		}
		if( gShowOverlayTitle && !mOverlayTitle.empty() )
		{
			const S32 DIST_FROM_TOP = 20;
			LLFontGL::getFontSansSerifBig()->renderUTF8(
				mOverlayTitle, 0,
				ll_round( getWindowWidthScaled() * 0.5f),
				getWindowHeightScaled() - DIST_FROM_TOP,
				LLColor4(1, 1, 1, 0.4f),
				LLFontGL::HCENTER, LLFontGL::TOP);
		}
		LLUI::setScaleFactor(old_scale_factor);
	}
	LLUI::popMatrix();
	gGL.popMatrix();
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.unbind();
	}
#if LL_DEBUG
	LLView::sIsDrawing = FALSE;
#endif
}
BOOL LLViewerWindow::handleKeyUp(KEY key, MASK mask)
{
    LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
    if (keyboard_focus
		&& !(mask & (MASK_CONTROL | MASK_ALT))
		&& !gFocusMgr.getKeystrokesOnly())
	{
        if (keyboard_focus && keyboard_focus->wantsKeyUpKeyDown())
        {
            return keyboard_focus->handleKeyUp(key, mask, FALSE);
        }
        else if (key < 0x80)
		{
			return (gFocusMgr.getKeyboardFocus() != NULL);
		}
	}
	if (keyboard_focus)
	{
		if (keyboard_focus->handleKeyUp(key, mask, FALSE))
		{
			LL_DEBUGS() << "LLviewerWindow::handleKeyUp - in 'traverse up' - no loops seen... just called keyboard_focus->handleKeyUp an it returned true" << LL_ENDL;
			return TRUE;
		}
		else {
			LL_DEBUGS() << "LLviewerWindow::handleKeyUp - in 'traverse up' - no loops seen... just called keyboard_focus->handleKeyUp an it returned FALSE" << LL_ENDL;
		}
	}
	return gFocusMgr.childHasKeyboardFocus(mRootView)
		|| LLMenuGL::getKeyboardMode()
		|| (gMenuBarView && gMenuBarView->getHighlightedItem() && gMenuBarView->getHighlightedItem()->isActive());
}
BOOL LLViewerWindow::handleKey(KEY key, MASK mask)
{
	mToolTipBlocked = TRUE;
	if (gHoverView)
	{
		gHoverView->cancelHover();
		gHoverView->setTyping(TRUE);
	}
	if (gFocusMgr.getKeyboardFocus()
		&& !(mask & (MASK_CONTROL | MASK_ALT))
		&& !gFocusMgr.getKeystrokesOnly())
	{
        if (gFocusMgr.getKeyboardFocus() && gFocusMgr.getKeyboardFocus()->wantsKeyUpKeyDown())
        {
            return gFocusMgr.getKeyboardFocus()->handleKey(key, mask, FALSE );
        }
		else if (key < 0x80)
		{
			return (gFocusMgr.getKeyboardFocus() != NULL);
		}
	}
	if (LLView::sEditingUI && LLFloaterEditUI::processKeystroke(key, mask))
	{
		return TRUE;
	}
	if ((MASK_ALT & mask) &&
		(MASK_CONTROL & mask) &&
		('D' == key || 'd' == key))
	{
		if (gSavedSettings.getBOOL("LiruUseAdvancedMenuShortcut"))
			toggle_debug_menus(NULL);
	}
	if ((MASK_ALT & mask) &&
		(MASK_CONTROL & mask) &&
		!(MASK_SHIFT & mask) &&
		('T' == key || 't' == key))
	{
		if (!(gRlvHandler.hasBehaviour(RLV_BHVR_EDIT) && !LLDrawPoolAlpha::sShowDebugAlpha))
		{
			LLDrawPoolAlpha::sShowDebugAlpha = !LLDrawPoolAlpha::sShowDebugAlpha;
		}
		return TRUE;
	}
	if (key == KEY_ESCAPE && mask == MASK_SHIFT)
	{
		handle_reset_view();
		return TRUE;
	}
	if ((gMenuBarView && gMenuBarView->handleKey(key, mask, TRUE))
		|| (gLoginMenuBarView && gLoginMenuBarView->handleKey(key, mask, TRUE))
		|| (gMenuHolder && gMenuHolder->handleKey(key, mask, TRUE)))
	{
		return TRUE;
	}
	LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
	if ((mask & MASK_CONTROL) && LLPanelLogin::handleLoginAccelerators(key, mask))
	{
		return TRUE;
	}
	if (mask & (MASK_CONTROL | MASK_ALT) && !gFocusMgr.focusLocked())
	{
		if (gFocusMgr.keyboardFocusHasAccelerators()
			&& keyboard_focus
			&& keyboard_focus->handleKey(key,mask,FALSE))
		{
			return TRUE;
		}
		if ((mask & MASK_ALT)
			&& ((gMenuBarView && gMenuBarView->handleAcceleratorKey(key, mask))
				|| (gLoginMenuBarView && gLoginMenuBarView->handleAcceleratorKey(key, mask))))
		{
			return TRUE;
		}
	}
	if (key == KEY_TAB && (mask & MASK_CONTROL || gFocusMgr.getKeyboardFocus() == NULL))
	{
		if (gMenuHolder) gMenuHolder->hideMenus();
		gFloaterView->setCycleMode((mask & MASK_CONTROL) != 0);
		if (mask & MASK_SHIFT)
		{
			mRootView->focusPrevRoot();
		}
		else
		{
			mRootView->focusNextRoot();
		}
		return TRUE;
	}
	if( keyboard_focus )
	{
		if (gChatBar && gChatBar->inputEditorHasFocus())
		{
			if (gChatBar->getCurrentChat().empty()
				|| gSavedSettings.getBOOL("ArrowKeysMoveAvatar"))
			{
				{
					switch(key)
					{
					case KEY_LEFT:
					case KEY_RIGHT:
					case KEY_UP:
					case KEY_DOWN:
						if (mask == MASK_CONTROL)
							break;
					case KEY_PAGE_UP:
					case KEY_PAGE_DOWN:
					case KEY_HOME:
						return FALSE;
					default:
						break;
					}
				}
			}
		}
		if (keyboard_focus->handleKey(key, mask, FALSE))
		{
			return TRUE;
		}
	}
	if( LLToolMgr::getInstance()->getCurrentTool()->handleKey(key, mask) )
	{
		return TRUE;
	}
	if (LLGestureMgr::instance().triggerGesture(key, mask))
	{
		return TRUE;
	}
	if (gGestureList.trigger(key, mask))
	{
		return TRUE;
	}
	if (gSavedSettings.getBOOL("LetterKeysFocusChatBar") && !gAgentCamera.cameraMouselook() &&
		!keyboard_focus && key < 0x80 && (mask == MASK_NONE || mask == MASK_SHIFT))
	{
		{
			LLChatBar::startChat(NULL);
			return TRUE;
		}
	}
	if ((gMenuBarView && gMenuBarView->handleAcceleratorKey(key, mask))
		||(gLoginMenuBarView && gLoginMenuBarView->handleAcceleratorKey(key, mask)))
	{
		return TRUE;
	}
	return gFocusMgr.childHasKeyboardFocus(mRootView)
		|| LLMenuGL::getKeyboardMode()
		|| (gMenuBarView && gMenuBarView->getHighlightedItem() && gMenuBarView->getHighlightedItem()->isActive());
}
BOOL LLViewerWindow::handleUnicodeChar(llwchar uni_char, MASK mask)
{
	if ((uni_char == 13 && mask != MASK_CONTROL)
		|| (uni_char == 3 && mask == MASK_NONE))
	{
		return gViewerKeyboard.handleKey(KEY_RETURN, mask, gKeyboard->getKeyRepeated(KEY_RETURN));
	}
	if (gMenuBarView && gMenuBarView->handleUnicodeChar(uni_char, TRUE))
	{
		return TRUE;
	}
	LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
	if( keyboard_focus )
	{
		if (keyboard_focus->handleUnicodeChar(uni_char, FALSE))
		{
			return TRUE;
		}
		return TRUE;
	}
	return FALSE;
}
void LLViewerWindow::handleScrollWheel(S32 clicks)
{
	LLView::sMouseHandlerMessage.clear();
	gMouseIdleTimer.reset();
	if( mToolTip )
	{
		mToolTip->setVisible( FALSE );
	}
	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( mCurrentMousePoint.mX, mCurrentMousePoint.mY, &local_x, &local_y );
		mouse_captor->handleScrollWheel(local_x, local_y, clicks);
		if (LLView::sDebugMouseHandling)
		{
			LL_INFOS() << "Scroll Wheel handled by captor " << mouse_captor->getName() << LL_ENDL;
		}
		return;
	}
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x;
		S32 local_y;
		top_ctrl->screenPointToLocal( mCurrentMousePoint.mX, mCurrentMousePoint.mY, &local_x, &local_y );
		if (top_ctrl->handleScrollWheel(local_x, local_y, clicks)) return;
	}
	if (mRootView->handleScrollWheel(mCurrentMousePoint.mX, mCurrentMousePoint.mY, clicks) )
	{
		if (LLView::sDebugMouseHandling)
		{
			LL_INFOS() << "Scroll Wheel" << LLView::sMouseHandlerMessage << LL_ENDL;
		}
		return;
	}
	else if (LLView::sDebugMouseHandling)
	{
		LL_INFOS() << "Scroll Wheel not handled by view" << LL_ENDL;
	}
	if(top_ctrl == 0
		&& getWorldViewRectScaled().pointInRect(mCurrentMousePoint.mX, mCurrentMousePoint.mY)
		&& gAgentCamera.isInitialized())
	gAgentCamera.handleScrollWheel(clicks);
	return;
}
void LLViewerWindow::moveCursorToCenter()
{
	if (gSavedSettings.getBOOL("SGAbsolutePointer")) {
		return;
	}
	S32 x = getWorldViewWidthScaled() / 2;
	S32 y = getWorldViewHeightScaled() / 2;
	mCurrentMousePoint.set(x,y);
	mLastMousePoint.set(x,y);
	mCurrentMouseDelta.set(0,0);
	LLUI::setMousePositionScreen(x, y);
}
void LLViewerWindow::updateUI()
{
	static LLTrace::BlockTimerStatHandle ftm("Update UI");
	LL_RECORD_BLOCK_TIME(ftm);
	static std::string last_handle_msg;
	LLLayoutStack::updateClass();
	LLView::sMouseHandlerMessage.clear();
	S32 x = mCurrentMousePoint.mX;
	S32 y = mCurrentMousePoint.mY;
	MASK mask = gKeyboard->currentMask(TRUE);
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST))
	{
		gDebugRaycastFaceHit = -1;
		gDebugRaycastObject = cursorIntersect(-1, -1, 512.f, NULL, -1, FALSE, FALSE,
											  &gDebugRaycastFaceHit,
											  &gDebugRaycastIntersection,
											  &gDebugRaycastTexCoord,
											  &gDebugRaycastNormal,
											  &gDebugRaycastTangent,
											  &gDebugRaycastStart,
											  &gDebugRaycastEnd);
		gDebugRaycastParticle = gPipeline.lineSegmentIntersectParticle(gDebugRaycastStart, gDebugRaycastEnd, &gDebugRaycastParticleIntersection, NULL);
	}
	updateMouseDelta();
	if (gNoRender)
	{
		return;
	}
	updateKeyboardFocus();
	BOOL handled = FALSE;
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	LLView* captor_view = dynamic_cast<LLView*>(mouse_captor);
	view_handle_set_t mouse_hover_set;
	LLView* root_view = captor_view;
	if (!root_view)
	{
		root_view = mRootView;
	}
	{
		if (captor_view)
		{
			LLView* captor_parent_view = captor_view->getParent();
			while(captor_parent_view)
			{
				mouse_hover_set.insert(captor_parent_view->getHandle());
				captor_parent_view = captor_parent_view->getParent();
			}
		}
		if (top_ctrl && top_ctrl->calcScreenBoundingRect().pointInRect(x, y))
		{
			for (LLView::tree_iterator_t it = top_ctrl->beginTreeDFS();
				it != top_ctrl->endTreeDFS();
				++it)
			{
				LLView* viewp = *it;
				if (viewp->getVisible()
					&& viewp->calcScreenBoundingRect().pointInRect(x, y))
				{
					mouse_hover_set.insert(viewp->getHandle());
				}
				else
				{
					it.skipDescendants();
				}
			}
		}
		else
		{
			for (LLView::tree_iterator_t it = root_view->beginTreeDFS();
				it != root_view->endTreeDFS();
				++it)
			{
				LLView* viewp = *it;
				if (viewp->getVisible()
					&& viewp->calcScreenBoundingRect().pointInRect(x, y))
				{
					if (viewp->getMouseOpaque())
					{
						it = viewp->beginTreeDFS();
					}
					mouse_hover_set.insert(viewp->getHandle());
				}
				else
				{
					it.skipDescendants();
				}
			}
		}
	}
	typedef std::vector<LLHandle<LLView> > view_handle_list_t;
	view_handle_list_t mouse_enter_views;
	std::set_difference(mouse_hover_set.begin(), mouse_hover_set.end(),
						mMouseHoverViews.begin(), mMouseHoverViews.end(),
						std::back_inserter(mouse_enter_views));
	for (view_handle_list_t::iterator it = mouse_enter_views.begin();
		it != mouse_enter_views.end();
		++it)
	{
		LLView* viewp = it->get();
		if (viewp)
		{
			LLRect view_screen_rect = viewp->calcScreenRect();
			viewp->onMouseEnter(x - view_screen_rect.mLeft, y - view_screen_rect.mBottom, mask);
		}
	}
	view_handle_list_t mouse_leave_views;
	std::set_difference(mMouseHoverViews.begin(), mMouseHoverViews.end(),
						mouse_hover_set.begin(), mouse_hover_set.end(),
						std::back_inserter(mouse_leave_views));
	for (view_handle_list_t::iterator it = mouse_leave_views.begin();
		it != mouse_leave_views.end();
		++it)
	{
		LLView* viewp = it->get();
		if (viewp)
		{
			LLRect view_screen_rect = viewp->calcScreenRect();
			viewp->onMouseLeave(x - view_screen_rect.mLeft, y - view_screen_rect.mBottom, mask);
		}
	}
	swap(mMouseHoverViews, mouse_hover_set);
	if (mMouseInWindow)
	{
		if( mouse_captor )
		{
			S32 local_x;
			S32 local_y;
			mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
			handled = mouse_captor->handleHover(local_x, local_y, mask);
			if (LLView::sDebugMouseHandling)
			{
				LL_INFOS() << "Hover handled by captor " << mouse_captor->getName() << LL_ENDL;
			}
			if( !handled )
			{
				LL_DEBUGS("UserInput") << "hover not handled by mouse captor" << LL_ENDL;
			}
		}
		else
		{
			if (top_ctrl)
			{
				S32 local_x, local_y;
				top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
				handled = top_ctrl->pointInView(local_x, local_y) && top_ctrl->handleHover(local_x, local_y, mask);
			}
			if ( !handled )
			{
				if (mMouseInWindow && mRootView->handleHover(x, y, mask) )
				{
					if (LLView::sDebugMouseHandling && LLView::sMouseHandlerMessage != last_handle_msg)
					{
						last_handle_msg = LLView::sMouseHandlerMessage;
						LL_INFOS() << "Hover" << LLView::sMouseHandlerMessage << LL_ENDL;
					}
					handled = TRUE;
				}
				else if (LLView::sDebugMouseHandling)
				{
					if (last_handle_msg != LLStringUtil::null)
					{
						last_handle_msg.clear();
						LL_INFOS() << "Hover not handled by view" << LL_ENDL;
					}
				}
			}
			if( !handled )
			{
				LL_DEBUGS("UserInput") << "hover not handled by top view or root" << LL_ENDL;
			}
		}
		LLTool *tool = NULL;
		if (gHoverView)
		{
			tool = LLToolMgr::getInstance()->getCurrentTool();
			if(!handled && tool)
			{
				handled = tool->handleHover(x, y, mask);
				if (!mWindow->isCursorHidden())
				{
					gHoverView->updateHover(tool);
				}
			}
			else
			{
				gHoverView->cancelHover();
			}
		}
		BOOL tool_tip_handled = FALSE;
		std::string tool_tip_msg;
		static const LLCachedControl<F32> tool_tip_delay("ToolTipDelay",.7f);
		F32 tooltip_delay = tool_tip_delay;
		if ((mouse_captor && !mouse_captor->isView()) || LLUI::sShowXUINames)
		{
			static const LLCachedControl<F32> drag_and_drop_tool_tip_delay("DragAndDropToolTipDelay",.1f);
			tooltip_delay = drag_and_drop_tool_tip_delay;
		}
		if( handled &&
			gMouseIdleTimer.getElapsedTimeF32() > tooltip_delay &&
			!mWindow->isCursorHidden() )
		{
			LLRect screen_sticky_rect;
			LLMouseHandler *mh;
			S32 local_x, local_y;
			if (mouse_captor)
			{
				mouse_captor->screenPointToLocal(x, y, &local_x, &local_y);
				mh = mouse_captor;
			}
			else if (top_ctrl)
			{
				top_ctrl->screenPointToLocal(x, y, &local_x, &local_y);
				mh = top_ctrl;
			}
			else
			{
				local_x = x; local_y = y;
				mh = mRootView;
			}
			BOOL tooltip_vis = FALSE;
			if (shouldShowToolTipFor(mh))
			{
				tool_tip_handled = mh->handleToolTip(local_x, local_y, tool_tip_msg, &screen_sticky_rect );
				if( tool_tip_handled && !tool_tip_msg.empty() )
				{
					mToolTipStickyRect = screen_sticky_rect;
					mToolTip->setWrappedText( tool_tip_msg, 200 );
					mToolTip->reshapeToFitText();
					mToolTip->setOrigin( x, y );
					LLRect virtual_window_rect(0, getWindowHeight(), getWindowWidth(), 0);
					mToolTip->translateIntoRect( virtual_window_rect, FALSE );
					tooltip_vis = TRUE;
				}
			}
			if (mToolTip)
			{
				mToolTip->setVisible( tooltip_vis );
			}
		}
	}
	updateLayout();
	mLastMousePoint = mCurrentMousePoint;
	if (LLModalDialog::activeCount() == 0)
	{
		LLViewerParcelMgr::getInstance()->deselectUnused();
	}
	if (LLModalDialog::activeCount() == 0)
	{
		LLSelectMgr::getInstance()->deselectUnused();
	}
	return;
}
void LLViewerWindow::updateLayout()
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",0);
	LLTool* tool = LLToolMgr::getInstance()->getCurrentTool();
	if (gHoverView != NULL &&
		gFloaterTools != NULL
		&& tool != NULL
		&& tool != gToolNull
		&& tool != LLToolCompInspect::getInstance()
	&& tool != LLToolDragAndDrop::getInstance()
	&& !freeze_time)
	{
		bool suppress_toolbox =
			(LLToolMgr::getInstance()->getBaseTool() == LLToolPie::getInstance()) &&
			(LLToolMgr::getInstance()->getCurrentTool() != LLToolPie::getInstance());
		LLMouseHandler *captor = gFocusMgr.getMouseCapture();
		if (gFloaterTools->isMinimized()
			||	(tool != LLToolPie::getInstance()
				&& tool != LLToolCompGun::getInstance()
				&& !suppress_toolbox
				&& LLToolMgr::getInstance()->getCurrentToolset()->isShowFloaterTools()
				&& (!captor || dynamic_cast<LLView*>(captor) != NULL)))
		{
			if (!gFloaterTools->getVisible())
			{
				gFloaterTools->open();
			}
			LLCoordGL select_center_screen;
			MASK	mask = gKeyboard->currentMask(TRUE);
			gFloaterTools->updatePopup( select_center_screen, mask );
		}
		else
		{
			gFloaterTools->setVisible(FALSE);
		}
	}
	if (gToolBar)
	{
		gToolBar->refresh();
	}
	if (gNavigationBar)
	{
		gNavigationBar->refresh();
	}
	if (gChatBar)
	{
		gChatBar->refresh();
	}
	if (gOverlayBar)
	{
		gOverlayBar->refresh();
	}
	if (gOverlayBar && gNotifyBoxView && gToolBar && gHUDView)
	{
		LLRect bar_rect(-1, STATUS_BAR_HEIGHT, getWindowWidth()+1, -1);
		S32 occupied_nav = (gNavigationBar) ? gNavigationBar->getOccupiedHeight() : 0;
		S32 menu_h = (gMenuBarView) ? gMenuBarView->getRect().getHeight() : MENU_BAR_HEIGHT;
		S32 desired_top = getRootView()->getRect().getHeight() - menu_h - occupied_nav;
		S32 notify_top = getRootView()->getRect().getHeight() - menu_h;
		LLRect notify_box_rect = gNotifyBoxView->getRect();
		notify_box_rect.mBottom = bar_rect.mBottom;
		notify_box_rect.mTop = notify_top;
		gNotifyBoxView->reshape(notify_box_rect.getWidth(), notify_box_rect.getHeight());
		gNotifyBoxView->setShape(notify_box_rect);
		LLRect floater_rect = gFloaterView->getRect();
		bool floater_rect_changed = false;
		if (floater_rect.mBottom != bar_rect.mBottom+1)
		{
			floater_rect.mBottom = bar_rect.mBottom+1;
			floater_rect_changed = true;
		}
		if (floater_rect.mTop != desired_top)
		{
			floater_rect.mTop = desired_top;
			floater_rect_changed = true;
		}
		if (floater_rect_changed)
		{
			gFloaterView->reshapeFloater(floater_rect.getWidth(), floater_rect.getHeight(),
										 TRUE, ADJUST_VERTICAL_NO);
			gFloaterView->setShape(floater_rect);
		}
		LLView* chatbar_and_buttons = gOverlayBar->getChatbarAndButtons();
		if (chatbar_and_buttons && chatbar_and_buttons->getLocalBoundingRect().notEmpty())
		{
			S32 top, left;
			chatbar_and_buttons->localPointToOtherView(
												chatbar_and_buttons->getLocalBoundingRect().mLeft,
												chatbar_and_buttons->getLocalBoundingRect().mTop,
												&left,
												&top,
												gFloaterView);
			gFloaterView->setSnapOffsetBottom(top);
		}
		else if (gToolBar->getVisible())
		{
			S32 top, left;
			gToolBar->localPointToOtherView(
											gToolBar->getLocalBoundingRect().mLeft,
											gToolBar->getLocalBoundingRect().mTop,
											&left,
											&top,
											gFloaterView);
			gFloaterView->setSnapOffsetBottom(top);
		}
		else
		{
			gFloaterView->setSnapOffsetBottom(0);
		}
	}
	if (gConsole)
	{
		LLRect console_rect = getChatConsoleRect();
		if (gHUDView) console_rect.mBottom = gHUDView->getRect().mBottom + getChatConsoleBottomPad();
		gConsole->reshape(console_rect.getWidth(), console_rect.getHeight());
		gConsole->setRect(console_rect);
	}
	static const LLCachedControl<bool> chat_bar_steals_focus("ChatBarStealsFocus",true);
	if (chat_bar_steals_focus
		&& gChatBar
		&& gFocusMgr.getKeyboardFocus() == NULL
		&& gChatBar->isInVisibleChain())
	{
		LLChatBar::startChat(NULL);
	}
}
void LLViewerWindow::updateMouseDelta()
{
	S32 dx = lltrunc((F32) (mCurrentMousePoint.mX - mLastMousePoint.mX) * LLUI::getScaleFactor().mV[VX]);
	S32 dy = lltrunc((F32) (mCurrentMousePoint.mY - mLastMousePoint.mY) * LLUI::getScaleFactor().mV[VY]);
	LLCoordWindow mouse_pos;
	mWindow->getCursorPosition(&mouse_pos);
	if (mouse_pos.mX < 0 ||
		mouse_pos.mY < 0 ||
		mouse_pos.mX > mWindowRectRaw.getWidth() ||
		mouse_pos.mY > mWindowRectRaw.getHeight())
	{
		mMouseInWindow = FALSE;
	}
	else
	{
		mMouseInWindow = TRUE;
	}
	LLVector2 mouse_vel;
	static const  LLCachedControl<bool> mouse_smooth("MouseSmooth",false);
	if (mouse_smooth)
	{
		static F32 fdx = 0.f;
		static F32 fdy = 0.f;
		F32 amount = 16.f;
		fdx = fdx + ((F32) dx - fdx) * llmin(gFrameIntervalSeconds.value()*amount,1.f);
		fdy = fdy + ((F32) dy - fdy) * llmin(gFrameIntervalSeconds.value()*amount,1.f);
		mCurrentMouseDelta.set(ll_round(fdx), ll_round(fdy));
		mouse_vel.setVec(fdx,fdy);
	}
	else
	{
		mCurrentMouseDelta.set(dx, dy);
		mouse_vel.setVec((F32) dx, (F32) dy);
	}
	mMouseVelocityStat.addValue(mouse_vel.magVec());
}
void LLViewerWindow::updateKeyboardFocus()
{
	if (!gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		gFocusMgr.setKeyboardFocus(NULL);
	}
	LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
	if (cur_focus)
	{
		if (!cur_focus->isInVisibleChain() || !cur_focus->isInEnabledChain())
		{
			gFocusMgr.releaseFocusIfNeeded(cur_focus);
			LLUICtrl* parent = cur_focus->getParentUICtrl();
			const LLUICtrl* focus_root = cur_focus->findRootMostFocusRoot();
			while(parent)
			{
				if (parent->isCtrl()
					&& (parent->hasTabStop() || parent == focus_root)
					&& !parent->getIsChrome()
					&& parent->isInVisibleChain()
					&& parent->isInEnabledChain())
				{
					if (!parent->focusFirstItem())
					{
						parent->setFocus(TRUE);
					}
					break;
				}
				parent = parent->getParentUICtrl();
			}
		}
		else if (cur_focus->isFocusRoot())
		{
			cur_focus->focusFirstItem();
		}
	}
	if (LLEditMenuHandler::gEditMenuHandler == NULL && LLSelectMgr::getInstance()->getSelection()->getObjectCount())
	{
		LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
	}
	if (gFloaterView->getCycleMode())
	{
		gFloaterView->highlightFocusedFloater();
		gSnapshotFloaterView->highlightFocusedFloater();
		MASK	mask = gKeyboard->currentMask(TRUE);
		if ((mask & MASK_CONTROL) == 0)
		{
			gFloaterView->setCycleMode(FALSE);
			gFloaterView->syncFloaterTabOrder();
		}
		else
		{
		}
	}
	else
	{
		gFloaterView->highlightFocusedFloater();
		gSnapshotFloaterView->highlightFocusedFloater();
		gFloaterView->syncFloaterTabOrder();
	}
}
void LLViewerWindow::saveLastMouse(const LLCoordGL &point)
{
	if (point.mX < 0)
	{
		mCurrentMousePoint.mX = 0;
	}
	else if (point.mX > getWindowWidthScaled())
	{
		mCurrentMousePoint.mX = getWindowWidthScaled();
	}
	else
	{
		mCurrentMousePoint.mX = point.mX;
	}
	if (point.mY < 0)
	{
		mCurrentMousePoint.mY = 0;
	}
	else if (point.mY > getWindowHeightScaled() )
	{
		mCurrentMousePoint.mY = getWindowHeightScaled();
	}
	else
	{
		mCurrentMousePoint.mY = point.mY;
	}
}
void LLViewerWindow::renderSelections( BOOL for_gl_pick, BOOL pick_parcel_walls, BOOL for_hud )
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (!for_hud && !for_gl_pick)
	{
		LLSelectMgr::getInstance()->updateSilhouettes();
	}
	if (for_gl_pick)
	{
		if (pick_parcel_walls)
		{
			LLViewerParcelMgr::getInstance()->renderParcelCollision();
		}
	}
	else if (( for_hud && selection->getSelectType() == SELECT_TYPE_HUD) ||
			 (!for_hud && selection->getSelectType() != SELECT_TYPE_HUD))
	{
		LLSelectMgr::getInstance()->renderSilhouettes(for_hud);
		stop_glerror();
		if (selection->getSelectType() == SELECT_TYPE_HUD && LLSelectMgr::getInstance()->getSelection()->getObjectCount())
		{
			LLBBox hud_bbox = gAgentAvatarp->getHUDBBox();
			gGL.matrixMode(LLRender::MM_PROJECTION);
			gGL.pushMatrix();
			gGL.loadIdentity();
			F32 depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
			gGL.ortho(-0.5f * LLViewerCamera::getInstance()->getAspect(), 0.5f * LLViewerCamera::getInstance()->getAspect(), -0.5f, 0.5f, 0.f, depth);
			gGL.matrixMode(LLRender::MM_MODELVIEW);
			gGL.pushMatrix();
			gGL.loadIdentity();
			gGL.loadMatrix(OGL_TO_CFR_ROTATION);
			gGL.translatef(-hud_bbox.getCenterLocal().mV[VX] + (depth *0.5f), 0.f, 0.f);
		}
		if (LLSelectMgr::sRenderLightRadius && LLToolMgr::getInstance()->inEdit())
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			LLGLEnable<GL_BLEND> gls_blend;
			LLGLEnable<GL_CULL_FACE> gls_cull;
			LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
			gGL.matrixMode(LLRender::MM_MODELVIEW);
			gGL.pushMatrix();
			if (selection->getSelectType() == SELECT_TYPE_HUD)
			{
				F32 zoom = gAgentCamera.mHUDCurZoom;
				gGL.scalef(zoom, zoom, zoom);
			}
			struct f : public LLSelectedObjectFunctor
			{
				virtual bool apply(LLViewerObject* object)
				{
					LLDrawable* drawable = object->mDrawable;
					if (drawable && drawable->isLight())
					{
						LLVOVolume* vovolume = drawable->getVOVolume();
						gGL.pushMatrix();
						LLVector3 center = drawable->getPositionAgent();
						gGL.translatef(center[0], center[1], center[2]);
						F32 scale = vovolume->getLightRadius();
						gGL.scalef(scale, scale, scale);
						LLColor4 color(vovolume->getLightColor(), .5f);
						gGL.color4fv(color.mV);
						gSphere.render();
						glCullFace(GL_FRONT);
						gSphere.render();
						glCullFace(GL_BACK);
						gGL.popMatrix();
					}
					return true;
				}
			} func;
			LLSelectMgr::getInstance()->getSelection()->applyToObjects(&func);
			gGL.popMatrix();
		}
		LLTool* tool = LLToolMgr::getInstance()->getCurrentTool();
		if (tool)
		{
			if(tool->isAlwaysRendered())
			{
				tool->render();
			}
			else
			{
				if( !LLSelectMgr::getInstance()->getSelection()->isEmpty() )
				{
					BOOL moveable_object_selected = FALSE;
					BOOL all_selected_objects_move = TRUE;
					BOOL all_selected_objects_modify = TRUE;
					BOOL selecting_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");
					for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
						 iter != LLSelectMgr::getInstance()->getSelection()->end(); iter++)
					{
						LLSelectNode* nodep = *iter;
						LLViewerObject* object = nodep->getObject();
						LLViewerObject *root_object = (object == NULL) ? NULL : object->getRootEdit();
						BOOL this_object_movable = FALSE;
						if (object->permMove() && !object->isPermanentEnforced() &&
							((root_object == NULL) || !root_object->isPermanentEnforced()) &&
							(object->permModify() || selecting_linked_set))
						{
							moveable_object_selected = TRUE;
							this_object_movable = TRUE;
							if ( (rlv_handler_t::isEnabled()) &&
								 ((gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) || (gRlvHandler.hasBehaviour(RLV_BHVR_SITTP))) )
							{
								LLVOAvatar* pAvatar = gAgentAvatarp;
								if ( (pAvatar) && (pAvatar->isSitting()) && (pAvatar->getRoot() == object->getRootEdit()) )
									moveable_object_selected = this_object_movable = FALSE;
							}
						}
						all_selected_objects_move = all_selected_objects_move && this_object_movable;
						all_selected_objects_modify = all_selected_objects_modify && object->permModify();
					}
					BOOL draw_handles = TRUE;
					if (tool == LLToolCompTranslate::getInstance() && (!moveable_object_selected || !all_selected_objects_move))
					{
						draw_handles = FALSE;
					}
					if (tool == LLToolCompRotate::getInstance() && (!moveable_object_selected || !all_selected_objects_move))
					{
						draw_handles = FALSE;
					}
					if ( !all_selected_objects_modify && tool == LLToolCompScale::getInstance() )
					{
						draw_handles = FALSE;
					}
					if( draw_handles )
					{
						tool->render();
					}
				}
			}
			if (selection->getSelectType() == SELECT_TYPE_HUD && selection->getObjectCount())
			{
				gGL.matrixMode(LLRender::MM_PROJECTION);
				gGL.popMatrix();
				gGL.matrixMode(LLRender::MM_MODELVIEW);
				gGL.popMatrix();
				stop_glerror();
			}
		}
	}
}
LLVector3d LLViewerWindow::clickPointInWorldGlobal(S32 x, S32 y_from_bot, LLViewerObject* clicked_object) const
{
	LLVector3 mouse_direction_global = mouseDirectionGlobal( x, y_from_bot );
	LLVector3d relative_object = clicked_object->getPositionGlobal() - gAgentCamera.getCameraPositionGlobal();
	mouse_direction_global *= (F32) relative_object.magVec();
	LLVector3d new_pos;
	new_pos.setVec(mouse_direction_global);
	new_pos += gAgentCamera.getCameraPositionGlobal();
	return new_pos;
}
BOOL LLViewerWindow::clickPointOnSurfaceGlobal(const S32 x, const S32 y, LLViewerObject *objectp, LLVector3d &point_global) const
{
	BOOL intersect = FALSE;
	if (!intersect)
	{
		point_global = clickPointInWorldGlobal(x, y, objectp);
		LL_INFOS() << "approx intersection at " <<  (objectp->getPositionGlobal() - point_global) << LL_ENDL;
	}
	else
	{
		LL_INFOS() << "good intersection at " <<  (objectp->getPositionGlobal() - point_global) << LL_ENDL;
	}
	return intersect;
}
void LLViewerWindow::pickAsync( S32 x,
								S32 y_from_bot,
								MASK mask,
								void (*callback)(const LLPickInfo& info),
								BOOL pick_transparent,
								BOOL pick_rigged,
								BOOL pick_unselectable,
								BOOL get_surface_info)
{
	if (gNoRender)
	{
		return;
	}
	BOOL in_build_mode = gFloaterTools && gFloaterTools->getVisible();
	if (in_build_mode || LLDrawPoolAlpha::sShowDebugAlpha)
	{
		pick_transparent = TRUE;
	}
	LLPickInfo pick_info(LLCoordGL(x, y_from_bot), mask, pick_transparent, pick_rigged, FALSE, get_surface_info, pick_unselectable, callback);
	schedulePick(pick_info);
}
void LLViewerWindow::schedulePick(LLPickInfo& pick_info)
{
	if (mPicks.size() >= 1024 || mWindow->getMinimized())
	{
		if (pick_info.mPickCallback)
		{
			pick_info.mPickCallback(pick_info);
		}
		return;
	}
	mPicks.push_back(pick_info);
	mWindow->delayInputProcessing();
}
void LLViewerWindow::performPick()
{
	if (gNoRender)
	{
		return;
	}
	if (!gFocusMgr.getAppHasFocus())
	{
		return;
	}
	if (!mPicks.empty())
	{
		std::vector<LLPickInfo>::iterator pick_it;
		for (pick_it = mPicks.begin(); pick_it != mPicks.end(); ++pick_it)
		{
			pick_it->fetchResults();
		}
		mLastPick = mPicks.back();
		mPicks.clear();
	}
}
void LLViewerWindow::returnEmptyPicks()
{
	std::vector<LLPickInfo>::iterator pick_it;
	for (pick_it = mPicks.begin(); pick_it != mPicks.end(); ++pick_it)
	{
		mLastPick = *pick_it;
		if (pick_it->mPickCallback)
		{
			pick_it->mPickCallback(*pick_it);
		}
	}
	mPicks.clear();
}
LLPickInfo LLViewerWindow::pickImmediate(S32 x, S32 y_from_bot, BOOL pick_transparent, BOOL pick_rigged, BOOL pick_particle)
{
	if (gNoRender)
	{
		return LLPickInfo();
	}
	if (!gFocusMgr.getAppHasFocus())
	{
		return LLPickInfo();
	}
	BOOL in_build_mode = gFloaterTools && gFloaterTools->getVisible();
	if (in_build_mode || LLDrawPoolAlpha::sShowDebugAlpha)
	{
		pick_transparent = TRUE;
	}
	MASK	key_mask = gKeyboard->currentMask(TRUE);
	mLastPick = LLPickInfo(LLCoordGL(x, y_from_bot), key_mask, pick_transparent, pick_rigged, pick_particle, TRUE, FALSE, NULL);
	mLastPick.fetchResults();
	return mLastPick;
}
LLHUDIcon* LLViewerWindow::cursorIntersectIcon(S32 mouse_x, S32 mouse_y, F32 depth,
										   LLVector4a* intersection)
{
	S32 x = mouse_x;
	S32 y = mouse_y;
	if ((mouse_x == -1) && (mouse_y == -1))
	{
		x = getCurrentMouseX();
		y = getCurrentMouseY();
	}
	LLVector3 mouse_direction_global = mouseDirectionGlobal(x,y);
	LLVector3 mouse_point_global = LLViewerCamera::getInstance()->getOrigin();
	LLVector3 mouse_world_start = mouse_point_global;
	LLVector3 mouse_world_end   = mouse_point_global + mouse_direction_global * depth;
	LLVector4a start, end;
	start.load3(mouse_world_start.mV);
	end.load3(mouse_world_end.mV);
	return LLHUDIcon::lineSegmentIntersectAll(start, end, intersection);
}
LLViewerObject* LLViewerWindow::cursorIntersect(S32 mouse_x, S32 mouse_y, F32 depth,
												LLViewerObject *this_object,
												S32 this_face,
												BOOL pick_transparent,
												BOOL pick_rigged,
												S32* face_hit,
												LLVector4a *intersection,
												LLVector2 *uv,
												LLVector4a *normal,
												LLVector4a *tangent,
												LLVector4a* start,
												LLVector4a* end)
{
	S32 x = mouse_x;
	S32 y = mouse_y;
	if ((mouse_x == -1) && (mouse_y == -1))
	{
		x = getCurrentMouseX();
		y = getCurrentMouseY();
	}
	LLVector3 mouse_point_hud = mousePointHUD(x, y);
	LLVector3 mouse_hud_start = mouse_point_hud - LLVector3(depth, 0, 0);
	LLVector3 mouse_hud_end   = mouse_point_hud + LLVector3(depth, 0, 0);
	LLVector3 mouse_direction_global = mouseDirectionGlobal(x,y);
	LLVector3 mouse_point_global = LLViewerCamera::getInstance()->getOrigin();
	LLVector3 n = LLViewerCamera::getInstance()->getAtAxis();
	LLVector3 p = mouse_point_global + n * LLViewerCamera::getInstance()->getNear();
	LLVector3 pos;
	line_plane(mouse_point_global, mouse_direction_global, p, n, pos);
	mouse_point_global = pos;
	LLVector3 mouse_world_start = mouse_point_global;
	LLVector3 mouse_world_end   = mouse_point_global + mouse_direction_global * depth;
	if (!LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		gDebugRaycastIntersection.load3(mouse_world_end.mV);
	}
	LLVector4a mw_start;
	mw_start.load3(mouse_world_start.mV);
	LLVector4a mw_end;
	mw_end.load3(mouse_world_end.mV);
	LLVector4a mh_start;
	mh_start.load3(mouse_hud_start.mV);
	LLVector4a mh_end;
	mh_end.load3(mouse_hud_end.mV);
	if (start)
	{
		*start = mw_start;
	}
	if (end)
	{
		*end = mw_end;
	}
	LLViewerObject* found = NULL;
	if (this_object)
	{
		if (this_object->isHUDAttachment())
		{
			if (this_object->lineSegmentIntersect(mh_start, mh_end, this_face, pick_transparent, pick_rigged,
												  face_hit, intersection, uv, normal, tangent))
			{
				found = this_object;
			}
		}
		else
		{
			if (this_object->lineSegmentIntersect(mw_start, mw_end, this_face, pick_transparent, pick_rigged,
												  face_hit, intersection, uv, normal, tangent))
			{
				found = this_object;
			}
		}
	}
	else
	{
		found = gPipeline.lineSegmentIntersectInHUD(mh_start, mh_end, pick_transparent,
													face_hit, intersection, uv, normal, tangent);
		if ( (rlv_handler_t::isEnabled()) && (LLToolCamera::getInstance()->hasMouseCapture()) && (gKeyboard->currentMask(TRUE) & MASK_ALT) )
		{
			found = NULL;
		}
		if (!found)
		{
			found = gPipeline.lineSegmentIntersectInWorld(mw_start, mw_end, pick_transparent, pick_rigged,
														  face_hit, intersection, uv, normal, tangent);
			if (found && !pick_transparent)
			{
				gDebugRaycastIntersection = *intersection;
			}
#ifdef RLV_EXTENSION_CMD_INTERACT
			if ( (rlv_handler_t::isEnabled()) && (found) && (gRlvHandler.hasBehaviour(RLV_BHVR_INTERACT)) )
			{
				LLTool* pCurTool = LLToolMgr::getInstance()->getCurrentTool();
				if ( (LLToolDragAndDrop::getInstance() != pCurTool) &&
					 (!LLToolCamera::getInstance()->hasMouseCapture()) &&
					 ((LLToolPie::getInstance() != pCurTool) || (gAgent.getID() != found->getID())) )
				{
					found = NULL;
				}
			}
#endif
			if (found && !pick_transparent)
			{
				gDebugRaycastIntersection = *intersection;
			}
		}
	}
	return found;
}
LLVector3 LLViewerWindow::mouseDirectionGlobal(const S32 x, const S32 y) const
{
	F32			fov = LLViewerCamera::getInstance()->getView();
	F32			center_x = getWorldViewRectScaled().getCenterX();
	F32			center_y = getWorldViewRectScaled().getCenterY();
	F32			distance = ((F32)getWorldViewHeightScaled() * 0.5f) / (tan(fov / 2.f));
	F32			click_x = x - center_x;
	F32			click_y = y - center_y;
	LLVector3	mouse_vector =	distance * LLViewerCamera::getInstance()->getAtAxis()
								- click_x * LLViewerCamera::getInstance()->getLeftAxis()
								+ click_y * LLViewerCamera::getInstance()->getUpAxis();
	mouse_vector.normVec();
	return mouse_vector;
}
LLVector3 LLViewerWindow::mousePointHUD(const S32 x, const S32 y) const
{
	S32			height = getWorldViewHeightScaled();
	F32			center_x = getWorldViewRectScaled().getCenterX();
	F32			center_y = getWorldViewRectScaled().getCenterY();
	F32 hud_x = -((F32)x - center_x)  / height;
	F32 hud_y = ((F32)y - center_y) / height;
	return LLVector3(0.f, hud_x/gAgentCamera.mHUDCurZoom, hud_y/gAgentCamera.mHUDCurZoom);
}
LLVector3 LLViewerWindow::mouseDirectionCamera(const S32 x, const S32 y) const
{
	F32			fov_height = LLViewerCamera::getInstance()->getView();
	F32			fov_width = fov_height * LLViewerCamera::getInstance()->getAspect();
	S32			height = getWorldViewHeightScaled();
	S32			width = getWorldViewWidthScaled();
	F32			center_x = getWorldViewRectScaled().getCenterX();
	F32			center_y = getWorldViewRectScaled().getCenterY();
	F32			click_x = (((F32)x - center_x) / (F32)width) * fov_width * -1.f;
	F32			click_y = (((F32)y - center_y) / (F32)height) * fov_height;
	LLVector3	mouse_vector =	LLVector3(0.f, 0.f, -1.f);
	LLQuaternion mouse_rotate;
	mouse_rotate.setQuat(click_y, click_x, 0.f);
	mouse_vector = mouse_vector * mouse_rotate;
	mouse_vector = mouse_vector * (-1.f / mouse_vector.mV[VZ]);
	return mouse_vector;
}
BOOL LLViewerWindow::mousePointOnPlaneGlobal(LLVector3d& point, const S32 x, const S32 y,
										const LLVector3d &plane_point_global,
										const LLVector3 &plane_normal_global)
{
	LLVector3d	mouse_direction_global_d;
	mouse_direction_global_d.setVec(mouseDirectionGlobal(x,y));
	LLVector3d	plane_normal_global_d;
	plane_normal_global_d.setVec(plane_normal_global);
	F64 plane_mouse_dot = (plane_normal_global_d * mouse_direction_global_d);
	LLVector3d plane_origin_camera_rel = plane_point_global - gAgentCamera.getCameraPositionGlobal();
	F64	mouse_look_at_scale = (plane_normal_global_d * plane_origin_camera_rel)
								/ plane_mouse_dot;
	if (llabs(plane_mouse_dot) < 0.00001)
	{
		LLVector3d plane_origin_dir = plane_origin_camera_rel;
		plane_origin_dir.normVec();
		mouse_look_at_scale = plane_origin_camera_rel.magVec() / (plane_origin_dir * mouse_direction_global_d);
	}
	point = gAgentCamera.getCameraPositionGlobal() + mouse_look_at_scale * mouse_direction_global_d;
	return mouse_look_at_scale > 0.0;
}
BOOL LLViewerWindow::mousePointOnLandGlobal(const S32 x, const S32 y, LLVector3d *land_position_global, BOOL ignore_distance)
{
	LLVector3		mouse_direction_global = mouseDirectionGlobal(x,y);
	F32				mouse_dir_scale;
	BOOL			hit_land = FALSE;
	LLViewerRegion	*regionp;
	F32			land_z;
	const F32	FIRST_PASS_STEP = 1.0f;
	const F32	SECOND_PASS_STEP = 0.1f;
	const F32	draw_distance = ignore_distance ? MAX_FAR_CLIP : (gAgentCamera.mDrawDistance * 4);
	LLVector3d	camera_pos_global;
	camera_pos_global = gAgentCamera.getCameraPositionGlobal();
	LLVector3d		probe_point_global;
	LLVector3		probe_point_region;
	for (mouse_dir_scale = FIRST_PASS_STEP; mouse_dir_scale < draw_distance; mouse_dir_scale += FIRST_PASS_STEP)
	{
		LLVector3d mouse_direction_global_d;
		mouse_direction_global_d.setVec(mouse_direction_global * mouse_dir_scale);
		probe_point_global = camera_pos_global + mouse_direction_global_d;
		regionp = LLWorld::getInstance()->resolveRegionGlobal(probe_point_region, probe_point_global);
		if (!regionp)
		{
			continue;
		}
		S32 i = (S32) (probe_point_region.mV[VX]/regionp->getLand().getMetersPerGrid());
		S32 j = (S32) (probe_point_region.mV[VY]/regionp->getLand().getMetersPerGrid());
		S32 grids_per_edge = (S32) regionp->getLand().mGridsPerEdge;
		if ((i >= grids_per_edge) || (j >= grids_per_edge))
		{
			continue;
		}
		land_z = regionp->getLand().resolveHeightRegion(probe_point_region);
		if (probe_point_region.mV[VZ] < land_z)
		{
			hit_land = TRUE;
			break;
		}
	}
	if (hit_land)
	{
		F32 stop_mouse_dir_scale = mouse_dir_scale + FIRST_PASS_STEP;
		for ( mouse_dir_scale -= FIRST_PASS_STEP; mouse_dir_scale <= stop_mouse_dir_scale; mouse_dir_scale += SECOND_PASS_STEP)
		{
			LLVector3d mouse_direction_global_d;
			mouse_direction_global_d.setVec(mouse_direction_global * mouse_dir_scale);
			probe_point_global = camera_pos_global + mouse_direction_global_d;
			regionp = LLWorld::getInstance()->resolveRegionGlobal(probe_point_region, probe_point_global);
			if (!regionp)
			{
				continue;
			}
			land_z = regionp->getLand().resolveHeightRegion(probe_point_region);
			if (probe_point_region.mV[VZ] < land_z)
			{
				*land_position_global = probe_point_global;
				return TRUE;
			}
		}
	}
	return FALSE;
}
void LLViewerWindow::saveImageNumbered(LLPointer<LLImageFormatted> image, int index)
{
	if (!image)
	{
		LLFloaterSnapshot::saveLocalDone(false, index);
		return;
	}
	ESaveFilter pick_type;
	std::string extension("." + image->getExtension());
	if (extension == ".j2c")
		pick_type = FFSAVE_J2C;
	else if (extension == ".bmp")
		pick_type = FFSAVE_BMP;
	else if (extension == ".jpg")
		pick_type = FFSAVE_JPEG;
	else if (extension == ".png")
		pick_type = FFSAVE_PNG;
	else if (extension == ".tga")
		pick_type = FFSAVE_TGA;
	else
		pick_type = FFSAVE_ALL;
	if (!isSnapshotLocSet())
	{
		std::string proposed_name( sSnapshotBaseName );
		AIFilePicker* filepicker = AIFilePicker::create();
		filepicker->open(proposed_name, pick_type, "", "snapshot");
		filepicker->run(boost::bind(&LLViewerWindow::saveImageNumbered_continued1, this, image, extension, filepicker, index));
		return;
	}
	saveImageNumbered_continued2(image, extension, index);
}
void LLViewerWindow::saveImageNumbered_continued1(LLPointer<LLImageFormatted> image, std::string const& extension, AIFilePicker* filepicker, int index)
{
	if (filepicker->hasFilename())
	{
		std::string filepath = filepicker->getFilename();
		LLViewerWindow::sSnapshotBaseName = gDirUtilp->getBaseFileName(filepath, true);
		LLViewerWindow::sSnapshotDir = gDirUtilp->getDirName(filepath);
		saveImageNumbered_continued2(image, extension, index);
	}
	else
	{
		LLFloaterSnapshot::saveLocalDone(false, index);
	}
}
void LLViewerWindow::saveImageNumbered_continued2(LLPointer<LLImageFormatted> image, std::string const& extension, int index)
{
	std::string filepath;
	S32 i = 1;
	S32 err = 0;
	do
	{
		filepath = sSnapshotDir;
		filepath += gDirUtilp->getDirDelimiter();
		filepath += sSnapshotBaseName;
		filepath += llformat("_%.3d",i);
		filepath += extension;
		llstat stat_info;
		err = LLFile::stat( filepath, &stat_info );
		i++;
	}
	while( -1 != err );
	if (image->save(filepath))
	{
		playSnapshotAnimAndSound();
		LLFloaterSnapshot::saveLocalDone(true, index);
	}
	else
	{
		LLFloaterSnapshot::saveLocalDone(false, index);
	}
}
void LLViewerWindow::resetSnapshotLoc()
{
	sSnapshotDir.clear();
}
static S32 BORDERHEIGHT = 0;
static S32 BORDERWIDTH = 0;
void LLViewerWindow::movieSize(S32 new_width, S32 new_height)
{
	LLCoordScreen size;
	gViewerWindow->getWindow()->getSize(&size);
	if (  (size.mX != new_width + BORDERWIDTH)
		||(size.mY != new_height + BORDERHEIGHT))
	{
		S32 x = gViewerWindow->getWindowWidthRaw();
		S32 y = gViewerWindow->getWindowHeightRaw();
		BORDERWIDTH = size.mX - x;
		BORDERHEIGHT = size.mY- y;
		LLCoordScreen new_size(new_width + BORDERWIDTH,
							   new_height + BORDERHEIGHT);
		S32 vsync_mode = gSavedSettings.getS32("SHRenderVsyncMode");
		if(vsync_mode == -1 && !gGLManager.mHasAdaptiveVsync)
		{
			vsync_mode = 0;
		}
		if (gViewerWindow->getWindow()->getFullscreen())
		{
			LLGLStateValidator::checkStates();
			LLGLStateValidator::checkTextureChannels();
			gViewerWindow->changeDisplaySettings(FALSE,
												new_size,
												vsync_mode,
												TRUE);
			LLGLStateValidator::checkStates();
			LLGLStateValidator::checkTextureChannels();
		}
		else
		{
			gViewerWindow->getWindow()->setSize(new_size);
		}
	}
}
BOOL LLViewerWindow::saveSnapshot( const std::string& filepath, S32 image_width, S32 image_height, BOOL show_ui, BOOL do_rebuild, ESnapshotType type)
{
	LL_INFOS() << "Saving snapshot to: " << filepath << LL_ENDL;
	LLPointer<LLImageRaw> raw = new LLImageRaw;
	BOOL success = rawSnapshot(raw, image_width, image_height, (F32)image_width / image_height, show_ui, do_rebuild);
	if (success)
	{
		LLPointer<LLImageBMP> bmp_image = new LLImageBMP;
		success = bmp_image->encode(raw, 0.0f);
		if( success )
		{
			success = bmp_image->save(filepath);
		}
		else
		{
			LL_WARNS() << "Unable to encode bmp snapshot" << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS() << "Unable to capture raw snapshot" << LL_ENDL;
	}
	return success;
}
void LLViewerWindow::playSnapshotAnimAndSound()
{
	if (gSavedSettings.getBOOL("QuietSnapshotsToDisk"))
	{
		return;
	}
	gAgent.sendAnimationRequest(ANIM_AGENT_SNAPSHOT, ANIM_REQUEST_START);
	send_sound_trigger(LLUUID(gSavedSettings.getString("UISndSnapshot")), 1.0f);
}
BOOL LLViewerWindow::thumbnailSnapshot(LLImageRaw *raw, S32 preview_width, S32 preview_height, BOOL show_ui, BOOL do_rebuild, ESnapshotType type)
{
	return rawSnapshot(raw, preview_width, preview_height, (F32)gViewerWindow->getWindowWidthRaw() / gViewerWindow->getWindowHeightRaw(), show_ui, do_rebuild, type);
}
bool LLViewerWindow::rawRawSnapshot(LLImageRaw *raw,
	S32 image_width, S32 image_height, F32 snapshot_aspect, BOOL show_ui,
	BOOL do_rebuild, ESnapshotType type, S32 max_size, F32 supersample, bool uncrop)
{
	if (!raw)
	{
		return false;
	}
	if(LLPipeline::sMemAllocationThrottled)
	{
		return false;
	}
	if(image_width * image_height > (1 << 22))
	{
		if(!LLMemory::tryToAlloc(NULL, image_width * image_height * 3))
		{
			LL_WARNS() << "No enough memory to take the snapshot with size (w : h): " << image_width << " : " << image_height << LL_ENDL ;
			return false;
		}
	}
	gDisplaySwapBuffers = FALSE;
	gGL.syncContextState();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	setCursor(UI_CURSOR_WAIT);
	BOOL prev_draw_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
	if ( prev_draw_ui != show_ui)
	{
		LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}
	BOOL hide_hud = !gSavedSettings.getBOOL("RenderHUDInSnapshot") && LLPipeline::sShowHUDAttachments;
	if (hide_hud)
	{
		LLPipeline::sShowHUDAttachments = FALSE;
	}
	LLRect const window_rect = show_ui ? getWindowRectRaw() : getWorldViewRectRaw();
	S32 window_width = window_rect.getWidth();
	S32 window_height = window_rect.getHeight();
#if 1
	F32 internal_scale = llmin(llmax(supersample,1.f),3.f);
	image_height *= internal_scale;
	image_width *= internal_scale;
#endif
	if ((image_width > window_width || image_height > window_height))
	{
		if(LLPipeline::sShowHUDAttachments)
		{
			hide_hud=true;
			LLPipeline::sShowHUDAttachments = FALSE;
		}
		if(show_ui)
		{
			show_ui=false;
			LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
	}
	S32 buffer_x_offset = 0;
	S32 buffer_y_offset = 0;
	F32 scale_factor = 1.0f;
	S32 image_buffer_x;
	S32 image_buffer_y;
	F32 const window_aspect = (F32)window_width / window_height;
	F32 snapshot_width = (snapshot_aspect > window_aspect) ? (F32)window_width : window_height * snapshot_aspect;
	F32 snapshot_height = (snapshot_aspect < window_aspect) ? (F32)window_height : window_width / snapshot_aspect;
	S32 original_width = 0;
	S32 original_height = 0;
	bool reset_deferred = false;
	LLRenderTarget scratch_space;
	if ((image_width > window_width || image_height > window_height) && LLPipeline::sRenderDeferred && !show_ui)
	{
		if (scratch_space.allocate(image_width, image_height, GL_RGBA, true, true))
		{
			original_width = gPipeline.mDeferredScreen.getWidth();
			original_height = gPipeline.mDeferredScreen.getHeight();
			if (gPipeline.allocateScreenBuffer(image_width, image_height))
			{
				image_width = snapshot_width = window_width = scratch_space.getWidth();
				image_height = snapshot_height = window_height = scratch_space.getHeight();
				reset_deferred = true;
				mWindowRectRaw.set(0, image_height, image_width, 0);
				scratch_space.bindTarget();
			}
			else
			{
				scratch_space.release();
				gPipeline.allocateScreenBuffer(original_width, original_height);
			}
		}
	}
	F32 ratio = llmin(snapshot_width / image_width, snapshot_height / image_height);
	S32 unscaled_image_buffer_x = snapshot_width;
	S32 unscaled_image_buffer_y = snapshot_height;
	if (uncrop)
	{
	  unscaled_image_buffer_x = window_width;
	  unscaled_image_buffer_y = window_height;
	}
	for(scale_factor = llmax(1.0f, 1.0f / ratio);;
		scale_factor = llmin(max_size / snapshot_width, max_size / snapshot_height))
	{
		image_buffer_x = ll_round(unscaled_image_buffer_x * scale_factor);
		image_buffer_y = ll_round(unscaled_image_buffer_y * scale_factor);
		S32 image_size_x = ll_round(snapshot_width * scale_factor);
		S32 image_size_y = ll_round(snapshot_width * scale_factor);
		if (llmax(image_size_x, image_size_y) > max_size &&
			internal_scale <= 1.f && !reset_deferred)
		{
			continue;
		}
		break;
	}
	buffer_x_offset = llfloor(((window_width - unscaled_image_buffer_x) * scale_factor) / 2.f);
	buffer_y_offset = llfloor(((window_height - unscaled_image_buffer_y) * scale_factor) / 2.f);
	Dout(dc::snapshot, "rawRawSnapshot(" << image_width << ", " << image_height << ", " << snapshot_aspect << "): image_buffer_x = " << image_buffer_x << "; image_buffer_y = " << image_buffer_y);
	bool error = !(image_buffer_x > 0 && image_buffer_y > 0);
	if (!error)
	{
		raw->resize(image_buffer_x, image_buffer_y, 3);
		error = raw->isBufferInvalid();
	}
	if (error)
	{
		if (prev_draw_ui != gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
		{
			LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
		if (hide_hud)
		{
			LLPipeline::sShowHUDAttachments = TRUE;
		}
		setCursor(UI_CURSOR_ARROW);
		return false;
	}
	BOOL is_tiling = scale_factor > 1.f;
	if (is_tiling)
	{
		Dout(dc::warning, "USING TILING FOR SNAPSHOT!");
		send_agent_pause();
		if (show_ui || !hide_hud)
		{
			initFonts(scale_factor);
			LLHUDObject::reshapeAll();
		}
	}
	S32 output_buffer_offset_y = 0;
	F32 depth_conversion_factor_1 = (LLViewerCamera::getInstance()->getFar() + LLViewerCamera::getInstance()->getNear()) / (2.f * LLViewerCamera::getInstance()->getFar() * LLViewerCamera::getInstance()->getNear());
	F32 depth_conversion_factor_2 = (LLViewerCamera::getInstance()->getFar() - LLViewerCamera::getInstance()->getNear()) / (2.f * LLViewerCamera::getInstance()->getFar() * LLViewerCamera::getInstance()->getNear());
	gObjectList.generatePickList(*LLViewerCamera::getInstance());
	for (int subimage_y = 0; subimage_y < scale_factor; ++subimage_y)
	{
		S32 subimage_y_offset = llclamp(buffer_y_offset - (subimage_y * window_height), 0, window_height);
		U32 read_height = llmax(0, (window_height - subimage_y_offset) -
			llmax(0, (window_height * (subimage_y + 1)) - (buffer_y_offset + raw->getHeight())));
		S32 output_buffer_offset_x = 0;
		for (int subimage_x = 0; subimage_x < scale_factor; ++subimage_x)
		{
			gDisplaySwapBuffers = FALSE;
			gDepthDirty = TRUE;
			S32 subimage_x_offset = llclamp(buffer_x_offset - (subimage_x * window_width), 0, window_width);
			U32 read_width = llmax(0, (window_width - subimage_x_offset) -
									llmax(0, (window_width * (subimage_x + 1)) - (buffer_x_offset + raw->getWidth())));
			if (read_width && read_height)
			{
				const U32 subfield = subimage_x+(subimage_y*llceil(scale_factor));
				display(do_rebuild, scale_factor, subfield, TRUE, is_tiling);
				if (!LLPipeline::sRenderDeferred)
				{
					render_ui(scale_factor, subfield);
				}
				for (U32 out_y = 0; out_y < read_height ; out_y++)
				{
					S32 output_buffer_offset = (
												(out_y * (raw->getWidth()))
												+ (window_width * subimage_x)
												+ (raw->getWidth() * window_height * subimage_y)
												- output_buffer_offset_x
												- (output_buffer_offset_y * (raw->getWidth()))
												) * raw->getComponents();
					if (out_y % 100 == 0)
					{
						LLAppViewer::instance()->pingMainloopTimeout("LLViewerWindow::rawRawSnapshot");
					}
					if (type == SNAPSHOT_TYPE_COLOR)
					{
						glReadPixels(
									 subimage_x_offset, out_y + subimage_y_offset,
									 read_width, 1,
									 GL_RGB, GL_UNSIGNED_BYTE,
									 raw->getData() + output_buffer_offset
									 );
					}
					else
					{
						LLPointer<LLImageRaw> depth_line_buffer = new LLImageRaw(read_width, 1, sizeof(GL_FLOAT));
						glReadPixels(
									 subimage_x_offset, out_y + subimage_y_offset,
									 read_width, 1,
									 GL_DEPTH_COMPONENT, GL_FLOAT,
									 depth_line_buffer->getData()
									 );
						for (S32 i = 0; i < (S32)read_width; i++)
						{
							F32 depth_float = *(F32*)(depth_line_buffer->getData() + (i * sizeof(F32)));
							F32 linear_depth_float = 1.f / (depth_conversion_factor_1 - (depth_float * depth_conversion_factor_2));
							U8 depth_byte = F32_to_U8(linear_depth_float, LLViewerCamera::getInstance()->getNear(), LLViewerCamera::getInstance()->getFar());
							for (S32 j = 0; j < raw->getComponents(); j++)
							{
								*(raw->getData() + output_buffer_offset + (i * raw->getComponents()) + j) = depth_byte;
							}
						}
					}
				}
			}
			output_buffer_offset_x += subimage_x_offset;
			stop_glerror();
		}
		output_buffer_offset_y += subimage_y_offset;
	}
	gDisplaySwapBuffers = FALSE;
	gDepthDirty = TRUE;
	if (prev_draw_ui != gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}
	if (hide_hud)
	{
		LLPipeline::sShowHUDAttachments = TRUE;
	}
	if (is_tiling && (show_ui || !hide_hud))
	{
		initFonts(1.f);
		LLHUDObject::reshapeAll();
	}
	setCursor(UI_CURSOR_ARROW);
	if (do_rebuild)
	{
		gPipeline.resetDrawOrders();
	}
	if (reset_deferred)
	{
		mWindowRectRaw = window_rect;
		scratch_space.flush();
		scratch_space.release();
		gPipeline.allocateScreenBuffer(original_width, original_height);
	}
	if (is_tiling)
	{
		send_agent_resume();
	}
	return true;
}
bool LLViewerWindow::rawSnapshot(LLImageRaw *raw,
	S32 image_width, S32 image_height, F32 snapshot_aspect, BOOL show_ui,
	BOOL do_rebuild, ESnapshotType type, S32 max_size, F32 supersample)
{
	bool ret = rawRawSnapshot(raw, image_width, image_height, snapshot_aspect, show_ui, do_rebuild, type, max_size, supersample);
#if 1
	if (ret && !raw->scale(image_width, image_height))
	{
		ret = false;
	}
#else
	if (ret)
	{
		int n = 4;
		for (int c = raw->getComponents(); c % 2 == 0 && n > 1; c /= 2) { n /= 2; }
		image_width += (image_width * (n - 1)) % n;
		if (llabs(image_width - image_buffer_x) > 4 || llabs(image_height - image_buffer_y) > 4)
		{
			ret = raw->scale( image_width, image_height );
		}
		else if (image_width != image_buffer_x || image_height != image_buffer_y)
		{
			ret = raw->scale( image_width, image_height, FALSE );
		}
	}
#endif
	return ret;
}
void LLViewerWindow::destroyWindow()
{
	if (mWindow)
	{
		LLWindowManager::destroyWindow(mWindow);
	}
	mWindow = NULL;
}
void LLViewerWindow::drawMouselookInstructions()
{
	const LLFontGL* font = LLFontGL::getFontSansSerifBig();
	const S32 INSTRUCTIONS_PAD = getWorldViewRectScaled().mTop - 15;
	const S32 text_pos_start = getWorldViewRectScaled().getCenterX() - 150;
	if (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
	{
		const LLVector3& vec = gAgent.getPositionAgent();
		font->renderUTF8(
			llformat("X: %.2f", vec.mV[VX]), 0,
			text_pos_start,
			INSTRUCTIONS_PAD,
			LLColor4(1.0f, 0.5f, 0.5f, 0.5),
			LLFontGL::HCENTER, LLFontGL::TOP,
			LLFontGL::BOLD, LLFontGL::DROP_SHADOW_SOFT);
		font->renderUTF8(
			llformat("Y: %.2f", vec.mV[VY]), 0,
			text_pos_start + 100,
			INSTRUCTIONS_PAD,
			LLColor4(0.5f, 1.0f, 0.5f, 0.5),
			LLFontGL::HCENTER, LLFontGL::TOP,
			LLFontGL::BOLD, LLFontGL::DROP_SHADOW_SOFT);
		font->renderUTF8(
			llformat("Z: %.2f", vec.mV[VZ]), 0,
			text_pos_start + 200,
			INSTRUCTIONS_PAD,
			LLColor4(0.5f, 0.5f, 1.0f, 0.5),
			LLFontGL::HCENTER, LLFontGL::TOP,
			LLFontGL::BOLD, LLFontGL::DROP_SHADOW_SOFT);
	}
	const LLViewerParcelMgr& vpm = LLViewerParcelMgr::instance();
	const bool allow_damage = vpm.allowAgentDamage(gAgent.getRegion(), vpm.getAgentParcel());
	if (allow_damage)
	{
		const S32 health = gStatusBar ? gStatusBar->getHealth() : -1;
		font->renderUTF8(
			llformat("HP: %d%%", health), 0,
			text_pos_start + 300,
			INSTRUCTIONS_PAD,
			LLColor4(1.0f, 1.0f, 1.0f, 0.5),
			LLFontGL::HCENTER, LLFontGL::TOP,
			LLFontGL::BOLD, LLFontGL::DROP_SHADOW_SOFT);
	}
}
void* LLViewerWindow::getPlatformWindow() const
{
	return mWindow->getPlatformWindow();
}
void* LLViewerWindow::getMediaWindow() 	const
{
	return mWindow->getMediaWindow();
}
void LLViewerWindow::focusClient()		const
{
	return mWindow->focusClient();
}
S32	LLViewerWindow::getWindowHeight()	const
{
	return mWindowRectScaled.getHeight();
}
S32	LLViewerWindow::getWindowWidth() const
{
	return mWindowRectScaled.getWidth();
}
S32	LLViewerWindow::getWindowDisplayHeight()	const
{
	return mWindowRectRaw.getHeight();
}
S32	LLViewerWindow::getWindowDisplayWidth() const
{
	return mWindowRectRaw.getWidth();
}
void LLViewerWindow::setup2DRender()
{
	gl_state_for_2d(mWindowRectRaw.getWidth(), mWindowRectRaw.getHeight());
	setup2DViewport();
}
void LLViewerWindow::setup2DViewport(S32 x_offset, S32 y_offset)
{
	gGLViewport = mWindowRectRaw;
	gGLViewport.translate(x_offset, y_offset);
	gGL.setViewport(gGLViewport);
	gGL.setScissor(gGLViewport);
}
void LLViewerWindow::setup3DRender()
{
	LLViewerCamera::getInstance()->setPerspective(NOT_FOR_SELECTION, getWindowRectRaw().mLeft, getWindowRectRaw().mBottom,  getWindowRectRaw().getWidth(), getWindowRectRaw().getHeight(), FALSE, LLViewerCamera::getInstance()->getNear(), MAX_FAR_CLIP*2.f);
	setup3DViewport();
}
void LLViewerWindow::setup3DViewport(S32 x_offset, S32 y_offset)
{
	gGLViewport = mWindowRectRaw;
	gGLViewport.translate(x_offset, y_offset);
	gGL.setViewport(gGLViewport);
	gGL.setScissor(gGLViewport);
}
void LLViewerWindow::revealIntroPanel()
{
	if (mProgressView)
	{
		mProgressView->revealIntroPanel();
	}
}
void LLViewerWindow::setShowProgress(const BOOL show)
{
	if (mProgressView)
	{
		mProgressView->setVisible(show);
	}
}
void LLViewerWindow::setStartupComplete()
{
	if (mProgressView)
	{
		mProgressView->setStartupComplete();
	}
}
BOOL LLViewerWindow::getShowProgress() const
{
	return (mProgressView && mProgressView->getVisible());
}
void LLViewerWindow::setProgressString(const std::string& string)
{
	if (mProgressView)
	{
		mProgressView->setText(string);
	}
}
void LLViewerWindow::setProgressMessage(const std::string& msg)
{
	if(mProgressView)
	{
		mProgressView->setMessage(msg);
	}
}
void LLViewerWindow::setProgressPercent(const F32 percent)
{
	if (mProgressView)
	{
		mProgressView->setPercent(percent);
	}
}
void LLViewerWindow::setProgressCancelButtonVisible( BOOL b, const std::string& label )
{
	if (mProgressView)
	{
		mProgressView->setCancelButtonVisible( b, label );
	}
}
LLProgressView *LLViewerWindow::getProgressView() const
{
	return mProgressView;
}
void LLViewerWindow::dumpState()
{
	LL_INFOS() << "LLViewerWindow Active " << S32(mActive) << LL_ENDL;
	LL_INFOS() << "mWindow visible " << S32(mWindow->getVisible())
		<< " minimized " << S32(mWindow->getMinimized())
		<< LL_ENDL;
}
void LLViewerWindow::stopGL(BOOL save_state)
{
	stop_glerror();
	if (!gGLManager.mIsDisabled)
	{
		LL_INFOS() << "Shutting down GL..." << LL_ENDL;
		LLAppViewer::getTextureCache()->pause();
		LLAppViewer::getImageDecodeThread()->pause();
		LLAppViewer::getTextureFetch()->pause();
		gSky.destroyGL();
		stop_glerror();
		LLManipTranslate::destroyGL() ;
		stop_glerror();
		gBumpImageList.destroyGL();
		stop_glerror();
		LLFontGL::destroyAllGL();
		stop_glerror();
		LLVOAvatar::destroyGL();
		stop_glerror();
		LLVOPartGroup::destroyGL();
		stop_glerror();
		LLViewerDynamicTexture::destroyGL();
		stop_glerror();
		if (gPipeline.isInit())
		{
			gPipeline.destroyGL();
		}
		stop_glerror();
		gBox.cleanupGL();
		stop_glerror();
		if(LLPostProcess::instanceExists())
		{
			LLPostProcess::getInstance()->destroyGL();
		}
		gTextureList.destroyGL(save_state);
		stop_glerror();
		gGL.destroyGL();
		stop_glerror();
		gGLManager.mIsDisabled = TRUE;
		stop_glerror();
		LLVertexBuffer::cleanupClass();
		stop_glerror();
		LL_INFOS() << "Remaining allocated texture memory: " << LLImageGL::sGlobalTextureMemory << " bytes" << LL_ENDL;
	}
}
void LLViewerWindow::restoreGLState()
{
	gGL.init();
	stop_glerror();
	initGLDefaults();
	stop_glerror();
	gGL.refreshState();
	stop_glerror();
	LLGLStateValidator::restoreGL();
	stop_glerror();
}
void LLViewerWindow::restoreGL(bool full_restore, const std::string& progress_message)
{
	if (!gGLManager.mIsDisabled && !full_restore)
	{
		LL_INFOS() << "Restoring GL state..." << LL_ENDL;
		restoreGLState();
		gPipeline.releaseOcclusionBuffers();
		return;
	}
	if (gGLManager.mIsDisabled)
	{
		stop_glerror();
		LL_INFOS() << "Restoring GL..." << LL_ENDL;
		gGLManager.mIsDisabled = FALSE;
		restoreGLState();
		gTextureList.restoreGL();
		stop_glerror();
		initFonts();
		stop_glerror();
		gSky.restoreGL();
		stop_glerror();
		gPipeline.restoreGL();
		stop_glerror();
		LLDrawPoolWater::restoreGL();
		stop_glerror();
		LLManipTranslate::restoreGL();
		stop_glerror();
		gBumpImageList.restoreGL();
		stop_glerror();
		LLViewerDynamicTexture::restoreGL();
		stop_glerror();
		LLVOAvatar::restoreGL();
		stop_glerror();
		LLVOPartGroup::restoreGL();
		stop_glerror();
		gResizeScreenTexture = TRUE;
		gWindowResized = TRUE;
		if (isAgentAvatarValid() && gAgentAvatarp->isEditingAppearance())
		{
			LLVisualParamHint::requestHintUpdates();
		}
		if (!progress_message.empty())
		{
			gRestoreGLTimer.reset();
			gRestoreGL = TRUE;
			setShowProgress(TRUE);
			setProgressString(progress_message);
		}
		LL_INFOS() << "...Restoring GL done" << LL_ENDL;
		if(!LLAppViewer::instance()->restoreErrorTrap())
		{
			LL_WARNS() << " Someone took over my signal/exception handler (post restoreGL)!" << LL_ENDL;
		}
		stop_glerror();
	}
}
void LLViewerWindow::initFonts(F32 zoom_factor)
{
	if(gGLManager.mIsDisabled)
		return;
	LLFontGL::destroyAllGL();
	LLFontGL::initClass( gSavedSettings.getF32("FontScreenDPI"),
								mDisplayScale.mV[VX] * zoom_factor,
								mDisplayScale.mV[VY] * zoom_factor,
								font_dir());
	LLFontGL::loadDefaultFonts();
}
void LLViewerWindow::toggleFullscreen(BOOL show_progress)
{
	if (mWindow)
	{
		mWantFullscreen = mWindow->getFullscreen() ? FALSE : TRUE;
		mIsFullscreenChecked =  mWindow->getFullscreen() ? FALSE : TRUE;
		mShowFullscreenProgress = show_progress;
	}
}
void LLViewerWindow::getTargetWindow(BOOL& fullscreen, S32& width, S32& height) const
{
	fullscreen = mWantFullscreen;
	if (mWindow
	&&  mWindow->getFullscreen() == mWantFullscreen)
	{
		width = getWindowDisplayWidth();
		height = getWindowDisplayHeight();
	}
	else if (mWantFullscreen)
	{
		width = gSavedSettings.getS32("FullScreenWidth");
		height = gSavedSettings.getS32("FullScreenHeight");
	}
	else
	{
		width = gSavedSettings.getS32("WindowWidth");
		height = gSavedSettings.getS32("WindowHeight");
	}
}
void LLViewerWindow::requestResolutionUpdate(bool fullscreen_checked)
{
	mResDirty = true;
	mWantFullscreen = fullscreen_checked;
	mIsFullscreenChecked = fullscreen_checked;
}
BOOL LLViewerWindow::checkSettings()
{
	if (mResDirty)
	{
		if (gSavedSettings.getBOOL("FullScreenAutoDetectAspectRatio"))
		{
			getWindow()->setNativeAspectRatio(0.f);
		}
		else
		{
			getWindow()->setNativeAspectRatio(gSavedSettings.getF32("FullScreenAspectRatio"));
		}
		reshape(getWindowWidthRaw(), getWindowHeightRaw());
		if (mIsFullscreenChecked)
		{
			LLViewerCamera::getInstance()->setAspect( getDisplayAspectRatio() );
		}
		mResDirty = false;
	}
	BOOL is_fullscreen = mWindow->getFullscreen();
	if(mWantFullscreen)
	{
		if (mWindow->getMinimized()) {
			return FALSE;
		}
		LLCoordScreen screen_size;
		LLCoordScreen desired_screen_size(gSavedSettings.getS32("FullScreenWidth"),
								   gSavedSettings.getS32("FullScreenHeight"));
		getWindow()->getSize(&screen_size);
		if(!is_fullscreen ||
			screen_size.mX != desired_screen_size.mX
			|| screen_size.mY != desired_screen_size.mY)
		{
			if (!LLStartUp::canGoFullscreen())
			{
				return FALSE;
			}
			LLGLStateValidator::checkStates();
			LLGLStateValidator::checkTextureChannels();
			S32 vsync_mode = gSavedSettings.getS32("SHRenderVsyncMode");
			if(vsync_mode == -1 && !gGLManager.mHasAdaptiveVsync)
			{
				vsync_mode = 0;
			}
			changeDisplaySettings(TRUE,
								  desired_screen_size,
								  vsync_mode,
								  mShowFullscreenProgress);
			LLGLStateValidator::checkStates();
			LLGLStateValidator::checkTextureChannels();
			return TRUE;
		}
	}
	else
	{
		if(is_fullscreen)
		{
			LLGLStateValidator::checkStates();
			LLGLStateValidator::checkTextureChannels();
			changeDisplaySettings(FALSE,
								  LLCoordScreen(gSavedSettings.getS32("WindowWidth"),
												gSavedSettings.getS32("WindowHeight")),
								  TRUE,
								  mShowFullscreenProgress);
			LLGLStateValidator::checkStates();
			LLGLStateValidator::checkTextureChannels();
			return TRUE;
		}
	}
	return FALSE;
}
void LLViewerWindow::restartDisplay(BOOL show_progress_bar)
{
	LL_INFOS() << "Restaring GL" << LL_ENDL;
	stopGL();
	if (show_progress_bar)
	{
		restoreGL(true, LLTrans::getString("ProgressChangingResolution"));
	}
	else
	{
		restoreGL(true);
	}
}
BOOL LLViewerWindow::changeDisplaySettings(BOOL fullscreen, LLCoordScreen size, const S32 vsync_mode, BOOL show_progress_bar)
{
	BOOL was_maximized = mWindow->getMaximized();
	mWantFullscreen = fullscreen;
	mShowFullscreenProgress = show_progress_bar;
	gSavedSettings.setBOOL("FullScreen", mWantFullscreen);
	gResizeScreenTexture = TRUE;
	BOOL old_fullscreen = mWindow->getFullscreen();
	if (!old_fullscreen && fullscreen && !LLStartUp::canGoFullscreen())
	{
		return TRUE;
	}
	U32 fsaa = LLRenderTarget::sUseFBO ? 0 : gSavedSettings.getU32("RenderFSAASamples");
	U32 old_fsaa = mWindow->getFSAASamples();
	if (!old_fullscreen && !fullscreen)
	{
		if (!mWindow->getMaximized())
		{
			mWindow->setSize(size);
		}
		if (fsaa == old_fsaa && vsync_mode == mWindow->getVsyncMode())
		{
			return TRUE;
		}
	}
	LLFloaterSnapshot::hide(0);
	BOOL result_first_try = FALSE;
	BOOL result_second_try = FALSE;
	LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
	send_agent_pause();
	LL_INFOS() << "Stopping GL during changeDisplaySettings" << LL_ENDL;
	mIgnoreActivate = TRUE;
	LLCoordScreen old_size;
	LLCoordScreen old_pos;
	LLCoordScreen new_pos;
	mWindow->getSize(&old_size);
	BOOL got_position = mWindow->getPosition(&old_pos);
	if(!old_fullscreen && !mWindow->getMaximized() && got_position)
	{
		gSavedSettings.setS32("WindowX", old_pos.mX);
		gSavedSettings.setS32("WindowY", old_pos.mY);
	}
	if (!fullscreen && !mWindow->getMaximized())
	{
		new_pos.mX = gSavedSettings.getS32("WindowX");
		new_pos.mY = gSavedSettings.getS32("WindowY");
	}
	mWindow->setFSAASamples(fsaa);
	mWindow->setVsyncMode(vsync_mode);
	auto stopfn = [this]() { this->stopGL(); };
	auto restoreFn = [this, show_progress_bar](bool full_restore) {
		LL_INFOS() << "Restoring GL during resolution change" << LL_ENDL;
		this->restoreGL(full_restore, show_progress_bar ? LLTrans::getString("ProgressChangingResolution") : "");
	};
	result_first_try = mWindow->switchContext(fullscreen, size, vsync_mode, stopfn, restoreFn, &new_pos);
	if (!result_first_try)
	{
		mWindow->setFSAASamples(old_fsaa);
		result_second_try = mWindow->switchContext(old_fullscreen, old_size, vsync_mode, stopfn, restoreFn, &new_pos);
		if (!result_second_try)
		{
			send_agent_resume();
			mIgnoreActivate = FALSE;
			return FALSE;
		}
	}
	send_agent_resume();
	if (!result_first_try)
	{
		LLSD args;
		args["RESX"] = llformat("%d",size.mX);
		args["RESY"] = llformat("%d",size.mY);
		LLNotificationsUtil::add("ResolutionSwitchFail", args);
		size = old_size;
	}
	BOOL success = result_first_try || result_second_try;
	if (success)
	{
#if LL_WINDOWS
		if (fullscreen && result_first_try)
#endif
		{
			reshape(size.mX, size.mY);
		}
	}
	if (!mWindow->getFullscreen() && success)
	{
		if (was_maximized)
		{
			mWindow->maximize();
		}
		else
		{
			mWindow->setPosition(new_pos);
		}
	}
	mIgnoreActivate = FALSE;
	gFocusMgr.setKeyboardFocus(keyboard_focus);
	mWantFullscreen = mWindow->getFullscreen();
	mShowFullscreenProgress = FALSE;
	return success;
}
F32 LLViewerWindow::getDisplayAspectRatio() const
{
	if (mWindow->getFullscreen())
	{
		if (gSavedSettings.getBOOL("FullScreenAutoDetectAspectRatio"))
		{
			return mWindow->getNativeAspectRatio();
		}
		else
		{
			return gSavedSettings.getF32("FullScreenAspectRatio");
		}
	}
	else
	{
		return mWindow->getNativeAspectRatio();
	}
}
void LLViewerWindow::calcDisplayScale()
{
	LLVector2 ui_scale_factor = getUIScale();
	LLVector2 display_scale;
	display_scale.setVec(llmax(1.f / mWindow->getPixelAspectRatio(), 1.f), llmax(mWindow->getPixelAspectRatio(), 1.f));
	if(mWindow->getFullscreen())
	{
		F32 height_normalization = gSavedSettings.getBOOL("UIAutoScale") ? ((F32)mWindowRectRaw.getHeight() / display_scale.mV[VY]) / 768.f : 1.f;
		display_scale.scaleVec(ui_scale_factor * height_normalization);
	}
	else
	{
		display_scale.scaleVec(ui_scale_factor);
	}
	if (display_scale.mV[VX] < MIN_DISPLAY_SCALE || display_scale.mV[VY] < MIN_DISPLAY_SCALE)
	{
		display_scale *= MIN_DISPLAY_SCALE / llmin(display_scale.mV[VX], display_scale.mV[VY]);
	}
	if (mWindow->getFullscreen())
	{
		display_scale.mV[0] = ll_round(display_scale.mV[0], 2.0f/(F32) mWindowRectRaw.getWidth());
		display_scale.mV[1] = ll_round(display_scale.mV[1], 2.0f/(F32) mWindowRectRaw.getHeight());
	}
	if (display_scale != mDisplayScale)
	{
		LL_INFOS() << "Setting display scale to " << display_scale << LL_ENDL;
		mDisplayScale = display_scale;
		initFonts();
	}
}
LLVector2 LLViewerWindow::getUIScale() const
{
	static LLCachedControl<F32> ui_scale_factor("UIScaleFactor");
	if (mWindow->getFullscreen())
	{
		return LLVector2(ui_scale_factor, ui_scale_factor);
	}
	else
	{
		return LLVector2(mDPIScaleX * ui_scale_factor, mDPIScaleY * ui_scale_factor);
	}
}
S32 LLViewerWindow::getChatConsoleBottomPad()
{
	static const LLCachedControl<S32> user_offset("ConsoleBottomOffset");
	S32 offset = user_offset;
	if(gToolBar && gToolBar->getVisible())
		offset += TOOL_BAR_HEIGHT;
	return offset;
}
LLRect LLViewerWindow::getChatConsoleRect()
{
	LLRect full_window(0, getWindowHeightScaled(), getWindowWidthScaled(), 0);
	LLRect console_rect = full_window;
	const S32 CONSOLE_PADDING_TOP = 24;
	const S32 CONSOLE_PADDING_LEFT = 24;
	const S32 CONSOLE_PADDING_RIGHT = 10;
	console_rect.mTop    -= CONSOLE_PADDING_TOP;
	console_rect.mBottom += getChatConsoleBottomPad();
	console_rect.mLeft   += CONSOLE_PADDING_LEFT;
	static const LLCachedControl<bool> CHAT_FULL_WIDTH("ChatFullWidth",true);
	if (CHAT_FULL_WIDTH)
	{
		console_rect.mRight -= CONSOLE_PADDING_RIGHT;
	}
	else
	{
		console_rect.mRight  = console_rect.mLeft + 2 * getWindowWidthScaled() / 3;
	}
	return console_rect;
}
bool LLViewerWindow::onAlert(const LLSD& notify)
{
	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
	if (gNoRender)
	{
		LL_INFOS() << "Alert: " << notification->getName() << LL_ENDL;
		notification->respond(LLSD::emptyMap());
		LLNotifications::instance().cancel(notification);
		return false;
	}
	if( gAgentCamera.cameraMouselook() )
	{
		gAgentCamera.changeCameraToDefault();
	}
	return false;
}
LLBottomPanel::LLBottomPanel(const LLRect &rect) :
	LLPanel(LLStringUtil::null, rect, FALSE),
	mIndicator(NULL)
{
	setFocusRoot(TRUE);
	setIsChrome(TRUE);
	mFactoryMap["toolbar"] = LLCallbackMap(createToolBar, NULL);
	mFactoryMap["overlay"] = LLCallbackMap(createOverlayBar, NULL);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_bars.xml", &getFactoryMap());
	setOrigin(rect.mLeft, rect.mBottom);
	reshape(rect.getWidth(), rect.getHeight());
}
void LLBottomPanel::setFocusIndicator(LLView * indicator)
{
	mIndicator = indicator;
}
void LLBottomPanel::draw()
{
	if(mIndicator)
	{
		BOOL hasFocus = gFocusMgr.childHasKeyboardFocus(this);
		mIndicator->setVisible(hasFocus);
		mIndicator->setEnabled(hasFocus);
	}
	LLPanel::draw();
}
void* LLBottomPanel::createOverlayBar(void* data)
{
	delete gOverlayBar;
	gOverlayBar = new LLOverlayBar();
	return gOverlayBar;
}
void* LLBottomPanel::createToolBar(void* data)
{
	delete gToolBar;
	gToolBar = new LLToolBar();
	return gToolBar;
}
LLPickInfo::LLPickInfo()
	: mKeyMask(MASK_NONE),
	  mPickCallback(NULL),
	  mPickType(PICK_INVALID),
	  mWantSurfaceInfo(FALSE),
	  mObjectFace(-1),
	  mUVCoords(-1.f, -1.f),
	  mSTCoords(-1.f, -1.f),
	  mXYCoords(-1, -1),
	  mIntersection(),
	  mNormal(),
	  mTangent(),
	  mBinormal(),
	  mHUDIcon(NULL),
	  mPickTransparent(FALSE),
	  mPickRigged(FALSE),
	  mPickParticle(FALSE)
{
}
LLPickInfo::LLPickInfo(const LLCoordGL& mouse_pos,
						MASK keyboard_mask,
		       BOOL pick_transparent,
			   BOOL pick_rigged,
			   BOOL pick_particle,
		       BOOL pick_uv_coords,
			   BOOL pick_unselectable,
		       void (*pick_callback)(const LLPickInfo& pick_info))
	: mMousePt(mouse_pos),
	  mKeyMask(keyboard_mask),
	  mPickCallback(pick_callback),
	  mPickType(PICK_INVALID),
	  mWantSurfaceInfo(pick_uv_coords),
	  mObjectFace(-1),
	  mUVCoords(-1.f, -1.f),
	  mSTCoords(-1.f, -1.f),
	  mXYCoords(-1, -1),
	  mNormal(),
	  mTangent(),
	  mBinormal(),
	  mHUDIcon(NULL),
	  mPickTransparent(pick_transparent),
	  mPickRigged(pick_rigged),
	  mPickParticle(pick_particle),
	  mPickUnselectable(pick_unselectable)
{
}
void LLPickInfo::fetchResults()
{
	S32 face_hit = -1;
	LLVector4a intersection, normal;
	LLVector4a tangent;
	LLVector2 uv;
	LLHUDIcon* hit_icon = gViewerWindow->cursorIntersectIcon(mMousePt.mX, mMousePt.mY, 512.f, &intersection);
	LLVector4a origin;
	origin.load3(LLViewerCamera::getInstance()->getOrigin().mV);
	F32 icon_dist = 0.f;
	LLVector4a start;
	LLVector4a end;
	LLVector4a particle_end;
	if (hit_icon)
	{
		LLVector4a delta;
		delta.setSub(intersection, origin);
		icon_dist = delta.getLength3().getF32();
	}
	LLViewerObject* hit_object = gViewerWindow->cursorIntersect(mMousePt.mX, mMousePt.mY, 512.f,
									NULL, -1, mPickTransparent, mPickRigged, &face_hit,
									&intersection, &uv, &normal, &tangent, &start, &end);
	mPickPt = mMousePt;
	U32 te_offset = face_hit > -1 ? face_hit : 0;
	if (mPickParticle)
	{
		if (hit_object)
		{
			particle_end = intersection;
		}
		else
		{
			particle_end = end;
		}
	}
	LLViewerObject* objectp = hit_object;
	LLVector4a delta;
	delta.setSub(origin, intersection);
	if (hit_icon &&
		(!objectp ||
		icon_dist < delta.getLength3().getF32()))
	{
		mHUDIcon = hit_icon;
		mPickType = PICK_ICON;
		mPosGlobal = mHUDIcon->getPositionGlobal();
	}
	else if (objectp)
	{
		if( objectp->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH )
		{
			mPickType = PICK_LAND;
			mObjectID.setNull();
			LLVector3d land_pos;
			if (!gViewerWindow->mousePointOnLandGlobal(mPickPt.mX, mPickPt.mY, &land_pos, mPickUnselectable))
			{
				return;
			}
			mPosGlobal = land_pos + LLVector3d::z_axis * 0.1f;
		}
		else
		{
			if(isFlora(objectp))
			{
				mPickType = PICK_FLORA;
			}
			else
			{
				mPickType = PICK_OBJECT;
			}
			LLVector3 v_intersection(intersection.getF32ptr());
			mObjectOffset = gAgentCamera.calcFocusOffset(objectp, v_intersection, mPickPt.mX, mPickPt.mY);
			mObjectID = objectp->mID;
			mObjectFace = (te_offset == NO_FACE) ? -1 : (S32)te_offset;
			mPosGlobal = gAgent.getPosGlobalFromAgent(v_intersection);
			if (mWantSurfaceInfo)
			{
				getSurfaceInfo();
			}
		}
	}
	if (mPickParticle)
	{
		S32 part_face = -1;
		LLVOPartGroup* group = gPipeline.lineSegmentIntersectParticle(start, particle_end, NULL, &part_face);
		if (group)
		{
			mParticleOwnerID = group->getPartOwner(part_face);
			mParticleSourceID = group->getPartSource(part_face);
		}
	}
	if (mPickCallback)
	{
		mPickCallback(*this);
	}
}
LLPointer<LLViewerObject> LLPickInfo::getObject() const
{
	return gObjectList.findObject( mObjectID );
}
void LLPickInfo::updateXYCoords()
{
	if (mObjectFace > -1)
	{
		const LLTextureEntry* tep = getObject()->getTE(mObjectFace);
		LLPointer<LLViewerTexture> imagep = LLViewerTextureManager::getFetchedTexture(tep->getID());
		if(mUVCoords.mV[VX] >= 0.f && mUVCoords.mV[VY] >= 0.f && imagep.notNull())
		{
			mXYCoords.mX = ll_round(mUVCoords.mV[VX] * (F32)imagep->getWidth());
			mXYCoords.mY = ll_round((1.f - mUVCoords.mV[VY]) * (F32)imagep->getHeight());
		}
	}
}
void LLPickInfo::getSurfaceInfo()
{
	mObjectFace   = -1;
	mUVCoords     = LLVector2(-1, -1);
	mSTCoords     = LLVector2(-1, -1);
	mXYCoords     = LLCoordScreen(-1, -1);
	mIntersection = LLVector3(0,0,0);
	mNormal       = LLVector3(0,0,0);
	mBinormal     = LLVector3(0,0,0);
	mTangent	  = LLVector4(0,0,0,0);
	LLVector4a tangent;
	LLVector4a intersection;
	LLVector4a normal;
	tangent.clear();
	normal.clear();
	intersection.clear();
	LLViewerObject* objectp = getObject();
	if (objectp)
	{
		if (gViewerWindow->cursorIntersect(ll_round((F32)mMousePt.mX), ll_round((F32)mMousePt.mY), 1024.f,
										   objectp, -1, mPickTransparent, mPickRigged,
										   &mObjectFace,
										   &intersection,
										   &mSTCoords,
										   &normal,
										   &tangent))
		{
			if (objectp->mDrawable.notNull() && mObjectFace > -1)
			{
				LLFace* facep = objectp->mDrawable->getFace(mObjectFace);
				if (facep)
				{
					mUVCoords = facep->surfaceToTexture(mSTCoords, intersection, normal);
				}
			}
			mIntersection.set(intersection.getF32ptr());
			mNormal.set(normal.getF32ptr());
			mTangent.set(tangent.getF32ptr());
			LLVector4a binormal;
			binormal.setCross3(normal, tangent);
			binormal.mul(tangent.getF32ptr()[3]);
			mBinormal.set(binormal.getF32ptr());
			mBinormal.normalize();
			mNormal.normalize();
			mTangent.normalize();
			updateXYCoords();
		}
	}
}
bool LLPickInfo::isFlora(LLViewerObject* object)
{
	if (!object) return false;
	LLPCode pcode = object->getPCode();
	if( (LL_PCODE_LEGACY_GRASS == pcode)
		|| (LL_PCODE_LEGACY_TREE == pcode)
		|| (LL_PCODE_TREE_NEW == pcode))
	{
		return true;
	}
	return false;
}
