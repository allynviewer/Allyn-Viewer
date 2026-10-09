/** 
 * @file llnotify.h
 * @brief Non-blocking notification that doesn't take keyboard focus.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_LLNOTIFY_H
#define LL_LLNOTIFY_H
#include <vector>
#include "llfontgl.h"
#include "llpanel.h"
#include "lleventtimer.h"
#include "llnotifications.h"
#include "llstyle.h"
#include "lluuid.h"
class LLButton;
class LLNotifyBoxTemplate;
class LLScrollContainer;
class LLTextEditor;
class LLToastScrollArea
{
public:
	static const S32 TOP_RESERVE;
	static const S32 MIN_HEIGHT;
	bool isActive() const { return mScroller != nullptr; }
	S32 getNaturalWidth(const LLPanel* toast) const;
	void activate(LLPanel* toast, LLView* keep_on_toast, S32 content_height, LLButton* default_btn);
	void deactivate(LLPanel* toast, LLView* keep_on_toast);
	bool layout(LLPanel* toast, S32 content_height, S32 available_width, S32 available_height);
	void revealFocus();
private:
	struct SavedChild
	{
		LLHandle<LLView> mView;
		LLRect mRect;
		S32 mTabGroup = 0;
		S32 mOrdinal = 0;
		bool mIsCtrl = false;
	};
	void moveSavedChildren(LLView* from, LLView* to);
	std::vector<SavedChild> mSaved;
	LLScrollContainer* mScroller = nullptr;
	LLPanel* mContent = nullptr;
	S32 mNaturalWidth = 0;
	LLHandle<LLView> mLastFocus;
};
class LLNotifyBox final :
	public LLPanel,
	public LLEventTimer,
	public LLInitClass<LLNotifyBox>,
	public LLInstanceTracker<LLNotifyBox, LLUUID>
{
public:
	static void initClass();
	bool isTip() const { return mIsTip; }
	bool isCaution() const { return mIsCaution; }
	void setVisible(BOOL visible) override;
	void stopAnimation() { mAnimating = false; }
	void close();
	void sinkToWell();
	void unsinkFromWell();
	void hideToWell();
	bool isSunkToWell() const { return mSunkToWell; }
	bool isFriendPresenceTip() const;
	LLNotificationPtr getNotification() const { return mNotification; }
	static void format(std::string& msg, const LLStringUtil::format_map_t& args);
	static constexpr F32 MIN_TOAST_DURATION = 5.f;
	static constexpr F32 MAX_TOAST_DURATION = 600.f;
	static F32 getToastDuration();
	static bool isStartupSync();
	static bool isStartupSync(const LLNotificationPtr& notification);
	static bool archiveDuringStartup(const LLNotificationPtr& notification, LLPanel* box);
	static void flushStartupSummary();
	static void setOfflineMessagesPending(bool pending);
	bool fitTextToContent(bool force);
	void refreshMessage();
	class StartupSyncScope
	{
	public:
		explicit StartupSyncScope(bool active);
		~StartupSyncScope();
		StartupSyncScope(const StartupSyncScope&) = delete;
		StartupSyncScope& operator=(const StartupSyncScope&) = delete;
	private:
		bool mActive;
	};
protected:
	LLNotifyBox(LLNotificationPtr notification);
	~LLNotifyBox();
	LLButton* addButton(const std::string& name, const std::string& label, bool is_option, bool is_default, bool layout_script_dialog);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask) override;
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	BOOL handleHover(S32 x, S32 y, MASK mask) override;
	bool userIsInteracting() const;
	void draw() override;
	BOOL tick() override;
	void moveToBack(bool getfocus = false);
	static LLRect getNotifyRect(S32 num_options, bool layout_script_dialog, bool is_caution);
	static LLRect getNotifyTipRect(const std::string &message);
	static LLRect getFriendPresenceTipRect(const std::string &message);
	void onClickButton(const std::string name);
private:
	static bool onNotification(const LLSD& notify);
	void drawBackground() const;
	F32 fadeAlpha() const;
	bool fitScrollArea(S32 available, S32 available_width);
	void placeHideButton();
protected:
	LLTextEditor *mUserInputBox = nullptr, *mText = nullptr;
	LLNotificationPtr mNotification;
	bool mIsTip;
	bool mIsCaution;
	bool mAnimating;
	bool mSunkToWell;
	bool mFading;
	LLTimer mAnimateTimer;
	LLButton* mNextBtn;
	LLButton* mHideBtn;
	S32 mNumOptions;
	S32 mNumButtons;
	bool mAddedDefaultBtn;
	U32 mFittedTextRevision = 0;
	bool mTextFitted = false;
	LLToastScrollArea mScrollArea;
	LLStyleSP mTextStyle;
	S32 mNaturalWidth = 0;
};
class LLNotifyBoxView final : public LLUICtrl
{
public:
	LLNotifyBoxView(const std::string& name, const LLRect& rect, BOOL mouse_opaque, U32 follows=FOLLOWS_NONE);
	void showOnly(LLView* ctrl);
	LLNotifyBox* getFirstNontipBox() const;
	void deleteAllChildren() override;
	bool addChild(LLView* view, S32 tab_group = 0) override;
	void removeChild(LLView* view) override;
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE) override;
	void layoutStack();
	void draw() override;
	void bringToFront(const LLUUID& id);
	void closeAll();
	void closeOne(const LLUUID& id);
	S32 getOverflowCount() const;
	S32 getAvailableStackHeight() const;
	struct Matcher
	{
		Matcher(){}
		virtual ~Matcher() {}
		virtual bool matches(const LLNotificationPtr) const = 0;
	};
	void purgeMessagesMatching(const Matcher& matcher);
private:
	bool isGroupNotifyBox(const LLView* view) const;
	bool isStackable(LLView* view) const;
	bool isSunkToWell(LLView* view) const;
	bool belongsInWell(LLView* view) const;
	LLUUID getBoxID(LLView* view) const;
	std::string getBoxTitle(LLView* view) const;
	S32 getStackTop() const;
	S32 getStackBottom() const;
	void refreshWell();
	bool stackEnabled() const;
	bool mInDeleteAll = false;
	bool mInLayout = false;
	S32 mOverflowCount = 0;
	S32 mLastAvailableHeight = -1;
	S32 mLastAvailableWidth = -1;
	std::vector<LLHandle<LLView> > mSpaceHidden;
	bool takeSpaceHidden(LLView* view);
};
void drawAllynToastChrome(S32 width, S32 height, const LLColor4& fill, bool focused);
extern LLNotifyBoxView* gNotifyBoxView;
#endif
