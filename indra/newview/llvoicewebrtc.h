/**
 * @file llvoicewebrtc.h
 * @brief Declaration of LLWebRTCVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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
#ifndef LL_VOICE_WEBRTC_H
#define LL_VOICE_WEBRTC_H
class LLWebRTCProtocolParser;
#include "lliopipe.h"
#include "llpumpio.h"
#include "llchainio.h"
#include "lliosocket.h"
#include "v3math.h"
#include "llframetimer.h"
#include "llviewerregion.h"
#include "llcallingcard.h"
#include "lleventcoro.h"
#include "llcoros.h"
#include "llparcel.h"
#include "llmutelist.h"
#include "workqueue.h"
#include <queue>
#include "boost/json.hpp"
#ifdef LL_USESYSTEMLIBS
# include "expat.h"
#else
# include "expat/expat.h"
#endif
#include "llvoiceclient.h"
#include <llwebrtc.h>
class LLAvatarName;
class LLVoiceWebRTCConnection;
typedef std::shared_ptr<LLVoiceWebRTCConnection> connectionPtr_t;
extern const std::string WEBRTC_VOICE_SERVER_TYPE;
class LLWebRTCVoiceClient : public LLSingleton<LLWebRTCVoiceClient>,
                            virtual public LLVoiceModuleInterface,
                            public llwebrtc::LLWebRTCDevicesObserver,
                            public LLMuteListObserver,
                            public llwebrtc::LLWebRTCLogCallback
{
    LLSINGLETON(LLWebRTCVoiceClient);
    LOG_CLASS(LLWebRTCVoiceClient);
    virtual ~LLWebRTCVoiceClient();
public:
    void cleanupSingleton();
    void init(LLPumpIO *pump) override;
    void terminate() override;
    static bool isShuttingDown() { return sShuttingDown; }
    const LLVoiceVersionInfo& getVersion() override;
    void                      updateVersion();
    void updateSettings() override;
    bool isVoiceWorking() const override;
    LLSD getP2PChannelInfoTemplate(const LLUUID& id) const override;
    void setHidden(bool hidden) override;
    void LogMessage(llwebrtc::LLWebRTCLogCallback::LogLevel level, const std::string& message) override;
    void tuningStart() override;
    void tuningStop() override;
    bool inTuningMode() override;
    void tuningSetMicVolume(float volume) override;
    void tuningSetSpeakerVolume(float volume) override;
    float tuningGetEnergy(void) override;
    bool deviceSettingsAvailable() override;
    bool deviceSettingsUpdated() override;
    void refreshDeviceLists(bool clearCurrentList = true) override;
    void setCaptureDevice(const std::string& name) override;
    void setRenderDevice(const std::string& name) override;
    LLVoiceDeviceList& getCaptureDevices() override;
    LLVoiceDeviceList& getRenderDevices() override;
    void getParticipantList(uuid_set_t &participants) override;
    bool isParticipant(const LLUUID& speaker_id) override;
    bool isSessionCallBackPossible(const LLUUID &session_id) override;
    bool isSessionTextIMPossible(const LLUUID &session_id) override { return true; }
    bool inProximalChannel() override;
    void setNonSpatialChannel(const LLSD& channelInfo, bool notify_on_first_join, bool hangup_on_last_leave) override
    {
        startAdHocSession(channelInfo, notify_on_first_join, hangup_on_last_leave);
    }
    bool setSpatialChannel(const LLSD &channelInfo) override;
    void leaveNonSpatialChannel() override;
    void processChannels(bool process) override;
    void leaveChannel(bool stopTalking);
    bool isCurrentChannel(const LLSD &channelInfo) override;
    bool compareChannels(const LLSD &channelInfo1, const LLSD &channelInfo2) override;
    LLVoiceP2POutgoingCallInterface *getOutgoingCallInterface() override { return nullptr; }
    LLVoiceP2PIncomingCallInterfacePtr getIncomingCallInterface(const LLSD &voice_call_info) override { return nullptr; }
    void setVoiceVolume(F32 volume) override;
    void setMicGain(F32 volume) override;
    void setVoiceEnabled(bool enabled) override;
    void setMuteMic(bool muted) override;
    std::string getDisplayName(const LLUUID& id) override;
    bool isParticipantAvatar(const LLUUID &id) override;
    bool getIsSpeaking(const LLUUID& id) override;
    bool getIsModeratorMuted(const LLUUID& id) override;
    F32 getCurrentPower(const LLUUID& id) override;
    F32 getUserVolume(const LLUUID& id) override;
    void setUserVolume(const LLUUID& id, F32 volume) override;
    void onChange() override;
    void onChangeDetailed(const LLMute& ) override;
    void userAuthorized(const std::string &user_id, const LLUUID &agentID) override;
    void OnConnectionEstablished(const std::string& channelID, const LLUUID& regionID);
    void OnConnectionShutDown(const std::string &channelID, const LLUUID &regionID);
    void OnConnectionFailure(const std::string &channelID,
        const LLUUID &regionID,
        LLVoiceClientStatusObserver::EStatusType status_type = LLVoiceClientStatusObserver::ERROR_UNKNOWN);
    void updatePosition(void);
    void sendPositionUpdate(bool force);
    void updateOwnVolume();
    void addObserver(LLVoiceClientStatusObserver* observer) override;
    void removeObserver(LLVoiceClientStatusObserver* observer) override;
    void addObserver(LLFriendObserver* observer) override;
    void removeObserver(LLFriendObserver* observer) override;
    void addObserver(LLVoiceClientParticipantObserver* observer) override;
    void removeObserver(LLVoiceClientParticipantObserver* observer) override;
    void OnDevicesChanged(const llwebrtc::LLWebRTCVoiceDeviceList &render_devices,
                          const llwebrtc::LLWebRTCVoiceDeviceList &capture_devices) override;
    void OnDevicesChangedImpl(const llwebrtc::LLWebRTCVoiceDeviceList &render_devices,
                              const llwebrtc::LLWebRTCVoiceDeviceList &capture_devices);
    struct participantState
    {
    public:
        participantState(const LLUUID& agent_id, const LLUUID& region);
        bool isAvatar();
        std::string mURI;
        LLUUID mAvatarID;
        std::string mDisplayName;
        LLFrameTimer mSpeakingTimeout;
        F32 mLevel;
        F32 mVolume;
        bool mIsSpeaking;
        bool mIsModeratorMuted;
        LLUUID mRegion;
    };
    typedef std::shared_ptr<participantState> participantStatePtr_t;
    participantStatePtr_t findParticipantByID(const std::string &channelID, const LLUUID &id);
    participantStatePtr_t addParticipantByID(const std::string& channelID, const LLUUID &id, const LLUUID& region);
    void removeParticipantByID(const std::string& channelID, const LLUUID &id, const LLUUID& region);
  protected:
    typedef std::map<const LLUUID, participantStatePtr_t> participantUUIDMap;
    class sessionState
    {
    public:
        typedef std::shared_ptr<sessionState> ptr_t;
        typedef std::weak_ptr<sessionState> wptr_t;
        typedef std::function<void(const ptr_t &)> sessionFunc_t;
        static void addSession(const std::string &channelID, ptr_t& session);
        virtual ~sessionState();
        participantStatePtr_t addParticipant(const LLUUID& agent_id, const LLUUID& region);
        void removeParticipant(const participantStatePtr_t &participant);
        void removeAllParticipants(const LLUUID& region = LLUUID());
        participantStatePtr_t findParticipantByID(const LLUUID& id);
        static ptr_t matchSessionByChannelID(const std::string& channel_id);
        void shutdownAllConnections();
        void revive();
        const std::string getVersion() const;
        static void processSessionStates();
        virtual bool processConnectionStates();
        virtual void sendData(const std::string &data);
        void setMuteMic(bool muted);
        void setSpeakerVolume(F32 volume);
        void setUserVolume(const LLUUID& id, F32 volume);
        void setUserMute(const LLUUID& id, bool mute);
        static void for_each(sessionFunc_t func);
        static void reapEmptySessions();
        static void clearSessions();
        bool isEmpty() { return mWebRTCConnections.empty(); }
        virtual bool isSpatial() = 0;
        virtual bool isEstate()  = 0;
        virtual bool isCallbackPossible() = 0;
        std::string mHandle;
        std::string mChannelID;
        std::string mName;
        bool    mMuted;
        F32     mSpeakerVolume;
        bool        mShuttingDown;
        participantUUIDMap mParticipantsByUUID;
        static bool hasSession(const std::string &sessionID)
        { return sSessions.find(sessionID) != sSessions.end(); }
       bool mHangupOnLastLeave;
       bool mNotifyOnFirstJoin;
    protected:
        sessionState();
        std::list<connectionPtr_t> mWebRTCConnections;
    private:
        static std::map<std::string, ptr_t> sSessions;
        static void for_eachPredicate(const std::pair<std::string,
                                      LLWebRTCVoiceClient::sessionState::wptr_t> &a,
                                      sessionFunc_t func);
    };
    typedef std::shared_ptr<sessionState> sessionStatePtr_t;
    typedef std::map<std::string, sessionStatePtr_t> sessionMap;
    class estateSessionState : public sessionState
    {
      public:
        estateSessionState();
        bool processConnectionStates() override;
        bool isSpatial() override { return true; }
        bool isEstate() override { return true; }
        bool isCallbackPossible() override { return false; }
      private:
        bool isRegionWebRTCEnabled(const LLUUID& regionID);
    };
    class parcelSessionState : public sessionState
    {
      public:
        parcelSessionState(const std::string& channelID, S32 parcel_local_id);
        bool isSpatial() override { return true; }
        bool isEstate() override { return false; }
        bool isCallbackPossible() override { return false; }
    };
    class adhocSessionState : public sessionState
    {
    public:
        adhocSessionState(const std::string &channelID,
            const std::string& credentials,
            bool notify_on_first_join,
            bool hangup_on_last_leave);
        bool isSpatial() override { return false; }
        bool isEstate() override { return false; }
        bool isCallbackPossible() override { return mNotifyOnFirstJoin && mHangupOnLastLeave; }
        void sendData(const std::string &data) override { }
    protected:
        std::string mCredentials;
    };
    static void predSendData(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const std::string& spatial_data);
    static void predUpdateOwnVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 audio_level);
    static void predSetMuteMic(const LLWebRTCVoiceClient::sessionStatePtr_t &session, bool mute);
    static void predSetSpeakerVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 volume);
    static void predShutdownSession(const LLWebRTCVoiceClient::sessionStatePtr_t &session);
    static void predSetUserMute(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const LLUUID& id, bool mute);
    static void predSetUserVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const LLUUID& id, F32 volume);
    void clearCaptureDevices();
    void addCaptureDevice(const LLVoiceDevice& device);
    void clearRenderDevices();
    void addRenderDevice(const LLVoiceDevice& device);
    void setDevicesListUpdated(bool state);
    void setListenerPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot);
    void setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot);
    LLVector3d getListenerPosition() { return mListenerPosition; }
    LLVector3d getSpeakerPosition() { return mAvatarPosition; }
    void setEarLocation(S32 loc);
    sessionStatePtr_t findP2PSession(const LLUUID &agent_id);
    sessionStatePtr_t addSession(const std::string &channel_id, sessionState::ptr_t session);
    void deleteSession(const sessionStatePtr_t &session);
    void leaveAudioSession();
    friend class LLWebRTCVoiceClientCapResponder;
    void lookupName(const LLUUID &id);
    void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
    void avatarNameResolved(const LLUUID &id, const std::string &name);
    static void predAvatarNameResolution(const LLWebRTCVoiceClient::sessionStatePtr_t &session, LLUUID id, std::string name);
    boost::signals2::connection mAvatarNameCacheConnection;
private:
    void initWebRTC();
    void voiceConnectionCoro();
    void cleanUp();
    LL::WorkQueue::weak_t mMainQueue;
    F32 mTuningMicGain;
    int mTuningSpeakerVolume;
    bool mDevicesListUpdated;
    std::string mSpatialSessionCredentials;
    std::string mMainSessionGroupHandle;
    sessionStatePtr_t mSession;
    sessionStatePtr_t mNextSession;
    llwebrtc::LLWebRTCDeviceInterface *mWebRTCDeviceInterface;
    LLVoiceDeviceList mCaptureDevices;
    LLVoiceDeviceList mRenderDevices;
    bool startEstateSession();
    bool startParcelSession(const std::string& channelID, S32 parcelID);
    bool startAdHocSession(const LLSD &channelInfo, bool notify_on_first_join, bool hangup_on_last_leave);
    bool inSpatialChannel();
    bool inOrJoiningChannel(const std::string &channelID);
    bool inEstateChannel();
    LLSD getAudioSessionChannelInfo();
    void enforceTether();
    void updateNeighboringRegions();
    std::set<LLUUID> getNeighboringRegions() { return mNeighboringRegions; }
    LLVoiceVersionInfo mVoiceVersion;
    bool         mSpatialCoordsDirty;
    LLVector3d   mListenerPosition;
    LLVector3d   mListenerRequestedPosition;
    LLVector3    mListenerVelocity;
    LLQuaternion mListenerRot;
    LLVector3d   mAvatarPosition;
    LLVector3    mAvatarVelocity;
    LLQuaternion mAvatarRot;
    std::set<LLUUID> mNeighboringRegions;
    bool         mMuteMic;
    bool         mHidden;
    enum
    {
        earLocCamera = 0,
        earLocAvatar,
        earLocMixed
    };
    S32   mEarLocation;
    float mSpeakerVolume;
    F32   mMicGain;
    bool  mVoiceEnabled;
    bool  mProcessChannels;
    typedef std::set<LLVoiceClientParticipantObserver*> observer_set_t;
    observer_set_t mParticipantObservers;
    void notifyParticipantObservers();
    typedef std::set<LLVoiceClientStatusObserver*> status_observer_set_t;
    status_observer_set_t mStatusObservers;
    void notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status);
    bool    mIsInTuningMode;
    bool    mIsProcessingChannels;
    bool    mIsCoroutineActive;
    static bool sShuttingDown;
    std::string mWebRTCPump;
    LLSD mLastWebRTCStats;
};
class LLVoiceWebRTCStats : public LLSingleton<LLVoiceWebRTCStats>
{
    LLSINGLETON(LLVoiceWebRTCStats);
    LOG_CLASS(LLVoiceWebRTCStats);
    virtual ~LLVoiceWebRTCStats();
  private:
    F64SecondsImplicit mStartTime;
    U32 mConnectCycles;
    F64 mConnectTime;
    U32 mConnectAttempts;
    F64 mProvisionTime;
    U32 mProvisionAttempts;
    F64 mEstablishTime;
    U32 mEstablishAttempts;
  public:
    void reset();
    void connectionAttemptStart();
    void connectionAttemptEnd(bool success);
    void provisionAttemptStart();
    void provisionAttemptEnd(bool success);
    void establishAttemptStart();
    void establishAttemptEnd(bool success);
    LLSD read();
};
class LLVoiceWebRTCConnection :
    public llwebrtc::LLWebRTCSignalingObserver,
    public llwebrtc::LLWebRTCDataObserver,
    public std::enable_shared_from_this<LLVoiceWebRTCConnection>
{
  public:
    LLVoiceWebRTCConnection(const LLUUID &regionID, const std::string &channelID);
    virtual ~LLVoiceWebRTCConnection() = 0;
    void OnIceGatheringState(EIceGatheringState state) override;
    void OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate &candidate) override;
    void OnOfferAvailable(const std::string &sdp) override;
    void OnRenegotiationNeeded() override;
    void OnPeerConnectionClosed() override;
    void OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface *audio_interface) override;
    void OnDataReceived(const std::string &data, bool binary) override;
    void OnDataChannelReady(llwebrtc::LLWebRTCDataInterface *data_interface) override;
    void OnStatsDelivered(const llwebrtc::LLWebRTCStatsMap& stats_data) override;
    void OnDataReceivedImpl(const std::string &data, bool binary);
    void sendJoin();
    void sendData(const std::string &data);
    const std::string& getVersion();
    void processIceUpdates();
    static void processIceUpdatesCoro(connectionPtr_t connection);
    virtual void setMuteMic(bool muted);
    virtual void setSpeakerVolume(F32 volume);
    void setUserVolume(const LLUUID& id, F32 volume);
    void setUserMute(const LLUUID& id, bool mute);
    bool connectionStateMachine();
    virtual bool isSpatial() { return false; }
    bool         isPrimary() const { return mPrimary; }
    LLUUID getRegionID() { return mRegionID; }
    void shutDown()
    {
        mShutDown = true;
    }
    bool isShuttingDown()
    {
        return mShutDown;
    }
    void OnVoiceConnectionRequestSuccess(const LLSD &body);
    void resetConnectionStats();
  protected:
    typedef enum e_voice_connection_state
    {
        VOICE_STATE_ERROR                  = 0x0,
        VOICE_STATE_START_SESSION          = 0x1,
        VOICE_STATE_WAIT_FOR_SESSION_START = 0x2,
        VOICE_STATE_REQUEST_CONNECTION     = 0x4,
        VOICE_STATE_CONNECTION_WAIT        = 0x8,
        VOICE_STATE_SESSION_ESTABLISHED    = 0x10,
        VOICE_STATE_WAIT_FOR_DATA_CHANNEL  = 0x20,
        VOICE_STATE_SESSION_UP             = 0x40,
        VOICE_STATE_SESSION_RETRY          = 0x80,
        VOICE_STATE_DISCONNECT             = 0x100,
        VOICE_STATE_WAIT_FOR_EXIT          = 0x200,
        VOICE_STATE_SESSION_EXIT           = 0x400,
        VOICE_STATE_WAIT_FOR_CLOSE         = 0x800,
        VOICE_STATE_CLOSED                 = 0x1000,
        VOICE_STATE_SESSION_STOPPING       = 0x1F80
    } EVoiceConnectionState;
    EVoiceConnectionState mVoiceConnectionState;
    LL::WorkQueue::weak_t mMainQueue;
    void setVoiceConnectionState(EVoiceConnectionState new_voice_connection_state)
    {
        if (new_voice_connection_state & VOICE_STATE_SESSION_STOPPING)
        {
            mVoiceConnectionState = new_voice_connection_state;
            return;
        }
        if (mVoiceConnectionState & VOICE_STATE_SESSION_STOPPING)
        {
            return;
        }
        mVoiceConnectionState = new_voice_connection_state;
    }
    EVoiceConnectionState getVoiceConnectionState()
    {
        return mVoiceConnectionState;
    }
    virtual void requestVoiceConnection() = 0;
    static void requestVoiceConnectionCoro(connectionPtr_t connection) { connection->requestVoiceConnection(); }
    static void breakVoiceConnectionCoro(connectionPtr_t connection);
    LLVoiceClientStatusObserver::EStatusType mCurrentStatus;
    LLUUID mRegionID;
    bool   mPrimary;
    LLUUID mViewerSession;
    std::string mChannelID;
    std::string mServerVersion;
    std::string mChannelSDP;
    std::string mRemoteChannelSDP;
    bool mMuted;
    F32  mSpeakerVolume;
    bool mShutDown;
    S32  mOutstandingRequests;
    S32  mRetryWaitPeriod;
    F32  mRetryWaitSecs;
    std::vector<llwebrtc::LLWebRTCIceCandidate> mIceCandidates;
    bool                                        mIceCompleted;
    llwebrtc::LLWebRTCPeerConnectionInterface *mWebRTCPeerConnectionInterface;
    llwebrtc::LLWebRTCAudioInterface *mWebRTCAudioInterface;
    llwebrtc::LLWebRTCDataInterface  *mWebRTCDataInterface;
};
class LLVoiceWebRTCSpatialConnection :
    public LLVoiceWebRTCConnection
{
  public:
    LLVoiceWebRTCSpatialConnection(const LLUUID &regionID, S32 parcelLocalID, const std::string &channelID);
    virtual ~LLVoiceWebRTCSpatialConnection();
    void setMuteMic(bool muted) override;
    bool isSpatial() override { return true; }
protected:
    void requestVoiceConnection() override;
    S32    mParcelLocalID;
};
class LLVoiceWebRTCAdHocConnection : public LLVoiceWebRTCConnection
{
  public:
    LLVoiceWebRTCAdHocConnection(const LLUUID &regionID, const std::string &channelID, const std::string& credentials);
    virtual ~LLVoiceWebRTCAdHocConnection();
    bool isSpatial() override { return false; }
  protected:
    void requestVoiceConnection() override;
    std::string mCredentials;
};
#define VOICE_ELAPSED LLVoiceTimer(__FUNCTION__);
#endif
