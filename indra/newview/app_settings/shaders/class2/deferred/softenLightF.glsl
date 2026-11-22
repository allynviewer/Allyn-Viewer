/** 
 * @file softenLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * Modifications Copyright (C) 2026 - Allyn Viewer
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
uniform sampler2D specularRect;
uniform sampler2D normalMap;
uniform sampler2D lightMap;
uniform sampler2D depthMap;
uniform samplerCube environmentMap;
uniform sampler2D	  lightFunc;

uniform float blur_size;
uniform float blur_fidelity;
uniform float pbr_refl_detail;
uniform float pbr_ssr;
uniform float pbr_mirrors;
uniform sampler2D emissiveRect;
uniform float pbr_hdr;
uniform float pbr_is_hud;
uniform float pbr_probe_lod_max;
uniform float pbr_probe_count;
uniform float pbr_deferred_atmospheric;
uniform vec3 pbr_cam_pos;
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

// Inputs
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
//uniform vec4 camPosWorld;
uniform vec4 gamma;
uniform vec4 lightnorm;
uniform vec4 sunlight_color;
uniform vec4 ambient;
uniform vec4 blue_horizon;
uniform vec4 blue_density;
uniform float haze_horizon;
uniform float haze_density;
uniform float cloud_shadow;
uniform float density_multiplier;
uniform float distance_multiplier;
uniform float max_y;
uniform vec4 glow;
uniform float global_gamma;
uniform float scene_light_strength;
uniform mat3 env_mat;
uniform vec4 shadow_clip;
uniform float ssao_effect;

uniform vec3 sun_dir;
VARYING vec2 vary_fragcoord;

vec3 vary_PositionEye;

vec3 vary_SunlitColor;
vec3 vary_AmblitColor;
vec3 vary_AdditiveColor;
vec3 vary_AtmosAttenuation;

uniform mat4 inv_proj;

vec3 decode_normal(vec2 enc);
vec3 srgb_to_linear(vec3 cs);
vec3 linear_to_srgb(vec3 cl);

vec4 getPosition_d(vec2 pos_screen, float depth)
{
	vec2 sc = pos_screen.xy*2.0;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

vec3 getPositionEye()
{
	return vary_PositionEye;
}
vec3 getSunlitColor()
{
	return vary_SunlitColor;
}
vec3 getAmblitColor()
{
	return vary_AmblitColor;
}
vec3 getAdditiveColor()
{
	return vary_AdditiveColor;
}
vec3 getAtmosAttenuation()
{
	return vary_AtmosAttenuation;
}

void setPositionEye(vec3 v)
{
	vary_PositionEye = v;
}

void setSunlitColor(vec3 v)
{
	vary_SunlitColor = v;
}

void setAmblitColor(vec3 v)
{
	vary_AmblitColor = v;
}

void setAdditiveColor(vec3 v)
{
	vary_AdditiveColor = v;
}

void setAtmosAttenuation(vec3 v)
{
	vary_AtmosAttenuation = v;
}

#ifdef WATER_FOG
uniform vec4 waterPlane;
uniform vec4 waterFogColor;
uniform float waterFogDensity;
uniform float waterFogKS;

vec4 applyWaterFogDeferred(vec3 pos, vec4 color)
{
	//normalize view vector
	vec3 view = normalize(pos);
	float es = -(dot(view, waterPlane.xyz));

	//find intersection point with water plane and eye vector
	
	//get eye depth
	float e0 = max(-waterPlane.w, 0.0);
	
	vec3 int_v = waterPlane.w > 0.0 ? view * waterPlane.w/es : vec3(0.0, 0.0, 0.0);
	
	//get object depth
	float depth = length(pos - int_v);
		
	//get "thickness" of water
	float l = max(depth, 0.1);

	float kd = waterFogDensity;
	float ks = waterFogKS;
	vec4 kc = waterFogColor;
	
	float F = 0.98;
	
	float t1 = -kd * pow(F, ks * e0);
	float t2 = kd + ks * es;
	float t3 = pow(F, t2*l) - 1.0;
	
	float L = min(t1/t2*t3, 1.0);
	
	float D = pow(0.98, l*kd);
	
	color.rgb = color.rgb * D + kc.rgb * L;
	color.a = kc.a + color.a;
	
	return color;
}
#endif

void calcAtmospherics(vec3 inPositionEye, float ambFactor) {

	vec3 P = inPositionEye;
	setPositionEye(P);
	
	vec3 tmpLightnorm = lightnorm.xyz;

	vec3 Pn = normalize(P);
	float Plen = length(P);

	vec4 temp1 = vec4(0);
	vec3 temp2 = vec3(0);
	vec4 blue_weight;
	vec4 haze_weight;
	vec4 sunlight = sunlight_color;
	vec4 light_atten;

	//sunlight attenuation effect (hue and brightness) due to atmosphere
	//this is used later for sunlight modulation at various altitudes
	light_atten = (blue_density + vec4(haze_density * 0.25)) * (density_multiplier * max_y);
		//I had thought blue_density and haze_density should have equal weighting,
		//but attenuation due to haze_density tends to seem too strong

	temp1 = blue_density + vec4(haze_density);
	blue_weight = blue_density / temp1;
	haze_weight = vec4(haze_density) / temp1;

	//(TERRAIN) compute sunlight from lightnorm only (for short rays like terrain)
	temp2.y = max(0.0, tmpLightnorm.y);
	temp2.y = 1. / temp2.y;
	sunlight *= exp( - light_atten * temp2.y);

	// main atmospheric scattering line integral
	temp2.z = Plen * density_multiplier;

	// Transparency (-> temp1)
	// ATI Bugfix -- can't store temp1*temp2.z*distance_multiplier in a variable because the ati
	// compiler gets confused.
	temp1 = exp(-temp1 * temp2.z * distance_multiplier);

	//final atmosphere attenuation factor
	setAtmosAttenuation(temp1.rgb);
	
	//compute haze glow
	//(can use temp2.x as temp because we haven't used it yet)
	temp2.x = dot(Pn, tmpLightnorm.xyz);
	temp2.x = 1. - temp2.x;
		//temp2.x is 0 at the sun and increases away from sun
	temp2.x = max(temp2.x, .03);	//was glow.y
		//set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
	temp2.x *= glow.x;
		//higher glow.x gives dimmer glow (because next step is 1 / "angle")
	temp2.x = pow(temp2.x, glow.z);
		//glow.z should be negative, so we're doing a sort of (1 / "angle") function

	//add "minimum anti-solar illumination"
	temp2.x += .25;
	
	//increase ambient when there are more clouds
	vec4 tmpAmbient = ambient + (vec4(1.) - ambient) * cloud_shadow * 0.5;

	//haze color
	setAdditiveColor(
		vec3(blue_horizon * blue_weight * (sunlight*(1.-cloud_shadow) + tmpAmbient)
	  + (haze_horizon * haze_weight) * (sunlight*(1.-cloud_shadow) * temp2.x
		  + tmpAmbient)));

	// decrease ambient value for occluded areas
	tmpAmbient *= mix(ssao_effect, 1.0, ambFactor);
	
	//brightness of surface both sunlight and ambient
	/*setSunlitColor(pow(vec3(sunlight * .5), vec3(global_gamma)) * global_gamma);
	setAmblitColor(pow(vec3(tmpAmbient * .25), vec3(global_gamma)) * global_gamma);
	setAdditiveColor(pow(getAdditiveColor() * vec3(1.0 - temp1), vec3(global_gamma)) * global_gamma);*/

	setSunlitColor(vec3(sunlight * .5));
	setAmblitColor(vec3(tmpAmbient * .25));
	setAdditiveColor(getAdditiveColor() * vec3(1.0 - temp1));
}

vec3 atmosLighting(vec3 light)
{
	light *= getAtmosAttenuation().r;
	light += getAdditiveColor();
	return (2.0 * light);
}

vec3 atmosTransport(vec3 light) {
	light *= getAtmosAttenuation().r;
	light += getAdditiveColor() * 2.0;
	return light;
}

vec3 fullbrightAtmosTransport(vec3 light) {
	float brightness = dot(light.rgb, vec3(0.33333));

	return mix(atmosTransport(light.rgb), light.rgb + getAdditiveColor().rgb, brightness * brightness);
}



vec3 atmosGetDiffuseSunlightColor()
{
	return getSunlitColor();
}

vec3 scaleDownLight(vec3 light)
{
	return (light / scene_light_strength );
}

vec3 scaleUpLight(vec3 light)
{
	return (light * scene_light_strength);
}

vec3 atmosAmbient(vec3 light)
{
	return getAmblitColor() + light / 2.0;
}

vec3 atmosAffectDirectionalLight(float lightIntensity)
{
	return getSunlitColor() * lightIntensity;
}

vec3 scaleSoftClip(vec3 light)
{
	//soft clip effect:
	light = 1. - clamp(light, vec3(0.), vec3(1.));
	light = 1. - pow(light, gamma.xxx);

	return light;
}


vec3 fullbrightScaleSoftClip(vec3 light)
{
	//soft clip effect:
	return light;
}

float luminance(vec3 color)
{
	/// CALCULATING LUMINANCE (Using NTSC lum weights)
	/// http://en.wikipedia.org/wiki/Luma_%28video%29
	return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 allyn_pbr_light(vec3 albedo, vec3 orm, vec3 N, vec3 emissive, vec3 view_pos, bool unlit)
{
	float ao = orm.r;
	float rough = clamp(orm.g, 0.045, 1.0);
	float metal = clamp(orm.b, 0.0, 1.0);
	if (unlit)
	{
		return albedo + emissive;
	}
	vec3 V = normalize(-view_pos);
	vec3 F0 = mix(vec3(0.04), albedo, metal);
	vec3 L = normalize(sun_dir);
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
	vec3 specbrdf = D * G * F / max(4.0 * NdotV * NdotL, 0.001);
	vec3 kd = (1.0 - F) * (1.0 - metal);
	vec3 direct = (kd * albedo / 3.14159265 + specbrdf) * atmosAffectDirectionalLight(1.0) * NdotL;
	vec3 R = env_mat * reflect(-V, N);
	vec3 Nworld = env_mat * N;
	vec3 Fenv = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
	vec3 world = pbr_cam_pos + (env_mat * view_pos);
	vec3 irradiance = vec3(0.0);
	vec3 radiance = vec3(0.0);
	float lod = rough * pbr_probe_lod_max;
#if __VERSION__ >= 400
	if (pbr_use_array < 0.5)
	{
		if (pbr_probe_count > 0.5)
		{
#if __VERSION__ >= 130
			irradiance = textureLod(pbrIrradianceCube, Nworld, 0.0).rgb;
			radiance = textureLod(pbrProbeCube, R, lod).rgb;
#else
			irradiance = textureCube(pbrIrradianceCube, Nworld).rgb;
			radiance = textureCube(pbrProbeCube, R).rgb;
#endif
		}
	}
	else
	{
		float wsum = 0.0;
		vec3 isum = vec3(0.0);
		vec3 rsum = vec3(0.0);
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
				isum += textureLod(pbrIrradianceArray, vec4(Nworld, pbr_probe_param[i].x), 0.0).rgb * w;
				rsum += textureLod(pbrProbeArray, vec4(R, pbr_probe_param[i].x), lod).rgb * w;
				wsum += w;
			}
		}
		if (wsum > 0.0)
		{
			irradiance = isum / wsum;
			radiance = rsum / wsum;
		}
	}
#else
	if (pbr_probe_count > 0.5)
	{
#if __VERSION__ >= 130
		irradiance = textureLod(pbrIrradianceCube, Nworld, 0.0).rgb;
		radiance = textureLod(pbrProbeCube, R, lod).rgb;
#else
		irradiance = textureCube(pbrIrradianceCube, Nworld).rgb;
		radiance = textureCube(pbrProbeCube, R).rgb;
#endif
	}
#endif
	if (dot(irradiance, vec3(1.0)) <= 0.0)
	{
		irradiance = atmosAmbient(vec3(0.0));
	}
	if (dot(radiance, vec3(1.0)) <= 0.0)
	{
		radiance = textureCube(environmentMap, R).rgb;
	}
	vec3 indirect = irradiance * albedo * (1.0 - metal) * ao + radiance * Fenv * ao;
	vec3 mirror = vec3(0.0);
	if (pbr_mirror_present > 0.5)
	{
		vec4 clip = pbr_mirror_vp * vec4(world, 1.0);
		vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
		vec4 boxp = pbr_mirror_box * vec4(world, 1.0);
		float side = dot(pbr_mirror_normal, world - pbr_mirror_point) * dot(pbr_mirror_normal, pbr_cam_pos - pbr_mirror_point);
		if (clip.w > 0.0 && side > 0.0 && uv.x > 0.0 && uv.x < 1.0 && uv.y > 0.0 && uv.y < 1.0 && abs(boxp.x) <= 1.0 && abs(boxp.y) <= 1.0 && abs(boxp.z) <= 1.0)
		{
			mirror = texture2D(pbrMirrorMap, uv).rgb * Fenv;
		}
	}
	return direct + indirect + mirror + emissive;
}

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	float depth = texture2D(depthMap, tc.xy).r;
	vec3 pos = getPosition_d(tc, depth).xyz;
	vec4 norm = texture2D(normalMap, tc);
	float envIntensity = norm.z;
	norm.xyz = decode_normal(norm.xy); // unpack norm
		
	vec4 diffuse = texture2D(diffuseRect, tc);
	vec4 spec = texture2D(specularRect, vary_fragcoord.xy);
	if (norm.w > 0.2 && norm.w < 0.85)
	{
		calcAtmospherics(pos.xyz, 1.0);
		vec3 lit = allyn_pbr_light(diffuse.rgb, spec.rgb, normalize(norm.xyz), texture2D(emissiveRect, vary_fragcoord.xy).rgb, pos.xyz, norm.w > 0.5);
		if (pbr_hdr < 0.5 && norm.w <= 0.5)
		{
			lit = srgb_to_linear(scaleSoftClip(lit));
		}
		frag_color = vec4(lit, 0.0);
		return;
	}
	if (pbr_deferred_atmospheric < 0.5)
	{
		frag_color = vec4(0.0);
		return;
	}

	diffuse.rgb = linear_to_srgb(diffuse.rgb);
	spec.rgb = srgb_to_linear(spec.rgb);
	
	vec3 col;
	float bloom = 0.0;
	{
		bloom = spec.r*norm.w;
		
		if (norm.w < 0.5)
		{
		
		float da = max(dot(norm.xyz, sun_dir.xyz), 0.0);

		float light_gamma = 1.0/1.3;
		da = pow(da, light_gamma);
	
		vec2 scol_ambocc = texture2D(lightMap, vary_fragcoord.xy).rg;
		scol_ambocc = pow(scol_ambocc, vec2(light_gamma));

		float scol = max(scol_ambocc.r, diffuse.a); 

		

		float ambocc = scol_ambocc.g;
	
		calcAtmospherics(pos.xyz, ambocc);
	
		col = atmosAmbient(vec3(0));
		float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
		ambient *= 0.5;
		ambient *= ambient;
		ambient = (1.0-ambient);

		col.rgb *= ambient;

		col += atmosAffectDirectionalLight(max(min(da, scol), 0.0));
	
		col *= diffuse.rgb;
	
		vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));
		
		if (spec.a > 0.0) // specular reflection
		{
			// the old infinite-sky shiny reflection
			//
			
			float sa = dot(refnormpersp, sun_dir.xyz);
			vec3 dumbshiny = vary_SunlitColor*scol_ambocc.r*(texture2D(lightFunc, vec2(sa, spec.a)).r);
			
			// add the two types of shiny together
			vec3 spec_contrib = dumbshiny * spec.rgb;
			bloom = dot(spec_contrib, spec_contrib) / 6;
			col += spec_contrib;
			
		}
	
		
		col = mix(col, diffuse.rgb, diffuse.a);

		if (envIntensity > 0.0)
		{
			vec3 env_vec = env_mat * refnormpersp;
			vec3 refcol = textureCube(environmentMap, env_vec).rgb;
			bloom = (luminance(refcol) - .45)*.25*scol_ambocc.r;
			col = mix(col.rgb, refcol, envIntensity);
		}
						
		//if (norm.w < 0.5)
		{
			col = mix(atmosLighting(col), fullbrightAtmosTransport(col), diffuse.a);
			col = mix(scaleSoftClip(col), fullbrightScaleSoftClip(col), diffuse.a);
			//bloom += (luminance(col))*.075; //This looks nice, but requires a larger glow rendertarget.
		}

		#ifdef WATER_FOG
			vec4 fogged = applyWaterFogDeferred(pos,vec4(col, bloom));
			col = fogged.rgb;
			bloom = fogged.a;
		#endif
		}
		else
		{
			col = diffuse.rgb;
		}

		col = srgb_to_linear(col);

		//col = vec3(1,0,1);
		//col.g = envIntensity;
	}
	
	frag_color.rgb = col;
	frag_color.a = bloom;
}
