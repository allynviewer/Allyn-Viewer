/** 
 * @file message.h
 * @brief LLMessageSystem class header file
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
#ifndef LL_MESSAGE_H
#define LL_MESSAGE_H
#include <cstring>
#include <set>
#if LL_SOLARIS
#include <netinet/in.h>
#endif
#if LL_WINDOWS
#include "winsock2.h"
#endif
#include "llerror.h"
#include "net.h"
#include "llstringtable.h"
#include "llcircuit.h"
#include "lltimer.h"
#include "llhost.h"
#include "llhttpnode.h"
#include "llpacketack.h"
#include "llsingleton.h"
#include "message_prehash.h"
#include "llstl.h"
#include "llmsgvariabletype.h"
#include "llmessagesenderinterface.h"
#include "llstoredmessage.h"
class LLPacketRing;
namespace
{
	class LLFnPtrResponder;
}
const U32 MESSAGE_MAX_STRINGS_LENGTH = 64;
const U32 MESSAGE_NUMBER_OF_HASH_BUCKETS = 8192;
const S32 MESSAGE_MAX_PER_FRAME = 400;
class LLMessageStringTable : public LLSingleton<LLMessageStringTable>
{
public:
	LLMessageStringTable();
	~LLMessageStringTable();
	char *getString(const char *str);
	U32	 mUsed;
	BOOL mEmpty[MESSAGE_NUMBER_OF_HASH_BUCKETS];
	char mString[MESSAGE_NUMBER_OF_HASH_BUCKETS][MESSAGE_MAX_STRINGS_LENGTH];
};
const S32 MAX_MESSAGE_INTERNAL_NAME_SIZE = 255;
const S32 MAX_BUFFER_SIZE = NET_BUFFER_SIZE;
const S32 MAX_BLOCKS = 255;
const U8 LL_ZERO_CODE_FLAG = 0x80;
const U8 LL_RELIABLE_FLAG = 0x40;
const U8 LL_RESENT_FLAG = 0x20;
const U8 LL_ACK_FLAG = 0x10;
const S32 LL_MINIMUM_VALID_PACKET_SIZE = LL_PACKET_ID_SIZE + 1;
enum EPacketHeaderLayout
{
	PHL_FLAGS = 0,
	PHL_PACKET_ID = 1,
	PHL_OFFSET = 5,
	PHL_NAME = 6
};
const S32 LL_DEFAULT_RELIABLE_RETRIES = 3;
const F32Seconds LL_MINIMUM_RELIABLE_TIMEOUT_SECONDS(1.f);
const F32Seconds LL_MINIMUM_SEMIRELIABLE_TIMEOUT_SECONDS(1.f);
const F32Seconds LL_PING_BASED_TIMEOUT_DUMMY(0.0f);
const F32 LL_SEMIRELIABLE_TIMEOUT_FACTOR	= 5.f;
const F32 LL_RELIABLE_TIMEOUT_FACTOR		= 5.f;
const F32 LL_FILE_XFER_TIMEOUT_FACTOR		= 5.f;
const F32 LL_LOST_TIMEOUT_FACTOR			= 16.f;
const F32Seconds LL_MAX_LOST_TIMEOUT(5.f);
const S32 MAX_MESSAGE_COUNT_NUM = 1024;
class LLCircuit;
class LLVector3;
class LLVector4;
class LLVector3d;
class LLQuaternion;
class LLSD;
class LLUUID;
class LLMessageSystem;
class LLPumpIO;
enum EMessageException
{
	MX_UNREGISTERED_MESSAGE,
	MX_PACKET_TOO_SHORT,
	MX_RAN_OFF_END_OF_PACKET,
	MX_WROTE_PAST_BUFFER_SIZE
};
typedef void (*msg_exception_callback)(LLMessageSystem*,void*,EMessageException);
class LLMsgData;
class LLMsgBlkData;
class LLMessageTemplate;
class LLMessagePollInfo;
class LLMessageBuilder;
class LLTemplateMessageBuilder;
class LLSDMessageBuilder;
class LLMessageReader;
class LLTemplateMessageReader;
class LLSDMessageReader;
class LLUseCircuitCodeResponder
{
	LOG_CLASS(LLMessageSystem);
public:
	virtual ~LLUseCircuitCodeResponder();
	virtual void complete(const LLHost& host, const LLUUID& agent) const = 0;
};
class LLMessageSystem : public LLMessageSenderInterface
{
 private:
	U8					mSendBuffer[MAX_BUFFER_SIZE];
	S32					mSendSize;
	bool				mBlockUntrustedInterface;
	LLHost				mUntrustedInterface;
 public:
	LLPacketRing*				mPacketRing;
	LLReliablePacketParams			mReliablePacketParams;
	BOOL mVerboseLog;
	F32                                     mMessageFileVersionNumber;
	typedef std::map<const char *, LLMessageTemplate*> message_template_name_map_t;
	typedef std::map<U32, LLMessageTemplate*> message_template_number_map_t;
private:
	message_template_name_map_t		mMessageTemplates;
	message_template_number_map_t		mMessageNumbers;
	friend class LLFloaterMessageLogItem;
	friend class LLFloaterMessageLog;
public:
	S32					mSystemVersionMajor;
	S32					mSystemVersionMinor;
	S32					mSystemVersionPatch;
	S32					mSystemVersionServer;
	U32					mVersionFlags;
	BOOL					mbProtected;
	U32					mNumberHighFreqMessages;
	U32					mNumberMediumFreqMessages;
	U32					mNumberLowFreqMessages;
	S32					mPort;
	S32					mSocket;
	U32					mPacketsIn;
	U32					mPacketsOut;
	U64					mBytesIn;
	U64					mBytesOut;
	U32					mCompressedPacketsIn;
	U32					mCompressedPacketsOut;
	U32					mReliablePacketsIn;
	U32					mReliablePacketsOut;
	U32                                     mDroppedPackets;
	U32                                     mResentPackets;
	U32                                     mFailedResendPackets;
	U32                                     mOffCircuitPackets;
	U32                                     mInvalidOnCircuitPackets;
	S64					mUncompressedBytesIn;
	S64					mUncompressedBytesOut;
	S64					mCompressedBytesIn;
	S64					mCompressedBytesOut;
	S64					mTotalBytesIn;
	S64					mTotalBytesOut;
	BOOL                                    mSendReliable;
	LLCircuit 	 			mCircuitInfo;
	F64Seconds			mCircuitPrintTime;
	F32Seconds			mCircuitPrintFreq;
	std::map<U64, U32>			mIPPortToCircuitCode;
	std::map<U32, U64>			mCircuitCodeToIPPort;
	U32					mOurCircuitCode;
	S32					mSendPacketFailureCount;
	S32					mUnackedListDepth;
	S32					mUnackedListSize;
	S32					mDSMaxListDepth;
public:
	LLMessageSystem(const std::string& filename, U32 port, S32 version_major,
					S32 version_minor, S32 version_patch,
					bool failure_is_fatal,
					const F32 circuit_heartbeat_interval, const F32 circuit_timeout);
	~LLMessageSystem();
	BOOL isOK() const { return !mbError; }
	S32 getErrorCode() const { return mErrorCode; }
	void loadTemplateFile(const std::string& filename, bool failure_is_fatal);
	void	setHandlerFuncFast(const char *name, void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data = NULL);
	void	setHandlerFunc(const char *name, void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data = NULL)
	{
		setHandlerFuncFast(LLMessageStringTable::getInstance()->getString(name), handler_func, user_data);
	}
	void setExceptionFunc(EMessageException exception, msg_exception_callback func, void* data = NULL);
	BOOL callExceptionFunc(EMessageException exception);
	typedef void (*msg_timing_callback)(const char* hashed_name, F32 time, void* data);
	void setTimingFunc(msg_timing_callback func, void* data = NULL);
	msg_timing_callback getTimingCallback()
	{
		return mTimingCallback;
	}
	void* getTimingCallbackData()
	{
		return mTimingCallbackData;
	}
	BOOL isCircuitCodeKnown(U32 code) const;
	bool addCircuitCode(U32 code, const LLUUID& session_id);
	BOOL	poll(F32 seconds);
	BOOL	checkMessages(S64 frame_count = 0);
	void	processAcks(F32 collect_time = 0.f);
	BOOL	isMessageFast(const char *msg);
	BOOL	isMessage(const char *msg)
	{
		return isMessageFast(LLMessageStringTable::getInstance()->getString(msg));
	}
	void dumpPacketToLog();
	char	*getMessageName();
	const LLHost& getSender() const;
	U32		getSenderIP() const;
	U32		getSenderPort() const;
	const LLHost& getReceivingInterface() const;
	const LLUUID& getSenderID() const;
	const LLUUID& getSenderSessionID() const;
	void setMySessionID(const LLUUID& session_id) { mSessionID = session_id; }
	const LLUUID& getMySessionID() { return mSessionID; }
	void newMessageFast(const char *name);
	void newMessage(const char *name);
public:
	LLStoredMessagePtr getReceivedMessage() const;
	LLStoredMessagePtr getBuiltMessage() const;
	S32 sendMessage(const LLHost &host, LLStoredMessagePtr message);
private:
	LLSD getReceivedMessageLLSD() const;
	LLSD getBuiltMessageLLSD() const;
	LLSD wrapReceivedTemplateData() const;
	LLSD wrapBuiltTemplateData() const;
public:
	void copyMessageReceivedToSend();
	void clearMessage();
	void nextBlockFast(const char *blockname);
	void nextBlock(const char *blockname);
public:
	void addBinaryDataFast(const char *varname, const void *data, S32 size);
	void addBinaryData(const char *varname, const void *data, S32 size);
	void	addBOOLFast( const char* varname, BOOL b);
	void	addBOOL( const char* varname, BOOL b);
	void	addS8Fast(	const char *varname, S8 s);
	void	addS8(	const char *varname, S8 s);
	void	addU8Fast(	const char *varname, U8 u);
	void	addU8(	const char *varname, U8 u);
	void	addS16Fast(	const char *varname, S16 i);
	void	addS16(	const char *varname, S16 i);
	void	addU16Fast(	const char *varname, U16 i);
	void	addU16(	const char *varname, U16 i);
	void	addF32Fast(	const char *varname, F32 f);
	void	addF32(	const char *varname, F32 f);
	void	addS32Fast(	const char *varname, S32 s);
	void	addS32(	const char *varname, S32 s);
	void addU32Fast(	const char *varname, U32 u);
	void	addU32(	const char *varname, U32 u);
	void	addU64Fast(	const char *varname, U64 lu);
	void	addU64(	const char *varname, U64 lu);
	void	addF64Fast(	const char *varname, F64 d);
	void	addF64(	const char *varname, F64 d);
	void	addVector3Fast(	const char *varname, const LLVector3& vec);
	void	addVector3(	const char *varname, const LLVector3& vec);
	void	addVector4Fast(	const char *varname, const LLVector4& vec);
	void	addVector4(	const char *varname, const LLVector4& vec);
	void	addVector3dFast( const char *varname, const LLVector3d& vec);
	void	addVector3d( const char *varname, const LLVector3d& vec);
	void	addQuatFast( const char *varname, const LLQuaternion& quat);
	void	addQuat( const char *varname, const LLQuaternion& quat);
	void addUUIDFast( const char *varname, const LLUUID& uuid);
	void	addUUID( const char *varname, const LLUUID& uuid);
	void	addIPAddrFast( const char *varname, const U32 ip);
	void	addIPAddr( const char *varname, const U32 ip);
	void	addIPPortFast( const char *varname, const U16 port);
	void	addIPPort( const char *varname, const U16 port);
	void	addStringFast( const char* varname, const char* s);
	void	addString( const char* varname, const char* s);
	void	addStringFast( const char* varname, const std::string& s);
	void	addString( const char* varname, const std::string& s);
	S32 getCurrentSendTotal() const;
	TPACKETID getCurrentRecvPacketID() { return mCurrentRecvPacketID; }
	BOOL isSendFull(const char* blockname = NULL);
	BOOL isSendFullFast(const char* blockname = NULL);
	S32     zeroCode(U8 **data, S32 *data_size);
	S32		zeroCodeExpand(U8 **data, S32 *data_size);
	S32		zeroCodeAdjustCurrentSendTotal();
	S32 sendReliable(const LLHost &host);
	S32	sendReliable(const U32 circuit)			{ return sendReliable(findHost(circuit)); }
	S32	sendReliable(	const LLHost &host,
							S32 retries,
							BOOL ping_based_retries,
							F32Seconds timeout,
							void (*callback)(void **,S32),
							void ** callback_data);
	S32 sendSemiReliable(	const LLHost &host,
							void (*callback)(void **,S32), void ** callback_data);
	S32	 flushSemiReliable(	const LLHost &host,
								void (*callback)(void **,S32), void ** callback_data);
	S32	flushReliable(	const LLHost &host );
	void forwardMessage(const LLHost &host);
	void forwardReliable(const LLHost &host);
	void forwardReliable(const U32 circuit_code);
	S32 forwardReliable(
		const LLHost &host,
		S32 retries,
		BOOL ping_based_timeout,
		F32Seconds timeout,
		void (*callback)(void **,S32),
		void ** callback_data);
	LLFnPtrResponder* createResponder(const std::string& name);
	S32		sendMessage(const LLHost &host);
	S32		sendMessage(const U32 circuit);
private:
	S32		sendMessage(const LLHost &host, const char* name,
						const LLSD& message);
public:
	void	getBinaryDataFast(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum = 0, S32 max_size = S32_MAX);
	void	getBinaryData(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum = 0, S32 max_size = S32_MAX);
	void	getBOOLFast(	const char *block, const char *var, BOOL &data, S32 blocknum = 0);
	void	getBOOL(	const char *block, const char *var, BOOL &data, S32 blocknum = 0);
	void	getS8Fast(		const char *block, const char *var, S8 &data, S32 blocknum = 0);
	void	getS8(		const char *block, const char *var, S8 &data, S32 blocknum = 0);
	void	getU8Fast(		const char *block, const char *var, U8 &data, S32 blocknum = 0);
	void	getU8(		const char *block, const char *var, U8 &data, S32 blocknum = 0);
	void	getS16Fast(		const char *block, const char *var, S16 &data, S32 blocknum = 0);
	void	getS16(		const char *block, const char *var, S16 &data, S32 blocknum = 0);
	void	getU16Fast(		const char *block, const char *var, U16 &data, S32 blocknum = 0);
	void	getU16(		const char *block, const char *var, U16 &data, S32 blocknum = 0);
	void	getS32Fast(		const char *block, const char *var, S32 &data, S32 blocknum = 0);
	void	getS32(		const char *block, const char *var, S32 &data, S32 blocknum = 0);
	void	getF32Fast(		const char *block, const char *var, F32 &data, S32 blocknum = 0);
	void	getF32(		const char *block, const char *var, F32 &data, S32 blocknum = 0);
	void getU32Fast(		const char *block, const char *var, U32 &data, S32 blocknum = 0);
	void	getU32(		const char *block, const char *var, U32 &data, S32 blocknum = 0);
	void getU64Fast(		const char *block, const char *var, U64 &data, S32 blocknum = 0);
	void	getU64(		const char *block, const char *var, U64 &data, S32 blocknum = 0);
	void	getF64Fast(		const char *block, const char *var, F64 &data, S32 blocknum = 0);
	void	getF64(		const char *block, const char *var, F64 &data, S32 blocknum = 0);
	void	getVector3Fast(	const char *block, const char *var, LLVector3 &vec, S32 blocknum = 0);
	void	getVector3(	const char *block, const char *var, LLVector3 &vec, S32 blocknum = 0);
	void	getVector4Fast(	const char *block, const char *var, LLVector4 &vec, S32 blocknum = 0);
	void	getVector4(	const char *block, const char *var, LLVector4 &vec, S32 blocknum = 0);
	void	getVector3dFast(const char *block, const char *var, LLVector3d &vec, S32 blocknum = 0);
	void	getVector3d(const char *block, const char *var, LLVector3d &vec, S32 blocknum = 0);
	void	getQuatFast(	const char *block, const char *var, LLQuaternion &q, S32 blocknum = 0);
	void	getQuat(	const char *block, const char *var, LLQuaternion &q, S32 blocknum = 0);
	void getUUIDFast(	const char *block, const char *var, LLUUID &uuid, S32 blocknum = 0);
	void	getUUID(	const char *block, const char *var, LLUUID &uuid, S32 blocknum = 0);
	void getIPAddrFast(	const char *block, const char *var, U32 &ip, S32 blocknum = 0);
	void	getIPAddr(	const char *block, const char *var, U32 &ip, S32 blocknum = 0);
	void getIPPortFast(	const char *block, const char *var, U16 &port, S32 blocknum = 0);
	void	getIPPort(	const char *block, const char *var, U16 &port, S32 blocknum = 0);
	void getStringFast(	const char *block, const char *var, S32 buffer_size, char *buffer, S32 blocknum = 0);
	void	getString(	const char *block, const char *var, S32 buffer_size, char *buffer, S32 blocknum = 0);
	void getStringFast(	const char *block, const char *var, std::string& outstr, S32 blocknum = 0);
	void	getString(	const char *block, const char *var, std::string& outstr, S32 blocknum = 0);
	bool generateDigestForNumberAndUUIDs(char* digest, const U32 number, const LLUUID &id1, const LLUUID &id2) const;
	bool generateDigestForWindowAndUUIDs(char* digest, const S32 window, const LLUUID &id1, const LLUUID &id2) const;
	bool isMatchingDigestForWindowAndUUIDs(const char* digest, const S32 window, const LLUUID &id1, const LLUUID &id2) const;
	bool generateDigestForNumber(char* digest, const U32 number) const;
	bool generateDigestForWindow(char* digest, const S32 window) const;
	bool isMatchingDigestForWindow(const char* digest, const S32 window) const;
	void	showCircuitInfo();
	void getCircuitInfo(LLSD& info) const;
	U32 getOurCircuitCode();
	void	enableCircuit(const LLHost &host, BOOL trusted);
	void	disableCircuit(const LLHost &host);
	void sendCreateTrustedCircuit(const LLHost& host, const LLUUID & id1, const LLUUID & id2);
	void	sendDenyTrustedCircuit(const LLHost &host);
	bool isTrustedSender(const LLHost& host) const;
	bool isTrustedSender() const;
	bool isTrustedMessage(const std::string& name) const;
	bool isUntrustedMessage(const std::string& name) const;
	void setUntrustedInterface( const LLHost host ) { mUntrustedInterface = host; }
	LLHost getUntrustedInterface() const { return mUntrustedInterface; }
	void setBlockUntrustedInterface( bool block ) { mBlockUntrustedInterface = block; }
	bool getBlockUntrustedInterface() const { return mBlockUntrustedInterface; }
	void banUdpMessage(const std::string& name);
private:
	typedef std::set<LLHost> host_set_t;
	host_set_t mDenyTrustedCircuitSet;
	void	reallySendDenyTrustedCircuit(const LLHost &host);
public:
	void	establishBidirectionalTrust(const LLHost &host, S64 frame_count = 0);
	BOOL    getCircuitTrust(const LLHost &host);
	void	setCircuitAllowTimeout(const LLHost &host, BOOL allow);
	void	setCircuitTimeoutCallback(const LLHost &host, void (*callback_func)(const LLHost &host, void *user_data), void *user_data);
	BOOL	checkCircuitBlocked(const U32 circuit);
	BOOL	checkCircuitAlive(const U32 circuit);
	BOOL	checkCircuitAlive(const LLHost &host);
	void	setCircuitProtection(BOOL b_protect);
	U32		findCircuitCode(const LLHost &host);
	LLHost	findHost(const U32 circuit_code);
	void	sanityCheck();
	BOOL	has(const char *blockname) const;
	S32		getNumberOfBlocksFast(const char *blockname) const;
	S32		getNumberOfBlocks(const char *blockname) const;
	S32		getSizeFast(const char *blockname, const char *varname) const;
	S32		getSize(const char *blockname, const char *varname) const;
	S32		getSizeFast(const char *blockname, S32 blocknum,
						const char *varname) const;
	S32		getSize(const char *blockname, S32 blocknum, const char *varname) const;
	void	resetReceiveCounts();
	void	dumpReceiveCounts();
	void	dumpCircuitInfo();
	BOOL	isClear() const;
	S32 	flush(const LLHost &host);
	U32		getListenPort( void ) const;
	void startLogging();
	void stopLogging();
	void summarizeLogs(std::ostream& str);
	S32		getReceiveSize() const;
	S32		getReceiveCompressedSize() const { return mIncomingCompressedSize; }
	S32		getReceiveBytes() const;
	S32		getUnackedListSize() const			{ return mUnackedListSize; }
	friend std::ostream&	operator<<(std::ostream& s, LLMessageSystem &msg);
	void setMaxMessageTime(const F32 seconds);
	void setMaxMessageCounts(const S32 num);
	static U64Microseconds getMessageTimeUsecs(const BOOL update = FALSE);
	static F64Seconds getMessageTimeSeconds(const BOOL update = FALSE);
	static void setTimeDecodes(BOOL b);
	static void setTimeDecodesSpamThreshold(F32 seconds);
	static void processAddCircuitCode(LLMessageSystem* msg, void**);
	static void processUseCircuitCode(LLMessageSystem* msg, void**);
	static void processError(LLMessageSystem* msg, void**);
	static void dispatch(const std::string& msg_name,
						 const LLSD& message);
	static void dispatch(const std::string& msg_name,
						 const LLSD& message,
						 LLHTTPNode::ResponsePtr responsep);
	static void dispatchTemplate(const std::string& msg_name,
						 const LLSD& message,
						 LLHTTPNode::ResponsePtr responsep);
	void setMessageBans(const LLSD& trusted, const LLSD& untrusted);
	S32 sendError(
		const LLHost& host,
		const LLUUID& agent_id,
		S32 code,
		const std::string& token,
		const LLUUID& id,
		const std::string& system,
		const std::string& message,
		const LLSD& data);
	bool checkAllMessages(S64 frame_count, LLPumpIO* http_pump);
	void clearReceiveState();
	void receivedMessageFromTrustedSender();
private:
	bool mLastMessageFromTrustedMessageService;
	typedef std::map<U32, LLUUID> code_session_map_t;
	code_session_map_t mCircuitCodes;
	LLUUID mSessionID;
	void	addTemplate(LLMessageTemplate *templatep);
	BOOL		decodeTemplate( const U8* buffer, S32 buffer_size, LLMessageTemplate** msg_template );
	void		logMsgFromInvalidCircuit( const LLHost& sender, BOOL recv_reliable );
	void		logTrustedMsgFromUntrustedCircuit( const LLHost& sender );
	void		logValidMsg(LLCircuitData *cdp, const LLHost& sender, BOOL recv_reliable, BOOL recv_resent, BOOL recv_acks );
	void		logRanOffEndOfPacket( const LLHost& sender );
	class LLMessageCountInfo
	{
	public:
		U32 mMessageNum;
		U32 mMessageBytes;
		BOOL mInvalid;
	};
	LLMessagePollInfo						*mPollInfop;
	U8	mEncodedRecvBuffer[MAX_BUFFER_SIZE];
	U8	mTrueReceiveBuffer[MAX_BUFFER_SIZE];
	S32	mTrueReceiveSize;
	BOOL	mbError;
	S32	mErrorCode;
	F64Seconds										mResendDumpTime;
	LLMessageCountInfo mMessageCountList[MAX_MESSAGE_COUNT_NUM];
	S32 mNumMessageCounts;
	F32Seconds mReceiveTime;
	F32Seconds mMaxMessageTime;
	S32 mMaxMessageCounts;
	F64Seconds mMessageCountTime;
	F64Seconds mCurrentMessageTime;
	typedef std::pair<msg_exception_callback, void*> exception_t;
	typedef std::map<EMessageException, exception_t> callbacks_t;
	callbacks_t mExceptionCallbacks;
	LLTimer mMessageSystemTimer;
	static F32 mTimeDecodesSpamThreshold;
	static BOOL mTimeDecodes;
	msg_timing_callback mTimingCallback;
	void* mTimingCallbackData;
	void init();
	LLHost mLastSender;
	LLHost mLastReceivingIF;
	S32 mIncomingCompressedSize;
	TPACKETID mCurrentRecvPacketID;
	LLMessageBuilder* mMessageBuilder;
	LLTemplateMessageBuilder* mTemplateMessageBuilder;
	LLSDMessageBuilder* mLLSDMessageBuilder;
	LLMessageReader* mMessageReader;
	LLTemplateMessageReader* mTemplateMessageReader;
	LLSDMessageReader* mLLSDMessageReader;
	friend class LLMessageHandlerBridge;
	bool callHandler(const char *name, bool trustedSource,
					 LLMessageSystem* msg);
	LLCircuitData* findCircuit(const LLHost& host, bool resetPacketId);
};
extern LLMessageSystem	*gMessageSystem;
bool start_messaging_system(
	const std::string& template_name,
	U32 port,
	S32 version_major,
	S32 version_minor,
	S32 version_patch,
	bool b_dump_prehash_file,
	const std::string& secret,
	const LLUseCircuitCodeResponder* responder,
	bool failure_is_fatal,
	const F32 circuit_heartbeat_interval,
	const F32 circuit_timeout);
void end_messaging_system(bool print_summary = true);
void null_message_callback(LLMessageSystem *msg, void **data);
#if !defined( LL_BIG_ENDIAN ) && !defined( LL_LITTLE_ENDIAN )
#error Unknown endianness for htonmemcpy. Did you miss a common include?
#endif
static inline void *htonmemcpy(void *vs, const void *vct, EMsgVariableType type, size_t n)
{
	char *s = (char *)vs;
	const char *ct = (const char *)vct;
#ifdef LL_BIG_ENDIAN
	S32 i, length;
#endif
	switch(type)
	{
	case MVT_FIXED:
	case MVT_VARIABLE:
	case MVT_U8:
	case MVT_S8:
	case MVT_BOOL:
	case MVT_LLUUID:
	case MVT_IP_ADDR:
	case MVT_IP_PORT:
		return(memcpy(s,ct,n));
	case MVT_U16:
	case MVT_S16:
		if (n != 2)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		*(s + 1) = *(ct);
		*(s) = *(ct + 1);
		return(vs);
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_U32:
	case MVT_S32:
	case MVT_F32:
		if (n != 4)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		*(s + 3) = *(ct);
		*(s + 2) = *(ct + 1);
		*(s + 1) = *(ct + 2);
		*(s) = *(ct + 3);
		return(vs);
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_U64:
	case MVT_S64:
	case MVT_F64:
		if (n != 8)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		*(s + 7) = *(ct);
		*(s + 6) = *(ct + 1);
		*(s + 5) = *(ct + 2);
		*(s + 4) = *(ct + 3);
		*(s + 3) = *(ct + 4);
		*(s + 2) = *(ct + 5);
		*(s + 1) = *(ct + 6);
		*(s) = *(ct + 7);
		return(vs);
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_LLVector3:
	case MVT_LLQuaternion:
		if (n != 12)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 8, ct + 8, MVT_F32, 4);
		htonmemcpy(s + 4, ct + 4, MVT_F32, 4);
		return(htonmemcpy(s, ct, MVT_F32, 4));
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_LLVector3d:
		if (n != 24)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 16, ct + 16, MVT_F64, 8);
		htonmemcpy(s + 8, ct + 8, MVT_F64, 8);
		return(htonmemcpy(s, ct, MVT_F64, 8));
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_LLVector4:
		if (n != 16)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 12, ct + 12, MVT_F32, 4);
		htonmemcpy(s + 8, ct + 8, MVT_F32, 4);
		htonmemcpy(s + 4, ct + 4, MVT_F32, 4);
		return(htonmemcpy(s, ct, MVT_F32, 4));
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_U16Vec3:
		if (n != 6)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 4, ct + 4, MVT_U16, 2);
		htonmemcpy(s + 2, ct + 2, MVT_U16, 2);
		return(htonmemcpy(s, ct, MVT_U16, 2));
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_U16Quat:
		if (n != 8)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 6, ct + 6, MVT_U16, 2);
		htonmemcpy(s + 4, ct + 4, MVT_U16, 2);
		htonmemcpy(s + 2, ct + 2, MVT_U16, 2);
		return(htonmemcpy(s, ct, MVT_U16, 2));
#else
		return(memcpy(s,ct,n));
#endif
	case MVT_S16Array:
		if (n % 2)
		{
			LL_ERRS() << "Size argument passed to htonmemcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		length = n % 2;
		for (i = 1; i < length; i++)
		{
			htonmemcpy(s + i*2, ct + i*2, MVT_S16, 2);
		}
		return(htonmemcpy(s, ct, MVT_S16, 2));
#else
		return(memcpy(s,ct,n));
#endif
	default:
		return(memcpy(s,ct,n));
	}
}
inline void *ntohmemcpy(void *s, const void *ct, EMsgVariableType type, size_t n)
{
	return(htonmemcpy(s,ct,type, n));
}
inline const LLHost& LLMessageSystem::getReceivingInterface() const {return mLastReceivingIF;}
inline U32 LLMessageSystem::getSenderIP() const
{
	return mLastSender.getAddress();
}
inline U32 LLMessageSystem::getSenderPort() const
{
	return mLastSender.getPort();
}
inline S32 LLMessageSystem::sendMessage(const U32 circuit)
{
	return sendMessage(findHost(circuit));
}
#endif
