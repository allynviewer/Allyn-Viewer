/** 
 * @file llfloatertest.cpp
 * @author James Cook
 * @brief Torture-test floater for all UI elements
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "llfloatertest.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloater.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llradiogroup.h"
#include "llscrolllistctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "llviewborder.h"
#include "llnamelistctrl.h"
#include "lluictrlfactory.h"
class LLFloaterTestImpl : public LLFloater
{
public:
	LLFloaterTestImpl();
	BOOL postBuild();
private:
	static void onClickButton();
	static void onClickText();
	static void onClickTab();
	static void onCommitCheck();
	static void onCommitCombo(LLUICtrl* ctrl, const LLSD& value);
	static void onCommitLine();
	static void onKeyLine();
	static void onFocusLostLine();
	static void onChangeRadioGroup();
	static void onCommitList();
	LLButton* mBtnSimple;
	LLButton* mBtnUnicode;
	LLButton* mBtnImages;
	LLCheckBoxCtrl* mCheckSimple;
	LLCheckBoxCtrl* mCheckUnicode;
	LLComboBox* mCombo;
	LLIconCtrl* mIcon;
	LLLineEditor* mLineEditor;
	LLRadioGroup* mRadioGroup;
	LLScrollListCtrl* mScrollList;
	LLTabContainer* mTab;
	LLTextEditor* mTextEditor;
	LLViewBorder* mViewBorder;
	LLNameListCtrl* mNameList;
};
LLFloaterTestImpl::LLFloaterTestImpl()
:	LLFloater(std::string("test"))
,	mBtnSimple(NULL)
,	mBtnUnicode(NULL)
,	mBtnImages(NULL)
,	mCheckSimple(NULL)
,	mCheckUnicode(NULL)
,	mCombo(NULL)
,	mIcon(NULL)
,	mLineEditor(NULL)
,	mRadioGroup(NULL)
,	mScrollList(NULL)
,	mTab(NULL)
,	mTextEditor(NULL)
,	mViewBorder(NULL)
,	mNameList(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_test.xml");
}
BOOL LLFloaterTestImpl::postBuild()
{
	mTab = getChild<LLTabContainer>("test_tab");
	mTab->setCommitCallback(boost::bind(&LLFloaterTestImpl::onClickTab));
	mBtnSimple = getChild<LLButton>("Simple Button");
	mBtnUnicode = getChild<LLButton>("unicode_btn");
	mBtnImages = getChild<LLButton>("image_btn");
	mCheckSimple = getChild<LLCheckBoxCtrl>("simple_check");
	mCheckUnicode = getChild<LLCheckBoxCtrl>("unicode_check");
	mCombo = getChild<LLComboBox>("combo");
	mIcon = getChild<LLIconCtrl>("test_icon");
	mLineEditor = getChild<LLLineEditor>("test_line");
	mRadioGroup = getChild<LLRadioGroup>("radio_group");
	mScrollList = getChild<LLScrollListCtrl>("test_scroll_list");
	mTextEditor = getChild<LLTextEditor>("test_text_editor");
	mViewBorder = getChild<LLViewBorder>("test_view_border");
	mNameList = getChild<LLNameListCtrl>("test_name_list");
	getChild<LLButton>("can click")->setClickedCallback(boost::bind(&LLFloaterTestImpl::onClickButton));
	mBtnSimple->setClickedCallback(boost::bind(&LLFloaterTestImpl::onClickButton));
	mBtnUnicode->setClickedCallback(boost::bind(&LLFloaterTestImpl::onClickButton));
	mBtnImages->setClickedCallback(boost::bind(&LLFloaterTestImpl::onClickButton));
	getChild<LLTextBox>("simple_text")->setClickedCallback(boost::bind(&LLFloaterTestImpl::onClickText));
	mCheckSimple->setCommitCallback(boost::bind(&LLFloaterTestImpl::onCommitCheck));
	mCheckUnicode->setCommitCallback(boost::bind(&LLFloaterTestImpl::onCommitCheck));
	mCombo->setCommitCallback(boost::bind(&LLFloaterTestImpl::onCommitCombo, _1, _2));
	mRadioGroup->setCommitCallback(boost::bind(&LLFloaterTestImpl::onChangeRadioGroup));
	mScrollList->setCommitCallback(boost::bind(&LLFloaterTestImpl::onCommitList));
	mLineEditor->setCommitCallback(boost::bind(&LLFloaterTestImpl::onCommitLine));
	mLineEditor->setKeystrokeCallback(boost::bind(&LLFloaterTestImpl::onKeyLine));
	mLineEditor->setFocusLostCallback(boost::bind(&LLFloaterTestImpl::onFocusLostLine));
	if (mCombo && mCombo->mList)
	{
		LLScrollListItem* disabled = mCombo->mList->getItem(LLSD("disabled"));
		if (disabled)
		{
			disabled->setEnabled(FALSE);
		}
	}
	static const char* kRows[][2] =
	{
		{ "Button", "Click handlers log to the console" },
		{ "Checkbox", "Simple checkbox writes UIFloaterTestBool" },
		{ "Combo box", "Includes a disabled item" },
		{ "Radio", "Three mutually exclusive choices" },
		{ "Line editor", "Commit, keystroke and focus-lost callbacks" },
		{ "Slider", "Numeric value with editable text" },
		{ "Name list", "Avatar name list control" },
		{ "Text editor", "Word-wrapped multi-line field" },
		{ "Icon", "object_cone.tga" },
		{ "Image button", "tool_zoom.tga / tool_zoom_active.tga" }
	};
	for (size_t i = 0; i < LL_ARRAY_SIZE(kRows); ++i)
	{
		LLSD row;
		row["columns"][0]["column"] = "widget";
		row["columns"][0]["value"] = kRows[i][0];
		row["columns"][1]["column"] = "notes";
		row["columns"][1]["value"] = kRows[i][1];
		mScrollList->addElement(row);
	}
	mNameList->setCommentText(std::string("Drop a calling card here"));
	center();
	return TRUE;
}
void LLFloaterTestImpl::onClickButton()
{
	LL_INFOS() << "button clicked" << LL_ENDL;
}
void LLFloaterTestImpl::onClickText()
{
	LL_INFOS() << "text clicked" << LL_ENDL;
}
void LLFloaterTestImpl::onClickTab()
{
	LL_INFOS() << "click tab" << LL_ENDL;
}
void LLFloaterTestImpl::onCommitCheck()
{
	LL_INFOS() << "commit check" << LL_ENDL;
}
void LLFloaterTestImpl::onCommitCombo(LLUICtrl* ctrl, const LLSD& value)
{
	LLComboBox* combo = (LLComboBox*)ctrl;
	LL_INFOS() << "commit combo name " << combo->getSimple() << " value " << value.asString() << LL_ENDL;
}
void LLFloaterTestImpl::onCommitLine()
{
	LL_INFOS() << "commit line editor" << LL_ENDL;
}
void LLFloaterTestImpl::onKeyLine()
{
	LL_INFOS() << "keystroke line editor" << LL_ENDL;
}
void LLFloaterTestImpl::onFocusLostLine()
{
	LL_INFOS() << "focus lost line editor" << LL_ENDL;
}
void LLFloaterTestImpl::onChangeRadioGroup()
{
	LL_INFOS() << "change radio group" << LL_ENDL;
}
void LLFloaterTestImpl::onCommitList()
{
	LL_INFOS() << "commit scroll list" << LL_ENDL;
}
void LLFloaterTest::show(void*)
{
	new LLFloaterTest();
}
LLFloaterTest::LLFloaterTest()
:	impl(* new LLFloaterTestImpl)
{
}
LLFloaterTest::~LLFloaterTest()
{
	delete &impl;
}
LLFloaterSimple::LLFloaterSimple(const std::string& xml_filename)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, xml_filename);
}
