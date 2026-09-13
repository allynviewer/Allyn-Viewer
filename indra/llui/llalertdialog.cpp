/* @file llalertdialog.cpp
 * @brief LLAlertDialog base class
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
#include "linden_common.h"
#include "llboost.h"
#include "llalertdialog.h"
#include "llfontgl.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llkeyboard.h"
#include "llfocusmgr.h"
#include "lliconctrl.h"
#include "llui.h"
#include "llxmlnode.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "llnotifications.h"
#include "llfunctorregistry.h"
#include "lldraghandle.h"
const S32 MAX_ALLOWED_MSG_WIDTH = 400;
const F32 DEFAULT_BUTTON_DELAY = 0.5f;
const S32 MSG_PAD = 8;
LLControlGroup* LLAlertDialog::sSettings = NULL;
LLAlertDialog::URLLoader* LLAlertDialog::sURLLoader;
void LLAlertDialog::initClass()
{
	LLNotificationChannel::buildChannel("Alerts", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alert"));
	LLNotificationChannel::buildChannel("AlertModal", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alertmodal"));
	LLNotifications::instance().getChannel("Alerts")->connectChanged(boost::bind(&onNewNotification, _1, false));
	LLNotifications::instance().getChannel("AlertModal")->connectChanged(boost::bind(&onNewNotification, _1, true));
}
bool LLAlertDialog::onNewNotification(const LLSD& notify, bool is_modal)
{
	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
	if(notification)
	{
		if (notify["sigtype"].asString() == "add" || notify["sigtype"].asString() == "load")
		{
			LLAlertDialog* dialog = new LLAlertDialog(notification, is_modal);
			dialog->show();
		}
		else if (notify["sigtype"].asString() == "change")
		{
			LLAlertDialog* dialog = getInstance(notification->getID());
			if (dialog)
			{
				dialog->show();
			}
			else
			{
				LLAlertDialog* dialog = new LLAlertDialog(notification, is_modal);
				dialog->show();
			}
		}
	}
	return false;
}
static const S32 VPAD = 16;
static const S32 HPAD = 25;
static const S32 BTN_HPAD = 8;
static const LLFONT_ID FONT_NAME = LLFONT_SANSSERIF;
LLAlertDialog::LLAlertDialog( LLNotificationPtr notification, bool modal)
	:	LLModalDialog( notification->getLabel(), 100, 100, modal ),
		LLInstanceTracker<LLAlertDialog, LLUUID>(notification->getID()),
		mDefaultButton( NULL ),
		mCheck(NULL),
		mCaution(notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH),
		mLabel(notification->getName()),
		mLineEditor(NULL),
		mNote(notification)
{
	const LLFontGL* font = LLResMgr::getInstance()->getRes( FONT_NAME );
	const S32 LINE_HEIGHT = llfloor(font->getLineHeight() + 0.99f);
	const S32 EDITOR_HEIGHT = 20;
	LLNotificationFormPtr form = mNote->getForm();
	std::string edit_text_name;
	std::string edit_text_contents;
	bool is_password = false;
	setBackgroundVisible(TRUE);
	setBackgroundOpaque(TRUE);
	typedef std::list<ButtonData> options_t;
	options_t options;
	LLSD form_sd = form->asLLSD();
	for (LLSD::array_const_iterator it = form_sd.beginArray(); it != form_sd.endArray(); ++it)
	{
		std::string type = (*it)["type"].asString();
		if (type == "button")
		{
			options.push_back(ButtonData());
			ButtonData& button_data = options.back();
			button_data.mName = (*it)["name"].asString();
			button_data.mText = (*it)["text"].asString();
			button_data.mDefault = (*it)["default"].asBoolean();
			if(options.size()-1 == mNote->getURLOption())
				button_data.mUrl = mNote->getURL();
		}
		else if (type == "text")
		{
			edit_text_contents = (*it)["value"].asString();
			edit_text_name = (*it)["name"].asString();
		}
		else if (type == "password")
		{
			edit_text_contents = (*it)["value"].asString();
			edit_text_name = (*it)["name"].asString();
			is_password = true;
		}
	}
	if (options.empty())
	{
		options.push_back(ButtonData());
		ButtonData& button_data = options.back();
		button_data.mName = "close";
		button_data.mText = "Close";
		button_data.mDefault = true;
	}
	S32 num_options = options.size();
	S32 button_width = 0;
	S32 sp = font->getWidth(std::string("OO"));
	for( options_t::iterator it = options.begin(); it != options.end(); it++ )
	{
		S32 w = S32(font->getWidth( it->mText ) + 0.99f) + sp + 2 * LLBUTTON_H_PAD;
		button_width = llmax( w, button_width );
	}
	S32 btn_total_width = button_width;
	if( num_options > 1 )
	{
		btn_total_width = (num_options * button_width) + ((num_options - 1) * BTN_HPAD);
	}
	std::string msg = mNote->getMessage();
	LL_WARNS() << "Alert: " << msg << LL_ENDL;
	LLTextBox* msg_box = new LLTextBox( std::string("Alert message"), msg, (F32)MAX_ALLOWED_MSG_WIDTH, font );
	const LLRect& text_rect = msg_box->getRect();
	S32 dialog_width = llmax( btn_total_width, text_rect.getWidth() ) + 2 * HPAD;
	S32 dialog_height = text_rect.getHeight() + 3 * VPAD + BTN_HEIGHT;
	if (hasTitleBar())
	{
		dialog_height += LINE_HEIGHT;
	}
	if (!edit_text_name.empty())
	{
		dialog_height += EDITOR_HEIGHT + VPAD;
		dialog_width = llmax(dialog_width, (S32)(font->getWidth( edit_text_contents ) + 0.99f));
	}
	if (mCaution)
	{
		dialog_width += 32 + HPAD;
	}
	reshape( dialog_width, dialog_height, FALSE );
	S32 msg_y = getRect().getHeight() - VPAD;
	S32 msg_x = HPAD;
	if (hasTitleBar())
	{
		msg_y -= LINE_HEIGHT;
	}
	if (mCaution)
	{
		LLIconCtrl* icon = new LLIconCtrl(std::string("icon"), LLRect(msg_x, msg_y, msg_x+32, msg_y-32), std::string("notify_caution_icon.tga"));
		icon->setMouseOpaque(FALSE);
		addChild(icon);
		msg_x += 32 + HPAD;
		msg_box->setColor( LLUI::sColorsGroup->getColor( "AlertCautionTextColor" ) );
	}
	else
	{
		msg_box->setColor( LLUI::sColorsGroup->getColor( "AlertTextColor" ) );
	}
	LLDragHandle* handle = getDragHandle();
	if (handle)
	{
		getDragHandle()->setTextColor(LLUI::sColorsGroup->getColor(mCaution ? "AlertCautionTextColor" : "AlertTextColor"));
	}
	LLRect rect;
	rect.setLeftTopAndSize( msg_x, msg_y, text_rect.getWidth(), text_rect.getHeight() );
	msg_box->setRect( rect );
	addChild(msg_box);
	S32 button_left = (getRect().getWidth() - btn_total_width) / 2;
	for( options_t::iterator it = options.begin(); it != options.end(); it++ )
	{
		LLRect button_rect;
		button_rect.setOriginAndSize( button_left, VPAD, button_width, BTN_HEIGHT );
		ButtonData& button_data = *it;
		LLButton* btn = new LLButton(
			button_data.mName, button_rect,
			"","", "",
			NULL,
			font,
			button_data.mText,
			button_data.mText);
		btn->setClickedCallback(boost::bind(&LLAlertDialog::onButtonPressed, this, _1, button_data.mUrl));
		if (mCaution)
		{
			btn->setColor(LLUI::sColorsGroup->getColor("ButtonCautionImageColor"));
		}
		addChild(btn);
		if(!mDefaultButton || button_data.mDefault)
		{
			mDefaultButton = btn;
		}
		button_left += button_width + BTN_HPAD;
	}
	llassert(mDefaultButton);
	mDefaultButton->setFocus(TRUE);
	if (!edit_text_name.empty())
	{
		S32 y = VPAD + BTN_HEIGHT + VPAD/2;
		mLineEditor = new LLLineEditor(edit_text_name,
			LLRect( HPAD, y+EDITOR_HEIGHT, dialog_width-HPAD, y),
			edit_text_contents,
			LLFontGL::getFontSansSerif(),
			STD_STRING_STR_LEN);
		mLineEditor->setHandleEditKeysDirectly(TRUE);
		addChild(mLineEditor);
	}
	if (mLineEditor)
	{
		mLineEditor->setDrawAsterixes(is_password);
		setEditTextArgs(notification->getSubstitutions());
	}
	std::string ignore_label;
	if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE)
	{
		setCheckBox(LLNotificationTemplates::instance().getGlobalString("skipnexttime"), ignore_label);
	}
	else if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
	{
		setCheckBox(LLNotificationTemplates::instance().getGlobalString("alwayschoose"), ignore_label);
	}
}
bool LLAlertDialog::show()
{
	if (mCaution)
	{
		setBackgroundColor( LLUI::sColorsGroup->getColor( "AlertCautionBoxColor" ) );
	}
	else
	{
		setBackgroundColor( LLUI::sColorsGroup->getColor( "AlertBoxColor" ) );
	}
	startModal();
	gFloaterView->adjustToFitScreen(this, FALSE);
	open();
 	setFocus(TRUE);
	if (mLineEditor)
	{
		mLineEditor->setFocus(TRUE);
		mLineEditor->selectAll();
	}
	if(mDefaultButton)
	{
		mDefaultBtnTimer.start(DEFAULT_BUTTON_DELAY);
	}
	LLUUID context_key = mNote->getPayload()["context"].asUUID();
	LLFloaterNotificationContext* contextp = dynamic_cast<LLFloaterNotificationContext*>(LLNotificationContext::getInstance(context_key));
	if (contextp && contextp->getFloater())
	{
		contextp->getFloater()->addDependentFloater(this, FALSE);
	}
	return true;
}
bool LLAlertDialog::setCheckBox( const std::string& check_title, const std::string& check_control )
{
	const LLFontGL* font = LLResMgr::getInstance()->getRes( FONT_NAME );
	const S32 LINE_HEIGHT = llfloor(font->getLineHeight() + 0.99f);
	S32 max_msg_width = getRect().getWidth() - 2 * HPAD;
	S32 check_width = S32(font->getWidth(check_title) + 0.99f) + 16;
	max_msg_width = llmax(max_msg_width, check_width);
	S32 dialog_width = max_msg_width + 2 * HPAD;
	S32 dialog_height = getRect().getHeight();
	dialog_height += LINE_HEIGHT;
	dialog_height += LINE_HEIGHT / 2;
	reshape( dialog_width, dialog_height, FALSE );
	S32 msg_x = (getRect().getWidth() - max_msg_width) / 2;
	LLRect check_rect;
	check_rect.setOriginAndSize(msg_x, VPAD+BTN_HEIGHT+LINE_HEIGHT/2,
								max_msg_width, LINE_HEIGHT);
	mCheck = new LLCheckboxCtrl(std::string("check"), check_rect, check_title, font, boost::bind(&LLAlertDialog::onClickIgnore, this, _1));
	mCheck->setEnabledColor(LLUI::sColorsGroup->getColor( mCaution ? "AlertCautionTextColor" : "AlertTextColor"));
	if (mCaution)
	{
		mCheck->setButtonColor(LLUI::sColorsGroup->getColor("ButtonCautionImageColor"));
	}
	addChild(mCheck);
	return true;
}
void LLAlertDialog::setVisible( BOOL visible )
{
	LLModalDialog::setVisible( visible );
	if( visible )
	{
		centerOnScreen();
		make_ui_sound("UISndAlert");
	}
}
void LLAlertDialog::onClose(bool app_quitting)
{
	if(mNote.get() && !mNote->isRespondedTo() && !mNote->isIgnored())
	{
		LLButton* btn = NULL;
		bool found_cancel = false;
		for(child_list_const_iter_t it = beginChild(); it != endChild(); ++it)
		{
			LLButton* cur_btn = dynamic_cast<LLButton*>(*it);
			if(!cur_btn)
				continue;
			if(	LLStringUtil::compareInsensitive(cur_btn->getName(), "close") == 0 )
			{
				btn = cur_btn;
				break;
			}
			else if(LLStringUtil::compareInsensitive(cur_btn->getName(), "cancel") == 0 )
			{
				btn = cur_btn;
				found_cancel = true;
			}
			else if(!found_cancel && LLStringUtil::compareInsensitive(cur_btn->getName(), "ok") == 0 )
			{
				btn = cur_btn;
			}
		}
		LLSD response = mNote->getResponseTemplate();
		if(btn)
			response[btn->getName()] = true;
		else
		{
			response["Close"] = true;
		}
		mNote->respond(response);
	}
	LLModalDialog::onClose(app_quitting);
}
LLAlertDialog::~LLAlertDialog()
{
}
BOOL LLAlertDialog::hasTitleBar() const
{
	return (getCurrentTitle() != "" && getCurrentTitle() != " ")
			|| isMinimizeable()
			|| isCloseable();
}
BOOL LLAlertDialog::handleKeyHere(KEY key, MASK mask )
{
	if( KEY_RETURN == key && mask == MASK_NONE )
	{
		LLModalDialog::handleKeyHere( key, mask );
		return TRUE;
	}
	else if (KEY_RIGHT == key)
	{
		focusNextItem(FALSE);
		return TRUE;
	}
	else if (KEY_LEFT == key)
	{
		focusPrevItem(FALSE);
		return TRUE;
	}
	else if (KEY_TAB == key && mask == MASK_NONE)
	{
		focusNextItem(FALSE);
		return TRUE;
	}
	else if (KEY_TAB == key && mask == MASK_SHIFT)
	{
		focusPrevItem(FALSE);
		return TRUE;
	}
	else
	{
		return LLModalDialog::handleKeyHere( key, mask );
	}
}
void LLAlertDialog::draw()
{
	if(mDefaultBtnTimer.hasExpired() && mDefaultBtnTimer.getStarted())
	{
		mDefaultBtnTimer.stop();
		setDefaultBtn(mDefaultButton);
	}
	static LLColor4 shadow_color = LLUI::sColorsGroup->getColor("ColorDropShadow");
	static S32 shadow_lines = LLUI::sConfigGroup->getS32("DropShadowFloater");
	gl_drop_shadow( 0, getRect().getHeight(), getRect().getWidth(), 0,
		shadow_color, shadow_lines);
	LLModalDialog::draw();
}
void LLAlertDialog::setEditTextArgs(const LLSD& edit_args)
{
	if (mLineEditor)
	{
		std::string msg = mLineEditor->getText();
		mLineEditor->setText(msg);
	}
	else
	{
		LL_WARNS() << "LLAlertDialog::setEditTextArgs called on dialog with no line editor" << LL_ENDL;
	}
}
void LLAlertDialog::onButtonPressed( LLUICtrl* ctrl, const std::string url )
{
	LLSD response = mNote->getResponseTemplate();
	if (mLineEditor)
	{
		response[mLineEditor->getName()] = mLineEditor->getValue();
	}
	response[ctrl->getName()] = true;
	if (!url.empty() && sURLLoader != NULL)
	{
		sURLLoader->load(url, false);
	}
	mNote->respond(response);
	close();
}
void LLAlertDialog::onClickIgnore(LLUICtrl* ctrl)
{
	BOOL check = ctrl->getValue();
	if (mNote->getForm()->getIgnoreType() == LLNotificationForm::IGNORE_SHOW_AGAIN)
	{
		check = !check;
	}
	mNote->setIgnored(check);
}
