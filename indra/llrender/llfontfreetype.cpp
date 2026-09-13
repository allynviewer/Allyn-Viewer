/**
 * @file llfontfreetype.cpp
 * @brief Freetype font library wrapper
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "linden_common.h"
#include "llfontfreetype.h"
#include "llfontgl.h"
#include <ft2build.h>
#ifdef LL_WINDOWS
#include <freetype2\freetype\ftsystem.h>
#endif
#include "llfontfreetypesvg.h"
#ifdef FT_FREETYPE_H
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#endif
#include "lldir.h"
#include "llerror.h"
#include "llimage.h"
#include "llimagepng.h"
#include "llmath.h"
#include "llstring.h"
#include "llfontbitmapcache.h"
#include "llgl.h"
#define ENABLE_OT_SVG_SUPPORT
FT_Render_Mode gFontRenderMode = FT_RENDER_MODE_NORMAL;
bool LLFontFreetype::sOpenGLcrashOnRestart = false;
LLFontManager *gFontManagerp = nullptr;
FT_Library gFTLibrary = nullptr;
void LLFontManager::initClass()
{
    if (!gFontManagerp)
    {
        gFontManagerp = new LLFontManager;
    }
}
void LLFontManager::cleanupClass()
{
    delete gFontManagerp;
    gFontManagerp = nullptr;
}
LLFontManager::LLFontManager()
{
    int error;
    error = FT_Init_FreeType(&gFTLibrary);
    if (error)
    {
        LL_ERRS() << "Freetype initialization failure!" << LL_ENDL;
        FT_Done_FreeType(gFTLibrary);
    }
#ifdef ENABLE_OT_SVG_SUPPORT
    SVG_RendererHooks hooks = {
        LLFontFreeTypeSvgRenderer::OnInit,
        LLFontFreeTypeSvgRenderer::OnFree,
        LLFontFreeTypeSvgRenderer::OnRender,
        LLFontFreeTypeSvgRenderer::OnPresetGlypthSlot,
    };
    FT_Property_Set(gFTLibrary, "ot-svg", "svg-hooks", &hooks);
#endif
}
LLFontManager::~LLFontManager()
{
    FT_Done_FreeType(gFTLibrary);
    unloadAllFonts();
}
LLFontGlyphInfo::LLFontGlyphInfo(U32 index, EFontGlyphType glyph_type)
:   mGlyphIndex(index),
    mGlyphType(glyph_type),
    mChar(0),
    mWidth(0),
    mHeight(0),
    mXAdvance(0.f),
    mYAdvance(0.f),
    mXBitmapOffset(0),
    mYBitmapOffset(0),
    mXBearing(0),
    mYBearing(0),
    mLsbDelta(0),
    mRsbDelta(0),
    mBitmapEntry(std::make_pair(EFontGlyphType::Unspecified, -1))
{
}
LLFontGlyphInfo::LLFontGlyphInfo(const LLFontGlyphInfo& fgi)
    : mGlyphIndex(fgi.mGlyphIndex)
    , mGlyphType(fgi.mGlyphType)
    , mChar(fgi.mChar)
    , mWidth(fgi.mWidth)
    , mHeight(fgi.mHeight)
    , mXAdvance(fgi.mXAdvance)
    , mYAdvance(fgi.mYAdvance)
    , mXBitmapOffset(fgi.mXBitmapOffset)
    , mYBitmapOffset(fgi.mYBitmapOffset)
    , mXBearing(fgi.mXBearing)
    , mYBearing(fgi.mYBearing)
    , mLsbDelta(fgi.mLsbDelta)
    , mRsbDelta(fgi.mRsbDelta)
{
    mBitmapEntry = fgi.mBitmapEntry;
}
LLFontFreetype::LLFontFreetype()
:   mFontBitmapCachep(new LLFontBitmapCache),
    mAscender(0.f),
    mDescender(0.f),
    mLineHeight(0.f),
    mIsFallback(false),
    mHinting(EFontHinting::FORCE_AUTOHINT),
    mFTFace(nullptr),
    mRenderGlyphCount(0),
    mStyle(0),
    mPointSize(0),
    mMaxDigitWidth(0.0f)
{
}
LLFontFreetype::~LLFontFreetype()
{
    if (mFTFace)
        FT_Done_Face(mFTFace);
    mFTFace = nullptr;
    std::for_each(mCharGlyphInfoMap.begin(), mCharGlyphInfoMap.end(), DeletePairedPointer());
    mCharGlyphInfoMap.clear();
    delete mFontBitmapCachep;
}
bool LLFontFreetype::loadFace(const std::string& filename, F32 point_size, F32 vert_dpi, F32 horz_dpi, S32 weight, bool is_fallback, S32 face_n, EFontHinting hinting, S32 flags)
{
    if (mFTFace)
    {
        FT_Done_Face(mFTFace);
        mFTFace = nullptr;
    }
    FT_Open_Args openArgs;
    memset( &openArgs, 0, sizeof( openArgs ) );
    openArgs.memory_base = gFontManagerp->loadFont( filename, openArgs.memory_size );
    if( !openArgs.memory_base )
        return false;
    openArgs.flags = FT_OPEN_MEMORY;
    int error = FT_Open_Face( gFTLibrary, &openArgs, 0, &mFTFace );
    if (error)
        return false;
    mIsFallback = is_fallback;
    mHinting = hinting;
    mFontFlags = flags;
    mWeight = weight;
    bool variable_font = false;
    if (weight >= 0)
    {
        variable_font = setVariationAxis("wght", static_cast<F32>(weight));
        setVariationAxis("opsz", point_size);
    }
    F32 pixels_per_em = (point_size / 72.f)*vert_dpi;
    error = FT_Set_Char_Size(mFTFace,
                            0,
                            (S32)(point_size*64),
                            (U32)horz_dpi,
                            (U32)vert_dpi);
    if (error)
    {
        FT_Done_Face(mFTFace);
        mFTFace = nullptr;
        return false;
    }
    F32 y_max, y_min, x_max, x_min;
    F32 ems_per_unit = 1.f/ mFTFace->units_per_EM;
    F32 pixels_per_unit = pixels_per_em * ems_per_unit;
    y_max = mFTFace->bbox.yMax * pixels_per_unit;
    y_min = mFTFace->bbox.yMin * pixels_per_unit;
    x_max = mFTFace->bbox.xMax * pixels_per_unit;
    x_min = mFTFace->bbox.xMin * pixels_per_unit;
    mAscender = mFTFace->ascender * pixels_per_unit;
    mDescender = -mFTFace->descender * pixels_per_unit;
    mLineHeight = mFTFace->height * pixels_per_unit;
    S32 max_char_width = ll_round(0.5f + (x_max - x_min));
    S32 max_char_height = ll_round(0.5f + (y_max - y_min));
    mFontBitmapCachep->init(max_char_width, max_char_height);
    if (!mFTFace->charmap)
    {
        FT_Set_Charmap(mFTFace, mFTFace->charmaps[0]);
    }
    if (!mIsFallback || !sOpenGLcrashOnRestart)
    {
        addGlyphFromFont(this, 0, 0, EFontGlyphType::Grayscale);
    }
    mName = filename;
    mPointSize = point_size;
    mStyle = LLFontGL::NORMAL;
    if(mFTFace->style_flags & FT_STYLE_FLAG_BOLD)
    {
        mStyle |= LLFontGL::BOLD;
    }
    else if (flags & LLFontGL::BOLD)
    {
        mStyle |= LLFontGL::BOLD;
    }
    else if (weight >= 600 && variable_font)
    {
        mStyle |= LLFontGL::BOLD;
    }
    if(mFTFace->style_flags & FT_STYLE_FLAG_ITALIC)
    {
        mStyle |= LLFontGL::ITALIC;
    }
    return true;
}
S32 LLFontFreetype::getNumFaces(const std::string& filename)
{
    if (mFTFace)
    {
        FT_Done_Face(mFTFace);
        mFTFace = nullptr;
    }
    S32 num_faces = 1;
    FT_Open_Args openArgs;
    memset( &openArgs, 0, sizeof( openArgs ) );
    openArgs.memory_base = gFontManagerp->loadFont( filename, openArgs.memory_size );
    if( !openArgs.memory_base )
        return 0;
    openArgs.flags = FT_OPEN_MEMORY;
    int error = FT_Open_Face( gFTLibrary, &openArgs, 0, &mFTFace );
    if (error)
        return 0;
    else
        num_faces = mFTFace->num_faces;
    FT_Done_Face(mFTFace);
    mFTFace = nullptr;
    return num_faces;
}
void LLFontFreetype::addFallbackFont(const LLPointer<LLFontFreetype>& fallback_font,
                                     const char_functor_t& functor)
{
    mFallbackFonts.emplace_back(fallback_font, functor);
}
F32 LLFontFreetype::getLineHeight() const
{
    return mLineHeight;
}
F32 LLFontFreetype::getAscenderHeight() const
{
    return mAscender;
}
F32 LLFontFreetype::getDescenderHeight() const
{
    return mDescender;
}
F32 LLFontFreetype::getXAdvance(llwchar wch) const
{
    if (mFTFace == nullptr)
        return 0.0;
    LLFontGlyphInfo* gi = getGlyphInfo(wch, EFontGlyphType::Unspecified);
    if (gi)
    {
        if (wch >= '0' && wch <= '9' && mMaxDigitWidth > 0.0f)
        {
            return mMaxDigitWidth;
        }
        return gi->mXAdvance;
    }
    else
    {
        char_glyph_info_map_t::iterator found_it = mCharGlyphInfoMap.find((llwchar)0);
        if (found_it != mCharGlyphInfoMap.end())
        {
            return found_it->second->mXAdvance;
        }
    }
    return (F32)mFontBitmapCachep->getMaxCharWidth();
}
F32 LLFontFreetype::getXAdvance(const LLFontGlyphInfo* glyph) const
{
    if (mFTFace == nullptr)
        return 0.0;
    if (mWeight > 0 && glyph->mChar >= '0' && glyph->mChar <= '9' && mMaxDigitWidth > 0.0f)
    {
        return mMaxDigitWidth;
    }
    return glyph->mXAdvance;
}
F32 LLFontFreetype::getXKerning(llwchar char_left, llwchar char_right) const
{
    if (mFTFace == nullptr)
        return 0.0;
    LLFontGlyphInfo* left_glyph_info = getGlyphInfo(char_left, EFontGlyphType::Unspecified);;
    LLFontGlyphInfo* right_glyph_info = getGlyphInfo(char_right, EFontGlyphType::Unspecified);
    return getXKerning(left_glyph_info, right_glyph_info);
}
F32 LLFontFreetype::getXKerning(const LLFontGlyphInfo* left_glyph_info, const LLFontGlyphInfo* right_glyph_info) const
{
    if (mFTFace == nullptr)
        return 0.0;
    U32 left_glyph = 0;
    U32 right_glyph = 0;
    if (left_glyph_info)
    {
        if (mWeight > 0 && left_glyph_info->mChar >= '0' && left_glyph_info->mChar <= '9')
        {
            return 0.0;
        }
        left_glyph = left_glyph_info->mGlyphIndex;
    }
    if (right_glyph_info)
    {
        if (mWeight > 0 && right_glyph_info->mChar >= '0' && right_glyph_info->mChar <= '9')
        {
            return 0.0;
        }
        right_glyph = right_glyph_info->mGlyphIndex;
    }
    FT_Vector  delta;
    llverify(!FT_Get_Kerning(mFTFace, left_glyph, right_glyph, FT_KERNING_UNFITTED, &delta));
    F32 delta_correction = 0.0f;
    if (left_glyph_info && right_glyph_info)
    {
        S32 delta_diff = left_glyph_info->mRsbDelta - right_glyph_info->mLsbDelta;
        if (delta_diff > 32)
            delta_correction = -1.0f;
        else if (delta_diff < -31)
            delta_correction = 1.0f;
    }
    return (F32)(delta.x * (1.f / 64.f)) + delta_correction;
}
bool LLFontFreetype::hasGlyph(llwchar wch) const
{
    llassert(!mIsFallback);
    return(mCharGlyphInfoMap.find(wch) != mCharGlyphInfoMap.end());
}
LLFontGlyphInfo* LLFontFreetype::addGlyph(llwchar wch, EFontGlyphType glyph_type) const
{
    if (!mFTFace)
    {
        return nullptr;
    }
    llassert(!mIsFallback);
    llassert(glyph_type < EFontGlyphType::Count);
    FT_UInt glyph_index = FT_Get_Char_Index(mFTFace, wch);
    if (glyph_index == 0)
    {
        size_t count = mFallbackFonts.size();
        if (LLStringOps::isEmoji(wch))
        {
            for (size_t i = 0; i < count; ++i)
            {
                const fallback_font_t& pair = mFallbackFonts[i];
                if (!pair.second || !pair.second(wch))
                {
                    continue;
                }
                glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
                if (glyph_index)
                {
                    return addGlyphFromFont(pair.first, wch, glyph_index,
                                            glyph_type);
                }
            }
        }
        std::vector<size_t> emoji_fonts_idx;
        for (size_t i = 0; i < count; ++i)
        {
            const fallback_font_t& pair = mFallbackFonts[i];
            if (pair.second)
            {
                emoji_fonts_idx.push_back(i);
                continue;
            }
            glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
            if (glyph_index)
            {
                return addGlyphFromFont(pair.first, wch, glyph_index,
                                        glyph_type);
            }
        }
        for (size_t j = 0, count2 = emoji_fonts_idx.size(); j < count2; ++j)
        {
            const fallback_font_t& pair = mFallbackFonts[emoji_fonts_idx[j]];
            glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
            if (glyph_index)
            {
                return addGlyphFromFont(pair.first, wch, glyph_index,
                                        glyph_type);
            }
        }
    }
    auto range_it = mCharGlyphInfoMap.equal_range(wch);
    char_glyph_info_map_t::iterator iter =
        std::find_if(range_it.first, range_it.second,
                     [&glyph_type](const char_glyph_info_map_t::value_type& entry)
                     {
                        return entry.second->mGlyphType == glyph_type;
                     });
    if (iter == range_it.second)
    {
        return addGlyphFromFont(this, wch, glyph_index, glyph_type);
    }
    return nullptr;
}
LLFontGlyphInfo* LLFontFreetype::addGlyphFromFont(const LLFontFreetype *fontp, llwchar wch, U32 glyph_index, EFontGlyphType requested_glyph_type) const
{
    LL_PROFILE_ZONE_SCOPED;
    if (mFTFace == nullptr)
        return nullptr;
    llassert(!mIsFallback);
    fontp->renderGlyph(requested_glyph_type, glyph_index, wch);
    EFontGlyphType bitmap_glyph_type = EFontGlyphType::Unspecified;
    switch (fontp->mFTFace->glyph->bitmap.pixel_mode)
    {
        case FT_PIXEL_MODE_MONO:
        case FT_PIXEL_MODE_GRAY:
            bitmap_glyph_type = EFontGlyphType::Grayscale;
            break;
        case FT_PIXEL_MODE_BGRA:
            bitmap_glyph_type = EFontGlyphType::Color;
            break;
        default:
            llassert_always(true);
            break;
    }
    S32 width = fontp->mFTFace->glyph->bitmap.width;
    S32 height = fontp->mFTFace->glyph->bitmap.rows;
    S32 pos_x, pos_y;
    U32 bitmap_num;
    mFontBitmapCachep->nextOpenPos(width, pos_x, pos_y, bitmap_glyph_type, bitmap_num);
    LLFontGlyphInfo* gi = new LLFontGlyphInfo(glyph_index, requested_glyph_type);
    gi->mChar = wch;
    gi->mXBitmapOffset = pos_x;
    gi->mYBitmapOffset = pos_y;
    gi->mBitmapEntry = std::make_pair(bitmap_glyph_type, bitmap_num);
    gi->mWidth = width;
    gi->mHeight = height;
    gi->mXBearing = fontp->mFTFace->glyph->bitmap_left;
    gi->mYBearing = fontp->mFTFace->glyph->bitmap_top;
    gi->mLsbDelta = (S32)fontp->mFTFace->glyph->lsb_delta;
    gi->mRsbDelta = (S32)fontp->mFTFace->glyph->rsb_delta;
    gi->mXAdvance = fontp->mFTFace->glyph->advance.x / 64.f;
    gi->mYAdvance = fontp->mFTFace->glyph->advance.y / 64.f;
    if (mWeight > 0 && wch >= '0' && wch <= '9')
    {
        mMaxDigitWidth = llmax(mMaxDigitWidth, gi->mXAdvance);
    }
    insertGlyphInfo(wch, gi);
    if (requested_glyph_type != bitmap_glyph_type)
    {
        LLFontGlyphInfo* gi_temp = new LLFontGlyphInfo(*gi);
        gi_temp->mGlyphType = bitmap_glyph_type;
        insertGlyphInfo(wch, gi_temp);
    }
    if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO
        || fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
    {
        U8 *buffer_data = fontp->mFTFace->glyph->bitmap.buffer;
        S32 buffer_row_stride = fontp->mFTFace->glyph->bitmap.pitch;
        U8 *tmp_graydata = nullptr;
        if (fontp->mFTFace->glyph->bitmap.pixel_mode
            == FT_PIXEL_MODE_MONO)
        {
            tmp_graydata = new U8[width * height];
            S32 xpos, ypos;
            for (ypos = 0; ypos < height; ++ypos)
            {
                S32 bm_row_offset = buffer_row_stride * ypos;
                for (xpos = 0; xpos < width; ++xpos)
                {
                    U32 bm_col_offsetbyte = xpos / 8;
                    U32 bm_col_offsetbit = 7 - (xpos % 8);
                    U32 bit =
                    !!(buffer_data[bm_row_offset
                               + bm_col_offsetbyte
                       ] & (1 << bm_col_offsetbit) );
                    tmp_graydata[width*ypos + xpos] =
                        255 * bit;
                }
            }
            buffer_data = tmp_graydata;
            buffer_row_stride = width;
        }
        setSubImageLuminanceAlpha(pos_x,
                                    pos_y,
                                    bitmap_num,
                                    width,
                                    height,
                                    buffer_data,
                                    buffer_row_stride);
        if (tmp_graydata)
            delete[] tmp_graydata;
    }
    else if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA)
    {
        setSubImageBGRA(pos_x,
                        pos_y,
                        bitmap_num,
                        fontp->mFTFace->glyph->bitmap.width,
                        fontp->mFTFace->glyph->bitmap.rows,
                        fontp->mFTFace->glyph->bitmap.buffer,
                        llabs(fontp->mFTFace->glyph->bitmap.pitch));
    } else {
        llassert(false);
    }
    LLImageGL *image_gl = mFontBitmapCachep->getImageGL(bitmap_glyph_type, bitmap_num);
    LLImageRaw *image_raw = mFontBitmapCachep->getImageRaw(bitmap_glyph_type, bitmap_num);
    if (image_gl && image_raw)
    {
        image_gl->setSubImage(image_raw, 0, 0, image_gl->getWidth(), image_gl->getHeight());
    }
    else
    {
        llassert(false);
    }
    return gi;
}
LLFontGlyphInfo* LLFontFreetype::getGlyphInfo(llwchar wch, EFontGlyphType glyph_type) const
{
    std::pair<char_glyph_info_map_t::iterator, char_glyph_info_map_t::iterator> range_it = mCharGlyphInfoMap.equal_range(wch);
    char_glyph_info_map_t::iterator iter = (EFontGlyphType::Unspecified != glyph_type)
        ? std::find_if(range_it.first, range_it.second, [&glyph_type](const char_glyph_info_map_t::value_type& entry) { return entry.second->mGlyphType == glyph_type; })
        : range_it.first;
    if (iter != range_it.second)
    {
        return iter->second;
    }
    else
    {
        return addGlyph(wch, (EFontGlyphType::Unspecified != glyph_type) ? glyph_type : EFontGlyphType::Grayscale);
    }
}
void LLFontFreetype::insertGlyphInfo(llwchar wch, LLFontGlyphInfo* gi) const
{
    llassert(gi->mGlyphType < EFontGlyphType::Count);
    std::pair<char_glyph_info_map_t::iterator, char_glyph_info_map_t::iterator> range_it = mCharGlyphInfoMap.equal_range(wch);
    char_glyph_info_map_t::iterator iter =
        std::find_if(range_it.first, range_it.second, [&gi](const char_glyph_info_map_t::value_type& entry) { return entry.second->mGlyphType == gi->mGlyphType; });
    if (iter != range_it.second)
    {
        delete iter->second;
        iter->second = gi;
    }
    else
    {
        mCharGlyphInfoMap.insert(std::make_pair(wch, gi));
    }
}
void LLFontFreetype::renderGlyph(EFontGlyphType bitmap_type, U32 glyph_index, llwchar wch) const
{
    if (mFTFace == nullptr)
        return;
    FT_Int32 load_flags = (FT_Int32)mHinting;
    if (EFontGlyphType::Color == bitmap_type)
    {
        load_flags |= FT_LOAD_COLOR;
    }
    FT_Error error = FT_Load_Glyph(mFTFace, glyph_index, load_flags);
    if (FT_Err_Ok != error)
    {
        if (error == FT_Err_Out_Of_Memory)
        {
            LL_WARNS() << "Out of memory loading glyph for character " << llformat("U+%xu", U32(wch)) << LL_ENDL;
            LL_ERRS() << "Out of memory loading glyph for character " << llformat("U+%xu", U32(wch)) << LL_ENDL;
        }
        std::string message = llformat(
            "Error %d (%s) loading wchar %u glyph %u/%u: bitmap_type=%u, load_flags=%d",
            error, FT_Error_String(error), wch, glyph_index, mFTFace->num_glyphs, bitmap_type, load_flags);
        LL_WARNS_ONCE() << message << LL_ENDL;
        error = FT_Load_Glyph(mFTFace, glyph_index, load_flags ^ FT_LOAD_COLOR);
        if (FT_Err_Invalid_Outline == error
            || FT_Err_Invalid_Composite == error
            || (FT_Err_Ok != error && LLStringOps::isEmoji(wch)))
        {
            error = FT_Load_Glyph(mFTFace, 0, FT_LOAD_FORCE_AUTOHINT);
            if (FT_Err_Ok != error)
            {
                LL_ERRS() << "Loading fallback for char '" << (U32)wch << "', glyph " << glyph_index << " failed with error : " << (S32)error << LL_ENDL;
            }
        }
        llassert_always_msg(FT_Err_Ok == error, message.c_str());
    }
    if (!mFTFace->glyph)
    {
        LL_ERRS() << "FT_Load_Glyph succeeded but glyph slot is null for wchar " << llformat("U+%xu", U32(wch)) << LL_ENDL;
        return;
    }
    if (!mFTFace->glyph->bitmap.buffer)
    {
        error = FT_Render_Glyph(mFTFace->glyph, gFontRenderMode);
        if (error != FT_Err_Ok)
        {
            std::string render_message = llformat(
                "Error %d (%s) rendering wchar %u glyph %u: format=%lu, pixel_mode=%d, render_mode=%d",
                error, FT_Error_String(error), wch, glyph_index,
                (unsigned long)mFTFace->glyph->format, mFTFace->glyph->bitmap.pixel_mode, gFontRenderMode);
            if (gFontRenderMode != FT_RENDER_MODE_NORMAL)
            {
                LL_WARNS_ONCE() << render_message << LL_ENDL;
                error = FT_Render_Glyph(mFTFace->glyph, FT_RENDER_MODE_NORMAL);
                if (error != FT_Err_Ok)
                {
                    LL_ERRS() << "Fallback to FT_RENDER_MODE_NORMAL failed. " << render_message << LL_ENDL;
                }
            }
            else
            {
                LL_ERRS() << render_message << LL_ENDL;
            }
        }
    }
    mRenderGlyphCount++;
}
void LLFontFreetype::reset(F32 vert_dpi, F32 horz_dpi)
{
    resetBitmapCache();
    loadFace(mName, mPointSize, vert_dpi ,horz_dpi, mWeight, mIsFallback, 0, mHinting, mFontFlags);
    if (!mIsFallback)
    {
        if (mFallbackFonts.empty())
        {
            LL_WARNS() << "LLFontGL::reset(), no fallback fonts present" << LL_ENDL;
        }
        else
        {
            for (fallback_font_vector_t::iterator it = mFallbackFonts.begin(); it != mFallbackFonts.end(); ++it)
            {
                it->first->reset(vert_dpi, horz_dpi);
            }
        }
    }
}
void LLFontFreetype::resetBitmapCache()
{
    for (char_glyph_info_map_t::iterator it = mCharGlyphInfoMap.begin(), end_it = mCharGlyphInfoMap.end();
        it != end_it;
        ++it)
    {
        delete it->second;
    }
    mCharGlyphInfoMap.clear();
    mFontBitmapCachep->reset();
    mMaxDigitWidth = 0.0f;
    if (!mIsFallback || !sOpenGLcrashOnRestart)
    {
        addGlyphFromFont(this, 0, 0, EFontGlyphType::Grayscale);
    }
}
void LLFontFreetype::destroyGL()
{
    mFontBitmapCachep->destroyGL();
}
const std::string &LLFontFreetype::getName() const
{
    return mName;
}
static void dumpFontBitmap(const LLImageRaw* image_raw, const std::string& file_name)
{
    LLPointer<LLImagePNG> tmpImage = new LLImagePNG();
    if ( (tmpImage->encode(image_raw, 0.0f)) && (tmpImage->save(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, file_name))) )
    {
        LL_INFOS("Font") << "Successfully saved " << file_name << LL_ENDL;
    }
    else
    {
        LL_WARNS("Font") << "Failed to save " << file_name << LL_ENDL;
    }
}
void LLFontFreetype::dumpFontBitmaps() const
{
    for (int idx = 0, cnt = mFontBitmapCachep->getNumBitmaps(EFontGlyphType::Grayscale); idx < cnt; idx++)
    {
        dumpFontBitmap(mFontBitmapCachep->getImageRaw(EFontGlyphType::Grayscale, idx), llformat("%s_%d_%d_%d.png", mFTFace->family_name, (int)(mPointSize * 10), mStyle, idx));
    }
    for (int idx = 0, cnt = mFontBitmapCachep->getNumBitmaps(EFontGlyphType::Color); idx < cnt; idx++)
    {
        dumpFontBitmap(mFontBitmapCachep->getImageRaw(EFontGlyphType::Color, idx), llformat("%s_%d_%d_%d_clr.png", mFTFace->family_name, (int)(mPointSize * 10), mStyle, idx));
    }
}
const LLFontBitmapCache* LLFontFreetype::getFontBitmapCache() const
{
    return mFontBitmapCachep;
}
void LLFontFreetype::setStyle(U8 style)
{
    mStyle = style;
}
U8 LLFontFreetype::getStyle() const
{
    return mStyle;
}
bool LLFontFreetype::setSubImageBGRA(U32 x, U32 y, U32 bitmap_num, U16 width, U16 height, const U8* data, U32 stride) const
{
    LLImageRaw* image_raw = mFontBitmapCachep->getImageRaw(EFontGlyphType::Color, bitmap_num);
    llassert(!mIsFallback);
    if (!image_raw)
    {
        llassert(false);
        return false;
    }
    llassert(image_raw->getComponents() == 4);
    U32* image_data = (U32*)image_raw->getData();
    if (!image_data)
    {
        return false;
    }
    for (U32 idxRow = 0; idxRow < height; idxRow++)
    {
        const U32 nSrcRow = height - 1 - idxRow;
        const U32 nSrcOffset = nSrcRow * width * image_raw->getComponents();
        const U32 nDstOffset = (y + idxRow) * image_raw->getWidth() + x;
        for (U32 idxCol = 0; idxCol < width; idxCol++)
        {
            U32 nTemp = nSrcOffset + idxCol * 4;
            image_data[nDstOffset + idxCol] = data[nTemp + 3] << 24 | data[nTemp] << 16 | data[nTemp + 1] << 8 | data[nTemp + 2];
        }
    }
    return true;
}
void LLFontFreetype::setSubImageLuminanceAlpha(U32 x, U32 y, U32 bitmap_num, U32 width, U32 height, U8 *data, S32 stride) const
{
    LLImageRaw *image_raw = mFontBitmapCachep->getImageRaw(EFontGlyphType::Grayscale, bitmap_num);
    llassert(!mIsFallback);
    if (!image_raw)
    {
        llassert(false);
        return;
    }
    llassert(image_raw->getComponents() == 2);
    U8 *target = image_raw->getData();
    llassert(target);
    if (!data || !target)
    {
        return;
    }
    if (0 == stride)
        stride = width;
    U32 i, j;
    U32 to_offset;
    U32 from_offset;
    U32 target_width = image_raw->getWidth();
    for (i = 0; i < height; i++)
    {
        to_offset = (y + i)*target_width + x;
        from_offset = (height - 1 - i)*stride;
        for (j = 0; j < width; j++)
        {
            *(target + to_offset*2 + 1) = *(data + from_offset);
            to_offset++;
            from_offset++;
        }
    }
}
bool LLFontFreetype::setVariationAxis(const std::string& axis_tag, F32 value)
{
    if (!mFTFace)
        return false;
    FT_MM_Var* master = nullptr;
    if (FT_Get_MM_Var(mFTFace, &master) != 0)
    {
        return false;
    }
    FT_UInt axis_index = 0;
    bool found = false;
    for (FT_UInt i = 0; i < master->num_axis; i++)
    {
        if (master->axis[i].tag == FT_MAKE_TAG(axis_tag[0], axis_tag[1], axis_tag[2], axis_tag[3]))
        {
            axis_index = i;
            found = true;
            F32 min_val = master->axis[i].minimum / 65536.0f;
            F32 max_val = master->axis[i].maximum / 65536.0f;
            value = llclamp(value, min_val, max_val);
            break;
        }
    }
    if (!found)
    {
        FT_Done_MM_Var(gFTLibrary, master);
        LL_WARNS_ONCE("Font") << "Axis '" << axis_tag << "' not found in font: " << mName << LL_ENDL;
        return false;
    }
    FT_UInt num_coords = master->num_axis;
    FT_Fixed* coords = new FT_Fixed[num_coords];
    FT_Get_Var_Design_Coordinates(mFTFace, num_coords, coords);
    coords[axis_index] = (FT_Fixed)(value * 65536.0f);
    int error = FT_Set_Var_Design_Coordinates(mFTFace, num_coords, coords);
    delete[] coords;
    FT_Done_MM_Var(gFTLibrary, master);
    if (error != 0)
    {
        LL_WARNS() << "Failed to set variation coordinates for " << axis_tag
            << " = " << value << " in font: " << mName << LL_ENDL;
        return false;
    }
    LL_DEBUGS("Font") << "Set " << axis_tag << " = " << value
        << " for font: " << mName << LL_ENDL;
    return true;
}
namespace ll
{
    namespace fonts
    {
        class LoadedFont
        {
            public:
            LoadedFont( std::string aName , std::string const &aAddress, std::size_t aSize )
            : mAddress( aAddress )
            {
                mName = aName;
                mSize = aSize;
                mRefs = 1;
            }
            std::string mName;
            std::string mAddress;
            std::size_t mSize;
            U32  mRefs;
        };
    }
}
U8 const* LLFontManager::loadFont( std::string const &aFilename, long &a_Size)
{
    try
    {
        a_Size = 0;
        std::map< std::string, std::shared_ptr<ll::fonts::LoadedFont> >::iterator itr = m_LoadedFonts.find(aFilename);
        if (itr != m_LoadedFonts.end())
        {
            ++itr->second->mRefs;
            a_Size = static_cast<long>(itr->second->mSize);
            return reinterpret_cast<U8 const*>(itr->second->mAddress.c_str());
        }
        auto strContent = std::string();
        {
            llifstream ifs(aFilename.c_str(), llifstream::binary);
            if (!ifs.is_open())
            {
                return nullptr;
            }
            std::ostringstream oss;
            oss << ifs.rdbuf();
            strContent = oss.str();
        }
        if (strContent.empty())
            return nullptr;
        llassert_always(strContent.size() < std::numeric_limits<long>::max());
        a_Size = static_cast<long>(strContent.size());
        auto pCache = std::make_shared<ll::fonts::LoadedFont>(aFilename, strContent, a_Size);
        itr = m_LoadedFonts.insert(std::make_pair(aFilename, pCache)).first;
        return reinterpret_cast<U8 const*>(itr->second->mAddress.c_str());
    }
    catch (const std::bad_alloc&)
    {
        LL_WARNS() << "Failed to load font. Out of memory." << LL_ENDL;
        LL_ERRS() << "Failed to load font. Out of memory." << LL_ENDL;
    }
    return nullptr;
}
void LLFontManager::unloadAllFonts()
{
    m_LoadedFonts.clear();
}
