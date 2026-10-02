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
	LLGLTFMaterialList() {}
public:
	LLGLTFMaterial* getMaterial(const LLUUID& id);

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
};

#endif
