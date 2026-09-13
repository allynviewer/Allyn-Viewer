/** 
 * @file llfiltersd2xmlrpc.cpp
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
#include "linden_common.h"
#include "llfiltersd2xmlrpc.h"
#include <sstream>
#include <iterator>
#include <xmlrpc-epi/xmlrpc.h>
#include "apr_base64.h"
#include "llbuffer.h"
#include "llbufferstream.h"
#include "llmemorystream.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "lluuid.h"
#include "llfasttimer.h"
static const char XML_HEADER[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
static const char XMLRPC_REQUEST_HEADER_1[] = "<methodCall><methodName>";
static const char XMLRPC_REQUEST_HEADER_2[] = "</methodName><params>";
static const char XMLRPC_REQUEST_FOOTER[] = "</params></methodCall>";
static const char XMLRPC_METHOD_RESPONSE_HEADER[] = "<methodResponse>";
static const char XMLRPC_METHOD_RESPONSE_FOOTER[] = "</methodResponse>";
static const char XMLRPC_RESPONSE_HEADER[] = "<params><param>";
static const char XMLRPC_RESPONSE_FOOTER[] = "</param></params>";
static const char XMLRPC_FAULT_1[] = "<fault><value><struct><member><name>faultCode</name><value><int>";
static const char XMLRPC_FAULT_2[] = "</int></value></member><member><name>faultString</name><value><string>";
static const char XMLRPC_FAULT_3[] = "</string></value></member></struct></value></fault>";
static const char LLSDRPC_RESPONSE_HEADER[] = "{'response':";
static const char LLSDRPC_RESPONSE_FOOTER[] = "}";
const char LLSDRPC_REQUEST_HEADER_1[] = "{'method':'";
const char LLSDRPC_REQUEST_HEADER_2[] = "', 'parameter': ";
const char LLSDRPC_REQUEST_FOOTER[] = "}";
static const char LLSDRPC_FAULT_HADER_1[] = "{ 'fault': {'code':i";
static const char LLSDRPC_FAULT_HADER_2[] = ", 'description':";
static const char LLSDRPC_FAULT_FOOTER[] = "} }";
static const S32 DEFAULT_PRECISION = 20;
LLFilterSD2XMLRPC::LLFilterSD2XMLRPC()
{
}
LLFilterSD2XMLRPC::~LLFilterSD2XMLRPC()
{
}
std::string xml_escape_string(const std::string& in)
{
	std::ostringstream out;
	std::string::const_iterator it = in.begin();
	std::string::const_iterator end = in.end();
	for(; it != end; ++it)
	{
		switch((*it))
		{
		case '<':
			out << "&lt;";
			break;
		case '>':
			out << "&gt;";
			break;
		case '&':
			out << "&amp;";
			break;
		case '\'':
			out << "&apos;";
			break;
		case '"':
			out << "&quot;";
			break;
		default:
			out << (*it);
			break;
		}
	}
	return out.str();
}
void LLFilterSD2XMLRPC::streamOut(std::ostream& ostr, const LLSD& sd)
{
	ostr << "<value>";
	switch(sd.type())
	{
	case LLSD::TypeMap:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(map) BEGIN" << LL_ENDL;
#endif
		ostr << "<struct>";
		if(ostr.fail())
		{
			LL_INFOS() << "STREAM FAILURE writing struct" << LL_ENDL;
		}
		LLSD::map_const_iterator it = sd.beginMap();
		LLSD::map_const_iterator end = sd.endMap();
		for(; it != end; ++it)
		{
			ostr << "<member><name>" << xml_escape_string((*it).first)
				<< "</name>";
			streamOut(ostr, (*it).second);
			if(ostr.fail())
			{
				LL_INFOS() << "STREAM FAILURE writing '" << (*it).first
						<< "' with sd type " << (*it).second.type() << LL_ENDL;
			}
			ostr << "</member>";
		}
		ostr << "</struct>";
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(map) END" << LL_ENDL;
#endif
		break;
	}
	case LLSD::TypeArray:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(array) BEGIN" << LL_ENDL;
#endif
		ostr << "<array><data>";
		LLSD::array_const_iterator it = sd.beginArray();
		LLSD::array_const_iterator end = sd.endArray();
		for(; it != end; ++it)
		{
			streamOut(ostr, *it);
			if(ostr.fail())
			{
				LL_INFOS() << "STREAM FAILURE writing array element sd type "
						<< (*it).type() << LL_ENDL;
			}
		}
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(array) END" << LL_ENDL;
#endif
		ostr << "</data></array>";
		break;
	}
	case LLSD::TypeUndefined:
	case LLSD::TypeBoolean:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(bool)" << LL_ENDL;
#endif
		ostr << "<boolean>" << (sd.asBoolean() ? "1" : "0") << "</boolean>";
		break;
	case LLSD::TypeInteger:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(int)" << LL_ENDL;
#endif
		ostr << "<i4>" << sd.asInteger() << "</i4>";
		break;
	case LLSD::TypeReal:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(real)" << LL_ENDL;
#endif
		ostr << "<double>" << sd.asReal() << "</double>";
		break;
	case LLSD::TypeString:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(string)" << LL_ENDL;
#endif
		ostr << "<string>" << xml_escape_string(sd.asString()) << "</string>";
		break;
	case LLSD::TypeUUID:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(uuid)" << LL_ENDL;
#endif
		ostr << "<string>" << sd.asString() << "</string>";
		break;
	case LLSD::TypeURI:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(uri)" << LL_ENDL;
#endif
		ostr << "<string>" << xml_escape_string(sd.asString()) << "</string>";
		break;
	}
	case LLSD::TypeBinary:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(binary)" << LL_ENDL;
#endif
		ostr << "<base64>";
		LLSD::Binary buffer = sd.asBinary();
		if(!buffer.empty())
		{
			int b64_buffer_length = apr_base64_encode_len(buffer.size());
			char* b64_buffer = new char[b64_buffer_length];
			b64_buffer_length = apr_base64_encode_binary(
				b64_buffer,
				&buffer[0],
				buffer.size());
			ostr.write(b64_buffer, b64_buffer_length - 1);
			delete[] b64_buffer;
		}
		ostr << "</base64>";
		break;
	}
	case LLSD::TypeDate:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		LL_INFOS() << "streamOut(date)" << LL_ENDL;
#endif
		ostr << "<dateTime.iso8601>" << sd.asString() << "</dateTime.iso8601>";
		break;
	default:
		LL_WARNS() << "Unhandled structured data type: " << sd.type()
			<< LL_ENDL;
		break;
	}
	ostr << "</value>";
}
LLFilterSD2XMLRPCResponse::LLFilterSD2XMLRPCResponse()
{
}
LLFilterSD2XMLRPCResponse::~LLFilterSD2XMLRPCResponse()
{
}
static LLTrace::BlockTimerStatHandle FTM_PROCESS_SD2XMLRPC_RESPONSE("SD2XMLRPC Response");
LLIOPipe::EStatus LLFilterSD2XMLRPCResponse::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LL_RECORD_BLOCK_TIME(FTM_PROCESS_SD2XMLRPC_RESPONSE);
	PUMP_DEBUG;
	if(!eos)
	{
		return STATUS_BREAK;
	}
	PUMP_DEBUG;
	LLBufferStream stream(channels, buffer.get());
	stream << XML_HEADER << XMLRPC_METHOD_RESPONSE_HEADER << std::flush;
	LLSD sd;
	LLSDSerialize::fromNotation(sd, stream, buffer->count(channels.in()));
	PUMP_DEBUG;
	LLIOPipe::EStatus rv = STATUS_ERROR;
	if(sd.has("response"))
	{
		PUMP_DEBUG;
		stream.precision(DEFAULT_PRECISION);
		stream << XMLRPC_RESPONSE_HEADER;
		streamOut(stream, sd["response"]);
		stream << XMLRPC_RESPONSE_FOOTER << XMLRPC_METHOD_RESPONSE_FOOTER;
		rv = STATUS_DONE;
	}
	else if(sd.has("fault"))
	{
		PUMP_DEBUG;
		stream << XMLRPC_FAULT_1 << sd["fault"]["code"].asInteger()
			<< XMLRPC_FAULT_2
			<< xml_escape_string(sd["fault"]["description"].asString())
			<< XMLRPC_FAULT_3 << XMLRPC_METHOD_RESPONSE_FOOTER;
		rv = STATUS_DONE;
	}
	else
	{
		LL_WARNS() << "Unable to determine the type of LLSD response." << LL_ENDL;
	}
	PUMP_DEBUG;
	return rv;
}
LLFilterSD2XMLRPCRequest::LLFilterSD2XMLRPCRequest()
{
}
LLFilterSD2XMLRPCRequest::LLFilterSD2XMLRPCRequest(const char* method)
{
	if(method)
	{
		mMethod.assign(method);
	}
}
LLFilterSD2XMLRPCRequest::~LLFilterSD2XMLRPCRequest()
{
}
static LLTrace::BlockTimerStatHandle FTM_PROCESS_SD2XMLRPC_REQUEST("S22XMLRPC Request");
LLIOPipe::EStatus LLFilterSD2XMLRPCRequest::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LL_RECORD_BLOCK_TIME(FTM_PROCESS_SD2XMLRPC_REQUEST);
	PUMP_DEBUG;
	if(!eos)
	{
		LL_INFOS() << "!eos" << LL_ENDL;
		return STATUS_BREAK;
	}
	LLBufferStream stream(channels, buffer.get());
	LLSD sd;
	LLSDSerialize::fromNotation(sd, stream, buffer->count(channels.in()));
	if(stream.fail())
	{
		LL_INFOS() << "STREAM FAILURE reading structure data." << LL_ENDL;
	}
	PUMP_DEBUG;
	std::string method;
	LLSD param_sd;
	if(sd.has("method") && sd.has("parameter"))
	{
		method = sd["method"].asString();
		param_sd = sd["parameter"];
	}
	else
	{
		method = mMethod;
		param_sd = sd;
	}
	if(method.empty())
	{
		LL_WARNS() << "SD -> XML Request no method found." << LL_ENDL;
		return STATUS_ERROR;
	}
	PUMP_DEBUG;
	LLBufferStream ostream(channels, buffer.get());
	ostream.precision(DEFAULT_PRECISION);
	if(ostream.fail())
	{
		LL_INFOS() << "STREAM FAILURE setting precision" << LL_ENDL;
	}
	ostream << XML_HEADER << XMLRPC_REQUEST_HEADER_1
		<< xml_escape_string(method) << XMLRPC_REQUEST_HEADER_2;
	if(ostream.fail())
	{
		LL_INFOS() << "STREAM FAILURE writing method headers" << LL_ENDL;
	}
	switch(param_sd.type())
	{
	case LLSD::TypeMap:
		ostream << "<param>";
		streamOut(ostream, param_sd);
		ostream << "</param>";
		break;
	case LLSD::TypeArray:
	{
		LLSD::array_iterator it = param_sd.beginArray();
		LLSD::array_iterator end = param_sd.endArray();
		for(; it != end; ++it)
		{
			ostream << "<param>";
			streamOut(ostream, *it);
			ostream << "</param>";
		}
		break;
	}
	default:
		ostream << "<param>";
		streamOut(ostream, param_sd);
		ostream << "</param>";
		break;
	}
	stream << XMLRPC_REQUEST_FOOTER << std::flush;
	return STATUS_DONE;
}
LLIOPipe::EStatus stream_out(std::ostream& ostr, XMLRPC_VALUE value)
{
	XMLRPC_VALUE_TYPE_EASY type = XMLRPC_GetValueTypeEasy(value);
	LLIOPipe::EStatus status = LLIOPipe::STATUS_OK;
	switch(type)
	{
	case xmlrpc_type_base64:
	{
		S32 len = XMLRPC_GetValueStringLen(value);
		const char* buf = XMLRPC_GetValueBase64(value);
		ostr << " b(";
		if((len > 0) && buf)
		{
			ostr << len << ")\"";
			ostr.write(buf, len);
			ostr << "\"";
		}
		else
		{
			ostr << "0)\"\"";
		}
		break;
	}
	case xmlrpc_type_boolean:
		ostr << " " << (XMLRPC_GetValueBoolean(value) ? "true" : "false");
		break;
	case xmlrpc_type_datetime:
		ostr << " d\"" << XMLRPC_GetValueDateTime_ISO8601(value) << "\"";
		break;
	case xmlrpc_type_double:
		ostr << " r" << XMLRPC_GetValueDouble(value);
		break;
	case xmlrpc_type_int:
		ostr << " i" << XMLRPC_GetValueInt(value);
		break;
	case xmlrpc_type_string:
		ostr << " s(" << XMLRPC_GetValueStringLen(value) << ")'"
			<< XMLRPC_GetValueString(value) << "'";
		break;
	case xmlrpc_type_array:
	case xmlrpc_type_mixed:
	{
		ostr << " [";
		U32 needs_comma = 0;
		XMLRPC_VALUE current = XMLRPC_VectorRewind(value);
		while(current && (LLIOPipe::STATUS_OK == status))
		{
			if(needs_comma++) ostr << ",";
			status = stream_out(ostr, current);
			current = XMLRPC_VectorNext(value);
		}
		ostr << "]";
		break;
	}
	case xmlrpc_type_struct:
	{
		ostr << " {";
		std::string name;
		U32 needs_comma = 0;
		XMLRPC_VALUE current = XMLRPC_VectorRewind(value);
		while(current && (LLIOPipe::STATUS_OK == status))
		{
			if(needs_comma++) ostr << ",";
			name.assign(XMLRPC_GetValueID(current));
			ostr << "'" << LLSDNotationFormatter::escapeString(name) << "':";
			status = stream_out(ostr, current);
			current = XMLRPC_VectorNext(value);
		}
		ostr << "}";
		break;
	}
	case xmlrpc_type_empty:
	case xmlrpc_type_none:
	default:
		status = LLIOPipe::STATUS_ERROR;
		LL_WARNS() << "Found an empty xmlrpc type.." << LL_ENDL;
		break;
	};
	return status;
}
LLFilterXMLRPCResponse2LLSD::LLFilterXMLRPCResponse2LLSD()
{
}
LLFilterXMLRPCResponse2LLSD::~LLFilterXMLRPCResponse2LLSD()
{
}
static LLTrace::BlockTimerStatHandle FTM_PROCESS_XMLRPC2LLSD_RESPONSE("XMLRPC2LLSD Response");
LLIOPipe::EStatus LLFilterXMLRPCResponse2LLSD::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LL_RECORD_BLOCK_TIME(FTM_PROCESS_XMLRPC2LLSD_RESPONSE);
	PUMP_DEBUG;
	if(!eos) return STATUS_BREAK;
	if(!buffer) return STATUS_ERROR;
	PUMP_DEBUG;
	S32 bytes = buffer->countAfter(channels.in(), NULL);
	if(!bytes) return STATUS_ERROR;
	char* buf = new char[bytes + 1];
	buf[bytes] = '\0';
	buffer->readAfter(channels.in(), NULL, (U8*)buf, bytes);
	PUMP_DEBUG;
	XMLRPC_REQUEST response = XMLRPC_REQUEST_FromXML(
		buf,
		bytes,
		NULL);
	if(!response)
	{
		LL_WARNS() << "XML -> SD Response unable to parse xml." << LL_ENDL;
		delete[] buf;
		return STATUS_ERROR;
	}
	PUMP_DEBUG;
	LLBufferStream stream(channels, buffer.get());
	stream.precision(DEFAULT_PRECISION);
	if(XMLRPC_ResponseIsFault(response))
	{
		PUMP_DEBUG;
		stream << LLSDRPC_FAULT_HADER_1
			   << XMLRPC_GetResponseFaultCode(response)
			   << LLSDRPC_FAULT_HADER_2;
		const char* fault_str = XMLRPC_GetResponseFaultString(response);
		std::string fault_string;
		if(fault_str)
		{
			fault_string.assign(fault_str);
		}
		stream << "'" << LLSDNotationFormatter::escapeString(fault_string)
		   << "'" <<LLSDRPC_FAULT_FOOTER << std::flush;
	}
	else
	{
		PUMP_DEBUG;
		stream << LLSDRPC_RESPONSE_HEADER;
		XMLRPC_VALUE param = XMLRPC_RequestGetData(response);
		if(param)
		{
			stream_out(stream, param);
		}
		stream << LLSDRPC_RESPONSE_FOOTER << std::flush;
	}
	PUMP_DEBUG;
	XMLRPC_RequestFree(response, 1);
	delete[] buf;
	PUMP_DEBUG;
	return STATUS_DONE;
}
LLFilterXMLRPCRequest2LLSD::LLFilterXMLRPCRequest2LLSD()
{
}
LLFilterXMLRPCRequest2LLSD::~LLFilterXMLRPCRequest2LLSD()
{
}
static LLTrace::BlockTimerStatHandle FTM_PROCESS_XMLRPC2LLSD_REQUEST("XMLRPC2LLSD Request");
LLIOPipe::EStatus LLFilterXMLRPCRequest2LLSD::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LL_RECORD_BLOCK_TIME(FTM_PROCESS_XMLRPC2LLSD_REQUEST);
	PUMP_DEBUG;
	if(!eos) return STATUS_BREAK;
	if(!buffer) return STATUS_ERROR;
	PUMP_DEBUG;
	S32 bytes = buffer->countAfter(channels.in(), NULL);
	if(!bytes) return STATUS_ERROR;
	char* buf = new char[bytes + 1];
	buf[bytes] = '\0';
	buffer->readAfter(channels.in(), NULL, (U8*)buf, bytes);
	U8* cur_pBuf = (U8*)buf;
    U8 cur_char;
	for (S32 i=0; i<bytes; i++)
	{
        cur_char = *cur_pBuf;
		if (   cur_char < 0x20
            && 0x09 != cur_char
            && 0x0a != cur_char
            && 0x0d != cur_char )
        {
			*cur_pBuf = '?';
        }
		++cur_pBuf;
	}
	PUMP_DEBUG;
	XMLRPC_REQUEST request = XMLRPC_REQUEST_FromXML(
		buf,
		bytes,
		NULL);
	if(!request)
	{
		LL_WARNS() << "XML -> SD Request process parse error." << LL_ENDL;
		delete[] buf;
		return STATUS_ERROR;
	}
	PUMP_DEBUG;
	LLBufferStream stream(channels, buffer.get());
	stream.precision(DEFAULT_PRECISION);
	const char* name = XMLRPC_RequestGetMethodName(request);
	stream << LLSDRPC_REQUEST_HEADER_1 << (name ? name : "")
		   << LLSDRPC_REQUEST_HEADER_2;
	XMLRPC_VALUE param = XMLRPC_RequestGetData(request);
	if(param)
	{
		PUMP_DEBUG;
		S32 size = XMLRPC_VectorSize(param);
		if(size > 1)
		{
			stream << "[";
		}
 		XMLRPC_VALUE current = XMLRPC_VectorRewind(param);
		bool needs_comma = false;
 		while(current)
 		{
			if(needs_comma)
			{
				stream << ",";
			}
			needs_comma = true;
 			stream_out(stream, current);
 			current = XMLRPC_VectorNext(param);
 		}
		if(size > 1)
		{
			stream << "]";
		}
	}
	stream << LLSDRPC_REQUEST_FOOTER << std::flush;
	XMLRPC_RequestFree(request, 1);
	delete[] buf;
	PUMP_DEBUG;
	return STATUS_DONE;
}
