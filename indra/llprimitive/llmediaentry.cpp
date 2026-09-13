/** 
 * @file llmediaentry.cpp
 * @brief This is a single instance of media data related to the face of a prim
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
#include "linden_common.h"
#include "llmediaentry.h"
#include "lllslconstants.h"
#include <boost/regex.hpp>
#define MEDIA_ALT_IMAGE_ENABLE_KEY_STR   "alt_image_enable"
#define MEDIA_CONTROLS_KEY_STR           "controls"
#define MEDIA_CURRENT_URL_KEY_STR        "current_url"
#define MEDIA_HOME_URL_KEY_STR           "home_url"
#define MEDIA_AUTO_LOOP_KEY_STR          "auto_loop"
#define MEDIA_AUTO_PLAY_KEY_STR          "auto_play"
#define MEDIA_AUTO_SCALE_KEY_STR         "auto_scale"
#define MEDIA_AUTO_ZOOM_KEY_STR          "auto_zoom"
#define MEDIA_FIRST_CLICK_INTERACT_KEY_STR  "first_click_interact"
#define MEDIA_WIDTH_PIXELS_KEY_STR       "width_pixels"
#define MEDIA_HEIGHT_PIXELS_KEY_STR      "height_pixels"
#define MEDIA_WHITELIST_ENABLE_KEY_STR   "whitelist_enable"
#define MEDIA_WHITELIST_KEY_STR          "whitelist"
#define MEDIA_PERMS_INTERACT_KEY_STR     "perms_interact"
#define MEDIA_PERMS_CONTROL_KEY_STR      "perms_control"
const char* LLMediaEntry::ALT_IMAGE_ENABLE_KEY  = MEDIA_ALT_IMAGE_ENABLE_KEY_STR;
const char* LLMediaEntry::CONTROLS_KEY          = MEDIA_CONTROLS_KEY_STR;
const char* LLMediaEntry::CURRENT_URL_KEY       = MEDIA_CURRENT_URL_KEY_STR;
const char* LLMediaEntry::HOME_URL_KEY          = MEDIA_HOME_URL_KEY_STR;
const char* LLMediaEntry::AUTO_LOOP_KEY         = MEDIA_AUTO_LOOP_KEY_STR;
const char* LLMediaEntry::AUTO_PLAY_KEY         = MEDIA_AUTO_PLAY_KEY_STR;
const char* LLMediaEntry::AUTO_SCALE_KEY        = MEDIA_AUTO_SCALE_KEY_STR;
const char* LLMediaEntry::AUTO_ZOOM_KEY         = MEDIA_AUTO_ZOOM_KEY_STR;
const char* LLMediaEntry::FIRST_CLICK_INTERACT_KEY = MEDIA_FIRST_CLICK_INTERACT_KEY_STR;
const char* LLMediaEntry::WIDTH_PIXELS_KEY      = MEDIA_WIDTH_PIXELS_KEY_STR;
const char* LLMediaEntry::HEIGHT_PIXELS_KEY     = MEDIA_HEIGHT_PIXELS_KEY_STR;
const char* LLMediaEntry::WHITELIST_ENABLE_KEY  = MEDIA_WHITELIST_ENABLE_KEY_STR;
const char* LLMediaEntry::WHITELIST_KEY         = MEDIA_WHITELIST_KEY_STR;
const char* LLMediaEntry::PERMS_INTERACT_KEY    = MEDIA_PERMS_INTERACT_KEY_STR;
const char* LLMediaEntry::PERMS_CONTROL_KEY     = MEDIA_PERMS_CONTROL_KEY_STR;
#define DEFAULT_URL_PREFIX  "http://"
LLMediaEntry::LLMediaEntry() :
    mAltImageEnable(false),
    mControls(STANDARD),
    mCurrentURL(""),
    mHomeURL(""),
    mAutoLoop(false),
    mAutoPlay(false),
    mAutoScale(false),
    mAutoZoom(false),
    mFirstClickInteract(false),
    mWidthPixels(0),
    mHeightPixels(0),
    mWhiteListEnable(false),
    mPermsInteract(PERM_ALL),
    mPermsControl(PERM_ALL),
    mMediaIDp(NULL)
{
}
LLMediaEntry::LLMediaEntry(const LLMediaEntry &rhs) :
    mMediaIDp(NULL)
{
    mAltImageEnable = rhs.mAltImageEnable;
    mControls = rhs.mControls;
    mCurrentURL = rhs.mCurrentURL;
    mHomeURL = rhs.mHomeURL;
    mAutoLoop = rhs.mAutoLoop;
    mAutoPlay = rhs.mAutoPlay;
    mAutoScale = rhs.mAutoScale;
    mAutoZoom = rhs.mAutoZoom;
    mFirstClickInteract = rhs.mFirstClickInteract;
    mWidthPixels = rhs.mWidthPixels;
    mHeightPixels = rhs.mHeightPixels;
    mWhiteListEnable = rhs.mWhiteListEnable;
    mWhiteList = rhs.mWhiteList;
    mPermsInteract = rhs.mPermsInteract;
    mPermsControl = rhs.mPermsControl;
}
LLMediaEntry::~LLMediaEntry()
{
    if (NULL != mMediaIDp)
    {
        delete mMediaIDp;
    }
}
LLSD LLMediaEntry::asLLSD() const
{
    LLSD sd;
    asLLSD(sd);
    return sd;
}
void LLMediaEntry::asLLSD(LLSD& sd) const
{
    sd[ALT_IMAGE_ENABLE_KEY] = mAltImageEnable;
    sd[CONTROLS_KEY] = (LLSD::Integer)mControls;
    sd[CURRENT_URL_KEY] = mCurrentURL;
    sd[HOME_URL_KEY] = mHomeURL;
    sd[AUTO_LOOP_KEY] = mAutoLoop;
    sd[AUTO_PLAY_KEY] = mAutoPlay;
    sd[AUTO_SCALE_KEY] = mAutoScale;
    sd[AUTO_ZOOM_KEY] = mAutoZoom;
    sd[FIRST_CLICK_INTERACT_KEY] = mFirstClickInteract;
    sd[WIDTH_PIXELS_KEY] = mWidthPixels;
    sd[HEIGHT_PIXELS_KEY] = mHeightPixels;
    sd[WHITELIST_ENABLE_KEY] = mWhiteListEnable;
	sd.erase(WHITELIST_KEY);
    for (U32 i=0; i<mWhiteList.size(); i++)
	{
        sd[WHITELIST_KEY].append(mWhiteList[i]);
    }
    sd[PERMS_INTERACT_KEY] = mPermsInteract;
    sd[PERMS_CONTROL_KEY] = mPermsControl;
}
bool LLMediaEntry::checkLLSD(const LLSD& sd)
{
    if (sd.isUndefined()) return true;
    LLMediaEntry temp;
    return temp.fromLLSDInternal(sd, true);
}
void LLMediaEntry::fromLLSD(const LLSD& sd)
{
    (void)fromLLSDInternal(sd, true);
}
void LLMediaEntry::mergeFromLLSD(const LLSD& sd)
{
    (void)fromLLSDInternal(sd, false);
}
bool LLMediaEntry::fromLLSDInternal(const LLSD& sd, bool overwrite)
{
    U32 status = LSL_STATUS_OK;
    if ( overwrite || sd.has(ALT_IMAGE_ENABLE_KEY) )
    {
        status |= setAltImageEnable( sd[ALT_IMAGE_ENABLE_KEY] );
    }
    if ( overwrite || sd.has(CONTROLS_KEY) )
    {
        status |= setControls( (MediaControls)(LLSD::Integer)sd[CONTROLS_KEY] );
    }
    if ( overwrite || sd.has(CURRENT_URL_KEY) )
    {
        status |= setCurrentURLInternal( sd[CURRENT_URL_KEY], false );
    }
    if ( overwrite || sd.has(HOME_URL_KEY) )
    {
        status |= setHomeURL( sd[HOME_URL_KEY] );
    }
    if ( overwrite || sd.has(AUTO_LOOP_KEY) )
    {
        status |= setAutoLoop( sd[AUTO_LOOP_KEY] );
    }
    if ( overwrite || sd.has(AUTO_PLAY_KEY) )
    {
        status |= setAutoPlay( sd[AUTO_PLAY_KEY] );
    }
    if ( overwrite || sd.has(AUTO_SCALE_KEY) )
    {
        status |= setAutoScale( sd[AUTO_SCALE_KEY] );
    }
    if ( overwrite || sd.has(AUTO_ZOOM_KEY) )
    {
        status |= setAutoZoom( sd[AUTO_ZOOM_KEY] );
    }
    if ( overwrite || sd.has(FIRST_CLICK_INTERACT_KEY) )
    {
        status |= setFirstClickInteract( sd[FIRST_CLICK_INTERACT_KEY] );
    }
    if ( overwrite || sd.has(WIDTH_PIXELS_KEY) )
    {
        status |= setWidthPixels( (LLSD::Integer)sd[WIDTH_PIXELS_KEY] );
    }
    if ( overwrite || sd.has(HEIGHT_PIXELS_KEY) )
    {
        status |= setHeightPixels( (LLSD::Integer)sd[HEIGHT_PIXELS_KEY] );
    }
    if ( overwrite || sd.has(WHITELIST_ENABLE_KEY) )
    {
        status |= setWhiteListEnable( sd[WHITELIST_ENABLE_KEY] );
    }
    if ( overwrite || sd.has(WHITELIST_KEY) )
    {
        status |= setWhiteList( sd[WHITELIST_KEY] );
    }
    if ( overwrite || sd.has(PERMS_INTERACT_KEY) )
    {
        status |= setPermsInteract( 0xff & (LLSD::Integer)sd[PERMS_INTERACT_KEY] );
    }
    if ( overwrite || sd.has(PERMS_CONTROL_KEY) )
    {
        status |= setPermsControl( 0xff & (LLSD::Integer)sd[PERMS_CONTROL_KEY] );
    }
    return LSL_STATUS_OK == status;
}
LLMediaEntry& LLMediaEntry::operator=(const LLMediaEntry &rhs)
{
    if (this != &rhs)
    {
        mAltImageEnable = rhs.mAltImageEnable;
        mControls = rhs.mControls;
        mCurrentURL = rhs.mCurrentURL;
        mHomeURL = rhs.mHomeURL;
        mAutoLoop = rhs.mAutoLoop;
        mAutoPlay = rhs.mAutoPlay;
        mAutoScale = rhs.mAutoScale;
        mAutoZoom = rhs.mAutoZoom;
        mFirstClickInteract = rhs.mFirstClickInteract;
        mWidthPixels = rhs.mWidthPixels;
        mHeightPixels = rhs.mHeightPixels;
        mWhiteListEnable = rhs.mWhiteListEnable;
        mWhiteList = rhs.mWhiteList;
        mPermsInteract = rhs.mPermsInteract;
        mPermsControl = rhs.mPermsControl;
    }
    return *this;
}
bool LLMediaEntry::operator==(const LLMediaEntry &rhs) const
{
    return (
        mAltImageEnable == rhs.mAltImageEnable &&
        mControls == rhs.mControls &&
        mCurrentURL == rhs.mCurrentURL &&
        mHomeURL == rhs.mHomeURL &&
        mAutoLoop == rhs.mAutoLoop &&
        mAutoPlay == rhs.mAutoPlay &&
        mAutoScale == rhs.mAutoScale &&
        mAutoZoom == rhs.mAutoZoom &&
        mFirstClickInteract == rhs.mFirstClickInteract &&
        mWidthPixels == rhs.mWidthPixels &&
        mHeightPixels == rhs.mHeightPixels &&
        mWhiteListEnable == rhs.mWhiteListEnable &&
        mWhiteList == rhs.mWhiteList &&
        mPermsInteract == rhs.mPermsInteract &&
        mPermsControl == rhs.mPermsControl
        );
}
bool LLMediaEntry::operator!=(const LLMediaEntry &rhs) const
{
    return (
        mAltImageEnable != rhs.mAltImageEnable ||
        mControls != rhs.mControls ||
        mCurrentURL != rhs.mCurrentURL ||
        mHomeURL != rhs.mHomeURL ||
        mAutoLoop != rhs.mAutoLoop ||
        mAutoPlay != rhs.mAutoPlay ||
        mAutoScale != rhs.mAutoScale ||
        mAutoZoom != rhs.mAutoZoom ||
        mFirstClickInteract != rhs.mFirstClickInteract ||
        mWidthPixels != rhs.mWidthPixels ||
        mHeightPixels != rhs.mHeightPixels ||
        mWhiteListEnable != rhs.mWhiteListEnable ||
        mWhiteList != rhs.mWhiteList ||
        mPermsInteract != rhs.mPermsInteract ||
        mPermsControl != rhs.mPermsControl
        );
}
U32 LLMediaEntry::setWhiteList( const std::vector<std::string> &whitelist )
{
    U32 size = 0;
    U32 count = 0;
    std::vector<std::string>::const_iterator iter = whitelist.begin();
    std::vector<std::string>::const_iterator end = whitelist.end();
    for ( ; iter < end; ++iter)
    {
        const std::string &entry = (*iter);
        size += entry.length() + 1;
        count ++;
        if (size > MAX_WHITELIST_SIZE || count > MAX_WHITELIST_COUNT)
		{
            return LSL_STATUS_BOUNDS_ERROR;
        }
    }
    mWhiteList.clear();
    iter = whitelist.begin();
    for ( ; iter < end; ++iter)
    {
        const std::string &entry = (*iter);
        mWhiteList.push_back(entry);
    }
    return LSL_STATUS_OK;
}
U32 LLMediaEntry::setWhiteList( const LLSD &whitelist )
{
    if (whitelist.isUndefined())
	{
		mWhiteList.clear();
		return LSL_STATUS_OK;
	}
    if (whitelist.isArray())
    {
        U32 size = 0;
        U32 count = 0;
        LLSD::array_const_iterator iter = whitelist.beginArray();
        LLSD::array_const_iterator end = whitelist.endArray();
        for ( ; iter < end; ++iter)
        {
            const std::string &entry = (*iter).asString();
            size += entry.length() + 1;
            count ++;
            if (size > MAX_WHITELIST_SIZE || count > MAX_WHITELIST_COUNT)
			{
                return LSL_STATUS_BOUNDS_ERROR;
            }
        }
        mWhiteList.clear();
        iter = whitelist.beginArray();
        for ( ; iter < end; ++iter)
        {
            const std::string &entry = (*iter).asString();
            mWhiteList.push_back(entry);
        }
        return LSL_STATUS_OK;
    }
    else
	{
        return LSL_STATUS_MALFORMED_PARAMS;
    }
}
static void prefix_with(std::string &str, const char *chars, const char *prefix)
{
    size_t found = str.find_first_of(chars);
    size_t prefix_len = strlen(prefix);
    while (found != std::string::npos)
    {
        str.insert(found, prefix, prefix_len);
        found = str.find_first_of(chars, found+prefix_len+1);
    }
}
static bool pattern_match(const std::string &candidate_str, const std::string &pattern)
{
    if (pattern.empty()) return true;
    std::string expression = pattern;
    prefix_with(expression, ".[{()\\+?|^$", "\\");
    prefix_with(expression, "*", ".");
    boost::regex regexp(expression, boost::regex::perl|boost::regex::icase);
    return boost::regex_match(candidate_str, regexp);
}
bool LLMediaEntry::checkCandidateUrl(const std::string& url) const
{
    if (getWhiteListEnable())
    {
        return checkUrlAgainstWhitelist(url, getWhiteList());
    }
    else
	{
        return true;
    }
}
bool LLMediaEntry::checkUrlAgainstWhitelist(const std::string& url,
                                            const std::vector<std::string> &whitelist)
{
    bool passes = true;
    if (whitelist.size() > 0)
    {
        passes = false;
        std::string candidate_url = url;
        LLURI candidate_uri(candidate_url);
        std::vector<std::string>::const_iterator iter = whitelist.begin();
        std::vector<std::string>::const_iterator end = whitelist.end();
        for ( ; iter < end; ++iter )
        {
            std::string filter = *iter;
            LLURI filter_uri(filter);
            bool scheme_passes = pattern_match( candidate_uri.scheme(), filter_uri.scheme() );
            if (filter_uri.scheme().empty())
            {
                filter_uri = LLURI(DEFAULT_URL_PREFIX + filter);
            }
            bool authority_passes = pattern_match( candidate_uri.authority(), filter_uri.authority() );
            bool path_passes = pattern_match( candidate_uri.escapedPath(), filter_uri.escapedPath() );
            if (scheme_passes && authority_passes && path_passes)
            {
                passes = true;
                break;
            }
        }
    }
    return passes;
}
U32 LLMediaEntry::setStringFieldWithLimit( std::string &field, const std::string &value, U32 limit )
{
    if ( value.length() > limit )
	{
        return LSL_STATUS_BOUNDS_ERROR;
    }
    else
	{
        field = value;
        return LSL_STATUS_OK;
    }
}
U32 LLMediaEntry::setControls(LLMediaEntry::MediaControls controls)
{
    if (controls == STANDARD ||
        controls == MINI)
    {
        mControls = controls;
        return LSL_STATUS_OK;
    }
    return LSL_STATUS_BOUNDS_ERROR;
}
U32 LLMediaEntry::setPermsInteract( U8 val )
{
    mPermsInteract = val & PERM_MASK;
    return LSL_STATUS_OK;
}
U32 LLMediaEntry::setPermsControl( U8 val )
{
    mPermsControl = val & PERM_MASK;
    return LSL_STATUS_OK;
}
U32 LLMediaEntry::setCurrentURL(const std::string& current_url)
{
    return setCurrentURLInternal( current_url, true );
}
U32 LLMediaEntry::setCurrentURLInternal(const std::string& current_url, bool check_whitelist)
{
    if ( ! check_whitelist || checkCandidateUrl(current_url))
    {
        return setStringFieldWithLimit( mCurrentURL, current_url, MAX_URL_LENGTH );
    }
    else
	{
        return LSL_STATUS_WHITELIST_FAILED;
    }
}
U32 LLMediaEntry::setHomeURL(const std::string& home_url)
{
    return setStringFieldWithLimit( mHomeURL, home_url, MAX_URL_LENGTH );
}
U32 LLMediaEntry::setWidthPixels(U16 width)
{
    if (width > MAX_WIDTH_PIXELS) return LSL_STATUS_BOUNDS_ERROR;
    mWidthPixels = width;
    return LSL_STATUS_OK;
}
U32 LLMediaEntry::setHeightPixels(U16 height)
{
    if (height > MAX_HEIGHT_PIXELS) return LSL_STATUS_BOUNDS_ERROR;
    mHeightPixels = height;
    return LSL_STATUS_OK;
}
const LLUUID &LLMediaEntry::getMediaID() const
{
    if (NULL == mMediaIDp)
    {
        mMediaIDp = new LLUUID();
        mMediaIDp->generate();
    }
    return *mMediaIDp;
}
