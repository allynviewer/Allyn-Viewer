/**
 * @file ailist.h
 * @brief A linked list with iterators that advance when their element is deleted.
 *
 * Copyright (c) 2013, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   05/02/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AILIST_H
#define AILIST_H
#include <list>
template<typename T>
class AIList;
template<typename T>
class AIConstListIterator;
template<typename T>
struct AINode {
  T mElement;
  mutable unsigned short count;
  mutable unsigned short dead;
  AINode(void) : count(0), dead(0) { }
  explicit AINode(T const& __val) : mElement(__val), count(0), dead(0) { }
  bool operator==(AINode const& __node) const { llassert(!__node.dead); return mElement == __node.mElement && !dead; }
  bool operator!=(AINode const& __node) const { llassert(!__node.dead); return mElement != __node.mElement || dead; }
  bool operator<(AINode const& __node) const { return mElement < __node.mElement; }
};
template<typename T>
class AIListIterator {
  private:
	typedef AIListIterator<T>				_Self;
	typedef std::list<AINode<T> >			_Container;
	typedef typename _Container::iterator	_Iterator;
	_Container* mContainer;
	_Iterator mIterator;
	void ref(void)
	{
	  if (mContainer && mContainer->end() != mIterator)
	  {
		llassert(!mIterator->dead && mIterator->count < 100);
		++(mIterator->count);
	  }
	}
	void unref(void)
	{
	  if (mContainer && mContainer->end() != mIterator)
	  {
		llassert(mIterator->count > 0);
		if (--(mIterator->count) == 0 && mIterator->dead)
		{
		  mContainer->erase(mIterator);
		}
	  }
	}
  public:
	typedef typename _Iterator::difference_type		difference_type;
	typedef typename _Iterator::iterator_category	iterator_category;
	typedef T										value_type;
	typedef T*										pointer;
	typedef T&										reference;
	AIListIterator(void) : mContainer(NULL) { }
	AIListIterator(_Container* __c, _Iterator const& __i) : mContainer(__c), mIterator(__i)
	{
	  llassert(mContainer);
	  ref();
	}
	AIListIterator(AIListIterator const& __i) : mContainer(__i.mContainer), mIterator(__i.mIterator)
	{
	  ref();
	}
	~AIListIterator()
	{
	  unref();
	}
	_Self& operator=(_Self const& __x)
	{
	  unref();
	  mContainer = __x.mContainer;
	  mIterator = __x.mIterator;
	  llassert(mContainer);
	  ref();
	  return *this;
	}
	reference operator*() const
	{
	  llassert(mContainer && !mIterator->dead);
	  return mIterator->mElement;
	}
	pointer operator->() const
	{
	  llassert(mContainer && !mIterator->dead);
	  return &mIterator->mElement;
	}
	_Self& operator++()
	{
	  _Iterator cur = mIterator;
	  ++cur;
	  unref();
	  while(cur != mContainer->end() && cur->dead)
	  {
		++cur;
	  }
	  mIterator = cur;
	  ref();
	  return *this;
	}
	_Self operator++(int)
	{
	  _Self tmp = *this;
	  this->operator++();
	  return tmp;
	}
	_Self& operator--()
	{
	  _Iterator cur = mIterator;
	  --cur;
	  unref();
	  while(cur->dead)
	  {
		--cur;
	  }
	  mIterator = cur;
	  ref();
	  return *this;
	}
	_Self operator--(int)
	{
	  _Self tmp = *this;
	  this->operator--();
	  return tmp;
	}
	bool operator==(_Self const& __x) const { return mIterator == __x.mIterator; }
	bool operator!=(_Self const& __x) const { return mIterator != __x.mIterator; }
	friend class AIList<T>;
	friend class AIConstListIterator<T>;
	template<typename T2> friend bool operator==(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
	template<typename T2> friend bool operator!=(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
	int count(void) const
	{
	  if (mContainer && mIterator != mContainer->end())
	  {
		return mIterator->count;
	  }
	  return 0;
	}
};
template<typename T>
class AIConstListIterator {
  private:
	typedef AIConstListIterator<T>					_Self;
	typedef std::list<AINode<T> >					_Container;
	typedef typename _Container::iterator			_Iterator;
	typedef typename _Container::const_iterator		_ConstIterator;
	typedef AIListIterator<T>						iterator;
	_Container const* mContainer;
	_ConstIterator mConstIterator;
	void ref(void)
	{
	  if (mContainer && mContainer->end() != mConstIterator)
	  {
		llassert(mConstIterator->count < 100);
		mConstIterator->count++;
	  }
	}
	void unref(void)
	{
	  if (mContainer && mContainer->end() != mConstIterator)
	  {
		llassert(mConstIterator->count > 0);
		mConstIterator->count--;
		if (mConstIterator->count == 0 && mConstIterator->dead)
		{
		  const_cast<_Container*>(mContainer)->erase(mConstIterator);
		}
	  }
	}
  public:
	typedef typename _ConstIterator::difference_type	difference_type;
	typedef typename _ConstIterator::iterator_category	iterator_category;
	typedef T											value_type;
	typedef T const*									pointer;
	typedef T const&									reference;
	AIConstListIterator(void) : mContainer(NULL) { }
	AIConstListIterator(_Container const* __c, _ConstIterator const& __i) : mContainer(__c), mConstIterator(__i)
	{
	  llassert(mContainer);
	  ref();
	}
	AIConstListIterator(iterator const& __x) : mContainer(__x.mContainer), mConstIterator(__x.mIterator)
	{
	  ref();
	}
	AIConstListIterator(AIConstListIterator const& __i) : mContainer(__i.mContainer), mConstIterator(__i.mConstIterator)
	{
	  ref();
	}
	~AIConstListIterator()
	{
	  unref();
	}
	_Self& operator=(_Self const& __x)
	{
	  unref();
	  mContainer = __x.mContainer;
	  mConstIterator = __x.mConstIterator;
	  llassert(mContainer);
	  ref();
	  return *this;
	}
	_Self& operator=(iterator const& __x)
	{
	  unref();
	  mContainer = __x.mContainer;
	  mConstIterator = __x.mIterator;
	  llassert(mContainer);
	  ref();
	  return *this;
	}
	reference operator*() const
	{
	  llassert(mContainer && !mConstIterator->dead);
	  return mConstIterator->mElement;
	}
	pointer operator->() const
	{
	  llassert(mContainer && !mConstIterator->dead);
	  return &mConstIterator->mElement;
	}
	_Self& operator++()
	{
	  _ConstIterator cur = mConstIterator;
	  ++cur;
	  unref();
	  while(cur != mContainer->end() && cur->dead)
	  {
		++cur;
	  }
	  mConstIterator = cur;
	  ref();
	  return *this;
	}
	_Self operator++(int)
	{
	  _Self tmp = *this;
	  this->operator++();
	  return tmp;
	}
	_Self& operator--()
	{
	  _ConstIterator cur = mConstIterator;
	  --cur;
	  unref();
	  while(cur->dead)
	  {
		--cur;
	  }
	  mConstIterator = cur;
	  ref();
	  return *this;
	}
	_Self operator--(int)
	{
	  _Self tmp = *this;
	  this->operator--();
	  return tmp;
	}
	bool operator==(_Self const& __x) const { return mConstIterator == __x.mConstIterator; }
	bool operator!=(_Self const& __x) const { return mConstIterator != __x.mConstIterator; }
	bool operator==(iterator const& __x) const { return mConstIterator == __x.mIterator; }
	bool operator!=(iterator const& __x) const { return mConstIterator != __x.mIterator; }
	template<typename T2> friend bool operator==(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
	template<typename T2> friend bool operator!=(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
	int count(void) const
	{
	  if (mContainer && mConstIterator != mContainer->end())
	  {
		return mConstIterator->count;
	  }
	  return 0;
	}
};
template<typename T>
inline bool operator==(AIListIterator<T> const& __x, AIConstListIterator<T> const& __y)
{
  return __x.mIterator == __y.mConstIterator;
}
template<typename T>
inline bool operator!=(AIListIterator<T> const& __x, AIConstListIterator<T> const& __y)
{
  return __x.mIterator != __y.mConstIterator;
}
template<typename T>
class AIList {
  private:
	typedef std::list<AINode<T> >					_Container;
	typedef typename _Container::iterator			_Iterator;
	typedef typename _Container::const_iterator		_ConstIterator;
	_Container mContainer;
	size_t mSize;
  public:
	typedef T										value_type;
	typedef T*										pointer;
	typedef T const*								const_pointer;
	typedef T&										reference;
	typedef T const&								const_reference;
	typedef AIListIterator<T>						iterator;
	typedef AIConstListIterator<T>					const_iterator;
	typedef std::reverse_iterator<iterator>			reverse_iterator;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;
	typedef size_t									size_type;
	typedef ptrdiff_t								difference_type;
	AIList(void) : mSize(0) { }
#ifdef LL_DEBUG
	~AIList() { clear(); }
#endif
	explicit AIList(size_type __n, value_type const& __val = value_type()) : mContainer(__n, AINode<T>(__val)), mSize(__n) { }
	AIList(AIList const& __list) : mSize(0)
	{
	  for (_ConstIterator __i = __list.mContainer.begin(); __i != __list.mContainer.end(); ++__i)
	  {
		if (!__i->dead)
		{
		  mContainer.push_back(AINode<T>(__i->mElement));
		  ++mSize;
		}
	  }
	}
	template<typename _InputIterator>
	  AIList(_InputIterator __first, _InputIterator __last) : mSize(0)
	{
	  for (_InputIterator __i = __first; __i != __last; ++__i)
	  {
		mContainer.push_back(AINode<T>(*__i));
		++mSize;
	  }
	}
	AIList& operator=(AIList const& __list)
	{
	  clear();
	  for (_ConstIterator __i = __list.mContainer.begin(); __i != __list.mContainer.end(); ++__i)
	  {
		if (!__i->dead)
		{
		  mContainer.push_back(AINode<T>(__i->mElement));
		  ++mSize;
		}
	  }
	  return *this;
	}
	iterator begin()
	{
	  _Iterator __i = mContainer.begin();
	  while(__i != mContainer.end() && __i->dead)
	  {
		++__i;
	  }
	  return iterator(&mContainer, __i);
	}
	const_iterator begin() const
	{
	  _Iterator __i = const_cast<_Container&>(mContainer).begin();
	  while(__i != mContainer.end() && __i->dead)
	  {
		++__i;
	  }
	  return const_iterator(&mContainer, __i);
	}
	iterator end() { return iterator(&mContainer, mContainer.end()); }
	const_iterator end() const { return const_iterator(&mContainer, const_cast<_Container&>(mContainer).end()); }
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	bool empty() const { return mSize == 0; }
	size_type size() const { return mSize; }
	size_type max_size() const { return mContainer.max_size(); }
	reference front() { iterator __i = begin(); return *__i; }
	const_reference front() const { const_iterator __i = begin(); return *__i; }
	reference back() { iterator __i = end(); --__i; return *__i; }
	const_reference back() const { const_iterator __i = end(); --__i; return *__i; }
	void push_front(value_type const& __x)
	{
	  mContainer.push_front(AINode<T>(__x));
	  ++mSize;
	}
	void pop_front()
	{
	  iterator __i = begin();
	  erase(__i);
	}
	void push_back(value_type const& __x)
	{
	  mContainer.push_back(AINode<T>(__x));
	  ++mSize;
	}
	void pop_back()
	{
	  iterator __i = end();
	  --__i;
	  erase(__i);
	}
	iterator insert(iterator __position, value_type const& __x)
	{
	  ++mSize;
	  return iterator(&mContainer, mContainer.insert(__position.mIterator, AINode<T>(__x)));
	}
	void clear()
	{
#ifdef LL_DEBUG
	  for (_Iterator __i = mContainer.begin(); __i != mContainer.end(); ++__i)
	  {
		llassert(__i->count == 0);
	  }
#endif
	  mContainer.clear();
	  mSize = 0;
	}
	iterator erase(iterator __position)
	{
	  llassert(__position.mContainer == &mContainer && __position.mIterator != mContainer.end() && __position.mIterator->count > 0 && !__position.mIterator->dead);
	  __position.mIterator->dead = 1;
	  --mSize;
	  return ++__position;
	}
	void remove(value_type const& __val)
	{
	  _Iterator const __e = mContainer.end();
	  for (_Iterator __i = mContainer.begin(); __i != __e;)
	  {
		if (!__i->dead && __i->mElement == __val)
		{
		  --mSize;
		  if (__i->count == 0)
		  {
			mContainer.erase(__i++);
			continue;
		  }
		  __i->dead = 1;
		}
		++__i;
	  }
	}
	void sort()
	{
#ifdef LL_DEBUG
	  for (_Iterator __i = mContainer.begin(); __i != mContainer.end(); ++__i)
	  {
		llassert(__i->count == 0);
	  }
#endif
	  mContainer.sort();
	}
	template<typename _StrictWeakOrdering>
	  struct PredWrapper
	  {
		_StrictWeakOrdering mPred;
		PredWrapper(_StrictWeakOrdering const& pred) : mPred(pred) { }
		bool operator()(AINode<T> const& __x, AINode<T> const& __y) const { return mPred(__x.mElement, __y.mElement); }
	  };
	template<typename _StrictWeakOrdering>
	  void sort(_StrictWeakOrdering const& pred)
	{
#ifdef LL_DEBUG
	  for (_Iterator __i = mContainer.begin(); __i != mContainer.end(); ++__i)
	  {
		llassert(__i->count == 0);
	  }
#endif
	  mContainer.sort(PredWrapper<_StrictWeakOrdering>(pred));
	}
};
#endif
