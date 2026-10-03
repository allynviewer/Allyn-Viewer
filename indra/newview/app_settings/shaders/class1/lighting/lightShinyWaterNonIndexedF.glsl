/** 
 * @file lightShinyWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_texcoord1;

uniform sampler2D diffuseMap;
uniform samplerCube environmentMap;
uniform float pbr_probe_scale;
uniform float pbr_refl_coverage;
uniform float pbr_refl_detail;
uniform float pbr_mirrors;
uniform float pbr_ssr;

float pbr_shiny_amount(float gloss)
{
	float refl = gloss;
	if (pbr_refl_coverage > 0.0 || pbr_probe_scale > 0.0)
	{
		float cov = 0.0;
		if (pbr_refl_coverage > 2.5) cov = 1.0;
		else if (pbr_refl_coverage > 1.5) cov = 0.7;
		else if (pbr_refl_coverage > 0.5) cov = gloss > 0.5 ? 1.0 : 0.0;
		float probes = pbr_probe_scale > 0.0 ? pbr_probe_scale : 1.0;
		refl = gloss * cov * probes;
		if (pbr_mirrors > 0.5 && gloss > 0.25) refl = max(refl, 0.8 * probes);
		if (pbr_ssr > 0.5) refl = min(refl + gloss * 0.25, 1.0);
	}
	return clamp(refl, 0.0, 1.0);
}
float pbr_shiny_lod()
{
	float lod = 0.0;
	if (pbr_refl_coverage > 0.0 || pbr_probe_scale > 0.0)
	{
		lod = (2.0 - clamp(pbr_refl_detail, 0.0, 2.0)) * 2.5;
		if (pbr_mirrors > 0.5) lod = 0.0;
	}
	return lod;
}

vec3 atmosLighting(vec3 light);
vec4 applyWaterFog(vec4 color);

void shiny_lighting_water()
{
	vec4 color = texture2D(diffuseMap,vary_texcoord0.xy);
	color.rgb *= vertex_color.rgb;
	
	vec3 envColor = textureCube(environmentMap, vary_texcoord1.xyz, pbr_shiny_lod()).rgb;
	color.rgb = mix(color.rgb, envColor.rgb, pbr_shiny_amount(vertex_color.a));

	color.rgb = atmosLighting(color.rgb);
	color.a = 1.0;
	frag_color = applyWaterFog(color);
}

