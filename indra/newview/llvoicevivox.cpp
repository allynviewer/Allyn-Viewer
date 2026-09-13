/**
 * @file LLVivoxVoiceClient.cpp
 * @brief Implementation of LLVivoxVoiceClient class which is the interface to the voice client process.
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
#include "llvoicevivox.h"
#include "llsdutil.h"
#include "llavatarnamecache.h"
#include "llvoavatarself.h"
#include "llbufferstream.h"
#include "llcallbacklist.h"
#include "llbase64.h"
#include "llappviewer.h"
#include "llmutelist.h"
#include "llagent.h"
#include "llimpanel.h"
#include "llimview.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "llspeakers.h"
#include "lltrans.h"
#include "llviewercamera.h"
#include "llviewernetwork.h"
#include "llvoicechannel.h"
#include "llnotificationsutil.h"
#include "stringize.h"
#include "apr_base64.h"
#define USE_SESSION_GROUPS 0
const F32 VOLUME_SCALE_VIVOX = 0.01f;
const F32 SPEAKING_TIMEOUT = 1.f;
static const std::string VOICE_SERVER_TYPE = "Vivox";
const F32 CONNECT_THROTTLE_SECONDS = 1.0f;
const F32 UPDATE_THROTTLE_SECONDS = 0.1f;
const F32 LOGIN_RETRY_SECONDS = 10.0f;
const int MAX_LOGIN_RETRIES = 12;
const int MAX_NORMAL_JOINING_SPATIAL_NUM = 50;
const F32 VOICE_FONT_EXPIRY_INTERVAL = 10.f;
static const std::string VOICE_FONT_EXPIRY_TIME = "T05:00:00Z";
const F32 CAPTURE_BUFFER_MAX_TIME = 10.f;
static int scale_mic_volume(float volume)
{
	return 30 + (int)(volume * 20.0f);
}
static int scale_speaker_volume(float volume)
{
	return 30 + (int)(volume * 40.0f);
}
class LLVivoxVoiceAccountProvisionResponder :
	public LLHTTPClient::ResponderWithResult
{
	LOG_CLASS(LLVivoxVoiceAccountProvisionResponder);
public:
	LLVivoxVoiceAccountProvisionResponder(int retries)
	{
		mRetries = retries;
	}
private:
	void httpFailure()
	{
		LL_WARNS("Voice") << "ProvisionVoiceAccountRequest returned an error, "
			<<  ( (mRetries > 0) ? "retrying" : "too many retries (giving up)" )
			<< " " << dumpResponse() << LL_ENDL;
		if ( mRetries > 0 )
		{
			LLVivoxVoiceClient::getInstance()->requestVoiceAccountProvision(mRetries - 1);
		}
		else
		{
			LLVivoxVoiceClient::getInstance()->giveUp();
		}
	}
	void httpSuccess()
	{
		std::string voice_sip_uri_hostname;
		std::string voice_account_server_uri;
		LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response:" << dumpResponse() << LL_ENDL;
		const LLSD& content = getContent();
		if (!content.isMap())
		{
			failureResult(HTTP_INTERNAL_ERROR_OTHER, "Malformed response contents", content);
			return;
		}
		if(content.has("voice_sip_uri_hostname"))
			voice_sip_uri_hostname = content["voice_sip_uri_hostname"].asString();
		if(content.has("voice_account_server_name"))
			voice_account_server_uri = content["voice_account_server_name"].asString();
		LLVivoxVoiceClient::getInstance()->login(
			content["username"].asString(),
			content["password"].asString(),
			voice_sip_uri_hostname,
			voice_account_server_uri);
	}
	char const* getName(void) const { return "LLVivoxVoiceAccountProvisionResponder"; }
private:
	int mRetries;
};
class LLVivoxVoiceClientMuteListObserver : public LLMuteListObserver
{
	void onChange()  { LLVivoxVoiceClient::getInstance()->muteListChanged();}
};
static LLVivoxVoiceClientMuteListObserver mutelist_listener;
static bool sMuteListListener_listening = false;
class LLVivoxVoiceClientCapResponder : public LLHTTPClient::ResponderWithResult
{
	LOG_CLASS(LLVivoxVoiceClientCapResponder);
public:
	LLVivoxVoiceClientCapResponder(LLVivoxVoiceClient::state requesting_state) : mRequestingState(requesting_state) {};
private:
	void httpFailure();
	void httpSuccess();
	char const* getName() const { return "LLVivoxVoiceClientCapResponder"; }
	LLVivoxVoiceClient::state mRequestingState;
};
void LLVivoxVoiceClientCapResponder::httpFailure()
{
	LL_WARNS("Voice") << dumpResponse() << LL_ENDL;
	LLVivoxVoiceClient::getInstance()->sessionTerminate();
}
void LLVivoxVoiceClientCapResponder::httpSuccess()
{
	LLSD::map_const_iterator iter;
	LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest response:" << dumpResponse() << LL_ENDL;
	std::string uri;
	std::string credentials;
	const LLSD& content = getContent();
	if ( content.has("voice_credentials") )
	{
		LLSD voice_credentials = content["voice_credentials"];
		if ( voice_credentials.has("channel_uri") )
		{
			uri = voice_credentials["channel_uri"].asString();
		}
		if ( voice_credentials.has("channel_credentials") )
		{
			credentials =
				voice_credentials["channel_credentials"].asString();
		}
	}
	if(LLVivoxVoiceClient::getInstance()->parcelVoiceInfoReceived(mRequestingState))
	{
		LLVivoxVoiceClient::getInstance()->setSpatialChannel(uri, credentials);
	}
}
#if LL_WINDOWS
static HANDLE sGatewayHandle = 0;
static bool isGatewayRunning()
{
	bool result = false;
	if (sGatewayHandle != 0)
	{
		DWORD waitresult = WaitForSingleObject(sGatewayHandle, 0);
		if (waitresult != WAIT_OBJECT_0)
		{
			result = true;
		}
	}
	return result;
}
static void killGateway()
{
	if (sGatewayHandle != 0)
	{
		TerminateProcess(sGatewayHandle,0);
	}
}
#else
static pid_t sGatewayPID = 0;
static bool isGatewayRunning()
{
	bool result = false;
	if (sGatewayPID != 0)
	{
		if(kill(sGatewayPID, 0) == 0)
		{
			result = true;
		}
	}
	return result;
}
static void killGateway()
{
	if (sGatewayPID != 0)
	{
		kill(sGatewayPID, SIGTERM);
	}
}
#endif
LLVivoxVoiceClient::LLVivoxVoiceClient() :
	mState(stateDisabled),
	mSessionTerminateRequested(false),
	mRelogRequested(false),
	mConnected(false),
	mTerminateDaemon(false),
	mPump(NULL),
	mSpatialJoiningNum(0),
	mTuningMode(false),
	mTuningEnergy(0.0f),
	mTuningMicVolume(0),
	mTuningMicVolumeDirty(true),
	mTuningSpeakerVolume(0),
	mTuningSpeakerVolumeDirty(true),
	mTuningExitState(stateDisabled),
	mAreaVoiceDisabled(false),
	mAudioSession(NULL),
	mAudioSessionChanged(false),
	mNextAudioSession(NULL),
	mCurrentParcelLocalID(0),
	mNumberOfAliases(0),
	mCommandCookie(0),
	mLoginRetryCount(0),
	mBuddyListMapPopulated(false),
	mBlockRulesListReceived(false),
	mAutoAcceptRulesListReceived(false),
	mCaptureDeviceDirty(false),
	mRenderDeviceDirty(false),
	mSpatialCoordsDirty(false),
	mIsInitialized(false),
	mMuteMic(false),
	mMuteMicDirty(false),
	mFriendsListDirty(true),
	mEarLocation(0),
	mSpeakerVolumeDirty(true),
	mSpeakerMuteDirty(true),
	mMicVolume(0),
	mMicVolumeDirty(true),
	mVoiceEnabled(false),
	mWriteInProgress(false),
	mLipSyncEnabled(false),
	mVoiceFontsReceived(false),
	mVoiceFontsNew(false),
	mVoiceFontListDirty(false),
	mCaptureBufferMode(false),
	mCaptureBufferRecording(false),
	mCaptureBufferRecorded(false),
	mCaptureBufferPlaying(false),
	mShutdownComplete(true),
	mPlayRequestCount(0),
	mAvatarNameCacheConnection()
{
	mSpeakerVolume = scale_speaker_volume(0);
	mVoiceVersion.serverVersion = "";
	mVoiceVersion.serverType = VOICE_SERVER_TYPE;
	setState(stateDisabled);
	gIdleCallbacks.addFunction(idle, this);
}
LLVivoxVoiceClient::~LLVivoxVoiceClient()
{
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
}
void LLVivoxVoiceClient::init(LLPumpIO *pump)
{
	LLVivoxVoiceClient::getInstance()->mPump = pump;
}
void LLVivoxVoiceClient::terminate()
{
	if(mConnected)
	{
		logout();
		connectorShutdown();
#ifdef LL_WINDOWS
		S32 count = 0;
		while (!mShutdownComplete && 10 > ++count)
		{
			stateMachine();
			_sleep(1000);
		}
#endif
		closeSocket();
		cleanUp();
	}
	else
	{
		killGateway();
	}
}
void LLVivoxVoiceClient::cleanUp()
{
	deleteAllSessions();
	deleteAllVoiceFonts();
	deleteVoiceFontTemplates();
}
const LLVoiceVersionInfo& LLVivoxVoiceClient::getVersion()
{
	return mVoiceVersion;
}
void LLVivoxVoiceClient::updateSettings()
{
	setVoiceEnabled(gSavedSettings.getBOOL("EnableVoiceChat"));
	setEarLocation(gSavedSettings.getS32("VoiceEarLocation"));
	std::string inputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	setCaptureDevice(inputDevice);
	std::string outputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	setRenderDevice(outputDevice);
	F32 mic_level = gSavedSettings.getF32("AudioLevelMic");
	setMicGain(mic_level);
	setLipSyncEnabled(gSavedSettings.getBOOL("LipSyncEnabled"));
}
bool LLVivoxVoiceClient::writeString(const std::string &str)
{
	bool result = false;
	if(mConnected)
	{
		apr_status_t err;
		apr_size_t size = (apr_size_t)str.size();
		apr_size_t written = size;
		err = apr_socket_send(
				mSocket->getSocket(),
				(const char*)str.data(),
				&written);
		if(err == 0)
		{
			result = true;
		}
		else
		{
			char buf[MAX_STRING];
			LL_WARNS("Voice") << "apr error " << err << " ("<< apr_strerror(err, buf, MAX_STRING) << ") sending data to vivox daemon." << LL_ENDL;
			daemonDied();
		}
	}
	return result;
}
void LLVivoxVoiceClient::connectorCreate()
{
	std::ostringstream stream;
	std::string loglevel = "0";
	setState(stateConnectorStarting);
	std::string logpath = gSavedSettings.getString("VivoxLogDirectory");
	if (logpath.empty())
	{
		logpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
	}
	if (LLStringUtil::endsWith(logpath, gDirUtilp->getDirDelimiter()))
	{
		logpath.resize(logpath.size()-1);
	}
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.Create.1\">"
		<< "<ClientName>V2 SDK</ClientName>"
		<< "<AccountManagementServer>" << mVoiceAccountServerURI << "</AccountManagementServer>"
		<< "<Mode>Normal</Mode>";
	if (gSavedSettings.getBOOL("VoiceMultiInstance"))
	{
		stream
			<< "<MinimumPort>30000</MinimumPort>"
			<< "<MaximumPort>50000</MaximumPort>";
	}
	stream
		<< "<Logging>"
		<< "<Folder>" << logpath << "</Folder>"
		<< "<FileNamePrefix>Connector</FileNamePrefix>"
		<< "<FileNameSuffix>.log</FileNameSuffix>"
		<< "<LogLevel>" << loglevel << "</LogLevel>"
		<< "</Logging>"
		<< "<Application></Application>"
		<< "<MaxCalls>12</MaxCalls>"
		<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::connectorShutdown()
{
	setState(stateConnectorStopping);
	if(!mConnectorHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.InitiateShutdown.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
		<< "</Request>"
		<< "\n\n\n";
		mShutdownComplete = false;
		mConnectorHandle.clear();
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::userAuthorized(const std::string& user_id, const LLUUID &agentID)
{
	mAccountDisplayName = user_id;
	LL_INFOS("Voice") << "name \"" << mAccountDisplayName << "\" , ID " << agentID << LL_ENDL;
	mAccountName = nameFromID(agentID);
}
void LLVivoxVoiceClient::requestVoiceAccountProvision(S32 retries)
{
	LLViewerRegion *region = gAgent.getRegion();
	if ( region &&
		 region->capabilitiesReceived() &&
		 (mVoiceEnabled || !mIsInitialized))
	{
		std::string url =
		region->getCapability("ProvisionVoiceAccountRequest");
		if ( !url.empty() )
		{
				LLHTTPClient::post(
							   url,
							   LLSD(),
							   new LLVivoxVoiceAccountProvisionResponder(retries));
			setState(stateConnectorStart);
		}
	}
}
void LLVivoxVoiceClient::login(
	const std::string& account_name,
	const std::string& password,
	const std::string& voice_sip_uri_hostname,
	const std::string& voice_account_server_uri)
{
	mVoiceSIPURIHostName = voice_sip_uri_hostname;
	mVoiceAccountServerURI = voice_account_server_uri;
	if(!mAccountHandle.empty())
	{
		LL_WARNS("Voice") << "Called while already logged in." << LL_ENDL;
		return;
	}
	else if ( account_name != mAccountName )
	{
		LL_WARNS("Voice") << "Wrong account name! " << account_name
				<< " instead of " << mAccountName << LL_ENDL;
	}
	else
	{
		mAccountPassword = password;
	}
	std::string debugSIPURIHostName = gSavedSettings.getString("VivoxDebugSIPURIHostName");
	if( !debugSIPURIHostName.empty() )
	{
		mVoiceSIPURIHostName = debugSIPURIHostName;
	}
	if( mVoiceSIPURIHostName.empty() )
	{
		if(LLViewerLogin::getInstance()->isInProductionGrid())
		{
			mVoiceSIPURIHostName = "bhr.vivox.com";
		}
		else
		{
			mVoiceSIPURIHostName = "bhd.vivox.com";
		}
	}
	std::string debugAccountServerURI = gSavedSettings.getString("VivoxDebugVoiceAccountServerURI");
	if( !debugAccountServerURI.empty() )
	{
		mVoiceAccountServerURI = debugAccountServerURI;
	}
	if( mVoiceAccountServerURI.empty() )
	{
		mVoiceAccountServerURI = "https://www." + mVoiceSIPURIHostName + "/api2/";
	}
}
void LLVivoxVoiceClient::idle(void* user_data)
{
	LLVivoxVoiceClient* self = (LLVivoxVoiceClient*)user_data;
	self->stateMachine();
}
std::string LLVivoxVoiceClient::state2string(LLVivoxVoiceClient::state inState)
{
	std::string result = "UNKNOWN";
#define CASE(x)  case x:  result = #x;  break
	switch(inState)
	{
		CASE(stateDisableCleanup);
		CASE(stateDisabled);
		CASE(stateStart);
		CASE(stateDaemonLaunched);
		CASE(stateConnecting);
		CASE(stateConnected);
		CASE(stateIdle);
		CASE(stateMicTuningStart);
		CASE(stateMicTuningRunning);
		CASE(stateMicTuningStop);
		CASE(stateCaptureBufferPaused);
		CASE(stateCaptureBufferRecStart);
		CASE(stateCaptureBufferRecording);
		CASE(stateCaptureBufferPlayStart);
		CASE(stateCaptureBufferPlaying);
		CASE(stateConnectorStart);
		CASE(stateConnectorStarting);
		CASE(stateConnectorStarted);
		CASE(stateLoginRetry);
		CASE(stateLoginRetryWait);
		CASE(stateNeedsLogin);
		CASE(stateLoggingIn);
		CASE(stateLoggedIn);
		CASE(stateVoiceFontsWait);
		CASE(stateVoiceFontsReceived);
		CASE(stateCreatingSessionGroup);
		CASE(stateNoChannel);
		CASE(stateRetrievingParcelVoiceInfo);
		CASE(stateJoiningSession);
		CASE(stateSessionJoined);
		CASE(stateRunning);
		CASE(stateLeavingSession);
		CASE(stateSessionTerminated);
		CASE(stateLoggingOut);
		CASE(stateLoggedOut);
		CASE(stateConnectorStopping);
		CASE(stateConnectorStopped);
		CASE(stateConnectorFailed);
		CASE(stateConnectorFailedWaiting);
		CASE(stateLoginFailed);
		CASE(stateLoginFailedWaiting);
		CASE(stateJoinSessionFailed);
		CASE(stateJoinSessionFailedWaiting);
		CASE(stateJail);
	}
#undef CASE
	return result;
}
void LLVivoxVoiceClient::setState(state inState)
{
	LL_DEBUGS("Voice") << "entering state " << state2string(inState) << LL_ENDL;
	mState = inState;
}
void LLVivoxVoiceClient::stateMachine()
{
	if(gDisconnected)
	{
		setVoiceEnabled(false);
	}
	if(mVoiceEnabled || (!mIsInitialized && !mTerminateDaemon) )
	{
		updatePosition();
	}
	else if(mTuningMode)
	{
	}
	else
	{
		if((getState() != stateDisabled) && (getState() != stateDisableCleanup))
		{
			if(!mConnected || mTerminateDaemon)
			{
				LL_INFOS("Voice") << "Disabling voice before connection to daemon, terminating." << LL_ENDL;
				killGateway();
				mTerminateDaemon = false;
			}
			logout();
			connectorShutdown();
			setState(stateDisableCleanup);
		}
	}
	switch(getState())
	{
		case stateDisableCleanup:
			closeSocket();
			cleanUp();
			mAccountHandle.clear();
			mAccountPassword.clear();
			mVoiceAccountServerURI.clear();
			setState(stateDisabled);
		break;
		case stateDisabled:
			if(mTuningMode || ((mVoiceEnabled || !mIsInitialized) && !mAccountName.empty()))
			{
				setState(stateStart);
			}
		break;
		case stateStart:
			if(gSavedSettings.getBOOL("CmdLineDisableVoice"))
			{
				setState(stateJail);
			}
			else if(!isGatewayRunning() && gSavedSettings.getBOOL("EnableVoiceChat"))
			{
				if (true)
				{
					std::string exe_path = gDirUtilp->getExecutableDir();
					exe_path += gDirUtilp->getDirDelimiter();
					exe_path += "SLVoice.exe";
					llstat s;
					if (!LLFile::stat(exe_path, &s))
					{
						std::string args, cmd;
						std::string loglevel = "0";
						std::string shutdown_timeout = gSavedSettings.getString("VivoxShutdownTimeout");
						args += " -ll ";
						args += loglevel;
						std::string log_folder = gSavedSettings.getString("VivoxLogDirectory");
						if (log_folder.empty())
						{
							log_folder = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
						}
						if (LLStringUtil::endsWith(log_folder, gDirUtilp->getDirDelimiter()))
						{
							log_folder.resize(log_folder.size()-1);
						}
						args += " -lf ";
						args += '"' + log_folder + '"';
						if(!shutdown_timeout.empty())
						{
							args += " -st ";
							args += shutdown_timeout;
						}
						if (gSavedSettings.getBOOL("VoiceMultiInstance"))
						{
							LLControlVariable* voice_port = gSavedSettings.getControl("VoicePort");
							if (voice_port)
							{
								const BOOL DO_NOT_PERSIST = FALSE;
								S32 port_nr = 30000 + ll_rand(20000);
								voice_port->setValue(LLSD(port_nr), DO_NOT_PERSIST);
							}
							args += llformat(" -i 127.0.0.1:%u",  gSavedSettings.getU32("VoicePort"));
						}
						LL_DEBUGS("Voice") << "Args for SLVoice: " << args << LL_ENDL;
#if LL_WINDOWS
						PROCESS_INFORMATION pinfo;
						STARTUPINFOA sinfo;
						memset(&sinfo, 0, sizeof(sinfo));
						std::string exe_dir = gDirUtilp->getAppRODataDir();
						cmd = "SLVoice.exe";
						cmd += args;
						char *args2 = new char[args.size() + 1];
						strcpy(args2, args.c_str());
						if(!CreateProcessA(exe_path.c_str(), args2, NULL, NULL, FALSE, 0, NULL, exe_dir.c_str(), &sinfo, &pinfo))
						{
						}
						else
						{
							sGatewayHandle = pinfo.hProcess;
							CloseHandle(pinfo.hThread);
						}
						delete[] args2;
#else
						{
							std::vector<std::string> arglist;
							arglist.push_back(exe_path);
							typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
							boost::char_separator<char> sep(" ");
							tokenizer tokens(args, sep);
							tokenizer::iterator token_iter;
							for(token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
							{
								arglist.push_back(*token_iter);
							}
							char **fakeargv = new char*[arglist.size() + 1];
							int i;
							for(i=0; i < arglist.size(); i++)
								fakeargv[i] = const_cast<char*>(arglist[i].c_str());
							fakeargv[i] = NULL;
							fflush(NULL);
							pid_t id = vfork();
							if(id == 0)
							{
								execv(exe_path.c_str(), fakeargv);
								_exit(0);
							}
							delete[] fakeargv;
							sGatewayPID = id;
						}
#endif
						mDaemonHost = LLHost(gSavedSettings.getString("VoiceHost").c_str(), gSavedSettings.getU32("VoicePort"));
					}
					else
					{
						LL_INFOS("Voice") << exe_path << " not found." << LL_ENDL;
					}
				}
				else
				{
					mDaemonHost = LLHost(gSavedSettings.getString("VoiceHost"), gSavedSettings.getU32("VoicePort"));
				}
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(CONNECT_THROTTLE_SECONDS);
				setState(stateDaemonLaunched);
				mMuteMicDirty = true;
				mMicVolumeDirty = true;
				mSpeakerVolumeDirty = true;
				mSpeakerMuteDirty = true;
				mCaptureDeviceDirty = !mCaptureDevice.empty();
				mRenderDeviceDirty = !mRenderDevice.empty();
				mMainSessionGroupHandle.clear();
			}
		break;
		case stateDaemonLaunched:
			if(mUpdateTimer.hasExpired())
			{
				LL_DEBUGS("Voice") << "Connecting to vivox daemon:" << mDaemonHost << LL_ENDL;
				mUpdateTimer.setTimerExpirySec(CONNECT_THROTTLE_SECONDS);
				if(!mSocket)
				{
					mSocket = LLSocket::create(LLSocket::STREAM_TCP);
				}
				mConnected = mSocket->blockingConnect(mDaemonHost);
				if(mConnected)
				{
					setState(stateConnecting);
				}
				else
				{
					closeSocket();
				}
			}
		break;
		case stateConnecting:
		if(mPump)
		{
			LLPumpIO::chain_t readChain;
			readChain.push_back(LLIOPipe::ptr_t(new LLIOSocketReader(mSocket)));
			readChain.push_back(LLIOPipe::ptr_t(new LLVivoxProtocolParser()));
			mPump->addChain(readChain, NEVER_CHAIN_EXPIRY_SECS);
			setState(stateConnected);
		}
		break;
		case stateConnected:
			getCaptureDevicesSendMessage();
			getRenderDevicesSendMessage();
			mLoginRetryCount = 0;
			setState(stateIdle);
		break;
		case stateIdle:
			if(mTuningMode)
			{
				mTuningExitState = stateIdle;
				setState(stateMicTuningStart);
			}
			else if(!mVoiceEnabled && mIsInitialized)
			{
				setState(stateConnectorStopped);
			}
			else if(!mAccountName.empty())
			{
				if ( mAccountPassword.empty() )
				{
					requestVoiceAccountProvision();
				}
			}
		break;
		case stateMicTuningStart:
			if(mUpdateTimer.hasExpired())
			{
				if(mCaptureDeviceDirty || mRenderDeviceDirty)
				{
					std::ostringstream stream;
					buildSetCaptureDevice(stream);
					buildSetRenderDevice(stream);
					if(!stream.str().empty())
					{
						writeString(stream.str());
					}
					mUpdateTimer.start();
					mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
				}
				else
				{
					tuningCaptureStartSendMessage(10000);
					setState(stateMicTuningRunning);
				}
			}
		break;
		case stateMicTuningRunning:
			if(!mTuningMode || mCaptureDeviceDirty || mRenderDeviceDirty)
			{
				setState(stateMicTuningStop);
			}
			else
			{
				if(mTuningMicVolumeDirty || mTuningSpeakerVolumeDirty)
				{
					std::ostringstream stream;
					if(mTuningMicVolumeDirty)
					{
						LL_INFOS("Voice") << "setting tuning mic level to " << mTuningMicVolume << LL_ENDL;
						stream
						<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetMicLevel.1\">"
						<< "<Level>" << mTuningMicVolume << "</Level>"
						<< "</Request>\n\n\n";
					}
					if(mTuningSpeakerVolumeDirty)
					{
						stream
						<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetSpeakerLevel.1\">"
						<< "<Level>" << mTuningSpeakerVolume << "</Level>"
						<< "</Request>\n\n\n";
					}
					mTuningMicVolumeDirty = false;
					mTuningSpeakerVolumeDirty = false;
					if(!stream.str().empty())
					{
						writeString(stream.str());
					}
				}
			}
		break;
		case stateMicTuningStop:
		{
			tuningCaptureStopSendMessage();
			setState(mTuningExitState);
			mUpdateTimer.start();
			mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
		}
		break;
		case stateCaptureBufferPaused:
			if (!mCaptureBufferMode)
			{
				mCaptureBufferRecording = false;
				mCaptureBufferRecorded = false;
				mCaptureBufferPlaying = false;
				setState(stateNoChannel);
			}
			else if (mCaptureBufferRecording)
			{
				setState(stateCaptureBufferRecStart);
			}
			else if (mCaptureBufferPlaying)
			{
				setState(stateCaptureBufferPlayStart);
			}
		break;
		case stateCaptureBufferRecStart:
			captureBufferRecordStartSendMessage();
			mCaptureBufferRecorded = true;
			mCaptureTimer.start();
			mCaptureTimer.setTimerExpirySec(CAPTURE_BUFFER_MAX_TIME);
			notifyVoiceFontObservers();
			setState(stateCaptureBufferRecording);
		break;
		case stateCaptureBufferRecording:
			if (!mCaptureBufferMode || !mCaptureBufferRecording ||
				mCaptureBufferPlaying || mCaptureTimer.hasExpired())
			{
				captureBufferRecordStopSendMessage();
				mCaptureBufferRecording = false;
				notifyVoiceFontObservers();
				setState(stateCaptureBufferPaused);
			}
		break;
		case stateCaptureBufferPlayStart:
			captureBufferPlayStartSendMessage(mPreviewVoiceFont);
			mPreviewVoiceFontLast = mPreviewVoiceFont;
			notifyVoiceFontObservers();
			setState(stateCaptureBufferPlaying);
		break;
		case stateCaptureBufferPlaying:
			if (mCaptureBufferPlaying && mPreviewVoiceFont != mPreviewVoiceFontLast)
			{
				setState(stateCaptureBufferPlayStart);
			}
			else if (!mCaptureBufferMode || !mCaptureBufferPlaying || mCaptureBufferRecording)
			{
				captureBufferPlayStopSendMessage();
				mCaptureBufferPlaying = false;
				notifyVoiceFontObservers();
				setState(stateCaptureBufferPaused);
		}
		break;
		case stateConnectorStart:
			if(!mVoiceEnabled && mIsInitialized)
			{
				setState(stateLoggedOut);
			}
			else if(!mVoiceAccountServerURI.empty())
			{
				connectorCreate();
			}
		break;
		case stateConnectorStarting:
		break;
		case stateConnectorStarted:
			if(!mVoiceEnabled && mIsInitialized)
			{
				setState(stateLoggedOut);
			}
			else
			{
				setState(stateNeedsLogin);
			}
		break;
		case stateLoginRetry:
			if(mLoginRetryCount == 0)
			{
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGIN_RETRY);
			}
			mLoginRetryCount++;
			if(mLoginRetryCount > MAX_LOGIN_RETRIES)
			{
				LL_WARNS("Voice") << "too many login retries, giving up." << LL_ENDL;
				setState(stateLoginFailed);
				LLSD args;
				std::stringstream errs;
				errs << mVoiceAccountServerURI << "\n:UDP: 3478, 3479, 5060, 5062, 12000-17000";
				args["HOSTID"] = errs.str();
				mTerminateDaemon = true;
				LLNotificationsUtil::add("NoVoiceConnect", args);
			}
			else
			{
				LL_INFOS("Voice") << "will retry login in " << LOGIN_RETRY_SECONDS << " seconds." << LL_ENDL;
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(LOGIN_RETRY_SECONDS);
				setState(stateLoginRetryWait);
			}
		break;
		case stateLoginRetryWait:
			if(mUpdateTimer.hasExpired())
			{
				setState(stateNeedsLogin);
			}
		break;
		case stateNeedsLogin:
			if(!mAccountPassword.empty())
			{
				setState(stateLoggingIn);
				loginSendMessage();
			}
		break;
		case stateLoggingIn:
		break;
		case stateLoggedIn:
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);
			if (LLVoiceClient::instance().getVoiceEffectEnabled())
			{
				setState(stateVoiceFontsWait);
				refreshVoiceEffectLists(true);
			}
			else
			{
				setState(stateVoiceFontsReceived);
			}
			if((!sMuteListListener_listening))
			{
				LLMuteList::getInstance()->addObserver(&mutelist_listener);
				sMuteListListener_listening = true;
			}
			{
				std::ostringstream stream;
				buildLocalAudioUpdates(stream);
				if(!stream.str().empty())
				{
					writeString(stream.str());
				}
			}
		break;
		case stateVoiceFontsWait:
		break;
		case stateVoiceFontsReceived:
			mVoiceFontExpiryTimer.start();
			mVoiceFontExpiryTimer.setTimerExpirySec(VOICE_FONT_EXPIRY_INTERVAL);
#if USE_SESSION_GROUPS
			setState(stateCreatingSessionGroup);
			sessionGroupCreateSendMessage();
#else
			setState(stateNoChannel);
#endif
		break;
		case stateCreatingSessionGroup:
			if(mSessionTerminateRequested || (!mVoiceEnabled && mIsInitialized))
			{
				setState(stateSessionTerminated);
			}
			else if(!mMainSessionGroupHandle.empty())
			{
				recordingLoopStart();
				setState(stateNoChannel);
			}
		break;
		case stateRetrievingParcelVoiceInfo:
			if(mSessionTerminateRequested || (!mVoiceEnabled && mIsInitialized))
			{
				setState(stateSessionTerminated);
			}
			break;
		case stateNoChannel:
			LL_DEBUGS("Voice") << "State No Channel" << LL_ENDL;
			mSpatialJoiningNum = 0;
			if(mSessionTerminateRequested || (!mVoiceEnabled && mIsInitialized))
			{
				setState(stateSessionTerminated);
			}
			else if(mTuningMode)
			{
				mTuningExitState = stateNoChannel;
				setState(stateMicTuningStart);
			}
			else if(mCaptureBufferMode)
			{
				setState(stateCaptureBufferPaused);
			}
			else if(checkParcelChanged() || (!mAreaVoiceDisabled && mNextAudioSession == NULL))
			{
				if(requestParcelVoiceInfo())
				{
					setState(stateRetrievingParcelVoiceInfo);
				}
			}
			else if(sessionNeedsRelog(mNextAudioSession))
			{
				requestRelog();
				setState(stateSessionTerminated);
			}
			else if(mNextAudioSession)
			{
				sessionState *oldSession = mAudioSession;
				mAudioSession = mNextAudioSession;
				mAudioSessionChanged = true;
				if(!mAudioSession->mReconnect)
				{
					mNextAudioSession = NULL;
				}
				reapSession(oldSession);
				if(!mAudioSession->mHandle.empty())
				{
					sessionMediaConnectSendMessage(mAudioSession);
				}
				else
				{
					sessionCreateSendMessage(mAudioSession, true, false);
				}
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINING);
				setState(stateJoiningSession);
			}
		break;
		case stateJoiningSession:
			if(mSpatialJoiningNum == MAX_NORMAL_JOINING_SPATIAL_NUM)
			{
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
				LL_WARNS() << "There seems to be problem with connection to voice server. Disabling voice chat abilities." << LL_ENDL;
			}
			if(mAudioSession && mAudioSession->mIsSpatial)
			{
				mSpatialJoiningNum++;
			}
			if(!mVoiceEnabled && mIsInitialized)
			{
				setState(stateSessionTerminated);
			}
			else if(mSessionTerminateRequested)
			{
				if(mAudioSession && !mAudioSession->mHandle.empty())
				{
					if(mAudioSession->mIsP2P)
					{
						sessionMediaDisconnectSendMessage(mAudioSession);
						setState(stateSessionTerminated);
					}
				}
			}
		break;
		case stateSessionJoined:
			mSpatialJoiningNum = 0;
			if(mAudioSession && mAudioSession->mVoiceEnabled)
			{
				mMuteMicDirty = true;
				mSpeakerVolumeDirty = true;
				mSpatialCoordsDirty = true;
				setState(stateRunning);
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
			}
			else if(!mVoiceEnabled && mIsInitialized)
			{
				setState(stateSessionTerminated);
			}
			else if(mSessionTerminateRequested)
			{
				if(mAudioSession && mAudioSession->mIsP2P)
				{
					sessionMediaDisconnectSendMessage(mAudioSession);
					setState(stateSessionTerminated);
				}
			}
		break;
		case stateRunning:
			if((!mVoiceEnabled && mIsInitialized) || mSessionTerminateRequested)
			{
				leaveAudioSession();
			}
			else
			{
				if(!inSpatialChannel())
				{
					mSpatialCoordsDirty = false;
				}
				else
				{
					if(checkParcelChanged())
					{
						if(requestParcelVoiceInfo())
						{
							setState(stateRetrievingParcelVoiceInfo);
						}
					}
					enforceTether();
				}
				if (mVoiceFontExpiryTimer.hasExpired())
				{
					expireVoiceFonts();
					mVoiceFontExpiryTimer.setTimerExpirySec(VOICE_FONT_EXPIRY_INTERVAL);
				}
				if((mAudioSession && mAudioSession->mMuteDirty) || mMuteMicDirty || mUpdateTimer.hasExpired())
				{
					mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
					sendPositionalUpdate();
				}
				mIsInitialized = true;
			}
		break;
		case stateLeavingSession:
		break;
		case stateSessionTerminated:
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
			if(mAudioSession)
			{
				sessionState *oldSession = mAudioSession;
				mAudioSession = NULL;
				mAudioSessionChanged = false;
				reapSession(oldSession);
			}
			else
			{
				LL_WARNS("Voice") << "stateSessionTerminated with NULL mAudioSession" << LL_ENDL;
			}
			mSessionTerminateRequested = false;
			if((mVoiceEnabled || !mIsInitialized) && !mRelogRequested  && !LLApp::isExiting())
			{
				setState(stateNoChannel);
			}
			else
			{
				logout();
				mRelogRequested = false;
			}
		break;
		case stateLoggingOut:
		break;
		case stateLoggedOut:
			mAccountHandle.clear();
			cleanUp();
			if((mVoiceEnabled || !mIsInitialized) && !mRelogRequested)
			{
				setState(stateNeedsLogin);
			}
			else
			{
				connectorShutdown();
			}
		break;
		case stateConnectorStopping:
			mShutdownComplete = true;
		break;
		case stateConnectorStopped:
			setState(stateDisableCleanup);
		break;
		case stateConnectorFailed:
			setState(stateConnectorFailedWaiting);
		break;
		case stateConnectorFailedWaiting:
			if(!mVoiceEnabled)
			{
				setState(stateDisableCleanup);
			}
		break;
		case stateLoginFailed:
			setState(stateLoginFailedWaiting);
		break;
		case stateLoginFailedWaiting:
			if(!mVoiceEnabled)
			{
				setState(stateDisableCleanup);
			}
		break;
		case stateJoinSessionFailed:
			if(mAudioSession)
			{
				LL_WARNS("Voice") << "stateJoinSessionFailed: (" << mAudioSession->mErrorStatusCode << "): " << mAudioSession->mErrorStatusString << LL_ENDL;
			}
			else
			{
				LL_WARNS("Voice") << "stateJoinSessionFailed with no current session" << LL_ENDL;
			}
			notifyStatusObservers(LLVoiceClientStatusObserver::ERROR_UNKNOWN);
			setState(stateJoinSessionFailedWaiting);
		break;
		case stateJoinSessionFailedWaiting:
			if(mSessionTerminateRequested)
			{
				setState(stateSessionTerminated);
			}
		break;
		case stateJail:
		break;
	}
	if (mAudioSessionChanged)
	{
		mAudioSessionChanged = false;
		notifyParticipantObservers();
		notifyVoiceFontObservers();
	}
	else if (mAudioSession && mAudioSession->mParticipantsChanged)
	{
		mAudioSession->mParticipantsChanged = false;
		notifyParticipantObservers();
	}
}
void LLVivoxVoiceClient::closeSocket(void)
{
	mSocket.reset();
	mConnected = false;
	mConnectorHandle.clear();
	mAccountHandle.clear();
}
void LLVivoxVoiceClient::loginSendMessage()
{
	std::ostringstream stream;
	bool autoPostCrashDumps = gSavedSettings.getBOOL("VivoxAutoPostCrashDumps");
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Login.1\">"
		<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
		<< "<AccountName>" << mAccountName << "</AccountName>"
		<< "<AccountPassword>" << mAccountPassword << "</AccountPassword>"
		<< "<AudioSessionAnswerMode>VerifyAnswer</AudioSessionAnswerMode>"
		<< "<EnableBuddiesAndPresence>false</EnableBuddiesAndPresence>"
		<< "<BuddyManagementMode>Application</BuddyManagementMode>"
		<< "<ParticipantPropertyFrequency>5</ParticipantPropertyFrequency>"
		<< (autoPostCrashDumps?"<AutopostCrashDumps>true</AutopostCrashDumps>":"")
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::logout()
{
	mAccountPassword.clear();
	mVoiceAccountServerURI.clear();
	setState(stateLoggingOut);
	logoutSendMessage();
}
void LLVivoxVoiceClient::logoutSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Logout.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";
		mAccountHandle.clear();
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::sessionGroupCreateSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		LL_DEBUGS("Voice") << "creating session group" << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Create.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
			<< "<Type>Normal</Type>"
		<< "</Request>"
		<< "\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::sessionCreateSendMessage(sessionState *session, bool startAudio, bool startText)
{
	LL_DEBUGS("Voice") << "Requesting create: " << session->mSIPURI << LL_ENDL;
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "With voice font: " << session->mVoiceFontID << " (" << font_index << ")" << LL_ENDL;
	session->mCreateInProgress = true;
	if(startAudio)
	{
		session->mMediaConnectInProgress = true;
	}
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mSIPURI << "\" action=\"Session.Create.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "<URI>" << session->mSIPURI << "</URI>";
	static const std::string allowed_chars =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
				"0123456789"
				"-._~";
	if(!session->mHash.empty())
	{
		stream
			<< "<Password>" << LLURI::escape(session->mHash, allowed_chars) << "</Password>"
			<< "<PasswordHashAlgorithm>SHA1UserName</PasswordHashAlgorithm>";
	}
	stream
		<< "<ConnectAudio>" << (startAudio?"true":"false") << "</ConnectAudio>"
		<< "<ConnectText>" << (startText?"true":"false") << "</ConnectText>"
		<< "<VoiceFontID>" << font_index << "</VoiceFontID>"
		<< "<Name>" << mChannelName << "</Name>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::sessionGroupAddSessionSendMessage(sessionState *session, bool startAudio, bool startText)
{
	LL_DEBUGS("Voice") << "Requesting create: " << session->mSIPURI << LL_ENDL;
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "With voice font: " << session->mVoiceFontID << " (" << font_index << ")" << LL_ENDL;
	session->mCreateInProgress = true;
	if(startAudio)
	{
		session->mMediaConnectInProgress = true;
	}
	std::string password;
	if(!session->mHash.empty())
	{
		static const std::string allowed_chars =
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
					"0123456789"
					"-._~"
					;
		password = LLURI::escape(session->mHash, allowed_chars);
	}
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mSIPURI << "\" action=\"SessionGroup.AddSession.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<URI>" << session->mSIPURI << "</URI>"
		<< "<Name>" << mChannelName << "</Name>"
		<< "<ConnectAudio>" << (startAudio?"true":"false") << "</ConnectAudio>"
		<< "<ConnectText>" << (startText?"true":"false") << "</ConnectText>"
		<< "<VoiceFontID>" << font_index << "</VoiceFontID>"
		<< "<Password>" << password << "</Password>"
		<< "<PasswordHashAlgorithm>SHA1UserName</PasswordHashAlgorithm>"
	<< "</Request>\n\n\n"
	;
	writeString(stream.str());
}
void LLVivoxVoiceClient::sessionMediaConnectSendMessage(sessionState *session)
{
	LL_DEBUGS("Voice") << "Connecting audio to session handle: " << session->mHandle << LL_ENDL;
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "With voice font: " << session->mVoiceFontID << " (" << font_index << ")" << LL_ENDL;
	session->mMediaConnectInProgress = true;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mHandle << "\" action=\"Session.MediaConnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
		<< "<VoiceFontID>" << font_index << "</VoiceFontID>"
		<< "<Media>Audio</Media>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::sessionTextConnectSendMessage(sessionState *session)
{
	LL_DEBUGS("Voice") << "connecting text to session handle: " << session->mHandle << LL_ENDL;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mHandle << "\" action=\"Session.TextConnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::sessionTerminate()
{
	mSessionTerminateRequested = true;
}
void LLVivoxVoiceClient::requestRelog()
{
	mSessionTerminateRequested = true;
	mRelogRequested = true;
}
void LLVivoxVoiceClient::leaveAudioSession()
{
	if(mAudioSession)
	{
		LL_DEBUGS("Voice") << "leaving session: " << mAudioSession->mSIPURI << LL_ENDL;
		switch(getState())
		{
			case stateNoChannel:
				setState(stateJoinSessionFailedWaiting);
			break;
			case stateJoiningSession:
			case stateSessionJoined:
			case stateRunning:
				if(!mAudioSession->mHandle.empty())
				{
#if RECORD_EVERYTHING
					std::string savepath("/tmp/vivoxrecording");
					{
						time_t now = time(NULL);
						const size_t BUF_SIZE = 64;
						char time_str[BUF_SIZE];
						strftime(time_str, BUF_SIZE, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
						savepath += time_str;
					}
					recordingLoopSave(savepath);
#endif
					sessionMediaDisconnectSendMessage(mAudioSession);
					setState(stateLeavingSession);
				}
				else
				{
					LL_WARNS("Voice") << "called with no session handle" << LL_ENDL;
					setState(stateSessionTerminated);
				}
			break;
			case stateJoinSessionFailed:
			case stateJoinSessionFailedWaiting:
				setState(stateSessionTerminated);
			break;
			default:
				LL_WARNS("Voice") << "called from unknown state" << LL_ENDL;
			break;
		}
	}
	else
	{
		LL_WARNS("Voice") << "called with no active session" << LL_ENDL;
		setState(stateSessionTerminated);
	}
}
void LLVivoxVoiceClient::sessionTerminateSendMessage(sessionState *session)
{
	std::ostringstream stream;
	LL_DEBUGS("Voice") << "Sending Session.Terminate with handle " << session->mHandle << LL_ENDL;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Terminate.1\">"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::sessionGroupTerminateSendMessage(sessionState *session)
{
	std::ostringstream stream;
	LL_DEBUGS("Voice") << "Sending SessionGroup.Terminate with handle " << session->mGroupHandle << LL_ENDL;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Terminate.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::sessionMediaDisconnectSendMessage(sessionState *session)
{
	std::ostringstream stream;
	LL_DEBUGS("Voice") << "Sending Session.MediaDisconnect with handle " << session->mHandle << LL_ENDL;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.MediaDisconnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
		<< "<Media>Audio</Media>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::sessionTextDisconnectSendMessage(sessionState *session)
{
	std::ostringstream stream;
	LL_DEBUGS("Voice") << "Sending Session.TextDisconnect with handle " << session->mHandle << LL_ENDL;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.TextDisconnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::getCaptureDevicesSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.GetCaptureDevices.1\">"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::getRenderDevicesSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.GetRenderDevices.1\">"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::clearCaptureDevices()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mCaptureDevices.clear();
}
void LLVivoxVoiceClient::addCaptureDevice(const std::string& name)
{
	LL_DEBUGS("Voice") << name << LL_ENDL;
	mCaptureDevices.push_back(name);
}
LLVoiceDeviceList& LLVivoxVoiceClient::getCaptureDevices()
{
	return mCaptureDevices;
}
void LLVivoxVoiceClient::setCaptureDevice(const std::string& name)
{
	if(name == "Default")
	{
		if(!mCaptureDevice.empty())
		{
			mCaptureDevice.clear();
			mCaptureDeviceDirty = true;
		}
	}
	else
	{
		if(mCaptureDevice != name)
		{
			mCaptureDevice = name;
			mCaptureDeviceDirty = true;
		}
	}
}
void LLVivoxVoiceClient::clearRenderDevices()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mRenderDevices.clear();
}
void LLVivoxVoiceClient::addRenderDevice(const std::string& name)
{
	LL_DEBUGS("Voice") << name << LL_ENDL;
	mRenderDevices.push_back(name);
}
LLVoiceDeviceList& LLVivoxVoiceClient::getRenderDevices()
{
	return mRenderDevices;
}
void LLVivoxVoiceClient::setRenderDevice(const std::string& name)
{
	if(name == "Default")
	{
		if(!mRenderDevice.empty())
		{
			mRenderDevice.clear();
			mRenderDeviceDirty = true;
		}
	}
	else
	{
		if(mRenderDevice != name)
		{
			mRenderDevice = name;
			mRenderDeviceDirty = true;
		}
	}
}
void LLVivoxVoiceClient::tuningStart()
{
	mTuningMode = true;
	LL_DEBUGS("Voice") << "Starting tuning" << LL_ENDL;
	if(getState() >= stateNoChannel)
	{
		LL_DEBUGS("Voice") << "no channel" << LL_ENDL;
		sessionTerminate();
	}
}
void LLVivoxVoiceClient::tuningStop()
{
	mTuningMode = false;
}
bool LLVivoxVoiceClient::inTuningMode()
{
	bool result = false;
	switch(getState())
	{
	case stateMicTuningRunning:
		result = true;
		break;
	default:
		break;
	}
	return result;
}
void LLVivoxVoiceClient::tuningRenderStartSendMessage(const std::string& name, bool loop)
{
	mTuningAudioFile = name;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStart.1\">"
	<< "<SoundFilePath>" << mTuningAudioFile << "</SoundFilePath>"
	<< "<Loop>" << (loop?"1":"0") << "</Loop>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::tuningRenderStopSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
	<< "<SoundFilePath>" << mTuningAudioFile << "</SoundFilePath>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::tuningCaptureStartSendMessage(int duration)
{
	LL_DEBUGS("Voice") << "sending CaptureAudioStart" << LL_ENDL;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStart.1\">"
	<< "<Duration>" << duration << "</Duration>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::tuningCaptureStopSendMessage()
{
	LL_DEBUGS("Voice") << "sending CaptureAudioStop" << LL_ENDL;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
	<< "</Request>\n\n\n";
	writeString(stream.str());
	mTuningEnergy = 0.0f;
}
void LLVivoxVoiceClient::tuningSetMicVolume(float volume)
{
	int scaled_volume = scale_mic_volume(volume);
	if(scaled_volume != mTuningMicVolume)
	{
		mTuningMicVolume = scaled_volume;
		mTuningMicVolumeDirty = true;
	}
}
void LLVivoxVoiceClient::tuningSetSpeakerVolume(float volume)
{
	int scaled_volume = scale_speaker_volume(volume);
	if(scaled_volume != mTuningSpeakerVolume)
	{
		mTuningSpeakerVolume = scaled_volume;
		mTuningSpeakerVolumeDirty = true;
	}
}
float LLVivoxVoiceClient::tuningGetEnergy(void)
{
	return mTuningEnergy;
}
bool LLVivoxVoiceClient::deviceSettingsAvailable()
{
	bool result = true;
	if(!mConnected)
		result = false;
	if(mRenderDevices.empty())
		result = false;
	return result;
}
void LLVivoxVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
	if(clearCurrentList)
	{
		clearCaptureDevices();
		clearRenderDevices();
	}
	getCaptureDevicesSendMessage();
	getRenderDevicesSendMessage();
}
void LLVivoxVoiceClient::daemonDied()
{
	LL_WARNS("Voice") << "Connection to vivox daemon lost.  Resetting state."<< LL_ENDL;
	setState(stateDisableCleanup);
}
void LLVivoxVoiceClient::giveUp()
{
	closeSocket();
	cleanUp();
	setState(stateJail);
}
static void oldSDKTransform (LLVector3 &left, LLVector3 &up, LLVector3 &at, LLVector3d &pos, LLVector3 &vel)
{
	F32 nat[3], nup[3], nl[3];
	F64 npos[3];
#if 1
	nat[0] = left.mV[VX];
	nat[1] = up.mV[VX];
	nat[2] = at.mV[VX];
	nup[0] = left.mV[VZ];
	nup[1] = up.mV[VY];
	nup[2] = at.mV[VZ];
	nl[0] = left.mV[VY];
	nl[1] = up.mV[VZ];
	nl[2] = at.mV[VY];
	npos[0] = pos.mdV[VX];
	npos[1] = pos.mdV[VZ];
	npos[2] = pos.mdV[VY];
	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
	nat[0] = at.mV[2];
	nat[1] = 0;
	nat[2] = -1 * left.mV[2];
	nup[0] = 0;
	nup[1] = 1;
	nup[2] = 0;
	nl[0] = at.mV[0];
	nl[1] = 0;
	nl[2] = -1 * left.mV[0];
	npos[2] = pos.mdV[2] * -1.0;
	npos[1] = pos.mdV[1];
	npos[0] = pos.mdV[0];
	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
#else
	nat[0] = at.mV[VX];
	nat[1] = 0;
	nat[2] = -1 * up.mV[VZ];
	nup[0] = 0;
	nup[1] = 1;
	nup[2] = 0;
	nl[0] = left.mV[VX];
	nl[1] = 0;
	nl[2] = -1 * left.mV[VY];
	npos[0] = pos.mdV[VX];
	npos[1] = pos.mdV[VZ];
	npos[2] = pos.mdV[VY] * -1.0;
	nvel[0] = vel.mV[VX];
	nvel[1] = vel.mV[VZ];
	nvel[2] = vel.mV[VY];
	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
#endif
}
void LLVivoxVoiceClient::sendPositionalUpdate(void)
{
	std::ostringstream stream;
	if(mSpatialCoordsDirty)
	{
		LLVector3 l, u, a, vel;
		LLVector3d pos;
					mSpatialCoordsDirty = false;
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Set3DPosition.1\">"
			<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>";
		stream << "<SpeakerPosition>";
		l = mAvatarRot.getLeftRow();
		u = mAvatarRot.getUpRow();
		a = mAvatarRot.getFwdRow();
		pos = mAvatarPosition;
		vel = mAvatarVelocity;
		oldSDKTransform(l, u, a, pos, vel);
		stream
			<< "<Position>"
				<< "<X>" << pos.mdV[VX] << "</X>"
				<< "<Y>" << pos.mdV[VY] << "</Y>"
				<< "<Z>" << pos.mdV[VZ] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << vel.mV[VX] << "</X>"
				<< "<Y>" << vel.mV[VY] << "</Y>"
				<< "<Z>" << vel.mV[VZ] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << a.mV[VX] << "</X>"
				<< "<Y>" << a.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << u.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << u.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
			<< "<LeftOrientation>"
				<< "<X>" << l.mV [VX] << "</X>"
				<< "<Y>" << l.mV [VY] << "</Y>"
				<< "<Z>" << l.mV [VZ] << "</Z>"
			<< "</LeftOrientation>";
		stream << "</SpeakerPosition>";
		stream << "<ListenerPosition>";
		if (mEarLocation != earLocSpeaker)
		{
			LLVector3d	earPosition;
			LLVector3	earVelocity;
			LLMatrix3	earRot;
			switch(mEarLocation)
			{
				case earLocCamera:
				default:
					earPosition = mCameraPosition;
					earVelocity = mCameraVelocity;
					earRot = mCameraRot;
				break;
				case earLocAvatar:
					earPosition = mAvatarPosition;
					earVelocity = mAvatarVelocity;
					earRot = mAvatarRot;
				break;
				case earLocMixed:
					earPosition = mAvatarPosition;
					earVelocity = mAvatarVelocity;
					earRot = mCameraRot;
				break;
			}
			l = earRot.getLeftRow();
			u = earRot.getUpRow();
			a = earRot.getFwdRow();
			pos = earPosition;
			vel = earVelocity;
			oldSDKTransform(l, u, a, pos, vel);
		}
		stream
			<< "<Position>"
				<< "<X>" << pos.mdV[VX] << "</X>"
				<< "<Y>" << pos.mdV[VY] << "</Y>"
				<< "<Z>" << pos.mdV[VZ] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << vel.mV[VX] << "</X>"
				<< "<Y>" << vel.mV[VY] << "</Y>"
				<< "<Z>" << vel.mV[VZ] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << a.mV[VX] << "</X>"
				<< "<Y>" << a.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << u.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << u.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
			<< "<LeftOrientation>"
				<< "<X>" << l.mV [VX] << "</X>"
				<< "<Y>" << l.mV [VY] << "</Y>"
				<< "<Z>" << l.mV [VZ] << "</Z>"
			<< "</LeftOrientation>";
		stream << "</ListenerPosition>";
		stream << "</Request>\n\n\n";
	}
	if(mAudioSession && (mAudioSession->mVolumeDirty || mAudioSession->mMuteDirty))
	{
		participantList::iterator iter = mAudioSession->mParticipantList.begin();
		mAudioSession->mVolumeDirty = false;
		mAudioSession->mMuteDirty = false;
		for(; iter != mAudioSession->mParticipantList.end(); iter++)
		{
			participantState *p = &*iter;
			if(p->mVolumeDirty)
			{
				if(!p->mIsSelf)
				{
					S32 volume = ll_pos_round(p->mVolume / VOLUME_SCALE_VIVOX);
					bool mute = p->mOnMuteList;
					if(mute)
					{
						volume = 0;
						p->mVolumeSet = true;
					}
					if(volume == 0)
					{
						mute = true;
					}
					LL_DEBUGS("Voice") << "Setting volume/mute for avatar " << p->mAvatarID << " to " << volume << (mute ? "/true" : "/false") << LL_ENDL;
					stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetParticipantVolumeForMe.1\">"
						<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>"
						<< "<ParticipantURI>" << p->mURI << "</ParticipantURI>"
						<< "<Volume>" << volume << "</Volume>"
						<< "</Request>\n\n\n";
					if(!mAudioSession->mIsP2P)
					{
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetParticipantMuteForMe.1\">"
						  << "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>"
						  << "<ParticipantURI>" << p->mURI << "</ParticipantURI>"
						  << "<Mute>" << (mute?"1":"0") << "</Mute>"
						  << "<Scope>Audio</Scope>"
						  << "</Request>\n\n\n";
					}
				}
				p->mVolumeDirty = false;
			}
		}
	}
	buildLocalAudioUpdates(stream);
	if(!stream.str().empty())
	{
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::buildSetCaptureDevice(std::ostringstream &stream)
{
	if(mCaptureDeviceDirty)
	{
		LL_DEBUGS("Voice") << "Setting input device = \"" << mCaptureDevice << "\"" << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetCaptureDevice.1\">"
			<< "<CaptureDeviceSpecifier>" << mCaptureDevice << "</CaptureDeviceSpecifier>"
		<< "</Request>"
		<< "\n\n\n";
		mCaptureDeviceDirty = false;
	}
}
void LLVivoxVoiceClient::buildSetRenderDevice(std::ostringstream &stream)
{
	if(mRenderDeviceDirty)
	{
		LL_DEBUGS("Voice") << "Setting output device = \"" << mRenderDevice << "\"" << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetRenderDevice.1\">"
			<< "<RenderDeviceSpecifier>" << mRenderDevice << "</RenderDeviceSpecifier>"
		<< "</Request>"
		<< "\n\n\n";
		mRenderDeviceDirty = false;
	}
}
void LLVivoxVoiceClient::buildLocalAudioUpdates(std::ostringstream &stream)
{
	buildSetCaptureDevice(stream);
	buildSetRenderDevice(stream);
	if(mMuteMicDirty)
	{
		mMuteMicDirty = false;
		LL_DEBUGS("Voice") << "Sending MuteLocalMic command with parameter " << (mMuteMic?"true":"false") << LL_ENDL;
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << (mMuteMic?"true":"false") << "</Value>"
			<< "</Request>\n\n\n";
	}
	if(mSpeakerMuteDirty)
	{
		const char *muteval = ((mSpeakerVolume <= scale_speaker_volume(0))?"true":"false");
		mSpeakerMuteDirty = false;
		LL_INFOS("Voice") << "Setting speaker mute to " << muteval  << LL_ENDL;
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalSpeaker.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << muteval << "</Value>"
			<< "</Request>\n\n\n";
	}
	if(mSpeakerVolumeDirty)
	{
		mSpeakerVolumeDirty = false;
		LL_INFOS("Voice") << "Setting speaker volume to " << mSpeakerVolume  << LL_ENDL;
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalSpeakerVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mSpeakerVolume << "</Value>"
			<< "</Request>\n\n\n";
	}
	if(mMicVolumeDirty)
	{
		mMicVolumeDirty = false;
		LL_INFOS("Voice") << "Setting mic volume to " << mMicVolume  << LL_ENDL;
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalMicVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mMicVolume << "</Value>"
			<< "</Request>\n\n\n";
	}
}
void LLVivoxVoiceClient::connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle, std::string &versionID)
{
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.Create response failure: " << statusString << LL_ENDL;
		setState(stateConnectorFailed);
		LLSD args;
		std::stringstream errs;
		errs << mVoiceAccountServerURI << "\n:UDP: 3478, 3479, 5060, 5062, 12000-17000";
		args["HOSTID"] = errs.str();
		mTerminateDaemon = true;
		LLNotificationsUtil::add("NoVoiceConnect", args);
	}
	else
	{
		LL_INFOS("Voice") << "Connector.Create succeeded, Vivox SDK version is " << versionID << LL_ENDL;
		mVoiceVersion.serverVersion = versionID;
		mConnectorHandle = connectorHandle;
		mTerminateDaemon = false;
		if(getState() == stateConnectorStarting)
		{
			setState(stateConnectorStarted);
		}
	}
}
void LLVivoxVoiceClient::loginResponse(int statusCode, std::string &statusString, std::string &accountHandle, int numberOfAliases)
{
	LL_DEBUGS("Voice") << "Account.Login response (" << statusCode << "): " << statusString << LL_ENDL;
	if ( statusCode == HTTP_UNAUTHORIZED )
	{
		LL_INFOS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
		setState(stateLoginRetry);
	}
	else if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
		setState(stateLoginFailed);
	}
	else
	{
		mAccountHandle = accountHandle;
		mNumberOfAliases = numberOfAliases;
	}
}
void LLVivoxVoiceClient::sessionCreateResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{
	sessionState *session = findSessionBeingCreatedByURI(requestId);
	if(session)
	{
		session->mCreateInProgress = false;
	}
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Session.Create response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mErrorStatusCode = statusCode;
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
			{
				setState(stateJoinSessionFailed);
			}
			else
			{
				reapSession(session);
			}
		}
	}
	else
	{
		LL_INFOS("Voice") << "Session.Create response received (success), session handle is " << sessionHandle << LL_ENDL;
		if(session)
		{
			setSessionHandle(session, sessionHandle);
		}
	}
}
void LLVivoxVoiceClient::sessionGroupAddSessionResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{
	sessionState *session = findSessionBeingCreatedByURI(requestId);
	if(session)
	{
		session->mCreateInProgress = false;
	}
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "SessionGroup.AddSession response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mErrorStatusCode = statusCode;
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
			{
				setState(stateJoinSessionFailed);
			}
			else
			{
				reapSession(session);
			}
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "SessionGroup.AddSession response received (success), session handle is " << sessionHandle << LL_ENDL;
		if(session)
		{
			setSessionHandle(session, sessionHandle);
		}
	}
}
void LLVivoxVoiceClient::sessionConnectResponse(std::string &requestId, int statusCode, std::string &statusString)
{
	sessionState *session = findSession(requestId);
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Session.Connect response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mMediaConnectInProgress = false;
			session->mErrorStatusCode = statusCode;
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
				setState(stateJoinSessionFailed);
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Session.Connect response received (success)" << LL_ENDL;
	}
}
void LLVivoxVoiceClient::logoutResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Logout response failure: " << statusString << LL_ENDL;
	}
}
void LLVivoxVoiceClient::connectorShutdownResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.InitiateShutdown response failure: " << statusString << LL_ENDL;
	}
	mConnected = false;
	if(getState() == stateConnectorStopping)
	{
		setState(stateConnectorStopped);
	}
}
void LLVivoxVoiceClient::sessionAddedEvent(
		std::string &uriString,
		std::string &alias,
		std::string &sessionHandle,
		std::string &sessionGroupHandle,
		bool isChannel,
		bool incoming,
		std::string &nameString,
		std::string &applicationString)
{
	sessionState *session = NULL;
	LL_INFOS("Voice") << "session " << uriString << ", alias " << alias << ", name " << nameString << " handle " << sessionHandle << LL_ENDL;
	session = addSession(uriString, sessionHandle);
	if(session)
	{
		session->mGroupHandle = sessionGroupHandle;
		session->mIsChannel = isChannel;
		session->mIncoming = incoming;
		session->mAlias = alias;
		if(!session->mIsChannel)
		{
			if(IDFromName(session->mSIPURI, session->mCallerID))
			{
			}
			else if(!session->mAlias.empty() && IDFromName(session->mAlias, session->mCallerID))
			{
				session->mAlternateSIPURI = session->mSIPURI;
				setSessionURI(session, sipURIFromID(session->mCallerID));
			}
			else
			{
				LL_INFOS("Voice") << "Could not generate caller id from uri, using hash of uri " << session->mSIPURI << LL_ENDL;
				session->mCallerID.generate(session->mSIPURI);
				session->mSynthesizedCallerID = true;
				std::string namePortion = nameFromsipURI(session->mSIPURI);
				if(namePortion.empty())
				{
					namePortion = nameString;
				}
				LLStringUtil::replaceChar(namePortion, '_', ' ');
				avatarNameResolved(session->mCallerID, namePortion);
			}
			LL_INFOS("Voice") << "caller ID: " << session->mCallerID << LL_ENDL;
			if(!session->mSynthesizedCallerID)
			{
				lookupName(session->mCallerID);
			}
		}
	}
}
void LLVivoxVoiceClient::sessionGroupAddedEvent(std::string &sessionGroupHandle)
{
	LL_DEBUGS("Voice") << "handle " << sessionGroupHandle << LL_ENDL;
#if USE_SESSION_GROUPS
	if(mMainSessionGroupHandle.empty())
	{
		mMainSessionGroupHandle = sessionGroupHandle;
	}
	else
	{
		LL_DEBUGS("Voice") << "Already had a session group handle " << mMainSessionGroupHandle << LL_ENDL;
	}
#endif
}
void LLVivoxVoiceClient::joinedAudioSession(sessionState *session)
{
	LL_DEBUGS("Voice") << "Joined Audio Session" << LL_ENDL;
	if(mAudioSession != session)
	{
		sessionState *oldSession = mAudioSession;
		mAudioSession = session;
		mAudioSessionChanged = true;
		reapSession(oldSession);
	}
	if(getState() == stateJoiningSession)
	{
		setState(stateSessionJoined);
		participantState *participant = session->addParticipant(sipURIFromName(mAccountName));
		if(participant)
		{
			participant->mIsSelf = true;
			lookupName(participant->mAvatarID);
			LL_INFOS("Voice") << "added self as participant \"" << participant->mAccountName
					<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;
		}
		if(!session->mIsChannel)
		{
			participantState *participant = session->addParticipant(session->mSIPURI);
			if(participant)
			{
				if(participant->mAvatarIDValid)
				{
					lookupName(participant->mAvatarID);
				}
				else if(!session->mName.empty())
				{
					participant->mDisplayName = session->mName;
					avatarNameResolved(participant->mAvatarID, session->mName);
				}
				LL_INFOS("Voice") << "added caller as participant \"" << participant->mAccountName
						<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;
			}
		}
	}
}
void LLVivoxVoiceClient::sessionRemovedEvent(
	std::string &sessionHandle,
	std::string &sessionGroupHandle)
{
	LL_INFOS("Voice") << "handle " << sessionHandle << LL_ENDL;
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		leftAudioSession(session);
		setSessionHandle(session);
		sessionGroupTerminateSendMessage(session);
		session->mMediaStreamState = streamStateUnknown;
		session->mTextStreamState = streamStateUnknown;
		reapSession(session);
	}
	else
	{
		LL_WARNS("Voice") << "unknown session " << sessionHandle << " removed" << LL_ENDL;
	}
}
void LLVivoxVoiceClient::reapSession(sessionState *session)
{
	if(session)
	{
		if(!session->mHandle.empty())
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (non-null session handle)" << LL_ENDL;
		}
		else if(session->mCreateInProgress)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (create in progress)" << LL_ENDL;
		}
		else if(session->mMediaConnectInProgress)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (connect in progress)" << LL_ENDL;
		}
		else if(session == mAudioSession)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (it's the current session)" << LL_ENDL;
		}
		else if(session == mNextAudioSession)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (it's the next session)" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("Voice") << "deleting session " << session->mSIPURI << LL_ENDL;
			deleteSession(session);
			session = NULL;
		}
	}
	else
	{
	}
}
bool LLVivoxVoiceClient::sessionNeedsRelog(sessionState *session)
{
	bool result = false;
	if(session != NULL)
	{
		if(session->mIsSpatial)
		{
			std::string::size_type atsign;
			atsign = session->mSIPURI.find("@");
			if(atsign != std::string::npos)
			{
				std::string urihost = session->mSIPURI.substr(atsign + 1);
				if(stricmp(urihost.c_str(), mVoiceSIPURIHostName.c_str()))
				{
					result = true;
				}
			}
		}
	}
	return result;
}
void LLVivoxVoiceClient::leftAudioSession(
	sessionState *session)
{
	if(mAudioSession == session)
	{
		switch(getState())
		{
			case stateJoiningSession:
			case stateSessionJoined:
			case stateRunning:
			case stateLeavingSession:
			case stateJoinSessionFailed:
			case stateJoinSessionFailedWaiting:
				LL_DEBUGS("Voice") << "left session " << session->mHandle << " in state " << state2string(getState()) << LL_ENDL;
				setState(stateSessionTerminated);
		break;
			case stateSessionTerminated:
				LL_WARNS("Voice") << "left session " << session->mHandle << " in state " << state2string(getState()) << LL_ENDL;
		break;
			default:
				LL_WARNS("Voice") << "unexpected SessionStateChangeEvent (left session) in state " << state2string(getState()) << LL_ENDL;
				setState(stateSessionTerminated);
			break;
		}
	}
}
void LLVivoxVoiceClient::accountLoginStateChangeEvent(
		std::string &accountHandle,
		int statusCode,
		std::string &statusString,
		int state)
{
	LL_DEBUGS("Voice") << "state change event: " << state << LL_ENDL;
	switch(state)
	{
		case 1:
		if(getState() == stateLoggingIn)
		{
			setState(stateLoggedIn);
		}
		break;
		case 3:
			setState(stateLoggingOut);
		break;
		case 0:
			setState(stateLoggedOut);
		break;
		default:
			LL_DEBUGS("Voice") << "unknown state: " << state << LL_ENDL;
		break;
	}
}
void LLVivoxVoiceClient::mediaCompletionEvent(std::string &sessionGroupHandle, std::string &mediaCompletionType)
{
	if (mediaCompletionType == "AuxBufferAudioCapture")
	{
		mCaptureBufferRecording = false;
	}
	else if (mediaCompletionType == "AuxBufferAudioRender")
	{
		if (--mPlayRequestCount <= 0)
		{
			mCaptureBufferPlaying = false;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Unknown MediaCompletionType: " << mediaCompletionType << LL_ENDL;
	}
}
void LLVivoxVoiceClient::mediaStreamUpdatedEvent(
	std::string &sessionHandle,
	std::string &sessionGroupHandle,
	int statusCode,
	std::string &statusString,
	int state,
	bool incoming)
{
	sessionState *session = findSession(sessionHandle);
	LL_DEBUGS("Voice") << "session " << sessionHandle << ", status code " << statusCode << ", string \"" << statusString << "\"" << LL_ENDL;
	if(session)
	{
		session->mMediaStreamState = state;
		switch(statusCode)
		{
			case 0:
			case HTTP_OK:
			break;
			default:
				session->mErrorStatusCode = statusCode;
			break;
		}
		switch(state)
		{
			case streamStateIdle:
				session->mVoiceEnabled = false;
				session->mMediaConnectInProgress = false;
				leftAudioSession(session);
			break;
			case streamStateConnected:
				session->mVoiceEnabled = true;
				session->mMediaConnectInProgress = false;
				joinedAudioSession(session);
			break;
			case streamStateRinging:
				if(incoming)
				{
					session->mIMSessionID = LLIMMgr::computeSessionID(IM_SESSION_P2P_INVITE, session->mCallerID);
					session->mVoiceInvitePending = true;
					if(session->mName.empty())
					{
						lookupName(session->mCallerID);
					}
					else
					{
						avatarNameResolved(session->mCallerID, session->mName);
					}
				}
			break;
			default:
				LL_WARNS("Voice") << "unknown state " << state << LL_ENDL;
			break;
		}
	}
	else
	{
		LL_WARNS("Voice") << "session " << sessionHandle << "not found"<< LL_ENDL;
	}
}
void LLVivoxVoiceClient::textStreamUpdatedEvent(
	std::string &sessionHandle,
	std::string &sessionGroupHandle,
	bool enabled,
	int state,
	bool incoming)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		session->mTextStreamState = state;
		switch(state)
		{
			case 0:
				LL_DEBUGS("Voice") << "stream closed" << LL_ENDL;
			break;
			case 1:
				sendQueuedTextMessages(session);
				session->mTextInvitePending = true;
				if(session->mName.empty())
				{
					lookupName(session->mCallerID);
				}
				else
				{
					avatarNameResolved(session->mCallerID, session->mName);
				}
			break;
			default:
				LL_WARNS("Voice") << "unknown state " << state << LL_ENDL;
			break;
		}
	}
}
void LLVivoxVoiceClient::participantAddedEvent(
		std::string &sessionHandle,
		std::string &sessionGroupHandle,
		std::string &uriString,
		std::string &alias,
		std::string &nameString,
		std::string &displayNameString,
		int participantType)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->addParticipant(uriString);
		if(participant)
		{
			participant->mAccountName = nameString;
			LL_DEBUGS("Voice") << "added participant \"" << participant->mAccountName
					<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;
			if(participant->mAvatarIDValid)
			{
				lookupName(participant->mAvatarID);
			}
			else
			{
				std::string namePortion = nameFromsipURI(uriString);
				if(namePortion.empty())
				{
					namePortion = displayNameString;
				}
				if(namePortion.empty())
				{
					namePortion = nameString;
				}
				participant->mDisplayName = namePortion;
				avatarNameResolved(participant->mAvatarID, namePortion);
			}
		}
	}
}
void LLVivoxVoiceClient::participantRemovedEvent(
		std::string &sessionHandle,
		std::string &sessionGroupHandle,
		std::string &uriString,
		std::string &alias,
		std::string &nameString)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		session->removeParticipant(uriString);
	}
	else
	{
		LL_DEBUGS("Voice") << "unknown session " << sessionHandle << LL_ENDL;
	}
}
void LLVivoxVoiceClient::participantUpdatedEvent(
		std::string &sessionHandle,
		std::string &sessionGroupHandle,
		std::string &uriString,
		std::string &alias,
		bool isModeratorMuted,
		bool isSpeaking,
		int volume,
		F32 energy)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
		if(participant)
		{
			participant->mIsSpeaking = isSpeaking;
			participant->mIsModeratorMuted = isModeratorMuted;
			if (isSpeaking)
			{
				participant->mSpeakingTimeout.reset();
				participant->mPower = energy;
			}
			else
			{
				participant->mPower = 0.0f;
			}
			if ( !participant->mVolumeSet && !participant->mVolumeDirty)
			{
				participant->mVolume = (F32)volume * VOLUME_SCALE_VIVOX;
			}
			LLVoiceChannel* voice_cnl = LLVoiceChannel::getCurrentVoiceChannel();
			bool moderate = gAgentID == participant->mAvatarID;
			if (voice_cnl && voice_cnl->getSessionID().notNull())
			{
				if (LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(voice_cnl->getSessionID()))
				if (LLSpeakerMgr* speaker_manager = floaterp->getSpeakerManager())
				{
					speaker_manager->update(true);
					if (moderate)
					{
						speaker_manager->initVoiceModerateMode();
					}
				}
			}
			else if (voice_cnl)
			{
				LLLocalSpeakerMgr::instance().update(true);
			}
			auto& inst(LLActiveSpeakerMgr::instance());
			inst.update(true);
			if (moderate) inst.initVoiceModerateMode();
		}
		else
		{
			LL_WARNS("Voice") << "unknown participant: " << uriString << LL_ENDL;
		}
	}
	else
	{
		LL_INFOS("Voice") << "unknown session " << sessionHandle << LL_ENDL;
	}
}
void LLVivoxVoiceClient::messageEvent(
		std::string &sessionHandle,
		std::string &uriString,
		std::string &alias,
		std::string &messageHeader,
		std::string &messageBody,
		std::string &applicationString)
{
	LL_DEBUGS("Voice") << "Message event, session " << sessionHandle << " from " << uriString << LL_ENDL;
	if(messageHeader.find("text/html") != std::string::npos)
	{
		std::string message;
		{
			const std::string startMarker = "<body";
			const std::string startMarker2 = ">";
			const std::string endMarker = "</body>";
			const std::string startSpan = "<span";
			const std::string endSpan = "</span>";
			std::string::size_type start;
			std::string::size_type end;
			message = messageBody;
			start = messageBody.find(startMarker);
			start = messageBody.find(startMarker2, start);
			end = messageBody.find(endMarker);
			if(start != std::string::npos)
			{
				start += startMarker2.size();
				if(end != std::string::npos)
					end -= start;
				message.assign(messageBody, start, end);
			}
			else
			{
				start = messageBody.find(startSpan);
				start = messageBody.find(startMarker2, start);
				end = messageBody.find(endSpan);
				if(start != std::string::npos)
				{
					start += startMarker2.size();
					if(end != std::string::npos)
						end -= start;
					message.assign(messageBody, start, end);
				}
			}
		}
		{
			std::string::size_type start;
			std::string::size_type end;
			while((start = message.find('<')) != std::string::npos)
			{
				if((end = message.find('>', start + 1)) != std::string::npos)
				{
					message.erase(start, (end + 1) - start);
				}
				else
				{
					break;
				}
			}
		}
		{
			std::string::size_type mark = 0;
			mark = 0;
			while((mark = message.find("&lt;", mark)) != std::string::npos)
			{
				message.replace(mark, 4, "<");
				mark += 1;
			}
			mark = 0;
			while((mark = message.find("&gt;", mark)) != std::string::npos)
			{
				message.replace(mark, 4, ">");
				mark += 1;
			}
			mark = 0;
			while((mark = message.find("&amp;", mark)) != std::string::npos)
			{
				message.replace(mark, 5, "&");
				mark += 1;
			}
		}
		LLStringUtil::trim(message);
		sessionState *session = findSession(sessionHandle);
		if(session)
		{
			bool is_do_not_disturb = gAgent.isDoNotDisturb();
			bool is_muted = LLMuteList::getInstance()->isMuted(session->mCallerID, session->mName, LLMute::flagTextChat);
			bool is_linden = LLMuteList::getInstance()->isLinden(session->mName);
			LLChat chat;
			chat.mMuted = is_muted && !is_linden;
			if(!chat.mMuted)
			{
				chat.mFromID = session->mCallerID;
				chat.mFromName = session->mName;
				chat.mSourceType = CHAT_SOURCE_AGENT;
				if(is_do_not_disturb && !is_linden)
				{
				}
				LL_DEBUGS("Voice") << "adding message, name " << session->mName << " session " << session->mIMSessionID << ", target " << session->mCallerID << LL_ENDL;
				LLIMMgr::getInstance()->addMessage(session->mIMSessionID,
						session->mCallerID,
						session->mName.c_str(),
						message.c_str(),
						LLStringUtil::null,
						IM_NOTHING_SPECIAL,
						0,
						LLUUID::null,
						LLVector3::zero,
						true);
			}
		}
	}
}
void LLVivoxVoiceClient::sessionNotificationEvent(std::string &sessionHandle, std::string &uriString, std::string &notificationType)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
		if(participant)
		{
			if (!stricmp(notificationType.c_str(), "Typing"))
			{
			}
			else if (!stricmp(notificationType.c_str(), "NotTyping"))
			{
			}
			else
			{
				LL_DEBUGS("Voice") << "Unknown notification type " << notificationType << "for participant " << uriString << " in session " << session->mSIPURI << LL_ENDL;
			}
		}
		else
		{
			LL_DEBUGS("Voice") << "Unknown participant " << uriString << " in session " << session->mSIPURI << LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Unknown session handle " << sessionHandle << LL_ENDL;
	}
}
void LLVivoxVoiceClient::auxAudioPropertiesEvent(F32 energy)
{
	LL_DEBUGS("Voice") << "got energy " << energy << LL_ENDL;
	mTuningEnergy = energy;
}
void LLVivoxVoiceClient::muteListChanged()
{
	if(mAudioSession)
	{
		participantList::iterator iter = mAudioSession->mParticipantList.begin();
		for(; iter != mAudioSession->mParticipantList.end(); iter++)
		{
			if(iter->updateMuteState())
				mAudioSession->mVolumeDirty = true;
		}
	}
}
LLVivoxVoiceClient::participantState::participantState(const std::string &uri, const LLUUID& id, bool isAv) :
	 mURI(uri),
	 mPTT(false),
	 mIsSpeaking(false),
	 mIsModeratorMuted(false),
	 mLastSpokeTimestamp(0.f),
	 mPower(0.f),
	 mVolume(LLVoiceClient::VOLUME_DEFAULT),
	 mUserVolume(0),
	 mOnMuteList(false),
	 mVolumeSet(false),
	 mVolumeDirty(false),
	 mAvatarIDValid(isAv),
	 mIsSelf(false),
	 mAvatarID(id)
{
}
LLVivoxVoiceClient::participantState *LLVivoxVoiceClient::sessionState::addParticipant(const std::string &uri)
{
	if (participantState* p = findParticipant(uri))
		return p;
	const std::string& desired_uri = (!mAlternateSIPURI.empty() && (uri == mAlternateSIPURI)) ? mSIPURI : uri;
	{
		mParticipantsChanged = true;
		{
			LLUUID id;
			if (LLVivoxVoiceClient::getInstance()->IDFromName(desired_uri, id))
			{
				mParticipantList.push_back(participantState(desired_uri, id, true));
			}
			else
			{
				id.generate(uri);
				mParticipantList.push_back(participantState(desired_uri, id, false));
			}
		}
		participantState* result = &mParticipantList.back();
		if (result->updateMuteState())
		{
			mMuteDirty = true;
		}
		if (LLSpeakerVolumeStorage::getInstance()->getSpeakerVolume(result->mAvatarID, result->mVolume))
		{
			result->mVolumeDirty = true;
			mVolumeDirty = true;
		}
		LL_DEBUGS("Voice") << "participant \"" << result->mURI << "\" added." << LL_ENDL;
		return result;
	}
}
bool LLVivoxVoiceClient::participantState::updateMuteState()
{
	bool result = false;
	bool isMuted = LLMuteList::getInstance()->isMuted(mAvatarID, LLMute::flagVoiceChat);
	if(mOnMuteList != isMuted)
	{
		mOnMuteList = isMuted;
		mVolumeDirty = true;
		result = true;
	}
	return result;
}
bool LLVivoxVoiceClient::participantState::isAvatar()
{
	return mAvatarIDValid;
}
void LLVivoxVoiceClient::sessionState::removeParticipant(const std::string& uri)
{
	participantList::iterator iter = std::find_if(mParticipantList.begin(), mParticipantList.end(), boost::bind(&participantList::value_type::mURI, _1) == uri);
	if (iter != mParticipantList.end())
	{
		vector_replace_with_last(mParticipantList, iter);
		if (mParticipantList.empty() || mParticipantList.capacity() - mParticipantList.size() > 16)
		{
			mParticipantList.shrink_to_fit();
		}
		mParticipantsChanged = true;
		LL_DEBUGS("Voice") << "participant \"" << uri << "\" (" << iter->mAvatarID << ") removed." << LL_ENDL;
	}
	else
	{
		LL_DEBUGS("Voice") << "unknown participant " << uri << LL_ENDL;
	}
}
void LLVivoxVoiceClient::sessionState::removeAllParticipants()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mParticipantList.clear();
	mParticipantList.shrink_to_fit();
}
void LLVivoxVoiceClient::getParticipantList(uuid_set_t &participants)
{
	if(mAudioSession)
	{
		for (participantList::iterator iter = mAudioSession->mParticipantList.begin();
			iter != mAudioSession->mParticipantList.end();
			iter++)
		{
			participants.insert(iter->mAvatarID);
		}
	}
}
bool LLVivoxVoiceClient::isParticipant(const LLUUID &speaker_id)
{
	return findParticipantByID(speaker_id);
}
LLVivoxVoiceClient::participantState *LLVivoxVoiceClient::sessionState::findParticipant(const std::string &uri)
{
	participantList& vec = mParticipantList;
	participantList::iterator iter = std::find_if(vec.begin(), vec.end(), boost::bind(&participantList::value_type::mURI, _1) == uri);
	if(iter == vec.end())
	{
		if(!mAlternateSIPURI.empty() && (uri == mAlternateSIPURI))
		{
			iter = std::find_if(vec.begin(), vec.end(), boost::bind(&participantList::value_type::mURI, _1) == mSIPURI);
		}
	}
	if (iter != mParticipantList.end())
	{
		return &*iter;
	}
	return NULL;
}
LLVivoxVoiceClient::participantState* LLVivoxVoiceClient::sessionState::findParticipantByID(const LLUUID& id)
{
	participantList& vec = mParticipantList;
	participantList::iterator iter = std::find_if(vec.begin(), vec.end(), boost::bind(&participantList::value_type::mAvatarID, _1) == id);
	if(iter != mParticipantList.end())
	{
		return &*iter;
	}
	return NULL;
}
LLVivoxVoiceClient::participantState* LLVivoxVoiceClient::findParticipantByID(const LLUUID& id)
{
	return mAudioSession ? mAudioSession->findParticipantByID(id) : NULL;
}
bool LLVivoxVoiceClient::checkParcelChanged(bool update)
{
	LLViewerRegion *region = gAgent.getRegion();
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if(region && parcel)
	{
		S32 parcelLocalID = parcel->getLocalID();
		std::string regionName = region->getName();
		if(!regionName.empty())
		{
			if((parcelLocalID != mCurrentParcelLocalID) || (regionName != mCurrentRegionName))
			{
				if (update)
				{
					mCurrentParcelLocalID = parcelLocalID;
					mCurrentRegionName = regionName;
					mAreaVoiceDisabled = false;
				}
				return true;
			}
		}
	}
	return false;
}
bool LLVivoxVoiceClient::parcelVoiceInfoReceived(state requesting_state)
{
	if(getState() == stateRetrievingParcelVoiceInfo)
	{
		setState(requesting_state);
		return true;
	}
	else
	{
		return false;
	}
}
bool LLVivoxVoiceClient::requestParcelVoiceInfo()
{
	LLViewerRegion * region = gAgent.getRegion();
	if (region == NULL || !region->capabilitiesReceived())
	{
		LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest capability not yet available, deferring" << LL_ENDL;
		return false;
	}
	std::string url = gAgent.getRegion()->getCapability("ParcelVoiceInfoRequest");
	if (url.empty())
	{
		LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest capability not available in this region" << LL_ENDL;
		setState(stateDisableCleanup);
		return false;
	}
	else
	{
		checkParcelChanged(true);
		LLSD data;
		LL_DEBUGS("Voice") << "sending ParcelVoiceInfoRequest (" << mCurrentRegionName << ", " << mCurrentParcelLocalID << ")" << LL_ENDL;
		LLHTTPClient::post(
						url,
						data,
						new LLVivoxVoiceClientCapResponder(getState()));
		return true;
	}
}
void LLVivoxVoiceClient::switchChannel(
	std::string uri,
	bool spatial,
	bool no_reconnect,
	bool is_p2p,
	std::string hash)
{
	bool needsSwitch = false;
	LL_DEBUGS("Voice")
		<< "called in state " << state2string(getState())
		<< " with uri \"" << uri << "\""
		<< (spatial?", spatial is true":", spatial is false")
		<< LL_ENDL;
	switch(getState())
	{
		case stateJoinSessionFailed:
		case stateJoinSessionFailedWaiting:
		case stateNoChannel:
		case stateRetrievingParcelVoiceInfo:
			needsSwitch = true;
		break;
		default:
			if(mSessionTerminateRequested)
			{
				if(mNextAudioSession)
				{
					if(mNextAudioSession->mSIPURI != uri)
						needsSwitch = true;
				}
				else
				{
					if(!uri.empty())
					{
						needsSwitch = true;
					}
				}
			}
			else
			{
				if(mAudioSession)
				{
					if(mAudioSession->mSIPURI != uri)
					{
						needsSwitch = true;
					}
				}
				else
				{
					if(!uri.empty())
					{
						LL_WARNS("Voice") << "No current audio session." << LL_ENDL;
					}
				}
			}
		break;
	}
	if(needsSwitch)
	{
		if(uri.empty())
		{
			LL_DEBUGS("Voice") << "leaving channel" << LL_ENDL;
			sessionState *oldSession = mNextAudioSession;
			mNextAudioSession = NULL;
			reapSession(oldSession);
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
		}
		else
		{
			LL_DEBUGS("Voice") << "switching to channel " << uri << LL_ENDL;
			mNextAudioSession = addSession(uri);
			mNextAudioSession->mHash = hash;
			mNextAudioSession->mIsSpatial = spatial;
			mNextAudioSession->mReconnect = !no_reconnect;
			mNextAudioSession->mIsP2P = is_p2p;
		}
		if(getState() >= stateRetrievingParcelVoiceInfo)
		{
			sessionTerminate();
		}
	}
}
void LLVivoxVoiceClient::joinSession(sessionState *session)
{
	mNextAudioSession = session;
	if(getState() <= stateNoChannel)
	{
	}
	else
	{
		sessionTerminate();
	}
}
void LLVivoxVoiceClient::setNonSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	switchChannel(uri, false, false, false, credentials);
}
void LLVivoxVoiceClient::setSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	mSpatialSessionURI = uri;
	mSpatialSessionCredentials = credentials;
	mAreaVoiceDisabled = mSpatialSessionURI.empty();
	LL_DEBUGS("Voice") << "got spatial channel uri: \"" << uri << "\"" << LL_ENDL;
	if((mAudioSession && !(mAudioSession->mIsSpatial)) || (mNextAudioSession && !(mNextAudioSession->mIsSpatial)))
	{
		LL_INFOS("Voice") << "in non-spatial chat, not switching channels" << LL_ENDL;
	}
	else
	{
		switchChannel(mSpatialSessionURI, true, false, false, mSpatialSessionCredentials);
	}
}
void LLVivoxVoiceClient::callUser(const LLUUID &uuid)
{
	std::string userURI = sipURIFromID(uuid);
	switchChannel(userURI, false, true, true);
}
LLVivoxVoiceClient::sessionState* LLVivoxVoiceClient::startUserIMSession(const LLUUID &uuid)
{
	sessionState *session = findSession(uuid);
	if(!session)
	{
		std::string uri = sipURIFromID(uuid);
		session = addSession(uri);
		llassert(session);
		if (!session) return NULL;
		session->mIsSpatial = false;
		session->mReconnect = false;
		session->mIsP2P = true;
		session->mCallerID = uuid;
	}
	if(session->mHandle.empty())
	{
		sessionCreateSendMessage(session, false, true);
	}
	else
	{
		sessionTextConnectSendMessage(session);
	}
	return session;
}
BOOL LLVivoxVoiceClient::sendTextMessage(const LLUUID& participant_id, const std::string& message)
{
	bool result = false;
	sessionState *session = startUserIMSession(participant_id);
	if(session)
	{
		session->mTextMsgQueue.push(message);
		sendQueuedTextMessages(session);
		result = true;
	}
	else
	{
		LL_DEBUGS("Voice") << "Session not found for participant ID " << participant_id << LL_ENDL;
	}
	return result;
}
void LLVivoxVoiceClient::sendQueuedTextMessages(sessionState *session)
{
	if(session->mTextStreamState == 1)
	{
		if(!session->mTextMsgQueue.empty())
		{
			std::ostringstream stream;
			while(!session->mTextMsgQueue.empty())
			{
				std::string message = session->mTextMsgQueue.front();
				session->mTextMsgQueue.pop();
				stream
				<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SendMessage.1\">"
					<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
					<< "<MessageHeader>text/HTML</MessageHeader>"
					<< "<MessageBody>" << message << "</MessageBody>"
				<< "</Request>"
				<< "\n\n\n";
			}
			writeString(stream.str());
		}
	}
	else
	{
	}
}
void LLVivoxVoiceClient::endUserIMSession(const LLUUID &uuid)
{
	sessionState *session = findSession(uuid);
	if(session)
	{
		if(!session->mHandle.empty())
		{
			sessionTextDisconnectSendMessage(session);
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Session not found for participant ID " << uuid << LL_ENDL;
	}
}
bool LLVivoxVoiceClient::isValidChannel(std::string &sessionHandle)
{
	return(findSession(sessionHandle) != NULL);
}
bool LLVivoxVoiceClient::answerInvite(std::string &sessionHandle)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		session->mIsSpatial = false;
		session->mReconnect = false;
		session->mIsP2P = true;
		joinSession(session);
		return true;
	}
	return false;
}
bool LLVivoxVoiceClient::isVoiceWorking() const
{
	return (mSpatialJoiningNum < MAX_NORMAL_JOINING_SPATIAL_NUM) && (stateLoggedIn <= mState) && (mState <= stateSessionTerminated);
}
BOOL LLVivoxVoiceClient::isParticipantAvatar(const LLUUID &id)
{
	BOOL result = TRUE;
	sessionState *session = findSession(id);
	if(session != NULL)
	{
		if(session->mSynthesizedCallerID)
			result = FALSE;
	}
	else
	{
		if(mAudioSession != NULL)
		{
			participantState *participant = findParticipantByID(id);
			if(participant != NULL)
			{
				result = participant->isAvatar();
			}
		}
	}
	return result;
}
BOOL LLVivoxVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
	BOOL result = TRUE;
	sessionState *session = findSession(session_id);
	if(session != NULL)
	{
		result = session->isCallBackPossible();
	}
	return result;
}
BOOL LLVivoxVoiceClient::isSessionTextIMPossible(const LLUUID &session_id)
{
	bool result = TRUE;
	sessionState *session = findSession(session_id);
	if(session != NULL)
	{
		result = session->isTextIMPossible();
	}
	return result;
}
void LLVivoxVoiceClient::declineInvite(std::string &sessionHandle)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		sessionMediaDisconnectSendMessage(session);
	}
}
void LLVivoxVoiceClient::leaveNonSpatialChannel()
{
	LL_DEBUGS("Voice")
		<< "called in state " << state2string(getState())
		<< LL_ENDL;
	sessionState *oldNextSession = mNextAudioSession;
	mNextAudioSession = NULL;
	reapSession(oldNextSession);
	verifySessionState();
	sessionTerminate();
}
std::string LLVivoxVoiceClient::getCurrentChannel()
{
	std::string result;
	if((getState() == stateRunning) && !mSessionTerminateRequested)
	{
		result = getAudioSessionURI();
	}
	return result;
}
bool LLVivoxVoiceClient::inProximalChannel()
{
	bool result = false;
	if((getState() == stateRunning) && !mSessionTerminateRequested)
	{
		result = inSpatialChannel();
	}
	return result;
}
std::string LLVivoxVoiceClient::sipURIFromID(const LLUUID &id)
{
	std::string result;
	result = "sip:";
	result += nameFromID(id);
	result += "@";
	result += mVoiceSIPURIHostName;
	return result;
}
std::string LLVivoxVoiceClient::sipURIFromAvatar(LLVOAvatar *avatar)
{
	std::string result;
	if(avatar)
	{
		result = "sip:";
		result += nameFromID(avatar->getID());
		result += "@";
		result += mVoiceSIPURIHostName;
	}
	return result;
}
std::string LLVivoxVoiceClient::nameFromAvatar(LLVOAvatar *avatar)
{
	std::string result;
	if(avatar)
	{
		result = nameFromID(avatar->getID());
	}
	return result;
}
std::string LLVivoxVoiceClient::nameFromID(const LLUUID &uuid)
{
	std::string result;
	if (uuid.isNull()) {
		LLStringUtil::replaceChar(result, '_', ' ');
		return result;
	}
	result = "x";
	result += LLBase64::encode(uuid.mData, UUID_BYTES);
	LLStringUtil::replaceChar(result, '+', '-');
	LLStringUtil::replaceChar(result, '/', '_');
	return result;
}
bool LLVivoxVoiceClient::IDFromName(const std::string inName, LLUUID &uuid)
{
	bool result = false;
	std::string name = nameFromsipURI(inName);
	if(name.empty())
		name = inName;
	if((name.size() == 25) && (name[0] == 'x') && (name[23] == '=') && (name[24] == '='))
	{
		std::string temp = name;
		LLStringUtil::replaceChar(temp, '-', '+');
		LLStringUtil::replaceChar(temp, '_', '/');
		U8 rawuuid[UUID_BYTES + 1];
		int len = apr_base64_decode_binary(rawuuid, temp.c_str() + 1);
		if(len == UUID_BYTES)
		{
			memcpy(uuid.mData, rawuuid, UUID_BYTES);
			result = true;
		}
	}
	if(!result)
	{
		uuid.setNull();
	}
	return result;
}
std::string LLVivoxVoiceClient::displayNameFromAvatar(LLVOAvatar *avatar)
{
	return avatar->getFullname();
}
std::string LLVivoxVoiceClient::sipURIFromName(std::string &name)
{
	std::string result;
	result = "sip:";
	result += name;
	result += "@";
	result += mVoiceSIPURIHostName;
	return result;
}
std::string LLVivoxVoiceClient::nameFromsipURI(const std::string &uri)
{
	std::string result;
	std::string::size_type sipOffset, atOffset;
	sipOffset = uri.find("sip:");
	atOffset = uri.find("@");
	if((sipOffset != std::string::npos) && (atOffset != std::string::npos))
	{
		result = uri.substr(sipOffset + 4, atOffset - (sipOffset + 4));
	}
	return result;
}
bool LLVivoxVoiceClient::inSpatialChannel(void)
{
	bool result = false;
	if(mAudioSession)
		result = mAudioSession->mIsSpatial;
	return result;
}
std::string LLVivoxVoiceClient::getAudioSessionURI()
{
	std::string result;
	if(mAudioSession)
		result = mAudioSession->mSIPURI;
	return result;
}
std::string LLVivoxVoiceClient::getAudioSessionHandle()
{
	std::string result;
	if(mAudioSession)
		result = mAudioSession->mHandle;
	return result;
}
void LLVivoxVoiceClient::enforceTether(void)
{
	LLVector3d tethered = mCameraRequestedPosition;
	{
		F32 max_dist = 50.0f;
		LLVector3d camera_offset = mCameraRequestedPosition - mAvatarPosition;
		F32 camera_distance = (F32)camera_offset.magVec();
		if(camera_distance > max_dist)
		{
			tethered = mAvatarPosition +
				(max_dist / camera_distance) * camera_offset;
		}
	}
	if(dist_vec_squared(mCameraPosition, tethered) > 0.01)
	{
		mCameraPosition = tethered;
		mSpatialCoordsDirty = true;
	}
}
void LLVivoxVoiceClient::updatePosition(void)
{
	LLViewerRegion *region = gAgent.getRegion();
	if(region && isAgentAvatarValid())
	{
		LLMatrix3 rot;
		LLVector3d pos;
		rot.setRows(LLViewerCamera::getInstance()->getAtAxis(), LLViewerCamera::getInstance()->getLeftAxis (),  LLViewerCamera::getInstance()->getUpAxis());
		pos = gAgent.getRegion()->getPosGlobalFromRegion(LLViewerCamera::getInstance()->getOrigin());
		LLVivoxVoiceClient::getInstance()->setCameraPosition(
															 pos,
															 LLVector3::zero,
															 rot);
		rot = gAgentAvatarp->getRootJoint()->getWorldRotation().getMatrix3();
		pos = gAgentAvatarp->getPositionGlobal();
		pos += LLVector3d(0.f, 0.f, 1.f);
		LLVivoxVoiceClient::getInstance()->setAvatarPosition(
															 pos,
															 LLVector3::zero,
															 rot);
	}
}
void LLVivoxVoiceClient::setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot)
{
	mCameraRequestedPosition = position;
	if(mCameraVelocity != velocity)
	{
		mCameraVelocity = velocity;
		mSpatialCoordsDirty = true;
	}
	if(mCameraRot != rot)
	{
		mCameraRot = rot;
		mSpatialCoordsDirty = true;
	}
}
void LLVivoxVoiceClient::setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot)
{
	if(dist_vec_squared(mAvatarPosition, position) > 0.01)
	{
		mAvatarPosition = position;
		mSpatialCoordsDirty = true;
	}
	if(mAvatarVelocity != velocity)
	{
		mAvatarVelocity = velocity;
		mSpatialCoordsDirty = true;
	}
	if(mAvatarRot != rot)
	{
		mAvatarRot = rot;
		mSpatialCoordsDirty = true;
	}
}
bool LLVivoxVoiceClient::channelFromRegion(LLViewerRegion *region, std::string &name)
{
	bool result = false;
	if(region)
	{
		name = region->getName();
	}
	if(!name.empty())
		result = true;
	return result;
}
void LLVivoxVoiceClient::leaveChannel(void)
{
	if(getState() == stateRunning)
	{
		LL_DEBUGS("Voice") << "leaving channel for teleport/logout" << LL_ENDL;
		mChannelName.clear();
		sessionTerminate();
	}
}
void LLVivoxVoiceClient::setMuteMic(bool muted)
{
	if(mMuteMic != muted)
	{
		mMuteMic = muted;
		mMuteMicDirty = true;
	}
}
void LLVivoxVoiceClient::setVoiceEnabled(bool enabled)
{
	if (enabled != mVoiceEnabled)
	{
		mVoiceEnabled = enabled;
		LLVoiceClientStatusObserver::EStatusType status;
		if (enabled)
		{
			LLVoiceChannel::getCurrentVoiceChannel()->activate();
			status = LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED;
		}
		else
		{
			LLVoiceChannel::getCurrentVoiceChannel()->deactivate();
			status = LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED;
		}
		notifyStatusObservers(status);
	}
}
bool LLVivoxVoiceClient::voiceEnabled()
{
	static const LLCachedControl<bool> enable_voice_chat("EnableVoiceChat",true);
	static const LLCachedControl<bool> cmdline_disable_voice("CmdLineDisableVoice",false);
	return enable_voice_chat && !cmdline_disable_voice;
}
void LLVivoxVoiceClient::setLipSyncEnabled(BOOL enabled)
{
	mLipSyncEnabled = enabled;
}
BOOL LLVivoxVoiceClient::lipSyncEnabled()
{
	if ( mVoiceEnabled && stateDisabled != getState() )
	{
		return mLipSyncEnabled;
	}
	else
	{
		return FALSE;
	}
}
void LLVivoxVoiceClient::setEarLocation(S32 loc)
{
	if(mEarLocation != loc)
	{
		LL_DEBUGS("Voice") << "Setting mEarLocation to " << loc << LL_ENDL;
		mEarLocation = loc;
		mSpatialCoordsDirty = true;
	}
}
void LLVivoxVoiceClient::setVoiceVolume(F32 volume)
{
	int scaled_volume = scale_speaker_volume(volume);
	if(scaled_volume != mSpeakerVolume)
	{
		int min_volume = scale_speaker_volume(0);
		if((scaled_volume == min_volume) || (mSpeakerVolume == min_volume))
		{
			mSpeakerMuteDirty = true;
		}
		mSpeakerVolume = scaled_volume;
		mSpeakerVolumeDirty = true;
	}
}
void LLVivoxVoiceClient::setMicGain(F32 volume)
{
	int scaled_volume = scale_mic_volume(volume);
	if(scaled_volume != mMicVolume)
	{
		mMicVolume = scaled_volume;
		mMicVolumeDirty = true;
	}
}
BOOL LLVivoxVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	BOOL result = FALSE;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = TRUE;
	}
	return result;
}
std::string LLVivoxVoiceClient::getDisplayName(const LLUUID& id)
{
	std::string result;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mDisplayName;
	}
	return result;
}
BOOL LLVivoxVoiceClient::getIsSpeaking(const LLUUID& id)
{
	BOOL result = FALSE;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		if (participant->mSpeakingTimeout.getElapsedTimeF32() > SPEAKING_TIMEOUT)
		{
			participant->mIsSpeaking = FALSE;
		}
		result = participant->mIsSpeaking;
	}
	return result;
}
BOOL LLVivoxVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	BOOL result = FALSE;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mIsModeratorMuted;
	}
	return result;
}
F32 LLVivoxVoiceClient::getCurrentPower(const LLUUID& id)
{
	F32 result = 0;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mPower;
	}
	return result;
}
BOOL LLVivoxVoiceClient::getUsingPTT(const LLUUID& id)
{
	BOOL result = FALSE;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
	}
	return result;
}
BOOL LLVivoxVoiceClient::getOnMuteList(const LLUUID& id)
{
	BOOL result = FALSE;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mOnMuteList;
	}
	return result;
}
F32 LLVivoxVoiceClient::getUserVolume(const LLUUID& id)
{
	F32 result = LLVoiceClient::VOLUME_MIN;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mVolume;
	}
	return result;
}
void LLVivoxVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
	if(mAudioSession)
	{
		participantState *participant = findParticipantByID(id);
		if (participant && !participant->mIsSelf)
		{
			if (!is_approx_equal(volume, LLVoiceClient::VOLUME_DEFAULT))
			{
				LLSpeakerVolumeStorage::getInstance()->storeSpeakerVolume(id, volume);
			}
			else
			{
				LLSpeakerVolumeStorage::getInstance()->removeSpeakerVolume(id);
			}
			participant->mVolume = llclamp(volume, LLVoiceClient::VOLUME_MIN, LLVoiceClient::VOLUME_MAX);
			participant->mVolumeDirty = true;
			mAudioSession->mVolumeDirty = true;
		}
	}
}
std::string LLVivoxVoiceClient::getGroupID(const LLUUID& id)
{
	std::string result;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mGroupID;
	}
	return result;
}
BOOL LLVivoxVoiceClient::getAreaVoiceDisabled()
{
	return mAreaVoiceDisabled;
}
void LLVivoxVoiceClient::recordingLoopStart(int seconds, int deltaFramesPerControlFrame)
{
	if(!mMainSessionGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Start</RecordingControlType>"
		<< "<DeltaFramesPerControlFrame>" << deltaFramesPerControlFrame << "</DeltaFramesPerControlFrame>"
		<< "<Filename>" << "" << "</Filename>"
		<< "<EnableAudioRecordingEvents>false</EnableAudioRecordingEvents>"
		<< "<LoopModeDurationSeconds>" << seconds << "</LoopModeDurationSeconds>"
		<< "</Request>\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::recordingLoopSave(const std::string& filename)
{
	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Flush</RecordingControlType>"
		<< "<Filename>" << filename << "</Filename>"
		<< "</Request>\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::recordingStop()
{
	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Stop</RecordingControlType>"
		<< "</Request>\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::filePlaybackStart(const std::string& filename)
{
	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlPlayback.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Start</RecordingControlType>"
		<< "<Filename>" << filename << "</Filename>"
		<< "</Request>\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::filePlaybackStop()
{
	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlPlayback.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Stop</RecordingControlType>"
		<< "</Request>\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::filePlaybackSetPaused(bool paused)
{
}
void LLVivoxVoiceClient::filePlaybackSetMode(bool vox, float speed)
{
}
LLVivoxVoiceClient::sessionState::sessionState() :
	mErrorStatusCode(0),
	mMediaStreamState(streamStateUnknown),
	mTextStreamState(streamStateUnknown),
	mCreateInProgress(false),
	mMediaConnectInProgress(false),
	mVoiceInvitePending(false),
	mTextInvitePending(false),
	mSynthesizedCallerID(false),
	mIsChannel(false),
	mIsSpatial(false),
	mIsP2P(false),
	mIncoming(false),
	mVoiceEnabled(false),
	mReconnect(false),
	mVolumeDirty(false),
	mMuteDirty(false),
	mParticipantsChanged(false)
{
}
LLVivoxVoiceClient::sessionState::~sessionState()
{
	removeAllParticipants();
}
bool LLVivoxVoiceClient::sessionState::isCallBackPossible()
{
	return !mSynthesizedCallerID;
}
bool LLVivoxVoiceClient::sessionState::isTextIMPossible()
{
	return !mSynthesizedCallerID;
}
LLVivoxVoiceClient::sessionIterator LLVivoxVoiceClient::sessionsBegin(void)
{
	return mSessions.begin();
}
LLVivoxVoiceClient::sessionIterator LLVivoxVoiceClient::sessionsEnd(void)
{
	return mSessions.end();
}
LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::findSession(const std::string &handle)
{
	sessionState *result = NULL;
	sessionMap::iterator iter = mSessionsByHandle.find(handle);
	if(iter != mSessionsByHandle.end())
	{
		result = iter->second;
	}
	return result;
}
LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::findSessionBeingCreatedByURI(const std::string &uri)
{
	sessionState *result = NULL;
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		if(session->mCreateInProgress && (session->mSIPURI == uri))
		{
			result = session;
			break;
		}
	}
	return result;
}
LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::findSession(const LLUUID &participant_id)
{
	sessionState *result = NULL;
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		if((session->mCallerID == participant_id) || (session->mIMSessionID == participant_id))
		{
			result = session;
			break;
		}
	}
	return result;
}
LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::addSession(const std::string &uri, const std::string &handle)
{
	sessionState *result = NULL;
	if(handle.empty())
	{
		for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
		{
			sessionState *s = *iter;
			if((s->mSIPURI == uri) || (s->mAlternateSIPURI == uri))
			{
				result = s;
				break;
			}
		}
	}
	else
	{
		sessionMap::iterator iter = mSessionsByHandle.find(handle);
		if(iter != mSessionsByHandle.end())
		{
			result = iter->second;
		}
	}
	if(!result)
	{
		LL_DEBUGS("Voice") << "adding new session: handle " << handle << " URI " << uri << LL_ENDL;
		result = new sessionState();
		result->mSIPURI = uri;
		result->mHandle = handle;
		if (LLVoiceClient::instance().getVoiceEffectEnabled())
		{
			result->mVoiceFontID = LLVoiceClient::instance().getVoiceEffectDefault();
		}
		mSessions.insert(result);
		if(!result->mHandle.empty())
		{
			mSessionsByHandle.insert(sessionMap::value_type(result->mHandle, result));
		}
	}
	else
	{
		if(uri != result->mSIPURI)
		{
			LL_DEBUGS("Voice") << "changing uri from " << result->mSIPURI << " to " << uri << LL_ENDL;
			setSessionURI(result, uri);
		}
		if(handle != result->mHandle)
		{
			if(handle.empty())
			{
				LL_DEBUGS("Voice") << "NOT clearing handle " << result->mHandle << LL_ENDL;
			}
			else
			{
				LL_DEBUGS("Voice") << "changing handle from " << result->mHandle << " to " << handle << LL_ENDL;
				setSessionHandle(result, handle);
			}
		}
		LL_DEBUGS("Voice") << "returning existing session: handle " << handle << " URI " << uri << LL_ENDL;
	}
	verifySessionState();
	return result;
}
void LLVivoxVoiceClient::setSessionHandle(sessionState *session, const std::string &handle)
{
	if(!session->mHandle.empty())
	{
		sessionMap::iterator iter = mSessionsByHandle.find(session->mHandle);
		if(iter != mSessionsByHandle.end())
		{
			if(iter->second != session)
			{
				LL_ERRS("Voice") << "Internal error: session mismatch!" << LL_ENDL;
			}
			mSessionsByHandle.erase(iter);
		}
		else
		{
			LL_ERRS("Voice") << "Internal error: session handle not found in map!" << LL_ENDL;
		}
	}
	session->mHandle = handle;
	if(!handle.empty())
	{
		mSessionsByHandle.insert(sessionMap::value_type(session->mHandle, session));
	}
	verifySessionState();
}
void LLVivoxVoiceClient::setSessionURI(sessionState *session, const std::string &uri)
{
	session->mSIPURI = uri;
	verifySessionState();
}
void LLVivoxVoiceClient::deleteSession(sessionState *session)
{
	if(!session->mHandle.empty())
	{
		sessionMap::iterator iter = mSessionsByHandle.find(session->mHandle);
		if(iter != mSessionsByHandle.end())
		{
			if(iter->second != session)
			{
				LL_ERRS("Voice") << "Internal error: session mismatch" << LL_ENDL;
			}
			mSessionsByHandle.erase(iter);
		}
	}
	mSessions.erase(session);
	verifySessionState();
	if(mAudioSession == session)
	{
		mAudioSession = NULL;
		mAudioSessionChanged = true;
	}
	if(mNextAudioSession == session)
	{
		mNextAudioSession = NULL;
	}
	delete session;
}
void LLVivoxVoiceClient::deleteAllSessions()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	while(!mSessions.empty())
	{
		deleteSession(*(sessionsBegin()));
	}
	if(!mSessionsByHandle.empty())
	{
		LL_ERRS("Voice") << "Internal error: empty session map, non-empty handle map" << LL_ENDL;
	}
}
void LLVivoxVoiceClient::verifySessionState(void)
{
	LL_DEBUGS("Voice") << "Total session count: " << mSessions.size() << " , session handle map size: " << mSessionsByHandle.size() << LL_ENDL;
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		LL_DEBUGS("Voice") << "session " << session << ": handle " << session->mHandle << ", URI " << session->mSIPURI << LL_ENDL;
		if(!session->mHandle.empty())
		{
			sessionMap::iterator i2 = mSessionsByHandle.find(session->mHandle);
			if(i2 == mSessionsByHandle.end())
			{
				LL_ERRS("Voice") << "internal error (handle " << session->mHandle << " not found in session map)" << LL_ENDL;
			}
			else
			{
				if(i2->second != session)
				{
					LL_ERRS("Voice") << "internal error (handle " << session->mHandle << " in session map points to another session)" << LL_ENDL;
				}
			}
		}
	}
	for(sessionMap::iterator iter = mSessionsByHandle.begin(); iter != mSessionsByHandle.end(); iter++)
	{
		sessionState *session = iter->second;
		sessionIterator i2 = mSessions.find(session);
		if(i2 == mSessions.end())
		{
			LL_ERRS("Voice") << "internal error (session for handle " << session->mHandle << " not found in session map)" << LL_ENDL;
		}
		else
		{
			if(session->mHandle != (*i2)->mHandle)
			{
				LL_ERRS("Voice") << "internal error (session for handle " << session->mHandle << " points to session with different handle " << (*i2)->mHandle << ")" << LL_ENDL;
			}
		}
	}
}
void LLVivoxVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.insert(observer);
}
void LLVivoxVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.erase(observer);
}
void LLVivoxVoiceClient::notifyParticipantObservers()
{
	for (observer_set_t::iterator it = mParticipantObservers.begin();
		it != mParticipantObservers.end();
		)
	{
		LLVoiceClientParticipantObserver* observer = *it;
		observer->onParticipantsChanged();
		it = mParticipantObservers.upper_bound(observer);
	}
}
void LLVivoxVoiceClient::addObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.insert(observer);
}
void LLVivoxVoiceClient::removeObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.erase(observer);
}
void LLVivoxVoiceClient::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
{
	if(mAudioSession)
	{
		if(status == LLVoiceClientStatusObserver::ERROR_UNKNOWN)
		{
			switch(mAudioSession->mErrorStatusCode)
			{
				case 20713:		status = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL; 		break;
				case 20714:		status = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED; 	break;
				case 20715:
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
					break;
				case 1009:
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
					break;
			}
			mAudioSession->mErrorStatusCode = 0;
		}
		else if(status == LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL)
		{
			switch(mAudioSession->mErrorStatusCode)
			{
				case HTTP_NOT_FOUND:
				case 480:
				case HTTP_REQUEST_TIME_OUT:
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
					mAudioSession->mErrorStatusCode = 0;
				break;
			}
		}
	}
	LL_DEBUGS("Voice")
		<< " " << LLVoiceClientStatusObserver::status2string(status)
		<< ", session URI " << getAudioSessionURI()
		<< (inSpatialChannel()?", proximal is true":", proximal is false")
	<< LL_ENDL;
	for (status_observer_set_t::iterator it = mStatusObservers.begin();
		it != mStatusObservers.end();
		)
	{
		LLVoiceClientStatusObserver* observer = *it;
		observer->onChange(status, getAudioSessionURI(), inSpatialChannel());
		it = mStatusObservers.upper_bound(observer);
	}
	if (   status != LLVoiceClientStatusObserver::STATUS_JOINING
		&& status != LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL)
	{
		bool voice_status = LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();
		gAgent.setVoiceConnected(voice_status);
	}
}
void LLVivoxVoiceClient::addObserver(LLFriendObserver* observer)
{
	mFriendObservers.insert(observer);
}
void LLVivoxVoiceClient::removeObserver(LLFriendObserver* observer)
{
	mFriendObservers.erase(observer);
}
void LLVivoxVoiceClient::notifyFriendObservers()
{
	for (friend_observer_set_t::iterator it = mFriendObservers.begin();
		it != mFriendObservers.end();
		)
	{
		LLFriendObserver* observer = *it;
		it++;
		observer->changed(LLFriendObserver::ONLINE);
	}
}
void LLVivoxVoiceClient::lookupName(const LLUUID &id)
{
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
	mAvatarNameCacheConnection = LLAvatarNameCache::get(id, boost::bind(&LLVivoxVoiceClient::onAvatarNameCache, this, _1, _2));
}
void LLVivoxVoiceClient::onAvatarNameCache(const LLUUID& agent_id,
										   const LLAvatarName& av_name)
{
	mAvatarNameCacheConnection.disconnect();
	std::string display_name = av_name.getDisplayName();
	avatarNameResolved(agent_id, display_name);
}
void LLVivoxVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		participantState *participant = session->findParticipantByID(id);
		if(participant)
		{
			participant->mAccountName = name;
			session->mParticipantsChanged = true;
		}
		if(session->mCallerID == id)
		{
			session->mName = name;
			if(session->mTextInvitePending)
			{
				session->mTextInvitePending = false;
			}
			if(session->mVoiceInvitePending)
			{
				session->mVoiceInvitePending = false;
				LLIMMgr::getInstance()->inviteToSession(
										session->mIMSessionID,
										session->mName,
										session->mCallerID,
										session->mName,
										IM_SESSION_P2P_INVITE,
										LLIMMgr::INVITATION_TYPE_VOICE,
										session->mHandle,
										session->mSIPURI);
			}
		}
	}
}
bool LLVivoxVoiceClient::setVoiceEffect(const LLUUID& id)
{
	if (!mAudioSession)
	{
		return false;
	}
	if (!id.isNull())
	{
		if (mVoiceFontMap.empty())
		{
			LL_DEBUGS("Voice") << "Voice fonts not available." << LL_ENDL;
			return false;
		}
		else if (mVoiceFontMap.find(id) == mVoiceFontMap.end())
		{
			LL_DEBUGS("Voice") << "Invalid voice font " << id << LL_ENDL;
			return false;
		}
	}
	mAudioSession->mVoiceFontID = id;
	gSavedPerAccountSettings.setString("VoiceEffectDefault", id.asString());
	sessionSetVoiceFontSendMessage(mAudioSession);
	notifyVoiceFontObservers();
	return true;
}
const LLUUID LLVivoxVoiceClient::getVoiceEffect()
{
	return mAudioSession ? mAudioSession->mVoiceFontID : LLUUID::null;
}
LLSD LLVivoxVoiceClient::getVoiceEffectProperties(const LLUUID& id)
{
	LLSD sd;
	voice_font_map_t::iterator iter = mVoiceFontMap.find(id);
	if (iter != mVoiceFontMap.end())
	{
		sd["template_only"] = false;
	}
	else
	{
		iter = mVoiceFontTemplateMap.find(id);
		if (iter == mVoiceFontTemplateMap.end())
		{
			LL_WARNS("Voice") << "Voice effect " << id << "not found." << LL_ENDL;
			return sd;
		}
		sd["template_only"] = true;
	}
	voiceFontEntry *font = iter->second;
	sd["name"] = font->mName;
	sd["expiry_date"] = font->mExpirationDate;
	sd["is_new"] = font->mIsNew;
	return sd;
}
LLVivoxVoiceClient::voiceFontEntry::voiceFontEntry(LLUUID& id) :
	mID(id),
	mFontIndex(0),
	mFontType(VOICE_FONT_TYPE_NONE),
	mFontStatus(VOICE_FONT_STATUS_NONE),
	mIsNew(false)
{
	mExpiryTimer.stop();
	mExpiryWarningTimer.stop();
}
LLVivoxVoiceClient::voiceFontEntry::~voiceFontEntry()
{
}
void LLVivoxVoiceClient::refreshVoiceEffectLists(bool clear_lists)
{
	if (clear_lists)
	{
		mVoiceFontsReceived = false;
		deleteAllVoiceFonts();
		deleteVoiceFontTemplates();
	}
	accountGetSessionFontsSendMessage();
	accountGetTemplateFontsSendMessage();
}
const voice_effect_list_t& LLVivoxVoiceClient::getVoiceEffectList() const
{
	return mVoiceFontList;
}
const voice_effect_list_t& LLVivoxVoiceClient::getVoiceEffectTemplateList() const
{
	return mVoiceFontTemplateList;
}
void LLVivoxVoiceClient::addVoiceFont(const S32 font_index,
								 const std::string &name,
								 const std::string &description,
								 const LLDate &expiration_date,
								 bool has_expired,
								 const S32 font_type,
								 const S32 font_status,
								 const bool template_font)
{
	LLUUID font_id;
	if (LLUUID::validate(name))
	{
		font_id = LLUUID(name);
	}
	else
	{
		font_id.generate(STRINGIZE(font_type << ":" << name));
	}
	voiceFontEntry *font = NULL;
	voice_font_map_t& font_map = template_font ? mVoiceFontTemplateMap : mVoiceFontMap;
	voice_effect_list_t& font_list = template_font ? mVoiceFontTemplateList : mVoiceFontList;
	voice_font_map_t::iterator iter = font_map.find(font_id);
	bool new_font = (iter == font_map.end());
	if (expiration_date.secondsSinceEpoch() < (LLDate::now().secondsSinceEpoch() + VOICE_FONT_EXPIRY_INTERVAL))
	{
		has_expired = true;
	}
	if (has_expired)
	{
		LL_DEBUGS("Voice") << "Expired " << (template_font ? "Template " : "")
		<< expiration_date.asString() << " " << font_id
		<< " (" << font_index << ") " << name << LL_ENDL;
		if (!new_font && !template_font)
		{
			deleteVoiceFont(font_id);
		}
		return;
	}
	if (new_font)
	{
		font = new voiceFontEntry(font_id);
	}
	else
	{
		font = iter->second;
	}
	if (font)
	{
		font->mFontIndex = font_index;
		font->mName = description.empty() ? name : description;
		font->mFontType = font_type;
		font->mFontStatus = font_status;
		if (!template_font && (new_font || font->mExpirationDate != expiration_date))
		{
			font->mExpirationDate = expiration_date;
			font->mExpiryTimer.start();
			font->mExpiryTimer.setExpiryAt(expiration_date.secondsSinceEpoch() - VOICE_FONT_EXPIRY_INTERVAL);
			S32 warning_time = gSavedSettings.getS32("VoiceEffectExpiryWarningTime");
			if (warning_time != 0)
			{
				font->mExpiryWarningTimer.start();
				F64 expiry_time = (expiration_date.secondsSinceEpoch() - (F64)warning_time);
				font->mExpiryWarningTimer.setExpiryAt(expiry_time - VOICE_FONT_EXPIRY_INTERVAL);
			}
			else
			{
				font->mExpiryWarningTimer.stop();
			}
			if (mVoiceFontsReceived)
			{
				font->mIsNew = true;
				mVoiceFontsNew = true;
			}
		}
		LL_DEBUGS("Voice") << (template_font ? "Template " : "")
			<< font->mExpirationDate.asString() << " " << font->mID
			<< " (" << font->mFontIndex << ") " << name << LL_ENDL;
		if (new_font)
		{
			font_map.insert(voice_font_map_t::value_type(font->mID, font));
			font_list.insert(voice_effect_list_t::value_type(font->mName, font->mID));
		}
		mVoiceFontListDirty = true;
		if (font_type < VOICE_FONT_TYPE_NONE || font_type >= VOICE_FONT_TYPE_UNKNOWN)
		{
			LL_DEBUGS("Voice") << "Unknown voice font type: " << font_type << LL_ENDL;
		}
		if (font_status < VOICE_FONT_STATUS_NONE || font_status >= VOICE_FONT_STATUS_UNKNOWN)
		{
			LL_DEBUGS("Voice") << "Unknown voice font status: " << font_status << LL_ENDL;
		}
	}
}
void LLVivoxVoiceClient::expireVoiceFonts()
{
	bool have_expired = false;
	bool will_expire = false;
	bool expired_in_use = false;
	LLUUID current_effect = LLVoiceClient::instance().getVoiceEffectDefault();
	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontMap.begin(); iter != mVoiceFontMap.end(); ++iter)
	{
		voiceFontEntry* voice_font = iter->second;
		LLFrameTimer& expiry_timer  = voice_font->mExpiryTimer;
		LLFrameTimer& warning_timer = voice_font->mExpiryWarningTimer;
		if (expiry_timer.getStarted() && expiry_timer.hasExpired())
		{
			if (voice_font->mID == current_effect)
			{
				setVoiceEffect(LLUUID::null);
				expired_in_use = true;
			}
			LL_DEBUGS("Voice") << "Voice Font " << voice_font->mName << " has expired." << LL_ENDL;
			deleteVoiceFont(voice_font->mID);
			have_expired = true;
		}
		if (warning_timer.getStarted() && warning_timer.hasExpired())
		{
			LL_DEBUGS("Voice") << "Voice Font " << voice_font->mName << " will expire soon." << LL_ENDL;
			will_expire = true;
			warning_timer.stop();
		}
	}
	LLSD args;
	args["URL"] = LLTrans::getString("voice_morphing_url");
	if (have_expired)
	{
		if (expired_in_use)
		{
			LLNotificationsUtil::add("VoiceEffectsExpiredInUse", args);
		}
		else
		{
			LLNotificationsUtil::add("VoiceEffectsExpired", args);
		}
		notifyVoiceFontObservers();
	}
	if (will_expire)
	{
		S32 seconds = gSavedSettings.getS32("VoiceEffectExpiryWarningTime");
		args["INTERVAL"] = llformat("%d", seconds / SEC_PER_DAY);
		LLNotificationsUtil::add("VoiceEffectsWillExpire", args);
	}
}
void LLVivoxVoiceClient::deleteVoiceFont(const LLUUID& id)
{
	voice_effect_list_t::iterator list_iter = mVoiceFontList.begin();
	while (list_iter != mVoiceFontList.end())
	{
		if (list_iter->second == id)
		{
			LL_DEBUGS("Voice") << "Removing " << id << " from the voice font list." << LL_ENDL;
			mVoiceFontList.erase(list_iter++);
			mVoiceFontListDirty = true;
		}
		else
		{
			++list_iter;
		}
	}
	voice_font_map_t::iterator map_iter = mVoiceFontMap.find(id);
	if (map_iter != mVoiceFontMap.end())
	{
		delete map_iter->second;
	}
	mVoiceFontMap.erase(map_iter);
}
void LLVivoxVoiceClient::deleteAllVoiceFonts()
{
	mVoiceFontList.clear();
	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontMap.begin(); iter != mVoiceFontMap.end(); ++iter)
	{
		delete iter->second;
	}
	mVoiceFontMap.clear();
}
void LLVivoxVoiceClient::deleteVoiceFontTemplates()
{
	mVoiceFontTemplateList.clear();
	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontTemplateMap.begin(); iter != mVoiceFontTemplateMap.end(); ++iter)
	{
		delete iter->second;
	}
	mVoiceFontTemplateMap.clear();
}
S32 LLVivoxVoiceClient::getVoiceFontIndex(const LLUUID& id) const
{
	S32 result = 0;
	if (!id.isNull())
	{
		voice_font_map_t::const_iterator it = mVoiceFontMap.find(id);
		if (it != mVoiceFontMap.end())
		{
			result = it->second->mFontIndex;
		}
		else
		{
			LL_DEBUGS("Voice") << "Selected voice font " << id << " is not available." << LL_ENDL;
		}
	}
	return result;
}
S32 LLVivoxVoiceClient::getVoiceFontTemplateIndex(const LLUUID& id) const
{
	S32 result = 0;
	if (!id.isNull())
	{
		voice_font_map_t::const_iterator it = mVoiceFontTemplateMap.find(id);
		if (it != mVoiceFontTemplateMap.end())
		{
			result = it->second->mFontIndex;
		}
		else
		{
			LL_DEBUGS("Voice") << "Selected voice font template " << id << " is not available." << LL_ENDL;
		}
	}
	return result;
}
void LLVivoxVoiceClient::accountGetSessionFontsSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		LL_DEBUGS("Voice") << "Requesting voice font list." << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetSessionFonts.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::accountGetTemplateFontsSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		LL_DEBUGS("Voice") << "Requesting voice font template list." << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetTemplateFonts.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::sessionSetVoiceFontSendMessage(sessionState *session)
{
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "Requesting voice font: " << session->mVoiceFontID << " (" << font_index << "), session handle: " << session->mHandle << LL_ENDL;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetVoiceFont.1\">"
	<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "<SessionFontID>" << font_index << "</SessionFontID>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}
void LLVivoxVoiceClient::accountGetSessionFontsResponse(int statusCode, const std::string &statusString)
{
	if(getState() == stateVoiceFontsWait)
	{
		setState(stateVoiceFontsReceived);
	}
	notifyVoiceFontObservers();
	mVoiceFontsReceived = true;
}
void LLVivoxVoiceClient::accountGetTemplateFontsResponse(int statusCode, const std::string &statusString)
{
	notifyVoiceFontObservers();
}
void LLVivoxVoiceClient::addObserver(LLVoiceEffectObserver* observer)
{
	mVoiceFontObservers.insert(observer);
}
void LLVivoxVoiceClient::removeObserver(LLVoiceEffectObserver* observer)
{
	mVoiceFontObservers.erase(observer);
}
void LLVivoxVoiceClient::notifyVoiceFontObservers()
{
	LL_DEBUGS("Voice") << "Notifying voice effect observers. Lists changed: " << mVoiceFontListDirty << LL_ENDL;
	for (voice_font_observer_set_t::iterator it = mVoiceFontObservers.begin();
		 it != mVoiceFontObservers.end();
		 )
	{
		LLVoiceEffectObserver* observer = *it;
		observer->onVoiceEffectChanged(mVoiceFontListDirty);
		it = mVoiceFontObservers.upper_bound(observer);
	}
	mVoiceFontListDirty = false;
	if (mVoiceFontsNew)
	{
		if(mVoiceFontsReceived)
		{
			LLNotificationsUtil::add("VoiceEffectsNew");
		}
		mVoiceFontsNew = false;
	}
}
void LLVivoxVoiceClient::enablePreviewBuffer(bool enable)
{
	mCaptureBufferMode = enable;
	if(mCaptureBufferMode && getState() >= stateNoChannel)
	{
		LL_DEBUGS("Voice") << "no channel" << LL_ENDL;
		sessionTerminate();
	}
}
void LLVivoxVoiceClient::recordPreviewBuffer()
{
	if (!mCaptureBufferMode)
	{
		LL_DEBUGS("Voice") << "Not in voice effect preview mode, cannot start recording." << LL_ENDL;
		mCaptureBufferRecording = false;
		return;
	}
	mCaptureBufferRecording = true;
}
void LLVivoxVoiceClient::playPreviewBuffer(const LLUUID& effect_id)
{
	if (!mCaptureBufferMode)
	{
		LL_DEBUGS("Voice") << "Not in voice effect preview mode, no buffer to play." << LL_ENDL;
		mCaptureBufferRecording = false;
		return;
	}
	if (!mCaptureBufferRecorded)
	{
		mCaptureBufferPlaying = false;
		return;
	}
	mPreviewVoiceFont = effect_id;
	mCaptureBufferPlaying = true;
}
void LLVivoxVoiceClient::stopPreviewBuffer()
{
	mCaptureBufferRecording = false;
	mCaptureBufferPlaying = false;
}
bool LLVivoxVoiceClient::isPreviewRecording()
{
	return (mCaptureBufferMode && mCaptureBufferRecording);
}
bool LLVivoxVoiceClient::isPreviewPlaying()
{
	return (mCaptureBufferMode && mCaptureBufferPlaying);
}
void LLVivoxVoiceClient::captureBufferRecordStartSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		LL_DEBUGS("Voice") << "Starting audio capture to buffer." << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.StartBufferCapture.1\">"
		<< "</Request>"
		<< "\n\n\n";
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>false</Value>"
		<< "</Request>\n\n\n";
		mMuteMicDirty = true;
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::captureBufferRecordStopSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		LL_DEBUGS("Voice") << "Stopping audio capture to buffer." << LL_ENDL;
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>true</Value>"
		<< "</Request>\n\n\n";
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::captureBufferPlayStartSendMessage(const LLUUID& voice_font_id)
{
	if(!mAccountHandle.empty())
	{
		++mPlayRequestCount;
		std::ostringstream stream;
		LL_DEBUGS("Voice") << "Starting audio buffer playback." << LL_ENDL;
		S32 font_index = getVoiceFontTemplateIndex(voice_font_id);
		LL_DEBUGS("Voice") << "With voice font: " << voice_font_id << " (" << font_index << ")" << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.PlayAudioBuffer.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
			<< "<TemplateFontID>" << font_index << "</TemplateFontID>"
			<< "<FontDelta />"
		<< "</Request>"
		<< "\n\n\n";
		writeString(stream.str());
	}
}
void LLVivoxVoiceClient::captureBufferPlayStopSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		LL_DEBUGS("Voice") << "Stopping audio buffer playback." << LL_ENDL;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";
		writeString(stream.str());
	}
}
LLVivoxProtocolParser::LLVivoxProtocolParser()
{
	parser = XML_ParserCreate(NULL);
	reset();
}
void LLVivoxProtocolParser::reset()
{
	responseDepth = 0;
	ignoringTags = false;
	accumulateText = false;
	energy = 0.f;
	hasText = false;
	hasAudio = false;
	hasVideo = false;
	terminated = false;
	ignoreDepth = 0;
	isChannel = false;
	incoming = false;
	enabled = false;
	isEvent = false;
	isLocallyMuted = false;
	isModeratorMuted = false;
	isSpeaking = false;
	participantType = 0;
	squelchDebugOutput = false;
	returnCode = -1;
	state = 0;
	statusCode = 0;
	volume = 0;
	textBuffer.clear();
	alias.clear();
	numberOfAliases = 0;
	applicationString.clear();
}
LLVivoxProtocolParser::~LLVivoxProtocolParser()
{
	if (parser)
		XML_ParserFree(parser);
}
static LLTrace::BlockTimerStatHandle FTM_VIVOX_PROCESS("Vivox Process");
LLIOPipe::EStatus LLVivoxProtocolParser::process_impl(
													  const LLChannelDescriptors& channels,
													  buffer_ptr_t& buffer,
													  bool& eos,
													  LLSD& context,
													  LLPumpIO* pump)
{
	LL_RECORD_BLOCK_TIME(FTM_VIVOX_PROCESS);
	LLBufferStream istr(channels, buffer.get());
	std::ostringstream ostr;
	while (istr.good())
	{
		char buf[1024];
		istr.read(buf, sizeof(buf));
		mInput.append(buf, istr.gcount());
	}
	int start = 0;
	int delim;
	while((delim = mInput.find("\n\n\n", start)) != std::string::npos)
	{
		reset();
		XML_ParserReset(parser, NULL);
		XML_SetElementHandler(parser, ExpatStartTag, ExpatEndTag);
		XML_SetCharacterDataHandler(parser, ExpatCharHandler);
		XML_SetUserData(parser, this);
		XML_Parse(parser, mInput.data() + start, delim - start, false);
		if(!squelchDebugOutput)
		{
			LL_DEBUGS("Voice") << "parsing: " << mInput.substr(start, delim - start) << LL_ENDL;
		}
		start = delim + 3;
	}
	if(start != 0)
		mInput = mInput.substr(start);
	LL_DEBUGS("VivoxProtocolParser") << "at end, mInput is: " << mInput << LL_ENDL;
	if(!LLVivoxVoiceClient::getInstance()->mConnected)
	{
		LL_INFOS("Voice") << "returning STATUS_STOP" << LL_ENDL;
		return STATUS_STOP;
	}
	return STATUS_OK;
}
void XMLCALL LLVivoxProtocolParser::ExpatStartTag(void *data, const char *el, const char **attr)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->StartTag(el, attr);
	}
}
void XMLCALL LLVivoxProtocolParser::ExpatEndTag(void *data, const char *el)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->EndTag(el);
	}
}
void XMLCALL LLVivoxProtocolParser::ExpatCharHandler(void *data, const XML_Char *s, int len)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->CharData(s, len);
	}
}
void LLVivoxProtocolParser::StartTag(const char *tag, const char **attr)
{
	textBuffer.clear();
	accumulateText = !ignoringTags;
	if (responseDepth == 0)
	{
		isEvent = !stricmp("Event", tag);
		if (!stricmp("Response", tag) || isEvent)
		{
			while (*attr)
			{
				const char	*key = *attr++;
				const char	*value = *attr++;
				if (!stricmp("requestId", key))
				{
					requestId = value;
				}
				else if (!stricmp("action", key))
				{
					actionString = value;
				}
				else if (!stricmp("type", key))
				{
					eventTypeString = value;
				}
			}
		}
		LL_DEBUGS("VivoxProtocolParser") << tag << " (" << responseDepth << ")"  << LL_ENDL;
	}
	else
	{
		if (ignoringTags)
		{
			LL_DEBUGS("VivoxProtocolParser") << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("VivoxProtocolParser") << tag << " (" << responseDepth << ")"  << LL_ENDL;
			if (!stricmp("InputXml", tag))
			{
				ignoringTags = true;
				ignoreDepth = responseDepth;
				accumulateText = false;
				LL_DEBUGS("VivoxProtocolParser") << "starting ignore, ignoreDepth is " << ignoreDepth << LL_ENDL;
			}
			else if (!stricmp("CaptureDevices", tag))
			{
				LLVivoxVoiceClient::getInstance()->clearCaptureDevices();
			}
			else if (!stricmp("RenderDevices", tag))
			{
				LLVivoxVoiceClient::getInstance()->clearRenderDevices();
			}
			else if (!stricmp("CaptureDevice", tag))
			{
				deviceString.clear();
			}
			else if (!stricmp("RenderDevice", tag))
			{
				deviceString.clear();
			}
			else if (!stricmp("SessionFont", tag))
			{
				id = 0;
				nameString.clear();
				descriptionString.clear();
				expirationDate = LLDate();
				hasExpired = false;
				fontType = 0;
				fontStatus = 0;
			}
			else if (!stricmp("TemplateFont", tag))
			{
				id = 0;
				nameString.clear();
				descriptionString.clear();
				expirationDate = LLDate();
				hasExpired = false;
				fontType = 0;
				fontStatus = 0;
			}
			else if (!stricmp("MediaCompletionType", tag))
			{
				mediaCompletionType.clear();
			}
		}
	}
	responseDepth++;
}
void LLVivoxProtocolParser::EndTag(const char *tag)
{
	const std::string& string = textBuffer;
	responseDepth--;
	if (ignoringTags)
	{
		if (ignoreDepth == responseDepth)
		{
			LL_DEBUGS("VivoxProtocolParser") << "end of ignore" << LL_ENDL;
			ignoringTags = false;
		}
		else
		{
			LL_DEBUGS("VivoxProtocolParser") << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		}
	}
	if (!ignoringTags)
	{
		LL_DEBUGS("VivoxProtocolParser") << "processing tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		if (!stricmp("ReturnCode", tag))
			returnCode = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("SessionHandle", tag))
			sessionHandle = string;
		else if (!stricmp("SessionGroupHandle", tag))
			sessionGroupHandle = string;
		else if (!stricmp("StatusCode", tag))
			statusCode = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("StatusString", tag))
			statusString = string;
		else if (!stricmp("ParticipantURI", tag))
			uriString = string;
		else if (!stricmp("Volume", tag))
			volume = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("Energy", tag))
			energy = (F32)strtod(string.c_str(), NULL);
		else if (!stricmp("IsModeratorMuted", tag))
			isModeratorMuted = !stricmp(string.c_str(), "true");
		else if (!stricmp("IsSpeaking", tag))
			isSpeaking = !stricmp(string.c_str(), "true");
		else if (!stricmp("Alias", tag))
			alias = string;
		else if (!stricmp("NumberOfAliases", tag))
			numberOfAliases = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("Application", tag))
			applicationString = string;
		else if (!stricmp("ConnectorHandle", tag))
			connectorHandle = string;
		else if (!stricmp("VersionID", tag))
			versionID = string;
		else if (!stricmp("AccountHandle", tag))
			accountHandle = string;
		else if (!stricmp("State", tag))
			state = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("URI", tag))
			uriString = string;
		else if (!stricmp("IsChannel", tag))
			isChannel = !stricmp(string.c_str(), "true");
		else if (!stricmp("Incoming", tag))
			incoming = !stricmp(string.c_str(), "true");
		else if (!stricmp("Enabled", tag))
			enabled = !stricmp(string.c_str(), "true");
		else if (!stricmp("Name", tag))
			nameString = string;
		else if (!stricmp("AudioMedia", tag))
			audioMediaString = string;
		else if (!stricmp("ChannelName", tag))
			nameString = string;
		else if (!stricmp("DisplayName", tag))
			displayNameString = string;
		else if (!stricmp("Device", tag))
			deviceString = string;
		else if (!stricmp("AccountName", tag))
			nameString = string;
		else if (!stricmp("ParticipantType", tag))
			participantType = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("IsLocallyMuted", tag))
			isLocallyMuted = !stricmp(string.c_str(), "true");
		else if (!stricmp("MicEnergy", tag))
			energy = (F32)strtod(string.c_str(), NULL);
		else if (!stricmp("ChannelName", tag))
			nameString = string;
		else if (!stricmp("ChannelURI", tag))
			uriString = string;
		else if (!stricmp("BuddyURI", tag))
			uriString = string;
		else if (!stricmp("Presence", tag))
			statusString = string;
		else if (!stricmp("CaptureDevice", tag))
		{
			LLVivoxVoiceClient::getInstance()->addCaptureDevice(deviceString);
		}
		else if (!stricmp("RenderDevice", tag))
		{
			LLVivoxVoiceClient::getInstance()->addRenderDevice(deviceString);
		}
		else if (!stricmp("BlockMask", tag))
			blockMask = string;
		else if (!stricmp("PresenceOnly", tag))
			presenceOnly = string;
		else if (!stricmp("AutoAcceptMask", tag))
			autoAcceptMask = string;
		else if (!stricmp("AutoAddAsBuddy", tag))
			autoAddAsBuddy = string;
		else if (!stricmp("MessageHeader", tag))
			messageHeader = string;
		else if (!stricmp("MessageBody", tag))
			messageBody = string;
		else if (!stricmp("NotificationType", tag))
			notificationType = string;
		else if (!stricmp("HasText", tag))
			hasText = !stricmp(string.c_str(), "true");
		else if (!stricmp("HasAudio", tag))
			hasAudio = !stricmp(string.c_str(), "true");
		else if (!stricmp("HasVideo", tag))
			hasVideo = !stricmp(string.c_str(), "true");
		else if (!stricmp("Terminated", tag))
			terminated = !stricmp(string.c_str(), "true");
		else if (!stricmp("SubscriptionHandle", tag))
			subscriptionHandle = string;
		else if (!stricmp("SubscriptionType", tag))
			subscriptionType = string;
		else if (!stricmp("SessionFont", tag))
		{
			LLVivoxVoiceClient::getInstance()->addVoiceFont(id, nameString, descriptionString, expirationDate, hasExpired, fontType, fontStatus, false);
		}
		else if (!stricmp("TemplateFont", tag))
		{
			LLVivoxVoiceClient::getInstance()->addVoiceFont(id, nameString, descriptionString, expirationDate, hasExpired, fontType, fontStatus, true);
		}
		else if (!stricmp("ID", tag))
		{
			id = strtol(string.c_str(), NULL, 10);
		}
		else if (!stricmp("Description", tag))
		{
			descriptionString = string;
		}
		else if (!stricmp("ExpirationDate", tag))
		{
			expirationDate = expiryTimeStampToLLDate(string);
		}
		else if (!stricmp("Expired", tag))
		{
			hasExpired = !stricmp(string.c_str(), "1");
		}
		else if (!stricmp("Type", tag))
		{
			fontType = strtol(string.c_str(), NULL, 10);
		}
		else if (!stricmp("Status", tag))
		{
			fontStatus = strtol(string.c_str(), NULL, 10);
		}
		else if (!stricmp("MediaCompletionType", tag))
		{
			mediaCompletionType = string;;
		}
		textBuffer.clear();
		accumulateText= false;
		if (responseDepth == 0)
		{
			processResponse(tag);
		}
	}
}
void LLVivoxProtocolParser::CharData(const char *buffer, int length)
{
	if (accumulateText)
		textBuffer.append(buffer, length);
}
LLDate LLVivoxProtocolParser::expiryTimeStampToLLDate(const std::string& vivox_ts)
{
	std::string time_stamp = vivox_ts.substr(0, 10);
	time_stamp += VOICE_FONT_EXPIRY_TIME;
	LL_DEBUGS("VivoxProtocolParser") << "Vivox timestamp " << vivox_ts << " modified to: " << time_stamp << LL_ENDL;
	return LLDate(time_stamp);
}
void LLVivoxProtocolParser::processResponse(std::string tag)
{
	LL_DEBUGS("VivoxProtocolParser") << tag << LL_ENDL;
	if(returnCode == 0)
		statusCode = 0;
	if (isEvent)
	{
		const char *eventTypeCstr = eventTypeString.c_str();
		if (!stricmp(eventTypeCstr, "AccountLoginStateChangeEvent"))
		{
			LLVivoxVoiceClient::getInstance()->accountLoginStateChangeEvent(accountHandle, statusCode, statusString, state);
		}
		else if (!stricmp(eventTypeCstr, "SessionAddedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->sessionAddedEvent(uriString, alias, sessionHandle, sessionGroupHandle, isChannel, incoming, nameString, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "SessionRemovedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->sessionRemovedEvent(sessionHandle, sessionGroupHandle);
		}
		else if (!stricmp(eventTypeCstr, "SessionGroupAddedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->sessionGroupAddedEvent(sessionGroupHandle);
		}
		else if (!stricmp(eventTypeCstr, "MediaStreamUpdatedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->mediaStreamUpdatedEvent(sessionHandle, sessionGroupHandle, statusCode, statusString, state, incoming);
		}
		else if (!stricmp(eventTypeCstr, "MediaCompletionEvent"))
		{
			LLVivoxVoiceClient::getInstance()->mediaCompletionEvent(sessionGroupHandle, mediaCompletionType);
		}
		else if (!stricmp(eventTypeCstr, "TextStreamUpdatedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->textStreamUpdatedEvent(sessionHandle, sessionGroupHandle, enabled, state, incoming);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantAddedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->participantAddedEvent(sessionHandle, sessionGroupHandle, uriString, alias, nameString, displayNameString, participantType);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantRemovedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->participantRemovedEvent(sessionHandle, sessionGroupHandle, uriString, alias, nameString);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantUpdatedEvent"))
		{
			squelchDebugOutput = true;
			LLVivoxVoiceClient::getInstance()->participantUpdatedEvent(sessionHandle, sessionGroupHandle, uriString, alias, isModeratorMuted, isSpeaking, volume, energy);
		}
		else if (!stricmp(eventTypeCstr, "AuxAudioPropertiesEvent"))
		{
			squelchDebugOutput = true;
			LLVivoxVoiceClient::getInstance()->auxAudioPropertiesEvent(energy);
		}
		else if (!stricmp(eventTypeCstr, "BuddyChangedEvent"))
		{
		}
		else if (!stricmp(eventTypeCstr, "MessageEvent"))
		{
			LLVivoxVoiceClient::getInstance()->messageEvent(sessionHandle, uriString, alias, messageHeader, messageBody, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "SessionNotificationEvent"))
		{
			LLVivoxVoiceClient::getInstance()->sessionNotificationEvent(sessionHandle, uriString, notificationType);
		}
		else if (!stricmp(eventTypeCstr, "SessionUpdatedEvent"))
		{
		}
		else if (!stricmp(eventTypeCstr, "SessionGroupRemovedEvent"))
		{
		}
		else if (!stricmp(eventTypeCstr, "VoiceServiceConnectionStateChangedEvent"))
		{
		}
		else
		{
			LL_WARNS("VivoxProtocolParser") << "Unknown event type " << eventTypeString << LL_ENDL;
		}
	}
	else
	{
		const char *actionCstr = actionString.c_str();
		if (!stricmp(actionCstr, "Connector.Create.1"))
		{
			LLVivoxVoiceClient::getInstance()->connectorCreateResponse(statusCode, statusString, connectorHandle, versionID);
		}
		else if (!stricmp(actionCstr, "Account.Login.1"))
		{
			LLVivoxVoiceClient::getInstance()->loginResponse(statusCode, statusString, accountHandle, numberOfAliases);
		}
		else if (!stricmp(actionCstr, "Session.Create.1"))
		{
			LLVivoxVoiceClient::getInstance()->sessionCreateResponse(requestId, statusCode, statusString, sessionHandle);
		}
		else if (!stricmp(actionCstr, "SessionGroup.AddSession.1"))
		{
			LLVivoxVoiceClient::getInstance()->sessionGroupAddSessionResponse(requestId, statusCode, statusString, sessionHandle);
		}
		else if (!stricmp(actionCstr, "Session.Connect.1"))
		{
			LLVivoxVoiceClient::getInstance()->sessionConnectResponse(requestId, statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Account.Logout.1"))
		{
			LLVivoxVoiceClient::getInstance()->logoutResponse(statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Connector.InitiateShutdown.1"))
		{
			LLVivoxVoiceClient::getInstance()->connectorShutdownResponse(statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Session.Set3DPosition.1"))
		{
			squelchDebugOutput = true;
		}
		else if (!stricmp(actionCstr, "Account.GetSessionFonts.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountGetSessionFontsResponse(statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Account.GetTemplateFonts.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountGetTemplateFontsResponse(statusCode, statusString);
		}
	}
}
