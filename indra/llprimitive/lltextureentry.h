/** 
 * @file lltextureentry.h
 * @brief LLTextureEntry base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
#ifndef LL_LLTEXTUREENTRY_H
#define LL_LLTEXTUREENTRY_H
#include "lluuid.h"
#include "v4color.h"
#include "llsd.h"
#include "llmaterialid.h"
#include "llmaterial.h"
const S32 TEM_CHANGE_NONE = 0x0;
const S32 TEM_CHANGE_COLOR = 0x1;
const S32 TEM_CHANGE_TEXTURE = 0x2;
const S32 TEM_CHANGE_MEDIA = 0x4;
const S32 TEM_INVALID = 0x8;
const S32 TEM_BUMPMAP_COUNT = 32;
const S32 TEM_BUMP_MASK 		= 0x1f;
const S32 TEM_FULLBRIGHT_MASK 	= 0x01;
const S32 TEM_SHINY_MASK 		= 0x03;
const S32 TEM_BUMP_SHINY_MASK 	= (0xc0 | 0x1f);
const S32 TEM_FULLBRIGHT_SHIFT 	= 5;
const S32 TEM_SHINY_SHIFT 		= 6;
const S32 TEM_MEDIA_MASK		= 0x01;
const S32 TEM_TEX_GEN_MASK		= 0x06;
const S32 TEM_TEX_GEN_SHIFT		= 1;
class LLMediaEntry;
class LLTextureEntry
{
public:
	static LLTextureEntry* newTextureEntry();
	typedef enum e_texgen
	{
		TEX_GEN_DEFAULT			= 0x00,
		TEX_GEN_PLANAR			= 0x02,
		TEX_GEN_SPHERICAL		= 0x04,
		TEX_GEN_CYLINDRICAL		= 0x06
	} eTexGen;
	LLTextureEntry();
	LLTextureEntry(const LLUUID& tex_id);
	LLTextureEntry(const LLTextureEntry &rhs);
	LLTextureEntry &operator=(const LLTextureEntry &rhs);
    virtual ~LLTextureEntry();
	bool operator==(const LLTextureEntry &rhs) const;
	bool operator!=(const LLTextureEntry &rhs) const;
	bool operator <(const LLTextureEntry &rhs) const;
	LLSD asLLSD() const;
	void asLLSD(LLSD& sd) const;
	operator LLSD() const { return asLLSD(); }
	bool fromLLSD(const LLSD& sd);
	virtual LLTextureEntry* newBlank() const;
	virtual LLTextureEntry* newCopy() const;
	void init(const LLUUID& tex_id, F32 scale_s, F32 scale_t, F32 offset_s, F32 offset_t, F32 rotation, U8 bump);
	bool hasPendingMaterialUpdate() const { return mMaterialUpdatePending; }
	bool isSelected() const { return mSelected; }
	bool setSelected(bool sel) { bool prev_sel = mSelected; mSelected = sel; return prev_sel; }
	S32  setID (const LLUUID &tex_id);
	S32  setColor(const LLColor4 &color);
	S32  setColor(const LLColor3 &color);
	S32  setAlpha(const F32 alpha);
	S32  setScale(F32 s, F32 t);
	S32  setScaleS(F32 s);
	S32  setScaleT(F32 t);
	S32  setOffset(F32 s, F32 t);
	S32  setOffsetS(F32 s);
	S32  setOffsetT(F32 t);
	S32  setRotation(F32 theta);
	S32  setBumpmap(U8 bump);
	S32  setFullbright(U8 bump);
	S32  setShiny(U8 bump);
	S32  setBumpShiny(U8 bump);
 	S32  setBumpShinyFullbright(U8 bump);
	S32  setMediaFlags(U8 media_flags);
	S32	 setTexGen(U8 texGen);
	S32  setMediaTexGen(U8 media);
    S32  setGlow(F32 glow);
	S32  setMaterialID(const LLMaterialID& pMaterialID);
	S32  setMaterialParams(const LLMaterialPtr pMaterialParams);
	virtual const LLUUID &getID() const { return mID; }
	const LLColor4 &getColor() const { return mColor; }
	void getScale(F32 *s, F32 *t) const { *s = mScaleS; *t = mScaleT; }
	F32  getScaleS() const { return mScaleS; }
	F32  getScaleT() const { return mScaleT; }
	void getOffset(F32 *s, F32 *t) const { *s = mOffsetS; *t = mOffsetT; }
	F32  getOffsetS() const { return mOffsetS; }
	F32  getOffsetT() const { return mOffsetT; }
	F32  getRotation() const { return mRotation; }
	void getRotation(F32 *theta) const { *theta = mRotation; }
	U8	 getBumpmap() const { return mBump & TEM_BUMP_MASK; }
	U8	 getFullbright() const { return (mBump>>TEM_FULLBRIGHT_SHIFT) & TEM_FULLBRIGHT_MASK; }
	U8	 getShiny() const { return (mBump>>TEM_SHINY_SHIFT) & TEM_SHINY_MASK; }
	U8	 getBumpShiny() const { return mBump & TEM_BUMP_SHINY_MASK; }
 	U8	 getBumpShinyFullbright() const { return mBump; }
	U8	 getMediaFlags() const { return mMediaFlags & TEM_MEDIA_MASK; }
	LLTextureEntry::e_texgen	 getTexGen() const	{ return LLTextureEntry::e_texgen(mMediaFlags & TEM_TEX_GEN_MASK); }
	U8	 getMediaTexGen() const { return mMediaFlags; }
    F32  getGlow() const { return mGlow; }
	const LLMaterialID& getMaterialID() const { return mMaterialID; };
	const LLMaterialPtr getMaterialParams() const { return mMaterial; };
	bool hasMedia() const { return (bool)(mMediaFlags & MF_HAS_MEDIA); }
	LLMediaEntry* getMediaData() const { return mMediaEntry; }
    void setMediaData(const LLMediaEntry &media_entry);
	bool updateMediaData(const LLSD& media_data);
    void clearMediaData();
    void mergeIntoMediaData(const LLSD& media_fields);
    static std::string touchMediaVersionString(const std::string &in_version, const LLUUID &agent_id);
    static U32 getVersionFromMediaVersionString(const std::string &version_string);
    static LLUUID getAgentIDFromMediaVersionString(const std::string &version_string);
	static bool isMediaVersionString(const std::string &version_string);
	enum { MF_NONE = 0x0, MF_HAS_MEDIA = 0x1 };
public:
	F32                 mScaleS;
	F32                 mScaleT;
	F32                 mOffsetS;
	F32                 mOffsetT;
	F32                 mRotation;
	static const LLTextureEntry null;
	static const char* OBJECT_ID_KEY;
	static const char* OBJECT_MEDIA_DATA_KEY;
    static const char* MEDIA_VERSION_KEY;
	static const char* TEXTURE_INDEX_KEY;
	static const char* TEXTURE_MEDIA_DATA_KEY;
protected:
	bool                mSelected;
	LLUUID				mID;
	LLColor4			mColor;
	U8					mBump;
	U8					mMediaFlags;
	F32                 mGlow;
	bool                mMaterialUpdatePending;
	LLMaterialID        mMaterialID;
	LLMaterialPtr		mMaterial;
	LLMediaEntry*		mMediaEntry;
};
#endif
