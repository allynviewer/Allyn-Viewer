/** 
 * @file llview.h
 * @brief Container for other views, anything that draws.
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
#ifndef LL_LLVIEW_H
#define LL_LLVIEW_H
#include "llcoord.h"
#include "llfontgl.h"
#include "llhandle.h"
#include "llmortician.h"
#include "llmousehandler.h"
#include "llnametable.h"
#include "llsd.h"
#include "llstring.h"
#include "llrect.h"
#include "llui.h"
#include "lluistring.h"
#include "lluixmltags.h"
#include "llviewquery.h"
#include "llxmlnode.h"
#include "stdenums.h"
#include "lluistring.h"
#include "llcursortypes.h"
#include "llinitparam.h"
#include "lltreeiterators.h"
#include "llfocusmgr.h"
#include <boost/container/flat_map.hpp>
#include "ailist.h"
const U32	FOLLOWS_NONE	= 0x00;
const U32	FOLLOWS_LEFT	= 0x01;
const U32	FOLLOWS_RIGHT	= 0x02;
const U32	FOLLOWS_TOP		= 0x10;
const U32	FOLLOWS_BOTTOM	= 0x20;
const U32	FOLLOWS_ALL		= 0x33;
const BOOL	MOUSE_OPAQUE = TRUE;
const BOOL	NOT_MOUSE_OPAQUE = FALSE;
const U32 GL_NAME_UI_RESERVED = 2;
class LLUICtrlFactory;
class LLWidgetClassRegistry : public LLSingleton<LLWidgetClassRegistry>
{
	friend class LLSingleton<LLWidgetClassRegistry>;
public:
	typedef LLView* (*factory_func_t)(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	typedef std::map<std::string, factory_func_t> factory_map_t;
	void registerCtrl(const std::string& xml_tag, factory_func_t function);
	BOOL isTagRegistered(const std::string& xml_tag);
	factory_func_t getCreatorFunc(const std::string& xml_tag);
	template <class T> std::string getTag()
	{
		factory_map_t::iterator it;
		for(it = mCreatorFunctions.begin(); it != mCreatorFunctions.end(); ++it)
		{
			if (it->second == T::fromXML)
			{
				return it->first;
			}
		}
		return "";
	}
private:
	LLWidgetClassRegistry();
	virtual ~LLWidgetClassRegistry() {};
	typedef std::set<std::string> ctrl_name_set_t;
	ctrl_name_set_t mUICtrlNames;
	factory_map_t mCreatorFunctions;
};
template<class T>
class LLRegisterWidget
{
public:
	LLRegisterWidget(const std::string& tag)
	{
		LLWidgetClassRegistry* registry = LLWidgetClassRegistry::getInstance();
		if (registry->isTagRegistered(tag))
		{
			LL_ERRS() << "Widget named " << tag << " already registered!" << LL_ENDL;
		}
		else
		{
			registry->registerCtrl(tag, T::fromXML);
		}
	}
};
class LLViewDrawContext
{
public:
	F32 mAlpha;
	LLViewDrawContext(F32 alpha = 1.f)
	:	mAlpha(alpha)
	{
		if (!sDrawContextStack.empty())
		{
			LLViewDrawContext* context_top = sDrawContextStack.back();
			mAlpha *= context_top->mAlpha;
		}
		sDrawContextStack.push_back(this);
	}
	~LLViewDrawContext()
	{
		sDrawContextStack.pop_back();
	}
	static const LLViewDrawContext& getCurrentContext();
private:
	static std::vector<LLViewDrawContext*> sDrawContextStack;
};
class LLView
:	public LLMouseHandler,
	public LLFocusableElement,
	public LLMortician,
	public LLHandleProvider<LLView>
{
public:
	struct Follows : public LLInitParam::ChoiceBlock<Follows>
	{
		Alternative<std::string>	string;
		Alternative<U32>			flags;
        Follows();
	};
	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<std::string>		name;
		Optional<bool>				enabled,
									visible,
									mouse_opaque,
									use_bounding_rect,
									from_xui,
									focus_root;
		Optional<S32>				tab_group,
									default_tab_group;
		Optional<std::string>		tool_tip;
		Optional<S32>				sound_flags;
		Optional<Follows>			follows;
		Optional<std::string>		hover_cursor;
		Optional<std::string>		layout;
		Optional<LLRect>			rect;
		Optional<S32>				bottom_delta,
									top_pad,
									top_delta,
									left_pad,
									left_delta;
		Ignored						needs_translate,
									xmlns,
									xmlns_xsi,
									xsi_schemaLocation,
									xsi_type;
		Params();
	};
	void initFromParams(const LLView::Params&);
	template<typename T>
	struct CachedUICtrl
	{
		CachedUICtrl():mPtr(NULL){}
		T* connect(LLView* parent,const char* pName){return mPtr = parent->getChild<T>(pName);}
		T* operator->(){return mPtr;}
		operator T*() const{return mPtr;}
		T* mPtr;
	};
protected:
	LLView(const LLView::Params&);
private:
	void init(const LLView::Params&);
private:
	LLView(const LLView& other) = delete;
public:
#if LL_DEBUG
	static BOOL sIsDrawing;
#endif
	enum ESoundFlags
	{
		SILENT = 0,
		MOUSE_DOWN = 1,
		MOUSE_UP = 2
	};
	enum ESnapType
	{
		SNAP_PARENT,
		SNAP_SIBLINGS,
		SNAP_PARENT_AND_SIBLINGS
	};
	enum ESnapEdge
	{
		SNAP_LEFT,
		SNAP_TOP,
		SNAP_RIGHT,
		SNAP_BOTTOM
	};
	typedef AIList<LLView*> child_list_t;
	typedef child_list_t::iterator					child_list_iter_t;
	typedef child_list_t::const_iterator  			child_list_const_iter_t;
	typedef child_list_t::reverse_iterator 			child_list_reverse_iter_t;
	typedef child_list_t::const_reverse_iterator 	child_list_const_reverse_iter_t;
	typedef std::vector<class LLUICtrl *>				ctrl_list_t;
	typedef std::pair<S32, S32>							tab_order_t;
	typedef std::pair<LLUICtrl *, tab_order_t>			tab_order_pair_t;
	typedef std::map<const LLUICtrl*, tab_order_t>		child_tab_order_t;
	typedef child_tab_order_t::iterator					child_tab_order_iter_t;
	typedef child_tab_order_t::const_iterator			child_tab_order_const_iter_t;
	typedef child_tab_order_t::reverse_iterator			child_tab_order_reverse_iter_t;
	typedef child_tab_order_t::const_reverse_iterator	child_tab_order_const_reverse_iter_t;
	LLView();
	LLView(const std::string& name, BOOL mouse_opaque);
	LLView(const std::string& name, const LLRect& rect, BOOL mouse_opaque, U32 follows=FOLLOWS_NONE);
	virtual ~LLView();
	BOOL isView() const;
	virtual BOOL isCtrl() const;
	virtual BOOL isPanel() const;
	void		setMouseOpaque( BOOL b )		{ mMouseOpaque = b; }
	BOOL		getMouseOpaque() const			{ return mMouseOpaque; }
	void		setToolTip( const LLStringExplicit& msg );
	BOOL		setToolTipArg( const LLStringExplicit& key, const LLStringExplicit& text );
	void		setToolTipArgs( const LLStringUtil::format_map_t& args );
	virtual void setRect(const LLRect &rect);
	void		setFollows(U32 flags)			{ mReshapeFlags = flags; }
	void		setFollowsNone()				{ mReshapeFlags = FOLLOWS_NONE; }
	void		setFollowsLeft()				{ mReshapeFlags |= FOLLOWS_LEFT; }
	void		setFollowsTop()					{ mReshapeFlags |= FOLLOWS_TOP; }
	void		setFollowsRight()				{ mReshapeFlags |= FOLLOWS_RIGHT; }
	void		setFollowsBottom()				{ mReshapeFlags |= FOLLOWS_BOTTOM; }
	void		setFollowsAll()					{ mReshapeFlags |= FOLLOWS_ALL; }
	void        setSoundFlags(U8 flags)			{ mSoundFlags = flags; }
	void		setName(std::string name)			{ mName = name; }
	void		setUseBoundingRect( BOOL use_bounding_rect );
	BOOL		getUseBoundingRect() const;
	ECursorType	getHoverCursor() { return mHoverCursor; }
	const std::string& getToolTip() const			{ return mToolTipMsg.getString(); }
	void		sendChildToFront(LLView* child);
	void		sendChildToBack(LLView* child);
	void		moveChildToFrontOfTabGroup(LLUICtrl* child);
	void		moveChildToBackOfTabGroup(LLUICtrl* child);
	virtual bool addChild(LLView* view, S32 tab_group = 0);
	bool		addChildInBack(LLView* view,  S32 tab_group = 0);
	virtual void	removeChild(LLView* view);
	virtual BOOL	postBuild() { return TRUE; }
	const child_tab_order_t& getCtrlOrder() const		{ return mCtrlOrder; }
	ctrl_list_t getCtrlList() const;
	ctrl_list_t getCtrlListSorted() const;
	void setDefaultTabGroup(S32 d)				{ mDefaultTabGroup = d; }
	S32 getDefaultTabGroup() const				{ return mDefaultTabGroup; }
	S32 getLastTabGroup()						{ return mLastTabGroup; }
	BOOL		isInVisibleChain() const;
	BOOL		isInEnabledChain() const;
	void		setFocusRoot(BOOL b)			{ mIsFocusRoot = b; }
	BOOL		isFocusRoot() const				{ return mIsFocusRoot; }
	virtual BOOL canFocusChildren() const;
	BOOL focusNextRoot();
	BOOL focusPrevRoot();
	virtual bool	hasAccelerators() const { return false; }
	virtual void deleteAllChildren();
	virtual void	setTentative(BOOL b);
	virtual BOOL	getTentative() const;
	void 	setAllChildrenEnabled(BOOL b);
	virtual void	setVisible(BOOL visible);
	const BOOL&		getVisible() const			{ return mVisible; }
	virtual void	setEnabled(BOOL enabled);
	BOOL			getEnabled() const			{ return mEnabled; }
	U8              getSoundFlags() const       { return mSoundFlags; }
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	virtual void	handleVisibilityChange ( BOOL new_visibility );
	void			pushVisible(BOOL visible)	{ mLastVisible = mVisible; setVisible(visible); }
	void			popVisible()				{ setVisible(mLastVisible); }
	BOOL			getLastVisible()	const	{ return mLastVisible; }
	U32			getFollows() const				{ return mReshapeFlags; }
	BOOL		followsLeft() const				{ return mReshapeFlags & FOLLOWS_LEFT; }
	BOOL		followsRight() const			{ return mReshapeFlags & FOLLOWS_RIGHT; }
	BOOL		followsTop() const				{ return mReshapeFlags & FOLLOWS_TOP; }
	BOOL		followsBottom() const			{ return mReshapeFlags & FOLLOWS_BOTTOM; }
	BOOL		followsAll() const				{ return mReshapeFlags & FOLLOWS_ALL; }
	const LLRect&	getRect() const				{ return mRect; }
	const LLRect&	getBoundingRect() const		{ return mBoundingRect; }
	LLRect	getLocalBoundingRect() const;
	LLRect	calcScreenRect() const;
	LLRect	calcScreenBoundingRect() const;
	LLRect	getLocalRect() const;
	virtual LLRect getSnapRect() const;
	LLRect getLocalSnapRect() const;
	virtual LLRect getRequiredRect();
	LLRect calcBoundingRect();
	void updateBoundingRect();
	LLView*		getRootView();
	LLView*		getParent() const				{ return mParentView; }
	LLView*		getFirstChild() const			{ return (mChildList.empty()) ? NULL : *(mChildList.begin()); }
	LLView*		findPrevSibling(LLView* child);
	LLView*		findNextSibling(LLView* child);
	S32			getChildCount()	const			{ return (S32)mChildList.size(); }
	template<class _Pr3> void sortChildren(_Pr3 _Pred) { mChildList.sort(_Pred); }
	BOOL		hasAncestor(const LLView* parentp) const;
	BOOL		hasChild(const std::string& childname, BOOL recurse = FALSE) const;
	BOOL 		childHasKeyboardFocus( const std::string& childname ) const;
	typedef LLTreeDFSIter<LLView, child_list_const_iter_t> tree_iterator_t;
	tree_iterator_t beginTreeDFS();
	tree_iterator_t endTreeDFS();
	typedef LLTreeDFSPostIter<LLView, child_list_const_iter_t> tree_post_iterator_t;
	tree_post_iterator_t beginTreeDFSPost();
	tree_post_iterator_t endTreeDFSPost();
	typedef LLTreeBFSIter<LLView, child_list_const_iter_t> bfs_tree_iterator_t;
	bfs_tree_iterator_t beginTreeBFS();
	bfs_tree_iterator_t endTreeBFS();
	typedef LLTreeDownIter<LLView> root_to_view_iterator_t;
	root_to_view_iterator_t beginRootToView();
	root_to_view_iterator_t endRootToView();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	translate( S32 x, S32 y );
	void			setOrigin( S32 x, S32 y )	{ mRect.translate( x - mRect.mLeft, y - mRect.mBottom ); }
	BOOL			translateIntoRect( const LLRect& constraint, BOOL allow_partial_outside );
	BOOL			translateIntoRectWithExclusion( const LLRect& inside, const LLRect& exclude, BOOL allow_partial_outside );
	void			centerWithin(const LLRect& bounds);
	void	setShape(const LLRect& new_rect, bool by_user = false);
	virtual LLView*	findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type, S32 threshold, S32 padding = 0);
	virtual LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding = 0);
	virtual BOOL	canSnapTo(const LLView* other_view);
	virtual void	setSnappedTo(const LLView* snap_view);
	BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	BOOL	handleKeyUp(KEY key, MASK mask, BOOL called_from_parent);
	BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  EAcceptance* accept,
									  std::string& tooltip_msg);
	std::string getShowNamesToolTip();
	virtual void	draw();
	void parseFollowsFlags(const LLView::Params& params);
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory);
	virtual void initFromXML(LLXMLNodePtr node, LLView* parent);
	void parseFollowsFlags(LLXMLNodePtr node);
	BOOL getSaveToXML() const { return mSaveToXML; }
	void setSaveToXML(BOOL b) { mSaveToXML = b; }
	typedef enum e_hit_test_type
	{
		HIT_TEST_USE_BOUNDING_RECT,
		HIT_TEST_IGNORE_BOUNDING_RECT
	}EHitTestType;
	BOOL parentPointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
	BOOL pointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
	BOOL blockMouseEvent(S32 x, S32 y) const;
	BOOL localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, LLView* other_view) const;
	BOOL localRectToOtherView( const LLRect& local, LLRect* other, LLView* other_view ) const;
	void screenRectToLocal( const LLRect& screen, LLRect* local ) const;
	void localRectToScreen( const LLRect& local, LLRect* screen ) const;
	LLOldEvents::LLSimpleListener* getListenerByName(const std::string& callback_name);
	void registerEventListener(std::string name, LLOldEvents::LLSimpleListener* function);
	void deregisterEventListener(std::string name);
	typedef boost::signals2::signal<void (LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)> event_signal_t;
	void registerEventListener(std::string name, event_signal_t::slot_type &cb);
	std::string findEventListener(LLOldEvents::LLSimpleListener *listener) const;
	void addListenerToControl(LLOldEvents::LLEventDispatcher *observer, const std::string& name, LLSD filter, LLSD userdata);
	void addBoolControl(const std::string& name, bool initial_value);
	LLControlVariable *getControl(const std::string& name);
	LLControlVariable *findControl(const std::string& name);
	bool setControlValue(const LLSD& value);
	virtual void	setControlName(const std::string& control, LLView *context);
	virtual std::string getControlName() const { return mControlName; }
	virtual void	setValue(const LLSD& value);
	virtual LLSD	getValue() const;
	const child_list_t*	getChildList() const { return &mChildList; }
	child_list_const_iter_t	beginChild() const { return mChildList.begin(); }
	child_list_const_iter_t	endChild() const { return mChildList.end(); }
	boost::container::flat_map<std::string, LLView*> mChildHashMap;
	BOOL	handleHover(S32 x, S32 y, MASK mask);
	BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL	handleMiddleMouseUp(S32 x, S32 y, MASK mask);
	BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask);
	BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect);
	const std::string& getName() const;
	void	onMouseCaptureLost();
	BOOL	hasMouseCapture();
	BOOL isView();
	void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const;
	void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const;
	virtual		LLView*	childFromPoint(S32 x, S32 y, bool recur=false);
	virtual void	onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void	onMouseLeave(S32 x, S32 y, MASK mask);
	template <class T> T* findChild(const std::string& name)
	{
		return getChild<T>(name,true,false);
	}
	template <class T> T* getChild(const std::string& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const
	{
		LLView* child = getChildView(name, recurse, FALSE);
		T* result = dynamic_cast<T*>(child);
		if (!result)
		{
			if (child)
			{
				LL_WARNS() << "Found child named " << name << " but of wrong type " << typeid(child).name() << ", expecting " << typeid(T).name() << LL_ENDL;
			}
			if (create_if_missing)
			{
				result = createDummyWidget<T>(name);
			}
		}
		return result;
	}
	template <class T> T& getChildRef(const std::string& name, BOOL recurse = TRUE) const
	{
		return *getChild<T>(name, recurse, TRUE);
	}
	virtual LLView* getChildView(const std::string& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const;
	template <class T> T* createDummyWidget(const std::string& name) const
	{
		T* widget = getDummyWidget<T>(name);
		if (!widget)
		{
			std::string xml_tag = LLWidgetClassRegistry::getInstance()->getTag<T>();
			if (xml_tag.empty())
			{
				LL_WARNS() << "No xml tag registered for this class " << LL_ENDL;
				return NULL;
			}
			LLXMLNodePtr new_node_ptr = new LLXMLNode(xml_tag.c_str(), FALSE);
			new_node_ptr->createChild("name", TRUE)->setStringValue(name);
			widget = dynamic_cast<T*>(createWidget(new_node_ptr));
			if (widget)
			{
				LL_WARNS() << "Making dummy " << xml_tag << " named " << name << " in " << getName() << LL_ENDL;
				mDummyWidgets.insert(std::make_pair(name, widget));
			}
			else
			{
				LL_WARNS() << "Failed to create dummy widget of requested type " << LL_ENDL;
				return NULL;
			}
		}
		return widget;
	}
	template <class T> T* getDummyWidget(const std::string& name) const
	{
		dummy_widget_map_t::const_iterator found_it = mDummyWidgets.find(name);
		if (found_it == mDummyWidgets.end())
		{
			return NULL;
		}
		return dynamic_cast<T*>(found_it->second);
	}
	LLView* createWidget(LLXMLNodePtr xml_node) const;
	static U32 createRect(LLXMLNodePtr node, LLRect &rect, LLView* parent_view, const LLRect &required_rect = LLRect());
	static LLFontGL* selectFont(LLXMLNodePtr node);
	static LLFontGL::HAlign selectFontHAlign(LLXMLNodePtr node);
	static LLFontGL::VAlign selectFontVAlign(LLXMLNodePtr node);
	static LLFontGL::StyleFlags selectFontStyle(LLXMLNodePtr node);
	static void addColorXML(LLXMLNodePtr node, const LLColor4& color,
							const char* xml_name, const char* control_name);
	static LLWString escapeXML(const LLWString& xml);
	static std::string escapeXML(const std::string& xml, std::string& indent);
	static	BOOL focusNext(viewList_t& result);
	static	BOOL focusPrev(viewList_t& result);
	static const LLCtrlQuery & getTabOrderQuery();
	static const LLCtrlQuery & getFocusRootsQuery();
	static BOOL deleteViewByHandle(LLHandle<LLView> handle);
	static LLWindow*	getWindow(void) { return LLUI::sWindow; }
	virtual void	handleReshape(const LLRect& rect, bool by_user);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleKeyUpHere(KEY key, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);
	virtual S32	notifyParent(const LLSD& info);
	virtual bool	notifyChildren(const LLSD& info);
	virtual S32	notify(const LLSD& info) { return 0;};
	static const LLViewDrawContext& getDrawContext();
protected:
	void			drawDebugRect();
	void			drawChild(LLView* childp, S32 x_offset = 0, S32 y_offset = 0, BOOL force_draw = FALSE);
	void			drawChildren();
	bool			visibleAndContains(S32 local_x, S32 local_Y);
	bool			visibleEnabledAndContains(S32 local_x, S32 local_y);
	void			logMouseEvent();
	LLView*	childrenHandleKey(KEY key, MASK mask);
	LLView*	childrenHandleKeyUp(KEY key, MASK mask);
	LLView* childrenHandleUnicodeChar(llwchar uni_char);
	LLView*	childrenHandleDragAndDrop(S32 x, S32 y, MASK mask,
											  BOOL drop,
											  EDragAndDropType type,
											  void* data,
											  EAcceptance* accept,
											  std::string& tooltip_msg);
	LLView*	childrenHandleHover(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMouseUp(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMiddleMouseUp(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMiddleMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleDoubleClick(S32 x, S32 y, MASK mask);
	LLView*	childrenHandleScrollWheel(S32 x, S32 y, S32 clicks);
	LLView* childrenHandleRightMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleRightMouseUp(S32 x, S32 y, MASK mask);
	static bool controlListener(const LLSD& newvalue, LLHandle<LLView> handle, std::string type);
	typedef std::map<std::string, LLControlVariable*> control_map_t;
	control_map_t mFloaterControls;
private:
	template <typename METHOD, typename XDATA>
	LLView* childrenHandleMouseEvent(const METHOD& method, S32 x, S32 y, XDATA extra, bool allow_mouse_block = true);
	template <typename METHOD, typename CHARTYPE>
	LLView* childrenHandleCharEvent(const std::string& desc, const METHOD& method,
									CHARTYPE c, MASK mask);
	BOOL	handleUnicodeCharWithDummyMask(llwchar uni_char, MASK , BOOL from_parent)
	{
		return handleUnicodeChar(uni_char, from_parent);
	}
	LLView*		mParentView;
	child_list_t mChildList;
	BOOL		mVisible;
	LLRect		mRect;
	LLRect		mBoundingRect;
	std::string	mName;
	U32			mReshapeFlags;
	child_tab_order_t mCtrlOrder;
	S32			mDefaultTabGroup;
	S32			mLastTabGroup;
	BOOL		mEnabled;
	BOOL		mMouseOpaque;
	LLUIString	mToolTipMsg;
	U8          mSoundFlags;
	BOOL		mSaveToXML;
	BOOL		mIsFocusRoot;
	BOOL		mUseBoundingRect;
	BOOL		mLastVisible;
	S32			mNextInsertionOrdinal;
	bool		mInDraw;
private:
	static LLWindow* sWindow;
	typedef std::map<std::string, LLPointer<LLOldEvents::LLSimpleListener> > dispatch_list_t;
	dispatch_list_t mDispatchList;
	std::string		mControlName;
	typedef std::map<std::string, LLView*> dummy_widget_map_t;
	mutable dummy_widget_map_t mDummyWidgets;
	boost::signals2::connection mControlConnection;
	ECursorType mHoverCursor;
	typedef boost::function<bool(const LLView*, S32 x, S32 y)> DrilldownFunc;
	static DrilldownFunc sDrilldown;
public:
	class TemporaryDrilldownFunc: public boost::noncopyable
	{
	public:
		TemporaryDrilldownFunc(const DrilldownFunc& func):
			mOldDrilldown(sDrilldown)
		{
			sDrilldown = func;
		}
		~TemporaryDrilldownFunc()
		{
			sDrilldown = mOldDrilldown;
		}
	private:
		DrilldownFunc mOldDrilldown;
	};
public:
	static BOOL	sDebugRects;
	static BOOL sDebugKeys;
	static S32	sDepth;
	static BOOL sDebugMouseHandling;
	static std::string sMouseHandlerMessage;
	static S32	sSelectID;
	static BOOL sEditingUI;
	static LLView* sEditingUIView;
	static S32 sLastLeftXML;
	static S32 sLastBottomXML;
	static BOOL sForceReshape;
};
class LLCompareByTabOrder
{
public:
	LLCompareByTabOrder(LLView::child_tab_order_t order) : mTabOrder(order) {}
	virtual ~LLCompareByTabOrder() {}
	bool operator() (const LLView* const a, const LLView* const b) const;
private:
	virtual bool compareTabOrders(const LLView::tab_order_t & a, const LLView::tab_order_t & b) const { return a < b; }
	LLView::child_tab_order_t mTabOrder;
};
#endif
