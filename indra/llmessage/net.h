/** 
 * @file net.h
 * @brief Cross platform UDP network code.
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
#ifndef LL_NET_H
#define LL_NET_H
class LLTimer;
class LLHost;
#define NET_BUFFER_SIZE (0x2000)
#define NET_USE_OS_ASSIGNED_PORT 0
S32		start_net(S32& socket_out, int& nPort);
void	end_net(S32& socket_out);
S32		receive_packet(int hSocket, char * receiveBuffer);
BOOL	send_packet(int hSocket, const char *sendBuffer, int size, U32 recipient, int nPort);
LLHost	get_sender();
U32		get_sender_port();
U32		get_sender_ip(void);
LLHost	get_receiving_interface();
U32		get_receiving_interface_ip(void);
const char*	u32_to_ip_string(U32 ip);
char*		u32_to_ip_string(U32 ip, char *ip_string);
U32			ip_string_to_u32(const char* ip_string);
extern const char* LOOPBACK_ADDRESS_STRING;
extern const char* BROADCAST_ADDRESS_STRING;
const S32	MTUBYTES = 1200;
const S32	ETHERNET_MTU_BYTES = 1500;
const S32	MTUBITS = MTUBYTES*8;
const S32	MTUU32S = MTUBITS/32;
const	U32		PORT_DISCOVERY_RANGE_MIN		= 13000;
const	U32		PORT_DISCOVERY_RANGE_MAX		= PORT_DISCOVERY_RANGE_MIN + 50;
#endif
