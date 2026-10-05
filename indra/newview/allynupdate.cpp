/**
 * @file allynupdate.cpp
 * @brief Download the official Allyn Viewer installer with progress logging
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "allynupdate.h"

#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

#include <curl/curl.h>

#include "llappviewer.h"
#include "lldir.h"
#include "llfile.h"
#include "llstring.h"
#include "shupdatechecker.h"

#if LL_WINDOWS
#include "llwin32headerslean.h"
#include <shellapi.h>
#endif

namespace
{
	std::mutex sErrorMutex;
	std::atomic<AllynUpdateDownloadState> sState{ALLYN_UPDATE_IDLE};
	std::atomic<int> sPercent{0};
	std::atomic<S64> sBytes{0};
	std::atomic<S64> sTotal{0};
	std::atomic<S64> sSpeedBps{0};
	std::atomic<bool> sCancel{false};
	std::string sError;
	std::string sDestPath;
	int sLastFileLogPercent = -1;
	S64 sSpeedSampleBytes = 0;
	S64 sSpeedEma = 0;
	std::chrono::steady_clock::time_point sSpeedSampleTime;
}

void allyn_update_log(const std::string& msg)
{
	LL_INFOS("AllynUpdate") << msg << LL_ENDL;
}

int allyn_update_percent(S64 downloaded, S64 total)
{
	if (total <= 0 || downloaded <= 0)
	{
		return 0;
	}
	if (downloaded >= total)
	{
		return 100;
	}
	return static_cast<int>((downloaded * 100) / total);
}

std::string allyn_update_format_bytes(S64 bytes)
{
	if (bytes < 0)
	{
		bytes = 0;
	}
	const double value = static_cast<double>(bytes);
	if (bytes < 1024)
	{
		return llformat("%d B", static_cast<int>(bytes));
	}
	if (bytes < 1024 * 1024)
	{
		return llformat("%.1f KB", value / 1024.0);
	}
	if (bytes < 1024LL * 1024 * 1024)
	{
		return llformat("%.1f MB", value / (1024.0 * 1024.0));
	}
	return llformat("%.2f GB", value / (1024.0 * 1024.0 * 1024.0));
}

std::string allyn_update_format_speed(S64 bytes_per_sec)
{
	if (bytes_per_sec < 0)
	{
		bytes_per_sec = 0;
	}
	return allyn_update_format_bytes(bytes_per_sec) + "/s";
}

bool allyn_update_url_allowed(const std::string& url)
{
	if (url.size() < 12)
	{
		return false;
	}
	std::string lower(url);
	LLStringUtil::toLower(lower);
	if (lower.compare(0, 8, "https://") != 0)
	{
		return false;
	}
	if (lower.find("..") != std::string::npos)
	{
		return false;
	}

	std::string rest = lower.substr(8);
	std::string host = rest.substr(0, rest.find('/'));
	const size_t colon = host.find(':');
	if (colon != std::string::npos)
	{
		host = host.substr(0, colon);
	}

	if (host == "github.com")
	{
		return rest.find("/allynviewer/allyn-viewer/") != std::string::npos;
	}
	return host == "objects.githubusercontent.com"
		|| host == "release-assets.githubusercontent.com"
		|| host == "github-releases.githubusercontent.com"
		|| host == "allynviewer.discloud.app";
}

std::string allyn_update_filename_from_url(const std::string& url)
{
	std::string path = url;
	const size_t query = path.find('?');
	if (query != std::string::npos)
	{
		path = path.substr(0, query);
	}
	const size_t slash = path.find_last_of("/\\");
	std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
	if (name.size() < 5)
	{
		return "AllynViewerSetup.exe";
	}
	std::string lower(name);
	LLStringUtil::toLower(lower);
	if (lower.size() < 4 || lower.substr(lower.size() - 4) != ".exe")
	{
		return "AllynViewerSetup.exe";
	}
	for (unsigned char c : name)
	{
		if (!std::isalnum(c) && c != '.' && c != '-' && c != '_')
		{
			return "AllynViewerSetup.exe";
		}
	}
	return name;
}

AllynUpdateDownloadState allyn_update_download_state()
{
	return sState.load();
}

int allyn_update_download_percent()
{
	return sPercent.load();
}

S64 allyn_update_download_bytes()
{
	return sBytes.load();
}

S64 allyn_update_download_total()
{
	return sTotal.load();
}

S64 allyn_update_download_speed_bps()
{
	return sSpeedBps.load();
}

std::string allyn_update_download_error()
{
	std::lock_guard<std::mutex> lock(sErrorMutex);
	return sError;
}

std::string allyn_update_download_path()
{
	std::lock_guard<std::mutex> lock(sErrorMutex);
	return sDestPath;
}

void allyn_update_cancel_download()
{
	sCancel.store(true);
	allyn_update_log("download cancel requested");
}

namespace
{
	void set_error(const std::string& error)
	{
		std::lock_guard<std::mutex> lock(sErrorMutex);
		sError = error;
	}

	void set_dest(const std::string& path)
	{
		std::lock_guard<std::mutex> lock(sErrorMutex);
		sDestPath = path;
	}

	void reset_speed_meter()
	{
		sSpeedBps.store(0);
		sSpeedSampleBytes = 0;
		sSpeedEma = 0;
		sSpeedSampleTime = std::chrono::steady_clock::now();
	}

	void update_speed_meter(S64 bytes)
	{
		const auto now = std::chrono::steady_clock::now();
		const double elapsed = std::chrono::duration<double>(now - sSpeedSampleTime).count();
		if (elapsed < 0.2)
		{
			return;
		}
		S64 delta = bytes - sSpeedSampleBytes;
		if (delta < 0)
		{
			delta = 0;
		}
		const S64 instant = static_cast<S64>(delta / elapsed);
		sSpeedEma = (sSpeedEma <= 0) ? instant : ((sSpeedEma * 7) + (instant * 3)) / 10;
		sSpeedBps.store(sSpeedEma);
		sSpeedSampleBytes = bytes;
		sSpeedSampleTime = now;
	}

	void apply_progress(S64 downloaded, S64 total)
	{
		if (total > 0)
		{
			sTotal.store(total);
		}
		if (downloaded < 0)
		{
			downloaded = 0;
		}
		sBytes.store(downloaded);
		sPercent.store(allyn_update_percent(downloaded, sTotal.load()));
		update_speed_meter(downloaded);
	}

	void log_percent_if_needed(int percent)
	{
		if (percent / 10 != sLastFileLogPercent / 10 || percent == 100 || percent == 0)
		{
			sLastFileLogPercent = percent;
			allyn_update_log(llformat("download progress %d%% (%lld / %lld bytes)",
				percent,
				static_cast<long long>(sBytes.load()),
				static_cast<long long>(sTotal.load())));
		}
	}

	size_t write_file(char* ptr, size_t size, size_t nmemb, void* userdata)
	{
		FILE* file = static_cast<FILE*>(userdata);
		if (!file)
		{
			return 0;
		}
		return fwrite(ptr, 1, size * nmemb, file);
	}

	size_t header_cb(char* buffer, size_t size, size_t nitems, void*)
	{
		const size_t length = size * nitems;
		if (!buffer || length == 0)
		{
			return length;
		}
		std::string line(buffer, length);
		LLStringUtil::toLower(line);
		const std::string prefix = "content-length:";
		if (line.compare(0, prefix.size(), prefix) == 0)
		{
			const S64 content_length = strtoll(line.c_str() + prefix.size(), NULL, 10);
			if (content_length > 0)
			{
				sTotal.store(content_length);
			}
		}
		return length;
	}

	int xfer_progress(void*, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
	{
		if (sCancel.load())
		{
			return 1;
		}
		apply_progress(static_cast<S64>(dlnow), static_cast<S64>(dltotal));
		log_percent_if_needed(sPercent.load());
		return 0;
	}

	int progress_legacy(void*, double dltotal, double dlnow, double, double)
	{
		return xfer_progress(nullptr, static_cast<curl_off_t>(dltotal), static_cast<curl_off_t>(dlnow), 0, 0);
	}

	void run_real_download(const std::string url, const std::string dest, const std::string ca_file)
	{
		allyn_update_log("real download start url=" + url);
		allyn_update_log("real download dest=" + dest);
		sState.store(ALLYN_UPDATE_DOWNLOADING);
		sPercent.store(0);
		sBytes.store(0);
		sLastFileLogPercent = -1;
		reset_speed_meter();

		LLFILE* file = LLFile::fopen(dest, "wb");
		if (!file)
		{
			set_error("Could not create destination file");
			sState.store(ALLYN_UPDATE_FAILED);
			allyn_update_log("FAIL open dest file");
			return;
		}

		CURL* curl = curl_easy_init();
		if (!curl)
		{
			fclose(file);
			set_error("Could not start download");
			sState.store(ALLYN_UPDATE_FAILED);
			allyn_update_log("FAIL curl_easy_init");
			return;
		}

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 8L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "AllynViewer-Update/1.0");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_legacy);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, nullptr);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, nullptr);
		curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 256L * 1024L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
		if (!ca_file.empty())
		{
			curl_easy_setopt(curl, CURLOPT_CAINFO, ca_file.c_str());
		}

		const CURLcode result = curl_easy_perform(curl);
		long http_status = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
		double downloaded = 0.0;
		curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);
		double content_length = 0.0;
		curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
		double avg_speed = 0.0;
		curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &avg_speed);
		if (content_length > 0.0)
		{
			sTotal.store(static_cast<S64>(content_length));
		}
		if (avg_speed > 0.0)
		{
			sSpeedBps.store(static_cast<S64>(avg_speed));
		}
		curl_easy_cleanup(curl);
		fclose(file);

		if (sCancel.load())
		{
			LLFile::remove(dest);
			sState.store(ALLYN_UPDATE_IDLE);
			allyn_update_log("real download cancelled");
			return;
		}

		if (result != CURLE_OK)
		{
			LLFile::remove(dest);
			set_error(curl_easy_strerror(result));
			sState.store(ALLYN_UPDATE_FAILED);
			allyn_update_log(llformat("FAIL curl %s http=%ld", curl_easy_strerror(result), http_status));
			return;
		}

		if (downloaded < 1024.0 * 1024.0)
		{
			LLFile::remove(dest);
			set_error("Installer file is too small");
			sState.store(ALLYN_UPDATE_FAILED);
			allyn_update_log(llformat("FAIL file too small (%lld bytes)", static_cast<long long>(downloaded)));
			return;
		}

		sBytes.store(static_cast<S64>(downloaded));
		sPercent.store(100);
		sState.store(ALLYN_UPDATE_DONE);
		allyn_update_log(llformat("real download complete bytes=%lld path=%s",
			static_cast<long long>(downloaded), dest.c_str()));
	}
}

void allyn_update_start_download(const std::string& url)
{
	if (sState.load() == ALLYN_UPDATE_DOWNLOADING)
	{
		allyn_update_log("download already running");
		return;
	}

	sCancel.store(false);
	set_error("");
	sPercent.store(0);
	sBytes.store(0);
	sTotal.store(0);
	sSpeedBps.store(0);
	sState.store(ALLYN_UPDATE_DOWNLOADING);

	if (!allyn_update_url_allowed(url))
	{
		set_error("Installer URL is not allowed");
		sState.store(ALLYN_UPDATE_FAILED);
		allyn_update_log("FAIL blocked installer url=" + url);
		return;
	}

	const std::string filename = allyn_update_filename_from_url(url);
	const std::string dest = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, filename);
	set_dest(dest);
	const std::string ca_file = gDirUtilp->getCAFile();
	allyn_update_log("using CA file " + ca_file);
	std::thread(run_real_download, url, dest, ca_file).detach();
}

bool allyn_update_launch_installer_and_quit()
{
	const std::string path = allyn_update_download_path();
	if (path.empty() || !LLFile::isfile(path))
	{
		allyn_update_log("FAIL installer file missing: " + path);
		return false;
	}

	allyn_update_log("launching installer " + path);
#if LL_WINDOWS
	const INT_PTR code = reinterpret_cast<INT_PTR>(ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL));
	if (code <= 32)
	{
		allyn_update_log(llformat("FAIL ShellExecuteA code=%d", static_cast<int>(code)));
		return false;
	}
	allyn_update_log("installer process started, requesting viewer quit");
	if (LLAppViewer::instance())
	{
		LLAppViewer::instance()->requestQuit();
	}
	return true;
#else
	allyn_update_log("FAIL installer launch is Windows-only");
	return false;
#endif
}
