#ifndef LL_LLGLTFMATERIAL_H
#define LL_LLGLTFMATERIAL_H

#include "llpointer.h"
#include "llrefcount.h"
#include "llsd.h"
#include "v3color.h"
#include "v4color.h"
#include "lluuid.h"
#include <string>

class LLGLTFMaterial : public LLRefCount
{
public:
	enum TextureInfo
	{
		BASE_COLOR = 0,
		NORMAL,
		METALLIC_ROUGHNESS,
		EMISSIVE,
		TEXTURE_COUNT
	};

	enum AlphaMode
	{
		ALPHA_MODE_OPAQUE = 0,
		ALPHA_MODE_BLEND,
		ALPHA_MODE_MASK
	};

	LLGLTFMaterial();

	LLSD asLLSD() const;
	void fromLLSD(const LLSD& data);
	bool fromBytes(const U8* data, S32 size);

	const LLUUID& getTextureId(TextureInfo slot) const { return mTextureId[slot]; }
	void setTextureId(TextureInfo slot, const LLUUID& id) { mTextureId[slot] = id; }

	LLUUID mAssetId;
	LLUUID mTextureId[TEXTURE_COUNT];
	LLColor4 mBaseColor;
	F32 mMetallic;
	F32 mRoughness;
	LLColor3 mEmissive;
	AlphaMode mAlphaMode;
	F32 mAlphaCutoff;
	bool mDoubleSided;
	bool mUnlit;
	bool mReady;
	std::string mParseError;

private:
	void resetParsed();
	bool fromJsonText(const std::string& json);
};

typedef LLPointer<LLGLTFMaterial> LLGLTFMaterialPtr;

#endif
