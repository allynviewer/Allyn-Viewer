/**
 * @file llproxy.h
 * @brief UDP and HTTP proxy communications
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
#ifndef LL_PROXY_H
#define LL_PROXY_H
#include "llcurl.h"
#include "llhost.h"
#include "lliosocket.h"
#include "llmemory.h"
#include "llsingleton.h"
#include "llthread.h"
#include "aithreadsafe.h"
#include <string>
#define SOCKS_OK 0
#define SOCKS_CONNECT_ERROR (-1)
#define SOCKS_NOT_PERMITTED (-2)
#define SOCKS_NOT_ACCEPTABLE (-3)
#define SOCKS_AUTH_FAIL (-4)
#define SOCKS_UDP_FWD_NOT_GRANTED (-5)
#define SOCKS_HOST_CONNECT_FAILED (-6)
#define SOCKS_INVALID_HOST (-7)
#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN (255 + 1)
#endif
#define SOCKSMAXUSERNAMELEN 255
#define SOCKSMAXPASSWORDLEN 255
#define SOCKSMINUSERNAMELEN 1
#define SOCKSMINPASSWORDLEN 1
#define SOCKS_VERSION 0x05
#define SOCKS_HEADER_SIZE 10
#define ADDRESS_IPV4     0x01
#define ADDRESS_HOSTNAME 0x03
#define ADDRESS_IPV6     0x04
union ipv4_address_t {
	U8		octets[4];
	U32		addr32;
};
#define COMMAND_TCP_STREAM    0x01
#define COMMAND_TCP_BIND      0x02
#define COMMAND_UDP_ASSOCIATE 0x03
#define REPLY_REQUEST_GRANTED     0x00
#define REPLY_GENERAL_FAIL        0x01
#define REPLY_RULESET_FAIL        0x02
#define REPLY_NETWORK_UNREACHABLE 0x03
#define REPLY_HOST_UNREACHABLE    0x04
#define REPLY_CONNECTION_REFUSED  0x05
#define REPLY_TTL_EXPIRED         0x06
#define REPLY_PROTOCOL_ERROR      0x07
#define REPLY_TYPE_NOT_SUPPORTED  0x08
#define FIELD_RESERVED 0x00
#pragma pack(push,1)
struct socks_command_request_t {
	U8		version;
	U8		command;
	U8		reserved;
	U8		atype;
	U32		address;
	U16		port;
};
struct socks_command_response_t {
	U8		version;
	U8		reply;
	U8		reserved;
	U8		atype;
	U8		add_bytes[4];
	U16		port;
};
#define AUTH_NOT_ACCEPTABLE 0xFF
#define AUTH_SUCCESS        0x00
struct socks_auth_request_t {
	U8		version;
	U8		num_methods;
	U8		methods;
};
struct socks_auth_response_t {
	U8		version;
	U8		method;
};
struct authmethod_password_reply_t {
	U8		version;
	U8		status;
};
struct proxywrap_t {
	U16		rsv;
	U8		frag;
	U8		atype;
	U32		addr;
	U16		port;
};
#pragma pack(pop)
enum LLHttpProxyType
{
	LLPROXY_SOCKS = 0,
	LLPROXY_HTTP  = 1
};
enum LLSocks5AuthType
{
	METHOD_NOAUTH   = 0x00,
	METHOD_GSSAPI   = 0x01,
	METHOD_PASSWORD = 0x02
};
struct ProxyUnshared
{
	LLHost mUDPProxy;
	LLHost mTCPProxy;
	LLSocket::ptr_t mProxyControlChannel;
};
struct ProxyShared
{
	ProxyShared(void);
	LLHost mHTTPProxy;
	LLHttpProxyType mProxyType;
	LLSocks5AuthType mAuthMethodSelected;
	std::string mSocksUsername;
	std::string mSocksPassword;
};
class LLProxy: public LLSingleton<LLProxy>
{
	LOG_CLASS(LLProxy);
public:
	typedef AISTAccessConst<ProxyUnshared> Unshared_crat;
	typedef AISTAccess<ProxyUnshared> Unshared_rat;
	typedef AISTAccess<ProxyUnshared> Unshared_wat;
	typedef AIReadAccessConst<ProxyShared> Shared_crat;
	typedef AIReadAccess<ProxyShared> Shared_rat;
	typedef AIWriteAccess<ProxyShared> Shared_wat;
	LLProxy();
	static bool isSOCKSProxyEnabled(void) { llassert(is_main_thread()); return sUDPProxyEnabled; }
	LLHost getUDPProxy(void) const { return Unshared_crat(mUnshared)->mUDPProxy; }
	bool HTTPProxyEnabled(void) const { return mHTTPProxyEnabled; }
	~LLProxy();
	static void cleanupClass();
	void applyProxySettings(AICurlEasyRequest_wat const& curlEasyRequest_w);
	S32 startSOCKSProxy(LLHost host);
	void stopSOCKSProxy();
	bool setAuthPassword(const std::string &username, const std::string &password);
	void setAuthNone();
	bool enableHTTPProxy(LLHost httpHost, LLHttpProxyType type);
	bool enableHTTPProxy();
	void disableHTTPProxy(Shared_wat const& shared_w) { mHTTPProxyEnabled = false; }
	void disableHTTPProxy(void) { disableHTTPProxy(Shared_wat(mShared)); }
	LLHost const& getHTTPProxy(Shared_crat const& shared_r) const { return shared_r->mHTTPProxy; }
	LLHttpProxyType getHTTPProxyType(Shared_crat const& shared_r) const { return shared_r->mProxyType; }
	LLSocks5AuthType getSelectedAuthMethod(Shared_crat const& shared_r) const { return shared_r->mAuthMethodSelected; }
	std::string getSocksUser(Shared_crat const& shared_r) const { return shared_r->mSocksUsername; }
	std::string getSocksPwd(Shared_crat const& shared_r) const { return shared_r->mSocksPassword; }
private:
	S32 proxyHandshake(LLHost proxy);
private:
	mutable LLAtomic32<bool> mHTTPProxyEnabled;
	static bool sUDPProxyEnabled;
	AIThreadSafeSingleThreadDC<ProxyUnshared> mUnshared;
	AIThreadSafeDC<ProxyShared> mShared;
public:
	AIThreadSafeSingleThreadDC<ProxyUnshared> const& unshared_lockobj(void) const { return mUnshared; }
	AIThreadSafeDC<ProxyShared> const& shared_lockobj(void) const { return mShared; }
};
#endif
