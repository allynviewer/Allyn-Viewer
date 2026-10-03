/** 
 * @file bumpF.glsl
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
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;

VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

vec2 encode_normal(vec3 n);

uniform float pbr_active;
uniform vec3 pbr_spec;
uniform float pbr_gloss;
uniform float pbr_env;
uniform float pbr_emissive;
uniform vec3 pbr_emit_color;

void main() 
{
	vec3 col = vertex_color.rgb * texture2D(diffuseMap, vary_texcoord0.xy).rgb;
	vec3 norm = texture2D(bumpMap, vary_texcoord0.xy).rgb * 2.0 - 1.0;

	vec3 tnorm = vec3(dot(norm,vary_mat0),
			  dot(norm,vary_mat1),
			  dot(norm,vary_mat2));

	vec4 spec = vertex_color.aaaa;
	float env = vertex_color.a;
	float emissive = 0.0;
	if (pbr_active > 0.5)
	{
		spec = vec4(pbr_spec, pbr_gloss);
		env = pbr_env;
		emissive = pbr_emissive;
		col = mix(col, pbr_emit_color, emissive);
	}
						
	frag_data[0] = vec4(col, emissive);
	frag_data[1] = spec;
	vec3 nvn = normalize(tnorm);
	frag_data[2] = vec4(encode_normal(nvn.xyz), env, 0.0);
}
