/** 
 * @file lightF.glsl
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

uniform sampler2D diffuseMap;

vec3 atmosLighting(vec3 light);
vec3 scaleSoftClip(vec3 light);

uniform float pbr_active;
uniform vec3 pbr_spec;
uniform float pbr_gloss;
uniform float pbr_env;
uniform float pbr_emissive;
uniform vec3 pbr_emit_color;
uniform float pbr_refl_detail;
uniform float pbr_ssr;
uniform float pbr_mirrors;

void pbr_forward(inout vec3 color)
{
	if (pbr_active < 0.5)
	{
		return;
	}
	float sharp = clamp(pbr_refl_detail, 0.0, 2.0) / 2.0;
	vec3 refl = mix(vec3(0.45, 0.52, 0.62), vec3(1.15, 1.05, 0.92), sharp);
	float amt = clamp(pbr_env, 0.0, 1.0);
	color = mix(color, refl * (0.35 + color) + pbr_spec * amt, amt);
	if (pbr_ssr > 0.5)
	{
		color += refl * pbr_gloss * amt * 0.35;
	}
	if (pbr_mirrors > 0.5 && pbr_gloss > 0.7)
	{
		color = mix(color, refl, amt);
	}
	if (pbr_emissive > 0.0)
	{
		color = mix(color, pbr_emit_color, pbr_emissive);
	}
}

void default_lighting() 
{
	vec4 color = texture2D(diffuseMap,vary_texcoord0.xy) * vertex_color;
	
	if(color.a < .004)
	{
		discard;
	}

	color.rgb = atmosLighting(color.rgb);
	pbr_forward(color.rgb);

	color.rgb = scaleSoftClip(color.rgb);

	frag_color = color;
}

