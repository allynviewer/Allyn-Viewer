/** 
 * @file llgltfmateriallist.cpp
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
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
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llgltfmateriallist.h"
#include "llassetstorage.h"
#include "llframetimer.h"
#include "llvfile.h"
#include "llviewercontrol.h"
#include "llglslshader.h"
#include "lltextureentry.h"
#include "llface.h"
#include "llreflectionprobe.h"
#include "llviewertexture.h"
#include "llrender.h"
#include <nlohmann/json.hpp>
#include <map>
#include <vector>

namespace
{
	struct MapUV
	{
		F32 scale_x;
		F32 scale_y;
		F32 off_x;
		F32 off_y;
		F32 rot;
		MapUV()
		:	scale_x(1.f),
			scale_y(1.f),
			off_x(0.f),
			off_y(0.f),
			rot(0.f)
		{
		}
	};

	struct MaterialUV
	{
		MapUV slot[4];
	};

	std::map<LLUUID, MaterialUV> gMaterialUV;

	F32 jsonReal(const nlohmann::json& value, F32 fallback)
	{
		if (value.is_number_float())
		{
			return (F32)value.get<double>();
		}
		if (value.is_number_integer())
		{
			return (F32)value.get<std::int64_t>();
		}
		if (value.is_number_unsigned())
		{
			return (F32)value.get<std::uint64_t>();
		}
		return fallback;
	}

	void readUV(const nlohmann::json& info, MapUV& uv)
	{
		if (!info.is_object() || !info.contains("extensions") || !info["extensions"].is_object())
		{
			return;
		}
		const nlohmann::json& ext = info["extensions"];
		if (!ext.contains("KHR_texture_transform") || !ext["KHR_texture_transform"].is_object())
		{
			return;
		}
		const nlohmann::json& transform = ext["KHR_texture_transform"];
		if (transform.contains("scale") && transform["scale"].is_array() && transform["scale"].size() >= 2)
		{
			uv.scale_x = jsonReal(transform["scale"][0], 1.f);
			uv.scale_y = jsonReal(transform["scale"][1], 1.f);
		}
		if (transform.contains("offset") && transform["offset"].is_array() && transform["offset"].size() >= 2)
		{
			uv.off_x = jsonReal(transform["offset"][0], 0.f);
			uv.off_y = jsonReal(transform["offset"][1], 0.f);
		}
		if (transform.contains("rotation"))
		{
			uv.rot = jsonReal(transform["rotation"], 0.f);
		}
	}

	void storeMaterialUV(const LLUUID& id, const U8* data, S32 size)
	{
		MaterialUV uv;
		if (data && size > 0)
		{
			try
			{
				const nlohmann::json model = nlohmann::json::parse(data, data + size);
				if (model.is_object() && model.contains("materials") && model["materials"].is_array() && !model["materials"].empty() && model["materials"][0].is_object())
				{
					const nlohmann::json& mat = model["materials"][0];
					if (mat.contains("pbrMetallicRoughness") && mat["pbrMetallicRoughness"].is_object())
					{
						const nlohmann::json& pbr = mat["pbrMetallicRoughness"];
						if (pbr.contains("baseColorTexture"))
						{
							readUV(pbr["baseColorTexture"], uv.slot[LLGLTFMaterial::BASE_COLOR]);
						}
						if (pbr.contains("metallicRoughnessTexture"))
						{
							readUV(pbr["metallicRoughnessTexture"], uv.slot[LLGLTFMaterial::METALLIC_ROUGHNESS]);
						}
					}
					if (mat.contains("normalTexture"))
					{
						readUV(mat["normalTexture"], uv.slot[LLGLTFMaterial::NORMAL]);
					}
					if (mat.contains("emissiveTexture"))
					{
						readUV(mat["emissiveTexture"], uv.slot[LLGLTFMaterial::EMISSIVE]);
					}
				}
			}
			catch (const std::exception&)
			{
			}
		}
		gMaterialUV[id] = uv;
	}
}

void LLGLTFMaterialList::onAsset(LLVFS* vfs, const LLUUID& asset_id, LLAssetType::EType, void*, S32 status, LLExtStat)
{
	if (LLGLTFMaterialList::instanceExists())
	{
		LLGLTFMaterialList::instance().onAssetArrived(vfs, asset_id, status);
	}
}

void LLGLTFMaterialList::onAssetArrived(LLVFS* vfs, const LLUUID& asset_id, S32 status)
{
	std::map<LLUUID, Entry>::iterator it = mEntries.find(asset_id);
	if (it == mEntries.end() || !it->second.mat)
	{
		return;
	}
	Entry& entry = it->second;
	entry.pending = false;
	if (status < 0 || !vfs)
	{
		++entry.tries;
		entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * llmax(entry.tries, 1);
		LL_WARNS("GLTF") << "material " << asset_id << " status " << status << " try " << entry.tries << LL_ENDL;
		return;
	}
	LLVFile file(vfs, asset_id, LLAssetType::AT_MATERIAL, LLVFile::READ);
	const S32 size = file.getSize();
	if (size <= 0)
	{
		++entry.tries;
		entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * llmax(entry.tries, 1);
		LL_WARNS("GLTF") << "material " << asset_id << " empty try " << entry.tries << LL_ENDL;
		return;
	}
	std::vector<U8> buffer((size_t)size);
	if (!file.read(&buffer[0], size, FALSE))
	{
		++entry.tries;
		entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * llmax(entry.tries, 1);
		LL_WARNS("GLTF") << "material " << asset_id << " read fail try " << entry.tries << LL_ENDL;
		return;
	}
	if (!entry.mat->fromBytes(&buffer[0], size))
	{
		++entry.tries;
		entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * llmax(entry.tries, 1);
		LL_WARNS("GLTF") << "material " << asset_id << " " << entry.mat->mParseError << " try " << entry.tries << LL_ENDL;
		return;
	}
	entry.tries = 0;
	storeMaterialUV(asset_id, &buffer[0], size);
	++mReadyGeneration;
}

LLGLTFMaterial* LLGLTFMaterialList::findMaterial(const LLUUID& id) const
{
	if (id.isNull())
	{
		return NULL;
	}
	std::map<LLUUID, Entry>::const_iterator it = mEntries.find(id);
	if (it == mEntries.end())
	{
		return NULL;
	}
	return it->second.mat;
}
static const LLUUID kBlankPBRMaterial("968cbad0-4dad-d64e-71b5-72bf13ad051a");
LLGLTFMaterial* LLGLTFMaterialList::getMaterial(const LLUUID& id)
{
	if (id.isNull())
	{
		return NULL;
	}
	Entry& entry = mEntries[id];
	if (id == kBlankPBRMaterial)
	{
		if (!entry.mat)
		{
			entry.mat = new LLGLTFMaterial();
			entry.mat->mAssetId = id;
		}
		entry.mat->mReady = true;
		entry.pending = false;
		return entry.mat;
	}
	if (!entry.mat)
	{
		entry.mat = new LLGLTFMaterial();
		entry.mat->mAssetId = id;
	}
	if (entry.mat->mReady || entry.pending)
	{
		return entry.mat;
	}
	const F64 now = LLFrameTimer::getTotalSeconds();
	if (entry.tries > 0 && now < entry.next)
	{
		return entry.mat;
	}
	if (entry.tries >= 8)
	{
		entry.next = now + 30.0;
		entry.tries = 4;
		return entry.mat;
	}
	if (!gAssetStorage)
	{
		return entry.mat;
	}
	entry.pending = true;
	gAssetStorage->getAssetData(id, LLAssetType::AT_MATERIAL, onAsset, NULL, TRUE);
	return entry.mat;
}

namespace
{
	struct PBRState
	{
		S32 detail;
		S32 coverage;
		S32 probes;
		bool hdr;
		F32 exposure;
		bool ssr;
		bool mirrors;
		S32 tonemap;
		F32 mix;
	};

	PBRState gPBR;

	F32 probeScale()
	{
		return llclamp((F32)gPBR.probes / 256.f, 0.f, 1.f);
	}

}

void LLPBRGraphics::update()
{
	static const LLCachedControl<S32> detail("RenderReflectionProbeDetail");
	static const LLCachedControl<S32> coverage("RenderReflectionProbeLevel");
	static const LLCachedControl<S32> probes("RenderReflectionProbeCount");
	static const LLCachedControl<bool> hdr("RenderDisableVintageMode");
	static const LLCachedControl<F32> exposure("RenderExposure");
	static const LLCachedControl<bool> ssr("RenderScreenSpaceReflections");
	static const LLCachedControl<bool> mirrors("RenderMirrors");
	static const LLCachedControl<S32> tonemap("RenderTonemapType");
	static const LLCachedControl<F32> mix("RenderTonemapMix");
	gPBR.detail = detail;
	gPBR.coverage = coverage;
	gPBR.probes = probes;
	gPBR.hdr = hdr;
	gPBR.exposure = exposure;
	gPBR.ssr = ssr;
	gPBR.mirrors = mirrors;
	gPBR.tonemap = tonemap;
	gPBR.mix = mix;
}

void LLPBRGraphics::bindGlobals(LLGLSLShader* shader)
{
	if (!shader)
	{
		return;
	}
	update();
	static const LLStaticHashedString detail_name("pbr_refl_detail");
	static const LLStaticHashedString coverage_name("pbr_refl_coverage");
	static const LLStaticHashedString probe_name("pbr_probe_scale");
	static const LLStaticHashedString ssr_name("pbr_ssr");
	static const LLStaticHashedString mirror_name("pbr_mirrors");
	static const LLStaticHashedString hdr_name("pbr_hdr");
	shader->uniform1f(detail_name, (F32)gPBR.detail);
	shader->uniform1f(coverage_name, (F32)gPBR.coverage);
	shader->uniform1f(probe_name, probeScale());
	shader->uniform1f(ssr_name, gPBR.ssr ? 1.f : 0.f);
	shader->uniform1f(mirror_name, gPBR.mirrors ? 1.f : 0.f);
	shader->uniform1f(hdr_name, allynHDRColorTarget() ? 1.f : 0.f);
}

void LLPBRGraphics::bindFace(LLGLSLShader* shader, const LLTextureEntry* te)
{
	if (!shader)
	{
		return;
	}
	update();
	bindGlobals(shader);
	MapUV uv[4];
	F32 rough = 1.f;
	F32 metal = 0.f;
	LLColor3 emit(0.f, 0.f, 0.f);
	F32 alpha_mode = 0.f;
	F32 unlit = 0.f;
	LLGLTFMaterial* mat = te ? te->getGLTFMaterial() : NULL;
	if (mat && mat->mReady)
	{
		rough = mat->mRoughness;
		metal = mat->mMetallic;
		emit = mat->mEmissive;
		if (mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
		{
			alpha_mode = 2.f;
		}
		else if (mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND)
		{
			alpha_mode = 1.f;
		}
		unlit = mat->mUnlit ? 1.f : 0.f;
		if (te)
		{
			std::map<LLUUID, MaterialUV>::const_iterator found = gMaterialUV.find(te->getGLTFMaterialId());
			if (found != gMaterialUV.end())
			{
				for (S32 i = 0; i < 4; ++i)
				{
					uv[i] = found->second.slot[i];
				}
			}
		}
	}
	shader->uniform2f(LLStaticHashedString("pbr_uv_scale_b"), uv[LLGLTFMaterial::BASE_COLOR].scale_x, uv[LLGLTFMaterial::BASE_COLOR].scale_y);
	shader->uniform2f(LLStaticHashedString("pbr_uv_off_b"), uv[LLGLTFMaterial::BASE_COLOR].off_x, uv[LLGLTFMaterial::BASE_COLOR].off_y);
	shader->uniform1f(LLStaticHashedString("pbr_uv_rot_b"), uv[LLGLTFMaterial::BASE_COLOR].rot);
	shader->uniform2f(LLStaticHashedString("pbr_uv_scale_n"), uv[LLGLTFMaterial::NORMAL].scale_x, uv[LLGLTFMaterial::NORMAL].scale_y);
	shader->uniform2f(LLStaticHashedString("pbr_uv_off_n"), uv[LLGLTFMaterial::NORMAL].off_x, uv[LLGLTFMaterial::NORMAL].off_y);
	shader->uniform1f(LLStaticHashedString("pbr_uv_rot_n"), uv[LLGLTFMaterial::NORMAL].rot);
	shader->uniform2f(LLStaticHashedString("pbr_uv_scale_m"), uv[LLGLTFMaterial::METALLIC_ROUGHNESS].scale_x, uv[LLGLTFMaterial::METALLIC_ROUGHNESS].scale_y);
	shader->uniform2f(LLStaticHashedString("pbr_uv_off_m"), uv[LLGLTFMaterial::METALLIC_ROUGHNESS].off_x, uv[LLGLTFMaterial::METALLIC_ROUGHNESS].off_y);
	shader->uniform1f(LLStaticHashedString("pbr_uv_rot_m"), uv[LLGLTFMaterial::METALLIC_ROUGHNESS].rot);
	shader->uniform2f(LLStaticHashedString("pbr_uv_scale_e"), uv[LLGLTFMaterial::EMISSIVE].scale_x, uv[LLGLTFMaterial::EMISSIVE].scale_y);
	shader->uniform2f(LLStaticHashedString("pbr_uv_off_e"), uv[LLGLTFMaterial::EMISSIVE].off_x, uv[LLGLTFMaterial::EMISSIVE].off_y);
	shader->uniform1f(LLStaticHashedString("pbr_uv_rot_e"), uv[LLGLTFMaterial::EMISSIVE].rot);
	shader->uniform1f(LLStaticHashedString("pbr_rough_factor"), rough);
	shader->uniform1f(LLStaticHashedString("pbr_metal_factor"), metal);
	shader->uniform3f(LLStaticHashedString("pbr_emit_factor"), emit.mV[0], emit.mV[1], emit.mV[2]);
	shader->uniform1f(LLStaticHashedString("pbr_alpha_mode"), alpha_mode);
	shader->uniform1f(LLStaticHashedString("pbr_unlit"), unlit);
}

void LLPBRGraphics::bindDraw(LLGLSLShader* shader, LLFace* face)
{
	const LLTextureEntry* te = NULL;
	if (face && face->getViewerObject())
	{
		te = face->getTextureEntry();
	}
	bindFace(shader, te);
	if (!shader || !te || te->getGLTFMaterialId().isNull() || !face)
	{
		return;
	}
	LLViewerTexture* white = LLViewerFetchedTexture::sWhiteImagep.get();
	LLViewerTexture* base = face->getTexture(LLRender::DIFFUSE_MAP);
	LLViewerTexture* normal = face->getTexture(LLRender::NORMAL_MAP);
	LLViewerTexture* orm = face->getTexture(LLRender::SPECULAR_MAP);
	LLViewerTexture* emissive = face->getTexture(LLRender::EMISSIVE_MAP);
	if (!base)
	{
		base = white;
	}
	if (!orm)
	{
		orm = white;
	}
	if (!emissive)
	{
		emissive = white;
	}
	shader->uniform1i(LLStaticHashedString("diffuseMap"), 0);
	shader->uniform1i(LLStaticHashedString("specularMap"), 1);
	shader->uniform1i(LLStaticHashedString("bumpMap"), 2);
	shader->uniform1i(LLStaticHashedString("emissiveMap"), 3);
	shader->uniform1f(LLStaticHashedString("pbr_has_normal"), normal ? 1.f : 0.f);
	if (base)
	{
		gGL.getTexUnit(0)->bind(base);
	}
	if (orm)
	{
		gGL.getTexUnit(1)->bind(orm);
	}
	if (normal)
	{
		gGL.getTexUnit(2)->bind(normal);
	}
	if (emissive)
	{
		gGL.getTexUnit(3)->bind(emissive);
	}
	gGL.getTexUnit(0)->activate();
}

bool LLPBRGraphics::apply(const LLTextureEntry*, LLColor4&, F32&, F32&)
{
	return false;
}

bool LLPBRGraphics::gradeActive()
{
	update();
	if (!allynHDRColorTarget())
	{
		return false;
	}
	if (gPBR.mix > 0.01f)
	{
		return true;
	}
	return fabsf(gPBR.exposure - 1.f) > 0.01f;
}

void LLPBRGraphics::bindGrade(LLGLSLShader& shader, bool grade, F32 scale_x, F32 scale_y)
{
	update();
	bool tone = grade && gPBR.hdr && allynHDRColorTarget();
	static const LLStaticHashedString srgb_name("pbr_input_srgb");
	static const LLStaticHashedString hdr_name("pbr_hdr");
	static const LLStaticHashedString exposure_name("pbr_exposure");
	static const LLStaticHashedString type_name("pbr_tonemap_type");
	static const LLStaticHashedString mix_name("pbr_tonemap_mix");
	static const LLStaticHashedString scale_name("pbr_tex_scale");
	shader.uniform1f(srgb_name, 0.f);
	shader.uniform1f(hdr_name, tone ? 1.f : 0.f);
	shader.uniform1f(exposure_name, tone ? gPBR.exposure : 1.f);
	shader.uniform1f(type_name, tone ? (F32)gPBR.tonemap : 0.f);
	shader.uniform1f(mix_name, tone ? llclamp(gPBR.mix, 0.f, 1.f) : 0.f);
	shader.uniform2f(scale_name, scale_x, scale_y);
}
