/** 
 * @file llsd.h
 * @brief LLSD flexible data system.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#ifndef LL_LLSD_NEW_H
#define LL_LLSD_NEW_H
#include <map>
#include <string>
#include <vector>
#include "stdtypes.h"
#include "lldate.h"
#include "lluri.h"
#include "lluuid.h"
class LL_COMMON_API LLSD
{
public:
		LLSD();
		~LLSD();
		LLSD(const LLSD&);
		void assign(const LLSD& other);
		LLSD& operator=(const LLSD& other)	{ assign(other); return *this; }
	void clear();
		typedef bool			Boolean;
		typedef S32				Integer;
		typedef F64				Real;
		typedef F32				Float;
		typedef std::string		String;
		typedef LLUUID			UUID;
		typedef LLDate			Date;
		typedef LLURI			URI;
		typedef std::vector<U8>	Binary;
		LLSD(Boolean);
		LLSD(Integer);
		LLSD(Real);
		LLSD(const String&);
		LLSD(const UUID&);
		LLSD(const Date&);
		LLSD(const URI&);
		LLSD(const Binary&);
		LLSD(F32);
		void assign(Boolean);
		void assign(Integer);
		void assign(Real);
		void assign(const String&);
		void assign(const UUID&);
		void assign(const Date&);
		void assign(const URI&);
		void assign(const Binary&);
		LLSD& operator=(Boolean v)			{ assign(v); return *this; }
		LLSD& operator=(Integer v)			{ assign(v); return *this; }
		LLSD& operator=(Real v)				{ assign(v); return *this; }
		LLSD& operator=(const String& v)	{ assign(v); return *this; }
		LLSD& operator=(const UUID& v)		{ assign(v); return *this; }
		LLSD& operator=(const Date& v)		{ assign(v); return *this; }
		LLSD& operator=(const URI& v)		{ assign(v); return *this; }
		LLSD& operator=(const Binary& v)	{ assign(v); return *this; }
		Boolean	asBoolean() const;
		Integer	asInteger() const;
		Real	asReal() const;
		Float	asFloat() const {return (F32)asReal();}
		String	asString() const;
		UUID	asUUID() const;
		Date	asDate() const;
		URI		asURI() const;
		const Binary&	asBinary() const;
		const String&	asStringRef() const;
		operator Boolean() const	{ return asBoolean(); }
		operator Integer() const	{ return asInteger(); }
		operator Real() const		{ return asReal(); }
		operator Float() const		{ return asFloat(); }
		operator String() const		{ return asString(); }
		operator UUID() const		{ return asUUID(); }
		operator Date() const		{ return asDate(); }
		operator URI() const		{ return asURI(); }
		operator Binary() const		{ return asBinary(); }
		bool operator!() const {return !asBoolean();}
		LLSD(const char*);
		void assign(const char*);
		LLSD& operator=(const char* v)	{ assign(v); return *this; }
		static LLSD emptyMap();
		bool has(const String&) const;
		LLSD get(const String&) const;
		LLSD getKeys() const;
		void insert(const String&, const LLSD&);
		void erase(const String&);
		LLSD& with(const String&, const LLSD&);
		LLSD& operator[](const String&);
		LLSD& operator[](const char* c)			{ return (*this)[String(c)]; }
		const LLSD& operator[](const String&) const;
		const LLSD& operator[](const char* c) const	{ return (*this)[String(c)]; }
		static LLSD emptyArray();
		LLSD get(Integer) const;
		void set(Integer, const LLSD&);
		void insert(Integer, const LLSD&);
		LLSD& append(const LLSD&);
		void erase(Integer);
		LLSD& with(Integer, const LLSD&);
		const LLSD& operator[](Integer) const;
		LLSD& operator[](Integer);
		int size() const;
		typedef std::map<String, LLSD>::iterator		map_iterator;
		typedef std::map<String, LLSD>::const_iterator	map_const_iterator;
		std::map<String, LLSD>& map();
		const std::map<String, LLSD>& map() const;
		map_iterator		beginMap();
		map_iterator		endMap();
		map_const_iterator	beginMap() const;
		map_const_iterator	endMap() const;
		typedef std::vector<LLSD>::iterator			array_iterator;
		typedef std::vector<LLSD>::const_iterator	array_const_iterator;
		typedef std::vector<LLSD>::reverse_iterator reverse_array_iterator;
		std::vector<LLSD>&      array();
		const std::vector<LLSD>& array() const;
		array_iterator			beginArray();
		array_iterator			endArray();
		array_const_iterator	beginArray() const;
		array_const_iterator	endArray() const;
		reverse_array_iterator	rbeginArray();
		reverse_array_iterator	rendArray();
		enum Type {
			TypeUndefined = 0,
			TypeBoolean,
			TypeInteger,
			TypeReal,
			TypeString,
			TypeUUID,
			TypeDate,
			TypeURI,
			TypeBinary,
			TypeMap,
			TypeArray,
			TypeLLSDTypeEnd,
			TypeLLSDTypeBegin = TypeUndefined,
			TypeLLSDNumTypes = (TypeLLSDTypeEnd - TypeLLSDTypeBegin)
		};
		Type type() const;
		bool isUndefined() const	{ return type() == TypeUndefined; }
		bool isDefined() const		{ return type() != TypeUndefined; }
		bool isBoolean() const		{ return type() == TypeBoolean; }
		bool isInteger() const		{ return type() == TypeInteger; }
		bool isReal() const			{ return type() == TypeReal; }
		bool isString() const		{ return type() == TypeString; }
		bool isUUID() const			{ return type() == TypeUUID; }
		bool isDate() const			{ return type() == TypeDate; }
		bool isURI() const			{ return type() == TypeURI; }
		bool isBinary() const		{ return type() == TypeBinary; }
		bool isMap() const			{ return type() == TypeMap; }
		bool isArray() const		{ return type() == TypeArray; }
		LLSD(const void*) = delete;
		void assign(const void*) = delete;
		LLSD& operator=(const void*) = delete;
		bool has(Integer) const;
public:
		class Impl;
private:
		Impl* impl;
		friend class LLSD::Impl;
private:
		static const char *dumpXML(const LLSD &llsd);
		static const char *dump(const LLSD &llsd);
public:
	static std::string		typeString(Type type);
};
struct llsd_select_bool : public std::unary_function<LLSD, LLSD::Boolean>
{
	LLSD::Boolean operator()(const LLSD& sd) const
	{
		return sd.asBoolean();
	}
};
struct llsd_select_integer : public std::unary_function<LLSD, LLSD::Integer>
{
	LLSD::Integer operator()(const LLSD& sd) const
	{
		return sd.asInteger();
	}
};
struct llsd_select_real : public std::unary_function<LLSD, LLSD::Real>
{
	LLSD::Real operator()(const LLSD& sd) const
	{
		return sd.asReal();
	}
};
struct llsd_select_float : public std::unary_function<LLSD, F32>
{
	F32 operator()(const LLSD& sd) const
	{
		return (F32)sd.asReal();
	}
};
struct llsd_select_uuid : public std::unary_function<LLSD, LLSD::UUID>
{
	LLSD::UUID operator()(const LLSD& sd) const
	{
		return sd.asUUID();
	}
};
struct llsd_select_string : public std::unary_function<LLSD, LLSD::String>
{
	LLSD::String operator()(const LLSD& sd) const
	{
		return sd.asString();
	}
};
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLSD& llsd);
namespace llsd
{
#ifdef LLSD_DEBUG_INFO
	LL_COMMON_API void dumpStats(const LLSD&);
	LL_COMMON_API U32 allocationCount();
	LL_COMMON_API U32 outstandingCount();
	LL_COMMON_API extern S32 sLLSDAllocationCount;
	LL_COMMON_API extern S32 sLLSDNetObjects;
#endif
}
#endif
