/**
 * @file shupdatechecker.h
 * @brief Check the official Allyn site for a newer viewer version
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
 * Modifications Copyright (C) 2026 - Allyn Viewer
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
#ifndef SH_SHUPDATECHECKER_H
#define SH_SHUPDATECHECKER_H

#include "stdtypes.h"
#include <string>

void check_for_updates();

bool allyn_parse_version(const std::string& text, S32& major, S32& minor, S32& patch, S32& build);
bool allyn_version_is_newer(S32 remote_major, S32 remote_minor, S32 remote_patch, S32 remote_build,
	S32 local_major, S32 local_minor, S32 local_patch, S32 local_build);

#endif
