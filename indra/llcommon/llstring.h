/** 
 * @file llstring.h
 * @brief String utility functions and std::string class.
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
#ifndef LL_LLSTRING_H
#define LL_LLSTRING_H
#include <boost/optional/optional.hpp>
#include <string>
#if __cplusplus < 201606
#include <absl/strings/string_view.h>
namespace std {
    typedef absl::string_view string_view;
}
#else
    #include <string_view>
#endif
#include <cstdio>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <map>
#include "stdtypes.h"
#include "llpreprocessor.h"
#include <list>
#if LL_SOLARIS
#include <wctype.h>
#include <wchar.h>
#endif
#include <string.h>
#include <boost/scoped_ptr.hpp>
#if LL_SOLARIS
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif
const char LL_UNKNOWN_CHAR = '?';
class LLSD;
#if LL_DARWIN || LL_SOLARIS
#include <cstring>
namespace std
{
template<>
struct char_traits<U16>
{
	typedef U16 		char_type;
	typedef int 	    int_type;
	typedef streampos 	pos_type;
	typedef streamoff 	off_type;
	typedef mbstate_t 	state_type;
	static void
		assign(char_type& __c1, const char_type& __c2)
	{ __c1 = __c2; }
	static bool
		eq(const char_type& __c1, const char_type& __c2)
	{ return __c1 == __c2; }
	static bool
		lt(const char_type& __c1, const char_type& __c2)
	{ return __c1 < __c2; }
	static int
		compare(const char_type* __s1, const char_type* __s2, size_t __n)
	{ return memcmp(__s1, __s2, __n * sizeof(char_type)); }
	static size_t
		length(const char_type* __s)
	{
		const char_type *cur_char = __s;
		while (*cur_char != 0)
		{
			++cur_char;
		}
		return cur_char - __s;
	}
	static const char_type*
		find(const char_type* __s, size_t __n, const char_type& __a)
	{ return static_cast<const char_type*>(memchr(__s, __a, __n * sizeof(char_type))); }
	static char_type*
		move(char_type* __s1, const char_type* __s2, size_t __n)
	{ return static_cast<char_type*>(memmove(__s1, __s2, __n * sizeof(char_type))); }
	static char_type*
		copy(char_type* __s1, const char_type* __s2, size_t __n)
	{  return static_cast<char_type*>(memcpy(__s1, __s2, __n * sizeof(char_type))); }
	static char_type*
		assign(char_type* __s, size_t __n, char_type __a)
	{
		size_t __i;
		for(__i = 0; __i < __n; __i++)
		{
			__s[__i] = __a;
		}
		return __s;
	}
	static char_type
		to_char_type(const int_type& __c)
	{ return static_cast<char_type>(__c); }
	static int_type
		to_int_type(const char_type& __c)
	{ return static_cast<int_type>(__c); }
	static bool
		eq_int_type(const int_type& __c1, const int_type& __c2)
	{ return __c1 == __c2; }
	static int_type
		eof() { return static_cast<int_type>(EOF); }
	static int_type
		not_eof(const int_type& __c)
      { return (__c == eof()) ? 0 : __c; }
  };
};
#endif
class LL_COMMON_API LLStringOps
{
private:
	static long sPacificTimeOffset;
	static long sLocalTimeOffset;
	static bool sPacificDaylightTime;
	static std::map<std::string, std::string> datetimeToCodes;
public:
	static std::vector<std::string> sWeekDayList;
	static std::vector<std::string> sWeekDayShortList;
	static std::vector<std::string> sMonthList;
	static std::vector<std::string> sMonthShortList;
	static std::string sDayFormat;
	static std::string sAM;
	static std::string sPM;
	static char toUpper(char elem) { return toupper((unsigned char)elem); }
	static llwchar toUpper(llwchar elem) { return towupper(elem); }
	static char toLower(char elem) { return tolower((unsigned char)elem); }
	static llwchar toLower(llwchar elem) { return towlower(elem); }
	static bool isSpace(char elem) { return isspace((unsigned char)elem) != 0; }
	static bool isSpace(llwchar elem) { return iswspace(elem) != 0; }
	static bool isUpper(char elem) { return isupper((unsigned char)elem) != 0; }
	static bool isUpper(llwchar elem) { return iswupper(elem) != 0; }
	static bool isLower(char elem) { return islower((unsigned char)elem) != 0; }
	static bool isLower(llwchar elem) { return iswlower(elem) != 0; }
	static bool isDigit(char a) { return isdigit((unsigned char)a) != 0; }
	static bool isDigit(llwchar a) { return iswdigit(a) != 0; }
	static bool isPunct(char a) { return ispunct((unsigned char)a) != 0; }
	static bool isPunct(llwchar a) { return iswpunct(a) != 0; }
	static bool isAlpha(char a) { return isalpha((unsigned char)a) != 0; }
	static bool isAlpha(llwchar a) { return iswalpha(a) != 0; }
	static bool isAlnum(char a) { return isalnum((unsigned char)a) != 0; }
	static bool isAlnum(llwchar a) { return iswalnum(a) != 0; }
	static bool isEmoji(llwchar a);
	static S32	collate(const char* a, const char* b) { return strcoll(a, b); }
	static S32	collate(const llwchar* a, const llwchar* b);
	static bool isHexString(const std::string& str);
	static void setupDatetimeInfo(bool pacific_daylight_time);
	static void setupWeekDaysNames(const std::string& data);
	static void setupWeekDaysShortNames(const std::string& data);
	static void setupMonthNames(const std::string& data);
	static void setupMonthShortNames(const std::string& data);
	static void setupDayFormat(const std::string& data);
	static long getPacificTimeOffset(void) { return sPacificTimeOffset;}
	static long getLocalTimeOffset(void) { return sLocalTimeOffset;}
	static bool getPacificDaylightTime(void) { return sPacificDaylightTime;}
	static std::string getDatetimeCode (const std::string& key);
    static std::string getReadableNumber(F64 num);
};
LL_COMMON_API std::string ll_safe_string(const char* in);
LL_COMMON_API std::string ll_safe_string(const char* in, S32 maxlen);
class LLFormatMapString
{
public:
	LLFormatMapString() {};
	LLFormatMapString(const char* s) : mString(ll_safe_string(s)) {};
	LLFormatMapString(const std::string& s) : mString(s) {};
	operator std::string() const { return mString; }
	bool operator<(const LLFormatMapString& rhs) const { return mString < rhs.mString; }
	std::size_t length() const { return mString.length(); }
	~LLFormatMapString() noexcept { }
private:
	std::string mString;
};
template <class T>
class LLStringUtilBase
{
private:
	static std::string sLocale;
public:
	typedef std::basic_string<T> string_type;
	typedef typename string_type::size_type size_type;
public:
	static const string_type null;
	typedef std::map<LLFormatMapString, LLFormatMapString> format_map_t;
	LL_COMMON_API static void getTokens(const string_type& instr,
										std::vector<string_type >& tokens,
										const string_type& delims);
	static std::vector<string_type> getTokens(const string_type& instr,
											  const string_type& delims);
	static void getTokens(const string_type& instr,
						  std::vector<string_type>& tokens,
						  const string_type& drop_delims,
						  const string_type& keep_delims,
						  const string_type& quotes=string_type());
	static std::vector<string_type> getTokens(const string_type& instr,
											  const string_type& drop_delims,
											  const string_type& keep_delims,
											  const string_type& quotes=string_type());
	static void getTokens(const string_type& instr,
						  std::vector<string_type>& tokens,
						  const string_type& drop_delims,
						  const string_type& keep_delims,
						  const string_type& quotes,
						  const string_type& escapes);
	static std::vector<string_type> getTokens(const string_type& instr,
											  const string_type& drop_delims,
											  const string_type& keep_delims,
											  const string_type& quotes,
											  const string_type& escapes);
	LL_COMMON_API static void formatNumber(string_type& numStr, string_type decimals);
	LL_COMMON_API static bool formatDatetime(string_type& replacement, string_type token, string_type param, S32 secFromEpoch);
	LL_COMMON_API static S32 format(string_type& s, const format_map_t& substitutions);
	LL_COMMON_API static S32 format(string_type& s, const LLSD& substitutions);
	LL_COMMON_API static bool simpleReplacement(string_type& replacement, string_type token, const format_map_t& substitutions);
	LL_COMMON_API static bool simpleReplacement(string_type& replacement, string_type token, const LLSD& substitutions);
	LL_COMMON_API static void setLocale (std::string inLocale);
	LL_COMMON_API static std::string getLocale (void);
	static bool isValidIndex(const string_type& string, size_type i)
	{
		return !string.empty() && (0 <= i) && (i <= string.size());
	}
	static bool contains(const string_type& string, T c, size_type i=0)
	{
		return string.find(c, i) != string_type::npos;
	}
	static void	trimHead(string_type& string);
	static void	trimTail(string_type& string);
	static void trimTail(string_type& string, const string_type& tokens);
	static void	trim(string_type& string)	{ trimHead(string); trimTail(string); }
	static void truncate(string_type& string, size_type count);
	static void	toUpper(string_type& string);
	static void	toLower(string_type& string);
	static BOOL	isHead( const string_type& string, const T* s );
	static bool startsWith(
		const string_type& string,
		const string_type& substr);
	static bool endsWith(
		const string_type& string,
		const string_type& substr);
	static string_type getenv(const std::string& key, const string_type& dflt="");
	static boost::optional<string_type> getoptenv(const std::string& key);
	static void	addCRLF(string_type& string);
	static void	removeCRLF(string_type& string);
	static void removeWindowsCR(string_type& string);
	static void	replaceTabsWithSpaces( string_type& string, size_type spaces_per_tab );
	static void	replaceNonstandardASCII( string_type& string, T replacement );
	static void	replaceChar( string_type& string, T target, T replacement );
	static void replaceString( string_type& string, string_type target, string_type replacement );
	static BOOL	containsNonprintable(const string_type& string);
	static void	stripNonprintable(string_type& string);
	static string_type quote(const string_type& str,
							 const string_type& triggers=" \"",
							 const string_type& escape="\\");
	static void _makeASCII(string_type& string);
	static bool _isASCII(std::basic_string<T> const& string);
	static BOOL	convertToBOOL(const string_type& string, BOOL& value);
	static BOOL	convertToU8(const string_type& string, U8& value);
	static BOOL	convertToS8(const string_type& string, S8& value);
	static BOOL	convertToS16(const string_type& string, S16& value);
	static BOOL	convertToU16(const string_type& string, U16& value);
	static BOOL	convertToU32(const string_type& string, U32& value);
	static BOOL	convertToS32(const string_type& string, S32& value);
	static BOOL	convertToF32(const string_type& string, F32& value);
	static BOOL	convertToF64(const string_type& string, F64& value);
	static S32		compareStrings(const T* lhs, const T* rhs);
	static S32		compareStrings(const string_type& lhs, const string_type& rhs);
	static S32		compareInsensitive(const T* lhs, const T* rhs);
	static S32		compareInsensitive(const string_type& lhs, const string_type& rhs);
	static S32		compareDict(const string_type& a, const string_type& b);
	static S32		compareDictInsensitive(const string_type& a, const string_type& b);
	static BOOL		precedesDict( const string_type& a, const string_type& b );
	static void		copy(T* dst, const T* src, size_type dst_size);
	static void		copyInto(string_type& dst, const string_type& src, size_type offset);
	static bool		isPartOfWord(T c) { return (c == (T)'_') || LLStringOps::isAlnum(c); }
#ifdef _DEBUG
	LL_COMMON_API static void		testHarness();
#endif
private:
	LL_COMMON_API static size_type getSubstitution(const string_type& instr, size_type& start, std::vector<string_type >& tokens);
};
template<class T> const std::basic_string<T> LLStringUtilBase<T>::null;
template<class T> std::string LLStringUtilBase<T>::sLocale;
typedef LLStringUtilBase<char> LLStringUtil;
typedef LLStringUtilBase<llwchar> LLWStringUtil;
typedef std::basic_string<llwchar> LLWString;
class LLStringExplicit : public std::string
{
public:
	explicit LLStringExplicit(const char* s) : std::string(s) {}
	LLStringExplicit(const std::string& s) : std::string(s) {}
	LLStringExplicit(const std::string& s, size_type pos, size_type n = std::string::npos) : std::string(s, pos, n) {}
};
struct LLDictionaryLess
{
public:
	bool operator()(const std::string& a, const std::string& b) const
	{
		return (LLStringUtil::precedesDict(a, b) ? true : false);
	}
};
inline std::string chop_tail_copy(
	const std::string& in,
	std::string::size_type count)
{
	return std::string(in, 0, in.length() - count);
}
LL_COMMON_API bool is_char_hex(char hex);
LL_COMMON_API U8 hex_as_nybble(char hex);
LL_COMMON_API bool _read_file_into_string(std::string& str, const std::string& filename);
LL_COMMON_API bool iswindividual(llwchar elem);
template<typename TO, typename FROM, typename Enable=void>
struct ll_convert_impl
{
    TO operator()(const FROM& in) const;
};
template<typename TO, typename FROM>
TO ll_convert(const FROM& in)
{
    return ll_convert_impl<TO, FROM>()(in);
}
template<typename T>
struct ll_convert_impl<T, T>
{
    T operator()(const T& in) const { return in; }
};
#define ll_convert_alias(TO, FROM, EXPR)                    \
template<>                                                  \
struct ll_convert_impl<TO, FROM>                            \
{                                                           \
    TO operator()(const FROM& in) const { return EXPR; }    \
}
LL_COMMON_API std::string rawstr_to_utf8(const std::string& raw);
#if _WIN32 && _NATIVE_WCHAR_T_DEFINED
typedef wchar_t utf16strtype;
#else
typedef U16 utf16strtype;
#endif
typedef std::basic_string<utf16strtype> llutf16string;
#if ! defined(LL_WCHAR_T_NATIVE)
#define ll_convert_u16_alias(TO, FROM, EXPR)
#else
#define ll_convert_u16_alias(TO, FROM, EXPR) ll_convert_alias(TO, FROM, EXPR)
#if LL_WINDOWS
ll_convert_alias(llutf16string, std::wstring, llutf16string(in.begin(), in.end()));
ll_convert_alias(std::wstring, llutf16string,  std::wstring(in.begin(), in.end()));
#endif
#endif
LL_COMMON_API LLWString utf16str_to_wstring(const llutf16string &utf16str, S32 len);
LL_COMMON_API LLWString utf16str_to_wstring(const llutf16string &utf16str);
ll_convert_u16_alias(LLWString, llutf16string, utf16str_to_wstring(in));
LL_COMMON_API llutf16string wstring_to_utf16str(const LLWString &utf32str, S32 len);
LL_COMMON_API llutf16string wstring_to_utf16str(const LLWString &utf32str);
ll_convert_u16_alias(llutf16string, LLWString, wstring_to_utf16str(in));
LL_COMMON_API llutf16string utf8str_to_utf16str ( const std::string& utf8str, S32 len);
LL_COMMON_API llutf16string utf8str_to_utf16str ( const std::string& utf8str );
ll_convert_u16_alias(llutf16string, std::string, utf8str_to_utf16str(in));
LL_COMMON_API LLWString utf8str_to_wstring(const std::string &utf8str, S32 len);
LL_COMMON_API LLWString utf8str_to_wstring(const std::string &utf8str);
inline LLWString utf8string_to_wstring(const std::string& utf8_string) { return utf8str_to_wstring(utf8_string); }
ll_convert_alias(LLWString, std::string, utf8string_to_wstring(in));
LL_COMMON_API S32 wchar_to_utf8chars(llwchar inchar, char* outchars);
LL_COMMON_API std::string wstring_to_utf8str(const LLWString &utf32str, S32 len);
LL_COMMON_API std::string wstring_to_utf8str(const LLWString &utf32str);
ll_convert_alias(std::string, LLWString, wstring_to_utf8str(in));
LL_COMMON_API std::string utf16str_to_utf8str(const llutf16string &utf16str, S32 len);
LL_COMMON_API std::string utf16str_to_utf8str(const llutf16string &utf16str);
ll_convert_u16_alias(std::string, llutf16string, utf16str_to_utf8str(in));
#if LL_WINDOWS
inline std::string wstring_to_utf8str(const llutf16string &utf16str) { return utf16str_to_utf8str(utf16str);}
#endif
LL_COMMON_API S32 wstring_utf8_length(const LLWString& wstr);
LL_COMMON_API S32 wchar_utf8_length(const llwchar wc);
LL_COMMON_API std::string utf8str_tolower(const std::string& utf8str);
LL_COMMON_API S32 utf16str_wstring_length(const llutf16string &utf16str, S32 len);
LL_COMMON_API S32 wstring_utf16_length(const LLWString & wstr, S32 woffset, S32 wlen);
LL_COMMON_API S32 wstring_wstring_length_from_utf16_length(const LLWString & wstr, S32 woffset, S32 utf16_length, BOOL *unaligned = nullptr);
LL_COMMON_API std::string utf8str_truncate(const std::string& utf8str, const S32 max_len);
LL_COMMON_API std::string utf8str_substr(const std::string& utf8str, const S32 index, const S32 max_len);
LL_COMMON_API void utf8str_split(std::list<std::string>& split_list, const std::string& utf8str, size_t maxlen, char split_token);
LL_COMMON_API std::string utf8str_trim(const std::string& utf8str);
LL_COMMON_API S32 utf8str_compare_insensitive(
	const std::string& lhs,
	const std::string& rhs);
LL_COMMON_API std::string utf8str_symbol_truncate(const std::string& utf8str, const S32 symbol_len);
LL_COMMON_API std::string utf8str_substChar(
	const std::string& utf8str,
	const llwchar target_char,
	const llwchar replace_char);
LL_COMMON_API std::string utf8str_makeASCII(const std::string& utf8str);
LL_COMMON_API std::string mbcsstring_makeASCII(const std::string& str);
LL_COMMON_API std::string utf8str_removeCRLF(const std::string& utf8str);
LL_COMMON_API bool wstring_has_emoji(const LLWString& wstr);
LL_COMMON_API bool wstring_remove_emojis(LLWString& wstr);
LL_COMMON_API bool utf8str_remove_emojis(std::string& utf8str);
#if LL_WINDOWS
LL_COMMON_API std::string ll_convert_wide_to_string(const wchar_t* in, unsigned int code_page);
LL_COMMON_API std::string ll_convert_wide_to_string(const wchar_t* in);
inline std::string ll_convert_wide_to_string(const std::wstring& in, unsigned int code_page)
{
    return ll_convert_wide_to_string(in.c_str(), code_page);
}
inline std::string ll_convert_wide_to_string(const std::wstring& in)
{
    return ll_convert_wide_to_string(in.c_str());
}
ll_convert_alias(std::string, std::wstring, ll_convert_wide_to_string(in));
LL_COMMON_API wchar_t* ll_convert_string_to_wide(const std::string& in,
                                                     unsigned int code_page);
LL_COMMON_API wchar_t* ll_convert_string_to_wide(const std::string& in);
ll_convert_alias(wchar_t*, std::string, ll_convert_string_to_wide(in));
LL_COMMON_API LLWString ll_convert_wide_to_wstring(const std::wstring& in);
ll_convert_alias(LLWString, std::wstring, ll_convert_wide_to_wstring(in));
LL_COMMON_API std::wstring ll_convert_wstring_to_wide(const LLWString& in);
ll_convert_alias(std::wstring, LLWString, ll_convert_wstring_to_wide(in));
LL_COMMON_API std::string ll_convert_string_to_utf8_string(const std::string& in);
template<typename STRING>
STRING windows_message(unsigned long error)
{
    return ll_convert<STRING>(windows_message<std::wstring>(error));
}
template<>
LL_COMMON_API std::wstring windows_message<std::wstring>(unsigned long error);
template<typename STRING>
STRING windows_message() { return windows_message<STRING>(GetLastError()); }
LL_COMMON_API boost::optional<std::wstring> llstring_getoptenv(const std::string& key);
#else
LL_COMMON_API boost::optional<std::string>  llstring_getoptenv(const std::string& key);
#endif
namespace LLStringFn
{
	LL_COMMON_API void replace_nonprintable_in_ascii(
		std::basic_string<char>& string,
		char replacement);
	LL_COMMON_API void replace_nonprintable_and_pipe_in_ascii(std::basic_string<char>& str,
									   char replacement);
	LL_COMMON_API std::string strip_invalid_xml(const std::string& input);
	LL_COMMON_API void replace_ascii_controlchars(
		std::basic_string<char>& string,
		char replacement);
}
template <class T>
std::vector<typename LLStringUtilBase<T>::string_type>
LLStringUtilBase<T>::getTokens(const string_type& instr, const string_type& delims)
{
	std::vector<string_type> tokens;
	getTokens(instr, tokens, delims);
	return tokens;
}
template <class T>
std::vector<typename LLStringUtilBase<T>::string_type>
LLStringUtilBase<T>::getTokens(const string_type& instr,
							   const string_type& drop_delims,
							   const string_type& keep_delims,
							   const string_type& quotes)
{
	std::vector<string_type> tokens;
	getTokens(instr, tokens, drop_delims, keep_delims, quotes);
	return tokens;
}
template <class T>
std::vector<typename LLStringUtilBase<T>::string_type>
LLStringUtilBase<T>::getTokens(const string_type& instr,
							   const string_type& drop_delims,
							   const string_type& keep_delims,
							   const string_type& quotes,
							   const string_type& escapes)
{
	std::vector<string_type> tokens;
	getTokens(instr, tokens, drop_delims, keep_delims, quotes, escapes);
	return tokens;
}
namespace LLStringUtilBaseImpl
{
template <class T>
struct InString
{
	typedef std::basic_string<T> string_type;
	typedef typename string_type::const_iterator const_iterator;
	InString(const_iterator b, const_iterator e):
		mIter(b),
		mEnd(e)
	{}
	virtual ~InString() {}
	bool done() const { return mIter == mEnd; }
	virtual bool escaped() const { return false; }
	virtual T next() { return *mIter++; }
	virtual bool is(T ch) const { return (! done()) && *mIter == ch; }
	virtual bool oneof(const string_type& delims) const
	{
		return (! done()) && LLStringUtilBase<T>::contains(delims, *mIter);
	}
	virtual bool collect_until(string_type& into, const_iterator from, T delim)
	{
		const_iterator found = std::find(from, mEnd, delim);
		if (found == mEnd)
			return false;
		into.append(from, found);
		mIter = found + 1;
		return true;
	}
	const_iterator mIter, mEnd;
};
template <class T>
class InEscString: public InString<T>
{
public:
	typedef InString<T> super;
	typedef typename super::string_type string_type;
	typedef typename super::const_iterator const_iterator;
	using super::done;
	using super::mIter;
	using super::mEnd;
	InEscString(const_iterator b, const_iterator e, const string_type& escapes):
		super(b, e),
		mEscapes(escapes)
	{
		setiter(b);
	}
	bool escaped() const override { return mIsEsc; }
	T next() override
	{
		if (mIsEsc)
			++mIter;
		T result(*mIter);
		setiter(mIter + 1);
		return result;
	}
	bool is(T ch) const override
	{
		return (! done()) && (! mIsEsc) && *mIter == ch;
	}
	bool oneof(const string_type& delims) const override
	{
		return (! done()) && (! mIsEsc) && LLStringUtilBase<T>::contains(delims, *mIter);
	}
	bool collect_until(string_type& into, const_iterator from, T delim) override
	{
		string_type collected;
		const_iterator save_iter(mIter);
		setiter(from);
		while (! done())
		{
			if ((! mIsEsc) && *mIter == delim)
			{
				into.append(collected);
				setiter(mIter + 1);
				return true;
			}
			collected.push_back(next());
		}
		setiter(save_iter);
		return false;
	}
private:
	void setiter(const_iterator i)
	{
		mIter = i;
		mIsEsc = (! done()) &&
				LLStringUtilBase<T>::contains(mEscapes, *mIter) &&
				(mIter+1) != mEnd;
	}
	const string_type mEscapes;
	bool mIsEsc;
};
template <typename INSTRING, typename string_type>
void getTokens(INSTRING& instr, std::vector<string_type>& tokens,
			   const string_type& drop_delims, const string_type& keep_delims,
			   const string_type& quotes)
{
	string_type all_delims(drop_delims + keep_delims);
	tokens.clear();
	while (! instr.done())
	{
		while (instr.oneof(drop_delims))
		{
			instr.next();
			if (instr.done())
				return;
		}
		tokens.push_back(string_type());
		if (instr.oneof(keep_delims))
		{
			tokens.back().push_back(instr.next());
			continue;
		}
		while (! instr.oneof(all_delims))
		{
			if (instr.oneof(quotes) &&
				instr.collect_until(tokens.back(), instr.mIter+1, *instr.mIter))
			{
			}
			else
			{
				tokens.back().push_back(instr.next());
			}
			if (instr.done())
				return;
		}
	}
}
}
template <class T>
void LLStringUtilBase<T>::getTokens(const string_type& string, std::vector<string_type>& tokens,
									const string_type& drop_delims, const string_type& keep_delims,
									const string_type& quotes)
{
	LLStringUtilBaseImpl::InString<T> instring(string.begin(), string.end());
	LLStringUtilBaseImpl::getTokens(instring, tokens, drop_delims, keep_delims, quotes);
}
template <class T>
void LLStringUtilBase<T>::getTokens(const string_type& string, std::vector<string_type>& tokens,
									const string_type& drop_delims, const string_type& keep_delims,
									const string_type& quotes, const string_type& escapes)
{
	boost::scoped_ptr< LLStringUtilBaseImpl::InString<T> > instrp;
	if (escapes.empty())
		instrp.reset(new LLStringUtilBaseImpl::InString<T>(string.begin(), string.end()));
	else
		instrp.reset(new LLStringUtilBaseImpl::InEscString<T>(string.begin(), string.end(), escapes));
	LLStringUtilBaseImpl::getTokens(*instrp, tokens, drop_delims, keep_delims, quotes);
}
template<class T>
S32 LLStringUtilBase<T>::compareStrings(const T* lhs, const T* rhs)
{
	S32 result;
	if( lhs == rhs )
	{
		result = 0;
	}
	else
	if ( !lhs || !lhs[0] )
	{
		result = ((!rhs || !rhs[0]) ? 0 : 1);
	}
	else
	if ( !rhs || !rhs[0])
	{
		result = -1;
	}
	else
	{
		result = LLStringOps::collate(lhs, rhs);
	}
	return result;
}
template<class T>
S32 LLStringUtilBase<T>::compareStrings(const string_type& lhs, const string_type& rhs)
{
	return LLStringOps::collate(lhs.c_str(), rhs.c_str());
}
template<class T>
S32 LLStringUtilBase<T>::compareInsensitive(const T* lhs, const T* rhs )
{
	S32 result;
	if( lhs == rhs )
	{
		result = 0;
	}
	else
	if ( !lhs || !lhs[0] )
	{
		result = ((!rhs || !rhs[0]) ? 0 : 1);
	}
	else
	if ( !rhs || !rhs[0] )
	{
		result = -1;
	}
	else
	{
		string_type lhs_string(lhs);
		string_type rhs_string(rhs);
		LLStringUtilBase<T>::toUpper(lhs_string);
		LLStringUtilBase<T>::toUpper(rhs_string);
		result = LLStringOps::collate(lhs_string.c_str(), rhs_string.c_str());
	}
	return result;
}
template<class T>
S32 LLStringUtilBase<T>::compareInsensitive(const string_type& lhs, const string_type& rhs)
{
	string_type lhs_string(lhs);
	string_type rhs_string(rhs);
	LLStringUtilBase<T>::toUpper(lhs_string);
	LLStringUtilBase<T>::toUpper(rhs_string);
	return LLStringOps::collate(lhs_string.c_str(), rhs_string.c_str());
}
template<class T>
S32 LLStringUtilBase<T>::compareDict(const string_type& astr, const string_type& bstr)
{
	const T* a = astr.c_str();
	const T* b = bstr.c_str();
	T ca, cb;
	S32 ai, bi, cnt = 0;
	S32 bias = 0;
	ca = *(a++);
	cb = *(b++);
	while( ca && cb ){
		if( bias==0 ){
			if( LLStringOps::isUpper(ca) ){ ca = LLStringOps::toLower(ca); bias--; }
			if( LLStringOps::isUpper(cb) ){ cb = LLStringOps::toLower(cb); bias++; }
		}else{
			if( LLStringOps::isUpper(ca) ){ ca = LLStringOps::toLower(ca); }
			if( LLStringOps::isUpper(cb) ){ cb = LLStringOps::toLower(cb); }
		}
		if( LLStringOps::isDigit(ca) ){
			if( cnt-->0 ){
				if( cb!=ca ) break;
			}else{
				if( !LLStringOps::isDigit(cb) ) break;
				for(ai=0; LLStringOps::isDigit(a[ai]); ai++);
				for(bi=0; LLStringOps::isDigit(b[bi]); bi++);
				if( ai<bi ){ ca=0; break; }
				if( bi<ai ){ cb=0; break; }
				if( ca!=cb ) break;
				cnt = ai;
			}
		}else if( ca!=cb ){   break;
		}
		ca = *(a++);
		cb = *(b++);
	}
	if( ca==cb ) ca += bias;
	return ca-cb;
}
template<class T>
S32 LLStringUtilBase<T>::compareDictInsensitive(const string_type& astr, const string_type& bstr)
{
	const T* a = astr.c_str();
	const T* b = bstr.c_str();
	T ca, cb;
	S32 ai, bi, cnt = 0;
	ca = *(a++);
	cb = *(b++);
	while( ca && cb ){
		if( LLStringOps::isUpper(ca) ){ ca = LLStringOps::toLower(ca); }
		if( LLStringOps::isUpper(cb) ){ cb = LLStringOps::toLower(cb); }
		if( LLStringOps::isDigit(ca) ){
			if( cnt-->0 ){
				if( cb!=ca ) break;
			}else{
				if( !LLStringOps::isDigit(cb) ) break;
				for(ai=0; LLStringOps::isDigit(a[ai]); ai++);
				for(bi=0; LLStringOps::isDigit(b[bi]); bi++);
				if( ai<bi ){ ca=0; break; }
				if( bi<ai ){ cb=0; break; }
				if( ca!=cb ) break;
				cnt = ai;
			}
		}else if( ca!=cb ){   break;
		}
		ca = *(a++);
		cb = *(b++);
	}
	return ca-cb;
}
template<class T>
BOOL LLStringUtilBase<T>::precedesDict( const string_type& a, const string_type& b )
{
	if( a.size() && b.size() )
	{
		return (LLStringUtilBase<T>::compareDict(a, b) < 0);
	}
	else
	{
		return (!b.empty());
	}
}
template<class T>
void LLStringUtilBase<T>::toUpper(string_type& string)
{
	if( !string.empty() )
	{
		std::transform(
			string.begin(),
			string.end(),
			string.begin(),
			(T(*)(T)) &LLStringOps::toUpper);
	}
}
template<class T>
void LLStringUtilBase<T>::toLower(string_type& string)
{
	if( !string.empty() )
	{
		std::transform(
			string.begin(),
			string.end(),
			string.begin(),
			(T(*)(T)) &LLStringOps::toLower);
	}
}
template<class T>
void LLStringUtilBase<T>::trimHead(string_type& string)
{
	if( !string.empty() )
	{
		size_type i = 0;
		while( i < string.length() && LLStringOps::isSpace( string[i] ) )
		{
			i++;
		}
		string.erase(0, i);
	}
}
template<class T>
void LLStringUtilBase<T>::trimTail(string_type& string)
{
	if(!string.empty())
	{
		size_type len = string.length();
		size_type i = len;
		while( i > 0 && LLStringOps::isSpace( string[i-1] ) )
		{
			i--;
		}
		string.erase( i, len - i );
	}
}
template<class T>
void LLStringUtilBase<T>::trimTail(string_type& string, const string_type& tokens)
{
	if(!string.empty())
	{
		size_type len = string.length();
		size_type i = len;
		while( i > 0 && (tokens.find_first_of(string[i-1]) != string_type::npos) )
		{
			i--;
		}
		string.erase( i, len - i );
	}
}
template<class T>
void LLStringUtilBase<T>::addCRLF(string_type& string)
{
	if (string.empty())
		return;
	const T LF = 10;
	const T CR = 13;
	size_type count = 0;
	size_type len = string.size();
	size_type i;
	for( i = 0; i < len; i++ )
	{
		if( string[i] == LF )
		{
			count++;
		}
	}
	if( count )
	{
		size_type size = len + count;
		T *t = new T[size];
		size_type j = 0;
		for( i = 0; i < len; ++i )
		{
			if( string[i] == LF )
			{
				t[j] = CR;
				++j;
			}
			t[j] = string[i];
			++j;
		}
		string.assign(t, size);
		delete[] t;
	}
}
template<class T>
void LLStringUtilBase<T>::removeCRLF(string_type& string)
{
	if (string.empty())
		return;
	const T CR = 13;
	size_type cr_count = 0;
	size_type len = string.size();
	size_type i;
	for( i = 0; i < len - cr_count; i++ )
	{
		if( string[i+cr_count] == CR )
		{
			cr_count++;
		}
		string[i] = string[i+cr_count];
	}
	string.erase(i, cr_count);
}
template<class T>
void LLStringUtilBase<T>::removeWindowsCR(string_type& string)
{
    if (string.empty())
    {
        return;
    }
    const T LF = 10;
    const T CR = 13;
    size_type cr_count = 0;
    size_type len = string.size();
    size_type i;
    for( i = 0; i < len - cr_count - 1; i++ )
    {
        if( string[i+cr_count] == CR && string[i+cr_count+1] == LF)
        {
            cr_count++;
        }
        string[i] = string[i+cr_count];
    }
    string.erase(i, cr_count);
}
template<class T>
void LLStringUtilBase<T>::replaceChar( string_type& string, T target, T replacement )
{
	size_type found_pos = 0;
	while( (found_pos = string.find(target, found_pos)) != string_type::npos )
	{
		string[found_pos] = replacement;
		found_pos++;
	}
}
template<class T>
void LLStringUtilBase<T>::replaceString( string_type& string, string_type target, string_type replacement )
{
	size_type found_pos = 0;
	while( (found_pos = string.find(target, found_pos)) != string_type::npos )
	{
		string.replace( found_pos, target.length(), replacement );
		found_pos += replacement.length();
	}
}
template<class T>
void LLStringUtilBase<T>::replaceNonstandardASCII( string_type& string, T replacement )
{
	const char LF = 10;
	const S8 MIN = 32;
	size_type len = string.size();
	for( size_type i = 0; i < len; i++ )
	{
		if( ( S8(string[i]) < MIN ) && (string[i] != LF) )
		{
			string[i] = replacement;
		}
	}
}
template<class T>
void LLStringUtilBase<T>::replaceTabsWithSpaces( string_type& str, size_type spaces_per_tab )
{
	const T TAB = '\t';
	const T SPACE = ' ';
	string_type out_str;
	for (size_type i = 0; i < str.length(); i++)
	{
		if (str[i] == TAB)
		{
			for (size_type j = 0; j < spaces_per_tab; j++)
				out_str += SPACE;
		}
		else
		{
			out_str += str[i];
		}
	}
	str = out_str;
}
template<class T>
BOOL LLStringUtilBase<T>::containsNonprintable(const string_type& string)
{
	const char MIN = 32;
	BOOL rv = FALSE;
	for (size_type i = 0; i < string.size(); i++)
	{
		if(string[i] < MIN)
		{
			rv = TRUE;
			break;
		}
	}
	return rv;
}
template<class T>
void LLStringUtilBase<T>::stripNonprintable(string_type& string)
{
	const char MIN = 32;
	size_type j = 0;
	if (string.empty())
	{
		return;
	}
	const size_t src_size = string.size();
	auto c_string = std::make_unique<char[]>(src_size + 1);
	copy(c_string.get(), string.c_str(), src_size+1);
	for (size_type i = 0; i < src_size; i++)
	{
		if(string[i] >= MIN)
		{
			c_string[j] = string[i];
			++j;
		}
	}
	c_string[j]= '\0';
	string.assign(c_string.get());
}
template<class T>
std::basic_string<T> LLStringUtilBase<T>::quote(const string_type& str,
												const string_type& triggers,
												const string_type& escape)
{
	size_type len(str.length());
	if (len >= 2 && str[0] == '"' && str[len-1] == '"')
	{
		return str;
	}
	if ((! triggers.empty()) && str.find_first_of(triggers) == string_type::npos)
	{
		return str;
	}
	auto needed_escapes = std::count(str.begin(), str.end(), '"');
	string_type result;
	result.reserve(len + (needed_escapes * escape.length()));
	result.push_back('"');
	for (typename string_type::const_iterator ci(str.begin()), cend(str.end()); ci != cend; ++ci)
	{
		if (*ci == '"')
		{
			result.append(escape);
		}
		result.push_back(*ci);
	}
	result.push_back('"');
	return result;
}
template<class T>
void LLStringUtilBase<T>::_makeASCII(string_type& string)
{
	for (size_type i = 0; i < string.length(); i++)
	{
		if (string[i] > 0x7f)
		{
			string[i] = LL_UNKNOWN_CHAR;
		}
	}
}
template<class T>
bool LLStringUtilBase<T>::_isASCII(std::basic_string<T> const& string)
{
	size_type const len = string.length();
	T bit_collector = 0;
	for (size_type i = 0; i < len; ++i)
	{
		bit_collector |= string[i];
	}
	T const ascii_bits = 0x7f;
	return !(bit_collector & ~ascii_bits);
}
template<class T>
void LLStringUtilBase<T>::copy( T* dst, const T* src, size_type dst_size )
{
	if( dst_size > 0 )
	{
		size_type min_len = 0;
		if( src )
		{
			min_len = llmin( dst_size - 1, strlen( src ) );
			memcpy(dst, src, min_len * sizeof(T));
		}
		dst[min_len] = '\0';
	}
}
template<class T>
void LLStringUtilBase<T>::copyInto(string_type& dst, const string_type& src, size_type offset)
{
	if ( offset == dst.length() )
	{
		dst += src;
	}
	else
	{
		string_type tail = dst.substr(offset);
		dst = dst.substr(0, offset);
		dst += src;
		dst += tail;
	};
}
template<class T>
BOOL LLStringUtilBase<T>::isHead( const string_type& string, const T* s )
{
	if( string.empty() )
	{
		return FALSE;
	}
	else
	{
		return (strncmp( s, string.c_str(), string.size() ) == 0);
	}
}
template<class T>
bool LLStringUtilBase<T>::startsWith(
	const string_type& string,
	const string_type& substr)
{
	if(string.empty() || (substr.empty())) return false;
	if(0 == string.find(substr)) return true;
	return false;
}
template<class T>
bool LLStringUtilBase<T>::endsWith(
	const string_type& string,
	const string_type& substr)
{
	if(string.empty() || (substr.empty())) return false;
	std::string::size_type idx = string.rfind(substr);
	if(std::string::npos == idx) return false;
	return (idx == (string.size() - substr.size()));
}
template<class T>
auto LLStringUtilBase<T>::getoptenv(const std::string& key) -> boost::optional<string_type>
{
    auto found(llstring_getoptenv(key));
    if (found)
    {
        return { ll_convert<string_type>(*found) };
    }
    else
    {
        return {};
    }
}
template<class T>
auto LLStringUtilBase<T>::getenv(const std::string& key, const string_type& dflt) -> string_type
{
    auto found(getoptenv(key));
    if (found)
    {
        return *found;
    }
    else
    {
        return dflt;
    }
}
template<class T>
BOOL LLStringUtilBase<T>::convertToBOOL(const string_type& string, BOOL& value)
{
	if( string.empty() )
	{
		return FALSE;
	}
	string_type temp( string );
	trim(temp);
	if(
		(temp == "1") ||
		(temp == "T") ||
		(temp == "t") ||
		(temp == "TRUE") ||
		(temp == "true") ||
		(temp == "True") )
	{
		value = TRUE;
		return TRUE;
	}
	else
	if(
		(temp == "0") ||
		(temp == "F") ||
		(temp == "f") ||
		(temp == "FALSE") ||
		(temp == "false") ||
		(temp == "False") )
	{
		value = FALSE;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToU8(const string_type& string, U8& value)
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (U8_MIN <= value32) && (value32 <= U8_MAX) )
	{
		value = (U8) value32;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToS8(const string_type& string, S8& value)
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (S8_MIN <= value32) && (value32 <= S8_MAX) )
	{
		value = (S8) value32;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToS16(const string_type& string, S16& value)
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (S16_MIN <= value32) && (value32 <= S16_MAX) )
	{
		value = (S16) value32;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToU16(const string_type& string, U16& value)
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (U16_MIN <= value32) && (value32 <= U16_MAX) )
	{
		value = (U16) value32;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToU32(const string_type& string, U32& value)
{
	if( string.empty() )
	{
		return FALSE;
	}
	string_type temp( string );
	trim(temp);
	U32 v;
	std::basic_istringstream<T> i_stream((string_type)temp);
	if(i_stream >> v)
	{
		value = v;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToS32(const string_type& string, S32& value)
{
	if( string.empty() )
	{
		return FALSE;
	}
	string_type temp( string );
	trim(temp);
	S32 v;
	std::basic_istringstream<T> i_stream((string_type)temp);
	if(i_stream >> v)
	{
		value = v;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToF32(const string_type& string, F32& value)
{
	F64 value64 = 0.0;
	BOOL success = convertToF64(string, value64);
	if( success && (-F32_MAX <= value64) && (value64 <= F32_MAX) )
	{
		value = (F32) value64;
		return TRUE;
	}
	return FALSE;
}
template<class T>
BOOL LLStringUtilBase<T>::convertToF64(const string_type& string, F64& value)
{
	if( string.empty() )
	{
		return FALSE;
	}
	string_type temp( string );
	trim(temp);
	F64 v;
	std::basic_istringstream<T> i_stream((string_type)temp);
	if(i_stream >> v)
	{
		value = v;
		return TRUE;
	}
	return FALSE;
}
template<class T>
void LLStringUtilBase<T>::truncate(string_type& string, size_type count)
{
	size_type cur_size = string.size();
	string.resize(count < cur_size ? count : cur_size);
}
#endif
