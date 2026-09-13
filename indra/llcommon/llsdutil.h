/** 
 * @file llsdutil.h
 * @author Phoenix
 * @date 2006-05-24
 * @brief Utility classes, functions, etc, for using structured data.
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
#ifndef LL_LLSDUTIL_H
#define LL_LLSDUTIL_H
#include "llsd.h"
#include <boost/functional/hash.hpp>
LL_COMMON_API LLSD ll_sd_from_U32(const U32);
LL_COMMON_API U32 ll_U32_from_sd(const LLSD& sd);
LL_COMMON_API LLSD ll_sd_from_U64(const U64);
LL_COMMON_API U64 ll_U64_from_sd(const LLSD& sd);
LL_COMMON_API LLSD ll_sd_from_ipaddr(const U32);
LL_COMMON_API U32 ll_ipaddr_from_sd(const LLSD& sd);
LL_COMMON_API LLSD ll_string_from_binary(const LLSD& sd);
LL_COMMON_API LLSD ll_binary_from_string(const LLSD& sd);
LL_COMMON_API char* ll_print_sd(const LLSD& sd);
LL_COMMON_API char* ll_pretty_print_sd_ptr(const LLSD* sd);
LL_COMMON_API char* ll_pretty_print_sd(const LLSD& sd);
LL_COMMON_API std::string ll_stream_notation_sd(const LLSD& sd);
LL_COMMON_API BOOL compare_llsd_with_template(
	const LLSD& llsd_to_test,
	const LLSD& template_llsd,
	LLSD& resultant_llsd);
bool filter_llsd_with_template(
	const LLSD & llsd_to_test,
	const LLSD & template_llsd,
	LLSD & resultant_llsd);
LL_COMMON_API std::string llsd_matches(const LLSD& prototype, const LLSD& data, const std::string& pfx="");
LL_COMMON_API bool llsd_equals(const LLSD& lhs, const LLSD& rhs, int bits=-1);
inline bool operator==(const LLSD& lhs, const LLSD& rhs)
{
	return llsd_equals(lhs, rhs, 8);
}
template<typename Input> LLSD llsd_copy_array(Input iter, Input end)
{
	LLSD dest;
	for (; iter != end; ++iter)
	{
		dest.append(*iter);
	}
	return dest;
}
class LLSDArray
{
public:
    LLSDArray():
        _data(LLSD::emptyArray())
    {}
    LLSDArray(const LLSDArray& inner):
        _data(LLSD::emptyArray())
    {
        _data.append(inner);
    }
    LLSDArray(const LLSD& value):
        _data(LLSD::emptyArray())
    {
        _data.append(value);
    }
    LLSDArray& operator()(const LLSD& value)
    {
        _data.append(value);
        return *this;
    }
    operator LLSD() const { return _data; }
    LLSD get() const { return _data; }
private:
    LLSD _data;
};
class LLSDMap
{
public:
    LLSDMap():
        _data(LLSD::emptyMap())
    {}
    LLSDMap(const LLSD::String& key, const LLSD& value):
        _data(LLSD::emptyMap())
    {
        _data[key] = value;
    }
    LLSDMap& operator()(const LLSD::String& key, const LLSD& value)
    {
        _data[key] = value;
        return *this;
    }
    operator LLSD() const { return _data; }
    LLSD get() const { return _data; }
private:
    LLSD _data;
};
template <typename T>
class LLSDParam
{
public:
    LLSDParam(const LLSD& value):
        _value(value)
    {}
    operator T() const { return _value; }
private:
    T _value;
};
#define LLSDParam_for(T, AS)                    \
template <>                                     \
class LLSDParam<T>                              \
{                                               \
public:                                         \
    LLSDParam(const LLSD& value):               \
        _value((T)value.AS())                      \
    {}                                          \
                                                \
    operator T() const { return _value; }       \
                                                \
private:                                        \
    T _value;                                   \
}
LLSDParam_for(float,        asReal);
LLSDParam_for(LLUUID,       asUUID);
LLSDParam_for(LLDate,       asDate);
LLSDParam_for(LLURI,        asURI);
LLSDParam_for(LLSD::Binary, asBinary);
template <>
class LLSDParam<const char*>
{
private:
    std::string _value;
    bool _undefined;
public:
    LLSDParam(const LLSD& value):
        _value(value),
        _undefined(value.isUndefined())
    {}
    operator const char*() const
    {
        if (_undefined)
        {
            return NULL;
        }
        return _value.c_str();
    }
};
namespace llsd
{
class inArray
{
public:
    inArray(const LLSD& array):
        _array(array)
    {}
    typedef LLSD::array_const_iterator const_iterator;
    typedef LLSD::array_iterator iterator;
    iterator begin() { return _array.beginArray(); }
    iterator end()   { return _array.endArray(); }
    const_iterator begin() const { return _array.beginArray(); }
    const_iterator end()   const { return _array.endArray(); }
private:
    LLSD _array;
};
typedef std::map<LLSD::String, LLSD>::value_type MapEntry;
class inMap
{
public:
    inMap(const LLSD& map):
        _map(map)
    {}
    typedef LLSD::map_const_iterator const_iterator;
    typedef LLSD::map_iterator iterator;
    iterator begin() { return _map.beginMap(); }
    iterator end()   { return _map.endMap(); }
    const_iterator begin() const { return _map.beginMap(); }
    const_iterator end()   const { return _map.endMap(); }
private:
    LLSD _map;
};
}
LLSD llsd_clone(LLSD value, LLSD filter = LLSD());
LLSD llsd_shallow(LLSD value, LLSD filter = LLSD());
namespace boost {
	template<> struct hash<LLSD>
{
    typedef LLSD argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const
    {
        result_type seed(0);
        LLSD::Type stype = s.type();
        boost::hash_combine(seed, (S32)stype);
        switch (stype)
        {
        case LLSD::TypeBoolean:
            boost::hash_combine(seed, s.asBoolean());
            break;
        case LLSD::TypeInteger:
            boost::hash_combine(seed, s.asInteger());
            break;
        case LLSD::TypeReal:
            boost::hash_combine(seed, s.asReal());
            break;
        case LLSD::TypeURI:
        case LLSD::TypeString:
            boost::hash_combine(seed, s.asString());
            break;
        case LLSD::TypeUUID:
            boost::hash_combine(seed, s.asUUID());
            break;
        case LLSD::TypeDate:
            boost::hash_combine(seed, s.asDate().secondsSinceEpoch());
            break;
        case LLSD::TypeBinary:
        {
            const LLSD::Binary &b(s.asBinary());
            boost::hash_range(seed, b.begin(), b.end());
            break;
        }
        case LLSD::TypeMap:
        {
            for (LLSD::map_const_iterator itm = s.beginMap(); itm != s.endMap(); ++itm)
            {
                boost::hash_combine(seed, (*itm).first);
                boost::hash_combine(seed, (*itm).second);
            }
            break;
        }
        case LLSD::TypeArray:
            for (LLSD::array_const_iterator ita = s.beginArray(); ita != s.endArray(); ++ita)
            {
                boost::hash_combine(seed, (*ita));
            }
            break;
        case LLSD::TypeUndefined:
        default:
            break;
        }
        return seed;
    }
};
}
#endif
