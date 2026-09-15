/**
 * @file shupdatechecker.cpp
 * @brief Check the official Allyn site for a newer viewer version
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "shupdatechecker.h"

#include <cctype>
#include <sstream>

#include "aihttpheaders.h"
#include "allynupdate.h"
#include "llbufferstream.h"
#include "llcontrol.h"
#include "llfloaterallynupdate.h"
#include "llhttpclient.h"
#include "llsdjson.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"

namespace
{
	const char* ALLYN_VERSION_URL = "https://allynviewer.discloud.app/api/version";
}

bool allyn_parse_version(const std::string& text, S32& major, S32& minor, S32& patch, S32& build)
{
	major = minor = patch = build = 0;
	if (text.empty())
	{
		return false;
	}

	const char* p = text.c_str();
	while (*p && !isdigit(static_cast<unsigned char>(*p)))
	{
		++p;
	}
	if (!*p)
	{
		return false;
	}

	int n = sscanf(p, "%d.%d.%d.%d", &major, &minor, &patch, &build);
	if (n == 3)
	{
		build = 0;
		return true;
	}
	return n == 4;
}

bool allyn_version_is_newer(S32 remote_major, S32 remote_minor, S32 remote_patch, S32 remote_build,
	S32 local_major, S32 local_minor, S32 local_patch, S32 local_build)
{
	if (remote_major != local_major) return remote_major > local_major;
	if (remote_minor != local_minor) return remote_minor > local_minor;
	if (remote_patch != local_patch) return remote_patch > local_patch;
	return remote_build > local_build;
}

void onCompleted(const LLSD& data)
{
	if (!data.isMap())
	{
		allyn_update_log("version json is not a map");
		return;
	}
	if (data.has("ok") && !data["ok"].asBoolean())
	{
		allyn_update_log("version json ok=false");
		return;
	}

	S32 remote_major = data.has("major") ? data["major"].asInteger() : 0;
	S32 remote_minor = data.has("minor") ? data["minor"].asInteger() : 0;
	S32 remote_patch = data.has("patch") ? data["patch"].asInteger() : 0;
	S32 remote_build = data.has("build") ? data["build"].asInteger() : 0;

	if (remote_major == 0 && remote_minor == 0 && remote_patch == 0 && remote_build == 0)
	{
		if (!allyn_parse_version(data["version"].asString(), remote_major, remote_minor, remote_patch, remote_build)
			&& !allyn_parse_version(data["label"].asString(), remote_major, remote_minor, remote_patch, remote_build))
		{
			allyn_update_log("Could not parse version from site");
			return;
		}
	}

	std::string label = data["label"].asString();
	if (label.empty())
	{
		label = data["version"].asString();
	}

	const std::string current = LLVersionInfo::getVersion();
	allyn_update_log("site version=" + label + " local=" + current);

	const bool newer = allyn_version_is_newer(remote_major, remote_minor, remote_patch, remote_build,
		LLVersionInfo::getMajor(), LLVersionInfo::getMinor(), LLVersionInfo::getPatch(), LLVersionInfo::getBuild());
	const bool force = gSavedSettings.getBOOL("AllynUpdateForcePrompt")
		|| gSavedSettings.getBOOL("AllynUpdateSimulateDownload");

	if (!newer && !force)
	{
		allyn_update_log("viewer is up to date, no prompt");
		return;
	}

	if (!newer && force)
	{
		allyn_update_log("forcing update prompt for test (AllynUpdateForcePrompt or AllynUpdateSimulateDownload)");
	}

	std::string installer_url = data["installerUrl"].asString();
	if (installer_url.empty() && allyn_update_is_simulate())
	{
		installer_url = "https://github.com/allynviewer/Allyn-Viewer/releases/download/v1.0.0.11/Allyn_Viewer_1_0_0_11_x86_64_Setup.exe";
		allyn_update_log("simulate using placeholder installer url");
	}

	std::string remote_key = llformat("%d.%d.%d.%d", remote_major, remote_minor, remote_patch, remote_build);
	static LLCachedControl<std::string> last_notified("AllynLastNotifiedVersion", "");
	if (last_notified.get() != remote_key)
	{
		last_notified = remote_key;
	}

	LLSD info;
	info["label"] = label;
	info["current"] = current;
	info["installerUrl"] = installer_url;
	LLFloaterAllynUpdate::offer(info);
}

extern AIHTTPTimeoutPolicy getUpdateInfoResponder_timeout;

class GetUpdateInfoResponder final : public LLHTTPClient::ResponderWithCompleted
{
	LOG_CLASS(GetUpdateInfoResponder);
public:
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer) override
	{
		if (mStatus != HTTP_OK)
		{
			allyn_update_log(llformat("Failed to get update info HTTP %d", mStatus));
			if (gSavedSettings.getBOOL("AllynUpdateForcePrompt")
				|| gSavedSettings.getBOOL("AllynUpdateSimulateDownload"))
			{
				LLSD info;
				info["label"] = "v0.0.0.0 Test";
				info["current"] = LLVersionInfo::getVersion();
				info["installerUrl"] = "https://github.com/allynviewer/Allyn-Viewer/releases/download/v1.0.0.11/Allyn_Viewer_1_0_0_11_x86_64_Setup.exe";
				allyn_update_log("forcing test prompt because version API failed");
				LLFloaterAllynUpdate::offer(info);
			}
			return;
		}
		LLBufferStream istr(channels, buffer.get());
		std::stringstream strstrm;
		strstrm << istr.rdbuf();
		LLSD data = LlsdFromJsonString(strstrm.str());
		if (data.isUndefined() || !data.isMap())
		{
			allyn_update_log("Failed to parse version json");
			return;
		}
		onCompleted(data);
	}
protected:
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy() const override { return getUpdateInfoResponder_timeout; }
	char const* getName() const override { return "GetUpdateInfoResponder"; }
};

void check_for_updates()
{
	allyn_update_run_self_tests();
	allyn_update_log(std::string("checking ") + ALLYN_VERSION_URL);
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "application/json");
	LLHTTPClient::get(ALLYN_VERSION_URL, new GetUpdateInfoResponder(), headers);
}
