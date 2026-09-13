/**
 * @file   llsortedvector.h
 * @author Nat Goodspeed
 * @date   2012-04-08
 * @brief  LLSortedVector class wraps a vector that we maintain in sorted
 *         order so we can perform binary-search lookups.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */
#if ! defined(LL_LLSORTEDVECTOR_H)
#define LL_LLSORTEDVECTOR_H
#include <vector>
#include <algorithm>
template <typename KEY, typename VALUE>
class LLSortedVector
{
public:
    typedef LLSortedVector<KEY, VALUE> self;
    typedef KEY key_type;
    typedef VALUE mapped_type;
    typedef std::pair<key_type, mapped_type> value_type;
    typedef std::vector<value_type> PairVector;
    typedef typename PairVector::iterator iterator;
    typedef typename PairVector::const_iterator const_iterator;
    LLSortedVector() {}
    LLSortedVector(std::size_t size):
        mVector(size)
    {}
    template <typename ITER>
    LLSortedVector(ITER begin, ITER end):
        mVector(begin, end)
    {
        std::sort(mVector.begin(), mVector.end());
    }
    std::pair<iterator, bool> insert(const key_type& key, const mapped_type& value)
    {
        return insert(value_type(key, value));
    }
    std::pair<iterator, bool> insert(const value_type& pair)
    {
        typedef std::pair<iterator, bool> iterbool;
        iterator found = std::lower_bound(mVector.begin(), mVector.end(), pair,
                                          less<value_type>());
        if (found == mVector.end())
        {
            std::size_t index(mVector.size());
            mVector.push_back(pair);
            return iterbool(mVector.begin() + index, true);
        }
        if (found->first == pair.first)
        {
            return iterbool(found, false);
        }
        std::size_t index(found - mVector.begin());
        mVector.insert(found, pair);
        return iterbool(mVector.begin() + index, true);
    }
    iterator begin() { return mVector.begin(); }
    iterator end()   { return mVector.end(); }
    const_iterator begin() const { return mVector.begin(); }
    const_iterator end()   const { return mVector.end(); }
    bool empty() const { return mVector.empty(); }
    std::size_t size() const { return mVector.size(); }
    iterator find(const key_type& key)
    {
        iterator found = std::lower_bound(mVector.begin(), mVector.end(),
                                          value_type(key, mapped_type()),
                                          less<value_type>());
        if (found == mVector.end() || found->first != key)
            return mVector.end();
        return found;
    }
    const_iterator find(const key_type& key) const
    {
        return const_cast<self*>(this)->find(key);
    }
	mapped_type& operator[] (const key_type& key)
	{
		 return insert(std::make_pair(key,mapped_type())).first->second;
	}
	const mapped_type& operator[] (const key_type& key) const
	{
		 return insert(std::make_pair(key,mapped_type())).first->second;
	}
private:
    template <typename T>
    struct less: public std::less<T> {};
    template <typename T>
    struct less< std::pair<std::type_info*, T> >:
        public std::binary_function<std::pair<std::type_info*, T>,
                                    std::pair<std::type_info*, T>,
                                    bool>
    {
        bool operator()(const std::pair<std::type_info*, T>& lhs,
                        const std::pair<std::type_info*, T>& rhs) const
        {
            return lhs.first->before(*rhs.first);
        }
    };
    template <typename T>
    struct less< std::pair<const std::type_info*, T> >:
        public std::binary_function<std::pair<const std::type_info*, T>,
                                    std::pair<const std::type_info*, T>,
                                    bool>
    {
        bool operator()(const std::pair<const std::type_info*, T>& lhs,
                        const std::pair<const std::type_info*, T>& rhs) const
        {
            return lhs.first->before(*rhs.first);
        }
    };
    PairVector mVector;
};
#endif
