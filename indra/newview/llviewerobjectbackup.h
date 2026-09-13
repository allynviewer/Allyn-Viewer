/** 
 * @file llviewerobjectbackup.h
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_LLVIEWEROBJECTBACKUP_H
#define LL_LLVIEWEROBJECTBACKUP_H
#include <deque>
#include <string>
#include <vector>
#include "boost/unordered_map.hpp"
#include "boost/unordered_set.hpp"
#include "llfloater.h"
#include "statemachine/aifilepicker.h"
#include "lluuid.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
class LLSelectNode;
class LLViewerObject;
enum export_states {
	EXPORT_INIT,
	EXPORT_CHECK_PERMS,
	EXPORT_FETCH_PHYSICS,
	EXPORT_STRUCTURE,
	EXPORT_TEXTURES,
	EXPORT_LLSD,
	EXPORT_DONE,
	EXPORT_FAILED,
	EXPORT_ABORTED
};
class LLObjectBackup : public LLFloater,
					   public LLFloaterSingleton<LLObjectBackup>
{
	friend class LLUISingleton<LLObjectBackup, VisibilityPolicy<LLFloater> >;
protected:
	LOG_CLASS(LLObjectBackup);
public:
	~LLObjectBackup();
	void onClose(bool app_quitting);
	void importObject(bool upload = false);
	void importObject_continued(AIFilePicker* filepicker);
	void exportObject();
	void exportObject_continued(AIFilePicker* filepicker);
	void updateMap(LLUUID uploaded_asset);
	void uploadNextAsset();
	static void exportWorker(void *userdata);
	static void primUpdate(LLViewerObject* object);
	static void newPrim(LLViewerObject* object);
	std::string getFolder() { return mFolder; }
	static void setDefaultTextures();
	static bool validatePerms(const LLPermissions* item_permissions);
	static bool validateTexturePerms(const LLUUID& asset_id);
	static bool validateNode(LLSelectNode* node);
private:
	LLObjectBackup(const LLSD&);
	void showFloater(bool exporting);
	static bool confirmCloseCallback(const LLSD& notification,
									 const LLSD& response);
	void updateImportNumbers();
	void updateExportNumbers();
	LLUUID validateTextureID(const LLUUID& asset_id);
	LLSD primsToLLSD(LLViewerObject::child_list_t child_list,
					 bool is_attachment);
	void importFirstObject();
	void importNextObject();
	void exportNextTexture();
	void xmlToPrim(LLSD prim_llsd, LLViewerObject* object);
	void rezAgentOffset(LLVector3 offset);
	LLVector3 offsetAgent(LLVector3 offset);
public:
	static const U32 TEXTURE_OK = 0x00;
	static const U32 TEXTURE_BAD_PERM = 0x01;
	static const U32 TEXTURE_MISSING = 0x02;
	static const U32 TEXTURE_BAD_ENCODING = 0x04;
	static const U32 TEXTURE_IS_NULL = 0x08;
	static const U32 TEXTURE_SAVED_FAILED = 0x10;
	enum export_states mExportState;
	U32 mNonExportedTextures;
	bool mGotExtraPhysics;
	bool mCheckNextTexture;
private:
	bool mRunning;
	bool mRetexture;
	U32 mObjects;
	U32 mCurObject;
	U32 mPrims;
	U32 mCurPrim;
	U32 mRezCount;
	LLVector3 mRootPos;
	LLQuaternion mRootRot;
	LLVector3 mRootRootPos;
	LLVector3 mGroupOffset;
	LLVector3 mAgentPos;
	LLQuaternion mAgentRot;
	LLUUID mCurrentAsset;
	LLUUID mExpectingUpdate;
	LLSD::map_const_iterator mPrimImportIter;
	LLSD::array_const_iterator mGroupPrimImportIter;
	std::string mFileName;
	std::string mFolder;
	typedef uuid_set_t textures_set_t;
	textures_set_t mTexturesList;
	textures_set_t mBadPermsTexturesList;
	boost::unordered_map<LLUUID, LLUUID> mAssetMap;
	std::vector<LLViewerObject*> mToSelect;
	std::vector<LLViewerObject*>::iterator mProcessIter;
	LLSD mLLSD;
	LLSD mThisGroup;
};
extern LLUUID LL_TEXTURE_PLYWOOD;
extern LLUUID LL_TEXTURE_BLANK;
extern LLUUID LL_TEXTURE_INVISIBLE;
extern LLUUID LL_TEXTURE_TRANSPARENT;
extern LLUUID LL_TEXTURE_MEDIA;
#endif
