/** 
 * @file llcircuit.h
 * @brief Provides a method for tracking network circuit information
 * for the UDP message system
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LL_LLCIRCUIT_H
#define LL_LLCIRCUIT_H
#include <map>
#include <vector>
#include "llerror.h"
#include "lltimer.h"
#include "net.h"
#include "llhost.h"
#include "llpacketack.h"
#include "lluuid.h"
#include "llthrottle.h"
const F32 LL_AVERAGED_PING_ALPHA = 0.2f;
const F32Milliseconds LL_AVERAGED_PING_MAX(2000);
const F32Milliseconds LL_AVERAGED_PING_MIN(100);
const U32Milliseconds INITIAL_PING_VALUE_MSEC(1000);
const TPACKETID LL_MAX_OUT_PACKET_ID = 0x01000000;
const int LL_ERR_CIRCUIT_GONE   = -23017;
const int LL_ERR_TCP_TIMEOUT    = -23016;
const U8 LL_PACKET_ID_SIZE = 6;
const S32 LL_MAX_RESENT_PACKETS_PER_FRAME = 100;
const S32 LL_MAX_ACKED_PACKETS_PER_FRAME = 200;
const F32 LL_COLLECT_ACK_TIME_MAX = 2.f;
class LLMessageSystem;
class LLEncodedDatagramService;
class LLSD;
class LLCircuitData
{
public:
	LLCircuitData(const LLHost &host, TPACKETID in_id,
				  const F32Seconds circuit_heartbeat_interval, const F32Seconds circuit_timeout);
	~LLCircuitData();
	S32		resendUnackedPackets(const F64Seconds now);
	void	clearDuplicateList(TPACKETID oldest_id);
	void	dumpResendCountAndReset();
	void		pingTimerStart();
	void		pingTimerStop(const U8 ping_id);
	void			ackReliablePacket(TPACKETID packet_num);
	const LLUUID& getRemoteID() const { return mRemoteID; }
	const LLUUID& getRemoteSessionID() const { return mRemoteSessionID; }
	void setRemoteID(const LLUUID& id) { mRemoteID = id; }
	void setRemoteSessionID(const LLUUID& id) { mRemoteSessionID = id; }
	void		setTrusted(BOOL t);
	const		LLUUID& getLocalEndPointID() const { return mLocalEndPointID; }
	U32Milliseconds	getPingDelay() const;
	S32				getPingsInTransit() const			{ return mPingsInTransit; }
	BOOL		isAlive() const;
	BOOL		isBlocked() const;
	BOOL		getAllowTimeout() const;
	F32Milliseconds	getPingDelayAveraged();
	F32Milliseconds	getPingInTransitTime();
	U32			getPacketsIn() const;
	S32Bytes	getBytesIn() const;
	S32Bytes	getBytesOut() const;
	U32			getPacketsOut() const;
	U32			getPacketsLost() const;
	TPACKETID	getPacketOutID() const;
	BOOL		getTrusted() const;
	F32			getAgeInSeconds() const;
	S32			getUnackedPacketCount() const	{ return mUnackedPacketCount; }
	S32			getUnackedPacketBytes() const	{ return mUnackedPacketBytes; }
	F64Seconds  getNextPingSendTime() const { return mNextPingSendTime; }
    U32         getLastPacketGap() const { return mLastPacketGap; }
    LLHost      getHost() const { return mHost; }
	F64Seconds	getLastPacketInTime() const		{ return mLastPacketInTime;	}
	LLThrottleGroup &getThrottleGroup()		{	return mThrottles; }
	class less
	{
	public:
		bool operator()(const LLCircuitData* lhs, const LLCircuitData* rhs) const
		{
			if (lhs->getNextPingSendTime() < rhs->getNextPingSendTime())
			{
				return true;
			}
			else if (lhs->getNextPingSendTime() > rhs->getNextPingSendTime())
			{
				return false;
			}
			else return lhs > rhs;
		}
	};
	void					checkPeriodTime();
	friend std::ostream&	operator<<(std::ostream& s, LLCircuitData &circuit);
	void getInfo(LLSD& info) const;
	friend class LLCircuit;
	friend class LLMessageSystem;
	friend class LLEncodedDatagramService;
	friend void crash_on_spaceserver_timeout (const LLHost &host, void *);
protected:
	TPACKETID		nextPacketOutID();
	void				setPacketInID(TPACKETID id);
	void					checkPacketInID(TPACKETID id, BOOL receive_resent);
	void			setPingDelay(U32Milliseconds ping);
	BOOL			checkCircuitTimeout();
	void			addBytesIn(S32Bytes bytes);
	void			addBytesOut(S32Bytes bytes);
	U8				nextPingID()			{ mLastPingID++; return mLastPingID; }
	BOOL			updateWatchDogTimers(LLMessageSystem *msgsys);
	void			addReliablePacket(S32 mSocket, U8 *buf_ptr, S32 buf_len, LLReliablePacketParams *params);
	BOOL			isDuplicateResend(TPACKETID packetnum);
	BOOL collectRAck(TPACKETID packet_num);
	void			setTimeoutCallback(void (*callback_func)(const LLHost &host, void *user_data), void *user_data);
	void			setAlive(BOOL b_alive);
	void			setAllowTimeout(BOOL allow);
protected:
	LLHost mHost;
	LLUUID mRemoteID;
	LLUUID mRemoteSessionID;
	LLThrottleGroup	mThrottles;
	TPACKETID		mWrapID;
	TPACKETID		mPacketsOutID;
	TPACKETID		mPacketsInID;
	TPACKETID		mHighestPacketID;
	void	(*mTimeoutCallback)(const LLHost &host, void *user_data);
	void	*mTimeoutUserData;
	BOOL	mTrusted;
	BOOL	mbAllowTimeout;
	BOOL	mbAlive;
	BOOL	mBlocked;
	F64Seconds	mPingTime;
	F64Seconds	mLastPingSendTime;
	F64Seconds	mLastPingReceivedTime;
	F64Seconds  mNextPingSendTime;
	S32			mPingsInTransit;
	U8			mLastPingID;
	U32Milliseconds		mPingDelay;
	F32Milliseconds		mPingDelayAveraged;
	typedef std::map<TPACKETID, U64Microseconds> packet_time_map;
	packet_time_map							mPotentialLostPackets;
	packet_time_map							mRecentlyReceivedReliablePackets;
	std::vector<TPACKETID> mAcks;
	F32 mAckCreationTime;
	typedef std::map<TPACKETID, LLReliablePacket *> reliable_map;
	typedef reliable_map::iterator					reliable_iter;
	reliable_map							mUnackedPackets;
	reliable_map							mFinalRetryPackets;
	S32										mUnackedPacketCount;
	S32										mUnackedPacketBytes;
	F64Seconds								mLastPacketInTime;
	LLUUID									mLocalEndPointID;
	U32		mPacketsOut;
	U32		mPacketsIn;
	S32		mPacketsLost;
	S32Bytes	mBytesIn,
				mBytesOut;
	F32Seconds	mLastPeriodLength;
	S32Bytes	mBytesInLastPeriod;
	S32Bytes	mBytesOutLastPeriod;
	S32Bytes	mBytesInThisPeriod;
	S32Bytes	mBytesOutThisPeriod;
	F32		mPeakBPSIn;
	F32		mPeakBPSOut;
	F64Seconds	mPeriodTime;
	LLTimer	mExistenceTimer;
	S32		mCurrentResendCount;
    U32     mLastPacketGap;
	const F32Seconds mHeartbeatInterval;
	const F32Seconds mHeartbeatTimeout;
};
class LLCircuit
{
public:
	LLCircuit(const F32Seconds circuit_heartbeat_interval, const F32Seconds circuit_timeout);
	~LLCircuit();
	LLCircuitData* findCircuit(const LLHost& host) const;
	BOOL isCircuitAlive(const LLHost& host) const;
	LLCircuitData	*addCircuitData(const LLHost &host, TPACKETID in_id);
	void			removeCircuitData(const LLHost &host);
	void		    updateWatchDogTimers(LLMessageSystem *msgsys);
	void			resendUnackedPackets(S32& unacked_list_length, S32& unacked_list_size);
	void sendAcks(F32 collect_time);
	friend std::ostream& operator<<(std::ostream& s, LLCircuit &circuit);
	void getInfo(LLSD& info) const;
	void			dumpResends();
	typedef std::map<LLHost, LLCircuitData*> circuit_data_map;
	void getCircuitRange(
		const LLHost& key,
		circuit_data_map::iterator& first,
		circuit_data_map::iterator& end);
	std::vector<LLCircuitData*> getCircuitDataList();
	circuit_data_map mUnackedCircuitMap;
	circuit_data_map mSendAckMap;
protected:
	circuit_data_map mCircuitData;
	typedef std::set<LLCircuitData *, LLCircuitData::less> ping_set_t;
	ping_set_t mPingSet;
	mutable LLCircuitData* mLastCircuit;
private:
	const F32Seconds mHeartbeatInterval;
	const F32Seconds mHeartbeatTimeout;
};
#endif
