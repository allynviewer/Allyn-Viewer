/**
 * @file   lleventdispatcher.h
 * @author Nat Goodspeed
 * @date   2009-06-18
 * @brief  Central mechanism for dispatching events by string name. This is
 *         useful when you have a single LLEventPump listener on which you can
 *         request different operations, vs. instantiating a different
 *         LLEventPump for each such operation.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
 *
 * The invoker machinery that constructs a boost::fusion argument list for use
 * with boost::fusion::invoke() is derived from
 * http://www.boost.org/doc/libs/1_45_0/libs/function_types/example/interpreter.hpp
 * whose license information is copied below:
 *
 * "(C) Copyright Tobias Schwinger
 *
 * Use modification and distribution are subject to the boost Software License,
 * Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)."
 */
#if ! defined(LL_LLEVENTDISPATCHER_H)
#define LL_LLEVENTDISPATCHER_H
#if defined(nil)
static const int nil_(nil);
#undef nil
static const int& nil(nil_);
#endif
#include <string>
#include <boost/function.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/function_types/is_nonmember_callable_builtin.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/fusion/include/push_back.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/invoke.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>
#include <typeinfo>
#include "llevents.h"
#include "llsdutil.h"
class LLSD;
class LL_COMMON_API LLEventDispatcher
{
public:
    LLEventDispatcher(const std::string& desc, const std::string& key);
    virtual ~LLEventDispatcher();
    typedef boost::function<void(const LLSD&)> Callable;
    void add(const std::string& name,
             const std::string& desc,
             const Callable& callable,
             const LLSD& required=LLSD());
    void add(const std::string& name,
             const std::string& desc,
             void (*f)(const LLSD&),
             const LLSD& required=LLSD())
    {
        add(name, desc, Callable(f), required);
    }
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(const LLSD&),
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(const LLSD&) const,
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }
    template<typename Function>
    typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Function f);
    template<typename Method, typename InstanceGetter>
    typename boost::enable_if< boost::function_types::is_member_function_pointer<Method>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Method f,
                                           const InstanceGetter& getter);
    template<typename Function>
    typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Function f,
                                           const LLSD& params,
                                           const LLSD& defaults=LLSD());
    template<typename Method, typename InstanceGetter>
    typename boost::enable_if< boost::function_types::is_member_function_pointer<Method>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Method f,
                                           const InstanceGetter& getter,
                                           const LLSD& params,
                                           const LLSD& defaults=LLSD());
    bool remove(const std::string& name);
    void operator()(const std::string& name, const LLSD& event) const;
    bool try_call(const std::string& name, const LLSD& event) const;
    void operator()(const LLSD& event) const;
    bool try_call(const LLSD& event) const;
    typedef std::pair<std::string, std::string> NameDesc;
private:
    struct DispatchEntry
    {
        DispatchEntry(const std::string& desc);
        virtual ~DispatchEntry() {}
        std::string mDesc;
        virtual void call(const std::string& desc, const LLSD& event) const = 0;
        virtual LLSD addMetadata(LLSD) const = 0;
    };
    typedef std::map<std::string, std::shared_ptr<DispatchEntry> > DispatchMap;
public:
    typedef boost::transform_iterator<NameDesc(*)(const DispatchMap::value_type&), DispatchMap::const_iterator> const_iterator;
    const_iterator begin() const
    {
        return boost::make_transform_iterator(mDispatch.begin(), makeNameDesc);
    }
    const_iterator end() const
    {
        return boost::make_transform_iterator(mDispatch.end(), makeNameDesc);
    }
    LLSD getMetadata(const std::string& name) const;
    std::string getDispatchKey() const { return mKey; }
private:
    template <class CLASS, typename METHOD>
    void addMethod(const std::string& name, const std::string& desc,
                   const METHOD& method, const LLSD& required)
    {
        CLASS* downcast = dynamic_cast<CLASS*>(this);
        if (! downcast)
        {
            addFail(name, typeid(CLASS).name());
        }
        else
        {
            add(name, desc, boost::bind(method, downcast, _1), required);
        }
    }
    void addFail(const std::string& name, const std::string& classname) const;
    std::string mDesc, mKey;
    DispatchMap mDispatch;
    static NameDesc makeNameDesc(const DispatchMap::value_type& item)
    {
        return NameDesc(item.first, item.second->mDesc);
    }
    struct LLSDDispatchEntry;
    struct ParamsDispatchEntry;
    struct ArrayParamsDispatchEntry;
    struct MapParamsDispatchEntry;
    template< typename Function
              , class From = typename boost::mpl::begin< boost::function_types::parameter_types<Function> >::type
              , class To   = typename boost::mpl::end< boost::function_types::parameter_types<Function> >::type
              >
    struct invoker;
    typedef boost::function<LLSD()> args_source;
    typedef boost::function<void(const args_source&)> invoker_function;
    template <typename Function>
    invoker_function make_invoker(Function f);
    template <typename Method, typename InstanceGetter>
    invoker_function make_invoker(Method f, const InstanceGetter& getter);
    void addArrayParamsDispatchEntry(const std::string& name,
                                     const std::string& desc,
                                     const invoker_function& invoker,
                                     LLSD::Integer arity);
    void addMapParamsDispatchEntry(const std::string& name,
                                   const std::string& desc,
                                   const invoker_function& invoker,
                                   const LLSD& params,
                                   const LLSD& defaults);
};
template<typename Function, class From, class To>
struct LLEventDispatcher::invoker
{
    template<typename T>
    struct remove_cv_ref
        : boost::remove_cv< typename boost::remove_reference<T>::type >
    { };
    template<typename Args>
    static inline
    void apply(Function func, const args_source& argsrc, Args const & args)
    {
        typedef typename boost::mpl::deref<From>::type arg_type;
        typedef typename boost::mpl::next<From>::type next_iter_type;
        typedef typename remove_cv_ref<arg_type>::type plain_arg_type;
        invoker<Function, next_iter_type, To>::apply
        ( func, argsrc, boost::fusion::push_back(args, LLSDParam<plain_arg_type>(argsrc())));
    }
    template <typename InstanceGetter>
    static inline
    void method_apply(Function func, const args_source& argsrc, const InstanceGetter& getter)
    {
        typedef typename boost::mpl::next<From>::type next_iter_type;
        invoker<Function, next_iter_type, To>::apply
        ( func, argsrc, boost::fusion::push_back(boost::fusion::nil(), std::ref(getter())));
    }
};
template<typename Function, class To>
struct LLEventDispatcher::invoker<Function,To,To>
{
    template<typename Args>
    static inline
    void apply(Function func, const args_source&, Args const & args)
    {
        boost::fusion::invoke(func, args);
    }
};
template<typename Function>
typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Function f)
{
    addArrayParamsDispatchEntry(name, desc, make_invoker(f),
                                boost::function_types::function_arity<Function>::value);
}
template<typename Method, typename InstanceGetter>
typename boost::enable_if< boost::function_types::is_member_function_pointer<Method> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Method f,
                       const InstanceGetter& getter)
{
    addArrayParamsDispatchEntry(name, desc, make_invoker(f, getter),
                                boost::function_types::function_arity<Method>::value - 1);
}
template<typename Function>
typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Function f,
                       const LLSD& params, const LLSD& defaults)
{
    addMapParamsDispatchEntry(name, desc, make_invoker(f), params, defaults);
}
template<typename Method, typename InstanceGetter>
typename boost::enable_if< boost::function_types::is_member_function_pointer<Method> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Method f,
                       const InstanceGetter& getter,
                       const LLSD& params, const LLSD& defaults)
{
    addMapParamsDispatchEntry(name, desc, make_invoker(f, getter), params, defaults);
}
template <typename Function>
LLEventDispatcher::invoker_function
LLEventDispatcher::make_invoker(Function f)
{
    return boost::bind(&invoker<Function>::template apply<boost::fusion::nil>,
                       f,
                       _1,
                       boost::fusion::nil());
}
template <typename Method, typename InstanceGetter>
LLEventDispatcher::invoker_function
LLEventDispatcher::make_invoker(Method f, const InstanceGetter& getter)
{
    return boost::bind(&invoker<Method>::template method_apply<InstanceGetter>,
                       f,
                       _1,
                       getter);
}
class LL_COMMON_API LLDispatchListener: public LLEventDispatcher
{
public:
    LLDispatchListener(const std::string& pumpname, const std::string& key);
    std::string getPumpName() const { return mPump.getName(); }
private:
    bool process(const LLSD& event);
    LLEventStream mPump;
    LLTempBoundListener mBoundListener;
};
#endif
