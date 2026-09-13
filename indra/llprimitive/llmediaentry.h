/** 
 * @file llmediaentry.h
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
#ifndef LL_LLMEDIAENTRY_H
#define LL_LLMEDIAENTRY_H
#include "llsd.h"
#include "llstring.h"
#include "lllslconstants.h"
class LLMediaEntry
{
public:
    enum MediaControls {
        STANDARD = 0,
        MINI
    };
    LLMediaEntry();
    LLMediaEntry(const LLMediaEntry &rhs);
    LLMediaEntry &operator=(const LLMediaEntry &rhs);
    virtual ~LLMediaEntry();
    bool operator==(const LLMediaEntry &rhs) const;
    bool operator!=(const LLMediaEntry &rhs) const;
    LLSD asLLSD() const;
    void asLLSD(LLSD& sd) const;
    operator LLSD() const { return asLLSD(); }
    static bool checkLLSD(const LLSD& sd);
    void fromLLSD(const LLSD& sd);
    void mergeFromLLSD(const LLSD& sd);
    bool getAltImageEnable() const { return mAltImageEnable; }
    MediaControls getControls() const { return mControls; }
    std::string getCurrentURL() const { return mCurrentURL; }
    std::string getHomeURL() const { return mHomeURL; }
    bool getAutoLoop() const { return mAutoLoop; }
    bool getAutoPlay() const { return mAutoPlay; }
    bool getAutoScale() const { return mAutoScale; }
    bool getAutoZoom() const { return mAutoZoom; }
    bool getFirstClickInteract() const { return mFirstClickInteract; }
    U16 getWidthPixels() const { return mWidthPixels; }
    U16 getHeightPixels() const { return mHeightPixels; }
    bool getWhiteListEnable() const { return mWhiteListEnable; }
    const std::vector<std::string> &getWhiteList() const { return mWhiteList; }
    U8 getPermsInteract() const { return mPermsInteract; }
    U8 getPermsControl() const { return mPermsControl; }
    U32 setAltImageEnable(bool alt_image_enable) { mAltImageEnable = alt_image_enable; return LSL_STATUS_OK; }
    U32 setControls(MediaControls controls);
    U32 setCurrentURL(const std::string& current_url);
    U32 setHomeURL(const std::string& home_url);
    U32 setAutoLoop(bool auto_loop) { mAutoLoop = auto_loop; return LSL_STATUS_OK; }
    U32 setAutoPlay(bool auto_play) { mAutoPlay = auto_play; return LSL_STATUS_OK; }
    U32 setAutoScale(bool auto_scale) { mAutoScale = auto_scale; return LSL_STATUS_OK; }
    U32 setAutoZoom(bool auto_zoom) { mAutoZoom = auto_zoom; return LSL_STATUS_OK; }
    U32 setFirstClickInteract(bool first_click) { mFirstClickInteract = first_click; return LSL_STATUS_OK; }
    U32 setWidthPixels(U16 width);
    U32 setHeightPixels(U16 height);
    U32 setWhiteListEnable( bool whitelist_enable ) { mWhiteListEnable = whitelist_enable; return LSL_STATUS_OK; }
    U32 setWhiteList( const std::vector<std::string> &whitelist );
    U32 setWhiteList( const LLSD &whitelist );
    U32 setPermsInteract( U8 val );
    U32 setPermsControl( U8 val );
    const LLUUID& getMediaID() const;
    bool checkCandidateUrl(const std::string& url) const;
public:
    static bool checkUrlAgainstWhitelist(const std::string &url,
                                         const std::vector<std::string> &whitelist);
public:
    static const char*  ALT_IMAGE_ENABLE_KEY;
    static const char*  CONTROLS_KEY;
    static const char*  CURRENT_URL_KEY;
    static const char*  HOME_URL_KEY;
    static const char*  AUTO_LOOP_KEY;
    static const char*  AUTO_PLAY_KEY;
    static const char*  AUTO_SCALE_KEY;
    static const char*  AUTO_ZOOM_KEY;
    static const char*  FIRST_CLICK_INTERACT_KEY;
    static const char*  WIDTH_PIXELS_KEY;
    static const char*  HEIGHT_PIXELS_KEY;
    static const char*  WHITELIST_ENABLE_KEY;
    static const char*  WHITELIST_KEY;
    static const char*  PERMS_INTERACT_KEY;
    static const char*  PERMS_CONTROL_KEY;
    enum Fields {
         ALT_IMAGE_ENABLE_ID = 0,
         CONTROLS_ID = 1,
         CURRENT_URL_ID = 2,
         HOME_URL_ID = 3,
         AUTO_LOOP_ID = 4,
         AUTO_PLAY_ID = 5,
         AUTO_SCALE_ID = 6,
         AUTO_ZOOM_ID = 7,
         FIRST_CLICK_INTERACT_ID = 8,
         WIDTH_PIXELS_ID = 9,
         HEIGHT_PIXELS_ID = 10,
         WHITELIST_ENABLE_ID = 11,
         WHITELIST_ID = 12,
         PERMS_INTERACT_ID = 13,
         PERMS_CONTROL_ID = 14,
         PARAM_MAX_ID = PERMS_CONTROL_ID
    };
    static const U8    PERM_NONE             = 0x0;
    static const U8    PERM_OWNER            = 0x1;
    static const U8    PERM_GROUP            = 0x2;
    static const U8    PERM_ANYONE           = 0x4;
    static const U8    PERM_ALL              = PERM_OWNER|PERM_GROUP|PERM_ANYONE;
    static const U8    PERM_MASK             = PERM_OWNER|PERM_GROUP|PERM_ANYONE;
    static const U32   MAX_URL_LENGTH        = 1024;
    static const U32   MAX_WHITELIST_SIZE    = 1024;
    static const U32   MAX_WHITELIST_COUNT   = 64;
    static const U16   MAX_WIDTH_PIXELS      = 2048;
    static const U16   MAX_HEIGHT_PIXELS     = 2048;
private:
    U32 setStringFieldWithLimit( std::string &field, const std::string &value, U32 limit );
    U32 setCurrentURLInternal( const std::string &url, bool check_whitelist);
    bool fromLLSDInternal(const LLSD &sd, bool overwrite);
private:
    bool mAltImageEnable;
    MediaControls mControls;
    std::string mCurrentURL;
    std::string mHomeURL;
    bool mAutoLoop;
    bool mAutoPlay;
    bool mAutoScale;
    bool mAutoZoom;
    bool mFirstClickInteract;
    U16 mWidthPixels;
    U16 mHeightPixels;
    bool mWhiteListEnable;
    std::vector<std::string> mWhiteList;
    U8 mPermsInteract;
    U8 mPermsControl;
    mutable LLUUID *mMediaIDp;
};
#endif
