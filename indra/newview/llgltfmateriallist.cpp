#include "llviewerprecompiledheaders.h"
#include "llgltfmateriallist.h"
#include "llassetstorage.h"
#include "llframetimer.h"
#include "llvfile.h"
#include "llviewercontrol.h"
#include "llglslshader.h"
#include "lltextureentry.h"
#include "llface.h"
#include <vector>

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
		if (entry.tries < 4)
		{
			entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * entry.tries;
		}
		else
		{
			entry.mat->mReady = true;
			++mReadyGeneration;
			LL_WARNS("GLTF") << "material " << asset_id << " status " << status << LL_ENDL;
		}
		return;
	}
	LLVFile file(vfs, asset_id, LLAssetType::AT_MATERIAL, LLVFile::READ);
	const S32 size = file.getSize();
	if (size <= 0)
	{
		++entry.tries;
		if (entry.tries < 4)
		{
			entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * entry.tries;
		}
		else
		{
			entry.mat->mReady = true;
			++mReadyGeneration;
		}
		return;
	}
	std::vector<U8> buffer((size_t)size);
	if (!file.read(&buffer[0], size, FALSE))
	{
		++entry.tries;
		if (entry.tries < 4)
		{
			entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * entry.tries;
		}
		else
		{
			entry.mat->mReady = true;
			++mReadyGeneration;
		}
		return;
	}
	if (!entry.mat->fromBytes(&buffer[0], size))
	{
		++entry.tries;
		if (entry.tries < 4)
		{
			entry.next = LLFrameTimer::getTotalSeconds() + 2.0 * entry.tries;
		}
		else
		{
			entry.mat->mReady = true;
			++mReadyGeneration;
		}
		LL_WARNS("GLTF") << "material " << asset_id << " " << entry.mat->mParseError << LL_ENDL;
		return;
	}
	++mReadyGeneration;
}

LLGLTFMaterial* LLGLTFMaterialList::getMaterial(const LLUUID& id)
{
	if (id.isNull())
	{
		return NULL;
	}
	Entry& entry = mEntries[id];
	if (!entry.mat)
	{
		entry.mat = new LLGLTFMaterial();
		entry.mat->mAssetId = id;
	}
	if (entry.mat->mReady || entry.pending || entry.tries >= 4)
	{
		return entry.mat;
	}
	const F64 now = LLFrameTimer::getTotalSeconds();
	if (entry.tries > 0 && now < entry.next)
	{
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

	F32 reflectionAmount(F32 gloss, F32 metal)
	{
		F32 probes = probeScale();
		if (gPBR.coverage <= 0)
		{
			return 0.f;
		}
		if (gPBR.coverage == 1)
		{
			return (gloss > 0.65f || metal > 0.75f) ? probes : 0.f;
		}
		if (gPBR.coverage == 2)
		{
			return probes * ((gloss > 0.3f || metal > 0.2f) ? 1.f : 0.35f);
		}
		return probes;
	}

	bool evalMaterial(const LLTextureEntry* te, LLColor4& spec, F32& env, F32& gloss, F32& emissive, LLColor3& emitColor)
	{
		if (!te)
		{
			return false;
		}
		LLGLTFMaterial* mat = te->getGLTFMaterial();
		if (!mat || !mat->mReady)
		{
			return false;
		}
		F32 metal = llclamp(mat->mMetallic, 0.f, 1.f);
		F32 rough = llclamp(mat->mRoughness, 0.f, 1.f);
		gloss = 1.f - rough;
		F32 dielectric = 0.04f;
		spec.mV[0] = dielectric * (1.f - metal) + mat->mBaseColor.mV[0] * metal;
		spec.mV[1] = dielectric * (1.f - metal) + mat->mBaseColor.mV[1] * metal;
		spec.mV[2] = dielectric * (1.f - metal) + mat->mBaseColor.mV[2] * metal;
		spec.mV[3] = gloss;
		F32 amount = reflectionAmount(gloss, metal);
		env = llmax(metal, gloss * metal) * amount;
		if (gPBR.mirrors && gloss > 0.7f)
		{
			env = llmax(env, amount);
		}
		emitColor = mat->mEmissive;
		emissive = 0.f;
		if (gPBR.hdr)
		{
			emissive = llclamp(llmax(emitColor.mV[0], llmax(emitColor.mV[1], emitColor.mV[2])), 0.f, 1.f);
		}
		return true;
	}
}

void LLPBRGraphics::update()
{
	gPBR.detail = gSavedSettings.getS32("RenderReflectionProbeDetail");
	gPBR.coverage = gSavedSettings.getS32("RenderReflectionProbeLevel");
	gPBR.probes = gSavedSettings.getS32("RenderReflectionProbeCount");
	gPBR.hdr = gSavedSettings.getBOOL("RenderDisableVintageMode");
	gPBR.exposure = gSavedSettings.getF32("RenderExposure");
	gPBR.ssr = gSavedSettings.getBOOL("RenderScreenSpaceReflections");
	gPBR.mirrors = gSavedSettings.getBOOL("RenderMirrors");
	gPBR.tonemap = gSavedSettings.getS32("RenderTonemapType");
	gPBR.mix = gSavedSettings.getF32("RenderTonemapMix");
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
	shader->uniform1f(hdr_name, gPBR.hdr ? 1.f : 0.f);
}

void LLPBRGraphics::bindFace(LLGLSLShader* shader, const LLTextureEntry* te)
{
	if (!shader)
	{
		return;
	}
	update();
	LLColor4 spec(0.f, 0.f, 0.f, 0.f);
	LLColor3 emit(0.f, 0.f, 0.f);
	F32 env = 0.f;
	F32 gloss = 0.f;
	F32 emissive = 0.f;
	bool active = evalMaterial(te, spec, env, gloss, emissive, emit);
	static const LLStaticHashedString active_name("pbr_active");
	static const LLStaticHashedString spec_name("pbr_spec");
	static const LLStaticHashedString gloss_name("pbr_gloss");
	static const LLStaticHashedString env_name("pbr_env");
	static const LLStaticHashedString emissive_name("pbr_emissive");
	static const LLStaticHashedString emit_name("pbr_emit_color");
	shader->uniform1f(active_name, active ? 1.f : 0.f);
	shader->uniform3f(spec_name, spec.mV[0], spec.mV[1], spec.mV[2]);
	shader->uniform1f(gloss_name, active ? gloss : 0.f);
	shader->uniform1f(env_name, active ? env : 0.f);
	shader->uniform1f(emissive_name, active ? emissive : 0.f);
	shader->uniform3f(emit_name, emit.mV[0], emit.mV[1], emit.mV[2]);
	bindGlobals(shader);
}

void LLPBRGraphics::bindDraw(LLGLSLShader* shader, LLFace* face)
{
	const LLTextureEntry* te = NULL;
	if (face && face->getViewerObject())
	{
		te = face->getTextureEntry();
	}
	bindFace(shader, te);
}

bool LLPBRGraphics::apply(const LLTextureEntry* te, LLColor4& spec, F32& env, F32& emissive)
{
	update();
	LLColor4 pbr_spec = spec;
	LLColor3 emit(0.f, 0.f, 0.f);
	F32 pbr_env = env;
	F32 gloss = spec.mV[3];
	F32 pbr_emissive = 0.f;
	if (!evalMaterial(te, pbr_spec, pbr_env, gloss, pbr_emissive, emit))
	{
		return false;
	}
	spec = pbr_spec;
	env = pbr_env;
	if (gPBR.hdr)
	{
		emissive = llmax(emissive, pbr_emissive);
	}
	return true;
}

bool LLPBRGraphics::gradeActive()
{
	update();
	if (gPBR.mix > 0.01f)
	{
		return true;
	}
	return gPBR.hdr && fabsf(gPBR.exposure - 1.f) > 0.01f;
}

void LLPBRGraphics::bindGrade(LLGLSLShader& shader, bool grade, F32 scale_x, F32 scale_y)
{
	update();
	static const LLStaticHashedString srgb_name("pbr_input_srgb");
	static const LLStaticHashedString hdr_name("pbr_hdr");
	static const LLStaticHashedString exposure_name("pbr_exposure");
	static const LLStaticHashedString type_name("pbr_tonemap_type");
	static const LLStaticHashedString mix_name("pbr_tonemap_mix");
	static const LLStaticHashedString scale_name("pbr_tex_scale");
	shader.uniform1f(srgb_name, grade ? 1.f : 0.f);
	shader.uniform1f(hdr_name, (grade && gPBR.hdr) ? 1.f : 0.f);
	shader.uniform1f(exposure_name, (grade && gPBR.hdr) ? gPBR.exposure : 1.f);
	shader.uniform1f(type_name, grade ? (F32)gPBR.tonemap : 0.f);
	shader.uniform1f(mix_name, grade ? llclamp(gPBR.mix, 0.f, 1.f) : 0.f);
	shader.uniform2f(scale_name, scale_x, scale_y);
}
