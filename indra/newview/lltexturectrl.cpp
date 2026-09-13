/** 
 * @file lltexturectrl.cpp
 * @author Richard Nelson, James Cook
 * @brief LLTextureCtrl class implementation including related functions
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
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
#include "llviewerprecompiledheaders.h"
#include "lltexturectrl.h"
#include "llrender.h"
#include "llagent.h"
#include "llviewertexturelist.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llbutton.h"
#include "lldraghandle.h"
#include "llfiltereditor.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llfoldervieweventlistener.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "lllineeditor.h"
#include "llui.h"
#include "llviewerinventory.h"
#include "llpermissions.h"
#include "llsaleinfo.h"
#include "llassetstorage.h"
#include "lltextbox.h"
#include "llresizehandle.h"
#include "llscrollcontainer.h"
#include "lltoolmgr.h"
#include "lltoolpipette.h"
#include "llglheaders.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewercontrol.h"
#include "llavatarappearancedefines.h"
#include "llmenugl.h"
#include "floaterlocalassetbrowse.h"
#include "llscrolllistctrl.h"
#define	LOCALLIST_COL_ID 1
static const S32 CLOSE_BTN_WIDTH = 100;
const S32 PIPETTE_BTN_WIDTH = 32;
static const S32 HPAD = 4;
static const S32 VPAD = 4;
static const S32 LINE = 16;
static const S32 SMALL_BTN_WIDTH = 64;
static const S32 TEX_PICKER_MIN_WIDTH = 520;
static const S32 CLEAR_BTN_WIDTH = 50;
static const S32 TEX_PICKER_MIN_HEIGHT = 400;
static const S32 PREVIEW_WIDTH = 156;
static const S32 PREVIEW_HEIGHT = 150;
static const F32 CONTEXT_CONE_IN_ALPHA = 0.0f;
static const F32 CONTEXT_CONE_OUT_ALPHA = 1.f;
static const F32 CONTEXT_FADE_TIME = 0.08f;
class LLFloaterTexturePicker : public LLFloater
{
public:
	LLFloaterTexturePicker(
		LLTextureCtrl* owner,
		const LLRect& rect,
		const std::string& label,
		PermissionMask immediate_filter_perm_mask,
		PermissionMask dnd_filter_perm_mask,
		PermissionMask non_immediate_filter_perm_mask,
		BOOL can_apply_immediately,
		const std::string& fallback_image_name);
	virtual ~LLFloaterTexturePicker();
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
						BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
						EAcceptance *accept,
						std::string& tooltip_msg);
	virtual void	draw();
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual void	onClose(bool app_quitting);
	virtual BOOL    postBuild();
	void setImageID( const LLUUID& image_asset_id);
	const LLUUID& getWhiteImageAssetID() const { return mWhiteImageAssetID; }
	void setWhiteImageAssetID(const LLUUID& id) { mWhiteImageAssetID = id; }
	void updateImageStats();
	const LLUUID& getAssetID() { return mImageAssetID; }
	const LLUUID& findItemID(const LLUUID& asset_id, BOOL copyable_only);
	void			setCanApplyImmediately(BOOL b);
	void			setDirty( BOOL b ) { mIsDirty = b; }
	BOOL			isDirty() const { return mIsDirty; }
	void			setActive( BOOL active );
	LLTextureCtrl*	getOwner() const { return mOwner; }
	void			setOwner(LLTextureCtrl* owner) { mOwner = owner; }
	void			stopUsingPipette();
	PermissionMask 	getFilterPermMask();
	void updateFilterPermMask();
	void commitIfImmediateSet();
	void setCanApply(bool can_preview, bool can_apply);
	void setTextureSelectedCallback(texture_selected_callback cb) {mTextureSelectedCallback = cb;}
	static void		onBtnSetToDefault( void* userdata );
	static void		onBtnSelect( void* userdata );
	static void		onBtnCancel( void* userdata );
		   void		onBtnPipette(  );
	static void		onBtnUUID( void* userdata );
	static void		onBtnWhite( void* userdata );
	static void		onBtnNone( void* userdata );
	static void		onBtnInvisible( void* userdata );
	static void		onBtnAlpha( void* userdata );
	static void		onBtnClear( void* userdata );
		   void		onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	static void		onShowFolders(LLUICtrl* ctrl, void* userdata);
	static void		onApplyImmediateCheck(LLUICtrl* ctrl, void* userdata);
	void			onBakeTextureSelect(const LLSD& val);
	void			onFilterEdit(const std::string& filter_string );
		   void		onTextureSelect( const LLTextureEntry& te );
	static void     onBtnAdd( void* userdata );
	static void     onBtnRemove( void* userdata );
	static void     onBtnBrowser( void* userdata );
	void			onLocalScrollCommit();
protected:
	LLPointer<LLViewerTexture> mTexturep;
	LLTextureCtrl*		mOwner;
	LLUUID				mImageAssetID;
	std::string			mFallbackImageName;
	LLUUID				mWhiteImageAssetID;
	LLUUID				mInvisibleImageAssetID;
	LLUUID				mSpecialCurrentImageAssetID;
	LLUUID				mAlphaImageAssetID;
	LLUUID				mOriginalImageAssetID;
	std::string			mLabel;
	LLTextBox*			mTentativeLabel;
	LLTextBox*			mResolutionLabel;
	std::string			mPendingName;
	BOOL				mIsDirty;
	BOOL				mActive;
	LLFilterEditor*		mFilterEdit;
	LLInventoryPanel*	mInventoryPanel;
	PermissionMask		mImmediateFilterPermMask;
	PermissionMask		mDnDFilterPermMask;
	PermissionMask		mNonImmediateFilterPermMask;
	BOOL				mCanApplyImmediately;
	BOOL				mNoCopyTextureSelected;
	F32					mContextConeOpacity;
	BOOL				mSelectedItemPinned;
	LLScrollListCtrl*   mLocalScrollCtrl;
private:
	bool mCanApply;
	bool mCanPreview;
	texture_selected_callback mTextureSelectedCallback;
};
LLFloaterTexturePicker::LLFloaterTexturePicker(
	LLTextureCtrl* owner,
	const LLRect& rect,
	const std::string& label,
	PermissionMask immediate_filter_perm_mask,
	PermissionMask dnd_filter_perm_mask,
	PermissionMask non_immediate_filter_perm_mask,
	BOOL can_apply_immediately,
	const std::string& fallback_image_name)
	:
	LLFloater( std::string("texture picker"),
		rect,
		std::string( "Pick: " ) + label,
		TRUE,
		TEX_PICKER_MIN_WIDTH, TEX_PICKER_MIN_HEIGHT ),
	mOwner( owner ),
	mImageAssetID( owner->getImageAssetID() ),
	mFallbackImageName( fallback_image_name ),
	mWhiteImageAssetID( gSavedSettings.getString( "UIImgWhiteUUID" ) ),
	mInvisibleImageAssetID(gSavedSettings.getString("UIImgInvisibleUUID")),
	mAlphaImageAssetID("8dcd4a48-2d37-4909-9f78-f7a9eb4ef903"),
	mOriginalImageAssetID(owner->getImageAssetID()),
	mLabel(label),
	mTentativeLabel(NULL),
	mResolutionLabel(NULL),
	mIsDirty( FALSE ),
	mActive( TRUE ),
	mFilterEdit(NULL),
	mImmediateFilterPermMask(immediate_filter_perm_mask),
	mDnDFilterPermMask(dnd_filter_perm_mask),
	mNonImmediateFilterPermMask(non_immediate_filter_perm_mask),
	mContextConeOpacity(0.f),
	mSelectedItemPinned(FALSE),
	mCanApply(true),
	mCanPreview(true)
{
	mCanApplyImmediately = can_apply_immediately;
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_texture_ctrl.xml");
	setCanMinimize(FALSE);
}
LLFloaterTexturePicker::~LLFloaterTexturePicker()
{
}
void LLFloaterTexturePicker::setImageID(const LLUUID& image_id)
{
	if( mImageAssetID != image_id && mActive)
	{
		mNoCopyTextureSelected = FALSE;
		mIsDirty = TRUE;
		mImageAssetID = image_id;
		std::string tab;
		if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
		{
			tab = "bake";
			getChild<LLScrollListCtrl>("l_bake_use_texture_combo_box")->selectByID(mImageAssetID);
		}
		else
		{
			LLUUID item_id = findItemID(mImageAssetID, FALSE);
			if (item_id.isNull())
			{
				mInventoryPanel->getRootFolder()->clearSelection();
			}
			else
			{
				tab = "server_tab";
				LLInventoryItem* itemp = gInventory.getItem(image_id);
				if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
				{
					getChild<LLUICtrl>("apply_immediate_check")->setValue(FALSE);
					mNoCopyTextureSelected = TRUE;
				}
				mInventoryPanel->setSelection(item_id, TAKE_FOCUS_NO);
			}
		}
		if (!tab.empty()) getChild<LLTabContainer>("actions_tab_container")->selectTabByName(tab);
	}
}
void LLFloaterTexturePicker::setActive( BOOL active )
{
	if (!active && getChild<LLUICtrl>("Pipette")->getValue().asBoolean())
	{
		stopUsingPipette();
	}
	mActive = active;
}
void LLFloaterTexturePicker::setCanApplyImmediately(BOOL b)
{
	mCanApplyImmediately = b;
	if (!mCanApplyImmediately)
	{
		getChild<LLUICtrl>("apply_immediate_check")->setValue(FALSE);
	}
	updateFilterPermMask();
}
void LLFloaterTexturePicker::stopUsingPipette()
{
	if (LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance())
	{
		LLToolMgr::getInstance()->clearTransientTool();
	}
}
void LLFloaterTexturePicker::updateImageStats()
{
	if (mTexturep.notNull())
	{
		if (mTexturep->getFullWidth() > 0 && mTexturep->getFullHeight() > 0)
		{
			std::string formatted_dims = llformat("%d x %d", mTexturep->getFullWidth(),mTexturep->getFullHeight());
			mResolutionLabel->setTextArg("[DIMENSIONS]", formatted_dims);
		}
		else
		{
			mResolutionLabel->setTextArg("[DIMENSIONS]", std::string("[? x ?]"));
		}
	}
	else
	{
		mResolutionLabel->setTextArg("[DIMENSIONS]", std::string(""));
	}
}
BOOL LLFloaterTexturePicker::handleDragAndDrop(
		S32 x, S32 y, MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type, void *cargo_data,
		EAcceptance *accept,
		std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	bool is_mesh = cargo_type == DAD_MESH;
	if ((cargo_type == DAD_TEXTURE) || is_mesh)
	{
		LLInventoryItem *item = (LLInventoryItem *)cargo_data;
		BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
		BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
		BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
															gAgent.getID());
		PermissionMask item_perm_mask = 0;
		if (copy) item_perm_mask |= PERM_COPY;
		if (mod)  item_perm_mask |= PERM_MODIFY;
		if (xfer) item_perm_mask |= PERM_TRANSFER;
		PermissionMask filter_perm_mask = mDnDFilterPermMask;
		if ( (item_perm_mask & filter_perm_mask) == filter_perm_mask )
		{
			if (drop)
			{
				setCanApply(true, true);
				setImageID( item->getAssetUUID() );
				commitIfImmediateSet();
			}
			*accept = ACCEPT_YES_SINGLE;
		}
		else
		{
			*accept = ACCEPT_NO;
		}
	}
	else
	{
		*accept = ACCEPT_NO;
	}
	handled = TRUE;
	LL_DEBUGS("UserInput")  << "dragAndDrop handled by LLFloaterTexturePicker " << getName() << LL_ENDL;
	return handled;
}
BOOL LLFloaterTexturePicker::handleKeyHere(KEY key, MASK mask)
{
	LLFolderView* root_folder = mInventoryPanel->getRootFolder();
	if (root_folder && mFilterEdit)
	{
		if (mFilterEdit->hasFocus()
			&& (key == KEY_RETURN || key == KEY_DOWN)
			&& mask == MASK_NONE)
		{
			if (!root_folder->getCurSelectedItem())
			{
				LLFolderViewItem* itemp = root_folder->getItemByID(gInventory.getRootFolderID());
				if (itemp)
				{
					root_folder->setSelection(itemp, FALSE, FALSE);
				}
			}
			root_folder->scrollToShowSelection();
			root_folder->setFocus(TRUE);
			commitIfImmediateSet();
			return TRUE;
		}
		if (root_folder->hasFocus() && key == KEY_UP)
		{
			mFilterEdit->focusFirstItem(TRUE);
		}
	}
	return LLFloater::handleKeyHere(key, mask);
}
void LLFloaterTexturePicker::onClose(bool app_quitting)
{
	if (mOwner)
	{
		mOwner->onFloaterClose();
	}
	stopUsingPipette();
	destroy();
}
BOOL LLFloaterTexturePicker::postBuild()
{
	LLFloater::postBuild();
	if (!mLabel.empty())
	{
		std::string pick = getString("pick title");
		setTitle(pick + mLabel);
	}
	mTentativeLabel = getChild<LLTextBox>("Multiple");
	mResolutionLabel = getChild<LLTextBox>("unknown");
	childSetAction("Default",LLFloaterTexturePicker::onBtnSetToDefault,this);
	childSetAction("None", LLFloaterTexturePicker::onBtnNone,this);
	childSetAction("Alpha", LLFloaterTexturePicker::onBtnAlpha,this);
	childSetAction("Blank", LLFloaterTexturePicker::onBtnWhite,this);
	childSetAction("Invisible", LLFloaterTexturePicker::onBtnInvisible,this);
	childSetAction("Add", LLFloaterTexturePicker::onBtnAdd, this);
	childSetAction("Remove", LLFloaterTexturePicker::onBtnRemove, this);
	childSetAction("Browser", LLFloaterTexturePicker::onBtnBrowser, this);
	mLocalScrollCtrl = getChild<LLScrollListCtrl>("local_name_list");
	mLocalScrollCtrl->setCommitCallback(boost::bind(&LLFloaterTexturePicker::onLocalScrollCommit, this));
	LocalAssetBrowser::UpdateTextureCtrlList( mLocalScrollCtrl );
	childSetCommitCallback("show_folders_check", onShowFolders, this);
	getChildView("show_folders_check")->setVisible( FALSE);
	mFilterEdit = getChild<LLFilterEditor>("inventory search editor");
	mFilterEdit->setCommitCallback(boost::bind(&LLFloaterTexturePicker::onFilterEdit, this, _2));
	mInventoryPanel = getChild<LLInventoryPanel>("inventory panel");
	if(mInventoryPanel)
	{
		U32 filter_types = 0x0;
		filter_types |= 0x1 << LLInventoryType::IT_TEXTURE;
		filter_types |= 0x1 << LLInventoryType::IT_SNAPSHOT;
		mInventoryPanel->setFilterTypes(filter_types);
		mInventoryPanel->setFilterPermMask(mImmediateFilterPermMask);
		mInventoryPanel->setSelectCallback(boost::bind(&LLFloaterTexturePicker::onSelectionChange, this, _1, _2));
		mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		mInventoryPanel->setAllowMultiSelect(FALSE);
		mInventoryPanel->getRootFolder()->setAutoSelectOverride(TRUE);
		mInventoryPanel->setSelection(findItemID(mImageAssetID, FALSE), TAKE_FOCUS_NO);
	}
	mNoCopyTextureSelected = FALSE;
	getChild<LLUICtrl>("apply_immediate_check")->setValue(gSavedSettings.getBOOL("ApplyTextureImmediately"));
	childSetCommitCallback("apply_immediate_check", onApplyImmediateCheck, this);
	if (!mCanApplyImmediately)
	{
		getChildView("show_folders_check")->setEnabled(FALSE);
	}
	getChild<LLUICtrl>("Pipette")->setCommitCallback( boost::bind(&LLFloaterTexturePicker::onBtnPipette, this));
	childSetAction("ApplyUUID", LLFloaterTexturePicker::onBtnUUID,this);
	childSetAction("Cancel", LLFloaterTexturePicker::onBtnCancel,this);
	childSetAction("Select", LLFloaterTexturePicker::onBtnSelect,this);
	mFilterEdit->setFocus(true);
	updateFilterPermMask();
	LLToolPipette::getInstance()->setToolSelectCallback(boost::bind(&LLFloaterTexturePicker::onTextureSelect, this, _1));
	getChild<LLUICtrl>("l_bake_use_texture_combo_box")->setCommitCallback(boost::bind(&LLFloaterTexturePicker::onBakeTextureSelect, this, _2));
	return TRUE;
}
void LLFloaterTexturePicker::draw()
{
	if (mOwner)
	{
		LLRect owner_rect;
		mOwner->localRectToOtherView(mOwner->getLocalRect(), &owner_rect, this);
		LLRect local_rect = getLocalRect();
		if (gFocusMgr.childHasKeyboardFocus(this) && mOwner->isInVisibleChain() && mContextConeOpacity > 0.001f)
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			LLGLEnable<GL_CULL_FACE> clip;
			gGL.begin(LLRender::TRIANGLE_STRIP);
			{
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mLeft, owner_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mRight, local_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mRight, owner_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mRight, owner_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mLeft, owner_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mLeft, owner_rect.mTop);
			}
			gGL.end();
		}
	}
	if (gFocusMgr.childHasMouseCapture(getDragHandle()))
	{
		mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLSmoothInterpolation::getInterpolant(CONTEXT_FADE_TIME));
	}
	else
	{
		mContextConeOpacity = lerp(mContextConeOpacity, 0.f, LLSmoothInterpolation::getInterpolant(CONTEXT_FADE_TIME));
	}
	updateImageStats();
	getChildView("show_folders_check")->setEnabled(mActive && mCanApplyImmediately && !mNoCopyTextureSelected);
	getChildView("Select")->setEnabled(mActive && mCanApply);
	getChildView("Pipette")->setEnabled(mActive);
	getChild<LLUICtrl>("Pipette")->setValue(LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance());
	mFilterEdit->setText(mInventoryPanel->getFilterSubString());
	if( mOwner )
	{
        mTexturep = nullptr;
		if(mImageAssetID.notNull())
		{
			LLPointer<LLViewerFetchedTexture> texture = NULL;
			if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
			{
				LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
				if (obj)
				{
					LLViewerTexture* viewerTexture = obj->getBakedTextureForMagicId(mImageAssetID);
					texture = viewerTexture ? dynamic_cast<LLViewerFetchedTexture*>(viewerTexture) : NULL;
				}
			}
			if (texture.isNull())
			{
				texture = LLViewerTextureManager::getFetchedTexture(mImageAssetID);
			}
			mTexturep = texture;
			mTexturep->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
		}
		else if (!mFallbackImageName.empty())
		{
			mTexturep = LLViewerTextureManager::getFetchedTextureFromFile(mFallbackImageName, FTT_LOCAL_FILE, MIPMAP_YES, LLGLTexture::BOOST_PREVIEW);
		}
		if (mTentativeLabel)
		{
			mTentativeLabel->setVisible( FALSE  );
		}
		getChildView("Default")->setEnabled(mImageAssetID != mOwner->getDefaultImageAssetID());
		getChildView("Blank")->setEnabled(mImageAssetID != mWhiteImageAssetID);
		getChildView("None")->setEnabled(mOwner->getAllowNoTexture() && !mImageAssetID.isNull() );
		getChildView("Invisible")->setEnabled(mOwner->getAllowInvisibleTexture() && mImageAssetID != mInvisibleImageAssetID);
		getChildView("Alpha")->setEnabled(mImageAssetID != mAlphaImageAssetID);
		LLFloater::draw();
		if( isMinimized() )
		{
			return;
		}
		const S32 preview_left = LLFLOATER_CONTENT_PAD;
		const S32 preview_right = preview_left + PREVIEW_WIDTH;
		const S32 preview_top = getRect().getHeight() - LLFLOATER_HEADER_SIZE - 4;
		const S32 preview_bottom = preview_top - PREVIEW_HEIGHT;
		LLRect border(preview_left, preview_top, preview_right, preview_bottom);
		gl_rect_2d( border, LLColor4::black, FALSE );
		LLRect interior = border;
		interior.stretch( -1 );
		if( mTexturep )
		{
			if( mTexturep->getComponents() == 4 )
			{
				gl_rect_2d_checkerboard( calcScreenRect(), interior );
			}
			gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep );
			mTexturep->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
			if( mOwner->getTentative() && !mIsDirty )
			{
				mTentativeLabel->setVisible( TRUE );
				drawChild(mTentativeLabel);
			}
		}
		else
		{
			gl_rect_2d( interior, LLColor4::grey, TRUE );
			gl_draw_x(interior, LLColor4::black );
		}
		if (mSelectedItemPinned) return;
		LLFolderView* folder_view = mInventoryPanel->getRootFolder();
		if (!folder_view) return;
		LLInventoryFilter& filter = folder_view->getFilter();
		bool is_filter_active = folder_view->getCompletedFilterGeneration() < filter.getCurrentGeneration() &&
				filter.isNotDefault();
		if (!is_filter_active && !mSelectedItemPinned)
		{
			folder_view->setPinningSelectedItem(mSelectedItemPinned);
			folder_view->dirtyFilter();
			folder_view->arrangeFromRoot();
			mSelectedItemPinned = TRUE;
		}
	}
}
const LLUUID& LLFloaterTexturePicker::findItemID(const LLUUID& asset_id, BOOL copyable_only)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLAssetIDMatches asset_id_matches(asset_id);
	gInventory.collectDescendentsIf(LLUUID::null,
							cats,
							items,
							LLInventoryModel::INCLUDE_TRASH,
							asset_id_matches);
	if (items.size())
	{
		for (U32 i = 0; i < items.size(); i++)
		{
			LLInventoryItem* itemp = items[i];
			LLPermissions item_permissions = itemp->getPermissions();
			if (item_permissions.allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
			{
				return itemp->getUUID();
			}
		}
		if (copyable_only)
		{
			return LLUUID::null;
		}
		else
		{
			return items[0]->getUUID();
		}
	}
	return LLUUID::null;
}
PermissionMask LLFloaterTexturePicker::getFilterPermMask()
{
	bool apply_immediate = getChild<LLUICtrl>("apply_immediate_check")->getValue().asBoolean();
	return apply_immediate ? mImmediateFilterPermMask : mNonImmediateFilterPermMask;
}
void LLFloaterTexturePicker::commitIfImmediateSet()
{
	if (!mNoCopyTextureSelected && mOwner && mCanApply && mCanPreview)
	{
		mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_CHANGE);
	}
}
void LLFloaterTexturePicker::onBtnSetToDefault(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setCanApply(true, true);
	if (self->mOwner)
	{
		self->setImageID( self->mOwner->getDefaultImageAssetID() );
	}
	self->commitIfImmediateSet();
}
void LLFloaterTexturePicker::onBtnWhite(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setCanApply(true, true);
	self->setImageID( self->mWhiteImageAssetID );
	self->commitIfImmediateSet();
}
void LLFloaterTexturePicker::onBtnNone(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setCanApply(true, true);
	self->setImageID( LLUUID::null );
	self->commitIfImmediateSet();
}
void LLFloaterTexturePicker::onBtnInvisible(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setCanApply(true, true);
	self->setImageID(self->mInvisibleImageAssetID);
	self->commitIfImmediateSet();
}
void LLFloaterTexturePicker::onBtnAlpha(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setCanApply(true, true);
	self->setImageID(self->mAlphaImageAssetID);
	self->commitIfImmediateSet();
}
void LLFloaterTexturePicker::onBtnCancel(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setImageID( self->mOriginalImageAssetID );
	if (self->mOwner)
	{
		self->mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_CANCEL);
	}
	self->mIsDirty = FALSE;
	self->close();
}
void LLFloaterTexturePicker::onBtnSelect(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	if (self->mOwner)
	{
		self->mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_SELECT);
	}
	self->close();
}
void LLFloaterTexturePicker::onBtnAdd(void *userdata)
{
	LocalAssetBrowser::AddBitmap();
}
void LLFloaterTexturePicker::onBtnRemove(void *userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	LocalAssetBrowser::DelBitmap( self->mLocalScrollCtrl->getAllSelected(), LOCALLIST_COL_ID );
}
void LLFloaterTexturePicker::onBtnBrowser(void *userdata)
{
	FloaterLocalAssetBrowser::show(NULL);
}
void LLFloaterTexturePicker::onLocalScrollCommit()
{
	LLUUID id(mLocalScrollCtrl->getStringUUIDSelectedItem());
	mOwner->setImageAssetID(id);
	if (childGetValue("apply_immediate_check").asBoolean())
		mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_CHANGE, id);
}
void LLFloaterTexturePicker::onBtnPipette()
{
	BOOL pipette_active = getChild<LLUICtrl>("Pipette")->getValue().asBoolean();
	pipette_active = !pipette_active;
	if (pipette_active)
	{
		LLToolMgr::getInstance()->setTransientTool(LLToolPipette::getInstance());
	}
	else
	{
		LLToolMgr::getInstance()->clearTransientTool();
	}
}
void LLFloaterTexturePicker::onBtnUUID( void* userdata )
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	if ( self)
	{
		std::string texture_uuid = self->childGetValue("texture_uuid").asString();
		if (texture_uuid.length() == 36)
		{
			self->setImageID( LLUUID(texture_uuid) );
			self->mIsDirty = TRUE;
			self->commitIfImmediateSet();
		}
	}
}
void LLFloaterTexturePicker::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	if (items.size())
	{
		LLFolderViewItem* first_item = items.front();
		LLInventoryItem* itemp = gInventory.getItem(first_item->getListener()->getUUID());
		mNoCopyTextureSelected = FALSE;
		if (itemp)
		{
			if (!mTextureSelectedCallback.empty())
			{
				mTextureSelectedCallback(itemp);
			}
			if (itemp->getPermissions().getMaskOwner() & PERM_ALL)
				childSetValue("texture_uuid", mImageAssetID);
			else
				childSetValue("texture_uuid", LLUUID::null.asString());
			if (!itemp->getPermissions().allowCopyBy(gAgent.getID()))
			{
				mNoCopyTextureSelected = TRUE;
			}
			setCanApply(true, true);
			mImageAssetID = itemp->getAssetUUID();
			mIsDirty = TRUE;
			if (user_action && mCanPreview)
			{
				commitIfImmediateSet();
			}
		}
	}
}
void LLFloaterTexturePicker::onShowFolders(LLUICtrl* ctrl, void *user_data)
{
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	LLFloaterTexturePicker* picker = (LLFloaterTexturePicker*)user_data;
	if (check_box->get())
	{
		picker->mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	}
	else
	{
		picker->mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NO_FOLDERS);
	}
}
void LLFloaterTexturePicker::onApplyImmediateCheck(LLUICtrl* ctrl, void *user_data)
{
	LLFloaterTexturePicker* picker = (LLFloaterTexturePicker*)user_data;
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	gSavedSettings.setBOOL("ApplyTextureImmediately", check_box->get());
	picker->setCanApply(true, true);
	picker->updateFilterPermMask();
	picker->commitIfImmediateSet();
}
void LLFloaterTexturePicker::onBakeTextureSelect(const LLSD& val)
{
	LLUUID imageID = val.asUUID();
	setImageID(imageID);
	if (mCanPreview)
	{
		commitIfImmediateSet();
	}
}
void LLFloaterTexturePicker::updateFilterPermMask()
{
}
void LLFloaterTexturePicker::setCanApply(bool can_preview, bool can_apply)
{
	getChildRef<LLUICtrl>("Select").setEnabled(can_apply);
	getChildRef<LLUICtrl>("preview_disabled").setVisible(!can_preview);
	getChildRef<LLUICtrl>("apply_immediate_check").setVisible(can_preview);
	mCanApply = can_apply;
	mCanPreview = can_preview ? gSavedSettings.getBOOL("ApplyTextureImmediately") : false;
}
void LLFloaterTexturePicker::onFilterEdit(const std::string& search_string )
{
	if (!mInventoryPanel)
	{
		return;
	}
	mInventoryPanel->setFilterSubString(search_string);
}
void LLFloaterTexturePicker::onTextureSelect( const LLTextureEntry& te )
{
	LLUUID inventory_item_id = findItemID(te.getID(), TRUE);
	if (inventory_item_id.notNull())
	{
		LLToolPipette::getInstance()->setResult(TRUE, "");
		setCanApply(true, true);
		setImageID(te.getID());
		mNoCopyTextureSelected = FALSE;
		LLInventoryItem* itemp = gInventory.getItem(inventory_item_id);
		if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
		{
			mNoCopyTextureSelected = TRUE;
		}
		else
		{
			childSetValue("texture_uuid", inventory_item_id.asString());
		}
		commitIfImmediateSet();
	}
	else
	{
		LLToolPipette::getInstance()->setResult(FALSE, LLTrans::getString("InventoryNoTexture"));
	}
}
static LLRegisterWidget<LLTextureCtrl> r("texture_picker");
LLTextureCtrl::LLTextureCtrl(
	const std::string& name,
	const LLRect &rect,
	const std::string& label,
	const LLUUID &image_id,
	const LLUUID &default_image_id,
	const std::string& default_image_name )
	:
	LLUICtrl(name, rect, TRUE, NULL, FOLLOWS_LEFT | FOLLOWS_TOP),
	mDragCallback(NULL),
	mDropCallback(NULL),
	mOnCancelCallback(NULL),
	mOnCloseCallback(NULL),
	mOnSelectCallback(NULL),
	mBorderColor( gColors.getColor("DefaultHighlightLight") ),
	mImageAssetID( image_id ),
	mDefaultImageAssetID( default_image_id ),
	mDefaultImageName( default_image_name ),
	mLabel( label ),
	mAllowNoTexture( FALSE ),
	mAllowInvisibleTexture(FALSE),
	mImmediateFilterPermMask( PERM_NONE ),
	mNonImmediateFilterPermMask( PERM_NONE ),
	mCanApplyImmediately( FALSE ),
	mNeedsRawImageData( FALSE ),
	mValid( TRUE ),
	mDirty( FALSE ),
	mShowLoadingPlaceholder( TRUE )
{
	mCaption = new LLTextBox( label,
		LLRect( 0, BTN_HEIGHT_SMALL, getRect().getWidth(), 0 ),
		label,
		LLFontGL::getFontSansSerifSmall() );
	mCaption->setFollows( FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM );
	addChild( mCaption );
	S32 image_top = getRect().getHeight();
	S32 image_bottom = BTN_HEIGHT_SMALL;
	S32 image_middle = (image_top + image_bottom) / 2;
	S32 line_height = ll_round(LLFontGL::getFontSansSerifSmall()->getLineHeight());
	mTentativeLabel = new LLTextBox( std::string("Multiple"),
		LLRect(
			0, image_middle + line_height / 2,
			getRect().getWidth(), image_middle - line_height / 2 ),
		LLTrans::getString("multiple_textures"),
		LLFontGL::getFontSansSerifSmall() );
	mTentativeLabel->setHAlign( LLFontGL::HCENTER );
	mTentativeLabel->setFollowsAll();
	mTentativeLabel->setMouseOpaque(FALSE);
	addChild( mTentativeLabel );
	LLRect border_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	border_rect.mBottom += BTN_HEIGHT_SMALL;
	mBorder = new LLViewBorder(std::string("border"), border_rect, LLViewBorder::BEVEL_IN);
	mBorder->setFollowsAll();
	addChild(mBorder);
	setEnabled(TRUE);
	mLoadingPlaceholderString = LLTrans::getString("texture_loading");
}
LLTextureCtrl::~LLTextureCtrl()
{
	closeFloater();
}
LLXMLNodePtr LLTextureCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();
	node->setName(LL_TEXTURE_CTRL_TAG);
	node->createChild("label", TRUE)->setStringValue(getLabel());
	node->createChild("default_image_name", TRUE)->setStringValue(getDefaultImageName());
	node->createChild("allow_no_texture", TRUE)->setBoolValue(mAllowNoTexture);
	node->createChild("allow_invisible_texture", TRUE)->setBoolValue(mAllowInvisibleTexture);
	node->createChild("can_apply_immediately", TRUE)->setBoolValue(mCanApplyImmediately );
	return node;
}
LLView* LLTextureCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLRect rect;
	createRect(node, rect, parent);
	std::string label;
	node->getAttributeString("label", label);
	std::string image_id("");
	node->getAttributeString("image", image_id);
	std::string default_image_id("");
	node->getAttributeString("default_image", default_image_id);
	std::string default_image_name("Default");
	node->getAttributeString("default_image_name", default_image_name);
	BOOL allow_no_texture = FALSE;
	node->getAttributeBOOL("allow_no_texture", allow_no_texture);
	BOOL allow_invisible_texture = FALSE;
	node->getAttributeBOOL("allow_invisible_texture", allow_invisible_texture);
	BOOL can_apply_immediately = FALSE;
	node->getAttributeBOOL("can_apply_immediately", can_apply_immediately);
	if (label.empty())
	{
		label.assign(node->getValue());
	}
	LLTextureCtrl* texture_picker = new LLTextureCtrl(
									"texture_picker",
									rect,
									label,
									LLUUID(image_id),
									LLUUID(default_image_id),
									default_image_name );
	texture_picker->setAllowNoTexture(allow_no_texture);
	texture_picker->setAllowInvisibleTexture(allow_invisible_texture);
	texture_picker->setCanApplyImmediately(can_apply_immediately);
	texture_picker->initFromXML(node, parent);
	return texture_picker;
}
void LLTextureCtrl::setShowLoadingPlaceholder(BOOL showLoadingPlaceholder)
{
	mShowLoadingPlaceholder = showLoadingPlaceholder;
}
void LLTextureCtrl::setBlankImageAssetID(const LLUUID& id)
{
	if (LLFloaterTexturePicker* floater = dynamic_cast<LLFloaterTexturePicker*>(mFloaterHandle.get()))
		floater->setWhiteImageAssetID(id);
}
const LLUUID& LLTextureCtrl::getBlankImageAssetID() const
{
	if (LLFloaterTexturePicker* floater = dynamic_cast<LLFloaterTexturePicker*>(mFloaterHandle.get()))
		return floater->getWhiteImageAssetID();
	return LLUUID::null;
}
void LLTextureCtrl::setCaption(const std::string& caption)
{
	mCaption->setText( caption );
}
void LLTextureCtrl::setCanApplyImmediately(BOOL b)
{
	mCanApplyImmediately = b;
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( floaterp )
	{
		floaterp->setCanApplyImmediately(b);
	}
}
void LLTextureCtrl::setCanApply(bool can_preview, bool can_apply)
{
	LLFloaterTexturePicker* floaterp = dynamic_cast<LLFloaterTexturePicker*>(mFloaterHandle.get());
	if( floaterp )
	{
		floaterp->setCanApply(can_preview, can_apply);
	}
}
void LLTextureCtrl::setVisible( BOOL visible )
{
	if( !visible )
	{
		closeFloater();
	}
	LLUICtrl::setVisible( visible );
}
void LLTextureCtrl::setEnabled( BOOL enabled )
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( enabled )
	{
		std::string tooltip;
		if (floaterp) tooltip = floaterp->getString("choose_picture");
		setToolTip( tooltip );
	}
	else
	{
		setToolTip( std::string() );
		closeFloater();
	}
	if( floaterp )
	{
		floaterp->setActive(enabled);
	}
	mCaption->setEnabled( enabled );
	mEnable = enabled;
}
void LLTextureCtrl::setValid(BOOL valid )
{
	mValid = valid;
	if (!valid)
	{
		LLFloaterTexturePicker* pickerp = (LLFloaterTexturePicker*)mFloaterHandle.get();
		if (pickerp)
		{
			pickerp->setActive(FALSE);
		}
	}
}
BOOL	LLTextureCtrl::isDirty() const
{
	return mDirty;
}
void	LLTextureCtrl::resetDirty()
{
	mDirty = FALSE;
}
void LLTextureCtrl::clear()
{
	setImageAssetID(LLUUID::null);
}
void LLTextureCtrl::setLabel(const std::string& label)
{
	mLabel = label;
	mCaption->setText(label);
}
void LLTextureCtrl::showPicker(BOOL take_focus)
{
	LLFloater* floaterp = mFloaterHandle.get();
	if( floaterp )
	{
		floaterp->open( );
	}
	else
	{
		if( !mLastFloaterLeftTop.mX && !mLastFloaterLeftTop.mY )
		{
			gFloaterView->getNewFloaterPosition(&mLastFloaterLeftTop.mX, &mLastFloaterLeftTop.mY);
		}
		LLRect rect = gSavedSettings.getRect("TexturePickerRect");
		rect.translate( mLastFloaterLeftTop.mX - rect.mLeft, mLastFloaterLeftTop.mY - rect.mTop );
		floaterp = new LLFloaterTexturePicker(
			this,
			rect,
			mLabel,
			mImmediateFilterPermMask,
			mDnDFilterPermMask,
			mNonImmediateFilterPermMask,
			mCanApplyImmediately,
			mFallbackImageName);
		mFloaterHandle = floaterp->getHandle();
		LLFloaterTexturePicker* texture_floaterp = dynamic_cast<LLFloaterTexturePicker*>(floaterp);
		if (texture_floaterp && mOnTextureSelectedCallback)
		{
			texture_floaterp->setTextureSelectedCallback(mOnTextureSelectedCallback);
		}
		gFloaterView->getParentFloater(this)->addDependentFloater(floaterp);
		floaterp->open();
	}
	if (take_focus)
	{
		floaterp->setFocus(TRUE);
	}
}
void LLTextureCtrl::closeFloater()
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( floaterp )
	{
		floaterp->setOwner(NULL);
		floaterp->close();
	}
}
BOOL LLTextureCtrl::handleHover(S32 x, S32 y, MASK mask)
{
	getWindow()->setCursor(mBorder->parentPointInView(x,y) ? UI_CURSOR_HAND : UI_CURSOR_ARROW);
	return TRUE;
}
BOOL LLTextureCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if(!mEnable) return FALSE;
	const auto clicked_picture = mBorder->parentPointInView(x, y);
	BOOL handled = (mCaption->getText().empty() || clicked_picture) && LLUICtrl::handleMouseDown( x, y , mask );
	if (!handled && clicked_picture)
	{
		showPicker(FALSE);
		LLInventoryModelBackgroundFetch::instance().start(gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE));
		LLInventoryModelBackgroundFetch::instance().start();
		handled = TRUE;
	}
	return handled;
}
void LLTextureCtrl::onFloaterClose()
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if (floaterp)
	{
		if (mOnCloseCallback)
		{
			mOnCloseCallback(this,LLSD());
		}
		floaterp->setOwner(NULL);
		mLastFloaterLeftTop.set( floaterp->getRect().mLeft, floaterp->getRect().mTop );
	}
	mFloaterHandle.markDead();
}
void LLTextureCtrl::onFloaterCommit(ETexturePickOp op)
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( floaterp && mEnable)
	{
		mDirty = (op != TEXTURE_CANCEL);
		if( floaterp->isDirty() )
		{
			setTentative( FALSE );
			mImageItemID = floaterp->findItemID(floaterp->getAssetID(), FALSE);
			LL_DEBUGS() << "mImageItemID: " << mImageItemID << LL_ENDL;
			mImageAssetID = floaterp->getAssetID();
			LL_DEBUGS() << "mImageAssetID: " << mImageAssetID << LL_ENDL;
			if (op == TEXTURE_SELECT && mOnSelectCallback)
			{
				mOnSelectCallback( this, LLSD() );
			}
			else if (op == TEXTURE_CANCEL && mOnCancelCallback)
			{
				mOnCancelCallback( this, LLSD() );
			}
			else
			{
				onCommit();
			}
		}
	}
}
void LLTextureCtrl::onFloaterCommit(ETexturePickOp op, LLUUID id)
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( floaterp && getEnabled())
	{
		mImageItemID = id;
		mImageAssetID = id;
		if (op == TEXTURE_SELECT && mOnSelectCallback)
		{
			mOnSelectCallback(this, LLSD());
		}
		else if (op == TEXTURE_CANCEL && mOnCancelCallback)
		{
			mOnCancelCallback(this, LLSD());
		}
		else
		{
			onCommit();
		}
	}
}
void LLTextureCtrl::setOnTextureSelectedCallback(texture_selected_callback cb)
{
	mOnTextureSelectedCallback = cb;
	LLFloaterTexturePicker* floaterp = dynamic_cast<LLFloaterTexturePicker*>(mFloaterHandle.get());
	if (floaterp)
	{
		floaterp->setTextureSelectedCallback(cb);
	}
}
void LLTextureCtrl::setImageAssetID( const LLUUID& asset_id )
{
	if( mImageAssetID != asset_id )
	{
		mImageItemID.setNull();
		mImageAssetID = asset_id;
		LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
		if( floaterp && mEnable )
		{
			floaterp->setImageID( asset_id );
			floaterp->setDirty( FALSE );
		}
	}
}
BOOL LLTextureCtrl::handleDragAndDrop(S32 x, S32 y, MASK mask,
					  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
					  EAcceptance *accept,
					  std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	LLInventoryItem* item = (LLInventoryItem*)cargo_data;
	if (mEnable && (cargo_type == DAD_TEXTURE) && allowDrop(item))
	{
		if (drop)
		{
			if(doDrop(item))
			{
				setTentative( FALSE );
				onCommit();
			}
		}
		*accept = ACCEPT_YES_SINGLE;
	}
	else
	{
		*accept = ACCEPT_NO;
	}
	handled = TRUE;
	LL_DEBUGS("UserInput") << "dragAndDrop handled by LLTextureCtrl " << getName() << LL_ENDL;
	return handled;
}
void LLTextureCtrl::draw()
{
	mBorder->setKeyboardFocusHighlight(hasFocus());
	if (!mValid)
	{
		mTexturep = nullptr;
	}
	else if (!mImageAssetID.isNull())
	{
		LLPointer<LLViewerFetchedTexture> texture = NULL;
		if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
		{
			LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
			if (obj)
			{
				LLViewerTexture* viewerTexture = obj->getBakedTextureForMagicId(mImageAssetID);
				texture = viewerTexture ? dynamic_cast<LLViewerFetchedTexture*>(viewerTexture) : NULL;
			}
		}
		if (texture.isNull())
		{
			texture = LLViewerTextureManager::getFetchedTexture(mImageAssetID, FTT_DEFAULT, MIPMAP_YES, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
		}
		texture->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
		texture->forceToSaveRawImage(0) ;
		mTexturep = texture;
	}
	else if (!mFallbackImageName.empty())
	{
		mTexturep =	LLViewerTextureManager::getFetchedTextureFromFile(mFallbackImageName, FTT_LOCAL_FILE, MIPMAP_YES,LLGLTexture::BOOST_PREVIEW, LLViewerTexture::LOD_TEXTURE);
	}
	else
	{
		mTexturep = nullptr;
	}
	LLRect border( 0, getRect().getHeight(), getRect().getWidth(), BTN_HEIGHT_SMALL );
	gl_rect_2d( border, mBorderColor, FALSE );
	LLRect interior = border;
	interior.stretch( -1 );
	if( mTexturep )
	{
		if( mTexturep->getComponents() == 4 )
		{
			gl_rect_2d_checkerboard( calcScreenRect(), interior );
		}
		gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep);
		mTexturep->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
	}
	else
	{
		gl_rect_2d( interior, LLColor4::grey, TRUE );
		gl_draw_x( interior, LLColor4::black );
	}
	mTentativeLabel->setVisible( !mTexturep.isNull() && getTentative() );
	if (mTexturep.notNull() &&
		(!mTexturep->isFullyLoaded()) &&
		(mShowLoadingPlaceholder == TRUE))
	{
		LLFontGL* font = LLFontGL::getFontSansSerifBig();
		font->renderUTF8(
			mLoadingPlaceholderString, 0,
			llfloor(interior.mLeft+10),
			llfloor(interior.mTop-20),
			LLColor4::white,
			LLFontGL::LEFT,
			LLFontGL::BASELINE,
			LLFontGL::NORMAL,
			LLFontGL::DROP_SHADOW);
	}
	LLUICtrl::draw();
}
BOOL LLTextureCtrl::allowDrop(LLInventoryItem* item)
{
	BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
	BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
	BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
														gAgent.getID());
	PermissionMask item_perm_mask = 0;
	if (copy) item_perm_mask |= PERM_COPY;
	if (mod)  item_perm_mask |= PERM_MODIFY;
	if (xfer) item_perm_mask |= PERM_TRANSFER;
	PermissionMask filter_perm_mask = mImmediateFilterPermMask;
	if ( (item_perm_mask & filter_perm_mask) == filter_perm_mask )
	{
		if(mDragCallback)
		{
			return mDragCallback(this, item);
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}
BOOL LLTextureCtrl::doDrop(LLInventoryItem* item)
{
	if(mDropCallback)
	{
		return mDropCallback(this, item);
	}
	setImageAssetID( item->getAssetUUID() );
	mImageItemID = item->getUUID();
	mDirty = true;
	return TRUE;
}
BOOL LLTextureCtrl::handleUnicodeCharHere(llwchar uni_char)
{
	if( ' ' == uni_char )
	{
		showPicker(TRUE);
		return TRUE;
	}
	return LLUICtrl::handleUnicodeCharHere(uni_char);
}
void LLTextureCtrl::setValue( const LLSD& value )
{
	setImageAssetID(value.asUUID());
}
LLSD LLTextureCtrl::getValue() const
{
	return LLSD(getImageAssetID());
}
