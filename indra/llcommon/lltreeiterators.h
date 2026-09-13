/**
 * @file   lltreeiterators.h
 * @author Nat Goodspeed
 * @date   2008-08-19
 * @brief  This file defines iterators useful for traversing arbitrary node
 *         classes, potentially polymorphic, linked into strict tree
 *         structures.
 *
 *         Dereferencing any one of these iterators actually yields a @em
 *         pointer to the node in question. For example, given an
 *         LLLinkedIter<MyNode> <tt>li</tt>, <tt>*li</tt> gets you a pointer
 *         to MyNode, and <tt>**li</tt> gets you the MyNode instance itself.
 *         More commonly, instead of writing <tt>li->member</tt>, you write
 *         <tt>(*li)->member</tt> -- as you would if you were traversing an
 *         STL container of MyNode pointers.
 *
 *         It would certainly be possible to build these iterators so that
 *         <tt>*iterator</tt> would return a reference to the node itself
 *         rather than a pointer to the node, and for many purposes it would
 *         even be more convenient. However, that would be insufficiently
 *         flexible. If you want to use an iterator range to (e.g.) initialize
 *         a std::vector collecting results -- you rarely want to actually @em
 *         copy the nodes in question. You're much more likely to want to copy
 *         <i>pointers to</i> the traversed nodes. Hence these iterators
 *         produce pointers.
 *
 *         Though you specify the actual NODE class as the template parameter,
 *         these iterators internally use LLPtrTo<> to discover whether to
 *         store and return an LLPointer<NODE> or a simple NODE*.
 *
 *         By strict tree structures, we mean that each child must have
 *         exactly one parent. This forbids a child claiming any ancestor as a
 *         child of its own. Child nodes with multiple parents will be visited
 *         once for each parent. Cycles in the graph will result in either an
 *         infinite loop or an out-of-memory crash. You Have Been Warned.
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
#if ! defined(LL_LLTREEITERATORS_H)
#define LL_LLTREEITERATORS_H
#include "llptrto.h"
#include <vector>
#include <deque>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/static_assert.hpp>
namespace LLTreeIter
{
    enum RootIter { UP, DOWN };
    enum WalkIter { DFS_PRE, DFS_POST, BFS };
}
template <class SELFTYPE, class NODE>
class LLBaseIter: public boost::iterator_facade<SELFTYPE,
                                                typename LLPtrTo<NODE>::type,
                                                boost::forward_traversal_tag>
{
protected:
    typedef typename LLPtrTo<NODE>::type ptr_type;
    typedef std::function<ptr_type(const ptr_type&)> func_type;
    typedef SELFTYPE self_type;
};
template <class NODE>
typename LLPtrTo<NODE>::type LLNullNextFunctor(const typename LLPtrTo<NODE>::type&)
{
    return typename LLPtrTo<NODE>::type();
}
template <class NODE>
class LLLinkedIter: public LLBaseIter<LLLinkedIter<NODE>, NODE>
{
    typedef LLBaseIter<LLLinkedIter<NODE>, NODE> super;
protected:
    typedef typename super::self_type self_type;
    typedef typename super::ptr_type ptr_type;
    typedef typename super::func_type func_type;
public:
    LLLinkedIter(const ptr_type& entry, const func_type& nextfunc):
        mCurrent(entry),
        mNextFunc(nextfunc)
    {}
    LLLinkedIter():
        mCurrent(),
        mNextFunc(LLNullNextFunctor<NODE>)
    {}
private:
    friend class boost::iterator_core_access;
    void increment()
    {
        mCurrent = mNextFunc(mCurrent);
    }
    bool equal(const self_type& that) const { return this->mCurrent == that.mCurrent; }
    ptr_type& dereference() const { return const_cast<ptr_type&>(mCurrent); }
    ptr_type mCurrent;
    func_type mNextFunc;
};
template <class NODE>
class LLTreeUpIter: public LLLinkedIter<NODE>
{
    typedef LLLinkedIter<NODE> super;
public:
    LLTreeUpIter(const typename super::ptr_type& node,
                 const typename super::func_type& parentfunc):
        super(node, parentfunc)
    {}
    LLTreeUpIter():
        super()
    {}
};
template <class NODE>
class LLTreeDownIter: public LLBaseIter<LLTreeDownIter<NODE>, NODE>
{
    typedef LLBaseIter<LLTreeDownIter<NODE>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    typedef typename super::func_type func_type;
private:
    typedef std::vector<ptr_type> list_type;
public:
    LLTreeDownIter(const ptr_type& node,
                   const func_type& parentfunc)
    {
        for (ptr_type n = node; n; n = parentfunc(n))
            mParents.push_back(n);
    }
    LLTreeDownIter() {}
private:
    friend class boost::iterator_core_access;
    void increment()
    {
        mParents.pop_back();
    }
    bool equal(const self_type& that) const { return this->mParents == that.mParents; }
    ptr_type& dereference() const { return const_cast<ptr_type&>(mParents.back()); }
    list_type mParents;
};
template <LLTreeIter::RootIter DISCRIM, class NODE>
class LLTreeRootIter
{
    enum { use_a_valid_LLTreeIter_RootIter_value = false };
public:
    template <typename TYPE1, typename TYPE2>
    LLTreeRootIter(TYPE1, TYPE2)
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_RootIter_value);
    }
    LLTreeRootIter()
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_RootIter_value);
    }
};
template <class NODE>
class LLTreeRootIter<LLTreeIter::UP, NODE>: public LLTreeUpIter<NODE>
{
    typedef LLTreeUpIter<NODE> super;
public:
    LLTreeRootIter(const typename super::ptr_type& node,
                   const typename super::func_type& parentfunc):
        super(node, parentfunc)
    {}
    LLTreeRootIter():
        super()
    {}
};
template <class NODE>
class LLTreeRootIter<LLTreeIter::DOWN, NODE>: public LLTreeDownIter<NODE>
{
    typedef LLTreeDownIter<NODE> super;
public:
    LLTreeRootIter(const typename super::ptr_type& node,
                   const typename super::func_type& parentfunc):
        super(node, parentfunc)
    {}
    LLTreeRootIter():
        super()
    {}
};
template <class NODE, typename CHILDITER>
class LLTreeDFSIter: public LLBaseIter<LLTreeDFSIter<NODE, CHILDITER>, NODE>
{
    typedef LLBaseIter<LLTreeDFSIter<NODE, CHILDITER>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    typedef std::function<CHILDITER(const ptr_type&)> func_type;
private:
    typedef std::vector<ptr_type> list_type;
public:
    LLTreeDFSIter(const ptr_type& node, const func_type& beginfunc, const func_type& endfunc)
	    : mBeginFunc(beginfunc),
	    mEndFunc(endfunc),
	    mSkipChildren(false)
    {
        if (node)
            mPending.push_back(node);
    }
    LLTreeDFSIter() : mSkipChildren(false) {}
    void skipDescendants(bool skip = true) { mSkipChildren = skip; }
private:
    friend class boost::iterator_core_access;
    void increment()
    {
        ptr_type current = mPending.back();
        mPending.pop_back();
		if (!mSkipChildren)
		{
			addChildren(current);
		}
		mSkipChildren = false;
    }
    bool equal(const self_type& that) const { return this->mPending == that.mPending; }
    ptr_type& dereference() const { return const_cast<ptr_type&>(mPending.back()); }
    void addChildren(const ptr_type& node)
    {
        CHILDITER chi = mBeginFunc(node), chend = mEndFunc(node);
        mPending.resize(mPending.size() + std::distance(chi, chend));
        std::copy(chi, chend, mPending.rbegin());
    }
    list_type mPending;
    func_type mBeginFunc;
    func_type mEndFunc;
    bool	mSkipChildren;
};
template <class NODE, typename CHILDITER>
class LLTreeDFSPostIter: public LLBaseIter<LLTreeDFSPostIter<NODE, CHILDITER>, NODE>
{
    typedef LLBaseIter<LLTreeDFSPostIter<NODE, CHILDITER>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    typedef std::function<CHILDITER(const ptr_type&)> func_type;
private:
    typedef std::vector< std::pair<ptr_type, bool> > list_type;
public:
    LLTreeDFSPostIter(const ptr_type& node, const func_type& beginfunc, const func_type& endfunc)
	    : mBeginFunc(beginfunc),
	    mEndFunc(endfunc),
	    mSkipAncestors(false)
	    {
        if (! node)
            return;
        mPending.push_back(typename list_type::value_type(node, false));
        makeCurrent();
    }
    LLTreeDFSPostIter() : mSkipAncestors(false) {}
    void skipAncestors(bool skip = true) { mSkipAncestors = skip; }
private:
    friend class boost::iterator_core_access;
    void increment()
    {
        mPending.pop_back();
        makeCurrent();
    }
    bool equal(const self_type& that) const { return this->mPending == that.mPending; }
    ptr_type& dereference() const { return const_cast<ptr_type&>(mPending.back().first); }
	struct isOpen
	{
		bool operator()(const typename list_type::value_type& item)
		{
			return item.second;
		}
	};
    void makeCurrent()
    {
		if (mSkipAncestors)
		{
			mPending.erase(std::remove_if(mPending.begin(), mPending.end(), isOpen()), mPending.end());
			mSkipAncestors = false;
		}
        if (mPending.empty())
            return;
		auto& pending = mPending.back();
        if (pending.second)
            return;
		pending.second = true;
        addChildren(pending.first);
        makeCurrent();
    }
    void addChildren(const ptr_type& node)
    {
        CHILDITER chi = mBeginFunc(node), chend = mEndFunc(node);
        mPending.resize(mPending.size() + std::distance(chi, chend));
        for (typename list_type::reverse_iterator pi = mPending.rbegin(); chi != chend; ++chi, ++pi)
        {
            pi->first = *chi;
            pi->second = false;
        }
    }
    list_type	mPending;
    func_type	mBeginFunc;
    func_type	mEndFunc;
	bool		mSkipAncestors;
};
template <class NODE, typename CHILDITER>
class LLTreeBFSIter: public LLBaseIter<LLTreeBFSIter<NODE, CHILDITER>, NODE>
{
    typedef LLBaseIter<LLTreeBFSIter<NODE, CHILDITER>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    typedef std::function<CHILDITER(const ptr_type&)> func_type;
private:
    typedef std::deque<ptr_type> list_type;
public:
    LLTreeBFSIter(const ptr_type& node, const func_type& beginfunc, const func_type& endfunc):
        mBeginFunc(beginfunc),
        mEndFunc(endfunc)
    {
        if (node)
            mPending.push_back(node);
    }
    LLTreeBFSIter() {}
private:
    friend class boost::iterator_core_access;
    void increment()
    {
        ptr_type current = mPending.front();
        mPending.pop_front();
        CHILDITER chend = mEndFunc(current);
        for (CHILDITER chi = mBeginFunc(current); chi != chend; ++chi)
            mPending.push_back(*chi);
    }
    bool equal(const self_type& that) const { return this->mPending == that.mPending; }
    ptr_type& dereference() const { return const_cast<ptr_type&>(mPending.front()); }
    list_type mPending;
    func_type mBeginFunc;
    func_type mEndFunc;
};
template <LLTreeIter::WalkIter DISCRIM, class NODE, typename CHILDITER>
class LLTreeWalkIter
{
    enum { use_a_valid_LLTreeIter_WalkIter_value = false };
public:
    template <typename TYPE1, typename TYPE2>
    LLTreeWalkIter(TYPE1, TYPE2)
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_WalkIter_value);
    }
    LLTreeWalkIter()
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_WalkIter_value);
    }
};
template <class NODE, typename CHILDITER>
class LLTreeWalkIter<LLTreeIter::DFS_PRE, NODE, CHILDITER>:
    public LLTreeDFSIter<NODE, CHILDITER>
{
    typedef LLTreeDFSIter<NODE, CHILDITER> super;
public:
    LLTreeWalkIter(const typename super::ptr_type& node,
                   const typename super::func_type& beginfunc,
                   const typename super::func_type& endfunc):
        super(node, beginfunc, endfunc)
    {}
    LLTreeWalkIter():
        super()
    {}
};
template <class NODE, typename CHILDITER>
class LLTreeWalkIter<LLTreeIter::DFS_POST, NODE, CHILDITER>:
    public LLTreeDFSPostIter<NODE, CHILDITER>
{
    typedef LLTreeDFSPostIter<NODE, CHILDITER> super;
public:
    LLTreeWalkIter(const typename super::ptr_type& node,
                   const typename super::func_type& beginfunc,
                   const typename super::func_type& endfunc):
        super(node, beginfunc, endfunc)
    {}
    LLTreeWalkIter():
        super()
    {}
};
template <class NODE, typename CHILDITER>
class LLTreeWalkIter<LLTreeIter::BFS, NODE, CHILDITER>:
    public LLTreeBFSIter<NODE, CHILDITER>
{
    typedef LLTreeBFSIter<NODE, CHILDITER> super;
public:
    LLTreeWalkIter(const typename super::ptr_type& node,
                   const typename super::func_type& beginfunc,
                   const typename super::func_type& endfunc):
        super(node, beginfunc, endfunc)
    {}
    LLTreeWalkIter():
        super()
    {}
};
#endif
