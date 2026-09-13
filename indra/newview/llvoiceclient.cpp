/**
 * @file llvoiceclient.cpp
 * @brief Voice client delegation class implementation.
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
#include "llvoiceclient.h"
#include "llvoicewebrtc.h"
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llcommandhandler.h"
#include "lldir.h"
#include "llhttpnode.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llui.h"
#include "llkeyboard.h"
#include "llagent.h"
#include "lltrans.h"
#include "llnearbyvoicemoderation.h"
const F32 LLVoiceClient::OVERDRIVEN_POWER_LEVEL = 0.7f;
const F32 LLVoiceClient::VOLUME_MIN = 0.f;
const F32 LLVoiceClient::VOLUME_DEFAULT = 0.5f;
const F32 LLVoiceClient::VOLUME_MAX = 1.0f;
class LLVoiceHandler : public LLCommandHandler
{
public:
    LLVoiceHandler() : LLCommandHandler("voice", UNTRUSTED_THROTTLE) {}
    bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
    {
        if (params[0].asString() == "effects")
        {
            LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
            if (!effect_interface)
            {
                return false;
            }
            if (params[1].asString() == "refresh")
            {
                effect_interface->refreshVoiceEffectLists(false);
                return true;
            }
        }
        return false;
    }
};
LLVoiceHandler gVoiceHandler;
std::string LLVoiceClientStatusObserver::status2string(LLVoiceClientStatusObserver::EStatusType inStatus)
{
    std::string result = "UNTRANSLATED";
#define CASE(x)  case x:  result = #x;  break
    switch(inStatus)
    {
            CASE(STATUS_LOGIN_RETRY);
            CASE(STATUS_LOGGED_IN);
            CASE(STATUS_JOINING);
            CASE(STATUS_JOINED);
            CASE(STATUS_LEFT_CHANNEL);
            CASE(STATUS_VOICE_DISABLED);
            CASE(STATUS_VOICE_ENABLED);
            CASE(BEGIN_ERROR_STATUS);
            CASE(ERROR_CHANNEL_FULL);
            CASE(ERROR_CHANNEL_LOCKED);
            CASE(ERROR_NOT_AVAILABLE);
            CASE(ERROR_UNKNOWN);
        default:
            {
                std::ostringstream stream;
                stream << "UNKNOWN(" << (int)inStatus << ")";
                result = stream.str();
            }
            break;
    }
#undef CASE
    return result;
}
LLVoiceModuleInterface *getVoiceModule(const std::string &voice_server_type)
{
    std::string type = voice_server_type.empty() ? WEBRTC_VOICE_SERVER_TYPE : voice_server_type;
    if (type == WEBRTC_VOICE_SERVER_TYPE)
    {
        return (LLVoiceModuleInterface *) LLWebRTCVoiceClient::getInstance();
    }
    else
    {
        LLNotificationsUtil::add("VoiceVersionMismatch");
        return nullptr;
    }
}
LLVoiceClient::LLVoiceClient()
    :
    mSpatialVoiceModule(NULL),
    mNonSpatialVoiceModule(NULL),
    m_servicePump(NULL),
    mVoiceEffectEnabled(LLCachedControl<bool>(gSavedSettings, "VoiceMorphingEnabled", true)),
    mVoiceEffectDefault(LLCachedControl<std::string>(gSavedPerAccountSettings, "VoiceEffectDefault", "00000000-0000-0000-0000-000000000000")),
    mVoiceEffectSupportNotified(false),
    mPTTDirty(true),
    mPTT(true),
    mUsePTT(true),
    mPTTMouseButton(0),
    mPTTKey(0),
    mPTTIsMiddleMouse(false),
    mPTTIsToggle(false),
    mUserPTTState(false),
    mMuteMic(false),
    mDisableMic(false)
{
}
LLVoiceClient::~LLVoiceClient()
{
}
void LLVoiceClient::init(LLPumpIO *pump)
{
    m_servicePump = pump;
    LLWebRTCVoiceClient::getInstance()->init(pump);
    std::string voice_server_type = gSavedSettings.getString("VoiceServerType");
    if (voice_server_type.empty())
    {
        voice_server_type = WEBRTC_VOICE_SERVER_TYPE;
    }
    setSpatialVoiceModule(voice_server_type);
}
void LLVoiceClient::userAuthorized(const std::string& user_id, const LLUUID &agentID)
{
    if (mRegionChangedCallbackSlot.connected())
    {
        mRegionChangedCallbackSlot.disconnect();
    }
    mRegionChangedCallbackSlot = gAgent.addRegionChangedCallback(boost::bind(&LLVoiceClient::onRegionChanged, this));
    LLWebRTCVoiceClient::getInstance()->userAuthorized(user_id, agentID);
    onRegionChanged();
}
void LLVoiceClient::handleSimulatorFeaturesReceived(const LLSD &simulatorFeatures)
{
    std::string voiceServerType = simulatorFeatures["VoiceServerType"].asString();
    if (voiceServerType.empty())
    {
        voiceServerType = WEBRTC_VOICE_SERVER_TYPE;
    }
    LL_INFOS("Voice") << "Simulator features VoiceServerType='"
                      << (simulatorFeatures.has("VoiceServerType") ? simulatorFeatures["VoiceServerType"].asString() : std::string("<missing>"))
                      << "' using='" << voiceServerType << "'" << LL_ENDL;
    if (mSpatialVoiceModule && !mNonSpatialVoiceModule)
    {
        LLVoiceVersionInfo version = mSpatialVoiceModule->getVersion();
        if (version.internalVoiceServerType != voiceServerType)
        {
            mSpatialVoiceModule->processChannels(false);
        }
    }
    setSpatialVoiceModule(voiceServerType);
    if (mSpatialVoiceModule && !mNonSpatialVoiceModule)
    {
        if (!mSpatialCredentials.isUndefined())
        {
            mSpatialVoiceModule->setSpatialChannel(mSpatialCredentials);
        }
        LL_INFOS("Voice") << "Enabling spatial processChannels" << LL_ENDL;
        mSpatialVoiceModule->processChannels(true);
    }
    else
    {
        LL_WARNS("Voice") << "Cannot enable spatial channels: module="
                          << (mSpatialVoiceModule ? "set" : "null")
                          << " nonSpatial=" << (mNonSpatialVoiceModule ? "set" : "null") << LL_ENDL;
    }
}
static void simulator_features_received_callback(const LLUUID& region_id)
{
    LLViewerRegion *region = gAgent.getRegion();
    if (region && (region->getRegionID() == region_id))
    {
        LLSD simulatorFeatures;
        region->getSimulatorFeatures(simulatorFeatures);
        if (LLVoiceClient::getInstance())
        {
            LLVoiceClient::getInstance()->handleSimulatorFeaturesReceived(simulatorFeatures);
        }
    }
}
void LLVoiceClient::onRegionChanged()
{
    LLViewerRegion *region = gAgent.getRegion();
    if (region && region->simulatorFeaturesReceived())
    {
        LLSD simulatorFeatures;
        region->getSimulatorFeatures(simulatorFeatures);
        if (LLVoiceClient::getInstance())
        {
            LLVoiceClient::getInstance()->handleSimulatorFeaturesReceived(simulatorFeatures);
        }
    }
    else if (region)
    {
        if (mSimulatorFeaturesReceivedSlot.connected())
        {
            mSimulatorFeaturesReceivedSlot.disconnect();
        }
        mSimulatorFeaturesReceivedSlot =
                region->setSimulatorFeaturesReceivedCallback(boost::bind(&simulator_features_received_callback, _1));
    }
}
void LLVoiceClient::setSpatialVoiceModule(const std::string &voice_server_type)
{
    LLVoiceModuleInterface *module = getVoiceModule(voice_server_type);
    if (!module)
    {
        return;
    }
    if (module != mSpatialVoiceModule)
    {
        if (inProximalChannel())
        {
            mSpatialVoiceModule->processChannels(false);
            module->processChannels(true);
        }
        mSpatialVoiceModule = module;
        mSpatialVoiceModule->updateSettings();
    }
}
void LLVoiceClient::setNonSpatialVoiceModule(const std::string &voice_server_type)
{
    mNonSpatialVoiceModule = getVoiceModule(voice_server_type);
    if (!mNonSpatialVoiceModule)
    {
        if (mSpatialVoiceModule)
        {
            mSpatialVoiceModule->processChannels(true);
        }
        return;
    }
    mNonSpatialVoiceModule->updateSettings();
}
void LLVoiceClient::setHidden(bool hidden)
{
    LL_INFOS("Voice") << "( " << (hidden ? "true" : "false") << " )" << LL_ENDL;
    LLWebRTCVoiceClient::getInstance()->setHidden(hidden);
}
void LLVoiceClient::terminate()
{
    if (LLWebRTCVoiceClient::instanceExists())
    {
        LLWebRTCVoiceClient::getInstance()->terminate();
    }
    mSpatialVoiceModule = NULL;
    m_servicePump = NULL;
    if (LLSpeakerVolumeStorage::instanceExists())
    {
        LLSpeakerVolumeStorage::deleteSingleton();
    }
}
const LLVoiceVersionInfo LLVoiceClient::getVersion()
{
    if (mSpatialVoiceModule)
    {
        return mSpatialVoiceModule->getVersion();
    }
    else
    {
        LLVoiceVersionInfo result;
        result.serverVersion = std::string();
        result.voiceServerType = std::string();
        result.mBuildVersion = std::string();
        return result;
    }
}
void LLVoiceClient::updateSettings()
{
    setUsePTT(gSavedSettings.getBOOL("PTTCurrentlyEnabled"));
    setPTTIsToggle(gSavedSettings.getBOOL("PushToTalkToggle"));
    mDisableMic = gSavedSettings.getBOOL("VoiceDisableMic");
    std::string ptt_button = gSavedSettings.getString("PushToTalkButton");
    mPTTIsMiddleMouse = (ptt_button == "MiddleMouse");
    if (!mPTTIsMiddleMouse)
    {
        KEY key = KEY_NONE;
        if (!LLKeyboard::keyFromString(ptt_button, &key))
        {
            key = KEY_NONE;
        }
        mPTTKey = key;
    }
    else
    {
        mPTTKey = KEY_NONE;
    }
    updateMicMuteLogic();
    LLWebRTCVoiceClient::getInstance()->updateSettings();
}
void LLVoiceClient::tuningStart()
{
    LLWebRTCVoiceClient::getInstance()->tuningStart();
}
void LLVoiceClient::tuningStop()
{
    LLWebRTCVoiceClient::getInstance()->tuningStop();
}
bool LLVoiceClient::inTuningMode()
{
    return LLWebRTCVoiceClient::getInstance()->inTuningMode();
}
void LLVoiceClient::tuningSetMicVolume(float volume)
{
    LLWebRTCVoiceClient::getInstance()->tuningSetMicVolume(volume);
}
void LLVoiceClient::tuningSetSpeakerVolume(float volume)
{
    LLWebRTCVoiceClient::getInstance()->tuningSetSpeakerVolume(volume);
}
float LLVoiceClient::tuningGetEnergy(void)
{
    return LLWebRTCVoiceClient::getInstance()->tuningGetEnergy();
}
bool LLVoiceClient::deviceSettingsAvailable()
{
    return LLWebRTCVoiceClient::getInstance()->deviceSettingsAvailable();
}
bool LLVoiceClient::deviceSettingsUpdated()
{
    return LLWebRTCVoiceClient::getInstance()->deviceSettingsUpdated();
}
void LLVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
    LLWebRTCVoiceClient::getInstance()->refreshDeviceLists(clearCurrentList);
}
void LLVoiceClient::setCaptureDevice(const std::string& name)
{
    LLWebRTCVoiceClient::getInstance()->setCaptureDevice(name);
}
void LLVoiceClient::setRenderDevice(const std::string& name)
{
    LLWebRTCVoiceClient::getInstance()->setRenderDevice(name);
}
const LLVoiceDeviceList& LLVoiceClient::getCaptureDevices()
{
    return LLWebRTCVoiceClient::getInstance()->getCaptureDevices();
}
const LLVoiceDeviceList& LLVoiceClient::getRenderDevices()
{
    return LLWebRTCVoiceClient::getInstance()->getRenderDevices();
}
void LLVoiceClient::getParticipantList(uuid_set_t &participants) const
{
    LLWebRTCVoiceClient::getInstance()->getParticipantList(participants);
}
void LLVoiceClient::declineInvite(const std::string& )
{
}
bool LLVoiceClient::isParticipant(const LLUUID &speaker_id) const
{
    return LLWebRTCVoiceClient::getInstance()->isParticipant(speaker_id);
}
bool LLVoiceClient::isSessionTextIMPossible(const LLUUID& id)
{
    return true;
}
bool LLVoiceClient::isSessionCallBackPossible(const LLUUID& id)
{
    return true;
}
bool LLVoiceClient::inProximalChannel()
{
    if (mSpatialVoiceModule)
    {
        return mSpatialVoiceModule->inProximalChannel();
    }
    else
    {
        return false;
    }
}
void LLVoiceClient::setNonSpatialChannel(
    const LLSD& channelInfo,
    bool notify_on_first_join,
    bool hangup_on_last_leave)
{
    std::string voice_server_type = channelInfo["voice_server_type"].asString();
    if (voice_server_type.empty())
    {
        voice_server_type = WEBRTC_VOICE_SERVER_TYPE;
    }
    setNonSpatialVoiceModule(voice_server_type);
    if (mSpatialVoiceModule && mSpatialVoiceModule != mNonSpatialVoiceModule)
    {
        mSpatialVoiceModule->processChannels(false);
    }
    if (mNonSpatialVoiceModule)
    {
        mNonSpatialVoiceModule->processChannels(true);
        mNonSpatialVoiceModule->setNonSpatialChannel(channelInfo, notify_on_first_join, hangup_on_last_leave);
    }
    else
    {
        LL_WARNS("Voice") << "setNonSpatialChannel: no module for type='" << voice_server_type << "'" << LL_ENDL;
    }
}
void LLVoiceClient::setSpatialChannel(const LLSD &channelInfo)
{
    mSpatialCredentials    = channelInfo;
    LLViewerRegion *region = gAgent.getRegion();
    if (region && region->simulatorFeaturesReceived())
    {
        LLSD simulatorFeatures;
        region->getSimulatorFeatures(simulatorFeatures);
        std::string voiceServerType = simulatorFeatures["VoiceServerType"].asString();
        if (voiceServerType.empty())
        {
            voiceServerType = WEBRTC_VOICE_SERVER_TYPE;
        }
        setSpatialVoiceModule(voiceServerType);
    }
    else
    {
        return;
    }
    if (mSpatialVoiceModule)
    {
        mSpatialVoiceModule->setSpatialChannel(channelInfo);
    }
}
void LLVoiceClient::leaveNonSpatialChannel()
{
    if (mNonSpatialVoiceModule)
    {
        mNonSpatialVoiceModule->leaveNonSpatialChannel();
        mNonSpatialVoiceModule->processChannels(false);
        mNonSpatialVoiceModule = nullptr;
    }
}
void LLVoiceClient::activateSpatialChannel(bool activate)
{
    if (mSpatialVoiceModule)
    {
        mSpatialVoiceModule->processChannels(activate);
    }
}
bool LLVoiceClient::isCurrentChannel(const LLSD& channelInfo)
{
    return LLWebRTCVoiceClient::getInstance()->isCurrentChannel(channelInfo);
}
bool LLVoiceClient::compareChannels(const LLSD &channelInfo1, const LLSD &channelInfo2)
{
    return LLWebRTCVoiceClient::getInstance()->compareChannels(channelInfo1, channelInfo2);
}
LLVoiceP2PIncomingCallInterfacePtr LLVoiceClient::getIncomingCallInterface(const LLSD& voice_call_info)
{
    LLVoiceModuleInterface *module = getVoiceModule(voice_call_info["voice_server_type"]);
    if (module)
    {
        return module->getIncomingCallInterface(voice_call_info);
    }
    return nullptr;
}
LLVoiceP2POutgoingCallInterface *LLVoiceClient::getOutgoingCallInterface(const LLSD& voiceChannelInfo)
{
    std::string voice_server_type = gSavedSettings.getString("VoiceServerType");
    if (voice_server_type.empty())
    {
        LLVoiceVersionInfo versionInfo = LLVoiceClient::getInstance()->getVersion();
        voice_server_type = versionInfo.internalVoiceServerType;
    }
    if (voiceChannelInfo.has("voice_server_type") && voiceChannelInfo["voice_server_type"] != voice_server_type)
    {
        voice_server_type = WEBRTC_VOICE_SERVER_TYPE;
    }
    LLVoiceModuleInterface *module = getVoiceModule(voice_server_type);
    return dynamic_cast<LLVoiceP2POutgoingCallInterface *>(module);
}
void LLVoiceClient::setVoiceVolume(F32 volume)
{
    LLWebRTCVoiceClient::getInstance()->setVoiceVolume(volume);
}
void LLVoiceClient::setMicGain(F32 gain)
{
    LLWebRTCVoiceClient::getInstance()->setMicGain(gain);
}
bool LLVoiceClient::onVoiceEffectsNotSupported(const LLSD &notification, const LLSD &response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch (option)
    {
        case 0:
            gSavedPerAccountSettings.setString("VoiceEffectDefault", LLUUID::null.asString());
            break;
        case 1:
            break;
        default:
            llassert(0);
            break;
    }
    return false;
}
bool LLVoiceClient::voiceEnabled()
{
    static LLCachedControl<bool> enable_voice_chat(gSavedSettings, "EnableVoiceChat");
    static LLCachedControl<bool> cmd_line_disable_voice(gSavedSettings, "CmdLineDisableVoice");
    bool enabled = enable_voice_chat && !cmd_line_disable_voice;
    if (enabled && !mVoiceEffectSupportNotified && getVoiceEffectEnabled() && !getVoiceEffectDefault().isNull())
    {
        LLSD args;
        args["FAQ_URL"] = LLTrans::getString("no_voice_morphing_faq_url");
        LLNotificationsUtil::add("VoiceEffectsNotSupported", args, LLSD(), &LLVoiceClient::onVoiceEffectsNotSupported);
        mVoiceEffectSupportNotified = true;
    }
    return enabled;
}
void LLVoiceClient::setVoiceEnabled(bool enabled)
{
    if (LLWebRTCVoiceClient::instanceExists())
    {
        LLWebRTCVoiceClient::getInstance()->setVoiceEnabled(enabled);
    }
}
void LLVoiceClient::updateMicMuteLogic()
{
    bool new_mic_mute = false;
    if(mUsePTT)
    {
        new_mic_mute = !mUserPTTState;
    }
    if(mMuteMic || mDisableMic)
    {
        new_mic_mute = true;
    }
    LLWebRTCVoiceClient::getInstance()->setMuteMic(new_mic_mute);
}
void LLVoiceClient::setMuteMic(bool muted)
{
    if (mMuteMic != muted)
    {
        mMuteMic = muted;
        updateMicMuteLogic();
        mMicroChangedSignal();
    }
}
void LLVoiceClient::setUserPTTState(bool ptt)
{
    if (ptt)
    {
        if (!mUserPTTState && LLNearbyVoiceModeration::getInstance()->showNotificationIfNeeded())
            return;
    }
    mUserPTTState = ptt;
    updateMicMuteLogic();
    mMicroChangedSignal();
}
bool LLVoiceClient::getUserPTTState()
{
    return mUserPTTState;
}
void LLVoiceClient::setUsePTT(bool usePTT)
{
    if(usePTT && !mUsePTT)
    {
        mUserPTTState = false;
    }
    mUsePTT = usePTT;
    updateMicMuteLogic();
}
void LLVoiceClient::setPTTIsToggle(bool PTTIsToggle)
{
    if(!PTTIsToggle && mPTTIsToggle)
    {
        mUserPTTState = false;
    }
    mPTTIsToggle = PTTIsToggle;
    updateMicMuteLogic();
}
bool LLVoiceClient::getPTTIsToggle()
{
    return mPTTIsToggle;
}
void LLVoiceClient::inputUserControlState(bool down)
{
    if(mPTTIsToggle)
    {
        if(down)
        {
            toggleUserPTTState();
        }
    }
    else
    {
        setUserPTTState(down);
    }
}
void LLVoiceClient::toggleUserPTTState(void)
{
    setUserPTTState(!getUserPTTState());
}
void LLVoiceClient::keyDown(KEY key, MASK mask)
{
    if (gKeyboard->getKeyRepeated(key))
    {
        return;
    }
    if (!mPTTIsMiddleMouse && mPTTKey != KEY_NONE)
    {
        bool down = gKeyboard->getKeyDown(mPTTKey);
        if (down)
        {
            inputUserControlState(down);
        }
    }
}
void LLVoiceClient::keyUp(KEY key, MASK mask)
{
    if (!mPTTIsMiddleMouse && mPTTKey != KEY_NONE)
    {
        bool down = gKeyboard->getKeyDown(mPTTKey);
        if (!down)
        {
            inputUserControlState(down);
        }
    }
}
void LLVoiceClient::middleMouseState(bool down)
{
    if (mPTTIsMiddleMouse)
    {
        inputUserControlState(down);
    }
}
void LLVoiceClient::leaveChannel()
{
    leaveNonSpatialChannel();
}
void LLVoiceClient::endUserIMSession(const LLUUID& )
{
}
bool LLVoiceClient::getVoiceEnabled(const LLUUID& id) const
{
    return isParticipant(id);
}
std::string LLVoiceClient::getDisplayName(const LLUUID& id) const
{
    std::string result = LLWebRTCVoiceClient::getInstance()->getDisplayName(id);
    return result;
}
bool LLVoiceClient::isVoiceWorking() const
{
    return LLWebRTCVoiceClient::getInstance()->isVoiceWorking();
}
bool LLVoiceClient::isParticipantAvatar(const LLUUID& id)
{
    return true;
}
bool LLVoiceClient::isOnlineSIP(const LLUUID& id)
{
    return false;
}
bool LLVoiceClient::getIsSpeaking(const LLUUID& id)
{
    return LLWebRTCVoiceClient::getInstance()->getIsSpeaking(id);
}
bool LLVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
    return LLWebRTCVoiceClient::getInstance()->getIsModeratorMuted(id);
}
F32 LLVoiceClient::getCurrentPower(const LLUUID& id)
{
    return LLWebRTCVoiceClient::getInstance()->getCurrentPower(id);
}
bool LLVoiceClient::getOnMuteList(const LLUUID& id)
{
    return LLMuteList::getInstance()->isMuted(id, LLMute::flagVoiceChat);
}
F32 LLVoiceClient::getUserVolume(const LLUUID& id)
{
    return LLWebRTCVoiceClient::getInstance()->getUserVolume(id);
}
void LLVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
    LLWebRTCVoiceClient::getInstance()->setUserVolume(id, volume);
}
void LLVoiceClient::addObserver(LLVoiceClientStatusObserver* observer)
{
    LLWebRTCVoiceClient::getInstance()->addObserver(observer);
}
void LLVoiceClient::removeObserver(LLVoiceClientStatusObserver* observer)
{
    if (LLWebRTCVoiceClient::instanceExists())
    {
        LLWebRTCVoiceClient::getInstance()->removeObserver(observer);
    }
}
void LLVoiceClient::addObserver(LLFriendObserver* observer)
{
    LLWebRTCVoiceClient::getInstance()->addObserver(observer);
}
void LLVoiceClient::removeObserver(LLFriendObserver* observer)
{
    if (LLWebRTCVoiceClient::instanceExists())
    {
        LLWebRTCVoiceClient::getInstance()->removeObserver(observer);
    }
}
void LLVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
    LLWebRTCVoiceClient::getInstance()->addObserver(observer);
}
void LLVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
    if (LLWebRTCVoiceClient::instanceExists())
    {
        LLWebRTCVoiceClient::getInstance()->removeObserver(observer);
    }
}
LLSD LLVoiceClient::getP2PChannelInfoTemplate(const LLUUID& id) const
{
    if (mNonSpatialVoiceModule)
    {
        return mNonSpatialVoiceModule->getP2PChannelInfoTemplate(id);
    }
    else if (mSpatialVoiceModule)
    {
        return mSpatialVoiceModule->getP2PChannelInfoTemplate(id);
    }
    else
    {
        return LLSD();
    }
}
LLVoiceEffectInterface* LLVoiceClient::getVoiceEffectInterface() const
{
    return NULL;
}
class LLViewerRequiredVoiceVersion : public LLHTTPNode
{
    static bool sAlertedUser;
    virtual void post(
                      LLHTTPNode::ResponsePtr response,
                      const LLSD& context,
                      const LLSD& input) const
    {
        std::string voice_server_type = "webrtc";
        if (input.has("body") && input["body"].has("voice_server_type"))
        {
            voice_server_type = input["body"]["voice_server_type"].asString();
        }
        LLVoiceModuleInterface *voiceModule = NULL;
        if (voice_server_type == "webrtc" || voice_server_type.empty())
        {
            voiceModule = (LLVoiceModuleInterface *) LLWebRTCVoiceClient::getInstance();
        }
        else
        {
            LL_WARNS("Voice") << "Unknown voice server type " << voice_server_type << LL_ENDL;
            if (!sAlertedUser)
            {
                LLNotificationsUtil::add("VoiceVersionMismatch");
            }
            return;
        }
        LLVoiceVersionInfo versionInfo = voiceModule->getVersion();
        if (input.has("body") && input["body"].has("major_version") &&
            input["body"]["major_version"].asInteger() > versionInfo.majorVersion)
        {
            if (!sAlertedUser)
            {
                LLNotificationsUtil::add("VoiceVersionMismatch");
                LL_WARNS("Voice") << "Voice server version mismatch " << input["body"]["major_version"].asInteger() << "/"
                                  << versionInfo.majorVersion
                                  << LL_ENDL;
            }
            return;
        }
    }
};
class LLViewerParcelVoiceInfo : public LLHTTPNode
{
    virtual void post(
                      LLHTTPNode::ResponsePtr response,
                      const LLSD& context,
                      const LLSD& input) const
    {
        if ( input.has("body") )
        {
            LLSD body = input["body"];
            if ( body.has("voice_credentials") )
            {
                LLVoiceClient::getInstance()->setSpatialChannel(body["voice_credentials"]);
            }
        }
    }
};
const std::string LLSpeakerVolumeStorage::SETTINGS_FILE_NAME = "volume_settings.xml";
LLSpeakerVolumeStorage::LLSpeakerVolumeStorage()
{
    load();
}
LLSpeakerVolumeStorage::~LLSpeakerVolumeStorage()
{
    save();
}
void LLSpeakerVolumeStorage::storeSpeakerVolume(const LLUUID& speaker_id, F32 volume)
{
    if ((volume >= LLVoiceClient::VOLUME_MIN) && (volume <= LLVoiceClient::VOLUME_MAX))
    {
        mSpeakersData[speaker_id] = volume;
    }
    else
    {
        LL_WARNS("Voice") << "Attempted to store out of range volume " << volume << " for " << speaker_id << LL_ENDL;
        llassert(0);
    }
}
bool LLSpeakerVolumeStorage::getSpeakerVolume(const LLUUID& speaker_id, F32& volume)
{
    speaker_data_map_t::const_iterator it = mSpeakersData.find(speaker_id);
    if (it != mSpeakersData.end())
    {
        volume = it->second;
        return true;
    }
    return false;
}
void LLSpeakerVolumeStorage::removeSpeakerVolume(const LLUUID& speaker_id)
{
    mSpeakersData.erase(speaker_id);
}
F32 LLSpeakerVolumeStorage::transformFromLegacyVolume(F32 volume_in)
{
    F32 volume_out = 0.f;
    volume_in = llclamp(volume_in, 0.f, 1.0f);
    if (volume_in <= 0.5f)
    {
        volume_out = volume_in * volume_in * 4.f * 0.56f;
    }
    else
    {
        volume_out = (1.f - 0.56f) * (4.f * volume_in * volume_in - 1.f) / 3.f + 0.56f;
    }
    return volume_out;
}
F32 LLSpeakerVolumeStorage::transformToLegacyVolume(F32 volume_in)
{
    F32 volume_out = 0.f;
    volume_in = llclamp(volume_in, 0.f, 1.0f);
    if (volume_in <= 0.56f)
    {
        volume_out = sqrt(volume_in / (4.f * 0.56f));
    }
    else
    {
        volume_out = sqrt((3.f * (volume_in - 0.56f) / (1.f - 0.56f) + 1.f) / 4.f);
    }
    return volume_out;
}
void LLSpeakerVolumeStorage::load()
{
    std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);
    LL_INFOS("Voice") << "Loading stored speaker volumes from: " << filename << LL_ENDL;
    LLSD settings_llsd;
    llifstream file;
    file.open(filename.c_str());
    if (file.is_open())
    {
        if (LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXML(settings_llsd, file))
        {
            LL_WARNS("Voice") << "failed to parse " << filename << LL_ENDL;
        }
    }
    for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
        iter != settings_llsd.endMap(); ++iter)
    {
        F32 volume = transformFromLegacyVolume((F32)iter->second.asReal());
        storeSpeakerVolume(LLUUID(iter->first), volume);
    }
}
void LLSpeakerVolumeStorage::save()
{
    std::string user_dir = gDirUtilp->getLindenUserDir();
    if (!user_dir.empty())
    {
        std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);
        LLSD settings_llsd;
        LL_INFOS("Voice") << "Saving stored speaker volumes to: " << filename << LL_ENDL;
        for(speaker_data_map_t::const_iterator iter = mSpeakersData.begin(); iter != mSpeakersData.end(); ++iter)
        {
            F32 volume = transformToLegacyVolume(iter->second);
            settings_llsd[iter->first.asString()] = volume;
        }
        llofstream file;
        file.open(filename.c_str());
        LLSDSerialize::toPrettyXML(settings_llsd, file);
    }
}
bool LLViewerRequiredVoiceVersion::sAlertedUser = false;
LLHTTPRegistration<LLViewerParcelVoiceInfo>
gHTTPRegistrationMessageParcelVoiceInfo(
                                        "/message/ParcelVoiceInfo");
LLHTTPRegistration<LLViewerRequiredVoiceVersion>
gHTTPRegistrationMessageRequiredVoiceVersion(
                                             "/message/RequiredVoiceVersion");
bool LLVoiceClient::lipSyncEnabled()
{
    return gSavedSettings.getBOOL("LipSyncEnabled");
}
void LLVoiceClient::setLipSyncEnabled(bool enabled)
{
    gSavedSettings.setBOOL("LipSyncEnabled", enabled);
}
