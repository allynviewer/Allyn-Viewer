/** 
 * @file llviewerjoint.cpp
 * @brief Implementation of LLViewerJoint class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llviewerjoint.h"
#include "llgl.h"
#include "llrender.h"
#include "llmath.h"
#include "llglheaders.h"
#include "llvoavatar.h"
#include "pipeline.h"
static const S32 MIN_PIXEL_AREA_3PASS_HAIR = 64*64;
LLViewerJoint::LLViewerJoint() :
	LLAvatarJoint()
{ }
LLViewerJoint::LLViewerJoint(S32 joint_num) :
	LLAvatarJoint(joint_num)
{ }
LLViewerJoint::LLViewerJoint(const std::string &name, LLJoint *parent) :
	LLAvatarJoint(name, parent)
{ }
LLViewerJoint::~LLViewerJoint()
{
}
U32 LLViewerJoint::render( F32 pixelArea, BOOL first_pass, BOOL is_dummy )
{
	stop_glerror();
	U32 triangle_count = 0;
	if ( mValid )
	{
		if ( is_dummy )
		{
			triangle_count += drawShape( pixelArea, first_pass, is_dummy );
		}
		else if (LLPipeline::sShadowRender)
		{
			triangle_count += drawShape(pixelArea, first_pass, is_dummy );
		}
		else if ( isTransparent() && !LLPipeline::sReflectionRender)
		{
			if ((pixelArea > MIN_PIXEL_AREA_3PASS_HAIR))
			{
				LLGLDisable<GL_CULL_FACE> cull;
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape( pixelArea, first_pass, is_dummy );
				}
				gGL.setColorMask(false, false);
				{
					triangle_count += drawShape( pixelArea, FALSE, is_dummy  );
				}
				gGL.setColorMask(true, false);
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape( pixelArea, FALSE, is_dummy  );
				}
			}
			else
			{
				glCullFace(GL_FRONT);
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape( pixelArea, first_pass, is_dummy  );
				}
				glCullFace(GL_BACK);
				{
					triangle_count += drawShape( pixelArea, FALSE, is_dummy  );
				}
			}
		}
		else
		{
			triangle_count += drawShape( pixelArea, first_pass );
		}
	}
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLAvatarJoint* joint = dynamic_cast<LLAvatarJoint*>(*iter);
		if (!joint)
			continue;
		F32 jointLOD = joint->getLOD();
		if (pixelArea >= jointLOD || sDisableLOD)
		{
			triangle_count += joint->render( pixelArea, TRUE, is_dummy );
			if (jointLOD != DEFAULT_AVATAR_JOINT_LOD)
			{
				break;
			}
		}
	}
	return triangle_count;
}
U32 LLViewerJoint::drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy )
{
	return 0;
}
