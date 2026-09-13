/** 
 * @file llprimtexturelist.h
 * @brief LLPrimTextureList (virtual) base class
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 * 
 */
#ifndef LL_LLPRIMTEXTURELIST_H
#define LL_LLPRIMTEXTURELIST_H
#include <vector>
#include "lluuid.h"
#include "v3color.h"
#include "v4color.h"
#include "llmaterial.h"
class LLTextureEntry;
class LLMaterialID;
typedef std::vector<LLTextureEntry*> texture_list_t;
class LLPrimTextureList
{
public:
	static LLTextureEntry* newTextureEntry();
	static void setNewTextureEntryCallback( LLTextureEntry* (*callback)() );
	static LLTextureEntry* (*sNewTextureEntryCallback)();
	LLPrimTextureList();
	virtual ~LLPrimTextureList();
	void clear();
	void copy(const LLPrimTextureList& other_list);
	void take(LLPrimTextureList& other_list);
	S32 copyTexture(const U8 index, const LLTextureEntry& te);
	S32 takeTexture(const U8 index, LLTextureEntry* te);
	LLTextureEntry* getTexture(const U8 index) const;
	S32 setID(const U8 index, const LLUUID& id);
	S32 setColor(const U8 index, const LLColor3& color);
	S32 setColor(const U8 index, const LLColor4& color);
	S32 setAlpha(const U8 index, const F32 alpha);
	S32 setScale(const U8 index, const F32 s, const F32 t);
	S32 setScaleS(const U8 index, const F32 s);
	S32 setScaleT(const U8 index, const F32 t);
	S32 setOffset(const U8 index, const F32 s, const F32 t);
	S32 setOffsetS(const U8 index, const F32 s);
	S32 setOffsetT(const U8 index, const F32 t);
	S32 setRotation(const U8 index, const F32 r);
	S32 setBumpShinyFullbright(const U8 index, const U8 bump);
	S32 setMediaTexGen(const U8 index, const U8 media);
	S32 setBumpMap(const U8 index, const U8 bump);
	S32 setBumpShiny(const U8 index, const U8 bump_shiny);
	S32 setTexGen(const U8 index, const U8 texgen);
	S32 setShiny(const U8 index, const U8 shiny);
	S32 setFullbright(const U8 index, const U8 t);
	S32 setMediaFlags(const U8 index, const U8 media_flags);
	S32 setGlow(const U8 index, const F32 glow);
	S32 setMaterialID(const U8 index, const LLMaterialID& pMaterialID);
	S32 setMaterialParams(const U8 index, const LLMaterialPtr pMaterialParams);
	LLMaterialPtr getMaterialParams(const U8 index);
	S32 size() const;
	void setSize(S32 new_size);
	void setAllIDs(const LLUUID& id);
protected:
	texture_list_t mEntryList;
private:
	LLPrimTextureList(const LLPrimTextureList& other_list)
	{
	}
};
#endif
