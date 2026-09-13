/** 
 * @file llfiltersd2xmlrpc.h
 * @author Phoenix
 * @date 2005-04-26
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
#ifndef LL_LLFILTERSD2XMLRPC_H
#define LL_LLFILTERSD2XMLRPC_H
#include <iosfwd>
#include "lliopipe.h"
class LLFilterSD2XMLRPC : public LLIOPipe
{
public:
	LLFilterSD2XMLRPC();
	virtual ~LLFilterSD2XMLRPC();
protected:
	void streamOut(std::ostream& ostr, const LLSD& sd);
};
class LLFilterSD2XMLRPCResponse : public LLFilterSD2XMLRPC
{
public:
	LLFilterSD2XMLRPCResponse();
	virtual ~LLFilterSD2XMLRPCResponse();
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
};
class LLFilterSD2XMLRPCRequest : public LLFilterSD2XMLRPC
{
public:
	LLFilterSD2XMLRPCRequest();
	LLFilterSD2XMLRPCRequest(const char* method);
	virtual ~LLFilterSD2XMLRPCRequest();
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
	std::string mMethod;
};
class LLFilterXMLRPCResponse2LLSD : public LLIOPipe
{
public:
	LLFilterXMLRPCResponse2LLSD();
	virtual ~LLFilterXMLRPCResponse2LLSD();
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
};
class LLFilterXMLRPCRequest2LLSD : public LLIOPipe
{
public:
	LLFilterXMLRPCRequest2LLSD();
	virtual ~LLFilterXMLRPCRequest2LLSD();
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
};
std::string xml_escape_string(const std::string& in);
extern const char LLSDRPC_REQUEST_HEADER_1[];
extern const char LLSDRPC_REQUEST_HEADER_2[];
extern const char LLSDRPC_REQUEST_FOOTER[];
#endif
