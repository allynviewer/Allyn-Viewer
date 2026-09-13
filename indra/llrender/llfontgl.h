/**
 * @file llfontgl.h
 * @author Doug Soo
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
#ifndef LL_LLFONTGL_H
#define LL_LLFONTGL_H
#include "llcoord.h"
#include "llfontregistry.h"
#include "llimagegl.h"
#include "llpointer.h"
#include "llrect.h"
#include "v2math.h"
class LLColor4;
class LLFontDescriptor;
class LLFontFreetype;
class LLFontRegistry;
class LLFontGL
{
public:
    enum HAlign
    {
        LEFT = 0,
        RIGHT = 1,
        HCENTER = 2,
    };
    enum VAlign
    {
        TOP = 3,
        VCENTER = 4,
        BASELINE = 5,
        BOTTOM = 6
    };
    enum StyleFlags
    {
        NORMAL    = 0x00,
        BOLD      = 0x01,
        ITALIC    = 0x02,
        UNDERLINE = 0x04
    };
    enum ShadowType
    {
        NO_SHADOW,
        DROP_SHADOW,
        DROP_SHADOW_SOFT
    };
    LLFontGL();
    ~LLFontGL();
    void reset();
    void destroyGL();
    bool loadFace(const std::string& filename, F32 point_size, const F32 vert_dpi, const F32 horz_dpi, S32 weight, bool is_fallback, S32 face_n, EFontHinting hinting, S32 flags);
    S32 getNumFaces(const std::string& filename);
    S32 getCacheGeneration() const;
    S32 render(const LLWString &text, S32 begin_offset,
                const LLRect& rect,
                const LLColor4 &color,
                HAlign halign = LEFT,  VAlign valign = BASELINE,
                U8 style = NORMAL, ShadowType shadow = NO_SHADOW,
                S32 max_chars = S32_MAX,
                F32* right_x=NULL,
                bool use_ellipses = false,
                bool use_color = true) const;
    S32 render(const LLWString &text, S32 begin_offset,
                const LLRectf& rect,
                const LLColor4 &color,
                HAlign halign = LEFT,  VAlign valign = BASELINE,
                U8 style = NORMAL, ShadowType shadow = NO_SHADOW,
                S32 max_chars = S32_MAX,
                F32* right_x=NULL,
                bool use_ellipses = false,
                bool use_color = true) const;
    S32 render(const LLWString &text, S32 begin_offset,
                F32 x, F32 y,
                const LLColor4 &color,
                HAlign halign = LEFT,  VAlign valign = BASELINE,
                U8 style = NORMAL, ShadowType shadow = NO_SHADOW,
                S32 max_chars = S32_MAX, S32 max_pixels = S32_MAX,
                F32* right_x=NULL,
                bool use_ellipses = false,
                bool use_color = true) const;
    S32 render(const LLWString &text, S32 begin_offset, F32 x, F32 y, const LLColor4 &color) const;
    S32 renderUTF8(const std::string &text, S32 begin_offset, F32 x, F32 y, const LLColor4 &color, HAlign halign,  VAlign valign, U8 style, ShadowType shadow, S32 max_chars = S32_MAX, S32 max_pixels = S32_MAX,  F32* right_x = NULL, bool use_ellipses = false, bool use_color = true) const;
    S32 renderUTF8(const std::string &text, S32 begin_offset, S32 x, S32 y, const LLColor4 &color) const;
    S32 renderUTF8(const std::string &text, S32 begin_offset, S32 x, S32 y, const LLColor4 &color, HAlign halign, VAlign valign, U8 style = NORMAL, ShadowType shadow = NO_SHADOW) const;
    F32 getAscenderHeight() const;
    F32 getDescenderHeight() const;
    S32 getLineHeight() const;
    S32 getWidth(const std::string& utf8text) const;
    S32 getWidth(const llwchar* wchars) const;
    S32 getWidth(const std::string& utf8text, S32 offset, S32 max_chars) const;
    S32 getWidth(const llwchar* wchars, S32 offset, S32 max_chars) const;
    S32 getWidth(const LLWString& wchars) const { return getWidth(wchars.c_str()); }
    S32 getWidth(const LLWString& wchars, S32 offset, S32 max_chars) const { return getWidth(wchars.c_str(), offset, max_chars); }
    S32 getWidth(const std::string& utf8str, S32 offset, S32 max_chars, BOOL use_embedded) const;
    S32 getWidth(const LLWString& utf32str, S32 offset, S32 max_chars, BOOL use_embedded) const;
    F32 getWidthF32(const std::string& utf8text) const;
    F32 getWidthF32(const llwchar* wchars) const;
    F32 getWidthF32(const std::string& text, S32 offset, S32 max_chars) const;
    F32 getWidthF32(const llwchar* wchars, S32 offset, S32 max_chars) const;
    F32 getWidthF32(const llwchar* wchars, S32 offset, S32 max_chars, BOOL use_embedded) const;
    F32 getWidthF32(const LLWString& wchars) const { return getWidthF32(wchars.c_str()); }
    F32 getWidthF32(const LLWString& wchars, S32 offset, S32 max_chars) const { return getWidthF32(wchars.c_str(), offset, max_chars); }
    F32 getWidthF32(const std::string& utf8str, S32 offset, S32 max_chars, BOOL use_embedded) const;
    F32 getWidthF32(const LLWString& utf32str, S32 offset, S32 max_chars, BOOL use_embedded) const;
    typedef enum e_word_wrap_style
    {
        ONLY_WORD_BOUNDARIES,
        WORD_BOUNDARY_IF_POSSIBLE,
        ANYWHERE
    } EWordWrapStyle ;
    S32 maxDrawableChars(const llwchar* wchars, F32 max_pixels, S32 max_chars = S32_MAX, EWordWrapStyle end_on_word_boundary = ANYWHERE) const;
    S32 maxDrawableChars(const LLWString& wchars, F32 max_pixels, S32 max_chars = S32_MAX, EWordWrapStyle end_on_word_boundary = ANYWHERE) const
    {
        return maxDrawableChars(wchars.c_str(), max_pixels, max_chars, end_on_word_boundary);
    }
    S32 maxDrawableChars(const LLWString& utf32str, F32 max_pixels, S32 max_chars, EWordWrapStyle end_on_word_boundary, BOOL use_embedded, F32* drawn_pixels = NULL) const;
    S32 firstDrawableChar(const llwchar* wchars, F32 max_pixels, S32 text_len, S32 start_pos=S32_MAX, S32 max_chars = S32_MAX) const;
    S32 firstDrawableChar(const LLWString& wchars, F32 max_pixels, S32 start_pos) const
    {
        return firstDrawableChar(wchars.c_str(), max_pixels, (S32)wchars.length(), start_pos);
    }
    S32 charFromPixelOffset(const llwchar* wchars, S32 char_offset, F32 x, F32 max_pixels=F32_MAX, S32 max_chars = S32_MAX, bool round = true) const;
    S32 charFromPixelOffset(const llwchar* wchars, S32 char_offset, F32 x, F32 max_pixels, S32 max_chars, BOOL round, BOOL use_embedded) const;
    const LLFontDescriptor& getFontDesc() const;
    void addEmbeddedChar(llwchar wc, LLTexture* image, const std::string& label) const;
    void addEmbeddedChar(llwchar wc, LLTexture* image, const LLWString& label) const;
    void removeEmbeddedChar(llwchar wc) const;
    void generateASCIIglyphs();
    static void initClass(F32 screen_dpi, F32 x_scale, F32 y_scale, const std::string& app_dir, bool create_gl_textures = true);
           void dumpTextures();
    static void dumpFonts();
    static void dumpFontTextures();
    static bool loadDefaultFonts();
    static void loadCommonFonts();
    static void destroyDefaultFonts();
    static void destroyAllGL();
    static U8 getStyleFromString(const std::string &style);
    static std::string getStringFromStyle(U8 style);
    static std::string nameFromFont(const LLFontGL* fontp);
    static std::string sizeFromFont(const LLFontGL* fontp);
    static std::string nameFromHAlign(LLFontGL::HAlign align);
    static LLFontGL::HAlign hAlignFromName(const std::string& name);
    static std::string nameFromVAlign(LLFontGL::VAlign align);
    static LLFontGL::VAlign vAlignFromName(const std::string& name);
    static void setFontDisplay(bool flag) { sDisplayFont = flag; }
    static LLFontGL* getFontEmojiSmall();
    static LLFontGL* getFontEmojiMedium();
    static LLFontGL* getFontEmojiLarge();
    static LLFontGL* getFontEmojiHuge();
    static LLFontGL* getFontMonospace();
    static LLFontGL* getFontSansSerifSmall();
    static LLFontGL* getFontSansSerifSmallBold();
    static LLFontGL* getFontSansSerifSmallItalic();
    static LLFontGL* getFontSansSerif();
    static LLFontGL* getFontSansSerifMedium();
    static LLFontGL* getFontSansSerifBig();
    static LLFontGL* getFontSansSerifHuge();
    static LLFontGL* getFontSansSerifBold();
    static LLFontGL* getFontExtChar();
    static LLFontGL* getFont(const LLFontDescriptor& desc);
    static LLFontGL* getFontByName(const std::string& name);
    static LLFontGL* getFontDefault();
    static std::string getFontPathLocal();
    static std::string getFontPathSystem();
    static LLCoordGL sCurOrigin;
    static F32          sCurDepth;
    static std::vector<std::pair<LLCoordGL, F32> > sOriginStack;
    static LLColor4 sShadowColor;
    static F32 sVertDPI;
    static F32 sHorizDPI;
    static F32 sScaleX;
    static F32 sScaleY;
    static S32 sResolutionGeneration;
    static bool sDisplayFont ;
    static std::string sAppDir;
private:
    friend class LLFontRegistry;
    friend class LLTextBillboard;
    friend class LLHUDText;
    struct embedded_data_t
    {
        embedded_data_t(LLImageGL* image, const LLWString& label) : mImage(image), mLabel(label) {}
        LLPointer<LLImageGL> mImage;
        LLWString            mLabel;
    };
    const embedded_data_t* getEmbeddedCharData(const llwchar wch) const;
    F32 getEmbeddedCharAdvance(const embedded_data_t* ext_data) const;
    void clearEmbeddedChars();
    LLFontGL(const LLFontGL &source);
    LLFontGL &operator=(const LLFontGL &source);
    LLFontDescriptor mFontDescriptor;
    LLPointer<LLFontFreetype> mFontFreetype;
    typedef std::map<llwchar, embedded_data_t*> embedded_map_t;
    mutable embedded_map_t mEmbeddedChars;
    void renderTriangle(LLVector4a* vertex_out, LLVector2* uv_out, LLColor4U* colors_out, const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4U& color, F32 slant_amt) const;
    void drawGlyph(S32& glyph_count, LLVector4a* vertex_out, LLVector2* uv_out, LLColor4U* colors_out, const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4U& color, U8 style, ShadowType shadow, F32 drop_shadow_fade) const;
    static LLFontRegistry* sFontRegistry;
};
#endif
