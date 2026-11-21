/**
 * @file reflectionmipF.glsl
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

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform float pbr_face;
uniform float pbr_roughness;
uniform float pbr_layer;

#if __VERSION__ >= 400
uniform samplerCubeArray pbrSourceArray;
uniform samplerCube pbrSourceCube;
uniform float pbr_use_array;
#else
uniform samplerCube pbrSourceCube;
#endif

VARYING vec2 vary_fragcoord;

vec3 pbr_face_dir(float face, vec2 uv)
{
	vec2 a = uv * 2.0 - 1.0;
	if (face < 0.5) return vec3(1.0, -a.y, -a.x);
	if (face < 1.5) return vec3(-1.0, -a.y, a.x);
	if (face < 2.5) return vec3(a.x, 1.0, a.y);
	if (face < 3.5) return vec3(a.x, -1.0, -a.y);
	if (face < 4.5) return vec3(a.x, -a.y, 1.0);
	return vec3(-a.x, -a.y, -1.0);
}

vec3 pbr_sample_source(vec3 dir)
{
#if __VERSION__ >= 400
	if (pbr_use_array > 0.5)
	{
		return textureLod(pbrSourceArray, vec4(dir, pbr_layer), 0.0).rgb;
	}
	return textureLod(pbrSourceCube, dir, 0.0).rgb;
#else
	return textureCube(pbrSourceCube, dir).rgb;
#endif
}

void main()
{
	vec3 N = normalize(pbr_face_dir(pbr_face, vary_fragcoord));
	vec2 texel = mix(vec2(1.0 / 128.0), vec2(1.0 / 4.0), clamp(pbr_roughness, 0.0, 1.0));
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 T = normalize(cross(up, N));
	vec3 B = cross(N, T);
	vec3 sum = pbr_sample_source(N);
	sum += pbr_sample_source(normalize(N + T * texel.x));
	sum += pbr_sample_source(normalize(N - T * texel.x));
	sum += pbr_sample_source(normalize(N + B * texel.y));
	sum += pbr_sample_source(normalize(N - B * texel.y));
	frag_color = vec4(sum / 5.0, 1.0);
}
