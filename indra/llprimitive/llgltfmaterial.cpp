#include "linden_common.h"
#include "llgltfmaterial.h"
#include "llmath.h"
#include "llsdserialize.h"
#include <nlohmann/json.hpp>
#include <sstream>

namespace
{
	bool acceptedMaterialVersion(const std::string& version)
	{
		return version == "1.0" || version == "1.1";
	}

	F32 jsonReal(const nlohmann::json& value, F32 fallback)
	{
		if (!value.is_number())
		{
			return fallback;
		}
		return (F32)value.get<double>();
	}

	LLUUID textureIdFromGltf(const nlohmann::json& model, const nlohmann::json& texture_info)
	{
		if (!texture_info.is_object() || !texture_info.contains("index") || !texture_info["index"].is_number())
		{
			return LLUUID::null;
		}
		const int texture_index = (int)texture_info["index"].get<double>();
		if (texture_index < 0 || !model.contains("textures") || !model["textures"].is_array())
		{
			return LLUUID::null;
		}
		const nlohmann::json& textures = model["textures"];
		if (texture_index >= (int)textures.size() || !textures[texture_index].is_object())
		{
			return LLUUID::null;
		}
		const nlohmann::json& texture = textures[texture_index];
		if (!texture.contains("source") || !texture["source"].is_number())
		{
			return LLUUID::null;
		}
		const int image_index = (int)texture["source"].get<double>();
		if (image_index < 0 || !model.contains("images") || !model["images"].is_array())
		{
			return LLUUID::null;
		}
		const nlohmann::json& images = model["images"];
		if (image_index >= (int)images.size() || !images[image_index].is_object())
		{
			return LLUUID::null;
		}
		const nlohmann::json& image = images[image_index];
		if (!image.contains("uri") || !image["uri"].is_string())
		{
			return LLUUID::null;
		}
		LLUUID id;
		if (!id.set(image["uri"].get<std::string>(), FALSE))
		{
			return LLUUID::null;
		}
		return id;
	}

	bool applyGltfJson(LLGLTFMaterial& material, const nlohmann::json& model)
	{
		if (!model.is_object() || !model.contains("materials") || !model["materials"].is_array() || model["materials"].empty())
		{
			material.mParseError = "json sem materials";
			return false;
		}
		const nlohmann::json& gltf_material = model["materials"][0];
		if (!gltf_material.is_object())
		{
			material.mParseError = "material 0 nao e objeto";
			return false;
		}
		if (gltf_material.contains("pbrMetallicRoughness") && gltf_material["pbrMetallicRoughness"].is_object())
		{
			const nlohmann::json& pbr = gltf_material["pbrMetallicRoughness"];
			if (pbr.contains("baseColorTexture"))
			{
				material.mTextureId[LLGLTFMaterial::BASE_COLOR] = textureIdFromGltf(model, pbr["baseColorTexture"]);
			}
			if (pbr.contains("metallicRoughnessTexture"))
			{
				material.mTextureId[LLGLTFMaterial::METALLIC_ROUGHNESS] = textureIdFromGltf(model, pbr["metallicRoughnessTexture"]);
			}
			if (pbr.contains("baseColorFactor") && pbr["baseColorFactor"].is_array() && pbr["baseColorFactor"].size() >= 3)
			{
				const nlohmann::json& factor = pbr["baseColorFactor"];
				material.mBaseColor.set(
					jsonReal(factor[0], 1.f),
					jsonReal(factor[1], 1.f),
					jsonReal(factor[2], 1.f),
					factor.size() >= 4 ? jsonReal(factor[3], 1.f) : 1.f);
			}
			if (pbr.contains("metallicFactor"))
			{
				material.mMetallic = llclamp(jsonReal(pbr["metallicFactor"], material.mMetallic), 0.f, 1.f);
			}
			if (pbr.contains("roughnessFactor"))
			{
				material.mRoughness = llclamp(jsonReal(pbr["roughnessFactor"], material.mRoughness), 0.f, 1.f);
			}
		}
		if (gltf_material.contains("normalTexture"))
		{
			material.mTextureId[LLGLTFMaterial::NORMAL] = textureIdFromGltf(model, gltf_material["normalTexture"]);
		}
		if (gltf_material.contains("emissiveTexture"))
		{
			material.mTextureId[LLGLTFMaterial::EMISSIVE] = textureIdFromGltf(model, gltf_material["emissiveTexture"]);
		}
		if (gltf_material.contains("emissiveFactor") && gltf_material["emissiveFactor"].is_array() && gltf_material["emissiveFactor"].size() >= 3)
		{
			const nlohmann::json& factor = gltf_material["emissiveFactor"];
			material.mEmissive.set(jsonReal(factor[0], 0.f), jsonReal(factor[1], 0.f), jsonReal(factor[2], 0.f));
		}
		if (gltf_material.contains("alphaMode") && gltf_material["alphaMode"].is_string())
		{
			const std::string mode = gltf_material["alphaMode"].get<std::string>();
			if (mode == "BLEND")
			{
				material.mAlphaMode = LLGLTFMaterial::ALPHA_MODE_BLEND;
			}
			else if (mode == "MASK")
			{
				material.mAlphaMode = LLGLTFMaterial::ALPHA_MODE_MASK;
			}
			else
			{
				material.mAlphaMode = LLGLTFMaterial::ALPHA_MODE_OPAQUE;
			}
		}
		if (gltf_material.contains("alphaCutoff"))
		{
			material.mAlphaCutoff = llclamp(jsonReal(gltf_material["alphaCutoff"], material.mAlphaCutoff), 0.f, 1.f);
		}
		if (gltf_material.contains("doubleSided") && gltf_material["doubleSided"].is_boolean())
		{
			material.mDoubleSided = gltf_material["doubleSided"].get<bool>();
		}
		if (gltf_material.contains("extensions") && gltf_material["extensions"].is_object()
			&& gltf_material["extensions"].contains("KHR_materials_unlit"))
		{
			material.mUnlit = true;
		}
		return true;
	}
}

LLGLTFMaterial::LLGLTFMaterial()
:	mBaseColor(1.f, 1.f, 1.f, 1.f),
	mMetallic(1.f),
	mRoughness(1.f),
	mEmissive(0.f, 0.f, 0.f),
	mAlphaMode(ALPHA_MODE_OPAQUE),
	mAlphaCutoff(0.5f),
	mDoubleSided(false),
	mUnlit(false),
	mReady(false)
{
}

void LLGLTFMaterial::resetParsed()
{
	for (S32 i = 0; i < TEXTURE_COUNT; ++i)
	{
		mTextureId[i].setNull();
	}
	mBaseColor.set(1.f, 1.f, 1.f, 1.f);
	mMetallic = 1.f;
	mRoughness = 1.f;
	mEmissive.set(0.f, 0.f, 0.f);
	mAlphaMode = ALPHA_MODE_OPAQUE;
	mAlphaCutoff = 0.5f;
	mDoubleSided = false;
	mUnlit = false;
	mReady = false;
	mParseError.clear();
}

LLSD LLGLTFMaterial::asLLSD() const
{
	LLSD out;
	static const char* keys[TEXTURE_COUNT] = {
		"base_color", "normal", "metallic_roughness", "emissive"
	};
	for (S32 i = 0; i < TEXTURE_COUNT; ++i)
	{
		if (mTextureId[i].notNull())
		{
			out[keys[i]] = mTextureId[i];
		}
	}
	out["base_color_factor"] = mBaseColor.getValue();
	out["metallic_factor"] = mMetallic;
	out["roughness_factor"] = mRoughness;
	out["emissive_factor"] = mEmissive.getValue();
	out["alpha_mode"] = (S32)mAlphaMode;
	out["alpha_cutoff"] = mAlphaCutoff;
	out["double_sided"] = mDoubleSided;
	out["unlit"] = mUnlit;
	return out;
}

void LLGLTFMaterial::fromLLSD(const LLSD& data)
{
	static const char* keys[TEXTURE_COUNT] = {
		"base_color", "normal", "metallic_roughness", "emissive"
	};
	for (S32 i = 0; i < TEXTURE_COUNT; ++i)
	{
		if (data.has(keys[i]))
		{
			mTextureId[i] = data[keys[i]].asUUID();
		}
	}
	if (data.has("base_color_factor"))
	{
		mBaseColor.setValue(data["base_color_factor"]);
	}
	if (data.has("metallic_factor"))
	{
		mMetallic = (F32)data["metallic_factor"].asReal();
	}
	if (data.has("roughness_factor"))
	{
		mRoughness = (F32)data["roughness_factor"].asReal();
	}
	if (data.has("emissive_factor"))
	{
		mEmissive.setValue(data["emissive_factor"]);
	}
	if (data.has("alpha_mode"))
	{
		mAlphaMode = (AlphaMode)data["alpha_mode"].asInteger();
	}
	if (data.has("alpha_cutoff"))
	{
		mAlphaCutoff = (F32)data["alpha_cutoff"].asReal();
	}
	if (data.has("double_sided"))
	{
		mDoubleSided = data["double_sided"].asBoolean();
	}
	if (data.has("unlit"))
	{
		mUnlit = data["unlit"].asBoolean();
	}
}

bool LLGLTFMaterial::fromJsonText(const std::string& json)
{
	nlohmann::json model;
	try
	{
		model = nlohmann::json::parse(json);
	}
	catch (const std::exception& e)
	{
		mParseError = std::string("json excecao ") + e.what();
		if (mParseError.size() > 80)
		{
			mParseError.resize(80);
		}
		return false;
	}
	if (model.is_object() && model.contains("version") && model.contains("type") && model.contains("data") && model["data"].is_string())
	{
		if (!acceptedMaterialVersion(model["version"].get<std::string>())
			|| model["type"].get<std::string>() != "GLTF 2.0")
		{
			mParseError = "envelope json recusado";
			return false;
		}
		return fromJsonText(model["data"].get<std::string>());
	}
	if (!applyGltfJson(*this, model))
	{
		if (mParseError.empty())
		{
			mParseError = "gltf json recusado";
		}
		return false;
	}
	mReady = true;
	return true;
}

bool LLGLTFMaterial::fromBytes(const U8* data, S32 size)
{
	const LLUUID asset_id = mAssetId;
	resetParsed();
	mAssetId = asset_id;
	if (!data || size <= 0)
	{
		mParseError = "bytes vazios";
		return false;
	}
	std::string text(reinterpret_cast<const char*>(data), (size_t)size);
	size_t start = 0;
	if (text.size() >= 3 && (U8)text[0] == 0xEF && (U8)text[1] == 0xBB && (U8)text[2] == 0xBF)
	{
		start = 3;
	}
	while (start < text.size() && (text[start] == ' ' || text[start] == '\n' || text[start] == '\r' || text[start] == '\t'))
	{
		++start;
	}
	if (start < text.size() && text[start] == '{')
	{
		if (fromJsonText(start ? text.substr(start) : text))
		{
			return true;
		}
		resetParsed();
		mAssetId = asset_id;
	}
	std::istringstream stream(text);
	LLSD llsd;
	if (!LLSDSerialize::deserialize(llsd, stream, size))
	{
		mParseError = "llsd invalido";
		return false;
	}
	if (llsd.isMap() && llsd.has("version") && llsd.has("type") && llsd.has("data") && llsd["data"].isString())
	{
		if (!acceptedMaterialVersion(llsd["version"].asString())
			|| llsd["type"].asString() != "GLTF 2.0")
		{
			mParseError = "versao=" + llsd["version"].asString() + " tipo=" + llsd["type"].asString();
			return false;
		}
		if (!fromJsonText(llsd["data"].asString()))
		{
			if (mParseError.empty())
			{
				mParseError = "gltf json recusado";
			}
			return false;
		}
		return true;
	}
	fromLLSD(llsd);
	mReady = true;
	return true;
}
