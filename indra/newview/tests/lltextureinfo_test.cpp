/** 
 * @file llwtextureinfo_test.cpp
 * @author Si & Gabriel
 * @date 2009-03-30
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "../llviewerprecompiledheaders.h"
#include "../lltextureinfo.h"
#include "../lltextureinfodetails.cpp"
#include "../test/lltut.h"
void send_texture_stats_to_sim(const LLSD &data)
{
}
namespace tut
{
	struct textureinfo_test
	{
		textureinfo_test()
		{
		}
		~textureinfo_test()
		{
		}
	};
	typedef test_group<textureinfo_test> textureinfo_t;
	typedef textureinfo_t::object textureinfo_object_t;
	tut::textureinfo_t tut_textureinfo("textureinfo");
	const U32 upload_byte_threshold = 100 * 1024;
	template<> template<>
	void textureinfo_object_t::test<1>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		ensure("have we crashed?", true);
	}
	template<> template<>
	void textureinfo_object_t::test<2>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
		ensure(!tex_info.has(nonExistant));
	}
	template<> template<>
	void textureinfo_object_t::test<3>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestStartTime(id, 200);
		ensure_equals(tex_info.getRequestStartTime(id), 200);
	}
	template<> template<>
	void textureinfo_object_t::test<4>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
		ensure_equals(tex_info.getRequestStartTime(nonExistant), 0);
	}
	template<> template<>
	void textureinfo_object_t::test<5>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
		ensure_equals(tex_info.getRequestCompleteTime(nonExistant), 0);
	}
	template<> template<>
	void textureinfo_object_t::test<6>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestSize(id, 600);
		ensure_equals(tex_info.getRequestSize(id), 600);
	}
	template<> template<>
	void textureinfo_object_t::test<7>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestType(id, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
		ensure_equals(tex_info.getRequestType(id), LLTextureInfoDetails::REQUEST_TYPE_HTTP);
	}
	template<> template<>
	void textureinfo_object_t::test<8>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestType(id, LLTextureInfoDetails::REQUEST_TYPE_UDP);
		ensure_equals(tex_info.getRequestType(id), LLTextureInfoDetails::REQUEST_TYPE_UDP);
	}
	template<> template<>
	void textureinfo_object_t::test<9>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestOffset(id, 1234);
		ensure_equals(tex_info.getRequestOffset(id), 1234);
	}
	template<> template<>
	void textureinfo_object_t::test<10>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		S32 requestStartTimeOne = 200;
		S32 requestEndTimeOne = 400;
		S32 requestSizeOne = 1024;
		S32 requestSizeOneBits = requestSizeOne * 8;
		LLUUID id1("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestStartTime(id1, requestStartTimeOne);
		tex_info.setRequestSize(id1, requestSizeOne);
		tex_info.setRequestType(id1, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
		tex_info.setRequestCompleteTimeAndLog(id1, requestEndTimeOne);
		U32 requestStartTimeTwo = 100;
		U32 requestEndTimeTwo = 500;
		U32 requestSizeTwo = 2048;
		S32 requestSizeTwoBits = requestSizeTwo * 8;
		LLUUID id2("10e65d70-46fd-429f-841a-bf698e9424d4");
		tex_info.setRequestStartTime(id2, requestStartTimeTwo);
		tex_info.setRequestSize(id2, requestSizeTwo);
		tex_info.setRequestType(id2, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
		tex_info.setRequestCompleteTimeAndLog(id2, requestEndTimeTwo);
		S32 averageBitRate = ((requestSizeOneBits/(requestEndTimeOne - requestStartTimeOne)) +
							(requestSizeTwoBits/(requestEndTimeTwo - requestStartTimeTwo))) / 2;
		S32 totalBytes = requestSizeOne + requestSizeTwo;
		LLSD results = tex_info.getAverages();
		ensure_equals("is average bits per second correct", results["bits_per_second"].asInteger(), averageBitRate);
		ensure_equals("is total bytes is correct", results["bytes_downloaded"].asInteger(), totalBytes);
		ensure_equals("is transport correct", results["transport"].asString(), std::string("HTTP"));
	}
	template<> template<>
	void textureinfo_object_t::test<11>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		S32 requestStartTimeOne = 200;
		S32 requestEndTimeOne = 400;
		S32 requestSizeOne = 1024;
		LLUUID id1("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestStartTime(id1, requestStartTimeOne);
		tex_info.setRequestSize(id1, requestSizeOne);
		tex_info.setRequestType(id1, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
		tex_info.setRequestCompleteTimeAndLog(id1, requestEndTimeOne);
		tex_info.resetTextureStatistics();
		LLSD results = tex_info.getAverages();
		ensure_equals("is average bits per second correct", results["bits_per_second"].asInteger(), 0);
		ensure_equals("is total bytes is correct", results["bytes_downloaded"].asInteger(), 0);
		ensure_equals("is transport correct", results["transport"].asString(), std::string("NONE"));
	}
	template<> template<>
	void textureinfo_object_t::test<12>()
	{
		LLTextureInfo tex_info;
		tex_info.setUpLogging(true, true, upload_byte_threshold);
		S32 requestStartTimeOne = 200;
		S32 requestEndTimeOne = 400;
		S32 requestSizeOne = 1024;
		LLUUID id1("10e65d70-46fd-429f-841a-bf698e9424d3");
		tex_info.setRequestStartTime(id1, requestStartTimeOne);
		tex_info.setRequestSize(id1, requestSizeOne);
		tex_info.setRequestType(id1, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
		ensure_equals("map item created", tex_info.getTextureInfoMapSize(), 1);
		tex_info.setRequestCompleteTimeAndLog(id1, requestEndTimeOne);
		ensure_equals("map item removed when consumed", tex_info.getTextureInfoMapSize(), 0);
	}
}
