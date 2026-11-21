/**
 * @file lldrawpoolgltfpbr.h
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * $/LicenseInfo$
 */
#ifndef LL_LLDRAWPOOLGLTFPBR_H
#define LL_LLDRAWPOOLGLTFPBR_H

#include "lldrawpool.h"

class LLGLSLShader;
extern LLGLSLShader gDeferredPBRProgram;

class LLDrawPoolGLTFPBR : public LLRenderPass
{
public:
	LLDrawPoolGLTFPBR(U32 type);
	LLDrawPool* instancePool();
	U32 getVertexDataMask();
	void prerender();
	void render(S32 pass = 0);
	S32 getNumPasses();
	S32 getNumDeferredPasses();
	void renderDeferred(S32 pass);
	S32 getNumPostDeferredPasses();
	void renderPostDeferred(S32 pass);
	S32 getNumShadowPasses();
	void renderShadow(S32 pass);
private:
	void drawGeometry(U32 pass, F32 output_mode);
};

#endif
