/** 
 * @file media_plugin_base.h
 * @brief Media plugin base class for LLMedia API plugin system
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 * 
 * @endcond
 */
#ifndef MEDIA_PLUGIN_BASE_H
#define MEDIA_PLUGIN_BASE_H
#include "basic_plugin_base.h"
class MediaPluginBase : public BasicPluginBase
{
public:
	MediaPluginBase(LLPluginInstance::sendMessageFunction send_message_function, LLPluginInstance* plugin_instance);
protected:
	typedef enum
	{
		STATUS_NONE,
		STATUS_LOADING,
		STATUS_LOADED,
		STATUS_ERROR,
		STATUS_PLAYING,
		STATUS_PAUSED,
		STATUS_DONE
	} EStatus;
	class SharedSegmentInfo
	{
	public:
		void *mAddress;
		size_t mSize;
	};
	void sendStatus();
	std::string statusString();
	void setStatus(EStatus status);
	virtual void setDirty(int left, int top, int right, int bottom);
	typedef std::map<std::string, SharedSegmentInfo> SharedSegmentMap;
	unsigned char* mPixels;
	std::string mTextureSegmentName;
	int mWidth;
	int mHeight;
	int mTextureWidth;
	int mTextureHeight;
	int mDepth;
	EStatus mStatus;
	SharedSegmentMap mSharedSegments;
};
#endif
