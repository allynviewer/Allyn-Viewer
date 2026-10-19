/**
 * @file allynupdate.h
 * @brief Download the official Allyn Viewer installer with progress logging
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
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
 * $/LicenseInfo$
 */
#ifndef ALLYNUPDATE_H
#define ALLYNUPDATE_H

#include "stdtypes.h"
#include <string>

enum AllynUpdateDownloadState
{
	ALLYN_UPDATE_IDLE = 0,
	ALLYN_UPDATE_DOWNLOADING,
	ALLYN_UPDATE_DONE,
	ALLYN_UPDATE_FAILED
};

void allyn_update_log(const std::string& msg);

bool allyn_update_url_allowed(const std::string& url);
std::string allyn_update_filename_from_url(const std::string& url);
int allyn_update_percent(S64 downloaded, S64 total);

void allyn_update_start_download(const std::string& url);
void allyn_update_cancel_download();

AllynUpdateDownloadState allyn_update_download_state();
int allyn_update_download_percent();
S64 allyn_update_download_bytes();
S64 allyn_update_download_total();
S64 allyn_update_download_speed_bps();
std::string allyn_update_format_bytes(S64 bytes);
std::string allyn_update_format_speed(S64 bytes_per_sec);
std::string allyn_update_download_error();
std::string allyn_update_download_path();

bool allyn_update_launch_installer_and_quit();

#endif
