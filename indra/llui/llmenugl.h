/** 
 * @file llmenugl.h
 * @brief Declaration of the opengl based menu system.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#ifndef LL_LLMENUGL_H
#define LL_LLMENUGL_H
#include <list>
#include "llstring.h"
#include "v4color.h"
#include "llframetimer.h"
#include "llevent.h"
#include "llkeyboard.h"
#include "llfloater.h"
#include "lluistring.h"
#include "llview.h"
extern S32 MENU_BAR_HEIGHT;
typedef void (*menu_callback)(void*);
typedef void (*on_disabled_callback)(void*);
typedef BOOL (*enabled_callback)(void*);
typedef BOOL (*check_callback)(void*);
typedef void (*label_callback)(std::string&,void*);
class LLMenuItemGL : public LLUICtrl
{
public:
	LLMenuItemGL( const std::string& name, const std::string& label, KEY key = KEY_NONE, MASK = MASK_NONE );
	virtual ~LLMenuItemGL();
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	virtual std::string getType() const	{ return "item"; }
	void handleVisibilityChange(BOOL new_visibility);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleRightMouseUp(S32 x, S32 y, MASK mask);
	void setValue(const LLSD& value);
	LLSD getValue() const;
	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);
	LLColor4 getHighlightBgColor() { return mHighlightBackground; }
	void setJumpKey(KEY key);
	KEY getJumpKey() const { return mJumpKey; }
	void setFont(const LLFontGL* font) { mFont = font; }
	const LLFontGL* getFont() const { return mFont; }
	void setFontStyle(U8 style) { mStyle = style; }
	U8 getFontStyle() const { return mStyle; }
	virtual U32 getNominalHeight( void ) const;
	virtual void setBriefItem(BOOL brief);
	virtual BOOL isBriefItem() const;
	virtual BOOL addToAcceleratorList(std::list<LLKeyBinding*> *listp);
	void setAllowKeyRepeat(BOOL allow) { mAllowKeyRepeat = allow; }
	BOOL getAllowKeyRepeat() const { return mAllowKeyRepeat; }
	void setLabel( const LLStringExplicit& label ) { mLabel = label; }
	std::string getLabel( void ) const { return mLabel.getString(); }
	virtual BOOL setLabelArg( const std::string& key, const LLStringExplicit& text );
	virtual class LLMenuGL*	getMenu() const;
	virtual U32 getNominalWidth( void ) const;
	virtual void buildDrawLabel( void );
	virtual void updateBranchParent( LLView* parentp ){};
	virtual void onCommit( void );
	virtual void setHighlight( BOOL highlight );
	virtual BOOL getHighlight() const { return mHighlight; }
	virtual BOOL isActive( void ) const { return FALSE; }
	virtual BOOL isOpen( void ) const { return FALSE; }
	virtual void setEnabledSubMenus(BOOL enable){};
	virtual BOOL handleKeyHere( KEY key, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL handleScrollWheel( S32 x, S32 y, S32 clicks );
	virtual void draw( void );
	BOOL getHover() const { return mGotHover; }
	void setDrawTextDisabled(BOOL disabled) { mDrawTextDisabled = disabled; }
	BOOL getDrawTextDisabled() const { return mDrawTextDisabled; }
protected:
	void setHover(BOOL hover) { mGotHover = hover; }
	void appendAcceleratorString( std::string& st ) const;
protected:
	KEY mAcceleratorKey;
	MASK mAcceleratorMask;
	LLUIString mLabel;
	LLUIString mDrawBoolLabel;
	LLUIString mDrawAccelLabel;
	LLUIString mDrawBranchLabel;
	LLColor4 mEnabledColor;
	LLColor4 mDisabledColor;
	LLColor4 mHighlightBackground;
	LLColor4 mHighlightForeground;
	BOOL mHighlight;
public:
	static LLColor4 sEnabledColor;
	static LLColor4 sDisabledColor;
	static LLColor4 sHighlightBackground;
	static LLColor4 sHighlightForeground;
private:
	BOOL mAllowKeyRepeat;
	BOOL mGotHover;
	BOOL mBriefItem;
	const LLFontGL* mFont;
	U8	mStyle;
	BOOL mDrawTextDisabled;
	KEY mJumpKey;
};
class LLMenuItemSeparatorGL : public LLMenuItemGL
{
public:
	LLMenuItemSeparatorGL( const std::string &name = std::string() );
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	virtual std::string getType() const	{ return "separator"; }
	virtual void onCommit( void ) {}
	void draw( void );
	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	U32 getNominalHeight( void ) const;
};
class LLMenuItemCallGL : public LLMenuItemGL, public LLOldEvents::LLObservable
{
public:
	LLMenuItemCallGL( const std::string& name,
 					  menu_callback clicked_cb,
					  enabled_callback enabled_cb = NULL,
					  void* user_data = NULL,
					  KEY key = KEY_NONE, MASK mask = MASK_NONE,
					  BOOL enabled = TRUE,
					  on_disabled_callback on_disabled_cb = NULL);
	LLMenuItemCallGL( const std::string& name,
					  const std::string& label,
 					  menu_callback clicked_cb,
					  enabled_callback enabled_cb = NULL,
					  void* user_data = NULL,
					  KEY key = KEY_NONE, MASK mask = MASK_NONE,
					  BOOL enabled = TRUE,
					  on_disabled_callback on_disabled_cb = NULL);
	LLMenuItemCallGL( const std::string& name,
					  const std::string& label,
					  menu_callback clicked_cb,
					  enabled_callback enabled_cb,
					  label_callback label_cb,
					  void* user_data,
					  KEY key = KEY_NONE, MASK mask = MASK_NONE,
					  BOOL enabled = TRUE,
					  on_disabled_callback on_disabled_c = NULL);
	LLMenuItemCallGL( const std::string& name,
					  menu_callback clicked_cb,
					  enabled_callback enabled_cb,
					  label_callback label_cb,
					  void* user_data,
					  KEY key = KEY_NONE, MASK mask = MASK_NONE,
					  BOOL enabled = TRUE,
					  on_disabled_callback on_disabled_c = NULL);
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	virtual std::string getType() const	{ return "call"; }
	void setEnabledControl(std::string enabled_control, LLView *context);
	void setVisibleControl(std::string enabled_control, LLView *context);
	void setMenuCallback(menu_callback callback, void* data) { mCallback = callback;  mUserData = data; };
	menu_callback getMenuCallback() const { return mCallback; }
	void setEnabledCallback(enabled_callback callback) { mEnabledCallback = callback; };
	void setUserData(void *userdata)	{ mUserData = userdata; }
	void* getUserData() const { return mUserData; }
protected:
	void updateEnabled( void );
public:
	virtual void buildDrawLabel( void );
	virtual void onCommit( void );
	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);
	virtual BOOL handleKeyHere(KEY key, MASK mask);
private:
	menu_callback			mCallback;
	enabled_callback		mEnabledCallback;
	label_callback			mLabelCallback;
	void*					mUserData;
	on_disabled_callback	mOnDisabledCallback;
};
class LLMenuItemCheckGL
:	public LLMenuItemCallGL
{
public:
	LLMenuItemCheckGL( const std::string& name,
					   const std::string& label,
					   menu_callback callback,
					   enabled_callback enabled_cb,
					   check_callback check,
					   void* user_data,
					   KEY key = KEY_NONE, MASK mask = MASK_NONE );
	LLMenuItemCheckGL( const std::string& name,
					   menu_callback callback,
					   enabled_callback enabled_cb,
					   check_callback check,
					   void* user_data,
					   KEY key = KEY_NONE, MASK mask = MASK_NONE );
	LLMenuItemCheckGL( const std::string& name,
					   const std::string& label,
					   menu_callback callback,
					   enabled_callback enabled_cb,
					   std::string control_name,
					   LLView *context,
					   void* user_data,
					   KEY key = KEY_NONE, MASK mask = MASK_NONE );
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	void setCheckedControl(std::string checked_control, LLView *context);
	virtual void setValue(const LLSD& value);
	virtual std::string getType() const	{ return "check"; }
	virtual void buildDrawLabel( void );
private:
	check_callback mCheckCallback;
	BOOL mChecked;
};
class LLMenuItemToggleGL : public LLMenuItemGL
{
public:
	LLMenuItemToggleGL( const std::string& name, const std::string& label,
						BOOL* toggle,
						KEY key = KEY_NONE, MASK mask = MASK_NONE );
	LLMenuItemToggleGL( const std::string& name,
						BOOL* toggle,
						KEY key = KEY_NONE, MASK mask = MASK_NONE );
	virtual std::string getType() const	{ return "toggle"; }
	virtual void buildDrawLabel( void );
	virtual void onCommit( void );
private:
	BOOL* mToggle;
};
class LLMenuGL
:	public LLUICtrl
{
public:
	static const std::string BOOLEAN_TRUE_PREFIX;
	static const std::string BRANCH_SUFFIX;
	static const std::string ARROW_UP;
	static const std::string ARROW_DOWN;
	typedef enum e_scrolling_direction
	{
		SD_UP = 0,
		SD_DOWN = 1,
		SD_BEGIN = 2,
		SD_END = 3
	} EScrollingDirection;
protected:
	friend class LLMenuItemBranchGL;
public:
	LLMenuGL( const std::string& name, const std::string& label);
	LLMenuGL( const std::string& label);
	virtual ~LLMenuGL( void );
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void initMenuXML(LLXMLNodePtr node, LLView* parent);
	void parseChildXML(LLXMLNodePtr child, LLView *parent);
	BOOL handleUnicodeCharHere( llwchar uni_char );
	BOOL handleHover( S32 x, S32 y, MASK mask );
	BOOL handleScrollWheel( S32 x, S32 y, S32 clicks );
	void draw( void );
	void drawBackground(LLMenuItemGL* itemp, LLColor4& color);
	void setVisible(BOOL visible);
	bool addChild(LLView* view, S32 tab_group = 0);
	void removeChild( LLView* ctrl);
	BOOL postBuild();
	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);
	LLMenuGL* getChildMenuByName(const std::string& name, BOOL recurse) const;
	BOOL clearHoverItem();
	const std::string& getLabel( void ) const { return mLabel.getString(); }
	void setLabel(const LLStringExplicit& label) { mLabel = label; }
	static void setDefaultBackgroundColor( const LLColor4& color ) { sDefaultBackgroundColor = color; }
	void setBackgroundColor( const LLColor4& color ) { mBackgroundColor = color; }
	const LLColor4& getBackgroundColor() const { return mBackgroundColor; }
	void setBackgroundVisible( BOOL b )	{ mBgVisible = b; }
	void setCanTearOff(BOOL tear_off);
	virtual BOOL addSeparator(const std::string& name = LLStringUtil::null);
	virtual void updateParent( LLView* parentp );
	void setItemEnabled( const std::string& name, BOOL enable );
	void setEnabledSubMenus(BOOL enable);
	void setItemVisible( const std::string& name, BOOL visible);
	void setLeftAndBottom(S32 left, S32 bottom);
	virtual BOOL handleJumpKey(KEY key);
	virtual BOOL jumpKeysActive();
	virtual BOOL isOpen();
	void needsArrange() { mNeedsArrange = TRUE; }
	virtual void arrange( void );
	void arrangeAndClear( void );
	void empty( void );
	void erase(S32 begin, S32 end, bool arrange = true);
	typedef std::list<LLMenuItemGL*> item_list_t;
	inline item_list_t::iterator erase(item_list_t::const_iterator first, item_list_t::const_iterator last)
	{
		for (auto it = first; it != last; ++it)
			LLUICtrl::removeChild(*it);
		return mItems.erase(first, last);
	}
	void insert(S32 begin, LLView* ctrl, bool arrange = true);
	void insert(item_list_t::const_iterator position_iter, LLMenuItemGL* item, bool arrange = true);
	item_list_t::const_iterator find(LLMenuItemGL* item) const { return std::find(mItems.begin(), mItems.end(), item); }
	item_list_t::const_iterator end() const { return mItems.cend(); }
	const item_list_t& getItems() const { return mItems; }
	item_list_t::size_type getItemCount() const { return mItems.size(); }
	void			setItemLastSelected(LLMenuItemGL* item);
	LLMenuItemGL*	getItem(S32 number);
	LLMenuItemGL*	getHighlightedItem();
	LLMenuItemGL*	highlightNextItem(LLMenuItemGL* cur_item, BOOL skip_disabled = TRUE);
	LLMenuItemGL*	highlightPrevItem(LLMenuItemGL* cur_item, BOOL skip_disabled = TRUE);
	void buildDrawLabels();
	void createJumpKeys();
	static void showPopup(LLMenuGL* menu);
	static void showPopup(LLView* spawning_view, LLMenuGL* menu, S32 x, S32 y);
	void setDropShadowed( const BOOL shadowed );
	void setParentMenuItem( LLMenuItemGL* parent_menu_item ) { mParentMenuItem = parent_menu_item->getHandle(); }
	LLMenuItemGL* getParentMenuItem() const { return dynamic_cast<LLMenuItemGL*>(mParentMenuItem.get()); }
	void setTornOff(BOOL torn_off);
	BOOL getTornOff() { return mTornOff; }
	BOOL getCanTearOff() { return mTearOffItem != NULL; }
	KEY getJumpKey() const { return mJumpKey; }
	void setJumpKey(KEY key) { mJumpKey = key; }
	static void setKeyboardMode(BOOL mode) { sKeyboardMode = mode; }
	static BOOL getKeyboardMode() { return sKeyboardMode; }
	S32 getShortcutPad() { return mShortcutPad; }
	bool scrollItems(EScrollingDirection direction);
	BOOL isScrollable() const { return mScrollable; }
	void setScrollable(bool b);
	static class LLMenuHolderGL* sMenuContainer;
	void resetScrollPositionOnShow(bool reset_scroll_pos) { mResetScrollPositionOnShow = reset_scroll_pos; }
	bool isScrollPositionOnShowReset() { return mResetScrollPositionOnShow; }
protected:
	void createSpilloverBranch();
	void cleanupSpilloverBranch();
public:
	virtual BOOL append( LLMenuItemGL* item );
	virtual BOOL appendMenu( LLMenuGL* menu );
protected:
	item_list_t mItems;
	LLMenuItemGL*mFirstVisibleItem;
	LLMenuItemGL *mArrowUpItem, *mArrowDownItem;
	typedef std::map<KEY, LLMenuItemGL*> navigation_key_map_t;
	navigation_key_map_t mJumpKeys;
	S32				mLastMouseX;
	S32				mLastMouseY;
	S32				mMouseVelX;
	S32				mMouseVelY;
	U32				mMaxScrollableItems;
	BOOL			mHorizontalLayout;
	BOOL			mScrollable;
	BOOL			mKeepFixedSize;
	BOOL			mNeedsArrange;
private:
	static LLColor4 sDefaultBackgroundColor;
	static BOOL		sKeyboardMode;
	LLColor4		mBackgroundColor;
	BOOL			mBgVisible;
	LLHandle<LLView> mParentMenuItem;
	LLUIString		mLabel;
	BOOL mDropShadowed;
	bool			mHasSelection;
	LLFrameTimer	mFadeTimer;
	LLTimer			mScrollItemsTimer;
	BOOL			mTornOff;
	class LLMenuItemTearOffGL* mTearOffItem;
	class LLMenuItemBranchGL* mSpilloverBranch;
	LLMenuGL*		mSpilloverMenu;
	KEY				mJumpKey;
	BOOL			mCreateJumpKeys;
	S32				mShortcutPad;
	bool			mResetScrollPositionOnShow;
};
class LLMenuItemBranchGL : public LLMenuItemGL
{
public:
	LLMenuItemBranchGL( const std::string& name, const std::string& label, LLHandle<LLView> branch,
						KEY key = KEY_NONE, MASK mask = MASK_NONE );
	virtual ~LLMenuItemBranchGL();
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	virtual std::string getType() const { return "menu"; }
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);
	virtual BOOL addToAcceleratorList(std::list <LLKeyBinding*> *listp);
	virtual void buildDrawLabel( void );
	virtual void onCommit( void );
	virtual BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	virtual void setHighlight( BOOL highlight );
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual BOOL isActive() const;
	virtual BOOL isOpen() const;
	LLMenuGL* getBranch() const { return (LLMenuGL*)mBranchHandle.get(); }
	virtual void updateBranchParent( LLView* parentp );
	virtual void handleVisibilityChange( BOOL curVisibilityIn );
	virtual void draw();
	virtual void setEnabledSubMenus(BOOL enabled) { if(getBranch()) getBranch()->setEnabledSubMenus(enabled); }
	virtual void openMenu();
	virtual LLView* getChildView(const std::string& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const;
private:
	LLHandle<LLView> mBranchHandle;
};
class LLContextMenu
: public LLMenuGL
{
public:
	LLContextMenu(const std::string& name, const std::string& label = "");
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	void initXML(LLXMLNodePtr node, LLView* context, LLUICtrlFactory* factory, bool is_context);
public:
	virtual ~LLContextMenu() {}
	virtual void setVisible(BOOL visible);
	virtual void show(S32 x, S32 y, bool context = true);
	virtual void hide();
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	BOOL handleHoverOver(LLMenuItemGL* item, S32 x, S32 y);
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleRightMouseUp( S32 x, S32 y, MASK mask );
	virtual bool addChild(LLView* view, S32 tab_group = 0);
	BOOL appendContextSubMenu(LLContextMenu* menu);
protected:
	BOOL			mHoveredAnyItem;
	LLMenuItemGL*	mHoverItem;
};
class LLPieMenu
: public LLContextMenu
{
public:
	LLPieMenu(const std::string& name, const std::string& label = "");
	virtual ~LLPieMenu() {}
	virtual bool addChild(LLView* view, S32 tab_group = 0);
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	virtual void draw();
	virtual void drawBackground(LLMenuItemGL* itemp, LLColor4& color);
private:
	virtual BOOL append(LLMenuItemGL* item);
public:
	virtual BOOL addSeparator(const std::string& name = LLStringUtil::null) override final;
	virtual void arrange( void );
	void show(S32 x, S32 y, bool mouse_down = true);
	void hide();
private:
	LLMenuItemGL *pieItemFromXY(S32 x, S32 y);
	LLMenuItemGL* pieItemFromIndex(S32 which);
	S32			  pieItemIndexFromXY(S32 x, S32 y);
	BOOL			mFirstMouseDown;
	BOOL			mUseInfiniteRadius;
	S32				mHoverIndex;
	BOOL			mHoverThisFrame;
	LLFrameTimer	mShrinkBorderTimer;
	F32				mOuterRingAlpha;
	F32				mCurRadius;
	BOOL			mRightMouseDown;
};
class LLContextMenuBranch : public LLMenuItemGL
{
public:
	LLContextMenuBranch(const std::string& name, const std::string& label, LLContextMenu* branch);
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	virtual void buildDrawLabel( void );
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask)
	{
		LLMenuItemGL::handleMouseUp(x,y,mask);
		return TRUE;
	}
	virtual void	onCommit( void );
	LLContextMenu* getBranch() { return mBranch; }
	void setHighlight( BOOL highlight );
protected:
	void showSubMenu();
	LLContextMenu* mBranch;
};
class LLMenuBarGL : public LLMenuGL
{
public:
	LLMenuBarGL( const std::string& name );
	virtual ~LLMenuBarGL();
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void setVisible(BOOL visible);
	BOOL handleAcceleratorKey(KEY key, MASK mask);
	BOOL handleKeyHere(KEY key, MASK mask);
	BOOL handleJumpKey(KEY key);
	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	void draw();
	BOOL jumpKeysActive();
	virtual BOOL addSeparator(const std::string& name = LLStringUtil::null) override final;
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	S32 getRightmostMenuEdge();
	void resetMenuTrigger() { mAltKeyTrigger = FALSE; }
private:
	virtual BOOL appendMenu( LLMenuGL* menu );
	virtual void arrange( void );
	void checkMenuTrigger();
	std::list <LLKeyBinding*>	mAccelerators;
	BOOL						mAltKeyTrigger;
};
class LLMenuHolderGL : public LLPanel
{
public:
	LLMenuHolderGL();
	LLMenuHolderGL(const std::string& name, const LLRect& rect, BOOL mouse_opaque, U32 follows = FOLLOWS_NONE);
	virtual ~LLMenuHolderGL() {}
	virtual BOOL hideMenus();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void setCanHide(BOOL can_hide) { mCanHide = can_hide; }
	virtual void draw();
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	BOOL handleRightMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual const LLRect getMenuRect() const { return getLocalRect(); }
	LLView*const getVisibleMenu() const;
	virtual BOOL hasVisibleMenu() const {return getVisibleMenu() != NULL;}
	static LLMenuItemGL* getActivatedItem() { return static_cast<LLMenuItemGL*>(sItemLastSelectedHandle.get()); }
	static void setActivatedItem(LLMenuItemGL* item);
	static LLCoordGL sContextMenuSpawnPos;
private:
	static LLHandle<LLView> sItemLastSelectedHandle;
	static LLFrameTimer sItemActivationTimer;
	BOOL mCanHide;
};
class LLTearOffMenu : public LLFloater
{
public:
	static LLTearOffMenu* create(LLMenuGL* menup);
	virtual ~LLTearOffMenu();
	virtual void onClose(bool app_quitting);
	virtual void draw(void);
	virtual void onFocusReceived();
	virtual void onFocusLost();
	virtual BOOL handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual void translate(S32 x, S32 y);
private:
	LLTearOffMenu(LLMenuGL* menup);
	LLView*		mOldParent;
	LLMenuGL*	mMenu;
	F32			mTargetHeight;
};
class LLMenuItemTearOffGL : public LLMenuItemGL
{
public:
	LLMenuItemTearOffGL();
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	virtual std::string getType() const { return "tearoff_menu"; }
	virtual void onCommit(void);
	virtual void draw(void);
	virtual U32 getNominalHeight() const;
	LLFloater* getParentFloater();
};
class LLEditMenuHandlerMgr
{
public:
	LLEditMenuHandlerMgr& getInstance() {
		static LLEditMenuHandlerMgr instance;
		return instance;
	}
	virtual ~LLEditMenuHandlerMgr() {}
private:
	LLEditMenuHandlerMgr() {};
};
#endif
