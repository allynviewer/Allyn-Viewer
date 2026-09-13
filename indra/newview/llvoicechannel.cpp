/**
 * @file llvoicechannel.cpp
 * @brief Voice Channel related classes
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
#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "llimview.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanel.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llvoicechannel.h"
#include "llvoicewebrtc.h"
#include "llcorehttputil.h"
#include "lleventcoro.h"
#include "llcoros.h"
LLVoiceChannel::voice_channel_map_t LLVoiceChannel::sVoiceChannelMap;
LLVoiceChannel* LLVoiceChannel::sCurrentVoiceChannel = NULL;
LLVoiceChannel* LLVoiceChannel::sSuspendedVoiceChannel = NULL;
LLVoiceChannel::channel_changed_signal_t LLVoiceChannel::sCurrentVoiceChannelChangedSignal;
bool LLVoiceChannel::sSuspended = false;
const U32 DEFAULT_RETRIES_COUNT = 3;
LLVoiceChannel::LLVoiceChannel(const LLUUID& session_id, const std::string& session_name) :
    mSessionID(session_id),
    mState(STATE_NO_CHANNEL_INFO),
    mSessionName(session_name),
    mCallDirection(OUTGOING_CALL),
    mIgnoreNextSessionLeave(false),
    mCallEndedByAgent(false),
    mCapRequestPending(false)
{
    mNotifyArgs["VOICE_CHANNEL_NAME"] = mSessionName;
    if (!sVoiceChannelMap.insert(std::make_pair(session_id, this)).second)
    {
        LL_WARNS("Voice") << "Duplicate voice channels registered for session_id " << session_id << LL_ENDL;
    }
}
LLVoiceChannel::~LLVoiceChannel()
{
    if (sSuspendedVoiceChannel == this)
    {
        sSuspendedVoiceChannel = NULL;
    }
    if (sCurrentVoiceChannel == this)
    {
        sCurrentVoiceChannel = NULL;
    }
    LLVoiceClient::removeObserver(this);
    sVoiceChannelMap.erase(mSessionID);
}
void LLVoiceChannel::setChannelInfo(const LLSD &channelInfo)
{
    mChannelInfo     = channelInfo;
    if (mState == STATE_NO_CHANNEL_INFO)
    {
        if (mChannelInfo.isUndefined() || !mChannelInfo.isMap() || mChannelInfo.size() == 0)
        {
            LLNotificationsUtil::add("VoiceChannelJoinFailed", mNotifyArgs);
            LL_WARNS("Voice") << "Received empty channel info for channel " << mSessionName << LL_ENDL;
            deactivate();
        }
        else
        {
            setState(STATE_READY);
            if (sCurrentVoiceChannel == this)
            {
                activate();
            }
        }
    }
}
void LLVoiceChannel::resetChannelInfo()
{
    mChannelInfo = LLSD();
    mState = STATE_NO_CHANNEL_INFO;
}
void LLVoiceChannel::onChange(EStatusType type, const LLSD& channelInfo, bool proximal)
{
    LL_DEBUGS("Voice") << "Incoming channel info: " << channelInfo << LL_ENDL;
    LL_DEBUGS("Voice") << "Current channel info: " << mChannelInfo << LL_ENDL;
    if (mChannelInfo.isUndefined() || (mChannelInfo.isMap() && mChannelInfo.size() == 0))
    {
        mChannelInfo = channelInfo;
    }
    if (!LLVoiceClient::instanceExists())
    {
        return;
    }
    if (!LLVoiceClient::getInstance()->compareChannels(mChannelInfo, channelInfo))
    {
        return;
    }
    if (type < BEGIN_ERROR_STATUS)
    {
        handleStatusChange(type);
    }
    else
    {
        handleError(type);
    }
}
void LLVoiceChannel::handleStatusChange(EStatusType type)
{
    switch(type)
    {
    case STATUS_LOGIN_RETRY:
        break;
    case STATUS_LOGGED_IN:
        break;
    case STATUS_LEFT_CHANNEL:
        if (callStarted() && !sSuspended)
        {
            deactivate();
        }
        break;
    case STATUS_JOINING:
        if (callStarted())
        {
            setState(STATE_RINGING);
        }
        break;
    case STATUS_JOINED:
        if (callStarted())
        {
            setState(STATE_CONNECTED);
        }
    default:
        break;
    }
}
void LLVoiceChannel::handleError(EStatusType type)
{
    deactivate();
    setState(STATE_ERROR);
}
bool LLVoiceChannel::isActive() const
{
    return callStarted() && LLVoiceClient::getInstance()->isCurrentChannel(mChannelInfo);
}
bool LLVoiceChannel::callStarted() const
{
    return mState >= STATE_CALL_STARTED;
}
void LLVoiceChannel::deactivate()
{
    if (mState >= STATE_RINGING)
    {
        mIgnoreNextSessionLeave = true;
    }
    if (callStarted())
    {
        setState(STATE_HUNG_UP);
        if (gSavedSettings.getBOOL("AutoDisengageMic") &&
            sCurrentVoiceChannel == this &&
            LLVoiceClient::getInstance()->getUserPTTState())
        {
            gSavedSettings.setBOOL("PTTCurrentlyEnabled", true);
            LLVoiceClient::getInstance()->setUserPTTState(false);
        }
    }
    LLVoiceClient::removeObserver(this);
    if (sCurrentVoiceChannel == this)
    {
        sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
        sCurrentVoiceChannel->activate();
    }
}
void LLVoiceChannel::activate()
{
    if (callStarted())
    {
        return;
    }
    if (sCurrentVoiceChannel != this)
    {
        LLVoiceChannel* old_channel = sCurrentVoiceChannel;
        sCurrentVoiceChannel = this;
        if (old_channel)
        {
            old_channel->deactivate();
        }
    }
    if (mState == STATE_NO_CHANNEL_INFO)
    {
        requestChannelInfo();
    }
    else
    {
        setState(STATE_CALL_STARTED);
    }
    LLVoiceClient::addObserver(this);
    sCurrentVoiceChannelChangedSignal(this->mSessionID);
}
void LLVoiceChannel::requestChannelInfo()
{
    if (sCurrentVoiceChannel == this)
    {
        setState(STATE_CALL_STARTED);
    }
}
LLVoiceChannel* LLVoiceChannel::getChannelByID(const LLUUID& session_id)
{
    voice_channel_map_t::iterator found_it = sVoiceChannelMap.find(session_id);
    if (found_it == sVoiceChannelMap.end())
    {
        return NULL;
    }
    else
    {
        return found_it->second;
    }
}
LLVoiceChannel* LLVoiceChannel::getCurrentVoiceChannel()
{
    return sCurrentVoiceChannel;
}
void LLVoiceChannel::updateSessionID(const LLUUID& new_session_id)
{
    sVoiceChannelMap.erase(sVoiceChannelMap.find(mSessionID));
    mSessionID = new_session_id;
    sVoiceChannelMap.insert(std::make_pair(mSessionID, this));
}
void LLVoiceChannel::setState(EState state)
{
    switch(state)
    {
    case STATE_RINGING:
        break;
    case STATE_CONNECTED:
        break;
    case STATE_HUNG_UP:
        break;
    default:
        break;
    }
    doSetState(state);
}
void LLVoiceChannel::doSetState(const EState& new_state)
{
    LL_DEBUGS("Voice") << "session '" << mSessionName << "' state " << mState << ", new_state " << new_state << ": "
        << (new_state == STATE_ERROR ? "ERROR" :
            new_state == STATE_HUNG_UP ? "HUNG_UP" :
            new_state == STATE_READY ? "READY" :
            new_state == STATE_CALL_STARTED ? "CALL_STARTED" :
            new_state == STATE_RINGING ? "RINGING" :
            new_state == STATE_CONNECTED ? "CONNECTED" :
            "NO_INFO")
        << LL_ENDL;
    EState old_state = mState;
    mState = new_state;
    if (!mStateChangedCallback.empty())
        mStateChangedCallback(old_state, mState, mCallDirection, mCallEndedByAgent, mSessionID);
}
void LLVoiceChannel::initClass()
{
    sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
}
void LLVoiceChannel::suspend()
{
    if (!sSuspended)
    {
        sSuspendedVoiceChannel = sCurrentVoiceChannel;
        sSuspended = true;
        sCurrentVoiceChannelChangedSignal(sSuspendedVoiceChannel->mSessionID);
    }
}
void LLVoiceChannel::resume()
{
    if (sSuspended)
    {
        sSuspended = false;
        if (LLVoiceClient::getInstance()->voiceEnabled())
        {
            if (sSuspendedVoiceChannel)
            {
                if (sSuspendedVoiceChannel->callStarted())
                {
                    sSuspendedVoiceChannel->setState(STATE_READY);
                }
                sSuspendedVoiceChannel->activate();
            }
            else
            {
                LLVoiceChannelProximal::getInstance()->activate();
            }
        }
    }
}
boost::signals2::connection LLVoiceChannel::setCurrentVoiceChannelChangedCallback(channel_changed_callback_t cb, bool at_front)
{
    if (at_front)
    {
        return sCurrentVoiceChannelChangedSignal.connect(cb,  boost::signals2::at_front);
    }
    else
    {
        return sCurrentVoiceChannelChangedSignal.connect(cb);
    }
}
LLVoiceChannelGroup::LLVoiceChannelGroup(const LLUUID      &session_id,
                                         const std::string &session_name,
                                         bool               is_p2p) :
                                         LLVoiceChannel(session_id, session_name),
                                         mIsP2P(is_p2p)
{
    mRetries = DEFAULT_RETRIES_COUNT;
    mIsRetrying = false;
}
void LLVoiceChannelGroup::deactivate()
{
    if (callStarted())
    {
        LLVoiceClient::getInstance()->leaveNonSpatialChannel();
    }
    LLVoiceChannel::deactivate();
    if (mIsP2P)
    {
        setState(STATE_NO_CHANNEL_INFO);
    }
 }
void LLVoiceChannelGroup::activate()
{
    if (callStarted()) return;
    LLVoiceChannel::activate();
    if (callStarted())
    {
        LLVoiceClient::getInstance()->setNonSpatialChannel(mChannelInfo,
                                                           mIsP2P && (mCallDirection == OUTGOING_CALL),
                                                           mIsP2P);
    }
}
void LLVoiceChannelGroup::requestChannelInfo()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        std::string url = region->getCapability("ChatSessionRequest");
        LLCoros::instance().launch("LLVoiceChannelGroup::voiceCallCapCoro",
            boost::bind(&LLVoiceChannelGroup::voiceCallCapCoro, this, url));
    }
}
void LLVoiceChannelGroup::setChannelInfo(const LLSD& channelInfo)
{
    mChannelInfo = channelInfo;
    if (mState == STATE_NO_CHANNEL_INFO)
    {
        if(mChannelInfo.isDefined() && mChannelInfo.isMap())
        {
            setState(STATE_READY);
            if (sCurrentVoiceChannel == this)
            {
                activate();
            }
        }
        else
        {
            LL_WARNS("Voice") << "Received invalid credentials for channel " << mSessionName << LL_ENDL;
            deactivate();
        }
    }
    else if ( mIsRetrying )
    {
        LLVoiceClient::getInstance()->setNonSpatialChannel(channelInfo,
                                                           mCallDirection == OUTGOING_CALL,
                                                           mIsP2P);
    }
}
void LLVoiceChannelGroup::handleStatusChange(EStatusType type)
{
    switch(type)
    {
    case STATUS_JOINED:
        mRetries = 3;
        mIsRetrying = false;
        LLVoiceClient::getInstance()->setUserPTTState(mIsP2P);
        break;
    default:
        break;
    }
    LLVoiceChannel::handleStatusChange(type);
}
void LLVoiceChannelGroup::handleError(EStatusType status)
{
    std::string notify;
    switch(status)
    {
    case ERROR_CHANNEL_LOCKED:
    case ERROR_CHANNEL_FULL:
        notify = "VoiceChannelFull";
        break;
    case ERROR_NOT_AVAILABLE:
        if ( mRetries > 0 )
        {
            mRetries--;
            mIsRetrying = true;
            mIgnoreNextSessionLeave = true;
            requestChannelInfo();
            return;
        }
        else
        {
            notify = "VoiceChannelJoinFailed";
            mRetries = DEFAULT_RETRIES_COUNT;
            mIsRetrying = false;
        }
        break;
    case ERROR_UNKNOWN:
    default:
        break;
    }
    if (!notify.empty())
    {
        LLNotificationPtr notification = LLNotificationsUtil::add(notify, mNotifyArgs);
        gIMMgr->addMessage(mSessionID, LLUUID::null, SYSTEM_FROM, notification->getMessage());
    }
    LLVoiceChannel::handleError(status);
}
void LLVoiceChannelGroup::setState(EState state)
{
    switch(state)
    {
    case STATE_RINGING:
        if ( !mIsRetrying )
        {
        }
        doSetState(state);
        break;
    default:
        LLVoiceChannel::setState(state);
    }
}
void LLVoiceChannelGroup::voiceCallCapCoro(std::string url)
{
    const bool track_cap_pending = mIsP2P;
    if (track_cap_pending)
    {
        setCapRequestPending(true);
    }
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("voiceCallCapCoro", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
    LLSD postData;
    postData["method"] = "call";
    postData["session-id"] = mSessionID;
    LLSD altParams;
    std::string  preferred_voice_server_type = gSavedSettings.getString("VoiceServerType");
    if (preferred_voice_server_type.empty())
    {
        LLVoiceVersionInfo versionInfo = LLVoiceClient::getInstance()->getVersion();
        preferred_voice_server_type = versionInfo.internalVoiceServerType;
    }
    if (preferred_voice_server_type.empty())
    {
        preferred_voice_server_type = WEBRTC_VOICE_SERVER_TYPE;
    }
    altParams["preferred_voice_server_type"] = preferred_voice_server_type;
    postData["alt_params"] = altParams;
    LL_INFOS("Voice", "voiceCallCapCoro") << "Generic POST for " << url
                                          << " session=" << mSessionID
                                          << " preferred_type=" << preferred_voice_server_type
                                          << " p2p=" << (mIsP2P ? 1 : 0) << LL_ENDL;
    LLSD result;
    LLCore::HttpStatus status;
    const int max_attempts = 3;
    for (int attempt = 1; attempt <= max_attempts; ++attempt)
    {
        result = httpAdapter->postAndSuspend(httpRequest, url, postData);
        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
        if (status)
        {
            break;
        }
        LL_WARNS("Voice") << "voiceCallCapCoro attempt " << attempt << "/" << max_attempts
                          << " failed status=" << status.getStatus()
                          << " reason=" << (httpResults.has("reason") ? httpResults["reason"].asString() : std::string("<none>"))
                          << " body=" << result << LL_ENDL;
        if (attempt < max_attempts)
        {
            llcoro::suspendUntilTimeout(1.0f);
        }
    }
    LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(mSessionID);
    if (!channelp)
    {
        LL_WARNS("Voice") << "Unable to retrieve channel with Id = " << mSessionID << LL_ENDL;
        return;
    }
    if (track_cap_pending)
    {
        channelp->setCapRequestPending(false);
    }
    if (!status)
    {
        if (status == LLCore::HttpStatus(HTTP_FORBIDDEN))
        {
            LLNotificationsUtil::add(
                "VoiceNotAllowed",
                channelp->getNotifyArgs());
        }
        else
        {
            LLNotificationsUtil::add(
                "VoiceCallGenericError",
                channelp->getNotifyArgs());
        }
        channelp->deactivate();
        return;
    }
    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    LL_INFOS("Voice") << "LLVoiceChannelGroup::voiceCallCapCoro got " << result << LL_ENDL;
    LLSD credentials = result["voice_credentials"];
    if (!credentials.isMap() || credentials.size() == 0)
    {
        LL_WARNS("Voice") << "voiceCallCapCoro: missing voice_credentials in " << result << LL_ENDL;
        LLNotificationsUtil::add("VoiceCallGenericError", channelp->getNotifyArgs());
        channelp->deactivate();
        return;
    }
    if (!credentials.has("voice_server_type") || credentials["voice_server_type"].asString().empty())
    {
        credentials["voice_server_type"] = preferred_voice_server_type;
    }
    channelp->setChannelInfo(credentials);
}
LLVoiceChannelProximal::LLVoiceChannelProximal() :
    LLVoiceChannel(LLUUID::null, LLStringUtil::null)
{
}
bool LLVoiceChannelProximal::isActive() const
{
    return callStarted() && LLVoiceClient::getInstance()->inProximalChannel();
}
void LLVoiceChannelProximal::activate()
{
    if (callStarted()) return;
    if((LLVoiceChannel::sCurrentVoiceChannel != this) && (LLVoiceChannel::getState() == STATE_CONNECTED))
    {
        LLVoiceClient::getInstance()->leaveNonSpatialChannel();
    }
    LLVoiceClient::getInstance()->activateSpatialChannel(true);
    LLVoiceChannel::activate();
}
void LLVoiceChannelProximal::onChange(EStatusType type, const LLSD& channelInfo, bool proximal)
{
    if (!proximal)
    {
        return;
    }
    if (type < BEGIN_ERROR_STATUS)
    {
        handleStatusChange(type);
    }
    else
    {
        handleError(type);
    }
}
void LLVoiceChannelProximal::handleStatusChange(EStatusType status)
{
    switch(status)
    {
    case STATUS_LEFT_CHANNEL:
        return;
    case STATUS_VOICE_DISABLED:
        LLVoiceClient::getInstance()->setUserPTTState(false);
        gAgent.setVoiceConnected(false);
        if(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking())
        {
        }
        return;
    default:
        break;
    }
    LLVoiceChannel::handleStatusChange(status);
}
void LLVoiceChannelProximal::handleError(EStatusType status)
{
    std::string notify;
    switch(status)
    {
      case ERROR_CHANNEL_LOCKED:
      case ERROR_CHANNEL_FULL:
        notify = "ProximalVoiceChannelFull";
        break;
      default:
         break;
    }
    if (!notify.empty())
    {
        LLNotificationsUtil::add(notify, mNotifyArgs);
    }
}
void LLVoiceChannelProximal::deactivate()
{
    if (callStarted())
    {
        setState(STATE_HUNG_UP);
    }
    LLVoiceClient::removeObserver(this);
    LLVoiceClient::getInstance()->activateSpatialChannel(false);
}
LLVoiceChannelP2P::LLVoiceChannelP2P(const LLUUID      &session_id,
                                     const std::string &session_name,
                                     const LLUUID      &other_user_id,
                                    LLVoiceP2POutgoingCallInterface* outgoing_call_interface) :
    LLVoiceChannelGroup(session_id, session_name, true),
    mOtherUserID(other_user_id),
    mReceivedCall(false),
    mOutgoingCallInterface(outgoing_call_interface)
{
    mChannelInfo = LLVoiceClient::getInstance()->getP2PChannelInfoTemplate(other_user_id);
}
void LLVoiceChannelP2P::handleStatusChange(EStatusType type)
{
    LL_INFOS("Voice") << "P2P CALL CHANNEL STATUS CHANGE: incoming=" << int(mReceivedCall) << " newstatus=" << LLVoiceClientStatusObserver::status2string(type) << " (mState=" << mState << ")" << LL_ENDL;
    switch(type)
    {
    case STATUS_LEFT_CHANNEL:
        if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
        {
            if (mState == STATE_RINGING)
            {
                LLNotificationsUtil::add("P2PCallDeclined", mNotifyArgs);
            }
            else
            {
                mCallEndedByAgent = false;
            }
            deactivate();
        }
        mIgnoreNextSessionLeave = false;
        return;
    case STATUS_JOINING:
        mIgnoreNextSessionLeave = false;
        break;
    default:
        break;
    }
    LLVoiceChannel::handleStatusChange(type);
}
void LLVoiceChannelP2P::handleError(EStatusType type)
{
    switch(type)
    {
    case ERROR_NOT_AVAILABLE:
        LLNotificationsUtil::add("P2PCallNoAnswer", mNotifyArgs);
        break;
    default:
        break;
    }
    LLVoiceChannel::handleError(type);
}
void LLVoiceChannelP2P::activate()
{
    if (callStarted()) return;
    mCallEndedByAgent = true;
    LLVoiceChannel::activate();
    if (callStarted())
    {
        if (mIncomingCallInterface == nullptr)
        {
            mReceivedCall = false;
            mOutgoingCallInterface->callUser(mOtherUserID);
        }
        else
        {
            if (!mIncomingCallInterface->answerInvite())
            {
                mCallEndedByAgent = false;
                mIncomingCallInterface.reset();
                handleError(ERROR_UNKNOWN);
                return;
            }
            mIncomingCallInterface.reset();
        }
        addToTheRecentPeopleList();
        if (!LLVoiceClient::getInstance()->getUserPTTState() && LLVoiceClient::getInstance()->getPTTIsToggle())
        {
            LLVoiceClient::getInstance()->inputUserControlState(true);
        }
    }
}
void LLVoiceChannelP2P::deactivate()
{
    if (callStarted())
    {
        mOutgoingCallInterface->hangup();
    }
    LLVoiceChannel::deactivate();
}
void LLVoiceChannelP2P::requestChannelInfo()
{
    if (sCurrentVoiceChannel == this)
    {
        setState(STATE_CALL_STARTED);
    }
}
void LLVoiceChannelP2P::setChannelInfo(const LLSD& channel_info)
{
    mChannelInfo        = channel_info;
    bool needs_activate = false;
    if (callStarted())
    {
        if (mOtherUserID < gAgent.getID())
        {
            deactivate();
            needs_activate = true;
        }
        else
        {
            mOutgoingCallInterface->callUser(mOtherUserID);
            return;
        }
    }
    mReceivedCall = true;
    if (channel_info.isDefined() && channel_info.isMap())
    {
        mIncomingCallInterface = LLVoiceClient::getInstance()->getIncomingCallInterface(channel_info);
    }
    if (needs_activate)
    {
        activate();
    }
}
void LLVoiceChannelP2P::resetChannelInfo()
{
    mChannelInfo = LLVoiceClient::getInstance()->getP2PChannelInfoTemplate(mOtherUserID);
    mState = STATE_NO_CHANNEL_INFO;
}
void LLVoiceChannelP2P::setSessionHandle(const std::string& handle, const std::string &inURI)
{
    if (!handle.empty())
    {
        mChannelInfo["session_handle"] = handle;
    }
    if (!inURI.empty())
    {
        mChannelInfo["channel_uri"] = inURI;
    }
    if (mState == STATE_NO_CHANNEL_INFO)
    {
        setState(STATE_READY);
    }
}
void LLVoiceChannelP2P::setState(EState state)
{
    LL_INFOS("Voice") << "P2P CALL STATE CHANGE: incoming=" << int(mReceivedCall) << " oldstate=" << mState << " newstate=" << state << LL_ENDL;
    if (mReceivedCall)
    {
        if (mReceivedCall && state == STATE_RINGING)
        {
            doSetState(state);
            return;
        }
    }
    LLVoiceChannel::setState(state);
}
void LLVoiceChannelP2P::addToTheRecentPeopleList()
{
}
