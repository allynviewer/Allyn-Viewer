/** 
 * @file llsdserialize.cpp
 * @author Phoenix
 * @date 2006-03-05
 * @brief Implementation of LLSD parsers and formatters
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
#include "linden_common.h"
#include "llsdserialize.h"
#include "llpointer.h"
#include "llstreamtools.h"
#include "llbase64.h"
#include <iostream>
#ifdef LL_STANDALONE
# include <zlib.h>
#else
# include "zlib-ng/zlib.h"
#endif
#if !LL_WINDOWS
#include <netinet/in.h>
#endif
#include "apr_general.h"
#include "lldate.h"
#include "llsd.h"
#include "llstring.h"
#include "lluri.h"
static const int MAX_HDR_LEN = 20;
static const char LEGACY_NON_HEADER[] = "<llsd>";
const std::string LLSD_BINARY_HEADER("LLSD/Binary");
const std::string LLSD_XML_HEADER("LLSD/XML");
const std::string LLSD_NOTATION_HEADER("llsd/notation");
#define windowBits 15
#define ENABLE_ZLIB_GZIP 32
void LLSDSerialize::serialize(const LLSD& sd, std::ostream& str, ELLSD_Serialize type, U32 options)
{
	LLPointer<LLSDFormatter> f = NULL;
	switch (type)
	{
	case LLSD_BINARY:
		str << "<? " << LLSD_BINARY_HEADER << " ?>\n";
		f = new LLSDBinaryFormatter;
		break;
	case LLSD_XML:
		str << "<? " << LLSD_XML_HEADER << " ?>\n";
		f = new LLSDXMLFormatter;
		break;
    case LLSD_NOTATION:
        str << "<? " << LLSD_NOTATION_HEADER << " ?>\n";
        f = new LLSDNotationFormatter;
        break;
	default:
		LL_WARNS() << "serialize request for unknown ELLSD_Serialize" << LL_ENDL;
	}
	if (f.notNull())
	{
		f->format(sd, str, options);
	}
}
bool LLSDSerialize::deserialize(LLSD& sd, std::istream& str, S32 max_bytes)
{
	LLPointer<LLSDParser> p = NULL;
	char hdr_buf[MAX_HDR_LEN + 1] = "";
	int i;
	int inbuf = 0;
	bool legacy_no_header = false;
	bool fail_if_not_legacy = false;
	std::string header;
	str.get(hdr_buf, MAX_HDR_LEN, '\n');
	if (str.fail())
	{
		str.clear();
		fail_if_not_legacy = true;
	}
	if (!strnicmp(LEGACY_NON_HEADER, hdr_buf, strlen(LEGACY_NON_HEADER)))
	{
		legacy_no_header = true;
		inbuf = (int)str.gcount();
	}
	else
	{
		if (fail_if_not_legacy)
			goto fail;
		for (i = 0; i < MAX_HDR_LEN; i++)
		{
			if (hdr_buf[i] == 0 || hdr_buf[i] == '\r' ||
				hdr_buf[i] == '\n')
			{
				hdr_buf[i] = 0;
				break;
			}
		}
		header = hdr_buf;
		std::string::size_type start = header.find_first_not_of("<? ");
		std::string::size_type end = std::string::npos;
		if (start != std::string::npos)
		{
			end = header.find_first_of(" ?", start);
		}
		if ((start == std::string::npos) || (end == std::string::npos))
			goto fail;
		header = header.substr(start, end - start);
		ws(str);
	}
	if (legacy_no_header)
	{
		LLSDXMLParser* x = new LLSDXMLParser();
		x->parsePart(hdr_buf, inbuf);
		x->parseLines(str, sd);
		delete x;
		return true;
	}
	if (header == LLSD_BINARY_HEADER)
	{
		p = new LLSDBinaryParser;
	}
	else if (header == LLSD_XML_HEADER)
	{
		p = new LLSDXMLParser;
	}
    else if (header == LLSD_NOTATION_HEADER)
    {
        p = new LLSDNotationParser;
    }
	else
	{
		LL_WARNS() << "deserialize request for unknown ELLSD_Serialize" << LL_ENDL;
	}
	if (p.notNull())
	{
		p->parse(str, sd, max_bytes);
		return true;
	}
fail:
	LL_WARNS() << "deserialize LLSD parse failure" << LL_ENDL;
	return false;
}
#if LL_BIG_ENDIAN
U64 ll_htonll(U64 hostlonglong) { return hostlonglong; }
U64 ll_ntohll(U64 netlonglong) { return netlonglong; }
F64 ll_htond(F64 hostlonglong) { return hostlonglong; }
F64 ll_ntohd(F64 netlonglong) { return netlonglong; }
#else
U64 ll_htonll(U64 hostlonglong)
{
	return ((U64)(htonl((U32)((hostlonglong >> 32) & 0xFFFFFFFF))) |
			((U64)(htonl((U32)(hostlonglong & 0xFFFFFFFF))) << 32));
}
U64 ll_ntohll(U64 netlonglong)
{
	return ((U64)(ntohl((U32)((netlonglong >> 32) & 0xFFFFFFFF))) |
			((U64)(ntohl((U32)(netlonglong & 0xFFFFFFFF))) << 32));
}
union LLEndianSwapper
{
	F64 d;
	U64 i;
};
F64 ll_htond(F64 hostdouble)
{
	LLEndianSwapper tmp;
	tmp.d = hostdouble;
	tmp.i = ll_htonll(tmp.i);
	return tmp.d;
}
F64 ll_ntohd(F64 netdouble)
{
	LLEndianSwapper tmp;
	tmp.d = netdouble;
	tmp.i = ll_ntohll(tmp.i);
	return tmp.d;
}
#endif
int deserialize_string(std::istream& istr, std::string& value, S32 max_bytes);
int deserialize_string_delim(std::istream& istr, std::string& value, char d);
int deserialize_string_raw(
	std::istream& istr,
	std::string& value,
	S32 max_bytes);
int deserialize_boolean(
	std::istream& istr,
	LLSD& data,
	const std::string& compare,
	bool value);
void serialize_string(const std::string& value, std::ostream& str);
static const std::string NOTATION_TRUE_SERIAL("true");
static const std::string NOTATION_FALSE_SERIAL("false");
static const char BINARY_TRUE_SERIAL = '1';
static const char BINARY_FALSE_SERIAL = '0';
LLSDParser::LLSDParser()
	: mCheckLimits(true), mMaxBytesLeft(0), mParseLines(false)
{
}
LLSDParser::~LLSDParser()
{ }
S32 LLSDParser::parse(std::istream& istr, LLSD& data, S32 max_bytes)
{
	mCheckLimits = (LLSDSerialize::SIZE_UNLIMITED == max_bytes) ? false : true;
	mMaxBytesLeft = max_bytes;
	return doParse(istr, data);
}
S32 LLSDParser::parseLines(std::istream& istr, LLSD& data)
{
	mCheckLimits = false;
	mParseLines = true;
	return doParse(istr, data);
}
int LLSDParser::get(std::istream& istr) const
{
	if(mCheckLimits) --mMaxBytesLeft;
	return istr.get();
}
std::istream& LLSDParser::get(
	std::istream& istr,
	char* s,
	std::streamsize n,
	char delim) const
{
	istr.get(s, n, delim);
	if(mCheckLimits) mMaxBytesLeft -= (int)istr.gcount();
	return istr;
}
std::istream& LLSDParser::get(
		std::istream& istr,
		std::streambuf& sb,
		char delim) const
{
	istr.get(sb, delim);
	if(mCheckLimits) mMaxBytesLeft -= (int)istr.gcount();
	return istr;
}
std::istream& LLSDParser::ignore(std::istream& istr) const
{
	istr.ignore();
	if(mCheckLimits) --mMaxBytesLeft;
	return istr;
}
std::istream& LLSDParser::putback(std::istream& istr, char c) const
{
	istr.putback(c);
	if(mCheckLimits) ++mMaxBytesLeft;
	return istr;
}
std::istream& LLSDParser::read(
	std::istream& istr,
	char* s,
	std::streamsize n) const
{
	istr.read(s, n);
	if(mCheckLimits) mMaxBytesLeft -= (int)istr.gcount();
	return istr;
}
void LLSDParser::account(S32 bytes) const
{
	if(mCheckLimits) mMaxBytesLeft -= bytes;
}
LLSDNotationParser::LLSDNotationParser()
{
}
LLSDNotationParser::~LLSDNotationParser()
{ }
S32 LLSDNotationParser::doParse(std::istream& istr, LLSD& data) const
{
	char c;
	c = istr.peek();
	while(isspace(c))
	{
		c = get(istr);
		c = istr.peek();
		continue;
	}
	if(!istr.good())
	{
		return 0;
	}
	S32 parse_count = 1;
	switch(c)
	{
	case '{':
	{
		S32 child_count = parseMap(istr, data);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading map." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case '[':
	{
		S32 child_count = parseArray(istr, data);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading array." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case '!':
		c = get(istr);
		data.clear();
		break;
	case '0':
		c = get(istr);
		data = false;
		break;
	case 'F':
	case 'f':
		ignore(istr);
		c = istr.peek();
		if(isalpha(c))
		{
			int cnt = deserialize_boolean(
				istr,
				data,
				NOTATION_FALSE_SERIAL,
				false);
			if(PARSE_FAILURE == cnt) parse_count = cnt;
			else account(cnt);
		}
		else
		{
			data = false;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading boolean." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	case '1':
		c = get(istr);
		data = true;
		break;
	case 'T':
	case 't':
		ignore(istr);
		c = istr.peek();
		if(isalpha(c))
		{
			int cnt = deserialize_boolean(istr,data,NOTATION_TRUE_SERIAL,true);
			if(PARSE_FAILURE == cnt) parse_count = cnt;
			else account(cnt);
		}
		else
		{
			data = true;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading boolean." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	case 'i':
	{
		c = get(istr);
		S32 integer = 0;
		istr >> integer;
		data = integer;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading integer." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 'r':
	{
		c = get(istr);
		F64 real = 0.0;
		istr >> real;
		data = real;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading real." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 'u':
	{
		c = get(istr);
		LLUUID id;
		istr >> id;
		data = id;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading uuid." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case '\"':
	case '\'':
	case 's':
		if(!parseString(istr, data))
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading string." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	case 'l':
	{
		c = get(istr);
		c = get(istr);
		std::string str;
		int cnt = deserialize_string_delim(istr, str, c);
		if(PARSE_FAILURE == cnt)
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			data = LLURI(str);
			account(cnt);
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading link." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 'd':
	{
		c = get(istr);
		c = get(istr);
		std::string str;
		int cnt = deserialize_string_delim(istr, str, c);
		if(PARSE_FAILURE == cnt)
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			data = LLDate(str);
			account(cnt);
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading date." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 'b':
		if(!parseBinary(istr, data))
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading data." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	default:
		parse_count = PARSE_FAILURE;
		LL_INFOS() << "Unrecognized character while parsing: int(" << (int)c
			<< ")" << LL_ENDL;
		break;
	}
	if(PARSE_FAILURE == parse_count)
	{
		data.clear();
	}
	return parse_count;
}
S32 LLSDNotationParser::parseMap(std::istream& istr, LLSD& map) const
{
	map = LLSD::emptyMap();
	S32 parse_count = 0;
	char c = get(istr);
	if(c == '{')
	{
		bool found_name = false;
		std::string name;
		c = get(istr);
		while(c != '}' && istr.good())
		{
			if(!found_name)
			{
				if((c == '\"') || (c == '\'') || (c == 's'))
				{
					putback(istr, c);
					found_name = true;
					int count = deserialize_string(istr, name, mMaxBytesLeft);
					if(PARSE_FAILURE == count) return PARSE_FAILURE;
					account(count);
				}
				c = get(istr);
			}
			else
			{
				if(isspace(c) || (c == ':'))
				{
					c = get(istr);
					continue;
				}
				putback(istr, c);
				LLSD child;
				S32 count = doParse(istr, child);
				if(count > 0)
				{
					parse_count += count;
					map.insert(name, child);
				}
				else
				{
					return PARSE_FAILURE;
				}
				found_name = false;
				c = get(istr);
			}
		}
		if(c != '}')
		{
			map.clear();
			return PARSE_FAILURE;
		}
	}
	return parse_count;
}
S32 LLSDNotationParser::parseArray(std::istream& istr, LLSD& array) const
{
	array = LLSD::emptyArray();
	S32 parse_count = 0;
	char c = get(istr);
	if(c == '[')
	{
		c = get(istr);
		while((c != ']') && istr.good())
		{
			LLSD child;
			if(isspace(c) || (c == ','))
			{
				c = get(istr);
				continue;
			}
			putback(istr, c);
			S32 count = doParse(istr, child);
			if(PARSE_FAILURE == count)
			{
				return PARSE_FAILURE;
			}
			else
			{
				parse_count += count;
				array.append(child);
			}
			c = get(istr);
		}
		if(c != ']')
		{
			return PARSE_FAILURE;
		}
	}
	return parse_count;
}
bool LLSDNotationParser::parseString(std::istream& istr, LLSD& data) const
{
	std::string value;
	int count = deserialize_string(istr, value, mMaxBytesLeft);
	if(PARSE_FAILURE == count) return false;
	account(count);
	data = value;
	return true;
}
bool LLSDNotationParser::parseBinary(std::istream& istr, LLSD& data) const
{
	const U32 BINARY_BUFFER_SIZE = 256;
	const U32 STREAM_GET_COUNT = 255;
	char buf[BINARY_BUFFER_SIZE];
	get(istr, buf, STREAM_GET_COUNT, '"');
	char c = get(istr);
	if(c != '"') return false;
	if(0 == strncmp("b(", buf, 2))
	{
		S32 len = strtol(buf + 2, NULL, 0);
		if(mCheckLimits && (len > mMaxBytesLeft)) return false;
		std::vector<U8> value;
		if(len)
		{
			value.resize(len);
			account((int)fullread(istr, (char *)&value[0], len));
		}
		c = get(istr);
		data = value;
	}
	else if(0 == strncmp("b64", buf, 3))
	{
		std::stringstream coded_stream;
		get(istr, *(coded_stream.rdbuf()), '\"');
		c = get(istr);
		std::string encoded(coded_stream.str());
		size_t len = LLBase64::requiredDecryptionSpace(encoded);
		std::vector<U8> value;
		if(len)
		{
			value.resize(len);
			len = LLBase64::decode(encoded, &value[0], len);
			value.resize(len);
		}
		data = value;
	}
	else if(0 == strncmp("b16", buf, 3))
	{
		char* read;
		U8 byte;
		U8 byte_buffer[BINARY_BUFFER_SIZE];
		U8* write;
		std::vector<U8> value;
		c = get(istr);
		while(c != '"')
		{
			putback(istr, c);
			read = buf;
			write = byte_buffer;
			get(istr, buf, STREAM_GET_COUNT, '"');
			c = get(istr);
			while(*read != '\0')
			{
				byte = hex_as_nybble(*read++);
				byte = byte << 4;
				byte |= hex_as_nybble(*read++);
				*write++ = byte;
			}
			value.insert(value.end(), byte_buffer, write);
		}
		data = value;
	}
	else
	{
		return false;
	}
	return true;
}
LLSDBinaryParser::LLSDBinaryParser()
{
}
LLSDBinaryParser::~LLSDBinaryParser()
{
}
S32 LLSDBinaryParser::doParse(std::istream& istr, LLSD& data) const
{
	char c;
	c = get(istr);
	if(!istr.good())
	{
		return 0;
	}
	S32 parse_count = 1;
	switch(c)
	{
	case '{':
	{
		S32 child_count = parseMap(istr, data);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary map." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case '[':
	{
		S32 child_count = parseArray(istr, data);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary array." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case '!':
		data.clear();
		break;
	case '0':
		data = false;
		break;
	case '1':
		data = true;
		break;
	case 'i':
	{
		U32 value_nbo = 0;
		read(istr, (char*)&value_nbo, sizeof(U32));
		data = (S32)ntohl(value_nbo);
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary integer." << LL_ENDL;
		}
		break;
	}
	case 'r':
	{
		F64 real_nbo = 0.0;
		read(istr, (char*)&real_nbo, sizeof(F64));
		data = ll_ntohd(real_nbo);
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary real." << LL_ENDL;
		}
		break;
	}
	case 'u':
	{
		LLUUID id;
		read(istr, (char*)(&id.mData), UUID_BYTES);
		data = id;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary uuid." << LL_ENDL;
		}
		break;
	}
	case '\'':
	case '"':
	{
		std::string value;
		int cnt = deserialize_string_delim(istr, value, c);
		if(PARSE_FAILURE == cnt)
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			data = value;
			account(cnt);
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary (notation-style) string."
				<< LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 's':
	{
		std::string value;
		if(parseString(istr, value))
		{
			data = value;
		}
		else
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary string." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 'l':
	{
		std::string value;
		if(parseString(istr, value))
		{
			data = LLURI(value);
		}
		else
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary link." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 'd':
	{
		F64 real = 0.0;
		read(istr, (char*)&real, sizeof(F64));
		data = LLDate(real);
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary date." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	case 'b':
	{
		U32 size_nbo = 0;
		read(istr, (char*)&size_nbo, sizeof(U32));
		S32 size = (S32)ntohl(size_nbo);
		if(size < 0 || mCheckLimits && (size > mMaxBytesLeft))
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			std::vector<U8> value;
			if(size > 0)
			{
				value.resize(size);
				account((int)fullread(istr, (char*)&value[0], size));
			}
			data = value;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}
	default:
		parse_count = PARSE_FAILURE;
		LL_INFOS() << "Unrecognized character while parsing: int(" << (int)c
			<< ")" << LL_ENDL;
		break;
	}
	if(PARSE_FAILURE == parse_count)
	{
		data.clear();
	}
	return parse_count;
}
S32 LLSDBinaryParser::parseMap(std::istream& istr, LLSD& map) const
{
	map = LLSD::emptyMap();
	U32 value_nbo = 0;
	read(istr, (char*)&value_nbo, sizeof(U32));
	S32 size = (S32)ntohl(value_nbo);
	if (size < 0)
	{
		return PARSE_FAILURE;
	}
	S32 parse_count = 0;
	S32 count = 0;
	char c = get(istr);
	while(c != '}' && (count < size) && istr.good())
	{
		std::string name;
		switch(c)
		{
		case 'k':
			if(!parseString(istr, name))
			{
				return PARSE_FAILURE;
			}
			break;
		case '\'':
		case '"':
		{
			int cnt = deserialize_string_delim(istr, name, c);
			if(PARSE_FAILURE == cnt) return PARSE_FAILURE;
			account(cnt);
			break;
		}
		}
		LLSD child;
		S32 child_count = doParse(istr, child);
		if(child_count > 0)
		{
			parse_count += child_count;
			map.insert(name, child);
		}
		else
		{
			return PARSE_FAILURE;
		}
		++count;
		c = get(istr);
	}
	if((c != '}') || (count < size))
	{
		return PARSE_FAILURE;
	}
	return parse_count;
}
S32 LLSDBinaryParser::parseArray(std::istream& istr, LLSD& array) const
{
	array = LLSD::emptyArray();
	U32 value_nbo = 0;
	read(istr, (char*)&value_nbo, sizeof(U32));
	S32 size = (S32)ntohl(value_nbo);
	if (size < 0)
	{
		return PARSE_FAILURE;
	}
	S32 parse_count = 0;
	S32 count = 0;
	char c = istr.peek();
	while((c != ']') && (count < size) && istr.good())
	{
		LLSD child;
		S32 child_count = doParse(istr, child);
		if(PARSE_FAILURE == child_count)
		{
			return PARSE_FAILURE;
		}
		if(child_count)
		{
			parse_count += child_count;
			array.append(child);
		}
		++count;
		c = istr.peek();
	}
	c = get(istr);
	if((c != ']') || (count < size))
	{
		return PARSE_FAILURE;
	}
	return parse_count;
}
bool LLSDBinaryParser::parseString(
	std::istream& istr,
	std::string& value) const
{
	U32 value_nbo = 0;
	read(istr, (char*)&value_nbo, sizeof(U32));
	S32 size = (S32)ntohl(value_nbo);
	if(size < 0 || mCheckLimits && (size > mMaxBytesLeft)) return false;
	std::vector<char> buf;
	if(size)
	{
		buf.resize(size);
		account((int)fullread(istr, &buf[0], size));
		value.assign(buf.begin(), buf.end());
	}
	return true;
}
LLSDFormatter::LLSDFormatter() :
	mBoolAlpha(false)
{
}
LLSDFormatter::~LLSDFormatter()
{ }
void LLSDFormatter::boolalpha(bool alpha)
{
	mBoolAlpha = alpha;
}
void LLSDFormatter::realFormat(const std::string& format)
{
	mRealFormat = format;
}
void LLSDFormatter::formatReal(LLSD::Real real, std::ostream& ostr) const
{
	std::string buffer = llformat(mRealFormat.c_str(), real);
	ostr << buffer;
}
LLSDNotationFormatter::LLSDNotationFormatter()
{
}
LLSDNotationFormatter::~LLSDNotationFormatter()
{ }
std::string LLSDNotationFormatter::escapeString(const std::string& in)
{
	std::ostringstream ostr;
	serialize_string(in, ostr);
	return ostr.str();
}
S32 LLSDNotationFormatter::format(const LLSD& data, std::ostream& ostr, U32 options) const
{
	S32 rv = format_impl(data, ostr, options, 0);
	return rv;
}
S32 LLSDNotationFormatter::format_impl(const LLSD& data, std::ostream& ostr, U32 options, U32 level) const
{
	S32 format_count = 1;
	std::string pre;
	std::string post;
	if (options & LLSDFormatter::OPTIONS_PRETTY)
	{
		for (U32 i = 0; i < level; i++)
		{
			pre += "    ";
		}
		post = "\n";
	}
	switch(data.type())
	{
	case LLSD::TypeMap:
	{
		if (0 != level) ostr << post << pre;
		ostr << "{";
		std::string inner_pre;
		if (options & LLSDFormatter::OPTIONS_PRETTY)
		{
			inner_pre = pre + "    ";
		}
		bool need_comma = false;
        auto iter = data.beginMap();
        auto end = data.endMap();
		for(; iter != end; ++iter)
		{
			if(need_comma) ostr << ",";
			need_comma = true;
			ostr << post << inner_pre << '\'';
			serialize_string((*iter).first, ostr);
			ostr << "':";
			format_count += format_impl((*iter).second, ostr, options, level + 2);
		}
		ostr << post << pre << "}";
		break;
	}
	case LLSD::TypeArray:
	{
		ostr << post << pre << "[";
		bool need_comma = false;
		for (const auto& entry : data.array())
		{
			if (need_comma) ostr << ",";
			need_comma = true;
			format_count += format_impl(entry, ostr, options, level + 1);
		}
		ostr << "]";
		break;
	}
	case LLSD::TypeUndefined:
		ostr << "!";
		break;
	case LLSD::TypeBoolean:
		if(mBoolAlpha || (ostr.flags() & std::ios::boolalpha))
		{
			ostr << (data.asBoolean()
					 ? NOTATION_TRUE_SERIAL : NOTATION_FALSE_SERIAL);
		}
		else
		{
			ostr << (data.asBoolean() ? 1 : 0);
		}
		break;
	case LLSD::TypeInteger:
		ostr << "i" << data.asInteger();
		break;
	case LLSD::TypeReal:
		ostr << "r";
		if(mRealFormat.empty())
		{
			ostr << data.asReal();
		}
		else
		{
			formatReal(data.asReal(), ostr);
		}
		break;
	case LLSD::TypeUUID:
		ostr << "u" << data.asUUID();
		break;
	case LLSD::TypeString:
		ostr << '\'';
		serialize_string(data.asStringRef(), ostr);
		ostr << '\'';
		break;
	case LLSD::TypeDate:
		ostr << "d\"" << data.asDate() << "\"";
		break;
	case LLSD::TypeURI:
		ostr << "l\"";
		serialize_string(data.asString(), ostr);
		ostr << "\"";
		break;
	case LLSD::TypeBinary:
	{
		const std::vector<U8>& buffer = data.asBinary();
		ostr << "b(" << buffer.size() << ")\"";
		if(!buffer.empty())
		{
			if (options & LLSDFormatter::OPTIONS_PRETTY_BINARY)
			{
				std::ios_base::fmtflags old_flags = ostr.flags();
				ostr.setf( std::ios::hex, std::ios::basefield );
				ostr << "0x";
				for (unsigned char i : buffer)
				{
					ostr << static_cast<int>(i);
				}
				ostr.flags(old_flags);
			}
			else
			{
				ostr.write(reinterpret_cast<const char*>(&buffer[0]), buffer.size());
			}
		}
		ostr << "\"";
		break;
	}
	default:
		ostr << "!";
		break;
	}
	return format_count;
}
LLSDBinaryFormatter::LLSDBinaryFormatter()
{
}
LLSDBinaryFormatter::~LLSDBinaryFormatter()
{ }
S32 LLSDBinaryFormatter::format(const LLSD& data, std::ostream& ostr, U32 options) const
{
	S32 format_count = 1;
	switch(data.type())
	{
	case LLSD::TypeMap:
	{
		ostr.put('{');
		U32 size_nbo = htonl(data.size());
		ostr.write(reinterpret_cast<const char*>(&size_nbo), sizeof(U32));
        auto iter = data.beginMap();
        auto end = data.endMap();
		for(; iter != end; ++iter)
		{
			ostr.put('k');
			formatString((*iter).first, ostr);
			format_count += format((*iter).second, ostr);
		}
		ostr.put('}');
		break;
	}
	case LLSD::TypeArray:
	{
		ostr.put('[');
		U32 size_nbo = htonl(data.size());
		ostr.write(reinterpret_cast<const char*>(&size_nbo), sizeof(U32));
		for (const auto& entry : data.array())
		{
			format_count += format(entry, ostr);
		}
		ostr.put(']');
		break;
	}
	case LLSD::TypeUndefined:
		ostr.put('!');
		break;
	case LLSD::TypeBoolean:
		if(data.asBoolean()) ostr.put(BINARY_TRUE_SERIAL);
		else ostr.put(BINARY_FALSE_SERIAL);
		break;
	case LLSD::TypeInteger:
	{
		ostr.put('i');
		U32 value_nbo = htonl(data.asInteger());
		ostr.write((const char*)(&value_nbo), sizeof(U32));
		break;
	}
	case LLSD::TypeReal:
	{
		ostr.put('r');
		F64 value_nbo = ll_htond(data.asReal());
		ostr.write((const char*)(&value_nbo), sizeof(F64));
		break;
	}
	case LLSD::TypeUUID:
	{
		ostr.put('u');
		LLUUID temp = data.asUUID();
		ostr.write((const char*)(&(temp.mData)), UUID_BYTES);
		break;
	}
	case LLSD::TypeString:
		ostr.put('s');
		formatString(data.asStringRef(), ostr);
		break;
	case LLSD::TypeDate:
	{
		ostr.put('d');
		F64 value = data.asReal();
		ostr.write((const char*)(&value), sizeof(F64));
		break;
	}
	case LLSD::TypeURI:
		ostr.put('l');
		formatString(data.asString(), ostr);
		break;
	case LLSD::TypeBinary:
	{
		ostr.put('b');
		const std::vector<U8>& buffer = data.asBinary();
		U32 size_nbo = htonl(buffer.size());
		ostr.write((const char*)(&size_nbo), sizeof(U32));
		if(!buffer.empty()) ostr.write((const char*)&buffer[0], buffer.size());
		break;
	}
	default:
		ostr.put('!');
		break;
	}
	return format_count;
}
void LLSDBinaryFormatter::formatString(
	const std::string& string,
	std::ostream& ostr) const
{
	U32 size_nbo = htonl(string.size());
	ostr.write((const char*)(&size_nbo), sizeof(U32));
	ostr.write(string.c_str(), string.size());
}
int deserialize_string(std::istream& istr, std::string& value, S32 max_bytes)
{
	int c = istr.get();
	if(istr.fail())
	{
		return LLSDParser::PARSE_FAILURE;
	}
	int rv = LLSDParser::PARSE_FAILURE;
	switch(c)
	{
	case '\'':
	case '"':
		rv = deserialize_string_delim(istr, value, c);
		break;
	case 's':
		rv = deserialize_string_raw(istr, value, max_bytes);
		break;
	default:
		break;
	}
	if(LLSDParser::PARSE_FAILURE == rv) return rv;
	return rv + 1;
}
int deserialize_string_delim(
	std::istream& istr,
	std::string& value,
	char delim)
{
	std::ostringstream write_buffer;
	bool found_escape = false;
	bool found_hex = false;
	bool found_digit = false;
	U8 byte = 0;
	int count = 0;
	while (true)
	{
		int next_byte = istr.get();
		++count;
		if(istr.fail())
		{
			value = write_buffer.str();
			return LLSDParser::PARSE_FAILURE;
		}
		char next_char = (char)next_byte;
		if(found_escape)
		{
			if(found_hex)
			{
				if(found_digit)
				{
					found_digit = false;
					found_hex = false;
					found_escape = false;
					byte = byte << 4;
					byte |= hex_as_nybble(next_char);
					write_buffer << byte;
					byte = 0;
				}
				else
				{
					found_digit = true;
					byte = hex_as_nybble(next_char);
				}
			}
			else if(next_char == 'x')
			{
				found_hex = true;
			}
			else
			{
				switch(next_char)
				{
				case 'a':
					write_buffer << '\a';
					break;
				case 'b':
					write_buffer << '\b';
					break;
				case 'f':
					write_buffer << '\f';
					break;
				case 'n':
					write_buffer << '\n';
					break;
				case 'r':
					write_buffer << '\r';
					break;
				case 't':
					write_buffer << '\t';
					break;
				case 'v':
					write_buffer << '\v';
					break;
				default:
					write_buffer << next_char;
					break;
				}
				found_escape = false;
			}
		}
		else if(next_char == '\\')
		{
			found_escape = true;
		}
		else if(next_char == delim)
		{
			break;
		}
		else
		{
			write_buffer << next_char;
		}
	}
	value = write_buffer.str();
	return count;
}
int deserialize_string_raw(
	std::istream& istr,
	std::string& value,
	S32 max_bytes)
{
	int count = 0;
	const S32 BUF_LEN = 20;
	char buf[BUF_LEN];
	istr.get(buf, BUF_LEN - 1, ')');
	count += (int)istr.gcount();
	int c = istr.get();
	c = istr.get();
	count += 2;
	if(((c == '"') || (c == '\'')) && (buf[0] == '('))
	{
		S32 len = strtol(buf + 1, NULL, 0);
		if((max_bytes>0)&&(len>max_bytes)) return LLSDParser::PARSE_FAILURE;
		std::vector<char> buf2;
		if(len)
		{
			buf2.resize(len);
			count += (int)fullread(istr, (char *)&buf2[0], len);
			value.assign(buf2.begin(), buf2.end());
		}
		c = istr.get();
		++count;
		if(!((c == '"') || (c == '\'')))
		{
			return LLSDParser::PARSE_FAILURE;
		}
	}
	else
	{
		return LLSDParser::PARSE_FAILURE;
	}
	return count;
}
static const char* NOTATION_STRING_CHARACTERS[256] =
{
	"\\x00",
	"\\x01",
	"\\x02",
	"\\x03",
	"\\x04",
	"\\x05",
	"\\x06",
	"\\a",
	"\\b",
	"\\t",
	"\\n",
	"\\v",
	"\\f",
	"\\r",
	"\\x0e",
	"\\x0f",
	"\\x10",
	"\\x11",
	"\\x12",
	"\\x13",
	"\\x14",
	"\\x15",
	"\\x16",
	"\\x17",
	"\\x18",
	"\\x19",
	"\\x1a",
	"\\x1b",
	"\\x1c",
	"\\x1d",
	"\\x1e",
	"\\x1f",
	" ",
	"!",
	"\"",
	"#",
	"$",
	"%",
	"&",
	"\\'",
	"(",
	")",
	"*",
	"+",
	",",
	"-",
	".",
	"/",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	":",
	";",
	"<",
	"=",
	">",
	"?",
	"@",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"[",
	"\\\\",
	"]",
	"^",
	"_",
	"`",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"p",
	"q",
	"r",
	"s",
	"t",
	"u",
	"v",
	"w",
	"x",
	"y",
	"z",
	"{",
	"|",
	"}",
	"~",
	"\\x7f",
	"\\x80",
	"\\x81",
	"\\x82",
	"\\x83",
	"\\x84",
	"\\x85",
	"\\x86",
	"\\x87",
	"\\x88",
	"\\x89",
	"\\x8a",
	"\\x8b",
	"\\x8c",
	"\\x8d",
	"\\x8e",
	"\\x8f",
	"\\x90",
	"\\x91",
	"\\x92",
	"\\x93",
	"\\x94",
	"\\x95",
	"\\x96",
	"\\x97",
	"\\x98",
	"\\x99",
	"\\x9a",
	"\\x9b",
	"\\x9c",
	"\\x9d",
	"\\x9e",
	"\\x9f",
	"\\xa0",
	"\\xa1",
	"\\xa2",
	"\\xa3",
	"\\xa4",
	"\\xa5",
	"\\xa6",
	"\\xa7",
	"\\xa8",
	"\\xa9",
	"\\xaa",
	"\\xab",
	"\\xac",
	"\\xad",
	"\\xae",
	"\\xaf",
	"\\xb0",
	"\\xb1",
	"\\xb2",
	"\\xb3",
	"\\xb4",
	"\\xb5",
	"\\xb6",
	"\\xb7",
	"\\xb8",
	"\\xb9",
	"\\xba",
	"\\xbb",
	"\\xbc",
	"\\xbd",
	"\\xbe",
	"\\xbf",
	"\\xc0",
	"\\xc1",
	"\\xc2",
	"\\xc3",
	"\\xc4",
	"\\xc5",
	"\\xc6",
	"\\xc7",
	"\\xc8",
	"\\xc9",
	"\\xca",
	"\\xcb",
	"\\xcc",
	"\\xcd",
	"\\xce",
	"\\xcf",
	"\\xd0",
	"\\xd1",
	"\\xd2",
	"\\xd3",
	"\\xd4",
	"\\xd5",
	"\\xd6",
	"\\xd7",
	"\\xd8",
	"\\xd9",
	"\\xda",
	"\\xdb",
	"\\xdc",
	"\\xdd",
	"\\xde",
	"\\xdf",
	"\\xe0",
	"\\xe1",
	"\\xe2",
	"\\xe3",
	"\\xe4",
	"\\xe5",
	"\\xe6",
	"\\xe7",
	"\\xe8",
	"\\xe9",
	"\\xea",
	"\\xeb",
	"\\xec",
	"\\xed",
	"\\xee",
	"\\xef",
	"\\xf0",
	"\\xf1",
	"\\xf2",
	"\\xf3",
	"\\xf4",
	"\\xf5",
	"\\xf6",
	"\\xf7",
	"\\xf8",
	"\\xf9",
	"\\xfa",
	"\\xfb",
	"\\xfc",
	"\\xfd",
	"\\xfe",
	"\\xff"
};
void serialize_string(const std::string& value, std::ostream& str)
{
	std::string::const_iterator it = value.begin();
	std::string::const_iterator end = value.end();
	U8 c;
	for(; it != end; ++it)
	{
		c = (U8)(*it);
		str << NOTATION_STRING_CHARACTERS[c];
	}
}
int deserialize_boolean(
	std::istream& istr,
	LLSD& data,
	const std::string& compare,
	bool value)
{
	int bytes_read = 0;
	std::string::size_type ii = 0;
	char c = istr.peek();
	while((++ii < compare.size())
		  && (tolower(c) == (int)compare[ii])
		  && istr.good())
	{
		istr.ignore();
		++bytes_read;
		c = istr.peek();
	}
	if(compare.size() != ii)
	{
		data.clear();
		return LLSDParser::PARSE_FAILURE;
	}
	data = value;
	return bytes_read;
}
std::ostream& operator<<(std::ostream& s, const LLSD& llsd)
{
	s << LLSDNotationStreamer(llsd);
	return s;
}
std::string zip_llsd(LLSD& data)
{
	std::stringstream llsd_strm;
	LLSDSerialize::toBinary(data, llsd_strm);
	const U32 CHUNK = 65536;
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	S32 ret = deflateInit(&strm, Z_BEST_COMPRESSION);
	if (ret != Z_OK)
	{
		LL_WARNS() << "Failed to compress LLSD block." << LL_ENDL;
		return std::string();
	}
	std::string source = llsd_strm.str();
	U8 out[CHUNK];
	strm.avail_in = source.size();
	strm.next_in = (U8*) source.data();
	U8* output = NULL;
	U32 cur_size = 0;
	U32 have = 0;
	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = deflate(&strm, Z_FINISH);
		if (ret == Z_OK || ret == Z_STREAM_END)
		{
			if (strm.avail_out >= CHUNK)
			{
				free(output);
				LL_WARNS() << "Failed to compress LLSD block." << LL_ENDL;
				return std::string();
			}
			have = CHUNK-strm.avail_out;
			U8* new_output = (U8*) realloc(output, cur_size+have);
			if (new_output == NULL)
			{
				LL_WARNS() << "Failed to compress LLSD block: can't reallocate memory, current size: " << cur_size << " bytes; requested " << cur_size + have << " bytes." << LL_ENDL;
				deflateEnd(&strm);
				if (output)
				{
					free(output);
				}
				return std::string();
			}
			output = new_output;
			memcpy(output+cur_size, out, have);
			cur_size += have;
		}
		else
		{
			free(output);
			LL_WARNS() << "Failed to compress LLSD block." << LL_ENDL;
			return std::string();
		}
	}
	while (ret == Z_OK);
	std::string::size_type size = cur_size;
	std::string result((char*) output, size);
	deflateEnd(&strm);
	free(output);
	return result;
}
bool unzip_llsd(LLSD& data, std::istream& is, S32 size)
{
	U8* result = NULL;
	U32 cur_size = 0;
	z_stream strm;
	const U32 CHUNK = 65536;
	U8 *in = new U8[size];
	is.read((char*) in, size);
	U8 out[CHUNK];
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = size;
	strm.next_in = in;
	S32 ret = inflateInit(&strm);
	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = inflate(&strm, Z_NO_FLUSH);
		switch (ret)
		{
		case Z_NEED_DICT:
			ret = Z_DATA_ERROR;
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
		case Z_STREAM_ERROR:
			inflateEnd(&strm);
			free(result);
			delete [] in;
			return false;
			break;
		}
		U32 have = CHUNK-strm.avail_out;
		U8* new_result = (U8*)realloc(result, cur_size + have);
		if (new_result == NULL)
		{
			LL_WARNS() << "Failed to unzip LLSD block: can't reallocate memory, current size: " << cur_size << " bytes; requested " << cur_size + have << " bytes." << LL_ENDL;
			inflateEnd(&strm);
			if (result)
			{
				free(result);
			}
			delete in;
			return false;
		}
		result = new_result;
		memcpy(result+cur_size, out, have);
		cur_size += have;
	} while (ret == Z_OK);
	inflateEnd(&strm);
	delete [] in;
	if (ret != Z_STREAM_END)
	{
		free(result);
		return false;
	}
	{
		std::string res_str((char*) result, cur_size);
		std::string deprecated_header("<? LLSD/Binary ?>");
		if (res_str.substr(0, deprecated_header.size()) == deprecated_header)
		{
			res_str = res_str.substr(deprecated_header.size()+1, cur_size);
		}
		cur_size = res_str.size();
		std::istringstream istr(res_str);
		if (!LLSDSerialize::fromBinary(data, istr, cur_size))
		{
			LL_WARNS() << "Failed to unzip LLSD block" << LL_ENDL;
			free(result);
			return false;
		}
	}
	free(result);
	return true;
}
U8* unzip_llsdNavMesh( bool& valid, unsigned int& outsize, std::istream& is, S32 size )
{
	if (size == 0)
	{
		LL_WARNS() << "No data to unzip." << LL_ENDL;
		return NULL;
	}
	U8* result = NULL;
	U32 cur_size = 0;
	z_stream strm;
	const U32 CHUNK = 0x4000;
	U8 *in = new U8[size];
	is.read((char*) in, size);
	U8 out[CHUNK];
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = size;
	strm.next_in = in;
	S32 ret = inflateInit2(&strm,  windowBits | ENABLE_ZLIB_GZIP );
	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = inflate(&strm, Z_NO_FLUSH);
		switch (ret)
		{
		case Z_NEED_DICT:
			ret = Z_DATA_ERROR;
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
		case Z_STREAM_ERROR:
			inflateEnd(&strm);
			free(result);
			delete [] in;
			valid = false;
			return NULL;
		}
		U32 have = CHUNK-strm.avail_out;
		U8* new_result = (U8*) realloc(result, cur_size + have);
		if (new_result == NULL)
		{
			LL_WARNS() << "Failed to unzip LLSD NavMesh block: can't reallocate memory, current size: " << cur_size
				<< " bytes; requested " << cur_size + have
				<< " bytes; total syze: ." << size << " bytes."
				<< LL_ENDL;
			inflateEnd(&strm);
			if (result)
			{
				free(result);
			}
			delete [] in;
			valid = false;
			return NULL;
		}
		result = new_result;
		memcpy(result+cur_size, out, have);
		cur_size += have;
	} while (ret == Z_OK);
	inflateEnd(&strm);
	delete [] in;
	if (ret != Z_STREAM_END)
	{
		free(result);
		valid = false;
		return NULL;
	}
	{
		outsize= cur_size;
		valid = true;
	}
	return result;
}
