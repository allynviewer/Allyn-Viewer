/**
 * @file llvoicewebrtc.cpp
 * @brief Implementation of LLWebRTCVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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
#include "workqueue.h"
#include "llcorehttputil.h"
#include "llcallbacklist.h"
#ifndef LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE
#define LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE
#endif
#ifndef LL_PROFILE_ZONE_NAMED_CATEGORY_VOICE
#define LL_PROFILE_ZONE_NAMED_CATEGORY_VOICE(x)
#endif
#include <algorithm>
#include "llvoicewebrtc.h"
#include "llsdutil.h"
#include "llavatarnamecache.h"
#include "llvoavatarself.h"
#include "llbufferstream.h"
#include "llfile.h"
#include "llmenugl.h"
#ifdef LL_USESYSTEMLIBS
# include "expat.h"
#else
# include "expat/expat.h"
#endif
#include "llcallbacklist.h"
#include "llbase64.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llmutelist.h"
#include "llagent.h"
#include "llcachename.h"
#include "llimview.h"
#include "llworld.h"
#include "llviewerregion.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "llfirstuse.h"
#include "llspeakers.h"
#include "lltrans.h"
#include "llrand.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llviewerstats.h"
#include "llversioninfo.h"
#include "llnotificationsutil.h"
#include "llnearbyvoicemoderation.h"
#include "llcorehttputil.h"
#include "lleventfilter.h"
#include "stringize.h"
#include "llwebrtc.h"
#include "apr_base64.h"
#include "boost/json.hpp"
const std::string WEBRTC_VOICE_SERVER_TYPE = "webrtc";
const F32 STATS_TIMER_DELAY = 2.0;
namespace {
    const F32      MAX_AUDIO_DIST           = 50.0f;
    const F32      VOLUME_SCALE_WEBRTC      = 0.01f;
    const F32      TUNING_LEVEL_SCALE       = 0.01f;
    const F32      TUNING_LEVEL_START_POINT = 0.8f;
    const F32      LEVEL_SCALE              = 0.005f;
    const F32      LEVEL_START_POINT        = 0.18f;
    const uint32_t SET_HIDDEN_RESTORE_DELAY_MS = 200;
    const uint32_t MUTE_FADE_DELAY_MS       = 500;
    const F32 SPEAKING_AUDIO_LEVEL = 0.30;
    const uint32_t PEER_GAIN_CONVERSION_FACTOR = 220;
    static const std::string REPORTED_VOICE_SERVER_TYPE = "Secondlife WebRTC Gateway";
    const F32 UPDATE_THROTTLE_SECONDS = 0.1f;
    const F32 MAX_RETRY_WAIT_SECONDS  = 10.0f;
    const F32 FOUR_DEGREES = 4.0f * (F_PI / 180.0f);
    const F32 MINUSCULE_ANGLE_COS = (F32) cos(0.5f * FOUR_DEGREES);
}
void LLVoiceWebRTCStats::reset()
{
    mStartTime = -1.0f;
    mConnectCycles = 0;
    mConnectTime = -1.0f;
    mConnectAttempts = 0;
    mProvisionTime = -1.0f;
    mProvisionAttempts = 0;
    mEstablishTime = -1.0f;
    mEstablishAttempts = 0;
}
LLVoiceWebRTCStats::LLVoiceWebRTCStats()
{
    reset();
}
LLVoiceWebRTCStats::~LLVoiceWebRTCStats()
{
}
void LLVoiceWebRTCStats::connectionAttemptStart()
{
    if (!mConnectAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
        mConnectCycles++;
    }
    mConnectAttempts++;
}
void LLVoiceWebRTCStats::connectionAttemptEnd(bool success)
{
    if ( success )
    {
        mConnectTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}
void LLVoiceWebRTCStats::provisionAttemptStart()
{
    if (!mProvisionAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
    }
    mProvisionAttempts++;
}
void LLVoiceWebRTCStats::provisionAttemptEnd(bool success)
{
    if ( success )
    {
        mProvisionTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}
void LLVoiceWebRTCStats::establishAttemptStart()
{
    if (!mEstablishAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
    }
    mEstablishAttempts++;
}
void LLVoiceWebRTCStats::establishAttemptEnd(bool success)
{
    if ( success )
    {
        mEstablishTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}
LLSD LLVoiceWebRTCStats::read()
{
    LLSD stats(LLSD::emptyMap());
    stats["connect_cycles"] = LLSD::Integer(mConnectCycles);
    stats["connect_attempts"] = LLSD::Integer(mConnectAttempts);
    stats["connect_time"] = LLSD::Real(mConnectTime);
    stats["provision_attempts"] = LLSD::Integer(mProvisionAttempts);
    stats["provision_time"] = LLSD::Real(mProvisionTime);
    stats["establish_attempts"] = LLSD::Integer(mEstablishAttempts);
    stats["establish_time"] = LLSD::Real(mEstablishTime);
    return stats;
}
bool LLWebRTCVoiceClient::sShuttingDown = false;
LLWebRTCVoiceClient::LLWebRTCVoiceClient() :
    mHidden(false),
    mTuningMicGain(0.0),
    mTuningSpeakerVolume(50),
    mDevicesListUpdated(false),
    mSpatialCoordsDirty(false),
    mMuteMic(false),
    mEarLocation(0),
    mMicGain(0.0),
    mVoiceEnabled(false),
    mProcessChannels(false),
    mAvatarNameCacheConnection(),
    mIsInTuningMode(false),
    mIsProcessingChannels(false),
    mIsCoroutineActive(false),
    mWebRTCPump("WebRTCClientPump"),
    mWebRTCDeviceInterface(nullptr)
{
    sShuttingDown = false;
    mSpeakerVolume = 0.0;
    mVoiceVersion.serverVersion = "";
    mVoiceVersion.voiceServerType = REPORTED_VOICE_SERVER_TYPE;
    mVoiceVersion.internalVoiceServerType = WEBRTC_VOICE_SERVER_TYPE;
    mVoiceVersion.minorVersion = 0;
    mVoiceVersion.majorVersion = 2;
    mVoiceVersion.mBuildVersion = "";
}
LLWebRTCVoiceClient::~LLWebRTCVoiceClient()
{
}
void LLWebRTCVoiceClient::cleanupSingleton()
{
    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
    sShuttingDown = true;
    if (mSession)
    {
        mSession->shutdownAllConnections();
    }
    if (mNextSession)
    {
        mNextSession->shutdownAllConnections();
    }
    cleanUp();
    sessionState::clearSessions();
    mStatusObservers.clear();
}
void LLWebRTCVoiceClient::init(LLPumpIO* pump)
{
    initWebRTC();
}
void LLWebRTCVoiceClient::initWebRTC()
{
    llwebrtc::init(this);
    mWebRTCDeviceInterface = llwebrtc::getDeviceInterface();
    mWebRTCDeviceInterface->setDevicesObserver(this);
    mMainQueue = LL::WorkQueue::getInstance("mainloop");
    refreshDeviceLists();
}
void LLWebRTCVoiceClient::terminate()
{
    if (sShuttingDown)
    {
        return;
    }
    LL_INFOS("Voice") << "Terminating WebRTC" << LL_ENDL;
    mVoiceEnabled = false;
    sShuttingDown = true;
    llwebrtc::terminate();
    mWebRTCDeviceInterface = nullptr;
}
void LLWebRTCVoiceClient::cleanUp()
{
    mNextSession.reset();
    mSession.reset();
    mNeighboringRegions.clear();
    sessionState::for_each(boost::bind(predShutdownSession, _1));
    LL_DEBUGS("Voice") << "Exiting" << LL_ENDL;
}
void LLWebRTCVoiceClient::LogMessage(llwebrtc::LLWebRTCLogCallback::LogLevel level, const std::string& message)
{
    switch (level)
    {
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_VERBOSE:
        LL_DEBUGS("Voice") << message << LL_ENDL;
        break;
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_INFO:
        LL_INFOS("Voice") << message << LL_ENDL;
        break;
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_WARNING:
        LL_WARNS("Voice") << message << LL_ENDL;
        break;
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_ERROR:
        LL_WARNS("Voice") << message << LL_ENDL;
        break;
    default:
        break;
    }
}
const LLVoiceVersionInfo& LLWebRTCVoiceClient::getVersion()
{
    return mVoiceVersion;
}
void LLWebRTCVoiceClient::updateVersion()
{
    sessionStatePtr_t session = mNextSession.get() ? mNextSession : mSession;
    if (session)
    {
        mVoiceVersion.serverVersion = session->getVersion();
        if (dynamic_cast<adhocSessionState*>(session.get()))
        {
            if (session->mHangupOnLastLeave)
            {
                mVoiceVersion.mBuildVersion = "p2p";
            }
            else
            {
                mVoiceVersion.mBuildVersion = "ad-hoc";
            }
        }
        else if (session->isEstate())
        {
            mVoiceVersion.mBuildVersion = "estate";
        }
        else if (session->isSpatial())
        {
            mVoiceVersion.mBuildVersion = "parcel";
        }
        else
        {
            mVoiceVersion.mBuildVersion = mVoiceVersion.serverVersion;
        }
    }
    else
    {
        mVoiceVersion.serverVersion = mVoiceVersion.mBuildVersion = "";
    }
}
void LLWebRTCVoiceClient::updateSettings()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    setVoiceEnabled(LLVoiceClient::getInstance()->voiceEnabled());
    if (mVoiceEnabled)
    {
        static LLCachedControl<S32> sVoiceEarLocation(gSavedSettings, "VoiceEarLocation");
        setEarLocation(sVoiceEarLocation);
        static LLCachedControl<std::string> sInputDevice(gSavedSettings, "VoiceInputAudioDevice");
        setCaptureDevice(sInputDevice);
        static LLCachedControl<std::string> sOutputDevice(gSavedSettings, "VoiceOutputAudioDevice");
        setRenderDevice(sOutputDevice);
        LL_INFOS("Voice") << "Input device: " << std::quoted(sInputDevice()) << ", output device: " << std::quoted(sOutputDevice())
                            << LL_ENDL;
        static LLCachedControl<F32> sMicLevel(gSavedSettings, "AudioLevelMic");
        setMicGain(sMicLevel);
        llwebrtc::LLWebRTCDeviceInterface::AudioConfig config;
        bool audioConfigChanged = false;
        static LLCachedControl<bool> sEchoCancellation(gSavedSettings, "VoiceEchoCancellation", true);
        if (sEchoCancellation != config.mEchoCancellation)
        {
            config.mEchoCancellation = sEchoCancellation;
            audioConfigChanged       = true;
        }
        static LLCachedControl<bool> sAGC(gSavedSettings, "VoiceAutomaticGainControl", true);
        if (sAGC != config.mAGC)
        {
            config.mAGC        = sAGC;
            audioConfigChanged = true;
        }
        static LLCachedControl<U32> sNoiseSuppressionLevel(
            gSavedSettings,
            "VoiceNoiseSuppressionLevel",
            llwebrtc::LLWebRTCDeviceInterface::AudioConfig::ENoiseSuppressionLevel::NOISE_SUPPRESSION_LEVEL_VERY_HIGH);
        auto noiseSuppressionLevel =
            (llwebrtc::LLWebRTCDeviceInterface::AudioConfig::ENoiseSuppressionLevel)(U32)sNoiseSuppressionLevel;
        if (noiseSuppressionLevel != config.mNoiseSuppressionLevel)
        {
            config.mNoiseSuppressionLevel = noiseSuppressionLevel;
            audioConfigChanged            = true;
        }
        if (audioConfigChanged && mWebRTCDeviceInterface)
        {
            mWebRTCDeviceInterface->setAudioConfig(config);
        }
    }
}
void LLWebRTCVoiceClient::addObserver(LLVoiceClientParticipantObserver *observer)
{
    mParticipantObservers.insert(observer);
}
void LLWebRTCVoiceClient::removeObserver(LLVoiceClientParticipantObserver *observer)
{
    mParticipantObservers.erase(observer);
}
void LLWebRTCVoiceClient::notifyParticipantObservers()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    for (observer_set_t::iterator it = mParticipantObservers.begin(); it != mParticipantObservers.end();)
    {
        LLVoiceClientParticipantObserver *observer = *it;
        observer->onParticipantsChanged();
        it = mParticipantObservers.upper_bound(observer);
    }
}
void LLWebRTCVoiceClient::addObserver(LLVoiceClientStatusObserver *observer)
{
    mStatusObservers.insert(observer);
}
void LLWebRTCVoiceClient::removeObserver(LLVoiceClientStatusObserver *observer)
{
    mStatusObservers.erase(observer);
}
void LLWebRTCVoiceClient::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    LL_DEBUGS("Voice") << "( " << LLVoiceClientStatusObserver::status2string(status) << " )"
                       << " mSession=" << mSession << LL_ENDL;
    bool in_spatial_channel = inSpatialChannel();
    LL_DEBUGS("Voice") << " " << LLVoiceClientStatusObserver::status2string(status) << ", session channelInfo "
                       << getAudioSessionChannelInfo() << ", proximal is " << in_spatial_channel << LL_ENDL;
    mIsProcessingChannels = status == LLVoiceClientStatusObserver::STATUS_JOINED;
    LLSD channelInfo = getAudioSessionChannelInfo();
    for (status_observer_set_t::iterator it = mStatusObservers.begin(); it != mStatusObservers.end();)
    {
        LLVoiceClientStatusObserver *observer = *it;
        observer->onChange(status, channelInfo, in_spatial_channel);
        it = mStatusObservers.upper_bound(observer);
    }
    if (status != LLVoiceClientStatusObserver::STATUS_JOINING &&
        status != LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL &&
        status != LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED)
    {
        bool voice_status = LLVoiceClient::getInstance()->voiceEnabled() && mIsProcessingChannels;
        gAgent.setVoiceConnected(voice_status);
        if (voice_status)
        {
            doOnIdleOneTime([](){ });
        }
    }
}
void LLWebRTCVoiceClient::addObserver(LLFriendObserver *observer)
{
}
void LLWebRTCVoiceClient::removeObserver(LLFriendObserver *observer)
{
}
void LLWebRTCVoiceClient::voiceConnectionCoro()
{
    LL_INFOS("Voice") << "voiceConnectionCoro starting" << LL_ENDL;
    mIsCoroutineActive = true;
    LLCoros::set_consuming(true);
    try
    {
        LLMuteList::getInstance()->addObserver(this);
        U32 loop_count = 0;
        while (!sShuttingDown)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VOICE("voiceConnectionCoroLoop")
            llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);
            if (sShuttingDown) return;
            bool voiceEnabled = mVoiceEnabled;
            ++loop_count;
            if (!isAgentAvatarValid())
            {
                if ((loop_count % 50) == 1)
                {
                    LL_INFOS("Voice") << "coro waiting: agent avatar not valid yet" << LL_ENDL;
                }
                continue;
            }
            LLViewerRegion *regionp = gAgent.getRegion();
            if (!regionp)
            {
                if ((loop_count % 50) == 1)
                {
                    LL_INFOS("Voice") << "coro waiting: no agent region" << LL_ENDL;
                }
                continue;
            }
            if (!mProcessChannels)
            {
                leaveChannel(false);
            }
            else if (inSpatialChannel())
            {
                bool useEstateVoice = true;
                if (!regionp || regionp->getRegionID().isNull())
                {
                    continue;
                }
                const bool region_voice = regionp->isVoiceEnabled();
                voiceEnabled = voiceEnabled && region_voice;
                if (voiceEnabled)
                {
                    LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
                    if (parcel && parcel->getLocalID() != INVALID_PARCEL_ID)
                    {
                        if (!parcel->getParcelFlagAllowVoice())
                        {
                            if ((loop_count % 50) == 1)
                            {
                                LL_INFOS("Voice") << "parcel disallows voice; local_id="
                                                  << parcel->getLocalID() << LL_ENDL;
                            }
                            voiceEnabled = false;
                        }
                        else if (!parcel->getParcelFlagUseEstateVoiceChannel())
                        {
                            S32         parcel_local_id = parcel->getLocalID();
                            std::string channelID       = regionp->getRegionID().asString() + "-" + std::to_string(parcel->getLocalID());
                            useEstateVoice = false;
                            if (!inOrJoiningChannel(channelID))
                            {
                                startParcelSession(channelID, parcel_local_id);
                            }
                        }
                    }
                    if (voiceEnabled && useEstateVoice && !inEstateChannel())
                    {
                        startEstateSession();
                    }
                }
                else if ((loop_count % 50) == 1)
                {
                    LL_INFOS("Voice") << "spatial voice not starting: mVoiceEnabled="
                                      << (mVoiceEnabled ? "1" : "0")
                                      << " regionVoice=" << (region_voice ? "1" : "0")
                                      << " regionFlags=" << regionp->getRegionFlags() << LL_ENDL;
                }
                if (!voiceEnabled)
                {
                    leaveChannel(true);
                }
                else
                {
                    updatePosition();
                }
            }
            LL::WorkQueue::postMaybe(mMainQueue,
                [=] {
                    if  (sShuttingDown)
                    {
                        return;
                    }
                    sessionState::processSessionStates();
                    if (mProcessChannels && voiceEnabled && !mHidden)
                    {
                        sendPositionUpdate(false);
                        updateOwnVolume();
                    }
            });
        }
    }
    catch (const LLCoros::Stop&)
    {
        LL_DEBUGS("LLWebRTCVoiceClient") << "Received a shutdown exception" << LL_ENDL;
    }
    catch (const LLContinueError&)
    {
        LOG_UNHANDLED_EXCEPTION("LLWebRTCVoiceClient");
    }
    catch (...)
    {
        LL_WARNS("Voice") << "voiceConnectionStateMachine crashed" << LL_ENDL;
        throw;
    }
    cleanUp();
}
void LLWebRTCVoiceClient::updateNeighboringRegions()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    static const std::vector<LLVector3d> neighbors {LLVector3d(0.0f, 1.0f, 0.0f),  LLVector3d(0.707f, 0.707f, 0.0f),
                                                    LLVector3d(1.0f, 0.0f, 0.0f),  LLVector3d(0.707f, -0.707f, 0.0f),
                                                    LLVector3d(0.0f, -1.0f, 0.0f), LLVector3d(-0.707f, -0.707f, 0.0f),
                                                    LLVector3d(-1.0f, 0.0f, 0.0f), LLVector3d(-0.707f, 0.707f, 0.0f)};
    mNeighboringRegions.clear();
    mNeighboringRegions.insert(gAgent.getRegion()->getRegionID());
    LLVector3d speaker_pos = LLWebRTCVoiceClient::getInstance()->getSpeakerPosition();
    for (auto &neighbor_pos : neighbors)
    {
        LLViewerRegion *neighbor = LLWorld::instance().getRegionFromPosGlobal(speaker_pos + 2 * MAX_AUDIO_DIST * neighbor_pos);
        if (neighbor && !neighbor->getRegionID().isNull())
        {
            mNeighboringRegions.insert(neighbor->getRegionID());
        }
    }
}
void LLWebRTCVoiceClient::leaveAudioSession()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if(mSession)
    {
        LL_DEBUGS("Voice") << "leaving session: " << mSession->mChannelID << LL_ENDL;
        mSession->shutdownAllConnections();
    }
    else
    {
        LL_WARNS("Voice") << "called with no active session" << LL_ENDL;
    }
}
void LLWebRTCVoiceClient::clearCaptureDevices()
{
    LL_DEBUGS("Voice") << "called" << LL_ENDL;
    mCaptureDevices.clear();
}
void LLWebRTCVoiceClient::addCaptureDevice(const LLVoiceDevice& device)
{
    LL_INFOS("Voice") << "Voice Capture Device: '" << device.display_name << "' (" << device.full_name << ")" << LL_ENDL;
    mCaptureDevices.push_back(device);
}
LLVoiceDeviceList& LLWebRTCVoiceClient::getCaptureDevices()
{
    return mCaptureDevices;
}
void LLWebRTCVoiceClient::setCaptureDevice(const std::string& name)
{
    if (mWebRTCDeviceInterface)
    {
        LL_DEBUGS("Voice") << "new capture device is " << name << LL_ENDL;
        mWebRTCDeviceInterface->setCaptureDevice(name);
    }
}
void LLWebRTCVoiceClient::setDevicesListUpdated(bool state)
{
    mDevicesListUpdated = state;
}
void LLWebRTCVoiceClient::OnDevicesChanged(const llwebrtc::LLWebRTCVoiceDeviceList& render_devices,
                                           const llwebrtc::LLWebRTCVoiceDeviceList& capture_devices)
{
    LL::WorkQueue::postMaybe(mMainQueue,
                             [=]
        {
            OnDevicesChangedImpl(render_devices, capture_devices);
        });
}
void LLWebRTCVoiceClient::OnDevicesChangedImpl(const llwebrtc::LLWebRTCVoiceDeviceList &render_devices,
                                               const llwebrtc::LLWebRTCVoiceDeviceList &capture_devices)
{
    if (sShuttingDown)
    {
        return;
    }
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    LL_DEBUGS("Voice") << "Reiniting " << LL_ENDL;
    std::string inputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
    std::string outputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
    LL_DEBUGS("Voice") << "Setting devices to-input: '" << inputDevice << "' output: '" << outputDevice << "'" << LL_ENDL;
    if (mRenderDevices.size() != render_devices.size() || !std::equal(mRenderDevices.begin(),
                    mRenderDevices.end(),
                    render_devices.begin(),
                    [](const LLVoiceDevice& a, const llwebrtc::LLWebRTCVoiceDevice& b) {
            return a.display_name == b.mDisplayName && a.full_name == b.mID; }))
    {
        clearRenderDevices();
        for (auto& device : render_devices)
        {
            addRenderDevice(LLVoiceDevice(device.mDisplayName, device.mID));
        }
        setRenderDevice(outputDevice);
    }
    if (mCaptureDevices.size() != capture_devices.size() ||!std::equal(mCaptureDevices.begin(),
                    mCaptureDevices.end(),
                    capture_devices.begin(),
                    [](const LLVoiceDevice& a, const llwebrtc::LLWebRTCVoiceDevice& b)
                    { return a.display_name == b.mDisplayName && a.full_name == b.mID; }))
    {
        clearCaptureDevices();
        for (auto& device : capture_devices)
        {
            LL_DEBUGS("Voice") << "Checking capture device:'" << device.mID << "'" << LL_ENDL;
            addCaptureDevice(LLVoiceDevice(device.mDisplayName, device.mID));
        }
        setCaptureDevice(inputDevice);
    }
    setDevicesListUpdated(true);
}
void LLWebRTCVoiceClient::clearRenderDevices()
{
    LL_DEBUGS("Voice") << "called" << LL_ENDL;
    mRenderDevices.clear();
}
void LLWebRTCVoiceClient::addRenderDevice(const LLVoiceDevice& device)
{
    LL_INFOS("Voice") << "Voice Render Device: '" << device.display_name << "' (" << device.full_name << ")" << LL_ENDL;
    mRenderDevices.push_back(device);
}
LLVoiceDeviceList& LLWebRTCVoiceClient::getRenderDevices()
{
    return mRenderDevices;
}
void LLWebRTCVoiceClient::setRenderDevice(const std::string& name)
{
    if (mWebRTCDeviceInterface)
    {
        LL_DEBUGS("Voice") << "new render device is " << name << LL_ENDL;
        mWebRTCDeviceInterface->setRenderDevice(name);
    }
}
void LLWebRTCVoiceClient::tuningStart()
{
    if (!mIsInTuningMode)
    {
        if (mWebRTCDeviceInterface)
        {
            mWebRTCDeviceInterface->setTuningMode(true);
        }
        mIsInTuningMode = true;
    }
}
void LLWebRTCVoiceClient::tuningStop()
{
    if (mIsInTuningMode)
    {
        if (mWebRTCDeviceInterface)
        {
            mWebRTCDeviceInterface->setTuningMode(false);
        }
        mIsInTuningMode = false;
    }
}
bool LLWebRTCVoiceClient::inTuningMode()
{
    return mIsInTuningMode;
}
void LLWebRTCVoiceClient::tuningSetMicVolume(float volume)
{
    if (volume != mTuningMicGain)
    {
        mTuningMicGain = volume;
        if (mWebRTCDeviceInterface)
        {
            mWebRTCDeviceInterface->setTuningMicGain(volume);
        }
    }
}
void LLWebRTCVoiceClient::tuningSetSpeakerVolume(float volume)
{
    if (volume != mTuningSpeakerVolume)
    {
        mTuningSpeakerVolume = (int)volume;
    }
}
float LLWebRTCVoiceClient::tuningGetEnergy(void)
{
    if (!mWebRTCDeviceInterface)
    {
        return 0.f;
    }
    float rms = mWebRTCDeviceInterface->getTuningAudioLevel();
    return TUNING_LEVEL_START_POINT - TUNING_LEVEL_SCALE * rms;
}
bool LLWebRTCVoiceClient::deviceSettingsAvailable()
{
    bool result = true;
    if(mRenderDevices.empty() || mCaptureDevices.empty())
        result = false;
    return result;
}
bool LLWebRTCVoiceClient::deviceSettingsUpdated()
{
    bool updated = mDevicesListUpdated;
    mDevicesListUpdated = false;
    return updated;
}
void LLWebRTCVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
    if(clearCurrentList)
    {
        clearCaptureDevices();
        clearRenderDevices();
    }
    if (mWebRTCDeviceInterface)
    {
        mWebRTCDeviceInterface->refreshDevices();
    }
}
void LLWebRTCVoiceClient::setHidden(bool hidden)
{
    mHidden = hidden;
    if (inSpatialChannel())
    {
        if (mWebRTCDeviceInterface)
        {
            mWebRTCDeviceInterface->setMute(mHidden || mMuteMic,
                                            mHidden ? 0 : SET_HIDDEN_RESTORE_DELAY_MS);
        }
        if (mHidden)
        {
            sessionState::for_each(boost::bind(predSetMuteMic, _1, true));
        }
        else
        {
            sessionState::for_each(boost::bind(predSetMuteMic, _1, mMuteMic));
            updatePosition();
            sendPositionUpdate(true);
        }
    }
}
void LLWebRTCVoiceClient::OnConnectionEstablished(const std::string &channelID, const LLUUID &regionID)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (gAgent.getRegion()->getRegionID() == regionID)
    {
        if (mNextSession && mNextSession->mChannelID == channelID)
        {
            if (mSession)
            {
                mSession->shutdownAllConnections();
            }
            mSession = mNextSession;
            mNextSession.reset();
        }
        if (mSession)
        {
            mSession->addParticipant(gAgentID, gAgent.getRegion()->getRegionID());
        }
        if (mSession && mSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);
            if (!mSession->mNotifyOnFirstJoin)
            {
                LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
            }
        }
    }
}
void LLWebRTCVoiceClient::OnConnectionShutDown(const std::string &channelID, const LLUUID &regionID)
{
    if (mSession && (mSession->mChannelID == channelID))
    {
        if (gAgent.getRegion()->getRegionID() == regionID)
        {
            if (mSession && mSession->mChannelID == channelID)
            {
                LL_INFOS("Voice") << "Main WebRTC Connection Shut Down." << LL_ENDL;
            }
        }
        mSession->removeAllParticipants(regionID);
    }
}
void LLWebRTCVoiceClient::OnConnectionFailure(const std::string                       &channelID,
                                              const LLUUID                            &regionID,
                                              LLVoiceClientStatusObserver::EStatusType status_type)
{
    LL_DEBUGS("Voice") << "A connection failed.  channel:" << channelID << LL_ENDL;
    if (gAgent.getRegion()->getRegionID() == regionID)
    {
        if (mNextSession && mNextSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(status_type);
        }
        else if (mSession && mSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(status_type);
        }
    }
}
void LLWebRTCVoiceClient::setEarLocation(S32 loc)
{
    if (mEarLocation != loc)
    {
        LL_DEBUGS("Voice") << "Setting mEarLocation to " << loc << LL_ENDL;
        mEarLocation        = loc;
        mSpatialCoordsDirty = true;
    }
}
void LLWebRTCVoiceClient::updatePosition(void)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    LLViewerRegion *region = gAgent.getRegion();
    if (region && isAgentAvatarValid())
    {
        LLVector3d   avatar_pos  = gAgentAvatarp->getPositionGlobal();
        LLQuaternion avatar_qrot = gAgentAvatarp->getRootJoint()->getWorldRotation();
        avatar_pos += LLVector3d(0.f, 0.f, 1.f);
        LLVector3d   earPosition;
        LLQuaternion earRot;
        switch (mEarLocation)
        {
            case earLocCamera:
            default:
                earPosition = region->getPosGlobalFromRegion(LLViewerCamera::getInstance()->getOrigin());
                earRot      = LLViewerCamera::getInstance()->getQuaternion();
                break;
            case earLocAvatar:
                earPosition = mAvatarPosition;
                earRot      = mAvatarRot;
                break;
            case earLocMixed:
                earPosition = mAvatarPosition;
                earRot      = LLViewerCamera::getInstance()->getQuaternion();
                break;
        }
        setListenerPosition(earPosition,
                            LLVector3::zero,
                            earRot);
        setAvatarPosition(avatar_pos,
                          LLVector3::zero,
                          avatar_qrot);
        enforceTether();
        updateNeighboringRegions();
        LLWebRTCVoiceClient::participantStatePtr_t participant = findParticipantByID("Estate", gAgentID);
        if(participant)
        {
            if (participant->mRegion != region->getRegionID()) {
                participant->mRegion = region->getRegionID();
            }
        }
    }
}
void LLWebRTCVoiceClient::setListenerPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
{
    mListenerRequestedPosition = position;
    if (mListenerVelocity != velocity)
    {
        mListenerVelocity   = velocity;
        mSpatialCoordsDirty = true;
    }
    if (mListenerRot != rot)
    {
        mListenerRot        = rot;
        mSpatialCoordsDirty = true;
    }
}
void LLWebRTCVoiceClient::setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
{
    if (dist_vec_squared(mAvatarPosition, position) > 0.01)
    {
        mAvatarPosition     = position;
        mSpatialCoordsDirty = true;
    }
    if (mAvatarVelocity != velocity)
    {
        mAvatarVelocity     = velocity;
        mSpatialCoordsDirty = true;
    }
    F32 rot_cos_diff = llabs(dot(mAvatarRot, rot));
    if ((mAvatarRot != rot) && (rot_cos_diff < MINUSCULE_ANGLE_COS))
    {
        mAvatarRot          = rot;
        mSpatialCoordsDirty = true;
    }
}
void LLWebRTCVoiceClient::enforceTether()
{
    LLVector3d tethered = mListenerRequestedPosition;
    {
        LLVector3d camera_offset   = mListenerRequestedPosition - mAvatarPosition;
        F32        camera_distance = (F32) camera_offset.magVec();
        if (camera_distance > MAX_AUDIO_DIST)
        {
            tethered = mAvatarPosition + (MAX_AUDIO_DIST / camera_distance) * camera_offset;
        }
    }
    if (dist_vec_squared(mListenerPosition, tethered) > 0.01)
    {
        mListenerPosition   = tethered;
        mSpatialCoordsDirty = true;
    }
}
void LLWebRTCVoiceClient::sendPositionUpdate(bool force)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    std::string      spatial_data;
    if (mSpatialCoordsDirty || force)
    {
        boost::json::object spatial;
        spatial["sp"] = {
            {"x", (int) (mAvatarPosition[0] * 100)},
            {"y", (int) (mAvatarPosition[1] * 100)},
            {"z", (int) (mAvatarPosition[2] * 100)}
        };
        spatial["sh"]  = {
            {"x", (int) (mAvatarRot.mQ[0] * 100)},
            {"y", (int) (mAvatarRot.mQ[1] * 100)},
            {"z", (int) (mAvatarRot.mQ[2] * 100)},
            {"w", (int) (mAvatarRot.mQ[3] * 100)}
        };
        spatial["lp"] = {
            {"x", (int) (mListenerPosition[0] * 100)},
            {"y", (int) (mListenerPosition[1] * 100)},
            {"z", (int) (mListenerPosition[2] * 100)}
        };
        spatial["lh"] = {
            {"x", (int) (mListenerRot.mQ[0] * 100)},
            {"y", (int) (mListenerRot.mQ[1] * 100)},
            {"z", (int) (mListenerRot.mQ[2] * 100)},
            {"w", (int) (mListenerRot.mQ[3] * 100)}};
        mSpatialCoordsDirty = false;
        spatial_data = boost::json::serialize(spatial);
        sessionState::for_each(boost::bind(predSendData, _1, spatial_data));
    }
}
void LLWebRTCVoiceClient::updateOwnVolume()
{
    F32 audio_level = 0.0f;
    if (!mMuteMic && mWebRTCDeviceInterface)
    {
        float rms = mWebRTCDeviceInterface->getPeerConnectionAudioLevel();
        audio_level = LEVEL_START_POINT - LEVEL_SCALE * rms;
    }
    sessionState::for_each(boost::bind(predUpdateOwnVolume, _1, audio_level));
}
bool LLWebRTCVoiceClient::isParticipantAvatar(const LLUUID &id)
{
    return true;
}
void LLWebRTCVoiceClient::getParticipantList(uuid_set_t &participants)
{
    if (mProcessChannels && mSession)
    {
        for (participantUUIDMap::iterator iter = mSession->mParticipantsByUUID.begin();
            iter != mSession->mParticipantsByUUID.end();
            iter++)
        {
            participants.insert(iter->first);
        }
    }
}
bool LLWebRTCVoiceClient::isParticipant(const LLUUID &speaker_id)
{
    if (mProcessChannels && mSession)
    {
        return (mSession->mParticipantsByUUID.find(speaker_id) != mSession->mParticipantsByUUID.end());
    }
    return false;
}
LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::findParticipantByID(const std::string &channelID, const LLUUID &id)
{
    participantStatePtr_t result;
    LLWebRTCVoiceClient::sessionState::ptr_t session = sessionState::matchSessionByChannelID(channelID);
    if (session)
    {
        result = session->findParticipantByID(id);
    }
    return result;
}
LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::addParticipantByID(const std::string &channelID, const LLUUID &id, const LLUUID& region)
{
    participantStatePtr_t result;
    LLWebRTCVoiceClient::sessionState::ptr_t session = sessionState::matchSessionByChannelID(channelID);
    if (session)
    {
        result = session->addParticipant(id, region);
        if (session->mNotifyOnFirstJoin && (id != gAgentID))
        {
            notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
        }
    }
    return result;
}
void LLWebRTCVoiceClient::removeParticipantByID(const std::string &channelID, const LLUUID &id, const LLUUID& region)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    participantStatePtr_t result;
    LLWebRTCVoiceClient::sessionState::ptr_t session = sessionState::matchSessionByChannelID(channelID);
    if (session)
    {
        participantStatePtr_t participant = session->findParticipantByID(id);
        if (participant && (participant->mRegion == region))
        {
            session->removeParticipant(participant);
        }
    }
}
LLWebRTCVoiceClient::participantState::participantState(const LLUUID& agent_id, const LLUUID& region) :
     mURI(agent_id.asString()),
     mAvatarID(agent_id),
     mIsSpeaking(false),
     mIsModeratorMuted(false),
     mLevel(0.f),
     mVolume(LLVoiceClient::VOLUME_DEFAULT),
     mRegion(region)
{
}
LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::addParticipant(const LLUUID& agent_id, const LLUUID& region)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    participantStatePtr_t result;
    participantUUIDMap::iterator iter = mParticipantsByUUID.find(agent_id);
    if (iter != mParticipantsByUUID.end())
    {
        result = iter->second;
        result->mRegion = region;
    }
    if (!result)
    {
        result = std::make_shared<participantState>(agent_id, region);
        mParticipantsByUUID.insert(participantUUIDMap::value_type(agent_id, result));
        result->mAvatarID = agent_id;
    }
    LLWebRTCVoiceClient::getInstance()->lookupName(agent_id);
    LLSpeakerVolumeStorage::getInstance()->getSpeakerVolume(result->mAvatarID, result->mVolume);
    if (!LLWebRTCVoiceClient::sShuttingDown)
    {
        LLWebRTCVoiceClient::getInstance()->notifyParticipantObservers();
    }
    LL_INFOS("Voice") << "Participant \"" << result->mURI << "\" added." << LL_ENDL;
    return result;
}
LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::findParticipantByID(const LLUUID& id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    participantStatePtr_t result;
    participantUUIDMap::iterator iter = mParticipantsByUUID.find(id);
    if(iter != mParticipantsByUUID.end())
    {
        result = iter->second;
    }
    return result;
}
void LLWebRTCVoiceClient::sessionState::removeParticipant(const LLWebRTCVoiceClient::participantStatePtr_t &participant)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (participant)
    {
        LLUUID participantID = participant->mAvatarID;
        participantUUIDMap::iterator iter = mParticipantsByUUID.find(participant->mAvatarID);
        LL_DEBUGS("Voice") << "participant \"" << participant->mURI << "\" (" << participantID << ") removed." << LL_ENDL;
        if (iter == mParticipantsByUUID.end())
        {
            LL_WARNS("Voice") << "Internal error: participant ID " << participantID << " not in UUID map" << LL_ENDL;
        }
        else
        {
            mParticipantsByUUID.erase(iter);
            if (!LLWebRTCVoiceClient::sShuttingDown)
            {
                LLWebRTCVoiceClient::getInstance()->notifyParticipantObservers();
            }
        }
        if (mHangupOnLastLeave && (participantID != gAgentID) && (mParticipantsByUUID.size() <= 1) && LLWebRTCVoiceClient::instanceExists())
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
        }
    }
}
void LLWebRTCVoiceClient::sessionState::removeAllParticipants(const LLUUID &region)
{
    std::vector<participantStatePtr_t> participantsToRemove;
    for (auto& participantEntry : mParticipantsByUUID)
    {
        if (region.isNull() || (participantEntry.second->mRegion == region))
        {
            participantsToRemove.push_back(participantEntry.second);
        }
    }
    for (auto& participant : participantsToRemove)
    {
        removeParticipant(participant);
    }
}
bool LLWebRTCVoiceClient::startEstateSession()
{
    LL_INFOS("Voice") << "Starting estate spatial session" << LL_ENDL;
    leaveChannel(false);
    mNextSession = addSession("Estate", sessionState::ptr_t(new estateSessionState()));
    return true;
}
bool LLWebRTCVoiceClient::startParcelSession(const std::string &channelID, S32 parcelID)
{
    leaveChannel(false);
    mNextSession = addSession(channelID, sessionState::ptr_t(new parcelSessionState(channelID, parcelID)));
    return true;
}
bool LLWebRTCVoiceClient::startAdHocSession(const LLSD& channelInfo, bool notify_on_first_join, bool hangup_on_last_leave)
{
    leaveChannel(false);
    LL_WARNS("Voice") << "Start AdHoc Session " << channelInfo << LL_ENDL;
    std::string channelID = channelInfo["channel_uri"];
    std::string credentials = channelInfo["channel_credentials"];
    mNextSession = addSession(channelID,
                              sessionState::ptr_t(new adhocSessionState(channelID,
                                                                        credentials,
                                                                        notify_on_first_join,
                                                                        hangup_on_last_leave)));
    return true;
}
bool LLWebRTCVoiceClient::isVoiceWorking() const
{
    return mIsCoroutineActive;
}
bool LLWebRTCVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
    sessionStatePtr_t session(findP2PSession(session_id));
    return session && session->isCallbackPossible();
}
bool LLWebRTCVoiceClient::setSpatialChannel(const LLSD &channelInfo)
{
    LL_INFOS("Voice") << "SetSpatialChannel " << channelInfo << LL_ENDL;
    LLViewerRegion *regionp = gAgent.getRegion();
    if (!regionp)
    {
        return false;
    }
    LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (channelInfo.isMap() && channelInfo.has("channel_uri"))
    {
        bool allow_voice = !channelInfo["channel_uri"].asString().empty();
        if (parcel)
        {
            parcel->setParcelFlag(PF_ALLOW_VOICE_CHAT, allow_voice);
            parcel->setParcelFlag(PF_USE_ESTATE_VOICE_CHAN, channelInfo["channel_uri"].asUUID() == regionp->getRegionID());
        }
        else
        {
            regionp->setRegionFlag(REGION_FLAGS_ALLOW_VOICE, allow_voice);
        }
    }
    return true;
}
void LLWebRTCVoiceClient::leaveNonSpatialChannel()
{
    LL_DEBUGS("Voice") << "Request to leave non-spatial channel." << LL_ENDL;
    deleteSession(mNextSession);
    leaveChannel(true);
}
void LLWebRTCVoiceClient::processChannels(bool process)
{
    LL_INFOS("Voice") << "processChannels(" << (process ? "true" : "false")
                      << ") was=" << (mProcessChannels ? "true" : "false")
                      << " voiceEnabled=" << (mVoiceEnabled ? "true" : "false")
                      << " coro=" << (mIsCoroutineActive ? "active" : "inactive") << LL_ENDL;
    mProcessChannels = process;
}
bool LLWebRTCVoiceClient::inProximalChannel()
{
    return inSpatialChannel();
}
bool LLWebRTCVoiceClient::inOrJoiningChannel(const std::string& channelID)
{
    return (mSession && mSession->mChannelID == channelID) || (mNextSession && mNextSession->mChannelID == channelID);
}
bool LLWebRTCVoiceClient::inEstateChannel()
{
    return (mSession && mSession->isEstate()) || (mNextSession && mNextSession->isEstate());
}
bool LLWebRTCVoiceClient::inSpatialChannel()
{
    bool result = true;
    if (mNextSession)
    {
        result = mNextSession->isSpatial();
    }
    else if(mSession)
    {
        result = mSession->isSpatial();
    }
    return result;
}
LLSD LLWebRTCVoiceClient::getAudioSessionChannelInfo()
{
    LLSD result;
    if (mSession)
    {
        result["voice_server_type"]   = WEBRTC_VOICE_SERVER_TYPE;
        result["channel_uri"]         = mSession->mChannelID;
    }
    return result;
}
void LLWebRTCVoiceClient::leaveChannel(bool stopTalking)
{
    if (mSession)
    {
        deleteSession(mSession);
    }
    if (mNextSession)
    {
        deleteSession(mNextSession);
    }
    if (stopTalking && LLVoiceClient::getInstance()->getUserPTTState())
    {
        LLVoiceClient::getInstance()->setUserPTTState(false);
    }
}
bool LLWebRTCVoiceClient::isCurrentChannel(const LLSD &channelInfo)
{
    if (!mProcessChannels || (channelInfo["voice_server_type"].asString() != WEBRTC_VOICE_SERVER_TYPE))
    {
        return false;
    }
    sessionStatePtr_t session = mSession;
    if (!session)
    {
        session = mNextSession;
    }
    if (session)
    {
        if (!channelInfo["session_handle"].asString().empty())
        {
            return session->mHandle == channelInfo["session_handle"].asString();
        }
        return channelInfo["channel_uri"].asString() == session->mChannelID;
    }
    return false;
}
bool LLWebRTCVoiceClient::compareChannels(const LLSD &channelInfo1, const LLSD &channelInfo2)
{
    return (channelInfo1["voice_server_type"] == WEBRTC_VOICE_SERVER_TYPE) &&
           (channelInfo1["voice_server_type"] == channelInfo2["voice_server_type"]) &&
           (channelInfo1["sip_uri"] == channelInfo2["sip_uri"]);
}
void LLWebRTCVoiceClient::setMuteMic(bool muted)
{
    if (mMuteMic != muted)
    {
        LL_INFOS("Voice") << "( " << (muted ? "true" : "false") << " )" << LL_ENDL;
    }
    mMuteMic = muted;
    if (mIsInTuningMode)
    {
        return;
    }
    if (mWebRTCDeviceInterface)
    {
        mWebRTCDeviceInterface->setMute(muted, muted ? MUTE_FADE_DELAY_MS : 0);
    }
    if (!mHidden)
    {
        sessionState::for_each(boost::bind(predSetMuteMic, _1, muted));
    }
}
void LLWebRTCVoiceClient::predSetMuteMic(const LLWebRTCVoiceClient::sessionStatePtr_t &session, bool muted)
{
    participantStatePtr_t participant = session->findParticipantByID(gAgentID);
    if (participant)
    {
        participant->mLevel = 0.0;
    }
    session->setMuteMic(muted);
}
void LLWebRTCVoiceClient::setVoiceVolume(F32 volume)
{
    if (volume != mSpeakerVolume)
    {
        {
            mSpeakerVolume      = volume;
        }
        sessionState::for_each(boost::bind(predSetSpeakerVolume, _1, volume));
    }
}
void LLWebRTCVoiceClient::predSetSpeakerVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 volume)
{
    if (session->mShuttingDown)
    {
        return;
    }
    session->setSpeakerVolume(volume);
}
void LLWebRTCVoiceClient::setMicGain(F32 gain)
{
    if (gain != mMicGain)
    {
        mMicGain = gain;
        if (mWebRTCDeviceInterface)
        {
            mWebRTCDeviceInterface->setMicGain(gain);
        }
    }
}
void LLWebRTCVoiceClient::setVoiceEnabled(bool enabled)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (enabled != mVoiceEnabled)
    {
        LL_INFOS("Voice") << "( " << (enabled ? "enabled" : "disabled") << " )"
                           << ", coro: " << (mIsCoroutineActive ? "active" : "inactive") << LL_ENDL;
        mVoiceEnabled = enabled;
        LLVoiceClientStatusObserver::EStatusType status;
        if (enabled)
        {
            LL_DEBUGS("Voice") << "enabling" << LL_ENDL;
            LLVoiceChannel::getCurrentVoiceChannel()->activate();
            status = LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED;
            mSpatialCoordsDirty = true;
            updatePosition();
            if (!mIsCoroutineActive)
            {
                LLCoros::instance().launch("LLWebRTCVoiceClient::voiceConnectionCoro",
                    boost::bind(&LLWebRTCVoiceClient::voiceConnectionCoro, LLWebRTCVoiceClient::getInstance()));
            }
            else
            {
                LL_DEBUGS("Voice") << "coro should be active.. not launching" << LL_ENDL;
            }
        }
        else
        {
            LLVoiceChannel::getCurrentVoiceChannel()->deactivate();
            gAgent.setVoiceConnected(false);
            status = LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED;
            cleanUp();
        }
        notifyStatusObservers(status);
    }
    else
    {
        LL_DEBUGS("Voice") << " no-op" << LL_ENDL;
    }
}
std::string LLWebRTCVoiceClient::getDisplayName(const LLUUID& id)
{
    std::string result;
    if (mProcessChannels && mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mDisplayName;
        }
    }
    return result;
}
bool LLWebRTCVoiceClient::getIsSpeaking(const LLUUID& id)
{
    bool result = false;
    if (mProcessChannels && mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mIsSpeaking;
        }
    }
    return result;
}
bool LLWebRTCVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
    bool result = false;
    if (mProcessChannels && mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mIsModeratorMuted;
        }
    }
    return result;
}
F32 LLWebRTCVoiceClient::getCurrentPower(const LLUUID &id)
{
    F32 result = 0.0;
    if (!mProcessChannels || !mSession)
    {
        return result;
    }
    participantStatePtr_t participant(mSession->findParticipantByID(id));
    if (participant)
    {
        if (participant->mIsSpeaking)
        {
            result = participant->mLevel;
        }
    }
    return result;
}
F32 LLWebRTCVoiceClient::getUserVolume(const LLUUID& id)
{
    F32 result = LLVoiceClient::VOLUME_MIN;
    if (mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mVolume;
        }
    }
    return result;
}
void LLWebRTCVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
    F32 clamped_volume = llclamp(volume, LLVoiceClient::VOLUME_MIN, LLVoiceClient::VOLUME_MAX);
    if(mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant && (participant->mAvatarID != gAgentID))
        {
            if (!is_approx_equal(volume, LLVoiceClient::VOLUME_DEFAULT))
            {
                LLSpeakerVolumeStorage::getInstance()->storeSpeakerVolume(id, volume);
            }
            else
            {
                LLSpeakerVolumeStorage::getInstance()->removeSpeakerVolume(id);
            }
            participant->mVolume = clamped_volume;
        }
    }
    sessionState::for_each(boost::bind(predSetUserVolume, _1, id, clamped_volume));
}
void LLWebRTCVoiceClient::predSetUserVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const LLUUID &id, F32 volume)
{
    session->setUserVolume(id, volume);
}
void LLWebRTCVoiceClient::onChange()
{
}
void LLWebRTCVoiceClient::onChangeDetailed(const LLMute& mute)
{
    if (mute.mType == LLMute::AGENT)
    {
        bool muted = ((mute.mFlags & LLMute::flagVoiceChat) == 0);
        sessionState::for_each(boost::bind(predSetUserMute, _1, mute.mID, muted));
    }
}
void LLWebRTCVoiceClient::userAuthorized(const std::string& user_id, const LLUUID& agentID)
{
    if (sShuttingDown)
    {
        sShuttingDown = false;
        initWebRTC();
    }
}
void LLWebRTCVoiceClient::predSetUserMute(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const LLUUID &id, bool mute)
{
    session->setUserMute(id, mute);
}
std::map<std::string, LLWebRTCVoiceClient::sessionState::ptr_t> LLWebRTCVoiceClient::sessionState::sSessions;
LLWebRTCVoiceClient::sessionState::sessionState() :
    mHangupOnLastLeave(false),
    mNotifyOnFirstJoin(false),
    mMuted(false),
    mSpeakerVolume(1.0),
    mShuttingDown(false)
{
}
void LLWebRTCVoiceClient::predUpdateOwnVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 audio_level)
{
    if (session->mShuttingDown)
    {
        return;
    }
    participantStatePtr_t participant = session->findParticipantByID(gAgentID);
    if (participant)
    {
        participant->mLevel = audio_level;
        participant->mIsSpeaking = audio_level > SPEAKING_AUDIO_LEVEL;
    }
}
void LLWebRTCVoiceClient::predSendData(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const std::string &spatial_data)
{
    if (session->isSpatial() && !spatial_data.empty())
    {
        session->sendData(spatial_data);
    }
}
void LLWebRTCVoiceClient::sessionState::sendData(const std::string &data)
{
    for (auto &connection : mWebRTCConnections)
    {
        connection->sendData(data);
    }
}
void LLWebRTCVoiceClient::sessionState::setMuteMic(bool muted)
{
    mMuted = muted;
    if (mShuttingDown)
    {
        return;
    }
    for (auto &connection : mWebRTCConnections)
    {
        if (!connection->isShuttingDown())
        {
            connection->setMuteMic(muted);
        }
    }
}
void LLWebRTCVoiceClient::sessionState::setSpeakerVolume(F32 volume)
{
    mSpeakerVolume = volume;
    for (auto &connection : mWebRTCConnections)
    {
        if (!connection->isShuttingDown())
        {
            connection->setSpeakerVolume(volume);
        }
    }
}
void LLWebRTCVoiceClient::sessionState::setUserVolume(const LLUUID &id, F32 volume)
{
    if (mParticipantsByUUID.find(id) == mParticipantsByUUID.end())
    {
        return;
    }
    for (auto &connection : mWebRTCConnections)
    {
        if (!connection->isShuttingDown())
        {
            connection->setUserVolume(id, volume);
        }
    }
}
void LLWebRTCVoiceClient::sessionState::setUserMute(const LLUUID &id, bool mute)
{
    if (mParticipantsByUUID.find(id) == mParticipantsByUUID.end())
    {
        return;
    }
    for (auto &connection : mWebRTCConnections)
    {
        if (!connection->isShuttingDown())
        {
            connection->setUserMute(id, mute);
        }
    }
}
void LLWebRTCVoiceClient::sessionState::addSession(
    const std::string & channelID,
    LLWebRTCVoiceClient::sessionState::ptr_t& session)
{
    sSessions[channelID] = session;
}
LLWebRTCVoiceClient::sessionState::~sessionState()
{
    LL_DEBUGS("Voice") << "Destroying session CHANNEL=" << mChannelID << LL_ENDL;
    if (!mShuttingDown)
    {
        shutdownAllConnections();
    }
    mWebRTCConnections.clear();
    removeAllParticipants();
}
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::matchSessionByChannelID(const std::string& channel_id)
{
    sessionStatePtr_t result;
    std::map<std::string, ptr_t>::iterator it = sSessions.find(channel_id);
    if (it != sSessions.end())
    {
        result = (*it).second;
    }
    return result;
}
void LLWebRTCVoiceClient::sessionState::for_each(sessionFunc_t func)
{
    std::for_each(sSessions.begin(), sSessions.end(), boost::bind(for_eachPredicate, _1, func));
}
void LLWebRTCVoiceClient::sessionState::reapEmptySessions()
{
    std::map<std::string, ptr_t>::iterator iter;
    for (iter = sSessions.begin(); iter != sSessions.end();)
    {
        if (iter->second->isEmpty())
        {
            iter = sSessions.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}
void LLWebRTCVoiceClient::sessionState::for_eachPredicate(const std::pair<std::string, LLWebRTCVoiceClient::sessionState::wptr_t> &a, sessionFunc_t func)
{
    ptr_t aLock(a.second.lock());
    if (aLock)
        func(aLock);
    else
    {
        LL_WARNS("Voice") << "Stale handle in session map!" << LL_ENDL;
    }
}
LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::addSession(const std::string &channel_id, sessionState::ptr_t session)
{
    sessionStatePtr_t existingSession = sessionState::matchSessionByChannelID(channel_id);
    if (!existingSession)
    {
        LL_DEBUGS("Voice") << "adding new session with channel: " << channel_id << LL_ENDL;
        session->setMuteMic(mMuteMic);
        session->setSpeakerVolume(mSpeakerVolume);
        sessionState::addSession(channel_id, session);
        return session;
    }
    else
    {
        LL_DEBUGS("Voice") << "Attempting to add already-existing session " << channel_id << LL_ENDL;
        existingSession->revive();
        return existingSession;
    }
}
void LLWebRTCVoiceClient::sessionState::clearSessions()
{
    sSessions.clear();
}
LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::findP2PSession(const LLUUID &agent_id)
{
    sessionStatePtr_t result = sessionState::matchSessionByChannelID(agent_id.asString());
    if (result && !result->isSpatial())
    {
        return result;
    }
    result.reset();
    return result;
}
void LLWebRTCVoiceClient::sessionState::shutdownAllConnections()
{
    mShuttingDown = true;
    for (auto &&connection : mWebRTCConnections)
    {
        connection->shutDown();
    }
}
void LLWebRTCVoiceClient::sessionState::revive()
{
    mShuttingDown = false;
}
const std::string LLWebRTCVoiceClient::sessionState::getVersion() const
{
    bool primary = true;
    do
    {
        for (auto& connection : mWebRTCConnections) {
            if (connection->isPrimary() == primary && connection->getVersion().length()) {
                return connection->getVersion();
            }
        }
        primary = !primary;
    } while (!primary);
    return "";
}
void LLWebRTCVoiceClient::sessionState::processSessionStates()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    auto iter = sSessions.begin();
    while (iter != sSessions.end())
    {
        if (!iter->second->processConnectionStates() && iter->second->mShuttingDown)
        {
            iter = sSessions.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}
bool LLWebRTCVoiceClient::sessionState::processConnectionStates()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    std::list<connectionPtr_t>::iterator iter = mWebRTCConnections.begin();
    while (iter != mWebRTCConnections.end())
    {
        if (!iter->get()->connectionStateMachine())
        {
            iter = mWebRTCConnections.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    return !mWebRTCConnections.empty();
}
bool LLWebRTCVoiceClient::estateSessionState::isRegionWebRTCEnabled(const LLUUID& regionID)
{
    LLViewerRegion* region = LLWorld::getInstance()->getRegionFromID(regionID);
    if (!region)
    {
        LL_WARNS("Voice") << "Could not find region " << regionID
                         << " for voice server type validation" << LL_ENDL;
        return false;
    }
    if (!region->simulatorFeaturesReceived())
    {
        LL_INFOS("Voice") << "Region " << regionID
                          << " simulator features not received yet; deferring WebRTC check" << LL_ENDL;
        return true;
    }
    LLSD simulatorFeatures;
    region->getSimulatorFeatures(simulatorFeatures);
    std::string voiceServerType = simulatorFeatures.has("VoiceServerType")
                                      ? simulatorFeatures["VoiceServerType"].asString()
                                      : std::string();
    if (voiceServerType.empty())
    {
        voiceServerType = WEBRTC_VOICE_SERVER_TYPE;
    }
    bool isWebRTCEnabled = (voiceServerType == WEBRTC_VOICE_SERVER_TYPE);
    if (!isWebRTCEnabled)
    {
        LL_INFOS("Voice") << "Region " << regionID << " VoiceServerType is not webrtc (got: "
                          << voiceServerType << ")" << LL_ENDL;
    }
    return isWebRTCEnabled;
}
bool LLWebRTCVoiceClient::estateSessionState::processConnectionStates()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (!mShuttingDown)
    {
        std::set<LLUUID> neighbor_ids = LLWebRTCVoiceClient::getInstance()->getNeighboringRegions();
        for (auto &connection : mWebRTCConnections)
        {
            std::shared_ptr<LLVoiceWebRTCSpatialConnection> spatialConnection =
                std::static_pointer_cast<LLVoiceWebRTCSpatialConnection>(connection);
            LLUUID regionID = spatialConnection.get()->getRegionID();
            if (neighbor_ids.find(regionID) == neighbor_ids.end())
            {
                spatialConnection.get()->shutDown();
            }
            else if (!isRegionWebRTCEnabled(regionID))
            {
                LL_DEBUGS("Voice") << "Shutting down connection to neighbor region " << regionID
                                  << " - no longer supports WebRTC voice" << LL_ENDL;
                spatialConnection.get()->shutDown();
            }
            if (!spatialConnection.get()->isShuttingDown())
            {
                neighbor_ids.erase(regionID);
            }
        }
        for (auto &neighbor : neighbor_ids)
        {
            if (isRegionWebRTCEnabled(neighbor))
            {
                connectionPtr_t connection = std::make_shared<LLVoiceWebRTCSpatialConnection>(neighbor, INVALID_PARCEL_ID, mChannelID);
                mWebRTCConnections.push_back(connection);
                connection->setMuteMic(mMuted);
                connection->setSpeakerVolume(mSpeakerVolume);
            }
            else
            {
                LL_DEBUGS("Voice") << "Skipping neighbor region " << neighbor
                                  << " - does not support WebRTC voice" << LL_ENDL;
            }
        }
    }
    return LLWebRTCVoiceClient::sessionState::processConnectionStates();
}
LLWebRTCVoiceClient::estateSessionState::estateSessionState()
{
    mHangupOnLastLeave = false;
    mNotifyOnFirstJoin = false;
    mChannelID         = "Estate";
    LLUUID region_id   = gAgent.getRegion()->getRegionID();
    mWebRTCConnections.emplace_back(new LLVoiceWebRTCSpatialConnection(region_id, INVALID_PARCEL_ID, "Estate"));
}
LLWebRTCVoiceClient::parcelSessionState::parcelSessionState(const std::string &channelID, S32 parcel_local_id)
{
    mHangupOnLastLeave = false;
    mNotifyOnFirstJoin = false;
    LLUUID region_id   = gAgent.getRegion()->getRegionID();
    mChannelID         = channelID;
    mWebRTCConnections.emplace_back(new LLVoiceWebRTCSpatialConnection(region_id, parcel_local_id, channelID));
}
LLWebRTCVoiceClient::adhocSessionState::adhocSessionState(const std::string &channelID,
                                                          const std::string &credentials,
                                                          bool notify_on_first_join,
                                                          bool hangup_on_last_leave) :
    mCredentials(credentials)
{
    mHangupOnLastLeave = hangup_on_last_leave;
    mNotifyOnFirstJoin = notify_on_first_join;
    LLUUID region_id   = gAgent.getRegion()->getRegionID();
    mChannelID         = channelID;
    mWebRTCConnections.emplace_back(new LLVoiceWebRTCAdHocConnection(region_id, channelID, credentials));
}
void LLWebRTCVoiceClient::predShutdownSession(const LLWebRTCVoiceClient::sessionStatePtr_t& session)
{
    session->shutdownAllConnections();
}
void LLWebRTCVoiceClient::deleteSession(const sessionStatePtr_t &session)
{
    if (!session)
    {
        return;
    }
    session->shutdownAllConnections();
    bool deleteAudioSession = mSession == session;
    bool deleteNextAudioSession = mNextSession == session;
    if (deleteAudioSession)
    {
        mSession.reset();
    }
    if (deleteNextAudioSession)
    {
        mNextSession.reset();
    }
    if (!sShuttingDown)
    {
        updateVersion();
    }
}
void LLWebRTCVoiceClient::lookupName(const LLUUID &id)
{
    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
    mAvatarNameCacheConnection = LLAvatarNameCache::get(id, boost::bind(&LLWebRTCVoiceClient::onAvatarNameCache, this, _1, _2));
}
void LLWebRTCVoiceClient::onAvatarNameCache(const LLUUID& agent_id,
                                           const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();
    std::string display_name = av_name.getDisplayName();
    avatarNameResolved(agent_id, display_name);
}
void LLWebRTCVoiceClient::predAvatarNameResolution(const LLWebRTCVoiceClient::sessionStatePtr_t &session, LLUUID id, std::string name)
{
    participantStatePtr_t participant(session->findParticipantByID(id));
    if (participant)
    {
        participant->mDisplayName = name;
        LLWebRTCVoiceClient::getInstance()->notifyParticipantObservers();
    }
}
void LLWebRTCVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
    sessionState::for_each(boost::bind(predAvatarNameResolution, _1, id, name));
}
LLSD LLWebRTCVoiceClient::getP2PChannelInfoTemplate(const LLUUID& id) const
{
    return LLSD();
}
LLVoiceWebRTCConnection::LLVoiceWebRTCConnection(const LLUUID &regionID, const std::string &channelID) :
    mWebRTCAudioInterface(nullptr),
    mWebRTCDataInterface(nullptr),
    mVoiceConnectionState(VOICE_STATE_START_SESSION),
    mCurrentStatus(LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED),
    mMuted(true),
    mShutDown(false),
    mIceCompleted(false),
    mSpeakerVolume(0.0),
    mOutstandingRequests(0),
    mChannelID(channelID),
    mRegionID(regionID),
    mPrimary(true),
    mRetryWaitPeriod(0)
{
    mRetryWaitSecs = (F32)((F32) rand() / (RAND_MAX)) + 0.5f;
    mWebRTCPeerConnectionInterface = llwebrtc::newPeerConnection();
    mWebRTCPeerConnectionInterface->setSignalingObserver(this);
    mMainQueue = LL::WorkQueue::getInstance("mainloop");
}
LLVoiceWebRTCConnection::~LLVoiceWebRTCConnection()
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    mWebRTCPeerConnectionInterface->unsetSignalingObserver(this);
    llwebrtc::freePeerConnection(mWebRTCPeerConnectionInterface);
}
void LLVoiceWebRTCConnection::OnIceGatheringState(llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState state)
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=] {
            LL_DEBUGS("Voice") << "Ice Gathering voice account. " << state << LL_ENDL;
            switch (state)
            {
                case llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_COMPLETE:
                {
                    mIceCompleted = true;
                    break;
                }
                case llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_NEW:
                {
                    mIceCompleted = false;
                }
                default:
                    break;
            }
        });
}
void LLVoiceWebRTCConnection::OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate& candidate)
{
    LL::WorkQueue::postMaybe(mMainQueue, [=] { mIceCandidates.push_back(candidate); });
}
void LLVoiceWebRTCConnection::processIceUpdates()
{
    mOutstandingRequests++;
    LLCoros::getInstance()->launch("LLVoiceWebRTCConnection::processIceUpdatesCoro",
                                   boost::bind(&LLVoiceWebRTCConnection::processIceUpdatesCoro, this->shared_from_this()));
}
void LLVoiceWebRTCConnection::processIceUpdatesCoro(connectionPtr_t connection)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (connection->mShutDown || LLWebRTCVoiceClient::isShuttingDown())
    {
        connection->mOutstandingRequests--;
        return;
    }
    LLSD body;
    if (!connection->mIceCandidates.empty() || connection->mIceCompleted)
    {
        LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(connection->mRegionID);
        if (!regionp || !regionp->capabilitiesReceived())
        {
            LL_DEBUGS("Voice") << "no capabilities for ice gathering; waiting " << LL_ENDL;
            connection->mOutstandingRequests--;
            return;
        }
        std::string url = regionp->getCapability("VoiceSignalingRequest");
        if (url.empty())
        {
            connection->mOutstandingRequests--;
            return;
        }
        LL_DEBUGS("Voice") << "region ready to complete voice signaling; url=" << url << LL_ENDL;
        if (!connection->mIceCandidates.empty())
        {
            LLSD candidates = LLSD::emptyArray();
            for (auto &ice_candidate : connection->mIceCandidates)
            {
                LLSD body_candidate;
                body_candidate["sdpMid"]        = ice_candidate.mSdpMid;
                body_candidate["sdpMLineIndex"] = ice_candidate.mMLineIndex;
                body_candidate["candidate"]     = ice_candidate.mCandidate;
                candidates.append(body_candidate);
            }
            body["candidates"] = candidates;
            connection->mIceCandidates.clear();
        }
        else if (connection->mIceCompleted)
        {
            LLSD body_candidate;
            body_candidate["completed"] = true;
            body["candidate"]           = body_candidate;
            connection->mIceCompleted   = false;
        }
        body["viewer_session"]    = connection->mViewerSession;
        body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter =
            std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("LLVoiceWebRTCAdHocConnection::processIceUpdatesCoro",
                                                                   LLCore::HttpRequest::DEFAULT_POLICY_ID);
        LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
        LLCore::HttpOptions::ptr_t httpOpts = std::make_shared<LLCore::HttpOptions>();
        httpOpts->setWantHeaders(true);
        LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);
        if (LLWebRTCVoiceClient::isShuttingDown())
        {
            connection->mOutstandingRequests--;
            return;
        }
        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
        if (!status)
        {
            connection->setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        }
    }
    connection->mOutstandingRequests--;
}
void LLVoiceWebRTCConnection::OnOfferAvailable(const std::string &sdp)
{
    connectionPtr_t connection = shared_from_this();
    LL::WorkQueue::postMaybe(mMainQueue,
        [=] {
            if (connection->mShutDown)
            {
                return;
            }
            LL_DEBUGS("Voice") << "On Offer Available." << LL_ENDL;
            connection->mChannelSDP = sdp;
            if (connection->mVoiceConnectionState == VOICE_STATE_WAIT_FOR_SESSION_START)
            {
                connection->mVoiceConnectionState = VOICE_STATE_REQUEST_CONNECTION;
            }
        });
}
void LLVoiceWebRTCConnection::OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface* audio_interface)
{
    connectionPtr_t connection = shared_from_this();
    LL::WorkQueue::postMaybe(mMainQueue,
        [=] {
            if (connection->mShutDown)
            {
                return;
            }
            LL_DEBUGS("Voice") << "On AudioEstablished." << LL_ENDL;
            connection->mWebRTCAudioInterface = audio_interface;
            connection->mWebRTCAudioInterface->setMute(true);
            connection->setVoiceConnectionState(VOICE_STATE_SESSION_ESTABLISHED);
        });
}
void LLVoiceWebRTCConnection::OnRenegotiationNeeded()
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=] {
            LL_DEBUGS("Voice") << "Voice channel requires renegotiation." << LL_ENDL;
            setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
            mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
        });
}
void LLVoiceWebRTCConnection::OnPeerConnectionClosed()
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=] {
            LL_DEBUGS("Voice") << "Peer connection has closed." << LL_ENDL;
            if (mVoiceConnectionState == VOICE_STATE_WAIT_FOR_CLOSE)
            {
                setVoiceConnectionState(VOICE_STATE_CLOSED);
                mOutstandingRequests--;
            }
            else if (LLWebRTCVoiceClient::isShuttingDown())
            {
                LL_INFOS("Voice") << "Peer connection has closed, but state is " << mVoiceConnectionState << LL_ENDL;
                setVoiceConnectionState(VOICE_STATE_CLOSED);
            }
        });
}
void LLVoiceWebRTCConnection::setMuteMic(bool muted)
{
    mMuted = muted;
    if (mWebRTCAudioInterface)
    {
        mWebRTCAudioInterface->setMute(muted);
    }
}
void LLVoiceWebRTCConnection::setSpeakerVolume(F32 volume)
{
    mSpeakerVolume = volume;
    if (mWebRTCAudioInterface)
    {
        mWebRTCAudioInterface->setReceiveVolume(volume);
    }
}
void LLVoiceWebRTCConnection::setUserVolume(const LLUUID& id, F32 volume)
{
    boost::json::object root      = { { "ug", { { id.asString(), (uint32_t)(volume * PEER_GAIN_CONVERSION_FACTOR) } } } };
    std::string json_data = boost::json::serialize(root);
    if (mWebRTCDataInterface)
    {
        mWebRTCDataInterface->sendData(json_data, false);
    }
}
void LLVoiceWebRTCConnection::setUserMute(const LLUUID& id, bool mute)
{
    boost::json::object root      = { { "m", { { id.asString(), mute } } } };
    std::string         json_data = boost::json::serialize(root);
    if (mWebRTCDataInterface)
    {
        mWebRTCDataInterface->sendData(json_data, false);
    }
}
void LLVoiceWebRTCConnection::sendData(const std::string &data)
{
    if (getVoiceConnectionState() == VOICE_STATE_SESSION_UP && mWebRTCDataInterface)
    {
        mWebRTCDataInterface->sendData(data, false);
    }
}
const std::string& LLVoiceWebRTCConnection::getVersion() {
    return mServerVersion;
}
void LLVoiceWebRTCConnection::breakVoiceConnectionCoro(connectionPtr_t connection)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    LL_INFOS("Voice") << "Disconnecting voice." << LL_ENDL;
    if (connection->mWebRTCDataInterface)
    {
        connection->mWebRTCDataInterface->unsetDataObserver(connection.get());
        connection->mWebRTCDataInterface = nullptr;
    }
    connection->mWebRTCAudioInterface   = nullptr;
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(connection->mRegionID);
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        connection->setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        connection->mOutstandingRequests--;
        return;
    }
    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        connection->setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        connection->mOutstandingRequests--;
        return;
    }
    LL_DEBUGS("Voice") << "region ready for voice break; url=" << url << LL_ENDL;
    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    body["logout"]         = true;
    body["viewer_session"] = connection->mViewerSession;
    body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter =
        std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("LLVoiceWebRTCAdHocConnection::breakVoiceConnection",
                                                               LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t httpOpts = std::make_shared<LLCore::HttpOptions>();
    httpOpts->setWantHeaders(true);
    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);
    connection->mOutstandingRequests--;
    if (connection->getVoiceConnectionState() == VOICE_STATE_WAIT_FOR_EXIT)
    {
        connection->setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
}
void LLVoiceWebRTCSpatialConnection::requestVoiceConnection()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        mOutstandingRequests--;
        return;
    }
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);
    LL_INFOS("Voice") << "Requesting spatial voice connection region=" << mRegionID << LL_ENDL;
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_INFOS("Voice") << "no capabilities for voice provisioning; waiting region=" << mRegionID << LL_ENDL;
        setVoiceConnectionState(VOICE_STATE_REQUEST_CONNECTION);
        mOutstandingRequests--;
        return;
    }
    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        LL_WARNS("Voice") << "ProvisionVoiceAccountRequest cap empty for region=" << mRegionID << LL_ENDL;
        setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        mOutstandingRequests--;
        return;
    }
    LL_INFOS("Voice") << "region ready for voice provisioning; url=" << url << LL_ENDL;
    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    LLSD jsep;
    jsep["type"] = "offer";
    jsep["sdp"] = mChannelSDP;
    body["jsep"] = jsep;
    if (mParcelLocalID != INVALID_PARCEL_ID)
    {
        body["parcel_local_id"] = mParcelLocalID;
    }
    body["channel_type"]      = "local";
    body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter =
        std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("LLVoiceWebRTCAdHocConnection::requestVoiceConnection",
                                                               LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t httpOpts = std::make_shared<LLCore::HttpOptions>();
    httpOpts->setWantHeaders(true);
    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    LL_INFOS("Voice") << "Voice connection request: " << (status ? "Success" : llformat("%d", status.getStatus())) << LL_ENDL;
    if (status)
    {
        OnVoiceConnectionRequestSuccess(result);
    }
    else
    {
        switch (status.getType())
        {
            case HTTP_CONFLICT:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL;
                break;
            case HTTP_UNAUTHORIZED:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED;
                break;
            default:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
                break;
        }
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
    mOutstandingRequests--;
}
void LLVoiceWebRTCConnection::OnVoiceConnectionRequestSuccess(const LLSD &result)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    LLVoiceWebRTCStats::getInstance()->provisionAttemptEnd(true);
    if (result.has("viewer_session") &&
        result.has("jsep") &&
        result["jsep"].has("type") &&
        result["jsep"]["type"] == "answer" &&
        result["jsep"].has("sdp"))
    {
        mRemoteChannelSDP = result["jsep"]["sdp"].asString();
        mViewerSession    = result["viewer_session"];
    }
    else
    {
        LL_WARNS("Voice") << "Invalid voice provision request result:" << result << LL_ENDL;
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
        return;
    }
    LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response"
                       << " channel sdp " << mRemoteChannelSDP << LL_ENDL;
    mWebRTCPeerConnectionInterface->AnswerAvailable(mRemoteChannelSDP);
}
static llwebrtc::LLWebRTCPeerConnectionInterface::InitOptions getConnectionOptions()
{
    llwebrtc::LLWebRTCPeerConnectionInterface::InitOptions options;
    llwebrtc::LLWebRTCPeerConnectionInterface::InitOptions::IceServers servers;
    std::string grid = "agni";
    std::transform(grid.begin(), grid.end(), grid.begin(), [](unsigned char c){ return std::tolower(c); });
    int num_servers = 2;
    if (grid == "agni")
    {
        num_servers = 3;
    }
    for (int i=1; i <= num_servers; i++)
    {
        servers.mUrls.push_back(llformat("stun:stun%d.%s.secondlife.io:3478", i, grid.c_str()));
    }
    options.mServers.push_back(servers);
    return options;
}
bool LLVoiceWebRTCConnection::connectionStateMachine()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (!mShutDown)
    {
        processIceUpdates();
    }
    switch (getVoiceConnectionState())
    {
        case VOICE_STATE_START_SESSION:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VOICE("VOICE_STATE_START_SESSION")
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
                break;
            }
            mIceCompleted = false;
            setVoiceConnectionState(VOICE_STATE_WAIT_FOR_SESSION_START);
            if (!mWebRTCPeerConnectionInterface->initializeConnection(getConnectionOptions()))
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
            }
            break;
        }
        case VOICE_STATE_WAIT_FOR_SESSION_START:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
            }
            break;
        }
        case VOICE_STATE_REQUEST_CONNECTION:
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
                break;
            }
            setVoiceConnectionState(VOICE_STATE_CONNECTION_WAIT);
            mOutstandingRequests++;
            LLCoros::getInstance()->launch("LLVoiceWebRTCConnection::requestVoiceConnectionCoro",
                                           boost::bind(&LLVoiceWebRTCConnection::requestVoiceConnectionCoro, this->shared_from_this()));
            break;
        case VOICE_STATE_CONNECTION_WAIT:
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
            }
            break;
        case VOICE_STATE_SESSION_ESTABLISHED:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                break;
            }
            if (isSpatial())
            {
                mPrimary = false;
            }
            mWebRTCAudioInterface->setReceiveVolume(mSpeakerVolume);
            LLWebRTCVoiceClient::getInstance()->OnConnectionEstablished(mChannelID, mRegionID);
            resetConnectionStats();
            setVoiceConnectionState(VOICE_STATE_WAIT_FOR_DATA_CHANNEL);
            break;
        }
        case VOICE_STATE_WAIT_FOR_DATA_CHANNEL:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                break;
            }
            if (mWebRTCDataInterface)
            {
                sendJoin();
                setVoiceConnectionState(VOICE_STATE_SESSION_UP);
                if (isSpatial())
                {
                    LLWebRTCVoiceClient::getInstance()->updatePosition();
                    LLWebRTCVoiceClient::getInstance()->sendPositionUpdate(true);
                }
                else
                {
                    mWebRTCAudioInterface->setMute(mMuted);
                }
            }
            break;
        }
        case VOICE_STATE_SESSION_UP:
        {
            mRetryWaitPeriod = 0;
            mRetryWaitSecs = (F32)((F32)rand() / (RAND_MAX)) + 0.5f;
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
            }
            else
            {
                if (isSpatial() && gAgent.getRegion())
                {
                    bool primary = (mRegionID == gAgent.getRegion()->getRegionID());
                    if (primary != mPrimary)
                    {
                        mPrimary = primary;
                        if (mWebRTCAudioInterface)
                        {
                            mWebRTCAudioInterface->setMute(mMuted || !mPrimary);
                        }
                        sendJoin();
                    }
                }
                static LLTimer stats_timer;
                if (stats_timer.getElapsedTimeF32() > STATS_TIMER_DELAY)
                {
                    mWebRTCPeerConnectionInterface->gatherConnectionStats();
                    stats_timer.reset();
                }
            }
            break;
        }
        case VOICE_STATE_SESSION_RETRY:
            if (mRetryWaitPeriod++ * UPDATE_THROTTLE_SECONDS > mRetryWaitSecs)
            {
                LLWebRTCVoiceClient::getInstance()->OnConnectionFailure(mChannelID, mRegionID, mCurrentStatus);
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                mRetryWaitPeriod = 0;
                if (mRetryWaitSecs < MAX_RETRY_WAIT_SECONDS)
                {
                    mRetryWaitSecs += (F32)((F32) rand() / (RAND_MAX)) + 0.5f;
                    mRetryWaitPeriod = 0;
                }
            }
            break;
        case VOICE_STATE_DISCONNECT:
            if (!LLWebRTCVoiceClient::isShuttingDown())
            {
                mOutstandingRequests++;
                setVoiceConnectionState(VOICE_STATE_WAIT_FOR_EXIT);
                LLCoros::instance().launch("LLVoiceWebRTCConnection::breakVoiceConnectionCoro",
                                           boost::bind(&LLVoiceWebRTCConnection::breakVoiceConnectionCoro, this->shared_from_this()));
            }
            else
            {
                setVoiceConnectionState(VOICE_STATE_WAIT_FOR_CLOSE);
            }
            break;
        case VOICE_STATE_WAIT_FOR_EXIT:
            break;
        case VOICE_STATE_SESSION_EXIT:
        {
            setVoiceConnectionState(VOICE_STATE_WAIT_FOR_CLOSE);
            mOutstandingRequests++;
            if (!LLWebRTCVoiceClient::isShuttingDown())
            {
                mWebRTCPeerConnectionInterface->shutdownConnection();
            }
            break;
        }
        case VOICE_STATE_WAIT_FOR_CLOSE:
            break;
        case VOICE_STATE_CLOSED:
        {
            if (!mShutDown)
            {
                mVoiceConnectionState = VOICE_STATE_START_SESSION;
            }
            else
            {
                if (mOutstandingRequests <= 0)
                {
                    LLWebRTCVoiceClient::getInstance()->OnConnectionShutDown(mChannelID, mRegionID);
                    return false;
                }
            }
            break;
        }
        default:
        {
            LL_WARNS("Voice") << "Unknown voice control state " << getVoiceConnectionState() << LL_ENDL;
            return false;
        }
    }
    return true;
}
void LLVoiceWebRTCConnection::OnDataReceived(const std::string& data, bool binary)
{
    LL::WorkQueue::postMaybe(mMainQueue, [=] { LLVoiceWebRTCConnection::OnDataReceivedImpl(data, binary); });
}
void LLVoiceWebRTCConnection::OnDataReceivedImpl(const std::string &data, bool binary)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (mShutDown)
    {
        return;
    }
    if (binary)
    {
        LL_WARNS("Voice") << "Binary data received from data channel." << LL_ENDL;
        return;
    }
    boost::system::error_code ec;
    boost::json::value voice_data_parsed = boost::json::parse(data, ec);
    if (!ec)
    {
        if (!voice_data_parsed.is_object())
        {
            LL_WARNS("Voice") << "Expected object from data channel:" << data << LL_ENDL;
            return;
        }
        bool is_primary_region = mPrimary;
        if (!mPrimary && isSpatial() && gAgent.getRegion())
        {
            is_primary_region = (mRegionID == gAgent.getRegion()->getRegionID());
            LL_WARNS() << "mPrimary is false, expected: " << is_primary_region << " connection state: " << getVoiceConnectionState() << LL_ENDL;
        }
        boost::json::object voice_data = voice_data_parsed.as_object();
        boost::json::object mute;
        boost::json::object user_gain;
        for (auto &participant_elem : voice_data)
        {
            boost::json::string participant_id(participant_elem.key());
            LLUUID agent_id(participant_id.c_str());
            if (agent_id.isNull())
            {
               continue;
            }
            if (!participant_elem.value().is_object())
            {
                continue;
            }
            boost::json::object participant_obj = participant_elem.value().as_object();
            if (participant_obj.contains("V") && participant_obj["V"].is_string() && agent_id == gAgentID)
            {
                mServerVersion = participant_obj["V"].as_string().c_str();
                LLWebRTCVoiceClient::getInstance()->updateVersion();
                LL_DEBUGS("Voice") << "Received version string \"" << participant_obj["V"].as_string().c_str()
                                   << "\" for connection: primary=" << mPrimary << ", spatial=" << isSpatial()
                                   << ", region=" << mRegionID << ", mChannelID=" << mChannelID << LL_ENDL;
            }
            LLWebRTCVoiceClient::participantStatePtr_t participant =
                LLWebRTCVoiceClient::getInstance()->findParticipantByID(mChannelID, agent_id);
            bool joined  = false;
            bool primary = false;
            if (participant_obj.contains("j") &&
                participant_obj["j"].is_object())
            {
                joined  = true;
                if (participant_elem.value().as_object()["j"].as_object().contains("p") &&
                    participant_elem.value().as_object()["j"].as_object()["p"].is_bool())
                {
                    primary = participant_elem.value().as_object()["j"].as_object()["p"].as_bool();
                }
                bool isMuted = LLMuteList::getInstance()->isMuted(agent_id, LLMute::flagVoiceChat);
                if (isMuted)
                {
                    mute[participant_id] = true;
                }
                F32 volume;
                if(LLSpeakerVolumeStorage::getInstance()->getSpeakerVolume(agent_id, volume))
                {
                    user_gain[participant_id] = (uint32_t)(volume * 200);
                }
            }
            if (!participant && joined && (primary || !isSpatial()))
            {
                participant = LLWebRTCVoiceClient::getInstance()->addParticipantByID(mChannelID, agent_id, mRegionID);
            }
            if (participant)
            {
                if (participant_obj.contains("l") && participant_obj["l"].is_bool() && participant_obj["l"].as_bool())
                {
                    if (agent_id != gAgentID)
                    {
                        LLWebRTCVoiceClient::getInstance()->removeParticipantByID(mChannelID, agent_id, mRegionID);
                    }
                }
                else
                {
                    if (participant_obj.contains("p") && participant_obj["p"].is_number())
                    {
                        participant->mLevel = (F32)participant_obj["p"].as_int64()/128.0f;
                    }
                    if (participant_obj.contains("v") && participant_obj["v"].is_bool())
                    {
                        participant->mIsSpeaking = participant_obj["v"].as_bool();
                    }
                    if (participant_obj.contains("m") && participant_obj["m"].is_bool())
                    {
                        bool is_moderator_muted = participant_obj["m"].as_bool();
                        if (isSpatial())
                        {
                            if (is_primary_region || primary)
                            {
                                participant->mIsModeratorMuted = is_moderator_muted;
                                if (gAgentID == agent_id)
                                {
                                    LLNearbyVoiceModeration::getInstance()->setMutedInfo(mChannelID, is_moderator_muted);
                                }
                            }
                        }
                        else
                        {
                            participant->mIsModeratorMuted = is_moderator_muted;
                        }
                    }
                }
            }
            else
            {
                if (isSpatial() && (is_primary_region || primary))
                {
                    if (participant_obj.contains("m") && participant_obj["m"].is_bool())
                    {
                        LL_WARNS() << "Mute info msg received: " << participant_obj["m"].as_bool()
                                   << " but participant " << agent_id
                                   << " was not found in channel " << mChannelID << LL_ENDL;
                        bool is_moderator_muted = participant_obj["m"].as_bool();
                        std::string channel_id = mChannelID;
                        F32 delay { 1.5f };
                        doAfterInterval(
                            [channel_id, agent_id, is_moderator_muted]()
                            {
                                LLWebRTCVoiceClient::participantStatePtr_t participant =
                                    LLWebRTCVoiceClient::getInstance()->findParticipantByID(channel_id, agent_id);
                                if (participant)
                                {
                                    participant->mIsModeratorMuted = is_moderator_muted;
                                    LL_WARNS() << "Participant " << agent_id << " is found after delay, is_muted: " << is_moderator_muted << LL_ENDL;
                                    if (gAgentID == agent_id)
                                    {
                                        LLNearbyVoiceModeration::getInstance()->setMutedInfo(channel_id, is_moderator_muted);
                                    }
                                }
                                else
                                {
                                    LL_WARNS() << "Participant " << agent_id << " is still not found in channel " << channel_id << LL_ENDL;
                                }
                            }, delay);
                    }
                }
            }
        }
        boost::json::object root;
        if (mute.size() > 0)
        {
            root["m"] = mute;
        }
        if (user_gain.size() > 0)
        {
            root["ug"] = user_gain;
        }
        if (root.size() > 0 && mWebRTCDataInterface)
        {
            std::string json_data = boost::json::serialize(root);
            mWebRTCDataInterface->sendData(json_data, false);
        }
    }
}
void LLVoiceWebRTCConnection::OnDataChannelReady(llwebrtc::LLWebRTCDataInterface *data_interface)
{
    connectionPtr_t connection = shared_from_this();
    LL::WorkQueue::postMaybe(mMainQueue,
        [=] {
            if (connection->mShutDown)
            {
                return;
            }
            if (data_interface)
            {
                connection->mWebRTCDataInterface = data_interface;
                connection->mWebRTCDataInterface->setDataObserver(connection.get());
            }
        });
}
void LLVoiceWebRTCConnection::sendJoin()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (!mWebRTCDataInterface)
    {
        return;
    }
    LL_INFOS("Voice") << "Sending WebRTC join on channel=" << mChannelID
                      << " primary=" << (mPrimary ? "true" : "false") << LL_ENDL;
    boost::json::object root;
    boost::json::object join_obj;
    if (mPrimary)
    {
        join_obj["p"] = true;
    }
    root["j"]             = join_obj;
    std::string json_data = boost::json::serialize(root);
    mWebRTCDataInterface->sendData(json_data, false);
}
void LLVoiceWebRTCConnection::OnStatsDelivered(const llwebrtc::LLWebRTCStatsMap& stats_data)
{
    (void)stats_data;
}
void LLVoiceWebRTCConnection::resetConnectionStats()
{
}
LLVoiceWebRTCSpatialConnection::LLVoiceWebRTCSpatialConnection(const LLUUID &regionID,
                                                               S32 parcelLocalID,
                                                               const std::string &channelID) :
    LLVoiceWebRTCConnection(regionID, channelID),
    mParcelLocalID(parcelLocalID)
{
    mPrimary = false;
}
LLVoiceWebRTCSpatialConnection::~LLVoiceWebRTCSpatialConnection()
{
}
void LLVoiceWebRTCSpatialConnection::setMuteMic(bool muted)
{
    mMuted = muted;
    if (mWebRTCAudioInterface)
    {
        LLViewerRegion *regionp = gAgent.getRegion();
        if (regionp && mRegionID == regionp->getRegionID())
        {
            mWebRTCAudioInterface->setMute(muted);
        }
        else
        {
            mWebRTCAudioInterface->setMute(true);
        }
    }
}
LLVoiceWebRTCAdHocConnection::LLVoiceWebRTCAdHocConnection(const LLUUID &regionID,
                                                           const std::string& channelID,
                                                           const std::string& credentials) :
    LLVoiceWebRTCConnection(regionID, channelID),
    mCredentials(credentials)
{
}
LLVoiceWebRTCAdHocConnection::~LLVoiceWebRTCAdHocConnection()
{
}
void LLVoiceWebRTCAdHocConnection::requestVoiceConnection()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        mOutstandingRequests--;
        return;
    }
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);
    LL_DEBUGS("Voice") << "Requesting voice connection." << LL_ENDL;
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; retrying " << LL_ENDL;
        setVoiceConnectionState(VOICE_STATE_REQUEST_CONNECTION);
        mOutstandingRequests--;
        return;
    }
    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        mOutstandingRequests--;
        return;
    }
    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    LLSD jsep;
    jsep["type"] = "offer";
    {
        jsep["sdp"] = mChannelSDP;
    }
    body["jsep"] = jsep;
    body["credentials"] = mCredentials;
    body["channel"] = mChannelID;
    body["channel_type"] = "multiagent";
    body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter =
        std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("LLVoiceWebRTCAdHocConnection::requestVoiceConnection",
                                                               LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t httpOpts = std::make_shared<LLCore::HttpOptions>();
    httpOpts->setWantHeaders(true);
    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);
    LLSD               httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status      = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        switch (status.getType())
        {
            case HTTP_CONFLICT:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL;
                break;
            case HTTP_UNAUTHORIZED:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED;
                break;
            default:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
                break;
        }
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
    else
    {
        OnVoiceConnectionRequestSuccess(result);
    }
    mOutstandingRequests--;
}
