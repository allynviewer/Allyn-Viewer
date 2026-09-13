/** 
 * @file llstring.cpp
 * @brief String utility functions and the std::string class.
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
#include "llstring.h"
#include "llerror.h"
#include "llfasttimer.h"
#include "llsd.h"
#if LL_WINDOWS
#include "llwin32headerslean.h"
#include <winnls.h>
#endif
LLTrace::BlockTimerStatHandle FT_STRING_FORMAT("String Format");
std::string ll_safe_string(const char* in)
{
	if(in) return std::string(in);
	return std::string();
}
std::string ll_safe_string(const char* in, S32 maxlen)
{
	if(in && maxlen > 0 ) return std::string(in, maxlen);
	return std::string();
}
bool is_char_hex(char hex)
{
	if((hex >= '0') && (hex <= '9'))
	{
		return true;
	}
	else if((hex >= 'a') && (hex <='f'))
	{
		return true;
	}
	else if((hex >= 'A') && (hex <='F'))
	{
		return true;
	}
	return false;
}
U8 hex_as_nybble(char hex)
{
	if((hex >= '0') && (hex <= '9'))
	{
		return (U8)(hex - '0');
	}
	else if((hex >= 'a') && (hex <='f'))
	{
		return (U8)(10 + hex - 'a');
	}
	else if((hex >= 'A') && (hex <='F'))
	{
		return (U8)(10 + hex - 'A');
	}
	return 0;
}
bool iswindividual(llwchar elem)
{
	U32 cur_char = (U32)elem;
	bool result = false;
	if (0x2E80<= cur_char && cur_char <= 0x9FFF)
	{
		result = true;
	}
	else if (0xAC00<= cur_char && cur_char <= 0xD7A0 )
	{
		result = true;
	}
	else if (0xF900<= cur_char && cur_char <= 0xFA60 )
	{
		result = true;
	}
	return result;
}
bool _read_file_into_string(std::string& str, const std::string& filename)
{
	llifstream ifs(filename.c_str(), llifstream::binary);
	if (!ifs.is_open())
	{
		LL_INFOS() << "Unable to open file " << filename << LL_ENDL;
		return false;
	}
	std::ostringstream oss;
	oss << ifs.rdbuf();
	str = oss.str();
	ifs.close();
	return true;
}
std::ostream& operator<<(std::ostream &s, const LLWString &wstr)
{
	std::string utf8_str = wstring_to_utf8str(wstr);
	s << utf8_str;
	return s;
}
std::string rawstr_to_utf8(const std::string& raw)
{
	LLWString wstr(utf8str_to_wstring(raw));
	return wstring_to_utf8str(wstr);
}
S32 wchar_to_utf8chars(llwchar in_char, char* outchars)
{
	U32 cur_char = (U32)in_char;
	char* base = outchars;
	if (cur_char < 0x80)
	{
		*outchars++ = (U8)cur_char;
	}
	else if (cur_char < 0x800)
	{
		*outchars++ = 0xC0 | (cur_char >> 6);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x10000)
	{
		*outchars++ = 0xE0 | (cur_char >> 12);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x200000)
	{
		*outchars++ = 0xF0 | (cur_char >> 18);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x4000000)
	{
		*outchars++ = 0xF8 | (cur_char >> 24);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x80000000)
	{
		*outchars++ = 0xFC | (cur_char >> 30);
		*outchars++ = 0x80 | ((cur_char >> 24) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else
	{
		LL_WARNS() << "Invalid Unicode character " << cur_char << "!" << LL_ENDL;
		*outchars++ = LL_UNKNOWN_CHAR;
	}
	return outchars - base;
}
S32 utf16chars_to_wchar(const utf16strtype* inchars, llwchar* outchar)
{
	const utf16strtype* base = inchars;
	utf16strtype cur_char = *inchars++;
	llwchar char32 = cur_char;
	if ((cur_char >= 0xD800) && (cur_char <= 0xDFFF))
	{
		char32 = ((llwchar)(cur_char - 0xD800)) << 10;
		cur_char = *inchars++;
		char32 += (llwchar)(cur_char - 0xDC00) + 0x0010000UL;
	}
	else
	{
		char32 = (llwchar)cur_char;
	}
	*outchar = char32;
	return inchars - base;
}
llutf16string wstring_to_utf16str(const LLWString &utf32str, S32 len)
{
	llutf16string out;
	S32 i = 0;
	while (i < len)
	{
		U32 cur_char = utf32str[i];
		if (cur_char > 0xFFFF)
		{
			out += (0xD7C0 + (cur_char >> 10));
			out += (0xDC00 | (cur_char & 0x3FF));
		}
		else
		{
			out += cur_char;
		}
		++i;
	}
	return out;
}
llutf16string wstring_to_utf16str(const LLWString &utf32str)
{
	const S32 len = (S32)utf32str.length();
	return wstring_to_utf16str(utf32str, len);
}
llutf16string utf8str_to_utf16str ( const std::string& utf8str )
{
	LLWString wstr = utf8str_to_wstring ( utf8str );
	return wstring_to_utf16str ( wstr );
}
LLWString utf16str_to_wstring(const llutf16string &utf16str, S32 len)
{
	LLWString wout;
	if((len <= 0) || utf16str.empty()) return wout;
	S32 i = 0;
	const utf16strtype* chars16 = &(*(utf16str.begin()));
	while (i < len)
	{
		llwchar cur_char;
		i += utf16chars_to_wchar(chars16+i, &cur_char);
		wout += cur_char;
	}
	return wout;
}
LLWString utf16str_to_wstring(const llutf16string &utf16str)
{
	const S32 len = (S32)utf16str.length();
	return utf16str_to_wstring(utf16str, len);
}
S32 utf16str_wstring_length(const llutf16string &utf16str, const S32 utf16_len)
{
	S32 surrogate_pairs = 0;
	const utf16strtype *const utf16_chars = &(*(utf16str.begin()));
	S32 i = 0;
	while (i < utf16_len)
	{
		const utf16strtype c = utf16_chars[i++];
		if (c >= 0xD800 && c <= 0xDBFF)
		{
			if (i >= utf16_len)
			{
				break;
			}
			const utf16strtype d = utf16_chars[i];
			if (d >= 0xDC00 && d <= 0xDFFF)
			{
				surrogate_pairs++;
				i++;
			}
		}
	}
	return utf16_len - surrogate_pairs;
}
S32 wstring_utf16_length(const LLWString &wstr, const S32 woffset, const S32 wlen)
{
	const S32 end = llmin((S32)wstr.length(), woffset + wlen);
	if (end < woffset)
	{
		return 0;
	}
	else
	{
		S32 length = end - woffset;
		for (S32 i = woffset; i < end; i++)
		{
			if (wstr[i] >= 0x10000)
			{
				length++;
			}
		}
		return length;
	}
}
S32 wstring_wstring_length_from_utf16_length(const LLWString & wstr, const S32 woffset, const S32 utf16_length, BOOL *unaligned)
{
	const S32 end = wstr.length();
	BOOL u = FALSE;
	S32 n = woffset + utf16_length;
	S32 i = woffset;
	while (i < end)
	{
		if (wstr[i] >= 0x10000)
		{
			--n;
		}
		if (i >= n)
		{
			u = (i > n);
			break;
		}
		i++;
	}
	if (unaligned)
	{
		*unaligned = u;
	}
	return i - woffset;
}
S32 wchar_utf8_length(const llwchar wc)
{
	if (wc < 0x80)
	{
		return 1;
	}
	else if (wc < 0x800)
	{
		return 2;
	}
	else if (wc < 0x10000)
	{
		return 3;
	}
	else if (wc < 0x200000)
	{
		return 4;
	}
	else if (wc < 0x4000000)
	{
		return 5;
	}
	else
	{
		return 6;
	}
}
S32 wstring_utf8_length(const LLWString& wstr)
{
	S32 len = 0;
	for (S32 i = 0; i < (S32)wstr.length(); i++)
	{
		len += wchar_utf8_length(wstr[i]);
	}
	return len;
}
LLWString utf8str_to_wstring(const std::string& utf8str, S32 len)
{
	llwchar unichar;
	LLWString wout;
	wout.reserve(len);
	for (S32 i = 0; i < len; i++)
	{
		U8 cur_char = utf8str[i];
		if (cur_char < 0x80)
		{
			unichar = cur_char;
		}
		else
		{
			S32 cont_bytes = 0;
			if ((cur_char >> 5) == 0x6)
			{
				unichar = (0x1F&cur_char);
				cont_bytes = 1;
			}
			else if ((cur_char >> 4) == 0xe)
			{
				unichar = (0x0F&cur_char);
				cont_bytes = 2;
			}
			else if ((cur_char >> 3) == 0x1e)
			{
				unichar = (0x07&cur_char);
				cont_bytes = 3;
			}
			else if ((cur_char >> 2) == 0x3e)
			{
				unichar = (0x03&cur_char);
				cont_bytes = 4;
			}
			else if ((cur_char >> 1) == 0x7e)
			{
				unichar = (0x01&cur_char);
				cont_bytes = 5;
			}
			else
			{
				wout += LL_UNKNOWN_CHAR;
				++i;
				continue;
			}
			S32 end = (len < (i + cont_bytes)) ? len : (i + cont_bytes);
			do
			{
				++i;
				cur_char = utf8str[i];
				if ( (cur_char >> 6) == 0x2 )
				{
					unichar <<= 6;
					unichar += (0x3F&cur_char);
				}
				else
				{
					unichar = LL_UNKNOWN_CHAR;
					--i;
					break;
				}
			} while(i < end);
			if ( ((cont_bytes == 1) && (unichar < 0x80))
				|| ((cont_bytes == 2) && (unichar < 0x800))
				|| ((cont_bytes == 3) && (unichar < 0x10000))
				|| ((cont_bytes == 4) && (unichar < 0x200000))
				|| ((cont_bytes == 5) && (unichar < 0x4000000)) )
			{
				unichar = LL_UNKNOWN_CHAR;
			}
		}
		wout += unichar;
	}
	return wout;
}
LLWString utf8str_to_wstring(const std::string& utf8str)
{
	const S32 len = (S32)utf8str.length();
	return utf8str_to_wstring(utf8str, len);
}
std::string wstring_to_utf8str(const LLWString& utf32str, S32 len)
{
	char tchars[8];
	std::string out;
	out.reserve(len);
	for (S32 i = 0; i < len; ++i)
	{
		S32 n = wchar_to_utf8chars(utf32str[i], tchars);
		tchars[n] = 0;
		out += tchars;
	}
	return out;
}
std::string wstring_to_utf8str(const LLWString& utf32str)
{
	const S32 len = (S32)utf32str.length();
	return wstring_to_utf8str(utf32str, len);
}
std::string utf16str_to_utf8str(const llutf16string& utf16str)
{
	return wstring_to_utf8str(utf16str_to_wstring(utf16str));
}
std::string utf16str_to_utf8str(const llutf16string& utf16str, S32 len)
{
	return wstring_to_utf8str(utf16str_to_wstring(utf16str, len), len);
}
std::string utf8str_trim(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::trim(wstr);
	return wstring_to_utf8str(wstr);
}
std::string utf8str_tolower(const std::string& utf8str)
{
	LLWString out_str = utf8str_to_wstring(utf8str);
	LLWStringUtil::toLower(out_str);
	return wstring_to_utf8str(out_str);
}
S32 utf8str_compare_insensitive(const std::string& lhs, const std::string& rhs)
{
	LLWString wlhs = utf8str_to_wstring(lhs);
	LLWString wrhs = utf8str_to_wstring(rhs);
	return LLWStringUtil::compareInsensitive(wlhs, wrhs);
}
std::string utf8str_truncate(const std::string& utf8str, const S32 max_len)
{
	if (0 == max_len)
	{
		return std::string();
	}
	if ((S32)utf8str.length() <= max_len)
	{
		return utf8str;
	}
	else
	{
		S32 cur_char = max_len;
		if ((U8)utf8str[cur_char] > 0x7f)
		{
			while (0x80 == (0xc0 & utf8str[cur_char]))
			{
				cur_char--;
				if (cur_char == 0)
				{
					break;
				}
			}
		}
		return utf8str.substr(0, cur_char);
	}
}
std::string utf8str_substr(const std::string& utf8str, const size_t index, const size_t max_len)
{
	if (0 == max_len)
	{
		return std::string();
	}
	if (utf8str.length() - index  <= max_len)
	{
		return utf8str.substr(index, max_len);
	}
	else
	{
		size_t cur_char = max_len;
		if ((U8)utf8str[index + cur_char] > 0x7f)
		{
			while (0x80 == (0xc0 & utf8str[index + cur_char]))
			{
				cur_char--;
				if (cur_char == 0)
				{
					break;
				}
			}
		}
		return utf8str.substr(index, cur_char);
	}
}
void utf8str_split(std::list<std::string>& split_list, const std::string& utf8str, size_t maxlen, char split_token)
{
	split_list.clear();
	std::string::size_type lenMsg = utf8str.length(), lenIt = 0;
	const char* pstrIt = utf8str.c_str(); std::string strTemp;
	while (lenIt < lenMsg)
	{
		if (lenIt + maxlen < lenMsg)
		{
			const char* pstrTemp = pstrIt + maxlen;
			while ( (pstrTemp > pstrIt) && (*pstrTemp != split_token) )
				pstrTemp--;
			if (pstrTemp > pstrIt)
				strTemp = utf8str.substr(lenIt, pstrTemp - pstrIt);
			else
				strTemp = utf8str_substr(utf8str, lenIt, maxlen);
		}
		else
		{
			strTemp = utf8str.substr(lenIt, std::string::npos);
		}
		split_list.push_back(strTemp);
		lenIt += strTemp.length();
		pstrIt = utf8str.c_str() + lenIt;
		if (*pstrIt == split_token)
			lenIt++;
	}
}
std::string utf8str_symbol_truncate(const std::string& utf8str, const size_t symbol_len)
{
    if (0 == symbol_len)
    {
        return std::string();
    }
    if ((S32)utf8str.length() <= symbol_len)
    {
        return utf8str;
    }
    else
    {
        size_t len = 0, byteIndex = 0;
        const char* aStr = utf8str.c_str();
        size_t origSize = utf8str.size();
        for (byteIndex = 0; len < symbol_len && byteIndex < origSize; byteIndex++)
        {
            if ((aStr[byteIndex] & 0xc0) != 0x80)
            {
                len += 1;
            }
        }
        return utf8str.substr(0, byteIndex);
    }
}
std::string utf8str_substChar(
	const std::string& utf8str,
	const llwchar target_char,
	const llwchar replace_char)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::replaceChar(wstr, target_char, replace_char);
	return wstring_to_utf8str(wstr);
}
std::string utf8str_makeASCII(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::_makeASCII(wstr);
	return wstring_to_utf8str(wstr);
}
std::string mbcsstring_makeASCII(const std::string& wstr)
{
	std::string out_str = wstr;
	for (S32 i = 0; i < (S32)out_str.length(); i++)
	{
		if ((U8)out_str[i] > 0x7f)
		{
			out_str[i] = LL_UNKNOWN_CHAR;
		}
	}
	return out_str;
}
bool wstring_has_emoji(const LLWString& wstr)
{
	for (size_t i = 0; i < wstr.size(); ++i)
	{
		if (LLStringOps::isEmoji(wstr[i]))
			return true;
	}
	return false;
}
bool wstring_remove_emojis(LLWString& wstr)
{
	bool found = false;
	for (size_t i = 0; i < wstr.size(); ++i)
	{
		if (LLStringOps::isEmoji(wstr[i]))
		{
			wstr.erase(i--, 1);
			found = true;
		}
	}
	return found;
}
bool utf8str_remove_emojis(std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	if (!wstring_remove_emojis(wstr))
		return false;
	utf8str = wstring_to_utf8str(wstr);
	return true;
}
std::string utf8str_removeCRLF(const std::string& utf8str)
{
	if (0 == utf8str.length())
	{
		return std::string();
	}
	const char CR = 13;
	std::string out;
	out.reserve(utf8str.length());
	const S32 len = (S32)utf8str.length();
	for( S32 i = 0; i < len; i++ )
	{
		if( utf8str[i] != CR )
		{
			out.push_back(utf8str[i]);
		}
	}
	return out;
}
bool LLStringOps::isHexString(const std::string& str)
{
	const char* buf = str.c_str();
	int len = str.size();
	while (--len >= 0)
	{
		if (!isxdigit(buf[len])) return false;
	}
	return true;
}
#if LL_WINDOWS
std::string ll_convert_wide_to_string(const wchar_t* in)
{
	return ll_convert_wide_to_string(in, CP_UTF8);
}
std::string ll_convert_wide_to_string(const wchar_t* in, unsigned int code_page)
{
	std::string out;
	if(in)
	{
		int len_in = wcslen(in);
		int len_out = WideCharToMultiByte(
			code_page,
			0,
			in,
			len_in,
			nullptr,
			0,
			nullptr,
			nullptr);
		char* pout = new char [len_out + 2];
		memset(pout, 0, len_out + 2);
		if(pout)
		{
			WideCharToMultiByte(
				code_page,
				0,
				in,
				len_in,
				pout,
				len_out,
				nullptr,
				nullptr);
			out.assign(pout);
			delete[] pout;
		}
	}
	return out;
}
wchar_t* ll_convert_string_to_wide(const std::string& in)
{
	return ll_convert_string_to_wide(in, CP_UTF8);
}
wchar_t* ll_convert_string_to_wide(const std::string& in, unsigned int code_page)
{
	int output_str_len = in.length();
	wchar_t* w_out = new wchar_t[output_str_len + 1];
	memset(w_out, 0, output_str_len + 1);
	int real_output_str_len = MultiByteToWideChar (code_page, 0, in.c_str(), in.length(), w_out, output_str_len);
	w_out[real_output_str_len] = 0;
	return w_out;
}
S32 wchartchars_to_llwchar(const std::wstring::value_type* inchars, llwchar* outchar)
{
	const std::wstring::value_type* base = inchars;
	std::wstring::value_type cur_char = *inchars++;
	llwchar char32 = cur_char;
	if ((cur_char >= 0xD800) && (cur_char <= 0xDFFF))
	{
		char32 = ((llwchar)(cur_char - 0xD800)) << 10;
		cur_char = *inchars++;
		char32 += (llwchar)(cur_char - 0xDC00) + 0x0010000UL;
	}
	else
	{
		char32 = (llwchar)cur_char;
	}
	*outchar = char32;
	return inchars - base;
}
LLWString ll_convert_wide_to_wstring(const std::wstring& in)
{
	LLWString wout;
	auto len = in.size();
	if ((len <= 0) || in.empty()) return wout;
	size_t i = 0;
	const std::wstring::value_type* chars16 = &(*(in.begin()));
	while (i < len)
	{
		llwchar cur_char;
		i += wchartchars_to_llwchar(chars16 + i, &cur_char);
		wout += cur_char;
	}
	return wout;
}
std::wstring ll_convert_wstring_to_wide(const LLWString& in)
{
	std::wstring out;
	size_t i = 0;
	while (i < in.size())
	{
		U32 cur_char = in[i];
		if (cur_char > 0xFFFF)
		{
			out += (0xD7C0 + (cur_char >> 10));
			out += (0xDC00 | (cur_char & 0x3FF));
		}
		else
		{
			out += cur_char;
		}
		i++;
	}
	return out;
}
std::string ll_convert_string_to_utf8_string(const std::string& in)
{
	wchar_t* w_mesg = ll_convert_string_to_wide(in, CP_ACP);
	std::string out_utf8(ll_convert_wide_to_string(w_mesg, CP_UTF8));
	delete[] w_mesg;
	return out_utf8;
}
namespace
{
void HeapFree_deleter(void* ptr)
{
    HeapFree(GetProcessHeap(), NULL, ptr);
}
}
template<>
std::wstring windows_message<std::wstring>(DWORD error)
{
    wchar_t* rawptr = nullptr;
    auto okay = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&rawptr,
        0,
        NULL);
    std::unique_ptr<wchar_t, void(*)(void*)> bufferptr(rawptr, HeapFree_deleter);
    if (okay && bufferptr)
    {
        return { bufferptr.get(), okay };
    }
    auto format_message_error = GetLastError();
    std::wostringstream out;
    out << L"GetLastError() " << error << L" (FormatMessageW() failed with "
        << format_message_error << L")";
    return out.str();
}
boost::optional<std::wstring> llstring_getoptenv(const std::string& key)
{
    auto wkey = ll_convert_string_to_wide(key);
    std::vector<wchar_t> buffer(1024);
    auto n = GetEnvironmentVariableW(wkey, &buffer[0], buffer.size());
    if (n > (buffer.size() - 1))
    {
        buffer.resize(n);
        n = GetEnvironmentVariableW(wkey, &buffer[0], buffer.size());
    }
    if (n)
    {
        return boost::optional<std::wstring>(&buffer[0]);
    }
    auto last_error = GetLastError();
    if (last_error != ERROR_ENVVAR_NOT_FOUND)
    {
        LL_WARNS() << "GetEnvironmentVariableW('" << key << "') failed: "
                   << windows_message<std::string>(last_error) << LL_ENDL;
    }
    return {};
}
#else
boost::optional<std::string> llstring_getoptenv(const std::string& key)
{
    auto found = getenv(key.c_str());
    if (found)
    {
        return boost::optional<std::string>(found);
    }
    else
    {
        return {};
    }
}
#endif
long LLStringOps::sPacificTimeOffset = 0;
long LLStringOps::sLocalTimeOffset = 0;
bool LLStringOps::sPacificDaylightTime = false;
std::map<std::string, std::string> LLStringOps::datetimeToCodes;
std::vector<std::string> LLStringOps::sWeekDayList;
std::vector<std::string> LLStringOps::sWeekDayShortList;
std::vector<std::string> LLStringOps::sMonthList;
std::vector<std::string> LLStringOps::sMonthShortList;
std::string LLStringOps::sDayFormat;
std::string LLStringOps::sAM;
std::string LLStringOps::sPM;
bool LLStringOps::isEmoji(llwchar a)
{
#if 0
    return a == 0xa9 || a == 0xae || (a >= 0x2000 && a < 0x3300) ||
           (a >= 0x1f000 && a < 0x20000);
#else
    return a >= 0x1f000 && a < 0x20000;
#endif
}
S32	LLStringOps::collate(const llwchar* a, const llwchar* b)
{
	#if LL_WINDOWS
		return strcoll(wstring_to_utf8str(LLWString(a)).c_str(), wstring_to_utf8str(LLWString(b)).c_str());
	#else
		return wcscoll(a, b);
	#endif
}
void LLStringOps::setupDatetimeInfo (bool daylight)
{
	time_t nowT, localT, gmtT;
	struct tm * tmpT;
	nowT = time (nullptr);
	tmpT = gmtime (&nowT);
	gmtT = mktime (tmpT);
	tmpT = localtime (&nowT);
	localT = mktime (tmpT);
	sLocalTimeOffset = (long) (gmtT - localT);
	if (tmpT->tm_isdst)
	{
		sLocalTimeOffset -= 60 * 60;
	}
	sPacificDaylightTime = daylight;
	sPacificTimeOffset = (sPacificDaylightTime? 7 : 8 ) * 60 * 60;
	datetimeToCodes["wkday"]	= "%a";
	datetimeToCodes["weekday"]	= "%A";
	datetimeToCodes["year4"]	= "%Y";
	datetimeToCodes["year"]		= "%Y";
	datetimeToCodes["year2"]	= "%y";
	datetimeToCodes["mth"]		= "%b";
	datetimeToCodes["month"]	= "%B";
	datetimeToCodes["mthnum"]	= "%m";
	datetimeToCodes["day"]		= "%d";
	datetimeToCodes["sday"]		= "%-d";
	datetimeToCodes["hour24"]	= "%H";
	datetimeToCodes["hour"]		= "%H";
	datetimeToCodes["hour12"]	= "%I";
	datetimeToCodes["min"]		= "%M";
	datetimeToCodes["ampm"]		= "%p";
	datetimeToCodes["second"]	= "%S";
	datetimeToCodes["timezone"]	= "%Z";
}
void tokenizeStringToArray(const std::string& data, std::vector<std::string>& output)
{
	output.clear();
	size_t length = data.size();
	std::string cur_word;
	for(size_t i = 0; i < length; ++i)
	{
		if(data[i] == ':')
		{
			output.push_back(cur_word);
			cur_word.clear();
		}
		else
		{
			cur_word.append(1, data[i]);
		}
	}
	output.push_back(cur_word);
}
void LLStringOps::setupWeekDaysNames(const std::string& data)
{
	tokenizeStringToArray(data,sWeekDayList);
}
void LLStringOps::setupWeekDaysShortNames(const std::string& data)
{
	tokenizeStringToArray(data,sWeekDayShortList);
}
void LLStringOps::setupMonthNames(const std::string& data)
{
	tokenizeStringToArray(data,sMonthList);
}
void LLStringOps::setupMonthShortNames(const std::string& data)
{
	tokenizeStringToArray(data,sMonthShortList);
}
void LLStringOps::setupDayFormat(const std::string& data)
{
	sDayFormat = data;
}
std::string LLStringOps::getDatetimeCode(const std::string& key)
{
	std::map<std::string, std::string>::iterator iter;
	iter = datetimeToCodes.find (key);
	if (iter != datetimeToCodes.end())
	{
		return iter->second;
	}
	else
	{
		return std::string();
	}
}
std::string LLStringOps::getReadableNumber(F64 num)
{
    if (fabs(num)>=1e9)
    {
		return llformat("%.2lfB", num / 1e9);
    }
    else if (fabs(num)>=1e6)
    {
		return llformat("%.2lfM", num / 1e6);
    }
    else if (fabs(num)>=1e3)
    {
		return llformat("%.2lfK", num / 1e3);
    }
    else
    {
		return llformat("%.2lf", num);
    }
}
namespace LLStringFn
{
	void replace_nonprintable_in_ascii(std::basic_string<char>& string, char replacement)
	{
		const char MIN = 0x20;
		std::basic_string<char>::size_type len = string.size();
		for(std::basic_string<char>::size_type ii = 0; ii < len; ++ii)
		{
			if(string[ii] < MIN)
			{
				string[ii] = replacement;
			}
		}
	}
	void replace_nonprintable_and_pipe_in_ascii(std::basic_string<char>& str,
									   char replacement)
	{
		const char MIN  = 0x20;
		const char PIPE = 0x7c;
		std::basic_string<char>::size_type len = str.size();
		for(std::basic_string<char>::size_type ii = 0; ii < len; ++ii)
		{
			if( (str[ii] < MIN) || (str[ii] == PIPE) )
			{
				str[ii] = replacement;
			}
		}
	}
	std::string strip_invalid_xml(const std::string& instr)
	{
		std::string output;
		output.reserve( instr.size() );
		std::string::const_iterator it = instr.begin();
		while (it != instr.end())
		{
			const unsigned char c = (unsigned char)*it;
			if (   c >= (unsigned char)0x20
				|| c == (unsigned char)0x09
				|| c == (unsigned char)0x0a
				|| c == (unsigned char)0x0d )
			{
				output.push_back(c);
			}
			++it;
		}
		return output;
	}
	void replace_ascii_controlchars(std::basic_string<char>& string, char replacement)
	{
		const unsigned char MIN = 0x20;
		std::basic_string<char>::size_type len = string.size();
		for(std::basic_string<char>::size_type ii = 0; ii < len; ++ii)
		{
			const unsigned char c = (unsigned char) string[ii];
			if(c < MIN)
			{
				string[ii] = replacement;
			}
		}
	}
}
template<>
S32 LLStringUtil::format(std::string& s, const format_map_t& substitutions);
template<>
void LLStringUtil::getTokens(const std::string& instr, std::vector<std::string >& tokens, const std::string& delims)
{
	for (std::string::size_type begIdx, endIdx = 0;
		 (begIdx = instr.find_first_not_of (delims, endIdx)) != std::string::npos; )
	{
		endIdx = instr.find_first_of (delims, begIdx);
		if (endIdx == std::string::npos)
		{
			endIdx = instr.length();
		}
		std::string currToken(instr.substr(begIdx, endIdx - begIdx));
		LLStringUtil::trim (currToken);
		tokens.push_back(currToken);
	}
}
template<>
LLStringUtil::size_type LLStringUtil::getSubstitution(const std::string& instr, size_type& start, std::vector<std::string>& tokens)
{
	const std::string delims (",");
	size_type pos1 = instr.find('[', start);
	if (pos1 == std::string::npos)
		return std::string::npos;
	size_type pos2 = instr.find(']', pos1);
	if (pos2 == std::string::npos)
		return std::string::npos;
	pos1 = instr.find_last_of('[', pos2-1);
	if (pos1 == std::string::npos || pos1 < start)
		return std::string::npos;
	getTokens(std::string(instr,pos1+1,pos2-pos1-1), tokens, delims);
	start = pos2+1;
	return pos1;
}
template<>
bool LLStringUtil::simpleReplacement(std::string &replacement, std::string token, const format_map_t& substitutions)
{
	format_map_t::const_iterator iter = substitutions.find(token);
	if (iter != substitutions.end())
	{
		replacement = iter->second;
		return true;
	}
	iter = substitutions.find(std::string("[" + token + "]"));
	if (iter != substitutions.end())
	{
		replacement = iter->second;
		return true;
	}
	return false;
}
template<>
bool LLStringUtil::simpleReplacement(std::string &replacement, std::string token, const LLSD& substitutions)
{
	if (substitutions.has(token))
	{
		replacement = substitutions[token].asString();
		return true;
	}
	else if (substitutions.has(std::string("[" + token + "]")))
	{
		replacement = substitutions[std::string("[" + token + "]")].asString();
		return true;
	}
	return false;
}
template<>
void LLStringUtil::setLocale(std::string inLocale)
{
	sLocale = inLocale;
};
template<>
std::string LLStringUtil::getLocale(void)
{
	return sLocale;
};
template<>
void LLStringUtil::formatNumber(std::string& numStr, std::string decimals)
{
	std::stringstream strStream;
	S32 intDecimals = 0;
	convertToS32 (decimals, intDecimals);
	if (!sLocale.empty())
	{
		try
		{
			std::locale locale(sLocale.c_str());
			strStream.imbue(locale);
		} catch (const std::exception &)
		{
			LL_WARNS_ONCE("Locale") << "Cannot set locale to " << sLocale << LL_ENDL;
		}
	}
	if (!intDecimals)
	{
		S32 intStr;
		if (convertToS32(numStr, intStr))
		{
			numStr = fmt::to_string(intStr);
		}
	}
	else
	{
		F32 floatStr;
		if (convertToF32(numStr, floatStr))
		{
			strStream << std::fixed << std::showpoint << std::setprecision(intDecimals) << floatStr;
			numStr = strStream.str();
		}
	}
}
template<>
bool LLStringUtil::formatDatetime(std::string& replacement, std::string token,
								  std::string param, S32 secFromEpoch)
{
	if (param == "local")
	{
		secFromEpoch -= LLStringOps::getLocalTimeOffset();
	}
	else if (param != "utc")
	{
		secFromEpoch -= LLStringOps::getPacificTimeOffset();
	}
	if (secFromEpoch < 0) secFromEpoch = 0;
	LLDate datetime((F64)secFromEpoch);
	std::string code = LLStringOps::getDatetimeCode (token);
	if (code == "%Z") {
		if (param == "utc")
		{
			replacement = "GMT";
		}
		else if (param == "local")
		{
			replacement.clear();
		}
		else
		{
			replacement = LLStringOps::getPacificDaylightTime() ? "PDT" : "PST";
		}
		return true;
	}
	time_t loc_seconds = (time_t) secFromEpoch;
	if(LLStringOps::sWeekDayList.size() == 7 && code == "%A")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = LLStringOps::sWeekDayList[gmt->tm_wday];
	}
	else if(LLStringOps::sWeekDayShortList.size() == 7 && code == "%a")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = LLStringOps::sWeekDayShortList[gmt->tm_wday];
	}
	else if(LLStringOps::sMonthList.size() == 12 && code == "%B")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = LLStringOps::sMonthList[gmt->tm_mon];
	}
	else if( !LLStringOps::sDayFormat.empty() && code == "%d" )
	{
		struct tm * gmt = gmtime (&loc_seconds);
		LLStringUtil::format_map_t args;
		args["[MDAY]"] = fmt::to_string(gmt->tm_mday);
		replacement = LLStringOps::sDayFormat;
		LLStringUtil::format(replacement, args);
	}
	else if (code == "%-d")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = fmt::to_string(gmt->tm_mday);
	}
	else if( !LLStringOps::sAM.empty() && !LLStringOps::sPM.empty() && code == "%p" )
	{
		struct tm * gmt = gmtime (&loc_seconds);
		if(gmt->tm_hour<12)
		{
			replacement = LLStringOps::sAM;
		}
		else
		{
			replacement = LLStringOps::sPM;
		}
	}
	else
	{
		replacement = datetime.toHTTPDateString(code);
	}
	if(code == "%I" && token == "hour12" && replacement.at(0) == '0')
	{
		replacement = replacement.at(1);
	}
	return !code.empty();
}
template<>
S32 LLStringUtil::format(std::string& s, const format_map_t& substitutions)
{
	LL_RECORD_BLOCK_TIME(FT_STRING_FORMAT);
	S32 res = 0;
	std::string output;
	std::vector<std::string> tokens;
	std::string::size_type start = 0;
	std::string::size_type prev_start = 0;
	std::string::size_type key_start = 0;
	while ((key_start = getSubstitution(s, start, tokens)) != std::string::npos)
	{
		output += std::string(s, prev_start, key_start-prev_start);
		prev_start = start;
		bool found_replacement = false;
		std::string replacement;
		if (tokens.size() == 0)
		{
			found_replacement = false;
		}
		else if (tokens.size() == 1)
		{
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
		}
		else if (tokens[1] == "number")
		{
			std::string param = "0";
			if (tokens.size() > 2) param = tokens[2];
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
			if (found_replacement) formatNumber (replacement, param);
		}
		else if (tokens[1] == "datetime")
		{
			std::string param;
			if (tokens.size() > 2) param = tokens[2];
			format_map_t::const_iterator iter = substitutions.find("datetime");
			if (iter != substitutions.end())
			{
				S32 secFromEpoch = 0;
				BOOL r = LLStringUtil::convertToS32(iter->second, secFromEpoch);
				if (r)
				{
					found_replacement = formatDatetime(replacement, tokens[0], param, secFromEpoch);
				}
			}
		}
		if (found_replacement)
		{
			output += replacement;
			res++;
		}
		else
		{
			output += std::string(s, key_start, start-key_start);
		}
		tokens.clear();
	}
	output += std::string(s, start);
	s = output;
	return res;
}
template<>
S32 LLStringUtil::format(std::string& s, const LLSD& substitutions)
{
	LL_RECORD_BLOCK_TIME(FT_STRING_FORMAT);
	S32 res = 0;
	if (!substitutions.isMap())
	{
		return res;
	}
	std::string output;
	std::vector<std::string> tokens;
	std::string::size_type start = 0;
	std::string::size_type prev_start = 0;
	std::string::size_type key_start = 0;
	while ((key_start = getSubstitution(s, start, tokens)) != std::string::npos)
	{
		output += std::string(s, prev_start, key_start-prev_start);
		prev_start = start;
		bool found_replacement = false;
		std::string replacement;
		if (tokens.size() == 0)
		{
			found_replacement = false;
		}
		else if (tokens.size() == 1)
		{
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
		}
		else if (tokens[1] == "number")
		{
			std::string param = "0";
			if (tokens.size() > 2) param = tokens[2];
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
			if (found_replacement) formatNumber (replacement, param);
		}
		else if (tokens[1] == "datetime")
		{
			std::string param;
			if (tokens.size() > 2) param = tokens[2];
			S32 secFromEpoch = (S32) substitutions["datetime"].asInteger();
			found_replacement = formatDatetime (replacement, tokens[0], param, secFromEpoch);
		}
		if (found_replacement)
		{
			output += replacement;
			res++;
		}
		else
		{
			output += std::string(s, key_start, start-key_start);
		}
		tokens.clear();
	}
	output += std::string(s, start);
	s = output;
	return res;
}
#ifdef _DEBUG
template<class T>
void LLStringUtilBase<T>::testHarness()
{
	std::string s1;
	llassert( s1.c_str() == NULL );
	llassert( s1.size() == 0 );
	llassert( s1.empty() );
	std::string s2( "hello");
	llassert( !strcmp( s2.c_str(), "hello" ) );
	llassert( s2.size() == 5 );
	llassert( !s2.empty() );
	std::string s3( s2 );
	llassert( "hello" == s2 );
	llassert( s2 == "hello" );
	llassert( s2 > "gello" );
	llassert( "gello" < s2 );
	llassert( "gello" != s2 );
	llassert( s2 != "gello" );
	std::string s4 = s2;
	llassert( !s4.empty() );
	s4.empty();
	llassert( s4.empty() );
	std::string s5("");
	llassert( s5.empty() );
	llassert( isValidIndex(s5, 0) );
	llassert( !isValidIndex(s5, 1) );
	s3 = s2;
	s4 = "hello again";
	s4 += "!";
	s4 += s4;
	llassert( s4 == "hello again!hello again!" );
	std::string s6 = s2 + " " + s2;
	std::string s7 = s6;
	llassert( s6 == s7 );
	llassert( !( s6 != s7) );
	llassert( !(s6 < s7) );
	llassert( !(s6 > s7) );
	llassert( !(s6 == "hi"));
	llassert( s6 == "hello hello");
	llassert( s6 < "hi");
	llassert( s6[1] == 'e' );
	s6[1] = 'f';
	llassert( s6[1] == 'f' );
	s2.erase( 4, 1 );
	llassert( s2 == "hell");
	s2.insert( s2.begin(), 'y' );
	llassert( s2 == "yhell");
	s2.erase( 1, 3 );
	llassert( s2 == "yl");
	s2.insert( 1, "awn, don't yel");
	llassert( s2 == "yawn, don't yell");
	std::string s8 = s2.substr( 6, 5 );
	llassert( s8 == "don't"  );
	std::string s9 = "   \t\ntest  \t\t\n  ";
	trim(s9);
	llassert( s9 == "test"  );
	s8 = "abc123&*(ABC";
	s9 = s8;
	toUpper(s9);
	llassert( s9 == "ABC123&*(ABC"  );
	s9 = s8;
	toLower(s9);
	llassert( s9 == "abc123&*(abc"  );
	std::string s10( 10, 'x' );
	llassert( s10 == "xxxxxxxxxx" );
	std::string s11( "monkey in the middle", 7, 2 );
	llassert( s11 == "in" );
	std::string s12;
	s12 += "foo";
	llassert( s12 == "foo" );
	std::string s13;
	s13 += 'f';
	llassert( s13 == "f" );
}
#endif
