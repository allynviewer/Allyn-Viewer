#include "llviewerprecompiledheaders.h"
#include "llgltfmateriallist.h"
#include "llassetstorage.h"
#include "llframetimer.h"
#include "llvfile.h"
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
