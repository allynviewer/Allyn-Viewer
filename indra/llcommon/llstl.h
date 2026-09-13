/** 
 * @file llstl.h
 * @brief helper object & functions for use with the stl.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_LLSTL_H
#define LL_LLSTL_H
#include <functional>
#include <algorithm>
#include <utility>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <typeinfo>
#include "stdtypes.h"
template <typename T1, typename T2>
struct compare_pair_first
{
	bool operator()(const std::pair<T1, T2>& a, const std::pair<T1, T2>& b) const
	{
		return a.first < b.first;
	}
};
template <typename T1, typename T2>
struct compare_pair_greater
{
	bool operator()(const std::pair<T1, T2>& a, const std::pair<T1, T2>& b) const
	{
		if (!(a.first < b.first))
			return true;
		else if (!(b.first < a.first))
			return false;
		else
			return !(a.second < b.second);
	}
};
template <typename T>
struct compare_pointer_contents
{
	typedef const T* Tptr;
	bool operator()(const Tptr& a, const Tptr& b) const
	{
		return *a < *b;
	}
};
struct DeletePointer
{
	template<typename T> void operator()(T* ptr) const
	{
		delete ptr;
	}
};
struct DeletePointerArray
{
	template<typename T> void operator()(T* ptr) const
	{
		delete[] ptr;
	}
};
struct DeletePairedPointer
{
	template<typename T> void operator()(T &ptr) const
	{
		delete ptr.second;
		ptr.second = NULL;
	}
};
struct DeletePairedPointerArray
{
	template<typename T> void operator()(T &ptr) const
	{
		delete[] ptr.second;
		ptr.second = NULL;
	}
};
template<typename T>
struct DeletePointerFunctor : public std::unary_function<T*, bool>
{
	bool operator()(T* ptr) const
	{
		delete ptr;
		return true;
	}
};
template<typename T>
struct DeleteArrayFunctor : public std::unary_function<T*, bool>
{
	bool operator()(T* ptr) const
	{
		delete[] ptr;
		return true;
	}
};
struct CopyNewPointer
{
	template<typename T> T* operator()(const T* ptr) const
	{
		return new T(*ptr);
	}
};
template<typename T, typename ALLOC>
void delete_and_clear(std::list<T*, ALLOC>& list)
{
	std::for_each(list.begin(), list.end(), DeletePointer());
	list.clear();
}
template<typename T, typename ALLOC>
void delete_and_clear(std::vector<T*, ALLOC>& vector)
{
	std::for_each(vector.begin(), vector.end(), DeletePointer());
	vector.clear();
}
template<typename T, typename COMPARE, typename ALLOC>
void delete_and_clear(std::set<T*, COMPARE, ALLOC>& set)
{
	std::for_each(set.begin(), set.end(), DeletePointer());
	set.clear();
}
template<typename K, typename V, typename COMPARE, typename ALLOC>
void delete_and_clear(std::map<K, V*, COMPARE, ALLOC>& map)
{
	std::for_each(map.begin(), map.end(), DeletePairedPointer());
	map.clear();
}
template<typename T>
void delete_and_clear(T*& ptr)
{
	delete ptr;
	ptr = NULL;
}
template<typename T>
void delete_and_clear_array(T*& ptr)
{
	delete[] ptr;
	ptr = NULL;
}
template <typename T>
inline bool is_in_map(const T& inmap, typename T::key_type const& key)
{
	return inmap.find(key) != inmap.end();
}
template <typename T>
inline typename T::mapped_type get_if_there(const T& inmap, typename T::key_type const& key, typename T::mapped_type default_value)
{
	typedef typename T::const_iterator map_iter;
	map_iter iter = inmap.find(key);
	if(iter == inmap.end())
	{
		return default_value;
	}
	else
	{
		return iter->second;
	}
};
template <typename T>
inline typename T::mapped_type get_ptr_in_map(const T& inmap, typename T::key_type const& key)
{
	return get_if_there(inmap,key,NULL);
};
template <typename T, typename P>
inline typename T::iterator get_in_vec(T& invec, P pred)
{
	return std::find_if(invec.begin(), invec.end(), pred);
}
template <typename T, typename K>
inline typename T::value_type::second_type& get_val_in_pair_vec(T& invec, K key)
{
	auto it = get_in_vec(invec, [&key](typename T::value_type& e) { return e.first == key; });
	if (it == invec.end())
	{
		invec.emplace_back(key, typename T::value_type::second_type());
		return invec.back().second;
	}
	return it->second;
}
template <typename T>
inline typename T::iterator vector_replace_with_last(T& invec, typename T::iterator iter)
{
	typename T::iterator last = invec.end();
	if (iter == invec.end())
	{
		return iter;
	}
	else if (iter == --last)
	{
		invec.pop_back();
		return invec.end();
	}
	else
	{
		*iter = *last;
		invec.pop_back();
		return iter;
	}
};
template <typename T>
inline bool vector_replace_with_last(T& invec, typename T::value_type const& val)
{
	typename T::iterator iter = std::find(invec.begin(), invec.end(), val);
	if (iter != invec.end())
	{
		typename T::iterator last = invec.end(); --last;
		*iter = *last;
		invec.pop_back();
		return true;
	}
	return false;
}
template <typename T>
inline T* vector_append(std::vector<T>& invec, size_t N)
{
	size_t sz = invec.size();
	invec.resize(sz+N);
	return &(invec[sz]);
}
template <class InputIter, class Size, class Function>
Function ll_for_n(InputIter first, Size n, Function f)
{
	for ( ; n > 0; --n, ++first)
		f(*first);
	return f;
}
template <class InputIter, class Size, class OutputIter>
OutputIter ll_copy_n(InputIter first, Size n, OutputIter result)
{
	for ( ; n > 0; --n, ++result, ++first)
		*result = *first;
	return result;
}
template <class InputIter, class OutputIter, class Size, class UnaryOp>
OutputIter ll_transform_n(
	InputIter first,
	Size n,
	OutputIter result,
	UnaryOp op)
{
	for ( ; n > 0; --n, ++result, ++first)
		*result = op(*first);
	return result;
}
template <class _Pair>
struct _LLSelect1st : public std::unary_function<_Pair, typename _Pair::first_type> {
  const typename _Pair::first_type& operator()(const _Pair& __x) const {
	return __x.first;
  }
};
template <class _Pair>
struct _LLSelect2nd : public std::unary_function<_Pair, typename _Pair::second_type>
{
  const typename _Pair::second_type& operator()(const _Pair& __x) const {
	return __x.second;
  }
};
template <class _Pair> struct llselect1st : public _LLSelect1st<_Pair> {};
template <class _Pair> struct llselect2nd : public _LLSelect2nd<_Pair> {};
template <class _Operation1, class _Operation2>
class ll_unary_compose :
	public std::unary_function<typename _Operation2::argument_type,
							   typename _Operation1::result_type>
{
protected:
  _Operation1 __op1;
  _Operation2 __op2;
public:
  ll_unary_compose(const _Operation1& __x, const _Operation2& __y)
	: __op1(__x), __op2(__y) {}
  typename _Operation1::result_type
  operator()(const typename _Operation2::argument_type& __x) const {
	return __op1(__op2(__x));
  }
};
template <class _Operation1, class _Operation2>
inline ll_unary_compose<_Operation1,_Operation2>
llcompose1(const _Operation1& __op1, const _Operation2& __op2)
{
  return ll_unary_compose<_Operation1,_Operation2>(__op1, __op2);
}
template <class _Operation1, class _Operation2, class _Operation3>
class ll_binary_compose
  : public std::unary_function<typename _Operation2::argument_type,
							   typename _Operation1::result_type> {
protected:
  _Operation1 _M_op1;
  _Operation2 _M_op2;
  _Operation3 _M_op3;
public:
  ll_binary_compose(const _Operation1& __x, const _Operation2& __y,
					const _Operation3& __z)
	: _M_op1(__x), _M_op2(__y), _M_op3(__z) { }
  typename _Operation1::result_type
  operator()(const typename _Operation2::argument_type& __x) const {
	return _M_op1(_M_op2(__x), _M_op3(__x));
  }
};
template <class _Operation1, class _Operation2, class _Operation3>
inline ll_binary_compose<_Operation1, _Operation2, _Operation3>
llcompose2(const _Operation1& __op1, const _Operation2& __op2,
		 const _Operation3& __op3)
{
  return ll_binary_compose<_Operation1,_Operation2,_Operation3>
	(__op1, __op2, __op3);
}
template <class _Operation>
class llbinder1st :
	public std::unary_function<typename _Operation::second_argument_type,
							   typename _Operation::result_type> {
protected:
  _Operation op;
  typename _Operation::first_argument_type value;
public:
  llbinder1st(const _Operation& __x,
			  const typename _Operation::first_argument_type& __y)
	  : op(__x), value(__y) {}
	typename _Operation::result_type
	operator()(const typename _Operation::second_argument_type& __x) const {
		return op(value, __x);
	}
};
template <class _Operation, class _Tp>
inline llbinder1st<_Operation>
llbind1st(const _Operation& __oper, const _Tp& __x)
{
  typedef typename _Operation::first_argument_type _Arg1_type;
  return llbinder1st<_Operation>(__oper, _Arg1_type(__x));
}
template <class _Operation>
class llbinder2nd
	: public std::unary_function<typename _Operation::first_argument_type,
								 typename _Operation::result_type> {
protected:
	_Operation op;
	typename _Operation::second_argument_type value;
public:
	llbinder2nd(const _Operation& __x,
				const typename _Operation::second_argument_type& __y)
		: op(__x), value(__y) {}
	typename _Operation::result_type
	operator()(const typename _Operation::first_argument_type& __x) const {
		return op(__x, value);
	}
};
template <class _Operation, class _Tp>
inline llbinder2nd<_Operation>
llbind2nd(const _Operation& __oper, const _Tp& __x)
{
  typedef typename _Operation::second_argument_type _Arg2_type;
  return llbinder2nd<_Operation>(__oper, _Arg2_type(__x));
}
inline
bool before(const std::type_info* lhs, const std::type_info* rhs)
{
    return lhs->before(*rhs) ? true : false;
}
namespace std
{
	template <>
	struct less<const std::type_info*>:
		public std::binary_function<const std::type_info*, const std::type_info*, bool>
	{
		bool operator()(const std::type_info* lhs, const std::type_info* rhs) const
		{
			return before(lhs, rhs);
		}
	};
	template <>
	struct less<std::type_info*>:
		public std::binary_function<std::type_info*, std::type_info*, bool>
	{
		bool operator()(std::type_info* lhs, std::type_info* rhs) const
		{
			return before(lhs, rhs);
		}
	};
}
template <typename T, typename U>
struct ll_template_cast_impl
{
	T operator()(U)
	{
		return 0;
	}
};
template <typename T, typename U>
T ll_template_cast(U value)
{
	return ll_template_cast_impl<T, U>()(value);
}
template <typename T>
struct ll_template_cast_impl<T, T>
{
	T operator()(T value)
	{
		return value;
	}
};
#define LL_TEMPLATE_CONVERTIBLE(DEST, SOURCE)   \
template <>                                     \
struct ll_template_cast_impl<DEST, SOURCE>      \
{                                               \
	DEST operator()(SOURCE wrapper)             \
	{                                           \
		return wrapper;                         \
	}                                           \
}
#endif
