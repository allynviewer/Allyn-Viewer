/** 
 * @file llpluginclassbasic.h
 * @brief LLPluginClassBasic handles interaction with a plugin which knows about the "basic" message class.
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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
#ifndef LL_LLPLUGINCLASSBASIC_H
#define LL_LLPLUGINCLASSBASIC_H
#include "llerror.h"
#include "stdtypes.h"
#include "llpluginprocessparent.h"
#include "llpluginclassmediaowner.h"
#include "llpluginmessage.h"
#include <string>
#include <queue>
class LLPluginClassBasic : public LLPluginProcessParentOwner
{
	LOG_CLASS(LLPluginClassBasic);
public:
	LLPluginClassBasic(void);
	virtual ~LLPluginClassBasic();
	bool init(std::string const& launcher_filename,
					  std::string const& plugin_dir,
					  std::string const& plugin_filename,
					  bool debug);
	void reset(void);
	void idle(void);
	void sendMessage(LLPluginMessage const& message);
	bool isPluginLoading(void) const { return mPlugin ? mPlugin->isLoading() : false; }
	bool isPluginRunning(void) const { return mPlugin ? mPlugin->isRunning() : false; }
	bool isPluginExited(void) const { return mPlugin ? mPlugin->isDone() : false; }
	std::string getPluginVersion() const { return mPlugin ? mPlugin->getPluginVersion() : std::string(""); }
	bool getDisableTimeout() const { return mPlugin ? mPlugin->getDisableTimeout() : false; }
	void setDisableTimeout(bool disable) { if (mPlugin) mPlugin->setDisableTimeout(disable); }
	enum EPriority
	{
		PRIORITY_SLEEP,
		PRIORITY_LOW,
		PRIORITY_NORMAL,
		PRIORITY_HIGH
	};
	static char const* priorityToString(EPriority priority);
	void setPriority(EPriority priority);
protected:
	EPriority mPriority;
	LLPluginProcessParent* mPlugin;
private:
	F64 mSleepTime;
	std::queue<LLPluginMessage> mSendQueue;
protected:
	virtual bool init_impl(void) { return true; }
	virtual void reset_impl(void) { }
	virtual void idle_impl(void) { }
	virtual void priorityChanged(EPriority priority) { }
	void receivePluginMessage(LLPluginMessage const&);
	void receivedShutdown() { mPlugin->exitState(); }
private:
	bool mDeleteOK;
public:
	void setDeleteOK(bool flag) { mDeleteOK = flag; }
};
#endif
