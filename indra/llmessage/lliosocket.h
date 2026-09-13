/** 
 * @file lliosocket.h
 * @author Phoenix
 * @date 2005-07-31
 * @brief Declaration of files used for handling sockets and associated pipes
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
#ifndef LL_LLIOSOCKET_H
#define LL_LLIOSOCKET_H
#include "llaprpool.h"
#include "lliopipe.h"
#include "apr_network_io.h"
#include "llchainio.h"
class LLHost;
class LLSocket
{
public:
	typedef boost::shared_ptr<LLSocket> ptr_t;
	enum EType
	{
		STREAM_TCP,
		DATAGRAM_UDP,
	};
	enum
	{
		PORT_INVALID = (U16)-1,
		PORT_EPHEMERAL = 0,
	};
	static ptr_t create(
		EType type,
		U16 port = PORT_EPHEMERAL);
	static ptr_t create(apr_status_t& status, ptr_t& listen_socket);
	bool blockingConnect(const LLHost& host);
	U16 getPort() const { return mPort; }
	apr_socket_t* getSocket() const { return mSocket; }
	void setBlocking(S32 timeout);
	void setNonBlocking();
protected:
	LLSocket(void);
public:
	~LLSocket();
protected:
	apr_socket_t* mSocket;
	LLAPRPool mPool;
	U16 mPort;
};
class LLIOSocketReader : public LLIOPipe
{
public:
	LLIOSocketReader(LLSocket::ptr_t socket);
	~LLIOSocketReader();
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
	LLSocket::ptr_t mSource;
	std::vector<U8> mBuffer;
	bool mInitialized;
};
class LLIOSocketWriter : public LLIOPipe
{
public:
	LLIOSocketWriter(LLSocket::ptr_t socket);
	~LLIOSocketWriter();
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
	LLSocket::ptr_t mDestination;
	U8* mLastWritten;
	bool mInitialized;
};
class LLIOServerSocket : public LLIOPipe
{
public:
	typedef LLSocket::ptr_t socket_t;
	typedef boost::shared_ptr<LLChainIOFactory> factory_t;
	LLIOServerSocket(socket_t listener, factory_t reactor);
	virtual ~LLIOServerSocket();
	void setResponseTimeout(F32 timeout_secs);
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
	socket_t mListenSocket;
	factory_t mReactor;
	bool mInitialized;
	F32 mResponseTimeout;
};
#if 0
class LLIODataSocket : public LLIOSocket
{
public:
	LLIODataSocket(
		U16 suggested_port,
		U16 start_discovery_port);
	virtual ~LLIODataSocket();
protected:
private:
	apr_socket_t* mSocket;
};
#endif
#endif
