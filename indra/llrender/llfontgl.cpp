/**
 * @file llfontgl.cpp
 * @brief Wrapper around FreeType
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
#include "linden_common.h"
#include "llfontgl.h"
#include "llfasttimer.h"
#include "llfontfreetype.h"
#include "llfontbitmapcache.h"
#include "llfontregistry.h"
#include "llgl.h"
#include "llimagegl.h"
#include "llrender.h"
#include "llstl.h"
#include "v4color.h"
#include "lltexture.h"
#include "lldir.h"
#include "llstring.h"
#include <boost/tokenizer.hpp>
#if LL_WINDOWS
#include <Shlobj.h>
#include <Knownfolders.h>
#include <Objbase.h>
#endif
const S32 BOLD_OFFSET = 1;
F32 LLFontGL::sVertDPI = 96.f;
F32 LLFontGL::sHorizDPI = 96.f;
F32 LLFontGL::sScaleX = 1.f;
F32 LLFontGL::sScaleY = 1.f;
S32 LLFontGL::sResolutionGeneration = 0;
bool LLFontGL::sDisplayFont = true ;
std::string LLFontGL::sAppDir;
LLColor4 LLFontGL::sShadowColor(0.f, 0.f, 0.f, 1.f);
LLFontRegistry* LLFontGL::sFontRegistry = NULL;
LLCoordGL LLFontGL::sCurOrigin;
F32 LLFontGL::sCurDepth;
std::vector<std::pair<LLCoordGL, F32> > LLFontGL::sOriginStack;
const F32 PAD_UVY = 0.5f;
const F32 DROP_SHADOW_SOFT_STRENGTH = 0.3f;
LLFontGL::LLFontGL()
{
}
LLFontGL::~LLFontGL()
{
}
void LLFontGL::reset()
{
    mFontFreetype->reset(sVertDPI, sHorizDPI);
}
void LLFontGL::destroyGL()
{
    mFontFreetype->destroyGL();
}
bool LLFontGL::loadFace(const std::string& filename, F32 point_size, const F32 vert_dpi, const F32 horz_dpi, S32 weight, bool is_fallback, S32 face_n, EFontHinting hinting, S32 flags)
{
    if(mFontFreetype == reinterpret_cast<LLFontFreetype*>(NULL))
    {
        mFontFreetype = new LLFontFreetype;
    }
    return mFontFreetype->loadFace(filename, point_size, vert_dpi, horz_dpi, weight, is_fallback, face_n, hinting, flags);
}
S32 LLFontGL::getNumFaces(const std::string& filename)
{
    if (mFontFreetype == reinterpret_cast<LLFontFreetype*>(NULL))
    {
        mFontFreetype = new LLFontFreetype;
    }
    return mFontFreetype->getNumFaces(filename);
}
S32 LLFontGL::getCacheGeneration() const
{
    const LLFontBitmapCache* font_bitmap_cache = mFontFreetype->getFontBitmapCache();
    return font_bitmap_cache->getCacheGeneration();
}
S32 LLFontGL::render(const LLWString &wstr, S32 begin_offset, const LLRect& rect, const LLColor4 &color, HAlign halign, VAlign valign, U8 style,
    ShadowType shadow, S32 max_chars, F32* right_x, bool use_ellipses, bool use_color) const
{
    LLRectf rect_float((F32)rect.mLeft, (F32)rect.mTop, (F32)rect.mRight, (F32)rect.mBottom);
    return render(wstr, begin_offset, rect_float, color, halign, valign, style, shadow, max_chars, right_x, use_ellipses, use_color);
}
S32 LLFontGL::render(const LLWString &wstr, S32 begin_offset, const LLRectf& rect, const LLColor4 &color, HAlign halign, VAlign valign, U8 style,
                     ShadowType shadow, S32 max_chars, F32* right_x, bool use_ellipses, bool use_color) const
{
    F32 x = rect.mLeft;
    F32 y = 0.f;
    switch(valign)
    {
    case TOP:
        y = rect.mTop;
        break;
    case VCENTER:
        y = rect.getCenterY();
        break;
    case BASELINE:
    case BOTTOM:
        y = rect.mBottom;
        break;
    default:
        y = rect.mBottom;
        break;
    }
    return render(wstr, begin_offset, x, y, color, halign, valign, style, shadow, max_chars, (S32)rect.getWidth(), right_x, use_ellipses, use_color);
}
S32 LLFontGL::render(const LLWString &wstr, S32 begin_offset, F32 x, F32 y, const LLColor4 &color, HAlign halign, VAlign valign, U8 style,
                     ShadowType shadow, S32 max_chars, S32 max_pixels, F32* right_x, bool use_ellipses, bool use_color) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    if(!sDisplayFont)
    {
        return static_cast<S32>(wstr.length());
    }
    if (wstr.empty())
    {
        return 0;
    }
    gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
    S32 scaled_max_pixels = max_pixels == S32_MAX ? S32_MAX : llceil((F32)max_pixels * sScaleX);
    U8 style_to_add = (style | mFontDescriptor.getStyle()) & ~mFontFreetype->getStyle();
    F32 drop_shadow_strength = 0.f;
    if (shadow != NO_SHADOW)
    {
        F32 luminance;
        color.calcHSL(NULL, NULL, &luminance);
        drop_shadow_strength = clamp_rescale(luminance, 0.35f, 0.6f, 0.f, 1.f);
        if (luminance < 0.35f)
        {
            shadow = NO_SHADOW;
        }
    }
    gGL.pushUIMatrix();
    gGL.loadUIIdentity();
    LLVector2 origin(floorf(sCurOrigin.mX*sScaleX), floorf(sCurOrigin.mY*sScaleY));
    gGL.translatef(0.f,0.f,sCurDepth);
    S32 chars_drawn = 0;
    S32 i;
    S32 length;
    if (-1 == max_chars)
    {
        max_chars = length = (S32)wstr.length() - begin_offset;
    }
    else
    {
        length = llmin((S32)wstr.length() - begin_offset, max_chars );
    }
    F32 cur_x, cur_y, cur_render_x, cur_render_y;
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    cur_x = ((F32)x * sScaleX) + origin.mV[VX];
    cur_y = ((F32)y * sScaleY) + origin.mV[VY];
    switch (valign)
    {
    case TOP:
        cur_y -= llceil(mFontFreetype->getAscenderHeight());
        break;
    case BOTTOM:
        cur_y += llceil(mFontFreetype->getDescenderHeight());
        break;
    case VCENTER:
        cur_y -= llceil((llceil(mFontFreetype->getAscenderHeight()) - llceil(mFontFreetype->getDescenderHeight())) / 2.f);
        break;
    case BASELINE:
        break;
    default:
        break;
    }
    switch (halign)
    {
    case LEFT:
        break;
    case RIGHT:
        cur_x -= llmin(scaled_max_pixels, ll_round(getWidthF32(wstr.c_str(), begin_offset, length) * sScaleX));
        break;
    case HCENTER:
        cur_x -= llmin(scaled_max_pixels, ll_round(getWidthF32(wstr.c_str(), begin_offset, length) * sScaleX)) / 2;
        break;
    default:
        break;
    }
    cur_render_y = cur_y;
    cur_render_x = cur_x;
    F32 start_x = (F32)ll_round(cur_x);
    const LLFontBitmapCache* font_bitmap_cache = mFontFreetype->getFontBitmapCache();
    F32 inv_width = 1.f / font_bitmap_cache->getBitmapWidth();
    F32 inv_height = 1.f / font_bitmap_cache->getBitmapHeight();
    const S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;
    bool draw_ellipses = false;
    if (use_ellipses)
    {
        S32 string_width = ll_round(getWidthF32(wstr.c_str(), begin_offset, max_chars) * sScaleX);
        if (string_width > scaled_max_pixels)
        {
            const LLWString dots(utf8str_to_wstring(std::string("....")));
            scaled_max_pixels = llmax(0, scaled_max_pixels - ll_round(getWidthF32(dots.c_str())));
            draw_ellipses = true;
        }
    }
    const LLFontGlyphInfo* next_glyph = NULL;
    static constexpr S32 GLYPH_BATCH_SIZE = 30;
    static thread_local LLVector4a vertices[GLYPH_BATCH_SIZE * 6];
    static thread_local LLVector2 uvs[GLYPH_BATCH_SIZE * 6];
    static thread_local LLColor4U colors[GLYPH_BATCH_SIZE * 6];
    LLColor4U text_color(color);
    LLColor4U emoji_color(255, 255, 255, text_color.mV[VALPHA]);
    std::pair<EFontGlyphType, S32> bitmap_entry = std::make_pair(EFontGlyphType::Grayscale, -1);
    S32 glyph_count = 0;
    llwchar last_char = wstr[begin_offset];
    for (i = begin_offset; i < begin_offset + length; i++)
    {
        llwchar wch = wstr[i];
        const LLFontGlyphInfo* fgi = next_glyph;
        next_glyph = NULL;
        if(!fgi)
        {
            fgi = mFontFreetype->getGlyphInfo(wch, (!use_color) ? EFontGlyphType::Grayscale : EFontGlyphType::Color);
        }
        if (!fgi)
        {
            LL_ERRS() << "Missing Glyph Info" << LL_ENDL;
            break;
        }
        std::pair<EFontGlyphType, S32> next_bitmap_entry = fgi->mBitmapEntry;
        if (next_bitmap_entry != bitmap_entry || last_char != wch)
        {
            if (glyph_count > 0)
            {
                gGL.begin(LLRender::TRIANGLES);
                {
                    gGL.vertexBatchPreTransformed(vertices, uvs, colors, glyph_count * 6);
                }
                gGL.end();
                glyph_count = 0;
            }
            bitmap_entry = next_bitmap_entry;
            LLImageGL* font_image = font_bitmap_cache->getImageGL(bitmap_entry.first, bitmap_entry.second);
            gGL.getTexUnit(0)->bind(font_image);
            last_char = wch;
        }
        if ((start_x + scaled_max_pixels) < (cur_x + fgi->mXBearing + fgi->mWidth))
        {
            break;
        }
        F32 x_offset = 0.0f;
        if (mFontFreetype->getFontWeight() > 0 && fgi->mChar >= '0' && fgi->mChar <= '9' && mFontFreetype->getMaxDigitWidth() > 0.0f)
        {
            x_offset = (mFontFreetype->getMaxDigitWidth() - fgi->mXAdvance) * 0.5f;
        }
        LLRectf uv_rect((fgi->mXBitmapOffset) * inv_width,
                (fgi->mYBitmapOffset + fgi->mHeight + PAD_UVY) * inv_height,
                (fgi->mXBitmapOffset + fgi->mWidth) * inv_width,
                (fgi->mYBitmapOffset - PAD_UVY) * inv_height);
        LLRectf screen_rect((F32)ll_round(cur_render_x + (F32)fgi->mXBearing + x_offset),
                    (F32)ll_round(cur_render_y + (F32)fgi->mYBearing),
                    (F32)ll_round(cur_render_x + (F32)fgi->mXBearing + x_offset) + (F32)fgi->mWidth,
                    (F32)ll_round(cur_render_y + (F32)fgi->mYBearing) - (F32)fgi->mHeight);
        if (glyph_count >= GLYPH_BATCH_SIZE)
        {
            gGL.begin(LLRender::TRIANGLES);
            {
                gGL.vertexBatchPreTransformed(vertices, uvs, colors, glyph_count * 6);
            }
            gGL.end();
            glyph_count = 0;
        }
        const LLColor4U& col =
            bitmap_entry.first == EFontGlyphType::Grayscale ? text_color
                                                            : emoji_color;
        drawGlyph(glyph_count, vertices, uvs, colors, screen_rect, uv_rect,
                  col, style_to_add, shadow, drop_shadow_strength);
        chars_drawn++;
        cur_x += mFontFreetype->getXAdvance(fgi);
        cur_y += fgi->mYAdvance;
        llwchar next_char = wstr[i+1];
        if (next_char && (next_char < LAST_CHARACTER))
        {
            next_glyph = mFontFreetype->getGlyphInfo(next_char, (!use_color) ? EFontGlyphType::Grayscale : EFontGlyphType::Color);
            cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
        }
        cur_x = (F32)ll_round(cur_x);
        cur_render_x = cur_x;
        cur_render_y = cur_y;
    }
    gGL.begin(LLRender::TRIANGLES);
    {
        gGL.vertexBatchPreTransformed(vertices, uvs, colors, glyph_count * 6);
    }
    gGL.end();
    if (right_x)
    {
        *right_x = (cur_x - origin.mV[VX]) / sScaleX;
    }
    if (style_to_add & UNDERLINE)
    {
        F32 descender = (F32)llfloor(mFontFreetype->getDescenderHeight());
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.begin(LLRender::LINES);
        gGL.vertex2f(start_x, cur_y - descender);
        gGL.vertex2f(cur_x, cur_y - descender);
        gGL.end();
    }
    if (draw_ellipses)
    {
        static LLWString elipses_wstr(utf8string_to_wstring(std::string("...")));
        render(elipses_wstr,
                0,
                (cur_x - origin.mV[VX]) / sScaleX, (F32)y,
                color,
                LEFT, valign,
                style_to_add,
                shadow,
                S32_MAX, max_pixels,
                right_x,
                false,
                use_color);
    }
    gGL.popUIMatrix();
    return chars_drawn;
}
S32 LLFontGL::render(const LLWString &text, S32 begin_offset, F32 x, F32 y, const LLColor4 &color) const
{
    return render(text, begin_offset, x, y, color, LEFT, BASELINE, NORMAL, NO_SHADOW);
}
S32 LLFontGL::renderUTF8(const std::string &text, S32 begin_offset, F32 x, F32 y, const LLColor4 &color, HAlign halign, VAlign valign, U8 style, ShadowType shadow, S32 max_chars, S32 max_pixels, F32* right_x, bool use_ellipses, bool use_color) const
{
    return render(utf8str_to_wstring(text), begin_offset, x, y, color, halign, valign, style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
}
S32 LLFontGL::renderUTF8(const std::string &text, S32 begin_offset, S32 x, S32 y, const LLColor4 &color) const
{
    return renderUTF8(text, begin_offset, (F32)x, (F32)y, color, LEFT, BASELINE, NORMAL, NO_SHADOW);
}
S32 LLFontGL::renderUTF8(const std::string &text, S32 begin_offset, S32 x, S32 y, const LLColor4 &color, HAlign halign, VAlign valign, U8 style, ShadowType shadow) const
{
    return renderUTF8(text, begin_offset, (F32)x, (F32)y, color, halign, valign, style, shadow);
}
F32 LLFontGL::getAscenderHeight() const
{
    return mFontFreetype->getAscenderHeight() / sScaleY;
}
F32 LLFontGL::getDescenderHeight() const
{
    return mFontFreetype->getDescenderHeight() / sScaleY;
}
S32 LLFontGL::getLineHeight() const
{
    return llceil(mFontFreetype->getAscenderHeight() / sScaleY) + llceil(mFontFreetype->getDescenderHeight() / sScaleY);
}
S32 LLFontGL::getWidth(const std::string& utf8text) const
{
    LLWString wtext = utf8str_to_wstring(utf8text);
    return getWidth(wtext.c_str(), 0, S32_MAX);
}
S32 LLFontGL::getWidth(const llwchar* wchars) const
{
    return getWidth(wchars, 0, S32_MAX);
}
S32 LLFontGL::getWidth(const std::string& utf8text, S32 begin_offset, S32 max_chars) const
{
    LLWString wtext = utf8str_to_wstring(utf8text);
    return getWidth(wtext.c_str(), begin_offset, max_chars);
}
S32 LLFontGL::getWidth(const llwchar* wchars, S32 begin_offset, S32 max_chars) const
{
    F32 width = getWidthF32(wchars, begin_offset, max_chars);
    return ll_round(width);
}
F32 LLFontGL::getWidthF32(const std::string& utf8text) const
{
    LLWString wtext = utf8str_to_wstring(utf8text);
    return getWidthF32(wtext.c_str(), 0, S32_MAX);
}
F32 LLFontGL::getWidthF32(const llwchar* wchars) const
{
    return getWidthF32(wchars, 0, S32_MAX);
}
F32 LLFontGL::getWidthF32(const std::string& utf8text, S32 begin_offset, S32 max_chars) const
{
    LLWString wtext = utf8str_to_wstring(utf8text);
    return getWidthF32(wtext.c_str(), begin_offset, max_chars);
}
F32 LLFontGL::getWidthF32(const llwchar* wchars, S32 begin_offset, S32 max_chars) const
{
    return getWidthF32(wchars, begin_offset, max_chars, FALSE);
}
F32 LLFontGL::getWidthF32(const llwchar* wchars, S32 begin_offset, S32 max_chars, BOOL use_embedded) const
{
    if (use_embedded)
    {
        LLWString wtext;
        for (S32 i = begin_offset; wchars[i] != 0 && i < begin_offset + max_chars; ++i)
        {
            wtext.push_back(wchars[i]);
        }
        return getWidthF32(wtext, 0, (S32)wtext.length(), TRUE);
    }
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    const S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;
    F32 cur_x = 0;
    const S32 max_index = begin_offset + max_chars;
    const LLFontGlyphInfo* next_glyph = NULL;
    F32 width_padding = 0.f;
    for (S32 i = begin_offset; i < max_index && wchars[i] != 0; i++)
    {
        llwchar wch = wchars[i];
        const LLFontGlyphInfo* fgi = next_glyph;
        next_glyph = NULL;
        if(!fgi)
        {
            fgi = mFontFreetype->getGlyphInfo(wch, EFontGlyphType::Unspecified);
        }
        F32 advance = mFontFreetype->getXAdvance(fgi);
        width_padding = llmax(0.f,
            width_padding - advance,
            (F32)(fgi->mWidth + fgi->mXBearing) - advance);
        cur_x += advance;
        llwchar next_char = wchars[i+1];
        if (((i + 1) < begin_offset + max_chars)
            && next_char
            && (next_char < LAST_CHARACTER))
        {
            next_glyph = mFontFreetype->getGlyphInfo(next_char, EFontGlyphType::Unspecified);
            cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
        }
        cur_x = (F32)ll_round(cur_x);
    }
    cur_x += width_padding;
    return cur_x / sScaleX;
}
void LLFontGL::generateASCIIglyphs()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    for (U32 i = 32; (i < 127); i++)
    {
        mFontFreetype->getGlyphInfo(i, EFontGlyphType::Grayscale);
    }
}
S32 LLFontGL::maxDrawableChars(const llwchar* wchars, F32 max_pixels, S32 max_chars, EWordWrapStyle end_on_word_boundary) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    if (!wchars || !wchars[0] || max_chars == 0)
    {
        return 0;
    }
    llassert(max_pixels >= 0.f);
    llassert(max_chars >= 0);
    bool clip = false;
    F32 cur_x = 0;
    S32 start_of_last_word = 0;
    bool in_word = false;
    F32 scaled_max_pixels = max_pixels * sScaleX;
    F32 width_padding = 0.f;
    LLFontGlyphInfo* next_glyph = NULL;
    S32 i;
    for (i=0; (i < max_chars); i++)
    {
        llwchar wch = wchars[i];
        if(wch == 0)
        {
            break;
        }
        if (in_word)
        {
            if (iswspace(wch))
            {
                if(wch !=(0x00A0))
                {
                    in_word = false;
                }
            }
            if (iswindividual(wch))
            {
                if (iswpunct(wchars[i+1]))
                {
                    in_word=true;
                }
                else
                {
                    in_word=false;
                    start_of_last_word = i;
                }
            }
        }
        else
        {
            start_of_last_word = i;
            if (!iswspace(wch)||!iswindividual(wch))
            {
                in_word = true;
            }
        }
        LLFontGlyphInfo* fgi = next_glyph;
        next_glyph = NULL;
        if(!fgi)
        {
            fgi = mFontFreetype->getGlyphInfo(wch, EFontGlyphType::Unspecified);
            if (NULL == fgi)
            {
                return 0;
            }
        }
        width_padding = llmax(  0.f,
                                width_padding - fgi->mXAdvance,
                                (F32)(fgi->mWidth + fgi->mXBearing) - fgi->mXAdvance);
        cur_x += fgi->mXAdvance;
        if (scaled_max_pixels < cur_x + width_padding)
        {
            clip = true;
            break;
        }
        if (((i+1) < max_chars) && wchars[i+1])
        {
            next_glyph = mFontFreetype->getGlyphInfo(wchars[i+1], EFontGlyphType::Unspecified);
            cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
        }
        cur_x = (F32)ll_round(cur_x);
    }
    if( clip )
    {
        switch (end_on_word_boundary)
        {
        case ONLY_WORD_BOUNDARIES:
            i = start_of_last_word;
            break;
        case WORD_BOUNDARY_IF_POSSIBLE:
            if (start_of_last_word != 0)
            {
                i = start_of_last_word;
            }
            break;
        default:
        case ANYWHERE:
            break;
        }
    }
    return i;
}
S32 LLFontGL::firstDrawableChar(const llwchar* wchars, F32 max_pixels, S32 text_len, S32 start_pos, S32 max_chars) const
{
    if (!wchars || !wchars[0] || max_chars == 0)
    {
        return 0;
    }
    F32 total_width = 0.0;
    S32 drawable_chars = 0;
    F32 scaled_max_pixels = max_pixels * sScaleX;
    S32 start = llmin(start_pos, text_len - 1);
    for (S32 i = start; i >= 0; i--)
    {
        llwchar wch = wchars[i];
        const LLFontGlyphInfo* fgi= mFontFreetype->getGlyphInfo(wch, EFontGlyphType::Unspecified);
        F32 width = (i == start)
            ? (F32)(fgi->mWidth + fgi->mXBearing)
            : fgi->mXAdvance;
        if( scaled_max_pixels < (total_width + width) )
        {
            break;
        }
        total_width += width;
        drawable_chars++;
        if( max_chars >= 0 && drawable_chars >= max_chars )
        {
            break;
        }
        if ( i > 0 )
        {
            total_width += mFontFreetype->getXKerning(wchars[i-1], wch);
        }
        total_width = (F32)ll_round(total_width);
    }
    if (drawable_chars == 0)
    {
        return start_pos;
    }
    else
    {
        return start_pos + 1 - drawable_chars;
    }
}
S32 LLFontGL::charFromPixelOffset(const llwchar* wchars, S32 begin_offset, F32 target_x, F32 max_pixels, S32 max_chars, bool round) const
{
    if (!wchars || !wchars[0] || max_chars == 0)
    {
        return 0;
    }
    F32 cur_x = 0;
    target_x *= sScaleX;
    const S32 max_index = begin_offset + llmin(S32_MAX - begin_offset, max_chars);
    F32 scaled_max_pixels = max_pixels * sScaleX;
    const LLFontGlyphInfo* next_glyph = NULL;
    S32 pos;
    for (pos = begin_offset; pos < max_index; pos++)
    {
        llwchar wch = wchars[pos];
        if (!wch)
        {
            break;
        }
        const LLFontGlyphInfo* glyph = next_glyph;
        next_glyph = NULL;
        if(!glyph)
        {
            glyph = mFontFreetype->getGlyphInfo(wch, EFontGlyphType::Unspecified);
        }
        F32 char_width = mFontFreetype->getXAdvance(glyph);
        if (round)
        {
            if (target_x  < cur_x + char_width*0.5f)
            {
                break;
            }
        }
        else if (target_x  < cur_x + char_width)
        {
            break;
        }
        if (scaled_max_pixels < cur_x + char_width)
        {
            break;
        }
        cur_x += char_width;
        if (((pos + 1) < max_index)
            && (wchars[(pos + 1)]))
        {
            next_glyph = mFontFreetype->getGlyphInfo(wchars[pos + 1], EFontGlyphType::Unspecified);
            cur_x += mFontFreetype->getXKerning(glyph, next_glyph);
        }
        cur_x = (F32)ll_round(cur_x);
    }
    return llmin(max_chars, pos - begin_offset);
}
const LLFontDescriptor& LLFontGL::getFontDesc() const
{
    return mFontDescriptor;
}
void LLFontGL::initClass(F32 screen_dpi, F32 x_scale, F32 y_scale, const std::string& app_dir, bool create_gl_textures)
{
    sVertDPI = (F32)llfloor(screen_dpi * y_scale);
    sHorizDPI = (F32)llfloor(screen_dpi * x_scale);
    sScaleX = x_scale;
    sScaleY = y_scale;
    sAppDir = app_dir;
    if (!sFontRegistry)
    {
        sFontRegistry = new LLFontRegistry(create_gl_textures);
        sFontRegistry->parseFontInfo("fonts.xml");
    }
    else
    {
        sFontRegistry->reset();
    }
    LLFontGL::loadDefaultFonts();
}
void LLFontGL::dumpTextures()
{
    if (mFontFreetype.notNull())
    {
        mFontFreetype->dumpFontBitmaps();
    }
}
void LLFontGL::dumpFonts()
{
    sFontRegistry->dump();
}
void LLFontGL::dumpFontTextures()
{
    sFontRegistry->dumpTextures();
}
bool LLFontGL::loadDefaultFonts()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    bool succ = true;
    succ &= (NULL != getFontSansSerifSmall());
    succ &= (NULL != getFontSansSerif());
    succ &= (NULL != getFontSansSerifBig());
    succ &= (NULL != getFontSansSerifHuge());
    succ &= (NULL != getFontSansSerifBold());
    succ &= (NULL != getFontMonospace());
    return succ;
}
void LLFontGL::loadCommonFonts()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    getFont(LLFontDescriptor("SansSerif", "Small", BOLD));
    getFont(LLFontDescriptor("SansSerif", "Large", BOLD));
    getFont(LLFontDescriptor("SansSerif", "Huge", BOLD));
    getFont(LLFontDescriptor("Monospace", "Medium", 0));
}
void LLFontGL::destroyDefaultFonts()
{
    delete sFontRegistry;
    sFontRegistry = NULL;
}
void LLFontGL::destroyAllGL()
{
    if (sFontRegistry)
    {
        sFontRegistry->destroyGL();
    }
}
U8 LLFontGL::getStyleFromString(const std::string &style)
{
    S32 ret = 0;
    if (style.find("BOLD") != style.npos)
    {
        ret |= BOLD;
    }
    if (style.find("ITALIC") != style.npos)
    {
        ret |= ITALIC;
    }
    if (style.find("UNDERLINE") != style.npos)
    {
        ret |= UNDERLINE;
    }
    return ret;
}
std::string LLFontGL::getStringFromStyle(U8 style)
{
    std::string style_string;
    if (style == NORMAL)
    {
        style_string += "|NORMAL";
    }
    if (style & BOLD)
    {
        style_string += "|BOLD";
    }
    if (style & ITALIC)
    {
        style_string += "|ITALIC";
    }
    if (style & UNDERLINE)
    {
        style_string += "|UNDERLINE";
    }
    return style_string;
}
std::string LLFontGL::nameFromFont(const LLFontGL* fontp)
{
    return fontp->mFontDescriptor.getName();
}
std::string LLFontGL::sizeFromFont(const LLFontGL* fontp)
{
    return fontp->mFontDescriptor.getSize();
}
std::string LLFontGL::nameFromHAlign(LLFontGL::HAlign align)
{
    if (align == LEFT)          return std::string("left");
    else if (align == RIGHT)    return std::string("right");
    else if (align == HCENTER)  return std::string("center");
    else return std::string();
}
LLFontGL::HAlign LLFontGL::hAlignFromName(const std::string& name)
{
    LLFontGL::HAlign gl_hfont_align = LLFontGL::LEFT;
    if (name == "left")
    {
        gl_hfont_align = LLFontGL::LEFT;
    }
    else if (name == "right")
    {
        gl_hfont_align = LLFontGL::RIGHT;
    }
    else if (name == "center")
    {
        gl_hfont_align = LLFontGL::HCENTER;
    }
    return gl_hfont_align;
}
std::string LLFontGL::nameFromVAlign(LLFontGL::VAlign align)
{
    if (align == TOP)           return std::string("top");
    else if (align == VCENTER)  return std::string("center");
    else if (align == BASELINE) return std::string("baseline");
    else if (align == BOTTOM)   return std::string("bottom");
    else return std::string();
}
LLFontGL::VAlign LLFontGL::vAlignFromName(const std::string& name)
{
    LLFontGL::VAlign gl_vfont_align = LLFontGL::BASELINE;
    if (name == "top")
    {
        gl_vfont_align = LLFontGL::TOP;
    }
    else if (name == "center")
    {
        gl_vfont_align = LLFontGL::VCENTER;
    }
    else if (name == "baseline")
    {
        gl_vfont_align = LLFontGL::BASELINE;
    }
    else if (name == "bottom")
    {
        gl_vfont_align = LLFontGL::BOTTOM;
    }
    return gl_vfont_align;
}
LLFontGL* LLFontGL::getFontEmojiSmall()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("Emoji", "Small", 0));
    return fontp;;
}
LLFontGL* LLFontGL::getFontEmojiMedium()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("Emoji", "Medium", 0));
    return fontp;;
}
LLFontGL* LLFontGL::getFontEmojiLarge()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("Emoji", "Large", 0));
    return fontp;;
}
LLFontGL* LLFontGL::getFontEmojiHuge()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("Emoji", "Huge", 0));
    return fontp;;
}
LLFontGL* LLFontGL::getFontMonospace()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("Monospace","Monospace",0));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerifSmall()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Small",0));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerifSmallBold()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Small",BOLD));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerifSmallItalic()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Small",ITALIC));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerif()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Small",0));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerifMedium()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif", "Medium", 0));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerifBig()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Large",0));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerifHuge()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Huge",0));
    return fontp;
}
LLFontGL* LLFontGL::getFontSansSerifBold()
{
    static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Medium",BOLD));
    return fontp;
}
LLFontGL* LLFontGL::getFont(const LLFontDescriptor& desc)
{
    return sFontRegistry->getFont(desc);
}
LLFontGL* LLFontGL::getFontByName(const std::string& name)
{
    if (name == "SANSSERIF")
    {
        return getFontSansSerif();
    }
    else if (name == "SANSSERIF_SMALL")
    {
        return getFontSansSerifSmall();
    }
    else if (name == "SANSSERIF_BIG")
    {
        return getFontSansSerifBig();
    }
    else if (name == "SMALL" || name == "OCRA")
    {
        return getFontMonospace();
    }
    else
    {
        return NULL;
    }
}
LLFontGL* LLFontGL::getFontDefault()
{
    return getFontSansSerif();
}
std::string LLFontGL::getFontPathSystem()
{
#if LL_DARWIN
    return "/System/Library/Fonts/";
#elif LL_WINDOWS
    auto system_root = LLStringUtil::getenv("SystemRoot");
    if (! system_root.empty())
    {
        std::string fontpath(gDirUtilp->add(system_root, "fonts") + gDirUtilp->getDirDelimiter());
        LL_INFOS() << "from SystemRoot: " << fontpath << LL_ENDL;
        return fontpath;
    }
    wchar_t *pwstr = NULL;
    HRESULT okay = SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &pwstr);
    if (SUCCEEDED(okay) && pwstr)
    {
        std::string fontpath(ll_convert_wide_to_string(pwstr));
        CoTaskMemFree(pwstr);
        LL_INFOS() << "from SHGetKnownFolderPath(): " << fontpath << LL_ENDL;
        return fontpath;
    }
#endif
    LL_WARNS() << "Could not determine system fonts path" << LL_ENDL;
    return {};
}
std::string LLFontGL::getFontPathLocal()
{
    std::string local_path;
    if (LLFontGL::sAppDir.length())
    {
        local_path = LLFontGL::sAppDir + "/fonts/";
    }
    else
    {
        local_path = "./fonts/";
    }
    return local_path;
}
LLFontGL::LLFontGL(const LLFontGL &source)
{
    LL_ERRS() << "Not implemented!" << LL_ENDL;
}
LLFontGL &LLFontGL::operator=(const LLFontGL &source)
{
    LL_ERRS() << "Not implemented" << LL_ENDL;
    return *this;
}
void LLFontGL::renderTriangle(LLVector4a* vertex_out, LLVector2* uv_out, LLColor4U* colors_out, const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4U& color, F32 slant_amt) const
{
    S32 index = 0;
    vertex_out[index].set(screen_rect.mRight, screen_rect.mTop, 0.f);
    uv_out[index].set(uv_rect.mRight, uv_rect.mTop);
    colors_out[index] = color;
    index++;
    vertex_out[index].set(screen_rect.mLeft, screen_rect.mTop, 0.f);
    uv_out[index].set(uv_rect.mLeft, uv_rect.mTop);
    colors_out[index] = color;
    index++;
    vertex_out[index].set(screen_rect.mLeft, screen_rect.mBottom, 0.f);
    uv_out[index].set(uv_rect.mLeft, uv_rect.mBottom);
    colors_out[index] = color;
    index++;
    vertex_out[index].set(screen_rect.mRight, screen_rect.mTop, 0.f);
    uv_out[index].set(uv_rect.mRight, uv_rect.mTop);
    colors_out[index] = color;
    index++;
    vertex_out[index].set(screen_rect.mLeft, screen_rect.mBottom, 0.f);
    uv_out[index].set(uv_rect.mLeft, uv_rect.mBottom);
    colors_out[index] = color;
    index++;
    vertex_out[index].set(screen_rect.mRight, screen_rect.mBottom, 0.f);
    uv_out[index].set(uv_rect.mRight, uv_rect.mBottom);
    colors_out[index] = color;
}
void LLFontGL::drawGlyph(S32& glyph_count, LLVector4a* vertex_out, LLVector2* uv_out, LLColor4U* colors_out, const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4U& color, U8 style, ShadowType shadow, F32 drop_shadow_strength) const
{
    F32 slant_offset;
    slant_offset = ((style & ITALIC) ? ( -mFontFreetype->getAscenderHeight() * 0.2f) : 0.f);
    if (style & BOLD)
    {
        for (S32 pass = 0; pass < 2; pass++)
        {
            LLRectf screen_rect_offset = screen_rect;
            screen_rect_offset.translate((F32)(pass * BOLD_OFFSET), 0.f);
            renderTriangle(&vertex_out[glyph_count * 6], &uv_out[glyph_count * 6], &colors_out[glyph_count * 6], screen_rect_offset, uv_rect, color, slant_offset);
            glyph_count++;
        }
    }
    else if (shadow == DROP_SHADOW_SOFT)
    {
        LLColor4U shadow_color = LLFontGL::sShadowColor;
        shadow_color.mV[VALPHA] = U8(color.mV[VALPHA] * drop_shadow_strength * DROP_SHADOW_SOFT_STRENGTH);
        for (S32 pass = 0; pass < 5; pass++)
        {
            LLRectf screen_rect_offset = screen_rect;
            switch(pass)
            {
            case 0:
                screen_rect_offset.translate(-1.f, -1.f);
                break;
            case 1:
                screen_rect_offset.translate(1.f, -1.f);
                break;
            case 2:
                screen_rect_offset.translate(1.f, 1.f);
                break;
            case 3:
                screen_rect_offset.translate(-1.f, 1.f);
                break;
            case 4:
                screen_rect_offset.translate(0, -2.f);
                break;
            }
            renderTriangle(&vertex_out[glyph_count * 6], &uv_out[glyph_count * 6], &colors_out[glyph_count * 6], screen_rect_offset, uv_rect, shadow_color, slant_offset);
            glyph_count++;
        }
        renderTriangle(&vertex_out[glyph_count * 6], &uv_out[glyph_count * 6], &colors_out[glyph_count * 6], screen_rect, uv_rect, color, slant_offset);
        glyph_count++;
    }
    else if (shadow == DROP_SHADOW)
    {
        LLColor4U shadow_color = LLFontGL::sShadowColor;
        shadow_color.mV[VALPHA] = U8(color.mV[VALPHA] * drop_shadow_strength);
        LLRectf screen_rect_shadow = screen_rect;
        screen_rect_shadow.translate(1.f, -1.f);
        renderTriangle(&vertex_out[glyph_count * 6], &uv_out[glyph_count * 6], &colors_out[glyph_count * 6], screen_rect_shadow, uv_rect, shadow_color, slant_offset);
        glyph_count++;
        renderTriangle(&vertex_out[glyph_count * 6], &uv_out[glyph_count * 6], &colors_out[glyph_count * 6], screen_rect, uv_rect, color, slant_offset);
        glyph_count++;
    }
    else
    {
        renderTriangle(&vertex_out[glyph_count * 6], &uv_out[glyph_count * 6], &colors_out[glyph_count * 6], screen_rect, uv_rect, color, slant_offset);
        glyph_count++;
    }
}
