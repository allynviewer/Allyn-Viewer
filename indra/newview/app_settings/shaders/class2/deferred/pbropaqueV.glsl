/**
 * @file pbropaqueV.glsl
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

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec2 texcoord0;
ATTRIBUTE vec4 tangent;

VARYING vec3 vary_tbn0;
VARYING vec3 vary_tbn1;
VARYING vec3 vary_tbn2;
VARYING vec3 vary_normal;
VARYING vec3 vary_pos;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

void main()
{
	gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
	vary_pos = (modelview_matrix * vec4(position.xyz, 1.0)).xyz;
	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0, 0.0, 1.0)).xy;
	vec3 n = normalize(normal_matrix * normal);
	vec3 t = normalize(normal_matrix * tangent.xyz);
	vec3 b = cross(n, t) * tangent.w;
	vary_tbn0 = vec3(t.x, b.x, n.x);
	vary_tbn1 = vec3(t.y, b.y, n.y);
	vary_tbn2 = vec3(t.z, b.z, n.z);
	vary_normal = n;
	vertex_color = diffuse_color;
}
