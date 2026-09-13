/** 
 * @file net.cpp
 * @brief Cross-platform routines for sending and receiving packets.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#include <stdexcept>
#if LL_WINDOWS
#include "llwin32headerslean.h"
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <errno.h>
#endif
#include "llerror.h"
#include "llhost.h"
#include "lltimer.h"
#include "indra_constants.h"
#if LL_WINDOWS
SOCKADDR_IN stDstAddr;
SOCKADDR_IN stSrcAddr;
SOCKADDR_IN stLclAddr;
static WSADATA stWSAData;
#else
struct sockaddr_in stDstAddr;
struct sockaddr_in stSrcAddr;
struct sockaddr_in stLclAddr;
#if LL_DARWIN
#ifndef _SOCKLEN_T
#define _SOCKLEN_T
typedef int socklen_t;
#endif
#endif
#endif
static U32 gsnReceivingIFAddr = INVALID_HOST_IP_ADDRESS;
const char* LOOPBACK_ADDRESS_STRING = "127.0.0.1";
const char* BROADCAST_ADDRESS_STRING = "255.255.255.255";
#if LL_DARWIN
	const int	SEND_BUFFER_SIZE	= 200000;
	const int	RECEIVE_BUFFER_SIZE	= 200000;
#else
	const int	SEND_BUFFER_SIZE	= 400000;
	const int	RECEIVE_BUFFER_SIZE	= 400000;
#endif
LLHost get_sender()
{
	return LLHost(stSrcAddr.sin_addr.s_addr, ntohs(stSrcAddr.sin_port));
}
U32 get_sender_ip(void)
{
	return stSrcAddr.sin_addr.s_addr;
}
U32 get_sender_port()
{
	return ntohs(stSrcAddr.sin_port);
}
LLHost get_receiving_interface()
{
	return LLHost(gsnReceivingIFAddr, INVALID_PORT);
}
U32 get_receiving_interface_ip(void)
{
	return gsnReceivingIFAddr;
}
const char* u32_to_ip_string(U32 ip)
{
	static char buffer[MAXADDRSTR];
	in_addr in;
	in.s_addr = ip;
	char* result = inet_ntoa(in);
	if (result != NULL)
	{
		strncpy( buffer, result, MAXADDRSTR );
		buffer[MAXADDRSTR-1] = '\0';
		return buffer;
	}
	else
	{
		return "(bad IP addr)";
	}
}
char *u32_to_ip_string(U32 ip, char *ip_string)
{
	char *result;
	in_addr in;
	in.s_addr = ip;
	result = inet_ntoa(in);
	if (result != NULL)
	{
		strcpy(ip_string, result);
		return ip_string;
	}
	else
	{
		return NULL;
	}
}
U32 ip_string_to_u32(const char* ip_string)
{
	U32 ip = inet_addr(ip_string);
	if (ip == INADDR_NONE
			&& strncmp(ip_string, BROADCAST_ADDRESS_STRING, MAXADDRSTR) != 0)
	{
		LL_WARNS() << "ip_string_to_u32() failed, Error: Invalid IP string '" << ip_string << "'" << LL_ENDL;
		return INVALID_HOST_IP_ADDRESS;
	}
	return ip;
}
#if LL_WINDOWS
S32 start_net(S32& socket_out, int& nPort)
{
	int nRet;
	int hSocket;
	int snd_size = SEND_BUFFER_SIZE;
	int rec_size = RECEIVE_BUFFER_SIZE;
	int buff_size = 4;
	if (WSAStartup(0x0202, &stWSAData))
	{
		S32 err = WSAGetLastError();
		WSACleanup();
		LL_WARNS("AppInit") << "Windows Sockets initialization failed, err " << err << LL_ENDL;
		return 1;
	}
	hSocket = (int)socket(AF_INET, SOCK_DGRAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		S32 err = WSAGetLastError();
		WSACleanup();
		LL_WARNS("AppInit") << "socket() failed, err " << err << LL_ENDL;
		return 2;
	}
	stLclAddr.sin_family      = AF_INET;
	stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	stLclAddr.sin_port        = htons(nPort);
	S32 attempt_port = nPort;
	LL_DEBUGS("AppInit") << "attempting to connect on port " << attempt_port << LL_ENDL;
	nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
	if (nRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEADDRINUSE)
		{
			for(attempt_port = PORT_DISCOVERY_RANGE_MIN;
				attempt_port <= PORT_DISCOVERY_RANGE_MAX;
				attempt_port++)
			{
				stLclAddr.sin_port = htons(attempt_port);
				LL_DEBUGS("AppInit") << "trying port " << attempt_port << LL_ENDL;
				nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
				if (!(nRet == SOCKET_ERROR &&
					WSAGetLastError() == WSAEADDRINUSE))
				{
					break;
				}
			}
			if (nRet == SOCKET_ERROR)
			{
				LL_WARNS("AppInit") << "startNet() : Couldn't find available network port." << LL_ENDL;
				return 3;
			}
		}
		else
		{
			LL_WARNS("AppInit") << llformat("bind() port: %d failed, Err: %d\n", nPort, WSAGetLastError()) << LL_ENDL;
			return 4;
		}
	}
	sockaddr_in socket_address;
	S32 socket_address_size = sizeof(socket_address);
	getsockname(hSocket, (SOCKADDR*) &socket_address, &socket_address_size);
	attempt_port = ntohs(socket_address.sin_port);
	LL_INFOS("AppInit") << "connected on port " << attempt_port << LL_ENDL;
	nPort = attempt_port;
	unsigned long argp = 1;
	nRet = ioctlsocket (hSocket, FIONBIO, &argp);
	if (nRet == SOCKET_ERROR)
	{
		printf("Failed to set socket non-blocking, Err: %d\n",
		WSAGetLastError());
	}
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, buff_size);
	if (nRet)
	{
		LL_INFOS("AppInit") << "Can't set receive buffer size!" << LL_ENDL;
	}
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, buff_size);
	if (nRet)
	{
		LL_INFOS("AppInit") << "Can't set send buffer size!" << LL_ENDL;
	}
	getsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, &buff_size);
	getsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, &buff_size);
	LL_DEBUGS("AppInit") << "startNet - receive buffer size : " << rec_size << LL_ENDL;
	LL_DEBUGS("AppInit") << "startNet - send buffer size    : " << snd_size << LL_ENDL;
	stDstAddr.sin_family =      AF_INET;
	stDstAddr.sin_addr.s_addr = INVALID_HOST_IP_ADDRESS;
	stDstAddr.sin_port =        htons(nPort);
	socket_out = hSocket;
	return 0;
}
void end_net(S32& socket_out)
{
	if (socket_out >= 0)
	{
		shutdown(socket_out, SD_BOTH);
		closesocket(socket_out);
	}
	WSACleanup();
}
S32 receive_packet(int hSocket, char * receiveBuffer)
{
	int nRet;
	int addr_size = sizeof(struct sockaddr_in);
	nRet = recvfrom(hSocket, receiveBuffer, NET_BUFFER_SIZE, 0, (struct sockaddr*)&stSrcAddr, &addr_size);
	if (nRet == SOCKET_ERROR )
	{
		if (WSAEWOULDBLOCK == WSAGetLastError())
			return 0;
		if (WSAECONNRESET == WSAGetLastError())
			return 0;
		LL_INFOS() << "receivePacket() failed, Error: " << WSAGetLastError() << LL_ENDL;
	}
	return nRet;
}
BOOL send_packet(int hSocket, const char *sendBuffer, int size, U32 recipient, int nPort)
{
	int nRet = 0;
	U32 last_error = 0;
	stDstAddr.sin_addr.s_addr = recipient;
	stDstAddr.sin_port = htons(nPort);
	do
	{
		nRet = sendto(hSocket, sendBuffer, size, 0, (struct sockaddr*)&stDstAddr, sizeof(stDstAddr));
		if (nRet == SOCKET_ERROR )
		{
			last_error = WSAGetLastError();
			if (last_error != WSAEWOULDBLOCK)
			{
				if (WSAECONNRESET == WSAGetLastError())
				{
					return TRUE;
				}
				LL_INFOS() << "sendto() failed to " << u32_to_ip_string(recipient) << ":" << nPort
					<< ", Error " << last_error << LL_ENDL;
			}
		}
	} while (  (nRet == SOCKET_ERROR)
			 &&(last_error == WSAEWOULDBLOCK));
	return (nRet != SOCKET_ERROR);
}
#else
S32 start_net(S32& socket_out, int& nPort)
{
	int hSocket, nRet;
	int snd_size = SEND_BUFFER_SIZE;
	int rec_size = RECEIVE_BUFFER_SIZE;
	socklen_t buff_size = 4;
	hSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (hSocket < 0)
	{
		LL_WARNS() << "socket() failed" << LL_ENDL;
		return 1;
	}
	if (NET_USE_OS_ASSIGNED_PORT == nPort)
	{
		stLclAddr.sin_family      = AF_INET;
		stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		stLclAddr.sin_port        = htons(0);
		LL_INFOS() << "attempting to connect on OS assigned port" << LL_ENDL;
		nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
		if (nRet < 0)
		{
			LL_WARNS() << "Failed to bind on an OS assigned port error: "
					<< nRet << LL_ENDL;
		}
		else
		{
			sockaddr_in socket_info;
			socklen_t len = sizeof(sockaddr_in);
			int err = getsockname(hSocket, (sockaddr*)&socket_info, &len);
			LL_INFOS() << "Get socket returned: " << err << " length " << len << LL_ENDL;
			nPort = ntohs(socket_info.sin_port);
			LL_INFOS() << "Assigned port: " << nPort << LL_ENDL;
		}
	}
	else
	{
		stLclAddr.sin_family      = AF_INET;
		stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		stLclAddr.sin_port        = htons(nPort);
		U32 attempt_port = nPort;
		LL_INFOS() << "attempting to connect on port " << attempt_port << LL_ENDL;
		nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
		if (nRet < 0)
		{
			if (errno == EADDRINUSE)
			{
				for(attempt_port = PORT_DISCOVERY_RANGE_MIN;
					attempt_port <= PORT_DISCOVERY_RANGE_MAX;
					attempt_port++)
				{
					stLclAddr.sin_port = htons(attempt_port);
					LL_INFOS() << "trying port " << attempt_port << LL_ENDL;
					nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
					if (!((nRet < 0) && (errno == EADDRINUSE)))
					{
						break;
					}
				}
				if (nRet < 0)
				{
					LL_WARNS() << "startNet() : Couldn't find available network port." << LL_ENDL;
					return 3;
				}
			}
			else
			{
				LL_WARNS() << llformat ("bind() port: %d failed, Err: %s\n", nPort, strerror(errno)) << LL_ENDL;
				return 4;
			}
		}
		LL_INFOS() << "connected on port " << attempt_port << LL_ENDL;
		nPort = attempt_port;
	}
	fcntl(hSocket, F_SETFL, O_NONBLOCK);
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, buff_size);
	if (nRet)
	{
		LL_INFOS() << "Can't set receive size!" << LL_ENDL;
	}
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, buff_size);
	if (nRet)
	{
		LL_INFOS() << "Can't set send size!" << LL_ENDL;
	}
	getsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, &buff_size);
	getsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, &buff_size);
	LL_INFOS() << "startNet - receive buffer size : " << rec_size << LL_ENDL;
	LL_INFOS() << "startNet - send buffer size    : " << snd_size << LL_ENDL;
	char achMCAddr[MAXADDRSTR] = "127.0.0.1";
	stDstAddr.sin_family =      AF_INET;
	stDstAddr.sin_addr.s_addr = ip_string_to_u32(achMCAddr);
	stDstAddr.sin_port =        htons(nPort);
	socket_out = hSocket;
	return 0;
}
void end_net(S32& socket_out)
{
	if (socket_out >= 0)
	{
		close(socket_out);
	}
}
int receive_packet(int hSocket, char * receiveBuffer)
{
	int nRet;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	gsnReceivingIFAddr = INVALID_HOST_IP_ADDRESS;
	int recv_flags = 0;
	nRet = recvfrom(hSocket, receiveBuffer, NET_BUFFER_SIZE, recv_flags, (struct sockaddr*)&stSrcAddr, &addr_size);
	if (nRet == -1)
	{
		return 0;
	}
	return nRet;
}
BOOL send_packet(int hSocket, const char * sendBuffer, int size, U32 recipient, int nPort)
{
	int		ret;
	BOOL	success;
	BOOL	resend;
	S32		send_attempts = 0;
	stDstAddr.sin_addr.s_addr = recipient;
	stDstAddr.sin_port = htons(nPort);
	do
	{
		ret = sendto(hSocket, sendBuffer, size, 0,	(struct sockaddr*)&stDstAddr, sizeof(stDstAddr));
		send_attempts++;
		if (ret >= 0)
		{
			success = TRUE;
			resend = FALSE;
		}
		else
		{
			success = FALSE;
			if (errno == EAGAIN)
			{
				LL_INFOS() << "sendto() reported buffer full, resending (attempt " << send_attempts << ")" << LL_ENDL;
				LL_INFOS() << inet_ntoa(stDstAddr.sin_addr) << ":" << nPort << LL_ENDL;
				resend = TRUE;
			}
			else if (errno == ECONNREFUSED)
			{
				LL_INFOS() << "sendto() reported connection refused, resending (attempt " << send_attempts << ")" << LL_ENDL;
				LL_INFOS() << inet_ntoa(stDstAddr.sin_addr) << ":" << nPort << LL_ENDL;
				resend = TRUE;
			}
			else
			{
				LL_INFOS() << "sendto() failed: " << errno << ", " << strerror(errno) << LL_ENDL;
				LL_INFOS() << inet_ntoa(stDstAddr.sin_addr) << ":" << nPort << LL_ENDL;
				resend = FALSE;
			}
		}
	}
	while (resend && send_attempts < 3);
	if (send_attempts >= 3)
	{
		LL_INFOS() << "sendPacket() bailed out of send!" << LL_ENDL;
		return FALSE;
	}
	return success;
}
#endif
