/**
 * @file llpreviewscript.cpp
 * @brief LLPreviewScript class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llpreviewscript.h"
#include "llassetstorage.h"
#include "llassetuploadresponders.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llcororesponder.h"
#include "lldir.h"
#include "llexternaleditor.h"
#include "statemachine/aifilepicker.h"
#include "llinventorydefines.h"
#include "llinventorymodel.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "lllivefile.h"
#include "llnotificationsutil.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llslider.h"
#include "lltextbox.h"
#include "lltooldraganddrop.h"
#include "llvfile.h"
#include "llagent.h"
#include "llmenugl.h"
#include "roles_constants.h"
#include "llfloatersearchreplace.h"
#include "llfloaterperms.h"
#include "llselectmgr.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"
#include "lluictrlfactory.h"
#include "lltrans.h"
#include "llappviewer.h"
#include "llexperiencecache.h"
#include "llfloaterexperienceprofile.h"
#include "llsdserialize.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
const std::string HELLO_LSL =
	"default\n"
	"{\n"
	"    state_entry()\n"
    "    {\n"
    "        llSay(0, \"Hello, Avatar!\");\n"
    "    }\n"
	"\n"
	"    touch_start(integer total_number)\n"
	"    {\n"
	"        llSay(0, \"Touched.\");\n"
	"    }\n"
	"}\n";
const std::string HELP_LSL_URL = "http://wiki.secondlife.com/wiki/LSL_Portal";
const std::string DEFAULT_SCRIPT_NAME = "New Script";
const std::string DEFAULT_SCRIPT_DESC = "(No Description)";
const S32 SCRIPT_BORDER = 4;
const S32 SCRIPT_PAD = 5;
const S32 SCRIPT_BUTTON_WIDTH = 128;
const S32 SCRIPT_BUTTON_HEIGHT = 24;
const S32 LINE_COLUMN_HEIGHT = 14;
const S32 SCRIPT_EDITOR_MIN_HEIGHT = 2 * SCROLLBAR_SIZE + 2 * LLPANEL_BORDER_WIDTH + 128;
const S32 SCRIPT_MIN_WIDTH =
	2 * SCRIPT_BORDER +
	2 * SCRIPT_BUTTON_WIDTH +
	SCRIPT_PAD + RESIZE_HANDLE_WIDTH +
	SCRIPT_PAD;
const S32 SCRIPT_MIN_HEIGHT =
	2 * SCRIPT_BORDER +
	3*(SCRIPT_BUTTON_HEIGHT + SCRIPT_PAD) +
	LINE_COLUMN_HEIGHT +
	SCRIPT_EDITOR_MIN_HEIGHT;
const S32 MAX_EXPORT_SIZE = 1000;
const S32 MAX_HISTORY_COUNT = 10;
const F32 LIVE_HELP_REFRESH_TIME = 1.f;
static bool have_script_upload_cap(LLUUID& object_id)
{
	LLViewerObject* object = gObjectList.findObject(object_id);
	return object && (! object->getRegion()->getCapability("UpdateScriptTask").empty());
}
class ExperienceResponder : public LLHTTPClient::ResponderWithResult
{
public:
	ExperienceResponder(const LLHandle<LLLiveLSLEditor>& parent) : mParent(parent)
	{
	}
	LLHandle<LLLiveLSLEditor> mParent;
	void httpSuccess()
	{
		LLLiveLSLEditor* parent = mParent.get();
		if (!parent)
			return;
		parent->setExperienceIds(getContent()["experience_ids"]);
	}
	char const* getName() const { return "ExperienceResponder"; }
};
class LLLiveLSLFile : public LLLiveFile
{
public:
	typedef std::function<bool (const std::string& filename)> change_callback_t;
	LLLiveLSLFile(std::string file_path, change_callback_t change_cb);
	~LLLiveLSLFile();
	void ignoreNextUpdate() { mIgnoreNextUpdate = true; }
protected:
	bool loadFile() override;
	change_callback_t	mOnChangeCallback;
	bool				mIgnoreNextUpdate;
};
LLLiveLSLFile::LLLiveLSLFile(std::string file_path, change_callback_t change_cb)
:	LLLiveFile(file_path, 1.0)
,	mOnChangeCallback(change_cb)
	,	mIgnoreNextUpdate(false)
{
	llassert(mOnChangeCallback);
}
LLLiveLSLFile::~LLLiveLSLFile()
{
	LLFile::remove(filename());
}
bool LLLiveLSLFile::loadFile()
{
	if (mIgnoreNextUpdate)
	{
		mIgnoreNextUpdate = false;
		return true;
	}
	return mOnChangeCallback(filename());
}
#if 0
class LLFloaterScriptSearch : public LLFloater
{
public:
	LLFloaterScriptSearch(LLScriptEdCore* editor_core);
	~LLFloaterScriptSearch();
	BOOL	postBuild();
	static void show(LLScriptEdCore* editor_core);
	static void onBtnSearch(void* userdata);
	void handleBtnSearch();
	static void onBtnReplace(void* userdata);
	void handleBtnReplace();
	static void onBtnReplaceAll(void* userdata);
	void handleBtnReplaceAll();
	LLScriptEdCore* getEditorCore() { return mEditorCore; }
	static LLFloaterScriptSearch* getInstance() { return sInstance; }
	virtual bool hasAccelerators() const;
	virtual BOOL handleKeyHere(KEY key, MASK mask);
private:
	LLScriptEdCore* mEditorCore;
	static LLFloaterScriptSearch*	sInstance;
protected:
	LLLineEditor*			mSearchBox;
	LLLineEditor*			mReplaceBox;
		void onSearchBoxCommit();
};
LLFloaterScriptSearch* LLFloaterScriptSearch::sInstance = NULL;
LLFloaterScriptSearch::LLFloaterScriptSearch(LLScriptEdCore* editor_core)
:	LLFloater(LLSD()),
	mSearchBox(NULL),
	mReplaceBox(NULL),
	mEditorCore(editor_core)
{
	buildFromFile("floater_script_search.xml");
	sInstance = this;
	LLView* viewp = (LLView*)editor_core;
	while(viewp)
	{
		LLFloater* floaterp = dynamic_cast<LLFloater*>(viewp);
		if (floaterp)
		{
			floaterp->addDependentFloater(this);
			break;
		}
		viewp = viewp->getParent();
	}
}
BOOL LLFloaterScriptSearch::postBuild()
{
	mReplaceBox = getChild<LLLineEditor>("replace_text");
	mSearchBox = getChild<LLLineEditor>("search_text");
	mSearchBox->setCommitCallback(boost::bind(&LLFloaterScriptSearch::onSearchBoxCommit, this));
	mSearchBox->setCommitOnFocusLost(FALSE);
	childSetAction("search_btn", onBtnSearch,this);
	childSetAction("replace_btn", onBtnReplace,this);
	childSetAction("replace_all_btn", onBtnReplaceAll,this);
	setDefaultBtn("search_btn");
	return TRUE;
}
void LLFloaterScriptSearch::show(LLScriptEdCore* editor_core)
{
	LLSD::String search_text;
	LLSD::String replace_text;
	if (sInstance && sInstance->mEditorCore && sInstance->mEditorCore != editor_core)
	{
		search_text=sInstance->mSearchBox->getValue().asString();
		replace_text=sInstance->mReplaceBox->getValue().asString();
		sInstance->closeFloater();
		delete sInstance;
	}
	if (!sInstance)
	{
		new LLFloaterScriptSearch(editor_core);
		sInstance->mSearchBox->setValue(search_text);
		sInstance->mReplaceBox->setValue(replace_text);
	}
	sInstance->openFloater();
}
LLFloaterScriptSearch::~LLFloaterScriptSearch()
{
	sInstance = NULL;
}
void LLFloaterScriptSearch::onBtnSearch(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnSearch();
}
void LLFloaterScriptSearch::handleBtnSearch()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->selectNext(mSearchBox->getValue().asString(), caseChk->get());
}
void LLFloaterScriptSearch::onBtnReplace(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplace();
}
void LLFloaterScriptSearch::handleBtnReplace()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceText(mSearchBox->getValue().asString(), mReplaceBox->getValue().asString(), caseChk->get());
}
void LLFloaterScriptSearch::onBtnReplaceAll(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplaceAll();
}
void LLFloaterScriptSearch::handleBtnReplaceAll()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceTextAll(mSearchBox->getValue().asString(), mReplaceBox->getValue().asString(), caseChk->get());
}
bool LLFloaterScriptSearch::hasAccelerators() const
{
	if (mEditorCore)
	{
		return mEditorCore->hasAccelerators();
	}
	return FALSE;
}
BOOL LLFloaterScriptSearch::handleKeyHere(KEY key, MASK mask)
{
	if (mEditorCore)
	{
		BOOL handled = mEditorCore->handleKeyHere(key, mask);
		if (!handled)
		{
			LLFloater::handleKeyHere(key, mask);
		}
	}
	return FALSE;
}
void LLFloaterScriptSearch::onSearchBoxCommit()
{
	if (mEditorCore && mEditorCore->mEditor)
	{
		LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
		mEditorCore->mEditor->selectNext(mSearchBox->getValue().asString(), caseChk->get());
	}
}
#endif
struct LLSECKeywordCompare
{
	bool operator()(const std::string& lhs, const std::string& rhs)
	{
		return (LLStringUtil::compareDictInsensitive( lhs, rhs ) < 0 );
	}
};
std::vector<LLScriptEdCore::LSLFunctionProps> LLScriptEdCore::mParsedFunctions;
void LLScriptEdCore::parseFunctions(const std::string& filename)
{
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, filename);
	if(LLFile::isfile(filepath))
	{
		LLSD function_list;
		llifstream importer(filepath);
		if(importer.is_open())
		{
			LLSDSerialize::fromXMLDocument(function_list, importer);
			importer.close();
			for (LLSD::map_const_iterator it = function_list.beginMap(); it != function_list.endMap(); ++it)
			{
				LSLFunctionProps fn;
				fn.mName = it->first;
				fn.mSleepTime = it->second["sleep_time"].asFloat();
				fn.mGodOnly = it->second["god_only"].asBoolean();
				mParsedFunctions.push_back(fn);
			}
		}
	}
}
LLScriptEdCore::LLScriptEdCore(
	LLScriptEdContainer* container,
	const std::string& sample,
	const LLHandle<LLFloater>& floater_handle,
	void (*load_callback)(void*),
	void (*save_callback)(void*, BOOL),
	void (*search_replace_callback) (void* userdata),
	void* userdata,
	S32 bottom_pad)
	:
	LLPanel(),
	mSampleText(sample),
	mEditor( NULL ),
	mLoadCallback( load_callback ),
	mSaveCallback( save_callback ),
	mSearchReplaceCallback( search_replace_callback ),
	mUserdata( userdata ),
	mForceClose( FALSE ),
	mLastHelpToken(NULL),
	mLiveHelpHistorySize(0),
	mEnableSave(FALSE),
	mHasScriptData(FALSE),
	mLiveFile(NULL),
	mContainer(container)
{
	setFollowsAll();
	setBorderVisible(FALSE);
	LLUICtrlFactory::getInstance()->buildPanel(this, "floater_script_ed_panel.xml");
	llassert_always(mContainer != NULL);
}
LLScriptEdCore::~LLScriptEdCore()
{
	deleteBridges();
	delete mLiveFile;
}
void LLLiveLSLEditor::experienceChanged()
{
	if (mScriptEd->getAssociatedExperience() != mExperiences->getSelectedValue().asUUID())
	{
		mScriptEd->enableSave(getIsModifiable());
		mScriptEd->setAssociatedExperience(mExperiences->getSelectedValue().asUUID());
		updateExperiencePanel();
	}
}
void LLLiveLSLEditor::onViewProfile(LLUICtrl* ui, void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	LLUUID id;
	if (self->mExperienceEnabled->get())
	{
		id = self->mScriptEd->getAssociatedExperience();
		if (id.notNull())
		{
			LLFloaterExperienceProfile::showInstance(id);
		}
	}
}
void LLLiveLSLEditor::onToggleExperience(LLUICtrl* ui, void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	LLUUID id;
	if (self->mExperienceEnabled->get())
	{
		if (self->mScriptEd->getAssociatedExperience().isNull())
		{
			id = self->mExperienceIds.beginArray()->asUUID();
		}
	}
	if (id != self->mScriptEd->getAssociatedExperience())
	{
		self->mScriptEd->enableSave(self->getIsModifiable());
	}
	self->mScriptEd->setAssociatedExperience(id);
	self->updateExperiencePanel();
}
BOOL LLScriptEdCore::postBuild()
{
	mErrorList = getChild<LLScrollListCtrl>("lsl errors");
	mFunctions = getChild<LLComboBox>( "Insert...");
	childSetCommitCallback("Insert...", &LLScriptEdCore::onBtnInsertFunction, this);
	mEditor = getChild<LLViewerTextEditor>("Script Editor");
	mEditor->setHandleEditKeysDirectly(TRUE);
	mEditor->setParseHighlights(TRUE);
	childSetCommitCallback("lsl errors", &LLScriptEdCore::onErrorList, this);
	childSetAction("Save_btn", boost::bind(&LLScriptEdCore::doSave,this,FALSE));
	childSetAction("Edit_btn", boost::bind(&LLScriptEdCore::openInExternalEditor, this));
	initMenu();
	std::vector<std::string> funcs;
	std::vector<std::string> tooltips;
	for (std::vector<LSLFunctionProps>::const_iterator i = mParsedFunctions.begin();
	i != mParsedFunctions.end(); ++i)
	{
		if (!i->mGodOnly || gAgent.isGodlike())
		{
			std::string name = i->mName;
			funcs.push_back(name);
			std::string desc_name = "LSLTipText_";
			desc_name += name;
			std::string desc = LLTrans::getString(desc_name);
			F32 sleep_time = i->mSleepTime;
			if( sleep_time )
			{
				desc += "\n";
				LLStringUtil::format_map_t args;
				args["[SLEEP_TIME]"] = llformat("%.1f", sleep_time );
				desc += LLTrans::getString("LSLTipSleepTime", args);
			}
			LLStringUtil::replaceString( desc, "\\n", "\n" );
			tooltips.push_back(desc);
		}
	}
    LLColor3 color = vec4to3(gColors.getColor("LSLFunctionColor"));
	std::string keywords_ini = gDirUtilp->getExpandedFilename(LL_PATH_TOP_SKIN, "keywords.ini");
	if(!LLFile::isfile(keywords_ini))
	{
		keywords_ini = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "keywords.ini");
	}
	mEditor->loadKeywords(keywords_ini, funcs, tooltips, color);
	std::vector<std::string> primary_keywords;
	std::vector<std::string> secondary_keywords;
	LLKeywordToken *token;
	LLKeywords::keyword_iterator_t token_it;
	for (token_it = mEditor->keywordsBegin(); token_it != mEditor->keywordsEnd(); ++token_it)
	{
		token = token_it->second;
		if (token->getColor() == color)
		{
			primary_keywords.push_back( wstring_to_utf8str(token->getToken()) );
		}
		else
		{
			secondary_keywords.push_back( wstring_to_utf8str(token->getToken()) );
		}
	}
	std::stable_sort( primary_keywords.begin(), primary_keywords.end(), LLSECKeywordCompare() );
	for (std::vector<std::string>::const_iterator iter= primary_keywords.begin();
			iter!= primary_keywords.end(); ++iter)
	{
		mFunctions->add(*iter);
	}
	for (std::vector<std::string>::const_iterator iter= secondary_keywords.begin();
			iter!= secondary_keywords.end(); ++iter)
	{
		mFunctions->add(*iter);
	}
	return TRUE;
}
void LLScriptEdCore::initMenu()
{
	LLMenuItemCallGL* menuItem;
	menuItem = getChild<LLMenuItemCallGL>("Save");
	menuItem->setMenuCallback(onBtnSave, this);
	menuItem->setEnabledCallback(hasChanged);
	menuItem = getChild<LLMenuItemCallGL>("Revert All Changes");
	menuItem->setMenuCallback(onBtnUndoChanges, this);
	menuItem->setEnabledCallback(hasChanged);
	menuItem = getChild<LLMenuItemCallGL>("Undo");
	menuItem->setMenuCallback(onUndoMenu, this);
	menuItem->setEnabledCallback(enableUndoMenu);
	menuItem = getChild<LLMenuItemCallGL>("Redo");
	menuItem->setMenuCallback(onRedoMenu, this);
	menuItem->setEnabledCallback(enableRedoMenu);
	menuItem = getChild<LLMenuItemCallGL>("Cut");
	menuItem->setMenuCallback(onCutMenu, this);
	menuItem->setEnabledCallback(enableCutMenu);
	menuItem = getChild<LLMenuItemCallGL>("Copy");
	menuItem->setMenuCallback(onCopyMenu, this);
	menuItem->setEnabledCallback(enableCopyMenu);
	menuItem = getChild<LLMenuItemCallGL>("Paste");
	menuItem->setMenuCallback(onPasteMenu, this);
	menuItem->setEnabledCallback(enablePasteMenu);
	menuItem = getChild<LLMenuItemCallGL>("Select All");
	menuItem->setMenuCallback(onSelectAllMenu, this);
	menuItem->setEnabledCallback(enableSelectAllMenu);
	menuItem = getChild<LLMenuItemCallGL>("Deselect");
	menuItem->setMenuCallback(onDeselectMenu, this);
	menuItem->setEnabledCallback(enableDeselectMenu);
	menuItem = getChild<LLMenuItemCallGL>("Search / Replace...");
	menuItem->setMenuCallback(onSearchMenu, this);
	menuItem->setEnabledCallback(NULL);
	menuItem = getChild<LLMenuItemCallGL>("Help...");
	menuItem->setMenuCallback(onBtnHelp, this);
	menuItem->setEnabledCallback(NULL);
	menuItem = getChild<LLMenuItemCallGL>("LSL Wiki Help...");
	menuItem->setMenuCallback(onBtnDynamicHelp, this);
	menuItem->setEnabledCallback(NULL);
}
void LLScriptEdCore::setScriptText(const std::string& text, BOOL is_valid)
{
	if (mEditor)
	{
		mEditor->setText(text);
		mHasScriptData = is_valid;
	}
}
bool LLScriptEdCore::loadScriptText(const std::string& filename)
{
	if (filename.empty())
	{
		LL_WARNS() << "Empty file name" << LL_ENDL;
		return false;
	}
	LLFILE* file = LLFile::fopen(filename, "rb");
	if (!file)
	{
		LL_WARNS() << "Error opening " << filename << LL_ENDL;
		return false;
	}
	fseek(file, 0L, SEEK_END);
	size_t file_length = ftell(file);
	fseek(file, 0L, SEEK_SET);
	if (file_length > 0)
	{
		char* buffer = new char[file_length+1];
		size_t nread = fread(buffer, 1, file_length, file);
		if (nread < file_length)
		{
			LL_WARNS() << "Short read" << LL_ENDL;
		}
		buffer[nread] = '\0';
		fclose(file);
		mEditor->setText(LLStringExplicit(buffer));
		delete[] buffer;
	}
	else
	{
		LL_WARNS() << "Error opening " << filename << LL_ENDL;
		return false;
	}
	return true;
}
bool LLScriptEdCore::writeToFile(const std::string& filename)
{
	LLFILE* fp = LLFile::fopen(filename, "wb");
	if (!fp)
	{
		LL_WARNS() << "Unable to write to " << filename << LL_ENDL;
		LLSD row;
		row["columns"][0]["value"] = LLTrans::getString("CompileQueueProblemWriting");
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		mErrorList->addElement(row);
		return false;
	}
	std::string utf8text = mEditor->getText();
	if (utf8text.empty())
	{
		utf8text.push_back(' ');
	}
	else
	{
		std::stringstream strm(utf8text);
		utf8text.clear();
		bool quote = false;
		for (std::string line; std::getline(strm, line);)
		{
			bool backslash = false;
			for (const auto& ch : line)
			{
				switch (ch)
				{
				case '\\': backslash = !backslash; break;
				case '"': if (!backslash) quote = !quote;
				default: backslash = false; break;
				}
			}
			if (!quote) LLStringUtil::trimTail(line);
			if (!utf8text.empty()) utf8text += '\n';
			utf8text += line;
		}
		if (utf8text.empty()) utf8text.push_back(' ');
	}
	fputs(utf8text.c_str(), fp);
	fclose(fp);
	return true;
}
void LLScriptEdCore::sync()
{
	std::string tmp_file = mContainer->getTmpFileName();
	llstat s;
	if (LLFile::stat(tmp_file, &s) == 0)
	{
		if (mLiveFile) mLiveFile->ignoreNextUpdate();
		writeToFile(tmp_file);
	}
}
bool LLScriptEdCore::hasChanged()
{
	if (!mEditor) return false;
	return ((!mEditor->isPristine() || mEnableSave) && mHasScriptData);
}
void LLScriptEdCore::draw()
{
	BOOL script_changed	= hasChanged();
	getChildView("Save_btn")->setEnabled(script_changed);
	if( mEditor->hasFocus() )
	{
		S32 line = 0;
		S32 column = 0;
		mEditor->getCurrentLineAndColumn( &line, &column, FALSE );
		std::string cursor_pos;
		cursor_pos = llformat("Line %d, Column %d", line, column );
		getChild<LLUICtrl>("line_col")->setValue(cursor_pos);
	}
	else
	{
		getChild<LLUICtrl>("line_col")->setValue(LLStringUtil::null);
	}
	updateDynamicHelp();
	LLPanel::draw();
}
void LLScriptEdCore::updateDynamicHelp(BOOL immediate)
{
	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;
	LLButton* fwd_button = help_floater->getChild<LLButton>("fwd_btn");
	LLButton* back_button = help_floater->getChild<LLButton>("back_btn");
	LLMediaCtrl* browser = help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	back_button->setEnabled(browser->canNavigateBack());
	fwd_button->setEnabled(browser->canNavigateForward());
	if (!immediate && !gSavedSettings.getBOOL("ScriptHelpFollowsCursor"))
	{
		return;
	}
	const LLTextSegment* segment = NULL;
	std::vector<LLTextSegmentPtr> selected_segments;
	mEditor->getSelectedSegments(selected_segments);
	std::vector<LLTextSegmentPtr>::iterator segment_iter;
	for (segment_iter = selected_segments.begin(); segment_iter != selected_segments.end(); ++segment_iter)
	{
		if((*segment_iter)->getToken() && (*segment_iter)->getToken()->getType() == LLKeywordToken::WORD)
		{
			segment = *segment_iter;
			break;
		}
	}
	if (!segment)
	{
		const LLTextSegment* test_segment = mEditor->getPreviousSegment();
		if(test_segment->getToken() && test_segment->getToken()->getType() == LLKeywordToken::WORD)
		{
			segment = test_segment;
		}
	}
	if (segment)
	{
		if (segment->getToken() != mLastHelpToken)
		{
			mLastHelpToken = segment->getToken();
			mLiveHelpTimer.start();
		}
		if (immediate || (mLiveHelpTimer.getStarted() && mLiveHelpTimer.getElapsedTimeF32() > LIVE_HELP_REFRESH_TIME))
		{
			std::string help_string = mEditor->getText().substr(segment->getStart(), segment->getEnd() - segment->getStart());
			setHelpPage(help_string);
			mLiveHelpTimer.stop();
		}
	}
	else
	{
		if (immediate)
		{
			setHelpPage(LLStringUtil::null);
		}
	}
}
void LLScriptEdCore::setHelpPage(const std::string& help_string)
{
	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;
	LLMediaCtrl* web_browser = help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	if (!web_browser) return;
	LLComboBox* history_combo = help_floater->getChild<LLComboBox>("history_combo");
	if (!history_combo) return;
	LLUIString url_string = gSavedSettings.getString("LSLHelpURL");
	url_string.setArg("[LSL_STRING]", help_string);
	addHelpItemToHistory(help_string);
	web_browser->navigateTo(url_string);
}
void LLScriptEdCore::addHelpItemToHistory(const std::string& help_string)
{
	if (help_string.empty()) return;
	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;
	LLComboBox* history_combo = help_floater->getChild<LLComboBox>("history_combo");
	if (!history_combo) return;
	if (mLiveHelpHistorySize == 0)
	{
		history_combo->addSeparator(ADD_TOP);
	}
	while(mLiveHelpHistorySize > MAX_HISTORY_COUNT - 1)
	{
		history_combo->remove(mLiveHelpHistorySize - 1);
		mLiveHelpHistorySize--;
	}
	history_combo->setSimple(help_string);
	S32 index = history_combo->getCurrentIndex();
	if (index >= 0)
	{
		S32 cur_index = history_combo->getCurrentIndex();
		if (cur_index < mLiveHelpHistorySize)
		{
			history_combo->remove(history_combo->getCurrentIndex());
			mLiveHelpHistorySize--;
		}
	}
	history_combo->add(help_string, LLSD(help_string), ADD_TOP);
	history_combo->selectFirstItem();
	mLiveHelpHistorySize++;
}
BOOL LLScriptEdCore::canClose()
{
	if(mForceClose || !hasChanged())
	{
		return TRUE;
	}
	else
	{
		LLNotificationsUtil::add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLScriptEdCore::handleSaveChangesDialog, this, _1, _2));
		return FALSE;
	}
}
void LLScriptEdCore::setEnableEditing(bool enable)
{
	mEditor->setEnabled(enable);
	getChildView("Edit_btn")->setEnabled(enable);
}
bool LLScriptEdCore::handleSaveChangesDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch( option )
	{
	case 0:
		doSave( TRUE );
		break;
	case 1:
		mForceClose = TRUE;
		((LLFloater*) getParent())->close();
		break;
	case 2:
	default:
        LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}
bool LLScriptEdCore::onHelpWebDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:
		LLWeb::loadURL(notification["payload"]["help_url"]);
		break;
	default:
		break;
	}
	return false;
}
void LLScriptEdCore::onBtnHelp(void* userdata)
{
	LLSD payload;
	payload["help_url"] = HELP_LSL_URL;
	LLNotificationsUtil::add("WebLaunchLSLGuide", LLSD(), payload, onHelpWebDialog);
}
void LLScriptEdCore::onBtnDynamicHelp(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		live_help_floater->setFocus(TRUE);
		corep->updateDynamicHelp(TRUE);
		return;
	}
	live_help_floater = new LLFloater(std::string("lsl_help"));
	LLUICtrlFactory::getInstance()->buildFloater(live_help_floater, "floater_lsl_guide.xml");
	((LLFloater*)corep->getParent())->addDependentFloater(live_help_floater, TRUE);
	live_help_floater->childSetCommitCallback("lock_check", onCheckLock, userdata);
	live_help_floater->childSetValue("lock_check", gSavedSettings.getBOOL("ScriptHelpFollowsCursor"));
	live_help_floater->childSetCommitCallback("history_combo", onHelpComboCommit, userdata);
	live_help_floater->childSetAction("back_btn", onClickBack, userdata);
	live_help_floater->childSetAction("fwd_btn", onClickForward, userdata);
	LLMediaCtrl* browser = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	browser->setAlwaysRefresh(TRUE);
	LLComboBox* help_combo = live_help_floater->getChild<LLComboBox>("history_combo");
	LLKeywordToken *token;
	LLKeywords::keyword_iterator_t token_it;
	for (token_it = corep->mEditor->keywordsBegin();
		token_it != corep->mEditor->keywordsEnd();
		++token_it)
	{
		token = token_it->second;
		help_combo->add(wstring_to_utf8str(token->getToken()));
	}
	help_combo->sortByName();
	corep->mLastHelpToken = NULL;
	corep->mLiveHelpHandle = live_help_floater->getHandle();
	corep->mLiveHelpHistorySize = 0;
	corep->updateDynamicHelp(TRUE);
}
void LLScriptEdCore::onClickBack(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		LLMediaCtrl* browserp = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		if (browserp)
		{
			browserp->navigateBack();
		}
	}
}
void LLScriptEdCore::onClickForward(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		LLMediaCtrl* browserp = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		if (browserp)
		{
			browserp->navigateForward();
		}
	}
}
void LLScriptEdCore::onCheckLock(LLUICtrl* ctrl, void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	gSavedSettings.setBOOL("ScriptHelpFollowsCursor", ctrl->getValue().asBoolean());
	corep->mLastHelpToken = NULL;
}
void LLScriptEdCore::onBtnInsertSample(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;
	self->mEditor->selectAll();
	self->mEditor->cut();
	self->mEditor->insertText(self->mSampleText);
}
void LLScriptEdCore::onHelpComboCommit(LLUICtrl* ctrl, void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		std::string help_string = ctrl->getValue().asString();
		corep->addHelpItemToHistory(help_string);
		LLMediaCtrl* web_browser = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		LLUIString url_string = gSavedSettings.getString("LSLHelpURL");
		url_string.setArg("[LSL_STRING]", help_string);
		web_browser->navigateTo(url_string);
	}
}
void LLScriptEdCore::onBtnInsertFunction(LLUICtrl *ui, void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;
	if(self->mEditor->getEnabled())
	{
		self->mEditor->insertText(self->mFunctions->getSimple());
	}
	self->mEditor->setFocus(TRUE);
	self->setHelpPage(self->mFunctions->getSimple());
}
void LLScriptEdCore::doSave( BOOL close_after_save)
{
	LLViewerStats::getInstance()->incStat( LLViewerStats::ST_LSL_SAVE_COUNT );
	if( mSaveCallback )
	{
		mSaveCallback( mUserdata, close_after_save );
	}
}
void LLScriptEdCore::openInExternalEditor()
{
	delete mLiveFile;
	std::string filename = mContainer->getTmpFileName();
	writeToFile(filename);
	mLiveFile = new LLLiveLSLFile(filename, boost::bind(&LLScriptEdContainer::onExternalChange, mContainer, _1));
	mLiveFile->addToEventTimer();
	{
		LLExternalEditor ed;
		LLExternalEditor::EErrorCode status;
		std::string msg;
		status = ed.setCommand("LL_SCRIPT_EDITOR");
		if (status != LLExternalEditor::EC_SUCCESS)
		{
			if (status == LLExternalEditor::EC_NOT_SPECIFIED)
			{
				msg = "External editor not set";
			}
			else
			{
				msg = LLExternalEditor::getErrorMessage(status);
			}
			LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", msg));
			return;
		}
		status = ed.run(filename);
		if (status != LLExternalEditor::EC_SUCCESS)
		{
			msg = LLExternalEditor::getErrorMessage(status);
			LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", msg));
		}
	}
}
void LLScriptEdCore::onBtnUndoChanges()
{
	if( !mEditor->tryToRevertToPristineState() )
	{
		LLNotificationsUtil::add("ScriptCannotUndo", LLSD(), LLSD(), boost::bind(&LLScriptEdCore::handleReloadFromServerDialog, this, _1, _2));
	}
}
BOOL LLScriptEdCore::hasChanged(void* userdata)
{
	return static_cast<LLScriptEdCore*>(userdata)->hasChanged();
}
void LLScriptEdCore::onBtnSave(void* data)
{
	static_cast<LLScriptEdCore*>(data)->doSave(FALSE);
}
void LLScriptEdCore::onBtnUndoChanges( void* userdata )
{
	static_cast<LLScriptEdCore*>(userdata)->onBtnUndoChanges();
}
void LLScriptEdCore::onSearchMenu(void* userdata)
{
	LLScriptEdCore* sec = (LLScriptEdCore*)userdata;
	if (sec && sec->mEditor)
	{
		LLFloaterSearchReplace::show(sec->mEditor);
	}
}
void LLScriptEdCore::onUndoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->undo();
}
void LLScriptEdCore::onRedoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->redo();
}
void LLScriptEdCore::onCutMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->cut();
}
void LLScriptEdCore::onCopyMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->copy();
}
void LLScriptEdCore::onPasteMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->paste();
}
void LLScriptEdCore::onSelectAllMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->selectAll();
}
void LLScriptEdCore::onDeselectMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->deselect();
}
BOOL LLScriptEdCore::enableUndoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canUndo();
}
BOOL LLScriptEdCore::enableRedoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canRedo();
}
BOOL LLScriptEdCore::enableCutMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canCut();
}
BOOL LLScriptEdCore::enableCopyMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canCopy();
}
BOOL LLScriptEdCore::enablePasteMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canPaste();
}
BOOL LLScriptEdCore::enableSelectAllMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canSelectAll();
}
BOOL LLScriptEdCore::enableDeselectMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canDeselect();
}
void LLScriptEdCore::onErrorList(LLUICtrl*, void* user_data)
{
	LLScriptEdCore* self = (LLScriptEdCore*)user_data;
	LLScrollListItem* item = self->mErrorList->getFirstSelected();
	if(item)
	{
		S32 row = 0;
		S32 column = 0;
		const LLScrollListCell* cell = item->getColumn(0);
		std::string line(cell->getValue().asString());
		line.erase(0, 1);
		LLStringUtil::replaceChar(line, ',',' ');
		LLStringUtil::replaceChar(line, ')',' ');
		sscanf(line.c_str(), "%d %d", &row, &column);
		self->mEditor->setCursor(row, column);
	}
}
bool LLScriptEdCore::handleReloadFromServerDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch( option )
	{
	case 0:
		if( mLoadCallback )
		{
			setScriptText(getString("loading"), FALSE);
			mLoadCallback( mUserdata );
		}
		break;
	case 1:
		break;
	default:
		llassert(0);
		break;
	}
	return false;
}
void LLScriptEdCore::selectFirstError()
{
	mErrorList->selectFirstItem();
	if (gSavedSettings.getBOOL("LiruScriptErrorsStealFocus"))
		onErrorList(mErrorList, this);
}
struct LLEntryAndEdCore
{
	LLScriptEdCore* mCore;
	LLEntryAndEdCore(LLScriptEdCore* core) :
		mCore(core)
		{}
};
void LLScriptEdCore::deleteBridges()
{
	S32 count = mBridges.size();
	LLEntryAndEdCore* eandc;
	for(S32 i = 0; i < count; i++)
	{
		eandc = mBridges.at(i);
		delete eandc;
		mBridges[i] = NULL;
	}
	mBridges.clear();
}
BOOL LLScriptEdCore::handleKeyHere(KEY key, MASK mask)
{
	bool just_control = MASK_CONTROL == (mask & MASK_MODIFIERS);
	if(('S' == key) && just_control)
	{
		if(mSaveCallback)
		{
			mSaveCallback(mUserdata, FALSE);
		}
		return TRUE;
	}
	if(('F' == key) && just_control)
	{
		if(mSearchReplaceCallback)
		{
			mSearchReplaceCallback(mUserdata);
		}
		return TRUE;
	}
	return FALSE;
}
LLUUID LLScriptEdCore::getAssociatedExperience() const
{
	return mAssociatedExperience;
}
void LLLiveLSLEditor::setExperienceIds(const LLSD& experience_ids)
{
	mExperienceIds = experience_ids;
	updateExperiencePanel();
}
void LLLiveLSLEditor::updateExperiencePanel()
{
	if (mScriptEd->getAssociatedExperience().isNull())
	{
		mExperienceEnabled->set(FALSE);
		mExperiences->setVisible(FALSE);
		if (mExperienceIds.size() > 0)
		{
			mExperienceEnabled->setEnabled(TRUE);
			mExperienceEnabled->setToolTip(getString("add_experiences"));
		}
		else
		{
			mExperienceEnabled->setEnabled(FALSE);
			mExperienceEnabled->setToolTip(getString("no_experiences"));
		}
		getChild<LLButton>("view_profile")->setVisible(FALSE);
	}
	else
	{
		mExperienceEnabled->setToolTip(getString("experience_enabled"));
		mExperienceEnabled->setEnabled(getIsModifiable());
		mExperiences->setVisible(TRUE);
		mExperienceEnabled->set(TRUE);
		getChild<LLButton>("view_profile")->setToolTip(getString("show_experience_profile"));
		buildExperienceList();
	}
}
void LLLiveLSLEditor::buildExperienceList()
{
	mExperiences->clearRows();
	bool foundAssociated = false;
	const LLUUID& associated = mScriptEd->getAssociatedExperience();
	LLUUID last;
	LLScrollListItem* item;
	for(LLSD::array_const_iterator it = mExperienceIds.beginArray(); it != mExperienceIds.endArray(); ++it)
	{
		LLUUID id = it->asUUID();
		EAddPosition position = ADD_BOTTOM;
		if (id == associated)
		{
			foundAssociated = true;
			position = ADD_TOP;
		}
        const LLSD& experience = LLExperienceCache::instance().get(id);
		if (experience.isUndefined())
		{
			mExperiences->add(getString("loading"), id, position);
			last = id;
		}
		else
		{
			std::string experience_name_string = experience[LLExperienceCache::NAME].asString();
			if (experience_name_string.empty())
			{
				experience_name_string = LLTrans::getString("ExperienceNameUntitled");
			}
			mExperiences->add(experience_name_string, id, position);
		}
	}
	if (!foundAssociated)
	{
        const LLSD& experience = LLExperienceCache::instance().get(associated);
		if (experience.isDefined())
		{
			std::string experience_name_string = experience[LLExperienceCache::NAME].asString();
			if (experience_name_string.empty())
			{
				experience_name_string = LLTrans::getString("ExperienceNameUntitled");
			}
			item=mExperiences->add(experience_name_string, associated, ADD_TOP);
		}
		else
		{
			item = mExperiences->add(getString("loading"), associated, ADD_TOP);
			last = associated;
		}
		item->setEnabled(FALSE);
	}
	if (last.notNull())
	{
		mExperiences->setEnabled(FALSE);
        LLExperienceCache::instance().get(last, boost::bind(&LLLiveLSLEditor::buildExperienceList, this));
	}
	else
	{
		mExperiences->setEnabled(TRUE);
		mExperiences->sortByName(TRUE);
		mExperiences->setCurrentByIndex(mExperiences->getCurrentIndex());
		getChild<LLButton>("view_profile")->setVisible(TRUE);
	}
}
void LLScriptEdCore::setAssociatedExperience(const LLUUID& experience_id)
{
	mAssociatedExperience = experience_id;
}
void LLLiveLSLEditor::requestExperiences()
{
	if (!getIsModifiable())
	{
		return;
	}
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string lookup_url = region->getCapability("GetCreatorExperiences");
		if (!lookup_url.empty())
		{
			LLHTTPClient::get(lookup_url, new LLCoroResponder(
				boost::bind(&LLLiveLSLEditor::receiveExperienceIds, _1, getDerivedHandle<LLLiveLSLEditor>())));
		}
	}
}
void LLLiveLSLEditor::receiveExperienceIds(const LLCoroResponder& responder, LLHandle<LLLiveLSLEditor> hparent)
{
    LLLiveLSLEditor* parent = hparent.get();
	if (!parent)
		return;
	parent->setExperienceIds(responder.getContent()["experience_ids"]);
}
LLScriptEdContainer::LLScriptEdContainer(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_id, const LLUUID& object_id)
:	LLPreview(name, rect, title, item_id, object_id, TRUE, SCRIPT_MIN_WIDTH, SCRIPT_MIN_HEIGHT)
,	mScriptEd(NULL)
{
}
std::string LLScriptEdContainer::getTmpFileName()
{
	std::string script_id = mObjectUUID.asString() + "_" + mItemUUID.asString();
	char script_id_hash_str[33];
	LLMD5 script_id_hash((const U8 *)script_id.c_str());
	script_id_hash.hex_digest(script_id_hash_str);
	return std::string(LLFile::tmpdir()) + "sl_script_" + script_id_hash_str + ".lsl";
}
bool LLScriptEdContainer::onExternalChange(const std::string& filename)
{
	if (!mScriptEd->loadScriptText(filename))
	{
		return false;
	}
	saveIfNeeded(false);
	return true;
}
BOOL LLScriptEdContainer::canSaveAs() const
{
	return TRUE;
}
void LLScriptEdContainer::saveAs()
{
	std::string default_filename("untitled.lsl");
	if (const LLInventoryItem* item = getItem())
	{
		default_filename = LLDir::getScrubbedFileName(item->getName());
	}
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open(default_filename, FFSAVE_SCRIPT);
	filepicker->run(boost::bind(&LLLiveLSLEditor::saveAs_continued, this, filepicker));
}
void LLScriptEdContainer::saveAs_continued(AIFilePicker* filepicker)
{
	if (!filepicker->hasFilename()) return;
	LLFILE* fp = LLFile::fopen(filepicker->getFilename(), "wb");
	fputs(mScriptEd->mEditor->getText().c_str(), fp);
	fclose(fp);
}
struct LLScriptSaveInfo
{
	LLUUID mItemUUID;
	std::string mDescription;
	LLTransactionID mTransactionID;
	LLScriptSaveInfo(const LLUUID& uuid, const std::string& desc, LLTransactionID tid) :
		mItemUUID(uuid), mDescription(desc),  mTransactionID(tid) {}
};
void* LLPreviewLSL::createScriptEdPanel(void* userdata)
{
	LLPreviewLSL *self = (LLPreviewLSL*)userdata;
	self->mScriptEd =  new LLScriptEdCore(
								   self,
								   HELLO_LSL,
								   self->getHandle(),
								   LLPreviewLSL::onLoad,
								   LLPreviewLSL::onSave,
								   LLPreviewLSL::onSearchReplace,
								   self,
								   0);
	return self->mScriptEd;
}
LLPreviewLSL::LLPreviewLSL(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_id )
:	LLScriptEdContainer(name, rect, title, item_id),
	mPendingUploads(0)
{
	mFactoryMap["script panel"] = LLCallbackMap(LLPreviewLSL::createScriptEdPanel, this);
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_script_preview.xml", &getFactoryMap());
}
BOOL LLPreviewLSL::postBuild()
{
	const LLInventoryItem* item = getItem();
	llassert(item);
	if (item)
	{
		getChild<LLUICtrl>("desc")->setValue(item->getDescription());
	}
	childSetCommitCallback("desc", LLPreview::onText, this);
	getChild<LLLineEditor>("desc")->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	return LLPreview::postBuild();
}
void LLPreviewLSL::callbackLSLCompileSucceeded()
{
	LL_INFOS() << "LSL Bytecode saved" << LL_ENDL;
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessful"));
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
	closeIfNeeded();
}
void LLPreviewLSL::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	LL_INFOS() << "Compile failed!" << LL_ENDL;
	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		std::string error_message = line->asString();
		LLStringUtil::stripNonprintable(error_message);
		row["columns"][0]["value"] = error_message;
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	closeIfNeeded();
}
void LLPreviewLSL::loadAsset()
{
	const LLInventoryItem* item = gInventory.getItem(mItemUUID);
	BOOL is_library = item
		&& !gInventory.isObjectDescendentOf(mItemUUID,
											gInventory.getRootFolderID());
	if(!item)
	{
		getItem();
	}
	if(item)
	{
		BOOL is_copyable = gAgent.allowOperation(PERM_COPY,
								item->getPermissions(), GP_OBJECT_MANIPULATE);
		BOOL is_modifiable = gAgent.allowOperation(PERM_MODIFY,
								item->getPermissions(), GP_OBJECT_MANIPULATE);
		if (gAgent.isGodlike() || (is_copyable && (is_modifiable || is_library)))
		{
			LLUUID* new_uuid = new LLUUID(mItemUUID);
			gAssetStorage->getInvItemAsset(LLHost::invalid,
										gAgent.getID(),
										gAgent.getSessionID(),
										item->getPermissions().getOwner(),
										LLUUID::null,
										item->getUUID(),
										item->getAssetUUID(),
										item->getType(),
										&LLPreviewLSL::onLoadComplete,
										(void*)new_uuid,
										TRUE);
			mAssetStatus = PREVIEW_ASSET_LOADING;
		}
		else
		{
			mScriptEd->setScriptText(mScriptEd->getString("can_not_view"), FALSE);
			mScriptEd->mEditor->makePristine();
			mScriptEd->mFunctions->setEnabled(FALSE);
			mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		getChildView("lock")->setVisible( !is_modifiable);
		mScriptEd->getChildView("Insert...")->setEnabled(is_modifiable);
	}
	else
	{
		mScriptEd->setScriptText(std::string(HELLO_LSL), TRUE);
		mScriptEd->setEnableEditing(TRUE);
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
}
BOOL LLPreviewLSL::canClose()
{
	return mScriptEd->canClose();
}
void LLPreviewLSL::closeIfNeeded()
{
	getWindow()->decBusyCount();
	mPendingUploads--;
	if (mPendingUploads <= 0 && mCloseAfterSave)
	{
		close();
	}
}
void LLPreviewLSL::onSearchReplace(void* userdata)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	LLScriptEdCore* sec = self->mScriptEd;
	if (sec && sec->mEditor)
	{
		LLFloaterSearchReplace::show(sec->mEditor);
	}
}
void LLPreviewLSL::onLoad(void* userdata)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	self->loadAsset();
}
void LLPreviewLSL::onSave(void* userdata, BOOL close_after_save)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	self->mCloseAfterSave = close_after_save;
	self->saveIfNeeded();
}
void LLPreviewLSL::saveIfNeeded(bool sync )
{
	if(!mScriptEd->hasChanged())
	{
		return;
	}
	mPendingUploads = 0;
	mScriptEd->mErrorList->deleteAllItems();
	mScriptEd->mErrorList->setCommentText("");
	mScriptEd->mEditor->makePristine();
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = filepath + ".lsl";
	mScriptEd->writeToFile(filename);
	if (sync)
	{
		mScriptEd->sync();
	}
    if (!gAgent.getRegion()) return;
	const LLInventoryItem *inv_item = getItem();
	if(inv_item)
	{
		getWindow()->incBusyCount();
		mPendingUploads++;
		std::string url = gAgent.getRegion()->getCapability("UpdateScriptAgent");
		if (!url.empty())
		{
			uploadAssetViaCaps(url, filename, mItemUUID);
		}
		else
		{
			LLSD row;
			mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileQueueProblemUploading"));
			LLFile::remove(filename);
		}
	}
}
void LLPreviewLSL::uploadAssetViaCaps(const std::string& url,
									  const std::string& filename,
									  const LLUUID& item_id)
{
	LL_INFOS() << "Update Agent Inventory via capability" << LL_ENDL;
	LLSD body;
	body["item_id"] = item_id;
	if (gSavedSettings.getBOOL("SaveInventoryScriptsAsMono"))
	{
		body["target"] = "mono";
	}
	else
	{
		body["target"] = "lsl2";
	}
	LLHTTPClient::post(url, body, new LLUpdateAgentInventoryResponder(body, filename, LLAssetType::AT_LSL_TEXT));
}
void LLPreviewLSL::onLoadComplete( LLVFS *vfs, const LLUUID& asset_uuid, LLAssetType::EType type,
								   void* user_data, S32 status, LLExtStat ext_status)
{
	LL_DEBUGS() << "LLPreviewLSL::onLoadComplete: got uuid " << asset_uuid
		 << LL_ENDL;
	LLUUID* item_uuid = (LLUUID*)user_data;
	LLPreviewLSL* preview = static_cast<LLPreviewLSL*>(LLPreview::find(*item_uuid));
	if( preview )
	{
		if(0 == status)
		{
			LLVFile file(vfs, asset_uuid, type);
			S32 file_length = file.getSize();
			char* buffer = new char[file_length+1];
			file.read((U8*)buffer, file_length);
			buffer[file_length] = 0;
			preview->mScriptEd->setScriptText(LLStringExplicit(&buffer[0]), TRUE);
			preview->mScriptEd->mEditor->makePristine();
			delete [] buffer;
			LLInventoryItem* item = gInventory.getItem(*item_uuid);
			BOOL is_modifiable = FALSE;
			if(item
			   && gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
				   					GP_OBJECT_MANIPULATE))
			{
				is_modifiable = TRUE;
			}
			preview->mScriptEd->setEnableEditing(is_modifiable);
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotificationsUtil::add("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotificationsUtil::add("ScriptNoPermissions");
			}
			else
			{
				LLNotificationsUtil::add("UnableToLoadScript");
			}
			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
			LL_WARNS() << "Problem loading script: " << status << LL_ENDL;
		}
	}
	delete item_uuid;
}
void* LLLiveLSLEditor::createScriptEdPanel(void* userdata)
{
	LLLiveLSLEditor *self = (LLLiveLSLEditor*)userdata;
	self->mScriptEd =  new LLScriptEdCore(
								   self,
								   HELLO_LSL,
								   self->getHandle(),
								   &LLLiveLSLEditor::onLoad,
								   &LLLiveLSLEditor::onSave,
								   &LLLiveLSLEditor::onSearchReplace,
								   self,
								   0);
	return self->mScriptEd;
}
LLLiveLSLEditor::LLLiveLSLEditor(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& object_id, const LLUUID& item_id) :
	LLScriptEdContainer(name, rect, title, item_id, object_id),
	mIsNew(false),
	mAskedForRunningInfo(FALSE),
	mHaveRunningInfo(FALSE),
	mCloseAfterSave(FALSE),
	mPendingUploads(0),
	mIsModifiable(FALSE)
{
	mFactoryMap["script ed panel"] = LLCallbackMap(LLLiveLSLEditor::createScriptEdPanel, this);
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_live_lsleditor.xml", &getFactoryMap());
}
BOOL LLLiveLSLEditor::postBuild()
{
	childSetCommitCallback("running", LLLiveLSLEditor::onRunningCheckboxClicked, this);
	getChildView("running")->setEnabled(FALSE);
	childSetAction("Reset",&LLLiveLSLEditor::onReset,this);
	getChildView("Reset")->setEnabled(TRUE);
	mMonoCheckbox =	getChild<LLCheckBoxCtrl>("mono");
	childSetCommitCallback("mono", &LLLiveLSLEditor::onMonoCheckboxClicked, this);
	getChildView("mono")->setEnabled(FALSE);
	mScriptEd->mEditor->makePristine();
	mScriptEd->mEditor->setFocus(TRUE);
	mExperiences = getChild<LLComboBox>("Experiences...");
	mExperiences->setCommitCallback(boost::bind(&LLLiveLSLEditor::experienceChanged, this));
	mExperienceEnabled = getChild<LLCheckBoxCtrl>("enable_xp");
	childSetCommitCallback("enable_xp", onToggleExperience, this);
	childSetCommitCallback("view_profile", onViewProfile, this);
	return LLPreview::postBuild();
}
void LLLiveLSLEditor::callbackLSLCompileSucceeded(const LLUUID& task_id,
												  const LLUUID& item_id,
												  bool is_script_running)
{
	LL_DEBUGS() << "LSL Bytecode saved" << LL_ENDL;
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessful"));
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
	closeIfNeeded();
}
void LLLiveLSLEditor::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	LL_DEBUGS() << "Compile failed!" << LL_ENDL;
	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		std::string error_message = line->asString();
		LLStringUtil::stripNonprintable(error_message);
		row["columns"][0]["value"] = error_message;
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	closeIfNeeded();
}
void LLLiveLSLEditor::loadAsset()
{
	if(!mIsNew)
	{
		LLViewerObject* object = gObjectList.findObject(mObjectUUID);
		if(object)
		{
			LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemUUID));
			if (item)
			{
				LLViewerRegion* region = object->getRegion();
				std::string url = std::string();
				if(region)
				{
					url = region->getCapability("GetMetadata");
				}
				LLExperienceCache::instance().fetchAssociatedExperience(item->getParentUUID(), item->getUUID(), url,
                        boost::bind(&LLLiveLSLEditor::setAssociatedExperience, getDerivedHandle<LLLiveLSLEditor>(), _1));
				bool isGodlike = gAgent.isGodlike();
				bool copyManipulate = gAgent.allowOperation(PERM_COPY, item->getPermissions(), GP_OBJECT_MANIPULATE);
				mIsModifiable = gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE);
				if (!isGodlike && (!copyManipulate || !mIsModifiable))
				{
					mItem = new LLViewerInventoryItem(item);
					mScriptEd->setScriptText(getString("not_allowed"), FALSE);
					mScriptEd->mEditor->makePristine();
					mScriptEd->enableSave(FALSE);
					mAssetStatus = PREVIEW_ASSET_LOADED;
				}
				else if (copyManipulate || isGodlike)
				{
					mItem = new LLViewerInventoryItem(item);
					LLUUID* user_data = new LLUUID(mItemUUID);
					gAssetStorage->getInvItemAsset(object->getRegion()->getHost(),
						gAgent.getID(),
						gAgent.getSessionID(),
						item->getPermissions().getOwner(),
						object->getID(),
						item->getUUID(),
						item->getAssetUUID(),
						item->getType(),
						&LLLiveLSLEditor::onLoadComplete,
						(void*)user_data,
						TRUE);
					LLMessageSystem* msg = gMessageSystem;
					msg->newMessageFast(_PREHASH_GetScriptRunning);
					msg->nextBlockFast(_PREHASH_Script);
					msg->addUUIDFast(_PREHASH_ObjectID, mObjectUUID);
					msg->addUUIDFast(_PREHASH_ItemID, mItemUUID);
					msg->sendReliable(object->getRegion()->getHost());
					mAskedForRunningInfo = TRUE;
					mAssetStatus = PREVIEW_ASSET_LOADING;
				}
			}
			if (mItem.isNull())
			{
				mScriptEd->setScriptText(LLStringUtil::null, FALSE);
				mScriptEd->mEditor->makePristine();
				mAssetStatus = PREVIEW_ASSET_LOADED;
				mIsModifiable = FALSE;
			}
			refreshFromItem(item);
		}
	}
	else
	{
		mScriptEd->setScriptText(std::string(HELLO_LSL), TRUE);
		mScriptEd->enableSave(FALSE);
		LLPermissions perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, gAgent.getGroupID());
		perm.initMasks(PERM_ALL, PERM_ALL, LLFloaterPerms::getEveryonePerms("Scripts"), LLFloaterPerms::getGroupPerms("Scripts"), LLFloaterPerms::getNextOwnerPerms("Scripts"));
		mItem = new LLViewerInventoryItem(mItemUUID,
										  mObjectUUID,
										  perm,
										  LLUUID::null,
										  LLAssetType::AT_LSL_TEXT,
										  LLInventoryType::IT_LSL,
										  DEFAULT_SCRIPT_NAME,
										  DEFAULT_SCRIPT_DESC,
										  LLSaleInfo::DEFAULT,
										  LLInventoryItemFlags::II_FLAGS_NONE,
										  time_corrected());
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
	requestExperiences();
}
void LLLiveLSLEditor::onLoadComplete(LLVFS *vfs, const LLUUID& asset_id,
									 LLAssetType::EType type,
									 void* user_data, S32 status, LLExtStat ext_status)
{
	LL_DEBUGS() << "LLLiveLSLEditor::onLoadComplete: got uuid " << asset_id
		 << LL_ENDL;
	LLUUID* xored_id = (LLUUID*)user_data;
	LLLiveLSLEditor* instance = static_cast<LLLiveLSLEditor*>(LLPreview::find(*xored_id));
	if(instance )
	{
		if( LL_ERR_NOERR == status )
		{
			instance->loadScriptText(vfs, asset_id, type);
			instance->mScriptEd->setEnableEditing(TRUE);
			instance->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotificationsUtil::add("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotificationsUtil::add("ScriptNoPermissions");
			}
			else
			{
				LLNotificationsUtil::add("UnableToLoadScript");
			}
			instance->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}
	delete xored_id;
}
void LLLiveLSLEditor::loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type)
{
	LLVFile file(vfs, uuid, type);
	S32 file_length = file.getSize();
	char *buffer = new char[file_length + 1];
	file.read((U8*)buffer, file_length);
	if (file.getLastBytesRead() != file_length ||
		file_length <= 0)
	{
		LL_WARNS() << "Error reading " << uuid << ":" << type << LL_ENDL;
	}
	buffer[file_length] = '\0';
	mScriptEd->setScriptText(LLStringExplicit(&buffer[0]), TRUE);
	mScriptEd->mEditor->makePristine();
	delete[] buffer;
}
void LLLiveLSLEditor::onRunningCheckboxClicked( LLUICtrl*, void* userdata )
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;
	LLViewerObject* object = gObjectList.findObject( self->mObjectUUID );
	LLCheckBoxCtrl* runningCheckbox = self->getChild<LLCheckBoxCtrl>("running");
	BOOL running =  runningCheckbox->get();
	if( object )
	{
		if ( (rlv_handler_t::isEnabled()) && (gRlvAttachmentLocks.isLockedAttachment(object->getRootEdit())) )
		{
			return;
		}
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_SetScriptRunning);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Script);
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectUUID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemUUID);
		msg->addBOOLFast(_PREHASH_Running, running);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		runningCheckbox->set(!running);
		LLNotificationsUtil::add("CouldNotStartStopScript");
	}
}
void LLLiveLSLEditor::onReset(void *userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;
	LLViewerObject* object = gObjectList.findObject( self->mObjectUUID );
	if(object)
	{
		if ( (rlv_handler_t::isEnabled()) && (gRlvAttachmentLocks.isLockedAttachment(object->getRootEdit())) )
		{
			return;
		}
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ScriptReset);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Script);
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectUUID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemUUID);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		LLNotificationsUtil::add("CouldNotStartStopScript");
	}
}
void LLLiveLSLEditor::draw()
{
	LLViewerObject* object = gObjectList.findObject(mObjectUUID);
	LLCheckBoxCtrl* runningCheckbox = getChild<LLCheckBoxCtrl>( "running");
	if(object && mAskedForRunningInfo && mHaveRunningInfo)
	{
		if(object->permAnyOwner())
		{
			runningCheckbox->setLabel(getString("script_running"));
			runningCheckbox->setEnabled(TRUE);
			if(object->permAnyOwner())
			{
				runningCheckbox->setLabel(getString("script_running"));
				runningCheckbox->setEnabled(TRUE);
			}
			else
			{
				runningCheckbox->setLabel(getString("public_objects_can_not_run"));
				runningCheckbox->setEnabled(FALSE);
				runningCheckbox->set(FALSE);
				mMonoCheckbox->set(FALSE);
			}
		}
		else
		{
			runningCheckbox->setLabel(getString("public_objects_can_not_run"));
			runningCheckbox->setEnabled(FALSE);
			runningCheckbox->set(FALSE);
			mMonoCheckbox->setEnabled(FALSE);
			mHaveRunningInfo = FALSE;
		}
	}
	else if(!object)
	{
		setTitle(LLTrans::getString("ObjectOutOfRange"));
		runningCheckbox->setEnabled(FALSE);
		mMonoCheckbox->setEnabled(FALSE);
		mHaveRunningInfo = FALSE;
	}
	LLPreview::draw();
}
void LLLiveLSLEditor::onSearchReplace(void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	LLScriptEdCore* sec = self->mScriptEd;
	if (sec && sec->mEditor)
	{
		LLFloaterSearchReplace::show(sec->mEditor);
	}
}
struct LLLiveLSLSaveData
{
	LLLiveLSLSaveData(const LLUUID& id, const LLViewerInventoryItem* item, BOOL active);
	LLUUID mSaveObjectID;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL mActive;
};
LLLiveLSLSaveData::LLLiveLSLSaveData(const LLUUID& id,
									 const LLViewerInventoryItem* item,
									 BOOL active) :
	mSaveObjectID(id),
	mActive(active)
{
	llassert(item);
	mItem = new LLViewerInventoryItem(item);
}
void LLLiveLSLEditor::saveIfNeeded(bool sync )
{
	LLViewerObject* object = gObjectList.findObject(mObjectUUID);
	if(!object)
	{
		LLNotificationsUtil::add("SaveScriptFailObjectNotFound");
		return;
	}
	if(mItem.isNull() || !mItem->isFinished())
	{
		LLNotificationsUtil::add("SaveScriptFailObjectNotFound");
		return;
	}
	if ( (rlv_handler_t::isEnabled()) && (gRlvAttachmentLocks.isLockedAttachment(object->getRootEdit())) )
	{
		return;
	}
	LLInventoryItem* inv_item = dynamic_cast<LLInventoryItem*>(object->getInventoryObject(mItemUUID));
	if(inv_item)
	{
		mItem->copyItem(inv_item);
	}
	if(!mScriptEd->hasChanged())
	{
		return;
	}
	mPendingUploads = 0;
	mScriptEd->enableSave(FALSE);
	mScriptEd->mEditor->makePristine();
	mScriptEd->mErrorList->deleteAllItems();
	mScriptEd->mErrorList->setCommentText("");
	mScriptEd->mEditor->makePristine();
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = llformat("%s.lsl", filepath.c_str());
	mItem->setAssetUUID(asset_id);
	mItem->setTransactionID(tid);
	mScriptEd->writeToFile(filename);
	if (sync)
	{
		mScriptEd->sync();
	}
	std::string url = object->getRegion()->getCapability("UpdateScriptTask");
	getWindow()->incBusyCount();
	mPendingUploads++;
	BOOL is_running = getChild<LLCheckBoxCtrl>( "running")->get();
	if (!url.empty())
	{
		uploadAssetViaCaps(url, filename, mObjectUUID, mItemUUID, is_running, mScriptEd->getAssociatedExperience());
	}
	else
	{
		mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileQueueProblemUploading"));
		LLFile::remove(filename);
	}
}
void LLLiveLSLEditor::uploadAssetViaCaps(const std::string& url,
										 const std::string& filename,
										 const LLUUID& task_id,
										 const LLUUID& item_id,
										 BOOL is_running,
										 const LLUUID& experience_public_id)
{
	LL_INFOS() << "Update Task Inventory via capability " << url << LL_ENDL;
	LLSD body;
	body["task_id"] = task_id;
	body["item_id"] = item_id;
	body["is_script_running"] = is_running;
	body["target"] = monoChecked() ? "mono" : "lsl2";
	body["experience"] = experience_public_id;
	LLHTTPClient::post(url, body,
		new LLUpdateTaskInventoryResponder(body, filename, LLAssetType::AT_LSL_TEXT));
}
BOOL LLLiveLSLEditor::canClose()
{
	return (mScriptEd->canClose());
}
void LLLiveLSLEditor::closeIfNeeded()
{
	getWindow()->decBusyCount();
	mPendingUploads--;
	if (mPendingUploads <= 0 && mCloseAfterSave)
	{
		close();
	}
}
void LLLiveLSLEditor::onLoad(void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	self->loadAsset();
}
void LLLiveLSLEditor::onSave(void* userdata, BOOL close_after_save)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	self->mCloseAfterSave = close_after_save;
	self->saveIfNeeded();
}
void LLLiveLSLEditor::processScriptRunningReply(LLMessageSystem* msg, void**)
{
	LLUUID item_id;
	LLUUID object_id;
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ItemID, item_id);
	LLLiveLSLEditor* instance = static_cast<LLLiveLSLEditor*>(LLPreview::find(item_id));
	if(instance)
	{
		instance->mHaveRunningInfo = TRUE;
		BOOL running;
		msg->getBOOLFast(_PREHASH_Script, _PREHASH_Running, running);
		LLCheckBoxCtrl* runningCheckbox = instance->getChild<LLCheckBoxCtrl>("running");
		runningCheckbox->set(running);
		BOOL mono;
		msg->getBOOLFast(_PREHASH_Script, "Mono", mono);
		LLCheckBoxCtrl* monoCheckbox = instance->getChild<LLCheckBoxCtrl>("mono");
		monoCheckbox->setEnabled(instance->getIsModifiable() && have_script_upload_cap(object_id));
		monoCheckbox->set(mono);
	}
}
void LLLiveLSLEditor::onMonoCheckboxClicked(LLUICtrl*, void* userdata)
{
	LLLiveLSLEditor* self = static_cast<LLLiveLSLEditor*>(userdata);
	self->mMonoCheckbox->setEnabled(have_script_upload_cap(self->mObjectUUID));
	self->mScriptEd->enableSave(self->getIsModifiable());
}
BOOL LLLiveLSLEditor::monoChecked() const
{
	if(NULL != mMonoCheckbox)
	{
		return mMonoCheckbox->getValue()? TRUE : FALSE;
	}
	return FALSE;
}
void LLLiveLSLEditor::setAssociatedExperience(LLHandle<LLLiveLSLEditor> editor, const LLSD& experience)
{
	LLLiveLSLEditor* scriptEd = editor.get();
	if (scriptEd)
	{
		LLUUID id;
		if (experience.has(LLExperienceCache::EXPERIENCE_ID))
		{
			id = experience[LLExperienceCache::EXPERIENCE_ID].asUUID();
		}
		scriptEd->mScriptEd->setAssociatedExperience(id);
		scriptEd->updateExperiencePanel();
	}
}
