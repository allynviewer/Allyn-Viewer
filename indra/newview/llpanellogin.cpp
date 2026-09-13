/** 
 * @file llpanellogin.cpp
 * @brief Login dialog and logo display
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
#include "llpanellogin.h"
#include "allynpresence.h"
#include "llpanelgeneral.h"
#include "hippogridmanager.h"
#include "indra_constants.h"
#include "llfontgl.h"
#include "llmd5.h"
#include "llsecondlifeurls.h"
#include "v4color.h"
#include "llappviewer.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcommandhandler.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llfloaterabout.h"
#include "llfloatertest.h"
#include "llfloaterpreference.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llurlhistory.h"
#include "llversioninfo.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewernetwork.h"
#include "llviewerwindow.h"
#include "llnotify.h"
#include "lluictrlfactory.h"
#include "llhttpclient.h"
#include "llweb.h"
#include "llmediactrl.h"
#include "llmenugl.h"
#include "llscrollcontainer.h"
#include "llfloatertos.h"
#include "llglheaders.h"
#include "rlvhandler.h"
#include "llspinctrl.h"
#include "llviewermessage.h"
#include <boost/algorithm/string.hpp>
#include "llsdserialize.h"
#include "llstring.h"
#include <cctype>
const S32 MAX_PASSWORD = 16;
static const S32 LOGIN_FORM_COLUMN_WIDTH = 300;
static const S32 LOGIN_VERSION_PAD = 6;
static const S32 LOGIN_VERSION_HEIGHT = 20;
static const S32 LOGIN_VERSION_RESERVE =
	LOGIN_VERSION_PAD + LOGIN_VERSION_HEIGHT + 8;
static const F32 LOGIN_SPLASH_DESIGN_WIDTH = 1400.f;
static const F32 LOGIN_SPLASH_DESIGN_HEIGHT = 850.f;
static void showLoginMenuBar()
{
	if (!gLoginMenuBarView)
		return;
	gLoginMenuBarView->setVisible(TRUE);
	gLoginMenuBarView->setEnabled(TRUE);
	if (LLView* parent = gLoginMenuBarView->getParent())
		parent->sendChildToFront(gLoginMenuBarView);
}
static S32 loginMenuBarHeight()
{
	if (gLoginMenuBarView && gLoginMenuBarView->getVisible())
		return llmax(MENU_BAR_HEIGHT, gLoginMenuBarView->getRect().getHeight());
	return MENU_BAR_HEIGHT;
}
static std::string formatarContagemLogin(S32 valor)
{
	if (valor < 0)
	{
		return "--";
	}
	std::string texto = llformat("%d", valor);
	std::string com_milhar;
	S32 digits = (S32)texto.size();
	for (S32 i = 0; i < digits; ++i)
	{
		if (i > 0 && ((digits - i) % 3) == 0)
		{
			com_milhar += ',';
		}
		com_milhar += texto[i];
	}
	return com_milhar;
}

static LLTextBox* textoLogin(LLView* form_bg, LLPanelLogin* self, const char* nome)
{
	LLTextBox* box = NULL;
	if (form_bg)
	{
		box = form_bg->getChild<LLTextBox>(nome, TRUE, FALSE);
	}
	if (!box && self)
	{
		box = self->getChild<LLTextBox>(nome, TRUE, FALSE);
	}
	return box;
}

static LLView* painelOnlineLogin(LLView* form_bg, LLPanelLogin* self)
{
	LLView* painel = NULL;
	if (form_bg)
	{
		painel = form_bg->getChildView("online_users_panel", TRUE, FALSE);
	}
	if (!painel && self)
	{
		painel = self->getChildView("online_users_panel", TRUE, FALSE);
	}
	return painel;
}

static void atualizarTextosOnlineLogin(LLPanelLogin* self, bool forcar = false)
{
	if (!self)
	{
		return;
	}
	static S32 ultimo_allyn = -999;
	static S32 ultimo_sl = -999;
	const S32 allyn_agora = AllynPresence::getAllynOnline();
	const S32 sl_agora = AllynPresence::getSecondLifeOnline();
	if (!forcar && allyn_agora == ultimo_allyn && sl_agora == ultimo_sl)
	{
		return;
	}
	ultimo_allyn = allyn_agora;
	ultimo_sl = sl_agora;
	LLView* form_bg = self->getChildView("login_form_bg", TRUE, FALSE);
	if (LLView* painel = painelOnlineLogin(form_bg, self))
	{
		painel->setVisible(TRUE);
		if (LLView* parent = painel->getParent())
		{
			parent->sendChildToFront(painel);
		}
	}
	if (LLTextBox* titulo = textoLogin(form_bg, self, "online_users_title"))
	{
		std::string rotulo_titulo = self->getString("online_users_title");
		if (rotulo_titulo.empty())
		{
			rotulo_titulo = "USUARIOS ONLINE";
		}
		titulo->setText(rotulo_titulo);
		titulo->setVisible(TRUE);
	}
	LLTextBox* allyn = textoLogin(form_bg, self, "online_allyn_text");
	LLTextBox* sl = textoLogin(form_bg, self, "online_sl_text");
	LLTextBox* allyn_qtd = textoLogin(form_bg, self, "online_allyn_count");
	LLTextBox* sl_qtd = textoLogin(form_bg, self, "online_sl_count");
	std::string rotulo_allyn = self->getString("online_allyn_label");
	std::string rotulo_sl = self->getString("online_sl_label");
	if (rotulo_allyn.empty())
	{
		rotulo_allyn = "ALLYN VIEWER";
	}
	if (rotulo_sl.empty())
	{
		rotulo_sl = "SECOND LIFE";
	}
	const std::string allyn_num = formatarContagemLogin(allyn_agora);
	const std::string sl_num = formatarContagemLogin(sl_agora);
	if (allyn)
	{
		allyn->setText(allyn_qtd ? rotulo_allyn : (rotulo_allyn + "  " + allyn_num));
		allyn->setVisible(TRUE);
	}
	if (allyn_qtd)
	{
		allyn_qtd->setText(allyn_num);
		allyn_qtd->setVisible(TRUE);
	}
	if (sl)
	{
		sl->setText(sl_qtd ? rotulo_sl : (rotulo_sl + "  " + sl_num));
		sl->setVisible(TRUE);
	}
	if (sl_qtd)
	{
		sl_qtd->setText(sl_num);
		sl_qtd->setVisible(TRUE);
	}
}

static void setChildRect(LLView* child, S32 left, S32 top, S32 width, S32 height)
{
	if (!child)
		return;
	LLRect r;
	r.setLeftTopAndSize(left, top, width, height);
	child->setRect(r);
	child->reshape(width, height, FALSE);
}
LLPanelLogin* LLPanelLogin::sInstance = NULL;
static bool nameSplit(const std::string& full, std::string& first, std::string& last)
{
	std::vector<std::string> fragments;
	boost::algorithm::split(fragments, full, boost::is_any_of(" ."));
	if (!fragments.size() || !fragments[0].length())
		return false;
	first = fragments[0];
	last = (fragments.size() == 1) ?
		gHippoGridManager->getCurrentGrid()->isWhiteCore() ? LLStringUtil::null : "Resident" :
		fragments[1];
	return (fragments.size() <= 2);
}
static std::string nameJoin(const std::string& first,const std::string& last, bool strip_resident)
{
	if (last.empty() || (strip_resident && boost::algorithm::iequals(last, "Resident")))
		return first;
	else if (std::islower(last[0]))
		return first + '.' + last;
	else
		return first + ' ' + last;
}
static std::string getDisplayString(const std::string& first, const std::string& last, const std::string& grid, bool is_secondlife)
{
	if (grid == gHippoGridManager->getDefaultGridName())
		return nameJoin(first, last, is_secondlife);
	else
		return nameJoin(first, last, is_secondlife) + " (" + grid + ')';
}
static std::string getDisplayString(const LLSavedLoginEntry& entry)
{
	return getDisplayString(entry.getFirstName(), entry.getLastName(), entry.getGrid(), entry.isSecondLife());
}
class LLLoginLocationAutoHandler : public LLCommandHandler
{
public:
	LLLoginLocationAutoHandler() : LLCommandHandler("location_login", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
		{
			if ( tokens.size() == 0 || tokens.size() > 4 )
				return false;
			const std::string region = LLURI::unescape( tokens[0].asString() );
			if ( tokens.size() == 1 )
			{
				LLSLURL slurl(region);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			if ( tokens.size() == 2 )
			{
				LLSLURL slurl(region);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			if ( tokens.size() == 3 )
			{
				F32 xpos;
				std::istringstream codec(tokens[1].asString());
				codec >> xpos;
				F32 ypos;
				codec.clear();
				codec.str(tokens[2].asString());
				codec >> ypos;
				const LLVector3 location(xpos, ypos, 0.0f);
				LLSLURL slurl(region, location);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			if ( tokens.size() == 4 )
			{
				F32 xpos;
				std::istringstream codec(tokens[1].asString());
				codec >> xpos;
				F32 ypos;
				codec.clear();
				codec.str(tokens[2].asString());
				codec >> ypos;
				F32 zpos;
				codec.clear();
				codec.str(tokens[3].asString());
				codec >> zpos;
				const LLVector3 location(xpos, ypos, zpos);
				LLSLURL slurl(region, location);
				LLPanelLogin::autologinToLocation(slurl);
			};
		}
		return true;
	}
};
LLLoginLocationAutoHandler gLoginLocationAutoHandler;
LLPanelLogin::LLPanelLogin(const LLRect& rect)
:	LLPanel(std::string("panel_login"), rect, FALSE),
	mLogoImage(LLUI::getUIImage("startup_logo.j2c")),
	mBrowserLayoutW(0),
	mBrowserLayoutH(0),
	mFormColumnWidth(300),
	mSplashColumnWidth(460),
	mLoginLayoutReady(false)
{
	setFocusRoot(TRUE);
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);
	LLPanelLogin::sInstance = this;
	gViewerWindow->getRootView()->addChildInBack(this);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_login.xml");
	reshape(rect.getWidth(), rect.getHeight());
	LLComboBox* username_combo(getChild<LLComboBox>("username_combo"));
	username_combo->setCommitCallback(boost::bind(LLPanelLogin::onSelectLoginEntry, _2));
	username_combo->setFocusLostCallback(boost::bind(&LLPanelLogin::onLoginComboLostFocus, this, username_combo));
	username_combo->setPrevalidate(LLLineEditor::prevalidatePrintableNotPipe);
	username_combo->setSuppressTentative(true);
	username_combo->setSuppressAutoComplete(true);
	username_combo->setButtonImages("icn_textfield_enabled.tga", "icn_textfield_enabled.tga");
	username_combo->setButtonOverlay("combobox_arrow.tga", LLFontGL::HCENTER,
		LLUI::sColorsGroup->getColor("LoginLabelColor"));
	username_combo->setButtonVisible(TRUE);
	if (LLLineEditor* username_entry = username_combo->getChild<LLLineEditor>("combo_text_entry", TRUE, FALSE))
	{
		username_entry->setVAlign(LLFontGL::VCENTER);
	}
	getChild<LLUICtrl>("remember_name_check")->setCommitCallback(boost::bind(&LLPanelLogin::onNameCheckChanged, this, _2));
	LLLineEditor* password_edit(getChild<LLLineEditor>("password_edit"));
	password_edit->setKeystrokeCallback(boost::bind(LLPanelLogin::onPassKey));
	password_edit->setCommitCallback(boost::bind(&LLPanelLogin::mungePassword, this, _2));
	password_edit->setDrawAsterixes(TRUE);
	password_edit->setVAlign(LLFontGL::VCENTER);
	getChild<LLUICtrl>("remove_login")->setCommitCallback(boost::bind(&LLPanelLogin::confirmDelete, this));
	if (LLView* splash = getChildView("login_html", TRUE, FALSE))
		sendChildToBack(splash);
	if (LLView* form_bg = getChildView("login_form_bg", TRUE, FALSE))
		sendChildToFront(form_bg);
	if (LLScrollContainer* scroll = findChild<LLScrollContainer>("login_form_scroll"))
	{
		scroll->setBorderVisible(FALSE);
		scroll->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
	}
	updateLoginVersionLabel();
	showLoginMenuBar();
	LLComboBox* location_combo = getChild<LLComboBox>("start_location_combo");
	updateLocationSelectorsVisibility();
	location_combo->setAllowTextEntry(TRUE, 128, FALSE);
	location_combo->setFocusLostCallback( boost::bind(&LLPanelLogin::onLocationSLURL, this) );
	location_combo->setButtonImages("icn_textfield_enabled.tga", "icn_textfield_enabled.tga");
	location_combo->setButtonOverlay("combobox_arrow.tga", LLFontGL::HCENTER,
		LLUI::sColorsGroup->getColor("LoginLabelColor"));
	LLComboBox* server_choice_combo = getChild<LLComboBox>("grids_combo");
	server_choice_combo->setCommitCallback(boost::bind(&LLPanelLogin::onSelectGrid, this, _1));
	server_choice_combo->setFocusLostCallback(boost::bind(&LLPanelLogin::onSelectGrid, this, server_choice_combo));
	server_choice_combo->setButtonImages("icn_textfield_enabled.tga", "icn_textfield_enabled.tga");
	server_choice_combo->setButtonOverlay("combobox_arrow.tga", LLFontGL::HCENTER,
		LLUI::sColorsGroup->getColor("LoginLabelColor"));
	if (LLView* grids_panel = getChildView("grids_panel", TRUE, FALSE))
		grids_panel->setVisible(FALSE);
	updateGridCombo();
	LLSLURL start_slurl(LLStartUp::getStartSLURL());
	if (!start_slurl.isSpatial())
	{
		std::string defaultStartLocation = gSavedSettings.getString("LoginLocation");
		LL_INFOS("AppInit")<<"default LoginLocation '" << defaultStartLocation << '\'' << LL_ENDL;
		LLSLURL defaultStart(defaultStartLocation);
		if ( defaultStart.isSpatial() )
		{
			LLStartUp::setStartSLURL(defaultStart);
		}
		else
		{
			LL_INFOS("AppInit")<<"no valid LoginLocation, using home"<<LL_ENDL;
			LLSLURL homeStart(LLSLURL::SIM_LOCATION_HOME);
			LLStartUp::setStartSLURL(homeStart);
		}
	}
	else
	{
		LLPanelLogin::onUpdateStartSLURL(start_slurl);
	}
	{
		LLButton* connect_btn(findChild<LLButton>("connect_btn"));
		if (connect_btn)
		{
			connect_btn->setCommitCallback(boost::bind(&LLPanelLogin::onClickConnect, this));
			setDefaultBtn(connect_btn);
			if (LLPanel* form_bg = findChild<LLPanel>("login_form_bg"))
				form_bg->setDefaultBtn(connect_btn);
			if (LLPanel* form_inner = findChild<LLPanel>("login_form_inner"))
				form_inner->setDefaultBtn(connect_btn);
			connect_btn->setScaleImage(TRUE);
		}
	}
	getChild<LLUICtrl>("grids_btn")->setCommitCallback(boost::bind(LLPanelLogin::onClickGrids));
	LLTextBox* forgot_password_text = getChild<LLTextBox>("forgot_password_text");
	forgot_password_text->setClickedCallback(boost::bind(&onClickForgotPassword));
	LLTextBox* create_new_account_text = getChild<LLTextBox>("create_new_account_text");
	create_new_account_text->setClickedCallback(boost::bind(&onClickNewAccount));
	if (LLMediaCtrl* web_browser = findChild<LLMediaCtrl>("login_html"))
	{
		web_browser->addObserver(this);
		web_browser->setBackgroundColor(LLColor4::black);
	}
	mLoginLayoutReady = true;
	reshapeBrowser();
	refreshLoginPage();
	AllynPresence::requestStatus();
	atualizarTextosOnlineLogin(this);
	gHippoGridManager->setCurrentGridChangeCallback(boost::bind(&LLPanelLogin::onCurGridChange,this,_1,_2));
	loadSavedLogins();
}
std::string LLPanelLogin::loginHistoryPath()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "saved_logins_sg2.xml");
}
void LLPanelLogin::loadSavedLogins()
{
	LLComboBox* username_combo = getChild<LLComboBox>("username_combo");
	if (!username_combo)
		return;
	username_combo->removeall();
	mLoginHistoryData = LLSavedLogins::loadFile(loginHistoryPath());
	const LLSavedLoginsList& saved_login_entries(mLoginHistoryData.getEntries());
	for (LLSavedLoginsList::const_reverse_iterator i = saved_login_entries.rbegin();
		 i != saved_login_entries.rend(); ++i)
	{
		const LLSD& e = i->asLLSD();
		if (e.isMap() && gHippoGridManager->getGrid(i->getGrid()))
			username_combo->add(getDisplayString(*i), e);
	}
	username_combo->setButtonVisible(TRUE);
	if (!saved_login_entries.empty())
	{
		setFields(*saved_login_entries.rbegin(), false);
	}
	addFavoritesToStartLocation();
}
void LLPanelLogin::saveSavedLogins()
{
	LLSavedLogins::saveFile(mLoginHistoryData, loginHistoryPath());
}
bool LLPanelLogin::hasLoginHistory()
{
	return sInstance && sInstance->mLoginHistoryData.size() > 0;
}
void LLPanelLogin::addFavoritesToStartLocation()
{
	auto combo = getChild<LLComboBox>("start_location_combo");
	if (!combo) return;
	S32 num_items = combo->getItemCount();
	for (S32 i = num_items - 1; i > 2; i--)
	{
		combo->remove(i);
	}
	const auto grid = gHippoGridManager->getCurrentGrid();
	std::string first, last, password;
	getFields(first, last, password);
	auto user_defined_name(first + ' ' + last);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites_" + grid->getGridName() + ".xml");
	std::string old_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	LLSD fav_llsd;
	llifstream file;
	file.open(filename);
	if (!file.is_open())
	{
		file.open(old_filename);
		if (!file.is_open()) return;
	}
	LLSDSerialize::fromXML(fav_llsd, file);
	for (LLSD::map_const_iterator iter = fav_llsd.beginMap();
		iter != fav_llsd.endMap(); ++iter)
	{
		S32 res = LLStringUtil::compareInsensitive(user_defined_name, iter->first);
		if (res != 0)
		{
			LL_DEBUGS() << "Skipping favorites for " << iter->first << LL_ENDL;
			continue;
		}
		combo->addSeparator();
		LL_DEBUGS() << "Loading favorites for " << iter->first << LL_ENDL;
		auto user_llsd = iter->second;
		for (LLSD::array_const_iterator iter1 = user_llsd.beginArray();
			iter1 != user_llsd.endArray(); ++iter1)
		{
			std::string label = (*iter1)["name"].asString();
			std::string value = (*iter1)["slurl"].asString();
			if (!label.empty() && !value.empty())
			{
				combo->add(label, value);
			}
		}
		break;
	}
}
void LLPanelLogin::setSiteIsAlive(bool alive)
{
	if (LLMediaCtrl* web_browser = findChild<LLMediaCtrl>("login_html"))
	{
		if (alive)
			loadLoginPage();
		else
			web_browser->navigateTo( "data:text/html,%3Chtml%3E%3Cbody%20bgcolor=%22%23050812%22%3E%3C/body%3E%3C/html%3E", "text/html" );
		web_browser->setVisible(TRUE);
	}
}
void LLPanelLogin::clearPassword()
{
	getChild<LLUICtrl>("password_edit")->setValue(mIncomingPassword = mMungedPassword = LLStringUtil::null);
}
void LLPanelLogin::hidePassword()
{
	getChild<LLUICtrl>("password_edit")->setValue("123456789!123456");
}
void LLPanelLogin::mungePassword(const std::string& password)
{
	if (password != mIncomingPassword)
	{
		if (password.length() == MD5HEX_STR_BYTES)
		{
			hidePassword();
			mMungedPassword = password;
		}
		else
		{
			LLMD5 pass((unsigned char *)utf8str_truncate(password, gHippoGridManager->getCurrentGrid()->isOpenSimulator() ? 24 : 16).c_str());
			char munged_password[MD5HEX_STR_SIZE];
			pass.hex_digest(munged_password);
			mMungedPassword = munged_password;
		}
		mIncomingPassword = password;
	}
}
void LLPanelLogin::reshapeBrowser()
{
	const S32 panel_width = getRect().getWidth();
	const S32 panel_height = getRect().getHeight();
	if (panel_width < 2 || panel_height < 2)
		return;
	mBrowserLayoutW = panel_width;
	mBrowserLayoutH = panel_height;
	mFormColumnWidth = llmin(LOGIN_FORM_COLUMN_WIDTH, llmax(1, panel_width / 3));
	mSplashColumnWidth = llmax(1, panel_width - mFormColumnWidth);
	const S32 menu_h = loginMenuBarHeight();
	const S32 content_top = llmax(1, panel_height - menu_h);
	const S32 content_h = content_top;
	if (LLView* form_bg = getChildView("login_form_bg", TRUE, FALSE))
	{
		form_bg->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
		form_bg->setMouseOpaque(TRUE);
		setChildRect(form_bg, 0, content_top, mFormColumnWidth, content_h);
		sendChildToFront(form_bg);
		if (LLScrollContainer* scroll = form_bg->findChild<LLScrollContainer>("login_form_scroll"))
		{
			const S32 scroll_h = llmax(1, content_h - LOGIN_VERSION_RESERVE);
			scroll->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
			setChildRect(scroll, 0, content_h, mFormColumnWidth, scroll_h);
			if (LLView* inner = scroll->getChildView("login_form_inner", FALSE, FALSE))
			{
				const S32 inner_h = inner->getRect().getHeight();
				inner->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_RIGHT);
				setChildRect(inner, 0, scroll_h, mFormColumnWidth, inner_h);
			}
			scroll->goToTop();
		}
	}
	if (LLMediaCtrl* web_browser = findChild<LLMediaCtrl>("login_html"))
	{
		web_browser->setFollowsNone();
		web_browser->setDecoupleTextureSize(false);
		web_browser->setTakeFocusOnClick(false);
		setChildRect(web_browser, mFormColumnWidth, content_top, mSplashColumnWidth, content_h);
		web_browser->setStretchToFill(true);
		web_browser->setMaintainAspectRatio(false);
		web_browser->setBorderVisible(false);
		sendChildToBack(web_browser);
		const LLVector2 ui_scale = LLUI::getScaleFactor();
		const F32 plugin_w = llmax(1.f, (F32)mSplashColumnWidth * ui_scale.mV[VX]);
		const F32 plugin_h = llmax(1.f, (F32)content_h * ui_scale.mV[VY]);
		const F32 zoom = llclamp(
			llmax(plugin_w / LOGIN_SPLASH_DESIGN_WIDTH, plugin_h / LOGIN_SPLASH_DESIGN_HEIGHT),
			0.5f,
			3.f);
		web_browser->setPageZoomOverride(zoom);
	}
	updateLoginVersionLabel();
}
void LLPanelLogin::updateLoginVersionLabel()
{
	LLView* form_bg = getChildView("login_form_bg", TRUE, FALSE);
	LLTextBox* ver = NULL;
	if (form_bg)
		ver = form_bg->getChild<LLTextBox>("channel_text", FALSE, FALSE);
	if (!ver)
		ver = getChild<LLTextBox>("channel_text", TRUE, FALSE);
	const S32 vw = llmax(1, mFormColumnWidth - LOGIN_VERSION_PAD * 2);
	if (ver)
	{
		ver->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
		setChildRect(ver, LOGIN_VERSION_PAD, LOGIN_VERSION_PAD + LOGIN_VERSION_HEIGHT, vw, LOGIN_VERSION_HEIGHT);
		ver->setHAlign(LLFontGL::HCENTER);
		ver->setText(LLVersionInfo::getChannelAndVersion());
		ver->setVisible(TRUE);
		ver->setEnabled(TRUE);
		if (form_bg)
			form_bg->sendChildToFront(ver);
	}

	atualizarTextosOnlineLogin(this, true);
}
void LLPanelLogin::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	if (!mLoginLayoutReady)
		return;
	if (width == mBrowserLayoutW && height == mBrowserLayoutH)
		return;
	static bool sInBrowserReshape = false;
	if (sInBrowserReshape)
		return;
	sInBrowserReshape = true;
	reshapeBrowser();
	sInBrowserReshape = false;
}
LLPanelLogin::~LLPanelLogin()
{
	saveSavedLogins();
	sInstance = nullptr;
	if (gFocusMgr.getDefaultKeyboardFocus() == this)
		gFocusMgr.setDefaultKeyboardFocus(nullptr);
}
void LLPanelLogin::draw()
{
	if (!mLoginLayoutReady)
	{
		LLPanel::draw();
		return;
	}
	S32 width = getRect().getWidth();
	S32 height = getRect().getHeight();
	if (width != mBrowserLayoutW || height != mBrowserLayoutH)
		reshapeBrowser();
	atualizarTextosOnlineLogin(this);
	LLColor4 form_bg = LLUI::sColorsGroup->getColor("LoginFormBgColor");
	const S32 menu_h = loginMenuBarHeight();
	gl_rect_2d(0, height - menu_h, mFormColumnWidth, 0, form_bg);
	gl_rect_2d(mFormColumnWidth, height - menu_h, width, 0, LLColor4::black);
	if (LLMediaCtrl* web_browser = findChild<LLMediaCtrl>("login_html"))
	{
		LLRect html_expected;
		html_expected.setLeftTopAndSize(mFormColumnWidth, height - menu_h, mSplashColumnWidth, height - menu_h);
		if (web_browser->getRect() != html_expected)
		{
			web_browser->setFollowsNone();
			setChildRect(web_browser, mFormColumnWidth, height - menu_h, mSplashColumnWidth, height - menu_h);
			web_browser->setStretchToFill(true);
			web_browser->setMaintainAspectRatio(false);
		}
	}
	LLPanel::draw();
	if (LLUI::sColorsGroup)
	{
		const S32 seam_x = mFormColumnWidth;
		const S32 seam_top = height - menu_h;
		LLColor4 edge = LLUI::sColorsGroup->getColor("LoginFormBorderColor");
		LLColor4 shadow = edge;
		shadow.mV[VALPHA] *= 0.55f;
		gl_line_2d(seam_x - 1, seam_top, seam_x - 1, 0, edge);
		gl_line_2d(seam_x, seam_top, seam_x, 0, shadow);
	}
}
bool LLPanelLogin::handleLoginAccelerators(KEY key, MASK mask)
{
	if (!sInstance || !sInstance->getVisible())
		return false;
	if (mask != MASK_CONTROL)
		return false;
	if (key == 'P' || key == 'p')
	{
		LLFloaterPreference::show(NULL);
		return true;
	}
	if ((key == 'Q' || key == 'q') && gLoginMenuBarView && gLoginMenuBarView->getVisible())
		return gLoginMenuBarView->handleAcceleratorKey(key, mask) != FALSE;
	return false;
}
BOOL LLPanelLogin::handleKeyHere(KEY key, MASK mask)
{
	if (handleLoginAccelerators(key, mask))
		return TRUE;
	if (('T' == key) && (MASK_CONTROL == mask))
	{
		new LLFloaterSimple("floater_test.xml");
		return TRUE;
	}
# if !LL_RELEASE_FOR_DOWNLOAD
	if ( KEY_F2 == key )
	{
		LL_INFOS() << "Spawning floater TOS window" << LL_ENDL;
		LLFloaterTOS* tos_dialog = LLFloaterTOS::show(LLFloaterTOS::TOS_TOS,LLStringUtil::null);
		tos_dialog->startModal();
		return TRUE;
	}
#endif
	return LLPanel::handleKeyHere(key, mask);
}
void LLPanelLogin::setFocus(BOOL b)
{
	if(b != hasFocus())
	{
		if(b)
		{
			LLPanelLogin::giveFocus();
		}
		else
		{
			LLPanel::setFocus(b);
		}
	}
}
void LLPanelLogin::giveFocus()
{
	if( sInstance )
	{
		if (!sInstance->getVisible()) sInstance->setVisible(true);
		std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
		std::string pass = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();
		BOOL have_username = !username.empty();
		BOOL have_pass = !pass.empty();
		LLLineEditor* edit = nullptr;
		LLComboBox* combo = nullptr;
		if (have_username && !have_pass)
		{
			edit = sInstance->getChild<LLLineEditor>("password_edit");
		}
		else
		{
			combo = sInstance->getChild<LLComboBox>("username_combo");
		}
		if (edit)
		{
			edit->setFocus(TRUE);
			edit->selectAll();
		}
		else if (combo)
		{
			combo->setFocus(TRUE);
		}
	}
}
void LLPanelLogin::show()
{
	if (sInstance) sInstance->setVisible(true);
	else new LLPanelLogin(gViewerWindow->getVirtualWindowRect());
	if (gLoginMenuBarView)
	{
		showLoginMenuBar();
	}
	if( !gFocusMgr.getKeyboardFocus() )
	{
		sInstance->setFocus(TRUE);
	}
	gFocusMgr.setDefaultKeyboardFocus(sInstance);
	AllynPresence::requestStatus();
	if (sInstance)
	{
		atualizarTextosOnlineLogin(sInstance);
	}
}
void LLPanelLogin::setFields(const std::string& firstname,
			     const std::string& lastname,
			     const std::string& password)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted fillFields with no login view shown" << LL_ENDL;
		return;
	}
	LLComboBox* login_combo = sInstance->getChild<LLComboBox>("username_combo");
	llassert_always(firstname.find(' ') == std::string::npos);
	login_combo->setLabel(nameJoin(firstname, lastname, false));
	sInstance->mungePassword(password);
	if (sInstance->mIncomingPassword != sInstance->mMungedPassword)
		sInstance->getChild<LLUICtrl>("password_edit")->setValue(password);
	else
		sInstance->hidePassword();
}
void LLPanelLogin::setFields(const LLSavedLoginEntry& entry, bool takeFocus)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted setFields with no login view shown" << LL_ENDL;
		return;
	}
	LLCheckBoxCtrl* remember_pass_check = sInstance->getChild<LLCheckBoxCtrl>("remember_check");
	std::string fullname = nameJoin(entry.getFirstName(), entry.getLastName(), entry.isSecondLife());
	LLComboBox* login_combo = sInstance->getChild<LLComboBox>("username_combo");
	login_combo->setSelectedByValue(entry.asLLSD(), TRUE);
	login_combo->setTextEntry(fullname);
	login_combo->resetTextDirty();
	const auto& grid = entry.getGrid();
	if (!grid.empty() && gHippoGridManager->getGrid(grid) && grid != gHippoGridManager->getCurrentGridName())
	{
		gHippoGridManager->setCurrentGrid(grid);
	}
	const auto& password = entry.getPassword();
	bool remember_pass = !password.empty();
	if (remember_pass)
	{
		sInstance->mIncomingPassword = sInstance->mMungedPassword = password;
		sInstance->hidePassword();
	}
	else sInstance->clearPassword();
	remember_pass_check->setValue(remember_pass);
	if (takeFocus) giveFocus();
}
void LLPanelLogin::getFields(std::string& firstname, std::string& lastname, std::string& password)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted getFields with no login view shown" << LL_ENDL;
		return;
	}
	nameSplit(sInstance->getChild<LLComboBox>("username_combo")->getTextEntry(), firstname, lastname);
	LLStringUtil::trim(firstname);
	LLStringUtil::trim(lastname);
	password = sInstance->mMungedPassword;
}
void LLPanelLogin::updateLocationSelectorsVisibility()
{
	if (sInstance)
	{
		BOOL show_start = gSavedSettings.getBOOL("ShowStartLocation");
	#ifndef RLV_EXTENSION_STARTLOCATION
		if (rlv_handler_t::isEnabled())
		{
			show_start = FALSE;
		}
	#endif
		sInstance->getChildView("location_panel")->setVisible(show_start);
	}
}
void LLPanelLogin::onUpdateStartSLURL(const LLSLURL& new_start_slurl)
{
	if (!sInstance) return;
	LL_DEBUGS("AppInit")<<new_start_slurl.asString()<<LL_ENDL;
	auto location_combo = sInstance->getChild<LLComboBox>("start_location_combo");
	enum LLSLURL::SLURL_TYPE new_slurl_type = new_start_slurl.getType();
	switch (new_slurl_type)
	{
	case LLSLURL::LOCATION:
	{
		location_combo->setCurrentByIndex(2);
		location_combo->setTextEntry(new_start_slurl.getLocationString());
	}
	break;
	case LLSLURL::HOME_LOCATION:
		location_combo->setCurrentByIndex(0);
		break;
	case LLSLURL::LAST_LOCATION:
		location_combo->setCurrentByIndex(1);
		break;
	default:
		LL_WARNS("AppInit")<<"invalid login slurl, using home"<<LL_ENDL;
		location_combo->setCurrentByIndex(1);
		break;
	}
	updateLocationSelectorsVisibility();
}
void LLPanelLogin::setLocation(const LLSLURL& slurl)
{
	LL_DEBUGS("AppInit")<<"setting Location "<<slurl.asString()<<LL_ENDL;
	LLStartUp::setStartSLURL(slurl);
}
void LLPanelLogin::autologinToLocation(const LLSLURL& slurl)
{
	LL_DEBUGS("AppInit")<<"automatically logging into Location "<<slurl.asString()<<LL_ENDL;
	LLStartUp::setStartSLURL(slurl);
	if ( LLPanelLogin::sInstance != NULL )
	{
		LLPanelLogin::sInstance->onClickConnect();
	}
}
void LLPanelLogin::close()
{
	if (sInstance)
	{
		sInstance->getParent()->removeChild(sInstance);
		delete sInstance;
		sInstance = nullptr;
	}
}
void LLPanelLogin::setAlwaysRefresh(bool refresh)
{
	if (sInstance && LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
		if (LLMediaCtrl* web_browser = sInstance->findChild<LLMediaCtrl>("login_html"))
			web_browser->setAlwaysRefresh(refresh);
}
void LLPanelLogin::updateGridCombo()
{
	const std::string& defaultGrid = gHippoGridManager->getDefaultGridName();
	LLComboBox* grids = getChild<LLComboBox>("grids_combo");
	std::string top_entry;
	grids->removeall();
	const HippoGridInfo* curGrid = gHippoGridManager->getCurrentGrid();
	const HippoGridInfo* defGrid = gHippoGridManager->getGrid(defaultGrid);
	S32 idx(-1);
	HippoGridManager::GridIterator it, end = gHippoGridManager->endGrid();
	for (it = gHippoGridManager->beginGrid(); it != end; ++it)
	{
		std::string grid = it->second->getGridName();
		if (grid.empty() || it->second == defGrid)
			continue;
		if (it->second == curGrid) idx = grids->getItemCount();
		grids->add(grid);
	}
	if (curGrid || defGrid)
	{
		if (defGrid)
		{
			grids->add(defGrid->getGridName(), ADD_TOP);
			++idx;
		}
		grids->setCurrentByIndex(idx);
	}
	else
	{
		grids->setLabel(LLStringUtil::null);
	}
}
void LLPanelLogin::loadLoginPage()
{
	if (!sInstance) return;
 	sInstance->updateGridCombo();
	std::string login_page_str = gHippoGridManager->getCurrentGrid()->getLoginPage();
	if (login_page_str.empty())
	{
		sInstance->setSiteIsAlive(false);
		return;
	}
	LLURI login_page = LLURI(login_page_str);
	LLSD params(login_page.queryMap());
	LL_DEBUGS("AppInit") << "login_page: " << login_page << LL_ENDL;
	params["lang"] = LLUI::getLanguage();
	if (gSavedSettings.getBOOL("FirstLoginThisInstall"))
	{
		params["firstlogin"] = "TRUE";
	}
	params["version"] = llformat("%s (%d)",
								 LLVersionInfo::getShortVersion().c_str(),
								 LLVersionInfo::getBuild());
	params["channel"] = LLVersionInfo::getChannel();
	if (gHippoGridManager->getCurrentGrid()->isSecondLife())
	{
		std::string tmp = gHippoGridManager->getCurrentGrid()->getLoginUri();
		int i = tmp.find(".lindenlab.com");
		if (i != std::string::npos) {
			tmp = tmp.substr(0, i);
			i = tmp.rfind('.');
			if (i == std::string::npos)
				i = tmp.rfind('/');
			if (i != std::string::npos) {
				tmp = tmp.substr(i+1);
				params["grid"] = tmp;
			}
		}
	}
	else if (gHippoGridManager->getCurrentGrid()->isOpenSimulator())
	{
		params["grid"] = gHippoGridManager->getCurrentGrid()->getGridNick();
	}
	else if (gHippoGridManager->getCurrentGrid()->getPlatform() == HippoGridInfo::PLATFORM_WHITECORE)
	{
		params["grid"] = LLViewerLogin::getInstance()->getGridLabel();
	}
	params["os"] = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
	sInstance->reshapeBrowser();
	params["splash_w"] = sInstance->mSplashColumnWidth;
	params["splash_h"] = sInstance->mBrowserLayoutH;
	params["viewer_embed"] = "1";
	auto&& uri_with_params = [](const LLURI& uri, const LLSD& params) {
		return LLURI(uri.scheme(), uri.userName(), uri.password(), uri.hostName(), uri.hostPort(), uri.path(),
			LLURI::mapToQueryString(params));
	};
	LLURI login_uri(uri_with_params(login_page, params));
	gViewerWindow->setMenuBackgroundColor(false, !LLViewerLogin::getInstance()->isInProductionGrid());
	gLoginMenuBarView->setBackgroundColor(gMenuBarView->getBackgroundColor());
	std::string Allyn_splash_uri = gSavedSettings.getString("AllynSplashPagePrefix");
	if (!Allyn_splash_uri.empty())
	{
		params["original_page"] = login_uri.asString();
		std::string splash_path = gSavedSettings.getString("AllynSplashPagePath");
		if (!splash_path.empty() && splash_path.back() != '/')
		{
			splash_path += '/';
		}
		splash_path += LLUI::getLanguage();
		login_uri = LLURI(Allyn_splash_uri + splash_path);
		auto& params_map = params.map();
		for (auto&& pair : login_uri.queryMap().map())
			params_map.emplace(pair);
		login_uri = uri_with_params(login_uri, params);
	}
	LLMediaCtrl* web_browser = sInstance->findChild<LLMediaCtrl>("login_html");
	if (web_browser && web_browser->getCurrentNavUrl() != login_uri.asString())
	{
		LL_DEBUGS("AppInit") << "loading:    " << login_uri << LL_ENDL;
		web_browser->navigateTo( login_uri.asString(), "text/html" );
	}
}
void LLPanelLogin::handleMediaEvent(LLPluginClassMedia* , EMediaEvent event)
{
	if (event == MEDIA_EVENT_NAVIGATE_COMPLETE || event == MEDIA_EVENT_SIZE_CHANGED)
	{
		reshapeBrowser();
	}
}
void LLPanelLogin::onClickConnect()
{
	gFocusMgr.setKeyboardFocus(NULL);
	std::string first, last;
	if (nameSplit(getChild<LLComboBox>("username_combo")->getTextEntry(), first, last))
		LLStartUp::setStartupState(STATE_LOGIN_CLEANUP);
	else if (gHippoGridManager->getCurrentGrid()->getRegisterUrl().empty())
		LLNotificationsUtil::add("MustHaveAccountToLogInNoLinks");
	else
		LLNotificationsUtil::add("MustHaveAccountToLogIn", LLSD(), LLSD(),
										LLPanelLogin::newAccountAlertCallback);
}
bool LLPanelLogin::newAccountAlertCallback(const LLSD& notification, const LLSD& response)
{
	if (0 == LLNotification::getSelectedOption(notification, response))
	{
		LL_INFOS() << "Going to account creation URL" << LL_ENDL;
		LLWeb::loadURLExternal(CREATE_ACCOUNT_URL);
	}
	return false;
}
void LLPanelLogin::onClickNewAccount()
{
	const std::string& url = gHippoGridManager->getCurrentGrid()->getRegisterUrl();
	if (!url.empty())
	{
		LL_INFOS() << "Going to account creation URL." << LL_ENDL;
		LLWeb::loadURLExternal(url);
	}
	else
	{
		LL_INFOS() << "Account creation URL is empty." << LL_ENDL;
	}
}
void LLPanelLogin::onClickGrids()
{
	LLFloaterPreference::show(NULL);
	LLFloaterPreference::switchTab(LLPreferenceCore::TAB_GRIDS);
}
void LLPanelLogin::onClickForgotPassword()
{
	const std::string& url = gHippoGridManager->getCurrentGrid()->getPasswordUrl();
	if (!url.empty())
		LLWeb::loadURLExternal(url);
	else
		LL_WARNS() << "Link for 'forgotton password' not set." << LL_ENDL;
}
void LLPanelLogin::onPassKey()
{
	static bool sCapslockDidNotification = false;
	if (gKeyboard->getKeyDown(KEY_CAPSLOCK) && sCapslockDidNotification == false)
	{
		LLNotificationsUtil::add("CapsKeyOn");
		sCapslockDidNotification = true;
	}
}
void LLPanelLogin::onCurGridChange(HippoGridInfo* new_grid, HippoGridInfo* old_grid)
{
	refreshLoginPage();
	if (old_grid != new_grid)
	{
		std::string defaultStartLocation = gSavedSettings.getString("LoginLocation");
		LLSLURL defaultStart(defaultStartLocation);
		LLStartUp::setStartSLURL(defaultStart.isSpatial() ? defaultStart : LLSLURL(LLSLURL::SIM_LOCATION_HOME));
	}
}
void LLPanelLogin::refreshLoginPage()
{
	if (!sInstance || (LLStartUp::getStartupState() >= STATE_LOGIN_CLEANUP))
		 return;
	sInstance->updateGridCombo();
	sInstance->getChildView("create_new_account_text")->setVisible(!gHippoGridManager->getCurrentGrid()->getRegisterUrl().empty());
	sInstance->getChildView("forgot_password_text")->setVisible(!gHippoGridManager->getCurrentGrid()->getPasswordUrl().empty());
	std::string login_page = gHippoGridManager->getCurrentGrid()->getLoginPage();
	if (!login_page.empty())
	{
		LLMediaCtrl* web_browser = sInstance->findChild<LLMediaCtrl>("login_html");
		if (web_browser && web_browser->getCurrentNavUrl() != login_page)
		{
			sInstance->setSiteIsAlive(true);
		}
	}
	else
	{
		sInstance->setSiteIsAlive(false);
	}
}
void LLPanelLogin::onSelectGrid(LLUICtrl *ctrl)
{
	std::string grid(ctrl->getValue().asString());
	LLStringUtil::trim(grid);
	if (!gHippoGridManager->getGrid(grid))
	{
		HippoGridInfo* info(new HippoGridInfo(LLStringUtil::null));
		info->setLoginUri(grid);
		try
		{
			info->getGridInfo();
			grid = info->getGridName();
			if (HippoGridInfo* nick_info = gHippoGridManager->getGrid(info->getGridNick()))
			{
				delete info;
				grid = nick_info->getGridName();
			}
			else
			{
				gHippoGridManager->addGrid(info);
			}
		}
		catch(AIAlert::ErrorCode const& error)
		{
			std::string::size_type pos1 = grid.find('.');
			std::string::size_type pos2 = grid.find_last_of(".:");
			if (grid.substr(0, 4) == "http" || (pos1 != std::string::npos && pos1 != pos2))
			{
				if (error.getCode() == HTTP_METHOD_NOT_ALLOWED || error.getCode() == HTTP_OK)
				{
					AIAlert::add("GridInfoError", error);
				}
				else
				{
					AIAlert::add("GridInfoError", AIAlert::Error(AIAlert::Prefix(), AIAlert::not_modal, error, "GridInfoErrorInstruction"));
				}
			}
			delete info;
			grid = gHippoGridManager->getCurrentGridName();
		}
	}
	gHippoGridManager->setCurrentGrid(grid);
	ctrl->setValue(grid);
	addFavoritesToStartLocation();
	auto location_combo = getChild<LLComboBox>("start_location_combo");
	S32 index = location_combo->getCurrentIndex();
	switch (index)
	{
	case 0:
	case 1:
		break;
	default:
		{
			std::string location = location_combo->getValue().asString();
			LLSLURL slurl(location);
			if (   slurl.getType() == LLSLURL::LOCATION
				&& slurl.getGrid() != gHippoGridManager->getCurrentGridNick()
				)
			{
				location_combo->setCurrentByIndex(0);
				location_combo->setTextEntry(LLStringUtil::null);
			}
		}
		break;
	}
}
void LLPanelLogin::onLocationSLURL()
{
	auto location_combo = getChild<LLComboBox>("start_location_combo");
	std::string location = location_combo->getValue().asString();
	LLStringUtil::trim(location);
	LL_DEBUGS("AppInit")<<location<<LL_ENDL;
	LLStartUp::setStartSLURL(location);
}
void LLPanelLogin::onSelectLoginEntry(const LLSD& selected_entry)
{
	if (selected_entry.isMap())
		setFields(LLSavedLoginEntry(selected_entry));
	LLViewerLogin::getInstance()->setNameEditted(true);
	sInstance->addFavoritesToStartLocation();
}
void LLPanelLogin::onLoginComboLostFocus(LLComboBox* combo_box)
{
	if (combo_box->isTextDirty())
	{
		clearPassword();
		combo_box->resetTextDirty();
	}
}
void LLPanelLogin::onNameCheckChanged(const LLSD& value)
{
	if (LLCheckBoxCtrl* remember_pass_check = findChild<LLCheckBoxCtrl>("remember_check"))
	{
		if (value.asBoolean())
		{
			remember_pass_check->setEnabled(true);
		}
		else
		{
			remember_pass_check->setValue(LLSD(false));
			remember_pass_check->setEnabled(false);
		}
	}
}
void LLPanelLogin::confirmDelete()
{
	LLNotificationsUtil::add("ConfirmDeleteUser", LLSD(), LLSD(), boost::bind(&LLPanelLogin::removeLogin, this, boost::bind(LLNotificationsUtil::getSelectedOption, _1, _2)));
}
void LLPanelLogin::removeLogin(bool knot)
{
	if (knot) return;
	LLComboBox* combo(getChild<LLComboBox>("username_combo"));
	const std::string label(combo->getTextEntry());
	if (combo->isTextDirty() || !combo->itemExists(label)) return;
	const LLSD& selected = combo->getSelectedValue();
	if (!selected.isUndefined())
	{
		mLoginHistoryData.deleteEntry(selected.get("firstname").asString(), selected.get("lastname").asString(), selected.get("grid").asString());
		combo->remove(label);
		saveSavedLogins();
		if (combo->selectFirstItem())
		{
			onSelectLoginEntry(combo->getSelectedValue());
		}
		else
		{
			combo->setTextEntry(LLStringUtil::null);
			clearPassword();
		}
	}
}
