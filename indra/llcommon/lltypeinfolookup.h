/**
 * @file   lltypeinfolookup.h
 * @author Nat Goodspeed
 * @date   2012-04-08
 * @brief  Template data structure like std::map<std::type_info*, T>
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */
#if ! defined(LL_LLTYPEINFOLOOKUP_H)
#define LL_LLTYPEINFOLOOKUP_H
#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>
#include <functional>
#include <typeinfo>
struct const_char_star_hash: public std::unary_function<const char*, std::size_t>
{
    std::size_t operator()(const char* str) const
    {
        std::size_t seed = 0;
        for ( ; *str; ++str)
        {
            boost::hash_combine(seed, *str);
        }
        return seed;
    }
};
struct const_char_star_equal: public std::binary_function<const char*, const char*, bool>
{
    bool operator()(const char* lhs, const char* rhs) const
    {
        return strcmp(lhs, rhs) == 0;
    }
};
template <typename VALUE>
class LLTypeInfoLookup
{
    typedef boost::unordered_map<const char*, VALUE,
                                 const_char_star_hash, const_char_star_equal> impl_map_type;
public:
    typedef VALUE value_type;
    LLTypeInfoLookup() {}
    bool empty() const { return mMap.empty(); }
    std::size_t size() const { return mMap.size(); }
    template <typename KEY>
    bool insert(const value_type& value)
    {
        return mMap.insert(typename impl_map_type::value_type(typeid(KEY).name(), value)).second;
    }
    template <typename KEY>
    boost::optional<value_type> find() const
    {
        typename impl_map_type::const_iterator found = mMap.find(typeid(KEY).name());
        if (found == mMap.end())
            return boost::optional<value_type>();
        return found->second;
    }
private:
    impl_map_type mMap;
};
#endif
