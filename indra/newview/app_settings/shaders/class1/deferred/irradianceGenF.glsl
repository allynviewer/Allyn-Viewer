/**
 * @file irradianceGenF.glsl
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
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 T = normalize(cross(up, N));
	vec3 B = cross(N, T);
	const float xi[16] = float[16](0.03, 0.09, 0.16, 0.22, 0.28, 0.34, 0.41, 0.47, 0.53, 0.59, 0.66, 0.72, 0.78, 0.84, 0.91, 0.97);
	const float yi[16] = float[16](0.11, 0.67, 0.23, 0.79, 0.35, 0.91, 0.47, 0.03, 0.59, 0.15, 0.71, 0.27, 0.83, 0.39, 0.95, 0.51);
	vec3 sum = vec3(0.0);
	float weight = 0.0;
	for (int i = 0; i < 16; i++)
	{
		float cos_theta = yi[i];
		float sin_theta = sqrt(max(1.0 - cos_theta * cos_theta, 0.0));
		float phi = 6.2831853 * xi[i];
		vec3 L = normalize(T * (sin_theta * cos(phi)) + B * (sin_theta * sin(phi)) + N * cos_theta);
		float NdotL = max(dot(N, L), 0.0);
		sum += pbr_sample_source(L) * NdotL;
		weight += NdotL;
	}
	frag_color = vec4(sum / max(weight, 0.001), 1.0);
}
