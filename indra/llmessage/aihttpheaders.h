/**
 * @file aihttpheaders.h
 * @brief Keep a list of HTTP headers.
 *
 * Copyright (c) 2012, Aleric Inglewood.
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
 *   15/08/2012
 *   Initial version, written by Aleric Inglewood @ SL
 *   21/10/2012
 *   Added AIHTTPReceivedHeaders
 */
#ifndef AIHTTPHEADERS_H
#define AIHTTPHEADERS_H
#include <string>
#include <map>
#include <iosfwd>
#include <algorithm>
#include "llpointer.h"
#include "llthread.h"
extern "C" struct curl_slist;
class AIHTTPHeaders {
  public:
	enum op_type
	{
	  new_header,
	  replace_if_exists,
	  keep_existing_header
	};
	AIHTTPHeaders(void) { }
	AIHTTPHeaders(std::string const& key, std::string const& value);
	void clear(void) { if (mContainer) mContainer->mKeyValuePairs.clear(); }
	bool addHeader(std::string const& key, std::string const& value, op_type op = new_header);
	bool empty(void) const { return !mContainer || mContainer->mKeyValuePairs.empty(); }
	bool hasHeader(std::string const& key) const;
	bool getValue(std::string const& key, std::string& value_out) const;
	void append_to(curl_slist*& slist) const;
	friend std::ostream& operator<<(std::ostream& os, AIHTTPHeaders const& headers);
  private:
	typedef std::map<std::string, std::string> container_t;
	typedef std::pair<container_t::iterator, bool> insert_t;
	struct Container : public LLThreadSafeRefCount {
	  container_t mKeyValuePairs;
	};
	LLPointer<Container> mContainer;
};
struct AIHTTPReceivedHeadersCharCompare {
  bool operator()(std::string::value_type c1, std::string::value_type c2) const
  {
	static std::string::value_type const bit5 = 0x20;
	return (c1 | bit5) < (c2 | bit5);
  }
};
struct AIHTTPReceivedHeadersCompare {
  bool operator()(std::string const& h1, std::string const& h2) const
  {
	static AIHTTPReceivedHeadersCharCompare predicate;
	return std::lexicographical_compare(h1.begin(), h1.end(), h2.begin(), h2.end(), predicate);
  }
};
class AIHTTPReceivedHeaders {
  private:
	typedef std::multimap<std::string, std::string, AIHTTPReceivedHeadersCompare> container_t;
  public:
	typedef container_t::const_iterator iterator_type;
	typedef std::pair<iterator_type, iterator_type> range_type;
	AIHTTPReceivedHeaders(void) { }
	void clear(void) { if (mContainer) mContainer->mKeyValuePairs.clear(); }
	void addHeader(std::string const& key, std::string const& value);
	void swap(AIHTTPReceivedHeaders& headers) { LLPointer<Container>::swap(mContainer, headers.mContainer); }
	bool empty(void) const { return !mContainer || mContainer->mKeyValuePairs.empty(); }
	bool hasHeader(std::string const& key) const;
	bool getFirstValue(std::string const& key, std::string& value_out) const;
	bool getValues(std::string const& key, range_type& value_out) const;
	friend std::ostream& operator<<(std::ostream& os, AIHTTPReceivedHeaders const& headers);
	static bool equal(std::string const& key1, std::string const& key2);
  private:
	struct Container : public LLThreadSafeRefCount {
	  container_t mKeyValuePairs;
	};
	LLPointer<Container> mContainer;
};
#endif
