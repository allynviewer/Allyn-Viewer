/** 
 * @file llsdutil.cpp
 * @author Phoenix
 * @date 2006-05-24
 * @brief Implementation of classes, functions, etc, for using structured data.
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
#include "llsdutil.h"
#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#elif LL_SOLARIS
#	include <netinet/in.h>
#elif LL_DARWIN
#	include <arpa/inet.h>
#endif
#include "llsdserialize.h"
#include "stringize.h"
#include "is_approx_equal_fraction.h"
#include <map>
#include <set>
LLSD ll_sd_from_U32(const U32 val)
{
	std::vector<U8> v;
	U32 net_order = htonl(val);
	v.resize(4);
	memcpy(&(v[0]), &net_order, 4);
	return LLSD(v);
}
U32 ll_U32_from_sd(const LLSD& sd)
{
	U32 ret;
	std::vector<U8> v = sd.asBinary();
	if (v.size() < 4)
	{
		return 0;
	}
	memcpy(&ret, &(v[0]), 4);
	ret = ntohl(ret);
	return ret;
}
LLSD ll_sd_from_U64(const U64 val)
{
	std::vector<U8> v;
	U32 high, low;
	high = (U32)(val >> 32);
	low = (U32)val;
	high = htonl(high);
	low = htonl(low);
	v.resize(8);
	memcpy(&(v[0]), &high, 4);
	memcpy(&(v[4]), &low, 4);
	return LLSD(v);
}
U64 ll_U64_from_sd(const LLSD& sd)
{
	U32 high, low;
	std::vector<U8> v = sd.asBinary();
	if (v.size() < 8)
	{
		return 0;
	}
	memcpy(&high, &(v[0]), 4);
	memcpy(&low, &(v[4]), 4);
	high = ntohl(high);
	low = ntohl(low);
	return ((U64)high) << 32 | low;
}
LLSD ll_sd_from_ipaddr(const U32 val)
{
	std::vector<U8> v;
	v.resize(4);
	memcpy(&(v[0]), &val, 4);
	return LLSD(v);
}
U32 ll_ipaddr_from_sd(const LLSD& sd)
{
	U32 ret;
	std::vector<U8> v = sd.asBinary();
	if (v.size() < 4)
	{
		return 0;
	}
	memcpy(&ret, &(v[0]), 4);
	return ret;
}
LLSD ll_string_from_binary(const LLSD& sd)
{
	std::vector<U8> value = sd.asBinary();
	std::string str;
	str.resize(value.size());
	memcpy(&str[0], &value[0], value.size());
	return str;
}
LLSD ll_binary_from_string(const LLSD& sd)
{
	std::vector<U8> binary_value;
	std::string string_value = sd.asString();
	for (std::string::iterator iter = string_value.begin();
		 iter != string_value.end(); ++iter)
	{
		binary_value.push_back(*iter);
	}
	binary_value.push_back('\0');
	return binary_value;
}
char* ll_print_sd(const LLSD& sd)
{
	const U32 bufferSize = 10 * 1024;
	static char buffer[bufferSize];
	std::ostringstream stream;
	stream << LLSDOStreamer<LLSDXMLFormatter>(sd);
	stream << std::ends;
	strncpy(buffer, stream.str().c_str(), bufferSize);
	buffer[bufferSize - 1] = '\0';
	return buffer;
}
char* ll_pretty_print_sd_ptr(const LLSD* sd)
{
	if (sd)
	{
		return ll_pretty_print_sd(*sd);
	}
	return NULL;
}
char* ll_pretty_print_sd(const LLSD& sd)
{
	const U32 bufferSize = 100 * 1024;
	static char buffer[bufferSize];
	std::ostringstream stream;
	stream << LLSDOStreamer<LLSDXMLFormatter>(sd, LLSDFormatter::OPTIONS_PRETTY);
	stream << std::ends;
	strncpy(buffer, stream.str().c_str(), bufferSize);
	buffer[bufferSize - 1] = '\0';
	return buffer;
}
std::string ll_stream_notation_sd(const LLSD& sd)
{
	std::ostringstream stream;
	stream << LLSDOStreamer<LLSDNotationFormatter>(sd);
    return stream.str();
}
BOOL compare_llsd_with_template(
	const LLSD& llsd_to_test,
	const LLSD& template_llsd,
	LLSD& resultant_llsd)
{
	if (
		llsd_to_test.isUndefined() &&
		template_llsd.isDefined() )
	{
		resultant_llsd = template_llsd;
		return TRUE;
	}
	else if ( llsd_to_test.type() != template_llsd.type() )
	{
		resultant_llsd = LLSD();
		return FALSE;
	}
	if ( llsd_to_test.isArray() )
	{
		LLSD data;
		LLSD::array_const_iterator test_iter;
		LLSD::array_const_iterator template_iter;
		resultant_llsd = LLSD::emptyArray();
		test_iter = llsd_to_test.beginArray();
		for (
			template_iter = template_llsd.beginArray();
			(template_iter != template_llsd.endArray() &&
			 test_iter != llsd_to_test.endArray());
			++template_iter)
		{
			if ( !compare_llsd_with_template(
					 *test_iter,
					 *template_iter,
					 data) )
			{
				resultant_llsd = LLSD();
				return FALSE;
			}
			else
			{
				resultant_llsd.append(data);
			}
			++test_iter;
		}
		for (;
			 template_iter != template_llsd.endArray();
			 ++template_iter)
		{
			resultant_llsd.append(*template_iter);
		}
	}
	else if ( llsd_to_test.isMap() )
	{
		LLSD value;
		LLSD::map_const_iterator template_iter;
		resultant_llsd = LLSD::emptyMap();
		for (
			template_iter = template_llsd.beginMap();
			template_iter != template_llsd.endMap();
			++template_iter)
		{
			if ( llsd_to_test.has(template_iter->first) )
			{
				if ( !compare_llsd_with_template(
						 llsd_to_test[template_iter->first],
						 template_iter->second,
						 value) )
				{
					resultant_llsd = LLSD();
					return FALSE;
				}
				else
				{
					resultant_llsd[template_iter->first] = value;
				}
			}
			else
			{
				resultant_llsd[template_iter->first] =
					template_iter->second;
			}
		}
	}
	else
	{
		resultant_llsd = llsd_to_test;
	}
	return TRUE;
}
bool filter_llsd_with_template(
	const LLSD & llsd_to_test,
	const LLSD & template_llsd,
	LLSD & resultant_llsd)
{
	if (llsd_to_test.isUndefined() && template_llsd.isDefined())
	{
		resultant_llsd = template_llsd;
		return true;
	}
	else if (llsd_to_test.type() != template_llsd.type())
	{
		resultant_llsd = LLSD();
		return false;
	}
	if (llsd_to_test.isArray())
	{
		LLSD data;
		LLSD::array_const_iterator test_iter;
		LLSD::array_const_iterator template_iter;
		resultant_llsd = LLSD::emptyArray();
		test_iter = llsd_to_test.beginArray();
		if (1 == template_llsd.size())
		{
			template_iter = template_llsd.beginArray();
			for (; test_iter != llsd_to_test.endArray(); ++test_iter)
			{
				if (! filter_llsd_with_template(*test_iter, *template_iter, data))
				{
					resultant_llsd = LLSD();
					return false;
				}
				else
				{
					resultant_llsd.append(data);
				}
			}
		}
		else
		{
			for (template_iter = template_llsd.beginArray();
				 template_iter != template_llsd.endArray() &&
					 test_iter != llsd_to_test.endArray();
				 ++template_iter, ++test_iter)
			{
				if (! filter_llsd_with_template(*test_iter, *template_iter, data))
				{
					resultant_llsd = LLSD();
					return false;
				}
				else
				{
					resultant_llsd.append(data);
				}
			}
			for (;
				 template_iter != template_llsd.endArray();
				 ++template_iter)
			{
				resultant_llsd.append(*template_iter);
			}
		}
	}
	else if (llsd_to_test.isMap())
	{
		resultant_llsd = LLSD::emptyMap();
		const LLSD::String wildcard_tag("*");
		const bool template_has_wildcard = template_llsd.has(wildcard_tag);
		LLSD wildcard_value;
		LLSD value;
		const LLSD::map_const_iterator template_iter_end(template_llsd.endMap());
		for (LLSD::map_const_iterator template_iter(template_llsd.beginMap());
			 template_iter_end != template_iter;
			 ++template_iter)
		{
			if (wildcard_tag == template_iter->first)
			{
				wildcard_value = template_iter->second;
			}
			else if (llsd_to_test.has(template_iter->first))
			{
				if (! filter_llsd_with_template(llsd_to_test[template_iter->first],
												template_iter->second,
												value))
				{
					resultant_llsd = LLSD();
					return false;
				}
				else
				{
					resultant_llsd[template_iter->first] = value;
				}
			}
			else if (! template_has_wildcard)
			{
				resultant_llsd[template_iter->first] = template_iter->second;
			}
		}
		if (template_has_wildcard)
		{
			LLSD sub_value;
			LLSD::map_const_iterator test_iter;
			for (test_iter = llsd_to_test.beginMap();
				 test_iter != llsd_to_test.endMap();
				 ++test_iter)
			{
				if (resultant_llsd.has(test_iter->first))
				{
					continue;
				}
				else if (! filter_llsd_with_template(test_iter->second,
													 wildcard_value,
													 sub_value))
				{
					resultant_llsd = LLSD();
					return false;
				}
				else
				{
					resultant_llsd[test_iter->first] = sub_value;
				}
			}
		}
	}
	else
	{
		resultant_llsd = llsd_to_test;
	}
	return true;
}
struct Data
{
    LLSD::Type type;
    const char* name;
} typedata[] =
{
#define def(type) { LLSD::type, #type + 4 }
    def(TypeUndefined),
    def(TypeBoolean),
    def(TypeInteger),
    def(TypeReal),
    def(TypeString),
    def(TypeUUID),
    def(TypeDate),
    def(TypeURI),
    def(TypeBinary),
    def(TypeMap),
    def(TypeArray)
#undef  def
};
class TypeLookup
{
    typedef std::map<LLSD::Type, std::string> MapType;
public:
    TypeLookup()
    {
        for (const Data *di(std::begin(typedata)), *dend(std::end(typedata)); di != dend; ++di)
        {
            mMap[di->type] = di->name;
        }
    }
    std::string lookup(LLSD::Type type) const
    {
        MapType::const_iterator found = mMap.find(type);
        if (found != mMap.end())
        {
            return found->second;
        }
        return STRINGIZE("<unknown LLSD type " << type << ">");
    }
private:
    MapType mMap;
};
static const TypeLookup sTypes;
const std::string op(" required instead of ");
static std::string colon(const std::string& pfx)
{
    if (pfx.empty())
        return pfx;
    return pfx + ": ";
}
typedef std::vector<LLSD::Type> TypeVector;
static std::string match_types(LLSD::Type expect,
                               const TypeVector& accept,
                               LLSD::Type actual,
                               const std::string& pfx)
{
    if (actual == expect)
        return "";
    std::ostringstream out;
    out << colon(pfx) << sTypes.lookup(expect);
    if (! accept.empty())
    {
        out << " (";
        const char* sep = "or ";
        for (TypeVector::const_iterator ai(accept.begin()), aend(accept.end());
             ai != aend; ++ai, sep = ", ")
        {
            if (actual == *ai)
                return "";
            out << sep << sTypes.lookup(*ai);
        }
        out << ')';
    }
    out << op << sTypes.lookup(actual);
    return out.str();
}
std::string llsd_matches(const LLSD& prototype, const LLSD& data, const std::string& pfx)
{
    if (prototype.isUndefined())
        return "";
    if (prototype.isArray())
    {
        if (! data.isArray())
        {
            return STRINGIZE(colon(pfx) << "Array" << op << sTypes.lookup(data.type()));
        }
        if (data.size() < prototype.size())
        {
            return STRINGIZE(colon(pfx) << "Array size " << prototype.size() << op
                             << "Array size " << data.size());
        }
        for (LLSD::Integer i = 0; i < prototype.size(); ++i)
        {
            std::string match(llsd_matches(prototype[i], data[i], STRINGIZE('[' << i << ']')));
            if (! match.empty())
            {
                return match;
            }
        }
        return "";
    }
    if (prototype.isMap())
    {
        if (! data.isMap())
        {
            return STRINGIZE(colon(pfx) << "Map" << op << sTypes.lookup(data.type()));
        }
        std::ostringstream out;
        out << colon(pfx);
        const char* init = "Map missing keys: ";
        const char* sep = init;
        for (LLSD::map_const_iterator mi = prototype.beginMap(); mi != prototype.endMap(); ++mi)
        {
            if (! data.has(mi->first))
            {
                out << sep << mi->first;
                sep = ", ";
            }
        }
        if (sep != init)
        {
            return out.str();
        }
        for (LLSD::map_const_iterator mi2 = prototype.beginMap(); mi2 != prototype.endMap(); ++mi2)
        {
            std::string match(llsd_matches(mi2->second, data[mi2->first],
                                           STRINGIZE("['" << mi2->first << "']")));
            if (! match.empty())
            {
                return match;
            }
        }
        return "";
    }
    if (prototype.isString())
    {
        static LLSD::Type accept[] =
        {
            LLSD::TypeBoolean,
            LLSD::TypeInteger,
            LLSD::TypeReal,
            LLSD::TypeUUID,
            LLSD::TypeDate,
            LLSD::TypeURI
        };
        return match_types(prototype.type(),
						   TypeVector(std::begin(accept), std::end(accept)),
                           data.type(),
                           pfx);
    }
    if (prototype.isBoolean() || prototype.isInteger() || prototype.isReal())
    {
        static LLSD::Type all[] =
        {
            LLSD::TypeBoolean,
            LLSD::TypeInteger,
            LLSD::TypeReal,
            LLSD::TypeString
        };
        std::set<LLSD::Type> rest(std::begin(all), std::end(all));
        rest.erase(prototype.type());
        return match_types(prototype.type(),
                           TypeVector(rest.begin(), rest.end()),
                           data.type(),
                           pfx);
    }
    if (prototype.isUUID() || prototype.isDate() || prototype.isURI())
    {
        static LLSD::Type accept[] =
        {
            LLSD::TypeString
        };
        return match_types(prototype.type(),
                           TypeVector(std::begin(accept), std::end(accept)),
                           data.type(),
                           pfx);
    }
    return match_types(prototype.type(), TypeVector(), data.type(), pfx);
}
bool llsd_equals(const LLSD& lhs, const LLSD& rhs, int bits)
{
    if (lhs.type() != rhs.type())
    {
        return false;
    }
    switch (lhs.type())
    {
    case LLSD::TypeUndefined:
        return true;
    case LLSD::TypeReal:
        if (bits >= 0)
        {
            return is_approx_equal_fraction(lhs.asReal(), rhs.asReal(), bits);
        }
        return (lhs.asReal() == rhs.asReal());
#define COMPARE_SCALAR(type)                                    \
    case LLSD::Type##type:                                      \
          \
                     \
        return (! (lhs.as##type() != rhs.as##type()))
    COMPARE_SCALAR(Boolean);
    COMPARE_SCALAR(Integer);
    COMPARE_SCALAR(String);
    COMPARE_SCALAR(UUID);
    COMPARE_SCALAR(Date);
    COMPARE_SCALAR(URI);
    COMPARE_SCALAR(Binary);
#undef COMPARE_SCALAR
    case LLSD::TypeArray:
    {
        LLSD::array_const_iterator
            lai(lhs.beginArray()), laend(lhs.endArray()),
            rai(rhs.beginArray()), raend(rhs.endArray());
        for ( ; lai != laend && rai != raend; ++lai, ++rai)
        {
            if (! llsd_equals(*lai, *rai, bits))
                return false;
        }
        return (lai == laend && rai == raend);
    }
    case LLSD::TypeMap:
    {
        std::set<LLSD::String> rhskeys;
        for (LLSD::map_const_iterator rmi(rhs.beginMap()), rmend(rhs.endMap());
             rmi != rmend; ++rmi)
        {
            rhskeys.insert(rmi->first);
        }
        for (LLSD::map_const_iterator lmi(lhs.beginMap()), lmend(lhs.endMap());
             lmi != lmend; ++lmi)
        {
            if (rhskeys.erase(lmi->first) != 1)
                return false;
            if (! llsd_equals(lmi->second, rhs[lmi->first], bits))
                return false;
        }
        return rhskeys.empty();
    }
    default:
        LL_ERRS("llsd_equals") << "llsd_equals(" << lhs << ", " << rhs << ", " << bits << "): "
            "unknown type " << lhs.type() << LL_ENDL;
        return false;
    }
}
LLSD llsd_clone(LLSD value, LLSD filter)
{
    LLSD clone;
    bool has_filter(filter.isMap());
    switch (value.type())
    {
    case LLSD::TypeMap:
        clone = LLSD::emptyMap();
        for (LLSD::map_const_iterator itm = value.beginMap(); itm != value.endMap(); ++itm)
        {
            if (has_filter)
            {
                if (filter.has((*itm).first))
                {
                    if (!filter[(*itm).first].asBoolean())
                        continue;
                }
                else if (filter.has("*"))
                {
                    if (!filter["*"].asBoolean())
                        continue;
                }
                else
                {
                    continue;
                }
            }
            clone[(*itm).first] = llsd_clone((*itm).second, filter);
        }
        break;
    case LLSD::TypeArray:
        clone = LLSD::emptyArray();
        for (auto const& entry : value.array())
        {
            clone.append(llsd_clone(entry, filter));
        }
        break;
    case LLSD::TypeBinary:
    {
        LLSD::Binary bin(value.asBinary().begin(), value.asBinary().end());
        clone = LLSD::Binary(bin);
        break;
    }
    default:
        clone = value;
    }
    return clone;
}
LLSD llsd_shallow(LLSD value, LLSD filter)
{
    LLSD shallow;
    bool has_filter(filter.isMap());
    if (value.isMap())
    {
        shallow = LLSD::emptyMap();
        for (LLSD::map_const_iterator itm = value.beginMap(); itm != value.endMap(); ++itm)
        {
            if (has_filter)
            {
                if (filter.has((*itm).first))
                {
                    if (!filter[(*itm).first].asBoolean())
                        continue;
                }
                else if (filter.has("*"))
                {
                    if (!filter["*"].asBoolean())
                        continue;
                }
                else
                {
                    continue;
                }
            }
            shallow[(*itm).first] = (*itm).second;
        }
    }
    else if (value.isArray())
    {
        shallow = LLSD::emptyArray();
        for (auto const& entry : value.array())
        {
            shallow.append(entry);
        }
    }
    else
    {
        return value;
    }
    return shallow;
}
