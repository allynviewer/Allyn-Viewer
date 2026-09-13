/** 
 * @file llares.h
 * @author Bryan O'Sullivan
 * @date 2007-08-15
 * @brief Wrapper for asynchronous DNS lookups.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#ifndef LL_LLARES_H
#define LL_LLARES_H
#ifdef LL_WINDOWS
# pragma warning(push)
# pragma warning(disable:4996)
# include <winsock2.h>
# include <ws2tcpip.h>
# pragma warning(pop)
#endif
#include <ares.h>
#include "llpointer.h"
#include "llrefcount.h"
#include "lluri.h"
#include <boost/shared_ptr.hpp>
class LLQueryResponder;
class LLAresListener;
enum LLResType
{
	RES_INVALID = 0,
	RES_A = 1,
	RES_NS = 2,
	RES_CNAME = 5,
	RES_PTR = 12,
	RES_AAAA = 28,
	RES_SRV = 33,
	RES_MAX = 65536
};
class LLDnsRecord : public LLRefCount
{
protected:
	friend class LLQueryResponder;
	LLResType mType;
	std::string mName;
	unsigned mTTL;
	virtual int parse(const char *buf, size_t len, const char *pos,
					  size_t rrlen) = 0;
	LLDnsRecord(LLResType type, const std::string &name, unsigned ttl);
public:
	const std::string &name() const { return mName; }
	unsigned ttl() const { return mTTL; }
	LLResType type() const { return mType; }
};
class LLAddrRecord : public LLDnsRecord
{
protected:
	friend class LLQueryResponder;
	LLAddrRecord(LLResType type, const std::string &name, unsigned ttl);
	union
	{
		sockaddr sa;
		sockaddr_in sin;
		sockaddr_in6 sin6;
	} mSA;
	socklen_t mSize;
public:
	const sockaddr &addr() const { return mSA.sa; }
	socklen_t size() const { return mSize; }
};
class LLARecord : public LLAddrRecord
{
protected:
	friend class LLQueryResponder;
	LLARecord(const std::string &name, unsigned ttl);
	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);
public:
	const sockaddr_in &addr_in() const { return mSA.sin; }
};
class LLAaaaRecord : public LLAddrRecord
{
protected:
	friend class LLQueryResponder;
	LLAaaaRecord(const std::string &name, unsigned ttl);
	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);
public:
	const sockaddr_in6 &addr_in6() const { return mSA.sin6; }
};
class LLHostRecord : public LLDnsRecord
{
protected:
	LLHostRecord(LLResType type, const std::string &name, unsigned ttl);
	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);
	std::string mHost;
public:
	const std::string &host() const { return mHost; }
};
class LLCnameRecord : public LLHostRecord
{
protected:
	friend class LLQueryResponder;
	LLCnameRecord(const std::string &name, unsigned ttl);
};
class LLPtrRecord : public LLHostRecord
{
protected:
	friend class LLQueryResponder;
	LLPtrRecord(const std::string &name, unsigned ttl);
};
class LLSrvRecord : public LLHostRecord
{
protected:
	U16 mPriority;
	U16 mWeight;
	U16 mPort;
	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);
public:
	LLSrvRecord(const std::string &name, unsigned ttl);
	U16 priority() const { return mPriority; }
	U16 weight() const { return mWeight; }
	U16 port() const { return mPort; }
	struct ComparePriorityLowest
	{
		bool operator()(const LLSrvRecord& lhs, const LLSrvRecord& rhs)
		{
			return lhs.mPriority < rhs.mPriority;
		}
	};
};
class LLNsRecord : public LLHostRecord
{
public:
	LLNsRecord(const std::string &name, unsigned ttl);
};
class LLQueryResponder;
class LLAres
{
public:
	class HostResponder : public LLRefCount
	{
	public:
		virtual ~HostResponder();
		virtual void hostResult(const hostent *ent);
		virtual void hostError(int code);
	};
	class NameInfoResponder : public LLRefCount
	{
	public:
		virtual ~NameInfoResponder();
		virtual void nameInfoResult(const char *node, const char *service);
		virtual void nameInfoError(int code);
	};
	class QueryResponder : public LLRefCount
	{
	public:
		virtual ~QueryResponder();
		virtual void queryResult(const char *buf, size_t len);
		virtual void queryError(int code);
	};
	class SrvResponder;
	class UriRewriteResponder;
	LLAres();
	~LLAres();
	void cancel();
	void getHostByName(const std::string &name, HostResponder *resp,
					   int family = AF_INET) {
		getHostByName(name.c_str(), resp, family);
	}
	void getHostByName(const char *name, HostResponder *resp,
					   int family = PF_INET);
	void getNameInfo(const struct sockaddr &sa, socklen_t salen, int flags,
					 NameInfoResponder *resp);
	void getSrvRecords(const std::string &name, SrvResponder *resp);
	void rewriteURI(const std::string &uri,
					UriRewriteResponder *resp);
	void search(const std::string &query, LLResType type,
				QueryResponder *resp);
	bool process(U64 timeoutUsecs = 0);
	bool processAll();
	static int expandName(const char *encoded, const char *abuf, size_t alen,
						  std::string &s) {
		size_t ignore;
		return expandName(encoded, abuf, alen, s, ignore);
	}
	static int expandName(const char *encoded, const char *abuf, size_t alen,
						  std::string &s, size_t &enclen);
	static const char *strerror(int code);
	bool isInitialized(void) { return mInitSuccess; }
protected:
	ares_channel chan_;
	bool mInitSuccess;
    boost::shared_ptr<LLAresListener> mListener;
};
typedef std::vector<LLPointer<LLDnsRecord> > dns_rrs_t;
class LLQueryResponder : public LLAres::QueryResponder
{
protected:
	int mResult;
	std::string mQuery;
	LLResType mType;
	dns_rrs_t mAnswers;
	dns_rrs_t mAuthorities;
	dns_rrs_t mAdditional;
	int parseRR(const char *buf, size_t len, const char *&pos,
				LLPointer<LLDnsRecord> &r);
	int parseSection(const char *buf, size_t len,
					 size_t count, const char *& pos, dns_rrs_t &rrs);
	void queryResult(const char *buf, size_t len);
	virtual void querySuccess();
public:
	LLQueryResponder();
	bool valid() const { return mResult == ARES_SUCCESS; }
	int result() const { return mResult; }
	const std::string &query() const { return mQuery; }
	const dns_rrs_t &answers() const { return mAnswers; }
	const dns_rrs_t &authorities() const { return mAuthorities; }
	const dns_rrs_t &additional() const { return mAdditional; }
};
class LLAres::SrvResponder : public LLQueryResponder
{
public:
	friend void LLAres::getSrvRecords(const std::string &name,
									  SrvResponder *resp);
	void querySuccess();
	void queryError(int code);
	virtual void srvResult(const dns_rrs_t &ents);
	virtual void srvError(int code);
};
class LLAres::UriRewriteResponder : public LLQueryResponder
{
protected:
	LLURI mUri;
public:
	friend void LLAres::rewriteURI(const std::string &uri,
								   UriRewriteResponder *resp);
	void querySuccess();
	void queryError(int code);
	virtual void rewriteResult(const std::vector<std::string> &uris);
};
extern LLAres *gAres;
extern LLAres *ll_init_ares();
#endif
