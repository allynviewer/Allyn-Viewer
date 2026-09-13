/** 
 * @file llwebprofile.cpp
 * @brief Web profile access.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "llwebprofile.h"
#include "llbufferstream.h"
#include "llhttpclient.h"
#include "llimagepng.h"
#include "llpanelprofile.h"
#include "llviewermedia.h"
#include "llsdjson.h"
extern AIHTTPTimeoutPolicy webProfileResponders_timeout;
class LLWebProfileResponders::ConfigResponder : public LLHTTPClient::ResponderWithCompleted
{
	LOG_CLASS(LLWebProfileResponders::ConfigResponder);
public:
	ConfigResponder(LLPointer<LLImageFormatted> imagep)
	:	mImagep(imagep)
	{
	}
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
	{
		LLBufferStream istr(channels, buffer.get());
		std::stringstream strstrm;
		strstrm << istr.rdbuf();
		const std::string body = strstrm.str();
		if (mStatus != HTTP_OK)
		{
			LL_WARNS() << "Failed to get upload config (" << mStatus << ')' << LL_ENDL;
			LLWebProfile::reportImageUploadStatus(false);
			return;
		}
		auto root = LlsdFromJsonString(body);
		if (root.isUndefined())
		{
			LL_WARNS() << "Failed to get valid json body" << LL_ENDL;
			LLWebProfile::reportImageUploadStatus(false);
			return;
		}
		const auto data = root["data"];
		const std::string upload_url = root["url"].asString();
		LLSD config = data;
		if (!data.has("add_loc")) config["add_loc"] = "0";
		if (!data.has("caption")) config["caption"] = LLStringUtil::null;
		LL_DEBUGS("Snapshots") << "Got upload config, POSTing image to " << upload_url << ", config=[" << config << ']' << LL_ENDL;
		LLWebProfile::post(mImagep, config, upload_url);
	}
protected:
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return webProfileResponders_timeout; }
	char const* getName(void) const { return "LLWebProfileResponders::ConfigResponder"; }
private:
	LLPointer<LLImageFormatted> mImagep;
};
class LLWebProfileResponders::PostImageRedirectResponder : public LLHTTPClient::ResponderWithCompleted
{
	LOG_CLASS(LLWebProfileResponders::PostImageRedirectResponder);
public:
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
	{
		if (mStatus != HTTP_OK)
		{
			LL_WARNS() << "Failed to upload image: " << mStatus << ' ' << mReason << LL_ENDL;
			LLWebProfile::reportImageUploadStatus(false);
			return;
		}
		LLBufferStream istr(channels, buffer.get());
		std::stringstream strstrm;
		strstrm << istr.rdbuf();
		const std::string body = strstrm.str();
		LL_INFOS() << "Image uploaded." << LL_ENDL;
		LL_DEBUGS("Snapshots") << "Uploading image succeeded. Response: [" << body << ']' << LL_ENDL;
		LLWebProfile::reportImageUploadStatus(true);
	}
protected:
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return webProfileResponders_timeout; }
	char const* getName(void) const { return "LLWebProfileResponders::PostImageRedirectResponder"; }
private:
	LLPointer<LLImageFormatted> mImagep;
};
class LLWebProfileResponders::PostImageResponder : public LLHTTPClient::ResponderWithCompleted
{
	LOG_CLASS(LLWebProfileResponders::PostImageResponder);
public:
	bool needsHeaders(void) const { return true; }
	void completedHeaders(void)
	{
		if (mStatus == HTTP_SEE_OTHER)
		{
			AIHTTPHeaders headers;
			headers.addHeader("Accept", "*/*");
			headers.addHeader("Cookie", LLWebProfile::getAuthCookie());
			headers.addHeader("User-Agent", LLViewerMedia::getCurrentUserAgent());
			std::string redir_url;
			mReceivedHeaders.getFirstValue("location", redir_url);
			LL_DEBUGS("Snapshots") << "Got redirection URL: " << redir_url << LL_ENDL;
			LLHTTPClient::get(redir_url, new LLWebProfileResponders::PostImageRedirectResponder, headers);
		}
		else
		{
			LL_WARNS() << "Unexpected POST status: " << mStatus << ' ' << mReason << LL_ENDL;
			LL_DEBUGS("Snapshots") << "received_headers: [" << mReceivedHeaders << ']' << LL_ENDL;
			LLWebProfile::reportImageUploadStatus(false);
		}
	}
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
	{
	}
protected:
	bool pass_redirect_status(void) const { return true; }
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return webProfileResponders_timeout; }
	char const* getName(void) const { return "LLWebProfileResponders::PostImageResponder"; }
};
std::string LLWebProfile::sAuthCookie;
LLWebProfile::status_callback_t LLWebProfile::mStatusCallback;
void LLWebProfile::uploadImage(LLPointer<LLImageFormatted> image, const std::string& caption, bool add_location)
{
	std::string config_url(getProfileURL(LLStringUtil::null) + "snapshots/s3_upload_config");
	config_url += "?caption=" + LLURI::escape(caption);
	config_url += "&add_loc=" + std::string(add_location ? "1" : "0");
	LL_DEBUGS("Snapshots") << "Requesting " << config_url << LL_ENDL;
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "*/*");
	headers.addHeader("Cookie", LLWebProfile::getAuthCookie());
	headers.addHeader("User-Agent", LLViewerMedia::getCurrentUserAgent());
	LLHTTPClient::get(config_url, new LLWebProfileResponders::ConfigResponder(image), headers);
}
void LLWebProfile::setAuthCookie(const std::string& cookie)
{
	LL_DEBUGS("Snapshots") << "Setting auth cookie: " << cookie << LL_ENDL;
	sAuthCookie = cookie;
}
void LLWebProfile::post(LLPointer<LLImageFormatted> image, const LLSD& config, const std::string& url)
{
	if (dynamic_cast<LLImagePNG*>(image.get()) == 0)
	{
		LL_WARNS() << "Image to upload is not a PNG" << LL_ENDL;
		llassert(dynamic_cast<LLImagePNG*>(image.get()) != 0);
		return;
	}
	const std::string boundary = "----------------------------0123abcdefab";
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "*/*");
	headers.addHeader("Cookie", LLWebProfile::getAuthCookie());
	headers.addHeader("User-Agent", LLViewerMedia::getCurrentUserAgent());
	headers.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
	std::ostringstream body;
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"key\"\r\n\r\n"
			<< config["key"].asString() << "\r\n";
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"AWSAccessKeyId\"\r\n\r\n"
			<< config["AWSAccessKeyId"].asString() << "\r\n";
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"acl\"\r\n\r\n"
			<< config["acl"].asString() << "\r\n";
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"Content-Type\"\r\n\r\n"
			<< config["Content-Type"].asString() << "\r\n";
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"policy\"\r\n\r\n"
			<< config["policy"].asString() << "\r\n";
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"signature\"\r\n\r\n"
			<< config["signature"].asString() << "\r\n";
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"success_action_redirect\"\r\n\r\n"
			<< config["success_action_redirect"].asString() << "\r\n";
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"file\"; filename=\"snapshot.png\"\r\n"
			<< "Content-Type: image/png\r\n\r\n";
	size_t const body_size = body.str().size();
	std::ostringstream footer;
	footer << "\r\n--" << boundary << "--\r\n";
	size_t const footer_size = footer.str().size();
	size_t size = body_size + image->getDataSize() + footer_size;
	U8* data = new U8 [size];
	memcpy(data, body.str().data(), body_size);
	memcpy(data + body_size, image->getData(), image->getDataSize());
	memcpy(data + body_size + image->getDataSize(), footer.str().data(), footer_size);
	LLHTTPClient::postRaw(url, data, size, new LLWebProfileResponders::PostImageResponder(), headers DEBUG_CURLIO_PARAM(debug_off), no_keep_alive);
}
void LLWebProfile::reportImageUploadStatus(bool ok)
{
	if (mStatusCallback)
	{
		mStatusCallback(ok);
	}
}
std::string LLWebProfile::getAuthCookie()
{
	const char* debug_cookie = getenv("LL_SNAPSHOT_COOKIE");
	return debug_cookie ? debug_cookie : sAuthCookie;
}
