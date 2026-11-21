/**
 * @file llreflectionprobe.h
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
#ifndef LL_LLREFLECTIONPROBE_H
#define LL_LLREFLECTIONPROBE_H

#include "llgl.h"

class LLGLSLShader;

bool allynPBRCubeArraySupported();
bool allynHDRColorTarget();
void allynSetHDRColorTarget(bool enabled);
void allynCaptureReflections();
void allynBindReflectionState(LLGLSLShader* shader, S32 probe_unit);
void allynReleaseReflections();

extern LLGLSLShader gRadianceGenProgram;
extern LLGLSLShader gReflectionMipProgram;
extern LLGLSLShader gIrradianceGenProgram;

#endif
