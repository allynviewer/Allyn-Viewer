/** 
 * @file postDeferredGammaCorrect.glsl
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

uniform sampler2D diffuseRect;

VARYING vec2 vary_fragcoord;

uniform float pbr_input_srgb;
uniform float pbr_hdr;
uniform float pbr_exposure;
uniform float pbr_tonemap_type;
uniform float pbr_tonemap_mix;
uniform vec2 pbr_tex_scale;

vec3 linear_to_srgb(vec3 cl);
vec3 srgb_to_linear(vec3 cs);

vec3 pbr_aces(vec3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 pbr_khronos(vec3 color)
{
	float startCompression = 0.76;
	float desaturation = 0.15;
	float x = min(color.r, min(color.g, color.b));
	float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
	color -= offset;
	float peak = max(color.r, max(color.g, color.b));
	if (peak < startCompression)
	{
		return color;
	}
	float d = 1.0 - startCompression;
	float newPeak = 1.0 - d * d / (peak + d - startCompression);
	color *= newPeak / peak;
	float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
	return mix(color, vec3(newPeak), g);
}

void main() 
{
	vec2 uv = vary_fragcoord;
	if (pbr_tex_scale.x > 0.0)
	{
		uv *= pbr_tex_scale;
	}
	vec4 diff = texture2D(diffuseRect, uv);
	vec3 col = diff.rgb;
	if (pbr_input_srgb > 0.5)
	{
		col = srgb_to_linear(col);
	}
	if (pbr_hdr > 0.5)
	{
		col *= pbr_exposure;
	}
	vec3 mapped = pbr_tonemap_type > 0.5 ? pbr_aces(col) : pbr_khronos(col);
	col = mix(col, mapped, clamp(pbr_tonemap_mix, 0.0, 1.0));
	diff.rgb = linear_to_srgb(col);
	frag_color = diff;
}

