/** 
 * @file lluictrl.h
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
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
#ifndef LL_LLUICTRL_H
#define LL_LLUICTRL_H
#include "llrect.h"
#include "llsd.h"
#include "llregistry.h"
#include "llinitparam.h"
#include "llview.h"
#include "llviewmodel.h"
class LLUICtrl
: public LLView, public boost::signals2::trackable
{
public:
	typedef boost::function<void (LLUICtrl* ctrl, const LLSD& param)> commit_callback_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param)> commit_signal_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, S32 x, S32 y, MASK mask)> mouse_signal_t;
	typedef boost::function<bool (LLUICtrl* ctrl, const LLSD& param)> enable_callback_t;
	typedef boost::signals2::signal<bool (LLUICtrl* ctrl, const LLSD& param), boost_boolean_combiner> enable_signal_t;
	struct CallbackParam : public LLInitParam::Block<CallbackParam>
	{
		Ignored					name;
		Optional<std::string>	function_name;
		Optional<LLSD>			parameter;
		Optional<std::string>	control_name;
		CallbackParam();
	};
	struct CommitCallbackParam : public LLInitParam::Block<CommitCallbackParam, CallbackParam >
	{
		Optional<commit_callback_t> function;
	};
	struct EnableCallbackParam : public LLInitParam::Block<EnableCallbackParam, CallbackParam >
	{
		Optional<enable_callback_t> function;
	};
	struct EnableControls : public LLInitParam::ChoiceBlock<EnableControls>
	{
		Alternative<std::string> enabled;
		Alternative<std::string> disabled;
		EnableControls();
	};
	struct ControlVisibility : public LLInitParam::ChoiceBlock<ControlVisibility>
	{
		Alternative<std::string> visible;
		Alternative<std::string> invisible;
		ControlVisibility();
	};
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string>			label;
		Optional<bool>					tab_stop,
										chrome,
										requests_front;
		Optional<LLSD>					initial_value;
		Optional<CommitCallbackParam>	init_callback,
										commit_callback;
		Optional<EnableCallbackParam>	validate_callback;
		Optional<CommitCallbackParam>	mouseenter_callback,
										mouseleave_callback;
		Optional<std::string>			control_name;
		Optional<EnableControls>		enabled_controls;
		Optional<ControlVisibility>		controls_visibility;
		Optional<const LLFontGL*>		font;
		Optional<LLFontGL::HAlign>		font_halign;
		Optional<LLFontGL::VAlign>		font_valign;
		Ignored							type,
										length;
		Params();
	};
	enum ETypeTransparency
	{
		TT_DEFAULT,
		TT_ACTIVE,
		TT_INACTIVE,
		TT_FADING,
	};
	~LLUICtrl();
	void initFromParams(const Params& p);
	static const Params& getDefaultParams();
	LLUICtrl(const Params& p = getDefaultParams(),
			 const LLViewModelPtr& viewmodel=LLViewModelPtr(new LLViewModel));
	LLUICtrl( const std::string& name, const LLRect rect = LLRect(), BOOL mouse_opaque = TRUE,
		commit_callback_t commit_callback = NULL,
		U32 reshape=FOLLOWS_NONE);
	commit_signal_t::slot_type initCommitCallback(const CommitCallbackParam& cb);
	enable_signal_t::slot_type initEnableCallback(const EnableCallbackParam& cb);
	virtual LLViewModel* getViewModel() const;
	BOOL	postBuild() override;
public:
	void	initFromXML(LLXMLNodePtr node, LLView* parent);
	LLXMLNodePtr getXML(bool save_children = true) const;
	BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text ) override;
	BOOL	isCtrl() const override;
	void	onMouseEnter(S32 x, S32 y, MASK mask) override;
	void	onMouseLeave(S32 x, S32 y, MASK mask) override;
	BOOL	canFocusChildren() const override;
	BOOL 	handleMouseDown(S32 x, S32 y, MASK mask) override;
	BOOL 	handleMouseUp(S32 x, S32 y, MASK mask) override;
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask) override;
	BOOL	handleDoubleClick(S32 x, S32 y, MASK mask) override;
	void	setFocus( BOOL b ) override;
	BOOL	hasFocus() const override;
	virtual class LLCtrlSelectionInterface* getSelectionInterface();
	virtual class LLCtrlListInterface* getListInterface();
	virtual class LLCtrlScrollInterface* getScrollInterface();
	void setEnabledControlVariable(LLControlVariable* control);
	void setDisabledControlVariable(LLControlVariable* control);
	void setMakeVisibleControlVariable(LLControlVariable* control);
	void setMakeInvisibleControlVariable(LLControlVariable* control);
	LLControlVariable* getMakeVisibleControlVariable() const { return mMakeVisibleControlVariable; }
	virtual void	setTentative(BOOL b);
	virtual BOOL	getTentative() const;
	virtual void	setValue(const LLSD& value);
	virtual LLSD	getValue() const;
	virtual void    shareViewModelFrom(const LLUICtrl& other);
	virtual BOOL	setTextArg(  const std::string& key, const LLStringExplicit& text );
	virtual void	setIsChrome(BOOL is_chrome);
	virtual BOOL	acceptsTextInput() const;
	virtual BOOL	isDirty() const;
	virtual void	resetDirty();
	virtual void	onCommit();
	virtual void	onTabInto();
	virtual void	clear();
	virtual void	setColor(const LLColor4& color);
	virtual void	setAlpha(F32 alpha);
	virtual void	setMinValue(LLSD min_value);
	virtual void	setMaxValue(LLSD max_value);
	F32 			getCurrentTransparency();
	void				setTransparencyType(ETypeTransparency type);
	ETypeTransparency	getTransparencyType() const {return mTransparencyType;}
	BOOL	focusNextItem(BOOL text_entry_only);
	BOOL	focusPrevItem(BOOL text_entry_only);
	virtual
	BOOL 	focusFirstItem(BOOL prefer_text_fields = FALSE, BOOL focus_flash = TRUE );
	BOOL	focusLastItem(BOOL prefer_text_fields = FALSE);
	LLHandle<LLUICtrl> getHandle() const { return getDerivedHandle<LLUICtrl>(); }
	BOOL			getIsChrome() const;
	void			setTabStop( BOOL b );
	BOOL			hasTabStop() const;
	LLUICtrl*		getParentUICtrl() const;
	void			setCommitOnReturn(BOOL commit) { mCommitOnReturn = commit; }
	BOOL			getCommitOnReturn() const { return mCommitOnReturn; }
	boost::signals2::connection setCommitCallback(const CommitCallbackParam& cb);
	boost::signals2::connection setValidateCallback(const EnableCallbackParam& cb);
	boost::signals2::connection setCommitCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setValidateCallback( const enable_signal_t::slot_type& cb );
	boost::signals2::connection setMouseEnterCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setMouseLeaveCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setMouseDownCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setMouseUpCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setRightMouseDownCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setRightMouseUpCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setDoubleClickCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setCommitCallback( std::function<void (LLUICtrl*,void*)> cb, void* data);
	boost::signals2::connection setValidateBeforeCommit( std::function<bool (const LLSD& data)> cb );
	static LLView* fromXML(LLXMLNodePtr node, LLView* parent, class LLUICtrlFactory* factory);
	LLUICtrl*		findRootMostFocusRoot();
	class LLTextInputFilter : public LLQueryFilter, public LLSingleton<LLTextInputFilter>
	{
		filterResult_t operator() (const LLView* const view, const viewList_t & children) const
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->acceptsTextInput(), TRUE);
		}
	};
	template <typename F, typename DERIVED> class CallbackRegistry : public LLRegistrySingleton<std::string, F, DERIVED >
	{};
	class CommitCallbackRegistry : public CallbackRegistry<commit_callback_t, CommitCallbackRegistry>
	{
	};
	class EnableCallbackRegistry : public CallbackRegistry<enable_callback_t, EnableCallbackRegistry>
	{
	};
protected:
	static bool controlListener(const LLSD& newvalue, LLHandle<LLUICtrl> handle, std::string type);
	commit_signal_t*		mCommitSignal;
	enable_signal_t*		mValidateSignal;
	commit_signal_t*		mMouseEnterSignal;
	commit_signal_t*		mMouseLeaveSignal;
	mouse_signal_t*		mMouseDownSignal;
	mouse_signal_t*		mMouseUpSignal;
	mouse_signal_t*		mRightMouseDownSignal;
	mouse_signal_t*		mRightMouseUpSignal;
	mouse_signal_t*		mDoubleClickSignal;
	LLViewModelPtr  mViewModel;
	LLControlVariable* mEnabledControlVariable;
	boost::signals2::connection mEnabledControlConnection;
	LLControlVariable* mDisabledControlVariable;
	boost::signals2::connection mDisabledControlConnection;
	LLControlVariable* mMakeVisibleControlVariable;
	boost::signals2::connection mMakeVisibleControlConnection;
	LLControlVariable* mMakeInvisibleControlVariable;
	boost::signals2::connection mMakeInvisibleControlConnection;
	static F32 sActiveControlTransparency;
	static F32 sInactiveControlTransparency;
private:
	BOOL			mIsChrome;
	BOOL			mRequestsFront;
	BOOL			mTabStop;
	BOOL			mTentative;
	ETypeTransparency mTransparencyType;
	bool			mCommitOnReturn;
	class DefaultTabGroupFirstSorter;
};
#endif
