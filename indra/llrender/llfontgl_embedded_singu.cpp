#include "linden_common.h"
#include "llfontgl.h"
#include "llfontfreetype.h"
#include "lltexture.h"
const F32 EXT_X_BEARING = 1.f;
const F32 EXT_Y_BEARING = 0.f;
const F32 EXT_KERNING = 1.f;
S32 LLFontGL::getWidth(const std::string& utf8text, const S32 begin_offset, const S32 max_chars, BOOL use_embedded) const
{
	return getWidth(utf8str_to_wstring(utf8text), begin_offset, max_chars, use_embedded);
}
S32 LLFontGL::getWidth(const LLWString& utf32text, const S32 begin_offset, const S32 max_chars, BOOL use_embedded) const
{
	F32 width = getWidthF32(utf32text, begin_offset, max_chars, use_embedded);
	return ll_pos_round(width);
}
F32 LLFontGL::getWidthF32(const std::string& utf8text, const S32 begin_offset, const S32 max_chars, BOOL use_embedded) const
{
	return getWidthF32(utf8str_to_wstring(utf8text), begin_offset, max_chars, use_embedded);
}
F32 LLFontGL::getWidthF32(const LLWString& utf32text, const S32 begin_offset, const S32 max_chars, BOOL use_embedded) const
{
	const S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;
	const S32 max_index = llmin(llmax(max_chars, begin_offset + max_chars), S32(utf32text.length()));
	if (max_index <= 0 || begin_offset >= max_index)
		return 0;
	F32 cur_x = 0;
	const LLFontGlyphInfo* next_glyph = NULL;
	F32 width_padding = 0.f;
	for (S32 i = begin_offset; i < max_index; i++)
	{
		const llwchar wch = utf32text[i];
		const embedded_data_t* ext_data = use_embedded ? getEmbeddedCharData(wch) : NULL;
		if (ext_data)
		{
			cur_x += getEmbeddedCharAdvance(ext_data);
			if(i+1 < max_index)
			{
				cur_x += EXT_KERNING * sScaleX;
			}
		}
		else
		{
			const LLFontGlyphInfo* fgi = next_glyph;
			next_glyph = NULL;
			if(!fgi)
			{
				fgi = mFontFreetype->getGlyphInfo(wch, EFontGlyphType::Unspecified);
			}
			F32 advance = mFontFreetype->getXAdvance(fgi);
			width_padding = llmax(	0.f,
									width_padding - advance,
									(F32)(fgi->mWidth + fgi->mXBearing) - advance);
			cur_x += advance;
			if ((i + 1) < max_index)
			{
				llwchar next_char = utf32text[i+1];
				if (next_char < LAST_CHARACTER)
				{
					next_glyph = mFontFreetype->getGlyphInfo(next_char, EFontGlyphType::Unspecified);
					cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
				}
			}
			cur_x = (F32)ll_pos_round(cur_x);
		}
	}
	cur_x += width_padding;
	return cur_x / sScaleX;
}
S32 LLFontGL::maxDrawableChars(const LLWString& utf32text, F32 max_pixels, S32 max_chars,
							   EWordWrapStyle end_on_word_boundary, const BOOL use_embedded,
							   F32* drawn_pixels) const
{
	const S32 max_index = llmin(max_chars, S32(utf32text.length()));
	if (max_index <= 0 || max_pixels <= 0.f)
		return 0;
	BOOL clip = FALSE;
	F32 cur_x = 0;
	F32 drawn_x = 0;
	S32 start_of_last_word = 0;
	BOOL in_word = FALSE;
	F32 scaled_max_pixels =	max_pixels * sScaleX;
	F32 width_padding = 0.f;
	LLFontGlyphInfo* next_glyph = NULL;
	S32 i;
	for (i=0; (i < max_index); i++)
	{
		llwchar wch = utf32text[i];
		const embedded_data_t* ext_data = use_embedded ? getEmbeddedCharData(wch) : NULL;
		if (ext_data)
		{
			if (in_word)
			{
				in_word = FALSE;
			}
			else
			{
				start_of_last_word = i;
			}
			cur_x += getEmbeddedCharAdvance(ext_data);
			if (scaled_max_pixels < cur_x)
			{
				clip = TRUE;
				break;
			}
			if ((i+1) < max_index)
			{
				cur_x += EXT_KERNING * sScaleX;
			}
			if( scaled_max_pixels < cur_x )
			{
				clip = TRUE;
				break;
			}
		}
		else
		{
			if (in_word)
			{
				if (iswspace(wch))
				{
					in_word = FALSE;
				}
			}
			else
			{
				start_of_last_word = i;
				if (!iswspace(wch))
				{
					in_word = TRUE;
				}
			}
			LLFontGlyphInfo* fgi = next_glyph;
			next_glyph = NULL;
			if(!fgi)
			{
				fgi = mFontFreetype->getGlyphInfo(wch, EFontGlyphType::Unspecified);
			}
			width_padding = llmax(	0.f,
									width_padding - fgi->mXAdvance,
									(F32)(fgi->mWidth + fgi->mXBearing) - fgi->mXAdvance);
			cur_x += fgi->mXAdvance;
			if (scaled_max_pixels < cur_x + width_padding)
			{
				clip = TRUE;
				break;
			}
			if ((i+1) < max_index)
			{
				next_glyph = mFontFreetype->getGlyphInfo(utf32text[i + 1], EFontGlyphType::Unspecified);
				cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
			}
		}
		cur_x = (F32)ll_pos_round(cur_x);
		drawn_x = cur_x;
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
	if (drawn_pixels)
	{
		*drawn_pixels = drawn_x;
	}
	return i;
}
S32 LLFontGL::charFromPixelOffset(const llwchar* utf32text, const S32 begin_offset, F32 target_x, F32 max_pixels, S32 max_chars, BOOL round, BOOL use_embedded) const
{
	S32 text_len = begin_offset;
	while (utf32text[text_len])
	{
		++text_len;
	}
	const S32 max_index = llmin(llmax(max_chars, begin_offset + max_chars), text_len);
	if (max_index <= 0 || begin_offset >= max_index || max_pixels <= 0.f)
		return 0;
	F32 cur_x = 0;
	target_x *= sScaleX;
	F32 scaled_max_pixels =	max_pixels * sScaleX;
	const LLFontGlyphInfo* next_glyph = NULL;
	S32 pos;
	for (pos = begin_offset; pos < max_index; pos++)
	{
		llwchar wch = utf32text[pos];
		if (!wch)
		{
			break;
		}
		const embedded_data_t* ext_data = use_embedded ? getEmbeddedCharData(wch) : NULL;
		const LLFontGlyphInfo* glyph = next_glyph;
		next_glyph = NULL;
		if(!glyph && !ext_data)
		{
			glyph = mFontFreetype->getGlyphInfo(wch, EFontGlyphType::Unspecified);
		}
		F32 char_width = ext_data ? getEmbeddedCharAdvance(ext_data) : mFontFreetype->getXAdvance(glyph);
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
		if ((pos + 1) < max_index)
		{
			if(ext_data)
			{
				cur_x += EXT_KERNING * sScaleX;
			}
			else
			{
				next_glyph = mFontFreetype->getGlyphInfo(utf32text[pos + 1], EFontGlyphType::Unspecified);
				cur_x += mFontFreetype->getXKerning(glyph, next_glyph);
			}
		}
		cur_x = (F32)ll_pos_round(cur_x);
	}
	return pos - begin_offset;
}
const LLFontGL::embedded_data_t* LLFontGL::getEmbeddedCharData(const llwchar wch) const
{
	embedded_map_t::const_iterator iter = mEmbeddedChars.find(wch);
	if (iter != mEmbeddedChars.end())
	{
		return iter->second;
	}
	return NULL;
}
F32 LLFontGL::getEmbeddedCharAdvance(const embedded_data_t* ext_data) const
{
	const LLWString& label = ext_data->mLabel;
	LLImageGL* ext_image = ext_data->mImage;
	F32 ext_width = (F32)ext_image->getWidth();
	if( !label.empty() )
	{
		ext_width += (EXT_X_BEARING + getFontExtChar()->getWidthF32(label.c_str())) * sScaleX;
	}
	return (EXT_X_BEARING * sScaleX) + ext_width;
}
void LLFontGL::clearEmbeddedChars()
{
	for_each(mEmbeddedChars.begin(), mEmbeddedChars.end(), DeletePairedPointer());
	mEmbeddedChars.clear();
}
void LLFontGL::addEmbeddedChar( llwchar wc, LLTexture* image, const std::string& label ) const
{
	LLWString wlabel = utf8str_to_wstring(label);
	addEmbeddedChar(wc, image, wlabel);
}
void LLFontGL::addEmbeddedChar( llwchar wc, LLTexture* image, const LLWString& wlabel ) const
{
	embedded_data_t* ext_data = new embedded_data_t(image->getGLTexture(), wlabel);
	mEmbeddedChars[wc] = ext_data;
}
void LLFontGL::removeEmbeddedChar( llwchar wc ) const
{
	embedded_map_t::iterator iter = mEmbeddedChars.find(wc);
	if (iter != mEmbeddedChars.end())
	{
		delete iter->second;
		mEmbeddedChars.erase(wc);
	}
}
LLFontGL* LLFontGL::getFontExtChar()
{
    return getFontSansSerif();
}
