/** 
 * @file lltexturestatsuploader_test.cpp
 * @author Si
 * @date 2009-05-27
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
#include "../lltexturestatsuploader.h"
#include "../test/lltut.h"
#include "boost/intrusive_ptr.hpp"
void boost::intrusive_ptr_add_ref(LLCurl::Responder*){}
void boost::intrusive_ptr_release(LLCurl::Responder* p){}
const F32 HTTP_REQUEST_EXPIRY_SECS = 0.0f;
static std::string most_recent_url;
static LLSD most_recent_body;
void LLHTTPClient::post(
		const std::string& url,
		const LLSD& body,
		ResponderPtr,
		const LLSD& headers,
		const F32 timeout)
{
	most_recent_url = url;
	most_recent_body = body;
	return;
}
namespace tut
{
	struct texturestatsuploader_test
	{
		texturestatsuploader_test()
		{
			most_recent_url = "some sort of default text that should never match anything the tests are expecting!";
			LLSD blank_llsd;
			most_recent_body = blank_llsd;
		}
		~texturestatsuploader_test()
		{
		}
	};
	typedef test_group<texturestatsuploader_test> texturestatsuploader_t;
	typedef texturestatsuploader_t::object texturestatsuploader_object_t;
	tut::texturestatsuploader_t tut_texturestatsuploader("texturestatsuploader");
	template<> template<>
	void texturestatsuploader_object_t::test<1>()
	{
		LLTextureStatsUploader tsu;
		LL_INFOS() << &tsu << LL_ENDL;
		ensure("have we crashed?", true);
	}
	template<> template<>
	void texturestatsuploader_object_t::test<2>()
	{
		LLTextureStatsUploader tsu;
		std::string url = "http://blahblahblah";
		LLSD texture_stats;
		tsu.uploadStatsToSimulator(url, texture_stats);
		ensure_equals("did the right url get called?", most_recent_url, url);
		ensure_equals("did the right body get sent?", most_recent_body, texture_stats);
	}
	template<> template<>
	void texturestatsuploader_object_t::test<3>()
	{
		LLTextureStatsUploader tsu;
		std::string url_for_ungranted_cap = "";
		LLSD texture_stats;
		std::string most_recent_url_before_test = most_recent_url;
		tsu.uploadStatsToSimulator(url_for_ungranted_cap, texture_stats);
		ensure_equals("hopefully no url got called!", most_recent_url, most_recent_url_before_test);
	}
}
