/** 
 * @file llsdserialize.h
 * @author Phoenix
 * @date 2006-02-26
 * @brief Declaration of parsers and formatters for LLSD
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#ifndef LL_LLSDSERIALIZE_H
#define LL_LLSDSERIALIZE_H
#include <iosfwd>
#include "llpointer.h"
#include "llrefcount.h"
#include "llsd.h"
class LL_COMMON_API LLSDParser : public LLRefCount
{
protected:
	virtual ~LLSDParser();
public:
	enum
	{
		PARSE_FAILURE = -1
	};
	LLSDParser();
	S32 parse(std::istream& istr, LLSD& data, S32 max_bytes);
	S32 parseLines(std::istream& istr, LLSD& data);
	void reset()	{ doReset();	};
protected:
	virtual S32 doParse(std::istream& istr, LLSD& data) const = 0;
	virtual void doReset()	{};
	int get(std::istream& istr) const;
	std::istream& get(
		std::istream& istr,
		char* s,
		std::streamsize n,
		char delim) const;
	std::istream& get(
		std::istream& istr,
		std::streambuf& sb,
		char delim) const;
	std::istream& ignore(std::istream& istr) const;
	std::istream& putback(std::istream& istr, char c) const;
	std::istream& read(std::istream& istr, char* s, std::streamsize n) const;
protected:
	void account(S32 bytes) const;
protected:
	bool mCheckLimits;
	mutable S32 mMaxBytesLeft;
	bool mParseLines;
};
class LL_COMMON_API LLSDNotationParser : public LLSDParser
{
protected:
	virtual ~LLSDNotationParser();
public:
	LLSDNotationParser();
protected:
	virtual S32 doParse(std::istream& istr, LLSD& data) const;
private:
	S32 parseMap(std::istream& istr, LLSD& map) const;
	S32 parseArray(std::istream& istr, LLSD& array) const;
	bool parseString(std::istream& istr, LLSD& data) const;
	bool parseBinary(std::istream& istr, LLSD& data) const;
};
class LL_COMMON_API LLSDXMLParser : public LLSDParser
{
protected:
	virtual ~LLSDXMLParser();
public:
	LLSDXMLParser(bool emit_errors=true);
protected:
	virtual S32 doParse(std::istream& istr, LLSD& data) const;
	virtual void doReset();
private:
	class Impl;
	Impl& impl;
	void parsePart(const char* buf, int len);
	friend class LLSDSerialize;
};
class LL_COMMON_API LLSDBinaryParser : public LLSDParser
{
protected:
	virtual ~LLSDBinaryParser();
public:
	LLSDBinaryParser();
protected:
	virtual S32 doParse(std::istream& istr, LLSD& data) const;
private:
	S32 parseMap(std::istream& istr, LLSD& map) const;
	S32 parseArray(std::istream& istr, LLSD& array) const;
	bool parseString(std::istream& istr, std::string& value) const;
};
class LL_COMMON_API LLSDFormatter : public LLRefCount
{
protected:
	virtual ~LLSDFormatter();
public:
	typedef enum e_formatter_options_type
	{
		OPTIONS_NONE = 0,
		OPTIONS_PRETTY = 1,
		OPTIONS_PRETTY_BINARY = 2
	} EFormatterOptions;
	LLSDFormatter();
	void boolalpha(bool alpha);
	void realFormat(const std::string& format);
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const = 0;
protected:
	void formatReal(LLSD::Real real, std::ostream& ostr) const;
protected:
	bool mBoolAlpha;
	std::string mRealFormat;
};
class LL_COMMON_API LLSDNotationFormatter : public LLSDFormatter
{
protected:
	virtual ~LLSDNotationFormatter();
public:
	LLSDNotationFormatter();
	static std::string escapeString(const std::string& in);
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const;
protected:
	S32 format_impl(const LLSD& data, std::ostream& ostr, U32 options, U32 level) const;
};
class LL_COMMON_API LLSDXMLFormatter : public LLSDFormatter
{
protected:
	virtual ~LLSDXMLFormatter();
public:
	LLSDXMLFormatter();
	static std::string escapeString(const std::string& in);
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const;
protected:
	S32 format_impl(const LLSD& data, std::ostream& ostr, U32 options, U32 level) const;
};
class LL_COMMON_API LLSDBinaryFormatter : public LLSDFormatter
{
protected:
	virtual ~LLSDBinaryFormatter();
public:
	LLSDBinaryFormatter();
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const;
protected:
	void formatString(const std::string& string, std::ostream& ostr) const;
};
template <class Formatter>
class LLSDOStreamer
{
public:
	LLSDOStreamer(const LLSD& data, U32 options = LLSDFormatter::OPTIONS_NONE) :
		mSD(data), mOptions(options) {}
	friend std::ostream& operator<<(
		std::ostream& str,
		const LLSDOStreamer<Formatter>& formatter)
	{
		LLPointer<Formatter> f = new Formatter;
		f->format(formatter.mSD, str, formatter.mOptions);
		return str;
	}
protected:
	LLSD mSD;
	U32 mOptions;
};
typedef LLSDOStreamer<LLSDNotationFormatter>	LLSDNotationStreamer;
typedef LLSDOStreamer<LLSDXMLFormatter>			LLSDXMLStreamer;
class LL_COMMON_API LLSDSerialize
{
public:
	enum ELLSD_Serialize
	{
        LLSD_BINARY, LLSD_XML, LLSD_NOTATION
	};
	enum
	{
		SIZE_UNLIMITED = -1,
	};
	static void serialize(const LLSD& sd, std::ostream& str, ELLSD_Serialize,
		U32 options = LLSDFormatter::OPTIONS_NONE);
	static bool deserialize(LLSD& sd, std::istream& str, S32 max_bytes);
	static S32 toNotation(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDNotationFormatter> f = new LLSDNotationFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_NONE);
	}
	static S32 toPrettyNotation(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDNotationFormatter> f = new LLSDNotationFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_PRETTY);
	}
	static S32 toPrettyBinaryNotation(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDNotationFormatter> f = new LLSDNotationFormatter;
		return f->format(sd, str,
				LLSDFormatter::OPTIONS_PRETTY |
				LLSDFormatter::OPTIONS_PRETTY_BINARY);
	}
	static S32 fromNotation(LLSD& sd, std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDNotationParser> p = new LLSDNotationParser;
		return p->parse(str, sd, max_bytes);
	}
	static LLSD fromNotation(std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDNotationParser> p = new LLSDNotationParser;
		LLSD sd;
		(void)p->parse(str, sd, max_bytes);
		return sd;
	}
	static S32 toXML(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDXMLFormatter> f = new LLSDXMLFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_NONE);
	}
	static S32 toPrettyXML(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDXMLFormatter> f = new LLSDXMLFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_PRETTY);
	}
	static S32 fromXMLEmbedded(LLSD& sd, std::istream& str, bool emit_errors=true)
	{
		LLPointer<LLSDXMLParser> p = new LLSDXMLParser(emit_errors);
		return p->parse(str, sd, LLSDSerialize::SIZE_UNLIMITED);
	}
	static S32 fromXMLDocument(LLSD& sd, std::istream& str, bool emit_errors=true)
	{
		LLPointer<LLSDXMLParser> p = new LLSDXMLParser(emit_errors);
		return p->parseLines(str, sd);
	}
	static S32 fromXML(LLSD& sd, std::istream& str, bool emit_errors=true)
	{
		return fromXMLEmbedded(sd, str, emit_errors);
	}
	static S32 toBinary(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDBinaryFormatter> f = new LLSDBinaryFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_NONE);
	}
	static S32 fromBinary(LLSD& sd, std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDBinaryParser> p = new LLSDBinaryParser;
		return p->parse(str, sd, max_bytes);
	}
	static LLSD fromBinary(std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDBinaryParser> p = new LLSDBinaryParser;
		LLSD sd;
		(void)p->parse(str, sd, max_bytes);
		return sd;
	}
};
LL_COMMON_API std::string zip_llsd(LLSD& data);
LL_COMMON_API bool unzip_llsd(LLSD& data, std::istream& is, S32 size);
LL_COMMON_API U8* unzip_llsdNavMesh( bool& valid, unsigned int& outsize,std::istream& is, S32 size);
#endif
