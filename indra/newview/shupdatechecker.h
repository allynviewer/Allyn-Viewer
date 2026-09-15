/**
 * @file shupdatechecker.h
 * @brief Check the official Allyn site for a newer viewer version
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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
