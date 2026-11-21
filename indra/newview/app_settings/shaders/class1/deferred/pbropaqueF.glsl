/**
 * @file pbropaqueF.glsl
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
out vec4 frag_data[4];
#else
#define frag_data gl_FragData
#endif

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D bumpMap;
uniform sampler2D emissiveMap;

uniform vec2 pbr_uv_scale_b;
uniform vec2 pbr_uv_off_b;
uniform float pbr_uv_rot_b;
uniform vec2 pbr_uv_scale_n;
uniform vec2 pbr_uv_off_n;
uniform float pbr_uv_rot_n;
uniform vec2 pbr_uv_scale_m;
uniform vec2 pbr_uv_off_m;
uniform float pbr_uv_rot_m;
uniform vec2 pbr_uv_scale_e;
uniform vec2 pbr_uv_off_e;
uniform float pbr_uv_rot_e;
uniform float pbr_rough_factor;
uniform float pbr_metal_factor;
uniform vec3 pbr_emit_factor;
uniform float pbr_has_normal;
uniform float pbr_alpha_mode;
uniform float pbr_unlit;
uniform float pbr_hdr;
uniform float pbr_is_hud;
uniform float pbr_output_mode;
uniform float minimum_alpha;
uniform vec3 pbr_sun_dir;
uniform vec3 pbr_sun_color;
uniform mat3 pbr_view_to_world;
uniform vec3 pbr_cam_pos;
uniform float pbr_probe_lod_max;
uniform float pbr_probe_count;
uniform sampler2D pbrMirrorMap;
uniform float pbr_mirror_present;
uniform vec3 pbr_mirror_point;
uniform vec3 pbr_mirror_normal;
uniform mat4 pbr_mirror_vp;
uniform mat4 pbr_mirror_box;

#if __VERSION__ >= 400
uniform samplerCubeArray pbrProbeArray;
uniform samplerCubeArray pbrIrradianceArray;
uniform samplerCube pbrProbeCube;
uniform samplerCube pbrIrradianceCube;
uniform float pbr_use_array;
uniform vec4 pbr_probe_origin[8];
uniform vec4 pbr_probe_param[8];
#else
uniform samplerCube pbrProbeCube;
uniform samplerCube pbrIrradianceCube;
#endif

VARYING vec3 vary_tbn0;
VARYING vec3 vary_tbn1;
VARYING vec3 vary_tbn2;
VARYING vec3 vary_normal;
VARYING vec3 vary_pos;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

vec2 encode_normal(vec3 n);
vec3 srgb_to_linear(vec3 cs);
vec3 linear_to_srgb(vec3 cl);

vec2 pbr_map_uv(vec2 uv, vec2 scale, vec2 offset, float rot)
{
	vec2 scaled = uv * scale;
	float c = cos(rot);
	float s = sin(rot);
	return vec2(c * scaled.x + s * scaled.y, -s * scaled.x + c * scaled.y) + offset;
}

vec3 pbr_sample_radiance(vec3 dir, float lod)
{
#if __VERSION__ >= 400
	if (pbr_use_array < 0.5)
	{
		if (pbr_probe_count > 0.5)
		{
			return textureLod(pbrProbeCube, dir, lod).rgb;
		}
		return vec3(0.0);
	}
	vec3 sum = vec3(0.0);
	float wsum = 0.0;
	vec3 world = pbr_cam_pos + (pbr_view_to_world * vary_pos);
	for (int i = 0; i < 8; i++)
	{
		if (float(i) >= pbr_probe_count)
		{
			break;
		}
		vec4 org = pbr_probe_origin[i];
		float dist = length(world - org.xyz);
		float band = max(org.w * 0.3, 8.0);
		float start = max(org.w - band, 0.0);
		float w = 1.0 - smoothstep(start, org.w, dist);
		if (w > 0.0)
		{
			sum += textureLod(pbrProbeArray, vec4(dir, pbr_probe_param[i].x), lod).rgb * w;
			wsum += w;
		}
	}
	if (wsum <= 0.0)
	{
		return vec3(0.0);
	}
	return sum / wsum;
#else
	if (pbr_probe_count > 0.5)
	{
#if __VERSION__ >= 130
		return textureLod(pbrProbeCube, dir, lod).rgb;
#else
		return textureCube(pbrProbeCube, dir).rgb;
#endif
	}
	return vec3(0.0);
#endif
}

vec3 pbr_sample_irradiance(vec3 dir)
{
#if __VERSION__ >= 400
	if (pbr_use_array < 0.5)
	{
		if (pbr_probe_count > 0.5)
		{
			return textureLod(pbrIrradianceCube, dir, 0.0).rgb;
		}
		return vec3(0.0);
	}
	vec3 sum = vec3(0.0);
	float wsum = 0.0;
	vec3 world = pbr_cam_pos + (pbr_view_to_world * vary_pos);
	for (int i = 0; i < 8; i++)
	{
		if (float(i) >= pbr_probe_count)
		{
			break;
		}
		vec4 org = pbr_probe_origin[i];
		float dist = length(world - org.xyz);
		float band = max(org.w * 0.3, 8.0);
		float start = max(org.w - band, 0.0);
		float w = 1.0 - smoothstep(start, org.w, dist);
		if (w > 0.0)
		{
			sum += textureLod(pbrIrradianceArray, vec4(dir, pbr_probe_param[i].x), 0.0).rgb * w;
			wsum += w;
		}
	}
	if (wsum <= 0.0)
	{
		return vec3(0.0);
	}
	return sum / wsum;
#else
	if (pbr_probe_count > 0.5)
	{
#if __VERSION__ >= 130
		return textureLod(pbrIrradianceCube, dir, 0.0).rgb;
#else
		return textureCube(pbrIrradianceCube, dir).rgb;
#endif
	}
	return vec3(0.0);
#endif
}

vec3 pbr_forward_light(vec3 albedo, vec3 orm, vec3 N, vec3 emissive)
{
	vec3 V = normalize(-vary_pos);
	float ao = orm.r;
	float rough = clamp(orm.g, 0.045, 1.0);
	float metal = clamp(orm.b, 0.0, 1.0);
	vec3 F0 = mix(vec3(0.04), albedo, metal);
	vec3 L = normalize(pbr_sun_dir);
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	vec3 H = normalize(V + L);
	float NdotH = max(dot(N, H), 0.0);
	float VdotH = max(dot(V, H), 0.0);
	float a = rough * rough;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	float D = a2 / (3.14159265 * denom * denom);
	float k = (rough + 1.0) * (rough + 1.0) / 8.0;
	float G = (NdotV / (NdotV * (1.0 - k) + k)) * (NdotL / (NdotL * (1.0 - k) + k));
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
	vec3 spec = D * G * F / max(4.0 * NdotV * NdotL, 0.001);
	vec3 kd = (1.0 - F) * (1.0 - metal);
	vec3 direct = (kd * albedo / 3.14159265 + spec) * pbr_sun_color * NdotL;
	vec3 R = pbr_view_to_world * reflect(-V, N);
	vec3 Nworld = pbr_view_to_world * N;
	vec3 Fenv = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
	vec3 indirect = pbr_sample_irradiance(Nworld) * albedo * (1.0 - metal) * ao + pbr_sample_radiance(R, rough * pbr_probe_lod_max) * Fenv * ao;
	vec3 mirror = vec3(0.0);
	if (pbr_mirror_present > 0.5)
	{
		vec3 world = pbr_cam_pos + (pbr_view_to_world * vary_pos);
		vec4 clip = pbr_mirror_vp * vec4(world, 1.0);
		vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
		vec4 boxp = pbr_mirror_box * vec4(world, 1.0);
		float side = dot(pbr_mirror_normal, world - pbr_mirror_point) * dot(pbr_mirror_normal, pbr_cam_pos - pbr_mirror_point);
		if (clip.w > 0.0 && side > 0.0 && uv.x > 0.0 && uv.x < 1.0 && uv.y > 0.0 && uv.y < 1.0 && abs(boxp.x) <= 1.0 && abs(boxp.y) <= 1.0 && abs(boxp.z) <= 1.0)
		{
			mirror = texture2D(pbrMirrorMap, uv).rgb * Fenv;
		}
	}
	if (pbr_unlit > 0.5)
	{
		return albedo + emissive;
	}
	return direct + indirect + mirror + emissive;
}

void main()
{
	vec2 uv_b = pbr_map_uv(vary_texcoord0, pbr_uv_scale_b, pbr_uv_off_b, pbr_uv_rot_b);
	vec2 uv_n = pbr_map_uv(vary_texcoord0, pbr_uv_scale_n, pbr_uv_off_n, pbr_uv_rot_n);
	vec2 uv_m = pbr_map_uv(vary_texcoord0, pbr_uv_scale_m, pbr_uv_off_m, pbr_uv_rot_m);
	vec2 uv_e = pbr_map_uv(vary_texcoord0, pbr_uv_scale_e, pbr_uv_off_e, pbr_uv_rot_e);
	vec4 base = texture2D(diffuseMap, uv_b);
	vec3 albedo = srgb_to_linear(base.rgb) * vertex_color.rgb;
	float alpha = base.a * vertex_color.a;
	if (pbr_alpha_mode > 1.5 && alpha < minimum_alpha)
	{
		discard;
	}
	vec3 orm = texture2D(specularMap, uv_m).rgb;
	orm.g = clamp(orm.g * pbr_rough_factor, 0.045, 1.0);
	orm.b = clamp(orm.b * pbr_metal_factor, 0.0, 1.0);
	vec3 N = normalize(vary_normal);
	if (pbr_has_normal > 0.5)
	{
		vec3 ts = texture2D(bumpMap, uv_n).xyz * 2.0 - 1.0;
		N = normalize(vary_tbn0 * ts.x + vary_tbn1 * ts.y + vary_tbn2 * ts.z);
	}
	if (!gl_FrontFacing)
	{
		N = -N;
	}
	vec3 emissive = srgb_to_linear(texture2D(emissiveMap, uv_e).rgb) * pbr_emit_factor;
	if (pbr_output_mode < 0.5 && pbr_hdr < 0.5 && pbr_is_hud < 0.5)
	{
		emissive = vec3(0.0);
	}
	if (pbr_output_mode > 0.5)
	{
		vec3 lit = pbr_forward_light(albedo, orm, N, emissive);
		frag_data[0] = vec4(linear_to_srgb(lit), alpha);
		frag_data[1] = vec4(0.0);
		frag_data[2] = vec4(0.0);
		frag_data[3] = vec4(0.0);
		return;
	}
	frag_data[0] = vec4(linear_to_srgb(albedo), 0.0);
	frag_data[1] = vec4(orm, 0.0);
	frag_data[2] = vec4(encode_normal(normalize(N)), 0.0, pbr_unlit > 0.5 ? 0.667 : 0.333);
	frag_data[3] = vec4(emissive, 0.0);
}
