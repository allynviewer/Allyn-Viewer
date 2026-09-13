/** 
 * @file lluserauth.h
 * @brief LLUserAuth class header file
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
#ifndef LLUSERAUTH_H
#define LLUSERAUTH_H
#include <string>
#include <vector>
#include <map>
#include <boost/intrusive_ptr.hpp>
class XMLRPCResponder;
typedef struct _xmlrpc_value* XMLRPC_VALUE;
class LLUserAuth : public LLSingleton<LLUserAuth>
{
public:
	LLUserAuth();
	~LLUserAuth();
	typedef enum {
		E_NO_RESPONSE_YET = -2,
		E_DOWNLOADING = -1,
		E_OK = 0,
		E_COULDNT_RESOLVE_HOST,
		E_SSL_PEER_CERTIFICATE,
		E_SSL_CACERT,
		E_SSL_CONNECT_ERROR,
		E_HTTP_SERVER_ERROR,
		E_UNHANDLED_ERROR,
		E_LAST
	} UserAuthcode;
	void authenticate(
		const std::string& auth_uri,
		const std::string& auth_method,
		const std::string& firstname,
		const std::string& lastname,
		LLUUID web_login_key,
		const std::string& start,
		BOOL skip_optional_update,
		BOOL accept_tos,
		BOOL accept_critical_message,
		BOOL last_exec_froze,
		const std::vector<const char*>& requested_options,
		const std::string& hashed_mac,
		const std::string& hashed_volume_serial);
	void authenticate(
		const std::string& auth_uri,
		const std::string& auth_method,
		const std::string& firstname,
		const std::string& lastname,
		const std::string& password,
		const std::string& start,
		BOOL skip_optional_update,
		BOOL accept_tos,
		BOOL accept_critical_message,
		BOOL last_exec_froze,
		const std::vector<const char*>& requested_options,
		const std::string& hashed_mac,
		const std::string& hashed_volume_serial);
	UserAuthcode authResponse();
	LLSD mResult;
	UserAuthcode mAuthResponse;
	void reset();
	std::string errorMessage() const { return mErrorMessage; }
	LLSD getResponse() const { return mResponses; }
	LLSD getResponse(const std::string& entry) const { return mResponses[entry]; }
	F64 getLastTransferRateBPS() const { return mLastTransferRateBPS; }
private:
	boost::intrusive_ptr<XMLRPCResponder> mResponder;
	std::string mErrorMessage;
	LLSD mResponses;
	UserAuthcode parseResponse();
	LLSD parseValues(UserAuthcode &auth_code, const std::string& key_pfx, XMLRPC_VALUE param);
	F64 mLastTransferRateBPS;
};
#endif
