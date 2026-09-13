/**
 * @file   stringize.h
 * @author Nat Goodspeed
 * @date   2008-12-17
 * @brief  stringize(item) template function and STRINGIZE(expression) macro
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
#if ! defined(LL_STRINGIZE_H)
#define LL_STRINGIZE_H
#include <sstream>
#include <boost/lambda/lambda.hpp>
template <typename T>
std::string stringize(const T& item)
{
    std::ostringstream out;
    out << item;
    return out.str();
}
template <typename Functor>
std::string stringize_f(Functor const & f)
{
    std::ostringstream out;
    f(out);
    return out.str();
}
#define STRINGIZE(EXPRESSION) (stringize_f(boost::lambda::_1 << EXPRESSION))
template <typename T>
T destringize(std::string const & str)
{
	T val;
    std::istringstream in(str);
	in >> val;
    return val;
}
template <typename Functor>
void destringize_f(std::string const & str, Functor const & f)
{
    std::istringstream in(str);
    f(in);
}
#define DESTRINGIZE(STR, EXPRESSION) (destringize_f((STR), (boost::lambda::_1 >> EXPRESSION)))
#endif
