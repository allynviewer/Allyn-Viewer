#ifndef LL_LLGLTFMATERIALLIST_H
#define LL_LLGLTFMATERIALLIST_H

#include "llassetstorage.h"
#include "llgltfmaterial.h"
#include "llsingleton.h"
#include <map>

class LLGLTFMaterialList : public LLSingleton<LLGLTFMaterialList>
{
	friend class LLSingleton<LLGLTFMaterialList>;
protected:
	LLGLTFMaterialList() : mReadyGeneration(0) {}
public:
	LLGLTFMaterial* getMaterial(const LLUUID& id);
	LLGLTFMaterial* findMaterial(const LLUUID& id) const;
	S32 getReadyGeneration() const { return mReadyGeneration; }

private:
	struct Entry
	{
		LLGLTFMaterialPtr mat;
		bool pending;
		S32 tries;
		F64 next;
		Entry() : pending(false), tries(0), next(0.0) {}
	};

	static void onAsset(LLVFS* vfs, const LLUUID& asset_id, LLAssetType::EType asset_type, void* user_data, S32 status, LLExtStat ext_status);
	void onAssetArrived(LLVFS* vfs, const LLUUID& asset_id, S32 status);

	std::map<LLUUID, Entry> mEntries;
	S32 mReadyGeneration;
};

class LLGLSLShader;
class LLTextureEntry;
class LLColor4;
class LLFace;

class LLPBRGraphics
{
public:
	static void update();
	static void bindGlobals(LLGLSLShader* shader);
	static void bindFace(LLGLSLShader* shader, const LLTextureEntry* te);
	static void bindDraw(LLGLSLShader* shader, LLFace* face);
	static bool apply(const LLTextureEntry* te, LLColor4& spec, F32& env, F32& emissive);
	static bool gradeActive();
	static void bindGrade(LLGLSLShader& shader, bool grade, F32 scale_x, F32 scale_y);
};

#endif
