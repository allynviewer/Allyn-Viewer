/**
 * @file   lldependencies.h
 * @author Nat Goodspeed
 * @date   2008-09-17
 * @brief  LLDependencies: a generic mechanism for expressing "b must follow a,
 *         but precede c"
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#if ! defined(LL_LLDEPENDENCIES_H)
#define LL_LLDEPENDENCIES_H
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <iosfwd>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#ifndef BOOST_FUNCTION_HPP_INCLUDED
#include <boost/function.hpp>
#define BOOST_FUNCTION_HPP_INCLUDED
#endif
#include <boost/bind.hpp>
template<typename FUNCTION, typename RANGE>
inline
boost::iterator_range<boost::transform_iterator<FUNCTION,
                                                typename boost::range_const_iterator<RANGE>::type> >
make_transform_range(const RANGE& range, FUNCTION function)
{
    typedef boost::transform_iterator<FUNCTION, typename boost::range_const_iterator<RANGE>::type>
        transform_iterator;
    return boost::make_iterator_range(transform_iterator(boost::begin(range), function),
                                      transform_iterator(boost::end(range),   function));
}
template<typename FUNCTION, typename RANGE>
inline
boost::iterator_range<boost::transform_iterator<FUNCTION,
                                                typename boost::range_iterator<RANGE>::type> >
make_transform_range(RANGE& range, FUNCTION function)
{
    typedef boost::transform_iterator<FUNCTION, typename boost::range_iterator<RANGE>::type>
        transform_iterator;
    return boost::make_iterator_range(transform_iterator(boost::begin(range), function),
                                      transform_iterator(boost::end(range),   function));
}
template<class TYPE>
struct instance_from_range: public TYPE
{
    template<typename RANGE>
    instance_from_range(RANGE range):
        TYPE(boost::begin(range), boost::end(range))
    {}
};
class LL_COMMON_API LLDependenciesBase
{
public:
    virtual ~LLDependenciesBase() {}
    struct Cycle: public std::runtime_error
    {
        Cycle(const std::string& what): std::runtime_error(what) {}
    };
    virtual std::ostream& describe(std::ostream& out, bool full=true) const;
    virtual std::string describe(bool full=true) const;
protected:
    typedef std::vector< std::pair<std::size_t, std::size_t> > EdgeList;
    typedef std::vector<std::size_t> VertexList;
    VertexList topo_sort(std::size_t vertices, const EdgeList& edges) const;
    template<typename T1, typename T2>
    struct refpair
    {
        refpair(T1 value1, T2 value2):
            first(value1),
            second(value2)
        {}
        T1 first;
        T2 second;
    };
};
template<typename T>
inline
std::ostream& LLDependencies_describe(std::ostream& out, const T& key)
{
    out << key;
    return out;
}
template<>
inline
std::ostream& LLDependencies_describe(std::ostream& out, const std::string& key)
{
    out << '"' << key << '"';
    return out;
}
struct LLDependenciesEmpty
{
    LLDependenciesEmpty() {}
    LLDependenciesEmpty(void*) {}
};
template<typename KEY = std::string,
         typename NODE = LLDependenciesEmpty>
class LLDependencies: public LLDependenciesBase
{
    typedef LLDependencies<KEY, NODE> self_type;
    struct DepNode
    {
        typedef std::set<KEY> dep_set;
        DepNode(const NODE& node_, const dep_set& after_, const dep_set& before_):
            node(node_),
            after(after_),
            before(before_)
        {}
        NODE node;
        dep_set after, before;
    };
    typedef std::map<KEY, DepNode> DepNodeMap;
    typedef typename DepNodeMap::value_type DepNodeMapEntry;
    typedef boost::function<const typename DepNode::dep_set&(const DepNode&)> dep_selector;
public:
    LLDependencies() {}
    typedef KEY key_type;
    typedef NODE node_type;
    typedef std::vector<KEY> KeyList;
    NODE& add(const KEY& key, const NODE& node = NODE(),
              const KeyList& after = KeyList(),
              const KeyList& before = KeyList())
    {
        typename DepNode::dep_set
            after_set(after.begin(), after.end()),
            before_set(before.begin(), before.end());
        std::pair<typename DepNodeMap::iterator, bool> inserted =
            mNodes.insert(typename DepNodeMap::value_type(key,
                                                          DepNode(node, after_set, before_set)));
        if (! inserted.second)
        {
            if (inserted.first->second.after  != after_set ||
                inserted.first->second.before != before_set)
            {
                mCache.clear();
                inserted.first->second.after  = after_set;
                inserted.first->second.before = before_set;
            }
        }
        else
        {
            mCache.clear();
        }
        return inserted.first->second.node;
    }
    typedef refpair<const KEY&, NODE&> value_type;
    typedef refpair<const KEY&, const NODE&> const_value_type;
private:
    static value_type value_extract(DepNodeMapEntry& entry)
    {
        return value_type(entry.first, entry.second.node);
    }
    static const_value_type const_value_extract(const DepNodeMapEntry& entry)
    {
        return const_value_type(entry.first, entry.second.node);
    }
    template<typename ITERATOR, typename FUNCTION>
    boost::iterator_range<ITERATOR> generic_range(FUNCTION function)
    {
        return make_transform_range(mNodes, function);
    }
    template<typename ITERATOR, typename FUNCTION>
    boost::iterator_range<ITERATOR> generic_range(FUNCTION function) const
    {
        return make_transform_range(mNodes, function);
    }
public:
    typedef boost::transform_iterator<boost::function<value_type(DepNodeMapEntry&)>,
                                      typename DepNodeMap::iterator> iterator;
    typedef boost::iterator_range<iterator> range;
    range get_range()
    {
        return generic_range<iterator>(value_extract);
    }
    typedef boost::transform_iterator<boost::function<const_value_type(const DepNodeMapEntry&)>,
                                      typename DepNodeMap::const_iterator> const_iterator;
    typedef boost::iterator_range<const_iterator> const_range;
    const_range get_range() const
    {
        return generic_range<const_iterator>(const_value_extract);
    }
    typedef boost::transform_iterator<boost::function<NODE&(DepNodeMapEntry&)>,
                                      typename DepNodeMap::iterator> node_iterator;
    typedef boost::iterator_range<node_iterator> node_range;
    node_range get_node_range()
    {
        return generic_range<node_iterator>(
            boost::bind<NODE&>(&DepNode::node,
                               boost::bind<DepNode&>(&DepNodeMapEntry::second, _1)));
    }
    typedef boost::transform_iterator<boost::function<const NODE&(const DepNodeMapEntry&)>,
                                      typename DepNodeMap::const_iterator> const_node_iterator;
    typedef boost::iterator_range<const_node_iterator> const_node_range;
    const_node_range get_node_range() const
    {
        return generic_range<const_node_iterator>(
            boost::bind<const NODE&>(&DepNode::node,
                                     boost::bind<const DepNode&>(&DepNodeMapEntry::second, _1)));
    }
    typedef boost::transform_iterator<boost::function<const KEY&(const DepNodeMapEntry&)>,
                                      typename DepNodeMap::const_iterator> const_key_iterator;
    typedef boost::iterator_range<const_key_iterator> const_key_range;
    const_key_range get_key_range() const
    {
        return generic_range<const_key_iterator>(
            boost::bind<const KEY&>(&DepNodeMapEntry::first, _1));
    }
    const NODE* get(const KEY& key) const
    {
        typename DepNodeMap::const_iterator found = mNodes.find(key);
        if (found != mNodes.end())
        {
            return &found->second.node;
        }
        return NULL;
    }
    NODE* get(const KEY& key)
    {
        return const_cast<NODE*>(const_cast<const self_type*>(this)->get(key));
    }
    bool remove(const KEY& key)
    {
        typename DepNodeMap::iterator found = mNodes.find(key);
        if (found != mNodes.end())
        {
            mNodes.erase(found);
            return true;
        }
        return false;
    }
private:
    typedef std::vector<iterator> iterator_list;
    typedef typename iterator_list::iterator iterator_list_iterator;
public:
    typedef boost::iterator_range<boost::indirect_iterator<iterator_list_iterator> > sorted_range;
    typedef typename sorted_range::iterator sorted_iterator;
    sorted_range sort() const
    {
        if (mCache.empty() && ! mNodes.empty())
        {
            typedef std::map<KEY, std::size_t> VertexMap;
            VertexMap vmap;
            {
                for (typename DepNodeMap::const_iterator nmi = mNodes.begin(), nmend = mNodes.end();
                     nmi != nmend; ++nmi)
                {
                    vmap.insert(typename VertexMap::value_type(nmi->first, vmap.size()));
                    for (typename DepNode::dep_set::const_iterator ai = nmi->second.after.begin(),
                                                                   aend = nmi->second.after.end();
                         ai != aend; ++ai)
                    {
                        vmap.insert(typename VertexMap::value_type(*ai, vmap.size()));
                    }
                    for (typename DepNode::dep_set::const_iterator bi = nmi->second.before.begin(),
                                                                   bend = nmi->second.before.end();
                         bi != bend; ++bi)
                    {
                        vmap.insert(typename VertexMap::value_type(*bi, vmap.size()));
                    }
                }
            }
            EdgeList edges;
            {
                for (typename DepNodeMap::const_iterator nmi = mNodes.begin(), nmend = mNodes.end();
                     nmi != nmend; ++nmi)
                {
                    std::size_t thisnode = vmap[nmi->first];
                    for (typename DepNode::dep_set::const_iterator ai = nmi->second.after.begin(),
                                                                   aend = nmi->second.after.end();
                         ai != aend; ++ai)
                    {
                        edges.push_back(EdgeList::value_type(vmap[*ai], thisnode));
                    }
                    for (typename DepNode::dep_set::const_iterator bi = nmi->second.before.begin(),
                                                                   bend = nmi->second.before.end();
                         bi != bend; ++bi)
                    {
                        edges.push_back(EdgeList::value_type(thisnode, vmap[*bi]));
                    }
                }
            }
            VertexList sorted(topo_sort(vmap.size(), edges));
            KeyList vkeys(vmap.size());
            for (typename VertexMap::const_iterator vmi = vmap.begin(), vmend = vmap.end();
                 vmi != vmend; ++vmi)
            {
                vkeys[vmi->second] = vmi->first;
            }
            mCache.clear();
            for (VertexList::const_iterator svi = sorted.begin(), svend = sorted.end();
                 svi != svend; ++svi)
            {
                self_type* non_const_this(const_cast<self_type*>(this));
                typename DepNodeMap::iterator found = non_const_this->mNodes.find(vkeys[*svi]);
                if (found != non_const_this->mNodes.end())
                {
                    mCache.push_back(iterator(found, value_extract));
                }
            }
        }
        boost::indirect_iterator<iterator_list_iterator>
            begin(mCache.begin()),
            end(mCache.end());
        return sorted_range(begin, end);
    }
	using LLDependenciesBase::describe;
    virtual std::ostream& describe(std::ostream& out, bool full=true) const
    {
        typename DepNodeMap::const_iterator dmi(mNodes.begin()), dmend(mNodes.end());
        if (dmi != dmend)
        {
            std::string sep;
            describe(out, sep, *dmi, full);
            while (++dmi != dmend)
            {
                describe(out, sep, *dmi, full);
            }
        }
        return out;
    }
    static std::ostream& describe(std::ostream& out, std::string& sep,
                                  const DepNodeMapEntry& entry, bool full)
    {
        if (full || (! entry.second.after.empty()) || (! entry.second.before.empty()))
        {
            out << sep;
            sep = "\n";
            if (! entry.second.after.empty())
            {
                out << "after ";
                describe(out, entry.second.after);
                out << " -> ";
            }
            LLDependencies_describe(out, entry.first);
            if (! entry.second.before.empty())
            {
                out << " -> before ";
                describe(out, entry.second.before);
            }
        }
        return out;
    }
    static std::ostream& describe(std::ostream& out, const typename DepNode::dep_set& keys)
    {
        out << '(';
        typename DepNode::dep_set::const_iterator ki(keys.begin()), kend(keys.end());
        if (ki != kend)
        {
            LLDependencies_describe(out, *ki);
            while (++ki != kend)
            {
                out << ", ";
                LLDependencies_describe(out, *ki);
            }
        }
        out << ')';
        return out;
    }
    typedef typename DepNode::dep_set::const_iterator dep_iterator;
    typedef boost::iterator_range<dep_iterator> dep_range;
    dep_range get_dep_range_from_key(const KEY& key, const dep_selector& selector) const
    {
        typename DepNodeMap::const_iterator found = mNodes.find(key);
        if (found != mNodes.end())
        {
            return dep_range(selector(found->second));
        }
        static const typename DepNode::dep_set empty_deps;
        return dep_range(empty_deps.begin(), empty_deps.end());
    }
    template<typename ITERATOR>
    dep_range get_dep_range_from_xform(const ITERATOR& iterator, const dep_selector& selector) const
    {
        return dep_range(selector(iterator.base()->second));
    }
    dep_range get_dep_range_from_sorted(const sorted_iterator& sortiter,
                                        const dep_selector& selector) const
    {
        return get_dep_range_from_xform(*sortiter.base(), selector);
    }
    template<typename KEY_OR_ITER>
    dep_range get_after_range(const KEY_OR_ITER& key) const;
    template<typename KEY_OR_ITER>
    dep_range get_before_range(const KEY_OR_ITER& key) const;
private:
    DepNodeMap mNodes;
    mutable iterator_list mCache;
};
template<typename KEY_ITER>
struct LLDependencies_dep_range_from
{
    template<typename KEY, typename NODE, typename SELECTOR>
    typename LLDependencies<KEY, NODE>::dep_range
    operator()(const LLDependencies<KEY, NODE>& deps,
               const KEY_ITER& key,
               const SELECTOR& selector)
    {
        return deps.get_dep_range_from_key(key, selector);
    }
};
template<typename FUNCTION, typename ITERATOR>
struct LLDependencies_dep_range_from< boost::transform_iterator<FUNCTION, ITERATOR> >
{
    template<typename KEY, typename NODE, typename SELECTOR>
    typename LLDependencies<KEY, NODE>::dep_range
    operator()(const LLDependencies<KEY, NODE>& deps,
               const boost::transform_iterator<FUNCTION, ITERATOR>& iter,
               const SELECTOR& selector)
    {
        return deps.get_dep_range_from_xform(iter, selector);
    }
};
template<typename BASEITER>
struct LLDependencies_dep_range_from< boost::indirect_iterator<BASEITER> >
{
    template<typename KEY, typename NODE, typename SELECTOR>
    typename LLDependencies<KEY, NODE>::dep_range
    operator()(const LLDependencies<KEY, NODE>& deps,
               const boost::indirect_iterator<BASEITER>& iter,
               const SELECTOR& selector)
    {
        return deps.get_dep_range_from_sorted(iter, selector);
    }
};
template<typename KEY, typename NODE>
template<typename KEY_OR_ITER>
typename LLDependencies<KEY, NODE>::dep_range
LLDependencies<KEY, NODE>::get_after_range(const KEY_OR_ITER& key_iter) const
{
    return LLDependencies_dep_range_from<KEY_OR_ITER>()(
        *this,
        key_iter,
        boost::bind<const typename DepNode::dep_set&>(&DepNode::after, _1));
}
template<typename KEY, typename NODE>
template<typename KEY_OR_ITER>
typename LLDependencies<KEY, NODE>::dep_range
LLDependencies<KEY, NODE>::get_before_range(const KEY_OR_ITER& key_iter) const
{
    return LLDependencies_dep_range_from<KEY_OR_ITER>()(
        *this,
        key_iter,
        boost::bind<const typename DepNode::dep_set&>(&DepNode::before, _1));
}
#endif
